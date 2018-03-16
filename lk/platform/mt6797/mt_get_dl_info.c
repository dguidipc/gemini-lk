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

#include <printf.h>
#include <debug.h>
#include <malloc.h>
#include <string.h>
#include <platform/errno.h>
#include "platform/partition.h"

struct checksum_info {
	u32 image_index;
	u32 pc_checksum;
	u32 da_checksum;
	char checksum_status[8];
};

struct image_info {
	char image_name[16];
};

struct dl_status {
	char magic_num[24];
	char version[8];
	struct checksum_info cs_info[PART_MAX_COUNT];
	char ram_checksum[16];
	char download_status[16];
	struct image_info img_info[PART_MAX_COUNT];
};

#define DL_PASS         (0)
#define DL_FAIL         (1)
#define DL_NOT_FOUND    (2)
#define DL_INFO_SIZE    (8096)

int mmc_get_dl_info(void)
{
	struct dl_status download_info;
	u64 dl_addr;
	u8 *dl_buf;

	part_dev_t *dev;
	part_t *part;

	int i, err, loglevel;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "[DL_INFO]fail to get device\n");
		err = -ENODEV;
		goto out;
	}

	part = get_part("flashinfo");
	if (!part) {
		dprintf(CRITICAL, "[DL_INFO]fail to find partition flashinfo\n");
		err = -ENODEV;
		goto out;
	}

	dl_addr = (u64)(part->start_sect) * (u64)(dev->blkdev->blksz);
	dprintf(ALWAYS, "[DL_INFO]get dl info from 0x%llx\n", dl_addr);

	dl_buf = (u8 *)calloc(1, DL_INFO_SIZE);
	if (!dl_buf) {
		dprintf(CRITICAL, "[DL_INFO]fail to calloc buffer(count=%d)\n", DL_INFO_SIZE);
		err = -ENOMEM;
		goto fail_malloc;
	}

	err = dev->read(dev, dl_addr, dl_buf, DL_INFO_SIZE, part->part_id);
	if (err != DL_INFO_SIZE) {
		dprintf(CRITICAL, "[DL_INFO]fail to read data(%d)\n", err);
		err = -EIO;
		goto fail_read;
	}

	memcpy(&download_info, dl_buf, sizeof(download_info));

	if (memcmp(download_info.magic_num, "DOWNLOAD INFORMATION!!", 22)) {
		dprintf(CRITICAL, "[DL_INFO]fail to find DL INFO magic\n");
		err = DL_NOT_FOUND;
		goto fail_read;
	}

	if (!memcmp(download_info.download_status, "DL_DONE", 7) ||
	        !memcmp(download_info.download_status, "DL_CK_DONE", 10)) {
		loglevel = INFO;
		err = DL_PASS;
	} else {
		loglevel = CRITICAL;
		err = DL_FAIL;
	}

	dprintf(loglevel, "[DL_INFO]version: %s\n", download_info.version);
	dprintf(loglevel, "[DL_INFO]dl_status: %s\n", download_info.download_status);
	dprintf(loglevel, "[DL_INFO]dram_checksum: %s\n", download_info.ram_checksum);
	for (i = 0; i < PART_MAX_COUNT; i++) {
		if (download_info.cs_info[i].image_index != 0) {
			dprintf(loglevel, "[DL_INFO]image:[%02d]%-12s, checksum: %s\n",
			        download_info.cs_info[i].image_index,
			        download_info.img_info[i].image_name,
			        download_info.cs_info[i].checksum_status);
		}
	}

fail_read:
	free(dl_buf);
fail_malloc:
	//put_part(part);
out:
	return err;
}
