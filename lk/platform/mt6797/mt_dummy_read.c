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

#include <reg.h>
#include <debug.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/env.h>
#include <libfdt.h>

extern BOOT_ARGUMENT *g_boot_arg;
static char dram_dummy_read_str[][40]= {"reserve-memory-dram_r0_dummy_read", "reserve-memory-dram_r1_dummy_read"};
#define MIN_MBLOCK_SIZE 0x8000000	/* 128MB */

int target_fdt_dram_dummy_read(void *fdt, unsigned int rank_num)
{
	u32 i, j;
	int nodeoffset, parentoffset, ret = 0;
	dt_dram_info rsv_mem_reg_property[2];
	dt_size_info rsv_mem_size_property[2];
	mblock_info_t *mblock_info = &g_boot_arg->mblock_info;
	dram_info_t *orig_dram_info = &g_boot_arg->orig_dram_info;

	parentoffset = fdt_path_offset(fdt, "/reserved-memory");
	if (parentoffset < 0) {
		dprintf(CRITICAL, "[DRAMC] Warning: can't find reserved-memory node in device tree\n");
		return -1;
	}

	for (j = 0; j < orig_dram_info->rank_num; j++) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			dprintf(CRITICAL, "j:%d, mblock[%d].rank: %d, size: 0x%llx\n",
					j, i, mblock_info->mblock[i].rank,
					(unsigned long long)mblock_info->mblock[i].size);
			if ((mblock_info->mblock[i].rank != j) ||
					(mblock_info->mblock[i].size) < MIN_MBLOCK_SIZE)
				continue;
			rsv_mem_reg_property[j].start_hi = cpu_to_fdt32((mblock_info->mblock[i].start)>>32);
			rsv_mem_reg_property[j].start_lo = cpu_to_fdt32(mblock_info->mblock[i].start);
			rsv_mem_reg_property[j].size_hi = cpu_to_fdt32((mblock_info->mblock[i].size)>>32);
			rsv_mem_reg_property[j].size_lo = cpu_to_fdt32(mblock_info->mblock[i].size);
			rsv_mem_size_property[j].size_hi = cpu_to_fdt32(0);
			rsv_mem_size_property[j].size_lo = cpu_to_fdt32(0x00001000);

			nodeoffset = fdt_add_subnode(fdt, parentoffset, dram_dummy_read_str[j]);
			if (nodeoffset < 0) {
				dprintf(CRITICAL, "Warning: can't find reserved dram rank%d dummy read node in device tree\n", j);
				return -1;
			}

			ret = fdt_setprop_string(fdt, nodeoffset, "compatible", dram_dummy_read_str[j]);
			if (ret) {
				dprintf(CRITICAL, "Warning: can't add compatible  in device tree\n");
				return -1;
			}

			ret = fdt_setprop(fdt, nodeoffset, "size", &rsv_mem_size_property[j], sizeof(dt_size_info));
			ret |= fdt_setprop(fdt, nodeoffset, "alignment", &rsv_mem_size_property[j], sizeof(dt_size_info));
			dprintf(INFO," rsv mem rsv_mem_reg_property[%d].start_hi = 0x%08X\n", j, rsv_mem_reg_property[j].start_hi);
			dprintf(INFO," rsv mem rsv_mem_reg_property[%d].start_lo = 0x%08X\n", j, rsv_mem_reg_property[j].start_lo);
			dprintf(INFO," rsv mem rsv_mem_reg_property[%d].size_hi = 0x%08X\n", j, rsv_mem_reg_property[j].size_hi);
			dprintf(INFO," rsv mem rsv_mem_reg_property[%d].size_lo = 0x%08X\n", j, rsv_mem_reg_property[j].size_lo);
			ret |= fdt_setprop(fdt, nodeoffset, "alloc-ranges", &rsv_mem_reg_property[j], sizeof(dt_dram_info));
			if (ret) {
				dprintf(CRITICAL, "Warning: can't setprop size, alignment and alloc-ranges in device tree\n");
				return -1;
			}
			break;
		}
		if (!rsv_mem_size_property[j].size_lo) {
			dprintf(CRITICAL, "cannot find a mblock for dummy read\n");
			return -1;
		}
	}

	return 0;
}

