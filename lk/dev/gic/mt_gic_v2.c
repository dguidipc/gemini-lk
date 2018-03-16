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
#include <reg.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_irq.h>
#include <mt_gic.h>

#include <debug.h>

extern uint32_t mt_irq_get_target(void);

static void mt_gic_cpu_init(void)
{
	DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_PRIMASK, 0xF0);
	DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_CTRL, 0x1);
	dsb();
}

static void mt_gic_dist_init(void)
{
	unsigned int i;
	unsigned int cpumask = mt_irq_get_target();

	DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CTRL, 0);

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < (MT_NR_SPI + 32); i += 16) {
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CONFIG + i * 4 / 16, 0);
	}

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < (MT_NR_SPI + 32); i += 4) {
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_TARGET + i * 4 / 4, cpumask);
	}

	/*
	 * Set priority on all interrupts.
	 */
	for (i = 0; i < NR_IRQ_LINE; i += 4) {
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_PRI + i * 4 / 4, 0xA0A0A0A0);
	}

	/*
	* Disable all interrupts.
	*/
	for (i = 0; i < NR_IRQ_LINE; i += 32) {
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + i * 4 / 32, 0xFFFFFFFF);
	}

	dsb();

	DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CTRL, 1);
}

void platform_init_interrupts(void)
{
	mt_gic_dist_init();
	mt_gic_cpu_init();
}

void platform_deinit_interrupts(void)
{
	unsigned int irq;

	for (irq = 0; irq < NR_IRQ_LINE; irq += 32) {
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq * 4 / 32, 0xFFFFFFFF);
	}

	dsb();

	while ((irq = DRV_Reg32(GIC_CPU_BASE + GIC_CPU_INTACK)) != 1023 ) {
		DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_EOI, irq);
	}
}

uint32_t mt_irq_get(void)
{
	return DRV_Reg32(GIC_CPU_BASE + GIC_CPU_INTACK);
}

void mt_irq_set_polarity(unsigned int irq, unsigned int polarity)
{
	unsigned int offset;
	unsigned int reg_index;
	unsigned int value;

	// peripheral device's IRQ line is using GIC's SPI, and line ID >= GIC_PRIVATE_SIGNALS
	if (irq < GIC_PRIVATE_SIGNALS) {
		return;
	}

	offset = (irq - GIC_PRIVATE_SIGNALS) & 0x1F;
	reg_index = (irq - GIC_PRIVATE_SIGNALS) >> 5;
	if (polarity == 0) {
		value = DRV_Reg32(INT_POL_CTL0 + (reg_index * 4));
		value |= (1 << offset); // always invert the incoming IRQ's polarity
		DRV_WriteReg32((INT_POL_CTL0 + (reg_index * 4)), value);
	} else {
		value = DRV_Reg32(INT_POL_CTL0 + (reg_index * 4));
		value &= ~(0x1 << offset);
		DRV_WriteReg32(INT_POL_CTL0 + (reg_index * 4), value);
	}
}

void mt_irq_set_sens(unsigned int irq, unsigned int sens)
{
	unsigned int config;

	if (sens == MT65xx_EDGE_SENSITIVE) {
		config = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
		config |= (0x2 << (irq % 16) * 2);
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4, config);
	} else {
		config = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
		config &= ~(0x2 << (irq % 16) * 2);
		DRV_WriteReg32( GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4, config);
	}
	dsb();
}

/*
 * mt_irq_mask: mask one IRQ
 * @irq: IRQ line of the IRQ to mask
 */
void mt_irq_mask(unsigned int irq)
{
	unsigned int mask = 1 << (irq % 32);

	DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4, mask);
	dsb();
}

/*
 * mt_irq_unmask: unmask one IRQ
 * @irq: IRQ line of the IRQ to unmask
 */
void mt_irq_unmask(unsigned int irq)
{
	unsigned int mask = 1 << (irq % 32);

	DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4, mask);
	dsb();
}

/*
 * mt_irq_ack: ack IRQ
 * @irq: IRQ line of the IRQ to mask
 */
void mt_irq_ack(unsigned int irq)
{
	DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_EOI, irq);
	dsb();
}

/*
 * mt_irq_mask_all: mask all IRQ lines. (This is ONLY used for the sleep driver)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_irq_mask_all(struct mtk_irq_mask *mask)
{
	unsigned int i;

	if (mask) {
		for (i = 0; i < IRQ_REGS; i++) {
			mask->mask[i] = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + i * 4);
			DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + i * 4, 0xFFFFFFFF);
		}

		dsb();

		mask->header = IRQ_MASK_HEADER;
		mask->footer = IRQ_MASK_FOOTER;

		return 0;
	} else {
		return -1;
	}
}

/*
 * mt_irq_mask_restore: restore all IRQ lines' masks. (This is ONLY used for the sleep driver)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_irq_mask_restore(struct mtk_irq_mask *mask)
{
	unsigned int i;

	if (!mask) {
		return -1;
	}
	if (mask->header != IRQ_MASK_HEADER) {
		return -1;
	}
	if (mask->footer != IRQ_MASK_FOOTER) {
		return -1;
	}

	for (i = 0; i < IRQ_REGS; i++) {
		DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + i * 4, mask->mask[i]);
	}

	dsb();


	return 0;
}
