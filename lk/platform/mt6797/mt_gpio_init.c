/******************************************************************************
 * gpio_init.c - MT6516 Linux GPIO Device Driver
 *
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 *
 * DESCRIPTION:
 *     default GPIO init
 *
 ******************************************************************************/

#include <platform/mt_gpio.h>


#if (!defined(MACH_FPGA) || defined (FPGA_SIMULATION))
#include <cust_power.h>
#include <cust_gpio_boot.h>
#endif

#include <platform/mt_reg_base.h>
#include <platform/gpio_init.h>
#include <platform/gpio_cfg.h>

#include "boot_mode.h"
#include <debug.h>
#define GPIO_INIT_DEBUG 1
/*----------------------------------------------------------------------------*/

#define GPIOTAG "[GPIO] "
/*

#define GPIODBG(fmt, arg...)    dprintf(INFO, GPIOTAG "%s: " fmt, __FUNCTION__ ,##arg)
#define GPIOERR(fmt, arg...)    dprintf(INFO, GPIOTAG "%s: " fmt, __FUNCTION__ ,##arg)
#define GPIOVER(fmt, arg...)    dprintf(INFO, GPIOTAG "%s: " fmt, __FUNCTION__ ,##arg)
*/
#define GPIODBG(fmt, arg...)    dprintf(CRITICAL, GPIOTAG "%s: " fmt, __FUNCTION__ ,##arg)
#define GPIOERR(fmt, arg...)    dprintf(CRITICAL, GPIOTAG "%s: " fmt, __FUNCTION__ ,##arg)
#define GPIOVER(fmt, arg...)    dprintf(CRITICAL, GPIOTAG "%s: " fmt, __FUNCTION__ ,##arg)




#define GPIO_WR32(addr, data)   DRV_WriteReg32(addr,data)
#define GPIO_RD32(addr)         DRV_Reg32(addr)

#define ADDR_BIT 0
#define VAL_BIT  1
#define MASK_BIT 2
/*----------------------------------------------------------------------------*/
#if (defined(MACH_FPGA) && !defined (FPGA_SIMULATION))

void mt_gpio_set_default(void)
{
	return;
}

void mt_gpio_set_default_dump(void)
{
	return;
}

#else

void gpio_dump_reg(void)
{
	/*
	    u32 idx;
	    u32 val;

	    GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);

	    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++) {
	        GPIOVER("idx=[%d],mode_reg(%x)=%x \n",idx,&pReg->mode[idx],GPIO_RD32(&pReg->mode[idx]));
	        //GPIOVER("mode[%d](rst%x,set%x) \n",idx,&pReg->mode[idx].rst,&pReg->mode[idx].set);
	    }
	    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++){
	        GPIOVER("dir[%d]=%x \n",idx,&pReg->dir[idx]);
	        //GPIOVER("dir[%d](rst%x,set%x) \n",idx,&pReg->dir[idx].rst,&pReg->dir[idx].set);
	    }
	    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++) {
	        GPIOVER("dout[%d]=%x \n",idx,&pReg->dout[idx]);
	        //GPIOVER("dout[%d](rst%x,set%x) \n",idx,&pReg->dout[idx].rst,&pReg->dout[idx].set);
	    }
	    for (idx = 0; idx < sizeof(pReg->din)/sizeof(pReg->din[0]); idx++) {
	        GPIOVER("din[%d]=%x \n",idx,&pReg->din[idx]);
	        //GPIOVER("din[%d](rst%x,set%x) \n",idx,&pReg->din[idx].rst,&pReg->din[idx].set);
	    }
	    */
}
void gpio_dump(void)
{
	int i=0;
	int idx=0;
	u32 val=0;
	GPIOVER("fwq .... gpio dct config ++++++++++++++++++++++++++++\n");
	/*
	    for (idx = 0; idx < sizeof(gpio_init_dir_data)/(sizeof(UINT32)); idx++){
	        val = gpio_init_dir_data[idx];
	        GPIOVER("gpio_init_dir_reg[%d],[0x%x]\n",idx,GPIO_RD32(GPIO_BASE+16*idx));

	    }
	*/
	for (i=0; i<MAX_GPIO_PIN; i++) {
		//GPIOVER(" \n");
		dprintf(INFO, "g[%d]\n",i);
		dprintf(INFO, "g[%d], mode(%x)\n",i,mt_get_gpio_mode(0x80000000+i));
		dprintf(INFO, "g[%d], dir(%x)\n",i,mt_get_gpio_dir(0x80000000+i));
		dprintf(INFO, "g[%d], pull_en(%x)\n",i,mt_get_gpio_pull_enable(0x80000000+i));
		dprintf(INFO, "g[%d], pull_sel(%x)\n",i,mt_get_gpio_pull_select(0x80000000+i));
		dprintf(INFO, "g[%d], out(%x)\n",i,mt_get_gpio_out(0x80000000+i));
		dprintf(INFO, "g[%d], smt(%x)\n",i,mt_get_gpio_smt(0x80000000+i));
		// GPIOVER("gpio[%d], ies(%x)\n",i,mt_get_gpio_ies(0x80000000+i));
		// GPIOVER("gpio[%d], in(%x)\n",i,mt_get_gpio_in(0x80000000+i));


	}

	GPIOVER("fwq .... gpio dct config ----------------------------\n");
}

