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
#include <debug.h>
#include "ccci_ld_md_core.h"
#include "ccci_ld_md_errno.h"

#define MODULE_NAME "LK_LD_MD"

/*==============================================================================*/
/* region          |idx | bits | AP    | MD1    | MDHW                     */
/* md1_rom(dsp_rom)| 0  | 3:0  | f     | f      | f                        */
/* md1_mcurw_hwro  | 1  | 7:4  | f     | f      | f                        */
/* md1_mcuro_hwrw  | 2  |11:8  | f     | f      | f                        */
/* md1_mcurw_hwrw  | 3  |15:12 | f     | f      | f                        */
/* md1_rw          | 4  |19:16 | f     | f      | f                        */
static void parse_mem_layout_attr_v5_hdr(struct md_check_header_v5 *hdr, struct image_section_desc tbl[])
{
	unsigned int i, j;
	unsigned int region_attr_merged, shift_num, tmp;

	for (i = 0; i < 8; i++) {
		region_attr_merged = 0;
		for (j = 0; j < 4; j++) {
			shift_num = i<<2;/* equal *4 */
			tmp = (hdr->domain_attr[j]&(0x0000000F<<shift_num))>>shift_num;
			region_attr_merged |= (tmp<<(j*4));
		}

		tbl[i].mpu_attr = region_attr_merged;
		tbl[i].offset = hdr->region_info[i].region_offset;
		tbl[i].size = hdr->region_info[i].region_size;
		tbl[i].relate_idx = 0;
		tbl[i].ext_flag = 0;
	}

	/* Mark for end */
	tbl[i].offset = 0;
	tbl[i].size = 0;
}

static void parse_mem_layout_attr_v6_hdr(struct md_check_header_v6 *hdr, struct image_section_desc tbl[])
{
	unsigned int i, j;
	unsigned int region_attr_merged, shift_num, tmp;

	for (i = 0; i < 8; i++) {
		region_attr_merged = 0;
		for (j = 0; j < 4; j++) {
			shift_num = i<<2;/* equal *4 */
			tmp = (hdr->domain_attr[j]&(0x0000000F<<shift_num))>>shift_num;
			region_attr_merged |= (tmp<<(j*4));
		}

		tbl[i].mpu_attr = region_attr_merged;
		tbl[i].offset = hdr->region_info[i].region_offset;
		tbl[i].size = hdr->region_info[i].region_size;
		tbl[i].relate_idx = 0;
		tbl[i].ext_flag = 0;
	}

	/* Mark for end */
	tbl[i].offset = 0;
	tbl[i].size = 0;
}

static void *ccci_get_md_header(void *tail_addr, int *header_ver)
{
	int size;
	void *header_addr = NULL;
	LD_DBG_LOG("ccci_get_md_header tail_addr[%p]\n", tail_addr);
	if (!tail_addr)
		return NULL;

	size = *((unsigned int *)(tail_addr - 4));
	LD_DBG_LOG("ccci_get_md_header check headr v1(%d), v3(%d), v5(%d), v6(%d) curr(%d)\n",
	           sizeof(struct md_check_header_v1), sizeof(struct md_check_header_v3),
	           sizeof(struct md_check_header_v5), sizeof(struct md_check_header_v6), size);

	if (size == sizeof(struct md_check_header_v1)) {
		header_addr = tail_addr - sizeof(struct md_check_header_v1);
		if (header_ver)
			*header_ver = 1;
	} else if (size == sizeof(struct md_check_header_v3)) {
		header_addr = tail_addr - sizeof(struct md_check_header_v3);
		if (header_ver)
			*header_ver = 3;
	} else if (size == sizeof(struct md_check_header_v5)) {
		header_addr = tail_addr - sizeof(struct md_check_header_v5);
		if (header_ver)
			*header_ver = 5;
	} else if (size == sizeof(struct md_check_header_v6)) {
		header_addr = tail_addr - sizeof(struct md_check_header_v6);
		if (header_ver)
			*header_ver = 6;
	}

	return header_addr;
}

static int ccci_get_max_chk_hdr_size(void)
{
	int max = (int)sizeof(struct md_check_header_v5);
	int size = (int)sizeof(struct md_check_header_v4);

	if (max < size)
		max = size;
	size = (int)sizeof(struct md_check_header_v3);
	if (max < size)
		max = size;
	size = (int)sizeof(struct md_check_header_v1);
	if (max < size)
		max = size;
	size = (int)sizeof(struct md_check_header_v6);
	if (max < size)
		max = size;

	return max;
}

