
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

#define FRAME_WIDTH  										(600)
#define FRAME_HEIGHT 										(1024)

#define REGFLAG_DELAY             							0xFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

#define LCM_ID                                  0x06
//#define GPIO_LCD_RST_EN      GPIO131

#ifdef GPIO_LCM_PWR_EN
#define GPIO_LCD_PWR_EN      GPIO_LCM_PWR_EN
#else
#define GPIO_LCD_PWR_EN      0xFFFFFFFF
#endif

#ifdef GPIO_LCM_RST
#define GPIO_LCD_RST      GPIO_LCM_RST
#else
#define GPIO_LCD_RST      0xFFFFFFFF
#endif

#ifndef BUILD_LK
static bool fgisFirst = TRUE;
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)   lcm_set_gpio_output(GPIO_LCD_RST, v) //(lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

     //unsigned int tmp=0x50;
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size) 

       


static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[128];
};

static struct LCM_setting_table lcm_initialization_setting[] = {

    /*
Note :

Data ID will depends on the following rule.

count of parameters > 1	=> Data ID = 0x39
count of parameters = 1	=> Data ID = 0x15
count of parameters = 0	=> Data ID = 0x05

Structure Format :

{DCS command, count of parameters, {parameter list}}
{REGFLAG_DELAY, milliseconds of time, {}},

*/
//This init code for HX9279A driver IC
		{0xB0, 1, {0x00}},
		{0xBA, 1, {0xA7}},
		{0xBD, 1, {0x41}},
		{0xBE, 1, {0x77}},
		{0xBF, 1, {0x1F}},
		{0xC0, 1, {0x0C}},
		{0xC1, 1, {0x78}},
		{0xC2, 1, {0x5C}},
		{0xC3, 1, {0xE8}},
		{0xC4, 1, {0x73}},
		{0xC5, 1, {0x14}},
		{0xC6, 1, {0x02}},
		{0xC7, 1, {0x14}},
		{0xC8, 1, {0x02}},
		{0xC9, 1, {0x06}},
		{0xCA, 1, {0x00}},
		{0xCB, 1, {0x00}},
		
		{0xB0, 1, {0x01}},
		{0xB1, 1, {0x5C}},
		{0xB2, 1, {0x5E}},
		{0xB3, 1, {0x72}},
		{0xB4, 1, {0xBC}},
		{0xB5, 1, {0xE9}},
		{0xB6, 1, {0x5E}},
		{0xB7, 1, {0x93}},
		{0xB8, 1, {0xBD}},
		{0xB9, 1, {0xFC}},
		{0xBA, 1, {0x2A}},
		{0xBB, 1, {0x54}},
		{0xBC, 1, {0x7C}},
		{0xBD, 1, {0xAE}},
		{0xBE, 1, {0xD3}},
		{0xBF, 1, {0xF2}},
		{0xC0, 1, {0x16}},
		{0xC1, 1, {0xBC}},
		{0xC2, 1, {0x00}},
		{0xC3, 1, {0x15}},
		{0xC4, 1, {0x6A}},
		{0xC5, 1, {0xAB}},
		{0xC6, 1, {0x03}},
		{0xC7, 1, {0x5C}},
		{0xC8, 1, {0x5E}},
		{0xC9, 1, {0x72}},
		{0xCA, 1, {0xBC}},
		{0xCB, 1, {0x19}},
		{0xCC, 1, {0x5E}},
		{0xCD, 1, {0x93}},
		{0xCE, 1, {0xBD}},
		{0xCF, 1, {0xFC}},
		{0xD0, 1, {0x2A}},
		{0xD1, 1, {0x54}},
		{0xD2, 1, {0x7C}},
		{0xD3, 1, {0xAE}},
		{0xD4, 1, {0xD3}},
		{0xD5, 1, {0xF2}},
		{0xD6, 1, {0x16}},
		{0xD7, 1, {0xBC}},
		{0xD8, 1, {0x00}},
		{0xD9, 1, {0x15}},
		{0xDA, 1, {0x6A}},
		{0xDB, 1, {0xAB}},
		{0xDC, 1, {0x03}},
		{0xDD, 1, {0x5C}},
		{0xDE, 1, {0x5E}},
		{0xDF, 1, {0x72}},
		{0xE0, 1, {0xBC}},
		{0xE1, 1, {0xE9}},
		{0xE2, 1, {0x5E}},
		{0xE3, 1, {0x93}},
		{0xE4, 1, {0xBD}},
		{0xE5, 1, {0xFC}},
		{0xE6, 1, {0x2A}},
		{0xE7, 1, {0x54}},
		{0xE8, 1, {0x7C}},
		{0xE9, 1, {0xAE}},
		{0xEA, 1, {0xD3}},
		{0xEB, 1, {0xF2}},
		{0xEC, 1, {0x16}},
		{0xED, 1, {0xBC}},
		{0xEE, 1, {0x00}},
		{0xEF, 1, {0x15}},
		{0xF0, 1, {0x6A}},
		{0xF1, 1, {0xAB}},
		{0xF2, 1, {0x03}},
		
