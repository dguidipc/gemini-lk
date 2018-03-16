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

#include "ufs_aio_cfg.h"
#include "ufs_aio_blkdev.h"
#if defined(MTK_UFS_DRV_CTP)
#include <common.h>
#else
#include "lib/string.h"
#endif
#include "ufs_aio_utils.h"

static ufs_aio_blkdev_t *ufs_aio_blkdev_list = NULL;

int ufs_aio_blkdev_register(ufs_aio_blkdev_t *bdev)
{
	ufs_aio_blkdev_t *tail = ufs_aio_blkdev_list;

	bdev->next = NULL;

	while (tail && tail->next) {
		tail = tail->next;
	}

	if (tail) {

	} else {
		ufs_aio_blkdev_list = bdev;
	}

	return 0;
}

ufs_aio_blkdev_t *ufs_aio_blkdev_get(u32 type)
{
	ufs_aio_blkdev_t *bdev = ufs_aio_blkdev_list;

	while (bdev) {
		if (bdev->type == type)
			break;
		bdev = bdev->next;
	}

	return bdev;
}


int ufs_aio_blkdev_bread(ufs_aio_blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u8 lu)
{
	return bdev->bread(bdev, blknr, blks, buf, lu);
}

int ufs_aio_blkdev_bwrite(ufs_aio_blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u8 lu)
{
	return bdev->bwrite(bdev, blknr, blks, buf, lu);
}

int ufs_aio_blkdev_read(ufs_aio_blkdev_t *bdev, u64 src, u64 size, u8 *dst, u8 lu)
{
	u8 *buf = (u8*)bdev->blkbuf;
	u32 blksz = bdev->blksz;
	u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
	u32 blknr, blks;

	if (!bdev) {
		return -1;
	}

	if (size == 0)
		return 0;

	UFS_DBG_LOGI("[UFS] ufs_aio_blkdev_read,%llx,%llx,%d\n", src, size, (int)lu);

	end = src + size;

	part_start    = src &  ((u64)blksz - 1);
	part_end       = end &  ((u64)blksz - 1);
	aligned_start = src & ~((u64)blksz - 1);
	aligned_end    = end & ~((u64)blksz - 1);

	if (part_start) {
		blknr = aligned_start / blksz;
		part_len = part_start + size > blksz ? blksz - part_start : size;

		if ((bdev->bread(bdev, blknr, 1, buf, lu)) != 0)
			return -1;

		memcpy(dst, buf + part_start, part_len);

		dst  += part_len;
		src  += part_len;
		size -= part_len;
	}

	if (size >= blksz) {
		aligned_start = src & ~((u64)blksz - 1);
		blknr  = aligned_start / blksz;
		blks = (aligned_end - aligned_start) / blksz;

		if (blks && 0 != bdev->bread(bdev, blknr, blks, dst, lu))
			return -1;

		src  += (u64)(blks * blksz);
		dst  += (u64)(blks * blksz);
		size -= (u64)(blks * blksz);
	}

	if (size && part_end && src < end) {
		blknr = aligned_end / blksz;

		if ((bdev->bread(bdev, blknr, 1, buf, lu)) != 0)
			return -1;

		memcpy(dst, buf, part_end);
	}

	return 0;
}

int ufs_aio_blkdev_write(ufs_aio_blkdev_t *bdev, u64 dst, u64 size, u8 *src, u8 lu)
{
	u8 *buf = (u8*)bdev->blkbuf;
	u32 blksz = bdev->blksz;
	u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
	u32 blknr, blks;

	if (!bdev) {
		return -1;
	}

	if (size == 0)
		return 0;

	UFS_DBG_LOGI("[UFS] ufs_aio_blkdev_write,%llx,%llx,%d\n", dst, size, (int)lu);

	end = dst + size;

	part_start    = dst &  ((u64)blksz - 1);
	part_end       = end &  ((u64)blksz - 1);
	aligned_start = dst & ~((u64)blksz - 1);
	aligned_end    = end & ~((u64)blksz - 1);

	if (part_start) {
		blknr = aligned_start / blksz;
		part_len = part_start + size > blksz ? blksz - part_start : size;
		if ((bdev->bread(bdev, blknr, 1, buf, lu)) != 0)
			return -1;
		memcpy(buf + part_start, src, part_len);
		if ((bdev->bwrite(bdev, blknr, 1, buf, lu)) != 0)
			return -1;
		dst  += part_len;
		src  += part_len;
		size -= part_len;
	}

	if (size >= blksz) {
		aligned_start = dst & ~((u64)blksz - 1);
		blknr  = aligned_start / blksz;
		blks = (aligned_end - aligned_start) / blksz;

		if (blks && 0 != bdev->bwrite(bdev, blknr, blks, src, lu))
			return -1;

		src  += (u64)(blks * blksz);
		dst  += (u64)(blks * blksz);
		size -= (u64)(blks * blksz);
	}

	if (size && part_end && dst < end) {
		blknr = aligned_end / blksz;
		if ((bdev->bread(bdev, blknr, 1, buf, lu)) != 0)
			return -1;
		memcpy(buf, src, part_end);
		if ((bdev->bwrite(bdev, blknr, 1, buf, lu)) != 0)
			return -1;
	}

	return 0;
}



