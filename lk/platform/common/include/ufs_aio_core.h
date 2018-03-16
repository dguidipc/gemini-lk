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

#ifndef __UFS_AIO_CORE_H
#define __UFS_AIO_CORE_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"
#include "ufs_aio.h"
#include "ufs_aio_hcd.h"

#if defined(MTK_UFS_DRV_LK) || defined(MTK_UFS_DRV_PRELOADER) || defined(MTK_UFS_DRV_CTP)

typedef enum {
	UFS_LU_UNKNOWN = 0
	,UFS_LU_BOOT1
	,UFS_LU_BOOT2
	,UFS_LU_USER
	,UFS_LU_RPMB
	,UFS_LU_END
} ufs_logical_unit;

#endif

enum {
	UFS_MODE_DEFAULT   = 0
	,UFS_MODE_DMA       = 1
	,UFS_MODE_PIO       = 2
	,UFS_MODE_MAX       = 3
};

#define UFS_TYPE_UNKNOWN        (0)             /* Unknown UFS */
#define UFS_TYPE_EMBEDDED_UFS   (0x00000003)    /* Embedded UFS */

/* Well-known LUN */

#define WLUN_ID (1<<7)
#define WLUN_REPORT_LUNS (WLUN_ID | 0x1)
#define WLUN_UFS_DEVICE (WLUN_ID | 0x50)
#define WLUN_BOOT (WLUN_ID | 0x30)
#define WLUN_RPMB (WLUN_ID | 0x44)

int     ufs_aio_block_read(u32 host_id, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf);
int     ufs_aio_block_write(u32 host_id, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf);
int     ufs_aio_cfg_mode(u32 host_id, u8 mode);
void    ufs_aio_scsi_cmd_inquiry(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, unsigned long * buf);
void    ufs_aio_scsi_cmd_read_capacity(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, unsigned long * buf);
void    ufs_aio_scsi_cmd_read10(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, u32 lba, u32 blk_cnt, unsigned long * buf, u32 attr);
void    ufs_aio_scsi_cmd_write10(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, u32 lba, u32 blk_cnt, unsigned long * buf, u32 attr);
void    ufs_aio_scsi_cmd_test_unit_ready(struct ufs_aio_scsi_cmd *cmd, u32 tag, u32 lun);


#endif  // __UFS_AIO_CORE_H

