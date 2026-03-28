/*	$NetBSD: trap_subr.s,v 1.28 2026/03/28 01:44:36 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

#include "opt_m68k_arch.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_fpsp.h"
#include "opt_m060sp.h"

#include <machine/asm.h>
#include <machine/trap.h>

#include "assym.h"

	.file	"trap_subr.s"
	.text

/*
 * Common fault handling code.  Called by exception vector handlers.
 * Registers have been saved, and type for trap() placed in d0.
 */
ASENTRY_NOPROFILE(fault)
	movl	%usp,%a0		| get and save
	movl	%a0,FR_SP(%sp)		|   the user stack pointer
	clrl	-(%sp)			| no VA arg
	clrl	-(%sp)			| or code arg
	movl	%d0,-(%sp)		| push trap type
	pea	12(%sp)			| address of trap frame
	jbsr	_C_LABEL(trap)		| handle trap
	lea	16(%sp),%sp		| pop value args
	movl	FR_SP(%sp),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	(%sp)+,#0x7FFF		| restore most user regs
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

/*
 * Similar to above, but will tidy up the stack, if necessary.
 */
ASENTRY(faultstkadj)
	pea	12(%sp)			| address of trap frame
	jbsr	_C_LABEL(trap)		| handle the error
	lea	16(%sp),%sp		| pop value args
/* for new 68060 Branch Prediction Error handler */
ASGLOBAL(faultstkadjnotrap2)
	movl	FR_SP(%sp),%a0		| restore user SP
	movl	%a0,%usp		|   from save area
	movw	FR_ADJ(%sp),%d0		| need to adjust stack?
	jne	1f			| yes, go to it
	moveml	(%sp)+,#0x7FFF		| no, restore most user regs
	addql	#8,%sp			| toss SSP and stkadj
	jra	_ASM_LABEL(rei)		| all done
1:
	lea	FR_HW(%sp),%a1		| pointer to HW frame
	addql	#8,%a1			| source pointer
	movl	%a1,%a0			| source
	addw	%d0,%a0			|  + hole size = dest pointer
	movl	-(%a1),-(%a0)		| copy
	movl	-(%a1),-(%a0)		|  8 bytes
	movl	%a0,FR_SP(%sp)		| new SSP
	moveml	(%sp)+,#0x7FFF		| restore user registers
	movl	(%sp),%sp		| and our SP
	jra	_ASM_LABEL(rei)		| all done

ASGLOBAL(buserr_common)
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	1f			| no, handle as usual
#ifdef mac68k
	/*
	 * TL;DR - mac68k SCSI DMA needs to peek under the covers
	 * here, and needs the value of %a2 when the fault occurred
	 * as well as the faulting address.
	 *
	 * %a2 can be retrieved from the saved registers in the stack
	 * frame, and the faulting address has already been pushed
	 * onto the stack by the CPU-specific stub and currently resides
	 * at 4(%sp) (it's the 4th argument to trap(), and the trap type
	 * and frame address are yet to be pushed).  Yes, 2 args for trap()
	 * have already been pushed onto the stack, hence the +8 adjustment.
	 *
	 * XXX should find a cleaner way to do this.
	 */
	movl	%sp@(FR_A2+8),_C_LABEL(mac68k_a2_fromfault)
	movl	%sp@(4),_C_LABEL(m68k_fault_addr)
#endif
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
1:
	movl	#T_BUSERR,%sp@-		| mark bus error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

#if defined(COMPAT_13) || defined(COMPAT_SUNOS)
/*
 * Trap 1 - compat_13_sigreturn13
 */
ENTRY_NOPROFILE(trap1)
	jra	_C_LABEL(m68k_compat_13_sigreturn13_stub)
#endif

/*
 * Trap 2 - trace trap
 *
 * XXX SunOS uses this for a cache flush!  What do we do here?
 * XXX
 * XXX	movl	#IC_CLEAR,%d0
 * XXX	movc	%d0,%cacr
 * XXX	rte
 */
ENTRY_NOPROFILE(trap2)
	jra	_C_LABEL(trace)

#if defined(COMPAT_16)
/*
 * Trap 3 - sigreturn system call
 */
ENTRY_NOPROFILE(trap3)
	jra	_C_LABEL(m68k_compat_16_sigreturn14_stub)
#endif

/*
 * The following exceptions only cause four and six word stack frames
 * and require no post-trap stack adjustment.
 */
ENTRY_NOPROFILE(illinst)
	clrl	-(%sp)
	moveml	#0xFFFF,-(%sp)
	moveq	#T_ILLINST,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(zerodiv)
	clrl	-(%sp)
	moveml	#0xFFFF,-(%sp)
	moveq	#T_ZERODIV,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(chkinst)
	clrl	-(%sp)
	moveml	#0xFFFF,-(%sp)
	moveq	#T_CHKINST,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(trapvinst)
	clrl	-(%sp)
	moveml	#0xFFFF,-(%sp)
	moveq	#T_TRAPVINST,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(privinst)
	clrl	-(%sp)
	moveml	#0xFFFF,-(%sp)
	moveq	#T_PRIVINST,%d0
	jra	_ASM_LABEL(fault)

