/*	$NetBSD: ofw_irq.S,v 1.1.14.2 2002/06/23 17:34:55 jdolecek Exp $	*/

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
#include <machine/irqhandler.h>

	.text
	.align	0

/*
 *
 * irq_entry
 *
 * Main entry point for the IRQ vector
 *
 * This function is called only on timer ticks, passed on to the
 * kernel from the OFW tick handler.
 * 
 * For now, I am trying to re-use as much of the code from the
 * IOMD interrupt-handler as possible.  In time, I will strip this
 * down to something OFW-specific.
 * 
 * Here's the original, IOMD-specific description:
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

Lofw_ticktmp:
	.word	_C_LABEL(ofw_ticktmp)

Lirq_entry:
	.word	irq_entry

Lofwirqstk:			/* hack */
	.word	ofwirqstk + 4096

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
	/*
	 *  We come here following an OFW-handled timer tick.
	 *  
 	 *  We are in the SVC frame, and interrupts are disabled.
 	 *  The state of the interrupted context is partially in
 	 *  the registers and partially in the global storage area
 	 *  labeled ofw_ticktmp.  ofw_ticktmp is filled-in by the
 	 *  tick callback that is invoked by OFW on the way out of
 	 *  its interrupt handler.  ofw_ticktmp contains the following:
 	 *  
 	 *      pc			// interrupted instruction
 	 *      lr_usr
 	 *      sp_usr
 	 *      r1			// makes r1 available for scratch
 	 *      r0			// makes r0 available for scratch
 	 *      spsr_svc		// cpsr of interrupted context
 	 *  
 	 *  The prologue of this routine must re-construct the
 	 *  machine state that existed at the time OFW's interrupt-
 	 *  handler fielded the interrupt.  That allows us to use
 	 *  the rest of the code in this routine, and have it all
 	 *  "just work."
	 */

	/*
	 * Switch to IRQ mode.
	 * First check the spsr in ofw_ticktmp to see what the FIQ bit should be.
	 *
	 * I need 2 scratch registers to do this.
	 * Fortunately, r0 and r1 are already saved in ofw_ticktmp.
	 * How convenient.
	 */
	ldr	r0, Lofw_ticktmp
	ldr	r0, [r0]
	and	r0, r0, #F32_bit
	mov	r1, #(I32_bit | PSR_IRQ32_MODE)
	orr	r1, r1, r0
	msr	cpsr_all, r1

	/* Now we're in IRQ mode. */
	/* Restore contents of ofw_ticktmp. */
	add	r0, pc, #(Lofwirqstk - . - 8)	/* Bummer!  Mitch hasn't left me a stack. */
	ldr	sp, [r0]			/* I'll use my own for now... */
	ldr	r0, Lofw_ticktmp	/* r0 now points to ofw_ticktmp[0] */
	ldr	r1, [r0], #(4*3)	/* skip over saved {r0, r1} */
	msr	spsr_all, r1		/* restore spsr */
	ldmia	r0, {sp, lr}^		/* restore user sp and lr */
	add	r0, r0, #(4*2)		/* previous instruction can't writeback */
					/* this one can't use banked registers */
	ldr	lr, [r0], #(-4*4)	/* restore pc; point r0 at ofw_ticktmp[1] */
	add	lr, lr, #4		/* pc += 4; will be decremented below */
	ldmia	r0, {r0, r1}		/* restore r0 and r1 */

	/* OK, the machine state should be identical now to that when */
	/* OFW fielded the interrupt.  So just fall through... */

	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */

	/*
	 *  Can't field this interrupt now if priority is _SPL_CLOCK
	 *  or higher.  For now, we'll just ignore the interrupt.
	 *  Soon, we will have to schedule it for later action.
	 */
	ldr	r0, Lcurrent_spl_level
	ldr	r0, [r0]
	cmp	r0, #_SPL_CLOCK
	blt	ofwtakeint

	PULLFRAMEFROMSVCANDEXIT
	movs	pc, lr			/* Exit */

 	/*  
 	 *  Stuff a bit-mask into r8 indicating which interrupts
 	 *  are pending.  In our case, that is just the timer0
 	 *  interrupt:  (1 << TIMER0).  The existing code will take
 	 *  care of invoking that handler and the softint/ast stuff
 	 *  which follows it.
	 */
