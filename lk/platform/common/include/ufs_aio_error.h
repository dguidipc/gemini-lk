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

#ifndef __UFS_ERROR_H
#define __UFS_ERROR_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"

#if defined(MTK_UFS_DRV_DA)
#include "type_define.h"
#include "error_code.h"
#endif

// Error codes - UFS General

// Error codes - PIO specific
#define UFS_EXIT_FAILURE                 (-1) //test only
#define UFS_ERR_NONE                     (0)
#define UFS_ERR_TIMEOUT                  (1)
#define UFS_EN_HCI_ERR_TIMEOUT           (2)
#define UFS_UIC_CMD_ERR_TIMEOUT          (3)
#define UFS_UIC_CMD_ERR                  (4)
#define UFS_CPORT_DIR_ACC_ALIGN_8BYTE_ERR (5)
#define UFS_CPORT_DIR_ACC_ERR            (6)
#define UFS_CPORT_DIR_ACC_ERR_TIMEOUT    (7)
#define UFS_LINK_STARTUP_ERR             (8)
#define UFS_NOPOUT_NOPIN_ERR             (9)
#define UFS_NOPOUT_NOPIN_CPORT_TX_ERR   (10)
#define UFS_NOPOUT_NOPIN_CPORT_RX_ERR   (11)
#define UFS_READ_DESC_ERR               (12)
#define UFS_READ_DESC_CPORT_TX_ERR      (13)
#define UFS_READ_DESC_CPORT_RX_ERR      (14)
#define UFS_TEST_UNIT_RDY_ERR           (15)
#define UFS_TEST_UNIT_RDY_CPORT_TX_ERR  (16)
#define UFS_TEST_UNIT_RDY_CPORT_RX_ERR  (17)
#define UFS_TEST_UNIT_RDY_ERR_TIMEOUT   (18)
#define UFS_MEM_ALLOC_ERR               (19)
#define UFS_DATA_COMPARE_ERR            (20)
#define UFS_READ10_ERR                  (21)
#define UFS_READ10_DATA_MISMATCH_ERR    (22)
#define UFS_EFUSE_UNIPRO_ERR            (23)
#define UFS_PEER_PL_LCC_DISABLE_ERR     (24)
#define UFS_PEER_PA_LCC_DISABLE_ERR     (25)
#define UFS_READ_DME_ERR                (26)
#define UFS_SET_DME_ERR                 (27)
#define UFS_PWM_MOD_CHG_ERR             (28)
#define UFS_PWM_MOD_CHG_USR_DATA_ERR    (29)
#define UFS_PWM_MOD_CHG_WRITE_DME_ERR   (30)
#define UFS_PWM_MOD_CHG_WRITE_DME_TIMEOUT_ERR (31)
#define UFS_PWM_MOD_UIC_CHG_ERR         (32)
#define UFS_LINK_STARTUP2_ERR           (33)
#define UFS_LINK_STARTUP3_ERR           (34)
#define UFS_LINK_STARTUP4_ERR           (35)
#define UFS_LINK_STARTUP5_ERR           (36)
#define UFS_LINK_STARTUP6_ERR           (37)
#define UFS_LINK_STARTUP7_ERR           (38)
#define UFS_LINK_STARTUP8_ERR           (39)
#define UFS_POWER_ON_WP_ERR             (40)
#define UFS_POWER_ON_WP_ENTRY_FULL      (41)
#define UFS_OTP_ALREADY_LOCKED          (42)

#define UFS_ERR_NON_BOOTABLE             (-10)
#define UFS_ERR_NEED_REINIT_HOST         (-11)
#define UFS_ERR_INVALID_ERASE_SIZE       (-12)
#define UFS_ERR_INVALID_ALIGNMENT        (-13)
#define UFS_ERR_INVALID_LU               (-14)
#define UFS_ERR_TIMEOUT_QUERY_REQUEST    (-15)
#define UFS_ERR_OCS_ERROR                (-16)
#define UFS_ERR_TASK_RESP_ERROR          (-17)
#define UFS_ERR_OUT_OF_RANGE             (-18)
#define UFS_ERR_INVALID_LU_CONFIGURATION (-19)


#if defined(MTK_UFS_DRV_DA)
extern status_t ufs_aio_err_trans(int ufs_error);
#endif

#endif  // !__UFS_ERROR_H

