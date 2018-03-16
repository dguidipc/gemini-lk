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

#include <app.h>
#include <debug.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <dev/udc.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <arch/ops.h>

#include <target.h>
#include <platform.h>

#include <platform/mt_reg_base.h>
#include <platform/boot_mode.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_rtc.h>
#include <platform/bootimg.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/mt_gpt.h>
#include <platform/disp_drv_platform.h>
#include <platform/mt_disp_drv.h>
#include "fastboot.h"
#include "sys_commands.h"
extern boot_img_hdr *g_boot_hdr;
extern int g_is_64bit_kernel;
extern struct fastboot_var *varlist;
extern struct fastboot_cmd *cmdlist;
extern int mboot_android_check_img_info(char *part_name, part_hdr_t *part_hdr);
extern int boot_linux_from_storage(void);
extern const char* mt_disp_get_lcm_id(void);
extern void boot_linux(void *kernel, unsigned *tags,char *cmdline, unsigned machtype,void *ramdisk, unsigned ramdisk_size);
extern unsigned int memory_size();
extern void custom_port_in_kernel(BOOTMODE boot_mode, char *command);
#ifdef MTK_OFF_MODE_CHARGE_SUPPORT
extern void set_off_mode_charge_status(int enable);
extern bool get_off_mode_charge_status(void);
#endif

#define MODULE_NAME "FASTBOOT"
#define MAX_RSP_SIZE 64


extern BOOTMODE g_boot_mode;
extern bool cmdline_append(const char *append_string);
extern bool cmdline_overwrite(const char* overwrite_string);
extern timer_t wdt_timer;
extern unsigned int _heap_end;

int has_set_p2u = 0; //0-unset; 1-on; 2-off
#ifdef MTK_TC7_COMMON_DEVICE_INTERFACE
static unsigned char *head_of_adb_cmdline;
static unsigned char adb_cmdline_appended;
#endif

void cmd_overwirte_cmdline(const char *arg, void *data, unsigned sz)
{
	const char *new_kernel_cmd_line = arg;
	if (cmdline_overwrite(new_kernel_cmd_line) == false) {
		dprintf(CRITICAL, "[FASTBOOT] Input cmdline length is too long!");
		fastboot_fail("cmdline length is too long");
	}
	dprintf(INFO, "[FASTBOOT] New command line is %s\n", (char *)cmdline_get());
	fastboot_okay("");
}


void cmd_oem_append_cmdline(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "APPEND KERNEL CMDLINE\n");
	fastboot_info("APPEND KERNEL CMDLINE\n");
	if (cmdline_append(arg) == false) {
		dprintf(CRITICAL, "[FASTBOOT] Input cmdline length is too long!");
		fastboot_fail("cmdline length is too long");
		return;
	}
	dprintf(INFO, "[FASTBOOT] New command line:%s\n", (char *)cmdline_get());
	fastboot_okay("");
	return;
}

#ifdef MTK_TC7_COMMON_DEVICE_INTERFACE
void cmd_oem_ADB_Auto_Enable(const char *arg, void *data, unsigned sz)
{
	char *str_enable=" enable";
	char *str_disable=" disable";
	char *str_help=" help";
	if (adb_cmdline_appended) {
		if (memcmp(arg,str_enable,strlen(str_enable))==0) {
			*head_of_adb_cmdline='1';
			dprintf(INFO, "[FASTBOOT] New command line:%s\n", cmdline_get());
			fastboot_okay("");
			return;
		} else if (memcmp(arg,str_disable,strlen(str_disable))==0) {
			*head_of_adb_cmdline='0';
			dprintf(INFO, "[FASTBOOT] New command line:%s\n", cmdline_get());
			fastboot_okay("");
			return;
		} else if (memcmp(arg,str_help,strlen(str_help))==0) {
			fastboot_info("fastboot oem auto-ADB enable    Enable Auto ADB");
			fastboot_info("fastboot oem auto-ADB disable   Disable Auto ADB");
			fastboot_okay("");
			return;
		} else {
			fastboot_fail("Please enter correct cmd");
		}
	} else {
		if (memcmp(arg,str_enable,strlen(str_enable))==0) {
			cmd_oem_append_cmdline(" AdbAutoEnable=1",data,sz);
			head_of_adb_cmdline=strchr(cmdline_get(),'\0')-0x1;
			dprintf(INFO, "Head of auto-adb cmd %p,%s\n",head_of_adb_cmdline,head_of_adb_cmdline);
			adb_cmdline_appended=1;
			return;
		} else if (memcmp(arg,str_disable,strlen(str_disable))==0) {
			cmd_oem_append_cmdline(" AdbAutoEnable=0",data,sz);
			head_of_adb_cmdline=strchr(cmdline_get(),'\0')-0x1;
			dprintf(INFO,"Head of auto-adb cmd %p,%s\n",head_of_adb_cmdline,head_of_adb_cmdline);
			adb_cmdline_appended=1;
			return;
		} else if (memcmp(arg,str_help,strlen(str_help))==0) {
			fastboot_info("fastboot oem auto-ADB enable:  enable Auto ADB");
			fastboot_info("fastboot oem auto-ADB disable: disable Auto ADB");
			fastboot_okay("");
			return;
		} else {
			fastboot_fail("Please enter correct cmd");
		}
	}
	return;
}
#endif

