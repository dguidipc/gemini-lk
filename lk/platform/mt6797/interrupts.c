/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arch/arm.h>
#include <reg.h>
#include <kernel/thread.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>
#include <mt_gic.h>
#include <platform/boot_mode.h>


#include <debug.h>

#define MPIDR_LEVEL_BITS 8
#define MPIDR_LEVEL_MASK    ((1 << MPIDR_LEVEL_BITS) - 1)
#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
        ((mpidr >> (MPIDR_LEVEL_BITS * level)) & MPIDR_LEVEL_MASK)


extern uint32_t mt_mpidr_read(void);
extern void lk_scheduler(void);
extern void lk_usb_scheduler(void);
extern void lk_nand_irq_handler(unsigned int irq);
extern void lk_msdc_irq_handler(unsigned int irq);
#ifdef DUMMY_AP
extern void dummy_ap_irq_handler(unsigned int irq);
#endif

extern BOOT_ARGUMENT    *g_boot_arg;

uint64_t mt_irq_get_affinity(void)
{
	uint64_t mpidr, aff;
	int mp0off;

#ifdef MTK_FORCE_CLUSTER1
	mp0off = 1;
#elif defined(MTK_CAN_FROM_CLUSTER1)
	mp0off = (get_devinfo_with_index(8) & 0x80) ? 1 : 0;
#else
	mp0off = 0;
#endif
	mpidr = (uint64_t) mt_mpidr_read();

	aff = (
//			MPIDR_AFFINITY_LEVEL(mpidr, 3) << 32 |
	          MPIDR_AFFINITY_LEVEL(mpidr, 2) << 16 |
	          MPIDR_AFFINITY_LEVEL(mpidr, 1) << 8  |
	          MPIDR_AFFINITY_LEVEL(mpidr, 0)
	      );

	/*
	 * cluster id + 1 when mp0 off
	 */
	if (mp0off)
		aff += (1 << 8);

	return aff;
}

uint32_t mt_interrupt_needed_for_secure(void)
{
	if (g_boot_arg->boot_mode == DOWNLOAD_BOOT)
		return 1;

	return 0;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	unsigned int irq = mt_irq_get();

	if (irq == MT_GPT_IRQ_ID)
		lk_scheduler();
	else if (irq == MT_SSUSB_IRQ_ID)
		lk_usb_scheduler();
#ifndef MTK_EMMC_SUPPORT
	else if (irq == MT_NFI_IRQ_ID)
		lk_nand_irq_handler(irq);
#endif
	else if (irq == MT_MSDC0_IRQ_ID || irq == MT_MSDC1_IRQ_ID)
		lk_msdc_irq_handler(irq);

#ifdef DUMMY_AP
	dummy_ap_irq_handler(irq);
#endif

	//return INT_NO_RESCHEDULE;
	return INT_RESCHEDULE;
}

void platform_fiq(struct arm_iframe *frame)
{

}
