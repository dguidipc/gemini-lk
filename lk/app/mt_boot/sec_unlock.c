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

#include <platform/mt_typedefs.h>
#include <platform/sec_status.h>
#include <platform/mt_reg_base.h>
#include <platform/env.h>
#include "sec_unlock.h"
#include "fastboot.h"
#include <target/cust_key.h>
#include <platform/mt_gpt.h>
#include <video.h>
#include <string.h>
#include <debug.h>
#include <platform/mtk_key.h>
#include <printf.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <part_interface.h>
#include <part_status.h>

#define UNLOCK_KEY_SIZE 32
#define SERIAL_NUMBER_SIZE 16
#define DEFAULT_SERIAL_NUM "0123456789ABCDEF"

char fb_unlock_key_str[UNLOCK_KEY_SIZE+1] = {0};

extern u32 get_devinfo_with_index(u32 index);
extern int get_serial(u64 hwkey, u32 chipid, char ser[38]);
extern void sec_get_serial_number_from_unlock_key(u8 *unlock_key, u32 unlock_key_len, u8 *cal_serial, u32 cal_serial_len);
extern int sec_set_device_lock(int do_lock);
extern int sec_query_device_lock(int *lock_state);

static int fastboot_data_part_wipe()
{
	int ret = B_OK;
	int err;

	set_env("unlock_erase", "start");

	err = partition_erase("userdata");
	if (err) {
		ret = PART_ERASE_FAIL;
		set_env("unlock_erase", "fail");
	} else {
		ret = B_OK;
		set_env("unlock_erase", "pass");
	}

	return ret;
}

static int fastboot_get_unlock_perm(unsigned int *unlock_allowed)
{
	int ret = B_OK;
	int index = 0;
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	unsigned long long unlock_allowed_flag_offset = 0;

	ret = partition_exists("frp");
	if (ret == PART_NOT_EXIST) {
		dprintf(CRITICAL,"frp paritition does not exist\n");
		/* backward compatible with KK, where frp partition does not exist */
		*unlock_allowed = 1;
		return B_OK;
	}

	/* get unlock enable flag address, which is inside frp partition */
	size = partition_get_size_by_name("frp");
	unlock_allowed_flag_offset = size - sizeof(unsigned int);

	dprintf(CRITICAL,"frp paritition size: 0x%llx\n", size);
	dprintf(CRITICAL,"unlock_allowed_flag_offset: 0x%llx\n", unlock_allowed_flag_offset);

	ret = partition_read("frp", unlock_allowed_flag_offset, (u8 *)unlock_allowed, sizeof(unsigned int));
	if (ret < 0) {
		*unlock_allowed = 0;
		return ret;
	}

	return B_OK;
}

void fastboot_boot_menu(void)
{
	const char* title_msg = "Select Boot Mode:\n[VOLUME_UP to select.  VOLUME_DOWN is OK.]\n\n";
	video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	video_printf(title_msg);
	video_printf("[Recovery    Mode]             \n");
#ifdef MTK_FASTBOOT_SUPPORT
	video_printf("[Fastboot    Mode]         <<==\n");
#endif
	video_printf("[Normal      Boot]             \n");
#ifndef USER_BUILD
	video_printf("[Normal      Boot +ftrace]    \n");
	video_printf("[Normal      slub debug off]     \n");
#endif
	video_printf(" => FASTBOOT mode...\n");
}

void unlock_warranty(void)
{
	const char* title_msg = "Unlock bootloader?\n\n";
	video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	video_printf(title_msg);
	video_printf("If you unlock the bootloader,you will be able to install custom operating\n");
	video_printf("system software on this phone.\n\n");

	video_printf("A custom OS is not subject to the same testing as the original OS, and can\n");
	video_printf("cause your phone and installed applications to stop working properly.\n\n");

	video_printf("To prevent unauthorized access to your personal data,unlocking the bootloader\n");
	video_printf("will also delete all personal data from your phone(a \"factory data reset\").\n\n");

	video_printf("Press the Volume UP/Down buttons to select Yes or No. \n\n");
	video_printf("Yes (Volume UP):Unlock(may void warranty).\n\n");
	video_printf("No (Volume Down):Do not unlock bootloader.\n\n");

}

void lock_warranty(void)
{
	const char* title_msg = "lock bootloader?\n\n";
	video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	video_printf(title_msg);
	video_printf("If you lock the bootloader,you will need to install official operating\n");
	video_printf("system software on this phone.\n\n");


	video_printf("To prevent unauthorized access to your personal data,locking the bootloader\n");
	video_printf("will also delete all personal data from your phone(a \"factory data reset\").\n\n");

	video_printf("Press the Volume UP/Down buttons to select Yes or No. \n\n");
	video_printf("Yes (Volume UP):Lock bootloader.\n\n");
	video_printf("No (Volume Down):Do not lock bootloader.\n\n");
}

void fastboot_get_unlock_ability(const char *arg, void *data, unsigned sz)
{
#define UNLOCK_CAP_RESP_MAX_SIZE 64
	int ret = B_OK;
	unsigned int unlock_allowed = 0;
	char msg[UNLOCK_CAP_RESP_MAX_SIZE] = {0};

	ret = fastboot_get_unlock_perm(&unlock_allowed);
	if (ret != B_OK) {
		snprintf(msg, UNLOCK_CAP_RESP_MAX_SIZE, "\nFailed to get unlock permission - Err:0x%x \n", ret);
		msg[UNLOCK_CAP_RESP_MAX_SIZE - 1] = '\0'; /* prevent msg from not being ended with '\0'*/
		fastboot_fail(msg);
	} else {
		snprintf(msg, UNLOCK_CAP_RESP_MAX_SIZE, "unlock_ability = %d", unlock_allowed);
		msg[UNLOCK_CAP_RESP_MAX_SIZE - 1] = '\0'; /* prevent msg from not being ended with '\0'*/
		fastboot_info(msg);
		fastboot_okay("");
	}

	return;
}

void fastboot_oem_key(const char *arg, void *data, unsigned sz)
{
	int key_length;
	key_length = strlen(arg+1);
	if (key_length != UNLOCK_KEY_SIZE) {
		fastboot_fail("argument size is wrong\n");
	} else {
		strcpy(fb_unlock_key_str,arg+1);
		dprintf(INFO,"key is '%s' and length is %d\n",fb_unlock_key_str,key_length);
		fastboot_okay("");
	}

	return;
}

void fastboot_oem_query_lock_state(const char *arg, void *data, unsigned sz)
{
#define LKS_RESP_MAX_SIZE 64
	int ret = B_OK;
	int lock_state;
	char msg[LKS_RESP_MAX_SIZE] = {0};

	ret = sec_query_device_lock(&lock_state);
	if (ret != B_OK) {
		snprintf(msg, LKS_RESP_MAX_SIZE, "cannot get lks (ret = 0x%x)", ret);
		msg[LKS_RESP_MAX_SIZE - 1] = '\0'; /* prevent msg from not being ended with '\0'*/
		fastboot_fail(msg);
	} else {
		snprintf(msg, LKS_RESP_MAX_SIZE, "lks = %d", lock_state);
		msg[LKS_RESP_MAX_SIZE - 1] = '\0'; /* prevent msg from not being ended with '\0'*/
		fastboot_info(msg);
		fastboot_okay("");
	}
}

int fastboot_oem_unlock(const char *arg, void *data, unsigned sz)
{
	int ret = B_OK;
	char msg[128] = {0};
	unsigned int unlock_allowed = 0;

	unlock_warranty();

	while (1) {
		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY)) { //VOL_UP
			fastboot_info("Start unlock flow\n");
			//Invoke security check after confirming "yes" by user
			ret = fastboot_get_unlock_perm(&unlock_allowed);
			if (ret != B_OK) {
				sprintf(msg, "\nFailed to get unlock permission - Err:0x%x \n", ret);
				video_printf("Unlock failed...return to fastboot in 3s\n");
				mdelay(3000);
				fastboot_boot_menu();
				fastboot_fail(msg);
				break;
			}

			dprintf(CRITICAL,"unlock_allowed = 0x%x\n", unlock_allowed);

			if (!unlock_allowed) {
				sprintf(msg, "\nUnlock operation is not allowed\n");
				video_printf("Unlock failed...return to fastboot in 3s\n");
				mdelay(3000);
				fastboot_boot_menu();
				fastboot_fail(msg);
				break;
			}

			ret = fastboot_oem_unlock_chk();
			if (ret != B_OK) {
				sprintf(msg, "\nUnlock failed - Err:0x%x \n", ret);
				video_printf("Unlock failed...return to fastboot in 3s\n");
				mdelay(3000);
				fastboot_boot_menu();
				fastboot_fail(msg);
			} else {
				video_printf("Unlock Pass...return to fastboot in 3s\n");
				mdelay(3000);
				fastboot_boot_menu();
				fastboot_okay("");
			}
			break;
		} else if (mtk_detect_key(MT65XX_MENU_OK_KEY)) { //VOL_DOWN
			video_printf("return to fastboot in 3s\n");
			mdelay(3000);
			fastboot_boot_menu();
			fastboot_okay("");
			break;
		} else {
			//If we press other keys, discard it.
		}
	}
	return ret;

}