void set_p2u(int mode)
{
	if (mode == 1) {
		if (cmdline_append("printk.disable_uart=0") == false) {
			dprintf(CRITICAL, "command line is too long, will not set printk_on\n");
			fastboot_info("command line is too long, will not set printk_on");
		} else {
			fastboot_info("printk to uart is on!");
		}
	} else if (mode == 2) {
		if (cmdline_append("printk.disable_uart=1") == false) {
			dprintf(CRITICAL, "command line is too long, will not set printk_off\n");
			fastboot_info("command line is too long, will not set printk_off");
		} else {
			fastboot_info("printk to uart is off!");
		}
	}
}

void cmd_continue(const char *arg, void *data, unsigned sz)
{
	g_boot_mode = NORMAL_BOOT;
	if (has_set_p2u) {
		set_p2u(has_set_p2u);
		fastboot_info("phone will continue boot up after 5s...");
		fastboot_okay("");
		mdelay(5000);
	} else {
		fastboot_okay("");
	}
	udc_stop();
	//mtk_wdt_init(); //re-open wdt
	timer_cancel(&wdt_timer);
	mtk_wdt_restart();

	/*Will not return*/
	boot_linux_from_storage();
}

void cmd_oem_p2u(const char *arg, void *data, unsigned sz)
{
	if (!strncmp(arg, " on", strlen(" on"))) {
		has_set_p2u = 1;
	} else if (!strncmp(arg, " off", strlen(" off"))) {
		has_set_p2u = 2;
	} else {
		has_set_p2u = 0;
		fastboot_info("unknown argument");
	}
	fastboot_okay("");
}

