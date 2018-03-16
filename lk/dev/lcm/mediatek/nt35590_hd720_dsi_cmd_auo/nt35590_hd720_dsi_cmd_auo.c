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
#ifndef BUILD_LK
#include <linux/string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH                                         (720)
#define FRAME_HEIGHT                                        (1280)
#define LCM_ID       (0x69)
#define REGFLAG_DELAY                                       0xAB
#define REGFLAG_END_OF_TABLE                                0xAA   // END OF REGISTERS MARKER
#define LCM_ID_NT35590 (0x90)

#define LCM_DSI_CMD_MODE                                    1

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                           (lcm_util.udelay(n))
#define MDELAY(n)                                           (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                       lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

static unsigned int need_set_lcm_addr = 1;
struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy((void*)&lcm_util, (void*)util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset((void*)params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = BURST_VDO_MODE;
#endif
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM                = LCM_THREE_LANE;
	// The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
#ifndef MACH_FPGA
	params->dsi.vertical_sync_active                = 1;// 3    2
	params->dsi.vertical_backporch                  = 1;// 20   1
	params->dsi.vertical_frontporch                 = 2; // 1  12
	params->dsi.vertical_active_line                = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 2;// 50  2
	params->dsi.horizontal_backporch                = 12;
	params->dsi.horizontal_frontporch               = 80;
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.CLK_HS_POST=26;
	params->dsi.compatibility_for_nvk = 0;

	params->dsi.PLL_CLOCK = 330;//dsi clock customization: should config clock value directly
	params->dsi.pll_div1=0;
	params->dsi.pll_div2=0;
	params->dsi.fbk_div=11;
#else
	params->dsi.vertical_sync_active                = 1;// 3    2
	params->dsi.vertical_backporch                  = 1;// 20   1
	params->dsi.vertical_frontporch                 = 2; // 1  12
	params->dsi.vertical_active_line                = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 10;// 50  2
	params->dsi.horizontal_backporch                = 42;
	params->dsi.horizontal_frontporch               = 52;
	params->dsi.horizontal_bllp             = 85;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.compatibility_for_nvk = 0;

	params->dsi.PLL_CLOCK = 26;//dsi clock customization: should config clock value directly
#endif
}

static void lcm_init(void)
{
	unsigned int data_array[16];

	MDELAY(40);
	SET_RESET_PIN(1);
	MDELAY(5);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000826;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000026;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);

	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(40);


	data_array[0]=0x00023902;
#if (LCM_DSI_CMD_MODE)
	data_array[1]=0x000008C2;//cmd mode
#else
	data_array[1]=0x000003C2;//video mode
#endif
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00023902;
	data_array[1]=0x000002BA;//MIPI lane
	dsi_set_cmdq(data_array, 2, 1);

	//{0x44,    2,  {((FRAME_HEIGHT/2)>>8), ((FRAME_HEIGHT/2)&0xFF)}},
	data_array[0] = 0x00033902;
	data_array[1] = (((FRAME_HEIGHT/2)&0xFF) << 16) | (((FRAME_HEIGHT/2)>>8) << 8) | 0x44;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00351500;// TE ON
	dsi_set_cmdq(data_array, 1, 1);
	//MDELAY(10);

	data_array[0]=0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00005012;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00000213;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;////CMD1
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00290500;
	dsi_set_cmdq(data_array, 1, 1);

	need_set_lcm_addr = 1;
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0]=0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);

	data_array[0]=0x00023902;
	data_array[1]=0x0000014F;
	dsi_set_cmdq(data_array, 2, 1);

}


static void lcm_resume(void)
{
	unsigned int data_array[16];

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(50);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000826;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000026;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);

	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(40);

	data_array[0]=0x00023902;
#if (LCM_DSI_CMD_MODE)
	data_array[1]=0x000008C2;//cmd mode
#else
	data_array[1]=0x000003C2;//cmd mode
#endif
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00023902;
	data_array[1]=0x000002BA;//MIPI lane
	dsi_set_cmdq(data_array, 2, 1);

	//{0x44,    2,  {((FRAME_HEIGHT/2)>>8), ((FRAME_HEIGHT/2)&0xFF)}},
	data_array[0] = 0x00033902;
	data_array[1] = (((FRAME_HEIGHT/2)&0xFF) << 16) | (((FRAME_HEIGHT/2)>>8) << 8) | 0x44;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00351500;// TE ON
	dsi_set_cmdq(data_array, 1, 1);
	//MDELAY(10);

	data_array[0]=0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);


	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00005012;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00000213;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;////CMD1
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]=0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	need_set_lcm_addr = 1;

}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	// need update at the first time
	if (need_set_lcm_addr) {
		data_array[0]= 0x00053902;
		data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
		data_array[2]= (x1_LSB);
		dsi_set_cmdq(data_array, 3, 1);

		data_array[0]= 0x00053902;
		data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
		data_array[2]= (y1_LSB);
		dsi_set_cmdq(data_array, 3, 1);

		need_set_lcm_addr = 0;
	}

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}


static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0]; //we only need ID

	dprintf(INFO, "%s, LK nt35590 debug: nt35590 id = 0x%08x\n", __func__, id);

	if (id == LCM_ID_NT35590)
		return 1;
	else
		return 0;

}


static unsigned int lcm_esd_check(void)
{
	return FALSE;
}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	lcm_resume();

	return TRUE;
}

unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 0;
}


// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35590_hd720_dsi_cmd_auo_lcm_drv = {
	.name           = "nt35590_AUO",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.ata_check      = lcm_ata_check,
	.esd_check      = lcm_esd_check,
	.esd_recover    = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};
