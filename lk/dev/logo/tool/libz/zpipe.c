/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler */

/* Version history:
   1.0  30 Oct 2004  First version
   1.1   8 Nov 2004  Add void casting for unused return values
   Use switch statement for inflate() return values
   1.2   9 Nov 2004  Add assertions to document zlib guarantees
   1.3   6 Apr 2005  Remove incorrect assertion in inf()
   1.4  11 Dec 2005  Add hack to avoid MSDOS end-of-line conversions
   Avoid some compiler warnings for input and output buffers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, level);
	if (ret != Z_OK)
		return ret;

	/* compress until end of file */
	do 
	{
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) 
		{
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}
		flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		/* run deflate() on input until output buffer not full, finish
		   compression if all of source has been read in */
		do 
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) 
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
	int ret;
	unsigned int have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	do 
	{
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		/* run inflate() on input until output buffer not full */
		do 
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) 
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}

			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) 
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
	fputs("zpipe: ", stderr);
	switch (ret) 
	{
		case Z_ERRNO:
			if (ferror(stdin))
				fputs("error reading stdin\n", stderr);
			if (ferror(stdout))
				fputs("error writing stdout\n", stderr);
			break;
		case Z_STREAM_ERROR:
			fputs("invalid compression level\n", stderr);
			break;
		case Z_DATA_ERROR:
			fputs("invalid or incomplete deflate data\n", stderr);
			break;
		case Z_MEM_ERROR:
			fputs("out of memory\n", stderr);
			break;
		case Z_VERSION_ERROR:
			fputs("zlib version mismatch!\n", stderr);
	}
}

/* compress or decompress from stdin to stdout */
int process(int argc, char **argv)
{
	int ret;

	/* avoid end-of-line conversions */
	SET_BINARY_MODE(stdin);
	SET_BINARY_MODE(stdout);

	/* do compression if no arguments */
	if (argc == 1) 
	{
		ret = def(stdin, stdout, Z_DEFAULT_COMPRESSION);
		if (ret != Z_OK)
			zerr(ret);
		return ret;
	}

	/* do decompression if -d specified */
	else if (argc == 2 && strcmp(argv[1], "-d") == 0) 
	{
		ret = inf(stdin, stdout);
		if (ret != Z_OK)
			zerr(ret);
		return ret;
	}

	/* otherwise, report usage */
	else 
	{
		fputs("zpipe usage: zpipe [-d] < source > dest\n", stderr);
		return 1;
	}
}

int main(int argc, char **argv)
{
	int ret = 0;
	int i;
	FILE *input, *output;
	if (argc < 4 || (strcmp(argv[1], "-l") && strcmp(argv[1], "-d"))) 
	{
		printf("[Usage] compress -l n output input1 [input2]\n");
		printf("Example: compress -l 9 logo.raw battery.raw\n");
		return -1;
	}

	// compress
	if(!strcmp(argv[1], "-l"))
	{
		char *ptemp;
		unsigned int *pinfo;
		int level = -1;
		int filenum = argc - 2;
		level = atoi(argv[2]);

		input = fopen(argv[4], "rb");
		output = fopen(argv[3], "wb");
		ptemp = (char*)malloc(1024*1024);
		pinfo = (unsigned int*)malloc(filenum*sizeof(int));
		if(input < 0 || output < 0 || ptemp == NULL || pinfo == NULL)
		{
			fprintf(stderr, "open file and allocate temp buffer fail\n");
			fprintf(stderr, "input = %d, output = %d, ptemp = 0x%08x, pinfo = 0x%08x\n", input, output, ptemp, pinfo);
			ret = -1;
			goto done;
		}

		// structre of pinfo:
		// pinfo[0] ==> size of pinfo 
		// pinfo[1] ==> offset of zip chunk[1]
		// pinfo[2] ==> offset of zip chunk[2]
		// ...
		// pinfo[n] ==> offset of zip chunk[n]
		// "offset" is the distance from the begining of the file, not the zip chunk
		memset((void*)pinfo, 0, sizeof(int)*filenum);
		// write information header to output first
		if(sizeof(int)*filenum != fwrite(pinfo, 1, filenum*sizeof(int), output))
		{
			ret = -2;
			goto done;
		}

		pinfo[pinfo[0]+2]=ftell(output);
		if (Z_OK != def(input, output, level)) 
		{
			ret = -2;
			goto done;
		}
		pinfo[0] = pinfo[0]+1;

		for (i = 5; i < argc; i++) 
		{
			fclose(input);

			input = fopen(argv[i], "rb");
			pinfo[pinfo[0]+2]=ftell(output);
			if (Z_OK != def(input, output, level)) 
			{
				fprintf(stderr, "compress error\n");
				ret = -2;
				goto done;
			}
			pinfo[0] = pinfo[0]+1;
		}

done:
		fseek(output, 0L, SEEK_END);
		pinfo[1] = ftell(output);
		fseek(output, 0L, SEEK_SET);
		fwrite(pinfo, 1, filenum*sizeof(int), output);

		fclose(input);
		fclose(output);
		free(ptemp);
		free(pinfo);

		return ret;
	}
	// decompress
	else
	{
		unsigned int temp;
		unsigned int *pinfo;
		char outputfilename[256];
		input = fopen(argv[3], "rb");
		if(input < 0)
		{
			printf("open file fail\n");
			printf("input = %d\n", input);
			ret = -1;
			goto done2;
		}

		fread(&temp, 1, sizeof(int), input);
		printf("temp=%d\n", temp);
		pinfo = malloc(temp*sizeof(int));
		if(pinfo == NULL)
		{
			printf("allocate pinfo failed\n");
			ret = -1;
			goto done2;
		}

		fread(pinfo, 1, temp*sizeof(int), input);

		for(i=0;i<temp;i++)
			printf("pinfo[%d]=%d\n", i, pinfo[i]);

		for(i = 0;i < temp;i++)
		{
			sprintf(outputfilename, "%d_%s", i, argv[2]);
			output = fopen(outputfilename, "wb");
			if(output < 0)
			{
				printf("create output file %s fail\n", outputfilename);
				goto done2;
			}

			fseek(input, pinfo[i], SEEK_SET);
			if(Z_OK != inf(input, output))
			{
				printf("decompress error\n");
				ret = -2;
				goto done2;
			}

			fclose(output);
			output = 0;
		}
done2:
		fclose(input);
		if(output) fclose(output);
		return ret;
	}
}
