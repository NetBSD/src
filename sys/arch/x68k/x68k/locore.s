/*	$NetBSD: locore.s,v 1.151 2026/03/21 20:14:58 thorpej Exp $	*/

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

#include "ite.h"
#include "fd.h"
#include "par.h"
#include "assym.h"
#include "ksyms.h"

#include <machine/asm.h>

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	PAGE_SIZE
ASLOCAL(tmpstk)

#include <x68k/x68k/vectors.s>

	.text
/*
 * This is where we wind up if the kernel jumps to location 0.
 * (i.e. a bogus PC)  This is known to immediately follow the vector
 * table and is hence at 0x400 (see reset vector in vectors.s).
 */
	PANIC("kernel jump to zero")
	/* NOTREACHED */

/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#define	IMMEDIATE	#
#define	_RELOC(var, ar)				\
	movl	IMMEDIATE var,ar;		\
	addl	%a5,ar

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

/*
 * Initialization
 *
 * A4 contains the address of the end of the symtab
 * A5 contains physical load point from boot
 * VBR contains zero from ROM.  Exceptions will continue to vector
 * through ROM until MMU is turned on at which time they will vector
 * through our table (vectors.s).
 */

BSS(lowram,4)
BSS(esym,4)

GLOBAL(_verspad)
	.word	0
GLOBAL(boot_version)
	.word	X68K_BOOTIF_VERS

ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts

	addql	#4,%sp
	movel	%sp@+,%a5		| firstpa
	movel	%sp@+,%d5		| fphysize -- last page
	movel	%sp@,%a4		| esym

	RELOC(vectab,%a0)		| set Vector Base Register temporarily
	movc	%a0,%vbr

#if 0	/* XXX this should be done by the boot loader */
	RELOC(edata, %a0)		| clear out BSS
	movl	#_C_LABEL(end)-4,%d0	| (must be <= 256 kB)
	subl	#_C_LABEL(edata),%d0
	lsrl	#2,%d0
1:	clrl	%a0@+
	dbra	%d0,1b
#endif

	ASRELOC(tmpstk, %a0)
	movl	%a0,%sp			| give ourselves a temporary stack
	RELOC(esym, %a0)
#if 1
	movl	%a4,%a0@		| store end of symbol table
#else
	clrl	%a0@			| no symbol table, yet
#endif
	RELOC(boothowto, %a0)
	movl	%d7,%a0@		| save reboot flags
	RELOC(bootdev, %a0)
	movl	%d6,%a0@		|   and boot device
	RELOC(lowram, %a0)
	movl	%a5,%a0@		| store start of physical memory

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

/* determine our CPU/MMU combo - check for all regardless of kernel config */
	movl	#DC_FREEZE,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68020/68040/68060
	jra	Lstart1			| no, we have 68030
Lnot68030:
	bset	#31,%d0			| data cache enable bit
	movc	%d0,%cacr		|   only exists on 68040/68060
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lis68020		| yes, we have 68020
	moveq	#0,%d0			| now turn it back off
	movec	%d0,%cacr		|   before we access any data
	.word	0xf4d8			| cinva bc - invalidate caches XXX
	bset	#30,%d0			| data cache no allocate mode bit
	movc	%d0,%cacr		|   only exists on 68060
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lis68040		| yes, we have 68040
	RELOC(mmutype, %a0)		| no, we have 68060
	movl	#MMU_68040,%a0@		| with a 68040 compatible MMU
	RELOC(cputype, %a0)
	movl	#CPU_68060,%a0@		| and a 68060 CPU
	jra	Lstart1
Lis68040:
	RELOC(mmutype, %a0)
	movl	#MMU_68040,%a0@		| with a 68040 MMU
	RELOC(cputype, %a0)
	movl	#CPU_68040,%a0@		| and a 68040 CPU
	jra	Lstart1
