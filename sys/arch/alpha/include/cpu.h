/* $NetBSD: cpu.h,v 1.35 1999/09/17 19:59:37 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#ifndef _ALPHA_CPU_H_
#define _ALPHA_CPU_H_

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_multiprocessor.h"
#endif

/*
 * Exported definitions unique to Alpha cpu support.
 */

#include <machine/alpha_cpu.h>

#ifdef _KERNEL
#include <machine/frame.h>

#if defined(MULTIPROCESSOR)

#include <sys/lock.h>			/* will also get LOCKDEBUG */

struct cpu_info {
	/*
	 * Public members.
	 */
	struct proc *ci_curproc;	/* current owner of the processor */
	struct simplelock ci_slock;	/* lock on this data structure */
	cpuid_t ci_cpuid;		/* our CPU ID */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif

	/*
	 * Private members.
	 */
	struct proc *ci_fpcurproc;	/* current owner of the FPU */
	paddr_t ci_curpcb;		/* PA of current HW PCB */
	struct proc *ci_idle_thread;	/* our idle thread */
	u_long ci_flags;		/* flags; see below */
	u_long ci_ipis;			/* interprocessor interrupts pending */
	struct device *ci_dev;		/* pointer to our device */
};

#define	CPUF_PRIMARY	0x01		/* CPU is primary CPU */
#define	CPUF_PRESENT	0x02		/* CPU is present */
#define	CPUF_RUNNING	0x04		/* CPU is running */

extern	u_long cpus_running;
extern	struct cpu_info cpu_info[];

#define	curcpu()	(&cpu_info[cpu_number()])

#define	fpcurproc	curcpu()->ci_fpcurproc
#define	curpcb		curcpu()->ci_curpcb
#else
extern	struct proc *fpcurproc;		/* current owner of FPU */
extern	paddr_t curpcb;			/* PA of current HW context */
#endif /* MULTIPROCESSOR */

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_wait(p)		/* nothing */
#define	cpu_number()		alpha_pal_whami()

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the Alpha, we use
 * what we push on an interrupt (a trapframe).
 */
struct clockframe {
	struct trapframe	cf_tf;
};
#define	CLKF_USERMODE(framep)						\
	(((framep)->cf_tf.tf_regs[FRAME_PS] & ALPHA_PSL_USERMODE) != 0)
#define	CLKF_BASEPRI(framep)						\
	(((framep)->cf_tf.tf_regs[FRAME_PS] & ALPHA_PSL_IPL_MASK) == 0)
#define	CLKF_PC(framep)		((framep)->cf_tf.tf_regs[FRAME_PC])
/*
 * XXX No way to accurately tell if we were in interrupt mode before taking
 * clock interrupt.
 */
#define	CLKF_INTR(framep)	(0)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched()	{ want_resched = 1; aston(); }

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	{ (p)->p_flag |= P_OWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

#define	aston()		(astpending = 1)

u_int64_t astpending;		/* need to trap before returning to user mode */
u_int64_t want_resched;		/* resched() was called */
#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_ROOT_DEVICE		2	/* string: root device name */
#define	CPU_UNALIGNED_PRINT	3	/* int: print unaligned accesses */
#define	CPU_UNALIGNED_FIX	4	/* int: fix unaligned accesses */
#define	CPU_UNALIGNED_SIGBUS	5	/* int: SIGBUS unaligned accesses */
#define	CPU_BOOTED_KERNEL	6	/* string: booted kernel name */
#define	CPU_MAXID		7	/* 6 valid machdep IDs */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "root_device", CTLTYPE_STRING }, \
	{ "unaligned_print", CTLTYPE_INT }, \
	{ "unaligned_fix", CTLTYPE_INT }, \
	{ "unaligned_sigbus", CTLTYPE_INT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
}

#ifdef _KERNEL

struct pcb;
struct proc;
struct reg;
struct rpb;
struct trapframe;

int	badaddr	__P((void *, size_t));

#endif /* _KERNEL */
#endif /* _ALPHA_CPU_H_ */
