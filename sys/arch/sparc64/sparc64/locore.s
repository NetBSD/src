/*	$NetBSD: locore.s,v 1.377.2.2 2015/12/27 12:09:44 skrll Exp $	*/

/*
 * Copyright (c) 2006-2010 Matthew R. Green
 * Copyright (c) 1996-2002 Eduardo Horvath
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

#undef	PARANOID		/* Extremely expensive consistency checks */
#undef	NO_VCACHE		/* Map w/D$ disabled */
#undef	TRAPSTATS		/* Count traps */
#undef	TRAPS_USE_IG		/* Use Interrupt Globals for all traps */
#define	HWREF			/* Track ref/mod bits in trap handlers */
#undef	DCACHE_BUG		/* Flush D$ around ASI_PHYS accesses */
#undef	NO_TSB			/* Don't use TSB */
#define	BB_ERRATA_1		/* writes to TICK_CMPR may fail */
#undef	TLB_FLUSH_LOWVA		/* also flush 32-bit entries from the MMU */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/param.h>
#include <machine/types.h>
#include <sparc64/sparc64/intreg.h>
#include <sparc64/sparc64/timerreg.h>
#include <machine/ctlreg.h>
#include <machine/psl.h>
#include <machine/signal.h>
#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/intr.h>
#include <machine/asm.h>
#include <machine/locore.h>
#ifdef SUN4V
#include <machine/hypervisor.h>
#endif	
#include <sys/syscall.h>

#define BLOCK_SIZE SPARC64_BLOCK_SIZE
#define BLOCK_ALIGN SPARC64_BLOCK_ALIGN

#ifdef SUN4V
#define SUN4V_N_REG_WINDOWS    8  /* As per UA2005 spec */
#define SUN4V_NWINDOWS           (SUN4V_N_REG_WINDOWS-1) /* This is an index number, so subtract one */
#endif
	
#include "ksyms.h"

	/* Misc. macros */

	.macro	GET_MAXCWP reg
#ifdef SUN4V
	sethi	%hi(cputyp), \reg
	ld	[\reg + %lo(cputyp)], \reg
	cmp	\reg, CPU_SUN4V
	bne,pt	%icc, 2f
	 nop
	/* sun4v */
	ba	3f
	 mov	SUN4V_NWINDOWS, \reg
2:		
#endif	
	/* sun4u */
	rdpr	%ver, \reg
	and	\reg, CWP, \reg
3:
	.endm

	.macro	SET_MMU_CONTEXTID_SUN4U ctxid,ctx
	stxa	\ctxid, [\ctx] ASI_DMMU;
	.endm
	
#ifdef SUN4V
	.macro	SET_MMU_CONTEXTID_SUN4V ctxid,ctx
	stxa	\ctxid, [\ctx] ASI_MMU;
	.endm
#endif	
		
	.macro	SET_MMU_CONTEXTID ctxid,ctx,scratch
#ifdef SUN4V
	sethi	%hi(cputyp), \scratch
	ld	[\scratch + %lo(cputyp)], \scratch
	cmp	\scratch, CPU_SUN4V
	bne,pt	%icc, 2f
	 nop
	/* sun4v */
	SET_MMU_CONTEXTID_SUN4V \ctxid,\ctx
	ba	3f
	 nop
2:		
#endif	
	/* sun4u */
	SET_MMU_CONTEXTID_SUN4U \ctxid,\ctx
3:
	.endm

#ifdef SUN4V
	.macro	NORMAL_GLOBALS_SUN4V
	 wrpr	%g0, 0, %gl				! Set globals to level 0
	.endm
#endif
	.macro	NORMAL_GLOBALS_SUN4U
	wrpr	%g0, PSTATE_KERN, %pstate		! Alternate Globals (AG) bit set to zero
	.endm
		
#ifdef SUN4V
	.macro	ALTERNATE_GLOBALS_SUN4V
	 wrpr	%g0, 1, %gl				! Set globals to level 1
	.endm
#endif	
	.macro	ALTERNATE_GLOBALS_SUN4U
	 wrpr    %g0, PSTATE_KERN|PSTATE_AG, %pstate	! Alternate Globals (AG) bit set to one
	.endm
	
	.macro	ENABLE_INTERRUPTS scratch
	rdpr	 %pstate, \scratch
	or	\scratch, PSTATE_IE, \scratch	! Interrupt Enable (IE) bit set to one
	wrpr	%g0, \scratch, %pstate
	.endm

	.macro	DISABLE_INTERRUPTS scratch
	rdpr	 %pstate, \scratch
	and	\scratch, ~PSTATE_IE, \scratch	! Interrupt Enable (IE) bit set to zero
	wrpr	%g0, \scratch, %pstate
	.endm
		

#ifdef SUN4V
	/* Misc. sun4v macros */
	
	.macro	GET_MMFSA reg
	sethi	%hi(CPUINFO_VA + CI_MMFSA), \reg
	LDPTR	[\reg + %lo(CPUINFO_VA + CI_MMFSA)], \reg
	.endm

	.macro	GET_CTXBUSY reg
	sethi	%hi(CPUINFO_VA + CI_CTXBUSY), \reg
	LDPTR	[\reg + %lo(CPUINFO_VA + CI_CTXBUSY)], \reg
	.endm

	.macro	GET_TSB_DMMU reg
	sethi	%hi(CPUINFO_VA + CI_TSB_DMMU), \reg
	LDPTR	[\reg + %lo(CPUINFO_VA + CI_TSB_DMMU)], \reg
	.endm

#endif
		
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

	.register	%g2,#scratch
	.register	%g3,#scratch


	.data
	.globl	_C_LABEL(data_start)
_C_LABEL(data_start):					! Start of data segment

#ifdef KGDB
/*
 * Another item that must be aligned, easiest to put it here.
 */
KGDB_STACK_SIZE = 2048
	.globl	_C_LABEL(kgdb_stack)
_C_LABEL(kgdb_stack):
	.space	KGDB_STACK_SIZE		! hope this is enough
#endif

#ifdef NOTDEF_DEBUG
/*
 * This stack is used when we detect kernel stack corruption.
 */
	.space	USPACE
	.align	16
panicstack:
#endif

/*
 * romp is the prom entry pointer
 * romtba is the prom trap table base address
 */
	.globl	romp
romp:	POINTER	0
	.globl	romtba
romtba:	POINTER	0

	.globl	cputyp
cputyp:	.word	CPU_SUN4U ! Default to sun4u		
			
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
 *	that information.  Trap types in these macros are all dummys.
 */
	/* regular vectored traps */

#define	VTRAP(type, label) \
	ba,a,pt	%icc,label; nop; NOTREACHED; TA8

	/* hardware interrupts (can be linked or made `fast') */
#define	HARDINT4U(lev) \
	VTRAP(lev, _C_LABEL(sparc_interrupt))
#ifdef SUN4V
#define HARDINT4V(lev) HARDINT4U(lev)	
#endif

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
	andcc	%sp, 1, %g0; \
	bnz,pt	%xcc, label64+4;	/* Is it a v9 or v8 stack? */ \
	 wr	%g0, as, %asi; \
	ba,pt	%xcc, label32+8; \
	 srl	%sp, 0, %sp; /* fixup 32-bit pointers */ \
	NOTREACHED; \
	TA32

	/* fill a 64-bit register window */
#define FILL64(label,as) \
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
	andcc	%sp, 1, %i0; \
	bnz	(label64)+4; /* See if it's a v9 stack or v8 */ \
	 wr	%g0, as, %asi; \
	ba	(label32)+8; \
	 srl	%sp, 0, %sp; /* fixup 32-bit pointers */ \
	NOTREACHED; \
	TA32

	/* handle clean window trap when trap level = 0 */
	.macro CLEANWIN0
	rdpr %cleanwin, %o7
	inc %o7				!	This handler is in-lined and cannot fault
#ifdef DEBUG
	set	0xbadcafe, %l0		! DEBUG -- compiler should not rely on zero-ed registers.
#else
	clr	%l0
#endif
	wrpr %g0, %o7, %cleanwin	!       Nucleus (trap&IRQ) code does not need clean windows

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
	.endm

	/* handle clean window trap when trap level = 1 */
	.macro CLEANWIN1
	clr	%l0
#ifdef DEBUG
	set	0xbadbeef, %l0		! DEBUG
#endif
	mov %l0, %l1; mov %l0, %l2
	rdpr %cleanwin, %o7		!	This handler is in-lined and cannot fault
	inc %o7; mov %l0, %l3		!       Nucleus (trap&IRQ) code does not need clean windows
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
	mov %l0, %l4; mov %l0, %l5; mov %l0, %l6; mov %l0, %l7
	mov %l0, %o0; mov %l0, %o1; mov %l0, %o2; mov %l0, %o3

	mov %l0, %o4; mov %l0, %o5; mov %l0, %o6; mov %l0, %o7
	CLRTT
	retry; nop; TA32
	.endm
	
	.globl	start, _C_LABEL(kernel_text)
	_C_LABEL(kernel_text) = kernel_start		! for kvm_mkdb(8)
kernel_start:
	/* Traps from TL=0 -- traps from user mode */
#ifdef __STDC__
#define TABLE(name)	user_ ## name
#else
#define	TABLE(name)	user_/**/name
#endif
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
	VTRAP(T_INST_EXCEPT, textfault)	! 008 = instr. access except
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
	TRAP(T_FP_IEEE_754)		! 021 = ieee 754 exception
	TRAP(T_FP_OTHER)		! 022 = other fp exception
	TRAP(T_TAGOF)			! 023 = tag overflow
	CLEANWIN0			! 024-027 = clean window trap
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
	HARDINT4U(15)			! 04f = nonmaskable interrupt
	UTRAP(0x050); UTRAP(0x051); UTRAP(0x052); UTRAP(0x053); UTRAP(0x054); UTRAP(0x055)
	UTRAP(0x056); UTRAP(0x057); UTRAP(0x058); UTRAP(0x059); UTRAP(0x05a); UTRAP(0x05b)
	UTRAP(0x05c); UTRAP(0x05d); UTRAP(0x05e); UTRAP(0x05f)
	VTRAP(0x060, interrupt_vector); ! 060 = interrupt vector
	TRAP(T_PA_WATCHPT)		! 061 = physical address data watchpoint
	TRAP(T_VA_WATCHPT)		! 062 = virtual address data watchpoint
	TRAP(T_ECCERR)			! 063 = corrected ECC error
ufast_IMMU_miss:			! 064 = fast instr access MMU miss
	ldxa	[%g0] ASI_IMMU_8KPTR, %g2 ! Load IMMU 8K TSB pointer
#ifdef NO_TSB
	ba,a	%icc, instr_miss
#endif
	ldxa	[%g0] ASI_IMMU, %g1	! Load IMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	! Load TSB tag:data into %g4:%g5
	brgez,pn %g5, instr_miss	! Entry invalid?  Punt
	 cmp	%g1, %g4		! Compare TLB tags
	bne,pn %xcc, instr_miss		! Got right tag?
	 nop
	CLRTT
	stxa	%g5, [%g0] ASI_IMMU_DATA_IN ! Enter new mapping
	retry				! Try new mapping
1:
	sir
	TA32
ufast_DMMU_miss:			! 068 = fast data access MMU miss
	ldxa	[%g0] ASI_DMMU_8KPTR, %g2! Load DMMU 8K TSB pointer
#ifdef NO_TSB
	ba,a	%icc, data_miss
#endif
	ldxa	[%g0] ASI_DMMU, %g1	! Load DMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	! Load TSB tag and data into %g4 and %g5
	brgez,pn %g5, data_miss		! Entry invalid?  Punt
	 cmp	%g1, %g4		! Compare TLB tags
	bnz,pn	%xcc, data_miss		! Got right tag?
	 nop
	CLRTT
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(udhit)), %g1
	lduw	[%g1+%lo(_C_LABEL(udhit))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(udhit))]
#endif
	stxa	%g5, [%g0] ASI_DMMU_DATA_IN ! Enter new mapping
	retry				! Try new mapping
1:
	sir
	TA32
ufast_DMMU_protection:			! 06c = fast data access MMU protection
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(udprot)), %g1
	lduw	[%g1+%lo(_C_LABEL(udprot))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(udprot))]
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
TABLE(uspill):
	SPILL64(uspill8,ASI_AIUS)	! 0x080 spill_0_normal -- used to save user windows in user mode
	SPILL32(uspill4,ASI_AIUS)	! 0x084 spill_1_normal
	SPILLBOTH(uspill8,uspill4,ASI_AIUS)	 ! 0x088 spill_2_normal
	UTRAP(0x08c); TA32		! 0x08c spill_3_normal
TABLE(kspill):
	SPILL64(kspill8,ASI_N)		! 0x090 spill_4_normal -- used to save supervisor windows
	SPILL32(kspill4,ASI_N)		! 0x094 spill_5_normal
	SPILLBOTH(kspill8,kspill4,ASI_N) ! 0x098 spill_6_normal
	UTRAP(0x09c); TA32		! 0x09c spill_7_normal
TABLE(uspillk):
	SPILL64(uspillk8,ASI_AIUS)	! 0x0a0 spill_0_other -- used to save user windows in supervisor mode
	SPILL32(uspillk4,ASI_AIUS)	! 0x0a4 spill_1_other
	SPILLBOTH(uspillk8,uspillk4,ASI_AIUS) ! 0x0a8 spill_2_other
	UTRAP(0x0ac); TA32		! 0x0ac spill_3_other
	UTRAP(0x0b0); TA32		! 0x0b0 spill_4_other
	UTRAP(0x0b4); TA32		! 0x0b4 spill_5_other
	UTRAP(0x0b8); TA32		! 0x0b8 spill_6_other
	UTRAP(0x0bc); TA32		! 0x0bc spill_7_other
TABLE(ufill):
	FILL64(ufill8,ASI_AIUS)		! 0x0c0 fill_0_normal -- used to fill windows when running user mode
	FILL32(ufill4,ASI_AIUS)		! 0x0c4 fill_1_normal
	FILLBOTH(ufill8,ufill4,ASI_AIUS) ! 0x0c8 fill_2_normal
	UTRAP(0x0cc); TA32		! 0x0cc fill_3_normal
TABLE(kfill):
	FILL64(kfill8,ASI_N)		! 0x0d0 fill_4_normal -- used to fill windows when running supervisor mode
	FILL32(kfill4,ASI_N)		! 0x0d4 fill_5_normal
	FILLBOTH(kfill8,kfill4,ASI_N)	! 0x0d8 fill_6_normal
	UTRAP(0x0dc); TA32		! 0x0dc fill_7_normal
TABLE(ufillk):
	FILL64(ufillk8,ASI_AIUS)	! 0x0e0 fill_0_other
	FILL32(ufillk4,ASI_AIUS)	! 0x0e4 fill_1_other
	FILLBOTH(ufillk8,ufillk4,ASI_AIUS) ! 0x0e8 fill_2_other
	UTRAP(0x0ec); TA32		! 0x0ec fill_3_other
	UTRAP(0x0f0); TA32		! 0x0f0 fill_4_other
	UTRAP(0x0f4); TA32		! 0x0f4 fill_5_other
	UTRAP(0x0f8); TA32		! 0x0f8 fill_6_other
	UTRAP(0x0fc); TA32		! 0x0fc fill_7_other
TABLE(syscall):
	SYSCALL				! 0x100 = sun syscall
	BPT				! 0x101 = pseudo breakpoint instruction
	STRAP(0x102); STRAP(0x103); STRAP(0x104); STRAP(0x105); STRAP(0x106); STRAP(0x107)
	SYSCALL				! 0x108 = svr4 syscall
	SYSCALL				! 0x109 = bsd syscall
	BPT_KGDB_EXEC			! 0x10a = enter kernel gdb on kernel startup
	STRAP(0x10b); STRAP(0x10c); STRAP(0x10d); STRAP(0x10e); STRAP(0x10f);
	STRAP(0x110); STRAP(0x111); STRAP(0x112); STRAP(0x113); STRAP(0x114); STRAP(0x115); STRAP(0x116); STRAP(0x117)
	STRAP(0x118); STRAP(0x119); STRAP(0x11a); STRAP(0x11b); STRAP(0x11c); STRAP(0x11d); STRAP(0x11e); STRAP(0x11f)
	STRAP(0x120); STRAP(0x121); STRAP(0x122); STRAP(0x123); STRAP(0x124); STRAP(0x125); STRAP(0x126); STRAP(0x127)
	STRAP(0x128); STRAP(0x129); STRAP(0x12a); STRAP(0x12b); STRAP(0x12c); STRAP(0x12d); STRAP(0x12e); STRAP(0x12f)
	STRAP(0x130); STRAP(0x131); STRAP(0x132); STRAP(0x133); STRAP(0x134); STRAP(0x135); STRAP(0x136); STRAP(0x137)
	STRAP(0x138); STRAP(0x139); STRAP(0x13a); STRAP(0x13b); STRAP(0x13c); STRAP(0x13d); STRAP(0x13e); STRAP(0x13f)
	SYSCALL				! 0x140 SVID syscall (Solaris 2.7)
	SYSCALL				! 0x141 SPARC International syscall
	SYSCALL				! 0x142	OS Vendor syscall
	SYSCALL				! 0x143 HW OEM syscall
	STRAP(0x144); STRAP(0x145); STRAP(0x146); STRAP(0x147)
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

	/* Traps from TL>0 -- traps from supervisor mode */
#undef TABLE
#ifdef __STDC__
#define	TABLE(name)	nucleus_ ## name
#else
#define	TABLE(name)	nucleus_/**/name
#endif
trapbase_priv:
	UTRAP(0x000)			! 000 = reserved -- Use it to boot
	/* We should not get the next 5 traps */
	UTRAP(0x001)			! 001 = POR Reset -- ROM should get this
	UTRAP(0x002)			! 002 = WDR Watchdog -- ROM should get this
	UTRAP(0x003)			! 003 = XIR -- ROM should get this
	UTRAP(0x004)			! 004 = SIR -- ROM should get this
	UTRAP(0x005)			! 005 = RED state exception
	UTRAP(0x006); UTRAP(0x007)
ktextfault:
	VTRAP(T_INST_EXCEPT, textfault)	! 008 = instr. access except
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
	TRAP(T_FP_IEEE_754)		! 021 = ieee 754 exception
	TRAP(T_FP_OTHER)		! 022 = other fp exception
	TRAP(T_TAGOF)			! 023 = tag overflow
	CLEANWIN1			! 024-027 = clean window trap
	TRAP(T_DIV0)			! 028 = divide by zero
	UTRAP(0x029)			! 029 = internal processor error
	UTRAP(0x02a); UTRAP(0x02b); UTRAP(0x02c); UTRAP(0x02d); UTRAP(0x02e); UTRAP(0x02f)
kdatafault:
	VTRAP(T_DATAFAULT, winfault)	! 030 = data fetch fault
	UTRAP(0x031)			! 031 = data MMU miss -- no MMU
	VTRAP(T_DATA_ERROR, winfault)	! 032 = data fetch fault
	VTRAP(T_DATA_PROT, winfault)	! 033 = data fetch fault
	VTRAP(T_ALIGN, checkalign)	! 034 = address alignment error -- we could fix it inline...
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
	HARDINT4U(15)			! 04f = nonmaskable interrupt
	UTRAP(0x050); UTRAP(0x051); UTRAP(0x052); UTRAP(0x053); UTRAP(0x054); UTRAP(0x055)
	UTRAP(0x056); UTRAP(0x057); UTRAP(0x058); UTRAP(0x059); UTRAP(0x05a); UTRAP(0x05b)
	UTRAP(0x05c); UTRAP(0x05d); UTRAP(0x05e); UTRAP(0x05f)
	VTRAP(0x060, interrupt_vector); ! 060 = interrupt vector
	TRAP(T_PA_WATCHPT)		! 061 = physical address data watchpoint
	TRAP(T_VA_WATCHPT)		! 062 = virtual address data watchpoint
	TRAP(T_ECCERR)			! 063 = corrected ECC error
kfast_IMMU_miss:			! 064 = fast instr access MMU miss
	ldxa	[%g0] ASI_IMMU_8KPTR, %g2 ! Load IMMU 8K TSB pointer
#ifdef NO_TSB
	ba,a	%icc, instr_miss
#endif
	ldxa	[%g0] ASI_IMMU, %g1	! Load IMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	! Load TSB tag:data into %g4:%g5
	brgez,pn %g5, instr_miss	! Entry invalid?  Punt
	 cmp	%g1, %g4		! Compare TLB tags
	bne,pn %xcc, instr_miss		! Got right tag?
	 nop
	CLRTT
	stxa	%g5, [%g0] ASI_IMMU_DATA_IN ! Enter new mapping
	retry				! Try new mapping
1:
	sir
	TA32
kfast_DMMU_miss:			! 068 = fast data access MMU miss
	ldxa	[%g0] ASI_DMMU_8KPTR, %g2! Load DMMU 8K TSB pointer
#ifdef NO_TSB
	ba,a	%icc, data_miss
#endif
	ldxa	[%g0] ASI_DMMU, %g1	! Load DMMU tag target register
	ldda	[%g2] ASI_NUCLEUS_QUAD_LDD, %g4	! Load TSB tag and data into %g4 and %g5
	brgez,pn %g5, data_miss		! Entry invalid?  Punt
	 cmp	%g1, %g4		! Compare TLB tags
	bnz,pn	%xcc, data_miss		! Got right tag?
	 nop
	CLRTT
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(kdhit)), %g1
	lduw	[%g1+%lo(_C_LABEL(kdhit))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(kdhit))]
#endif
	stxa	%g5, [%g0] ASI_DMMU_DATA_IN ! Enter new mapping
	retry				! Try new mapping
1:
	sir
	TA32
kfast_DMMU_protection:			! 06c = fast data access MMU protection
#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(kdprot)), %g1
	lduw	[%g1+%lo(_C_LABEL(kdprot))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(kdprot))]
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
TABLE(uspill):
	SPILL64(1,ASI_AIUS)		! 0x080 spill_0_normal -- used to save user windows
	SPILL32(2,ASI_AIUS)		! 0x084 spill_1_normal
	SPILLBOTH(1b,2b,ASI_AIUS)	! 0x088 spill_2_normal
	UTRAP(0x08c); TA32		! 0x08c spill_3_normal
TABLE(kspill):
	SPILL64(1,ASI_N)		! 0x090 spill_4_normal -- used to save supervisor windows
	SPILL32(2,ASI_N)		! 0x094 spill_5_normal
	SPILLBOTH(1b,2b,ASI_N)		! 0x098 spill_6_normal
	UTRAP(0x09c); TA32		! 0x09c spill_7_normal
TABLE(uspillk):
	SPILL64(1,ASI_AIUS)		! 0x0a0 spill_0_other -- used to save user windows in nucleus mode
	SPILL32(2,ASI_AIUS)		! 0x0a4 spill_1_other
	SPILLBOTH(1b,2b,ASI_AIUS)	! 0x0a8 spill_2_other
	UTRAP(0x0ac); TA32		! 0x0ac spill_3_other
	UTRAP(0x0b0); TA32		! 0x0b0 spill_4_other
	UTRAP(0x0b4); TA32		! 0x0b4 spill_5_other
	UTRAP(0x0b8); TA32		! 0x0b8 spill_6_other
	UTRAP(0x0bc); TA32		! 0x0bc spill_7_other
