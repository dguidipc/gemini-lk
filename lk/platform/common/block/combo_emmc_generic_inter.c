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

/* alignment handler
 * MSDC HW dma does not need to make alignment
 * MSDC PIO mode needs to be 4 byte align */
#define DMA_BUF_ALIGN_SIZE   1
#define PIO_BUF_ALIGN_SIZE   4

#define MAX(a,b) (((a)>(b))?(a):(b))
#define PART_BUF_ALIGN_SIZE    MAX(DMA_BUF_ALIGN_SIZE,PIO_BUF_ALIGN_SIZE)

static part_dev_t *mt_part_dev;
static uchar *mt_part_buf;

static ulong mt_part_alloc_buffer_align(uchar** buf_alloc, ulong item_cnt, int item_size) {
	ulong blkcnt_buf = item_cnt;
	*buf_alloc = NULL;
	while (blkcnt_buf > 0) {
		*buf_alloc = (uchar*)memalign(PART_BUF_ALIGN_SIZE, blkcnt_buf * item_size);
		if (*buf_alloc != NULL) {
			break;
		} else {
			blkcnt_buf = blkcnt_buf >> 1;
		}
	}
	if (*buf_alloc == NULL) {
		dprintf(CRITICAL, "[%s:%d] allocate %lu bytes buffer for alignment handler fail\n", __FUNCTION__, __LINE__, blkcnt_buf * item_size);
		*buf_alloc = (uchar *) &mt_part_buf[0];
		blkcnt_buf = 1;
	}
	dprintf(INFO, "[%s:%d] allocate %lu bytes buffer for alignment handler, addr:%x\n", __FUNCTION__, __LINE__, blkcnt_buf * item_size, (u32)*buf_alloc);
	return blkcnt_buf;
}