static int get_chk_hdr_size_by_ver(int ver)
{
	switch (ver) {
		case 5:
			return (int)sizeof(struct md_check_header_v5);
		case 6:
			return (int)sizeof(struct md_check_header_v6);
		case 1:
			return (int)sizeof(struct md_check_header_v1);
		case 4:
			return (int)sizeof(struct md_check_header_v4);
		case 3:
			return (int)sizeof(struct md_check_header_v3);
		default:
			return 0;
	}

	return 0;
}

static int ccci_md_header_dump(void *header_addr, int ver)
{
#ifdef LD_IMG_DUMP_LOG_EN
	//int size;
	struct md_check_header_v1 *v1_hdr;
	struct md_check_header_v3 *v3_hdr;
	struct md_check_header_v5 *v5_hdr;
	struct md_check_header_v6 *v6_hdr;
	int i;
	if (!header_addr)
		return -LD_ERR_NULL_PTR;

	switch (ver) {
		case 1:
			/* parsing md check header */
			v1_hdr = (struct md_check_header_v1 *)header_addr;
			LD_DBG_LOG("===== Dump v1 md header =====\n");
			LD_DBG_LOG("[%s] MD IMG  - Magic          : %s\n"    , MODULE_NAME, v1_hdr->common.check_header);
			LD_DBG_LOG("[%s] MD IMG  - header_verno   : 0x%08X\n", MODULE_NAME, v1_hdr->common.header_verno);
			LD_DBG_LOG("[%s] MD IMG  - product_ver    : 0x%08X\n", MODULE_NAME, v1_hdr->common.product_ver);
			LD_DBG_LOG("[%s] MD IMG  - image_type     : 0x%08X\n", MODULE_NAME, v1_hdr->common.image_type);
			LD_DBG_LOG("[%s] MD IMG  - mem_size       : 0x%08X\n", MODULE_NAME, v1_hdr->common.mem_size);
			LD_DBG_LOG("[%s] MD IMG  - md_img_size    : 0x%08X\n", MODULE_NAME, v1_hdr->common.md_img_size);
			LD_DBG_LOG("[%s] MD IMG  - ap_md_smem_size: 0x%08X\n", MODULE_NAME, v1_hdr->ap_md_smem_size);
			LD_DBG_LOG("=============================\n");
			break;
		case 3: /* V3 */
		case 4:
			/* parsing md check header */
			v3_hdr = (struct md_check_header_v3 *)header_addr;
			LD_DBG_LOG("===== Dump v3 md header =====\n");
			LD_DBG_LOG("[%s] MD IMG  - Magic          : %s\n"    , MODULE_NAME, v3_hdr->common.check_header);
			LD_DBG_LOG("[%s] MD IMG  - header_verno   : 0x%08X\n", MODULE_NAME, v3_hdr->common.header_verno);
			LD_DBG_LOG("[%s] MD IMG  - product_ver    : 0x%08X\n", MODULE_NAME, v3_hdr->common.product_ver);
			LD_DBG_LOG("[%s] MD IMG  - image_type     : 0x%08X\n", MODULE_NAME, v3_hdr->common.image_type);
			LD_DBG_LOG("[%s] MD IMG  - mem_size       : 0x%08X\n", MODULE_NAME, v3_hdr->common.mem_size);
			LD_DBG_LOG("[%s] MD IMG  - md_img_size    : 0x%08X\n", MODULE_NAME, v3_hdr->common.md_img_size);
			LD_DBG_LOG("[%s] MD IMG  - dsp offset     : 0x%08X\n", MODULE_NAME, v3_hdr->dsp_img_offset);
			LD_DBG_LOG("[%s] MD IMG  - dsp size       : 0x%08X\n", MODULE_NAME, v3_hdr->dsp_img_size);
			LD_DBG_LOG("[%s] MD IMG  - rpc_sec_mem_addr : 0x%08X\n", MODULE_NAME, v3_hdr->rpc_sec_mem_addr);
			LD_DBG_LOG("=============================\n");
			break;
		case 5:
			/* parsing md check header */
			v5_hdr = (struct md_check_header_v5 *)header_addr;
			LD_DBG_LOG("===== Dump v5 md header =====\n");
			LD_DBG_LOG("[%s] MD IMG  - Magic          : %s\n"    , MODULE_NAME, v5_hdr->common.check_header);
			LD_DBG_LOG("[%s] MD IMG  - header_verno   : 0x%08X\n", MODULE_NAME, v5_hdr->common.header_verno);
			LD_DBG_LOG("[%s] MD IMG  - product_ver    : 0x%08X\n", MODULE_NAME, v5_hdr->common.product_ver);
			LD_DBG_LOG("[%s] MD IMG  - image_type     : 0x%08X\n", MODULE_NAME, v5_hdr->common.image_type);
			LD_DBG_LOG("[%s] MD IMG  - mem_size       : 0x%08X\n", MODULE_NAME, v5_hdr->common.mem_size);
			LD_DBG_LOG("[%s] MD IMG  - md_img_size    : 0x%08X\n", MODULE_NAME, v5_hdr->common.md_img_size);
			LD_DBG_LOG("[%s] MD IMG  - dsp offset     : 0x%08X\n", MODULE_NAME, v5_hdr->dsp_img_offset);
			LD_DBG_LOG("[%s] MD IMG  - dsp size       : 0x%08X\n", MODULE_NAME, v5_hdr->dsp_img_size);
			LD_DBG_LOG("[%s] MD IMG  - rpc_sec_mem_addr : 0x%08X\n", MODULE_NAME, v5_hdr->rpc_sec_mem_addr);
			LD_DBG_LOG("[%s] MD IMG  - armv7 offset   : 0x%08X\n", MODULE_NAME, v5_hdr->arm7_img_offset);
			LD_DBG_LOG("[%s] MD IMG  - armv7 size     : 0x%08X\n", MODULE_NAME, v5_hdr->arm7_img_size);
			LD_DBG_LOG("[%s] MD IMG  - region num     : 0x%08X\n", MODULE_NAME, v5_hdr->region_num);
			LD_DBG_LOG("[%s] MD IMG  - ap_md_smem_size: 0x%08X\n", MODULE_NAME, v5_hdr->ap_md_smem_size);
			LD_DBG_LOG("[%s] MD IMG  - md_md_smem_size: 0x%08X\n", MODULE_NAME, v5_hdr->md_to_md_smem_size);
			for (i = 0; i < 8; i++) {
				LD_DBG_LOG("[%s] MD IMG  - region[%d] off : 0x%08X\n", MODULE_NAME, i,
				           v5_hdr->region_info[i].region_offset);
				LD_DBG_LOG("[%s] MD IMG  - region[%d] size: 0x%08X\n", MODULE_NAME, i,
				           v5_hdr->region_info[i].region_size);
			}
			for (i = 0; i < 4; i++) {
				LD_DBG_LOG("[%s] MD IMG  - domain_attr[%d] : 0x%08X\n", MODULE_NAME, i,
				           v5_hdr->domain_attr[i]);
			}
			for (i = 0; i < MAX_PADDING_NUM_V5_HDR; i++) {
				LD_DBG_LOG("[%s] MD IMG  - padding info[%d] offset: 0x%08X\n", MODULE_NAME, i,
				           v5_hdr->padding_blk[i].start_offset);
				LD_DBG_LOG("[%s] MD IMG  - padding info[%d] size: 0x%08X\n", MODULE_NAME, i,
				           v5_hdr->padding_blk[i].length);
			}
			LD_DBG_LOG("=============================\n");
			break;
		case 6:
			/* parsing md check header */
			v6_hdr = (struct md_check_header_v6 *)header_addr;
			LD_DBG_LOG("===== Dump v6 md header =====\n");
			LD_DBG_LOG("[%s] MD IMG  - Magic          : %s\n"    , MODULE_NAME, v6_hdr->common.check_header);
			LD_DBG_LOG("[%s] MD IMG  - header_verno   : 0x%08X\n", MODULE_NAME, v6_hdr->common.header_verno);
			LD_DBG_LOG("[%s] MD IMG  - product_ver    : 0x%08X\n", MODULE_NAME, v6_hdr->common.product_ver);
			LD_DBG_LOG("[%s] MD IMG  - image_type     : 0x%08X\n", MODULE_NAME, v6_hdr->common.image_type);
			LD_DBG_LOG("[%s] MD IMG  - mem_size       : 0x%08X\n", MODULE_NAME, v6_hdr->common.mem_size);
			LD_DBG_LOG("[%s] MD IMG  - md_img_size    : 0x%08X\n", MODULE_NAME, v6_hdr->common.md_img_size);
			LD_DBG_LOG("[%s] MD IMG  - dsp offset     : 0x%08X\n", MODULE_NAME, v6_hdr->dsp_img_offset);
			LD_DBG_LOG("[%s] MD IMG  - dsp size       : 0x%08X\n", MODULE_NAME, v6_hdr->dsp_img_size);
			LD_DBG_LOG("[%s] MD IMG  - rpc_sec_mem_addr : 0x%08X\n", MODULE_NAME, v6_hdr->rpc_sec_mem_addr);
			LD_DBG_LOG("[%s] MD IMG  - armv7 offset   : 0x%08X\n", MODULE_NAME, v6_hdr->arm7_img_offset);
			LD_DBG_LOG("[%s] MD IMG  - armv7 size     : 0x%08X\n", MODULE_NAME, v6_hdr->arm7_img_size);
			LD_DBG_LOG("[%s] MD IMG  - region num     : 0x%08X\n", MODULE_NAME, v6_hdr->region_num);
			LD_DBG_LOG("[%s] MD IMG  - ap_md_smem_size: 0x%08X\n", MODULE_NAME, v6_hdr->ap_md_smem_size);
			LD_DBG_LOG("[%s] MD IMG  - md_md_smem_size: 0x%08X\n", MODULE_NAME, v6_hdr->md_to_md_smem_size);
			for (i = 0; i < 8; i++) {
				LD_DBG_LOG("[%s] MD IMG  - region[%d] off : 0x%08X\n", MODULE_NAME, i,
				           v6_hdr->region_info[i].region_offset);
				LD_DBG_LOG("[%s] MD IMG  - region[%d] size: 0x%08X\n", MODULE_NAME, i,
				           v6_hdr->region_info[i].region_size);
			}
			for (i = 0; i < 4; i++) {
				LD_DBG_LOG("[%s] MD IMG  - domain_attr[%d] : 0x%08X\n", MODULE_NAME, i,
				           v6_hdr->domain_attr[i]);
			}
			for (i = 0; i < MAX_PADDING_NUM_V6_HDR; i++) {
				LD_DBG_LOG("[%s] MD IMG  - padding info[%d] offset: 0x%08X\n", MODULE_NAME, i,
				           v6_hdr->padding_blk[i].start_offset);
				LD_DBG_LOG("[%s] MD IMG  - padding info[%d] size: 0x%08X\n", MODULE_NAME, i,
				           v6_hdr->padding_blk[i].length);
			}
			LD_DBG_LOG("=============================\n");
			break;
		default:
			break;
	}
#endif
	return 0;
}

