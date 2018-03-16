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


/***************************************************************************************************
** Sub section:
**   padding memory for v5 check hdr
***************************************************************************************************/
/* This function only support 3 cases
**   case 0: |======|======| ==> |++====|======|
**   case 1: |======|======| ==> |==++==|======|
**   case 2: |======|======| ==> |====++|======|
**   case 4: |======|======| ==> |=====+|+=====| NOT suppose this case !!!!
**
** return value:
**   1 : retrieve success and used one mpu region
**   0 : retrieve success but not used one mpu region
**  -1 : error
*/
static int free_padding_mem_blk_match(modem_info_t *info, struct image_section_desc mem_tbl[],
                                      int mem_tbl_item_num, free_padding_block_t *free_mem)
{
	int i;
	int mem_blk_num = mem_tbl_item_num;
	unsigned int tmp_start_addr, tmp_end_addr;
	unsigned int old_start, old_end;
	unsigned char *mem_addr = (unsigned char *)((unsigned long)info->base_addr);

	ALWAYS_LOG("== Get retrieve %p~%p =======\n",
	           (unsigned char *)free_mem->start_offset,
	           (unsigned char *)free_mem->start_offset
	           + free_mem->length - 1);

	if (free_mem->length < MPU_64K_VALUE) {
		ALWAYS_LOG("free memory too small\n");
		return -LD_ERR_PAD_SIZE_LESS_THAN_64K;
	}

	/* Find free memory position */
	for (i = 0; i < mem_blk_num; i++) {
		if ((free_mem->start_offset >= mem_tbl[i].offset) &&
		        (free_mem->start_offset <= (mem_tbl[i].offset + mem_tbl[i].size -1)))
			break;
	}

	if (i == mem_blk_num) {
		ALWAYS_LOG("invalid free memory slot info, not at any block\n");
		return -LD_ERR_PAD_INVALID_INF;
	}

	old_start = mem_tbl[i].offset;
	old_end = mem_tbl[i].offset + mem_tbl[i].size -1;
	DBG_LOG("[%p~%p] ==>\n", mem_addr + old_start, mem_addr + old_end);
	if (free_mem->start_offset == mem_tbl[i].offset) {
		/* case 0, no need use one mpu region */
		if (free_mem->length > mem_tbl[i].size) {
			ALWAYS_LOG("free memory size too large\n");
			return -LD_ERR_PAD_FREE_INF_ABNORMAL;
		}
		/* Adjust mpu start address */
		tmp_start_addr  = mem_tbl[i].offset;
		mem_tbl[i].offset += free_mem->length;
		mem_tbl[i].offset &= MPU_64K_ALIGN_MASK;
		mem_tbl[i].size = mem_tbl[i].size - (mem_tbl[i].offset - tmp_start_addr);
		mem_tbl[i].ext_flag = VALID_PADDING;
		ALWAYS_LOG("[%p-Retrieve-%p|%p-Reserved-%p]\n",
		           mem_addr + free_mem->start_offset, mem_addr + mem_tbl[i].offset + mem_tbl[i].size -1,
		           mem_addr + mem_tbl[i].offset, mem_addr + mem_tbl[i].offset + mem_tbl[i].size - 1);
		ccci_retrieve_mem((unsigned char*)(free_mem->start_offset + mem_addr), mem_tbl[i].offset - tmp_start_addr);
		return 0;
	}

	if ((free_mem->start_offset + free_mem->length) < (mem_tbl[i].offset + mem_tbl[i].size)) {
		/* case 1, need use one mpu region, seperate it to three part */
		tmp_end_addr = mem_tbl[i].offset + mem_tbl[i].size -1;
		mem_tbl[i].size = ((free_mem->start_offset + MPU_64K_VALUE - 1) & MPU_64K_ALIGN_MASK) - mem_tbl[i].offset;
		tmp_start_addr = (free_mem->start_offset + free_mem->length) & MPU_64K_ALIGN_MASK;
		if (((mem_tbl[i].size - mem_tbl[i].offset + 1) > MPU_64K_VALUE) &&
		        ((tmp_end_addr - tmp_start_addr +1) > MPU_64K_VALUE)) {
			ALWAYS_LOG("[%p-Reserved-%p|%p-Retrieve-0%p|%p-Reserved-%p]\n",
			           mem_addr + mem_tbl[i].offset, mem_addr + mem_tbl[i].offset + mem_tbl[i].size -1,
			           mem_addr + mem_tbl[i].offset + mem_tbl[i].size, mem_addr + tmp_start_addr - 1,
			           mem_addr + tmp_start_addr, mem_addr + tmp_end_addr);
			ccci_retrieve_mem((unsigned char*)(mem_tbl[i].offset + mem_tbl[i].size + mem_addr),
			                  tmp_start_addr - (mem_tbl[i].offset + mem_tbl[i].size));
			mem_tbl[mem_blk_num].offset = tmp_start_addr;
			mem_tbl[mem_blk_num].size = tmp_end_addr - tmp_start_addr + 1;
			mem_tbl[mem_blk_num].ext_flag = NEED_MPU_MORE|VALID_PADDING;
			if (mem_tbl[i].relate_idx != 0)
				mem_tbl[mem_blk_num].relate_idx = mem_tbl[i].relate_idx; /* Multiple - cut */
			else {
				mem_tbl[mem_blk_num].relate_idx = LOGIC_BINDING_IDX_START + i;
				mem_tbl[i].relate_idx = LOGIC_BINDING_IDX_START + i;
			}
			mem_blk_num++;
		}
		return 1;
	}

	if ((free_mem->start_offset + free_mem->length) == (mem_tbl[i].offset + mem_tbl[i].size)) {
		/* case 2, no need use one mpu region */
		tmp_end_addr = mem_tbl[i].offset + mem_tbl[i].size - 1;
		mem_tbl[i].size = ((free_mem->start_offset + MPU_64K_VALUE - 1) & MPU_64K_ALIGN_MASK) - mem_tbl[i].offset;
		mem_tbl[i].ext_flag = VALID_PADDING;
		ALWAYS_LOG("[%p-Reserved-%p|%p-Retrieve-%p]\n",
		           mem_addr + mem_tbl[i].offset, mem_addr + mem_tbl[i].offset + mem_tbl[i].size -1,
		           mem_addr + mem_tbl[i].offset + mem_tbl[i].size, mem_addr + tmp_end_addr);
		ccci_retrieve_mem((unsigned char*)(mem_addr + mem_tbl[i].offset + mem_tbl[i].size),
		                  tmp_end_addr - (mem_tbl[i].offset + mem_tbl[i].size) + 1);
		return 0;
	}

	if ((free_mem->start_offset + free_mem->length) > (mem_tbl[i].offset + mem_tbl[i].size)) {
		ALWAYS_LOG("over two region\n");
		return -LD_ERR_PAD_OVER_TWO_REGION;
	}

	return -LD_ERR_PAD_MISC;
}

