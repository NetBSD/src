/*	$NetBSD: cpu.h,v 1.1 2003/08/19 10:53:05 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PDP10_CPU_H_
#define _PDP10_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

#ifdef _KERNEL

#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/sched.h>

struct cpu_info {
	/*
	 * Public members.
	 */
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;           /* # of spin locks held */
	u_long ci_simple_locks;         /* # of simple locks held */
#endif

	struct proc *ci_curproc;        /* current owner of the processor */

};

extern struct cpu_info cpu_info_store;
#define	curcpu()	(&cpu_info_store)

volatile int want_resched, astpending;
#define	need_resched(ci) (want_resched = 1, astpending = 1)
#define cpu_proc_fork(x, y)
void signotify(struct proc *);
void need_proftick(struct proc *);

#define	cpu_number()	0

/*
 * Info given to hardclock; current ac block (DATAI PAG).
 * If current ac block is not interrupt block, CLKF_BASEPRI. XXX - not for now
 * If current ac block is user block, CLKF_USERMODE.
 * If current ac block is interrupt block, CLKF_INTR.
 */
struct clockframe {
	int dataiw;
};
#define	CAC(y) (((y)->dataiw >> 27) & 7)

#define	CLKF_USERMODE(x)	(CAC(x) == 1)
#define	CLKF_BASEPRI(x)		/* (CAC(x) < 2) */ (0)
#define	CLKF_PC(x)		(panic("CLKF_PC"), 0)
#define	CLKF_INTR(x)		(CAC(x) == 2)

void kl10_conf(void);

/* Type conversion without pointer conversion */
#ifdef __GNUC__
#define TCONV(rtype, inval) \
	({ union { typeof(inval) p; rtype i; } f; f.p = inval; f.i; })
#endif
typedef union {
	int *intp;
	char *cp;
	int intop;
} TUNION;
#define TPTOINT(b) (uu.intp = (int *)b, uu.intop)
#define TINTTOP(b) (uu.intop = (int)b, uu.intp)
#define TINTTOCP(b) (uu.intop = (int)b, uu.cp)


#endif /* _KERNEL */

#endif /* _PDP10_CPU_H_ */
