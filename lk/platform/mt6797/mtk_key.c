#include <printf.h>
#include <platform/mt_typedefs.h>
#include <platform/mtk_key.h>
#include <platform/boot_mode.h>
#include <platform/mt_pmic.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/sync_write.h>
#include <platform/upmu_common.h>
#include <target/cust_key.h>

extern int pmic_detect_powerkey(void);
extern int pmic_detect_homekey(void);
extern kal_uint16 pmic_get_register_value(PMU_FLAGS_LIST_ENUM flagname);
extern kal_uint16 pmic_set_register_value(PMU_FLAGS_LIST_ENUM flagname,kal_uint32 val);

void set_kpd_pmic_mode(void)
{
	return;
}

void disable_PMIC_kpd_clock(void)
{
	return;
}

void enable_PMIC_kpd_clock(void)
{
	return;
}

BOOL mtk_detect_key(unsigned short key) /* key: HW keycode */
{
#ifdef MACH_FPGA
	return FALSE;
#else

	dprintf(ALWAYS, "mtk detect key function key = %d\n", key);

	unsigned short idx, bit, din;

	if (key >= KPD_NUM_KEYS)
		return FALSE;
#if 0
	if (key % 9 == 8)
		key = 8;
#endif
	if (key == MTK_PMIC_PWR_KEY) {
		/* Power key */
		if (1 == pmic_detect_powerkey()) {
			//dbg_print ("power key is pressed\n");
			return TRUE;
		}
		return FALSE;
	}

#ifdef MTK_PMIC_RST_KEY
	if (key == MTK_PMIC_RST_KEY) {
		dprintf(INFO, "mtk detect key function pmic_detect_homekey MTK_PMIC_RST_KEY = %d\n", MTK_PMIC_RST_KEY);
		if (1 == pmic_detect_homekey()) {
			dprintf(ALWAYS, "mtk detect key function pmic_detect_homekey pressed\n");
			return TRUE;
		}
		return FALSE;
	}
#endif

	idx = key / 16;
	bit = key % 16;

	din = DRV_Reg16(KP_MEM1 + (idx << 2)) & (1U << bit);
	if (!din) {
		dprintf(ALWAYS, "key %d is pressed\n", key);
		return TRUE;
	}
	return FALSE;
#endif
}

BOOL mtk_detect_pmic_just_rst(void)
{
	kal_uint32 just_rst = 0;

	dprintf(ALWAYS, "detecting pmic just reset\n");

	just_rst = pmic_get_register_value(MT6351_PMIC_JUST_PWRKEY_RST);
	if (just_rst) {
		dprintf(ALWAYS, "Just recover from a reset\n");
		pmic_set_register_value(MT6351_PMIC_CLR_JUST_RST, 0x01);
		return TRUE;
	}
	return FALSE;
}
