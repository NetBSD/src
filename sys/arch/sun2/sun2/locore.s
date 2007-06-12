/*	$NetBSD: locore.s,v 1.19 2007/06/12 03:34:32 mhitch Exp $	*/

/*
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
 *	from: Utah $Hdr: locore.s 1.66 92/12/22$
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

/*
 * Copyright (c) 2001 Matthew Fredette
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
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
 *	from: Utah $Hdr: locore.s 1.66 92/12/22$
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

| Remember this is a fun project!

| This is for kvm_mkdb, and should be the address of the beginning
| of the kernel text segment (not necessarily the same as kernbase).
	.text
GLOBAL(kernel_text)

| This is the entry point, as well as the end of the temporary stack
| used during process switch (two 2K pages ending at start)
ASGLOBAL(tmpstk)
ASGLOBAL(start)

| As opposed to the sun3, on the sun2 the kernel is linked low.  The
| boot loader loads us exactly where we are linked, so we don't have
| to worry about writing position independent code or moving the 
| kernel around.
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	moveq	#FC_CONTROL,%d0		| make movs access "control"
	movc	%d0,%sfc		| space where the sun2 designers
	movc	%d0,%dfc		| put all the "useful" stuff

| Set context zero and stay there until pmap_bootstrap.
	moveq	#0,%d0
	movsb	%d0,CONTEXT_REG
	movsb	%d0,SCONTEXT_REG

| Jump around the g0 and g4 entry points.
	jra	L_high_code

| These entry points are here in pretty low memory, so that they
| can be reached from virtual address zero using the classic,
| old-school "g0" and "g4" commands from the monitor.  (I.e., 
| they need to be reachable using 16-bit displacements from PCs 
| 0 and 4).
ENTRY(g0_entry)
	jra	_C_LABEL(g0_handler)
ENTRY(g4_entry)
	jra	_C_LABEL(g4_handler)
	
L_high_code:
| We are now running in the correctly relocated kernel, so
| we are no longer restricted to position-independent code.

| Disable interrupts, and initialize the soft copy of the
| enable register.
	movsw	SYSTEM_ENAB, %d0	| read the enable register
	moveq	#ENA_INTS, %d1
	notw	%d1
	andw	%d1, %d0
	movsw	%d0, SYSTEM_ENAB	| disable all interrupts
	movw	%d0, _C_LABEL(enable_reg_soft)

| Do bootstrap stuff needed before main() gets called.
| Make sure the initial frame pointer is zero so that
| the backtrace algorithm used by KGDB terminates nicely.
	lea	_ASM_LABEL(tmpstk),%sp
	movl	#0,%a6
	jsr	_C_LABEL(_bootstrap)	| See locore2.c

| Now that _bootstrap() is done using the PROM functions,
| we can safely set the %sfc/dfc to something != FC_CONTROL
	moveq	#FC_USERD,%d0		| make movs access "user data"
	movc	%d0,%sfc		| space for copyin/copyout
	movc	%d0,%dfc

| Setup process zero user/kernel stacks.
	movl	_C_LABEL(proc0paddr),%a1 | get proc0 pcb addr
	lea	%a1@(USPACE-4),%sp	| set SSP to last word
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP

| Note curpcb was already set in _bootstrap().
| Will do fpu initialization during autoconfig (see fpu.c)
| The interrupt vector table and stack are now ready.
| Interrupts will be enabled later, AFTER  autoconfiguration
| is finished, to avoid spurrious interrupts.

/*
 * Create a fake exception frame so that cpu_fork() can copy it.
 * main() nevers returns; we exit to user mode from a forked process
 * later on.
 */
	clrw	%sp@-			| tf_format,tf_vector
	clrl	%sp@-			| tf_pc (filled in later)
	movw	#PSL_USER,%sp@-		| tf_sr for user mode
	clrl	%sp@-			| tf_stackadj
	lea	%sp@(-64),%sp		| tf_regs[16]
	lea	_C_LABEL(lwp0),%a0	| proc0.p_md.md_regs = 
	movl	%a1,%a0@(L_MD_REGS)	|   trapframe
	jbsr	_C_LABEL(main)		| main(&trapframe)
	PANIC("main() returned")

| That is all the assembly startup code we need on the sun3!
| The rest of this is like the hp300/locore.s where possible.

/*
 * Trap/interrupt vector routines
 */
#include <m68k/m68k/trap_subr.s>

