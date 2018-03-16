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

#include "msdc.h"

#if defined(MMC_MSDC_DRV_CTP)
#include <common.h>
#include "api.h"        //For invocation cache_clean_invalidate()
#include "cache_api.h"  //For invocation cache_clean_invalidate()
#endif

#if defined(MMC_MSDC_DRV_LK)
#include <kernel/event.h>
#include <platform/mt_irq.h>
#endif

#if defined(MMC_MSDC_DRV_CTP)
#include "gpio.h"

#if defined(MSDC_USE_DCM)
#include "dcm.h"
#endif

#if !defined(FPGA_PLATFORM)
#include "pmic.h"
#include "clock_manager.h"
#endif
#endif

static int msdc_rsp[] = {
	0,  /* RESP_NONE */
	1,  /* RESP_R1 */
	2,  /* RESP_R2 */
	3,  /* RESP_R3 */
	4,  /* RESP_R4 */
	1,  /* RESP_R5 */
	1,  /* RESP_R6 */
	1,  /* RESP_R7 */
	7,  /* RESP_R1b */
};

static msdc_priv_t msdc_priv[MSDC_MAX_NUM];

void msdc_dump_card_status(u32 card_status)
{
#if MSDC_DEBUG
	static char *state[] = {
		"Idle",            /* 0 */
		"Ready",           /* 1 */
		"Ident",           /* 2 */
		"Stby",            /* 3 */
		"Tran",            /* 4 */
		"Data",            /* 5 */
		"Rcv",             /* 6 */
		"Prg",             /* 7 */
		"Dis",             /* 8 */
		"Ina",             /* 9 */
		"Sleep",           /* 10 */
		"Reserved",        /* 11 */
		"Reserved",        /* 12 */
		"Reserved",        /* 13 */
		"Reserved",        /* 14 */
		"I/O mode",        /* 15 */
	};
	if (card_status & R1_OUT_OF_RANGE)
		dprintf(CRITICAL, "\t[CARD_STATUS] Out of Range\n");
	if (card_status & R1_ADDRESS_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] Address Error\n");
	if (card_status & R1_BLOCK_LEN_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] Block Len Error\n");
	if (card_status & R1_ERASE_SEQ_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] Erase Seq Error\n");
	if (card_status & R1_ERASE_PARAM)
		dprintf(CRITICAL, "\t[CARD_STATUS] Erase Param\n");
	if (card_status & R1_WP_VIOLATION)
		dprintf(CRITICAL, "\t[CARD_STATUS] WP Violation\n");
	if (card_status & R1_CARD_IS_LOCKED)
		dprintf(CRITICAL, "\t[CARD_STATUS] Card is Locked\n");
	if (card_status & R1_LOCK_UNLOCK_FAILED)
		dprintf(CRITICAL, "\t[CARD_STATUS] Lock/Unlock Failed\n");
	if (card_status & R1_COM_CRC_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] Command CRC Error\n");
	if (card_status & R1_ILLEGAL_COMMAND)
		dprintf(CRITICAL, "\t[CARD_STATUS] Illegal Command\n");
	if (card_status & R1_CARD_ECC_FAILED)
		dprintf(CRITICAL, "\t[CARD_STATUS] Card ECC Failed\n");
	if (card_status & R1_CC_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] CC Error\n");
	if (card_status & R1_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] Error\n");
	if (card_status & R1_UNDERRUN)
		dprintf(CRITICAL, "\t[CARD_STATUS] Underrun\n");
	if (card_status & R1_OVERRUN)
		dprintf(CRITICAL, "\t[CARD_STATUS] Overrun\n");
	if (card_status & R1_CID_CSD_OVERWRITE)
		dprintf(CRITICAL, "\t[CARD_STATUS] CID/CSD Overwrite\n");
	if (card_status & R1_WP_ERASE_SKIP)
		dprintf(CRITICAL, "\t[CARD_STATUS] WP Eraser Skip\n");
	if (card_status & R1_CARD_ECC_DISABLED)
		dprintf(CRITICAL, "\t[CARD_STATUS] Card ECC Disabled\n");
	if (card_status & R1_ERASE_RESET)
		dprintf(CRITICAL, "\t[CARD_STATUS] Erase Reset\n");
	if (card_status & R1_READY_FOR_DATA)
		dprintf(CRITICAL, "\t[CARD_STATUS] Ready for Data\n");
	if (card_status & R1_SWITCH_ERROR)
		dprintf(CRITICAL, "\t[CARD_STATUS] Switch error\n");
	if (card_status & R1_URGENT_BKOPS)
		dprintf(CRITICAL, "\t[CARD_STATUS] Urgent background operations\n");
	if (card_status & R1_APP_CMD)
		dprintf(CRITICAL, "\t[CARD_STATUS] App Command\n");

	dprintf(CRITICAL, "\t[CARD_STATUS] '%s' State\n",
	       state[R1_CURRENT_STATE(card_status)]);
#endif
}

void msdc_dump_ocr_reg(u32 resp)
{
#if MSDC_DEBUG
	if (resp & (1 << 7))
		dprintf(CRITICAL, "\t[OCR] Low Voltage Range\n");
	if (resp & (1 << 15))
		dprintf(CRITICAL, "\t[OCR] 2.7-2.8 volt\n");
	if (resp & (1 << 16))
		dprintf(CRITICAL, "\t[OCR] 2.8-2.9 volt\n");
	if (resp & (1 << 17))
		dprintf(CRITICAL, "\t[OCR] 2.9-3.0 volt\n");
	if (resp & (1 << 18))
		dprintf(CRITICAL, "\t[OCR] 3.0-3.1 volt\n");
	if (resp & (1 << 19))
		dprintf(CRITICAL, "\t[OCR] 3.1-3.2 volt\n");
	if (resp & (1 << 20))
		dprintf(CRITICAL, "\t[OCR] 3.2-3.3 volt\n");
	if (resp & (1 << 21))
		dprintf(CRITICAL, "\t[OCR] 3.3-3.4 volt\n");
	if (resp & (1 << 22))
		dprintf(CRITICAL, "\t[OCR] 3.4-3.5 volt\n");
	if (resp & (1 << 23))
		dprintf(CRITICAL, "\t[OCR] 3.5-3.6 volt\n");
	if (resp & (1 << 24))
		dprintf(CRITICAL, "\t[OCR] Switching to 1.8V Accepted (S18A)\n");
	if (resp & (1 << 30))
		dprintf(CRITICAL, "\t[OCR] Card Capacity Status (CCS)\n");
	if (resp & (1UL << 31))
		dprintf(CRITICAL, "\t[OCR] Card Power Up Status (Idle)\n");
	else
		dprintf(CRITICAL, "\t[OCR] Card Power Up Status (Busy)\n");
#endif
}

void msdc_dump_io_resp(u32 resp)
{
#if MSDC_DEBUG
	u32 flags = (resp >> 8) & 0xFF;
	char *state[] = {"DIS", "CMD", "TRN", "RFU"};

	if (flags & (1 << 7))
		dprintf(CRITICAL, "\t[IO] COM_CRC_ERR\n");
	if (flags & (1 << 6))
		dprintf(CRITICAL, "\t[IO] Illgal command\n");
	if (flags & (1 << 3))
		dprintf(CRITICAL, "\t[IO] Error\n");
	if (flags & (1 << 2))
		dprintf(CRITICAL, "\t[IO] RFU\n");
	if (flags & (1 << 1))
		dprintf(CRITICAL, "\t[IO] Function number error\n");
	if (flags & (1 << 0))
		dprintf(CRITICAL, "\t[IO] Out of range\n");

	dprintf(CRITICAL, "[IO] State: %s, Data:0x%x\n", state[(resp >> 12) & 0x3], resp & 0xFF);
#endif
}

void msdc_dump_rca_resp(u32 resp)
{
#if MSDC_DEBUG
	u32 card_status = (((resp >> 15) & 0x1) << 23) |
	                  (((resp >> 14) & 0x1) << 22) |
	                  (((resp >> 13) & 0x1) << 19) |
	                  (resp & 0x1fff);

	dprintf(CRITICAL, "\t[RCA] 0x%x\n", resp >> 16);
	msdc_dump_card_status(card_status);
#endif
}

static void msdc_dump_dbg_register(struct mmc_host *host)
{
	u32 base = host->base;
	u32 i;

	for (i = 0; i <= 0x3c; i++) {
		MSDC_WRITE32(MSDC_DBG_SEL, i);
		dprintf(CRITICAL, "[SD%d]SW_DBG_SEL: write reg[%x] to 0x%x\n", host->id, OFFSET_MSDC_DBG_SEL, i);
		dprintf(CRITICAL, "[SD%d]SW_DBG_OUT: read  reg[%x] to 0x%x\n", host->id, OFFSET_MSDC_DBG_OUT, MSDC_READ32(MSDC_DBG_OUT));
	}

	MSDC_WRITE32(MSDC_DBG_SEL, 0);
}

void msdc_dump_register(struct mmc_host *host)
{
	u32 base = host->base;

	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_CFG       = 0x%x\n", host->id, OFFSET_MSDC_CFG,       MSDC_READ32(MSDC_CFG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_IOCON     = 0x%x\n", host->id, OFFSET_MSDC_IOCON,     MSDC_READ32(MSDC_IOCON));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_PS        = 0x%x\n", host->id, OFFSET_MSDC_PS,        MSDC_READ32(MSDC_PS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_INT       = 0x%x\n", host->id, OFFSET_MSDC_INT,       MSDC_READ32(MSDC_INT));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_INTEN     = 0x%x\n", host->id, OFFSET_MSDC_INTEN,     MSDC_READ32(MSDC_INTEN));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_FIFOCS    = 0x%x\n", host->id, OFFSET_MSDC_FIFOCS,    MSDC_READ32(MSDC_FIFOCS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_TXDATA    = not read\n", host->id, OFFSET_MSDC_TXDATA);
	dprintf(CRITICAL, "[SD%d] Reg[%x] MSDC_RXDATA    = not read\n", host->id, OFFSET_MSDC_RXDATA);
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_CFG        = 0x%x\n", host->id, OFFSET_SDC_CFG,        MSDC_READ32(SDC_CFG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_CMD        = 0x%x\n", host->id, OFFSET_SDC_CMD,        MSDC_READ32(SDC_CMD));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_ARG        = 0x%x\n", host->id, OFFSET_SDC_ARG,        MSDC_READ32(SDC_ARG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_STS        = 0x%x\n", host->id, OFFSET_SDC_STS,        MSDC_READ32(SDC_STS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_RESP0      = 0x%x\n", host->id, OFFSET_SDC_RESP0,      MSDC_READ32(SDC_RESP0));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_RESP1      = 0x%x\n", host->id, OFFSET_SDC_RESP1,      MSDC_READ32(SDC_RESP1));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_RESP2      = 0x%x\n", host->id, OFFSET_SDC_RESP2,      MSDC_READ32(SDC_RESP2));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_RESP3      = 0x%x\n", host->id, OFFSET_SDC_RESP3,      MSDC_READ32(SDC_RESP3));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_BLK_NUM    = 0x%x\n", host->id, OFFSET_SDC_BLK_NUM,    MSDC_READ32(SDC_BLK_NUM));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_VOL_CHG    = 0x%x\n", host->id, OFFSET_SDC_VOL_CHG,    MSDC_READ32(SDC_VOL_CHG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_CSTS       = 0x%x\n", host->id, OFFSET_SDC_CSTS,       MSDC_READ32(SDC_CSTS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_CSTS_EN    = 0x%x\n", host->id, OFFSET_SDC_CSTS_EN,    MSDC_READ32(SDC_CSTS_EN));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_DATCRC_STS = 0x%x\n", host->id, OFFSET_SDC_DCRC_STS,   MSDC_READ32(SDC_DCRC_STS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC_CFG0      = 0x%x\n", host->id, OFFSET_EMMC_CFG0,      MSDC_READ32(EMMC_CFG0));
	dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC_CFG1      = 0x%x\n", host->id, OFFSET_EMMC_CFG1,      MSDC_READ32(EMMC_CFG1));
	dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC_STS       = 0x%x\n", host->id, OFFSET_EMMC_STS,       MSDC_READ32(EMMC_STS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC_IOCON     = 0x%x\n", host->id, OFFSET_EMMC_IOCON,     MSDC_READ32(EMMC_IOCON));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_ACMD_RESP  = 0x%x\n", host->id, OFFSET_SDC_ACMD_RESP,  MSDC_READ32(SDC_ACMD_RESP));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_ACMD19_TRG = 0x%x\n", host->id, OFFSET_SDC_ACMD19_TRG, MSDC_READ32(SDC_ACMD19_TRG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SDC_ACMD19_STS = 0x%x\n", host->id, OFFSET_SDC_ACMD19_STS, MSDC_READ32(SDC_ACMD19_STS));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DMA_SA_HIGH4BIT= 0x%x\n", host->id, OFFSET_MSDC_DMA_SA_HIGH, MSDC_READ32(OFFSET_MSDC_DMA_SA_HIGH));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DMA_SA         = 0x%x\n", host->id, OFFSET_MSDC_DMA_SA,    MSDC_READ32(MSDC_DMA_SA));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DMA_CA         = 0x%x\n", host->id, OFFSET_MSDC_DMA_CA,    MSDC_READ32(MSDC_DMA_CA));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DMA_CTRL       = 0x%x\n", host->id, OFFSET_MSDC_DMA_CTRL,  MSDC_READ32(MSDC_DMA_CTRL));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DMA_CFG        = 0x%x\n", host->id, OFFSET_MSDC_DMA_CFG,   MSDC_READ32(MSDC_DMA_CFG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SW_DBG_SEL     = 0x%x\n", host->id, OFFSET_MSDC_DBG_SEL,   MSDC_READ32(MSDC_DBG_SEL));
	dprintf(CRITICAL, "[SD%d] Reg[%x] SW_DBG_OUT     = 0x%x\n", host->id, OFFSET_MSDC_DBG_OUT,   MSDC_READ32(MSDC_DBG_OUT));
	dprintf(CRITICAL, "[SD%d] Reg[%x] PATCH_BIT0     = 0x%x\n", host->id, OFFSET_MSDC_PATCH_BIT0,MSDC_READ32(MSDC_PATCH_BIT0));
	dprintf(CRITICAL, "[SD%d] Reg[%x] PATCH_BIT1     = 0x%x\n", host->id, OFFSET_MSDC_PATCH_BIT1,MSDC_READ32(MSDC_PATCH_BIT1));
	dprintf(CRITICAL, "[SD%d] Reg[%x] PATCH_BIT2     = 0x%x\n", host->id, OFFSET_MSDC_PATCH_BIT2,MSDC_READ32(MSDC_PATCH_BIT2));
	dprintf(CRITICAL, "[SD%d] Reg[%x] PAD_TUNE0      = 0x%x\n", host->id, OFFSET_MSDC_PAD_TUNE0, MSDC_READ32(MSDC_PAD_TUNE0));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DAT_RD_DLY0    = 0x%x\n", host->id, OFFSET_MSDC_DAT_RDDLY0,MSDC_READ32(MSDC_DAT_RDDLY0));
	dprintf(CRITICAL, "[SD%d] Reg[%x] DAT_RD_DLY1    = 0x%x\n", host->id, OFFSET_MSDC_DAT_RDDLY1,MSDC_READ32(MSDC_DAT_RDDLY1));
	dprintf(CRITICAL, "[SD%d] Reg[%x] HW_DBG_SEL     = 0x%x\n", host->id, OFFSET_MSDC_HW_DBG,    MSDC_READ32(MSDC_HW_DBG));
	dprintf(CRITICAL, "[SD%d] Reg[%x] MAIN_VER       = 0x%x\n", host->id, OFFSET_MSDC_VERSION,   MSDC_READ32(MSDC_VERSION));

	if (host->id == 0) {
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_CTL0          = 0x%x\n", host->id, OFFSET_EMMC50_PAD_CTL0,       MSDC_READ32(EMMC50_PAD_CTL0));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_DS_CTL0       = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DS_CTL0,    MSDC_READ32(EMMC50_PAD_DS_CTL0));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_DS_TUNE       = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DS_TUNE,    MSDC_READ32(EMMC50_PAD_DS_TUNE));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_CMD_TUNE      = 0x%x\n", host->id, OFFSET_EMMC50_PAD_CMD_TUNE,   MSDC_READ32(EMMC50_PAD_CMD_TUNE));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_DAT01_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT01_TUNE, MSDC_READ32(EMMC50_PAD_DAT01_TUNE));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_DAT23_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT23_TUNE, MSDC_READ32(EMMC50_PAD_DAT23_TUNE));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_DAT45_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT45_TUNE, MSDC_READ32(EMMC50_PAD_DAT45_TUNE));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_PAD_DAT67_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT67_TUNE, MSDC_READ32(EMMC50_PAD_DAT67_TUNE));

		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC51_CFG0              = 0x%x\n", host->id, OFFSET_EMMC51_CFG0,    MSDC_READ32(EMMC51_CFG0));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_CFG0              = 0x%x\n", host->id, OFFSET_EMMC50_CFG0,    MSDC_READ32(EMMC50_CFG0));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_CFG1              = 0x%x\n", host->id, OFFSET_EMMC50_CFG1,    MSDC_READ32(EMMC50_CFG1));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_CFG2              = 0x%x\n", host->id, OFFSET_EMMC50_CFG2,    MSDC_READ32(EMMC50_CFG2));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_CFG3              = 0x%x\n", host->id, OFFSET_EMMC50_CFG3,    MSDC_READ32(EMMC50_CFG3));
		dprintf(CRITICAL, "[SD%d] Reg[%x] EMMC50_CFG4              = 0x%x\n", host->id, OFFSET_EMMC50_CFG4,    MSDC_READ32(EMMC50_CFG4));
	}

	msdc_dump_dbg_register(host);
}

#if defined(MMC_MSDC_DRV_CTP)
#define HS400_BACKUP_REG_NUM (42)
static struct msdc_reg_control hs400_backup_reg_list[HS400_BACKUP_REG_NUM] = {
	//{addr,                                         mask,                                value,     default value, func},
	{(MSDC0_BASE + OFFSET_MSDC_PATCH_BIT1),        (MSDC_PB1_WRDAT_CRCS_TA_CNTR),       0x0,       0x1,         NULL},//0xB4[2:0],
	{(MSDC0_BASE + OFFSET_MSDC_PATCH_BIT0),        (MSDC_PB0_INT_DAT_LATCH_CK_SEL),     0x0,       0x0,         NULL},//0xB0[9:7]
	{(MSDC0_BASE + OFFSET_MSDC_IOCON),             (MSDC_IOCON_R_D_SMPL),               0x0,       0x0,         NULL},//0x04[2:2]
	{(MSDC0_BASE + OFFSET_MSDC_PAD_TUNE0),         (MSDC_PAD_TUNE0_DATRRDLY),           0x0,       0x0,         NULL},//0xEC[12:8]
	{(MSDC0_BASE + OFFSET_MSDC_IOCON),             (MSDC_IOCON_DDLSEL),                 0x0,       0x0,         NULL},//0x04[3:3]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY0),        (MSDC_DAT_RDDLY0_D3),                0x0,       0x0,         NULL},//0xF0[4:0]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY0),        (MSDC_DAT_RDDLY0_D2),                0x0,       0x0,         NULL},//0xF0[12:8]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY0),        (MSDC_DAT_RDDLY0_D1),                0x0,       0x0,         NULL},//0xF0[20:16]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY0),        (MSDC_DAT_RDDLY0_D0),                0x0,       0x0,         NULL},//0xF0[28:24]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY1),        (MSDC_DAT_RDDLY1_D7),                0x0,       0x0,         NULL},//0xF4[4:0]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY1),        (MSDC_DAT_RDDLY1_D6),                0x0,       0x0,         NULL},//0xF4[12:8]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY1),        (MSDC_DAT_RDDLY1_D5),                0x0,       0x0,         NULL},//0xF4[20:16]
	{(MSDC0_BASE + OFFSET_MSDC_DAT_RDDLY1),        (MSDC_DAT_RDDLY1_D4),                0x0,       0x0,         NULL},//0xF4[28:24]
	{(MSDC0_BASE + OFFSET_MSDC_IOCON),             (MSDC_IOCON_R_D_SMPL_SEL),           0x0,       0x0,         NULL},//0x04[5:5]
	{(MSDC0_BASE + OFFSET_MSDC_IOCON),             (MSDC_IOCON_R_D0SPL),                0x0,       0x0,         NULL},//0x04[16:16]
	{(MSDC0_BASE + OFFSET_MSDC_IOCON),             (MSDC_IOCON_W_D_SMPL),               0x0,       0x0,         NULL},//0x04[8:8]
	{(MSDC0_BASE + OFFSET_MSDC_PAD_TUNE0),         (MSDC_PAD_TUNE0_DATWRDLY),            0x0,       0x0,         NULL},//0xEC[4:0]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT0_RXDLY3SEL),    0x0,       0x0,         NULL},//0x190[0:0]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT1_RXDLY3SEL),    0x0,       0x0,         NULL},//0x190[16:16]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT2_RXDLY3SEL),    0x0,       0x0,         NULL},//0x194[0:0]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT3_RXDLY3SEL),    0x0,       0x0,         NULL},//0x194[16:16]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT4_RXDLY3SEL),    0x0,       0x0,         NULL},//0x198[0:0]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT5_RXDLY3SEL),    0x0,       0x0,         NULL},//0x198[16:16]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT6_RXDLY3SEL),    0x0,       0x0,         NULL},//0x19C[0:0]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT7_RXDLY3SEL),    0x0,       0x0,         NULL},//0x19C[16:16]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT0_RXDLY3),       0x0,       0x0,         NULL},//0x190[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT1_RXDLY3),       0x0,       0x0,         NULL},//0x190[21:17]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT2_RXDLY3),       0x0,       0x0,         NULL},//0x194[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT3_RXDLY3),       0x0,       0x0,         NULL},//0x194[21:17]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT4_RXDLY3),       0x0,       0x0,         NULL},//0x198[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT5_RXDLY3),       0x0,       0x0,         NULL},//0x198[21:17]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT6_RXDLY3),       0x0,       0x0,         NULL},//0x19C[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT7_RXDLY3),       0x0,       0x0,         NULL},//0x19C[21:17]


	/* _HQA asked cmd line delay 8 and dat line delay 4 under hs400 mode */
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_CMD_TUNE),    (MSDC_EMMC50_PAD_CMD_TUNE_TXDLY),    0x0,       0x8,         NULL},//0x190[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT0_TXDLY),        0x0,       0x4,         NULL},//0x190[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT1_TXDLY),        0x0,       0x4,         NULL},//0x190[21:17]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT2_TXDLY),        0x0,       0x4,         NULL},//0x194[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT3_TXDLY),        0x0,       0x4,         NULL},//0x194[21:17]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT4_TXDLY),        0x0,       0x4,         NULL},//0x198[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT5_TXDLY),        0x0,       0x4,         NULL},//0x198[21:17]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT6_TXDLY),        0x0,       0x4,         NULL},//0x19C[5:1]
	{(MSDC0_BASE + OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT7_TXDLY),        0x0,       0x4,         NULL},//0x19C[21:17]
};

/* need reset some register while switch to hs400 mode with emmc50
 * do stress test need change mode from hs400 to others, so need backup if switched */
int msdc_register_partial_backup_and_reset(struct mmc_host* host)
{
	int i = 0, err = 0;

	for (i = 0; i < HS400_BACKUP_REG_NUM; i++) {
		MSDC_GET_FIELD(hs400_backup_reg_list[i].addr, hs400_backup_reg_list[i].mask,  hs400_backup_reg_list[i].value);
		MSDC_SET_FIELD(hs400_backup_reg_list[i].addr, hs400_backup_reg_list[i].mask,  hs400_backup_reg_list[i].default_value);
		if (hs400_backup_reg_list[i].restore_func) {
			err = hs400_backup_reg_list[i].restore_func(0);
			if (err) {
				dprintf(CRITICAL, "[%s]: failed to restore reg[0x%x][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
				       __func__, hs400_backup_reg_list[i].addr, hs400_backup_reg_list[i].mask, hs400_backup_reg_list[i].default_value, MSDC_READ32(hs400_backup_reg_list[i].addr), err);
			}
		}
	}

	return 0;
}

int msdc_register_partial_restore(struct mmc_host* host)
{
	int i = 0, err = 0;

	for (i = 0; i < HS400_BACKUP_REG_NUM; i++) {
		MSDC_SET_FIELD(hs400_backup_reg_list[i].addr, hs400_backup_reg_list[i].mask,  hs400_backup_reg_list[i].value);
		if (hs400_backup_reg_list[i].restore_func) {
			err = hs400_backup_reg_list[i].restore_func(1);
			if (err) {
				dprintf(CRITICAL, "[%s]:failed to restore reg[0x%x][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
				       __func__, hs400_backup_reg_list[i].addr, hs400_backup_reg_list[i].mask, hs400_backup_reg_list[i].value, MSDC_READ32(hs400_backup_reg_list[i].addr), err);
			}
		}
	}

	return 0;
}
#endif

static void msdc_dump_info(struct mmc_host *host)
{
	// 1: dump msdc hw register
	msdc_dump_register(host);

	// 2: For designer
	msdc_dump_dbg_register(host);

#if !defined(FPGA_PLATFORM)
	// 3: check msdc clock gate and clock source
	msdc_dump_clock_sts(host);

	// 4: check msdc pmic ldo
	msdc_dump_ldo_sts(host);

	// 5: check msdc gpio
#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_padctl_by_id(host->id);
#endif
#endif
}

void msdc_clr_fifo(struct mmc_host *host)
{
	u32 base = host->base;
	MSDC_CLR_FIFO();
}

void msdc_reset(struct mmc_host *host)
{
	u32 base = host->base;
	MSDC_RESET();
}

void msdc_abort(struct mmc_host *host)
{
	u32 base = host->base;

	MSG(INF, "[SD%d] Abort: MSDC_FIFOCS=%xh MSDC_PS=%xh SDC_STS=%xh\n",
	    host->id, MSDC_READ32(MSDC_FIFOCS), MSDC_READ32(MSDC_PS), MSDC_READ32(SDC_STS));

	/* reset controller */
	msdc_reset(host);

	/* clear fifo */
	msdc_clr_fifo(host);

	/* make sure txfifo and rxfifo are empty */
	if (MSDC_TXFIFOCNT() != 0 || MSDC_RXFIFOCNT() != 0) {
		MSG(INF, "[SD%d] Abort: TXFIFO(%d), RXFIFO(%d) != 0\n",
		    host->id, MSDC_TXFIFOCNT(), MSDC_RXFIFOCNT());
	}

	/* clear all interrupts */
	MSDC_WRITE32(MSDC_INT, MSDC_READ32(MSDC_INT));
}



void msdc_set_axi_burst_len(struct mmc_host *host, u8 len)
{
	u32 base = host->base;

	/* set axi burst len */
	MSDC_SET_FIELD(EMMC50_CFG2, MSDC_EMMC50_CFG2_AXI_SET_LEN, len);
}

void msdc_set_axi_outstanding(struct mmc_host *host, u8 rw, u8 num)
{
	u32 base = host->base;

	/* set axi outstanding num */
	if (rw == 0) /* read */
		MSDC_SET_FIELD(EMMC50_CFG2, MSDC_EMMC50_CFG2_AXI_RD_OUTS_NUM, num);
	else /* write */
		MSDC_SET_FIELD(EMMC50_CFG3, MSDC_EMMC50_CFG3_OUTS_WR, num);
}

void msdc_set_startbit(struct mmc_host *host, u8 start_bit)
{
	u32 base = host->base;
	u32 l_start_bit;
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	if (host->id != 0) {
		return;
	}

	/* set start bit */
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_START_BIT, start_bit);
	priv->start_bit = start_bit;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_START_BIT, l_start_bit);
#if 1
	switch (l_start_bit) {
		case 0:
			dprintf(CRITICAL, "[info][%s %d] read data start bit at rising edge\n", __func__, __LINE__);
			break;
		case 1:
			dprintf(CRITICAL, "[info][%s %d] read data start bit at falling edge\n", __func__, __LINE__);
			break;
		case 2:
			dprintf(CRITICAL, "[info][%s %d] read data start bit at rising & falling edge\n", __func__, __LINE__);
			break;
		case 3:
			dprintf(CRITICAL, "[info][%s %d] read data start bit at rising | falling edge\n", __func__, __LINE__);
			break;
		default:
			break;
	}
#endif
}

void msdc_set_smpl(struct mmc_host *host, u8 HS400, u8 mode, u8 type)
{
	u32 base = host->base;
	int i=0;
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	static u8 read_data_edge[8] = {MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING,
	                               MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING
	                              };
	static u8 write_data_edge[4] = {MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING};

	switch (type) {
		case TYPE_CMD_RESP_EDGE:
			if (HS400) {
				// eMMC5.0 only output resp at CLK pin, so no need to select DS pin
				MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_PADCMD_LATCHCK, 0); //latch cmd resp at CLK pin
				MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL, 0);//latch cmd resp
				// 0: from delay path for eMMC4.5
				// 1: from FIFO path for eMMC5.0
			}

			if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
#if 0 // HS400 tune MSDC_EMMC50_CFG_CMD_EDGE_SEL use DS latch, but now no DS latch
				if (HS400) {
					MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_EDGE_SEL, mode);
				} else {
					MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
				}
#else
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
#endif
				priv->rsmpl = mode;
			} else {
				dprintf(CRITICAL, "[%s]: SD%d invalid resp parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
			}
			break;

		case TYPE_WRITE_CRC_EDGE:
			if (HS400) {
				MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 1);//latch write crc status at DS pin
			} else {
				MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin
			}

			if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
				if (HS400) {
					MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_EDGE, mode);
				} else {
					MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
					MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
				}
				priv->wdsmpl = mode;
			} else if (mode == MSDC_SMPL_SEPERATE && !HS400) {
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D0SPL, write_data_edge[0]); //only dat0 is for write crc status.
				priv->wdsmpl = mode;
			} else {
				dprintf(CRITICAL, "[%s]: SD%d invalid crc parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
			}
			break;

		case TYPE_READ_DATA_EDGE:
			if (HS400) {
				msdc_set_startbit(host, START_AT_RISING_AND_FALLING); //for HS400, start bit is output both on rising and falling edge
				priv->start_bit = START_AT_RISING_AND_FALLING;
			} else {
				//for the other mode, start bit is only output on rising edge. but DDR50 can try falling edge if error casued by pad delay
				if (mmc_card_ddr(host->card)) {
					msdc_set_startbit(host, mode);
					priv->start_bit = mode;
				} else {
					msdc_set_startbit(host, START_AT_RISING);
					priv->start_bit = START_AT_RISING;
				}
			}
			if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, mode);
				priv->rdsmpl = mode;
			} else if (mode == MSDC_SMPL_SEPERATE) {
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 1);
				for (i=0; i<8; i++);
				{
					MSDC_SET_FIELD(MSDC_IOCON, (MSDC_IOCON_R_D0SPL << i), read_data_edge[i]);
				}
				priv->rdsmpl = mode;
			} else {
				dprintf(CRITICAL, "[%s]: SD%d invalid read parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
			}
			break;

		case TYPE_WRITE_DATA_EDGE:
			MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin

			if (mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING) {
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
				priv->wdsmpl = mode;
			} else if (mode == MSDC_SMPL_SEPERATE) {
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 1);
				for (i=0; i<4; i++);
				{
					MSDC_SET_FIELD(MSDC_IOCON, (MSDC_IOCON_W_D0SPL << i), write_data_edge[i]);//dat0~4 is for SDIO card.
				}
				priv->wdsmpl = mode;
			} else {
				dprintf(CRITICAL, "[%s]: SD%d invalid write parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
			}
			break;

		default:
			printf("[%s]: SD%d invalid parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
			break;
	}
}

static u32 msdc_cal_timeout(struct mmc_host *host, u64 ns, u32 clks, u32 clkunit)
{
	u32 timeout, clk_ns;

	clk_ns  = 1000000000UL / host->cur_bus_clk;
	timeout = ns / clk_ns + clks;
	timeout = timeout / clkunit;
	return timeout;
}

void msdc_set_timeout(struct mmc_host *host, u64 ns, u32 clks)
{
	u32 base = host->base;
	u32 timeout, clk_ns;
	u32 mode = 0;

	if (host->cur_bus_clk == 0) {
		timeout = 0;
	} else {
		clk_ns  = 1000000000UL / host->cur_bus_clk;
		timeout = (ns + clk_ns - 1) / clk_ns + clks;
		timeout = (timeout + (1 << TMO_IN_CLK_2POWER) - 1) >> TMO_IN_CLK_2POWER; /* in 1048576 sclk cycle unit */
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, mode);
		timeout = mode >= 2 ? timeout * 2 : timeout; //DDR mode will double the clk cycles for data timeout
		timeout = timeout > 1 ? timeout - 1 : 0;
		timeout = timeout > 255 ? 255 : timeout;
	}
	MSDC_SET_FIELD(SDC_CFG, SDC_CFG_DTOC, timeout);

	MSG(OPS, "[SD%d] Set read data timeout: %dus %dclks -> %d x 1048576 cycles, mode:%d, clk_freq=%dKHz\n",
	    host->id, (u32)(ns/1000), clks, timeout + 1, mode, (host->cur_bus_clk / 1000));
}

void msdc_set_blklen(struct mmc_host *host, u32 blklen)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	host->blklen     = blklen;
	priv->cfg.blklen = blklen;
	msdc_clr_fifo(host);
}

void msdc_set_blknum(struct mmc_host *host, u32 blknum)
{
	u32 base = host->base;
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	/* autocmd23 with packed cmd, this feature is conflict with data tag, reliable write, and force flush cache */
#if defined(MMC_MSDC_DRV_CTP)
	if (priv->autocmd & MSDC_AUTOCMD23) {
#if MSDC_USE_DATA_TAG
		blknum |= (1 << 29);
		blknum &= ~(1 << 30);
#endif
#if MSDC_USE_RELIABLE_WRITE
		blknum |= (1 << 31);
		blknum &= ~(1 << 30);
#endif
#if MSDC_USE_FORCE_FLUSH
		blknum |= (1 << 24);
		blknum &= ~(1 << 30);
#endif
#if MSDC_USE_PACKED_CMD
		blknum &= ~0xffff;
		blknum |= (1 << 30);
#endif
	}
#endif
	if (priv->cmd23_flags & MSDC_RELIABLE_WRITE) {
		blknum |= (1 << 31);
		blknum &= ~(1 << 30);
	}

	MSDC_WRITE32(SDC_BLK_NUM, blknum);
}

