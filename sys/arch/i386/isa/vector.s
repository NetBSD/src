/*	$Id: vector.s,v 1.10.2.2 1993/10/09 08:50:36 mycroft Exp $ */

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
 *
 * Coding notes for *.s:
 *
 * If possible, avoid operations that involve an operand size override.
 * Word-sized operations might be smaller, but the operand size override
 * makes them slower on on 486's and no faster on 386's unless perhaps
 * the instruction pipeline is depleted.  E.g.,
 *
 *	Use movl to seg regs instead of the equivalent but more descriptive
 *	movw - gas generates an irelevant (slower) operand size override.
 *
 *	Use movl to ordinary regs in preference to movw and especially
 *	in preference to movz[bw]l.  Use unsigned (long) variables with the
 *	top bits clear instead of unsigned short variables to provide more
 *	opportunities for movl.
 *
 * If possible, use byte-sized operations.  They are smaller and no slower.
 *
 * Use (%reg) instead of 0(%reg) - gas generates larger code for the latter.
 *
 * We may not need to load the segment registers to check ipending.
 */

	.globl	_isa_strayintr

#define	INTR(irq_num, icu, enable_icus) \
	pushl	$0 ;		/* dummy error code */ \
	pushl	$T_ASTFLT ; \
	pushl	%ds ; 		/* save our data and extra segments ... */ \
	pushl	%es ; \
	pushl	%eax ; \
	movl	$KDSEL,%eax ;	/* ... and reload with kernel's own ... */ \
	movl	%ax,%ds ; 	/* ... early in case SHOW_A_LOT is on */ \
	movl	%ax,%es ; \
	movb	_imen + IRQ_BYTE(irq_num),%al ; /* mask interrupt in hw */ \
	orb	$IRQ_BIT(irq_num),%al ; \
	movb	%al,_imen + IRQ_BYTE(irq_num) ; \
	outb	%al,$icu+1 ; \
	enable_icus ; 		/* reenable hw interrupts */ \
				/* check if intr masked */ \
	testb	$IRQ_BIT(irq_num),_cpl + IRQ_BYTE(irq_num) ; \
	jne	2f ; \
1: ; \
	popl	%eax ; 		/* fill out trap frame */ \
	pushal ; \
	incl	_cnt+V_INTR ; 	/* increment statistical counters */ \
	COUNT_INTR(_intrcnt_actv, irq_num) ; \
	movl	_cpl,%eax ; 	/* finish interrupt frame */ \
	pushl	%eax ; \
	movl	_intrmask + 4*irq_num,%ebx ; /* add interrupt's spl mask */ \
	orl	(%ebx),%eax ; \
	movl	%eax,_cpl ; \
	sti ; \
	xorl	%ecx,%ecx ; 	/* clear stray flag */ \
	movl	_intrhand + 4*irq_num,%ebx ; /* head of handler chain */ \
	testl	%ebx,%ebx ; 	/* exit loop if no handlers */ \
	jz	3f ; \
	movl	(%ebx),%eax ; 	/* push argument or pointer to frame */ \
	jnz	4f ; \
	movl	%esp,%eax ; \
4: ; \
	pushl	%eax ; \
	movl	4(%ebx),%eax ; 	/* call handler */ \
	call	%eax ; \
	addl	$4,%esp	;	/* pop argument */ \
	orl	%eax,%ecx ; 	/* set stray flag */ \
	movl	8(%ebx),%eax ; 	/* next handler in chain */ \
	testl	%ebx,%ebx ; 	/* exit loop if no more */ \
	jz	3f ; \
	movl	(%ebx),%eax ; 	/* push argument or pointer to frame */ \
	jnz	4b ; \
	movl	%esp,%eax ; \
	jmp	4b ; \
; \
	ALIGN_TEXT ; \
3: ; \
	testl	%ecx,%ecx ; 	/* check for stray interrupt */ \
	jnz	5f ; \
	pushl	$irq_num ; \
	call	_isa_strayintr ; \
5: ; \
	cli ; 			/* unmask interrupt in hw */ \
	movb	_imen + IRQ_BYTE(irq_num),%al ; \
	andb	$~IRQ_BIT(irq_num),%al ; \
	movb	%al,_imen + IRQ_BYTE(irq_num) ; \
	sti ; \
	outb	%al,$icu+1 ; \
	jmp	doreti ; 	/* finish up */ \
; \
	ALIGN_TEXT ; \
2: ; \
	COUNT_EVENT(_intrcnt_pend, irq_num) ; \
	movl	$1b,%eax ; 	/* save resume address; set pending flag */ \
	movl	%eax,Vresume + 4*irq_num ; \
	orb	$IRQ_BIT(irq_num),_ipending + IRQ_BYTE(irq_num) ; \
	popl	%eax ; \
	popl	%es ; \
	popl	%ds ; \
	addl	$8,%esp ; \
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
	.data
Vresume:	.space	16 * 4	/* where to resume intr handler after unpend */
	.globl	_intrcnt
_intrcnt:			/* used by vmstat to calc size of table */
	.globl	_intrcnt_stray
_intrcnt_stray:	.space	4	/* total count of stray interrupts */
	.globl	_intrcnt_actv
_intrcnt_actv:	.space	16 * 4	/* active interrupts */
	.globl	_intrcnt_pend
_intrcnt_pend:	.space	16 * 4	/* pending interrupts */
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

#ifdef INTR_DEBUG
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

/*
 * now the spl names
 */
	.asciz	"unpend_v"
	.asciz	"doreti"
	.asciz	"p0!ni"
	.asciz	"!p0!ni"
	.asciz	"p0ni"
	.asciz	"netisr_raw"
	.asciz	"netisr_ip"
	.asciz	"netisr_imp"
	.asciz	"netisr_ns"
	.asciz	"softclock"
	.asciz	"trap"
	.asciz	"doreti_exit2"
	.asciz	"splbio"
	.asciz	"splclock"
	.asciz	"splhigh"
	.asciz	"splimp"
	.asciz	"splnet"
	.asciz	"splsoftclock"
	.asciz	"spltty"
	.asciz	"splnone"
	.asciz	"netisr_raw2"
	.asciz	"netisr_ip2"
	.asciz	"splx"
	.asciz	"splx!0"
	.asciz	"unpend_V"
	.asciz	"netisr_iso"
	.asciz	"netisr_imp2"
	.asciz	"netisr_ns2"
	.asciz	"netisr_iso2"
	.asciz	"spl29"		/* spl29-spl31 are spares */
	.asciz	"spl30"
	.asciz	"spl31"
/*
 * now the mask names
 */
	.asciz	"cli"
	.asciz	"cpl"
	.asciz	"imen"
	.asciz	"ipending"
	.asciz	"sti"
	.asciz	"mask5"		/* mask5-mask7 are spares */
	.asciz	"mask6"
	.asciz	"mask7"
#endif /* INTR_DEBUG */
	
_eintrnames:
