/*	$NetBSD: locore.s,v 1.35 1995/03/29 07:38:50 briggs Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 */
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Allen and Mike here..
	Help me, Obi Wan Kenobi! You're our only hope. 
	And Use the Source, Luke.
   there are a couple of "LAK the hp300 didn't do this for a 68851"
   these are a damn good reason why Daystar caches don't work
   the other is pmap.c: EXTERNAL_CACHE

   Sprinkle gratuitously with pflusha and code this up next week.
   

   Party on...
*/

/*
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 */

#include "assym.s"
#include "vectors.s"
#include "macglobals.s"

#define PSL_T	0x8000

#define BLITZ	movl	\#0xfd000020,a0; \
	blitz:	andl	\#0x0f0f0f0f,a0@+; \
		jmp	blitz

	.globl _dprintf
	.globl _strprintf
#define DPRINTFREG(str,reg)	\
	moveml	\#0xFFFF,sp@-;	\
	movl	reg,sp@- ;	\
	pea	str;         	\
	jbsr	_strprintf ; 	\
	addql	\#8,sp ;     	\
	moveml	sp@+,\#0xFFFF
	
#define DPRINTFVAL(str,val)	\
	moveml	\#0xFFFF,sp@-; 	\
	movl	val,d7 ;	\
	movl	d7,sp@- ;	\
	pea	str;		\
	jbsr	_strprintf ;	\
	addql	\#8,sp	;	\
	moveml	sp@+,\#0xFFFF

	.text

/*
 * This is where we wind up if the kernel jumps to location 0.
 * (i.e. a bogus PC)  This is known to immediately follow the vector
 * table and is hence at 0x400 (see reset vector in vectors.s).
 */
	.globl	_panic
	.globl	_jmp0panic

_jmp0panic:
	pea	Ljmp0panic
	jbsr	_panic
	/* NOTREACHED */

Ljmp0panic:
	.asciz	"kernel jump to zero"
	.even

/*
 * Do a dump.
 * Called by auto-restart.
 */
	.globl	_dumpsys
	.globl	_doadump
_doadump:
	jbsr	_dumpsys
	jbsr	_doboot
	/*NOTREACHED*/


	.globl	_stacknquit, _stack_list

_stacknquit:
	moveml	#0xFFFF,sp@-
	movl	sp, sp@-
	jbsr	_stack_trace
	stop	#0x2700


/*
 * Trap/interrupt vector routines
 */ 

	.globl	_cpu040, _trap, _nofault, _longjmp, _print_bus
_buserr:
	tstl	_nofault		| device probe?
	jeq	_addrerr		| no, handle as usual
	movl	_nofault,sp@-		| yes,
	jbsr	_longjmp		|  longjmp(nofault)
_addrerr:
	clrl	sp@-			| pad SR to longword
	moveml	#0xFFFF,sp@-		| save user registers
	movl	usp,a0			| save the user SP
	movl	a0,sp@(FR_SP)		|   in the savearea
	lea	sp@(FR_HW),a1		| grab base of HW berr frame
	tstl	_cpu040
	jeq	Lbe030			| If we're not an '040
	movl	a1@(8),sp@-		| V = exception address
	clrl	sp@-			| dummy code
	moveq	#0,d0
	movw	a1@(6),d0		| get vector offset
	andw	#0x0fff,d0
	cmpw	#12,d0			| is it address error
	jeq	Lisaerr
	movl	a1@(20),d1		| get fault address
	moveq	#0,d0
	movw	a1@(12),d0		| get SSW
	btst	#11,d0			| check for mis-aligned
	jeq	Lbe1stpg		| no skip
	addl	#3,d1			| get into next page
	andl	#PG_FRAME,d1		| and truncate
Lbe1stpg:
	movl	d1,sp@(4)		| pass fault address.
	movl	d0,sp@			| pass SSW as code
	btst	#10,d0			| test ATC
	jeq	Lisberr			| it's a bus error
	jra	Lismerr
Lbe030:
	moveq	#0,d0
	movw	a1@(10),d0		| grab SSW for fault processing
	btst	#12,d0			| RB set?
	jeq	LbeX0			| no, test RC
	bset	#14,d0			| yes, must set FB
	movw	d0,a1@(10)		| for hardware too
LbeX0:
	btst	#13,d0			| RC set?
	jeq	LbeX1			| no, skip
	bset	#15,d0			| yes, must set FC
	movw	d0,a1@(10)		| for hardware too
LbeX1:
	btst	#8,d0			| data fault?
	jeq	Lbe0			| no, check for hard cases
	movl	a1@(16),d1		| fault address is as given in frame
	jra	Lbe10			| thats it
Lbe0:
	btst	#4,a1@(6)		| long (type B) stack frame?
	jne	Lbe4			| yes, go handle
	movl	a1@(2),d1		| no, can use save PC
	btst	#14,d0			| FB set?
	jeq	Lbe3			| no, try FC
	addql	#4,d1			| yes, adjust address
	jra	Lbe10			| done
Lbe3:
	btst	#15,d0			| FC set?
	jeq	Lbe10			| no, done
	addql	#2,d1			| yes, adjust address
	jra	Lbe10			| done
Lbe4:
	movl	a1@(36),d1		| long format, use stage B address
	btst	#15,d0			| FC set?
	jeq	Lbe10			| no, all done
	subql	#2,d1			| yes, adjust address
Lbe10:
	movl	d1,sp@-			| push fault VA
	movl	d0,sp@-			| and padded SSW
	movw	a1@(6),d0		| get frame format/vector offset
	andw	#0x0FFF,d0		| clear out frame format
	cmpw	#12,d0			| address error vector?
	jeq	Lisaerr			| yes, go to it
	movl	d1,a0			| fault address
	ptestr	#1,a0@,#7		| do a table search
	pmove	psr,sp@			| save result
	btst	#7,sp@			| bus error bit set?
	jeq	Lismerr			| no, must be MMU fault
	clrw	sp@			| yes, re-clear pad word
	jra	Lisberr			| and process as normal bus error
Lismerr:
	movl	#T_MMUFLT,sp@-		| show that we are an MMU fault
	jra	Ltrapnstkadj		| and deal with it
Lisaerr:
	movl	#T_ADDRERR,sp@-		| mark address error
	jra	Ltrapnstkadj		| and deal with it
Lisberr:
	movl	#T_BUSERR,sp@-		| mark bus error
Ltrapnstkadj:
	jbsr	_trap			| handle the error
	lea	sp@(12),sp		| pop value args
	movl	sp@(FR_SP),a0		| restore user SP
	movl	a0,usp			|   from save area
	movw	sp@(FR_ADJ),d0		| need to adjust stack?
	jne	Lstkadj			| yes, go to it
	moveml	sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,sp			| toss SSP and pad
	jra	rei			| all done
Lstkadj:
	lea	sp@(FR_HW),a1		| pointer to HW frame
	addql	#8,a1			| source pointer
	movl	a1,a0			| source
	addw	d0,a0			|  + hole size = dest pointer
	movl	a1@-,a0@-		| copy
	movl	a1@-,a0@-		|  8 bytes
	movl	a0,sp@(FR_SP)		| new SSP
	moveml	sp@+,#0x7FFF		| restore user registers
	movl	sp@,sp			| and our SP
	jra	rei			| all done

/*
 * FP exceptions.
 */
_fpfline:
	clrl	sp@-		| pad SR to longword
	moveml	#0xFFFF,sp@-	| save user registers
	moveq	#T_FPEMULI,d0	| denote it as an FP emulation trap.
	jra	fault		| do it.

_fpunsupp:
	clrl	sp@-		| pad SR to longword
	moveml	#0xFFFF,sp@-	| save user registers
	moveq	#T_FPEMULD,d0	| denote it as an FP emulation trap.
	jra	fault		| do it.
#if 0
	tstl	_cpu040
	jne	_illinst
#ifdef FPSP
	.globl	fpsp_unsupp
	jmp	fpsp_unsupp
#endif
	jra	_illinst
#endif

/*
 * Handles all other FP coprocessor exceptions.
 * Note that since some FP exceptions generate mid-instruction frames
 * and may cause signal delivery, we need to test for stack adjustment
 * after the trap call.
 */
_fpfault:
	clrl	sp@-		| pad SR to longword
	moveml	#0xFFFF,sp@-	| save user registers
	movl	usp,a0		| and save
	movl	a0,sp@(FR_SP)	|   the user stack pointer
	clrl	sp@-		| no VA arg
	movl	_curpcb,a0	| current pcb
	lea	a0@(PCB_FPCTX),a0 | address of FP savearea
	fsave	a0@		| save state
	tstb	a0@		| null state frame?
	jeq	Lfptnull	| yes, safe
	clrw	d0		| no, need to tweak BIU
	movb	a0@(1),d0	| get frame size
	bset	#3,a0@(0,d0:w)	| set exc_pend bit of BIU
Lfptnull:
	fmovem	fpsr,sp@-	| push fpsr!as code argument
	frestore a0@		| restore state
	movl	#T_FPERR,sp@-	| push type arg
	jra	Ltrapnstkadj	| call trap and deal with stack cleanup

/*
 * Coprocessor and format errors can generate mid-instruction stack
 * frames and cause signal delivery hence we need to check for potential
 * stack adjustment.
 */
_coperr:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	movl	usp,a0		| get and save
	movl	a0,sp@(FR_SP)	|   the user stack pointer
	clrl	sp@-		| no VA arg
	clrl	sp@-		| or code arg
	movl	#T_COPERR,sp@-	| push trap type
	jra	Ltrapnstkadj	| call trap and deal with stack adjustments

_fmterr:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	movl	usp,a0		| get and save
	movl	a0,sp@(FR_SP)	|   the user stack pointer
	clrl	sp@-		| no VA arg
	clrl	sp@-		| or code arg
	movl	#T_FMTERR,sp@-	| push trap type
	jra	Ltrapnstkadj	| call trap and deal with stack adjustments

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */
_illinst:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	moveq	#T_ILLINST,d0
	jra	fault

_zerodiv:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	moveq	#T_ZERODIV,d0
	jra	fault

_chkinst:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	moveq	#T_CHKINST,d0
	jra	fault

_trapvinst:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	moveq	#T_TRAPVINST,d0
	jra	fault

_privinst:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	moveq	#T_PRIVINST,d0
	jra	fault

	.globl	fault
fault:
	movl	usp,a0			| get and save
	movl	a0,sp@(FR_SP)		|   the user stack pointer
	clrl	sp@-			| no VA arg
	clrl	sp@-			| or code arg
	movl	d0,sp@-			| push trap type
	jbsr	_trap			| handle trap
	lea	sp@(12),sp		| pop value args
	movl	sp@(FR_SP),a0		| restore
	movl	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| restore most user regs
	addql	#8,sp			| pop SP and pad word
	jra	rei			| all done

	.globl	_straytrap
_badtrap:
	clrl	sp@-			| do that alignment thang
	moveml	#0xffff, sp@-		| save scratch regs
	jbsr	_straytrap		| report
	moveml	sp@+, #0xffff		| restore regs
	addql	#4, sp			| pop align thang
	jra	rei

	.globl	_syscall
_trap0:
	clrl	sp@-			| pad SR to longword
	moveml	#0xFFFF,sp@-		| save user registers
	movl	usp,a0			| save the user SP
	movl	a0,sp@(FR_SP)		|   in the savearea
	movl	d0,sp@-			| push syscall number
	jbsr	_syscall		| handle it
	addql	#4,sp			| pop syscall arg
	movl	sp@(FR_SP),a0		| grab and restore
	movl	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| restore most registers
	addql	#8,sp			| pop SSP and align word
	jra	rei			| all done

/*
 * Our native 4.3 implementation uses trap 1 as sigreturn() and trap 2
 * as a breakpoint trap.
 */
_trap1:
	jra	sigreturn

_trap2:
	jra	_trace

/*
 * Trap 12 is the entry point for the cachectl "syscall" (both HPUX & BSD)
 *	cachectl(command, addr, length)
 * command in d0, addr in a1, length in d1
 */
	.globl	_cachectl
_trap12:
	movl	d1,sp@-			| push length
	movl	a1,sp@-			| push addr
	movl	d0,sp@-			| push command
	jbsr	_cachectl		| do it
	lea	sp@(12),sp		| pop args
	jra	rei			| all done

/*
 * Trap 15 is used for:
 *	- KGDB traps
 *	- trace traps for SUN binaries (not fully supported yet)
 * We just pass it on and let trap() sort it all out
 */
_trap15:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
#ifdef KGDB
	moveq	#T_TRAP15,d0
	movl	sp@(FR_HW),d1		| from user mode?
	andl	#PSL_S,d1
	jeq	fault
	movl	d0,sp@-
	.globl	_kgdb_trap_glue
	jbsr	_kgdb_trap_glue		| returns if no debugger
	addl	#4,sp
#endif
	moveq	#T_TRAP15,d0
	jra	fault


traceloc:
	.asciz	"trace:"
	.even

/*
 * Hit a breakpoint (trap 1 or 2) instruction.
 * Push the code and treat as a normal fault.
 */
_trace:
	clrl	sp@-
	moveml	#0xFFFF,sp@-
#ifdef KGDB
	moveq	#T_TRACE,d0
	movl	sp@(FR_HW),d1		| from user mode?
	andl	#PSL_S,d1
	jeq	fault
	movl	d0,sp@-
	jbsr	_kgdb_trap_glue		| returns if no debugger
	addl	#4,sp
#endif
	moveq	#T_TRACE,d0
	jra	fault

/*
 * The sigreturn() syscall comes here.  It requires special handling
 * because we must open a hole in the stack to fill in the (possibly much
 * larger) original stack frame.
 */
sigreturn:
	lea	sp@(-84),sp		| leave enough space for largest frame
	movl	sp@(84),sp@		| move up current 8 byte frame
	movl	sp@(88),sp@(4)
	movl	#84,sp@-		| default: adjust by 84 bytes
	moveml	#0xFFFF,sp@-		| save user registers
	movl	usp,a0			| save the user SP
	movl	a0,sp@(FR_SP)		|   in the savearea
	movl	#SYS_sigreturn,sp@-	| push syscall number
	jbsr	_syscall		| handle it
	addql	#4,sp			| pop syscall#
	movl	sp@(FR_SP),a0		| grab and restore
	movl	a0,usp			|   user SP
	lea	sp@(FR_HW),a1		| pointer to HW frame
	movw	sp@(FR_ADJ),d0		| do we need to adjust the stack?
	jeq	Lsigr1			| no, just continue
	moveq	#92,d1			| total size
	subw	d0,d1			|  - hole size = frame size
	lea	a1@(92),a0		| destination
	addw	d1,a1			| source
	lsrw	#1,d1			| convert to word count
	subqw	#1,d1			| minus 1 for dbf
Lsigrlp:
	movw	a1@-,a0@-		| copy a word
	dbf	d1,Lsigrlp		| continue
	movl	a0,a1			| new HW frame base
Lsigr1:
	movl	a1,sp@(FR_SP)		| new SP value
	moveml	sp@+,#0x7FFF		| restore user registers
	movl	sp@,sp			| and our SP
	jra	rei			| all done

/*
 * Interrupt handlers.
 *
 *	Level 0:	Spurious: ignored.
 *	Level 1:	HIL
 *	Level 2:
 *	Level 3:	Internal HP-IB
 *	Level 4:	"Fast" HP-IBs, SCSI
 *	Level 5:	DMA, Ethernet, Built-in RS232
 *	Level 6:	Clock
 *	Level 7:	Non-maskable: parity errors, RESET key
 *
 * ALICE: Here are our assignments:
 *
 *      Level 0:        Spurious: ignored
 *      Level 1:        VIA1 (clock, ADB)
 *      Level 2:        VIA2 (NuBus, SCSI)
 *      Level 3:
 *      Level 4:        Serial (SCC)
 *      Level 5:
 *      Level 6:
 *      Level 7:        Non-maskable: parity errors, RESET button, FOO key
 *
 * On the Q700, at least, in "A/UX mode," this should become:
 *
 *	Level 0:        Spurious: ignored
 *	Level 1:        Software
 *	Level 2:        VIA2 (except ethernet, sound)
 *	Level 3:        Ethernet
 *	Level 4:        Serial (SCC)
 *	Level 5:        Sound
 *	Level 6:        VIA1
 *	Level 7:        NMIs: parity errors, RESET button, YANCC error
 */
/* BARF We must re-configure this. */
	.globl	_hardclock, _nmihand

_spurintr:
_lev3intr:
_lev5intr:
_lev6intr:
	addql	#1,_intrcnt+0
	addql	#1,_cnt+V_INTR
	jra	rei

_lev1intr:
	addql	#1,_intrcnt+4
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	movl	sp, sp@-
	jbsr	_via1_intr
	addql	#4,sp
	moveml	sp@+,#0xFFFF
	addql	#4,sp
	addql	#1,_cnt+V_INTR
	jra	rei

	.globl	_real_via2_intr
_lev2intr:
	addql	#1,_intrcnt+8
	clrl	sp@-
	moveml	#0xFFFF,sp@-
	movl	sp, sp@-
	movl	_real_via2_intr,a2
	jbsr	a2@
	addql	#4,sp
	moveml	sp@+,#0xFFFF
	addql	#4,sp
	addql	#1,_cnt+V_INTR
	jra	rei

	.globl _ser_intr

_lev4intr:
	/* handle level 4 (SCC) interrupt special... */
	addql	#1,_intrcnt+12
	clrl	sp@-
	moveml	#0xFFFF,sp@-	| save registers
	movl	sp,sp@-		| push pointer to frame
	jsr	_ser_intr	| call C routine to deal with it (console.c)
	addl	#4,sp		| throw away frame pointer
	moveml	sp@+, #0xFFFF	| restore registers
	addql	#4,sp
	rte			| return from exception
|	jra	rei		| Apparently we don't know what we're doing.

	.globl _rtclock_intr

/* MAJORBARF: Fix this routine to be like Mac clocks */
_rtclock_intr:
	movl	a6@(8),a1		| get pointer to frame in via1_intr
	movl	a1@(64), sp@-		| push ps
	movl	a1@(68), sp@-		| push pc
	movl	sp, sp@-		| push pointer to ps, pc
	jbsr	_hardclock		| call generic clock int routine
	lea	sp@(12), sp		| pop params
	addql	#1,_intrcnt+20		| add another system clock interrupt

	addql	#1,_cnt+V_INTR		| chalk up another interrupt

	movl	#1, d0			| clock taken care of
	rts				| go back to lev1intr...
	|jra	rei			| all done

_lev7intr:
	addql	#1, _intrcnt+16
	clrl	sp@-			| pad SR to longword
	moveml	#0xFFFF,sp@-		| save registers
	movl	usp,a0			| and save
	movl	a0,sp@(FR_SP)		|   the user stack pointer
	jbsr	_nmihand		| call handler
	movl	sp@(FR_SP),a0		| restore
	movl	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| and remaining registers
	addql	#8,sp			| pop SSP and align word
	jra	rei			| all done

/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing ASTs
 * (profiling, scheduling) and software interrupts (network, softclock).
 * We check for ASTs first, just like the VAX.  To avoid excess overhead
 * the T_ASTFLT handling code will also check for software interrupts so we
 * do not have to do it here.  After identifying that we need an AST we
 * drop the IPL to allow device interrupts.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.
 */
	.comm	_ssir,1
	.globl	_astpending
	.globl	rei
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
	tstb	_ssir			| SIR pending?
	jeq	Ldorte			| no, all done
	movl	d0,sp@-			| need a scratch register
	movw	sp@(4),d0		| get SR
	andw	#PSL_IPL7,d0		| mask all but IPL
	jne	Lnosir			| came from interrupt, no can do
	movl	sp@+,d0			| restore scratch register
Lgotsir:
	movw	#SPL1,sr		| prevent others from servicing int
	tstb	_ssir			| too late?
	jeq	Ldorte			| yes, oh well...
	clrl	sp@-			| stack adjust
	moveml	#0xFFFF,sp@-		| save all registers
	movl	usp,a1			| including
	movl	a1,sp@(FR_SP)		|    the users SP
	clrl	sp@-			| VA == none
	clrl	sp@-			| code == none
	movl	#T_SSIR,sp@-		| type == software interrupt
	jbsr	_trap			| go handle it
	lea	sp@(12),sp		| pop value args
	movl	sp@(FR_SP),a0		| restore
	movl	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| and all remaining registers
	addql	#8,sp			| pop SP and stack adjust
#ifdef STACKCHECK
	jra	Ldorte
#else
	rte
#endif
Lnosir:
	movl	sp@+,d0			| restore scratch register
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

/*
 * Kernel access to the current processes kernel stack is via a fixed
 * virtual address.  It is at the same address as in the users VA space.
 * Umap contains the KVA of the first of UPAGES PTEs mapping VA _kstack.
 */
		.data
		.set	_kstack, USRSTACK  
_Umap:		.long	0
| Scratch memory.  Careful when messing with these...
longscratch:	.long	0
longscratch2:	.long	0
pte_tmp:	.long	0  | for get_pte()
_macos_crp1:	.long	0
_macos_crp2:	.long	0
_macos_tc:	.long	0
_macos_tt0:	.long	0
_macos_tt1:	.long	0
_bletch:	.long	0
_esym:		.long	0
		.globl	_kstack, _Umap, _esym, _bletch
		.globl	_macos_crp1, _macos_crp2, _macos_tc
		.globl	_macos_tt0, _macos_tt1

/*
 * Initialization
 *
 * This has been changed majorly in Alice.  The load point, which used
 * to be arbitrary and passed in A5 from the standalone, is now fixed
 * at 0.
 *
 * ALICE: A4 contains the address of the very last page of memory
 */
print1:
	.asciz	"Booting properly, 0xF0F0 should be "
print2:
	.asciz	"Sysptsize "
print3:
	.asciz	"Offset of NuBus PTs in page tables "
print4:
	.asciz	"Offset of NuBus PTs in bytes "
print5:
	.asciz	"Actual address of NuBus space PTs "
print6:
	.asciz	"Address of end of PTs "
print7:
	.asciz	"First PTE for NuBus space "
stackis:
	.asciz	"Stack pointer is "
breakptstr:
	.asciz	"Checkpoint "
justbefmmu:
	.asciz	"About to activate MMU"
memory_last:
	.asciz	"Last page of memory is at:"
abouttouser:
	.asciz	"About to go into user mode."
	.even

	.text
	.globl	_edata
	.globl	_etext
	.globl	start
	.globl _gray_bar
	.globl _videoaddr, _videorowbytes
	.globl _videobitdepth
	.globl _machineid
	.globl _videosize
	.globl _IOBase
	.globl _NuBusBase

	.globl _locore_dodebugmarks

#define debug_mark(s) \
	.data ; \
0:	.asciz	s ; \
	.text ; \
	tstl	_locore_dodebugmarks ; \
	beq	1f ; \
	movml	#0xC0C0, sp@- ; \
	pea	0b ; \
	jbsr	_printf ; \
	addql	#4, sp ; \
	movml	sp@+, #0x0303 ; \
1:	;

start:
	movw	#PSL_HIGHIPL,sr		| no interrupts.  ever.

| Give ourself a stack
	lea	tmpstk,sp		| give ourselves a temporary stack
	movl	#CACHE_OFF,d0
	movc	d0, cacr

