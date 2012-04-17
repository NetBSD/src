/*	$NetBSD: locore.s,v 1.265.2.1 2012/04/17 00:06:54 yamt Exp $	*/

/*
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by Paul Kranenburg.
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
 *	@(#)locore.s	8.4 (Berkeley) 12/10/93
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/param.h>
#include <machine/asm.h>
#include <sparc/sparc/intreg.h>
#include <sparc/sparc/timerreg.h>
#include <sparc/sparc/vaddrs.h>
#ifdef notyet
#include <sparc/dev/zsreg.h>
#endif
#include <machine/ctlreg.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/signal.h>
#include <machine/trap.h>
	
#include <sys/syscall.h>

/* use as needed to align things on longword boundaries */
#define	_ALIGN	.align 4

/*
 * CCFSZ (C Compiler Frame SiZe) is the size of a stack frame required if
 * a function is to call C code.  It should be just 64, but Sun defined
 * their frame with space to hold arguments 0 through 5 (plus some junk),
 * and varargs routines (such as printf) demand this, and gcc uses this
 * area at times anyway.
 */
#define	CCFSZ	96

/* We rely on the fact that %lo(CPUINFO_VA) is zero */
.if CPUINFO_VA & 0x1fff
BARF
.endif

#if EV_COUNT != 0
# error "this code does not work with EV_COUNT != 0"
#endif
#if EV_STRUCTSIZE != 32
# error "this code does not work with EV_STRUCTSIZE != 32"
#else
# define EV_STRUCTSHIFT	5
#endif

/*
 * Another handy macro: load one register window, given `base' address.
 * This can be either a simple register (e.g., %sp) or include an initial
 * offset (e.g., %g6 + PCB_RW).
 */
#define	LOADWIN(addr) \
	ldd	[addr], %l0; \
	ldd	[addr + 8], %l2; \
	ldd	[addr + 16], %l4; \
	ldd	[addr + 24], %l6; \
	ldd	[addr + 32], %i0; \
	ldd	[addr + 40], %i2; \
	ldd	[addr + 48], %i4; \
	ldd	[addr + 56], %i6

/*
 * To return from trap we need the two-instruction sequence
 * `jmp %l1; rett %l2', which is defined here for convenience.
 */
#define	RETT	jmp %l1; rett %l2

	.data
/*
 * The interrupt stack.
 *
 * This is the very first thing in the data segment, and therefore has
 * the lowest kernel stack address.  We count on this in the interrupt
 * trap-frame setup code, since we may need to switch from the kernel
 * stack to the interrupt stack (iff we are not already on the interrupt
 * stack).  One sethi+cmp is all we need since this is so carefully
 * arranged.
 *
 * In SMP kernels, each CPU has its own interrupt stack and the computation
 * to determine whether we're already on the interrupt stack is slightly
 * more time consuming (see INTR_SETUP() below).
 */
	.globl	_C_LABEL(intstack)
	.globl	_C_LABEL(eintstack)
_C_LABEL(intstack):
	.skip	INT_STACK_SIZE		! 16k = 128 128-byte stack frames
_C_LABEL(eintstack):

_EINTSTACKP = CPUINFO_VA + CPUINFO_EINTSTACK

/*
 * CPUINFO_VA is a CPU-local virtual address; cpi->ci_self is a global
 * virtual address for the same structure.  It must be stored in p->p_cpu
 * upon context switch.
 */
_CISELFP	= CPUINFO_VA + CPUINFO_SELF
_CIFLAGS	= CPUINFO_VA + CPUINFO_FLAGS

/* Per-CPU AST requests */
_WANT_AST	= CPUINFO_VA + CPUINFO_WANT_AST

/*
 * Process 0's u.
 *
 * This must be aligned on an 8 byte boundary.
 */
	.globl	_C_LABEL(u0)
_C_LABEL(u0):	.skip	USPACE
estack0:

#ifdef KGDB
/*
 * Another item that must be aligned, easiest to put it here.
 */
KGDB_STACK_SIZE = 2048
	.globl	_C_LABEL(kgdb_stack)
_C_LABEL(kgdb_stack):
	.skip	KGDB_STACK_SIZE		! hope this is enough
#endif

/*
 * cpcb points to the current pcb (and hence u. area).
 * Initially this is the special one.
 */
cpcb = CPUINFO_VA + CPUINFO_CURPCB

/* curlwp points to the current LWP that has the CPU */
curlwp = CPUINFO_VA + CPUINFO_CURLWP

/*
 * cputyp is the current CPU type, used to distinguish between
 * the many variations of different sun4* machines. It contains
 * the value CPU_SUN4, CPU_SUN4C, or CPU_SUN4M.
 */
	.globl	_C_LABEL(cputyp)
_C_LABEL(cputyp):
	.word	1

#if defined(SUN4C) || defined(SUN4M)
cputypval:
	.asciz	"sun4c"
	.ascii	"     "
cputypvar:
	.asciz	"compatible"
	_ALIGN
#endif

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

	.globl	_C_LABEL(trapbase)
_C_LABEL(trapbase):
	.word	0

#if 0
#if defined(SUN4M)
_mapme:
	.asciz "0 0 f8000000 15c6a0 map-pages"
#endif
#endif

#if !defined(SUN4D)
sun4d_notsup:
	.asciz	"cr .( NetBSD/sparc: this kernel does not support the sun4d) cr"
#endif
#if !defined(SUN4M)
sun4m_notsup:
	.asciz	"cr .( NetBSD/sparc: this kernel does not support the sun4m) cr"
#endif
#if !defined(SUN4C)
sun4c_notsup:
	.asciz	"cr .( NetBSD/sparc: this kernel does not support the sun4c) cr"
#endif
#if !defined(SUN4)
sun4_notsup:
	! the extra characters at the end are to ensure the zs fifo drains
	! before we halt. Sick, eh?
	.asciz	"NetBSD/sparc: this kernel does not support the sun4\n\r \b"
#endif
	_ALIGN

	.text

/*
 * The first thing in the real text segment is the trap vector table,
 * which must be aligned on a 4096 byte boundary.  The text segment
 * starts beyond page 0 of KERNBASE so that there is a red zone
 * between user and kernel space.  Since the boot ROM loads us at
 * PROM_LOADADDR, it is far easier to start at KERNBASE+PROM_LOADADDR than to
 * buck the trend.  This is two or four pages in (depending on if
 * pagesize is 8192 or 4096).    We place two items in this area:
 * the message buffer (phys addr 0) and the cpu_softc structure for
 * the first processor in the system (phys addr 0x2000).
 * Because the message buffer is in our "red zone" between user and
 * kernel space we remap it in configure() to another location and
 * invalidate the mapping at KERNBASE.
 */

/*
 * Each trap has room for four instructions, of which one perforce must
 * be a branch.  On entry the hardware has copied pc and npc to %l1 and
 * %l2 respectively.  We use two more to read the psr into %l0, and to
 * put the trap type value into %l3 (with a few exceptions below).
 * We could read the trap type field of %tbr later in the code instead,
 * but there is no need, and that would require more instructions
 * (read+mask, vs 1 `mov' here).
 *
 * I used to generate these numbers by address arithmetic, but gas's
 * expression evaluator has about as much sense as your average slug
 * (oddly enough, the code looks about as slimy too).  Thus, all the
 * trap numbers are given as arguments to the trap macros.  This means
 * there is one line per trap.  Sigh.
 *
 * Note that only the local registers may be used, since the trap
 * window is potentially the last window.  Its `in' registers are
 * the previous window's outs (as usual), but more important, its
 * `out' registers may be in use as the `topmost' window's `in' registers.
 * The global registers are of course verboten (well, until we save
 * them away).
 *
 * Hardware interrupt vectors can be `linked'---the linkage is to regular
 * C code---or rewired to fast in-window handlers.  The latter are good
 * for unbuffered hardware like the Zilog serial chip and the AMD audio
 * chip, where many interrupts can be handled trivially with pseudo-DMA or
 * similar.  Only one `fast' interrupt can be used per level, however, and
 * direct and `fast' interrupts are incompatible.  Routines in intr.c
 * handle setting these, with optional paranoia.
 */

	/* regular vectored traps */
#define	VTRAP(type, label) \
	mov (type), %l3; b label; mov %psr, %l0; nop

	/* hardware interrupts (can be linked or made `fast') */
#define	HARDINT44C(lev) \
	mov (lev), %l3; b _C_LABEL(sparc_interrupt44c); mov %psr, %l0; nop

	/* hardware interrupts (can be linked or made `fast') */
#define	HARDINT4M(lev) \
	mov (lev), %l3; b _C_LABEL(sparc_interrupt4m); mov %psr, %l0; nop

	/* software interrupts (may not be made direct, sorry---but you
	   should not be using them trivially anyway) */
#define	SOFTINT44C(lev, bit) \
	mov (lev), %l3; mov (bit), %l4; b softintr_sun44c; mov %psr, %l0

	/* There's no SOFTINT4M(): both hard and soft vector the same way */

	/* traps that just call trap() */
#define	TRAP(type)	VTRAP(type, slowtrap)

	/* architecturally undefined traps (cause panic) */
#define	UTRAP(type)	VTRAP(type, slowtrap)

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

/* special high-speed 1-instruction-shaved-off traps (get nothing in %l3) */
#define	SYSCALL		b _C_LABEL(_syscall); mov %psr, %l0; nop; nop
#define	WINDOW_OF	b window_of; mov %psr, %l0; nop; nop
#define	WINDOW_UF	b window_uf; mov %psr, %l0; nop; nop
#ifdef notyet
#define	ZS_INTERRUPT	b zshard; mov %psr, %l0; nop; nop
#else
#define	ZS_INTERRUPT44C	HARDINT44C(12)
#define	ZS_INTERRUPT4M	HARDINT4M(12)
#endif

#ifdef DEBUG
#define TRAP_TRACE(tt, tmp)					\
	sethi	%hi(CPUINFO_VA + CPUINFO_TT), tmp;		\
	st	tt, [tmp + %lo(CPUINFO_VA + CPUINFO_TT)];
#define TRAP_TRACE2(tt, tmp1, tmp2)				\
	mov	tt, tmp1;					\
	TRAP_TRACE(tmp1, tmp2)
#else /* DEBUG */
#define TRAP_TRACE(tt,tmp)		/**/
#define TRAP_TRACE2(tt,tmp1,tmp2)	/**/
#endif /* DEBUG */

	.globl	_ASM_LABEL(start), _C_LABEL(kernel_text)
	_C_LABEL(kernel_text) = start		! for kvm_mkdb(8)
_ASM_LABEL(start):
/*
 * Put sun4 traptable first, since it needs the most stringent aligment (8192)
 */
#if defined(SUN4)
trapbase_sun4:
	/* trap 0 is special since we cannot receive it */
	b dostart; nop; nop; nop	! 00 = reset (fake)
	VTRAP(T_TEXTFAULT, memfault_sun4)	! 01 = instr. fetch fault
	TRAP(T_ILLINST)			! 02 = illegal instruction
	TRAP(T_PRIVINST)		! 03 = privileged instruction
	TRAP(T_FPDISABLED)		! 04 = fp instr, but EF bit off in psr
	WINDOW_OF			! 05 = window overflow
	WINDOW_UF			! 06 = window underflow
	TRAP(T_ALIGN)			! 07 = address alignment error
	VTRAP(T_FPE, fp_exception)	! 08 = fp exception
	VTRAP(T_DATAFAULT, memfault_sun4)	! 09 = data fetch fault
	TRAP(T_TAGOF)			! 0a = tag overflow
	UTRAP(0x0b)
	UTRAP(0x0c)
	UTRAP(0x0d)
	UTRAP(0x0e)
	UTRAP(0x0f)
	UTRAP(0x10)
	SOFTINT44C(1, IE_L1)		! 11 = level 1 interrupt
	HARDINT44C(2)			! 12 = level 2 interrupt
	HARDINT44C(3)			! 13 = level 3 interrupt
	SOFTINT44C(4, IE_L4)		! 14 = level 4 interrupt
	HARDINT44C(5)			! 15 = level 5 interrupt
	SOFTINT44C(6, IE_L6)		! 16 = level 6 interrupt
	HARDINT44C(7)			! 17 = level 7 interrupt
	HARDINT44C(8)			! 18 = level 8 interrupt
	HARDINT44C(9)			! 19 = level 9 interrupt
	HARDINT44C(10)			! 1a = level 10 interrupt
	HARDINT44C(11)			! 1b = level 11 interrupt
	ZS_INTERRUPT44C			! 1c = level 12 (zs) interrupt
	HARDINT44C(13)			! 1d = level 13 interrupt
	HARDINT44C(14)			! 1e = level 14 interrupt
	VTRAP(15, nmi_sun4)		! 1f = nonmaskable interrupt
	UTRAP(0x20)
	UTRAP(0x21)
	UTRAP(0x22)
	UTRAP(0x23)
	TRAP(T_CPDISABLED)	! 24 = coprocessor instr, EC bit off in psr
	UTRAP(0x25)
	UTRAP(0x26)
	UTRAP(0x27)
	TRAP(T_CPEXCEPTION)	! 28 = coprocessor exception
	UTRAP(0x29)
	UTRAP(0x2a)
	UTRAP(0x2b)
	UTRAP(0x2c)
	UTRAP(0x2d)
	UTRAP(0x2e)
	UTRAP(0x2f)
	UTRAP(0x30)
	UTRAP(0x31)
	UTRAP(0x32)
	UTRAP(0x33)
	UTRAP(0x34)
	UTRAP(0x35)
	UTRAP(0x36)
	UTRAP(0x37)
	UTRAP(0x38)
	UTRAP(0x39)
	UTRAP(0x3a)
	UTRAP(0x3b)
	UTRAP(0x3c)
	UTRAP(0x3d)
	UTRAP(0x3e)
	UTRAP(0x3f)
	UTRAP(0x40)
	UTRAP(0x41)
	UTRAP(0x42)
	UTRAP(0x43)
	UTRAP(0x44)
	UTRAP(0x45)
	UTRAP(0x46)
	UTRAP(0x47)
	UTRAP(0x48)
	UTRAP(0x49)
	UTRAP(0x4a)
	UTRAP(0x4b)
	UTRAP(0x4c)
	UTRAP(0x4d)
	UTRAP(0x4e)
	UTRAP(0x4f)
	UTRAP(0x50)
	UTRAP(0x51)
	UTRAP(0x52)
	UTRAP(0x53)
	UTRAP(0x54)
	UTRAP(0x55)
	UTRAP(0x56)
	UTRAP(0x57)
	UTRAP(0x58)
	UTRAP(0x59)
	UTRAP(0x5a)
	UTRAP(0x5b)
	UTRAP(0x5c)
	UTRAP(0x5d)
	UTRAP(0x5e)
	UTRAP(0x5f)
	UTRAP(0x60)
	UTRAP(0x61)
	UTRAP(0x62)
	UTRAP(0x63)
	UTRAP(0x64)
	UTRAP(0x65)
	UTRAP(0x66)
	UTRAP(0x67)
	UTRAP(0x68)
	UTRAP(0x69)
	UTRAP(0x6a)
	UTRAP(0x6b)
	UTRAP(0x6c)
	UTRAP(0x6d)
	UTRAP(0x6e)
	UTRAP(0x6f)
	UTRAP(0x70)
	UTRAP(0x71)
	UTRAP(0x72)
	UTRAP(0x73)
	UTRAP(0x74)
	UTRAP(0x75)
	UTRAP(0x76)
	UTRAP(0x77)
	UTRAP(0x78)
	UTRAP(0x79)
	UTRAP(0x7a)
	UTRAP(0x7b)
	UTRAP(0x7c)
	UTRAP(0x7d)
	UTRAP(0x7e)
	UTRAP(0x7f)
	SYSCALL			! 80 = sun syscall
	BPT			! 81 = pseudo breakpoint instruction
	TRAP(T_DIV0)		! 82 = divide by zero
	TRAP(T_FLUSHWIN)	! 83 = flush windows
	TRAP(T_CLEANWIN)	! 84 = provide clean windows
	TRAP(T_RANGECHECK)	! 85 = ???
	TRAP(T_FIXALIGN)	! 86 = fix up unaligned accesses
	TRAP(T_INTOF)		! 87 = integer overflow
	SYSCALL			! 88 = svr4 syscall
	SYSCALL			! 89 = bsd syscall
	BPT_KGDB_EXEC		! 8a = enter kernel gdb on kernel startup
	STRAP(0x8b)
	STRAP(0x8c)
	STRAP(0x8d)
	STRAP(0x8e)
	STRAP(0x8f)
	STRAP(0x90)
	STRAP(0x91)
	STRAP(0x92)
	STRAP(0x93)
	STRAP(0x94)
	STRAP(0x95)
	STRAP(0x96)
	STRAP(0x97)
	STRAP(0x98)
	STRAP(0x99)
	STRAP(0x9a)
	STRAP(0x9b)
	STRAP(0x9c)
	STRAP(0x9d)
	STRAP(0x9e)
	STRAP(0x9f)
	STRAP(0xa0)
	STRAP(0xa1)
	STRAP(0xa2)
	STRAP(0xa3)
	STRAP(0xa4)
	STRAP(0xa5)
	STRAP(0xa6)
	STRAP(0xa7)
	STRAP(0xa8)
	STRAP(0xa9)
	STRAP(0xaa)
	STRAP(0xab)
	STRAP(0xac)
	STRAP(0xad)
	STRAP(0xae)
	STRAP(0xaf)
	STRAP(0xb0)
	STRAP(0xb1)
	STRAP(0xb2)
	STRAP(0xb3)
	STRAP(0xb4)
	STRAP(0xb5)
	STRAP(0xb6)
	STRAP(0xb7)
	STRAP(0xb8)
	STRAP(0xb9)
	STRAP(0xba)
	STRAP(0xbb)
	STRAP(0xbc)
	STRAP(0xbd)
	STRAP(0xbe)
	STRAP(0xbf)
	STRAP(0xc0)
	STRAP(0xc1)
	STRAP(0xc2)
	STRAP(0xc3)
	STRAP(0xc4)
	STRAP(0xc5)
	STRAP(0xc6)
	STRAP(0xc7)
	STRAP(0xc8)
	STRAP(0xc9)
	STRAP(0xca)
	STRAP(0xcb)
	STRAP(0xcc)
	STRAP(0xcd)
	STRAP(0xce)
	STRAP(0xcf)
	STRAP(0xd0)
	STRAP(0xd1)
	STRAP(0xd2)
	STRAP(0xd3)
	STRAP(0xd4)
	STRAP(0xd5)
	STRAP(0xd6)
	STRAP(0xd7)
	STRAP(0xd8)
	STRAP(0xd9)
	STRAP(0xda)
	STRAP(0xdb)
	STRAP(0xdc)
	STRAP(0xdd)
	STRAP(0xde)
	STRAP(0xdf)
	STRAP(0xe0)
	STRAP(0xe1)
	STRAP(0xe2)
	STRAP(0xe3)
	STRAP(0xe4)
	STRAP(0xe5)
	STRAP(0xe6)
	STRAP(0xe7)
	STRAP(0xe8)
	STRAP(0xe9)
	STRAP(0xea)
	STRAP(0xeb)
	STRAP(0xec)
	STRAP(0xed)
	STRAP(0xee)
	STRAP(0xef)
	STRAP(0xf0)
	STRAP(0xf1)
	STRAP(0xf2)
	STRAP(0xf3)
	STRAP(0xf4)
	STRAP(0xf5)
	STRAP(0xf6)
	STRAP(0xf7)
	STRAP(0xf8)
	STRAP(0xf9)
	STRAP(0xfa)
	STRAP(0xfb)
	STRAP(0xfc)
	STRAP(0xfd)
	STRAP(0xfe)
	STRAP(0xff)
#endif

#if defined(SUN4C)
trapbase_sun4c:
/* trap 0 is special since we cannot receive it */
	b dostart; nop; nop; nop	! 00 = reset (fake)
	VTRAP(T_TEXTFAULT, memfault_sun4c)	! 01 = instr. fetch fault
	TRAP(T_ILLINST)			! 02 = illegal instruction
	TRAP(T_PRIVINST)		! 03 = privileged instruction
	TRAP(T_FPDISABLED)		! 04 = fp instr, but EF bit off in psr
	WINDOW_OF			! 05 = window overflow
	WINDOW_UF			! 06 = window underflow
	TRAP(T_ALIGN)			! 07 = address alignment error
	VTRAP(T_FPE, fp_exception)	! 08 = fp exception
	VTRAP(T_DATAFAULT, memfault_sun4c)	! 09 = data fetch fault
	TRAP(T_TAGOF)			! 0a = tag overflow
	UTRAP(0x0b)
	UTRAP(0x0c)
	UTRAP(0x0d)
	UTRAP(0x0e)
	UTRAP(0x0f)
	UTRAP(0x10)
	SOFTINT44C(1, IE_L1)		! 11 = level 1 interrupt
	HARDINT44C(2)			! 12 = level 2 interrupt
	HARDINT44C(3)			! 13 = level 3 interrupt
	SOFTINT44C(4, IE_L4)		! 14 = level 4 interrupt
	HARDINT44C(5)			! 15 = level 5 interrupt
	SOFTINT44C(6, IE_L6)		! 16 = level 6 interrupt
	HARDINT44C(7)			! 17 = level 7 interrupt
	HARDINT44C(8)			! 18 = level 8 interrupt
	HARDINT44C(9)			! 19 = level 9 interrupt
	HARDINT44C(10)			! 1a = level 10 interrupt
	HARDINT44C(11)			! 1b = level 11 interrupt
	ZS_INTERRUPT44C			! 1c = level 12 (zs) interrupt
	HARDINT44C(13)			! 1d = level 13 interrupt
	HARDINT44C(14)			! 1e = level 14 interrupt
	VTRAP(15, nmi_sun4c)		! 1f = nonmaskable interrupt
	UTRAP(0x20)
	UTRAP(0x21)
	UTRAP(0x22)
	UTRAP(0x23)
	TRAP(T_CPDISABLED)	! 24 = coprocessor instr, EC bit off in psr
	UTRAP(0x25)
	UTRAP(0x26)
	UTRAP(0x27)
	TRAP(T_CPEXCEPTION)	! 28 = coprocessor exception
	UTRAP(0x29)
	UTRAP(0x2a)
	UTRAP(0x2b)
	UTRAP(0x2c)
	UTRAP(0x2d)
	UTRAP(0x2e)
	UTRAP(0x2f)
	UTRAP(0x30)
	UTRAP(0x31)
	UTRAP(0x32)
	UTRAP(0x33)
	UTRAP(0x34)
	UTRAP(0x35)
	UTRAP(0x36)
	UTRAP(0x37)
	UTRAP(0x38)
	UTRAP(0x39)
	UTRAP(0x3a)
	UTRAP(0x3b)
	UTRAP(0x3c)
	UTRAP(0x3d)
	UTRAP(0x3e)
	UTRAP(0x3f)
	UTRAP(0x40)
	UTRAP(0x41)
	UTRAP(0x42)
	UTRAP(0x43)
	UTRAP(0x44)
	UTRAP(0x45)
	UTRAP(0x46)
	UTRAP(0x47)
	UTRAP(0x48)
	UTRAP(0x49)
	UTRAP(0x4a)
	UTRAP(0x4b)
	UTRAP(0x4c)
	UTRAP(0x4d)
	UTRAP(0x4e)
	UTRAP(0x4f)
	UTRAP(0x50)
	UTRAP(0x51)
	UTRAP(0x52)
	UTRAP(0x53)
	UTRAP(0x54)
	UTRAP(0x55)
	UTRAP(0x56)
	UTRAP(0x57)
	UTRAP(0x58)
	UTRAP(0x59)
	UTRAP(0x5a)
	UTRAP(0x5b)
	UTRAP(0x5c)
	UTRAP(0x5d)
	UTRAP(0x5e)
	UTRAP(0x5f)
	UTRAP(0x60)
	UTRAP(0x61)
	UTRAP(0x62)
	UTRAP(0x63)
	UTRAP(0x64)
	UTRAP(0x65)
	UTRAP(0x66)
	UTRAP(0x67)
	UTRAP(0x68)
	UTRAP(0x69)
	UTRAP(0x6a)
	UTRAP(0x6b)
	UTRAP(0x6c)
	UTRAP(0x6d)
	UTRAP(0x6e)
	UTRAP(0x6f)
	UTRAP(0x70)
	UTRAP(0x71)
	UTRAP(0x72)
	UTRAP(0x73)
	UTRAP(0x74)
	UTRAP(0x75)
	UTRAP(0x76)
	UTRAP(0x77)
	UTRAP(0x78)
	UTRAP(0x79)
	UTRAP(0x7a)
	UTRAP(0x7b)
	UTRAP(0x7c)
	UTRAP(0x7d)
	UTRAP(0x7e)
	UTRAP(0x7f)
	SYSCALL			! 80 = sun syscall
	BPT			! 81 = pseudo breakpoint instruction
	TRAP(T_DIV0)		! 82 = divide by zero
	TRAP(T_FLUSHWIN)	! 83 = flush windows
	TRAP(T_CLEANWIN)	! 84 = provide clean windows
	TRAP(T_RANGECHECK)	! 85 = ???
	TRAP(T_FIXALIGN)	! 86 = fix up unaligned accesses
	TRAP(T_INTOF)		! 87 = integer overflow
	SYSCALL			! 88 = svr4 syscall
	SYSCALL			! 89 = bsd syscall
	BPT_KGDB_EXEC		! 8a = enter kernel gdb on kernel startup
	STRAP(0x8b)
	STRAP(0x8c)
	STRAP(0x8d)
	STRAP(0x8e)
	STRAP(0x8f)
	STRAP(0x90)
	STRAP(0x91)
	STRAP(0x92)
	STRAP(0x93)
	STRAP(0x94)
	STRAP(0x95)
	STRAP(0x96)
	STRAP(0x97)
	STRAP(0x98)
	STRAP(0x99)
	STRAP(0x9a)
	STRAP(0x9b)
	STRAP(0x9c)
	STRAP(0x9d)
	STRAP(0x9e)
	STRAP(0x9f)
	STRAP(0xa0)
	STRAP(0xa1)
	STRAP(0xa2)
	STRAP(0xa3)
	STRAP(0xa4)
	STRAP(0xa5)
	STRAP(0xa6)
	STRAP(0xa7)
	STRAP(0xa8)
	STRAP(0xa9)
	STRAP(0xaa)
	STRAP(0xab)
	STRAP(0xac)
	STRAP(0xad)
	STRAP(0xae)
	STRAP(0xaf)
	STRAP(0xb0)
	STRAP(0xb1)
	STRAP(0xb2)
	STRAP(0xb3)
	STRAP(0xb4)
	STRAP(0xb5)
	STRAP(0xb6)
	STRAP(0xb7)
	STRAP(0xb8)
	STRAP(0xb9)
	STRAP(0xba)
	STRAP(0xbb)
	STRAP(0xbc)
	STRAP(0xbd)
	STRAP(0xbe)
	STRAP(0xbf)
	STRAP(0xc0)
	STRAP(0xc1)
	STRAP(0xc2)
	STRAP(0xc3)
	STRAP(0xc4)
	STRAP(0xc5)
	STRAP(0xc6)
	STRAP(0xc7)
	STRAP(0xc8)
	STRAP(0xc9)
	STRAP(0xca)
	STRAP(0xcb)
	STRAP(0xcc)
	STRAP(0xcd)
	STRAP(0xce)
	STRAP(0xcf)
	STRAP(0xd0)
	STRAP(0xd1)
	STRAP(0xd2)
	STRAP(0xd3)
	STRAP(0xd4)
	STRAP(0xd5)
	STRAP(0xd6)
	STRAP(0xd7)
	STRAP(0xd8)
	STRAP(0xd9)
	STRAP(0xda)
	STRAP(0xdb)
	STRAP(0xdc)
	STRAP(0xdd)
	STRAP(0xde)
	STRAP(0xdf)
	STRAP(0xe0)
	STRAP(0xe1)
	STRAP(0xe2)
	STRAP(0xe3)
	STRAP(0xe4)
	STRAP(0xe5)
	STRAP(0xe6)
	STRAP(0xe7)
	STRAP(0xe8)
	STRAP(0xe9)
	STRAP(0xea)
	STRAP(0xeb)
	STRAP(0xec)
	STRAP(0xed)
	STRAP(0xee)
	STRAP(0xef)
	STRAP(0xf0)
	STRAP(0xf1)
	STRAP(0xf2)
	STRAP(0xf3)
	STRAP(0xf4)
	STRAP(0xf5)
	STRAP(0xf6)
	STRAP(0xf7)
	STRAP(0xf8)
	STRAP(0xf9)
	STRAP(0xfa)
	STRAP(0xfb)
	STRAP(0xfc)
	STRAP(0xfd)
	STRAP(0xfe)
	STRAP(0xff)
