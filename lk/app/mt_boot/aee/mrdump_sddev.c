/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2016. All rights reserved.
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

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <block_generic_interface.h>
#if 0
#include <platform/mmc_common_inter.h>
#endif

#include "aee.h"
#include "kdump.h"
#include "kdump_sdhc.h"

static part_t *part_device_init(int dev, const char *part_name)
{
	part_t *part = mt_part_get_partition((char *)part_name);
	part_dev_t *part_dev;

	if (part == NULL)
		return NULL;

	part_dev = mt_part_get_device();

	if (!part_dev)
		return NULL;

#ifdef MTK_GPT_SCHEME_SUPPORT
	voprintf_info("%s offset: %lu, size: %lu Mb\n", part->info->name, part->start_sect, (part->nr_sects * part_dev->blkdev->blksz) / 0x100000UL);
#else
	voprintf_info("%s offset: %lu, size: %lu Mb\n", part->name, part->startblk, (part->blknum * part_dev->blkdev->blksz) / 0x100000UL);
#endif

	return part;
}

static bool part_device_check_range(part_dev_t *part_dev, part_t *part, uint64_t offset, int32_t len)
{
#ifdef MTK_GPT_SCHEME_SUPPORT
	if ((offset / part_dev->blkdev->blksz + len / part_dev->blkdev->blksz) >= part->nr_sects) {
		voprintf_error("GPT: Read %s partition overflow, size%lu, block %lu\n", part->info->name, offset / part_dev->blkdev->blksz, part->nr_sects);
		return 0;
	}
#else
	if ((offset / part_dev->blkdev->blksz + len / part_dev->blkdev->blksz) >= part->blknum) {
		voprintf_error("Read %s partition overflow, size%lu, block %lu\n", part->name, offset / part_dev->blkdev->blksz, part->blknum);
		return 0;
	}
#endif
	return 1;
}

static bool part_device_read(struct mrdump_dev *dev, uint64_t offset, uint8_t *buf, int32_t len)
{
	part_dev_t *part_dev = mt_part_get_device();
	if (!part_dev)
		return 0;

	part_t *part = dev->handle;

	if (!part_device_check_range(part_dev, part, offset, len))
		return 0;

	uint64_t part_offset = ((uint64_t)part->start_sect * part_dev->blkdev->blksz) + offset;

#if defined(MTK_EMMC_SUPPORT)
#if defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	return part_dev->read(part_dev, part_offset, (uchar *)buf, len, part->part_id) == len;
#else
	return part_dev->read(part_dev, part_offset, (uchar *)buf, len) == len;
#endif
#else
	return part_dev->read(part_dev, part_offset, (uchar *)buf, len, part->part_id) == len;
#endif
}

static bool part_device_write(struct mrdump_dev *dev, uint64_t offset, uint8_t *buf, int32_t len)
{
	part_dev_t *part_dev = mt_part_get_device();
	if (!part_dev)
		return 0;

	part_t *part = dev->handle;

	if (!part_device_check_range(part_dev, part, offset, len))
		return 0;

	uint64_t part_offset = ((uint64_t)part->start_sect * part_dev->blkdev->blksz) + offset;
#if defined(MTK_EMMC_SUPPORT)
#if defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	return part_dev->write(part_dev, (uchar *)buf, part_offset, len, part->part_id) == len;
#else
	return part_dev->write(part_dev, (uchar *)buf, part_offset, len) == len;
#endif
#else
	return part_dev->write(part_dev, (uchar *)buf, part_offset, len, part->part_id) == len;
#endif
}

struct mrdump_dev *mrdump_dev_emmc_vfat(void)
{
	struct mrdump_dev *dev = malloc(sizeof(struct mrdump_dev));

	part_t *fatpart = part_device_init(0, "intsd");
	if (fatpart == NULL) {
		voprintf_error("No VFAT partition found!\n");
		return NULL;
	}
	dev->name = "emmc";
	dev->handle = fatpart;
	dev->read = part_device_read;
	dev->write = part_device_write;
	return dev;
}

static part_t *mrdump_get_ext4_partition(void)
{
	part_t *ext4part;

	ext4part = part_device_init(0, "userdata");   //mt6735, mt6752, mt6795
	if (ext4part != NULL)
		return ext4part;

	ext4part = part_device_init(0, "USRDATA");    //mt6582, mt6592, mt8127
	if (ext4part != NULL)
		return ext4part;

	ext4part = part_device_init(0, "PART_USER");  //mt6572
	if (ext4part != NULL)
		return ext4part;

	return NULL;
}

struct mrdump_dev *mrdump_dev_emmc_ext4(void)
{
	struct mrdump_dev *dev = malloc(sizeof(struct mrdump_dev));

	part_t *ext4part = mrdump_get_ext4_partition();
	if (ext4part == NULL) {
		voprintf_error("No EXT4 partition found!\n");
		return NULL;
	}
	dev->name = "emmc";
	dev->handle = ext4part;
	dev->read = part_device_read;
	dev->write = part_device_write;
	return dev;
}


#if 0
static bool mrdump_dev_sdcard_read(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
	return mmc_wrap_bread(1, sector_addr, blockLen, pdBuf) == 1;
}

static bool mrdump_dev_sdcard_write(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
	return mmc_wrap_bwrite(1, sector_addr, blockLen, pdBuf) == 1;
}

struct mrdump_dev *mrdump_dev_sdcard(void)
{
	struct mrdump_dev *dev = malloc(sizeof(struct mrdump_dev));
	dev->name = "sdcard";
	dev->handle = NULL;
	dev->read = mrdump_dev_sdcard_read;
	dev->write = mrdump_dev_sdcard_write;

	mmc_legacy_init(2);
	return dev;
}
#endif
