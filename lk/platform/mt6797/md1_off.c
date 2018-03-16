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

#include <platform/upmu_hw.h>
#include <debug.h>

#define UINT32P         (volatile unsigned int *)

#define MD_P_TOPSM_BASE			(0x200D0000)
#define REG_MD_P_TOPSM_RM_PWR0_CON	(UINT32P (MD_P_TOPSM_BASE+0x0800))
#define REG_MD_P_TOPSM_RM_PWR1_CON	(UINT32P (MD_P_TOPSM_BASE+0x0804))
#define REG_MD_P_TOPSM_RM_PWR2_CON	(UINT32P (MD_P_TOPSM_BASE+0x0808))
#define REG_MD_P_TOPSM_RM_PWR3_CON	(UINT32P (MD_P_TOPSM_BASE+0x080C))
#define REG_MD_P_TOPSM_RM_PWR4_CON	(UINT32P (MD_P_TOPSM_BASE+0x0810))
#define REG_MD_P_TOPSM_RM_TMR_PWR0	(UINT32P (MD_P_TOPSM_BASE+0x0018))
#define REG_MD_P_TOPSM_RM_TMR_PWR1	(UINT32P (MD_P_TOPSM_BASE+0x001C))

#define MD_L1_TOPSM_BASE			(0x26070000)
#define REG_MD_L1_TOPSM_SM_TMR_PWR0	(UINT32P (MD_L1_TOPSM_BASE+0x0140))
#define REG_MD_L1_TOPSM_SM_TMR_PWR1	(UINT32P (MD_L1_TOPSM_BASE+0x0144))
#define REG_MD_L1_TOPSM_SM_TMR_PWR2	(UINT32P (MD_L1_TOPSM_BASE+0x0148))
#define REG_MD_L1_TOPSM_SM_TMR_PWR3	(UINT32P (MD_L1_TOPSM_BASE+0x014C))
#define REG_MD_L1_TOPSM_SM_TMR_PWR4	(UINT32P (MD_L1_TOPSM_BASE+0x0150))

#define STA_POWER_DOWN  0
#define STA_POWER_ON    1
void md1_power_down(void)
{
	int ret = 0;

	spm_mtcmos_ctrl_md1(STA_POWER_ON);

	dprintf(CRITICAL, "[ccci-off]0.power on MD_INFRA/MODEM_TOP ret=%d\n", ret);
	if (ret)
		return;

	/* 1. Shutting off ARM7, HSPAL2, LTEL2 power domains */
	/* Shutting off ARM7 through software */
	*REG_MD_P_TOPSM_RM_PWR1_CON &= ~0xE6045;
	*REG_MD_P_TOPSM_RM_PWR1_CON |= 0xB8;
	/* Masking control of ostimer on ARM7,HSPAL2,LTEL2 */
	*REG_MD_P_TOPSM_RM_TMR_PWR0 = 0x01;
	/* De-asserting software power req */
	*REG_MD_P_TOPSM_RM_PWR0_CON &= ~0x44; /* PSMCU */
	*REG_MD_P_TOPSM_RM_PWR2_CON &= ~0x44; /* LTEL2 */
	*REG_MD_P_TOPSM_RM_PWR3_CON &= ~0x44; /* LTEL2 */
	*REG_MD_P_TOPSM_RM_PWR4_CON &= ~0x44; /* INFRA */

	/* 2. PSMCU and INFRA power domains should be shut off at the end,
	after complete register sequence has been executed: */
	*REG_MD_P_TOPSM_RM_TMR_PWR0 = 0x00; /* PSMCU into sleep */
	*REG_MD_P_TOPSM_RM_TMR_PWR1 = 0x00; /* INFRA into sleep */

	/* 3. Shutting off power domains except L1MCU by masking all ostimers control
	on mtcmos power domain: */
	*REG_MD_L1_TOPSM_SM_TMR_PWR0 |= ~(0x1);
	*REG_MD_L1_TOPSM_SM_TMR_PWR1 = 0xFFFFFFFF;
	*REG_MD_L1_TOPSM_SM_TMR_PWR2 = 0xFFFFFFFF;
	*REG_MD_L1_TOPSM_SM_TMR_PWR3 = 0xFFFFFFFF;
	*REG_MD_L1_TOPSM_SM_TMR_PWR4 = 0xFFFFFFFF;

	/* 4. L1MCU power domain is shut off in the end
	after all register sequence has been executed: */

	*REG_MD_L1_TOPSM_SM_TMR_PWR0 = 0xFFFFFFFF;

	dprintf(CRITICAL, "[ccci-off]8.power off ARM7, HSPAL2, LTEL2\n");
	/* no need to poll, as MD SW didn't run and enter sleep mode, polling will not get result */

	spm_mtcmos_ctrl_md1(STA_POWER_DOWN);

	/* VMODEM off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN, 0); /* 0x063A[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_EN, 0); /* 0x062C[0]=0, 0:Disable, 1:Enable */
	/* VMD1 off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VSLEEP_EN, 0); /* 0x064E[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_EN, 0); /* 0x0640[0]=0, 0:Disable, 1:Enable */
	/* VSRAM_MD off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN, 0); /* 0x0662[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_EN, 0); /* 0x0654[0]=0, 0:Disable, 1:Enable */

}
