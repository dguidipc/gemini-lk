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

#include <atags.h>
#include <app.h>
#include <debug.h>
#include <arch.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <dev/udc.h>
#include <reg.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <arch/ops.h>
#include <ctype.h>

#include <target.h>
#include <platform.h>

#include <platform/mt_reg_base.h>
#include <platform/boot_mode.h>
#include <platform/bootimg.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/mt_disp_drv.h>
#include <platform/env.h>
#include <target/cust_usb.h>
#include <platform/mt_gpt.h>
#if defined(MTK_SECURITY_SW_SUPPORT) && defined(MTK_VERIFIED_BOOT_SUPPORT)
#include "oemkey.h"
#endif

#define BOOT_STATE_GREEN   0x0
#define BOOT_STATE_ORANGE  0x1
#define BOOT_STATE_YELLOW  0x2
#define BOOT_STATE_RED     0x3

#ifdef MBLOCK_LIB_SUPPORT
#include <mblock.h>
#endif

#include <platform/mtk_key.h>

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
#include "platform/mtk_wdt.h"
extern int kernel_charging_boot(void);
extern int pmic_detect_powerkey(void);
extern void mt6575_power_off(void);
extern void mt65xx_backlight_off(void);
#endif
extern void jumparch64(u32 addr, u32 arg1, u32 arg2);
extern void jumparch64_smc(u32 addr, u32 arg1, u32 arg2, u32 arg3);

extern u32 memory_size(void);
extern unsigned *target_atag_devinfo_data(unsigned *ptr);
extern unsigned *target_atag_videolfb(unsigned *ptr);
extern unsigned *target_atag_mdinfo(unsigned *ptr);
extern unsigned *target_atag_ptp(unsigned *ptr);
extern void platform_uninit(void);
extern int mboot_android_load_bootimg_hdr(char *part_name, unsigned long addr);
extern int mboot_android_load_bootimg(char *part_name, unsigned long addr);
extern int mboot_android_load_recoveryimg_hdr(char *part_name, unsigned long addr);
extern int mboot_android_load_recoveryimg(char *part_name, unsigned long addr);
extern int mboot_android_load_factoryimg_hdr(char *part_name, unsigned long addr);
extern int mboot_android_load_factoryimg(char *part_name, unsigned long addr);
extern void custom_port_in_kernel(BOOTMODE boot_mode, char *command);
extern const char* mt_disp_get_lcm_id(void);
extern unsigned int DISP_GetVRamSize(void);
extern int mt_disp_is_lcm_connected(void);
extern int fastboot_init(void *base, unsigned size);
extern int sec_boot_check (int try_lock);
extern int seclib_set_oemkey(u8 *key, u32 key_size);
extern BI_DRAM bi_dram[MAX_NR_BANK];
#ifdef DEVICE_TREE_SUPPORT
#include <libfdt.h>
#ifdef USE_ITS_BOOTIMG
#include <platform/dtb.h>
#endif
extern unsigned int *device_tree, device_tree_size;
#endif
extern unsigned int g_boot_state;
extern int platform_skip_hibernation(void) __attribute__((weak));

int g_is_64bit_kernel = 0;

boot_img_hdr *g_boot_hdr = NULL;

#define CMDLINE_LEN   1024
static char g_cmdline[CMDLINE_LEN] = COMMANDLINE_TO_KERNEL;
char g_boot_reason[][16]= {"power_key","usb","rtc","wdt","wdt_by_pass_pwk","tool_by_pass_pwk","2sec_reboot","unknown","kernel_panic","reboot","watchdog"};
#if defined(MTK_SECURITY_SW_SUPPORT) && defined(MTK_VERIFIED_BOOT_SUPPORT)
u8 g_oemkey[OEM_PUBK_SZ] = {OEM_PUBK};
#endif

signed int fg_swocv_v;
signed int fg_swocv_i;
int shutdown_time;
int boot_voltage;

/* Please define SN_BUF_LEN in cust_usb.h */
#ifndef SN_BUF_LEN
#define SN_BUF_LEN  19  /* fastboot use 13 bytes as default, max is 19 */
#endif

#define FDT_BUFF_SIZE  1024
#define FDT_BUFF_PATTERN  "BUFFEND"

#define DEFAULT_SERIAL_NUM "0123456789ABCDEF"

#define KERNEL_64BITS 1
#define KERNEL_32BITS 0

#define DTB_MAX_SIZE    (512*1024)
/*
 * Support read barcode from /dev/pro_info to be serial number.
 * Then pass the serial number from cmdline to kernel.
 */
/* #define SERIAL_NUM_FROM_BARCODE */

#if defined(CONFIG_MTK_USB_UNIQUE_SERIAL) || (defined(MTK_SECURITY_SW_SUPPORT) && defined(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT))
#define SERIALNO_LEN    38  /* from preloader */
char sn_buf[SN_BUF_LEN+1] = ""; /* will read from EFUSE_CTR_BASE */
#else
#define SERIALNO_LEN    38
char sn_buf[SN_BUF_LEN+1] = FASTBOOT_DEVNAME;
#endif

static struct udc_device surf_udc_device = {
	.vendor_id  = USB_VENDORID,
	.product_id = USB_PRODUCTID,
	.version_id = USB_VERSIONID,
	.manufacturer   = USB_MANUFACTURER,
	.product    = USB_PRODUCT_NAME,
};

char *cmdline_get(void)
{
	return g_cmdline;
}

bool cmdline_append(const char* append_string)
{
	//cmdline size check
	int append_len;
	append_len = strlen(append_string);

	if ((append_len + strlen(g_cmdline)) > CMDLINE_LEN - 2) { //careful about whitespace between two strings
		dprintf(CRITICAL,"[MBOOT] ERROR CMDLINE overflow\n");
		video_printf("[MBOOT] ERROR CMDLINE overflow\n");
		assert(0);
	}
	snprintf(g_cmdline, CMDLINE_LEN - 1, "%s %s", g_cmdline, append_string);
	return true;
}

bool cmdline_overwrite(const char* overwrite_string)
{
	int cmd_len;
	cmd_len = strlen(overwrite_string);
	if (cmd_len > CMDLINE_LEN - 1) {
		dprintf(CRITICAL,"[MBOOT] ERROR CMDLINE overflow\n");
		return false;
	}
	strncpy(g_cmdline, overwrite_string, CMDLINE_LEN - 1);
	return true;
}

void msg_header_error(char *img_name)
{
	dprintf(CRITICAL,"[MBOOT] Load '%s' partition Error\n", img_name);
	dprintf(CRITICAL,"\n*******************************************************\n");
	dprintf(CRITICAL,"ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR\n");
	dprintf(CRITICAL,"*******************************************************\n");
	dprintf(CRITICAL,"> If you use NAND boot\n");
	dprintf(CRITICAL,"> (1) %s is wrong !!!! \n", img_name);
	dprintf(CRITICAL,"> (2) please make sure the image you've downloaded is correct\n");
	dprintf(CRITICAL,"\n> If you use MSDC boot\n");
	dprintf(CRITICAL,"> (1) %s is not founded in SD card !!!! \n",img_name);
	dprintf(CRITICAL,"> (2) please make sure the image is put in SD card\n");
	dprintf(CRITICAL,"*******************************************************\n");
	dprintf(CRITICAL,"ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR\n");
	dprintf(CRITICAL,"*******************************************************\n");
	mtk_wdt_disable();
	mdelay(8000);
	mtk_arch_reset(1);
}

void msg_img_error(char *img_name)
{
	dprintf(CRITICAL,"[MBOOT] Load '%s' partition Error\n", img_name);
	dprintf(CRITICAL,"\n*******************************************************\n");
	dprintf(CRITICAL,"ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR\n");
	dprintf(CRITICAL,"*******************************************************\n");
	dprintf(CRITICAL,"> Please check kernel and rootfs in %s are both correct.\n",img_name);
	dprintf(CRITICAL,"*******************************************************\n");
	dprintf(CRITICAL,"ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR.ERROR\n");
	dprintf(CRITICAL,"*******************************************************\n");
	mtk_wdt_disable();
	mdelay(8000);
	mtk_arch_reset(1);
}

//*********
//* Notice : it's kernel start addr (and not include any debug header)
extern unsigned int g_kmem_off;

//*********
//* Notice : it's rootfs start addr (and not include any debug header)
extern unsigned int g_rmem_off;
#ifdef USE_ITS_BOOTIMG
extern unsigned int g_smem_off;
#endif
extern unsigned int g_rimg_sz;
extern int g_nr_bank;
extern unsigned int boot_time;
extern BOOT_ARGUMENT *g_boot_arg;
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
extern bool g_boot_reason_change;
#endif
extern int has_set_p2u;
extern unsigned int g_fb_base;
extern unsigned int g_fb_size;

void *g_fdt;

