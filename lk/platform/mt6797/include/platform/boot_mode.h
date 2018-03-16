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

#ifndef _MT_BOOT_MODE_H_
#define _MT_BOOT_MODE_H_

#include <platform/mt_typedefs.h>
#include <platform/partition.h>
#include <mblock.h>

/******************************************************************************
 * FORBIDEN MODE
 ******************************************************************************/
typedef enum {
	F_FACTORY_MODE = 0x0001,
} FORBIDDEN_MODE;

/******************************************************************************
 * LIMITATION
 ******************************************************************************/
#define SEC_LIMIT_MAGIC  0x4C4C4C4C // LLLL

typedef struct {
	unsigned int magic_num;
	FORBIDDEN_MODE forbid_mode;
} SEC_LIMIT;


/* boot type definitions */
typedef enum {
	NORMAL_BOOT = 0,
	META_BOOT = 1,
	RECOVERY_BOOT = 2,
	SW_REBOOT = 3,
	FACTORY_BOOT = 4,
	ADVMETA_BOOT = 5,
	ATE_FACTORY_BOOT = 6,
	ALARM_BOOT = 7,
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
	KERNEL_POWER_OFF_CHARGING_BOOT = 8,
	LOW_POWER_OFF_CHARGING_BOOT = 9,
#endif
	FASTBOOT = 99,
	DOWNLOAD_BOOT = 100,
	UNKNOWN_BOOT
} BOOTMODE;

typedef enum {
	BR_POWER_KEY = 0,
	BR_USB,
	BR_RTC,
	BR_WDT,
	BR_WDT_BY_PASS_PWK,
	BR_TOOL_BY_PASS_PWK,
	BR_2SEC_REBOOT,
	BR_UNKNOWN,
	BR_KERNEL_PANIC,
	BR_WDT_SW,
	BR_WDT_HW
} boot_reason_t;

/*META COM port type*/
typedef enum {
	META_UNKNOWN_COM = 0,
	META_UART_COM,
	META_USB_COM
} META_COM_TYPE;

typedef struct {
	u32 addr;    /* Download Agent address */
	u32 arg1;    /* Download Agent arg 1*/
	u32 arg2;    /* Download Agent arg 2*/
	u32 len;     /* length of da */
	u32 sig_len; /* signature length of da */
} da_info_t;

typedef struct {
	u32 pll_setting_num;
	u32 freq_setting_num;
	u32 low_freq_pll_setting_addr;
	u32 low_freq_cha_setting_addr;
	u32 low_freq_chb_setting_addr;
	u32 high_freq_pll_setting_addr;
	u32 high_freq_cha_setting_addr;
	u32 high_freq_chb_setting_addr;
} vcore_dvfs_info_t;

typedef struct {
	u32 first_volt;
	u32 second_volt;
	u32 third_volt;
	u32 have_550;
} ptp_info_t;


typedef struct {
	u32      maggic_number;
	BOOTMODE boot_mode;
	u32      e_flag;
	u32      log_port;
	u32      log_baudrate;
	u8       log_enable;
	u8       part_num;
	u8       reserved[2];
	u32      dram_rank_num;
	u32      dram_rank_size[4];
	mblock_info_t mblock_info;
	dram_info_t orig_dram_info;
	mem_desc_t lca_reserved_mem;
	mem_desc_t tee_reserved_mem;
	u32      boot_reason;
	META_COM_TYPE meta_com_type;
	u32      meta_com_id;
	u32      boot_time;
	da_info_t da_info;
	SEC_LIMIT sec_limit;
	part_hdr_t *part_info;
	u8 md_type[4];
	u32  ddr_reserve_enable;
	u32  ddr_reserve_success;
	u32  ddr_reserve_ready;
	ptp_info_t ptp_volt_info;
	u32  dram_buf_size;
	u32  meta_uart_port;
	u32  smc_boot_opt;
	u32  lk_boot_opt;
	u32  kernel_boot_opt;
	u32 non_secure_sram_addr;
	u32 non_secure_sram_size;
	char pl_version[8];
	u32 extbuck_fan53526_exist;
} BOOT_ARGUMENT;


typedef enum {
	BOOT_OPT_64S3 = 0,
	BOOT_OPT_64S1,
	BOOT_OPT_32S3,
	BOOT_OPT_32S1,
	BOOT_OPT_64N2,
	BOOT_OPT_64N1,
	BOOT_OPT_32N2,
	BOOT_OPT_32N1,
	BOOT_OPT_UNKNOWN
} boot_option_t;


typedef enum {
	CHIP_SW_VER_01 = 0x0000,
	CHIP_SW_VER_02 = 0x0001
} CHIP_SW_VER;

typedef enum {
	CHIP_INFO_G1 = 0xCA00,
	CHIP_INFO_G2 = 0xCA01
} CHIP_INFO;

typedef struct {      /* RAM configuration */
#ifndef MTK_LM_MODE
	unsigned long start;
	unsigned long size;
#else
	unsigned long long start;
	unsigned long long size;
#endif
} BI_DRAM;


/* ====== Preloader to LK Tags START ===== */

/* boot reason */
#define BOOT_TAG_BOOT_REASON     0x88610001
struct boot_tag_boot_reason {
	u32 boot_reason;
};