		{0xB0, 1, {0x02}},
		{0xB1, 1, {0x5C}},
		{0xB2, 1, {0x5E}},
		{0xB3, 1, {0x72}},
		{0xB4, 1, {0xBC}},
		{0xB5, 1, {0xE9}},
		{0xB6, 1, {0x5E}},
		{0xB7, 1, {0x93}},
		{0xB8, 1, {0xBD}},
		{0xB9, 1, {0xFC}},
		{0xBA, 1, {0x2A}},
		{0xBB, 1, {0x54}},
		{0xBC, 1, {0x7C}},
		{0xBD, 1, {0xAE}},
		{0xBE, 1, {0xD3}},
		{0xBF, 1, {0xF2}},
		{0xC0, 1, {0x16}},
		{0xC1, 1, {0xBC}},
		{0xC2, 1, {0x00}},
		{0xC3, 1, {0x15}},
		{0xC4, 1, {0x6A}},
		{0xC5, 1, {0xAB}},
		{0xC6, 1, {0x03}},
		{0xC7, 1, {0x5C}},
		{0xC8, 1, {0x5E}},
		{0xC9, 1, {0x72}},
		{0xCA, 1, {0xBC}},
		{0xCB, 1, {0xE9}},
		{0xCC, 1, {0x5E}},
		{0xCD, 1, {0x93}},
		{0xCE, 1, {0xBD}},
		{0xCF, 1, {0xFC}},
		{0xD0, 1, {0x2A}},
		{0xD1, 1, {0x54}},
		{0xD2, 1, {0x7C}},
		{0xD3, 1, {0xAE}},
		{0xD4, 1, {0xD3}},
		{0xD5, 1, {0xF2}},
		{0xD6, 1, {0x16}},
		{0xD7, 1, {0xBC}},
		{0xD8, 1, {0x00}},
		{0xD9, 1, {0x15}},
		{0xDA, 1, {0x6A}},
		{0xDB, 1, {0xAB}},
		{0xDC, 1, {0x03}},
		{0xDD, 1, {0x5C}},
		{0xDE, 1, {0x5E}},
		{0xDF, 1, {0x72}},
		{0xE0, 1, {0xBC}},
		{0xE1, 1, {0xE9}},
		{0xE2, 1, {0x5E}},
		{0xE3, 1, {0x93}},
		{0xE4, 1, {0xBD}},
		{0xE5, 1, {0xFC}},
		{0xE6, 1, {0x2A}},
		{0xE7, 1, {0x54}},
		{0xE8, 1, {0x7C}},
		{0xE9, 1, {0xAE}},
		{0xEA, 1, {0xD3}},
		{0xEB, 1, {0xF2}},
		{0xEC, 1, {0x16}},
		{0xED, 1, {0xBC}},
		{0xEE, 1, {0x00}},
		{0xEF, 1, {0x15}},
		{0xF0, 1, {0x6A}},
		{0xF1, 1, {0xAB}},
		{0xF2, 1, {0x03}},
		