Lis68020:
	RELOC(mmutype, %a0)
	movl	#MMU_68851,%a0@		| we have PMMU
	RELOC(cputype, %a0)
	movl	#CPU_68020,%a0@		| and a 68020 CPU

Lstart1:
/* initialize memory sizes (for pmap_bootstrap) */
	movl	%d5,%d1			| last page
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page (click) number
	RELOC(maxmem, %a0)
	movl	%d1,%a0@		| save as maxmem
	movl	%a5,%d0			| lowram value from ROM via boot
	lsrl	%d2,%d0			| convert to page number
	subl	%d0,%d1			| compute amount of RAM present
	RELOC(physmem, %a0)
	movl	%d1,%a0@		| and physmem

/* configure kernel and lwp0 VA space so we can get going */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	RELOC(esym,%a0)			| end of static kernel test/data/syms
	movl	%a0@,%a4
	tstl	%a4
	jne	Lstart3
#endif
	movl	#_C_LABEL(end),%a4	| end of static kernel text/data
Lstart3:
	RELOC(setmemrange,%a0)		| call setmemrange()
	jbsr	%a0@			|  to probe all memory regions
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| reloff
	pea	%a4@			| nextpa
	RELOC(pmap_bootstrap1,%a0)
	jbsr	%a0@			| pmap_bootstrap1(firstpa, nextpa)
	addql	#8,%sp

	/*
	 * Updated nextpa returned in %d0.  We need to squirrel
	 * that away in a callee-saved register to use later,
	 * after the MMU is enabled.
	 */
	movl	%d0, %d7

	/* NOTE: %d7 is now off-limits!! */

/*
 * Prepare to enable MMU.
 * Since the kernel is mapped logical == physical, we just turn it on.
 */
	RELOC(Sysseg_pa, %a0)		| system segment table addr
	movl	%a0@,%d1		| read value (a PA)
	RELOC(mmutype, %a0)
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu1		| no, skip
	.long	0x4e7b1807		| movc %d1,%srp
	jra	Lstploaddone
Lmotommu1:
	RELOC(protorp, %a0)
	movl	%d1,%a0@(4)		| segtable address
	pmove	%a0@,%srp		| load the supervisor root pointer
Lstploaddone:
	RELOC(mmutype, %a0)
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu2		| no, skip
#include "opt_jupiter.h"
#ifdef JUPITER
	/* JUPITER-X: set system register "SUPER" bit */
	movl	#0x0200a240,%d0		| translate DRAM area transparently
	.long	0x4e7b0006		| movc d0,dtt0
	lea	0x00c00000,%a0		| %a0: graphic VRAM
	lea	0x02c00000,%a1		| %a1: graphic VRAM ( not JUPITER-X )
					|      DRAM ( JUPITER-X )
	movw	%a0@,%d0
	movw	%d0,%d1
	notw	%d1
	movw	%d1,%a1@
	movw	%d0,%a0@
	cmpw	%a1@,%d1		| JUPITER-X?
	jne	Ljupiterdone		| no, skip
	movl	#0x0100a240,%d0		| to access system register
	.long	0x4e7b0006		| movc %d0,%dtt0
	movb	#0x01,0x01800003	| set "SUPER" bit
Ljupiterdone:
#endif /* JUPITER */
	moveq	#0,%d0			| ensure TT regs are disabled
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0006		| movc %d0,%dtt0
	.long	0x4e7b0007		| movc %d0,%dtt1
	.word	0xf4d8			| cinva bc
	.word	0xf518			| pflusha
	movl	#MMU40_TCR_BITS,%d0
	.long	0x4e7b0003		| movc %d0,%tc
#ifdef M68060
	RELOC(cputype, %a0)
	cmpl	#CPU_68060,%a0@		| 68060?
	jne	Lnot060cache
	movl	#1,%d0
	.long	0x4e7b0808		| movcl %d0,%pcr
	movl	#0xa0808000,%d0
	movc	%d0,%cacr		| enable store buffer, both caches
	jmp	Lenab1
