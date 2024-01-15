/* $NetBSD: cpu.h,v 1.40 2024/01/15 02:16:52 thorpej Exp $ */

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

#ifndef _MACHINE_CPU_H
#define _MACHINE_CPU_H

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>

#if defined(_KERNEL)

#define M68K_MMU_MOTOROLA

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the luna68k, we use
 * what the locore.s glue puts on the stack before calling C-code.
 */
struct clockframe {
	u_int	cf_regs[4];	/* d0,d1,a0,a1 */
	u_short	cf_sr;		/* sr at time of interrupt */
	u_long	cf_pc;		/* pc at time of interrupt */
	u_short	cf_vo;		/* vector offset (4-word frame) */
} __attribute__((packed));

#define CLKF_USERMODE(framep)	(((framep)->cf_sr & PSL_S) == 0)
#define CLKF_PC(framep)		((framep)->cf_pc)
#if 0
/* We would like to do it this way... */
#define CLKF_INTR(framep)	(((framep)->cf_sr & PSL_M) == 0)
#else
/* but until we start using PSL_M, we have to do this instead */
#define CLKF_INTR(framep)	(0)	/* XXX */
#endif


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci,l,flags)	do {	\
	__USE(flags);				\
	aston();				\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the luna68k, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define cpu_need_proftick(l)	{ (l)->l_pflag |= LP_OWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define cpu_signotify(l)	aston()

#define aston()		(astpending = 1)

extern int	astpending;	/* need to trap before returning to user mode */

#endif /* _KERNEL */

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
void	dumpsys(void);

/* machdep.c functions */
int	badaddr(void *, int);

#endif

#endif /* _MACHINE_CPU_H */