bool dtb_overlay(void* fdt, int size)
{
	char *base_buf;
	char *overlay_buf;
	struct fdt_header *blob;
	struct fdt_header *new_blob;
	size_t blob_len = 0, overlay_len = 0, new_blob_size = 0;
	int ret;
	g_fdt = fdt;
	#ifdef LK_PROFILING
	unsigned int time_overlay;
	#endif

	if (fdt == NULL) {
		dprintf(CRITICAL, "fdt is NULL\n");
		return FALSE;
	}
	if (size == 0) {
		dprintf(CRITICAL, "fdt size is zero\n");
		return FALSE;
	}

	overlay_buf = load_overlay_dtbo("odmdtbo", &overlay_len);
	if ((overlay_buf == NULL) || (overlay_len == 0)) {
		dprintf(CRITICAL, "load overlay dtbo failed !\n");
		return FALSE;
	}

	struct fdt_header *fdth = (struct fdt_header*)g_fdt;
	fdth->totalsize = cpu_to_fdt32(size);

	ret = fdt_open_into(g_fdt, g_fdt, size);
	if (ret) {
		dprintf(CRITICAL, "fdt_open_into failed \n");
		return FALSE;
	}
	ret = fdt_check_header(g_fdt);
	if (ret) {
		dprintf(CRITICAL, "fdt_check_header check failed !\n");
		return FALSE;
	}

	base_buf = fdt;
	blob_len = size;
	blob = fdt_install_blob(base_buf, blob_len);
	if (!blob) {
		dprintf(CRITICAL, "fdt_install_blob failed !\n");
		return FALSE;
	}

	dprintf(INFO, "blob_len len: 0x%x, overlay_len: 0x%x\n", blob_len, overlay_len);
	#ifdef LK_PROFILING
	time_overlay = get_timer(0);
	#endif

	ret = apply_overlay(blob, blob_len, overlay_buf, overlay_len, fdt);

	#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- Overlay takes %d ms -------- \n", (int)get_timer(time_overlay));
	#endif
	if (ret) {
		dprintf(CRITICAL, "apply_overlay failed !\n");
		return FALSE;
	}

	g_fdt = fdt;
	new_blob_size = fdt32_to_cpu(*(unsigned int *)(g_fdt+0x4));

	return TRUE;
}


static bool setup_fdt(void* fdt, int size)
{
	int ret;
	u32 addr = (u32)fdt;
	g_fdt = fdt;
#ifdef MTK_3LEVEL_PAGETABLE
	arch_mmu_map((uint64_t) ROUNDDOWN(addr, PAGE_SIZE), (uint32_t) ROUNDDOWN(addr, PAGE_SIZE),
	             MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(size, PAGE_SIZE));
#endif
	ret = fdt_open_into(g_fdt, g_fdt, size); //DTB maximum size is 2MB
	if (ret) return FALSE;
	ret = fdt_check_header(g_fdt);
	if (ret) return FALSE;
	return TRUE;
}

extern int mboot_recovery_load_raw_part(char *part_name, unsigned long *addr, unsigned int size);

bool bootimage_header_valid(char *buf)
{
	if (strncmp(buf, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0)
		return TRUE;
	else
		return FALSE;
}

void boot_get_os_version(unsigned *os_version)
{
	if (NULL == os_version)
		return;

	if (g_boot_hdr && (TRUE == bootimage_header_valid(g_boot_hdr))) {
		*os_version = g_boot_hdr->os_version;
	}
	else {
		*os_version = 0;
	}

	return;
}

void platform_parse_bootopt(u8* bootopt_str)
{
	int i = 0;
	int find = 0;
	/* search "bootopt" string */
	for (; i < (BOOT_ARGS_SIZE-0x16); i++) {
		if ( 0 == strncmp(&bootopt_str[i], "bootopt=", sizeof("bootopt=")-1)) {
			/* skip SMC, LK option */
			//Kernel option offset 0x12
			if ( 0 == strncmp(&bootopt_str[i+0x12], "64", sizeof("64")-1)) {
				g_is_64bit_kernel = 1;
				find = 1;
				break;
			}
			if ( 0 == strncmp(&bootopt_str[i+0x12], "32", sizeof("32")-1)) {
				find = 1;
				break;
			}
		}
	}
	if (!find) {
		dprintf(CRITICAL, "Warning! No bootopt info!\n");
		//No endless loop here because it will stuck when boot partition data lost
	}

}

#if defined(CFG_DTB_EARLY_LOADER_SUPPORT)
int bldr_load_dtb(char* boot_load_partition)
{
	char *ptr;
	char *dtb_sz;
	u32 dtb_kernel_addr;
	u32 zimage_addr,zimage_size,dtb_size,addr;
	u32 dtb_addr = 0;
	u32 tmp;
	u32 offset;
	unsigned char *magic;
	boot_img_hdr *p_boot_hdr;
	int ret = 0;

	ptr = malloc (DTB_MAX_SIZE);
	if (ptr == NULL) {
		dprintf(CRITICAL, "malloc failed!\n");
		return -1;
	}
	if (Check_RTC_Recovery_Mode() || unshield_recovery_detection() ) {
		boot_load_partition = "recovery";
	}
	//load boot hdr
	ret = mboot_recovery_load_raw_part(boot_load_partition, ptr, sizeof(boot_img_hdr)+0x800);
	if (ret < 0) {
		dprintf(CRITICAL, "mboot_recovery_load_raw_part failed, ret: 0x%x\n", ret);
		goto _end;
	}
	if (FALSE == bootimage_header_valid(ptr)) {
		ret = -1;
		goto _end;
	}

	p_boot_hdr = (void *)ptr;
	dtb_kernel_addr = p_boot_hdr->tags_addr;

	platform_parse_bootopt(p_boot_hdr->cmdline);

	if (!g_is_64bit_kernel) {
		//Offset into zImage    Value   Description
		//0x24  0x016F2818  Magic number used to identify this is an ARM Linux zImage
		//0x28  start address   The address the zImage starts at
		//0x2C  end address The address the zImage ends at
		/* bootimg header size is 0x800, the kernel img text is next to the header */
		zimage_addr = ptr + p_boot_hdr->page_size;
		zimage_size = *(unsigned int *)((unsigned int)zimage_addr+0x2c) - *(unsigned int *)((unsigned int)zimage_addr+0x28);
		//dtb_addr = (unsigned int)zimage_addr + zimage_size;
		offset = p_boot_hdr->page_size + zimage_size;
		tmp = ROUNDDOWN( p_boot_hdr->page_size + zimage_size, p_boot_hdr->page_size);
		ret = partition_read(boot_load_partition, (off_t)tmp, ptr, (size_t)DTB_MAX_SIZE);
		if (ret < 0) {
			dprintf(CRITICAL, "partition_read failed, ret: 0x%x\n", ret);
			goto _end;
		}

		dtb_addr = ptr + offset - tmp;
		dtb_size = fdt32_to_cpu(*(unsigned int *)(ptr + (offset - tmp) +0x4));
	}

	else {
		/* bootimg header size is 0x800, the kernel img text is next to the header */
		int i;

		zimage_size = p_boot_hdr->kernel_size;
		offset = p_boot_hdr->page_size + p_boot_hdr->kernel_size - (DTB_MAX_SIZE);
		tmp = ROUNDUP(offset, p_boot_hdr->page_size);
		ret = partition_read(boot_load_partition, (off_t)tmp, ptr, (size_t)DTB_MAX_SIZE);
		if (ret < 0) {
			dprintf(CRITICAL, "partition_read failed, ret: 0x%x\n", ret);
			goto _end;
		}

		dtb_addr = 0;
		dtb_size = 0;
		addr = ptr + (DTB_MAX_SIZE) - 4;
		for (i=0; i<(DTB_MAX_SIZE-4); i++, addr--) {
			//FDT_MAGIC 0xd00dfeed
			//dtb append after image.gz may not 4 byte alignment
			magic = (unsigned char *)addr;
			if ( *(magic + 3) == 0xED &&
			        *(magic + 2) == 0xFE &&
			        *(magic + 1) == 0x0D &&
			        *(magic + 0) == 0xD0) {
				dtb_addr = addr;

				break;
			}
		}

		if (dtb_addr == 0) {
			dprintf(CRITICAL, "can't find dtb\n");
			ret = -1;
			goto _end;
		}

		dtb_sz = (char *)(dtb_addr+4);
		dtb_size = *(dtb_sz) * 0x1000000 + *(dtb_sz+1)*0x10000 + *(dtb_sz+2) * 0x100 + *(dtb_sz+3);

		dprintf(CRITICAL, "Kernel(%d) zimage_size:0x%x,dtb_addr:0x%x(dtb_size:0x%x)\n",g_is_64bit_kernel, zimage_size, dtb_addr, dtb_size);
	}

#ifdef MTK_3LEVEL_PAGETABLE
	arch_mmu_map((uint64_t) ROUNDDOWN(dtb_kernel_addr, PAGE_SIZE), (uint32_t) ROUNDDOWN(dtb_kernel_addr, PAGE_SIZE), MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(dtb_size, PAGE_SIZE));
#endif
	dprintf(INFO, "Copy DTB from 0x%x to 0x%x(size: 0x%x)\n", dtb_addr, dtb_kernel_addr, dtb_size);
	memcpy(dtb_kernel_addr, (void *)dtb_addr, dtb_size);

	//placed setup_fdt() after bldr_load_dtb() due to it set the fdt_header-> totalsize
	ret = setup_fdt((void *)dtb_kernel_addr, DTB_MAX_SIZE);	//0x200000);
	dprintf(CRITICAL, "[LK] fdt setup addr:0x%x status:%d!!!\n", dtb_kernel_addr, ret);
	if (ret == FALSE) {
		dprintf(CRITICAL, "setup_fdt fail, ret: 0x%x!\n", ret);
		ret = -1;
	}

_end:
	free(ptr);
	return ret;
}
#endif

static void check_hibernation()
{
	int hibboot = 0;
#define TMPBUF_SIZE 128
	char tmpbuf[TMPBUF_SIZE];

	hibboot = get_env("hibboot") == NULL ? 0 : atoi(get_env("hibboot"));

	switch (g_boot_mode) {
		case RECOVERY_BOOT:
		case FACTORY_BOOT:
		case ALARM_BOOT:
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		case KERNEL_POWER_OFF_CHARGING_BOOT:
		case LOW_POWER_OFF_CHARGING_BOOT:
#endif
			goto SKIP_HIB_BOOT;
	}

	if (platform_skip_hibernation && platform_skip_hibernation())
		goto SKIP_HIB_BOOT;

	if (get_env("resume") != NULL) {
		if (1 == hibboot) {
			snprintf(tmpbuf, TMPBUF_SIZE, "%s%s", " resume=", get_env("resume"));
			cmdline_append(tmpbuf);
#if defined(MTK_MLC_NAND_SUPPORT)
			snprintf(tmpbuf, TMPBUF_SIZE, "%s%s", " ubi.mtd=", get_env("ubi_data_mtd"));
			cmdline_append(tmpbuf);
#endif
			//cmdline_append(" no_console_suspend");
		} else if (0 != hibboot)
			dprintf(CRITICAL,"resume = %s but hibboot = %s\n", get_env("resume"), get_env("hibboot"));
	} else {
		dprintf(CRITICAL,"resume = NULL \n");
	}

	return;

SKIP_HIB_BOOT:
	if (hibboot != 0)
		if (set_env("hibboot", "0") != 0)
			dprintf(CRITICAL,"lk_env hibboot set failed!!!\n");
	if (get_env("resume") != NULL)
		if (set_env("resume", '\0') != 0)
			dprintf(CRITICAL,"lk_evn resume set resume failed!!!\n");
}





#ifdef DEVICE_TREE_SUPPORT

void lk_jump64(u32 addr, u32 arg1, u32 arg2, u32 arg3)
{
	dprintf(CRITICAL,"\n[LK]jump to K64 0x%x\n", addr);
	dprintf(INFO,"smc jump\n");
	jumparch64_smc(addr, arg1, arg2, arg3);
	dprintf(CRITICAL,"Do nothing now! wait for SMC done\n");
	while (1)
		;
}

void memcpy_u8(unsigned char *dest, unsigned char *src, unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++) {
		*(dest + i) = *(src + i);
	}
}

