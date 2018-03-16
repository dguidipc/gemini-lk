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
#ifndef __CCCI_LD_MD_CORE_H__
#define __CCCI_LD_MD_CORE_H__

#include "ccci_ld_md_log_cfg.h"

/* Log part */
#define ALWAYS_LOG(fmt, args...) do {dprintf(ALWAYS, fmt, ##args);} while (0)
#define DBG_LOG(fmt, args...) do {dprintf(INFO, fmt, ##args);} while (0)

/* Switch log */
#ifdef TAG_DEBUG_LOG_EN
#define TAG_DBG_LOG(fmt, args...) do {dprintf(ALWAYS, fmt, ##args);} while (0)
#else
#define TAG_DBG_LOG(fmt, args...)
#endif
#ifdef PADDING_MEM_DEBUG_LOG_EN
#define PADDING_LOG(fmt, args...) do {dprintf(ALWAYS, fmt, ##args);} while (0)
#else
#define PADDING_LOG(fmt, args...)
#endif
#ifdef LD_IMG_DUMP_LOG_EN
#define LD_DBG_LOG(fmt, args...) do {dprintf(ALWAYS, fmt, ##args);} while (0)
#else
#define LD_DBG_LOG(fmt, args...)
#endif
#ifdef MPU_DBG_LOG_EN
#define MPU_DBG_LOG(fmt, args...) do {dprintf(INFO, fmt, ##args);} while (0)
#else
#define MPU_DBG_LOG(fmt, args...)
#endif
#ifdef ENABLE_DT_DBG_LOG
#define DT_DBG_LOG(fmt, args...) do {dprintf(INFO, fmt, ##args);} while (0)
#else
#define DT_DBG_LOG(fmt, args...)
#endif


/***************************************************************************************************
** Core Global variable and macro defination part
***************************************************************************************************/

#define MAX_MD_NUM      4
#define MAX_PADDING_NUM_V5_HDR  3
#define MAX_PADDING_NUM_V6_HDR  8

#define MAIN_IMG_MIN_SIZE   0x8000
#define MPU_64K_ALIGN_MASK  (~(0x10000-1))
#define MPU_64K_VALUE       (0x10000)
#define MEM_4K_ALIGN_MASK   (~(0x1000-1))
#define MEM_4K_VALUE        (0x1000)

enum {
	MD_SYS1 = 0,
	MD_SYS2,
	MD_SYS3,
	MD_SYS4,
};

/***************************************************************************************************
** Sub module: ccci_lk_info (LK to Kernel arguments and information)
**  Using share memory and device tree
**  ccci_lk_info structure is stored at device tree
**  other more detail parameters are stored at share memory
***************************************************************************************************/
typedef struct _ccci_lk_info {
	unsigned long long lk_info_base_addr;
	unsigned int       lk_info_size;
	int                lk_info_err_no;
	int                lk_info_version;
	int                lk_info_tag_num;
	unsigned int       lk_info_ld_flag;
	int                lk_info_ld_md_errno[MAX_MD_NUM];
} ccci_lk_info_t;

#define MAX_LK_INFO_SIZE    (0x10000)
#define CCCI_TAG_NAME_LEN   (64)
#define CCCI_LK_INFO_VER    (2)

typedef struct _ccci_tag {
	char tag_name[CCCI_TAG_NAME_LEN];
	unsigned int data_offset;
	unsigned int data_size;
	unsigned int next_tag_offset;
} ccci_tag_t;

/***************************************************************************************************
** Sub sectoin:
**   modem info
***************************************************************************************************/
typedef struct _modem_info {
	unsigned long long base_addr;
	unsigned int resv_mem_size;
	char md_id;
	char errno;
	char md_type;
	char ver;
	int load_size;/*ROM + Check header*/
	int ro_rw_size;
} modem_info_t;

/***************************************************************************************************
** Sub sectoin:
**   partition header
***************************************************************************************************/
#define IMG_TYPE_ID_OFFSET (0)
#define IMG_TYPE_RESERVED0_OFFSET (8)
#define IMG_TYPE_RESERVED1_OFFSET (16)
#define IMG_TYPE_GROUP_OFFSET (24)

#define IMG_TYPE_ID_MASK (0xffU << IMG_TYPE_ID_OFFSET)
#define IMG_TYPE_RESERVED0_MASK (0xffU << IMG_TYPE_RESERVED0_OFFSET)
#define IMG_TYPE_RESERVED1_MASK (0xffU << IMG_TYPE_RESERVED1_OFFSET)
#define IMG_TYPE_GROUP_MASK (0xffU << IMG_TYPE_GROUP_OFFSET)

#define IMG_TYPE_GROUP_AP (0x00U << IMG_TYPE_GROUP_OFFSET)
#define IMG_TYPE_GROUP_MD (0x01U << IMG_TYPE_GROUP_OFFSET)
#define IMG_TYPE_GROUP_CERT (0x02U << IMG_TYPE_GROUP_OFFSET)

