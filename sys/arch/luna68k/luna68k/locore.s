/* $NetBSD: locore.s,v 1.59 2014/03/22 16:52:07 tsutsui Exp $ */

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
#include "opt_ddb.h"
#include "opt_fpsp.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_fpu_emulate.h"
#include "opt_m68k_arch.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

#include "ksyms.h"

#define	PRINT(msg) \
	pea	9f		; \
	jbsr	_C_LABEL(printf); \
	addl	#4,%sp		; \
	.data			; \
     9:	.asciz	msg		; \
	.text
#undef	PRINT

/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	PAGE_SIZE
ASLOCAL(tmpstk)

#include <luna68k/luna68k/vectors.s>

/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#define	_RELOC(var,ar)		\
	lea	var,ar;		\
	addl	%a5,ar
#define	RELOC(var,ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var,ar)	_RELOC(_ASM_LABEL(var), ar)

BSS(lowram,4)
BSS(esym,4)

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
	movl	#0,%a5			| RAM starts at 0
	ASRELOC(tmpstk,%a0)
	movl	%a0,%sp			| give ourselves a temporary stack

	RELOC(edata,%a0)		| clear out BSS
	movl	#_C_LABEL(end)-4,%d0	| (must be <= 256 kB)
	subl	#_C_LABEL(edata),%d0
	lsrl	#2,%d0
1:	clrl	%a0@+
	dbra	%d0,1b

	RELOC(boothowto,%a0)
	movl	%d7,%a0@		| save boothowto
	RELOC(bootdev,%a0)
	movl	%d6,%a0@		| save bootdev
	RELOC(lowram,%a0)
	movl	%a5,%a0@		| store start of physical memory

	movl	#0x41000000,%a0		| available memory in bytes
	movl	%a0@(12),%a0		| (int *)base[3])
	movl	%a0@,%d5
	RELOC(memavail,%a0)
	movl	%d5,%a0@		| save memavail

	movl	#0x41000000,%a0		| planemask; 0x0f or 0xff
	movl	%a0@(184),%a0		| (int *)base[46]
	movl	%a0@,%d5
	RELOC(hwplanemask,%a0)
	movl	%d5,%a0@		| save hwplanemask

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

/* determine our CPU/MMU combo - check for all regardless of kernel config */
	movl	#DC_FREEZE,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68040
	movl	#CPU_68030,%d0
	movl	#MMU_68030,%d1
	movl	#FPU_68881,%d2
	jra	Lstart0
Lnot68030:
	movl	#CPU_68040,%d0
	movl	#MMU_68040,%d1
	movl	#FPU_68040,%d2
Lstart0:
	RELOC(cputype,%a0)
	movl	%d0,%a0@
	RELOC(mmutype,%a0)
	movl	%d1,%a0@
	RELOC(fputype,%a0)
	movl	%d2,%a0@

	/*
	 * save argument of 'x' command on boot per machine type
	 * XXX: assume CPU_68040 is LUNA-II
	 */
	movl	#0x41000000,%a0
	cmpl	#CPU_68040,%d0		| 68040?
	jne	1f			| no, assume 68030 LUNA
	movl	%a0@(8),%a0		| arg at (char *)base[2] on LUNA-II
	jra	Lstart1
1:
	movl	%a0@(212),%a0		| arg at (char *)base[53] on LUNA
Lstart1:
	RELOC(bootarg,%a1)
	movl	#63,%d0
1:	movb	%a0@+,%a1@+		| copy to bootarg
	dbra	%d0,1b			| upto 63 characters

	/*
	 * Now that we know what CPU we have, initialize the address error
	 * and bus error handlers in the vector table:
	 *
	 *	vectab+8	bus error
	 *	vectab+12	address error
	 */
	lea	_C_LABEL(cputype),%a0
	lea	_C_LABEL(vectab),%a2
#if defined(M68040)
	cmpl	#CPU_68040,%a0@		| 68040?
	jne	1f			| no, skip
	movl	#_C_LABEL(buserr40),%a2@(8)
	movl	#_C_LABEL(addrerr4060),%a2@(12)
	jra	Lstart2
