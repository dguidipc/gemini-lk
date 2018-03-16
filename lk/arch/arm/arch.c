/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <platform.h>
#ifdef MTK_FORCE_CLUSTER1
#include <platform/boot_mode.h>
#endif

extern void isb(void);
extern void dsb(void);

#if ARM_ISA_ARMV7
static void set_vector_base(addr_t addr)
{
	__asm__ volatile("mcr	p15, 0, %0, c12, c0, 0" :: "r" (addr));
}
#endif

void arch_early_init(void)
{
	/* turn off the cache */
	arch_disable_cache(UCACHE);
#ifdef ENABLE_L2_SHARING
	config_L2_size();
#endif

#ifdef MTK_FORCE_CLUSTER1
	if (mt_get_chip_sw_ver() < CHIP_SW_VER_02) {
		/* set L2 data latency to 1 */
		uint32_t tmp;
		__asm__ volatile("mrc p15, 1, %0, c9, c0, 2" : "=r" (tmp));
		tmp &= 0xfffc0000;
		tmp |= 0x9251;
		isb();
		dsb();
		__asm__ volatile("mcr p15, 1, %0, c9, c0, 2" :: "r" (tmp));
		dsb();
		isb();
	}
#endif

	/* set the vector base to our exception vectors so we dont need to double map at 0 */
#if ARM_ISA_ARMV7
	set_vector_base(MEMBASE);
#endif

#if ARM_WITH_MMU
#ifdef MTK_3LEVEL_PAGETABLE
	platform_init_mmu();
#else	//!MTK_3LEVEL_PAGETABLE
#ifndef MTK_LM_MODE
	platform_init_mmu_mappings();
#else
	platform_init_mmu();
#endif
#endif	//MTK_3LEVEL_PAGETABLE
#endif

	/* turn the cache back on */
	arch_enable_cache(UCACHE);

#ifdef HAVE_CACHE_PL310
	arch_enable_l2cache();
#endif

#if ARM_WITH_NEON
	/* enable cp10 and cp11 */
	uint32_t val;
	__asm__ volatile("mrc	p15, 0, %0, c1, c0, 2" : "=r" (val));
	val |= (3<<22)|(3<<20);
	__asm__ volatile("mcr	p15, 0, %0, c1, c0, 2" :: "r" (val));

	/* set enable bit in fpexc */
	__asm__ volatile("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
	val |= (1<<30);
	__asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));
#endif

#if 0
#if ARM_ISA_ARMV7
	/* enable the cycle count register */
	uint32_t en;
	__asm__ volatile("mrc	p15, 0, %0, c9, c12, 0" : "=r" (en));
	en &= ~(1<<3); /* cycle count every cycle */
	en |= 1; /* enable all performance counters */
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 0" :: "r" (en));

	/* enable cycle counter */
	en = (1<<31);
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 1" :: "r" (en));
#endif
#endif
}

void arch_init(void)
{
}

void arch_uninit(void)
{
#if ARM_WITH_NEON
	uint32_t val;
	/* set disable bit in fpexc */
	__asm__ volatile("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
	val &= ~(1<<30);
	__asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));

	/* disable cp10 and cp11 */
	__asm__ volatile("mrc	p15, 0, %0, c1, c0, 2" : "=r" (val));
	val &= ~((3<<22)|(3<<20));
	__asm__ volatile("mcr	p15, 0, %0, c1, c0, 2" :: "r" (val));
#endif

#if ARM_WITH_CP15
	/* disable alignment faults */
	arm_write_cr1(arm_read_cr1() & ~(0x1<<1));
	dsb();
#endif
}

