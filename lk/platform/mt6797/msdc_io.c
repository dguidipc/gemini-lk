/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2010
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#include "msdc.h"
#include "msdc_io.h"

#if defined(MMC_MSDC_DRV_CTP)
#include <common.h>
#include "api.h"    //For invocation cache_clean_invalidate()
#include "cache_api.h"  //For invocation cache_clean_invalidate()
#endif

#if defined(MMC_MSDC_DRV_LK)
#include <kernel/event.h>
#include <platform/mt_irq.h>
#endif

#if defined(MMC_MSDC_DRV_CTP)
#include "gpio.h"

#if defined(MSDC_USE_DCM)
#include "dcm.h"
#endif

#if !defined(FPGA_PLATFORM)
#include "pmic.h"
#include "clock_manager.h"
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Section 1. Power Control -- Common for ASIC and FPGA
//
////////////////////////////////////////////////////////////////////////////////
//Note: MTKDRV_XXX are used by CTP
#if defined(MTKDRV_PMIC) && defined(MTKDRV_PMIC_WRAP) && defined(MTKDRV_I2C)
#include <upmu_common.h>
#else
#define pmic_config_interface(RegNum, val, MASK, SHIFT);
#define pmic_read_interface(RegNum, val, MASK, SHIFT);
#endif

u32 g_msdc0_io;
u32 g_msdc1_io;
u32 g_msdc2_io;
u32 g_msdc3_io;

u32 g_msdc0_flash;
u32 g_msdc1_flash;
u32 g_msdc2_flash;
u32 g_msdc3_flash;

#if defined(FPGA_PLATFORM)
//#define FPGA_GPIO_DEBUG
static void msdc_clr_gpio(u32 bits)
{
	u32 l_val = 0;
	char *name;

	switch (bits) {
		case PWR_MASK_CARD:
			name="card pwr";
			break;
		case PWR_MASK_VOL_18:
			name="SD/eMMC bus pwer 1.8V";
			break;
		case PWR_MASK_VOL_33:
			name="SD/eMMC bus pwer 3.3V";
			break;
		case PWR_MASK_L4:
			name="L4";
			break;
		default:
			printf("[%s:%d]invalid value: 0x%x\n", __FILE__, __func__, bits);
			return;
	}

	MSDC_GET_FIELD(PWR_GPIO_EO, bits, l_val);
	//printf("====%s====%d\n", name, l_val);
	if (0 == l_val) {
		printf("check me! gpio for %s is input\n", name);
		MSDC_SET_FIELD(PWR_GPIO_EO, bits, 1);
	}
	/* check for set before */
	if (bits &  MSDC_READ32(PWR_GPIO)) {
		printf("clear %s\n", name);
		MSDC_SET_FIELD(PWR_GPIO, bits, 0);
		//l_val = MSDC_READ32(PWR_GPIO);
	}

#ifdef FPGA_GPIO_DEBUG
	{
		u32 val = 0;
		val = MSDC_READ32(PWR_GPIO);
		printf("[clr]PWR_GPIO[8-11]:0x%x\n", val);
		val = MSDC_READ32(PWR_GPIO_EO);
		printf("[clr]GPIO_DIR[8-11]	   :0x%x\n", val);
	}
#endif
}

static void msdc_set_gpio(u32 bits)
{
	u32 l_val = 0;
	char *name;

	switch (bits) {
		case PWR_MASK_CARD:
			name="card pwr";
			break;
		case PWR_MASK_VOL_18:
			name="SD/eMMC bus pwer 1.8V";
			break;
		case PWR_MASK_VOL_33:
			name="SD/eMMC bus pwer 3.3V";
			break;
		case PWR_MASK_L4:
			name="L4";
			break;
		default:
			printf("[%s:%d]invalid value: 0x%x\n", __FILE__, __func__, bits);
			return;
	}

	MSDC_GET_FIELD(PWR_GPIO_EO, bits, l_val);
	//printf("====%s====%d\n", name, l_val);
	if (0 == l_val) {
		printf("check me! gpio for %s is input\n", name);
		MSDC_SET_FIELD(PWR_GPIO_EO, bits, 1);
	}
	/* check for set before */
	if ((bits &  MSDC_READ32(PWR_GPIO))==0) {
		printf("Set %s\n", name);
		MSDC_SET_FIELD(PWR_GPIO, bits, 1);
		//l_val = MSDC_READ32(PWR_GPIO);
	}

#ifdef FPGA_GPIO_DEBUG
	{
		u32 val = 0;
		val = MSDC_READ32(PWR_GPIO);
		printf("[set]PWR_GPIO[8-11]:0x%x\n", val);
		val = MSDC_READ32(PWR_GPIO_EO);
		printf("[set]GPIO_DIR[8-11]	   :0x%x\n", val);
	}
#endif
}
#endif