1:
#endif
	cmpl	#CPU_68030,%a0@		| 68030?
	jne	1f			| no, skip
	movl	#_C_LABEL(busaddrerr2030),%a2@(8)
	movl	#_C_LABEL(busaddrerr2030),%a2@(12)
	jra	Lstart2
1:
	/* Config botch; no hope. */
	PANIC("Config botch in locore")

Lstart2:
/* initialize source/destination control registers for movs */
	moveq	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers
/* initialize memory sizes (for pmap_bootstrap) */
	RELOC(memavail,%a0)
	movl	%a0@,%d1
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page (click) number
	RELOC(maxmem,%a0)
	movl	%d1,%a0@		| save as maxmem
	RELOC(physmem,%a0)
	movl	%d1,%a0@		| and physmem

/* check if symbol table is loaded and set esym address */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	RELOC(end,%a0)
	pea	%a0@
	RELOC(_C_LABEL(symtab_size),%a0)
	jbsr	%a0@			| symtab_size(end)
	addql	#4,%sp
	tstl	%d0			| check if valid symtab is loaded
	jeq	1f			|  no, skip
	lea	_C_LABEL(end),%a0	| calculate end of symtab address
	addl	%a0,%d0
#else
	clrl	%d0			| no symbol table
#endif
1:
	RELOC(esym,%a0)
	movl	%d0,%a0@

/* configure kernel and lwp0 VA space so we can get going */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	RELOC(esym,%a0)			| end of static kernel test/data/syms
	movl	%a0@,%d2
	jne	Lstart3
#endif
	RELOC(end,%a0)
	movl	%a0,%d2			| end of static kernel text/data
Lstart3:
	addl	#PAGE_SIZE-1,%d2
	andl	#PG_FRAME,%d2		| round to a page
	movl	%d2,%a4
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| firstpa
	pea	%a4@			| nextpa
	RELOC(pmap_bootstrap,%a0)
	jbsr	%a0@			| pmap_bootstrap(firstpa, nextpa)
	addql	#8,%sp
/*
 * Enable the MMU.
 * Since the kernel is mapped logical == physical, we just turn it on.
 */
	RELOC(Sysseg_pa,%a0)		| system segment table addr
	movl	%a0@,%d1		| read value (a PA)
#if defined(M68040)
	RELOC(mmutype,%a0)
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu1		| no, skip
Lmotommu0:
	.long	0x4e7b1807		| movc %d1,%srp
	RELOC(proto040tt0,%a0)
	movl	%a0@,%d0		| tt0 range 4000.0000-7fff.ffff
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0006		| movc %d0,%dtt0
	RELOC(proto040tt1,%a0)
	movl	%a0@,%d0		| tt1 range 8000.0000-ffff.ffff
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0007		| movc %d0,%dtt1
	.word	0xf4d8			| cinva bc
	.word	0xf518			| pflusha
	RELOC(proto040tc,%a0)
	movl	%a0@,%d0
	.long	0x4e7b0003		| movc %d0,%tc
	movl	#CACHE40_ON,%d0
	movc	%d0,%cacr		| turn on both caches
	jmp	Lenab1
Lmotommu1:
#endif
	RELOC(protosrp,%a0)		| nolimit + share global + 4 byte PTEs
	movl	%d1,%a0@(4)		| + segtable address
	RELOC(protocrp,%a1)
	movl	%d1,%a1@(4)		| set lower half of %CRP
	pmove	%a0@,%srp		| load the supervisor root pointer
	RELOC(protott0,%a0)		| tt0 range 4000.0000-7fff.ffff
	.long	0xf0100800		| pmove %a0@,mmutt0
	RELOC(protott1,%a0)		| tt1 range 8000.0000-ffff.ffff
	.long	0xf0100c00		| pmove %a0@,mmutt1
	pflusha
	RELOC(prototc,%a0)		| %tc: SRP,CRP,4KB page,A=10bit,B=10bit
	pmove	%a0@,%tc
/*
 * Should be running mapped from this point on
 */
Lenab1:
	lea	_ASM_LABEL(tmpstk),%sp	| temporary stack
/* call final pmap setup */
	jbsr	_C_LABEL(pmap_bootstrap_finalize)
