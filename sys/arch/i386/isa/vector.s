/*	$NetBSD: vector.s,v 1.35.6.1 1997/03/12 14:34:52 is Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission.
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
 */

#include <i386/isa/icu.h>
#include <dev/isa/isareg.h>

#define ICU_HARDWARE_MASK

/*
 * These macros are fairly self explanatory.  If ICU_SPECIAL_MASK_MODE is
 * defined, we try to take advantage of the ICU's `special mask mode' by only
 * EOIing the interrupts on return.  This avoids the requirement of masking and
 * unmasking.  We can't do this without special mask mode, because the ICU
 * would also hold interrupts that it thinks are of lower priority.
 *
 * Many machines do not support special mask mode, so by default we don't try
 * to use it.
 */

#define	IRQ_BIT(irq_num)	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	((irq_num) / 8)

#ifdef ICU_SPECIAL_MASK_MODE

#define	ACK1(irq_num)
#define	ACK2(irq_num) \
	movb	$(0x60|IRQ_SLAVE),%al	/* specific EOI for IRQ2 */	;\
	outb	%al,$IO_ICU1
#define	MASK(irq_num, icu)
#define	UNMASK(irq_num, icu) \
	movb	$(0x60|(irq_num%8)),%al	/* specific EOI */		;\
	outb	%al,$icu

#else /* ICU_SPECIAL_MASK_MODE */

#ifndef	AUTO_EOI_1
#define	ACK1(irq_num) \
	movb	$(0x60|(irq_num%8)),%al	/* specific EOI */		;\
	outb	%al,$IO_ICU1
#else
#define	ACK1(irq_num)
#endif

#ifndef AUTO_EOI_2
#define	ACK2(irq_num) \
	movb	$(0x60|(irq_num%8)),%al	/* specific EOI */		;\
	outb	%al,$IO_ICU2		/* do the second ICU first */	;\
	movb	$(0x60|IRQ_SLAVE),%al	/* specific EOI for IRQ2 */	;\
	outb	%al,$IO_ICU1
#else
#define	ACK2(irq_num)
#endif

#ifdef ICU_HARDWARE_MASK

#define	MASK(irq_num, icu) \
	movb	_imen + IRQ_BYTE(irq_num),%al				;\
	orb	$IRQ_BIT(irq_num),%al					;\
	movb	%al,_imen + IRQ_BYTE(irq_num)				;\
	FASTER_NOP							;\
	outb	%al,$(icu+1)
#define	UNMASK(irq_num, icu) \
	cli								;\
	movb	_imen + IRQ_BYTE(irq_num),%al				;\
	andb	$~IRQ_BIT(irq_num),%al					;\
	movb	%al,_imen + IRQ_BYTE(irq_num)				;\
	FASTER_NOP							;\
	outb	%al,$(icu+1)						;\
	sti

#else /* ICU_HARDWARE_MASK */

#define	MASK(irq_num, icu)
#define	UNMASK(irq_num, icu)

#endif /* ICU_HARDWARE_MASK */

#endif /* ICU_SPECIAL_MASK_MODE */

/*
 * Macros for interrupt entry, call to handler, and exit.
 *
 * XXX
 * The interrupt frame is set up to look like a trap frame.  This may be a
 * waste.  The only handler which needs a frame is the clock handler, and it
 * only needs a few bits.  Xdoreti() needs a trap frame for handling ASTs, but
 * it could easily convert the frame on demand.
 *
 * The direct costs of setting up a trap frame are two pushl's (error code and
 * trap number), an addl to get rid of these, and pushing and popping the
 * callee-saved registers %esi, %edi, %ebx, and %ebp twice.
 *
 * If the interrupt frame is made more flexible,  INTR can push %eax first and
 * decide the ipending case with less overhead, e.g., by avoiding loading the
 * segment registers.
 *
 * XXX
 * Should we do a cld on every system entry to avoid the requirement for
 * scattered cld's?
 */

	.globl	_isa_strayintr

/*
 * Normal vectors.
 *
 * We cdr down the intrhand chain, calling each handler with its appropriate
 * argument (0 meaning a pointer to the frame, for clock interrupts).
 *
 * The handler returns one of three values:
 *   0 - This interrupt wasn't for me.
 *   1 - This interrupt was for me.
 *  -1 - This interrupt might have been for me, but I don't know.
 * If there are no handlers, or they all return 0, we flags it as a `stray'
 * interrupt.  On a system with level-triggered interrupts, we could terminate
 * immediately when one of them returns 1; but this is a PC.
 *
 * On exit, we jump to Xdoreti(), to process soft interrupts and ASTs.
 */
#define	INTR(irq_num, icu, ack) \
IDTVEC(recurse/**/irq_num)						;\
	pushfl								;\
	pushl	%cs							;\
	pushl	%esi							;\
	cli								;\
_Xintr/**/irq_num/**/:							;\
	pushl	$0			/* dummy error code */		;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	MAKE_FRAME							;\
	MASK(irq_num, icu)		/* mask it in hardware */	;\
	ack(irq_num)			/* and allow other intrs */	;\
	incl	_cnt+V_INTR		/* statistical info */		;\
	testb	$IRQ_BIT(irq_num),_cpl + IRQ_BYTE(irq_num)		;\
	jnz	_Xhold/**/irq_num	/* currently masked; hold it */	;\
