/*	$NetBSD: locore.s,v 1.42 2026/03/22 22:23:56 thorpej Exp $	*/

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

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_fpsp.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_m68k_arch.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

#include "ksyms.h"

/*
 * Memory starts at 0x0000.0000, and we have linked the kernel
 * at 0x0000.2000 to ensure that VA==0 is unmapped once we turn
 * on the MMU.  We arrive here running VA==PA and with the MMU
 * disabled.
 *
 * This first 8KB of RAM at PA==0 won't go to waste, though; we
 * will use it for the kernel message buffer.
 */

/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	PAGE_SIZE
ASLOCAL(tmpstk)

/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#define	IMMEDIATE		#
#define	_RELOC(var, ar)			\
	movl	IMMEDIATE var,ar;	\
	addl	%a5,ar

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

BSS(esym,4)

	.globl	_C_LABEL(edata)
	.globl	_C_LABEL(etext),_C_LABEL(end)

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

/*
 * start of kernel and .text!
 */
ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

	/*
	 * Determine our relocation offset.  We need this to manually
	 * translate the virtual addresses of global references to the
	 * physical addresses we need to use before the MMU is enabled.
	 */
	lea	%pc@(_ASM_LABEL(start)), %a5
	movl	%a5,%d0			| %d0 = phys address of start
	subl	#_ASM_LABEL(start), %d0	| %d0 -= virt address of start
	movl	%d0, %a5		| %a5 = relocation offset

	/*
	 * NOTE: %a5 cannot be used until after the MMU is enabled; it
	 * is used by the RELOC() macro.
	 */

	ASRELOC(tmpstk, %a0)
	movl	%a0,%sp			| give ourselves a temporary stack

	RELOC(edata,%a0)		| clear out BSS
	movl	#_C_LABEL(end) - 4, %d0	| (must be <= 256 kB)
	subl	#_C_LABEL(edata), %d0
	lsrl	#2,%d0
1:	clrl	%a0@+
	dbra	%d0,1b

	/*
	 * Qemu does not pass us the symbols, so leave esym alone.
	 * The bootinfo immediately follows the kernel.  Go parse
	 * it to get CPU/FPU/MMU information and figure out where
	 * the end of the loaded image really is.
	 */
	movl	#_C_LABEL(end),%a4	| end of static kernel text/data
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| reloff
	pea	%a4@			| endpa
	RELOC(bootinfo_startup1,%a0)
	jbsr	%a0@			| bootinfo_startup1(endpa, reloff)
	addql	#8,%sp

	/*
	 * End of boot info returned in %d0.  That represents the
	 * end of the loaded image.
	 */
	pea	%a5@			| reloff
	movl	%d0,%sp@-		| nextpa
	RELOC(pmap_bootstrap1,%a0)
	jbsr	%a0@			| pmap_bootstrap1(nextpa, reloff)
	addql	#8,%sp

	/*
	 * Updated nextpa returned in %d0.  We need to squirrel
	 * that away in a callee-saved register to use later,
	 * after the MMU is enabled.
	 */
	movl	%d0, %d7

	/* NOTE: %d7 is now off-limits!! */

/*
 * Enable the MMU.
 * Since the kernel is mapped logical == physical, there is no prep
 * work to do.
 */
#include <m68k/m68k/mmu_enable.s>

/*
 * Should be running mapped from this point on
 */
Lmmuenabled:
	lea	_ASM_LABEL(tmpstk),%sp	| re-load the temporary stack
	jbsr	_C_LABEL(vec_init)	| initialize the vector table

	/* phase 2 of pmap setup, returns lwp0 SP in %a0 */
	jbsr	_C_LABEL(pmap_bootstrap2)
	movl	%a0,%sp			| now running on lwp0's stack

	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jeq	1f			| yes, cache already on
	pflusha
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
	jra	2f
1:
	.word	0xf518			| pflusha
2:
	movl	%d7,%sp@-		| push nextpa saved above
	jbsr	_C_LABEL(machine_init)	| additional pre-main initialization
	addql	#4,%sp
	jra	_C_LABEL(main)		| main() (never returns)

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

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
	movw	%sp@(FR_HW),%d1		| get PSW
	andw	#PSL_S,%d1		| from system mode?
	jne	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

Lkbrkpt: | Kernel-mode breakpoint or trace trap. (d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If were are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	Lbrkpt2			| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0			| a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1 | a1=dst
	movl	%a1,%sp			| sp=new frame
	movql	#FR_SIZE,%d1
Lbrkpt1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	jbgt	Lbrkpt1

Lbrkpt2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to do it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	| If we have both DDB and KGDB, let KGDB see it first,
	| because KGDB will just return 0 if not connected.
	| Save args in d2, a2
	movl	%d0,%d2			| trap type
	movl	%sp,%a2			| frame ptr
#ifdef KGDB
	| Let KGDB handle it (if connected)
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kgdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
	cmpl	#0,%d0			| did kgdb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#ifdef DDB
	| Let DDB handle it
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
#endif
	/* Sun 3 drops into PROM here. */
Lbrkpt3:
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
 * Interrupt handlers.
 *
 * For auto-vectored interrupts, the CPU provides the
 * vector 0x18+level.
 *
 * intrhand_autovec is the entry point for auto-vectored
 * interrupts.
 */

ENTRY_NOPROFILE(intrhand_autovec)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(intr_dispatch)	| call dispatcher
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		| all done

/*
 * Primitives
 */

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#define	FPCOPROC	/* XXX: Temp. Reqd. */
#include <m68k/m68k/switch_subr.s>

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

ENTRY(paravirt_membar_sync)
	/*
	 * Store-before-load ordering with respect to matching logic
	 * on the hypervisor side.
	 *
	 * This is the same as membar_sync, but guaranteed never to be
	 * conditionalized or hotpatched away even on uniprocessor
	 * builds and boots -- because under virtualization, we still
	 * have to coordinate with a `device' backed by a hypervisor
	 * that is potentially on another physical CPU even if we
	 * observe only one virtual CPU as the guest.
	 *
	 * I don't see an obvious ordering-only instruction in the m68k
	 * instruction set, but qemu implements CAS with
	 * store-before-load ordering, so this should work for virtio.
	 */
	clrl	%d0
	casl	%d0,%d0,%sp@
	rts
END(paravirt_membar_sync)

/*
 * Misc. global variables.
 */
	.data

GLOBAL(mmutype)
	.long	MMU_68040	| default to MMU_68040

GLOBAL(cputype)
	.long	CPU_68040	| default to CPU_68040
