/*	$NetBSD: cpu.h,v 1.6.2.3 2001/01/05 17:34:54 bouyer Exp $	*/

/*
 * Copyright (C) 1995-1997 Wolfgang Solfrank.
 * Copyright (C) 1995-1997 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_MACHINE_CPU_H_
#define	_MACHINE_CPU_H_

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_lockdebug.h"
#endif

#include <machine/frame.h>

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

#define	curcpu()		(&cpu_info_store)
#endif

struct machvec {
	int (*splhigh) __P((void));
	int (*spl0) __P((void));
	int (*splbio) __P((void));
	int (*splnet) __P((void));
	int (*spltty) __P((void));
	int (*splimp) __P((void));
	int (*splclock) __P((void));
	int (*spllowersoftclock) __P((void));
	int (*splsoftclock) __P((void));
	int (*splsoftnet) __P((void));
	int (*splx) __P((int));
	void (*setsoftclock) __P((void));
	void (*setsoftnet) __P((void));
	void (*clock_return) __P((struct clockframe *, int));
	void (*irq_establish) __P((int, int, void (*)(void *), void *));
};
extern struct machvec machine_interface;

#include <machine/psl.h>

#define	splhigh()	((*machine_interface.splhigh)())
#define	spl0()		((*machine_interface.spl0)())
#define	splbio()	((*machine_interface.splbio)())
#define	splnet()	((*machine_interface.splnet)())
#define	spltty()	((*machine_interface.spltty)())
#define	splimp()	((*machine_interface.splimp)())
#define	splclock()	((*machine_interface.splclock)())
#define	spllowersoftclock() ((*machine_interface.spllowersoftclock)())
#define	splsoftclock()	((*machine_interface.splsoftclock)())
#define	splstatclock()	splclock()
#define	splsoftnet()	((*machine_interface.splsoftnet)())
#define	splx(new)	((*machine_interface.splx)(new))
#define	setsoftclock()	((*machine_interface.setsoftclock)())
#define	setsoftnet()	((*machine_interface.setsoftnet)())
#define	clock_return(frame, level)		\
	((*machine_interface.clock_return)((frame), (level)))
#define	irq_establish(irq, level, handler, arg)	\
	((*machine_interface.irq_establish)((irq), (level), (handler), (arg)))

#define	splsched()	splhigh()
#define	spllock()	splhigh()

#define	CLKF_USERMODE(frame)	(((frame)->srr1 & PSL_PR) != 0)
#define	CLKF_BASEPRI(frame)	((frame)->pri == 0)
#define	CLKF_PC(frame)		((frame)->srr0)
#define	CLKF_INTR(frame)	((frame)->depth >= 0)

#define	PROC_PC(p)		(trapframe(p)->srr0)

#define	cpu_swapout(p)
#define cpu_wait(p)
#define	cpu_number()		0

extern void delay __P((unsigned));
#define	DELAY(n)		delay(n)

extern __volatile int want_resched;
extern __volatile int astpending;

#define	need_resched(ci)	(want_resched = 1, astpending = 1)
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, astpending = 1)
#define	signotify(p)		(astpending = 1)

extern char *bootpath;

#if defined(_KERNEL) || defined(_STANDALONE)
#define	CACHELINESIZE	32
#endif

#include <powerpc/cpu.h>

#endif	/* _MACHINE_CPU_H_ */