/* AP group */
#define IMG_TYPE_IMG_AP_BIN (0x00 | IMG_TYPE_GROUP_AP)

/* MD group */
#define IMG_TYPE_IMG_MD_LTE (0x00 | IMG_TYPE_GROUP_MD)
#define IMG_TYPE_IMG_MD_C2K (0x01 | IMG_TYPE_GROUP_MD)

/* CERT group */
#define IMG_TYPE_CERT1 (0x00 | IMG_TYPE_GROUP_CERT)
#define IMG_TYPE_CERT1_MD (0x01 | IMG_TYPE_GROUP_CERT)
#define IMG_TYPE_CERT2 (0x02 | IMG_TYPE_GROUP_CERT)

#define SEC_IMG_TYPE_LTE    (0x01000000)
#define SEC_IMG_TYPE_C2K    (0x01000001)
#define SEC_IMG_TYPE_DSP    (0x00000000)
#define SEC_IMG_TYPE_ARMV7  (0x00000000)

#define IMG_MAGIC           0x58881688
#define EXT_MAGIC           0x58891689

#define IMG_NAME_SIZE       32
#define IMG_HDR_SIZE        512

extern int retrieve_info_num;

typedef union {
	struct {
		unsigned int magic;     /* always IMG_MAGIC */
		unsigned int dsize;     /* image size, image header and padding are not included */
		char name[IMG_NAME_SIZE];
		unsigned int maddr;     /* image load address in RAM */
		unsigned int mode;      /* maddr is counted from the beginning or end of RAM */
		/* extension */
		unsigned int ext_magic;     /* always EXT_MAGIC */
		unsigned int hdr_size;      /* header size is 512 bytes currently, but may extend in the future */
		unsigned int hdr_version;   /* see HDR_VERSION */
		unsigned int img_type;      /* please refer to #define beginning with SEC_IMG_TYPE_ */
		unsigned int img_list_end;  /* end of image list? 0: this image is followed by another image 1: end */
		unsigned int align_size;    /* image size alignment setting in bytes, 16 by default for AES encryption */
		unsigned int dsize_extend;  /* high word of image size for 64 bit address support */
		unsigned int maddr_extend;  /* high word of image load address in RAM for 64 bit address support */
	} info;
	unsigned char data[IMG_HDR_SIZE];
} IMG_HDR_T;

/***************************************************************************************************
** Sub section:
**   modem/dsp check header
***************************************************************************************************/
struct md_check_header_comm {
	unsigned char check_header[12];  /* magic number is "CHECK_HEADER"*/
	unsigned int  header_verno;   /* header structure version number */
	unsigned int  product_ver;     /* 0x0:invalid; 0x1:debug version; 0x2:release version */
	unsigned int  image_type;       /* 0x0:invalid; 0x1:2G modem; 0x2: 3G modem */
	unsigned char platform[16];   /* MT6573_S01 or MT6573_S02 */
	unsigned char build_time[64];   /* build time string */
	unsigned char build_ver[64];     /* project version, ex:11A_MD.W11.28 */

	unsigned char bind_sys_id;     /* bind to md sys id, MD SYS1: 1, MD SYS2: 2 */
	unsigned char ext_attr;       /* no shrink: 0, shrink: 1*/
	unsigned char reserved[2];     /* for reserved */

	unsigned int  mem_size;       /* md ROM/RAM image size requested by md */
	unsigned int  md_img_size;     /* md image size, exclude head size*/
} __attribute__((packed));

struct md_check_header_v1 {
	struct md_check_header_comm common; /* common part */
	unsigned int  ap_md_smem_size;     /* share memory size */
	unsigned int  size;           /* the size of this structure */
} __attribute__((packed));

struct md_check_header_v3 {
	struct md_check_header_comm common; /* common part */
	unsigned int  rpc_sec_mem_addr;  /* RPC secure memory address */

	unsigned int  dsp_img_offset;
	unsigned int  dsp_img_size;
	unsigned char reserved2[88];

	unsigned int  size;           /* the size of this structure */
} __attribute__((packed));

typedef struct _md_regin_info {
	unsigned int region_offset;
	unsigned int region_size;
} md_regin_info;

struct md_check_header_v4 {
	struct md_check_header_comm common; /* common part */
	unsigned int  rpc_sec_mem_addr;  /* RPC secure memory address */

	unsigned int  dsp_img_offset;
	unsigned int  dsp_img_size;

	unsigned int  region_num;    /* total region number */
	md_regin_info region_info[8];    /* max support 8 regions */
	unsigned int  domain_attr[4];    /* max support 4 domain settings, each region has 4 control bits*/

	unsigned char reserved2[4];