int verify_main_img_check_header(modem_info_t *info)
{
	struct md_check_header_comm *common_hdr;
	int md_hdr_ver;
	char md_str[16];
	int ret;

	/* Parse check header */
	common_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, &md_hdr_ver);

	if (common_hdr == NULL) {
		ALWAYS_LOG("parse check header fail\n");
		ret = -LD_ERR_GET_COM_CHK_HDR_FAIL;
		goto _Hdr_Verity_Exit;
	}

	if (strncmp((char const *)common_hdr->check_header, "CHECK_HEADER", 12)) {
		ALWAYS_LOG("invald md check header str[%s]\n", common_hdr->check_header);
		ret = -LD_ERR_CHK_HDR_PATTERN;
		goto _Hdr_Verity_Exit;
	}

	/* Dump header info */
	ccci_md_header_dump(common_hdr, md_hdr_ver);

	/* Post to platform */
	ccci_hal_post_hdr_info((void*)common_hdr, md_hdr_ver, (int)info->md_id);

	if (info->resv_mem_size < common_hdr->mem_size) {
		ALWAYS_LOG("Reserved memory not enough, resv:%d, require:%d\n",
		           (int)info->resv_mem_size, (int)common_hdr->mem_size);
		ret = -LD_ERR_RESERVE_MEM_NOT_ENOUGH;
		goto _Hdr_Verity_Exit;
	}

	info->resv_mem_size = common_hdr->mem_size;
	info->ro_rw_size = common_hdr->md_img_size;
	info->md_type = common_hdr->image_type;

	/* Inssert image size and check header info to arguments array for kernel */
	snprintf(md_str, 16, "md%dimg", info->md_id + 1);
	if (insert_ccci_tag_inf(md_str, (char*)&info->ro_rw_size, sizeof(unsigned int)) < 0)
		ALWAYS_LOG("insert %s fail\n", md_str);

	/* Inssert image size and check header info to arguments array for kernel */
	snprintf(md_str, 16, "md%d_chk", info->md_id + 1);
	if (insert_ccci_tag_inf(md_str, (char*)common_hdr, get_chk_hdr_size_by_ver(md_hdr_ver)) < 0)
		ALWAYS_LOG("insert %s fail\n", md_str);

	return 0;

