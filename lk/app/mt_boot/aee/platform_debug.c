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
#include <stdlib.h>
#include <dev/aee_platform_debug.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <printf.h>
#include <arch/arm/mmu.h>

unsigned long mrdump_setup_cpm(void)  __attribute__((weak));
unsigned long mrdump_setup_cpm(void)
{
	return 0UL;
}

unsigned int (* plat_dfd20_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_dram_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_cpu_bus_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_spm_data_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_atf_log_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_atf_crash_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_atf_raw_log_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_hvfs_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_sspm_coredump_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_sspm_data_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;
unsigned int (* plat_sspm_log_get)(u64 offset, int *len, CALLBACK dev_write) = NULL;


#define MTK_SIP_LK_DUMP_ATF_LOG_INFO_AARCH32	0x8200010A
#define ATF_CRASH_MAGIC_NO			0xdead1abf
#define ATF_LAST_LOG_MAGIC_NO			0x41544641
#define ATF_DUMP_DONE_MAGIC_NO			0xd07ed07e
#define ATF_SMC_UNK				0xffffffff

static unsigned int atf_log_buf_addr;
static unsigned int atf_log_buf_size;
static unsigned int atf_crash_flag_addr;

static unsigned int save_atf_log(u64 offset, int *len, CALLBACK dev_write);
static void atf_log_init(void);

/* in case that platform didn't support platform_debug_init() */
int platform_debug_init(void) __attribute__((weak));
int platform_debug_init(void)
{
	return 0;
}

int lkdump_debug_init(void)
{
	if (g_boot_arg->boot_mode != DOWNLOAD_BOOT)
		atf_log_init();

	return platform_debug_init();
}

/* function pointer should be set after platform_debug_init() */
unsigned int kedump_plat_savelog(int condition, u64 offset, int *len, CALLBACK dev_write)
{
	switch (condition) {
		case AEE_PLAT_DFD20:
			return (!plat_dfd20_get ? 0 : plat_dfd20_get(offset, len, dev_write));
		case AEE_PLAT_DRAM:
			return (!plat_dram_get ? 0 : plat_dram_get(offset, len, dev_write));
		case AEE_PLAT_CPU_BUS:
			return (!plat_cpu_bus_get ? 0 : plat_cpu_bus_get(offset, len, dev_write));
		case AEE_PLAT_SPM_DATA:
			return (!plat_spm_data_get ? 0 : plat_spm_data_get(offset, len, dev_write));
		case AEE_PLAT_ATF_LAST_LOG:
			return (!plat_atf_log_get ? 0 : plat_atf_log_get(offset, len, dev_write));
		case AEE_PLAT_ATF_CRASH_REPORT:
			return (!plat_atf_crash_get ? 0 : plat_atf_crash_get(offset, len, dev_write));
		case AEE_PLAT_ATF_RAW_LOG:
			return (!plat_atf_raw_log_get ? 0 : plat_atf_raw_log_get(offset, len, dev_write));
		case AEE_PLAT_HVFS:
			return (!plat_hvfs_get ? 0 : plat_hvfs_get(offset, len, dev_write));
		case AEE_PLAT_SSPM_COREDUMP:
			return (!plat_sspm_coredump_get ? 0 : plat_sspm_coredump_get(offset, len, dev_write));
		case AEE_PLAT_SSPM_DATA:
			return (!plat_sspm_data_get ? 0 : plat_sspm_data_get(offset, len, dev_write));
		case AEE_PLAT_SSPM_LAST_LOG:
			return (!plat_sspm_log_get ? 0 : plat_sspm_log_get(offset, len, dev_write));
		default:
			break;
	}
	return 0;
}

void mrdump_write_log(u64 offset_dst, void *data, int len)
{
	dprintf(CRITICAL, "%s: Enter\n", __func__);

	if ((offset_dst > MRDUMP_EXPDB_DRAM_KLOG_OFFSET) || (offset_dst < MRDUMP_EXPDB_BOTTOM_OFFSET)) {
		dprintf(CRITICAL, "%s: access not permitted. offset(0x%llx).\n", __func__, offset_dst);
		return;
	}

	if ((offset_dst - len) < MRDUMP_EXPDB_BOTTOM_OFFSET) {
		dprintf(CRITICAL, "%s: log size(0x%x) too big.\n", __func__, len);
		return;
	}

	int index = partition_get_index("expdb");
	part_dev_t *dev = mt_part_get_device();
	if (index == -1 || dev == NULL) {
		dprintf(CRITICAL, "%s: no %s partition[%d]\n", __func__, "expdb", index);
		return;
	}
#if defined(MTK_NEW_COMBO_EMMC_SUPPORT)  || defined(MTK_TLC_NAND_SUPPORT) || defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_UFS_BOOTING)
	int part_id = partition_get_region(index);
#endif
	u64 part_size = partition_get_size(index);
	if (part_size < offset_dst) {
		dprintf(CRITICAL, "%s: partition size(%llx) is less then reserved (%llx)\n", __func__, part_size, offset_dst);
		return;
	}
	u64 part_offset = partition_get_offset(index) + part_size - offset_dst;

	dprintf(CRITICAL, "%s: offset %lld size %lld\n", __func__, part_offset, part_size);

#if defined(MTK_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
#if defined(MTK_NEW_COMBO_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
	dev->write(dev, (uchar *)data, part_offset, len, part_id);
#else
	dev->write(dev, (uchar *)data, part_offset, len);
#endif
#else
	dev->write(dev, (uchar *)data, part_offset, len, part_id);
#endif
}

void mrdump_read_log(void *data, int len, u64 offset_src)
{
	dprintf(CRITICAL, "%s: Enter\n", __func__);

	if ((offset_src > MRDUMP_EXPDB_DRAM_KLOG_OFFSET) || (offset_src < MRDUMP_EXPDB_BOTTOM_OFFSET)) {
		dprintf(CRITICAL, "%s: access not permitted. offset(0x%llx).\n", __func__, offset_src);
		return;
	}

	if ((offset_src - len) < MRDUMP_EXPDB_BOTTOM_OFFSET) {
		dprintf(CRITICAL, "%s: log size(0x%x) too big.\n", __func__, len);
		return;
	}

	int index = partition_get_index("expdb");
	part_dev_t *dev = mt_part_get_device();
	if (index == -1 || dev == NULL) {
		dprintf(CRITICAL, "%s: no %s partition[%d]\n", __func__, "expdb", index);
		return;
	}
#if defined(MTK_NEW_COMBO_EMMC_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT) || defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_UFS_BOOTING)
	int part_id = partition_get_region(index);
#endif
	u64 part_size = partition_get_size(index);
	if (part_size < offset_src) {
		dprintf(CRITICAL, "%s: partition size(%llx) is less then reserved (%llx)\n", __func__, part_size, offset_src);
		return;
	}
	u64 part_offset = partition_get_offset(index) + part_size - offset_src;

	dprintf(CRITICAL, "%s: offset %lld size %lld\n", __func__, part_offset, part_size);

#if defined(MTK_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
#if defined(MTK_NEW_COMBO_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
	dev->read(dev, part_offset, (uchar *)data, len, part_id);
#else
	dev->read(dev, part_offset, (uchar *)data, len);
#endif
#else
	dev->read(dev, part_offset, (uchar *)data, len, part_id);
#endif
}

#ifdef MTK_3LEVEL_PAGETABLE
static unsigned int save_atf_log(u64 offset, int *len, CALLBACK dev_write)
{
	unsigned int datasize = 0;

	/* ATF log is located in DRAM, we must allocate it first */
	arch_mmu_map(ROUNDDOWN((uint64_t)atf_log_buf_addr, PAGE_SIZE), ROUNDDOWN((uint32_t)atf_log_buf_addr, PAGE_SIZE),
				MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(atf_log_buf_size, PAGE_SIZE));

	*len = atf_log_buf_size;
	datasize = dev_write((void *)atf_log_buf_addr, *len);
	/* Clear ATF crash flag */
	*(unsigned int *)atf_crash_flag_addr = ATF_DUMP_DONE_MAGIC_NO;
	arch_clean_cache_range((addr_t)atf_crash_flag_addr, sizeof(unsigned int));

	dprintf(INFO, "ATF: dev_write:%u, atf_log_buf_addr:0x%x, atf_log_buf_size:%u, crash_flag:0x%x\n", datasize, atf_log_buf_addr, atf_log_buf_size, *(unsigned int *)atf_crash_flag_addr);
	return datasize;
}

static void atf_log_init(void)
{
	unsigned int smc_id = MTK_SIP_LK_DUMP_ATF_LOG_INFO_AARCH32;

	plat_atf_log_get = NULL;
	plat_atf_crash_get = NULL;

	__asm__ volatile("mov r0, %[smcid]\n"
			"smc #0x0\n"
			"mov %[addr], r0\n"
			"mov %[size], r1\n"
			"mov %[type], r2\n"
			:[addr]"=r"(atf_log_buf_addr), [size]"=r"(atf_log_buf_size), [type]"=r"(atf_crash_flag_addr):[smcid]"r"(smc_id):"cc", "r0", "r1" ,"r2", "r3"
	);

	if(atf_log_buf_addr == ATF_SMC_UNK) {
		dprintf(CRITICAL, "LK Dump: atf_log_init not supported\n");
	} else {
		arch_mmu_map(ROUNDDOWN((uint64_t)atf_crash_flag_addr, PAGE_SIZE), ROUNDDOWN((uint32_t)atf_crash_flag_addr, PAGE_SIZE),
						MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, PAGE_SIZE);
		if(atf_log_buf_size == 0x60000){
			dprintf(CRITICAL, "ATF: RAW BUFF\n");
			plat_atf_raw_log_get = save_atf_log;
		}
		else if(ATF_CRASH_MAGIC_NO == *(unsigned int *)atf_crash_flag_addr){
			dprintf(CRITICAL, "ATF: CRASH BUFF\n");
			plat_atf_crash_get = save_atf_log;
		}
		else if(ATF_LAST_LOG_MAGIC_NO == *(unsigned int *)atf_crash_flag_addr){
			dprintf(CRITICAL, "ATF: LAST BUFF\n");
			plat_atf_log_get = save_atf_log;
		}
		dprintf(CRITICAL, "atf_log_buf_addr:0x%x, atf_log_buf_size:%u, atf_crash_flag addr:0x%x, atf_log_type:0x%x\n", atf_log_buf_addr, atf_log_buf_size, atf_crash_flag_addr, *(unsigned int *)atf_crash_flag_addr);
		dprintf(CRITICAL, "plat_atf_log_get:%p, plat_atf_crash_get:%p\n", plat_atf_log_get, plat_atf_crash_get);
	}
}
#else
static unsigned int save_atf_log(u64 offset, int *len, CALLBACK dev_write)
{
	return 0;
}
static void atf_log_init(void)
{
}
#endif
