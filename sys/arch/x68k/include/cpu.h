/*	$NetBSD: cpu.h,v 1.58 2014/07/31 14:41:19 isaki Exp $	*/

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

#ifndef _X68K_CPU_H_
#define	_X68K_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_m68k_arch.h"
#include "opt_lockdebug.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>

#if defined(_KERNEL)
/*
 * Exported definitions unique to x68k/68k cpu support.
 */
#define	M68K_MMU_MOTOROLA

/*
 * Get interrupt glue.
 */
#include <machine/intr.h>

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the x68k, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	vo;		/* vector offset (4-word frame) */
};

#define	CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->pc)
#if 0
/* We would like to do it this way... */
#define	CLKF_INTR(framep)	(((framep)->sr & PSL_M) == 0)
#else
/* but until we start using PSL_M, we have to do this instead */
#define	CLKF_INTR(framep)	(0)	/* XXX */
#endif


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci,flags)	do {	\
	__USE(flags); 				\
	ci->ci_want_resched = 1;		\
	aston();				\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the x68k, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)	\
	do { (l)->l_pflag |= LP_OWEUPC; aston(); } while (/* CONSTCOND */ 0)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	aston()

extern int astpending;		/* need to trap before returning to user mode */
#define aston() (astpending++)

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_MAXID		2	/* number of valid machdep ids */

/*
 * The rest of this should probably be moved to <machine/x68kcpu.h>
 * although some of it could probably be put into generic 68k headers.
 */

#ifdef _KERNEL
extern uint8_t *intiobase;
extern uint8_t *intiolimit;

/* machdep.c functions */
void	dumpsys(void);

/* locore.s functions */
int	suline(void *, void *);
void	loadustp(int);

/* machdep.c functions */
int	badaddr(volatile void*);
int	badbaddr(volatile void*);

#endif

/* physical memory sections */
#define INTIOBASE	(0x00C00000)
#define INTIOTOP	(0x01000000)

/*
 * Internal IO space:
 *
 * Ranges from 0xC00000 to 0x1000000 (IIOMAPSIZE).
 *
 * Internal IO space is mapped in the kernel from ``IODEVbase'' to
 * ``intiolimit'' (defined in locore.s).  Since it is always mapped,
 * conversion between physical and kernel virtual addresses is easy.
 */
#define	IIOV(pa)	((u_int)(pa) - INTIOBASE + (u_int)intiobase)
#define	IIOP(va)	((u_int)(va) - (u_int)intiobase + INTIOBASE)
#define	IIOPOFF(pa)	((int)(pa)-INTIOBASE)
#define	IIOMAPSIZE	btoc(INTIOTOP-INTIOBASE)	/* 4mb */

#endif /* _X68K_CPU_H_ */
