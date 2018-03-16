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

#ifndef _MT_GPIO_H_
#define _MT_GPIO_H_

#include <kernel/mutex.h>
#include <debug.h>
#include <platform/mt_typedefs.h>
#if (!defined(MACH_FPGA))
#include <cust_gpio_usage.h>
#endif
#include <platform/gpio_const.h>



//#define FPGA_SIMULATION
//#define SELF_TEST


mutex_t gpio_mutex;

/*----------------------------------------------------------------------------*/
//  Error Code No.
#define RSUCCESS    0
#define ERACCESS    1
#define ERINVAL     2
#define ERWRAPPER   3


/*----------------------------------------------------------------------------*/
#ifndef s32
#define s32 signed int
#endif
#ifndef s64
#define s64 signed long long
#endif
/*----------------------------------------------------------------------------*/

#define MAX_GPIO_PIN    (MT_GPIO_BASE_MAX)
/******************************************************************************
* Enumeration for GPIO pin
******************************************************************************/
typedef enum {
	GPIO_DRV_UNSUPPORTED = -1,
	GPIO_DRV0   = 0x00,
	GPIO_DRV1   = 0x01,
	GPIO_DRV2   = 0x10,
	GPIO_DRV3   = 0x11,
	GPIO_DRV4   = 0x100,
	GPIO_DRV5   = 0x101,
	GPIO_DRV6   = 0x110,
	GPIO_DRV7   = 0x111,

} GPIO_DRV;


/* GPIO MODE CONTROL VALUE*/
typedef enum {
	GPIO_MODE_UNSUPPORTED = -1,
	GPIO_MODE_GPIO  = 0,
	GPIO_MODE_00    = 0,
	GPIO_MODE_01    = 1,
	GPIO_MODE_02    = 2,
	GPIO_MODE_03    = 3,
	GPIO_MODE_04    = 4,
	GPIO_MODE_05    = 5,
	GPIO_MODE_06    = 6,
	GPIO_MODE_07    = 7,
	GPIO_MODE_08    = 8,
	GPIO_MODE_09    = 9,
	GPIO_MODE_10    = 10,
	GPIO_MODE_11    = 11,
	GPIO_MODE_12    = 12,
	GPIO_MODE_13    = 13,
	GPIO_MODE_14    = 14,
	GPIO_MODE_15    = 15,

	GPIO_MODE_MAX,
	GPIO_MODE_DEFAULT = GPIO_MODE_01,
} GPIO_MODE;
/*----------------------------------------------------------------------------*/
/* GPIO DIRECTION */
typedef enum {
	GPIO_DIR_UNSUPPORTED = -1,
	GPIO_DIR_IN     = 0,
	GPIO_DIR_OUT    = 1,

	GPIO_DIR_MAX,
	GPIO_DIR_DEFAULT = GPIO_DIR_IN,
} GPIO_DIR;
/*----------------------------------------------------------------------------*/
/* GPIO PULL ENABLE*/
typedef enum {
	GPIO_PULL_EN_UNSUPPORTED = -1,
	GPIO_NOPULLUP       = -4,
	GPIO_NOPULLDOWN     = -5,
	GPIO_PULL_DISABLE = 0,
	GPIO_PULL_ENABLE  = 1,

	GPIO_PULL_EN_MAX,
	GPIO_PULL_EN_DEFAULT = GPIO_PULL_ENABLE,
} GPIO_PULL_EN;
/*----------------------------------------------------------------------------*/
/* GPIO SMT*/
typedef enum {
	GPIO_SMT_UNSUPPORTED = -1,
	GPIO_SMT_DISABLE = 0,
	GPIO_SMT_ENABLE  = 1,

	GPIO_SMT_MAX,
	GPIO_SMT_DEFAULT = GPIO_SMT_ENABLE,
} GPIO_SMT;
/*----------------------------------------------------------------------------*/
/* GPIO IES*/
typedef enum {
	GPIO_IES_UNSUPPORTED = -1,
	GPIO_IES_DISABLE = 0,
	GPIO_IES_ENABLE  = 1,

	GPIO_IES_MAX,
	GPIO_IES_DEFAULT = GPIO_IES_ENABLE,
} GPIO_IES;
/*----------------------------------------------------------------------------*/
/* GPIO PULL-UP/PULL-DOWN*/
typedef enum {
	GPIO_PULL_UNSUPPORTED = -1,
	GPIO_PULL_DOWN  = 0,
	GPIO_PULL_UP    = 1,
	GPIO_NO_PULL = 2,

	GPIO_PULL_MAX,
	GPIO_PULL_DEFAULT = GPIO_PULL_DOWN
} GPIO_PULL;
/*----------------------------------------------------------------------------*/
/* GPIO OUTPUT */
typedef enum {
	GPIO_OUT_UNSUPPORTED = -1,
	GPIO_OUT_ZERO = 0,
	GPIO_OUT_ONE  = 1,

	GPIO_OUT_MAX,
	GPIO_OUT_DEFAULT = GPIO_OUT_ZERO,
	GPIO_DATA_OUT_DEFAULT = GPIO_OUT_ZERO,  /*compatible with DCT*/
} GPIO_OUT;
/*----------------------------------------------------------------------------*/
/* GPIO INPUT */
typedef enum {
	GPIO_IN_UNSUPPORTED = -1,
	GPIO_IN_ZERO = 0,
	GPIO_IN_ONE  = 1,

	GPIO_IN_MAX,
} GPIO_IN;
/*----------------------------------------------------------------------------*/
/* GPIO POWER*/
typedef enum {
	GPIO_VIO28 = 0,
	GPIO_VIO18 = 1,
	GPIO_GPIO_VIO18 = 2,
	GPIO_VMC = 3,
	GPIO_VSIM = 4,
	GPIO_VIO_MAX,
} GPIO_POWER;
/*----------------------------------------------------------------------------*/
typedef struct {
	unsigned int val;
	unsigned int set;
	unsigned int rst;
	unsigned int _align1;
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS    dir[7];            /*0x0000 ~ 0x006F: 112 bytes*/
	unsigned char       rsv00[144];  /*0x00E0 ~ 0x00FF:     32 bytes*/
	VAL_REGS    dout[7];           /*0x0400 ~ 0x04DF: 224 bytes*/
	unsigned char       rsv04[144];  /*0x04B0 ~ 0x04FF:     32 bytes*/
	VAL_REGS    din[7];            /*0x0500 ~ 0x05DF: 224 bytes*/
	unsigned char       rsv05[144]; /*0x05E0 ~ 0x05FF:  32 bytes*/
	VAL_REGS    mode[21];           /*0x0600 ~ 0x08AF: 688 bytes*/

} GPIO_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	unsigned int no     : 16;
	unsigned int mode   : 3;
	unsigned int pullsel: 1;
	unsigned int din    : 1;
	unsigned int dout   : 1;
	unsigned int pullen : 1;
	unsigned int dir    : 1;
	unsigned int dinv   : 1;
	unsigned int _align : 7;
} GPIO_CFG;
/******************************************************************************
* GPIO Driver interface
******************************************************************************/
/*direction*/
S32 mt_set_gpio_dir(u32 pin, u32 dir);
S32 mt_get_gpio_dir(u32 pin);

