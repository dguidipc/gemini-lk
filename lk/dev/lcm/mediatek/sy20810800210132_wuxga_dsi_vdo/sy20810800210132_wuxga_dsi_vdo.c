#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#include <linux/xlog.h>
#include <mach/mt_pm_ldo.h>
#endif

#define FRAME_WIDTH                                         (1200)
#define FRAME_HEIGHT                                        (1920)

#define REGFLAG_DELAY                                       0xFE
#define REGFLAG_END_OF_TABLE                                0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

#define LCM_ID                                  0x06
//#define GPIO_LCD_RST_EN      GPIO131

#ifdef GPIO_LCM_PWR_EN
#define GPIO_LCD_PWR_EN      GPIO_LCM_PWR_EN
#else
#define GPIO_LCD_PWR_EN      0xFFFFFFFF
#endif

#ifndef BUILD_LK
static bool fgisFirst = TRUE;
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


static void lcm_init_register(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK \n", __func__);
#else
	printk("%s, kernel", __func__);
#endif
	//DSI_Continuous_HS();
	//DSI_clk_HS_mode(1);

	/* 0xF0, 0x5A, 0x5A */
	data_array[0] = 0x00033902;
	data_array[1] = 0x005A5AF0;
	dsi_set_cmdq(data_array, 2, 1);

	/* 0xF1, 0x5A, 0x5A */
	data_array[0] = 0x00033902;
	data_array[1] = 0x005A5AF1;
	dsi_set_cmdq(data_array, 2, 1);

	/* 0xFC, 0xA5, 0xA5 */
	data_array[0] = 0x00033902;
	data_array[1] = 0x00A5A5FC;
	dsi_set_cmdq(data_array, 2, 1);

	/* 0xD0, 0x00, 0x10 */
	data_array[0] = 0x00033902;
	data_array[1] = 0x001000D0;
	dsi_set_cmdq(data_array, 2, 1);

	/* 0x35. TE */
	data_array[0] = 0x00350500;
	dsi_set_cmdq(data_array, 1, 1);

	/*0xC3, 0x40, 0x00, 0x28*/
	data_array[0] = 0x00043902;
	data_array[1] = 0x280040C3;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(20);

	/*0xF6, 0x63, 0x20, 0x86, 0x00, 0x00, 0x10*/
	data_array[0] = 0x00073902;
	data_array[1] = 0x862063F6;
	data_array[2] = 0x00100000;
	dsi_set_cmdq(data_array, 3, 1);

	/* MIPI Video on, no need care about it */
	/* 0x11. Sleep out */
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);

	/*0x36, 0x00*/
	data_array[0] = 0x00361500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);
	/* 0x29. Display on */
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(200);
	/* Backlight turn on, no need to control it */

}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}

static void lcd_poweron(unsigned char enabled)
{
	if (enabled > 0) {
#ifdef BUILD_LK
		printf("[LK/LCM] lcm_resume_power() enter\n");
		lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
		MDELAY(5);
		lcm_Enable_HW(1800);
		MDELAY(20);

#else
		printk("[Kernel/LCM] lcm_resume_power() enter\n");
		lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
		MDELAY(5);
		hwPowerOn(MT6328_POWER_LDO_VCAMA, VOL_1800, "LCM");
		MDELAY(20);
#endif
	} else {
#ifdef BUILD_LK
		printf("[LK/LCM] lcm_suspend_power() enter\n");
		lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
		MDELAY(20);
		lcm_Disable_HW();
#else
		printk("[Kernel/LCM] lcm_suspend_power() enter\n");
		lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
		MDELAY(20);
		if (fgisFirst == TRUE) {
			fgisFirst = FALSE;
			hwPowerOn(MT6328_POWER_LDO_VCAMA, VOL_1800, "LCM");
		}
		hwPowerDown(MT6328_POWER_LDO_VCAMA, "LCM");

#endif
	}
}

static void lcd_reset(unsigned char enabled)
{
	if (enabled > 0) {
		SET_RESET_PIN(1);
	} else {
		SET_RESET_PIN(0);
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

	params->type   = LCM_TYPE_DSI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->dsi.mode   = SYNC_EVENT_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM                = LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.

	params->dsi.word_count=FRAME_WIDTH*3;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active                = 4;
	params->dsi.vertical_backporch                  = 25;
	params->dsi.vertical_frontporch                 = 35;
	params->dsi.vertical_active_line                = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 1;
	params->dsi.horizontal_backporch                = 60;
	params->dsi.horizontal_frontporch               = 80;
	params->dsi.horizontal_active_pixel         = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 519;
	params->dsi.edp_panel =1;
}


static void lcm_init_power(void)
{

	lcd_reset(0);
	/* Power supply(VDD=1.8V) */
	MDELAY(10);
	lcd_poweron(0);
	MDELAY(20);
	lcd_poweron(1);
	MDELAY(5);
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(25);
	lcd_reset(1);
	MDELAY(10);
}

static void lcm_suspend_power(void)
{
	lcd_reset(0);
	MDELAY(10);
	lcd_poweron(0);
	MDELAY(10);
}

static void lcm_resume_power(void)
{
	MDELAY(20);
	lcm_init_power();
}


static void lcm_init(void)
{

	MDELAY(20);
	// lcm_init_register();
	// MDELAY(20);

}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
	data_array[0] = 0x00280500;  //display off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(200);

}

static void lcm_resume(void)
{
	lcm_init();
}


LCM_DRIVER sy20810800210132_wuxga_dsi_vdo_lcm_drv = {
	.name       = "sy20810800210132_wuxga_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.init_power     = lcm_init_power,
	.suspend        = lcm_suspend,
	.suspend_power  = lcm_suspend_power,
	.resume         = lcm_resume,
	.resume_power = lcm_resume_power,
};