GLOBAL(buserr)
	tstl	_C_LABEL(nofault)	| device probe?
	jeq	_C_LABEL(addrerr)	| no, handle as usual
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
GLOBAL(addrerr)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	lea	%sp@(FR_HW),%a1		| grab base of HW berr frame
	moveq	#0,%d0
	movw	%a1@(8),%d0		| grab SSW for fault processing
	movl	%a1@(10),%d1		| fault address is as given in frame
	movl	%d1,%sp@-		| push fault VA
	movl	%d0,%sp@-		| and padded SSW
	movw	%a1@(6),%d0		| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it

/*
 * the sun2 specific code
 *
 * our mission: figure out whether what we are looking at is
 *              bus error in the UNIX sense, or
 *	        a memory error i.e a page fault
 *
 * [this code replaces similarly mmu specific code in the hp300 code]
 */
sun2_mmu_specific:
	clrl %d0			| make sure top bits are cleard too
	movl %d1,%sp@-			| save %d1
	movc %sfc,%d1			| save %sfc to %d1
	moveq #FC_CONTROL,%d0		| %sfc = FC_CONTROL
	movc %d0,%sfc
	movsw BUSERR_REG,%d0		| get value of bus error register
	movc %d1,%sfc			| restore %sfc
	movl %sp@+,%d1			| restore %d1
#ifdef	DEBUG
	movw %d0, _C_LABEL(buserr_reg)	| save bus error register value
#endif
	andb #BUSERR_PROTERR, %d0 	| is this an MMU (protection *or* page unavailable) fault?
	jeq Lisberr			| non-MMU bus error
/* End of sun2 specific code. */

Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisberr:
	movl	#T_BUSERR,%sp@-		| mark bus error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

/*
 * FP exceptions.
 */
GLOBAL(fpfline)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULI,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it

GLOBAL(fpunsupp)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it

| Message for fpfault panic
Lfp0:   
	.asciz	"fpfault"
	.even   

/*
 * Handles all other FP coprocessor exceptions.
 * Since we can never have an FP coprocessor, this just panics.
 */
GLOBAL(fpfault)
	movl	#Lfp0,%sp@-
	jbsr	_C_LABEL(panic)
	/*NOTREACHED*/

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */
GLOBAL(badtrap)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save std frame regs
	jbsr	_C_LABEL(straytrap)	| report
	moveml	%sp@+,#0xFFFF		| restore regs
	addql	#4,%sp			| stack adjust count
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trap 0 is for system calls
 */
GLOBAL(trap0)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%d0,%sp@-		| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall arg
	movl	%sp@(FR_SP),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trap 12 is the entry point for the cachectl "syscall"
 *	cachectl(command, addr, length)
 * command in %d0, addr in %a1, length in %d1
 */
GLOBAL(trap12)
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trace (single-step) trap.  Kernel-mode is special.
 * User mode traps are simply passed on to trap().
 */
GLOBAL(trace)
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
	jeq	_ASM_LABEL(kbrkpt)	|  yes, kernel brkpt
	jra	_ASM_LABEL(fault)	| no, user-mode fault

/*
 * Trap 15 is used for:
 *	- GDB breakpoints (in user programs)
 *	- KGDB breakpoints (in the kernel)
 *	- trace traps for SUN binaries (not fully supported yet)
 * User mode traps are simply passed to trap().
 */
GLOBAL(trap15)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAP15,%d0
	btst	#5,%sp@(FR_HW)		| was supervisor mode?
	jne	_ASM_LABEL(kbrkpt)	|  yes, kernel brkpt
	jra	_ASM_LABEL(fault)	| no, user-mode fault