| Some parameters provided by MacOS
|
| LAK: This section is the new way to pass information from the booter
| to the kernel.  At A1 there is an environment variable which has
| a bunch of stuff in ascii format, "VAR=value\0VAR=value\0\0".

	.globl	_initenv, _getenvvars	| in machdep.c
	.globl	_setmachdep		| in machdep.c
	.globl	_printenvvars
	.globl	_gothere, _getphysical

	movl	a1, sp@-		| Address of buffer
	movl	d4, sp@-		| Some flags... (probably not used)
	jbsr	_initenv
	addql	#8, sp

	jbsr	_getenvvars		| Parse the environment buffer

	jbsr	_gray_bar		| first graybar call (we need stack).
	jbsr	_setmachdep		| Set some machine-dep stuff

	jbsr	_vm_set_page_size	| Set the vm system page size, now.
	jbsr	_consinit		| XXX Should only be if graybar on

	debug_mark("DEBUG: Console initialized.\n")

	tstl	_cpu040
	beq	Lstartnot040		| It's not an '040
	.word	0xf4f8			| cpusha bc - push and invalidate caches

	movl	#CACHE40_OFF,d0		| 68040 cache disable
	movc	d0, cacr
	movl	#MMU_68040, _mmutype	| 68040 MMU

	jbsr	_gray_bar
	movel	#0x0, d0
	.word	0x4e7b, 0x0004		| Disable itt0
	.word	0x4e7b, 0x0005		| Disable itt1
	.word	0x4e7b, 0x0006		| Disable dtt0
	.word	0x4e7b, 0x0007		| Disable dtt1
	.word	0x4e7b, 0x0003		| Disable MMU
	jbsr	_gray_bar

	jsr	_get_top_of_ram		| Get amount of memory in machine
	addl	_load_addr, d0
	movl	d0,lastpage		| save very last page of memory

	jbsr	_map040			| This is a monster.

	jra	Ldoproc0		| Jump down to setup proc 0

Lstartnot040:

| BG - Figure out our MMU
	movl	#0x200, d0		| data freeze bit (??)
	movc	d0, cacr		| only exists in 68030
	movc	cacr, d0		| on an '851, it'll go away.
	tstl	d0
	jeq	Lisa68020
	movl	#MMU_68030, _mmutype	| 68030 MMU
	jra	Lmmufigured
Lisa68020:
	movl	#MMU_68851, _mmutype	| 68020, implies 68851, or crash.
Lmmufigured:

| LAK: (7/25/94) We need to find out if the MMU is already on.  If it is
|  not, fine.  If it is, then we must get at it because it tells us a lot
|  of information, such as our load address (_load_addr) and the video
|  physical address, not to mention how the memory banks are setup.
|  All this is only applicable to '030s.

	lea	_macos_tc,a0
	pmove	tc,a0@
	btst	#31,a0@			| Bit #31 is Enabled bit
	jeq	mmu_off

| MMU is on; if not '030, then just turn it off
	cmpl	#MMU_68030, _mmutype
	jne	Lnoremap		| not '030 -- skip all this

| find out how it is mapped
	jbsr	_get_mapping		| in machdep; returns logical 0 in d0
	movl	d0,_load_addr		| this is our physical load address
	debug_mark("DEBUG: 68030 - MMU mapping parsed.\n");
	jra	Ldoneremap

Lnoremap:
	| If not on '030, then just turn off the MMU cuase we don't need it.
	lea	longscratch,a0
	movl	#0,a0@
	pmove	a0@,tc

	debug_mark("DEBUG: 68020 - MMU turned off.\n");

Ldoneremap:

mmu_off:

| A4 is passed (was) from MacOS as the very last page in physical memory
	jsr	_get_top_of_ram		| Get amount of memory in machine
	addl	_load_addr, d0
	movl	d0,lastpage		| save very last page of memory
	debug_mark("DEBUG: Got memory size as passed from Booter.\n")

	/* Start setting up the virtual memory spaces */

/* initialize source/destination control registers for movs */
| ALICE LAK: The next three lines set up the movs instructions.  The value
| of FC_USERD is 1, and the actual address of the user space
| is set later, probably on every task switch:
	moveq	#FC_USERD,d0		| user space
	movc	d0,sfc			|   as source
	movc	d0,dfc			|   and destination of transfers

/*
 * Allocate kernel segment/page table resources.
 *	a4 contains the PA/VA of first available page throughout boot
 *	   (since we assume a zero load point, it is also the size of
 *	   allocated space at any time)
 * We assume (i.e. do not check) that the initial page table size
 * (Sysptsize) is big enough to map everything we allocate here.
 */
	.globl	_Sysseg, _Sysmap, _Sysptmap, _Sysptsize
| ALICE the next five lines load the first page after the kernel into a4
	movl	_esym,a4		| end of static kernel text/data
	addl	#NBPG-1,a4		| number of bytes per page
	movl	a4,d0			| cant andl with address reg
	andl	#PG_FRAME,d0		| round to a page
	movl	d0,a4
/* allocate kernel segment table */
| ALICE store addr of system segment table (first virtual page after
|  kernel in Sysseg), and push physical address onto stack
	movl	a4,_Sysseg		| remember for pmap module
	movl	a4,sp@-			| remember for loading MMU
	addl	#NBPG,a4
/* allocate initial page table pages (including internal IO map) */
/* LAK: Sysptsize is initialized at 2 in pmap.c (from param.h)   */
/* The IO map size and NuBus map size are defined in cpu.h.      */
	movl	_Sysptsize,d0		| initial system PT size (pages)
	addl	#(IIOMAPSIZE+NPTEPG-1)/NPTEPG+(NBMAPSIZE+NPTEPG-1)/NPTEPG,d0
					| add pages for IO maps and NB maps
	movl	#PGSHIFT,d1
	lsll	d1,d0			| convert to bytes
	movl	a4,sp@-			| remember for ST load
	addl	d0,a4
/* allocate kernel page table map */
	movl	a4,_Sysptmap		| remember for pmap module
	movl	a4,sp@-			| remember for PT map load
	addl	#NBPG,a4
/* compute KVA of Sysptmap; mapped after page table pages */
/* LAK:  There seems to be some confusion here about the next five lines,
so I'll explain.  The kernel needs some way of dynamically modifying
the page tables for its own virtual memory.  What it does is that it
has a page table map.  This page table map is mapped right after the
kernel itself (in our implementation; in HP's it was after the I/O space).
Therefore, the first three (or so) entries in the segment table point
to the first three pages of the page tables (which point to the kernel)
and the next entry in the segment table points to the page table map
(this is done later).  Therefore, the value of the pointer "Sysmap"
will be something like 4M*3 = 12M.  When the kernel addresses this
pointer (e.g., Sysmap[0]), it will get the first longword of the first
page map.  Since the page map mirrors the segment table, addressing any
index of Sysmap will give you a PTE of the page maps which map the kernel. */
	movl	d0,d2			| remember PT size (bytes)
	movl	_Sysptsize,d0		| # of pages for kernel
	moveq	#SG_ISHIFT,d1		| * 4megs per page
	lsll	d1,d0			| page table size serves as seg index
	movl	d0,_Sysmap		| remember VA for pmap module
/* initialize ST and PT map: PT pages + PT map */
	movl	sp@+,a1			| PT map (Kernel page table map)
	movl	sp@+,d4			| start of PT pages
	movl	sp@+,a0			| ST addr (Kern segment table map)
	lea	a0@(NBPG-4),a2		| (almost) end of ST
	addl	_load_addr,d4		| we want physical address
	movl	d4,d3
	orl	#SG_RW+SG_V,d4		| create proto STE for ST
	orl	#PG_RW+PG_CI+PG_V,d3	| create proto PTE for PT map

/* ALICE LAK 6/27/92: The next two loops (which have been #ifdefed out)
used to map all of the page tables (which had previously been allocated)
linearly.  This is bad.  This would mean that the IO space (both internal
and Nubus) would be mapped right after the kernel space.  Since we would
prefer to have the IO space mapped at PA=VA, we must skip most of the
segment table.  So we allocate the first 2 (or whatever) entries in
the segment table for the kernel page tables, then the rest are IO
space.  Does any of this make sense or am I completely insane? */

#ifdef IO_MAP_RIGHT_AFTER_KERNEL
List1:
	movl	d4,a0@+
	movl	d3,a1@+
	addl	#NBPG,d4
	addl	#NBPG,d3
	cmpl	a4,d4			| sleezy, but works ok
	jcs	List1
/* initialize ST and PT map: invalidate up to last entry */
List2:
	movl	#SG_NV,a0@+
	movl	#PG_NV,a1@+
	cmpl	a2,a0
	jcs	List2
#else
	movl	a0,sp@-			| save start of ST
	movl	a1,sp@-			| save start of PM
	movl	_Sysptsize,d0		| initial system PT size (pages)
List1:
	movl	d4,a0@+			| Fill kernel part of ST + PM
	movl	d3,a1@+
	addl	#NBPG,d4
	addl	#NBPG,d3
	subl	#1,d0
	bne	List1
/* The original HP code mapped the system page table map along with every
thing else.  Since we do it seperately, we must map it here: */
	movl	_Sysptmap,d0		| Logical address of the map
	addl	_load_addr,d0		| we want the physical address
	andl	#SG_FRAME,d0
	orl	#SG_RW+SG_V,d0
	movl	d0,a0@+			| Right after kernel in ST
	andl	#PG_FRAME,d0
	orl	#PG_RW+PG_CI+PG_V,d0
	movl	d0,a1@+			| Right after kernel in map
/* LAK: a2 was initialized up there as being the last entry in the ST */
List2:
	movl	#SG_NV,a0@+		| Invalidate rest of ST + PM
	movl	#PG_NV,a1@+
	cmpl	a2,a0
	jcs	List2

	movl	a1,a3			| Save for later

	movl	sp@,a1			| Get start of PM
	movl	sp@(4),a0		| and ST
	movl	_IOBase,d1		| Find offset to IO space
	movl	#SG_ISHIFT,d0
	lsrl	d0,d1			| Find which segment it is (/4M)
	lsll	#2,d1			| 4 bytes per PTE
	addl	d1,a1
	addl	d1,a0
	movl	#(IIOMAPSIZE+NPTEPG-1)/NPTEPG,d0	| How many PT
List3:					| map internal IO space
	movl	d4,a0@+			| d3 and d4 are still correct
	movl	d3,a1@+			| Really. I swear to God.
	addl	#NBPG,d4
	addl	#NBPG,d3
	subl	#1,d0
	bne	List3

	movl	sp@+,a1			| Get start of PM
	movl	sp@+,a0			| and ST
	movl	#NBBASE,d1		| Find offset to start of Nubus
	movl	#SG_ISHIFT,d0
	lsrl	d0,d1			| Find which segment it is (/4M)
	lsll	#2,d1			| 4 bytes per PTE
	addl	d1,a1
	addl	d1,a0
	movl	#(NBMAPSIZE+NPTEPG-1)/NPTEPG,d0 | How many PT
List4:					| map Nubus space
	movl	d4,a0@+
	movl	d3,a1@+
	addl	#NBPG,d4
	addl	#NBPG,d3
	subl	#1,d0
	bne	List4
	movl	a2,a0			| a0 is now last entry in ST
	movl	a3,a1			| a1 is now last entry in PM
