/*
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
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/Attic/interrupt.s,v 1.7 1993/12/12 09:08:43 glass Exp $
 */

.globl _cnt

.data
.globl _intrcnt
_intrcnt:
    /* spurious  1  2  3  4  5  6  7*/
	.long 0, 0, 0, 0, 0, 0, 0, 0
.globl _clock_turn
_clock_turn:
	.long 0	
.globl	timebomb
timebomb:
	.long	0
.text

#define INTERRUPT_BEGIN(interrupt_num) \
	clrw	sp@-		;	/* ???? stack alignment?*/\
	moveml	#0xC0C0,sp@-	;	/* save a0 a1, d0, d1 */\
	addql #1,_intrcnt+interrupt_num*4;/*increment interrupt counter */

#define INTERRUPT_INTRHAND \
	movw	sr,sp@-		;	/* push current SR value */\
	clrw	sp@-		;	/*    padded to longword */\
	jbsr	_intrhand	;	/* handle interrupt 	 */\
	addql	#4,sp		;	/* pop SR		 */\

#define INTERRUPT_END \
	moveml	sp@+,#0x0303	;	/* restore a0, a1, d0, d1*/\
	addql	#2,sp		;	/* undo stack alignment? hanck*/\
	addql #1, _cnt+V_INTR	;	/* more statistics gathering */\
	jra rei

#define INTERRUPT_HANDLE(num) \
	INTERRUPT_BEGIN(num) \
	INTERRUPT_INTRHAND \
	INTERRUPT_END

.globl _level0intr, _level1intr, _level2intr, _level3intr, _level4intr
.globl _level5intr, _level6intr, _level7intr

.align 4
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

.align 4
_level5intr:
	INTERRUPT_HANDLE(5)

/* clock */
.globl _level5intr_clock, _interrupt_reg, _clock_intr
.align 4
_level5intr_clock:
	tstb CLOCK_VA+INTERSIL_INTR_OFFSET
	andb #~IREG_CLOCK_ENAB_5, INTERREG_VA
	orb #IREG_CLOCK_ENAB_5, INTERREG_VA
	tstb CLOCK_VA+INTERSIL_INTR_OFFSET
	notl _clock_turn
	jeq cont
	rte
cont:
	INTERRUPT_BEGIN(5)	| stack aligned, a0, a1, d0, d1 saved
#define CLOCK_DEBUG
#ifdef CLOCK_DEBUG
	.globl	_panicstr, _regdump, _panic
	tstl	timebomb		| set to go off?
	jeq	Lnobomb			| no, skip it
	subql	#1,timebomb		| decrement
	jne	Lnobomb			| not ready to go off
	moveml	sp@+,#0x0303		| temporarily restore regs
	jra	Lbomb			| go die
Lnobomb:
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
	lea sp@(16), a1
	movl a1@, sp@-		| clockframe.sr (ps)
	movl a1@(4), sp@-	| clockframe.pc
	jbsr _clock_intr
	addl #8,sp		| pop clockframe
	INTERRUPT_END

/* SCCs */
.align 4
_level6intr:
	INTERRUPT_HANDLE(6)

/* Memory Error/NMI */
.align 4
_level7intr:
	INTERRUPT_HANDLE(7)

_spurintr:
	