TABLE(ufill):
	FILL64(nufill8,ASI_AIUS)	! 0x0c0 fill_0_normal -- used to fill windows when running nucleus mode from user
	FILL32(nufill4,ASI_AIUS)	! 0x0c4 fill_1_normal
	FILLBOTH(nufill8,nufill4,ASI_AIUS) ! 0x0c8 fill_2_normal
	UTRAP(0x0cc); TA32		! 0x0cc fill_3_normal
TABLE(sfill):
	FILL64(sfill8,ASI_N)		! 0x0d0 fill_4_normal -- used to fill windows when running nucleus mode from supervisor
	FILL32(sfill4,ASI_N)		! 0x0d4 fill_5_normal
	FILLBOTH(sfill8,sfill4,ASI_N)	! 0x0d8 fill_6_normal
	UTRAP(0x0dc); TA32		! 0x0dc fill_7_normal
TABLE(kfill):
	FILL64(nkfill8,ASI_AIUS)	! 0x0e0 fill_0_other -- used to fill user windows when running nucleus mode -- will we ever use this?
	FILL32(nkfill4,ASI_AIUS)	! 0x0e4 fill_1_other
	FILLBOTH(nkfill8,nkfill4,ASI_AIUS)! 0x0e8 fill_2_other
	UTRAP(0x0ec); TA32		! 0x0ec fill_3_other
	UTRAP(0x0f0); TA32		! 0x0f0 fill_4_other
	UTRAP(0x0f4); TA32		! 0x0f4 fill_5_other
	UTRAP(0x0f8); TA32		! 0x0f8 fill_6_other
	UTRAP(0x0fc); TA32		! 0x0fc fill_7_other
TABLE(syscall):
	SYSCALL				! 0x100 = sun syscall
	BPT				! 0x101 = pseudo breakpoint instruction
	STRAP(0x102); STRAP(0x103); STRAP(0x104); STRAP(0x105); STRAP(0x106); STRAP(0x107)
	SYSCALL				! 0x108 = svr4 syscall
	SYSCALL				! 0x109 = bsd syscall
	BPT_KGDB_EXEC			! 0x10a = enter kernel gdb on kernel startup
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

#ifdef SUN4V

/* Macros for sun4v traps */

	.macro	sun4v_trap_entry count
	.rept	\count
	ba	slowtrap
	 nop
	.align	32
	.endr
	.endm

	.macro	sun4v_trap_entry_fail count
	.rept	\count
	sir
	.align	32
	.endr
	.endm

	.macro	sun4v_trap_entry_spill_fill_fail count
	.rept	\count
	sir
	.align	128
	.endr
	.endm

/* The actual trap base for sun4v */
	.align	0x8000
	.globl	_C_LABEL(trapbase_sun4v)
_C_LABEL(trapbase_sun4v):
	!
	! trap level 0
	!
	sun4v_trap_entry 36					! 0x000-0x023
	CLEANWIN0						! 0x24-0x27 = clean window
	sun4v_trap_entry 9					! 0x028-0x030			
	VTRAP(T_DATA_MMU_MISS, sun4v_dtsb_miss)			! 0x031 = data MMU miss
	sun4v_trap_entry 15					! 0x032-0x040
	HARDINT4V(1)						! 0x041 = level 1 interrupt
	HARDINT4V(2)						! 0x042 = level 2 interrupt
	HARDINT4V(3)						! 0x043 = level 3 interrupt
	HARDINT4V(4)						! 0x044 = level 4 interrupt
	HARDINT4V(5)						! 0x045 = level 5 interrupt
	HARDINT4V(6)						! 0x046 = level 6 interrupt
	HARDINT4V(7)						! 0x047 = level 7 interrupt
	HARDINT4V(8)						! 0x048 = level 8 interrupt
	HARDINT4V(9)						! 0x049 = level 9 interrupt
	HARDINT4V(10)						! 0x04a = level 10 interrupt
	HARDINT4V(11)						! 0x04b = level 11 interrupt
	HARDINT4V(12)						! 0x04c = level 12 interrupt
	HARDINT4V(13)						! 0x04d = level 13 interrupt
	HARDINT4V(14)						! 0x04e = level 14 interrupt
	HARDINT4V(15)						! 0x04f = level 15 interrupt
	sun4v_trap_entry 44					! 0x050-0x07b
	VTRAP(T_CPU_MONDO, sun4v_cpu_mondo)			! 0x07c = cpu mondo
	sun4v_trap_entry 3					! 0x07d-0x07f
	SPILL64(uspill8_sun4vt0,ASI_AIUS)			! 0x080 spill_0_normal -- used to save user windows in user mode
	SPILL32(uspill4_sun4vt0,ASI_AIUS)			! 0x084 spill_1_normal
	SPILLBOTH(uspill8_sun4vt0,uspill4_sun4vt0,ASI_AIUS)	! 0x088 spill_2_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x08c spill_3_normal
	SPILL64(kspill8_sun4vt0,ASI_N)				! 0x090 spill_4_normal  -- used to save supervisor windows
	SPILL32(kspill4_sun4vt0,ASI_N)				! 0x094 spill_5_normal
	SPILLBOTH(kspill8_sun4vt0,kspill4_sun4vt0,ASI_N)	! 0x098 spill_6_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x09c spill_7_normal
	SPILL64(uspillk8_sun4vt0,ASI_AIUS)			! 0x0a0 spill_0_other -- used to save user windows in supervisor mode
	SPILL32(uspillk4_sun4vt0,ASI_AIUS)			! 0x0a4 spill_1_other
	SPILLBOTH(uspillk8_sun4vt0,uspillk4_sun4vt0,ASI_AIUS)	! 0x0a8 spill_2_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0ac spill_3_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0b0 spill_4_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0b4 spill_5_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0b8 spill_6_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0bc spill_7_other
	FILL64(ufill8_sun4vt0,ASI_AIUS)				! 0x0c0 fill_0_normal -- used to fill windows when running user mode
	FILL32(ufill4_sun4vt0,ASI_AIUS)				! 0x0c4 fill_1_normal
	FILLBOTH(ufill8_sun4vt0,ufill4_sun4vt0,ASI_AIUS)	! 0x0c8 fill_2_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x0cc fill_3_normal
	FILL64(kfill8_sun4vt0,ASI_N)				! 0x0d0 fill_4_normal  -- used to fill windows when running supervisor mode
	FILL32(kfill4_sun4vt0,ASI_N)				! 0x0d4 fill_5_normal
	FILLBOTH(kfill8_sun4vt0,kfill4_sun4vt0,ASI_N)		! 0x0d8 fill_6_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x0dc fill_7_normal
	FILL64(ufillk8_sun4vt0,ASI_AIUS)			! 0x0e0 fill_0_other
	FILL32(ufillk4_sun4vt0,ASI_AIUS)			! 0x0e4 fill_1_other
	FILLBOTH(ufillk8_sun4vt0,ufillk4_sun4vt0,ASI_AIUS)	! 0x0e8 fill_2_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0ec fill_3_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0f0 fill_4_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0f4 fill_5_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0f8 fill_6_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0fc fill_7_other
	sun4v_trap_entry 256					! 0x100-0x1ff
	!
	! trap level 1
	!
	sun4v_trap_entry_fail 36				! 0x000-0x023
	CLEANWIN1						! 0x24-0x27 = clean window
	sun4v_trap_entry_fail 9					! 0x028-0x030			
	VTRAP(T_DATA_MMU_MISS, sun4v_dtsb_miss)			! 0x031 = data MMU miss
	sun4v_trap_entry_fail 78				! 0x032-0x07f
	SPILL64(uspill8_sun4vt1,ASI_AIUS)			! 0x080 spill_0_normal -- save user windows
	SPILL32(uspill4_sun4vt1,ASI_AIUS)			! 0x084 spill_1_normal
	SPILLBOTH(uspill8_sun4vt1,uspill4_sun4vt1,ASI_AIUS)	! 0x088 spill_2_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x08c spill_3_normal
	SPILL64(kspill8_sun4vt1,ASI_N)				! 0x090 spill_4_normal -- save supervisor windows
	SPILL32(kspill4_sun4vt1,ASI_N)				! 0x094 spill_5_normal
	SPILLBOTH(kspill8_sun4vt1,kspill4_sun4vt1,ASI_N)	! 0x098 spill_6_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x09c spill_7_normal
	SPILL64(uspillk8_sun4vt1,ASI_AIUS)			! 0x0a0 spill_0_other -- save user windows in nucleus mode
	SPILL32(uspillk4_sun4vt1,ASI_AIUS)			! 0x0a4 spill_1_other
	SPILLBOTH(uspillk8_sun4vt1,uspillk4_sun4vt1,ASI_AIUS)	! 0x0a8 spill_2_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0ac spill_3_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0b0 spill_4_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0b4 spill_5_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0b8 spill_6_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0bc spill_7_other
	FILL64(ufill8_sun4vt1,ASI_AIUS)				! 0x0c0 fill_0_normal -- fill windows when running nucleus mode from user
	FILL32(ufill4_sun4vt1,ASI_AIUS)				! 0x0c4 fill_1_normal
	FILLBOTH(ufill8_sun4vt1,ufill4_sun4vt1,ASI_AIUS)	! 0x0c8 fill_2_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x0cc fill_3_normal
	FILL64(kfill8_sun4vt1,ASI_N)				! 0x0d0 fill_4_normal -- fill windows when running nucleus mode from supervisor
	FILL32(kfill4_sun4vt1,ASI_N)				! 0x0d4 fill_5_normal
	FILLBOTH(kfill8_sun4vt1,kfill4_sun4vt1,ASI_N)		! 0x0d8 fill_6_normal
	sun4v_trap_entry_spill_fill_fail 1			! 0x0dc fill_7_normal
	FILL64(ufillk8_sun4vt1,ASI_AIUS)			! 0x0e0 fill_0_other -- fill user windows when running nucleus mode -- will we ever use this?
	FILL32(ufillk4_sun4vt1,ASI_AIUS)			! 0x0e4 fill_1_other
	FILLBOTH(ufillk8_sun4vt1,ufillk4_sun4vt1,ASI_AIUS)	! 0x0e8 fill_2_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0ec fill_3_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0f0 fill_4_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0f4 fill_5_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0f8 fill_6_other
	sun4v_trap_entry_spill_fill_fail 1			! 0x0fc fill_7_other
	sun4v_trap_entry_fail 256				! 0x100-0x1ff

#endif
		
#if 0
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
#endif

#ifdef NOTDEF_DEBUG
/*
 * A hardware red zone is impossible.  We simulate one in software by
 * keeping a `red zone' pointer; if %sp becomes less than this, we panic.
 * This is expensive and is only enabled when debugging.
 */
#define	REDSIZE	(PCB_SIZE)	/* Mark used portion of pcb structure out of bounds */
#define	REDSTACK 2048		/* size of `panic: stack overflow' region */
	.data
	_ALIGN
redzone:
	.xword	_C_LABEL(XXX) + REDSIZE
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
 * XXX it would be good to save the interrupt stack frame to the kernel
 * stack so we wouldn't have to copy it later if we needed to handle a AST.
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
  * can potentially be lost.  This trap can be caused when allocating
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

#ifdef DEBUG
	/* Only save a snapshot of locals and ins in DEBUG kernels */
#define	SAVE_LOCALS_INS	\
	/* Save local registers to trap frame */ \
	stx	%l0, [%g6 + CC64FSZ + STKB + TF_L + (0*8)]; \
	stx	%l1, [%g6 + CC64FSZ + STKB + TF_L + (1*8)]; \
	stx	%l2, [%g6 + CC64FSZ + STKB + TF_L + (2*8)]; \
	stx	%l3, [%g6 + CC64FSZ + STKB + TF_L + (3*8)]; \
	stx	%l4, [%g6 + CC64FSZ + STKB + TF_L + (4*8)]; \
	stx	%l5, [%g6 + CC64FSZ + STKB + TF_L + (5*8)]; \
	stx	%l6, [%g6 + CC64FSZ + STKB + TF_L + (6*8)]; \
	stx	%l7, [%g6 + CC64FSZ + STKB + TF_L + (7*8)]; \
\
	/* Save in registers to trap frame */ \
	stx	%i0, [%g6 + CC64FSZ + STKB + TF_I + (0*8)]; \
	stx	%i1, [%g6 + CC64FSZ + STKB + TF_I + (1*8)]; \
	stx	%i2, [%g6 + CC64FSZ + STKB + TF_I + (2*8)]; \
	stx	%i3, [%g6 + CC64FSZ + STKB + TF_I + (3*8)]; \
	stx	%i4, [%g6 + CC64FSZ + STKB + TF_I + (4*8)]; \
	stx	%i5, [%g6 + CC64FSZ + STKB + TF_I + (5*8)]; \
	stx	%i6, [%g6 + CC64FSZ + STKB + TF_I + (6*8)]; \
	stx	%i7, [%g6 + CC64FSZ + STKB + TF_I + (7*8)]; \
\
	stx	%g1, [%g6 + CC64FSZ + STKB + TF_FAULT];
#else
#define	SAVE_LOCALS_INS
#endif

#ifdef _LP64
#define	FIXUP_TRAP_STACK \
	btst	1, %g6;						/* Fixup 64-bit stack if necessary */ \
	bnz,pt	%icc, 1f; \
	 add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	inc	-BIAS, %g6; \
1:
#else
#define	FIXUP_TRAP_STACK \
	srl	%g6, 0, %g6;					/* truncate at 32-bits */ \
	btst	1, %g6;						/* Fixup 64-bit stack if necessary */ \
	add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	add	%g6, BIAS, %g5; \
	movne	%icc, %g5, %g6;
#endif

#ifdef _LP64
#define	TRAP_SETUP(stackspace) \
	sethi	%hi(CPCB), %g6; \
	sethi	%hi((stackspace)), %g5; \
	LDPTR	[%g6 + %lo(CPCB)], %g6; \
	sethi	%hi(USPACE), %g7;				/* Always multiple of page size */ \
	or	%g5, %lo((stackspace)), %g5; \
	add	%g6, %g7, %g6; \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	\
	sub	%g7, WSTATE_KERN, %g7;				/* Compare & leave in register */ \
	movrz	%g7, %sp, %g6;					/* Select old (kernel) stack or base of kernel stack */ \
	FIXUP_TRAP_STACK \
	SAVE_LOCALS_INS	\
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + BIAS + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + BIAS + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + BIAS + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + BIAS + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + BIAS + TF_O + (4*8)]; \
	stx	%i5, [%sp + CC64FSZ + BIAS + TF_O + (5*8)]; \
\
	stx	%i6, [%sp + CC64FSZ + BIAS + TF_O + (6*8)]; \
	brz,pt	%g7, 1f;					/* If we were in kernel mode start saving globals */ \
	 stx	%i7, [%sp + CC64FSZ + BIAS + TF_O + (7*8)]; \
	mov	CTX_PRIMARY, %g7; \
	/* came from user mode -- switch to kernel mode stack */ \
	rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	wrpr	%g0, 0, %canrestore; \
	wrpr	%g0, %g5, %otherwin; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
\
	stxa	%g0, [%g7] ASI_DMMU; 				/* Switch MMU to kernel primary context */ \
	sethi	%hi(KERNBASE), %g5; \
	flush	%g5;						/* Some convenient address that won't trap */ \
1:
	
/*
 * Interrupt setup is almost exactly like trap setup, but we need to
 * go to the interrupt stack if (a) we came from user mode or (b) we
 * came from kernel mode on the kernel stack.
 *
 * We don't guarantee any registers are preserved during this operation.
 * So we can be more efficient.
 */
#define	INTR_SETUP(stackspace) \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	\
	sethi	%hi(EINTSTACK-BIAS), %g6; \
	sethi	%hi(EINTSTACK-INTSTACK), %g4; \
	\
	or	%g6, %lo(EINTSTACK-BIAS), %g6;			/* Base of interrupt stack */ \
	dec	%g4;						/* Make it into a mask */ \
	\
	sub	%g6, %sp, %g1;					/* Offset from interrupt stack */ \
	sethi	%hi((stackspace)), %g5; \
	\
	or	%g5, %lo((stackspace)), %g5; \
\
	andn	%g1, %g4, %g4;					/* Are we out of the interrupt stack range? */ \
	xor	%g7, WSTATE_KERN, %g3;				/* Are we on the user stack ? */ \
	\
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	orcc	%g3, %g4, %g0;					/* Definitely not off the interrupt stack */ \
	\
	sethi	%hi(CPUINFO_VA + CI_EINTSTACK), %g4; \
	bz,a,pt	%xcc, 1f; \
	 mov	%sp, %g6; \
	\
	ldx	[%g4 + %lo(CPUINFO_VA + CI_EINTSTACK)], %g4; \
	movrnz	%g4, %g4, %g6;					/* Use saved intr stack if exists */ \
	\
1:	add	%g6, %g5, %g5;					/* Allocate a stack frame */ \
	btst	1, %g6; \
	bnz,pt	%icc, 1f; \
\
	 mov	%g5, %g6; \
	\
	add	%g5, -BIAS, %g6; \
	\
1:	SAVE_LOCALS_INS	\
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + BIAS + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + BIAS + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + BIAS + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + BIAS + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + BIAS + TF_O + (4*8)]; \
\
	stx	%i5, [%sp + CC64FSZ + BIAS + TF_O + (5*8)]; \
	stx	%i6, [%sp + CC64FSZ + BIAS + TF_O + (6*8)]; \
	stx	%i6, [%sp + CC64FSZ + BIAS + TF_G + (0*8)];		/* Save fp in clockframe->cf_fp */ \
	brz,pt	%g3, 1f;					/* If we were in kernel mode start saving globals */ \
	 stx	%i7, [%sp + CC64FSZ + BIAS + TF_O + (7*8)]; \
	/* came from user mode -- switch to kernel mode stack */ \
	 rdpr	%otherwin, %g5;					/* Has this already been done? */ \
	\
	brnz,pn	%g5, 1f;					/* Don't set this twice */ \
	\
	 rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
\
	wrpr	%g0, 0, %canrestore; \
	\
	wrpr	%g0, %g5, %otherwin; \
	\
	mov	CTX_PRIMARY, %g7; \
	\
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	\
	SET_MMU_CONTEXTID %g0, %g7, %g5;			/* Switch MMU to kernel primary context */ \
	\
	sethi	%hi(KERNBASE), %g5; \
	flush	%g5;						/* Some convenient address that won't trap */ \
1:
	
#else /* _LP64 */

#define	TRAP_SETUP(stackspace) \
	sethi	%hi(CPCB), %g6; \
	sethi	%hi((stackspace)), %g5; \
	LDPTR	[%g6 + %lo(CPCB)], %g6; \
	sethi	%hi(USPACE), %g7; \
	or	%g5, %lo((stackspace)), %g5; \
	add	%g6, %g7, %g6; \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	\
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	subcc	%g7, WSTATE_KERN, %g7;				/* Compare & leave in register */ \
	movz	%icc, %sp, %g6;					/* Select old (kernel) stack or base of kernel stack */ \
	FIXUP_TRAP_STACK \
	SAVE_LOCALS_INS \
	save	%g6, 0, %sp;					/* If we fault we should come right back here */ \
	stx	%i0, [%sp + CC64FSZ + STKB + TF_O + (0*8)];		/* Save out registers to trap frame */ \
	stx	%i1, [%sp + CC64FSZ + STKB + TF_O + (1*8)]; \
	stx	%i2, [%sp + CC64FSZ + STKB + TF_O + (2*8)]; \
	stx	%i3, [%sp + CC64FSZ + STKB + TF_O + (3*8)]; \
	stx	%i4, [%sp + CC64FSZ + STKB + TF_O + (4*8)]; \
	stx	%i5, [%sp + CC64FSZ + STKB + TF_O + (5*8)]; \
	\
	stx	%i6, [%sp + CC64FSZ + STKB + TF_O + (6*8)]; \
	brz,pn	%g7, 1f;					/* If we were in kernel mode start saving globals */ \
	 stx	%i7, [%sp + CC64FSZ + STKB + TF_O + (7*8)]; \
	mov	CTX_PRIMARY, %g7; \
	/* came from user mode -- switch to kernel mode stack */ \
	rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	wrpr	%g0, 0, %canrestore; \
	wrpr	%g0, %g5, %otherwin; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	\
	stxa	%g0, [%g7] ASI_DMMU; 				/* Switch MMU to kernel primary context */ \
	sethi	%hi(KERNBASE), %g5; \
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
	sethi	%hi(EINTSTACK), %g1; \
	sethi	%hi((stackspace)), %g5; \
	btst	1, %sp; \
	add	%sp, BIAS, %g6; \
	movz	%icc, %sp, %g6; \
	or	%g1, %lo(EINTSTACK), %g1; \
	srl	%g6, 0, %g6;					/* truncate at 32-bits */ \
	set	(EINTSTACK-INTSTACK), %g7; \
	or	%g5, %lo((stackspace)), %g5; \
	sub	%g1, %g6, %g2;					/* Determine if we need to switch to intr stack or not */ \
	dec	%g7;						/* Make it into a mask */ \
	sethi	%hi(CPUINFO_VA + CI_EINTSTACK), %g3; \
	andncc	%g2, %g7, %g0;					/* XXXXXXXXXX This assumes kernel addresses are unique from user addresses */ \
	LDPTR	[%g3 + %lo(CPUINFO_VA + CI_EINTSTACK)], %g3; \
	rdpr	%wstate, %g7;					/* Find if we're from user mode */ \
	movrnz	%g3, %g3, %g1;					/* Use saved intr stack if exists */ \
	sra	%g5, 0, %g5;					/* Sign extend the damn thing */ \
	movnz	%xcc, %g1, %g6;					/* Stay on interrupt stack? */ \
	cmp	%g7, WSTATE_KERN;				/* User or kernel sp? */ \
	movnz	%icc, %g1, %g6;					/* Stay on interrupt stack? */ \
	add	%g6, %g5, %g6;					/* Allocate a stack frame */ \
	\
	SAVE_LOCALS_INS \
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
	tst	%g5; tnz %xcc, 1; nop; /* DEBUG -- this should _NEVER_ happen */ \
	brnz,pn	%g5, 1f;					/* Don't set this twice */ \
	 rdpr	%canrestore, %g5;				/* Fixup register window state registers */ \
	wrpr	%g0, 0, %canrestore; \
	mov	CTX_PRIMARY, %g7; \
	wrpr	%g0, %g5, %otherwin; \
	wrpr	%g0, WSTATE_KERN, %wstate;			/* Enable kernel mode window traps -- now we can trap again */ \
	SET_MMU_CONTEXTID %g0, %g7, %g5;			/* Switch MMU to kernel primary context */ \
	sethi	%hi(KERNBASE), %g5; \
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
	.asciz	"asmptechk: %x %x %x %x:%x\n"
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
 */
	ICACHE_ALIGN
