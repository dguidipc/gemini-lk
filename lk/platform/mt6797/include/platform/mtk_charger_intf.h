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

#ifndef __MTK_CHARGER_INTF_H
#define __MTK_CHARGER_INTF_H

#include <platform/mt_typedefs.h>
#include <string.h>
#include <printf.h>

struct mtk_charger_info;
struct mtk_charger_ops {
	int (*dump_register)(struct mtk_charger_info *mchr_info);
	int (*enable_charging)(struct mtk_charger_info *mchr_info, bool enable);
	int (*get_ichg)(struct mtk_charger_info *mchr_info, unsigned int *ichg);
	int (*set_ichg)(struct mtk_charger_info *mchr_info, unsigned int ichg);
	int (*get_aicr)(struct mtk_charger_info *mchr_info, unsigned int *aicr);
	int (*set_aicr)(struct mtk_charger_info *mchr_info, unsigned int aicr);
	int (*set_mivr)(struct mtk_charger_info *mchr_info, unsigned int mivr);
	int (*enable_power_path)(struct mtk_charger_info *mchr_info,
		bool enable);
	int (*reset_pumpx)(struct mtk_charger_info *mchr_info, bool reset);
	int (*reset_wdt)(struct mtk_charger_info *mchr_info);
	int (*sw_reset)(struct mtk_charger_info *mchr_info);
};

struct mtk_charger_info {
	const char *name;
	const char *alias_name;
	unsigned char device_id;
	struct mtk_charger_ops *mchr_ops;
};

extern int mtk_charger_init(void);
extern int mtk_charger_set_info(struct mtk_charger_info *mchr_info);
extern struct mtk_charger_info *mtk_charger_get_by_name(const char *name);
static inline bool mtk_charger_check_alias_name(
	struct mtk_charger_info *mchr_info, const char *name)
{
	if (!mchr_info)
		return false;

	if (strcmp(mchr_info->alias_name, name) == 0)
		return true;

	return false;
}

extern int mtk_charger_dump_register(struct mtk_charger_info *mchr_info);
extern int mtk_charger_enable_charging(struct mtk_charger_info *mchr_info,
	bool enable);
extern int mtk_charger_get_ichg(struct mtk_charger_info *mchr_info,
	unsigned int *ichg);
extern int mtk_charger_set_ichg(struct mtk_charger_info *mchr_info,
	unsigned int ichg);
extern int mtk_charger_get_aicr(struct mtk_charger_info *mchr_info,
	unsigned int *aicr);
extern int mtk_charger_set_aicr(struct mtk_charger_info *mchr_info,
	unsigned int aicr);
extern int mtk_charger_set_mivr(struct mtk_charger_info *mchr_info,
	unsigned int mivr);
extern int mtk_charger_enable_power_path(struct mtk_charger_info *mchr_info,
	bool enable);
extern int mtk_charger_reset_pumpx(struct mtk_charger_info *mchr_info,
	bool reset);
extern int mtk_charger_reset_wdt(struct mtk_charger_info *mchr_info);
extern int mtk_charger_sw_reset(struct mtk_charger_info *mchr_info);

#endif /* __MTK_CHARGER_INTF_H */
