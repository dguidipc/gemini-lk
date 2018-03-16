/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2010
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifndef _MSDC_H_
#define _MSDC_H_

#include "cust_msdc.h"
#include "msdc_cfg.h"
#include "msdc_utils.h"
#include "mmc_core.h"
#include "msdc_irq.h"
#include "mmc_test.h"

#if defined(MMC_MSDC_DRV_PRELOADER)
#include "platform.h"
#include "pll.h"
#include "sync_write.h"
#endif

#if defined(MMC_MSDC_DRV_LK)
#include "sync_write.h"
#include "mt_reg_base.h"
#endif

#if defined(MMC_MSDC_DRV_CTP)
#include "reg_base.H"
#include "intrCtrl.h"
#endif

#include "msdc_dma.h"

#if defined(FEATURE_MMC_SW_CMDQ)
#include "mmc_cmdq.h"
#endif

//#define MTK_MSDC_DUMP_FIFO

//#define MSDC2_3_PORTING_DON

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (64)

#define MSDC_MS                 (0)
#define MSDC_SDMMC              (1)

#define MSDC_MODE_UNKNOWN       (0)
#define MSDC_MODE_PIO           (1)
#define MSDC_MODE_DMA_BASIC     (2)
#define MSDC_MODE_DMA_DESC      (3)
#define MSDC_MODE_DMA_ENHANCED  (4)

#if defined(MSDC_MODE_DEFAULT_PIO)
#define MSDC_MODE_DEFAULT   MSDC_MODE_PIO
#elif defined(MSDC_MODE_DEFAULT_DMA_BASIC)
#define MSDC_MODE_DEFAULT   MSDC_MODE_DMA_BASIC
#elif defined(MSDC_MODE_DEFAULT_DMA_DESC)
#define MSDC_MODE_DEFAULT   MSDC_MODE_DMA_DESC
#elif defined(MSDC_MODE_DEFAULT_DMA_ENHANCED)
#define MSDC_MODE_DEFAULT   MSDC_MODE_DMA_ENHANCED
#else
#define MSDC_MODE_DEFAULT   MSDC_MODE_UNKNOWN
#endif

#define MSDC_BUS_1BITS          (0)
#define MSDC_BUS_4BITS          (1)
#define MSDC_BUS_8BITS          (2)

#define MSDC_BRUST_8B           (3)
#define MSDC_BRUST_16B          (4)
#define MSDC_BRUST_32B          (5)
#define MSDC_BRUST_64B          (6)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)

#define MSDC_PATH_USE_DELAY_LINE    (0)
#define MSDC_PATH_USE_ASYNC_FIFO    (1)

#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

#define START_AT_RISING                 (0x0)
#define START_AT_FALLING                (0x1)
#define START_AT_RISING_AND_FALLING     (0x2)
#define START_AT_RISING_OR_FALLING      (0x3)

/* Tuning Parameter */
#define DEFAULT_DEBOUNCE   (8)    /* 8 cycles */
#define DEFAULT_DTOC       (40)   /* data timeout counter. 65536x40 sclk. */
#define DEFAULT_WDOD       (0)    /* write data output delay. no delay. */
#define DEFAULT_BSYDLY     (8)    /* card busy delay. 8 extend sclk */

/* Timeout Parameter */
#define TMO_IN_CLK_2POWER   20              /* 2^20=1048576 */
#define EMMC_BOOT_TMO_IN_CLK_2POWER    16   /* 2^16=65536   */

#include "msdc_reg.h"
#include "msdc_io.h"

typedef struct {
	int    pio_bits;                   /* PIO width: 32, 16, or 8bits */
	int    autocmd;
	int    cmd23_flags;
	struct dma_config  cfg;
	struct scatterlist sg[MAX_SG_POOL_SZ];
	int    alloc_gpd;
	int    alloc_bd;
	int    rdsmpl;
	int    wdsmpl;
	int    rsmpl;
	int    start_bit;
	gpd_t *active_head;
	gpd_t *active_tail;
	gpd_t *gpd_pool;
	bd_t  *bd_pool;
} msdc_priv_t;

#define DMA_FLAG_NONE       (0x00000000)
#define DMA_FLAG_EN_CHKSUM  (0x00000001)
#define DMA_FLAG_PAD_BLOCK  (0x00000002)
#define DMA_FLAG_PAD_DWORD  (0x00000004)