/* set kernel stack, user SP */
	movl	_C_LABEL(lwp0uarea),%a1	| get lwp0 uarea
	lea	%a1@(USPACE-4),%sp	| set kernel stack to end of area
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP

/* detect FPU type */
	jbsr	_C_LABEL(fpu_probe)
	movl	%d0,_C_LABEL(fputype)
	tstl	_C_LABEL(fputype)	| Have an FPU?
	jeq	Lenab2			| No, skip.
	clrl	%a1@(PCB_FPCTX)		| ensure null FP context
	movl	%a1,%sp@-
	jbsr	_C_LABEL(m68881_restore) | restore it (does not kill %a1)
	addql	#4,%sp
Lenab2:
	jbsr	_C_LABEL(_TBIA)		| invalidate TLB
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lenab3			| yes, cache already on
	tstl	_C_LABEL(mmutype)
	jpl	Lenab3			| 68851 implies no d-cache
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
Lenab3:

/* final setup for C code */
	movl	#_C_LABEL(vectab),%d0	| get our %vbr address
	movc	%d0,%vbr
	jbsr	_C_LABEL(luna68k_init)	| additional pre-main initialization

/*
 * Create a fake exception frame that returns to user mode,
 * and save its address in p->p_md.md_regs for cpu_lwp_fork().
 * The new frames for process 1 and 2 will be adjusted by
 * cpu_set_kpc() to arrange for a call to a kernel function
 * before the new process does its rte out to user mode.
 */
	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count and padding
	lea	%sp@(-64),%sp		| construct space for D0-D7/A0-A7
	lea	_C_LABEL(lwp0),%a0	| save pointer to frame
	movl	%sp,%a0@(L_MD_REGS)	|   in lwp0.l_md.md_regs

	jra	_C_LABEL(main)		| main()
	PANIC("main() returned")
	/* NOTREACHED */

/*
 * Trap/interrupt vector routines
 */
#include <m68k/m68k/trap_subr.s>

/*
 * Use common m68k bus error and address error handlers.
 */
#include <m68k/m68k/busaddrerr.s>

/*
 * FP exceptions.
 */
ENTRY_NOPROFILE(fpfline)
#if defined(M68040)
	cmpl	#FPU_68040,_C_LABEL(fputype) | 68040 FPU?
	jne	Lfp_unimp		| no, skip FPSP
	cmpw	#0x202c,%sp@(6)		| format type 2?
	jne	_C_LABEL(illinst)	| no, not an FP emulation
Ldofp_unimp:
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unimp)	| yes, go handle it
#endif
Lfp_unimp:
#endif /* M68040 */
#ifdef FPU_EMULATE
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULI,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#else
	jra	_C_LABEL(illinst)
#endif

ENTRY_NOPROFILE(fpunsupp)
#if defined(M68040)
	cmpl	#FPU_68040,_C_LABEL(fputype) | 68040 FPU?
	jne	_C_LABEL(illinst)	| no, treat as illinst
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unsupp)	| yes, go handle it
#endif
Lfp_unsupp:
#endif /* M68040 */
#ifdef FPU_EMULATE
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#else
	jra	_C_LABEL(illinst)
#endif

/*
 * Handles all other FP coprocessor exceptions.
 * Note that since some FP exceptions generate mid-instruction frames
 * and may cause signal delivery, we need to test for stack adjustment
 * after the trap call.
 */
ENTRY_NOPROFILE(fpfault)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	clrl	%sp@-			| no VA arg
	movl	_C_LABEL(curpcb),%a0	| current pcb
	lea	%a0@(PCB_FPCTX),%a0	| address of FP savearea
	fsave	%a0@			| save state
#if defined(M68040) || defined(M68060)
	/* always null state frame on 68040, 68060 */
	cmpl	#FPU_68040,_C_LABEL(fputype)
	jge	Lfptnull
#endif
	tstb	%a0@			| null state frame?
	jeq	Lfptnull		| yes, safe
	clrw	%d0			| no, need to tweak BIU
	movb	%a0@(1),%d0		| get frame size
	bset	#3,%a0@(0,%d0:w)	| set exc_pend bit of BIU