static int mt_part_generic_read(part_dev_t *dev, u64 src, uchar *dst, int size, unsigned int part_id)
{
	int dev_id = dev->id;
	uchar *buf;
	block_dev_desc_t *blkdev = dev->blkdev;
	u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
	u32 dst_start;
	ulong blknr, blkcnt, blkcnt_buf;

	if (!blkdev) {
		dprintf(CRITICAL,"No block device registered\n");
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

	if (part_start) {
		buf = (uchar *) &mt_part_buf[0];
		blknr = aligned_start >> (blkdev->blk_bits);
		part_len = blkdev->blksz - part_start;
		if (part_len > (u64)size) {
			part_len = size;
		}

		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}

		memcpy(dst, buf + part_start, part_len);
		dst += part_len;
		src += part_len;
	}
	aligned_start = src & ~((u64)blkdev->blksz - 1);
	blknr  = aligned_start >> (blkdev->blk_bits);
	blkcnt = (aligned_end - aligned_start) >> (blkdev->blk_bits);

	dst_start = (u32)dst & (PART_BUF_ALIGN_SIZE - 1); // dst buffer alignment
	if (blkcnt > 0) {
		if (dst_start) {
			dprintf(INFO,"[%s:%d] %llx, %x, %d, %lu, %lu, %d\n", __FUNCTION__, __LINE__, src, (u32)dst, dst_start, blknr, blkcnt, PART_BUF_ALIGN_SIZE);
			blkcnt_buf = mt_part_alloc_buffer_align(&buf, blkcnt, blkdev->blksz);
			while (blkcnt >= blkcnt_buf) {
				if ((blkdev->block_read(dev_id, blknr, blkcnt_buf, (unsigned long *)buf, part_id)) != blkcnt_buf) {
					if (buf)
						free(buf);
					return -EIO;
				}
				memcpy(dst, buf,  blkcnt_buf * blkdev->blksz);
				blknr += blkcnt_buf;
				blkcnt -= blkcnt_buf;
				src += (blkcnt_buf << blkdev->blk_bits);
				dst += (blkcnt_buf << blkdev->blk_bits);
			}
			dprintf(INFO, "[%s:%d] remain blkcnt: %lu\n", __FUNCTION__, __LINE__, blkcnt);
			if (blkcnt > 0) {
				if ((blkdev->block_read(dev_id, blknr, blkcnt, (unsigned long *)buf, part_id)) != blkcnt) {
					if (buf)
						free(buf);
					  return -EIO;
				}
				memcpy(dst, buf, blkcnt * blkdev->blksz);
				blknr += blkcnt;
				blkcnt = 0;
				src += (blkcnt << blkdev->blk_bits);
				dst += (blkcnt << blkdev->blk_bits);
			}
			if (buf)
				free(buf);
		} else {
			if ((blkdev->block_read(dev_id, blknr, blkcnt, (unsigned long *)(dst), part_id)) != blkcnt)
				return -EIO;
			src += (blkcnt << (blkdev->blk_bits));
			dst += (blkcnt << (blkdev->blk_bits));
		}
	}

	if (part_end && src < end) {
		buf = (uchar *) &mt_part_buf[0];
		blknr = aligned_end >> (blkdev->blk_bits);
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
	uchar *buf;
	block_dev_desc_t *blkdev = dev->blkdev;
	u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
	u32 src_start;
	ulong blknr, blkcnt, blkcnt_buf;

	dprintf(INFO,"[%s:%d] %x, %llx, %d, %d, align: %d\n", __FUNCTION__, __LINE__, (u32)src, dst, size, part_id, PART_BUF_ALIGN_SIZE);

	if (!blkdev) {
		dprintf(CRITICAL,"No block device registered\n");
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

	if (part_start) {
		buf = (uchar *) &mt_part_buf[0];
		blknr = aligned_start >> (blkdev->blk_bits);
		part_len = blkdev->blksz - part_start;
		if (part_len > (u64)size) {
			part_len = size;
		}
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}
		memcpy(buf + part_start, src, part_len);

		if ((blkdev->block_write(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}
		dst += part_len;
		src += part_len;
	}

	aligned_start = dst & ~((u64)blkdev->blksz - 1);
	blknr  = aligned_start >> (blkdev->blk_bits);
	blkcnt = (aligned_end - aligned_start) >> (blkdev->blk_bits);

	src_start = (u32)src & (PART_BUF_ALIGN_SIZE - 1); // src buffer alignment
	dprintf(INFO,"[%s:%d] %x, %llx, %x, %lu, %lu\n", __FUNCTION__, __LINE__, (u32)src, dst, src_start, blknr, blkcnt);
	if (blkcnt > 0) {
		if (src_start) {
			blkcnt_buf = mt_part_alloc_buffer_align(&buf, blkcnt, blkdev->blksz);
			while (blkcnt >= blkcnt_buf) {
				memcpy(buf, src, blkcnt_buf * blkdev->blksz);
				if ((blkdev->block_write(dev_id, blknr, blkcnt_buf, (unsigned long *)buf, part_id)) != blkcnt_buf) {
					if (buf)
						free(buf);
					return -EIO;
				}
				blknr += blkcnt_buf;
				blkcnt -= blkcnt_buf;
				src += (blkcnt_buf << blkdev->blk_bits);
				dst += (blkcnt_buf << blkdev->blk_bits);
			}
			dprintf(INFO, "[%s:%d] remain blkcnt: %lu\n", __FUNCTION__, __LINE__, blkcnt);
			if (blkcnt > 0) {
				memcpy(buf, src, blkcnt * blkdev->blksz);
				if ((blkdev->block_write(dev_id, blknr, blkcnt, (unsigned long *)buf, part_id)) != blkcnt) {
					if (buf)
						free(buf);
						return -EIO;
				}
				blknr += blkcnt;
				blkcnt = 0;
				src += (blkcnt << blkdev->blk_bits);
				dst += (blkcnt << blkdev->blk_bits);
			}
			if (buf)
				free(buf);
		} else {
			if ((blkdev->block_write(dev_id, blknr, blkcnt, (unsigned long *)(src), part_id)) != blkcnt)
				return -EIO;
			src += (blkcnt << blkdev->blk_bits);
			dst += (blkcnt << blkdev->blk_bits);
		}
	}

	if (part_end && dst < end) {
		buf = (uchar *) &mt_part_buf[0];
		blknr = aligned_end >> (blkdev->blk_bits);
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}
		memcpy(buf, src, part_end);
		if ((blkdev->block_write(dev_id, blknr, 1, (unsigned long*)buf, part_id)) != 1) {
			return -EIO;
		}
	}

	dprintf(INFO,"[%s:%d] ret %d\n", __FUNCTION__, __LINE__, size);
	return size;
}

static int mt_part_generic_erase(int dev_num,u64 start_addr,u64 len,u32 part_id)
{
	return -1;
}

int mt_part_register_device(part_dev_t *dev)
{
	dprintf(CRITICAL,"[mt_part_register_device]\n");
	if (!mt_part_dev) {
		if (!dev->read) {
			dev->read = mt_part_generic_read;
		}
		if (!dev->write) {
			dev->write = mt_part_generic_write;
		}
		if (!dev->erase) {
			dev->erase = mt_part_generic_erase;
		}
		mt_part_dev = dev;

		mt_part_buf = (uchar*)memalign(PART_BUF_ALIGN_SIZE, dev->blkdev->blksz * 2);
		if (mt_part_buf == NULL) {
			dprintf(CRITICAL, "allocate %lu buffer fail!\n", dev->blkdev->blksz * 2);
		}

		if (part_init(dev)) {
			dprintf(CRITICAL,"partition init error!\n");
		}
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