void cmd_oem_reboot2recovery(const char *arg, void *data, unsigned sz)
{
	extern void Set_RTC_Recovery_Mode(bool flag)__attribute__((weak));

	if (Set_RTC_Recovery_Mode) {
		Set_RTC_Recovery_Mode(1);
		fastboot_okay("");
		mtk_arch_reset(1); //bypass pwr key when reboot
	} else {
		fastboot_fail("Not support this function (need RTC porting)");
	}
}
#ifdef MTK_OFF_MODE_CHARGE_SUPPORT
void set_off_mode_charge(int mode)
{
	struct fastboot_var *var;

	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, "off-mode-charge")) {
			var->value = (mode == 1) ? "1" : "0";
		}
	}
	set_off_mode_charge_status(mode);
}
void cmd_oem_off_mode_charge(const char *arg, void *data, unsigned sz)
{
	if (!strncmp(arg, " 1", strlen(" 1"))) {
		//CHARGE MODE
		set_off_mode_charge(1);
	} else if (!strncmp(arg, " 0", strlen(" 0"))) {
		//NORMAL MODE
		set_off_mode_charge(0);
	} else {
		fastboot_info("unknown argument");
	}
	fastboot_okay("");
}
#endif
#ifdef MTK_JTAG_SWITCH_SUPPORT
extern unsigned int set_ap_jtag(unsigned int);
extern unsigned int get_ap_jtag(void);
void cmd_oem_ap_jtag(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];

	if (!strncmp(arg, " 1", strlen(" 1"))) {
		//turn ap jtag on
		fastboot_info("Enable AP JTAG");
		set_ap_jtag(1);
	} else if (!strncmp(arg, " 0", strlen(" 0"))) {
		//turn ap jtag off
		fastboot_info("Disable AP JTAG");
		set_ap_jtag(0);
	} else {
		snprintf(response, MAX_RSP_SIZE,"\tCurrent AP JTAG setting:%s", get_ap_jtag()?"Enable":"Disable");
		fastboot_info(response);
	}
	fastboot_okay("");
}
#endif
#ifdef MTK_TINYSYS_SCP_SUPPORT
extern unsigned int set_scp_status(unsigned int);
extern unsigned int get_scp_status(void);
void cmd_oem_scp_status(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];

	if (!strncmp(arg, " 1", strlen(" 1"))) {
		//turn scp on
		fastboot_info("Turn on SCP");
		fastboot_info("Plz reboot system to make it applied.");
		set_scp_status(1);
	} else if (!strncmp(arg, " 0", strlen(" 0"))) {
		//turn scp off
		fastboot_info("Turn off SCP");
		fastboot_info("Plz reboot system to make it applied.");
		set_scp_status(0);
	} else {
		snprintf(response, MAX_RSP_SIZE,"\tCurrent SCP setting:%s", get_scp_status()?"Enable":"Disable");
		fastboot_info(response);
	}
	fastboot_okay("");
}
#endif
#ifdef MTK_USB2JTAG_SUPPORT
extern unsigned int set_usb2jtag(unsigned int);
extern unsigned int get_usb2jtag(void);
void cmd_oem_usb2jtag(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];

	if (!strncmp(arg, " 1", strlen(" 1"))) {
		//turn usb2jtag on
		fastboot_info("Enable USB2JTAG");
		set_usb2jtag(1);
	} else if (!strncmp(arg, " 0", strlen(" 0"))) {
		//turn usb2jtag off
		fastboot_info("Disable USB2JTAG");
		set_usb2jtag(0);
	} else {
		snprintf(response, MAX_RSP_SIZE,"\tCurrent USB2JTAG setting:%s", get_usb2jtag()?"Enable":"Disable");
		fastboot_info(response);
	}
	fastboot_info("rebooting device for usb2jtag support..\n");
	fastboot_okay("");
	mtk_arch_reset(1);
}
#endif
void cmd_getvar(const char *arg, void *data, unsigned sz)
{
	struct fastboot_var *var;
	char response[MAX_RSP_SIZE];

	if (!strcmp(arg, "all")) {
		for (var = varlist; var; var = var->next) {
			snprintf(response, MAX_RSP_SIZE,"\t%s: %s", var->name, var->value);
			fastboot_info(response);
		}
		fastboot_okay("Done!!");
		return;
	}
	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			fastboot_okay(var->value);
			return;
		}
	}
	fastboot_okay("");
}

void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	mtk_arch_reset(1); //bypass pwr key when reboot
}

void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device to bootloader\n");
	fastboot_okay("");
	Set_Clr_RTC_PDN1_bit13(true); //Set RTC fastboot bit
	mtk_arch_reset(1); //bypass pwr key when reboot
}

