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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform/boot_mode.h>
#include <mt_gic.h>
#include <arch/arm/mmu.h>

#define MTK_SIP_LK_WDT_AARCH32 0x82000106
#define TIMER_DUMP_REG         0x10008004

enum {
	REG_LR = 18,
	REG_SP = 19,
};

struct atf_aee_regs {
	unsigned long long regs[31];
	unsigned long long sp;
	unsigned long long pc;
	unsigned long long pstate;
};

extern unsigned long mt_secure_call(unsigned long, unsigned long, unsigned long, unsigned long);

static void *atf_aee_debug_addr = NULL;

static void lk_wdt_dump_regs(struct atf_aee_regs *regs)
{
	dprintf(CRITICAL, "Dump register from ATF..\n");
	dprintf(CRITICAL, "CPSR: 0x%08lx\n", (unsigned long) regs->pstate);
	dprintf(CRITICAL, "PC: 0x%08lx\n", (unsigned long) regs->pc);
	dprintf(CRITICAL, "SP: 0x%08lx\n", (unsigned long) regs->regs[REG_SP]);
	dprintf(CRITICAL, "LR: 0x%08lx\n", (unsigned long) regs->regs[REG_LR]);
}

static void lk_wdt_dump(void)
{
	unsigned int reg;

	thread_t *thread = current_thread;

	dprintf(CRITICAL, "%s(): watchdog timeout in LK....\n", __func__);

	dprintf(CRITICAL, "current_thread = %s\n", thread->name);

	if (atf_aee_debug_addr) {
		lk_wdt_dump_regs((struct atf_aee_regs *) atf_aee_debug_addr);
	}

	mt_irq_register_dump();

	reg = DRV_Reg32(TIMER_DUMP_REG);
	dprintf(CRITICAL, "timer_dump_reg: 0x%08x\n", reg);

	dprintf(CRITICAL, "%s(): finished...\n", __func__);

	/* try to flush all cache */
	platform_uninit();
#ifdef HAVE_CACHE_PL310
	l2_disable();
#endif

	arch_disable_cache(UCACHE);
	arch_disable_mmu();

	/* never return */
	platform_halt();
}

void lk_register_wdt_callback(void)
{
	if (g_boot_arg->boot_mode != DOWNLOAD_BOOT) {
		uint32_t start, size;
		atf_aee_debug_addr = (void *) mt_secure_call(MTK_SIP_LK_WDT_AARCH32, (unsigned long) &lk_wdt_dump, 0, 0);
		start = ROUNDDOWN((uint32_t)atf_aee_debug_addr, PAGE_SIZE);
		size = sizeof(struct atf_aee_regs);
		size = ROUNDUP(((uint32_t)atf_aee_debug_addr - start + size), PAGE_SIZE);
		arch_mmu_map((uint64_t)start, (uint32_t)start,
		             MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(size, PAGE_SIZE));
	}
}