void msdc_set_dmode(struct mmc_host *host, int mode)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;
#if defined(MSDC_ENABLE_DMA_MODE)
	priv->cfg.mode = mode;
#endif

	if (mode == MSDC_MODE_PIO) {
		host->blk_read  = msdc_pio_bread;
		host->blk_write = msdc_pio_bwrite;
#if defined(MSDC_ENABLE_DMA_MODE)
	} else {
		host->blk_read  = msdc_dma_bread;
		host->blk_write = msdc_dma_bwrite;
#endif
	}
}

int msdc_get_dmode(struct mmc_host *host)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;
	int mode = priv->cfg.mode;
	return mode;
}

void msdc_set_pio_bits(struct mmc_host *host, int bits)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	priv->pio_bits = bits;
}

void msdc_set_autocmd(struct mmc_host *host, int cmd, int on)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	if (on) {
		priv->autocmd |= cmd;
	} else {
		priv->autocmd &= ~cmd;
	}
}

void msdc_set_reliable_write(struct mmc_host *host, int on)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;

	if (on) {
		priv->cmd23_flags |= MSDC_RELIABLE_WRITE;
	} else {
		priv->cmd23_flags &= ~MSDC_RELIABLE_WRITE;
	}
}

void msdc_set_autocmd23_feature(struct mmc_host *host, int on)
{
	u32 base = host->base;
	if (on) {
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_BLKNUM_SEL, 0);
	} else {
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_BLKNUM_SEL, 1);
	}
}

int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;
	u32 base   = host->base;
	u32 opcode = cmd->opcode;
	u32 rsptyp;
	u32 rawcmd;
	u32 timeout = cmd->timeout;
	u32 error = MMC_ERR_NONE;
#ifdef FEATURE_MMC_CMDQ
	u32 cmdq_reg_setting=0;

	if (opcode == MMC_SET_QUEUE_CONTEXT) {
		cmd->rsptyp = RESP_R1;
		cmdq_reg_setting=(MMC_SET_QUEUE_CONTEXT<<1)|1;
	} else if (opcode == MMC_SET_QUEUE_ADDRESS) {
		cmd->rsptyp = RESP_R1;
		cmdq_reg_setting=(MMC_SET_QUEUE_ADDRESS<<1)|1;
	} else if (opcode == MMC_WRITE_REQUESTED_QUEUE || opcode == MMC_READ_REQUESTED_QUEUE) {
		cmd->rsptyp = RESP_R1;
	} else if ( opcode==MMC_CMD_SEND_STATUS ) {
		cmd->rsptyp = RESP_R1;
		if ( MSDC_READ32(EMMC51_CFG0)&0x1 ) {
			cmdq_reg_setting=(MMC_CMD_SEND_STATUS<<1)|1;
		}
	}
	//Chiachun: for SS vendor command
	else if (opcode == 62) { //Light: To be clarified
		cmd->rsptyp = RESP_R1;
	}

#if !defined(FEATURE_MMC_USE_EMMC51_CFG0_FOR_CMD44_45)
	cmdq_reg_setting=0;
#endif
#endif

	rsptyp=cmd->rsptyp;

	/* rawcmd :
	 * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
	 * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
	 */
	rawcmd = (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)) |
	         msdc_rsp[rsptyp] << 7 | host->blklen << 16;

	if (opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
		rawcmd |= ((2 << 11) | (1 << 13));
		if (priv->autocmd & MSDC_AUTOCMD12) {
			rawcmd |= (1 << 28);
		} else if (priv->autocmd & MSDC_AUTOCMD23) {
			rawcmd |= (2 << 28);
		}
	} else if (opcode == MMC_CMD_WRITE_BLOCK || opcode == MMC_CMD50) {
		rawcmd |= ((1 << 11) | (1 << 13));
	} else if (opcode == MMC_CMD_READ_MULTIPLE_BLOCK) {
		rawcmd |= (2 << 11);
		if (priv->autocmd & MSDC_AUTOCMD12) {
			rawcmd |= (1 << 28);
		} else if (priv->autocmd & MSDC_AUTOCMD23) {
			rawcmd |= (2 << 28);
		}
	} else if (opcode == MMC_CMD_READ_SINGLE_BLOCK ||
	           opcode == SD_ACMD_SEND_SCR ||
	           opcode == SD_CMD_SWITCH ||
	           opcode == MMC_CMD_SEND_EXT_CSD ||
	           opcode == MMC_CMD_SEND_WRITE_PROT ||
	           opcode == MMC_CMD_SEND_WRITE_PROT_TYPE ||
	           opcode == MMC_CMD21) {
		rawcmd |= (1 << 11);
	} else if (opcode == MMC_CMD_STOP_TRANSMISSION) {
		rawcmd |= (1 << 14);
		rawcmd &= ~(0x0FFF << 16);
	} else if (opcode == SD_IO_RW_EXTENDED) {
		if (cmd->arg & 0x80000000)  /* R/W flag */
			rawcmd |= (1 << 13);
		if ((cmd->arg & 0x08000000) && ((cmd->arg & 0x1FF) > 1))
			rawcmd |= (2 << 11); /* multiple block mode */
		else
			rawcmd |= (1 << 11);
	} else if (opcode == SD_IO_RW_DIRECT) {
		if ((cmd->arg & 0x80000000) && ((cmd->arg >> 9) & 0x1FFFF))/* I/O abt */
			rawcmd |= (1 << 14);
	} else if (opcode == SD_CMD_VOL_SWITCH) {
		rawcmd |= (1 << 30);
	} else if (opcode == SD_CMD_SEND_TUNING_BLOCK) {
		rawcmd |= (1 << 11); /* CHECKME */
		if (priv->autocmd & MSDC_AUTOCMD19)
			rawcmd |= (3 << 28);
	} else if (opcode == MMC_CMD_GO_IRQ_STATE) {
		rawcmd |= (1 << 15);
	} else if (opcode == MMC_CMD_WRITE_DAT_UNTIL_STOP) {
		rawcmd |= ((1<< 13) | (3 << 11));
	} else if (opcode == MMC_CMD_READ_DAT_UNTIL_STOP) {
		rawcmd |= (3 << 11);
	}
#ifdef FEATURE_MMC_CMDQ
	else if (opcode == MMC_READ_REQUESTED_QUEUE) {
		rawcmd |= (2 << 11);
	} else if (opcode == MMC_WRITE_REQUESTED_QUEUE) {
		rawcmd |= ((2 << 11) | (1 << 13));
		//} else if ( (opcode==MMC_CMD_SEND_STATUS) && (cmd->arg&SEND_QUEUE_STATUS_SQS_BIT_SHIFT) ) {
	}
#endif

	MSG(CMD, "[SD%d] CMD(%d): ARG(0x%x), RAW(0x%x), BLK_NUM(0x%x) RSP(%d)\n",
	    host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)), cmd->arg, rawcmd, MSDC_READ32(SDC_BLK_NUM), rsptyp);

#ifdef FEATURE_MMC_CMDQ
	if ( cmdq_reg_setting ) {
		//cmdq_reg_setting|=(0x6a<<22)|(0x6a<<12)|(msdc_rsp[rsptyp]<<7);
		cmdq_reg_setting|=(msdc_rsp[rsptyp]<<7);

		//printf("busy status: %x\n", MSDC_READ32(SDC_STS));
		if (SDC_IS_CMD_BUSY()) {
			WAIT_COND(!SDC_IS_CMD_BUSY(), cmd->timeout, timeout);
			if (timeout == 0) {
				error = MMC_ERR_TIMEOUT;
				printf("[SD%d] CMD(%d): SDC_IS_CMD_BUSY timeout\n",
				       host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)));
				goto end;
			}
		}

		MSDC_SET_FIELD(EMMC51_CFG0, 0xfff, (cmdq_reg_setting));
		SDC_SEND_CMD(0, cmd->arg);
		goto end;
	}
#endif

	if (opcode == MMC_CMD_SEND_STATUS) {
		if (SDC_IS_CMD_BUSY()) {
			WAIT_COND(!SDC_IS_CMD_BUSY(), cmd->timeout, timeout);
			if (timeout == 0) {
				error = MMC_ERR_TIMEOUT;
				printf("[SD%d] CMD(%d): SDC_IS_CMD_BUSY timeout\n",
				       host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)));
				goto end;
			}
		}
	} else {
		if (SDC_IS_BUSY()) {
			WAIT_COND(!SDC_IS_BUSY(), 1000, timeout);
			if (timeout == 0) {
				error = MMC_ERR_TIMEOUT;
				printf("[SD%d] CMD(%d): SDC_IS_BUSY timeout\n",
				       host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)));
				goto end;
			}
		}
	}

#ifdef FEATURE_MMC_CMDQ
	MSDC_SET_FIELD(EMMC51_CFG0, 0xfff, 0);
#endif

	SDC_SEND_CMD(rawcmd, cmd->arg);

end:
	cmd->error = error;

	return error;
}

int msdc_wait_rsp(struct mmc_host *host, struct mmc_command *cmd)
{
	u32 base   = host->base;
	u32 rsptyp = cmd->rsptyp;
	u32 status;
	u32 opcode = (cmd->opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT));
	u32 error = MMC_ERR_NONE;
	u32 wints = MSDC_INT_CMDTMO | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR |
	            MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO |
	            MSDC_INT_ACMD19_DONE;

#if defined(FEATURE_MMC_SDIO)
	wints |= MSDC_INT_SDIOIRQ;
#endif

	if (cmd->opcode == MMC_CMD_GO_IRQ_STATE)
		wints |= MSDC_INT_MMCIRQ;

	status = msdc_intr_wait(host, wints);
#if defined(FEATURE_MMC_SDIO)
	if (status & MSDC_INT_SDIOIRQ) {
		if (mmc_card_sdio(host->card)) {
			struct sdio_func *func = host->card->io_func[0];
			if (func->irq_handler)
				func->irq_handler(func);
		}
	}
#endif
	if (status == 0) {
		error = MMC_ERR_TIMEOUT;
		goto end;
	}

	if ((status & MSDC_INT_CMDRDY) || (status & MSDC_INT_ACMDRDY) ||
	        (status & MSDC_INT_ACMD19_DONE)) {
		switch (rsptyp) {
			case RESP_NONE:
				MSG(RSP, "[SD%d] CMD(%d): RSP(%d)\n", host->id, opcode, rsptyp);
				break;
			case RESP_R2: {
				u32 *resp = &cmd->resp[0];
				*resp++ = MSDC_READ32(SDC_RESP3);
				*resp++ = MSDC_READ32(SDC_RESP2);
				*resp++ = MSDC_READ32(SDC_RESP1);
				*resp++ = MSDC_READ32(SDC_RESP0);
				MSG(RSP, "[SD%d] CMD(%d): RSP(%d) = 0x%x 0x%x 0x%x 0x%x\n",
				    host->id, opcode, cmd->rsptyp, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
				break;
			}
			default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
				if ((status & MSDC_INT_ACMDRDY) || (status & MSDC_INT_ACMD19_DONE))
					cmd->resp[0] = MSDC_READ32(SDC_ACMD_RESP);
				else
					cmd->resp[0] = MSDC_READ32(SDC_RESP0);
				MSG(RSP, "[SD%d] CMD(%d): RSP(%d) = 0x%x AUTO(%d)\n", host->id, opcode,
				    cmd->rsptyp, cmd->resp[0],
				    ((status & MSDC_INT_ACMDRDY) || (status & MSDC_INT_ACMD19_DONE)) ? 1 : 0);
				break;
		}
	} else if ((status & MSDC_INT_RSPCRCERR) || (status & MSDC_INT_ACMDCRCERR)) {
		error = MMC_ERR_BADCRC;
		printf("[SD%d] CMD(%d): RSP(%d) ERR(BADCRC)\n",
		       host->id, opcode, cmd->rsptyp);
	} else if ((status & MSDC_INT_CMDTMO) || (status & MSDC_INT_ACMDTMO)) {
		error = MMC_ERR_TIMEOUT;
		printf("[SD%d] CMD(%d): RSP(%d) ERR(CMDTO) AUTO(%d)\n",
		       host->id, opcode, cmd->rsptyp, status & MSDC_INT_ACMDTMO ? 1: 0);
	} else {
		error = MMC_ERR_INVALID;
		printf("[SD%d] CMD(%d): RSP(%d) ERR(INVALID), Status:%x\n",
		       host->id, opcode, cmd->rsptyp, status);
	}

end:

	if (rsptyp == RESP_R1B) {
		while ((MSDC_READ32(MSDC_PS) & 0x10000) != 0x10000);
	}

#if MSDC_DEBUG
	if ((error == MMC_ERR_NONE) && (MSG_EVT_MASK & MSG_EVT_RSP)) {
		switch (cmd->rsptyp) {
			case RESP_R1:
			case RESP_R1B:
				msdc_dump_card_status(cmd->resp[0]);
				break;
			case RESP_R3:
				msdc_dump_ocr_reg(cmd->resp[0]);
				break;
			case RESP_R5:
				msdc_dump_io_resp(cmd->resp[0]);
				break;
			case RESP_R6:
				msdc_dump_rca_resp(cmd->resp[0]);
				break;
		}
	}
#endif

	cmd->error = error;
	if (cmd->opcode == MMC_CMD_APP_CMD && error == MMC_ERR_NONE) {
		host->app_cmd = 1;
		host->app_cmd_arg = cmd->arg;
	} else {
		host->app_cmd = 0;
	}

#ifdef FEATURE_MMC_CMDQ
	if ( (opcode == MMC_SET_QUEUE_CONTEXT) ||
	        (opcode == MMC_SET_QUEUE_ADDRESS) ||
	        (opcode == MMC_CMD_SEND_STATUS) ) {
		//MSDC_WRITE32(EMMC51_CFG0, 0);
	}
#endif

#ifdef FEATURE_MMC_CMDQ
	//MSDC_SET_FIELD(EMMC51_CFG0, 0xfff, 0);
#endif

	return error;
}


int msdc_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
	int err;

	err = msdc_send_cmd(host, cmd);
	if (err != MMC_ERR_NONE)
		return err;

	err = msdc_wait_rsp(host, cmd);

	if (err == MMC_ERR_BADCRC) {
		u32 base = host->base;
		u32 tmp = MSDC_READ32(SDC_CMD);

		/* check if data is used by the command or not */
		if (tmp & SDC_CMD_DTYP) {
			msdc_abort_handler(host, 1);
		}

#if defined(FEATURE_MMC_CM_TUNING)
		//Light: For CMD17/18/24/25, tuning may have been done by
		//       msdc_abort_handler()->msdc_get_card_status()->msdc_cmd() for CMD13->msdc_tune_cmdrsp().
		//       This means that 2nd invocation of msdc_tune_cmdrsp() occurs here!
		//--> To Do: consider if 2nd invocation can be avoid
		if ( host->app_cmd!=2 ) { //Light 20121225, to prevent recursive call path: msdc_tune_cmdrsp->msdc_app_cmd->msdc_cmd->msdc_tune_cmdrsp
			err = msdc_tune_cmdrsp(host, cmd);
			if (err != MMC_ERR_NONE) {
				printf("[Err handle][%s:%d]tune cmd fail\n", __func__, __LINE__);
			}
		}

		/* After tuning, erase sequence will error */
		if ((cmd->opcode == MMC_CMD_ERASE_GROUP_START) || (cmd->opcode == MMC_CMD_ERASE_GROUP_END) ||
		        (cmd->opcode == MMC_CMD_ERASE_WR_BLK_START) || (cmd->opcode == MMC_CMD_ERASE_WR_BLK_END)) {
			err = MMC_ERR_ERASE_SEQ;
		}
#endif
	}
	return err;
}

int msdc_cmd_stop(struct mmc_host *host, struct mmc_command *cmd)
{
	struct mmc_command stop;
	u32 err;

	if (mmc_card_mmc(host->card) && (cmd) && (cmd->opcode == 18))
		stop.rsptyp  = RESP_R1;
	else
		stop.rsptyp  = RESP_R1B;
	stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
	stop.arg     = 0;
	stop.retries = CMD_RETRIES;
	stop.timeout = CMD_TIMEOUT;

	err = msdc_cmd(host, &stop);
#ifdef MTK_EMMC_POWER_ON_WP
	if ((err == MMC_ERR_NONE) && (stop.resp[0] & R1_WP_VIOLATION)) {
		err = MMC_ERR_WP_VIOLATION;
	}
#endif
	return err;
}

#if defined(FEATURE_MMC_SDIO)
int msdc_cmd_io_abort(struct mmc_host *host)
{
	struct mmc_command abort;

	memset(&abort, 0, sizeof(struct mmc_command));
	abort.opcode = SD_IO_RW_DIRECT;
	abort.arg    = 0x80000000;        /* write */
	abort.arg   |= 0 << 28;           /* function 0 */
	abort.arg   |= SDIO_CCCR_ABORT << 9;  /* address */
	abort.arg   |= 0;             /* abort function 0 */
	abort.rsptyp = RESP_R1B;
	abort.retries = CMD_RETRIES;
	abort.timeout = CMD_TIMEOUT;

	return msdc_cmd(host, &abort);
}
#endif