void mt_gpio_set_default_dump(void)
{
	gpio_dump();
}


#include <platform/boot_mode.h>
#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>
#include <dev/uart.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <arch/ops.h>
#include <target/board.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_disp_drv.h>
#include <platform/disp_drv.h>
#include <platform/boot_mode.h>
#include <platform/mt_logo.h>
#include <platform/partition.h>
#include <platform/env.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_i2c.h>
#include <platform/mtk_key.h>
#include <platform/mt_rtc.h>
#include <platform/mt_leds.h>
#include <platform/upmu_common.h>
#include <platform/mtk_wdt.h>
#include <platform/disp_drv_platform.h>

const UINT32 gpio_init_value[][3] = {
	{
		MIPI_RX_ANA_BASE+0x040
		,((((GPIO1_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO0_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO3_MODE==GPIO_MODE_01)?0x0:0x1)) << 26) | ((((GPIO2_MODE==GPIO_MODE_01)?0x0:0x1)) << 16) | ((((GPIO2_MODE==GPIO_MODE_01)?0x0:0x1)) << 18) | ((((GPIO1_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO3_MODE==GPIO_MODE_01)?0x0:0x1)) << 24) | ((((GPIO0_MODE==GPIO_MODE_01)?0x0:0x1)) << 0)
		,(0x1 << 10) | (0x1 << 2) | (0x1 << 26) | (0x1 << 16) | (0x1 << 18) | (0x1 << 8) | (0x1 << 24) | (0x1 << 0)
	},
	{
		MIPI_RX_ANA_BASE+0x840
		,((((GPIO23_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO24_MODE==GPIO_MODE_01)?0x0:0x1)) << 18) | ((((GPIO22_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO23_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO22_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO25_MODE==GPIO_MODE_01)?0x0:0x1)) << 24) | ((((GPIO24_MODE==GPIO_MODE_01)?0x0:0x1)) << 16) | ((((GPIO25_MODE==GPIO_MODE_01)?0x0:0x1)) << 26)
		,(0x1 << 10) | (0x1 << 18) | (0x1 << 0) | (0x1 << 8) | (0x1 << 2) | (0x1 << 24) | (0x1 << 16) | (0x1 << 26)
	},
	{
		MIPI_RX_ANA_BASE+0x640
		,((((GPIO19_MODE==GPIO_MODE_01)?0x0:0x1)) << 24) | ((((GPIO16_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO18_MODE==GPIO_MODE_01)?0x0:0x1)) << 18) | ((((GPIO18_MODE==GPIO_MODE_01)?0x0:0x1)) << 16) | ((((GPIO19_MODE==GPIO_MODE_01)?0x0:0x1)) << 26) | ((((GPIO17_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO17_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO16_MODE==GPIO_MODE_01)?0x0:0x1)) << 2)
		,(0x1 << 24) | (0x1 << 0) | (0x1 << 18) | (0x1 << 16) | (0x1 << 26) | (0x1 << 8) | (0x1 << 10) | (0x1 << 2)
	},
	{
		MIPI_RX_ANA_BASE+0x440
		,((((GPIO11_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO13_MODE==GPIO_MODE_01)?0x0:0x1)) << 26) | ((((GPIO13_MODE==GPIO_MODE_01)?0x0:0x1)) << 24) | ((((GPIO10_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO11_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO10_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO12_MODE==GPIO_MODE_01)?0x0:0x1)) << 16) | ((((GPIO12_MODE==GPIO_MODE_01)?0x0:0x1)) << 18)
		,(0x1 << 10) | (0x1 << 26) | (0x1 << 24) | (0x1 << 2) | (0x1 << 8) | (0x1 << 0) | (0x1 << 16) | (0x1 << 18)
	},
	{
		MIPI_TX1_BASE+0x070
		,((((GPIO176_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO177_MODE==GPIO_MODE_01)?0x0:0x1)) << 9) | ((((GPIO175_MODE==GPIO_MODE_01)?0x0:0x1)) << 7) | ((((GPIO170_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO173_MODE==GPIO_MODE_01)?0x0:0x1)) << 5) | ((((GPIO168_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO171_MODE==GPIO_MODE_01)?0x0:0x1)) << 3) | ((((GPIO174_MODE==GPIO_MODE_01)?0x0:0x1)) << 6) | ((((GPIO169_MODE==GPIO_MODE_01)?0x0:0x1)) << 1) | ((((GPIO172_MODE==GPIO_MODE_01)?0x0:0x1)) << 4)
		,(0x1 << 8) | (0x1 << 9) | (0x1 << 7) | (0x1 << 2) | (0x1 << 5) | (0x1 << 0) | (0x1 << 3) | (0x1 << 6) | (0x1 << 1) | (0x1 << 4)
	},
	{
		IOCFG_B_BASE+0x00D0
		,((0x3 | 0x3) << 20) | ((0x3 | 0x3) << 28) | ((0x3) << 12) | ((0x3) << 0) | ((0x3) << 4) | ((0x3) << 24) | ((0x3) << 8) | ((0x3) << 16)
		,(0xF << 20) | (0xF << 28) | (0xF << 12) | (0xF << 0) | (0xF << 4) | (0xF << 24) | (0xF << 8) | (0xF << 16)
	},
	{
		MIPI_RX_ANA_BASE+0x844
		,((((GPIO27_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO26_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO27_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO26_MODE==GPIO_MODE_01)?0x0:0x1)) << 2)
		,(0x1 << 10) | (0x1 << 0) | (0x1 << 8) | (0x1 << 2)
	},
	{
		MIPI_RX_ANA_BASE+0x644
		,((((GPIO21_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO20_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO20_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO21_MODE==GPIO_MODE_01)?0x0:0x1)) << 10)
		,(0x1 << 8) | (0x1 << 2) | (0x1 << 0) | (0x1 << 10)
	},
	{
		MIPI_RX_ANA_BASE+0x240
		,((((GPIO6_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO8_MODE==GPIO_MODE_01)?0x0:0x1)) << 18) | ((((GPIO7_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO9_MODE==GPIO_MODE_01)?0x0:0x1)) << 26) | ((((GPIO9_MODE==GPIO_MODE_01)?0x0:0x1)) << 24) | ((((GPIO7_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO8_MODE==GPIO_MODE_01)?0x0:0x1)) << 16) | ((((GPIO6_MODE==GPIO_MODE_01)?0x0:0x1)) << 0)
		,(0x1 << 2) | (0x1 << 18) | (0x1 << 8) | (0x1 << 26) | (0x1 << 24) | (0x1 << 10) | (0x1 << 16) | (0x1 << 0)
	},
	{
		MIPI_TX0_BASE+0x070
		,((((GPIO161_MODE==GPIO_MODE_01)?0x0:0x1)) << 3) | ((((GPIO164_MODE==GPIO_MODE_01)?0x0:0x1)) << 6) | ((((GPIO163_MODE==GPIO_MODE_01)?0x0:0x1)) << 5) | ((((GPIO166_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO165_MODE==GPIO_MODE_01)?0x0:0x1)) << 7) | ((((GPIO160_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO158_MODE==GPIO_MODE_01)?0x0:0x1)) << 0) | ((((GPIO167_MODE==GPIO_MODE_01)?0x0:0x1)) << 9) | ((((GPIO159_MODE==GPIO_MODE_01)?0x0:0x1)) << 1) | ((((GPIO162_MODE==GPIO_MODE_01)?0x0:0x1)) << 4)
		,(0x1 << 3) | (0x1 << 6) | (0x1 << 5) | (0x1 << 8) | (0x1 << 7) | (0x1 << 2) | (0x1 << 0) | (0x1 << 9) | (0x1 << 1) | (0x1 << 4)
	},
	{
		MIPI_RX_ANA_BASE+0x044
		,((((GPIO5_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO5_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO4_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO4_MODE==GPIO_MODE_01)?0x0:0x1)) << 0)
		,(0x1 << 8) | (0x1 << 10) | (0x1 << 2) | (0x1 << 0)
	},
	{
		IOCFG_B_BASE+0x00C0
		,((0x3) << 24) | ((0x3) << 20) | ((0x3) << 16)
		,(0xF << 24) | (0xF << 20) | (0xF << 16)
	},
	{
		IOCFG_B_BASE+0x01A0
		,((0x1) << 6) | ((0x1) << 0) | ((0x1) << 3) | ((0x1) << 9)
		,(0x7 << 6) | (0x7 << 0) | (0x7 << 3) | (0x7 << 9)
	},
	{
		IOCFG_B_BASE+0x0040
		,((((GPIO126_MODE==GPIO_MODE_01)?0x3:(GPIO126_MODE==GPIO_MODE_02)?0x3:0x0)) << 7) | ((((GPIO127_MODE==GPIO_MODE_01)?0x3:(GPIO127_MODE==GPIO_MODE_02)?0x3:0x0) | ((GPIO128_MODE==GPIO_MODE_01)?0x3:(GPIO128_MODE==GPIO_MODE_02)?0x3:0x0)) << 9) | ((((GPIO156_MODE==GPIO_MODE_01)?0x3:(GPIO156_MODE==GPIO_MODE_02)?0x3:0x0) | ((GPIO157_MODE==GPIO_MODE_01)?0x3:(GPIO157_MODE==GPIO_MODE_02)?0x3:0x0)) << 15) | ((((GPIO155_MODE==GPIO_MODE_01)?0x3:(GPIO155_MODE==GPIO_MODE_02)?0x3:0x0)) << 13)
		,(0x3 << 7) | (0x3 << 9) | (0x3 << 15) | (0x3 << 13)
	},
	{
		IOCFG_B_BASE+0x01B0
		,((0x4) << 21) | ((0x4) << 27) | ((0x4) << 24)
		,(0x7 << 21) | (0x7 << 27) | (0x7 << 24)
	},
	{
		IOCFG_B_BASE+0x1a0
		,((0x5) << 12) | ((0x5) << 21)
		,(0x1F << 12) | (0x1F << 21)
	},
	{
		MIPI_RX_ANA_BASE+0x444
		,((((GPIO15_MODE==GPIO_MODE_01)?0x0:0x1)) << 8) | ((((GPIO14_MODE==GPIO_MODE_01)?0x0:0x1)) << 2) | ((((GPIO15_MODE==GPIO_MODE_01)?0x0:0x1)) << 10) | ((((GPIO14_MODE==GPIO_MODE_01)?0x0:0x1)) << 0)
		,(0x1 << 8) | (0x1 << 2) | (0x1 << 10) | (0x1 << 0)
	},
	{
		IOCFG_B_BASE+0x1b0
		,((0x5) << 16)
		,(0x1F << 16)
	}
};

