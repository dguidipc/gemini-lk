#ifndef _MT65XX_LEDS_H
#define _MT65XX_LEDS_H

#define ISINK_CHOP_CLK
#include <cust_leds.h>
#include <platform/mt_typedefs.h>

#define ERROR_BL_LEVEL 0xFFFFFFFF

enum led_color {
	LED_RED,
	LED_GREEN,
	LED_BLUE,
};

enum led_brightness {
	LED_OFF     = 0,
	LED_HALF    = 127,
	LED_FULL    = 255,
};

typedef enum {
	ISINK_PWM_MODE = 0,
	ISINK_BREATH_MODE = 1,
	ISINK_REGISTER_MODE = 2
} MT65XX_PMIC_ISINK_MODE;

typedef enum {
	ISINK_0 = 0,  //4mA
	ISINK_1 = 1,  //8mA
	ISINK_2 = 2,  //12mA
	ISINK_3 = 3,  //16mA
	ISINK_4 = 4,  //20mA
	ISINK_5 = 5   //24mA
} MT65XX_PMIC_ISINK_STEP;

typedef enum {
	//32K clock
	ISINK_1KHZ = 0,
	ISINK_200HZ = 4,
	ISINK_5HZ = 199,
	ISINK_2HZ = 499,
	ISINK_1HZ = 999,
	ISINK_05HZ = 1999,
	ISINK_02HZ = 4999,
	ISINK_01HZ = 9999,
	//2M clock
	ISINK_2M_20KHZ = 2,
	ISINK_2M_1KHZ = 61,
	ISINK_2M_200HZ = 311,
	ISINK_2M_5HZ = 12499,
	ISINK_2M_2HZ = 31249,
	ISINK_2M_1HZ = 62499
} MT65XX_PMIC_ISINK_FSEL;


#ifndef MACH_FPGA_LED_SUPPORT
int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level);
void leds_battery_full_charging(void);
void leds_battery_low_charging(void);
void leds_battery_medium_charging(void);
void mt65xx_backlight_on(void);
void mt65xx_backlight_off(void);
void leds_init(void);
void leds_deinit(void);
#else
static int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level) {return 0;}
static void leds_battery_full_charging(void) {}
static void leds_battery_low_charging(void) {}
static void leds_battery_medium_charging(void) {}
static inline void mt65xx_backlight_on(void) {}
static void mt65xx_backlight_off(void) {}
static void leds_init(void) {}
static void leds_deinit(void) {}
#endif

#endif // !_MT65XX_LEDS_H