static int msdc_get_card_status(struct mmc_host *host, u32 *status)
{
	int err;
	struct mmc_command cmd;

	cmd.opcode  = MMC_CMD_SEND_STATUS;
	cmd.arg     = host->card->rca << 16;
	cmd.rsptyp  = RESP_R1;
	cmd.retries = CMD_RETRIES;
	cmd.timeout = CMD_TIMEOUT;

	err = msdc_cmd(host, &cmd);

	if (err == MMC_ERR_NONE) {
		*status = cmd.resp[0];
	}
	return err;
}

#ifdef MTK_EMMC_POWER_ON_WP
int msdc_get_err_from_card_status(struct mmc_host *host)
{
	u32 status;
	int err = msdc_get_card_status(host, &status);
	if (err == MMC_ERR_NONE) {
		//*status = cmd.resp[0];
		if (status & R1_WP_VIOLATION)
			err = MMC_ERR_WP_VIOLATION;
	}
	return err;
}
#endif

int msdc_abort_handler(struct mmc_host *host, int abort_card)
{
	u32 status = 0;
	u32 state = 0;
	u32 err;

	if ( !host->card ) return;

	while (state != 4) { // until status to "tran"; //20130125 Comment out by Light
		//while ( abort_card ) { //20130125 Light
		msdc_abort(host);
		err=msdc_get_card_status(host, &status);
		//To do: move the following 2 if clause into msdc_get_card_status() or write as a function
#if 0 //Light: turn if off before I verify it
		//#if defined(MMC_MSDC_DRV_CTP)
		if (err == MMC_ERR_BADCRC) {
			printf("[Err handle][%s:%d]cmd13 crc error\n", __func__, __LINE__);
			msdc_tune_update_cmdrsp(host, count++);

			if (count >= 512)
				count = 0;
		}

		if (err == MMC_ERR_TIMEOUT) {
			printf("[Err handle][%s:%d]cmd13 timeout\n", __func__, __LINE__);
			msdc_tune_update_cmdrsp(host, count++);

			if (count >= 512)
				count = 0;
		}
#else
		if (err != MMC_ERR_NONE) {
			printf("[Err handle][%s:%d]cmd13 fail\n", __func__, __LINE__);
			goto out;
		}
#endif

		state = R1_CURRENT_STATE(status);
#if MMC_DEBUG
		mmc_dump_card_status(status);
#endif

		printf("check card state<%d>\n", state);
		if (state == 5 || state == 6) {
			if (abort_card) {
				printf("state<%d> need cmd12 to stop\n", state);

				err=msdc_cmd_stop(host, NULL);

				//To do: move the following 2 if clause into msdc_cmd_stop() or write as a function
#if 0 //Light: turn if off before I verify it
				//#if defined(MMC_MSDC_DRV_CTP)
				if (err == MMC_ERR_BADCRC) {
					printf("[Err handle][%s:%d]cmd12 crc error\n", __func__, __LINE__);
					msdc_tune_update_cmdrsp(host, count++);

					if (count >= 512)
						count = 0;

					continue;
				}

				if (err == MMC_ERR_TIMEOUT) {
					printf("[Err handle][%s:%d]cmd12 timeout\n", __func__, __LINE__);
					msdc_tune_update_cmdrsp(host, count++);

					if (count >= 512)
						count = 0;

					continue;
				}
#else
				if (err != MMC_ERR_NONE) {
					printf("[Err handle][%s:%d]cmd12 fail\n", __func__, __LINE__);
					goto out;
				}
#endif
			}
			//break;  //20130125 Light
		} else if (state == 7) {  // busy in programing
			printf("state<%d> card is busy\n", state);
			mdelay(100);
		} else if (state != 4) {
			printf("state<%d> ??? \n", state);
			goto out;
		}
	}

	msdc_abort(host);

	return 0;

out:
	printf("[SD%d] data abort failed\n",host->id);
	return 1;
}

void msdc_intr_unmask(struct mmc_host *host, u32 bits)
{
	u32 base = host->base;
	u32 val;

	val  = MSDC_READ32(MSDC_INTEN);
	val |= bits;
	MSDC_WRITE32(MSDC_INTEN, val);
}

void msdc_intr_mask(struct mmc_host *host, u32 bits)
{
	u32 base = host->base;
	u32 val;

	val  = MSDC_READ32(MSDC_INTEN);
	val &= ~bits;
	MSDC_WRITE32(MSDC_INTEN, val);
}

static int msdc_app_cmd(struct mmc_host *host)
{
	struct mmc_command appcmd;
	int err = MMC_ERR_NONE;
	int retries = 10;
	appcmd.opcode = MMC_CMD_APP_CMD;
	appcmd.arg = host->app_cmd_arg;
	appcmd.rsptyp  = RESP_R1;
	appcmd.retries = CMD_RETRIES;
	appcmd.timeout = CMD_TIMEOUT;

	do {
		err = msdc_cmd(host, &appcmd);
		if (err == MMC_ERR_NONE)
			break;
	} while (retries--);
	return err;
}

#if defined(MSDC_ENABLE_DMA_MODE)
int msdc_dma_send_sandisk_fwid(struct mmc_host *host, uchar *buf,u32 opcode, ulong nblks)
{
	struct mmc_command cmd;
	struct mmc_data data;

	BUG_ON(nblks > host->max_phys_segs);

	//MSG(OPS, "[SD%d] Read data %d blks from 0x%x\n", host->id, nblks, src);

	/* send read command */
	cmd.opcode  = opcode;
	cmd.rsptyp  = RESP_R1;
	cmd.arg     = 0;  //src;
	cmd.retries = 0;
	cmd.timeout = CMD_TIMEOUT;

	data.blks    = nblks;
	data.buf     = (u8*)buf;
	data.timeout = 100; /* 100ms */

	return msdc_dma_transfer(host, &cmd, &data);
}
#endif

void msdc_brk_cmd(struct mmc_host *host)
{
	u32 base = host->base;
	u32 tmo =0;

	WAIT_COND(SDC_IS_BUSY() == 0, tmo, tmo);
	if (tmo == 0)
		printf("[%s]: SDC BUSY timeout happend, before send break cmd\n", __func__);
	SDC_SEND_CMD(0x000000e8, 0);
}

int msdc_pio_read(struct mmc_host *host, u32 *ptr, u32 size)
{
	int err = MMC_ERR_NONE;
#if defined(MMC_MSDC_DRV_CTP)
	u8 *ptr8;
	u16 *ptr16;
	u32 pio_bits = ((msdc_priv_t*)host->priv)->pio_bits;
#else
	u32 pio_bits = 32;
#endif
	u32 base = host->base;
	u32 ints = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO | MSDC_INT_XFER_COMPL;
	//u32 timeout = 100000;
	u32 status;
	u32 totalsz = size;
	u8  done = 0;
	u32 size_per_round;
	u32 dcrc;
	u8* u8ptr;

#if defined(FEATURE_MMC_SDIO)
	ints |= MSDC_INT_SDIOIRQ;
#endif

#if defined(MMC_MSDC_DRV_CTP)
	if (pio_bits == 16)
		ptr16 = (u16 *) ptr;
	else if (pio_bits == 8)
		ptr8 = (u8 *) ptr;
#endif

	while (1) {
#if defined(MSDC_USE_IRQ)
		//For CTP only
		DisableIRQ();
		status = msdc_irq_sts[host->id];
		msdc_irq_sts[host->id] &= ~ints;
		EnableIRQ();
#else
		status = MSDC_READ32(MSDC_INT);
		MSDC_WRITE32(MSDC_INT, status);
#if defined(FEATURE_MMC_SDIO)
		if (status & MSDC_INT_SDIOIRQ) {
			printf("(%s)INT status:0x%x\n", __func__, status);
			if ( (host->id == 2) || (host->id == 3) ) {
				mmc_sdio_proc_pending_irqs(host->card);
				//sdio_read_pending_irq(host->card->io_func[0]);
			}
		}
#endif
#endif

		if (status & ~ints) {
			MSG(WRN, "[SD%d]<CHECKME> Unexpected INT(0x%x)\n",
			    host->id, status);
		}
		if (status & MSDC_INT_DATCRCERR) {
			MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
			printf("[SD%d] DAT CRC error (0x%x), Left:%d/%d bytes, RXFIFO:%d,dcrc:0x%x\n",
			       host->id, status, size, totalsz, MSDC_RXFIFOCNT(),dcrc);
			err = MMC_ERR_BADCRC;
			break;
		} else if (status & MSDC_INT_DATTMO) {
			printf("[SD%d] DAT TMO error (0x%x), Left: %d/%d bytes, RXFIFO:%d\n",
			       host->id, status, size, totalsz, MSDC_RXFIFOCNT());
			err = MMC_ERR_TIMEOUT;
			break;
		} else if (status & MSDC_INT_ACMDCRCERR) {
			MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
			printf("[SD%d] AUTOCMD CRC error (0x%x), Left:%d/%d bytes, RXFIFO:%d,dcrc:0x%x\n",
			       host->id, status, size, totalsz, MSDC_RXFIFOCNT(),dcrc);
			err = MMC_ERR_ACMD_RSPCRC;
			break;
		} else if (status & MSDC_INT_XFER_COMPL) {
			done = 1;
		}

		if (size == 0 && done)
			break;

		/* Note. RXFIFO count would be aligned to 4-bytes alignment size */
		//if ((size >=  MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= MSDC_FIFO_THD))
		if (size > 0) {
			int left;
			if ( (size >= MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= MSDC_FIFO_THD) )
				left = MSDC_FIFO_THD;
			else if ( (size < MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= size) )
				left = size;
			else
				continue;

			size_per_round = left;

#if defined(MMC_MSDC_DRV_CTP)
			if (pio_bits == 8) {
				do {
#ifdef MTK_MSDC_DUMP_FIFO
					printf("0x%x ",MSDC_FIFO_READ8());
#else
					*ptr8++ = MSDC_FIFO_READ8();
#endif
					left--;
				} while (left);

			} else if (pio_bits == 16) {
				do {
					if (left> 1) {
#ifdef MTK_MSDC_DUMP_FIFO
						printf("0x%x ",MSDC_FIFO_READ16());
#else
						*ptr16++ = MSDC_FIFO_READ16();
#endif
						left-=2;
					} else {
						u8ptr = (u8*)ptr;
						while (left--) {
#ifdef MTK_MSDC_DUMP_FIFO
							printf("0x%x ",MSDC_FIFO_READ8());
#else
							*u8ptr++ = MSDC_FIFO_READ8();
#endif
						}
					}
				} while (left);

			} else
#endif
			{
				//if (pio_bits==32 )
				do {
					if (left> 3) {
#ifdef MTK_MSDC_DUMP_FIFO
						printf("0x%x ",MSDC_FIFO_READ32());
#else
						*ptr++ = MSDC_FIFO_READ32();
#endif
						left-=4;
					} else {
						u8ptr = (u8*)ptr;
						while (left--) {
#ifdef MTK_MSDC_DUMP_FIFO
							printf("0x%x ",MSDC_FIFO_READ8());
#else
							*u8ptr++ = MSDC_FIFO_READ8();
#endif
						}
					}
				} while (left);
			}
			size -= size_per_round;

			//MSG(FIO, "[SD%d] Read %d bytes, RXFIFOCNT: %d,  Left: %d/%d\n",
			//    host->id, size_per_round, MSDC_RXFIFOCNT(), size, totalsz);
		}
	}

	if (err != MMC_ERR_NONE) {
		msdc_abort(host); /* reset internal fifo and state machine */
		printf("[SD%d] %d-bit PIO Read Error (%d)\n", host->id,
		       pio_bits, err);
	}

	return err;
}

int msdc_pio_write(struct mmc_host *host, u32 *ptr, u32 size)
{
	int err = MMC_ERR_NONE;
	u8 *ptr8=(u8 *)ptr;
	u32 base = host->base;
	u32 ints = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO | MSDC_INT_XFER_COMPL;
	//u32 timeout = 250000;
	u32 status;
#if defined(MMC_MSDC_DRV_CTP)
	u32 pio_bits = ((msdc_priv_t*)host->priv)->pio_bits;
#else
	u32 pio_bits = 32;
#endif
	u32 size_per_round;

#if defined(FEATURE_MMC_SDIO)
	ints |= MSDC_INT_SDIOIRQ;
#endif

	while (1) {
#if defined(MSDC_USE_IRQ)
		//For CTP only
		DisableIRQ();
		status = msdc_irq_sts[host->id];
		msdc_irq_sts[host->id] &= ~ints;
		EnableIRQ();
#else
		status = MSDC_READ32(MSDC_INT);
		MSDC_WRITE32(MSDC_INT, status);
#if defined(FEATURE_MMC_SDIO)
		if (status & MSDC_INT_SDIOIRQ) {
			printf("(%s)INT status:0x%x\n", __func__, status);
			if ( (host->id == 2) || (host->id == 3) ) {
				mmc_sdio_proc_pending_irqs(host->card);
				//sdio_read_pending_irq(host->card->io_func[0]);
			}
		}
#endif
#endif
		if (status & ~ints) {
			MSG(WRN, "[SD%d]<CHECKME> Unexpected INT(0x%x)\n",
			    host->id, status);
		}
		if (status & MSDC_INT_DATCRCERR) {
			printf("[SD%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",
			       host->id, status, size);
			err = MMC_ERR_BADCRC;
			break;
		} else if (status & MSDC_INT_DATTMO) {
			printf("[SD%d] DAT TMO error (0x%x), Left DAT: %d bytes, MSDC_FIFOCS=%xh\n",
			       host->id, status, size, MSDC_READ32(MSDC_FIFOCS));
			err = MMC_ERR_TIMEOUT;
			break;
		} else if (status & MSDC_INT_ACMDCRCERR) {
			printf("[SD%d] AUTO CMD CRC error (0x%x), Left DAT: %d bytes\n",
			       host->id, status, size);
			err = MMC_ERR_ACMD_RSPCRC;
			break;
		} else if (status & MSDC_INT_XFER_COMPL) {
			if (size == 0) {
				MSG(OPS, "[SD%d] all data flushed to card\n", host->id);
				break;
			} else {
				MSG(WRN, "[SD%d]<CHECKME> XFER_COMPL before all data written\n",
				    host->id);
			}
		}

		if (size == 0)
			continue;


		if (MSDC_TXFIFOCNT() == 0) {

			int left;
#if defined(MMC_MSDC_DRV_CTP)
			if ( pio_bits==32 ) {
				if ( size >= MSDC_FIFO_THD )
					left = MSDC_FIFO_THD;
				else
					left = size;
			} else
#endif
			{
				if ( size >= MSDC_FIFO_SZ )
					left = MSDC_FIFO_SZ;
				else
					left = size;
			}

			size_per_round = left;


#if defined(MMC_MSDC_DRV_CTP)
			if (pio_bits == 8) {
				do {
					MSDC_FIFO_WRITE8(*ptr8);
					ptr8++;
					left--;
				} while (left);
			} else if (pio_bits == 16) {
				do {
					if (left > 1) {
						MSDC_FIFO_WRITE16(*(u16*)ptr8);
						ptr8+=2;
						left-=2;
					} else {
						while (left--) {
							MSDC_FIFO_WRITE8(*ptr8);
							ptr8++;
						}
					}
				} while (left);
			} else
#endif
			{
				//if ( write_unit==4 )
				do {
					if (left > 3) {
						MSDC_FIFO_WRITE32(*(u32*)ptr8);
						ptr8+=4;
						left-=4;
					} else {
						while (left--) {
							MSDC_FIFO_WRITE8(*ptr8);
							ptr8++;
						}
					}
				} while (left);
			}

			size -= size_per_round;
		}
	}

	if (err != MMC_ERR_NONE) {
		msdc_abort(host); /* reset internal fifo and state machine */
		MSG(OPS, "[SD%d] %d-bit PIO Write Error (%d)\n", host->id,
		    pio_bits, err);
	}

	return err;
}