void mt_gpio_set_default_chip(void)
{
	u32 idx;
	u32 val;
	//u32 *p;
	u32 mask;
	unsigned int g_fb_base;
	GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);
	unsigned pin = 0;

#ifdef FPGA_SIMULATION

	GPIOVER("fwq .... gpio.................................. 6797 real 0805\n");
	GPIOVER("gpio debug base =%x\n", GPIO_RD32(GPIO_BASE));
	memset(GPIO_BASE,0 ,4096);
#endif

	for (idx = 0; idx < sizeof(gpio_init_value)/((sizeof(UINT32))*(MASK_BIT+1)); idx++) {
		mask = gpio_init_value[idx][MASK_BIT];
		val = GPIO_RD32(gpio_init_value[idx][ADDR_BIT]);
		val &= ~(mask);
		val |= ((gpio_init_value[idx][VAL_BIT])&mask);
		GPIO_WR32(gpio_init_value[idx][ADDR_BIT],val);
	}

	for (pin = 0; pin < MT_GPIO_BASE_MAX; pin++) {
#ifdef FPGA_SIMULATION

		GPIOVER("GPIO %d dump\n",pin);
#endif
		/* set GPIOx_MODE*/
		mt_set_gpio_mode(0x80000000+pin ,gpio_array[pin].mode);

		/* set GPIOx_DIR*/
		mt_set_gpio_dir(0x80000000+pin ,gpio_array[pin].dir);

		/* set GPIOx_PULLEN*/
		mt_set_gpio_pull_enable(0x80000000+pin ,gpio_array[pin].pullen);

		/* set GPIOx_PULL*/
		if (mt_get_gpio_pull_enable(0x80000000+pin) == GPIO_PULL_ENABLE) {
			mt_set_gpio_pull_select(0x80000000+pin ,gpio_array[pin].pull);
		}

		/* set GPIOx_DATAOUT*/
		mt_set_gpio_out(0x80000000+pin ,gpio_array[pin].dataout);

		/* set GPIOx_SMT */
		mt_set_gpio_smt(0x80000000+pin ,gpio_array[pin].smt);

		/* set GPIOx_IES */
		mt_set_gpio_ies(0x80000000+pin ,gpio_array[pin].ies);

	}

	/* workaround for fingerprint low power */