extern bool decompress_kernel(unsigned char *in, void *out, int inlen, int outlen);

#if defined(MTK_GOOGLE_TRUSTY_SUPPORT)
int trusty_dts_append(void *fdt)
{
	int offset, ret = 0;
	int nodeoffset = 0;
	unsigned int trusty_reserved_mem[4] = {0};

	offset = fdt_path_offset(fdt, "/reserved-memory");
	nodeoffset = fdt_add_subnode(fdt, offset, "trusty-reserved-memory");
	if (nodeoffset < 0) {
		dprintf(CRITICAL, "Warning: can't add trusty-reserved-memory node in device tree\n");
		return 1;
	}
	ret = fdt_setprop_string(fdt, nodeoffset, "compatible", "mediatek,trusty-reserved-memory");
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add trusty compatible property in device tree\n");
		return 1;
	}
	ret = fdt_setprop(fdt, nodeoffset, "no-map", NULL, 0);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add trusty no-map property in device tree\n");
		return 1;
	}
	trusty_reserved_mem[0] = 0;
	trusty_reserved_mem[1] = (u32)cpu_to_fdt32(g_boot_arg->tee_reserved_mem.start);
	trusty_reserved_mem[2] = 0;
	trusty_reserved_mem[3] = (u32)cpu_to_fdt32(g_boot_arg->tee_reserved_mem.size);
	ret = fdt_setprop(fdt, nodeoffset, "reg", trusty_reserved_mem, sizeof(unsigned int)*4);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add trusty reg property in device tree\n");
		return 1;
	}
	dprintf(CRITICAL,"trusty-reserved-memory is appended (0x%llx, 0x%llx)\n",
	        g_boot_arg->tee_reserved_mem.start, g_boot_arg->tee_reserved_mem.size);

	return ret;
}
#endif