#endif
/*
 * Portions of the last segment of KVA space (0xFFF00000 - 0xFFFFFFFF)
 * are mapped for a couple of purposes. 0xFFF00000 for UPAGES is used
 * for mapping the current process u-area (u + kernel stack).  The
 * very last page (0xFFFFF000) is mapped to the last physical page of
 * RAM to give us a region in which PA == VA.  We use this page for
 * enabling/disabling mapping.
 *
 * ALICE: The above has been changed.  Since we load the kernel at 0, we
 * don't need all this garbage.
 */
	movl	a4,d1			| grab next available for PT page
	addl	_load_addr, d1		| we want the physical address
	andl	#SG_FRAME,d1		| mask to frame number
	orl	#SG_RW+SG_V,d1		| RW and valid
	movl	d1,a0@+			| store in last ST entry
	movl	a0,a2			| remember addr for PT load
	andl	#PG_FRAME,d1
	orl	#PG_RW+PG_V,d1		| convert to PTE
	movl	d1,a1@+			| store in PT map
	movl	a4,a0			| physical beginning of PT page
	lea	a0@(NBPG),a1		| end of page
Lispt7:
	movl	#PG_NV,a0@+		| invalidate all of it
	cmpl	a1,a0
	jcs	Lispt7

	addl	#NBPG,a4		| Skip over used PT

/* record KVA at which to access current u-area PTEs */
	movl	_Sysmap,d0		| get system PT address
	addl	#NPTEPG*NBPG,d0		| end of system PT
	subl	#HIGHPAGES*4,d0		| back up to first PTE for u-area
	movl	d0,_Umap		| remember location
/* initialize page table pages */
	movl	a2,a0			| end of ST is start of PT
	movl	a0,sp@-			| store that for later
	addl	d2,a2			| add size to get end of PT
/* text pages are read-only */
	movl	_load_addr,d1		| get load address
	orl	#PG_RW+PG_V,d1		| XXX: RW for now
	movl	#_etext,a1		| go til end of text
	addl	_load_addr,a1		| we want the physical address
	movl	d1, a0@+		| low page of memory is read-write
					|       (ack...!)
	addl	#NBPG,d1		| increment page frame number
#if !defined(KGDB)
	orl	#PG_RO,d1		| XXX: RW for now
#endif
Lipt1:
	movl	d1,a0@+			| load PTE
	addl	#NBPG,d1		| increment page frame number
	cmpl	a1,d1			| done yet?
	jcs	Lipt1			| no, keep going
/* data, bss and dynamic tables are read/write */
	andl	#PG_FRAME,d1		| mask out old prot bits
	orl	#PG_RW+PG_V,d1		| mark as valid and RW
	movl	a4,a1			| go til end of data allocated so far
	addl	#(UPAGES+1)*NBPG,a1	| and proc0 PT/u-area (to be allocated)
	addl	_load_addr,a1		| we want the physical address
Lipt2:
	movl	d1,a0@+			| load PTE
	addl	#NBPG,d1		| increment page frame number
	cmpl	a1,d1			| done yet?
	jcs	Lipt2			| no, keep going
/* invalidate remainder of kernel PT */
	movl	a2,a1			| end of PT
Lipt3:
	movl	#PG_NV,a0@+		| invalidate PTE
	cmpl	a1,a0			| done yet?
	jcs	Lipt3			| no, keep going
/* go back and validate internal IO PTEs at end of allocated PT space */
/* LAK: Initialize internal IO PTE in kernel PT */
	movl	_Sysptsize,d7		| initial system PT size (pages)
	movl	#PGSHIFT,d1
	lsll	d1,d7			| # of byte of system PT
	addl	sp@,d7			| + start of kernel PT = start of IO
	movl	d7,a0
	movl	a0,a2			| ptr to end of PTEs to init
	addl	#IIOMAPSIZE*4,a2		| # of PTE to initialize
	movl	#INTIOBASE,d1		| Start of IO space
	andl	#PG_FRAME,d1		| Round to a page frame
	orl	#PG_RW+PG_CI+PG_V,d1	| create proto PTE
Lipt4:
	movl	d1,a0@+			| load PTE
	addl	#NBPG,d1		| increment page frame number
	cmpl	a2,a0			| done yet?
	jcs	Lipt4			| no, keep going
/* record base KVA of IO spaces (they are mapped PA == VA) */
	movl	#NBBASE,d0		| base of NuBus
	movl	d0,_NuBusBase		| and record
	| BARF: intiolimit is wrong:
	movl	d0,_intiolimit		| external base is also internal limit

/* LAK: Initialize external IO PTE in kernel PT (this is the nubus space) */
	movl	_Sysptsize,d0		| initial system PT size (pages)
	addl	#(IIOMAPSIZE+NPTEPG-1)/NPTEPG,d0 | start of nubus PT
	movl	#PGSHIFT,d1		| PT to bytes
	lsll	d1,d0			| # of page tables
	movl	d0,a0
	addl	sp@,a0			| + start of kernel PT = start of IO
	movl	a0,a2			| ptr to end of PTEs to init
	addl	#NBMAPSIZE*4,a2		| # of PTE to initialize
	movl	#NBBASE,d1		| Start of IO space
	andl	#PG_FRAME,d1		| Round to a page frame
	orl	#PG_RW+PG_CI+PG_V,d1	| create proto PTE
Lipt5:
	movl	d1,a0@+			| load PTE
	addl	#NBPG,d1		| increment page frame number
	cmpl	a2,a0			| done yet?
	jcs	Lipt5			| no, keep going

#ifdef MACHINE_NONCONTIG
	/*
	 * LAK: (7/30/94) If the MMU was on when we booted and we're on
	 *  an '030, then we have information about how MacOS had mapped
	 *  the NuBus space.  This function re-maps NuBus the same way
	 *  so we can use internal video.  At the same time, it remaps
	 *  the kernel pages in case we were split between banks (e.g.,
	 *  on a IIsi with internal video plugged out).
	 *
	 *  This is the first function in this part of locore that we
	 *  took out to C instead of writing in assembly.  I hope we
	 *  can take more out, cause all this stuff is pretty unreadable.
	 */

	movl	sp@,a0			| can I do this in one step?
	movl	a0,sp@-			| address of start of kernel PT
	movl	_Sysseg, a0
	movl	a0,sp@-			| address of start of kernel ST
	jbsr	_remap_MMU
	addl	#8,sp			| pop start of PT & ST addresses
#endif /* MACHINE_NONCONTIG */
	addl	#4,sp			| pop start of PT address

/*
 * Setup page table for process 0.
 *
 * We set up page table access for the kernel via Usrptmap (usrpt)
 * and access to the u-area itself via Umap (u).  First available
 * page (A4) is used for proc0 page table.  Next UPAGES pages following
 * are for u-area.
 */
	movl	a4,d0
	movl	d0,d1
	addl	_load_addr,d1		| we want physical address
	andl	#PG_FRAME,d1		| mask to page frame number
	orl	#PG_RW+PG_V,d1		| RW and valid
	movl	d1,d4			| remember for later Usrptmap load
	movl	d0,a0			| base of proc0 PT
	addl	#NBPG,d0		| plus one page yields base of u-area
	movl	d0,a2			|   and end of PT
/* invalidate entire page table */
Liudot1:
	movl	#PG_NV,a0@+		| invalidate PTE
	cmpl	a2,a0			| done yet?
	jcs	Liudot1			| no, keep going
/* now go back and validate u-area PTEs in PT and in Umap */
	lea	a0@(-HIGHPAGES*4),a0	| base of PTEs for u-area (p_addr)
	lea	a0@(UPAGES*4),a1	| end of PTEs for u-area
	lea	a4@(-HIGHPAGES*4),a3	| u-area PTE base in Umap PT
	movl	d0,d1			| get base of u-area
	addl	_load_addr,d1		| we want physical address
	andl	#PG_FRAME,d1		| mask to page frame number
	orl	#PG_RW+PG_V,d1		| add valid and writable
Liudot2:
	movl	d1,a0@+			| validate p_addr PTE
	movl	d1,a3@+			| validate u PTE
	addl	#NBPG,d1		| to next page
	cmpl	a1,a0			| done yet?
	jcs	Liudot2			| no, keep going
	movl	a0,_proc0paddr		| save address of proc0 u-area
/* clear process 0 u-area */
	addl	#NBPG*UPAGES,d0		| end of u-area
Lclru1:
	clrl	a2@+			| clear
	cmpl	d0,a2			| done yet?
	jcs	Lclru1			| no, keep going
	movl	a2,a4			| save phys addr of first avail page

	.globl	_dump_pmaps
|	jbsr	_dump_pmaps

	debug_mark("DEBUG: preparing new MMU context.\n")
/*
 * Prepare to enable MMU.
 */
	lea	_protorp,a0
	movl	_Sysseg,a1		| system segment table addr
	addl	_load_addr,a1		| we want physical address
	movl	#0x80000202,a0@		| nolimit + share global + 4 byte PTEs
	movl	a1,a0@(4)		| + segtable address
	pmove	a0@,srp			| load the supervisor root pointer
	pflusha
	movl	#0x80000002,a0@		| reinit upper half for CRP loads

| LAK: Kill the TT0 and TT1 registers so the don't screw us up later.
	tstl	_mmutype		| ttx instructions will break 68851
	jgt	LnokillTT
	debug_mark("DEBUG: clearing TT registers...\n")
	lea	longscratch,a0
	movl	#0, a0@
	.long	0xF0100800		| movl a0@,tt0
	.long	0xF0100C00		| movl a0@,tt1
LnokillTT:

/* BARF: A line which was here enabled the FPE and i-cache */
/* LAK: a2 is at a location we can clobber: */
	debug_mark("DEBUG: about to set MMU control (hold your breath).\n")
	movl	#0x82c0aa00,a2@		| value to load TC with
	pmove	a2@,tc			| load it

	debug_mark("DEBUG: yay!  MMU enabled with new mapping.\n")

	pflusha				| make sure it's clean

	debug_mark("DEBUG: MMU flushed.\n")

#if defined (MACHINE_NONCONTIG)
#if 0
	jmp	foobar1
#endif
foobar2:
#endif	/* MACHINE_NONCONTIG */
/*
 * Should be running mapped from this point on
 */
/* init mem sizes */
/*
 * XXX LAK: we should get rid of maxmem and physmem entirely now that
 * we use noncontig.
 */
	movl	lastpage,d1		| last page of ram from MacOS
	moveq	#PGSHIFT,d2
	lsrl	d2,d1			| convert to page (click) number
	movl	d1,_maxmem		| save as maxmem
	movl	d1,_physmem		| and physmem
/*
 * pmap_bootstrap is supposed to be called with mapping off early on
 * to set up the kernel VA space.  However, this only works easily if
 * you have a kernel PA == VA mapping.  Since we do not, we just set
 * up and enable mapping here and then call the bootstrap routine to
 * get the pmap module in sync with reality.
 *
 * LAK: We do have PA == VA, but we'll leave things the way they
 *      are for now.
 */
	debug_mark("DEBUG: calling pmap_bootstrap() ...\n")
	lea	tmpstk,sp		| temporary stack
	movl	_load_addr,sp@-		| phys load address
	addl	_load_addr,a4		| need physical address
	movl	a4,sp@-			| first available PA
	jbsr	_pmap_bootstrap		| sync up pmap module
	addql	#8,sp
	debug_mark("DEBUG: done.\n")

Ldoproc0:				| The 040 comes back here...
/* set kernel stack, user SP, and initial pcb */
	lea	_kstack,a1		| proc0 kernel stack
	lea	a1@(UPAGES*NBPG-4),sp	| set kernel stack to end of area
	jbsr	_TBIA			| invalidate TLB
	movl	#USRSTACK-4,a2
	movl	a2,usp			| init user SP
	movl	_proc0paddr,a1		| get proc0 pcb addr
	movl	a1,_curpcb		| proc0 is running
	clrw	a1@(PCB_FLAGS)		| clear flags
/* flush TLB and turn on caches */
	debug_mark("DEBUG: invalidating TLB.\n")
	jbsr	_TBIA			| invalidate TLB
	debug_mark("DEBUG: activating cache(s).\n")
	movl	#CACHE_ON,d0
	movc	d0,cacr			| clear cache(s)
