/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#include <printf.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_rtc.h>
#include <platform/boot_mode.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_pmic.h>
#include <platform/env.h>


#define OFF_MODE_CHARGE         "off-mode-charge"
extern BOOL meta_mode_check(void);
extern int mtk_wdt_boot_check(void);
extern bool rtc_2sec_boot_check(void);

void set_off_mode_charge_status(int enable_charge)
{
	static char *env_buf = NULL;

	env_buf = (char *)malloc(sizeof(int));
	memset(env_buf,0x00,sizeof(int));

	sprintf(env_buf,"%d", enable_charge);
	set_env(OFF_MODE_CHARGE, env_buf);

	free(env_buf);
	return ;
}

int get_off_mode_charge_status(void)
{
	char *charge_buf = NULL;
	int enable_charge = 1;

	charge_buf = get_env(OFF_MODE_CHARGE);

	if (charge_buf == NULL) {
		set_off_mode_charge_status(1);
		return 1;
	}
	enable_charge =  atoi(charge_buf);
	return enable_charge;
}

BOOL is_force_boot(void)
{
	if (rtc_boot_check(true)) {
		dprintf(INFO, "[%s] Bypass Kernel Power off charging mode and enter Alarm Boot\n", __func__);
		return TRUE;
	} else if (meta_mode_check()) {
		dprintf(INFO, "[%s] Bypass Kernel Power off charging mode and enter Meta Boot\n", __func__);
		return TRUE;
	} else if (pmic_detect_powerkey() || mtk_wdt_boot_check()==WDT_BY_PASS_PWK_REBOOT || rtc_2sec_boot_check()) {
		dprintf(INFO, "[%s] Bypass Kernel Power off charging mode and enter Normal Boot\n", __func__);
		g_boot_mode = NORMAL_BOOT;
		return TRUE;
	}

	return FALSE;

}


BOOL kernel_power_off_charging_detection(void)
{
	int off_mode_status = 1;
	/* */
	if (is_force_boot()) {
		//upmu_set_rg_chrind_on(0);
		dprintf(INFO, "[%s] Turn off HW Led\n", __func__);
		return FALSE;
	}

	off_mode_status = get_off_mode_charge_status();
	dprintf(INFO, "[%s] off_mode_status %d\n", __func__, off_mode_status);
	if (upmu_is_chr_det() == KAL_TRUE) {
		if (off_mode_status) {
			g_boot_mode = KERNEL_POWER_OFF_CHARGING_BOOT;
		} else {
			g_boot_mode = NORMAL_BOOT;
			//upmu_set_rg_chrind_on(0);
			return FALSE;
		}
		return TRUE;
	} else {
		/* power off */
#ifndef NO_POWER_OFF
		dprintf(INFO, "[kernel_power_off_charging_detection] power off\n");
		mt6575_power_off();
#endif
		return FALSE;
	}
	/* */
}





