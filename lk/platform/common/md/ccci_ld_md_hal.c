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

#define MODULE_NAME "LK_LD_MD"

/******************************************************************************/
/* Platform code wrapper */
long long ccci_hal_get_ld_md_plat_setting(char cfg_name[])
{
	long long (*NULL_FP)(char str[]) = 0;
	if (strcmp(cfg_name, "support_detect") == 0) {
		if (NULL_FP == plat_ccci_get_ld_md_plat_setting)
			return 0LL;
		return 1LL;
	}

	if (NULL_FP == plat_ccci_get_ld_md_plat_setting)
		return 0LL;

	return plat_ccci_get_ld_md_plat_setting(cfg_name);
}

int ccci_hal_get_mpu_num_for_padding_mem(void)
{
	int (*NULL_FP)(void) = 0;
	if (NULL_FP == plat_get_padding_mpu_num)
		return 0;

	return plat_get_padding_mpu_num();
}

int ccci_hal_apply_hw_remap_for_md_ro_rw(void *md_info)
{
	int (*NULL_FP)(void *) = 0;
	if (NULL_FP == plat_apply_hw_remap_for_md_ro_rw)
		return -1;

	return plat_apply_hw_remap_for_md_ro_rw(md_info);
}

int ccci_hal_apply_hw_remap_for_md_smem(void *addr, int size)
{
	int (*NULL_FP)(void *, int) = 0;
	if (NULL_FP == plat_apply_hw_remap_for_md_smem)
		return -1;

	return plat_apply_hw_remap_for_md_smem(addr, size);
}

int ccci_hal_send_mpu_info_to_platorm(void *md_info, void *mem_info)
{
	int (*NULL_FP)(void *, void *) = 0;
	if (NULL_FP == plat_send_mpu_info_to_platorm)
		return -1;

	return plat_send_mpu_info_to_platorm(md_info, mem_info);
}

int ccci_hal_apply_platform_setting(int load_flag)
{
	int (*NULL_FP)(int) = 0;
	if (NULL_FP == plat_apply_platform_setting)
		return -1;

	return plat_apply_platform_setting(load_flag);
}

void ccci_hal_post_hdr_info(void *hdr, int ver, int id)
{
	void (*NULL_FP)(void *, int, int) = 0;
	if (NULL_FP == plat_post_hdr_info)
		return;

	plat_post_hdr_info(hdr, ver, id);
}


