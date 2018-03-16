/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2016. All rights reserved.
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

#include <block_generic_interface.h>
#include <malloc.h>
#include <printf.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>
#include <dev/mrdump.h>
#include <platform/env.h>
#include <platform/mtk_key.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_gpt.h>
#include <target/cust_key.h>
#include <platform/boot_mode.h>
#include <platform/bootimg.h>
#include <platform/ram_console.h>
#include <arch/ops.h>
#include <libfdt.h>
#include <platform.h>
#include "aee.h"
#include "kdump.h"

#ifdef MTK_3LEVEL_PAGETABLE
#include <target.h>
#endif

extern BOOT_ARGUMENT *g_boot_arg;
extern bool cmdline_append(const char* append_string);
extern void *g_fdt;
extern boot_img_hdr *g_boot_hdr;

#define MRDUMP_DELAY_TIME 10

static struct mrdump_control_block *mrdump_cblock = NULL;

static struct mrdump_cblock_result cblock_result;
static unsigned int log_size;

static void voprintf(char type, const char *msg, va_list ap)
{
	char msgbuf[128], *p;

	p = msgbuf;
	if (msg[0] == '\r') {
		*p++ = msg[0];
		msg++;
	}

	*p++ = type;
	*p++ = ':';
	vsnprintf(p, sizeof(msgbuf) - (p - msgbuf), msg, ap);
	switch (type) {
		case 'I':
		case 'W':
		case 'E':
			video_printf("%s", msgbuf);
			break;
	}

	dprintf(CRITICAL,"%s", msgbuf);

	/* Write log buffer */
	p = msgbuf;
	while ((*p != 0) && (log_size < sizeof(cblock_result.log_buf))) {
		cblock_result.log_buf[log_size] = *p++;
		log_size++;
	}
}

void voprintf_verbose(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	voprintf('V', msg, ap);
	va_end(ap);
}

void voprintf_debug(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	voprintf('D', msg, ap);
	va_end(ap);
}

void voprintf_info(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	voprintf('I', msg, ap);
	va_end(ap);
}

void voprintf_warning(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	voprintf('W', msg, ap);
	va_end(ap);
}

void voprintf_error(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	voprintf('E', msg, ap);
	va_end(ap);
}

void vo_show_progress(int sizeM)
{
	video_set_cursor((video_get_rows() / 4) * 3, (video_get_colums() - 22)/ 2);
	video_printf("=====================\n");
	video_set_cursor((video_get_rows() / 4) * 3 + 1, (video_get_colums() - 22)/ 2);
	video_printf(">>> Written %4dM <<<\n", sizeM);
	video_set_cursor((video_get_rows() / 4) * 3 + 2, (video_get_colums() - 22)/ 2);
	video_printf("=====================\n");
	video_set_cursor(video_get_rows() - 1, 0);

	dprintf(CRITICAL,"... Written %dM\n", sizeM);
}

static void mrdump_status(const char *status, const char *fmt, va_list ap)
{
	char *dest = cblock_result.status;
	dest += strlcpy(dest, status, sizeof(cblock_result.status));
	*dest++ = '\n';

	vsnprintf(dest, sizeof(cblock_result.status) - (dest - cblock_result.status), fmt, ap);
}

void mrdump_status_ok(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	mrdump_status("OK", fmt, ap);
	va_end(ap);
}

void mrdump_status_none(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	mrdump_status("NONE", fmt, ap);
	va_end(ap);
}

void mrdump_status_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	mrdump_status("FAILED", fmt, ap);
	va_end(ap);
}

uint32_t g_aee_mode = AEE_MODE_MTK_ENG;

