/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
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
 *	from: Utah $Hdr: locore.s 1.58 91/04/22$
 *	from: @(#)locore.s	7.11 (Berkeley) 5/9/91
 *
 *	$Id: softint.s,v 1.7 1994/09/20 16:52:29 gwr Exp $
 */

/*
 * This code is (mostly) un-altered from the hp300 code,
 * except that sun machines do not need a simulated SIR
 * because they have a real software interrupt register.
 */
/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing ASTs
 * (profiling, scheduling) and software interrupts (network, softclock).
 * We check for ASTs first, just like the VAX.  To avoid excess overhead
 * the T_ASTFLT handling code will also check for software interrupts so we
 * do not have to do it here.  After identifing that we need an AST we
 * drop the IPL to allow device interrupts.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.
 */

	.globl	_astpending
rei:
#ifdef STACKCHECK
	tstl	_panicstr		| have we paniced?
	jne	Ldorte1			| yes, do not make matters worse
#endif
	tstl	_astpending		| AST pending?
	jeq	Lchksir			| no, go check for SIR
Lrei1:
	btst	#5,sp@			| yes, are we returning to user mode?
	jne	Lchksir			| no, go check for SIR
	movw	#PSL_LOWIPL,sr		| lower SPL
	clrl	sp@-			| stack adjust
	moveml	#0xFFFF,sp@-		| save all registers
	movl	usp,a1			| including
	movl	a1,sp@(FR_SP)		|    the users SP
	clrl	sp@-			| VA == none
	clrl	sp@-			| code == none
	movl	#T_ASTFLT,sp@-		| type == async system trap
	jbsr	_trap			| go handle it
	lea	sp@(12),sp		| pop value args
	movl	sp@(FR_SP),a0		| restore user SP
	movl	a0,usp			|   from save area
	movw	sp@(FR_ADJ),d0		| need to adjust stack?
	jne	Laststkadj		| yes, go to it
	moveml	sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,sp			| toss SP and stack adjust
#ifdef STACKCHECK
	jra	Ldorte
#else
	rte				| and do real RTE
#endif
Laststkadj:
	lea	sp@(FR_HW),a1		| pointer to HW frame
	addql	#8,a1			| source pointer
	movl	a1,a0			| source
	addw	d0,a0			|  + hole size = dest pointer
	movl	a1@-,a0@-		| copy
	movl	a1@-,a0@-		|  8 bytes
	movl	a0,sp@(FR_SP)		| new SSP
	moveml	sp@+,#0x7FFF		| restore user registers
	movl	sp@,sp			| and our SP
#ifdef STACKCHECK
	jra	Ldorte
#else
	rte				| and do real RTE
#endif

Lchksir:
| Sun3 has real interrupt register (no need for simulated one).
Ldorte:
#ifdef STACKCHECK
	movw	#SPL6,sr		| avoid trouble
	btst	#5,sp@			| are we returning to user mode?
	jne	Ldorte1			| no, skip it
	movl	a6,tmpstk-20
	movl	d0,tmpstk-76
	moveq	#0,d0
	movb	sp@(6),d0		| get format/vector
	lsrl	#3,d0			| convert to index
	lea	_exframesize,a6		|  into exframesize
	addl	d0,a6			|  to get pointer to correct entry
	movw	a6@,d0			| get size for this frame
	addql	#8,d0			| adjust for unaccounted for bytes
	lea	_kstackatbase,a6	| desired stack base
	subl	d0,a6			|   - frame size == our stack
	cmpl	a6,sp			| are we where we think?
	jeq	Ldorte2			| yes, skip it
	lea	tmpstk,a6		| will be using tmpstk
	movl	sp@(4),a6@-		| copy common
	movl	sp@,a6@-		|   frame info
	clrl	a6@-
	movl	sp,a6@-			| save sp
	subql	#4,a6			| skip over already saved a6
	moveml	#0x7FFC,a6@-		| push remaining regs (d0/a6/a7 done)
	lea	a6@(-4),sp		| switch to tmpstk (skip saved d0)
	clrl	sp@-			| is an underflow
	jbsr	_badkstack		| badkstack(0, frame)
	addql	#4,sp
	moveml	sp@+,#0x7FFF		| restore most registers
	movl	sp@,sp			| and SP
	rte
Ldorte2:
	movl	tmpstk-76,d0
	movl	tmpstk-20,a6
Ldorte1:
#endif
	rte				| real return


| Set processor priority level calls.  Most are implemented with
| inline asm expansions.  However, we need one instantiation here
| in case some non-optimized code makes external references.
| Most places will use the inlined function param.h supplies.
	.globl	__spl
__spl:
	movl	sp@(4),d1
	clrl	d0
	movw	sr,d0
	movw	d1,sr
	rts