Lfptnull:
	fmovem	%fpsr,%sp@-		| push %fpsr as code argument
	frestore %a0@			| restore state
	movl	#T_FPERR,%sp@-		| push type arg
	jra	_ASM_LABEL(faultstkadj)	| call trap and deal with stack cleanup

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(badtrap)
	moveml	#0xC0C0,%sp@-		| save scratch regs
	movw	%sp@(22),%sp@-		| push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		| and PC
	jbsr	_C_LABEL(straytrap)	| report
	addql	#8,%sp			| pop args
	moveml	%sp@+,#0x0303		| restore regs
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(trap0)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%d0,%sp@-		| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall arg
	tstl	_C_LABEL(astpending)	| AST pending?
	jne	Lrei			| yes, handle it via trap
	movl	%sp@(FR_SP),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most registers
	addql	#8,%sp			| pop SP and stack adjust
	rte

/*
 * Trap 12 is the entry point for the cachectl "syscall" (both HPUX & BSD)
 *	cachectl(command, addr, length)
 * command in %d0, addr in %a1, length in %d1
 */
ENTRY_NOPROFILE(trap12)
	movl	_C_LABEL(curlwp),%a0
	movl	%a0@(L_PROC),%sp@-	| push current proc pointer
	movl	%d1,%sp@-		| push length
	movl	%a1,%sp@-		| push addr
	movl	%d0,%sp@-		| push command
	jbsr	_C_LABEL(cachectl1)	| do it
	lea	%sp@(16),%sp		| pop args
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
	movw	%sp@(FR_HW),%d1		| get PSW
	andw	#PSL_S,%d1		| from system mode?
	jne	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

Lkbrkpt: | Kernel-mode breakpoint or trace trap. (%d0=trap_type)
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
	movl	%sp,%a0			| %a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1 | %a1=dst
	movl	%a1,%sp			| %sp=new frame
	moveq	#FR_SIZE,%d1
Lbrkpt1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	jgt	Lbrkpt1

Lbrkpt2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to do it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	| If we have both DDB and KGDB, let KGDB see it first,
	| because KGDB will just return 0 if not connected.
	| Save args in %d2, %a2
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
#if 0	/* not needed on hp300 */
	cmpl	#0,%d0			| did ddb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#endif
	/* Sun 3 drops into PROM here. */
Lbrkpt3:
	| The stack pointer may have been modified, or
	| data below it modified (by kgdb push call),
	| so push the hardware frame at the current sp
	| before restoring registers and returning.

	movl	%sp@(FR_SP),%a0		| modified %sp
	lea	%sp@(FR_SIZE),%a1	| end of our frame
	movl	%a1@-,%a0@-		| copy 2 longs with
	movl	%a1@-,%a0@-		| ... predecrement
	movl	%a0,%sp@(FR_SP)		| %sp = h/w frame
	moveml	%sp@+,#0x7FFF		| restore all but %sp
	movl	%sp@,%sp		| ... and %sp
	rte				| all done

/* Use common m68k sigreturn */
#include <m68k/m68k/sigreturn.s>

/*
 * Interrupt handlers.
 *
 * For auto-vectored interrupts, the CPU provides the
 * vector 0x18+level.  Note we count spurious interrupts,
 * but don't do anything else with them.
 *
 * _intrhand_autovec is the entry point for auto-vectored
 * interrupts.
 *
 * For vectored interrupts, we pull the pc, evec, and exception frame
 * and pass them to the vectored interrupt dispatcher.  The vectored
 * interrupt dispatcher will deal with strays.
 *
 * _intrhand_vectored is the entry point for vectored interrupts.
 */

ENTRY_NOPROFILE(spurintr)		/* Level 0 */
	addql	#1,_C_LABEL(intrcnt)+0
	INTERRUPT_SAVEREG
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(intrhand_autovec)	/* Levels 1 through 6 */
	INTERRUPT_SAVEREG
	movw	%sp@(22),%sp@-		| push exception vector
	clrw	%sp@-
	jbsr	_C_LABEL(isrdispatch_autovec)	| call dispatcher
	addql	#4,%sp
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev7intr)		/* Level 7: NMI */
	addql	#1,_C_LABEL(intrcnt)+32
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-		| save registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	jbsr	_C_LABEL(nmihand)	| call handler
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(intrhand_vectored)
	INTERRUPT_SAVEREG
	lea	%sp@(16),%a1		| get pointer to frame
	movl	%a1,%sp@-
	movw	%sp@(26),%d0
	movl	%d0,%sp@-		| push exception vector info
	movl	%sp@(26),%sp@-		| and PC
	jbsr	_C_LABEL(isrdispatch_vectored)	| call dispatcher
	lea	%sp@(12),%sp		| pop value args
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

