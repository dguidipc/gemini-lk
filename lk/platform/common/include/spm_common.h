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

#if !defined(__SPM_COMMON_H__)
#define __SPM_COMMON_H__

#include <sys/types.h>
#include <platform/mt_typedefs.h>

#ifdef SPM_FW_USE_PARTITION
#define SPM_FW_MAGIC    0x53504D32
#define SPM_FW_UNIT     4

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((unsigned int) &((TYPE *)0)->MEMBER)
#endif

#define align_sz(a, b) (((a) / (b)) * (b))

struct pcm_desc {
	const char *version;    /* PCM code version */
	const u32 *base;        /* binary array base */
	const u32 base_dma;     /* dma addr of base */
	const u16 size;         /* binary array size */
	const u8 sess;          /* session number */
	const u8 replace;       /* replace mode */
	const u16 addr_2nd;     /* 2nd binary array size */
	const u16 reserved;     /* for 32bit alignment */

	u32 vec0;               /* event vector 0 config */
	u32 vec1;               /* event vector 1 config */
	u32 vec2;               /* event vector 2 config */
	u32 vec3;               /* event vector 3 config */
	u32 vec4;               /* event vector 4 config */
	u32 vec5;               /* event vector 5 config */
	u32 vec6;               /* event vector 6 config */
	u32 vec7;               /* event vector 7 config */
	u32 vec8;               /* event vector 8 config */
	u32 vec9;               /* event vector 9 config */
	u32 vec10;              /* event vector 10 config */
	u32 vec11;              /* event vector 11 config */
	u32 vec12;              /* event vector 12 config */
	u32 vec13;              /* event vector 13 config */
	u32 vec14;              /* event vector 14 config */
	u32 vec15;              /* event vector 15 config */
};

#pragma pack(push)
#pragma pack(2)
struct spm_fw_header {
        unsigned int magic;
        unsigned int size;
        char name[58];
};
#pragma pack(pop)

void get_spmfw_version(char *part_name, char *img_name, char *buf, int *wp);

#endif /* SPM_FW_USE_PARTITION */

#endif /* __SPM_COMMON_H__ */

