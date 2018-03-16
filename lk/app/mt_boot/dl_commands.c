/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <stdlib.h>
#include <app.h>
#include <debug.h>
#include <arch/arm.h>
#include <dev/udc.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <arch/ops.h>
#include <target.h>
#include <platform.h>
#include <platform/mt_reg_base.h>
#include <platform/boot_mode.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_rtc.h>
#include <platform/bootimg.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#if !defined(MTK_EMMC_SUPPORT) && !defined(MTK_UFS_BOOTING)
#include <platform/mtk_nand.h>
#endif

/*For gpt update*/
#ifdef MTK_GPT_SCHEME_SUPPORT
#ifdef PLATFORM_FASTBOOT_EMPTY_STORAGE
extern part_t *partition;
extern int gpt_partition_table_update(const char *arg, void *data, unsigned sz);
extern int read_gpt(part_t *part);
#endif
#endif

/*For image write*/
#include "sparse_format.h"
#include "dl_commands.h"
#include <platform/mmc_core.h>
#include <platform/mt_gpt.h>
#include <platform/errno.h>
#ifdef FASTBOOT_WHOLE_FLASH_SUPPORT
#include "partition_parser.h"
#endif

#include "fastboot.h"
#include "mt_pmic.h"
#include "blockheader.h"

#define MODULE_NAME "FASTBOOT_DOWNLOAD"
#define MAX_RSP_SIZE 64

extern void *download_base;
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
extern unsigned long long download_max;
#else
extern unsigned download_max;
#endif
extern unsigned download_size;
extern unsigned fastboot_state;

/*LXO: !Download related command*/

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
#define INVALID_PTN -1
//For test: Display info on boot screen
#define DISPLAY_INFO_ON_LCM
#if (defined(MTK_UFS_BOOTING) || defined(MTK_EMMC_SUPPORT))  //use another macro.
#define EMMC_TYPE
#else
#define NAND_TYPE
#endif
#if defined(MTK_SPI_NOR_SUPPORT)
#define NOR_TYPE
#endif
extern void video_printf (const char *fmt, ...);
extern int video_get_rows(void);
extern void video_set_cursor(int row, int col);
extern void video_clean_screen(void);
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
extern int nand_write_img(u64 addr, void *data, u32 img_sz,u64 partition_size,int partition_type);
extern int nand_write_img_ex(u64 addr, void *data, u32 length,u64 total_size, u32 *next_offset, u64 partition_start,u64 partition_size, int img_type);
#else
extern int nand_write_img(u32 addr, void *data, u32 img_sz,u32 partition_size,int partition_type);
extern int nand_write_img_ex(u32 addr, void *data, u32 length,u32 total_size, u32 *next_offset, u32 partition_start,u32 partition_size, int img_type);
#endif
extern u32 gpt4_tick2time_ms (u32 tick);

unsigned start_time_ms;
#define TIME_STAMP gpt4_tick2time_ms(gpt4_get_current_tick())
#define TIME_START {start_time_ms = gpt4_tick2time_ms(gpt4_get_current_tick());}
#define TIME_ELAPSE (gpt4_tick2time_ms(gpt4_get_current_tick()) - start_time_ms)

extern int usb_write(void *buf, unsigned len);
extern int usb_read(void *buf, unsigned len);


extern int sec_dl_permission_chk(const char *part_name, unsigned int *permitted);
extern int sec_format_permission_chk(const char *part_name, unsigned int *permitted);

/* todo: give lk strtoul and nuke this */
static unsigned hex2unsigned(const char *x)
{
	unsigned n = 0;

	while (*x) {
		switch (*x) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				n = (n << 4) | (*x - '0');
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				n = (n << 4) | (*x - 'a' + 10);
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				n = (n << 4) | (*x - 'A' + 10);
				break;
			default:
				return n;
		}
		x++;
	}

	return n;
}

static void init_display_xy()
{
#if defined(DISPLAY_INFO_ON_LCM)
	video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	//video_set_cursor(1, 0);
#endif
}

static void display_info(const char* msg)
{
#if defined(DISPLAY_INFO_ON_LCM)
	if (msg == 0) {
		return;
	}
	video_printf("%s\n", msg);
#endif
}

static void display_progress(const char* msg_prefix, unsigned size, unsigned totle_size)
{
#if defined(DISPLAY_INFO_ON_LCM)
	unsigned vel = 0;
	u64 prog = 0;
	unsigned time = TIME_ELAPSE;


	if (msg_prefix == 0) {
		msg_prefix = "Unknown";
	}

	if (time != 0) {
		vel = (unsigned)(size / time); //approximate  1024/1000
		time /= 1000;
	}
	if (totle_size != 0) {
		prog = (u64)size*100/totle_size;
	}
	video_printf("%s > %3d%% Time:%4ds Vel:%5dKB/s", msg_prefix, (unsigned)prog, time, vel);
#endif
}