#if 1	/* XXX wild timer -- how can I disable/enable the interrupt? */
ENTRY_NOPROFILE(lev5intr)
	addql	#1,_C_LABEL(idepth)
	btst	#7,0x63000000		| check whether system clock
	beq	1f
	movb	#1,0x63000000		| clear the interrupt
	tstl	_C_LABEL(clock_enable)	| is hardclock() available?
	jeq	1f
	INTERRUPT_SAVEREG
	lea	%sp@(16),%a1		| %a1 = &clockframe
	movl	%a1,%sp@-
	jbsr	_C_LABEL(hardclock)	| hardclock(&frame)
	addql	#4,%sp
	addql	#1,_C_LABEL(intrcnt)+20
	INTERRUPT_RESTOREREG
1:
	subql	#1,_C_LABEL(idepth)
	jra	_ASM_LABEL(rei)		| all done
#endif

#undef INTERRUPT_SAVEREG
#undef INTERRUPT_RESTOREREG

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

ASENTRY_NOPROFILE(rei)
	tstl	_C_LABEL(astpending)	| AST pending?
	jne	1f			| no, done
	rte
1:
	btst	#5,%sp@			| yes, are we returning to user mode?
	jeq	2f			| no, done
	rte
2:
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
Lrei:
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
	rte				| and do real RTE

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
#define FPCOPROC	/* XXX: Temp. reqd. */
#include <m68k/m68k/switch_subr.s>


#if defined(M68040)
ENTRY(suline)
	movl	%sp@(4),%a0		| address to write
	movl	_C_LABEL(curpcb),%a1	| current pcb
	movl	#Lslerr,%a1@(PCB_ONFAULT) | where to return to on a fault
	movl	%sp@(8),%a1		| address of line
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	moveq	#0,%d0			| indicate no fault
	jra	Lsldone
Lslerr:
	moveq	#-1,%d0
Lsldone:
	movl	_C_LABEL(curpcb),%a1	| current pcb
	clrl	%a1@(PCB_ONFAULT)	| clear fault address
	rts
#endif

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
	movl	%sp@(4),%d0		| new USTP
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0			| convert to addr
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jne	LmotommuC		| no, skip
	.word	0xf518			| yes, pflusha
	.long	0x4e7b0806		| movc %d0,%urp
	rts
LmotommuC:
#endif
	pflusha				| flush entire TLB
	lea	_C_LABEL(protocrp),%a0	| %crp prototype
	movl	%d0,%a0@(4)		| stash USTP
	pmove	%a0@,%crp		| load root pointer
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	rts

ENTRY(ploadw)
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lploadwskp		| yes, skip
#endif
	movl	%sp@(4),%a0		| address to load
	ploadw	#1,%a0@			| pre-load translation
#if defined(M68040)
Lploadwskp:
#endif
	rts

ENTRY(getsr)
	moveq	#0,%d0
	movw	%sr,%d0
	rts

/*
 * _delay(u_int N)
 *
 * Delay for at least (N/256) microseconds.
 * This routine depends on the variable:  delay_divisor
 * which should be set based on the CPU clock rate.
 */
ENTRY_NOPROFILE(_delay)
	| %d0 = arg = (usecs << 8)
	movl	%sp@(4),%d0
	| %d1 = delay_divisor
	movl	_C_LABEL(delay_divisor),%d1
	jra	L_delay			/* Jump into the loop! */

	/*
	 * Align the branch target of the loop to a half-line (8-byte)
	 * boundary to minimize cache effects.  This guarantees both
	 * that there will be no prefetch stalls due to cache line burst
	 * operations and that the loop will run from a single cache
	 * half-line.
	 */
	.align	8
L_delay:
	subl	%d1,%d0
	jgt	L_delay
	rts

/*
 * Do a dump.
 * Called by auto-restart.
 */
