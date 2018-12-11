/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"


#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

#include <cust_gpio_usage.h>

#ifndef MACH_FPGA
#include <cust_i2c.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static LCM_UTIL_FUNCS lcm_util;


#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define LCM_ID_NT36672 (0x8070)

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

/*****************************************************************************
 * Define
 *****************************************************************************/
#ifndef MACH_FPGA

#define LP_I2C_BUSNUM  1    /* for I2C channel 1 */
#define I2C_ID_NAME "lp3101"
#define LP_ADDR 0x3E
/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static struct i2c_board_info lp3101_board_info __initdata = {I2C_BOARD_INFO(I2C_ID_NAME, LP_ADDR)};

static struct i2c_client *lp3101_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int lp3101_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lp3101_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/
struct lp3101_dev	{	
	struct i2c_client *client;

};

static const struct i2c_device_id lp3101_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

/* #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)) */
/* static struct i2c_client_address_data addr_data = { .forces = forces,}; */
/* #endif */
static struct i2c_driver lp3101_iic_driver = {
	.id_table	= lp3101_id,
	.probe		= lp3101_probe,
	.remove		= lp3101_remove,
	/* .detect               = mt6605_detect, */
	.driver = {
		.owner = THIS_MODULE,
		.name	= "lp3101",
	},
};

/*****************************************************************************
 * Function
 *****************************************************************************/
static int lp3101_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	LCM_LOGI("lp3101_iic_probe\n");
	LCM_LOGI("LP: info==>name=%s addr=0x%x\n",client->name,client->addr);
	lp3101_i2c_client  = client;		
	return 0;
}

static int lp3101_remove(struct i2c_client *client)
{
	LCM_LOGI("lp3101_remove\n");
	lp3101_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

static int lp3101_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = lp3101_i2c_client;
	char write_data[2] = { 0 };
	if (client == NULL) {
		LCM_LOGI("i2c_client = NULL, skip lp3101_write_bytes\n");
		return 0;
	}
	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		LCM_LOGI("lp3101 write data fail !!\n");
	return ret;
}

static int __init lp3101_iic_init(void)
	{
   printf( "lp3101_iic_init\n");
   i2c_register_board_info(LP_I2C_BUSNUM, &lp3101_board_info, 1);
   LCM_LOGI( "lp3101_iic_init2\n");
   i2c_add_driver(&lp3101_iic_driver);
   LCM_LOGI( "lp3101_iic_init success\n");	
	return 0;
}

static void __exit lp3101_iic_exit(void)
{
  LCM_LOGI( "lp3101_iic_exit\n");
  i2c_del_driver(&lp3101_iic_driver);  
}