#if defined(MMC_MSDC_DRV_CTP) && !defined(FPGA_PLATFORM)
u32 hwPowerOn(MSDC_POWER powerId, MSDC_POWER_VOLTAGE powerVolt)
{
	switch (powerId) {
		case MSDC_VEMC:
			if (powerVolt == VOL_3000) {
				pmic_config_interface(REG_VEMC_VOSEL, VEMC_VOSEL_3V, MASK_VEMC_VOSEL, SHIFT_VEMC_VOSEL);
				if (CARD_VOL_ACTUAL != VOL_3000) {
					pmic_config_interface(REG_VEMC_VOSEL_CAL,
					                      VEMC_VOSEL_CAL_mV(CARD_VOL_ACTUAL - VOL_3000),
					                      MASK_VEMC_VOSEL_CAL,
					                      SHIFT_VEMC_VOSEL_CAL);
				}
			} else if (powerVolt == VOL_3300) {
				pmic_config_interface(REG_VEMC_VOSEL, VEMC_VOSEL_3V3, MASK_VEMC_VOSEL, SHIFT_VEMC_VOSEL);
			} else {
				printf("Not support to Set VEMC_3V3 power to %d\n", powerVolt);
			}
			pmic_config_interface(REG_VEMC_EN, 1, MASK_VEMC_EN, SHIFT_VEMC_EN);
			break;

		case MSDC_VMC:
			if (powerVolt == VOL_3300 || powerVolt == VOL_3000)
				pmic_config_interface(REG_VMC_VOSEL, VMC_VOSEL_3V, MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
			else if (powerVolt == VOL_1800)
				pmic_config_interface(REG_VMC_VOSEL, VMC_VOSEL_1V8, MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
			else
				printf("Not support to Set VMC power to %d\n", powerVolt);
			pmic_config_interface(REG_VMC_EN, 1, MASK_VMC_EN, SHIFT_VMC_EN);
			break;

		case MSDC_VMCH:
			if (powerVolt == VOL_3000) {
				pmic_config_interface(REG_VMCH_VOSEL, VMCH_VOSEL_3V, MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
				if (CARD_VOL_ACTUAL != VOL_3000) {
					pmic_config_interface(REG_VMCH_VOSEL_CAL,
					                      VMCH_VOSEL_CAL_mV(CARD_VOL_ACTUAL - VOL_3000),
					                      MASK_VMCH_VOSEL_CAL,
					                      SHIFT_VMCH_VOSEL_CAL);
				}
			} else if (powerVolt == VOL_3300) {
				pmic_config_interface(REG_VMCH_VOSEL, VMCH_VOSEL_3V3, MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
			} else {
				printf("Not support to Set VMCH power to %d\n", powerVolt);
			}
			pmic_config_interface(REG_VMCH_EN, 1, MASK_VMCH_EN, SHIFT_VMCH_EN);
			break;

		default:
			printf("Not support to Set %d power on\n", powerId);
			break;
	}

	mdelay(100); /* requires before voltage stable */

	return 0;
}

//FIX ME
//To do: check if PMIC can provide unified interface for PL/LK/CTP and independent of PMIC
u32 hwPowerDown(MSDC_POWER powerId)
{
	switch (powerId) {
		case MSDC_VEMC:
			pmic_config_interface(REG_VEMC_EN, 0, MASK_VEMC_EN, SHIFT_VEMC_EN);
			break;
		case MSDC_VMC:
			pmic_config_interface(REG_VMC_EN, 0, MASK_VMC_EN, SHIFT_VMC_EN);
			break;
		case MSDC_VMCH:
			pmic_config_interface(REG_VMCH_EN, 0, MASK_VMCH_EN, SHIFT_VMCH_EN);
			break;
		default:
			printf("Not support to Set %d power down\n", powerId);
			break;
	}
	return 0;
}

static u32 msdc_ldo_power(u32 on, MSDC_POWER powerId, MSDC_POWER_VOLTAGE powerVolt, u32 *status)
{
	if (on) { // want to power on
		if (*status == 0) {  // can power on
			printf("msdc LDO<%d> power on<%d>\n", powerId, powerVolt);
			hwPowerOn(powerId, powerVolt);
			*status = powerVolt;
		} else if (*status == powerVolt) {
			printf("msdc LDO<%d><%d> power on again!\n", powerId, powerVolt);
		} else { // for sd3.0 later
			printf("msdc LDO<%d> change<%d>	to <%d>\n", powerId, *status, powerVolt);
			hwPowerDown(powerId);
			hwPowerOn(powerId, powerVolt);
			*status = powerVolt;
		}
	} else {  // want to power off
		if (*status != 0) {  // has been powerred on
			printf("msdc LDO<%d> power off\n", powerId);
			hwPowerDown(powerId);
			*status = 0;
		} else {
			printf("LDO<%d>	not power on\n", powerId);
		}
	}

	return 0;
}
#endif /* defined(MMC_MSDC_DRV_CTP) && !defined(FPGA_PLATFORM) */

#if defined(FPGA_PLATFORM)
void msdc_card_power(struct mmc_host *host, u32 on)
{
	MSG(CFG, "[SD%d] Turn %s card power \n", host->id, on ? "on" : "off");
	switch (host->id) {
		case 0:
			if (on) {
				//On V6/V7 FPGA, card power need to be on/off
				//On HAPS FPGA, card power maybe always on.
				//Left this code for safety prevent from card power not always not.
				msdc_set_gpio(PWR_MASK_CARD);
			} else {
				msdc_clr_gpio(PWR_MASK_CARD);
			}
			mdelay(10);
			break;
		default:
			//No MSDC1 in FPGA
			break;
	}

	//add for fpga debug
	msdc_set_gpio(PWR_MASK_L4);
}

void msdc_host_power(struct mmc_host *host, u32 on, u32 level)
{
	MSG(CFG, "[SD%d] Turn %s host power \n", host->id, on ? "on" : "off");

	// GPO[3:2] = {LVL_PWR33, LVL_PWR18};
	msdc_clr_gpio(PWR_MASK_VOL_18);
	msdc_clr_gpio(PWR_MASK_VOL_33);

	switch (host->id) {
		case 0:
			if (on) {
				if (level==VOL_1800)
					msdc_set_gpio(PWR_MASK_VOL_18);
				else
					msdc_set_gpio(PWR_MASK_VOL_33);
			}
			mdelay(10);
			break;
		default:
			//No MSDC1 in FPGA
			break;
	}

	//add for fpga debug
	msdc_set_gpio(PWR_MASK_L4);
}
#else
void msdc_card_power(struct mmc_host *host, u32 on)
{
	//Only CTP support power on/off for verification purose.
	//Preload and LK need not touch power since it is default on
#if defined(MMC_MSDC_DRV_CTP)
	switch (host->id) {
		case 0:
			msdc_ldo_power(on, MSDC_VEMC, VOL_3000, &g_msdc0_flash);
			mdelay(10);
			break;
		case 1:
			msdc_ldo_power(on, MSDC_VMCH, VOL_3000, &g_msdc1_flash);
			mdelay(10);
			break;
		case 2:
			if (on && (host->cur_pwr < VOL_2000) ) {
				//For MTK SDIO device initialized at 1.8V, use external source of 1.8V
				//So we just turn VMCH off
				on = 0;
			}
			msdc_ldo_power(on, MSDC_VMCH, VOL_3000, &g_msdc2_flash);
			mdelay(10);
			break;
		default:
			break;
	}
#endif
}

void msdc_host_power(struct mmc_host *host, u32 on, u32 level)
{
	//Only CTP support power on/off for verification purose.
	//Preload and LK need not touch power since it is default on
#if defined(MMC_MSDC_DRV_CTP)
	u32 card_on=on;
	if (host->id==1 || host->id==2) {
		host->cur_pwr=level;
	}
	msdc_set_tdsel(host, 0, (host->cur_pwr == VOL_1800));
	msdc_set_rdsel(host, 0, (host->cur_pwr == VOL_1800));
	msdc_set_driving(host, &msdc_cap[host->id], (level == VOL_1800));

	MSG(CFG, "[SD%d] Turn %s host power \n", host->id, on ? "on" : "off");

	switch (host->id) {
		case 0:
			//no need change;
			break;
		case 1:
			msdc_ldo_power(on, MSDC_VMC, level, &g_msdc1_io);
			mdelay(10);
			break;
		case 2:
			//For MSDC with SD card, not for SDIO
			msdc_ldo_power(on, MSDC_VMC, level, &g_msdc2_io);
			mdelay(10);
			break;
		default:
			break;
	}
#endif
}
#endif

void msdc_power(struct mmc_host *host, u8 mode)
{
	if (mode == MMC_POWER_ON || mode == MMC_POWER_UP) {
		msdc_pin_config(host, MSDC_PIN_PULL_UP);
		msdc_host_power(host, 1, host->cur_pwr);
		msdc_card_power(host, 1);
		msdc_clock(host, 1);
	} else {
		msdc_clock(host, 0);
		msdc_host_power(host, 0, host->cur_pwr);
		msdc_card_power(host, 0);
		msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
	}
}

void msdc_dump_ldo_sts(struct mmc_host *host)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
	u32 ldo_en = 0, ldo_vol = 0;

	pmic_read_interface(REG_VEMC_EN, &ldo_en, MASK_VEMC_EN,
	                    SHIFT_VEMC_EN);
	pmic_read_interface(REG_VEMC_VOSEL, &ldo_vol, MASK_VEMC_VOSEL,
	                    SHIFT_VEMC_VOSEL);
	printf(" VEMC_EN=0x%x, should:1b'1, "
	       "VEMC_VOL=0x%x, should:1b'0(3.0V),1b'1(3.3V)\n",
	       ldo_en, ldo_vol);
#endif
}


/**************************************************************/
/* Section 2: Clock                    */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
/*  msdc clock source
 *  Clock manager will set a reference value in CCF
 *  Normal code doesn't change clock source
 *  Lower frequence will change clock source
 */
/* msdc50_0_hclk reference value is 273M */
u32 hclks_msdc50_0_hclk[] = {PLLCLK_50M, PLLCLK_273M, PLLCLK_182M, PLLCLK_78M};
/* msdc0 clock source reference value is 400M */
u32 hclks_msdc50_0[] = {PLLCLK_50M, PLLCLK_400M, PLLCLK_364M, PLLCLK_156M,
                        PLLCLK_182M, PLLCLK_156M, PLLCLK_200M, PLLCLK_312M,
                        PLLCLK_416M
                       };
/* msdc1 clock source reference value is 200M */
u32 hclks_msdc30_1[] = {PLLCLK_50M, PLLCLK_208M, PLLCLK_200M, PLLCLK_156M,
                        PLLCLK_182M, PLLCLK_156M, PLLCLK_178M
                       };
u32 *msdc_src_clks = hclks_msdc50_0;
#else
u32 msdc_src_clks[] = {12000000, 12000000, 12000000, 12000000, 12000000,
                       12000000, 12000000, 12000000, 12000000
                      };
#endif

#if !defined(FPGA_PLATFORM)
void msdc_dump_clock_sts(struct mmc_host *host)
{
	/* FIXME need check */
	printf(" msdcpll en[0x1000C250][bit0 should be 0x1]=0x%x\n", MSDC_READ32(0x1000C250));
	printf(" msdcpll on[0x1000C25C][bit0~1 should be 0x1]=0x%x\n", MSDC_READ32(0x1000C25C));

	printf(" pdn, mux[0x10000070][bit23, 16~19 for msdc0, bit31, 24~26 for msdc1]=0x%x\n", MSDC_READ32(0x10000070));
}
#endif

void msdc_clock(struct mmc_host *host, int on)
{
	//Only CTP need enable/disable clock
	//Preloader will turn on clock with predefined clock source and module need not to touch clock setting.
	//LK will use preloader's setting and need not to touch clock setting.
#if !defined(FPGA_PLATFORM)
#if defined(MMC_MSDC_DRV_CTP) && defined(MTKDRV_CLOCK_MANAGER)
	int clk_id[MSDC_MAX_NUM] = {MT_CG_INFRA_MSDC_0, MT_CG_INFRA_MSDC_1, MT_CG_INFRA_MSDC_2, MT_CG_INFRA_MSDC_3};
	MSG(CFG, "[SD%d] Turn %s %s clock \n", host->id, on ? "on" : "off", "host");

	if (on) {
		INFRA_enable_clock(clk_id[host->id]);
	} else {
		INFRA_disable_clock(clk_id[host->id]);
	}
#endif
#endif /* end of FPGA_PLATFORM */
#if !defined(FPGA_PLATFORM)
#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_clock_sts(host);
#endif
#endif
}

/* perloader will pre-set msdc pll and the mux channel of msdc pll */
/* note: pll will not changed */
void msdc_config_clksrc(struct mmc_host *host, u8 clksrc)
{
	// modify the clock
#if !defined(FPGA_PLATFORM)
	if (host->id == 0) {
		msdc_src_clks = hclks_msdc50_0;
	} else {
		msdc_src_clks = hclks_msdc30_1;
	}
#endif

	if (host->card && mmc_card_hs400(host->card)) {
		/* after the card init flow, if the card support hs400 mode
		* modify the mux channel of the msdc pll */

		// mux select
		host->pll_mux_clk = MSDC0_CLKSRC_DEFAULT;
		host->src_clk = msdc_src_clks[MSDC0_CLKSRC_DEFAULT];
		printf("[info][%s] hs400 mode, change pll mux to %dhz\n", __func__, host->src_clk/1000000);
	} else {
		/* Perloader and LK use 200 is ok, no need change source */
#if defined(MMC_MSDC_DRV_PRELOADER) || defined(MMC_MSDC_DRV_LK)
		host->pll_mux_clk = MSDC0_CLKSRC_DEFAULT;
		host->src_clk = msdc_src_clks[MSDC0_CLKSRC_DEFAULT];
#endif

#if defined(MMC_MSDC_DRV_CTP)
		host->pll_mux_clk = clksrc;
		host->src_clk    = msdc_src_clks[clksrc];
#endif
	}

#if !defined(FPGA_PLATFORM)
#if defined(MMC_MSDC_DRV_CTP)
	/* FIXME: need check*/
	if (host->id == 0) {
		MSDC_SET_FIELD((TOPCKGEN_BASE + 0x0070), 0xf << 16, host->pll_mux_clk);
	} else if (host->id == 1) {
		MSDC_SET_FIELD((TOPCKGEN_BASE + 0x0070), 0x7 << 24, host->pll_mux_clk);
	} else if (host->id == 2) {
		MSDC_SET_FIELD((TOPCKGEN_BASE + 0x0080), 0x7 << 0, host->pll_mux_clk);
	}
	MSDC_WRITE32(TOPCKGEN_BASE+0x04, 0x07FFFFFF);
#endif
#endif

	printf("[info][%s] input clock is %dkHz\n", __func__, host->src_clk/1000);
}



/**************************************************************/
/* Section 3: GPIO and Pad                    */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
void msdc_dump_padctl_by_id(u32 id)
{
	switch (id) {
		case 0:
			printf("MSDC0 MODE14[0x%x] =0x%x\tshould:0x111111??\n",
			       MSDC0_GPIO_MODE14, MSDC_READ32(MSDC0_GPIO_MODE14));
			printf("MSDC0 MODE15[0x%x] =0x%x\tshould:0x??111111\n",
			       MSDC0_GPIO_MODE15, MSDC_READ32(MSDC0_GPIO_MODE15));
			printf("MSDC0 SMT   [0x%x] =0x%x\tshould:0x???????f\n",
			       MSDC0_GPIO_SMT_ADDR, MSDC_READ32(MSDC0_GPIO_SMT_ADDR));
			printf("MSDC0 IES   [0x%x] =0x%x\tshould:0x?????fff\n",
			       MSDC0_GPIO_IES_ADDR, MSDC_READ32(MSDC0_GPIO_IES_ADDR));
			printf("PUPD/R0/R1: dat/cmd:0/1/0, clk/dst: 1/0/1\n");
			printf("PUPD/R0/R1: rstb: 0/0/1\n");
			printf("MSDC0 PUPD [0x%x] =0x%x\tshould: 011000000000 ??????b\n",
			       MSDC0_GPIO_PUPD_ADDR,
			       MSDC_READ32(MSDC0_GPIO_PUPD_ADDR));
			printf("MSDC0 RO [0x%x]   =0x%x\tshould: 000111111111 ??????b\n",
			       MSDC0_GPIO_R0_ADDR,
			       MSDC_READ32(MSDC0_GPIO_R0_ADDR));
			printf("MSDC0 R1 [0x%x]   =0x%x\tshould: 111000000000 ??????b\n",
			       MSDC0_GPIO_R1_ADDR,
			       MSDC_READ32(MSDC0_GPIO_R1_ADDR));
			printf("MSDC0 SR    [0x%x] =0x%x\n",
			       MSDC0_GPIO_SR_ADDR,
			       MSDC_READ32(MSDC0_GPIO_SR_ADDR));
			printf("MSDC0 TDSEL [0x%x] =0x%x\tshould:0x????0000\n",
			       MSDC0_GPIO_TDSEL_ADDR,
			       MSDC_READ32(MSDC0_GPIO_TDSEL_ADDR));
			printf("MSDC0 RDSEL [0x%x] =0x%x\tshould:0x???00000\n",
			       MSDC0_GPIO_RDSEL_ADDR,
			       MSDC_READ32(MSDC0_GPIO_RDSEL_ADDR));
			printf("MSDC0 DRV   [0x%x] =0x%x\tshould: 1001001001b\n",
			       MSDC0_GPIO_DRV_ADDR,
			       MSDC_READ32(MSDC0_GPIO_DRV_ADDR));
			break;
		case 1:
			printf("MSDC1 MODE16 [0x%x] =0x%x\n",
			       MSDC1_GPIO_MODE16, MSDC_READ32(MSDC1_GPIO_MODE16));
			printf("MSDC1 SMT    [0x%x] =0x%x\n",
			       MSDC1_GPIO_SMT_ADDR, MSDC_READ32(MSDC1_GPIO_SMT_ADDR));
			printf("MSDC1 IES    [0x%x] =0x%x\n",
			       MSDC1_GPIO_IES_ADDR, MSDC_READ32(MSDC1_GPIO_IES_ADDR));
			printf("MSDC1 PUPD   [0x%x] =0x%x\n",
			       MSDC1_GPIO_PUPD_ADDR,
			       MSDC_READ32(MSDC1_GPIO_PUPD_ADDR));
			printf("MSDC1 RO     [0x%x] =0x%x\n",
			       MSDC1_GPIO_R0_ADDR,
			       MSDC_READ32(MSDC1_GPIO_R0_ADDR));
			printf("MSDC1 R1     [0x%x] =0x%x\n",
			       MSDC1_GPIO_R1_ADDR,
			       MSDC_READ32(MSDC1_GPIO_R1_ADDR));

			printf("MSDC1 SR     [0x%x] =0x%x\n",
			       MSDC1_GPIO_SR_ADDR, MSDC_READ32(MSDC1_GPIO_SR_ADDR));
			printf("MSDC1 TDSEL  [0x%x] =0x%x\n",
			       MSDC1_GPIO_TDSEL_ADDR,
			       MSDC_READ32(MSDC1_GPIO_TDSEL_ADDR));
			/* Note1=> For Vcore=0.7V sleep mode
			 * if TDSEL0~3 don't set to [1111]
			 * the digital output function will fail
			 */
			printf("should 1.8v: sleep:0x?fff????, awake:0x?aaa????\n");
			printf("MSDC1 RDSEL  [0x%x] =0x%x\n",
			       MSDC1_GPIO_RDSEL_ADDR,
			       MSDC_READ32(MSDC1_GPIO_RDSEL_ADDR));
			printf("1.8V: ??0000??, 2.9v: 0x??c30c??\n");
			printf("MSDC1 DRV    [0x%x] =0x%x\n",
			       MSDC1_GPIO_DRV_ADDR, MSDC_READ32(MSDC1_GPIO_DRV_ADDR));
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}
void msdc_set_pin_mode(struct mmc_host *host)
{
#if defined(MMC_MSDC_DRV_CTP)
	switch (host->id) {
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_MODE14, MSDC0_MODE_DAT5_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE14, MSDC0_MODE_DAT4_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE14, MSDC0_MODE_DAT3_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE14, MSDC0_MODE_DAT2_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE14, MSDC0_MODE_DAT1_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE14, MSDC0_MODE_DAT0_MASK, 0x1);

			MSDC_SET_FIELD(MSDC0_GPIO_MODE15, MSDC0_MODE_RSTB_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE15, MSDC0_MODE_DSL_MASK,  0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE15, MSDC0_MODE_CLK_MASK,  0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE15, MSDC0_MODE_CMD_MASK,  0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE15, MSDC0_MODE_DAT7_MASK, 0x1);
			MSDC_SET_FIELD(MSDC0_GPIO_MODE15, MSDC0_MODE_DAT6_MASK, 0x1);
			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_MODE16, MSDC1_MODE_CMD_MASK,  0x1);
			MSDC_SET_FIELD(MSDC1_GPIO_MODE16, MSDC1_MODE_DAT0_MASK, 0x1);
			MSDC_SET_FIELD(MSDC1_GPIO_MODE16, MSDC1_MODE_DAT1_MASK, 0x1);
			MSDC_SET_FIELD(MSDC1_GPIO_MODE16, MSDC1_MODE_DAT2_MASK, 0x1);
			MSDC_SET_FIELD(MSDC1_GPIO_MODE16, MSDC1_MODE_DAT3_MASK, 0x1);
			MSDC_SET_FIELD(MSDC1_GPIO_MODE16, MSDC1_MODE_CLK_MASK,  0x1);
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;

	}
#endif
}
void msdc_set_smt_by_id(u32 id, int set_smt)
{
	switch (id) {
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_SMT_ADDR, MSDC0_SMT_ALL_MASK,
			               (set_smt ? 0xf : 0));
			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_SMT_ADDR, MSDC1_SMT_ALL_MASK,
			               (set_smt ? 0x7 : 0));
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}
/* set pin ies:
 * 0: Disable
 * 1: Enable
 */
void msdc_set_ies_by_id(u32 id, int set_ies)
{
	switch (id) {
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_IES_ADDR, MSDC0_IES_ALL_MASK,
			               (set_ies ? 0xfff : 0));
			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_IES_ADDR, MSDC1_IES_ALL_MASK,
			               (set_ies ? 0x3f : 0));
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}
/* msdc pin config
 * MSDC0
 * PUPD/RO/R1
 * 0/0/0: High-Z
 * 0/0/1: Pull-up with 50Kohm
 * 0/1/0: Pull-up with 10Kohm
 * 0/1/1: Pull-up with 10Kohm//50Kohm
 * 1/0/0: High-Z
 * 1/0/1: Pull-down with 50Kohm
 * 1/1/0: Pull-down with 10Kohm
 * 1/1/1: Pull-down with 10Kohm//50Kohm
 */
