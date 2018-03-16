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

#ifndef _UFS_AIO_UTILS_H
#define _UFS_AIO_UTILS_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"
#include "ufs_aio.h"
#include "ufs_aio_hcd.h"

#if defined(MTK_UFS_DRV_LK)
#include <debug.h>
#include <string.h> //For memcpy, memset
#include <mt_gpt.h>
#endif

#if defined(MTK_UFS_DRV_DA)
#include <debug.h>
#include <string.h> //For memcpy, memset
#endif

/* Byte order for UTF-16 strings */
enum utf16_endian {
	UTF16_HOST_ENDIAN,
	UTF16_LITTLE_ENDIAN,
	UTF16_BIG_ENDIAN
};

extern struct ufs_ocs_error g_ufs_ocs_error_str[];

int ufs_mtk_dump_asc_ascq(struct ufs_hba *hba, u8 asc, u8 ascq);

/**
 * printing
 */

#undef MSG
#define MSG(evt, fmt, args...)
#define MSG_FUNC(f)

#undef BUG_ON
#define BUG_ON(x) \
    do { \
        if (x) { \
            while(1); \
        } \
    }while(0)

#undef WARN_ON
#define WARN_ON(x) \
    do { \
        if (x) { \
            MSG(WRN, "[WARN] %s LINE:%d\n", #x, __LINE__); \
        } \
    }while(0)

#if defined(MTK_UFS_DRV_DA)
#define printf                      LOGI
#endif

#if defined(MTK_UFS_DRV_LK)
#undef printf
#define printf(fmt, args...)        dprintf(CRITICAL, fmt, ##args)
#endif

#if defined(MTK_UFS_DRV_PRELOADER)
#define printf						print
#endif

#ifndef printf
#define printf(fmt, args...)        do{} while(0)
#endif

#if defined(MTK_UFS_DRV_DA)

/*
 * In DA, LOGE and LOGI will always print logs.
 * Use LOGE here and ensure log control is by UFS driver.
 */
#if (UFS_DBG_LVL <= UFS_DBG_LVL_FATAL)
#define UFS_DBG_LOGF LOGE
#else
#define UFS_DBG_LOGF(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_CRITICAL)
#define UFS_DBG_LOGE LOGE
#else
#define UFS_DBG_LOGE(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_INFO)
#define UFS_DBG_LOGI LOGE
#else
#define UFS_DBG_LOGI(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_DEBUG)
#define UFS_DBG_LOGD LOGE
#else
#define UFS_DBG_LOGD(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_VERBOSE)
#define UFS_DBG_LOGV LOGE
#else
#define UFS_DBG_LOGV(...) do{} while(0)
#endif

#else   // !MTK_UFS_DRV_DA

#if (UFS_DBG_LVL <= UFS_DBG_LVL_FATAL)
#define UFS_DBG_LOGF printf
#else
#define UFS_DBG_LOGF(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_CRITICAL)
#define UFS_DBG_LOGE printf
#else
#define UFS_DBG_LOGE(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_INFO)
#define UFS_DBG_LOGI printf
#else
#define UFS_DBG_LOGI(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_DEBUG)
#define UFS_DBG_LOGD printf
#else
#define UFS_DBG_LOGD(...) do{} while(0)
#endif

#if (UFS_DBG_LVL <= UFS_DBG_LVL_VERBOSE)
#define UFS_DBG_LOGV printf
#else
#define UFS_DBG_LOGV(...) do{} while(0)
#endif

#endif  // MTK_UFS_DRV_DA


#undef ARRAY_SIZE
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))

/**
 * min, max tool
 */
#ifndef min_t
#define min_t(type, x, y) ({    \
    type __min1 = (x);          \
    type __min2 = (y);          \
    __min1 < __min2 ? __min1: __min2; })

#endif

/**
 * upper_32_bits - return bits 32-63 of a number
 * @n: the number we're accessing
 *
 * A basic shift-right of a 64- or 32-bit quantity.  Use this to suppress
 * the "right shift count >= width of type" warning when that quantity is
 * 32-bits.
 */