_Hdr_Verity_Exit:
	return ret;
}

/***************************************************************************************************
****************************************************************************************************
** Sub section:
**   Download image list part: normal load and dummy AP
**   MD1 default using v5 header; MD3 default using v1 header
****************************************************************************************************
***************************************************************************************************/
static download_info_t *find_ptr_at_img_list(download_info_t img_list[], int img_type)
{
	download_info_t *curr = img_list;
	while (curr->img_type != 0) {
		if (curr->img_type == img_type)
			return curr;
		curr++;
	}
	return NULL;
}
/* --- Assistant function for MD check header v5 --- */
int ass_func_for_v5_normal_img(void *load_info, void *data)
{
	modem_info_t *info = (modem_info_t *)load_info;
	download_info_t *ld_list = (download_info_t *)data;
	struct md_check_header_v5 *chk_hdr;
	download_info_t *curr;
	unsigned char *md_resv_mem_addr = NULL;
	struct image_section_desc *tbl;
	int mpu_num_for_padding;
	int ret;

	/* region: 8; padding:3; mark for end:1 total: 12(=8+3+1) */
	tbl = (struct image_section_desc *)malloc(sizeof(struct image_section_desc)*12);
	if (tbl == NULL)
		return -LD_ERR_ASS_FUNC_ALLOC_MEM_FAIL;

	/* find start addr for check header */
	chk_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, NULL);
	if (chk_hdr == NULL) {
		ret = -LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL;
		goto _free_alloc_mem;
	}

	/* Get md main image addr at memory */
	curr = find_ptr_at_img_list(ld_list, main_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_MAIN_INF_FAIL;
		goto _free_alloc_mem;
	} else
		md_resv_mem_addr = curr->mem_addr;

	/* Get dsp image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, dsp_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_DSP_INF_FAIL;
		goto _free_alloc_mem;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->dsp_img_offset;
		curr->img_size = chk_hdr->dsp_img_size;
	}

	/* Get armv7 image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, armv7_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_ARMV7_INF_FAIL;
		goto _free_alloc_mem;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->arm7_img_offset;
		curr->img_size = chk_hdr->arm7_img_size;
	}

	parse_mem_layout_attr_v5_hdr(chk_hdr, tbl);
	mpu_num_for_padding = ccci_hal_get_mpu_num_for_padding_mem();
	if (mpu_num_for_padding >= 1)
		retrieve_free_padding_mem_v5_hdr((modem_info_t *)load_info, tbl, chk_hdr, mpu_num_for_padding);

	ret = ccci_hal_send_mpu_info_to_platorm(load_info, tbl);
	if (ret < 0)
		goto _free_alloc_mem;

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

_free_alloc_mem:
	if (tbl)
		free(tbl);

	return ret;
}

int ass_func_for_v6_normal_img(void *load_info, void *data)
{
	modem_info_t *info = (modem_info_t *)load_info;
	download_info_t *ld_list = (download_info_t *)data;
	struct md_check_header_v6 *chk_hdr;
	download_info_t *curr;
	unsigned char *md_resv_mem_addr = NULL;
	struct image_section_desc *tbl;
	int mpu_num_for_padding;
	int ret;

	/* region: 8; padding:8; mark for end:1 total: 17(=8+8+1) */
	tbl = (struct image_section_desc *)malloc(sizeof(struct image_section_desc)*17);
	if (tbl == NULL)
		return -LD_ERR_ASS_FUNC_ALLOC_MEM_FAIL;

	/* find start addr for check header */
	chk_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, NULL);
	if (chk_hdr == NULL) {
		ret = -LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL;
		goto _free_alloc_mem;
	}

	/* Get md main image addr at memory */
	curr = find_ptr_at_img_list(ld_list, main_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_MAIN_INF_FAIL;
		goto _free_alloc_mem;
	} else
		md_resv_mem_addr = curr->mem_addr;

	/* Get dsp image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, dsp_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_DSP_INF_FAIL;
		goto _free_alloc_mem;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->dsp_img_offset;
		curr->img_size = chk_hdr->dsp_img_size;
	}

	/* Get armv7 image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, armv7_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_ARMV7_INF_FAIL;
		goto _free_alloc_mem;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->arm7_img_offset;
		curr->img_size = chk_hdr->arm7_img_size;
	}

	parse_mem_layout_attr_v6_hdr(chk_hdr, tbl);
	mpu_num_for_padding = ccci_hal_get_mpu_num_for_padding_mem();
	if (mpu_num_for_padding >= 1)
		retrieve_free_padding_mem_v6_hdr((modem_info_t *)load_info, tbl, chk_hdr, mpu_num_for_padding);

	ret = ccci_hal_send_mpu_info_to_platorm(load_info, tbl);
	if (ret < 0)
		goto _free_alloc_mem;

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

_free_alloc_mem:
	if (tbl)
		free(tbl);

	return ret;
}

int ass_func_for_v1_normal_img(void *load_info, void *data)
{
	modem_info_t *info = (modem_info_t *)load_info;
	struct md_check_header_v1 *chk_hdr;
	struct image_section_desc *tbl;
	int ret;

	/* region: 2; mark for end:1 total: 3(=2+1) */
	tbl = (struct image_section_desc *)malloc(sizeof(struct image_section_desc)*3);
	if (tbl == NULL)
		return -LD_ERR_ASS_FUNC_ALLOC_MEM_FAIL;

	/* find start addr for check header */
	chk_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, NULL);
	if (chk_hdr == NULL) {
		ret = -LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL;
		goto _free_alloc_mem;
	}

	/* Configure mpu info */
	/* RO part */
	tbl[0].mpu_attr = 0xFFFFFFFF;
	tbl[0].offset = 0;
	tbl[0].size = chk_hdr->common.md_img_size;
	tbl[0].relate_idx = 0;
	tbl[0].ext_flag = 0;
	/* RW part */
	tbl[1].mpu_attr = 0xFFFFFFFF;
	tbl[1].offset = tbl[0].offset + tbl[0].size;
	tbl[1].size = chk_hdr->common.mem_size - chk_hdr->common.md_img_size;
	tbl[1].relate_idx = 0;
	tbl[1].ext_flag = 0;
	/* Mark for end */
	tbl[2].mpu_attr = 0xFFFFFFFF;
	tbl[2].offset = 0;
	tbl[2].size = 0;
	tbl[2].relate_idx = 0;
	tbl[2].ext_flag = 0;

	ret = ccci_hal_send_mpu_info_to_platorm(load_info, tbl);
	if (ret < 0)
		goto _free_alloc_mem;

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