/*pull enable*/
S32 mt_set_gpio_pull_enable(u32 pin, u32 enable);
S32 mt_get_gpio_pull_enable(u32 pin);
/*pull select*/
S32 mt_set_gpio_pull_select(u32 pin, u32 select);
S32 mt_get_gpio_pull_select(u32 pin);

/*schmitt trigger*/
S32 mt_set_gpio_smt(u32 pin, u32 enable);
S32 mt_get_gpio_smt(u32 pin);

/*IES*/
S32 mt_set_gpio_ies(u32 pin, u32 enable);
S32 mt_get_gpio_ies(u32 pin);

/*data inversion*/
//S32 mt_set_gpio_inversion(u32 pin, u32 enable);
//S32 mt_get_gpio_inversion(u32 pin);

/*input/output*/
S32 mt_set_gpio_out(u32 pin, u32 output);
S32 mt_get_gpio_out(u32 pin);
S32 mt_get_gpio_in(u32 pin);

/*mode control*/
S32 mt_set_gpio_mode(u32 pin, u32 mode);
S32 mt_get_gpio_mode(u32 pin);

/*misc functions for protect GPIO*/
//void mt_gpio_unlock_init(int all);
void mt_gpio_set_default(void);
void mt_gpio_dump(void);
void mt_gpio_load(GPIO_REGS *regs);
void mt_gpio_checkpoint_save(void);
void mt_gpio_checkpoint_compare(void);
void mt_gpio_set_default_dump(void);

#endif //_MT_GPIO_H_
