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

#ifndef _PART_H
#define _PART_H

#include <sys/types.h>
#include <stdint.h>


typedef ulong lbaint_t; //Hong-Rong: Add for lk porting

typedef struct block_dev_desc {
	int     if_type;    /* type of the interface */
	int     dev;        /* device number */
	unsigned char   part_type;  /* partition type */
	unsigned char   target;     /* target SCSI ID */
	unsigned char   lun;        /* target LUN */
	unsigned char   type;       /* device type */
	unsigned char   removable;  /* removable device */
#ifdef CONFIG_LBA48
	unsigned char   lba48;      /* device can use 48bit addr (ATA/ATAPI v7) */
#endif
	lbaint_t        lba;        /* number of blocks */
	unsigned long   blksz;      /* block size */
#ifdef MTK_SPI_NOR_SUPPORT
	unsigned long   blksz512;   /* block size 512 bytes */
#endif
	unsigned char   blk_bits;
	unsigned char	part_boot1;
	unsigned char	part_boot2;
	unsigned char	part_user;
	char        vendor [40+1];  /* IDE model, SCSI Vendor */
	char        product[20+1];  /* IDE Serial no, SCSI product */
	char        revision[8+1];  /* firmware revision */
#if defined(PART_DEV_API_V2) || defined(PART_DEV_API_V3)
	unsigned long   (*block_read)(int dev,
	                              unsigned long start,
	                              lbaint_t blkcnt,
	                              void *buffer,
	                              unsigned int part_id);
	unsigned long   (*block_write)(int dev,
	                               unsigned long start,
	                               lbaint_t blkcnt,
	                               const void *buffer,
	                               unsigned int part_id);
#elif defined PART_DEV_API_V1
	unsigned long   (*block_read)(int dev,
					unsigned long start,
					lbaint_t blkcnt,
					void *buffer);
	unsigned long   (*block_write)(int dev,
					unsigned long start,
					lbaint_t blkcnt,
					const void *buffer);
#endif
	void        *priv;      /* driver private struct pointer */
} block_dev_desc_t;


#endif /* _PART_H */