extern unsigned int g_kmem_off;
extern unsigned int g_rmem_off;
extern unsigned int g_bimg_sz;
#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
void cmd_boot(const char *arg, void *data, unsigned sz)
{
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	struct boot_img_hdr *boot_hdr;
	char *ptr = ((char*) data);
	unsigned page_size = 0;
	unsigned page_mask = 0;
#define CMDLINE_TMP_CONCAT_SIZE 100     //only for string concat, 100 bytes is enough
	char cmdline_tmpbuf[CMDLINE_TMP_CONCAT_SIZE];

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));

	if (!boot_hdr) {
		dprintf(CRITICAL, "cmd_boot,boot_hdr = NULL\n");
		return;
	}

	memcpy(boot_hdr, data, sizeof(boot_img_hdr));

	dprintf(INFO, "\n============================================================\n");
	boot_hdr->magic[7] = '\0';
	dprintf(INFO, "[%s] Android Boot IMG Hdr - Magic 	        : %s\n",MODULE_NAME,boot_hdr->magic);
	dprintf(INFO, "[%s] Android Boot IMG Hdr - Kernel Size 	: 0x%x\n",MODULE_NAME,boot_hdr->kernel_size);
	dprintf(INFO, "[%s] Android Boot IMG Hdr - Kernel addr 	: 0x%x\n",MODULE_NAME,boot_hdr->kernel_addr);
	dprintf(INFO, "[%s] Android Boot IMG Hdr - Rootfs Size 	: 0x%x\n",MODULE_NAME,boot_hdr->ramdisk_size);
	dprintf(INFO, "[%s] Android Boot IMG Hdr - Page Size    	: 0x%x\n",MODULE_NAME,boot_hdr->page_size);
	dprintf(INFO, "============================================================\n");

	/* ensure commandline is terminated */
	boot_hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;

	if (boot_hdr->page_size) {
		page_size = boot_hdr->page_size;
		page_mask = page_size - 1;
		//page_mask = 2*1024 ; /*FIXME*/
	} else {
		dprintf(CRITICAL, "[FASTBOOT] Please specify the storage page-size in the boot header!\n");
		fastboot_fail("Please specify the storage page-size in the boot header!\n");
		goto _cmd_boot_exit;
	}

	kernel_actual = ROUND_TO_PAGE(boot_hdr->kernel_size, page_mask);
	ramdisk_actual = ROUND_TO_PAGE(boot_hdr->ramdisk_size, page_mask);

	/* sz should have atleast raw boot image */
	if (page_size + kernel_actual + ramdisk_actual > sz) {
		fastboot_fail("incomplete bootimage");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->kernel_addr <= ROUND_TO_PAGE((boot_hdr->ramdisk_addr + boot_hdr->ramdisk_size), page_mask)) && (ROUND_TO_PAGE((boot_hdr->kernel_addr + boot_hdr->kernel_size), page_mask) >= boot_hdr->ramdisk_addr)) {
		fastboot_fail("invalid kernel & ramdisk address: images overlap");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->kernel_addr < DRAM_PHY_ADDR) || (ROUND_TO_PAGE((boot_hdr->kernel_addr + boot_hdr->kernel_size), page_mask) > (DRAM_PHY_ADDR + (u64)memory_size()))) {
		fastboot_fail("invalid kernel address: not lie in memory");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->kernel_addr <= _heap_end) && (ROUND_TO_PAGE((boot_hdr->kernel_addr + boot_hdr->kernel_size), page_mask) >= MEMBASE)) {
		fastboot_fail("invalid kernel address: overlap with lk");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->kernel_addr <= ROUND_TO_PAGE((SCRATCH_ADDR + boot_hdr->kernel_size + boot_hdr->ramdisk_size), page_mask)) && (ROUND_TO_PAGE((boot_hdr->kernel_addr + boot_hdr->kernel_size), page_mask) >= SCRATCH_ADDR)) {
		fastboot_fail("invalid kernel address: overlap with the download image");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->ramdisk_addr < DRAM_PHY_ADDR) || (ROUND_TO_PAGE((boot_hdr->ramdisk_addr + boot_hdr->ramdisk_size), page_mask) > (DRAM_PHY_ADDR + (u64)memory_size()))) {
		fastboot_fail("invalid ramdisk address: not lie in memory");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->ramdisk_addr <= _heap_end) && (ROUND_TO_PAGE((boot_hdr->ramdisk_addr + boot_hdr->ramdisk_size), page_mask) >= MEMBASE)) {
		fastboot_fail("invalid ramdisk address: overlap with lk");
		goto _cmd_boot_exit;
	}

	if ((boot_hdr->ramdisk_addr <= ROUND_TO_PAGE((SCRATCH_ADDR + boot_hdr->kernel_size + boot_hdr->ramdisk_size), page_mask)) && (ROUND_TO_PAGE((boot_hdr->ramdisk_addr + boot_hdr->ramdisk_size), page_mask) >= SCRATCH_ADDR)) {
		fastboot_fail("invalid ramdisk address: overlap with the download image");
		goto _cmd_boot_exit;
	}

	/*
	 * check mkimg header
	 */
	dprintf(INFO, "check mkimg header\n");
	if (mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)(ptr + boot_hdr->page_size)) == -1) {
		dprintf(INFO, "no mkimg header in kernel image\n");
		if (g_is_64bit_kernel) {
			memmove((void*) SCRATCH_ADDR, (ptr + boot_hdr->page_size), boot_hdr->kernel_size);
		} else {
			memmove((void*) boot_hdr->kernel_addr, (ptr + boot_hdr->page_size), boot_hdr->kernel_size);
		}
	} else {
		dprintf(INFO, "mkimg header exist in kernel image\n");
		if (g_is_64bit_kernel) {
			memmove((void*) SCRATCH_ADDR, (ptr + boot_hdr->page_size + MKIMG_HEADER_SZ), boot_hdr->kernel_size);
		} else {
			memmove((void*) boot_hdr->kernel_addr, (ptr + boot_hdr->page_size + MKIMG_HEADER_SZ), boot_hdr->kernel_size);
		}
	}