_free_alloc_mem:
	if (tbl)
		free(tbl);

	return ret;
}

int ass_func_for_v1_r8_normal_img(void *load_info, void *data)
{
	modem_info_t *info = (modem_info_t *)load_info;
	struct md_check_header_v1 *chk_hdr;
	struct image_section_desc *tbl;
	int ret;
	long long plat_ret;
	unsigned int smem_size;

	/* region: 2; mark for end:1 total: 3(=2+1) */
	tbl = (struct image_section_desc *)malloc(sizeof(struct image_section_desc)*3);
	if (tbl == NULL)
		return -LD_ERR_ASS_FUNC_ALLOC_MEM_FAIL;

	/* find start addr for check header */
	chk_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, NULL);
	if (chk_hdr == NULL) {
		ret = -LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL;
		goto _free_alloc_mem;
	}

	plat_ret = ccci_hal_get_ld_md_plat_setting("smem_at_tail_size");

	if (plat_ret <= 0) {
		ALWAYS_LOG("Share memory size abnormal:%d\n", (int)plat_ret);
		ret = -LD_ERR_PT_SMEM_SIZE_ABNORMAL;
		goto _free_alloc_mem;
	}
	smem_size = (unsigned int)plat_ret;
	info->resv_mem_size += smem_size; /* R8 modem share memory at end of MD RW */

	/* Configure mpu info */
	/* RO part */
	tbl[0].mpu_attr = 0xFFFFFFFF;
	tbl[0].offset = 0;
	tbl[0].size = chk_hdr->common.md_img_size;
	tbl[0].size = (tbl[0].size + 0xFFFF)&(~0xFFFF);
	tbl[0].relate_idx = 0;
	tbl[0].ext_flag = 0;
	/* RW part */
	tbl[1].mpu_attr = 0xFFFFFFFF;
	tbl[1].offset = tbl[0].offset + tbl[0].size;
	tbl[1].size = chk_hdr->common.mem_size - tbl[0].size;
	tbl[1].relate_idx = 0;
	tbl[1].ext_flag = 0;
	/* Mark for end */
	tbl[2].mpu_attr = 0xFFFFFFFF;
	tbl[2].offset = 0;
	tbl[2].size = 0;
	tbl[2].relate_idx = 0;
	tbl[2].ext_flag = 0;

	ret = ccci_hal_send_mpu_info_to_platorm(load_info, tbl);
	if (ret < 0)
		goto _free_alloc_mem;

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

