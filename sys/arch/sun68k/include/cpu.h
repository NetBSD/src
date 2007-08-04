/*	$NetBSD: cpu.h,v 1.18 2007/08/04 09:49:53 ad Exp $	*/

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	from: Utah Hdr: cpu.h 1.16 91/03/25
 *	from: @(#)cpu.h	7.7 (Berkeley) 6/27/91
 *	cpu.h,v 1.2 1993/05/22 07:58:17 cgd Exp
 */

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	from: Utah Hdr: cpu.h 1.16 91/03/25
 *	from: @(#)cpu.h	7.7 (Berkeley) 6/27/91
 *	cpu.h,v 1.2 1993/05/22 07:58:17 cgd Exp
 */

#ifndef _CPU_H_
#define _CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

#include <m68k/cpu.h>

#ifdef _KERNEL

/*
 * External definitions unique to sun68k cpu support.
 * These are the "public" declarations - those needed in
 * machine-independent source code.  The "private" ones
 * are in machdep.h (used only inside sys/arch/sun{2,3,68k}).
 *
 * Note that the name of this file is NOT meant to imply
 * that it has anything to do with Motorola CPU stuff.
 * The name "cpu" is historical, and used in the common
 * code to identify machine-dependent functions, etc.
 */

#include <sys/cpu_data.h>
struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	cpuid_t	ci_cpuid;
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
#define	cpu_number()			0
#define	cpu_swapin(p)			/* nothing */
#define	cpu_swapout(p)			/* nothing */

void	cpu_proc_fork(struct proc *, struct proc *);

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the sun68k, we use
 * what the locore.s glue puts on the stack before calling C-code.
 */
struct clockframe {
	u_int	cf_regs[4];	/* d0,d1,a0,a1 */
	u_short	cf_sr;		/* sr at time of interrupt */
	u_long	cf_pc;		/* pc at time of interrupt */
	u_short	cf_vo;		/* vector offset (4-word frame) */
} __attribute__((packed));

#define	CLKF_USERMODE(framep)	(((framep)->cf_sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->cf_pc)
#if 0
/* We would like to do it this way... */
#define	CLKF_INTR(framep)	(((framep)->cf_sr & PSL_M) == 0)
#else
/* but until we start using PSL_M, we have to do this instead */
#define	CLKF_INTR(framep)	(0)	/* XXX */
#endif

extern int astpending;	 /* need to trap before returning to user mode */
#define aston() (astpending = 1)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci, flags)	\
	do { ci->ci_want_resched = 1; aston(); } while (/* CONSTCOND */0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the sun68k, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, aston())

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	aston()

/*
 * For some reason the sparc has this prototype in its machine/cpu.h,
 * so for now we do the same.
 */
void	fb_unblank(void);

/*
 * more stuff here for lack of a better place.
 */

#ifndef _SUN3X_
#define M68K_VAC
#endif

#endif	/* _KERNEL */

#include <m68k/sysctl.h>
#endif /* _CPU_H_ */