dmmu_write_fault:
	mov	TLB_TAG_ACCESS, %g3
	sethi	%hi(0x1fff), %g6			! 8K context mask
	ldxa	[%g3] ASI_DMMU, %g3			! Get fault addr from Tag Target
	sethi	%hi(CPUINFO_VA+CI_CTXBUSY), %g4
	or	%g6, %lo(0x1fff), %g6
	LDPTR	[%g4 + %lo(CPUINFO_VA+CI_CTXBUSY)], %g4
	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	and	%g3, %g6, %g6				! Isolate context

	inc	%g5					! (0 or -1) -> (1 or 0)
	sllx	%g6, 3, %g6				! Make it into an offset into ctxbusy
	ldx	[%g4+%g6], %g4				! Load up our page table.
	srlx	%g3, STSHIFT, %g6
	cmp	%g5, 1
	bgu,pn %xcc, winfix				! Error!
	 srlx	%g3, PDSHIFT, %g5
	and	%g6, STMASK, %g6
	sll	%g6, 3, %g6

	and	%g5, PDMASK, %g5
	sll	%g5, 3, %g5
	add	%g6, %g4, %g4
	DLFLUSH(%g4,%g6)
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	DLFLUSH2(%g6)
	srlx	%g3, PTSHIFT, %g6			! Convert to ptab offset
	and	%g6, PTMASK, %g6
	add	%g5, %g4, %g5
	brz,pn	%g4, winfix				! NULL entry? check somewhere else
	 nop	

	ldxa	[%g5] ASI_PHYS_CACHED, %g4
	sll	%g6, 3, %g6
	brz,pn	%g4, winfix				! NULL entry? check somewhere else
	 add	%g6, %g4, %g6
1:
	ldxa	[%g6] ASI_PHYS_CACHED, %g4
	brgez,pn %g4, winfix				! Entry invalid?  Punt
	 or	%g4, SUN4U_TTE_MODIFY|SUN4U_TTE_ACCESS|SUN4U_TTE_W, %g7	! Update the modified bit

	btst	SUN4U_TTE_REAL_W|SUN4U_TTE_W, %g4			! Is it a ref fault?
	bz,pn	%xcc, winfix				! No -- really fault
#ifdef DEBUG
	/* Make sure we don't try to replace a kernel translation */
	/* This should not be necessary */
	sllx	%g3, 64-13, %g2				! Isolate context bits
	sethi	%hi(KERNBASE), %g5			! Don't need %lo
	brnz,pt	%g2, 0f					! Ignore context != 0
	 set	0x0800000, %g2				! 8MB
	sub	%g3, %g5, %g5
	cmp	%g5, %g2
	tlu	%xcc, 1; nop
	blu,pn	%xcc, winfix				! Next insn in delay slot is unimportant
0:
#endif
	/* Need to check for and handle large pages. */
	 srlx	%g4, 61, %g5				! Isolate the size bits
	ldxa	[%g0] ASI_DMMU_8KPTR, %g2		! Load DMMU 8K TSB pointer
	andcc	%g5, 0x3, %g5				! 8K?
	bnz,pn	%icc, winfix				! We punt to the pmap code since we can't handle policy
	 ldxa	[%g0] ASI_DMMU, %g1			! Load DMMU tag target register
	casxa	[%g6] ASI_PHYS_CACHED, %g4, %g7		!  and write it out
	membar	#StoreLoad
	cmp	%g4, %g7
	bne,pn	%xcc, 1b
	 or	%g4, SUN4U_TTE_MODIFY|SUN4U_TTE_ACCESS|SUN4U_TTE_W, %g4	! Update the modified bit
	stx	%g1, [%g2]				! Update TSB entry tag
	mov	SFSR, %g7
	stx	%g4, [%g2+8]				! Update TSB entry data
	nop

#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(protfix)), %g1
	lduw	[%g1+%lo(_C_LABEL(protfix))], %g2
	inc	%g2
	stw	%g2, [%g1+%lo(_C_LABEL(protfix))]
#endif
	mov	DEMAP_PAGE_SECONDARY, %g1		! Secondary flush
	mov	DEMAP_PAGE_NUCLEUS, %g5			! Nucleus flush
	stxa	%g0, [%g7] ASI_DMMU			! clear out the fault
	sllx	%g3, (64-13), %g7			! Need to demap old entry first
	andn	%g3, 0xfff, %g6
	movrz	%g7, %g5, %g1				! Pick one
	or	%g6, %g1, %g6
	membar	#Sync
	stxa	%g6, [%g6] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync
	
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
	sethi	%hi(0x1fff), %g6			! 8K context mask
	ldxa	[%g3] ASI_DMMU, %g3			! from tag access register
	sethi	%hi(CPUINFO_VA+CI_CTXBUSY), %g4
	or	%g6, %lo(0x1fff), %g6
	LDPTR	[%g4 + %lo(CPUINFO_VA+CI_CTXBUSY)], %g4
	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	and	%g3, %g6, %g6				! Isolate context
	
	inc	%g5					! (0 or -1) -> (1 or 0)
	sllx	%g6, 3, %g6				! Make it into an offset into ctxbusy
	ldx	[%g4+%g6], %g4				! Load up our page table.
#ifdef DEBUG
	/* Make sure we don't try to replace a kernel translation */
	/* This should not be necessary */
	brnz,pt	%g6, 1f			! If user context continue miss
	sethi	%hi(KERNBASE), %g7			! Don't need %lo
	set	0x0800000, %g6				! 8MB
	sub	%g3, %g7, %g7
	cmp	%g7, %g6
	tlu	%xcc, 1; nop
1:	
#endif
	srlx	%g3, STSHIFT, %g6
	cmp	%g5, 1
	bgu,pn %xcc, winfix				! Error!
	 srlx	%g3, PDSHIFT, %g5
	and	%g6, STMASK, %g6
	
	sll	%g6, 3, %g6
	and	%g5, PDMASK, %g5
	sll	%g5, 3, %g5
	add	%g6, %g4, %g4
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	srlx	%g3, PTSHIFT, %g6			! Convert to ptab offset
	and	%g6, PTMASK, %g6
	add	%g5, %g4, %g5
	brz,pn	%g4, data_nfo				! NULL entry? check somewhere else
	
	 nop
	ldxa	[%g5] ASI_PHYS_CACHED, %g4
	sll	%g6, 3, %g6
	brz,pn	%g4, data_nfo				! NULL entry? check somewhere else
	 add	%g6, %g4, %g6

1:
	ldxa	[%g6] ASI_PHYS_CACHED, %g4
	brgez,pn %g4, data_nfo				! Entry invalid?  Punt
	 or	%g4, SUN4U_TTE_ACCESS, %g7			! Update the access bit
	
	btst	SUN4U_TTE_ACCESS, %g4				! Need to update access git?
	bne,pt	%xcc, 1f
	 nop
	casxa	[%g6] ASI_PHYS_CACHED, %g4, %g7		!  and write it out
	cmp	%g4, %g7
	bne,pn	%xcc, 1b
	 or	%g4, SUN4U_TTE_ACCESS, %g4			! Update the access bit

1:	
	stx	%g1, [%g2]				! Update TSB entry tag
	stx	%g4, [%g2+8]				! Update TSB entry data
	stxa	%g4, [%g0] ASI_DMMU_DATA_IN		! Enter new mapping
	membar	#Sync
	CLRTT
	retry
	NOTREACHED
/*
 * We had a data miss but did not find a mapping.  Insert
 * a NFO mapping to satisfy speculative loads and return.
 * If this had been a real load, it will re-execute and
 * result in a data fault or protection fault rather than
 * a TLB miss.  We insert an 8K TTE with the valid and NFO
 * bits set.  All others should zero.  The TTE looks like this:
 *
 *	0x9000000000000000
 *
 */
data_nfo:
	sethi	%hi(0x90000000), %g4			! V(0x8)|NFO(0x1)
	sllx	%g4, 32, %g4
	stxa	%g4, [%g0] ASI_DMMU_DATA_IN		! Enter new mapping
	membar	#Sync
	CLRTT
	retry	

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
	mov	TLB_TAG_ACCESS, %g3	! Get real fault page from tag access register
	ldxa	[%g3] ASI_DMMU, %g3	! And put it into the non-MMU alternate regs
winfix:
	rdpr	%tl, %g2
	subcc	%g2, 1, %g1
	ble,pt	%icc, datafault		! Don't go below trap level 1
	 sethi	%hi(CPCB), %g6		! get current pcb


	wrpr	%g1, 0, %tl		! Pop a trap level
	rdpr	%tt, %g7		! Read type of prev. trap
	rdpr	%tstate, %g4		! Try to restore prev %cwp if we were executing a restore
	andn	%g7, 0x3f, %g5		!   window fill traps are all 0b 0000 11xx xxxx

#if 1
	cmp	%g7, 0x30		! If we took a datafault just before this trap
	bne,pt	%icc, winfixfill	! our stack's probably bad so we need to switch somewhere else
	 nop

	!!
	!! Double data fault -- bad stack?
	!!
	wrpr	%g2, %tl		! Restore trap level.
	sir				! Just issue a reset and don't try to recover.
	mov	%fp, %l6		! Save the frame pointer
	set	EINTSTACK+USPACE+CC64FSZ-STKB, %fp ! Set the frame pointer to the middle of the idle stack
	add	%fp, -CC64FSZ, %sp	! Create a stackframe
	wrpr	%g0, 15, %pil		! Disable interrupts, too
	wrpr	%g0, %g0, %canrestore	! Our stack is hozed and our PCB
	wrpr	%g0, 7, %cansave	!  probably is too, so blow away
	ba	slowtrap		!  all our register windows.
	 wrpr	%g0, 0x101, %tt
#endif

winfixfill:
	cmp	%g5, 0x0c0		!   so we mask lower bits & compare to 0b 0000 1100 0000
	bne,pt	%icc, winfixspill	! Dump our trap frame -- we will retry the fill when the page is loaded
	 cmp	%g5, 0x080		!   window spill traps are all 0b 0000 10xx xxxx

	!!
	!! This was a fill
	!!
#ifdef TRAPSTATS
	set	_C_LABEL(wfill), %g1
	lduw	[%g1], %g5
	inc	%g5
	stw	%g5, [%g1]
#endif
	btst	TSTATE_PRIV, %g4	! User mode?
	and	%g4, CWP, %g5		! %g4 = %cwp of trap
	wrpr	%g7, 0, %tt
	bz,a,pt	%icc, datafault		! We were in user mode -- normal fault
	 wrpr	%g5, %cwp		! Restore cwp from before fill trap -- regs should now be consisent

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
	set	return_from_trap, %g4			! XXX - need to set %g1 to tstate
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
 * The following is duplicated from datafault:
 */
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! We need to save volatile stuff to interrupt globals
#else
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! We need to save volatile stuff to alternate globals
#endif
	wr	%g0, ASI_DMMU, %asi			! We need to re-load trap info
	ldxa	[%g0 + TLB_TAG_ACCESS] %asi, %g1	! Get fault address from tag access register
	ldxa	[SFAR] %asi, %g2			! sync virt addr; must be read first
	ldxa	[SFSR] %asi, %g3			! get sync fault status register
	stxa	%g0, [SFSR] %asi			! Clear out fault now

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
	saved						! Blow away that one register window we didn't ever use.
	ba,a,pt	%icc, Ldatafault_internal		! Now we should return directly to user mode
	 nop
#endif
winfixspill:
	bne,a,pt	%xcc, datafault			! Was not a spill -- handle it normally
	 wrpr	%g2, 0, %tl				! Restore trap level for now XXXX

	!!
	!! This was a spill
	!!
#if 1
	btst	TSTATE_PRIV, %g4	! From user mode?
	wrpr	%g2, 0, %tl		! We need to load the fault type so we can
	rdpr	%tt, %g5		! overwrite the lower trap and get it to the fault handler
	wrpr	%g1, 0, %tl
	wrpr	%g5, 0, %tt		! Copy over trap type for the fault handler
	and	%g4, CWP, %g5		! find %cwp from trap
	be,a,pt	%xcc, datafault		! Let's do a regular datafault.  When we try a save in datafault we'll
	 wrpr	%g5, 0, %cwp		!  return here and write out all dirty windows.
#endif
	wrpr	%g2, 0, %tl				! Restore trap level for now XXXX
	LDPTR	[%g6 + %lo(CPCB)], %g6	! This is in the locked TLB and should not fault
#ifdef TRAPSTATS
	set	_C_LABEL(wspill), %g7
	lduw	[%g7], %g5
	inc	%g5
	stw	%g5, [%g7]
#endif

	/*
	 * Traverse kernel map to find paddr of cpcb and only us ASI_PHYS_CACHED to
	 * prevent any faults while saving the windows.  BTW if it isn't mapped, we
	 * will trap and hopefully panic.
	 */

!	ba	0f					! DEBUG -- don't use phys addresses
	 wr	%g0, ASI_NUCLEUS, %asi			! In case of problems finding PA
	sethi	%hi(CPUINFO_VA+CI_CTXBUSY), %g1
	LDPTR	[%g1 + %lo(CPUINFO_VA+CI_CTXBUSY)], %g1	! Load start of ctxbusy
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
	set	PANICSTACK-CC64FSZ-STKB, %sp
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
!	rdpr	%tl, %g2				! DEBUG DEBUG -- did we trap somewhere?
	sub	%g2, 1, %g1
	rdpr	%tt, %g2
	wrpr	%g1, 0, %tl				! We will not attempt to re-execute the spill, so dump our trap frame permanently
	wrpr	%g2, 0, %tt				! Move trap type from fault frame here, overwriting spill

	/* Did we save a user or kernel window ? */
!	srax	%g3, 48, %g5				! User or kernel store? (TAG TARGET)
	sllx	%g3, (64-13), %g5			! User or kernel store? (TAG ACCESS)
	sethi	%hi(dcache_size), %g7
	ld	[%g7 + %lo(dcache_size)], %g7
	sethi	%hi(dcache_line_size), %g6
	ld	[%g6 + %lo(dcache_line_size)], %g6
	brnz,pt	%g5, 1f					! User fault -- save windows to pcb
	 sub	%g7, %g6, %g7

	and	%g4, CWP, %g4				! %g4 = %cwp of trap
	wrpr	%g4, 0, %cwp				! Kernel fault -- restore %cwp and force and trap to debugger
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
	 sub	%g7, %g6, %g7
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
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#else
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate
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
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! We need to save volatile stuff to interrupt globals
#else
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! We need to save volatile stuff to alternate globals
#endif
	wr	%g0, ASI_DMMU, %asi			! We need to re-load trap info
	ldxa	[%g0 + TLB_TAG_ACCESS] %asi, %g1	! Get fault address from tag access register
	ldxa	[SFAR] %asi, %g2			! sync virt addr; must be read first
	ldxa	[SFSR] %asi, %g3			! get sync fault status register
	stxa	%g0, [SFSR] %asi			! Clear out fault now

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
Ldatafault_internal:
	INCR64(CPUINFO_VA+CI_NFAULT)			! cnt.v_faults++ (clobbers %o0,%o1)
!	ldx	[%sp + CC64FSZ + STKB + TF_FAULT], %g1	! DEBUG make sure this has not changed
	mov	%g1, %o0				! Move these to the out regs so we can save the globals
	mov	%g2, %o4
	mov	%g3, %o5

	ldxa	[%g0] ASI_AFAR, %o2			! get async fault address
	ldxa	[%g0] ASI_AFSR, %o3			! get async fault status
	mov	-1, %g7
	stxa	%g7, [%g0] ASI_AFSR			! And clear this out, too

	wrpr	%g0, PSTATE_KERN, %pstate		! Get back to normal globals

	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]	! save g1
	rdpr	%tt, %o1					! find out what trap brought us here
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + (2*8)]	! save g2
	rdpr	%tstate, %g1
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + (3*8)]	! (sneak g3 in here)
	rdpr	%tpc, %g2
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + (4*8)]	! sneak in g4
	rdpr	%tnpc, %g3
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + (5*8)]	! sneak in g5
	mov	%g2, %o7					! Make the fault address look like the return address
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + (6*8)]	! sneak in g6
	rd	%y, %g5						! save y
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + (7*8)]	! sneak in g7

	sth	%o1, [%sp + CC64FSZ + STKB + TF_TT]
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]		! set tf.tf_psr, tf.tf_pc
	stx	%g2, [%sp + CC64FSZ + STKB + TF_PC]		! set tf.tf_npc
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]

	rdpr	%pil, %g4
	stb	%g4, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g4, [%sp + CC64FSZ + STKB + TF_OLDPIL]

#if 1
	rdpr	%tl, %g7
	dec	%g7
	movrlz	%g7, %g0, %g7
	wrpr	%g0, %g7, %tl		! Revert to kernel mode
#else
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
	!! In the EMBEDANY memory model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

	/*
	 * Right now the registers have the following values:
	 *
	 *	%o0 -- MMU_TAG_ACCESS
	 *	%o1 -- TT
	 *	%o2 -- afar
	 *	%o3 -- afsr
	 *	%o4 -- sfar
	 *	%o5 -- sfsr
	 */

	cmp	%o1, T_DATA_ERROR
	st	%g5, [%sp + CC64FSZ + STKB + TF_Y]
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi	! Restore default ASI
	be,pn	%icc, data_error
	 wrpr	%g0, PSTATE_INTR, %pstate	! reenable interrupts

	mov	%o0, %o3			! (argument: trap address)
	mov	%g2, %o2			! (argument: trap pc)
	call	_C_LABEL(data_access_fault)	! data_access_fault(&tf, type, 
						!	pc, addr, sfva, sfsr)
	 add	%sp, CC64FSZ + STKB, %o0	! (argument: &tf)

data_recover:
#ifdef TRAPSTATS
	set	_C_LABEL(uintrcnt), %g1
	stw	%g0, [%g1]
	set	_C_LABEL(iveccnt), %g1
	stw	%g0, [%g1]
#endif
	wrpr	%g0, PSTATE_KERN, %pstate		! disable interrupts
	b	return_from_trap			! go return
	 ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1		! Load this for return_from_trap
	NOTREACHED

data_error:
	call	_C_LABEL(data_access_error)	! data_access_error(&tf, type, 
						!	afva, afsr, sfva, sfsr)
	 add	%sp, CC64FSZ + STKB, %o0	! (argument: &tf)
	ba	data_recover
	 nop
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
	mov	TLB_TAG_ACCESS, %g3			! Get real fault page
	sethi	%hi(0x1fff), %g7			! 8K context mask
	ldxa	[%g3] ASI_IMMU, %g3			! from tag access register
	sethi	%hi(CPUINFO_VA+CI_CTXBUSY), %g4
	or	%g7, %lo(0x1fff), %g7
	LDPTR	[%g4 + %lo(CPUINFO_VA+CI_CTXBUSY)], %g4
	srax	%g3, HOLESHIFT, %g5			! Check for valid address
	and	%g3, %g7, %g6				! Isolate context
	sllx	%g6, 3, %g6				! Make it into an offset into ctxbusy
	inc	%g5					! (0 or -1) -> (1 or 0)
	ldx	[%g4+%g6], %g4				! Load up our page table.
#ifdef DEBUG
	/* Make sure we don't try to replace a kernel translation */
	/* This should not be necessary */
	brnz,pt	%g6, 1f					! If user context continue miss
	sethi	%hi(KERNBASE), %g7			! Don't need %lo
	set	0x0800000, %g6				! 8MB
	sub	%g3, %g7, %g7
	cmp	%g7, %g6
	tlu	%xcc, 1; nop
1:	
#endif
	srlx	%g3, STSHIFT, %g6
	cmp	%g5, 1
	bgu,pn %xcc, textfault				! Error!
	 srlx	%g3, PDSHIFT, %g5
	and	%g6, STMASK, %g6
	sll	%g6, 3, %g6
	and	%g5, PDMASK, %g5
	nop

	sll	%g5, 3, %g5
	add	%g6, %g4, %g4
	ldxa	[%g4] ASI_PHYS_CACHED, %g4
	srlx	%g3, PTSHIFT, %g6			! Convert to ptab offset
	and	%g6, PTMASK, %g6
	add	%g5, %g4, %g5
	brz,pn	%g4, textfault				! NULL entry? check somewhere else
	 nop
	
	ldxa	[%g5] ASI_PHYS_CACHED, %g4
	sll	%g6, 3, %g6
	brz,pn	%g4, textfault				! NULL entry? check somewhere else
	 add	%g6, %g4, %g6		
1:
	ldxa	[%g6] ASI_PHYS_CACHED, %g4
	brgez,pn %g4, textfault
	 nop

	/* Check if it's an executable mapping. */
	andcc	%g4, SUN4U_TTE_EXEC, %g0
	bz,pn	%xcc, textfault
	 nop

	or	%g4, SUN4U_TTE_ACCESS, %g7			! Update accessed bit
	btst	SUN4U_TTE_ACCESS, %g4				! Need to update access git?
	bne,pt	%xcc, 1f
	 nop
	casxa	[%g6] ASI_PHYS_CACHED, %g4, %g7		!  and store it
	cmp	%g4, %g7
	bne,pn	%xcc, 1b
	 or	%g4, SUN4U_TTE_ACCESS, %g4			! Update accessed bit
1:
	stx	%g1, [%g2]				! Update TSB entry tag
	stx	%g4, [%g2+8]				! Update TSB entry data
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
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! We need to save volatile stuff to interrupt globals
#else
	wrpr	%g0, PSTATE_KERN|PSTATE_AG, %pstate	! We need to save volatile stuff to alternate globals