#ifdef GPIO_FP_EINT_PIN
	mt_set_gpio_mode(GPIO_FP_EINT_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_FP_EINT_PIN, GPIO_DIR_IN);
	mt_get_gpio_pull_enable(GPIO_FP_EINT_PIN);
	mt_set_gpio_pull_select(GPIO_FP_EINT_PIN, GPIO_PULL_DOWN);
#endif

#ifdef GPIO_FINGERPRINT_RST_PIN
	mt_set_gpio_mode(GPIO_FINGERPRINT_RST_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_FINGERPRINT_RST_PIN, GPIO_DIR_IN);
	mt_get_gpio_pull_enable(GPIO_FINGERPRINT_RST_PIN);
	mt_set_gpio_pull_select(GPIO_FINGERPRINT_RST_PIN, GPIO_PULL_DOWN);
#endif

	if (g_boot_arg->extbuck_fan53526_exist) {
		mt_set_gpio_out(0x80000000+110, 1);
		mt_set_gpio_out(0x80000000+111, 1);
	}

#ifdef SELF_TEST
	mt_gpio_self_test();
#endif
}

void mt_gpio_set_power(u8 mc1_power,u8 mc2_power, u8 sim1_power, u8 sim2_power)
{

	u32 reg=0;
	if (mc1_power == GPIO_VIO28) {

		reg = GPIO_RD32(IOCFG_B_BASE+0x1b0);
		reg &= ~(0x1f<<16);
		reg |= 0x1f<<16;
		GPIO_WR32(IOCFG_B_BASE+0x0c28, reg);

	} else if (mc1_power == GPIO_VIO18) {
		reg = GPIO_RD32(IOCFG_B_BASE+0x1b0);
		reg &= ~(0x1f<<16);
		GPIO_WR32(IOCFG_B_BASE+0x1b0, reg);
	} else {
		/*VCM*/
	}

	//sim1
	if (sim1_power == GPIO_VIO28) {

		reg = GPIO_RD32(IOCFG_B_BASE+0x1a0);
		reg &= ~(0x3f<<20);
		reg |= 0x3f<<20;
		GPIO_WR32(IOCFG_B_BASE+0x1a0, reg);
	} else if (sim1_power == GPIO_VIO18) {
		reg = GPIO_RD32(IOCFG_B_BASE+0x1a0);
		reg &= ~(0x3f<<20);
		GPIO_WR32(IOCFG_B_BASE+0x1a0, reg);
	} else {
		/*VSIM*/
	}
	//sim2
	if (sim2_power == GPIO_VIO28) {

		reg = GPIO_RD32(IOCFG_B_BASE+0x1a0);
		reg &= ~(0x1f);
		reg |= 0x1f<<12;
		GPIO_WR32(IOCFG_B_BASE+0x1a0, reg);
	} else if (sim1_power == GPIO_VIO18) {
		reg = GPIO_RD32(IOCFG_B_BASE+0x1a0);
		reg &= ~(0x1f);
		GPIO_WR32(IOCFG_B_BASE+0x1a0, reg);
	} else {
		/*VSIM*/
	}

}


