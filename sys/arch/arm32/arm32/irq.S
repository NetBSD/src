/*	$NetBSD: irq.S,v 1.12.2.1 1997/03/11 21:53:12 is Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * irq.S
 *
 * Low level irq and fiq handlers
 *
 * Created      : 27/09/94
 */

#include "assym.h"
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/iomd.h>

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

/*
 *
 * irq_entry
 *
 * Main entry point for the IRQ vector
 *
 * This function reads the irq request bits in the IOMD registers
 * IRQRQA, IRQRQB and DMARQ
 * It then calls an installed handler for each bit that is set.
 * The function stray_irqhandler is called if a handler is not defined
 * for a particular interrupt.
 * If a interrupt handler is found then it is called with r0 containing
 * the argument defined in the handler structure. If the field ih_arg
 * is zero then a pointer to the IRQ frame on the stack is passed instead.
 */

Ldisabled_mask:
	.word	_disabled_mask

Lcurrent_spl_level:	
	.word	_current_spl_level

Lcurrent_intr_depth:
	.word	_current_intr_depth

	.text
	.global	irq_entry

/*
 * Regsister usage
 *
 *  r6  - Address of current handler
 *  r7  - Pointer to handler pointer list
 *  r8  - Current IRQ requests.
 *  r9  - Used to count through possible IRQ bits.
 *  r10 - Base address of IOMD
 */

irq_entry:
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */

/*        mov     r11, #0x00000000*/	/* Trace back stops here */

	/* Load r8 with the IOMD interrupt requests */

 	mov	r10, #(IOMD_BASE)	/* Point to the IOMD */
	ldrb	r8, [r10, #(IOMD_IRQRQA - IOMD_BASE)]	/* Get IRQ request A */
	ldrb	r9, [r10, #(IOMD_IRQRQB - IOMD_BASE)]	/* Get IRQ request B */
	orr	r8, r8, r9, lsl #8
#ifdef CPU_ARM7500
	ldrb	r9, [r10, #(IOMD_IRQRQC - IOMD_BASE)]	/* Get IRQ request C */
	orr	r8, r8, r9, lsl #16
	ldrb	r9, [r10, #(IOMD_IRQRQD - IOMD_BASE)]	/* Get IRQ request D */
	orr	r8, r8, r9, lsl #24
	ldrb	r9, [r10, #(IOMD_DMARQ - IOMD_BASE)]	/* Get DMA Request */
	tst	r9, #0x10
	orrne	r8, r8, r9, lsl #27
#else
	ldrb	r9, [r10, #(IOMD_DMARQ - IOMD_BASE)]	/* Get DMA Request */
	orr	r8, r8, r9, lsl #16
#endif	/* CPU_ARM7500 */

	and	r0, r8, #0x7d		/* Clear IOMD IRQA bits */
	strb	r0, [r10, #(IOMD_IRQRQA - IOMD_BASE)]

	/*
	 * Note that we have entered the IRQ handler.
	 * We are in SVC mode so we cannot use the processor mode
	 * to determine if we are in an IRQ. Instead we will count the
	 * each time the interrupt handler is nested.
	 */

	ldr	r0, Lcurrent_intr_depth
	ldr	r1, [r0]
	add	r1, r1, #1
	str	r1, [r0]

	/* Block the current requested interrupts */

	ldr	r1, Ldisabled_mask
	ldr	r0, [r1]
	stmfd	sp!, {r0}
	orr	r0, r0, r8
	str	r0, [r1]

	/*
 	 * Need to block all interrupts at the IPL or lower for
	 * all asserted interrupts.
	 * This basically emulates hardware interrupt priority levels.
	 * Means we need to go through the interrupt mask and for
	 * every asserted interrupt we need to mask out all other
	 * interrupts at the same or lower IPL.
	 * If only we could wait until the main loop but we need to sort
	 * this out first so interrupts can be re-enabled.
	 *
	 * This would benefit from a special ffs type routine
	 */

	mov	r9, #0x00000001
	ldr	r7, Lirqblock

emulate_hwipl_loop:
	tst	r8, r9
	ldrne	r6, [r7]
	orrne	r0, r0, r6

	add	r7, r7, #4
	mov	r9, r9, lsl #1		/* move on to next bit */
#ifdef CPU_ARM7500
	teq	r9, #0			/* done the last bit ? */
#else
	teq	r9, #(1 << 24)		/* done the last bit ? */
#endif	/* CPU_ARM7500 */
	bne	emulate_hwipl_loop	/* no - loop back. */

	str	r0, [r1]	

	/* Update the IOMD irq masks */

	bl	_irq_setmasks

        mrs     r0, cpsr_all		/* Enable IRQ's */
	bic	r0, r0, #I32_bit
	msr	cpsr_all, r0

	ldr	r7, [pc, #irqhandlers - . - 8]
        mov	r9, #0x00000001

	stmfd	sp!, {r8}

irqloop:
	/* This would benefit from a special ffs type routine */
	tst	r8, r9			/* Is a bit set ? */
	beq	nextirq			/* No ? try next bit */

	ldr	r6, [r7]		/* Get address of first handler structure */

	teq	r6, #0x00000000		/* Do we have a handler */
	moveq	r0, r8			/* IRQ requests as arg 0 */
	beq	_stray_irqhandler	/* call special handler */

        ldr	r0, Lcnt
	ldr	r1, [r0, #(V_INTR)]
	add	r1, r1, #0x00000001
	str	r1, [r0, #(V_INTR)]

irqchainloop:
	add	lr, pc, #nextinchain - . - 8	/* return address */

/*
 * XXX: Should stats be accumlated for every interrupt routine called
 * or for every physical interrupt that is serviced.
 */

#ifdef IRQSTATS
        ldr	r0, Lintrcnt
	ldr	r1, [r6, #(IH_NUM)]

	add	r0, r0, r1, lsl #2
	ldr	r1, [r0]
	add	r1, r1, #0x00000001
	str	r1, [r0]
#endif	/* IRQSTATS */

	ldr	r0, [r6, #(IH_ARG)]	/* Get argument pointer */
	teq	r0, #0x00000000		/* If arg is zero pass stack frame */
	addeq	r0, sp, #8		/* ... stack frame */
	ldr	pc, [r6, #(IH_FUNC)]	/* Call handler */

nextinchain:
	teq	r0, #0x00000001		/* Was the irq serviced ? */
	beq	irqdone

	ldr	r6, [r6, #(IH_NEXT)]
	teq	r6, #0x00000000
	bne	irqchainloop

irqdone:
nextirq:
	add	r7, r7, #0x00000004	/* update pointer to handlers */
	mov	r9, r9, lsl #1		/* move on to next bit */
#ifdef CPU_ARM7500
	teq	r9, #0			/* done the last bit ? */
#else
	teq	r9, #(1 << 24)		/* done the last bit ? */
#endif	/* CPU_ARM7500 */
	bne	irqloop			/* no - loop back. */

	ldmfd	sp!, {r8}

	/* Restore previous disabled mask */
	ldmfd	sp!, {r2}
	ldr	r1, Ldisabled_mask
	str	r2, [r1]
	bl	_irq_setmasks

	bl	_dosoftints		/* Handle the soft interrupts */

	/* Manage AST's. Maybe this should be done as a soft interrupt ? */

	ldr	r0, [sp]		/* Get the SPSR from stack */

	and	r0, r0, #(PSR_MODE)	/* Test for USR32 mode before the IRQ */
	teq	r0, #(PSR_USR32_MODE)
	ldreq	r0, Lastpending		/* Do we have an AST pending ? */
	ldreq	r1, [r0]
	teqeq	r1, #0x00000001

	beq	irqast			/* call the AST handler */

	/* Kill IRQ's in preparation for exit */

        mrs     r0, cpsr_all
        orr     r0, r0, #(I32_bit)
        msr     cpsr_all, r0

	/* Decrement the nest count */

	ldr	r0, Lcurrent_intr_depth
	ldr	r1, [r0]
	sub	r1, r1, #1
	str	r1, [r0]

	PULLFRAMEFROMSVCANDEXIT

	movs	pc, lr			/* Exit */

	/*
	 * Ok, snag with current intr depth ...
	 * If ast() calls mi_sleep() the current_intr_depth will not be
	 * decremented until the process is woken up. This can result
	 * in the system believing it is still in the interrupt handler.
	 * If we are calling ast() then correct the current_intr_depth
	 * before the call.
	 */
irqast:
	mov	r1, #0x00000000		/* Clear ast_pending */
	str	r1, [r0]

	/* Kill IRQ's so we atomically decrement current_intr_depth */

        mrs     r2, cpsr_all
        orr     r3, r2, #(I32_bit)
        msr     cpsr_all, r3

	/* Decrement the nest count */

	ldr	r0, Lcurrent_intr_depth
	ldr	r1, [r0]
	sub	r1, r1, #1
	str	r1, [r0]

	/* Restore IRQ's */
        msr     cpsr_all, r2

	mov	r0, sp
	bl	_ast

/* Kill IRQ's in preparation for exit */

        mrs     r0, cpsr_all
        orr     r0, r0, #(I32_bit)
        msr     cpsr_all, r0

	PULLFRAMEFROMSVCANDEXIT

	movs	pc, lr			/* Exit */
	

Lspl_mask:
	.word	_spl_mask		/* irq's allowed at current spl level */

Lcurrent_mask:
	.word	_current_mask		/* irq's that are usable */

Lirqblock:
	.word	_irqblock

	.global	_irq_setmasks

_irq_setmasks:
	/* Disable interrupts */
	mrs	r3, cpsr_all
	orr	r1, r3,  #(I32_bit)
	msr	cpsr_all, r1

	/* Calculate IOMD interrupt mask */
	ldr	r1, Lcurrent_mask	/* All the enabled interrupts */
	ldr	r1, [r1]
	ldr	r2, Lspl_mask		/* Block due to current spl level */
	ldr	r2, [r2]
	and	r1, r1, r2
	ldr	r2, Ldisabled_mask	/* Block due to active interrupts */
	ldr	r2, [r2]
	bic	r1, r1, r2

 	mov	r0, #(IOMD_BASE)	/* Point to the IOMD */
	strb	r1, [r0, #(IOMD_IRQMSKA - IOMD_BASE)]	/* Set IRQ mask A */
	mov	r1, r1, lsr #8
	strb	r1, [r0, #(IOMD_IRQMSKB - IOMD_BASE)]	/* Set IRQ mask B */
	mov	r1, r1, lsr #8
#ifdef CPU_ARM7500
	strb	r1, [r0, #(IOMD_IRQMSKC - IOMD_BASE)]
	mov	r1, r1, lsr #8
	and	r2, r1, #0xef
	strb	r2, [r0, #(IOMD_IRQMSKD - IOMD_BASE)]
	mov	r1, r1, lsr #3
	and	r2, r1, #0x10
	strb	r2, [r0, #(IOMD_DMAMSK - IOMD_BASE)]	/* Set DMA mask */
#else
	strb	r1, [r0, #(IOMD_DMAMSK - IOMD_BASE)]	/* Set DMA mask */
#endif	/* CPU_ARM7500 */

	/* Restore old cpsr and exit */
	msr	cpsr_all, r3
	mov	pc, lr


Lcnt:
	.word	_cnt

Lintrcnt:
	.word	_intrcnt


irqhandlers:
	.word	_irqhandlers		/* Pointer to array of irqhandlers */

Lastpending:
	.word	_astpending

#ifdef IRQSTATS
/* These symbols are used by vmstat */

	.text
	.global	__intrnames
__intrnames:
	.word	_intrnames

	.data

        .globl  _intrnames, _eintrnames, _intrcnt, _eintrcnt
_intrnames:
	.asciz	"interrupt  0 "
	.asciz	"softnet      "	/* reserved0 */
	.asciz	"interrupt  2 "
	.asciz	"interrupt  3 "
	.asciz	"interrupt  4 "
	.asciz	"interrupt  5 "
	.asciz	"interrupt  6 "
	.asciz	"softclock    "	/* reserved1 */
	.asciz	"softplip     " /* reserved2 */
	.asciz	"interrupt  9 "
	.asciz	"interrupt 10 "
	.asciz	"interrupt 11 "
	.asciz	"interrupt 12 "
	.asciz	"interrupt 13 "
	.asciz	"interrupt 14 "
	.asciz	"interrupt 15 "
	.asciz	"dma channel 0"
	.asciz	"dma channel 1"
	.asciz	"dma channel 2"
	.asciz	"dma channel 3"
	.asciz	"interrupt 20 "
	.asciz	"interrupt 21 "
	.asciz	"reserved 3   "
	.asciz	"reserved 4   "
	.asciz	"exp card 0   "
	.asciz	"exp card 1   "
	.asciz	"exp card 2   "
	.asciz	"exp card 3   "
	.asciz	"exp card 4   "
	.asciz	"exp card 5   "
	.asciz	"exp card 6   "
	.asciz	"exp card 7   "
_eintrnames:

	.bss
	.align	0
_intrcnt:
	.space	32*4	/* XXX Should be linked to number of interrupts */
_eintrcnt:

#else	/* IRQSTATS */
/* Dummy entries to keep vmstat happy */

	.text
        .globl  _intrnames, _eintrnames, _intrcnt, _eintrcnt
_intrnames:
	.long	0
_eintrnames:

_intrcnt:
	.long	0
_eintrcnt:
#endif	/* IRQSTATS */

/* FIQ code */

	.text
	.align	0
	.global	_fiq_setregs		/* Sets up the FIQ handler */

_fiq_setregs:
        mrs	r2, cpsr_all
	mov	r3, r2
	bic	r2, r2, #(PSR_MODE)
	orr	r2, r2, #(PSR_FIQ32_MODE)
        msr	cpsr_all, r2

	ldr	r8,  [r0, #FH_R8]	/* Update FIQ registers*/
	ldr	r9,  [r0, #FH_R9]
	ldr	r10, [r0, #FH_R10]
	ldr	r11, [r0, #FH_R11]
	ldr	r12, [r0, #FH_R12]
	ldr	r13, [r0, #FH_R13]

        msr	cpsr_all, r3		/* Back to old mode */

	mov	pc, lr			/* Exit */

	.global	_fiq_getregs		/* Gets the FIQ registers */

_fiq_getregs:
        mrs	r2, cpsr_all
	mov	r3, r2
	bic	r2, r2, #(PSR_MODE)
	orr	r2, r2, #(PSR_FIQ32_MODE)
        msr	cpsr_all, r2

	str	r8,  [r0, #FH_R8]	/* Update FIQ registers*/
	str	r9,  [r0, #FH_R9]
	str	r10, [r0, #FH_R10]
	str	r11, [r0, #FH_R11]
	str	r12, [r0, #FH_R12]
	str	r13, [r0, #FH_R13]

        msr	cpsr_all, r3		/* Back to old mode */

	mov	pc, lr			/* Exit */

/* End of irq.S */
