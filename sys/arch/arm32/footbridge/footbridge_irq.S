/*	$NetBSD: footbridge_irq.S,v 1.2 1998/12/28 09:39:02 mark Exp $	*/

/*
 * Copyright (c) 1998 Mark Brinicombe.
 * Copyright (c) 1998 Causality Limited
 * All rights reserved.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_irqstats.h"
#include "opt_uvm.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm32/footbridge/dc21285mem.h>
#include <arm32/footbridge/dc21285reg.h>

	.text
	.align	0

/*
 *
 * irq_entry
 *
 * Main entry point for the IRQ vector
 *
 * This function reads the IRQ request register in the 21285
 * and then calls the installed handlers for each bit that is set.
 * The function stray_irqhandler is called if a handler is not defined
 * for a particular interrupt.
 * If a interrupt handler is found then it is called with r0 containing
 * the argument defined in the handler structure. If the field ih_arg
 * is zero then a pointer to the IRQ frame on the stack is passed instead.
 */

Lintr_disabled_mask:
	.word	_intr_disabled_mask

Lintr_claimed_mask:
	.word	_intr_claimed_mask

Lcurrent_spl_level:	
	.word	_current_spl_level

Lcurrent_intr_depth:
	.word	_current_intr_depth

Lspl_masks:
	.word	_spl_masks

/*
 * Regsister usage
 *
 *  r6  - Address of current handler
 *  r7  - Pointer to handler pointer list
 *  r8  - Current IRQ requests.
 *  r9  - Used to count through possible IRQ bits.
 *  r10 - Base address of IOMD
 */

ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */

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

	/* Load r8 with the Footbridge interrupt requests */
	mov	r10, #(DC21285_ARMCSR_VBASE)
	ldr	r8, [r10, #(IRQ_STATUS)]

	/* This condition needs further examination */
	teq	r8, #0
	beq	irq_unknown

	/* Block the current requested interrupts */
	ldr	r1, Lintr_disabled_mask
	ldr	r0, [r1]
	stmfd	sp!, {r0}
	orr	r0, r0, r8

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
	mov	r9, #(_SPL_LEVELS - 1)
	ldr	r7, Lspl_masks

Lfind_highest_ipl:
	ldr	r2, [r7, r9, lsl #2]
	tst	r8, r2
	subeq	r9, r9, #1
	beq	Lfind_highest_ipl

	/* r9 = SPL level of highest priority interrupt */
	add	r9, r9, #1
	ldr	r2, [r7, r9, lsl #2]
	mvn	r2, r2
	orr	r0, r0, r2

	str	r0, [r1]		/* store new disabled mask */

	ldr	r2, Lcurrent_spl_level
	ldr	r1, [r2]
	str	r9, [r2]
	stmfd	sp!, {r1}

	ldr	r7, Lintr_claimed_mask	/* get claimed mask */
	ldr	r6, [r7]
	bic	r6, r6, r0		/* mask out disabled */
	ldr	r7, Lintr_current_mask	/* get claimed mask */
	str	r6, [r7]		/* new current mask */	

	/* Update the DC21285 irq masks */
	bl	_irq_setmasks_nointr

        mrs     r0, cpsr_all		/* Enable IRQ's */
	bic	r0, r0, #I32_bit
	msr	cpsr_all, r0

	ldr	r7, [pc, #irqhandlers - . - 8]
        mov	r9, #0x00000001

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
/*	teq	r0, #0x00000001*/		/* Was the irq serviced ? */
/*	beq	irqdone*/

	ldr	r6, [r6, #(IH_NEXT)]
	teq	r6, #0x00000000
	bne	irqchainloop

/*irqdone:*/
nextirq:
	add	r7, r7, #0x00000004	/* update pointer to handlers */
	mov	r9, r9, lsl #1		/* move on to next bit */
	teq	r9, #0			/* done the last bit ? */
	bne	irqloop			/* no - loop back. */

	ldmfd	sp!, {r2}
	ldr	r1, Lcurrent_spl_level
	str	r2, [r1]

	/* Restore previous disabled mask */
	ldmfd	sp!, {r2}
	ldr	r1, Lintr_disabled_mask
	str	r2, [r1]

	ldr	r1, Lintr_claimed_mask	/* get claimed mask */
	ldr	r0, [r1]
	bic	r0, r0, r2		/* mask out disabled */
	ldr	r1, Lintr_current_mask	/* get claimed mask */
	str	r0, [r1]		/* new current mask */	
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

irq_unknown:
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
	
Lcnt:
#if defined (UVM)
	.word	_uvmexp
#else
	.word	_cnt
#endif

Lintrcnt:
	.word	_intrcnt

irqhandlers:
	.word	_irqhandlers		/* Pointer to array of irqhandlers */

Lastpending:
	.word	_astpending

Lspl_mask:
	.word	_spl_mask	/* irq's allowed at current spl level */

Lintr_current_mask:
	.word	_intr_current_mask

ENTRY(irq_setmasks)
	/* Disable interrupts */
	mrs	r3, cpsr_all
	orr	r1, r3,  #(I32_bit)
	msr	cpsr_all, r1

	/* Calculate DC21285 interrupt mask */
	ldr	r1, Lintr_current_mask	/* All the enabled interrupts */
	ldr	r1, [r1]
	ldr	r2, Lspl_mask		/* Block due to current spl level */
	ldr	r2, [r2]
	and	r1, r1, r2

	mov	r0, #(DC21285_ARMCSR_VBASE)
	str	r1, [r0, #(IRQ_ENABLE_SET)]
	mvn	r1, r1
	str	r1, [r0, #(IRQ_ENABLE_CLEAR)]

	/* Restore old cpsr and exit */
	msr	cpsr_all, r3
	mov	pc, lr

ENTRY(irq_setmasks_nointr)
	/* Calculate DC21285 interrupt mask */
	ldr	r1, Lintr_current_mask	/* All the enabled interrupts */
	ldr	r1, [r1]
	ldr	r2, Lspl_mask		/* Block due to current spl level */
	ldr	r2, [r2]
	and	r1, r1, r2

	mov	r0, #(DC21285_ARMCSR_VBASE)
	str	r1, [r0, #(IRQ_ENABLE_SET)]
	mvn	r1, r1
	str	r1, [r0, #(IRQ_ENABLE_CLEAR)]

	mov	pc, lr

#ifdef IRQSTATS
	/* These symbols are used by vmstat */

	.text
	.global	__intrnames
__intrnames:
	.word	_intrnames

	.data
	.align	0
        .global	_intrnames, _sintrnames, _eintrnames
	.global	_intrcnt, _sintrcnt, _eintrcnt
_intrnames:
	.asciz	"interrupt  0 "
	.asciz	"interrupt  1 "
	.asciz	"interrupt  2 "
	.asciz	"interrupt  3 "
	.asciz	"interrupt  4 "
	.asciz	"interrupt  5 "
	.asciz	"interrupt  6 "
	.asciz	"interrupt  7 "
	.asciz	"interrupt  8 "
	.asciz	"interrupt  9 "
	.asciz	"interrupt 10 "
	.asciz	"interrupt 11 "
	.asciz	"interrupt 12 "
	.asciz	"interrupt 13 "
	.asciz	"interrupt 14 "
	.asciz	"interrupt 15 "
	.asciz	"interrupt 16 "
	.asciz	"interrupt 17 "
	.asciz	"interrupt 18 "
	.asciz	"interrupt 19 "
	.asciz	"interrupt 20 "
	.asciz	"interrupt 21 "
	.asciz	"interrupt 22 "
	.asciz	"interrupt 23 "
	.asciz	"interrupt 24 "
	.asciz	"interrupt 25 "
	.asciz	"interrupt 26 "
	.asciz	"interrupt 27 "
	.asciz	"interrupt 28 "
	.asciz	"interrupt 29 "
	.asciz	"interrupt 30 "
	.asciz	"interrupt 31 "
_sintrnames:
	.asciz	"soft int  0  "
	.asciz	"soft int  1  "
	.asciz	"soft int  2  "
	.asciz	"soft int  3  "
	.asciz	"soft int  4  "
	.asciz	"soft int  5  "
	.asciz	"soft int  6  "
	.asciz	"soft int  7  "
	.asciz	"soft int  8  "
	.asciz	"soft int  9  "
	.asciz	"soft int 10  "
	.asciz	"soft int 11  "
	.asciz	"soft int 12  "
	.asciz	"soft int 13  "
	.asciz	"soft int 14  "
	.asciz	"soft int 15  "
	.asciz	"soft int 16  "
	.asciz	"soft int 17  "
	.asciz	"soft int 18  "
	.asciz	"soft int 19  "
	.asciz	"soft int 20  "
	.asciz	"soft int 21  "
	.asciz	"soft int 22  "
	.asciz	"soft int 23  "
	.asciz	"soft int 24  "
	.asciz	"soft int 25  "
	.asciz	"soft int 26  "
	.asciz	"soft int 27  "
	.asciz	"soft int 28  "
	.asciz	"soft int 29  "
	.asciz	"soft int 30  "
	.asciz	"soft int 31  "
_eintrnames:

	.bss
	.align	0
	.global	_intrcnt, _sintrcnt, _eintrcnt
_intrcnt:
	.space	32*4	/* XXX Should be linked to number of interrupts */
_sintrcnt:
	.space	32*4	/* XXX Should be linked to number of soft ints */
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

/* End of footbridge_irq.S */
