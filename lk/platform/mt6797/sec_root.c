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
#include <string.h>
#include <video.h>
#include <printf.h>
#include <platform/sec_status.h>

#if defined(MTK_EMMC_SUPPORT) && defined(TRUSTONIC_TEE_SUPPORT)

#include <platform/mmc_rpmb.h>

extern int sec_query_device_lock(int *lock_state);
extern u32 get_devinfo_with_index(u32 index);
extern int32_t rpmb_init(void);
extern int32_t rpmb_uninit(void);
extern int compute_random_pattern(unsigned int *magic_pattern);

#define DATA_LEN            (8)
#define BOOT_IMG_VERIFIED   (1)
#define ID_INDEX            (12)
unsigned int g_random_pattern;
/* #define RPMB_DEBUG */


void show_root_status(void)
{
	int ret = 0;
	u32 *root_status = NULL;
	u32 data_buf[DATA_LEN] = {0};

	ret = mmc_rpmb_read_data(0x0, data_buf, DATA_LEN);
	if (ret) {
		dprintf(CRITICAL, "[%s] mmc_rpmb_read_data failed\n", __func__);
		video_printf(" => Software status: unable to read!!\n");
		return;
	}

#if defined(RPMB_DEBUG)
	dprintf(INFO, "[%s] after readback, dump buf\n", __func__);
	dprintf(INFO,"%02x %02x %02x %02x %02x %02x %02x %02x \n",
	        data_buf[0], data_buf[1], data_buf[2], data_buf[3],
	        data_buf[4], data_buf[5], data_buf[6], data_buf[7]);
#endif

	root_status = (u32*)data_buf;

	if ((*root_status) > 1) {
		video_printf(" => Software status: Modified(0x%x)\n", *root_status);
		dprintf(INFO," => Software status: Modified(0x%x)\n", *root_status);
	} else {
		video_printf(" => Software status: Official(0x%x)\n", *root_status);
		dprintf(INFO," => Software status: Official(0x%x)\n", *root_status);
	}

}

int reset_root_status(u32 sec_boot_check_status)
{
	u32 data_buf[DATA_LEN] = {0};
	int ret = B_OK;

	if (BOOT_IMG_VERIFIED == sec_boot_check_status) {
		/* clear root status */
		ret = mmc_rpmb_write_data(0x0, data_buf, DATA_LEN);

		if (ret) {
			dprintf(CRITICAL, "[%s] mmc_rpmb_write_data failed\n", __func__);
		} else {
			dprintf(INFO, "[%s] mmc_rpmb_write_data succeed\n", __func__);
			ret = B_OK;
		}
	}

	return ret;
}

void root_status_check(u32 sec_boot_check_status)
{
	int ret = B_OK;
	int lock_state;

	rpmb_init();
	/* compute random pattern for rpmb driver */
	compute_random_pattern(&g_random_pattern);

	ret = sec_query_device_lock(&lock_state);
	if (ret == B_OK) {
		/*
		* if it's in lock state and boot image is verified pass,
		* reset the root status.
		*/
		dprintf(INFO, "[%s] lock state:%d sec_boot_check:%d\n", __func__
		        , lock_state, sec_boot_check_status);
		if ((1 == lock_state) && (1 == sec_boot_check_status)) {
			dprintf(INFO, "[%s] reset root status\n", __func__);
			ret = reset_root_status(sec_boot_check_status);
			if (ret == B_OK) {
				dprintf(INFO, "[%s] reset root status succeed!!\n", __func__);
			} else {
				dprintf(CRITICAL, "[%s] reset root status failed!!\n", __func__);
			}
		}
	} else {
		dprintf(CRITICAL, "[%s] query lock state failed!!\n", __func__);
	}

	show_root_status();
	/* clear random pattern */
	g_random_pattern = 0;

	/* mark leaving lk. */
	rpmb_uninit();
}

#ifdef RPMB_DEBUG
void dump_buf(u8 *buf, uint32_t size)
{
	unsigned int i;
	for (i = 0; i < size/8; i++) {
		dprintf(CRITICAL,"%02x %02x %02x %02x %02x %02x %02x %02x \n",
		        buf[0+i*8], buf[1+i*8], buf[2+i*8], buf[3+i*8],
		        buf[4+i*8], buf[5+i*8], buf[6+i*8], buf[7+i*8]);
	}
	for (i = 0; i < size%8; i++) {
		dprintf(CRITICAL,"%02x \n", buf[((size/8)*8) + i]);
	}
	return;
}
#endif //RPMB_DEBUG

#endif //defined(MTK_EMMC_SUPPORT) && defined(TRUSTONIC_TEE_SUPPORT)