/* boot mode */
#define BOOT_TAG_BOOT_MODE       0x88610002
struct boot_tag_boot_mode {
	u32 boot_mode;
};

/* META com port information */
#define BOOT_TAG_META_COM        0x88610003
struct boot_tag_meta_com {
	u32 meta_com_type;
	u32 meta_com_id;
	u32 meta_uart_port;
};

/* log com port information */
#define BOOT_TAG_LOG_COM         0x88610004
struct boot_tag_log_com {
	u32 log_port;
	u32 log_baudrate;
	u32 log_enable;
};

/* memory information */
#define BOOT_TAG_MEM             0x88610005
struct boot_tag_mem {
	u32 dram_rank_num;
	u32 dram_rank_size[4];
	mblock_info_t mblock_info;
	dram_info_t orig_dram_info;
	mem_desc_t lca_reserved_mem;
	mem_desc_t tee_reserved_mem;
};

/* MD information */
#define BOOT_TAG_MD_INFO         0x88610006
struct boot_tag_md_info {
	u32 md_type[4];
};

/* boot time */
#define BOOT_TAG_BOOT_TIME       0x88610007
struct boot_tag_boot_time {
	u32 boot_time;
};

/* DA information */
#define BOOT_TAG_DA_INFO         0x88610008
struct boot_tag_da_info {
	da_info_t da_info;
};

/* security limitation information */
#define BOOT_TAG_SEC_INFO        0x88610009
struct boot_tag_sec_info {
	SEC_LIMIT sec_limit;
};

/* (for dummy AP) partition number */
#define BOOT_TAG_PART_NUM        0x8861000A
struct boot_tag_part_num {
	u32 part_num;
};

/* (for dummy AP) partition info */
#define BOOT_TAG_PART_INFO       0x8861000B
struct boot_tag_part_info {
	part_hdr_t *part_info;
};

/* eflag */
#define BOOT_TAG_EFLAG           0x8861000C
struct boot_tag_eflag {
	u32 e_flag;
};

/* DDR reserve */
#define BOOT_TAG_DDR_RESERVE     0x8861000D
struct boot_tag_ddr_reserve {
	u32 ddr_reserve_enable;
	u32 ddr_reserve_success;
	u32 ddr_reserve_ready;
};

/* DRAM BUFF */
#define BOOT_TAG_DRAM_BUF        0x8861000E
struct boot_tag_dram_buf {
	u32 dram_buf_size;
};

/* VCORE DVFS */
#define BOOT_TAG_VCORE_DVFS      0x8861000F
struct boot_tag_vcore_dvfs {
	vcore_dvfs_info_t vcore_dvfs_info;
};

/* PTP */
#define BOOT_TAG_PTP      0x88610010
struct boot_tag_ptp {
	ptp_info_t ptp_volt_info;
};

#define BOOT_TAG_BOOT_OPT      0x88610011
struct boot_tag_boot_opt {
	u32  smc_boot_opt;
	u32  lk_boot_opt;
	u32  kernel_boot_opt;
};

#define BOOT_TAG_SRAM_INFO      0x88610012
struct boot_tag_sram_info {
	u32 non_secure_sram_addr;
	u32 non_secure_sram_size;
};

#define BOOT_TAG_EXTBUCK_FAN53526       0x88610014
struct boot_tag_extbuck_fan53526 {
    u32 exist;
};

struct boot_tag_header {
	u32 size;
	u32 tag;
};

struct boot_tag {
	struct boot_tag_header hdr;
	union {
		struct boot_tag_boot_reason boot_reason;
		struct boot_tag_boot_mode boot_mode;
		struct boot_tag_meta_com meta_com;
		struct boot_tag_log_com log_com;
		struct boot_tag_mem mem;
		struct boot_tag_md_info md_info;
		struct boot_tag_boot_time boot_time;
		struct boot_tag_da_info da_info;
		struct boot_tag_sec_info sec_info;
		struct boot_tag_part_num part_num;
		struct boot_tag_part_info part_info;
		struct boot_tag_eflag eflag;
		struct boot_tag_ddr_reserve ddr_reserve;
		struct boot_tag_dram_buf dram_buf;
		struct boot_tag_vcore_dvfs vcore_dvfs;
		struct boot_tag_boot_opt boot_opt;
		struct boot_tag_sram_info sram_info;
		struct boot_tag_ptp ptp_volt;
		struct boot_tag_extbuck_fan53526  extbuck_fan53526;
	} u;
};

#define boot_tag_next(t)    ((struct boot_tag *)((u32 *)(t) + (t)->hdr.size))

/* ====== Preloader to LK Tags END ===== */

#define BOOT_ARGUMENT_MAGIC 0x504c504c

extern unsigned int BOOT_ARGUMENT_LOCATION;
extern void boot_mode_select(void);
extern BOOTMODE g_boot_mode;
extern CHIP_INFO mt_get_chip_info(void);
extern CHIP_SW_VER  mt_get_chip_sw_ver(void);
extern int g_is_64bit_kernel;
extern void platform_k64_check(void);
extern BOOT_ARGUMENT *g_boot_arg;
/*FIXME: NAND/eMMC boot. Needo to specifiy by MTK build system. TEMP define here*/
//#define CFG_NAND_BOOT
//#define CFG_MMC_BOOT

#endif