ASLOCAL(kbrkpt)
	| Kernel-mode breakpoint or trace trap. (%d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If we are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	Lbrkpt2 		| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0			| %a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1	| %a1=dst
	movl	%a1,%sp			| sp=new frame
	moveq	#FR_SIZE,%d1
Lbrkpt1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	bgt	Lbrkpt1

Lbrkpt2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to handle it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	movl	%d0,%sp@-		| push trap type
	jbsr	_C_LABEL(trap_kdebug)
	addql	#4,%sp			| pop args

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

/* Use common m68k sigreturn */
#include <m68k/m68k/sigreturn.s>

/*
 * Interrupt handlers.  Most are auto-vectored,
 * and hard-wired the same way on all sun3 models.
 * Format in the stack is:
 *   %d0,%d1,%a0,%a1, sr, pc, vo
 */

#define INTERRUPT_SAVEREG \
	moveml	#0xC0C0,%sp@-

#define INTERRUPT_RESTORE \
	moveml	%sp@+,#0x0303

/*
 * This is the common auto-vector interrupt handler,
 * for which the CPU provides the vector=0x18+level.
 * These are installed in the interrupt vector table.
 */
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
GLOBAL(_isr_autovec)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(isr_autovec)
	INTERRUPT_RESTORE
	jra	_ASM_LABEL(rei)

/* clock: see clock.c */
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
GLOBAL(_isr_clock)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(clock_intr)
	INTERRUPT_RESTORE
	jra	_ASM_LABEL(rei)

| Handler for all vectored interrupts (i.e. VME interrupts)
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
GLOBAL(_isr_vectored)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(isr_vectored)
	INTERRUPT_RESTORE
	jra	_ASM_LABEL(rei)

#undef	INTERRUPT_SAVEREG
#undef	INTERRUPT_RESTORE

/* interrupt counters (needed by vmstat) */
GLOBAL(intrnames)
	.asciz	"spur"	| 0
	.asciz	"lev1"	| 1
	.asciz	"lev2"	| 2
	.asciz	"lev3"	| 3
	.asciz	"lev4"	| 4
	.asciz	"clock"	| 5
	.asciz	"lev6"	| 6
	.asciz	"nmi"	| 7
GLOBAL(eintrnames)

	.data
	.even
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)
	.text

/*
 * Emulation of VAX REI instruction.
 *
 * This code is (mostly) un-altered from the hp300 code,
 * except that sun machines do not need a simulated SIR
 * because they have a real software interrupt register.
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

ASGLOBAL(rei)
#ifdef	DIAGNOSTIC
	tstl	_C_LABEL(panicstr)	| have we paniced?
	jne	Ldorte			| yes, do not make matters worse
#endif
	tstl	_C_LABEL(astpending)	| AST pending?
	jeq	Ldorte			| no, done
Lrei1:
	btst	#5,%sp@			| yes, are we returning to user mode?
	jne	Ldorte			| no, done
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
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
	moveml	%sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	rte				| and do real RTE
Laststkadj:
	lea	%sp@(FR_HW),%a1		| pointer to HW frame
	addql	#8,%a1			| source pointer
	movl	%a1,%a0			| source
	addw	%d0,%a0			|  + hole size = dest pointer
	movl	%a1@-,%a0@-		| copy
	movl	%a1@-,%a0@-		|  8 bytes
	movl	%a0,%sp@(FR_SP)		| new SSP
	moveml	%sp@+,#0x7FFF		| restore user registers
	movl	%sp@,%sp		| and our SP
Ldorte:
	rte				| real return

/*
 * Initialization is at the beginning of this file, because the
 * kernel entry point needs to be at zero for compatibility with
 * the Sun boot loader.  This works on Sun machines because the
 * interrupt vector table for reset is NOT at address zero.
 * (The MMU has a "boot" bit that forces access to the PROM)
 */

/*
 * Use common m68k sigcode.
 */
#include <m68k/m68k/sigcode.s>
#ifdef COMPAT_SUNOS
#include <m68k/m68k/sunos_sigcode.s>
#endif
#ifdef COMPAT_SVR4
#include <m68k/m68k/svr4_sigcode.s>
#endif

	.text

/*
 * Primitives
 */

/*
 * Use common m68k support routines.
 */
#include <m68k/m68k/support.s>

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#undef FPCOPROC
#include <m68k/m68k/switch_subr.s>


/*
 * Get callers current SP value.
 * Note that simply taking the address of a local variable in a C function
 * doesn't work because callee saved registers may be outside the stack frame
 * defined by A6 (e.g. GCC generated code).
 *
 * [I don't think the ENTRY() macro will do the right thing with this -- glass]
 */
GLOBAL(getsp)
	movl	%sp,%d0			| get current SP
	addql	#4,%d0			| compensate for return address
	rts

ENTRY(getsfc)
	movc	%sfc,%d0
	rts

ENTRY(getdfc)
	movc	%dfc,%d0
	rts

ENTRY(getvbr)
	movc	%vbr,%d0
#ifdef __ELF__
	movl	%d0, %a0
#endif /* __ELF__ */
	rts

ENTRY(setvbr)
	movl	%sp@(4),%d0
	movc	%d0,%vbr
	rts

/* loadustp, ptest_addr */

/*
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, we need one instantiation here
 * in case some non-optimized code makes external references.
 * Most places will use the inlined functions param.h supplies.
 */

ENTRY(_getsr)
	clrl	%d0
	movw	%sr,%d0
	rts