const const char *mrdump_mode2string(uint8_t mode)
{
	switch (mode) {
		case AEE_REBOOT_MODE_NORMAL:
			return "NORMAL-BOOT";

		case AEE_REBOOT_MODE_KERNEL_OOPS:
			return "KERNEL-OOPS";

		case AEE_REBOOT_MODE_KERNEL_PANIC:
			return "KERNEL-PANIC";

		case AEE_REBOOT_MODE_NESTED_EXCEPTION:
			return "NESTED-CPU-EXCEPTION";

		case AEE_REBOOT_MODE_WDT:
			return "HWT";

		case AEE_REBOOT_MODE_EXCEPTION_KDUMP:
			return "MANUALDUMP";
		case AEE_REBOOT_MODE_MRDUMP_KEY:
			return "MRDUMP_KEY";

		default:
			return "UNKNOWN-BOOT";
	}
}

#define MRDUMP_EXPDB_OFFSET 3145728

static void mrdump_write_result(void)
{
	dprintf(CRITICAL, "%s: Enter\n", __func__);

	int index = partition_get_index("expdb");
	part_dev_t *dev = mt_part_get_device();
	if (index == -1 || dev == NULL) {
		dprintf(CRITICAL, "%s: no %s partition[%d]\n", __func__, "expdb", index);
		return;
	}
#if defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT) || defined(MTK_MLC_NAND_SUPPORT)
	int part_id = partition_get_region(index);
#endif
	u64 part_size = partition_get_size(index);
	if (part_size < MRDUMP_EXPDB_OFFSET) {
		dprintf(CRITICAL, "%s: partition size(%llx) is less then reserved (%x)\n", __func__, part_size, MRDUMP_EXPDB_OFFSET);
		return;
	}
	u64 part_offset = partition_get_offset(index) + part_size - MRDUMP_EXPDB_OFFSET;

	dprintf(CRITICAL, "%s: offset %lld size %lld\n", __func__, part_offset, part_size);

#if (defined(MTK_UFS_BOOTING) || defined(MTK_EMMC_SUPPORT))
#if (defined(MTK_UFS_BOOTING) || defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	dev->write(dev, (uchar *)&cblock_result, part_offset, sizeof(cblock_result), part_id);
#else
	dev->write(dev, (uchar *)&cblock_result, part_offset, sizeof(cblock_result));
#endif
#else
	dev->write(dev, (uchar *)&cblock_result, part_offset, sizeof(cblock_result), part_id);
#endif
}

static int mrdump_sdcard_output(const struct mrdump_control_block *kparams, uint64_t total_dump_size)
{
	mrdump_vfat_output(kparams, total_dump_size, mrdump_dev_sdcard());
	return 0;
}

#define SIZE_1MB 1048576ULL
#define SIZE_64MB 67108864ULL

static uint64_t mrdump_mem_size(void)
{
	uint64_t total_dump_size = physical_memory_size();
	const char *mem_size_param = get_env("mrdump_mem_size");
	if (mem_size_param != NULL) {
		uint64_t mem_size = atoi(mem_size_param) * SIZE_1MB;
		voprintf_info("Memory dump size set to %uM\n", (unsigned int) (mem_size / SIZE_1MB));
		if (mem_size >= SIZE_64MB) {
                        /* minimum 64m */
			total_dump_size = MIN(total_dump_size, mem_size);
		}
	}
	/* 4G - 1M only for now, workaround zip32 format issue */
	return MIN(total_dump_size, 0xfff00000ULL);
}

static int mrdump_output_device(void)
{
	const char *output_device_param = get_env("mrdump_output");
	if (output_device_param != NULL) {
		if (strcmp(output_device_param, "none") == 0)
			return MRDUMP_DEV_NONE;
		if (strcmp(output_device_param, "null") == 0)
			return MRDUMP_DEV_NULL;
		if (strcmp(output_device_param, "internal-storage:ext4") == 0)
			return MRDUMP_DEV_ISTORAGE_EXT4;
		if (strcmp(output_device_param, "internal-storage:vfat") == 0)
			return MRDUMP_DEV_ISTORAGE_VFAT;
		return MRDUMP_DEV_UNKNOWN;
	}
	return MRDUMP_DEV_ISTORAGE_EXT4;
}

