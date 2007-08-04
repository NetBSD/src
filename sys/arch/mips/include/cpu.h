/*	$NetBSD: cpu.h,v 1.82.8.1 2007/08/04 12:33:01 jmcneill Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)cpu.h	8.4 (Berkeley) 1/4/94
 */

#ifndef _CPU_H_
#define _CPU_H_

#include <mips/cpuregs.h>

/*
 * Exported definitions unique to NetBSD/mips cpu support.
 */

#ifdef _KERNEL
#ifndef _LOCORE
#include <sys/cpu_data.h>

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	cpuid_t	ci_cpuid;
	u_long ci_cpu_freq;		/* CPU frequency */
	u_long ci_cycles_per_hz;	/* CPU freq / hz */
	u_long ci_divisor_delay;	/* for delay/DELAY */
	u_long ci_divisor_recip;	/* scaled reciprocal of previous;
					   see below */
	struct lwp *ci_curlwp;		/* currently running lwp */
	struct lwp *ci_fpcurlwp;	/* the current FPU owner */
	int ci_want_resched;		/* user preemption pending */
	int ci_mtx_count;		/* negative count of held mutexes */
	int ci_mtx_oldspl;		/* saved SPL value */
};

/*
 * To implement a more accurate microtime using the CP0 COUNT register
 * we need to divide that register by the number of cycles per MHz.
 * But...
 *
 * DIV and DIVU are expensive on MIPS (eg 75 clocks on the R4000).  MULT
 * and MULTU are only 12 clocks on the same CPU.
 *
 * The strategy we use is to calculate the reciprical of cycles per MHz,
 * scaled by 1<<32.  Then we can simply issue a MULTU and pluck of the
 * HI register and have the results of the division.
 */
#define	MIPS_SET_CI_RECIPRICAL(cpu)					\
do {									\
	KASSERT((cpu)->ci_divisor_delay != 0);				\
	(cpu)->ci_divisor_recip = 0x100000000ULL / (cpu)->ci_divisor_delay; \
} while (0)

#define	MIPS_COUNT_TO_MHZ(cpu, count, res)				\
	__asm volatile("multu %1,%2 ; mfhi %0"				\
	    : "=r"((res)) : "r"((count)), "r"((cpu)->ci_divisor_recip))

#endif /* !_LOCORE */
#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_BOOTED_KERNEL	2	/* string: booted kernel name */
#define CPU_ROOT_DEVICE		3	/* string: root device name */
#define CPU_LLSC		4	/* OS/CPU supports LL/SC instruction */

/*
 * Platform can override, but note this breaks userland compatibility
 * with other mips platforms.
 */
#ifndef CPU_MAXID
#define CPU_MAXID		5	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "root_device", CTLTYPE_STRING }, \
	{ "llsc", CTLTYPE_INT }, \
}
#endif

#ifdef _KERNEL
#ifdef _LKM
/* Assume all CPU architectures are valid for LKM's */
#define	MIPS1	1
#define	MIPS3	1
#define	MIPS4	1
#define	MIPS32	1
#define	MIPS64	1
#endif

#if (MIPS1 + MIPS3 + MIPS4 + MIPS32 + MIPS64) == 0
#error at least one of MIPS1, MIPS3, MIPS4, MIPS32 or MIPS64 must be specified
#endif

/* Shortcut for MIPS3 or above defined */
#if defined(MIPS3) || defined(MIPS4) || defined(MIPS32) || defined(MIPS64)
#define	MIPS3_PLUS	1
#else
#undef MIPS3_PLUS
#endif

/*
 * Macros to find the CPU architecture we're on at run-time,
 * or if possible, at compile-time.
 */

#define	CPU_ARCH_MIPSx	0		/* XXX unknown */
#define	CPU_ARCH_MIPS1	(1 << 0)
#define	CPU_ARCH_MIPS2	(1 << 1)
#define	CPU_ARCH_MIPS3	(1 << 2)
#define	CPU_ARCH_MIPS4	(1 << 3)
#define	CPU_ARCH_MIPS5	(1 << 4)
#define	CPU_ARCH_MIPS32	(1 << 5)
#define	CPU_ARCH_MIPS64	(1 << 6)

/* Note: must be kept in sync with -ffixed-?? Makefile.mips. */
#define MIPS_CURLWP             $23
#define MIPS_CURLWP_QUOTED      "$23"
#define MIPS_CURLWP_CARD	23
#define	MIPS_CURLWP_FRAME(x)	FRAME_S7(x)

#ifndef _LOCORE

extern struct cpu_info cpu_info_store;
register struct lwp *mips_curlwp asm(MIPS_CURLWP_QUOTED);

#define	curlwp			mips_curlwp
#define	curcpu()		(curlwp->l_cpu)
#define	curpcb			((struct pcb *)curlwp->l_addr)
#define	fpcurlwp		(curcpu()->ci_fpcurlwp)
#define	cpu_number()		(0)
#define	cpu_proc_fork(p1, p2)

/* XXX simonb
 * Should the following be in a cpu_info type structure?
 * And how many of these are per-cpu vs. per-system?  (Ie,
 * we can assume that all cpus have the same mmu-type, but
 * maybe not that all cpus run at the same clock speed.
 * Some SGI's apparently support R12k and R14k in the same
 * box.)
 */
extern int cpu_arch;
extern int mips_cpu_flags;
extern int mips_has_r4k_mmu;
extern int mips_has_llsc;
extern int mips3_pg_cached;
extern u_int mips3_pg_shift;

#define	CPU_MIPS_R4K_MMU		0x0001
#define	CPU_MIPS_NO_LLSC		0x0002
#define	CPU_MIPS_CAUSE_IV		0x0004
#define	CPU_MIPS_HAVE_SPECIAL_CCA	0x0008	/* Defaults to '3' if not set. */
#define	CPU_MIPS_CACHED_CCA_MASK	0x0070
#define	CPU_MIPS_CACHED_CCA_SHIFT	 4
#define	CPU_MIPS_DOUBLE_COUNT		0x0080	/* 1 cp0 count == 2 clock cycles */
#define	CPU_MIPS_USE_WAIT		0x0100	/* Use "wait"-based cpu_idle() */
#define	CPU_MIPS_NO_WAIT		0x0200	/* Inverse of previous, for mips32/64 */
#define	CPU_MIPS_D_CACHE_COHERENT	0x0400	/* D-cache is fully coherent */
#define	CPU_MIPS_I_D_CACHE_COHERENT	0x0800	/* I-cache funcs don't need to flush the D-cache */
#define	MIPS_NOT_SUPP			0x8000

#endif	/* !_LOCORE */

#if ((MIPS1 + MIPS3 + MIPS4 + MIPS32 + MIPS64) == 1) || defined(_LOCORE)

#if defined(MIPS1)

# define CPUISMIPS3		0
# define CPUIS64BITS		0
# define CPUISMIPS32		0
# define CPUISMIPS64		0
# define CPUISMIPSNN		0
# define MIPS_HAS_R4K_MMU	0
# define MIPS_HAS_CLOCK		0
# define MIPS_HAS_LLSC		0

#elif defined(MIPS3) || defined(MIPS4)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS64		0
# define CPUISMIPSNN		0
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# if defined(_LOCORE)
#  if !defined(MIPS3_5900) && !defined(MIPS3_4100)
#   define MIPS_HAS_LLSC	1
#  else
#   define MIPS_HAS_LLSC	0
#  endif
# else	/* _LOCORE */
#  define MIPS_HAS_LLSC		(mips_has_llsc)
# endif	/* _LOCORE */

#elif defined(MIPS32)

# define CPUISMIPS3		1
# define CPUIS64BITS		0
# define CPUISMIPS32		1
# define CPUISMIPS64		0
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1

#elif defined(MIPS64)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS64		1
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1

#endif

#else /* run-time test */

#ifndef	_LOCORE

#define	MIPS_HAS_R4K_MMU	(mips_has_r4k_mmu)
#define	MIPS_HAS_LLSC		(mips_has_llsc)

/* This test is ... rather bogus */
#define	CPUISMIPS3	((cpu_arch & \
	(CPU_ARCH_MIPS3 | CPU_ARCH_MIPS4 | CPU_ARCH_MIPS32 | CPU_ARCH_MIPS64)) != 0)

/* And these aren't much better while the previous test exists as is... */
#define	CPUISMIPS32	((cpu_arch & CPU_ARCH_MIPS32) != 0)
#define	CPUISMIPS64	((cpu_arch & CPU_ARCH_MIPS64) != 0)
#define	CPUISMIPSNN	((cpu_arch & (CPU_ARCH_MIPS32 | CPU_ARCH_MIPS64)) != 0)
#define	CPUIS64BITS	((cpu_arch & \
	(CPU_ARCH_MIPS3 | CPU_ARCH_MIPS4 | CPU_ARCH_MIPS64)) != 0)

#define	MIPS_HAS_CLOCK	(cpu_arch >= CPU_ARCH_MIPS3)

#else	/* !_LOCORE */

#define	MIPS_HAS_LLSC	0

#endif	/* !_LOCORE */

#endif /* run-time test */

#ifndef	_LOCORE

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapout(p)			panic("cpu_swapout: can't get here");

void cpu_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.
 */
struct clockframe {
	int	pc;	/* program counter at time of interrupt */
	int	sr;	/* status register at time of interrupt */
	int	ppl;	/* previous priority level at time of interrupt */
};

/*
 * A port must provde CLKF_USERMODE() for use in machine-independent code.
 * These differ on r4000 and r3000 systems; provide them in the
 * port-dependent file that includes this one, using the macros below.
 */

/* mips1 versions */
#define	MIPS1_CLKF_USERMODE(framep)	((framep)->sr & MIPS_SR_KU_PREV)

/* mips3 versions */
#define	MIPS3_CLKF_USERMODE(framep)	((framep)->sr & MIPS_SR_KSU_USER)

#define	CLKF_PC(framep)		((framep)->pc)
#define	CLKF_INTR(framep)	(0)

#if defined(MIPS3_PLUS) && !defined(MIPS1)		/* XXX bogus! */
#define	CLKF_USERMODE(framep)	MIPS3_CLKF_USERMODE(framep)
#endif

#if !defined(MIPS3_PLUS) && defined(MIPS1)		/* XXX bogus! */
#define	CLKF_USERMODE(framep)	MIPS1_CLKF_USERMODE(framep)
#endif

#if defined(MIPS3_PLUS) && defined(MIPS1)		/* XXX bogus! */
#define CLKF_USERMODE(framep) \
    ((CPUISMIPS3) ? MIPS3_CLKF_USERMODE(framep):  MIPS1_CLKF_USERMODE(framep))
#endif

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	PROC_PC(p)							\
	(((struct frame *)(p)->p_md.md_regs)->f_regs[37])	/* XXX PC */

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
void	cpu_need_resched(struct cpu_info *, int);

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the MIPS, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)						\
do {									\
	(l)->l_pflag |= LP_OWEUPC;					\
	aston(l);							\
} while (/*CONSTCOND*/0)

/*
 * Notify the current lwp (l) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	aston(l)

#define aston(l)		((l)->l_md.md_astpending = 1)

/*
 * Misc prototypes and variable declarations.
 */
struct lwp;
struct user;

extern struct segtab *segbase;	/* current segtab base */

/* trap.c */
void	netintr(void);
int	kdbpeek(vaddr_t);

/* mips_machdep.c */
void	dumpsys(void);
int	savectx(struct user *);
void	mips_init_msgbuf(void);
void	savefpregs(struct lwp *);
void	loadfpregs(struct lwp *);

/* locore*.S */
int	badaddr(void *, size_t);
int	badaddr64(uint64_t, size_t);

/* mips_machdep.c */
void	cpu_identify(void);
void	mips_vector_init(void);

#endif /* ! _LOCORE */
#endif /* _KERNEL */
#endif /* _CPU_H_ */