static int padding_mem_pre_process_v5_hdr(struct image_section_desc mem_tbl[],
        free_padding_block_t free_slot[], int mpu_num)
{
	int i, j;
	int mem_blk_num = 0;
	int padding_mem_num = 0;
	free_padding_block_t *padding_mem;
	struct padding_tag padding_tag_tbl[MAX_PADDING_NUM_V5_HDR+1], *small_ptr = NULL;
	int padding_with_additional_num = 0;
	unsigned int small_length = 0;

	while (mem_tbl[mem_blk_num].offset || mem_tbl[mem_blk_num].size) {
		PADDING_LOG("mem_tbl[mem_blk_num].offset:%x\n", mem_tbl[mem_blk_num].offset);
		PADDING_LOG("mem_tbl[mem_blk_num].size:%x\n", mem_tbl[mem_blk_num].size);
		mem_blk_num++;
	}

	PADDING_LOG("mem_blk_num:%d\n", mem_blk_num);

	while (free_slot[padding_mem_num].start_offset || free_slot[padding_mem_num].length) {
		padding_tag_tbl[padding_mem_num].padding_mem.start_offset = free_slot[padding_mem_num].start_offset;
		padding_tag_tbl[padding_mem_num].padding_mem.length = free_slot[padding_mem_num].length;
		padding_tag_tbl[padding_mem_num].status = VALID_PADDING;

		PADDING_LOG("padding_tag_tbl[%d].offset:0x%x\n", padding_mem_num,
		            padding_tag_tbl[padding_mem_num].padding_mem.start_offset);
		PADDING_LOG("padding_tag_tbl[%d].length:0x%x\n", padding_mem_num,
		            padding_tag_tbl[padding_mem_num].padding_mem.length);
		PADDING_LOG("padding_tag_tbl[%d].status:0x%x\n", padding_mem_num,
		            padding_tag_tbl[padding_mem_num].status);

		padding_mem_num++;
		if (padding_mem_num >= MAX_PADDING_NUM_V5_HDR)
			/* For current design, only have MAX_PADDING_NUM_V5_HDR number padding */
			break;
	}
	for (i = padding_mem_num; i < (MAX_PADDING_NUM_V5_HDR+1); i++)
		padding_tag_tbl[i].status = 0;

	for (j = 0; j < padding_mem_num; j++) {
		padding_mem = &padding_tag_tbl[j].padding_mem;
		/* Find free memory position */
		for (i = 0; i < mem_blk_num; i++) {
			if ((padding_mem->start_offset >= mem_tbl[i].offset) &&
			        (padding_mem->start_offset < (mem_tbl[i].offset + mem_tbl[i].size)))
				break;
		}
		if (i == mem_blk_num)
			continue;

		PADDING_LOG("padding_pre, get: offset:0x%x, size:0x%x\n",
		            padding_mem->start_offset, padding_mem->length);

		if (padding_mem->start_offset == mem_tbl[i].offset) {
			PADDING_LOG("case I\n");
			continue;/* case 0, no need use one mpu region */
		} else if ((padding_mem->start_offset + padding_mem->length) < (mem_tbl[i].offset + mem_tbl[i].size)) {
			/* case 1, need use one mpu region, seperate it to three part */
			padding_tag_tbl[j].status |= NEED_MPU_MORE;
			padding_with_additional_num++;
			PADDING_LOG("case II\n");
		} else if ((padding_mem->start_offset + padding_mem->length) == (mem_tbl[i].offset + mem_tbl[i].size)) {
			PADDING_LOG("case III\n");
			continue;/* case 2, no need use one mpu region */
		} else if ((padding_mem->start_offset + padding_mem->length) > (mem_tbl[i].offset + mem_tbl[i].size)) {
			PADDING_LOG("case IV\n");
			continue;/* over two region */
		}
	}
	ALWAYS_LOG("padding_with_additional_num:%d with mpu_num:%d\n", padding_with_additional_num, mpu_num);

	/* If mpu region not enough, remove the small padding memory part */
	while (padding_with_additional_num > mpu_num) {
		/* Find first tag that with additional region */
		for (i = 0; i < (MAX_PADDING_NUM_V5_HDR + 1); i++) {
			if (padding_tag_tbl[i].status & NEED_MPU_MORE) {
				small_ptr = &padding_tag_tbl[i];
				small_length = small_ptr->padding_mem.length;
				break;
			}
		}

		for (j = i; j < (MAX_PADDING_NUM_V5_HDR + 1); j++) {
			/* Find the smallest padding with mpu */
			if ((padding_tag_tbl[j].status & NEED_MPU_MORE) &&
			        (padding_tag_tbl[j].padding_mem.length < small_length)) {
				small_ptr = &padding_tag_tbl[j];
				small_length = small_ptr->padding_mem.length;
			}
		}
		small_ptr->status |= NEED_REMOVE;
		small_ptr->status &= (~NEED_MPU_MORE);
		padding_mem_num--;
		padding_with_additional_num--;
		ALWAYS_LOG("MPU region not enough, cancel to retrieve padding(offset):0x%08x ~ 0x%08x\n",
		           small_ptr->padding_mem.start_offset,
		           small_ptr->padding_mem.start_offset+small_ptr->padding_mem.length);
	}

	/* Update final padding list
	** Remove smallest item, j always <= i */
	j = 0;
	for (i = 0; i < MAX_PADDING_NUM_V5_HDR ; i++) {
		if (padding_tag_tbl[i].status & NEED_REMOVE)
			continue;

		free_slot[j].start_offset = padding_tag_tbl[i].padding_mem.start_offset;
		free_slot[j].length = padding_tag_tbl[i].padding_mem.length;
		j++;
	}

	free_slot[j].start_offset = 0;/* mark for new end */
	free_slot[j].length = 0;/* mark for new end */

	return padding_mem_num;
}

