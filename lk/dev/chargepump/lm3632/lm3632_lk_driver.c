//#include <platform/mt_pwm.h>
#include <mt_gpio.h>
#include <mt_i2c.h>
#include <cust_gpio_usage.h>
//#include <lge_bootmode.h>
//#include <platform/boot_mode.h>

#define CPD_TAG                         "[LK][chargepump] "
#define CPD_FUN(f)                     printf(CPD_TAG"%s\n", __FUNCTION__)
#define CPD_ERR(fmt, args...)   printf(CPD_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define CPD_LOG(fmt, args...)   printf(CPD_TAG fmt, ##args)

#define LM3632_SLAVE_ADDR (0x11<<1)

#define LM3632_CHIP_ENABLE_PIN GPIO_LCD_BL_EN
#define LM3632_CHIP_ENABLE_PIN_MODE GPIO_LCD_BL_EN_M_GPIO

#define LM3632_DSV_VPOS_EN  GPIO_DSV_AVEE_EN
#define LM3632_DSV_VPOS_EN_MODE GPIO_DSV_AVEE_EN_M_GPIO

#define LM3632_DSV_VNEG_EN GPIO_DSV_AVDD_EN
#define LM3632_DSV_VNEG_EN_MODE GPIO_DSV_AVDD_EN_M_GPIO

static unsigned long lm3632_i2c_write_byte(unsigned char reg_addr, unsigned char data)
{
        unsigned long ret_code = I2C_OK;
        unsigned char write_data[I2C_FIFO_SIZE];
        unsigned char slave_addr = LM3632_SLAVE_ADDR;

        write_data[0] = reg_addr;
        write_data[1] = data;

        ret_code = mt_i2c_write(I2C2, slave_addr, write_data, 2, 0);

        return ret_code;
}

static int lm3632_hw_init()
{
        int ret_code;

        CPD_FUN();

        // GPIO for DSV setting
	mt_set_gpio_mode(LM3632_DSV_VPOS_EN, LM3632_DSV_VPOS_EN_MODE);
	mt_set_gpio_pull_enable(LM3632_DSV_VPOS_EN, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(LM3632_DSV_VPOS_EN, GPIO_DIR_OUT);
	mt_set_gpio_mode(LM3632_DSV_VNEG_EN, LM3632_DSV_VNEG_EN_MODE);
	mt_set_gpio_pull_enable(LM3632_DSV_VNEG_EN, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(LM3632_DSV_VNEG_EN, GPIO_DIR_OUT);

        // Backlight Enable Pin
        mt_set_gpio_mode(LM3632_CHIP_ENABLE_PIN, LM3632_CHIP_ENABLE_PIN_MODE);
        mt_set_gpio_dir(LM3632_CHIP_ENABLE_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(LM3632_CHIP_ENABLE_PIN, GPIO_OUT_ONE);
        udelay(100);
        mt_set_gpio_out(LM3632_CHIP_ENABLE_PIN,GPIO_OUT_ZERO);
        udelay(10000);
        mt_set_gpio_out(LM3632_CHIP_ENABLE_PIN,GPIO_OUT_ONE);
        udelay(10);

        ret_code = lm3632_i2c_write_byte(0x02, 0x70);  // OVP 29V, linear mode, TI recommand
        if(ret_code != I2C_OK) {
            CPD_ERR("I2C fail: %d\n", ret_code);
            return ret_code;
        }

        // LCM bias(Vpos, Vneg) controlled by external pin, not I2C
        ret_code = lm3632_i2c_write_byte(0x0C, 0x01);  // VPOS_EN=0, VNEG_EN=0, EXT_EN=1
        if(ret_code != I2C_OK) {
            CPD_ERR("I2C fail: %d\n", ret_code);
            return ret_code;
        }

        // Vpos voltage setting as +5.5v
        ret_code = lm3632_i2c_write_byte(0x0E, 0x1E);
        if(ret_code != I2C_OK) {
            CPD_ERR("I2C fail: %d\n", ret_code);
            return ret_code;
        }

        // Vneg voltage setting as -5.5v
        ret_code = lm3632_i2c_write_byte(0x0F, 0x1E);
        if(ret_code != I2C_OK) {
            CPD_ERR("I2C fail: %d\n", ret_code);
            return ret_code;
        }

        return 0;
}

static int get_backlight_enable=0;
int chargepump_set_backlight_level(int level)
{
        unsigned char cmd = 0x0;
        int cmd_len = 1;
        unsigned char data = 0x00;
        int data_len = 1;
        unsigned long ret_code;
        int i = 0;

        if(get_backlight_enable == 0)
        {
            lm3632_hw_init();
            get_backlight_enable=1;		
        }

        CPD_LOG("chargepump_set_backlight_level, level : %d\n", level);

        if(level != 0) 
        {
            // Backlight brightness setting
            ret_code = lm3632_i2c_write_byte(0x04, 0x07);  // Backlight brightness LSB 3bits, 0b111
            if(ret_code != I2C_OK) {
                CPD_ERR("I2C fail: %d\n", ret_code);
                return ret_code;
            }

            if(level == 33)  //download mode
            {
                ret_code = lm3632_i2c_write_byte(0x05, 0xb2);  // Backlight brightness MSB 8bits, 7/10 of Max brightness
            }
            else
            {
                ret_code = lm3632_i2c_write_byte(0x05, 0xff);  // Max Brightness
            }

            if(ret_code != I2C_OK) {
                CPD_ERR("I2C fail: %d\n", ret_code);
                return ret_code;
            }

            // backlight enable
            ret_code = lm3632_i2c_write_byte(0x0A, 0x11);  // TI recommand, BL_EN enable
            if(ret_code != I2C_OK) {
                CPD_ERR("I2C fail: %d\n", ret_code);
                return ret_code;
            }
        }
        else
        {
            // backlight disable
            ret_code = lm3632_i2c_write_byte(0x0A, 0x10);  // BL_EN == 0
            if(ret_code != I2C_OK) {
                CPD_ERR("I2C fail: %d\n", ret_code);
                return ret_code;
            }
        }
        return 1;
}

void chargepump_DSV_on()
{
        //CPD_FUN();        
        mt_set_gpio_out(LM3632_DSV_VPOS_EN, GPIO_OUT_ONE);
        mdelay(1);
        mt_set_gpio_out(LM3632_DSV_VNEG_EN, GPIO_OUT_ONE);
}

void chargepump_DSV_off()
{
        //CPD_FUN();
        mt_set_gpio_out(LM3632_DSV_VPOS_EN, GPIO_OUT_ZERO);
        mdelay(1);
        mt_set_gpio_out(LM3632_DSV_VNEG_EN, GPIO_OUT_ZERO);
}
