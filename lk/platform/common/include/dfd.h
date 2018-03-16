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

#ifndef __DFD_H__
#define __DFD_H__

enum {
	DFD_V1_0, /* no DFD internal dump support */
	DFD_V2_0,
	DFD_V2_5,
	DFD_V2_6,
	DFD_V3_0,
};

enum {
	DFD_DUMP_TO_SRAM,
	DFD_DUMP_TO_DRAM,
	DFD_DUMP_NOT_SUPPORT,
};

struct decoded_lastpc {
	unsigned long long pc;
	unsigned long long fp_64;
	unsigned long long sp_64;
	unsigned long fp_32;
	unsigned long sp_32;
};

struct plt_dfd_op {
	void (*acquire_ram_control)(void);
	void (*release_ram_control)(void);
};

struct plt_cfg_dfd {
	unsigned int version;
	unsigned int nr_header_row;
	unsigned long long buffer_addr;
	unsigned long buffer_length;
	unsigned long plat_sram_flag1;
	unsigned long plat_sram_flag2;
	unsigned long dfd_disable_devinfo_index;
	unsigned long dfd_disable_bit;
	unsigned long chip_id_offset;
	char chip_id[8];
};

extern const struct plt_cfg_dfd cfg_dfd;
extern struct plt_dfd_op dfd_op;

unsigned int get_efuse_dfd_disabled(void);

unsigned int dfd_internal_dump_before_reboot();
unsigned int dfd_internal_dump_get_decoded_lastpc(char* buf);

extern unsigned int plt_get_dfd_dump_type();
#endif
