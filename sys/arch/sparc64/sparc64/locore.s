/*	$NetBSD: locore.s,v 1.80 2000/07/19 04:36:42 eeh Exp $	*/
/*
 * Copyright (c) 1996-1999 Eduardo Horvath
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College.
 *	All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.
 *	All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by Paul Kranenburg.
 * 4. Neither the name of the University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *	@(#)locore.s	8.4 (Berkeley) 12/10/93
 */
#define	DIAGNOSTIC

#undef	INTR_INTERLOCK		/* Use IH_PEND field to interlock interrupts */
#undef	PARANOID		/* Extremely expensive consistency checks */
#undef	NO_VCACHE		/* Map w/D$ disabled */
#define	TRAPTRACE		/* Keep history of all traps (unsafe) */
#undef	FLTRACE			/* Keep history of all page faults */
#define	TRAPSTATS		/* Count traps */
#undef	TRAPS_USE_IG		/* Use Interrupt Globals for all traps */
#define	HWREF			/* Track ref/mod bits in trap handlers */
#undef	MMUDEBUG		/* Check use of regs during MMU faults */
#define	VECTORED_INTERRUPTS	/* Use interrupt vectors */
#define	PMAP_FPSTATE		/* Allow nesting of VIS pmap copy/zero */
#define	NEW_FPSTATE
#define	PMAP_PHYS_PAGE		/* Use phys ASIs for pmap copy/zero */
#define	DCACHE_BUG		/* Flush D$ around ASI_PHYS accesses */
#undef	NO_TSB			/* Don't use TSB */
#undef	TICK_IS_TIME		/* Keep %tick synchronized with time */
#undef	SCHED_DEBUG

#include "opt_ddb.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_compat_netbsd32.h"
#include "opt_multiprocessor.h"

#include "assym.h"
#include <machine/param.h>
#include <sparc64/sparc64/intreg.h>
#include <sparc64/sparc64/timerreg.h>
#include <machine/ctlreg.h>
#include <machine/psl.h>
#include <machine/signal.h>
#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_syscall.h>
#endif
#ifdef COMPAT_SVR4
#include <compat/svr4/svr4_syscall.h>
#endif
#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32_syscall.h>
#endif
#include <machine/asm.h>

#undef	CURPROC
#undef	CPCB
#undef	FPPROC
#ifndef MULTIPROCESSOR
#define	CURPROC	_C_LABEL(curproc)
#define CPCB	_C_LABEL(cpcb)
#define	FPPROC	_C_LABEL(fpproc)
#else
#define	CURPROC	(CPUINFO_VA+CI_CURPROC)
#define CPCB	(CPUINFO_VA+CI_CPCB)
#define	FPPROC	(CPUINFO_VA+CI_FPPROC)
#endif

/* Let us use same syntax as C code */
#define Debugger()	ta	1; nop

#if 1
/*
 * Try to issue an elf note to ask the Solaris
 * bootloader to align the kernel properly.
 */
	.section	.note
	.word	0x0d
	.word	4		! Dunno why
	.word	1
0:	.asciz	"SUNW Solaris"
1:
	.align	4
	.word	0x0400000
#endif

/*
 * Here are some defines to try to maintain consistency but still
 * support 32-and 64-bit compilers.
 */
#ifdef _LP64
/* reg that points to base of data/text segment */
#define	BASEREG	%g4
/* first constants for storage allocation */
#define PTRSZ	8
#define PTRSHFT	3
#define	POINTER	.xword
/* Now instructions to load/store pointers */
#define LDPTR	ldx
#define LDPTRA	ldxa
#define STPTR	stx
#define STPTRA	stxa
#define	CASPTR	casxa
/* Now something to calculate the stack bias */
#define STKB	BIAS
#else
#define	BASEREG	%g0
#define PTRSZ	4
#define PTRSHFT	2
#define POINTER	.word
#define LDPTR	lduw
#define LDPTRA	lduwa
#define STPTR	stw
#define STPTRA	stwa
#define	CASPTR	casa
#define STKB	0
#endif

/*
 * GNU assembler does not understand `.empty' directive; Sun assembler
 * gripes about labels without it.  To allow cross-compilation using
 * the Sun assembler, and because .empty directives are useful
 * documentation, we use this trick.
 */
#ifdef SUN_AS
#define	EMPTY	.empty
#else
#define	EMPTY	/* .empty */
#endif

/* use as needed to align things on longword boundaries */
#define	_ALIGN	.align 8
#define ICACHE_ALIGN	.align	32

/* Give this real authority: reset the machine */
#if 1
#define NOTREACHED	sir
#else
#define NOTREACHED
#endif

/*
 * This macro will clear out a cache line before an explicit
 * access to that location.  It's mostly used to make certain
 * loads bypassing the D$ do not get stale D$ data.
 *
 * It uses a register with the address to clear and a temporary
 * which is destroyed.
 */
#ifdef DCACHE_BUG
#define DLFLUSH(a,t) \
	andn	a, 0x1f, t; \
	stxa	%g0, [ t ] ASI_DCACHE_TAG; \
	membar	#Sync
/* The following can be used if the pointer is 16-byte aligned */
#define DLFLUSH2(t) \
	stxa	%g0, [ t ] ASI_DCACHE_TAG; \
	membar	#Sync
#else
#define DLFLUSH(a,t)
#define DLFLUSH2(t)
#endif


/*
 * A handy macro for maintaining instrumentation counters.
 * Note that this clobbers %o0 and %o1.  Normal usage is
 * something like:
 *	foointr:
 *		TRAP_SETUP(...)		! makes %o registers safe
 *		INCR(_C_LABEL(cnt)+V_FOO)	! count a foo
 */
#define INCR(what) \
	sethi	%hi(what), %o0; \
	ldsw	[%o0 + %lo(what)], %o1; \
	inc	%o1; \
	stw	%o1, [%o0 + %lo(what)]

/*
 * A couple of handy macros to save and restore globals to/from
 * locals.  Since udivrem uses several globals, and it's called
 * from vsprintf, we need to do this before and after doing a printf.
 */
#define GLOBTOLOC \
	mov	%g1, %l1; \
	mov	%g2, %l2; \
	mov	%g3, %l3; \
	mov	%g4, %l4; \
	mov	%g5, %l5; \
	mov	%g6, %l6; \
	mov	%g7, %l7

#define LOCTOGLOB \
	mov	%l1, %g1; \
	mov	%l2, %g2; \
	mov	%l3, %g3; \
	mov	%l4, %g4; \
	mov	%l5, %g5; \
	mov	%l6, %g6; \
	mov	%l7, %g7

/* Load strings address into register; NOTE: hidden local label 99 */
#define LOAD_ASCIZ(reg, s)	\
	set	99f, reg ;	\
	.data ;			\
99:	.asciz	s ;		\
	_ALIGN ;		\
	.text

/*
 * Handy stack conversion macros.
 * They correctly switch to requested stack type
 * regardless of the current stack.
 */

#define TO_STACK64(size)					\
	andcc	%sp, 1, %g0; /* 64-bit stack? */		\
	save	%sp, size, %sp;					\
	add	%sp, -BIAS, %o0; /* Convert to 64-bits */	\
	movz	%icc, %o0, %sp

#define TO_STACK32(size)					\
	andcc	%sp, 1, %g0; /* 64-bit stack? */		\
	save	%sp, size, %sp;					\
	add	%sp, +BIAS, %o0; /* Convert to 32-bits */	\
	movnz	%icc, %o0, %sp

#ifdef _LP64
#define	STACKFRAME(size)	TO_STACK64(size)
#else
#define	STACKFRAME(size)	TO_STACK32(size)
#endif


	.data
	.globl	_C_LABEL(data_start)
_C_LABEL(data_start):					! Start of data segment
#define DATA_START	_C_LABEL(data_start)

/*
 * The interrupt stack.
 *
 * This is the very first thing in the data segment, and therefore has
 * the lowest kernel stack address.  We count on this in the interrupt
 * trap-frame setup code, since we may need to switch from the kernel
 * stack to the interrupt stack (iff we are not already on the interrupt
 * stack).  One sethi+cmp is all we need since this is so carefully
 * arranged.
 */
#if 0
	.globl	_C_LABEL(intstack)
	.globl	_C_LABEL(eintstack)
_C_LABEL(intstack):
	.space	(2*USPACE)
_C_LABEL(eintstack):
#endif

/*
 * When a process exits and its u. area goes away, we set cpcb to point
 * to this `u.', leaving us with something to use for an interrupt stack,
 * and letting all the register save code have a pcb_uw to examine.
 * This is also carefully arranged (to come just before u0, so that
 * process 0's kernel stack can quietly overrun into it during bootup, if
 * we feel like doing that).
 */
	.globl	_C_LABEL(idle_u)
_C_LABEL(idle_u):
	.space	USPACE

/*
 * Process 0's u.
 *
 * This must be aligned on an 8 byte boundary.
 */
#if 0
	.globl	_C_LABEL(u0)
_C_LABEL(u0):	.space	(2*USPACE)
estack0:
#else
	.globl	_C_LABEL(u0)
_C_LABEL(u0):	POINTER	0
estack0:	POINTER	0
#endif

#ifdef KGDB
/*
 * Another item that must be aligned, easiest to put it here.
 */
KGDB_STACK_SIZE = 2048
	.globl	_C_LABEL(kgdb_stack)
_C_LABEL(kgdb_stack):
	.space	KGDB_STACK_SIZE		! hope this is enough
#endif

#ifdef DEBUG
/*
 * This stack is used when we detect kernel stack corruption.
 */
	.space	USPACE
	.align	16
panicstack:
#endif

/*
 * _cpcb points to the current pcb (and hence u. area).
 * Initially this is the special one.
 */
	.globl	_C_LABEL(cpcb)
_C_LABEL(cpcb):	POINTER	_C_LABEL(u0)

/*
 * romp is the prom entry pointer
 */
	.globl	romp
romp:	POINTER	0


/* NB:	 Do we really need the following around? */
/*
 * _cputyp is the current cpu type, used to distinguish between
 * the many variations of different sun4* machines. It contains
 * the value CPU_SUN4, CPU_SUN4C, or CPU_SUN4M.
 */
	.globl	_C_LABEL(cputyp)
_C_LABEL(cputyp):
	.word	1
/*
 * _cpumod is the current cpu model, used to distinguish between variants
 * in the Sun4 and Sun4M families. See /sys/arch/sparc64/include/param.h
 * for possible values.
 */
	.globl	_C_LABEL(cpumod)
_C_LABEL(cpumod):
	.word	1
/*
 * _mmumod is the current mmu model, used to distinguish between the
 * various implementations of the SRMMU in the sun4m family of machines.
 * See /sys/arch/sparc64/include/param.h for possible values.
 */
	.globl	_C_LABEL(mmumod)
_C_LABEL(mmumod):
	.word	0

/*
 * There variables are pointed to by the cpp symbols PGSHIFT, NBPG,
 * and PGOFSET.
 */
	.globl	_C_LABEL(pgshift), _C_LABEL(nbpg), _C_LABEL(pgofset)
_C_LABEL(pgshift):
	.word	0
_C_LABEL(nbpg):
	.word	0
_C_LABEL(pgofset):
	.word	0

	_ALIGN

	.text

/*
 * The v9 trap frame is stored in the special trap registers.  The
 * register window is only modified on window overflow, underflow,
 * and clean window traps, where it points to the register window
 * needing service.  Traps have space for 8 instructions, except for
 * the window overflow, underflow, and clean window traps which are
 * 32 instructions long, large enough to in-line.
 *
 * The spitfire CPU (Ultra I) has 4 different sets of global registers.
 * (blah blah...)
 *
 * I used to generate these numbers by address arithmetic, but gas's
 * expression evaluator has about as much sense as your average slug
 * (oddly enough, the code looks about as slimy too).  Thus, all the
 * trap numbers are given as arguments to the trap macros.  This means
 * there is one line per trap.  Sigh.
 *
 * Hardware interrupt vectors can be `linked'---the linkage is to regular
 * C code---or rewired to fast in-window handlers.  The latter are good
 * for unbuffered hardware like the Zilog serial chip and the AMD audio
 * chip, where many interrupts can be handled trivially with pseudo-DMA
 * or similar.  Only one `fast' interrupt can be used per level, however,
 * and direct and `fast' interrupts are incompatible.  Routines in intr.c
 * handle setting these, with optional paranoia.
 */

/*
 *	TA8 -- trap align for 8 instruction traps
 *	TA32 -- trap align for 32 instruction traps
 */
#define TA8	.align 32
#define TA32	.align 128

/*
 * v9 trap macros:
 *
 *	We have a problem with v9 traps; we have no registers to put the
 *	trap type into.  But we do have a %tt register which already has
 *	that information.  Tryap types in these macros are all dummys.
 */
	/* regular vectored traps */
#ifdef DEBUG
#ifdef TRAPTRACE
#define TRACEME		sethi %hi(1f), %g1; ba,pt %icc,traceit;\
 or %g1, %lo(1f), %g1; 1:
#if 0
#define TRACEWIN	sethi %hi(9f), %l6; ba,pt %icc,traceitwin;\
 or %l6, %lo(9f), %l6; 9:
#endif
#ifdef TRAPS_USE_IG
#define TRACEWIN	wrpr %g0, PSTATE_KERN|PSTATE_AG, %pstate;\
 sethi %hi(9f), %g1; ba,pt %icc,traceit; or %g1, %lo(9f), %g1; 9:
#else
#define TRACEWIN	wrpr %g0, PSTATE_KERN|PSTATE_IG, %pstate;\
 sethi %hi(9f), %g1; ba,pt %icc,traceit; or %g1, %lo(9f), %g1; 9:
#endif
#define TRACERELOAD32	ba reload32; nop;
#define TRACERELOAD64	ba reload64; nop;
#define TRACEFLT	TRACEME
#define	VTRAP(type, label) \
	sethi %hi(label), %g1; ba,pt %icc,traceit;\
 or %g1, %lo(label), %g1; NOTREACHED; TA8
#else
#define TRACEME
#define TRACEWIN	TRACEME
#define TRACERELOAD32
#define TRACERELOAD64
#ifdef FLTRACE
#define TRACEFLT	sethi %hi(1f), %g1; ba,pt %icc,traceit;\
 or %g1, %lo(1f), %g1; 1:
#else
#define TRACEFLT	TRACEME
#endif
#define	VTRAP(type, label) \
	sethi %hi(DATA_START),%g1; rdpr %tt,%g2; or %g1,0x28,%g1; b label;\
 stx %g2,[%g1]; NOTREACHED; TA8
#endif
#else
#ifdef TRAPTRACE
#define TRACEME		sethi %hi(1f), %g1; ba,pt %icc,traceit;\
 or %g1, %lo(1f), %g1; 1:
#if 0
/* Can't use this 'cause we have no clean registers during a spill */
#define TRACEWIN	sethi %hi(9f), %l6; ba,pt %icc,traceitwin;\
 or %l6, %lo(9f), %l6; 9:
#endif
#ifdef TRAPS_USE_IG
#define TRACEWIN	wrpr %g0, PSTATE_KERN|PSTATE_AG, %pstate;\
 sethi %hi(9f), %g1; ba,pt %icc,traceit; or %g1, %lo(9f), %g1; 9:
#else
#define TRACEWIN	wrpr %g0, PSTATE_KERN|PSTATE_IG, %pstate;\
 sethi %hi(9f), %g1; ba,pt %icc,traceit; or %g1, %lo(9f), %g1; 9:
#endif
#define TRACERELOAD32	ba reload32; nop;
#define TRACERELOAD64	ba reload64; nop;
#define TRACEFLT	TRACEME
#define	VTRAP(type, label) \
	sethi %hi(label), %g1; ba,pt %icc,traceit;\
 or %g1, %lo(label), %g1; NOTREACHED; TA8
#else
#define TRACEME
#define TRACEWIN	TRACEME
#define TRACERELOAD32
#define TRACERELOAD64
#ifdef FLTRACE
#define TRACEFLT	sethi %hi(1f), %g1; ba,pt %icc,traceit;\
 or %g1, %lo(1f), %g1; 1:
#else
#define TRACEFLT	TRACEME
#endif
#define	VTRAP(type, label) \
	ba,a,pt	%icc,label; nop; NOTREACHED; TA8
#endif
#endif
	/* hardware interrupts (can be linked or made `fast') */
#define	HARDINT4U(lev) \
	VTRAP(lev, _C_LABEL(sparc_interrupt))

	/* software interrupts (may not be made direct, sorry---but you
	   should not be using them trivially anyway) */
#define	SOFTINT4U(lev, bit) \
	HARDINT4U(lev)

	/* traps that just call trap() */
#define	TRAP(type)	VTRAP(type, slowtrap)

	/* architecturally undefined traps (cause panic) */
#ifndef DEBUG
#define	UTRAP(type)	sir; VTRAP(type, slowtrap)
#else
#define	UTRAP(type)	VTRAP(type, slowtrap)
#endif

	/* software undefined traps (may be replaced) */
#define	STRAP(type)	VTRAP(type, slowtrap)

/* breakpoint acts differently under kgdb */
#ifdef KGDB
#define	BPT		VTRAP(T_BREAKPOINT, bpt)
#define	BPT_KGDB_EXEC	VTRAP(T_KGDB_EXEC, bpt)
#else
#define	BPT		TRAP(T_BREAKPOINT)
#define	BPT_KGDB_EXEC	TRAP(T_KGDB_EXEC)
#endif

#define	SYSCALL		VTRAP(0x100, syscall_setup)
#ifdef notyet
#define	ZS_INTERRUPT	ba,a,pt %icc, zshard; nop; TA8
#else
#define	ZS_INTERRUPT4U	HARDINT4U(12)
#endif


/*
 * Macro to clear %tt so we don't get confused with old traps.
 */
#ifdef DEBUG
#define CLRTT	wrpr	%g0,0x1ff,%tt
#else
#define CLRTT
#endif
/*
 * Here are some oft repeated traps as macros.
 */

	/* spill a 64-bit register window */
#define SPILL64(label,as) \
	TRACEWIN; \
label:	\
	wr	%g0, as, %asi; \
	stxa	%l0, [%sp+BIAS+0x00]%asi; \
	stxa	%l1, [%sp+BIAS+0x08]%asi; \
	stxa	%l2, [%sp+BIAS+0x10]%asi; \
	stxa	%l3, [%sp+BIAS+0x18]%asi; \
	stxa	%l4, [%sp+BIAS+0x20]%asi; \
	stxa	%l5, [%sp+BIAS+0x28]%asi; \
	stxa	%l6, [%sp+BIAS+0x30]%asi; \
	\
	stxa	%l7, [%sp+BIAS+0x38]%asi; \
	stxa	%i0, [%sp+BIAS+0x40]%asi; \
	stxa	%i1, [%sp+BIAS+0x48]%asi; \
	stxa	%i2, [%sp+BIAS+0x50]%asi; \
	stxa	%i3, [%sp+BIAS+0x58]%asi; \
	stxa	%i4, [%sp+BIAS+0x60]%asi; \
	stxa	%i5, [%sp+BIAS+0x68]%asi; \
	stxa	%i6, [%sp+BIAS+0x70]%asi; \
	\
	stxa	%i7, [%sp+BIAS+0x78]%asi; \
	saved; \
	CLRTT; \
	retry; \
	NOTREACHED; \
	TA32

	/* spill a 32-bit register window */
#define SPILL32(label,as) \
	TRACEWIN; \
label:	\
	wr	%g0, as, %asi; \
	srl	%sp, 0, %sp; /* fixup 32-bit pointers */ \
	stwa	%l0, [%sp+0x00]%asi; \
	stwa	%l1, [%sp+0x04]%asi; \
	stwa	%l2, [%sp+0x08]%asi; \
	stwa	%l3, [%sp+0x0c]%asi; \
	stwa	%l4, [%sp+0x10]%asi; \
	stwa	%l5, [%sp+0x14]%asi; \
	\
	stwa	%l6, [%sp+0x18]%asi; \
	stwa	%l7, [%sp+0x1c]%asi; \
	stwa	%i0, [%sp+0x20]%asi; \
	stwa	%i1, [%sp+0x24]%asi; \
	stwa	%i2, [%sp+0x28]%asi; \
	stwa	%i3, [%sp+0x2c]%asi; \
	stwa	%i4, [%sp+0x30]%asi; \
	stwa	%i5, [%sp+0x34]%asi; \
	\
	stwa	%i6, [%sp+0x38]%asi; \
	stwa	%i7, [%sp+0x3c]%asi; \
	saved; \
	CLRTT; \
	retry; \
	NOTREACHED; \
	TA32

	/* Spill either 32-bit or 64-bit register window. */
#define SPILLBOTH(label64,label32,as) \
	TRACEWIN; \
	andcc	%sp, 1, %g0; \
	bnz,pt	%xcc, label64+4;	/* Is it a v9 or v8 stack? */ \
	 wr	%g0, as, %asi; \
	ba,pt	%xcc, label32+8; \
	 srl	%sp, 0, %sp; /* fixup 32-bit pointers */ \
	NOTREACHED; \
	TA32

	/* fill a 64-bit register window */
#define FILL64(label,as) \
	TRACEWIN; \
label: \
	wr	%g0, as, %asi; \
	ldxa	[%sp+BIAS+0x00]%asi, %l0; \
	ldxa	[%sp+BIAS+0x08]%asi, %l1; \
	ldxa	[%sp+BIAS+0x10]%asi, %l2; \
	ldxa	[%sp+BIAS+0x18]%asi, %l3; \
	ldxa	[%sp+BIAS+0x20]%asi, %l4; \
	ldxa	[%sp+BIAS+0x28]%asi, %l5; \
	ldxa	[%sp+BIAS+0x30]%asi, %l6; \
	\
	ldxa	[%sp+BIAS+0x38]%asi, %l7; \
	ldxa	[%sp+BIAS+0x40]%asi, %i0; \
	ldxa	[%sp+BIAS+0x48]%asi, %i1; \
	ldxa	[%sp+BIAS+0x50]%asi, %i2; \
	ldxa	[%sp+BIAS+0x58]%asi, %i3; \
	ldxa	[%sp+BIAS+0x60]%asi, %i4; \
	ldxa	[%sp+BIAS+0x68]%asi, %i5; \
	ldxa	[%sp+BIAS+0x70]%asi, %i6; \
	\
	ldxa	[%sp+BIAS+0x78]%asi, %i7; \
	restored; \
	CLRTT; \
	retry; \
	NOTREACHED; \
	TA32

	/* fill a 32-bit register window */
#define FILL32(label,as) \
	TRACEWIN; \
label:	\
	wr	%g0, as, %asi; \
	srl	%sp, 0, %sp; /* fixup 32-bit pointers */ \
	lda	[%sp+0x00]%asi, %l0; \
	lda	[%sp+0x04]%asi, %l1; \
	lda	[%sp+0x08]%asi, %l2; \
	lda	[%sp+0x0c]%asi, %l3; \
	lda	[%sp+0x10]%asi, %l4; \
	lda	[%sp+0x14]%asi, %l5; \
	\
	lda	[%sp+0x18]%asi, %l6; \
	lda	[%sp+0x1c]%asi, %l7; \
	lda	[%sp+0x20]%asi, %i0; \
	lda	[%sp+0x24]%asi, %i1; \
	lda	[%sp+0x28]%asi, %i2; \
	lda	[%sp+0x2c]%asi, %i3; \
	lda	[%sp+0x30]%asi, %i4; \
	lda	[%sp+0x34]%asi, %i5; \
	\
	lda	[%sp+0x38]%asi, %i6; \
	lda	[%sp+0x3c]%asi, %i7; \
	restored; \
	CLRTT; \
	retry; \
	NOTREACHED; \
	TA32

	/* fill either 32-bit or 64-bit register window. */
#define FILLBOTH(label64,label32,as) \
	TRACEWIN; \
	andcc	%sp, 1, %i0; \
	bnz	(label64)+4; /* See if it's a v9 stack or v8 */ \
	 wr	%g0, as, %asi; \
	ba	(label32)+8; \
	 srl	%sp, 0, %sp; /* fixup 32-bit pointers */ \
	NOTREACHED; \
	TA32

	.globl	start, _C_LABEL(kernel_text)
	_C_LABEL(kernel_text) = start		! for kvm_mkdb(8)
start:
	/* Traps from TL=0 -- traps from user mode */
#define TABLE	user_
	.globl	_C_LABEL(trapbase)
_C_LABEL(trapbase):
	b dostart; nop; TA8	! 000 = reserved -- Use it to boot
	/* We should not get the next 5 traps */
	UTRAP(0x001)		! 001 = POR Reset -- ROM should get this
	UTRAP(0x002)		! 002 = WDR -- ROM should get this
	UTRAP(0x003)		! 003 = XIR -- ROM should get this
	UTRAP(0x004)		! 004 = SIR -- ROM should get this
	UTRAP(0x005)		! 005 = RED state exception
	UTRAP(0x006); UTRAP(0x007)
	VTRAP(T_INST_EXCEPT, textfault)	! 008 = instr. access exept
	VTRAP(T_TEXTFAULT, textfault)	! 009 = instr access MMU miss
	VTRAP(T_INST_ERROR, textfault)	! 00a = instr. access err
	UTRAP(0x00b); UTRAP(0x00c); UTRAP(0x00d); UTRAP(0x00e); UTRAP(0x00f)
	TRAP(T_ILLINST)			! 010 = illegal instruction
	TRAP(T_PRIVINST)		! 011 = privileged instruction
	UTRAP(0x012)			! 012 = unimplemented LDD
	UTRAP(0x013)			! 013 = unimplemented STD
	UTRAP(0x014); UTRAP(0x015); UTRAP(0x016); UTRAP(0x017); UTRAP(0x018)
	UTRAP(0x019); UTRAP(0x01a); UTRAP(0x01b); UTRAP(0x01c); UTRAP(0x01d)
	UTRAP(0x01e); UTRAP(0x01f)
	TRAP(T_FPDISABLED)		! 020 = fp instr, but EF bit off in psr
	VTRAP(T_FP_IEEE_754, fp_exception)		! 021 = ieee 754 exception
	VTRAP(T_FP_OTHER, fp_exception)		! 022 = other fp exception
	TRAP(T_TAGOF)			! 023 = tag overflow
	TRACEWIN			! DEBUG -- 4 insns
	rdpr %cleanwin, %o7		! 024-027 = clean window trap
	inc %o7				!	This handler is in-lined and cannot fault
	wrpr %g0, %o7, %cleanwin	!       Nucleus (trap&IRQ) code does not need clean windows

	clr	%l0
!	set	0xbadcafe, %l0		! DEBUG
	mov %l0,%l1; mov %l0,%l2	!	Clear out %l0-%l8 and %o0-%o8 and inc %cleanwin and done
	mov %l0,%l3; mov %l0,%l4
#if 0
#ifdef DIAGNOSTIC
	!!
	!! Check the sp redzone
	!!
	!! Since we can't spill the current window, we'll just keep
	!! track of the frame pointer.  Problems occur when the routine
	!! allocates and uses stack storage.
	!!
!	rdpr	%wstate, %l5	! User stack?
!	cmp	%l5, WSTATE_KERN
!	bne,pt	%icc, 7f
	 sethi	%hi(CPCB), %l5
	LDPTR	[%l5 + %lo(CPCB)], %l5	! If pcb < fp < pcb+sizeof(pcb)
	inc	PCB_SIZE, %l5		! then we have a stack overflow
	btst	%fp, 1			! 64-bit stack?
	sub	%fp, %l5, %l7
	bnz,a,pt	%icc, 1f
	 inc	BIAS, %l7		! Remove BIAS
1:
	cmp	%l7, PCB_SIZE
	blu	%xcc, cleanwin_overflow
#endif
#endif
	mov %l0, %l5
	mov %l0, %l6; mov %l0, %l7; mov %l0, %o0; mov %l0, %o1

	mov %l0, %o2; mov %l0, %o3; mov %l0, %o4; mov %l0, %o5;
	mov %l0, %o6; mov %l0, %o7
	CLRTT
	retry; nop; NOTREACHED; TA32
	TRAP(T_DIV0)			! 028 = divide by zero
	UTRAP(0x029)			! 029 = internal processor error
	UTRAP(0x02a); UTRAP(0x02b); UTRAP(0x02c); UTRAP(0x02d); UTRAP(0x02e); UTRAP(0x02f)
	VTRAP(T_DATAFAULT, winfault)	! 030 = data fetch fault
	UTRAP(0x031)			! 031 = data MMU miss -- no MMU
	VTRAP(T_DATA_ERROR, winfault)	! 032 = data access error
	VTRAP(T_DATA_PROT, winfault)	! 033 = data protection fault
	TRAP(T_ALIGN)			! 034 = address alignment error -- we could fix it inline...
	TRAP(T_LDDF_ALIGN)		! 035 = LDDF address alignment error -- we could fix it inline...
	TRAP(T_STDF_ALIGN)		! 036 = STDF address alignment error -- we could fix it inline...
	TRAP(T_PRIVACT)			! 037 = privileged action
	UTRAP(0x038); UTRAP(0x039); UTRAP(0x03a); UTRAP(0x03b); UTRAP(0x03c);
	UTRAP(0x03d); UTRAP(0x03e); UTRAP(0x03f);
	VTRAP(T_ASYNC_ERROR, winfault)	! 040 = data fetch fault
	SOFTINT4U(1, IE_L1)		! 041 = level 1 interrupt
	HARDINT4U(2)			! 042 = level 2 interrupt
	HARDINT4U(3)			! 043 = level 3 interrupt
	SOFTINT4U(4, IE_L4)		! 044 = level 4 interrupt
	HARDINT4U(5)			! 045 = level 5 interrupt
	SOFTINT4U(6, IE_L6)		! 046 = level 6 interrupt
	HARDINT4U(7)			! 047 = level 7 interrupt
	HARDINT4U(8)			! 048 = level 8 interrupt
	HARDINT4U(9)			! 049 = level 9 interrupt
	HARDINT4U(10)			! 04a = level 10 interrupt
	HARDINT4U(11)			! 04b = level 11 interrupt
	ZS_INTERRUPT4U			! 04c = level 12 (zs) interrupt
	HARDINT4U(13)			! 04d = level 13 interrupt
	HARDINT4U(14)			! 04e = level 14 interrupt
	VTRAP(15, winfault)		! 04f = nonmaskable interrupt
	UTRAP(0x050); UTRAP(0x051); UTRAP(0x052); UTRAP(0x053); UTRAP(0x054); UTRAP(0x055)
	UTRAP(0x056); UTRAP(0x057); UTRAP(0x058); UTRAP(0x059); UTRAP(0x05a); UTRAP(0x05b)
	UTRAP(0x05c); UTRAP(0x05d); UTRAP(0x05e); UTRAP(0x05f)
	VTRAP(0x060, interrupt_vector); ! 060 = interrupt vector
	TRAP(T_PA_WATCHPT)		! 061 = physical address data watchpoint
	TRAP(T_VA_WATCHPT)		! 062 = virtual address data watchpoint
	UTRAP(T_ECCERR)			! We'll implement this one later
ufast_IMMU_miss:			! 064 = fast instr access MMU miss
	TRACEFLT			! DEBUG
	ldxa	[%g0] ASI_IMMU_8KPTR, %g2	!				Load IMMU 8K TSB pointer
	ldxa	[%g0] ASI_IMMU, %g1	! Hard coded for unified 8K TSB		Load IMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	!				Load TSB tag and data into %g4 and %g5
#ifdef MMUDEBUG
	rdpr	%tstate, %g7				! DEBUG record if we're on MMU globals
	srlx	%g7, TSTATE_PSTATE_SHIFT, %g7		! DEBUG
	btst	PSTATE_MG, %g7				! DEBUG
	bz	0f					! DEBUG
	 sethi	%hi(_C_LABEL(missmmu)), %g7		! DEBUG
	lduw	[%g7+%lo(_C_LABEL(missmmu))], %g6	! DEBUG
	inc	%g6					! DEBUG
	stw	%g6, [%g7+%lo(_C_LABEL(missmmu))]	! DEBUG
0:							! DEBUG
#endif
#ifdef NO_TSB
	ba,a	%icc, instr_miss;
	 nop
#endif
	brgez,pn %g5, instr_miss	!					Entry invalid?  Punt
	 xor	%g1, %g4, %g4		!					Compare TLB tags
	brnz,pn %g4, instr_miss		!					Got right tag?
	 nop
	CLRTT
	stxa	%g5, [%g0] ASI_IMMU_DATA_IN!					Enter new mapping
	retry				!					Try new mapping
1:
	sir
	TA32
ufast_DMMU_miss:			! 068 = fast data access MMU miss
	TRACEFLT			! DEBUG
	ldxa	[%g0] ASI_DMMU_8KPTR, %g2!					Load DMMU 8K TSB pointer
	ldxa	[%g0] ASI_DMMU, %g1	! Hard coded for unified 8K TSB		Load DMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	!				Load TSB tag and data into %g4 and %g5
#ifdef MMUDEBUG
	rdpr	%tstate, %g7				! DEBUG record if we're on MMU globals
	srlx	%g7, TSTATE_PSTATE_SHIFT, %g7		! DEBUG
	btst	PSTATE_MG, %g7				! DEBUG
	bz	0f					! DEBUG
	 sethi	%hi(_C_LABEL(missmmu)), %g7		! DEBUG
	lduw	[%g7+%lo(_C_LABEL(missmmu))], %g6	! DEBUG
	inc	%g6					! DEBUG
	stw	%g6, [%g7+%lo(_C_LABEL(missmmu))]	! DEBUG
0:							! DEBUG
#endif
#ifdef NO_TSB
	ba,a	%icc, data_miss;
	 nop
#endif
	brgez,pn %g5, data_miss		!					Entry invalid?  Punt
	 xor	%g1, %g4, %g4		!					Compare TLB tags
	brnz,pn	%g4, data_miss		!					Got right tag?
	 nop
	CLRTT
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(udhit)), %g1
	lduw	[%g1+%lo(_C_LABEL(udhit))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(udhit))]
#endif
	stxa	%g5, [%g0] ASI_DMMU_DATA_IN!					Enter new mapping
	retry				!					Try new mapping
1:
	sir
	TA32
ufast_DMMU_protection:			! 06c = fast data access MMU protection
	TRACEFLT			! DEBUG -- we're perilously close to 32 insns
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(udprot)), %g1
	lduw	[%g1+%lo(_C_LABEL(udprot))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(udprot))]
#endif
#ifdef MMUDEBUG
	rdpr	%tstate, %g7				! DEBUG record if we're on MMU globals
	srlx	%g7, TSTATE_PSTATE_SHIFT, %g7		! DEBUG
	btst	PSTATE_MG, %g7				! DEBUG
	bz	0f					! DEBUG
	 sethi	%hi(_C_LABEL(protmmu)), %g7		! DEBUG
	lduw	[%g7+%lo(_C_LABEL(protmmu))], %g6	! DEBUG
	inc	%g6					! DEBUG
	stw	%g6, [%g7+%lo(_C_LABEL(protmmu))]	! DEBUG
0:							! DEBUG
#endif
#ifdef HWREF
	ba,a,pt	%xcc, dmmu_write_fault
#else
	ba,a,pt	%xcc, winfault
#endif
	nop
	TA32
	UTRAP(0x070)			! Implementation dependent traps
	UTRAP(0x071); UTRAP(0x072); UTRAP(0x073); UTRAP(0x074); UTRAP(0x075); UTRAP(0x076)
	UTRAP(0x077); UTRAP(0x078); UTRAP(0x079); UTRAP(0x07a); UTRAP(0x07b); UTRAP(0x07c)
	UTRAP(0x07d); UTRAP(0x07e); UTRAP(0x07f)
TABLE/**/uspill:
	SPILL64(uspill8,ASI_AIUS)	! 0x080 spill_0_normal -- used to save user windows in user mode
	SPILL32(uspill4,ASI_AIUS)	! 0x084 spill_1_normal
	SPILLBOTH(uspill8,uspill4,ASI_AIUS)		! 0x088 spill_2_normal
#ifdef DEBUG
	sir
#endif
	UTRAP(0x08c); TA32	! 0x08c spill_3_normal
TABLE/**/kspill:
	SPILL64(kspill8,ASI_N)	! 0x090 spill_4_normal -- used to save supervisor windows
	SPILL32(kspill4,ASI_N)	! 0x094 spill_5_normal
	SPILLBOTH(kspill8,kspill4,ASI_N)	! 0x098 spill_6_normal
	UTRAP(0x09c); TA32	! 0x09c spill_7_normal
TABLE/**/uspillk:
	SPILL64(uspillk8,ASI_AIUS)	! 0x0a0 spill_0_other -- used to save user windows in supervisor mode
	SPILL32(uspillk4,ASI_AIUS)	! 0x0a4 spill_1_other
	SPILLBOTH(uspillk8,uspillk4,ASI_AIUS)	! 0x0a8 spill_2_other
	UTRAP(0x0ac); TA32	! 0x0ac spill_3_other
	UTRAP(0x0b0); TA32	! 0x0b0 spill_4_other
	UTRAP(0x0b4); TA32	! 0x0b4 spill_5_other
	UTRAP(0x0b8); TA32	! 0x0b8 spill_6_other
	UTRAP(0x0bc); TA32	! 0x0bc spill_7_other
TABLE/**/ufill:
	FILL64(ufill8,ASI_AIUS) ! 0x0c0 fill_0_normal -- used to fill windows when running user mode
	FILL32(ufill4,ASI_AIUS)	! 0x0c4 fill_1_normal
	FILLBOTH(ufill8,ufill4,ASI_AIUS)	! 0x0c8 fill_2_normal
	UTRAP(0x0cc); TA32	! 0x0cc fill_3_normal
TABLE/**/kfill:
	FILL64(kfill8,ASI_N)	! 0x0d0 fill_4_normal -- used to fill windows when running supervisor mode
	FILL32(kfill4,ASI_N)	! 0x0d4 fill_5_normal
	FILLBOTH(kfill8,kfill4,ASI_N)	! 0x0d8 fill_6_normal
	UTRAP(0x0dc); TA32	! 0x0dc fill_7_normal
TABLE/**/ufillk:
	FILL64(ufillk8,ASI_AIUS)	! 0x0e0 fill_0_other
	FILL32(ufillk4,ASI_AIUS)	! 0x0e4 fill_1_other
	FILLBOTH(ufillk8,ufillk4,ASI_AIUS)	! 0x0e8 fill_2_other
	UTRAP(0x0ec); TA32	! 0x0ec fill_3_other
	UTRAP(0x0f0); TA32	! 0x0f0 fill_4_other
	UTRAP(0x0f4); TA32	! 0x0f4 fill_5_other
	UTRAP(0x0f8); TA32	! 0x0f8 fill_6_other
	UTRAP(0x0fc); TA32	! 0x0fc fill_7_other
TABLE/**/syscall:
	SYSCALL			! 0x100 = sun syscall
	BPT			! 0x101 = pseudo breakpoint instruction
	STRAP(0x102); STRAP(0x103); STRAP(0x104); STRAP(0x105); STRAP(0x106); STRAP(0x107)
	SYSCALL			! 0x108 = svr4 syscall
	SYSCALL			! 0x109 = bsd syscall
	BPT_KGDB_EXEC		! 0x10a = enter kernel gdb on kernel startup
	STRAP(0x10b); STRAP(0x10c); STRAP(0x10d); STRAP(0x10e); STRAP(0x10f);
	STRAP(0x110); STRAP(0x111); STRAP(0x112); STRAP(0x113); STRAP(0x114); STRAP(0x115); STRAP(0x116); STRAP(0x117)
	STRAP(0x118); STRAP(0x119); STRAP(0x11a); STRAP(0x11b); STRAP(0x11c); STRAP(0x11d); STRAP(0x11e); STRAP(0x11f)
	STRAP(0x120); STRAP(0x121); STRAP(0x122); STRAP(0x123); STRAP(0x124); STRAP(0x125); STRAP(0x126); STRAP(0x127)
	STRAP(0x128); STRAP(0x129); STRAP(0x12a); STRAP(0x12b); STRAP(0x12c); STRAP(0x12d); STRAP(0x12e); STRAP(0x12f)
	STRAP(0x130); STRAP(0x131); STRAP(0x132); STRAP(0x133); STRAP(0x134); STRAP(0x135); STRAP(0x136); STRAP(0x137)
	STRAP(0x138); STRAP(0x139); STRAP(0x13a); STRAP(0x13b); STRAP(0x13c); STRAP(0x13d); STRAP(0x13e); STRAP(0x13f)
	STRAP(0x140); STRAP(0x141); STRAP(0x142); STRAP(0x143); STRAP(0x144); STRAP(0x145); STRAP(0x146); STRAP(0x147)
	STRAP(0x148); STRAP(0x149); STRAP(0x14a); STRAP(0x14b); STRAP(0x14c); STRAP(0x14d); STRAP(0x14e); STRAP(0x14f)
	STRAP(0x150); STRAP(0x151); STRAP(0x152); STRAP(0x153); STRAP(0x154); STRAP(0x155); STRAP(0x156); STRAP(0x157)
	STRAP(0x158); STRAP(0x159); STRAP(0x15a); STRAP(0x15b); STRAP(0x15c); STRAP(0x15d); STRAP(0x15e); STRAP(0x15f)
	STRAP(0x160); STRAP(0x161); STRAP(0x162); STRAP(0x163);
	SYSCALL			! 0x164 SVID syscall (Solaris 2.7)
	SYSCALL			! 0x165 SPARC International syscall
	SYSCALL			! 0x166	OS Vendor syscall
	SYSCALL			! 0x167 HW OEM syscall
	STRAP(0x168); STRAP(0x169); STRAP(0x16a); STRAP(0x16b); STRAP(0x16c); STRAP(0x16d); STRAP(0x16e); STRAP(0x16f)
	STRAP(0x170); STRAP(0x171); STRAP(0x172); STRAP(0x173); STRAP(0x174); STRAP(0x175); STRAP(0x176); STRAP(0x177)
	STRAP(0x178); STRAP(0x179); STRAP(0x17a); STRAP(0x17b); STRAP(0x17c); STRAP(0x17d); STRAP(0x17e); STRAP(0x17f)
	! Traps beyond 0x17f are reserved
	UTRAP(0x180); UTRAP(0x181); UTRAP(0x182); UTRAP(0x183); UTRAP(0x184); UTRAP(0x185); UTRAP(0x186); UTRAP(0x187)
	UTRAP(0x188); UTRAP(0x189); UTRAP(0x18a); UTRAP(0x18b); UTRAP(0x18c); UTRAP(0x18d); UTRAP(0x18e); UTRAP(0x18f)
	UTRAP(0x190); UTRAP(0x191); UTRAP(0x192); UTRAP(0x193); UTRAP(0x194); UTRAP(0x195); UTRAP(0x196); UTRAP(0x197)
	UTRAP(0x198); UTRAP(0x199); UTRAP(0x19a); UTRAP(0x19b); UTRAP(0x19c); UTRAP(0x19d); UTRAP(0x19e); UTRAP(0x19f)
	UTRAP(0x1a0); UTRAP(0x1a1); UTRAP(0x1a2); UTRAP(0x1a3); UTRAP(0x1a4); UTRAP(0x1a5); UTRAP(0x1a6); UTRAP(0x1a7)
	UTRAP(0x1a8); UTRAP(0x1a9); UTRAP(0x1aa); UTRAP(0x1ab); UTRAP(0x1ac); UTRAP(0x1ad); UTRAP(0x1ae); UTRAP(0x1af)
	UTRAP(0x1b0); UTRAP(0x1b1); UTRAP(0x1b2); UTRAP(0x1b3); UTRAP(0x1b4); UTRAP(0x1b5); UTRAP(0x1b6); UTRAP(0x1b7)
	UTRAP(0x1b8); UTRAP(0x1b9); UTRAP(0x1ba); UTRAP(0x1bb); UTRAP(0x1bc); UTRAP(0x1bd); UTRAP(0x1be); UTRAP(0x1bf)
	UTRAP(0x1c0); UTRAP(0x1c1); UTRAP(0x1c2); UTRAP(0x1c3); UTRAP(0x1c4); UTRAP(0x1c5); UTRAP(0x1c6); UTRAP(0x1c7)
	UTRAP(0x1c8); UTRAP(0x1c9); UTRAP(0x1ca); UTRAP(0x1cb); UTRAP(0x1cc); UTRAP(0x1cd); UTRAP(0x1ce); UTRAP(0x1cf)
	UTRAP(0x1d0); UTRAP(0x1d1); UTRAP(0x1d2); UTRAP(0x1d3); UTRAP(0x1d4); UTRAP(0x1d5); UTRAP(0x1d6); UTRAP(0x1d7)
	UTRAP(0x1d8); UTRAP(0x1d9); UTRAP(0x1da); UTRAP(0x1db); UTRAP(0x1dc); UTRAP(0x1dd); UTRAP(0x1de); UTRAP(0x1df)
	UTRAP(0x1e0); UTRAP(0x1e1); UTRAP(0x1e2); UTRAP(0x1e3); UTRAP(0x1e4); UTRAP(0x1e5); UTRAP(0x1e6); UTRAP(0x1e7)
	UTRAP(0x1e8); UTRAP(0x1e9); UTRAP(0x1ea); UTRAP(0x1eb); UTRAP(0x1ec); UTRAP(0x1ed); UTRAP(0x1ee); UTRAP(0x1ef)
	UTRAP(0x1f0); UTRAP(0x1f1); UTRAP(0x1f2); UTRAP(0x1f3); UTRAP(0x1f4); UTRAP(0x1f5); UTRAP(0x1f6); UTRAP(0x1f7)
	UTRAP(0x1f8); UTRAP(0x1f9); UTRAP(0x1fa); UTRAP(0x1fb); UTRAP(0x1fc); UTRAP(0x1fd); UTRAP(0x1fe); UTRAP(0x1ff)

	/* Traps from TL>0 -- traps from supervisor mode */
#undef TABLE
#define TABLE	nucleus_
trapbase_priv:
	UTRAP(0x000)		! 000 = reserved -- Use it to boot
	/* We should not get the next 5 traps */
	UTRAP(0x001)		! 001 = POR Reset -- ROM should get this
	UTRAP(0x002)		! 002 = WDR Watchdog -- ROM should get this
	UTRAP(0x003)		! 003 = XIR -- ROM should get this
	UTRAP(0x004)		! 004 = SIR -- ROM should get this
	UTRAP(0x005)		! 005 = RED state exception
	UTRAP(0x006); UTRAP(0x007)
ktextfault:
	VTRAP(T_INST_EXCEPT, textfault)	! 008 = instr. access exept
	VTRAP(T_TEXTFAULT, textfault)	! 009 = instr access MMU miss -- no MMU
	VTRAP(T_INST_ERROR, textfault)	! 00a = instr. access err
	UTRAP(0x00b); UTRAP(0x00c); UTRAP(0x00d); UTRAP(0x00e); UTRAP(0x00f)
	TRAP(T_ILLINST)			! 010 = illegal instruction
	TRAP(T_PRIVINST)		! 011 = privileged instruction
	UTRAP(0x012)			! 012 = unimplemented LDD
	UTRAP(0x013)			! 013 = unimplemented STD
	UTRAP(0x014); UTRAP(0x015); UTRAP(0x016); UTRAP(0x017); UTRAP(0x018)
	UTRAP(0x019); UTRAP(0x01a); UTRAP(0x01b); UTRAP(0x01c); UTRAP(0x01d)
	UTRAP(0x01e); UTRAP(0x01f)
	TRAP(T_FPDISABLED)		! 020 = fp instr, but EF bit off in psr
	VTRAP(T_FP_IEEE_754, fp_exception)		! 021 = ieee 754 exception
	VTRAP(T_FP_OTHER, fp_exception)		! 022 = other fp exception
	TRAP(T_TAGOF)			! 023 = tag overflow
	TRACEWIN			! DEBUG
	clr	%l0
!	set	0xbadbeef, %l0		! DEBUG
	mov %l0, %l1; mov %l0, %l2	! 024-027 = clean window trap
	rdpr %cleanwin, %o7		!	This handler is in-lined and cannot fault
	inc %o7; mov %l0, %l3; mov %l0, %l4	!       Nucleus (trap&IRQ) code does not need clean windows
	wrpr %g0, %o7, %cleanwin	!	Clear out %l0-%l8 and %o0-%o8 and inc %cleanwin and done
#ifdef NOT_DEBUG
	!!
	!! Check the sp redzone
	!!
	rdpr	%wstate, t1
	cmp	t1, WSTATE_KERN
	bne,pt	icc, 7f
	 sethi	%hi(_C_LABEL(redzone)), t1
	ldx	[t1 + %lo(_C_LABEL(redzone))], t2
	cmp	%sp, t2			! if sp >= t2, not in red zone
	blu	panic_red		! and can continue normally
7:
#endif
	mov %l0, %l5; mov %l0, %l6; mov %l0, %l7
	mov %l0, %o0; mov %l0, %o1; mov %l0, %o2; mov %l0, %o3

	mov %l0, %o4; mov %l0, %o5; mov %l0, %o6; mov %l0, %o7
	CLRTT
	retry; nop; TA32
	TRAP(T_DIV0)			! 028 = divide by zero
	UTRAP(0x029)			! 029 = internal processor error
	UTRAP(0x02a); UTRAP(0x02b); UTRAP(0x02c); UTRAP(0x02d); UTRAP(0x02e); UTRAP(0x02f)
kdatafault:
	VTRAP(T_DATAFAULT, winfault)	! 030 = data fetch fault
	UTRAP(0x031)			! 031 = data MMU miss -- no MMU
	VTRAP(T_DATA_ERROR, winfault)	! 032 = data fetch fault
	VTRAP(T_DATA_PROT, winfault)	! 033 = data fetch fault
	TRAP(T_ALIGN)			! 034 = address alignment error -- we could fix it inline...
!	sir; nop; TA8	! DEBUG -- trap all kernel alignment errors
	TRAP(T_LDDF_ALIGN)		! 035 = LDDF address alignment error -- we could fix it inline...
	TRAP(T_STDF_ALIGN)		! 036 = STDF address alignment error -- we could fix it inline...
	TRAP(T_PRIVACT)			! 037 = privileged action
	UTRAP(0x038); UTRAP(0x039); UTRAP(0x03a); UTRAP(0x03b); UTRAP(0x03c);
	UTRAP(0x03d); UTRAP(0x03e); UTRAP(0x03f);
	VTRAP(T_ASYNC_ERROR, winfault)	! 040 = data fetch fault
	SOFTINT4U(1, IE_L1)		! 041 = level 1 interrupt
	HARDINT4U(2)			! 042 = level 2 interrupt
	HARDINT4U(3)			! 043 = level 3 interrupt
	SOFTINT4U(4, IE_L4)		! 044 = level 4 interrupt
	HARDINT4U(5)			! 045 = level 5 interrupt
	SOFTINT4U(6, IE_L6)		! 046 = level 6 interrupt
	HARDINT4U(7)			! 047 = level 7 interrupt
	HARDINT4U(8)			! 048 = level 8 interrupt
	HARDINT4U(9)			! 049 = level 9 interrupt
	HARDINT4U(10)			! 04a = level 10 interrupt
	HARDINT4U(11)			! 04b = level 11 interrupt
	ZS_INTERRUPT4U			! 04c = level 12 (zs) interrupt
	HARDINT4U(13)			! 04d = level 13 interrupt
	HARDINT4U(14)			! 04e = level 14 interrupt
	VTRAP(15, winfault)		! 04f = nonmaskable interrupt
	UTRAP(0x050); UTRAP(0x051); UTRAP(0x052); UTRAP(0x053); UTRAP(0x054); UTRAP(0x055)
	UTRAP(0x056); UTRAP(0x057); UTRAP(0x058); UTRAP(0x059); UTRAP(0x05a); UTRAP(0x05b)
	UTRAP(0x05c); UTRAP(0x05d); UTRAP(0x05e); UTRAP(0x05f)
	VTRAP(0x060, interrupt_vector); ! 060 = interrupt vector
	TRAP(T_PA_WATCHPT)		! 061 = physical address data watchpoint
	TRAP(T_VA_WATCHPT)		! 062 = virtual address data watchpoint
	UTRAP(T_ECCERR)			! We'll implement this one later
kfast_IMMU_miss:			! 064 = fast instr access MMU miss
	TRACEFLT			! DEBUG
	ldxa	[%g0] ASI_IMMU_8KPTR, %g2	!				Load IMMU 8K TSB pointer
	ldxa	[%g0] ASI_IMMU, %g1	! Hard coded for unified 8K TSB		Load IMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	!				Load TSB tag and data into %g4 and %g5
#ifdef MMUDEBUG
	rdpr	%tstate, %g7				! DEBUG record if we're on MMU globals
	srlx	%g7, TSTATE_PSTATE_SHIFT, %g7		! DEBUG
	btst	PSTATE_MG, %g7				! DEBUG
	bz	0f					! DEBUG
	 sethi	%hi(_C_LABEL(missmmu)), %g7		! DEBUG
	lduw	[%g7+%lo(_C_LABEL(missmmu))], %g6	! DEBUG
	inc	%g6					! DEBUG
	stw	%g6, [%g7+%lo(_C_LABEL(missmmu))]	! DEBUG
0:							! DEBUG
#endif
#ifdef NO_TSB
	ba,a	%icc, instr_miss;
	 nop
#endif
	brgez,pn %g5, instr_miss	!					Entry invalid?  Punt
	 xor	%g1, %g4, %g4		!					Compare TLB tags
	brnz,pn %g4, instr_miss		!					Got right tag?
	 nop
	CLRTT
	stxa	%g5, [%g0] ASI_IMMU_DATA_IN!					Enter new mapping
	retry				!					Try new mapping
1:
	sir
	TA32
kfast_DMMU_miss:			! 068 = fast data access MMU miss
	TRACEFLT			! DEBUG
	ldxa	[%g0] ASI_DMMU_8KPTR, %g2!					Load DMMU 8K TSB pointer
	ldxa	[%g0] ASI_DMMU, %g1	! Hard coded for unified 8K TSB		Load DMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	!				Load TSB tag and data into %g4 and %g5
#ifdef MMUDEBUG
	rdpr	%tstate, %g7				! DEBUG record if we're on MMU globals
	srlx	%g7, TSTATE_PSTATE_SHIFT, %g7		! DEBUG
	btst	PSTATE_MG, %g7				! DEBUG
	bz	0f					! DEBUG
	 sethi	%hi(_C_LABEL(missmmu)), %g7		! DEBUG
	lduw	[%g7+%lo(_C_LABEL(missmmu))], %g6	! DEBUG
	inc	%g6					! DEBUG
	stw	%g6, [%g7+%lo(_C_LABEL(missmmu))]	! DEBUG
0:							! DEBUG
#endif
#ifdef NO_TSB
	ba,a	%icc, data_miss;
	 nop
#endif
	brgez,pn %g5, data_miss		!					Entry invalid?  Punt
	 xor	%g1, %g4, %g4		!					Compare TLB tags
	brnz,pn	%g4, data_miss		!					Got right tag?
	 nop
	CLRTT
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(kdhit)), %g1
	lduw	[%g1+%lo(_C_LABEL(kdhit))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(kdhit))]
#endif
	stxa	%g5, [%g0] ASI_DMMU_DATA_IN!					Enter new mapping
	retry				!					Try new mapping
1:
	sir
	TA32
kfast_DMMU_protection:			! 06c = fast data access MMU protection
	TRACEFLT			! DEBUG
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(kdprot)), %g1
	lduw	[%g1+%lo(_C_LABEL(kdprot))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(kdprot))]
#endif
#ifdef MMUDEBUG
	rdpr	%tstate, %g7				! DEBUG record if we're on MMU globals
	srlx	%g7, TSTATE_PSTATE_SHIFT, %g7		! DEBUG
	btst	PSTATE_MG, %g7				! DEBUG
	bz	0f					! DEBUG
	 sethi	%hi(_C_LABEL(protmmu)), %g7		! DEBUG
	lduw	[%g7+%lo(_C_LABEL(protmmu))], %g6	! DEBUG
	inc	%g6					! DEBUG
	stw	%g6, [%g7+%lo(_C_LABEL(protmmu))]	! DEBUG
0:							! DEBUG
#endif
#ifdef HWREF
	ba,a,pt	%xcc, dmmu_write_fault
#else
	ba,a,pt	%xcc, winfault
#endif
	nop
	TA32
	UTRAP(0x070)			! Implementation dependent traps
	UTRAP(0x071); UTRAP(0x072); UTRAP(0x073); UTRAP(0x074); UTRAP(0x075); UTRAP(0x076)
	UTRAP(0x077); UTRAP(0x078); UTRAP(0x079); UTRAP(0x07a); UTRAP(0x07b); UTRAP(0x07c)
	UTRAP(0x07d); UTRAP(0x07e); UTRAP(0x07f)
TABLE/**/uspill:
	SPILL64(1,ASI_AIUS)	! 0x080 spill_0_normal -- used to save user windows
	SPILL32(2,ASI_AIUS)	! 0x084 spill_1_normal
	SPILLBOTH(1b,2b,ASI_AIUS)	! 0x088 spill_2_normal
	UTRAP(0x08c); TA32	! 0x08c spill_3_normal
TABLE/**/kspill:
	SPILL64(1,ASI_N)	! 0x090 spill_4_normal -- used to save supervisor windows
	SPILL32(2,ASI_N)	! 0x094 spill_5_normal
	SPILLBOTH(1b,2b,ASI_N)	! 0x098 spill_6_normal
	UTRAP(0x09c); TA32	! 0x09c spill_7_normal
TABLE/**/uspillk:
	SPILL64(1,ASI_AIUS)	! 0x0a0 spill_0_other -- used to save user windows in nucleus mode
	SPILL32(2,ASI_AIUS)	! 0x0a4 spill_1_other
	SPILLBOTH(1b,2b,ASI_AIUS)	! 0x0a8 spill_2_other
	UTRAP(0x0ac); TA32	! 0x0ac spill_3_other
	UTRAP(0x0b0); TA32	! 0x0b0 spill_4_other
	UTRAP(0x0b4); TA32	! 0x0b4 spill_5_other
	UTRAP(0x0b8); TA32	! 0x0b8 spill_6_other
	UTRAP(0x0bc); TA32	! 0x0bc spill_7_other
TABLE/**/ufill:
	FILL64(1,ASI_AIUS)	! 0x0c0 fill_0_normal -- used to fill windows when running nucleus mode from user
	FILL32(2,ASI_AIUS)	! 0x0c4 fill_1_normal
	FILLBOTH(1b,2b,ASI_AIUS)	! 0x0c8 fill_2_normal
	UTRAP(0x0cc); TA32	! 0x0cc fill_3_normal
TABLE/**/sfill:
	FILL64(1,ASI_N)		! 0x0d0 fill_4_normal -- used to fill windows when running nucleus mode from supervisor
	FILL32(2,ASI_N)		! 0x0d4 fill_5_normal
	FILLBOTH(1b,2b,ASI_N)	! 0x0d8 fill_6_normal
	UTRAP(0x0dc); TA32	! 0x0dc fill_7_normal
TABLE/**/kfill:
	FILL64(1,ASI_AIUS)	! 0x0e0 fill_0_other -- used to fill user windows when running nucleus mode -- will we ever use this?
	FILL32(2,ASI_AIUS)	! 0x0e4 fill_1_other
	FILLBOTH(1b,2b,ASI_AIUS)! 0x0e8 fill_2_other
	UTRAP(0x0ec); TA32	! 0x0ec fill_3_other
	UTRAP(0x0f0); TA32	! 0x0f0 fill_4_other
	UTRAP(0x0f4); TA32	! 0x0f4 fill_5_other
	UTRAP(0x0f8); TA32	! 0x0f8 fill_6_other
	UTRAP(0x0fc); TA32	! 0x0fc fill_7_other
TABLE/**/syscall:
	SYSCALL			! 0x100 = sun syscall
	BPT			! 0x101 = pseudo breakpoint instruction
	STRAP(0x102); STRAP(0x103); STRAP(0x104); STRAP(0x105); STRAP(0x106); STRAP(0x107)
	SYSCALL			! 0x108 = svr4 syscall
	SYSCALL			! 0x109 = bsd syscall
	BPT_KGDB_EXEC		! 0x10a = enter kernel gdb on kernel startup
	STRAP(0x10b); STRAP(0x10c); STRAP(0x10d); STRAP(0x10e); STRAP(0x10f);
	STRAP(0x110); STRAP(0x111); STRAP(0x112); STRAP(0x113); STRAP(0x114); STRAP(0x115); STRAP(0x116); STRAP(0x117)
	STRAP(0x118); STRAP(0x119); STRAP(0x11a); STRAP(0x11b); STRAP(0x11c); STRAP(0x11d); STRAP(0x11e); STRAP(0x11f)
	STRAP(0x120); STRAP(0x121); STRAP(0x122); STRAP(0x123); STRAP(0x124); STRAP(0x125); STRAP(0x126); STRAP(0x127)
	STRAP(0x128); STRAP(0x129); STRAP(0x12a); STRAP(0x12b); STRAP(0x12c); STRAP(0x12d); STRAP(0x12e); STRAP(0x12f)
	STRAP(0x130); STRAP(0x131); STRAP(0x132); STRAP(0x133); STRAP(0x134); STRAP(0x135); STRAP(0x136); STRAP(0x137)
	STRAP(0x138); STRAP(0x139); STRAP(0x13a); STRAP(0x13b); STRAP(0x13c); STRAP(0x13d); STRAP(0x13e); STRAP(0x13f)
	STRAP(0x140); STRAP(0x141); STRAP(0x142); STRAP(0x143); STRAP(0x144); STRAP(0x145); STRAP(0x146); STRAP(0x147)
	STRAP(0x148); STRAP(0x149); STRAP(0x14a); STRAP(0x14b); STRAP(0x14c); STRAP(0x14d); STRAP(0x14e); STRAP(0x14f)
	STRAP(0x150); STRAP(0x151); STRAP(0x152); STRAP(0x153); STRAP(0x154); STRAP(0x155); STRAP(0x156); STRAP(0x157)
	STRAP(0x158); STRAP(0x159); STRAP(0x15a); STRAP(0x15b); STRAP(0x15c); STRAP(0x15d); STRAP(0x15e); STRAP(0x15f)
	STRAP(0x160); STRAP(0x161); STRAP(0x162); STRAP(0x163); STRAP(0x164); STRAP(0x165); STRAP(0x166); STRAP(0x167)
	STRAP(0x168); STRAP(0x169); STRAP(0x16a); STRAP(0x16b); STRAP(0x16c); STRAP(0x16d); STRAP(0x16e); STRAP(0x16f)
	STRAP(0x170); STRAP(0x171); STRAP(0x172); STRAP(0x173); STRAP(0x174); STRAP(0x175); STRAP(0x176); STRAP(0x177)
	STRAP(0x178); STRAP(0x179); STRAP(0x17a); STRAP(0x17b); STRAP(0x17c); STRAP(0x17d); STRAP(0x17e); STRAP(0x17f)
	! Traps beyond 0x17f are reserved
	UTRAP(0x180); UTRAP(0x181); UTRAP(0x182); UTRAP(0x183); UTRAP(0x184); UTRAP(0x185); UTRAP(0x186); UTRAP(0x187)
	UTRAP(0x188); UTRAP(0x189); UTRAP(0x18a); UTRAP(0x18b); UTRAP(0x18c); UTRAP(0x18d); UTRAP(0x18e); UTRAP(0x18f)
	UTRAP(0x190); UTRAP(0x191); UTRAP(0x192); UTRAP(0x193); UTRAP(0x194); UTRAP(0x195); UTRAP(0x196); UTRAP(0x197)
	UTRAP(0x198); UTRAP(0x199); UTRAP(0x19a); UTRAP(0x19b); UTRAP(0x19c); UTRAP(0x19d); UTRAP(0x19e); UTRAP(0x19f)
	UTRAP(0x1a0); UTRAP(0x1a1); UTRAP(0x1a2); UTRAP(0x1a3); UTRAP(0x1a4); UTRAP(0x1a5); UTRAP(0x1a6); UTRAP(0x1a7)
	UTRAP(0x1a8); UTRAP(0x1a9); UTRAP(0x1aa); UTRAP(0x1ab); UTRAP(0x1ac); UTRAP(0x1ad); UTRAP(0x1ae); UTRAP(0x1af)
	UTRAP(0x1b0); UTRAP(0x1b1); UTRAP(0x1b2); UTRAP(0x1b3); UTRAP(0x1b4); UTRAP(0x1b5); UTRAP(0x1b6); UTRAP(0x1b7)
	UTRAP(0x1b8); UTRAP(0x1b9); UTRAP(0x1ba); UTRAP(0x1bb); UTRAP(0x1bc); UTRAP(0x1bd); UTRAP(0x1be); UTRAP(0x1bf)
	UTRAP(0x1c0); UTRAP(0x1c1); UTRAP(0x1c2); UTRAP(0x1c3); UTRAP(0x1c4); UTRAP(0x1c5); UTRAP(0x1c6); UTRAP(0x1c7)
	UTRAP(0x1c8); UTRAP(0x1c9); UTRAP(0x1ca); UTRAP(0x1cb); UTRAP(0x1cc); UTRAP(0x1cd); UTRAP(0x1ce); UTRAP(0x1cf)
	UTRAP(0x1d0); UTRAP(0x1d1); UTRAP(0x1d2); UTRAP(0x1d3); UTRAP(0x1d4); UTRAP(0x1d5); UTRAP(0x1d6); UTRAP(0x1d7)
	UTRAP(0x1d8); UTRAP(0x1d9); UTRAP(0x1da); UTRAP(0x1db); UTRAP(0x1dc); UTRAP(0x1dd); UTRAP(0x1de); UTRAP(0x1df)
	UTRAP(0x1e0); UTRAP(0x1e1); UTRAP(0x1e2); UTRAP(0x1e3); UTRAP(0x1e4); UTRAP(0x1e5); UTRAP(0x1e6); UTRAP(0x1e7)
	UTRAP(0x1e8); UTRAP(0x1e9); UTRAP(0x1ea); UTRAP(0x1eb); UTRAP(0x1ec); UTRAP(0x1ed); UTRAP(0x1ee); UTRAP(0x1ef)
	UTRAP(0x1f0); UTRAP(0x1f1); UTRAP(0x1f2); UTRAP(0x1f3); UTRAP(0x1f4); UTRAP(0x1f5); UTRAP(0x1f6); UTRAP(0x1f7)
	UTRAP(0x1f8); UTRAP(0x1f9); UTRAP(0x1fa); UTRAP(0x1fb); UTRAP(0x1fc); UTRAP(0x1fd); UTRAP(0x1fe); UTRAP(0x1ff)

/*
 * If the cleanwin trap handler detects an overfow we come here.
 * We need to fix up the window registers, switch to the interrupt
 * stack, and then trap to the debugger.
 */
cleanwin_overflow:
	!! We've already incremented %cleanwin
	!! So restore %cwp
	rdpr	%cwp, %l0
	dec	%l0
	wrpr	%l0, %g0, %cwp
	set	EINTSTACK-STKB-CC64FSZ, %l0
	save	%l0, 0, %sp

	ta	1		! Enter debugger
	sethi	%hi(1f), %o0
	call	_C_LABEL(panic)
	 or	%o0, %lo(1f), %o0
	restore
	retry
	.data
1:
	.asciz	"Kernel stack overflow!"
	_ALIGN
	.text

#ifdef DEBUG
#define CHKREG(r) \
	ldx	[%o0 + 8*1], %o1; \
	cmp	r, %o1; \
	stx	%o0, [%o0]; \
	tne	1
	.data
globreg_debug:
	.xword	-1, 0, 0, 0, 0, 0, 0, 0
	.text
globreg_set:
	save	%sp, -CC64FSZ, %sp
	set	globreg_debug, %o0
	stx	%g0, [%o0]
	stx	%g1, [%o0 + 8*1]
	stx	%g2, [%o0 + 8*2]
	stx	%g3, [%o0 + 8*3]
	stx	%g4, [%o0 + 8*4]
	stx	%g5, [%o0 + 8*5]
	stx	%g6, [%o0 + 8*6]
	stx	%g7, [%o0 + 8*7]
	ret
	 restore
globreg_check:
	save	%sp, -CC64FSZ, %sp
	rd	%pc, %o7
	set	globreg_debug, %o0
	ldx	[%o0], %o1
	brnz,pn	%o1, 1f		! Don't re-execute this
	CHKREG(%g1)
	CHKREG(%g2)
	CHKREG(%g3)
	CHKREG(%g4)
	CHKREG(%g5)
	CHKREG(%g6)
	CHKREG(%g7)
	nop
1:	ret
	 restore

	/*
	 * Checkpoint:	 store a byte value at DATA_START+0x21
	 *		uses two temp regs
	 */
#define CHKPT(r1,r2,val) \
	sethi	%hi(DATA_START), r1; \
	mov	val, r2; \
	stb	r2, [r1 + 0x21]

	/*
	 * Debug routine:
	 *
	 * If datafault manages to get an unaligned pmap entry
	 * we come here.  We want to save as many regs as we can.
	 * %g3 has the sfsr, and %g7 the result of the wstate
	 * both of which we can toast w/out much lossage.
	 *
	 */
	.data
pmap_dumpflag:
	.xword	0		! semaphore
	.globl	pmap_dumparea	! Get this into the kernel syms
pmap_dumparea:
	.space	(32*8)		! room to save 32 registers
pmap_edumparea:
	.text
pmap_screwup:
	rd	%pc, %g3
	sub	%g3, (pmap_edumparea-pmap_dumparea), %g3! pc relative addressing 8^)
	ldstub	[%g3+( 0*0x8)], %g3
	tst	%g3		! Semaphore set?
	tnz	%xcc, 1; nop		! Then trap
	set	pmap_dumparea, %g3
	stx	%g3, [%g3+( 0*0x8)]	! set semaphore
	stx	%g1, [%g3+( 1*0x8)]	! Start saving regs
	stx	%g2, [%g3+( 2*0x8)]
	stx	%g3, [%g3+( 3*0x8)]	! Redundant, I know...
	stx	%g4, [%g3+( 4*0x8)]
	stx	%g5, [%g3+( 5*0x8)]
	stx	%g6, [%g3+( 6*0x8)]
	stx	%g7, [%g3+( 7*0x8)]
	stx	%i0, [%g3+( 8*0x8)]
	stx	%i1, [%g3+( 9*0x8)]
	stx	%i2, [%g3+(10*0x8)]
	stx	%i3, [%g3+(11*0x8)]
	stx	%i4, [%g3+(12*0x8)]
	stx	%i5, [%g3+(13*0x8)]
	stx	%i6, [%g3+(14*0x8)]
	stx	%i7, [%g3+(15*0x8)]
	stx	%l0, [%g3+(16*0x8)]
	stx	%l1, [%g3+(17*0x8)]
	stx	%l2, [%g3+(18*0x8)]
	stx	%l3, [%g3+(19*0x8)]
	stx	%l4, [%g3+(20*0x8)]
	stx	%l5, [%g3+(21*0x8)]
	stx	%l6, [%g3+(22*0x8)]
	stx	%l7, [%g3+(23*0x8)]
	stx	%o0, [%g3+(24*0x8)]
	stx	%o1, [%g3+(25*0x8)]
	stx	%o2, [%g3+(26*0x8)]
	stx	%o3, [%g3+(27*0x8)]
	stx	%o4, [%g3+(28*0x8)]
	stx	%o5, [%g3+(29*0x8)]
	stx	%o6, [%g3+(30*0x8)]
	stx	%o7, [%g3+(31*0x8)]
	ta	1; nop		! Break into the debugger

#else
#define	CHKPT(r1,r2,val)
#define CHKREG(r)
#endif

#ifdef DEBUG_NOTDEF
/*
 * A hardware red zone is impossible.  We simulate one in software by
 * keeping a `red zone' pointer; if %sp becomes less than this, we panic.
 * This is expensive and is only enabled when debugging.
 */
#define	REDSIZE	(USIZ)		/* Mark used portion of user structure out of bounds */
#define	REDSTACK 2048		/* size of `panic: stack overflow' region */
	.data
	_ALIGN
redzone:
	.xword	_C_LABEL(idle_u) + REDSIZE
redstack:
	.space	REDSTACK
eredstack:
Lpanic_red:
	.asciz	"kernel stack overflow"
	_ALIGN
	.text

	/* set stack pointer redzone to base+minstack; alters base */
#define	SET_SP_REDZONE(base, tmp) \
	add	base, REDSIZE, base; \
	sethi	%hi(_C_LABEL(redzone)), tmp; \
	stx	base, [tmp + %lo(_C_LABEL(redzone))]

	/* variant with a constant */
#define	SET_SP_REDZONE_CONST(const, tmp1, tmp2) \
	set	(const) + REDSIZE, tmp1; \
	sethi	%hi(_C_LABEL(redzone)), tmp2; \
	stx	tmp1, [tmp2 + %lo(_C_LABEL(redzone))]

	/* check stack pointer against redzone (uses two temps) */
#define	CHECK_SP_REDZONE(t1, t2) \
	sethi	KERNBASE, t1;	\
	cmp	%sp, t1;	\
	blu,pt	%xcc, 7f;	\
	 sethi	%hi(_C_LABEL(redzone)), t1; \
	ldx	[t1 + %lo(_C_LABEL(redzone))], t2; \
	cmp	%sp, t2;	/* if sp >= t2, not in red zone */ \
	blu	panic_red; nop;	/* and can continue normally */ \
7:

panic_red:
	/* move to panic stack */
	stx	%g0, [t1 + %lo(_C_LABEL(redzone))];
	set	eredstack - BIAS, %sp;
	/* prevent panic() from lowering ipl */
	sethi	%hi(_C_LABEL(panicstr)), t2;
	set	Lpanic_red, t2;
	st	t2, [t1 + %lo(_C_LABEL(panicstr))];
	wrpr	g0, 15, %pil		/* t1 = splhigh() */
	save	%sp, -CCF64SZ, %sp;	/* preserve current window */
	sethi	%hi(Lpanic_red), %o0;
	call	_C_LABEL(panic);
	 or %o0, %lo(Lpanic_red), %o0;


#else

#define	SET_SP_REDZONE(base, tmp)
#define	SET_SP_REDZONE_CONST(const, t1, t2)
#define	CHECK_SP_REDZONE(t1, t2)
#endif

#define TRACESIZ	0x01000
	.globl	_C_LABEL(trap_trace)
	.globl	_C_LABEL(trap_trace_ptr)
	.globl	_C_LABEL(trap_trace_end)
	.globl	_C_LABEL(trap_trace_dis)
	.data
_C_LABEL(trap_trace_dis):
	.word	1, 1		! Starts disabled.  DDB turns it on.
_C_LABEL(trap_trace_ptr):
	.word	0, 0, 0, 0
_C_LABEL(trap_trace):
	.space	TRACESIZ
_C_LABEL(trap_trace_end):
	.space	0x20		! safety margin
#if	defined(TRAPTRACE)||defined(FLTRACE)
#define TRACEPTR	(_C_LABEL(trap_trace_ptr)-_C_LABEL(trap_trace))
#define TRACEDIS	(_C_LABEL(trap_trace_dis)-_C_LABEL(trap_trace))
#define	TRACEIT(tt,r3,r4,r2,r6,r7)					\
	set	trap_trace, r2;						\
	lduw	[r2+TRACEDIS], r4;					\
	brnz,pn	r4, 1f;							\
	 lduw	[r2+TRACEPTR], r3;					\
	rdpr	%tl, r4;						\
	cmp	r4, 1;							\
	sllx	r4, 13, r4;						\
	rdpr	%pil, r6;						\
	or	r4, %g5, r4;						\
	mov	%g0, %g5;						\
	andncc	r3, (TRACESIZ-1), %g0;	/* At end of buffer? */		\
	sllx	r6, 9, r6;						\
	or	r6, r4, r4;						\
	movnz	%icc, %g0, r3;		/* Wrap buffer if needed */	\
	rdpr	%tstate, r6;						\
	rdpr	%tpc, r7;						\
	sth	r4, [r2+r3];						\
	inc	2, r3;							\
	sth	%g5, [r2+r3];						\
	inc	2, r3;							\
	stw	r6, [r2+r3];						\
	inc	4, r3;							\
	stw	%sp, [r2+r3];						\
	inc	4, r3;							\
	stw	r7, [r2+r3];						\
	inc	4, r3;							\
	mov	TLB_TAG_ACCESS, r7;					\
	ldxa	[r7] ASI_DMMU, r7;					\
	stw	r7, [r2+r3];						\
	inc	4, r3;							\
	stw	r3, [r2+TRACEPTR];					\
1:

		
	.text
traceit:
	set	trap_trace, %g2
	lduw	[%g2+TRACEDIS], %g4
	brnz,pn	%g4, 1f
	 lduw	[%g2+TRACEPTR], %g3
	rdpr	%tl, %g4
	rdpr	%tt, %g5
	set	CURPROC, %g6
	cmp	%g4, 1
	sllx	%g4, 13, %g4
	bnz,a,pt	%icc, 3f
	 clr	%g6
	cmp	%g5, 0x68
	bnz,a,pt	%icc, 3f
	 clr	%g6
	cmp	%g5, 0x64
	bnz,a,pt	%icc, 3f
	 clr	%g6
	cmp	%g5, 0x6c
	bnz,a,pt	%icc, 3f
	 clr	%g6
	LDPTR	[%g6], %g6
3:
	or	%g4, %g5, %g4
	mov	%g0, %g5
	brz,pn	%g6, 2f
	 andncc	%g3, (TRACESIZ-1), %g0	! At end of buffer? wrap
	LDPTR	[%g6+P_PID], %g5	! Load PID

	set	CPCB, %g6	! Load up nsaved
	LDPTR	[%g6], %g6
	ldub	[%g6 + PCB_NSAVED], %g6
	sllx	%g6, 9, %g6
	or	%g6, %g4, %g4
2:

	movnz	%icc, %g0, %g3		! Wrap buffer if needed
	rdpr	%tstate, %g6
	rdpr	%tpc, %g7
	sth	%g4, [%g2+%g3]
	inc	2, %g3
	sth	%g5, [%g2+%g3]
	inc	2, %g3
	stw	%g6, [%g2+%g3]
	inc	4, %g3
	stw	%sp, [%g2+%g3]
	inc	4, %g3
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	mov	TLB_TAG_ACCESS, %g7
	ldxa	[%g7] ASI_DMMU, %g7
	stw	%g7, [%g2+%g3]
	inc	4, %g3
1:
	jmpl	%g1, %g0
	 stw	%g3, [%g2+TRACEPTR]
traceitwin:
	set	trap_trace, %l2
	lduw	[%l2+TRACEDIS], %l4
	brnz,pn	%l4, 1f
	 nop
	lduw	[%l2+TRACEPTR], %l3
	rdpr	%tl, %l4
	rdpr	%tt, %l5
	sllx	%l4, 13, %l4
	or	%l4, %l5, %l4
	clr	%l5		! Don't load PID
	andncc	%l3, (TRACESIZ-1), %g0
	movnz	%icc, %g0, %l3	! Wrap?

	clr	%l0		! Don't load nsaved
	sllx	%l0, 9, %l1
	or	%l1, %l4, %l4
	rdpr	%tpc, %l7

	sth	%l4, [%l2+%l3]
	inc	2, %l3
	sth	%l5, [%l2+%l3]
	inc	2, %l3
	stw	%l0, [%l2+%l3]
	inc	4, %l3
	stw	%sp, [%l2+%l3]
	inc	4, %l3
	stw	%l7, [%l2+%l3]
	inc	4, %l3
	stw	%g0, [%l2+%l3]
	inc	4, %l3
	stw	%l3, [%l2+TRACEPTR]
1:
	jmpl	%l6, %g0
	 nop
reload64:
	ldxa	[%sp+BIAS+0x00]%asi, %l0
	ldxa	[%sp+BIAS+0x08]%asi, %l1
	ldxa	[%sp+BIAS+0x10]%asi, %l2
	ldxa	[%sp+BIAS+0x18]%asi, %l3
	ldxa	[%sp+BIAS+0x20]%asi, %l4
	ldxa	[%sp+BIAS+0x28]%asi, %l5
	ldxa	[%sp+BIAS+0x30]%asi, %l6
	ldxa	[%sp+BIAS+0x38]%asi, %l7
	CLRTT
	retry
reload32:
	lda	[%sp+0x00]%asi, %l0
	lda	[%sp+0x04]%asi, %l1
	lda	[%sp+0x08]%asi, %l2
	lda	[%sp+0x0c]%asi, %l3
	lda	[%sp+0x10]%asi, %l4
	lda	[%sp+0x14]%asi, %l5
	lda	[%sp+0x18]%asi, %l6
	lda	[%sp+0x1c]%asi, %l7
	CLRTT
	retry
#endif

/*
 * Every trap that enables traps must set up stack space.
 * If the trap is from user mode, this involves switching to the kernel
 * stack for the current process, which means putting WSTATE_KERN in
 * %wstate, moving the contents of %canrestore to %otherwin and clearing
 * %canrestore.
 *
 * Things begin to grow uglier....
 *
 * It is possible that the user stack is invalid or unmapped.  This
 * happens right after a fork() before the stack is faulted in to the
 * child process address space, or when growing the stack, or when paging
 * the stack in from swap.  In this case we can't save the contents of the
 * CPU to the stack, so we must save it to the PCB.
 *
 * ASSUMPTIONS: TRAP_SETUP() is called with:
 *	%i0-%o7 - previous stack frame
 *	%g1-%g7 - One of: interrupt globals, alternate globals, MMU globals.
 *
 * We need to allocate a trapframe, generate a new stack window then
 * switch to the normal globals, not necessarily in that order.  The
 * trapframe is allocated on either kernel (or interrupt for INTR_SETUP())
 * stack.
 *
 * The `stackspace' argument is the number of stack bytes to allocate
 * for register-saving, and must be at least -64 (and typically more,
 * for global registers and %y).
 *
 * Trapframes should use -CCFSZ-TF_SIZE.  (TF_SIZE = sizeof(struct trapframe);
 * see trap.h.  This basically means EVERYONE.  Interrupt frames could
 * get away with less, but currently do not.)
 *
 * The basic outline here is:
 *
 *	if (trap came from kernel mode) {
 *		%sp = %fp - stackspace;
 *	} else {
 *		%sp = (top of kernel stack) - stackspace;
 *	}
 *
 * NOTE: if you change this code, you will have to look carefully
 * at the window overflow and underflow handlers and make sure they
 * have similar changes made as needed.
 */

/*
 * v9 machines do not have a trap window.
 *
 * When we take a trap the trap state is pushed on to the stack of trap
 * registers, interrupts are disabled, then we switch to an alternate set
 * of global registers.
 *
 * The trap handling code needs to allocate a trap frame on the kernel, or
 * for interrupts, the interrupt stack, save the out registers to the trap
 * frame, then switch to the normal globals and save them to the trap frame
 * too.
 *
 * Since kernel stacks are all on one page and the interrupt stack is entirely
 * within the locked TLB, we can use physical addressing to save out our
 * trap frame so we don't trap during the TRAP_SETUP() operation.  There
 * is unfortunately no supportable method for issuing a non-trapping save.
 *
 * However, if we use physical addresses to save our trapframe, we will need
 * to clear out the data cache before continuing much further.
 *
 * In short, what we need to do is:
 *
 *	all preliminary processing is done using the alternate globals
 *
 *	When we allocate our trap windows we must give up our globals because
 *	their state may have changed during the save operation
 *
 *	we need to save our normal globals as soon as we have a stack
 *
 * Finally, we may now call C code.
 *
 * This macro will destroy %g5-%g7.  %g0-%g4 remain unchanged.
 *
 * In order to properly handle nested traps without lossage, alternate
 * global %g6 is used as a kernel stack pointer.  It is set to the last
 * allocated stack pointer (trapframe) and the old value is stored in
 * tf_kstack.  It is restored when returning from a trap.  It is cleared
 * on entering user mode.
 */

 /*
  * Other misc. design criteria:
  *
  * When taking an address fault, fault info is in the sfsr, sfar,
  * TLB_TAG_ACCESS registers.  If we take another address fault
  * while trying to handle the first fault then that information,
  * the only information that tells us what address we trapped on,
  * can be potentially lost.  This trap can be caused when allocating
  * a register window with which to handle the trap because the save
  * may try to store or restore a register window that corresponds
  * to part of the stack that is not mapped.  Preventing this trap,
  * while possible, is much too complicated to do in a trap handler,
  * and then we will need to do just as much work to restore the processor
  * window state.
  *
  * Possible solutions to the problem:
  *
  * Since we have separate AG, MG, and IG, we could have all traps
  * above level-1 preserve AG and use other registers.  This causes
  * a problem for the return from trap code which is coded to use
  * alternate globals only.
  *
  * We could store the trapframe and trap address info to the stack
  * using physical addresses.  Then we need to read it back using
  * physical addressing, or flush the D$.
  *
  * We could identify certain registers to hold address fault info.
  * this means that these registers need to be preserved across all
  * fault handling.  But since we only have 7 useable globals, that
  * really puts a cramp in our style.
  *
  * Finally, there is the issue of returning from kernel mode to user
  * mode.  If we need to issue a restore of a user window in kernel
  * mode, we need the window control registers in a user mode setup.
  * If the trap handlers notice the register windows are in user mode,
  * they will allocate a trapframe at the bottom of the kernel stack,
  * overwriting the frame we were trying to return to.  This means that
  * we must complete the restoration of all registers *before* switching
  * to a user-mode window configuration.
  *
  * Essentially we need to be able to write re-entrant code w/no stack.
  */
	.data
trap_setup_msg:
	.asciz	"TRAP_SETUP: tt=%x osp=%x nsp=%x tl=%x tpc=%x\n"
	_ALIGN
intr_setup_msg:
	.asciz	"INTR_SETUP: tt=%x osp=%x nsp=%x tl=%x tpc=%x\n"
	_ALIGN
	.text

#ifdef _LP64
#define	TRAP_SETUP(stackspace) \
	sethi	%hi(USPACE), %g7; \
	sethi	%hi(CPCB), %g6; \
	or	%g7, %lo(USPACE), %g7; \
	sethi	%hi((stackspace)), %g5; \
	ldx	[%g6 + %lo(CPCB)], %g6; \
	or	%g5, %lo((stackspace)), %g5; \
	add	%g6, %g7, %g6; \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	\
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	subcc	%g7, WSTATE_KERN, %g7;				/* Compare & leave in register */ \
	srl	%g6, 0, %g6;					/* XXXXX truncate at 32-bits */ \
	movz	%icc, %sp, %g6;					/* Select old (kernel) stack or base of kernel stack */ \
	add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	btst	1, %g6;						/* Fixup 64-bit stack if necessary */ \
	add	%g6, -BIAS, %g5; \
	movz	%icc, %g5, %g6; \
	\
	stx	%g1, [%g6 + CC64FSZ + BIAS + TF_FAULT]; \
	stx	%l0, [%g6 + CC64FSZ + BIAS + TF_L + (0*8)];		/* Save local registers to trap frame */ \
	stx	%l1, [%g6 + CC64FSZ + BIAS + TF_L + (1*8)]; \
	stx	%l2, [%g6 + CC64FSZ + BIAS + TF_L + (2*8)]; \
	stx	%l3, [%g6 + CC64FSZ + BIAS + TF_L + (3*8)]; \
	stx	%l4, [%g6 + CC64FSZ + BIAS + TF_L + (4*8)]; \
	stx	%l5, [%g6 + CC64FSZ + BIAS + TF_L + (5*8)]; \
	stx	%l6, [%g6 + CC64FSZ + BIAS + TF_L + (6*8)]; \
	\
	stx	%l7, [%g6 + CC64FSZ + BIAS + TF_L + (7*8)]; \
	stx	%i0, [%g6 + CC64FSZ + BIAS + TF_I + (0*8)];		/* Save in registers to trap frame */ \
	stx	%i1, [%g6 + CC64FSZ + BIAS + TF_I + (1*8)]; \
	stx	%i2, [%g6 + CC64FSZ + BIAS + TF_I + (2*8)]; \
	stx	%i3, [%g6 + CC64FSZ + BIAS + TF_I + (3*8)]; \
	stx	%i4, [%g6 + CC64FSZ + BIAS + TF_I + (4*8)]; \
	stx	%i5, [%g6 + CC64FSZ + BIAS + TF_I + (5*8)]; \
	stx	%i6, [%g6 + CC64FSZ + BIAS + TF_I + (6*8)]; \
	\
	stx	%i7, [%g6 + CC64FSZ + BIAS + TF_I + (7*8)]; \
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + BIAS + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + BIAS + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + BIAS + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + BIAS + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + BIAS + TF_O + (4*8)]; \
	stx	%i5, [%sp + CC64FSZ + BIAS + TF_O + (5*8)]; \
	stx	%i6, [%sp + CC64FSZ + BIAS + TF_O + (6*8)]; \
	\
	stx	%i7, [%sp + CC64FSZ + BIAS + TF_O + (7*8)]; \
	rdpr	%wstate, %g7; sub %g7, WSTATE_KERN, %g7; /* DEBUG */ \
	brz,pn	%g7, 1f;					/* If we were in kernel mode start saving globals */ \
	 rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	/* came from user mode -- switch to kernel mode stack */ \
	wrpr	%g0, 0, %canrestore; \
	wrpr	%g0, %g5, %otherwin; \
	mov	CTX_PRIMARY, %g7; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	\
	stxa	%g0, [%g7] ASI_DMMU; 				/* Switch MMU to kernel primary context */ \
	sethi	%hi(KERNBASE), %g5; \
	membar	#Sync;						/* XXXX Should be taken care of by flush */ \
	flush	%g5;						/* Some convenient address that won't trap */ \
1:

/*
 * Interrupt setup is almost exactly like trap setup, but we need to
 * go to the interrupt stack if (a) we came from user mode or (b) we
 * came from kernel mode on the kernel stack.
 *
 * We don't guarantee any registers are preserved during this operation.
 */
#define	INTR_SETUP(stackspace) \
	sethi	%hi(EINTSTACK-BIAS), %g6; \
	sethi	%hi((stackspace)), %g5; \
	btst	1, %sp; \
	bnz,pt	%icc, 0f; \
	 mov	%sp, %g1; \
	add	%sp, -BIAS, %g1; \
0: \
	or	%g6, %lo(EINTSTACK-BIAS), %g6; \
	set	(EINTSTACK-INTSTACK), %g7;	/* XXXXXXXXXX This assumes kernel addresses are unique from user addresses */ \
	or	%g5, %lo((stackspace)), %g5; \
	sub	%g6, %g1, %g2;					/* Determine if we need to switch to intr stack or not */ \
	dec	%g7;						/* Make it into a mask */ \
	andncc	%g2, %g7, %g0;					/* XXXXXXXXXX This assumes kernel addresses are unique from user addresses */ \
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	srl	%g1, 0, %g1;					/* XXXXX truncate at 32-bits */ \
	movz	%xcc, %g1, %g6;					/* Stay on interrupt stack? */ \
	add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	\
	stx	%l0, [%g6 + CC64FSZ + BIAS + TF_L + (0*8)];		/* Save local registers to trap frame */ \
	stx	%l1, [%g6 + CC64FSZ + BIAS + TF_L + (1*8)]; \
	stx	%l2, [%g6 + CC64FSZ + BIAS + TF_L + (2*8)]; \
	stx	%l3, [%g6 + CC64FSZ + BIAS + TF_L + (3*8)]; \
	stx	%l4, [%g6 + CC64FSZ + BIAS + TF_L + (4*8)]; \
	stx	%l5, [%g6 + CC64FSZ + BIAS + TF_L + (5*8)]; \
	stx	%l6, [%g6 + CC64FSZ + BIAS + TF_L + (6*8)]; \
	stx	%l7, [%g6 + CC64FSZ + BIAS + TF_L + (7*8)]; \
	stx	%i0, [%g6 + CC64FSZ + BIAS + TF_I + (0*8)];		/* Save in registers to trap frame */ \
	stx	%i1, [%g6 + CC64FSZ + BIAS + TF_I + (1*8)]; \
	stx	%i2, [%g6 + CC64FSZ + BIAS + TF_I + (2*8)]; \
	stx	%i3, [%g6 + CC64FSZ + BIAS + TF_I + (3*8)]; \
	stx	%i4, [%g6 + CC64FSZ + BIAS + TF_I + (4*8)]; \
	stx	%i5, [%g6 + CC64FSZ + BIAS + TF_I + (5*8)]; \
	stx	%i6, [%g6 + CC64FSZ + BIAS + TF_I + (6*8)]; \
	stx	%i7, [%g6 + CC64FSZ + BIAS + TF_I + (7*8)]; \
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + BIAS + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + BIAS + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + BIAS + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + BIAS + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + BIAS + TF_O + (4*8)]; \
	stx	%i5, [%sp + CC64FSZ + BIAS + TF_O + (5*8)]; \
	stx	%i6, [%sp + CC64FSZ + BIAS + TF_O + (6*8)]; \
	stx	%i6, [%sp + CC64FSZ + BIAS + TF_G + (0*8)];		/* Save fp in clockframe->cf_fp */ \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	stx	%i7, [%sp + CC64FSZ + BIAS + TF_O + (7*8)]; \
	cmp	%g7, WSTATE_KERN;				/* Compare & leave in register */ \
	be,pn	%icc, 1f;					/* If we were in kernel mode start saving globals */ \
	/* came from user mode -- switch to kernel mode stack */ \
	 rdpr	%otherwin, %g5;					/* Has this already been done? */ \
	/* mov %g5, %g5; tst %g5; tnz %xcc, 1; nop; /* DEBUG -- this should _NEVER_ happen */ \
	brnz,pn	%g5, 1f;					/* Don't set this twice */ \
	 rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	wrpr	%g0, 0, %canrestore; \
	mov	CTX_PRIMARY, %g7; \
	wrpr	%g0, %g5, %otherwin; \
	sethi	%hi(KERNBASE), %g5; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	stxa	%g0, [%g7] ASI_DMMU; 				/* Switch MMU to kernel primary context */ \
	membar	#Sync;						/* XXXX Should be taken care of by flush */ \
	flush	%g5;						/* Some convenient address that won't trap */ \
1:
#else
#define	TRAP_SETUP(stackspace) \
	sethi	%hi(USPACE), %g7; \
	sethi	%hi(CPCB), %g6; \
	or	%g7, %lo(USPACE), %g7; \
	sethi	%hi((stackspace)), %g5; \
	lduw	[%g6 + %lo(CPCB)], %g6; \
	or	%g5, %lo((stackspace)), %g5; \
	add	%g6, %g7, %g6; \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	\
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	subcc	%g7, WSTATE_KERN, %g7;				/* Compare & leave in register */ \
	movz	%icc, %sp, %g6;					/* Select old (kernel) stack or base of kernel stack */ \
	add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	btst	1, %g6;						/* Fixup 64-bit stack if necessary */ \
	add	%g6, BIAS, %g5; \
	srl	%g6, 0, %g6;					/* truncate at 32-bits */ \
	movne	%icc, %g5, %g6; \
	\
	stx	%g1, [%g6 + CC64FSZ + STKB + TF_FAULT]; \
	stx	%l0, [%g6 + CC64FSZ + STKB + TF_L + (0*8)];		/* Save local registers to trap frame */ \
	stx	%l1, [%g6 + CC64FSZ + STKB + TF_L + (1*8)]; \
	stx	%l2, [%g6 + CC64FSZ + STKB + TF_L + (2*8)]; \
	stx	%l3, [%g6 + CC64FSZ + STKB + TF_L + (3*8)]; \
	stx	%l4, [%g6 + CC64FSZ + STKB + TF_L + (4*8)]; \
	stx	%l5, [%g6 + CC64FSZ + STKB + TF_L + (5*8)]; \
	stx	%l6, [%g6 + CC64FSZ + STKB + TF_L + (6*8)]; \
	\
	stx	%l7, [%g6 + CC64FSZ + STKB + TF_L + (7*8)]; \
	stx	%i0, [%g6 + CC64FSZ + STKB + TF_I + (0*8)];		/* Save in registers to trap frame */ \
	stx	%i1, [%g6 + CC64FSZ + STKB + TF_I + (1*8)]; \
	stx	%i2, [%g6 + CC64FSZ + STKB + TF_I + (2*8)]; \
	stx	%i3, [%g6 + CC64FSZ + STKB + TF_I + (3*8)]; \
	stx	%i4, [%g6 + CC64FSZ + STKB + TF_I + (4*8)]; \
	stx	%i5, [%g6 + CC64FSZ + STKB + TF_I + (5*8)]; \
	stx	%i6, [%g6 + CC64FSZ + STKB + TF_I + (6*8)]; \
	\
	stx	%i7, [%g6 + CC64FSZ + STKB + TF_I + (7*8)]; \
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + STKB + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + STKB + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + STKB + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + STKB + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + STKB + TF_O + (4*8)]; \
	stx	%i5, [%sp + CC64FSZ + STKB + TF_O + (5*8)]; \
	stx	%i6, [%sp + CC64FSZ + STKB + TF_O + (6*8)]; \
	\
	stx	%i7, [%sp + CC64FSZ + STKB + TF_O + (7*8)]; \
	rdpr	%wstate, %g7; sub %g7, WSTATE_KERN, %g7; /* DEBUG */ \
	brz,pn	%g7, 1f;					/* If we were in kernel mode start saving globals */ \
	 rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	/* came from user mode -- switch to kernel mode stack */ \
	wrpr	%g0, 0, %canrestore; \
	wrpr	%g0, %g5, %otherwin; \
	mov	CTX_PRIMARY, %g7; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	\
	stxa	%g0, [%g7] ASI_DMMU; 				/* Switch MMU to kernel primary context */ \
	sethi	%hi(KERNBASE), %g5; \
	membar	#Sync;						/* XXXX Should be taken care of by flush */ \
	flush	%g5;						/* Some convenient address that won't trap */ \
1:

/*
 * Interrupt setup is almost exactly like trap setup, but we need to
 * go to the interrupt stack if (a) we came from user mode or (b) we
 * came from kernel mode on the kernel stack.
 *
 * We don't guarantee any registers are preserved during this operation.
 */
#define	INTR_SETUP(stackspace) \
	sethi	%hi(EINTSTACK), %g6; \
	sethi	%hi((stackspace)), %g5; \
	btst	1, %sp; \
	bz,pt	%icc, 0f; \
	 mov	%sp, %g1; \
	add	%sp, BIAS, %g1; \
0: \
	srl	%g1, 0, %g1;					/* truncate at 32-bits */ \
	or	%g6, %lo(EINTSTACK), %g6; \
	set	(EINTSTACK-INTSTACK), %g7;	/* XXXXXXXXXX This assumes kernel addresses are unique from user addresses */ \
	or	%g5, %lo((stackspace)), %g5; \
	sub	%g6, %g1, %g2;					/* Determine if we need to switch to intr stack or not */ \
	dec	%g7;						/* Make it into a mask */ \
	andncc	%g2, %g7, %g0;					/* XXXXXXXXXX This assumes kernel addresses are unique from user addresses */ \
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	movz	%icc, %g1, %g6;					/* Stay on interrupt stack? */ \
	add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	\
	stx	%l0, [%g6 + CC64FSZ + STKB + TF_L + (0*8)];		/* Save local registers to trap frame */ \
	stx	%l1, [%g6 + CC64FSZ + STKB + TF_L + (1*8)]; \
	stx	%l2, [%g6 + CC64FSZ + STKB + TF_L + (2*8)]; \
	stx	%l3, [%g6 + CC64FSZ + STKB + TF_L + (3*8)]; \
	stx	%l4, [%g6 + CC64FSZ + STKB + TF_L + (4*8)]; \
	stx	%l5, [%g6 + CC64FSZ + STKB + TF_L + (5*8)]; \
	stx	%l6, [%g6 + CC64FSZ + STKB + TF_L + (6*8)]; \
	stx	%l7, [%g6 + CC64FSZ + STKB + TF_L + (7*8)]; \
	stx	%i0, [%g6 + CC64FSZ + STKB + TF_I + (0*8)];		/* Save in registers to trap frame */ \
	stx	%i1, [%g6 + CC64FSZ + STKB + TF_I + (1*8)]; \
	stx	%i2, [%g6 + CC64FSZ + STKB + TF_I + (2*8)]; \
	stx	%i3, [%g6 + CC64FSZ + STKB + TF_I + (3*8)]; \
	stx	%i4, [%g6 + CC64FSZ + STKB + TF_I + (4*8)]; \
	stx	%i5, [%g6 + CC64FSZ + STKB + TF_I + (5*8)]; \
	stx	%i6, [%g6 + CC64FSZ + STKB + TF_I + (6*8)]; \
	stx	%i7, [%g6 + CC64FSZ + STKB + TF_I + (7*8)]; \
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + STKB + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + STKB + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + STKB + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + STKB + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + STKB + TF_O + (4*8)]; \
	stx	%i5, [%sp + CC64FSZ + STKB + TF_O + (5*8)]; \
	stx	%i6, [%sp + CC64FSZ + STKB + TF_O + (6*8)]; \
	stx	%i6, [%sp + CC64FSZ + STKB + TF_G + (0*8)];		/* Save fp in clockframe->cf_fp */ \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	stx	%i7, [%sp + CC64FSZ + STKB + TF_O + (7*8)]; \
	cmp	%g7, WSTATE_KERN;				/* Compare & leave in register */ \
	be,pn	%icc, 1f;					/* If we were in kernel mode start saving globals */ \
	/* came from user mode -- switch to kernel mode stack */ \
	 rdpr	%otherwin, %g5;					/* Has this already been done? */ \
	/* tst	%g5; tnz %xcc, 1; nop; /* DEBUG -- this should _NEVER_ happen */ \
	brnz,pn	%g5, 1f;					/* Don't set this twice */ \
	 rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	wrpr	%g0, 0, %canrestore; \
	mov	CTX_PRIMARY, %g7; \
	wrpr	%g0, %g5, %otherwin; \
	sethi	%hi(KERNBASE), %g5; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	stxa	%g0, [%g7] ASI_DMMU; 				/* Switch MMU to kernel primary context */ \
	membar	#Sync;						/* XXXX Should be taken care of by flush */ \
	flush	%g5;						/* Some convenient address that won't trap */ \
1:
#endif /* _LP64 */

#ifdef DEBUG

	/* Look up kpte to test algorithm */
	.globl	asmptechk
asmptechk:
	mov	%o0, %g4	! pmap->pm_segs
	mov	%o1, %g3	! Addr to lookup -- mind the context

	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	brz,pt	%g5, 0f					! Should be zero or -1
	 inc	%g5					! Make -1 -> 0
	brnz,pn	%g5, 1f					! Error!
0:
	 srlx	%g3, STSHIFT, %g5
	and	%g5, STMASK, %g5
	sll	%g5, 3, %g5
	add	%g4, %g5, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4		! Remember -- UNSIGNED
	DLFLUSH2(%g5)
	brz,pn	%g4, 1f					! NULL entry? check somewhere else

	 srlx	%g3, PDSHIFT, %g5
	and	%g5, PDMASK, %g5
	sll	%g5, 3, %g5
	add	%g4, %g5, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4		! Remember -- UNSIGNED
	DLFLUSH2(%g5)
	brz,pn	%g4, 1f					! NULL entry? check somewhere else

	 srlx	%g3, PTSHIFT, %g5			! Convert to ptab offset
	and	%g5, PTMASK, %g5
	sll	%g5, 3, %g5
	add	%g4, %g5, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g6
	DLFLUSH2(%g5)
	brgez,pn %g6, 1f				! Entry invalid?  Punt
	 srlx	%g6, 32, %o0
	retl
	 srl	%g6, 0, %o1
1:
	mov	%g0, %o1
	retl
	 mov	%g0, %o0

	.data
2:
	.asciz	"asmptechk: %x %x %x %x:%x\r\n"
	_ALIGN
	.text
#endif

/*
 * This is the MMU protection handler.  It's too big to fit
 * in the trap table so I moved it here.  It's relatively simple.
 * It looks up the page mapping in the page table associated with
 * the trapping context.  It checks to see if the S/W writable bit
 * is set.  If so, it sets the H/W write bit, marks the tte modified,
 * and enters the mapping into the MMU.  Otherwise it does a regular
 * data fault.
 *
 *
 */
	ICACHE_ALIGN
dmmu_write_fault:
	mov	TLB_TAG_ACCESS, %g3			! Get real fault page
	ldxa	[%g3] ASI_DMMU, %g3			! from tag access register
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
	sethi	%hi(_C_LABEL(ctxbusy)), %g4
	LDPTR	[%g4 + %lo(_C_LABEL(ctxbusy))], %g4
	sllx	%g3, (64-13), %g6			! Mask away address
	srlx	%g6, (64-13-3), %g6			! This is now the offset into ctxbusy
	ldx	[%g4+%g6], %g4				! Load up our page table.

	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	brz,pt	%g5, 0f					! Should be zero or -1
	 inc	%g5					! Make -1 + 1 -> 0
	brnz,pn	%g5, winfix				! Error!
0:
	 srlx	%g3, STSHIFT, %g5
	and	%g5, STMASK, %g5
	sll	%g5, 3, %g5
	add	%g5, %g4, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)

	srlx	%g3, PDSHIFT, %g5
	and	%g5, PDMASK, %g5
	sll	%g5, 3, %g5
	brz,pn	%g4, winfix				! NULL entry? check somewhere else
	 add	%g5, %g4, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)

	srlx	%g3, PTSHIFT, %g5			! Convert to ptab offset
	and	%g5, PTMASK, %g5
	sll	%g5, 3, %g5
	 brz,pn	%g4, winfix				! NULL entry? check somewhere else
	add	%g5, %g4, %g6
	DLFLUSH(%g6,%g5)
	ldxa	[%g6] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)
	brgez,pn %g4, winfix				! Entry invalid?  Punt
	 btst	TTE_REAL_W|TTE_W, %g4			! Is it a ref fault?
	bz,pn	%xcc, winfix				! No -- really fault
	 or	%g4, TTE_MODIFY|TTE_ACCESS|TTE_W, %g4	! Update the modified bit

#ifdef DEBUG
	/* Make sure we don't try to replace a kernel translation */
	/* This should not be necessary */
	sethi	%hi(KERNBASE), %g5			! Don't need %lo
	set	0x0400000, %g2				! 4MB
	sub	%g3, %g5, %g5
	cmp	%g5, %g2
	tlu	%xcc, 1; nop
	blu,pn	%xcc, winfix				! Next insn in delay slot is unimportant
#endif

	/* Need to check for and handle large pages. */
	srlx	%g4, 61, %g5				! Isolate the size bits
	andcc	%g5, 0x3, %g5				! 8K?
	bnz,pn	%icc, winfix				! We punt to the pmap code since we can't handle policy

	 ldxa	[%g0] ASI_DMMU_8KPTR, %g2		! Load DMMU 8K TSB pointer
	ldxa	[%g0] ASI_DMMU, %g1			! Hard coded for unified 8K TSB		Load DMMU tag target register
	stxa	%g4, [%g6] ASI_PHYS_CACHED		!  and write it out
	DLFLUSH(%g6, %g6)
	stx	%g1, [%g2]				! Update TSB entry tag
	stx	%g4, [%g2+8]				! Update TSB entry data


#ifdef DEBUG
	set	DATA_START, %g6	! debug
	stx	%g1, [%g6+0x40]	! debug
	set	0x88, %g5	! debug
	stx	%g4, [%g6+0x48]	! debug -- what we tried to enter in TLB
	stb	%g5, [%g6+0x8]	! debug
#endif
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(protfix)), %g1
	lduw	[%g1+%lo(_C_LABEL(protfix))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(protfix))]
#endif
	wr	%g0, ASI_DMMU, %asi
	stxa	%g0, [SFSR] %asi			! clear out the fault
	membar	#Sync

	sllx	%g3, (64-12), %g6			! Need to demap old entry first
	mov	0x010, %g1				! Secondary flush
	mov	0x020, %g5				! Nucleus flush
	movrz	%g6, %g5, %g1				! Pick one
	andn	%g3, 0xfff, %g6
	or	%g6, %g1, %g6
	stxa	%g6, [%g6] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync					! No real reason for this XXXX

	stxa	%g4, [%g0] ASI_DMMU_DATA_IN		! Enter new mapping
	membar	#Sync
	retry

/*
 * Each memory data access fault from a fast access miss handler comes here.
 * We will quickly check if this is an original prom mapping before going
 * to the generic fault handler
 *
 * We will assume that %pil is not lost so we won't bother to save it
 * unless we're in an interrupt handler.
 *
 * On entry:
 *	We are on one of the alternate set of globals
 *	%g1 = MMU tag target
 *	%g2 = 8Kptr
 *	%g3 = TLB TAG ACCESS
 *
 * On return:
 *
 */
	ICACHE_ALIGN
data_miss:
#ifdef TRAPSTATS
	set	_C_LABEL(kdmiss), %g3
	set	_C_LABEL(udmiss), %g4
	rdpr	%tl, %g6
	dec	%g6
	movrz	%g6, %g4, %g3
	lduw	[%g3], %g4
	inc	%g4
	stw	%g4, [%g3]
#endif

	mov	TLB_TAG_ACCESS, %g3			! Get real fault page
	sethi	%hi(_C_LABEL(ctxbusy)), %g4
	ldxa	[%g3] ASI_DMMU, %g3			! from tag access register
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
	LDPTR	[%g4 + %lo(_C_LABEL(ctxbusy))], %g4
	sllx	%g3, (64-13), %g6			! Mask away address
	srlx	%g6, (64-13-3), %g6			! This is now the offset into ctxbusy
	ldx	[%g4+%g6], %g4				! Load up our page table.

#ifdef DEBUG
	/* Make sure we don't try to replace a kernel translation */
	/* This should not be necessary */
	brnz,pt	%g6, Ludata_miss			! If user context continue miss
	sethi	%hi(KERNBASE), %g5			! Don't need %lo
	set	0x0800000, %g6				! 8MB
	sub	%g3, %g5, %g5
	cmp	%g5, %g6
	sethi	%hi(DATA_START), %g7
	mov	6, %g6		! debug
	stb	%g6, [%g7+0x20]	! debug
	tlu	%xcc, 1; nop
	blu,pn	%xcc, winfix				! Next insn in delay slot is unimportant
	 mov	7, %g6		! debug
	stb	%g6, [%g7+0x20]	! debug
#endif

	/*
	 * Try to parse our page table.
	 */
Ludata_miss:
	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	brz,pt	%g5, 0f					! Should be zero or -1
	 inc	%g5					! Make -1 -> 0
	brnz,pn	%g5, winfix				! Error!
0:
	srlx	%g3, STSHIFT, %g5
	and	%g5, STMASK, %g5
	sll	%g5, 3, %g5
	add	%g5, %g4, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)

	srlx	%g3, PDSHIFT, %g5
	and	%g5, PDMASK, %g5
	sll	%g5, 3, %g5
	brz,pn	%g4, winfix				! NULL entry? check somewhere else
	 add	%g5, %g4, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)

	srlx	%g3, PTSHIFT, %g5			! Convert to ptab offset
	and	%g5, PTMASK, %g5
	sll	%g5, 3, %g5
	brz,pn	%g4, winfix				! NULL entry? check somewhere else
	 add	%g5, %g4, %g6
	DLFLUSH(%g6,%g5)
	ldxa	[%g6] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)
	brgez,pn %g4, winfix				! Entry invalid?  Punt
	 bset	TTE_ACCESS, %g4				! Update the modified bit
	stxa	%g4, [%g6] ASI_PHYS_CACHED		!  and write it out
	DLFLUSH(%g6, %g5)
	stx	%g1, [%g2]				! Update TSB entry tag
	stx	%g4, [%g2+8]				! Update TSB entry data
#ifdef DEBUG
	set	DATA_START, %g6	! debug
	stx	%g3, [%g6+8]	! debug
	set	0xa, %g5	! debug
	stx	%g4, [%g6]	! debug -- what we tried to enter in TLB
	stb	%g5, [%g6+0x20]	! debug
#endif

	sllx	%g3, (64-12), %g6			! Need to demap old entry first
	mov	0x010, %g1				! Secondary flush
	mov	0x020, %g5				! Nucleus flush
	movrz	%g6, %g5, %g1				! Pick one
	andn	%g3, 0xfff, %g6
	or	%g6, %g1, %g6
	stxa	%g6, [%g6] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync					! No real reason for this XXXX

	stxa	%g4, [%g0] ASI_DMMU_DATA_IN		! Enter new mapping
	membar	#Sync
	CLRTT
	retry
	NOTREACHED
	!!
	!!  Check our prom mappings -- temporary
	!!
/*
 * Handler for making the trap window shiny clean.
 *
 * If the store that trapped was to a kernel address, panic.
 *
 * If the store that trapped was to a user address, stick it in the PCB.
 * Since we don't want to force user code to use the standard register
 * convention if we don't have to, we will not assume that %fp points to
 * anything valid.
 *
 * On entry:
 *	We are on one of the alternate set of globals
 *	%g1 = %tl - 1, tstate[tl-1], scratch	- local
 *	%g2 = %tl				- local
 *	%g3 = MMU tag access			- in
 *	%g4 = %cwp				- local
 *	%g5 = scratch				- local
 *	%g6 = cpcb				- local
 *	%g7 = scratch				- local
 *
 * On return:
 *
 * NB:	 remove most of this from main codepath & cleanup I$
 */
winfault:
#ifdef DEBUG
	sethi	%hi(DATA_START), %g7			! debug
!	stx	%g0, [%g7]				! debug This is a real fault -- prevent another trap from watchdoging
	set	0x10, %g4				! debug
	stb	%g4, [%g7 + 0x20]			! debug
	CHKPT(%g4,%g7,0x19)
#endif
	mov	TLB_TAG_ACCESS, %g3			! Get real fault page from tag access register
	ldxa	[%g3] ASI_DMMU, %g3			! And put it into the non-MMU alternate regs
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
winfix:
	rdpr	%tl, %g2
	subcc	%g2, 1, %g1
	brlez,pt	%g1, datafault			! Don't go below trap level 1
	 sethi	%hi(CPCB), %g6		! get current pcb


	CHKPT(%g4,%g7,0x20)
	wrpr	%g1, 0, %tl				! Pop a trap level
	rdpr	%tt, %g7				! Read type of prev. trap
	rdpr	%tstate, %g4				! Try to restore prev %cwp if we were executing a restore
	andn	%g7, 0x3f, %g5				!   window fill traps are all 0b 0000 11xx xxxx

#if 1
	cmp	%g7, 0x68				! If we took a datafault just before this trap
	bne,pt	%icc, winfixfill			! our stack's probably bad so we need to switch somewhere else
	 nop

	!!
	!! Double data fault -- bad stack?
	!!
	set	EINTSTACK+1024-STKB, %sp		! Set the stack pointer to the middle of the idle stack
	wrpr	%g0, 15, %pil				! Disable interrupts, too
	wrpr	%g0, %g0, %canrestore			! Our stack is hozed and our PCB
	wrpr	%g0, 7, %cansave			!  probably is too, so blow away
	ta	1					!  all our register windows.
#endif

winfixfill:
	cmp	%g5, 0x0c0				!   so we mask lower bits & compare to 0b 0000 1100 0000
	bne,pt	%icc, winfixspill			! Dump our trap frame -- we will retry the fill when the page is loaded
	 cmp	%g5, 0x080				!   window spill traps are all 0b 0000 10xx xxxx

	!!
	!! This was a fill
	!!
#ifdef TRAPSTATS
	set	_C_LABEL(wfill), %g1
	lduw	[%g1], %g5
	inc	%g5
	stw	%g5, [%g1]
#endif
	btst	TSTATE_PRIV, %g4			! User mode?
	and	%g4, CWP, %g5				! %g4 = %cwp of trap
	wrpr	%g7, 0, %tt
	bz,a,pt	%icc, datafault				! We were in user mode -- normal fault
	 wrpr	%g5, %cwp				! Restore cwp from before fill trap -- regs should now be consisent

	/*
	 * We're in a pickle here.  We were trying to return to user mode
	 * and the restore of the user window failed, so now we have one valid
	 * kernel window and a user window state.  If we do a TRAP_SETUP() now,
	 * our kernel window will be considered a user window and cause a
	 * fault when we try to save it later due to an invalid user address.
	 * If we return to where we faulted, our window state will not be valid
	 * and we will fault trying to enter user with our primary context of zero.
	 *
	 * What we'll do is arrange to have us return to return_from_trap so we will
	 * start the whole business over again.  But first, switch to a kernel window
	 * setup.  Let's see, canrestore and otherwin are zero.  Set WSTATE_KERN and
	 * make sure we're in kernel context and we're done.
	 */

#ifdef TRAPSTATS
	set	_C_LABEL(kwfill), %g4
	lduw	[%g4], %g7
	inc	%g7
	stw	%g7, [%g4]
#endif
#if 0 /* Need to switch over to new stuff to fix WDR bug */
	wrpr	%g5, %cwp				! Restore cwp from before fill trap -- regs should now be consisent
	wrpr	%g2, %g0, %tl				! Restore trap level -- we need to reuse it
	set	return_from_trap, %g4
	set	CTX_PRIMARY, %g7
	wrpr	%g4, 0, %tpc
	stxa	%g0, [%g7] ASI_DMMU
	inc	4, %g4
	membar	#Sync
	flush	%g4					! Isn't this convenient?
	wrpr	%g0, WSTATE_KERN, %wstate
	wrpr	%g0, 0, %canrestore			! These should be zero but
	wrpr	%g0, 0, %otherwin			! clear them just in case
	rdpr	%ver, %g5
	and	%g5, CWP, %g5
	wrpr	%g0, 0, %cleanwin
	dec	1, %g5					! NWINDOWS-1-1
	wrpr	%g5, 0, %cansave			! Invalidate all windows
	CHKPT(%g5,%g7,0xe)
!	flushw						! DEBUG
	ba,pt	%icc, datafault
	 wrpr	%g4, 0, %tnpc
#else
	wrpr	%g2, %g0, %tl				! Restore trap level
	cmp	%g2, 3
	tne	%icc, 1
	rdpr	%tt, %g5
	wrpr	%g0, 1, %tl				! Revert to TL==1 XXX what if this wasn't in rft_user? Oh well.
	wrpr	%g5, %g0, %tt				! Set trap type correctly
	CHKPT(%g5,%g7,0xe)
/*
 * Here we need to implement the beginning of datafault.
 * TRAP_SETUP expects to come from either kernel mode or
 * user mode with at least one valid register window.  It
 * will allocate a trap frame, save the out registers, and
 * fix the window registers to think we have one user
 * register window.
 *
 * However, under these circumstances we don't have any
 * valid register windows, so we need to clean up the window
 * registers to prevent garbage from being saved to either
 * the user stack or the PCB before calling the datafault
 * handler.
 *
 * We could simply jump to datafault if we could somehow
 * make the handler issue a `saved' instruction immediately
 * after creating the trapframe.
 *
 * The fillowing is duplicated from datafault:
 */
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! We need to save volatile stuff to AG regs
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! We need to save volatile stuff to AG regs
#endif
#ifdef DEBUG
	set	DATA_START, %g7				! debug
	set	0x20, %g6				! debug
	stx	%g0, [%g7]				! debug
	stb	%g6, [%g7 + 0x20]			! debug
	CHKPT(%g4,%g7,0xf)
#endif
	wr	%g0, ASI_DMMU, %asi			! We need to re-load trap info
	ldxa	[%g0 + TLB_TAG_ACCESS] %asi, %g1	! Get fault address from tag access register
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
	ldxa	[SFAR] %asi, %g2			! sync virt addr; must be read first
	ldxa	[SFSR] %asi, %g3			! get sync fault status register
	stxa	%g0, [SFSR] %asi			! Clear out fault now
	membar	#Sync					! No real reason for this XXXX

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
	saved						! Blow away that one register window we didn't ever use.
	ba,a,pt	%icc, Ldatafault_internal		! Now we should return directly to user mode
	 nop
#endif
winfixspill:
	bne,a,pt	%xcc, datafault				! Was not a spill -- handle it normally
	 wrpr	%g2, 0, %tl				! Restore trap level for now XXXX

	!!
	!! This was a spill
	!!
#if 1
	btst	TSTATE_PRIV, %g4			! From user mode?
!	cmp	%g2, 2					! From normal execution? take a fault.
	wrpr	%g2, 0, %tl				! We need to load the fault type so we can
	rdpr	%tt, %g5				! overwrite the lower trap and get it to the fault handler
	wrpr	%g1, 0, %tl
	wrpr	%g5, 0, %tt				! Copy over trap type for the fault handler
	and	%g4, CWP, %g5				! find %cwp from trap
#ifndef TRAPTRACE
	be,a,pt	%xcc, datafault				! Let's do a regular datafault.  When we try a save in datafault we'll
	 wrpr	%g5, 0, %cwp				!  return here and write out all dirty windows.
#else
	bne,pt	%xcc, 3f				! Let's do a regular datafault.  When we try a save in datafault we'll
	 nop
	wrpr	%g5, 0, %cwp				!  return here and write out all dirty windows.
	set	trap_trace, %g2
	lduw	[%g2+TRACEDIS], %g4
	brnz,pn	%g4, 1f
	 nop
	lduw	[%g2+TRACEPTR], %g3
	rdpr	%tl, %g4
	mov	2, %g5
	set	CURPROC, %g6
	sllx	%g4, 13, %g4
!	LDPTR	[%g6], %g6	! Never touch PID
	clr	%g6		! DISABLE PID
	or	%g4, %g5, %g4
	mov	%g0, %g5
	brz,pn	%g6, 2f
	 andncc	%g3, (TRACESIZ-1), %g0
!	ldsw	[%g6+P_PID], %g5	! Load PID
2:
	movnz	%icc, %g0, %g3		! Wrap if needed
	ba,a,pt	%xcc, 4f

	set	CPCB, %g6	! Load up nsaved
	LDPTR	[%g6], %g6
	ldub	[%g6 + PCB_NSAVED], %g6
	sllx	%g6, 9, %g6
	or	%g6, %g4, %g4
4:
	rdpr	%tstate, %g6
	rdpr	%tpc, %g7
	sth	%g4, [%g2+%g3]
	inc	2, %g3
	sth	%g5, [%g2+%g3]
	inc	2, %g3
	stw	%g6, [%g2+%g3]
	inc	4, %g3
	stw	%sp, [%g2+%g3]
	inc	4, %g3
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	mov	TLB_TAG_ACCESS, %g7
	ldxa	[%g7] ASI_DMMU, %g7
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	stw	%g3, [%g2+TRACEPTR]
1:
	ba	datafault
	 nop
3:
#endif
#endif
	wrpr	%g2, 0, %tl				! Restore trap level for now XXXX
	LDPTR	[%g6 + %lo(CPCB)], %g6	! This is in the locked TLB and should not fault
#ifdef TRAPSTATS
	set	_C_LABEL(wspill), %g7
	lduw	[%g7], %g5
	inc	%g5
	stw	%g5, [%g7]
#endif
#ifdef DEBUG
	set	0x12, %g5				! debug
	sethi	%hi(DATA_START), %g7			! debug
	stb	%g5, [%g7 + 0x20]			! debug
	CHKPT(%g5,%g7,0x11)
#endif

	/*
	 * Traverse kernel map to find paddr of cpcb and only us ASI_PHYS_CACHED to
	 * prevent any faults while saving the windows.  BTW if it isn't mapped, we
	 * will trap and hopefully panic.
	 */

!	ba	0f					! DEBUG -- don't use phys addresses
	 wr	%g0, ASI_NUCLEUS, %asi			! In case of problems finding PA
	sethi	%hi(_C_LABEL(ctxbusy)), %g1
	LDPTR	[%g1 + %lo(_C_LABEL(ctxbusy))], %g1	! Load start of ctxbusy
#ifdef DEBUG
	srax	%g6, HOLESHIFT, %g7			! Check for valid address
	brz,pt	%g7, 1f					! Should be zero or -1
	 addcc	%g7, 1, %g7					! Make -1 -> 0
	tnz	%xcc, 1					! Invalid address??? How did this happen?
1:
#endif
	srlx	%g6, STSHIFT, %g7
	ldx	[%g1], %g1				! Load pointer to kernel_pmap
	and	%g7, STMASK, %g7
	sll	%g7, 3, %g7
	add	%g7, %g1, %g1
	DLFLUSH(%g1,%g7)
	ldxa	[%g1] ASI_PHYS_CACHED, %g1		! Load pointer to directory
	DLFLUSH2(%g7)

	srlx	%g6, PDSHIFT, %g7			! Do page directory
	and	%g7, PDMASK, %g7
	sll	%g7, 3, %g7
	brz,pn	%g1, 0f
	 add	%g7, %g1, %g1
	DLFLUSH(%g1,%g7)
	ldxa	[%g1] ASI_PHYS_CACHED, %g1
	DLFLUSH2(%g7)

	srlx	%g6, PTSHIFT, %g7			! Convert to ptab offset
	and	%g7, PTMASK, %g7
	brz	%g1, 0f
	 sll	%g7, 3, %g7
	add	%g1, %g7, %g7
	DLFLUSH(%g7,%g1)
	ldxa	[%g7] ASI_PHYS_CACHED, %g7		! This one is not
	DLFLUSH2(%g1)
	brgez	%g7, 0f
	 srlx	%g7, PGSHIFT, %g7			! Isolate PA part
	sll	%g6, 32-PGSHIFT, %g6			! And offset
	sllx	%g7, PGSHIFT+23, %g7			! There are 23 bits to the left of the PA in the TTE
	srl	%g6, 32-PGSHIFT, %g6
	srax	%g7, 23, %g7
	or	%g7, %g6, %g6				! Then combine them to form PA

	wr	%g0, ASI_PHYS_CACHED, %asi		! Use ASI_PHYS_CACHED to prevent possible page faults
0:
	/*
	 * Now save all user windows to cpcb.
	 */
#ifdef NOTDEF_DEBUG
	add	%g6, PCB_NSAVED, %g7
	DLFLUSH(%g7,%g5)
	lduba	[%g6 + PCB_NSAVED] %asi, %g7		! make sure that pcb_nsaved
	DLFLUSH2(%g5)
	brz,pt	%g7, 1f					! is zero, else
	 nop
	wrpr	%g0, 4, %tl
	sir						! Force a watchdog
1:
#endif
	CHKPT(%g5,%g7,0x12)
	rdpr	%otherwin, %g7
	brnz,pt	%g7, 1f
	 rdpr	%canrestore, %g5
	rdpr	%cansave, %g1
	add	%g5, 1, %g7				! add the %cwp window to the list to save
!	movrnz	%g1, %g5, %g7				! If we're issuing a save
!	mov	%g5, %g7				! DEBUG
	wrpr	%g0, 0, %canrestore
	wrpr	%g7, 0, %otherwin			! Still in user mode -- need to switch to kernel mode
1:
	mov	%g7, %g1
	CHKPT(%g5,%g7,0x13)
	add	%g6, PCB_NSAVED, %g7
	DLFLUSH(%g7,%g5)
	lduba	[%g6 + PCB_NSAVED] %asi, %g7		! Start incrementing pcb_nsaved
	DLFLUSH2(%g5)

#ifdef DEBUG
	wrpr	%g0, 5, %tl
#endif
	mov	%g6, %g5
	brz,pt	%g7, winfixsave				! If it's in use, panic
	 saved						! frob window registers

	/* PANIC */
!	CHKPT(%g4,%g7,0x10)	! Checkpoint
!	sir						! Force a watchdog
#ifdef DEBUG
	wrpr	%g2, 0, %tl
#endif
	mov	%g7, %o2
	rdpr	%ver, %o1
	sethi	%hi(2f), %o0
	and	%o1, CWP, %o1
	wrpr	%g0, %o1, %cleanwin
	dec	1, %o1
	wrpr	%g0, %o1, %cansave			! kludge away any more window problems
	wrpr	%g0, 0, %canrestore
	wrpr	%g0, 0, %otherwin
	or	%lo(2f), %o0, %o0
	wrpr	%g0, WSTATE_KERN, %wstate
#ifdef DEBUG
	set	panicstack-CC64FSZ-STKB, %sp		! Use panic stack.
#else
	set	estack0, %sp
	LDPTR	[%sp], %sp
	add	%sp, -CC64FSZ-STKB, %sp			! Overwrite proc 0's stack.
#endif
	ta	1; nop					! This helps out traptrace.
	call	_C_LABEL(panic)				! This needs to be fixed properly but we should panic here
	 mov	%g1, %o1
	NOTREACHED
	.data
2:
	.asciz	"winfault: double invalid window at %p, nsaved=%d"
	_ALIGN
	.text
3:
	saved
	save
winfixsave:
	stxa	%l0, [%g5 + PCB_RW + ( 0*8)] %asi	! Save the window in the pcb, we can schedule other stuff in here
	stxa	%l1, [%g5 + PCB_RW + ( 1*8)] %asi
	stxa	%l2, [%g5 + PCB_RW + ( 2*8)] %asi
	stxa	%l3, [%g5 + PCB_RW + ( 3*8)] %asi
	stxa	%l4, [%g5 + PCB_RW + ( 4*8)] %asi
	stxa	%l5, [%g5 + PCB_RW + ( 5*8)] %asi
	stxa	%l6, [%g5 + PCB_RW + ( 6*8)] %asi
	stxa	%l7, [%g5 + PCB_RW + ( 7*8)] %asi

	stxa	%i0, [%g5 + PCB_RW + ( 8*8)] %asi
	stxa	%i1, [%g5 + PCB_RW + ( 9*8)] %asi
	stxa	%i2, [%g5 + PCB_RW + (10*8)] %asi
	stxa	%i3, [%g5 + PCB_RW + (11*8)] %asi
	stxa	%i4, [%g5 + PCB_RW + (12*8)] %asi
	stxa	%i5, [%g5 + PCB_RW + (13*8)] %asi
	stxa	%i6, [%g5 + PCB_RW + (14*8)] %asi
	stxa	%i7, [%g5 + PCB_RW + (15*8)] %asi

!	rdpr	%otherwin, %g1	! Check to see if we's done
	dec	%g1
	wrpr	%g0, 7, %cleanwin			! BUGBUG -- we should not hardcode this, but I have no spare globals
	inc	16*8, %g5				! Move to next window
	inc	%g7					! inc pcb_nsaved
	brnz,pt	%g1, 3b
	 stxa	%o6, [%g5 + PCB_RW + (14*8)] %asi	! Save %sp so we can write these all out

	/* fix up pcb fields */
	stba	%g7, [%g6 + PCB_NSAVED] %asi		! cpcb->pcb_nsaved = n
	CHKPT(%g5,%g1,0x14)
#if 0
	mov	%g7, %g5				! fixup window registers
5:
	dec	%g5
	brgz,a,pt	%g5, 5b
	 restore
#ifdef NOT_DEBUG
	rdpr	%wstate, %g5				! DEBUG
	wrpr	%g0, WSTATE_KERN, %wstate		! DEBUG
	wrpr	%g0, 4, %tl
	rdpr	%cansave, %g7
	rdpr	%canrestore, %g6
	flushw						! DEBUG
	wrpr	%g2, 0, %tl
	wrpr	%g5, 0, %wstate				! DEBUG
#endif
#else
	/*
	 * We just issued a bunch of saves, so %cansave is now 0,
	 * probably (if we were doing a flushw then we may have
	 * come in with only partially full register windows and
	 * it may not be 0).
	 *
	 * %g7 contains the count of the windows we just finished
	 * saving.
	 *
	 * What we need to do now is move some of the windows from
	 * %canrestore to %cansave.  What we should do is take
	 * min(%canrestore, %g7) and move that over to %cansave.
	 *
	 * %g7 is the number of windows we flushed, so we should
	 * use that as a base.  Clear out %otherwin, set %cansave
	 * to min(%g7, NWINDOWS - 2), set %cleanwin to %canrestore
	 * + %cansave and the rest follows:
	 *
	 * %otherwin = 0
	 * %cansave = NWINDOWS - 2 - %canrestore
	 */
	wrpr	%g0, 0, %otherwin
	rdpr	%canrestore, %g1
	sub	%g1, %g7, %g1				! Calculate %canrestore - %g7
	movrlz	%g1, %g0, %g1				! Clamp at zero
	wrpr	%g1, 0, %canrestore			! This is the new canrestore
	rdpr	%ver, %g5
	and	%g5, CWP, %g5				! NWINDOWS-1
	dec	%g5					! NWINDOWS-2
	wrpr	%g5, 0, %cleanwin			! Set cleanwin to max, since we're in-kernel
	sub	%g5, %g1, %g5				! NWINDOWS-2-%canrestore
#ifdef xTRAPTRACE
	wrpr	%g5, 0, %cleanwin			! Force cleanwindow faults
#endif
	wrpr	%g5, 0, %cansave
#ifdef NOT_DEBUG
	rdpr	%wstate, %g5				! DEBUG
	wrpr	%g0, WSTATE_KERN, %wstate		! DEBUG
	wrpr	%g0, 4, %tl
	flushw						! DEBUG
	wrpr	%g2, 0, %tl
	wrpr	%g5, 0, %wstate				! DEBUG
#endif
#endif

#ifdef NOTDEF_DEBUG
	set	panicstack-CC64FSZ, %g1
	save	%g1, 0, %sp
	GLOBTOLOC
	rdpr	%wstate, %l0
	wrpr	%g0, WSTATE_KERN, %wstate
	set	8f, %o0
	mov	%g7, %o1
	call	printf
	 mov	%g5, %o2
	wrpr	%l0, 0, %wstate
	LOCTOGLOB
	restore
	.data
8:
	.asciz	"winfix: spill fixup\n"
	_ALIGN
	.text
#endif
	CHKPT(%g5,%g1,0x15)
!	rdpr	%tl, %g2				! DEBUG DEBUG -- did we trap somewhere?
	sub	%g2, 1, %g1
	rdpr	%tt, %g2
	wrpr	%g1, 0, %tl				! We will not attempt to re-execute the spill, so dump our trap frame permanently
	wrpr	%g2, 0, %tt				! Move trap type from fault frame here, overwriting spill
	CHKPT(%g2,%g5,0x16)

	/* Did we save a user or kernel window ? */
!	srax	%g3, 48, %g7				! User or kernel store? (TAG TARGET)
	sllx	%g3, (64-13), %g7			! User or kernel store? (TAG ACCESS)
	brnz,pt	%g7, 1f					! User fault -- save windows to pcb
	 set	(2*NBPG)-8, %g7

	and	%g4, CWP, %g4				! %g4 = %cwp of trap
	wrpr	%g4, 0, %cwp				! Kernel fault -- restore %cwp and force and trap to debugger
#ifdef DEBUG
	set	DATA_START, %g7				! debug
	set	0x11, %g6				! debug
	stb	%g6, [%g7 + 0x20]			! debug
	CHKPT(%g2,%g1,0x17)
!	sir
#endif
	!!
	!! Here we managed to fault trying to access a kernel window
	!! This is a bug.  Switch to the interrupt stack if we aren't
	!! there already and then trap into the debugger or panic.
	!!
	sethi	%hi(EINTSTACK-BIAS), %g6
	btst	1, %sp
	bnz,pt	%icc, 0f
	 mov	%sp, %g1
	add	%sp, -BIAS, %g1
0:
	or	%g6, %lo(EINTSTACK-BIAS), %g6
	set	(EINTSTACK-INTSTACK), %g7	! XXXXXXXXXX This assumes kernel addresses are unique from user addresses
	sub	%g6, %g1, %g2				! Determine if we need to switch to intr stack or not
	dec	%g7					! Make it into a mask
	andncc	%g2, %g7, %g0				! XXXXXXXXXX This assumes kernel addresses are unique from user addresses */ \
	movz	%xcc, %g1, %g6				! Stay on interrupt stack?
	add	%g6, -CCFSZ, %g6			! Allocate a stack frame
	mov	%sp, %l6				! XXXXX Save old stack pointer
	mov	%g6, %sp
	ta	1; nop					! Enter debugger
	NOTREACHED
1:
#if 1
	/* Now we need to blast away the D$ to make sure we're in sync */
	stxa	%g0, [%g7] ASI_DCACHE_TAG
	brnz,pt	%g7, 1b
	 dec	8, %g7
#endif

#ifdef DEBUG
	CHKPT(%g2,%g1,0x18)
	set	DATA_START, %g7				! debug
	set	0x19, %g6				! debug
	stb	%g6, [%g7 + 0x20]			! debug
#endif
#ifdef NOTDEF_DEBUG
	set	panicstack-CC64FSZ, %g5
	save	%g5, 0, %sp
	GLOBTOLOC
	rdpr	%wstate, %l0
	wrpr	%g0, WSTATE_KERN, %wstate
	set	8f, %o0
	call	printf
	 mov	%fp, %o1
	wrpr	%l0, 0, %wstate
	LOCTOGLOB
	restore
	.data
8:
	.asciz	"winfix: kernel spill retry\n"
	_ALIGN
	.text
#endif
#ifdef TRAPTRACE
	and	%g4, CWP, %g2	! Point our regwin at right place
	wrpr	%g2, %cwp

	set	trap_trace, %g2
	lduw	[%g2+TRACEDIS], %g4
	brnz,pn	%g4, 1f
	 nop
	lduw	[%g2+TRACEPTR], %g3
	rdpr	%tl, %g4
	mov	3, %g5
	set	CURPROC, %g6
	sllx	%g4, 13, %g4
!	LDPTR	[%g6], %g6	! Never do faultable loads
	clr	%g6		! DISABLE PID
	or	%g4, %g5, %g4
	mov	%g0, %g5
	brz,pn	%g6, 2f
	 andncc	%g3, (TRACESIZ-1), %g0
!	ldsw	[%g6+P_PID], %g5	! Load PID
2:
	movnz	%icc, %g0, %g3	! Wrap if needed

	set	CPCB, %g6	! Load up nsaved
	LDPTR	[%g6], %g6
	ldub	[%g6 + PCB_NSAVED], %g6
	sllx	%g6, 9, %g6
	or	%g6, %g4, %g4

	rdpr	%tstate, %g6
	rdpr	%tpc, %g7
	sth	%g4, [%g2+%g3]
	inc	2, %g3
	sth	%g5, [%g2+%g3]
	inc	2, %g3
	stw	%g6, [%g2+%g3]
	inc	4, %g3
	stw	%sp, [%g2+%g3]
	inc	4, %g3
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	mov	TLB_TAG_ACCESS, %g7
	ldxa	[%g7] ASI_DMMU, %g7
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	stw	%g3, [%g2+TRACEPTR]
1:
#endif
#ifdef TRAPSTATS
	set	_C_LABEL(wspillskip), %g4
	lduw	[%g4], %g5
	inc	%g5
	stw	%g5, [%g4]
#endif
	/*
	 * If we had WSTATE_KERN then we had at least one valid kernel window.
	 * We should re-execute the trapping save.
	 */
	rdpr	%wstate, %g3
	mov	%g3, %g3
	cmp	%g3, WSTATE_KERN
	bne,pt	%icc, 1f
	 nop
	retry						! Now we can complete the save
1:
	/*
	 * Since we had a WSTATE_USER, we had no valid kernel windows.  This should
	 * only happen inside TRAP_SETUP or INTR_SETUP. Emulate
	 * the instruction, clean up the register windows, then done.
	 */
	rdpr	%cwp, %g1
	inc	%g1
	rdpr	%tstate, %g2
	wrpr	%g1, %cwp
	andn	%g2, CWP, %g2
	wrpr	%g1, %g2, %tstate
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif
	mov	%g6, %sp
	done

/*
 * Each memory data access fault, from user or kernel mode,
 * comes here.
 *
 * We will assume that %pil is not lost so we won't bother to save it
 * unless we're in an interrupt handler.
 *
 * On entry:
 *	We are on one of the alternate set of globals
 *	%g1 = MMU tag target
 *	%g2 = %tl
 *
 * On return:
 *
 */
datafault:
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! We need to save volatile stuff to AG regs
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! We need to save volatile stuff to AG regs
#endif
#ifdef DEBUG
	set	DATA_START, %g7				! debug
	set	0x20, %g6				! debug
	stx	%g0, [%g7]				! debug
	stb	%g6, [%g7 + 0x20]			! debug
	CHKPT(%g4,%g7,0xf)
#endif
	wr	%g0, ASI_DMMU, %asi			! We need to re-load trap info
	ldxa	[%g0 + TLB_TAG_ACCESS] %asi, %g1	! Get fault address from tag access register
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
	ldxa	[SFAR] %asi, %g2			! sync virt addr; must be read first
	ldxa	[SFSR] %asi, %g3			! get sync fault status register
	stxa	%g0, [SFSR] %asi			! Clear out fault now
	membar	#Sync					! No real reason for this XXXX

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
Ldatafault_internal:
	INCR(_C_LABEL(uvmexp)+V_FAULTS)			! cnt.v_faults++ (clobbers %o0,%o1) should not fault
!	ldx	[%sp + CC64FSZ + STKB + TF_FAULT], %g1		! DEBUG make sure this has not changed
	mov	%g1, %o5				! Move these to the out regs so we can save the globals
	mov	%g2, %o1
	mov	%g3, %o2

	ldxa	[%g0] ASI_AFAR, %o3			! get async fault address
	ldxa	[%g0] ASI_AFSR, %o4			! get async fault status
	mov	-1, %g7
	stxa	%g7, [%g0] ASI_AFSR			! And clear this out, too
	membar	#Sync					! No real reason for this XXXX

#ifdef TRAPTRACE
	rdpr	%tt, %o0				! find out what trap brought us here
	wrpr	%g0, 0x69, %tt	! We claim to be trap type 69, not a valid trap
	TRACEME
	wrpr	%g0, PSTATE_KERN, %pstate		! Get back to normal globals

	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]	! save g1
#else
	wrpr	%g0, PSTATE_KERN, %pstate		! Get back to normal globals

	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]	! save g1
	rdpr	%tt, %o0				! find out what trap brought us here
#endif
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + (2*8)]	! save g2
	rdpr	%tstate, %g1
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + (3*8)]	! (sneak g3 in here)
	rdpr	%tpc, %g2
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + (4*8)]	! sneak in g4
	rdpr	%tnpc, %g3
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + (5*8)]	! sneak in g5
	rd	%y, %g4					! save y
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + (6*8)]	! sneak in g6
	mov	%g2, %o7				! Make the fault address look like the return address
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + (7*8)]	! sneak in g7

#ifdef DEBUG
	set	DATA_START, %g7				! debug
	set	0x21, %g6				! debug
	stb	%g6, [%g7 + 0x20]			! debug
	sth	%o0, [%sp + CC64FSZ + STKB + TF_TT]! debug
#endif
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]		! set tf.tf_psr, tf.tf_pc
	stx	%g2, [%sp + CC64FSZ + STKB + TF_PC]		! set tf.tf_npc
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]

	rdpr	%pil, %g5
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]

#if 1
	rdpr	%tl, %g7
	dec	%g7
	movrlz	%g7, %g0, %g7
	CHKPT(%g1,%g3,0x21)
	wrpr	%g0, %g7, %tl		! Revert to kernel mode
#else
	CHKPT(%g1,%g3,0x21)
	wrpr	%g0, 0, %tl		! Revert to kernel mode
#endif
	/* Finish stackframe, call C trap handler */
	flushw						! Get this clean so we won't take any more user faults
#ifdef NOTDEF_DEBUG
	set	CPCB, %o7
	LDPTR	[%o7], %o7
	ldub	[%o7 + PCB_NSAVED], %o7
	brz,pt	%o7, 2f
	 nop
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	call printf
	 mov	%i7, %o1
	ta	1; nop
	 restore
	.data
1:	.asciz	"datafault: nsaved = %d\n"
	_ALIGN
	.text
2:
#endif
	!! In the medium anywhere model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

	/* Use trap type to see what handler to call */
	cmp	%o0, T_FIMMU_MISS
	st	%g4, [%sp + CC64FSZ + STKB + TF_Y]		! set tf.tf_y
	bl	data_error
	 wrpr	%g0, PSTATE_INTR, %pstate		! reenable interrupts

	mov	%o5, %o1				! (argument:	trap address)
	mov	%g2, %o2				! (argument:	trap pc)
	call	_C_LABEL(data_access_fault)		! data_access_fault(type, addr, pc, &tf);
	 add	%sp, CC64FSZ + STKB, %o3				! (argument: &tf)

	ba	data_recover
	 nop

data_error:
	wrpr	%g0, PSTATE_INTR, %pstate		! reenable interrupts
	call	_C_LABEL(data_access_error)		! data_access_error(type, sfva, sfsr,
							!		afva, afsr, &tf);
	 add	%sp, CC64FSZ + STKB, %o5				! (argument: &tf)

data_recover:
	CHKPT(%o1,%o2,1)
	wrpr	%g0, PSTATE_KERN, %pstate		! disable interrupts
#ifdef TRAPSTATS
	set	_C_LABEL(uintrcnt), %g1
	stw	%g0, [%g1]
	set	_C_LABEL(iveccnt), %g1
	stw	%g0, [%g1]
#endif
	b	return_from_trap			! go return
	 ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1		! Load this for return_from_trap
	NOTREACHED
/*
 * Each memory instruction access fault from a fast access handler comes here.
 * We will quickly check if this is an original prom mapping before going
 * to the generic fault handler
 *
 * We will assume that %pil is not lost so we won't bother to save it
 * unless we're in an interrupt handler.
 *
 * On entry:
 *	We are on one of the alternate set of globals
 *	%g1 = MMU tag target
 *	%g2 = TSB entry ptr
 *	%g3 = TLB Tag Access
 *
 * On return:
 *
 */

	ICACHE_ALIGN
instr_miss:
#ifdef TRAPSTATS
	set	_C_LABEL(ktmiss), %g3
	set	_C_LABEL(utmiss), %g4
	rdpr	%tl, %g6
	dec	%g6
	movrz	%g6, %g4, %g3
	lduw	[%g3], %g4
	inc	%g4
	stw	%g4, [%g3]
#endif

#if 1
	mov	TLB_TAG_ACCESS, %g3			! Get real fault page
	ldxa	[%g3] ASI_IMMU, %g3			! from tag access register
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
#endif

	sethi	%hi(_C_LABEL(ctxbusy)), %g4
	sllx	%g3, (64-13), %g6			! Mask away address
	LDPTR	[%g4 + %lo(_C_LABEL(ctxbusy))], %g4
	srlx	%g6, (64-13-3), %g6			! This is now the offset into ctxbusy
	ldx	[%g4+%g6], %g4				! Load up our page table.

#ifdef DEBUG
	/* Make sure we don't try to replace a kernel translation */
	/* This should not be necessary */
	brnz,pt	%g6, Lutext_miss			! If user context continue miss
	sethi	%hi(KERNBASE), %g5			! Don't need %lo
	sethi	%hi(DATA_START), %g7
	set	0x0800000, %g6				! 8MB
	add	%g7, 16, %g7
	sub	%g3, %g5, %g5
	cmp	%g5, %g6
	mov	6, %g6		! debug
	stb	%g6, [%g7+0x20]	! debug
	tlu	%xcc, 1; nop
	blu,pn	%xcc, textfault				! Next insn in delay slot is unimportant
	 mov	7, %g6		! debug
	stb	%g6, [%g7+0x20]	! debug
#endif

	/*
	 * Try to parse our page table.
	 */
Lutext_miss:
	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	brz,pt	%g5, 0f					! Should be zero or -1
	 inc	%g5					! Make -1 -> 0
	brnz,pn	%g5, textfault				! Error!
0:
	srlx	%g3, STSHIFT, %g5
	and	%g5, STMASK, %g5
	sll	%g5, 3, %g5
	add	%g5, %g4, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)

	srlx	%g3, PDSHIFT, %g5
	and	%g5, PDMASK, %g5
	sll	%g5, 3, %g5
	brz,pn	%g4, textfault				! NULL entry? check somewhere else
	 add	%g5, %g4, %g4
	DLFLUSH(%g4,%g5)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)

	srlx	%g3, PTSHIFT, %g5			! Convert to ptab offset
	and	%g5, PTMASK, %g5
	sll	%g5, 3, %g5
	brz,pn	%g4, textfault				! NULL entry? check somewhere else
	 add	%g5, %g4, %g6
	DLFLUSH(%g6,%g5)
	ldxa	[%g6] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g5)
	brgez,pn %g4, textfault
	 bset	TTE_ACCESS, %g4				! Update accessed bit
	stxa	%g4, [%g6] ASI_PHYS_CACHED		!  and store it
	DLFLUSH(%g6,%g5)
	stx	%g1, [%g2]				! Update TSB entry tag
	stx	%g4, [%g2+8]				! Update TSB entry data
#ifdef DEBUG
	set	DATA_START, %g6	! debug
	stx	%g3, [%g6+8]	! debug
	set	0xaa, %g3	! debug
	stx	%g4, [%g6]	! debug -- what we tried to enter in TLB
	stb	%g3, [%g6+0x20]	! debug
#endif

	sllx	%g3, (64-12), %g6			! Need to demap old entry first
	mov	0x010, %g1				! Secondary flush
	mov	0x020, %g5				! Nucleus flush
	movrz	%g6, %g5, %g1				! Pick one
	andn	%g3, 0xfff, %g6
	or	%g6, %g1, %g6
	stxa	%g6, [%g6] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync					! No real reason for this XXXX

	stxa	%g4, [%g0] ASI_IMMU_DATA_IN		! Enter new mapping
	membar	#Sync
	CLRTT
	retry
	NOTREACHED
	!!
	!!  Check our prom mappings -- temporary
	!!

/*
 * Each memory text access fault, from user or kernel mode,
 * comes here.
 *
 * We will assume that %pil is not lost so we won't bother to save it
 * unless we're in an interrupt handler.
 *
 * On entry:
 *	We are on one of the alternate set of globals
 *	%g1 = MMU tag target
 *	%g2 = %tl
 *	%g3 = %tl - 1
 *
 * On return:
 *
 */


textfault:
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! We need to save volatile stuff to AG regs
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! We need to save volatile stuff to AG regs
#endif
	wr	%g0, ASI_IMMU, %asi
	ldxa	[%g0 + TLB_TAG_ACCESS] %asi, %g1	! Get fault address from tag access register
! 	nop; nop; nop		! Linux sez we need this after reading TAG_ACCESS
	ldxa	[SFSR] %asi, %g3			! get sync fault status register
	stxa	%g0, [SFSR] %asi			! Clear out old info
	membar	#Sync					! No real reason for this XXXX

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
	INCR(_C_LABEL(uvmexp)+V_FAULTS)			! cnt.v_faults++ (clobbers %o0,%o1)

	mov	%g3, %o2

	wrpr	%g0, PSTATE_KERN, %pstate		! Switch to normal globals
	ldxa	[%g0] ASI_AFSR, %o3			! get async fault status
	ldxa	[%g0] ASI_AFAR, %o4			! get async fault address
	mov	-1, %o0
	stxa	%o0, [%g0] ASI_AFSR			! Clear this out
	membar	#Sync					! No real reason for this XXXX
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]	! save g1
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + (2*8)]	! save g2
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + (3*8)]	! (sneak g3 in here)
	rdpr	%tt, %o0				! Find out what caused this trap
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + (4*8)]	! sneak in g4
	rdpr	%tstate, %g1
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + (5*8)]	! sneak in g5
	rdpr	%tpc, %o1				! sync virt addr; must be read first
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + (6*8)]	! sneak in g6
	rdpr	%tnpc, %g3
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + (7*8)]	! sneak in g7
	rd	%y, %g7					! save y

	/* Finish stackframe, call C trap handler */
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]		! set tf.tf_psr, tf.tf_pc
	sth	%o0, [%sp + CC64FSZ + STKB + TF_TT]! debug

	stx	%o1, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]		! set tf.tf_npc

	rdpr	%pil, %g5
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]

	rdpr	%tl, %g7
	dec	%g7
	movrlz	%g7, %g0, %g7
	CHKPT(%g1,%g3,0x22)
	wrpr	%g0, %g7, %tl		! Revert to kernel mode
	/* Now we need to blast away the D$ to make sure we're in sync */
	set	(2*NBPG)-8, %g7
1:
	stxa	%g0, [%g7] ASI_DCACHE_TAG
	brnz,pt	%g7, 1b
	 dec	8, %g7

	!! In the medium anywhere model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

	/* Use trap type to see what handler to call */
	flushw						! Get rid of any user windows so we don't deadlock
	cmp	%o0, T_FIMMU_MISS
	bl	text_error
	 st	%g7, [%sp + CC64FSZ + STKB + TF_Y]		! set tf.tf_y

	wrpr	%g0, PSTATE_INTR, %pstate	! reenable interrupts
	call	_C_LABEL(text_access_fault)	! mem_access_fault(type, pc, &tf);
	 add	%sp, CC64FSZ + STKB, %o2	! (argument: &tf)

	ba	text_recover
	 nop

text_error:
	wrpr	%g0, PSTATE_INTR, %pstate	! reenable interrupts
	call	_C_LABEL(text_access_error)	! mem_access_fault(type, sfva [pc], sfsr,
						!		afva, afsr, &tf);
	 add	%sp, CC64FSZ + STKB, %o5	! (argument: &tf)

text_recover:
	CHKPT(%o1,%o2,2)
	wrpr	%g0, PSTATE_KERN, %pstate	! disable interrupts
	b	return_from_trap		! go return
	 ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1	! Load this for return_from_trap
	NOTREACHED

/*
 * fp_exception has to check to see if we are trying to save
 * the FP state, and if so, continue to save the FP state.
 *
 * We do not even bother checking to see if we were in kernel mode,
 * since users have no access to the special_fp_store instruction.
 *
 * This whole idea was stolen from Sprite.
 */
fp_exception:
	rdpr	%tpc, %g1
	set	special_fp_store, %g4	! see if we came from the special one
	cmp	%g1, %g4		! pc == special_fp_store?
	bne,a	slowtrap		! no, go handle per usual
	 sethi	%hi(savefpcont), %g4	! yes, "return" to the special code
	or	%lo(savefpcont), %g4, %g4
	wrpr	%g0, %g4, %tnpc
	 done
	NOTREACHED

/*
 * slowtrap() builds a trap frame and calls trap().
 * This is called `slowtrap' because it *is*....
 * We have to build a full frame for ptrace(), for instance.
 *
 * Registers:
 *
 */
slowtrap:
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif
	/* Make sure kernel stack is aligned */
	btst	0x03, %sp		! 32-bit stack OK?
	bz,pt	%icc, 1f
	 and	%sp, 0x07, %g4		! 64-bit stack OK?
	cmp	%g4, 0x1		! Must end in 0b001
	be,pt	%icc, 1f
	 rdpr	%wstate, %g4
	cmp	%g7, WSTATE_KERN
	bnz,pt	%icc, 1f		! User stack -- we'll blow it away
	 nop
#ifdef DEBUG
	set	panicstack, %sp		! Kernel stack corrupt -- use panicstack
#else
	set	estack0, %sp
	LDPTR	[%sp], %sp
	add	%sp, -CC64FSZ-STKB, %sp	! Overwrite proc 0's stack.
#endif
1:
	rdpr	%tt, %g4
	rdpr	%tstate, %g1
	rdpr	%tpc, %g2
	rdpr	%tnpc, %g3

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
Lslowtrap_reenter:
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
	mov	%g4, %o0		! (type)
	stx	%g2, [%sp + CC64FSZ + STKB + TF_PC]
	rd	%y, %g5
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]
	mov	%g1, %o1		! (pstate)
	st	%g5, [%sp + CC64FSZ + STKB + TF_Y]
	mov	%g2, %o2		! (pc)
	sth	%o0, [%sp + CC64FSZ + STKB + TF_TT]! debug

	wrpr	%g0, PSTATE_KERN, %pstate		! Get back to normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + (2*8)]
	add	%sp, CC64FSZ + STKB, %o3		! (&tf)
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + (3*8)]
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + (4*8)]
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + (5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + (6*8)]
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + (7*8)]
	rdpr	%pil, %g5
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]
	/*
	 * Phew, ready to enable traps and call C code.
	 */
	rdpr	%tl, %g1
	dec	%g1
	movrlz	%g1, %g0, %g1
	CHKPT(%g2,%g3,0x24)
	wrpr	%g0, %g1, %tl		! Revert to kernel mode
	!! In the medium anywhere model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

!	flushw			! DEBUG
	wrpr	%g0, PSTATE_INTR, %pstate	! traps on again
	call	_C_LABEL(trap)			! trap(type, pstate, pc, &tf)
	 nop

!	wrpr	%g0, PSTATE_KERN, %pstate	! traps off again
	CHKPT(%o1,%o2,3)
	ba,a,pt	%icc, return_from_trap
	 nop
	NOTREACHED
#if 1
/*
 * This code is no longer needed.
 */
/*
 * Do a `software' trap by re-entering the trap code, possibly first
 * switching from interrupt stack to kernel stack.  This is used for
 * scheduling and signal ASTs (which generally occur from softclock or
 * tty or net interrupts).
 *
 * We enter with the trap type in %g1.  All we have to do is jump to
 * Lslowtrap_reenter above, but maybe after switching stacks....
 *
 * We should be running alternate globals.  The normal globals and
 * out registers were just loaded from the old trap frame.
 *
 *	Input Params:
 *	%g1 = tstate
 *	%g2 = tpc
 *	%g3 = tnpc
 *	%g4 = tt == T_AST
 */
softtrap:
	sethi	%hi(EINTSTACK), %g7
	or	%g7, %lo(EINTSTACK), %g7
	cmp	%g6, %g7
	bgeu,pt	%icc, Lslowtrap_reenter
	 nop
	sethi	%hi(CPCB), %g7
	LDPTR	[%g7 + %lo(CPCB)], %g7
	set	USPACE-CC64FSZ-TF_SIZE-STKB, %g5
	add	%g7, %g5, %g6
	SET_SP_REDZONE(%g7, %g5)
	stx	%g1, [%g6 + CC64FSZ + STKB + TF_FAULT]		! Generate a new trapframe
	stx	%i0, [%g6 + CC64FSZ + STKB + TF_O + (0*8)]	!	but don't bother with
	stx	%i1, [%g6 + CC64FSZ + STKB + TF_O + (1*8)]	!	locals and ins
	stx	%i2, [%g6 + CC64FSZ + STKB + TF_O + (2*8)]
	stx	%i3, [%g6 + CC64FSZ + STKB + TF_O + (3*8)]
	stx	%i4, [%g6 + CC64FSZ + STKB + TF_O + (4*8)]
	stx	%i5, [%g6 + CC64FSZ + STKB + TF_O + (5*8)]
	stx	%i6, [%g6 + CC64FSZ + STKB + TF_O + (6*8)]
	stx	%i7, [%g6 + CC64FSZ + STKB + TF_O + (7*8)]
#ifdef DEBUG
	ldx	[%sp + CC64FSZ + STKB + TF_I + (0*8)], %l0	! Copy over the rest of the regs
	ldx	[%sp + CC64FSZ + STKB + TF_I + (1*8)], %l1	! But just dirty the locals
	ldx	[%sp + CC64FSZ + STKB + TF_I + (2*8)], %l2
	ldx	[%sp + CC64FSZ + STKB + TF_I + (3*8)], %l3
	ldx	[%sp + CC64FSZ + STKB + TF_I + (4*8)], %l4
	ldx	[%sp + CC64FSZ + STKB + TF_I + (5*8)], %l5
	ldx	[%sp + CC64FSZ + STKB + TF_I + (6*8)], %l6
	ldx	[%sp + CC64FSZ + STKB + TF_I + (7*8)], %l7
	stx	%l0, [%g6 + CC64FSZ + STKB + TF_I + (0*8)]
	stx	%l1, [%g6 + CC64FSZ + STKB + TF_I + (1*8)]
	stx	%l2, [%g6 + CC64FSZ + STKB + TF_I + (2*8)]
	stx	%l3, [%g6 + CC64FSZ + STKB + TF_I + (3*8)]
	stx	%l4, [%g6 + CC64FSZ + STKB + TF_I + (4*8)]
	stx	%l5, [%g6 + CC64FSZ + STKB + TF_I + (5*8)]
	stx	%l6, [%g6 + CC64FSZ + STKB + TF_I + (6*8)]
	stx	%l7, [%g6 + CC64FSZ + STKB + TF_I + (7*8)]
	ldx	[%sp + CC64FSZ + STKB + TF_L + (0*8)], %l0
	ldx	[%sp + CC64FSZ + STKB + TF_L + (1*8)], %l1
	ldx	[%sp + CC64FSZ + STKB + TF_L + (2*8)], %l2
	ldx	[%sp + CC64FSZ + STKB + TF_L + (3*8)], %l3
	ldx	[%sp + CC64FSZ + STKB + TF_L + (4*8)], %l4
	ldx	[%sp + CC64FSZ + STKB + TF_L + (5*8)], %l5
	ldx	[%sp + CC64FSZ + STKB + TF_L + (6*8)], %l6
	ldx	[%sp + CC64FSZ + STKB + TF_L + (7*8)], %l7
	stx	%l0, [%g6 + CC64FSZ + STKB + TF_L + (0*8)]
	stx	%l1, [%g6 + CC64FSZ + STKB + TF_L + (1*8)]
	stx	%l2, [%g6 + CC64FSZ + STKB + TF_L + (2*8)]
	stx	%l3, [%g6 + CC64FSZ + STKB + TF_L + (3*8)]
	stx	%l4, [%g6 + CC64FSZ + STKB + TF_L + (4*8)]
	stx	%l5, [%g6 + CC64FSZ + STKB + TF_L + (5*8)]
	stx	%l6, [%g6 + CC64FSZ + STKB + TF_L + (6*8)]
	stx	%l7, [%g6 + CC64FSZ + STKB + TF_L + (7*8)]
#endif
	ba,pt	%xcc, Lslowtrap_reenter
	 mov	%g6, %sp
#endif

#if 0
/*
 * breakpoint:	capture as much info as possible and then call DDB
 * or trap, as the case may be.
 *
 * First, we switch to interrupt globals, and blow away %g7.  Then
 * switch down one stackframe -- just fiddle w/cwp, don't save or
 * we'll trap.  Then slowly save all the globals into our static
 * register buffer.  etc. etc.
 */

breakpoint:
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! Get IG to use
	rdpr	%cwp, %g7
	inc	1, %g7					! Equivalent of save
	wrpr	%g7, 0, %cwp				! Now we have some unused locals to fiddle with
	set	_C_LABEL(ddb_regs), %l0
	stx	%g1, [%l0+DBR_IG+(1*8)]			! Save IGs
	stx	%g2, [%l0+DBR_IG+(2*8)]
	stx	%g3, [%l0+DBR_IG+(3*8)]
	stx	%g4, [%l0+DBR_IG+(4*8)]
	stx	%g5, [%l0+DBR_IG+(5*8)]
	stx	%g6, [%l0+DBR_IG+(6*8)]
	stx	%g7, [%l0+DBR_IG+(7*8)]
	wrpr	%g0, PSTATE_KERN|PSTATE_MG, %pstate	! Get MG to use
	stx	%g1, [%l0+DBR_MG+(1*8)]			! Save MGs
	stx	%g2, [%l0+DBR_MG+(2*8)]
	stx	%g3, [%l0+DBR_MG+(3*8)]
	stx	%g4, [%l0+DBR_MG+(4*8)]
	stx	%g5, [%l0+DBR_MG+(5*8)]
	stx	%g6, [%l0+DBR_MG+(6*8)]
	stx	%g7, [%l0+DBR_MG+(7*8)]
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! Get AG to use
	stx	%g1, [%l0+DBR_AG+(1*8)]			! Save AGs
	stx	%g2, [%l0+DBR_AG+(2*8)]
	stx	%g3, [%l0+DBR_AG+(3*8)]
	stx	%g4, [%l0+DBR_AG+(4*8)]
	stx	%g5, [%l0+DBR_AG+(5*8)]
	stx	%g6, [%l0+DBR_AG+(6*8)]
	stx	%g7, [%l0+DBR_AG+(7*8)]
	wrpr	%g0, PSTATE_KERN, %pstate	! Get G to use
	stx	%g1, [%l0+DBR_G+(1*8)]			! Save Gs
	stx	%g2, [%l0+DBR_G+(2*8)]
	stx	%g3, [%l0+DBR_G+(3*8)]
	stx	%g4, [%l0+DBR_G+(4*8)]
	stx	%g5, [%l0+DBR_G+(5*8)]
	stx	%g6, [%l0+DBR_G+(6*8)]
	stx	%g7, [%l0+DBR_G+(7*8)]
	rdpr	%canrestore, %l1
	stb	%l1, [%l0+DBR_CANRESTORE]
	rdpr	%cansave, %l2
	stb	%l2, [%l0+DBR_CANSAVE]
	rdpr	%cleanwin, %l3
	stb	%l3, [%l0+DBR_CLEANWIN]
	rdpr	%wstate, %l4
	stb	%l4, [%l0+DBR_WSTATE]
	rd	%y, %l5
	stw	%l5, [%l0+DBR_Y]
	rdpr	%tl, %l6
	stb	%l6, [%l0+DBR_TL]
	dec	1, %g7
#endif

/*
 * I will not touch any of the DDB or KGDB stuff until I know what's going
 * on with the symbol table.  This is all still v7/v8 code and needs to be fixed.
 */
#ifdef KGDB
/*
 * bpt is entered on all breakpoint traps.
 * If this is a kernel breakpoint, we do not want to call trap().
 * Among other reasons, this way we can set breakpoints in trap().
 */
bpt:
	set	TSTATE_PRIV, %l4
	andcc	%l4, %l0, %g0		! breakpoint from kernel?
	bz	slowtrap		! no, go do regular trap
	 nop

	/*
	 * Build a trap frame for kgdb_trap_glue to copy.
	 * Enable traps but set ipl high so that we will not
	 * see interrupts from within breakpoints.
	 */
	save	%sp, -CCFSZ-TF_SIZE, %sp		! allocate a trap frame
	TRAP_SETUP(-CCFSZ-TF_SIZE)
	or	%l0, PSR_PIL, %l4	! splhigh()
	wr	%l4, 0, %psr		! the manual claims that this
	wr	%l4, PSR_ET, %psr	! song and dance is necessary
	std	%l0, [%sp + CCFSZ + 0]	! tf.tf_psr, tf.tf_pc
	mov	%l3, %o0		! trap type arg for kgdb_trap_glue
	rd	%y, %l3
	std	%l2, [%sp + CCFSZ + 8]	! tf.tf_npc, tf.tf_y
	rd	%wim, %l3
	st	%l3, [%sp + CCFSZ + 16]	! tf.tf_wim (a kgdb-only r/o field)
	st	%g1, [%sp + CCFSZ + 20]	! tf.tf_global[1]
	std	%g2, [%sp + CCFSZ + 24]	! etc
	std	%g4, [%sp + CCFSZ + 32]
	std	%g6, [%sp + CCFSZ + 40]
	std	%i0, [%sp + CCFSZ + 48]	! tf.tf_in[0..1]
	std	%i2, [%sp + CCFSZ + 56]	! etc
	std	%i4, [%sp + CCFSZ + 64]
	std	%i6, [%sp + CCFSZ + 72]

	/*
	 * Now call kgdb_trap_glue(); if it returns, call trap().
	 */
	mov	%o0, %l3		! gotta save trap type
	call	_C_LABEL(kgdb_trap_glue)		! kgdb_trap_glue(type, &trapframe)
	 add	%sp, CCFSZ, %o1		! (&trapframe)

	/*
	 * Use slowtrap to call trap---but first erase our tracks
	 * (put the registers back the way they were).
	 */
	mov	%l3, %o0		! slowtrap will need trap type
	ld	[%sp + CCFSZ + 12], %l3
	wr	%l3, 0, %y
	ld	[%sp + CCFSZ + 20], %g1
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	b	Lslowtrap_reenter
	 ldd	[%sp + CCFSZ + 40], %g6

/*
 * Enter kernel breakpoint.  Write all the windows (not including the
 * current window) into the stack, so that backtrace works.  Copy the
 * supplied trap frame to the kgdb stack and switch stacks.
 *
 * kgdb_trap_glue(type, tf0)
 *	int type;
 *	struct trapframe *tf0;
 */
	.globl	_C_LABEL(kgdb_trap_glue)
_C_LABEL(kgdb_trap_glue):
	save	%sp, -CCFSZ, %sp

	flushw				! flush all windows
	mov	%sp, %l4		! %l4 = current %sp

	/* copy trapframe to top of kgdb stack */
	set	_C_LABEL(kgdb_stack) + KGDB_STACK_SIZE - 80, %l0
					! %l0 = tfcopy -> end_of_kgdb_stack
	mov	80, %l1
1:	ldd	[%i1], %l2
	inc	8, %i1
	deccc	8, %l1
	std	%l2, [%l0]
	bg	1b
	 inc	8, %l0

#ifdef DEBUG
	/* save old red zone and then turn it off */
	sethi	%hi(_C_LABEL(redzone)), %l7
	ld	[%l7 + %lo(_C_LABEL(redzone))], %l6
	st	%g0, [%l7 + %lo(_C_LABEL(redzone))]
#endif
	/* switch to kgdb stack */
	add	%l0, -CCFSZ-TF_SIZE, %sp

	/* if (kgdb_trap(type, tfcopy)) kgdb_rett(tfcopy); */
	mov	%i0, %o0
	call	_C_LABEL(kgdb_trap)
	add	%l0, -80, %o1
	tst	%o0
	bnz,a	kgdb_rett
	 add	%l0, -80, %g1

	/*
	 * kgdb_trap() did not handle the trap at all so the stack is
	 * still intact.  A simple `restore' will put everything back,
	 * after we reset the stack pointer.
	 */
	mov	%l4, %sp
#ifdef DEBUG
	st	%l6, [%l7 + %lo(_C_LABEL(redzone))]	! restore red zone
#endif
	ret
	 restore

/*
 * Return from kgdb trap.  This is sort of special.
 *
 * We know that kgdb_trap_glue wrote the window above it, so that we will
 * be able to (and are sure to have to) load it up.  We also know that we
 * came from kernel land and can assume that the %fp (%i6) we load here
 * is proper.  We must also be sure not to lower ipl (it is at splhigh())
 * until we have traps disabled, due to the SPARC taking traps at the
 * new ipl before noticing that PSR_ET has been turned off.  We are on
 * the kgdb stack, so this could be disastrous.
 *
 * Note that the trapframe argument in %g1 points into the current stack
 * frame (current window).  We abandon this window when we move %g1->tf_psr
 * into %psr, but we will not have loaded the new %sp yet, so again traps
 * must be disabled.
 */
kgdb_rett:
	rd	%psr, %g4		! turn off traps
	wr	%g4, PSR_ET, %psr
	/* use the three-instruction delay to do something useful */
	ld	[%g1], %g2		! pick up new %psr
	ld	[%g1 + 12], %g3		! set %y
	wr	%g3, 0, %y
#ifdef DEBUG
	st	%l6, [%l7 + %lo(_C_LABEL(redzone))] ! and restore red zone
#endif
	wr	%g0, 0, %wim		! enable window changes
	nop; nop; nop
	/* now safe to set the new psr (changes CWP, leaves traps disabled) */
	wr	%g2, 0, %psr		! set rett psr (including cond codes)
	/* 3 instruction delay before we can use the new window */
/*1*/	ldd	[%g1 + 24], %g2		! set new %g2, %g3
/*2*/	ldd	[%g1 + 32], %g4		! set new %g4, %g5
/*3*/	ldd	[%g1 + 40], %g6		! set new %g6, %g7

	/* now we can use the new window */
	mov	%g1, %l4
	ld	[%l4 + 4], %l1		! get new pc
	ld	[%l4 + 8], %l2		! get new npc
	ld	[%l4 + 20], %g1		! set new %g1

	/* set up returnee's out registers, including its %sp */
	ldd	[%l4 + 48], %i0
	ldd	[%l4 + 56], %i2
	ldd	[%l4 + 64], %i4
	ldd	[%l4 + 72], %i6

	/* load returnee's window, making the window above it be invalid */
	restore
	restore	%g0, 1, %l1		! move to inval window and set %l1 = 1
	rd	%psr, %l0
	srl	%l1, %l0, %l1
	wr	%l1, 0, %wim		! %wim = 1 << (%psr & 31)
	sethi	%hi(CPCB), %l1
	LDPTR	[%l1 + %lo(CPCB)], %l1
	and	%l0, 31, %l0		! CWP = %psr & 31;
	st	%l0, [%l1 + PCB_WIM]	! cpcb->pcb_wim = CWP;
	save	%g0, %g0, %g0		! back to window to reload
	LOADWIN(%sp)
	save	%g0, %g0, %g0		! back to trap window
	/* note, we have not altered condition codes; safe to just rett */
	RETT
#endif

/*
 * syscall_setup() builds a trap frame and calls syscall().
 * sun_syscall is same but delivers sun system call number
 * XXX	should not have to save&reload ALL the registers just for
 *	ptrace...
 */
syscall_setup:
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif
	TRAP_SETUP(-CC64FSZ-TF_SIZE)

#ifdef DEBUG
	rdpr	%tt, %o0	! debug
	sth	%o0, [%sp + CC64FSZ + STKB + TF_TT]! debug
#endif

	wrpr	%g0, PSTATE_KERN, %pstate		! Get back to normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + ( 1*8)]
	mov	%g1, %o0				! code
	rdpr	%tpc, %o2				! (pc)
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + ( 2*8)]
	rdpr	%tstate, %g1
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + ( 3*8)]
	rdpr	%tnpc, %o3
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + ( 4*8)]
	rd	%y, %o4
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + ( 5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + ( 6*8)]
	CHKPT(%g5,%g6,0x31)
	wrpr	%g0, 0, %tl				! return to tl=0
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + ( 7*8)]
	add	%sp, CC64FSZ + STKB, %o1		! (&tf)

	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
	stx	%o2, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%o3, [%sp + CC64FSZ + STKB + TF_NPC]
	st	%o4, [%sp + CC64FSZ + STKB + TF_Y]

	rdpr	%pil, %g5
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]

	!! In the medium anywhere model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

!	flushw			! DEBUG
!	ldx	[%sp + CC64FSZ + STKB + TF_G + ( 1*8)], %o0	! (code)
	call	_C_LABEL(syscall)		! syscall(code, &tf, pc, suncompat)
	 wrpr	%g0, PSTATE_INTR, %pstate	! turn on interrupts

	/* see `proc_trampoline' for the reason for this label */
return_from_syscall:
	wrpr	%g0, PSTATE_KERN, %pstate		! Disable intterrupts
	CHKPT(%o1,%o2,0x32)
	wrpr	%g0, 0, %tl			! Return to tl==0
	CHKPT(%o1,%o2,4)
	ba,a,pt	%icc, return_from_trap
	 nop
	NOTREACHED

/*
 * interrupt_vector:
 *
 * Spitfire chips never get level interrupts directly from H/W.
 * Instead, all interrupts come in as interrupt_vector traps.
 * The interrupt number or handler address is an 11 bit number
 * encoded in the first interrupt data word.  Additional words
 * are application specific and used primarily for cross-calls.
 *
 * The interrupt vector handler then needs to identify the
 * interrupt source from the interrupt number and arrange to
 * invoke the interrupt handler.  This can either be done directly
 * from here, or a softint at a particular level can be issued.
 *
 * To call an interrupt directly and not overflow the trap stack,
 * the trap registers should be saved on the stack, registers
 * cleaned, trap-level decremented, the handler called, and then
 * the process must be reversed.
 *
 * To simplify life all we do here is issue an appropriate softint.
 *
 * Note:	It is impossible to identify or change a device's
 *		interrupt number until it is probed.  That's the
 *		purpose for all the funny interrupt acknowledge
 *		code.
 *
 */

/*
 * Vectored interrupts:
 *
 * When an interrupt comes in, interrupt_vector uses the interrupt
 * vector number to lookup the appropriate intrhand from the intrlev
 * array.  It then looks up the interrupt level from the intrhand
 * structure.  It uses the level to index the intrpending array,
 * which is 8 slots for each possible interrupt level (so we can
 * shift instead of multiply for address calculation).  It hunts for
 * any available slot at that level.  Available slots are NULL.
 *
 * NOTE: If no slots are available, we issue an un-vectored interrupt,
 * but it will probably be lost anyway.
 *
 * Then interrupt_vector uses the interrupt level in the intrhand
 * to issue a softint of the appropriate level.  The softint handler
 * figures out what level interrupt it's handling and pulls the first
 * intrhand pointer out of the intrpending array for that interrupt
 * level, puts a NULL in its place, clears the interrupt generator,
 * and invokes the interrupt handler.
 */

	.data
	.globl	intrpending
intrpending:
	.space	15 * 8 * PTRSZ

#ifdef DEBUG
#define INTRDEBUG_VECTOR	0x1
#define INTRDEBUG_LEVEL		0x2
#define INTRDEBUG_FUNC		0x4
#define INTRDEBUG_SPUR		0x8
	.globl	_C_LABEL(intrdebug)
_C_LABEL(intrdebug):	.word 0x0
#endif
	.text
interrupt_vector:
#ifdef TRAPSTATS
	set	_C_LABEL(kiveccnt), %g1
	set	_C_LABEL(iveccnt), %g2
	rdpr	%tl, %g3
	dec	%g3
	movrz	%g3, %g2, %g1
	lduw	[%g1], %g2
	inc	%g2
	stw	%g2, [%g1]
#endif
	ldxa	[%g0] ASI_IRSR, %g1
	mov	IRDR_0H, %g2
	ldxa	[%g2] ASI_IRDR, %g2	! Get interrupt number
#if NOT_DEBUG
	STACKFRAME(-CC64FSZ)		! Get a clean register window
	mov	%g1, %o1
	mov	%g2, %o2

	LOAD_ASCIZ(%o0, "interrupt_vector: ASI_IRSR %lx ASI_IRDR(0x40) %lx\r\n")
	GLOBTOLOC
	call	prom_printf
	 clr	%g4
	LOCTOGLOB
	restore
	 nop
#endif
	btst	IRSR_BUSY, %g1
	set	_C_LABEL(intrlev), %g3
	bz,pn	%icc, 3f		! spurious interrupt
	 cmp	%g2, MAXINTNUM

	stxa	%g0, [%g0] ASI_IRSR	! Ack IRQ
	membar	#Sync			! Should not be needed due to retry
#ifdef DEBUG
	tgeu	55
#endif
	bgeu	3f
	 sllx	%g2, PTRSHFT, %g5	! Calculate entry number
	add	%g3, %g5, %g5
	DLFLUSH(%g5, %g6)
	LDPTR	[%g5], %g5		! We have a pointer to the handler
	DLFLUSH2(%g6)
#ifdef DEBUG
	tst	%g5
	tz	56
#endif
	brz,pn	%g5, 3f			! NULL means it isn't registered yet.  Skip it.
	 nop
setup_sparcintr:
#ifdef	INTR_INTERLOCK
	add	%g5, IH_PEND, %g6
	DLFLUSH(%g6, %g7)
	ldstub	[%g5+IH_PEND], %g6	! Read pending flag
	DLFLUSH2(%g7)
	brnz,pn	%g6, ret_from_intr_vector ! Skip it if it's running
#endif
	 add	%g5, IH_PIL, %g6
	DLFLUSH(%g6, %g7)
	ldub	[%g5+IH_PIL], %g6	! Read interrupt mask
	DLFLUSH2(%g7)
#ifdef	VECTORED_INTERRUPTS
	set	intrpending, %g1
	mov	8, %g7			! Number of slots to search
	sll	%g6, PTRSHFT+3, %g3	! Find start of table for this IPL
	add	%g1, %g3, %g1
2:
#if 1
	DLFLUSH(%g1, %g3)
	mov	%g5, %g3
	CASPTR	[%g1] ASI_N, %g0, %g3	! Try a slot -- MPU safe
	brz,pt	%g3, 5f			! Available?
#else
	DLFLUSH(%g1, %g3)
	LDPTR	[%g1], %g3		! Try a slot
	brz,a	%g3, 5f			! Available?
	 STPTR	%g5, [%g1]		! Grab it
#endif
#ifdef DEBUG
	cmp	%g5, %g3		! if these are the same
	bne,pt	%icc, 1f		! then we aleady have the
	 nop				! interrupt registered
	set	_C_LABEL(intrdebug), %g4
	ld	[%g4], %g4
	btst	INTRDEBUG_VECTOR, %g4
	bz,pt	%icc, 1f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "interrupt_vector: duplicate handler %p\r\n")
	GLOBTOLOC
	clr	%g4
	call	prom_printf
	 mov	%g3, %o1
	LOCTOGLOB
	ba	1f
	 restore
1:
#endif
	 dec	%g7
	brgz,pt	%g7, 2b
	 inc	PTRSZ, %g1		! Next slot

	!! If we get here we have a problem.
	!! There were no available slots and the interrupt was lost.
	!! We'll resort to polling in this case.
#ifdef DIAGNOSTIC
	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "interrupt_vector: level %d out of slots\r\n")
	mov	%g6, %o1
	GLOBTOLOC
	clr	%g4
	call	prom_printf
	 rdpr	%pil, %o2
	LOCTOGLOB
	 restore
#endif
5:
#ifdef DEBUG
	set	_C_LABEL(intrdebug), %g7
	ld	[%g7], %g7
	btst	INTRDEBUG_VECTOR, %g7
	bz,pt	%icc, 1f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0,\
	    "interrupt_vector: number %lx softint mask %lx pil %lu slot %p\r\n")
	mov	%g2, %o1
	rdpr	%pil, %o3
	mov	%g1, %o4
	GLOBTOLOC
	clr	%g4
	call	prom_printf
	 mov	%g6, %o2
	LOCTOGLOB
	ba	1f
	 restore
1:
#endif
	 DLFLUSH(%g1, %g3)		! Prevent D$ pollution
#endif	/* VECTORED_INTERRUPTS */
	set	1, %g7
	sll	%g7, %g6, %g6
	wr	%g6, 0, SET_SOFTINT	! Invoke a softint

ret_from_intr_vector:
	CLRTT
	retry
	NOTREACHED

3:
#ifdef DEBUG
	set	_C_LABEL(intrdebug), %g7
	ld	[%g7], %g7
	btst	INTRDEBUG_SPUR, %g7
	bz,pt	%icc, ret_from_intr_vector
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "interrupt_vector: spurious vector %lx at pil %d\r\n")
	mov	%g2, %o1
	GLOBTOLOC
	clr	%g4
	call	prom_printf
	 rdpr	%pil, %o2
	LOCTOGLOB
	ba	ret_from_intr_vector
	 restore
#endif

/*
 * Ultra1 and Ultra2 CPUs use soft interrupts for everything.  What we do
 * on a soft interrupt, is we should check which bits in ASR_SOFTINT(0x16)
 * are set, handle those interrupts, then clear them by setting the
 * appropriate bits in ASR_CLEAR_SOFTINT(0x15).
 *
 * We have an array of 8 interrupt vector slots for each of 15 interrupt
 * levels.  If a vectored interrupt can be dispatched, the dispatch
 * routine will place a pointer to an intrhand structure in one of
 * the slots.  The interrupt handler will go through the list to look
 * for an interrupt to dispatch.  If it finds one it will pull it off
 * the list, free the entry, and call the handler.  The code is like
 * this:
 *
 *	for (i=0; i<8; i++)
 *		if (ih = intrpending[intlev][i]) {
 *			intrpending[intlev][i] = NULL;
 *			if ((*ih->ih_fun)(ih->ih_arg ? ih->ih_arg : &frame))
 *				return;
 *			strayintr(&frame);
 *			return;
 *		}
 *
 * Otherwise we go back to the old style of polled interrupts.
 *
 * After preliminary setup work, the interrupt is passed to each
 * registered handler in turn.  These are expected to return nonzero if
 * they took care of the interrupt.  If a handler claims the interrupt,
 * we exit (hardware interrupts are latched in the requestor so we'll
 * just take another interrupt in the unlikely event of simultaneous
 * interrupts from two different devices at the same level).  If we go
 * through all the registered handlers and no one claims it, we report a
 * stray interrupt.  This is more or less done as:
 *
 *	for (ih = intrhand[intlev]; ih; ih = ih->ih_next)
 *		if ((*ih->ih_fun)(ih->ih_arg ? ih->ih_arg : &frame))
 *			return;
 *	strayintr(&frame);
 *
 * Inputs:
 *	%l0 = %tstate
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l3 = interrupt level
 *	(software interrupt only) %l4 = bits to clear in interrupt register
 *
 * Internal:
 *	%l4, %l5: local variables
 *	%l6 = %y
 *	%l7 = %g1
 *	%g2..%g7 go to stack
 *
 * An interrupt frame is built in the space for a full trapframe;
 * this contains the psr, pc, npc, and interrupt level.
 *
 * The level of this interrupt is determined by:
 *
 *       IRQ# = %tt - 0x40
 */

	.comm	_C_LABEL(intrhand), 15 * PTRSZ	! intrhand[0..14]; 0 => error
	.globl _C_LABEL(sparc_interrupt)	! This is for interrupt debugging
_C_LABEL(sparc_interrupt):
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif
	/*
	 * If this is a %tick softint, clear it then call interrupt_vector.
	 */
	rd	SOFTINT, %g1
	btst	1, %g1
	bz,pt	%icc, 0f
	 set	_C_LABEL(intrlev), %g3
	wr	%g0, 1, CLEAR_SOFTINT
	DLFLUSH(%g3, %g2)
#ifndef TICK_IS_TIME
	wrpr	%g0, 0, %tick	! Reset %tick so we'll get another interrupt
#endif
	ba,pt	%icc, setup_sparcintr
	 LDPTR	[%g3 + PTRSZ], %g5	! intrlev[1] is reserved for %tick intr.
0:
#ifdef TRAPSTATS
	set	_C_LABEL(kintrcnt), %g1
	set	_C_LABEL(uintrcnt), %g2
	rdpr	%tl, %g3
	dec	%g3
	movrz	%g3, %g2, %g1
	lduw	[%g1], %g2
	inc	%g2
	stw	%g2, [%g1]
	/* See if we're on the interrupt stack already. */
	set	EINTSTACK, %g2
	set	(EINTSTACK-INTSTACK), %g1
	btst	1, %sp
	add	%sp, BIAS, %g3
	movz	%icc, %sp, %g3
	srl	%g3, 0, %g3
	sub	%g2, %g3, %g3
	cmp	%g3, %g1
	bgu	1f
	 set	_C_LABEL(intristk), %g1
	lduw	[%g1], %g2
	inc	%g2
	stw	%g2, [%g1]
1:
#endif
	INTR_SETUP(-CC64FSZ-TF_SIZE)
	! Switch to normal globals so we can save them
	wrpr	%g0, PSTATE_KERN, %pstate
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + ( 1*8)]
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + ( 2*8)]
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + ( 3*8)]
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + ( 4*8)]
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + ( 5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + ( 6*8)]
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + ( 7*8)]

	/*
	 * In the medium anywhere model %g4 points to the start of the
	 * data segment.  In our case we need to clear it before calling
	 * any C-code.
	 */
	clr	%g4

#ifdef DEBUG
	flushw			! DEBUG
#endif
	rd	%y, %l6
	INCR(_C_LABEL(uvmexp)+V_INTR)	! cnt.v_intr++; (clobbers %o0,%o1)
	rdpr	%tt, %l5		! Find out our current IPL
	rdpr	%tstate, %l0
	rdpr	%tpc, %l1
	rdpr	%tnpc, %l2
	rdpr	%tl, %l3		! Dump our trap frame now we have taken the IRQ
	stw	%l6, [%sp + CC64FSZ + STKB + TF_Y]	! Silly, but we need to save this for rft
	dec	%l3
	CHKPT(%l4,%l7,0x26)
	wrpr	%g0, %l3, %tl
	sth	%l5, [%sp + CC64FSZ + STKB + TF_TT]! debug
	stx	%l0, [%sp + CC64FSZ + STKB + TF_TSTATE]	! set up intrframe/clockframe
	stx	%l1, [%sp + CC64FSZ + STKB + TF_PC]
	btst	TSTATE_PRIV, %l0		! User mode?
	stx	%l2, [%sp + CC64FSZ + STKB + TF_NPC]
!	bnz,pt	%xcc, 1f			! No.
	 stx	%fp, [%sp + CC64FSZ + STKB + TF_KSTACK]	!  old frame pointer

!	call	_C_LABEL(blast_vcache)		! Clear out our D$ if from user mode
1:
	 sub	%l5, 0x40, %l6			! Convert to interrupt level

	set	_C_LABEL(intrcnt), %l4		! intrcnt[intlev]++;
	stb	%l6, [%sp + CC64FSZ + STKB + TF_PIL]	! set up intrframe/clockframe
	rdpr	%pil, %o1
	sll	%l6, PTRSHFT, %l3
	stb	%o1, [%sp + CC64FSZ + STKB + TF_OLDPIL]	! old %pil
	ld	[%l4 + %l3], %o0
	inc	%o0
	st	%o0, [%l4 + %l3]
	wrpr	%l6, %pil

	clr	%l5			! Zero handled count
	mov	1, %l3			! Ack softint
	sll	%l3, %l6, %l3		! Generate IRQ mask

sparc_intr_retry:
	wr	%l3, 0, CLEAR_SOFTINT	! (don't clear possible %tick IRQ)
#ifdef	VECTORED_INTERRUPTS
	set	intrpending, %l4
	wrpr	%g0, PSTATE_INTR, %pstate	! Reenable interrupts
	sll	%l6, PTRSHFT+3, %l2
	add	%l2, %l4, %l4
	mov	8, %l7
3:
!	DLFLUSH(%l4, %l2)
	LDPTR	[%l4], %l2		! Check a slot
	dec	%l7
	brnz,pt	%l2, 1f			! Pending?
	 nop
	brgz,pt	%l7, 3b
	 inc	PTRSZ, %l4		! Next slot
	ba,a,pt	%icc, 2f		! Not found -- use the old scheme
	 nop				! XXX Spitfire bug
1:
!	DLFLUSH(%l2, %o3)
	LDPTR	[%l2 + IH_CLR], %l1
	add	%sp, CC64FSZ+STKB, %o2	! tf = %sp + CC64FSZ + STKB
	LDPTR	[%l2 + IH_FUN], %o4	! ih->ih_fun
	LDPTR	[%l2 + IH_ARG], %o0	! ih->ih_arg
#ifdef DEBUG
	set	_C_LABEL(intrdebug), %o3
	ld	[%o3], %o3
	btst	INTRDEBUG_FUNC, %o3
	bz,a,pt	%icc, 0f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "sparc_interrupt:  calling %lx(%lx) sp = %p\r\n")
	mov	%i0, %o2		! arg
	mov	%i6, %o3		! sp
	GLOBTOLOC
	call	prom_printf
	 mov	%i4, %o1		! fun
	LOCTOGLOB
	restore
0:
#endif
	mov	%l4, %o1	! XXXXXXX DEBUGGGGGG!
	STPTR	%g0, [%l4]		! Clear the slot
	jmpl	%o4, %o7		! handled = (*ih->ih_fun)(...)
	 movrz	%o0, %o2, %o0		! arg = (arg == 0) ? arg : tf
	clrb	[%l2 + IH_PEND]		! Clear pending flag
#ifdef DEBUG
	set	_C_LABEL(intrdebug), %o3
	ld	[%o3], %o3
	btst	INTRDEBUG_FUNC, %o3
	bz,a,pt	%icc, 0f
	 nop
#if 0
	brnz,pt	%l1, 0f
	 nop
#endif

	mov	%l4, %o5
	mov	%l1, %o3
	STACKFRAME(-CC64FSZ)		! Get a clean register window
	mov	%i5, %o1
	mov	%i3, %o3
	LOAD_ASCIZ(%o0, "sparc_interrupt:  ih %p fun %p has %p clear\r\n")
	GLOBTOLOC
	call	prom_printf
	 mov	%i4, %o2		! fun
	LOCTOGLOB
	restore
0:
#endif
	brz,pn	%l1, 0f
	 add	%l5, %o0, %l5
	stx	%g0, [%l1]		! Clear intr source
	membar	#Sync				! Should not be needed
0:
	brnz,pt	%o0, 3b			! Handle any others
	 nop
	mov	1, %o1
	call	_C_LABEL(strayintr)	! strayintr(&intrframe, 1)
	 add	%sp, CC64FSZ + STKB, %o0
	ba,a,pt	%icc, 3b		! Try another
2:
	ba,a,pt	%icc, intrcmplt		! Only handle vectors -- don't poll XXXX
	 nop
	brnz,pt	%l5, intrcmplt		! Finish up
	 nop
#endif	/* VECTORED_INTERRUPTS */
#ifdef TRAPSTATS
	set	_C_LABEL(intrpoll), %l4
	ld	[%l4], %o0
	inc	%o0			! Increment non-vectored interrupts.
	st	%o0, [%l4]
#endif
	sll	%l6, PTRSHFT, %l3
	set	_C_LABEL(intrhand), %l4		! %l4 = intrhand[intlev];
	add	%l4, %l3, %l4
!	DLFLUSH(%l4, %o6)	! Not really needed
	LDPTR	[%l4], %l4
	wrpr	%g0, PSTATE_INTR, %pstate	! Reenable interrupts
	clr	%l3
#ifdef DEBUG
	set	trapdebug, %o2
	ld	[%o2], %o2
	btst	0x80, %o2			! (trapdebug & TDB_TL) ?
	bz	1f
	rdpr	%tl, %o2			! Trap if we're not at TL=0 since that's an error condition
	tst	%o2
	tnz	%icc, 1; nop
1:
	set	EINTSTACK, %o2
	cmp	%sp, %o2
	bleu	0f

	LOAD_ASCIZ(%o0, "sparc_interrupt:  stack %p eintstack %p\r\n")
	call	prom_printf
	 mov	%sp, %o1
	ta	1; nop
0:
	set	_C_LABEL(intrdebug), %o0	! Check intrdebug
	ld	[%o0], %o0
	btst	INTRDEBUG_LEVEL, %o0
	bz,a,pt	%icc, 3f
	 clr	%l5
	LOAD_ASCIZ(%o0, "sparc_interrupt:  got lev %ld\r\n")
	call	prom_printf
	 mov	%l6, %o1
#endif
	b	3f
	 clr	%l5
#ifdef DEBUG
	.data
	_ALIGN
	.text
#endif

1:
!	DLFLUSH(%l4, %o1)	! Should not be needed
	LDPTR	[%l4 + IH_FUN], %o1	! do {
	LDPTR	[%l4 + IH_ARG], %o0
#ifdef DEBUG
#ifndef	VECTORED_INTERRUPTS
	set	_C_LABEL(intrdebug), %o2
	ld	[%o2], %o2
	btst	INTRDEBUG_FUNC, %o2
	bz,a,pt	%icc, 7f			! Always print this
	 nop
#endif

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "sparc_interrupt(2):      calling %lx(%lx) sp = %p\r\n")
	mov	%i0, %o2		! arg
	mov	%i6, %o3		! sp
	GLOBTOLOC
	call	prom_printf
	 mov	%i1, %o1		! fun
	LOCTOGLOB
	restore
	ta	1
7:
#endif
	add	%sp, CC64FSZ + STKB, %o2
	jmpl	%o1, %o7		!	handled = (*ih->ih_fun)(...)
	 movrz	%o0, %o2, %o0
	clrb	[%l4 + IH_PEND]		! Clear the pending bit
	LDPTR	[%l4 + IH_CLR], %l3
	brz,pn	%l3, 5f			! Clear intr?
	 nop
	stx	%g0, [%l3]		! Clear intr source
	membar	#Sync				! Should not be needed
5:	brnz,pn	%o0, intrcmplt		! if (handled) break
	 LDPTR	[%l4 + IH_NEXT], %l4	!	and ih = ih->ih_next
3:	brnz,pt	%l4, 1b			! while (ih)
	 clr	%l3			! Make sure we don't have a valid pointer
	clr	%o1
	call	_C_LABEL(strayintr)	!	strayintr(&intrframe, 0)
	 add	%sp, CC64FSZ + STKB, %o0
	/* all done: restore registers and go return */
intrcmplt:
#ifdef VECTORED_INTERRUPTS
	rd	SOFTINT, %l7		! %l5 contains #intr handled.
	mov	1, %l3			! Ack softint
	sll	%l3, %l6, %l3		! Generate IRQ mask
	btst	%l3, %l7		! leave mask in %l3 for retry code
	bnz,pn	%icc, sparc_intr_retry
	 mov	1, %l5
#endif
#ifdef DEBUG
	set	_C_LABEL(intrdebug), %o2
	ld	[%o2], %o2
	btst	INTRDEBUG_FUNC, %o2
	bz,a,pt	%icc, 7f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "sparc_interrupt:  done\r\n")
	GLOBTOLOC
	call	prom_printf
	 nop
	LOCTOGLOB
	restore

7:
#endif
	ldub	[%sp + CC64FSZ + STKB + TF_OLDPIL], %l3	! restore old %pil
	wrpr	%g0, PSTATE_KERN, %pstate	! Disable interrupts
	wrpr	%l3, 0, %pil

	CHKPT(%o1,%o2,5)
	ba,a,pt	%icc, return_from_trap
	 nop

#if 0
/*
 * Level 10 %tick interrupt
 */
tickhndlr:
	mov	14, %l6
	set	_C_LABEL(intrcnt), %l4		! intrcnt[intlev]++;
	stb	%l6, [%sp + CC64FSZ + STKB + TF_PIL]	! set up intrframe/clockframe
	rdpr	%pil, %o1
	sll	%l6, PTRSHFT, %l3
	stb	%o1, [%sp + CC64FSZ + STKB + TF_OLDPIL]	! old %pil
	ld	[%l4 + %l3], %o0
	inc	%o0
	st	%o0, [%l4 + %l3]
	wr	%g0, 1, CLEAR_SOFTINT
	wrpr	%l6, %pil
	set	_C_LABEL(intrlev), %l3
	wrpr	%g0, PSTATE_INTR, %pstate	! Reenable interrupts
	LDPTR	[%l3 + PTRSZ], %l2		! %tick uses vector 1
	add	%sp, CC64FSZ+STKB, %o2	! tf = %sp + CC64FSZ + STKB
	LDPTR	[%l2 + IH_FUN], %o1	! ih->ih_fun
	LDPTR	[%l2 + IH_ARG], %o0	! ih->ih_arg
	jmpl	%o1, %o7		! handled = (*ih->ih_fun)(...)
	 movrz	%o0, %o2, %o0		! arg = (arg == 0) ? arg : tf
	clr	%l6
	ba,a,pt	%icc, intrcmplt
	 nop					! spitfire bug
#endif

#ifdef notyet
/*
 * Level 12 (ZS serial) interrupt.  Handle it quickly, schedule a
 * software interrupt, and get out.  Do the software interrupt directly
 * if we would just take it on the way out.
 *
 * Input:
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 * Internal:
 *	%l3 = zs device
 *	%l4, %l5 = temporary
 *	%l6 = rr3 (or temporary data) + 0x100 => need soft int
 *	%l7 = zs soft status
 */
zshard:
#endif /* notyet */

	.globl	return_from_trap, rft_kernel, rft_user
	.globl	softtrap, slowtrap
	.globl	syscall


/*
 * Various return-from-trap routines (see return_from_trap).
 */

/*
 * Return from trap.
 * registers are:
 *
 *	[%sp + CC64FSZ + STKB] => trap frame
 *
 * We must load all global, out, and trap registers from the trap frame.
 *
 * If returning to kernel, we should be at the proper trap level because
 * we don't touch %tl.
 *
 * When returning to user mode, the trap level does not matter, as it
 * will be set explicitly.
 *
 * If we are returning to user code, we must:
 *  1.  Check for register windows in the pcb that belong on the stack.
 *	If there are any, reload them
 */
return_from_trap:
#ifdef DEBUG
	!! Make sure we don't have pc == npc == 0 or we suck.
	ldx	[%sp + CC64FSZ + STKB + TF_PC], %g2
	ldx	[%sp + CC64FSZ + STKB + TF_NPC], %g3
	orcc	%g2, %g3, %g0
	tz	%icc, 1
#endif
#ifdef NOTDEF_DEBUG
	mov	%i6, %o1
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i1, %o1
	ldx	[%fp + CC64FSZ + STKB + TF_PC], %o3
	ldx	[%fp + CC64FSZ + STKB + TF_NPC], %o4
	GLOBTOLOC
	call	printf
	 mov	%i6, %o2
	LOCTOGLOB
	restore
	.data
1:	.asciz	"rft[%x,%x,%p,%p]"
3:	.asciz	"return_from_trap: fp=%x sp=%x pc=%x\n"
	_ALIGN
	.text
2:
#endif

#ifdef NOTDEF_DEBUG
	ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g2
	set	TSTATE_AG, %g3
	set	4f, %g4
	and	%g2, %g3, %g3
	clr	%o1
	movrnz	%g3, %g4, %o1
	set	TSTATE_MG, %g3
	set	3f, %g4
	and	%g2, %g3, %g3
	movrnz	%g3, %g4, %o1
	set	TSTATE_IG, %g3
	set	5f, %g4
	and	%g2, %g3, %g3
	movrnz	%g3, %g4, %o1
	brz,pt	%o1, 2f
	 set	1f, %o0
	call	printf
	 nop
	ta	1; nop
	.data
1:	.asciz	"Returning to trap from %s globals\n"
3:	.asciz	"MMU"
4:	.asciz	"Altermate"
5:	.asciz	"Interrupt"
	_ALIGN
	.text
2:
#endif
	!!
	!! We'll make sure we flush our pcb here, rather than later.
	!!
	ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1
	btst	TSTATE_PRIV, %g1			! returning to userland?
#if 0
	bnz,pt	%icc, 0f
	 sethi	%hi(CURPROC), %o1
	call	_C_LABEL(rwindow_save)			! Flush out our pcb
	 LDPTR	[%o1 + %lo(CURPROC)], %o0
0:
#endif
	!!
	!! Let all pending interrupts drain before returning to userland
	!!
	bnz,pn	%icc, 1f				! Returning to userland?
	 nop
	wrpr	%g0, PSTATE_INTR, %pstate
	wrpr	%g0, %g0, %pil				! Lower IPL
1:
	wrpr	%g0, PSTATE_KERN, %pstate		! Make sure we have normal globals & no IRQs

	/* Restore normal globals */
	ldx	[%sp + CC64FSZ + STKB + TF_G + (1*8)], %g1
	ldx	[%sp + CC64FSZ + STKB + TF_G + (2*8)], %g2
	ldx	[%sp + CC64FSZ + STKB + TF_G + (3*8)], %g3
	ldx	[%sp + CC64FSZ + STKB + TF_G + (4*8)], %g4
	ldx	[%sp + CC64FSZ + STKB + TF_G + (5*8)], %g5
	ldx	[%sp + CC64FSZ + STKB + TF_G + (6*8)], %g6
	ldx	[%sp + CC64FSZ + STKB + TF_G + (7*8)], %g7
	/* Switch to alternate globals and load outs */
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif
	ldx	[%sp + CC64FSZ + STKB + TF_O + (0*8)], %i0
	ldx	[%sp + CC64FSZ + STKB + TF_O + (1*8)], %i1
	ldx	[%sp + CC64FSZ + STKB + TF_O + (2*8)], %i2
	ldx	[%sp + CC64FSZ + STKB + TF_O + (3*8)], %i3
	ldx	[%sp + CC64FSZ + STKB + TF_O + (4*8)], %i4
	ldx	[%sp + CC64FSZ + STKB + TF_O + (5*8)], %i5
	ldx	[%sp + CC64FSZ + STKB + TF_O + (6*8)], %i6
	ldx	[%sp + CC64FSZ + STKB + TF_O + (7*8)], %i7
	/* Now load trap registers into alternate globals */
	ld	[%sp + CC64FSZ + STKB + TF_Y], %g4
	ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1		! load new values
	wr	%g4, 0, %y
	ldx	[%sp + CC64FSZ + STKB + TF_PC], %g2
	ldx	[%sp + CC64FSZ + STKB + TF_NPC], %g3

#ifdef DEBUG
	tst	%g2
	tz	1		! tpc NULL? Panic
	tst	%i6
	tz	1		! %fp NULL? Panic
#endif

#ifdef NOTDEF_DEBUG
	ldub	[%sp + CC64FSZ + STKB + TF_PIL], %g5		! restore %pil
	wrpr	%g5, %pil				! DEBUG
#endif

	/* Returning to user mode or kernel mode? */
	btst	TSTATE_PRIV, %g1		! returning to userland?
	CHKPT(%g4, %g7, 6)
	bz,pt	%icc, rft_user
	 sethi	%hi(_C_LABEL(want_ast)), %g7	! first instr of rft_user

/*
 * Return from trap, to kernel.
 *
 * We will assume, for the moment, that all kernel traps are properly stacked
 * in the trap registers, so all we have to do is insert the (possibly modified)
 * register values into the trap registers then do a retry.
 *
 */
rft_kernel:
	rdpr	%tl, %g4				! Grab a set of trap registers
	inc	%g4
	wrpr	%g4, %g0, %tl
	wrpr	%g3, 0, %tnpc
	wrpr	%g2, 0, %tpc
	wrpr	%g1, 0, %tstate
	CHKPT(%g1,%g2,7)
	restore
	CHKPT(%g1,%g2,0)			! Clear this out
	rdpr	%tstate, %g1			! Since we may have trapped our regs may be toast
	rdpr	%cwp, %g2
	andn	%g1, CWP, %g1
	wrpr	%g1, %g2, %tstate		! Put %cwp in %tstate
	CLRTT
#ifdef TRAPTRACE
	set	trap_trace, %g2
	lduw	[%g2+TRACEDIS], %g4
	brnz,pn	%g4, 1f
	 nop
	lduw	[%g2+TRACEPTR], %g3
	rdpr	%tl, %g4
	set	CURPROC, %g6
	sllx	%g4, 13, %g4
	LDPTR	[%g6], %g6
	clr	%g6		! DISABLE PID
	mov	%g0, %g5
	brz,pn	%g6, 2f
	 andncc	%g3, (TRACESIZ-1), %g0
!	ldsw	[%g6+P_PID], %g5	! Load PID
2:

	set	CPCB, %g6	! Load up nsaved
	LDPTR	[%g6], %g6
	ldub	[%g6 + PCB_NSAVED], %g6
	sllx	%g6, 9, %g6
	or	%g6, %g4, %g4

	movnz	%icc, %g0, %g3	! Wrap if needed
	rdpr	%tstate, %g6
	rdpr	%tpc, %g7
	sth	%g4, [%g2+%g3]
	inc	2, %g3
	sth	%g5, [%g2+%g3]
	inc	2, %g3
	stw	%g6, [%g2+%g3]
	inc	4, %g3
	stw	%sp, [%g2+%g3]
	inc	4, %g3
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	mov	TLB_TAG_ACCESS, %g7
	ldxa	[%g7] ASI_DMMU, %g7
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	stw	%g3, [%g2+TRACEPTR]
1:
#endif
#ifdef TRAPSTATS
	rdpr	%tl, %g2
	set	_C_LABEL(rftkcnt), %g1
	sllx	%g2, 2, %g2
	add	%g1, %g2, %g1
	lduw	[%g1], %g2
	inc	%g2
	stw	%g2, [%g1]
#endif
#if	0
	wrpr	%g0, 0, %cleanwin	! DEBUG
#endif
	retry					! We should allow some way to distinguish retry/done
	NOTREACHED
/*
 * Return from trap, to user.  Checks for scheduling trap (`ast') first;
 * will re-enter trap() if set.  Note that we may have to switch from
 * the interrupt stack to the kernel stack in this case.
 *	%g1 = %tstate
 *	%g2 = return %pc
 *	%g3 = return %npc
 * If returning to a valid window, just set psr and return.
 */
	.data
rft_wcnt:	.word 0
	.text

rft_user:
!	sethi	%hi(_C_LABEL(want_ast)), %g7	! (done above)
	lduw	[%g7 + %lo(_C_LABEL(want_ast))], %g7! want AST trap?
#if 0
	/* This is probably necessary */
	wrpr	%g0, 1, %tl			! Sometimes we get here w/TL=0 (How?)
	wrpr	%g3, 0, %tnpc
	wrpr	%g2, 0, %tpc
	wrpr	%g1, 0, %tstate
#endif
	brnz,pn	%g7, softtrap			! yes, re-enter trap with type T_AST
	 mov	T_AST, %g4

	CHKPT(%g4,%g7,8)
#ifdef NOTDEF_DEBUG
	sethi	%hi(CPCB), %g4
	LDPTR	[%g4 + %lo(CPCB)], %g4
	ldub	[%g4 + PCB_NSAVED], %g4		! nsaved
	brz,pt	%g4, 2f		! Only print if nsaved <> 0
	 nop
#if 0
	set	panicstack-CC64FSZ, %g7
	mov	%sp, %g6
	mov	%g7, %sp
	save	%sp, -CC64FSZ, %sp
	wrpr	%g0, WSTATE_KERN, %wstate
#endif
	set	1f, %o0
	mov	%g4, %o1
	mov	%g2, %o2			! pc
	wr	%g0, ASI_DMMU, %asi		! restore the user context
	ldxa	[CTX_SECONDARY] %asi, %o3	! ctx
	GLOBTOLOC
	mov	%g3, %o5
	call	printf
	 mov	%i6, %o4			! sp
!	wrpr	%g0, PSTATE_INTR, %pstate		! Allow IRQ service
!	wrpr	%g0, PSTATE_KERN, %pstate		! DenyIRQ service
	LOCTOGLOB
#if 0
	restore
!	wrpr	%g0, WSTATE_USER, %wstate	! Comment
	mov	%g6, %sp
	Debugger()
#endif
1:
	.data
	.asciz	"rft_user: nsaved=%x pc=%d ctx=%x sp=%x npc=%p\n"
	_ALIGN
	.text
#endif

	/*
	 * First: blast away our caches
	 */
!	call	_C_LABEL(blast_vcache)		! Clear any possible cache conflict
	/*
	 * NB: only need to do this after a cache miss
	 */
#ifdef TRAPSTATS
	set	_C_LABEL(rftucnt), %g6
	lduw	[%g6], %g7
	inc	%g7
	stw	%g7, [%g6]
#endif
	/* Here we need to undo the damage caused by switching to a kernel stack */

	rdpr	%otherwin, %g7			! restore register window controls
#ifdef DEBUG
	rdpr	%canrestore, %g5		! DEBUG
	tst	%g5				! DEBUG
	tnz	%icc, 1; nop			! DEBUG
!	mov	%g0, %g5			! There shoud be *NO* %canrestore
	add	%g7, %g5, %g7			! DEBUG
#endif
	wrpr	%g0, %g7, %canrestore
	wrpr	%g0, 0, %otherwin

	CHKPT(%g4,%g7,9)
	wrpr	%g0, WSTATE_USER, %wstate	! Need to know where our sp points

	/*
	 * Now check to see if any regs are saved in the pcb and restore them.
	 *
	 * We will use alternate globals %g4..%g7 because %g1..%g3 are used
	 * by the data fault trap handlers and we don't want possible conflict.
	 */
	sethi	%hi(CPCB), %g6
	LDPTR	[%g6 + %lo(CPCB)], %g6
	ldub	[%g6 + PCB_NSAVED], %g7			! Any saved reg windows?
#ifdef DEBUG
	set	rft_wcnt, %g4	! Keep track of all the windows we restored
	stw	%g7, [%g4]
#endif

	brz,pt	%g7, 5f					! No
	 nop
	dec	%g7					! We can do this now or later.  Move to last entry
	sll	%g7, 7, %g5				! calculate ptr into rw64 array 8*16 == 128 or 7 bits

#ifdef DEBUG
	rdpr	%canrestore, %g4			! DEBUG Make sure we've restored everything
	brnz,a,pn	%g4, 0f				! DEBUG
	 sir						! DEBUG we should NOT have any usable windows here
0:							! DEBUG
	wrpr	%g0, 5, %tl
#endif
	rdpr	%otherwin, %g4
!	wrpr	%g0, 4, %tl				! DEBUG -- don't allow *any* traps in this section
	brz,pt	%g4, 6f					! We should not have any user windows left
	 add	%g5, %g6, %g5

!	wrpr	%g0, 0, %tl				! DEBUG -- allow traps again while we panic
	set	1f, %o0
	mov	%g7, %o1
	mov	%g4, %o2
	call	printf
	 wrpr	%g0, PSTATE_KERN, %pstate
	set	2f, %o0
	call	panic
	 nop
	NOTREACHED
	.data
1:	.asciz	"pcb_nsaved=%x and otherwin=%x\n"
2:	.asciz	"rft_user\n"
	_ALIGN
	.text
6:
3:
	restored					! Load in the window
	restore						! This should not trap!
	ldx	[%g5 + PCB_RW + ( 0*8)], %l0		! Load the window from the pcb
	ldx	[%g5 + PCB_RW + ( 1*8)], %l1
	ldx	[%g5 + PCB_RW + ( 2*8)], %l2
	ldx	[%g5 + PCB_RW + ( 3*8)], %l3
	ldx	[%g5 + PCB_RW + ( 4*8)], %l4
	ldx	[%g5 + PCB_RW + ( 5*8)], %l5
	ldx	[%g5 + PCB_RW + ( 6*8)], %l6
	ldx	[%g5 + PCB_RW + ( 7*8)], %l7

	ldx	[%g5 + PCB_RW + ( 8*8)], %i0
	ldx	[%g5 + PCB_RW + ( 9*8)], %i1
	ldx	[%g5 + PCB_RW + (10*8)], %i2
	ldx	[%g5 + PCB_RW + (11*8)], %i3
	ldx	[%g5 + PCB_RW + (12*8)], %i4
	ldx	[%g5 + PCB_RW + (13*8)], %i5
	ldx	[%g5 + PCB_RW + (14*8)], %i6
	ldx	[%g5 + PCB_RW + (15*8)], %i7

#ifdef DEBUG
	stx	%g0, [%g5 + PCB_RW + (14*8)]		! DEBUG mark that we've saved this one
#endif

	cmp	%g5, %g6
	bgu,pt	%xcc, 3b				! Next one?
	 dec	8*16, %g5

	stb	%g0, [%g6 + PCB_NSAVED]			! Clear them out so we won't do this again
	rdpr	%ver, %g5
	and	%g5, CWP, %g5
	add	%g5, %g7, %g4
	dec	1, %g5					! NWINDOWS-1-1
	wrpr	%g5, 0, %cansave
	wrpr	%g0, 0, %canrestore			! Make sure we have no freeloaders XXX
	wrpr	%g0, WSTATE_USER, %wstate		! Save things to user space
	mov	%g7, %g5				! We already did one restore
4:
	rdpr	%canrestore, %g4
	inc	%g4
	deccc	%g5
	wrpr	%g4, 0, %cleanwin			! Make *sure* we don't trap to cleanwin
	bge,a,pt	%xcc, 4b				! return to starting regwin
	 save	%g0, %g0, %g0				! This may force a datafault

#ifdef DEBUG
	wrpr	%g0, 0, %tl
#endif
#ifdef TRAPSTATS
	set	_C_LABEL(rftuld), %g5
	lduw	[%g5], %g4
	inc	%g4
	stw	%g4, [%g5]
#endif
	!!
	!! We can't take any save faults in here 'cause they will never be serviced
	!!

#ifdef DEBUG
	sethi	%hi(CPCB), %g5
	LDPTR	[%g5 + %lo(CPCB)], %g5
	ldub	[%g5 + PCB_NSAVED], %g5		! Any saved reg windows?
	tst	%g5
	tnz	%icc, 1; nop			! Debugger if we still have saved windows
	bne,a	rft_user			! Try starting over again
	 sethi	%hi(_C_LABEL(want_ast)), %g7
#endif
	/*
	 * Set up our return trapframe so we can recover if we trap from here
	 * on in.
	 */
	wrpr	%g0, 1, %tl			! Set up the trap state
	wrpr	%g2, 0, %tpc
	wrpr	%g3, 0, %tnpc
	ba,pt	%icc, 6f
	 wrpr	%g1, %g0, %tstate

5:
	/*
	 * Set up our return trapframe so we can recover if we trap from here
	 * on in.
	 */
	wrpr	%g0, 1, %tl			! Set up the trap state
	wrpr	%g2, 0, %tpc
	wrpr	%g3, 0, %tnpc
	wrpr	%g1, %g0, %tstate
	restore
6:
	CHKPT(%g4,%g7,0xa)
	rdpr	%canrestore, %g5
	wrpr	%g5, 0, %cleanwin			! Force cleanup of kernel windows

#ifdef NOTDEF_DEBUG
	ldx	[%g6 + CC64FSZ + STKB + TF_L + (0*8)], %g5! DEBUG -- get proper value for %l0
	cmp	%l0, %g5
	be,a,pt %icc, 1f
	 nop
!	sir			! WATCHDOG
	set	badregs, %g1	! Save the suspect regs
	stw	%l0, [%g1+(4*0)]
	stw	%l1, [%g1+(4*1)]
	stw	%l2, [%g1+(4*2)]
	stw	%l3, [%g1+(4*3)]
	stw	%l4, [%g1+(4*4)]
	stw	%l5, [%g1+(4*5)]
	stw	%l6, [%g1+(4*6)]
	stw	%l7, [%g1+(4*7)]
	stw	%i0, [%g1+(4*8)+(4*0)]
	stw	%i1, [%g1+(4*8)+(4*1)]
	stw	%i2, [%g1+(4*8)+(4*2)]
	stw	%i3, [%g1+(4*8)+(4*3)]
	stw	%i4, [%g1+(4*8)+(4*4)]
	stw	%i5, [%g1+(4*8)+(4*5)]
	stw	%i6, [%g1+(4*8)+(4*6)]
	stw	%i7, [%g1+(4*8)+(4*7)]
	save
	inc	%g7
	wrpr	%g7, 0, %otherwin
	wrpr	%g0, 0, %canrestore
	wrpr	%g0, WSTATE_KERN, %wstate	! Need to know where our sp points
	set	rft_wcnt, %g4	! Restore nsaved before trapping
	sethi	%hi(CPCB), %g6
	LDPTR	[%g6 + %lo(CPCB)], %g6
	lduw	[%g4], %g4
	stb	%g4, [%g6 + PCB_NSAVED]
	ta	1
	sir
	.data
badregs:
	.space	16*4
	.text
1:
#endif

	rdpr	%tstate, %g1
	rdpr	%cwp, %g7			! Find our cur window
	andn	%g1, CWP, %g1			! Clear it from %tstate
	wrpr	%g1, %g7, %tstate		! Set %tstate with %cwp
	CHKPT(%g4,%g7,0xb)

	wr	%g0, ASI_DMMU, %asi		! restore the user context
	ldxa	[CTX_SECONDARY] %asi, %g4
	stxa	%g4, [CTX_PRIMARY] %asi
	sethi	%hi(KERNBASE), %g7		! Should not be needed due to retry
	membar	#Sync				! Should not be needed due to retry
	flush	%g7				! Should not be needed due to retry
	CLRTT
	CHKPT(%g4,%g7,0xd)
#ifdef TRAPTRACE
	set	trap_trace, %g2
	lduw	[%g2+TRACEDIS], %g4
	brnz,pn	%g4, 1f
	 nop
	lduw	[%g2+TRACEPTR], %g3
	rdpr	%tl, %g4
	mov	1, %g5
	set	CURPROC, %g6
	sllx	%g4, 13, %g4
	LDPTR	[%g6], %g6
!	clr	%g6		! DISABLE PID
	or	%g4, %g5, %g4
	mov	%g0, %g5
	brz,pn	%g6, 2f
	 andncc	%g3, (TRACESIZ-1), %g0
!	ldsw	[%g6+P_PID], %g5	! Load PID
2:

	set	CPCB, %g6	! Load up nsaved
	LDPTR	[%g6], %g6
	ldub	[%g6 + PCB_NSAVED], %g6
	sllx	%g6, 9, %g6
	or	%g6, %g4, %g4

	movnz	%icc, %g0, %g3	! Wrap if needed
	rdpr	%tstate, %g6
	rdpr	%tpc, %g7
	sth	%g4, [%g2+%g3]
	inc	2, %g3
	sth	%g5, [%g2+%g3]
	inc	2, %g3
	stw	%g6, [%g2+%g3]
	inc	4, %g3
	stw	%sp, [%g2+%g3]
	inc	4, %g3
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	mov	TLB_TAG_ACCESS, %g7
	ldxa	[%g7] ASI_DMMU, %g7
	stw	%g7, [%g2+%g3]
	inc	4, %g3
	stw	%g3, [%g2+TRACEPTR]
1:
#endif
#ifdef TRAPSTATS
	set	_C_LABEL(rftudone), %g1
	lduw	[%g1], %g2
	inc	%g2
	stw	%g2, [%g1]
#endif
#ifdef DEBUG
	sethi	%hi(CPCB), %g5
	LDPTR	[%g5 + %lo(CPCB)], %g5
	ldub	[%g5 + PCB_NSAVED], %g5		! Any saved reg windows?
	tst	%g5
	tnz	%icc, 1; nop			! Debugger if we still have saved windows!
#endif
	wrpr	%g0, 0, %pil			! Enable all interrupts
	retry

! exported end marker for kernel gdb
	.globl	_C_LABEL(endtrapcode)
_C_LABEL(endtrapcode):

#ifdef DDB
!!!
!!! Dump the DTLB to phys address in %o0 and print it
!!!
!!! Only toast a few %o registers
!!!
	.globl	dump_dtlb
dump_dtlb:
	clr	%o1
	add	%o1, (64*8), %o3
1:
	ldxa	[%o1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	stx	%o2, [%o0]
	membar	#Sync
	inc	8, %o0
	ldxa	[%o1] ASI_DMMU_TLB_DATA, %o4
	membar	#Sync
	inc	8, %o1
	stx	%o4, [%o0]
	cmp	%o1, %o3
	membar	#Sync
	bl	1b
	 inc	8, %o0

	retl
	 nop
#endif /* DDB */
#if defined(DDB)
	.globl	print_dtlb
print_dtlb:
#ifdef _LP64
	save	%sp, -CC64FSZ, %sp
	clr	%l1
	add	%l1, (64*8), %l3
	clr	%l2
1:
	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	mov	%l2, %o1
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o3
	membar	#Sync
	inc	%l2
	set	2f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	mov	%l2, %o1
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o3
	membar	#Sync
	inc	%l2
	set	3f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	cmp	%l1, %l3
	bl	1b
	 inc	8, %l0

	ret
	 restore
	.data
2:
	.asciz	"%2d:%016lx %016lx "
3:
	.asciz	"%2d:%016lx %016lx\r\n"
	.text
#else
	save	%sp, -CC64FSZ, %sp
	clr	%l1
	add	%l1, (64*8), %l3
	clr	%l2
1:
	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	srl	%o2, 0, %o3
	mov	%l2, %o1
	srax	%o2, 32, %o2
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o4
	membar	#Sync
	srl	%o4, 0, %o5
	inc	%l2
	srax	%o4, 32, %o4
	set	2f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	srl	%o2, 0, %o3
	mov	%l2, %o1
	srax	%o2, 32, %o2
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o4
	membar	#Sync
	srl	%o4, 0, %o5
	inc	%l2
	srax	%o4, 32, %o4
	set	3f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	cmp	%l1, %l3
	bl	1b
	 inc	8, %l0

	ret
	 restore
	.data
2:
	.asciz	"%2d:%08x:%08x %08x:%08x "
3:
	.asciz	"%2d:%08x:%08x %08x:%08x\r\n"
	.text
#endif
#endif

	.align	8
dostart:
	wrpr	%g0, 0, %tick	! XXXXXXX clear %tick register for now
	/*
	 * Startup.
	 *
	 * The Sun FCODE bootloader is nice and loads us where we want
	 * to be.  We have a full set of mappings already set up for us.
	 *
	 * I think we end up having an entire 16M allocated to us.
	 *
	 * We enter with the prom entry vector in %o0, dvec in %o1,
	 * and the bootops vector in %o2.
	 *
	 * All we need to do is:
	 *
	 *	1:	Save the prom vector
	 *
	 *	2:	Create a decent stack for ourselves
	 *
	 *	3:	Install the permanent 4MB kernel mapping
	 *
	 *	4:	Call the C language initialization code
	 *
	 */

	/*
	 * Set the psr into a known state:
	 * Set supervisor mode, interrupt level >= 13, traps enabled
	 */
	wrpr	%g0, 13, %pil
	wrpr	%g0, PSTATE_INTR|PSTATE_PEF, %pstate
	wr	%o0, FPRS_FEF, %fprs		! Turn on FPU
#ifdef DDB
	/*
	 * First, check for DDB arguments.  A pointer to an argument
	 * is passed in %o1 who's length is passed in %o2.  Our
	 * bootloader passes in a magic number as the first argument,
	 * followed by esym as argument 2, so check that %o2 == 8,
	 * then extract esym and check the magic number.
	 *
	 *  Oh, yeah, start of elf symtab is arg 3.
	 */
	cmp	%o2, 8
	blt	1f			! Not enuff args

	/*
	 * First we'll see if we were loaded by a 64-bit bootloader
	 */
	 btst	0x7, %o1		! Check alignment
	bne	0f
	 set	0x44444230, %l3

	ldx	[%o1], %l4
	cmp	%l3, %l4		! chk magic
	bne	%xcc, 0f
	 nop

	ldx	[%o1+8], %l4
	sethi	%hi(_C_LABEL(esym)), %l3	! store _esym
	STPTR	%l4, [%l3 + %lo(_C_LABEL(esym))]

	cmp	%o2, 12
	blt	1f
	 nop

	ldx	[%o1+16], %l4
	sethi	%hi(_C_LABEL(ssym)), %l3	! store _esym
	ba	1f
	 STPTR	%l4, [%l3 + %lo(_C_LABEL(ssym))]
0:
	/*
	 * Now we can try again with for a 32-bit bootloader
	 */
	cmp	%o2, 8
	blt	1f			! Not enuff args

	 set	0x44444230, %l3
	ld	[%o1], %l4
	cmp	%l3, %l4		! chk magic
	bne	1f
	 nop

	ld	[%o1+4], %l4
	sethi	%hi(_C_LABEL(esym)), %l3	! store _esym
	STPTR	%l4, [%l3 + %lo(_C_LABEL(esym))]

	cmp	%o2, 12
	blt	1f
	 nop

	ld	[%o1+8], %l4
	sethi	%hi(_C_LABEL(ssym)), %l3	! store _esym
	STPTR	%l4, [%l3 + %lo(_C_LABEL(ssym))]
1:
#endif
	/*
	 * Step 1: Save rom entry pointer
	 */

	mov	%o4, %g7	! save prom vector pointer
	set	romp, %o5
	STPTR	%o4, [%o5]	! It's initialized data, I hope

	/*
	 * Step 2: Set up a v8-like stack if we need to
	 */

#ifdef _LP64
	btst	1, %sp
	bnz,pt	%icc, 0f
	 nop
	add	%sp, -BIAS, %sp
#else
	btst	1, %sp
	bz,pt	%icc, 0f
	 nop
	add	%sp, BIAS, %sp
#endif
0:
	/*
	 * Step 3: clear BSS.  This may just be paranoia; the boot
	 * loader might already do it for us; but what the hell.
	 */
	set	_C_LABEL(edata), %o0		! bzero(edata, end - edata)
	set	_C_LABEL(end), %o1
	call	_C_LABEL(bzero)
	 sub	%o1, %o0, %o1

	/*
	 * Step 4: compute number of windows and set up tables.
	 * We could do some of this later.
	 */
	rdpr	%ver, %g1
	and	%g1, 0x0f, %g1		! want just the CWP bits
	add	%g1, 1, %o0		! compute nwindows
	sethi	%hi(_C_LABEL(nwindows)), %o1	! may as well tell everyone
	st	%o0, [%o1 + %lo(_C_LABEL(nwindows))]

#if 0
	/*
	 * Disable the DCACHE entirely for debug.
	 */
	ldxa	[%g0] ASI_MCCR, %o1
	andn	%o1, MCCR_DCACHE_EN, %o1
	stxa	%o1, [%g0] ASI_MCCR
	membar	#Sync
#endif

	/*
	 * Ready to run C code; finish bootstrap.
	 */
	set	CTX_SECONDARY, %o1		! Store -1 in the context register
	sub	%g0, 1, %o2
	stxa	%o2, [%o1] ASI_DMMU
	membar	#Sync
	ldxa	[%o1] ASI_DMMU, %o0		! then read it back
	stxa	%g0, [%o1] ASI_DMMU
	membar	#Sync
	clr	%g4				! Clear data segment pointer
	call	_C_LABEL(bootstrap)
	 inc	%o0				! and add 1 to discover maxctx

	/*
	 * pmap_bootstrap should have allocated a stack for proc 0 and
	 * stored the start and end in u0 and estack0.  Switch to that
	 * stack now.
	 */

/*
 * Initialize a CPU.  This is used both for bootstrapping the first CPU
 * and spinning up each subsequent CPU.  Basically:
 *
 *	Establish the 4MB locked mappings for kernel data and text.
 *	Locate the cpu_info structure for this CPU.
 *	Establish a locked mapping for interrupt stack.
 *	Switch to the initial stack.
 *	Call the routine passed in in cpu_info->ci_spinup
 */


_C_LABEL(cpu_initialize):
	/*
	 * Step 5: install the permanent 4MB kernel mapping in both the
	 * immu and dmmu.  We will clear out other mappings later.
	 *
	 * Register usage in this section:
	 *
	 *	%l0 = KERNBASE
	 *	%l1 = TLB Data w/o low bits
	 *	%l2 = TLB Data w/low bits
	 *	%l3 = tmp
	 *	%l4 = tmp
	 *	%l5 = tmp && TLB_TAG_ACCESS
	 *	%l6 = tmp && CTX_PRIMARY
	 *	%l7 = routine to jump to
	 *	%g1 = TLB Data for data segment w/o low bits
	 *	%g2 = TLB Data for data segment w/low bits
	 *	%g3 = DATA_START
	 */

#ifdef	NO_VCACHE
	!! Turn off D$ in LSU
	ldxa	[%g0] ASI_LSU_CONTROL_REGISTER, %g1
	bclr	MCCR_DCACHE_EN, %g1
	stxa	%g1, [%g0] ASI_LSU_CONTROL_REGISTER
	membar	#Sync
#endif

	wrpr	%g0, 0, %tl			! Make sure we're not in NUCLEUS mode
	sethi	%hi(KERNBASE), %l0		! Find our xlation
	sethi	%hi(DATA_START), %g3

	set	_C_LABEL(ktextp), %l1		! Find phys addr
	ldx	[%l1], %l1			! The following gets ugly:	We need to load the following mask
	set	_C_LABEL(kdatap), %g1
	ldx	[%g1], %g1

	sethi	%hi(0xe0000000), %l2		! V=1|SZ=11|NFO=0|IE=0
	sllx	%l2, 32, %l2			! Shift it into place

	mov	-1, %l3				! Create a nice mask
	sllx	%l3, 41, %l4			! Mask off high bits
	or	%l4, 0xfff, %l4			! We can just load this in 12 (of 13) bits

	andn	%l1, %l4, %l1			! Mask the phys page number
	andn	%o1, %l4, %o1			! Mask the phys page number

	or	%l2, %l1, %l1			! Now take care of the high bits
	or	%l2, %g1, %g1			! Now take care of the high bits
!	or	%l1, 0x076, %l1			! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=0|G=0
!	or	%g1, 0x076, %g2			! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=1(ugh)|G=0

	wrpr	%g0, PSTATE_KERN, %pstate	! Disable interrupts

!	call	print_dtlb			! Debug printf
!	 nop					! delay
#ifdef NODEF_DEBUG
	set	1f, %o0		! Debug printf
	srlx	%l0, 32, %o1
	srl	%l0, 0, %o2
	or	%l1, TTE_L|TTE_CP|TTE_CV|TTE_P|TTE_W, %l2	! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=1(ugh)|G=0
	srlx	%l2, 32, %o3
	call	_C_LABEL(prom_printf)
	 srl	%l2, 0, %o4
	.data
1:
	.asciz	"Setting DTLB entry %08x %08x data %08x %08x\r\n"
	_ALIGN
	.text
#endif
	set	0x400000, %l3			! Demap all of kernel dmmu text segment
	mov	%l0, %l5
	mov	%g3, %l6
	set	0x2000, %l4			! 8K page size
	add	%l0, %l3, %l3
0:
	stxa	%l6, [%l6] ASI_DMMU_DEMAP
	stxa	%l5, [%l5] ASI_DMMU_DEMAP	! Demap it
	add	%l6, %l4, %l6
	membar	#Sync
	cmp	%l5, %l3
	bleu	0b
	 add	%l5, %l4, %l5

	set	(1<<14)-8, %o0			! Clear out DCACHE
1:
	stxa	%g0, [%o0] ASI_DCACHE_TAG	! clear DCACHE line
	membar	#Sync
	brnz,pt	%o0, 1b
	 dec	8, %o0

	set	TLB_TAG_ACCESS, %l5		! Now map it back in with a locked TTE
#ifdef NO_VCACHE
	or	%l1, TTE_L|TTE_CP|TTE_P, %l2	! And low bits:	L=1|CP=1|CV=0(ugh)|E=0|P=1|W=0|G=0
#else
	or	%l1, TTE_L|TTE_CP|TTE_CV|TTE_P, %l2	! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=0|G=0
#endif
	set	1f, %o5
	stxa	%l0, [%l5] ASI_DMMU		! Same for DMMU
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Same for DMMU
	membar	#Sync				! We may need more membar #Sync in here
	flush	%o5				! Make IMMU see this too
#ifdef NO_VCACHE
	or	%g1, TTE_L|TTE_CP|TTE_P|TTE_W, %l2	! And low bits:	L=1|CP=1|CV=0(ugh)|E=0|P=1|W=1|G=0
#else
	or	%g1, TTE_L|TTE_CP|TTE_CV|TTE_P|TTE_W, %l2	! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=1|G=0
#endif
	stxa	%g3, [%l5] ASI_DMMU		! Same for DMMU
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Same for DMMU
	membar	#Sync				! We may need more membar #Sync in here
	flush	%o5				! Make IMMU see this too
1:
#ifdef NODEF_DEBUG
	set	1f, %o0		! Debug printf
	srlx	%l0, 32, %o1
	srl	%l0, 0, %o2
	srlx	%l2, 32, %o3
	call	_C_LABEL(prom_printf)
	 srl	%l2, 0, %o4
	.data
1:
	.asciz	"Setting ITLB entry %08x %08x data %08x %08x\r\n"
	_ALIGN
	.text
#endif
#if 1
	!!
	!! Finished the DMMU, now we need to do the IMMU which is more difficult 'cause
	!! we're execting instructions through the IMMU while we're flushing it.  We need
	!! to remap the entire kernel to a new context, flush the entire context 0 IMMU,
	!! map it back into context 0, switch to context 0, and flush context 1.
	!!
	!!
	!!  First, map in the kernel text as context==1
	!!
	set	TLB_TAG_ACCESS, %l5
	or	%l1, TTE_CP|TTE_P, %l2		! And low bits:	L=0|CP=1|CV=0|E=0|P=1|G=0
	or	%l0, 1, %l4			! Context = 1
	set	1f, %o5
	stxa	%l4, [%l5] ASI_DMMU		! Make DMMU point to it
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Store it
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l4, [%l5] ASI_IMMU		! Make IMMU point to it
	membar	#Sync				! We may need more membar #Sync in here
	flush	%l0				! Make IMMU see this too
	stxa	%l2, [%g0] ASI_IMMU_DATA_IN	! Store it
	membar	#Sync				! We may need more membar #Sync in here
	flush	%o5				! Make IMMU see this too

	or	%g3, 1, %l4			! Do the data segment, too
	or	%g1, TTE_CP|TTE_P, %l2		! And low bits:	L=0|CP=1|CV=0|E=0|P=1|G=0
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Store it
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l4, [%l5] ASI_IMMU		! Make IMMU point to it
	membar	#Sync				! We may need more membar #Sync in here
	flush	%l0				! Make IMMU see this too
	stxa	%l2, [%g0] ASI_IMMU_DATA_IN	! Store it
	membar	#Sync				! We may need more membar #Sync in here
	flush	%o5				! Make IMMU see this too
1:
	!!
	!! Now load 1 as primary context
	!!
	mov	1, %l4
	mov	CTX_PRIMARY, %l6
	set	1f, %o5
	stxa	%l4, [%l6] ASI_DMMU
	membar	#Sync				! This probably should be a flush, but it works
	flush	%o5				! This should be KERNBASE
1:

	!!
	!! Now demap entire context 0 kernel
	!!
	set	0x400000, %l3			! Demap all of kernel immu segment
	or	%l0, 0x020, %l5			! Context = Nucleus
	or	%g3, 0x020, %g5
	set	0x2000, %l4			! 8K page size
	add	%l0, %l3, %l3
0:
	stxa	%l5, [%l5] ASI_IMMU_DEMAP	! Demap it
	membar	#Sync
	stxa	%g5, [%g5] ASI_IMMU_DEMAP
	membar	#Sync
	flush	%o5				! Assume low bits are benign
	cmp	%l5, %l3
	add	%g5, %l4, %g5
	bleu	0b				! Next page
	 add	%l5, %l4, %l5

#endif
	!!
	!!  Now, map in the kernel text as context==0
	!!
	set	TLB_TAG_ACCESS, %l5
#ifdef NO_VCACHE
	or	%l1, TTE_L|TTE_CP|TTE_P, %l2	! And low bits:	L=1|CP=1|CV=0|E=0|P=1|W=0|G=0
#else
	or	%l1, TTE_L|TTE_CP|TTE_CV|TTE_P, %l2	! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=0|G=0
#endif
	set	1f, %o5
	stxa	%l0, [%l5] ASI_IMMU		! Make IMMU point to it
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l2, [%g0] ASI_IMMU_DATA_IN	! Store it
	membar	#Sync				! We may need more membar #Sync in here
	flush	%o5
1:
	!!
	!! Restore 0 as primary context
	!!
	mov	CTX_PRIMARY, %l6
	set	1f, %o5
	stxa	%g0, [%l6] ASI_DMMU
	membar	#Sync					! No real reason for this XXXX
	flush	%o5
1:
	!!
	!! Now demap context 1
	!!
	mov	1, %l4
	mov	CTX_SECONDARY, %l6
	stxa	%l4, [%l6] ASI_DMMU
	membar	#Sync				! This probably should be a flush, but it works
	flush	%l0
	set	0x030, %l4
	stxa	%l4, [%l4] ASI_DMMU_DEMAP
	membar	#Sync
	stxa	%l4, [%l4] ASI_IMMU_DEMAP
	membar	#Sync
	flush	%l0
	stxa	%g0, [%l6] ASI_DMMU
	membar	#Sync
	flush	%l0

	/*
	 * Step 6: hunt through cpus list and find the one that
	 * matches our UPAID.
	 */
	sethi	%hi(_C_LABEL(cpus)), %l1
	ldxa	[%g0] ASI_MID_REG, %l2
	LDPTR	[%l1 + %lo(_C_LABEL(cpus))], %l1
	sllx	%l2, 17, %l2			! Isolate UPAID from CPU reg
	and	%l2, 0x1f, %l2
0:
	ld	[%l1 + CI_UPAID], %l3		! Load UPAID
	cmp	%l3, %l2			! Does it match?
	bne,a,pt	%icc, 0b		! no
	 ld	[%l1 + CI_NEXT], %l1		! Load next cpu_info pointer


	/*
	 * Get pointer to our cpu_info struct
	 */

	ldx	[%l1 + CI_PADDR], %l1		! Load the interrupt stack's PA

	sethi	%hi(0xa0000000), %l2		! V=1|SZ=01|NFO=0|IE=0
	sllx	%l2, 32, %l2			! Shift it into place

	mov	-1, %l3				! Create a nice mask
	sllx	%l3, 41, %l4			! Mask off high bits
	or	%l4, 0xfff, %l4			! We can just load this in 12 (of 13) bits

	andn	%l1, %l4, %l1			! Mask the phys page number

	or	%l2, %l1, %l1			! Now take care of the high bits
#ifdef NO_VCACHE
	or	%l1, TTE_L|TTE_CP|TTE_P|TTE_W, %l2	! And low bits:	L=1|CP=1|CV=0|E=0|P=1|W=0|G=0
#else
	or	%l1, TTE_L|TTE_CP|TTE_CV|TTE_P|TTE_W, %l2	! And low bits:	L=1|CP=1|CV=1|E=0|P=1|W=0|G=0
#endif

	!!
	!!  Now, map in the interrupt stack as context==0
	!!
	set	TLB_TAG_ACCESS, %l5
	sethi	%hi(INTSTACK), %l0
	set	1f, %o5
	stxa	%l0, [%l5] ASI_DMMU		! Make DMMU point to it
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Store it
	membar	#Sync				! We may need more membar #Sync in here
	flush	%o5
1:

!!! Make sure our stack's OK.
#define SAVE	save %sp, -CC64FSZ, %sp
	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE
	restore;restore;restore;restore;restore;restore;restore;restore;restore;restore
	sethi	%hi(CPUINFO_VA+CI_INITSTACK), %l0
	LDPTR	[%l0 + %lo(CPUINFO_VA+CI_INITSTACK)], %l0
 	add	%l0, - CC64FSZ - 80, %l0	! via syscall(boot_me_up) or somesuch
#ifdef _LP64
	andn	%l0, 0x0f, %l0			! Needs to be 16-byte aligned
	sub	%l0, BIAS, %l0			! and biased
#endif
	save	%g0, %l0, %sp
!!! Make sure our stack's OK.
	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE;	SAVE
	restore;restore;restore;restore;restore;restore;restore;restore;restore;restore


	/*
	 * Step 7: change the trap base register, and install our TSB
	 *
	 * XXXX -- move this to CPUINFO_VA+32KB?
	 */
	set	_C_LABEL(tsb), %l0
	LDPTR	[%l0], %l0
	set	_C_LABEL(tsbsize), %l1
	ld	[%l1], %l1
	andn	%l0, 0xfff, %l0			! Mask off size bits
	or	%l0, %l1, %l0			! Make a TSB pointer
!	srl	%l0, 0, %l0	! DEBUG -- make sure this is a valid pointer by zeroing the high bits

#ifdef NODEF_DEBUG
	set	1f, %o0		! Debug printf
	srlx	%l0, 32, %o1
	call	_C_LABEL(prom_printf)
	 srl	%l0, 0, %o2
	.data
1:
	.asciz	"Setting TSB pointer %08x %08x\r\n"
	_ALIGN
	.text
#endif

	set	TSB, %l2
	stxa	%l0, [%l2] ASI_IMMU		! Install insn TSB pointer
	membar	#Sync				! We may need more membar #Sync in here
	stxa	%l0, [%l2] ASI_DMMU		! Install data TSB pointer
	membar	#Sync
	set	_C_LABEL(trapbase), %l1
	call	_C_LABEL(prom_set_trap_table)	! Now we should be running 100% from our handlers
	 mov	%l1, %o0
	wrpr	%l1, 0, %tba			! Make sure the PROM didn't foul up.
	wrpr	%g0, WSTATE_KERN, %wstate

#ifdef NODEF_DEBUG
	wrpr	%g0, 1, %tl			! Debug -- start at tl==3 so we'll watchdog
	wrpr	%g0, 0x1ff, %tt			! Debug -- clear out unused trap regs
	wrpr	%g0, 0, %tpc
	wrpr	%g0, 0, %tnpc
	wrpr	%g0, 0, %tstate
#endif

#ifdef DEBUG
	set	1f, %o0		! Debug printf
	srax	%l0, 32, %o1
	call	_C_LABEL(prom_printf)
	 srl	%l0, 0, %o2
	.data
1:
	.asciz	"Our trap handler is enabled\r\n"
	_ALIGN
	.text
#endif
	/*
	 * Call our startup routine.
	 */

	sethi	%hi(CPUINFO_VA+CI_SPINUP), %l0
	LDPTR	[%l0 + %lo(CPUINFO_VA+CI_SPINUP)], %o1

	call	%o1				! Call routine
	 clr	%o0				! our frame arg is ignored
	NOTREACHED

	set	1f, %o0				! Main should never come back here
	call	_C_LABEL(panic)
	 nop
	.data
1:
	.asciz	"main() returned\r\n"
	_ALIGN
	.text

/*
 * openfirmware(cell* param);
 *
 * OpenFirmware entry point
 *
 * If we're running in 32-bit mode we need to convert to a 64-bit stack
 * and 64-bit cells.  The cells we'll allocate off the stack for simplicity.
 */
	.align 8
	.globl	_C_LABEL(openfirmware)
	.proc 1
	FTYPE(openfirmware)
_C_LABEL(openfirmware):
	andcc	%sp, 1, %g0
	bz,pt	%icc, 1f

	 sethi	%hi(romp), %l7
	LDPTR	[%l7+%lo(romp)], %o4		! v9 stack, just load the addr and callit
	save	%sp, -CC64FSZ, %sp
	rdpr	%pil, %i2
	mov	PIL_IMP, %i3
	cmp	%i3, %i2
	movle	%icc, %i2, %i3
	wrpr	%g0, %i3, %pil
#if 0
!!!
!!! Since prom addresses overlap user addresses
!!! we need to clear out the dcache on the way
!!! in and out of the prom to make sure
!!! there is no prom/user data confusion
!!!
	call	_C_LABEL(blast_vcache)
	 nop
#endif
	mov	%i0, %o0
	mov	%g1, %l1
	mov	%g2, %l2
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	rdpr	%pstate, %l0
	jmpl	%i4, %o7
#if defined(_LP64) || defined(TRAPTRACE)
	 wrpr	%g0, PSTATE_PROM, %pstate
#else
	 wrpr	%g0, PSTATE_PROM|PSTATE_IE, %pstate
#endif
	wrpr	%l0, %g0, %pstate
	mov	%l1, %g1
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7
#if 0
!!!
!!! Since prom addresses overlap user addresses
!!! we need to clear out the dcache on the way
!!! in and out of the prom to make sure
!!! there is no prom/user data confusion
!!!
	call	_C_LABEL(blast_vcache)
	 nop
#endif
	wrpr	%i2, 0, %pil
	ret
	 restore	%o0, %g0, %o0

1:	! v8 -- need to screw with stack & params
#ifdef NOTDEF_DEBUG
	mov	%o7, %o5
	call	globreg_check
	 nop
	mov	%o5, %o7
#endif
	save	%sp, -CC64FSZ, %sp		! Get a new 64-bit stack frame
#if 0
	call	_C_LABEL(blast_vcache)
	 nop
#endif
	add	%sp, -BIAS, %sp
	sethi	%hi(romp), %o1
	rdpr	%pstate, %l0
	LDPTR	[%o1+%lo(romp)], %o1		! Do the actual call
	srl	%sp, 0, %sp
	rdpr	%pil, %i2	! s = splx(level)
	mov	%i0, %o0
	mov	PIL_IMP, %i3
	mov	%g1, %l1
	mov	%g2, %l2
	cmp	%i3, %i2
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	movle	%icc, %i2, %i3
	mov	%g6, %l6
	mov	%g7, %l7
	jmpl	%o1, %o7
	! Enable 64-bit addresses for the prom
#if !defined(_LP64) || defined(TRAPTRACE)
	 wrpr	%g0, PSTATE_PROM, %pstate
#else
	 wrpr	%g0, PSTATE_PROM|PSTATE_IE, %pstate
#endif
	wrpr	%l0, 0, %pstate
	wrpr	%i2, 0, %pil
#if 0
	call	_C_LABEL(blast_vcache)
	 nop
#endif
	mov	%l1, %g1
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7
	ret
	 restore	%o0, %g0, %o0

/*
 * tlb_flush_pte(vaddr_t va, int ctx)
 *
 * Flush tte from both IMMU and DMMU.
 *
 */
	.align 8
	.globl	_C_LABEL(tlb_flush_pte)
	.proc 1
	FTYPE(tlb_flush_pte)
_C_LABEL(tlb_flush_pte):
#ifdef DEBUG
	set	DATA_START, %o4				! Forget any recent TLB misses
	stx	%g0, [%o4]
	stx	%g0, [%o4+16]
#endif
#ifdef DEBUG
	set	pmapdebug, %o3
	lduw	[%o3], %o3
!	movrz	%o1, -1, %o3				! Print on either pmapdebug & PDB_DEMAP or ctx == 0
	btst	0x0020, %o3
	bz,pt	%icc, 2f
	 nop
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i1, %o1
	andn	%i0, 0xfff, %o3
	or	%o3, 0x010, %o3
	call	_C_LABEL(printf)
	 mov	%i0, %o2
	restore
	set	KERNBASE, %o4
	brz,pt	%o1, 2f					! If ctx != 0
	 cmp	%o0, %o4				! and va > KERNBASE
	tgu	1; nop					! Debugger
	.data
1:
	.asciz	"tlb_flush_pte:	demap ctx=%x va=%08x res=%x\r\n"
	_ALIGN
	.text
2:
#endif
	wr	%g0, ASI_DMMU, %asi
	ldxa	[CTX_SECONDARY] %asi, %g1		! Save secondary context
	andn	%o0, 0xfff, %g2				! drop unused va bits
	sethi	%hi(KERNBASE), %o4
	stxa	%o1, [CTX_SECONDARY] %asi		! Insert context to demap
	membar	#Sync
	or	%g2, 0x010, %g2				! Demap page from secondary context only
	stxa	%g2, [%g2] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync
	stxa	%g2, [%g2] ASI_IMMU_DEMAP		! to both TLBs
	membar	#Sync					! No real reason for this XXXX
	flush	%o4
	srl	%g2, 0, %g2				! and make sure it's both 32- and 64-bit entries
	stxa	%g2, [%g2] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync
	stxa	%g2, [%g2] ASI_IMMU_DEMAP		! Do the demap
	membar	#Sync					! No real reason for this XXXX
	flush	%o4
	stxa	%g1, [CTX_SECONDARY] %asi		! Restore secondary asi
	membar	#Sync					! No real reason for this XXXX
	flush	%o4
	retl
	 nop

/*
 * tlb_flush_ctx(int ctx)
 *
 * Flush entire context from both IMMU and DMMU.
 *
 */
	.align 8
	.globl	_C_LABEL(tlb_flush_ctx)
	.proc 1
	FTYPE(tlb_flush_ctx)
_C_LABEL(tlb_flush_ctx):
#ifdef DEBUG
	set	DATA_START, %o4				! Forget any recent TLB misses
	stx	%g0, [%o4]
#endif
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	call	printf
	 mov	%i0, %o1
	restore
	.data
1:
	.asciz	"tlb_flush_ctx:	context flush of %d attempted\r\n"
	_ALIGN
	.text
#endif
#ifdef DIAGNOSTIC
	brnz,pt	%o0, 2f
	 nop
	set	1f, %o0
	call	panic
	 nop
	.data
1:
	.asciz	"tlb_flush_ctx:	attempted demap of NUCLEUS context\r\n"
	_ALIGN
	.text
2:
#endif
	wr	%g0, ASI_DMMU, %asi
	ldxa	[CTX_SECONDARY] %asi, %g1		! Save secondary context
	sethi	%hi(KERNBASE), %o4
	stxa	%o0, [CTX_SECONDARY] %asi		! Insert context to demap
	membar	#Sync
	set	0x030, %g2				! Demap context from secondary context only
	stxa	%g2, [%g2] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync					! No real reason for this XXXX
	stxa	%g2, [%g2] ASI_IMMU_DEMAP		! Do the demap
	membar	#Sync
	stxa	%g1, [CTX_SECONDARY] %asi		! Restore secondary asi
	membar	#Sync					! No real reason for this XXXX
	flush	%o4
	retl
	 nop

/*
 * blast_vcache()
 *
 * Clear out all of both I$ and D$ regardless of contents
 * Does not modify %o0
 *
 */
	.align 8
	.globl	_C_LABEL(blast_vcache)
	.proc 1
	FTYPE(blast_vcache)
_C_LABEL(blast_vcache):
/*
 * We turn off interrupts for the duration to prevent RED exceptions.
 */
	rdpr	%pstate, %o3
	set	(2*NBPG)-8, %o1
	andn	%o3, PSTATE_IE, %o4			! Turn off PSTATE_IE bit
	wrpr	%o4, 0, %pstate
1:
	stxa	%g0, [%o1] ASI_ICACHE_TAG
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	brnz,pt	%o1, 1b
	 dec	8, %o1
	sethi	%hi(KERNBASE), %o2
	flush	%o2
	retl
	 wrpr	%o3, %pstate
/*
 * dcache_flush_page()
 *
 * Clear one page from D$.  We should do one for the I$,
 * but it does not alias and is not likely as large a problem.
 *
 */
	.align 8
	.globl	_C_LABEL(dcache_flush_page)
	.proc 1
	FTYPE(dcache_flush_page)
_C_LABEL(dcache_flush_page):
	mov	-1, %g1		! Generate mask for tag: bits [29..2]
	srlx	%o0, 13-2, %g2	! Tag is VA bits <40:13> in bits <29:2>
	srl	%g1, 2, %g1	! Now we have bits <29:0> set
	andn	%g1, 3, %g1	! Now we have bits <29:2> set

	set	(2*NBPG), %o3
	clr	%o1
1:
	ldxa	[%o1] ASI_DCACHE_TAG, %g3
	xor	%g3, %g2, %g3
	andcc	%g3, %g1, %g0
	bne,pt	%xcc, 2f
	 dec	16, %o3
	stxa	%g0, [%o1] ASI_DCACHE_TAG
2:
	brnz,pt	%o3, 1b
	 inc	16, %o1
	sethi	%hi(KERNBASE), %o5
	flush	%o5
	retl
	 nop

#ifdef _LP64
/*
 * XXXXX Still needs lotsa cleanup after sendsig is complete and offsets are known
 *
 * The following code is copied to the top of the user stack when each
 * process is exec'ed, and signals are `trampolined' off it.
 *
 * When this code is run, the stack looks like:
 *	[%sp]			128 bytes to which registers can be dumped
 *	[%sp + 128]		signal number (goes in %o0)
 *	[%sp + 128 + 4]		signal code (goes in %o1)
 *	[%sp + 128 + 8]		first word of saved state (sigcontext)
 *	    .
 *	    .
 *	    .
 *	[%sp + NNN]	last word of saved state
 * (followed by previous stack contents or top of signal stack).
 * The address of the function to call is in %g1; the old %g1 and %o0
 * have already been saved in the sigcontext.  We are running in a clean
 * window, all previous windows now being saved to the stack.
 *
 * Note that [%sp + 128 + 8] == %sp + 128 + 16.  The copy at %sp+128+8
 * will eventually be removed, with a hole left in its place, if things
 * work out.
 */
	.globl	_C_LABEL(sigcode)
	.globl	_C_LABEL(esigcode)
_C_LABEL(sigcode):
	/*
	 * XXX  the `save' and `restore' below are unnecessary: should
	 *	replace with simple arithmetic on %sp
	 *
	 * Make room on the stack for 64 %f registers + %fsr.  This comes
	 * out to 64*4+8 or 264 bytes, but this must be aligned to a multiple
	 * of 64, or 320 bytes.
	 */
	save	%sp, -CC64FSZ - 320, %sp
	mov	%g2, %l2		! save globals in %l registers
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	/*
	 * Saving the fpu registers is expensive, so do it iff it is
	 * enabled and dirty.
	 */
	rd	%fprs, %l0
	btst	FPRS_DL|FPRS_DU, %l0	! All clean?
	bz,pt	%icc, 2f
	 btst	FPRS_DL, %l0		! test dl
	bz,pt	%icc, 1f
	 btst	FPRS_DU, %l0		! test du

	! fpu is enabled, oh well
	stx	%fsr, [%sp + CC64FSZ + BIAS + 0]
	add	%sp, BIAS, %l0		! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block store
	stda	%f0, [%l0] ASI_BLK_P
	inc	BLOCK_SIZE, %l0
	stda	%f16, [%l0] ASI_BLK_P
1:
	bz,pt	%icc, 2f
	 rd	%y, %l1			! in any case, save %y
	add	%sp, BIAS, %l0		! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block store
	add	%l0, 2*BLOCK_SIZE, %l0	! and skip what we already stored
	stda	%f32, [%l0] ASI_BLK_P
	inc	BLOCK_SIZE, %l0
	stda	%f48, [%l0] ASI_BLK_P
2:
	membar	#StoreLoad
	lduw	[%fp + BIAS + CC64FSZ], %o0	! sig
	lduw	[%fp + BIAS + CC64FSZ + 4], %o1	! code
	call	%g1			! (*sa->sa_handler)(sig,code,scp)
	 add	%fp, BIAS + 128 + 8, %o2	! scp

	/*
	 * Now that the handler has returned, re-establish all the state
	 * we just saved above, then do a sigreturn.
	 */
	btst	3, %l0			! All clean?
	bz,pt	%icc, 2f
	 btst	1, %l0			! test dl
	bz,pt	%icc, 1f
	 btst	2, %l0			! test du

	ldx	[%sp + CC64FSZ + BIAS + 0], %fsr
	add	%sp, BIAS, %l0		! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block load
	ldda	[%l0] ASI_BLK_P, %f0
	inc	BLOCK_SIZE, %o0
	ldda	[%l0] ASI_BLK_P, %f16
1:
	bz,pt	%icc, 2f
	 wr	%l1, %g0, %y		! in any case, restore %y
	add	%sp, BIAS, %l0		! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block load
	inc	2*BLOCK_SIZE, %o0	! and skip what we already loaded
	ldda	[%l0] ASI_BLK_P, %f32
	inc	BLOCK_SIZE, %o0
	ldda	[%l0] ASI_BLK_P, %f48
2:
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7

	restore	%g0, SYS___sigreturn14, %g1 ! get registers back & set syscall #
	add	%sp, BIAS + 128 + 8, %o0! compute scp
!	andn	%o0, 0x0f, %o0
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
_C_LABEL(esigcode):
#endif

#if defined(COMPAT_NETBSD32) || ! defined(_LP64)
/*
 * The following code is copied to the top of the user stack when each
 * process is exec'ed, and signals are `trampolined' off it.
 *
 * When this code is run, the stack looks like:
 *	[%sp]		64 bytes to which registers can be dumped
 *	[%sp + 64]	signal number (goes in %o0)
 *	[%sp + 64 + 4]	signal code (goes in %o1)
 *	[%sp + 64 + 8]	placeholder
 *	[%sp + 64 + 12]	argument for %o3, currently unsupported (always 0)
 *	[%sp + 64 + 16]	first word of saved state (sigcontext)
 *	    .
 *	    .
 *	    .
 *	[%sp + NNN]	last word of saved state
 * (followed by previous stack contents or top of signal stack).
 * The address of the function to call is in %g1; the old %g1 and %o0
 * have already been saved in the sigcontext.  We are running in a clean
 * window, all previous windows now being saved to the stack.
 *
 * Note that [%sp + 64 + 8] == %sp + 64 + 16.  The copy at %sp+64+8
 * will eventually be removed, with a hole left in its place, if things
 * work out.
 */
#ifdef _LP64
	.globl	_C_LABEL(netbsd32_sigcode)
	.globl	_C_LABEL(netbsd32_esigcode)
_C_LABEL(netbsd32_sigcode):
#else
	.globl	_C_LABEL(sigcode)
	.globl	_C_LABEL(esigcode)
_C_LABEL(sigcode):
#endif
	/*
	 * XXX  the `save' and `restore' below are unnecessary: should
	 *	replace with simple arithmetic on %sp
	 *
	 * Make room on the stack for 32 %f registers + %fsr.  This comes
	 * out to 33*4 or 132 bytes, but this must be aligned to a multiple
	 * of 8, or 136 bytes.
	 */
	save	%sp, -CCFSZ - 136, %sp
	mov	%g2, %l2		! save globals in %l registers
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	/*
	 * Saving the fpu registers is expensive, so do it iff it is
	 * enabled and dirty.  We only deal w/lower 32 regs
	 */
	rd	%fprs, %l0
	btst	FPRS_DL, %l0		! test dl
	bz,pt	%icc, 1f
	 rd	%y, %l1			! in any case, save %y

	! fpu is enabled, oh well
	st	%fsr, [%sp + CCFSZ + 0]
	std	%f0, [%sp + CCFSZ + 8]
	std	%f2, [%sp + CCFSZ + 16]
	std	%f4, [%sp + CCFSZ + 24]
	std	%f6, [%sp + CCFSZ + 32]
	std	%f8, [%sp + CCFSZ + 40]
	std	%f10, [%sp + CCFSZ + 48]
	std	%f12, [%sp + CCFSZ + 56]
	std	%f14, [%sp + CCFSZ + 64]
	std	%f16, [%sp + CCFSZ + 72]
	std	%f18, [%sp + CCFSZ + 80]
	std	%f20, [%sp + CCFSZ + 88]
	std	%f22, [%sp + CCFSZ + 96]
	std	%f24, [%sp + CCFSZ + 104]
	std	%f26, [%sp + CCFSZ + 112]
	std	%f28, [%sp + CCFSZ + 120]
	std	%f30, [%sp + CCFSZ + 128]
1:
	ldd	[%fp + 64], %o0		! sig, code
	ld	[%fp + 76], %o3		! arg3
	call	%g1			! (*sa->sa_handler)(sig,code,scp,arg3)
	 add	%fp, 64 + 16, %o2	! scp

	/*
	 * Now that the handler has returned, re-establish all the state
	 * we just saved above, then do a sigreturn.
	 */
	btst	FPRS_DL, %l0		! test dl
	bz,pt	%icc, 1f
	 wr	%l1, %g0, %y		! in any case, restore %y

	ld	[%sp + CCFSZ + 0], %fsr
	ldd	[%sp + CCFSZ + 8], %f0
	ldd	[%sp + CCFSZ + 16], %f2
	ldd	[%sp + CCFSZ + 24], %f4
	ldd	[%sp + CCFSZ + 32], %f6
	ldd	[%sp + CCFSZ + 40], %f8
	ldd	[%sp + CCFSZ + 48], %f10
	ldd	[%sp + CCFSZ + 56], %f12
	ldd	[%sp + CCFSZ + 64], %f14
	ldd	[%sp + CCFSZ + 72], %f16
	ldd	[%sp + CCFSZ + 80], %f18
	ldd	[%sp + CCFSZ + 88], %f20
	ldd	[%sp + CCFSZ + 96], %f22
	ldd	[%sp + CCFSZ + 104], %f24
	ldd	[%sp + CCFSZ + 112], %f26
	ldd	[%sp + CCFSZ + 120], %f28
	ldd	[%sp + CCFSZ + 128], %f30

1:
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7

#ifdef _LP64
	restore	%g0, netbsd32_SYS_netbsd32___sigreturn14, %g1	! get registers back & set syscall #
	add	%sp, 64 + 16, %o0	! compute scp
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	netbsd32_SYS_netbsd32_exit, %g1		! exit(errno)
	t	ST_SYSCALL
_C_LABEL(netbsd32_esigcode):
#else
	restore	%g0, SYS___sigreturn14, %g1 ! get registers back & set syscall #
	add	%sp, 64 + 16, %o0	! compute scp
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
_C_LABEL(esigcode):
#endif
#endif

#ifdef COMPAT_SUNOS
/*
 * This code is still 32-bit only.
 */
/*
 * The following code is copied to the top of the user stack when each
 * process is exec'ed, and signals are `trampolined' off it.
 *
 * When this code is run, the stack looks like:
 *	[%sp]		64 bytes to which registers can be dumped
 *	[%sp + 64]	signal number (goes in %o0)
 *	[%sp + 64 + 4]	signal code (goes in %o1)
 *	[%sp + 64 + 8]	placeholder
 *	[%sp + 64 + 12]	argument for %o3, currently unsupported (always 0)
 *	[%sp + 64 + 16]	first word of saved state (sigcontext)
 *	    .
 *	    .
 *	    .
 *	[%sp + NNN]	last word of saved state
 * (followed by previous stack contents or top of signal stack).
 * The address of the function to call is in %g1; the old %g1 and %o0
 * have already been saved in the sigcontext.  We are running in a clean
 * window, all previous windows now being saved to the stack.
 *
 * Note that [%sp + 64 + 8] == %sp + 64 + 16.  The copy at %sp+64+8
 * will eventually be removed, with a hole left in its place, if things
 * work out.
 */
	.globl	_C_LABEL(sunos_sigcode)
	.globl	_C_LABEL(sunos_esigcode)
_C_LABEL(sunos_sigcode):
	/*
	 * XXX  the `save' and `restore' below are unnecessary: should
	 *	replace with simple arithmetic on %sp
	 *
	 * Make room on the stack for 32 %f registers + %fsr.  This comes
	 * out to 33*4 or 132 bytes, but this must be aligned to a multiple
	 * of 8, or 136 bytes.
	 */
	save	%sp, -CCFSZ - 136, %sp
	mov	%g2, %l2		! save globals in %l registers
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	/*
	 * Saving the fpu registers is expensive, so do it iff the fsr
	 * stored in the sigcontext shows that the fpu is enabled.
	 */
	ld	[%fp + 64 + 16 + SC_PSR_OFFSET], %l0
	sethi	%hi(PSR_EF), %l1	! FPU enable bit is too high for andcc
	andcc	%l0, %l1, %l0		! %l0 = fpu enable bit
	be	1f			! if not set, skip the saves
	 rd	%y, %l1			! in any case, save %y

	! fpu is enabled, oh well
	st	%fsr, [%sp + CCFSZ + 0]
	std	%f0, [%sp + CCFSZ + 8]
	std	%f2, [%sp + CCFSZ + 16]
	std	%f4, [%sp + CCFSZ + 24]
	std	%f6, [%sp + CCFSZ + 32]
	std	%f8, [%sp + CCFSZ + 40]
	std	%f10, [%sp + CCFSZ + 48]
	std	%f12, [%sp + CCFSZ + 56]
	std	%f14, [%sp + CCFSZ + 64]
	std	%f16, [%sp + CCFSZ + 72]
	std	%f18, [%sp + CCFSZ + 80]
	std	%f20, [%sp + CCFSZ + 88]
	std	%f22, [%sp + CCFSZ + 96]
	std	%f24, [%sp + CCFSZ + 104]
	std	%f26, [%sp + CCFSZ + 112]
	std	%f28, [%sp + CCFSZ + 120]
	std	%f30, [%sp + CCFSZ + 128]

1:
	ldd	[%fp + 64], %o0		! sig, code
	ld	[%fp + 76], %o3		! arg3
	call	%g1			! (*sa->sa_handler)(sig,code,scp,arg3)
	 add	%fp, 64 + 16, %o2	! scp

	/*
	 * Now that the handler has returned, re-establish all the state
	 * we just saved above, then do a sigreturn.
	 */
	tst	%l0			! reload fpu registers?
	be	1f			! if not, skip the loads
	 wr	%l1, %g0, %y		! in any case, restore %y

	ld	[%sp + CCFSZ + 0], %fsr
	ldd	[%sp + CCFSZ + 8], %f0
	ldd	[%sp + CCFSZ + 16], %f2
	ldd	[%sp + CCFSZ + 24], %f4
	ldd	[%sp + CCFSZ + 32], %f6
	ldd	[%sp + CCFSZ + 40], %f8
	ldd	[%sp + CCFSZ + 48], %f10
	ldd	[%sp + CCFSZ + 56], %f12
	ldd	[%sp + CCFSZ + 64], %f14
	ldd	[%sp + CCFSZ + 72], %f16
	ldd	[%sp + CCFSZ + 80], %f18
	ldd	[%sp + CCFSZ + 88], %f20
	ldd	[%sp + CCFSZ + 96], %f22
	ldd	[%sp + CCFSZ + 104], %f24
	ldd	[%sp + CCFSZ + 112], %f26
	ldd	[%sp + CCFSZ + 120], %f28
	ldd	[%sp + CCFSZ + 128], %f30

1:
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7

	! get registers back & set syscall #
	restore	%g0, SUNOS_SYS_sigreturn, %g1
	add	%sp, 64 + 16, %o0	! compute scp
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	SUNOS_SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
_C_LABEL(sunos_esigcode):
#endif /* COMPAT_SUNOS */

#ifdef COMPAT_SVR4
/*
 * This code is still 32-bit only.
 */
/*
 * The following code is copied to the top of the user stack when each
 * process is exec'ed, and signals are `trampolined' off it.
 *
 * When this code is run, the stack looks like:
 *	[%sp]		64 bytes to which registers can be dumped
 *	[%sp + 64]	signal number (goes in %o0)
 *	[%sp + 64 + 4]	pointer to saved siginfo
 *	[%sp + 64 + 8]	pointer to saved context
 *	[%sp + 64 + 12]	address of the user's handler
 *	[%sp + 64 + 16]	first word of saved state (context)
 *	    .
 *	    .
 *	    .
 *	[%sp + NNN]	last word of saved state (siginfo)
 * (followed by previous stack contents or top of signal stack).
 * The address of the function to call is in %g1; the old %g1 and %o0
 * have already been saved in the sigcontext.  We are running in a clean
 * window, all previous windows now being saved to the stack.
 *
 * Note that [%sp + 64 + 8] == %sp + 64 + 16.  The copy at %sp+64+8
 * will eventually be removed, with a hole left in its place, if things
 * work out.
 */
	.globl	_C_LABEL(svr4_sigcode)
	.globl	_C_LABEL(svr4_esigcode)
_C_LABEL(svr4_sigcode):
	/*
	 * XXX  the `save' and `restore' below are unnecessary: should
	 *	replace with simple arithmetic on %sp
	 *
	 * Make room on the stack for 32 %f registers + %fsr.  This comes
	 * out to 33*4 or 132 bytes, but this must be aligned to a multiple
	 * of 8, or 136 bytes.
	 */
	save	%sp, -CCFSZ - 136, %sp
	mov	%g2, %l2		! save globals in %l registers
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	/*
	 * Saving the fpu registers is expensive, so do it iff the fsr
	 * stored in the sigcontext shows that the fpu is enabled.
	 */
	ld	[%fp + 64 + 16 + SC_PSR_OFFSET], %l0
	sethi	%hi(PSR_EF), %l1	! FPU enable bit is too high for andcc
	andcc	%l0, %l1, %l0		! %l0 = fpu enable bit
	be	1f			! if not set, skip the saves
	 rd	%y, %l1			! in any case, save %y

	! fpu is enabled, oh well
	st	%fsr, [%sp + CCFSZ + 0]
	std	%f0, [%sp + CCFSZ + 8]
	std	%f2, [%sp + CCFSZ + 16]
	std	%f4, [%sp + CCFSZ + 24]
	std	%f6, [%sp + CCFSZ + 32]
	std	%f8, [%sp + CCFSZ + 40]
	std	%f10, [%sp + CCFSZ + 48]
	std	%f12, [%sp + CCFSZ + 56]
	std	%f14, [%sp + CCFSZ + 64]
	std	%f16, [%sp + CCFSZ + 72]
	std	%f18, [%sp + CCFSZ + 80]
	std	%f20, [%sp + CCFSZ + 88]
	std	%f22, [%sp + CCFSZ + 96]
	std	%f24, [%sp + CCFSZ + 104]
	std	%f26, [%sp + CCFSZ + 112]
	std	%f28, [%sp + CCFSZ + 120]
	std	%f30, [%sp + CCFSZ + 128]

1:
	ldd	[%fp + 64], %o0		! sig, siginfo
	ld	[%fp + 72], %o2		! uctx
	call	%g1			! (*sa->sa_handler)(sig,siginfo,uctx)
	 nop

	/*
	 * Now that the handler has returned, re-establish all the state
	 * we just saved above, then do a sigreturn.
	 */
	tst	%l0			! reload fpu registers?
	be	1f			! if not, skip the loads
	 wr	%l1, %g0, %y		! in any case, restore %y

	ld	[%sp + CCFSZ + 0], %fsr
	ldd	[%sp + CCFSZ + 8], %f0
	ldd	[%sp + CCFSZ + 16], %f2
	ldd	[%sp + CCFSZ + 24], %f4
	ldd	[%sp + CCFSZ + 32], %f6
	ldd	[%sp + CCFSZ + 40], %f8
	ldd	[%sp + CCFSZ + 48], %f10
	ldd	[%sp + CCFSZ + 56], %f12
	ldd	[%sp + CCFSZ + 64], %f14
	ldd	[%sp + CCFSZ + 72], %f16
	ldd	[%sp + CCFSZ + 80], %f18
	ldd	[%sp + CCFSZ + 88], %f20
	ldd	[%sp + CCFSZ + 96], %f22
	ldd	[%sp + CCFSZ + 104], %f24
	ldd	[%sp + CCFSZ + 112], %f26
	ldd	[%sp + CCFSZ + 120], %f28
	ldd	[%sp + CCFSZ + 128], %f30

1:
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7

	restore	%g0, SVR4_SYS_context, %g1	! get registers & set syscall #
	mov	1, %o0
	add	%sp, 64 + 16, %o1	! compute ucontextp
	t	ST_SYSCALL		! svr4_context(1, ucontextp)
	! setcontext does not return unless it fails
	mov	SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
_C_LABEL(svr4_esigcode):
#endif /* COMPAT_SVR4 */

/*
 * Primitives
 */
#ifdef ENTRY
#undef ENTRY
#endif

#ifdef GPROF
	.globl	mcount
#define	ENTRY(x) \
	.globl _C_LABEL(x); _C_LABEL(x): ; \
	save	%sp, -CC64FSZ, %sp; \
	call	mcount; \
	nop; \
	restore
#else
#define	ENTRY(x)	.globl _C_LABEL(x); _C_LABEL(x):
#endif
#define	ALTENTRY(x)	.globl _C_LABEL(x); _C_LABEL(x):

/*
 * getfp() - get stack frame pointer
 */
ENTRY(getfp)
	retl
	 mov %fp, %o0

/*
 * copyinstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 */
ENTRY(copyinstr)
	! %o0 = fromaddr, %o1 = toaddr, %o2 = maxlen, %o3 = &lencopied
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	8f, %o0
	mov	%i0, %o1
	mov	%i1, %o2
	mov	%i2, %o3
	call	printf
	 mov	%i3, %o4
	restore
	.data
8:	.asciz	"copyinstr: from=%x to=%x max=%x &len=%x\n"
	_ALIGN
	.text
#endif
	brgz,pt	%o2, 1f					! Make sure len is valid
	 sethi	%hi(CPCB), %o4		! (first instr of copy)
	retl
	 mov	ENAMETOOLONG, %o0
1:
	LDPTR	[%o4 + %lo(CPCB)], %o4	! catch faults
	set	Lcsfault, %o5
	membar	#Sync
	STPTR	%o5, [%o4 + PCB_ONFAULT]

	mov	%o1, %o5		!	save = toaddr;
! XXX should do this in bigger chunks when possible
0:					! loop:
	ldsba	[%o0] ASI_AIUS, %g1	!	c = *fromaddr;
	stb	%g1, [%o1]		!	*toaddr++ = c;
	inc	%o1
	brz,a,pn	%g1, Lcsdone	!	if (c == NULL)
	 clr	%o0			!		{ error = 0; done; }
	deccc	%o2			!	if (--len > 0) {
	bg,pt	%icc, 0b		!		fromaddr++;
	 inc	%o0			!		goto loop;
	ba,pt	%xcc, Lcsdone		!	}
	 mov	ENAMETOOLONG, %o0	!	error = ENAMETOOLONG;
	NOTREACHED

/*
 * copyoutstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 */
ENTRY(copyoutstr)
	! %o0 = fromaddr, %o1 = toaddr, %o2 = maxlen, %o3 = &lencopied
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	8f, %o0
	mov	%i0, %o1
	mov	%i1, %o2
	mov	%i2, %o3
	call	printf
	 mov	%i3, %o4
	restore
	.data
8:	.asciz	"copyoutstr: from=%x to=%x max=%x &len=%x\n"
	_ALIGN
	.text
#endif
	brgz,pt	%o2, 1f					! Make sure len is valid
	 sethi	%hi(CPCB), %o4		! (first instr of copy)
	retl
	 mov	ENAMETOOLONG, %o0
1:
	LDPTR	[%o4 + %lo(CPCB)], %o4	! catch faults
	set	Lcsfault, %o5
	membar	#Sync
	STPTR	%o5, [%o4 + PCB_ONFAULT]

	mov	%o1, %o5		!	save = toaddr;
! XXX should do this in bigger chunks when possible
0:					! loop:
	ldsb	[%o0], %g1		!	c = *fromaddr;
	stba	%g1, [%o1] ASI_AIUS	!	*toaddr++ = c;
	inc	%o1
	brz,a,pn	%g1, Lcsdone	!	if (c == NULL)
	 clr	%o0			!		{ error = 0; done; }
	deccc	%o2			!	if (--len > 0) {
	bg,pt	%icc, 0b		!		fromaddr++;
	 inc	%o0			!		goto loop;
					!	}
	mov	ENAMETOOLONG, %o0	!	error = ENAMETOOLONG;
Lcsdone:				! done:
	sub	%o1, %o5, %o1		!	len = to - save;
	brnz,a	%o3, 1f			!	if (lencopied)
	 STPTR	%o1, [%o3]		!		*lencopied = len;
1:
	retl				! cpcb->pcb_onfault = 0;
	 STPTR	%g0, [%o4 + PCB_ONFAULT]! return (error);

Lcsfault:
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	5f, %o0
	call	printf
	 nop
	restore
	.data
5:	.asciz	"Lcsfault: recovering\n"
	_ALIGN
	.text
#endif
	b	Lcsdone			! error = EFAULT;
	 mov	EFAULT, %o0		! goto ret;

/*
 * copystr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from one point to another in
 * the kernel address space.  (This is a leaf procedure, but
 * it does not seem that way to the C compiler.)
 */
ENTRY(copystr)
	brgz,pt	%o2, 0f	! Make sure len is valid
	 mov	%o1, %o5		!	to0 = to;
	retl
	 mov	ENAMETOOLONG, %o0
0:					! loop:
	ldsb	[%o0], %o4		!	c = *from;
	tst	%o4
	stb	%o4, [%o1]		!	*to++ = c;
	be	1f			!	if (c == 0)
	 inc	%o1			!		goto ok;
	deccc	%o2			!	if (--len > 0) {
	bg,a	0b			!		from++;
	 inc	%o0			!		goto loop;
	b	2f			!	}
	 mov	ENAMETOOLONG, %o0	!	ret = ENAMETOOLONG; goto done;
1:					! ok:
	clr	%o0			!	ret = 0;
2:
	sub	%o1, %o5, %o1		!	len = to - to0;
	tst	%o3			!	if (lencopied)
	bnz,a	3f
	 STPTR	%o1, [%o3]		!		*lencopied = len;
3:
	retl
	 nop
#ifdef DIAGNOSTIC
4:
	sethi	%hi(5f), %o0
	call	_C_LABEL(panic)
	 or	%lo(5f), %o0, %o0
	.data
5:
	.asciz	"copystr"
	_ALIGN
	.text
#endif

/*
 * Copyin(src, dst, len)
 *
 * Copy specified amount of data from user space into the kernel.
 *
 * This is a modified version of bcopy that uses ASI_AIUS.  When
 * bcopy is optimized to use block copy ASIs, this should be also.
 */

#define	BCOPY_SMALL	32	/* if < 32, copy by bytes */

ENTRY(copyin)
!	flushw			! Make sure we don't have stack probs & lose hibits of %o
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i0, %o1
	mov	%i1, %o2
	call	printf
	 mov	%i2, %o3
	restore
	.data
1:	.asciz	"copyin: src=%x dest=%x len=%x\n"
	_ALIGN
	.text
#endif
	sethi	%hi(CPCB), %o3
	wr	%g0, ASI_AIUS, %asi
	LDPTR	[%o3 + %lo(CPCB)], %o3
	set	Lcopyfault, %o4
!	mov	%o7, %g7		! save return address
	membar	#Sync
	STPTR	%o4, [%o3 + PCB_ONFAULT]
	cmp	%o2, BCOPY_SMALL
Lcopyin_start:
	bge,a	Lcopyin_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
0:
	 inc	%o0
	ldsba	[%o0 - 1] %asi, %o4!	*dst++ = (++src)[-1];
	stb	%o4, [%o1]
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	ba	Lcopyin_done
	 clr	%o0
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lcopyin_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 EMPTY
	btst	7, %o1
	be,a	Lcopyin_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto copyin_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsba	[%o0] %asi, %o4	!	do {
	inc	%o0		!		(++dst)[-1] = *src++;
	inc	%o1
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 stb	%o4, [%o1 - 1]
	ba	Lcopyin_done
	 clr	%o0
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsba	[%o0] %asi, %o4	!	*dst++ = *src++;
	stb	%o4, [%o1]
	inc	%o0
	inc	%o1
	dec	%o2		!	len--;
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsha	[%o0] %asi, %o4	!	do {
	sth	%o4, [%o1]	!		*(short *)dst = *(short *)src;
	inc	2, %o0		!		dst += 2, src += 2;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lcopyin_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsha	[%o0] %asi, %o4	!	(*short *)dst = *(short *)src;
	sth	%o4, [%o1]
	inc	2, %o0		!	dst += 2;
	inc	2, %o1		!	src += 2;
	dec	2, %o2		!	len -= 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	lduwa	[%o0] %asi, %o4	!	do {
	st	%o4, [%o1]	!		*(int *)dst = *(int *)src;
	inc	4, %o0		!		dst += 4, src += 4;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lcopyin_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	lduwa	[%o0] %asi, %o4	!	*(int *)dst = *(int *)src;
	st	%o4, [%o1]
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	inc	4, %o1
	dec	4, %o2		! }
1:
Lcopyin_doubles:
	ldxa	[%o0] %asi, %g1	! do {
	stx	%g1, [%o1]	!	*(double *)dst = *(double *)src;
	inc	8, %o0		!	dst += 8, src += 8;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lcopyin_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lcopyin_done	!	goto copyin_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lcopyin_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	lduwa	[%o0] %asi, %o4	!	*(int *)dst = *(int *)src;
	st	%o4, [%o1]
	inc	4, %o0		!	dst += 4;
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lcopyin_mopw:
	be	Lcopyin_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsha	[%o0] %asi, %o4	! *(short *)dst = *(short *)src;
	be	Lcopyin_done	! if ((len & 1) == 0) goto done;
	 sth	%o4, [%o1]
	ldsba	[%o0 + 2] %asi, %o4	! dst[2] = src[2];
	stb	%o4, [%o1 + 2]
	ba	Lcopyin_done
	 clr	%o0
	NOTREACHED

	! mop up trailing byte (if present).
Lcopyin_mopb:
	be,a	Lcopyin_done
	 nop
	ldsba	[%o0] %asi, %o4
	stb	%o4, [%o1]

Lcopyin_done:
	sethi	%hi(CPCB), %o3
!	stb	%o4,[%o1]	! Store last byte -- should not be needed
	LDPTR	[%o3 + %lo(CPCB)], %o3
	membar	#Sync
	STPTR	%g0, [%o3 + PCB_ONFAULT]
	retl
	 clr	%o0			! return 0

/*
 * Copyout(src, dst, len)
 *
 * Copy specified amount of data from kernel to user space.
 * Just like copyin, except that the `dst' addresses are user space
 * rather than the `src' addresses.
 *
 * This is a modified version of bcopy that uses ASI_AIUS.  When
 * bcopy is optimized to use block copy ASIs, this should be also.
 */
 /*
  * This needs to be reimplemented to really do the copy.
  */
ENTRY(copyout)
	/*
	 * ******NOTE****** this depends on bcopy() not using %g7
	 */
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i0, %o1
	set	CTX_SECONDARY, %o4
	mov	%i1, %o2
	ldxa	[%o4] ASI_DMMU, %o4
	call	printf
	 mov	%i2, %o3
	restore
	.data
1:	.asciz	"copyout: src=%x dest=%x len=%x ctx=%d\n"
	_ALIGN
	.text
#endif
Ldocopy:
	sethi	%hi(CPCB), %o3
	wr	%g0, ASI_AIUS, %asi
	LDPTR	[%o3 + %lo(CPCB)], %o3
	set	Lcopyfault, %o4
!	mov	%o7, %g7		! save return address
	membar	#Sync
	STPTR	%o4, [%o3 + PCB_ONFAULT]
	cmp	%o2, BCOPY_SMALL
Lcopyout_start:
	bge,a	Lcopyout_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 EMPTY
0:
	inc	%o0
	ldsb	[%o0 - 1], %o4!	(++dst)[-1] = *src++;
	stba	%o4, [%o1] %asi
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	ba	Lcopyout_done
	 clr	%o0
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lcopyout_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 EMPTY
	btst	7, %o1
	be,a	Lcopyout_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto copyout_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsb	[%o0], %o4	!	do {
	inc	%o0		!		(++dst)[-1] = *src++;
	inc	%o1
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 stba	%o4, [%o1 - 1] %asi
	ba	Lcopyout_done
	 clr	%o0
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	stba	%o4, [%o1] %asi
	inc	%o0
	inc	%o1
	dec	%o2		!	len--;
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsh	[%o0], %o4	!	do {
	stha	%o4, [%o1] %asi	!		*(short *)dst = *(short *)src;
	inc	2, %o0		!		dst += 2, src += 2;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lcopyout_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsh	[%o0], %o4	!	(*short *)dst = *(short *)src;
	stha	%o4, [%o1] %asi
	inc	2, %o0		!	dst += 2;
	inc	2, %o1		!	src += 2;
	dec	2, %o2		!	len -= 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	lduw	[%o0], %o4	!	do {
	sta	%o4, [%o1] %asi	!		*(int *)dst = *(int *)src;
	inc	4, %o0		!		dst += 4, src += 4;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lcopyout_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	lduw	[%o0], %o4	!	*(int *)dst = *(int *)src;
	sta	%o4, [%o1] %asi
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	inc	4, %o1
	dec	4, %o2		! }
1:
Lcopyout_doubles:
	ldx	[%o0], %g1	! do {
	stxa	%g1, [%o1] %asi	!	*(double *)dst = *(double *)src;
	inc	8, %o0		!	dst += 8, src += 8;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lcopyout_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lcopyout_done	!	goto copyout_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lcopyout_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	lduw	[%o0], %o4	!	*(int *)dst = *(int *)src;
	sta	%o4, [%o1] %asi
	inc	4, %o0		!	dst += 4;
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lcopyout_mopw:
	be	Lcopyout_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsh	[%o0], %o4	! *(short *)dst = *(short *)src;
	be	Lcopyout_done	! if ((len & 1) == 0) goto done;
	 stha	%o4, [%o1] %asi
	ldsb	[%o0 + 2], %o4	! dst[2] = src[2];
	stba	%o4, [%o1 + 2] %asi
	ba	Lcopyout_done
	 clr	%o0
	NOTREACHED

	! mop up trailing byte (if present).
Lcopyout_mopb:
	be,a	Lcopyout_done
	 nop
	ldsb	[%o0], %o4
	stba	%o4, [%o1] %asi

Lcopyout_done:
	sethi	%hi(CPCB), %o3
	LDPTR	[%o3 + %lo(CPCB)], %o3
	membar	#Sync
	STPTR	%g0, [%o3 + PCB_ONFAULT]
!	jmp	%g7 + 8		! Original instr
	retl			! New instr
	 clr	%o0			! return 0

! Copyin or copyout fault.  Clear cpcb->pcb_onfault and return EFAULT.
! Note that although we were in bcopy, there is no state to clean up;
! the only special thing is that we have to return to [g7 + 8] rather than
! [o7 + 8].
Lcopyfault:
	sethi	%hi(CPCB), %o3
	LDPTR	[%o3 + %lo(CPCB)], %o3
	STPTR	%g0, [%o3 + PCB_ONFAULT]
	membar	#Sync
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	call	printf
	 nop
	restore
	.data
1:	.asciz	"copyfault: fault occured\n"
	_ALIGN
	.text
#endif
	retl
	 mov	EFAULT, %o0


	.data
	_ALIGN
	.comm	_C_LABEL(want_resched),4

/*
 * Switch statistics (for later tweaking):
 *	nswitchdiff = p1 => p2 (i.e., chose different process)
 *	nswitchexit = number of calls to switchexit()
 *	_cnt.v_swtch = total calls to swtch+swtchexit
 */
	.comm	_C_LABEL(nswitchdiff), 4
	.comm	_C_LABEL(nswitchexit), 4
	.text
/*
 * REGISTER USAGE IN cpu_switch AND switchexit:
 * This is split into two phases, more or less
 * `before we locate a new proc' and `after'.
 * Some values are the same in both phases.
 * Note that the %o0-registers are not preserved across
 * the psr change when entering a new process, since this
 * usually changes the CWP field (hence heavy usage of %g's).
 *
 *	%g1 = <free>; newpcb -- WARNING this register tends to get trashed
 *	%g2 = %hi(_whichqs); newpsr
 *	%g3 = p
 *	%g4 = lastproc
 *	%g5 = oldpsr (excluding ipl bits)
 *	%g6 = %hi(cpcb)
 *	%g7 = %hi(curproc)
 *	%o0 = tmp 1
 *	%o1 = tmp 2
 *	%o2 = tmp 3
 *	%o3 = tmp 4; whichqs; vm
 *	%o4 = tmp 4; which; sswap
 *	%o5 = tmp 5; q; <free>
 */

/*
 * switchexit is called only from cpu_exit() before the current process
 * has freed its vmspace and kernel stack; we must schedule them to be
 * freed.  (curproc is already NULL.)
 *
 * We lay the process to rest by changing to the `idle' kernel stack,
 * and note that the `last loaded process' is nonexistent.
 */
ENTRY(switchexit)
	flushw				! We don't have anything else to run, so why not
#ifdef DEBUG
	save	%sp, -CC64FSZ, %sp
	flushw
	restore
#endif
	wrpr	%g0, PSTATE_KERN, %pstate ! Make sure we're on the right globals
	mov	%o0, %g2		! save proc arg for exit2() call

#ifdef SCHED_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	call	printf
	 nop
	LOCTOGLOB
	restore
	.data
1:	.asciz	"switchexit()\r\n"
	_ALIGN
	.text
#endif
	/*
	 * Change pcb to idle u. area, i.e., set %sp to top of stack
	 * and %psr to PSR_S|PSR_ET, and set cpcb to point to _idle_u.
	 * Once we have left the old stack, we can call kmem_free to
	 * destroy it.  Call it any sooner and the register windows
	 * go bye-bye.
	 */
	set	_C_LABEL(idle_u), %g1
	sethi	%hi(CPCB), %g6
#if 0
	/* Get rid of the stack	*/
	rdpr	%ver, %o0
	wrpr	%g0, 0, %canrestore	! Fixup window state regs
	and	%o0, 0x0f, %o0
	wrpr	%g0, 0, %otherwin
	wrpr	%g0, %o0, %cleanwin	! kernel don't care, but user does
	dec	1, %o0			! What happens if we don't subtract 2?
	wrpr	%g0, %o0, %cansave
	flushw						! DEBUG
#endif

	STPTR	%g1, [%g6 + %lo(CPCB)]	! cpcb = &idle_u
	set	_C_LABEL(idle_u) + USPACE - CC64FSZ, %o0	! set new %sp
#ifdef _LP64
	sub	%o0, BIAS, %sp		! Maybe this should be a save?
#else
	mov	%o0, %sp		! Maybe this should be a save?
#endif
	save	%sp,-CC64FSZ,%sp	! Get an extra frame for good measure
	flushw				! DEBUG this should not be needed
	wrpr	%g0, 0, %canrestore
	wrpr	%g0, 0, %otherwin
	rdpr	%ver, %l7
	and	%l7, CWP, %l7
	wrpr	%l7, 0, %cleanwin
	dec	1, %l7					! NWINDOWS-1-1
	wrpr	%l7, %cansave
	flushw						! DEBUG
#ifdef DEBUG
	set	_C_LABEL(idle_u), %l6
	SET_SP_REDZONE(%l6, %l5)
#endif
	wrpr	%g0, PSTATE_INTR, %pstate	! and then enable traps
	call	_C_LABEL(exit2)			! exit2(p)
	 mov	%g2, %o0

	/*
	 * Now fall through to `the last switch'.  %g6 was set to
	 * %hi(cpcb), but may have been clobbered in kmem_free,
	 * so all the registers described below will be set here.
	 *
	 * REGISTER USAGE AT THIS POINT:
	 *	%g2 = %hi(_whichqs)
	 *	%g4 = lastproc
	 *	%g5 = oldpsr (excluding ipl bits)
	 *	%g6 = %hi(cpcb)
	 *	%g7 = %hi(curproc)
	 *	%o0 = tmp 1
	 *	%o1 = tmp 2
	 *	%o3 = whichqs
	 */

	INCR(_C_LABEL(nswitchexit))		! nswitchexit++;
	INCR(_C_LABEL(uvmexp)+V_SWTCH)		! cnt.v_switch++;

	sethi	%hi(_C_LABEL(sched_whichqs)), %g2
	clr	%g4			! lastproc = NULL;
	sethi	%hi(CPCB), %g6
	sethi	%hi(CURPROC), %g7
	LDPTR	[%g6 + %lo(CPCB)], %g5
	wr	%g0, ASI_DMMU, %asi
	ldxa	[CTX_SECONDARY] %asi, %g1	! Don't demap the kernel
	brz,pn	%g1, 1f
	 set	0x030, %g1			! Demap secondary context
	stxa	%g1, [%g1] ASI_DMMU_DEMAP
	stxa	%g1, [%g1] ASI_IMMU_DEMAP
	membar	#Sync
1:
	stxa	%g0, [CTX_SECONDARY] %asi	! Clear out our context
	membar	#Sync
	/* FALLTHROUGH */

/*
 * When no processes are on the runq, switch
 * idles here waiting for something to come ready.
 * The registers are set up as noted above.
 */
	.globl	idle
idle:
	STPTR	%g0, [%g7 + %lo(CURPROC)] ! curproc = NULL;
1:					! spin reading _whichqs until nonzero
	wrpr	%g0, PSTATE_INTR, %pstate		! Make sure interrupts are enabled
	wrpr	%g0, 0, %pil		! (void) spl0();
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	idlemsg, %o0
	mov	%g1, %o1
	mov	%g2, %o2
	mov	%g3, %o3
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	call	_C_LABEL(prom_printf)
	 mov	%g4, %o4
	set	idlemsg1, %o0
	mov	%l5, %o1
	mov	%l6, %o2
	call	_C_LABEL(prom_printf)
	 mov	%l7, %o3
	LOCTOGLOB
	restore
#endif
	ld	[%g2 + %lo(_C_LABEL(sched_whichqs))], %o3
	brnz,a,pt	%o3, Lsw_scan
	 wrpr	%g0, PIL_CLOCK, %pil	! (void) splclock();
	ba,a,pt	%icc, 1b

Lsw_panic_rq:
	sethi	%hi(1f), %o0
	call	_C_LABEL(panic)
	 or	%lo(1f), %o0, %o0
Lsw_panic_wchan:
	sethi	%hi(2f), %o0
	call	_C_LABEL(panic)
	 or	%lo(2f), %o0, %o0
Lsw_panic_srun:
	sethi	%hi(3f), %o0
	call	_C_LABEL(panic)
	 or	%lo(3f), %o0, %o0
	.data
1:	.asciz	"switch rq"
2:	.asciz	"switch wchan"
3:	.asciz	"switch SRUN"
idlemsg:	.asciz	"idle %x %x %x %x"
idlemsg1:	.asciz	" %x %x %x\r\n"
	_ALIGN
	.text
/*
 * cpu_switch() picks a process to run and runs it, saving the current
 * one away.  On the assumption that (since most workstations are
 * single user machines) the chances are quite good that the new
 * process will turn out to be the current process, we defer saving
 * it here until we have found someone to load.  If that someone
 * is the current process we avoid both store and load.
 *
 * cpu_switch() is always entered at splstatclock or splhigh.
 *
 * IT MIGHT BE WORTH SAVING BEFORE ENTERING idle TO AVOID HAVING TO
 * SAVE LATER WHEN SOMEONE ELSE IS READY ... MUST MEASURE!
 *
 * Apparently cpu_switch() is called with curproc as the first argument,
 * but no port seems to make use of that parameter.
 */
	.globl	_C_LABEL(time)
ENTRY(cpu_switch)
	/*
	 * REGISTER USAGE AT THIS POINT:
	 *	%g1 = tmp 0
	 *	%g2 = %hi(_C_LABEL(whichqs))
	 *	%g3 = p
	 *	%g4 = lastproc
	 *	%g5 = cpcb
	 *	%g6 = %hi(CPCB)
	 *	%g7 = %hi(CURPROC)
	 *	%o0 = tmp 1
	 *	%o1 = tmp 2
	 *	%o2 = tmp 3
	 *	%o3 = tmp 4, then at Lsw_scan, whichqs
	 *	%o4 = tmp 5, then at Lsw_scan, which
	 *	%o5 = tmp 6, then at Lsw_scan, q
	 */
#ifdef DEBUG
	set	swdebug, %o1
	ld	[%o1], %o1
	brz,pt	%o1, 2f
	 set	1f, %o0
	call	printf
	 nop
	.data
1:	.asciz	"s"
	_ALIGN
	.globl	swdebug
swdebug:	.word 0
	.text
2:
#endif
#ifdef NOTDEF_DEBUG
	set	_C_LABEL(intrdebug), %g1
	mov	INTRDEBUG_FUNC, %o1
	st	%o1, [%g1]
#endif
	flushw				! We don't have anything else to run, so why not flush
#ifdef DEBUG
	save	%sp, -CC64FSZ, %sp
	flushw
	restore
#endif
	rdpr	%pstate, %o1		! oldpstate = %pstate;
	wrpr	%g0, PSTATE_INTR, %pstate ! make sure we're on normal globals
	sethi	%hi(CPCB), %g6
	sethi	%hi(_C_LABEL(sched_whichqs)), %g2	! set up addr regs
	LDPTR	[%g6 + %lo(CPCB)], %g5
	sethi	%hi(CURPROC), %g7
	stx	%o7, [%g5 + PCB_PC]	! cpcb->pcb_pc = pc;
	LDPTR	[%g7 + %lo(CURPROC)], %g4	! lastproc = curproc;
	sth	%o1, [%g5 + PCB_PSTATE]	! cpcb->pcb_pstate = oldpstate;

	/*
	 * In all the fiddling we did to get this far, the thing we are
	 * waiting for might have come ready, so let interrupts in briefly
	 * before checking for other processes.  Note that we still have
	 * curproc set---we have to fix this or we can get in trouble with
	 * the run queues below.
	 */
	rdpr	%pil, %g3		! %g3 has not been used yet
	STPTR	%g0, [%g7 + %lo(CURPROC)]	! curproc = NULL;
	wrpr	%g0, 0, %pil			! (void) spl0();
	stb	%g3, [%g5 + PCB_PIL]	! save old %pil
	wrpr	%g0, PIL_CLOCK, %pil	! (void) splclock();

Lsw_scan:
	ld	[%g2 + %lo(_C_LABEL(sched_whichqs))], %o3

#ifndef POPC
	.globl	_C_LABEL(__ffstab)
	/*
	 * Optimized inline expansion of `which = ffs(whichqs) - 1';
	 * branches to idle if ffs(whichqs) was 0.
	 */
	set	_C_LABEL(__ffstab), %o2
	andcc	%o3, 0xff, %o1		! byte 0 zero?
	bz,a,pn	%icc, 1f		! yes, try byte 1
	 srl	%o3, 8, %o0
	ba,pt	%icc, 2f		! ffs = ffstab[byte0]; which = ffs - 1;
	 ldsb	[%o2 + %o1], %o0
1:	andcc	%o0, 0xff, %o1		! byte 1 zero?
	bz,a,pn	%icc, 1f		! yes, try byte 2
	 srl	%o0, 8, %o0
	ldsb	[%o2 + %o1], %o0	! which = ffstab[byte1] + 7;
	ba,pt	%icc, 3f
	 add	%o0, 7, %o4
1:	andcc	%o0, 0xff, %o1		! byte 2 zero?
	bz,a,pn	%icc, 1f		! yes, try byte 3
	 srl	%o0, 8, %o0
	ldsb	[%o2 + %o1], %o0	! which = ffstab[byte2] + 15;
	ba,pt	%icc, 3f
	 add	%o0, 15, %o4
1:	ldsb	[%o2 + %o0], %o0	! ffs = ffstab[byte3] + 24
	addcc	%o0, 24, %o0		! (note that ffstab[0] == -24)
	bz,pn	%icc, idle		! if answer was 0, go idle
	 EMPTY
2:	sub	%o0, 1, %o4
3:	/* end optimized inline expansion */

#else
	/*
	 * Optimized inline expansion of `which = ffs(whichqs) - 1';
	 * branches to idle if ffs(whichqs) was 0.
	 *
	 * This version uses popc.
	 *
	 * XXXX spitfires and blackbirds don't implement popc.
	 *
	 */
	brz,pn	%o3, idle				! Don't bother if queues are empty
	 neg	%o3, %o1				! %o1 = -zz
	xnor	%o3, %o1, %o2				! %o2 = zz ^ ~ -zz
	popc	%o2, %o4				! which = popc(whichqs)
	dec	%o4					! which = ffs(whichqs) - 1

#endif
	/*
	 * We found a nonempty run queue.  Take its first process.
	 */
	set	_C_LABEL(sched_qs), %o5	! q = &qs[which];
	sll	%o4, PTRSHFT+1, %o0
	add	%o0, %o5, %o5
	LDPTR	[%o5], %g3		! p = q->ph_link;
	cmp	%g3, %o5		! if (p == q)
	be,pn	%icc, Lsw_panic_rq	!	panic("switch rq");
	 EMPTY
	LDPTR	[%g3], %o0		! tmp0 = p->p_forw;
	STPTR	%o0, [%o5]		! q->ph_link = tmp0;
	STPTR	%o5, [%o0 + PTRSZ]	! tmp0->p_back = q;
	cmp	%o0, %o5		! if (tmp0 == q)
	bne	1f
	 EMPTY
	mov	1, %o1			!	whichqs &= ~(1 << which);
	sll	%o1, %o4, %o1
	andn	%o3, %o1, %o3
	st	%o3, [%g2 + %lo(_C_LABEL(sched_whichqs))]
1:
	/*
	 * PHASE TWO: NEW REGISTER USAGE:
	 *	%g1 = newpcb
	 *	%g2 = newpstate
	 *	%g3 = p
	 *	%g4 = lastproc
	 *	%g5 = cpcb
	 *	%g6 = %hi(_cpcb)
	 *	%g7 = %hi(_curproc)
	 *	%o0 = tmp 1
	 *	%o1 = tmp 2
	 *	%o2 = tmp 3
	 *	%o3 = vm
	 *	%o4 = sswap
	 *	%o5 = <free>
	 */

	/* firewalls */
	LDPTR	[%g3 + P_WCHAN], %o0	! if (p->p_wchan)
	brnz,pn	%o0, Lsw_panic_wchan	!	panic("switch wchan");
	 EMPTY
	ldsb	[%g3 + P_STAT], %o0	! if (p->p_stat != SRUN)
	cmp	%o0, SRUN
	bne	Lsw_panic_srun		!	panic("switch SRUN");
	 EMPTY

	/*
	 * Committed to running process p.
	 * It may be the same as the one we were running before.
	 */
#if defined(MULTIPROCESSOR)
	/*
	 * XXXSMP
	 * p->p_cpu = curcpu();
	 */
#endif
	mov	SONPROC, %o0			! p->p_stat = SONPROC
	stb	%o0, [%g3 + P_STAT]
	sethi	%hi(_C_LABEL(want_resched)), %o0
	st	%g0, [%o0 + %lo(_C_LABEL(want_resched))]	! want_resched = 0;
	LDPTR	[%g3 + P_ADDR], %g1		! newpcb = p->p_addr;
	STPTR	%g0, [%g3 + PTRSZ]		! p->p_back = NULL;
	ldub	[%g1 + PCB_PIL], %g2		! newpil = newpcb->pcb_pil;
	STPTR	%g4, [%g7 + %lo(CURPROC)]	! restore old proc so we can save it

	cmp	%g3, %g4		! p == lastproc?
	be,a	Lsw_sameproc		! yes, go return 0
	 wrpr	%g2, 0, %pil		! (after restoring pil)

	/*
	 * Not the old process.  Save the old process, if any;
	 * then load p.
	 */
#ifdef SCHED_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	mov	%g4, %o1
	ld	[%o1+P_PID], %o2
	mov	%g3, %o3
	call	printf
	 ld	[%o3+P_PID], %o4
	LOCTOGLOB
	ba	2f
	 restore
	.data
1:	.asciz	"cpu_switch: %x(%d)->%x(%d)\r\n"
	_ALIGN
	.text
	Debugger();
2:
#endif
	flushw				! DEBUG -- make sure we don't hold on to any garbage
	brz,pn	%g4, Lsw_load		! if no old process, go load
	 wrpr	%g0, PSTATE_KERN, %pstate

	INCR(_C_LABEL(nswitchdiff))	! clobbers %o0,%o1
wb1:
	flushw				! save all register windows except this one
	save	%sp, -CC64FSZ, %sp	! Get space for this one
	stx	%i7, [%g5 + PCB_PC]	! Save rpc
	flushw				! save this window, too
	stx	%i6, [%g5 + PCB_SP]
	rdpr	%cwp, %o2
	stb	%o2, [%g5 + PCB_CWP]

	/*
	 * Load the new process.  To load, we must change stacks and
	 * alter cpcb and the window control registers, hence we must
	 * disable interrupts.
	 *
	 * We also must load up the `in' and `local' registers.
	 */
Lsw_load:
#ifdef SCHED_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	call	printf
	 nop
	LOCTOGLOB
	restore
	.data
1:	.asciz	"cpu_switch: loading the new process:\r\n"
	_ALIGN
	.text
#endif
	/* set new cpcb */
	STPTR	%g3, [%g7 + %lo(CURPROC)]	! curproc = p;
	STPTR	%g1, [%g6 + %lo(CPCB)]	! cpcb = newpcb;

#ifdef SCHED_DEBUG
	ldx	[%g1 + PCB_SP], %o0
	brnz,pt	%o0, 2f
	 ldx	[%o0], %o0			! Force a fault if needed
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	call	printf
	 nop
	LOCTOGLOB
	restore
	ta 1
	.data
1:	.asciz	"cpu_switch: NULL %sp\r\n"
	_ALIGN
	.text
2:
#endif
	ldx	[%g1 + PCB_SP], %i6
	call	_C_LABEL(blast_vcache)		! Clear out I$ and D$
	 ldx	[%g1 + PCB_PC], %i7
	wrpr	%g0, 0, %otherwin	! These two insns should be redundant
	wrpr	%g0, 0, %canrestore
	rdpr	%ver, %l7
	and	%l7, CWP, %l7
	wrpr	%g0, %l7, %cleanwin
!	wrpr	%g0, 0, %cleanwin	! DEBUG
	dec	1, %l7					! NWINDOWS-1-1
	wrpr	%l7, %cansave
	wrpr	%g0, 4, %tl				! DEBUG -- force watchdog
	flushw						! DEBUG
	wrpr	%g0, 0, %tl				! DEBUG
	/* load window */
	restore				! The logic is just too complicated to handle here.  Let the traps deal with the problem
!	flushw						! DEBUG
#ifdef SCHED_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	call	printf
	 mov	%fp, %o1
	LOCTOGLOB
	restore
	.data
1:	.asciz	"cpu_switch: setup new process stack regs at %08x\r\n"
	_ALIGN
	.text
#endif
#ifdef DEBUG
	mov	%g1, %o0
	SET_SP_REDZONE(%o0, %o1)
	CHECK_SP_REDZONE(%o0, %o1)
#endif
	/* finally, enable traps */
!	lduh	[%g1 + PCB_PSTATE], %o3	! Load newpstate
!	wrpr	%o3, 0, %pstate		! psr = newpsr;
	wrpr	%g0, PSTATE_INTR, %pstate

	/*
	 * Now running p.  Make sure it has a context so that it
	 * can talk about user space stuff.  (Its pcb_uw is currently
	 * zero so it is safe to have interrupts going here.)
	 */
	save	%sp, -CC64FSZ, %sp
	LDPTR	[%g3 + P_VMSPACE], %o3	! vm = p->p_vmspace;
	set	_C_LABEL(kernel_pmap_), %o1
	LDPTR	[%o3 + VM_PMAP], %o2		! if (vm->vm_pmap.pm_ctx != NULL)
	cmp	%o2, %o1
	bz,pn	%xcc, Lsw_havectx		! Don't replace kernel context!
	 ld	[%o2 + PM_CTX], %o0
	brnz,pt	%o0, Lsw_havectx		!	goto havecontext;
	 nop

	/* p does not have a context: call ctx_alloc to get one */
	call	_C_LABEL(ctx_alloc)		! ctx_alloc(&vm->vm_pmap);
	 mov	%o2, %o0

#ifdef SCHED_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	call	printf
 	 mov	%i0, %o1
	LOCTOGLOB
	restore
	.data
1:	.asciz	"cpu_switch: got new ctx %d in new process\r\n"
	_ALIGN
	.text
#endif
	/* p does have a context: just switch to it */
Lsw_havectx:
	! context is in %o0
	/*
	 * We probably need to flush the cache here.
	 */
	wr	%g0, ASI_DMMU, %asi		! restore the user context
	ldxa	[CTX_SECONDARY] %asi, %o1
	brz,pn	%o1, 1f
	 set	0x030, %o1
	stxa	%o1, [%o1] ASI_DMMU_DEMAP
	stxa	%o1, [%o1] ASI_IMMU_DEMAP
	membar	#Sync
1:
	stxa	%o0, [CTX_SECONDARY] %asi	! Maybe we should invalidate the old context?
	membar	#Sync				! Maybe we should use flush here?
	flush	%sp

!	call	blast_vcache	! Maybe we don't need to do this now
	 nop

	restore
#ifdef SCHED_DEBUG
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	mov	%i0, %o2
	call	printf
	 mov	%i7, %o1
	LOCTOGLOB
	restore
	.data
1:	.asciz	"cpu_switch: in new process pc=%08x ctx %d\r\n"
	_ALIGN
	.text
#endif
#ifdef TRAPTRACE
	set	trap_trace, %o2
	lduw	[%o2+TRACEDIS], %o4
	brnz,pn	%o4, 1f
	 nop
	lduw	[%o2+TRACEPTR], %o3
	rdpr	%tl, %o4
	mov	4, %o5
	set	CURPROC, %o0
	sllx	%o4, 13, %o4
	LDPTR	[%o0], %o0
!	clr	%o0		! DISABLE PID
	or	%o4, %o5, %o4
	mov	%g0, %o5
	brz,pn	%o0, 2f
	 andncc	%o3, (TRACESIZ-1), %g0
!	ldsw	[%o0+P_PID], %o5	!  Load PID
2:
	movnz	%icc, %g0, %o3	! Wrap if needed

	set	CPCB, %o0	! Load up nsaved
	LDPTR	[%o0], %o0
	ldub	[%o0 + PCB_NSAVED], %o0
	sllx	%o0, 9, %o1
	or	%o1, %o4, %o4

	sth	%o4, [%o2+%o3]
	inc	2, %o3
	sth	%o5, [%o2+%o3]
	inc	2, %o3
	stw	%o0, [%o2+%o3]
	inc	4, %o3
	stw	%sp, [%o2+%o3]
	inc	4, %o3
	stw	%o7, [%o2+%o3]
	inc	4, %o3
	mov	TLB_TAG_ACCESS, %o4
	ldxa	[%o4] ASI_DMMU, %o4
	stw	%o4, [%o2+%o3]
	inc	4, %o3
	stw	%o3, [%o2+TRACEPTR]
1:
#endif


Lsw_sameproc:
	/*
	 * We are resuming the process that was running at the
	 * call to switch().  Just set psr ipl and return.
	 */
#ifdef SCHED_DEBUG
	mov	%l0, %o0
	save	%sp, -CC64FSZ, %sp
	GLOBTOLOC
	set	1f, %o0
	mov	%i0, %o2
	set	CURPROC, %o3
	LDPTR	[%o3], %o3
	ld	[%o3 + P_VMSPACE], %o3
	call	printf
	 mov	%i7, %o1
#ifdef DEBUG
	set	swtchdelay, %o0
	call	delay
	 ld	[%o0], %o0
	set	pmapdebug, %o0
	ld	[%o0], %o0
	tst	%o0
	tnz	%icc, 1; nop	! Call debugger if we're in pmapdebug
#endif
	LOCTOGLOB
	ba	2f		! Skip debugger
	 restore
	.data
1:	.asciz	"cpu_switch: vectoring to pc=%08x thru %08x vmspace=%p\r\n"
	_ALIGN
	.globl	swtchdelay
swtchdelay:
	.word	1000
	.text
	Debugger();
2:
#endif
!	wrpr	%g0, 0, %cleanwin	! DEBUG
	clr	%g4		! This needs to point to the base of the data segment
	retl
	 wrpr	%g0, PSTATE_INTR, %pstate


/*
 * Snapshot the current process so that stack frames are up to date.
 * Only used just before a crash dump.
 */
ENTRY(snapshot)
	rdpr	%pstate, %o1		! save psr
	stx	%o6, [%o0 + PCB_SP]	! save sp
	rdpr	%pil, %o2
	sth	%o1, [%o0 + PCB_PSTATE]
	rdpr	%cwp, %o3
	stb	%o2, [%o0 + PCB_PIL]
	stb	%o3, [%o0 + PCB_CWP]

	flushw
	save	%sp, -CC64FSZ, %sp
	flushw
	ret
	 restore

/*
 * cpu_set_kpc() and cpu_fork() arrange for proc_trampoline() to run
 * after after a process gets chosen in switch(). The stack frame will
 * contain a function pointer in %l0, and an argument to pass to it in %l2.
 *
 * If the function *(%l0) returns, we arrange for an immediate return
 * to user mode. This happens in two known cases: after execve(2) of init,
 * and when returning a child to user mode after a fork(2).
 */
	nop; nop					! Make sure we don't get lost getting here.
ENTRY(proc_trampoline)
#ifdef SCHED_DEBUG
	nop; nop; nop; nop				! Try to make sure we don't vector into the wrong instr
	mov	%l0, %o0
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i6, %o2
	call	printf
	 mov	%i0, %o1
	ba	2f
	 restore
	.data
1:	.asciz	"proc_trampoline: calling %x sp %x\r\n"
	_ALIGN
	.text
	Debugger()
2:
#endif
	call	%l0			! re-use current frame
	 mov	%l1, %o0

	/*
	 * Here we finish up as in syscall, but simplified.  We need to
	 * fiddle pc and npc a bit, as execve() / setregs() /cpu_set_kpc()
	 * have only set npc, in anticipation that trap.c will advance past
	 * the trap instruction; but we bypass that, so we must do it manually.
	 */
!	save	%sp, -CC64FSZ, %sp		! Save a kernel frame to emulate a syscall
#if 0
	/* This code doesn't seem to work, but it should. */
	ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1
	ldx	[%sp + CC64FSZ + STKB + TF_NPC], %g2	! pc = tf->tf_npc from execve/fork
	andn	%g1, CWP, %g1			! Clear the CWP bits
	add	%g2, 4, %g3			! npc = pc+4
	rdpr	%cwp, %g5			! Fixup %cwp in %tstate
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]
	or	%g1, %g5, %g1
	stx	%g2, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
#else
	mov	PSTATE_USER, %g1		! XXXX user pstate (no need to load it)
	ldx	[%sp + CC64FSZ + STKB + TF_NPC], %g2	! pc = tf->tf_npc from execve/fork
	sllx	%g1, TSTATE_PSTATE_SHIFT, %g1	! Shift it into place
	add	%g2, 4, %g3			! npc = pc+4
	rdpr	%cwp, %g5			! Fixup %cwp in %tstate
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]
	or	%g1, %g5, %g1
	stx	%g2, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
#endif
#ifdef SCHED_DEBUG
!	set	panicstack-CC64FSZ-STKB, %o0! DEBUG
!	save	%g0, %o0, %sp	! DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	ldx	[%fp + CC64FSZ + STKB + TF_O + ( 6*8)], %o2
	mov	%fp, %o2
	add	%fp, CC64FSZ + STKB, %o3
	GLOBTOLOC
	call	printf
	 mov	%g2, %o1
	LOCTOGLOB
	set	3f, %o0
	mov	%g1, %o1
	mov	%g2, %o2
	mov	CTX_SECONDARY, %o4
	ldxa	[%o4] ASI_DMMU, %o4
	call	printf
	 mov	%g3, %o3
	LOCTOGLOB
	ba 2f
	restore
	.data
1:	.asciz	"proc_trampoline: returning to %p, sp=%p, tf=%p\r\n"
3:	.asciz	"tstate=%p tpc=%p tnpc=%p ctx=%x\r\n"
	_ALIGN
	.text
	Debugger()
2:
#endif
	CHKPT(%o3,%o4,0x35)
	ba,a,pt	%icc, return_from_trap
	 nop

/*
 * {fu,su}{,i}{byte,word}
 */
ALTENTRY(fuiword)
ENTRY(fuword)
	btst	3, %o0			! has low bits set...
	bnz	Lfsbadaddr		!	go return -1
	EMPTY
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	set	Lfserr, %o3
	LDPTR	[%o2 + %lo(CPCB)], %o2
	membar	#Sync
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	LDPTRA	[%o0] ASI_AIUS, %o0	! fetch the word
	membar	#Sync
	retl				! phew, made it, return the word
	 STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

Lfserr:
	STPTR	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	membar	#Sync
Lfsbadaddr:
#ifndef _LP64
	mov	-1, %o1
#endif
	retl				! and return error indicator
	 mov	-1, %o0

	/*
	 * This is just like Lfserr, but it's a global label that allows
	 * mem_access_fault() to check to see that we don't want to try to
	 * page in the fault.  It's used by fuswintr() etc.
	 */
	.globl	_C_LABEL(Lfsbail)
_C_LABEL(Lfsbail):
	STPTR	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	retl				! and return error indicator
	 mov	-1, %o0

	/*
	 * Like fusword but callable from interrupt context.
	 * Fails if data isn't resident.
	 */
ENTRY(fuswintr)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = _Lfsbail;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	_C_LABEL(Lfsbail), %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	lduha	[%o0] ASI_AIUS, %o0	! fetch the halfword
	membar	#Sync
	retl				! made it
	 STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

ENTRY(fusword)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	lduha	[%o0] ASI_AIUS, %o0		! fetch the halfword
	membar	#Sync
	retl				! made it
	 STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

ALTENTRY(fuibyte)
ENTRY(fubyte)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	lduba	[%o0] ASI_AIUS, %o0	! fetch the byte
	membar	#Sync
	retl				! made it
	 STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

ALTENTRY(suiword)
ENTRY(suword)
	btst	3, %o0			! or has low bits set ...
	bnz	Lfsbadaddr		!	go return error
	EMPTY
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	STPTRA	%o1, [%o0] ASI_AIUS	! store the word
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	 clr	%o0

ENTRY(suswintr)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = _Lfsbail;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	_C_LABEL(Lfsbail), %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	stha	%o1, [%o0] ASI_AIUS	! store the halfword
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	 clr	%o0

ENTRY(susword)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	stha	%o1, [%o0] ASI_AIUS	! store the halfword
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	 clr	%o0

ALTENTRY(suibyte)
ENTRY(subyte)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	stba	%o1, [%o0] ASI_AIUS	! store the byte
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	 clr	%o0

/* probeget and probeset are meant to be used during autoconfiguration */
/*
 * The following probably need to be changed, but to what I don't know.
 */

/*
 * probeget(addr, asi, size)
 *	paddr_t addr;
 *	int asi;
 *	int size;
 *
 * Read or write a (byte,word,longword) from the given address.
 * Like {fu,su}{byte,halfword,word} but our caller is supposed
 * to know what he is doing... the address can be anywhere.
 *
 * We optimize for space, rather than time, here.
 */
ENTRY(probeget)
#ifndef _LP64
	!! Shuffle the args around into LP64 format
	sllx	%o0, 32, %o0
	or	%o0, %o1, %o0
	mov	%o2, %o1
	mov	%o3, %o2
#endif
	mov	%o2, %o4
	! %o0 = addr, %o1 = asi, %o4 = (1,2,4)
	sethi	%hi(CPCB), %o2
	LDPTR	[%o2 + %lo(CPCB)], %o2	! cpcb->pcb_onfault = Lfserr;
	set	_C_LABEL(Lfsprobe), %o5
	STPTR	%o5, [%o2 + PCB_ONFAULT]
	or	%o0, 0x9, %o3		! if (PHYS_ASI(asi)) {
	sub	%o3, 0x1d, %o3
	brz,a	%o3, 0f
	 mov	%g0, %o5
	DLFLUSH(%o0,%o5)		!	flush cache line
					! }
0:
#ifndef _LP64
	rdpr	%pstate, %g1
	wrpr	%g1, PSTATE_AM, %pstate
#endif
	btst	1, %o4
	wr	%o1, 0, %asi
	membar	#Sync
	bz	0f			! if (len & 1)
	 btst	2, %o4
	ba,pt	%icc, 1f
	 lduba	[%o0] %asi, %o0		!	value = *(char *)addr;
0:
	bz	0f			! if (len & 2)
	 btst	4, %o4
	ba,pt	%icc, 1f
	 lduha	[%o0] %asi, %o0		!	value = *(short *)addr;
0:
	bz	0f			! if (len & 4)
	 btst	8, %o4
	ba,pt	%icc, 1f
	 lda	[%o0] %asi, %o0		!	value = *(int *)addr;
0:
	ldxa	[%o0] %asi, %o0		!	value = *(long *)addr;
#ifndef _LP64
	srl	%o0, 0, %o1		! Split the result again
	srlx	%o0, 32, %o0
#endif
1:	membar	#Sync
#ifndef _LP64
	wrpr	%g1, 0, %pstate
#endif
	brz	%o5, 1f			! if (cache flush addr != 0)
	 nop
	DLFLUSH2(%o5)			!	flush cache line again
1:
	retl				! made it, clear onfault and return
	 STPTR	%g0, [%o2 + PCB_ONFAULT]

	/*
	 * Fault handler for probeget
	 */
_C_LABEL(Lfsprobe):
#ifndef _LP64
	wrpr	%g1, 0, %pstate
#endif
	STPTR	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	retl				! and return error indicator
	 mov	-1, %o0

/*
 * probeset(addr, asi, size, val)
 *	paddr_t addr;
 *	int asi;
 *	int size;
 *	long val;
 *
 * As above, but we return 0 on success.
 */
ENTRY(probeset)
#ifndef _LP64
	!! Shuffle the args around into LP64 format
	sllx	%o0, 32, %o0
	or	%o0, %o1, %o0
	mov	%o2, %o1
	mov	%o3, %o2
	sllx	%o4, 32, %o3
	or	%o3, %o5, %o3
#endif
	mov	%o2, %o4
	! %o0 = addr, %o1 = asi, %o4 = (1,2,4), %o3 = val
	sethi	%hi(CPCB), %o2		! Lfserr requires CPCB in %o2
	LDPTR	[%o2 + %lo(CPCB)], %o2	! cpcb->pcb_onfault = Lfserr;
	set	_C_LABEL(Lfsbail), %o5
	STPTR	%o5, [%o2 + PCB_ONFAULT]
	btst	1, %o4
	wr	%o1, 0, %asi
	membar	#Sync
	bz	0f			! if (len & 1)
	 btst	2, %o4
	ba,pt	%icc, 1f
	 stba	%o3, [%o0] %asi		!	*(char *)addr = value;
0:
	bz	0f			! if (len & 2)
	 btst	4, %o4
	ba,pt	%icc, 1f
	 stha	%o3, [%o0] %asi		!	*(short *)addr = value;
0:
	bz	0f			! if (len & 4)
	 btst	8, %o4
	ba,pt	%icc, 1f
	 sta	%o3, [%o0] %asi		!	*(int *)addr = value;
0:
	bz	Lfserr			! if (len & 8)
	ba,pt	%icc, 1f
	 sta	%o3, [%o0] %asi		!	*(int *)addr = value;
1:	membar	#Sync
	clr	%o0			! made it, clear onfault and return 0
	retl
	 STPTR	%g0, [%o2 + PCB_ONFAULT]

/*
 * Insert entry into doubly-linked queue.
 * We could just do this in C, but gcc does not do leaves well (yet).
 */
ENTRY(insque)
ENTRY(_insque)
	! %o0 = e = what to insert; %o1 = after = entry to insert after
	STPTR	%o1, [%o0 + PTRSZ]	! e->prev = after;
	LDPTR	[%o1], %o2		! tmp = after->next;
	STPTR	%o2, [%o0]		! e->next = tmp;
	STPTR	%o0, [%o1]		! after->next = e;
	retl
	 STPTR	%o0, [%o2 + PTRSZ]	! tmp->prev = e;


/*
 * Remove entry from doubly-linked queue.
 */
ENTRY(remque)
ENTRY(_remque)
	! %o0 = e = what to remove
	LDPTR	[%o0], %o1		! n = e->next;
	LDPTR	[%o0 + PTRSZ], %o2	! p = e->prev;
	STPTR	%o2, [%o1 + PTRSZ]	! n->prev = p;
	retl
	 STPTR	%o1, [%o2]		! p->next = n;

/*
 * pmap_zero_page(pa)
 *
 * Zero one page physically addressed
 *
 * Block load/store ASIs do not exist for physical addresses,
 * so we won't use them.
 *
 * While we do the zero operation, we also need to blast away
 * the contents of the D$.  We will execute a flush at the end
 * to sync the I$.
 */
	.data
paginuse:
	.word	0
	.text
ENTRY(pmap_zero_page)
	!!
	!! If we have 64-bit physical addresses (and we do now)
	!! we need to move the pointer from %o0:%o1 to %o0
	!!
#ifndef _LP64
#if PADDRT == 8
	sllx	%o0, 32, %o0
	or	%o0, %o1, %o0
#endif
#endif
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	set	2f, %o0
	call	printf
	 mov	%i0, %o1
!	ta	1; nop
	restore
	.data
2:	.asciz	"pmap_zero_page(%p)\n"
	_ALIGN
	.text
3:
#endif
#ifndef PMAP_PHYS_PAGE
/*
 * Here we use VIS instructions to do a block clear of a page.
 * First we will tickle the FPU.  If is was not not enabled this
 * should cause a trap.  The trap will check if they belong to a
 * user process and if so save them and clear %fprs.  It will
 * also enable FP in PSTATE.
 *
 * We may now check the contents of %fprs.  If either the upper
 * or lower FPU is dirty then that means some other kernel routine
 * is using the FPU and we should use the slow routine.
 *
 * Otherwise, we zero out the FP registers we'll use.  Then we map
 * the page into the special VA we use for this purpose.  When we're
 * done, we clear %fprs, so we'll know we can use it the nest time.
 */
	sethi	%hi(_C_LABEL(vmmap)), %o2	! Get VA
	LDPTR	[%o2 + %lo(_C_LABEL(vmmap))], %o2
	brz,pn	%o2, pmap_zero_phys		! Only do VIS if traps are enabled
	 or	%o2, 0x020, %o3			! Nucleus flush page

#ifdef PMAP_FPSTATE
#ifndef NEW_FPSTATE
	!!
	!! This code will allow us to save the fpstate around this
	!! routine and nest FP use in the kernel
	!!
	save	%sp, -(CC64FSZ+FS_SIZE+BLOCK_SIZE), %sp	! Allocate an fpstate
	add	%sp, (CC64FSZ+STKB+BLOCK_SIZE-1), %l0	! Calculate pointer to fpstate
	rd	%fprs, %l1			! Save old fprs so we can restore it later
	andn	%l0, BLOCK_ALIGN, %l0		! And make it block aligned
	call	_C_LABEL(savefpstate)
	 mov	%l0, %o0
	mov	%i0, %o0
	mov	%i2, %o2
	mov	%i3, %o3
	wr	%g0, FPRS_FEF, %fprs
#else
/*
 * New version, new scheme:
 *
 * Here we use VIS instructions to do a block clear of a page.
 * But before we can do that we need to save and enable the FPU.
 * The last owner of the FPU registers is fpproc, and
 * fpproc->p_md.md_fpstate is the current fpstate.  If that's not
 * null, call savefpstate() with it to store our current fp state.
 *
 * Next, allocate an aligned fpstate on the stack.  We will properly
 * nest calls on a particular stack so this should not be a problem.
 *
 * Now we grab either curproc (or if we're on the interrupt stack
 * proc0).  We stash its existing fpstate in a local register and
 * put our new fpstate in curproc->p_md.md_fpstate.  We point
 * fpproc at curproc (or proc0) and enable the FPU.
 *
 * If we are ever preempted, our FPU state will be saved in our
 * fpstate.  Then, when we're resumed and we take an FPDISABLED
 * trap, the trap handler will be able to fish our FPU state out
 * of curproc (or proc0).
 *
 * On exiting this routine we undo the damage: restore the original
 * pointer to curproc->p_md.md_fpstate, clear our fpproc, and disable
 * the MMU.
 *
 */
	!!
	!! This code will allow us to save the fpstate around this
	!! routine and nest FP use in the kernel
	!!
	save	%sp, -(CC64FSZ+FS_SIZE+BLOCK_SIZE), %sp	! Allocate an fpstate
	sethi	%hi(FPPROC), %l1
	LDPTR	[%l1 + %lo(FPPROC)], %l2		! Load fpproc
	add	%sp, (CC64FSZ+STKB+BLOCK_SIZE-1), %l0	! Calculate pointer to fpstate
	brz,pt	%l2, 1f					! fpproc == NULL?
	 andn	%l0, BLOCK_ALIGN, %l0			! And make it block aligned
	LDPTR	[%l2 + P_FPSTATE], %l3
	brz,pn	%l3, 1f					! Make sure we have an fpstate
	 mov	%l3, %o0
	call	_C_LABEL(savefpstate)			! Save the old fpstate
	 set	EINTSTACK, %l4				! Are we on intr stack?
	cmp	%sp, %l4
	bgu,pt	%xcc, 1f
	 set	INTSTACK, %l4
	cmp	%sp, %l4
	blu	%xcc, 1f
0:
	 sethi	%hi(_C_LABEL(proc0)), %l4		! Yes, use proc0
	ba,pt	%xcc, 2f
	 or	%l4, %lo(_C_LABEL(proc0)), %l5
1:
	sethi	%hi(CURPROC), %l4			! Use curproc
	LDPTR	[%l4 + %lo(CURPROC)], %l5
	brz,pn	%l5, 0b					! If curproc is NULL need to use proc0
2:
	mov	%i0, %o0
	mov	%i2, %o2
	LDPTR	[%l5 + P_FPSTATE], %l6			! Save old fpstate
	mov	%i3, %o3
	STPTR	%l0, [%l5 + P_FPSTATE]			! Insert new fpstate
	STPTR	%l5, [%l1 + %lo(FPPROC)]		! Set new fpproc
	wr	%g0, FPRS_FEF, %fprs			! Enable FPU
#endif
#else
	!!
	!! Don't use FP regs if the kernel's already using them
	!!
	rd	%fprs, %o1			! Read old %fprs
	sethi	%hi(FPPROC), %o4		! Also load fpproc
	btst	FPRS_DU|FPRS_DL, %o1		! Is it dirty?
	LDPTR	[%o4 + %lo(FPPROC)], %o4
	bz,pt	%icc, 1f			! No, use fpregs
	 bset	FPRS_FEF, %o1
	brz,pn	%o4, pmap_zero_phys		! No userland fpstate so do this the slow way
1:
	 wr	%o1, 0, %fprs			! Enable the FPU
#endif

#ifdef DEBUG
	sethi	%hi(paginuse), %o4		! Prevent this from nesting
	lduw	[%o4 + %lo(paginuse)], %o5
	tst	%o5
	tnz	%icc, 1
	bnz,pn	%icc, pmap_zero_phys
	 inc	%o5
	stw	%o5, [%o4 + %lo(paginuse)]
#endif

	rdpr	%pil, %g1
	wrpr	%g0, 15, %pil			! s = splhigh()

	fzero	%f0				! Set up FPU
	fzero	%f2
	fzero	%f4
	fzero	%f6
	fzero	%f8
	fzero	%f10
	fzero	%f12
	fzero	%f14
	fzero	%f16				! And second bank
	fzero	%f18
	fzero	%f20
	fzero	%f22
	fzero	%f24
	fzero	%f26
	fzero	%f28
	fzero	%f30

	stxa	%o3, [%o3] ASI_DMMU_DEMAP	! Do the demap
	membar	#Sync				! No real reason for this XXXX

	sethi	%hi(0x80000000), %o4		! Setup TTE:
	sllx	%o4, 32, %o4			!  V = 1
	or	%o4, TTE_CP|TTE_P|TTE_W|TTE_L, %o4	!  CP=1|P=1|W=1|L=1
	or	%o4, %o0, %o4			!  PA

	mov	TLB_TAG_ACCESS, %o5
	stxa	%o2, [%o5] ASI_DMMU		! Store new address for mapping
	membar	#Sync				! No real reason for this XXXX
	stxa	%o4, [%g0] ASI_DMMU_DATA_IN	! Store TTE for new mapping
	membar	#Sync

	set	NBPG, %o4
1:
	stda	%f0, [%o2] ASI_BLK_COMMIT_P		! Store 64 bytes
	add	%o2, 64, %o2
	dec	128, %o4
	stda	%f16, [%o2] ASI_BLK_COMMIT_P		! Store 64 bytes
	brgz,pt %o4, 1b
	 add	%o2, 64, %o2

	membar	#Sync				! Finish the operation
	stxa	%o3, [%o3] ASI_DMMU_DEMAP	! Demap the page again
	membar	#Sync				! No real reason for this XXXX

#ifdef PARANOID
	!!
	!! Use phys accesses to verify page is clear
	!!
	set	NBPG, %o4
1:
	DLFLUSH(%o0,%o2)
	ldxa	[%o0] ASI_PHYS_CACHED, %o1
	DLFLUSH2(%o2)
	dec	8, %o4
	tst	%o1
	tnz	%icc, 1
	brnz,pt	%o4, 1b
	 inc	8, %o0

	sethi	%hi(paginuse), %o4		! Prevent this from nesting
	stw	%g0, [%o4 + %lo(paginuse)]

#endif

	wrpr	%g1, 0, %pil			! splx(s)

#ifdef PMAP_FPSTATE
#ifndef NEW_FPSTATE
	btst	FPRS_DU|FPRS_DL, %l1		! Anything to restore?
	bz,pt	%icc, 1f
	 nop
	call	_C_LABEL(loadfpstate)
	 mov	%l0, %o0
1:
!	return			! Does this work?
	 wr	%l1, 0, %fprs
	ret
	 restore
#else
#ifdef DEBUG
	LDPTR	[%l1 + %lo(FPPROC)], %l7
	cmp	%l7, %l5
	tnz	1		! fpproc has changed!
	LDPTR	[%l5 + P_FPSTATE], %l7
	cmp	%l7, %l0
	tnz	1		! fpstate has changed!
#endif
	STPTR	%g0, [%l1 + %lo(FPPROC)]		! Clear fpproc
	STPTR	%l6, [%l5 + P_FPSTATE]			! Restore old fpstate
	wr	%g0, 0, %fprs				! Disable FPU
	ret
	 restore
#endif
#else
	retl					! Any other mappings have inconsistent D$
	 wr	%g0, 0, %fprs			! Turn off FPU and mark as clean
#endif
pmap_zero_phys:
#endif
#if 1
	set	NBPG, %o2		! Loop count
	clr	%o1
1:
	dec	8, %o2
	stxa	%g0, [%o0] ASI_PHYS_CACHED
	inc	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	brgz	%o2, 1b
	 inc	16, %o1

	sethi	%hi(KERNBASE), %o3
	flush	%o3
	retl
	 nop
#else
	set	NBPG-8, %o1
	add	%o1, %o0, %o1
1:
	stxa	%g0, [%o0] ASI_PHYS_CACHED
	cmp	%o0, %o1
	blt	1b
	 inc	8, %o0
	ba	_C_LABEL(blast_vcache)	! Clear out D$ and return
	 nop
	retl
	 nop
#endif
/*
 * pmap_copy_page(src, dst)
 *
 * Copy one page physically addressed
 * We need to use a global reg for ldxa/stxa
 * so the top 32-bits cannot be lost if we take
 * a trap and need to save our stack frame to a
 * 32-bit stack.  We will unroll the loop by 8 to
 * improve performance.
 *
 * We also need to blast the D$ and flush like
 * pmap_zero_page.
 */
ENTRY(pmap_copy_page)
	!!
	!! If we have 64-bit physical addresses (and we do now)
	!! we need to move the pointer from %o0:%o1 to %o0 and
	!! %o2:%o3 to %o1
	!!
#ifndef _LP64
#if PADDRT == 8
	sllx	%o0, 32, %o0
	or	%o0, %o1, %o0
	sllx	%o2, 32, %o1
	or	%o3, %o1, %o1
#endif
#endif
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	mov	%i0, %o1
	set	2f, %o0
	call	printf
	 mov	%i1, %o2
!	ta	1; nop
	restore
	.data
2:	.asciz	"pmap_copy_page(%p,%p)\n"
	_ALIGN
	.text
3:
#endif
#ifndef PMAP_PHYS_PAGE
/*
 * Here we use VIS instructions to do a block clear of a page.
 * First we zero out the FP registers we'll use.  If they were
 * dirty this should cause a trap to save them.
 * Then we need to turn off interrupts so we don't have to deal
 * with possibly saving or restoring state.  Then we map the page
 * into the special VA we use for this purpose.
 *
 * NB:	 THIS WILL ALWAYS ENABLE INTERRUPTS IN PSTATE ON EXIT
 */
	sethi	%hi(_C_LABEL(vmmap)), %o2	! Get VA
	LDPTR	[%o2 + %lo(_C_LABEL(vmmap))], %o2
	brz,pn	%o2, pmap_copy_phys
	 or	%o2, 0x020, %o3			! Nucleus flush page

#ifdef PMAP_FPSTATE
#ifndef NEW_FPSTATE
	!!
	!! This code will allow us to save the fpstate around this
	!! routine and nest FP use in the kernel
	!!
	save	%sp, -(CC64FSZ+FS_SIZE+BLOCK_SIZE), %sp	! Allocate an fpstate
	add	%sp, (CC64FSZ+STKB+BLOCK_SIZE-1), %l0	! Calculate pointer to fpstate
	andn	%l0, BLOCK_ALIGN, %l0		! And make it block aligned
	rd	%fprs, %l1			! Save old fprs so we can restore it later
	call	_C_LABEL(savefpstate)
	 mov	%l0, %o0
	mov	%i0, %o0
	mov	%i1, %o1
	mov	%i2, %o2
	mov	%i3, %o3
	wr	%g0, FPRS_FEF, %fprs
#else
/*
 * New version, new scheme:
 *
 * Here we use VIS instructions to do a block clear of a page.
 * But before we can do that we need to save and enable the FPU.
 * The last owner of the FPU registers is fpproc, and
 * fpproc->p_md.md_fpstate is the current fpstate.  If that's not
 * null, call savefpstate() with it to store our current fp state.
 *
 * Next, allocate an aligned fpstate on the stack.  We will properly
 * nest calls on a particular stack so this should not be a problem.
 *
 * Now we grab either curproc (or if we're on the interrupt stack
 * proc0).  We stash its existing fpstate in a local register and
 * put our new fpstate in curproc->p_md.md_fpstate.  We point
 * fpproc at curproc (or proc0) and enable the FPU.
 *
 * If we are ever preempted, our FPU state will be saved in our
 * fpstate.  Then, when we're resumed and we take an FPDISABLED
 * trap, the trap handler will be able to fish our FPU state out
 * of curproc (or proc0).
 *
 * On exiting this routine we undo the damage: restore the original
 * pointer to curproc->p_md.md_fpstate, clear our fpproc, and disable
 * the MMU.
 *
 */
	!!
	!! This code will allow us to save the fpstate around this
	!! routine and nest FP use in the kernel
	!!
	save	%sp, -(CC64FSZ+FS_SIZE+BLOCK_SIZE), %sp	! Allocate an fpstate
	sethi	%hi(FPPROC), %l1
	LDPTR	[%l1 + %lo(FPPROC)], %l2		! Load fpproc
	add	%sp, (CC64FSZ+STKB+BLOCK_SIZE-1), %l0	! Calculate pointer to fpstate
	brz,pt	%l2, 1f					! fpproc == NULL?
	 andn	%l0, BLOCK_ALIGN, %l0			! And make it block aligned
	LDPTR	[%l2 + P_FPSTATE], %l3
	brz,pn	%l3, 1f					! Make sure we have an fpstate
	 mov	%l3, %o0
	call	_C_LABEL(savefpstate)			! Save the old fpstate
	 set	EINTSTACK, %l4				! Are we on intr stack?
	cmp	%sp, %l4
	bgu,pt	%xcc, 1f
	 set	INTSTACK, %l4
	cmp	%sp, %l4
	blu	%xcc, 1f
0:
	 sethi	%hi(_C_LABEL(proc0)), %l4		! Yes, use proc0
	ba,pt	%xcc, 2f
	 or	%l4, %lo(_C_LABEL(proc0)), %l5
1:
	sethi	%hi(CURPROC), %l4			! No, use curproc
	LDPTR	[%l4 + %lo(CURPROC)], %l5
	brz,pn	%l5, 0b					! If curproc is NULL need to use proc0
2:
	mov	%i0, %o0
	mov	%i2, %o2
	LDPTR	[%l5 + P_FPSTATE], %l6			! Save old fpstate
	mov	%i3, %o3
	STPTR	%l0, [%l5 + P_FPSTATE]			! Insert new fpstate
	STPTR	%l5, [%l1 + %lo(FPPROC)]		! Set new fpproc
	wr	%g0, FPRS_FEF, %fprs			! Enable FPU
#endif
#else
	!!
	!! Don't use FP regs if the kernel's already using them
	!!
	rd	%fprs, %o5			! Read old %fprs
	sethi	%hi(FPPROC), %o4		! Also load fpproc
	btst	FPRS_DU|FPRS_DL, %o5		! Is it dirty?
	LDPTR	[%o4 + %lo(FPPROC)], %o4
	bz,pt	%icc, 1f			! No, use fpregs
	 bset	FPRS_FEF, %o5
	brz,pn	%o4, pmap_copy_phys		! No userland fpstate so do this the slow way
1:
	 wr	%o5, 0, %fprs			! Enable the FPU
#endif

#ifdef DEBUG
	sethi	%hi(paginuse), %o4		! Prevent this from nesting
	lduw	[%o4 + %lo(paginuse)], %o5
	tst	%o5
	tnz	%icc, 1
	bnz,pn	%icc, pmap_copy_phys
	 inc	%o5
	stw	%o5, [%o4 + %lo(paginuse)]
#endif

	rdpr	%pil, %g1
	wrpr	%g0, 15, %pil			! s = splhigh();

	stxa	%o3, [%o3] ASI_DMMU_DEMAP	! Do the demap
	sethi	%hi(NBPG), %o4
	membar	#Sync				! No real reason for this XXXX
	add	%o3, %o4, %o3
	stxa	%o3, [%o3] ASI_DMMU_DEMAP	! Demap the next page too
	membar	#Sync				! No real reason for this XXXX

	sethi	%hi(0x80000000), %o4		! Setup TTE:
	sllx	%o4, 32, %o4			!  V = 1
	or	%o4, TTE_CP|TTE_P|TTE_W|TTE_L, %o4	!  CP=1|P=1|W=1|L=1
	or	%o4, %o0, %o0			! TTE for source page XXX Should be RO
	or	%o4, %o1, %o1			! TTE for dest page

	mov	TLB_TAG_ACCESS, %o5
	stxa	%o2, [%o5] ASI_DMMU		! Store new address for mapping
	membar	#Sync				! No real reason for this XXXX
	stxa	%o0, [%g0] ASI_DMMU_DATA_IN	! Store TTE for new mapping
	membar	#Sync

	sethi	%hi(NBPG), %o4
	add	%o2, %o4, %o4			! %o4 point to dest
	stxa	%o4, [%o5] ASI_DMMU		! Store new address for mapping
	membar	#Sync				! No real reason for this XXXX
	stxa	%o1, [%g0] ASI_DMMU_DATA_IN	! Store TTE for new mapping
	membar	#Sync

	set	NBPG, %o5			! # bytes to move

	ldda	[%o2] ASI_BLK_P, %f0		! Load 1st bank
	dec	64, %o5
	add	%o2, 64, %o2
1:
	membar	#StoreLoad
	ldda	[%o2] ASI_BLK_P, %f16		! Load 2nd bank
	dec	64, %o5
	add	%o2, 64, %o2

	membar	#LoadStore
	fmovd	%f14, %f14			! Sync 1st bank
	stda	%f0, [%o4] ASI_BLK_COMMIT_P	! Store 1st bank
	brlez,pn	%o5, 1f			! Finished?
	 add	%o4, 64, %o4

	membar	#StoreLoad
	ldda	[%o2] ASI_BLK_P, %f0		! Load 1st bank
	dec	64, %o5
	add	%o2, 64, %o2

	membar	#LoadStore
	fmovd	%f30, %f30			! Sync 2nd bank
	stda	%f16, [%o4] ASI_BLK_COMMIT_P	! Store 2nd bank
	brgz,pt	%o5, 1b				! Finished?
	 add	%o4, 64, %o4

	!!
	!! If we got here we have loaded bank 1 and stored bank 2
	!!
	membar	#LoadStore
	fmovd	%f14, %f14			! Sync 1st bank
	stda	%f0, [%o4] ASI_BLK_COMMIT_P	! Store 1st bank
	ba,pt	%icc, 2f			! Finished?
	 add	%o4, 64, %o4

1:
	!!
	!! If we got here we have loaded bank 2 and stored bank 1
	!!
	membar	#LoadStore
	fmovd	%f30, %f30			! Sync 2nd bank
	stda	%f16, [%o4] ASI_BLK_COMMIT_P	! Store 2nd bank
	add	%o4, 64, %o4

2:
	membar	#Sync				! Finish the operation
	stxa	%o3, [%o3] ASI_DMMU_DEMAP	! Demap the dest page again
	sethi	%hi(NBPG), %o4
	membar	#Sync				! No real reason for this XXXX
	sub	%o3, %o4, %o3
	stxa	%o3, [%o3] ASI_DMMU_DEMAP	! Demap the source page again
	membar	#Sync				! No real reason for this XXXX

#ifdef PARANOID
	!!
	!! Use phys accesses to verify copy
	!!
	sethi	%hi(0x80000000), %o4		! Setup TTE:
	sllx	%o4, 32, %o4			!  V = 1
	or	%o4, TTE_CP|TTE_P|TTE_W|TTE_L, %o4	!  CP=1|P=1|W=1|L=1
	andn	%o1, %o4, %o0			! Clear out TTE to get PADDR
	andn	%o1, %o4, %o1			! Clear out TTE to get PADDR

	set	NBPG, %o3

1:
	DLFLUSH(%o0,%o2)
	ldxa	[%o0] ASI_PHYS_CACHED, %o4
	DLFLUSH2(%o2)
	DLFLUSH(%o1,%o2)
	ldxa	[%o1] ASI_PHYS_CACHED, %o5
	DLFLUSH2(%o2)
	dec	8, %o3
	cmp	%o4, %o5
	tne	%icc, 1
	inc	8, %o0
	brnz,pt	%o4, 1b
	 inc	8, %o1

	sethi	%hi(paginuse), %o4		! Prevent this from nesting
	stw	%g0, [%o4 + %lo(paginuse)]
#endif

	wrpr	%g1, 0, %pil			! splx(s)

#ifdef PMAP_FPSTATE
#ifndef NEW_FPSTATE
	btst	FPRS_DU|FPRS_DL, %l1		! Anything to restore?
	bz,pt	%icc, 1f
	 nop
	call	_C_LABEL(loadfpstate)
	 mov	%l0, %o0
1:
!	return			! Does this work?
	 wr	%l1, 0, %fprs
	ret
	 restore
#else
#ifdef DEBUG
	LDPTR	[%l1 + %lo(FPPROC)], %l7
	cmp	%l7, %l5
	tnz	1		! fpproc has changed!
	LDPTR	[%l5 + P_FPSTATE], %l7
	cmp	%l7, %l0
	tnz	1		! fpstate has changed!
#endif
	STPTR	%g0, [%l1 + %lo(FPPROC)]		! Clear fpproc
	STPTR	%l6, [%l5 + P_FPSTATE]			! Save old fpstate
	wr	%g0, 0, %fprs				! Disable FPU
	ret
	 restore
#endif
#else
	ba	_C_LABEL(blast_vcache)
	 wr	%g0, 0, %fprs			! Turn off FPU and mark as clean
	
	retl					! Any other mappings have inconsistent D$
	 wr	%g0, 0, %fprs			! Turn off FPU and mark as clean
#endif
pmap_copy_phys:
#endif
#if 0
#if 0
	save	%sp, -CC64FSZ, %sp	! Get 8 locals for scratch
	set	NBPG, %o1
	sub	%o1, 8, %o0
1:
	ldxa	[%i0] ASI_PHYS_CACHED, %l0
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l1
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l2
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l3
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l4
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l5
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l6
	inc	8, %i0
	ldxa	[%i0] ASI_PHYS_CACHED, %l7
	inc	8, %i0
	stxa	%l0, [%i1] ASI_PHYS_CACHED
	inc	8, %i1
	stxa	%l1, [%i1] ASI_PHYS_CACHED
	inc	8, %i1
	stxa	%l2, [%i1] ASI_PHYS_CACHED
	inc	8,%i1
	stxa	%l3, [%i1] ASI_PHYS_CACHED
	inc	8, %i1
	stxa	%l4, [%i1] ASI_PHYS_CACHED
	inc	8, %i1
	stxa	%l5, [%i1] ASI_PHYS_CACHED
	inc	8, %i1
	stxa	%l6, [%i1] ASI_PHYS_CACHED
	inc	8, %i1
	stxa	%l7, [%i1] ASI_PHYS_CACHED
	inc	8, %i1

	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	inc	8, %o1
	stxa	%g0, [%o0] ASI_DCACHE_TAG! Blast away at the D$
	dec	8, %o0
	stxa	%g0, [%o1] ASI_DCACHE_TAG
	brnz,pt	%o0, 1b
	 inc	8, %o1
	sethi	%hi(KERNBASE), %o2
	flush	%o2
	return
	 nop
#else
	/* This is the short, slow, safe version that uses %g1 */

	set	NBPG, %o3
	clr	%o2
	mov	%g1, %o4		! Save g1
1:
	DLFLUSH(%o0,%g1)
	ldxa	[%o0] ASI_PHYS_CACHED, %g1
	inc	8, %o0
	stxa	%g1, [%o1] ASI_PHYS_CACHED
	inc	8, %o1

	dec	8, %o3
	stxa	%g0, [%o2] ASI_DCACHE_TAG! Blast away at the D$
	brnz,pt	%o3, 1b
	 inc	16, %o2
	mov	%o4, %g1
	sethi	%hi(KERNBASE), %o5
	flush	%o5
	retl
	 nop
#endif
#else
	set	NBPG, %o3
	add	%o3, %o0, %o3
	mov	%g1, %o4		! Save g1
1:
	DLFLUSH(%o0,%g1)
	ldxa	[%o0] ASI_PHYS_CACHED, %g1
	inc	8, %o0
	cmp	%o0, %o3
	stxa	%g1, [%o1] ASI_PHYS_CACHED
	DLFLUSH(%o1,%g1)
	bl,pt	%icc, 1b		! We don't care about pages >4GB
	 inc	8, %o1
#if 0
	ba	_C_LABEL(blast_vcache)	! Clear out D$ and return
	 mov	%o4, %g1		! Restore g1
#endif
	retl
	 mov	%o4, %g1		! Restore g1
#endif

/*
 * extern int64_t pseg_get(struct pmap* %o0, vaddr_t addr %o1);
 *
 * Return TTE at addr in pmap.  Uses physical addressing only.
 * pmap->pm_physaddr must by the physical address of pm_segs
 *
 */
ENTRY(pseg_get)
!	flushw			! Make sure we don't have stack probs & lose hibits of %o
	ldx	[%o0 + PM_PHYS], %o2			! pmap->pm_segs

	srax	%o1, HOLESHIFT, %o3			! Check for valid address
	brz,pt	%o3, 0f					! Should be zero or -1
	 inc	%o3					! Make -1 -> 0
	brnz,pn	%o3, 1f					! Error! In hole!
0:
	srlx	%o1, STSHIFT, %o3
	and	%o3, STMASK, %o3			! Index into pm_segs
	sll	%o3, 3, %o3
	add	%o2, %o3, %o2
	DLFLUSH(%o2,%o3)
	ldxa	[%o2] ASI_PHYS_CACHED, %o2		! Load page directory pointer
	DLFLUSH2(%o3)

	srlx	%o1, PDSHIFT, %o3
	and	%o3, PDMASK, %o3
	sll	%o3, 3, %o3
	brz,pn	%o2, 1f					! NULL entry? check somewhere else
	 add	%o2, %o3, %o2
	DLFLUSH(%o2,%o3)
	ldxa	[%o2] ASI_PHYS_CACHED, %o2		! Load page table pointer
	DLFLUSH2(%o3)

	srlx	%o1, PTSHIFT, %o3			! Convert to ptab offset
	and	%o3, PTMASK, %o3
	sll	%o3, 3, %o3
	brz,pn	%o2, 1f					! NULL entry? check somewhere else
	 add	%o2, %o3, %o2
	DLFLUSH(%o2,%o3)
	ldxa	[%o2] ASI_PHYS_CACHED, %o0
	DLFLUSH2(%o3)
	brgez,pn %o0, 1f				! Entry invalid?  Punt
	 btst	1, %sp
	bz,pn	%icc, 0f				! 64-bit mode?
	 nop
	retl						! Yes, return full value
	 nop
0:
#if 1
	srl	%o0, 0, %o1
	retl						! No, generate a %o0:%o1 double
	 srlx	%o0, 32, %o0
#else
	DLFLUSH(%o2,%o3)
	ldda	[%o2] ASI_PHYS_CACHED, %o0
	DLFLUSH2(%o3)
	retl						! No, generate a %o0:%o1 double
	 nop
#endif
1:
	clr	%o1
	retl
	 clr	%o0

/*
 * In 32-bit mode:
 *
 * extern void pseg_set(struct pmap* %o0, vaddr_t addr %o1, int64_t tte %o2:%o3, paddr_t spare %o4:%o5);
 *
 * In 64-bit mode:
 *
 * extern void pseg_set(struct pmap* %o0, vaddr_t addr %o1, int64_t tte %o2, paddr_t spare %o3);
 *
 * Set a pseg entry to a particular TTE value.  Returns 0 on success, 1 if it needs to fill
 * a pseg, and -1 if the address is in the virtual hole.  (NB: nobody in pmap checks for the
 * virtual hole, so the system will hang.)  Allocate a page, pass the phys addr in as the spare,
 * and try again.  If spare is not NULL it is assumed to be the address of a zeroed physical page
 * that can be used to generate a directory table or page table if needed.
 *
 */
ENTRY(pseg_set)
#ifndef _LP64
	btst	1, %sp					! 64-bit mode?
	bnz,pt	%icc, 0f
	 sllx	%o4, 32, %o4				! Put args into 64-bit format

	sllx	%o2, 32, %o2				! Shift to high 32-bits
	sll	%o3, 0, %o3				! Zero extend
	sll	%o5, 0, %o5
	sll	%o1, 0, %o1
	or	%o2, %o3, %o2
	or	%o4, %o5, %o3
0:
#endif
#ifdef NOT_DEBUG
	!! Trap any changes to pmap_kernel below 0xf0000000
	set	_C_LABEL(kernel_pmap_), %o5
	cmp	%o0, %o5
	bne	0f
	 sethi	%hi(0xf0000000), %o5
	cmp	%o1, %o5
	tlu	1
0:
#endif
	!!
	!! However we managed to get here we now have:
	!!
	!! %o0 = *pmap
	!! %o1 = addr
	!! %o2 = tte
	!! %o3 = spare
	!!
	srax	%o1, HOLESHIFT, %o4			! Check for valid address
	brz,pt	%o4, 0f					! Should be zero or -1
	 inc	%o4					! Make -1 -> 0
	brz,pt	%o4, 0f
	 nop
#ifdef DEBUG
	ta	1					! Break into debugger
#endif
	mov	-1, %o0					! Error -- in hole!
	retl
	 mov	-1, %o1
0:
	ldx	[%o0 + PM_PHYS], %o4			! pmap->pm_segs
	srlx	%o1, STSHIFT, %o5
	and	%o5, STMASK, %o5
	sll	%o5, 3, %o5
	add	%o4, %o5, %o4
	DLFLUSH(%o4,%g1)
	ldxa	[%o4] ASI_PHYS_CACHED, %o5		! Load page directory pointer
	DLFLUSH2(%g1)

	brnz,a,pt	%o5, 0f				! Null pointer?
	 mov	%o5, %o4
	brz,pn	%o3, 1f					! Have a spare?
	 nop
	stxa	%o3, [%o4] ASI_PHYS_CACHED
	DLFLUSH(%o4, %o4)
	mov	%o3, %o4
	clr	%o3					! Mark spare as used
0:
	srlx	%o1, PDSHIFT, %o5
	and	%o5, PDMASK, %o5
	sll	%o5, 3, %o5
	add	%o4, %o5, %o4
	DLFLUSH(%o4,%g1)
	ldxa	[%o4] ASI_PHYS_CACHED, %o5		! Load table directory pointer
	DLFLUSH2(%g1)

	brnz,a,pt	%o5, 0f				! Null pointer?
	 mov	%o5, %o4
	brz,pn	%o3, 1f					! Have a spare?
	 nop
	stxa	%o3, [%o4] ASI_PHYS_CACHED
	DLFLUSH(%o4, %o4)
	mov	%o3, %o4
	clr	%o3					! Mark spare as used
0:
	srlx	%o1, PTSHIFT, %o5			! Convert to ptab offset
	and	%o5, PTMASK, %o5
	sll	%o5, 3, %o5
	add	%o5, %o4, %o4
	stxa	%o2, [%o4] ASI_PHYS_CACHED		! Easier than shift+or
	DLFLUSH(%o4, %o4)
#ifdef PARANOID
	!! Try pseg_get to verify we did this right
	mov	%o7, %o4
	call	pseg_get
	 mov	%o2, %o5
#ifndef _LP64
	sllx	%o0, 32, %o0
	or	%o1, %o0, %o0
#endif
	cmp	%o0, %o5
	tne	1
	mov	%o4, %o7
#endif
	retl
	 clr	%o0
1:
	retl
	 mov	1, %o0

/*
 * copywords(src, dst, nbytes)
 *
 * Copy `nbytes' bytes from src to dst, both of which are word-aligned;
 * nbytes is a multiple of four.  It may, however, be zero, in which case
 * nothing is to be copied.
 */
ENTRY(copywords)
	! %o0 = src, %o1 = dst, %o2 = nbytes
	b	1f
	 deccc	4, %o2
0:
	st	%o3, [%o1 + %o2]
	deccc	4, %o2			! while ((n -= 4) >= 0)
1:
	bge,a	0b			!    *(int *)(dst+n) = *(int *)(src+n);
	 ld	[%o0 + %o2], %o3
	retl
	 nop

/*
 * qcopy(src, dst, nbytes)
 *
 * (q for `quad' or `quick', as opposed to b for byte/block copy)
 *
 * Just like copywords, but everything is multiples of 8.
 */
ENTRY(qcopy)
	ba,pt	%icc, 1f
	 deccc	8, %o2
0:
	stx	%g1, [%o1 + %o2]
	deccc	8, %o2
1:
	bge,a,pt	%icc, 0b
	 ldx	[%o0 + %o2], %g1
	retl
	nop

/*
 * qzero(addr, nbytes)
 *
 * Zeroes `nbytes' bytes of a quad-aligned virtual address,
 * where nbytes is itself a multiple of 8.
 */
ENTRY(qzero)
	! %o0 = addr, %o1 = len (in bytes)
0:
	deccc	8, %o1			! while ((n =- 8) >= 0)
	bge,a,pt	%icc, 0b
	 stx	%g0, [%o0 + %o1]	!	*(quad *)(addr + n) = 0;
	retl
	nop

#if 1
/*
 * kernel bcopy/memcpy
 * Assumes regions do not overlap; has no useful return value.
 *
 * Must not use %g7 (see copyin/copyout above).
 */
ENTRY(memcpy) /* dest, src, size */
	/*
	 * Swap args for bcopy.  Gcc generates calls to memcpy for
	 * structure assignments.
	 */
	mov	%o0, %o3
	mov	%o1, %o0
	mov	%o3, %o1
#endif
ENTRY(bcopy) /* src, dest, size */
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	mov	%i0, %o1
	set	2f, %o0
	mov	%i1, %o2
	call	printf
	 mov	%i2, %o3
!	ta	1; nop
	restore
	.data
2:	.asciz	"bcopy(%p->%p,%x)\n"
	_ALIGN
	.text
3:
#endif
	cmp	%o2, BCOPY_SMALL
Lbcopy_start:
	bge,a	Lbcopy_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 EMPTY
0:
	inc	%o0
	ldsb	[%o0 - 1], %o4	!	(++dst)[-1] = *src++;
	stb	%o4, [%o1]
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	retl
	 nop
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lbcopy_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 EMPTY
	btst	7, %o1
	be,a	Lbcopy_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto bcopy_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsb	[%o0], %o4	!	do {
	inc	%o0		!		(++dst)[-1] = *src++;
	inc	%o1
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 stb	%o4, [%o1 - 1]
	retl
	 nop
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	stb	%o4, [%o1]
	inc	%o0
	inc	%o1
	dec	%o2		!	len--;
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsh	[%o0], %o4	!	do {
	sth	%o4, [%o1]	!		*(short *)dst = *(short *)src;
	inc	2, %o0		!		dst += 2, src += 2;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lbcopy_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsh	[%o0], %o4	!	(*short *)dst = *(short *)src;
	sth	%o4, [%o1]
	inc	2, %o0		!	dst += 2;
	inc	2, %o1		!	src += 2;
	dec	2, %o2		!	len -= 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	ld	[%o0], %o4	!	do {
	st	%o4, [%o1]	!		*(int *)dst = *(int *)src;
	inc	4, %o0		!		dst += 4, src += 4;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lbcopy_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	st	%o4, [%o1]
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	inc	4, %o1
	dec	4, %o2		! }
1:
Lbcopy_doubles:
	ldx	[%o0], %g5	! do {
	stx	%g5, [%o1]	!	*(double *)dst = *(double *)src;
	inc	8, %o0		!	dst += 8, src += 8;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lbcopy_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lbcopy_done	!	goto bcopy_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lbcopy_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	st	%o4, [%o1]
	inc	4, %o0		!	dst += 4;
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lbcopy_mopw:
	be	Lbcopy_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsh	[%o0], %o4	! *(short *)dst = *(short *)src;
	be	Lbcopy_done	! if ((len & 1) == 0) goto done;
	 sth	%o4, [%o1]
	ldsb	[%o0 + 2], %o4	! dst[2] = src[2];
	retl
	 stb	%o4, [%o1 + 2]
	NOTREACHED

	! mop up trailing byte (if present).
Lbcopy_mopb:
	bne,a	1f
	 ldsb	[%o0], %o4

Lbcopy_done:
	retl
	 nop
1:
	retl
	 stb	%o4,[%o1]

#if 1
/*
 * XXXXXXXXXXXXXXXXXXXX
 * We need to make sure that this doesn't use floating point
 * before our trap handlers are installed or we could panic
 * XXXXXXXXXXXXXXXXXXXX
 */
/*
 * memset(addr, c, len)
 *
 * Duplicate the pattern so it fills 64-bits, then swap around the
 * arguments and call bzero.
 */
ENTRY(memset)
	and	%o1, 0x0ff, %o3
	mov	%o2, %o1
	sllx	%o3, 8, %o2
	or	%o2, %o3, %o2
	mov	%o0, %o4		! Save original pointer
	sllx	%o2, 16, %o3
	or	%o2, %o3, %o2
	sllx	%o2, 32, %o3
	ba,pt	%icc, Lbzero_internal
	 or	%o2, %o3, %o2
/*
 * bzero(addr, len)
 *
 * We want to use VIS instructions if we're clearing out more than
 * 256 bytes, but to do that we need to properly save and restore the
 * FP registers.  Unfortunately the code to do that in the kernel needs
 * to keep track of the current owner of the FPU, hence the different
 * code.
 *
 */
ENTRY(bzero)
	! %o0 = addr, %o1 = len
	clr	%o2			! Initialize our pattern
Lbzero_internal:
#ifndef _LP64
#ifdef	DEBUG
	tst	%o1			! DEBUG
	tneg	%icc, 1			! DEBUG -- trap if negative.
#endif
	sra	%o1, 0, %o1		! Sign extend 32-bits
#endif
	brlez,pn	%o1, Lbzero_done	! No bytes to copy??
!	 cmp	%o1, 8			! Less than 8 bytes to go?
!	ble,a,pn	%icc, Lbzero_small	! Do it byte at a time.
!	 deccc	8, %o1			! pre-decrement

	 btst	7, %o0			! 64-bit aligned?  Optimization
	bz,pt	%xcc, 2f
	 btst	3, %o0			! 32-bit aligned?
	bz,pt	%xcc, 1f
	 btst	1, %o0			! 16-bit aligned?
	bz,pt	%xcc, 0f
	 btst	3, %o0

	!! unaligned -- store 1 byte
	stb	%o2, [%o0]
	dec	1, %o1			! Record storing 1 byte
	inc	%o0
	cmp	%o1, 2
	bl,a,pn	%icc, 7f		! 1 or 0 left
	 dec	8, %o1			! Fixup count -8
0:
	btst	3, %o0
	bz,pt	%xcc, 1f
	 btst	7, %o0			! 64-bit aligned?

	!! 16-bit aligned -- store half word
	sth	%o2, [%o0]
	dec	2, %o1			! Prepare to store 2 bytes
	inc	2, %o0
	cmp	%o1, 4
	bl,a,pn	%icc, 5f		! Less than 4 left
	 dec	8, %o1			! Fixup count -8
1:
	btst	7, %o0			! 64-bit aligned?
	bz,pt	%xcc, 2f
	 nop
	!! 32-bit aligned -- store word
	stw	%o2, [%o0]
	dec	4, %o1
	inc	4, %o0
	cmp	%o1, 8
	bl,a,pn	%icc, Lbzero_cleanup	! Less than 8 left
	 dec	8, %o1			! Fixup count -8
2:
	!! Now we're 64-bit aligned
	cmp	%o1, 256		! Use block clear if len > 256
	bge,pt	%xcc, Lbzero_block	! use block store insns
	 deccc	8, %o1
Lbzero_longs:
	bl,pn	%xcc, Lbzero_cleanup	! Less than 8 bytes left
	 nop
3:
	stx	%o2, [%o0]		! Do 1 longword at a time
	deccc	8, %o1
	bge,pt	%xcc, 3b
	 inc	8, %o0

	/*
	 * Len is in [-8..-1] where -8 => done, -7 => 1 byte to zero,
	 * -6 => two bytes, etc.  Mop up this remainder, if any.
	 */
Lbzero_cleanup:
	btst	4, %o1
	bz,pt	%xcc, 6f		! if (len & 4) {
	 btst	2, %o1
	stw	%o2, [%o0]		!	*(int *)addr = 0;
	inc	4, %o0			!	addr += 4;
5:
	btst	2, %o1
6:
	bz,pt	%xcc, 8f		! if (len & 2) {
	 btst	1, %o1
	sth	%o2, [%o0]		!	*(short *)addr = 0;
	inc	2, %o0			!	addr += 2;
7:
	btst	1, %o1
8:
	bnz,a	%icc, Lbzero_done	! if (len & 1)
	 stb	%o2, [%o0]		!	*addr = 0;
Lbzero_done:
	retl
	 mov	%o4, %o0		! Restore ponter for memset (ugh)

	/*
	 * Len is in [-8..-1] where -8 => done, -7 => 1 byte to zero,
	 * -6 => two bytes, etc. but we're potentially unaligned.
	 * Do byte stores since it's easiest.
	 */
Lbzero_small:
	inccc	8, %o1
	bz,pn	%icc, Lbzero_done
1:
	 deccc	%o1
	stb	%o2, [%o0]
	bge,pt	%icc, 1b
	 inc	%o0
	ba,a,pt	%icc, Lbzero_done
	 nop				! XXX spitfire bug?

Lbzero_block:
	!! Make sure our trap table is installed
	rdpr	%tba, %o3
	set	_C_LABEL(trapbase), %o5
	sub	%o3, %o5, %o3
	brnz,pn	%o3, Lbzero_longs	! No, then don't use block load/store
	 nop
/*
 * Kernel:
 *
 * Here we use VIS instructions to do a block clear of a page.
 * But before we can do that we need to save and enable the FPU.
 * The last owner of the FPU registers is fpproc, and
 * fpproc->p_md.md_fpstate is the current fpstate.  If that's not
 * null, call savefpstate() with it to store our current fp state.
 *
 * Next, allocate an aligned fpstate on the stack.  We will properly
 * nest calls on a particular stack so this should not be a problem.
 *
 * Now we grab either curproc (or if we're on the interrupt stack
 * proc0).  We stash its existing fpstate in a local register and
 * put our new fpstate in curproc->p_md.md_fpstate.  We point
 * fpproc at curproc (or proc0) and enable the FPU.
 *
 * If we are ever preempted, our FPU state will be saved in our
 * fpstate.  Then, when we're resumed and we take an FPDISABLED
 * trap, the trap handler will be able to fish our FPU state out
 * of curproc (or proc0).
 *
 * On exiting this routine we undo the damage: restore the original
 * pointer to curproc->p_md.md_fpstate, clear our fpproc, and disable
 * the MMU.
 *
 */

	!!
	!! This code will allow us to save the fpstate around this
	!! routine and nest FP use in the kernel
	!!
	save	%sp, -(CC64FSZ+FS_SIZE+BLOCK_SIZE), %sp	! Allocate an fpstate
	sethi	%hi(FPPROC), %l1
	LDPTR	[%l1 + %lo(FPPROC)], %l2		! Load fpproc
	btst	1, %sp
	add	%sp, (CC64FSZ+BLOCK_SIZE-1), %l0	! Calculate pointer to fpstate
	add	%l0, BIAS, %l3
	movnz	%xcc, %l3, %l0
	brz,pt	%l2, 1f					! fpproc == NULL?
	 andn	%l0, BLOCK_ALIGN, %l0			! And make it block aligned
	LDPTR	[%l2 + P_FPSTATE], %l3
	brz,pn	%l3, 1f					! Make sure we have an fpstate
	 mov	%l3, %o0
	call	_C_LABEL(savefpstate)			! Save the old fpstate
	 set	EINTSTACK, %l4				! Are we on intr stack?
	cmp	%sp, %l4
	bgu,pt	%xcc, 1f
	 set	INTSTACK, %l4
	cmp	%sp, %l4
	blu	%xcc, 1f
0:
	 sethi	%hi(_C_LABEL(proc0)), %l4		! Yes, use proc0
	ba,pt	%xcc, 2f				! XXXX needs to change to CPU's idle proc
	 or	%l4, %lo(_C_LABEL(proc0)), %l5
1:
	sethi	%hi(CURPROC), %l4			! Use curproc
	LDPTR	[%l4 + %lo(CURPROC)], %l5
	brz,pn	%l5, 0b					! If curproc is NULL need to use proc0
2:
	mov	%i0, %o0
	mov	%i2, %o2
	LDPTR	[%l5 + P_FPSTATE], %l6			! Save old fpstate
	mov	%i3, %o3
	STPTR	%l0, [%l5 + P_FPSTATE]			! Insert new fpstate
	STPTR	%l5, [%l1 + %lo(FPPROC)]		! Set new fpproc
	wr	%g0, FPRS_FEF, %fprs			! Enable FPU

	!! We are now 8-byte aligned.  We need to become 64-byte aligned.
	btst	63, %i0
	bz,pt	%xcc, 2f
	 nop
1:
	stx	%i2, [%i0]
	inc	8, %i0
	btst	63, %i0
	bnz,pt	%xcc, 1b
	 dec	8, %i1

2:
	brz,pt	%i2, 4f					! Do we have a pattern to load?
	 fzero	%f0					! Set up FPU

	btst	1, %fp
	bnz,pt	%icc, 3f				! 64-bit stack?
	 nop
	stw	%i2, [%fp + 0x28]			! Flush this puppy to RAM
	membar	#StoreLoad
	ld	[%fp + 0x28], %f0
	ba,pt	%icc, 4f
	 fmovsa	%icc, %f0, %f1
3:
	stx	%i2, [%fp + BIAS + 0x50]		! Flush this puppy to RAM
	membar	#StoreLoad
	ldd	[%fp + BIAS + 0x50], %f0
4:
	fmovda	%icc, %f0, %f2				! Duplicate the pattern
	fmovda	%icc, %f0, %f4
	fmovda	%icc, %f0, %f6
	fmovda	%icc, %f0, %f8
	fmovda	%icc, %f0, %f10
	fmovda	%icc, %f0, %f12
	fmovda	%icc, %f0, %f14
	fmovda	%icc, %f0, %f16				! And second bank
	fmovda	%icc, %f0, %f18
	fmovda	%icc, %f0, %f20
	fmovda	%icc, %f0, %f22
	fmovda	%icc, %f0, %f24
	fmovda	%icc, %f0, %f26
	fmovda	%icc, %f0, %f28
	fmovda	%icc, %f0, %f30

	!! Remember: we were 8 bytes too far
	dec	56, %i1			! Go one iteration too far
5:
	stda	%f0, [%i0] ASI_BLK_COMMIT_P		! Store 64 bytes
	deccc	64, %i1
	ble,pn	%xcc, 6f
	 inc	64, %i0

	stda	%f0, [%i0] ASI_BLK_COMMIT_P		! Store 64 bytes
	deccc	64, %i1
	bg,pn	%xcc, 5b
	 inc	64, %i0
6:
/*
 * We've saved our possible fpstate, now disable the fpu
 * and continue with life.
 */
#ifdef DEBUG
	LDPTR	[%l1 + %lo(FPPROC)], %l7
	cmp	%l7, %l5
!	tnz	1		! fpproc has changed!
	LDPTR	[%l5 + P_FPSTATE], %l7
	cmp	%l7, %l0
	tnz	1		! fpstate has changed!
#endif
	STPTR	%g0, [%l1 + %lo(FPPROC)]		! Clear fpproc
	STPTR	%l6, [%l5 + P_FPSTATE]			! Restore old fpstate
	wr	%g0, 0, %fprs				! Disable FPU
	addcc	%i1, 56, %i1	! Restore the count
	ba,pt	%xcc, Lbzero_longs	! Finish up the remainder
	 restore
#endif

/*
 * kcopy() is exactly like bcopy except that it set pcb_onfault such that
 * when a fault occurs, it is able to return -1 to indicate this to the
 * caller.
 */
ENTRY(kcopy)
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	mov	%i0, %o1
	set	2f, %o0
	mov	%i1, %o2
	call	printf
	 mov	%i2, %o3
!	ta	1; nop
	restore
	.data
2:	.asciz	"kcopy(%p->%p,%x)\n"
	_ALIGN
	.text
3:
#endif
	sethi	%hi(CPCB), %o5		! cpcb->pcb_onfault = Lkcerr;
	LDPTR	[%o5 + %lo(CPCB)], %o5
	set	Lkcerr, %o3
	LDPTR	[%o5 + PCB_ONFAULT], %g1! save current onfault handler
	STPTR	%o3, [%o5 + PCB_ONFAULT]

	cmp	%o2, BCOPY_SMALL
Lkcopy_start:
	bge,a	Lkcopy_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 EMPTY
0:
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	inc	%o0
	stb	%o4, [%o1]
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl
	 clr	%o0
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lkcopy_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 EMPTY
	btst	7, %o1
	be,a	Lkcopy_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto kcopy_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsb	[%o0], %o4	!	do {
	inc	%o0		!		*dst++ = *src++;
	stb	%o4, [%o1]
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 inc	%o1
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl
	 clr	%o0
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	inc	%o0
	stb	%o4, [%o1]
	dec	%o2		!	len--;
	inc	%o1
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsh	[%o0], %o4	!	do {
	inc	2, %o0		!		dst += 2, src += 2;
	sth	%o4, [%o1]	!		*(short *)dst = *(short *)src;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lkcopy_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsh	[%o0], %o4	!	(*short *)dst = *(short *)src;
	inc	2, %o0		!	dst += 2;
	sth	%o4, [%o1]
	dec	2, %o2		!	len -= 2;
	inc	2, %o1		!	src += 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	ld	[%o0], %o4	!	do {
	inc	4, %o0		!		dst += 4, src += 4;
	st	%o4, [%o1]	!		*(int *)dst = *(int *)src;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lkcopy_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	st	%o4, [%o1]
	dec	4, %o2		! }
	inc	4, %o1
1:
Lkcopy_doubles:
	ldx	[%o0], %g5	! do {
	inc	8, %o0		!	dst += 8, src += 8;
	stx	%g5, [%o1]	!	*(double *)dst = *(double *)src;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lkcopy_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lkcopy_done	!	goto kcopy_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lkcopy_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	inc	4, %o0		!	dst += 4;
	st	%o4, [%o1]
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lkcopy_mopw:
	be	Lkcopy_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsh	[%o0], %o4	! *(short *)dst = *(short *)src;
	be	Lkcopy_done	! if ((len & 1) == 0) goto done;
	 sth	%o4, [%o1]
	ldsb	[%o0 + 2], %o4	! dst[2] = src[2];
	stb	%o4, [%o1 + 2]
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl
	 clr	%o0
	NOTREACHED

	! mop up trailing byte (if present).
Lkcopy_mopb:
	bne,a	1f
	 ldsb	[%o0], %o4

Lkcopy_done:
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl
	 clr	%o0
	NOTREACHED

1:
	stb	%o4, [%o1]
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl
	 clr	%o0
	NOTREACHED

Lkcerr:
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	set	2f, %o0
	call	printf
	 nop
!	ta	1; nop
	restore
	.data
2:	.asciz	"kcopy error\n"
	_ALIGN
	.text
3:
#endif
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl				! and return error indicator
	 mov	EFAULT, %o0
	NOTREACHED

#if 0
/*
 * This version uses the UltraSPARC's v9 block copy extensions.
 * We need to use the floating point registers.  However, they
 * get disabled on entering the kernel.  When we try to access
 * them we should trap, which will result in saving them.
 *
 * Otherwise we can simply save them to the stack.
 */
Lbcopy_vis:


/*	2) Align source & dest		*/
	alignaddr	%o1, x, %o4	! This is our destination
	alignaddr	%o0, x, %o3
/*	3) load in the first batch	*/
	ldda		[%o3] ASI_BLK_P, %f0
loop:
	faligndata	%f0, %f2, %f34
	faligndata	%f2, %f4, %f36
	faligndata	%f4, %f6, %f38
	faligndata	%f6, %f8, %f40
	faligndata	%f8, %f10, %f42
	faligndata	%f10, %f12, %f44
	faligndata	%f12, %f14, %f46
	addcc		%l0, -1, %l0
	bg,pt		l1
	 fmovd		%f14, %f48
	/* end of loop handling */
l1:
	ldda		[regaddr] ASI_BLK_P, %f0
	stda		%f32, [regaddr] ASI_BLK_P
	faligndata	%f48, %f16, %f32
	faligndata	%f16, %f18, %f34
	faligndata	%f18, %f20, %f36
	faligndata	%f20, %f22, %f38
	faligndata	%f22, %f24, %f40
	faligndata	%f24, %f26, %f42
	faligndata	%f26, %f28, %f44
	faligndata	%f28, %f30, %f46
	addcc		%l0, -1, %l0
	bne,pt		done
	 fmovd		%f30, %f48
	ldda		[regaddr] ASI_BLK_P, %f16
	stda		%f32, [regaddr] ASI_BLK_P
	ba		loop
	 faligndata	%f48, %f0, %f32
done:

#endif

/*
 * ovbcopy(src, dst, len): like bcopy, but regions may overlap.
 */
ENTRY(ovbcopy)
	cmp	%o0, %o1	! src < dst?
	bgeu	Lbcopy_start	! no, go copy forwards as via bcopy
	 cmp	%o2, BCOPY_SMALL! (check length for doublecopy first)

	/*
	 * Since src comes before dst, and the regions might overlap,
	 * we have to do the copy starting at the end and working backwards.
	 */
	add	%o2, %o0, %o0	! src += len
	add	%o2, %o1, %o1	! dst += len
	bge,a	Lback_fancy	! if len >= BCOPY_SMALL, go be fancy
	 btst	3, %o0

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 EMPTY
0:
	dec	%o0		!	*--dst = *--src;
	ldsb	[%o0], %o4
	dec	%o1
	deccc	%o2
	bge	0b
	 stb	%o4, [%o1]
1:
	retl
	 nop

	/*
	 * Plenty to copy, try to be optimal.
	 * We only bother with word/halfword/byte copies here.
	 */
Lback_fancy:
!	btst	3, %o0		! done already
	bnz	1f		! if ((src & 3) == 0 &&
	 btst	3, %o1		!     (dst & 3) == 0)
	bz,a	Lback_words	!	goto words;
	 dec	4, %o2		! (done early for word copy)

1:
	/*
	 * See if the low bits match.
	 */
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3
	bz,a	3f		! if (t & 1) == 0, can do better
	 btst	1, %o0

	/*
	 * Nope; gotta do byte copy.
	 */
2:
	dec	%o0		! do {
	ldsb	[%o0], %o4	!	*--dst = *--src;
	dec	%o1
	deccc	%o2		! } while (--len != 0);
	bnz	2b
	 stb	%o4, [%o1]
	retl
	 nop

3:
	/*
	 * Can do halfword or word copy, but might have to copy 1 byte first.
	 */
!	btst	1, %o0		! done earlier
	bz,a	4f		! if (src & 1) {	/* copy 1 byte */
	 btst	2, %o3		! (done early)
	dec	%o0		!	*--dst = *--src;
	ldsb	[%o0], %o4
	dec	%o1
	stb	%o4, [%o1]
	dec	%o2		!	len--;
	btst	2, %o3		! }

4:
	/*
	 * See if we can do a word copy ((t&2) == 0).
	 */
!	btst	2, %o3		! done earlier
	bz,a	6f		! if (t & 2) == 0, can do word copy
	 btst	2, %o0		! (src&2, done early)

	/*
	 * Gotta do halfword copy.
	 */
	dec	2, %o2		! len -= 2;
5:
	dec	2, %o0		! do {
	ldsh	[%o0], %o4	!	src -= 2;
	dec	2, %o1		!	dst -= 2;
	deccc	2, %o0		!	*(short *)dst = *(short *)src;
	bge	5b		! } while ((len -= 2) >= 0);
	 sth	%o4, [%o1]
	b	Lback_mopb	! goto mop_up_byte;
	 btst	1, %o2		! (len&1, done early)

6:
	/*
	 * We can do word copies, but we might have to copy
	 * one halfword first.
	 */
!	btst	2, %o0		! done already
	bz	7f		! if (src & 2) {
	 dec	4, %o2		! (len -= 4, done early)
	dec	2, %o0		!	src -= 2, dst -= 2;
	ldsh	[%o0], %o4	!	*(short *)dst = *(short *)src;
	dec	2, %o1
	sth	%o4, [%o1]
	dec	2, %o2		!	len -= 2;
				! }

7:
Lback_words:
	/*
	 * Do word copies (backwards), then mop up trailing halfword
	 * and byte if any.
	 */
!	dec	4, %o2		! len -= 4, done already
0:				! do {
	dec	4, %o0		!	src -= 4;
	dec	4, %o1		!	src -= 4;
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	deccc	4, %o2		! } while ((len -= 4) >= 0);
	bge	0b
	 st	%o4, [%o1]

	/*
	 * Check for trailing shortword.
	 */
	btst	2, %o2		! if (len & 2) {
	bz,a	1f
	 btst	1, %o2		! (len&1, done early)
	dec	2, %o0		!	src -= 2, dst -= 2;
	ldsh	[%o0], %o4	!	*(short *)dst = *(short *)src;
	dec	2, %o1
	sth	%o4, [%o1]	! }
	btst	1, %o2

	/*
	 * Check for trailing byte.
	 */
1:
Lback_mopb:
!	btst	1, %o2		! (done already)
	bnz,a	1f		! if (len & 1) {
	 ldsb	[%o0 - 1], %o4	!	b = src[-1];
	retl
	 nop
1:
	retl			!	dst[-1] = b;
	 stb	%o4, [%o1 - 1]	! }


/*
 * savefpstate(f) struct fpstate *f;
 *
 * Store the current FPU state.  The first `st %fsr' may cause a trap;
 * our trap handler knows how to recover (by `returning' to savefpcont).
 *
 * Since the kernel may need to use the FPU and we have problems atomically
 * testing and enabling the FPU, we leave here with the FPRS_FEF bit set.
 * Normally this should be turned on in loadfpstate().
 */
 /* XXXXXXXXXX  Assume caller created a proper stack frame */
ENTRY(savefpstate)
!	flushw			! Make sure we don't have stack probs & lose hibits of %o
	rdpr	%pstate, %o1		! enable FP before we begin
	rd	%fprs, %o5
	wr	%g0, FPRS_FEF, %fprs
	or	%o1, PSTATE_PEF, %o1
	wrpr	%o1, 0, %pstate
	/* do some setup work while we wait for PSR_EF to turn on */
	set	FSR_QNE, %o2		! QNE = 0x2000, too big for immediate
	clr	%o3			! qsize = 0;
special_fp_store:
	/* This may need to be done w/rdpr/stx combo */
	stx	%fsr, [%o0 + FS_FSR]	! f->fs_fsr = getfsr();
	/*
	 * Even if the preceding instruction did not trap, the queue
	 * is not necessarily empty: this state save might be happening
	 * because user code tried to store %fsr and took the FPU
	 * from `exception pending' mode to `exception' mode.
	 * So we still have to check the blasted QNE bit.
	 * With any luck it will usually not be set.
	 */
	rd	%gsr, %o4		! Save %gsr
	st	%o4, [%o0 + FS_GSR]

	ldx	[%o0 + FS_FSR], %o4	! if (f->fs_fsr & QNE)
	btst	%o2, %o4
	add	%o0, FS_REGS, %o2
	bnz	Lfp_storeq		!	goto storeq;
Lfp_finish:
	 btst	BLOCK_ALIGN, %o2	! Needs to be re-executed
	bnz,pn	%icc, 2f		! Check alignment
	 st	%o3, [%o0 + FS_QSIZE]	! f->fs_qsize = qsize;
	btst	FPRS_DL, %o5		! Lower FPU clean?
	bz,a,pt	%icc, 1f		! Then skip it
	 add	%o2, 128, %o2		! Skip a block
	

	stda	%f0, [%o2] ASI_BLK_COMMIT_P	! f->fs_f0 = etc;
	inc	BLOCK_SIZE, %o2
	stda	%f16, [%o2] ASI_BLK_COMMIT_P
	inc	BLOCK_SIZE, %o2
1:
	btst	FPRS_DU, %o5		! Upper FPU clean?
	bz,pt	%icc, 2f		! Then skip it
	 nop

	stda	%f32, [%o2] ASI_BLK_COMMIT_P
	inc	BLOCK_SIZE, %o2
	stda	%f48, [%o2] ASI_BLK_COMMIT_P
2:
	membar	#Sync			! Finish operation so we can
	retl
	 wr	%g0, FPRS_FEF, %fprs	! Mark FPU clean
3:
	btst	FPRS_DL, %o5		! Lower FPU clean?
	bz,a,pt	%icc, 4f		! Then skip it
	 add	%o0, 128, %o0

	std	%f0, [%o0 + FS_REGS + (4*0)]	! f->fs_f0 = etc;
	std	%f2, [%o0 + FS_REGS + (4*2)]
	std	%f4, [%o0 + FS_REGS + (4*4)]
	std	%f6, [%o0 + FS_REGS + (4*6)]
	std	%f8, [%o0 + FS_REGS + (4*8)]
	std	%f10, [%o0 + FS_REGS + (4*10)]
	std	%f12, [%o0 + FS_REGS + (4*12)]
	std	%f14, [%o0 + FS_REGS + (4*14)]
	std	%f16, [%o0 + FS_REGS + (4*16)]
	std	%f18, [%o0 + FS_REGS + (4*18)]
	std	%f20, [%o0 + FS_REGS + (4*20)]
	std	%f22, [%o0 + FS_REGS + (4*22)]
	std	%f24, [%o0 + FS_REGS + (4*24)]
	std	%f26, [%o0 + FS_REGS + (4*26)]
	std	%f28, [%o0 + FS_REGS + (4*28)]
	std	%f30, [%o0 + FS_REGS + (4*30)]
4:
	btst	FPRS_DU, %o5		! Upper FPU clean?
	bz,pt	%icc, 5f		! Then skip it
	 nop

	std	%f32, [%o0 + FS_REGS + (4*32)]
	std	%f34, [%o0 + FS_REGS + (4*34)]
	std	%f36, [%o0 + FS_REGS + (4*36)]
	std	%f38, [%o0 + FS_REGS + (4*38)]
	std	%f40, [%o0 + FS_REGS + (4*40)]
	std	%f42, [%o0 + FS_REGS + (4*42)]
	std	%f44, [%o0 + FS_REGS + (4*44)]
	std	%f46, [%o0 + FS_REGS + (4*46)]
	std	%f48, [%o0 + FS_REGS + (4*48)]
	std	%f50, [%o0 + FS_REGS + (4*50)]
	std	%f52, [%o0 + FS_REGS + (4*52)]
	std	%f54, [%o0 + FS_REGS + (4*54)]
	std	%f56, [%o0 + FS_REGS + (4*56)]
	std	%f58, [%o0 + FS_REGS + (4*58)]
	std	%f60, [%o0 + FS_REGS + (4*60)]
	std	%f62, [%o0 + FS_REGS + (4*62)]
5:
	retl
	 wr	%g0, FPRS_FEF, %fprs		! Mark FPU clean

/*
 * Store the (now known nonempty) FP queue.
 * We have to reread the fsr each time in order to get the new QNE bit.
 *
 * UltraSPARCs don't have floating point queues.
 */
Lfp_storeq:
	add	%o0, FS_QUEUE, %o1	! q = &f->fs_queue[0];
1:
	rdpr	%fq, %o4
	stx	%o4, [%o1 + %o3]	! q[qsize++] = fsr_qfront();
	stx	%fsr, [%o0 + FS_FSR] 	! reread fsr
	ldx	[%o0 + FS_FSR], %o4	! if fsr & QNE, loop
	btst	%o5, %o4
	bnz	1b
	 inc	8, %o3
	b	Lfp_finish		! set qsize and finish storing fregs
	 srl	%o3, 3, %o3		! (but first fix qsize)

/*
 * The fsr store trapped.  Do it again; this time it will not trap.
 * We could just have the trap handler return to the `st %fsr', but
 * if for some reason it *does* trap, that would lock us into a tight
 * loop.  This way we panic instead.  Whoopee.
 */
savefpcont:
	b	special_fp_store + 4	! continue
	 stx	%fsr, [%o0 + FS_FSR]	! but first finish the %fsr store

/*
 * Load FPU state.
 */
 /* XXXXXXXXXX  Should test to see if we only need to do a partial restore */
ENTRY(loadfpstate)
	flushw			! Make sure we don't have stack probs & lose hibits of %o
	rdpr	%pstate, %o1		! enable FP before we begin
	ld	[%o0 + FS_GSR], %o3	! Restore %gsr
	set	PSTATE_PEF, %o2
	wr	%g0, FPRS_FEF, %fprs
	or	%o1, %o2, %o1
	wrpr	%o1, 0, %pstate
	wr	%o3, %g0, %gsr
	ldx	[%o0 + FS_FSR], %fsr	! setfsr(f->fs_fsr);
	add	%o0, FS_REGS, %o3
	btst	BLOCK_ALIGN, %o3
	bne,a,pt	%icc, 1f	! Only use block loads on aligned blocks
	 ldd	[%o0 + FS_REGS + (4*0)], %f0
	ldda	[%o3] ASI_BLK_P, %f0
	inc	BLOCK_SIZE, %o0
	ldda	[%o3] ASI_BLK_P, %f16
	inc	BLOCK_SIZE, %o0
	ldda	[%o3] ASI_BLK_P, %f32
	inc	BLOCK_SIZE, %o0
	ldda	[%o3] ASI_BLK_P, %f48
	retl
	 wr	%g0, FPRS_FEF, %fprs	! Clear dirty bits
1:
	/* Unaligned -- needs to be done the long way
	ldd	[%o0 + FS_REGS + (4*0)], %f0
	ldd	[%o0 + FS_REGS + (4*2)], %f2
	ldd	[%o0 + FS_REGS + (4*4)], %f4
	ldd	[%o0 + FS_REGS + (4*6)], %f6
	ldd	[%o0 + FS_REGS + (4*8)], %f8
	ldd	[%o0 + FS_REGS + (4*10)], %f10
	ldd	[%o0 + FS_REGS + (4*12)], %f12
	ldd	[%o0 + FS_REGS + (4*14)], %f14
	ldd	[%o0 + FS_REGS + (4*16)], %f16
	ldd	[%o0 + FS_REGS + (4*18)], %f18
	ldd	[%o0 + FS_REGS + (4*20)], %f20
	ldd	[%o0 + FS_REGS + (4*22)], %f22
	ldd	[%o0 + FS_REGS + (4*24)], %f24
	ldd	[%o0 + FS_REGS + (4*26)], %f26
	ldd	[%o0 + FS_REGS + (4*28)], %f28
	ldd	[%o0 + FS_REGS + (4*30)], %f30
	ldd	[%o0 + FS_REGS + (4*32)], %f32
	ldd	[%o0 + FS_REGS + (4*34)], %f34
	ldd	[%o0 + FS_REGS + (4*36)], %f36
	ldd	[%o0 + FS_REGS + (4*38)], %f38
	ldd	[%o0 + FS_REGS + (4*40)], %f40
	ldd	[%o0 + FS_REGS + (4*42)], %f42
	ldd	[%o0 + FS_REGS + (4*44)], %f44
	ldd	[%o0 + FS_REGS + (4*46)], %f46
	ldd	[%o0 + FS_REGS + (4*48)], %f48
	ldd	[%o0 + FS_REGS + (4*50)], %f50
	ldd	[%o0 + FS_REGS + (4*52)], %f52
	ldd	[%o0 + FS_REGS + (4*54)], %f54
	ldd	[%o0 + FS_REGS + (4*56)], %f56
	ldd	[%o0 + FS_REGS + (4*58)], %f58
	ldd	[%o0 + FS_REGS + (4*60)], %f60
 	ldd	[%o0 + FS_REGS + (4*62)], %f62
	retl
	 wr	%g0, FPRS_FEF, %fprs	! Clear dirty bits

/*
 * ienab_bis(bis) int bis;
 * ienab_bic(bic) int bic;
 *
 * Set and clear bits in the interrupt register.
 */

/*
 * sun4u has separate asr's for clearing/setting the interrupt mask.
 */
ENTRY(ienab_bis)
	retl
	 wr	%o0, 0, SET_SOFTINT	! SET_SOFTINT

ENTRY(ienab_bic)
	retl
	 wr	%o0, 0, CLEAR_SOFTINT	! CLEAR_SOFTINT

/*
 * send_softint(cpu, level, intrhand)
 *
 * Send a softint with an intrhand pointer so we can cause a vectored
 * interrupt instead of a polled interrupt.  This does pretty much the
 * same as interrupt_vector.  If intrhand is NULL then it just sends
 * a polled interrupt.  If cpu is -1 then send it to this CPU, if it's
 * -2 send it to any CPU, otherwise send it to a particular CPU.
 *
 * XXXX Dispatching to different CPUs is not implemented yet.
 *
 * XXXX We do not block interrupts here so it's possible that another
 *	interrupt of the same level is dispatched before we get to
 *	enable the softint, causing a spurious interrupt.
 */
ENTRY(send_softint)
	rdpr	%pil, %g1	! s = splx(level)
	cmp	%g1, %o1
	bge,pt	%icc, 1f
	 nop
	wrpr	%o1, 0, %pil
1:
#ifdef	VECTORED_INTERRUPTS
	brz,pn	%o2, 1f
	 set	intrpending, %o3
	ldstub	[%o2 + IH_PEND], %o5
	mov	8, %o4			! Number of slots to search
#ifdef INTR_INTERLOCK
	brnz	%o5, 1f
#endif
	 sll	%o1, PTRSHFT+3, %o5	! Find start of table for this IPL
	add	%o3, %o5, %o3
2:
#if 1
	DLFLUSH(%o3, %o5)
	mov	%o2, %o5
	CASPTR	[%o3] ASI_N, %g0, %o5	! Try a slot -- MPU safe
	brz,pt	%o5, 4f			! Available?
#else
	DLFLUSH(%o3, %o5)
	LDPTR	[%o3], %o5		! Try a slog
	brz,a	%o5, 4f			! Available?
	 STPTR	%o2, [%o3]		! Grab it
#endif
	 dec	%o4
	brgz,pt	%o4, 2b
	 inc	PTRSZ, %o3		! Next slot

	!! If we get here we have a problem.
	!! There were no available slots and the interrupt was lost.
	!! We'll resort to polling in this case.
4:
	 DLFLUSH(%o3, %o3)		! Prevent D$ pollution
1:
#endif	/* VECTORED_INTERRUPTS */
	mov	1, %o3			! Change from level to bitmask
	sllx	%o3, %o1, %o3
	wr	%o3, 0, SET_SOFTINT	! SET_SOFTINT
	retl
	 wrpr	%g1, 0, %pil		! restore IPL

/*
 * Here is a very good random number generator.  This implementation is
 * based on _Two Fast Implementations of the `Minimal Standard' Random
 * Number Generator_, David G. Carta, Communications of the ACM, Jan 1990,
 * Vol 33 No 1.
 */
/*
 * This should be rewritten using the mulx instr. if I ever understand what it
 * does.
 */
	.data
randseed:
	.word	1
	.text
ENTRY(random)
	sethi	%hi(16807), %o1
	wr	%o1, %lo(16807), %y
	 sethi	%hi(randseed), %o5
	 ld	[%o5 + %lo(randseed)], %o0
	 andcc	%g0, 0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %o0, %o2
	mulscc  %o2, %g0, %o2
	rd	%y, %o3
	srl	%o2, 16, %o1
	set	0xffff, %o4
	and	%o4, %o2, %o0
	sll	%o0, 15, %o0
	srl	%o3, 17, %o3
	or	%o3, %o0, %o0
	addcc	%o0, %o1, %o0
	bneg	1f
	 sethi	%hi(0x7fffffff), %o1
	retl
	 st	%o0, [%o5 + %lo(randseed)]
1:
	or	%o1, %lo(0x7fffffff), %o1
	add	%o0, 1, %o0
	and	%o1, %o0, %o0
	retl
	 st	%o0, [%o5 + %lo(randseed)]

/*
 * void microtime(struct timeval *tv)
 *
 * LBL's sparc bsd 'microtime': We don't need to spl (so this routine
 * can be a leaf routine) and we don't keep a 'last' timeval (there
 * can't be two calls to this routine in a microsecond).  This seems to
 * be about 20 times faster than the Sun code on an SS-2. - vj
 *
 * Read time values from slowest-changing to fastest-changing,
 * then re-read out to slowest.  If the values read before
 * the innermost match those read after, the innermost value
 * is consistent with the outer values.  If not, it may not
 * be and we must retry.  Typically this loop runs only once;
 * occasionally it runs twice, and only rarely does it run longer.
 *
 * If we used the %tick register we could go into the nano-seconds,
 * and it must run for at least 10 years according to the v9 spec.
 *
 * For some insane reason timeval structure members are `long's so
 * we need to change this code depending on the memory model.
 *
 * NB: if somehow time was 128-bit aligned we could use an atomic
 * quad load to read it in and not bother de-bouncing it.
 */
#define MICROPERSEC	(1000000)

	.data
	.align	8
	.globl	_C_LABEL(cpu_clockrate)
_C_LABEL(cpu_clockrate):
	!! Pretend we have a 200MHz clock -- cpu_attach will fix this
	.xword	200000000
	!! Here we'll store cpu_clockrate/1000000 so we can calculate usecs
	.xword	0
	.text

ENTRY(microtime)
	sethi	%hi(timerreg_4u), %g3
	sethi	%hi(_C_LABEL(time)), %g2
	LDPTR	[%g3+%lo(timerreg_4u)], %g3			! usec counter
	brz,pn	%g3, microtick					! If we have no counter-timer use %tick
2:
	!!  NB: if we could guarantee 128-bit alignment of these values we could do an atomic read
	LDPTR	[%g2+%lo(_C_LABEL(time))], %o2			! time.tv_sec & time.tv_usec
	LDPTR	[%g2+%lo(_C_LABEL(time)+PTRSZ)], %o3		! time.tv_sec & time.tv_usec
	ldx	[%g3], %o4					! Load usec timer valuse
	LDPTR	[%g2+%lo(_C_LABEL(time))], %g1			! see if time values changed
	LDPTR	[%g2+%lo(_C_LABEL(time)+PTRSZ)], %g5		! see if time values changed
	cmp	%g1, %o2
	bne	2b						! if time.tv_sec changed
	 cmp	%g5, %o3
	bne	2b						! if time.tv_usec changed
	 add	%o4, %o3, %o3					! Our timers have 1usec resolution

	set	MICROPERSEC, %o5				! normalize usec value
	sub	%o3, %o5, %o5					! Did we overflow?
	brlz,pn	%o5, 4f
	 nop
	add	%o2, 1, %o2					! overflow
	mov	%o5, %o3
4:
	STPTR	%o2, [%o0]					! (should be able to std here)
	retl
	 STPTR	%o3, [%o0+PTRSZ]

microtick:
#ifndef TICK_IS_TIME
/*
 * The following code only works if %tick is reset each interrupt.
 */
2:
	!!  NB: if we could guarantee 128-bit alignment of these values we could do an atomic read
	LDPTR	[%g2+%lo(_C_LABEL(time))], %o2			! time.tv_sec & time.tv_usec
	LDPTR	[%g2+%lo(_C_LABEL(time)+PTRSZ)], %o3		! time.tv_sec & time.tv_usec
	rdpr	%tick, %o4					! Load usec timer value
	LDPTR	[%g2+%lo(_C_LABEL(time))], %g1			! see if time values changed
	LDPTR	[%g2+%lo(_C_LABEL(time)+PTRSZ)], %g5		! see if time values changed
	cmp	%g1, %o2
	bne	2b						! if time.tv_sec changed
	 cmp	%g5, %o3
	bne	2b						! if time.tv_usec changed
	 sethi	%hi(_C_LABEL(cpu_clockrate)), %g1
	ldx	[%g1 + %lo(_C_LABEL(cpu_clockrate) + 8)], %o1
	sethi	%hi(MICROPERSEC), %o5
	brnz,pt	%o1, 3f
	 or	%o5, %lo(MICROPERSEC), %o5

	!! Calculate ticks/usec
	ldx	[%g1 + %lo(_C_LABEL(cpu_clockrate))], %o1	! No, we need to calculate it
	udivx	%o1, %o5, %o1
	stx	%o1, [%g1 + %lo(_C_LABEL(cpu_clockrate) + 8)]	! Save it so we don't need to divide again
3:
	udivx	%o4, %o1, %o4					! Convert to usec
	add	%o4, %o3, %o3

	sub	%o3, %o5, %o5					! Did we overflow?
	brlz,pn	%o5, 4f
	 nop
	add	%o2, 1, %o2					! overflow
	mov	%o5, %o3
4:
	STPTR	%o2, [%o0]					! (should be able to std here)
	retl
	 STPTR	%o3, [%o0+PTRSZ]
#else
/*
 * The following code only works if %tick is synchronized with time.
 */
	sethi	%hi(_C_LABEL(cpu_clockrate)), %o3
	ldx	[%o3 + %lo(_C_LABEL(cpu_clockrate) + 8)], %o4	! Get scale factor
	rdpr	%tick, %o1
	sethi	%hi(MICROPERSEC), %o2
	brnz,pt	%o4, 1f						! Already scaled?
	 or	%o2, %lo(MICROPERSEC), %o2

	!! Calculate ticks/usec
	ldx	[%o3 + %lo(_C_LABEL(cpu_clockrate))], %o4	! No, we need to calculate it
	udivx	%o4, %o2, %o4					! Hz / 10^6 = MHz
	stx	%o4, [%o3 + %lo(_C_LABEL(cpu_clockrate))]	! Save it so we don't need to divide again
1:

	udivx	%o1, %o4, %o1					! Scale it: ticks / MHz = usec

	udivx	%o1, %o2, %o3					! Now %o3 has seconds
	STPTR	%o3, [%o0]					! and store it

	mulx	%o3, %o2, %o2					! Now calculate usecs -- damn no remainder insn
	sub	%o1, %o2, %o1					! %o1 has the remainder

	retl
	 STPTR	%o1, [%o0+PTRSZ]				! Save time_t low word
#endif

/*
 * delay function
 *
 * void delay(N)  -- delay N microseconds
 *
 * Register usage: %o0 = "N" number of usecs to go (counts down to zero)
 *		   %o1 = "timerblurb" (stays constant)
 *		   %o2 = counter for 1 usec (counts down from %o1 to zero)
 *
 *
 *	cpu_clockrate should be tuned during CPU probe to the CPU clockrate in Hz
 *
 */
ENTRY(delay)			! %o0 = n
#if 1
	rdpr	%tick, %o1					! Take timer snapshot
	sethi	%hi(_C_LABEL(cpu_clockrate)), %o2
	sethi	%hi(MICROPERSEC), %o3
	ldx	[%o2 + %lo(_C_LABEL(cpu_clockrate) + 8)], %o4	! Get scale factor
	brnz,pt	%o4, 0f
	 or	%o3, %lo(MICROPERSEC), %o3

	!! Calculate ticks/usec
	ldx	[%o2 + %lo(_C_LABEL(cpu_clockrate))], %o4	! No, we need to calculate it
	udivx	%o4, %o3, %o4
	stx	%o4, [%o2 + %lo(_C_LABEL(cpu_clockrate) + 8)]	! Save it so we don't need to divide again
0:

	mulx	%o0, %o4, %o0					! Convert usec -> ticks
	rdpr	%tick, %o2					! Top of next itr
1:
	sub	%o2, %o1, %o3					! How many ticks have gone by?
	sub	%o0, %o3, %o4					! Decrement count by that much
	movrgz	%o3, %o4, %o0					! But only if we're decrementing
	mov	%o2, %o1					! Remember last tick
	brgz,pt	%o0, 1b						! Done?
	 rdpr	%tick, %o2					! Get new tick

	retl
	 nop
#else
/* This code only works if %tick does not wrap */
	rdpr	%tick, %g1					! Take timer snapshot
	sethi	%hi(_C_LABEL(cpu_clockrate)), %g2
	sethi	%hi(MICROPERSEC), %o2
	ldx	[%g2 + %lo(_C_LABEL(cpu_clockrate))], %g2	! Get scale factor
	or	%o2, %lo(MICROPERSEC), %o2
!	sethi	%hi(_C_LABEL(timerblurb), %o5			! This is if we plan to tune the clock
!	ld	[%o5 + %lo(_C_LABEL(timerblurb))], %o5		!  with respect to the counter/timer
	mulx	%o0, %g2, %g2					! Scale it: (usec * Hz) / 1 x 10^6 = ticks
	udivx	%g2, %o2, %g2
	add	%g1, %g2, %g2
!	add	%o5, %g2, %g2					! But this gets complicated
	rdpr	%tick, %g1					! Top of next itr
	mov	%g1, %g1	! Erratum 50
1:
	cmp	%g1, %g2
	bl,a,pn %xcc, 1b					! Done?
	 rdpr	%tick, %g1

	retl
	 nop
#endif
	/*
	 * If something's wrong with the standard setup do this stupid loop
	 * calibrated for a 143MHz processor.
	 */
Lstupid_delay:
	set	142857143/MICROPERSEC, %o1
Lstupid_loop:
	brnz,pt	%o1, Lstupid_loop
	 dec	%o1
	brnz,pt	%o0, Lstupid_delay
	 dec	%o0
	retl
	 nop


/*
 * next_tick(long increment)
 *
 * Sets the %tick_cmpr register to fire off in `increment' machine
 * cycles in the future.  Also handles %tick wraparound.  In 32-bit
 * mode we're limited to a 32-bit increment.
 */
ENTRY(next_tick)
#ifndef TICK_IS_TIME
/*
 * Synchronizing %tick with time is hard.  Just reset it to zero.
 * This entire %tick thing will break on an MPU.
 */
	wr	%o0, TICK_CMPR	! Make sure we enable the interrupt
	retl
	 wrpr	%g0, 0, %tick	! Reset the clock
#else
	rdpr	%tick, %o1
	mov	1, %o2
	sllx	%o2, 63, %o2
	andn	%o1, %o2, %o3	! Mask off the NPT bit
	add	%o3, %o0, %o3	! Add increment
	brlez,pn	%o3, 1f	! Overflow?
	 nop
	retl
	 wr	%o3, TICK_CMPR
1:
	wrpr	%g0, %tick	! XXXXX Reset %tick on overflow
	retl
	 wr	%o0, TICK_CMPR
#endif

ENTRY(setjmp)
	save	%sp, -CC64FSZ, %sp	! Need a frame to return to.
	flushw
	stx	%fp, [%i0+0]	! 64-bit stack pointer
	stx	%i7, [%i0+8]	! 64-bit return pc
	ret
	 restore	%g0, 0, %o0

	.data
Lpanic_ljmp:
	.asciz	"longjmp botch"
	_ALIGN
	.text

ENTRY(longjmp)
	save	%sp, -CC64FSZ, %sp	! prepare to restore to (old) frame
	flushw
	mov	1, %i2
	ldx	[%i0+0], %fp	! get return stack
	movrz	%i1, %i1, %i2	! compute v ? v : 1
	ldx	[%i0+8], %i7	! get rpc
	ret
	 restore	%i2, 0, %o0

#ifdef DDB
	/*
	 * Debug stuff.  Dump the trap registers into buffer & set tl=0.
	 *
	 *  %o0 = *ts
	 */
	ENTRY(savetstate)
	mov	%o0, %o1
	CHKPT(%o4,%o3,0x28)
	rdpr	%tl, %o0
	brz	%o0, 2f
	 mov	%o0, %o2
1:
	rdpr	%tstate, %o3
	stx	%o3, [%o1]
	deccc	%o2
	inc	8, %o1
	rdpr	%tpc, %o4
	stx	%o4, [%o1]
	inc	8, %o1
	rdpr	%tnpc, %o5
	stx	%o5, [%o1]
	inc	8, %o1
	rdpr	%tt, %o4
	stx	%o4, [%o1]
	inc	8, %o1
	bnz	1b
	 wrpr	%o2, 0, %tl
2:
	retl
	 nop

	/*
	 * Debug stuff.  Resore trap registers from buffer.
	 *
	 *  %o0 = %tl
	 *  %o1 = *ts
	 *
	 * Maybe this should be re-written to increment tl instead of decrementing.
	 */
	ENTRY(restoretstate)
	CHKPT(%o4,%o3,0x36)
	flushw			! Make sure we don't have stack probs & lose hibits of %o
	brz,pn	%o0, 2f
	 mov	%o0, %o2
	CHKPT(%o4,%o3,0x29)
	wrpr	%o0, 0, %tl
1:
	ldx	[%o1], %o3
	deccc	%o2
	inc	8, %o1
	wrpr	%o3, 0, %tstate
	ldx	[%o1], %o4
	inc	8, %o1
	wrpr	%o4, 0, %tpc
	ldx	[%o1], %o5
	inc	8, %o1
	wrpr	%o5, 0, %tnpc
	ldx	[%o1], %o4
	inc	8, %o1
	wrpr	%o4, 0, %tt
	bnz	1b
	 wrpr	%o2, 0, %tl
2:
	CHKPT(%o4,%o3,0x30)
	retl
	 wrpr	%o0, 0, %tl

	/*
	 * Switch to context in %o0
	 */
	ENTRY(switchtoctx)
	set	0x030, %o3
	stxa	%o3, [%o3] ASI_DMMU_DEMAP
	membar	#Sync
	stxa	%o3, [%o3] ASI_IMMU_DEMAP
	membar	#Sync
	wr	%g0, ASI_DMMU, %asi
	stxa	%o0, [CTX_SECONDARY] %asi	! Maybe we should invali
	membar	#Sync					! No real reason for this XXXX
	sethi	%hi(KERNBASE), %o2
	flush	%o2
	retl
	 nop

#ifndef _LP64
	/*
	 * Convert to 32-bit stack then call OF_sym2val()
	 */
	ENTRY(_C_LABEL(OF_sym2val32))
	save	%sp, -CC64FSZ, %sp
	btst	7, %i0
	bnz,pn	%icc, 1f
	 add	%sp, BIAS, %o1
	btst	1, %sp
	movnz	%icc, %o1, %sp
	call	_C_LABEL(OF_sym2val)
	 mov	%i0, %o0
1:
	ret
	 restore	%o0, 0, %o0

	/*
	 * Convert to 32-bit stack then call OF_val2sym()
	 */
	ENTRY(_C_LABEL(OF_val2sym32))
	save	%sp, -CC64FSZ, %sp
	btst	7, %i0
	bnz,pn	%icc, 1f
	 add	%sp, BIAS, %o1
	btst	1, %sp
	movnz	%icc, %o1, %sp
	call	_C_LABEL(OF_val2sym)
	 mov	%i0, %o0
1:
	ret
	 restore	%o0, 0, %o0
#endif /* _LP64 */
#endif /* DDB */

	.data
	_ALIGN
#ifdef DDB
	.globl	_C_LABEL(esym)
_C_LABEL(esym):
	POINTER	0
	.globl	_C_LABEL(ssym)
_C_LABEL(ssym):
	POINTER	0
#endif
	.globl	_C_LABEL(proc0paddr)
_C_LABEL(proc0paddr):
	POINTER	_C_LABEL(u0)		! KVA of proc0 uarea

/* interrupt counters	XXX THESE BELONG ELSEWHERE (if anywhere) */
	.globl	_C_LABEL(intrcnt), _C_LABEL(eintrcnt), _C_LABEL(intrnames), _C_LABEL(eintrnames)
_C_LABEL(intrnames):
	.asciz	"spur"
	.asciz	"lev1"
	.asciz	"lev2"
	.asciz	"lev3"
	.asciz	"lev4"
	.asciz	"lev5"
	.asciz	"lev6"
	.asciz	"lev7"
	.asciz  "lev8"
	.asciz	"lev9"
	.asciz	"clock"
	.asciz	"lev11"
	.asciz	"lev12"
	.asciz	"lev13"
	.asciz	"prof"
	.asciz  "lev15"
_C_LABEL(eintrnames):
	_ALIGN
_C_LABEL(intrcnt):
	.space	4*15
_C_LABEL(eintrcnt):

	.comm	_C_LABEL(curproc), PTRSZ
	.comm	_C_LABEL(promvec), PTRSZ
	.comm	_C_LABEL(nwindows), 4