#endif

#if defined(SUN4M)
trapbase_sun4m:
/* trap 0 is special since we cannot receive it */
	b dostart; nop; nop; nop	! 00 = reset (fake)
	VTRAP(T_TEXTFAULT, memfault_sun4m)	! 01 = instr. fetch fault
	VTRAP(T_ILLINST, illinst4m)	! 02 = illegal instruction
	TRAP(T_PRIVINST)		! 03 = privileged instruction
	TRAP(T_FPDISABLED)		! 04 = fp instr, but EF bit off in psr
	WINDOW_OF			! 05 = window overflow
	WINDOW_UF			! 06 = window underflow
	TRAP(T_ALIGN)			! 07 = address alignment error
	VTRAP(T_FPE, fp_exception)	! 08 = fp exception
	VTRAP(T_DATAFAULT, memfault_sun4m)	! 09 = data fetch fault
	TRAP(T_TAGOF)			! 0a = tag overflow
	UTRAP(0x0b)
	UTRAP(0x0c)
	UTRAP(0x0d)
	UTRAP(0x0e)
	UTRAP(0x0f)
	UTRAP(0x10)
	HARDINT4M(1)			! 11 = level 1 interrupt
	HARDINT4M(2)			! 12 = level 2 interrupt
	HARDINT4M(3)			! 13 = level 3 interrupt
	HARDINT4M(4)			! 14 = level 4 interrupt
	HARDINT4M(5)			! 15 = level 5 interrupt
	HARDINT4M(6)			! 16 = level 6 interrupt
	HARDINT4M(7)			! 17 = level 7 interrupt
	HARDINT4M(8)			! 18 = level 8 interrupt
	HARDINT4M(9)			! 19 = level 9 interrupt
	HARDINT4M(10)			! 1a = level 10 interrupt
	HARDINT4M(11)			! 1b = level 11 interrupt
	ZS_INTERRUPT4M			! 1c = level 12 (zs) interrupt
	HARDINT4M(13)			! 1d = level 13 interrupt
	HARDINT4M(14)			! 1e = level 14 interrupt
	VTRAP(15, nmi_sun4m)		! 1f = nonmaskable interrupt
	UTRAP(0x20)
	VTRAP(T_TEXTERROR, memfault_sun4m)	! 21 = instr. fetch error
	UTRAP(0x22)
	UTRAP(0x23)
	TRAP(T_CPDISABLED)	! 24 = coprocessor instr, EC bit off in psr
	UTRAP(0x25)
	UTRAP(0x26)
	UTRAP(0x27)
	TRAP(T_CPEXCEPTION)	! 28 = coprocessor exception
	VTRAP(T_DATAERROR, memfault_sun4m)	! 29 = data fetch error
	UTRAP(0x2a)
	VTRAP(T_STOREBUFFAULT, memfault_sun4m) ! 2b = SuperSPARC store buffer fault
	UTRAP(0x2c)
	UTRAP(0x2d)
	UTRAP(0x2e)
	UTRAP(0x2f)
	UTRAP(0x30)
	UTRAP(0x31)
	UTRAP(0x32)
	UTRAP(0x33)
	UTRAP(0x34)
	UTRAP(0x35)
	UTRAP(0x36)
	UTRAP(0x37)
	UTRAP(0x38)
	UTRAP(0x39)
	UTRAP(0x3a)
	UTRAP(0x3b)
	UTRAP(0x3c)
	UTRAP(0x3d)
	UTRAP(0x3e)
	UTRAP(0x3f)
	UTRAP(0x40)
	UTRAP(0x41)
	UTRAP(0x42)
	UTRAP(0x43)
	UTRAP(0x44)
	UTRAP(0x45)
	UTRAP(0x46)
	UTRAP(0x47)
	UTRAP(0x48)
	UTRAP(0x49)
	UTRAP(0x4a)
	UTRAP(0x4b)
	UTRAP(0x4c)
	UTRAP(0x4d)
	UTRAP(0x4e)
	UTRAP(0x4f)
	UTRAP(0x50)
	UTRAP(0x51)
	UTRAP(0x52)
	UTRAP(0x53)
	UTRAP(0x54)
	UTRAP(0x55)
	UTRAP(0x56)
	UTRAP(0x57)
	UTRAP(0x58)
	UTRAP(0x59)
	UTRAP(0x5a)
	UTRAP(0x5b)
	UTRAP(0x5c)
	UTRAP(0x5d)
	UTRAP(0x5e)
	UTRAP(0x5f)
	UTRAP(0x60)
	UTRAP(0x61)
	UTRAP(0x62)
	UTRAP(0x63)
	UTRAP(0x64)
	UTRAP(0x65)
	UTRAP(0x66)
	UTRAP(0x67)
	UTRAP(0x68)
	UTRAP(0x69)
	UTRAP(0x6a)
	UTRAP(0x6b)
	UTRAP(0x6c)
	UTRAP(0x6d)
	UTRAP(0x6e)
	UTRAP(0x6f)
	UTRAP(0x70)
	UTRAP(0x71)
	UTRAP(0x72)
	UTRAP(0x73)
	UTRAP(0x74)
	UTRAP(0x75)
	UTRAP(0x76)
	UTRAP(0x77)
	UTRAP(0x78)
	UTRAP(0x79)
	UTRAP(0x7a)
	UTRAP(0x7b)
	UTRAP(0x7c)
	UTRAP(0x7d)
	UTRAP(0x7e)
	UTRAP(0x7f)
	SYSCALL			! 80 = sun syscall
	BPT			! 81 = pseudo breakpoint instruction
	TRAP(T_DIV0)		! 82 = divide by zero
	TRAP(T_FLUSHWIN)	! 83 = flush windows
	TRAP(T_CLEANWIN)	! 84 = provide clean windows
	TRAP(T_RANGECHECK)	! 85 = ???
	TRAP(T_FIXALIGN)	! 86 = fix up unaligned accesses
	TRAP(T_INTOF)		! 87 = integer overflow
	SYSCALL			! 88 = svr4 syscall
	SYSCALL			! 89 = bsd syscall
	BPT_KGDB_EXEC		! 8a = enter kernel gdb on kernel startup
	TRAP(T_DBPAUSE)		! 8b = hold CPU for kernel debugger
	STRAP(0x8c)
	STRAP(0x8d)
	STRAP(0x8e)
	STRAP(0x8f)
	STRAP(0x90)
	STRAP(0x91)
	STRAP(0x92)
	STRAP(0x93)
	STRAP(0x94)
	STRAP(0x95)
	STRAP(0x96)
	STRAP(0x97)
	STRAP(0x98)
	STRAP(0x99)
	STRAP(0x9a)
	STRAP(0x9b)
	STRAP(0x9c)
	STRAP(0x9d)
	STRAP(0x9e)
	STRAP(0x9f)
	STRAP(0xa0)
	STRAP(0xa1)
	STRAP(0xa2)
	STRAP(0xa3)
	STRAP(0xa4)
	STRAP(0xa5)
	STRAP(0xa6)
	STRAP(0xa7)
	STRAP(0xa8)
	STRAP(0xa9)
	STRAP(0xaa)
	STRAP(0xab)
	STRAP(0xac)
	STRAP(0xad)
	STRAP(0xae)
	STRAP(0xaf)
	STRAP(0xb0)
	STRAP(0xb1)
	STRAP(0xb2)
	STRAP(0xb3)
	STRAP(0xb4)
	STRAP(0xb5)
	STRAP(0xb6)
	STRAP(0xb7)
	STRAP(0xb8)
	STRAP(0xb9)
	STRAP(0xba)
	STRAP(0xbb)
	STRAP(0xbc)
	STRAP(0xbd)
	STRAP(0xbe)
	STRAP(0xbf)
	STRAP(0xc0)
	STRAP(0xc1)
	STRAP(0xc2)
	STRAP(0xc3)
	STRAP(0xc4)
	STRAP(0xc5)
	STRAP(0xc6)
	STRAP(0xc7)
	STRAP(0xc8)
	STRAP(0xc9)
	STRAP(0xca)
	STRAP(0xcb)
	STRAP(0xcc)
	STRAP(0xcd)
	STRAP(0xce)
	STRAP(0xcf)
	STRAP(0xd0)
	STRAP(0xd1)
	STRAP(0xd2)
	STRAP(0xd3)
	STRAP(0xd4)
	STRAP(0xd5)
	STRAP(0xd6)
	STRAP(0xd7)
	STRAP(0xd8)
	STRAP(0xd9)
	STRAP(0xda)
	STRAP(0xdb)
	STRAP(0xdc)
	STRAP(0xdd)
	STRAP(0xde)
	STRAP(0xdf)
	STRAP(0xe0)
	STRAP(0xe1)
	STRAP(0xe2)
	STRAP(0xe3)
	STRAP(0xe4)
	STRAP(0xe5)
	STRAP(0xe6)
	STRAP(0xe7)
	STRAP(0xe8)
	STRAP(0xe9)
	STRAP(0xea)
	STRAP(0xeb)
	STRAP(0xec)
	STRAP(0xed)
	STRAP(0xee)
	STRAP(0xef)
	STRAP(0xf0)
	STRAP(0xf1)
	STRAP(0xf2)
	STRAP(0xf3)
	STRAP(0xf4)
	STRAP(0xf5)
	STRAP(0xf6)
	STRAP(0xf7)
	STRAP(0xf8)
	STRAP(0xf9)
	STRAP(0xfa)
	STRAP(0xfb)
	STRAP(0xfc)
	STRAP(0xfd)
	STRAP(0xfe)
	STRAP(0xff)
#endif

/*
 * Pad the trap table to max page size.
 * Trap table size is 0x100 * 4instr * 4byte/instr = 4096 bytes;
 * need to .skip 4096 to pad to page size iff. the number of trap tables
 * defined above is odd.
 */
#if (defined(SUN4) + defined(SUN4C) + defined(SUN4M)) % 2 == 1
	.skip	4096
#endif

/* redzones don't work currently in multi-processor mode */
#if defined(DEBUG) && !defined(MULTIPROCESSOR)
/*
 * A hardware red zone is impossible.  We simulate one in software by
 * keeping a `red zone' pointer; if %sp becomes less than this, we panic.
 * This is expensive and is only enabled when debugging.
 */

/* `redzone' is located in the per-CPU information structure */
_redzone = CPUINFO_VA + CPUINFO_REDZONE
	.data
#define	REDSTACK 2048		/* size of `panic: stack overflow' region */
_redstack:
	.skip	REDSTACK
	.text
Lpanic_red:
	.asciz	"stack overflow"
	_ALIGN

	/* set stack pointer redzone to base+minstack; alters base */
#define	SET_SP_REDZONE(base, tmp) \
	add	base, REDSIZE, base; \
	sethi	%hi(_redzone), tmp; \
	st	base, [tmp + %lo(_redzone)]

	/* variant with a constant */
#define	SET_SP_REDZONE_CONST(const, tmp1, tmp2) \
	set	(const) + REDSIZE, tmp1; \
	sethi	%hi(_redzone), tmp2; \
	st	tmp1, [tmp2 + %lo(_redzone)]

	/* variant with a variable & offset */
#define	SET_SP_REDZONE_VAR(var, offset, tmp1, tmp2) \
	sethi	%hi(var), tmp1; \
	ld	[tmp1 + %lo(var)], tmp1; \
	sethi	%hi(offset), tmp2; \
	add	tmp1, tmp2, tmp1; \
	SET_SP_REDZONE(tmp1, tmp2)

	/* check stack pointer against redzone (uses two temps) */
#define	CHECK_SP_REDZONE(t1, t2) \
	sethi	%hi(_redzone), t1; \
	ld	[t1 + %lo(_redzone)], t2; \
	cmp	%sp, t2;	/* if sp >= t2, not in red zone */ \
	bgeu	7f; nop;	/* and can continue normally */ \
	/* move to panic stack */ \
	st	%g0, [t1 + %lo(_redzone)]; \
	set	_redstack + REDSTACK - 96, %sp; \
	/* prevent panic() from lowering ipl */ \
	sethi	%hi(_C_LABEL(panicstr)), t1; \
	set	Lpanic_red, t2; \
	st	t2, [t1 + %lo(_C_LABEL(panicstr))]; \
	rd	%psr, t1;		/* t1 = splhigh() */ \
	or	t1, PSR_PIL, t2; \
	wr	t2, 0, %psr; \
	wr	t2, PSR_ET, %psr;	/* turn on traps */ \
	nop; nop; nop; \
	save	%sp, -CCFSZ, %sp;	/* preserve current window */ \
	sethi	%hi(Lpanic_red), %o0; \
	call	_C_LABEL(panic); or %o0, %lo(Lpanic_red), %o0; \
7:

#else

#define	SET_SP_REDZONE(base, tmp)
#define	SET_SP_REDZONE_CONST(const, t1, t2)
#define	SET_SP_REDZONE_VAR(var, offset, t1, t2)
#define	CHECK_SP_REDZONE(t1, t2)
#endif /* DEBUG */

/*
 * The window code must verify user stack addresses before using them.
 * A user stack pointer is invalid if:
 *	- it is not on an 8 byte boundary;
 *	- its pages (a register window, being 64 bytes, can occupy
 *	  two pages) are not readable or writable.
 * We define three separate macros here for testing user stack addresses.
 *
 * PTE_OF_ADDR locates a PTE, branching to a `bad address'
 *	handler if the stack pointer points into the hole in the
 *	address space (i.e., top 3 bits are not either all 1 or all 0);
 * CMP_PTE_USER_READ compares the located PTE against `user read' mode;
 * CMP_PTE_USER_WRITE compares the located PTE against `user write' mode.
 * The compares give `equal' if read or write is OK.
 *
 * Note that the user stack pointer usually points into high addresses
 * (top 3 bits all 1), so that is what we check first.
 *
 * The code below also assumes that PTE_OF_ADDR is safe in a delay
 * slot; it is, at it merely sets its `pte' register to a temporary value.
 */
#if defined(SUN4) || defined(SUN4C)
	/* input: addr, output: pte; aux: bad address label */
#define	PTE_OF_ADDR4_4C(addr, pte, bad, page_offset) \
	sra	addr, PG_VSHIFT, pte; \
	cmp	pte, -1; \
	be,a	1f; andn addr, page_offset, pte; \
	tst	pte; \
	bne	bad; .empty; \
	andn	addr, page_offset, pte; \
1:

	/* input: pte; output: condition codes */
#define	CMP_PTE_USER_READ4_4C(pte) \
	lda	[pte] ASI_PTE, pte; \
	srl	pte, PG_PROTSHIFT, pte; \
	andn	pte, (PG_W >> PG_PROTSHIFT), pte; \
	cmp	pte, PG_PROTUREAD

	/* input: pte; output: condition codes */
#define	CMP_PTE_USER_WRITE4_4C(pte) \
	lda	[pte] ASI_PTE, pte; \
	srl	pte, PG_PROTSHIFT, pte; \
	cmp	pte, PG_PROTUWRITE
#endif

/*
 * The Sun4M does not have the memory hole that the 4C does. Thus all
 * we need to do here is clear the page offset from addr.
 */
#if defined(SUN4M)
#define	PTE_OF_ADDR4M(addr, pte, bad, page_offset) \
	andn	addr, page_offset, pte

/*
 * After obtaining the PTE through ASI_SRMMUFP, we read the Sync Fault
 * Status register. This is necessary on Hypersparcs which stores and
 * locks the fault address and status registers if the translation
 * fails (thanks to Chris Torek for finding this quirk).
 */
#define CMP_PTE_USER_READ4M(pte, tmp) \
	/*or	pte, ASI_SRMMUFP_L3, pte; -- ASI_SRMMUFP_L3 == 0 */ \
	lda	[pte] ASI_SRMMUFP, pte; \
	set	SRMMU_SFSR, tmp; \
	lda	[tmp] ASI_SRMMU, %g0; \
	and	pte, SRMMU_TETYPE, tmp; \
	/* Check for valid pte */ \
	cmp	tmp, SRMMU_TEPTE; \
	bnz	8f; \
	and	pte, SRMMU_PROT_MASK, pte; \
	/* check for one of: R_R, RW_RW, RX_RX and RWX_RWX */ \
	cmp	pte, PPROT_X_X; \
	bcs,a	8f; \
	 /* Now we have carry set if OK; turn it into Z bit */ \
	 subxcc	%g0, -1, %g0; \
	/* One more case to check: R_RW */ \
	cmp	pte, PPROT_R_RW; \
8:


/* note: PTE bit 4 set implies no user writes */
#define CMP_PTE_USER_WRITE4M(pte, tmp) \
	or	pte, ASI_SRMMUFP_L3, pte; \
	lda	[pte] ASI_SRMMUFP, pte; \
	set	SRMMU_SFSR, tmp; \
	lda	[tmp] ASI_SRMMU, %g0; \
	and	pte, (SRMMU_TETYPE | 0x14), pte; \
	cmp	pte, (SRMMU_TEPTE | PPROT_WRITE)
#endif /* 4m */

#if defined(SUN4M) && !(defined(SUN4C) || defined(SUN4))

#define PTE_OF_ADDR(addr, pte, bad, page_offset, label) \
	PTE_OF_ADDR4M(addr, pte, bad, page_offset)
#define CMP_PTE_USER_WRITE(pte, tmp, label)	CMP_PTE_USER_WRITE4M(pte,tmp)
#define CMP_PTE_USER_READ(pte, tmp, label)	CMP_PTE_USER_READ4M(pte,tmp)

#elif (defined(SUN4C) || defined(SUN4)) && !defined(SUN4M)

#define PTE_OF_ADDR(addr, pte, bad, page_offset,label) \
	PTE_OF_ADDR4_4C(addr, pte, bad, page_offset)
#define CMP_PTE_USER_WRITE(pte, tmp, label)	CMP_PTE_USER_WRITE4_4C(pte)
#define CMP_PTE_USER_READ(pte, tmp, label)	CMP_PTE_USER_READ4_4C(pte)

#else /* both defined, ugh */

#define	PTE_OF_ADDR(addr, pte, bad, page_offset, label) \
label:	b,a	2f; \
	PTE_OF_ADDR4M(addr, pte, bad, page_offset); \
	b,a	3f; \
2: \
	PTE_OF_ADDR4_4C(addr, pte, bad, page_offset); \
3:

#define CMP_PTE_USER_READ(pte, tmp, label) \
label:	b,a	1f; \
	CMP_PTE_USER_READ4M(pte,tmp); \
	b,a	2f; \
1: \
	CMP_PTE_USER_READ4_4C(pte); \
2:

#define CMP_PTE_USER_WRITE(pte, tmp, label) \
label:	b,a	1f; \
	CMP_PTE_USER_WRITE4M(pte,tmp); \
	b,a	2f; \
1: \
	CMP_PTE_USER_WRITE4_4C(pte); \
2:
#endif


/*
 * The calculations in PTE_OF_ADDR and CMP_PTE_USER_* are rather slow:
 * in particular, according to Gordon Irlam of the University of Adelaide
 * in Australia, these consume at least 18 cycles on an SS1 and 37 on an
 * SS2.  Hence, we try to avoid them in the common case.
 *
 * A chunk of 64 bytes is on a single page if and only if:
 *
 *	((base + 64 - 1) & ~(NBPG-1)) == (base & ~(NBPG-1))
 *
 * Equivalently (and faster to test), the low order bits (base & 4095) must
 * be small enough so that the sum (base + 63) does not carry out into the
 * upper page-address bits, i.e.,
 *
 *	(base & (NBPG-1)) < (NBPG - 63)
 *
 * so we allow testing that here.  This macro is also assumed to be safe
 * in a delay slot (modulo overwriting its temporary).
 */
#define	SLT_IF_1PAGE_RW(addr, tmp, page_offset) \
	and	addr, page_offset, tmp; \
	sub	page_offset, 62, page_offset; \
	cmp	tmp, page_offset

/*
 * Every trap that enables traps must set up stack space.
 * If the trap is from user mode, this involves switching to the kernel
 * stack for the current process, and we must also set cpcb->pcb_uw
 * so that the window overflow handler can tell user windows from kernel
 * windows.
 *
 * The number of user windows is:
 *
 *	cpcb->pcb_uw = (cpcb->pcb_wim - 1 - CWP) % nwindows
 *
 * (where pcb_wim = log2(current %wim) and CWP = low 5 bits of %psr).
 * We compute this expression by table lookup in uwtab[CWP - pcb_wim],
 * which has been set up as:
 *
 *	for i in [-nwin+1 .. nwin-1]
 *		uwtab[i] = (nwin - 1 - i) % nwin;
 *
 * (If you do not believe this works, try it for yourself.)
 *
 * We also keep one or two more tables:
 *
 *	for i in 0..nwin-1
 *		wmask[i] = 1 << ((i + 1) % nwindows);
 *
 * wmask[CWP] tells whether a `rett' would return into the invalid window.
 */
	.data
	.skip	32			! alignment byte & negative indicies
uwtab:	.skip	32			! u_char uwtab[-31..31];
wmask:	.skip	32			! u_char wmask[0..31];

	.text
/*
 * Things begin to grow uglier....
 *
 * Each trap handler may (always) be running in the trap window.
 * If this is the case, it cannot enable further traps until it writes
 * the register windows into the stack (or, if the stack is no good,
 * the current pcb).
 *
 * ASSUMPTIONS: TRAP_SETUP() is called with:
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l3 = (some value that must not be altered)
 * which means we have 4 registers to work with.
 *
 * The `stackspace' argument is the number of stack bytes to allocate
 * for register-saving, and must be at least -64 (and typically more,
 * for global registers and %y).
 *
 * Trapframes should use -CCFSZ-80.  (80 = sizeof(struct trapframe);
 * see trap.h.  This basically means EVERYONE.  Interrupt frames could
 * get away with less, but currently do not.)
 *
 * The basic outline here is:
 *
 *	if (trap came from kernel mode) {
 *		if (we are in the trap window)
 *			save it away;
 *		%sp = %fp - stackspace;
 *	} else {
 *		compute the number of user windows;
 *		if (we are in the trap window)
 *			save it away;
 *		%sp = (top of kernel stack) - stackspace;
 *	}
 *
 * Again, the number of user windows is:
 *
 *	cpcb->pcb_uw = (cpcb->pcb_wim - 1 - CWP) % nwindows
 *
 * (where pcb_wim = log2(current %wim) and CWP is the low 5 bits of %psr),
 * and this is computed as `uwtab[CWP - pcb_wim]'.
 *
 * NOTE: if you change this code, you will have to look carefully
 * at the window overflow and underflow handlers and make sure they
 * have similar changes made as needed.
 */
#define	CALL_CLEAN_TRAP_WINDOW \
	sethi	%hi(clean_trap_window), %l7; \
	jmpl	%l7 + %lo(clean_trap_window), %l4; \
	 mov	%g7, %l7	/* save %g7 in %l7 for clean_trap_window */

#define	TRAP_SETUP(stackspace) \
	TRAP_TRACE(%l3,%l5); \
	rd	%wim, %l4; \
	mov	1, %l5; \
	sll	%l5, %l0, %l5; \
	btst	PSR_PS, %l0; \
	bz	1f; \
	 btst	%l5, %l4; \
	/* came from kernel mode; cond codes indicate trap window */ \
	bz,a	3f; \
	 add	%fp, stackspace, %sp;	/* want to just set %sp */ \
	CALL_CLEAN_TRAP_WINDOW;		/* but maybe need to clean first */ \
	b	3f; \
	 add	%fp, stackspace, %sp; \
1: \
	/* came from user mode: compute pcb_nw */ \
	sethi	%hi(cpcb), %l6; \
	ld	[%l6 + %lo(cpcb)], %l6; \
	ld	[%l6 + PCB_WIM], %l5; \
	and	%l0, 31, %l4; \
	sub	%l4, %l5, %l5; \
	set	uwtab, %l4; \
	ldub	[%l4 + %l5], %l5; \
	st	%l5, [%l6 + PCB_UW]; \
	/* cond codes still indicate whether in trap window */ \
	bz,a	2f; \
	 sethi	%hi(USPACE+(stackspace)), %l5; \
	/* yes, in trap window; must clean it */ \
	CALL_CLEAN_TRAP_WINDOW; \
	sethi	%hi(cpcb), %l6; \
	ld	[%l6 + %lo(cpcb)], %l6; \
	sethi	%hi(USPACE+(stackspace)), %l5; \
