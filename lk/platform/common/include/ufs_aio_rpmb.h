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

#ifndef __UFS_AIO_RPMB_H
#define __UFS_AIO_RPMB_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"
#include "ufs_aio.h"
#include "ufs_aio_hcd.h"

#if defined(MTK_UFS_DRV_LK) || defined(MTK_UFS_DRV_PRELOADER)

#define UFS_SECURITY_PROTOCOL       (0xEC)
#define RPMB_PROTOCOL_ID        (0x1)
#define RPMB_FRM_STUFF_LEN      (196)
#define RPMB_FRM_KEY_MAC_LEN        (32)
#define RPMB_FRM_DATA_LEN       (256)
#define RPMB_FRM_NONCE_LEN      (16)
#define RPMB_FRM_WR_CNT_LEN     (4)
#define RPMB_FRM_ADDR_LEN       (2)
#define RPMB_FRM_BLK_CNT_LEN        (2)
#define RPMB_FRM_RESULT_LEN     (2)
#define RPMB_FRM_REQ_RESP_LEN   (2)
#define HMAC_TEXT_LEN           (284)

#define LK_SHARED_MEM_ADDR              (0x44A02000)                /* fixed and pre-allocated in lk */

static const unsigned char rpmb_authen_key[RPMB_FRM_KEY_MAC_LEN] =
{"vutsrqponmlkjihgfedcba9876543210"};
//	{"0123456789abcdefghijklmnopqrstuv"};
static const char rpmb_oper_result[][64] = {
	"Operation OK",
	"General failure",
	"Authentication failure(MAC not match)",
	"Counter failure(Counter not match or Inc fail) ",
	"Address failure (out of range or wrong alignment)",
	"write failure (data/counter/result wite failure)",
	"Read failure (data/counter.result read failure)",
	"Authentication Key not yet programmed",
	"Secure Write Protect Configration Block access failure",
	"Invalid Secure Write Protect Block Configration parameter",
	"Secure Write Protection not applicable",
};


struct rpmb_data_frame {
	u8 stuff[RPMB_FRM_STUFF_LEN];
	u8 key_MAC[RPMB_FRM_KEY_MAC_LEN];
	u8 data[RPMB_FRM_DATA_LEN];
	u8 nonce[RPMB_FRM_NONCE_LEN];

	u32 wr_cnt;
	u16 addr;
	u16 blk_cnt;
	u16 result;
	u16 req_resp;
};

enum enum_rpmb_msg {
	RPMB_REQ_AUTHEN_KEY_PRG         = 0x0001,
	RPMB_REQ_READ_WR_CNT        = 0x0002,
	RPMB_REQ_AUTHEN_WRITE       = 0x0003,
	RPMB_REQ_AUTHEN_READ        = 0x0004,
	RPMB_REQ_RESULT_RD          = 0x0005,
	RPMB_REQ_SEC_WPCB_WRITE     = 0x0006,
	RPMB_REQ_SEC_WPCB_READ      = 0x0007,

	RPMB_RSP_AUTHEN_KEY_PRG         = 0x0100,
	RPMB_RSP_READ_WR_CNT        = 0x0200,
	RPMB_RSP_AUTHEN_WRITE       = 0x0300,
	RPMB_RSP_AUTHEN_READ        = 0x0400,
	RPMB_RSP_SEC_WPCB_WRITE     = 0x0600,
	RPMB_RSP_SEC_WPCB_READ      = 0x0700,
};



#define SWP_ENTRY_RESERVED       (3)
#define WPF_BIT                  (0)
#define WPF_SIZE                 (1)
#define WPF_MASK                 (0xFE)
#define WPT_BIT                  (1)
#define WPT_SIZE                 (2)
#define WPT_MASK                 (0xF9)
struct secure_wp_entry {
	u8 wpt_wpf;
	u8 reserved1[SWP_ENTRY_RESERVED];
	u32 upper_addr;
	u32 lower_addr;
	u32 block_num;
};

#define SWPCB_RESERVED1            (14)
#define SWPWP_ENTRY_NUM            (4)
#define SWPCB_RESERVED2            (176)
struct secure_wp_config_block {
	u8 lun;
	u8 data_length;
	u8 reserved1[SWPCB_RESERVED1];
	struct secure_wp_entry wp_entry[SWPWP_ENTRY_NUM];
	u8 reserved2[SWPCB_RESERVED2];
};


#define WP_WPF_EN                  (1)
#define WP_WPF_DISABLE             (0)
#define WP_ENTRY_SIZE             (16)
#define WP_MAX_ENTRY_SIZE         (WP_ENTRY_SIZE * SWPWP_ENTRY_NUM)
#define WP_CONF_BLOCK_SIZE        (256)


int ufs_rpmb_authen_key_program(const char *authen_key);

#endif //defined(MTK_UFS_DRV_LK)

#endif  // __UFS_AIO_RPMB_H