static void kdump_ui(struct mrdump_control_block *mrdump_cblock)
{
	video_clean_screen();
	video_set_cursor(0, 0);

	mrdump_status_error("Unknown error\n");
	voprintf_info("Kdump triggerd by '%s'\n", mrdump_mode2string(mrdump_cblock->crash_record.reboot_mode));

	struct aee_timer elapse_time;
	aee_timer_init(&elapse_time);
	aee_timer_start(&elapse_time);
	int output_device = mrdump_output_device();
	switch (output_device) {
		case MRDUMP_DEV_NULL:
			kdump_null_output(mrdump_cblock, mrdump_mem_size());
			break;
		case MRDUMP_DEV_ISTORAGE_EXT4:
			voprintf_info("Output to EXT4 Partition\n");
			mrdump_ext4_output(mrdump_cblock, mrdump_mem_size(), mrdump_dev_emmc_ext4());
			break;
		case MRDUMP_DEV_ISTORAGE_VFAT:
			voprintf_info("Output to VFAT Partition\n");
			mrdump_vfat_output(mrdump_cblock, mrdump_mem_size(), mrdump_dev_emmc_vfat());
			break;
		default:
			voprintf_error("Unsupport device id %d\n", output_device);
	}
	aee_mrdump_flush_cblock(mrdump_cblock);
	aee_timer_stop(&elapse_time);

	voprintf_info("Reset count down %d ...\n", MRDUMP_DELAY_TIME);
	mtk_wdt_restart();

	int timeout = MRDUMP_DELAY_TIME;
	while (timeout-- >= 0) {
		mdelay(1000);
		mtk_wdt_restart();
		voprintf_info("\rsec %d", timeout);
	}

	video_clean_screen();
	video_set_cursor(0, 0);
}

void mrdump_append_cmdline(void)
{

	char *mrdump_rsv_mem;
	char *mrdump_rsv_tmp = malloc(128);
	mrdump_rsv_mem = malloc(128);
	cmdline_append("mrdump.lk=" MRDUMP_GO_DUMP);
	/*
	 * mrdump_rsv_mem format , a pair of start address and size
	 * example 0x46000000 0x200000,0x47000000,0x100000
	 * current limitation is 16 pair in kernel
	 * */
	if (mrdump_rsv_mem && mrdump_rsv_tmp) {
		snprintf(mrdump_rsv_mem, 128, "mrdump_rsvmem=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",
		         MEMBASE, AEE_MRDUMP_LK_RSV_SIZE, (unsigned int)BOOT_ARGUMENT_LOCATION&0xfff00000
		         , (g_boot_arg->dram_buf_size)?g_boot_arg->dram_buf_size:0x100000,
			g_boot_hdr->tags_addr, fdt_totalsize(g_fdt));

		if(platform_get_bootarg_addr && platform_get_bootarg_size) {
			snprintf(mrdump_rsv_tmp,128,",0x%x,0x%x", platform_get_bootarg_addr(), platform_get_bootarg_size());
			strncat(mrdump_rsv_mem, mrdump_rsv_tmp, 128);
		}

		cmdline_append(mrdump_rsv_mem);
	} else
		dprintf(CRITICAL, "MT-RAMDUMP: mrdump_rsv_mem malloc memory failed");

	free(mrdump_rsv_mem);
	free(mrdump_rsv_tmp);

	unsigned long mpa_addr = mrdump_setup_cpm();
	if (mpa_addr != 0) {
		char cmdline_buf[64];
		snprintf(cmdline_buf, sizeof(cmdline_buf), "mrdump.mpa=0x%lx", mpa_addr);
		cmdline_append(cmdline_buf);
	}

	if (g_boot_arg->ddr_reserve_ready != AEE_MRDUMP_DDR_RSV_READY) {
		cmdline_append("mrdump_ddr_reserve_ready=no");
		dprintf(CRITICAL, "MT-RAMDUMP: DDR reserve mode not ready, skipped (0x%x)\n", g_boot_arg->ddr_reserve_ready);
	}
	else
		cmdline_append("mrdump_ddr_reserve_ready=yes");


}

