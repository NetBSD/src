/*	$NetBSD: sched.h,v 1.36.2.3 2008/01/09 01:58:16 matt Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey, Jason R. Thorpe, Nathan J. Williams, Andrew Doran and
 * Daniel Sieger.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 */

#ifndef	_SYS_SCHED_H_
#define	_SYS_SCHED_H_

#include <sys/featuretest.h>

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#endif

struct sched_param {
	int	sched_priority;
};

/*
 * Scheduling policies required by IEEE Std 1003.1-2001
 */
#define	SCHED_OTHER	0	/* Behavior can be FIFO or RR, or not */
#define	SCHED_FIFO	1
#define	SCHED_RR	2

/* Other nonstandard policies: */

#if defined(_NETBSD_SOURCE)

/*
 * CPU states.
 * XXX Not really scheduler state, but no other good place to put
 * it right now, and it really is per-CPU.
 */
#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_INTR		3
#define	CP_IDLE		4
#define	CPUSTATES	5

#if defined(_KERNEL)

#include <sys/mutex.h>
#include <sys/time.h>

/*
 * Per-CPU scheduler state.  Field markings and the corresponding locks: 
 *
 * s:	splsched, may only be safely accessed by the CPU itself
 * m:	spc_mutex
 * (:	unlocked, stable
 * c:	cpu_lock
 */
struct schedstate_percpu {
	/* First set of data is likely to be accessed by other CPUs. */
	kmutex_t	*spc_mutex;	/* (: lock on below, runnable LWPs */
	kmutex_t	spc_lwplock;	/* (: general purpose lock for LWPs */
	pri_t		spc_curpriority;/* m: usrpri of curlwp */
	time_t		spc_lastmod;	/* c: time of last cpu state change */

	/* For the most part, this set of data is CPU-private. */
	void		*spc_sched_info;/* (: scheduler-specific structure */
	volatile int	spc_flags;	/* s: flags; see below */
	u_int		spc_schedticks;	/* s: ticks for schedclock() */
	uint64_t	spc_cp_time[CPUSTATES];/* s: CPU state statistics */
	int		spc_ticks;	/* s: ticks until sched_tick() */
	int		spc_pscnt;	/* s: prof/stat counter */
	int		spc_psdiv;	/* s: prof/stat divisor */
};

/* spc_flags */
#define	SPCF_SEENRR		0x0001	/* process has seen roundrobin() */
#define	SPCF_SHOULDYIELD	0x0002	/* process should yield the CPU */
#define	SPCF_OFFLINE		0x0004	/* CPU marked offline */

#define	SPCF_SWITCHCLEAR	(SPCF_SEENRR|SPCF_SHOULDYIELD)

#endif /* defined(_KERNEL) */

/*
 * Flags passed to the Linux-compatible __clone(2) system call.
 */
#define	CLONE_CSIGNAL		0x000000ff	/* signal to be sent at exit */
#define	CLONE_VM		0x00000100	/* share address space */
#define	CLONE_FS		0x00000200	/* share "file system" info */
#define	CLONE_FILES		0x00000400	/* share file descriptors */
#define	CLONE_SIGHAND		0x00000800	/* share signal actions */
#define	CLONE_PID		0x00001000	/* share process ID */
#define	CLONE_PTRACE		0x00002000	/* ptrace(2) continues on
						   child */
#define	CLONE_VFORK		0x00004000	/* parent blocks until child
						   exits */

#endif /* !_POSIX_SOURCE && !_XOPEN_SOURCE && !_ANSI_SOURCE */

#ifdef _KERNEL

extern int schedhz;			/* ideally: 16 */
extern const int schedppq;

struct proc;
struct cpu_info;

/*
 * Common Scheduler Interface.
 */

/* Scheduler initialization */
void		sched_init(void);
void		sched_rqinit(void);
void		sched_cpuattach(struct cpu_info *);
void		sched_setup(void);

/* Time-driven events */
void		sched_tick(struct cpu_info *);
void		schedclock(struct lwp *);
void		sched_schedclock(struct lwp *);
void		sched_pstats(void *);
void		sched_pstats_hook(struct lwp *);

/* Runqueue-related functions */
bool		sched_curcpu_runnable_p(void);
void		sched_dequeue(struct lwp *);
void		sched_enqueue(struct lwp *, bool);
struct lwp *	sched_nextlwp(void);

/* Priority adjustment */
void		sched_nice(struct proc *, int);

/* Handlers of fork and exit */
void		sched_proc_fork(struct proc *, struct proc *);
void		sched_proc_exit(struct proc *, struct proc *);
void		sched_lwp_fork(struct lwp *, struct lwp *);
void		sched_lwp_exit(struct lwp *);
void		sched_lwp_collect(struct lwp *);

void		sched_slept(struct lwp *);
void		sched_wakeup(struct lwp *);

void		setrunnable(struct lwp *);
void		sched_setrunnable(struct lwp *);

struct cpu_info *sched_takecpu(struct lwp *);
void		sched_print_runqueue(void (*pr)(const char *, ...));

/* Dispatching */
void		preempt(void);
int		mi_switch(struct lwp *);
void		resched_cpu(struct lwp *);
void		updatertime(lwp_t *, const struct bintime *);

#endif	/* _KERNEL */
#endif	/* _SYS_SCHED_H_ */
