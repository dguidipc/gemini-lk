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
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/bootimg.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <platform/mt_gpt.h>
#include <platform/sec_status.h>

#include <platform/mtk_key.h>
#include <target/cust_key.h>
#include <part_interface.h>
#define MODULE_NAME "LK_BOOT"
#define BOOTIMG_DEF_PGSIZE  (2 * 1024)

// ************************************************************************

#define PUBK_LEN 256
#define BOOT_STATE_GREEN   0x0
#define BOOT_STATE_ORANGE  0x1
#define BOOT_STATE_YELLOW  0x2
#define BOOT_STATE_RED     0x3

#define DEVICE_STATE_UNLOCKED 0x0
#define DEVICE_STATE_LOCKED   0x1

#define UART_EARLY_CONSOLE_LEN 128

//*********
//* Notice : it's kernel start addr (and not include any debug header)
unsigned int g_kmem_off = 0;

//*********
//* Notice : it's rootfs start addr (and not include any debug header)
unsigned int g_rmem_off = 0;


unsigned int g_pg_sz = BOOTIMG_DEF_PGSIZE;
unsigned int g_bimg_sz = 0;
unsigned int g_rcimg_sz = 0;
unsigned int g_fcimg_sz = 0;
int g_kimg_sz = 0;
int g_rimg_sz = 0;
unsigned int g_boot_state = BOOT_STATE_GREEN;

extern boot_img_hdr *g_boot_hdr;
extern bool cmdline_append(const char *append_string);
extern void platform_parse_bootopt(u8* bootopt_str);

#ifdef TRUSTONIC_TEE_SUPPORT
extern void root_status_check(u32 sec_boot_check);
#endif
extern void mtk_wdt_restart(void);

int print_boot_state(void)
{
	int ret = 0;
	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			dprintf(CRITICAL, "boot state: orange\n");
			break;
		case BOOT_STATE_YELLOW:
			dprintf(CRITICAL, "boot state: yellow\n");
			break;
		case BOOT_STATE_RED:
			dprintf(CRITICAL, "boot state: red\n");
			break;
		case BOOT_STATE_GREEN:
			dprintf(CRITICAL, "boot state: green\n");
			break;
		default:
			dprintf(CRITICAL, "boot state: unknown\n");
			break;
	}

	return ret;
}

int yellow_state_warning(void)
{
	const char* title_msg = "yellow state\n\n";
	unsigned char pubk[PUBK_LEN] = {0};
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has loaded a different operating system\n");
	video_printf("ID:\n");
	ret = sec_get_custom_pubk(pubk, PUBK_LEN);
	if (ret) {
		video_printf("Cannot get custom public key, abort in 5 seconds\n");
		mtk_wdt_restart();
		mdelay(5000);
		mtk_wdt_restart();
		return -1;
	}
	video_printf("%x %x %x %x %x %x %x %x\n", pubk[0], pubk[1], pubk[2], pubk[3], pubk[4], pubk[5], pubk[6], pubk[7]);
	video_printf("Yes (Volume UP)   : Confirm and Boot.\n\n");
	video_printf("No  (Volume Down) : Abort.\n\n");

	while (1) {
		mtk_wdt_restart();
		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY)) //VOL_UP
			return 0;
		else if (mtk_detect_key(MT65XX_MENU_OK_KEY)) //VOL_DOWN
			return -1;
		else {
			/* ignore */
		}
	}
}

int orange_state_warning(void)
{
	const char* title_msg = "Orange State\n\n";
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has been unlocked and can't be trusted\n");
	video_printf("Your device will boot in 5 seconds\n");
	mtk_wdt_restart();
	mdelay(5000);
	mtk_wdt_restart();

	return 0;
}

int red_state_warning(void)
{
	const char* title_msg = "Red State\n\n";
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has failed verification and may not\n");
	video_printf("work properly\n");
	video_printf("Your device will boot in 5 seconds\n");
	mtk_wdt_restart();
	mdelay(5000);
	mtk_wdt_restart();

	return -1;
}

int show_warning(void)
{
	int ret = 0;
	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			ret = orange_state_warning();
			break;
		case BOOT_STATE_YELLOW:
#ifdef MTK_SECURITY_YELLOW_STATE_SUPPORT
			ret = yellow_state_warning();
			if (0 == ret) /* user confirms to boot into yellow state */
				break;
			/* fall into red state if user refuses to enter yellow state */
#else
			ret = -1;
			/* fall into red state since yellow state is not supported */
#endif
		case BOOT_STATE_RED:
			ret = red_state_warning();
			ret = -1; /* return error */
			break;
		case BOOT_STATE_GREEN:
		default:
			break;
	}

	return ret;
}