Lnot060cache:
#endif
	movl	#CACHE40_ON,%d0
	movc	%d0,%cacr		| turn on both caches
	jmp	Lenab1
Lmotommu2:
	pflusha
	movl	#MMU51_TCR_BITS,%sp@-	| value to load TC with
	pmove	%sp@,%tc		| load it

/*
 * Should be running mapped from this point on
 */
Lenab1:
	lea	_ASM_LABEL(tmpstk),%sp	| temporary stack
	jbsr	_C_LABEL(vec_init)	| initialize the vector table
/* phase 2 of pmap setup, returns pointer to lwp0 uarea in %a0 */
	jbsr	_C_LABEL(pmap_bootstrap2)
/* set kernel stack, user SP */
	lea	%a0@(USPACE-4),%sp	| set kernel stack to end of area
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP

	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jeq	Ltbia040		| yes, cache already on
	pflusha
	tstl	_C_LABEL(mmutype)
	jpl	Lenab3			| 68851 implies no d-cache
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
	jra	Lenab3
Ltbia040:
	.word	0xf518			| pflusha
Lenab3:
/* final setup for C code */
	movl	%d7,%sp@-		| push nextpa saved above
	jbsr	_C_LABEL(x68k_init)	| additional pre-main initialization
	addql	#4,%sp

/*
 * Create a fake exception frame so that cpu_lwp_fork() can copy it.
 * main() nevers returns; we exit to user mode from a forked process
 * later on.
 */
	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count and padding
	lea	%sp@(-64),%sp		| construct space for D0-D7/A0-A7
	lea	_C_LABEL(lwp0),%a0	| save pointer to frame
	movl	%sp,%a0@(L_MD_REGS)	|   in lwp0.l_md.md_regs

	jra	_C_LABEL(main)		| main()

	PANIC("main() returned")	| Yow!  Main returned!
	/* NOTREACHED */

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

/*
 * Provide a generic interrupt dispatcher, only handle hardclock (int6)
 * specially, to improve performance
 */

ENTRY_NOPROFILE(spurintr)	/* level 0 */
	addql	#1,_C_LABEL(intrcnt)+0
	INTERRUPT_SAVEREG
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	rte				| XXX mfpcure (x680x0 hardware bug)

ENTRY_NOPROFILE(kbdtimer)
	rte

ENTRY_NOPROFILE(intiotrap)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG
	pea	%sp@(16-(FR_HW))	| XXX
	jbsr	_C_LABEL(intio_intr)
	addql	#4,%sp
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(intr_depth)
	jra	rei

ENTRY_NOPROFILE(lev1intr)
ENTRY_NOPROFILE(lev2intr)
ENTRY_NOPROFILE(lev3intr)
ENTRY_NOPROFILE(lev4intr)
ENTRY_NOPROFILE(lev5intr)
ENTRY_NOPROFILE(lev6intr)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG
Lnotdma:
	lea	_C_LABEL(intrcnt),%a0
	movw	%sp@(22),%d0		| use vector offset
	andw	#0xfff,%d0		|   sans frame type
	addql	#1,%a0@(-0x60,%d0:w)	|     to increment apropos counter
	movw	%sr,%sp@-		| push current SR value
	clrw	%sp@-			|    padded to longword
	jbsr	_C_LABEL(intrhand)	| handle interrupt
	addql	#4,%sp			| pop SR
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(timertrap)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG		| save scratch registers
	addql	#1,_C_LABEL(intrcnt)+32	| count hardclock interrupts
	movl	%sp,%sp@-		| push pointer to clockframe
	jbsr	_C_LABEL(hardclock)	| hardclock(&frame)
	addql	#4,%sp
	CPUINFO_INCREMENT(CI_NINTR)	| chalk up another interrupt
	INTERRUPT_RESTOREREG		| restore scratch registers
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev7intr)
	addql	#1,_C_LABEL(intr_depth)
	addql	#1,_C_LABEL(intrcnt)+28
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-		| save registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	jbsr	_C_LABEL(nmihand)	| call handler
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		| all done