static void display_speed_info(const char* msg_prefix, unsigned size)
{
#if defined(DISPLAY_INFO_ON_LCM)
	unsigned vel = 0;
	unsigned time = TIME_ELAPSE;

	if (msg_prefix == 0) {
		msg_prefix = "Unknown";
	}

	if (time != 0) {
		vel = (unsigned)(size / time); //approximate  1024/1000
	}
	video_printf("%s  Time:%dms Vel:%dKB/s \n", msg_prefix, time, vel);
#endif
}

static void fastboot_fail_wrapper(const char* msg)
{
	display_info(msg);
	fastboot_fail(msg);
}

static void fastboot_ok_wrapper(const char* msg, unsigned data_size)
{
	display_speed_info(msg, data_size);
	fastboot_okay("");
}
void cmd_install_sig(const char *arg, void *data, unsigned sz)
{
	fastboot_fail_wrapper("Signature command not supported");
}


bool power_check()
{
	//3500 mV. shut down threshold 3.45V
	if (get_bat_sense_volt(5) < 3500) {
		return false;
	} else {
		return true;
	}
}

void cmd_download(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];
	unsigned len = hex2unsigned(arg);
	int r;

	if (!power_check()) {
		fastboot_fail("low power, need battery charging.");
		return;
	}
	init_display_xy();
	download_size = 0;
	if (len > download_max) {
		fastboot_fail_wrapper("data is too large");
		return;
	}

	snprintf(response, MAX_RSP_SIZE, "DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0) {
		return;
	}
	display_info("USB Transferring... ");
	TIME_START;
	r = usb_read(download_base, len);
	if ((r < 0) || ((unsigned) r != len)) {
		fastboot_fail_wrapper("Read USB error");
		fastboot_state = STATE_ERROR;
		return;
	}
	download_size = len;

	fastboot_ok_wrapper("USB Transmission OK", len);
}

#ifdef NAND_TYPE
static int get_nand_image_type(const char* arg)
{
	int img_type = 0;
	if (!strcmp(arg, "system") ||
	        !strcmp(arg, "userdata") ||
	        !strcmp(arg, "fat") ) {
#if defined(MTK_NAND_UBIFS_SUPPORT) || defined(MTK_NAND_MTK_FTL_SUPPORT)
		img_type = UBIFS_IMG;
#else
		img_type = YFFS2_IMG;
#endif
	} else {
		img_type = RAW_DATA_IMG;
	}
	return img_type;
}
#endif

void cmd_flash_mmc_img(const char *arg, void *data, unsigned sz)
{
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;

#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	unsigned int part_id;
#endif


#ifdef MTK_SPI_NOR_SUPPORT
	if (!strcmp(arg, "NOR")) {
		ptn = 0;
		part_id = 0; /* If need, example : NOR_PART_BOOT1 */
		goto write_nor_part;
	} else
#endif

#ifdef FASTBOOT_WHOLE_FLASH_SUPPORT
		if (!strcmp(arg, "boot0")) {
			ptn = 0;
			part_id = EMMC_PART_BOOT1;
			mmc_emmc_boot_prepare();
			goto write_part;
		} else if (!strcmp(arg,"boot1")) {
			ptn = 0;
			part_id  = EMMC_PART_BOOT2;
			goto write_part;
		} else if (!strcmp(arg, "partition")) {
			ptn = 0;
			part_id = EMMC_PART_USER;
			goto write_part;
		}
#else
		if (!strcmp(arg, "partition")) {
			dprintf(ALWAYS, "Attempt to write partition image.\n");
			fastboot_fail_wrapper("Do not support this operation.");
			return;
		}
#endif
		else {
			index = partition_get_index(arg);
			if (index == INVALID_PTN) {
				fastboot_fail("This partition doesn't exist");
				return;
			}
			ptn = partition_get_offset(index);
			if (ptn == (unsigned long long)(-1)) {
				fastboot_fail("partition table doesn't exist");
				return;
			}
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
			part_id = partition_get_region(index);
#endif

			if (!strcmp(arg, "boot") || !strcmp(arg, "recovery")) {
				if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE-1)) {
					fastboot_fail_wrapper("image is not a boot image");
					return;
				}
			}
			if (!strcmp(arg, "preloader")) {
#ifdef MTK_EMMC_SUPPORT
				int boot_wp = mmc_get_card(0)->raw_ext_csd[EXT_CSD_BOOT_WP];
				if (boot_wp & EXT_CSD_BOOT_WP_EN_PERM_WP) {
					fastboot_fail_wrapper("flash preloader is not permitted.");
					return;
				}
#endif

#ifdef PLATFORM_FASTBOOT_EMPTY_STORAGE
				if (process_preloader(data, &sz)) {
					fastboot_fail("not a valid preloader.");
					return;
				}
				ptn = 0;
#else
				ptn = HEADER_BLOCK_SIZE;
#endif
			}

			size = partition_get_size(index);
			if (ROUND_TO_PAGE(sz,511) > size) {
				fastboot_fail_wrapper("size too large");
				return;
			}
		}