int mrdump_detection(void)
{

	if (!ram_console_is_abnormal_boot()) {
		dprintf(CRITICAL, "MT-RAMDUMP: No exception detected, skipped\n");
		return 0;
	}

	mrdump_cblock = aee_mrdump_get_params();
	if (mrdump_cblock == NULL) {
		dprintf(CRITICAL, "MT-RAMDUMP control block not found\n");
		return 0;
	}

	memset(&cblock_result, 0, sizeof(struct mrdump_cblock_result));
	log_size = 0;
	strlcpy(cblock_result.sig, MRDUMP_GO_DUMP, sizeof(cblock_result.sig));

	uint8_t reboot_mode = mrdump_cblock->crash_record.reboot_mode;

	if (!g_boot_arg->ddr_reserve_enable) {
		voprintf_debug("DDR reserve mode disabled\n");
		mrdump_status_none("DDR reserve mode disabled\n");
		goto error;
	}

	if (!g_boot_arg->ddr_reserve_success) {
		voprintf_debug("DDR reserve mode failed\n");
		mrdump_status_none("DDR reserve mode failed\n");
		goto error;
	}

	if (mrdump_cblock->enabled != MRDUMP_ENABLE_COOKIE) {
		voprintf_debug("Runtime disabled %x\n", mrdump_cblock->enabled);
		mrdump_status_none("Runtime disabled\n");
		goto error;
	}

	voprintf_debug("sram record with mode %d\n", reboot_mode);
	switch (reboot_mode) {
		case AEE_REBOOT_MODE_WDT: {
			/* FIXME: return 1 after LK dump implement */
			return 0;
		}
		case AEE_REBOOT_MODE_NORMAL: {
			/* MRDUMP_KEY reboot*/
			if (ram_console_reboot_by_mrdump_key && ram_console_reboot_by_mrdump_key()) {
				mrdump_cblock->crash_record.reboot_mode = AEE_REBOOT_MODE_MRDUMP_KEY;
				return 1;
			} else
				return 0;
		}
		case AEE_REBOOT_MODE_KERNEL_OOPS:
		case AEE_REBOOT_MODE_KERNEL_PANIC:
		case AEE_REBOOT_MODE_NESTED_EXCEPTION:
		case AEE_REBOOT_MODE_EXCEPTION_KDUMP:
			return 1;
	}
	voprintf_debug("Unsupport exception type\n");
	mrdump_status_none("Unsupport exception type\n");

error:
	mrdump_write_result();
	return 0;
}

int mrdump_run2(void)
{
	if (mrdump_cblock != NULL) {
		kdump_ui(mrdump_cblock);
#ifndef MTK_TC7_FEATURE
#ifdef MTK_PMIC_FULL_RESET
		voprintf_debug("Ready for full pmic reset\n");
		mrdump_write_result();
		mtk_arch_full_reset();
#else
		voprintf_debug("Ready for reset\n");
		mrdump_write_result();
		mtk_arch_reset(1);
#endif
#endif
		mrdump_write_result();
		return 1;
	}
	return 0;
}

void aee_timer_init(struct aee_timer *t)
{
	memset(t, 0, sizeof(struct aee_timer));
}

void aee_timer_start(struct aee_timer *t)
{
	t->start_ms = get_timer_masked();
}

void aee_timer_stop(struct aee_timer *t)
{
	t->acc_ms += (get_timer_masked() - t->start_ms);
	t->start_ms = 0;
}

void *kdump_core_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size)
{
	voprintf_info("kernel page offset %llu\n", kparams->machdesc.page_offset);
	if (kparams->machdesc.page_offset <= 0xffffffffULL) {
		voprintf_info("32b kernel detected\n");
		return kdump_core32_header_init(kparams, kmem_address, kmem_size);
	} else {
		voprintf_info("64b kernel detected\n");
		return kdump_core64_header_init(kparams, kmem_address, kmem_size);
	}
}

#ifdef MTK_3LEVEL_PAGETABLE
vaddr_t scratch_addr(void)
{
	return (vaddr_t)target_get_scratch_address();
}
#endif