_free_alloc_mem:
	if (tbl)
		free(tbl);

	return ret;
}

int ass_func_for_v5_md_only_img(void *load_info, void *data)
{
	modem_info_t *info = (modem_info_t *)load_info;
	download_info_t *ld_list = (download_info_t *)data;
	struct md_check_header_v5 *chk_hdr;
	download_info_t *curr;
	unsigned char *md_resv_mem_addr = NULL;
	int ret;

	/* find start addr for check header */
	chk_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, NULL);
	if (chk_hdr == NULL) {
		ret = -LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL;
		goto _exit;
	}

	/* Get md main image(MD only/HVT) addr at memory */
	curr = find_ptr_at_img_list(ld_list, main_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_MAIN_INF_FAIL;
		goto _exit;
	} else
		md_resv_mem_addr = curr->mem_addr;

	/* Get dsp image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, dsp_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_DSP_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->dsp_img_offset;
		curr->img_size = chk_hdr->dsp_img_size;
	}

	/* Get armv7 image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, armv7_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_ARMV7_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->arm7_img_offset; /* using arm7 section as ramdisk */
		curr->img_size = chk_hdr->arm7_img_size;
	}

	/* Get ramdisk image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, ramdisk_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_RAMDISK_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->ramdisk_offset;
		curr->img_size = chk_hdr->ramdisk_size;
	}

	/* Get l1_core image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, l1_core_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_L1CORE_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->region_info[7].region_offset; /* using padding[1] section as l1core */
		curr->img_size = chk_hdr->region_info[7].region_size;
	}

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