_Xresume/**/irq_num/**/:						;\
	movl	_cpl,%eax		/* cpl to restore on exit */	;\
	pushl	%eax							;\
	orl	_intrmask + (irq_num) * 4,%eax				;\
	movl	%eax,_cpl		/* add in this intr's mask */	;\
	sti				/* safe to take intrs now */	;\
	movl	_intrhand + (irq_num) * 4,%ebx	/* head of chain */	;\
	testl	%ebx,%ebx						;\
	jz	_Xstray/**/irq_num	/* no handlears; we're stray */	;\
	STRAY_INITIALIZE		/* nobody claimed it yet */	;\
	incl	_intrcnt + (4*(irq_num))	/* XXX */		;\
7:	movl	IH_ARG(%ebx),%eax	/* get handler arg */		;\
	testl	%eax,%eax						;\
	jnz	4f							;\
	movl	%esp,%eax		/* 0 means frame pointer */	;\
4:	pushl	%eax							;\
	call	IH_FUN(%ebx)		/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
	STRAY_INTEGRATE			/* maybe he claimed it */	;\
	incl	IH_COUNT(%ebx)		/* count the intrs */		;\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	7b							;\
	STRAY_TEST			/* see if it's a stray */	;\
5:	UNMASK(irq_num, icu)		/* unmask it in hardware */	;\
	jmp	_Xdoreti		/* lower spl and do ASTs */	;\
IDTVEC(stray/**/irq_num)						;\
	pushl	$irq_num						;\
	call	_isa_strayintr						;\
	addl	$4,%esp							;\
	incl	_strayintrcnt + (4*(irq_num))				;\
	jmp	5b							;\
IDTVEC(hold/**/irq_num)							;\
	orb	$IRQ_BIT(irq_num),_ipending + IRQ_BYTE(irq_num)		;\
	INTRFASTEXIT

#if defined(DEBUG) && defined(notdef)
#define	STRAY_INITIALIZE \
	xorl	%esi,%esi
#define	STRAY_INTEGRATE \
	orl	%eax,%esi
#define	STRAY_TEST \
	testl	%esi,%esi						;\
	jz	_Xstray/**/irq_num
#else /* !DEBUG */
#define	STRAY_INITIALIZE
#define	STRAY_INTEGRATE
#define	STRAY_TEST
#endif /* DEBUG */

#ifdef DDB
#define	MAKE_FRAME \
	leal	-8(%esp),%ebp
#else /* !DDB */
#define	MAKE_FRAME
#endif /* DDB */

INTR(0, IO_ICU1, ACK1)
INTR(1, IO_ICU1, ACK1)
INTR(2, IO_ICU1, ACK1)
INTR(3, IO_ICU1, ACK1)
INTR(4, IO_ICU1, ACK1)
INTR(5, IO_ICU1, ACK1)
INTR(6, IO_ICU1, ACK1)
INTR(7, IO_ICU1, ACK1)
INTR(8, IO_ICU2, ACK2)
INTR(9, IO_ICU2, ACK2)
INTR(10, IO_ICU2, ACK2)
INTR(11, IO_ICU2, ACK2)
INTR(12, IO_ICU2, ACK2)
INTR(13, IO_ICU2, ACK2)
INTR(14, IO_ICU2, ACK2)
INTR(15, IO_ICU2, ACK2)

/*
 * These tables are used by the ISA configuration code.
 */
/* interrupt service routine entry points */
IDTVEC(intr)
	.long   _Xintr0, _Xintr1, _Xintr2, _Xintr3, _Xintr4, _Xintr5, _Xintr6
	.long   _Xintr7, _Xintr8, _Xintr9, _Xintr10, _Xintr11, _Xintr12
	.long   _Xintr13, _Xintr14, _Xintr15

/*
 * These tables are used by Xdoreti() and Xspllower().
 */
/* resume points for suspended interrupts */
IDTVEC(resume)
	.long   _Xresume0, _Xresume1, _Xresume2, _Xresume3, _Xresume4
	.long   _Xresume5, _Xresume6, _Xresume7, _Xresume8, _Xresume9
	.long   _Xresume10, _Xresume11, _Xresume12, _Xresume13, _Xresume14
	.long   _Xresume15
	/* for soft interrupts */
	.long	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.long	_Xsoftserial, _Xsoftnet, _Xsoftclock
/* fake interrupts to resume from splx() */
IDTVEC(recurse)
	.long   _Xrecurse0, _Xrecurse1, _Xrecurse2, _Xrecurse3, _Xrecurse4
	.long   _Xrecurse5, _Xrecurse6, _Xrecurse7, _Xrecurse8, _Xrecurse9
	.long   _Xrecurse10, _Xrecurse11, _Xrecurse12, _Xrecurse13, _Xrecurse14
	.long   _Xrecurse15
	/* for soft interrupts */
	.long	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.long	_Xsoftserial, _Xsoftnet, _Xsoftclock


/* Old-style vmstat -i interrupt counters.  Should be replaced with evcnts. */
	.globl	_intrnames, _eintrnames, _intrcnt, _eintrcnt

	/* Names */
_intrnames:
	.asciz	"irq0", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
_strayintrnames:
	.asciz	"stray0", "stray1", "stray2", "stray3"
	.asciz	"stray4", "stray5", "stray6", "stray7"
	.asciz	"stray8", "stray9", "stray10", "stray11"
	.asciz	"stray12", "stray13", "stray14", "stray15"
_eintrnames:

	/* And counters */
	.data
	.align 4
_intrcnt:
	.long	0, 0, 0, 0, 0, 0, 0, 0
	.long	0, 0, 0, 0, 0, 0, 0, 0
_strayintrcnt:
	.long	0, 0, 0, 0, 0, 0, 0, 0
	.long	0, 0, 0, 0, 0, 0, 0, 0
_eintrcnt:

	.text