int msdc_pio_get_sandisk_fwid(struct mmc_host *host, uchar *dst)
{
	u32 blksz = host->blklen;
	int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
	struct mmc_command cmd;

	ulong *ptr = (ulong *)dst;

	//MSG(OPS, "[SD%d] Read data %d bytes from 0x%x\n", host->id, nblks * blksz, src);

	msdc_clr_fifo(host);
	msdc_set_blknum(host, 1);
	msdc_set_blklen(host, blksz);
	msdc_set_timeout(host, 100000000, 0);

	/* send read command */
	cmd.opcode  = MMC_CMD21;
	cmd.rsptyp  = RESP_R1;
	cmd.arg     = 0;
	cmd.retries = 0;
	cmd.timeout = CMD_TIMEOUT;
	err = msdc_cmd(host, &cmd);

	if (err != MMC_ERR_NONE)
		goto done;

	err = derr = msdc_pio_read(host, (u32*)ptr, 1 * blksz);


done:
	if (err != MMC_ERR_NONE) {
		if (derr != MMC_ERR_NONE) {
			printf("[SD%d] Read data error (%d)\n", host->id, derr);
			msdc_abort_handler(host, 1);
		} else {
			printf("[SD%d] Read error (%d)\n", host->id, err);
		}
	}
	return (derr == MMC_ERR_NONE) ? err : derr;
}
int msdc_pio_send_sandisk_fwid(struct mmc_host *host,uchar *src)
{
	int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
	u32 blksz = host->blklen;
	struct mmc_command cmd;

	ulong *ptr = (ulong *)src;

	//MSG(OPS, "[SD%d] Write data %d bytes to 0x%x\n", host->id, nblks * blksz, dst);

	msdc_clr_fifo(host);
	msdc_set_blknum(host, 1);
	msdc_set_blklen(host, blksz);

	/* No need since MSDC always waits 8 cycles for write data timeout */

	/* send write command */
	cmd.opcode  = MMC_CMD50;
	cmd.rsptyp  = RESP_R1;
	cmd.arg     = 0;
	cmd.retries = 0;
	cmd.timeout = CMD_TIMEOUT;
	err = msdc_cmd(host, &cmd);

	if (err != MMC_ERR_NONE)
		goto done;

	err = derr = msdc_pio_write(host, (u32*)ptr, 1 * blksz);

done:
	if (err != MMC_ERR_NONE) {
		if (derr != MMC_ERR_NONE) {
			printf("[SD%d] Write data error (%d)\n", host->id, derr);
			msdc_abort_handler(host, 1);
		} else {
			printf("[SD%d] Write error (%d)\n", host->id, err);
		}
	}
	return (derr == MMC_ERR_NONE) ? err : derr;
}

int msdc_pio_bread(struct mmc_host *host, uchar *dst, ulong src, ulong nblks)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;
	u32 blksz = host->blklen;
	int err = MMC_ERR_NONE, derr = MMC_ERR_NONE, cmd_err = MMC_ERR_NONE;
	int multi;
	struct mmc_command cmd;
	ulong *ptr = (ulong *)dst;

	MSG(OPS, "[SD%d] Read data %d bytes from 0x%x\n", host->id, (unsigned int)nblks * blksz, src);

	multi = nblks > 1 ? 1 : 0;

	msdc_clr_fifo(host);
	msdc_set_blknum(host, nblks);
	msdc_set_blklen(host, blksz);
	msdc_set_timeout(host, 100000000, 0);

	/* send read command */
	cmd.opcode  = multi ? MMC_CMD_READ_MULTIPLE_BLOCK : MMC_CMD_READ_SINGLE_BLOCK;

	/* CMD23 with length only 1 */
	if (priv->autocmd & MSDC_AUTOCMD23)
		cmd.opcode = MMC_CMD_READ_MULTIPLE_BLOCK;

	cmd.rsptyp  = RESP_R1;
	cmd.arg     = src;
	cmd.retries = 0;
	cmd.timeout = CMD_TIMEOUT;

	host->cmd = &cmd;
	err = msdc_cmd(host, &cmd);
	if (err != MMC_ERR_NONE)
		goto done;

	derr = msdc_pio_read(host, (u32*)ptr, nblks * blksz);
	if (derr != MMC_ERR_NONE)
		goto done;

	if (multi && (priv->autocmd == 0)) {
		cmd_err = msdc_cmd_stop(host, &cmd);
	}

done:
	if (err != MMC_ERR_NONE) {
		/* msdc_cmd will do cmd tuning flow, so if enter here, cmd maybe timeout.
		 * need reset host */
		//Light: msdc_abort_handler() combined from preloader/LK and CTP can not meet this purpose,
		//       so call msdc_abort() directly
		//msdc_abort_handler(host, 0);
		msdc_abort(host);
		return err;                    // high level will retry
	}

	if (derr != MMC_ERR_NONE) {
		/* crc error find in data transfer. need reset host & send cmd12 */
		/* if autocmd crc occur, will enter here too */
		msdc_abort_handler(host, 1);

		return derr;
	}

	if (cmd_err != MMC_ERR_NONE) {
		/* msdc_cmd will do cmd tuning flow, so if enter here, cmd maybe timeout
		 * need reset host */
		//Light: msdc_abort_handler() combined from preloader/LK and CTP can not meet this purpose,
		//       so call msdc_abort() directly
		//msdc_abort_handler(host, 0);
		msdc_abort(host);
	}
	return MMC_ERR_NONE;
}

int msdc_pio_bwrite(struct mmc_host *host, ulong dst, uchar *src, ulong nblks)
{
	msdc_priv_t *priv = (msdc_priv_t*)host->priv;
	u32 base = host->base;
	int err = MMC_ERR_NONE, derr = MMC_ERR_NONE, cmd_err = MMC_ERR_NONE;
	int multi;
	u32 blksz = host->blklen;
	struct mmc_command cmd;
	ulong *ptr = (ulong *)src;

	MSG(OPS, "[SD%d] Write data %d bytes to 0x%x\n", host->id, (unsigned int)nblks * blksz, dst);

	multi = nblks > 1 ? 1 : 0;

	msdc_clr_fifo(host);
	msdc_set_blknum(host, nblks);
	msdc_set_blklen(host, blksz);

	/* send write command */
	cmd.opcode  = multi ? MMC_CMD_WRITE_MULTIPLE_BLOCK : MMC_CMD_WRITE_BLOCK;

	/* CMD23 with length only 1 */
	if (priv->autocmd & MSDC_AUTOCMD23)
		cmd.opcode = MMC_CMD_WRITE_MULTIPLE_BLOCK;

	cmd.rsptyp  = RESP_R1;
	cmd.arg     = dst;
	cmd.retries = 0;
	cmd.timeout = CMD_TIMEOUT;
	err = msdc_cmd(host, &cmd);

	if (err != MMC_ERR_NONE)
		goto done;

	host->cmd = &cmd;
	derr = msdc_pio_write(host, (u32*)ptr, nblks * blksz);
	if (multi && (priv->autocmd == 0)) {
		cmd_err = msdc_cmd_stop(host, &cmd);
	}

#ifdef MTK_EMMC_POWER_ON_WP
	else if (multi && (priv->autocmd & MSDC_AUTOCMD12)) {
		if (MSDC_READ32(SDC_ACMD_RESP) & R1_WP_VIOLATION) {
			err = MMC_ERR_WP_VIOLATION;
			goto done;
		}
	}
	err = msdc_get_err_from_card_status(host);
#endif

done:
	if (err != MMC_ERR_NONE) {
		/* msdc_cmd will do cmd tuning flow, so if enter here, cmd maybe timeout.
		 * need reset host */
		//Light: msdc_abort_handler() combined from preloader/LK and CTP can not meet this purpose,
		//       so call msdc_abort() directly
		//msdc_abort_handler(host, 0);
		msdc_abort(host);
		return err;                    // high level will retry
	}

	if (derr != MMC_ERR_NONE) {
		/* crc error find in data transfer. need reset host & send cmd12 */
		/* if autocmd crc occur, will enter here too */
		msdc_abort_handler(host, 1);

		return derr;
	}

	if (cmd_err != MMC_ERR_NONE) {
		/* msdc_cmd will do cmd tuning flow, so if enter here, cmd maybe timeout
		 * need reset host */
		//Light: msdc_abort_handler() combined from preloader/LK and CTP can not meet this purpose,
		//       so call msdc_abort() directly
		//msdc_abort_handler(host, 0);
		msdc_abort(host);
		return MMC_ERR_FAILED;  // high level will retry
	}

	return MMC_ERR_NONE;
}

void msdc_config_clock(struct mmc_host *host, int ddr, u32 hz, u32 hs_timing)
{
	msdc_priv_t *priv = host->priv;
	u32 base = host->base;
	u32 mode, hs400_div_dis = 0;
	u32 div;
	u32 sclk;
	u32 orig_clksrc = host->pll_mux_clk;

	if (hz >= host->f_max) {
		hz = host->f_max;
	} else if (hz < host->f_min) {
		hz = host->f_min;
	}

	//if ( hz>400000 ) hz=450000; //Light debug
	if (hs_timing & EXT_CSD_HS_TIMEING_HS400) {
		mode = 0x3; /* HS400 mode */

		host->pll_mux_clk = MSDC0_CLKSRC_DEFAULT;
		host->src_clk = msdc_src_clks[host->pll_mux_clk];

		if (hz >= host->src_clk) {
			hs400_div_dis = 1;
			div  = 0;
			sclk = host->src_clk >> 1; // use 400Mhz source
		} else {
			hs400_div_dis = 0;
			if (hz >= (host->src_clk >> 2)) {
				div  = 0;               /* mean div = 1/2 */
				sclk = host->src_clk >> 2; /* sclk = clk/div/2. 2: internal divisor */
			} else {
				div  = (host->src_clk + ((hz << 2) - 1)) / (hz << 2);
				sclk = (host->src_clk >> 2) / div;
				div  = (div >> 1);     /* since there is 1/2 internal divisor */
			}
		}
	} else if (ddr) {
		mode = 0x2; /* ddr mode and use divisor */

		if (hz >= (host->src_clk >> 2)) {
			div  = 0;               /* mean div = 1/2 */
			sclk = host->src_clk >> 2; /* sclk = clk/div/2. 2: internal divisor */
		} else {
			div  = (host->src_clk + ((hz << 2) - 1)) / (hz << 2);
			sclk = (host->src_clk >> 2) / div;
			div  = (div >> 1);     /* since there is 1/2 internal divisor */
		}
#if !defined(FPGA_PLATFORM)
	} else if (hz >= host->src_clk) {
		mode = 0x1; /* no divisor and divisor is ignored */
		div  = 0;
		sclk = host->src_clk;
#endif
	} else {
		mode = 0x0; /* use divisor */
		if (hz >= (host->src_clk >> 1)) {
			div  = 0;               /* mean div = 1/2 */
			sclk = host->src_clk >> 1; /* sclk = clk / 2 */
		} else {
			div  = (host->src_clk + ((hz << 2) - 1)) / (hz << 2);
			sclk = (host->src_clk >> 2) / div;
		}
	}
	host->cur_bus_clk = sclk;

	//msdc_config_clksrc(host, MSDC_CLKSRC_NONE);

	/* set clock mode and divisor */
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD_HS400 | MSDC_CFG_CKMOD |
	               MSDC_CFG_CKDIV,
	               (hs400_div_dis << (MSDC_CFG_CKMOD_BITS+MSDC_CFG_CKDIV_BITS)) |
	               (mode << MSDC_CFG_CKDIV_BITS) | div);

	msdc_config_clksrc(host, orig_clksrc);

	/* wait clock stable */
	while (!(MSDC_READ32(MSDC_CFG) & MSDC_CFG_CKSTB));

	if (hs_timing & EXT_CSD_HS_TIMEING_HS400) {
		msdc_set_smpl(host, 1, priv->rsmpl, TYPE_CMD_RESP_EDGE);
		msdc_set_smpl(host, 1, priv->rdsmpl, TYPE_READ_DATA_EDGE);
		msdc_set_smpl(host, 1, priv->wdsmpl, TYPE_WRITE_CRC_EDGE);
	} else {
		msdc_set_smpl(host, 0, priv->rsmpl, TYPE_CMD_RESP_EDGE);
#if !defined(FPGA_PLATFORM)
		msdc_set_smpl(host, 0, priv->rdsmpl, TYPE_READ_DATA_EDGE);
#else
		msdc_set_smpl(host, 0, MSDC_SMPL_FALLING, TYPE_READ_DATA_EDGE);
#endif
		msdc_set_smpl(host, 0, priv->wdsmpl, TYPE_WRITE_CRC_EDGE);
	}

	if ( mode==2 || mode==3 ) {
		MSDC_CLR_BIT32(MSDC_PATCH_BIT0, (1<<3));
	} else {
		MSDC_SET_BIT32(MSDC_PATCH_BIT0, (1<<3));
	}

	printf("[SD%d] SET_CLK(%dkHz): SCLK(%dkHz) MODE(%d) DDR(%d) DIV(%d) DS(%d) RS(%d)\n",
	       host->id, hz/1000, sclk/1000, mode, ddr > 0 ? 1 : 0, div,
	       msdc_cap[host->id].data_edge, msdc_cap[host->id].cmd_edge);
}

void msdc_config_bus(struct mmc_host *host, u32 width)
{
	u32 base = host->base;
	u32 val  = MSDC_READ32(SDC_CFG);

	val &= ~SDC_CFG_BUSWIDTH;

	switch (width) {
		case HOST_BUS_WIDTH_1:
			val |= (MSDC_BUS_1BITS << 16);
			break;
		case HOST_BUS_WIDTH_4:
			val |= (MSDC_BUS_4BITS << 16);
			break;
		case HOST_BUS_WIDTH_8:
			val |= (MSDC_BUS_8BITS << 16);
			break;
		default:
			val |= (MSDC_BUS_1BITS << 16);
			break;
	}
	MSDC_WRITE32(SDC_CFG, val);

	printf("[SD%d] Bus Width: %d\n", host->id, width);
}

#if defined(FEATURE_MMC_UHS1)
int msdc_switch_volt(struct mmc_host *host, int volt)
{
	u32 base = host->base;
	int err = MMC_ERR_FAILED;
	u32 timeout = 1000;
	u32 status;
	u32 bus_clk = host->cur_bus_clk;

	/* make sure SDC is not busy (TBC) */
	WAIT_COND(!SDC_IS_BUSY(), timeout, timeout);
	if (timeout == 0) {
		err = MMC_ERR_TIMEOUT;
		goto out;
	}

	/* check if CMD/DATA lines both 0 */
	if ((MSDC_READ32(MSDC_PS) & ((1 << 24) | (0xF << 16))) == 0) {

		/* pull up disabled in CMD and DAT[3:0] */
		msdc_pin_config(host, MSDC_PIN_PULL_NONE);

		/* change signal from 3.3v to 1.8v */
		msdc_host_power(host, 1, VOL_1800);

		/* wait at least 5ms for 1.8v signal switching in card */
		mdelay(10);

		/* config clock to 10~12MHz mode for volt switch detection by host. */
		msdc_config_clock(host, 0, 12000000, 0);/*For FPGA 13MHz clock,this not work*/

		/* pull up enabled in CMD and DAT[3:0] */
		msdc_pin_config(host, MSDC_PIN_PULL_UP);
		mdelay(5);

		/* start to detect volt change by providing 1.8v signal to card */
		MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_BV18SDT);

		/* wait at max. 1ms */
		mdelay(1);

		while ((status = MSDC_READ32(MSDC_CFG)) & MSDC_CFG_BV18SDT);

		if (status & MSDC_CFG_BV18PSS)
			err = MMC_ERR_NONE;
		else
			printf("[%s] sd%d v18 switch failed, MSDC_CFG=0x%x\n", __func__, host->id, status);

		/* config clock back to init clk freq. */
		msdc_config_clock(host, 0, bus_clk, 0);
	}

out:

	return err;
}
#endif

void msdc_reset_tune_counter(struct mmc_host *host)
{
	host->time_read = 0;
}

#if defined(FEATURE_ASYNC_PATH_ENABLE)
#include "msdc_tune_async.c"
#endif

#if defined(FEATURE_MMC_CM_TUNING)
#if defined(FEATURE_ASYNC_PATH_ENABLE)
int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd)
{
	return msdc_async_tune_cmd(host, cmd);
}

#else //else part for if defined(FEATURE_ASYNC_PATH_ENABLE)
int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd)
{
	u32 base = host->base;
	u32 sel = 0;
	u32 rsmpl,cur_rsmpl, orig_rsmpl;
	u32 rrdly,cur_rrdly, orig_rrdly;
	u32 cntr,cur_cntr,orig_cmdrtc;
	u32 dl_cksel, cur_dl_cksel, orig_dl_cksel;
	u32 times = 0;
	int result = MMC_ERR_CMDTUNEFAIL;
	u8 hs400 = 0, orig_clkmode;
	if (host->cur_bus_clk > 100000000) {
		sel = 1;
	}

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, orig_rrdly);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, orig_cmdrtc);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);

	hs400 = (orig_clkmode == 3) ? 1 : 0;

	dl_cksel = 0;
	do {
		cntr = 0;
		do {
			rrdly = 0;
			do {
				for (rsmpl = 0; rsmpl < 2; rsmpl++) {
					cur_rsmpl = (orig_rsmpl + rsmpl) % 2;
					msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE);
					if (host->cur_bus_clk <= 400000) {
						MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, 0);
					}
					if (cmd->opcode != MMC_CMD_STOP_TRANSMISSION) {
						if (host->app_cmd) {
							host->app_cmd = false;
							result = msdc_app_cmd(host);
							host->app_cmd = true;
							if (result != MMC_ERR_NONE)
								return MMC_ERR_CMDTUNEFAIL;
						}
						result = msdc_send_cmd(host, cmd);
						if (result == MMC_ERR_TIMEOUT)
							rsmpl--;
						if (result != MMC_ERR_NONE && cmd->opcode != MMC_CMD_STOP_TRANSMISSION) {
							if (cmd->opcode == MMC_CMD_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK ||
							        cmd->opcode == MMC_CMD_READ_SINGLE_BLOCK ||cmd->opcode == MMC_CMD_WRITE_BLOCK ||
							        cmd->opcode == MMC_CMD_SEND_WRITE_PROT_TYPE)
								msdc_abort_handler(host,1);
							continue;
						}
						result = msdc_wait_rsp(host, cmd);
					} else if (cmd->opcode == MMC_CMD_STOP_TRANSMISSION) {
						result = MMC_ERR_NONE;
						goto done;
					} else
						result = MMC_ERR_BADCRC;

#if MSDC_TUNE_LOG
					/* for debugging */
					{
						u32 t_rrdly, t_rsmpl, t_dl_cksel, t_cmdrtc;

						MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, t_rsmpl);
						MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, t_rrdly);
						MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, t_cmdrtc);
						MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, t_dl_cksel);

						times++;
						printf("[SD%d] <TUNE_CMD%d><%d><%s> CMDRRDLY=%d, RSPL=%dh\n",
						       host->id, (cmd->opcode & (~(SD_CMD_BIT | SD_CMD_APP_BIT))), times, (result == MMC_ERR_NONE) ?
						       "PASS" : "FAIL", t_rrdly, t_rsmpl);
						printf("[SD%d] <TUNE_CMD><%d><%s> CMD_RSP_TA_CNTR=%xh\n",
						       host->id, times, (result == MMC_ERR_NONE) ?
						       "PASS" : "FAIL", t_cmdrtc);
						if (host->cur_bus_clk > 100000000) {
							printf("[SD%d] <TUNE_CMD%d><%d><%s> CMD_RSP_TA_CNTR=%xh, INT_DAT_LATCH_CK_SEL=%xh\n",
							       host->id, (cmd->opcode & (~(SD_CMD_BIT | SD_CMD_APP_BIT))), times, (result == MMC_ERR_NONE) ?
							       "PASS" : "FAIL", t_cmdrtc, t_dl_cksel);
						}
					}
