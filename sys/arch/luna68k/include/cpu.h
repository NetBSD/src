/* $NetBSD: cpu.h,v 1.4 2000/12/19 21:09:56 scw Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _MACHINE_CPU_H
#define _MACHINE_CPU_H

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_lockdebug.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>
#define M68K_MMU_MOTOROLA

#include <sys/sched.h>
struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};

#ifdef _KERNEL
extern struct cpu_info cpu_info_store;

#define	curcpu()			(&cpu_info_store)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define cpu_swapin(p)			/* nothing */
#define cpu_wait(p)			/* nothing */
#define cpu_swapout(p)			/* nothing */
#define cpu_number()			0

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the luna68k, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	vo;		/* vector offset (4-word frame) */
};

#define CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define CLKF_BASEPRI(framep)	(((framep)->sr & PSL_IPL) == 0)
#define CLKF_PC(framep)		((framep)->pc)
#if 0
/* We would like to do it this way... */
#define CLKF_INTR(framep)	(((framep)->sr & PSL_M) == 0)
#else
/* but until we start using PSL_M, we have to do this instead */
#define CLKF_INTR(framep)	(0)	/* XXX */
#endif


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define need_resched(ci)	{ want_resched = 1; aston(); }

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define need_proftick(p)	{ (p)->p_flag |= P_OWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define signotify(p)	aston()

#define aston()		(astpending = 1)

extern int	astpending;	/* need to trap before returning to user mode */
extern int	want_resched;	/* resched() was called */

/*
 * simulated software interrupt register
 */
extern unsigned char ssir;

#define SIR_NET		0x1
#define SIR_CLOCK	0x2

#define siron(x)	\
	__asm __volatile ("orb %0,%1" : : "di" ((u_char)(x)), "g" (ssir))
#define siroff(x)	\
	__asm __volatile ("andb %0,%1" : : "di" ((u_char)~(x)), "g" (ssir))

#define setsoftnet()	siron(SIR_NET)
#define setsoftclock()	siron(SIR_CLOCK)

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_MAXID		2	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

/*
 * Values for machtype
 */
#define LUNA_I		1
#define LUNA_II		2

#ifdef _KERNEL
extern	int machtype;
extern	char *intiobase, *intiolimit;		/* XXX */
extern	u_int intiobase_phys, intiotop_phys;	/* XXX */

/* machdep.c functions */
void	dumpconf __P((void));
void	dumpsys __P((void));

/* locore.s functions */
struct pcb;
struct fpframe;
int	suline __P((caddr_t, caddr_t));
void	savectx __P((struct pcb *));
void	switch_exit __P((struct proc *));
void	proc_trampoline __P((void));
void	loadustp __P((int));
void	m68881_save __P((struct fpframe *));
void	m68881_restore __P((struct fpframe *));

/* machdep.c functions */
int	badaddr __P((caddr_t, int));

/* sys_machdep.c functions */
int	cachectl1 __P((unsigned long, vaddr_t, size_t, struct proc *));
int	dma_cachectl __P((caddr_t, int));

/* vm_machdep.c functions */
void	physaccess __P((caddr_t, caddr_t, int, int));
void	physunaccess __P((caddr_t, int));
int	kvtop __P((caddr_t));

#endif

#endif /* _MACHINE_CPU_H */
