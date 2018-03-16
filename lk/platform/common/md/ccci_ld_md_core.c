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

#include <sys/types.h>
#include <stdint.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <libfdt.h>
#include <platform/mt_gpt.h>
#include <platform/sec_export.h>
#include <debug.h>
#include "ccci_ld_md_core.h"
#include "ccci_ld_md_errno.h"

#define MODULE_NAME "LK_LD_MD"

/***************************************************************************************************
* Quick search pattern:
* 1. Download list
***************************************************************************************************/

/***************************************************************************************************
** Sub section:
**   Memory operation: Reserve, Resize, Return
***************************************************************************************************/
extern BOOT_ARGUMENT *g_boot_arg;

static int ccci_resize_reserve_mem(unsigned char *addr, int old_size, int new_size)
{
	return mblock_resize(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, (u64)(unsigned long)addr,
	                     (u64)old_size, (u64)new_size);
}

/* This function will export to platform code to use */
void *ccci_request_mem(unsigned int mem_size, unsigned long long limit, unsigned long align)
{
	void *resv_addr;
	resv_addr = (void *)((unsigned long)mblock_reserve(&g_boot_arg->mblock_info, mem_size, align, limit, RANKMAX));
	ALWAYS_LOG("request size: 0x%08x, get start address: %p\n", mem_size, resv_addr);
	return resv_addr;
}

int ccci_retrieve_mem(unsigned char *addr, int size)
{
	ALWAYS_LOG("  0x%xB retrieved by AP\n", (int)size);
	log_retrieve_info(addr, size);
	return mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, (u64)(unsigned long)addr,(u64)size);
}

/***************************************************************************************************
** Sub sectoin:
**   modem info
***************************************************************************************************/
static modem_info_t s_g_md_ld_status[MAX_MD_NUM];
static int s_g_md_ld_record_num;
static void add_hdr_info(modem_info_t tbl[], modem_info_t *hdr)
{
	tbl[s_g_md_ld_record_num].base_addr = hdr->base_addr;
	tbl[s_g_md_ld_record_num].resv_mem_size = hdr->resv_mem_size;
	tbl[s_g_md_ld_record_num].errno = hdr->errno;
	tbl[s_g_md_ld_record_num].md_id = hdr->md_id;
	tbl[s_g_md_ld_record_num].ver = 0;
	tbl[s_g_md_ld_record_num].md_type = hdr->md_type;
	tbl[s_g_md_ld_record_num].load_size = hdr->load_size;
	tbl[s_g_md_ld_record_num].ro_rw_size = hdr->ro_rw_size;
	s_g_md_ld_record_num++;
}

/***************************************************************************************************
** Sub section:
**   String to unsigned int lib function, pure software code
***************************************************************************************************/
unsigned int str2uint(char *str)
{
	/* max 32bit integer is 4294967296, buf[16] is enough */
	char buf[16];
	int i;
	int num;
	int base = 10;
	int ret_val;
	if (NULL == str)
		return 0;

	i = 0;
	while (i<16) {
		/* Format it */
		if ((str[i] == 'X') || (str[i] == 'x')) {
			buf[i] = 'x';
			if (i != 1)
				return 0; /* not 0[x]XXXXXXXX */
			else if (buf[0] != '0')
				return 0; /* not [0]xXXXXXXXX */
			else
				base = 16;
		} else if ((str[i] >= '0') && (str[i] <= '9'))
			buf[i] = str[i];
		else if ((str[i] >= 'a') && (str[i] <= 'f')) {
			if (base != 16)
				return 0;
			buf[i] = str[i];
		} else if ((str[i] >= 'A') && (str[i] <= 'F')) {
			if (base != 16)
				return 0;
			buf[i] = str[i] - 'A' + 'a';
		} else if (str[i] == 0) {
			buf[i] = 0;
			i++;
			break;
		} else
			return 0;

		i++;
	}

	num = i-1;
	ret_val = 0;
	if (base == 10) {
		for (i=0; i<num; i++)
			ret_val = ret_val*10 + buf[i] - '0';
	} else if (base == 16) {
		for (i=2; i<num; i++) {
			if (buf[i] >= 'a')
				ret_val = ret_val*16 + buf[i] - 'a' + 10;
			else
				ret_val = ret_val*16 + buf[i] - '0';
		}
	}
	return ret_val;
}

