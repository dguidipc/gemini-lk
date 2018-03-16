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

#ifndef __UFS_BLKDEV_H
#define __UFS_BLKDEV_H

#include "ufs_aio_cfg.h"

#if defined(MTK_UFS_DRV_DA) || defined(MTK_UFS_DRV_CTP)
#include "ufs_aio_types.h"

#if defined(MTK_UFS_DRV_DA)
#include "common_struct.h"
#include "sys/types.h"
#endif

/* ufs_aio_blkdev type */
enum {
	BLKDEV_NAND,
	BLKDEV_SDMMC,
	BLKDEV_NOR,
	BLKDEV_UFS,
};

typedef struct ufs_aio_blkdev ufs_aio_blkdev_t;

struct ufs_aio_blkdev {
	u32 type;       /* block device type */
	u32 blksz;      /* block size. (read/write unit) */
	u32 erasesz;    /* erase size */
	u32 blks;       /* number of blocks in the device */
	u8 *blkbuf;     /* block size buffer */
	void *priv;     /* device private data */
	ufs_aio_blkdev_t *next; /* next block device */
	int (*bread)(ufs_aio_blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u32 lun);
	int (*bwrite)(ufs_aio_blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u32 lun);
};

extern int ufs_aio_blkdev_register(ufs_aio_blkdev_t *bdev);
extern int ufs_aio_blkdev_read(ufs_aio_blkdev_t *bdev, u64 src, u64 size, u8 *dst, u8 lun);
extern int ufs_aio_blkdev_write(ufs_aio_blkdev_t *bdev, u64 dst, u64 size, u8 *src, u8 lun);
extern int ufs_aio_blkdev_bread(ufs_aio_blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u8 lun);
extern int ufs_aio_blkdev_bwrite(ufs_aio_blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u8 lun);
extern ufs_aio_blkdev_t *ufs_aio_blkdev_get(u32 type);

#endif  // MTK_UFS_DRV_DA

#endif /* __UFS_BLKDEV_H */
