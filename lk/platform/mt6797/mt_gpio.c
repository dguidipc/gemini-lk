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

/******************************************************************************
 * mt_gpio.c - MTKLinux GPIO Device Driver
 *
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 *
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/

#include <platform/mt_reg_base.h>
#include <platform/mt_gpio.h>
#include <platform/gpio_cfg.h>
#include <debug.h>
/******************************************************************************
 MACRO Definition
******************************************************************************/
//#define  GIO_SLFTEST
#define GPIO_DEVICE "mt-gpio"
#define VERSION     GPIO_DEVICE
#define GPIO_BRINGUP
/*---------------------------------------------------------------------------*/
#define GPIO_WR32(addr, data)   DRV_WriteReg32(addr,data)
#define GPIO_RD32(addr)         DRV_Reg32(addr)
#define GPIO_SW_SET_BITS(BIT,REG)   GPIO_WR32(REG,GPIO_RD32(REG) | ((unsigned long)(BIT)))
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile u32*)(REG)) = (u32)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile u32*)(REG)) &= ~((u32)(BIT)))

/*---------------------------------------------------------------------------*/
#define TRUE                   1
#define FALSE                  0
/*---------------------------------------------------------------------------*/
//#define MAX_GPIO_REG_BITS      16
//#define MAX_GPIO_MODE_PER_REG  5
//#define GPIO_MODE_BITS         3
/*---------------------------------------------------------------------------*/
#define GPIOTAG                "[GPIO] "
#define GPIOLOG(fmt, arg...)   dprintf(INFO,GPIOTAG fmt, ##arg)
#define GPIOMSG(fmt, arg...)   dprintf(INFO,fmt, ##arg)
#define GPIOERR(fmt, arg...)   dprintf(INFO,GPIOTAG "%5d: "fmt, __LINE__, ##arg)
#define GPIOFUC(fmt, arg...)   //dprintk(INFO,GPIOTAG "%s\n", __FUNCTION__)
#define GIO_INVALID_OBJ(ptr)   ((ptr) != gpio_obj)
/******************************************************************************
Enumeration/Structure
******************************************************************************/
#if ( defined(MACH_FPGA) && !defined (FPGA_SIMULATION) )

S32 mt_set_gpio_dir(u32 pin, u32 dir)           {return RSUCCESS;}
S32 mt_get_gpio_dir(u32 pin)                {return GPIO_DIR_UNSUPPORTED;}
S32 mt_set_gpio_pull_enable(u32 pin, u32 enable)    {return RSUCCESS;}
S32 mt_get_gpio_pull_enable(u32 pin)            {return GPIO_PULL_EN_UNSUPPORTED;}
S32 mt_set_gpio_pull_select(u32 pin, u32 select)    {return RSUCCESS;}
S32 mt_get_gpio_pull_select(u32 pin)            {return GPIO_PULL_UNSUPPORTED;}
S32 mt_set_gpio_smt(u32 pin, u32 enable)        {return RSUCCESS;}
S32 mt_get_gpio_smt(u32 pin)                {return GPIO_SMT_UNSUPPORTED;}
S32 mt_set_gpio_ies(u32 pin, u32 enable)        {return RSUCCESS;}
S32 mt_get_gpio_ies(u32 pin)                {return GPIO_IES_UNSUPPORTED;}
S32 mt_set_gpio_out(u32 pin, u32 output)        {return RSUCCESS;}
S32 mt_get_gpio_out(u32 pin)                {return GPIO_OUT_UNSUPPORTED;}
S32 mt_get_gpio_in(u32 pin)                 {return GPIO_IN_UNSUPPORTED;}
S32 mt_set_gpio_mode(u32 pin, u32 mode)         {return RSUCCESS;}
S32 mt_get_gpio_mode(u32 pin)               {return GPIO_MODE_UNSUPPORTED;}
#else

typedef struct {
	S32 en_flag;// 1 enable
} GPIO_PUPD_flag;

GPIO_PUPD_flag gpio_pupd_en_flag[MT_GPIO_BASE_MAX] = {0
                                                     };


struct mt_gpio_obj {
	GPIO_REGS       *reg;
};
static struct mt_gpio_obj gpio_dat = {
	.reg  = (GPIO_REGS*)(GPIO_BASE),
};
static struct mt_gpio_obj *gpio_obj = &gpio_dat;
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_dir_chip(u32 pin, u32 dir)
{
	u32 pos;
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;

	//pos = pin / MAX_GPIO_REG_BITS;
	bit = DIR_offset[pin].offset;
#ifdef GPIO_BRINGUP
	reg = GPIO_RD32(DIR_addr[pin].addr);
	if (dir == GPIO_DIR_IN)
		reg &= (~(1<<bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(DIR_addr[pin].addr,reg);
#else
	if (dir == GPIO_DIR_IN)
		GPIO_SET_BITS((1L << bit), &obj->reg->dir[pos].rst);
	else
		GPIO_SET_BITS((1L << bit), &obj->reg->dir[pos].set);
#endif
	return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_dir_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DIR_offset[pin].offset;

	reg = GPIO_RD32(DIR_addr[pin].addr);
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_enable_chip(u32 pin, u32 enable)
{
	u32 reg=0;
	u32 bit=0;
	int err =0;
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	        && -1 == PD_offset[pin].offset)
		return GPIO_PULL_UNSUPPORTED;

	if (GPIO_PULL_DISABLE == enable) {
		/*r0==0 && r1 ==0*/
		err = mt_set_gpio_pull_select_chip(pin,GPIO_NO_PULL);
		return err;
	} else {
		err = mt_set_gpio_pull_select_chip(pin,GPIO_PULL_UP);
		return err;
	}
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_enable_chip(u32 pin)
{
	int res = 0;
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	res = mt_get_gpio_pull_select_chip(pin);

	if (2 == res)
		return 0; /*disable*/
	if (1 == res || 0 == res)
		return 1; /*enable*/
	if (-1 == res)
		return -1;
	return -ERWRAPPER;

}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_smt_chip(u32 pin, u32 enable)
{
	//unsigned long flags;
	u32 reg=0;
	u32 bit=0;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#ifdef GPIO_BRINGUP

	if (SMT_offset[pin].offset == -1) {
		return GPIO_SMT_UNSUPPORTED;

	} else {
		bit = SMT_offset[pin].offset;
		reg = GPIO_RD32(SMT_addr[pin].addr);
		if (enable == GPIO_SMT_DISABLE)
			reg &= (~(1<<bit));
		else
			reg |= (1 << bit);
	}
	//dprintf(CRITICAL, "SMT addr(%x),value(%x)\n",SMT_addr[pin].addr,GPIO_RD32(SMT_addr[pin].addr));
	GPIO_WR32(SMT_addr[pin].addr, reg);
#else

	if (SMT_offset[pin].offset == -1) {
		return GPIO_SMT_UNSUPPORTED;
	} else {
		if (enable == GPIO_SMT_DISABLE)
			GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), SMT_addr[pin].addr + 8);
		else
			GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), SMT_addr[pin].addr + 4);
	}
#endif

	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_smt_chip(u32 pin)
{
	unsigned long data;
	u32 bit =0;
	bit = SMT_offset[pin].offset;
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset == -1) {
		return GPIO_SMT_UNSUPPORTED;
	} else {
		data = GPIO_RD32(SMT_addr[pin].addr);
		return (((data & (1L << bit)) != 0)? 1: 0);
	}
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_ies_chip(u32 pin, u32 enable)
{
	//unsigned long flags;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == -1) {
		return GPIO_IES_UNSUPPORTED;
	} else {
		if (enable == GPIO_IES_DISABLE)
			GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 8);
		else
			GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 4);
	}

	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_ies_chip(u32 pin)
{
	unsigned long data;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == -1) {
		return GPIO_IES_UNSUPPORTED;
	} else {
		data = GPIO_RD32(IES_addr[pin].addr);

		return (((data & (1L << (IES_offset[pin].offset))) != 0)? 1: 0);
	}
}
/*---------------------------------------------------------------------------*/

/*
r0 ==0 && r1 ==0 High-Z(no pull)

*/

S32 mt_set_gpio_pull_select_rx_chip(u32 pin,u32 r0, u32 r1)
{
	u32 reg=0;
#ifdef GPIO_BRINGUP
	if (0 == r0) {
		reg = GPIO_RD32(R0_addr[pin].addr);
		reg &= (~(1<<R0_offset[pin].offset));
		GPIO_WR32(R0_addr[pin].addr, reg);
	}
	if (1 == r0) {
		reg = GPIO_RD32(R0_addr[pin].addr);
		reg |= (1<<R0_offset[pin].offset);
		GPIO_WR32(R0_addr[pin].addr, reg);
	}

	if (0 == r1) {
		reg = GPIO_RD32(R1_addr[pin].addr);
		reg &= (~(1<<R1_offset[pin].offset));
		GPIO_WR32(R1_addr[pin].addr, reg);
	}
	if (1 == r1) {
		reg = GPIO_RD32(R1_addr[pin].addr);
		reg |= (1<<R1_offset[pin].offset);
		GPIO_WR32(R1_addr[pin].addr, reg);
	}
#else
	if (0 == r0)
		GPIO_SET_BITS((1L << (R0_offset[pin].offset)), R0_addr[pin].addr + 8);
	if (1 == r0)
		GPIO_SET_BITS((1L << (R0_offset[pin].offset)), R0_addr[pin].addr + 4);

	if (0 == r1)
		GPIO_SET_BITS((1L << (R1_offset[pin].offset)), R1_addr[pin].addr + 8);
	if (1 == r1)
		GPIO_SET_BITS((1L << (R1_offset[pin].offset)), R1_addr[pin].addr + 4);
#endif
	return 0;
}

S32 mt_set_gpio_pull_select_chip(u32 pin, u32 select)
{
	//unsigned long flags;
	u32 reg=0;
	u32 pu_reg=0;
	u32 pd_reg=0;
	unsigned long pupd_bit = 0;
	unsigned long pu_bit = 0;
	unsigned long pd_bit = 0;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

#ifdef GPIO_BRINGUP


	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	        && -1 == PD_offset[pin].offset)
		return GPIO_PULL_UNSUPPORTED;

	if (-1 != PUPD_offset[pin].offset) {

		/*PULL UP */
		mt_set_gpio_pull_select_rx_chip(pin,0,1); /*must first set default value r0=0, r1 =1*/
		pupd_bit = PUPD_offset[pin].offset;
		reg = GPIO_RD32(PUPD_addr[pin].addr);
		if (select == GPIO_PULL_UP)
			reg &= (~(1<<pupd_bit));
		else if (select == GPIO_PULL_DOWN)
			reg |= (1 << pupd_bit);
		else if (GPIO_NO_PULL == select) {
			mt_set_gpio_pull_select_rx_chip(pin,0,0);
		} else
			return -ERWRAPPER;
		GPIO_WR32(PUPD_addr[pin].addr, reg);
		return RSUCCESS;
	}

	if (-1 != PU_offset[pin].offset && -1 != PD_offset[pin].offset) {

		pu_bit = PU_offset[pin].offset;
		pd_bit = PD_offset[pin].offset;

		pu_reg = GPIO_RD32(PU_addr[pin].addr);
		pd_reg = GPIO_RD32(PD_addr[pin].addr);

		if (select == GPIO_PULL_UP) {
			pu_reg |= (1 << pu_bit);
			pd_reg &= (~(1<<pd_bit));
		} else if (GPIO_PULL_DOWN ==  select) {
			pu_reg &= (~(1 << pu_bit));
			pd_reg |= (1 << pd_bit);
		} else if (GPIO_NO_PULL) {
			pd_reg &= (~(1 << pd_bit));
			pu_reg &= (~(1 << pu_bit));
		} else {
			return -ERWRAPPER;
		}
		GPIO_WR32(PU_addr[pin].addr, pu_reg);
		GPIO_WR32(PD_addr[pin].addr, pd_reg);

		return RSUCCESS;

	} else if (-1 != PU_offset[pin].offset) { /*pu==1 is pull up, pu ==0  is no pull*/
		pu_reg = GPIO_RD32(PU_addr[pin].addr);
		pu_bit = PU_offset[pin].offset;
		if (select == GPIO_PULL_UP) {
			pu_reg |= (1 << pu_bit);
		} else if (GPIO_PULL_DOWN ==  select) {
			return GPIO_NOPULLDOWN;
		} else if (GPIO_NO_PULL) {
			pu_reg &= (~(1 << pu_bit));
		} else
			return -ERWRAPPER;

		GPIO_WR32(PU_addr[pin].addr, pu_reg);
		return RSUCCESS;
	} else {
		pd_reg = GPIO_RD32(PD_addr[pin].addr);
		pd_bit = PD_offset[pin].offset;
		if (select == GPIO_PULL_UP) {
			return GPIO_NOPULLUP;
		} else if (GPIO_PULL_DOWN ==  select) {
			pd_reg |= (1 << pd_bit);
		} else if (GPIO_NO_PULL) {
			pd_reg &= (~(1 << pd_bit));
		} else
			return -ERWRAPPER;

		GPIO_WR32(PD_addr[pin].addr, pd_reg);
		return RSUCCESS;
	}

