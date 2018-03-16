#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
#include <debug.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <platform/mt_typedefs.h>
#include <platform/upmu_common.h>
#include <platform/upmu_hw.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#include <cust_gpio_usage.h>

#include "lcm_define.h"
#include "lcm_drv.h"
#include "lcm_gpio.h"


#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN


static LCM_STATUS _lcm_gpio_check_data(char type, const LCM_DATA_T1 *t1)
{
	switch (type) {
		case LCM_GPIO_MODE:
			switch (t1->data) {
				case LCM_GPIO_MODE_00:
				case LCM_GPIO_MODE_01:
				case LCM_GPIO_MODE_02:
				case LCM_GPIO_MODE_03:
				case LCM_GPIO_MODE_04:
				case LCM_GPIO_MODE_05:
				case LCM_GPIO_MODE_06:
				case LCM_GPIO_MODE_07:
					break;

				default:
					dprintf(0, "[LCM][ERROR] %s: %d, %d\n", __func__, (unsigned int)type, (unsigned int)t1->data);
					return LCM_STATUS_ERROR;
			}
			break;

		case LCM_GPIO_DIR:
			switch (t1->data) {
				case LCM_GPIO_DIR_IN:
				case LCM_GPIO_DIR_OUT:
					break;

				default:
					dprintf(0, "[LCM][ERROR] %s: %d, %d\n", __func__, (unsigned int)type, (unsigned int)t1->data);
					return LCM_STATUS_ERROR;
			}
			break;

		case LCM_GPIO_OUT:
			switch (t1->data) {
				case LCM_GPIO_OUT_ZERO:
				case LCM_GPIO_OUT_ONE:
					break;

				default:
					dprintf(0, "[LCM][ERROR] %s: %d, %d\n", __func__, (unsigned int)type, (unsigned int)t1->data);
					return LCM_STATUS_ERROR;
			}
			break;

		default:
			dprintf(0, "[LCM][ERROR] %s: %d\n", __func__, (unsigned int)type);
			return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


LCM_STATUS lcm_gpio_set_data(char type, const LCM_DATA_T1 *t1)
{
	// check parameter is valid
	if (LCM_STATUS_OK == _lcm_gpio_check_data(type, t1)) {
		switch (type) {
			case LCM_GPIO_MODE:
				mt_set_gpio_mode(GPIO_65132_EN, (unsigned int)t1->data);
				break;

			case LCM_GPIO_DIR:
				mt_set_gpio_dir(GPIO_65132_EN, (unsigned int)t1->data);
				break;

			case LCM_GPIO_OUT:
				mt_set_gpio_out(GPIO_65132_EN, (unsigned int)t1->data);
				break;

			default:
				dprintf(0, "[LCM][ERROR] %s: %d\n", __func__, (unsigned int)type);
				return LCM_STATUS_ERROR;
		}
	} else {
		dprintf(0, "[LCM][ERROR] %s: 0x%x, 0x%x\n", __func__, (unsigned int)type, (unsigned int)t1->data);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}
#endif