static int fastboot_oem_key_chk(void)
{
#define SERIALNO_LEN    38
	int ret = B_OK;
	u32 chip_code = 0x0;
	char serial_number[SERIALNO_LEN] = {0};
	u64 key;
	char cal_serial_number[SERIAL_NUMBER_SIZE+1] = {0};

	/* Check for the unlock key */
	if (UNLOCK_KEY_SIZE != strlen(fb_unlock_key_str)) {
		//fastboot_fail("Unlock key length is incorrect!");
		ret = ERR_UNLOCK_KEY_WRONG_LENGTH;
		return ret;
	}

	/* Get the device serial number */
	key = get_devinfo_with_index(13);
	key = (key << 32) | get_devinfo_with_index(12);
	chip_code = DRV_Reg32(APHW_CODE);
	if (key != 0)
		get_serial(key, chip_code, serial_number);
	else
		memcpy(serial_number, DEFAULT_SERIAL_NUM, SERIAL_NUMBER_SIZE);

	/* Calculate the serial number from the unlock key */
	sec_get_serial_number_from_unlock_key((u8 *)fb_unlock_key_str, UNLOCK_KEY_SIZE, (u8 *)cal_serial_number, SERIAL_NUMBER_SIZE);

	/* Compare the results */
	if (0 != memcmp(serial_number, cal_serial_number, SERIAL_NUMBER_SIZE)) {
		//fastboot_fail("Unlock key code is incorrect!");
		ret = ERR_UNLOCK_WRONG_KEY_CODE;
		return ret;
	}

	return ret;
}

int fastboot_oem_unlock_chk()
{
	int ret = B_OK;

#ifdef MTK_SEC_FASTBOOT_UNLOCK_KEY_SUPPORT
	if (B_OK != (ret = fastboot_oem_key_chk())) {
		goto _fail;
	}
#endif

	/* Do format operation of data partition */
	if (B_OK != (ret = fastboot_data_part_wipe())) {
		//fastboot_fail("Data partition wipe failed!");
		goto _fail;
	}

	/* Set unlock done flag */
	ret = sec_set_device_lock(0);

_fail:
	return ret;
}


int fastboot_oem_lock(const char *arg, void *data, unsigned sz)
{
	int ret = B_OK;
	char msg[128] = {0};
	int inFactory = 0;
	int requireConfirmation = 0;

	ret = sec_is_in_factory(&inFactory);
	if (ret)
		return ret;

	requireConfirmation = inFactory ? 0 : 1;

	lock_warranty();

	while (1) {
		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY) || !requireConfirmation) { //VOL_UP
			fastboot_info("Start lock flow\n");
			//Invoke security check after confiming "yes" by user
			ret = fastboot_oem_lock_chk();
			if (ret != B_OK) {
				sprintf(msg, "\nlock failed - Err:0x%x \n", ret);
				video_printf("lock failed...return to fastboot in 3s\n");
				mdelay(3000);
				fastboot_boot_menu();
				fastboot_fail(msg);
			} else {
				video_printf("lock Pass...return to fastboot in 3s\n");
				mdelay(3000);
				fastboot_boot_menu();
				fastboot_okay("");
			}
			break;
		} else if (mtk_detect_key(MT65XX_MENU_OK_KEY)) { //VOL_DOWN
			video_printf("return to fastboot in 3s\n");
			mdelay(3000);
			fastboot_boot_menu();
			fastboot_okay("");
			break;
		} else {
			//If we press other keys, discard it.
		}
	}
	return ret;
}

int fastboot_oem_lock_chk()
{
#define TRY_LOCK 1
	int ret = B_OK;
	int inFactory = 0;
	int wipe_userdata = 0;

	if (B_OK != (ret = sec_boot_check(TRY_LOCK))) {
		goto _image_verify_fail;
	}

	ret = sec_is_in_factory(&inFactory);
	if (ret)
		return ret;

	/* tool connection in preloader is not disabled by default */
	/* and device must be locked before product delivery */
	/* at this time, don't wipe userdata to accelerate boot time */
	wipe_userdata = inFactory ? 0 : 1;
	if (wipe_userdata) {
		if (B_OK != (ret = fastboot_data_part_wipe())) {
			//fastboot_fail("Data partition wipe failed!");
			goto _erase_data_fail;
		}
	}

	/* Set lock done flag */
	ret = sec_set_device_lock(1);

_image_verify_fail:
_erase_data_fail:
	return ret;
}

