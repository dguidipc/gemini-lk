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
#include <stdint.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/errno.h>
#include <debug.h>

#define PMT 1

__attribute__ ((aligned(64))) static uchar mt_part_buf[16384];

static part_dev_t *mt_part_dev = NULL;
extern void part_init_pmt(unsigned long totalblks,part_dev_t *dev);

static int mt_part_generic_read(part_dev_t *dev, u64 src, uchar *dst, int size, unsigned int part_id)
{
	int dev_id = dev->id;
	uchar *buf = &mt_part_buf[0];
	block_dev_desc_t *blkdev = dev->blkdev;
	u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
	ulong blknr, blkcnt;

	if (!blkdev) {
		dprintf(CRITICAL, "No block device registered\n");
		return -ENODEV;
	}

	if (size == 0) {
		return 0;
	}

	end = src + size;

	part_start    = src &  ((u64)blkdev->blksz - 1);
	part_end      = end &  ((u64)blkdev->blksz - 1);
	aligned_start = src & ~((u64)blkdev->blksz - 1);
	aligned_end   = end & ~((u64)blkdev->blksz - 1);

	/* dprintf(INFO, "%s part_start:0x%x size:0x%x\n", __func__, part_start, size); */

	if (part_start) {
		blknr = (ulong)(aligned_start >> blkdev->blk_bits);
		part_len = (u64)blkdev->blksz - part_start;
		if (part_len>(u64)size) {
			part_len = (u64)size;
		}
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id))!= 1) {
			return -EIO;
		}
		memcpy(dst, buf + part_start, part_len);
		dst += part_len;
		src += part_len;
	}

	aligned_start = src & ~((u64)blkdev->blksz - 1);
	blknr  = (ulong)(aligned_start >> blkdev->blk_bits);
	blkcnt = (ulong)((aligned_end - aligned_start) >> blkdev->blk_bits);

	if (blkcnt != 0) {
		if ((blkdev->block_read(dev_id, blknr, blkcnt, (unsigned long *)(dst), part_id)) != blkcnt) {
			return -EIO;
		}
	}

	src += ((u64)blkcnt << blkdev->blk_bits);
	dst += ((u64)blkcnt << blkdev->blk_bits);

	if (part_end && src < end) {
        	blknr = (ulong)(aligned_end >> blkdev->blk_bits);
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}
		memcpy(dst, buf, part_end);
	}
	return size;
}

static int mt_part_generic_write(part_dev_t *dev, uchar *src, u64 dst, int size, unsigned int part_id)
{
	int dev_id = dev->id;
	uchar *buf = &mt_part_buf[0];
	block_dev_desc_t *blkdev = dev->blkdev;
	u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
	ulong blknr, blkcnt;

	if (!blkdev) {
		dprintf(CRITICAL, "No block device registered\n");
		return -ENODEV;
	}

	if (size == 0) {
		return 0;
	}

	end = dst + size;

	part_start    = dst &  ((u64)blkdev->blksz - 1);
	part_end      = end &  ((u64)blkdev->blksz - 1);
	aligned_start = dst & ~((u64)blkdev->blksz - 1);
	aligned_end   = end & ~((u64)blkdev->blksz - 1);

	/* dprintf(INFO, "%s part_start:0x%x size:0x%x\n", __func__, part_start, size); */

	if (part_start) {
		blknr = (ulong)(aligned_start >> blkdev->blk_bits);
		part_len = (u64)blkdev->blksz - part_start;
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1)
			return -EIO;
		memcpy(buf + part_start, src, part_len);
		if ((blkdev->block_write(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1)
			return -EIO;
		dst += part_len;
		src += part_len;
	}

	aligned_start = dst & ~((u64)blkdev->blksz - 1);
	blknr  = (ulong)(aligned_start >> blkdev->blk_bits);
	blkcnt = (ulong)((aligned_end - aligned_start) >> blkdev->blk_bits);

	if ((blkdev->block_write(dev_id, blknr, blkcnt, (unsigned long *)(src), part_id)) != blkcnt) {
		return -EIO;
	}

	src += ((u64)blkcnt << blkdev->blk_bits);
	dst += ((u64)blkcnt << blkdev->blk_bits);

	if (part_end && dst < end) {
		blknr = (u64)(aligned_end >> blkdev->blk_bits);
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}
		memcpy(buf, src, part_end);
		if ((blkdev->block_write(dev_id, blknr, 1, (unsigned long*)buf, part_id))!= 1) {
			return -EIO;
		}
	}
	return size;
}

int mt_part_register_device(part_dev_t *dev)
{
	/* dprintf(INFO, "[mt_part_register_device]\n"); */
	if (!mt_part_dev) {
		if (!dev->read) {
			dev->read = mt_part_generic_read;
		}
		if (!dev->write) {
			dev->write = mt_part_generic_write;
		}
		mt_part_dev = dev;

		dprintf(INFO, "[mt_part_register_device]malloc %d : mt_part_buf:%x mt_part_dev:0x%lx\n",
			(16384), mt_part_buf, mt_part_dev);

#ifdef PMT
		part_init_pmt(BLK_NUM(1 * GB), dev);
#else
		mt_part_init(BLK_NUM(1 * GB));
#endif
	}

	return 0;
}

part_dev_t *mt_part_get_device(void)
{
	if (mt_part_dev && !mt_part_dev->init && mt_part_dev->init_dev) {
		mt_part_dev->init_dev(mt_part_dev->id);
		mt_part_dev->init = 1;
	}
	return mt_part_dev;
}

