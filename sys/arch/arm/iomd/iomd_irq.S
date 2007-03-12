/*	$NetBSD: iomd_irq.S,v 1.5.26.1 2007/03/12 05:47:06 rmind Exp $	*/

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
 * Low level irq and fiq handlers
 *
 * Created      : 27/09/94
 */

#include "opt_irqstats.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/iomd/iomdreg.h>

	.text
	.align	0
/*
 * ffs table used for servicing irq's quickly must be here otherwise adr can't
 * reach it
 * The algorithm for ffs was devised by D. Seal and posted to
 * comp.sys.arm on 16 Feb 1994.
 */
.type Lirq_ffs_table, _ASM_TYPE_OBJECT;
Lirq_ffs_table:
/* same as ffs table but all nums are -1 from that */
/*               0   1   2   3   4   5   6   7           */
	.byte	 0,  0,  1, 12,  2,  6,  0, 13  /*  0- 7 */
	.byte	 3,  0,  7,  0,  0,  0,  0, 14  /*  8-15 */
	.byte	10,  4,  0,  0,  8,  0,  0, 25  /* 16-23 */
	.byte	 0,  0,  0,  0,  0, 21, 27, 15  /* 24-31 */
	.byte	31, 11,  5,  0,  0,  0,  0,  0	/* 32-39 */
	.byte	 9,  0,  0, 24,  0,  0, 20, 26  /* 40-47 */
	.byte	30,  0,  0,  0,  0, 23,  0, 19  /* 48-55 */
	.byte   29,  0, 22, 18, 28, 17, 16,  0  /* 56-63 */

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
	.word	_C_LABEL(disabled_mask)

Lcurrent_spl_level:	
	.word	_C_LABEL(current_spl_level)

Lcurrent_intr_depth:
	.word	_C_LABEL(current_intr_depth)

Lspl_masks:
	.word	_C_LABEL(spl_masks)

LOCK_CAS_CHECK_LOCALS

AST_ALIGNMENT_FAULT_LOCALS

/*
 * Register usage
 *
 *  r5  - Address of ffs table
 *  r6  - Address of current handler
 *  r7  - Pointer to handler pointer list
 *  r8  - Current IRQ requests.
 *  r10 - Base address of IOMD
 *  r11 - IRQ requests still to service.
 */

Liomd_base:
	.word	_C_LABEL(iomd_base)