int retrieve_free_padding_mem_v5_hdr(modem_info_t *info,
                                     struct image_section_desc mem_tbl[], void *hdr, int mpu_num)
{
	int i, j;
	int retrieve_blk_num = 0;
	struct md_check_header_v5 *chk_hdr = (struct md_check_header_v5 *)hdr;
	int mem_blk_num = 0;
	free_padding_block_t free_slot[MAX_PADDING_NUM_V5_HDR+1]; /* Maximux is 3, that confirned with MD */

	if (mpu_num < 1) {
		ALWAYS_LOG("free mpu region not enough for padding memory feature\n");
		return -LD_ERR_PAD_REGION_NOT_ENOUGH;
	}

	j = 0;
	for (i = 0; i < MAX_PADDING_NUM_V5_HDR; i++) {
		if ((chk_hdr->padding_blk[i].start_offset == 0) &&
		        (chk_hdr->padding_blk[i].length == 0))
			continue;
		else {
			free_slot[j].start_offset = chk_hdr->padding_blk[i].start_offset;
			free_slot[j].length = chk_hdr->padding_blk[i].length;
			j++;
		}
	}
	free_slot[j].start_offset = 0; /* Mark for last */
	free_slot[j].length = 0;

	if (0 == j) {
		ALWAYS_LOG("no free padding memmory to retrieve\n");
		return -LD_ERR_PAD_NO_REGION_RETRIEVE;
	}