#endif

					if (result == MMC_ERR_NONE) {
						host->app_cmd = false;
						goto done;
					}

					if (cmd->opcode == MMC_CMD_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK || cmd->opcode == MMC_CMD_READ_SINGLE_BLOCK ||cmd->opcode == MMC_CMD_WRITE_BLOCK)
						msdc_abort_handler(host,1);
				}
				cur_rrdly = (orig_rrdly + rrdly + 1) % 32;
				MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, cur_rrdly);
			} while (++rrdly < 32);
			if (!sel)
				break;
			cur_cntr = (orig_cmdrtc + cntr + 1) % 8;
			MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, cur_cntr);
		} while (++cntr < 8);
		/* no need to update data ck sel */
		if (!sel)
			break;
		cur_dl_cksel = (orig_dl_cksel +dl_cksel+1) % 8;
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel);
		dl_cksel++;
	} while (dl_cksel < 8);

	/* no need to update ck sel */
	if (result != MMC_ERR_NONE)
		result = MMC_ERR_CMDTUNEFAIL;
done:
	return result;
}
#endif
#endif

#if defined(MMC_MSDC_DRV_CTP)
void msdc_tune_update_cmdrsp(struct mmc_host *host, u32 count)
{
	u32 base = host->base;
	u32 sel = 0;
	u32 rsmpl,cur_rsmpl, orig_rsmpl;
	u32 rrdly,cur_rrdly, orig_rrdly;
	u32 cntr,cur_cntr,orig_cmdrtc;
	u32 dl_cksel, cur_dl_cksel, orig_dl_cksel;
	u32 times = 0;
	u8 hs400 = 0, orig_clkmode;

	printf("cur_bus_clk = %d\n", host->cur_bus_clk);
	if (host->cur_bus_clk > 100000000) {
		sel = 1;
	}

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, orig_rrdly);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, orig_cmdrtc);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);

	hs400 = (orig_clkmode == 3) ? 1 : 0;
	dl_cksel = 0;
	cntr = 0;
	rrdly = 0;
	if (sel == 1) {
		if (count >= 8 * 64 && count < 8 * 8 * 64) {
			dl_cksel = count % 8;
			cur_dl_cksel = (orig_dl_cksel + dl_cksel + 1) % 8;
			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel);

			count = count % (8 * 64);
		}

		if (count >= 64 && count < 8 * 64) {
			cntr = count % 8;
			cur_cntr = (orig_cmdrtc + cntr + 1) % 8;
			MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, cur_cntr);

			count = count % 64;
		}
	}

	if (count >= 2 && count < 64) {
		rrdly = count % 32;
		cur_rrdly = (orig_rrdly + rrdly + 1) % 32;
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, cur_rrdly);

		count = (count > 32 ? 1 : 0);
	}

	if (count >= 0 && count < 2) {
		cur_rsmpl = (orig_rsmpl + count) % 2;
		msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE);
	}
}
#endif

#if defined(FEATURE_MMC_RD_TUNING)
#if defined(FEATURE_ASYNC_PATH_ENABLE)
int msdc_tune_bread(struct mmc_host *host, uchar *dst, ulong src, ulong nblks)
{
	//Let mmc_core to invoke tuning function by using old name - msdc_tune_bread
	return msdc_async_tune_bread(host, dst, src ,nblks);
}

int msdc_tune_read(struct mmc_host *host)
{
	return msdc_async_tune_read(host);
}

#else //else part for if defined(FEATURE_ASYNC_PATH_ENABLE)
int msdc_tune_bread(struct mmc_host *host, uchar *dst, ulong src, ulong nblks)
{
	u32 base = host->base;
	u32 dcrc, ddr = 0, sel = 0;
	u32 cur_rxdly0, cur_rxdly1;
	u32 rdsmpl, cur_rdsmpl, orig_rdsmpl;
	u32 dsel,cur_dsel,orig_dsel;
	u32 dl_cksel,cur_dl_cksel,orig_dl_cksel;
	u32 rxdly;
	u32 cur_dat0, cur_dat1, cur_dat2, cur_dat3, cur_dat4, cur_dat5,
	    cur_dat6, cur_dat7;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5,
	    orig_dat6, orig_dat7;
	u32 orig_clkmode;
	u32 times = 0;
	int result = MMC_ERR_READTUNEFAIL;
	u8 hs400 = 0;

	if (host->cur_bus_clk > 100000000)
		sel = 1;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_dsel);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_rdsmpl);

	/* Tune Method 2. delay each data line */
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

	dl_cksel = 0;
	do {
		dsel = 0;
		do {
			rxdly = 0;
			do {
				for (rdsmpl = 0; rdsmpl < 2; rdsmpl++) {
					cur_rdsmpl = (orig_rdsmpl + rdsmpl) % 2;
					msdc_set_smpl(host, hs400, cur_rdsmpl, TYPE_READ_DATA_EDGE);

					result = host->blk_read(host, dst, src, nblks);
					if (result == MMC_ERR_CMDTUNEFAIL || result == MMC_ERR_CMD_RSPCRC || result == MMC_ERR_ACMD_RSPCRC)
						goto done;

					MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);

					if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

#if MSDC_TUNE_LOG
					/* for debugging */
					{
						u32 t_dspl, t_ckgen_dsel, t_int_cksel;

						MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, t_dspl);
						MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, t_ckgen_dsel);
						MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, t_int_cksel);

						times++;
						printf("[SD%d] <TUNE_BREAD_%d><%s><cmd%d:0x%x> DCRC=%xh, ret=%d\n",
						       host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?
						       "PASS" : "FAIL", (nblks == 1 ? 17 : 18), src, dcrc, result);
						printf("[SD%d] <TUNE_BREAD_%d><%s><cmd%d:0x%x> DATRDDLY0=%xh, DATRDDLY1=%xh, DSMPL=%xh\n",
						       host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?
						       "PASS" : "FAIL", (nblks == 1 ? 17 : 18), src, MSDC_READ32(MSDC_DAT_RDDLY0), MSDC_READ32(MSDC_DAT_RDDLY1), t_dspl);

						if (host->cur_bus_clk >= 100000000) {
							printf("[SD%d] <TUNE_BREAD_%d><%s><cmd%d:0x%x> CKGEN_MSDC_DLY_SEL=%xh, INT_DAT_LATCH_CK_SEL=%xh\n",
							       host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?
							       "PASS" : "FAIL", (nblks == 1 ? 17 : 18), src, t_ckgen_dsel, t_int_cksel);
						}
					}
#endif

					/* no crc error in this data line */
					if (result == MMC_ERR_NONE && dcrc == 0) {
						goto done;
					} else {
						result = MMC_ERR_BADCRC;
					}
				}
				cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
				cur_rxdly1 = MSDC_READ32(MSDC_DAT_RDDLY1);

				orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
				orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
				orig_dat2 = (cur_rxdly0 >> 8)  & 0x1F;
				orig_dat3 = (cur_rxdly0 >> 0)  & 0x1F;
				orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
				orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
				orig_dat6 = (cur_rxdly1 >> 8)  & 0x1F;
				orig_dat7 = (cur_rxdly1 >> 0)  & 0x1F;

				// Bits8~15 of dcrc have been masked for non-ddr case,
				// so we can process ddr and non-ddr cases with the same code
				cur_dat0 = (dcrc & ((1 << 0) | (1 << 8)) ) ?  (orig_dat0 + 1) : orig_dat0;
				cur_dat1 = (dcrc & ((1 << 1) | (1 << 9)) ) ?  (orig_dat1 + 1) : orig_dat1;
				cur_dat2 = (dcrc & ((1 << 2) | (1 << 10)) ) ? (orig_dat2 + 1) : orig_dat2;
				cur_dat3 = (dcrc & ((1 << 3) | (1 << 11)) ) ? (orig_dat3 + 1) : orig_dat3;
				cur_dat4 = (dcrc & ((1 << 4) | (1 << 12)) ) ? (orig_dat4 + 1) : orig_dat4;
				cur_dat5 = (dcrc & ((1 << 5) | (1 << 13)) ) ? (orig_dat5 + 1) : orig_dat5;
				cur_dat6 = (dcrc & ((1 << 6) | (1 << 14)) ) ? (orig_dat6 + 1) : orig_dat6;
				cur_dat7 = (dcrc & ((1 << 7) | (1 << 15)) ) ? (orig_dat7 + 1) : orig_dat7;

				cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
				             ((cur_dat2 & 0x1F)<< 8) | ((cur_dat3 & 0x1F) << 0);
				cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F) << 16) |
				             ((cur_dat6 & 0x1F) << 8) | ((cur_dat7 & 0x1F) << 0);

				MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
				MSDC_WRITE32(MSDC_DAT_RDDLY1, cur_rxdly1);
			} while (++rxdly < 32);
			if (!sel)
				break;
			cur_dsel = (orig_dsel + dsel + 1) % 32;

			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_dsel);
		} while (++dsel < 32);
		/* no need to update data ck sel */
		if (orig_clkmode != 1)
			break;

		cur_dl_cksel = (orig_dl_cksel + dl_cksel + 1) % 8;
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel);
		dl_cksel++;
	} while (dl_cksel < 8);
done:

	return result;
}

#define READ_TUNING_MAX_HS (2 * 32)
#define READ_TUNING_MAX_UHS (2 * 32 * 32)
#define READ_TUNING_MAX_UHS_CLKMOD1 (2 * 32 * 32 *8)

int msdc_tune_read(struct mmc_host *host)
{
	u32 base = host->base;
	u32 dcrc, ddr = 0, sel = 0;
	u32 cur_rxdly0 = 0 , cur_rxdly1 = 0;
	u32 cur_dsmpl = 0, orig_dsmpl;
	u32 cur_dsel = 0,orig_dsel;
	u32 cur_dl_cksel = 0,orig_dl_cksel;
	u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0, cur_dat4 = 0, cur_dat5 = 0,
	    cur_dat6 = 0, cur_dat7 = 0;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5,
	    orig_dat6, orig_dat7;
	u32 orig_clkmode;
	int result = MMC_ERR_NONE;
	u8 hs400 = 0;

	if (host->cur_bus_clk > 100000000)
		sel = 1;

	if (host->card) {
		ddr = mmc_card_ddr(host->card);
	}
	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,orig_clkmode);
	hs400 = (orig_clkmode == 3) ? 1 : 0;
	//if(orig_clkmode == 1)
	//MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_RX_SDCLKO_SEL, 0);

	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_dsel);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);

	/* Tune Method 2. delay each data line */
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

	cur_dsmpl = (orig_dsmpl + 1) ;
	msdc_set_smpl(host, hs400, (cur_dsmpl % 2), TYPE_READ_DATA_EDGE);
	if (cur_dsmpl >= 2) {
		MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
		if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

		cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
		cur_rxdly1 = MSDC_READ32(MSDC_DAT_RDDLY1);

		orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
		orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
		orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
		orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;
		orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
		orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
		orig_dat6 = (cur_rxdly1 >> 8) & 0x1F;
		orig_dat7 = (cur_rxdly1 >> 0) & 0x1F;

		// Bits8~15 of dcrc have been masked for non-ddr case,
		// so we can process ddr and non-ddr cases with the same code
		cur_dat0 = (dcrc & ((1 << 0) | (1 << 8)) ) ?  (orig_dat0 + 1) : orig_dat0;
		cur_dat1 = (dcrc & ((1 << 1) | (1 << 9)) ) ?  (orig_dat1 + 1) : orig_dat1;
		cur_dat2 = (dcrc & ((1 << 2) | (1 << 10)) ) ? (orig_dat2 + 1) : orig_dat2;
		cur_dat3 = (dcrc & ((1 << 3) | (1 << 11)) ) ? (orig_dat3 + 1) : orig_dat3;
		cur_dat4 = (dcrc & ((1 << 4) | (1 << 12)) ) ? (orig_dat4 + 1) : orig_dat4;
		cur_dat5 = (dcrc & ((1 << 5) | (1 << 13)) ) ? (orig_dat5 + 1) : orig_dat5;
		cur_dat6 = (dcrc & ((1 << 6) | (1 << 14)) ) ? (orig_dat6 + 1) : orig_dat6;
		cur_dat7 = (dcrc & ((1 << 7) | (1 << 15)) ) ? (orig_dat7 + 1) : orig_dat7;

		cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
		             ((cur_dat2 & 0x1F) << 8) | ((cur_dat3 & 0x1F) << 0);
		cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F)<< 16) |
		             ((cur_dat6 & 0x1F) << 8) | ((cur_dat7 & 0x1F) << 0);

		MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
		MSDC_WRITE32(MSDC_DAT_RDDLY1, cur_rxdly1);

	}
	if (cur_dat0 >= 32 || cur_dat1 >= 32 || cur_dat2 >= 32 || cur_dat3 >= 32 ||
	        cur_dat4 >= 32 || cur_dat5 >= 32 || cur_dat6 >= 32 || cur_dat7 >= 32) {
		if (sel) {
			cur_dsel = (orig_dsel + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_dsel % 32);
		}
	}
	if (cur_dsel >= 32) {
		if (orig_clkmode == 1 && sel) {
			cur_dl_cksel = (orig_dl_cksel + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
		}
	}
	++(host->time_read);
	if ((sel == 1 && orig_clkmode == 1 && host->time_read == READ_TUNING_MAX_UHS_CLKMOD1)||
	        (sel == 1 && orig_clkmode != 1 && host->time_read == READ_TUNING_MAX_UHS)||
	        (sel == 0 && orig_clkmode != 1 && host->time_read == READ_TUNING_MAX_HS)) {
		result = MMC_ERR_READTUNEFAIL;
	}
	return result;
}
#endif
#endif  /* end of FEATURE_MMC_RD_TUNING */

int msdc_tune_rw_hs400(struct mmc_host *host, uchar *dst, ulong src, ulong nblks, unsigned int rw)
{
	u32 ds_dly1 = 0, ds_dly3 = 0, orig_ds_dly1 = 0, orig_ds_dly3 = 0;
	u32 ds_dly1_count, ds_dly3_count = 0;
	int result = MMC_ERR_READTUNEFAIL;
#if MSDC_TUNE_LOG
	u32 times = 0;
#endif
	u32 base = host->base;

	if (host->id != 0) {
		return result;
	}

	printf("[tune][%s:%d] start hs400 read tune\n", __func__, __LINE__);

	MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, orig_ds_dly1);
	MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, orig_ds_dly3);

	ds_dly3 = orig_ds_dly3;
	ds_dly1 = orig_ds_dly1;
	do {
		if (ds_dly3 >= 31) {
			ds_dly3 = 0;
		} else {
			ds_dly3 += 1;
		}
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, ds_dly3);

		ds_dly1_count = 0;
		do {
			if (ds_dly1 == 0) {
				ds_dly1 = 31;
			} else {
				ds_dly1 -= 1;
			}

			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, ds_dly1);

			/* resend the r/w command */
			if (rw == 0) {
				result = host->blk_read(host, dst, src, nblks);
			} else if (rw == 1) {
				result = host->blk_write(host, (ulong) dst, (uchar *) src, nblks);
			}

#if MSDC_TUNE_LOG
			/* for debugging */
			{
				times++;
				if (rw == 0) {
					printf("[SD%d] <TUNE_BREAD_%d><%s><cmd%d:0x%x> ret=%d, DS_DLY1=%d, DS_DLY3=%d\n",
					       host->id, times, result == MMC_ERR_NONE ? "PASS" : "FAIL", (nblks == 1 ? 17 : 18), (unsigned int)dst,
					       result, ds_dly1, ds_dly3);
				} else if (rw == 1) {
					printf("[SD%d] <TUNE_BEWRITE_%d><%s><cmd%d:0x%x> ret=%d, DS_DLY1=%d, DS_DLY3=%d\n",
					       host->id, times, result == MMC_ERR_NONE ? "PASS" : "FAIL", (nblks == 1 ? 24 : 25), (unsigned int)dst,
					       result, ds_dly1, ds_dly3);
				}
			}