void msdc_pin_config_by_id(u32 id, u32 mode)
{
	switch (id) {
		case 0:
			/*Attention: don't pull CLK high; Don't toggle RST to prevent
			  from entering boot mode */
			if (MSDC_PIN_PULL_NONE == mode) {
				/* 0/0/0: High-Z */
				MSDC_SET_FIELD(MSDC0_GPIO_PUPD_ADDR,
				               MSDC0_PUPD_ALL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R0_ADDR,
				               MSDC0_R0_ALL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R1_ADDR,
				               MSDC0_R1_ALL_MASK, 0x0);
			} else if (MSDC_PIN_PULL_DOWN == mode) {
				/* all:1/0/1: Pull-down with 50Kohm*/
				MSDC_SET_FIELD(MSDC0_GPIO_PUPD_ADDR,
				               MSDC0_PUPD_ALL_MASK, 0xfff);
				MSDC_SET_FIELD(MSDC0_GPIO_R0_ADDR,
				               MSDC0_R0_ALL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R1_ADDR,
				               MSDC0_R1_ALL_MASK, 0xfff);
			} else if (MSDC_PIN_PULL_UP == mode) {
				/* clk/dsl: 1/0/1: Pull-down with 50Kohm */
				MSDC_SET_FIELD(MSDC0_GPIO_PUPD_ADDR,
				               MSDC0_PUPD_CLK_DSL_MASK, 0x3);
				MSDC_SET_FIELD(MSDC0_GPIO_R0_ADDR,
				               MSDC0_R0_CLK_DSL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R1_ADDR,
				               MSDC0_R1_CLK_DSL_MASK, 0x3);
				/* cmd/dat: 0/1/0: Pull-up with 10Kohm*/
				MSDC_SET_FIELD(MSDC0_GPIO_PUPD_ADDR,
				               MSDC0_PUPD_CMD_DAT_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R0_ADDR,
				               MSDC0_R0_CMD_DAT_MASK, 0x1ff);
				MSDC_SET_FIELD(MSDC0_GPIO_R1_ADDR,
				               MSDC0_R1_CMD_DAT_MASK, 0x0);
				/* rstb: 0/0/1: Pull-up with 50Kohm*/
				MSDC_SET_FIELD(MSDC0_GPIO_PUPD_ADDR,
				               MSDC0_PUPD_RSTB_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R0_ADDR,
				               MSDC0_R0_RSTB_MASK, 0x0);
				MSDC_SET_FIELD(MSDC0_GPIO_R1_ADDR,
				               MSDC0_R1_RSTB_MASK, 0x1);
			}
			break;
		case 1:
			if (MSDC_PIN_PULL_NONE == mode) {
				/* high-Z */
				MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				               MSDC1_PUPD_ALL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC1_GPIO_R0_ADDR,
				               MSDC1_R0_ALL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC1_GPIO_R1_ADDR,
				               MSDC1_R1_ALL_MASK, 0x0);
			} else if (MSDC_PIN_PULL_DOWN == mode) {
				/* ALL: 1/0/1: Pull-down with 50Kohm */
				MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				               MSDC1_PUPD_ALL_MASK, 0x3f);
				MSDC_SET_FIELD(MSDC1_GPIO_R0_ADDR,
				               MSDC1_R0_ALL_MASK, 0x0);
				MSDC_SET_FIELD(MSDC1_GPIO_R1_ADDR,
				               MSDC1_R1_ALL_MASK, 0x3f);
			} else if (MSDC_PIN_PULL_UP == mode) {
				/* cmd/dat:0/0/1: Pull-up with 50Kohm */
				MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				               MSDC1_PUPD_CMD_DAT_MASK, 0x0);
				MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				               MSDC1_R0_CMD_DAT_MASK, 0x0);
				MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				               MSDC1_R1_CMD_DAT_MASK, 0x1f);
				/* clk: 1/0/1: Pull-down with 50Kohm */
				MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				               MSDC1_PUPD_CLK_MASK, 0x1);
				MSDC_SET_FIELD(MSDC1_GPIO_R0_ADDR,
				               MSDC1_R0_CLK_MASK, 0x0);
				MSDC_SET_FIELD(MSDC1_GPIO_R1_ADDR,
				               MSDC1_R1_CLK_MASK, 0x1);

			}
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}
void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat)
{
	switch (id) {
		case 0:
			/* clk */
			MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_CLK_MASK,
			               (clk != 0));
			/* cmd dsl rstb */
			MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_CMD_DSL_RSTB_MASK,
			               (cmd != 0));
			/* dat */
			MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_DAT3_0_MASK,
			               (dat != 0));
			MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_DAT7_4_MASK,
			               (dat != 0));

			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_SR_ADDR, MSDC1_SR_CMD_MASK,
			               (cmd != 0));
			MSDC_SET_FIELD(MSDC1_GPIO_SR_ADDR, MSDC1_SR_CLK_MASK,
			               (clk != 0));
			MSDC_SET_FIELD(MSDC1_GPIO_SR_ADDR, MSDC1_SR_DAT_MASK,
			               (dat != 0));
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}
void msdc_set_tdsel_by_id(u32 id, bool sleep, bool sd_18)
{
	switch (id) {
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_ALL_MASK,
			               (sleep ? 0xfff : 0x0));
			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_ALL_MASK,
			               (sleep ? 0xfff : 0xaaa));
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}

