/*	$NetBSD: cpu.h,v 1.26.2.1 2007/05/27 12:27:51 ad Exp $	*/

/*
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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

#ifndef _NEWS68K_CPU_H_
#define _NEWS68K_CPU_H_

#if defined(_KERNEL)

/*
 * Exported definitions unique to news68k cpu support.
 */

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>

/*
 * XXX news1700 L2 cache would be corrupted with DC_BE and IC_BE...
 * XXX Should these be defined in machine/cpu.h?
 */
#undef CACHE_ON
#undef CACHE_CLR
#undef IC_CLEAR
#undef DC_CLEAR
#define CACHE_ON	(DC_WA|DC_CLR|DC_ENABLE|IC_CLR|IC_ENABLE)
#define CACHE_CLR	CACHE_ON
#define IC_CLEAR	(DC_WA|DC_ENABLE|IC_CLR|IC_ENABLE)
#define DC_CLEAR	(DC_WA|DC_CLR|DC_ENABLE|IC_ENABLE)

#define DCIC_CLR	(DC_CLR|IC_CLR)
#define CACHE_BE	(DC_BE|IC_BE)

/*
 * Get interrupt glue.
 */
#include <machine/intr.h>

#include <sys/cpu_data.h>
struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	int	ci_mtx_count;
	int	ci_mtx_oldspl;
	int	ci_want_resched;
};

extern struct cpu_info cpu_info_store;

#define	curcpu()			(&cpu_info_store)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define cpu_swapin(p)			/* nothing */
#define cpu_swapout(p)			/* nothing */
#define cpu_number()			0

void	cpu_proc_fork(struct proc *, struct proc *);

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the hp300, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	vo;		/* vector offset (4-word frame) */
};

#define CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
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
#define cpu_need_resched(ci, flags)	\
	do { ci->ci_want_resched = 1; aston(); } while (/* CONSTCOND */0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define cpu_need_proftick(l)	\
	do { (l)->l_flag |= LP_OWEUPC; aston(); } while (/* CONSTCOND */0)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define cpu_signotify(l)	aston()

extern int astpending;		/* need to trap before returning to user mode */
extern volatile u_char *ctrl_ast;
#define aston()		\
	do { astpending++; *ctrl_ast = 0xff; } while (/* CONSTCOND */0)

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

#ifdef _KERNEL

#if defined(news1700) || defined(news1200)
#ifndef M68030
#define M68030
#endif
#define M68K_MMU_MOTOROLA
#endif

#if defined(news1700)
#define CACHE_HAVE_PAC
#endif

#endif

#ifdef _KERNEL
extern int systype;
#define NEWS1700	0
#define NEWS1200	1

extern int cpuspeed;
extern char *intiobase, *intiolimit, *extiobase;
extern u_int intiobase_phys, intiotop_phys;
extern u_int extiobase_phys, extiotop_phys;
extern u_int intrcnt[];

extern void (*vectab[])(void);

struct frame;
struct fpframe;

/* locore.s functions */
void m68881_save(struct fpframe *);
void m68881_restore(struct fpframe *);

int suline(void *, void *);
void loadustp(int);
void badtrap(void);
void intrhand_vectored(void);
int getsr(void);


void doboot(int)
	__attribute__((__noreturn__));
void nmihand(struct frame *);
void ecacheon(void);
void ecacheoff(void);

/* machdep.c functions */
int badaddr(void *, int);
int badbaddr(void *);

#endif

/* physical memory sections */
#define ROMBASE		0xe0000000

#define INTIOBASE1700	0xe0c00000
#define INTIOTOP1700	0xe1d00000 /* XXX */
#define EXTIOBASE1700	0xf0f00000
#define EXTIOTOP1700	0xf1000000 /* XXX */
#define CTRL_POWER1700	0xe1380000
#define CTRL_LED1700	0xe0dc0000

#define INTIOBASE1200	0xe1000000
#define INTIOTOP1200	0xe1d00000 /* XXX */
#define EXTIOBASE1200	0xe4000000
#define EXTIOTOP1200	0xe4020000 /* XXX */
#define CTRL_POWER1200	0xe1000000
#define CTRL_LED1200	0xe1500001

#define MAXADDR		0xfffff000

/*
 * Internal IO space:
 *
 * Internal IO space is mapped in the kernel from ``intiobase'' to
 * ``intiolimit'' (defined in locore.s).  Since it is always mapped,
 * conversion between physical and kernel virtual addresses is easy.
 */
#define ISIIOVA(va) \
	((char *)(va) >= intiobase && (char *)(va) < intiolimit)
#define IIOV(pa)	(((u_int)(pa) - intiobase_phys) + (u_int)intiobase)
#define ISIIOPA(pa) \
	((u_int)(pa) >= intiobase_phys && (u_int)(pa) < intiotop_phys)
#define IIOP(va)	(((u_int)(va) - (u_int)intiobase) + intiobase_phys)
#define IIOPOFF(pa)	((u_int)(pa) - intiobase_phys)

/* XXX EIO space mapping should be modified like hp300 XXX */
#define	EIOSIZE		(extiotop_phys - extiobase_phys)
#define ISEIOVA(va) \
	((char *)(va) >= extiobase && (char *)(va) < (char *)EIOSIZE)
#define EIOV(pa)	(((u_int)(pa) - extiobase_phys) + (u_int)extiobase)

#if defined(CACHE_HAVE_PAC) || defined(CACHE_HAVE_VAC)
#define M68K_CACHEOPS_MACHDEP
#endif

#ifdef CACHE_HAVE_PAC
#define M68K_CACHEOPS_MACHDEP_PCIA
#endif

#ifdef CACHE_HAVE_VAC
#define M68K_CACHEOPS_MACHDEP_DCIA
#define M68K_CACHEOPS_MACHDEP_DCIS
#define M68K_CACHEOPS_MACHDEP_DCIU
#endif

#endif /* !_NEWS68K_CPU_H_ */