		{0xB0, 1, {0x03}},
		{0xC0, 1, {0x49}},
		{0xC1, 1, {0x10}},
		{0xC2, 1, {0x01}},
		{0xC3, 1, {0x28}},
		{0xC4, 1, {0x28}},
		{0xC5, 1, {0x0C}},
		{0xC8, 1, {0x42}},
		{0xC9, 1, {0x40}},
		{0xCA, 1, {0x01}},
		{0xCB, 1, {0x08}},
		{0xCC, 1, {0x02}},
		{0xCD, 1, {0x08}},
		{0xCE, 1, {0x09}},
		{0xD0, 1, {0x03}},
		{0xD1, 1, {0x0A}},
		{0xDC, 1, {0x00}},
		{0xDD, 1, {0x00}},
		{0xDE, 1, {0x00}},
		{0xDF, 1, {0x00}},
		{0xE0, 1, {0x12}},
		{0xE1, 1, {0x11}},
		{0xE2, 1, {0x05}},
		{0xE3, 1, {0x06}},
		{0xE4, 1, {0x07}},
		{0xE5, 1, {0x08}},
		{0xE6, 1, {0x09}},
		{0xE7, 1, {0x0A}},
		{0xE8, 1, {0x0B}},
		{0xE9, 1, {0x0C}},
		{0xEA, 1, {0x02}},
		{0xEB, 1, {0x01}},
		{0xEC, 1, {0x12}},
		{0xED, 1, {0x11}},
		{0xEE, 1, {0x05}},
		{0xEF, 1, {0x06}},
		{0xF0, 1, {0x07}},
		{0xF1, 1, {0x08}},
		{0xF2, 1, {0x09}},
		{0xF3, 1, {0x0A}},
		{0xF4, 1, {0x0B}},
		{0xF5, 1, {0x0C}},
		{0xF6, 1, {0x02}},
		{0xF7, 1, {0x01}},
		{0xF8, 1, {0x00}},
		{0xF9, 1, {0x00}},
		{0xFA, 1, {0x00}},
		{0xFB, 1, {0x00}},
		{0xB0, 1, {0x05}},
		{0xB3, 1, {0x52}},
		{0xB0, 1, {0x06}},
		{0xB8, 1, {0xA5}},
		{0xC0, 1, {0xA5}},
		{0xC5, 1, {0x44}},
		{0xC7, 1, {0x1F}},
		{0xB8, 1, {0x5A}},
		{0xC0, 1, {0x5A}},
		
		{0xB0, 1, {0x03}},
		{0xB2, 1, {0xA5}},
		{0xB3, 1, {0x04}},
		{0xB0, 1, {0x0F}},
		{0x11, 0, {0x00}},
		{0x29, 0, {0x00}},
		{REGFLAG_DELAY, 80, {}},
		{REGFLAG_END_OF_TABLE, 0x00, {}}
};

#if 1
//static int vcom = 0x7A;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;



    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
		/*	  case 0xb6:
			table[i].para_list[0] = vcom;
			table[i].para_list[1] = vcom;
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
			vcom -= 1;
			break;
			*/
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}

#endif
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

    // enable tearing-free
    //params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
    //params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

    //params->dsi.mode   = BURST_VDO_MODE; 
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE; //BURST_VDO_MODE; 

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	params->dsi.packet_size=256;
	//video mode timing
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.word_count=FRAME_WIDTH*3;

    params->dsi.vertical_sync_active				= 2;
    params->dsi.vertical_backporch					= 8;//10;
    params->dsi.vertical_frontporch					= 14;//20;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
    params->dsi.horizontal_sync_active				= 24;
    params->dsi.horizontal_backporch				= 36;//24;
    params->dsi.horizontal_frontporch				= 100;//112;
    params->dsi.horizontal_active_pixel			= FRAME_WIDTH;


    // Video mode setting		
    //params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.pll_select=1;
    params->dsi.PLL_CLOCK = 150;//240;//LCM_DSI_6589_PLL_CLOCK_253_5;//LCM_DSI_6589_PLL_CLOCK_240_5;//LCM_DSI_6589_PLL_CLOCK_227_5;//this value must be in MTK suggested table 227_5
   	params->dsi.ssc_disable = 1;  // disable ssc
    //params->dsi.cont_clock = 1;  // clcok always hs mode
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
   mt_set_gpio_mode(GPIO, GPIO_MODE_00);
   mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
   mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}


static void lcm_init_power(void)
{
#ifdef BUILD_LK 
	printf("[LK/LCM] lcm_init_power() enter\n");

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	lcm_Enable_HW(1800);
	MDELAY(1);	
#else
	printk("[Kernel/LCM] lcm_init_power() enter\n");
	
#endif

}