void msdc_set_rdsel_by_id(u32 id, bool sleep, bool sd_18)
{
	switch (id) {
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_ALL_MASK, 0);
			break;

		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_RDSEL_ADDR, MSDC1_RDSEL_ALL_MASK,
			               (sd_18 ? 0 : 0xc30c));
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}

/*
 * Set pin driving:
 * MSDC0,MSDC1
 * 000: 2mA
 * 001: 4mA
 * 010: 6mA
 * 011: 8mA
 * 100: 10mA
 * 101: 12mA
 * 110: 14mA
 * 111: 16mA
 */
void msdc_set_driving_by_id(u32 id, struct msdc_cust *hw, bool sd_18)
{
	switch (id) {
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_DSL_RSTB_MASK,
			               hw->cmd_drv);
			MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			               hw->clk_drv);
			MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT3_0_MASK,
			               hw->dat_drv);
			MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT7_4_MASK,
			               hw->dat_drv);

			break;
		case 1:
			if (sd_18) {
				MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
				               hw->cmd_18v_drv);
				MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
				               hw->clk_18v_drv);
				MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
				               hw->dat_18v_drv);
			} else {
				MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
				               hw->cmd_drv);
				MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
				               hw->clk_drv);
				MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
				               hw->dat_drv);
			}
			break;
			/* set BIAS Tune is 0x5 */
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_BIAS_MASK, 0x5);
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}