#ifndef upper_32_bits
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#endif

/**
 * lower_32_bits - return bits 0-31 of a number
 * @n: the number we're accessing
 */
#ifndef lower_32_bits
#define lower_32_bits(n) ((u32)(n))
#endif

/**
 * for sleep and delay (mdelay and udelay)
 */
#if defined(MTK_UFS_DRV_PRELOADER)
#include "timer.h"
#elif defined(MTK_UFS_DRV_LK)
#include "mt_gpt.h"
#elif defined(MTK_UFS_DRV_DA)
#include <dev/gpt_timer/gpt_timer.h>
#endif

#ifndef msleep
#define msleep      mdelay
#endif

#ifndef usleep
#define usleep      udelay
#endif

#ifndef swab16
#define swab16(x) \
        ((u16)( \
                (((u16)(x) & (u16)0x00ffU) << 8) | \
                (((u16)(x) & (u16)0xff00U) >> 8) ))
#endif

#ifndef swab32
#define swab32(x) \
        ((u32)( \
                (((u32)(x) & (u32)0x000000ffUL) << 24) | \
                (((u32)(x) & (u32)0x0000ff00UL) <<  8) | \
                (((u32)(x) & (u32)0x00ff0000UL) >>  8) | \
                (((u32)(x) & (u32)0xff000000UL) >> 24) ))
#endif

#ifndef swab64
#define swab64(x) \
        ((u64)( \
                (((u64)(x) & (u64)0x00000000000000ffULL) << 56) | \
                (((u64)(x) & (u64)0x000000000000ff00ULL) << 40) | \
                (((u64)(x) & (u64)0x0000000000ff0000ULL) << 24) | \
                (((u64)(x) & (u64)0x00000000ff000000ULL) << 8) | \
                (((u64)(x) & (u64)0x000000ff00000000ULL) >> 8) | \
                (((u64)(x) & (u64)0x0000ff0000000000ULL) >> 24) | \
                (((u64)(x) & (u64)0x00ff000000000000ULL) >> 40) | \
                (((u64)(x) & (u64)0xff00000000000000ULL) >> 56) ))
#endif

// must ensure we are using little endian CPU

#ifndef cpu_to_be16
#define cpu_to_be16(x)  ({ u16 _x = x; swab16(_x); })
#endif

#ifndef cpu_to_be32
#define cpu_to_be32(x)  ({ u32 _x = x; swab32(_x); })
#endif

#ifndef cpu_to_be64
#define cpu_to_be64(x)  ({ u64 _x = x; swab64(_x); })
#endif

#ifndef be16_to_cpu
#define be16_to_cpu(x)  cpu_to_be16(x)
#endif

#ifndef be32_to_cpu
#define be32_to_cpu(x)  cpu_to_be32(x)
#endif

#ifndef be64_to_cpu
#define be64_to_cpu(x)  cpu_to_be64(x)
#endif

#ifndef cpu_to_le16
#define cpu_to_le16(x)  ((u16)(x))
#endif

#ifndef cpu_to_le32
#define cpu_to_le32(x)  ((u32)(x))
#endif

#ifndef le16_to_cpu
#define le16_to_cpu(x)  ((u16)(x))
#endif

#ifndef le32_to_cpu
#define le32_to_cpu(x)  ((u32)(x))
#endif

int utf16s_to_utf8s(const wchar_t *pwcs, int inlen, enum utf16_endian endian, u8 *s, int maxout);
int ufs_util_sanitize_inquiry_string(unsigned char *s, int len);

#if defined(MTK_UFS_DRV_DA)
int strcmp(const char *cs, const char *ct);
#endif

#if defined(MTK_UFS_DRV_CTP)
int strncmp(const char *s1, const char *s2, size_t n);
#endif

#endif /* _UFS_AIO_UTILS_H */

