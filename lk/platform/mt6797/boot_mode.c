/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*This file implements MTK boot mode.*/

#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <reg.h>

#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <target/cust_key.h>
#include <platform/meta.h>
#include <dev/mrdump.h>
#include <platform/mt_rtc.h>
#include <platform/mtk_key.h>
#include <platform/mt_gpt.h>
#include <platform/mtk_wdt.h>
// global variable for specifying boot mode (default = NORMAL)
extern  int unshield_recovery_detection(void);
extern  void mtk_wdt_disable(void);
extern  void boot_mode_menu_select();
BOOTMODE g_boot_mode = NORMAL_BOOT;
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
extern BOOL kernel_power_off_charging_detection(void);
#endif
#ifdef MTK_TC7_FEATURE
extern void cmdline_append(const char* append_string);
#endif
BOOL meta_mode_check(void)
{
	if (g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT || g_boot_mode ==ATE_FACTORY_BOOT || g_boot_mode == FACTORY_BOOT) {
		return TRUE;
	} else {
		return FALSE;
	}
}

extern BOOT_ARGUMENT *g_boot_arg;
#define MODULE_NAME "[FACTORY]"
// check the boot mode : (1) meta mode or (2) recovery mode ...


static int mrdump_check(void)
{
	if (mrdump_detection()) {
#ifdef MTK_TC7_FEATURE
		g_boot_mode = FACTORY_BOOT;
		cmdline_append("frdump_boot=1");
#endif
		mt65xx_backlight_on();
		mrdump_run2();
		return 1;
	} else {
		return 0;
	}
}
void boot_mode_select(void)
{

	int factory_forbidden = 0;
	//  int forbid_mode;
	/*We put conditions here to filer some cases that can not do key detection*/
	extern int kedump_mini(void) __attribute__((weak));
	if (kedump_mini) {
		if (kedump_mini()) {
			mrdump_check();
			return;
		}
	}
	if (meta_detection()) {
		return;
	}
	mrdump_check();

#if defined (HAVE_LK_TEXT_MENU)
	/*Check RTC to know if system want to reboot to Fastboot*/
	if (Check_RTC_PDN1_bit13()) {
		dprintf(CRITICAL, "[FASTBOOT] reboot to boot loader\n");
		g_boot_mode = FASTBOOT;
		Set_Clr_RTC_PDN1_bit13(false);
		return;
	}
	/*If forbidden mode is factory, cacel the factory key detection*/
	if (g_boot_arg->sec_limit.magic_num == 0x4C4C4C4C) {
		if (g_boot_arg->sec_limit.forbid_mode == F_FACTORY_MODE) {
			//Forbid to enter factory mode
			dprintf(CRITICAL, "%s Forbidden\n",MODULE_NAME);
			factory_forbidden=1;
		}
	}
	//  forbid_mode = g_boot_arg->boot_mode &= 0x000000FF;
	/*If boot reason is power key + volumn down, then
	 disable factory mode dectection*/
	if (mtk_detect_pmic_just_rst()) {
		factory_forbidden=1;
	}
	/*Check RTC to know if system want to reboot to Recovery*/
	if (Check_RTC_Recovery_Mode()) {
		g_boot_mode = RECOVERY_BOOT;
		return;
	}
	/*If MISC Write has not completed  in recovery mode
	 before system reboot, go to recovery mode to
	finish remain tasks*/
	if (unshield_recovery_detection()) {
		return;
	}
	ulong begin = get_timer(0);

	/*we put key dectection here to detect key which is pressed*/
	dprintf(INFO, "eng build\n");
#ifdef MT65XX_FACTORY_KEY
	dprintf(CRITICAL, "MT65XX_FACTORY_KEY 0x%x\n",MT65XX_FACTORY_KEY);
#endif
#ifdef MT65XX_BOOT_MENU_KEY
	dprintf(CRITICAL, "MT65XX_BOOT_MENU_KEY 0x%x\n",MT65XX_BOOT_MENU_KEY);
#endif
#ifdef MT65XX_RECOVERY_KEY
	dprintf(CRITICAL, "MT65XX_RECOVERY_KEY 0x%x\n",MT65XX_RECOVERY_KEY);
#endif
	while (get_timer(begin)<50) {

#ifdef MT65XX_FACTORY_KEY

		if (!factory_forbidden) {
			if (mtk_detect_key(MT65XX_FACTORY_KEY)) {
				dprintf(CRITICAL, "%s Detect key\n",MODULE_NAME);
				dprintf(CRITICAL, "%s Enable factory mode\n",MODULE_NAME);
				g_boot_mode = FACTORY_BOOT;
				//video_printf("%s : detect factory mode !\n",MODULE_NAME);
				return;
			}
		}
#endif
		if (mtk_detect_key(MT65XX_BOOT_MENU_KEY)) {
			dprintf(CRITICAL, "\n%s Check  boot menu\n",MODULE_NAME);
			dprintf(CRITICAL, "%s Wait 50ms for special keys\n",MODULE_NAME);
			mtk_wdt_disable();
			/*************************/
			mt65xx_backlight_on();
			/*************************/
			boot_mode_menu_select();
			mtk_wdt_init();
			return;
		}
#ifdef MT65XX_RECOVERY_KEY
		if (mtk_detect_key(MTK_PMIC_PWR_KEY) && !mtk_detect_key(17)) {
			dprintf(CRITICAL, "%s Detect cal key\n",MODULE_NAME);
			dprintf(CRITICAL, "%s Enable recovery mode\n",MODULE_NAME);
			g_boot_mode = RECOVERY_BOOT;
			//video_printf("%s : detect recovery mode !\n",MODULE_NAME);
			return;
		}
#endif
	}
#else

	/*We put conditions here to filer some cases that can not do key detection*/

	/*Check RTC to know if system want to reboot to Fastboot*/
#ifdef MTK_FASTBOOT_SUPPORT
	if (Check_RTC_PDN1_bit13()) {
		dprintf(INFO,"[FASTBOOT] reboot to boot loader\n");
		g_boot_mode = FASTBOOT;
		Set_Clr_RTC_PDN1_bit13(false);
		return TRUE;
	}
#endif

	/*If forbidden mode is factory, cacel the factory key detection*/
	if (g_boot_arg->sec_limit.magic_num == 0x4C4C4C4C) {
		if (g_boot_arg->sec_limit.forbid_mode == F_FACTORY_MODE) {
			//Forbid to enter factory mode
			dprintf(INFO, "%s Forbidden\n",MODULE_NAME);
			factory_forbidden=1;
		}
	}
//    forbid_mode = g_boot_arg->boot_mode &= 0x000000FF;
	/*If boot reason is power key + volumn down, then
	 disable factory mode dectection*/
	if (mtk_detect_pmic_just_rst()) {
		factory_forbidden=1;
	}
	/*Check RTC to know if system want to reboot to Recovery*/
	if (Check_RTC_Recovery_Mode()) {
		g_boot_mode = RECOVERY_BOOT;
		return TRUE;
	}
	/*If MISC Write has not completed  in recovery mode
	 and interrupted by system reboot, go to recovery mode to
	finish remain tasks*/
	if (unshield_recovery_detection()) {
		return TRUE;
	}
	ulong begin = get_timer(0);
	/*we put key dectection here to detect key which is pressed*/
	while (get_timer(begin)<50) {
#ifdef MTK_FASTBOOT_SUPPORT
		if (mtk_detect_key(MT_CAMERA_KEY)) {
			dprintf(INFO,"[FASTBOOT]Key Detect\n");
			g_boot_mode = FASTBOOT;
			return TRUE;
		}
#endif
#ifdef MT65XX_FACTORY_KEY

		if (!factory_forbidden) {
			if (mtk_detect_key(MT65XX_FACTORY_KEY)) {
				dprintf(INFO, "%s Detect key\n",MODULE_NAME);
				dprintf(INFO, "%s Enable factory mode\n",MODULE_NAME);
				g_boot_mode = FACTORY_BOOT;
				//video_printf("%s : detect factory mode !\n",MODULE_NAME);
				return TRUE;
			}
		}
#endif
#ifdef MT65XX_RECOVERY_KEY
		if (mtk_detect_key(MTK_PMIC_PWR_KEY) && !mtk_detect_key(17)) {
			dprintf(INFO, "%s Detect cal key\n",MODULE_NAME);
			dprintf(INFO, "%s Enable recovery mode\n",MODULE_NAME);
			g_boot_mode = RECOVERY_BOOT;
			//video_printf("%s : detect recovery mode !\n",MODULE_NAME);
			return TRUE;
		}
#endif
	}

#endif

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (kernel_power_off_charging_detection()) {
		dprintf(CRITICAL, " < Kernel Power Off Charging Detection Ok> \n");
		return;
	} else {
		dprintf(CRITICAL, "< Kernel Enter Normal Boot > \n");
	}
#endif
}

#if 0
unsigned int get_chip_sw_ver_code(void)
{
	return DRV_Reg32(APSW_VER);
}

CHIP_SW_VER mt_get_chip_sw_ver(void)
{
	return (CHIP_SW_VER)get_chip_sw_ver_code();
}
#endif
CHIP_INFO mt_get_chip_info(void)
{
	return DRV_Reg32(APHW_VER);
}