write_part:

#ifdef EMMC_TYPE
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	dprintf (ALWAYS, "partid %d, addr 0x%llx, size 0x%x\n", part_id, ptn, sz);
	if (emmc_write(part_id, ptn , (unsigned int *)data, sz) != sz)
#else
	if (emmc_write(ptn , (unsigned int *)data, sz) != sz)
#endif
#endif

#ifdef NAND_TYPE
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
		if (nand_write_img((u64)ptn, (char*)data, sz,(u64)size,get_nand_image_type(arg)))
#else
		if (nand_write_img((u32)ptn, (char*)data, sz,(u32)size,get_nand_image_type(arg)))
#endif
#endif
		{
			fastboot_fail_wrapper("flash write failure");
			return;
		}
#ifdef FASTBOOT_WHOLE_FLASH_SUPPORT
	if (!strcmp(arg, "partition")) {
		mt_part_init(0);
	}
#endif
	fastboot_okay("");
	return;

#ifdef NOR_TYPE
#ifdef MTK_SPI_NOR_SUPPORT
write_nor_part:

	dprintf (ALWAYS, "partid %d, addr 0x%llx, size 0x%x\n", part_id, ptn, sz);
	if (nor_write_part(part_id, ptn , (unsigned int *)data, (u64)sz) != sz) {
		fastboot_fail_wrapper("SPI NOR flash write failure");
		return;
	}
	fastboot_okay("");
	return;
#endif
#endif
}

