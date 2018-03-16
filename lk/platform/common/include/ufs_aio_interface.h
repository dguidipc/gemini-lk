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

#ifndef _UFS_AIO_INTERFACE_H
#define _UFS_AIO_INTERFACE_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"

#if defined(MTK_UFS_DRV_PRELOADER)
u32                 ufs_init_device(void);
#endif

#if defined(MTK_UFS_DRV_LK)
int                 ufs_lk_init(void);
int                 ufs_lk_get_active_boot_part(u32 *active_boot_part);
unsigned long long  ufs_lk_get_device_size(void);
int                 ufs_lk_erase(int dev_num, u64 start_addr, u64 len,u32 part_id);
#endif  // MTK_UFS_DRV_LK

#if defined(MTK_UFS_DRV_DA)
status_t            interface_ufs_init(u32 boot_channel);
status_t            interface_switch_ufs_section(u32 section);
status_t            interface_ufs_write(u64 address, u8* buffer, u64 length);
status_t            interface_ufs_read(u64 address, u8* buffer, u64 length);
status_t            interface_ufs_erase(u64 address, u64 length);
status_t            interface_get_ufs_info(struct ufs_info_struct* info);
status_t            interface_ufs_device_ctrl(u32 ctrl_code, void* in, u32 in_len, void* out, u32 out_len, u32* ret_len);
#endif  // MTK_UFS_DRV_DA

#endif // !_UFS_AIO_INTERFACE_H