#endif

			if (result == MMC_ERR_CMDTUNEFAIL || result == MMC_ERR_CMD_RSPCRC)
				goto done;

			if (result == MMC_ERR_NONE) {
				goto done;
			}

		} while (++ds_dly1_count < 32);
	} while (++ds_dly3_count < 32);

done:
	return result;
}

#if defined(FEATURE_MMC_WR_TUNING)
#if defined(FEATURE_ASYNC_PATH_ENABLE)
int msdc_tune_bwrite(struct mmc_host *host, ulong dst, uchar *src, ulong nblks)
{
	return msdc_async_tune_bwrite(host, dst, src, nblks);
}

#else //else part for if defined(FEATURE_ASYNC_PATH_ENABLE)
int msdc_tune_bwrite(struct mmc_host *host, ulong dst, uchar *src, ulong nblks)
{
	u32 base = host->base;
	u32 orig_clkmode;
	u32 sel = 0;
	//u32 ddrckdly = 0;
	u32 wrrdly, cur_wrrdly, orig_wrrdly;
	u32 wdsmpl, cur_wdsmpl, orig_wdsmpl;
	u32 d_cntr,orig_d_cntr,cur_d_cntr;
	u32 rxdly, cur_rxdly0;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
	u32 cur_dat0, cur_dat1, cur_dat2, cur_dat3;
#if MSDC_TUNE_LOG
	u32 times = 0;
#endif
	//u32 status;
	int result = MMC_ERR_WRITETUNEFAIL;
	u8 hs400 = 0;

	if (host->cur_bus_clk > 100000000)
		sel = 1;

	//if (mmc_card_ddr(host->card)) ddrckdly = 1;

	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,orig_clkmode);

	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, orig_wrrdly);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_wdsmpl);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, orig_d_cntr);

	/* Tune Method 2. delay data0 line */
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

	cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);

	orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
	orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
	orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
	orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;

	d_cntr = 0;
	do {
		rxdly = 0;
		do {
			wrrdly = 0;
			do {
				for (wdsmpl = 0; wdsmpl < 2; wdsmpl++) {
					cur_wdsmpl = (orig_wdsmpl + wdsmpl) % 2;
					msdc_set_smpl(host, hs400, cur_wdsmpl, TYPE_WRITE_CRC_EDGE);
					result = host->blk_write(host, dst, src, nblks);
					if (result == MMC_ERR_CMDTUNEFAIL || result == MMC_ERR_CMD_RSPCRC || result == MMC_ERR_ACMD_RSPCRC)
						goto done;

#if MSDC_TUNE_LOG
					/* for debugging */
					{
						u32 t_dspl, t_wrrdly, t_d_cntr;// t_dl_cksel, t_ddrdly, t_cksel;

						MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, t_wrrdly);
						MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, t_dspl);
						MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, t_d_cntr);

						times++;
						printf("[SD%d] <TUNE_BWRITE_%d><%s><cmd%d:0x%x> ret=%d, DSPL=%d, WRRDLY=%d, MSDC_DAT_RDDLY0=%xh\n",
						       host->id, times, result == MMC_ERR_NONE ? "PASS" : "FAIL", (nblks == 1 ? 24 : 25), (unsigned int)dst,
						       result, t_dspl, t_wrrdly, MSDC_READ32(MSDC_DAT_RDDLY0));

						if (host->cur_bus_clk >= 100000000) {
							printf("[SD%d] <TUNE_BWRITE_%d><%s><cmd%d:0x%x> MSDC_PB1_WRDAT_CRCS_TA_CNTR=%xh\n",
							       host->id, times, (result == MMC_ERR_NONE) ? "PASS" : "FAIL", (nblks == 1 ? 24 : 25), (unsigned int)dst,
							       t_d_cntr);
						}
					}
#endif

					if (result == MMC_ERR_NONE) {
						goto done;
					}
				}
				cur_wrrdly = ++orig_wrrdly % 32;
				MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, cur_wrrdly);
			} while (++wrrdly < 32);

			cur_dat0 = ++orig_dat0 % 32; /* only adjust bit-1 for crc */
			cur_dat1 = orig_dat1;
			cur_dat2 = orig_dat2;
			cur_dat3 = orig_dat3;

			cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);

			MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
		} while (++rxdly < 32);

		/* no need to update data ck sel */
		if (!sel)
			break;

		cur_d_cntr= (orig_d_cntr + d_cntr +1 )% 8;
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, cur_d_cntr);
		d_cntr++;
	} while (d_cntr < 8);
done:

	return result;
}
#endif

#endif /* end of FEATURE_MMC_WR_TUNING */

#if defined(FEATURE_MMC_UHS1)
int msdc_tune_uhs1(struct mmc_host *host, struct mmc_card *card)
{
	u32 base = host->base;
	u32 status;
	int i;
	int err = MMC_ERR_FAILED;
	struct mmc_command cmd;

	cmd.opcode  = SD_CMD_SEND_TUNING_BLOCK;
	cmd.arg     = 0;
	cmd.rsptyp  = RESP_R1;
	cmd.retries = CMD_RETRIES;
	cmd.timeout = 0xFFFFFFFF;

	msdc_set_timeout(host, 100000000, 0);
	msdc_set_autocmd(host, MSDC_AUTOCMD19, 1);

	for (i = 0; i < 13; i++) {
		/* Note. select a pad to be tuned. msdc only tries 32 times to tune the
		 * pad since there is only 32 tuning steps for a pad.
		 */
		MSDC_SET_FIELD(SDC_ACMD19_TRG, SDC_ACMD19_TRG_TUNESEL, i);

		/* Note. autocmd19 will only trigger done interrupt and won't trigger
		 * autocmd timeout and crc error interrupt. (autocmd19 is a special command
		 * and is different from autocmd12 and autocmd23.
		 */
		err = msdc_cmd(host, &cmd);
		if (err != MMC_ERR_NONE)
			goto out;

		/* read and check acmd19 sts. bit-1: success, bit-0: fail */
		status = MSDC_READ32(SDC_ACMD19_STS);

		if (!status) {
			printf("[SD%d] ACMD19_TRG(%d), STS(0x%x) Failed\n", host->id, i,
			       status);
			err = MMC_ERR_FAILED;
			goto out;
		}
	}
	err = MMC_ERR_NONE;

out:
	msdc_set_autocmd(host, MSDC_AUTOCMD19, 0);
	return err;
}

int msdc_tune_hs200(struct mmc_host *host, struct mmc_card *card)
{
	return 0;
}

int msdc_tune_hs400(struct mmc_host *host, struct mmc_card *card)
{
	return 0;
}
#endif

#if defined(FEATURE_MMC_CARD_DETECT)
void msdc_card_detect(struct mmc_host *host, int on)
{
	u32 base = host->base;

	if ((msdc_cap[host->id].flags & MSDC_CD_PIN_EN) == 0) {
		MSDC_CARD_DETECTION_OFF();
		return;
	}

	if (on) {
		MSDC_SET_FIELD(MSDC_PS, MSDC_PS_CDDEBOUNCE, DEFAULT_DEBOUNCE);
		MSDC_CARD_DETECTION_ON();
	} else {
		MSDC_CARD_DETECTION_OFF();
		MSDC_SET_FIELD(MSDC_PS, MSDC_PS_CDDEBOUNCE, 0);
	}
}

int msdc_card_avail(struct mmc_host *host)
{
	u32 base = host->base;
	u32 sts, avail = 0;

	if ((msdc_cap[host->id].flags & MSDC_REMOVABLE) == 0)
		return 1;

	if (msdc_cap[host->id].flags & MSDC_CD_PIN_EN) {
		MSDC_GET_FIELD(MSDC_PS, MSDC_PS_CDSTS, sts);
		avail = sts == 0 ? 1 : 0;
	}

	return avail;
}
#endif

#if defined(MMC_MSDC_DRV_CTP)
int msdc_card_protected(struct mmc_host *host)
{
	u32 base = host->base;
	u32 prot;

	if (msdc_cap[host->id].flags & MSDC_WP_PIN_EN) {
		MSDC_GET_FIELD(MSDC_PS, MSDC_PS_WP, prot);
	} else {
		prot = 0;
	}

	return prot;
}
#endif

#if defined(MMC_MSDC_DRV_CTP) || defined(MMC_MSDC_DRV_LK)
void msdc_hard_reset(struct mmc_host *host)
{
	msdc_card_power(host, 0);
	mdelay(10);

	msdc_card_power(host, 1);
	mdelay(10);
}

void msdc_soft_reset(struct mmc_host *host)
{
	u32 base = host->base;
	u32 tmo = 0x0000ffff;

	MSDC_RESET();
	MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
	WAIT_COND((MSDC_READ32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS) == 0, 0xFFFF, tmo);

	if (tmo == 0) {
		MSG(DMA, "[SD%d] MSDC_DMA_CFG_STS != inactive\n", host->id);
	}
	MSDC_CLR_FIFO();
}
#endif

#if defined(MMC_MSDC_DRV_CTP) || defined(MMC_MSDC_DRV_LK)
void msdc_emmc_hard_reset(struct mmc_host *host)
{
	u32 base = host->base;

	MSDC_SET_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);
	//mt_set_gpio_out(PAD_MSDC0_RSTB,GPIO_OUT_ZERO);
	mdelay(10);
	MSDC_CLR_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);
	//mt_set_gpio_out(PAD_MSDC0_RSTB,GPIO_OUT_ONE);
}
#endif

#ifdef FEATURE_MMC_BOOT_MODE
int msdc_emmc_boot_start(struct mmc_host *host, u32 hz, int ddr, int mode, int ackdis, u8 hostbuswidth, u64 size)
{
	int err = MMC_ERR_NONE;
	u32 sts;
	u32 base = host->base;
	u32 tmo = 0xFFFFFFFF;
	u32 acktmo, dattmo;
	u64 acktime,dattime;
	u32 test_timer1;
	u32 test_timer2;

	MSDC_RESET();
	MSDC_CLR_FIFO();
	msdc_set_blklen(host, 512);
	msdc_set_blknum(host, size/512);
	msdc_config_bus(host, hostbuswidth);
	msdc_config_clksrc(host, MSDC50_CLKSRC_26MHZ);
	msdc_config_clock(host, (ddr ? MMC_STATE_DDR : 0), hz, 0);
	//MSDC_SET_FIELD(MSDC_DMA_CFG,3 << 12,0x2);
	//MSDC_SET_FIELD(MSDC_DMA_CFG,3 <<  8,0x1);
	/* requires 74 clocks/1ms before CMD0 */
	MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
	mdelay(2);
	MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);

	/* configure boot timeout value */
	WAIT_COND(SDC_IS_BUSY() == 0, tmo, tmo);
	acktime = 50 * 1000 * 1000ULL;
	dattime = 1000 * 1000 * 1000ULL;
	acktmo = msdc_cal_timeout(host, acktime, 0, 1<<EMMC_BOOT_TMO_IN_CLK_2POWER);   /* 50ms  MT6583 MSDC IP eMMC boot timeout unit change to 2^16*/
	dattmo = msdc_cal_timeout(host, dattime, 0, 1<<EMMC_BOOT_TMO_IN_CLK_2POWER);   /* 1sec */

	if (acktmo == 0) acktmo = 1;
	if (dattmo == 0) dattmo = 1;
	acktmo = acktmo > 0xFFE ? 0xFFE : acktmo;
	dattmo = dattmo > 0xFFFFE ? 0xFFFFE : dattmo;

	printf("[SD%d] EMMC BOOT ACK timeout: %d ms (clkcnt: %d)(host->cur_bus_clk = %d)\n", host->id,
	       (acktmo * 65536) / (host->cur_bus_clk / 1000), acktmo, host->cur_bus_clk);
	printf("[SD%d] EMMC BOOT DAT timeout: %d ms (clkcnt: %d)\n", host->id,
	       (dattmo * 65536) / (host->cur_bus_clk / 1000), dattmo);

	MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);
	MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTACKDIS, ackdis);
	MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTMODE, mode);
	MSDC_SET_FIELD(EMMC_CFG1, EMMC_CFG1_BOOTACKTMC, acktmo);
	MSDC_SET_FIELD(EMMC_CFG1, EMMC_CFG1_BOOTDATTMC, dattmo);

	if (mode == EMMC_BOOT_RST_CMD_MODE) {
		MSDC_WRITE32(SDC_ARG, 0xFFFFFFFA);
	} else {
		MSDC_WRITE32(SDC_ARG, 0);
	}
	MSDC_WRITE32(SDC_CMD, 0x02001000); /* bit[12]: 1 multiple block read, 0: single block read */
#if 0 //init timer to test MT6583 ACK/DAT timeour modification test case
	MSDC_WRITE32(0x10008040,0x31);
	MSDC_WRITE32(0x10008044,0x0);
	test_timer1 = MSDC_READ32(0x10008048);//init timer to test MT6583 ACK/DAT timeour modification test case
#endif
	MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSTART);

	WAIT_COND((MSDC_READ32(EMMC_STS) & EMMC_STS_BOOTUPSTATE) == EMMC_STS_BOOTUPSTATE, tmo, tmo);
	if (!ackdis) {
		do {
			sts = MSDC_READ32(EMMC_STS);
			if (sts == 0)
				continue;
			MSDC_WRITE32(EMMC_STS, sts);    /* write 1 to clear */

			/* if ack is error, hw will first set bootackrcv bit, then set bootackerr bit
			 * so the best way is check EMMC_STS_BOOTACKERR  bit after EMMC_STS_BOOTACKRCV bit set*/
			if (sts & EMMC_STS_BOOTACKERR) {
				printf("[%s]: [SD%d] EMMC_STS(0x%x): boot up ack error\n", __func__, host->id, sts);
				err = MMC_ERR_BADCRC;
				goto out;
			} else if (sts & EMMC_STS_BOOTACKRCV) {
				printf("[%s]: [SD%d] EMMC_STS(0x%x): boot ack received\n",  __func__,host->id, sts);
				break;
			} else if (sts & EMMC_STS_BOOTACKTMO) {
#if 0
				test_timer2 = MSDC_READ32(0x10008048);
				test_timer1 = (test_timer2 - test_timer1) /6000;
				printf("[SD%d] EMMC_STS(%x): boot up ack timeout(%d ms)\n", host->id, sts,test_timer1);
				//test MT6583 ACK/DAT timeour modification test case
#endif
				printf("[%s]: [SD%d] EMMC_STS(0x%x): boot up ack timeout\n",  __func__,host->id, sts);
				err = MMC_ERR_TIMEOUT;
				goto out;
			} else if (sts & EMMC_STS_BOOTUPSTATE) {
				//printf("[%s]: [SD%d] EMMC_STS(%x): boot up mode state\n",  __func__, host->id, sts);
			} else {
				printf("[%s]: [SD%d] EMMC_STS(0x%x): boot up unexpected\n",  __func__,host->id, sts);
			}
		} while (1);
	}
	//printf("ackdis(%d) err(%d)\n",ackdis,err);

	/* check if data received */
	do {
		sts = MSDC_READ32(EMMC_STS);
		if (sts == 0)
			continue;
		if (sts & EMMC_STS_BOOTDATRCV) {
			printf("[%s]: [SD%d] EMMC_STS(0x%x): boot dat received\n",  __func__,host->id, sts);
			break;
		}
		if (sts & EMMC_STS_BOOTCRCERR) {
			printf("[%s]: [SD%d] EMMC_STS(0x%x): boot up data crc error\n",  __func__,host->id, sts);
			err = MMC_ERR_BADCRC;
			goto out;
		} else if (sts & EMMC_STS_BOOTDATTMO) {
#if 0
			test_timer2 = MSDC_READ32(0x10008048);
			test_timer1 = (test_timer2 - test_timer1) /6000;
			printf("[%s]: [SD%d] EMMC_STS(%x): boot up data timeout(%d s)\n",  __func__,host->id, sts,test_timer1);
			//test MT6583 ACK/DAT timeour modification test case
#endif
			printf("[%s]: [SD%d] EMMC_STS(0x%x): boot up data timeout\n",  __func__,host->id, sts);
			err = MMC_ERR_TIMEOUT;
			goto out;
		}
	} while (1);
out:
	return err;
}

void msdc_emmc_boot_stop(struct mmc_host *host)
{
	u32 base = host->base;
	u32 tmo = 0xFFFFFFFF;

	/* Step5. stop the boot mode */
	MSDC_WRITE32(SDC_ARG, 0x00000000);
	MSDC_WRITE32(SDC_CMD, 0x00001000);

	MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTWDLY, 2);
	MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSTOP);
	WAIT_COND((MSDC_READ32(EMMC_STS) & EMMC_STS_BOOTUPSTATE) == 0, tmo, tmo);

	/* Step6. */
	MSDC_CLR_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);

	/* Step7. clear EMMC_STS bits */
	MSDC_WRITE32(EMMC_STS, MSDC_READ32(EMMC_STS));
}