int set_boot_state_to_cmdline()
{
	int ret = 0;

	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			cmdline_append("androidboot.verifiedbootstate=orange");
			break;
		case BOOT_STATE_YELLOW:
			cmdline_append("androidboot.verifiedbootstate=yellow");
			break;
		case BOOT_STATE_RED:
			cmdline_append("androidboot.verifiedbootstate=red");
			break;
		case BOOT_STATE_GREEN:
			cmdline_append("androidboot.verifiedbootstate=green");
			break;
		default:
			break;
	}

	return ret;
}

/* the only possible boot state change here is green->red */
int verified_boot_init(unsigned char *part_name, unsigned char *img_name)
{
	int ret = 0;

	unsigned int policy_entry_idx = 0;
	unsigned int img_auth_required = 0;
	unsigned int img_vfy_time = 0;
	int lock_state = 0;

	g_boot_state = BOOT_STATE_GREEN;

#ifdef MTK_SECURITY_SW_SUPPORT
	/* please refer to the following website for verified boot flow */
	/* http://source.android.com/devices/tech/security/verifiedboot/verified-boot.html */

	img_vfy_time = get_timer(0);
	policy_entry_idx = get_policy_entry_idx(part_name);
	img_auth_required = get_vfy_policy(policy_entry_idx);
	if (img_auth_required) {
		ret = (int)sec_img_auth_init(part_name, img_name);
#ifdef MTK_SECURITY_YELLOW_STATE_SUPPORT
		if (ERR_RETRY_WITH_VERIFIED_BOOT == ret)
			ret = 0;
#endif

		dprintf(INFO, "[SBC] boot cert vfy(%d ms)\n", (unsigned int)get_timer(img_vfy_time));

		if (ret) {
			g_boot_state = BOOT_STATE_RED;
			ret = -1;
		}
	} else
		dprintf(INFO, "[SBC] (%s, %s) cert vfy disabled\n", part_name, img_name);
#endif

_end:
	ret = print_boot_state();
	if (ret)
		return ret;

	ret = show_warning();
	if (ret)
		return ret;

	return ret;
}

int verified_boot_flow(unsigned long img_addr, unsigned int img_sz)
{
	int ret = 0;

	unsigned int policy_entry_idx = 0;
	unsigned int img_auth_required = 0;
	unsigned int img_vfy_time = 0;
	int lock_state = 0;

	if (g_boot_state == BOOT_STATE_RED)
		goto _end;

	g_boot_state = BOOT_STATE_GREEN;

	/* loading boot image without verifying boot image header first would not
	   become a security hole since initial load address is not specified by
	   boot image header, boot image is loaded to an address specified by lk,
	   and kernel and ramdisk will be relocated later after boot image
	   verification is done. Need to consider whether boot image size will be
	   too big so it overlaps with other important region. */

	/* if MTK_SECURITY_SW_SUPPORT is not defined, boot state is always green */
#ifdef MTK_SECURITY_SW_SUPPORT
	/* please refer to the following website for verified boot flow */
	/* http://source.android.com/devices/tech/security/verifiedboot/verified-boot.html */
	ret = sec_query_device_lock(&lock_state);
	if (ret) {
		g_boot_state = BOOT_STATE_RED;
		goto _end;
	}

	if (DEVICE_STATE_LOCKED == lock_state) {
		img_vfy_time = get_timer(0);
		policy_entry_idx = get_policy_entry_idx("boot");
		img_auth_required = get_vfy_policy(policy_entry_idx);
		if (img_auth_required) {
			img_vfy_time = get_timer(0);
			ret = (int)sec_img_auth(img_addr, img_sz);
			if (ret) {
				/* show public key on LCM and requires user confirmation to proceed */
				ret = sec_img_auth_custom(img_addr, img_sz);
				if (0 == ret)
					g_boot_state = BOOT_STATE_YELLOW;
				else
					g_boot_state = BOOT_STATE_RED;
			}
			dprintf(INFO, "[SBC] img vfy(%d ms)\n", (unsigned int)get_timer(img_vfy_time));
#if defined(TRUSTONIC_TEE_SUPPORT) && defined(RDA_STATUS_SUPPORT)
			if (g_boot_state == BOOT_STATE_GREEN)
				root_status_check(1); /* boot image verification succeeds */
			else
				root_status_check(0); /* boot image verification fails */
#endif //TRUSTONIC_TEE_SUPPORT && RDA_STATUS_SUPPORT
		} else
			dprintf(CRITICAL, "[SBC] img vfy disabled\n");
	} else if (DEVICE_STATE_UNLOCKED == lock_state) {
		g_boot_state = BOOT_STATE_ORANGE;
	} else { /* unknown lock state*/
		g_boot_state = BOOT_STATE_RED;
	}
#endif //MTK_SECURITY_SW_SUPPORT

_end:
	ret = print_boot_state();
	if (ret)
		return ret;

	ret = show_warning();
	if (ret)
		return ret;

	ret = set_boot_state_to_cmdline();
	if (ret)
		return ret;

	return ret;
}

