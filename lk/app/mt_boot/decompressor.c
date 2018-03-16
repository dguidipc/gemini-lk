#include <lib/zlib.h>
#include <string.h>
#include <debug.h>
#include <malloc.h>

#define RESERVED		0xe0
#define COMMENT			0x10
#define ORIG_NAME		8
#define EXTRA_FIELD		4
#define HEAD_CRC		2
#define	ZALLOC_ALIGNMENT	16

static void *zlib_alloc(void *x, unsigned items, unsigned size)
{
	void *p;

	size *= items;
	size = (size + ZALLOC_ALIGNMENT - 1) & ~(ZALLOC_ALIGNMENT - 1);

	p = malloc (size);
 	return (p);
}

static void zlib_free(void *x, void *addr)
{
	free (addr);
}


int zunzip(unsigned char *src, unsigned long *lenp, void *dst, int dstlen, int offset)
{
	z_stream s;
	int r;

    memset(&s, 0, sizeof(z_stream));

	r = inflateInit2(&s, -MAX_WBITS);
	if (r != Z_OK) {
		dprintf (CRITICAL, "inflateInit2() error, returned %d\n", r);
		return -1;
	}
	s.zalloc = zlib_alloc;
	s.zfree = zlib_free;
	s.next_in = src + offset;
	s.next_out = dst;
	s.avail_in = *lenp - offset;
	s.avail_out = dstlen;
	r = inflate(&s, Z_FINISH);
	if ((r != Z_STREAM_END)) {
		dprintf(CRITICAL, "inflate() error! return %d\n", r);
		inflateEnd(&s);
		return -1;
	}
	*lenp = s.next_out - (unsigned char *) dst;
	inflateEnd(&s);

	return 0;
}

int gunzip(unsigned char *src, unsigned long *lenp, void *dst, int dstlen)
{
	unsigned long i, flags;
	int ret;

	i = 10;
	flags = src[3];

	/* Skip the header of gzip */
	if (src[2] != Z_DEFLATED || (flags & RESERVED) != 0) {
		dprintf (CRITICAL, "gzip header is not correct. Error!\n");
		return -1;
	}
	if ((flags & EXTRA_FIELD) != 0)
		i = 12 + src[10] + (src[11] << 8);
	if ((flags & ORIG_NAME) != 0)
		while (src[i++] != 0)
			;
	if ((flags & COMMENT) != 0)
		while (src[i++] != 0)
			;
	if ((flags & HEAD_CRC) != 0)
		i += 2;

	if (i >= *lenp) {
		dprintf(CRITICAL, "gunzip header is bigger than expectation! Error!\n");
		return -1;
	}

    ret = zunzip(src, lenp, dst, dstlen, i);

	return ret;
}


bool decompress_kernel(unsigned char *in, void *out, int inlen, int outlen)
{
	unsigned long lenp = inlen;
	int ret;
#ifdef LK_PROFILING
	unsigned int time_decompress = get_timer(0);
#endif

	ret = gunzip(in, &lenp, out, outlen);

#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- decompress_kernel takes %d ms -------- \n", (int)get_timer(time_decompress));
#endif
	return ret;
}