#endif
	wr	%g0, ASI_IMMU, %asi
	ldxa	[%g0 + TLB_TAG_ACCESS] %asi, %g1	! Get fault address from tag access register
	ldxa	[SFSR] %asi, %g3			! get sync fault status register
	membar	#LoadStore
	stxa	%g0, [SFSR] %asi			! Clear out old info

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
	INCR64(CPUINFO_VA+CI_NFAULT)			! cnt.v_faults++ (clobbers %o0,%o1)

	mov	%g3, %o3

	wrpr	%g0, PSTATE_KERN, %pstate		! Switch to normal globals
	ldxa	[%g0] ASI_AFSR, %o4			! get async fault status
	ldxa	[%g0] ASI_AFAR, %o5			! get async fault address
	mov	-1, %o0
	stxa	%o0, [%g0] ASI_AFSR			! Clear this out
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]	! save g1
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + (2*8)]	! save g2
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + (3*8)]	! (sneak g3 in here)
	rdpr	%tt, %o1					! Find out what caused this trap
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + (4*8)]	! sneak in g4
	rdpr	%tstate, %g1
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + (5*8)]	! sneak in g5
	rdpr	%tpc, %o2					! sync virt addr; must be read first
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + (6*8)]	! sneak in g6
	rdpr	%tnpc, %g3
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + (7*8)]	! sneak in g7
	rd	%y, %g5						! save y

	/* Finish stackframe, call C trap handler */
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]		! set tf.tf_psr, tf.tf_pc
	sth	%o1, [%sp + CC64FSZ + STKB + TF_TT]		! debug

	stx	%o2, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]		! set tf.tf_npc

	rdpr	%pil, %g4
	stb	%g4, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g4, [%sp + CC64FSZ + STKB + TF_OLDPIL]

	rdpr	%tl, %g7
	dec	%g7
	movrlz	%g7, %g0, %g7
	wrpr	%g0, %g7, %tl		! Revert to kernel mode

	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore default ASI
	flushw						! Get rid of any user windows so we don't deadlock
	
	!! In the EMBEDANY memory model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

	/* Use trap type to see what handler to call */
	cmp	%o1, T_INST_ERROR
	be,pn	%xcc, text_error
	 st	%g5, [%sp + CC64FSZ + STKB + TF_Y]		! set tf.tf_y

	wrpr	%g0, PSTATE_INTR, %pstate	! reenable interrupts
	call	_C_LABEL(text_access_fault)	! mem_access_fault(&tf, type, pc, sfsr)
	 add	%sp, CC64FSZ + STKB, %o0	! (argument: &tf)
text_recover:
	wrpr	%g0, PSTATE_KERN, %pstate	! disable interrupts
	b	return_from_trap		! go return
	 ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1	! Load this for return_from_trap
	NOTREACHED

text_error:
	wrpr	%g0, PSTATE_INTR, %pstate	! reenable interrupts
	call	_C_LABEL(text_access_error)	! mem_access_fault(&tfm type, sfva [pc], sfsr,
						!		afva, afsr);
	 add	%sp, CC64FSZ + STKB, %o0	! (argument: &tf)
	ba	text_recover
	 nop
	NOTREACHED

#ifdef SUN4V

/*
 * Traps for sun4v.
 */

sun4v_dtsb_miss:
	GET_MMFSA %g1				! MMU Fault status area
	add	%g1, 0x48, %g3
	LDPTRA	[%g3] ASI_PHYS_CACHED, %g3	! Data fault address
	add	%g1, 0x50, %g6
	LDPTRA	[%g6] ASI_PHYS_CACHED, %g6	! Data fault context

	GET_CTXBUSY %g4
	sllx	%g6, 3, %g6			! Make it into an offset into ctxbusy
	LDPTR	[%g4 + %g6], %g4		! Load up our page table.

	srax	%g3, HOLESHIFT, %g5		! Check for valid address
	brz,pt	%g5, 0f				! Should be zero or -1
	 inc	%g5				! Make -1 -> 0
	brnz,pn	%g5, sun4v_datatrap		! Error! In hole!
0:
	srlx	%g3, STSHIFT, %g6
	and	%g6, STMASK, %g6		! Index into pm_segs
	sll	%g6, 3, %g6
	add	%g4, %g6, %g4
	LDPTRA	[%g4] ASI_PHYS_CACHED, %g4	! Load page directory pointer
	srlx	%g3, PDSHIFT, %g6
	and	%g6, PDMASK, %g6
	sll	%g6, 3, %g6
	brz,pn	%g4, sun4v_datatrap		! NULL entry? check somewhere else
	 add	%g4, %g6, %g4
	LDPTRA	[%g4] ASI_PHYS_CACHED, %g4	! Load page table pointer

	srlx	%g3, PTSHIFT, %g6		! Convert to ptab offset
	and	%g6, PTMASK, %g6
	sll	%g6, 3, %g6
	brz,pn	%g4, sun4v_datatrap		! NULL entry? check somewhere else
	 add	%g4, %g6, %g6
1:
	LDPTRA	[%g6] ASI_PHYS_CACHED, %g4	! Fetch TTE
	brgez,pn %g4, sun4v_datatrap		! Entry invalid?  Punt
	 or	%g4, A_SUN4V_TLB_ACCESS, %g7	! Update the access bit

	btst	A_SUN4V_TLB_ACCESS, %g4		! Need to update access bit?
	bne,pt	%xcc, 2f
	 nop
	casxa	[%g6] ASI_PHYS_CACHED, %g4, %g7	!  and write it out
	cmp	%g4, %g7
	bne,pn	%xcc, 1b
	 or	%g4, A_SUN4V_TLB_ACCESS, %g4	! Update the access bit
2:
	GET_TSB_DMMU %g2

	/* Construct TSB tag word. */
	add	%g1, 0x50, %g6
	LDPTRA	[%g6] ASI_PHYS_CACHED, %g6	! Data fault context
	mov	%g3, %g1			! Data fault address
	srlx	%g1, 22, %g1			! 63..22 of virt addr
	sllx	%g6, 48, %g6			! context_id in 63..48
	or	%g1, %g6, %g1			! construct TTE tag
	srlx	%g3, PTSHIFT, %g3
	sethi	%hi(_C_LABEL(tsbsize)), %g5
	mov	512, %g6
	ld	[%g5 + %lo(_C_LABEL(tsbsize))], %g5
	sllx	%g6, %g5, %g5			! %g5 = 512 << tsbsize = TSBENTS
	sub	%g5, 1, %g5			! TSBENTS -> offset
	and	%g3, %g5, %g3			! mask out TTE index
	sllx	%g3, 4, %g3			! TTE size is 16 bytes
	add	%g2, %g3, %g2			! location of TTE in ci_tsb_dmmu

	membar	#StoreStore

	STPTR	%g4, [%g2 + 8]		! store TTE data
	STPTR	%g1, [%g2]		! store TTE tag

	retry
	NOTREACHED

sun4v_datatrap:			! branch further based on trap level
	rdpr	%tl, %g1
	dec	%g1
	beq	sun4v_datatrap_tl0
	 nop
	ba	sun4v_datatrap_tl1
	 nop
sun4v_datatrap_tl0:
	/* XXX missing implementaion */
	sir
sun4v_datatrap_tl1:
	/* XXX missing implementaion */
	sir
			
/*
 * End of traps for sun4v.
 */
	
#endif		

/*
 * We're here because we took an alignment fault in NUCLEUS context.
 * This could be a kernel bug or it could be due to saving a user
 * window to an invalid stack pointer.  
 * 
 * If the latter is the case, we could try to emulate unaligned accesses, 
 * but we really don't know where to store the registers since we can't 
 * determine if there's a stack bias.  Or we could store all the regs 
 * into the PCB and punt, until the user program uses up all the CPU's
 * register windows and we run out of places to store them.  So for
 * simplicity we'll just blow them away and enter the trap code which
 * will generate a bus error.  Debugging the problem will be a bit
 * complicated since lots of register windows will be lost, but what
 * can we do?
 */
checkalign:
	rdpr	%tl, %g2
	subcc	%g2, 1, %g1
	bneg,pn	%icc, slowtrap		! Huh?
	 sethi	%hi(CPCB), %g6		! get current pcb

	wrpr	%g1, 0, %tl
	rdpr	%tt, %g7
	rdpr	%tstate, %g4
	andn	%g7, 0x3f, %g5
	cmp	%g5, 0x080		!   window spill traps are all 0b 0000 10xx xxxx
	bne,a,pn	%icc, slowtrap
	 wrpr	%g1, 0, %tl		! Revert TL  XXX wrpr in a delay slot...

#ifdef DEBUG
	cmp	%g7, 0x34		! If we took a datafault just before this trap
	bne,pt	%icc, checkalignspill	! our stack's probably bad so we need to switch somewhere else
	 nop

	!!
	!! Double data fault -- bad stack?
	!!
	wrpr	%g2, %tl		! Restore trap level.
	sir				! Just issue a reset and don't try to recover.
	mov	%fp, %l6		! Save the frame pointer
	set	EINTSTACK+USPACE+CC64FSZ-STKB, %fp ! Set the frame pointer to the middle of the idle stack
	add	%fp, -CC64FSZ, %sp	! Create a stackframe
	wrpr	%g0, 15, %pil		! Disable interrupts, too
	wrpr	%g0, %g0, %canrestore	! Our stack is hozed and our PCB
	wrpr	%g0, 7, %cansave	!  probably is too, so blow away
	ba	slowtrap		!  all our register windows.
	 wrpr	%g0, 0x101, %tt
#endif
checkalignspill:
	/*
         * %g1 -- current tl
	 * %g2 -- original tl
	 * %g4 -- tstate
         * %g7 -- tt
	 */

	and	%g4, CWP, %g5
	wrpr	%g5, %cwp		! Go back to the original register win

	/*
	 * Remember:
	 * 
	 * %otherwin = 0
	 * %cansave = NWINDOWS - 2 - %canrestore
	 */

	rdpr	%otherwin, %g6
	rdpr	%canrestore, %g3
	rdpr	%ver, %g5
	sub	%g3, %g6, %g3		! Calculate %canrestore - %g7
	and	%g5, CWP, %g5		! NWINDOWS-1
	movrlz	%g3, %g0, %g3		! Clamp at zero
	wrpr	%g0, 0, %otherwin
	wrpr	%g3, 0, %canrestore	! This is the new canrestore
	dec	%g5			! NWINDOWS-2
	wrpr	%g5, 0, %cleanwin	! Set cleanwin to max, since we're in-kernel
	sub	%g5, %g3, %g5		! NWINDOWS-2-%canrestore
	wrpr	%g5, 0, %cansave

	wrpr	%g0, T_ALIGN, %tt	! This was an alignment fault 
	/*
	 * Now we need to determine if this was a userland store or not.
	 * Userland stores occur in anything other than the kernel spill
	 * handlers (trap type 09x).
	 */
	and	%g7, 0xff0, %g5
	cmp	%g5, 0x90
	bz,pn	%icc, slowtrap
	 nop
	bclr	TSTATE_PRIV, %g4
	wrpr	%g4, 0, %tstate
	ba,a,pt	%icc, slowtrap
	 nop
	
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
#ifdef DIAGNOSTIC
	/* Make sure kernel stack is aligned */
	btst	0x03, %sp		! 32-bit stack OK?
	 and	%sp, 0x07, %g4		! 64-bit stack OK?
	bz,pt	%icc, 1f
	cmp	%g4, 0x1		! Must end in 0b001
	be,pt	%icc, 1f
	 rdpr	%wstate, %g7
	cmp	%g7, WSTATE_KERN
	bnz,pt	%icc, 1f		! User stack -- we'll blow it away
	 nop
	set	PANICSTACK-CC64FSZ-STKB, %sp
1:
#endif
	rdpr	%tt, %g4
	rdpr	%tstate, %g1
	rdpr	%tpc, %g2
	rdpr	%tnpc, %g3

	TRAP_SETUP(-CC64FSZ-TF_SIZE)
Lslowtrap_reenter:
	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
	mov	%g4, %o1		! (type)
	stx	%g2, [%sp + CC64FSZ + STKB + TF_PC]
	rd	%y, %g5
	stx	%g3, [%sp + CC64FSZ + STKB + TF_NPC]
	mov	%g1, %o3		! (pstate)
	st	%g5, [%sp + CC64FSZ + STKB + TF_Y]
	mov	%g2, %o2		! (pc)
	sth	%o1, [%sp + CC64FSZ + STKB + TF_TT]! debug

	wrpr	%g0, PSTATE_KERN, %pstate		! Get back to normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + (1*8)]
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + (2*8)]
	add	%sp, CC64FSZ + STKB, %o0		! (&tf)
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + (3*8)]
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + (4*8)]
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + (5*8)]
	rdpr	%pil, %g5
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + (6*8)]
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + (7*8)]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]
	/*
	 * Phew, ready to enable traps and call C code.
	 */
	rdpr	%tl, %g1
	dec	%g1
	movrlz	%g1, %g0, %g1
	wrpr	%g0, %g1, %tl		! Revert to kernel mode
	!! In the EMBEDANY memory model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4

	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore default ASI
	wrpr	%g0, PSTATE_INTR, %pstate	! traps on again
	call	_C_LABEL(trap)			! trap(tf, type, pc, pstate)
	 nop

	b	return_from_trap
	 ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1	! Load this for return_from_trap
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
	sethi	%hi(EINTSTACK-STKB), %g5
	sethi	%hi(EINTSTACK-INTSTACK), %g7
	or	%g5, %lo(EINTSTACK-STKB), %g5
	dec	%g7
	sub	%g5, %sp, %g5
	sethi	%hi(CPCB), %g6
	andncc	%g5, %g7, %g0
	bnz,pt	%xcc, Lslowtrap_reenter
	 LDPTR	[%g6 + %lo(CPCB)], %g7
	set	USPACE-CC64FSZ-TF_SIZE-STKB, %g5
	add	%g7, %g5, %g6
	SET_SP_REDZONE(%g7, %g5)
#ifdef DEBUG
	stx	%g1, [%g6 + CC64FSZ + STKB + TF_FAULT]		! Generate a new trapframe
#endif
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
XXX ddb_regs is now ddb-regp and is a pointer not a symbol.
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
ENTRY_NOPROFILE(kgdb_trap_glue)
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

#ifdef NOTDEF_DEBUG
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
#ifdef NOTDEF_DEBUG
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
#ifdef NOTDEF_DEBUG
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
!	st	%l0, [%l1 + PCB_WIM]	! cpcb->pcb_wim = CWP;
	save	%g0, %g0, %g0		! back to window to reload
!	LOADWIN(%sp)
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
	rdpr	%tt, %o1	! debug
	sth	%o1, [%sp + CC64FSZ + STKB + TF_TT]! debug
#endif

	wrpr	%g0, PSTATE_KERN, %pstate	! Get back to normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + ( 1*8)]
	mov	%g1, %o1			! code
	rdpr	%tpc, %o2			! (pc)
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + ( 2*8)]
	rdpr	%tstate, %g1
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + ( 3*8)]
	rdpr	%tnpc, %o3
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + ( 4*8)]
	rd	%y, %o4
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + ( 5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + ( 6*8)]
	wrpr	%g0, 0, %tl			! return to tl=0
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + ( 7*8)]
	add	%sp, CC64FSZ + STKB, %o0	! (&tf)

	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
	stx	%o2, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%o3, [%sp + CC64FSZ + STKB + TF_NPC]
	st	%o4, [%sp + CC64FSZ + STKB + TF_Y]

	rdpr	%pil, %g5
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]

	!! In the EMBEDANY memory model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi	! Restore default ASI

	sethi	%hi(CURLWP), %l1
	LDPTR	[%l1 + %lo(CURLWP)], %l1
	LDPTR	[%l1 + L_PROC], %l1		! now %l1 points to p
	LDPTR	[%l1 + P_MD_SYSCALL], %l1
	call	%l1
	 wrpr	%g0, PSTATE_INTR, %pstate	! turn on interrupts

	/* see `lwp_trampoline' for the reason for this label */
return_from_syscall:
	wrpr	%g0, PSTATE_KERN, %pstate	! Disable intterrupts
	wrpr	%g0, 0, %tl			! Return to tl==0
	b	return_from_trap
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
 * Then interrupt_vector uses the interrupt level in the intrhand
 * to issue a softint of the appropriate level.  The softint handler
 * figures out what level interrupt it's handling and pulls the first
 * intrhand pointer out of the intrpending array for that interrupt
 * level, puts a NULL in its place, clears the interrupt generator,
 * and invokes the interrupt handler.
 */

/* intrpending array is now in per-CPU structure. */

#ifdef DEBUG
#define INTRDEBUG_VECTOR	0x1
#define INTRDEBUG_LEVEL		0x2
#define INTRDEBUG_FUNC		0x4
#define INTRDEBUG_SPUR		0x8
	.data
	.globl	_C_LABEL(intrdebug)
_C_LABEL(intrdebug):	.word 0x0
/*
 * Note: we use the local label `97' to branch forward to, to skip
 * actual debugging code following a `intrdebug' bit test.
 */
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
	mov	IRDR_0H, %g7
	ldxa	[%g7] ASI_IRDR, %g7	! Get interrupt number
	membar	#Sync

	btst	IRSR_BUSY, %g1
	bz,pn	%icc, 3f		! spurious interrupt
#ifdef MULTIPROCESSOR
	 sethi	%hi(KERNBASE), %g1

	cmp	%g7, %g1
	bl,a,pt	%xcc, Lsoftint_regular	! >= KERNBASE is a fast cross-call
	 and	%g7, (MAXINTNUM-1), %g7	! XXX make sun4us work

	mov	IRDR_1H, %g2
	ldxa	[%g2] ASI_IRDR, %g2	! Get IPI handler argument 1
	mov	IRDR_2H, %g3
	ldxa	[%g3] ASI_IRDR, %g3	! Get IPI handler argument 2

	stxa	%g0, [%g0] ASI_IRSR	! Ack IRQ
	membar	#Sync			! Should not be needed due to retry

	jmpl	%g7, %g0
	 nop
#else
	and	%g7, (MAXINTNUM-1), %g7	! XXX make sun4us work
#endif

Lsoftint_regular:
	stxa	%g0, [%g0] ASI_IRSR	! Ack IRQ
	membar	#Sync			! Should not be needed due to retry
	sethi	%hi(_C_LABEL(intrlev)), %g3
	sllx	%g7, PTRSHFT, %g5	! Calculate entry number
	or	%g3, %lo(_C_LABEL(intrlev)), %g3
	LDPTR	[%g3 + %g5], %g5	! We have a pointer to the handler
	brz,pn	%g5, 3f			! NULL means it isn't registered yet.  Skip it.
	 nop

	! increment per-ivec counter
	ldx	[%g5 + IH_CNT], %g1
	inc	%g1
	stx	%g1, [%g5 + IH_CNT]

setup_sparcintr:
	LDPTR	[%g5+IH_PEND], %g6	! Read pending flag
	brnz,pn	%g6, ret_from_intr_vector ! Skip it if it's running
	 ldub	[%g5+IH_PIL], %g6	! Read interrupt level
	sethi	%hi(CPUINFO_VA+CI_INTRPENDING), %g1
	sll	%g6, PTRSHFT, %g3	! Find start of table for this IPL
	or	%g1, %lo(CPUINFO_VA+CI_INTRPENDING), %g1
	add	%g1, %g3, %g1
1:
	LDPTR	[%g1], %g3		! Load list head
	STPTR	%g3, [%g5+IH_PEND]	! Link our intrhand node in
	mov	%g5, %g7
	CASPTRA	[%g1] ASI_N, %g3, %g7
	cmp	%g7, %g3		! Did it work?
	bne,pn	CCCR, 1b		! No, try again
	 .empty
2:
#ifdef NOT_DEBUG
	set	_C_LABEL(intrdebug), %g7
	ld	[%g7], %g7
	btst	INTRDEBUG_VECTOR, %g7
	bz,pt	%icc, 97f
	 nop

	cmp	%g6, 0xa		! ignore clock interrupts?
	bz,pt	%icc, 97f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0,\
	    "interrupt_vector: number %lx softint mask %lx pil %lu slot %p\n")
	mov	%g2, %o1
	rdpr	%pil, %o3
	mov	%g1, %o4
	GLOBTOLOC
	clr	%g4
	call	prom_printf
	 mov	%g6, %o2
	LOCTOGLOB
	restore
97:
#endif
	mov	1, %g7
	sll	%g7, %g6, %g6
	wr	%g6, 0, SET_SOFTINT	! Invoke a softint

	.global ret_from_intr_vector
ret_from_intr_vector:
	retry
	NOTREACHED

3:
#ifdef NOT_DEBUG	/* always do this */
	set	_C_LABEL(intrdebug), %g6
	ld	[%g6], %g6
	btst	INTRDEBUG_SPUR, %g6
	bz,pt	%icc, 97f
	 nop
#endif
#if 1
	set	PANICSTACK-STKB, %g1	! Use panic stack temporarily
	save	%g1, -CC64FSZ, %sp	! Get a clean register window
	LOAD_ASCIZ(%o0, "interrupt_vector: spurious vector %lx at pil %d\n")
	mov	%g7, %o1
	GLOBTOLOC
	clr	%g4
	call	prom_printf
	 rdpr	%pil, %o2
	LOCTOGLOB
	restore
97:
#endif
	ba,a	ret_from_intr_vector
	 nop				! XXX spitfire bug?

sun4v_cpu_mondo:
	mov	0x3c0, %g1			 ! CPU Mondo Queue Head
	ldxa	[%g1] ASI_QUEUE, %g2		 ! fetch index value for head
	set	CPUINFO_VA, %g3
	LDPTR	[%g3 + CI_PADDR], %g3
	add	%g3, CI_CPUMQ, %g3	
	ldxa	[%g3] ASI_PHYS_CACHED, %g3	 ! fetch head element
	ldxa	[%g3 + %g2] ASI_PHYS_CACHED, %g4 ! fetch func 
	add	%g2, 8, %g5
	ldxa	[%g3 + %g5] ASI_PHYS_CACHED, %g5 ! fetch arg1
	add	%g2, 16, %g6
	ldxa	[%g3 + %g6] ASI_PHYS_CACHED, %g6 ! fetch arg2
	add	%g2, 64, %g2			 ! point to next element in queue
	and	%g2, 0x7ff, %g2			 ! modulo queue size 2048 (32*64)
	stxa	%g2, [%g1] ASI_QUEUE		 ! update head index
	membar	#Sync

	mov	%g4, %g2
	mov	%g5, %g3
	mov	%g6, %g5
	jmpl	%g2, %g0
	 nop			! No store here!
	retry
	NOTREACHED

	
/*
 * Ultra1 and Ultra2 CPUs use soft interrupts for everything.  What we do
 * on a soft interrupt, is we should check which bits in SOFTINT(%asr22)
 * are set, handle those interrupts, then clear them by setting the
 * appropriate bits in CLEAR_SOFTINT(%asr21).
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

ENTRY_NOPROFILE(sparc_interrupt)
#ifdef TRAPS_USE_IG
	! This is for interrupt debugging
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif	
	/*
	 * If this is a %tick or %stick softint, clear it then call
	 * interrupt_vector. Only one of them should be enabled at any given
	 * time.
	 */
	rd	SOFTINT, %g1
	set	TICK_INT|STICK_INT, %g5
	andcc	%g5, %g1, %g5
	bz,pt	%icc, 0f
	 sethi	%hi(CPUINFO_VA+CI_TICK_IH), %g3
	wr	%g0, %g5, CLEAR_SOFTINT
	ba,pt	%icc, setup_sparcintr
	 LDPTR	[%g3 + %lo(CPUINFO_VA+CI_TICK_IH)], %g5
0:

#ifdef TRAPSTATS
	sethi	%hi(_C_LABEL(kintrcnt)), %g1
	sethi	%hi(_C_LABEL(uintrcnt)), %g2
	or	%g1, %lo(_C_LABEL(kintrcnt)), %g1
	or	%g1, %lo(_C_LABEL(uintrcnt)), %g2
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
#ifdef SUN4V
	sethi	%hi(cputyp), %g5
	ld	[%g5 + %lo(cputyp)], %g5
	cmp	%g5, CPU_SUN4V
	bne,pt	%icc, 1f
	 nop
	NORMAL_GLOBALS_SUN4V
	! Save the normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + ( 1*8)]
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + ( 2*8)]
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + ( 3*8)]
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + ( 4*8)]
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + ( 5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + ( 6*8)]
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + ( 7*8)]

	/*
	 * In the EMBEDANY memory model %g4 points to the start of the
	 * data segment.  In our case we need to clear it before calling
	 * any C-code.
	 */
	clr	%g4

	ba	2f
	 nop
1:		
#endif
	NORMAL_GLOBALS_SUN4U
	! Save the normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + ( 1*8)]
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + ( 2*8)]
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + ( 3*8)]
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + ( 4*8)]
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + ( 5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + ( 6*8)]
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + ( 7*8)]

	/*
	 * In the EMBEDANY memory model %g4 points to the start of the
	 * data segment.  In our case we need to clear it before calling
	 * any C-code.
	 */
	clr	%g4

	flushw			! Do not remove this insn -- causes interrupt loss

2:
	rd	%y, %l6
	INCR64(CPUINFO_VA+CI_NINTR)	! cnt.v_ints++ (clobbers %o0,%o1)
	rdpr	%tt, %l5		! Find out our current IPL
	rdpr	%tstate, %l0
	rdpr	%tpc, %l1
	rdpr	%tnpc, %l2
	rdpr	%tl, %l3		! Dump our trap frame now we have taken the IRQ
	stw	%l6, [%sp + CC64FSZ + STKB + TF_Y]	! Silly, but we need to save this for rft
	dec	%l3
	wrpr	%g0, %l3, %tl
	sth	%l5, [%sp + CC64FSZ + STKB + TF_TT]! debug
	stx	%l0, [%sp + CC64FSZ + STKB + TF_TSTATE]	! set up intrframe/clockframe
	stx	%l1, [%sp + CC64FSZ + STKB + TF_PC]
	btst	TSTATE_PRIV, %l0		! User mode?
	stx	%l2, [%sp + CC64FSZ + STKB + TF_NPC]
	
	sub	%l5, 0x40, %l6			! Convert to interrupt level
	sethi	%hi(_C_LABEL(intr_evcnts)), %l4
	stb	%l6, [%sp + CC64FSZ + STKB + TF_PIL]	! set up intrframe/clockframe
	rdpr	%pil, %o1
	mulx	%l6, EVC_SIZE, %l3
	or	%l4, %lo(_C_LABEL(intr_evcnts)), %l4	! intrcnt[intlev]++;
	stb	%o1, [%sp + CC64FSZ + STKB + TF_OLDPIL]	! old %pil
	ldx	[%l4 + %l3], %o0
	add	%l4, %l3, %l4
	clr	%l5			! Zero handled count
#ifdef MULTIPROCESSOR
	mov	1, %l3			! Ack softint
1:	add	%o0, 1, %l7
	casxa	[%l4] ASI_N, %o0, %l7
	cmp	%o0, %l7
	bne,a,pn %xcc, 1b		! retry if changed
	 mov	%l7, %o0
#else
	inc	%o0	
	mov	1, %l3			! Ack softint
	stx	%o0, [%l4]
#endif
	sll	%l3, %l6, %l3		! Generate IRQ mask
	
	wrpr	%l6, %pil

#define SOFTINT_INT \
	(1<<IPL_SOFTCLOCK|1<<IPL_SOFTBIO|1<<IPL_SOFTNET|1<<IPL_SOFTSERIAL)

	! Increment the per-cpu interrupt depth in case of hardintrs
	btst	SOFTINT_INT, %l3
	bnz,pn	%icc, sparc_intr_retry
	 sethi	%hi(CPUINFO_VA+CI_IDEPTH), %l1
	ld	[%l1 + %lo(CPUINFO_VA+CI_IDEPTH)], %l2
	inc	%l2
	st	%l2, [%l1 + %lo(CPUINFO_VA+CI_IDEPTH)]

sparc_intr_retry:
	wr	%l3, 0, CLEAR_SOFTINT	! (don't clear possible %tick IRQ)
	sethi	%hi(CPUINFO_VA+CI_INTRPENDING), %l4
	sll	%l6, PTRSHFT, %l2
	or	%l4, %lo(CPUINFO_VA+CI_INTRPENDING), %l4
	add	%l2, %l4, %l4

1:
	membar	#StoreLoad		! Make sure any failed casxa insns complete
	LDPTR	[%l4], %l2		! Check a slot
	cmp	%l2, -1
	beq,pn	CCCR, intrcmplt		! Empty list?
	 mov	-1, %l7
	membar	#LoadStore
	CASPTRA	[%l4] ASI_N, %l2, %l7	! Grab the entire list
	cmp	%l7, %l2
	bne,pn	CCCR, 1b
	 add	%sp, CC64FSZ+STKB, %o2	! tf = %sp + CC64FSZ + STKB
	LDPTR	[%l2 + IH_PEND], %l7
	cmp	%l7, -1			! Last slot?
	be,pt	CCCR, 3f
	 membar	#LoadStore

	/*
	 * Reverse a pending list since setup_sparcintr/send_softint
	 * makes it in a LIFO order.
	 */
	mov	-1, %o0			! prev = -1
1:	STPTR	%o0, [%l2 + IH_PEND]	! ih->ih_pending = prev
	mov	%l2, %o0		! prev = ih
	mov	%l7, %l2		! ih = ih->ih_pending
	LDPTR	[%l2 + IH_PEND], %l7
	cmp	%l7, -1			! Last slot?
	bne,pn	CCCR, 1b
	 membar	#LoadStore
	ba,pt	CCCR, 3f
	 mov	%o0, %l7		! save ih->ih_pending

2:
	add	%sp, CC64FSZ+STKB, %o2	! tf = %sp + CC64FSZ + STKB
	LDPTR	[%l2 + IH_PEND], %l7	! save ih->ih_pending
	membar	#LoadStore
3:
	STPTR	%g0, [%l2 + IH_PEND]	! Clear pending flag
	membar	#Sync
	LDPTR	[%l2 + IH_FUN], %o4	! ih->ih_fun
	LDPTR	[%l2 + IH_ARG], %o0	! ih->ih_arg

#ifdef NOT_DEBUG
	set	_C_LABEL(intrdebug), %o3
	ld	[%o2], %o3
	btst	INTRDEBUG_FUNC, %o3
	bz,a,pt	%icc, 97f
	 nop

	cmp	%l6, 0xa		! ignore clock interrupts?
	bz,pt	%icc, 97f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "sparc_interrupt: func %p arg %p\n")
	mov	%i0, %o2		! arg
	GLOBTOLOC
	call	prom_printf
	 mov	%i4, %o1		! func
	LOCTOGLOB
	restore
97:
	mov	%l4, %o1
#endif

	wrpr	%g0, PSTATE_INTR, %pstate	! Reenable interrupts
	jmpl	%o4, %o7		! handled = (*ih->ih_fun)(...)
	 movrz	%o0, %o2, %o0		! arg = (arg == 0) ? arg : tf
	wrpr	%g0, PSTATE_KERN, %pstate	! Disable interrupts
	LDPTR	[%l2 + IH_CLR], %l1
	membar	#Sync

	brz,pn	%l1, 0f
	 add	%l5, %o0, %l5
	stx	%g0, [%l1]		! Clear intr source
	membar	#Sync			! Should not be needed
0:
	cmp	%l7, -1
	bne,pn	CCCR, 2b		! 'Nother?
	 mov	%l7, %l2

intrcmplt:
	/*
	 * Re-read SOFTINT to see if any new  pending interrupts
	 * at this level.
	 */
	mov	1, %l3			! Ack softint
	rd	SOFTINT, %l7		! %l5 contains #intr handled.
	sll	%l3, %l6, %l3		! Generate IRQ mask
	btst	%l3, %l7		! leave mask in %l3 for retry code
	bnz,pn	%icc, sparc_intr_retry
	 mov	1, %l5			! initialize intr count for next run

	! Decrement this cpu's interrupt depth in case of hardintrs
	btst	SOFTINT_INT, %l3
	bnz,pn	%icc, 1f
	 sethi	%hi(CPUINFO_VA+CI_IDEPTH), %l4
	ld	[%l4 + %lo(CPUINFO_VA+CI_IDEPTH)], %l5
	dec	%l5
	st	%l5, [%l4 + %lo(CPUINFO_VA+CI_IDEPTH)]
1:

#ifdef NOT_DEBUG
	set	_C_LABEL(intrdebug), %o2
	ld	[%o2], %o2
	btst	INTRDEBUG_FUNC, %o2
	bz,a,pt	%icc, 97f
	 nop

	cmp	%l6, 0xa		! ignore clock interrupts?
	bz,pt	%icc, 97f
	 nop

	STACKFRAME(-CC64FSZ)		! Get a clean register window
	LOAD_ASCIZ(%o0, "sparc_interrupt:  done\n")
	GLOBTOLOC
	call	prom_printf
	 nop
	LOCTOGLOB
	restore
97:
#endif

	ldub	[%sp + CC64FSZ + STKB + TF_OLDPIL], %l3	! restore old %pil
	wrpr	%l3, 0, %pil

	b	return_from_trap
	 ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1	! Load this for return_from_trap

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

/*
 * Various return-from-trap routines (see return_from_trap).
 */

/*
 * Return from trap.
 * registers are:
 *
 *	[%sp + CC64FSZ + STKB] => trap frame
 *      %g1 => tstate from trap frame
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

	!!
	!! We'll make sure we flush our pcb here, rather than later.
	!!
!	ldx	[%sp + CC64FSZ + STKB + TF_TSTATE], %g1	! already passed in, no need to reload
	btst	TSTATE_PRIV, %g1			! returning to userland?

	!!
	!! Let all pending interrupts drain before returning to userland
	!!
	bnz,pn	%icc, 1f				! Returning to userland?
	 nop
	ENABLE_INTERRUPTS %g5
	wrpr	%g0, %g0, %pil				! Lower IPL
1:
	!! Make sure we have no IRQs
	DISABLE_INTERRUPTS %g5

#ifdef SUN4V
	sethi	%hi(cputyp), %g5
	ld	[%g5 + %lo(cputyp)], %g5
	cmp	%g5, CPU_SUN4V
	bne,pt	%icc, 1f
	 nop
	!! Make sure we have normal globals
	NORMAL_GLOBALS_SUN4V
	/* Restore normal globals */
	ldx	[%sp + CC64FSZ + STKB + TF_G + (1*8)], %g1
	ldx	[%sp + CC64FSZ + STKB + TF_G + (2*8)], %g2
	ldx	[%sp + CC64FSZ + STKB + TF_G + (3*8)], %g3
	ldx	[%sp + CC64FSZ + STKB + TF_G + (4*8)], %g4
	ldx	[%sp + CC64FSZ + STKB + TF_G + (5*8)], %g5
	ldx	[%sp + CC64FSZ + STKB + TF_G + (6*8)], %g6
	ldx	[%sp + CC64FSZ + STKB + TF_G + (7*8)], %g7
	/* Switch to alternate globals */
	ALTERNATE_GLOBALS_SUN4V
	ba	2f
	 nop
1:		
#endif
	!! Make sure we have normal globals
	NORMAL_GLOBALS_SUN4U
	/* Restore normal globals */
	ldx	[%sp + CC64FSZ + STKB + TF_G + (1*8)], %g1
	ldx	[%sp + CC64FSZ + STKB + TF_G + (2*8)], %g2
	ldx	[%sp + CC64FSZ + STKB + TF_G + (3*8)], %g3
	ldx	[%sp + CC64FSZ + STKB + TF_G + (4*8)], %g4
	ldx	[%sp + CC64FSZ + STKB + TF_G + (5*8)], %g5
	ldx	[%sp + CC64FSZ + STKB + TF_G + (6*8)], %g6
	ldx	[%sp + CC64FSZ + STKB + TF_G + (7*8)], %g7
	/* Switch to alternate globals */
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#else
	ALTERNATE_GLOBALS_SUN4U
#endif
2:		
	
	/* Load outs */
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

#ifdef NOTDEF_DEBUG
	ldub	[%sp + CC64FSZ + STKB + TF_PIL], %g5		! restore %pil
	wrpr	%g5, %pil				! DEBUG
#endif

	/* Returning to user mode or kernel mode? */
	btst	TSTATE_PRIV, %g1		! returning to userland?
	bz,pt	%icc, rft_user
	 sethi	%hi(CPUINFO_VA+CI_WANT_AST), %g7	! first instr of rft_user

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
	restore
	rdpr	%tstate, %g1			! Since we may have trapped our regs may be toast
	rdpr	%cwp, %g2
	andn	%g1, CWP, %g1
	wrpr	%g1, %g2, %tstate		! Put %cwp in %tstate
	CLRTT
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
#if defined(DDB) && defined(MULTIPROCESSOR)
	set	sparc64_ipi_pause_trap_point, %g1
	rdpr	%tpc, %g2
	cmp	%g1, %g2
	bne,pt	%icc, 0f
	 nop
	done
0:
#endif
	retry
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
!	sethi	%hi(CPUINFO_VA+CI_WANT_AST), %g7	! (done above)
	lduw	[%g7 + %lo(CPUINFO_VA+CI_WANT_AST)], %g7! want AST trap?
	brnz,pn	%g7, softtrap			! yes, re-enter trap with type T_AST
	 mov	T_AST, %g4

#ifdef NOTDEF_DEBUG
	sethi	%hi(CPCB), %g4
	LDPTR	[%g4 + %lo(CPCB)], %g4
	ldub	[%g4 + PCB_NSAVED], %g4		! nsaved
	brz,pt	%g4, 2f		! Only print if nsaved <> 0
	 nop

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
1:
	.data
	.asciz	"rft_user: nsaved=%x pc=%d ctx=%x sp=%x npc=%p\n"
	_ALIGN
	.text
#endif

	/*
	 * NB: only need to do this after a cache miss
	 */
#ifdef TRAPSTATS
	set	_C_LABEL(rftucnt), %g6
	lduw	[%g6], %g7
	inc	%g7
	stw	%g7, [%g6]
#endif
	/*
	 * Now check to see if any regs are saved in the pcb and restore them.
	 *
	 * Here we need to undo the damage caused by switching to a kernel 
	 * stack.
	 *
	 * We will use alternate globals %g4..%g7 because %g1..%g3 are used
	 * by the data fault trap handlers and we don't want possible conflict.
	 */

	sethi	%hi(CPCB), %g6
	rdpr	%otherwin, %g7			! restore register window controls
#ifdef DEBUG
	rdpr	%canrestore, %g5		! DEBUG
	tst	%g5				! DEBUG
	tnz	%icc, 1; nop			! DEBUG
!	mov	%g0, %g5			! There should be *NO* %canrestore
	add	%g7, %g5, %g7			! DEBUG
#endif
	wrpr	%g0, %g7, %canrestore
	LDPTR	[%g6 + %lo(CPCB)], %g6
	wrpr	%g0, 0, %otherwin

	ldub	[%g6 + PCB_NSAVED], %g7		! Any saved reg windows?
	wrpr	%g0, WSTATE_USER, %wstate	! Need to know where our sp points

#ifdef DEBUG
	set	rft_wcnt, %g4	! Keep track of all the windows we restored
	stw	%g7, [%g4]
#endif

	brz,pt	%g7, 5f				! No saved reg wins
	 nop
	dec	%g7				! We can do this now or later.  Move to last entry

#ifdef DEBUG
	rdpr	%canrestore, %g4			! DEBUG Make sure we've restored everything
	brnz,a,pn	%g4, 0f				! DEBUG
	 sir						! DEBUG we should NOT have any usable windows here
0:							! DEBUG
	wrpr	%g0, 5, %tl
#endif
	rdpr	%otherwin, %g4
	sll	%g7, 7, %g5			! calculate ptr into rw64 array 8*16 == 128 or 7 bits
	brz,pt	%g4, 6f				! We should not have any user windows left
	 add	%g5, %g6, %g5

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

	rdpr	%ver, %g5
	stb	%g0, [%g6 + PCB_NSAVED]			! Clear them out so we won't do this again
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
	 sethi	%hi(CPUINFO_VA+CI_WANT_AST), %g7
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

	wr	%g0, ASI_DMMU, %asi		! restore the user context
	ldxa	[CTX_SECONDARY] %asi, %g4
	sethi	%hi(KERNBASE), %g7		! Should not be needed due to retry
	stxa	%g4, [CTX_PRIMARY] %asi
	membar	#Sync				! Should not be needed due to retry
	flush	%g7				! Should not be needed due to retry
	CLRTT
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

/*
 * Kernel entry point.
 *
 * The contract between bootloader and kernel is:
 *
 * %o0		OpenFirmware entry point, to keep Sun's updaters happy
 * %o1		Address of boot information vector (see bootinfo.h)
 * %o2		Length of the vector, in bytes
 * %o3		OpenFirmware entry point, to mimic Sun bootloader behavior
 * %o4		OpenFirmware, to meet earlier NetBSD kernels expectations
 */
	.align	8
start:
dostart:
	mov	1, %g1
	sllx	%g1, 63, %g1
	wr	%g1, TICK_CMPR	! XXXXXXX clear and disable %tick_cmpr for now
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
	wr	%g0, FPRS_FEF, %fprs		! Turn on FPU

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

	call	_C_LABEL(bootstrap)
	 clr	%g4				! Clear data segment pointer

/*
 * Initialize the boot CPU.  Basically:
 *
 *	Locate the cpu_info structure for this CPU.
 *	Establish a locked mapping for interrupt stack.
 *	Switch to the initial stack.
 *	Call the routine passed in in cpu_info->ci_spinup
 */

#ifdef NO_VCACHE
#define	SUN4U_TTE_DATABITS	SUN4U_TTE_L|SUN4U_TTE_CP|SUN4U_TTE_P|SUN4U_TTE_W
#else
#define	SUN4U_TTE_DATABITS	SUN4U_TTE_L|SUN4U_TTE_CP|SUN4U_TTE_CV|SUN4U_TTE_P|SUN4U_TTE_W
#endif


ENTRY_NOPROFILE(cpu_initialize)	/* for cosmetic reasons - nicer backtrace */

	/* Cache the cputyp in %l6 for later use below */
	sethi	%hi(cputyp), %l6
	ld	[%l6 + %lo(cputyp)], %l6

	/*
	 * Step 5: is no more.
	 */
	
	/*
	 * Step 6: hunt through cpus list and find the one that matches our cpuid
	 */

	call	_C_LABEL(cpu_myid)	! Retrieve cpuid in %o0
	 mov	%g0, %o0
	
	sethi	%hi(_C_LABEL(cpus)), %l1
	LDPTR	[%l1 + %lo(_C_LABEL(cpus))], %l1
0:
	ld	[%l1 + CI_CPUID], %l3		! Load CPUID
	cmp	%l3, %o0			! Does it match?
	bne,a,pt	%icc, 0b		! no
	 LDPTR	[%l1 + CI_NEXT], %l1		! Load next cpu_info pointer

	/*
	 * Get pointer to our cpu_info struct
	 */
	mov	%l1, %l7			! save cpu_info pointer
	ldx	[%l1 + CI_PADDR], %l1		! Load the interrupt stack's PA
#ifdef SUN4V
	cmp	%l6, CPU_SUN4V
	bne,pt	%icc, 3f
	 nop

	/* sun4v */
	call	_C_LABEL(pmap_setup_intstack_sun4v)	! Call nice C function for mapping INTSTACK
	 mov	%l1, %o0
	ba	4f
	 nop
3:
#endif
	/* sun4u */
	sethi	%hi(0xa0000000), %l2		! V=1|SZ=01|NFO=0|IE=0
	sllx	%l2, 32, %l2			! Shift it into place

	mov	-1, %l3				! Create a nice mask
	sllx	%l3, 43, %l4			! Mask off high bits
	or	%l4, 0xfff, %l4			! We can just load this in 12 (of 13) bits

	andn	%l1, %l4, %l1			! Mask the phys page number

	or	%l2, %l1, %l1			! Now take care of the high bits
	or	%l1, SUN4U_TTE_DATABITS, %l2	! And low bits:	L=1|CP=1|CV=?|E=0|P=1|W=1|G=0

	!!
	!!  Now, map in the interrupt stack as context==0
	!!
	set	TLB_TAG_ACCESS, %l5
	set	INTSTACK, %l0
	stxa	%l0, [%l5] ASI_DMMU		! Make DMMU point to it
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Store it
	membar	#Sync
4:

	!! Setup kernel stack (we rely on curlwp on this cpu
	!! being lwp0 here and its uarea is mapped special
	!! and already accessible here)
	flushw
	LDPTR	[%l7 + CI_CPCB], %l0		! load PCB/uarea pointer
	set	USPACE - TF_SIZE - CC64FSZ, %l1
 	add	%l1, %l0, %l0
#ifdef _LP64
	andn	%l0, 0x0f, %l0			! Needs to be 16-byte aligned
	sub	%l0, BIAS, %l0			! and biased
#endif
	mov	%l0, %sp
	flushw

#ifdef DEBUG
	set	_C_LABEL(pmapdebug), %o1
	ld	[%o1], %o1
	sethi	%hi(0x40000), %o2
	btst	%o2, %o1
	bz	0f
	
	set	1f, %o0		! Debug printf
	call	_C_LABEL(prom_printf)
	 nop
	.data
1:
	.asciz	"Setting trap base...\n"
	_ALIGN
	.text
0:	
#endif
	/*
	 * Step 7: change the trap base register, and install our TSB pointers
	 */

	/*
	 * install our TSB pointers
	 */

#ifdef SUN4V
	cmp	%l6, CPU_SUN4V
	bne,pt	%icc, 5f
	 nop

	/* sun4v */
	LDPTR	[%l7 + CI_TSB_DESC], %o0
	call	_C_LABEL(pmap_setup_tsb_sun4v)
	 nop
	ba	1f
	 nop
5:
#endif
	/* sun4u */
	sethi	%hi(_C_LABEL(tsbsize)), %l2
	sethi	%hi(0x1fff), %l3
	sethi	%hi(TSB), %l4
	LDPTR	[%l7 + CI_TSB_DMMU], %l0
	LDPTR	[%l7 + CI_TSB_IMMU], %l1
	ld	[%l2 + %lo(_C_LABEL(tsbsize))], %l2
	or	%l3, %lo(0x1fff), %l3
	or	%l4, %lo(TSB), %l4

	andn	%l0, %l3, %l0			! Mask off size and split bits
	or	%l0, %l2, %l0			! Make a TSB pointer
	stxa	%l0, [%l4] ASI_DMMU		! Install data TSB pointer

	andn	%l1, %l3, %l1			! Mask off size and split bits
	or	%l1, %l2, %l1			! Make a TSB pointer
	stxa	%l1, [%l4] ASI_IMMU		! Install instruction TSB pointer
	membar	#Sync
	set	1f, %l1
	flush	%l1
