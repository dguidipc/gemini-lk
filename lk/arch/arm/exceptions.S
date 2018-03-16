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
#include <asm.h>

FUNCTION(arm_undefined)
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }^
	b		arm_undefined_handler
	b		.

FUNCTION(arm_syscall)
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }^
	b		arm_syscall_handler
	b		.

FUNCTION(arm_prefetch_abort)
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }^
	b		arm_prefetch_abort_handler
	b		.

FUNCTION(arm_data_abort)
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }^
	b		arm_data_abort_handler
	b		.

FUNCTION(arm_reserved)
	b	.

FUNCTION(arm_irq)
	/* XXX only deals with interrupting supervisor mode */

	/* save r4-r6 and use as a temporary place to save while we switch into supervisor mode */
	stmia	r13, { r4-r6 }
	mov		r4, r13
	sub		r5, lr, #4
	mrs		r6, spsr

	/* move into supervisor mode. irq/fiq disabled */
	msr	cpsr_c, #(3<<6 | 0x13)

	/* save the return address */
	stmfd	sp!, { r5 }

	/* save C trashed regs, supervisor lr */
	stmfd	sp!, { r0-r3, r12, lr }

	/* save spsr */
	stmfd	sp!, { r6 }

	/* restore r4-r6 */
	ldmia	r4, { r4-r6 }

	/* increment the global critical section count */
	ldr     r1, =critical_section_count
	ldr     r0, [r1]
	add     r0, r0, #1
	str     r0, [r1]

	/* call into higher level code */
	mov	r0, sp /* iframe */
	bl	platform_irq

	/* reschedule if the handler returns nonzero */
	cmp     r0, #0
	blne    thread_preempt

	/* decrement the global critical section count */
	ldr     r1, =critical_section_count
	ldr     r0, [r1]
	sub     r0, r0, #1
	str     r0, [r1]

	/* restore spsr */
	ldmfd	sp!, { r0 }
	msr     spsr_cxsf, r0

	/* restore back to where we came from */
	ldmfd	sp!, { r0-r3, r12, lr, pc }^

.bss
.align 2
	.global irq_save_spot
irq_save_spot:
	.word	0	/* r4 */
	.word	0	/* r5 */
	.word	0	/* r6 */

.text
FUNCTION(arm_fiq)
	sub	lr, lr, #4
	stmfd	sp!, { r0-r3, r12, lr }

	bl	platform_fiq

	ldmfd	sp!, { r0-r3, r12, pc }^

.ltorg



.text
FUNCTION(jumparch64)
    MOV r4, r1   /* r4 argument */
    MOV r5, r2   /* r5 argument */
    MOV r6, r0   /* keep Kernel jump addr */

    /* setup the reset vector base address after warm reset to Aarch64 */
    /* ldr r1,=RVBADDRESS_CPU0 */
    /* ldr r1,[r1] */
    /* LDR r0, =0x4000C000 */
    //LDR r0, =0x00001000
    LDR r0, =0x40000000
    LDR r1, =0x10200038
    str r0,[r1]

    /* setup the excution state after warm reset: 1:Aarch64, 0:Aarch32 */
    MRC p15,0,r0,c12,c0,2
    orr r0, r0, #1
    MCR p15,0,r0,c12,c0,2
    DSB
    ISB

    /* do warm reset:reset request */
    MRC p15,0,r0,c12,c0,2
    orr r0, r0, #2
    MCR p15,0,r0,c12,c0,2
    DSB
    ISB

    /* set r0 as 0xC000_0000 for ATF OP code check */
    MOV r0, r4
    MOV r1, r5
    MOV r2, r0

.globl WFI_LOOP
WFI_LOOP:
    /* enter WFI to request a warm reset */
    WFI
    B WFI_LOOP


.text
FUNCTION(jumparch64_smc)
        mov r4, r3
	mov r3, r2
	mov r2, r1
	mov r1, r0
	ldr r0, =0x82000200  /*r0: ATF Function ID, please check MTK_SIP_KERNEL_BOOT_AARCH32 in  vendor/arm/atf/include/bl31/services/sip_svc.h*/ 
	smc #0
	b   .

.text
FUNCTION(dapc_smc)
	mov r3, r2
	mov r2, r1
	mov r1, r0
	ldr r0, =0x82000101  /*please check MTK_SIP_LK_DAPC_INIT_AARCH32 in  vendor/arm/atf-1.0/include/bl31/services/sip_svc.h*/ 
	smc #0
	bx lr

.text
FUNCTION(md_reg_write_smc)
	mov r4, r3
	mov r3, r2
	mov r2, r1
	mov r1, r0
	ldr r0, =0x82000102  /*please check MTK_SIP_LK_LTE_REG_WRITE_TH_AARCH32 in  vendor/arm/atf-1.0/include/bl31/services/sip_svc.h*/ 
	smc #0
	bx lr

.text
FUNCTION(rpmb_init)
    ldr r0, =0x82000103  /*please check MTK_SIP_LK_RPMB_INIT_AARCH32 in vendor/arm/atf-1.0/include/bl31/services/sip_svc.h*/ 
    smc #0
    bx lr

.text
FUNCTION(rpmb_uninit)
    ldr r0, =0x82000104  /*please check MTK_SIP_LK_RPMB_UNINIT_AARCH32 in vendor/arm/atf-1.0/include/bl31/services/sip_svc.h*/ 
    smc #0
    bx lr

.text
FUNCTION(rpmb_hmac)
    mov r2, r1
    mov r1, r0
    ldr r0, =0x82000105  /*please check MTK_SIP_LK_RPMB_HMAC_AARCH32 in  vendor/arm/atf-1.0/include/bl31/services/sip_svc.h*/ 
    smc #0
    bx lr

.text
FUNCTION(mt_secure_call)
        smc #0
        bx lr

