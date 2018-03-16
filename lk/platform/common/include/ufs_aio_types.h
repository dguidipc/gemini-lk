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

#ifndef _UFS_TYPES_H_
#define _UFS_TYPES_H_

#include "ufs_aio_cfg.h"

#if defined(MTK_UFS_DRV_LK)
#include <sys/types.h>
#include "mt_typedefs.h"
#endif

#if defined(MTK_UFS_DRV_PRELOADER)
#include "typedefs.h"
#include "stddef.h"
#endif

#if defined(MTK_UFS_DRV_DA)
#include <sys/types.h>
#include "type_define.h"
#include "string.h"
#endif

#if defined(MTK_UFS_DRV_CTP)
#include <common.h>
#include <mmu.h>  /* for addr_t */
#endif

#if !defined(MTK_UFS_DRV_PRELOADER) && !defined(MTK_UFS_DRV_DA) && !defined(MTK_UFS_DRV_LK)
typedef signed char         s8;
typedef unsigned char       u8;
typedef signed short        s16;
typedef unsigned short      u16;
typedef signed int          s32;
typedef unsigned int        u32;
typedef signed long long    s64;
typedef unsigned long long  u64;
typedef u8                  bool;

#define FALSE               0
#define TRUE                1

#define NULL                0
#endif

#define __le32 u32
#define __le16 u16
#define __be32 u32
#define __be16 u16

#if !defined(MTK_UFS_DRV_DA) && !defined(MTK_UFS_DRV_LK) && !defined(MTK_UFS_DRV_PRELOADER)
typedef u16 wchar_t;
#endif

#if defined(MTK_UFS_DRV_DA) || defined(MTK_UFS_DRV_LK)
#define ufs_vaddr_t     addr_t
#define ufs_paddr_t     paddr_t     // for DMA operations
#define ufs_size_t      size_t
#elif defined(MTK_UFS_DRV_PRELOADER)
#define ufs_vaddr_t     u32
#define ufs_paddr_t     u32         // for DMA operations
#define ufs_size_t      u32
#elif defined(MTK_UFS_DRV_CTP)
#define ufs_vaddr_t     addr_t
#define ufs_paddr_t     addr_t      // for DMA operations
#define ufs_size_t      size_t
#else
#error "MUST PORTING DMA related type definitions"
#endif

#endif /* _UFS_TYPES_H_ */