2: \
	/* trap window is (now) clean: set %sp */ \
	or	%l5, %lo(USPACE+(stackspace)), %l5; \
	add	%l6, %l5, %sp; \
	SET_SP_REDZONE(%l6, %l5); \
3: \
	CHECK_SP_REDZONE(%l6, %l5)

/*
 * Interrupt setup is almost exactly like trap setup, but we need to
 * go to the interrupt stack if (a) we came from user mode or (b) we
 * came from kernel mode on the kernel stack.
 */
#if defined(MULTIPROCESSOR)
/*
 * SMP kernels: read `eintstack' from cpuinfo structure. Since the
 * location of the interrupt stack is not known in advance, we need
 * to check the current %fp against both ends of the stack space.
 */
#define	INTR_SETUP(stackspace) \
	TRAP_TRACE(%l3,%l5); \
	rd	%wim, %l4; \
	mov	1, %l5; \
	sll	%l5, %l0, %l5; \
	btst	PSR_PS, %l0; \
	bz	1f; \
	 btst	%l5, %l4; \
	/* came from kernel mode; cond codes still indicate trap window */ \
	bz,a	0f; \
	 sethi	%hi(_EINTSTACKP), %l7; \
	CALL_CLEAN_TRAP_WINDOW; \
	sethi	%hi(_EINTSTACKP), %l7; \
0:	/* now if not intstack > %fp >= eintstack, we were on the kernel stack */ \
	ld	[%l7 + %lo(_EINTSTACKP)], %l7; \
	cmp	%fp, %l7; \
	bge,a	3f;			/* %fp >= eintstack */ \
	 add	%l7, stackspace, %sp;	/* so switch to intstack */ \
	sethi	%hi(INT_STACK_SIZE), %l6; \
	sub	%l7, %l6, %l6; \
	cmp	%fp, %l6; \
	blu,a	3f;			/* %fp < intstack */ \
	 add	%l7, stackspace, %sp;	/* so switch to intstack */ \
	b	4f; \
	 add	%fp, stackspace, %sp;	/* else stay on intstack */ \
1: \
	/* came from user mode: compute pcb_nw */ \
	sethi	%hi(cpcb), %l6; \
	ld	[%l6 + %lo(cpcb)], %l6; \
	ld	[%l6 + PCB_WIM], %l5; \
	and	%l0, 31, %l4; \
	sub	%l4, %l5, %l5; \
	set	uwtab, %l4; \
	ldub	[%l4 + %l5], %l5; \
	st	%l5, [%l6 + PCB_UW]; \
	/* cond codes still indicate whether in trap window */ \
	bz,a	2f; \
	 sethi	%hi(_EINTSTACKP), %l7; \
	/* yes, in trap window; must save regs */ \
	CALL_CLEAN_TRAP_WINDOW; \
	sethi	%hi(_EINTSTACKP), %l7; \
2: \
	ld	[%l7 + %lo(_EINTSTACKP)], %l7; \
	add	%l7, stackspace, %sp; \
3: \
	SET_SP_REDZONE_VAR(_EINTSTACKP, -INT_STACK_SIZE, %l6, %l5); \
4: \
	CHECK_SP_REDZONE(%l6, %l5)

#else /* MULTIPROCESSOR */

#define	INTR_SETUP(stackspace) \
	TRAP_TRACE(%l3,%l5); \
	rd	%wim, %l4; \
	mov	1, %l5; \
	sll	%l5, %l0, %l5; \
	btst	PSR_PS, %l0; \
	bz	1f; \
	 btst	%l5, %l4; \
	/* came from kernel mode; cond codes still indicate trap window */ \
	bz,a	0f; \
	 sethi	%hi(_C_LABEL(eintstack)), %l7; \
	CALL_CLEAN_TRAP_WINDOW; \
	sethi	%hi(_C_LABEL(eintstack)), %l7; \
0:	/* now if %fp >= eintstack, we were on the kernel stack */ \
	cmp	%fp, %l7; \
	bge,a	3f; \
	 add	%l7, stackspace, %sp;	/* so switch to intstack */ \
	b	4f; \
	 add	%fp, stackspace, %sp;	/* else stay on intstack */ \
1: \
	/* came from user mode: compute pcb_nw */ \
	sethi	%hi(cpcb), %l6; \
	ld	[%l6 + %lo(cpcb)], %l6; \
	ld	[%l6 + PCB_WIM], %l5; \
	and	%l0, 31, %l4; \
	sub	%l4, %l5, %l5; \
	set	uwtab, %l4; \
	ldub	[%l4 + %l5], %l5; \
	st	%l5, [%l6 + PCB_UW]; \
	/* cond codes still indicate whether in trap window */ \
	bz,a	2f; \
	 sethi	%hi(_C_LABEL(eintstack)), %l7; \
	/* yes, in trap window; must save regs */ \
	CALL_CLEAN_TRAP_WINDOW; \
	sethi	%hi(_C_LABEL(eintstack)), %l7; \
2: \
	add	%l7, stackspace, %sp; \
3: \
	SET_SP_REDZONE_CONST(_C_LABEL(intstack), %l6, %l5); \
4: \
	CHECK_SP_REDZONE(%l6, %l5)
#endif /* MULTIPROCESSOR */

/*
 * Handler for making the trap window shiny clean.
 *
 * On entry:
 *	cpcb->pcb_nw = number of user windows
 *	%l0 = %psr
 *	%l1 must not be clobbered
 *	%l2 must not be clobbered
 *	%l3 must not be clobbered
 *	%l4 = address for `return'
 *	%l7 = saved %g7 (we put this in a delay slot above, to save work)
 *
 * On return:
 *	%wim has changed, along with cpcb->pcb_wim
 *	%g7 has been restored
 *
 * Normally, we push only one window.
 */
clean_trap_window:
	mov	%g5, %l5		! save %g5
	mov	%g6, %l6		! ... and %g6
/*	mov	%g7, %l7		! ... and %g7 (already done for us) */
	sethi	%hi(cpcb), %g6		! get current pcb
	ld	[%g6 + %lo(cpcb)], %g6

	/* Figure out whether it is a user window (cpcb->pcb_uw > 0). */
	ld	[%g6 + PCB_UW], %g7
	deccc	%g7
	bge	ctw_user
	 save	%g0, %g0, %g0		! in any case, enter window to save

	/* The window to be pushed is a kernel window. */
	std	%l0, [%sp + (0*8)]
ctw_merge:
	std	%l2, [%sp + (1*8)]
	std	%l4, [%sp + (2*8)]
	std	%l6, [%sp + (3*8)]
	std	%i0, [%sp + (4*8)]
	std	%i2, [%sp + (5*8)]
	std	%i4, [%sp + (6*8)]
	std	%i6, [%sp + (7*8)]

	/* Set up new window invalid mask, and update cpcb->pcb_wim. */
	rd	%psr, %g7		! g7 = (junk << 5) + new_cwp
	mov	1, %g5			! g5 = 1 << new_cwp;
	sll	%g5, %g7, %g5
	wr	%g5, 0, %wim		! setwim(g5);
	and	%g7, 31, %g7		! cpcb->pcb_wim = g7 & 31;
	sethi	%hi(cpcb), %g6		! re-get current pcb
	ld	[%g6 + %lo(cpcb)], %g6
	st	%g7, [%g6 + PCB_WIM]
	nop
	restore				! back to trap window

	mov	%l5, %g5		! restore g5
	mov	%l6, %g6		! ... and g6
	jmp	%l4 + 8			! return to caller
	 mov	%l7, %g7		! ... and g7
	/* NOTREACHED */

ctw_user:
	/*
	 * The window to be pushed is a user window.
	 * We must verify the stack pointer (alignment & permissions).
	 * See comments above definition of PTE_OF_ADDR.
	 */
	st	%g7, [%g6 + PCB_UW]	! cpcb->pcb_uw--;
	btst	7, %sp			! if not aligned,
	bne	ctw_invalid		! choke on it
	 .empty

	sethi	%hi(_C_LABEL(pgofset)), %g6	! trash %g6=curpcb
	ld	[%g6 + %lo(_C_LABEL(pgofset))], %g6
	PTE_OF_ADDR(%sp, %g7, ctw_invalid, %g6, NOP_ON_4M_1)
	CMP_PTE_USER_WRITE(%g7, %g5, NOP_ON_4M_2) ! likewise if not writable
	bne	ctw_invalid
	 .empty
	/* Note side-effect of SLT_IF_1PAGE_RW: decrements %g6 by 62 */
	SLT_IF_1PAGE_RW(%sp, %g7, %g6)
	bl,a	ctw_merge		! all ok if only 1
	 std	%l0, [%sp]
	add	%sp, 7*8, %g5		! check last addr too
	add	%g6, 62, %g6		/* restore %g6 to `pgofset' */
	PTE_OF_ADDR(%g5, %g7, ctw_invalid, %g6, NOP_ON_4M_3)
	CMP_PTE_USER_WRITE(%g7, %g6, NOP_ON_4M_4)
	be,a	ctw_merge		! all ok: store <l0,l1> and merge
	 std	%l0, [%sp]

	/*
	 * The window we wanted to push could not be pushed.
	 * Instead, save ALL user windows into the pcb.
	 * We will notice later that we did this, when we
	 * get ready to return from our trap or syscall.
	 *
	 * The code here is run rarely and need not be optimal.
	 */
ctw_invalid:
	/*
	 * Reread cpcb->pcb_uw.  We decremented this earlier,
	 * so it is off by one.
	 */
	sethi	%hi(cpcb), %g6		! re-get current pcb
	ld	[%g6 + %lo(cpcb)], %g6

	ld	[%g6 + PCB_UW], %g7	! (number of user windows) - 1
	add	%g6, PCB_RW, %g5

	/* save g7+1 windows, starting with the current one */
1:					! do {
	std	%l0, [%g5 + (0*8)]	!	rw->rw_local[0] = l0;
	std	%l2, [%g5 + (1*8)]	!	...
	std	%l4, [%g5 + (2*8)]
	std	%l6, [%g5 + (3*8)]
	std	%i0, [%g5 + (4*8)]
	std	%i2, [%g5 + (5*8)]
	std	%i4, [%g5 + (6*8)]
	std	%i6, [%g5 + (7*8)]
	deccc	%g7			!	if (n > 0) save(), rw++;
	bge,a	1b			! } while (--n >= 0);
	 save	%g5, 64, %g5

	/* stash sp for bottommost window */
	st	%sp, [%g5 + 64 + (7*8)]

	/* set up new wim */
	rd	%psr, %g7		! g7 = (junk << 5) + new_cwp;
	mov	1, %g5			! g5 = 1 << new_cwp;
	sll	%g5, %g7, %g5
	wr	%g5, 0, %wim		! wim = g5;
	and	%g7, 31, %g7
	st	%g7, [%g6 + PCB_WIM]	! cpcb->pcb_wim = new_cwp;

	/* fix up pcb fields */
	ld	[%g6 + PCB_UW], %g7	! n = cpcb->pcb_uw;
	add	%g7, 1, %g5
	st	%g5, [%g6 + PCB_NSAVED]	! cpcb->pcb_nsaved = n + 1;
	st	%g0, [%g6 + PCB_UW]	! cpcb->pcb_uw = 0;

	/* return to trap window */
1:	deccc	%g7			! do {
	bge	1b			!	restore();
	 restore			! } while (--n >= 0);

	mov	%l5, %g5		! restore g5, g6, & g7, and return
	mov	%l6, %g6
	jmp	%l4 + 8
	 mov	%l7, %g7
	/* NOTREACHED */


/*
 * Each memory access (text or data) fault, from user or kernel mode,
 * comes here.  We read the error register and figure out what has
 * happened.
 *
 * This cannot be done from C code since we must not enable traps (and
 * hence may not use the `save' instruction) until we have decided that
 * the error is or is not an asynchronous one that showed up after a
 * synchronous error, but which must be handled before the sync err.
 *
 * Most memory faults are user mode text or data faults, which can cause
 * signal delivery or ptracing, for which we must build a full trapframe.
 * It does not seem worthwhile to work to avoid this in the other cases,
 * so we store all the %g registers on the stack immediately.
 *
 * On entry:
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l3 = T_TEXTFAULT or T_DATAFAULT
 *
 * Internal:
 *	%l4 = %y, until we call mem_access_fault (then onto trapframe)
 *	%l5 = IE_reg_addr, if async mem error
 *
 */

#if defined(SUN4)
_ENTRY(memfault_sun4)
memfault_sun4:
	TRAP_SETUP(-CCFSZ-80)
	! tally interrupt (curcpu()->cpu_data.cpu_nfault++) (clobbers %o0,%o1)
	INCR64(CPUINFO_VA + CPUINFO_NFAULT)

	st	%g1, [%sp + CCFSZ + 20]	! save g1
	rd	%y, %l4			! save y

	/*
	 * registers:
	 * memerr.ctrl	= memory error control reg., error if 0x80 set
	 * memerr.vaddr	= address of memory error
	 * buserr	= basically just like sun4c sync error reg but
	 *		  no SER_WRITE bit (have to figure out from code).
	 */
	set	_C_LABEL(par_err_reg), %o0 ! memerr ctrl addr -- XXX mapped?
	ld	[%o0], %o0		! get it
	std	%g2, [%sp + CCFSZ + 24]	! save g2, g3
	ld	[%o0], %o1		! memerr ctrl register
	inc	4, %o0			! now VA of memerr vaddr register
	std	%g4, [%sp + CCFSZ + 32]	! (sneak g4,g5 in here)
	ld	[%o0], %o2		! memerr virt addr
	st	%g0, [%o0]		! NOTE: this clears latching!!!
	btst	ME_REG_IERR, %o1	! memory error?
					! XXX this value may not be correct
					! as I got some parity errors and the
					! correct bits were not on?
	std	%g6, [%sp + CCFSZ + 40]
	bz,a	0f			! no, just a regular fault
	 wr	%l0, PSR_ET, %psr	! (and reenable traps)

	/* memory error = death for now XXX */
	clr	%o3
	clr	%o4
	call	_C_LABEL(memerr4_4c)	! memerr(0, ser, sva, 0, 0)
	 clr	%o0
	call	_C_LABEL(prom_halt)
	 nop

0:
	/*
	 * have to make SUN4 emulate SUN4C.   4C code expects
	 * SER in %o1 and the offending VA in %o2, everything else is ok.
	 * (must figure out if SER_WRITE should be set)
	 */
	set	AC_BUS_ERR, %o0		! bus error register
	cmp	%l3, T_TEXTFAULT	! text fault always on PC
	be	normal_mem_fault	! go
	 lduba	[%o0] ASI_CONTROL, %o1	! get its value

#define STORE_BIT 21 /* bit that indicates a store instruction for sparc */
	ld	[%l1], %o3		! offending instruction in %o3 [l1=pc]
	srl	%o3, STORE_BIT, %o3	! get load/store bit (wont fit simm13)
	btst	1, %o3			! test for store operation

	bz	normal_mem_fault	! if (z) is a load (so branch)
	 sethi	%hi(SER_WRITE), %o5     ! damn SER_WRITE wont fit simm13
!	or	%lo(SER_WRITE), %o5, %o5! not necessary since %lo is zero
	or	%o5, %o1, %o1		! set SER_WRITE
#if defined(SUN4C) || defined(SUN4M)
	ba,a	normal_mem_fault
	 !!nop				! XXX make efficient later
#endif /* SUN4C || SUN4M */
#endif /* SUN4 */

#if defined(SUN4C)
_ENTRY(memfault_sun4c)
memfault_sun4c:
	TRAP_SETUP(-CCFSZ-80)
	! tally fault (curcpu()->cpu_data.cpu_nfault++) (clobbers %o0,%o1,%o2)
	INCR64(CPUINFO_VA + CPUINFO_NFAULT)

	st	%g1, [%sp + CCFSZ + 20]	! save g1
	rd	%y, %l4			! save y

	/*
	 * We know about the layout of the error registers here.
	 *	addr	reg
	 *	----	---
	 *	a	AC_SYNC_ERR
	 *	a+4	AC_SYNC_VA
	 *	a+8	AC_ASYNC_ERR
	 *	a+12	AC_ASYNC_VA
	 */

#if AC_SYNC_ERR + 4 != AC_SYNC_VA || \
    AC_SYNC_ERR + 8 != AC_ASYNC_ERR || AC_SYNC_ERR + 12 != AC_ASYNC_VA
	help help help		! I, I, I wanna be a lifeguard
#endif
	set	AC_SYNC_ERR, %o0
	std	%g2, [%sp + CCFSZ + 24]	! save g2, g3
	lda	[%o0] ASI_CONTROL, %o1	! sync err reg
	inc	4, %o0
	std	%g4, [%sp + CCFSZ + 32]	! (sneak g4,g5 in here)
	lda	[%o0] ASI_CONTROL, %o2	! sync virt addr
	btst	SER_MEMERR, %o1		! memory error?
	std	%g6, [%sp + CCFSZ + 40]
	bz,a	normal_mem_fault	! no, just a regular fault
 	 wr	%l0, PSR_ET, %psr	! (and reenable traps)

	/*
	 * We got a synchronous memory error.  It could be one that
	 * happened because there were two stores in a row, and the
	 * first went into the write buffer, and the second caused this
	 * synchronous trap; so there could now be a pending async error.
	 * This is in fact the case iff the two va's differ.
	 */
	inc	4, %o0
	lda	[%o0] ASI_CONTROL, %o3	! async err reg
	inc	4, %o0
	lda	[%o0] ASI_CONTROL, %o4	! async virt addr
	cmp	%o2, %o4
	be,a	1f			! no, not an async err
	 wr	%l0, PSR_ET, %psr	! (and reenable traps)

	/*
	 * Handle the async error; ignore the sync error for now
	 * (we may end up getting it again, but so what?).
	 * This code is essentially the same as that at `nmi' below,
	 * but the register usage is different and we cannot merge.
	 */
	sethi	%hi(INTRREG_VA), %l5	! ienab_bic(IE_ALLIE);
	ldub	[%l5 + %lo(INTRREG_VA)], %o0
	andn	%o0, IE_ALLIE, %o0
	stb	%o0, [%l5 + %lo(INTRREG_VA)]

	/*
	 * Now reenable traps and call C code.
	 * %o1 through %o4 still hold the error reg contents.
	 * If memerr() returns, return from the trap.
	 */
	wr	%l0, PSR_ET, %psr
	call	_C_LABEL(memerr4_4c)	! memerr(0, ser, sva, aer, ava)
	 clr	%o0

	ld	[%sp + CCFSZ + 20], %g1	! restore g1 through g7
	wr	%l0, 0, %psr		! and disable traps, 3 instr delay
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	/* now safe to set IE_ALLIE again */
	ldub	[%l5 + %lo(INTRREG_VA)], %o1
	or	%o1, IE_ALLIE, %o1
	stb	%o1, [%l5 + %lo(INTRREG_VA)]
	b	return_from_trap
	 wr	%l4, 0, %y		! restore y

	/*
	 * Trap was a synchronous memory error.
	 * %o1 through %o4 still hold the error reg contents.
	 */
1:
	call	_C_LABEL(memerr4_4c)	! memerr(1, ser, sva, aer, ava)
	 mov	1, %o0

	ld	[%sp + CCFSZ + 20], %g1	! restore g1 through g7
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	wr	%l4, 0, %y		! restore y
	b	return_from_trap
	 wr	%l0, 0, %psr
	/* NOTREACHED */
#endif /* SUN4C */

#if defined(SUN4M)
_ENTRY(memfault_sun4m)
memfault_sun4m:
	sethi	%hi(CPUINFO_VA), %l4
	ld	[%l4 + %lo(CPUINFO_VA+CPUINFO_GETSYNCFLT)], %l5
	jmpl	%l5, %l7
	 or	%l4, %lo(CPUINFO_SYNCFLTDUMP), %l4
	TRAP_SETUP(-CCFSZ-80)
	! tally fault (curcpu()->cpu_data.cpu_nfault++) (clobbers %o0,%o1,%o2)
	INCR64(CPUINFO_VA + CPUINFO_NFAULT)

	st	%g1, [%sp + CCFSZ + 20]	! save g1
	rd	%y, %l4			! save y

	std	%g2, [%sp + CCFSZ + 24]	! save g2, g3
	std	%g4, [%sp + CCFSZ + 32]	! save g4, g5
	std	%g6, [%sp + CCFSZ + 40]	! sneak in g6, g7

	! retrieve sync fault status/address
	sethi	%hi(CPUINFO_VA+CPUINFO_SYNCFLTDUMP), %o0
	ld	[%o0 + %lo(CPUINFO_VA+CPUINFO_SYNCFLTDUMP)], %o1
	ld	[%o0 + %lo(CPUINFO_VA+CPUINFO_SYNCFLTDUMP+4)], %o2

	wr	%l0, PSR_ET, %psr	! reenable traps

	/* Finish stackframe, call C trap handler */
	std	%l0, [%sp + CCFSZ + 0]	! set tf.tf_psr, tf.tf_pc
	mov	%l3, %o0		! (argument: type)
	st	%l2, [%sp + CCFSZ + 8]	! set tf.tf_npc
	st	%l4, [%sp + CCFSZ + 12]	! set tf.tf_y
	std	%i0, [%sp + CCFSZ + 48]	! tf.tf_out[0], etc
	std	%i2, [%sp + CCFSZ + 56]
	std	%i4, [%sp + CCFSZ + 64]
	std	%i6, [%sp + CCFSZ + 72]
					! mem_access_fault(type,sfsr,sfva,&tf);
	call	_C_LABEL(mem_access_fault4m)
	 add	%sp, CCFSZ, %o3		! (argument: &tf)

	ldd	[%sp + CCFSZ + 0], %l0	! load new values
	ldd	[%sp + CCFSZ + 8], %l2
	wr	%l3, 0, %y
	ld	[%sp + CCFSZ + 20], %g1
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	ldd	[%sp + CCFSZ + 48], %i0
	ldd	[%sp + CCFSZ + 56], %i2
	ldd	[%sp + CCFSZ + 64], %i4
	ldd	[%sp + CCFSZ + 72], %i6

	b	return_from_trap	! go return
	 wr	%l0, 0, %psr		! (but first disable traps again)
#endif /* SUN4M */

normal_mem_fault:
	/*
	 * Trap was some other error; call C code to deal with it.
	 * Must finish trap frame (psr,pc,npc,%y,%o0..%o7) in case
	 * we decide to deliver a signal or ptrace the process.
	 * %g1..%g7 were already set up above.
	 */
	std	%l0, [%sp + CCFSZ + 0]	! set tf.tf_psr, tf.tf_pc
	mov	%l3, %o0		! (argument: type)
	st	%l2, [%sp + CCFSZ + 8]	! set tf.tf_npc
	st	%l4, [%sp + CCFSZ + 12]	! set tf.tf_y
	mov	%l1, %o3		! (argument: pc)
	std	%i0, [%sp + CCFSZ + 48]	! tf.tf_out[0], etc
	std	%i2, [%sp + CCFSZ + 56]
	mov	%l0, %o4		! (argument: psr)
	std	%i4, [%sp + CCFSZ + 64]
	std	%i6, [%sp + CCFSZ + 72]
	call	_C_LABEL(mem_access_fault)! mem_access_fault(type, ser, sva,
					!		pc, psr, &tf);
	 add	%sp, CCFSZ, %o5		! (argument: &tf)

	ldd	[%sp + CCFSZ + 0], %l0	! load new values
	ldd	[%sp + CCFSZ + 8], %l2
	wr	%l3, 0, %y
	ld	[%sp + CCFSZ + 20], %g1
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	ldd	[%sp + CCFSZ + 48], %i0
	ldd	[%sp + CCFSZ + 56], %i2
	ldd	[%sp + CCFSZ + 64], %i4
	ldd	[%sp + CCFSZ + 72], %i6

	b	return_from_trap	! go return
	 wr	%l0, 0, %psr		! (but first disable traps again)

illinst4m:
	/*
	 * Cypress CPUs like to generate an Illegal Instruction trap
	 * for FLUSH instructions. Since we turn FLUSHes into no-ops
	 * (see also trap.c/emul.c), we check for this case here in
	 * the trap window, saving the overhead of a slow trap.
	 *
	 * We have to be careful not to incur a trap while probing
	 * for the instruction in user space. Use the Inhibit Fault
	 * bit in the PCR register to prevent that.
	 */

	btst	PSR_PS, %l0		! slowtrap() if from kernel
	bnz	slowtrap
	 .empty

	! clear fault status
	set	SRMMU_SFSR, %l7
	lda	[%l7]ASI_SRMMU, %g0

	! turn on the fault inhibit in PCR
	!set	SRMMU_PCR, reg			- SRMMU_PCR == 0, so use %g0
	lda	[%g0]ASI_SRMMU, %l4
	or	%l4, SRMMU_PCR_NF, %l5
	sta	%l5, [%g0]ASI_SRMMU

	! load the insn word as if user insn fetch
	lda	[%l1]ASI_USERI, %l5

	sta	%l4, [%g0]ASI_SRMMU		! restore PCR

	! check fault status; if we have a fault, take a regular trap
	set	SRMMU_SFAR, %l6
	lda	[%l6]ASI_SRMMU, %g0		! fault VA; must be read first
	lda	[%l7]ASI_SRMMU, %l6		! fault status
	andcc	%l6, SFSR_FAV, %l6		! get fault status bits
	bnz	slowtrap
	 .empty

	! we got the insn; check whether it was a FLUSH
	! instruction format: op=2, op3=0x3b (see also instr.h)
	set	((3 << 30) | (0x3f << 19)), %l7	! extract op & op3 fields
	and	%l5, %l7, %l6
	set	((2 << 30) | (0x3b << 19)), %l7	! any FLUSH opcode
	cmp	%l6, %l7
	bne	slowtrap
	 nop

	mov	%l2, %l1			! ADVANCE <pc,npc>
	mov	%l0, %psr			! and return from trap
	 add	%l2, 4, %l2
	RETT


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
	set	special_fp_store, %l4	! see if we came from the special one
	cmp	%l1, %l4		! pc == special_fp_store?
	bne	slowtrap		! no, go handle per usual
	 .empty
	sethi	%hi(savefpcont), %l4	! yes, "return" to the special code
	or	%lo(savefpcont), %l4, %l4
	jmp	%l4
	 rett	%l4 + 4

/*
 * slowtrap() builds a trap frame and calls trap().
 * This is called `slowtrap' because it *is*....
 * We have to build a full frame for ptrace(), for instance.
 *
 * Registers:
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l3 = trap code
 */
slowtrap:
	TRAP_SETUP(-CCFSZ-80)
	/*
	 * Phew, ready to enable traps and call C code.
	 */
	mov	%l3, %o0		! put type in %o0 for later
Lslowtrap_reenter:
	wr	%l0, PSR_ET, %psr	! traps on again
	std	%l0, [%sp + CCFSZ]	! tf.tf_psr = psr; tf.tf_pc = ret_pc;
	rd	%y, %l3
	std	%l2, [%sp + CCFSZ + 8]	! tf.tf_npc = return_npc; tf.tf_y = %y;
	st	%g1, [%sp + CCFSZ + 20]
	std	%g2, [%sp + CCFSZ + 24]
	std	%g4, [%sp + CCFSZ + 32]
	std	%g6, [%sp + CCFSZ + 40]
	std	%i0, [%sp + CCFSZ + 48]
	mov	%l0, %o1		! (psr)
	std	%i2, [%sp + CCFSZ + 56]
	mov	%l1, %o2		! (pc)
	std	%i4, [%sp + CCFSZ + 64]
	add	%sp, CCFSZ, %o3		! (&tf)
	call	_C_LABEL(trap)		! trap(type, psr, pc, &tf)
	 std	%i6, [%sp + CCFSZ + 72]

	ldd	[%sp + CCFSZ], %l0	! load new values
	ldd	[%sp + CCFSZ + 8], %l2
	wr	%l3, 0, %y
	ld	[%sp + CCFSZ + 20], %g1
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	ldd	[%sp + CCFSZ + 48], %i0
	ldd	[%sp + CCFSZ + 56], %i2
	ldd	[%sp + CCFSZ + 64], %i4
	ldd	[%sp + CCFSZ + 72], %i6
	b	return_from_trap
	 wr	%l0, 0, %psr

/*
 * Do a `software' trap by re-entering the trap code, possibly first
 * switching from interrupt stack to kernel stack.  This is used for
 * scheduling and signal ASTs (which generally occur from softclock or
 * tty or net interrupts) and register window saves (which might occur
 * from anywhere).
 *
 * The current window is the trap window, and it is by definition clean.
 * We enter with the trap type in %o0.  All we have to do is jump to
 * Lslowtrap_reenter above, but maybe after switching stacks....
 */
softtrap:
#if defined(MULTIPROCESSOR)
	/*
	 * The interrupt stack is not at a fixed location
	 * and %sp must be checked against both ends.
	 */
	sethi	%hi(_EINTSTACKP), %l6
	ld	[%l6 + %lo(_EINTSTACKP)], %l7
	cmp	%sp, %l7
	bge	Lslowtrap_reenter
	 .empty
	set	INT_STACK_SIZE, %l6
	sub	%l7, %l6, %l7
	cmp	%sp, %l7
	blu	Lslowtrap_reenter
	 .empty
#else
	sethi	%hi(_C_LABEL(eintstack)), %l7
	cmp	%sp, %l7
	bge	Lslowtrap_reenter
	 .empty
#endif
	sethi	%hi(cpcb), %l6
	ld	[%l6 + %lo(cpcb)], %l6
	set	USPACE-CCFSZ-80, %l5
	add	%l6, %l5, %l7
	SET_SP_REDZONE(%l6, %l5)
	b	Lslowtrap_reenter
	 mov	%l7, %sp

#ifdef KGDB
/*
 * bpt is entered on all breakpoint traps.
 * If this is a kernel breakpoint, we do not want to call trap().
 * Among other reasons, this way we can set breakpoints in trap().
 */
bpt:
	btst	PSR_PS, %l0		! breakpoint from kernel?
	bz	slowtrap		! no, go do regular trap
	 nop

/* XXXSMP */
	/*
	 * Build a trap frame for kgdb_trap_glue to copy.
	 * Enable traps but set ipl high so that we will not
	 * see interrupts from within breakpoints.
	 */
	TRAP_SETUP(-CCFSZ-80)
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
	call	_C_LABEL(kgdb_trap_glue)! kgdb_trap_glue(type, &trapframe)
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
_ENTRY(_C_LABEL(kgdb_trap_glue))
	save	%sp, -CCFSZ, %sp

	call	_C_LABEL(write_all_windows)
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

#if defined(DEBUG) && !defined(MULTIPROCESSOR)
	/* save old red zone and then turn it off */
	sethi	%hi(_redzone), %l7
	ld	[%l7 + %lo(_redzone)], %l6
	st	%g0, [%l7 + %lo(_redzone)]
#endif
	/* switch to kgdb stack */
	add	%l0, -CCFSZ-80, %sp

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
#if defined(DEBUG) && !defined(MULTIPROCESSOR)
	st	%l6, [%l7 + %lo(_redzone)]	! restore red zone
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
#if defined(DEBUG) && !defined(MULTIPROCESSOR)
	st	%l6, [%l7 + %lo(_redzone)] ! and restore red zone
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
	sll	%l1, %l0, %l1
	wr	%l1, 0, %wim		! %wim = 1 << (%psr & 31)
	sethi	%hi(cpcb), %l1
	ld	[%l1 + %lo(cpcb)], %l1
	and	%l0, 31, %l0		! CWP = %psr & 31;
	st	%l0, [%l1 + PCB_WIM]	! cpcb->pcb_wim = CWP;
	save	%g0, %g0, %g0		! back to window to reload
	LOADWIN(%sp)
	save	%g0, %g0, %g0		! back to trap window
	/* note, we have not altered condition codes; safe to just rett */
	RETT
#endif

/*
 * syscall() builds a trap frame and calls syscall().
 * sun_syscall is same but delivers sun system call number
 * XXX	should not have to save&reload ALL the registers just for
 *	ptrace...
 */
_C_LABEL(_syscall):
	TRAP_SETUP(-CCFSZ-80)
#ifdef DEBUG
	or	%g1, 0x1000, %l6	! mark syscall
	TRAP_TRACE(%l6,%l5)
#endif
	wr	%l0, PSR_ET, %psr
	std	%l0, [%sp + CCFSZ + 0]	! tf_psr, tf_pc
	rd	%y, %l3
	std	%l2, [%sp + CCFSZ + 8]	! tf_npc, tf_y
	st	%g1, [%sp + CCFSZ + 20]	! tf_g[1]
	std	%g2, [%sp + CCFSZ + 24]	! tf_g[2], tf_g[3]
	std	%g4, [%sp + CCFSZ + 32]	! etc
	std	%g6, [%sp + CCFSZ + 40]
	mov	%g1, %o0		! (code)
	std	%i0, [%sp + CCFSZ + 48]
	add	%sp, CCFSZ, %o1		! (&tf)
	std	%i2, [%sp + CCFSZ + 56]
	mov	%l1, %o2		! (pc)
	std	%i4, [%sp + CCFSZ + 64]

	sethi	%hi(curlwp), %l1
	ld	[%l1 + %lo(curlwp)], %l1
	ld	[%l1 + L_PROC], %l1
	ld	[%l1 + P_MD_SYSCALL], %l1
	call	%l1			! syscall(code, &tf, pc, suncompat)
	 std	%i6, [%sp + CCFSZ + 72]
	! now load em all up again, sigh
	ldd	[%sp + CCFSZ + 0], %l0	! new %psr, new pc
	ldd	[%sp + CCFSZ + 8], %l2	! new npc, new %y
	wr	%l3, 0, %y
	/* see `lwp_trampoline' for the reason for this label */
return_from_syscall:
	ld	[%sp + CCFSZ + 20], %g1
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	ldd	[%sp + CCFSZ + 48], %i0
	ldd	[%sp + CCFSZ + 56], %i2
	ldd	[%sp + CCFSZ + 64], %i4
	ldd	[%sp + CCFSZ + 72], %i6
	b	return_from_trap
	 wr	%l0, 0, %psr

/*
 * Interrupts.  Software interrupts must be cleared from the software
 * interrupt enable register.  Rather than calling ienab_bic for each,
 * we do them in-line before enabling traps.
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
 * Software interrupts are almost the same with three exceptions:
 * (1) we clear the interrupt from the software interrupt enable
 *     register before calling any handler (we have to clear it first
 *     to avoid an interrupt-losing race),
 * (2) we always call all the registered handlers (there is no way
 *     to tell if the single bit in the software interrupt register
 *     represents one or many requests)
 * (3) we never announce a stray interrupt (because of (1), another
 *     interrupt request can come in while we're in the handler.  If
 *     the handler deals with everything for both the original & the
 *     new request, we'll erroneously report a stray interrupt when
 *     we take the software interrupt for the new request.
 *
 * Inputs:
 *	%l0 = %psr
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
 */
softintr_sun44c:
	/*
	 * Entry point for level 1, 4 or 6 interrupts on sun4/sun4c
	 * which may be software interrupts. Check the interrupt
	 * register to see whether we're dealing software or hardware
	 * interrupt.
	 */
	sethi	%hi(INTRREG_VA), %l6
	ldub	[%l6 + %lo(INTRREG_VA)], %l5
	btst	%l5, %l4		! is IE_L{1,4,6} set?
	bz	sparc_interrupt44c	! if not, must be a hw intr
	andn	%l5, %l4, %l5		! clear soft intr bit
	stb	%l5, [%l6 + %lo(INTRREG_VA)]

softintr_common:
	INTR_SETUP(-CCFSZ-80)
	std	%g2, [%sp + CCFSZ + 24]	! save registers
	! tally softint (curcpu()->cpu_data.cpu_nintr++) (clobbers %o0,%o1,%o2)
	INCR64(CPUINFO_VA + CPUINFO_NSOFT)
	mov	%g1, %l7
	rd	%y, %l6
	std	%g4, [%sp + CCFSZ + 32]
	andn	%l0, PSR_PIL, %l4	! %l4 = psr & ~PSR_PIL |
	sll	%l3, 8, %l5		!	intlev << IPLSHIFT
	std	%g6, [%sp + CCFSZ + 40]
	or	%l5, %l4, %l4		!			;
	wr	%l4, 0, %psr		! the manual claims this
	wr	%l4, PSR_ET, %psr	! song and dance is necessary
	std	%l0, [%sp + CCFSZ + 0]	! set up intrframe/clockframe
	sll	%l3, 2, %l5

	set	CPUINFO_VA + CPUINFO_SINTRCNT, %l4	! sintrcnt[intlev].ev_count++;
	sll	%l3, EV_STRUCTSHIFT, %o2
	ldd	[%l4 + %o2], %o0
	std	%l2, [%sp + CCFSZ + 8]	! set up intrframe/clockframe
	inccc   %o1
	addx    %o0, 0, %o0
	std	%o0, [%l4 + %o2]

	set	_C_LABEL(sintrhand), %l4! %l4 = sintrhand[intlev];
	ld	[%l4 + %l5], %l4

	sethi	%hi(CPUINFO_VA), %o2
	ld	[ %o2 + CPUINFO_IDEPTH ], %o3
	inc	%o3
	st	%o3, [ %o2 + CPUINFO_IDEPTH ]

	b	3f
	 st	%fp, [%sp + CCFSZ + 16]

1:	ld	[%l4 + IH_CLASSIPL], %o2 ! ih->ih_classipl
	rd	%psr, %o3		!  (bits already shifted to PIL field)
	andn	%o3, PSR_PIL, %o3	! %o3 = psr & ~PSR_PIL
	wr	%o3, %o2, %psr		! splraise(ih->ih_classipl)
	ld	[%l4 + IH_FUN], %o1
	ld	[%l4 + IH_ARG], %o0
	nop				! one more isns before touching ICC
	tst	%o0
	bz,a	2f
	 add	%sp, CCFSZ, %o0
2:	jmpl	%o1, %o7		!	(void)(*ih->ih_fun)(...)
	 ld	[%l4 + IH_NEXT], %l4	!	and ih = ih->ih_next
3:	tst	%l4			! while ih != NULL
	bnz	1b
	 nop

	sethi	%hi(CPUINFO_VA), %o2
	ld	[ %o2 + CPUINFO_IDEPTH ], %o3
	dec	%o3
	st	%o3, [ %o2 + CPUINFO_IDEPTH ]

	mov	%l7, %g1
	wr	%l6, 0, %y
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	b	return_from_trap
	 wr	%l0, 0, %psr

	/*
	 * _sparc_interrupt{44c,4m} is exported for paranoia checking
	 * (see intr.c).
	 */
#if defined(SUN4M)
_ENTRY(_C_LABEL(sparc_interrupt4m))
#if !defined(MSIIEP)	/* "normal" sun4m */
	sethi	%hi(CPUINFO_VA), %l6
	ld	[%l6 + CPUINFO_INTREG], %l7
	mov	1, %l4
	ld	[%l7 + ICR_PI_PEND_OFFSET], %l5	! get pending interrupts
	sll	%l4, %l3, %l4	! hw intr bits are in the lower halfword

	btst	%l4, %l5	! has pending hw intr at this level?
	bnz	sparc_interrupt_common
	 nop

	! both softint pending and clear bits are in upper halfwords of
	! their respective registers so shift the test bit in %l4 up there
	sll	%l4, 16, %l4

	st	%l4, [%l7 + ICR_PI_CLR_OFFSET]	! ack soft intr
#if defined(MULTIPROCESSOR)
	cmp	%l3, 14
	be	lev14_softint
#endif
	/* Drain hw reg; might be necessary for Ross CPUs */
	 ld	[%l7 + ICR_PI_PEND_OFFSET], %g0

#ifdef DIAGNOSTIC
	btst	%l4, %l5	! make sure softint pending bit is set
	bnz	softintr_common
	/* FALLTHROUGH to sparc_interrupt4m_bogus */
#else
	b	softintr_common
#endif
	 nop

#else /* MSIIEP */
	sethi	%hi(MSIIEP_PCIC_VA), %l6
	mov	1, %l4
	xor	%l3, 0x18, %l7	! change endianness of the resulting bit mask
	ld	[%l6 + PCIC_PROC_IPR_REG], %l5 ! get pending interrupts
	sll	%l4, %l7, %l4	! hw intr bits are in the upper halfword
				! because the register is little-endian
	btst	%l4, %l5	! has pending hw intr at this level?
	bnz	sparc_interrupt_common
	 nop

	srl	%l4, 16, %l4	! move the mask bit into the lower 16 bit
				! so we can use it to clear a sw interrupt

#ifdef DIAGNOSTIC
	! check if there's really a sw interrupt pending
	btst	%l4, %l5	! make sure softint pending bit is set
	bnz	softintr_common
	 sth	%l4, [%l6 + PCIC_SOFT_INTR_CLEAR_REG]
	/* FALLTHROUGH to sparc_interrupt4m_bogus */
#else
	b	softintr_common
	 sth	%l4, [%l6 + PCIC_SOFT_INTR_CLEAR_REG]
#endif

#endif /* MSIIEP */

#ifdef DIAGNOSTIC
	/*
	 * sparc_interrupt4m detected that neither hardware nor software
	 * interrupt pending bit is set for this interrupt.  Report this
	 * situation, this is most probably a symptom of a driver bug.
	 */
sparc_interrupt4m_bogus:
	INTR_SETUP(-CCFSZ-80)
	std	%g2, [%sp + CCFSZ + 24]	! save registers
	! tally interrupt (curcpu()->cpu_data.cpu_nintr++) (clobbers %o0,%o1)
	INCR64X(CPUINFO_VA + CPUINFO_NINTR, %o0, %o1, %l7)
	mov	%g1, %l7
	rd	%y, %l6
	std	%g4, [%sp + CCFSZ + 32]
	andn	%l0, PSR_PIL, %l4	! %l4 = psr & ~PSR_PIL |
	sll	%l3, 8, %l5		!	intlev << IPLSHIFT
	std	%g6, [%sp + CCFSZ + 40]
	or	%l5, %l4, %l4		!			;
	wr	%l4, 0, %psr		! the manual claims this
	wr	%l4, PSR_ET, %psr	! song and dance is necessary
	std	%l0, [%sp + CCFSZ + 0]	! set up intrframe/clockframe
	sll	%l3, 2, %l5

	set	CPUINFO_VA + CPUINFO_INTRCNT, %l4	! intrcnt[intlev].ev_count++;
	sll	%l3, EV_STRUCTSHIFT, %o2
	ldd	[%l4 + %o2], %o0
	std	%l2, [%sp + CCFSZ + 8]	! set up intrframe/clockframe
	inccc   %o1
	addx    %o0, 0, %o0
	std	%o0, [%l4 + %o2]

	st	%fp, [%sp + CCFSZ + 16]

	/* Unhandled interrupts while cold cause IPL to be raised to `high' */
	sethi	%hi(_C_LABEL(cold)), %o0
	ld	[%o0 + %lo(_C_LABEL(cold))], %o0
	tst	%o0			! if (cold) {
	bnz,a	1f			!	splhigh();
	 or	%l0, 0xf00, %l0		! } else

	call	_C_LABEL(bogusintr)	!	strayintr(&intrframe)
	 add	%sp, CCFSZ, %o0
	/* all done: restore registers and go return */
1:
	mov	%l7, %g1
	wr	%l6, 0, %y
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	b	return_from_trap
	 wr	%l0, 0, %psr
#endif /* DIAGNOSTIC */
#endif /* SUN4M */

_ENTRY(_C_LABEL(sparc_interrupt44c))
sparc_interrupt_common:
	INTR_SETUP(-CCFSZ-80)
	std	%g2, [%sp + CCFSZ + 24]	! save registers
	! tally intr (curcpu()->cpu_data.cpu_nintr++) (clobbers %o0,%o1)
	INCR64X(CPUINFO_VA + CPUINFO_NINTR, %o0, %o1, %l7)
	mov	%g1, %l7
	rd	%y, %l6
	std	%g4, [%sp + CCFSZ + 32]
	andn	%l0, PSR_PIL, %l4	! %l4 = psr & ~PSR_PIL |
	sll	%l3, 8, %l5		!	intlev << IPLSHIFT
	std	%g6, [%sp + CCFSZ + 40]
	or	%l5, %l4, %l4		!			;
	wr	%l4, 0, %psr		! the manual claims this
	wr	%l4, PSR_ET, %psr	! song and dance is necessary
	std	%l0, [%sp + CCFSZ + 0]	! set up intrframe/clockframe
	sll	%l3, 2, %l5

	set	CPUINFO_VA + CPUINFO_INTRCNT, %l4	! intrcnt[intlev].ev_count++;
	sll	%l3, EV_STRUCTSHIFT, %o2
	ldd	[%l4 + %o2], %o0
	std	%l2, [%sp + CCFSZ + 8]	! set up intrframe/clockframe
	inccc   %o1
	addx    %o0, 0, %o0
	std	%o0, [%l4 + %o2]

	set	_C_LABEL(intrhand), %l4	! %l4 = intrhand[intlev];
	ld	[%l4 + %l5], %l4

	sethi	%hi(CPUINFO_VA), %o2
	ld	[ %o2 + CPUINFO_IDEPTH ], %o3
	inc	%o3
	st	%o3, [ %o2 + CPUINFO_IDEPTH ]

	b	3f
	 st	%fp, [%sp + CCFSZ + 16]

1:	ld	[%l4 + IH_CLASSIPL], %o2 ! ih->ih_classipl
	rd	%psr, %o3		!  (bits already shifted to PIL field)
	andn	%o3, PSR_PIL, %o3	! %o3 = psr & ~PSR_PIL
	wr	%o3, %o2, %psr		! splraise(ih->ih_classipl)
	ld	[%l4 + IH_FUN], %o1
	ld	[%l4 + IH_ARG], %o0
	nop				! one more isns before touching ICC
	tst	%o0
	bz,a	2f
	 add	%sp, CCFSZ, %o0
2:	jmpl	%o1, %o7		!	handled = (*ih->ih_fun)(...)
	 ld	[%l4 + IH_NEXT], %l4	!	and ih = ih->ih_next
	tst	%o0
	bnz	4f			! if (handled) break
	 nop
3:	tst	%l4
	bnz	1b			! while (ih)
	 nop

	/* Unhandled interrupts while cold cause IPL to be raised to `high' */
	sethi	%hi(_C_LABEL(cold)), %o0
	ld	[%o0 + %lo(_C_LABEL(cold))], %o0
	tst	%o0			! if (cold) {
	bnz,a	4f			!	splhigh();
	 or	%l0, 0xf00, %l0		! } else

	call	_C_LABEL(strayintr)	!	strayintr(&intrframe)
	 add	%sp, CCFSZ, %o0
	/* all done: restore registers and go return */
4:
	sethi	%hi(CPUINFO_VA), %o2
	ld	[ %o2 + CPUINFO_IDEPTH ], %o3
	dec	%o3
	st	%o3, [ %o2 + CPUINFO_IDEPTH ]

	mov	%l7, %g1
	wr	%l6, 0, %y
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	b	return_from_trap
	 wr	%l0, 0, %psr

#if defined(MULTIPROCESSOR)
/*
 * Level 14 software interrupt: fast IPI
 * <%l0,%l1,%l2> = <psr, pc, npc>
 * %l3 = int level
 * %l6 = &cpuinfo
 */
lev14_softint:
	sethi	%hi(CPUINFO_VA), %l7
	ldd	[%l7 + CPUINFO_LEV14], %l4
	inccc	%l5
	addx	%l4, %g0, %l4
	std	%l4, [%l7 + CPUINFO_LEV14]

	ld	[%l6 + CPUINFO_XMSG_TRAP], %l7
#ifdef DIAGNOSTIC
	tst	%l7
	bz	sparc_interrupt4m_bogus
	 nop
#endif
	jmp	%l7
	 ld	[%l6 + CPUINFO_XMSG_ARG0], %l3	! prefetch 1st arg

/*
 * Fast flush handlers. xcalled from other CPUs throught soft interrupt 14
 * On entry:	%l6 = CPUINFO_VA
 *		%l3 = first argument
 *
 * As always, these fast trap handlers should preserve all registers
 * except %l3 to %l7
 */
_ENTRY(_C_LABEL(ft_tlb_flush))
	!	<%l3 already fetched for us>	! va
	ld	[%l6 + CPUINFO_XMSG_ARG2], %l5	! level
	andn	%l3, 0xfff, %l3			! %l3 = (va&~0xfff | lvl);
	ld	[%l6 + CPUINFO_XMSG_ARG1], %l4	! context
	or	%l3, %l5, %l3

	mov	SRMMU_CXR, %l7			!
	lda	[%l7]ASI_SRMMU, %l5		! %l5 = old context
	sta	%l4, [%l7]ASI_SRMMU		! set new context

	sta	%g0, [%l3]ASI_SRMMUFP		! flush TLB

ft_rett:
	! common return from Fast Flush handlers
	! enter here with %l5 = ctx to restore, %l6 = CPUINFO_VA, %l7 = ctx reg
	mov	1, %l4				!
	sta	%l5, [%l7]ASI_SRMMU		! restore context
	st	%l4, [%l6 + CPUINFO_XMSG_CMPLT]	! completed = 1

	mov	%l0, %psr			! return from trap
	 nop
	RETT

_ENTRY(_C_LABEL(ft_srmmu_vcache_flush_page))
	!	<%l3 already fetched for us>	! va
	ld	[%l6 + CPUINFO_XMSG_ARG1], %l4	! context

	mov	SRMMU_CXR, %l7			!
	lda	[%l7]ASI_SRMMU, %l5		! %l5 = old context
	sta	%l4, [%l7]ASI_SRMMU		! set new context

	set	4096, %l4			! N = page size
	ld	[%l6 + CPUINFO_CACHE_LINESZ], %l7
1:
	sta	%g0, [%l3]ASI_IDCACHELFP	!  flush cache line
	subcc	%l4, %l7, %l4			!  p += linesz;
	bgu	1b				! while ((N -= linesz) > 0)
	 add	%l3, %l7, %l3

	ld	[%l6 + CPUINFO_XMSG_ARG0], %l3	! reload va
	!or	%l3, ASI_SRMMUFP_L3(=0), %l3	! va |= ASI_SRMMUFP_L3
	sta	%g0, [%l3]ASI_SRMMUFP		! flush TLB

	b	ft_rett
	 mov	SRMMU_CXR, %l7			! reload ctx register

_ENTRY(_C_LABEL(ft_srmmu_vcache_flush_segment))
	!	<%l3 already fetched for us>	! vr
	ld	[%l6 + CPUINFO_XMSG_ARG1], %l5	! vs
	ld	[%l6 + CPUINFO_XMSG_ARG2], %l4	! context

	sll	%l3, 24, %l3			! va = VSTOVA(vr,vs)
	sll	%l5, 18, %l5
	or	%l3, %l5, %l3

	mov	SRMMU_CXR, %l7			!
	lda	[%l7]ASI_SRMMU, %l5		! %l5 = old context
	sta	%l4, [%l7]ASI_SRMMU		! set new context

	ld	[%l6 + CPUINFO_CACHE_NLINES], %l4
	ld	[%l6 + CPUINFO_CACHE_LINESZ], %l7
1:
	sta	%g0, [%l3]ASI_IDCACHELFS	!  flush cache line
	deccc	%l4				!  p += linesz;
	bgu	1b				! while (--nlines > 0)
	 add	%l3, %l7, %l3

	b	ft_rett
	 mov	SRMMU_CXR, %l7			! reload ctx register

_ENTRY(_C_LABEL(ft_srmmu_vcache_flush_region))
	!	<%l3 already fetched for us>	! vr
	ld	[%l6 + CPUINFO_XMSG_ARG1], %l4	! context

	sll	%l3, 24, %l3			! va = VRTOVA(vr)

	mov	SRMMU_CXR, %l7			!
	lda	[%l7]ASI_SRMMU, %l5		! %l5 = old context
	sta	%l4, [%l7]ASI_SRMMU		! set new context

	ld	[%l6 + CPUINFO_CACHE_NLINES], %l4
	ld	[%l6 + CPUINFO_CACHE_LINESZ], %l7
1:
	sta	%g0, [%l3]ASI_IDCACHELFR	!  flush cache line
	deccc	%l4				!  p += linesz;
	bgu	1b				! while (--nlines > 0)
	 add	%l3, %l7, %l3

	b	ft_rett
	 mov	SRMMU_CXR, %l7			! reload ctx register

_ENTRY(_C_LABEL(ft_srmmu_vcache_flush_context))
	!	<%l3 already fetched for us>	! context

	mov	SRMMU_CXR, %l7			!
	lda	[%l7]ASI_SRMMU, %l5		! %l5 = old context
	sta	%l3, [%l7]ASI_SRMMU		! set new context

	ld	[%l6 + CPUINFO_CACHE_NLINES], %l4
	ld	[%l6 + CPUINFO_CACHE_LINESZ], %l7
	mov	%g0, %l3			! va = 0
1:
	sta	%g0, [%l3]ASI_IDCACHELFC	!  flush cache line
	deccc	%l4				!  p += linesz;
	bgu	1b				! while (--nlines > 0)
	 add	%l3, %l7, %l3

	b	ft_rett
	 mov	SRMMU_CXR, %l7			! reload ctx register

_ENTRY(_C_LABEL(ft_srmmu_vcache_flush_range))
	!	<%l3 already fetched for us>	! va
	ld	[%l6 + CPUINFO_XMSG_ARG2], %l4	! context

	mov	SRMMU_CXR, %l7			!
	lda	[%l7]ASI_SRMMU, %l5		! %l5 = old context
	sta	%l4, [%l7]ASI_SRMMU		! set new context

	ld	[%l6 + CPUINFO_XMSG_ARG1], %l4	! size
	and	%l3, 7, %l7			! double-word alignment
	andn	%l3, 7, %l3			!  off = va & 7; va &= ~7
	add	%l4, %l7, %l4			!  sz += off

	ld	[%l6 + CPUINFO_CACHE_LINESZ], %l7
1:
	sta	%g0, [%l3]ASI_IDCACHELFP	!  flush cache line
	subcc	%l4, %l7, %l4			!  p += linesz;
	bgu	1b				! while ((sz -= linesz) > 0)
	 add	%l3, %l7, %l3

	/* Flush TLB on all pages we visited */
	ld	[%l6 + CPUINFO_XMSG_ARG0], %l3	! reload va
	ld	[%l6 + CPUINFO_XMSG_ARG1], %l4	! reload sz
	add	%l3, %l4, %l4			! %l4 = round_page(va + sz)
	add	%l4, 0xfff, %l4
	andn	%l4, 0xfff, %l4
	andn	%l3, 0xfff, %l3			! va &= ~PGOFSET;
	sub	%l4, %l3, %l4			! and finally: size rounded
						! to page boundary
	set	4096, %l7			! page size

2:
	!or	%l3, ASI_SRMMUFP_L3(=0), %l3	!  va |= ASI_SRMMUFP_L3
	sta	%g0, [%l3]ASI_SRMMUFP		!  flush TLB
	subcc	%l4, %l7, %l4			! while ((sz -= PGSIZE) > 0)
	bgu	2b
	 add	%l3, %l7, %l3

	b	ft_rett
	 mov	SRMMU_CXR, %l7			! reload ctx register

#endif /* MULTIPROCESSOR */

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

/*
 * Level 15 interrupt.  An async memory error has occurred;
 * take care of it (typically by panicking, but hey...).
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l3 = 15 * 4 (why? just because!)
 *
 * Internal:
 *	%l4 = %y
 *	%l5 = %g1
 *	%l6 = %g6
 *	%l7 = %g7
 *  g2, g3, g4, g5 go to stack
 *
 * This code is almost the same as that in mem_access_fault,
 * except that we already know the problem is not a `normal' fault,
 * and that we must be extra-careful with interrupt enables.
 */

#if defined(SUN4)
_ENTRY(_C_LABEL(nmi_sun4))
	INTR_SETUP(-CCFSZ-80)
	! tally intr (curcpu()->cpu_data.cpu_nintr++) (clobbers %o0,%o1,%o2)
	INCR64(CPUINFO_VA + CPUINFO_NINTR)
	/*
	 * Level 15 interrupts are nonmaskable, so with traps off,
	 * disable all interrupts to prevent recursion.
	 */
	sethi	%hi(INTRREG_VA), %o0
	ldub	[%o0 + %lo(INTRREG_VA)], %o1
	andn	%o1, IE_ALLIE, %o1
	stb	%o1, [%o0 + %lo(INTRREG_VA)]
	wr	%l0, PSR_ET, %psr	! okay, turn traps on again

	std	%g2, [%sp + CCFSZ + 0]	! save g2, g3
	rd	%y, %l4			! save y

	std	%g4, [%sp + CCFSZ + 8]	! save g4, g5
	mov	%g1, %l5		! save g1, g6, g7
	mov	%g6, %l6
	mov	%g7, %l7
#if defined(SUN4C) || defined(SUN4M)
	b,a	nmi_common
#endif /* SUN4C || SUN4M */
#endif

#if defined(SUN4C)
_ENTRY(_C_LABEL(nmi_sun4c))
	INTR_SETUP(-CCFSZ-80)
	! tally intr (curcpu()->cpu_data.cpu_nintr++) (clobbers %o0,%o1,%o2)
	INCR64(CPUINFO_VA + CPUINFO_NINTR)
	/*
	 * Level 15 interrupts are nonmaskable, so with traps off,
	 * disable all interrupts to prevent recursion.
	 */
	sethi	%hi(INTRREG_VA), %o0
	ldub	[%o0 + %lo(INTRREG_VA)], %o1
	andn	%o1, IE_ALLIE, %o1
	stb	%o1, [%o0 + %lo(INTRREG_VA)]
	wr	%l0, PSR_ET, %psr	! okay, turn traps on again

	std	%g2, [%sp + CCFSZ + 0]	! save g2, g3
	rd	%y, %l4			! save y

	! must read the sync error register too.
	set	AC_SYNC_ERR, %o0
	lda	[%o0] ASI_CONTROL, %o1	! sync err reg
	inc	4, %o0
	lda	[%o0] ASI_CONTROL, %o2	! sync virt addr
	std	%g4, [%sp + CCFSZ + 8]	! save g4,g5
	mov	%g1, %l5		! save g1,g6,g7
	mov	%g6, %l6
	mov	%g7, %l7
	inc	4, %o0
	lda	[%o0] ASI_CONTROL, %o3	! async err reg
	inc	4, %o0
	lda	[%o0] ASI_CONTROL, %o4	! async virt addr
#if defined(SUN4M)
	!!b,a	nmi_common
#endif /* SUN4M */
#endif /* SUN4C */

_ENTRY(_C_LABEL(nmi_common))
	! and call C code
	call	_C_LABEL(memerr4_4c)	! memerr(0, ser, sva, aer, ava)
	 clr	%o0

	mov	%l5, %g1		! restore g1 through g7
	ldd	[%sp + CCFSZ + 0], %g2
	ldd	[%sp + CCFSZ + 8], %g4
	wr	%l0, 0, %psr		! re-disable traps
	mov	%l6, %g6
	mov	%l7, %g7

	! set IE_ALLIE again (safe, we disabled traps again above)
	sethi	%hi(INTRREG_VA), %o0
	ldub	[%o0 + %lo(INTRREG_VA)], %o1
	or	%o1, IE_ALLIE, %o1
	stb	%o1, [%o0 + %lo(INTRREG_VA)]
	b	return_from_trap
	 wr	%l4, 0, %y		! restore y

#if defined(SUN4M)
_ENTRY(_C_LABEL(nmi_sun4m))
	INTR_SETUP(-CCFSZ-80-8-8)	! normal frame, plus g2..g5

#if !defined(MSIIEP) /* normal sun4m */

	/* Read the Pending Interrupts register */
	sethi	%hi(CPUINFO_VA+CPUINFO_INTREG), %l6
	ld	[%l6 + %lo(CPUINFO_VA+CPUINFO_INTREG)], %l6
	ld	[%l6 + ICR_PI_PEND_OFFSET], %l5	! get pending interrupts

	set	_C_LABEL(nmi_soft), %o3		! assume a softint
	set	PINTR_IC, %o1			! hard lvl 15 bit
	sethi	%hi(PINTR_SINTRLEV(15)), %o0	! soft lvl 15 bit
	btst	%o0, %l5		! soft level 15?
	bnz,a	1f			!
	 mov	%o0, %o1		! shift int clear bit to SOFTINT 15

	set	_C_LABEL(nmi_hard), %o3	/* it's a hardint; switch handler */

	/*
	 * Level 15 interrupts are nonmaskable, so with traps off,
	 * disable all interrupts to prevent recursion.
	 */
	sethi	%hi(ICR_SI_SET), %o0
	set	SINTR_MA, %o2
	st	%o2, [%o0 + %lo(ICR_SI_SET)]
#if defined(MULTIPROCESSOR) && defined(DDB)
	b	2f
	 clr	%o0
#endif

1:
#if defined(MULTIPROCESSOR) && defined(DDB)
	/*
	 * Setup a trapframe for nmi_soft; this might be an IPI telling
	 * us to pause, so lets save some state for DDB to get at.
	 */
	std	%l0, [%sp + CCFSZ]	! tf.tf_psr = psr; tf.tf_pc = ret_pc;
	rd	%y, %l3
	std	%l2, [%sp + CCFSZ + 8]	! tf.tf_npc = return_npc; tf.tf_y = %y;
	st	%g1, [%sp + CCFSZ + 20]
	std	%g2, [%sp + CCFSZ + 24]
	std	%g4, [%sp + CCFSZ + 32]
	std	%g6, [%sp + CCFSZ + 40]
	std	%i0, [%sp + CCFSZ + 48]
	std	%i2, [%sp + CCFSZ + 56]
	std	%i4, [%sp + CCFSZ + 64]
	std	%i6, [%sp + CCFSZ + 72]
	add	%sp, CCFSZ, %o0
2:
#else
	clr	%o0
#endif
	/*
	 * Now clear the NMI. Apparently, we must allow some time
	 * to let the bits sink in..
	 */
	st	%o1, [%l6 + ICR_PI_CLR_OFFSET]
	 nop; nop; nop;
	ld	[%l6 + ICR_PI_PEND_OFFSET], %g0	! drain register!?
	 nop;

	or	%l0, PSR_PIL, %o4	! splhigh()
	wr	%o4, 0, %psr		!
	wr	%o4, PSR_ET, %psr	! turn traps on again

	std	%g2, [%sp + CCFSZ + 80]	! save g2, g3
	rd	%y, %l4			! save y
	std	%g4, [%sp + CCFSZ + 88]	! save g4,g5

	/* Finish stackframe, call C trap handler */
	mov	%g1, %l5		! save g1,g6,g7
	mov	%g6, %l6

	jmpl	%o3, %o7		! nmi_hard(0) or nmi_soft(&tf)
	 mov	%g7, %l7

	mov	%l5, %g1		! restore g1 through g7
	ldd	[%sp + CCFSZ + 80], %g2
	ldd	[%sp + CCFSZ + 88], %g4
	wr	%l0, 0, %psr		! re-disable traps
	mov	%l6, %g6
	mov	%l7, %g7

	!cmp	%o0, 0			! was this a soft nmi
	!be	4f
	/* XXX - we need to unblock `mask all ints' only on a hard nmi */

	! enable interrupts again (safe, we disabled traps again above)
	sethi	%hi(ICR_SI_CLR), %o0
	set	SINTR_MA, %o1
	st	%o1, [%o0 + %lo(ICR_SI_CLR)]

4:
	b	return_from_trap
	 wr	%l4, 0, %y		! restore y

#else /* MSIIEP*/
	sethi	%hi(MSIIEP_PCIC_VA), %l6

	/* Read the Processor Interrupt Pending register */
	ld	[%l6 + PCIC_PROC_IPR_REG], %l5

	/*
	 * Level 15 interrupts are nonmaskable, so with traps off,
	 * disable all interrupts to prevent recursion.
	 */
	mov	0x80, %l4	! htole32(MSIIEP_SYS_ITMR_ALL)
	st	%l4, [%l6 + PCIC_SYS_ITMR_SET_REG]

	set	(1 << 23), %l4	! htole32(1 << 15)
	btst	%l4, %l5	! has pending level 15 hw intr?
	bz	1f
	 nop

	/* hard level 15 interrupt */
	sethi	%hi(_C_LABEL(nmi_hard_msiiep)), %o3
	b	2f
	 or	%o3, %lo(_C_LABEL(nmi_hard_msiiep)), %o3

1:	/* soft level 15 interrupt */
	set	(1 << 7), %l4	! htole16(1 << 15)
	sth	%l4, [%l6 + PCIC_SOFT_INTR_CLEAR_REG]
	set	_C_LABEL(nmi_soft_msiiep), %o3
2:

	/* XXX:	call sequence is identical to sun4m case above. merge? */
	or	%l0, PSR_PIL, %o4	! splhigh()
	wr	%o4, 0, %psr		!
	wr	%o4, PSR_ET, %psr	! turn traps on again

	std	%g2, [%sp + CCFSZ + 80]	! save g2, g3
	rd	%y, %l4			! save y
	std	%g4, [%sp + CCFSZ + 88]	! save g4, g5

	/* Finish stackframe, call C trap handler */
	mov	%g1, %l5		! save g1, g6, g7
	mov	%g6, %l6

	call	%o3			! nmi_hard(0) or nmi_soft(&tf)
	 mov	%g7, %l7

	mov	%l5, %g1		! restore g1 through g7
	ldd	[%sp + CCFSZ + 80], %g2
	ldd	[%sp + CCFSZ + 88], %g4
	wr	%l0, 0, %psr		! re-disable traps
	mov	%l6, %g6
	mov	%l7, %g7

	! enable interrupts again (safe, we disabled traps again above)
	sethi	%hi(MSIIEP_PCIC_VA), %o0
	mov	0x80, %o1	! htole32(MSIIEP_SYS_ITMR_ALL)
	st	%o1, [%o0 + PCIC_SYS_ITMR_CLR_REG]

	b	return_from_trap
	 wr	%l4, 0, %y		! restore y
#endif /* MSIIEP */
#endif /* SUN4M */


#ifdef GPROF
	.globl	window_of, winof_user
	.globl	window_uf, winuf_user, winuf_ok, winuf_invalid
	.globl	return_from_trap, rft_kernel, rft_user, rft_invalid
	.globl	softtrap, slowtrap
	.globl	clean_trap_window, _C_LABEL(_syscall)
#endif

/*
 * Window overflow trap handler.
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 */
window_of:
#ifdef TRIVIAL_WINDOW_OVERFLOW_HANDLER
	/* a trivial version that assumes %sp is ok */
	/* (for testing only!) */
	save	%g0, %g0, %g0
	std	%l0, [%sp + (0*8)]
	rd	%psr, %l0
	mov	1, %l1
	sll	%l1, %l0, %l0
	wr	%l0, 0, %wim
	std	%l2, [%sp + (1*8)]
	std	%l4, [%sp + (2*8)]
	std	%l6, [%sp + (3*8)]
	std	%i0, [%sp + (4*8)]
	std	%i2, [%sp + (5*8)]
	std	%i4, [%sp + (6*8)]
	std	%i6, [%sp + (7*8)]
	restore
	RETT
#else
	/*
	 * This is similar to TRAP_SETUP, but we do not want to spend
	 * a lot of time, so we have separate paths for kernel and user.
	 * We also know for sure that the window has overflowed.
	 */
	TRAP_TRACE2(5,%l6,%l5)
	btst	PSR_PS, %l0
	bz	winof_user
	 sethi	%hi(clean_trap_window), %l7

	/*
	 * Overflow from kernel mode.  Call clean_trap_window to
	 * do the dirty work, then just return, since we know prev
	 * window is valid.  clean_trap_windows might dump all *user*
	 * windows into the pcb, but we do not care: there is at
	 * least one kernel window (a trap or interrupt frame!)
	 * above us.
	 */
	jmpl	%l7 + %lo(clean_trap_window), %l4
	 mov	%g7, %l7		! for clean_trap_window

	wr	%l0, 0, %psr		! put back the @%*! cond. codes
	nop				! (let them settle in)
	RETT

winof_user:
	/*
	 * Overflow from user mode.
	 * If clean_trap_window dumps the registers into the pcb,
	 * rft_user will need to call trap(), so we need space for
	 * a trap frame.  We also have to compute pcb_nw.
	 *
	 * SHOULD EXPAND IN LINE TO AVOID BUILDING TRAP FRAME ON
	 * `EASY' SAVES
	 */
	sethi	%hi(cpcb), %l6
	ld	[%l6 + %lo(cpcb)], %l6
	ld	[%l6 + PCB_WIM], %l5
	and	%l0, 31, %l3
	sub	%l3, %l5, %l5 		/* l5 = CWP - pcb_wim */
	set	uwtab, %l4
	ldub	[%l4 + %l5], %l5	/* l5 = uwtab[l5] */
	st	%l5, [%l6 + PCB_UW]
	jmpl	%l7 + %lo(clean_trap_window), %l4
	 mov	%g7, %l7		! for clean_trap_window
	sethi	%hi(cpcb), %l6
	ld	[%l6 + %lo(cpcb)], %l6
	set	USPACE-CCFSZ-80, %l5
	add	%l6, %l5, %sp		/* over to kernel stack */
	CHECK_SP_REDZONE(%l6, %l5)

	/*
	 * Copy return_from_trap far enough to allow us
	 * to jump directly to rft_user_or_recover_pcb_windows
	 * (since we know that is where we are headed).
	 */
!	and	%l0, 31, %l3		! still set (clean_trap_window
					! leaves this register alone)
	set	wmask, %l6
	ldub	[%l6 + %l3], %l5	! %l5 = 1 << ((CWP + 1) % nwindows)
	b	rft_user_or_recover_pcb_windows
	 rd	%wim, %l4		! (read %wim first)
#endif /* end `real' version of window overflow trap handler */

/*
 * Window underflow trap handler.
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *
 * A picture:
 *
 *	  T R I X
 *	0 0 0 1 0 0 0	(%wim)
 * [bit numbers increase towards the right;
 * `restore' moves right & `save' moves left]
 *
 * T is the current (Trap) window, R is the window that attempted
 * a `Restore' instruction, I is the Invalid window, and X is the
 * window we want to make invalid before we return.
 *
 * Since window R is valid, we cannot use rft_user to restore stuff
 * for us.  We have to duplicate its logic.  YUCK.
 *
 * Incidentally, TRIX are for kids.  Silly rabbit!
 */
window_uf:
#ifdef TRIVIAL_WINDOW_UNDERFLOW_HANDLER
	wr	%g0, 0, %wim		! allow us to enter I
	restore				! to R
	nop
	nop
	restore				! to I
	restore	%g0, 1, %l1		! to X
	rd	%psr, %l0
	sll	%l1, %l0, %l0
	wr	%l0, 0, %wim
	save	%g0, %g0, %g0		! back to I
	LOADWIN(%sp)
	save	%g0, %g0, %g0		! back to R
	save	%g0, %g0, %g0		! back to T
	RETT
#else
	TRAP_TRACE2(6,%l6,%l5)
	wr	%g0, 0, %wim		! allow us to enter I
	btst	PSR_PS, %l0
	restore				! enter window R
	bz	winuf_user
	 restore			! enter window I

	/*
	 * Underflow from kernel mode.  Just recover the
	 * registers and go (except that we have to update
	 * the blasted user pcb fields).
	 */
	restore	%g0, 1, %l1		! enter window X, then set %l1 to 1
	rd	%psr, %l0		! cwp = %psr & 31;
	and	%l0, 31, %l0
	sll	%l1, %l0, %l1		! wim = 1 << cwp;
	wr	%l1, 0, %wim		! setwim(wim);
	sethi	%hi(cpcb), %l1
	ld	[%l1 + %lo(cpcb)], %l1
	st	%l0, [%l1 + PCB_WIM]	! cpcb->pcb_wim = cwp;
	save	%g0, %g0, %g0		! back to window I
	LOADWIN(%sp)
	save	%g0, %g0, %g0		! back to R
	save	%g0, %g0, %g0		! and then to T
	wr	%l0, 0, %psr		! fix those cond codes....
	nop				! (let them settle in)
	RETT

winuf_user:
	/*
	 * Underflow from user mode.
	 *
	 * We cannot use rft_user (as noted above) because
	 * we must re-execute the `restore' instruction.
	 * Since it could be, e.g., `restore %l0,0,%l0',
	 * it is not okay to touch R's registers either.
	 *
	 * We are now in window I.
	 */
	btst	7, %sp			! if unaligned, it is invalid
	bne	winuf_invalid
	 .empty

	sethi	%hi(_C_LABEL(pgofset)), %l4
	ld	[%l4 + %lo(_C_LABEL(pgofset))], %l4
	PTE_OF_ADDR(%sp, %l7, winuf_invalid, %l4, NOP_ON_4M_5)
	CMP_PTE_USER_READ(%l7, %l5, NOP_ON_4M_6) ! if first page not readable,
	bne	winuf_invalid		! it is invalid
	 .empty
	SLT_IF_1PAGE_RW(%sp, %l7, %l4)	! first page is readable
	bl,a	winuf_ok		! if only one page, enter window X
	 restore %g0, 1, %l1		! and goto ok, & set %l1 to 1
	add	%sp, 7*8, %l5
	add     %l4, 62, %l4
	PTE_OF_ADDR(%l5, %l7, winuf_invalid, %l4, NOP_ON_4M_7)
	CMP_PTE_USER_READ(%l7, %l5, NOP_ON_4M_8) ! check second page too
	be,a	winuf_ok		! enter window X and goto ok
	 restore %g0, 1, %l1		! (and then set %l1 to 1)

winuf_invalid:
	/*
	 * We were unable to restore the window because %sp
	 * is invalid or paged out.  Return to the trap window
	 * and call trap(T_WINUF).  This will save R to the user
	 * stack, then load both R and I into the pcb rw[] area,
	 * and return with pcb_nsaved set to -1 for success, 0 for
	 * failure.  `Failure' indicates that someone goofed with the
	 * trap registers (e.g., signals), so that we need to return
	 * from the trap as from a syscall (probably to a signal handler)
	 * and let it retry the restore instruction later.  Note that
	 * window R will have been pushed out to user space, and thus
	 * be the invalid window, by the time we get back here.  (We
	 * continue to label it R anyway.)  We must also set %wim again,
	 * and set pcb_uw to 1, before enabling traps.  (Window R is the
	 * only window, and it is a user window).
	 */
	save	%g0, %g0, %g0		! back to R
	save	%g0, 1, %l4		! back to T, then %l4 = 1
	sethi	%hi(cpcb), %l6
	ld	[%l6 + %lo(cpcb)], %l6
	st	%l4, [%l6 + PCB_UW]	! pcb_uw = 1
	ld	[%l6 + PCB_WIM], %l5	! get log2(%wim)
	sll	%l4, %l5, %l4		! %l4 = old %wim
	wr	%l4, 0, %wim		! window I is now invalid again
	set	USPACE-CCFSZ-80, %l5
	add	%l6, %l5, %sp		! get onto kernel stack
	CHECK_SP_REDZONE(%l6, %l5)

	/*
	 * Okay, call trap(T_WINUF, psr, pc, &tf).
	 * See `slowtrap' above for operation.
	 */
	wr	%l0, PSR_ET, %psr
	std	%l0, [%sp + CCFSZ + 0]	! tf.tf_psr, tf.tf_pc
	rd	%y, %l3
	std	%l2, [%sp + CCFSZ + 8]	! tf.tf_npc, tf.tf_y
	mov	T_WINUF, %o0
	st	%g1, [%sp + CCFSZ + 20]	! tf.tf_global[1]
	mov	%l0, %o1
	std	%g2, [%sp + CCFSZ + 24]	! etc
	mov	%l1, %o2
	std	%g4, [%sp + CCFSZ + 32]
	add	%sp, CCFSZ, %o3
	std	%g6, [%sp + CCFSZ + 40]
	std	%i0, [%sp + CCFSZ + 48]	! tf.tf_out[0], etc
	std	%i2, [%sp + CCFSZ + 56]
	std	%i4, [%sp + CCFSZ + 64]
	call	_C_LABEL(trap)		! trap(T_WINUF, pc, psr, &tf)
	 std	%i6, [%sp + CCFSZ + 72]	! tf.tf_out[6]

	ldd	[%sp + CCFSZ + 0], %l0	! new psr, pc
	ldd	[%sp + CCFSZ + 8], %l2	! new npc, %y
	wr	%l3, 0, %y
	ld	[%sp + CCFSZ + 20], %g1
	ldd	[%sp + CCFSZ + 24], %g2
	ldd	[%sp + CCFSZ + 32], %g4
	ldd	[%sp + CCFSZ + 40], %g6
	ldd	[%sp + CCFSZ + 48], %i0	! %o0 for window R, etc
	ldd	[%sp + CCFSZ + 56], %i2
	ldd	[%sp + CCFSZ + 64], %i4
	wr	%l0, 0, %psr		! disable traps: test must be atomic
	ldd	[%sp + CCFSZ + 72], %i6
	sethi	%hi(cpcb), %l6
	ld	[%l6 + %lo(cpcb)], %l6
	ld	[%l6 + PCB_NSAVED], %l7	! if nsaved is -1, we have our regs
	tst	%l7
	bl,a	1f			! got them
	 wr	%g0, 0, %wim		! allow us to enter windows R, I
	b,a	return_from_trap

	/*
	 * Got 'em.  Load 'em up.
	 */
1:
	mov	%g6, %l3		! save %g6; set %g6 = cpcb
	mov	%l6, %g6
	st	%g0, [%g6 + PCB_NSAVED]	! and clear magic flag
	restore				! from T to R
	restore				! from R to I
	restore	%g0, 1, %l1		! from I to X, then %l1 = 1
	rd	%psr, %l0		! cwp = %psr;
	sll	%l1, %l0, %l1
	wr	%l1, 0, %wim		! make window X invalid
	and	%l0, 31, %l0
	st	%l0, [%g6 + PCB_WIM]	! cpcb->pcb_wim = cwp;
	nop				! unnecessary? old wim was 0...
	save	%g0, %g0, %g0		! back to I
	LOADWIN(%g6 + PCB_RW + 64)	! load from rw[1]
	save	%g0, %g0, %g0		! back to R
	LOADWIN(%g6 + PCB_RW)		! load from rw[0]
	save	%g0, %g0, %g0		! back to T
	wr	%l0, 0, %psr		! restore condition codes
	mov	%l3, %g6		! fix %g6
	RETT

	/*
	 * Restoring from user stack, but everything has checked out
	 * as good.  We are now in window X, and %l1 = 1.  Window R
	 * is still valid and holds user values.
	 */
winuf_ok:
	rd	%psr, %l0
	sll	%l1, %l0, %l1
	wr	%l1, 0, %wim		! make this one invalid
	sethi	%hi(cpcb), %l2
	ld	[%l2 + %lo(cpcb)], %l2
	and	%l0, 31, %l0
	st	%l0, [%l2 + PCB_WIM]	! cpcb->pcb_wim = cwp;
	save	%g0, %g0, %g0		! back to I
	LOADWIN(%sp)
	save	%g0, %g0, %g0		! back to R
	save	%g0, %g0, %g0		! back to T
	wr	%l0, 0, %psr		! restore condition codes
	nop				! it takes three to tangle
	RETT
#endif /* end `real' version of window underflow trap handler */

/*
 * Various return-from-trap routines (see return_from_trap).
 */

/*
 * Return from trap, to kernel.
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l4 = %wim
 *	%l5 = bit for previous window
 */
rft_kernel:
	btst	%l5, %l4		! if (wim & l5)
	bnz	1f			!	goto reload;
	 wr	%l0, 0, %psr		! but first put !@#*% cond codes back

	/* previous window is valid; just rett */
	nop				! wait for cond codes to settle in
	RETT

	/*
	 * Previous window is invalid.
	 * Update %wim and then reload l0..i7 from frame.
	 *
	 *	  T I X
	 *	0 0 1 0 0   (%wim)
	 * [see picture in window_uf handler]
	 *
	 * T is the current (Trap) window, I is the Invalid window,
	 * and X is the window we want to make invalid.  Window X
	 * currently has no useful values.
	 */
1:
	wr	%g0, 0, %wim		! allow us to enter window I
	nop; nop; nop			! (it takes a while)
	restore				! enter window I
	restore	%g0, 1, %l1		! enter window X, then %l1 = 1
	rd	%psr, %l0		! CWP = %psr & 31;
	and	%l0, 31, %l0
	sll	%l1, %l0, %l1		! wim = 1 << CWP;
	wr	%l1, 0, %wim		! setwim(wim);
	sethi	%hi(cpcb), %l1
	ld	[%l1 + %lo(cpcb)], %l1
	st	%l0, [%l1 + PCB_WIM]	! cpcb->pcb_wim = l0 & 31;
	save	%g0, %g0, %g0		! back to window I
	LOADWIN(%sp)
	save	%g0, %g0, %g0		! back to window T
	/*
	 * Note that the condition codes are still set from
	 * the code at rft_kernel; we can simply return.
	 */
	RETT

/*
 * Return from trap, to user.  Checks for scheduling trap (`ast') first;
 * will re-enter trap() if set.  Note that we may have to switch from
 * the interrupt stack to the kernel stack in this case.
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *	%l4 = %wim
 *	%l5 = bit for previous window
 *	%l6 = cpcb
 * If returning to a valid window, just set psr and return.
 */
rft_user:
!	sethi	%hi(_WANT_AST)), %l7	! (done below)
	ld	[%l7 + %lo(_WANT_AST)], %l7
	tst	%l7			! want AST trap?
	bne,a	softtrap		! yes, re-enter trap with type T_AST
	 mov	T_AST, %o0

	btst	%l5, %l4		! if (wim & l5)
	bnz	1f			!	goto reload;
	 wr	%l0, 0, %psr		! restore cond codes
	nop				! (three instruction delay)
	RETT

	/*
	 * Previous window is invalid.
	 * Before we try to load it, we must verify its stack pointer.
	 * This is much like the underflow handler, but a bit easier
	 * since we can use our own local registers.
	 */
1:
	btst	7, %fp			! if unaligned, address is invalid
	bne	rft_invalid
	 .empty

	sethi	%hi(_C_LABEL(pgofset)), %l3
	ld	[%l3 + %lo(_C_LABEL(pgofset))], %l3
	PTE_OF_ADDR(%fp, %l7, rft_invalid, %l3, NOP_ON_4M_9)
	CMP_PTE_USER_READ(%l7, %l5, NOP_ON_4M_10)	! try first page
	bne	rft_invalid		! no good
	 .empty
	SLT_IF_1PAGE_RW(%fp, %l7, %l3)
	bl,a	rft_user_ok		! only 1 page: ok
	 wr	%g0, 0, %wim
	add	%fp, 7*8, %l5
	add	%l3, 62, %l3
	PTE_OF_ADDR(%l5, %l7, rft_invalid, %l3, NOP_ON_4M_11)
	CMP_PTE_USER_READ(%l7, %l5, NOP_ON_4M_12)	! check 2nd page too
	be,a	rft_user_ok
	 wr	%g0, 0, %wim

	/*
	 * The window we wanted to pull could not be pulled.  Instead,
	 * re-enter trap with type T_RWRET.  This will pull the window
	 * into cpcb->pcb_rw[0] and set cpcb->pcb_nsaved to -1, which we
	 * will detect when we try to return again.
	 */
rft_invalid:
	b	softtrap
	 mov	T_RWRET, %o0

	/*
	 * The window we want to pull can be pulled directly.
	 */
rft_user_ok:
!	wr	%g0, 0, %wim		! allow us to get into it
	wr	%l0, 0, %psr		! fix up the cond codes now
	nop; nop; nop
	restore				! enter window I
	restore	%g0, 1, %l1		! enter window X, then %l1 = 1
	rd	%psr, %l0		! l0 = (junk << 5) + CWP;
	sll	%l1, %l0, %l1		! %wim = 1 << CWP;
	wr	%l1, 0, %wim
	sethi	%hi(cpcb), %l1
	ld	[%l1 + %lo(cpcb)], %l1
	and	%l0, 31, %l0
	st	%l0, [%l1 + PCB_WIM]	! cpcb->pcb_wim = l0 & 31;
	save	%g0, %g0, %g0		! back to window I
	LOADWIN(%sp)			! suck hard
	save	%g0, %g0, %g0		! back to window T
	RETT

/*
 * Return from trap.  Entered after a
 *	wr	%l0, 0, %psr
 * which disables traps so that we can rett; registers are:
 *
 *	%l0 = %psr
 *	%l1 = return pc
 *	%l2 = return npc
 *
 * (%l3..%l7 anything).
 *
 * If we are returning to user code, we must:
 *  1.  Check for register windows in the pcb that belong on the stack.
 *	If there are any, reenter trap with type T_WINOF.
 *  2.  Make sure the register windows will not underflow.  This is
 *	much easier in kernel mode....
 */
return_from_trap:
!	wr	%l0, 0, %psr		! disable traps so we can rett
! (someone else did this already)
	and	%l0, 31, %l5
	set	wmask, %l6
	ldub	[%l6 + %l5], %l5	! %l5 = 1 << ((CWP + 1) % nwindows)
	btst	PSR_PS, %l0		! returning to userland?
	bnz	rft_kernel		! no, go return to kernel
	 rd	%wim, %l4		! (read %wim in any case)

rft_user_or_recover_pcb_windows:
	/*
	 * (entered with %l4=%wim, %l5=wmask[cwp]; %l0..%l2 as usual)
	 *
	 * check cpcb->pcb_nsaved:
	 * if 0, do a `normal' return to user (see rft_user);
	 * if > 0, cpcb->pcb_rw[] holds registers to be copied to stack;
	 * if -1, cpcb->pcb_rw[0] holds user registers for rett window
	 * from an earlier T_RWRET pseudo-trap.
	 */
	sethi	%hi(cpcb), %l6
	ld	[%l6 + %lo(cpcb)], %l6
	ld	[%l6 + PCB_NSAVED], %l7
	tst	%l7
	bz,a	rft_user
	 sethi	%hi(_WANT_AST), %l7	! first instr of rft_user

	bg,a	softtrap		! if (pcb_nsaved > 0)
	 mov	T_WINOF, %o0		!	trap(T_WINOF);

	/*
	 * To get here, we must have tried to return from a previous
	 * trap and discovered that it would cause a window underflow.
	 * We then must have tried to pull the registers out of the
	 * user stack (from the address in %fp==%i6) and discovered
	 * that it was either unaligned or not loaded in memory, and
	 * therefore we ran a trap(T_RWRET), which loaded one set of
	 * registers into cpcb->pcb_pcb_rw[0] (if it had killed the
	 * process due to a bad stack, we would not be here).
	 *
	 * We want to load pcb_rw[0] into the previous window, which
	 * we know is currently invalid.  In other words, we want
	 * %wim to be 1 << ((cwp + 2) % nwindows).
	 */
	wr	%g0, 0, %wim		! enable restores
	mov	%g6, %l3		! save g6 in l3
	mov	%l6, %g6		! set g6 = &u
	st	%g0, [%g6 + PCB_NSAVED]	! clear cpcb->pcb_nsaved
	restore				! enter window I
	restore	%g0, 1, %l1		! enter window X, then %l1 = 1
	rd	%psr, %l0
	sll	%l1, %l0, %l1		! %wim = 1 << CWP;
	wr	%l1, 0, %wim
	and	%l0, 31, %l0
	st	%l0, [%g6 + PCB_WIM]	! cpcb->pcb_wim = CWP;
	nop				! unnecessary? old wim was 0...
	save	%g0, %g0, %g0		! back to window I
	LOADWIN(%g6 + PCB_RW)
	save	%g0, %g0, %g0		! back to window T (trap window)
	wr	%l0, 0, %psr		! cond codes, cond codes everywhere
	mov	%l3, %g6		! restore g6
	RETT

! exported end marker for kernel gdb
	.globl	_C_LABEL(endtrapcode)
_C_LABEL(endtrapcode):

/*
 * init_tables(nwin) int nwin;
 *
 * Set up the uwtab and wmask tables.
 * We know nwin > 1.
 */
init_tables:
	/*
	 * for (i = -nwin, j = nwin - 2; ++i < 0; j--)
	 *	uwtab[i] = j;
	 * (loop runs at least once)
	 */
	set	uwtab, %o3
	sub	%g0, %o0, %o1		! i = -nwin + 1
	inc	%o1
	add	%o0, -2, %o2		! j = nwin - 2;
0:
	stb	%o2, [%o3 + %o1]	! uwtab[i] = j;
1:
	inccc	%o1			! ++i < 0?
	bl	0b			! yes, continue loop
	 dec	%o2			! in any case, j--

	/*
	 * (i now equals 0)
	 * for (j = nwin - 1; i < nwin; i++, j--)
	 *	uwtab[i] = j;
	 * (loop runs at least twice)
	 */
	sub	%o0, 1, %o2		! j = nwin - 1
0:
	stb	%o2, [%o3 + %o1]	! uwtab[i] = j
	inc	%o1			! i++
1:
	cmp	%o1, %o0		! i < nwin?
	bl	0b			! yes, continue
	 dec	%o2			! in any case, j--

	/*
	 * We observe that, for i in 0..nwin-2, (i+1)%nwin == i+1;
	 * for i==nwin-1, (i+1)%nwin == 0.
	 * To avoid adding 1, we run i from 1 to nwin and set
	 * wmask[i-1].
	 *
	 * for (i = j = 1; i < nwin; i++) {
	 *	j <<= 1;	(j now == 1 << i)
	 *	wmask[i - 1] = j;
	 * }
	 * (loop runs at least once)
	 */
	set	wmask - 1, %o3
	mov	1, %o1			! i = 1;
	mov	2, %o2			! j = 2;
0:
	stb	%o2, [%o3 + %o1]	! (wmask - 1)[i] = j;
	inc	%o1			! i++
	cmp	%o1, %o0		! i < nwin?
	bl,a	0b			! yes, continue
	 sll	%o2, 1, %o2		! (and j <<= 1)

	/*
	 * Now i==nwin, so we want wmask[i-1] = 1.
	 */
	mov	1, %o2			! j = 1;
	retl
	 stb	%o2, [%o3 + %o1]	! (wmask - 1)[i] = j;


dostart:
	/*
	 * Startup.
	 *
	 * We may have been loaded in low RAM, at some address which
	 * is page aligned (PROM_LOADADDR actually) rather than where we
	 * want to run (KERNBASE+PROM_LOADADDR).  Until we get everything set,
	 * we have to be sure to use only pc-relative addressing.
	 */

	/*
	 * Find out if the above is the case.
	 */
0:	call	1f
	 sethi	%hi(0b), %l0		! %l0 = virtual address of 0:
1:	or	%l0, %lo(0b), %l0
	sub	%l0, %o7, %l7		! subtract actual physical address of 0:

	/*
	 * If we're already running at our desired virtual load address,
	 * %l7 will be set to 0, otherwise it will be KERNBASE.
	 * From now on until the end of locore bootstrap code, %l7 will
	 * be used to relocate memory references.
	 */
#define RELOCATE(l,r)		\
	set	l, r;		\
	sub	r, %l7, r

	/*
	 * We use the bootinfo method to pass arguments, and the new
	 * magic number indicates that. A pointer to the kernel top, i.e.
	 * the first address after the load kernel image (including DDB
	 * symbols, if any) is passed in %o4[0] and the bootinfo structure
	 * is passed in %o4[1].
	 *
	 * A magic number is passed in %o5 to allow for bootloaders
	 * that know nothing about the bootinfo structure or previous
	 * DDB symbol loading conventions.
	 *
	 * For compatibility with older versions, we check for DDB arguments
	 * if the older magic number is there. The loader passes `kernel_top'
	 * (previously known as `esym') in %o4.
	 *
	 * Note: we don't touch %o1-%o3; SunOS bootloaders seem to use them
	 * for their own mirky business.
	 *
	 * Pre-NetBSD 1.3 bootblocks had KERNBASE compiled in, and used it
	 * to compute the value of `kernel_top' (previously known as `esym').
	 * In order to successfully boot a kernel built with a different value
	 * for KERNBASE using old bootblocks, we fixup `kernel_top' here by
	 * the difference between KERNBASE and the old value (known to be
	 * 0xf8000000) compiled into pre-1.3 bootblocks.
	 */

	set	0x44444232, %l3		! bootinfo magic
	cmp	%o5, %l3
	bne	1f
	 nop

	/* The loader has passed to us a `bootinfo' structure */
	ld	[%o4], %l3		! 1st word is kernel_top
	add	%l3, %l7, %o5		! relocate: + KERNBASE
	RELOCATE(_C_LABEL(kernel_top),%l3)
	st	%o5, [%l3]		! and store it

	ld	[%o4 + 4], %l3		! 2nd word is bootinfo
	add	%l3, %l7, %o5		! relocate
	RELOCATE(_C_LABEL(bootinfo),%l3)
	st	%o5, [%l3]		! store bootinfo
	b,a	4f

1:
#ifdef DDB
	/* Check for old-style DDB loader magic */
	set	KERNBASE, %l4
	set	0x44444231, %l3		! Is it DDB_MAGIC1?
	cmp	%o5, %l3
	be,a	2f
	 clr	%l4			! if DDB_MAGIC1, clear %l4

	set	0x44444230, %l3		! Is it DDB_MAGIC0?
	cmp	%o5, %l3		! if so, need to relocate %o4
	bne	3f			/* if not, there's no bootloader info */

					! note: %l4 set to KERNBASE above.
	set	0xf8000000, %l5		! compute correction term:
	sub	%l5, %l4, %l4		!  old KERNBASE (0xf8000000 ) - KERNBASE

2:
	tst	%o4			! do we have the symbols?
	bz	3f
	 sub	%o4, %l4, %o4		! apply compat correction
	sethi	%hi(_C_LABEL(kernel_top) - KERNBASE), %l3 ! and store it
	st	%o4, [%l3 + %lo(_C_LABEL(kernel_top) - KERNBASE)]
	b,a	4f
3:
#endif
	/*
	 * The boot loader did not pass in a value for `kernel_top';
	 * let it default to `end'.
	 */
	set	end, %o4
	RELOCATE(_C_LABEL(kernel_top),%l3)
	st	%o4, [%l3]	! store kernel_top

4:

	/*
	 * Sun4 passes in the `load address'.  Although possible, its highly
	 * unlikely that OpenBoot would place the prom vector there.
	 */
	set	PROM_LOADADDR, %g7
	cmp	%o0, %g7
	be	is_sun4
	 nop

#if defined(SUN4C) || defined(SUN4M) || defined(SUN4D)
	/*
	 * Be prepared to get OF client entry in either %o0 or %o3.
	 * XXX Will this ever trip on sun4d?  Let's hope not!
	 */
	cmp	%o0, 0
	be	is_openfirm
	 nop

	mov	%o0, %g7		! save romp passed by boot code

	/* First, check `romp->pv_magic' */
	ld	[%g7 + PV_MAGIC], %o0	! v = pv->pv_magic
	set	OBP_MAGIC, %o1
	cmp	%o0, %o1		! if ( v != OBP_MAGIC) {
	bne	is_sun4m		!    assume this is an OPENFIRM machine
	 nop				! }

	/*
	 * are we on a sun4c or a sun4m or a sun4d?
	 */
	ld	[%g7 + PV_NODEOPS], %o4	! node = pv->pv_nodeops->no_nextnode(0)
	ld	[%o4 + NO_NEXTNODE], %o4
	call	%o4
	 mov	0, %o0			! node

	!mov	%o0, %l0
	RELOCATE(cputypvar,%o1)		! name = "compatible"
	RELOCATE(cputypval,%l2)		! buffer ptr (assume buffer long enough)
	ld	[%g7 + PV_NODEOPS], %o4	! (void)pv->pv_nodeops->no_getprop(...)
	ld	[%o4 + NO_GETPROP], %o4
	call	 %o4
	 mov	%l2, %o2
	!set	cputypval-KERNBASE, %o2	! buffer ptr
	ldub	[%l2 + 4], %o0		! which is it... "sun4c", "sun4m", "sun4d"?
	cmp	%o0, 'c'
	be	is_sun4c
	 nop
	cmp	%o0, 'm'
	be	is_sun4m
	 nop
	cmp	%o0, 'd'
	be	is_sun4d
	 nop
#endif /* SUN4C || SUN4M || SUN4D */

	/*
	 * Don't know what type of machine this is; just halt back
	 * out to the PROM.
	 */
	ld	[%g7 + PV_HALT], %o1	! by this kernel, then halt
	call	%o1
	 nop

is_openfirm:
	! OF client entry in %o3 (kernel booted directly by PROM?)
	mov	%o3, %g7
	/* FALLTHROUGH to sun4m case */

is_sun4m:
#if defined(SUN4M)
	set	trapbase_sun4m, %g6
	mov	SUN4CM_PGSHIFT, %g5
	b	start_havetype
	 mov	CPU_SUN4M, %g4
#else
	RELOCATE(sun4m_notsup,%o0)
	ld	[%g7 + PV_EVAL], %o1
	call	%o1			! print a message saying that the
	 nop				! sun4m architecture is not supported
	ld	[%g7 + PV_HALT], %o1	! by this kernel, then halt
	call	%o1
	 nop
	/*NOTREACHED*/
#endif
is_sun4d:
#if defined(SUN4D)
	set	trapbase_sun4m, %g6	/* XXXJRT trapbase_sun4d */
	mov	SUN4CM_PGSHIFT, %g5
	b	start_havetype
	 mov	CPU_SUN4D, %g4
#else
	RELOCATE(sun4d_notsup,%o0)
	ld	[%g7 + PV_EVAL], %o1
	call	%o1			! print a message saying that the
	 nop				! sun4d architecture is not supported
	ld	[%g7 + PV_HALT], %o1	! by this kernel, then halt
	call	%o1
	 nop
	/*NOTREACHED*/
#endif
is_sun4c:
#if defined(SUN4C)
	set	trapbase_sun4c, %g6
	mov	SUN4CM_PGSHIFT, %g5

	set	AC_CONTEXT, %g1		! paranoia: set context to kernel
	stba	%g0, [%g1] ASI_CONTROL

	b	start_havetype
	 mov	CPU_SUN4C, %g4		! XXX CPU_SUN4
#else
	RELOCATE(sun4c_notsup,%o0)

	ld	[%g7 + PV_ROMVEC_VERS], %o1
	cmp	%o1, 0
	bne	1f
	 nop

	! stupid version 0 rom interface is pv_eval(int length, char *string)
	mov	%o0, %o1
2:	ldub	[%o0], %o4
	tst	%o4
	bne	2b
	 inc	%o0
	dec	%o0
	sub	%o0, %o1, %o0

1:	ld	[%g7 + PV_EVAL], %o2
	call	%o2			! print a message saying that the
	 nop				! sun4c architecture is not supported
	ld	[%g7 + PV_HALT], %o1	! by this kernel, then halt
	call	%o1
	 nop
	/*NOTREACHED*/
#endif
is_sun4:
#if defined(SUN4)
	set	trapbase_sun4, %g6
	mov	SUN4_PGSHIFT, %g5

	set	AC_CONTEXT, %g1		! paranoia: set context to kernel
	stba	%g0, [%g1] ASI_CONTROL

	b	start_havetype
	 mov	CPU_SUN4, %g4
#else
	set	PROM_BASE, %g7

	RELOCATE(sun4_notsup,%o0)
	ld	[%g7 + OLDMON_PRINTF], %o1
	call	%o1			! print a message saying that the
	 nop				! sun4 architecture is not supported
	ld	[%g7 + OLDMON_HALT], %o1 ! by this kernel, then halt
	call	%o1
	 nop
	/*NOTREACHED*/
#endif

start_havetype:
	cmp	%l7, 0
	be	startmap_done

	/*
	 * Step 1: double map low RAM (addresses [0.._end-start-1])
	 * to KERNBASE (addresses [KERNBASE.._end-1]).  None of these
	 * are `bad' aliases (since they are all on segment boundaries)
	 * so we do not have to worry about cache aliasing.
	 *
	 * We map in another couple of segments just to have some
	 * more memory (512K, actually) guaranteed available for
	 * bootstrap code (pmap_bootstrap needs memory to hold MMU
	 * and context data structures). Note: this is only relevant
	 * for 2-level MMU sun4/sun4c machines.
	 */
	clr	%l0			! lowva
	set	KERNBASE, %l1		! highva

	sethi	%hi(_C_LABEL(kernel_top) - KERNBASE), %o0
	ld	[%o0 + %lo(_C_LABEL(kernel_top) - KERNBASE)], %o1
	set	(2 << 18), %o2		! add slack for sun4c MMU
	add	%o1, %o2, %l2		! last va that must be remapped

	/*
	 * Need different initial mapping functions for different
	 * types of machines.
	 */
#if defined(SUN4C)
	cmp	%g4, CPU_SUN4C
	bne	1f
	 set	1 << 18, %l3		! segment size in bytes
0:
	lduba	[%l0] ASI_SEGMAP, %l4	! segmap[highva] = segmap[lowva];
	stba	%l4, [%l1] ASI_SEGMAP
	add	%l3, %l1, %l1		! highva += segsiz;
	cmp	%l1, %l2		! done?
	blu	0b			! no, loop
	 add	%l3, %l0, %l0		! (and lowva += segsz)
	b,a	startmap_done
1:
#endif /* SUN4C */

#if defined(SUN4)
	cmp	%g4, CPU_SUN4
	bne	2f
#if defined(SUN4_MMU3L)
	set	AC_IDPROM+1, %l3
	lduba	[%l3] ASI_CONTROL, %l3
	cmp	%l3, 0x24 ! XXX - SUN4_400
	bne	no_3mmu
	 nop

	/*
	 * Three-level sun4 MMU.
	 * Double-map by duplicating a single region entry (which covers
	 * 16MB) corresponding to the kernel's virtual load address.
	 */
	add	%l0, 2, %l0		! get to proper half-word in RG space
	add	%l1, 2, %l1
	lduha	[%l0] ASI_REGMAP, %l4	! regmap[highva] = regmap[lowva];
	stha	%l4, [%l1] ASI_REGMAP
	b,a	startmap_done
no_3mmu:
#endif

	/*
	 * Three-level sun4 MMU.
	 * Double-map by duplicating the required number of segment
	 * entries corresponding to the kernel's virtual load address.
	 */
	set	1 << 18, %l3		! segment size in bytes
0:
	lduha	[%l0] ASI_SEGMAP, %l4	! segmap[highva] = segmap[lowva];
	stha	%l4, [%l1] ASI_SEGMAP
	add	%l3, %l1, %l1		! highva += segsiz;
	cmp	%l1, %l2		! done?
	blu	0b			! no, loop
	 add	%l3, %l0, %l0		! (and lowva += segsz)
	b,a	startmap_done
2:
#endif /* SUN4 */

#if defined(SUN4M) || defined(SUN4D)
	cmp	%g4, CPU_SUN4M
	beq	3f
	 nop
	cmp	%g4, CPU_SUN4D
	bne	4f

3:
	/*
	 * The OBP guarantees us a 16MB mapping using a level 1 PTE at
	 * the start of the memory bank in which we were loaded. All we
	 * have to do is copy the entry.
	 * Also, we must check to see if we have a TI Viking in non-mbus mode,
	 * and if so do appropriate flipping and turning off traps before
	 * we dork with MMU passthrough.  -grrr
	 */

	sethi	%hi(0x40000000), %o1	! TI version bit
	rd	%psr, %o0
	andcc	%o0, %o1, %g0
	be	remap_notvik		! is non-TI normal MBUS module
	lda	[%g0] ASI_SRMMU, %o0	! load MMU
	andcc	%o0, 0x800, %g0
	bne	remap_notvik		! It is a viking MBUS module
	nop

	/*
	 * Ok, we have a non-Mbus TI Viking, a MicroSparc.
	 * In this scenerio, in order to play with the MMU
	 * passthrough safely, we need turn off traps, flip
	 * the AC bit on in the mmu status register, do our
	 * passthroughs, then restore the mmu reg and %psr
	 */
	rd	%psr, %o4		! saved here till done
	andn	%o4, 0x20, %o5
	wr	%o5, 0x0, %psr
	nop; nop; nop;
	set	SRMMU_CXTPTR, %o0
	lda	[%o0] ASI_SRMMU, %o0	! get context table ptr
	sll	%o0, 4, %o0		! make physical
	lda	[%g0] ASI_SRMMU, %o3	! hold mmu-sreg here
	/* 0x8000 is AC bit in Viking mmu-ctl reg */
	set	0x8000, %o2
	or	%o3, %o2, %o2
	sta	%o2, [%g0] ASI_SRMMU	! AC bit on

	lda	[%o0] ASI_BYPASS, %o1
	srl	%o1, 4, %o1
	sll	%o1, 8, %o1		! get phys addr of l1 entry
	lda	[%o1] ASI_BYPASS, %l4
	srl	%l1, 22, %o2		! note: 22 == RGSHIFT - 2
	add	%o1, %o2, %o1
	sta	%l4, [%o1] ASI_BYPASS

	sta	%o3, [%g0] ASI_SRMMU	! restore mmu-sreg
	wr	%o4, 0x0, %psr		! restore psr
	b,a	startmap_done

	/*
	 * The following is generic and should work on all
	 * Mbus based SRMMU's.
	 */
remap_notvik:
	set	SRMMU_CXTPTR, %o0
	lda	[%o0] ASI_SRMMU, %o0	! get context table ptr
	sll	%o0, 4, %o0		! make physical
	lda	[%o0] ASI_BYPASS, %o1
	srl	%o1, 4, %o1
	sll	%o1, 8, %o1		! get phys addr of l1 entry
	lda	[%o1] ASI_BYPASS, %l4
	srl	%l1, 22, %o2		! note: 22 == RGSHIFT - 2
	add	%o1, %o2, %o1
	sta	%l4, [%o1] ASI_BYPASS
	!b,a	startmap_done
4:
#endif /* SUN4M || SUN4D */
	! botch! We should blow up.

startmap_done:
	/*
	 * All set, fix pc and npc.  Once we are where we should be,
	 * we can give ourselves a stack and enable traps.
	 */
	set	1f, %g1
	jmp	%g1
	 nop
1:
	sethi	%hi(_C_LABEL(cputyp)), %o0	! what type of CPU we are on
	st	%g4, [%o0 + %lo(_C_LABEL(cputyp))]

	sethi	%hi(_C_LABEL(pgshift)), %o0	! pgshift = log2(nbpg)
	st	%g5, [%o0 + %lo(_C_LABEL(pgshift))]

	mov	1, %o0			! nbpg = 1 << pgshift
	sll	%o0, %g5, %g5
	sethi	%hi(_C_LABEL(nbpg)), %o0	! nbpg = bytes in a page
	st	%g5, [%o0 + %lo(_C_LABEL(nbpg))]

	sub	%g5, 1, %g5
	sethi	%hi(_C_LABEL(pgofset)), %o0 ! page offset = bytes in a page - 1
	st	%g5, [%o0 + %lo(_C_LABEL(pgofset))]

	rd	%psr, %g3		! paranoia: make sure ...
	andn	%g3, PSR_ET, %g3	! we have traps off
	wr	%g3, 0, %psr		! so that we can fiddle safely
	nop; nop; nop

	wr	%g0, 0, %wim		! make sure we can set psr
	nop; nop; nop
	wr	%g0, PSR_S|PSR_PS|PSR_PIL, %psr	! set initial psr
	 nop; nop; nop

	wr	%g0, 2, %wim		! set initial %wim (w1 invalid)
	mov	1, %g1			! set pcb_wim (log2(%wim) = 1)
	sethi	%hi(_C_LABEL(u0) + PCB_WIM), %g2
	st	%g1, [%g2 + %lo(_C_LABEL(u0) + PCB_WIM)]

	set	USRSTACK - CCFSZ, %fp	! as if called from user code
	set	estack0 - CCFSZ - 80, %sp ! via syscall(boot_me_up) or somesuch
	rd	%psr, %l0
	wr	%l0, PSR_ET, %psr
	nop; nop; nop

	/* Export actual trapbase */
	sethi	%hi(_C_LABEL(trapbase)), %o0
	st	%g6, [%o0+%lo(_C_LABEL(trapbase))]

#ifdef notdef
	/*
	 * Step 2: clear BSS.  This may just be paranoia; the boot
	 * loader might already do it for us; but what the hell.
	 */
	set	_edata, %o0		! bzero(edata, end - edata)
	set	_end, %o1
	call	_C_LABEL(bzero)
	 sub	%o1, %o0, %o1
#endif

	/*
	 * Stash prom vectors now, after bzero, as it lives in bss
	 * (which we just zeroed).
	 * This depends on the fact that bzero does not use %g7.
	 */
	sethi	%hi(_C_LABEL(romp)), %l0
	st	%g7, [%l0 + %lo(_C_LABEL(romp))]

	/*
	 * Step 3: compute number of windows and set up tables.
	 * We could do some of this later.
	 */
	save	%sp, -64, %sp
	rd	%psr, %g1
	restore
	and	%g1, 31, %g1		! want just the CWP bits
	add	%g1, 1, %o0		! compute nwindows
	sethi	%hi(_C_LABEL(nwindows)), %o1	! may as well tell everyone
	call	init_tables
	 st	%o0, [%o1 + %lo(_C_LABEL(nwindows))]

#if defined(SUN4) || defined(SUN4C)
	/*
	 * Some sun4/sun4c models have fewer than 8 windows. For extra
	 * speed, we do not need to save/restore those windows
	 * The save/restore code has 6 "save"'s followed by 6
	 * "restore"'s -- we "nop" out the last "save" and first
	 * "restore"
	 */
	cmp	%o0, 8
	be	1f
noplab:	 nop
	sethi	%hi(noplab), %l0
	ld	[%l0 + %lo(noplab)], %l1
	set	Lwb1, %l0
	st	%l1, [%l0 + 5*4]
	st	%l1, [%l0 + 6*4]
1:
#endif

#if (defined(SUN4) || defined(SUN4C)) && (defined(SUN4M) || defined(SUN4D))

	/*
	 * Patch instructions at specified labels that start
	 * per-architecture code-paths.
	 */
Lgandul:	nop

#define MUNGE(label) \
	sethi	%hi(label), %o0; \
	st	%l0, [%o0 + %lo(label)]

	sethi	%hi(Lgandul), %o0
	ld	[%o0 + %lo(Lgandul)], %l0	! %l0 = NOP

	cmp	%g4, CPU_SUN4M
	beq,a	2f
	 nop

	cmp	%g4, CPU_SUN4D
	bne,a	1f
	 nop

2:	! this should be automated!
	MUNGE(NOP_ON_4M_1)
	MUNGE(NOP_ON_4M_2)
	MUNGE(NOP_ON_4M_3)
	MUNGE(NOP_ON_4M_4)
	MUNGE(NOP_ON_4M_5)
	MUNGE(NOP_ON_4M_6)
	MUNGE(NOP_ON_4M_7)
	MUNGE(NOP_ON_4M_8)
	MUNGE(NOP_ON_4M_9)
	MUNGE(NOP_ON_4M_10)
	MUNGE(NOP_ON_4M_11)
	MUNGE(NOP_ON_4M_12)
	b,a	2f

1:
#if 0 /* currently there are no NOP_ON_4_4C_* */
	MUNGE(NOP_ON_4_4C_1)
#endif

2:

#undef MUNGE
#endif /* (SUN4 || SUN4C) && (SUN4M || SUN4D) */

	/*
	 * Step 4: change the trap base register, now that our trap handlers
	 * will function (they need the tables we just set up).
	 * This depends on the fact that memset does not use %g6.
	 */
	wr	%g6, 0, %tbr
	nop; nop; nop			! paranoia

	/* Clear `cpuinfo': memset(&cpuinfo, 0, sizeof cpuinfo) */
	sethi	%hi(CPUINFO_VA), %o0
	set	CPUINFO_STRUCTSIZE, %o2
	call	_C_LABEL(memset)
	 clr	%o1

	/*
	 * Initialize `cpuinfo' fields which are needed early.  Note
	 * we make the cpuinfo self-reference at the local VA for now.
	 * It may be changed to reference a global VA later.
	 */
	set	_C_LABEL(u0), %o0		! cpuinfo.curpcb = u0;
	sethi	%hi(cpcb), %l0
	st	%o0, [%l0 + %lo(cpcb)]

	sethi	%hi(CPUINFO_VA), %o0		! cpuinfo.ci_self = &cpuinfo;
	sethi	%hi(_CISELFP), %l0
	st	%o0, [%l0 + %lo(_CISELFP)]

	set	_C_LABEL(eintstack), %o0	! cpuinfo.eintstack= _eintstack;
	sethi	%hi(_EINTSTACKP), %l0
	st	%o0, [%l0 + %lo(_EINTSTACKP)]

	/*
	 * Ready to run C code; finish bootstrap.
	 */
	call	_C_LABEL(bootstrap)
	 nop

	/*
	 * Call main.  This returns to us after loading /sbin/init into
	 * user space.  (If the exec fails, main() does not return.)
	 */
	call	_C_LABEL(main)
	 clr	%o0			! our frame arg is ignored
	/*NOTREACHED*/

/*
 * Openfirmware entry point: openfirmware(void *args)
 */
ENTRY(openfirmware)
	sethi	%hi(_C_LABEL(romp)), %o1
	ld	[%o1 + %lo(_C_LABEL(romp))], %o2
	jmp	%o2
	 nop

#if defined(SUN4M) || defined(SUN4D)
/*
 * V8 multiply and divide routines, to be copied over the code
 * for the V6/V7 routines.  Seems a shame to spend the call, but....
 * Note: while .umul and .smul return a 64-bit result in %o1%o0,
 * gcc only really cares about the low 32 bits in %o0.  This is
 * really just gcc output, cleaned up a bit.
 */
	.globl	_C_LABEL(sparc_v8_muldiv)
_C_LABEL(sparc_v8_muldiv):
	save    %sp, -CCFSZ, %sp

#define	OVERWRITE(rtn, v8_rtn, len)	\
	set	v8_rtn, %o0;		\
	set	rtn, %o1;		\
	call	_C_LABEL(bcopy);	\
	 mov	len, %o2;		\
	/* now flush the insn cache */	\
	set	rtn, %o0;		\
	 mov	len, %o1;		\
0:					\
	flush	%o0;			\
	subcc	%o1, 8, %o1;		\
	bgu	0b;			\
	 add	%o0, 8, %o0;		\

	OVERWRITE(.mul,  v8_smul, .Lv8_smul_len)
	OVERWRITE(.umul, v8_umul, .Lv8_umul_len)
	OVERWRITE(.div,  v8_sdiv, .Lv8_sdiv_len)
	OVERWRITE(.udiv, v8_udiv, .Lv8_udiv_len)
	OVERWRITE(.rem,  v8_srem, .Lv8_srem_len)
	OVERWRITE(.urem, v8_urem, .Lv8_urem_len)
#undef	OVERWRITE
	ret
	 restore

v8_smul:
	retl
	 smul	%o0, %o1, %o0
.Lv8_smul_len = .-v8_smul
v8_umul:
	retl
	 umul	%o0, %o1, %o0
!v8_umul_len = 2 * 4
.Lv8_umul_len = .-v8_umul
v8_sdiv:
	sra	%o0, 31, %g2
	wr	%g2, 0, %y
	nop; nop; nop
	retl
	 sdiv	%o0, %o1, %o0
.Lv8_sdiv_len = .-v8_sdiv
v8_udiv:
	wr	%g0, 0, %y
	nop; nop; nop
	retl
	 udiv	%o0, %o1, %o0
.Lv8_udiv_len = .-v8_udiv
v8_srem:
	sra	%o0, 31, %g3
	wr	%g3, 0, %y
	nop; nop; nop
	sdiv	%o0, %o1, %g2
	smul	%g2, %o1, %g2
	retl
	 sub	%o0, %g2, %o0
.Lv8_srem_len = .-v8_srem
v8_urem:
	wr	%g0, 0, %y
	nop; nop; nop
	udiv	%o0, %o1, %g2
	smul	%g2, %o1, %g2
	retl
	 sub	%o0, %g2, %o0
.Lv8_urem_len = .-v8_urem

#endif /* SUN4M || SUN4D */

#if defined(MULTIPROCESSOR)
	/*
	 * Entry point for non-boot CPUs in MP systems.
	 */
	.globl	_C_LABEL(cpu_hatch)
_C_LABEL(cpu_hatch):
	rd	%psr, %g3		! paranoia: make sure ...
	andn	%g3, PSR_ET, %g3	! we have traps off
	wr	%g3, 0, %psr		! so that we can fiddle safely
	nop; nop; nop

	wr	%g0, 0, %wim		! make sure we can set psr
	nop; nop; nop
	wr	%g0, PSR_S|PSR_PS|PSR_PIL, %psr	! set initial psr
	nop; nop; nop

	wr	%g0, 2, %wim		! set initial %wim (w1 invalid)

	/* Initialize Trap Base register */
	sethi	%hi(_C_LABEL(trapbase)), %o0
	ld	[%o0+%lo(_C_LABEL(trapbase))], %g6
	wr	%g6, 0, %tbr
	nop; nop; nop			! paranoia

	/*
	 * Use this CPUs idlelwp's stack
	 */
	sethi	%hi(cpcb), %o0
	ld	[%o0 + %lo(cpcb)], %o0
	set	USPACE - 80 - CCFSZ, %sp
	add	%o0, %sp, %sp

	add	80, %sp, %fp

	/* Enable traps */
	rd	%psr, %l0
	wr	%l0, PSR_ET, %psr
	nop; nop

	/* Call C code */
	call	_C_LABEL(cpu_setup)
	 nop				! 3rd from above

	/* Enable interrupts */
	rd	%psr, %l0
	andn	%l0, PSR_PIL, %l0	! psr &= ~PSR_PIL;
	wr	%l0, 0, %psr		! (void) spl0();
	nop; nop; nop

	/* Wait for go_smp_cpus to go */
	set	_C_LABEL(go_smp_cpus), %l1
	ld	[%l1], %l0
1:
	cmp	%l0, %g0
	be	1b
	 ld	[%l1], %l0

	b	idle_loop
	 nop

#endif /* MULTIPROCESSOR */

#ifdef COMPAT_16
#include "sigcode_state.s"

	.globl	_C_LABEL(sigcode)
	.globl	_C_LABEL(esigcode)
_C_LABEL(sigcode):

	SAVE_STATE

	ldd	[%fp + 64], %o0		! sig, code
	ld	[%fp + 76], %o3		! arg3
	call	%g1			! (*sa->sa_handler)(sig,code,scp,arg3)
	 add	%fp, 64 + 16, %o2	! scp

	RESTORE_STATE

	! get registers back & set syscall #
	restore	%g0, SYS_compat_16___sigreturn14, %g1
	add	%sp, 64 + 16, %o0	! compute scp
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
	/* NOTREACHED */
_C_LABEL(esigcode):
#endif /* COMPAT_16 */


/*
 * Primitives
 */

/*
 * General-purpose NULL routine.
 */
ENTRY(sparc_noop)
	retl
	 nop

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
	mov	%o1, %o5		! save = toaddr;
	tst	%o2			! maxlen == 0?
	beq,a	Lcstoolong		! yes, return ENAMETOOLONG
	 sethi	%hi(cpcb), %o4

	set	KERNBASE, %o4
	cmp	%o0, %o4		! fromaddr < KERNBASE?
	blu	Lcsdocopy		! yes, go do it
	 sethi	%hi(cpcb), %o4		! (first instr of copy)

	b	Lcsdone			! no, return EFAULT
	 mov	EFAULT, %o0

/*
 * copyoutstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 */
ENTRY(copyoutstr)
	! %o0 = fromaddr, %o1 = toaddr, %o2 = maxlen, %o3 = &lencopied
	mov	%o1, %o5		! save = toaddr;
	tst	%o2			! maxlen == 0?
	beq,a	Lcstoolong		! yes, return ENAMETOOLONG
	 sethi	%hi(cpcb), %o4

	set	KERNBASE, %o4
	cmp	%o1, %o4		! toaddr < KERNBASE?
	blu	Lcsdocopy		! yes, go do it
	 sethi	%hi(cpcb), %o4		! (first instr of copy)

	b	Lcsdone			! no, return EFAULT
	 mov	EFAULT, %o0

Lcsdocopy:
!	sethi	%hi(cpcb), %o4		! (done earlier)
	ld	[%o4 + %lo(cpcb)], %o4	! catch faults
	set	Lcsdone, %g1
	st	%g1, [%o4 + PCB_ONFAULT]

! XXX should do this in bigger chunks when possible
0:					! loop:
	ldsb	[%o0], %g1		!	c = *fromaddr;
	tst	%g1
	stb	%g1, [%o1]		!	*toaddr++ = c;
	be	1f			!	if (c == NULL)
	 inc	%o1			!		goto ok;
	deccc	%o2			!	if (--len > 0) {
	bgu	0b			!		fromaddr++;
	 inc	%o0			!		goto loop;
					!	}
Lcstoolong:				!
	b	Lcsdone			!	error = ENAMETOOLONG;
	 mov	ENAMETOOLONG, %o0	!	goto done;
1:					! ok:
	clr	%o0			!    error = 0;
Lcsdone:				! done:
	sub	%o1, %o5, %o1		!	len = to - save;
	tst	%o3			!	if (lencopied)
	bnz,a	3f
	 st	%o1, [%o3]		!		*lencopied = len;
3:
	retl				! cpcb->pcb_onfault = 0;
	 st	%g0, [%o4 + PCB_ONFAULT]! return (error);

/*
 * copystr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from one point to another in
 * the kernel address space.  (This is a leaf procedure, but
 * it does not seem that way to the C compiler.)
 */
ENTRY(copystr)
	mov	%o1, %o5		!	to0 = to;
	tst	%o2			! if (maxlength == 0)
	beq,a	2f			!
	 mov	ENAMETOOLONG, %o0	!	ret = ENAMETOOLONG; goto done;

0:					! loop:
	ldsb	[%o0], %o4		!	c = *from;
	tst	%o4
	stb	%o4, [%o1]		!	*to++ = c;
	be	1f			!	if (c == 0)
	 inc	%o1			!		goto ok;
	deccc	%o2			!	if (--len > 0) {
	bgu,a	0b			!		from++;
	 inc	%o0			!		goto loop;
	b	2f			!	}
	 mov	ENAMETOOLONG, %o0	!	ret = ENAMETOOLONG; goto done;
1:					! ok:
	clr	%o0			!	ret = 0;
2:
	sub	%o1, %o5, %o1		!	len = to - to0;
	tst	%o3			!	if (lencopied)
	bnz,a	3f
	 st	%o1, [%o3]		!		*lencopied = len;
3:
	retl
	 nop

/*
 * Copyin(src, dst, len)
 *
 * Copy specified amount of data from user space into the kernel.
 */
ENTRY(copyin)
	set	KERNBASE, %o3
	cmp	%o0, %o3		! src < KERNBASE?
	blu,a	Ldocopy			! yes, can try it
	 sethi	%hi(cpcb), %o3

	/* source address points into kernel space: return EFAULT */
	retl
	 mov	EFAULT, %o0

/*
 * Copyout(src, dst, len)
 *
 * Copy specified amount of data from kernel to user space.
 * Just like copyin, except that the `dst' addresses are user space
 * rather than the `src' addresses.
 */
ENTRY(copyout)
	set	KERNBASE, %o3
	cmp	%o1, %o3		! dst < KERBASE?
	blu,a	Ldocopy
	 sethi	%hi(cpcb), %o3

	/* destination address points into kernel space: return EFAULT */
	retl
	 mov	EFAULT, %o0

	/*
	 * ******NOTE****** this depends on bcopy() not using %g7
	 */
Ldocopy:
!	sethi	%hi(cpcb), %o3
	ld	[%o3 + %lo(cpcb)], %o3
	set	Lcopyfault, %o4
	mov	%o7, %g7		! save return address
	call	_C_LABEL(bcopy)		! bcopy(src, dst, len)
	 st	%o4, [%o3 + PCB_ONFAULT]

	sethi	%hi(cpcb), %o3
	ld	[%o3 + %lo(cpcb)], %o3
	st	%g0, [%o3 + PCB_ONFAULT]
	jmp	%g7 + 8
	 clr	%o0			! return 0

! Copyin or copyout fault.  Clear cpcb->pcb_onfault.
! The return value was already put in %o0 by the fault handler.
! Note that although we were in bcopy, there is no state to clean up;
! the only special thing is that we have to return to [g7 + 8] rather than
! [o7 + 8].
Lcopyfault:
	sethi	%hi(cpcb), %o3
	ld	[%o3 + %lo(cpcb)], %o3
	jmp	%g7 + 8
	 st	%g0, [%o3 + PCB_ONFAULT]


/*
 * Write all user windows presently in the CPU back to the user's stack.
 * We just do `save' instructions until pcb_uw == 0.
 *
 *	p = cpcb;
 *	nsaves = 0;
 *	while (p->pcb_uw > 0)
 *		save(), nsaves++;
 *	while (--nsaves >= 0)
 *		restore();
 */
ENTRY(write_user_windows)
	sethi	%hi(cpcb), %g6
	ld	[%g6 + %lo(cpcb)], %g6
	b	2f
	 clr	%g5
1:
	save	%sp, -64, %sp
2:
	ld	[%g6 + PCB_UW], %g7
	tst	%g7
	bg,a	1b
	 inc	%g5
3:
	deccc	%g5
	bge,a	3b
	 restore
	retl
	 nop

/*
 * cpu_switchto() runs an lwp, saving the current one away.
 */
ENTRY(cpu_switchto)
	/*
	 * Register Usage:
	 *	%g1 = oldlwp (return value)
	 *	%g2 = psr
	 *	%g3 = newlwp
	 *	%g5 = newpcb
	 *	%l1 = oldpsr (excluding ipl bits)
	 *	%l6 = %hi(cpcb)
	 *	%o0 = tmp 1
	 *	%o1 = tmp 2
	 *	%o2 = tmp 3
	 *	%o3 = vmspace->vm_pmap
	 */
	save	%sp, -CCFSZ, %sp
	mov	%i0, %g1			! save oldlwp
	mov	%i1, %g3			! and newlwp

	sethi	%hi(cpcb), %l6

	tst	%i0				! if (oldlwp == NULL)
	bz	Lnosaveoldlwp
	 rd	%psr, %l1			! psr = %psr;

	ld	[%l6 + %lo(cpcb)], %o0

	std	%i6, [%o0 + PCB_SP]		! cpcb->pcb_<sp,pc> = <fp,pc>;

	st	%l1, [%o0 + PCB_PSR]		! cpcb->pcb_pcb = psr

	/*
	 * Save the old process: write back all windows (excluding
	 * the current one).  XXX crude; knows nwindows <= 8
	 */
#define	SAVE save %sp, -64, %sp
Lwb1:	SAVE; SAVE; SAVE; SAVE; SAVE; SAVE;	/* 6 of each: */
	restore; restore; restore; restore; restore; restore

Lnosaveoldlwp:
	andn	%l1, PSR_PIL, %l1		! oldpsr &= ~PSR_PIL;

	/*
	 * Load the new process.  To load, we must change stacks and
	 * and alter cpcb. We must also load the CWP and WIM from the
	 * new process' PCB, since, when we finally return from
	 * the trap, the CWP of the trap window must match the
	 * CWP stored in the trap frame.
	 *
	 * Once the new CWP is set below our local registers become
	 * invalid, so, we use globals at that point for any values
	 * we need afterwards.
	 */

	ld	[%g3 + L_PCB], %g5	! newpcb
	ld	[%g5 + PCB_PSR], %g2    ! cwpbits = newpcb->pcb_psr;

	/* traps off while we switch to the new stack */
	wr	%l1, (IPL_SCHED << 8) | PSR_ET, %psr

	/* set new cpcb, and curlwp */
	sethi	%hi(curlwp), %l7
	st	%g5, [%l6 + %lo(cpcb)]		! cpcb = newpcb;
	st      %g3, [%l7 + %lo(curlwp)]        ! curlwp = l;

	/* compute new wim */
	ld	[%g5 + PCB_WIM], %o0
	mov	1, %o1
	sll	%o1, %o0, %o0
	wr	%o0, 0, %wim		! %wim = 1 << newpcb->pcb_wim;

	/* now must not change %psr for 3 more instrs */
	/* Clear FP & CP enable bits, as well as the PIL field */
/*1,2*/	set     PSR_EF|PSR_EC|PSR_PIL, %o0
/*3*/	andn    %g2, %o0, %g2           ! newpsr &= ~(PSR_EF|PSR_EC|PSR_PIL);
	/* set new psr, but with traps disabled */
	wr      %g2, (IPL_SCHED << 8)|PSR_ET, %psr ! %psr = newpsr ^ PSR_ET;
	/* load new stack and return address */
	ldd	[%g5 + PCB_SP], %i6	! <fp,pc> = newpcb->pcb_<sp,pc>
	add	%fp, -CCFSZ, %sp	! set stack frame for this window

#ifdef DEBUG
	mov	%g5, %o0
	SET_SP_REDZONE(%o0, %o1)
	CHECK_SP_REDZONE(%o0, %o1)
#endif

	/* finally, enable traps and continue at splsched() */
	wr      %g2, IPL_SCHED << 8 , %psr      ! psr = newpsr;

	/*
	 * Now running p.  
	 */
#if defined(MULTIPROCESSOR)
	ld	[%g3 + L_PROC], %o2	! p = l->l_proc;
	ld	[%o2 + P_VMSPACE], %o3	! vm = p->p_vmspace;
	ld	[%o3 + VM_PMAP], %o4	! pm = vm->vm_map.vm_pmap;
	/* Add this CPU to the pmap's CPU set */
	sethi	%hi(CPUINFO_VA + CPUINFO_CPUNO), %o0
	ld	[%o0 + %lo(CPUINFO_VA + CPUINFO_CPUNO)], %o1
	mov	1, %o2
	ld	[%o4 + PMAP_CPUSET], %o0
	sll	%o2, %o1, %o2
	or	%o0, %o2, %o0		! pm->pm_cpuset |= cpu_number();
	st	%o0, [%o4 + PMAP_CPUSET]
#endif

	ret
	 restore %g0, %g1, %o0		! return (lastproc)

/*
 * Call the idlespin() function if it exists, otherwise just return.
 */
ENTRY(cpu_idle)
	sethi	%hi(CPUINFO_VA), %o0
	ld	[%o0 + CPUINFO_IDLESPIN], %o1
	tst	%o1
	bz	1f
	 nop
	jmp	%o1
	 nop	! CPUINFO_VA is already in %o0
1:
	retl
	 nop

/*
 * Snapshot the current process so that stack frames are up to date.
 * Only used just before a crash dump.
 */
ENTRY(snapshot)
	std	%o6, [%o0 + PCB_SP]	! save sp
	rd	%psr, %o1		! save psr
	st	%o1, [%o0 + PCB_PSR]

	/*
	 * Just like switch(); same XXX comments apply.
	 * 7 of each.  Minor tweak: the 7th restore is
	 * done after a ret.
	 */
	SAVE; SAVE; SAVE; SAVE; SAVE; SAVE; SAVE
	restore; restore; restore; restore; restore; restore; ret; restore


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

	! newlwp in %l2, oldlwp already in %o0
	call	lwp_startup
	 mov	%l2, %o1

	call	%l0
	 mov	%l1, %o0

	/* 
	 * Here we finish up as in syscall, but simplified.
	 * cpu_lwp_fork() (or sendsig(), if we took a pending signal
	 * in child_return()) will have set the user-space return
	 * address in tf_pc. In both cases, %npc should be %pc + 4.
	 */
	rd      %psr, %l2
	ld	[%sp + CCFSZ + 4], %l1	! pc = tf->tf_pc from cpu_lwp_fork()
	and	%l2, PSR_CWP, %o1	! keep current CWP
	or	%o1, PSR_S, %l0		! user psr
	b	return_from_syscall
	 add	%l1, 4, %l2		! npc = pc+4

/*
 * {fu,su}{,i}{byte,word}
 */
_ENTRY(fuiword)
ENTRY(fuword)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE...
	bgeu	Lfsbadaddr
	 .empty
	btst	3, %o0			! or has low bits set...
	bnz	Lfsbadaddr		!	go return -1
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfserr;
	ld	[%o2 + %lo(cpcb)], %o2
	set	Lfserr, %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	ld	[%o0], %o0		! fetch the word
	retl				! phew, made it, return the word
	 st	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

Lfserr:
	st	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
Lfsbadaddr:
	retl				! and return error indicator
	 mov	-1, %o0

	/*
	 * This is just like Lfserr, but it's a global label that allows
	 * mem_access_fault() to check to see that we don't want to try to
	 * page in the fault.  It's used by fuswintr() etc.
	 */
	.globl	_C_LABEL(Lfsbail)
_C_LABEL(Lfsbail):
	st	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	retl				! and return error indicator
	 mov	-1, %o0

	/*
	 * Like fusword but callable from interrupt context.
	 * Fails if data isn't resident.
	 */
ENTRY(fuswintr)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE
	bgeu	Lfsbadaddr		!	return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfsbail;
	ld	[%o2 + %lo(cpcb)], %o2
	set	_C_LABEL(Lfsbail), %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	lduh	[%o0], %o0		! fetch the halfword
	retl				! made it
	st	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

ENTRY(fusword)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE
	bgeu	Lfsbadaddr		!	return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfserr;
	ld	[%o2 + %lo(cpcb)], %o2
	set	Lfserr, %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	lduh	[%o0], %o0		! fetch the halfword
	retl				! made it
	st	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

_ENTRY(fuibyte)
ENTRY(fubyte)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE
	bgeu	Lfsbadaddr		!	return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfserr;
	ld	[%o2 + %lo(cpcb)], %o2
	set	Lfserr, %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	ldub	[%o0], %o0		! fetch the byte
	retl				! made it
	st	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault

_ENTRY(suiword)
ENTRY(suword)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE ...
	bgeu	Lfsbadaddr
	 .empty
	btst	3, %o0			! or has low bits set ...
	bnz	Lfsbadaddr		!	go return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfserr;
	ld	[%o2 + %lo(cpcb)], %o2
	set	Lfserr, %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	st	%o1, [%o0]		! store the word
	st	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	clr	%o0

ENTRY(suswintr)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE
	bgeu	Lfsbadaddr		!	go return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfsbail;
	ld	[%o2 + %lo(cpcb)], %o2
	set	_C_LABEL(Lfsbail), %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	sth	%o1, [%o0]		! store the halfword
	st	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	clr	%o0

ENTRY(susword)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE
	bgeu	Lfsbadaddr		!	go return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfserr;
	ld	[%o2 + %lo(cpcb)], %o2
	set	Lfserr, %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	sth	%o1, [%o0]		! store the halfword
	st	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	clr	%o0

_ENTRY(suibyte)
ENTRY(subyte)
	set	KERNBASE, %o2
	cmp	%o0, %o2		! if addr >= KERNBASE
	bgeu	Lfsbadaddr		!	go return error
	 .empty
	sethi	%hi(cpcb), %o2		! cpcb->pcb_onfault = Lfserr;
	ld	[%o2 + %lo(cpcb)], %o2
	set	Lfserr, %o3
	st	%o3, [%o2 + PCB_ONFAULT]
	stb	%o1, [%o0]		! store the byte
	st	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	retl				! and return 0
	clr	%o0

/* probeget and probeset are meant to be used during autoconfiguration */

/*
 * probeget(addr, size) void *addr; int size;
 *
 * Read or write a (byte,word,longword) from the given address.
 * Like {fu,su}{byte,halfword,word} but our caller is supposed
 * to know what he is doing... the address can be anywhere.
 *
 * We optimize for space, rather than time, here.
 */
ENTRY(probeget)
	! %o0 = addr, %o1 = (1,2,4)
	sethi	%hi(cpcb), %o2
	ld	[%o2 + %lo(cpcb)], %o2	! cpcb->pcb_onfault = Lfserr;
	set	Lfserr, %o5
	st	%o5, [%o2 + PCB_ONFAULT]
	btst	1, %o1
	bnz,a	0f			! if (len & 1)
	 ldub	[%o0], %o0		!	value = *(char *)addr;
0:	btst	2, %o1
	bnz,a	0f			! if (len & 2)
	 lduh	[%o0], %o0		!	value = *(short *)addr;
0:	btst	4, %o1
	bnz,a	0f			! if (len & 4)
	 ld	[%o0], %o0		!	value = *(int *)addr;
0:	retl				! made it, clear onfault and return
	 st	%g0, [%o2 + PCB_ONFAULT]

/*
 * probeset(addr, size, val) void *addr; int size, val;
 *
 * As above, but we return 0 on success.
 */
ENTRY(probeset)
	! %o0 = addr, %o1 = (1,2,4), %o2 = val
	sethi	%hi(cpcb), %o3
	ld	[%o3 + %lo(cpcb)], %o3	! cpcb->pcb_onfault = Lfserr;
	set	Lfserr, %o5
	st	%o5, [%o3 + PCB_ONFAULT]
	btst	1, %o1
	bnz,a	0f			! if (len & 1)
	 stb	%o2, [%o0]		!	*(char *)addr = value;
0:	btst	2, %o1
	bnz,a	0f			! if (len & 2)
	 sth	%o2, [%o0]		!	*(short *)addr = value;
0:	btst	4, %o1
	bnz,a	0f			! if (len & 4)
	 st	%o2, [%o0]		!	*(int *)addr = value;
0:	clr	%o0			! made it, clear onfault and return 0
	retl
	 st	%g0, [%o3 + PCB_ONFAULT]

/*
 * int xldcontrolb(void *, pcb)
 *		    %o0     %o1
 *
 * read a byte from the specified address in ASI_CONTROL space.
 */
ENTRY(xldcontrolb)
	!sethi	%hi(cpcb), %o2
	!ld	[%o2 + %lo(cpcb)], %o2	! cpcb->pcb_onfault = Lfsbail;
	or	%o1, %g0, %o2		! %o2 = %o1
	set	_C_LABEL(Lfsbail), %o5
	st	%o5, [%o2 + PCB_ONFAULT]
	lduba	[%o0] ASI_CONTROL, %o0	! read
0:	retl
	 st	%g0, [%o2 + PCB_ONFAULT]

/*
 * int fkbyte(void *, pcb)
 *	      %o0      %o1
 *
 * Just like fubyte(), but for kernel space.
 * (currently used to work around unexplained transient bus errors
 *  when reading the VME interrupt vector)
 */
ENTRY(fkbyte)
	or	%o1, %g0, %o2		! %o2 = %o1
	set	_C_LABEL(Lfsbail), %o5
	st	%o5, [%o2 + PCB_ONFAULT]
	ldub	[%o0], %o0		! fetch the byte
	retl				! made it
	 st	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault


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
	b	1f
	deccc	8, %o2
0:
	std	%o4, [%o1 + %o2]
	deccc	8, %o2
1:
	bge,a	0b
	ldd	[%o0 + %o2], %o4
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
	clr	%g1
0:
	deccc	8, %o1			! while ((n =- 8) >= 0)
	bge,a	0b
	std	%g0, [%o0 + %o1]	!	*(quad *)(addr + n) = 0;
	retl
	nop

/*
 * kernel bcopy
 * Assumes regions do not overlap; has no useful return value.
 *
 * Must not use %g7 (see copyin/copyout above).
 */

#define	BCOPY_SMALL	32	/* if < 32, copy by bytes */

ENTRY(bcopy)
	cmp	%o2, BCOPY_SMALL
Lbcopy_start:
	bge,a	Lbcopy_fancy	! if >= this many, go be fancy.
	btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 .empty
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
	/* NOTREACHED */

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lbcopy_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 .empty
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
	/* NOTREACHED */

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
	/* NOTREACHED */

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
	/* NOTREACHED */

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
	ldd	[%o0], %o4	! do {
	std	%o4, [%o1]	!	*(double *)dst = *(double *)src;
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
	/* NOTREACHED */

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
	 .empty
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
 * kcopy() is exactly like bcopy except that it set pcb_onfault such that
 * when a fault occurs, it is able to return -1 to indicate this to the
 * caller.
 */
ENTRY(kcopy)
	sethi	%hi(cpcb), %o5		! cpcb->pcb_onfault = Lkcerr;
	ld	[%o5 + %lo(cpcb)], %o5
	set	Lkcerr, %o3
	ld	[%o5 + PCB_ONFAULT], %g1! save current onfault handler
	st	%o3, [%o5 + PCB_ONFAULT]

	cmp	%o2, BCOPY_SMALL
Lkcopy_start:
	bge,a	Lkcopy_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 .empty
0:
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	inc	%o0
	stb	%o4, [%o1]
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	st	%g1, [%o5 + PCB_ONFAULT]	! restore onfault
	retl
	 mov	0, %o0		! delay slot: return success
	/* NOTREACHED */

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lkcopy_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 .empty
	btst	7, %o1
	be,a	Lkcopy_doubles
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
	inc	%o0		!		*dst++ = *src++;
	stb	%o4, [%o1]
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 inc	%o1
	st	%g1, [%o5 + PCB_ONFAULT]	! restore onfault
	retl
	 mov	0, %o0		! delay slot: return success
	/* NOTREACHED */

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
	/* NOTREACHED */

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
	/* NOTREACHED */

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
	! swap %o4 with %o2 during doubles copy, since %o5 is verboten
	mov     %o2, %o4
Lkcopy_doubles2:
	ldd	[%o0], %o2	! do {
	inc	8, %o0		!	dst += 8, src += 8;
	std	%o2, [%o1]	!	*(double *)dst = *(double *)src;
	deccc	8, %o4		! } while ((len -= 8) >= 0);
	bge	Lkcopy_doubles2
	 inc	8, %o1
	mov	%o4, %o2	! restore len

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lkcopy_done	!	goto bcopy_done;

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
	st	%g1, [%o5 + PCB_ONFAULT]! restore onfault
	retl
	 mov	0, %o0		! delay slot: return success
	/* NOTREACHED */

	! mop up trailing byte (if present).
Lkcopy_mopb:
	bne,a	1f
	 ldsb	[%o0], %o4

Lkcopy_done:
	st	%g1, [%o5 + PCB_ONFAULT]	! restore onfault
	retl
	 mov	0, %o0		! delay slot: return success
	/* NOTREACHED */

1:
	stb	%o4, [%o1]
	st	%g1, [%o5 + PCB_ONFAULT]	! restore onfault
	retl
	 mov	0, %o0		! delay slot: return success
	/* NOTREACHED */

Lkcerr:
	retl
	 st	%g1, [%o5 + PCB_ONFAULT]	! restore onfault
	/* NOTREACHED */

/*
 * savefpstate(struct fpstate *f);
 * ipi_savefpstate(struct fpstate *f);
 *
 * Store the current FPU state.  The first `st %fsr' may cause a trap;
 * our trap handler knows how to recover (by `returning' to savefpcont).
 *
 * The IPI version just deals with updating event counters first.
 */
Lpanic_savefpstate:
	.asciz	"cpu%d: NULL fpstate"
	_ALIGN

ENTRY(ipi_savefpstate)
	sethi	%hi(CPUINFO_VA), %o5
	ldd	[%o5 + CPUINFO_SAVEFPSTATE], %o2
	inccc   %o3
	addx    %o2, 0, %o2
	std	%o2, [%o5 + CPUINFO_SAVEFPSTATE]

ENTRY(savefpstate)
	cmp	%o0, 0
	bz	Lfp_null_fpstate
	 nop
	rd	%psr, %o1		! enable FP before we begin
	set	PSR_EF, %o2
	or	%o1, %o2, %o1
	wr	%o1, 0, %psr
	/* do some setup work while we wait for PSR_EF to turn on */
	set	FSR_QNE, %o5		! QNE = 0x2000, too big for immediate
	clr	%o3			! qsize = 0;
	nop				! (still waiting for PSR_EF)
special_fp_store:
	st	%fsr, [%o0 + FS_FSR]	! f->fs_fsr = getfsr();
	/*
	 * Even if the preceding instruction did not trap, the queue
	 * is not necessarily empty: this state save might be happening
	 * because user code tried to store %fsr and took the FPU
	 * from `exception pending' mode to `exception' mode.
	 * So we still have to check the blasted QNE bit.
	 * With any luck it will usually not be set.
	 */
	ld	[%o0 + FS_FSR], %o2	! if (f->fs_fsr & QNE)
	btst	%o5, %o2
	bnz	Lfp_storeq		!	goto storeq;
	 std	%f0, [%o0 + FS_REGS + (4*0)]	! f->fs_f0 = etc;
Lfp_finish:
	st	%o3, [%o0 + FS_QSIZE]	! f->fs_qsize = qsize;
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
	retl
	 std	%f30, [%o0 + FS_REGS + (4*30)]

/*
 * We really should panic here but while we figure out what the bug is
 * that a remote CPU gets a NULL struct fpstate *, this lets the system
 * work at least seemingly stably.
 */
Lfp_null_fpstate:
#if 1
	sethi	%hi(CPUINFO_VA), %o5
	ldd	[%o5 + CPUINFO_SAVEFPSTATE_NULL], %o2
	inccc   %o3
	addx    %o2, 0, %o2
	retl
	 std	%o2, [%o5 + CPUINFO_SAVEFPSTATE_NULL]
#else
	ld	[%o5 + CPUINFO_CPUNO], %o1
	sethi	%hi(Lpanic_savefpstate), %o0
	call	_C_LABEL(panic)
	 or	%o0, %lo(Lpanic_savefpstate), %o0
#endif

/*
 * Store the (now known nonempty) FP queue.
 * We have to reread the fsr each time in order to get the new QNE bit.
 */
Lfp_storeq:
	add	%o0, FS_QUEUE, %o1	! q = &f->fs_queue[0];
1:
	std	%fq, [%o1 + %o3]	! q[qsize++] = fsr_qfront();
	st	%fsr, [%o0 + FS_FSR]	! reread fsr
	ld	[%o0 + FS_FSR], %o4	! if fsr & QNE, loop
	btst	%o5, %o4
	bnz	1b
	 inc	8, %o3
	st	%o2, [%o0 + FS_FSR]	! fs->fs_fsr = original_fsr
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
	 st	%fsr, [%o0 + FS_FSR]	! but first finish the %fsr store

/*
 * Load FPU state.
 */
ENTRY(loadfpstate)
	rd	%psr, %o1		! enable FP before we begin
	set	PSR_EF, %o2
	or	%o1, %o2, %o1
	wr	%o1, 0, %psr
	nop; nop; nop			! paranoia
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
	retl
	 ld	[%o0 + FS_FSR], %fsr	! setfsr(f->fs_fsr);

/*
 * ienab_bis(bis) int bis;
 * ienab_bic(bic) int bic;
 *
 * Set and clear bits in the sun4/sun4c interrupt register.
 */

#if defined(SUN4) || defined(SUN4C)
/*
 * Since there are no read-modify-write instructions for this,
 * and one of the interrupts is nonmaskable, we must disable traps.
 */
ENTRY(ienab_bis)
	! %o0 = bits to set
	rd	%psr, %o2
	wr	%o2, PSR_ET, %psr	! disable traps
	nop; nop			! 3-instr delay until ET turns off
	sethi	%hi(INTRREG_VA), %o3
	ldub	[%o3 + %lo(INTRREG_VA)], %o4
	or	%o4, %o0, %o4		! *INTRREG_VA |= bis;
	stb	%o4, [%o3 + %lo(INTRREG_VA)]
	wr	%o2, 0, %psr		! reenable traps
	nop
	retl
	 nop

ENTRY(ienab_bic)
	! %o0 = bits to clear
	rd	%psr, %o2
	wr	%o2, PSR_ET, %psr	! disable traps
	nop; nop
	sethi	%hi(INTRREG_VA), %o3
	ldub	[%o3 + %lo(INTRREG_VA)], %o4
	andn	%o4, %o0, %o4		! *INTRREG_VA &=~ bic;
	stb	%o4, [%o3 + %lo(INTRREG_VA)]
	wr	%o2, 0, %psr		! reenable traps
	nop
	retl
	 nop
#endif	/* SUN4 || SUN4C */

#if defined(SUN4M)
/*
 * raise(cpu, level)
 */
ENTRY(raise)
#if !defined(MSIIEP) /* normal suns */
	! *(ICR_PI_SET + cpu*_MAXNBPG) = PINTR_SINTRLEV(level)
	sethi	%hi(1 << 16), %o2
	sll	%o2, %o1, %o2
	set	ICR_PI_SET, %o1
	set	_MAXNBPG, %o3
1:
	subcc	%o0, 1, %o0
	bpos,a	1b
	 add	%o1, %o3, %o1
	retl
	 st	%o2, [%o1]
#else /* MSIIEP - ignore %o0, only one CPU ever */
	mov	1, %o2
	xor	%o1, 8, %o1	! change 'endianness' of the shift distance
	sethi	%hi(MSIIEP_PCIC_VA), %o0
	sll	%o2, %o1, %o2
	retl
	 sth	%o2, [%o0 + PCIC_SOFT_INTR_SET_REG]
#endif

/*
 * Read Synchronous Fault Status registers.
 * On entry: %l1 == PC, %l3 == fault type, %l4 == storage, %l7 == return address
 * Only use %l5 and %l6.
 * Note: not C callable.
 */
_ENTRY(_C_LABEL(srmmu_get_syncflt))
_ENTRY(_C_LABEL(hypersparc_get_syncflt))
	set	SRMMU_SFAR, %l5
	lda	[%l5] ASI_SRMMU, %l5	! sync virt addr; must be read first
	st	%l5, [%l4 + 4]		! => dump.sfva
	set	SRMMU_SFSR, %l5
	lda	[%l5] ASI_SRMMU, %l5	! get sync fault status register
	jmp	%l7 + 8			! return to caller
	 st	%l5, [%l4]		! => dump.sfsr

_ENTRY(_C_LABEL(viking_get_syncflt))
_ENTRY(_C_LABEL(ms1_get_syncflt))
_ENTRY(_C_LABEL(swift_get_syncflt))
_ENTRY(_C_LABEL(turbosparc_get_syncflt))
_ENTRY(_C_LABEL(cypress_get_syncflt))
	cmp	%l3, T_TEXTFAULT
	be,a	1f
	 mov	%l1, %l5		! use PC if type == T_TEXTFAULT

	set	SRMMU_SFAR, %l5
	lda	[%l5] ASI_SRMMU, %l5	! sync virt addr; must be read first
1:
	st	%l5, [%l4 + 4]		! => dump.sfva

	set	SRMMU_SFSR, %l5
	lda	[%l5] ASI_SRMMU, %l5	! get sync fault status register
	jmp	%l7 + 8			! return to caller
	 st	%l5, [%l4]		! => dump.sfsr

#if defined(MULTIPROCESSOR) && 0 /* notyet */
/*
 * Read Synchronous Fault Status registers.
 * On entry: %o0 == &sfsr, %o1 == &sfar
 */
_ENTRY(_C_LABEL(smp_get_syncflt))
	save    %sp, -CCFSZ, %sp

	sethi	%hi(CPUINFO_VA), %o4
	ld	[%l4 + %lo(CPUINFO_VA+CPUINFO_GETSYNCFLT)], %o5
	clr	%l1
	clr	%l3
	jmpl	%o5, %l7
	 or	%o4, %lo(CPUINFO_SYNCFLTDUMP), %l4

	! load values out of the dump
	ld	[%o4 + %lo(CPUINFO_VA+CPUINFO_SYNCFLTDUMP)], %o5
	st	%o5, [%i0]
	ld	[%o4 + %lo(CPUINFO_VA+CPUINFO_SYNCFLTDUMP+4)], %o5
	st	%o5, [%i1]
	ret
	 restore
#endif /* MULTIPROCESSOR */

/*
 * Read Asynchronous Fault Status registers.
 * On entry: %o0 == &afsr, %o1 == &afar
 * Return 0 if async register are present.
 */
_ENTRY(_C_LABEL(srmmu_get_asyncflt))
	set	SRMMU_AFAR, %o4
	lda	[%o4] ASI_SRMMU, %o4	! get async fault address
	set	SRMMU_AFSR, %o3	!
	st	%o4, [%o1]
	lda	[%o3] ASI_SRMMU, %o3	! get async fault status
	st	%o3, [%o0]
	retl
	 clr	%o0			! return value

_ENTRY(_C_LABEL(cypress_get_asyncflt))
_ENTRY(_C_LABEL(hypersparc_get_asyncflt))
	set	SRMMU_AFSR, %o3		! must read status before fault on HS
	lda	[%o3] ASI_SRMMU, %o3	! get async fault status
	st	%o3, [%o0]
	btst	AFSR_AFO, %o3		! and only read fault address
	bz	1f			! if valid.
	set	SRMMU_AFAR, %o4
	lda	[%o4] ASI_SRMMU, %o4	! get async fault address
	clr	%o0			! return value
	retl
	 st	%o4, [%o1]
1:
	retl
	 clr	%o0			! return value

_ENTRY(_C_LABEL(no_asyncflt_regs))
	retl
	 mov	1, %o0			! return value

_ENTRY(_C_LABEL(hypersparc_pure_vcache_flush))
	/*
	 * Flush entire on-chip instruction cache, which is
	 * a pure vitually-indexed/virtually-tagged cache.
	 */
	retl
	 sta	%g0, [%g0] ASI_HICACHECLR

#endif /* SUN4M */


/*
 * delay function
 *
 * void delay(N)  -- delay N microseconds
 *
 * Register usage: %o0 = "N" number of usecs to go (counts down to zero)
 *		   %o1 = "timerblurb" (stays constant)
 *		   %o2 = counter for 1 usec (counts down from %o1 to zero)
 *
 */

ENTRY(delay)			! %o0 = n
	subcc	%o0, %g0, %g0
	be	2f

	sethi	%hi(_C_LABEL(timerblurb)), %o1
	ld	[%o1 + %lo(_C_LABEL(timerblurb))], %o1	! %o1 = timerblurb

	 addcc	%o1, %g0, %o2		! %o2 = cntr (start @ %o1), clear CCs
					! first time through only

					! delay 1 usec
1:	bne	1b			! come back here if not done
	 subcc	%o2, 1, %o2		! %o2 = %o2 - 1 [delay slot]

	subcc	%o0, 1, %o0		! %o0 = %o0 - 1
	bne	1b			! done yet?
	 addcc	%o1, %g0, %o2		! reinit %o2 and CCs  [delay slot]
					! harmless if not branching
2:
	retl				! return
	 nop				! [delay slot]


/*
 * void __cpu_simple_lock(__cpu_simple_lock_t *alp)
 */
ENTRY_NOPROFILE(__cpu_simple_lock)
0:
	ldstub	[%o0], %o1
	tst	%o1
	bnz,a	2f
	 ldub	[%o0], %o1
1:
	retl
	 .empty
2:
	set	0x1000000, %o2	! set spinout counter
3:
	tst	%o1
	bz	0b		! lock has been released; try again
	deccc	%o2
	bcc,a	3b		! repeat until counter < 0
	 ldub	[%o0], %o1

	! spun out; check if already panicking
	sethi	%hi(_C_LABEL(panicstr)), %o2
	ld	[%o2 + %lo(_C_LABEL(panicstr))], %o1
	tst	%o1
	! if so, just take the lock and return on the assumption that
	! in panic mode we're running on a single CPU anyway.
	bnz,a	1b
	 ldstub	[%o0], %g0

	! set up stack frame and call panic
	save	%sp, -CCFSZ, %sp
	sethi	%hi(CPUINFO_VA + CPUINFO_CPUNO), %o0
	ld	[%o0 + %lo(CPUINFO_VA + CPUINFO_CPUNO)], %o1
	mov	%i0, %o2
	sethi	%hi(Lpanic_spunout), %o0
	call	_C_LABEL(panic)
	or	%o0, %lo(Lpanic_spunout), %o0

Lpanic_spunout:
	.asciz	"cpu%d: stuck on lock@%x"
	_ALIGN

#if defined(KGDB) || defined(DDB) || defined(DIAGNOSTIC)
/*
 * Write all windows (user or otherwise), except the current one.
 *
 * THIS COULD BE DONE IN USER CODE
 */
ENTRY(write_all_windows)
	/*
	 * g2 = g1 = nwindows - 1;
	 * while (--g1 > 0) save();
	 * while (--g2 > 0) restore();
	 */
	sethi	%hi(_C_LABEL(nwindows)), %g1
	ld	[%g1 + %lo(_C_LABEL(nwindows))], %g1
	dec	%g1
	mov	%g1, %g2

1:	deccc	%g1
	bg,a	1b
	 save	%sp, -64, %sp

2:	deccc	%g2
	bg,a	2b
	 restore

	retl
	nop
#endif /* KGDB */

ENTRY(setjmp)
	st	%sp, [%o0+0]	! stack pointer
	st	%o7, [%o0+4]	! return pc
	st	%fp, [%o0+8]	! frame pointer
	retl
	 clr	%o0

Lpanic_ljmp:
	.asciz	"longjmp botch"
	_ALIGN

ENTRY(longjmp)
	addcc	%o1, %g0, %g6	! compute v ? v : 1 in a global register
	be,a	0f
	 mov	1, %g6
0:
	mov	%o0, %g1	! save a in another global register
	ld	[%g1+8], %g7	/* get caller's frame */
1:
	cmp	%fp, %g7	! compare against desired frame
	bl,a	1b		! if below,
	 restore		!    pop frame and loop
	be,a	2f		! if there,
	 ldd	[%g1+0], %o2	!    fetch return %sp and pc, and get out

Llongjmpbotch:
				! otherwise, went too far; bomb out
	save	%sp, -CCFSZ, %sp	/* preserve current window */
	sethi	%hi(Lpanic_ljmp), %o0
	call	_C_LABEL(panic)
	or %o0, %lo(Lpanic_ljmp), %o0;
	unimp	0

2:
	cmp	%o2, %sp	! %sp must not decrease
	bge,a	3f
	 mov	%o2, %sp	! it is OK, put it in place
	b,a	Llongjmpbotch
3:
	jmp	%o3 + 8		! success, return %g6
	 mov	%g6, %o0

	.data
	.globl	_C_LABEL(kernel_top)
_C_LABEL(kernel_top):
	.word	0
	.globl	_C_LABEL(bootinfo)
_C_LABEL(bootinfo):
	.word	0

	.comm	_C_LABEL(nwindows), 4
	.comm	_C_LABEL(romp), 4
