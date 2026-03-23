/*	$NetBSD: locore.s,v 1.86 2026/03/23 02:43:46 thorpej Exp $	*/

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
 *	from: Utah $Hdr: locore.s 1.66 92/12/22$
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

| Remember this is a fun project!

	.data
GLOBAL(mon_crp)
	.long	0,0

| This is for kvm_mkdb, and should be the address of the beginning
| of the kernel text segment (not necessarily the same as kernbase).
	.text
GLOBAL(kernel_text)

| This is the entry point, as well as the end of the temporary stack
| used during process switch (one 8K page ending at start)
ASGLOBAL(tmpstk)
ASGLOBAL(start)

| The first step, after disabling interrupts, is to map enough of the kernel
| into high virtual address space so that we can use position dependent code.
| This is a tricky task on the sun3x because the MMU is already enabled and
| the ROM monitor provides no indication of where the root MMU table is mapped.
| Therefore we must use one of the 68030's 'transparent translation' registers
| to define a range in the address space where the MMU translation is
| turned off.  Once this is complete we can modify the MMU table directly
| without the need for it to be mapped into virtual memory.
| All code must be position independent until otherwise noted, as the
| boot loader has loaded us into low memory but all the symbols in this
| code have been linked high.
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	movl	#KERNBASE3X,%a5		| for vtop conversion
	lea	_C_LABEL(mon_crp),%a0	| where to store the CRP
	subl	%a5,%a0
	| Note: borrowing mon_crp for tt0 setup...
	movl	#0x3F8107,%a0@		| map the low 1GB v=p with the
	.long	0xf0100800		| transparent translation reg0
					| [ pmove a0@, tt0 ]
| In order to map the kernel into high memory we will copy the root table
| entry which maps the 16 megabytes of memory starting at 0x0 into the
| entry which maps the 16 megabytes starting at KERNBASE.
	pmove	%crp,%a0@		| Get monitor CPU root pointer
	movl	%a0@(4),%a1		| 2nd word is PA of level A table

	movl	%a1,%a0			| compute the descriptor address
	addl	#0x3e0,%a1		| for VA starting at KERNBASE
	movl	%a0@,%a1@		| copy descriptor type
	movl	%a0@(4),%a1@(4)		| copy physical address

| Kernel is now double mapped at zero and KERNBASE.
| Force a long jump to the relocated code (high VA).
	movl	#IC_CLEAR,%d0		| Flush the I-cache
	movc	%d0,%cacr
	jmp L_high_code:l		| long jump

L_high_code:
| We are now running in the correctly relocated kernel, so
| we are no longer restricted to position-independent code.
| It is handy to leave transparent translation enabled while
| for the low 1GB while _bootstrap() is doing its thing.

| Do bootstrap stuff needed before main() gets called.
| Our boot loader leaves a copy of the kernel's exec header
| just before the start of the kernel text segment, so the
| kernel can sanity-check the DDB symbols at [end...esym].
| Pass the struct exec at tmpstk-32 to _bootstrap().
| Also, make sure the initial frame pointer is zero so that
| the backtrace algorithm used by KGDB terminates nicely.
|
| _bootstrap() returns lwp0 SP in %a0
	lea	_ASM_LABEL(tmpstk)-32,%sp
	jsr	_C_LABEL(_bootstrap)	| See locore2.c
	movl	%a0,%sp			| now running on lwp0's stack
	movl	#0,%a6			| terminate the stack back trace

| Now turn off the transparent translation of the low 1GB.
| (this also flushes the ATC)
	clrl	%sp@-
	.long	0xf0170800		| pmove	sp@,tt0
	addql	#4,%sp

	jra	_C_LABEL(main)		| main() (never returns)

| That is all the assembly startup code we need on the sun3x!
| The rest of this is like the hp300/locore.s where possible.

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

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
	lea	_ASM_LABEL(tmpstk)-96,%a1 | %a1=dst
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

/*
 * Interrupt handlers.  Most are auto-vectored,
 * and hard-wired the same way on all sun3 models.
 * Format in the stack is:
 *   %d0,%d1,%a0,%a1, sr, pc, vo
 */

/* clock: see clock.c */
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
GLOBAL(_isr_clock)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(clock_intr)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)

/*
 * Initialization is at the beginning of this file, because the
 * kernel entry point needs to be at zero for compatibility with
 * the Sun boot loader.  This works on Sun machines because the
 * interrupt vector table for reset is NOT at address zero.
 * (The MMU has a "boot" bit that forces access to the PROM)
 */

/*
 * Primitives
 */

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#define FPCOPROC	/* XXX: Temp. Reqd. */
#include <m68k/m68k/switch_subr.s>


#ifdef DEBUG
	.data
ASGLOBAL(fulltflush)
	.long	0
ASGLOBAL(fullcflush)
	.long	0
	.text
#endif

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Load a new CPU Root Pointer (CRP) into the MMU.
 *	void	loadcrp(struct mmu_rootptr *);
 */
ENTRY(loadcrp)
	movl	%sp@(4),%a0		| arg1: &CRP
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	pflusha				| flush entire TLB
	pmove	%a0@,%crp		| load new user root pointer
	rts

ENTRY(getcrp)
	movl	%sp@(4),%a0		| arg1: &crp
	pmove	%crp,%a0@		| *crpp = %crp
	rts

/*
 * Get the physical address of the PTE for a given VA.
 */
ENTRY(ptest_addr)
	movl	%sp@(4),%a1		| VA
	ptestr	#5,%a1@,#7,%a0		| %a0 = addr of PTE
	movl	%a0,%d0			| Result in %d0 (not a pointer return)
	rts

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
	jra	L_delay			/* Jump into the loop! */

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

| Define some addresses, mostly so DDB can print useful info.
| Not using _C_LABEL() here because these symbols are never
| referenced by any C code, and if the leading underscore
| ever goes away, these lines turn into syntax errors...
	.set	_KERNBASE3X,KERNBASE3X
	.set	_MONSTART,SUN3X_MONSTART
	.set	_PROM_BASE,SUN3X_PROM_BASE
	.set	_MONEND,SUN3X_MONEND

|The end!
