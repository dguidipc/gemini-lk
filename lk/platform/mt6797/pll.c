/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <platform/mt_typedefs.h>


#define APMIXED_BASE      (0x1000C000)

#define MFGPLL_CON0              (APMIXED_BASE + 0x240)
#define MFGPLL_PWR_CON0          (APMIXED_BASE + 0x24C)

#define TVDPLL_CON0             (APMIXED_BASE + 0x270)
#define TVDPLL_PWR_CON0         (APMIXED_BASE + 0x27C)

#define CODECPLL_CON0          (APMIXED_BASE + 0x290)
#define CODECPLL_PWR_CON0      (APMIXED_BASE + 0x29C)

#define APLL1_CON0              (APMIXED_BASE + 0x2A0)
#define APLL1_PWR_CON0          (APMIXED_BASE + 0x2B0)

#define APLL2_CON0              (APMIXED_BASE + 0x2B4)
#define APLL2_PWR_CON0          (APMIXED_BASE + 0x2C4)

#define VDECPLL_CON0              (APMIXED_BASE + 0x2E4)
#define VDECPLL_PWR_CON0          (APMIXED_BASE + 0x2F0)

#define PLL_PWR_CON0_PWR_ON_BIT 0
#define PLL_PWR_CON0_ISO_EN_BIT 1
#define PLL_CON0_EN_BIT 0

void mt_pll_turn_off(void)
{
	unsigned int temp;

    /***********************
      * xPLL Frequency Disable
      ************************/
	temp = DRV_Reg32(MFGPLL_CON0);
	DRV_WriteReg32(MFGPLL_CON0, temp & ~(1<<PLL_CON0_EN_BIT));

	temp = DRV_Reg32(VDECPLL_CON0);
	DRV_WriteReg32(VDECPLL_CON0, temp & ~(1<<PLL_CON0_EN_BIT));

	temp = DRV_Reg32(CODECPLL_CON0);
	DRV_WriteReg32(CODECPLL_CON0, temp & ~(1<<PLL_CON0_EN_BIT));

	temp = DRV_Reg32(TVDPLL_CON0);
	DRV_WriteReg32(TVDPLL_CON0, temp & ~(1<<PLL_CON0_EN_BIT));

	temp = DRV_Reg32(APLL1_CON0);
	DRV_WriteReg32(APLL1_CON0, temp & ~(1<<PLL_CON0_EN_BIT));

	temp = DRV_Reg32(APLL2_CON0);
	DRV_WriteReg32(APLL2_CON0, temp & ~(1<<PLL_CON0_EN_BIT));

    /******************
    * xPLL PWR ISO Enable
    *******************/
	temp = DRV_Reg32(MFGPLL_PWR_CON0);
	DRV_WriteReg32(MFGPLL_PWR_CON0, temp | (1<<PLL_PWR_CON0_ISO_EN_BIT));

	temp = DRV_Reg32(VDECPLL_PWR_CON0);
	DRV_WriteReg32(VDECPLL_PWR_CON0, temp | (1<<PLL_PWR_CON0_ISO_EN_BIT));

	temp = DRV_Reg32(CODECPLL_PWR_CON0);
	DRV_WriteReg32(CODECPLL_PWR_CON0, temp | (1<<PLL_PWR_CON0_ISO_EN_BIT));

	temp = DRV_Reg32(TVDPLL_PWR_CON0);
	DRV_WriteReg32(TVDPLL_PWR_CON0, temp | (1<<PLL_PWR_CON0_ISO_EN_BIT));

	temp = DRV_Reg32(APLL1_PWR_CON0);
	DRV_WriteReg32(APLL1_PWR_CON0, temp | (1<<PLL_PWR_CON0_ISO_EN_BIT));

	temp = DRV_Reg32(APLL2_PWR_CON0);
	DRV_WriteReg32(APLL2_PWR_CON0, temp | (1<<PLL_PWR_CON0_ISO_EN_BIT));

    /*************
    * xPLL PWR OFF
    **************/
	temp = DRV_Reg32(MFGPLL_PWR_CON0);
	DRV_WriteReg32(MFGPLL_PWR_CON0, temp & ~(1<<PLL_PWR_CON0_PWR_ON_BIT));

	temp = DRV_Reg32(VDECPLL_PWR_CON0);
	DRV_WriteReg32(VDECPLL_PWR_CON0, temp & ~(1<<PLL_PWR_CON0_PWR_ON_BIT));

	temp = DRV_Reg32(CODECPLL_PWR_CON0);
	DRV_WriteReg32(CODECPLL_PWR_CON0, temp & ~(1<<PLL_PWR_CON0_PWR_ON_BIT));

	temp = DRV_Reg32(TVDPLL_PWR_CON0);
	DRV_WriteReg32(TVDPLL_PWR_CON0, temp & ~(1<<PLL_PWR_CON0_PWR_ON_BIT));

	temp = DRV_Reg32(APLL1_PWR_CON0);
	DRV_WriteReg32(APLL1_PWR_CON0, temp & ~(1<<PLL_PWR_CON0_PWR_ON_BIT));

	temp = DRV_Reg32(APLL2_PWR_CON0);
	DRV_WriteReg32(APLL2_PWR_CON0, temp & ~(1<<PLL_PWR_CON0_PWR_ON_BIT));

}

