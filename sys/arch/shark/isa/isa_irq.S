/*	$NetBSD: isa_irq.S,v 1.17 2018/01/30 19:22:28 skrll Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 	from: irq.S
 *
 * Low level irq and fiq handlers
 *
 * Created      : 27/09/94
 */

#include "assym.h"
#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <dev/isa/isareg.h>
#include <shark/isa/icu.h>
#include <machine/irqhandler.h>

	.text
	.align	0

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

.Ldisabled_mask:
	.word	_C_LABEL(disabled_mask)

.Lvam_io_data:
	.word	_C_LABEL(isa_io_bs_tag)

.Lspl_masks:
	.word	_C_LABEL(spl_masks)

/*
 * Register usage
 *
 *  r5  - Pointer to handler pointer list
 *  r6  - Address of current handler
 *  r8  - Current IRQ requests.
 *  r9  - Used to count through possible IRQ bits.
 *  r10 - Base address of IOMD
 */

/* Some documentation is in isa_machdep.c */
ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */
	ENABLE_ALIGNMENT_FAULTS		/* cpuinfo is in r4 after execution */

	/* Load r8 with the ISA 8259 irqs */
	/* r8 <- irq's pending [15:0] */

	/* address of 8259 #1 */
	ldr	r0, .Lvam_io_data
	ldr	r0, [r0]
	ldrb	r8, [r0, #IO_ICU1]                /* ocw3 = irr */

	/* clear the IRR bits that are currently masked. */
	ldr	r2, .Li8259_mask
	ldr	r2, [r2]
	mvn	r2, r2                            /* disabled -> enabled */

	/* address of 8259 #2 */
	tst	r2, #(1 << IRQ_SLAVE)             /* if slave is enabled */
	tstne	r8, #(1 << IRQ_SLAVE)             /* anything from slave? */
	ldrbne	r1, [r0, #IO_ICU2]                /* ocw3 = irr */
	orrne	r8, r8, r1, lsl #8

	and	r8, r8, r2                        /* clear disabled */

	/* clear IRQ 2, which is only used for slave 8259 */
	bic	r8, r8, #(1 << IRQ_SLAVE)

	/*
	 * Note that we have entered the IRQ handler.
	 * We are in SVC mode so we cannot use the processor mode
	 * to determine if we are in an IRQ. Instead we will count the
	 * each time the interrupt handler is nested.
	 */

	ldr	r1, [r4, #CI_INTR_DEPTH]
	add	r1, r1, #1
	str	r1, [r4, #CI_INTR_DEPTH]

	/* Block the current requested interrupts */

	ldr	r1, .Ldisabled_mask
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

	mov	r9, #(NIPL - 1)
	ldr	r5, .Lspl_masks

.Lfind_highest_ipl:
	ldr	r2, [r5, r9, lsl #2]
	tst	r8, r2
	subeq	r9, r9, #1
	beq	.Lfind_highest_ipl

	/* r9 = SPL level of highest priority interrupt */
	add	r9, r9, #1
	ldr	r2, [r5, r9, lsl #2]
	mvn	r2, r2
	orr	r0, r0, r2

	str	r0, [r1]

	ldr	r1, [r4, #CI_CPL]
	str	r9, [r4, #CI_CPL]
	stmfd	sp!, {r1}

	/* Update the IOMD irq masks */
	bl	_C_LABEL(irq_setmasks)

	mrs     r0, cpsr		/* Enable IRQ's */
	bic	r0, r0, #I32_bit
	msr	cpsr_all, r0

	ldr	r5, .Lirqhandlers
	mov	r9, #0x00000001

irqloop:
	/* This would benefit from a special ffs type routine */
	tst	r8, r9			/* Is a bit set ? */
	beq	nextirq			/* No ? try next bit */

	ldr	r6, [r5]		/* Get address of first handler structure */

	teq	r6, #0x00000000		/* Do we have a handler */
	moveq	r0, r8			/* IRQ requests as arg 0 */
	beq	_C_LABEL(stray_irqhandler) /* call special handler */

	ldr	r0, [r4, #(CI_CC_NINTR)]
	ldr	r1, [r4, #(CI_CC_NINTR+4)]
	adds	r0, r0, #0x00000001
	adc	r1, r1, #0x00000000
	str	r0, [r4, #(CI_CC_NINTR)]
	ldr	r1, [r4, #(CI_CC_NINTR+4)]

irqchainloop:
	ldr	r0, [r6, #(IH_ARG)]	/* Get argument pointer */
	teq	r0, #0x00000000		/* If arg is zero pass stack frame */
	addeq	r0, sp, #8		/* ... stack frame */

	mov	lr, pc			/* return address */
	ldr	pc, [r6, #(IH_FUNC)]	/* Call handler */

	teq	r0, #0x00000001		/* Was the irq serviced ? */
	beq	irqdone

	ldr	r6, [r6, #(IH_NEXT)]
	teq	r6, #0x00000000
	bne	irqchainloop
	b	nextirq

irqdone:
	add	r3, r6, #IH_EV_COUNT	/* get address of ih's ev_count */
	ldmia   r3, {r1-r2}		/* load ev_count */
	adds	r1, r1, #0x00000001	/* 64bit incr (lo) */
	adc     r2, r2, #0x00000000	/* 64bit incr (hi) */
	stmia   r3, {r1-r2}		/* store ev_count */

nextirq:
	add	r5, r5, #0x00000004	/* update pointer to handlers */
	mov	r9, r9, lsl #1		/* move on to next bit */
	teq	r9, #(1 << 16)		/* done the last bit ? */
	bne	irqloop			/* no - loop back. */

	ldmfd	sp!, {r2}
	str	r2, [r4, #CI_CPL]

	/* Restore previous disabled mask */
	ldmfd	sp!, {r2}
	ldr	r1, .Ldisabled_mask
	str	r2, [r1]
	bl	_C_LABEL(irq_setmasks)

#ifdef __HAVE_FAST_SOFTINTS
	bl	_C_LABEL(dosoftints)	/* Handle the soft interrupts */
#endif

	/* Kill IRQ's in preparation for exit */
	mrs     r0, cpsr
	orr     r0, r0, #(I32_bit)
	msr     cpsr_all, r0

	/* Decrement the nest count */
	ldr	r1, [r4, #CI_INTR_DEPTH]
	sub	r1, r1, #1
	str	r1, [r4, #CI_INTR_DEPTH]

	LOCK_CAS_CHECK

	DO_AST_AND_RESTORE_ALIGNMENT_FAULTS
	PULLFRAMEFROMSVCANDEXIT

	/* NOT REACHED */
	b	. - 8

.Lcurrent_mask:
	.word	_C_LABEL(current_mask)	/* irq's that are usable */

.Lcpu_info_store:
	.word	_C_LABEL(cpu_info_store)

LOCK_CAS_CHECK_LOCALS

AST_ALIGNMENT_FAULT_LOCALS


ENTRY(irq_setmasks)
	/* Disable interrupts */
	mrs	r3, cpsr
	orr	r1, r3,  #(I32_bit)
	msr	cpsr_all, r1

	/* Calculate interrupt mask */
	ldr	r1, .Lcurrent_mask	/* All the enabled interrupts */
	ldrh	r1, [r1]		/* get hardware bits of mask */
/*	.word	0xe0d110b0 */		/* hand-assembled ldrh r1, [r1] */
	ldr	r0, .Lspl_masks
	ldr	r2, .Lcpu_info_store
	ldr	r2, [r2, #CI_CPL]
	ldr	r2, [r0, r2, lsl #2]
	and	r1, r1, r2
	ldr	r2, .Ldisabled_mask	/* Block due to active interrupts */
	ldr	r2, [r2]
	bic	r1, r1, r2

	/* since 8259's are so slow to access, this code does everything
	   possible to avoid them */

	/* get current mask: these are the bits */
	ldr	r0, .Li8259_mask
	ldr	r2, [r0]
	/*       r2 = 0000.0000.0000.0000.ZZZZ.ZZZZ.ZZZZ.ZZZZ   */

	/* see if there's anything enabled on 8259 #2 */
	tst	r1, #0xff00

	biceq	r1, r1, #(1 << IRQ_SLAVE)  /* no, so disable it */
	orrne	r1, r1, #(1 << IRQ_SLAVE)  /* yes, so enable it */
	/* eq => r1 = 0000.0000.0000.0000.0000.0000.MMMM.M0MM
	   ne => r1 = 0000.0000.0000.0000.MMMM.MMMM.MMMM.M1MM   */

	/* 8259 bit high => disable */
	mvn	r1, r1
	/* eq => r1 = 1111.1111.1111.1111.1111.1111.YYYY.Y1YY
	   ne => r1 = 1111.1111.1111.1111.YYYY.YYYY.YYYY.Y0YY
	   (for each bit position Y = !M)                       */

	orreq	r1, r2, r1, lsl #16
	/* eq => r1 = 1111.1111.YYYY.Y1YY.ZZZZ.ZZZZ.ZZZZ.ZZZZ
	   ne => r1 = 1111.1111.1111.1111.YYYY.YYYY.YYYY.Y0YY   */
	orreq	r1, r1, #0x000000FF
	/* eq => r1 = 1111.1111.YYYY.Y1YY.ZZZZ.ZZZZ.1111.1111
	   ne => r1 = 1111.1111.1111.1111.YYYY.YYYY.YYYY.Y0YY   */
	and	r1, r1, r1, lsr #16
	/* eq => r1 = 0000.0000.0000.0000.ZZZZ.ZZZZ.YYYY.Y1YY
	   ne => r1 = 0000.0000.0000.0000.YYYY.YYYY.YYYY.Y0YY   */

	/* if old = new, don't bother to set again.
	   fast path to exit, since 8259's are so slow anyway */
	eors	r2, r1, r2                /* which bits are different? */
	msreq	cpsr_all, r3              /* no bits are different, return */
	moveq	pc, lr

	/* have to set at least one of the 8259's, store new mask */
	str	r1, [r0]
	ldr	r0, .Lvam_io_data
	ldr	r0, [r0]

	/* see if there's any change for 8259 #1 (master) */
	tst	r2, #0x00FF               /* bottom 8 bits different? */
	strbne	r1, [r0, #(IO_ICU1 + 1)]  /* icu1 / ocw1 */

	/* anything for 8259 #2? */
	tst	r2, #0xFF00
	mov	r1, r1, lsr #8            /* next byte */
	strbne	r1, [r0, #(IO_ICU2 + 1)]  /* icu2 / ocw1 */

	/* Restore old cpsr and exit */
	msr	cpsr_all, r3
	mov	pc, lr

.Li8259_mask:
	.word	_C_LABEL(i8259_mask)

.Lirqhandlers:
	.word	_C_LABEL(irqhandlers)	/* Pointer to array of irqhandlers */