void mt_gpio_set_avoid_leakage(void)
{
#if 0
#ifdef MTK_EMMC_SUPPORT
	GPIO_WR32(IO_CFG_B_BASE+0x58, 0x220000);
#endif
#endif
}

void mt_gpio_set_default(void)
{

	mt_gpio_set_default_chip();
	/*special setting for bring up finger print*/
	GPIO_WR32(IOCFG_B_BASE+0x190, 0x3<<16);

	return;
}

#ifdef SELF_TEST
int smt_test (void)
{

	int i, val;
	s32 out;
	int res;
	GPIOVER("smt_test test+++++\n");

	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;
		/*test*/
		for (val = 0; val < GPIO_SMT_MAX; val++) {
			GPIOVER("test gpio[%d],smt[%d]\n", i, val);
			if (-1 == mt_set_gpio_smt(i|0x80000000,val)) {
				GPIOERR(" set smt unsupport\n" );
				continue;
			}
			if ((res = mt_set_gpio_smt(i|0x80000000,val)) != RSUCCESS) {
				GPIOERR(" set smt[%d] fail: %d\n", val, res);
				return -1;
			}
			if ( val != mt_get_gpio_smt(i|0x80000000) ) {
				GPIOERR(" get smt[%d] fail: real get %d\n", val, mt_get_gpio_smt(i|0x80000000));
				return -1;
			}
			if (mt_get_gpio_smt(i|0x80000000) > 1) {
				GPIOERR(" get smt[%d] value fail: real get %d\n", val, mt_get_gpio_smt(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOVER("smt_test test----- PASS!\n");
	return 0;

}


int output_test (void)
{

	int i, val;
	s32 out;
	int res;
	GPIOVER("output test+++++\n");

	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;
		res = mt_set_gpio_dir(i|0x80000000,GPIO_DIR_OUT);
		if (res)
			return -1;
		/*test*/
		for (val = 0; val < GPIO_OUT_MAX; val++) {
			GPIOVER("test gpio[%d],output[%d]\n", i, val);
			if ((res = mt_set_gpio_out(i|0x80000000,val)) != RSUCCESS) {
				GPIOERR(" set out[%d] fail: %d\n", val, res);
				return -1;
			}
			if ( val != mt_get_gpio_out(i|0x80000000) ) {
				GPIOERR(" get out[%d] fail: real get %d\n", val, mt_get_gpio_out(i|0x80000000));
				return -1;
			}
			if (mt_get_gpio_out(i|0x80000000) > 1) {
				GPIOERR(" get out[%d] value fail: real get %d\n", val, mt_get_gpio_out(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOVER("output test----- PASS!\n");
	return 0;

}

int direction_test(void)
{
	int i, val;
	s32 out;
	int res;
	GPIOVER("direction_test test+++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;

		/*test*/
		for (val = 0; val < GPIO_DIR_MAX; val++) {
			GPIOVER("test gpio[%d],direction[%d]\n", i, val);
			if ((res = mt_set_gpio_dir(i|0x80000000,val)) != RSUCCESS) {
				GPIOERR(" set direction[%d] fail: %d\n", val, res);
				return -1;
			}
			if ( val != mt_get_gpio_dir(i|0x80000000) ) {
				GPIOERR(" get direction[%d] fail1 real get %d\n", val, mt_get_gpio_dir(i|0x80000000));
				return -1;
			}
			if ( mt_get_gpio_dir(i|0x80000000) > 1) {
				GPIOERR(" get direction[%d] value fail2 real get %d\n", val, mt_get_gpio_dir(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOVER("direction_test----- PASS!\n");

	return 0;
}

int mode_test(void)
{
	int i, val;
	s32 out;
	int res;
	GPIOVER("mode_test test+++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {


		/*test*/
		for (val = 0; val < GPIO_MODE_MAX; val++) {
			GPIOVER("test gpio[%d],dir[%d]\n", i, val);
			if ((res = mt_set_gpio_mode(i|0x80000000,val)) != RSUCCESS) {
				GPIOERR(" set mode[%d] fail: %d\n", val, res);
				return -1;
			}
			if ( val != mt_get_gpio_mode(i|0x80000000) ) {
				GPIOERR(" get mode[%d] fail: real get %d\n", val, mt_get_gpio_mode(i|0x80000000));
				return -1;
			}
			if ( mt_get_gpio_mode(i|0x80000000) > 7) {
				GPIOERR(" get mode[%d] value fail: real get %d\n", val, mt_get_gpio_mode(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOVER("mode_test----- PASS!\n");

	return 0;
}


int pullen_test(void)
{
	int i, val;
	s32 out;
	int res;
	GPIOVER("pullen_test  +++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;

		/*test*/
		for (val = 0; val < GPIO_PULL_EN_MAX; val++) {
			GPIOVER("test gpio[%d],pullen[%d]\n", i, val);
			if (-1 == mt_set_gpio_pull_enable(i|0x80000000,val)) {
				GPIOERR(" set pull_enable unsupport\n" );
				continue;
			}
			if (GPIO_NOPULLDOWN == mt_set_gpio_pull_enable(i|0x80000000,val)) {
				GPIOERR(" set pull_down unsupport\n" );
				continue;
			}
			if (GPIO_NOPULLUP== mt_set_gpio_pull_enable(i|0x80000000,val)) {
				GPIOERR(" set pull_up unsupport\n" );
				continue;
			}
			if ((res = mt_set_gpio_pull_enable(i|0x80000000,val)) != RSUCCESS) {
				GPIOERR(" set pull_enable[%d] fail1 %d\n", val, res);
				return -1;
			}
			if ( val != mt_get_gpio_pull_enable(i|0x80000000) ) {
				GPIOERR(" get pull_enable[%d] fail2 real get %d\n", val, mt_get_gpio_pull_enable(i|0x80000000));
				return -1;
			}

			if ( mt_get_gpio_pull_enable(i|0x80000000) > 1) {
				GPIOERR(" get pull_enable[%d] value fail3: real get %d\n", val, mt_get_gpio_pull_enable(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOVER("pullen_test----- PASS!\n");

	return 0;
}

int pullselect_test(void)
{
	int i, val;
	s32 out;
	int res;
	GPIOVER("pullselect_test  +++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0 );
		if (res)
			return -1;

		/*test*/
		for (val = 0; val < GPIO_PULL_MAX; val++) {
			GPIOVER("test gpio[%d],pull_select[%d]\n", i, val);
			res =mt_set_gpio_pull_select(i|0x80000000,val);
			if (GPIO_PULL_UNSUPPORTED == res
			        || GPIO_NOPULLUP == res
			        || GPIO_NOPULLDOWN ==res) {
				GPIOERR(" set gpio[%d] pull_select[%d] unsupport\n",i,val);
				continue;
			}

			if ((res = mt_set_gpio_pull_select(i|0x80000000,val)) != RSUCCESS) {
				GPIOERR(" set pull_select[%d] fail1: %d\n", val, res);
				return -1;
			}
			if ( val != mt_get_gpio_pull_select(i|0x80000000) ) {
				GPIOERR(" get pull_select[%d] fail2: real get %d\n", val, mt_get_gpio_pull_select(i|0x80000000));
				return -1;
			}
			if (-1 == mt_get_gpio_pull_select(i|0x80000000)) {
				GPIOERR(" set gpio[%d] pull_select not support\n",i);
			} else if (mt_get_gpio_pull_select(i|0x80000000) > 2) {
				GPIOERR(" get pull_select[%d] value fail: real get %d\n", val, mt_get_gpio_pull_select(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOVER("pullselect_test----- PASS!\n");

	return 0;
}



void mt_gpio_self_test(void)
{
	int err=0;
	GPIOVER("GPIO self_test start\n");
	err = mode_test();
	if (err) {
		GPIOVER("GPIO self_test FAIL\n");
		return;
	}

	err = direction_test();
	if (err) {
		GPIOVER("GPIO self_test FAIL\n");
		return;
	}

	err = output_test();
	if (err) {
		GPIOVER("GPIO self_test FAIL\n");
		return;
	}

	err = smt_test();
	if (err) {
		GPIOVER("GPIO self_test FAIL\n");
		return;
	}

	err = pullen_test();
	if (err) {
		GPIOVER("GPIO self_test FAIL\n");
		return;
	}

	err = pullselect_test();
	if (err) {
		GPIOVER("GPIO self_test FAIL\n");
		return;
	}

	GPIOVER("GPIO self_test PASS\n");
}

#endif

#endif
