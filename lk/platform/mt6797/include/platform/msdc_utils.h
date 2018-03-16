/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef _MSDC_UTILS_H_
#define _MSDC_UTILS_H_

#include "msdc_cfg.h"
#include "mmc_types.h"

#if defined(MMC_MSDC_DRV_CTP)
#include <common.h>
#endif

#if defined(MMC_MSDC_DRV_LK)
#include <debug.h>
#include <string.h> //For memcpy, memset
#include <mt_gpt.h>
#include <malloc.h>
#endif

/* Debug message event */
#define MSG_EVT_NONE        0x00000000  /* No event */
#define MSG_EVT_DMA         0x00000001  /* DMA related event */
#define MSG_EVT_CMD         0x00000002  /* MSDC CMD related event */
#define MSG_EVT_RSP         0x00000004  /* MSDC CMD RSP related event */
#define MSG_EVT_INT         0x00000008  /* MSDC INT event */
#define MSG_EVT_CFG         0x00000010  /* MSDC CFG event */
#define MSG_EVT_FUC         0x00000020  /* Function event */
#define MSG_EVT_OPS         0x00000040  /* Read/Write operation event */
#define MSG_EVT_FIO         0x00000080  /* FIFO operation event */
#define MSG_EVT_PWR         0x00000100  /* MSDC power related event */
#define MSG_EVT_INF         0x01000000  /* information event */
#define MSG_EVT_WRN         0x02000000  /* Warning event */
#define MSG_EVT_ERR         0x04000000  /* Error event */

#define MSG_EVT_ALL         0xffffffff

//#define MSG_EVT_MASK       (MSG_EVT_ALL & ~MSG_EVT_DMA & ~MSG_EVT_WRN & ~MSG_EVT_RSP & ~MSG_EVT_INT & ~MSG_EVT_CMD & ~MSG_EVT_OPS)
#define MSG_EVT_MASK       (MSG_EVT_ALL & ~MSG_EVT_FIO)
//#define MSG_EVT_MASK       (MSG_EVT_FIO)
//#define MSG_EVT_MASK       (MSG_EVT_ALL)
//#define MSG_EVT_MASK       (MSG_EVT_ALL& ~MSG_EVT_DMA& ~MSG_EVT_OPS& ~MSG_EVT_INT)
//#define MSG_EVT_MASK       (MSG_EVT_ALL& ~MSG_EVT_DMA& ~MSG_EVT_OPS)
//#define MSG_EVT_MASK       (MSG_EVT_ERR)

#undef MSG

#ifdef MSG_DEBUG
#define MSG(evt, fmt, args...) \
do {    \
    if ((MSG_EVT_##evt) & MSG_EVT_MASK) { \
        printf(fmt, ##args); \
    } \
} while(0)

#define MSG_FUNC(f) MSG(FUC, "<FUNC>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...)
#define MSG_FUNC(f)
#endif

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

#define ERR_EXIT(expr, ret, expected_ret) \
    do { \
        (ret) = (expr);\
        if ((ret) != (expected_ret)) { \
            goto exit; \
        } \
    } while(0)


#undef ARRAY_SIZE
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))

extern unsigned int msdc_uffs(unsigned int x);
extern unsigned int msdc_ntohl(unsigned int n);
extern void msdc_get_field(volatile u32 *reg, u32 field, u32 *val);

#define uffs(x)                       msdc_uffs(x)
#define ntohl(n)                      msdc_ntohl(n)
#define get_field(r,f,v)              msdc_get_field((volatile u32*)r,f,&v)

#ifndef printf
#define printf(fmt, args...)          do{}while(0)
#endif

#if defined(MMC_MSDC_DRV_LK)
#undef printf
#define printf(fmt, args...)          dprintf(CRITICAL, fmt, ##args)
#endif

#if defined(MMC_MSDC_DRV_CTP)
#include <kal_mem.h>
#define memcpy(dst,src,sz)          KAL_memcpy(dst, src, sz)
#define memset(p,v,s)               KAL_memset(p, v, s)
#define free(p)                     KAL_free(p)
#define malloc(sz)                  KAL_malloc(sz,4, KAL_USER_MSDC)
#endif

#ifndef min
#define min(x, y)   (x < y ? x : y)
#endif
#ifndef max
#define max(x, y)   (x > y ? x : y)
#endif


#if defined(MMC_MSDC_DRV_CTP)
#if !defined(FPGA_PLATFORM)
#define udelay(us) do{GPT_Delay_us(us);}while(0)
#define mdelay(ms) do{GPT_Delay_ms(ms);}while(0)

#else
#define udelay(us)  \
    do { \
        volatile int count = us * 10; \
        while (count--); \
    }while(0)

#define udelay1(us) \
    do{ \
        MSDC_WRITE32(0x10008040,0x31);\
        MSDC_WRITE32(0x10008044,0x0);\
        u32 test_t1 = MSDC_READ32(0x10008048); \
        u32 test_t2; \
        do{ \
            test_t2= MSDC_READ32(0x10008048); \
        } \
        while((test_t2-test_t1) < us * 6);\
    }while(0)

#define mdelay(ms) \
    do { \
        unsigned long i; \
        for (i = 0; i < ms; i++) \
            udelay(1000); \
    }while(0)

#define mdelay1(ms) \
    do { \
        unsigned long i; \
        for (i = 0; i < ms; i++) \
            udelay1(1000); \
    }while(0)
#endif
#endif

#define WAIT_COND(cond,tmo,left) \
    do { \
        u32 t = tmo; \
        while (1) { \
            if ((cond) || (t == 0)) break; \
            if (t > 0) { mdelay(1); t--; } \
        } \
        left = t; \
    }while(0)

#endif /* _MSDC_UTILS_H_ */