#define MSDC_WRITE32(addr, data)    (*(volatile uint32*)(addr) = (uint32)(data))
#define MSDC_READ32(addr)           (*(volatile uint32*)(addr))
#define MSDC_WRITE16(addr, data)    (*(volatile uint16*)(addr) = (uint16)(data))
#define MSDC_READ16(addr)           (*(volatile uint16*)(addr))
#define MSDC_WRITE8(addr, data)     (*(volatile uint8*)(addr)  = (uint8)(data))
#define MSDC_READ8(addr)            (*(volatile uint8*)(addr))

#define MSDC_SET_BIT32(addr, mask) \
    do { \
        unsigned int tv = MSDC_READ32(addr); \
        tv |=((u32)(mask)); \
        MSDC_WRITE32(addr, tv); \
} while(0)

#define MSDC_CLR_BIT32(addr, mask) \
    do { \
        unsigned int tv = MSDC_READ32(addr); \
        tv &= ~((u32)(mask)); \
        MSDC_WRITE32(addr, tv); \
} while(0)

#define MSDC_SET_BIT16(addr, mask) \
    do { \
        unsigned short tv = MSDC_READ16(addr); \
        tv |= ((u16)(mask)); \
        MSDC_WRITE16(addr, tv); \
} while(0)

#define MSDC_CLR_BIT16(addr, mask) \
    do { \
        unsigned short tv = MSDC_READ16(addr); \
        tv &= ~((u16)(mask)); \
        MSDC_WRITE16(addr, tv); \
} while(0)

#define MSDC_SET_FIELD(reg, field, val) \
    do { \
        volatile uint32 tv = MSDC_READ32(reg); \
        tv &= ~(field); \
        tv |= ((val) << (uffs(field) - 1)); \
        MSDC_WRITE32(reg, tv); \
    } while(0)

#define MSDC_GET_FIELD(reg, field, val) \
    do { \
        volatile uint32 tv = MSDC_READ32(reg); \
        val = ((tv & (field)) >> (uffs(field) - 1)); \
    } while(0)

#define MSDC_SET_FIELD_16(reg, field, val) \
    do { \
        volatile uint16 tv = MSDC_READ16(reg); \
        tv &= ~(field); \
        tv |= ((val) << (uffs(field) - 1)); \
        MSDC_WRITE16(reg, tv); \
    } while(0)

#define MSDC_GET_FIELD_16(reg, field, val) \
    do { \
        volatile uint16 tv = MSDC_READ16(reg); \
        val = ((tv & (field)) >> (uffs(field) - 1)); \
    } while(0)

#define MSDC_RETRY(expr, retry, cnt) \
    do { \
        uint32 t = cnt; \
        uint32 r = retry; \
        uint32 c = cnt; \
        while (r) { \
            if (!(expr)) break; \
            if (c-- == 0) { \
                r--; udelay(200); c = t; \
            } \
        } \
        BUG_ON(r == 0); \
    } while(0)

/*sd card change voltage wait time= (1/freq) * SDC_VOL_CHG_CNT(default 0x145)*/
#define msdc_set_vol_change_wait_count(count) \
    do { \
            MSDC_SET_FIELD(SDC_VOL_CHG,SDC_VOL_CHG_CNT, (count)); \
    } while(0)

#define MSDC_RESET() \
    do { \
        MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_RST); \
        MSDC_RETRY(MSDC_READ32(MSDC_CFG) & MSDC_CFG_RST, 5, 1000); \
    } while(0)

#define MSDC_CLR_INT() \
    do { \
        volatile uint32 val = MSDC_READ32(MSDC_INT); \
        MSDC_WRITE32(MSDC_INT, val); \
        if (MSDC_READ32(MSDC_INT)) { \
            MSG(ERR, "[ASSERT] MSDC_INT is NOT clear\n"); \
        } \
    } while(0)