	/* Reserve 1 region for all range protect usage */
	retrieve_blk_num = padding_mem_pre_process_v5_hdr(mem_tbl, free_slot, mpu_num - 1);

	/* Calculate mem_tbl number again */
	mem_blk_num = 0;
	while (mem_tbl[mem_blk_num].offset || mem_tbl[mem_blk_num].size)
		mem_blk_num++;

	ALWAYS_LOG("retrieve_blk_num: %d\n", retrieve_blk_num);

	for (i = 0; i < retrieve_blk_num; i++) {
		if (free_padding_mem_blk_match(info, mem_tbl, mem_blk_num, &free_slot[i]) > 0)
			mem_blk_num++;
	}

	/* Add lowest padding mpu setting to avoid prefetch violation */
	mem_tbl[mem_blk_num].offset = 0;
	mem_tbl[mem_blk_num].size = chk_hdr->common.mem_size;
	mem_tbl[mem_blk_num].mpu_attr = 0xFFFFFFFF;
	mem_tbl[mem_blk_num].ext_flag = MD_ALL_RANGE;
	mem_tbl[mem_blk_num].relate_idx = 0;

	/* Mark for end */
	mem_blk_num++;
	mem_tbl[mem_blk_num].offset = 0;
	mem_tbl[mem_blk_num].size = 0;

	return 0;
}