void cmd_flash_mmc_sparse_img(const char *arg, void *data, unsigned sz)
{
	unsigned int chunk;
	unsigned int chunk_data_sz;
	uint32_t *fill_buf = NULL;
	uint32_t *fill_val;
	uint32_t chunk_blk_cnt = 0;
	uint32_t i;
	unsigned long long size_wrote = 0;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	unsigned int part_id;
#endif

	index = partition_get_index(arg);
	if (INVALID_PTN == index) {
		fastboot_fail("This partition doesn't exist");
		return;
	}

	ptn = partition_get_offset(index);
	if (ptn == (unsigned long long)(-1)) {
		fastboot_fail("partition offset is wrong");
		return;
	}
	dprintf(ALWAYS, "partition(%s) index is %d, ptn is 0x%llx\n", arg, index, ptn);
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	part_id = partition_get_region(index);
#endif

	size = partition_get_size(index);

	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;
	dprintf(ALWAYS, "Image size span 0x%llx, partition size 0x%llx\n", (unsigned long long)sparse_header->total_blks*sparse_header->blk_sz, size);
	if ((unsigned long long)sparse_header->total_blks*sparse_header->blk_sz > size) {
		fastboot_fail("sparse image size span overflow.");
		return;
	}

	data += sparse_header->file_hdr_sz;
	if (sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
		/* Skip the remaining bytes in a header that is longer than
		 * we expected.
		 */
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	dprintf (ALWAYS, "=== Sparse Image Header ===\n");
	dprintf (ALWAYS, "magic: 0x%x\n", sparse_header->magic);
	dprintf (ALWAYS, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf (ALWAYS, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf (ALWAYS, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf (ALWAYS, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf (ALWAYS, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf (ALWAYS, "total_blks: %d\n", sparse_header->total_blks);
	dprintf (ALWAYS, "total_chunks: %d\n", sparse_header->total_chunks);

	display_info("\nWriting Flash ... ");
	/* Start processing chunks */
	for (chunk=0; chunk<sparse_header->total_chunks; chunk++) {
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *)data;
		data += sizeof(chunk_header_t);

		//dprintf (ALWAYS, "=== Chunk Header ===\n");
		//dprintf (ALWAYS, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		//dprintf (ALWAYS, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		//dprintf (ALWAYS, "total_size: 0x%x\n", chunk_header->total_sz);

		if (sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
			/* Skip the remaining bytes in a header that is longer than
			 * we expected.
			 */
			data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch (chunk_header->chunk_type) {
			case CHUNK_TYPE_RAW:
				if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
				                               chunk_data_sz)) {
					fastboot_fail("Bogus chunk size for chunk type Raw");
					return;
				}

				dprintf (ALWAYS, "Raw: start block addr: 0x%x\n", total_blocks);

#ifdef EMMC_TYPE
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
				//dprintf (ALWAYS, "partid %d, addr 0x%llx, partsz 0x%x\n", part_id, ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz) , chunk_data_sz);
				if (emmc_write(part_id, ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz) , data, chunk_data_sz) != chunk_data_sz)
#else
				if (emmc_write(ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz) , data, chunk_data_sz) != chunk_data_sz)
#endif
#endif

#ifdef NAND_TYPE
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
					if (nand_write_img((u64)(ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz)), (char*)data, chunk_data_sz,(u64)size,get_nand_image_type(arg)))
#else
					if (nand_write_img((u32)(ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz)), (char*)data, chunk_data_sz,(u32)size,get_nand_image_type(arg)))
#endif
#endif
					{
						fastboot_fail_wrapper("flash write failure");
						return;
					}

				total_blocks += chunk_header->chunk_sz;
				data += chunk_data_sz;
				break;

			case CHUNK_TYPE_DONT_CARE:
				dprintf (ALWAYS, "!!Blank: start: 0x%x  offset: 0x%x\n", total_blocks, chunk_header->chunk_sz);
				total_blocks += chunk_header->chunk_sz;
				break;

			case CHUNK_TYPE_FILL:
				dprintf (ALWAYS, "%s %d: CHUNK_TYPE_FILL=0x%x size=%d chunk_data_sz=%d\n", __FUNCTION__, __LINE__, *(uint32_t *)data, ROUNDUP(sparse_header->blk_sz, CACHE_LINE), chunk_data_sz);
				if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz + sizeof(uint32_t))) {
					fastboot_fail_wrapper("Bogus chunk size for chunk type FILL");
					return FALSE;
				}

				fill_buf = (uint32_t *)memalign(CACHE_LINE, ROUNDUP(sparse_header->blk_sz, CACHE_LINE));
				if (!fill_buf) {
					fastboot_fail_wrapper("Malloc failed for: CHUNK_TYPE_FILL");
					return FALSE;
				}

				fill_val = *(uint32_t *)data;
				data = (char *) data + sizeof(uint32_t);
				chunk_blk_cnt = chunk_data_sz / sparse_header->blk_sz;

				for (i = 0; i < (sparse_header->blk_sz / sizeof(fill_val)); i++) {
					fill_buf[i] = fill_val;
				}

				for (i = 0; i < chunk_blk_cnt; i++) {
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
					size_wrote = emmc_write(part_id, ptn + ((uint64_t)total_blocks*sparse_header->blk_sz),
					                        fill_buf, sparse_header->blk_sz);
#else
					size_wrote = emmc_write(ptn + ((uint64_t)total_blocks*sparse_header->blk_sz),
					                        fill_buf, sparse_header->blk_sz);
#endif

					if (size_wrote != sparse_header->blk_sz) {
						fastboot_fail_wrapper("CHUNK_TYPE_FILL flash write failure");
						free(fill_buf);
						return FALSE;
					}

					total_blocks++;
				}

				free(fill_buf);

				break;

			case CHUNK_TYPE_CRC:
				if (chunk_header->total_sz != sparse_header->chunk_hdr_sz) {
					fastboot_fail_wrapper("Bogus chunk size for chunk type Dont Care");
					return;
				}
				total_blocks += chunk_header->chunk_sz;
				data += chunk_data_sz;
				break;

			default:
				fastboot_fail_wrapper("Unknown chunk type");
				return;
		}
		display_progress("\rWrite Data", total_blocks*sparse_header->blk_sz, sparse_header->total_blks*sparse_header->blk_sz);
	}

	dprintf(ALWAYS, "Wrote %d blocks, expected to write %d blocks\n",
	        total_blocks, sparse_header->total_blks);

	if (total_blocks != sparse_header->total_blks) {
		fastboot_fail_wrapper("sparse image write failure");
	} else {
		display_info("\n\nOK");
		fastboot_okay("");
	}

	return;
}
extern int mboot_recovery_load_raw_part_offset(char *part_name, unsigned long *addr, unsigned long offset, unsigned int size) __attribute__((weak));