void msdc_get_driving_by_id(u32 id, struct msdc_cust *hw)
{
	switch (id) {
		case 0:
			MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_DSL_RSTB_MASK,
			               hw->cmd_drv);
			MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			               hw->clk_drv);
			MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			               hw->dat_drv);
			hw->ds_drv = hw->cmd_drv;
			hw->rst_drv = hw->cmd_drv;
			break;
		case 1:
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
			               hw->cmd_drv);
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
			               hw->clk_drv);
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
			               hw->dat_drv);
			break;
		default:
			printf("[%s] invalid host->id!\n", __func__);
			break;
	}
}
/**************************************************************/
/* Section 4: MISC                        */
/**************************************************************/
/* make sure the pad is msdc mode */
void msdc_gpio_and_pad_init(struct mmc_host *host)
{
	/* set smt enable */
	msdc_set_smt(host, 1);

	/* set pull enable */
	msdc_pin_config(host, MSDC_PIN_PULL_UP);

	/* set gpio to msdc mode */
	msdc_set_pin_mode(host);

	/* set driving */
	msdc_set_driving(host, &msdc_cap[host->id], (host->cur_pwr == VOL_1800));

	/* set tdsel and rdsel */
	msdc_set_tdsel(host, 0, (host->cur_pwr == VOL_1800));
	msdc_set_rdsel(host, 0, (host->cur_pwr == VOL_1800));
#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_padctl_by_id(host->id);
#endif
}
#endif