static int padding_mem_pre_process_v6_hdr(struct image_section_desc mem_tbl[],
        free_padding_block_t free_slot[], int mpu_num)
{
	int i, j;
	int mem_blk_num = 0;
	int padding_mem_num = 0;
	free_padding_block_t *padding_mem;
	struct padding_tag padding_tag_tbl[MAX_PADDING_NUM_V6_HDR+1], *small_ptr = NULL;
	int padding_with_additional_num = 0;
	unsigned int small_length = 0;

	while (mem_tbl[mem_blk_num].offset || mem_tbl[mem_blk_num].size) {
		PADDING_LOG("mem_tbl[mem_blk_num].offset:%x\n", mem_tbl[mem_blk_num].offset);
		PADDING_LOG("mem_tbl[mem_blk_num].size:%x\n", mem_tbl[mem_blk_num].size);
		mem_blk_num++;
	}

	PADDING_LOG("mem_blk_num:%d\n", mem_blk_num);

	while (free_slot[padding_mem_num].start_offset || free_slot[padding_mem_num].length) {
		padding_tag_tbl[padding_mem_num].padding_mem.start_offset = free_slot[padding_mem_num].start_offset;
		padding_tag_tbl[padding_mem_num].padding_mem.length = free_slot[padding_mem_num].length;
		padding_tag_tbl[padding_mem_num].status = VALID_PADDING;

		PADDING_LOG("padding_tag_tbl[%d].offset:0x%x\n", padding_mem_num,
		            padding_tag_tbl[padding_mem_num].padding_mem.start_offset);
		PADDING_LOG("padding_tag_tbl[%d].length:0x%x\n", padding_mem_num,
		            padding_tag_tbl[padding_mem_num].padding_mem.length);
		PADDING_LOG("padding_tag_tbl[%d].status:0x%x\n", padding_mem_num,
		            padding_tag_tbl[padding_mem_num].status);

		padding_mem_num++;
		if (padding_mem_num >= MAX_PADDING_NUM_V6_HDR)
			/* For current design, only have MAX_PADDING_NUM_V6_HDR number padding */
			break;
	}
	for (i = padding_mem_num; i < (MAX_PADDING_NUM_V6_HDR+1); i++)
		padding_tag_tbl[i].status = 0;

	for (j = 0; j < padding_mem_num; j++) {
		padding_mem = &padding_tag_tbl[j].padding_mem;
		/* Find free memory position */
		for (i = 0; i < mem_blk_num; i++) {
			if ((padding_mem->start_offset >= mem_tbl[i].offset) &&
			        (padding_mem->start_offset < (mem_tbl[i].offset + mem_tbl[i].size)))
				break;
		}
		if (i == mem_blk_num)
			continue;

		PADDING_LOG("padding_pre, get: offset:0x%x, size:0x%x\n",
		            padding_mem->start_offset, padding_mem->length);

		if (padding_mem->start_offset == mem_tbl[i].offset) {
			PADDING_LOG("case I\n");
			continue;/* case 0, no need use one mpu region */
		} else if ((padding_mem->start_offset + padding_mem->length) < (mem_tbl[i].offset + mem_tbl[i].size)) {
			/* case 1, need use one mpu region, seperate it to three part */
			padding_tag_tbl[j].status |= NEED_MPU_MORE;
			padding_with_additional_num++;
			PADDING_LOG("case II\n");
		} else if ((padding_mem->start_offset + padding_mem->length) == (mem_tbl[i].offset + mem_tbl[i].size)) {
			PADDING_LOG("case III\n");
			continue;/* case 2, no need use one mpu region */
		} else if ((padding_mem->start_offset + padding_mem->length) > (mem_tbl[i].offset + mem_tbl[i].size)) {
			PADDING_LOG("case IV\n");
			continue;/* over two region */
		}
	}
	ALWAYS_LOG("padding_with_additional_num:%d with mpu_num:%d\n", padding_with_additional_num, mpu_num);

	/* If mpu region not enough, remove the small padding memory part */
	while (padding_with_additional_num > mpu_num) {
		/* Find first tag that with additional region */
		for (i = 0; i < (MAX_PADDING_NUM_V6_HDR + 1); i++) {
			if (padding_tag_tbl[i].status & NEED_MPU_MORE) {
				small_ptr = &padding_tag_tbl[i];
				small_length = small_ptr->padding_mem.length;
				break;
			}
		}

		for (j = i; j < (MAX_PADDING_NUM_V6_HDR + 1); j++) {
			/* Find the smallest padding with mpu */
			if ((padding_tag_tbl[j].status & NEED_MPU_MORE) &&
			        (padding_tag_tbl[j].padding_mem.length < small_length)) {
				small_ptr = &padding_tag_tbl[j];
				small_length = small_ptr->padding_mem.length;
			}
		}
		small_ptr->status |= NEED_REMOVE;
		small_ptr->status &= (~NEED_MPU_MORE);
		padding_mem_num--;
		padding_with_additional_num--;
		ALWAYS_LOG("MPU region not enough, cancel to retrieve padding(offset):0x%08x ~ 0x%08x\n",
		           small_ptr->padding_mem.start_offset,
		           small_ptr->padding_mem.start_offset+small_ptr->padding_mem.length);
	}

	/* Update final padding list
	** Remove smallest item, j always <= i */
	j = 0;
	for (i = 0; i < MAX_PADDING_NUM_V6_HDR ; i++) {
		if (padding_tag_tbl[i].status & NEED_REMOVE)
			continue;

		free_slot[j].start_offset = padding_tag_tbl[i].padding_mem.start_offset;
		free_slot[j].length = padding_tag_tbl[i].padding_mem.length;
		j++;
	}

	free_slot[j].start_offset = 0;/* mark for new end */
	free_slot[j].length = 0;/* mark for new end */

	return padding_mem_num;
}