1:

	/* set trap table */
#ifdef SUN4V
	cmp	%l6, CPU_SUN4V
	bne,pt	%icc, 6f
	 nop
	/* sun4v */
	set	_C_LABEL(trapbase_sun4v), %l1
	GET_MMFSA %o1
	call	_C_LABEL(prom_set_trap_table_sun4v)	! Now we should be running 100% from our handlers
	 mov	%l1, %o0
	
	ba	7f
	 nop
6:	
#endif	
	/* sun4u */
	set	_C_LABEL(trapbase), %l1
	call	_C_LABEL(prom_set_trap_table_sun4u)	! Now we should be running 100% from our handlers
	 mov	%l1, %o0
7:
	wrpr	%l1, 0, %tba			! Make sure the PROM didn't foul up.

	/*
	 * Switch to the kernel mode and run away.
	 */
	wrpr	%g0, WSTATE_KERN, %wstate

#ifdef DEBUG
	wrpr	%g0, 1, %tl			! Debug -- start at tl==3 so we'll watchdog
	wrpr	%g0, 0x1ff, %tt			! Debug -- clear out unused trap regs
	wrpr	%g0, 0, %tpc
	wrpr	%g0, 0, %tnpc
	wrpr	%g0, 0, %tstate
	wrpr	%g0, 0, %tl
#endif

#ifdef DEBUG
	set	_C_LABEL(pmapdebug), %o1
	ld	[%o1], %o1
	sethi	%hi(0x40000), %o2
	btst	%o2, %o1
	bz	0f

	LDPTR	[%l7 + CI_SPINUP], %o1
	set	1f, %o0		! Debug printf
	call	_C_LABEL(prom_printf)
	 mov	%sp, %o2

	.data
1:
	.asciz	"Calling startup routine %p with stack at %p...\n"
	_ALIGN
	.text
0:	
#endif
	/*
	 * Call our startup routine.
	 */

	LDPTR	[%l7 + CI_SPINUP], %o1

	call	%o1				! Call routine
	 clr	%o0				! our frame arg is ignored

	set	1f, %o0				! Main should never come back here
	call	_C_LABEL(panic)
	 nop
	.data
1:
	.asciz	"main() returned\n"
	_ALIGN
	.text

	.align 8
ENTRY(get_romtba)
	retl
	 rdpr	%tba, %o0

ENTRY(setcputyp)
	sethi	%hi(cputyp), %o1	! Trash %o1 assuming this is ok
	st	%o0, [%o1 + %lo(cputyp)]
	retl
	 nop
		
#ifdef MULTIPROCESSOR
	/*
	 * cpu_mp_startup is called with:
	 *
	 *	%g2 = cpu_args
	 */
ENTRY(cpu_mp_startup)
	mov	1, %o0
	sllx	%o0, 63, %o0
	wr	%o0, TICK_CMPR	! XXXXXXX clear and disable %tick_cmpr for now
	wrpr    %g0, 0, %cleanwin
	wrpr	%g0, 0, %tl			! Make sure we're not in NUCLEUS mode
	wrpr	%g0, WSTATE_KERN, %wstate
	wrpr	%g0, PSTATE_KERN, %pstate
	flushw

	/* Cache the cputyp in %l6 for later use below */
	sethi	%hi(cputyp), %l6
	ld	[%l6 + %lo(cputyp)], %l6
	
	/*
	 * Get pointer to our cpu_info struct
	 */
	ldx	[%g2 + CBA_CPUINFO], %l1	! Load the interrupt stack's PA
	
#ifdef SUN4V
	cmp	%l6, CPU_SUN4V
	bne,pt	%icc, 3f
	 nop
	
	/* sun4v */
	
	sethi	%hi(0x80000000), %l2		! V=1|NFO=0|SW=0
	sllx	%l2, 32, %l2			! Shift it into place
	mov	-1, %l3				! Create a nice mask
	sllx	%l3, 56, %l4			! Mask off high 8 bits
	or	%l4, 0xfff, %l4			! We can just load this in 12 (of 13) bits
	andn	%l1, %l4, %l1			! Mask the phys page number into RA
	or	%l2, %l1, %l1			! Now take care of the 8 high bits V|NFO|SW
	or	%l1, 0x0741, %l2		! And low 13 bits IE=0|E=0|CP=1|CV=1|P=1|
						!		  X=0|W=1|SW=00|SZ=0001

	/*
	 *  Now, map in the interrupt stack & cpu_info as context==0
	 */
	
	set	INTSTACK, %o0			! vaddr
	clr	%o1				! reserved
	mov	%l2, %o2			! tte
	mov	MAP_DTLB, %o3			! flags
	mov	FT_MMU_MAP_PERM_ADDR, %o5	! hv fast trap function
	ta	ST_FAST_TRAP
	cmp	%o0, 0
	be,pt	%icc, 5f
	 nop
	sir					! crash if mapping fails
5:

	/*
	 * Set 0 as primary context XXX
	 */
	
	mov	CTX_PRIMARY, %o0
	SET_MMU_CONTEXTID_SUN4V %g0, %o0

	ba	4f		
	 nop
3:
#endif
	
	/* sun4u */
	
	sethi	%hi(0xa0000000), %l2		! V=1|SZ=01|NFO=0|IE=0
	sllx	%l2, 32, %l2			! Shift it into place
	mov	-1, %l3				! Create a nice mask
	sllx	%l3, 43, %l4			! Mask off high bits
	or	%l4, 0xfff, %l4			! We can just load this in 12 (of 13) bits
	andn	%l1, %l4, %l1			! Mask the phys page number
	or	%l2, %l1, %l1			! Now take care of the high bits
	or	%l1, SUN4U_TTE_DATABITS, %l2	! And low bits:	L=1|CP=1|CV=?|E=0|P=1|W=1|G=0

	/*
	 *  Now, map in the interrupt stack & cpu_info as context==0
	 */
	
	set	TLB_TAG_ACCESS, %l5
	set	INTSTACK, %l0
	stxa	%l0, [%l5] ASI_DMMU		! Make DMMU point to it
	stxa	%l2, [%g0] ASI_DMMU_DATA_IN	! Store it

	/*
	 * Set 0 as primary context XXX
	 */
	
	mov	CTX_PRIMARY, %o0
	SET_MMU_CONTEXTID_SUN4U %g0, %o0

4:	
	membar	#Sync

	/*
	 * Temporarily use the interrupt stack
	 */
#ifdef _LP64
	set	((EINTSTACK - CC64FSZ - TF_SIZE)) & ~0x0f - BIAS, %sp
#else
	set	EINTSTACK - CC64FSZ - TF_SIZE, %sp
#endif
	set	1, %fp
	clr	%i7

#ifdef SUN4V
	cmp	%l6, CPU_SUN4V
	bne,pt	%icc, 2f
	 nop
	
	/* sun4v */
	
	/*
	 * install our TSB pointers
	 */

	set	CPUINFO_VA, %o0
	LDPTR	[%o0 + CI_TSB_DESC], %o0
	call	_C_LABEL(pmap_setup_tsb_sun4v)
	 nop

	/* set trap table */

	set	_C_LABEL(trapbase_sun4v), %l1
	GET_MMFSA %o1
	call	_C_LABEL(prom_set_trap_table_sun4v)
	 mov	%l1, %o0

	! Now we should be running 100% from our handlers	
	ba	3f		
	 nop
2:
#endif
	/* sun4u */
	
	/*
	 * install our TSB pointers
	 */

	sethi	%hi(CPUINFO_VA+CI_TSB_DMMU), %l0
	sethi	%hi(CPUINFO_VA+CI_TSB_IMMU), %l1
	sethi	%hi(_C_LABEL(tsbsize)), %l2
	sethi	%hi(0x1fff), %l3
	sethi	%hi(TSB), %l4
	LDPTR	[%l0 + %lo(CPUINFO_VA+CI_TSB_DMMU)], %l0
	LDPTR	[%l1 + %lo(CPUINFO_VA+CI_TSB_IMMU)], %l1
	ld	[%l2 + %lo(_C_LABEL(tsbsize))], %l2
	or	%l3, %lo(0x1fff), %l3
	or	%l4, %lo(TSB), %l4

	andn	%l0, %l3, %l0			! Mask off size and split bits
	or	%l0, %l2, %l0			! Make a TSB pointer
	stxa	%l0, [%l4] ASI_DMMU		! Install data TSB pointer
	membar	#Sync

	andn	%l1, %l3, %l1			! Mask off size and split bits
	or	%l1, %l2, %l1			! Make a TSB pointer
	stxa	%l1, [%l4] ASI_IMMU		! Install instruction TSB pointer
	membar	#Sync
	set	1f, %o0
	flush	%o0
1:

	/* set trap table */
	
	set	_C_LABEL(trapbase), %l1
	call	_C_LABEL(prom_set_trap_table_sun4u)
	 mov	%l1, %o0
3:	
	wrpr	%l1, 0, %tba			! Make sure the PROM didn't
						! foul up.
	/*
	 * Use this CPUs idlelewp's uarea stack
	 */
	sethi	%hi(CPUINFO_VA+CI_IDLELWP), %l0
	LDPTR	[%l0 + %lo(CPUINFO_VA+CI_IDLELWP)], %l0
	set	USPACE - TF_SIZE - CC64FSZ, %l1
	LDPTR	[%l0 + L_PCB], %l0
	add	%l0, %l1, %l0
#ifdef _LP64
	andn	%l0, 0x0f, %l0			! Needs to be 16-byte aligned
	sub	%l0, BIAS, %l0			! and biased
#endif
	mov	%l0, %sp
	flushw

	/*
	 * Switch to the kernel mode and run away.
	 */
	wrpr	%g0, 13, %pil
	wrpr	%g0, PSTATE_INTR|PSTATE_PEF, %pstate
	wr	%g0, FPRS_FEF, %fprs			! Turn on FPU

	call	_C_LABEL(cpu_hatch)
	 clr %g4

	b	_C_LABEL(idle_loop)
	 clr	%o0

	NOTREACHED

	.globl cpu_mp_startup_end
cpu_mp_startup_end:
#endif

/*
 * openfirmware(cell* param);
 *
 * OpenFirmware entry point
 *
 * If we're running in 32-bit mode we need to convert to a 64-bit stack
 * and 64-bit cells.  The cells we'll allocate off the stack for simplicity.
 */
	.align 8
ENTRY(openfirmware)
	sethi	%hi(romp), %o4
	andcc	%sp, 1, %g0
	bz,pt	%icc, 1f
	 LDPTR	[%o4+%lo(romp)], %o4		! v9 stack, just load the addr and callit
	save	%sp, -CC64FSZ, %sp
	rdpr	%pil, %i2
	mov	PIL_HIGH, %i3
	cmp	%i3, %i2
	movle	%icc, %i2, %i3
	wrpr	%g0, %i3, %pil
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
#if !defined(_LP64)
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
	add	%sp, -BIAS, %sp
	rdpr	%pstate, %l0
	srl	%sp, 0, %sp
	rdpr	%pil, %i2	! s = splx(level)
	mov	%i0, %o0
	mov	PIL_HIGH, %i3
	mov	%g1, %l1
	mov	%g2, %l2
	cmp	%i3, %i2
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	movle	%icc, %i2, %i3
	mov	%g6, %l6
	mov	%g7, %l7
	wrpr	%i3, %g0, %pil
	jmpl	%i4, %o7
	! Enable 64-bit addresses for the prom
#if defined(_LP64)
	 wrpr	%g0, PSTATE_PROM, %pstate
#else
	 wrpr	%g0, PSTATE_PROM|PSTATE_IE, %pstate
#endif
	wrpr	%l0, 0, %pstate
	wrpr	%i2, 0, %pil
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
 * void ofw_exit(cell_t args[])
 */
ENTRY(openfirmware_exit)
	STACKFRAME(-CC64FSZ)
	flushw					! Flush register windows

	wrpr	%g0, PIL_HIGH, %pil		! Disable interrupts
	sethi	%hi(romtba), %l5
	LDPTR	[%l5 + %lo(romtba)], %l5
	wrpr	%l5, 0, %tba			! restore the ofw trap table

	/* Arrange locked kernel stack as PROM stack */
	set	EINTSTACK  - CC64FSZ, %l5

	andn	%l5, 0x0f, %l5			! Needs to be 16-byte aligned
	sub	%l5, BIAS, %l5			! and biased
	mov	%l5, %sp
	flushw

	sethi	%hi(romp), %l6
	LDPTR	[%l6 + %lo(romp)], %l6

	mov     CTX_PRIMARY, %l3		! set context 0
	stxa    %g0, [%l3] ASI_DMMU
	membar	#Sync

	wrpr	%g0, PSTATE_PROM, %pstate	! Disable interrupts
						! and enable 64-bit addresses
	wrpr	%g0, 0, %tl			! force trap level 0
	call	%l6
	 mov	%i0, %o0
	NOTREACHED

/*
 * sp_tlb_flush_pte_us(vaddr_t va, int ctx)
 * sp_tlb_flush_pte_usiii(vaddr_t va, int ctx)
 *
 * Flush tte from both IMMU and DMMU.
 *
 * This uses %o0-%o5
 */
	.align 8
ENTRY(sp_tlb_flush_pte_us)
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
	.data
1:
	.asciz	"sp_tlb_flush_pte_us:	demap ctx=%x va=%08x res=%x\n"
	_ALIGN
	.text
2:
#endif
#ifdef MULTIPROCESSOR
	rdpr	%pstate, %o3
	andn	%o3, PSTATE_IE, %o4			! disable interrupts
	wrpr	%o4, 0, %pstate
#endif
	srlx	%o0, PG_SHIFT4U, %o0			! drop unused va bits
	mov	CTX_SECONDARY, %o2
	sllx	%o0, PG_SHIFT4U, %o0
	ldxa	[%o2] ASI_DMMU, %o5			! Save secondary context
	sethi	%hi(KERNBASE), %o4
	membar	#LoadStore
	stxa	%o1, [%o2] ASI_DMMU			! Insert context to demap
	membar	#Sync
	or	%o0, DEMAP_PAGE_SECONDARY, %o0		! Demap page from secondary context only
	stxa	%o0, [%o0] ASI_DMMU_DEMAP		! Do the demap
	stxa	%o0, [%o0] ASI_IMMU_DEMAP		! to both TLBs
#ifdef TLB_FLUSH_LOWVA
	srl	%o0, 0, %o0				! and make sure it's both 32- and 64-bit entries
	stxa	%o0, [%o0] ASI_DMMU_DEMAP		! Do the demap
	stxa	%o0, [%o0] ASI_IMMU_DEMAP		! Do the demap
#endif
	flush	%o4
	stxa	%o5, [%o2] ASI_DMMU			! Restore secondary context
	membar	#Sync
	retl
#ifdef MULTIPROCESSOR
	 wrpr	%o3, %pstate				! restore interrupts
#else
	 nop
#endif

ENTRY(sp_tlb_flush_pte_usiii)
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
	.data
1:
	.asciz	"sp_tlb_flush_pte_usiii:	demap ctx=%x va=%08x res=%x\n"
	_ALIGN
	.text
2:
#endif
	! %o0 = VA [in]
	! %o1 = ctx value [in] / KERNBASE
	! %o2 = CTX_PRIMARY
	! %o3 = saved %tl
	! %o4 = saved %pstate
	! %o5 = saved primary ctx 

	! Need this for UP as well
	rdpr	%pstate, %o4
	andn	%o4, PSTATE_IE, %o3			! disable interrupts
	wrpr	%o3, 0, %pstate

	!!
	!! Cheetahs do not support flushing the IMMU from secondary context
	!!
	rdpr	%tl, %o3
	mov	CTX_PRIMARY, %o2
	brnz,pt	%o3, 1f
	 andn	%o0, 0xfff, %o0				! drop unused va bits
	wrpr	%g0, 1, %tl				! Make sure we're NUCLEUS
1:	
	ldxa	[%o2] ASI_DMMU, %o5			! Save primary context
	membar	#LoadStore
	stxa	%o1, [%o2] ASI_DMMU			! Insert context to demap
	sethi	%hi(KERNBASE), %o1
	membar	#Sync
	or	%o0, DEMAP_PAGE_PRIMARY, %o0
	stxa	%o0, [%o0] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync
	stxa	%o0, [%o0] ASI_IMMU_DEMAP		! to both TLBs
	membar	#Sync
#ifdef TLB_FLUSH_LOWVA
	srl	%o0, 0, %o0				! and make sure it's both 32- and 64-bit entries
	stxa	%o0, [%o0] ASI_DMMU_DEMAP		! Do the demap
	membar	#Sync
	stxa	%o0, [%o0] ASI_IMMU_DEMAP		! Do the demap
	membar	#Sync
#endif
	flush	%o1
	stxa	%o5, [%o2] ASI_DMMU			! Restore primary context
	membar	#Sync
	brnz,pt	%o3, 1f
	 flush	%o1
	wrpr	%g0, %o3, %tl				! Return to kernel mode.
1:	
	retl
	 wrpr	%o4, %pstate				! restore interrupts


/*
 * sp_tlb_flush_all_us(void)
 * sp_tlb_flush_all_usiii(void)
 *
 * Flush all user TLB entries from both IMMU and DMMU.
 * We have both UltraSPARC I+II, and UltraSPARC >=III versions.
 */
	.align 8
ENTRY(sp_tlb_flush_all_us)
	rdpr	%pstate, %o3
	andn	%o3, PSTATE_IE, %o4			! disable interrupts
	wrpr	%o4, 0, %pstate
	set	((TLB_SIZE_SPITFIRE-1) * 8), %o0
	set	CTX_SECONDARY, %o4
	ldxa	[%o4] ASI_DMMU, %o4			! save secondary context
	set	CTX_MASK, %o5
	membar	#Sync

	! %o0 = loop counter
	! %o1 = ctx value
	! %o2 = TLB tag value
	! %o3 = saved %pstate
	! %o4 = saved primary ctx
	! %o5 = CTX_MASK
	! %xx = saved %tl

0:
	ldxa	[%o0] ASI_DMMU_TLB_TAG, %o2		! fetch the TLB tag
	andcc	%o2, %o5, %o1				! context 0?
	bz,pt	%xcc, 1f				! if so, skip
	 mov	CTX_SECONDARY, %o2

	stxa	%o1, [%o2] ASI_DMMU			! set the context
	set	DEMAP_CTX_SECONDARY, %o2
	membar	#Sync
	stxa	%o2, [%o2] ASI_DMMU_DEMAP		! do the demap
	membar	#Sync

1:
	dec	8, %o0
	brgz,pt %o0, 0b					! loop over all entries
	 nop

/*
 * now do the IMMU
 */

	set	((TLB_SIZE_SPITFIRE-1) * 8), %o0

0:
	ldxa	[%o0] ASI_IMMU_TLB_TAG, %o2		! fetch the TLB tag
	andcc	%o2, %o5, %o1				! context 0?
	bz,pt	%xcc, 1f				! if so, skip
	 mov	CTX_SECONDARY, %o2

	stxa	%o1, [%o2] ASI_DMMU			! set the context
	set	DEMAP_CTX_SECONDARY, %o2
	membar	#Sync
	stxa	%o2, [%o2] ASI_IMMU_DEMAP		! do the demap
	membar	#Sync

1:
	dec	8, %o0
	brgz,pt %o0, 0b					! loop over all entries
	 nop

	set	CTX_SECONDARY, %o2
	stxa	%o4, [%o2] ASI_DMMU			! restore secondary ctx
	sethi	%hi(KERNBASE), %o4
	membar	#Sync
	flush	%o4
	retl
	 wrpr	%o3, %pstate

	.align 8
ENTRY(sp_tlb_flush_all_usiii)
	rdpr	%tl, %o5
	brnz,pt	%o5, 1f
	 set	DEMAP_ALL, %o2
	wrpr	1, %tl
1:
	rdpr	%pstate, %o3
	andn	%o3, PSTATE_IE, %o4			! disable interrupts
	wrpr	%o4, 0, %pstate

	stxa	%o2, [%o2] ASI_IMMU_DEMAP
	membar	#Sync
	stxa	%o2, [%o2] ASI_DMMU_DEMAP

	sethi	%hi(KERNBASE), %o4
	membar	#Sync
	flush	%o4

	wrpr	%o5, %tl
	retl
	 wrpr	%o3, %pstate

/*
 * sp_blast_dcache(int dcache_size, int dcache_line_size)
 *
 * Clear out all of D$ regardless of contents
 */
	.align 8
ENTRY(sp_blast_dcache)
/*
 * We turn off interrupts for the duration to prevent RED exceptions.
 */
#ifdef PROF
	save	%sp, -CC64FSZ, %sp
#endif

	rdpr	%pstate, %o3
	sub	%o0, %o1, %o0
	andn	%o3, PSTATE_IE, %o4			! Turn off PSTATE_IE bit
	wrpr	%o4, 0, %pstate
1:
	stxa	%g0, [%o0] ASI_DCACHE_TAG
	membar	#Sync
	brnz,pt	%o0, 1b
	 sub	%o0, %o1, %o0

	sethi	%hi(KERNBASE), %o2
	flush	%o2
	membar	#Sync
#ifdef PROF
	wrpr	%o3, %pstate
	ret
	 restore
#else
	retl
	 wrpr	%o3, %pstate
#endif

#ifdef MULTIPROCESSOR
/*
 * void sparc64_ipi_blast_dcache(int dcache_size, int dcache_line_size)
 *
 * Clear out all of D$ regardless of contents
 *
 * On entry:
 *	%g2 = dcache_size
 *	%g3 = dcache_line_size
 */
	.align 8
ENTRY(sparc64_ipi_blast_dcache)
	sub	%g2, %g3, %g2
1:
	stxa	%g0, [%g2] ASI_DCACHE_TAG
	membar	#Sync
	brnz,pt	%g2, 1b
	 sub	%g2, %g3, %g2

	sethi	%hi(KERNBASE), %g5
	flush	%g5
	membar	#Sync

	ba,a	ret_from_intr_vector
	 nop
#endif /* MULTIPROCESSOR */

/*
 * blast_icache_us()
 * blast_icache_usiii()
 *
 * Clear out all of I$ regardless of contents
 * Does not modify %o0
 *
 * We turn off interrupts for the duration to prevent RED exceptions.
 * For the Cheetah version, we also have to to turn off the I$ during this as
 * ASI_ICACHE_TAG accesses interfere with coherency.
 */
	.align 8
ENTRY(blast_icache_us)
	rdpr	%pstate, %o3
	sethi	%hi(icache_size), %o1
	ld	[%o1 + %lo(icache_size)], %o1
	sethi	%hi(icache_line_size), %o2
	ld	[%o2 + %lo(icache_line_size)], %o2
	sub	%o1, %o2, %o1
	andn	%o3, PSTATE_IE, %o4			! Turn off PSTATE_IE bit
	wrpr	%o4, 0, %pstate