#if 1
static int mboot_common_load_part_info(part_dev_t *dev, char *part_name, char *img_name, part_hdr_t *part_hdr, unsigned long long *img_start_addr)
{
	long len;
	u64 offset, start, part_size;
	part_t *part;
	part_dev_t *part_dev;

	part_dev = mt_part_get_device();

	if (!part_dev)
		return -ENODEV;

	part = mt_part_get_partition(part_name);
	if (part == NULL)
		return -ENOENT;

	part_size = part->nr_sects;
	part_size *= part_dev->blkdev->blksz;

	start = (u64)part->start_sect * part_dev->blkdev->blksz;

	offset = 0;

	//***************
	//* read partition header
	//*
	while (1) {

		len = partition_read(part_name, offset, (uchar*)part_hdr, sizeof(part_hdr_t));

		if (len < 0) {
			dprintf(CRITICAL, "[%s] %s partition read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
			break;
		}

		dprintf(CRITICAL, "\n=========================================\n");
		dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.magic);
		part_hdr->info.name[31]='\0'; //append end char
		dprintf(CRITICAL, "[%s] %s name         : %s\n",MODULE_NAME,part_name,part_hdr->info.name);
		dprintf(CRITICAL, "[%s] %s size         : %d\n",MODULE_NAME,part_name,part_hdr->info.dsize);
		dprintf(CRITICAL, "=========================================\n");

		//***************
		//* check partition magic
		//*
		if (part_hdr->info.magic != PART_MAGIC) {
			dprintf(CRITICAL, "[%s] %s partition magic error\n", MODULE_NAME, part_name);
			len = -1;
			break;
		}

		offset += sizeof(part_hdr_t);

		//***************
		//* check partition data size
		//*
		if ((offset + part_hdr->info.dsize) > part_size) {
			dprintf(CRITICAL, "[%s] %s partition size error 0x%llx, 0x%llx\n", MODULE_NAME, part_name, offset + part_hdr->info.dsize, part_size);
			len = -1;
			break;
		}

		//***************
		//* check image name
		//*
		if (!strncasecmp(part_hdr->info.name, img_name, sizeof(part_hdr->info.name))) {
			dprintf(INFO, "[%s] %s image name found\n", MODULE_NAME, img_name);
			break;
		}

		//***************
		//* find next image
		//* image with new layout will align to a specific size
		//*
		if (part_hdr->info.ext_magic == PART_EXT_MAGIC)
			offset += ROUNDUP(part_hdr->info.dsize, part_hdr->info.align_size);
		else
			offset += part_hdr->info.dsize;
	}

	if (len > 0)
		*img_start_addr = offset;

	return len;
}


/**********************************************************
 * Routine: mboot_common_load_part
 *
 * Description: common function for loading image from nand flash
 *              this function is called by
 *                  (1) 'mboot_common_load_logo' to display logo
 *
 **********************************************************/
int mboot_common_load_part(char *part_name, char *img_name, unsigned long addr)
{
	long len;
	unsigned long long start_addr = 0;
	part_t *part;
	part_dev_t *dev;
	part_hdr_t *part_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}

	part_hdr = (part_hdr_t*)malloc(sizeof(part_hdr_t));

	if (!part_hdr) {
		return -ENOMEM;
	}

	len = mboot_common_load_part_info(dev, part_name, img_name, part_hdr, &start_addr);
	if (len < 0) {
		len = -EINVAL;
		goto exit;
	}


	//****************
	//* read image data
	//*
	dprintf(INFO, "read the data of %s\n", part_name);

	len = partition_read(part_name, start_addr, (uchar*)addr, (size_t)part_hdr->info.dsize);

	if (len < 0) {
		len = -EIO;
		goto exit;
	}