extern void Debug_log_EMI_MPU(void) __attribute__((weak));
int boot_linux_fdt(void *kernel, unsigned *tags,
                   unsigned machtype,
                   void *ramdisk, unsigned ramdisk_size)
{
	void *fdt = tags;
	int ret;
	int offset;
#define TMPBUF_SIZE 128
	char tmpbuf[TMPBUF_SIZE];
#ifdef MTK_DTBO_FEATURE
	bool rtn;
#endif
	dt_dram_info mem_reg_property[128];

	int i;
	void (*entry)(unsigned,unsigned,unsigned*) = kernel;
	unsigned int lk_t = 0;
	unsigned int pl_t = 0;
	unsigned int boot_reason = 0;
	char buf[FDT_BUFF_SIZE+8], *ptr;
	int decompress_outbuf_size = 0x1C00000;
#ifndef USE_ITS_BOOTIMG
	unsigned int zimage_size;
	unsigned int dtb_addr=0;
	unsigned int dtb_size;

	if (g_is_64bit_kernel) {
		unsigned char *magic;
		unsigned int addr;
		unsigned int zimage_addr = (unsigned int)target_get_scratch_address();
		/* to boot k64*/
		dprintf(INFO,"64 bits kernel\n");

		dprintf(INFO,"g_boot_hdr=%p\n", g_boot_hdr);
		dprintf(INFO,"g_boot_hdr->kernel_size=0x%08x\n", g_boot_hdr->kernel_size);
		zimage_size = (g_boot_hdr->kernel_size);

		if (g_boot_hdr->kernel_addr & 0x7FFFF) {
			dprintf(CRITICAL,"64 bit kernel can't boot at g_boot_hdr->kernel_addr=0x%08x\n",g_boot_hdr->kernel_addr);
			dprintf(CRITICAL,"Please check your bootimg setting\n");
			while (1)
				;
		}

		addr = (unsigned int)(zimage_addr + zimage_size);


		for (dtb_size = 0; dtb_size < zimage_size; dtb_size++, addr--) {
			//FDT_MAGIC 0xd00dfeed ... dtf
			//dtb append after image.gz may not 4 byte alignment
			magic = (unsigned char *)addr;
			if ( *(magic + 3) == 0xED &&
			        *(magic + 2) == 0xFE &&
			        *(magic + 1) == 0x0D &&
			        *(magic + 0) == 0xD0)

			{
				dtb_addr = addr;
				dprintf(INFO,"get dtb_addr=0x%08x, dtb_size=0x%08x\n", dtb_addr, dtb_size);
				dprintf(INFO,"copy dtb, fdt=0x%08x\n", (unsigned int)fdt);

				//fix size to 4 byte alignment
				dtb_size = (dtb_size + 0x3) & (~0x3);
#if CFG_DTB_EARLY_LOADER_SUPPORT
				//skip dtb copy, we load dtb in preloader
#else
				memcpy_u8(fdt, (void *)dtb_addr, dtb_size);
#endif
				dtb_addr = (unsigned int)fdt;

				break;
			}
		}

		if (dtb_size != zimage_size) {
			zimage_size -= dtb_size;
		} else {
			dprintf(CRITICAL,"can't find device tree\n");
		}

		dprintf(INFO,"zimage_addr=0x%08x, zimage_size=0x%08x\n", zimage_addr, zimage_size);
		dprintf(INFO,"decompress kernel image...");

		/* for 64bit decompreesed size.
		 * LK start: 0x41E00000, Kernel Start: 0x40080000
		 * Max is 0x41E00000 - 0x40080000 = 0x1D80000.
		 * using 0x1C00000=28MB for decompressed kernel image size */
		#ifdef KERNEL_DECOMPRESS_SIZE
			decompress_outbuf_size = KERNEL_DECOMPRESS_SIZE;
		#endif
		if (decompress_kernel((unsigned char *)zimage_addr, (void *)g_boot_hdr->kernel_addr, (int)zimage_size, (int)decompress_outbuf_size)) {
			dprintf(CRITICAL,"decompress kernel image fail!!!\n");
			while (1)
				;
		}
	} else {
		dprintf(INFO,"32 bits kernel\n");
		zimage_size = *(unsigned int *)((unsigned int)kernel+0x2c) - *(unsigned int *)((unsigned int)kernel+0x28);
		dtb_addr = (unsigned int)kernel + zimage_size;
	}

	if (fdt32_to_cpu(*(unsigned int *)dtb_addr) == FDT_MAGIC) {
		dtb_size = fdt32_to_cpu(*(unsigned int *)(dtb_addr+0x4));
	} else {
		dprintf(CRITICAL,"Can't find device tree. Please check your kernel image\n");
		while (1)
			;
		//dprintf(CRITICAL,"use default device tree\n");
		//dtb_addr = (unsigned int)&device_tree;
		//dtb_size = device_tree_size;
	}
	dprintf(INFO,"dtb_addr = 0x%08X, dtb_size = 0x%08X\n", dtb_addr, dtb_size);

	if (((unsigned int)fdt + dtb_size) > g_fb_base) {
		dprintf(CRITICAL,"[ERROR] dtb end address (0x%08X) is beyond the memory (0x%08X).\n", (unsigned int)fdt+dtb_size, g_fb_base);
		return FALSE;
	}
#if CFG_DTB_EARLY_LOADER_SUPPORT
	//skip dtb copy, we load dtb in preloader
#else
	memcpy(fdt, (void *)dtb_addr, dtb_size);
#endif

#else // USE_ITS_BOOTIMG
	if (g_smem_off && !strncmp(((struct dtb_header *)g_smem_off)->magic, DTB_MAGIC, sizeof(DTB_MAGIC))) {
		unsigned int platform_id = 0, soc_rev = 0, version_id = 0; // These should be got from IDME???

		// multi-dtb found
		struct dtb_entry *dtb_ptr = (struct dtb_entry *)(g_smem_off + sizeof(struct dtb_header));
		for (i=0; i<(int)((struct dtb_header *)g_smem_off)->num_of_dtbs; i++) {
			// Compare platform/version/soc revision id with current dtb entry
			if (dtb_ptr->platform_id == platform_id && dtb_ptr->soc_rev == soc_rev && dtb_ptr->version_id == version_id) {
				memcpy(fdt, (void *)(g_smem_off + dtb_ptr->offset), dtb_ptr->size);
				break;
			}
			// Move to next dtb entry
			dtb_ptr ++;
		}
	} else if (g_smem_off)
		memcpy(fdt, (void *)g_smem_off, device_tree_size);
	else
		memcpy(fdt, (void *)&device_tree, device_tree_size);
#endif

	strcpy(&buf[FDT_BUFF_SIZE], FDT_BUFF_PATTERN);

#if CFG_DTB_EARLY_LOADER_SUPPORT
	//skip dtb setup, we setup dtb in platform.c
#else
	ret = setup_fdt(fdt, DTB_MAX_SIZE);	//MIN(0x100000, (g_fb_base-(unsigned int)fdt)));
	if (ret == FALSE) return FALSE;
#endif


#ifdef MTK_DTBO_FEATURE
	rtn = dtb_overlay(fdt, dtb_size);
	dtb_size = fdt32_to_cpu(*(unsigned int *)(fdt+0x4));
#endif

	extern int target_fdt_jtag(void *fdt)__attribute__((weak));
	if (target_fdt_jtag) {
		target_fdt_jtag(fdt);
	}

	extern int target_fdt_model(void *fdt)__attribute__((weak));
	if (target_fdt_model) {
		target_fdt_model(fdt);
	}

	extern int target_fdt_cpus(void *fdt)__attribute__((weak));
	if (target_fdt_cpus) {
		target_fdt_cpus(fdt);
	}

#ifndef MACH_FPGA
	/* MD standard alone partition feature start */
	/* NOTE: This MUST before setup_mem_property_use_mblock_info !!!!!! */
	if (g_boot_mode != RECOVERY_BOOT) {
		extern void load_modem_image(void)__attribute__((weak));
		if (load_modem_image) {
			load_modem_image();
		} else {
			dprintf(CRITICAL,"[ccci] modem standalone not support\n");
		}
	} else {
		dprintf(CRITICAL,"[ccci] recovery mode, no need load modem\n");
	}
	/* MD standard alone partition feature end */
#endif

#ifdef MTK_SECURITY_ANTI_ROLLBACK
	ret = sec_otp_ver_update();
	imgver_not_sync_warning(g_boot_arg->pl_imgver_status,ret);
#endif

	extern int setup_mem_property_use_mblock_info(dt_dram_info *, size_t) __attribute__((weak));
	if (setup_mem_property_use_mblock_info) {
		ret = setup_mem_property_use_mblock_info(
		          &mem_reg_property[0],
		          sizeof(mem_reg_property)/sizeof(dt_dram_info));
		if (ret) return FALSE;
	} else {
		for (i = 0; i < g_nr_bank; ++i) {
			unsigned int fb_size = (i == g_nr_bank-1) ? g_fb_size : 0;

#ifndef MTK_LM_MODE
			mem_reg_property[i].start_hi = cpu_to_fdt32(0);
			mem_reg_property[i].start_lo = cpu_to_fdt32(bi_dram[i].start);
			mem_reg_property[i].size_hi = cpu_to_fdt32(0);
			mem_reg_property[i].size_lo = cpu_to_fdt32(bi_dram[i].size-fb_size);

#else
			mem_reg_property[i].start_hi = cpu_to_fdt32(bi_dram[i].start>>32);
			mem_reg_property[i].start_lo = cpu_to_fdt32(bi_dram[i].start);
			mem_reg_property[i].size_hi = cpu_to_fdt32((bi_dram[i].size-fb_size)>>32);
			mem_reg_property[i].size_lo = cpu_to_fdt32(bi_dram[i].size-fb_size);

#endif
			dprintf(INFO," mem_reg_property[%d].start_hi = 0x%08X\n", i, mem_reg_property[i].start_hi);
			dprintf(INFO," mem_reg_property[%d].start_lo = 0x%08X\n", i, mem_reg_property[i].start_lo);
			dprintf(INFO," mem_reg_property[%d].size_hi = 0x%08X\n", i, mem_reg_property[i].size_hi);
			dprintf(INFO," mem_reg_property[%d].size_lo = 0x%08X\n", i, mem_reg_property[i].size_lo);
		}
	}

	extern int target_fdt_dram_dummy_read(void *fdt, unsigned int rank_num)__attribute__((weak));
	if (target_fdt_dram_dummy_read) {
		ret = target_fdt_dram_dummy_read(fdt, g_nr_bank);
		if (ret)
			dprintf(CRITICAL,"ERROR: DRAM dummy read address incorrect\n");
	}

	offset = fdt_path_offset(fdt, "/memory");
	extern int get_mblock_num(void) __attribute__((weak));
	ret = fdt_setprop(fdt, offset, "reg", mem_reg_property,
	                  ((int)get_mblock_num? get_mblock_num(): g_nr_bank ) * sizeof(dt_dram_info));
	if (ret) return FALSE;

	if (platform_atag_append) {
		ret = platform_atag_append(fdt);
		if (ret) return FALSE;
	}
#ifdef MBLOCK_LIB_SUPPORT
	ret = fdt_memory_append(fdt);
	if (ret) return FALSE;
#endif

#if defined(MTK_GOOGLE_TRUSTY_SUPPORT)
	ret = trusty_dts_append(fdt);
	if (ret) return FALSE;
#endif

	offset = fdt_path_offset(fdt, "/chosen");
	ret = fdt_setprop_cell(fdt, offset, "linux,initrd-start",(unsigned int) ramdisk);
	if (ret) return FALSE;
	ret = fdt_setprop_cell(fdt, offset, "linux,initrd-end", (unsigned int)ramdisk + ramdisk_size);
	if (ret) return FALSE;

	ptr = (char *)target_atag_boot((unsigned *)buf);
	ret = fdt_setprop(fdt, offset, "atag,boot", buf, ptr - buf);
	if (ret) return FALSE;

#if defined(MTK_DLPT_SUPPORT)
	ptr = (char *)target_atag_imix_r((unsigned *)buf);
	ret = fdt_setprop(fdt, offset, "atag,imix_r", buf, ptr - buf);
	if (ret) return FALSE;
#endif
	snprintf(buf, FDT_BUFF_SIZE, "%d", fg_swocv_v);
	ptr = buf + strlen(buf);
	ret = fdt_setprop(fdt, offset, "atag,fg_swocv_v", buf, ptr - buf);
	dprintf(CRITICAL,"fg_swocv_v buf [%s], [0x%x:0x%x:%d]\n", buf, (unsigned)buf, (unsigned)ptr, ptr - buf);

	snprintf(buf, FDT_BUFF_SIZE, "%d", fg_swocv_i);
	ptr = buf + strlen(buf);
	ret = fdt_setprop(fdt, offset, "atag,fg_swocv_i", buf, ptr - buf);
	dprintf(CRITICAL,"fg_swocv_i buf [%s], [0x%x:0x%x:%d]\n", buf, (unsigned)buf, (unsigned)ptr, ptr - buf);

	snprintf(buf, FDT_BUFF_SIZE, "%d", shutdown_time);
	ptr = buf + strlen(buf);
	ret = fdt_setprop(fdt, offset, "atag,shutdown_time", buf, ptr - buf);
	dprintf(CRITICAL,"shutdown_time buf [%s], [0x%x:0x%x:%d]\n", buf, (unsigned)buf, (unsigned)ptr, ptr - buf);

	snprintf(buf, FDT_BUFF_SIZE, "%d", boot_voltage);
	ptr = buf + strlen(buf);
	ret = fdt_setprop(fdt, offset, "atag,boot_voltage", buf, ptr - buf);
	dprintf(CRITICAL,"boot_voltage buf [%s], [0x%x:0x%x:%d]\n", buf, (unsigned)buf, (unsigned)ptr, ptr - buf);

	ptr = (char *)target_atag_mem((unsigned *)buf);
	ret = fdt_setprop(fdt, offset, "atag,mem", buf, ptr - buf);
	if (ret) return FALSE;

	if (target_atag_partition_data) {
		ptr = (char *)target_atag_partition_data((unsigned *)buf);
		if (ptr != buf) {
			ret = fdt_setprop(fdt, offset, "atag,mem", buf, ptr - buf);
			if (ret) return FALSE;
		}
	}
#if !(defined(MTK_UFS_BOOTING) || defined(MTK_EMMC_SUPPORT))
	if (target_atag_nand_data) {
		ptr = (char *)target_atag_nand_data((unsigned *)buf);
		if (ptr != buf) {
			ret = fdt_setprop(fdt, offset, "atag,mem", buf, ptr - buf);
			if (ret) return FALSE;
		}
	}
#endif
	extern unsigned int *target_atag_vcore_dvfs(unsigned *ptr)__attribute__((weak));
	if (target_atag_vcore_dvfs) {
		ptr = (char *)target_atag_vcore_dvfs((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,vcore_dvfs", buf, ptr - buf);
		if (ret) return FALSE;
	} else {
		dprintf(CRITICAL,"Not Support VCORE DVFS\n");
	}

	//some platform might not have this function, use weak reference for
	extern unsigned *target_atag_dfo(unsigned *ptr)__attribute__((weak));
	if (target_atag_dfo) {
		ptr = (char *)target_atag_dfo((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,dfo", buf, ptr - buf);
		if (ret) return FALSE;
	}

	if (g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT || g_boot_mode == ATE_FACTORY_BOOT || g_boot_mode == FACTORY_BOOT) {
		ptr = (char *)target_atag_meta((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,meta", buf, ptr - buf);
		if (ret) return FALSE;
		//sprintf(cmdline, "%s%s", cmdline, " rdinit=sbin/multi_init");
		cmdline_append("rdinit=sbin/multi_init");
		unsigned int meta_com_id = g_boot_arg->meta_com_id;
		if ((meta_com_id & 0x0001) != 0) {
			cmdline_append("androidboot.usbconfig=1");
		} else {
			cmdline_append("androidboot.usbconfig=0");
		}
		if (g_boot_mode == META_BOOT) {
			if ((meta_com_id & 0x0002) != 0) {
				cmdline_append("androidboot.mblogenable=0");
			} else {
				cmdline_append("androidboot.mblogenable=1");
			}
		}
	}

	ptr = (char *)target_atag_devinfo_data((unsigned *)buf);
	ret = fdt_setprop(fdt, offset, "atag,devinfo", buf, ptr - buf);
	if (ret) return FALSE;

#ifndef MACH_FPGA_NO_DISPLAY
	ptr = (char *)target_atag_videolfb((unsigned *)buf);
	ret = fdt_setprop(fdt, offset, "atag,videolfb", buf, ptr - buf);
	if (ret) return FALSE;
#else

	extern int mt_disp_config_frame_buffer(void *fdt)__attribute__((weak));
	if (mt_disp_config_frame_buffer) {
		ret = mt_disp_config_frame_buffer(fdt);
	}

#endif

	extern int lastpc_decode(void *fdt)__attribute__((weak));
	if (lastpc_decode) {
		ret = lastpc_decode(fdt);
		if (ret) return FALSE;
	}

	if (target_atag_mdinfo) {
		ptr = (char *)target_atag_mdinfo((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,mdinfo", buf, ptr - buf);
		if (ret) return FALSE;
	} else {
		dprintf(CRITICAL,"DFO_MODEN_INFO Only support in MT6582/MT6592\n");
	}

	extern unsigned int *update_md_hdr_info_to_dt(unsigned int *ptr, void *fdt)__attribute__((weak));
	if (update_md_hdr_info_to_dt) {
		ptr = (char *)update_md_hdr_info_to_dt((unsigned int *)buf, fdt);
		ret = fdt_setprop(fdt, offset, "ccci,modem_info", buf, ptr - buf);
		if (ret)
			return FALSE;
		else
			dprintf(CRITICAL,"[ccci] create modem mem info DT OK\n");
	} else {
		dprintf(CRITICAL,"[ccci] modem mem info not support\n");
	}

	extern unsigned int *update_lk_arg_info_to_dt(unsigned int *ptr, void *fdt)__attribute__((weak));
	if (update_lk_arg_info_to_dt) {
		ptr = (char *)update_lk_arg_info_to_dt((unsigned int *)buf, fdt);
		ret = fdt_setprop(fdt, offset, "ccci,modem_info_v2", buf, ptr - buf);
		if (ret)
			return FALSE;
		else
			dprintf(CRITICAL,"[ccci] create modem arguments info DT OK\n");
	} else {
		dprintf(CRITICAL,"[ccci] modem mem arguments info using v1\n");
	}

	extern unsigned int *target_atag_ptp(unsigned *ptr)__attribute__((weak));
	if (target_atag_ptp) {
		ptr = (char *)target_atag_ptp((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,ptp", buf, ptr - buf);
		if (ret)
			return FALSE;
		else
			dprintf(CRITICAL,"Create PTP DT OK\n");
	} else {
		dprintf(CRITICAL,"PTP_INFO Only support in MT6795\n");
	}

	extern unsigned int *target_atag_extbuck_fan53526(unsigned *ptr)__attribute__((weak));
	if (target_atag_extbuck_fan53526) {
		ptr = (char *)target_atag_extbuck_fan53526((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,extbuck_fan53526", buf, ptr - buf);
		if (ret)
			return FALSE;
		else
			dprintf(CRITICAL,"Create EXTBUCK_FAN53526 DT OK\n");
	} else {
		dprintf(CRITICAL,"EXTBUCK_FAN53526 not support\n");
	}

	extern unsigned int *target_atag_masp_data(unsigned *ptr)__attribute__((weak));
	if (target_atag_masp_data) {
		ptr = (char *)target_atag_masp_data((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "atag,masp", buf, ptr - buf);
		if (ret)
			return FALSE;
		else
			dprintf(CRITICAL,"create masp atag OK\n");
	} else {
		dprintf(CRITICAL,"masp atag not support in this platform\n");
	}

	extern unsigned int *target_atag_tee(unsigned *ptr)__attribute__((weak));
	if (target_atag_tee) {
		ptr = (char *)target_atag_tee((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "tee_reserved_mem", buf, ptr - buf);
		if (ret) return FALSE;
	} else {
		dprintf(CRITICAL,"tee_reserved_mem not supported\n");
	}

	extern unsigned int *target_atag_isram(unsigned *ptr)__attribute__((weak));
	if (target_atag_isram) {
		ptr = (char *)target_atag_isram((unsigned *)buf);
		ret = fdt_setprop(fdt, offset, "non_secure_sram", buf, ptr - buf);
		if (ret) return FALSE;
	} else {
		dprintf(CRITICAL,"non_secure_sram not supported\n");
	}


	if (!has_set_p2u) {
#ifdef USER_BUILD
		cmdline_append("printk.disable_uart=1");
#else
		cmdline_append(" printk.disable_uart=0 ddebug_query=\"file *mediatek* +p ; file *gpu* =_\"");
#endif

		/*Append pre-loader boot time to kernel command line*/
		pl_t = g_boot_arg->boot_time;
		snprintf(tmpbuf, TMPBUF_SIZE, "%s%d", "bootprof.pl_t=", pl_t);
		cmdline_append(tmpbuf);
		/*Append lk boot time to kernel command line*/
		lk_t = ((unsigned int)get_timer(boot_time));
		snprintf(tmpbuf, TMPBUF_SIZE, "%s%d", "bootprof.lk_t=", lk_t);
		cmdline_append(tmpbuf);

#ifdef LK_PROFILING
		dprintf(CRITICAL,"[PROFILE] ------- boot_time takes %d ms -------- \n", lk_t);
#endif
	}
	/*Append pre-loader boot reason to kernel command line*/
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (g_boot_reason_change) {
		boot_reason = 4;
	} else
#endif
	{
		boot_reason = g_boot_arg->boot_reason;
	}
	snprintf(tmpbuf, TMPBUF_SIZE, "%s%d", "boot_reason=", boot_reason);
	cmdline_append(tmpbuf);

	/* Append androidboot.serialno=xxxxyyyyzzzz in cmdline */
	snprintf(tmpbuf, TMPBUF_SIZE, "%s%s", "androidboot.serialno=", sn_buf);
	cmdline_append(tmpbuf);

	snprintf(tmpbuf, TMPBUF_SIZE, "%s%s",  "androidboot.bootreason=", g_boot_reason[boot_reason]);
	cmdline_append(tmpbuf);


	extern unsigned int *target_commandline_force_gpt(char *cmd)__attribute__((weak));
	if (target_commandline_force_gpt) {
		target_commandline_force_gpt((char *)cmdline_get());
	}

	extern void ccci_append_tel_fo_setting(char *cmdline)__attribute__((weak));
	if (ccci_append_tel_fo_setting) {
		ccci_append_tel_fo_setting((char *)cmdline_get());
	}

#ifndef USER_BUILD
	cmdline_append("initcall_debug=1");
#endif

	extern int dfd_set_base_addr(void *fdt)__attribute__((weak));
	if (dfd_set_base_addr) {
		ret = dfd_set_base_addr(fdt);
		if (ret) {
			dprintf(CRITICAL,"[DFD] failed to get base address (%d)\n", ret);
		}
	}

	extern unsigned int get_usb2jtag(void) __attribute__((weak));
	extern unsigned int set_usb2jtag(unsigned int en)  __attribute__((weak));
	if (get_usb2jtag) {
		if (get_usb2jtag() == 1)
			cmdline_append("usb2jtag_mode=1");
		else
			cmdline_append("usb2jtag_mode=0");
	}

	check_hibernation();

	/*DTS memory will be modified during lk boot process
	 * so we need to put the cmdline in the last mile*/
	mrdump_append_cmdline();


	ptr = (char *)target_atag_commmandline((unsigned *)buf, (char *)cmdline_get());
	ret = fdt_setprop(fdt, offset, "atag,cmdline", buf, ptr - buf);
	if (ret) return FALSE;

	ret = fdt_setprop_string(fdt, offset, "bootargs", (char *)cmdline_get());
	if (ret) return FALSE;

	extern int *target_fdt_firmware(void *fdt, char *serialno)__attribute__((weak));
	if (target_fdt_firmware) {
		target_fdt_firmware(fdt, sn_buf);
	}

	/* This depends on target_fdt_firmware, must after target_fdt_firmware */
	extern int update_md_opt_to_fdt_firmware(void *fdt)__attribute__((weak));
	if (update_md_opt_to_fdt_firmware) {
		update_md_opt_to_fdt_firmware(fdt);
	}

	ret = fdt_pack(fdt);
	if (ret) return FALSE;

	dprintf(CRITICAL,"booting linux @ %p, ramdisk @ %p (%d)\n",
	        kernel, ramdisk, ramdisk_size);

	if (strcmp(&buf[FDT_BUFF_SIZE], FDT_BUFF_PATTERN) != 0) {
		dprintf(CRITICAL,"ERROR: fdt buff overflow\n");
		return FALSE;
	}

	enter_critical_section();
	/* do any platform specific cleanup before kernel entry */
	platform_uninit();
#ifdef HAVE_CACHE_PL310
	l2_disable();
#endif

	arch_disable_cache(UCACHE);
	arch_disable_mmu();

#ifndef MACH_FPGA
	extern void platform_sec_post_init(void)__attribute__((weak));
	if (platform_sec_post_init) {
		platform_sec_post_init();
	}
#endif

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	/*Prevent the system jumps to Kernel if we unplugged Charger/USB before*/
	if (kernel_charging_boot() == -1) {
		dprintf(CRITICAL,"[%s] Unplugged Usb/Charger in Kernel Charging Mode Before Jumping to Kernel, Power Off\n", __func__);
#ifndef NO_POWER_OFF
		mt6575_power_off();
#endif

	}
	if (kernel_charging_boot() == 1) {
		if (pmic_detect_powerkey()) {
			dprintf(CRITICAL,"[%s] PowerKey Pressed in Kernel Charging Mode Before Jumping to Kernel, Reboot Os\n", __func__);
			//mt65xx_backlight_off();
			//mt_disp_power(0);
			mtk_arch_reset(1);
		}
	}
#endif
	dprintf(CRITICAL,"DRAM Rank :%d\n", g_nr_bank);
	for (i = 0; i < g_nr_bank; i++) {
#ifndef MTK_LM_MODE
		dprintf(CRITICAL,"DRAM Rank[%d] Start = 0x%x, Size = 0x%x\n", i, (unsigned int)bi_dram[i].start, (unsigned int)bi_dram[i].size);
#else
		dprintf(CRITICAL,"DRAM Rank[%d] Start = 0x%llx, Size = 0x%llx\n", i, bi_dram[i].start, bi_dram[i].size);
#endif
	}

#ifdef MBLOCK_LIB_SUPPORT
	mblock_show_info();
#endif

	dprintf(CRITICAL,"cmdline: %s\n", (char *)cmdline_get());
	dprintf(CRITICAL,"lk boot time = %d ms\n", lk_t);
	dprintf(CRITICAL,"lk boot mode = %d\n", g_boot_mode);
	dprintf(CRITICAL,"lk boot reason = %s\n", g_boot_reason[boot_reason]);
	dprintf(CRITICAL,"lk finished --> jump to linux kernel %s\n\n", g_is_64bit_kernel ? "64Bit" : "32Bit");
	if (Debug_log_EMI_MPU)
		Debug_log_EMI_MPU();

	if (g_is_64bit_kernel) {
		lk_jump64((u32)entry, (u32)tags, 0, KERNEL_64BITS);
	} else {
#ifdef MTK_SMC_K32_SUPPORT
		lk_jump64((u32)entry, (u32)machtype, (u32)tags, KERNEL_32BITS);
#else
		entry(0, machtype, tags);
#endif
	}
	while (1);
	return 0;
}



#endif // DEVICE_TREE_SUPPORT

void boot_linux(void *kernel, unsigned *tags,
                unsigned machtype,
                void *ramdisk, unsigned ramdisk_size)
{
	int i;
	unsigned *ptr = tags;
	void (*entry)(unsigned,unsigned,unsigned*) = kernel;
	unsigned int lk_t = 0;
	unsigned int pl_t = 0;
	unsigned int boot_reason = 0;
	int ret = 0;

#define TMPBUF_SIZE 128
	char tmpbuf[TMPBUF_SIZE];

#ifdef DEVICE_TREE_SUPPORT
	boot_linux_fdt((void *)kernel, (unsigned *)tags,
	               machtype,
	               (void *)ramdisk, ramdisk_size);

	while (1) ;
#endif

#ifdef MTK_SECURITY_ANTI_ROLLBACK
	ret = sec_otp_ver_update();
	imgver_not_sync_warning(g_boot_arg->pl_imgver_status,ret);
#endif


	/* CORE */
	*ptr++ = 2;
	*ptr++ = 0x54410001;

	ptr = target_atag_boot(ptr);
	ptr = target_atag_mem(ptr);

	if (target_atag_partition_data) {
		ptr = target_atag_partition_data(ptr);
	}
#if !(defined(MTK_UFS_BOOTING) || defined(MTK_EMMC_SUPPORT))
	if (target_atag_nand_data) {
		ptr = target_atag_nand_data(ptr);
	}
#endif
	//some platform might not have this function, use weak reference for
	extern unsigned *target_atag_dfo(unsigned *ptr)__attribute__((weak));
	if (target_atag_dfo) {
		ptr = target_atag_dfo(ptr);
	}

	if (g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT || g_boot_mode == ATE_FACTORY_BOOT || g_boot_mode == FACTORY_BOOT) {
		ptr = target_atag_meta(ptr);
		cmdline_append("rdinit=sbin/multi_init");
		unsigned int meta_com_id = g_boot_arg->meta_com_id;
		if ((meta_com_id & 0x0001) != 0) {
			cmdline_append("androidboot.usbconfig=1");
		} else {
			cmdline_append("androidboot.usbconfig=0");
		}
		if (g_boot_mode == META_BOOT) {
			if ((meta_com_id & 0x0002) != 0) {
				cmdline_append("androidboot.mblogenable=0");
			} else {
				cmdline_append("androidboot.mblogenable=1");
			}
		}
	}

	ptr = target_atag_devinfo_data(ptr);

	/*Append pre-loader boot time to kernel command line*/
	pl_t = g_boot_arg->boot_time;
	snprintf(tmpbuf, TMPBUF_SIZE, "%s%d", "bootprof.pl_t=", pl_t);
	cmdline_append(tmpbuf);

	/*Append lk boot time to kernel command line*/
	lk_t = ((unsigned int)get_timer(boot_time));
	snprintf(tmpbuf, TMPBUF_SIZE, "%s%d", "bootprof.lk_t=", lk_t);
	cmdline_append(tmpbuf);

#ifdef LK_PROFILING
	printf("[PROFILE] ------- boot_time takes %d ms -------- \n", lk_t);
#endif
	if (!has_set_p2u) {
#ifdef USER_BUILD
		cmdline_append("printk.disable_uart=1");
#else
		cmdline_append(" printk.disable_uart=0 ddebug_query=\"file *mediatek* +p ; file *gpu* =_\"");
#endif
	}
	/*Append pre-loader boot reason to kernel command line*/
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (g_boot_reason_change) {
		boot_reason = 4;
	} else
#endif
	{
		boot_reason = g_boot_arg->boot_reason;
	}
	snprintf(tmpbuf, TMPBUF_SIZE, "%s%d", "boot_reason=", boot_reason);
	cmdline_append(tmpbuf);

	/* Append androidboot.serialno=xxxxyyyyzzzz in cmdline */
	snprintf(tmpbuf, TMPBUF_SIZE, "%s%s", "androidboot.serialno=", sn_buf);
	cmdline_append(tmpbuf);

	snprintf(tmpbuf, TMPBUF_SIZE, "%s%s",  "androidboot.bootreason=", g_boot_reason[boot_reason]);
	cmdline_append(tmpbuf);

	check_hibernation();
	ptr = target_atag_commmandline(ptr, (char *)cmdline_get());
	ptr = target_atag_initrd(ptr,(unsigned long) ramdisk, ramdisk_size);
	ptr = target_atag_videolfb(ptr);

	extern unsigned int *target_atag_mdinfo(unsigned *ptr)__attribute__((weak));
	if (target_atag_mdinfo) {
		ptr = target_atag_mdinfo(ptr);
	} else {
		dprintf(CRITICAL,"DFO_MODEN_INFO Only support in MT6582/MT6592\n");
	}

	extern unsigned int *target_atag_masp_data(unsigned *ptr)__attribute__((weak));
	if (target_atag_masp_data) {
		ptr = (char *)target_atag_masp_data((unsigned *)ptr);
	} else {
		dprintf(CRITICAL,"masp atag not support in this platform\n");
	}

	/* END */
	*ptr++ = 0;
	*ptr++ = 0;

#if 0
	dprintf(CRITICAL,"atag start:0x%08X, end:0x%08X\n", tags, ptr);
	for (unsigned int*scan=tags, i=0; scan!=ptr; scan++, i++) {
		dprintf(CRITICAL,"0x%08X%c", *scan, (i%4==0)?'\n':'\t');
	}
	dprintf(CRITICAL,"\n");
#endif

	dprintf(CRITICAL,"booting linux @ %p, ramdisk @ %p (%d)\n",
	        kernel, ramdisk, ramdisk_size);

	enter_critical_section();
	/* do any platform specific cleanup before kernel entry */
	platform_uninit();
#ifdef HAVE_CACHE_PL310
	l2_disable();
#endif

	arch_disable_cache(UCACHE);
	arch_disable_mmu();

	extern void platform_sec_post_init(void)__attribute__((weak));
	if (platform_sec_post_init) {
		platform_sec_post_init();
	}

	arch_uninit();

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	/*Prevent the system jumps to Kernel if we unplugged Charger/USB before*/
	if (kernel_charging_boot() == -1) {
		dprintf(CRITICAL,"[%s] Unplugged Usb/Charger in Kernel Charging Mode Before Jumping to Kernel, Power Off\n", __func__);
#ifndef NO_POWER_OFF
		mt6575_power_off();
#endif

	}
	if (kernel_charging_boot() == 1) {
		if (pmic_detect_powerkey()) {
			dprintf(CRITICAL,"[%s] PowerKey Pressed in Kernel Charging Mode Before Jumping to Kernel, Reboot Os\n", __func__);
			//mt65xx_backlight_off();
			//mt_disp_power(0);
			mtk_arch_reset(1);
		}
	}
#endif
	dprintf(CRITICAL,"DRAM Rank :%d\n", g_nr_bank);
	for (i = 0; i < g_nr_bank; i++) {
#ifndef MTK_LM_MODE
		dprintf(CRITICAL,"DRAM Rank[%d] Start = 0x%x, Size = 0x%x\n", i, (unsigned int)bi_dram[i].start, (unsigned int)bi_dram[i].size);
#else
		dprintf(CRITICAL,"DRAM Rank[%d] Start = 0x%llx, Size = 0x%llx\n", i, bi_dram[i].start, bi_dram[i].size);
#endif
	}

#ifdef MBLOCK_LIB_SUPPORT
	mblock_show_info();
#endif

	dprintf(CRITICAL,"cmdline: %s\n", (char *)cmdline_get());
	dprintf(CRITICAL,"lk boot time = %d ms\n", lk_t);
	dprintf(CRITICAL,"lk boot mode = %d\n", g_boot_mode);
	dprintf(CRITICAL,"lk boot reason = %s\n", g_boot_reason[boot_reason]);
	dprintf(CRITICAL,"lk finished --> jump to linux kernel\n\n");
	entry(0, machtype, tags);
}

int boot_linux_from_storage(void)
{
	int ret=0;
#define CMDLINE_TMP_CONCAT_SIZE 100     //only for string concat, 200 bytes is enough
	char cmdline_tmpbuf[CMDLINE_TMP_CONCAT_SIZE];
	unsigned int kimg_load_addr;

#ifdef LK_PROFILING
	unsigned int time_load_recovery=0;
	unsigned int time_load_bootimg=0;
	unsigned int time_load_factory=0;
	time_load_recovery = get_timer(0);
	time_load_bootimg = get_timer(0);
#endif

#if 1

	switch (g_boot_mode) {
		case NORMAL_BOOT:
		case META_BOOT:
		case ADVMETA_BOOT:
		case SW_REBOOT:
		case ALARM_BOOT:
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		case KERNEL_POWER_OFF_CHARGING_BOOT:
		case LOW_POWER_OFF_CHARGING_BOOT:
#endif
#if defined(CFG_NAND_BOOT)
			snprintf(cmdline_tmpbuf, CMDLINE_TMP_CONCAT_SIZE, "%s%x%s%x",
			         NAND_MANF_CMDLINE, nand_flash_man_code, NAND_DEV_CMDLINE, nand_flash_dev_id);
			cmdline_append(cmdline_tmpbuf);
#endif
#ifdef MTK_GPT_SCHEME_SUPPORT
		        if (mtk_detect_key(17) && mtk_detect_key(8)) // 8 = POWER KEY
			    ret = mboot_android_load_bootimg_hdr("boot3", CFG_BOOTIMG_LOAD_ADDR);
			else if (mtk_detect_key(17)) // 17 = SIDE BUTTON KEY
			    ret = mboot_android_load_bootimg_hdr("boot2", CFG_BOOTIMG_LOAD_ADDR);
			else
			    ret = mboot_android_load_bootimg_hdr("boot", CFG_BOOTIMG_LOAD_ADDR);
#else
			ret = mboot_android_load_bootimg_hdr(PART_BOOTIMG, CFG_BOOTIMG_LOAD_ADDR);
#endif
			if (ret < 0) {
				msg_header_error("Android Boot Image");
			}

			if (g_is_64bit_kernel) {
				kimg_load_addr = (unsigned int)target_get_scratch_address();
			} else {
				kimg_load_addr = (g_boot_hdr!=NULL) ? g_boot_hdr->kernel_addr : CFG_BOOTIMG_LOAD_ADDR;
			}

#ifdef MTK_GPT_SCHEME_SUPPORT
			if (mtk_detect_key(17) && mtk_detect_key(8))
			    ret = mboot_android_load_bootimg("boot3", kimg_load_addr);
                        else if (mtk_detect_key(17))
			    ret = mboot_android_load_bootimg("boot2", kimg_load_addr);
			else
			    ret = mboot_android_load_bootimg("boot", kimg_load_addr);
#else
			ret = mboot_android_load_bootimg(PART_BOOTIMG, kimg_load_addr);
#endif

			if (ret < 0) {
				msg_img_error("Android Boot Image");
			}
#ifdef LK_PROFILING
			dprintf(CRITICAL,"[PROFILE] ------- load boot.img takes %d ms -------- \n", (int)get_timer(time_load_bootimg));
#endif
			break;

		case RECOVERY_BOOT:
#ifdef MTK_GPT_SCHEME_SUPPORT
			ret = mboot_android_load_recoveryimg_hdr("recovery", CFG_BOOTIMG_LOAD_ADDR);
#else
			ret = mboot_android_load_recoveryimg_hdr(PART_RECOVERY, CFG_BOOTIMG_LOAD_ADDR);
#endif
			if (ret < 0) {
				msg_header_error("Android Recovery Image");
			}

			if (g_is_64bit_kernel) {
				kimg_load_addr =  (unsigned int)target_get_scratch_address();
			} else {
				kimg_load_addr = (g_boot_hdr!=NULL) ? g_boot_hdr->kernel_addr : CFG_BOOTIMG_LOAD_ADDR;
			}

#ifdef MTK_GPT_SCHEME_SUPPORT
			ret = mboot_android_load_recoveryimg("recovery", kimg_load_addr);
#else
			ret = mboot_android_load_recoveryimg(PART_RECOVERY, kimg_load_addr);
#endif
			if (ret < 0) {
				msg_img_error("Android Recovery Image");
			}
#ifdef LK_PROFILING
			dprintf(CRITICAL,"[PROFILE] ------- load recovery.img takes %d ms -------- \n", (int)get_timer(time_load_recovery));
#endif
			break;

		case FACTORY_BOOT:
		case ATE_FACTORY_BOOT:
#if defined(CFG_NAND_BOOT)
			snprintf(cmdline_tmpbuf, CMDLINE_TMP_CONCAT_SIZE, "%s%x%s%x",
			         NAND_MANF_CMDLINE, nand_flash_man_code, NAND_DEV_CMDLINE, nand_flash_dev_id);
			cmdline_append(cmdline_tmpbuf);
#endif

#ifdef MTK_GPT_SCHEME_SUPPORT
			if (mtk_detect_key(17) && mtk_detect_key(8))
			    ret = mboot_android_load_bootimg_hdr("boot3", CFG_BOOTIMG_LOAD_ADDR);
                        else if (mtk_detect_key(17))
			    ret = mboot_android_load_bootimg_hdr("boot2", CFG_BOOTIMG_LOAD_ADDR);
                        else
			    ret = mboot_android_load_bootimg_hdr("boot", CFG_BOOTIMG_LOAD_ADDR);
#else
			ret = mboot_android_load_bootimg_hdr(PART_BOOTIMG, CFG_BOOTIMG_LOAD_ADDR);
#endif
			if (ret < 0) {
				msg_header_error("Android Boot Image");
			}

			if (g_is_64bit_kernel) {
				kimg_load_addr =  (unsigned int)target_get_scratch_address();
			} else {
				kimg_load_addr = (g_boot_hdr!=NULL) ? g_boot_hdr->kernel_addr : CFG_BOOTIMG_LOAD_ADDR;
			}

#ifdef MTK_GPT_SCHEME_SUPPORT
			if (mtk_detect_key(17) && mtk_detect_key(8))
			    ret = mboot_android_load_bootimg("boot3", kimg_load_addr);
                        else if (mtk_detect_key(17))
			    ret = mboot_android_load_bootimg("boot2", kimg_load_addr);
                        else
			    ret = mboot_android_load_bootimg("boot", kimg_load_addr);
#else
			ret = mboot_android_load_bootimg(PART_BOOTIMG, kimg_load_addr);
#endif
			if (ret < 0) {
				msg_img_error("Android Boot Image");
			}
#ifdef LK_PROFILING
			dprintf(CRITICAL,"[PROFILE] ------- load factory.img takes %d ms -------- \n", (int)get_timer(time_load_factory));
#endif
			break;

		case FASTBOOT:
		case DOWNLOAD_BOOT:
		case UNKNOWN_BOOT:
			break;

	}

#ifndef SKIP_LOADING_RAMDISK
	if (g_rimg_sz == 0) {
		if (g_boot_hdr != NULL) {
			g_rimg_sz = g_boot_hdr->ramdisk_size;
		}
	}
#ifdef MTK_3LEVEL_PAGETABLE
	/* rootfs addr */
	if (g_boot_hdr != NULL) {
		arch_mmu_map((uint64_t)g_boot_hdr->ramdisk_addr, (uint32_t)g_boot_hdr->ramdisk_addr,
		             MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(g_rimg_sz, PAGE_SIZE));
	}
#endif
	/* relocate rootfs (ignore rootfs header) */
	memcpy((g_boot_hdr!=NULL) ? (char *)g_boot_hdr->ramdisk_addr : (char *)CFG_RAMDISK_LOAD_ADDR, (char *)(g_rmem_off), g_rimg_sz);
	g_rmem_off = (g_boot_hdr!=NULL) ? g_boot_hdr->ramdisk_addr : CFG_RAMDISK_LOAD_ADDR;
#endif

#endif

	// 2 weak function for mt6572 memory preserved mode
	platform_mem_preserved_load_img();
	platform_mem_preserved_dump_mem();

	custom_port_in_kernel(g_boot_mode, cmdline_get());

	if (g_boot_hdr != NULL) {
		snprintf(cmdline_tmpbuf, CMDLINE_TMP_CONCAT_SIZE, "%s",g_boot_hdr->cmdline);
		cmdline_append(cmdline_tmpbuf);
	}

#ifdef SELINUX_STATUS
#if SELINUX_STATUS == 1
	cmdline_append("androidboot.selinux=disabled");
#elif SELINUX_STATUS == 2
	cmdline_append("androidboot.selinux=permissive");
#endif
#endif


#ifdef MTK_POWER_ON_WRITE_PROTECT
#if MTK_POWER_ON_WRITE_PROTECT == 1
#if defined(MTK_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)
	write_protect_flow();
#endif
#endif
#endif

	if (g_boot_hdr != NULL) {
		boot_linux((void *)g_boot_hdr->kernel_addr, (unsigned *)g_boot_hdr->tags_addr,
		           board_machtype(), (void *)g_boot_hdr->ramdisk_addr, g_rimg_sz);
	} else {
		boot_linux((void *)CFG_BOOTIMG_LOAD_ADDR, (unsigned *)CFG_BOOTARGS_ADDR,
		           board_machtype(), (void *)CFG_RAMDISK_LOAD_ADDR, g_rimg_sz);
	}

	while (1) ;

	return 0;
}

#if defined(CONFIG_MTK_USB_UNIQUE_SERIAL) || (defined(MTK_SECURITY_SW_SUPPORT) && defined(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT))
static char udc_chr[32] = {"ABCDEFGHIJKLMNOPQRSTUVWSYZ456789"};

int get_serial(u64 hwkey, u32 chipid, char ser[SERIALNO_LEN])
{
	u16 hashkey[4];
	u32 idx, ser_idx;
	u32 digit, id;
	u64 tmp = hwkey;

	memset(ser, 0x00, SERIALNO_LEN);

	/* split to 4 key with 16-bit width each */
	tmp = hwkey;
	for (idx = 0; idx < ARRAY_SIZE(hashkey); idx++) {
		hashkey[idx] = (u16)(tmp & 0xffff);
		tmp >>= 16;
	}

	/* hash the key with chip id */
	id = chipid;
	for (idx = 0; idx < ARRAY_SIZE(hashkey); idx++) {
		digit = (id % 10);
		hashkey[idx] = (hashkey[idx] >> digit) | (hashkey[idx] << (16-digit));
		id = (id / 10);
	}

	/* generate serail using hashkey */
	ser_idx = 0;
	for (idx = 0; idx < ARRAY_SIZE(hashkey); idx++) {
		ser[ser_idx++] = (hashkey[idx] & 0x001f);
		ser[ser_idx++] = (hashkey[idx] & 0x00f8) >> 3;
		ser[ser_idx++] = (hashkey[idx] & 0x1f00) >> 8;
		ser[ser_idx++] = (hashkey[idx] & 0xf800) >> 11;
	}
	for (idx = 0; idx < ser_idx; idx++)
		ser[idx] = udc_chr[(int)ser[idx]];
	ser[ser_idx] = 0x00;
	return 0;
}
#endif /* CONFIG_MTK_USB_UNIQUE_SERIAL */

#ifdef SERIAL_NUM_FROM_BARCODE
static inline int read_product_info(char *buf)
{
	int tmp = 0;

	if (!buf) return 0;

	mboot_recovery_load_raw_part("proinfo", buf, SN_BUF_LEN);

	for ( ; tmp < SN_BUF_LEN; tmp++) {
		if ( (buf[tmp] == 0 || buf[tmp] == 0x20) && tmp > 0) {
			break;
		} else if ( !isalpha(buf[tmp]) && !isdigit(buf[tmp]))
			return 0;
	}
	return tmp;
}
#endif

#ifdef CONFIG_MTK_USB_UNIQUE_SERIAL
static inline int read_product_usbid(char *serialno)
{
	u64 key;
	u32 hrid_size,ser_len;
	u32 i,chip_code,errcode = 0;
	char *cur_serialp = serialno;
	char serial_num[SERIALNO_LEN];

	/* read machine type */
	chip_code = board_machtype();

	/* read hrid */
	hrid_size = get_hrid_size();

	/* check ser_buf len. if need 128bit id, should defined into cust_usb.h */
	if(SN_BUF_LEN  < hrid_size * 8 ) {
		hrid_size = 2;
		errcode = 1;
	}

	for(i = 0; i < hrid_size/2; i++) {
		key = get_devinfo_with_index(13+i*2); /* 13, 15 */
		key = (key << 32) | (unsigned int)get_devinfo_with_index(12+i*2); /* 12, 14 */

		if (key != 0) {
			get_serial(key, chip_code, serial_num);
			ser_len = strlen(serial_num);
		}
		else {
			ser_len = strlen(DEFAULT_SERIAL_NUM);
			memcpy(serial_num, DEFAULT_SERIAL_NUM, ser_len);
			errcode = 2;
		}
		/* copy serial from serial_num to sn_buf */
		memcpy(cur_serialp, serial_num, ser_len);
		cur_serialp += ser_len;
	}
	cur_serialp = '\0';

	return errcode;
}
#endif

void mt_boot_init(const struct app_descriptor *app)
{
	unsigned usb_init = 0;
	unsigned sz = 0;
	int sec_ret = 0, errcode = 0;
#ifdef SERIAL_NUM_FROM_BARCODE
	char tmp[SN_BUF_LEN+1] = {0};
	unsigned ser_len = 0;
#endif
	char serial_num[SERIALNO_LEN];

#ifdef CONFIG_MTK_USB_UNIQUE_SERIAL
	errcode = read_product_usbid(sn_buf);
	if(errcode) {
		dprintf(CRITICAL,"serial number errcode :(%d)\n",errcode);
	}
#else
	memcpy(sn_buf, DEFAULT_SERIAL_NUM, strlen(DEFAULT_SERIAL_NUM));
#endif

#ifdef SERIAL_NUM_FROM_BARCODE
	ser_len = read_product_info(tmp);
	if (ser_len == 0) {
		ser_len = strlen(DEFAULT_SERIAL_NUM);
		strncpy(tmp, DEFAULT_SERIAL_NUM, ser_len);
	}
	memset( sn_buf, 0, sizeof(sn_buf));
	strncpy( sn_buf, tmp, ser_len);
#endif
	sn_buf[SN_BUF_LEN] = '\0';
	surf_udc_device.serialno = sn_buf;

	if (g_boot_mode == FASTBOOT)
		goto fastboot;

#ifdef MTK_SECURITY_SW_SUPPORT
#if MTK_FORCE_VERIFIED_BOOT_SIG_VFY
	/* verify oem image with android verified boot signature instead of mediatek proprietary signature */
	/* verification is postponed to boot image loading stage */
	/* note in this case, boot/recovery image will be verified even when secure boot is disabled */
	g_boot_state = BOOT_STATE_RED;
#else
	if (0 != sec_boot_check(0)) {
		g_boot_state = BOOT_STATE_RED;
	}
#endif
#endif

	/* Will not return */
	boot_linux_from_storage();

fastboot:
	target_fastboot_init();
	if (!usb_init)
		/*Hong-Rong: wait for porting*/
		udc_init(&surf_udc_device);

	mt_part_dump();
	sz = target_get_max_flash_size();
	fastboot_init(target_get_scratch_address(), sz);
	udc_start();

}


APP_START(mt_boot)
.init = mt_boot_init,
 APP_END