int _virtual_partition_support(const char *arg, void *data, unsigned int* p_sz)
{
#define TMP_DOWNLOAD_SIZE   52*1024*1024    //52MB, because kernel size(64M) - pagetable max size (8M) - ramdisk size(Estimate 4M)

	unsigned t_bootimg_addr = ((u32)data);
	void* new_data =(void *)((u32)data + SCRATCH_SIZE - TMP_DOWNLOAD_SIZE);
	bool _is_zimage = false;
	boot_img_hdr *p_boot_hdr;
	unsigned int t_kernel_addr;
	unsigned int t_ramdisk_addr;
	unsigned int sz = *p_sz;
	char* tmp = (void *)arg;
	int ret;

	if (!memcmp(arg, "zimage", strlen("zimage"))) {
		_is_zimage = true;
		dprintf(INFO,"Get zimage\n");
	} else if (!memcmp(arg, "ramdisk", strlen("ramdisk"))) {
		dprintf(INFO,"Get ramdisk\n");
	} else {
		return 0;
	}
	/* check size first */
	if (sz > TMP_DOWNLOAD_SIZE)
		return -ENOMEM;

	/* copy ori data to temp addr */
	memcpy(new_data, data, sz);
	/* copy bootimg header content */
	//clean space for img_hdr make the debug easier, 8k is enough for most case (page size < 8k)
	memset(data, 0, 0x2000);
#ifdef MTK_GPT_SCHEME_SUPPORT
	ret = mboot_recovery_load_raw_part_offset("boot", (void *)t_bootimg_addr, 0, sizeof(boot_img_hdr));
#else
	ret = mboot_recovery_load_raw_part_offset(PART_BOOTIMG, (void *)t_bootimg_addr, 0, sizeof(boot_img_hdr));
#endif
	if (ret < 0) {
		return ret;
	}
	p_boot_hdr = (void *)t_bootimg_addr;
	dprintf(INFO,"ori kernel_size: %x\n", p_boot_hdr->kernel_size);
	dprintf(INFO,"ori ramdisk_size: %x\n", p_boot_hdr->ramdisk_size);

	t_kernel_addr = t_bootimg_addr + p_boot_hdr->page_size;
	if (_is_zimage == true) {
		/* copy kernel content from new_data*/
		memcpy((void *)t_kernel_addr, new_data, sz);
		/* copy ramdisk content from flash */
		t_ramdisk_addr = ROUNDUP(t_kernel_addr + sz, p_boot_hdr->page_size);
#ifdef MTK_GPT_SCHEME_SUPPORT
		ret = mboot_recovery_load_raw_part_offset("boot", (void *)t_ramdisk_addr,
		        ROUNDUP(p_boot_hdr->kernel_size + p_boot_hdr->page_size, p_boot_hdr->page_size), p_boot_hdr->ramdisk_size);
#else
		ret = mboot_recovery_load_raw_part_offset(PART_BOOTIMG, (void *)t_ramdisk_addr,
		        ROUNDUP(p_boot_hdr->kernel_size + p_boot_hdr->page_size, p_boot_hdr->page_size), p_boot_hdr->ramdisk_size);
#endif

		if (ret < 0) {
			return ret;
		}
		/* update header */
		p_boot_hdr->kernel_size = sz;
		dprintf(INFO,"->kernel_size %x\n", sz);
	} else {
		/* copy kernel content from flash*/
#ifdef MTK_GPT_SCHEME_SUPPORT
		ret = mboot_recovery_load_raw_part_offset("boot", (void *)t_kernel_addr, p_boot_hdr->page_size, p_boot_hdr->kernel_size);
#else
		ret = mboot_recovery_load_raw_part_offset(PART_BOOTIMG, (void *)t_kernel_addr, p_boot_hdr->page_size, p_boot_hdr->kernel_size);
#endif
		if (ret < 0) {
			return ret;
		}

		t_ramdisk_addr = ROUNDUP(t_kernel_addr + p_boot_hdr->kernel_size, p_boot_hdr->page_size);
		/* copy ramdisk content from new_data*/
		memcpy((void *)t_ramdisk_addr, new_data, sz);
		/* update header */
		p_boot_hdr->ramdisk_size = sz;

		dprintf(INFO,"->ramdisk_size %x\n", sz);
	}

	/* overwrite arg*/
	*p_sz = p_boot_hdr->page_size + ROUNDUP(p_boot_hdr->kernel_size, p_boot_hdr->page_size)+ ROUNDUP(p_boot_hdr->ramdisk_size, p_boot_hdr->page_size);
	strcpy(tmp,"boot");

	return 0;

}
void cmd_flash_mmc(const char *arg, void *data, unsigned sz)
{
	sparse_header_t *sparse_header;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int permitted = 0;
	char msg[64];
#endif
#ifdef MTK_GPT_SCHEME_SUPPORT
#ifdef PLATFORM_FASTBOOT_EMPTY_STORAGE
	int err;
#endif
#endif
	part_hdr_t *part_hdr;
	char fsinglebl = 0;
	u32 len;
	u32 counter;

	if (sz  == 0) {
		fastboot_okay("");
		return;
	}

	if (!strcmp(arg, "singlebootloader"))
		fsinglebl = 1;

	len = sz;
	counter = 5;
	while (len && (counter > 0)) {
		sparse_header = (sparse_header_t *) data;
		part_hdr = (part_hdr_t *)data;
		if (1 == fsinglebl) {
			arg = data + 8;
			if ((!strncmp(arg, "lk", 2))|| (!strncmp(arg, "LK", 2))) {
				arg = "lk";
				sz = part_hdr->info.dsize;//+ sizeof(part_hdr_t);
			}
			if ((!strncmp(arg, "logo", 4)) || (!strncmp(arg, "LOGO", 4))) {
				arg = "logo";
				sz = part_hdr->info.dsize;// + sizeof(part_hdr_t);
			}
			if ((!strncmp(arg, "preloader", 9))|| (!strncmp(arg, "PRELOADER", 9))) {
				arg = "preloader";
				sz = part_hdr->info.dsize;
			}
			data += sizeof(part_hdr_t);
			len -= sizeof(part_hdr_t);
		}

		// security check here.
		// ret = decrypt_scm((uint32 **) &data, &sz);
#ifdef MTK_SECURITY_SW_SUPPORT
		if (sec_dl_permission_chk(arg, &permitted)) {
			snprintf(msg, sizeof(msg), "failed to get download permission for partition '%s'\n", arg);
			fastboot_fail(msg);
			return;
		}

		if (0 == permitted) {
			snprintf(msg, sizeof(msg), "download for partition '%s' is not allowed\n", arg);
			fastboot_fail(msg);
			return;
		}
#endif
#ifdef MTK_GPT_SCHEME_SUPPORT
#ifdef PLATFORM_FASTBOOT_EMPTY_STORAGE
		if (!strcmp(arg, "gpt")) { //if arg == "gpt" , update pmbr, pgpt, and sgpt. Then re-init partition table for fastboot.
			err = gpt_partition_table_update(arg, data, sz);
			if (err) {
				fastboot_fail("failed to update gpt partition");
				return;
			}
			read_gpt(partition); //re-init partition table afster update pmbor, pgpt and sgpt
			fastboot_okay("");
			return;
		}
#endif
#endif
		TIME_START;

		if (_virtual_partition_support(arg, data, &sz) < 0) {
			fastboot_fail_wrapper("virtual partition write fail");
			return;
		}
		sparse_header = (sparse_header_t *) data;
		if (sparse_header->magic != SPARSE_HEADER_MAGIC)
			cmd_flash_mmc_img(arg, data, sz);
		else
#ifdef NAND_TYPE
			cmd_flash_nand_sparse_img(arg, data, sz);
#endif

#ifdef EMMC_TYPE
		cmd_flash_mmc_sparse_img(arg, data, sz);
#endif

		len -= sz;
		data += sz;
		counter--;
	}

	return;
}

