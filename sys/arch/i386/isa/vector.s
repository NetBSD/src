/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	This product includes software developed by Charles Hannum.
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
 *
 *	$Id: vector.s,v 1.10.2.6 1993/10/15 03:19:44 mycroft Exp $
 */

#include "i386/isa/icu.h"
#include "i386/isa/isa.h"

#define	IRQ_BIT(irq_num)	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	((irq_num) / 8)

#ifndef AUTO_EOI_1
#define	ENABLE_ICU1 \
	movb	$ICU_EOI,%al ;	/* as soon as possible send EOI ... */ \
	outb	%al,$IO_ICU1	/* ... to clear in service bit */
#else /* AUTO_EOI_1 */
#define	ENABLE_ICU1		/* we now use auto-EOI to reduce i/o */
#endif

#ifndef AUTO_EOI_2
#define	ENABLE_ICU1_AND_2 \
	movb	$ICU_EOI,%al ;	/* as above */ \
	outb	%al,$IO_ICU2 ;	/* but do second icu first */ \
	FASTER_NOP ; \
	outb	%al,$IO_ICU1	/* then first icu */
#else /* AUTO_EOI_2 */
#define	ENABLE_ICU1_AND_2	/* data sheet says no auto-EOI on slave ... */
				/* ... but it sometimes works */
#endif

/*
 * Macros for interrupt interrupt entry, call to handler, and exit.
 *
 * XXX - the interrupt frame is set up to look like a trap frame.  This is
 * usually a waste of time.  The only interrupt handler that wants a frame
 * is the clock handler (it wants a clock frame).  The interrupt return
 * routine needs a trap frame for AST's (it could easily convert the frame).
 * The direct costs of setting up a trap frame are two pushl's (error
 * code and trap number), an addl to get rid of these, and pushing and
 * popping the call-saved regs %esi, %edi and %ebp twice,  The indirect
 * costs are making the driver interface nonuniform so unpending of
 * interrupts is more complicated and slower (call_driver(unit) would
 * be easier than ensuring an interrupt frame for all handlers.  Finally,
 * there are some struct copies in the npx handler and maybe in the clock
 * handler that could be avoided by working more with pointers to frames
 * instead of frames.
 *
 * XXX - should we do a cld on every system entry to avoid the requirement
 * for scattered cld's?
 */

/*
 * First-stage interrupt service routines
 */	

	.globl	_isa_strayintr

IDTVEC(stray)
	sti
	pushl	%ds
	pushl	%es
	pushal
	movl	$KDSEL,%eax
	movl	%ax,%ds
	movl	%ax,%es
	pushl	$-1
	call	_isa_strayintr
	addl	$4,%esp
	popal
	popl	%es
	popl	%ds
	iret

#define	INTR(irq_num, icu, enable_icus) \
IDTVEC(intr/**/irq_num) ; \
	ss ; \
	testb	$IRQ_BIT(irq_num),_cpl + IRQ_BYTE(irq_num) ; \
	jnz	2f ; \
	pushl	$0 ;		/* dummy error code */ \
	pushl	$T_ASTFLT ; \
	pushl	%ds ; 		/* save our data and extra segments ... */ \
	pushl	%es ; \
	pushal ; \
	movl	$KDSEL,%eax ;	/* ... and reload with kernel's own ... */ \
	movl	%ax,%ds ; 	/* ... early in case SHOW_A_LOT is on */ \
	movl	%ax,%es ; \
	movb	_imen + IRQ_BYTE(irq_num),%al ; /* mask interrupt in hw */ \
	orb	$IRQ_BIT(irq_num),%al ; \
	movb	%al,_imen + IRQ_BYTE(irq_num) ; \
	outb	%al,$icu+1 ; \
	enable_icus ; 		/* reenable hw interrupts */ \
_Xresume/**/irq_num: ; \
	incl	_cnt+V_INTR ; 	/* increment statistical counters */ \
	incl	_intrcnt_actv + 4*irq_num ; \
	movl	_cpl,%eax ; 	/* finish interrupt frame */ \
	pushl	%eax ; 		/* old spl level for intrframe */ \
	orl	_intrmask + 4*irq_num,%eax ; /* add interrupt's spl mask */ \
	movl	%eax,_cpl ; 	/* mask interrupt's spl level */ \
	sti ; 			/* enable interrupts */ \
	movl	_intrhand + 4*irq_num,%ebx ; /* head of handler chain */ \
	testl	%ebx,%ebx ; 	/* exit loop if no handlers */ \
	jz	6f ; \
	xorl	%ecx,%ecx ; 	/* clear stray flag */ \
7: ; \
	movl	4(%ebx),%eax ; 	/* push argument or pointer to frame */ \
	jnz	4f ; \
	movl	%esp,%eax ; \