ENTRY(_spl)
	clrl	%d0
	movw	%sr,%d0
	movl	%sp@(4),%d1
	movw	%d1,%sr
	rts

ENTRY(_splraise)
	clrl	%d0
	movw	%sr,%d0
	movl	%d0,%d1
	andl	#PSL_HIGHIPL,%d1 	| old &= PSL_HIGHIPL
	cmpl	%sp@(4),%d1		| (old - new)
	bge	Lsplr
	movl	%sp@(4),%d1
	movw	%d1,%sr
Lsplr:
	rts

#ifdef DIAGNOSTIC
| Message for 68881 save/restore panic
Lsr0: 
	.asciz	"m68881 save/restore"
	.even   
#endif

/*
 * Save and restore 68881 state.
 */
ENTRY(m68881_save)
ENTRY(m68881_restore)
#ifdef	DIAGNOSTIC
	movl	#Lsr0,%sp@-
        jbsr	_C_LABEL(panic)
        /*NOTREACHED*/
#else
	rts
#endif

/*
 * _delay(unsigned N)
 * Delay for at least (N/256) microseconds.
 * This routine depends on the variable:  delay_divisor
 * which should be set based on the CPU clock rate.
 * XXX: Currently this is set based on the CPU model,
 * XXX: but this should be determined at run time...
 */
GLOBAL(_delay)
	| %d0 = arg = (usecs << 8)
	movl	%sp@(4),%d0
	| %d1 = delay_divisor;
	movl	_C_LABEL(delay_divisor),%d1

	/*
	 * Align the branch target of the loop to a half-line (8-byte)
	 * boundary to minimize cache effects.  This guarantees both
	 * that there will be no prefetch stalls due to cache line burst
	 * operations and that the loop will run from a single cache
	 * half-line.
	 */
#ifdef __ELF__
	.align	8
#else
	.align	3
#endif
L_delay:
	subl	%d1,%d0
	jgt	L_delay
	rts

/*
 * Set or clear bits in the enable register.  This mimics the
 * strange behavior in SunOS' locore.o, where they keep a soft
 * copy of what they think is in the enable register and loop
 * making a change until it sticks.  This is apparently to
 * be concurrent-safe without disabling interrupts.  Why you
 * can't just disable interrupts while mucking with the register
 * I dunno, but it may jive with sun3/intreg.c using the single-instruction
 * bit operations and the sun3 intreg being memory-addressable,
 * i.e., once the sun2 was designed they realized the enable
 * register had to be treated this way, so on the sun3 they made
 * it memory-addressable so you could just use the single-instructions.
 */
ENTRY(enable_reg_and)
	movc	%dfc,%a1		| save current dfc
	moveq	#FC_CONTROL, %d1
	movc	%d1, %dfc		| make movs access "control"
	movl	%sp@(4), %d1		| get our AND mask
	clrl	%d0
1:	andw	%d1, _C_LABEL(enable_reg_soft)	| do our AND
	movew	_C_LABEL(enable_reg_soft), %d0	| get the result
	movsw	%d0, SYSTEM_ENAB		| install the result
	cmpw	_C_LABEL(enable_reg_soft), %d0
	bne	1b				| install it again if the soft value changed
	movc	%a1,%dfc		| restore dfc
	rts

ENTRY(enable_reg_or)
	movc	%dfc,%a1		| save current dfc
	moveq	#FC_CONTROL, %d1
	movc	%d1, %dfc		| make movs access "control"
	movl	%sp@(4), %d1		| get our OR mask
	clrl	%d0
1:	orw	%d1, _C_LABEL(enable_reg_soft)	| do our OR
	movew	_C_LABEL(enable_reg_soft), %d0	| get the result
	movsw	%d0, SYSTEM_ENAB			| install the result
	cmpw	_C_LABEL(enable_reg_soft), %d0
	bne	1b				| install it again if the soft value changed
	movc	%a1,%dfc		| restore dfc
	rts

/*
 * Use common m68k 16-bit aligned copy routines.
 */
#include <m68k/m68k/w16copy.s>

| Define some addresses, mostly so DDB can print useful info.
| Not using _C_LABEL() here because these symbols are never
| referenced by any C code, and if the leading underscore
| ever goes away, these lines turn into syntax errors...
	.set	_KERNBASE,KERNBASE
	.set	_MONSTART,SUN2_MONSTART
	.set	_PROM_BASE,SUN2_PROM_BASE
	.set	_MONEND,SUN2_MONEND

|The end!
