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

#if !defined(__AEE_PLATFORM_DEBUG_H__)
#define __AEE_PLATFORM_DEBUG_H__

#include <sys/types.h>
#include <platform/boot_mode.h>

typedef unsigned long long u64;
typedef unsigned long long (*CALLBACK)(void *data, unsigned long sz);

/* extern global variable */
extern BOOT_ARGUMENT *g_boot_arg;

enum {
	AEE_PLAT_DFD20,
	AEE_PLAT_DRAM,
	AEE_PLAT_CPU_BUS,
	AEE_PLAT_SPM_DATA,
	AEE_PLAT_ATF_LAST_LOG,
	AEE_PLAT_ATF_CRASH_REPORT,
	AEE_PLAT_ATF_RAW_LOG,
	AEE_PLAT_HVFS,
	AEE_PLAT_SSPM_COREDUMP,
	AEE_PLAT_SSPM_DATA,
	AEE_PLAT_SSPM_LAST_LOG,
	AEE_PLAT_DEBUG_NUM
};

/* function pointers */
extern unsigned int (* plat_dfd20_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_dram_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_cpu_bus_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_spm_data_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_hvfs_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_sspm_coredump_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_sspm_data_get)(u64 offset, int *len, CALLBACK dev_write);
extern unsigned int (* plat_sspm_log_get)(u64 offset, int *len, CALLBACK dev_write);

/* DRAM KLOG at MRDUMP area of expdb, offset from bottom = 3145728 - 16384 = 3129344 */
#define MRDUMP_EXPDB_BOTTOM_OFFSET 2097152
#define MRDUMP_EXPDB_DRAM_KLOG_OFFSET 3129344

/* common api */
unsigned int kedump_plat_savelog(int condition, u64 offset, int *len, CALLBACK dev_write);

/* common interface for platform */
void mrdump_write_log(u64 offset_dst, void *data, int len);
void mrdump_read_log(void *data, int len, u64 offset);

int lkdump_debug_init(void);
/* common interface from platform */
int platform_debug_init(void);

#endif /* __AEE_PLATFORM_DEBUG_H__ */