1:
	stxa	%g0, [%o1] ASI_ICACHE_TAG
	brnz,pt	%o1, 1b
	 sub	%o1, %o2, %o1
	sethi	%hi(KERNBASE), %o5
	flush	%o5
	membar	#Sync
	retl
	 wrpr	%o3, %pstate

	.align 8
ENTRY(blast_icache_usiii)
	rdpr	%pstate, %o3
	sethi	%hi(icache_size), %o1
	ld	[%o1 + %lo(icache_size)], %o1
	sethi	%hi(icache_line_size), %o2
	ld	[%o2 + %lo(icache_line_size)], %o2
	sub	%o1, %o2, %o1
	andn	%o3, PSTATE_IE, %o4			! Turn off PSTATE_IE bit
	wrpr	%o4, 0, %pstate
	ldxa    [%g0] ASI_MCCR, %o5
	andn	%o5, MCCR_ICACHE_EN, %o4		! Turn off the I$
	stxa	%o4, [%g0] ASI_MCCR
	flush 	%g0
1:
	stxa	%g0, [%o1] ASI_ICACHE_TAG
	membar	#Sync
	brnz,pt	%o1, 1b
	 sub	%o1, %o2, %o1
	stxa	%o5, [%g0] ASI_MCCR			! Restore the I$
	flush 	%g0
	retl
	 wrpr	%o3, %pstate

/*
 * dcache_flush_page_us(paddr_t pa)
 * dcache_flush_page_usiii(paddr_t pa)
 *
 * Clear one page from D$.
 *
 */
	.align 8
ENTRY(dcache_flush_page_us)
#ifndef _LP64
	COMBINE(%o0, %o1, %o0)
#endif
	mov	-1, %o1		! Generate mask for tag: bits [29..2]
	srlx	%o0, 13-2, %o2	! Tag is PA bits <40:13> in bits <29:2>
	clr	%o4
	srl	%o1, 2, %o1	! Now we have bits <29:0> set
	set	(2*NBPG), %o5
	ba,pt	%icc, 1f
	 andn	%o1, 3, %o1	! Now we have bits <29:2> set

	.align 8
1:
	ldxa	[%o4] ASI_DCACHE_TAG, %o3
	mov	%o4, %o0
	deccc	32, %o5
	bl,pn	%icc, 2f
	 inc	32, %o4

	xor	%o3, %o2, %o3
	andcc	%o3, %o1, %g0
	bne,pt	%xcc, 1b
	 membar	#LoadStore

	stxa	%g0, [%o0] ASI_DCACHE_TAG
	ba,pt	%icc, 1b
	 membar	#StoreLoad
2:

	sethi	%hi(KERNBASE), %o5
	flush	%o5
	retl
	 membar	#Sync

	.align 8
ENTRY(dcache_flush_page_usiii)
#ifndef _LP64
	COMBINE(%o0, %o1, %o0)
#endif
	set	NBPG, %o1
	sethi	%hi(dcache_line_size), %o2
	add	%o0, %o1, %o1	! end address
	ld	[%o2 + %lo(dcache_line_size)], %o2

1:
	stxa	%g0, [%o0] ASI_DCACHE_INVALIDATE
	add	%o0, %o2, %o0
	cmp	%o0, %o1
	bl,pt	%xcc, 1b
	 nop

	sethi	%hi(KERNBASE), %o5
	flush	%o5
	retl
	 membar	#Sync

/*
 *	cache_flush_phys_us(paddr_t, psize_t, int);
 *	cache_flush_phys_usiii(paddr_t, psize_t, int);
 *
 *	Clear a set of paddrs from the D$, I$ and if param3 is
 *	non-zero, E$.  (E$ is not supported yet).
 */

	.align 8
ENTRY(cache_flush_phys_us)
#ifndef _LP64
	COMBINE(%o0, %o1, %o0)
	COMBINE(%o2, %o3, %o1)
	mov	%o4, %o2
#endif
#ifdef DEBUG
	tst	%o2		! Want to clear E$?
	tnz	1		! Error!
#endif
	add	%o0, %o1, %o1	! End PA
	dec	%o1

	!!
	!! Both D$ and I$ tags match pa bits 42-13, but
	!! they are shifted different amounts.  So we'll
	!! generate a mask for bits 40-13.
	!!

	mov	-1, %o2		! Generate mask for tag: bits [40..13]
	srl	%o2, 5, %o2	! 32-5 = [27..0]
	sllx	%o2, 13, %o2	! 27+13 = [40..13]

	and	%o2, %o0, %o0	! Mask away uninteresting bits
	and	%o2, %o1, %o1	! (probably not necessary)

	set	(2*NBPG), %o5
	clr	%o4
1:
	ldxa	[%o4] ASI_DCACHE_TAG, %o3
	sllx	%o3, 40-29, %o3	! Shift D$ tag into place
	and	%o3, %o2, %o3	! Mask out trash

	cmp	%o0, %o3
	blt,pt	%xcc, 2f	! Too low
	 cmp	%o1, %o3
	bgt,pt	%xcc, 2f	! Too high
	 nop

	membar	#LoadStore
	stxa	%g0, [%o4] ASI_DCACHE_TAG ! Just right
	membar	#Sync
2:
	ldda	[%o4] ASI_ICACHE_TAG, %g0	! Tag goes in %g1
	sllx	%g1, 40-35, %g1			! Shift I$ tag into place
	and	%g1, %o2, %g1			! Mask out trash
	cmp	%o0, %g1
	blt,pt	%xcc, 3f
	 cmp	%o1, %g1
	bgt,pt	%xcc, 3f
	 nop
	stxa	%g0, [%o4] ASI_ICACHE_TAG
3:
	membar	#StoreLoad
	dec	32, %o5
	brgz,pt	%o5, 1b
	 inc	32, %o4

	sethi	%hi(KERNBASE), %o5
	flush	%o5
	retl
	 membar	#Sync

	.align 8
ENTRY(cache_flush_phys_usiii)
#ifndef _LP64
	COMBINE(%o0, %o1, %o0)
	COMBINE(%o2, %o3, %o1)
	mov	%o4, %o2
#endif
#ifdef DEBUG
	tst	%o2		! Want to clear E$?
	tnz	1		! Error!
#endif
	add	%o0, %o1, %o1	! End PA
	sethi	%hi(dcache_line_size), %o3
	ld	[%o3 + %lo(dcache_line_size)], %o3
	sethi	%hi(KERNBASE), %o5
1:
	stxa	%g0, [%o0] ASI_DCACHE_INVALIDATE
	add	%o0, %o3, %o0
	cmp	%o0, %o1
	bl,pt	%xcc, 1b
	 nop

	/* don't need to flush the I$ on cheetah */

	flush	%o5
	retl
	 membar	#Sync

#ifdef COMPAT_16
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
ENTRY_NOPROFILE(sigcode)
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
	add	%sp, BIAS+CC64FSZ+BLOCK_SIZE, %l0	! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block store
	stda	%f0, [%l0] ASI_BLK_P
	inc	BLOCK_SIZE, %l0
	stda	%f16, [%l0] ASI_BLK_P
1:
	bz,pt	%icc, 2f
	 add	%sp, BIAS+CC64FSZ+BLOCK_SIZE, %l0	! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block store
	add	%l0, 2*BLOCK_SIZE, %l0	! and skip what we already stored
	stda	%f32, [%l0] ASI_BLK_P
	inc	BLOCK_SIZE, %l0
	stda	%f48, [%l0] ASI_BLK_P
2:
	membar	#Sync
	rd	%fprs, %l0		! reload fprs copy, for checking after
	rd	%y, %l1			! in any case, save %y
	lduw	[%fp + BIAS + 128], %o0	! sig
	lduw	[%fp + BIAS + 128 + 4], %o1	! code
	call	%g1			! (*sa->sa_handler)(sig,code,scp)
	 add	%fp, BIAS + 128 + 8, %o2	! scp
	wr	%l1, %g0, %y		! in any case, restore %y

	/*
	 * Now that the handler has returned, re-establish all the state
	 * we just saved above, then do a sigreturn.
	 */
	btst	FPRS_DL|FPRS_DU, %l0	! All clean?
	bz,pt	%icc, 2f
	 btst	FPRS_DL, %l0		! test dl
	bz,pt	%icc, 1f
	 btst	FPRS_DU, %l0		! test du

	ldx	[%sp + CC64FSZ + BIAS + 0], %fsr
	add	%sp, BIAS+CC64FSZ+BLOCK_SIZE, %l0	! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block load
	ldda	[%l0] ASI_BLK_P, %f0
	inc	BLOCK_SIZE, %l0
	ldda	[%l0] ASI_BLK_P, %f16
1:
	bz,pt	%icc, 2f
	 nop
	add	%sp, BIAS+CC64FSZ+BLOCK_SIZE, %l0	! Generate a pointer so we can
	andn	%l0, BLOCK_ALIGN, %l0	! do a block load
	inc	2*BLOCK_SIZE, %l0	! and skip what we already loaded
	ldda	[%l0] ASI_BLK_P, %f32
	inc	BLOCK_SIZE, %l0
	ldda	[%l0] ASI_BLK_P, %f48
2:
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7
	membar	#Sync

	restore	%g0, SYS_compat_16___sigreturn14, %g1 ! get registers back & set syscall #
	add	%sp, BIAS + 128 + 8, %o0! compute scp
!	andn	%o0, 0x0f, %o0
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
	/* NOTREACHED */

	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):
#endif

#if !defined(_LP64)

#define SIGCODE_NAME		sigcode
#define ESIGCODE_NAME		esigcode
#define SIGRETURN_NAME		SYS_compat_16___sigreturn14
#define EXIT_NAME		SYS_exit

#include "sigcode32.s"

#endif
#endif

/*
 * getfp() - get stack frame pointer
 */
ENTRY(getfp)
	retl
	 mov %fp, %o0

/*
 * nothing MD to do in the idle loop
 */
ENTRY(cpu_idle)
	retl
	 nop

/*
 * cpu_switchto() switches to an lwp to run and runs it, saving the
 * current one away.
 *
 * stuct lwp * cpu_switchto(struct lwp *current, struct lwp *next)
 * Switch to the specified next LWP
 * Arguments:
 *	i0	'struct lwp *' of the current LWP
 *	i1	'struct lwp *' of the LWP to switch to
 *	i2	'bool' of the flag returning to a softint LWP or not
 * Returns:
 *	the old lwp switched away from
 */
ENTRY(cpu_switchto)
	save	%sp, -CC64FSZ, %sp
	/*
	 * REGISTER USAGE AT THIS POINT:
	 *	%l1 = newpcb
	 *	%l3 = new trapframe
	 *	%l4 = new l->l_proc
	 *	%l5 = pcb of oldlwp
	 *	%l6 = %hi(CPCB)
	 *	%l7 = %hi(CURLWP)
	 *	%i0 = oldlwp
	 *	%i1 = lwp
	 *	%i2 = returning
	 *	%o0 = tmp 1
	 *	%o1 = tmp 2
	 *	%o2 = tmp 3
	 *	%o3 = tmp 4
	 */

	flushw				! save all register windows except this one
	wrpr	%g0, PSTATE_KERN, %pstate	! make sure we're on normal globals
						! with traps turned off

	brz,pn	%i0, 1f
	 sethi	%hi(CPCB), %l6

	rdpr	%pstate, %o1			! oldpstate = %pstate;
	LDPTR	[%i0 + L_PCB], %l5

	stx	%i7, [%l5 + PCB_PC]
	stx	%i6, [%l5 + PCB_SP]
	sth	%o1, [%l5 + PCB_PSTATE]

	rdpr	%cwp, %o2		! Useless
	stb	%o2, [%l5 + PCB_CWP]

1:
	sethi	%hi(CURLWP), %l7

	LDPTR   [%i1 + L_PCB], %l1	! newpcb = l->l_pcb;

	/*
	 * Load the new lwp.  To load, we must change stacks and
	 * alter cpcb and the window control registers, hence we must
	 * keep interrupts disabled.
	 */

	STPTR	%i1, [%l7 + %lo(CURLWP)]	! curlwp = l;
	STPTR	%l1, [%l6 + %lo(CPCB)]		! cpcb = newpcb;

	ldx	[%l1 + PCB_SP], %i6
	ldx	[%l1 + PCB_PC], %i7

	wrpr	%g0, 0, %otherwin	! These two insns should be redundant
	wrpr	%g0, 0, %canrestore
	GET_MAXCWP %o3
	wrpr	%g0, %o3, %cleanwin
	dec	1, %o3			! CANSAVE + CANRESTORE + OTHERWIN = MAXCWP - 1
	/* Skip the rest if returning to a interrupted LWP. */
	brnz,pn	%i2, Lsw_noras
	 wrpr	%o3, %cansave

	/* finally, enable traps */
	wrpr	%g0, PSTATE_INTR, %pstate

	!flushw
	!membar #Sync

	/*
	 * Check for restartable atomic sequences (RAS)
	 */
	LDPTR	[%i1 + L_PROC], %l4		! now %l4 points to p
	mov	%l4, %o0		! p is first arg to ras_lookup
	LDPTR	[%o0 + P_RASLIST], %o1	! any RAS in p?
	brz,pt	%o1, Lsw_noras		! no, skip RAS check
	 LDPTR	[%i1 + L_TF], %l3	! pointer to trap frame
	call	_C_LABEL(ras_lookup)
	 LDPTR	[%l3 + TF_PC], %o1
	cmp	%o0, -1
	be,pt	%xcc, Lsw_noras
	 add	%o0, 4, %o1
	STPTR	%o0, [%l3 + TF_PC]	! store rewound %pc
	STPTR	%o1, [%l3 + TF_NPC]	! and %npc

Lsw_noras:

	/*
	 * We are resuming the process that was running at the
	 * call to switch().  Just set psr ipl and return.
	 */
!	wrpr	%g0, 0, %cleanwin	! DEBUG
	clr	%g4		! This needs to point to the base of the data segment
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore default ASI
	!wrpr	%g0, PSTATE_INTR, %pstate
	ret
	 restore %i0, %g0, %o0				! return old curlwp

#ifdef __HAVE_FAST_SOFTINTS
/*
 * Switch to the LWP assigned to handle interrupts from the given
 * source.  We borrow the VM context from the interrupted LWP.
 *
 * int softint_fastintr(void *l)
 *
 * Arguments:
 *	i0	softint lwp
 */
ENTRY(softint_fastintr)
	save	%sp, -CC64FSZ, %sp
	set	CPUINFO_VA, %l0			! l0 = curcpu()
	rdpr	%pil, %l7			! l7 = splhigh()
	wrpr	%g0, PIL_HIGH, %pil
	LDPTR	[%l0 + CI_EINTSTACK], %l6	! l6 = ci_eintstack
	add	%sp, -CC64FSZ, %l2		! ci_eintstack = sp - CC64FSZ
	STPTR	%l2, [%l0 + CI_EINTSTACK]	! save intstack for nested intr

	mov	%i0, %o0			! o0/i0 = softint lwp
	mov	%l7, %o1			! o1/i1 = ipl
	save	%sp, -CC64FSZ, %sp		! make one more register window
	flushw					! and save all

	sethi	%hi(CURLWP), %l7
	sethi	%hi(CPCB), %l6
	LDPTR	[%l7 + %lo(CURLWP)], %l0	! l0 = interrupted lwp (curlwp)

	/* save interrupted lwp/pcb info */
	sethi	%hi(softint_fastintr_ret - 8), %o0	! trampoline function
	LDPTR	[%l0 + L_PCB], %l5		! l5 = interrupted pcb
	or	%o0, %lo(softint_fastintr_ret - 8), %o0
	stx	%i6, [%l5 + PCB_SP]
	stx	%o0, [%l5 + PCB_PC]
	rdpr	%pstate, %o1
	rdpr	%cwp, %o2
	sth	%o1, [%l5 + PCB_PSTATE]
	stb	%o2, [%l5 + PCB_CWP]

	/* switch to softint lwp */
	sethi	%hi(USPACE - TF_SIZE - CC64FSZ - STKB), %o3
	LDPTR	[%i0 + L_PCB], %l1		! l1 = softint pcb
	or	%o3, %lo(USPACE - TF_SIZE - CC64FSZ - STKB), %o3
	STPTR	%i0, [%l7 + %lo(CURLWP)]
	add	%l1, %o3, %i6
	STPTR	%l1, [%l6 + %lo(CPCB)]
	stx	%i6, [%l1 + PCB_SP]
	add	%i6, -CC64FSZ, %sp		! new stack

	/* now switched, then invoke MI dispatcher */
	mov	%i1, %o1
	call	_C_LABEL(softint_dispatch)
	 mov	%l0, %o0

	/* switch back to interrupted lwp */
	ldx	[%l5 + PCB_SP], %i6
	STPTR	%l0, [%l7 + %lo(CURLWP)]
	STPTR	%l5, [%l6 + %lo(CPCB)]

	restore					! rewind register window

	STPTR	%l6, [%l0 + CI_EINTSTACK]	! restore ci_eintstack
	wrpr	%g0, %l7, %pil			! restore ipl
	ret
	 restore	%g0, 1, %o0

/*
 * Trampoline function that gets returned to by cpu_switchto() when
 * an interrupt handler blocks.
 *
 * Arguments:
 *	o0	old lwp from cpu_switchto()
 *
 * from softint_fastintr():
 *	l0	CPUINFO_VA
 *	l6	saved ci_eintstack
 *	l7	saved ipl
 */
softint_fastintr_ret:
	/* re-adjust after mi_switch() */
	ld	[%l0 + CI_MTX_COUNT], %o1
	inc	%o1				! ci_mtx_count++
	st	%o1, [%l0 + CI_MTX_COUNT]
	st	%g0, [%o0 + L_CTXSWTCH]		! prev->l_ctxswtch = 0

	STPTR	%l6, [%l0 + CI_EINTSTACK]	! restore ci_eintstack
	wrpr	%g0, %l7, %pil			! restore ipl
	ret
	 restore	%g0, 1, %o0

#endif /* __HAVE_FAST_SOFTINTS */

/*
 * Snapshot the current process so that stack frames are up to date.
 * Only used just before a crash dump.
 */
ENTRY(snapshot)
	rdpr	%pstate, %o1		! save psr
	stx	%o7, [%o0 + PCB_PC]	! save pc
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
 * cpu_lwp_fork() arranges for lwp_trampoline() to run when the
 * nascent lwp is selected by switch().
 *
 * The switch frame will contain pointer to struct lwp of this lwp in
 * %l2, a pointer to the function to call in %l0, and an argument to
 * pass to it in %l1 (we abuse the callee-saved registers).
 *
 * We enter lwp_trampoline as if we are "returning" from
 * cpu_switchto(), so %o0 contains previous lwp (the one we are
 * switching from) that we pass to lwp_startup().
 *
 * If the function *(%l0) returns, we arrange for an immediate return
 * to user mode.  This happens in two known cases: after execve(2) of
 * init, and when returning a child to user mode after a fork(2).
 *
 * If were setting up a kernel thread, the function *(%l0) will not
 * return.
 */
ENTRY(lwp_trampoline)
	/*
	 * Note: cpu_lwp_fork() has set up a stack frame for us to run
	 * in, so we can call other functions from here without using
	 * `save ... restore'.
	 */

	! newlwp in %l2, oldlwp in %o0
	call    lwp_startup
	 mov    %l2, %o1

	call	%l0			! re-use current frame
	 mov	%l1, %o0

	/*
	 * Here we finish up as in syscall, but simplified.
	 */
	b	return_from_trap
	 nop

/*
 * pmap_zero_page_phys(pa)
 *
 * Zero one page physically addressed
 *
 * Block load/store ASIs do not exist for physical addresses,
 * so we won't use them.
 *
 * We will execute a flush at the end to sync the I$.
 *
 * This version expects to have the dcache_flush_page_all(pa)
 * to have been called before calling into here.
 */
ENTRY(pmap_zero_page_phys)
#ifndef _LP64
	COMBINE(%o0, %o1, %o0)
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
	set	NBPG, %o2		! Loop count
	wr	%g0, ASI_PHYS_CACHED, %asi
1:
	/* Unroll the loop 8 times */
	stxa	%g0, [%o0 + 0x00] %asi
	deccc	0x40, %o2
	stxa	%g0, [%o0 + 0x08] %asi
	stxa	%g0, [%o0 + 0x10] %asi
	stxa	%g0, [%o0 + 0x18] %asi
	stxa	%g0, [%o0 + 0x20] %asi
	stxa	%g0, [%o0 + 0x28] %asi
	stxa	%g0, [%o0 + 0x30] %asi
	stxa	%g0, [%o0 + 0x38] %asi
	bg,pt	%icc, 1b
	 inc	0x40, %o0

	sethi	%hi(KERNBASE), %o3
	flush	%o3
	retl
	 wr	%g0, ASI_PRIMARY_NOFAULT, %asi	! Make C code happy

/*
 * pmap_copy_page_phys(paddr_t src, paddr_t dst)
 *
 * Copy one page physically addressed
 * We need to use a global reg for ldxa/stxa
 * so the top 32-bits cannot be lost if we take
 * a trap and need to save our stack frame to a
 * 32-bit stack.  We will unroll the loop by 4 to
 * improve performance.
 *
 * This version expects to have the dcache_flush_page_all(pa)
 * to have been called before calling into here.
 *
 */
ENTRY(pmap_copy_page_phys)
#ifndef _LP64
	COMBINE(%o0, %o1, %o0)
	COMBINE(%o2, %o3, %o1)
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
#if 1
	set	NBPG, %o2
	wr	%g0, ASI_PHYS_CACHED, %asi
1:
	ldxa	[%o0 + 0x00] %asi, %g1
	ldxa	[%o0 + 0x08] %asi, %o3
	ldxa	[%o0 + 0x10] %asi, %o4
	ldxa	[%o0 + 0x18] %asi, %o5
	inc	0x20, %o0
	deccc	0x20, %o2
	stxa	%g1, [%o1 + 0x00] %asi
	stxa	%o3, [%o1 + 0x08] %asi
	stxa	%o4, [%o1 + 0x10] %asi
	stxa	%o5, [%o1 + 0x18] %asi
	bg,pt	%icc, 1b		! We don't care about pages >4GB
	 inc	0x20, %o1
	retl
	 wr	%g0, ASI_PRIMARY_NOFAULT, %asi
#else
	set	NBPG, %o3
	add	%o3, %o0, %o3
	mov	%g1, %o4		! Save g1
1:
	ldxa	[%o0] ASI_PHYS_CACHED, %g1
	inc	8, %o0
	cmp	%o0, %o3
	stxa	%g1, [%o1] ASI_PHYS_CACHED
	bl,pt	%icc, 1b		! We don't care about pages >4GB
	 inc	8, %o1
	retl
	 mov	%o4, %g1		! Restore g1
#endif

/*
 * extern int64_t pseg_get_real(struct pmap *pm, vaddr_t addr);
 *
 * Return TTE at addr in pmap.  Uses physical addressing only.
 * pmap->pm_physaddr must by the physical address of pm_segs
 *
 */
ENTRY(pseg_get_real)
!	flushw			! Make sure we don't have stack probs & lose hibits of %o
#ifndef _LP64
	clruw	%o1					! Zero extend
