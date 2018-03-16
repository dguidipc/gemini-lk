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

#ifndef __ENV_H__
#define __ENV_H__

#define CFG_ENV_SIZE 0x4000 //(16KB)
#define CFG_ENV_RW_OFFSET   0x20000
#define CFG_ENV_RO_OFFSET   0x24000

#define CFG_ENV_DATA_SIZE (CFG_ENV_SIZE-sizeof(g_env[area].checksum)-sizeof(g_env[area].sig)-sizeof(g_env[area].sig_1))
#define CFG_ENV_DATA_OFFSET (sizeof(g_env[area].sig))
#define CFG_ENV_SIG_1_OFFSET (CFG_ENV_SIZE - sizeof(g_env[area].checksum)-sizeof(g_env[area].sig_1))
#define CFG_ENV_CHECKSUM_OFFSET (CFG_ENV_SIZE - sizeof(g_env[area].checksum))

#define ENV_PART "para"

#define ENV_SIG "ENV_v1"

typedef struct env_struct {
	char sig[8]; // "ENV_v1"
	char *env_data;
	char sig_1[8];  //"ENV_v1"
	int checksum; // checksum for env_data
} env_t;

enum {
	SYSENV_RW_AREA = 0,
	SYSENV_RO_AREA,
	SYSENV_AREA_MAX,
};

extern void env_init(void);
extern char *get_env(char *name);
extern char *get_ro_env(char *name);
extern int set_env(char *name,char *value);
extern void print_env(void);
#endif