Larm7500_ioc_found:
	.word	_C_LABEL(arm7500_ioc_found)

ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */
	ENABLE_ALIGNMENT_FAULTS

	/* Load r8 with the IOMD interrupt requests */

	ldr	r10, Liomd_base
 	ldr	r10, [r10]			/* Point to the IOMD */
	ldrb	r8, [r10, #(IOMD_IRQRQA << 2)]	/* Get IRQ request A */
	ldrb	r9, [r10, #(IOMD_IRQRQB << 2)]	/* Get IRQ request B */
	orr	r8, r8, r9, lsl #8

	ldr	r9, Larm7500_ioc_found
	ldr	r9, [r9]			/* get the flag      */
	cmp	r9, #0
	beq	skip_extended_IRQs_reading

	/* ARM 7500 only */
	ldrb	r9, [r10, #(IOMD_IRQRQC << 2)]	/* Get IRQ request C */
	orr	r8, r8, r9, lsl #16
	ldrb	r9, [r10, #(IOMD_IRQRQD << 2)]	/* Get IRQ request D */
	orr	r8, r8, r9, lsl #24
	ldrb	r9, [r10, #(IOMD_DMARQ << 2)]	/* Get DMA Request */
	tst	r9, #0x10
	orrne	r8, r8, r9, lsl #27
	b	irq_entry_continue

skip_extended_IRQs_reading:
	/* non ARM7500 machines */
	ldrb	r9, [r10, #(IOMD_DMARQ << 2)]	/* Get DMA Request */
	orr	r8, r8, r9, lsl #16
irq_entry_continue:

	and	r0, r8, #0x7d		/* Clear IOMD IRQA bits */
	strb	r0, [r10, #(IOMD_IRQRQA << 2)]

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

	str	r0, [r1]	

	ldr	r0, Lcurrent_spl_level
	ldr	r1, [r0]
	str	r9, [r0]
	stmfd	sp!, {r1}

	/* Update the IOMD irq masks */
	bl	_C_LABEL(irq_setmasks)

        mrs     r0, cpsr_all		/* Enable IRQ's */
	bic	r0, r0, #I32_bit
	msr	cpsr_all, r0

	ldr	r7, Lirqhandlers

	/* 
	 * take a copy of the IRQ request so that we can strip bits out of it
	 * note that we only use 24 bits with iomd2 chips
	 */
	ldr	r4, Larm7500_ioc_found
	ldr	r4, [r4]			/* get the flag      */
	cmp	r4, #0
	movne	r11, r8				/* ARM7500  -> copy all bits   */
	biceq	r11, r8, #0xff000000		/* !ARM7500 -> only use 24 bit */

	/* ffs routine to find first irq to service */
	/* standard trick to isolate bottom bit in a0 or 0 if a0 = 0 on entry */
	rsb	r4, r11, #0
	ands	r10, r11, r4

	/* 
	 * now r10 has at most 1 set bit, call this X
	 * if X = 0, branch to exit code
	 */
	beq	exitirq
	adr	r5, Lirq_ffs_table
irqloop:
	/*
	 * at this point:
	 *	r5 = address of ffs table
	 *	r7 = address of irq handlers table
	 *	r8 = irq request
	 *	r10 = bit of irq to be serviced
	 *	r11 = bitmask of IRQ's to service
	 */

	/* find the set bit */
	orr	r9, r10, r10, lsl #4	/* X * 0x11 */
	orr	r9, r9, r9, lsl #6	/* X * 0x451 */
	rsb	r9, r9, r9, lsl #16	/* X * 0x0450fbaf */
	/* fetch the bit number */
	ldrb	r9, [r5, r9, lsr #26 ]

	/* 
	 * r9 = irq to service
	 */

	/* apologies for the dogs dinner of code here, but it's in an attempt
	 * to minimise stalling on SA's, hence lots of things happen here:
	 *	- getting address of handler, if it doesn't exist we call
	 *	  stray_irqhandler this is assumed to be rare so we don't
	 *	  care about performance for it
	 *	- statinfo is updated
	 *	- unsetting of the irq bit in r11
	 *	- irq stats (if enabled) also get put in the mix
	 */
	ldr	r4, Lcnt		/* Stat info A */
	ldr	r6, [r7, r9, lsl #2]	/* Get address of first handler structure */

	ldr	r1, [r4, #(V_INTR)]	/* Stat info B */
	
	teq	r6, #0x00000000		/* Do we have a handler */
	moveq	r0, r8			/* IRQ requests as arg 0 */
	adreq	lr, nextirq		/* return Address */
	beq	_C_LABEL(stray_irqhandler) /* call special handler */
	
#ifdef IRQSTATS
	ldr	r2, Lintrcnt
	ldr	r3, [r6, #(IH_NUM)]
#endif
	/* stat info C */
	add	r1, r1, #0x00000001
	str	r1, [r4, #(V_INTR)]

#ifdef IRQSTATS
	ldr	r3, [r2, r3, lsl #2]!
#endif
	bic	r11, r11, r10		/* clear the IRQ bit */

#ifdef IRQSTATS
	add	r3, r3, #0x00000001
	str	r3, [r2]
#endif	/* IRQSTATS */

irqchainloop:
	ldr	r0, [r6, #(IH_ARG)]	/* Get argument pointer */
	teq	r0, #0x00000000		/* If arg is zero pass stack frame */
	addeq	r0, sp, #8		/* ... stack frame [XXX needs care] */
	mov	lr, pc			/* return address */
	ldr	pc, [r6, #(IH_FUNC)]	/* Call handler */

	ldr	r6, [r6, #(IH_NEXT)]	/* fetch next handler */
	
	teq	r0, #0x00000001		/* Was the irq serviced ? */
	
	/* if it was it'll just fall through this: */
	teqne	r6, #0x00000000
	bne	irqchainloop
nextirq:
	/* Check for next irq */
	rsb	r4, r11, #0
	ands	r10, r11, r4
	/* check if there are anymore irq's to service */
	bne 	irqloop

exitirq:
	ldmfd	sp!, {r2, r3}
	ldr	r9, Lcurrent_spl_level
	ldr	r1, Ldisabled_mask
	str	r2, [r9]
	str	r3, [r1]

	bl	_C_LABEL(irq_setmasks)

	bl	_C_LABEL(dosoftints)	/* Handle the soft interrupts */

	/* Kill IRQ's in preparation for exit */
        mrs     r0, cpsr_all
        orr     r0, r0, #(I32_bit)
        msr     cpsr_all, r0

	/* Decrement the nest count */
	ldr	r0, Lcurrent_intr_depth
	ldr	r1, [r0]
	sub	r1, r1, #1
	str	r1, [r0]

	LOCK_CAS_CHECK

	DO_AST_AND_RESTORE_ALIGNMENT_FAULTS
	PULLFRAMEFROMSVCANDEXIT

	/* NOT REACHED */
	b	. - 8

Lspl_mask:
	.word	_C_LABEL(spl_mask)	/* irq's allowed at current spl level */

Lcurrent_mask:
	.word	_C_LABEL(current_mask)	/* irq's that are usable */

ENTRY(irq_setmasks)
	/* Disable interrupts */
	mrs	r3, cpsr_all
	orr	r1, r3,  #(I32_bit)
	msr	cpsr_all, r1

	/* Calculate IOMD interrupt mask */
	ldr	r1, Lcurrent_mask	/* All the enabled interrupts */
	ldr	r2, Lspl_mask		/* Block due to current spl level */
	ldr	r1, [r1]
	ldr	r2, [r2]
	and	r1, r1, r2
	ldr	r2, Ldisabled_mask	/* Block due to active interrupts */
	ldr	r2, [r2]
	bic	r1, r1, r2

	ldr	r0, Liomd_base
 	ldr	r0, [r0]			/* Point to the IOMD */
	strb	r1, [r0, #(IOMD_IRQMSKA << 2)]	/* Set IRQ mask A */
	mov	r1, r1, lsr #8
	strb	r1, [r0, #(IOMD_IRQMSKB << 2)]	/* Set IRQ mask B */
	mov	r1, r1, lsr #8

	ldr	r2, Larm7500_ioc_found
	ldr	r2, [r2]
	cmp	r2, #0
	beq	skip_setting_extended_DMA_mask

	/* only for ARM7500's */
	strb	r1, [r0, #(IOMD_IRQMSKC << 2)]
	mov	r1, r1, lsr #8
	and	r2, r1, #0xef
	strb	r2, [r0, #(IOMD_IRQMSKD << 2)]
	mov	r1, r1, lsr #3
	and	r2, r1, #0x10
	strb	r2, [r0, #(IOMD_DMAMSK << 2)]	/* Set DMA mask */
	b	continue_setting_masks

skip_setting_extended_DMA_mask:
	/* non ARM7500's */
	strb	r1, [r0, #(IOMD_DMAMSK << 2)]	/* Set DMA mask */

continue_setting_masks:

	/* Restore old cpsr and exit */
	msr	cpsr_all, r3
	mov	pc, lr

Lcnt:
	.word	_C_LABEL(uvmexp)

Lintrcnt:
	.word	_C_LABEL(intrcnt)


Lirqhandlers:
	.word	_C_LABEL(irqhandlers)	/* Pointer to array of irqhandlers */

#ifdef IRQSTATS
/* These symbols are used by vmstat */

	.text
	.global	_C_LABEL(_intrnames)
_C_LABEL(_intrnames):
	.word	_C_LABEL(intrnames)

	.data

        .globl  _C_LABEL(intrnames), _C_LABEL(eintrnames), _C_LABEL(intrcnt), _C_LABEL(sintrcnt), _C_LABEL(eintrcnt)
_C_LABEL(intrnames):
	.asciz	"interrupt  0 "
	.asciz	"interrupt  1 "	/* reserved0 */
	.asciz	"interrupt  2 "
	.asciz	"interrupt  3 "
	.asciz	"interrupt  4 "
	.asciz	"interrupt  5 "
	.asciz	"interrupt  6 "
	.asciz	"interrupt  7 "	/* reserved1 */
	.asciz	"interrupt  8 " /* reserved2 */
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

_C_LABEL(sintrnames):
	.asciz	"softclock    "
	.asciz	"softnet      "
	.asciz	"softserial   "
	.asciz	"softintr  3  "
	.asciz	"softintr  4  "
	.asciz	"softintr  5  "
	.asciz	"softintr  6  "
	.asciz	"softintr  7   "
	.asciz	"softintr  8  "
	.asciz	"softintr  9  "
	.asciz	"softintr 10  "
	.asciz	"softintr 11  "
	.asciz	"softintr 12  "
	.asciz	"softintr 13  "
	.asciz	"softintr 14  "
	.asciz	"softintr 15  "
	.asciz	"softintr 16  "
	.asciz	"softintr 17  "
	.asciz	"softintr 18  "
	.asciz	"softintr 19  "
	.asciz	"softintr 20  "
	.asciz	"softintr 21  "
	.asciz	"softintr 22  "
	.asciz	"softintr 23  "
	.asciz	"softintr 24  "
	.asciz	"softintr 25  "
	.asciz	"softintr 26  "
	.asciz	"softintr 27  "
	.asciz	"softintr 28  "
	.asciz	"softintr 29  "
	.asciz	"softintr 30  "
	.asciz	"softintr 31  "
_C_LABEL(eintrnames):

	.bss
	.align	0
_C_LABEL(intrcnt):
	.space	32*4	/* XXX Should be linked to number of interrupts */

_C_LABEL(sintrcnt):
	.space	32*4	/* XXX Should be linked to number of interrupts */
_C_LABEL(eintrcnt):

#else	/* IRQSTATS */
	/* Dummy entries to keep vmstat happy */

	.text
        .globl  _C_LABEL(intrnames), _C_LABEL(eintrnames), _C_LABEL(intrcnt), _C_LABEL(eintrcnt)
_C_LABEL(intrnames):
	.long	0
_C_LABEL(eintrnames):

_C_LABEL(intrcnt):
	.long	0
_C_LABEL(eintrcnt):
#endif	/* IRQSTATS */