int retrieve_free_padding_mem_v6_hdr(modem_info_t *info,
                                     struct image_section_desc mem_tbl[], void *hdr, int mpu_num)
{
	int i, j;
	int retrieve_blk_num = 0;
	struct md_check_header_v6 *chk_hdr = (struct md_check_header_v6 *)hdr;
	int mem_blk_num = 0;
	free_padding_block_t free_slot[MAX_PADDING_NUM_V6_HDR+1]; /* Maximux is 8, that confirned with MD */

	if (mpu_num < 1) {
		ALWAYS_LOG("free mpu region not enough for padding memory feature\n");
		return -LD_ERR_PAD_REGION_NOT_ENOUGH;
	}

	j = 0;
	for (i = 0; i < MAX_PADDING_NUM_V6_HDR; i++) {
		if ((chk_hdr->padding_blk[i].start_offset == 0) &&
		        (chk_hdr->padding_blk[i].length == 0))
			continue;
		else {
			free_slot[j].start_offset = chk_hdr->padding_blk[i].start_offset;
			free_slot[j].length = chk_hdr->padding_blk[i].length;
			j++;
		}
	}
	free_slot[j].start_offset = 0; /* Mark for last */
	free_slot[j].length = 0;

	if (0 == j) {
		ALWAYS_LOG("no free padding memmory to retrieve\n");
		return -LD_ERR_PAD_NO_REGION_RETRIEVE;
	}

	/* Reserve 1 region for all range protect usage */
	retrieve_blk_num = padding_mem_pre_process_v6_hdr(mem_tbl, free_slot, mpu_num - 1);

	/* Calculate mem_tbl number again */
	mem_blk_num = 0;
	while (mem_tbl[mem_blk_num].offset || mem_tbl[mem_blk_num].size)
		mem_blk_num++;

	ALWAYS_LOG("retrieve_blk_num: %d\n", retrieve_blk_num);

	for (i = 0; i < retrieve_blk_num; i++) {
		if (free_padding_mem_blk_match(info, mem_tbl, mem_blk_num, &free_slot[i]) > 0)
			mem_blk_num++;
	}

	/* Add lowest padding mpu setting to avoid prefetch violation */
	mem_tbl[mem_blk_num].offset = 0;
	mem_tbl[mem_blk_num].size = chk_hdr->common.mem_size;
	mem_tbl[mem_blk_num].mpu_attr = 0xFFFFFFFF;
	mem_tbl[mem_blk_num].ext_flag = MD_ALL_RANGE;
	mem_tbl[mem_blk_num].relate_idx = 0;

	/* Mark for end */
	mem_blk_num++;
	mem_tbl[mem_blk_num].offset = 0;
	mem_tbl[mem_blk_num].size = 0;

	return 0;
}

int retrieve_info_num;
void log_retrieve_info(unsigned char *addr, int size)
{
	char buf[32];
	u64 array[2];
	array[0] = (u64)((unsigned long)addr);
	array[1] = (u64)size;
	snprintf(buf, 32, "retrieve%d", retrieve_info_num);
	if (insert_ccci_tag_inf(buf, (char*)&array, sizeof(array)) < 0)
		ALWAYS_LOG("insert %s fail\n", buf);

	retrieve_info_num++;
}