module_init(lp3101_iic_init);
module_exit(lp3101_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK lp3101 I2C Driver");
#endif
#endif

/* static unsigned char lcd_id_pins_value = 0xFF; */
//static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(2160)

#ifndef MACH_FPGA
#define GPIO_65132_ENP GPIO_LCD_ENP_PIN  //AVDD
#define GPIO_65132_ENN GPIO_LCD_ENN_PIN  //AVEE
#endif

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

//static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,0,{}},
	{REGFLAG_DELAY, 50, {}},
	{0x10,0,{}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};

static struct LCM_setting_table init_setting[] = {
	#if 1
	{0xFF,1,{0x20}},
	{0xFB,1,{0x01}},
			
	{0x01,1,{0x33}},
	{0x06,1,{0x99}},
	{0x07,1,{0x9E}},
	{0x0E,1,{0x30}},
	{0x0F,1,{0x2E}},
	{0x1D,1,{0x33}},
	{0x6D,1,{0x66}},
	{0x68,1,{0x03}},
	{0x69,1,{0x99}},
	{0x89,1,{0x0F}},
	{0x95,1,{0xCD}},
	{0x96,1,{0xCD}},
			
	{0xFF,1,{0x24}},
	{0xFB,1,{0x01}},
			
	{0x00,1,{0x01}},
	{0x01,1,{0x1C}},
	{0x02,1,{0x0B}},
	{0x03,1,{0x0C}},
	{0x04,1,{0x29}},
	{0x05,1,{0x0F}},
	{0x06,1,{0x0F}},
	{0x07,1,{0x03}},
	{0x08,1,{0x05}},
	{0x09,1,{0x22}},
	{0x0A,1,{0x00}},
	{0x0B,1,{0x24}},
	{0x0C,1,{0x13}},
	{0x0D,1,{0x13}},
	{0x0E,1,{0x15}},
	{0x0F,1,{0x15}},
	{0x10,1,{0x17}},
	{0x11,1,{0x17}},
	{0x12,1,{0x01}},
	{0x13,1,{0x1C}},
	{0x14,1,{0x0B}},
	{0x15,1,{0x0C}},
	{0x16,1,{0x29}},
	{0x17,1,{0x0F}},
	{0x18,1,{0x0F}},
	{0x19,1,{0x04}},
	{0x1A,1,{0x06}},
	{0x1B,1,{0x23}},
	{0x1C,1,{0x0F}},
	{0x1D,1,{0x24}},
	{0x1E,1,{0x13}},
	{0x1F,1,{0x13}},
	{0x20,1,{0x15}},
	{0x21,1,{0x15}},
	{0x22,1,{0x17}},
	{0x23,1,{0x17}},
	{0x2F,1,{0x04}},
	{0x30,1,{0x08}},
	{0x31,1,{0x04}},
	{0x32,1,{0x08}},
	{0x33,1,{0x04}},
	{0x34,1,{0x04}},
	{0x35,1,{0x00}},
	{0x37,1,{0x09}},
	{0x38,1,{0x75}},
	{0x39,1,{0x75}},
	{0x3B,1,{0xC0}},
	{0x3F,1,{0x75}},
	{0x60,1,{0x10}},
	{0x61,1,{0x00}},
	{0x68,1,{0xC2}},
	{0x78,1,{0x80}},
	{0x79,1,{0x23}},
	{0x7A,1,{0x10}},
	{0x7B,1,{0x9B}},
	{0x7C,1,{0x80}},
	{0x7D,1,{0x06}},
	{0x7E,1,{0x02}},
	{0x8E,1,{0xF0}},
	{0x92,1,{0x76}},
	{0x93,1,{0x0A}},
	{0x94,1,{0x0A}},
	{0x99,1,{0x33}},
	{0x9B,1,{0xFF}},
	{0x9F,1,{0x00}},
	{0xA3,1,{0x91}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x00}},
	{0xB5,1,{0x04}},
	{0xDC,1,{0x40}},
	{0xDD,1,{0x03}},
	{0xDE,1,{0x01}},
	{0xDF,1,{0x3D}},
	{0xE0,1,{0x3D}},
	{0xE1,1,{0x22}},
	{0xE2,1,{0x24}},
	{0xE3,1,{0x0A}},
	{0xE4,1,{0x0A}},
	{0xE8,1,{0x01}},
	{0xE9,1,{0x10}},
	{0xED,1,{0x40}},
			
	{0xFF,1,{0x25}},
	{0xFB,1,{0x01}},
		
	{0x0A,1,{0x81}},
	{0x0B,1,{0xCD}},
	{0x0C,1,{0x01}},
	{0x17,1,{0x82}},
	{0x21,1,{0x1B}},
	{0x22,1,{0x1B}},
	{0x24,1,{0x76}},
	{0x25,1,{0x76}},
	{0x30,1,{0x2A}},
	{0x31,1,{0x2A}},
	{0x38,1,{0x2A}},
	{0x3F,1,{0x11}}, 
	{0x40,1,{0x3A}},
	{0x4B,1,{0x31}},
	{0x4C,1,{0x3A}},
	{0x58,1,{0x22}},
	{0x59,1,{0x05}},
	{0x5A,1,{0x0A}},
	{0x5B,1,{0x0A}},
	{0x5C,1,{0x25}},
	{0x5D,1,{0x80}},
	{0x5E,1,{0x80}},
	{0x5F,1,{0x28}},
	{0x62,1,{0x3F}},
	{0x63,1,{0x82}},
	{0x65,1,{0x00}},
	{0x66,1,{0xDD}},
	{0x6C,1,{0x6D}},
	{0x71,1,{0x6D}},
	{0x78,1,{0x25}},
	{0xC3,1,{0x00}},
		
	{0xFF,1,{0x26}},
	{0xFB,1,{0x01}},
			
	{0x06,1,{0xC8}},
	{0x12,1,{0x5A}},
	{0x19,1,{0x09}},
	{0x1A,1,{0x84}},
	{0x1C,1,{0xFA}},
	{0x1D,1,{0x09}},
	{0x1E,1,{0x0B}},
	{0x99,1,{0x20}},
			
	{0xFF,1,{0x27}},
	{0xFB,1,{0x01}},
           
	{0x13,1,{0x08}},
	{0x14,1,{0x43}},
	{0x16,1,{0xB8}},
	{0x17,1,{0xB8}},
	{0x7A,1,{0x02}},
            
	{0xFF,1,{0x20}},
	{0xFB,1,{0x01}},

	//page selection cmd end
	//R(+) MCR cmd
	{0xB0,16,{0x00,0xAA,0x00,0xB6,0x00,0xC8,0x00,0xD9,0x00,0xEA,0x00,0xF6,0x01,0x07,0x01,0x11}},
	{0xB1,16,{0x01,0x1C,0x01,0x40,0x01,0x60,0x01,0x90,0x01,0xB6,0x01,0xF1,0x02,0x1B,0x02,0x1D}},
	{0xB2,16,{0x02,0x46,0x02,0x75,0x02,0x93,0x02,0xBE,0x02,0xDC,0x03,0x08,0x03,0x16,0x03,0x24}},
	{0xB3,12,{0x03,0x35,0x03,0x49,0x03,0x62,0x03,0x84,0x03,0xB3,0x03,0xFF}},
	//G(+) MCR cmd
	{0xB4,16,{0x01,0x03,0x01,0x09,0x01,0x15,0x01,0x20,0x01,0x2A,0x01,0x34,0x01,0x3C,0x01,0x45}},
	{0xB5,16,{0x01,0x4D,0x01,0x6A,0x01,0x82,0x01,0xAA,0x01,0xCA,0x01,0xFE,0x02,0x25,0x02,0x26}},
	{0xB6,16,{0x02,0x4D,0x02,0x7A,0x02,0x9A,0x02,0xC3,0x02,0xE1,0x03,0x0E,0x03,0x1C,0x03,0x2A}},
	{0xB7,12,{0x03,0x3A,0x03,0x4E,0x03,0x66,0x03,0x88,0x03,0xB5,0x03,0xFF}},
	//B(+) MCR cmd
	{0xB8,16,{0x00,0x00,0x00,0x34,0x00,0x6C,0x00,0x92,0x00,0xAD,0x00,0xC4,0x00,0xDC,0x00,0xEB}},
	{0xB9,16,{0x00,0xFC,0x01,0x2D,0x01,0x53,0x01,0x8A,0x01,0xB3,0x01,0xF1,0x02,0x1C,0x02,0x1D}},
	{0xBA,16,{0x02,0x46,0x02,0x75,0x02,0x94,0x02,0xBF,0x02,0xDD,0x03,0x0A,0x03,0x16,0x03,0x25}},
	{0xBB,12,{0x03,0x35,0x03,0x49,0x03,0x61,0x03,0x7D,0x03,0xB1,0x03,0xFF}},
	//page selection cmd start
	{0xFF,1,{0x21}},
	{0xFB,1,{0x01}},
	//page selection cmd end
	//R(-) MCR cmd
	{0xB0,16,{0x00,0xAA,0x00,0xB6,0x00,0xC8,0x00,0xD9,0x00,0xEA,0x00,0xF6,0x01,0x07,0x01,0x11}},
	{0xB1,16,{0x01,0x1C,0x01,0x40,0x01,0x60,0x01,0x90,0x01,0xB6,0x01,0xF1,0x02,0x1B,0x02,0x1D}},
	{0xB2,16,{0x02,0x46,0x02,0x75,0x02,0x93,0x02,0xBE,0x02,0xDC,0x03,0x08,0x03,0x16,0x03,0x24}},
	{0xB3,12,{0x03,0x35,0x03,0x49,0x03,0x62,0x03,0x84,0x03,0xB3,0x03,0xFF}},
	//G(-) MCR cmd
	{0xB4,16,{0x01,0x03,0x01,0x09,0x01,0x15,0x01,0x20,0x01,0x2A,0x01,0x34,0x01,0x3C,0x01,0x45}},
	{0xB5,16,{0x01,0x4D,0x01,0x6A,0x01,0x82,0x01,0xAA,0x01,0xCA,0x01,0xFE,0x02,0x25,0x02,0x26}},
	{0xB6,16,{0x02,0x4D,0x02,0x7A,0x02,0x9A,0x02,0xC3,0x02,0xE1,0x03,0x0E,0x03,0x1C,0x03,0x2A}},
	{0xB7,12,{0x03,0x3A,0x03,0x4E,0x03,0x66,0x03,0x88,0x03,0xB5,0x03,0xFF}},
	//B(-) MCR cmd
	{0xB8,16,{0x00,0x00,0x00,0x34,0x00,0x6C,0x00,0x92,0x00,0xAD,0x00,0xC4,0x00,0xDC,0x00,0xEB}},
	{0xB9,16,{0x00,0xFC,0x01,0x2D,0x01,0x53,0x01,0x8A,0x01,0xB3,0x01,0xF1,0x02,0x1C,0x02,0x1D}},
	{0xBA,16,{0x02,0x46,0x02,0x75,0x02,0x94,0x02,0xBF,0x02,0xDD,0x03,0x0A,0x03,0x16,0x03,0x25}},
	{0xBB,12,{0x03,0x35,0x03,0x49,0x03,0x61,0x03,0x7D,0x03,0xB1,0x03,0xFF}},



	{0xFF,1,{0x10}},
	{0xFB,1,{0x01}},

	{0x51,1,{0xFF}},
	{0x53,1,{0x24}},
	{0x55,1,{0x00}},

	{0x36, 1, {0x03}},
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
#else	
	{0xFF,1,{0x20}},
	{0xFB,1,{0x01}},
	{0x01,1,{0x33}},
	{0x06,1,{0x99}},
	{0x07,1,{0x9E}},
	{0x0E,1,{0x30}},
	{0x0F,1,{0x2E}},
	{0x1D,1,{0x33}},
	{0x6D,1,{0x66}},
	{0x68,1,{0x03}},
	{0x69,1,{0x99}},
	{0x89,1,{0x0F}},
	{0x95,1,{0xCD}},
	{0x96,1,{0xCD}},
	{0xFF,1,{0x24}},
	{0xFB,1,{0x01}},
	{0x00,1,{0x01}},
	{0x01,1,{0x1C}},
	{0x02,1,{0x0B}},
	{0x03,1,{0x0C}},
	{0x04,1,{0x29}},
	{0x05,1,{0x0F}},
	{0x06,1,{0x0F}},
	{0x07,1,{0x03}},
	{0x08,1,{0x05}},
	{0x09,1,{0x22}},
	{0x0A,1,{0x00}},
	{0x0B,1,{0x24}},
	{0x0C,1,{0x13}},
	{0x0D,1,{0x13}},
	{0x0E,1,{0x15}},
	{0x0F,1,{0x15}},
	{0x10,1,{0x17}},
	{0x11,1,{0x17}},
	{0x12,1,{0x01}},
	{0x13,1,{0x1C}},
	{0x14,1,{0x0B}},
	{0x15,1,{0x0C}},
	{0x16,1,{0x29}},
	{0x17,1,{0x0F}},
	{0x18,1,{0x0F}},
	{0x19,1,{0x04}},
	{0x1A,1,{0x06}},
	{0x1B,1,{0x23}},
	{0x1C,1,{0x0F}},
	{0x1D,1,{0x24}},
	{0x1E,1,{0x13}},
	{0x1F,1,{0x13}},
	{0x20,1,{0x15}},
	{0x21,1,{0x15}},
	{0x22,1,{0x17}},
	{0x23,1,{0x17}},
	{0x2F,1,{0x04}},
	{0x30,1,{0x08}},
	{0x31,1,{0x04}},
	{0x32,1,{0x08}},
	{0x33,1,{0x04}},
	{0x34,1,{0x04}},
	{0x35,1,{0x00}},
	{0x37,1,{0x09}},
	{0x38,1,{0x75}},
	{0x39,1,{0x75}},
	{0x3B,1,{0xC0}},
	{0x3F,1,{0x75}},
	{0x60,1,{0x10}},
	{0x61,1,{0x00}},
	{0x68,1,{0xC2}},
	{0x78,1,{0x80}},
	{0x79,1,{0x23}},
	{0x7A,1,{0x10}},
	{0x7B,1,{0x9B}},
	{0x7C,1,{0x80}},
	{0x7D,1,{0x06}},
	{0x7E,1,{0x02}},
	{0x8E,1,{0xF0}},
	{0x92,1,{0x76}},
	{0x93,1,{0x0A}},
	{0x94,1,{0x0A}},
	{0x99,1,{0x33}},
	{0x9B,1,{0xFF}},
	{0x9F,1,{0x00}},
	{0xA3,1,{0x91}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x00}},
	{0xB5,1,{0x04}},
	{0xDC,1,{0x40}},
	{0xDD,1,{0x03}},
	{0xDE,1,{0x01}},
	{0xDF,1,{0x3D}},
	{0xE0,1,{0x3D}},
	{0xE1,1,{0x22}},
	{0xE2,1,{0x24}},
	{0xE3,1,{0x0A}},
	{0xE4,1,{0x0A}},
	{0xE8,1,{0x01}},
	{0xE9,1,{0x10}},
	{0xED,1,{0x40}},
	{0xFF,1,{0x25}},
	{0xFB,1,{0x01}},
	{0x0A,1,{0x81}},
	{0x0B,1,{0xCD}},
	{0x0C,1,{0x01}},
	{0x17,1,{0x82}},
	{0x21,1,{0x1B}},
	{0x22,1,{0x1B}},
	{0x24,1,{0x76}},
	{0x25,1,{0x76}},
	{0x30,1,{0x2A}},
	{0x31,1,{0x2A}},
	{0x38,1,{0x2A}},
	{0x3F,1,{0x11}}, 
	{0x40,1,{0x3A}},
	{0x4B,1,{0x31}},
	{0x4C,1,{0x3A}},
	{0x58,1,{0x22}},
	{0x59,1,{0x05}},
	{0x5A,1,{0x0A}},
	{0x5B,1,{0x0A}},
	{0x5C,1,{0x25}},
	{0x5D,1,{0x80}},
	{0x5E,1,{0x80}},
	{0x5F,1,{0x28}},
	{0x62,1,{0x3F}},
	{0x63,1,{0x82}},
	{0x65,1,{0x00}},
	{0x66,1,{0xDD}},
	{0x6C,1,{0x6D}},
	{0x71,1,{0x6D}},
	{0x78,1,{0x25}},
	{0xC3,1,{0x00}},
	{0xFF,1,{0x26}},
	{0xFB,1,{0x01}},
	{0x06,1,{0xC8}},
	{0x12,1,{0x5A}},
	{0x19,1,{0x09}},
	{0x1A,1,{0x84}},
	{0x1C,1,{0xFA}},
	{0x1D,1,{0x09}},
	{0x1E,1,{0x0B}},
	{0x99,1,{0x20}},
	{0xFF,1,{0x27}},
	{0xFB,1,{0x01}},
	{0x13,1,{0x08}},
	{0x14,1,{0x43}},
	{0x16,1,{0xB8}},
	{0x17,1,{0xB8}},
	{0x7A,1,{0x02}},
	{0xFF,1,{0x10}},
	{0xFB,1,{0x01}},
	{0x51,1,{0xFF}},
	{0x53,1,{0x24}},
	{0x55,1,{0x00}},
	
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 200, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	#endif
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    
    params->type   = LCM_TYPE_DSI;
    
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    
#if (LCM_DSI_CMD_MODE)
    	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;//BURST_VDO_MODE;
