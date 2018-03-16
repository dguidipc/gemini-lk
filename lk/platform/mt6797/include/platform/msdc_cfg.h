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

#ifndef _MSDC_CFG_H_
#define _MSDC_CFG_H_

//#define MMC_MSDC_DRV_CTP
//#define MMC_MSDC_DRV_PRELOADER
#define MMC_MSDC_DRV_LK

#if !defined(MMC_MSDC_DRV_CTP) && !defined(MMC_MSDC_DRV_PRELOADER) && !defined(MMC_MSDC_DRV_LK)
#error Please define of MMC_MSDC_DRV_CTP, MMC_MSDC_DRV_PRELOADER, MMC_MSDC_DRV_LK
#elif defined(MMC_MSDC_DRV_CTP) && defined(MMC_MSDC_DRV_PRELOADER)
#error Please define only one of MMC_MSDC_DRV_CTP, MMC_MSDC_DRV_PRELOADER, MMC_MSDC_DRV_LK
#elif defined(MMC_MSDC_DRV_CTP) && defined(MMC_MSDC_DRV_LK)
#error Please define only one of MMC_MSDC_DRV_CTP, MMC_MSDC_DRV_PRELOADER, MMC_MSDC_DRV_LK
#elif defined(MMC_MSDC_DRV_PRELOADER) && defined(MMC_MSDC_DRV_LK)
#error Please define only one of MMC_MSDC_DRV_CTP, MMC_MSDC_DRV_PRELOADER, MMC_MSDC_DRV_LK
#endif

/*--------------------------------------------------------------------------*/
/* FPGA Definition                                                          */
/*--------------------------------------------------------------------------*/
/* LK Change to PL/CTP define */
#if defined(MMC_MSDC_DRV_LK)
#ifdef MACH_FPGA
#define CFG_FPGA_PLATFORM       (1)
#else
#define CFG_FPGA_PLATFORM       (0)
#endif
#endif

#if defined(MMC_MSDC_DRV_CTP)
/* Change to 0 when DVT */
#define CFG_FPGA_PLATFORM           (0)
#endif

/* Redifine a common define */
#if CFG_FPGA_PLATFORM
#define FPGA_PLATFORM
#else
/* #define MTK_MSDC_BRINGUP_DEBUG */
#endif

/*--------------------------------------------------------------------------*/
/* Transfer Mode Definition                                                 */
/*--------------------------------------------------------------------------*/
#define MSDC_ENABLE_DMA_MODE

#if defined(MMC_MSDC_DRV_CTP)
#define MSDC_ENABLE_ENH_DMA_MODE
#endif

#if !defined(MSDC_ENABLE_DMA_MODE) && defined(MSDC_ENABLE_ENH_DMA_MODE)
#error Please define MSDC_ENABLE_DMA_MODE first
#endif

//#define MSDC_MODE_DEFAULT_PIO
//#define MSDC_MODE_DEFAULT_DMA_BASIC
//#define MSDC_MODE_DEFAULT_DMA_DESC
//#define MSDC_MODE_DEFAULT_DMA_ENHANCED

#if defined(MMC_MSDC_DRV_CTP)
#define MSDC_MODE_DEFAULT_PIO
#endif

#if defined(MMC_MSDC_DRV_PRELOADER) || defined(MMC_MSDC_DRV_LK)
#if defined(MSDC_ENABLE_DMA_MODE)
#define MSDC_MODE_DEFAULT_DMA_DESC
#else
#define MSDC_MODE_DEFAULT_PIO
#endif
#endif


/*--------------------------------------------------------------------------*/
/* Speed Mode Definition                                                    */
/*--------------------------------------------------------------------------*/
#if defined(MMC_MSDC_DRV_CTP)
#define FEATURE_MMC_UHS1
#endif


/*--------------------------------------------------------------------------*/
/* Tuning Definition                                                        */
/*--------------------------------------------------------------------------*/
#define FEATURE_MMC_WR_TUNING
#define FEATURE_MMC_RD_TUNING
#define FEATURE_MMC_CM_TUNING


/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#if defined(MMC_MSDC_DRV_PRELOADER)
#define FEATURE_MMC_SLIM
#endif

#if defined(MMC_MSDC_DRV_LK)
#define MSDC_WITH_DEINIT
#endif

#define MTK_HS400_USED_800M       (0)

#if defined(FPGA_PLATFORM)
#define MSDC_USE_EMMC45_POWER     (1)
#endif

#if defined(MMC_MSDC_DRV_CTP)
#define FEATURE_MMC_CARD_DETECT

/* CTP use autocmd23 */
#define MSDC_USE_DATA_TAG         (0) /* autocmd23 with data tag */
#define MSDC_USE_RELIABLE_WRITE   (1) /* autocmd23 with reliable write */
#define MSDC_USE_FORCE_FLUSH      (0) /* autocmd23 with force flush cache */
#define MSDC_USE_PACKED_CMD       (0) /* autocmd23 with packed cmd, this feature is conflict with data tag, reliable write, and force flush cache */

#define FEATURE_MMC_BOOT_MODE

/* For CTP Only, but not necessary for CTP */
//#define FEATURE_MMC_SDIO
//#define MSDC_USE_DCM
//#define MSDC_USE_IRQ

//#define FEATURE_MMC_SW_CMDQ
//#define FEATURE_MMC_SW_CMDQ_CMD44and45_WHILE_DATA_XFR //unused, to be removed
//#define FEATURE_MMC_HW_CMDQ
//#define FEATURE_MMC_USE_EMMC51_CFG0_FOR_CMD44_45

#if defined(FEATURE_MMC_SW_CMDQ) || defined(FEATURE_MMC_SW_CMDQ_CMD44and45_WHILE_DATA_XFR) || defined(FEATURE_MMC_HW_CMDQ) || defined(FEATURE_MMC_USE_EMMC51_CFG0_FOR_CMD44_45)
#define FEATURE_MMC_CMDQ
#endif

//#define FEATURE_NONBLOCKING_RW

#endif


/*--------------------------------------------------------------------------*/
/* Test Definition                                                         */
/*--------------------------------------------------------------------------*/
#define MMC_TEST
//#define MTK_MSDC_PL_TEST
/*--------------------------------------------------------------------------*/
/* Debug Definition                                                         */
/*--------------------------------------------------------------------------*/
#define MMC_DEBUG               (0)
#define MSDC_DEBUG              (0)

#if MSDC_DEBUG
#define MSG_DEBUG
#endif

#define MSDC_TUNE_LOG           (1)

#define MSDC_SLT_MASK_LOG       (1)

//#define USE_SDIO_1V8

//BEGIN Experiment New Features
//#define FEATURE_ASYNC_PATH_ENABLE
//END

#endif /* end of _MSDC_CFG_H_ */

