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
** Sub module: ccci_lk_info (LK to Kernel arguments and information)
**  Using share memory and device tree
**  ccci_lk_info structure is stored at device tree
**  other more detail parameters are stored at share memory
***************************************************************************************************/
static ccci_lk_info_t s_g_ccci_lk_inf;

void ccci_lk_tag_info_init(unsigned long long base_addr)
{
	int i;

	s_g_ccci_lk_inf.lk_info_base_addr = base_addr;
	s_g_ccci_lk_inf.lk_info_size = 0;
	s_g_ccci_lk_inf.lk_info_tag_num = 0;
	s_g_ccci_lk_inf.lk_info_version = CCCI_LK_INFO_VER;
	s_g_ccci_lk_inf.lk_info_ld_flag = 0;
	s_g_ccci_lk_inf.lk_info_err_no = 0;
	for (i=0; i < MAX_MD_NUM; i++)
		s_g_ccci_lk_inf.lk_info_ld_md_errno[i] = 0;
}

void ccci_lk_info_re_cfg(unsigned long long base_addr, unsigned int size)
{
	s_g_ccci_lk_inf.lk_info_base_addr = base_addr;
	s_g_ccci_lk_inf.lk_info_size = size;
}

void update_common_err_to_lk_info(int error)
{
	s_g_ccci_lk_inf.lk_info_err_no = error;
}

void update_md_err_to_lk_info(int md_id, int error)
{
	s_g_ccci_lk_inf.lk_info_ld_md_errno[md_id] = error;
}

int get_md_err_from_lk_info(int md_id)
{
	return s_g_ccci_lk_inf.lk_info_ld_md_errno[md_id];
}

void update_md_load_flag_to_lk_info(int md_id)
{
	s_g_ccci_lk_inf.lk_info_ld_flag |= (1<<md_id);
}

int insert_ccci_tag_inf(char *name, char *data, unsigned int size)
{
	int i;
	unsigned int curr_offset = s_g_ccci_lk_inf.lk_info_size;
	ccci_tag_t *tag = (ccci_tag_t *)((unsigned long)(s_g_ccci_lk_inf.lk_info_base_addr + curr_offset));
	char* buf = (char *)((unsigned long)(s_g_ccci_lk_inf.lk_info_base_addr + curr_offset + sizeof(ccci_tag_t)));
	int total_size = (curr_offset + size + sizeof(ccci_tag_t) + 7)&(~7); /* make sure 8 bytes align */

	if (size == 0){
		ALWAYS_LOG("tag info size is 0\n");
		return 0;
	}

	if (total_size >= MAX_LK_INFO_SIZE) {
		ALWAYS_LOG("not enought memory to insert(%d)\n", MAX_LK_INFO_SIZE - total_size);
		return -LD_ERR_TAG_BUF_FULL;
	}

	/* Copy name */
	for (i=0; i<CCCI_TAG_NAME_LEN-1; i++) {
		if (name[i] == 0)
			break;

		tag->tag_name[i] = name[i];
	}
	tag->tag_name[i] = 0;

	/* Set offset */
	tag->data_offset = curr_offset + sizeof(ccci_tag_t);
	/* Set data size */
	tag->data_size = size;
	/* Set next offset */
	tag->next_tag_offset = total_size;
	/* Copy data */
	memcpy(buf, data, size);

	/* update control structure */
	s_g_ccci_lk_inf.lk_info_size = total_size;
	s_g_ccci_lk_inf.lk_info_tag_num++;

	TAG_DBG_LOG("tag insert(%d), [name]:%s [4 bytes]:[%x][%x][%x][%x] [size]:%d\n",
	            s_g_ccci_lk_inf.lk_info_tag_num, name, data[0], data[1], data[2], data[3], size);

	return 0;
}

void ccci_lk_info_ctl_dump(void)
{
	ALWAYS_LOG("lk info.lk_info_base_addr: 0x%x\n", (unsigned int)s_g_ccci_lk_inf.lk_info_base_addr);
	ALWAYS_LOG("lk info.lk_info_size:      0x%x\n", s_g_ccci_lk_inf.lk_info_size);
	ALWAYS_LOG("lk info.lk_info_tag_num:   0x%x\n", s_g_ccci_lk_inf.lk_info_tag_num);
}

static int dt_reserve_mem_size_fixup(void *fdt)
{
	if (plat_dt_reserve_mem_size_fixup)
		plat_dt_reserve_mem_size_fixup(fdt);

	return 0;
}
/*
** This function will using globle variable: s_g_ccci_lk_inf;
** and a weak function will be called: md_reserve_mem_size_fixup
**/
unsigned int *update_lk_arg_info_to_dt(unsigned int *ptr, void *fdt)
{
	unsigned int i;
	unsigned int *local_ptr;
	unsigned int size = sizeof(ccci_lk_info_t)/sizeof(unsigned int);

	local_ptr = (unsigned int *)&s_g_ccci_lk_inf;
	for (i = 0; i < size; i++) {
		*ptr = local_ptr[i];
		ptr++;
	}

	/* update kernel dt if needed, most platform code do nothing */
	dt_reserve_mem_size_fixup(fdt);

	return ptr;
}