#endif
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
#ifndef _LP64
	clr	%o1
#endif
	retl
	 clr	%o0

/*
 * In 32-bit mode:
 *
 * extern int pseg_set_real(struct pmap* %o0, vaddr_t addr %o1,
 *			    int64_t tte %o2:%o3, paddr_t spare %o4:%o5);
 *
 * In 64-bit mode:
 *
 * extern int pseg_set_real(struct pmap* %o0, vaddr_t addr %o1,
 *			    int64_t tte %o2, paddr_t spare %o3);
 *
 * Set a pseg entry to a particular TTE value.  Return values are:
 *
 *	-2	addr in hole
 *	0	success	(spare was not used if given)
 *	1	failure	(spare was not given, but one is needed)
 *	2	success	(spare was given, used for L2)
 *	3	failure	(spare was given, used for L2, another is needed for L3)
 *	4	success	(spare was given, used for L3)
 *
 *	rv == 0	success, spare not used if one was given
 *	rv & 4	spare was used for L3
 *	rv & 2	spare was used for L2
 *	rv & 1	failure, spare is needed
 *
 * (NB: nobody in pmap checks for the virtual hole, so the system will hang.)
 * The way to call this is:  first just call it without a spare page.
 * If that fails, allocate a page and try again, passing the paddr of the
 * new page as the spare.
 * If spare is non-zero it is assumed to be the address of a zeroed physical
 * page that can be used to generate a directory table or page table if needed.
 *
 * We keep track of valid (A_TLB_V bit set) and wired (A_TLB_TSB_LOCK bit set)
 * mappings that are set here. We check both bits on the new data entered
 * and increment counts, as well as decrementing counts if the bits are set
 * in the value replaced by this call.
 * The counters are 32 bit or 64 bit wide, depending on the kernel type we are
 * running!
 */
ENTRY(pseg_set_real)
#ifndef _LP64
	clruw	%o1					! Zero extend
	COMBINE(%o2, %o3, %o2)
	COMBINE(%o4, %o5, %o3)
#endif
	!!
	!! However we managed to get here we now have:
	!!
	!! %o0 = *pmap
	!! %o1 = addr
	!! %o2 = tte
	!! %o3 = paddr of spare page
	!!
	srax	%o1, HOLESHIFT, %o4			! Check for valid address
	brz,pt	%o4, 0f					! Should be zero or -1
	 inc	%o4					! Make -1 -> 0
	brz,pt	%o4, 0f
	 nop
#ifdef DEBUG
	ta	1					! Break into debugger
#endif
	retl
	 mov -2, %o0					! Error -- in hole!

0:
	ldx	[%o0 + PM_PHYS], %o4			! pmap->pm_segs
	clr	%g1
	srlx	%o1, STSHIFT, %o5
	and	%o5, STMASK, %o5
	sll	%o5, 3, %o5
	add	%o4, %o5, %o4
0:
	DLFLUSH(%o4,%g5)
	ldxa	[%o4] ASI_PHYS_CACHED, %o5		! Load page directory pointer
	DLFLUSH2(%g5)

	brnz,a,pt %o5, 0f				! Null pointer?
	 mov	%o5, %o4
	brz,pn	%o3, 9f					! Have a spare?
	 mov	%o3, %o5
	casxa	[%o4] ASI_PHYS_CACHED, %g0, %o5
	brnz,pn	%o5, 0b					! Something changed?
	DLFLUSH(%o4, %o5)
	mov	%o3, %o4
	mov	2, %g1					! record spare used for L2
	clr	%o3					! and not available for L3
0:
	srlx	%o1, PDSHIFT, %o5
	and	%o5, PDMASK, %o5
	sll	%o5, 3, %o5
	add	%o4, %o5, %o4
0:
	DLFLUSH(%o4,%g5)
	ldxa	[%o4] ASI_PHYS_CACHED, %o5		! Load table directory pointer
	DLFLUSH2(%g5)

	brnz,a,pt %o5, 0f				! Null pointer?
	 mov	%o5, %o4
	brz,pn	%o3, 9f					! Have a spare?
	 mov	%o3, %o5
	casxa	[%o4] ASI_PHYS_CACHED, %g0, %o5
	brnz,pn	%o5, 0b					! Something changed?
	DLFLUSH(%o4, %o4)
	mov	%o3, %o4
	mov	4, %g1					! record spare used for L3
0:
	srlx	%o1, PTSHIFT, %o5			! Convert to ptab offset
	and	%o5, PTMASK, %o5
	sll	%o5, 3, %o5
	add	%o5, %o4, %o4

	DLFLUSH(%o4,%g5)
	ldxa	[%o4] ASI_PHYS_CACHED, %o5		! save old value in %o5
	stxa	%o2, [%o4] ASI_PHYS_CACHED		! Easier than shift+or
	DLFLUSH2(%g5)

	!! at this point we have:
	!!  %g1 = return value
	!!  %o0 = struct pmap * (where the counts are)
	!!  %o2 = new TTE
	!!  %o5 = old TTE

	!! see if stats needs an update
#ifdef SUN4V
	sethi	%hi(cputyp), %g5
	ld	[%g5 + %lo(cputyp)], %g5
	cmp	%g5, CPU_SUN4V
	bne,pt	%icc, 0f
	 nop
	sethi	%hh(A_SUN4V_TLB_TSB_LOCK), %g5
	sllx	%g5, 32, %g5
	ba	1f
	 nop
0:		
#endif		
	set	A_SUN4U_TLB_TSB_LOCK, %g5
1:		
	xor	%o2, %o5, %o3			! %o3 - what changed

	brgez,pn %o3, 5f			! has resident changed? (we predict it has)
	 btst	%g5, %o3			! has wired changed?

	LDPTR	[%o0 + PM_RESIDENT], %o1	! gonna update resident count
	brlz	%o2, 0f
	 mov	1, %o4
	neg	%o4				! new is not resident -> decrement
0:	add	%o1, %o4, %o1
	STPTR	%o1, [%o0 + PM_RESIDENT]
	btst	%g5, %o3			! has wired changed?
5:	bz,pt	%xcc, 8f			! we predict it's not
	 btst	%g5, %o2			! don't waste delay slot, check if new one is wired
	LDPTR	[%o0 + PM_WIRED], %o1		! gonna update wired count
	bnz,pt	%xcc, 0f			! if wired changes, we predict it increments
	 mov	1, %o4
	neg	%o4				! new is not wired -> decrement
0:	add	%o1, %o4, %o1
	STPTR	%o1, [%o0 + PM_WIRED]
8:	retl
	 mov	%g1, %o0			! return %g1

9:	retl
	 or	%g1, 1, %o0			! spare needed, return flags + 1


/*
 * clearfpstate()
 *
 * Drops the current fpu state, without saving it.
 */
ENTRY(clearfpstate)
	rdpr	%pstate, %o1		! enable FPU
	wr	%g0, FPRS_FEF, %fprs
	or	%o1, PSTATE_PEF, %o1
	retl
	 wrpr	%o1, 0, %pstate

/*
 * savefpstate(f) struct fpstate *f;
 *
 * Store the current FPU state.
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

	stx	%fsr, [%o0 + FS_FSR]	! f->fs_fsr = getfsr();
	rd	%gsr, %o4		! Save %gsr
	st	%o4, [%o0 + FS_GSR]

	add	%o0, FS_REGS, %o2
#ifdef DIAGNOSTIC
	btst	BLOCK_ALIGN, %o2	! Needs to be re-executed
	bnz,pn	%icc, 6f		! Check alignment
#endif
	 st	%g0, [%o0 + FS_QSIZE]	! f->fs_qsize = 0;
	btst	FPRS_DL|FPRS_DU, %o5	! Both FPU halves clean?
	bz,pt	%icc, 5f		! Then skip it

	 btst	FPRS_DL, %o5		! Lower FPU clean?
	membar	#Sync
	bz,a,pt	%icc, 1f		! Then skip it, but upper FPU not clean
	 add	%o2, 2*BLOCK_SIZE, %o2	! Skip a block

	stda	%f0, [%o2] ASI_BLK_P	! f->fs_f0 = etc;
	inc	BLOCK_SIZE, %o2
	stda	%f16, [%o2] ASI_BLK_P

	btst	FPRS_DU, %o5		! Upper FPU clean?
	bz,pt	%icc, 2f		! Then skip it
	 inc	BLOCK_SIZE, %o2
1:
	stda	%f32, [%o2] ASI_BLK_P
	inc	BLOCK_SIZE, %o2
	stda	%f48, [%o2] ASI_BLK_P
2:
	membar	#Sync			! Finish operation so we can
5:
	retl
	 wr	%g0, FPRS_FEF, %fprs	! Mark FPU clean

#ifdef DIAGNOSTIC
	!!
	!! Damn thing is *NOT* aligned on a 64-byte boundary
	!! 
6:
	wr	%g0, FPRS_FEF, %fprs
	! XXX -- we should panic instead of silently entering debugger
	ta	1
	retl
	 nop
#endif

/*
 * Load FPU state.
 */
 /* XXXXXXXXXX  Should test to see if we only need to do a partial restore */
ENTRY(loadfpstate)
	flushw			! Make sure we don't have stack probs & lose hibits of %o
	rdpr	%pstate, %o1		! enable FP before we begin
	ld	[%o0 + FS_GSR], %o4	! Restore %gsr
	set	PSTATE_PEF, %o2
	wr	%g0, FPRS_FEF, %fprs
	or	%o1, %o2, %o1
	wrpr	%o1, 0, %pstate
	ldx	[%o0 + FS_FSR], %fsr	! setfsr(f->fs_fsr);
	add	%o0, FS_REGS, %o3	! This is zero...
#ifdef DIAGNOSTIC
	btst	BLOCK_ALIGN, %o3
	bne,pn	%icc, 1f	! Only use block loads on aligned blocks
#endif
	 wr	%o4, %g0, %gsr
	membar	#Sync
	ldda	[%o3] ASI_BLK_P, %f0
	inc	BLOCK_SIZE, %o3
	ldda	[%o3] ASI_BLK_P, %f16
	inc	BLOCK_SIZE, %o3
	ldda	[%o3] ASI_BLK_P, %f32
	inc	BLOCK_SIZE, %o3
	ldda	[%o3] ASI_BLK_P, %f48
	membar	#Sync			! Make sure loads are complete
	retl
	 wr	%g0, FPRS_FEF, %fprs	! Clear dirty bits

#ifdef DIAGNOSTIC
	!!
	!! Damn thing is *NOT* aligned on a 64-byte boundary
	!! 
1:
	wr	%g0, FPRS_FEF, %fprs	! Clear dirty bits
	! XXX -- we should panic instead of silently entering debugger
	ta	1
	retl
	 nop
#endif

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
 * interrupt instead of a polled interrupt.  This does pretty much the same
 * as interrupt_vector.  If cpu is -1 then send it to this CPU, if it's -2
 * send it to any CPU, otherwise send it to a particular CPU.
 *
 * XXXX Dispatching to different CPUs is not implemented yet.
 */
ENTRY(send_softint)
	rdpr	%pstate, %g1
	andn	%g1, PSTATE_IE, %g2	! clear PSTATE.IE
	wrpr	%g2, 0, %pstate

	sethi	%hi(CPUINFO_VA+CI_INTRPENDING), %o3
	LDPTR	[%o2 + IH_PEND], %o5
	or	%o3, %lo(CPUINFO_VA+CI_INTRPENDING), %o3
	brnz	%o5, 1f
	 sll	%o1, PTRSHFT, %o5	! Find start of table for this IPL
	add	%o3, %o5, %o3
2:
	LDPTR	[%o3], %o5		! Load list head
	STPTR	%o5, [%o2+IH_PEND]	! Link our intrhand node in
	mov	%o2, %o4
	CASPTRA	[%o3] ASI_N, %o5, %o4
	cmp	%o4, %o5		! Did it work?
	bne,pn	CCCR, 2b		! No, try again
	 .empty

	mov	1, %o4			! Change from level to bitmask
	sllx	%o4, %o1, %o4
	wr	%o4, 0, SET_SOFTINT	! SET_SOFTINT
1:
	retl
	 wrpr	%g1, 0, %pstate		! restore PSTATE.IE


#define MICROPERSEC	(1000000)

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
 *	ci_cpu_clockrate should be tuned during CPU probe to the CPU
 *	clockrate in Hz
 *
 */
ENTRY(delay)			! %o0 = n
#if 1
	rdpr	%tick, %o1					! Take timer snapshot
	sethi	%hi(CPUINFO_VA + CI_CLOCKRATE), %o2
	sethi	%hi(MICROPERSEC), %o3
	ldx	[%o2 + %lo(CPUINFO_VA + CI_CLOCKRATE + 8)], %o4	! Get scale factor
	brnz,pt	%o4, 0f
	 or	%o3, %lo(MICROPERSEC), %o3

	!! Calculate ticks/usec
	ldx	[%o2 + %lo(CPUINFO_VA + CI_CLOCKRATE)], %o4	! No, we need to calculate it
	udivx	%o4, %o3, %o4
	stx	%o4, [%o2 + %lo(CPUINFO_VA + CI_CLOCKRATE + 8)]	! Save it so we don't need to divide again
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
	sethi	%hi(CPUINFO_VA + CI_CLOCKRATE), %g2
	sethi	%hi(MICROPERSEC), %o2
	ldx	[%g2 + %lo(CPUINFO_VA + CI_CLOCKRATE)], %g2	! Get scale factor
	or	%o2, %lo(MICROPERSEC), %o2
!	sethi	%hi(_C_LABEL(timerblurb), %o5			! This is if we plan to tune the clock
!	ld	[%o5 + %lo(_C_LABEL(timerblurb))], %o5		!  with respect to the counter/timer
	mulx	%o0, %g2, %g2					! Scale it: (usec * Hz) / 1 x 10^6 = ticks
	udivx	%g2, %o2, %g2
	add	%g1, %g2, %g2
!	add	%o5, %g2, %g2			5, %g2, %g2					! But this gets complicated
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
	rd	TICK_CMPR, %o2
	rdpr	%tick, %o1

	mov	1, %o3		! Mask off high bits of these registers
	sllx	%o3, 63, %o3
	andn	%o1, %o3, %o1
	andn	%o2, %o3, %o2
	cmp	%o1, %o2	! Did we wrap?  (tick < tick_cmpr)
	bgt,pt	%icc, 1f
	 add	%o1, 1000, %o1	! Need some slack so we don't lose intrs.

	/*
	 * Handle the unlikely case of %tick wrapping.
	 *
	 * This should only happen every 10 years or more.
	 *
	 * We need to increment the time base by the size of %tick in
	 * microseconds.  This will require some divides and multiplies
	 * which can take time.  So we re-read %tick.
	 *
	 */

	/* XXXXX NOT IMPLEMENTED */



1:
	add	%o2, %o0, %o2
	andn	%o2, %o3, %o4
	brlz,pn	%o4, Ltick_ovflw
	 cmp	%o2, %o1	! Has this tick passed?
	blt,pn	%xcc, 1b	! Yes
	 nop

#ifdef BB_ERRATA_1
	ba,a	2f
	 nop
#else
	retl
	 wr	%o2, TICK_CMPR
#endif

Ltick_ovflw:
/*
 * When we get here tick_cmpr has wrapped, but we don't know if %tick
 * has wrapped.  If bit 62 is set then we have not wrapped and we can
 * use the current value of %o4 as %tick.  Otherwise we need to return
 * to our loop with %o4 as %tick_cmpr (%o2).
 */
	srlx	%o3, 1, %o5
	btst	%o5, %o1
	bz,pn	%xcc, 1b
	 mov	%o4, %o2
#ifdef BB_ERRATA_1
	ba,a	2f
	 nop
	.align	64
2:	wr	%o2, TICK_CMPR
	rd	TICK_CMPR, %g0
	retl
	 nop
#else
	retl
	 wr	%o2, TICK_CMPR
#endif

/*
 * next_stick(long increment)
 *
 * Sets the %stick_cmpr register to fire off in `increment' machine
 * cycles in the future.  Also handles %stick wraparound.  In 32-bit
 * mode we're limited to a 32-bit increment.
 */
ENTRY(next_stick)
	rd	STICK_CMPR, %o2
	rd	STICK, %o1

	mov	1, %o3		! Mask off high bits of these registers
	sllx	%o3, 63, %o3
	andn	%o1, %o3, %o1
	andn	%o2, %o3, %o2
	cmp	%o1, %o2	! Did we wrap?  (tick < tick_cmpr)
	bgt,pt	%icc, 1f
	 add	%o1, 1000, %o1	! Need some slack so we don't lose intrs.

	/*
	 * Handle the unlikely case of %stick wrapping.
	 *
	 * This should only happen every 10 years or more.
	 *
	 * We need to increment the time base by the size of %stick in
	 * microseconds.  This will require some divides and multiplies
	 * which can take time.  So we re-read %stick.
	 *
	 */

	/* XXXXX NOT IMPLEMENTED */



1:
	add	%o2, %o0, %o2
	andn	%o2, %o3, %o4
	brlz,pn	%o4, Lstick_ovflw
	 cmp	%o2, %o1	! Has this stick passed?
	blt,pn	%xcc, 1b	! Yes
	 nop
	retl
	 wr	%o2, STICK_CMPR

Lstick_ovflw:
/*
 * When we get here tick_cmpr has wrapped, but we don't know if %stick
 * has wrapped.  If bit 62 is set then we have not wrapped and we can
 * use the current value of %o4 as %stick.  Otherwise we need to return
 * to our loop with %o4 as %stick_cmpr (%o2).
 */
	srlx	%o3, 1, %o5
	btst	%o5, %o1
	bz,pn	%xcc, 1b
	 mov	%o4, %o2
	retl
	 wr	%o2, STICK_CMPR

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

#if defined(DDB) || defined(KGDB)
	/*
	 * Debug stuff.  Dump the trap registers into buffer & set tl=0.
	 *
	 *  %o0 = *ts
	 */
ENTRY(savetstate)
	mov	%o0, %o1
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
	flushw			! Make sure we don't have stack probs & lose hibits of %o
	brz,pn	%o0, 2f
	 mov	%o0, %o2
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
	retl
	 wrpr	%o0, 0, %tl

	/*
	 * Switch to context in abs(%o0)
	 */
ENTRY(switchtoctx_us)
	set	DEMAP_CTX_SECONDARY, %o3
	stxa	%o3, [%o3] ASI_DMMU_DEMAP
	mov	CTX_SECONDARY, %o4
	stxa	%o3, [%o3] ASI_IMMU_DEMAP
	membar	#Sync
	stxa	%o0, [%o4] ASI_DMMU		! Maybe we should invalid
	sethi	%hi(KERNBASE), %o2
	membar	#Sync
	flush	%o2
	retl
	 nop

ENTRY(switchtoctx_usiii)
	mov	CTX_SECONDARY, %o4
	ldxa	[%o4] ASI_DMMU, %o2		! Load secondary context
	mov	CTX_PRIMARY, %o5
	ldxa	[%o5] ASI_DMMU, %o1		! Save primary context
	membar	#LoadStore
	stxa	%o2, [%o5] ASI_DMMU		! Insert secondary for demap
	membar	#Sync
	set	DEMAP_CTX_PRIMARY, %o3
	stxa	%o3, [%o3] ASI_DMMU_DEMAP
	membar	#Sync
	stxa	%o0, [%o4] ASI_DMMU		! Maybe we should invalid
	membar	#Sync
	stxa	%o1, [%o5] ASI_DMMU		! Restore primary context
	sethi	%hi(KERNBASE), %o2
	membar	#Sync
	flush	%o2
	retl
	 nop

#ifndef _LP64
	/*
	 * Convert to 32-bit stack then call OF_sym2val()
	 */
ENTRY(OF_sym2val32)
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
ENTRY(OF_val2sym32)
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


#if defined(MULTIPROCESSOR)
/*
 * IPI target function to setup a C compatible environment and call a MI function.
 *
 * On entry:
 *	We are on one of the alternate set of globals
 *	%g2 = function to call
 *	%g3 = single argument to called function
 */
ENTRY(sparc64_ipi_ccall)
#ifdef TRAPS_USE_IG
	wrpr	%g0, PSTATE_KERN|PSTATE_IG, %pstate	! DEBUG
#endif
	TRAP_SETUP(-CC64FSZ-TF_SIZE)

#ifdef DEBUG
	rdpr	%tt, %o1	! debug
	sth	%o1, [%sp + CC64FSZ + STKB + TF_TT]! debug
#endif
	mov	%g3, %o0			! save argument of function to call
	mov	%g2, %o5			! save function pointer

	wrpr	%g0, PSTATE_KERN, %pstate	! Get back to normal globals
	stx	%g1, [%sp + CC64FSZ + STKB + TF_G + ( 1*8)]
	mov	%g1, %o1			! code
	rdpr	%tpc, %o2			! (pc)
	stx	%g2, [%sp + CC64FSZ + STKB + TF_G + ( 2*8)]
	rdpr	%tstate, %g1
	stx	%g3, [%sp + CC64FSZ + STKB + TF_G + ( 3*8)]
	rdpr	%tnpc, %o3
	stx	%g4, [%sp + CC64FSZ + STKB + TF_G + ( 4*8)]
	rd	%y, %o4
	stx	%g5, [%sp + CC64FSZ + STKB + TF_G + ( 5*8)]
	stx	%g6, [%sp + CC64FSZ + STKB + TF_G + ( 6*8)]
	wrpr	%g0, 0, %tl			! return to tl=0
	stx	%g7, [%sp + CC64FSZ + STKB + TF_G + ( 7*8)]

	stx	%g1, [%sp + CC64FSZ + STKB + TF_TSTATE]
	stx	%o2, [%sp + CC64FSZ + STKB + TF_PC]
	stx	%o3, [%sp + CC64FSZ + STKB + TF_NPC]
	st	%o4, [%sp + CC64FSZ + STKB + TF_Y]

	rdpr	%pil, %g5
	stb	%g5, [%sp + CC64FSZ + STKB + TF_PIL]
	stb	%g5, [%sp + CC64FSZ + STKB + TF_OLDPIL]

	!! In the EMBEDANY memory model %g4 points to the start of the data segment.
	!! In our case we need to clear it before calling any C-code
	clr	%g4
	wr	%g0, ASI_NUCLEUS, %asi			! default kernel ASI

	call %o5					! call function
	 nop

	ba,a	return_from_trap			! and return from IPI
	 nop

#endif


	.data
	_ALIGN
#if NKSYMS || defined(DDB) || defined(MODULAR)
	.globl	_C_LABEL(esym)
_C_LABEL(esym):
	POINTER	0
	.globl	_C_LABEL(ssym)
_C_LABEL(ssym):
	POINTER	0
#endif
	.comm	_C_LABEL(promvec), PTRSZ

#ifdef DEBUG
	.comm	_C_LABEL(trapdebug), 4
	.comm	_C_LABEL(pmapdebug), 4
#endif
