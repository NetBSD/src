/*	$NetBSD: interrupt.s,v 1.17 1995/01/11 20:31:30 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

	.data
.globl _clock_turn
_clock_turn:
	.long 0	
.globl	timebomb
timebomb:
	.long	0
	.text

#define INTERRUPT_SAVEREG \
	moveml	#0xC0C0,sp@-

#define INTERRUPT_BODY(num) \
	pea	num		;\
	jbsr	_isr_autovec	;\
	addql	#4,sp

#define INTERRUPT_RESTORE \
	moveml	sp@+,#0x0303

#define INTERRUPT_HANDLE(num) ;\
	INTERRUPT_SAVEREG ;\
	INTERRUPT_BODY(num) ;\
	INTERRUPT_RESTORE ;\
	jra rei			/* XXX - Just do rte here? */

.globl _level0intr, _level1intr, _level2intr, _level3intr
.globl _level4intr, _level5intr, _level6intr, _level7intr

.align 4
/*
 * These are the auto-vector interrupt handlers,
 * for which the CPU provides the vector=0x18+level
 * These are installed in the interrupt vector table.
 */
/* spurious interrupt */
_level0intr:	
	INTERRUPT_HANDLE(0)
/* system enable register 1 */
.align 4
_level1intr:	
	INTERRUPT_HANDLE(1)
/* system enable register 2, SCSI */
.align 4
_level2intr: 
	INTERRUPT_HANDLE(2)

/* system enable register 3, Ethernet */
.align 4
_level3intr:
	INTERRUPT_HANDLE(3)

/* video */
.align 4
_level4intr:
	INTERRUPT_HANDLE(4)

/* clock (see below) */
.align 4
_level5intr:
	INTERRUPT_HANDLE(5)

/* SCCs */
.align 4
_level6intr:
	INTERRUPT_HANDLE(6)

/* Memory Error/NMI */
.align 4
_level7intr:
	INTERRUPT_HANDLE(7)

/* clock */
.globl _level5intr_clock, _interrupt_reg, _clock_intr, _clock_va
.align 4
_level5intr_clock:
	INTERRUPT_SAVEREG 	| save a0, a1, d0, d1
	movl	_clock_va, a0
	movl	_interrupt_reg, a1
	tstb a0@(INTERSIL_INTR_OFFSET)
	andb #~IREG_CLOCK_ENAB_5, a1@
	orb #IREG_CLOCK_ENAB_5, a1@
	tstb a0@(INTERSIL_INTR_OFFSET)

#undef	CLOCK_DEBUG	/* XXX - Broken anyway... -gwr */
#ifdef	CLOCK_DEBUG
	.globl	_panicstr, _regdump, _panic
	tstl	timebomb		| set to go off?
	jeq	Lnobomb			| no, skip it
	subql	#1,timebomb		| decrement
	jne	Lnobomb			| not ready to go off
	INTERRUPT_RESTORE		| temporarily restore regs
	jra	Lbomb			| go die
Lnobomb:
	/* XXX - Needs to allow sp in tmpstack too. -gwr */
	cmpl	#_kstack+NBPG,sp	| are we still in stack pages?
	jcc	Lstackok		| yes, continue normally
	tstl	_curproc		| if !curproc could have swtch_exit'ed,
	jeq	Lstackok		|     might be on tmpstk
	tstl	_panicstr		| have we paniced?
	jne	Lstackok		| yes, do not re-panic
	lea	tmpstk,sp		| no, switch to tmpstk
	moveml	#0xFFFF,sp@-		| push all registers
	movl	#Lstkrip,sp@-		| push panic message
	jbsr	_printf			| preview
	addql	#4,sp
	movl	sp,a0			| remember this spot
	movl	#256,sp@-		| longword count
	movl	a0,sp@-			| and reg pointer
	jbsr	_regdump		| dump core
	addql	#8,sp			| pop params
	movl	#Lstkrip,sp@-		| push panic message
	jbsr	_panic			| ES and D
Lbomb:
	moveml	#0xFFFF,sp@-		| push all registers
	movl	sp,a0			| remember this spot
	movl	#256,sp@-		| longword count
	movl	a0,sp@-			| and reg pointer
	jbsr	_regdump		| dump core
	addql	#8,sp			| pop params
	movl	#Lbomrip,sp@-		| push panic message
	jbsr	_panic			| ES and D
Lstkrip:
	.asciz	"k-stack overflow"
Lbomrip:
	.asciz	"timebomb"
	.even
Lstackok:
#endif
	lea	sp@(16),a1		| a1 = &clockframe
	movl	a1,sp@-
	jbsr	_clock_intr
	addql	#4,sp
	INTERRUPT_RESTORE
	jra	rei

	.globl	_isr_vectored
	.globl	_vect_intr
_vect_intr:
	INTERRUPT_SAVEREG
	movw	sp@(22),sp@-		| push exception vector info
	clrw	sp@-
	jbsr	_isr_vectored		| C dispatcher
	addql	#4,sp			|
	INTERRUPT_RESTORE
	jra	rei			| all done


#undef	INTERRUPT_SAVEREG
#undef	INTERRUPT_BODY
#undef	INTERRUPT_RESTORE
#undef	INTERRUPT_HANDLE

/* interrupt counters (needed by vmstat) */
	.globl	_intrcnt,_eintrcnt,_intrnames,_eintrnames
_intrnames:
	.asciz	"spur"	| 0
	.asciz	"lev1"	| 1
	.asciz	"lev2"	| 2
	.asciz	"lev3"	| 3
	.asciz	"lev4"	| 4
	.asciz	"clock"	| 5
	.asciz	"lev6"	| 6
	.asciz	"nmi"	| 7
_eintrnames:

	.data
	.even
_intrcnt:
	.long	0,0,0,0,0,0,0,0,0,0
_eintrcnt:
