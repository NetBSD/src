/* $NetBSD: sched.h,v 1.14.10.5 2002/03/16 20:57:41 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2000, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey and Jason R. Thorpe.
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
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 */

#ifndef	_SYS_SCHED_H_
#define	_SYS_SCHED_H_

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#endif

/*
 * Posix defines a <sched.h> which may want to include <sys/sched.h>
 */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) && \
    !defined(_ANSI_SOURCE)

#include <sys/time.h>
#include <sys/queue.h>

/*
 * Sleep queues.
 *
 * We're only looking at 7 bits of the address; everything is
 * aligned to 4, lots of things are aligned to greater powers
 * of 2.  Shift right by 8, i.e. drop the bottom 256 worth.
 */
#define	SLPQUE_TABLESIZE	128
#define	SLPQUE_LOOKUP(x)	(((u_long)(x) >> 8) & (SLPQUE_TABLESIZE - 1))
struct slpque {
	struct proc *sq_head;
	struct proc **sq_tailp;
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
	struct proc *ph_link;
	struct proc *ph_rlink;
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
	__volatile int spc_flags;	/* flags; see below */
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
 * Turnstile sleep queue for kernel synchronization primitives.
 */
struct turnstile {
	LIST_ENTRY(turnstile) ts_chain;	/* link on hash chain */
	struct turnstile *ts_free;	/* turnstile free list */
	void		*ts_obj;	/* lock object */
	struct turnstile_sleepq {	/* queues of waiters */
		struct slpque tsq_q;
		u_int tsq_waiters;
	} ts_sleepq[2];
};

#define	TS_READER_Q	0		/* reader sleep queue */
#define	TS_WRITER_Q	1		/* writer sleep queue */

#define	TS_WAITERS(ts)							\
	((ts)->ts_sleepq[TS_READER_Q].tsq_waiters +			\
	 (ts)->ts_sleepq[TS_WRITER_Q].tsq_waiters)

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

#include <sys/pool.h>

#define	PPQ	(128 / RUNQUE_NQS)	/* priorities per queue */
#define NICE_WEIGHT 2			/* priorities per nice level */
#define	ESTCPULIM(e) min((e), NICE_WEIGHT * PRIO_MAX - PPQ)

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
extern struct slpque sched_slpque[];
extern __volatile u_int32_t sched_whichqs;

#define	SLPQUE(ident)	(&sched_slpque[SLPQUE_LOOKUP(ident)])

struct proc;
struct cpu_info;

void schedclock(struct proc *p);
void sched_wakeup(void *);
void roundrobin(struct cpu_info *);
void awaken(struct proc *);

void	turnstile_init(void);

struct turnstile *turnstile_lookup(void *);
void	turnstile_exit(void *);
int	turnstile_block(struct turnstile *, int, int, void *);
void	turnstile_wakeup(struct turnstile *, int, int, struct proc *);

extern struct pool_cache turnstile_cache;

/*
 * scheduler_fork_hook:
 *
 *	Inherit the parent's scheduler history.
 */
#define	scheduler_fork_hook(parent, child)				\
do {									\
	(child)->p_estcpu = (parent)->p_estcpu;				\
} while (/* CONSTCOND */ 0)

/*
 * scheduler_wait_hook:
 *
 *	Chargeback parents for the sins of their children.
 */
#define	scheduler_wait_hook(parent, child)				\
do {									\
	/* XXX Only if parent != init?? */				\
	(parent)->p_estcpu = ESTCPULIM((parent)->p_estcpu +		\
	    (child)->p_estcpu);						\
} while (/* CONSTCOND */ 0)

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#include <sys/lock.h>

extern struct simplelock sched_lock;

#define	SCHED_ASSERT_LOCKED()	LOCK_ASSERT(simple_lock_held(&sched_lock))
#define	SCHED_ASSERT_UNLOCKED()	LOCK_ASSERT(simple_lock_held(&sched_lock) == 0)

#define	_SCHED_LOCK			simple_lock(&sched_lock)
#define	_SCHED_UNLOCK			simple_unlock(&sched_lock)

void	sched_lock_idle(void);
void	sched_unlock_idle(void);

#else /* ! MULTIPROCESSOR || LOCKDEBUG */

#define	SCHED_ASSERT_LOCKED()		/* nothing */
#define	SCHED_ASSERT_UNLOCKED()		/* nothing */

#define	_SCHED_LOCK			/* nothing */
#define	_SCHED_UNLOCK			/* nothing */

#define	SCHED_LOCK(s)							\
do {									\
	s = splsched();							\
	_SCHED_LOCK;							\
} while (/* CONSTCOND */ 0)

#define	SCHED_UNLOCK(s)							\
do {									\
	_SCHED_UNLOCK;							\
	splx(s);							\
} while (/* CONSTCOND */ 0)

#endif /* MULTIPROCESSOR || LOCKDEBUG */

#endif	/* _KERNEL */
#endif	/* _SYS_SCHED_H_ */