void cmd_erase_mmc(const char *arg, void *data, unsigned sz)
{
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	unsigned int part_id;
#endif
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	int erase_ret = MMC_ERR_NONE;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int permitted = 0;
	char msg[64];
#endif

	dprintf (ALWAYS, "cmd_erase_mmc\n");

#ifdef MTK_SPI_NOR_SUPPORT
	if (!strcmp(arg, "NOR")) {
		ptn = 0;
		size = nor_get_device_capacity();
		part_id = 0;/* NOR_PART_BOOT1; */
		goto erase_nor_part;
	} else
#endif

#ifdef FASTBOOT_WHOLE_FLASH_SUPPORT
		if (!strcmp(arg, "boot0")) {
			ptn = 0;
			size = mmc_get_region_size(EMMC_PART_BOOT1);
			part_id = EMMC_PART_BOOT1;
		} else if (!strcmp(arg, "boot1")) {
			ptn = 0;
			size = mmc_get_region_size(EMMC_PART_BOOT2);
			part_id = EMMC_PART_BOOT2;
		} else if (!strcmp(arg, "partition")) {
			ptn = 0;
			size = mmc_get_region_size(EMMC_PART_USER);
			part_id = EMMC_PART_USER;
		} else
#endif
		{
			index = partition_get_index(arg);
			if (index == -1) {
				fastboot_fail_wrapper("Partition table doesn't exist");
				return;
			}

#ifdef MTK_SECURITY_SW_SUPPORT
			if (sec_format_permission_chk(arg, &permitted)) {
				snprintf(msg, sizeof(msg), "failed to get format permission for partition '%s'\n", arg);
				fastboot_fail(msg);
				return;
			}

			if (0 == permitted) {
				snprintf(msg, sizeof(msg), "format for partition '%s' is not allowed\n", arg);
				fastboot_fail(msg);
				return;
			}
#endif

#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
			part_id = partition_get_region(index);
#endif
			ptn = partition_get_offset(index);

			size = partition_get_size(index);
		}
	TIME_START;
#ifdef EMMC_TYPE
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	erase_ret = emmc_erase(part_id, ptn, size);
#else
	erase_ret = emmc_erase(ptn, size);
#endif
#endif

#ifdef NAND_TYPE
	erase_ret = nand_erase(ptn,size);
#endif

	if (erase_ret  == MMC_ERR_NONE) {
		fastboot_ok_wrapper("OK", size);
	} else {
		fastboot_fail_wrapper("Erase error.");
	}

	return;

#ifdef NOR_TYPE
erase_nor_part:

	dprintf (ALWAYS, "partid %d, addr 0x%llx, size 0x%llx\n", part_id, ptn, size);
	erase_ret = nor_erase_part(part_id, ptn, size);

	if (erase_ret  == 0/* NOR_ERR_NONE */) { /* NOR_ERR_NONE suppose to be zero */
		fastboot_ok_wrapper("OK", size);
	} else {
		fastboot_fail_wrapper("Erase SPI NOR flash error.");
	}

	return;
#endif
}


/****************NEW CACHE WRITE*************************/
#if defined(NAND_TYPE)