	unsigned int  size;           /* the size of this structure */
} __attribute__((packed));

typedef struct _free_padding_block {
	unsigned int start_offset;
	unsigned int length;
} free_padding_block_t;

struct md_check_header_v5 {
	struct md_check_header_comm common; /* common part */
	unsigned int  rpc_sec_mem_addr;  /* RPC secure memory address */

	unsigned int  dsp_img_offset;
	unsigned int  dsp_img_size;

	unsigned int  region_num;    /* total region number */
	md_regin_info region_info[8];    /* max support 8 regions */
	unsigned int  domain_attr[4];    /* max support 4 domain settings, each region has 4 control bits*/

	unsigned int  arm7_img_offset;
	unsigned int  arm7_img_size;

	free_padding_block_t padding_blk[MAX_PADDING_NUM_V5_HDR]; /* should be 3 */
	unsigned int  ap_md_smem_size;
	unsigned int  md_to_md_smem_size;

	unsigned int  ramdisk_offset;
	unsigned int  ramdisk_size;

	unsigned char reserved_1[16];

	unsigned int  size; /* the size of this structure */
};

struct md_check_header_v6 {
	struct md_check_header_comm common; /* common part */
	unsigned int  rpc_sec_mem_addr;  /* RPC secure memory address */

	unsigned int  dsp_img_offset;
	unsigned int  dsp_img_size;

	unsigned int  region_num;    /* total region number */
	md_regin_info region_info[8];    /* max support 8 regions */
	unsigned int  domain_attr[4];    /* max support 4 domain settings, each region has 4 control bits*/

	unsigned int  arm7_img_offset;
	unsigned int  arm7_img_size;

	free_padding_block_t padding_blk[MAX_PADDING_NUM_V6_HDR]; /* should be 8 */

	unsigned int  ap_md_smem_size;
	unsigned int  md_to_md_smem_size;

	unsigned int ramdisk_offset;
	unsigned int ramdisk_size;

	unsigned char reserved_1[144];

	unsigned int  size; /* the size of this structure */
};

struct image_section_desc {
	unsigned int offset;
	unsigned int size;
	unsigned int mpu_attr;
	unsigned int ext_flag;
	unsigned int relate_idx;
};

/* dsp check header */
struct dsp_check_header {
	unsigned char check_header[8];
	unsigned char file_info_H[12];
	unsigned char unknown[52];
	unsigned char chip_id[16];
	unsigned char dsp_info[48];
};

/***************************************************************************************************
** Sub section:
**   padding memory for v5 check hdr
***************************************************************************************************/
/* This function only support 3 cases
**   case 0: |======|======| ==> |++====|======|
**   case 1: |======|======| ==> |==++==|======|
**   case 2: |======|======| ==> |====++|======|
**   case 4: |======|======| ==> |=====+|+=====| NOT suppose this case !!!!
*/
#define VALID_PADDING       (1<<0)
#define NEED_MPU_MORE       (1<<1)
#define NEED_REMOVE     (1<<2)
#define MD_ALL_RANGE        (1<<3)

#define LOGIC_BINDING_IDX_START 0x1000

struct padding_tag {
	free_padding_block_t padding_mem;
	int status;
};

/***************************************************************************************************
** Sub section:
**   Telephony operation parsing and prepare part
***************************************************************************************************/
struct opt_cfg {
	char *name;
	int val;
};

/* 0 | 0 | Lf | Lt | W | C | T | G */
#define MD_CAP_KEY		(0x5A<<24)
#define MD_CAP_GSM		(1<<0)
#define MD_CAP_TDS_CDMA		(1<<1)
#define MD_CAP_CDMA2000		(1<<2)
#define MD_CAP_WCDMA		(1<<3)
#define MD_CAP_TDD_LTE		(1<<4)
#define MD_CAP_FDD_LTE		(1<<5)
#define MD_CAP_MASK		(MD_CAP_GSM|MD_CAP_TDS_CDMA|MD_CAP_WCDMA|MD_CAP_TDD_LTE|MD_CAP_FDD_LTE|MD_CAP_CDMA2000)


typedef enum {
	RAT_VER_DEFAULT = 0,
	RAT_VER_R8,
	RAT_VER_90,
	RAT_VER_91_92,
	RAT_VER_93,
} lk_md_generation_t;

/***************************************************************************************************
** Sub section
**   image loading part for each image list
**   NOTE: this structure has a duplicate one at platform code
***************************************************************************************************/
#define LD_IMG_NO_VERIFY    (1<<0)
#define DUMMY_AP_FLAG       LD_IMG_NO_VERIFY

typedef int (*ld_md_assistant_t)(void *load_info, void *data);