#else

	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	        && -1 == PD_offset[pin]) {
		return GPIO_PULL_UNSUPPORTED;
	}

	if (-1 != PUPD_offset[pin].offset) {
		if (select == GPIO_PULL_UP)
			GPIO_SET_BITS((1L << (PUPD_offset[pin].offset)), PUPD_addr[pin].addr + 8);
		else if (GPIO_PULL_DOWN ==  select)
			GPIO_SET_BITS((1L << (PUPD_offset[pin].offset)), PUPD_addr[pin].addr + 4);
		else if (GPIO_NO_PULL)
			mt_set_gpio_pull_select_rx_chip(pin,0,0);
		else
			return -ERWRAPPER;

		return RSUCCESS;
	}

	if (-1 != PU_offset[pin].offset && -1 != PD_offset[pin].offset) {
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr + 4);
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr + 8);
		} else if (GPIO_PULL_DOWN ==  select) {
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr + 4);
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr + 8);
		} else if (GPIO_NO_PULL) {
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr + 8);
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr + 8);
		} else
			return -ERWRAPPER;

		return RSUCCESS;
	} else if (-1 != PU_offset[pin].offset) {
		if (select == GPIO_PULL_UP)
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr + 4);
		else if (GPIO_PULL_DOWN ==  select)
			return GPIO_NOPULLDOWN;
		else if (GPIO_NO_PULL)
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr + 8);
		else
			return -ERWRAPPER;

		return RSUCCESS;
	} else {
		if (select == GPIO_PULL_UP) {
			return GPIO_NOPULLUP;
		} else if (GPIO_PULL_DOWN ==  select) {
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr + 4);
		} else if (GPIO_NO_PULL) {
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr + 8);
		} else
			return -ERWRAPPER;

		return RSUCCESS;
	}

#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_select_chip(u32 pin)
{

	unsigned long data;
	unsigned long r0;
	unsigned long r1;

	unsigned long bit = 0;
	unsigned long pu = 0;
	unsigned long pd = 0;

	u32 mask = (1L << 4) - 1;

	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	        && -1 == PD_offset[pin].offset) {
		return GPIO_PULL_UNSUPPORTED;
	}
	/*pupd==0 is pull up or no pull*/
	if (-1 != PUPD_offset[pin].offset) {
		data = GPIO_RD32(PUPD_addr[pin].addr);
		r0 = GPIO_RD32(R0_addr[pin].addr);
		r1 = GPIO_RD32(R1_addr[pin].addr);
		if (0 == (data & (1L << (PUPD_offset[pin].offset)))) {
			if (0 == (r0 & (1L << (R0_offset[pin].offset)))
			        && 0 == (r1 & (1L << (R1_offset[pin].offset))))
				return GPIO_NO_PULL;/*High Z(no pull)*/
			else
				return GPIO_PULL_UP; /* pull up */
		}
		/*pupd==1 is pull down or no pull*/
		if (0 != (data & (1L << (PUPD_offset[pin].offset)))) {
			if (0 == (r0 & (1L << (R0_offset[pin].offset)))
			        && 0 == (r1 & (1L << (R1_offset[pin].offset))))
				return GPIO_NO_PULL;/*High Z(no pull)*/
			else
				return GPIO_PULL_DOWN; /* pull down */
		}
	}

	if (-1 != PU_offset[pin].offset && -1 != PD_offset[pin].offset) {
		pu = GPIO_RD32(PU_addr[pin].addr);
		pd = GPIO_RD32(PD_addr[pin].addr);
		if (0 != (pu & (1L << (PU_offset[pin].offset))))
			pu =1;
		else
			pu=0;
		if (0 != (pd & (1L << (PD_offset[pin].offset))))
			pd =1;
		else
			pd=0;


		if (0 == pu && 0 == pd) /*no pull */
			return GPIO_NO_PULL;
		else if (1 == pu && 1 == pd)
			return -1; /* err */
		else if (1 == pu && 0 == pd)
			return GPIO_PULL_UP; /* pull up*/
		else
			return GPIO_PULL_DOWN; /* pull down*/

	} else if (-1 != PU_offset[pin].offset) { /*pu==1 is pull up, pu ==0  is no pull*/
		pu = GPIO_RD32(PU_addr[pin].addr);

		if (0 != (pu & (1L << (PU_offset[pin].offset))))
			return GPIO_PULL_UP; /* pull up*/
		else
			return GPIO_NO_PULL;/*High Z(no pull)*/
	} else {
		pd = GPIO_RD32(PD_addr[pin].addr);
		if (0 != (pd & (1L << (PD_offset[pin].offset))))
			return GPIO_PULL_DOWN; /* pull down*/
		else
			return GPIO_NO_PULL;/*High Z(no pull)*/
	}

}