typedef struct data_cache {
	unsigned char* base;
	unsigned int len;
	unsigned int offset;
	unsigned int blocks_per_cache;
	bool is_first_write;
	bool valid_data;
} data_cache_t;

data_cache_t* cache;
static void memfill(unsigned int* addr, unsigned int value, unsigned int count)
{
	unsigned int i = 0;
	for (; i<count; ++i) {
		addr[i] = value;
	}
	return;
}

typedef struct nand_arg {
	unsigned long long ptn;
	unsigned int img_type;
	unsigned long long size; //partition size
} nand_arg_t;

extern int nand_get_alignment();
unsigned int get_cache_size()
{
	return (unsigned int)nand_get_alignment();
}
extern int nand_img_read(unsigned long long source, unsigned char * dst, int size);

static bool cached_write_data(unsigned int* start_blocks, char* data, unsigned long long data_sz,unsigned int chunk_type, unsigned int sparse_blk_sz, void* arg)
{
	unsigned long long byte_to_process = data_sz;
	unsigned int byte_processed = 0;
	bool skip_zero_copy = false; //only affect CHUNK_TYPE_DONT_CARE
	unsigned int total_blocks = *start_blocks;

	nand_arg_t* nand = (nand_arg_t*)arg;

	while (byte_to_process != 0) {
		unsigned int slot_len = (byte_to_process > cache->len - cache->offset) ? cache->len - cache->offset :byte_to_process;
		if (chunk_type == CHUNK_TYPE_RAW ||chunk_type == CHUNK_TYPE_FILL) {
			if (cache->is_first_write && (!cache->valid_data) && cache->offset!=0) {
#if defined(MTK_MLC_NAND_SUPPORT)
				nand_img_read((u64)(nand->ptn + ((unsigned long long)total_blocks*sparse_blk_sz)), (char*)cache->base, cache->len);
#else
				nand_img_read((u32)(nand->ptn + ((unsigned long long)total_blocks*sparse_blk_sz)), (char*)cache->base, cache->len);
#endif
				dprintf(ALWAYS, "Read ptn = 0x%llx, Address 0x%x = 0x%x\n", nand->ptn, total_blocks,*(int*)cache->base);
				cache->is_first_write = false;
			}
		}

		if (chunk_type == CHUNK_TYPE_RAW) {
			memcpy(cache->base+cache->offset, data+byte_processed, slot_len);
			cache->valid_data = true;
		} else if (chunk_type == CHUNK_TYPE_DONT_CARE ) {
			if (!cache->valid_data) {
				skip_zero_copy = true;
			}
			if (!skip_zero_copy)
				memset(cache->base+cache->offset, 0xFF, slot_len);
		} else if (chunk_type == CHUNK_TYPE_FILL) {
			memfill((unsigned int*)(cache->base+cache->offset), *(unsigned int*)data, slot_len/sizeof(unsigned int));
			cache->valid_data = true;
		}

		cache->offset += slot_len;

		if (cache->offset == cache->len) { //cache full
			dprintf(ALWAYS, "@@@@write At block(x4K) 0x%x\n", total_blocks);
			if (!skip_zero_copy) {


#if defined(MTK_MLC_NAND_SUPPORT)
				dprintf(ALWAYS, "Address 0x%x = 0x%x\n", total_blocks,*(int*)cache->base);
				if (nand_write_img((u64)(nand->ptn + ((unsigned long long)total_blocks*sparse_blk_sz)), (char*)cache->base, cache->len,nand->size,nand->img_type))
#else
				if (nand_write_img((u32)(nand->ptn + ((unsigned long long)total_blocks*sparse_blk_sz)), (char*)cache->base, cache->len,nand->size,nand->img_type))
#endif

				{
					dprintf(ALWAYS, "@@@@write failed\n");
					return false;
				}
			}
			cache->offset = 0;   //start new package.
			byte_to_process -= slot_len;
			byte_processed += slot_len;
			total_blocks += cache->blocks_per_cache;
			cache->valid_data = false;
		} else {
			//can not full cache.
			break;
		}

		if (chunk_type == CHUNK_TYPE_DONT_CARE) {
			skip_zero_copy = true;
		}
	}
	*start_blocks = total_blocks;
	return true;
}