/*
 * Coprocessor and format errors can generate mid-instruction stack
 * frames and cause signal delivery, hence we need to check for potential
 * stack adjustment.
 */
#ifndef __mc68010__
ENTRY_NOPROFILE(coperr)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)
	movl	%usp,%a0		| get and save
	movl	%a0,FR_SP(%sp)		|   the user stack pointer
	clrl	-(%sp)			| no VA arg
	clrl	-(%sp)			| or code arg
	movl	#T_COPERR,-(%sp)	| push trap type
	jra	_ASM_LABEL(faultstkadj)	| call trap and deal with stack
					|   adjustments
#endif /* ! __mc68010__ */

ENTRY_NOPROFILE(fmterr)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)
	movl	%usp,%a0		| get and save
	movl	%a0,FR_SP(%sp)		|   the user stack pointer
	clrl	-(%sp)			| no VA arg
	clrl	-(%sp)			| or code arg
	movl	#T_FMTERR,-(%sp)	| push trap type
	jra	_ASM_LABEL(faultstkadj)	| call trap and deal with stack
					|   adjustments

/*
 * Floating point exceptions.
 */

#if defined(M68040) && defined(FPSP)
/*
 * These two stubs are used when a 68040 FPU is present and the FPSP
 * is compiled into the kernel.
 */
ENTRY_NOPROFILE(fpfline_fpsp40)
	cmpw	#0x202c,6(%sp)		| format type 2?
	jne	_C_LABEL(illinst)	| no, not an FP emulation
	jmp	_ASM_LABEL(fpsp_unimp)	| yes, go handle it

ENTRY_NOPROFILE(fpunsupp_fpsp40)
	jmp	_ASM_LABEL(fpsp_unsupp)
#endif /* M68040 && FPSP */

#if (defined(M68040) && !defined(FPSP)) || \
    (defined(M68060) && !defined(M060SP))
/*
 * This stub is used on either the 68040 or the 68060, but only
 * then that CPU's corresponding FP support package is **not**
 * compiled into the kernel.
 */
ENTRY_NOPROFILE(fpunsupp_4060)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)		| save registers
	moveq	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#endif /* (M68040 && ! FPSP) || (M68060 && ! M060SP) */

ENTRY_NOPROFILE(fpfline)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)		| save registers
	moveq	#T_FPEMULI,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it

#ifdef M68K_FPCOPROC
/*
 * Handles all other FP coprocessor exceptions.
 *
 * Note that since some FP exceptions generate mid-instruction frames
 * and may cause signal delivery, we need to test for stack adjustment
 * after the trap call.
 */
ENTRY_NOPROFILE(fpfault)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)		| save registers
	movl	%usp,%a0		| and save
	movl	%a0,FR_SP(%sp)		|   the user stack pointer
	clrl	-(%sp)			| no VA arg
	movl	_C_LABEL(curpcb),%a0	| current pcb
	lea	PCB_FPCTX(%a0),%a0	| address of FP savearea
	fsave	(%a0)			| save state
#if defined(M68020) || defined(M68030)
#if defined(M68040) || defined(M68060)
	/* not needed on 68040, 68040, skip for unknown */
	cmpl	#FPU_68040,_C_LABEL(fputype)
	jge	Lfptnull
#endif
	tstb	(%a0)			| null state frame?
	jeq	Lfptnull		| yes, safe
	clrw	%d0			| no, need to tweak BIU
	movb	1(%a0),%d0		| get frame size
	bset	#3,%a0@(0,%d0:w)	| set exc_pend bit of BIU
Lfptnull:
#endif
	fmovem	%fpsr,-(%sp)		| push %fpsr as code argument
	frestore (%a0)			| restore state
	movl	#T_FPERR,-(%sp)		| push type arg
	jra	_ASM_LABEL(faultstkadj)	| call trap and deal with stack cleanup
#endif /* M68K_FPCOPROC */

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(badtrap)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)		| save standard frame regs
	jbsr	_C_LABEL(straytrap)	| report
	moveml	(%sp)+,#0x7FFF		| restore all except %sp
	addql	#8,%sp			| pop stack adjust count and %sp
	jra	_ASM_LABEL(rei)		| all done

/* trap #0 -- system calls */
ENTRY_NOPROFILE(trap0)
	clrl	-(%sp)			| stack adjust count
	moveml	#0xFFFF,-(%sp)		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%d0,-(%sp)		| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall arg
	tstl	_C_LABEL(astpending)	| AST pending?
	jne	_ASM_LABEL(doast)	| Yup, go deal with it.
	movl	%sp@(FR_SP),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	moveml	(%sp)+,#0x7FFF		| restore most registers
	addql	#8,%sp			| pop SP and stack adjust
	rte				| all done