#define MSDC_CLR_FIFO() \
    do { \
        MSDC_SET_BIT32(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
        MSDC_RETRY(MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, 5, 1000); \
    } while(0)

#define MSDC_FIFO_WRITE32(val)  MSDC_WRITE32(MSDC_TXDATA, val)
#define MSDC_FIFO_READ32()      MSDC_READ32(MSDC_RXDATA)
#define MSDC_FIFO_WRITE16(val)  MSDC_WRITE16(MSDC_TXDATA, val)
#define MSDC_FIFO_READ16()      MSDC_READ16(MSDC_RXDATA)
#define MSDC_FIFO_WRITE8(val)   MSDC_WRITE8(MSDC_TXDATA, val)
#define MSDC_FIFO_READ8()       MSDC_READ8(MSDC_RXDATA)

#define MSDC_FIFO_WRITE(val)    MSDC_FIFO_WRITE32(val)
#define MSDC_FIFO_READ()        MSDC_FIFO_READ32()

#define MSDC_TXFIFOCNT() \
    ((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define MSDC_RXFIFOCNT() \
    ((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)

#define MSDC_CARD_DETECTION_ON()  MSDC_SET_BIT32(MSDC_PS, MSDC_PS_CDEN)
#define MSDC_CARD_DETECTION_OFF() MSDC_CLR_BIT32(MSDC_PS, MSDC_PS_CDEN)

#define MSDC_DMA_ON()   MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_PIO)
#define MSDC_DMA_OFF()  MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO)

#define SDC_IS_BUSY()       (MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY)
#define SDC_IS_CMD_BUSY()   (MSDC_READ32(SDC_STS) & SDC_STS_CMDBUSY)

#define SDC_SEND_CMD(cmd,arg) \
    do { \
        MSDC_WRITE32(SDC_ARG, (arg)); \
        MSDC_WRITE32(SDC_CMD, (cmd)); \
    } while(0)

#define MSDC_INIT_GPD_EX(gpd,extlen,cmd,arg,blknum) \
    do { \
        ((gpd_t*)gpd)->extlen = extlen; \
        ((gpd_t*)gpd)->cmd    = cmd; \
        ((gpd_t*)gpd)->arg    = arg; \
        ((gpd_t*)gpd)->blknum = blknum; \
    }while(0)

#define MSDC_INIT_BD(bd, blkpad, dwpad, dptr, dlen) \
    do { \
        BUG_ON(dlen > 0xFFFFFFUL); \
        ((bd_t*)bd)->blkpad = blkpad; \
        ((bd_t*)bd)->dwpad  = dwpad; \
        ((bd_t*)bd)->ptr    = (void*)dptr; \
        ((bd_t*)bd)->buflen = dlen; \
    }while(0)

typedef struct msdc_reg_control {
	u32 addr;
	u32 mask;
	u32 value;
	u32 default_value;
	int (*restore_func)(int restore);
} MSDC_REG_CONTROL_T;

#ifdef MMC_PROFILING
static inline void msdc_timer_init(void)
{
	/* clear. CLR[1]=1, EN[0]=0 */
	MSDC_WRITE32(GPT_BASE + 0x30, 0x0);
	MSDC_WRITE32(GPT_BASE + 0x30, 0x2);

	MSDC_WRITE32(GPT_BASE + 0x38, 0);
	MSDC_WRITE32(GPT_BASE + 0x3C, 32768);

	/* 32678HZ RTC free run */
	MSDC_WRITE32(GPT_BASE + 0x34, 0x30);
	MSDC_WRITE32(GPT_BASE + 0x30, 0x32);
}
static inline void msdc_timer_start(void)
{
	*(volatile unsigned int*)(GPT_BASE + 0x30) |= (1 << 0);
}
static inline void msdc_timer_stop(void)
{
	*(volatile unsigned int*)(GPT_BASE + 0x30) &= ~(1 << 0);
}
static inline void msdc_timer_stop_clear(void)
{
	*(volatile unsigned int*)(GPT_BASE + 0x30) &= ~(1 << 0); /* stop  */
	*(volatile unsigned int*)(GPT_BASE + 0x30) |= (1 << 1);  /* clear */
}
static inline unsigned int msdc_timer_get_count(void)
{
	return MSDC_READ32(GPT_BASE + 0x38);
}
#else
#define msdc_timer_init()       do{}while(0)
#define msdc_timer_start()      do{}while(0)
#define msdc_timer_stop()       do{}while(0)
#define msdc_timer_stop_clear() do{}while(0)
#define msdc_timer_get_count()  0
#endif

#define MSDC_RELIABLE_WRITE     (0x1 << 0)
#define MSDC_PACKED             (0x1 << 1)
#define MSDC_TAG_REQUEST        (0x1 << 2)
#define MSDC_CONTEXT_ID         (0x1 << 3)
#define MSDC_FORCED_PROG        (0x1 << 4)

extern void msdc_dump_card_status(u32 card_status);
extern void msdc_dump_ocr_reg(u32 resp);
extern void msdc_dump_rca_resp(u32 resp);

extern void msdc_set_reliable_write(struct mmc_host *host, int on);
extern void msdc_set_autocmd23_feature(struct mmc_host *host, int on);
extern void msdc_set_autocmd(struct mmc_host *host, int cmd, int on);
extern void msdc_set_pio_bits(struct mmc_host *host, int bits);
extern void msdc_clr_fifo(struct mmc_host *host);
extern void msdc_intr_unmask(struct mmc_host *host, u32 bits);
extern void msdc_intr_mask(struct mmc_host *host, u32 bits);
extern int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd);
extern int msdc_wait_rsp(struct mmc_host *host, struct mmc_command *cmd);
extern void msdc_abort(struct mmc_host *host);
extern void msdc_irq_deinit(struct mmc_host *host);
extern int msdc_init(int id, struct mmc_host *host, int clksrc, int mode);
extern int msdc_pio_bread(struct mmc_host *host, uchar *dst, ulong src, ulong nblks);
extern int msdc_pio_bwrite(struct mmc_host *host, ulong dst, uchar *src, ulong nblks);
extern int msdc_dma_bread(struct mmc_host *host, uchar *dst, ulong src, ulong nblks);
extern int msdc_dma_bwrite(struct mmc_host *host, ulong dst, uchar *src, ulong nblks);
extern int msdc_tune_bwrite(struct mmc_host *host, ulong dst, uchar *src, ulong nblks);
extern int msdc_tune_bread(struct mmc_host *host, uchar *dst, ulong src, ulong nblks);
extern int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd);
extern void msdc_reset_tune_counter(struct mmc_host *host);
extern int msdc_abort_handler(struct mmc_host *host, int abort_card);
extern int msdc_tune_read(struct mmc_host *host);
extern void msdc_intr_sdio(struct mmc_host *host, int enable);
extern void msdc_intr_sdio_gap(struct mmc_host * host, int enable);
extern void msdc_config_clock(struct mmc_host *host, int ddr, u32 hz, u32 hs_timing);
extern int msdc_cmd_stop(struct mmc_host *host, struct mmc_command *cmd);
extern int msdc_pio_send_sandisk_fwid(struct mmc_host *host,uchar *src);
extern int msdc_pio_get_sandisk_fwid(struct mmc_host * host,uchar * src);
extern int msdc_dma_send_sandisk_fwid(struct mmc_host *host, uchar *buf,u32 opcode, ulong nblks);
extern void msdc_set_startbit(struct mmc_host *host, u8 start_bit);
extern void msdc_set_axi_burst_len(struct mmc_host *host, u8 len);
extern void msdc_set_axi_outstanding(struct mmc_host *host, u8 rw, u8 num);
extern void msdc_set_blklen(struct mmc_host *host, u32 blklen);
extern void msdc_set_timeout(struct mmc_host *host, u64 ns, u32 clks);
extern int msdc_pio_read(struct mmc_host *host, u32 *ptr, u32 size);
extern int msdc_tune_rw_hs400(struct mmc_host *host, uchar *dst, ulong src, ulong nblks, unsigned int rw);
extern int msdc_cmd(struct mmc_host *host, struct mmc_command *cmd);
extern void msdc_set_blknum(struct mmc_host *host, u32 blknum);
extern void msdc_config_bus(struct mmc_host *host, u32 width);
extern int msdc_get_dmode(struct mmc_host *host);
extern void msdc_dump_register(struct mmc_host *host);
extern void msdc_emmc_hard_reset(struct mmc_host *host);
#if defined(MMC_MSDC_DRV_CTP) || defined(MMC_MSDC_DRV_LK)
extern void msdc_hard_reset(struct mmc_host *host);
#endif
#if defined(MMC_MSDC_DRV_CTP)
extern int msdc_register_partial_backup_and_reset(struct mmc_host* host);
extern int msdc_register_partial_restore(struct mmc_host* host);
#endif

#endif /* end of _MSDC_H_ */