_exit:
	return ret;
}

int ass_func_for_v6_md_only_img(void *load_info, void *data)
{
	modem_info_t *info = (modem_info_t *)load_info;
	download_info_t *ld_list = (download_info_t *)data;
	struct md_check_header_v6 *chk_hdr;
	download_info_t *curr;
	unsigned char *md_resv_mem_addr = NULL;
	int ret;

	/* find start addr for check header */
	chk_hdr = ccci_get_md_header((unsigned char*)((unsigned long)info->base_addr) + info->load_size, NULL);
	if (chk_hdr == NULL) {
		ret = -LD_ERR_ASS_FUNC_GET_CHK_HDR_FAIL;
		goto _exit;
	}

	/* Get md main image(MD only/HVT) addr at memory */
	curr = find_ptr_at_img_list(ld_list, main_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_MAIN_INF_FAIL;
		goto _exit;
	} else
		md_resv_mem_addr = curr->mem_addr;

	/* Get dsp image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, dsp_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_DSP_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->dsp_img_offset;
		curr->img_size = chk_hdr->dsp_img_size;
	}

	/* Get armv7 image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, armv7_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_ARMV7_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->arm7_img_offset; /* using arm7 section as ramdisk */
		curr->img_size = chk_hdr->arm7_img_size;
	}

	/* Get ramdisk image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, ramdisk_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_RAMDISK_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->ramdisk_offset;
		curr->img_size = chk_hdr->ramdisk_size;
	}

	/* Get l1_core image addr at memory and size */
	curr = find_ptr_at_img_list(ld_list, l1_core_img);
	if (curr == NULL) {
		ret = -LD_ERR_ASS_FIND_L1CORE_INF_FAIL;
		goto _exit;
	} else {
		curr->mem_addr = md_resv_mem_addr + chk_hdr->region_info[7].region_offset; /* using padding[1] section as l1core */
		curr->img_size = chk_hdr->region_info[7].region_size;
	}

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

_exit:
	return ret;
}

int ass_func_for_v1_md_only_img(void *load_info, void *data)
{
	int ret;

	ret = ccci_hal_apply_hw_remap_for_md_ro_rw(load_info);

	return ret;
}