/* BARF: Enable external cache here */
/* final setup for C code */
	debug_mark("DEBUG: calling setmachdep().\n")
	jbsr	_setmachdep		| Set some machine-dep stuff
	debug_mark("DEBUG: done.\n")
	debug_mark("DEBUG: enabling interrupts.\n")
	movl	#0,a6			| LAK: so that stack_trace() works
	movw	#PSL_LOWIPL,sr		| lower SPL ; enable interrupts
	debug_mark("DEBUG: done.\n")

	debug_mark("DEBUG: away we go!  (calling _main)\n")
	clrw	sp@-			| vector offset/frame type
	clrl	sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,sp@-		| in user mode
	clrl	sp@-			| stack adjust count
	lea	sp@(-64),sp		| construct space for D0-D7/A0-A7
	pea	sp@			| addr of space for D0
	jbsr	_main			| call main(&framespace)
	addql	#4,sp			| pop args
	tstl	_cpu040			| 040?
	jeq	Lnoflush		| no, skip
	.word	0xf478			| cpusha dc
	.word	0xf498			| cinva ic
Lnoflush:
	movl	sp@(FR_SP),a0		| grab and load
	movl	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| load most registers (all but SSP)
	addql	#8,sp			| pop SSP and stack adjust count
	rte

/*
 * Icode is copied out to process 1 to exec init.
 * If the exec fails, process 1 exits.  Nabbed from amiga
 */
	.globl	_icode,_szicode
	.text
_icode:
	jra	st1
init:
	.asciz	"/sbin/init"
	.byte	0
argv:
	.long	init+6-_icode		| argv[0] = "init" ("/sbin/init" + 6)
	.long	eicode-_icode		| argv[1] follows icode after copyout
	.long	0
st1:	clrl	sp@-
	.set	argvrpc,argv-.-2	| XXX-should include amiga comments.
	pea	pc@(argvrpc)
	.set	initrpc,init-.-2
	pea	pc@(initrpc)
	clrl	sp@-
	moveq	#SYS_execve,d0
	trap	#0
	moveq	#SYS_exit,d0
	trap	#0
eicode:

_szicode:
	.long	_szicode-_icode

/*
 * Signal "trampoline" code (18 bytes).  Invoked from RTE setup by sendsig().
 * 
 * Stack looks like:
 *
 *	sp+0 ->	signal number
 *	sp+4	signal specific code
 *	sp+8	pointer to signal context frame (scp)
 *	sp+12	address of handler
 *	sp+16	saved hardware state
 *			.
 *			.
 *	scp+0->	beginning of signal context frame
 */
	.globl	_sigcode, _esigcode
	.data
	.align	2
_sigcode:
	movl	sp@(12),a0		| signal handler addr	(4 bytes)
	jsr	a0@			| call signal handler	(2 bytes)
	addql	#4,sp			| pop signo		(2 bytes)
	trap	#1			| special syscall entry	(2 bytes)
	movl	d0,sp@(4)		| save errno		(4 bytes)
	moveq	#1,d0			| syscall == exit	(2 bytes)
	trap	#0			| exit(errno)		(2 bytes)
	.align	2
_esigcode:

/*
 * Primitives
 */ 

#include "m68k/asm.h"

/*
 * non-local gotos
 */
ENTRY(setjmp)
	movl	sp@(4),a0	| savearea pointer
	moveml	#0xFCFC,a0@	| save d2-d7/a2-a7
	movl	sp@,a0@(48)	| and return address
	moveq	#0,d0		| return 0
	rts

ENTRY(qsetjmp)
	movl	sp@(4),a0	| savearea pointer
	lea	a0@(40),a0	| skip regs we do not save
	movl	a6,a0@+		| save FP
	movl	sp,a0@+		| save SP
	movl	sp@,a0@		| and return address
	moveq	#0,d0		| return 0
	rts

ENTRY(longjmp)
	movl	sp@(4),a0
	moveml	a0@+,#0xFCFC
	movl	a0@,sp@
	moveq	#1,d0
	rts

/*
 * The following primitives manipulate the run queues.
 * _whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_priority, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */

	.globl	_whichqs,_qs,_cnt,_panic
	.globl	_curproc
	.comm	_want_resched,4

/*
 * setrunqueue(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrunqueue)
	movl	sp@(4),a0
	tstl	a0@(P_BACK)
	jeq	Lset1
	movl	#Lset2,sp@-
	jbsr	_panic
Lset1:
	clrl	d0
	movb	a0@(P_PRIORITY),d0
	lsrb	#2,d0
	movl	_whichqs,d1
	bset	d0,d1
	movl	d1,_whichqs
	lslb	#3,d0
	addl	#_qs,d0
	movl	d0,a0@(P_FORW)
	movl	d0,a1
	movl	a1@(P_BACK),a0@(P_BACK)
	movl	a0,a1@(P_BACK)
	movl	a0@(P_BACK),a1
	movl	a0,a1@(P_FORW)
	rts

Lset2:
	.asciz	"setrunqueue"
Lrem4:
	.asciz	"remrq : p NULL"
	.even

/*
 * Remrq(proc *p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrq)
	movl	sp@(4),a0		| proc *p
	clrl	d0
	movb	a0@(P_PRIORITY),d0	| d0 = processes priority
	lsrb	#2,d0			| d0 /= 4
	movl	_whichqs,d1		| d1 = whichqs
	bclr	d0,d1			| clear bit in whichqs corresponding to
					|  processes priority
	jne	Lrem1			| if (d1 & (1 << d0)) == 0
	movl	#Lrem3,sp@-		| (if that queue is empty)
	jbsr	_panic			| panic("remrq")
Lrem1:
	movl	d1,_whichqs
	movl	a0@(P_FORW),a1
	movl	a0@(P_BACK),a1@(P_BACK)
	movl	a0@(P_BACK),a1
	movl	a0@(P_FORW),a1@(P_FORW)
	movl	#_qs,a1
	movl	d0,d1
	lslb	#3,d1
	addl	d1,a1
	cmpl	a1@(P_FORW),a1
	jeq	Lrem2
	movl	_whichqs,d1
	bset	d0,d1
	movl	d1,_whichqs
Lrem2:
	clrl	a0@(P_BACK)
	rts

Lrem3:
	.asciz	"remrq"
Lsw0:
	.asciz	"cpu_switch"
	.even

	.globl	_curpcb
	.globl	_masterpaddr	| XXX compatibility (debuggers)
	.data
_masterpaddr:			| XXX compatibility (debuggers)
_curpcb:
	.long	0
pcbflag:
	.byte	0		| copy of pcb_flags low byte
	.align	2
	.comm	nullpcb,SIZEOF_PCB
	.text

/*
 * At exit of a process, do a cpu_switch for the last time.
 * The mapping of the pcb at p->p_addr has already been deleted,
 * and the memory for the pcb+stack has been freed.
 * The ipl is high enough to prevent the memory from being reallocated.
 */
ENTRY(switch_exit)
	movl	#nullpcb,_curpcb	| save state into garbage pcb
	lea	tmpstk,sp		| goto a tmp stack
	jra	_cpu_switch		| XXX LK: Should this be _mi_switch?

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
	.globl	Idle
Lidle:
	stop	#PSL_LOWIPL
Idle:
idle:
	movw	#PSL_HIGHIPL,sr
	tstl	_whichqs
	jeq	Lidle
	movw	#PSL_LOWIPL,sr
	jra	Lsw1

Lbadsw:
	movl	#Lsw0,sp@-
	jbsr	_panic
	/*NOTREACHED*/

/*
 * cpu_switch()
 *
 * NOTE: On the mc68851 (318/319/330) we attempt to avoid flushing the
 * entire ATC.  The effort involved in selective flushing may not be
 * worth it, maybe we should just flush the whole thing?
 *
 * NOTE 2: With the new VM layout we now no longer know if an inactive
 * user's PTEs have been changed (formerly denoted by the SPTECHG p_flag
 * bit).  For now, we just always flush the full ATC.
 */
ENTRY(cpu_switch)
	movl	_curpcb,a0		| current pcb
	movw	sr,a0@(PCB_PS)		| save sr before changing ipl
#ifdef notyet
	movl	_curproc,sp@-		| remember last proc running
#endif /* notyet */
	clrl	_curproc
Lsw1:
	/*
	 * Find the highest-priority queue that isn't empty,
	 * then take the first proc from that queue.
	 */
	clrl	d0
	lea	_whichqs,a0
	movl	a0@,d1
Lswchk:
	btst	d0,d1
	jne	Lswfnd
	addqb	#1,d0
	cmpb	#32,d0
	jne	Lswchk
	jra	idle