#endif
	params->dsi.switch_mode_enable = 0;
    
    // DSI
    /* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE; 
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format	  	= LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 3;
	params->dsi.vertical_backporch = 15;
	params->dsi.vertical_frontporch = 10;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 42;
	params->dsi.horizontal_frontporch = 42;
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
#ifndef MACH_FPGA
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 423;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 440;	/* this value must be in MTK suggested table */
#endif
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.CLK_HS_POST = 36;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

}

#ifdef BUILD_LK
#ifndef MACH_FPGA
#define lp3101_SLAVE_ADDR_WRITE  0x7C  
static struct mt_i2c_t lp3101_i2c;

static int lp3101_write_byte(kal_uint8 addr, kal_uint8 value)
    {
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;
		
	write_data[0] = addr;
	write_data[1] = value;
		
  lp3101_i2c.id = 1; /* I2C1; */
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
  lp3101_i2c.addr = (lp3101_SLAVE_ADDR_WRITE >> 1);
  lp3101_i2c.mode = ST_MODE;
  lp3101_i2c.speed = 100;
	len = 2;
			
  ret_code = i2c_write(&lp3101_i2c, write_data, len);
	/* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */
				
	return ret_code;
}
				
#else

/* extern int mt8193_i2c_write(u16 addr, u32 data); */
/* extern int mt8193_i2c_read(u16 addr, u32 *data); */

/* #define TPS65132_write_byte(add, data)  mt8193_i2c_write(add, data) */
/* #define TPS65132_read_byte(add)  mt8193_i2c_read(add) */

#endif
#endif

static void lcm_init_power(void)
{
/*#ifndef MACH_FPGA
#ifdef BUILD_LK
	mt6325_upmu_set_rg_vgp1_en(1);
#else
	LCM_LOGI("%s, begin\n", __func__);
	hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
	LCM_LOGI("%s, end\n", __func__);
#endif
#endif*/
}

static void lcm_suspend_power(void)
{
/*#ifndef MACH_FPGA
#ifdef BUILD_LK
	mt6325_upmu_set_rg_vgp1_en(0);
#else
	LCM_LOGI("%s, begin\n", __func__);
	hwPowerDown(MT6325_POWER_LDO_VGP1, "LCM_DRV");
	LCM_LOGI("%s, end\n", __func__);
#endif
#endif*/
}

static void lcm_resume_power(void)
{
/*#ifndef MACH_FPGA
#ifdef BUILD_LK
	mt6325_upmu_set_rg_vgp1_en(1);
#else
	LCM_LOGI("%s, begin\n", __func__);
	hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
	LCM_LOGI("%s, end\n", __func__);
#endif
#endif*/
}

static void lcm_poweron(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;
	cmd = 0x00;
	data = 0x0f;

	SET_RESET_PIN(0);
	MDELAY(50);
#ifndef MACH_FPGA
	mt_set_gpio_mode(GPIO_65132_ENP, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENP, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENP, GPIO_OUT_ONE);
	mt_set_gpio_mode(GPIO_65132_ENN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENN, GPIO_OUT_ONE);
	MDELAY(20);
#ifdef BUILD_LK
	ret = lp3101_write_byte(cmd, data);
#else
	ret = lp3101_write_bytes(cmd,data);
#endif

	if (ret < 0)
		LCM_LOGI("NT36672--------cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("NT36672--------cmd=%0x--i2c write success----\n", cmd);
	MDELAY(20);

	cmd = 0x01;
	data = 0x0f;

#ifdef BUILD_LK
	ret = lp3101_write_byte(cmd,data); 		
#else
	ret = lp3101_write_bytes(cmd,data);
#endif

	if (ret < 0)
		LCM_LOGI("NT36672--------cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("NT36672--------cmd=%0x--i2c write success----\n", cmd);

#endif
	SET_RESET_PIN(1);	
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(20);
}

static unsigned int lcm_compare_id(void)
{

	unsigned char buffer[2];
	unsigned int array[16];
	unsigned int id=0;
	unsigned int id1=0;
	unsigned int lcm_id=0;   
		
	array[0]=0x00043902;
	array[1]=0x9983FFB9;// page enable
	dsi_set_cmdq(array, 2, 1);
	MDELAY(10);

	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xDB, buffer, 1);
	id = buffer[0];
	read_reg_v2(0xF4, buffer, 1);
	id1 = buffer[0];
	lcm_id = (id<<8)|id1;
	
	printf("NT36672 id = 0x%x, id1=0x%x\n", id, id1);
	if(LCM_ID_NT36672 == lcm_id){
		return 1;
	}else{
		return 0;
	}
}

static void lcm_init(void)
{
	lcm_poweron();
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
#ifndef MACH_FPGA
	mt_set_gpio_mode(GPIO_65132_ENN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENN, GPIO_OUT_ZERO);
	mt_set_gpio_mode(GPIO_65132_ENP, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENP, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENP, GPIO_OUT_ZERO);
#endif
	//SET_RESET_PIN(0);
	//MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

/* return TRUE: need recovery */
/* return FALSE: No need recovery */

LCM_DRIVER aeon_nt36672_fhd_dsi_vdo_x600_xinli_lcm_drv = 
{
    .name			= "aeon_nt36672_fhd_dsi_vdo_x600_xinli",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	//.esd_check 	= lcm_esd_check,
//	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	//.ata_check = lcm_ata_check,
//	.update = lcm_update,
//	.switch_mode = lcm_switch_mode,
};