typedef struct _download_info {
	int img_type;   /* Main image, or plug-in image */
	char    *partition_name;
	char    *image_name;
	int max_size;
	int img_size;
	int ext_flag;
	unsigned char *mem_addr;
	ld_md_assistant_t ass_func;
} download_info_t;

enum {
	main_img = 1,
	dsp_img,
	armv7_img,
	ramdisk_img,
	l1_core_img,
	max_img_num
};


/* This function is used by common and platform code to use */
void *ccci_request_mem(unsigned int mem_size, unsigned long long limit, unsigned long align);
unsigned int str2uint(char *str);
int ccci_retrieve_mem(unsigned char *addr, int size);

/*************************************************************************/
/* Sub module exported API                                               */
/*************************************************************************/
/* ---  Tag info --- */
int insert_ccci_tag_inf(char *name, char *data, unsigned int size);
void ccci_lk_tag_info_init(unsigned long long base_addr);
void ccci_lk_info_ctl_dump(void);
void update_md_err_to_lk_info(int md_id, int error);
int get_md_err_from_lk_info(int md_id);
void update_md_load_flag_to_lk_info(int md_id);
void update_common_err_to_lk_info(int error);
void ccci_lk_info_re_cfg(unsigned long long base_addr, unsigned int size);
/* ---  Tel option --- */
int ccci_alloc_local_cmd_line_buf(int size);
//int get_tel_opt_array_case_num(void);
int prepare_tel_fo_setting(void);
int ccci_get_opt_val(char opt[]);
void ccci_free_local_cmd_line_buf(void);
/* ---  assistance --- */
int verify_main_img_check_header(modem_info_t *info);
int ass_func_for_v5_normal_img(void *load_info, void *data);
int ass_func_for_v6_normal_img(void *load_info, void *data);
int ass_func_for_v1_normal_img(void *load_info, void *data);
int ass_func_for_v1_r8_normal_img(void *load_info, void *data);
int ass_func_for_v5_md_only_img(void *load_info, void *data);
int ass_func_for_v6_md_only_img(void *load_info, void *data);
int ass_func_for_v1_md_only_img(void *load_info, void *data);
/* ---  padding memory --- */
int retrieve_free_padding_mem_v5_hdr(modem_info_t *info,
                                     struct image_section_desc mem_tbl[], void *hdr, int mpu_num);
int retrieve_free_padding_mem_v6_hdr(modem_info_t *info,
                                     struct image_section_desc mem_tbl[], void *hdr, int mpu_num);
void log_retrieve_info(unsigned char *addr, int size);
/* ---  errno string --- */
char *ld_md_errno_to_str(int errno);


/* API that implemented by other module */
extern int mblock_create(mblock_info_t *mblock_info, dram_info_t *orig_dram_info, u64 addr, u64 size);
extern void arch_sync_cache_range(addr_t start, size_t len);
extern char *get_env(char *name);
extern char *get_ro_env(char *name)__attribute__((weak));

extern u32 sec_img_auth_init(u8* part_name, u8* img_name, u32 feature_mask);
extern u32 sec_img_auth(u8 *buf, u32 buf_sz);
extern u32 sec_get_c2kmd_sbcen(void);
extern u32 sec_get_ltemd_sbcen(void);
extern u32 get_policy_entry_idx(u8* part_nam);
extern u32 get_vfy_policy(u8* part_nam);

/***************************************************************************************************
****************************************************************************************************
** Sub section:
**   Export API from platform, for reasons the following, weak key word added.
**    1. in order to make code compatible;
**    2. avoid build error that some old platform does not support
****************************************************************************************************
***************************************************************************************************/
extern int plat_dt_reserve_mem_size_fixup(void *fdt)__attribute__((weak));
extern long long plat_ccci_get_ld_md_plat_setting(char cfg_name[])__attribute__((weak));
extern int plat_get_padding_mpu_num(void)__attribute__((weak));
extern int plat_apply_hw_remap_for_md_ro_rw(void*)__attribute__((weak));
extern int plat_apply_hw_remap_for_md_smem(void* addr, int size)__attribute__((weak));
extern int plat_send_mpu_info_to_platorm(void*, void*)__attribute__((weak));
extern int plat_apply_platform_setting(int)__attribute__((weak));
extern void plat_post_hdr_info(void* hdr, int ver, int id)__attribute__((weak));

/* HAL API */
long long ccci_hal_get_ld_md_plat_setting(char cfg_name[]);
int  ccci_hal_get_mpu_num_for_padding_mem(void);
int  ccci_hal_apply_hw_remap_for_md_ro_rw(void *md_info);
int  ccci_hal_apply_hw_remap_for_md_smem(void *addr, int size);
int  ccci_hal_send_mpu_info_to_platorm(void *md_info, void *mem_info);
int  ccci_hal_apply_platform_setting(int load_flag);
void ccci_hal_post_hdr_info(void *hdr, int ver, int id);

#endif