4: ; \
	pushl	%eax ; \
	call	(%ebx) ; 	/* call handler */ \
	addl	$4,%esp	;	/* pop argument */ \
	orl	%eax,%ecx ; 	/* set stray flag */ \
	incl	8(%ebx) ; 	/* increment counter */ \
	movl	12(%ebx),%ebx ;	/* next handler in chain */ \
	testl	%ebx,%ebx ; 	/* exit loop if no more */ \
	jnz	7b ; \
	jecxz	6f ;  		/* check for stray interrupt */ \
5: ; \
	cli ; 			/* unmask interrupt in hw */ \
	movb	_imen + IRQ_BYTE(irq_num),%al ; \
	andb	$~IRQ_BIT(irq_num),%al ; \
	movb	%al,_imen + IRQ_BYTE(irq_num) ; \
	outb	%al,$icu+1 ; \
	sti ; \
	jmp	doreti ; 	/* finish up */ \
	ALIGN_TEXT ; \
6: ; \
	pushl	$irq_num ; \
	call	_isa_strayintr ; \
	addl	$4,%esp ; \
	jmp	5b ; \
	ALIGN_TEXT ; \
2: ; \
	pushl	%eax ; \
	ss ; \
	incl	_intrcnt_pend + 4*irq_num ; \
	ss ; \
	movb	_imen + IRQ_BYTE(irq_num),%al ; /* mask interrupt in hw */ \
	orb	$IRQ_BIT(irq_num),%al ; \
	ss ; \
	movb	%al,_imen + IRQ_BYTE(irq_num) ; \
	outb	%al,$icu+1 ; \
	enable_icus ; 		/* reenable hw interrupts */ \
	ss ; \
	orb	$IRQ_BIT(irq_num),_ipending + IRQ_BYTE(irq_num) ; \
	popl	%eax ; \
	iret

INTR(0, IO_ICU1, ENABLE_ICU1)
INTR(1, IO_ICU1, ENABLE_ICU1)
INTR(2, IO_ICU1, ENABLE_ICU1)
INTR(3, IO_ICU1, ENABLE_ICU1)
INTR(4, IO_ICU1, ENABLE_ICU1)
INTR(5, IO_ICU1, ENABLE_ICU1)
INTR(6, IO_ICU1, ENABLE_ICU1)
INTR(7, IO_ICU1, ENABLE_ICU1)
INTR(8, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(9, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(10, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(11, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(12, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(13, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(14, IO_ICU2, ENABLE_ICU1_AND_2)
INTR(15, IO_ICU2, ENABLE_ICU1_AND_2)

/*
 * Interrupt counters
 */
IDTVEC(intr)	/* interrupt service routine entry points */
	.long	_Xintr0, _Xintr1, _Xintr2, _Xintr3, _Xintr4, _Xintr5, _Xintr6
	.long	_Xintr7, _Xintr8, _Xintr9, _Xintr10, _Xintr11, _Xintr12
	.long	_Xintr13, _Xintr14, _Xintr15
IDTVEC(resume)	/* interrupt service routine resume points */
	.long	_Xresume0, _Xresume1, _Xresume2, _Xresume3, _Xresume4
	.long	_Xresume5, _Xresume6, _Xresume7, _Xresume8, _Xresume9
	.long	_Xresume10, _Xresume11, _Xresume12, _Xresume13, _Xresume14
	.long	_Xresume15

	.data
	.globl	_intrcnt
_intrcnt:			/* used by vmstat to calc size of table */
	.globl	_intrcnt_actv
_intrcnt_actv:	.space	16 * 4	/* total interrupts */
	.globl	_intrcnt_pend
_intrcnt_pend:	.space	16 * 4	/* masked interrupts */
	.globl	_intrcnt_stray
_intrcnt_stray:	.space	4	/* total count of stray interrupts */
	.globl	_eintrcnt
_eintrcnt:			/* used by vmstat to calc size of table */

	.text
	.globl	_intrnames, _eintrnames
_intrnames:
	.asciz	"stray"
	.asciz	"irq0"
	.asciz	"irq1"
	.asciz	"irq2"
	.asciz	"irq3"
	.asciz	"irq4"
	.asciz	"irq5"
	.asciz	"irq6"
	.asciz	"irq7"
	.asciz	"irq8"
	.asciz	"irq9"
	.asciz	"irq10"
	.asciz	"irq11"
	.asciz	"irq12"
	.asciz	"irq13"
	.asciz	"irq14"
	.asciz	"irq15"
	.asciz	"irq0 pend"
	.asciz	"irq1 pend"
	.asciz	"irq2 pend"
	.asciz	"irq3 pend"
	.asciz	"irq4 pend"
	.asciz	"irq5 pend"
	.asciz	"irq6 pend"
	.asciz	"irq7 pend"
	.asciz	"irq8 pend"
	.asciz	"irq9 pend"
	.asciz	"irq10 pend"
	.asciz	"irq11 pend"
	.asciz	"irq12 pend"
	.asciz	"irq13 pend"
	.asciz	"irq14 pend"
	.asciz	"irq15 pend"
_eintrnames:
