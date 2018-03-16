#include <stdlib.h>
#include <debug.h>
#include <video.h>
#define EFUSE_PART_NAME		"efuse"
#define CHK_SIZE			20
#define CHK_THRESHOLD		19

#define EFUSE_NONE			0x00
#define EFUSE_FINE			0xAA
#define EFUSE_BROKEN		0x5A
#define EFUSE_REBLOW		0x55

extern int mboot_recovery_load_raw_part(char *part_name, unsigned long *addr, unsigned int size);


static int mboot_efuse_load_misc(unsigned char *misc_addr, unsigned int size)
{
	int ret;

	dprintf(SPEW, "[%s]: size is %u\n", __func__, size);
	dprintf(SPEW, "[%s]: misc_addr is 0x%p\n", __func__, misc_addr);

	ret = mboot_recovery_load_raw_part(EFUSE_PART_NAME, (unsigned long *) misc_addr, size);

	return ret;
}
#ifdef MTK_EFUSE_WRITER_SUPPORT
unsigned int mt_efuse_get(void)
{
	unsigned char *data;
	unsigned char efuse = EFUSE_NONE;
	char *str = "";
	int ret, i, cnt;

	data = malloc(CHK_SIZE);
	if (!data) {
		dprintf(CRITICAL, "%s: malloc failed\n", __func__);
		goto fail;
	}

	ret = mboot_efuse_load_misc(data, CHK_SIZE);
	if (ret < 0) {
		dprintf(CRITICAL, "%s: read partition failed\n", __func__);
		goto fail;
	}

	switch (data[0]) {
	case EFUSE_FINE:
	case EFUSE_BROKEN:
	case EFUSE_REBLOW:
		efuse = data[0];
	break;
	}

	if (efuse != EFUSE_NONE) {
		for (i = cnt = 1; i < CHK_SIZE; i++) {
			if (data[i] == efuse)
				cnt++;
		}

		if (cnt >= CHK_THRESHOLD) {
			if (efuse == EFUSE_FINE)
				str = "success";
			else if (efuse == EFUSE_BROKEN)
				str = "broken";
			else if (efuse == EFUSE_REBLOW)
				str = "reblow";

			video_printf("efuse blow: %s\n", str);
		}
	}

fail:
	if (data)
		free(data);
	return (unsigned int)efuse;
}
#else
unsigned int mt_efuse_get(void)
{
	return 0;
}
#endif