GLOBAL(doadump)
	jbsr	_C_LABEL(dumpsys)
	jbsr	_C_LABEL(doboot)
	/*NOTREACHED*/

/*
 * Handle the nitty-gritty of rebooting the machine.
 * Basically we just turn off the MMU, restore the initial %vbr
 * and return to monitor.
 */
ENTRY_NOPROFILE(doboot)
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	movl	_C_LABEL(boothowto),%d7	| load howto
	movl	%sp@(4),%d2		| arg
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lnocache5		| yes, skip
#endif
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| disable on-chip cache(s)
	movl	#0,%a7@			| value for pmove to %tc
	pmove	%a7@,%tc		| disable MMU
	lea	_ASM_LABEL(nullrp),%a0
	pmove	%a0@,%crp		| Invalidate CPU root pointer
	pmove	%a0@,%srp		| and the Supervisor root pointer
	jra	Lbootcommon
#if defined(M68040)
Lnocache5:
	.word	0xf4f8			| cpusha bc
	movql	#0,%d0
	movc	%d0,%cacr
	.long	0x4e7b0003		| movc %d0,%tc (disable MMU)
	.long	0x4e7b0806		| movc %d0,%urp
	.long	0x4e7b0807		| movc %d0,%srp
#endif /* M68040 */
Lbootcommon:
	lea	_ASM_LABEL(tmpstk),%sp	| physical SP in case of NMI
	movl	#0,%d3
	movc	%d3,%vbr		| monitor %vbr

	movl	#0x41000000,%a0		| base = (int **)0x4100.0000
	movl	%a0@,%d0		| *((int *)base[0])
	movc	%d0,%isp		| set initial stack pointer
	movc	%d0,%msp		| set initial stack pointer
	movl	%a0@(4),%a0		| *((int (*)(void))base[1])
	jmp	%a0@			| go cold boot!

	.data
GLOBAL(cputype)
	.long	CPU_68030	| default to 68030
GLOBAL(mmutype)
	.long	MMU_68030	| default to 68030
GLOBAL(fputype)
	.long	FPU_68881	| default to 68881

GLOBAL(protosrp)
	.long	0x80000202,0	| prototype supervisor root pointer
GLOBAL(protocrp)
	.long	0x80000002,0	| prototype CPU root pointer

GLOBAL(prototc)
	.long	0x82c0aa00	| %tc (SRP,CRP,4KB page, TIA/TIB=10/10bits)
GLOBAL(protott0)		| tt0 0x4000.0000-0x7fff.ffff
	.long	0x403f8543	|
GLOBAL(protott1)		| tt1 0x8000.0000-0xffff.ffff
	.long	0x807f8543	|
GLOBAL(proto040tc)
	.long	0x8000		| %tc (4KB page)
GLOBAL(proto040tt0)		| tt0 0x4000.0000-0x7fff.ffff
	.long	0x403fa040	| kernel only, cache inhebit, serialized
GLOBAL(proto040tt1)		| tt1 0x8000.0000-0xffff.ffff
	.long	0x807fa040	| kernel only, cache inhebit, serialized
nullrp:
	.long	0x7fff0001	| do-nothing MMU root pointer

GLOBAL(memavail)
	.long	0
GLOBAL(bootdev)
	.long	0
GLOBAL(hwplanemask)
	.long	0
GLOBAL(bootarg)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

#ifdef DEBUG
ASGLOBAL(fulltflush)
	.long	0

ASGLOBAL(fullcflush)
	.long	0
#endif

GLOBAL(intiobase)
	.long	0		| KVA of base of internal IO space
GLOBAL(intiolimit)
	.long	0		| KVA of end of internal IO space
GLOBAL(intiobase_phys)
	.long	0		| PA of board's I/O registers
GLOBAL(intiotop_phys)
	.long	0		| PA of top of board's I/O registers

GLOBAL(intrnames)
	.asciz	"spur"
	.asciz	"lev1"
	.asciz	"scsi"
	.asciz	"network"
	.asciz	"lev4"
	.asciz	"clock"
	.asciz	"serial"
	.asciz	"nmi"
	.asciz	"statclock"
GLOBAL(eintrnames)
	.even
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)
