/* $NetBSD: sched.h,v 1.25 2005/12/24 19:01:28 perry Exp $ */

/*-
 * Copyright (c) 1999, 2000, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey, Jason R. Thorpe, and Nathan J. Williams.
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

#include <sys/time.h>

/*
 * Sleep queues.
 */
struct slpque {
	struct lwp *sq_head;
	struct lwp **sq_tailp;
};

/*
 * Run queues.
 *
 * We have 32 run queues in descending priority of 0..31.  We maintain
 * a bitmask of non-empty queues in order speed up finding the first
 * runnable process.  The bitmask is maintained only by machine-dependent
 * code, allowing the most efficient instructions to be used to find the
 * first non-empty queue.
 */
#define	RUNQUE_NQS		32
struct prochd {
	struct lwp *ph_link;
	struct lwp *ph_rlink;
};

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

/*
 * Per-CPU scheduler state.
 */
struct schedstate_percpu {
	struct timeval spc_runtime;	/* time curproc started running */
	volatile int spc_flags;	/* flags; see below */
	u_int spc_schedticks;		/* ticks for schedclock() */
	u_int64_t spc_cp_time[CPUSTATES]; /* CPU state statistics */
	u_char spc_curpriority;		/* usrpri of curproc */
	int spc_rrticks;		/* ticks until roundrobin() */
	int spc_pscnt;			/* prof/stat counter */
	int spc_psdiv;			/* prof/stat divisor */
};

/* spc_flags */
#define	SPCF_SEENRR		0x0001	/* process has seen roundrobin() */
#define	SPCF_SHOULDYIELD	0x0002	/* process should yield the CPU */

#define	SPCF_SWITCHCLEAR	(SPCF_SEENRR|SPCF_SHOULDYIELD)

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
extern int rrticks;			/* ticks per roundrobin() */

/*
 * Global scheduler state.  We would like to group these all together
 * in a single structure to make them easier to find, but leaving
 * whichqs and qs as independent globals makes for more efficient
 * assembly language in the low-level context switch code.  So we
 * simply give them meaningful names; the globals are actually declared
 * in kern/kern_synch.c.
 */
extern struct prochd sched_qs[];
extern volatile u_int32_t sched_whichqs;

struct proc;
struct cpu_info;

void schedclock(struct lwp *);
void sched_wakeup(volatile const void *);
void roundrobin(struct cpu_info *);

void scheduler_fork_hook(struct proc *, struct proc *);
void scheduler_wait_hook(struct proc *, struct proc *);

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#include <sys/lock.h>

extern struct simplelock sched_lock;

#define	SCHED_ASSERT_LOCKED()	LOCK_ASSERT(simple_lock_held(&sched_lock))
#define	SCHED_ASSERT_UNLOCKED()	LOCK_ASSERT(simple_lock_held(&sched_lock) == 0)

#define	SCHED_LOCK(s)							\
do {									\
	s = splsched();							\
	simple_lock(&sched_lock);					\
} while (/* CONSTCOND */ 0)

#define	SCHED_UNLOCK(s)							\
do {									\
	simple_unlock(&sched_lock);					\
	splx(s);							\
} while (/* CONSTCOND */ 0)

void	sched_lock_idle(void);
void	sched_unlock_idle(void);

#else /* ! MULTIPROCESSOR || LOCKDEBUG */

#define	SCHED_ASSERT_LOCKED()		/* nothing */
#define	SCHED_ASSERT_UNLOCKED()		/* nothing */

#define	SCHED_LOCK(s)			s = splsched()
#define	SCHED_UNLOCK(s)			splx(s)

#endif /* MULTIPROCESSOR || LOCKDEBUG */

#endif	/* _KERNEL */
#endif	/* _SYS_SCHED_H_ */
