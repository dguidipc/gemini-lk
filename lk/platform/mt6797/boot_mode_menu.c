/*This file implements MTK boot mode.*/

#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <reg.h>
#include <video.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/mtk_key.h>
#include <platform/mt_gpt.h>
#include <target/cust_key.h>
#include <platform/mtk_wdt.h>
extern  int unshield_recovery_detection(void);
extern BOOL recovery_check_command_trigger(void);
extern void boot_mode_menu_select();
extern void mtk_wdt_disable(void);
#define MODULE_NAME "[BOOT_MENU]"
extern void cmdline_append(const char* append_string);
bool g_boot_menu = false;

void boot_mode_menu_select()
{
	int select = 0;  // 0=recovery mode, 1=fastboot.  2=normal boot 3=normal boot + ftrace.4=slub debug off
	const char* title_msg = "Select Boot Mode:\n[VOLUME_UP to select.  VOLUME_DOWN is OK.]\n\n";

#ifdef MACH_FPGA_NO_DISPLAY
	g_boot_mode = RECOVERY_BOOT;
	return ;
#else
	video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	video_printf(title_msg);
	video_printf("[Recovery    Mode]         <<==\n");
#ifdef MTK_FASTBOOT_SUPPORT
	video_printf("[Fastboot    Mode]             \n");
#endif
	video_printf("[Normal      Boot]             \n");
#if !defined(USER_BUILD) || defined(MTK_BUILD_ROOT)
	video_printf("[Normal      Boot +ftrace]    \n");
	video_printf("[Normal      slub debug off]     \n");
#endif
	while (1) {
		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY)) { //VOL_UP
			g_boot_menu = true;
			switch (select) {

				case 0:
#ifdef MTK_FASTBOOT_SUPPORT
					select = 1;

					video_set_cursor(video_get_rows()/2, 0);
					video_printf(title_msg);
					video_printf("[Recovery    Mode]             \n");
					video_printf("[Fastboot    Mode]         <<==\n");
					video_printf("[Normal      Boot]             \n");
#if !defined(USER_BUILD) || defined(MTK_BUILD_ROOT)
					video_printf("[Normal      Boot +ftrace]     \n");
					video_printf("[Normal      slub debug off]     \n");
#endif
					break;
#endif
				case 1:
					select = 2;

					video_set_cursor(video_get_rows()/2, 0);
					video_printf(title_msg);
					video_printf("[Recovery    Mode]             \n");
#ifdef MTK_FASTBOOT_SUPPORT
					video_printf("[Fastboot    Mode]             \n");
#endif
					video_printf("[Normal      Boot]         <<==\n");
#if !defined(USER_BUILD) || defined(MTK_BUILD_ROOT)
					video_printf("[Normal      Boot +ftrace]     \n");
					video_printf("[Normal      slub debug off]     \n");
#endif
					break;

				case 2:
#if defined(USER_BUILD) && !defined(MTK_BUILD_ROOT)
					select = 0;
#else
					select = 3;
#endif

					video_set_cursor(video_get_rows()/2, 0);
					video_printf(title_msg);
#if defined(USER_BUILD) && !defined(MTK_BUILD_ROOT)
					video_printf("[Recovery    Mode]         <<==\n");
#ifdef MTK_FASTBOOT_SUPPORT
					video_printf("[Fastboot    Mode]             \n");
#endif
					video_printf("[Normal      Boot]             \n");
#else
					video_printf("[Recovery    Mode]             \n");
#ifdef MTK_FASTBOOT_SUPPORT
					video_printf("[Fastboot    Mode]             \n");
#endif
					video_printf("[Normal      Boot]             \n");
					video_printf("[Normal      Boot +ftrace] <<==\n");
					video_printf("[Normal      slub debug off]     \n");
#endif
					break;

#if !defined(USER_BUILD) || defined(MTK_BUILD_ROOT)
				case 3:
					select = 4;
					video_set_cursor(video_get_rows()/2, 0);
					video_printf(title_msg);
					video_printf("[Recovery    Mode]             \n");
#ifdef MTK_FASTBOOT_SUPPORT
					video_printf("[Fastboot    Mode]             \n");
#endif
					video_printf("[Normal      Boot]             \n");
					video_printf("[Normal      Boot +ftrace]     \n");
					video_printf("[Normal      slub debug off] <<==\n");

					break;
#endif


#if !defined(USER_BUILD) || defined(MTK_BUILD_ROOT)
				case 4:
					select = 0;
					video_set_cursor(video_get_rows()/2, 0);
					video_printf(title_msg);
					video_printf("[Recovery    Mode]         <<==\n");
#ifdef MTK_FASTBOOT_SUPPORT
					video_printf("[Fastboot    Mode]             \n");
#endif
					video_printf("[Normal      Boot]             \n");
					video_printf("[Normal      Boot +ftrace]     \n");
					video_printf("[Normal      slub debug off]     \n");
					break;
#endif

				default:
					break;
			}
			dprintf(0,  "[VOL_UP]Key Detect, current select:%d\n", select);
			mdelay(300);
		} else if (mtk_detect_key(MT65XX_MENU_OK_KEY)) { //VOL_DOWN,
			//use for OK
			break;
		} else {
			//pass
		}
	}
	if (select == 0) {
		g_boot_mode = RECOVERY_BOOT;
	} else if (select == 1) {
		g_boot_mode = FASTBOOT;
	} else if (select == 2) {
		g_boot_mode = NORMAL_BOOT;
	} else if (select == 3) {
                cmdline_append("boot_trace=1");
		g_boot_mode = NORMAL_BOOT;
	} else if (select == 4) {
		cmdline_append("slub_debug=-");

		g_boot_mode = NORMAL_BOOT;
	} else {
		//pass
	}

	video_set_cursor(video_get_rows()/2 +8, 0);
	return;
#endif
}

BOOL boot_menu_key_trigger(void)
{
#if 1
	//wait
	ulong begin = get_timer(0);
	dprintf(INFO, "\n%s Check  boot menu\n",MODULE_NAME);
	dprintf(INFO, "%s Wait 50ms for special keys\n",MODULE_NAME);

	//let some case of recovery mode pass.
	if (unshield_recovery_detection()) {
		return TRUE;
	}

	while (get_timer(begin)<50) {
		if (mtk_detect_key(MT65XX_BOOT_MENU_KEY)) {
			mtk_wdt_disable();
			boot_mode_menu_select();
			mtk_wdt_init();
			return TRUE;
		}
	}
#endif
	return FALSE;
}

BOOL boot_menu_detection(void)
{
	return boot_menu_key_trigger();
}


int unshield_recovery_detection(void)
{
	//because recovery_check_command_trigger's BOOL is different from the BOOL in this file.
	//so use code like this type.
	return recovery_check_command_trigger()? TRUE:FALSE;
}