/***************************************************************************************************
** Sub section:
**   Load raw data from partition, support function
***************************************************************************************************/
static int ccci_load_raw_data(char *part_name, unsigned char *mem_addr, unsigned int offset, int size)
{
	part_dev_t *dev;
	part_t *part;
	int len;
#if defined(MTK_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
	u64 storage_start_addr;
#else
	unsigned long storage_start_addr;
#endif

	LD_DBG_LOG("ccci_load_raw_data, name[%s], mem_addr[%p], offset[%x], size[%x]\n",
	           part_name, mem_addr, offset, size);

	dev = mt_part_get_device();
	if (!dev) {
		ALWAYS_LOG("load_md_partion_hdr, dev = NULL\n");
		return -LD_ERR_PT_DEV_NULL;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		ALWAYS_LOG("mt_part_get_partition (%s), part = NULL\n",part_name);
		return -LD_ERR_PT_NOT_FOUND;
	}

#if defined(MTK_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
	storage_start_addr = (u64)part->start_sect * dev->blkdev->blksz;
#else
	storage_start_addr = part->start_sect * dev->blkdev->blksz;
#endif
	LD_DBG_LOG("image[%s](%d) addr is 0x%llx in partition\n", part_name, offset, (u64)storage_start_addr);
	LD_DBG_LOG(" > to 0x%llx (at dram), size:0x%x, part id:%d\n",(u64)((unsigned long)mem_addr), size, part->part_id);

#if defined(MTK_UFS_BOOTING) || (defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT))
	len = dev->read(dev, storage_start_addr+offset, mem_addr, size, part->part_id);
#else
	len = dev->read(dev, storage_start_addr+offset, mem_addr, size);
#endif

	if (len < 0) {
		ALWAYS_LOG("[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -LD_ERR_PT_READ_RAW_FAIL;
	}

	return len;
}

static int free_not_used_reserved_memory(unsigned char *md_start_addr, int reserved, int required)
{
	ALWAYS_LOG("md memory require:0x%x, reserved:0x%x\n", required, reserved);

	/* Resize if acutal memory less then reserved*/
	if (required == 0) {
		ALWAYS_LOG("using default reserved\n");
		return reserved;
	} else if (required < reserved) {
		/* Resize reserved memory */
		ccci_resize_reserve_mem(md_start_addr, reserved, required);
	}
	return required;
}

/* --- load raw data to DRAM that listed at table --- */
static int ld_img_at_list(download_info_t img_list[], modem_info_t *info, unsigned long long limit, unsigned long align)
{
	IMG_HDR_T *p_hdr = NULL;
	int load_size;
	int md_mem_resv_size = 0;
	int md_mem_required_size = 0;
	unsigned char *md_resv_mem_addr = NULL;
	int curr_img_size;
	int ret = 0;
	int i;
	download_info_t *curr;

	/* Security policy */
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int policy_entry_idx = 0;
	unsigned int img_auth_required = 0;
	unsigned int verify_hash = 0;
	unsigned int lte_sbc_en = sec_get_ltemd_sbcen();
	unsigned int c2k_sbc_en = sec_get_c2kmd_sbcen();
	int time_md_auth = get_timer(0);
	int time_md_auth_init = get_timer(0);
	unsigned int sec_feature_mask = 0;
#endif

	if (img_list == NULL) {
		ALWAYS_LOG("image list is NULL!\n");
		ret = -LD_ERR_PT_IMG_LIST_NULL;
		return ret;
	}

	/* allocate partition header to reserved memory fisrt */
	p_hdr = (IMG_HDR_T *)malloc(sizeof(IMG_HDR_T));
	if (p_hdr==NULL) {
		ALWAYS_LOG("alloc mem for hdr fail\n");
		ret = -LD_ERR_PT_ALLOC_HDR_MEM_FAIL;
		goto _MD_Exit;
	}

	/* load image one by one */
	curr = img_list;
	while (curr->img_type != 0) {
		/* do fisrt step check */
		ALWAYS_LOG("load ptr[%s] hdr\n", curr->partition_name);

		/* By pass ext image that no need to load after query main image setting */
		if ((curr->img_type != main_img) &&
		        ((curr->mem_addr == NULL) || (curr->img_size == 0))) {
			/* update image pointer to next*/
			curr++;
			continue;
		}
		load_size = (int)ccci_load_raw_data(curr->partition_name, (unsigned char*)p_hdr, 0,
		                                    sizeof(IMG_HDR_T));
		if (load_size != sizeof(IMG_HDR_T)) {
			ALWAYS_LOG("load hdr fail(%d)\n", load_size);
			ret = -LD_ERR_PT_READ_HDR_SIZE_ABNORMAL;
			goto _MD_Exit;
		}
		if (p_hdr->info.magic!=IMG_MAGIC) {
			if (curr->img_type != main_img && curr->ext_flag & LD_IMG_NO_VERIFY) {
				ALWAYS_LOG("by pass loading image %s\n", curr->image_name);
				curr++;
				continue;
			}
			ALWAYS_LOG("invalid magic(%x):(%x)ref\n", p_hdr->info.magic, IMG_MAGIC);
			ret = -LD_ERR_PT_P_HDR_MAGIC_MIS_MATCH;
			goto _MD_Exit;
		}
		if ((curr->img_type == main_img) && (p_hdr->info.dsize < MAIN_IMG_MIN_SIZE)) { /* 32kB */
			ALWAYS_LOG("img size abnormal,size(0x%x)\n", p_hdr->info.dsize);
			ret = -LD_ERR_PT_MAIN_IMG_SIZE_ABNORMAL;
			goto _MD_Exit;
		}

		/* Dummy AP no need do verify */
		if ((curr->ext_flag & LD_IMG_NO_VERIFY) == 0) {
			/* Check sec policy to see if verification is required */
#ifdef MTK_SECURITY_SW_SUPPORT
			policy_entry_idx = get_policy_entry_idx((unsigned char *)curr->partition_name);
			ALWAYS_LOG("policy_entry_idx = %d \n", policy_entry_idx);

			img_auth_required = get_vfy_policy(policy_entry_idx);
			ALWAYS_LOG("img_auth_required = %d \n", img_auth_required);

			/* do verify md cert-chain if need */

			time_md_auth_init = get_timer(0);
			if (img_auth_required) {
#ifdef MTK_SECURITY_ANTI_ROLLBACK
				sec_feature_mask |= SEC_FEATURE_MASK_ANTIROLLBACK;
#endif
				ret = sec_img_auth_init((unsigned char *)curr->partition_name, (unsigned char *)curr->image_name, sec_feature_mask);

				if (0 != ret) {
					ALWAYS_LOG("img cert-chain verification fail: %d\n", ret);
					ret = -LD_ERR_PT_CERT_CHAIN_FAIL;
					goto _MD_Exit;
				}
#ifdef MTK_SECURITY_ANTI_ROLLBACK
				ret = sec_rollback_check(1);
				if (0 != ret)
					goto _MD_Exit;
#endif
				ALWAYS_LOG("Verify MD cert chain %s cost %d ms\n",curr->partition_name, (int)get_timer(time_md_auth_init));
			}
#else
#ifdef MD_RMA_SUPPORT
			ret = sec_set_md_default_pubk_hash();
			if (0 != ret)
					goto _MD_Exit;
#endif
#endif
		} else if (strcmp(p_hdr->info.name, curr->image_name) != 0) {
			ALWAYS_LOG("img name check fail,img name:[%s], should be:[%s]\n", p_hdr->info.name, curr->image_name);
			ret = -LD_ERR_PT_CHK_IMG_NAME_FAIL;
			goto _MD_Exit;
		}

		/* Verity pass, then begin load to dram step */
		LD_DBG_LOG("dump p_hdr info\n");
		LD_DBG_LOG(" p_hdr->info.magic:%x\n", p_hdr->info.magic);
		LD_DBG_LOG(" p_hdr->info.dsize:%x\n", p_hdr->info.dsize);
		LD_DBG_LOG(" p_hdr->info.name:%s\n", p_hdr->info.name);
		LD_DBG_LOG(" p_hdr->info.mode:%x\n", p_hdr->info.mode);
		LD_DBG_LOG(" p_hdr->info.hdr_size:%x\n", p_hdr->info.hdr_size);

		/* load data to memory */
		curr_img_size = (int)p_hdr->info.dsize;
		if (curr_img_size > curr->max_size) {
			ALWAYS_LOG("image size abnormal r:[0x%x]<>a:[0x%x]\n",
			           p_hdr->info.dsize, curr->max_size);
			ret = -LD_ERR_PT_IMG_TOO_LARGE;
			goto _MD_Exit;
		}

		if ((curr->img_size != 0) && (curr_img_size > curr->img_size)) {
			ALWAYS_LOG("image size not sync to chk_hdr hdr:[0x%x]<>a:[0x%x]\n",
			           curr->img_size, p_hdr->info.dsize);
			ret = -LD_ERR_PT_IMG_SIZE_NOT_SYNC_CHK_HDR;
			goto _MD_Exit;
		}

		if (curr->img_type == main_img) { /* Main image need allocate memory first */
			/* Reserve memory with max requirement */
			curr->mem_addr = ccci_request_mem(curr->max_size, limit, align);
			if (curr->mem_addr == NULL) {
				ALWAYS_LOG("allocate MD memory fail\n");
				ret = -LD_ERR_PT_ALLOC_MD_MEM_FAIL;
				goto _MD_Exit;
			}
			md_resv_mem_addr = curr->mem_addr;
			md_mem_resv_size = curr->max_size;
			/* ext image max size will update after main image loaded */
		}

		load_size = ccci_load_raw_data(curr->partition_name, curr->mem_addr,
		                               sizeof(IMG_HDR_T), p_hdr->info.dsize);
		if (load_size != curr_img_size) {
			ALWAYS_LOG("load image fail:%d\n", load_size);
			ret = -LD_ERR_PT_LD_IMG_DATA_FAIL;
			goto _MD_Exit;
		}
		curr->img_size = load_size;

		/* Calcualte size that add padding */
		curr_img_size = (curr_img_size + p_hdr->info.align_size - 1) & (~(p_hdr->info.align_size -1));
		/* Clear padding data to 0 */
		for (i = 0; i < curr_img_size - (int)p_hdr->info.dsize; i++)
			(curr->mem_addr)[p_hdr->info.dsize + i] = 0;

		/* Dummy AP no need do verify */
		if ((curr->ext_flag & LD_IMG_NO_VERIFY) == 0) {
#ifdef MTK_SECURITY_SW_SUPPORT
			/* Verify image hash value if needed */
			if (img_auth_required) {
				/* Verify md image hash value */
				/* When SBC_EN is fused, bypass sec_img_auth */
				verify_hash = 1;
				time_md_auth = get_timer(0);

				if (!strncmp((char const *)curr->partition_name, "md1img",6)) {
					if (0x01 == lte_sbc_en) {
						ALWAYS_LOG("LTE now, and lte sbc en = 0x1 \n");
						/* Bypass image hash verification at AP side*/
						verify_hash = 0;
					} else
						ALWAYS_LOG("LTE now, and lte sbc en != 0x1 \n");
				}
				if (!strncmp((char const *)curr->partition_name, "md3img",6)) {
					if (0x01 == c2k_sbc_en) {
						ALWAYS_LOG("C2K now, and c2k sbc en = 0x1 \n");
						/* Bypass image hash verification at AP side*/
						verify_hash = 0;
					} else
						ALWAYS_LOG("C2K now, and c2k sbc en != 0x1 \n");
				}
				if (1 == verify_hash) {
					time_md_auth = get_timer(0);
					ret = sec_img_auth(curr->mem_addr, curr_img_size);
					if (0 != ret) {
						ALWAYS_LOG("image hash verification fail: %d\n",ret);
						ret = -LD_ERR_PT_HASH_CHK_FAIL;
						goto _MD_Exit;
					}
					ALWAYS_LOG("Image hash verification success: %d\n",ret);
					ALWAYS_LOG("Verify MD image hash %s cost %d ms\n",curr->partition_name, (int)get_timer(time_md_auth));
				}
			}
#endif
		}

		if (curr->img_type == main_img) { /* Main image need parse header for ext image*/
			info->base_addr = (unsigned long long)((unsigned long)curr->mem_addr);
			info->resv_mem_size = md_mem_resv_size;
			info->load_size = curr->img_size;
			ret = verify_main_img_check_header(info);

			if (ret < 0) {
				ALWAYS_LOG("md check header verify fail:%d\n", ret);
				goto _MD_Exit;
			}

			if (curr->ass_func) {
				ret = curr->ass_func((void*)info, (void*)img_list);
				if (ret < 0) {
					ALWAYS_LOG("assistan func process fail:%d\n", ret);
					goto _MD_Exit;
				}
			}
			md_mem_required_size = info->resv_mem_size;
		}

		/* update image pointer */
		curr++;
	}

	md_mem_resv_size = free_not_used_reserved_memory(md_resv_mem_addr, md_mem_resv_size, md_mem_required_size);

	/* Free part header memory */
	if (p_hdr) {
		free(p_hdr);
		p_hdr = NULL;
	}

	/* Sync cache to make sure all data flash to DRAM to avoid MPU violation */
	arch_sync_cache_range((addr_t)md_resv_mem_addr, (size_t)md_mem_resv_size);

	info->errno = 0;

	return 0;
_MD_Exit:
	if (p_hdr) {
		free(p_hdr);
		p_hdr = NULL;
	}

	if (md_resv_mem_addr) {
		ALWAYS_LOG("Free reserved memory\n");
		ccci_resize_reserve_mem(md_resv_mem_addr, md_mem_resv_size, 0);
	}

	return ret;
}

/* --- Download list --------------------------------------- */
/* --- This part is used for normal load ------------------- */
static download_info_t md1_download_list[] = {/* for 90, 91 */
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr | ass func p */
	{main_img,   "md1img",   "md1rom",    0xA000000,   0,         0,                NULL,      ass_func_for_v5_normal_img},
	{dsp_img,    "md1dsp",   "md1dsp",    0x200000,    0,         0,                NULL,      NULL},
	{armv7_img,  "md1arm7",  "md1arm7",   0x200000,    0,         0,                NULL,      NULL},
	{0,          NULL,       NULL,        0,           0,         0,                NULL,      NULL},
};

static download_info_t md1_download_list_v20000[] = {/* for 92 */
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr | ass func p */
	{main_img,   "md1img",   "md1rom",    0xF800000,   0,         0,                NULL,      ass_func_for_v6_normal_img},
	{dsp_img,    "md1dsp",   "md1dsp",    0x200000,    0,         0,                NULL,      NULL},
	{armv7_img,  "md1arm7",  "md1arm7",   0x200000,    0,         0,                NULL,      NULL},
	{0,          NULL,       NULL,        0,           0,         0,                NULL,      NULL},
};
static download_info_t md1_download_list_v40000[] = {/* for r8 modem with v1 */
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr | ass func p */
	{main_img,   "md1img",   "md1rom",    0x4000000,   0,         0,                NULL,      ass_func_for_v1_r8_normal_img},
	{0,          NULL,       NULL,        0,           0,         0,                NULL,      NULL},
};


static download_info_t md3_download_list[] = {
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr */
	{main_img,   "md3img",   "md3rom",    0xC00000,    0,         0,                NULL,      ass_func_for_v1_normal_img},
	{0,          NULL,       NULL,        0,           0,         0,                NULL,      NULL},
};

/* --- This part is used for dummy ap load ------------------- */
static download_info_t md1_download_list_v10001[] = {/* for 90,91 */
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr */
	{main_img,   "md1img",    "md1rom",    0x20000000,  0,         DUMMY_AP_FLAG,    NULL,     ass_func_for_v5_md_only_img},
	{dsp_img,    "md1dsp",    "md1dsp",    0x200000,    0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{armv7_img,  "md1arm7",   "md1arm7",   0x200000,    0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{ramdisk_img,"userdata",  "md1ramdisk",0x2000000,   0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{l1_core_img,"boot" ,     "l1core",    0x1000000,   0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{0,          NULL,        NULL,        0,           0,         0,                NULL,     NULL},
};
static download_info_t md1_download_list_v20001[] = {/* for 92 */
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr */
	{main_img,   "md1img",    "md1rom",    0x20000000,  0,         DUMMY_AP_FLAG,    NULL,     ass_func_for_v6_md_only_img},
	{dsp_img,    "md1dsp",    "md1dsp",    0x200000,    0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{armv7_img,  "md1arm7",   "md1arm7",   0x200000,    0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{ramdisk_img,"userdata",  "md1ramdisk",0x2000000,   0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{l1_core_img,"boot" ,     "l1core",    0x1000000,   0,         DUMMY_AP_FLAG,    NULL,     NULL},
	{0,          NULL,        NULL,        0,           0,         0,                NULL,     NULL},
};

static download_info_t md3_download_list_v10001[] = {
	/* img type | partition | image name | max size  | img size | ext_flag         | mem addr */
	{main_img,   "md3img",   "md3rom",    0x10000000,   0,         DUMMY_AP_FLAG,    NULL,      ass_func_for_v1_md_only_img},
	{0,          NULL,       NULL,        0,            0,         0,                NULL,      NULL},
};


void load_modem_image(void)
{
	modem_info_t info;
	unsigned long long ro_rw_limit;
	unsigned long long smem_limit;
	unsigned long long ld_img_ver;
	unsigned long align;
	long long plat_query_ret;
	int err_code = 0;
	int ret;
	unsigned char *smem_addr;
	int smem_size;
	unsigned int md_load_status_flag = 0;
	int time_lk_md_init = get_timer(0);

	/* --- Get platform configure setting ---*/
	if (ccci_hal_get_ld_md_plat_setting("support_detect") > 0) {
		ALWAYS_LOG("Enter load_modem_image v2.0\n");
	} else {
		ALWAYS_LOG("Using load_modem_image v1.0\n");
		err_code = -LD_ERR_PT_V2_PLAT_NOT_RDY;
		goto _err_exit;
	}
	plat_query_ret = ccci_hal_get_ld_md_plat_setting("share_memory_size");

	if (plat_query_ret <= 0) {
		ALWAYS_LOG("Share memory size abnormal:%d\n", (int)plat_query_ret);
		err_code = -LD_ERR_PT_SMEM_SIZE_ABNORMAL;
		goto _err_exit;
	}
	smem_size = (int)plat_query_ret;

	plat_query_ret = ccci_hal_get_ld_md_plat_setting("share_mem_limit");

	if (plat_query_ret <= 0) {
		ALWAYS_LOG("limit abnormal:%d\n", (int)plat_query_ret);
		err_code = -LD_ERR_PT_LIMIT_SETTING_ABNORMAL;
		goto _err_exit;
	}
	smem_limit = (unsigned long long)plat_query_ret;

	plat_query_ret = ccci_hal_get_ld_md_plat_setting("share_mem_align");

	if (plat_query_ret <= 0) {
		ALWAYS_LOG("align abnormal:%d\n", (int)plat_query_ret);
		err_code = -LD_ERR_PT_ALIGN_SETTING_ABNORMAL;
		goto _err_exit;
	}
	align = (unsigned long)plat_query_ret;

	plat_query_ret = ccci_hal_get_ld_md_plat_setting("ro_rw_mem_limit");

	if (plat_query_ret <= 0) {
		ALWAYS_LOG("ro rw mem limit abnormal:%d\n", (int)plat_query_ret);
		err_code = -LD_ERR_PT_ALLOC_RORW_MEM_FAIL;
		goto _err_exit;
	}
	ro_rw_limit = (unsigned long long)plat_query_ret;

	ALWAYS_LOG("ccci_request_mem: ret:%x, smem_limit:%x, align:%x\n", smem_size, (int)smem_limit, (int)align);
	smem_addr = ccci_request_mem((unsigned int)smem_size, smem_limit, align);
	if (smem_addr == NULL) {
		ALWAYS_LOG("allocate MD share memory fail\n");
		err_code = -LD_ERR_PT_ALLOC_SMEM_FAIL;
		goto _err_exit;
	}
	ccci_hal_apply_hw_remap_for_md_smem(smem_addr, smem_size);

	if (ccci_alloc_local_cmd_line_buf(1024) < 0) {
		ALWAYS_LOG("allocate local cmd line memory fail\n");
		err_code = -LD_ERR_PT_ALLOC_CMD_BUF_FAIL;
		goto _free_memory;
	}

	ccci_lk_tag_info_init((unsigned long long)((unsigned long)smem_addr));

	prepare_tel_fo_setting();

	/* Prepare done, begin to load MD one by one */
	if (ccci_get_opt_val("opt_md1_support") > 0) {
		ALWAYS_LOG("-- MD1 --\n");

		plat_query_ret = ccci_hal_get_ld_md_plat_setting("ro_rw_mem_align");

		if (plat_query_ret <= 0) {
			ALWAYS_LOG("align abnormal for ro rw:%d\n", (int)plat_query_ret);
			err_code = -LD_ERR_PT_ALIGN_SETTING_ABNORMAL;
			update_md_err_to_lk_info(MD_SYS1, err_code);
			goto _load_md2;
		}
		align = (unsigned long)plat_query_ret;

		/* Load image */
		memset(&info, 0, sizeof(modem_info_t));
		info.md_id = MD_SYS1;
		ld_img_ver = ccci_hal_get_ld_md_plat_setting("ld_version");
		switch (ld_img_ver) {
			case 0x10001:
				ret = ld_img_at_list(md1_download_list_v10001, &info, ro_rw_limit, align);
				break;
			case 0x10000:
				ret = ld_img_at_list(md1_download_list, &info, ro_rw_limit, align);
				break;
			case 0x20000:
				ret = ld_img_at_list(md1_download_list_v20000, &info, ro_rw_limit, align);
				break;
			case 0x20001:
				ret = ld_img_at_list(md1_download_list_v20001, &info, ro_rw_limit, align);
				break;
			case 0x40000:
				ret = ld_img_at_list(md1_download_list_v40000, &info, ro_rw_limit, align);
				break;
			default:
				ret = ld_img_at_list(md1_download_list, &info, ro_rw_limit, align);
				break;
		}
		if (ret < 0) {
			err_code = -LD_ERR_PT_MD1_LOAD_FAIL;
			update_md_err_to_lk_info(MD_SYS1, ret);
			ALWAYS_LOG("md1 load fail:%d\n", ret);
			goto _load_md2;
		}

		/* Load success */
		update_md_load_flag_to_lk_info(MD_SYS1);
		add_hdr_info(s_g_md_ld_status, &info);
		md_load_status_flag |= (1<<MD_SYS1);
	}
_load_md2:
	/* Do nothong currently */
	goto _load_md3;
_load_md3:
	if (ccci_get_opt_val("opt_md3_support") > 0) {
		ALWAYS_LOG("-- MD3 --\n");

		plat_query_ret = ccci_hal_get_ld_md_plat_setting("ro_rw_mem_align");

		if (plat_query_ret <= 0) {
			ALWAYS_LOG("align abnormal for ro rw:%d\n", (int)plat_query_ret);
			err_code = -LD_ERR_PT_ALIGN_SETTING_ABNORMAL;
			update_md_err_to_lk_info(MD_SYS3, err_code);
			goto _load_end;
		}
		align = (unsigned long)plat_query_ret;

		/* Load image */
		memset(&info, 0, sizeof(modem_info_t));
		info.md_id = MD_SYS3;
		if (ccci_hal_get_ld_md_plat_setting("ld_version") == 0x10001 ||
				ccci_hal_get_ld_md_plat_setting("ld_version") == 0x20001)
			ret = ld_img_at_list(md3_download_list_v10001, &info, ro_rw_limit, align);
		else
			ret = ld_img_at_list(md3_download_list, &info, ro_rw_limit, align);
		if (ret < 0) {
			err_code = -LD_ERR_PT_MD3_LOAD_FAIL;
			update_md_err_to_lk_info(MD_SYS3, ret);
			ALWAYS_LOG("md3 load fail:%d\n", ret);
			goto _load_end;
		}

		/* Load success */
		update_md_load_flag_to_lk_info(MD_SYS3);
		add_hdr_info(s_g_md_ld_status, &info);
		md_load_status_flag |= (1<<MD_SYS3);
	}

_load_end:

	/* update hdr_count info */
	if (insert_ccci_tag_inf("hdr_count", (char*)&s_g_md_ld_record_num, sizeof(unsigned int)) < 0)
		ALWAYS_LOG("insert hdr_count fail\n");

	/* update hdr tbl info */
	if (insert_ccci_tag_inf("hdr_tbl_inf", (char*)s_g_md_ld_status,
	                        sizeof(modem_info_t)*s_g_md_ld_record_num) < 0)
		ALWAYS_LOG("insert hdr_tbl_inf fail\n");

	if (insert_ccci_tag_inf("retrieve_num", (char*)&retrieve_info_num, sizeof(int)) < 0)
		ALWAYS_LOG("insert retrieve_num fail\n");

	ret = ccci_hal_apply_platform_setting(md_load_status_flag);
	if (ret < 0) {
		/* free all reserved share memory */
		ALWAYS_LOG("ccci_hal_apply_platform_setting ret %d \n", ret);
		err_code = -LD_ERR_PT_APPLY_PLAT_SETTING_FAIL;
		update_common_err_to_lk_info(err_code);
		goto _free_memory;
	} else if (ret == 0) {
		/* free all reserved share memory */
		ALWAYS_LOG("No MD Image enabled %d \n", ret);
		/* err_code = 0; */
		goto _free_memory;
	}

	/* resize share memory to final size */
	if (ret < smem_size) {
		ALWAYS_LOG("re-size share memory form %x to %x\n", smem_size, ret);
		ccci_resize_reserve_mem(smem_addr, smem_size, ret);
	}
	goto _OK_and_exit;

_free_memory:
	if (smem_addr != NULL) {
		ccci_lk_info_re_cfg(0LL, 0);
		ccci_resize_reserve_mem(smem_addr, smem_size, 0);
	}

	ccci_free_local_cmd_line_buf();

_err_exit:
	update_common_err_to_lk_info(err_code);

_OK_and_exit:

	ccci_lk_info_ctl_dump();
	ALWAYS_LOG("[PROFILE] ------- load_modem_image init cost %d ms ----\n", (int)get_timer(time_lk_md_init));
}

static char *errno_str[] = {
	"LD_ERR_NULL_PTR",
	"LD_ERR_GET_COM_CHK_HDR_FAIL",
	"LD_ERR_CHK_HDR_PATTERN",
	"LD_ERR_RESERVE_MEM_NOT_ENOUGH",
	"LD_ERR_ASS_FUNC_ALLOC_MEM_FAIL",
	"LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL",
	"LD_ERR_ASS_FIND_MAIN_INF_FAIL",
	"LD_ERR_ASS_FIND_DSP_INF_FAIL",
	"LD_ERR_ASS_FIND_ARMV7_INF_FAIL",
	"LD_ERR_ASS_FIND_RAMDISK_INF_FAIL",
	"LD_ERR_ASS_FIND_L1CORE_INF_FAIL",
	"LD_ERR_TAG_BUF_FULL",
	"LD_ERR_PAD_SIZE_LESS_THAN_64K",
	"LD_ERR_PAD_INVALID_INF",
	"LD_ERR_PAD_FREE_INF_ABNORMAL",
	"LD_ERR_PAD_OVER_TWO_REGION",
	"LD_ERR_PAD_MISC",
	"LD_ERR_PAD_REGION_NOT_ENOUGH",
	"LD_ERR_PAD_NO_REGION_RETRIEVE",
	"LD_ERR_OPT_SETTING_INVALID",
	"LD_ERR_OPT_NOT_FOUND",
	"LD_ERR_OPT_CMD_BUF_ALLOC_FAIL",
	"LD_ERR_PT_DEV_NULL",
	"LD_ERR_PT_NOT_FOUND",
	"LD_ERR_PT_READ_RAW_FAIL",
	"LD_ERR_PT_IMG_LIST_NULL",
	"LD_ERR_PT_ALLOC_HDR_MEM_FAIL",
	"LD_ERR_PT_READ_HDR_SIZE_ABNORMAL",
	"LD_ERR_PT_P_HDR_MAGIC_MIS_MATCH",
	"LD_ERR_PT_MAIN_IMG_SIZE_ABNORMAL",
	"LD_ERR_PT_CERT_CHAIN_FAIL",
	"LD_ERR_PT_IMG_TOO_LARGE",
	"LD_ERR_PT_IMG_SIZE_NOT_SYNC_CHK_HDR",
	"LD_ERR_PT_ALLOC_MD_MEM_FAIL",
	"LD_ERR_PT_LD_IMG_DATA_FAIL",
	"LD_ERR_PT_HASH_CHK_FAIL",
	"LD_ERR_PT_V2_PLAT_NOT_RDY",
	"LD_ERR_PT_SMEM_SIZE_ABNORMAL",
	"LD_ERR_PT_LIMIT_SETTING_ABNORMAL",
	"LD_ERR_PT_ALIGN_SETTING_ABNORMAL",
	"LD_ERR_PT_ALLOC_RORW_MEM_FAIL",
	"LD_ERR_PT_ALLOC_SMEM_FAIL",
	"LD_ERR_PT_ALLOC_CMD_BUF_FAIL",
	"LD_ERR_PT_MD1_LOAD_FAIL",
	"LD_ERR_PT_MD3_LOAD_FAIL",
	"LD_ERR_PT_APPLY_PLAT_SETTING_FAIL",
	"LD_ERR_PT_CHK_IMG_NAME_FAIL",
	"LD_ERR_PLAT_INVALID_MD_ID",
	"LD_ERR_PLAT_MPU_REGION_EMPTY",
	"LD_ERR_PLAT_MPU_REGION_TOO_MORE",
	"LD_ERR_PLAT_MPU_REGION_NUM_NOT_SYNC",
	"LD_ERR_PLAT_ABNORMAL_FREE_REGION",
	"LD_ERR_PLAT_ABNORMAL_PAD_ARRAY",
	"LD_ERR_PLAT_NO_MORE_FREE_REGION",
	"LD_ERR_PLAT_MD1_NOT_RDY"
};
char *ld_md_errno_to_str(int errno)
{
	if (errno < 1)
		return "invalid errno";

	if ((errno-1) < (int)(sizeof(errno_str)/sizeof(char*)))
		return errno_str[errno-1];

	return "errno not found";
}