int msdc_emmc_boot_read(struct mmc_host *host, u64 size, u32 *to, int read_mode)
{
	int err = MMC_ERR_NONE;
	int derr = MMC_ERR_NONE;
	u32 sts;
	u64 totalsz = size;
	u32 base = host->base;
	u64 left_sz, xfer_sz;

	msdc_priv_t *priv = (msdc_priv_t*)host->priv;
	struct dma_config *cfg = &priv->cfg;
	BUG_ON((read_mode < MSDC_MODE_PIO) && (read_mode > MSDC_MODE_DMA_DESC));

	if (read_mode == MSDC_MODE_PIO) {
		MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO);
		while (size) {
			sts = MSDC_READ32(EMMC_STS);
			if (sts & EMMC_STS_BOOTCRCERR) {
				printf("[SD%d] EMMC_STS(0x%x): boot up data crc error\n", host->id, sts);
				err = MMC_ERR_BADCRC;
				goto out;
			} else if (sts & EMMC_STS_BOOTDATTMO) {
				printf("[SD%d] EMMC_STS(0x%x): boot up data timeout error\n", host->id, sts);
				err = MMC_ERR_TIMEOUT;
				goto out;
			}
			/* Note. RXFIFO count would be aligned to 4-bytes alignment size */
			if ((size >=  MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= MSDC_FIFO_THD)) {
				int left = MSDC_FIFO_THD >> 2;
				do {
#ifdef MTK_MSDC_DUMP_FIFO
					MSDC_EMMC50_CFG_CRC_STS_SELprintf("0x%x ",MSDC_FIFO_READ32());
#else
					*to++ = MSDC_FIFO_READ32();
#endif
				} while (--left);
				size -= MSDC_FIFO_THD;
				MSG(FIO, "[SD%d] Read %d bytes, RXFIFOCNT: %d,  Left: %d/%d\n",
				    host->id, MSDC_FIFO_THD, MSDC_RXFIFOCNT(), size, totalsz);
			} else if ((size < MSDC_FIFO_THD) && MSDC_RXFIFOCNT() >= size) {
				while (size) {
					if (size > 3) {
#ifdef MTK_MSDC_DUMP_FIFO
						printf("0x%x ",MSDC_FIFO_READ32());
#else
						*to++ = MSDC_FIFO_READ32();
#endif
						size -= 4;
					} else {
#ifdef MTK_MSDC_DUMP_FIFO
						printf("0x%x ",MSDC_FIFO_READ32());
#else
						u32 val = MSDC_FIFO_READ32();
						memcpy(to, &val, size);
#endif
						size = 0;
					}
				}
				MSG(FIO, "[SD%d] Read left bytes, RXFIFOCNT: %d, Left: %d/%d\n",
				    host->id, MSDC_RXFIFOCNT(), size, totalsz);
			}
		}

out:
		if (err) {
			printf("[SD%d] EMMC_BOOT: read boot code fail(%d), FIFOCNT=%d\n",
			       host->id, err, MSDC_RXFIFOCNT());
		}
	} else {
		//MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_PIO);
		cfg->mode = read_mode;
		left_sz = size;
		if (read_mode == MSDC_MODE_DMA_BASIC) {
			cfg->inboot = 1;
			xfer_sz = left_sz > MAX_DMA_CNT ? MAX_DMA_CNT : left_sz;
			//msdc_set_blknum(host, xfer_sz/512);
		} else {
			xfer_sz = left_sz;
		}

		while (left_sz) {

			u32 base = host->base;
			cfg->xfersz = xfer_sz;
			//printf("to (0x%x) xfer_sz(0x%x)\n",to,xfer_sz);
			if (cfg->mode == MSDC_MODE_DMA_BASIC) {
				cfg->sglen = 1;
				cfg->sg[0].addr = (u32)to;
				cfg->sg[0].len = xfer_sz;
				msdc_flush_membuf(to, xfer_sz);
			} else {

				cfg->sglen = msdc_sg_init(cfg->sg, to, xfer_sz);
				cfg->flags |= DMA_FLAG_EN_CHKSUM;
			}

			MSDC_DMA_ON();
			//printf("nblks(%d),xfer_sz(%d),left_sz(%d)\n",nblks,xfer_sz,left_sz);
			msdc_dma_config(host, cfg);
			if (left_sz - xfer_sz != 0)
				MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 0);
			msdc_dma_start(host);
			err = derr = msdc_dma_wait_done(host, 0xFFFFFFFF);

			msdc_dma_stop(host);
			msdc_flush_membuf(to, xfer_sz);
			if (err != MMC_ERR_NONE)
				goto done;
			to     =(u8*)to + xfer_sz;
			left_sz -= xfer_sz;

			/* left_sz > 0 only when in basic dma mode */
			if (left_sz) {
				xfer_sz  = (xfer_sz > left_sz) ? left_sz : xfer_sz;
			}
		}

done:
		if (derr != MMC_ERR_NONE) {
			printf("[SD%d] EMMC boot read error(%d)\n", host->id,derr);
			msdc_abort_handler(host, 1);
		}
	}

	return err;
}

void msdc_emmc_boot_reset(struct mmc_host *host, int reset)
{
	u32 base = host->base;
	u32 wints = MSDC_INT_CMDRDY | MSDC_INT_CMDTMO;
	u32 l_arg, l_cmd, status;
	u32 tmo=0xffffffff;

	switch (reset) {
		case EMMC_BOOT_PWR_RESET:
			msdc_hard_reset(host);
			break;
		case EMMC_BOOT_RST_N_SIG:
			if (msdc_cap[host->id].flags & MSDC_RST_PIN_EN) {
				/* set n_reset pin to low */
				MSDC_SET_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);

				/* tRSTW (RST_n pulse width) at least 1us */
				mdelay(1);

				/* set n_reset pin to high, mark this line if do boot ACK & boot DAT timeout test */
				MSDC_CLR_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);

				/* tRSCA (RST_n to command time) at least 200us,
				   tRSTH (RST_n high period) at least 1us */
				MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
				mdelay(1);
				MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
			}
			break;
		case EMMC_BOOT_PRE_IDLE_CMD:
			/* bring emmc to pre-idle mode by software reset command. (MMCv4.41)*/
			SDC_SEND_CMD(0x0, 0xF0F0F0F0);

			/* read SDC_ARG & SDC_CMD for avoid buffered register */
			l_arg = MSDC_READ32(SDC_ARG);
			l_cmd = MSDC_READ32(SDC_CMD);

			/* check cmd0 is send */
			status = msdc_intr_wait(host, wints);
			if (status & MSDC_INT_CMDTMO) {
				printf("[SD%d] CMD0:ERR(CMDTO)\n", host->id);
			}

			mdelay(1); //need delay to make sure pre-idle
			break;
	}
}
#endif

void msdc_reset_timing_register(struct mmc_host *host)
{
	u32 base = host->base;

	MSDC_WRITE32(MSDC_IOCON, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY2, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY3, 0x00000000);
	MSDC_WRITE32(MSDC_PATCH_BIT0, 0x403C000F);
	MSDC_WRITE32(MSDC_PATCH_BIT1, 0xFFFE00C9);
	//MSDC_PATCH_BIT1:WRDAT_CRCS_TA_CNTR need fix to 3'001 by default,(<50MHz) (>=50MHz set 3'001 as initial value is OK for tunning)
	//YD:CMD_RSP_TA_CNTR need fix to 3'001 by default(<50MHz)(>=50MHz set 3'001as initial value is OK for tunning)
	MSDC_WRITE32(MSDC_PATCH_BIT2, 0x14881803);
}

void msdc_select_delay_path(struct mmc_host *host, unsigned int path)
{
	u32 base = host->base;

	if ( path==MSDC_PATH_USE_ASYNC_FIFO ) {
		//PAD use data tune, common for CMD, Read, Write
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_RXDLYSEL, 0);

		//CMD
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP, 0);
		MSDC_SET_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLYSEL); //Both for CMD and Read
		MSDC_SET_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL);  //Both for CMD and Read

		//Read DAT
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 0);
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL, 0);
		MSDC_SET_FIELD(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL, 0);

		//Write CRC status
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS, 1);
	} else {
		//PAD use data tune, common for CMD, Read, Write
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_RXDLYSEL, 1);

		//CMD
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP, 1);
		MSDC_CLR_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLYSEL);
		MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL);

		//Read DAT
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL, 1);
		MSDC_SET_FIELD(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL, 1);

		//Write CRC status
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS, 0);
	}

}

int msdc_init(int id, struct mmc_host *host, int clksrc, int mode)
{
	u32 baddr[] = {MSDC0_BASE, MSDC1_BASE, MSDC2_BASE, MSDC3_BASE};
	u32 base = baddr[id];

	msdc_priv_t *priv;
	struct dma_config *cfg;

	printf("[%s]: msdc%d Host controller intialization start \n", __func__, id);
	clksrc = (clksrc == -1) ? msdc_cap[id].clk_src : clksrc;

	priv = &msdc_priv[id];
	cfg  = &priv->cfg;

	memset(priv, 0, sizeof(msdc_priv_t));

	host->id     = id;
	host->base   = base;

#if defined(MMC_MSDC_DRV_CTP)
#if !defined(FPGA_PLATFORM)
	if (host->id == 0) {
		msdc_src_clks = hclks_msdc50_0;
	} else {
		msdc_src_clks = hclks_msdc30_1;
	}
	host->f_max  = msdc_src_clks[clksrc];
#else
	host->f_max = msdc_src_clks[clksrc];
#endif

#else
#if !defined(FPGA_PLATFORM)
	host->f_max  = MSDC_MAX_SCLK;
#else
	host->f_max  = MSDC_MAX_SCLK>>1;
#endif

#endif

	host->f_min  = MSDC_MIN_SCLK;
	host->blkbits= MMC_BLOCK_BITS;
	host->blklen = 0;
	host->priv   = (void*)priv;

	host->caps   = MMC_CAP_MULTIWRITE;

	if (msdc_cap[id].flags & MSDC_HIGHSPEED)
		host->caps |= (MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED);
#if defined(FEATURE_MMC_UHS1)
	if (msdc_cap[id].flags & MSDC_UHS1)
		host->caps |= MMC_CAP_SD_UHS1;
#endif
	if (msdc_cap[id].flags & MSDC_DDR)
		host->caps |= MMC_CAP_DDR;
	if (msdc_cap[id].data_pins == 4)
		host->caps |= MMC_CAP_4_BIT_DATA;
	if (msdc_cap[id].data_pins == 8)
		host->caps |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
	if (msdc_cap[id].flags & MSDC_HS200)
		host->caps |= MMC_CAP_EMMC_HS200;
	if (msdc_cap[id].flags & MSDC_HS400)
		host->caps |= MMC_CAP_EMMC_HS400;

	host->ocr_avail = MMC_VDD_27_36;
	/* msdc0 only support 1.8 IO */
	if (host->caps & (MMC_CAP_EMMC_HS200 | MMC_CAP_EMMC_HS400))
		host->ocr_avail |= MMC_VDD_165_195;


	host->max_hw_segs   = MAX_DMA_TRAN_SIZE/512;
	host->max_phys_segs = MAX_DMA_TRAN_SIZE/512;
	host->max_seg_size  = MAX_DMA_TRAN_SIZE;
	host->max_blk_size  = 2048;
	host->max_blk_count = 65535;
	host->app_cmd = 0;
	host->app_cmd_arg = 0;

	priv->rdsmpl       = msdc_cap[id].data_edge;
	priv->wdsmpl       = msdc_cap[id].data_edge;
	priv->rsmpl       = msdc_cap[id].cmd_edge;

#if defined(MSDC_ENABLE_DMA_MODE)
	cfg->sg      = &priv->sg[0];
	cfg->burstsz = MSDC_BRUST_64B;
	cfg->flags   = DMA_FLAG_NONE;
	cfg->mode    = mode;
	cfg->inboot  = 0;

	msdc_init_gpd_bd(host);
	priv->alloc_bd    = 0;
	priv->alloc_gpd   = 0;
	priv->active_head = NULL;
	priv->active_tail = NULL;
#endif

#if defined(FPGA_PLATFORM)
	MSDC_WRITE32(PWR_GPIO_EO, PWR_MSDC); //setup GPIO mode (GPO or GPI)
	printf("set up GPIO for MSDC\n");
#endif

	// set current power level: VOL_1800 or VOL_3300
#if defined(USE_SDIO_1V8)
	if ( host->id!=0 ) {
		host->cur_pwr = VOL_1800;
	} else
#endif
	{
#if defined(FPGA_PLATFORM)
#if MSDC_USE_EMMC45_POWER
		host->cur_pwr = VOL_1800;
#else
		host->cur_pwr = VOL_3300;
#endif
#else
		if ( host->id==0 )
			host->cur_pwr = VOL_1800;
		else
			host->cur_pwr = VOL_3300;
#endif
	}

	msdc_clock(host, 1);
	msdc_power(host, MMC_POWER_OFF);
	msdc_power(host, MMC_POWER_ON);

	/* set to SD/MMC mode */
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
	MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO);

	MSDC_RESET();
	MSDC_CLR_FIFO();
	MSDC_CLR_INT();

	/* Disable async fifo use internal delay*/
	/* Commentted out and-then moved into msdc_select_delay_path
	MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
	MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP);
	*/

	/* enable SDIO mode. it's must otherwise sdio command failed */
	MSDC_SET_BIT32(SDC_CFG, SDC_CFG_SDIO);

	/* disable detect SDIO device interupt function */
	MSDC_CLR_BIT32(SDC_CFG, SDC_CFG_SDIOIDE);

	/* enable wake up events */
#if defined(MMC_MSDC_DRV_CTP)
	MSDC_SET_BIT32(SDC_CFG, SDC_CFG_INSWKUP);
#endif

	/* reset tuning parameter */
	msdc_reset_timing_register(host);
#if defined(FEATURE_ASYNC_PATH_ENABLE)
	msdc_select_delay_path(host, MSDC_PATH_USE_ASYNC_FIFO);
#else
	msdc_select_delay_path(host, MSDC_PATH_USE_DELAY_LINE);
#endif

	/* Disable support 64G */
	MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_SUPPORT64G);

#if !defined(FPGA_PLATFORM)
	msdc_gpio_and_pad_init(host);
#endif
	/* disable boot function, else eMMC intialization may be failed after BROM ops. */
	MSDC_CLR_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);

	/* set sampling edge */
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, msdc_cap[host->id].cmd_edge);
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, msdc_cap[host->id].data_edge);

	/* write crc timeout detection */
	MSDC_SET_FIELD(MSDC_PATCH_BIT0, 1 << 30, 1);
#ifdef FPGA_PLATFORM
	/* This must be set to 2, or DDR mode will have problem in FPGA platform */
	MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, 2);
#endif
#if defined(MMC_MSDC_DRV_CTP)
#if (MSDC_USE_FORCE_FLUSH || MSDC_USE_RELIABLE_WRITE || MSDC_USE_DATA_TAG || MSDC_USE_PACKED_CMD)
	MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_BLKNUM_SEL, 0);
#else
	MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_BLKNUM_SEL, 1);
#endif
#endif

	msdc_set_startbit(host, START_AT_RISING);

	msdc_config_clksrc(host, clksrc);
	msdc_config_bus(host, HOST_BUS_WIDTH_1);
	msdc_config_clock(host, 0, MSDC_MIN_SCLK, 0);

	msdc_set_dmode(host, mode);
	msdc_set_pio_bits(host, 32);

	/* disable sdio interrupt by default. sdio interrupt enable upon request */
	msdc_intr_unmask(host, 0x0001FF7B);

	msdc_irq_init(host);

	msdc_set_timeout(host, 100000000, 0);
#if defined(FEATURE_MMC_CARD_DETECT)
	msdc_card_detect(host, 1);
#endif
#if defined(MSDC_USE_DCM)
	dcm_disable(ALL_DCM);
	dcm_enable(MSDC_DCM);
#endif

	if ((host->id == 0) || (host->id == 1)) {
		/* disable SDIO func */
		MSDC_SET_FIELD(SDC_CFG, SDC_CFG_SDIO, 0);
		MSDC_SET_FIELD(SDC_CFG, SDC_CFG_SDIOIDE, 0);
		MSDC_SET_FIELD(SDC_CFG, SDC_CFG_INSWKUP, 0);
	}

	printf("[%s]: msdc%d Host controller intialization done\n", __func__, id);
	return 0;
}

#if defined(MSDC_WITH_DEINIT)
int msdc_deinit(struct mmc_host *host)
{
	u32 base = host->base;

#if defined(FEATURE_MMC_CARD_DETECT)
	msdc_card_detect(host, 0);
#endif

	msdc_intr_mask(host, 0x0001FFFB);
	msdc_irq_deinit(host);

	MSDC_RESET();
	MSDC_CLR_FIFO();
	MSDC_CLR_INT();
	msdc_power(host, MMC_POWER_OFF);

	return 0;
}
#endif

int msdc_polling_CD_interrupt(struct mmc_host *host)
{
	u32 base = host->base;
	u32 intsts;
	intsts = MSDC_READ32(MSDC_INT);
	MSDC_WRITE32(MSDC_INT, intsts);
	//printf("SDIO INT(0x%x)\n",intsts);
	if (intsts & MSDC_INT_CDSC)
		return 1;
	else
		return 0;
}