Lswfnd:
|	movw	#PSL_HIGHIPL,sr		| lock out interrupts
	movw	#0x2300,sr		| lock out clock and scsi, but not tty.
	movl	a0@,d1			| and check again...
	bclr	d0,d1
	jeq	Lsw1			| proc moved, rescan
	movl	d1,a0@			| update whichqs
	moveq	#1,d1			| double check for higher priority
	lsll	d0,d1			| process (which may have snuck in
	subql	#1,d1			| while we were finding this one)
	andl	a0@,d1
	jeq	Lswok			| no one got in, continue
	movl	a0@,d1
	bset	d0,d1			| otherwise put this one back
	movl	d1,a0@
	jra	Lsw1			| and rescan
Lswok:
	movl	d0,d1
	lslb	#3,d1			| convert queue number to index
	addl	#_qs,d1			| locate queue (q)
	movl	d1,a1
	cmpl	a1@(P_FORW),a1		| anyone on queue?
	jeq	Lbadsw			| no, panic
	movl	a1@(P_FORW),a0		| p = q->p_forw
	movl	a0@(P_FORW),a1@(P_FORW)	| q->p_forw = p->p_forw
	movl	a0@(P_FORW),a1		| q = p->p_forw
	movl	a0@(P_BACK),a1@(P_BACK)	| q->p_back = p->p_back
	cmpl	a0@(P_FORW),d1		| anyone left on queue?
	jeq	Lsw2			| no, skip
	movl	_whichqs,d1
	bset	d0,d1			| yes, reset bit
	movl	d1,_whichqs
Lsw2:
	movl	a0,_curproc
	clrl	_want_resched
#ifdef notyet
	movl	sp@+,a1
	cmpl	a0,a1			| switching to same proc?
	jeq	Lswdone			| yes, skip save and restore
#endif /* notyet */
	/*
	 * Save state of previous process in its pcb.
	 */
	movl	_curpcb,a1
	moveml	#0xFCFC,a1@(PCB_REGS)	| save non-scratch registers
	movl	usp,a2			| grab USP (a2 has been saved)
	movl	a2,a1@(PCB_USP)		| and save it
	movl	_CMAP2,a1@(PCB_CMAP2)	| save temporary map PTE

	tstl	_fpu_type		| Do we have an fpu?
	jeq	Lswnofpsave		| No?  Then don't attempt save.
	lea	a1@(PCB_FPCTX),a2	| pointer to FP save area
	fsave	a2@			| save FP state
	tstb	a2@			| null state frame?
	jeq	Lswnofpsave		| yes, all done
	fmovem	fp0-fp7,a2@(216)	| save FP general registers
	fmovem	fpcr/fpsr/fpi,a2@(312)	| save FP control registers
Lswnofpsave:

#ifdef DIAGNOSTIC
	tstl	a0@(P_WCHAN)
	jne	Lbadsw
	cmpb	#SRUN,a0@(P_STAT)
	jne	Lbadsw
#endif /* DIAGNOSTIC */
	clrl	a0@(P_BACK)		| clear back link
	movl	a0@(P_ADDR),a1		| get p_addr
	movl	a1,_curpcb
	movb	a1@(PCB_FLAGS+1),pcbflag | copy of pcb_flags low byte

	/* see if pmap_activate needs to be called; should remove this */
	movl	a0@(P_VMSPACE),a0	| vmspace = p->p_vmspace
#ifdef DIAGNOSTIC
	tstl	a0			| map == VM_MAP_NULL?
	jeq	Lbadsw			| panic
#endif /* DIAGNOSTIC */
	lea	a0@(VM_PMAP),a0		| pmap = &vmspace.vm_pmap
	tstl	a0@(PM_STCHG)		| pmap->st_changed?
	jeq	Lswnochg		| no, skip
	pea	a1@			| push pcb (at p_addr)
	pea	a0@			| push pmap
	jbsr	_pmap_activate		| pmap_activate(pmap, pcb)
	addql	#8,sp
	movl	_curpcb,a1		| restore p_addr
Lswnochg:
	movl	#PGSHIFT,d1
	movl	a1,d0
	lsrl	d1,d0			| convert p_addr to page number
	lsll	#2,d0			| and now to Systab offset
	addl	_Sysmap,d0		| add Systab base to get PTE addr
#ifdef notdef
	movw	#PSL_HIGHIPL,sr		| go crit while changing PTEs
#endif /* notdef */
	lea	tmpstk,sp		| now goto a tmp stack for NMI
	movl	d0,a0			| address of new context
	movl	_Umap,a2		| address of PTEs for kstack
	moveq	#UPAGES-1,d0		| sizeof kstack
Lres1:
	movl	a0@+,d1			| get PTE
	andl	#~PG_PROT,d1		| mask out old protection
	orl	#PG_RW+PG_V,d1		| ensure valid and writable
	movl	d1,a2@+			| load it up
	dbf	d0,Lres1		| til done
	tstl	_cpu040
	jne	Lres2
	movl	#CACHE_CLR,d0
	movc	d0,cacr			| invalidate cache(s)
	pflusha				| flush entire TLB
	jra	Lres3
Lres2:
|	.word	0xf4f8			| cpusha bc
	.word	0xf518			| pflusha (68040)
	movl	#CACHE40_ON,d0
	movc	d0, cacr		| invalidate caches
Lres3:
	movl	a1@(PCB_USTP),d0	| get USTP
	moveq	#PGSHIFT,d1
	lsll	d1,d0			| convert to addr
	tstl	_cpu040
	jne	Lres4
	lea	_protorp,a0		| CRP prototype
	movl	d0,a0@(4)		| stash USTP
	pmove	a0@,crp			| load new user root pointer
	jra	Lres5
Lres4:
	.word	0x4e7b, 0x0806		| movc d0, URP
Lres5:
	movl	a1@(PCB_CMAP2),_CMAP2	| reload tmp map
	moveml	a1@(PCB_REGS),#0xFCFC	| and registers
	movl	a1@(PCB_USP),a0
	movl	a0,usp			| and USP

	tstl	_fpu_type		| If we don't have an fpu,
	jeq	Lresfprest		|  don't try to restore it.
	lea	a1@(PCB_FPCTX),a0	| pointer to FP save area
	tstb	a0@			| null state frame?
	jeq	Lresfprest		| yes, easy
	fmovem	a0@(312),fpcr/fpsr/fpi	| restore FP control registers
	fmovem	a0@(216),fp0-fp7	| restore FP general registers
Lresfprest:
	frestore a0@			| restore state

	movw	a1@(PCB_PS),sr		| no, restore PS
	moveq	#1,d0			| return 1 (for alternate returns)
	rts

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in swtch if altreturn is true.
 */
ENTRY(savectx)
	movl	sp@(4),a1
	movw	sr,a1@(PCB_PS)
	movl	usp,a0			| grab USP
	movl	a0,a1@(PCB_USP)		| and save it
	moveml	#0xFCFC,a1@(PCB_REGS)	| save non-scratch registers
	movl	_CMAP2,a1@(PCB_CMAP2)	| save temporary map PTE

	tstl	_fpu_type		| Do we have FPU?
	jeq	Lsvnofpsave		| No?  Then don't save state.
	lea	a1@(PCB_FPCTX),a0	| pointer to FP save area
	fsave	a0@			| save FP state
	tstb	a0@			| null state frame?
	jeq	Lsvnofpsave		| yes, all done
	fmovem	fp0-fp7,a0@(216)	| save FP general registers
	fmovem	fpcr/fpsr/fpi,a0@(312)	| save FP control registers
Lsvnofpsave:
	tstl	sp@(8)			| altreturn?
	jeq	Lsavedone
	movl	sp,d0			| relocate current sp relative to a1
	subl	#_kstack,d0		|   (sp is relative to kstack):
	addl	d0,a1			|   a1 += sp - kstack;
	movl	sp@,a1@			| write return pc at (relocated) sp@
Lsavedone:
	moveq	#0,d0			| return 0
	rts

/*
 * Copy 1 relocation unit (NBPG bytes)
 * from user virtual address to physical address
 */
ENTRY(copyseg)
	movl	_curpcb,a1			| current pcb
	movl	#Lcpydone,a1@(PCB_ONFAULT)	| where to return to on a fault
	movl	sp@(8),d0			| destination page number
	moveq	#PGSHIFT,d1
	lsll	d1,d0				| convert to address
	orl	#PG_CI+PG_RW+PG_V,d0		| make sure valid and writable
	movl	_CMAP2,a0
	movl	_CADDR2,sp@-			| destination kernel VA
	movl	d0,a0@				| load in page table
	jbsr	_TBIS				| invalidate any old mapping
	addql	#4,sp
	movl	_CADDR2,a1			| destination addr
	movl	sp@(4),a0			| source addr
	movl	#NBPG/4-1,d0			| count
Lcpyloop:
	movsl	a0@+,d1				| read longword
	movl	d1,a1@+				| write longword
	dbf	d0,Lcpyloop			| continue until done
Lcpydone:
	movl	_curpcb,a1			| current pcb
	clrl	a1@(PCB_ONFAULT) 		| clear error catch
	rts

/*
 * Copy 1 relocation unit (NBPG bytes)
 * from physical address to physical address
 */
ENTRY(physcopyseg)
	movl	sp@(4),d0			| source page number
	moveq	#PGSHIFT,d1
	lsll	d1,d0				| convert to address
	orl	#PG_CI+PG_RW+PG_V,d0		| make sure valid and writable
	movl	_CMAP1,a0
	movl	d0,a0@				| load in page table
	movl	_CADDR1,sp@-			| destination kernel VA
	jbsr	_TBIS				| invalidate any old mapping
	addql	#4,sp

	movl	sp@(8),d0			| destination page number
	moveq	#PGSHIFT,d1
	lsll	d1,d0				| convert to address
	orl	#PG_CI+PG_RW+PG_V,d0		| make sure valid and writable
	movl	_CMAP2,a0
	movl	d0,a0@				| load in page table
	movl	_CADDR2,sp@-			| destination kernel VA
	jbsr	_TBIS				| invalidate any old mapping
	addql	#4,sp

	movl	_CADDR1,a0			| source addr
	movl	_CADDR2,a1			| destination addr
	movl	#NBPG/4-1,d0			| count
Lpcpy:
	movl	a0@+,a1@+			| copy longword
	dbf	d0,Lpcpy			| continue until done
	rts

/*
 * zero out physical memory
 * specified in relocation units (NBPG bytes)
 */
ENTRY(clearseg)
	movl	sp@(4),d0			| destination page number
	moveq	#PGSHIFT,d1
	lsll	d1,d0				| convert to address
	orl	#PG_CI+PG_RW+PG_V,d0		| make sure valid and writable
	movl	_CMAP1,a0
	movl	_CADDR1,sp@-			| destination kernel VA
	movl	d0,a0@				| load in page map
	jbsr	_TBIS				| invalidate any old mapping
	addql	#4,sp
	movl	_CADDR1,a1			| destination addr
	movl	#NBPG/4-1,d0			| count
/* simple clear loop is fastest on 68020 */
Lclrloop:
	clrl	a1@+				| clear a longword
	dbf	d0,Lclrloop			| continue til done
	rts

/*
 * Invalidate entire TLB.
 */
ENTRY(TBIA)
__TBIA:
	tstl	_cpu040
	jne	Ltbia040
	pflusha				| flush entire TLB
	tstl	_mmutype
	jgt	Ltbia851
	movl	#DC_CLEAR,d0
	movc	d0,cacr			| invalidate on-chip d-cache
Ltbia851:
	rts
Ltbia040:
	.word	0xf518		| pflusha
|	.word	0xf478		| cpush dc [ cinv or cpush ??]
	rts

/*
 * Invalidate any TLB entry for given VA (TB Invalidate Single)
 */
ENTRY(TBIS)
#ifdef DEBUG
	tstl	fulltflush		| being conservative?
	jne	__TBIA			| yes, flush entire TLB
#endif
	movl	sp@(4),a0		| get addr to flush
	tstl	_cpu040
	jne	Ltbis040
	tstl	_mmutype
	jgt	Ltbis851
	pflush	#0,#0,a0@		| flush address from both sides
	movl	#DC_CLEAR,d0
	movc	d0,cacr			| invalidate on-chip data cache
	rts
Ltbis851:
	pflushs	#0,#0,a0@		| flush address from both sides
	rts
Ltbis040:
	moveq	#FC_SUPERD,d0		| select supervisor
	movc	d0, dfc
	.word	0xf508			| pflush a0@
	moveq	#FC_USERD,d0		| select user
	movc	d0, dfc
	.word	0xf508			| pflush a0@
|	.word	0xf478			| cpusha dc [cinv or cpush ??]
	rts

/*
 * Invalidate supervisor side of TLB
 */
ENTRY(TBIAS)
#ifdef DEBUG
	tstl	fulltflush		| being conservative?
	jne	__TBIA			| yes, flush everything
#endif
	tstl	_cpu040
	jne	Ltbias040
	tstl	_mmutype
	jgt	Ltbias851
	pflush	#4,#4			| flush supervisor TLB entries
	movl	#DC_CLEAR,d0
	movc	d0,cacr			| invalidate on-chip d-cache
	rts
Ltbias851:
	pflushs	#4,#4			| flush supervisor TLB entries
	rts
Ltbias040:
| 68040 can't specify supervisor/user on pflusha, so we flush all
	.word	0xf518			| pflusha
|	.word	0xf478			| cpusha dc [cinv or cpush ??]
	rts

/*
 * Invalidate user side of TLB
 */
ENTRY(TBIAU)
#ifdef DEBUG
	tstl	fulltflush		| being conservative?
	jne	__TBIA			| yes, flush everything
#endif
	tstl	_cpu040
	jne	Ltbiau040
	tstl	_mmutype
	jgt	Ltbiau851
	pflush	#0,#4			| flush user TLB entries
	movl	#DC_CLEAR,d0
	movc	d0,cacr			| invalidate on-chip d-cache
	rts
Ltbiau851:
	pflush	#0,#4			| flush user TLB entries
	rts
Ltbiau040:
| 68040 can't specify supervisor/user on pflusha, so we flush all
	.word	0xf518			| pflusha
|	.word	0xf478			| cpusha dc [cinv or cpush ??]
	rts

/*
 * Invalidate instruction cache
 */
ENTRY(ICIA)
ENTRY(ICPA)
	tstl	_cpu040
	jne	Licia040
	movl	#IC_CLEAR,d0
	movc	d0,cacr			| invalidate i-cache
	rts
Licia040:
	.word	0xf498			| cinva ic
	rts

/*
 * Invalidate data cache.
 * NOTE: we do not flush 68030 on-chip cache as there are no aliasing
 * problems with DC_WA.  The only cases we have to worry about are context
 * switch and TLB changes, both of which are handled "in-line" in resume
 * and TBI*.
 */
ENTRY(DCIA)
__DCIA:
	tstl	_cpu040
	jeq	Ldciax
	.word	0xf478		| cpusha dc
Ldciax:
	rts

ENTRY(DCIS)
__DCIS:
	tstl	_cpu040
	jeq	Ldcisx
	.word	0xf478		| cpusha dc
Ldcisx:
	rts

ENTRY(DCIU)
__DCIU:
	tstl	_cpu040
	jeq	Ldciux
	.word	0xf478		| cpusha dc
Ldciux:
	rts

| Invalid single cache line
ENTRY(DCIAS)
__DCIAS:
	tstl	_cpu040
	jeq	Ldciasx
	movl	sp@(4),a0
	.word	0xf468		| cpushl dc,a0@
Ldciasx:
	rts

#ifdef M68040
ENTRY(ICPL)	/* invalidate instruction physical cache line */
	movl	sp@(4),a0		| address
	.word	0xf488			| cinvl ic,a0@
	rts
ENTRY(ICPP)	/* invalidate instruction physical cache page */
	movl	sp@(4),a0		| address
	.word	0xf490			| cinvp ic,a0@
	rts
ENTRY(DCPL)	/* invalidate data physical cache line */
	movl	sp@(4),a0		| address
	.word	0xf448			| cinvl dc,a0@
	rts
ENTRY(DCPP)	/* invalidate data physical cache page */
	movl	sp@(4),a0		| address
	.word	0xf450			| cinvp dc,a0@
	rts
ENTRY(DCPA)	/* invalidate instruction physical cache line */
	.word	0xf458			| cinva dc
	rts
ENTRY(DCFL)	/* data cache flush line */
	movl	sp@(4),a0		| address
	.word	0xf468			| cpushl dc,a0@
	rts
ENTRY(FCFP)	/* data cache flush page */
	movl	sp@(4),a0		| address
	.word	0xf470			| cpushp dc,a0@
	rts
#endif /* M68040 */

ENTRY(PCIA)
	tstl	_cpu040
	jne	Lpcia040
	movl	#DC_CLEAR,d0
	movc	d0,cacr			| invalidate on-chip d-cache
	rts
Lpcia040:
	.word	0xf478			| cpusha dc
	rts

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Get callers current SP value.
 * Note that simply taking the address of a local variable in a C function
 * doesn't work because callee saved registers may be outside the stack frame
 * defined by A6 (e.g. GCC generated code).
 */
	.globl	_getsp
_getsp:
	movl	sp,d0			| get current SP
	addql	#4,d0			| compensate for return address
	rts

	.globl	_getsfc, _getdfc
_getsfc:
	movc	sfc,d0
	rts
_getdfc:
	movc	dfc,d0
	rts

/*
 * Check out a virtual address to see if it's okay to write to.
 *
 * probeva(va, fc)
 *
 */
ENTRY(probeva)
	movl	sp@(8),d0
	movec	d0,dfc
	movl	sp@(4),a0
	.word	0xf548		| ptestw (a0)
	moveq	#FC_USERD,d0		| restore DFC to user space
	movc	d0,dfc
	.word	0x4e7a,0x0805	| movec  MMUSR,d0
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
	pflusha				| ALICE: LAK 10/11/92 68040 says to
					| do this before loading the crp.
					| Not in original code.
	movl	sp@(4),d0		| new USTP
	moveq	#PGSHIFT,d1
	lsll	d1,d0			| convert to addr
	tstl	_cpu040
	jne	Lldustp040
	lea	_protorp,a0		| CRP prototype
	movl	d0,a0@(4)		| stash USTP
	pmove	a0@,crp			| load root pointer
	movl	#DC_CLEAR,d0
	movc	d0,cacr			| invalidate on-chip d-cache
	rts				|   since pmove flushes TLB
Lldustp040:
	.word	0xf478			| cpush dc
	.word	0x4e7b,0x0806		| movec d0, URP
	rts

/*
 * Flush any hardware context associated with given USTP.
 * Only does something for HP330 where we must flush RPT
 * and ATC entries in PMMU.
 */
ENTRY(flushustp)
	tstl	_mmutype
	jlt	Lnot68851		| Should get '030 and '040
	movl	sp@(4),d0		| get USTP to flush
	moveq	#PGSHIFT,d1
	lsll	d1,d0			| convert to address
	movl	d0,_protorp+4		| stash USTP
	pflushr	_protorp		| flush RPT/TLB entries
Lnot68851:
	rts

ENTRY(ploadw)
	movl	sp@(4),a0		| address to load
	tstl	_cpu040
	jne	Lploadw040
	ploadw	#1,a0@			| pre-load translation
Lploadw040:				| should '040 do a ptest?
	rts

/*
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, spl0 requires special handling
 * as we need to check for our emulated software interrupts.
 */

ALTENTRY(splnone, _spl0)
ENTRY(spl0)
	moveq	#0,d0
	movw	sr,d0			| get old SR for return
	movw	#PSL_LOWIPL,sr		| restore new SR
	tstb	_ssir			| software interrupt pending?
	jeq	Lspldone		| no, all done
	subql	#4,sp			| make room for RTE frame
	movl	sp@(4),sp@(2)		| position return address
	clrw	sp@(6)			| set frame type 0
	movw	#PSL_LOWIPL,sp@		| and new SR
	jra	Lgotsir			| go handle it
Lspldone:
	rts

ENTRY(_insque)
	movw	sr,d0
	movw	#PSL_HIGHIPL,sr		| atomic
	movl	sp@(8),a0		| where to insert (after)
	movl	sp@(4),a1		| element to insert (e)
	movl	a0@,a1@			| e->next = after->next
	movl	a0,a1@(4)		| e->prev = after
	movl	a1,a0@			| after->next = e
	movl	a1@,a0
	movl	a1,a0@(4)		| e->next->prev = e
	movw	d0,sr
	rts

ENTRY(_remque)
	movw	sr,d0
	movw	#PSL_HIGHIPL,sr		| atomic
	movl	sp@(4),a0		| element to remove (e)
	movl	a0@,a1
	movl	a0@(4),a0
	movl	a0,a1@(4)		| e->next->prev = e->prev
	movl	a1,a0@			| e->prev->next = e->next
	movw	d0,sr
	rts

/*
 * bzero(addr, count)
 */
ALTENTRY(blkclr, _bzero)
ENTRY(bzero)
	movl	sp@(4),a0	| address
	movl	sp@(8),d0	| count
	jeq	Lbzdone		| if zero, nothing to do
	movl	a0,d1
	btst	#0,d1		| address odd?
	jeq	Lbzeven		| no, can copy words
	clrb	a0@+		| yes, zero byte to get to even boundary
	subql	#1,d0		| decrement count
	jeq	Lbzdone		| none left, all done
Lbzeven:
	movl	d0,d1
	andl	#31,d0
	lsrl	#5,d1		| convert count to 8*longword count
	jeq	Lbzbyte		| no such blocks, zero byte at a time
Lbzloop:
	clrl	a0@+; clrl	a0@+; clrl	a0@+; clrl	a0@+
	clrl	a0@+; clrl	a0@+; clrl	a0@+; clrl	a0@+
	subql	#1,d1		| one more block zeroed
	jne	Lbzloop		| more to go, do it
	tstl	d0		| partial block left?
	jeq	Lbzdone		| no, all done
Lbzbyte:
	clrb	a0@+
	subql	#1,d0		| one more byte cleared
	jne	Lbzbyte		| more to go, do it
Lbzdone:
	rts

/*
 * strlen(str)
 */
ENTRY(strlen)
	moveq	#-1,d0
	movl	sp@(4),a0	| string
Lslloop:
	addql	#1,d0		| increment count
	tstb	a0@+		| null?
	jne	Lslloop		| no, keep going
	rts

/*
 * bcmp(s1, s2, len)
 *
 * WARNING!  This guy only works with counts up to 64K
 */
ENTRY(bcmp)
	movl	sp@(4),a0		| string 1
	movl	sp@(8),a1		| string 2
	moveq	#0,d0
	movw	sp@(14),d0		| length
	jeq	Lcmpdone		| if zero, nothing to do
	subqw	#1,d0			| set up for DBcc loop
Lcmploop:
	cmpmb	a0@+,a1@+		| equal?
	dbne	d0,Lcmploop		| yes, keep going
	addqw	#1,d0			| +1 gives zero on match
Lcmpdone:
	rts
	
/*
 * {ov}bcopy(from, to, len)
 *
 * Works for counts up to 128K.
 */
ALTENTRY(ovbcopy, _bcopy)
ENTRY(bcopy)
	movl	sp@(12),d0		| get count
	jeq	Lcpyexit		| if zero, return
	movl	sp@(4),a0		| src address
	movl	sp@(8),a1		| dest address
	cmpl	a1,a0			| src before dest?
	jlt	Lcpyback		| yes, copy backwards (avoids overlap)
	movl	a0,d1
	btst	#0,d1			| src address odd?
	jeq	Lcfeven			| no, go check dest
	movb	a0@+,a1@+		| yes, copy a byte
	subql	#1,d0			| update count
	jeq	Lcpyexit		| exit if done
Lcfeven:
	movl	a1,d1
	btst	#0,d1			| dest address odd?
	jne	Lcfbyte			| yes, must copy by bytes
	movl	d0,d1			| no, get count
	lsrl	#2,d1			| convert to longwords
	jeq	Lcfbyte			| no longwords, copy bytes
	subql	#1,d1			| set up for dbf
Lcflloop:
	movl	a0@+,a1@+		| copy longwords
	dbf	d1,Lcflloop		| til done
	andl	#3,d0			| get remaining count
	jeq	Lcpyexit		| done if none
Lcfbyte:
	subql	#1,d0			| set up for dbf
Lcfbloop:
	movb	a0@+,a1@+		| copy bytes
	dbf	d0,Lcfbloop		| til done
Lcpyexit:
	rts
Lcpyback:
	addl	d0,a0			| add count to src
	addl	d0,a1			| add count to dest
	movl	a0,d1
	btst	#0,d1			| src address odd?
	jeq	Lcbeven			| no, go check dest
	movb	a0@-,a1@-		| yes, copy a byte
	subql	#1,d0			| update count
	jeq	Lcpyexit		| exit if done
Lcbeven:
	movl	a1,d1
	btst	#0,d1			| dest address odd?
	jne	Lcbbyte			| yes, must copy by bytes
	movl	d0,d1			| no, get count
	lsrl	#2,d1			| convert to longwords
	jeq	Lcbbyte			| no longwords, copy bytes
	subql	#1,d1			| set up for dbf
Lcblloop:
	movl	a0@-,a1@-		| copy longwords
	dbf	d1,Lcblloop		| til done
	andl	#3,d0			| get remaining count
	jeq	Lcpyexit		| done if none
Lcbbyte:
	subql	#1,d0			| set up for dbf
Lcbbloop:
	movb	a0@-,a1@-		| copy bytes
	dbf	d0,Lcbbloop		| til done
	rts

#if 0
/*
 * Emulate fancy VAX string operations:
 *	scanc(count, startc, table, mask)
 *	skpc(mask, count, startc)
 *	locc(mask, count, startc)
 */
ENTRY(scanc)
	movl	sp@(4),d0	| get length
	jeq	Lscdone		| nothing to do, return
	movl	sp@(8),a0	| start of scan
	movl	sp@(12),a1	| table to compare with
	movb	sp@(19),d1	| and mask to use
	movw	d2,sp@-		| need a scratch register
	clrw	d2		| clear it out
	subqw	#1,d0		| adjust for dbra
Lscloop:
	movb	a0@+,d2		| get character
	movb	a1@(0,d2:w),d2	| get table entry
	andb	d1,d2		| mask it
	dbne	d0,Lscloop	| keep going til no more or non-zero
	addqw	#1,d0		| overshot by one
	movw	sp@+,d2		| restore scratch
Lscdone:
	rts

ENTRY(skpc)
	movl	sp@(8),d0	| get length
	jeq	Lskdone		| nothing to do, return
	movb	sp@(7),d1	| mask to use
	movl	sp@(12),a0	| where to start
	subqw	#1,d0		| adjust for dbcc
Lskloop:
	cmpb	a0@+,d1		| compate with mask
	dbne	d0,Lskloop	| keep going til no more or zero
	addqw	#1,d0		| overshot by one
Lskdone:
	rts

ENTRY(locc)
	movl	sp@(8),d0	| get length
	jeq	Llcdone		| nothing to do, return
	movb	sp@(7),d1	| mask to use
	movl	sp@(12),a0	| where to start
	subqw	#1,d0		| adjust for dbcc
Llcloop:
	cmpb	a0@+,d1		| compate with mask
	dbeq	d0,Llcloop	| keep going til no more or non-zero
	addqw	#1,d0		| overshot by one
Llcdone:
	rts
#endif

/*
 * Emulate VAX FFS (find first set) instruction.
 */
ENTRY(ffs)
	moveq	#-1,d0
	movl	sp@(4),d1
	jeq	Lffsdone
Lffsloop:
	addql	#1,d0
	btst	d0,d1
	jeq	Lffsloop
Lffsdone:
	addql	#1,d0
	rts

/*
 * Save and restore 68881 state.
 * Pretty awful looking since our assembler does not
 * recognize FP mnemonics.
 */
ENTRY(m68881_save)
	movl	sp@(4),a0		| save area pointer
	fsave	a0@			| save state
	tstb	a0@			| null state frame?
	jeq	Lm68881sdone		| yes, all done
	fmovem fp0-fp7,a0@(216)		| save FP general registers
	fmovem fpcr/fpsr/fpi,a0@(312)	| save FP control registers
Lm68881sdone:
	rts

ENTRY(m68881_restore)
	movl	sp@(4),a0		| save area pointer
	tstb	a0@			| null state frame?
	jeq	Lm68881rdone		| yes, easy
	fmovem	a0@(312),fpcr/fpsr/fpi	| restore FP control registers
	fmovem	a0@(216),fp0-fp7	| restore FP general registers
Lm68881rdone:
	frestore a0@			| restore state
	rts

/*
 * Handle the nitty-gritty of rebooting the machine.
 * Basically we just turn off the MMU and jump to the appropriate ROM routine.
 * Note that we must be running in an address range that is mapped one-to-one
 * logical to physical so that the PC is still valid immediately after the MMU
 * is turned off.  We have conveniently mapped the last page of physical
 * memory this way.
 *
 * Boot and reboot are sitting in a tree.
 * Boot falls out.  Who's left?
 * Well, reboot.
 * Dong!!!!
 */
	.globl	_doboot
_doboot:
	movw	#PSL_HIGHIPL,sr		| no interrupts
	movl	#CACHE_OFF,d0
	movc	d0,cacr			| disable on-chip cache(s)

	cmpl	#MMU_68030, _mmutype
	jeq	Lmap030rom		| don't turn off MMU if '030

	lea	longscratch,a0		| make sure we have real memory
	movl	#0,a0@			| value for pmove to TC (turn off MMU)
	pmove	a0@,tc			| disable MMU
	jra	Lreboot

	| If '030, we can't turn off MMU because we might not be PA == VA.
	| We must therefore map the ROMs using the tt registers before
	| we jump there.
Lmap030rom:
	lea	longscratch,a0
	movl	#0x400F8307,a0@		| map all of 0x4xxxxxxx
	.long	0xf0100800		| pmove a0@,tt0

Lreboot:
	movl	#0x40800090,a1		| address of ROM reset routine
	jra	a1@			| jump to ROM to reset machine

	/*
	 * LAK: (7/24/94) This routine was added so that the
	 *  C routine that runs at startup can figure out how MacOS
	 *  had mapped memory.  We want to keep the same mapping so
	 *  that when we set our MMU pointer, the PC doesn't point
	 *  in the middle of nowhere.
	 *
	 * long get_pte(void *addr, unsigned long pte[2], unsigned short *psr)
	 *
	 *  Takes "addr" and looks it up in the current MMU pages.  Puts
	 *  the PTE of that address in "pte" and the result of the
	 *  search in "psr".  "pte" should be 2 longs in case it is
	 *  a long-format entry.
	 *
	 *  One possible problem here is that setting the tt register
	 *  may screw something up if, say, the address returned by ptest
	 *  in a0 has msb of 0.
	 *
	 *  Returns -1 on error, 0 if pte is a short-format pte, or
	 *  1 if pte is a long-format pte.
	 *
	 *  Be sure to only call this routine if the MMU is enabled.  This
	 *  routine is probably more general than it needs to be -- it
	 *  could simply return the physical address (replacing
	 *  get_physical() in machdep).
	 *
	 *  "gas" does not understand the tt0 register, so we must hand-
	 *  assemble the instructions.
	 */
	.globl	_get_pte
_get_pte:
	addl	#-4,sp		| make temporary space
	movl	sp@(8),a0	| logical address to look up
	movl	#0,a1		| clear in case of failure
	ptestr	#1,a0@,#7,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1
	movl	sp@(16),a0	| where to store the psr
	movw	d1,a0@		| send back to caller
	andw	#0xc400,d1	| if bus error, exceeded limit, or invalid
	jne	get_pte_fail1	| leave now
	tstl	a1		| check address we got back
	jeq	get_pte_fail2	| if 0, then was not set -- fail

	| enable tt0
	movl	a1,d0
	movl	d0,pte_tmp	| save for later
	andl	#0xff000000,d0	| keep msb
	orl	#0x00008707,d0	| enable tt for reading and writing
	movl	d0,longscratch
	lea	longscratch,a0
	.long	0xf0100800	| pmove a0@,tt0

	| send first long back to user
	movl	sp@(12),a0	| address of where to put pte
	movl	a1@,d0		|
	movl	d0,a0@		| first long

	andl	#3,d0		| dt bits of pte
	cmpl	#1,d0		| should be 1 if page descriptor
	jne	get_pte_fail3	| if not, get out now

	movl	sp@(16),a0	| addr of stored psr
	movw	a0@,d0		| get psr again
	andw	#7,d0		| number of levels it found
	addw	#-1,d0		| find previous level
	movl	sp@(8),a0	| logical address to look up
	movl	#0,a1		| clear in case of failure

	cmpl	#0,d0
	jeq	pte_level_zero
	cmpl	#1,d0
	jeq	pte_level_one
	cmpl	#2,d0
	jeq	pte_level_two
	cmpl	#3,d0
	jeq	pte_level_three
	cmpl	#4,d0
	jeq	pte_level_four
	cmpl	#5,d0
	jeq	pte_level_five
	cmpl	#6,d0
	jeq	pte_level_six
	jra	get_pte_fail4	| really should have been one of these...

pte_level_zero:
	| must get CRP to get length of entries at first level
	lea	longscratch,a0	| space for two longs
	pmove	crp,a0@		| save root pointer
	movl	a0@,d0		| load high long
	jra	pte_got_parent
pte_level_one:
	ptestr	#1,a0@,#1,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1
	jra	pte_got_it
pte_level_two:
	ptestr	#1,a0@,#2,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1
	jra	pte_got_it
pte_level_three:
	ptestr	#1,a0@,#3,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1
	jra	pte_got_it
pte_level_four:
	ptestr	#1,a0@,#4,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1
	jra	pte_got_it
pte_level_five:
	ptestr	#1,a0@,#5,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1
	jra	pte_got_it
pte_level_six:
	ptestr	#1,a0@,#6,a1	| search for logical address
	pmove	psr,sp@		| store processor status register
	movw	sp@,d1

pte_got_it:
	andw	#0xc400,d1	| if bus error, exceeded limit, or invalid
	jne	get_pte_fail5	| leave now
	tstl	a1		| check address we got back
	jeq	get_pte_fail6	| if 0, then was not set -- fail

	| change tt0
	movl	a1,d0
	andl	#0xff000000,d0	| keep msb
	orl	#0x00008707,d0	| enable tt for reading and writing
	movl	d0,longscratch
	lea	longscratch,a0
	.long	0xF0100800	| pmove a0@,tt0

	movl	a1@,d0		| get pte of parent
	movl	d0,_macos_tt0	| XXX for later analysis (kill this line)
pte_got_parent:
	andl	#3,d0		| dt bits of pte
	cmpl	#2,d0		| child is short-format descriptor
	jeq	short_format
	cmpl	#3,d0		| child is long-format descriptor
	jne	get_pte_fail7

	| long_format -- we must go back, change the tt, and get the
	|  second long.  The reason we didn't do this in the first place
	|  is that the first long might have been the last long of RAM.

	movl	pte_tmp,a1	| get address of our original pte
	addl	#4,a1		| address of ite second long

	| change tt0 back
	movl	a1,d0
	andl	#0xff000000,d0	| keep msb
	orl	#0x00008707,d0	| enable tt for reading and writing
	movl	d0,longscratch
	lea	longscratch,a0
	.long	0xF0100800	| pmove a0@,tt0

	movl	sp@(12),a0	| address of return pte
	movl	a1@,a0@(4)	| write in second long

	movl	#1,d0		| return long-format
	jra	get_pte_success

short_format:
	movl	#0,d0		| return short-format
	jra	get_pte_success

get_pte_fail:
	movl	#-1,d0		| return failure

get_pte_success:
	clrl	d1		| disable tt
	movl	d1,longscratch
	lea	longscratch,a0
	.long	0xF0100800	| pmove a0@,tt0

	addl	#4,sp		| return temporary space
	rts

get_pte_fail1:
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail2:
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail3:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail4:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail5:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail6:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail7:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail8:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail9:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail
get_pte_fail10:
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jbsr	_printstar
	jra	get_pte_fail

	.data
	.globl	_sanity_check
_sanity_check:
	.long	0x18621862	| this is our stack overflow checker.

	.space	4 * NBPG
tmpstk:
	.globl	_machineid
_machineid:
	.long	0		| default to 320
	.globl	_mmutype,_protorp
iottdata:
	.long	0x50018600	| maps IO space in TT1 register
_mmutype:
	.long	0		| Are we running 68851, 68030, or 68040?
				| (-1, 0, 1, respectively)
_protorp:
	.long	0,0		| prototype root pointer
	.globl	_cold
_cold:
	.long	1		| cold start flag
	.globl	_intiolimit
	.globl	_proc0paddr
_proc0paddr:
	.long	0		| KVA of proc0 u-area
_intiolimit:
	.long	0		| KVA of end of internal IO space
	.globl	_load_addr
_load_addr:
	.long	0		| Physical address of kernel
_cpu040:
	.long	0		| Flag: Are we currently running on a 68040
lastpage:
	.long	0		| LAK: to store the addr of last page in mem
#ifdef DEBUG
	.globl	fulltflush, fullcflush
fulltflush:
	.long	0
fullcflush:
	.long	0
	.globl	timebomb
timebomb:
	.long	0
#endif
/* interrupt counters */
	.globl	_intrcnt,_intrnames,_eintrcnt,_eintrnames
_intrnames:
	.asciz	"spur"
	.asciz	"via1"
	.asciz	"via2"
	.asciz	"scc"
	.asciz	"nmi"
	.asciz	"clock"
_eintrnames:
	.even
_intrcnt:
	.long	0,0,0,0,0,0
_eintrcnt:
_locore_dodebugmarks:
	.long	0