ofwtakeint:
	mov	r8, #0x00000001		/* timer interrupt pending! */
	mov	r8, r8, lsl #IRQ_TIMER0

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

	/* Update the irq masks */
	bl	_C_LABEL(irq_setmasks)

        mrs     r0, cpsr_all		/* Enable IRQ's */
	bic	r0, r0, #I32_bit
	msr	cpsr_all, r0

	ldr	r7, [pc, #Lirqhandlers - . - 8]
        mov	r9, #0x00000001

irqloop:
	/* This would benefit from a special ffs type routine */
	tst	r8, r9			/* Is a bit set ? */
	beq	nextirq			/* No ? try next bit */

	ldr	r6, [r7]		/* Get address of first handler structure */

	teq	r6, #0x00000000		/* Do we have a handler */
	moveq	r0, r8			/* IRQ requests as arg 0 */
	beq	_C_LABEL(stray_irqhandler) /* call special handler */

        ldr	r0, Lcnt
	ldr	r1, [r0, #(V_INTR)]
	add	r1, r1, #0x00000001
	str	r1, [r0, #(V_INTR)]

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

irqchainloop:
	add	lr, pc, #nextinchain - . - 8	/* return address */
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
	teq	r9, #(1 << 24)		/* done the last bit ? */
	bne	irqloop			/* no - loop back. */

	ldmfd	sp!, {r2}
	ldr	r1, Lcurrent_spl_level
	str	r2, [r1]

	/* Restore previous disabled mask */
	ldmfd	sp!, {r2}
	ldr	r1, Ldisabled_mask
	str	r2, [r1]
	bl	_C_LABEL(irq_setmasks)

	bl	_C_LABEL(dosoftints)	/* Handle the soft interrupts */

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
	bl	_C_LABEL(ast)

/* Kill IRQ's in preparation for exit */

        mrs     r0, cpsr_all
        orr     r0, r0, #(I32_bit)
        msr     cpsr_all, r0

	PULLFRAMEFROMSVCANDEXIT

	movs	pc, lr			/* Exit */
	

Lspl_mask:
	.word	_C_LABEL(spl_mask)	/* irq's allowed at current spl level */

Lcurrent_mask:
	.word	_C_LABEL(current_mask)	/* irq's that are usable */


ENTRY(irq_setmasks)
	/* Do nothing */
	mov	pc, lr


Lcnt:
	.word	_C_LABEL(uvmexp)

Lintrcnt:
	.word	_C_LABEL(intrcnt)


Lirqhandlers:
	.word	_C_LABEL(irqhandlers)	/* Pointer to array of irqhandlers */

Lastpending:
	.word	_C_LABEL(astpending)

	.text
	.global	_C_LABEL(dotickgrovelling)

/* 
 *  Do magic to cause OFW to call our irq_entry
 *  routine when it returns from its tick-handling.
 *
 *  This consists of two sub-tasks:
 *    - save some machine state in ofw_ticktmp
 *    - punch some new machine state into the
 *      OFW-supplied frame
 *
 *  We are running in the IRQ frame, with
 *  interrupts disabled.
 *
 *  r0 - base of saved OFW interrupt frame, which
 *       has the following format:
 *  
 *         pc			// interrupted instruction
 *         lr			// lr of interrupted context
 *         sp			// sp of interrupted context
 *         r12
 *         ...		// non-banked register values
 *         ...		//   of interrupted context        
 *         r0
 *         spsr		// psr of interrupted context
 *  
 */

_C_LABEL(dotickgrovelling):
	/*assert((cpsr & PSR_MODE) == PSR_IRQ32_MODE);*/

	stmfd	sp!, {r1-r5}		/* scratch registers r1-r5 */

	/*
	 *  Sub-task 1:
	 *
	 *    Our irq_entry routine needs to re-construct
	 *    the state of the machine at the time OFW
	 *    fielded the interrupt, so that we can use
	 *    the rest of the standard interrupt-handling
	 *    code.  Specifically, irq_entry needs to get
	 *    at the following machine state:
	 *
 	 *      pc              // interrupted instruction
 	 *      lr_usr
 	 *      sp_usr
	 *      r0-r12          // the non-banked registers
	 *                      //   at the time of interruption
 	 *      spsr            // cpsr of interrupted context
	 *
	 *    The non-banked registers will be valid at the
	 *    time irq_entry is called, but the other values
	 *    will not be.  We must save them here, in the
	 *    ofw_ticktmp storage block.  We also save r0
	 *    and r1 so that we have some free registers
	 *    when it's time to do the re-construction.
	 *
	 *    Note that interrupts are not enabled before
	 *    irq_entry is entered, so we don't have to
	 *    worry about ofw_ticktmp getting clobbered.
	 */
	ldr	r1, Lofw_ticktmp		/* r1 points to ofw_ticktmp[0] */

	ldr	r2, [r0, #0]			/* ofwframe[0] is spsr */
	stmia	r1!, {r2}			/* put it in ofw_ticktmp[0] */

	ldr	r2, [r0, #(4*1)]		/* ofwframe[1] is saved r0 */
	stmia	r1!, {r2}			/* put it in ofw_ticktmp[1] */

	ldr	r2, [r0, #(4*2)]		/* ofwframe[2] is saved r1 */
	stmia	r1!, {r2}			/* put it in ofw_ticktmp[2] */

	stmia	r1, {sp, lr}^		/* put {sp,lr}_usr in ofw_ticktmp[3,4]; */
							/* the user registers are still valid */
							/* because we haven't left IRQ mode */
	add	r1, r1, #(4*2)		/* previous instruction can't writeback */
							/* this one can't use banked registers */

	ldr	r2, [r0, #(4*16)]		/* ofwframe[16] is pc */
	stmia	r1!, {r2}			/* put it in ofw_ticktmp[5] */


	/*
	 *  Sub-task 2:
	 *
	 *    Diddle the OFW-supplied frame such that
	 *    control passes to irq_entry when OFW does
	 *    its return from interrupt.  There are 4
	 *    fields in that frame that we need to plug:
	 *
	 *        pc			// gets irq_entry
	 *        lr			// gets lr_svc
	 *        sp			// gets sp_svc
	 *        spsr		// gets (I32_bit | PSR_SVC32_MODE)
	 *
	 */
	mov	r1, #(I32_bit | PSR_SVC32_MODE)
	str	r1, [r0, #0]			/* plug spsr */

	/* Sneak into SVC mode to get sp and lr */
	mrs	r3, cpsr_all
	bic	r3, r3, #(PSR_MODE)
	orr	r3, r3, #(PSR_SVC32_MODE)
	msr	cpsr_all, r3
	mov	r4, lr				/* snarf lr_svc */
	mov	r5, sp				/* snarf sp_svc */
	bic	r3, r3, #(PSR_MODE)
	orr	r3, r3, #(PSR_IRQ32_MODE)
	msr	cpsr_all, r3
	str	r5, [r0, #(4*14)]		/* plug sp */
	str	r4, [r0, #(4*15)]		/* plug lr */

	ldr	r1, [pc, #(Lirq_entry - . - 8)]
	str	r1, [r0, #(4*16)]		/* plug pc */

	ldmfd	sp!, {r1-r5}
	mov	pc, lr
	

	.bss
	.align 0

_C_LABEL(ofw_ticktmp):
	.space	4 * 6	/* temporary storage for 6 words of machine state */

ofwirqstk:			/* hack */
	.space	4096

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