#if 1
/*
gpio driving
0x00
0x01
0x10
0x11
*/
S32 mt_set_gpio_driving_chip(u32 pin, GPIO_DRV driving)
{
	u32 pos;
	u32 bit;
	u32 reg=0;
	//struct mt_gpio_obj *obj = gpio_obj;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DRV_offset[pin].offset;
	if (-1 == DRV_width[pin].width)
		return -ERWRAPPER;

#ifdef GPIO_BRINGUP
	if (3 == DRV_width[pin].width) {
		if (driving > 0x111)
			return -ERWRAPPER;
		reg = GPIO_RD32(DRV_addr[pin].addr);
		reg &= ~(0x111 << bit);
		GPIO_WR32(DRV_addr[pin].addr,reg);

		reg = GPIO_RD32(DRV_addr[pin].addr);
		reg |= (driving << bit);
		GPIO_WR32(DRV_addr[pin].addr,reg);
	} else if (2 == DRV_width[pin].width) {
		if (driving > 0x11)
			return -ERWRAPPER;
		reg = GPIO_RD32(DRV_addr[pin].addr);
		reg &= ~(0x11 << bit);
		GPIO_WR32(DRV_addr[pin].addr,reg);

		reg = GPIO_RD32(DRV_addr[pin].addr);
		reg |= (driving << bit);
		GPIO_WR32(DRV_addr[pin].addr,reg);

	} else if (1 == DRV_width[pin].width) {
		if (driving > 1)
			return -ERWRAPPER;
		reg = GPIO_RD32(DRV_addr[pin].addr);
		reg &= ~(0x1 << bit);
		GPIO_WR32(DRV_addr[pin].addr,reg);

		reg = GPIO_RD32(DRV_addr[pin].addr);
		reg |= (driving << bit);
		GPIO_WR32(DRV_addr[pin].addr,reg);
	}

#else
	if (3 == DRV_width[pin].width) {
		if (driving > 0x111)
			return -ERWRAPPER;
		//clear  all bit first
		GPIO_SET_BITS((0x111 << bit), DRV_addr[pin].addr + 8);
		GPIO_SET_BITS((driving << bit), DRV_addr[pin].addr + 4);

	} else if (2 == DRV_width[pin].width) {
		if (driving > 0x11)
			return -ERWRAPPER;
		//clear  all bit first
		GPIO_SET_BITS((0x11 << bit), DRV_addr[pin].addr + 8);
		GPIO_SET_BITS((driving << bit), DRV_addr[pin].addr + 4);


	} else if (1 == DRV_width[pin].width) {
		if (driving > 1)
			return -ERWRAPPER;
		//clear  all bit first
		GPIO_SET_BITS((0x1 << bit), DRV_addr[pin].addr + 8);
		GPIO_SET_BITS((driving << bit), DRV_addr[pin].addr + 4);

	}


#endif
	return RSUCCESS;

}
#endif
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out_chip(u32 pin, u32 output)
{
	u32 pos;
	u32 bit;
	u32 reg=0;
	//struct mt_gpio_obj *obj = gpio_obj;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;

	bit = DATAOUT_offset[pin].offset;

#ifdef GPIO_BRINGUP
	reg = GPIO_RD32(DATAOUT_addr[pin].addr);
	if (output == GPIO_OUT_ZERO)
		reg &= (~(1<<bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(DATAOUT_addr[pin].addr,reg);

#else

	if (output == GPIO_OUT_ZERO)
		GPIO_SET_BITS((1L << bit), &obj->reg->dout[pos].rst);
	else
		GPIO_SET_BITS((1L << bit), &obj->reg->dout[pos].set);
#endif
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_out_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = GPIO_RD32(&obj->reg->dout[pos].val);
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_in_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DATAIN_offset[pin].offset;
	reg = GPIO_RD32(DATAIN_addr[pin].addr);
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_mode_chip(u32 pin, u32 mode)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

	//pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = MODE_offset[pin].offset;

#ifdef GPIO_BRINGUP
	if (MODEXT_offset[pin].offset != -1) {
		reg = GPIO_RD32(GPIO_BASE + 0x600);
		reg &= (~(0x1 << MODEXT_offset[pin].offset));
		reg |= ((mode >> 0x3) << MODEXT_offset[pin].offset);
		GPIO_WR32(GPIO_BASE + 0x600, reg);
	}

	mode = mode & 0x7;
	//dprintf(CRITICAL, "fwq pin=%d,mode=%d, offset=%d\n",pin,mode,bit);

	reg = GPIO_RD32(MODE_addr[pin].addr);
	//dprintf(CRITICAL, "fwq pin=%d,beforereg[%x]=%x\n",pin,MODE_addr[pin].addr,reg);

	reg &= (~(mask << bit));
	//dprintf(CRITICAL, "fwq pin=%d,clr=%x\n",pin,~(mask << (GPIO_MODE_BITS*bit)));
	reg |= (mode << bit);
	//dprintf(CRITICAL, "fwq pin=%d,reg[%x]=%x\n",pin,MODE_addr[pin].addr,reg);

	GPIO_WR32( MODE_addr[pin].addr,reg);
	//dprintf(CRITICAL, "fwq pin=%d,moderead=%x\n",pin,GPIO_RD32(MODE_addr[pin].addr));

#else

	reg = ((1L << (GPIO_MODE_BITS*bit)) << 3) | (mode << (GPIO_MODE_BITS*bit));

	GPIO_WR32(&obj->reg->mode[pos]._align1, reg);
#endif
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_mode_chip(u32 pin)
{

	u32 bit;
	u32 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	//struct mt_gpio_obj *obj = gpio_obj;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;


	bit = MODE_offset[pin].offset;
	// dprintf(CRITICAL, "fwqread bit=%d,offset=%d",bit,MODE_offset[pin].offset);
	//reg = GPIO_RD32(&obj->reg->mode[pos].val);
	reg = GPIO_RD32(MODE_addr[pin].addr);
	// dprintf(CRITICAL, "fwqread  pin=%d,moderead=%x, bit=%d\n",pin,GPIO_RD32(MODE_addr[pin].addr),bit);
	if (MODEXT_offset[pin].offset != -1 &&
		(GPIO_RD32(GPIO_BASE + 0x600) >> MODEXT_offset[pin].offset) & 0x1)
		return ((reg >> bit) & mask) | 0x8;
	return ((reg >> bit) & mask);
}
/*---------------------------------------------------------------------------*/
void mt_gpio_pin_decrypt(u32 *cipher)
{
	//just for debug, find out who used pin number directly
	if ((*cipher & (0x80000000)) == 0) {
#ifndef DUMMY_AP
		GPIOERR("GPIO%u HARDCODE warning!!! \n",(unsigned int)*cipher);
		//dump_stack();
		//return;
#endif
	}

	//GPIOERR("Pin magic number is %x\n",*cipher);
	*cipher &= ~(0x80000000);
	return;
}
//set GPIO function in fact
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_dir(u32 pin, u32 dir)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_dir_chip(pin,dir);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_dir(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_dir_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_enable(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_pull_enable_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_enable(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_pull_enable_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_select(u32 pin, u32 select)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_pull_select_chip(pin,select);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_select(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_pull_select_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_smt(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_smt_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_smt(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_smt_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_ies(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_ies_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_ies(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_ies_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out(u32 pin, u32 output)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_out_chip(pin,output);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_out(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_out_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_in(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_in_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_mode(u32 pin, u32 mode)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_mode_chip(pin,mode);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_mode(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_get_gpio_mode_chip(pin);
}

S32 mt_set_gpio_drving(u32 pin, GPIO_DRV driving)
{
	mt_gpio_pin_decrypt(&pin);
	return mt_set_gpio_driving_chip(pin,driving);
}


/*****************************************************************************/
/* sysfs operation                                                           */
/*****************************************************************************/

#endif
