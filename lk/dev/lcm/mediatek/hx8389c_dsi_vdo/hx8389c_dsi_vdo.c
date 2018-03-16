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
#include "lcm_drv.h"
#include <debug.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH             (540)
#define FRAME_HEIGHT            (960)

#define REGFLAG_DELAY           0xFE
#define REGFLAG_END_OF_TABLE    0xFF    /* END OF REGISTERS MARKER */

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)  

static struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	/*
	   Note :

	   Data ID will depends on the following rule.

	   count of parameters > 1      => Data ID = 0x39
	   count of parameters = 1      => Data ID = 0x15
	   count of parameters = 0      => Data ID = 0x05

	   Structure Format :

	   {DCS command, count of parameters, {parameter list}}
	   {REGFLAG_DELAY, milliseconds of time, {}},

	   ...

	   Setting ending by predefined flag

	   {REGFLAG_END_OF_TABLE, 0x00, {}}
	 */

	/* SET PASSWORD//10//F2//58 */
	{ 0xB9, 03, {0xFF, 0x83, 0x89} },

	/* SET POWER HI */
	{ 0xB1, 20, {0x7F, 0x10, 0x10, 0x32, 0x32, 0x50, 0x10, 0xF2, 0x58, 0x80,
		     0x20, 0x20, 0xF8, 0xAA, 0xAA, 0xA0, 0x00, 0x80, 0x30, 0x00} },

	{ 0xB2, 10, {0x80, 0x50, 0x05, 0x07, 0x40, 0x38, 0x11, 0x64, 0x5D, 0x09} },

	/* SET CYC HI */
	{ 0xB4, 11, {0x70, 0x70, 0x70, 0x70, 0x00, 0x00, 0x10, 0x76, 0x10, 0x76,
		     0xB0} },

	/* SET GAMMA */
	{ 0xD3, 35, {0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x32, 0x10, 0x00,
		     0x00, 0x00, 0x03, 0xC6, 0x03, 0xC6, 0x00, 0x00, 0x00, 0x00,
		     0x35, 0x33, 0x04, 0x04, 0x37, 0x00, 0x00, 0x00, 0x05, 0x08,
		     0x00, 0x00, 0x0A, 0x00, 0x01} },

	{ 0xD5, 38, {0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18, 0x20, 0x21,
		     0x24, 0x25, 0x18, 0x18, 0x18, 0x18, 0x00, 0x01, 0x04, 0x05,
		     0x02, 0x03, 0x06, 0x07, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		     0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18} },

	{ 0xD6, 38, {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x25, 0x24,
		     0x21, 0x20, 0x18, 0x18, 0x18, 0x18, 0x07, 0x06, 0x03, 0x02,
		     0x05, 0x04, 0x01, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		     0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18} },

	{ 0xCC, 01, {0x02} },

	{ 0xC7, 04, {0x00, 0x80, 0x00, 0xC0} },

	{ 0xD2, 01, {0x33} },

	{ 0xE0, 42, {0x00, 0x0F, 0x16, 0x35, 0x3B, 0x3F, 0x21, 0x43, 0x07, 0x0B,
		     0x0D, 0x18, 0x0E, 0x10, 0x12, 0x11, 0x13, 0x06, 0x10, 0x13,
		     0x18, 0x00, 0x0F, 0x15, 0x35, 0x3B, 0x3F, 0x21, 0x42, 0x07,
		     0x0B, 0x0D, 0x18, 0x0D, 0x11, 0x13, 0x11, 0x12, 0x07, 0x11,
		     0x12, 0x17} },

	{ 0xB6, 03, {0x88, 0x88, 0x00} },

	{ 0x35, 01, {0x00} },

	{ 0x11, 00, {} },

	/* DISPLAY ON */
	{ 0x29, 00, {} },

	/* stop loop */
	{ REGFLAG_END_OF_TABLE, 0, {0x00} }
};


static struct LCM_setting_table lcm_set_window[] = {
	{ 0x2A, 4, {0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)} },
	{ 0x2B, 4, {0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};


static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{ 0x10, 0, {} },
	{ REGFLAG_DELAY, 120, {} },

	// Display ON
	{ 0x29, 0, {} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Sleep Mode On
	{ 0x10, 0, {} },
	{ REGFLAG_DELAY, 120, {} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{ 0x51, 1, {0xFF} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;

		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 5;
	params->dsi.vertical_frontporch = 9;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 40;
	params->dsi.horizontal_backporch = 41;
	params->dsi.horizontal_frontporch = 60;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

#ifndef CONFIG_FPGA_EARLY_PORTING
	params->dsi.PLL_CLOCK = 230;	/* this value must be in MTK suggested table */
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif

	params->dsi.cont_clock = 1;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;
}


static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[3];
	char id0 = 0;
	char id1 = 0;
	char id2 = 0;

	SET_RESET_PIN(0);
	MDELAY(200);
	SET_RESET_PIN(1);
	MDELAY(200);

	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA, buffer, 1);


	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDB, buffer + 1, 1);


	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDC, buffer + 2, 1);

	id0 = buffer[0];	/* should be 0x00 */
	id1 = buffer[1];	/* should be 0xaa */
	id2 = buffer[2];	/* should be 0x55 */
#ifdef BUILD_LK
	dprintf(0, "%s, id0 = 0x%08x\n", __func__, id0);	/* should be 0x00 */
	dprintf(0, "%s, id1 = 0x%08x\n", __func__, id1);	/* should be 0xaa */
	dprintf(0, "%s, id2 = 0x%08x\n", __func__, id2);	/* should be 0x55 */
#endif
	if ((id0 == 0x83) && (id1 == 0x89) && (id2 == 0x0C)) {
		dprintf(0, "[LK] Compare HX8389C LCM ID Success: 0x%x, 0x%x, 0x%x\n", id0, id1, id2);
		return 1;
	} else {
		dprintf(0, "[LK] Compare HX8389C LCM ID Failed: 0x%x, 0x%x, 0x%x\n", id0, id1, id2);
		return 0;
	}
}


static void lcm_init(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);

	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting,
		   sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
	lcm_init();
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

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(&data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if (level > 255)
		level = 255;

	if (level >0)
		mapped_level = default_level+(level)*(255-default_level)/(255);
	else
		mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_setpwm(unsigned int divider)
{
	// TBD
}


static unsigned int lcm_getpwm(unsigned int divider)
{
	// ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
	// pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
	unsigned int pwm_clk = 23706 / (1<<divider);
	return pwm_clk;
}
LCM_DRIVER hx8389c_dsi_vdo_lcm_drv = {

	.name = "hx8389c_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.compare_id = lcm_compare_id,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