void cmd_flash_nand_sparse_img(const char *arg, void *data, unsigned sz)
{
	unsigned int chunk;
	unsigned long long chunk_data_sz;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;



	nand_arg_t sto_arg;
	sto_arg.img_type = get_nand_image_type(arg);

	unsigned int crc_value;
	unsigned int fill_value;
	data_cache_t cache_inst;
	cache_inst.len = get_cache_size();
	cache_inst.base = (unsigned char*)memalign(128, cache_inst.len);
	cache_inst.offset = 0;
	cache_inst.is_first_write = true;
	cache_inst.valid_data = false;
	cache = &cache_inst;

	dprintf(ALWAYS, "@@@ cache len 0x%x\n", cache_inst.len);
	index = partition_get_index(arg);
	sto_arg.ptn = partition_get_offset(index);
	if (sto_arg.ptn == (unsigned long long)(-1)) {
		fastboot_fail("partition table doesn't exist");
		goto exit;
	}


	sto_arg.size = partition_get_size(index);

	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;
	dprintf(ALWAYS, "Image size span 0x%llx, partition size 0x%llx\n", (unsigned long long)sparse_header->total_blks*sparse_header->blk_sz, sto_arg.size);
	if ((unsigned long long)sparse_header->total_blks*sparse_header->blk_sz > sto_arg.size) {
		fastboot_fail("sparse image size span overflow.");
		goto exit;
	}

	data += sparse_header->file_hdr_sz;
	if (sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
		/* Skip the remaining bytes in a header that is longer than
		* we expected.
		*/
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	cache->blocks_per_cache = (cache->len/sparse_header->blk_sz);
	dprintf (ALWAYS, "=== Sparse Image Header ===\n");
	dprintf (ALWAYS, "magic: 0x%x\n", sparse_header->magic);
	dprintf (ALWAYS, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf (ALWAYS, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf (ALWAYS, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf (ALWAYS, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf (ALWAYS, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf (ALWAYS, "total_blks: %d\n", sparse_header->total_blks);
	dprintf (ALWAYS, "total_chunks: %d\n", sparse_header->total_chunks);

	display_info("\nWriting Flash ... ");
	/* Start processing chunks */
	for (chunk=0; chunk<sparse_header->total_chunks; chunk++) {
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		data += sizeof(chunk_header_t);

		//dprintf (ALWAYS, "=== Chunk Header ===\n");
		//dprintf (ALWAYS, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		//dprintf (ALWAYS, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		//dprintf (ALWAYS, "total_size: 0x%x\n", chunk_header->total_sz);

		if (sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
			/* Skip the remaining bytes in a header that is longer than
			* we expected.
			*/
			data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = (unsigned long long)(sparse_header->blk_sz&0xFFFFFFFF);
		chunk_data_sz = chunk_data_sz * chunk_header->chunk_sz;
		switch (chunk_header->chunk_type) {
			case CHUNK_TYPE_RAW:
				if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
				                               chunk_data_sz)) {
					//fastboot_fail("Bogus chunk size for chunk type Raw");
					goto exit;
				}


				if (!cached_write_data(&total_blocks, data, chunk_data_sz, CHUNK_TYPE_RAW, sparse_header->blk_sz, (void*)&sto_arg)) {
					fastboot_fail_wrapper("storage write failed.");
					goto exit;
				}
				data += chunk_data_sz;

				break;

			case CHUNK_TYPE_DONT_CARE:
				if (!cached_write_data(&total_blocks, 0, chunk_data_sz, CHUNK_TYPE_DONT_CARE, sparse_header->blk_sz, (void*)&sto_arg)) {
					fastboot_fail_wrapper("storage write failed.");
					goto exit;
				}
				break;
			case CHUNK_TYPE_CRC:
				crc_value = *(unsigned int*)data;
				data += 4;
				break;

			case CHUNK_TYPE_FILL:
				fill_value = *(unsigned int*)data;

				if (!cached_write_data(&total_blocks, (char*)&fill_value, chunk_data_sz, CHUNK_TYPE_FILL, sparse_header->blk_sz, (void*)&sto_arg)) {
					fastboot_fail_wrapper("storage write failed.");
					goto exit;
				}
				data += 4;  //data length is 4.
				break;
			default:
				fastboot_fail_wrapper("Unknown chunk type");
				goto exit;
		}
		display_progress("\rWrite Data", total_blocks*sparse_header->blk_sz, sparse_header->total_blks*sparse_header->blk_sz);
	}

	/******************/
	//last cache
	if (cache->offset != 0) {

#if defined(MTK_MLC_NAND_SUPPORT)
		if (nand_write_img((u64)(sto_arg.ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz)), (char*)cache->base, cache->offset,sto_arg.size,sto_arg.img_type))
#else
		if (nand_write_img((u32)(sto_arg.ptn + ((unsigned long long)total_blocks*sparse_header->blk_sz)), (char*)cache->base, cache->offset,sto_arg.size,sto_arg.img_type))
#endif

			dprintf(ALWAYS, "@@@@Last block\n");
		total_blocks += cache->offset/sparse_header->blk_sz;
	}
	/******************/

	dprintf(ALWAYS, "Wrote %d blocks, expected to write %d blocks\n",
	        total_blocks, sparse_header->total_blks);

	if (total_blocks != sparse_header->total_blks) {
		fastboot_fail_wrapper("sparse image write failure");
	} else {
		display_info("\n\nOK");
		fastboot_okay("");
	}
exit:
	if (cache->base) free(cache->base);
	dprintf(ALWAYS, "@@@ END_2\n");
	return;
}

#endif
/**********END********/

/*LXO: END!Download related command*/