static void lcm_suspend_power(void)
{
#ifdef BUILD_LK 
	printf("[LK/LCM] lcm_suspend_power() enter\n");
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(20);
	lcm_Disable_HW();		
#else
	printk("[Kernel/LCM] lcm_suspend_power() enter\n");
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(20);
	if(fgisFirst == TRUE) {
    fgisFirst = FALSE;
    hwPowerOn(MT6328_POWER_LDO_VCAMA, VOL_1800, "LCM"); 
  	}
	hwPowerDown(MT6328_POWER_LDO_VCAMA, "LCM");	
		
#endif

}

static void lcm_resume_power(void)
{
#ifdef BUILD_LK 
	printf("[LK/LCM] lcm_resume_power() enter\n");
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	lcm_Enable_HW(1800);
	MDELAY(20);
			
#else
	printk("[Kernel/LCM] lcm_resume_power() enter\n");
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	hwPowerOn(MT6328_POWER_LDO_VCAMA, VOL_1800, "LCM");
	MDELAY(1);
#endif

}


static void lcm_init(void)
{

	SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(100);


	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}


static void lcm_suspend(void)
{

 
	//SET_RESET_PIN(1);
    //MDELAY(20);
    SET_RESET_PIN(0);
    //MDELAY(20);
    //SET_RESET_PIN(1);
    MDELAY(150);
  
}


static void lcm_resume(void)
{    
 #ifndef BUILD_LK
	lcm_init();
#endif
	
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

	data_array[0]= 0x00052902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(&data_array, 3, 1);
	
	data_array[0]= 0x00052902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(&data_array, 3, 1);

	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	dsi_set_cmdq(&data_array, 1, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(&data_array, 1, 0);

}


static unsigned int lcm_compare_id(void)
{
		unsigned int id=0;
		unsigned char buffer[2];
		unsigned int array[16];  
	

		SET_RESET_PIN(1);
		MDELAY(10);
		SET_RESET_PIN(0);
		MDELAY(50);
		SET_RESET_PIN(1);
		MDELAY(120);
		
	    array[0]=0x00B01501;//Enable external Command 
        dsi_set_cmdq(&array, 1, 1); 
        
		
		array[0] = 0x00043700;// return byte number
		dsi_set_cmdq(&array, 1, 1);
		
	    
	    read_reg_v2(0xff, buffer, 4);
	    id = buffer[0]; 
		
#ifdef BUILD_LK
		printf("=====>compare id for test %s, id = 0x%08x,buffer[1] = %x,buffer[2] = %x,buffer[3] = %x\n", __func__, id,buffer[1],buffer[2],buffer[3]);
#else 
		printk("=====>compare id for test %s, id = 0x%08x,buffer[1] = %x,buffer[2] = %x,buffer[3] = %x\n", __func__, id,buffer[1],buffer[2],buffer[3]);
#endif
	
		return (LCM_ID == id)?1:0;

}

static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK
char  buffer[3],buffer1[6],buffer2[3];
	int   array[16],array1[16],array2[16];

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}


	array[0] = 0x00043700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x09, buffer, 4);
	
	array1[0] = 0x00073700;
	dsi_set_cmdq(array1, 1, 1);
	read_reg_v2(0xB1, buffer1, 7);
	
	printk("esdcheck:0x%x ,0x%x ,0x%x, 0x%x \n",buffer[0],buffer[1],buffer[2],buffer[3]);
	
	if((buffer[0]==0x80)&&(buffer[1]==0x73)&&(buffer[2]==0x4)&&(buffer1[5]==0x11)&&(buffer1[6]==0x11))
	{
		return FALSE;
	}
	else
	{			 
		return TRUE;
	}
 #endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();

	return TRUE;
}

LCM_DRIVER ek79023_dsi_wsvga_vdo_lcm_drv = 
{
    .name		= "ek79023_dsi_wsvga_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
     .init_power		= lcm_init_power,
     .resume_power = lcm_resume_power,
     .suspend_power = lcm_suspend_power,
	//.compare_id     = lcm_compare_id,
	//.esd_check = lcm_esd_check,
	//.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
        //.set_backlight	= lcm_setbacklight,
		//.set_pwm        = lcm_setpwm,
		//.get_pwm        = lcm_getpwm,
        .update         = lcm_update
#endif
    };

