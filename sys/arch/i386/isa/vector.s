/*	$Id: vector.s,v 1.10.2.3 1993/10/12 23:36:36 mycroft Exp $ */

#include "i386/isa/icu.h"
#include "i386/isa/isa.h"

#define	IRQ_BIT(irq_num)	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	((irq_num) / 8)

#ifndef AUTO_EOI_1
#define	ENABLE_ICU1 \
	movb	$ICU_EOI,%al ;	/* as soon as possible send EOI ... */ \
	FASTER_NOP ;		/* ... ASAP ... */ \
	outb	%al,$IO_ICU1	/* ... to clear in service bit */
#else /* AUTO_EOI_1 */
#define	ENABLE_ICU1		/* we now use auto-EOI to reduce i/o */
#endif

#ifndef AUTO_EOI_2
#define	ENABLE_ICU1_AND_2 \
	movb	$ICU_EOI,%al ;	/* as above */ \
	FASTER_NOP ; \
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
 * usually a waste of time.  The only interrupt handlers that want a frame
 * are the clock handler (it wants a clock frame), the npx handler (it's
 * easier to do right all in assembler).  The interrupt return routine
 * needs a trap frame for rare AST's (it could easily convert the frame).
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

	.globl	_isa_strayintr

#define	INTR(irq_num, icu, enable_icus) \
IDTVEC(irq_num) ; \
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
Vresume/**/irq_num: ; \
	incl	_cnt+V_INTR ; 	/* increment statistical counters */ \
	COUNT_INTR(_intrcnt_actv, irq_num) ; \
	movl	_cpl,%eax ; 	/* finish interrupt frame */ \
	pushl	%eax ; \
	orl	_intrmask + 4*irq_num,%eax ; /* add interrupt's spl mask */ \
	movl	%eax,_cpl ; \
	sti ; \
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
 * These are the interrupt counters, I moved them here from icu.s so that
 * they are with the name table.  rgrimes
 *
 * There are now lots of counters, this has been redone to work with
 * Bruce Evans intr-0.1 code, which I modified some more to make it all
 * work with vmstat.
 */
	ALIGN_TEXT
Vresume:			/* where to resume intr handler after unpend */
	.long	Vresume0, Vresume1, Vresume2, Vresume3, Vresume4, Vresume5
	.long	Vresume6, Vresume7, Vresume8, Vresume9, Vresume10, Vresume11
	.long	Vresume12, Vresume13, Vresume14, Vresume15

	.data
	.globl	_intrcnt
_intrcnt:			/* used by vmstat to calc size of table */
	.globl	_intrcnt_stray
_intrcnt_stray:	.space	4	/* total count of stray interrupts */
	.globl	_intrcnt_actv
_intrcnt_actv:	.space	16 * 4	/* active interrupts */
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
_eintrnames:
