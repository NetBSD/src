/*	$NetBSD: cpu.h,v 1.37 2000/03/28 03:11:26 simonb Exp $	*/

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
 *	@(#)cpu.h	8.4 (Berkeley) 1/4/94
 */

#ifndef _CPU_H_
#define _CPU_H_

/*
 * Exported definitions unique to NetBSD/mips cpu support.
 */

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_BOOTED_KERNEL	2	/* string: booted kernel name */
#define CPU_ROOT_DEVICE		3	/* string: root device name */
#define CPU_MAXID		4	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "root_device", CTLTYPE_STRING }, \
}


#ifdef _KERNEL
#ifndef _LOCORE

/*
 * Macros to find the CPU architecture we're on at run-time,
 * or if possible, at compile-time.
 */

#if (MIPS1 + MIPS3) == 1
#ifdef MIPS1
# define CPUISMIPS3	0
#endif /* mips1 */

#ifdef MIPS3
#  define CPUISMIPS3	 1
#endif /* mips1 */

#else /* run-time test */
extern int cpu_arch;
#define CPUISMIPS3	(cpu_arch == 3)
#endif /* run-time test */

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_wait(p)			/* nothing */
#define	cpu_swapout(p)			panic("cpu_swapout: can't get here");
#define	cpu_number()			0

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.
 */
struct clockframe {
	int	pc;	/* program counter at time of interrupt */
	int	sr;	/* status register at time of interrupt */
};

/*
 * A port must provde CLKF_USERMODE() and CLKF_BASEPRI() for use
 * in machine-independent code. These differ on r4000 and r3000 systems;
 * provide them in the port-dependent file that includes this one, using
 * the macros below.
 */

/* mips1 versions */
#define	MIPS1_CLKF_USERMODE(framep)	((framep)->sr & MIPS_SR_KU_PREV)
#define	MIPS1_CLKF_BASEPRI(framep)	\
	((~(framep)->sr & (MIPS_INT_MASK | MIPS_SR_INT_ENA_PREV)) == 0)

/* mips3 versions */
#define	MIPS3_CLKF_USERMODE(framep)	((framep)->sr & MIPS_SR_KSU_USER)
#define	MIPS3_CLKF_BASEPRI(framep)	\
	((~(framep)->sr & (MIPS_INT_MASK | MIPS_SR_INT_IE)) == 0)

#define	CLKF_PC(framep)		((framep)->pc)
#define	CLKF_INTR(framep)	(0)

#if defined(MIPS3) && !defined(MIPS1)
#define	CLKF_USERMODE(framep)	MIPS3_CLKF_USERMODE(framep)
#define	CLKF_BASEPRI(framep)	MIPS3_CLKF_BASEPRI(framep)
#endif

#if !defined(MIPS3) && defined(MIPS1)
#define	CLKF_USERMODE(framep)	MIPS1_CLKF_USERMODE(framep)
#define	CLKF_BASEPRI(framep)	MIPS1_CLKF_BASEPRI(framep)
#endif


#if defined(MIPS3) && defined(MIPS1)
#define CLKF_USERMODE(framep) \
    ((CPUISMIPS3) ? MIPS3_CLKF_USERMODE(framep):  MIPS1_CLKF_USERMODE(framep))
#define CLKF_BASEPRI(framep) \
    ((CPUISMIPS3) ? MIPS3_CLKF_BASEPRI(framep):  MIPS1_CLKF_BASEPRI(framep))
#endif


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched()	{ want_resched = 1; aston(); }

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the MIPS, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	{ (p)->p_flag |= P_OWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

#define aston()		(astpending = 1)

extern int astpending;	/* need to trap before returning to user mode */
extern int want_resched;	/* resched() was called */
#ifdef MIPS3
extern u_int	mips_L2CacheSize;
extern int	mips_L2CacheIsSnooping; /* L2 cache snoops uncached writes ? */
extern int	mips_L2CacheMixed;

#ifdef MIPS3_INTERNAL_TIMER_INTERRUPT
extern u_int32_t mips3_intr_cycle_count;
extern u_int32_t mips3_timer_delta;
#endif
#endif /* MIPS3 */

/*
 * Misc prototypes and variable declarations.
 */
struct proc;
struct user;

extern struct proc *fpcurproc;

/* trap.c */
void	child_return __P((void *));
int	kdbpeek __P((vaddr_t));

/* mips_machdep.c */
void	dumpsys __P((void));
int	savectx __P((struct user *));
void	mips_init_msgbuf __P((void));

/* locore.S */
void	savefpregs __P((struct proc *));
void	switchfpregs __P((struct proc *, struct proc *));
int	badaddr __P((void *, size_t));

/* mips_machdep.c */
void	cpu_identify __P((void));
void	mips_vector_init __P((void));

#endif /* ! _LOCORE */
#endif /* _KERNEL */

#endif /* _CPU_H_ */