exit:
	if (part_hdr)
		free(part_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_common_load_logo
 *
 * Description: function to load logo to display
 *
 **********************************************************/
int mboot_common_load_logo(unsigned long logo_addr, char* filename)
{
	int ret;
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	long len;
#endif

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(filename, (unsigned char *)logo_addr, 0);

	if (len > 0)
		return (int)len;
#endif

	ret = mboot_common_load_part("logo", "logo", logo_addr);

	return ret;
}

/**********************************************************
 * Routine: mboot_android_check_img_info
 *
 * Description: this function is called to
 *              (1) check the header of kernel / rootfs
 *
 * Notice : this function will be called by 'mboot_android_check_bootimg_hdr'
 *
 **********************************************************/
int mboot_android_check_img_info(char *part_name, part_hdr_t *part_hdr)
{
	//***************
	//* check partition magic
	//*
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic not match\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition name
	//*
	if (strncasecmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name not match\n", MODULE_NAME, part_name);
		return -1;
	}

	dprintf(CRITICAL, "\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.magic);
	dprintf(CRITICAL, "[%s] %s size         : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.dsize);
	dprintf(CRITICAL, "=========================================\n");

	//***************
	//* return the image size
	//*
	return part_hdr->info.dsize;
}

/**********************************************************
 * Routine: mboot_android_check_bootimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_bootimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	long len;
	int ret = 0;
	u64 addr;
	part_t *part;
	part_dev_t *part_dev;

	part_dev = mt_part_get_device();

	if (!part_dev)
		return -ENODEV;

	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -1;
	}

	addr = (u64)part->start_sect * part_dev->blkdev->blksz;

	//***************
	//* read partition header
	//*

	dprintf(CRITICAL, "part page addr is 0x%llx\n", addr);

	len = partition_read(part_name, 0, (uchar*) boot_hdr, sizeof(boot_img_hdr));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] Android Partition Name                : %s\n"    , MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	dprintf(CRITICAL, "============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic,BOOT_MAGIC, BOOT_MAGIC_SIZE)!=0) {
		dprintf(CRITICAL, "[%s] boot image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;
	g_pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0;
		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else {
			g_kmem_off = boot_hdr->kernel_addr;
		}
		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		if (boot_hdr->ramdisk_size % pg_sz == 0) {
			r_pg_cnt = boot_hdr->ramdisk_size / pg_sz;
		} else {
			r_pg_cnt = (boot_hdr->ramdisk_size / pg_sz) + 1;
		}

		dprintf(INFO, " > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		dprintf(INFO, " > kernel mem offset = 0x%x\n",g_kmem_off);
		dprintf(INFO, " > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		g_bimg_sz = (k_pg_cnt + r_pg_cnt + 1)* pg_sz;

		dprintf(INFO, " > boot image size = 0x%x\n",g_bimg_sz);
	}

	ret = verified_boot_init("boot", "boot");
	if (ret)
		return ret;

	return 0;
}

/**********************************************************
 * Routine: mboot_android_check_recoveryimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_recoveryimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	int ret = 0;
	long len;
	u64 addr;
	part_t *part;

	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;


	part = mt_part_get_partition(part_name);
	if (!part) {
		return -1;
	}
	addr = (u64)part->start_sect * dev->blkdev->blksz;

	//***************
	//* read partition header
	//*
	len = partition_read(part_name, 0, (uchar*) boot_hdr, sizeof(boot_img_hdr));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s Recovery image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	dprintf(CRITICAL, "============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)!=0) {
		dprintf(CRITICAL, "[%s] Recovery image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;
	g_pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0;
		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else {
			g_kmem_off =  boot_hdr->kernel_addr;
		}
		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		if (boot_hdr->ramdisk_size % pg_sz == 0) {
			r_pg_cnt = boot_hdr->ramdisk_size / pg_sz;
		} else {
			r_pg_cnt = (boot_hdr->ramdisk_size / pg_sz) + 1;
		}

		dprintf(INFO, " > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		dprintf(INFO, " > kernel mem offset = 0x%x\n",g_kmem_off);
		dprintf(INFO, " > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		//g_rcimg_sz = part->start_sect * dev->blkdev->blksz;
		g_rcimg_sz = (k_pg_cnt + r_pg_cnt + 1)* pg_sz;

		dprintf(INFO, " > Recovery image size = 0x%x\n", g_rcimg_sz);
	}

	ret = verified_boot_init("recovery", "boot");
	if (ret)
		return ret;
	return 0;
}

/**********************************************************
 * Routine: mboot_android_load_bootimg_hdr
 *
 * Description: this is the entry function to handle boot image header
 *
 **********************************************************/
int mboot_android_load_bootimg_hdr(char *part_name, unsigned long addr)
{
	long len;
//	unsigned long begin;
//	unsigned long start_addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;


	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

//    start_addr = part->start_sect * dev->blkdev->blksz;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_bootimg_hdr(dev, part_name, boot_hdr);
	platform_parse_bootopt(boot_hdr->cmdline);

#ifndef USER_BUILD
	char cmdline[UART_EARLY_CONSOLE_LEN];
	if (g_is_64bit_kernel == 1) {
		snprintf(cmdline, UART_EARLY_CONSOLE_LEN, " earlycon=uartmtk,mmio32,0x%x,", AP_UART0_BASE);
	} else {
		snprintf(cmdline, UART_EARLY_CONSOLE_LEN, " earlyprintk=mt6797,0x%x ", AP_UART0_BASE);
	}

	cmdline_append(cmdline);
#endif

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg_hdr
 *
 * Description: this is the entry function to handle Recovery image header
 *
 **********************************************************/
int mboot_android_load_recoveryimg_hdr(char *part_name, unsigned long addr)
{
	long len;
//	unsigned long begin;
//	unsigned long start_addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_recoveryimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_recoveryimg_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

//    start_addr = part->start_sect * dev->blkdev->blksz;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_recoveryimg_hdr(dev, part_name, boot_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_bootimg
 *
 * Description: main function to load Android Boot Image
 *
 **********************************************************/
int mboot_android_load_bootimg(char *part_name, unsigned long addr)
{
	int ret = 0;
	long len;
	unsigned long long start_addr;
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}

	//***************
	//* not to include unused header
	//*
	start_addr =(u64)part->start_sect * dev->blkdev->blksz + g_boot_hdr->page_size;

	/*
	 * check mkimg header
	 */
	dprintf(INFO, "check mkimg header\n");

	len = partition_read(part_name, g_boot_hdr->page_size, (uchar*)addr, MKIMG_HEADER_SZ);
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s partition read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -EIO;
	}

	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		dprintf(INFO, "no mkimg header in kernel image\n");
	} else {
		dprintf(INFO, "mkimg header exist in kernel image\n");
		addr  = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

	//***************
	//* read image data
	//*
	dprintf(INFO, "\nread the data of %s (size = 0x%x)\n", part_name, g_bimg_sz);
	dprintf(INFO, " > from - 0x%016llx (skip boot img hdr)\n",start_addr);
	dprintf(INFO, " > to   - 0x%x (starts with kernel img hdr)\n",addr);

	len = partition_read(part_name, g_boot_hdr->page_size, (uchar*)addr, (size_t)g_bimg_sz);

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		dprintf(INFO, "no mkimg header in ramdisk image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		dprintf(INFO, "mkimg header exist in ramdisk image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
		return len;
	}

	ret = verified_boot_flow(addr, g_bimg_sz - g_pg_sz);
	if (ret)
		len = -EIO;

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg
 *
 * Description: main function to load Android Recovery Image
 *
 **********************************************************/
int mboot_android_load_recoveryimg(char *part_name, unsigned long addr)
{
	int ret = 0;
	long len;
	unsigned long long start_addr;
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}

	//***************
	//* not to include unused header
	//*
	start_addr = (u64)part->start_sect * dev->blkdev->blksz + g_boot_hdr->page_size;

	/*
	 * check mkimg header
	 */
	dprintf(INFO, "check mkimg header\n");
	len = partition_read(part_name, g_boot_hdr->page_size, (uchar*)addr, MKIMG_HEADER_SZ);
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s partition read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -EIO;
	}

	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		dprintf(INFO, "no mkimg header in kernel image\n");
	} else {
		dprintf(INFO, "mkimg header exist in kernel image\n");
		addr  = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

	//***************
	//* read image data
	//*
	dprintf(INFO, "\nread the data of %s (size = 0x%x)\n", part_name, g_rcimg_sz);
	dprintf(INFO, " > from - 0x%016llx (skip recovery img hdr)\n",start_addr);
	dprintf(INFO, " > to   - 0x%x (starts with kernel img hdr)\n",addr);

	len = partition_read(part_name, g_boot_hdr->page_size, (uchar*)addr, (size_t)g_rcimg_sz);

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info("recovery", (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		dprintf(INFO, "no mkimg header in recovery image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		dprintf(INFO, "mkimg header exist in recovery image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
		return len;
	}

	ret = verified_boot_flow(addr, g_rcimg_sz - g_pg_sz);
	if (ret)
		len = -EIO;

	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_raw_part
 *
 * Description: load raw data for recovery mode support
 *
 **********************************************************/
int mboot_recovery_load_raw_part(char *part_name, unsigned long *addr, unsigned int size)
{
	long len;
	unsigned long begin;
	unsigned long long start_addr;
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();

	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);

	if (!part) {
		return -ENOENT;
	}

	start_addr = (u64)part->start_sect * dev->blkdev->blksz;

	begin = get_timer(0);

	len = partition_read(part_name, 0, (uchar*)addr, (size_t)size);

	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(CRITICAL, "[%s] Load '%s' partition to 0x%08lX (%d bytes in %ld ms)\n", MODULE_NAME, part->info->name, (unsigned long)addr, size, get_timer(begin));

exit:
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_raw_part_offset
 *
 * Description: load partition raw data with offset
 *
 * offset and size must page alignemnt
 **********************************************************/
int mboot_recovery_load_raw_part_offset(char *part_name, unsigned long *addr, unsigned long offset, unsigned int size)
{
	long len;
	unsigned long begin;
	unsigned long long start_addr;
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}
	start_addr = (u64)part->start_sect * dev->blkdev->blksz + ROUNDUP(offset, dev->blkdev->blksz);
	begin = get_timer(0);

	len = partition_read(part_name, ROUNDUP(offset, dev->blkdev->blksz), (uchar*)addr, (size_t)ROUNDUP(size, dev->blkdev->blksz));

	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(INFO, "[%s] Load '%s' partition to 0x%08lX (%d bytes in %ld ms)\n",
	        MODULE_NAME, part->name, (unsigned long)addr, size, get_timer(begin));

exit:
	return len;
}


/**********************************************************
 * Routine: mboot_recovery_load_misc
 *
 * Description: load recovery command
 *
 **********************************************************/
int mboot_recovery_load_misc(unsigned long *misc_addr, unsigned int size)
{
	int ret;

	dprintf(INFO, "[mboot_recovery_load_misc]: size is %u\n", size);
	dprintf(INFO, "[mboot_recovery_load_misc]: misc_addr is 0x%x\n", misc_addr);

	ret = mboot_recovery_load_raw_part("para", misc_addr, size);

	if (ret < 0)
		return ret;

	return ret;
}

unsigned int mboot_get_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
	u64 addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr boot_hdr;
	unsigned page_size = 0x800; /* used to cache page size in boot image hdr, default 2KB */

	*size = 0;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_img_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_img_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}
	addr = (u64)part->start_sect * dev->blkdev->blksz;

	len = partition_read(part_name, 0, (uchar*)&boot_hdr, sizeof(boot_img_hdr));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	boot_hdr.magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Partition Name             : %s\n", MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Magic            : %s\n", MODULE_NAME, boot_hdr.magic);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Kernel Size      : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Kernel Address   : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Rootfs Size      : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Rootfs Address   : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Tags Address     : 0x%08X\n", MODULE_NAME, boot_hdr.tags_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Page Size        : 0x%08X\n", MODULE_NAME, boot_hdr.page_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Command Line     : %s\n"  , MODULE_NAME, boot_hdr.cmdline);
	dprintf(CRITICAL, "============================================================\n");

	page_size = boot_hdr.page_size;
	*size +=  page_size; /* boot header size is 1 page*/
	*size +=  (((boot_hdr.kernel_size + page_size - 1) / page_size) * page_size);
	*size +=  (((boot_hdr.ramdisk_size + page_size - 1) / page_size) * page_size);
	*size +=  (((boot_hdr.second_size + page_size - 1) / page_size) * page_size);

	return ret;
}

#endif