/*
 * trap #12 -- "cachectl" pseudo-syscall (originally from HP-UX, but also
 * used natively in BSD).
 *
 *	cachectl(command, addr, length)
 *
 * command in d0, addr in a1, length in d1
 */
ENTRY_NOPROFILE(trap12)
#ifndef __mc68010__
	movl	_C_LABEL(curlwp),%a0
	movl	%a0@(L_PROC),-(%sp)	| push current proc pointer
	movl	%d1,-(%sp)		| push length
	movl	%a1,-(%sp)		| push addr
	movl	%d0,-(%sp)		| push command
	jbsr	_C_LABEL(cachectl1)	| do it
	lea	%sp@(16),%sp		| pop args
#endif /* ! __mc68010__ */
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trace (single-step) trap.  Kernel-mode is special.
 * User mode traps are simply passed on to trap().
 */
ENTRY_NOPROFILE(trace)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRACE,%d0

	| Check PSW and see what happen.
	|   T=0 S=0	(should not happen)
	|   T=1 S=0	trace trap from user mode
	|   T=0 S=1	trace trap on a trap instruction
	|   T=1 S=1	trace trap from system mode (kernel breakpoint)

	movw	%sp@(FR_HW),%d1		| get PSW
	notw	%d1			| XXX no support for T0 on 680[234]0
	andw	#PSL_TS,%d1		| from system mode (T=1, S=1)?
	jeq	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

/*
 * Trap 15 is used for:
 *	- GDB breakpoints (in user programs)
 *	- KGDB breakpoints (in the kernel)
 *	- trace traps for SUN binaries (not fully supported yet)
 * User mode traps are simply passed to trap().
 */
ENTRY_NOPROFILE(trap15)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAP15,%d0
	btst	#5,%sp@(FR_HW)		| was supervisor mode?
	jne	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

Lkbrkpt:
	| Kernel-mode breakpoint or trace trap. (%d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If were are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	2f			| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0			| a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1 | a1=dst XXX MAGIC NUMBER 96
	movl	%a1,%sp			| sp=new frame
	movql	#FR_SIZE,%d1
1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	jbgt	1b

2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to do it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	movl	%sp,%a0			| frame pointer
	movl	%a0,%sp@-		| push frame ptr
	movl	%d0,%sp@-		| push trap type
	jbsr	_C_LABEL(trap_kdebug)
	addql	#8,%sp			| pop args

	| The stack pointer may have been modified, or
	| data below it modified (by kgdb push call),
	| so push the hardware frame at the current sp
	| before restoring registers and returning.

	movl	%sp@(FR_SP),%a0		| modified sp
	lea	%sp@(FR_SIZE),%a1	| end of our frame
	movl	%a1@-,%a0@-		| copy 2 longs with
	movl	%a1@-,%a0@-		| ... predecrement
	movl	%a0,%sp@(FR_SP)		| sp = h/w frame
	moveml	%sp@+,#0x7FFF		| restore all but sp
	movl	%sp@,%sp		| ... and sp
	rte				| all done

/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing ASTs (profiling,
 * scheduling).  We have side-channel entry point to provide a fast
 * path to this for system calls.
 *
 * After identifying that we need an AST we drop the IPL to allow device
 * interrupts.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.
 */
ASENTRY_NOPROFILE(rei)
	tstl	_C_LABEL(astpending)	| AST pending?
	jne	1f			| yes, next check...
	rte				| nope, done.
1:
	btst	#5,%sp@			| Are we returning to user mode?
	jeq	1f			| yes, go do the work.
	rte				| nope, done.
1:
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
ASGLOBAL(doast)	| fast-path for syscall trap vector
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	pea	%sp@(12)		| fp == address of trap frame
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(16),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore user SP
	movl	%a0,%usp		|   from save area
	movw	%sp@(FR_ADJ),%d0	| need to adjust stack?
	jne	Laststkadj		| yes, go to it
	moveml	%sp@+,%d0-%d7/%a0-%a6	| no, restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	rte				| done.

Laststkadj:
	lea	%sp@(FR_HW),%a1		| pointer to HW frame
	addql	#8,%a1			| source pointer
	movl	%a1,%a0			| source
	addw	%d0,%a0			|  + hole size = dest pointer
	movl	%a1@-,%a0@-		| copy
	movl	%a1@-,%a0@-		|  8 bytes
	movl	%a0,%sp@(FR_SP)		| new SSP
	moveml	%sp@+,%d0-%d7/%a0-%a6	| restore user registers
	movl	%sp@,%sp		| and our SP
	rte				| done.
