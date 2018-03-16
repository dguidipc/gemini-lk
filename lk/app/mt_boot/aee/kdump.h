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

#if !defined(__KDUMP_H__)
#define __KDUMP_H__

#include <stdint.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/mt_reg_base.h>
#include <dev/mrdump.h>

/* for LPAE (MTK_3LEVEL_PAGETABLE) */
#include <compiler.h>
#include <arch/arm/mmu.h>

#define MRDUMP_DEV_UNKNOWN 0
#define MRDUMP_DEV_NONE 1
#define MRDUMP_DEV_NULL 2
#define MRDUMP_DEV_ISTORAGE_EXT4 3
#define MRDUMP_DEV_ISTORAGE_VFAT 4

#define MRDUMP_GO_DUMP "MRDUMP06"

/* for dts */
#ifdef CFG_DTB_EARLY_LOADER_SUPPORT

#include <libfdt.h>
extern void *g_fdt;

struct mrdump_reserve_args {
	uint32_t hi_addr;
	uint32_t lo_addr;
	uint32_t hi_size;
	uint32_t lo_size;
};

#endif

// for ext4, InfoLBA (header), 2014/10/03
typedef enum {
	EXT4_1ST_LBA,
	EXT4_2ND_LBA,
	EXT4_CORE_DUMP_SIZE,
	EXT4_USER_FILESIZE,
	EXT4_INFOBLOCK_CRC,
	EXT4_LBA_INFO_NUM
} MRDUMP_LBA_INFO;

#define KZIP_ENTRY_MAX 8
#define LOCALHEADERMAGIC 0x04034b50UL
#define CENTRALHEADERMAGIC 0x02014b50UL
#define ENDOFCENTRALDIRMAGIC 0x06054b50UL

#define KDUMP_CORE_HEADER_SIZE 2 * 4096

struct kzip_entry {
	char *filename;
	int level;
	uint64_t localheader_offset;
	uint32_t comp_size;
	uint32_t uncomp_size;
	uint32_t crc32;
};

struct kzip_file {
	uint32_t reported_size;
	uint32_t wdk_kick_size;
	uint32_t current_size;

	uint32_t entries_num;
	struct kzip_entry zentries[KZIP_ENTRY_MAX];
	void *handle;

	int (*write_cb)(void *handle, void *buf, int size);
};

struct kzip_memlist {
	uint64_t address;
	uint64_t size;
	bool need2map;
};

#define MRDUMP_CB_ADDR (DRAM_PHY_ADDR + 0x1F00000)
#define MRDUMP_CB_SIZE 0x1000

#define MRDUMP_CPU_MAX 16

#define MRDUMP_ENABLE_COOKIE 0x590d2ba3

typedef uint32_t arm32_gregset_t[18];
typedef uint64_t arm64_gregset_t[34];

struct mrdump_crash_record {
	int reboot_mode;

	char msg[128];
	char backtrace[512];

	uint32_t fault_cpu;

	union {
		arm32_gregset_t arm32_regs;
		arm64_gregset_t arm64_regs;
	} cpu_regs[MRDUMP_CPU_MAX];
};

struct mrdump_ksyms_param {
	char     tag[4];
	uint32_t flag;
	uint32_t crc;
	uint64_t start_addr;
	uint32_t size;
	uint32_t addresses_off;
	uint32_t num_syms_off;
	uint32_t names_off;
	uint32_t markers_off;
	uint32_t token_table_off;
	uint32_t token_index_off;
} __attribute__((packed));

struct mrdump_machdesc {
	uint32_t crc;

	uint32_t nr_cpus;

	uint64_t page_offset;
	uint64_t high_memory;

	uint64_t kimage_vaddr;

	uint64_t vmalloc_start;
	uint64_t vmalloc_end;

	uint64_t modules_start;
	uint64_t modules_end;

	uint64_t phys_offset;
	uint64_t master_page_table;

	uint64_t memmap;

	uint64_t dfdmem_pa;  // Reserved for DFD 3.0+

	struct mrdump_ksyms_param kallsyms;
};

struct __attribute__((__packed__)) mrdump_cblock_result {
	char sig[9];
	char status[128];
	char log_buf[2048];
};

struct mrdump_control_block {
	char sig[8];

	struct mrdump_machdesc machdesc;

	uint32_t enabled;
	uint32_t output_fs_lbaooo;

	struct mrdump_crash_record crash_record;
};

struct kzip_file *kzip_open(void *handle, int (*write_cb)(void *handle, void *p, int size));
bool kzip_add_file(struct kzip_file *zf, const struct kzip_memlist *memlist, const char *zfilename);
bool kzip_close(struct kzip_file *zf);

struct mrdump_dev;

struct mrdump_dev *mrdump_dev_emmc_vfat(void);
struct mrdump_dev *mrdump_dev_emmc_ext4(void);
struct mrdump_dev *mrdump_dev_sdcard(void);

int kdump_null_output(const struct mrdump_control_block *kparams, uint64_t total_dump_size);
int mrdump_ext4_output(const struct mrdump_control_block *mrdump_cb, uint64_t total_dump_size, struct mrdump_dev *mrdump_dev);
int mrdump_vfat_output(const struct mrdump_control_block *mrdump_cb, uint64_t total_dump_size, struct mrdump_dev *mrdump_dev);

void *kdump_core_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size);
void *kdump_core32_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size);
void *kdump_core64_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size);

extern u64 physical_memory_size(void);

#ifdef MTK_3LEVEL_PAGETABLE
vaddr_t scratch_addr(void);
#endif

#endif