/*
 * floppy ejection trap
 */

ENTRY_NOPROFILE(fdeject)
	jra	_ASM_LABEL(rei)

/*
 * Primitives
 */

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#define FPCOPROC	/* XXX: Temp. reqd. */
#include <m68k/m68k/switch_subr.s>


#if defined(M68040) || defined(M68060)
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
 * _delay(u_int N)
 *
 * Delay for at least N microseconds.
 * This routine depends on the variable:  delay_divisor
 * which should be set based on the CPU clock rate.
 */
ENTRY_NOPROFILE(_delay)
	| %d0 = (usecs * a certain magnification factor)
	movl	%sp@(4),%d0
	lsll	#8,%d0
	| %d1 = delay_divisor
	movl	_C_LABEL(delay_divisor),%d1
L_delay:
	subl	%d1,%d0
	jgt	L_delay
	rts

/*
 * Handle the nitty-gritty of rebooting the machine.
 * Basically we just turn off the MMU and jump to the appropriate ROM routine.
 * Note that we must be running in an address range that is mapped one-to-one
 * logical to physical so that the PC is still valid immediately after the MMU
 * is turned off.  We have conveniently mapped the last page of physical
 * memory this way.
 */
ENTRY_NOPROFILE(doboot)
	movw	#PSL_HIGHIPL,%sr	| cut off any interrupts
	subal	%a1,%a1			| a1 = 0

	movl	#CACHE_OFF,%d0
#if defined(M68040) || defined(M68060)
	movl	_C_LABEL(mmutype),%d2	| d2 = mmutype
	addl	#(-1 * MMU_68040),%d2		| 68040?
	jne	Ldoboot0		| no, skip
	.word	0xf4f8			| cpusha bc - push and invalidate caches
	nop
	movl	#CACHE40_OFF,%d0
Ldoboot0:
#endif
	movc	%d0,%cacr		| disable on-chip cache(s)

	| ok, turn off MMU..
Ldoreboot:
#if defined(M68040) || defined(M68060)
	tstl	%d2			| 68040?
	jne	LmotommuF		| no, skip
	movc	%a1,%cacr		| caches off
	.long	0x4e7b9003		| movc a1(=0),tc ; disable MMU
	jra	Ldoreboot1
LmotommuF:
#endif
	clrl	%sp@
	pmove	%sp@,%tc		| disable MMU
Ldoreboot1:
	moveml	0x00ff0000,#0x0101	| get RESET vectors in ROM
					|	(d0: ssp, a0: pc)
	moveml	#0x0101,%a1@		| put them at 0x0000 (for Xellent30)
	movc	%a1,%vbr		| reset Vector Base Register
	jmp	%a0@			| reboot X680x0
Lebootcode:

/*
 * Misc. global variables.
 */
	.data
GLOBAL(mmutype)
	.long	MMU_68030		| default to 030 internal MMU

GLOBAL(cputype)
	.long	CPU_68030		| default to 68030 CPU

#ifdef M68K_MMU_HP
GLOBAL(ectype)
	.long	EC_NONE			| external cache type, default to none
#endif

GLOBAL(intiobase)
	.long	0			| KVA of base of internal IO space

#ifdef DEBUG
ASGLOBAL(fulltflush)
	.long	0

ASGLOBAL(fullcflush)
	.long	0
#endif

/* interrupt counters */

GLOBAL(intrnames)
	.asciz	"spur"
	.asciz	"lev1"
	.asciz	"lev2"
	.asciz	"lev3"
	.asciz	"lev4"
	.asciz	"lev5"
	.asciz	"lev6"
	.asciz	"nmi"
	.asciz	"clock"
	.asciz	"com"
GLOBAL(eintrnames)
	.even

GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)