#ifdef MTK_3LEVEL_PAGETABLE
	/* rootfs addr */
	arch_mmu_map((uint64_t)boot_hdr->ramdisk_addr, (uint32_t)boot_hdr->ramdisk_addr,
				 MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(boot_hdr->ramdisk_size, PAGE_SIZE));
#endif
	if ((mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *)(ptr + boot_hdr->page_size + kernel_actual)) == -1)
	        && (mboot_android_check_img_info("RECOVERY", (part_hdr_t *)(ptr + boot_hdr->page_size + kernel_actual)) == -1)) {
		dprintf(INFO, "no mkimg header in ramdisk image\n");
		memmove((void*) boot_hdr->ramdisk_addr, (ptr + boot_hdr->page_size + kernel_actual), boot_hdr->ramdisk_size);
	} else {
		dprintf(INFO, "mkimg header exist in ramdisk image\n");
		memmove((void*) boot_hdr->ramdisk_addr, (ptr + boot_hdr->page_size + kernel_actual + MKIMG_HEADER_SZ), boot_hdr->ramdisk_size);
	}

#ifndef MACH_FPGA
	cmdline_append(boot_hdr->cmdline);
	snprintf(cmdline_tmpbuf, CMDLINE_TMP_CONCAT_SIZE, "lcm=%1d-%s", DISP_IsLcmFound(), mt_disp_get_lcm_id());
	cmdline_append(cmdline_tmpbuf);
	snprintf(cmdline_tmpbuf, CMDLINE_TMP_CONCAT_SIZE, "fps=%1d", mt_disp_get_lcd_time());
	cmdline_append(cmdline_tmpbuf);
	snprintf(cmdline_tmpbuf, CMDLINE_TMP_CONCAT_SIZE, "vram=%1d", DISP_GetVRamSize());
	cmdline_append(cmdline_tmpbuf);
#endif

#ifdef SELINUX_STATUS
#if SELINUX_STATUS == 1
	cmdline_append("androidboot.selinux=disabled");
#elif SELINUX_STATUS == 2
	cmdline_append("androidboot.selinux=permissive");
#endif
#endif

	fastboot_okay("");
	udc_stop();
	//mtk_wdt_init(); //re-open wdt
	timer_cancel(&wdt_timer);
	mtk_wdt_restart();

	g_boot_hdr = boot_hdr;
	g_boot_mode = NORMAL_BOOT;
	custom_port_in_kernel(g_boot_mode, cmdline_get());
	dprintf(CRITICAL, "Kernel Address: 0x%8X\n", boot_hdr->kernel_addr);
	dprintf(CRITICAL, "Ramdisk Address: 0x%8X\n", boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "Atag Address: 0x%8X\n", boot_hdr->tags_addr);
	dprintf(CRITICAL, "Command: %s\n", boot_hdr->cmdline);

	boot_linux((void*) boot_hdr->kernel_addr, (void*) boot_hdr->tags_addr,
	           (char*) cmdline_get(), board_machtype(),
	           (void*) boot_hdr->ramdisk_addr, boot_hdr->ramdisk_size);
_cmd_boot_exit:
	free(boot_hdr);
	return;

}

