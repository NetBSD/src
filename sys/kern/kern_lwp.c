/*	$NetBSD: kern_lwp.c,v 1.250 2022/05/22 11:27:36 andvar Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007, 2008, 2009, 2019, 2020
 *     The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, and Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Overview
 *
 *	Lightweight processes (LWPs) are the basic unit or thread of
 *	execution within the kernel.  The core state of an LWP is described
 *	by "struct lwp", also known as lwp_t.
 *
 *	Each LWP is contained within a process (described by "struct proc"),
 *	Every process contains at least one LWP, but may contain more.  The
 *	process describes attributes shared among all of its LWPs such as a
 *	private address space, global execution state (stopped, active,
 *	zombie, ...), signal disposition and so on.  On a multiprocessor
 *	machine, multiple LWPs be executing concurrently in the kernel.
 *
 * Execution states
 *
 *	At any given time, an LWP has overall state that is described by
 *	lwp::l_stat.  The states are broken into two sets below.  The first
 *	set is guaranteed to represent the absolute, current state of the
 *	LWP:
 *
 *	LSONPROC
 *
 *		On processor: the LWP is executing on a CPU, either in the
 *		kernel or in user space.
 *
 *	LSRUN
 *
 *		Runnable: the LWP is parked on a run queue, and may soon be
 *		chosen to run by an idle processor, or by a processor that
 *		has been asked to preempt a currently runnning but lower
 *		priority LWP.
 *
 *	LSIDL
 *
 *		Idle: the LWP has been created but has not yet executed, or
 *		it has ceased executing a unit of work and is waiting to be
 *		started again.  This state exists so that the LWP can occupy
 *		a slot in the process & PID table, but without having to
 *		worry about being touched; lookups of the LWP by ID will
 *		fail while in this state.  The LWP will become visible for
 *		lookup once its state transitions further.  Some special
 *		kernel threads also (ab)use this state to indicate that they
 *		are idle (soft interrupts and idle LWPs).
 *
 *	LSSUSPENDED:
 *
 *		Suspended: the LWP has had its execution suspended by
 *		another LWP in the same process using the _lwp_suspend()
 *		system call.  User-level LWPs also enter the suspended
 *		state when the system is shutting down.
 *
 *	The second set represent a "statement of intent" on behalf of the
 *	LWP.  The LWP may in fact be executing on a processor, may be
 *	sleeping or idle. It is expected to take the necessary action to
 *	stop executing or become "running" again within a short timeframe.
 *	The LP_RUNNING flag in lwp::l_pflag indicates that an LWP is running.
 *	Importantly, it indicates that its state is tied to a CPU.
 *
 *	LSZOMB:
 *
 *		Dead or dying: the LWP has released most of its resources
 *		and is about to switch away into oblivion, or has already
 *		switched away.  When it switches away, its few remaining
 *		resources can be collected.
 *
 *	LSSLEEP:
 *
 *		Sleeping: the LWP has entered itself onto a sleep queue, and
 *		has switched away or will switch away shortly to allow other
 *		LWPs to run on the CPU.
 *
 *	LSSTOP:
 *
 *		Stopped: the LWP has been stopped as a result of a job
 *		control signal, or as a result of the ptrace() interface. 
 *
 *		Stopped LWPs may run briefly within the kernel to handle
 *		signals that they receive, but will not return to user space
 *		until their process' state is changed away from stopped. 
 *
 *		Single LWPs within a process can not be set stopped
 *		selectively: all actions that can stop or continue LWPs
 *		occur at the process level.
 *
 * State transitions
 *
 *	Note that the LSSTOP state may only be set when returning to
 *	user space in userret(), or when sleeping interruptably.  The
 *	LSSUSPENDED state may only be set in userret().  Before setting
 *	those states, we try to ensure that the LWPs will release all
 *	locks that they hold, and at a minimum try to ensure that the
 *	LWP can be set runnable again by a signal.
 *
 *	LWPs may transition states in the following ways:
 *
 *	 RUN -------> ONPROC		ONPROC -----> RUN
 *		    				    > SLEEP
 *		    				    > STOPPED
 *						    > SUSPENDED
 *						    > ZOMB
 *						    > IDL (special cases)
 *
 *	 STOPPED ---> RUN		SUSPENDED --> RUN
 *	            > SLEEP
 *
 *	 SLEEP -----> ONPROC		IDL --------> RUN
 *		    > RUN			    > SUSPENDED
 *		    > STOPPED			    > STOPPED
 *						    > ONPROC (special cases)
 *
 *	Some state transitions are only possible with kernel threads (eg
 *	ONPROC -> IDL) and happen under tightly controlled circumstances
 *	free of unwanted side effects.
 *
 * Migration
 *
 *	Migration of threads from one CPU to another could be performed
 *	internally by the scheduler via sched_takecpu() or sched_catchlwp()
 *	functions.  The universal lwp_migrate() function should be used for
 *	any other cases.  Subsystems in the kernel must be aware that CPU
 *	of LWP may change, while it is not locked.
 *
 * Locking
 *
 *	The majority of fields in 'struct lwp' are covered by a single,
 *	general spin lock pointed to by lwp::l_mutex.  The locks covering
 *	each field are documented in sys/lwp.h.
 *
 *	State transitions must be made with the LWP's general lock held,
 *	and may cause the LWP's lock pointer to change.  Manipulation of
 *	the general lock is not performed directly, but through calls to
 *	lwp_lock(), lwp_unlock() and others.  It should be noted that the
 *	adaptive locks are not allowed to be released while the LWP's lock
 *	is being held (unlike for other spin-locks).
 *
 *	States and their associated locks:
 *
 *	LSIDL, LSONPROC, LSZOMB, LSSUPENDED:
 *
 *		Always covered by spc_lwplock, which protects LWPs not
 *		associated with any other sync object.  This is a per-CPU
 *		lock and matches lwp::l_cpu.
 *
 *	LSRUN:
 *
 *		Always covered by spc_mutex, which protects the run queues.
 *		This is a per-CPU lock and matches lwp::l_cpu.
 *
 *	LSSLEEP:
 *
 *		Covered by a lock associated with the sleep queue (sometimes
 *		a turnstile sleep queue) that the LWP resides on.  This can
 *		be spc_lwplock for SOBJ_SLEEPQ_NULL (an "untracked" sleep).
 *
 *	LSSTOP:
 *
 *		If the LWP was previously sleeping (l_wchan != NULL), then
 *		l_mutex references the sleep queue lock.  If the LWP was
 *		runnable or on the CPU when halted, or has been removed from
 *		the sleep queue since halted, then the lock is spc_lwplock.
 *
 *	The lock order is as follows:
 *
 *		sleepq -> turnstile -> spc_lwplock -> spc_mutex
 *
 *	Each process has a scheduler state lock (proc::p_lock), and a
 *	number of counters on LWPs and their states: p_nzlwps, p_nrlwps, and
 *	so on.  When an LWP is to be entered into or removed from one of the
 *	following states, p_lock must be held and the process wide counters
 *	adjusted:
 *
 *		LSIDL, LSZOMB, LSSTOP, LSSUSPENDED
 *
 *	(But not always for kernel threads.  There are some special cases
 *	as mentioned above: soft interrupts, and the idle loops.)
 *
 *	Note that an LWP is considered running or likely to run soon if in
 *	one of the following states.  This affects the value of p_nrlwps:
 *
 *		LSRUN, LSONPROC, LSSLEEP
 *
 *	p_lock does not need to be held when transitioning among these
 *	three states, hence p_lock is rarely taken for state transitions.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_lwp.c,v 1.250 2022/05/22 11:27:36 andvar Exp $");

#include "opt_ddb.h"
#include "opt_lockdebug.h"
#include "opt_dtrace.h"

#define _LWP_API_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/syscall_stats.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>
#include <sys/kmem.h>
#include <sys/pset.h>
#include <sys/intr.h>
#include <sys/lwpctl.h>
#include <sys/atomic.h>
#include <sys/filedesc.h>
#include <sys/fstrans.h>
#include <sys/dtrace_bsd.h>
#include <sys/sdt.h>
#include <sys/ptrace.h>
#include <sys/xcall.h>
#include <sys/uidinfo.h>
#include <sys/sysctl.h>
#include <sys/psref.h>
#include <sys/msan.h>
#include <sys/kcov.h>
#include <sys/cprng.h>
#include <sys/futex.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>

static pool_cache_t	lwp_cache	__read_mostly;
struct lwplist		alllwp		__cacheline_aligned;

static int		lwp_ctor(void *, void *, int);
static void		lwp_dtor(void *, void *);

/* DTrace proc provider probes */
SDT_PROVIDER_DEFINE(proc);

SDT_PROBE_DEFINE1(proc, kernel, , lwp__create, "struct lwp *");
SDT_PROBE_DEFINE1(proc, kernel, , lwp__start, "struct lwp *");
SDT_PROBE_DEFINE1(proc, kernel, , lwp__exit, "struct lwp *");

struct turnstile turnstile0 __cacheline_aligned;
struct lwp lwp0 __aligned(MIN_LWP_ALIGNMENT) = {
#ifdef LWP0_CPU_INFO
	.l_cpu = LWP0_CPU_INFO,
#endif
#ifdef LWP0_MD_INITIALIZER
	.l_md = LWP0_MD_INITIALIZER,
#endif
	.l_proc = &proc0,
	.l_lid = 0,		/* we own proc0's slot in the pid table */
	.l_flag = LW_SYSTEM,
	.l_stat = LSONPROC,
	.l_ts = &turnstile0,
	.l_syncobj = &sched_syncobj,
	.l_refcnt = 0,
	.l_priority = PRI_USER + NPRI_USER - 1,
	.l_inheritedprio = -1,
	.l_class = SCHED_OTHER,
	.l_psid = PS_NONE,
	.l_pi_lenders = SLIST_HEAD_INITIALIZER(&lwp0.l_pi_lenders),
	.l_name = __UNCONST("swapper"),
	.l_fd = &filedesc0,
};

static int
lwp_maxlwp(void)
{
	/* Assume 1 LWP per 1MiB. */
	uint64_t lwps_per = ctob(physmem) / (1024 * 1024);

	return MAX(MIN(MAXMAXLWP, lwps_per), MAXLWP);
}

static int sysctl_kern_maxlwp(SYSCTLFN_PROTO);

/*
 * sysctl helper routine for kern.maxlwp. Ensures that the new
 * values are not too low or too high.
 */
static int
sysctl_kern_maxlwp(SYSCTLFN_ARGS)
{
	int error, nmaxlwp;
	struct sysctlnode node;

	nmaxlwp = maxlwp;
	node = *rnode;
	node.sysctl_data = &nmaxlwp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (nmaxlwp < 0 || nmaxlwp >= MAXMAXLWP)
		return EINVAL;
	if (nmaxlwp > lwp_maxlwp())
		return EINVAL;
	maxlwp = nmaxlwp;

	return 0;
}

static void
sysctl_kern_lwp_setup(void)
{
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxlwp",
		       SYSCTL_DESCR("Maximum number of simultaneous threads"),
		       sysctl_kern_maxlwp, 0, NULL, 0,
		       CTL_KERN, CTL_CREATE, CTL_EOL);
}

void
lwpinit(void)
{

	LIST_INIT(&alllwp);
	lwpinit_specificdata();
	/*
	 * Provide a barrier to ensure that all mutex_oncpu() and rw_oncpu()
	 * calls will exit before memory of LWPs is returned to the pool, where
	 * KVA of LWP structure might be freed and re-used for other purposes.
	 * Kernel preemption is disabled around mutex_oncpu() and rw_oncpu()
	 * callers, therefore a regular passive serialization barrier will
	 * do the job.
	 */
	lwp_cache = pool_cache_init(sizeof(lwp_t), MIN_LWP_ALIGNMENT, 0,
	    PR_PSERIALIZE, "lwppl", NULL, IPL_NONE, lwp_ctor, lwp_dtor, NULL);

	maxlwp = lwp_maxlwp();
	sysctl_kern_lwp_setup();
}

void
lwp0_init(void)
{
	struct lwp *l = &lwp0;

	KASSERT((void *)uvm_lwp_getuarea(l) != NULL);

	LIST_INSERT_HEAD(&alllwp, l, l_list);

	callout_init(&l->l_timeout_ch, CALLOUT_MPSAFE);
	callout_setfunc(&l->l_timeout_ch, sleepq_timeout, l);
	cv_init(&l->l_sigcv, "sigwait");
	cv_init(&l->l_waitcv, "vfork");

	kauth_cred_hold(proc0.p_cred);
	l->l_cred = proc0.p_cred;

	kdtrace_thread_ctor(NULL, l);
	lwp_initspecific(l);

	SYSCALL_TIME_LWP_INIT(l);
}

/*
 * Initialize the non-zeroed portion of an lwp_t.
 */
static int
lwp_ctor(void *arg, void *obj, int flags)
{
	lwp_t *l = obj;

	l->l_stat = LSIDL;
	l->l_cpu = curcpu();
	l->l_mutex = l->l_cpu->ci_schedstate.spc_lwplock;
	l->l_ts = pool_get(&turnstile_pool, flags);

	if (l->l_ts == NULL) {
		return ENOMEM;
	} else {
		turnstile_ctor(l->l_ts);
		return 0;
	}
}

static void
lwp_dtor(void *arg, void *obj)
{
	lwp_t *l = obj;

	/*
	 * The value of l->l_cpu must still be valid at this point.
	 */
	KASSERT(l->l_cpu != NULL);

	/*
	 * We can't return turnstile0 to the pool (it didn't come from it),
	 * so if it comes up just drop it quietly and move on.
	 */
	if (l->l_ts != &turnstile0)
		pool_put(&turnstile_pool, l->l_ts);
}

/*
 * Set an LWP suspended.
 *
 * Must be called with p_lock held, and the LWP locked.  Will unlock the
 * LWP before return.
 */
int
lwp_suspend(struct lwp *curl, struct lwp *t)
{
	int error;

	KASSERT(mutex_owned(t->l_proc->p_lock));
	KASSERT(lwp_locked(t, NULL));

	KASSERT(curl != t || curl->l_stat == LSONPROC);

	/*
	 * If the current LWP has been told to exit, we must not suspend anyone
	 * else or deadlock could occur.  We won't return to userspace.
	 */
	if ((curl->l_flag & (LW_WEXIT | LW_WCORE)) != 0) {
		lwp_unlock(t);
		return (EDEADLK);
	}

	if ((t->l_flag & LW_DBGSUSPEND) != 0) {
		lwp_unlock(t);
		return 0;
	}

	error = 0;

	switch (t->l_stat) {
	case LSRUN:
	case LSONPROC:
		t->l_flag |= LW_WSUSPEND;
		lwp_need_userret(t);
		lwp_unlock(t);
		break;

	case LSSLEEP:
		t->l_flag |= LW_WSUSPEND;

		/*
		 * Kick the LWP and try to get it to the kernel boundary
		 * so that it will release any locks that it holds.
		 * setrunnable() will release the lock.
		 */
		if ((t->l_flag & LW_SINTR) != 0)
			setrunnable(t);
		else
			lwp_unlock(t);
		break;

	case LSSUSPENDED:
		lwp_unlock(t);
		break;

	case LSSTOP:
		t->l_flag |= LW_WSUSPEND;
		setrunnable(t);
		break;

	case LSIDL:
	case LSZOMB:
		error = EINTR; /* It's what Solaris does..... */
		lwp_unlock(t);
		break;
	}

	return (error);
}

/*
 * Restart a suspended LWP.
 *
 * Must be called with p_lock held, and the LWP locked.  Will unlock the
 * LWP before return.
 */
void
lwp_continue(struct lwp *l)
{

	KASSERT(mutex_owned(l->l_proc->p_lock));
	KASSERT(lwp_locked(l, NULL));

	/* If rebooting or not suspended, then just bail out. */
	if ((l->l_flag & LW_WREBOOT) != 0) {
		lwp_unlock(l);
		return;
	}

	l->l_flag &= ~LW_WSUSPEND;

	if (l->l_stat != LSSUSPENDED || (l->l_flag & LW_DBGSUSPEND) != 0) {
		lwp_unlock(l);
		return;
	}

	/* setrunnable() will release the lock. */
	setrunnable(l);
}

/*
 * Restart a stopped LWP.
 *
 * Must be called with p_lock held, and the LWP NOT locked.  Will unlock the
 * LWP before return.
 */
void
lwp_unstop(struct lwp *l)
{
	struct proc *p = l->l_proc;

	KASSERT(mutex_owned(&proc_lock));
	KASSERT(mutex_owned(p->p_lock));

	lwp_lock(l);

	KASSERT((l->l_flag & LW_DBGSUSPEND) == 0);

	/* If not stopped, then just bail out. */
	if (l->l_stat != LSSTOP) {
		lwp_unlock(l);
		return;
	}

	p->p_stat = SACTIVE;
	p->p_sflag &= ~PS_STOPPING;

	if (!p->p_waited)
		p->p_pptr->p_nstopchild--;

	if (l->l_wchan == NULL) {
		/* setrunnable() will release the lock. */
		setrunnable(l);
	} else if (p->p_xsig && (l->l_flag & LW_SINTR) != 0) {
		/* setrunnable() so we can receive the signal */
		setrunnable(l);
	} else {
		l->l_stat = LSSLEEP;
		p->p_nrlwps++;
		lwp_unlock(l);
	}
}

/*
 * Wait for an LWP within the current process to exit.  If 'lid' is
 * non-zero, we are waiting for a specific LWP.
 *
 * Must be called with p->p_lock held.
 */
int
lwp_wait(struct lwp *l, lwpid_t lid, lwpid_t *departed, bool exiting)
{
	const lwpid_t curlid = l->l_lid;
	proc_t *p = l->l_proc;
	lwp_t *l2, *next;
	int error;

	KASSERT(mutex_owned(p->p_lock));

	p->p_nlwpwait++;
	l->l_waitingfor = lid;

	for (;;) {
		int nfound;

		/*
		 * Avoid a race between exit1() and sigexit(): if the
		 * process is dumping core, then we need to bail out: call
		 * into lwp_userret() where we will be suspended until the
		 * deed is done.
		 */
		if ((p->p_sflag & PS_WCORE) != 0) {
			mutex_exit(p->p_lock);
			lwp_userret(l);
			KASSERT(false);
		}

		/*
		 * First off, drain any detached LWP that is waiting to be
		 * reaped.
		 */
		while ((l2 = p->p_zomblwp) != NULL) {
			p->p_zomblwp = NULL;
			lwp_free(l2, false, false);/* releases proc mutex */
			mutex_enter(p->p_lock);
		}

		/*
		 * Now look for an LWP to collect.  If the whole process is
		 * exiting, count detached LWPs as eligible to be collected,
		 * but don't drain them here.
		 */
		nfound = 0;
		error = 0;

		/*
		 * If given a specific LID, go via pid_table and make sure
		 * it's not detached.
		 */
		if (lid != 0) {
			l2 = proc_find_lwp(p, lid);
			if (l2 == NULL) {
				error = ESRCH;
				break;
			}
			KASSERT(l2->l_lid == lid);
			if ((l2->l_prflag & LPR_DETACHED) != 0) {
				error = EINVAL;
				break;
			}
		} else {
			l2 = LIST_FIRST(&p->p_lwps);
		}
		for (; l2 != NULL; l2 = next) {
			next = (lid != 0 ? NULL : LIST_NEXT(l2, l_sibling));

			/*
			 * If a specific wait and the target is waiting on
			 * us, then avoid deadlock.  This also traps LWPs
			 * that try to wait on themselves.
			 *
			 * Note that this does not handle more complicated
			 * cycles, like: t1 -> t2 -> t3 -> t1.  The process
			 * can still be killed so it is not a major problem.
			 */
			if (l2->l_lid == lid && l2->l_waitingfor == curlid) {
				error = EDEADLK;
				break;
			}
			if (l2 == l)
				continue;
			if ((l2->l_prflag & LPR_DETACHED) != 0) {
				nfound += exiting;
				continue;
			}
			if (lid != 0) {
				/*
				 * Mark this LWP as the first waiter, if there
				 * is no other.
				 */
				if (l2->l_waiter == 0)
					l2->l_waiter = curlid;
			} else if (l2->l_waiter != 0) {
				/*
				 * It already has a waiter - so don't
				 * collect it.  If the waiter doesn't
				 * grab it we'll get another chance
				 * later.
				 */
				nfound++;
				continue;
			}
			nfound++;

			/* No need to lock the LWP in order to see LSZOMB. */
			if (l2->l_stat != LSZOMB)
				continue;

			/*
			 * We're no longer waiting.  Reset the "first waiter"
			 * pointer on the target, in case it was us.
			 */
			l->l_waitingfor = 0;
			l2->l_waiter = 0;
			p->p_nlwpwait--;
			if (departed)
				*departed = l2->l_lid;
			sched_lwp_collect(l2);

			/* lwp_free() releases the proc lock. */
			lwp_free(l2, false, false);
			mutex_enter(p->p_lock);
			return 0;
		}

		if (error != 0)
			break;
		if (nfound == 0) {
			error = ESRCH;
			break;
		}

		/*
		 * Note: since the lock will be dropped, need to restart on
		 * wakeup to run all LWPs again, e.g. there may be new LWPs.
		 */
		if (exiting) {
			KASSERT(p->p_nlwps > 1);
			error = cv_timedwait(&p->p_lwpcv, p->p_lock, 1);
			break;
		}

		/*
		 * Break out if all LWPs are in _lwp_wait().  There are
		 * other ways to hang the process with _lwp_wait(), but the
		 * sleep is interruptable so little point checking for them.
		 */
		if (p->p_nlwpwait == p->p_nlwps) {
			error = EDEADLK;
			break;
		}

		/*
		 * Sit around and wait for something to happen.  We'll be 
		 * awoken if any of the conditions examined change: if an
		 * LWP exits, is collected, or is detached.
		 */
		if ((error = cv_wait_sig(&p->p_lwpcv, p->p_lock)) != 0)
			break;
	}

	/*
	 * We didn't find any LWPs to collect, we may have received a 
	 * signal, or some other condition has caused us to bail out.
	 *
	 * If waiting on a specific LWP, clear the waiters marker: some
	 * other LWP may want it.  Then, kick all the remaining waiters
	 * so that they can re-check for zombies and for deadlock.
	 */
	if (lid != 0) {
		l2 = proc_find_lwp(p, lid);
		KASSERT(l2 == NULL || l2->l_lid == lid);

		if (l2 != NULL && l2->l_waiter == curlid)
			l2->l_waiter = 0;
	}
	p->p_nlwpwait--;
	l->l_waitingfor = 0;
	cv_broadcast(&p->p_lwpcv);

	return error;
}

/*
 * Create a new LWP within process 'p2', using LWP 'l1' as a template.
 * The new LWP is created in state LSIDL and must be set running,
 * suspended, or stopped by the caller.
 */
int
lwp_create(lwp_t *l1, proc_t *p2, vaddr_t uaddr, int flags,
    void *stack, size_t stacksize, void (*func)(void *), void *arg,
    lwp_t **rnewlwpp, int sclass, const sigset_t *sigmask,
    const stack_t *sigstk)
{
	struct lwp *l2;

	KASSERT(l1 == curlwp || l1->l_proc == &proc0);

	/*
	 * Enforce limits, excluding the first lwp and kthreads.  We must
	 * use the process credentials here when adjusting the limit, as
	 * they are what's tied to the accounting entity.  However for
	 * authorizing the action, we'll use the LWP's credentials.
	 */
	mutex_enter(p2->p_lock);
	if (p2->p_nlwps != 0 && p2 != &proc0) {
		uid_t uid = kauth_cred_getuid(p2->p_cred);
		int count = chglwpcnt(uid, 1);
		if (__predict_false(count >
		    p2->p_rlimit[RLIMIT_NTHR].rlim_cur)) {
			if (kauth_authorize_process(l1->l_cred,
			    KAUTH_PROCESS_RLIMIT, p2,
			    KAUTH_ARG(KAUTH_REQ_PROCESS_RLIMIT_BYPASS),
			    &p2->p_rlimit[RLIMIT_NTHR], KAUTH_ARG(RLIMIT_NTHR))
			    != 0) {
				(void)chglwpcnt(uid, -1);
				mutex_exit(p2->p_lock);
				return EAGAIN;
			}
		}
	}

	/*
	 * First off, reap any detached LWP waiting to be collected.
	 * We can re-use its LWP structure and turnstile.
	 */
	if ((l2 = p2->p_zomblwp) != NULL) {
		p2->p_zomblwp = NULL;
		lwp_free(l2, true, false);
		/* p2 now unlocked by lwp_free() */
		KASSERT(l2->l_ts != NULL);
		KASSERT(l2->l_inheritedprio == -1);
		KASSERT(SLIST_EMPTY(&l2->l_pi_lenders));
		memset(&l2->l_startzero, 0, sizeof(*l2) -
		    offsetof(lwp_t, l_startzero));
	} else {
		mutex_exit(p2->p_lock);
		l2 = pool_cache_get(lwp_cache, PR_WAITOK);
		memset(&l2->l_startzero, 0, sizeof(*l2) -
		    offsetof(lwp_t, l_startzero));
		SLIST_INIT(&l2->l_pi_lenders);
	}

	/*
	 * Because of lockless lookup via pid_table, the LWP can be locked
	 * and inspected briefly even after it's freed, so a few fields are
	 * kept stable.
	 */
	KASSERT(l2->l_stat == LSIDL);
	KASSERT(l2->l_cpu != NULL);
	KASSERT(l2->l_ts != NULL);
	KASSERT(l2->l_mutex == l2->l_cpu->ci_schedstate.spc_lwplock);

	l2->l_proc = p2;
	l2->l_refcnt = 0;
	l2->l_class = sclass;

	/*
	 * Allocate a process ID for this LWP.  We need to do this now
	 * while we can still unwind if it fails.  Because we're marked
	 * as LSIDL, no lookups by the ID will succeed.
	 *
	 * N.B. this will always succeed for the first LWP in a process,
	 * because proc_alloc_lwpid() will usurp the slot.  Also note
	 * that l2->l_proc MUST be valid so that lookups of the proc
	 * will succeed, even if the LWP itself is not visible.
	 */
	if (__predict_false(proc_alloc_lwpid(p2, l2) == -1)) {
		pool_cache_put(lwp_cache, l2);
		return EAGAIN;
	}

	/*
	 * If vfork(), we want the LWP to run fast and on the same CPU
	 * as its parent, so that it can reuse the VM context and cache
	 * footprint on the local CPU.
	 */
	l2->l_kpriority = ((flags & LWP_VFORK) ? true : false);
	l2->l_kpribase = PRI_KERNEL;
	l2->l_priority = l1->l_priority;
	l2->l_inheritedprio = -1;
	l2->l_protectprio = -1;
	l2->l_auxprio = -1;
	l2->l_flag = 0;
	l2->l_pflag = LP_MPSAFE;
	TAILQ_INIT(&l2->l_ld_locks);
	l2->l_psrefs = 0;
	kmsan_lwp_alloc(l2);

	/*
	 * For vfork, borrow parent's lwpctl context if it exists.
	 * This also causes us to return via lwp_userret.
	 */
	if (flags & LWP_VFORK && l1->l_lwpctl) {
		l2->l_lwpctl = l1->l_lwpctl;
		l2->l_flag |= LW_LWPCTL;
	}

	/*
	 * If not the first LWP in the process, grab a reference to the
	 * descriptor table.
	 */
	l2->l_fd = p2->p_fd;
	if (p2->p_nlwps != 0) {
		KASSERT(l1->l_proc == p2);
		fd_hold(l2);
	} else {
		KASSERT(l1->l_proc != p2);
	}

	if (p2->p_flag & PK_SYSTEM) {
		/* Mark it as a system LWP. */
		l2->l_flag |= LW_SYSTEM;
	}

	kdtrace_thread_ctor(NULL, l2);
	lwp_initspecific(l2);
	sched_lwp_fork(l1, l2);
	lwp_update_creds(l2);
	callout_init(&l2->l_timeout_ch, CALLOUT_MPSAFE);
	callout_setfunc(&l2->l_timeout_ch, sleepq_timeout, l2);
	cv_init(&l2->l_sigcv, "sigwait");
	cv_init(&l2->l_waitcv, "vfork");
	l2->l_syncobj = &sched_syncobj;
	PSREF_DEBUG_INIT_LWP(l2);

	if (rnewlwpp != NULL)
		*rnewlwpp = l2;

	/*
	 * PCU state needs to be saved before calling uvm_lwp_fork() so that
	 * the MD cpu_lwp_fork() can copy the saved state to the new LWP.
	 */
	pcu_save_all(l1);
#if PCU_UNIT_COUNT > 0
	l2->l_pcu_valid = l1->l_pcu_valid;
#endif

	uvm_lwp_setuarea(l2, uaddr);
	uvm_lwp_fork(l1, l2, stack, stacksize, func, (arg != NULL) ? arg : l2);

	mutex_enter(p2->p_lock);
	if ((flags & LWP_DETACHED) != 0) {
		l2->l_prflag = LPR_DETACHED;
		p2->p_ndlwps++;
	} else
		l2->l_prflag = 0;

	if (l1->l_proc == p2) {
		/*
		 * These flags are set while p_lock is held.  Copy with
		 * p_lock held too, so the LWP doesn't sneak into the
		 * process without them being set.
		 */
		l2->l_flag |= (l1->l_flag & (LW_WEXIT | LW_WREBOOT | LW_WCORE));
	} else {
		/* fork(): pending core/exit doesn't apply to child. */
		l2->l_flag |= (l1->l_flag & LW_WREBOOT);
	}

	l2->l_sigstk = *sigstk;
	l2->l_sigmask = *sigmask;
	TAILQ_INIT(&l2->l_sigpend.sp_info);
	sigemptyset(&l2->l_sigpend.sp_set);
	LIST_INSERT_HEAD(&p2->p_lwps, l2, l_sibling);
	p2->p_nlwps++;
	p2->p_nrlwps++;

	KASSERT(l2->l_affinity == NULL);

	/* Inherit the affinity mask. */
	if (l1->l_affinity) {
		/*
		 * Note that we hold the state lock while inheriting
		 * the affinity to avoid race with sched_setaffinity().
		 */
		lwp_lock(l1);
		if (l1->l_affinity) {
			kcpuset_use(l1->l_affinity);
			l2->l_affinity = l1->l_affinity;
		}
		lwp_unlock(l1);
	}

	/* This marks the end of the "must be atomic" section. */
	mutex_exit(p2->p_lock);

	SDT_PROBE(proc, kernel, , lwp__create, l2, 0, 0, 0, 0);

	mutex_enter(&proc_lock);
	LIST_INSERT_HEAD(&alllwp, l2, l_list);
	/* Inherit a processor-set */
	l2->l_psid = l1->l_psid;
	mutex_exit(&proc_lock);

	SYSCALL_TIME_LWP_INIT(l2);

	if (p2->p_emul->e_lwp_fork)
		(*p2->p_emul->e_lwp_fork)(l1, l2);

	return (0);
}

/*
 * Set a new LWP running.  If the process is stopping, then the LWP is
 * created stopped.
 */
void
lwp_start(lwp_t *l, int flags)
{
	proc_t *p = l->l_proc;

	mutex_enter(p->p_lock);
	lwp_lock(l);
	KASSERT(l->l_stat == LSIDL);
	if ((flags & LWP_SUSPENDED) != 0) {
		/* It'll suspend itself in lwp_userret(). */
		l->l_flag |= LW_WSUSPEND;
	}
	if (p->p_stat == SSTOP || (p->p_sflag & PS_STOPPING) != 0) {
		KASSERT(l->l_wchan == NULL);
	    	l->l_stat = LSSTOP;
		p->p_nrlwps--;
		lwp_unlock(l);
	} else {
		setrunnable(l);
		/* LWP now unlocked */
	}
	mutex_exit(p->p_lock);
}

/*
 * Called by MD code when a new LWP begins execution.  Must be called
 * with the previous LWP locked (so at splsched), or if there is no
 * previous LWP, at splsched.
 */
void
lwp_startup(struct lwp *prev, struct lwp *new_lwp)
{
	kmutex_t *lock;

	KASSERTMSG(new_lwp == curlwp, "l %p curlwp %p prevlwp %p", new_lwp, curlwp, prev);
	KASSERT(kpreempt_disabled());
	KASSERT(prev != NULL);
	KASSERT((prev->l_pflag & LP_RUNNING) != 0);
	KASSERT(curcpu()->ci_mtx_count == -2);

	/*
	 * Immediately mark the previous LWP as no longer running and
	 * unlock (to keep lock wait times short as possible).  If a
	 * zombie, don't touch after clearing LP_RUNNING as it could be
	 * reaped by another CPU.  Use atomic_store_release to ensure
	 * this -- matches atomic_load_acquire in lwp_free.
	 */
	lock = prev->l_mutex;
	if (__predict_false(prev->l_stat == LSZOMB)) {
		atomic_store_release(&prev->l_pflag,
		    prev->l_pflag & ~LP_RUNNING);
	} else {
		prev->l_pflag &= ~LP_RUNNING;
	}
	mutex_spin_exit(lock);

	/* Correct spin mutex count after mi_switch(). */
	curcpu()->ci_mtx_count = 0;

	/* Install new VM context. */
	if (__predict_true(new_lwp->l_proc->p_vmspace)) {
		pmap_activate(new_lwp);
	}

	/* We remain at IPL_SCHED from mi_switch() - reset it. */
	spl0();

	LOCKDEBUG_BARRIER(NULL, 0);
	SDT_PROBE(proc, kernel, , lwp__start, new_lwp, 0, 0, 0, 0);

	/* For kthreads, acquire kernel lock if not MPSAFE. */
	if (__predict_false((new_lwp->l_pflag & LP_MPSAFE) == 0)) {
		KERNEL_LOCK(1, new_lwp);
	}
}

/*
 * Exit an LWP.
 *
 * *** WARNING *** This can be called with (l != curlwp) in error paths.
 */
void
lwp_exit(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct lwp *l2;
	bool current;

	current = (l == curlwp);

	KASSERT(current || (l->l_stat == LSIDL && l->l_target_cpu == NULL));
	KASSERT(p == curproc);

	SDT_PROBE(proc, kernel, , lwp__exit, l, 0, 0, 0, 0);

	/* Verify that we hold no locks; for DIAGNOSTIC check kernel_lock. */
	LOCKDEBUG_BARRIER(NULL, 0);
	KASSERTMSG(curcpu()->ci_biglock_count == 0, "kernel_lock leaked");

	/*
	 * If we are the last live LWP in a process, we need to exit the
	 * entire process.  We do so with an exit status of zero, because
	 * it's a "controlled" exit, and because that's what Solaris does.
	 *
	 * We are not quite a zombie yet, but for accounting purposes we
	 * must increment the count of zombies here.
	 *
	 * Note: the last LWP's specificdata will be deleted here.
	 */
	mutex_enter(p->p_lock);
	if (p->p_nlwps - p->p_nzlwps == 1) {
		KASSERT(current == true);
		KASSERT(p != &proc0);
		exit1(l, 0, 0);
		/* NOTREACHED */
	}
	p->p_nzlwps++;

	/*
	 * Perform any required thread cleanup.  Do this early so
	 * anyone wanting to look us up with lwp_getref_lwpid() will
	 * fail to find us before we become a zombie.
	 *
	 * N.B. this will unlock p->p_lock on our behalf.
	 */
	lwp_thread_cleanup(l);

	if (p->p_emul->e_lwp_exit)
		(*p->p_emul->e_lwp_exit)(l);

	/* Drop filedesc reference. */
	fd_free();

	/* Release fstrans private data. */
	fstrans_lwp_dtor(l);

	/* Delete the specificdata while it's still safe to sleep. */
	lwp_finispecific(l);

	/*
	 * Release our cached credentials.
	 */
	kauth_cred_free(l->l_cred);
	callout_destroy(&l->l_timeout_ch);

	/*
	 * If traced, report LWP exit event to the debugger.
	 *
	 * Remove the LWP from the global list.
	 * Free its LID from the PID namespace if needed.
	 */
	mutex_enter(&proc_lock);

	if ((p->p_slflag & (PSL_TRACED|PSL_TRACELWP_EXIT)) ==
	    (PSL_TRACED|PSL_TRACELWP_EXIT)) {
		mutex_enter(p->p_lock);
		if (ISSET(p->p_sflag, PS_WEXIT)) {
			mutex_exit(p->p_lock);
			/*
			 * We are exiting, bail out without informing parent
			 * about a terminating LWP as it would deadlock.
			 */
		} else {
			eventswitch(TRAP_LWP, PTRACE_LWP_EXIT, l->l_lid);
			mutex_enter(&proc_lock);
		}
	}

	LIST_REMOVE(l, l_list);
	mutex_exit(&proc_lock);

	/*
	 * Get rid of all references to the LWP that others (e.g. procfs)
	 * may have, and mark the LWP as a zombie.  If the LWP is detached,
	 * mark it waiting for collection in the proc structure.  Note that
	 * before we can do that, we need to free any other dead, deatched
	 * LWP waiting to meet its maker.
	 *
	 * All conditions need to be observed upon under the same hold of
	 * p_lock, because if the lock is dropped any of them can change.
	 */
	mutex_enter(p->p_lock);
	for (;;) {
		if (lwp_drainrefs(l))
			continue;
		if ((l->l_prflag & LPR_DETACHED) != 0) {
			if ((l2 = p->p_zomblwp) != NULL) {
				p->p_zomblwp = NULL;
				lwp_free(l2, false, false);
				/* proc now unlocked */
				mutex_enter(p->p_lock);
				continue;
			}
			p->p_zomblwp = l;
		}
		break;
	}

	/*
	 * If we find a pending signal for the process and we have been
	 * asked to check for signals, then we lose: arrange to have
	 * all other LWPs in the process check for signals.
	 */
	if ((l->l_flag & LW_PENDSIG) != 0 &&
	    firstsig(&p->p_sigpend.sp_set) != 0) {
		LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
			lwp_lock(l2);
			signotify(l2);
			lwp_unlock(l2);
		}
	}

	/*
	 * Release any PCU resources before becoming a zombie.
	 */
	pcu_discard_all(l);

	lwp_lock(l);
	l->l_stat = LSZOMB;
	if (l->l_name != NULL) {
		strcpy(l->l_name, "(zombie)");
	}
	lwp_unlock(l);
	p->p_nrlwps--;
	cv_broadcast(&p->p_lwpcv);
	if (l->l_lwpctl != NULL)
		l->l_lwpctl->lc_curcpu = LWPCTL_CPU_EXITED;
	mutex_exit(p->p_lock);

	/*
	 * We can no longer block.  At this point, lwp_free() may already
	 * be gunning for us.  On a multi-CPU system, we may be off p_lwps.
	 *
	 * Free MD LWP resources.
	 */
	cpu_lwp_free(l, 0);

	if (current) {
		/* Switch away into oblivion. */
		lwp_lock(l);
		spc_lock(l->l_cpu);
		mi_switch(l);
		panic("lwp_exit");
	}
}

/*
 * Free a dead LWP's remaining resources.
 *
 * XXXLWP limits.
 */
void
lwp_free(struct lwp *l, bool recycle, bool last)
{
	struct proc *p = l->l_proc;
	struct rusage *ru;
	ksiginfoq_t kq;

	KASSERT(l != curlwp);
	KASSERT(last || mutex_owned(p->p_lock));

	/*
	 * We use the process credentials instead of the lwp credentials here
	 * because the lwp credentials maybe cached (just after a setuid call)
	 * and we don't want pay for syncing, since the lwp is going away
	 * anyway
	 */
	if (p != &proc0 && p->p_nlwps != 1)
		(void)chglwpcnt(kauth_cred_getuid(p->p_cred), -1);

	/*
	 * In the unlikely event that the LWP is still on the CPU,
	 * then spin until it has switched away.
	 *
	 * atomic_load_acquire matches atomic_store_release in
	 * lwp_startup and mi_switch.
	 */
	while (__predict_false((atomic_load_acquire(&l->l_pflag) & LP_RUNNING)
		!= 0)) {
		SPINLOCK_BACKOFF_HOOK;
	}

	/*
	 * Now that the LWP's known off the CPU, reset its state back to
	 * LSIDL, which defeats anything that might have gotten a hold on
	 * the LWP via pid_table before the ID was freed.  It's important
	 * to do this with both the LWP locked and p_lock held.
	 *
	 * Also reset the CPU and lock pointer back to curcpu(), since the
	 * LWP will in all likelyhood be cached with the current CPU in
	 * lwp_cache when we free it and later allocated from there again
	 * (avoid incidental lock contention).
	 */
	lwp_lock(l);
	l->l_stat = LSIDL;
	l->l_cpu = curcpu();
	lwp_unlock_to(l, l->l_cpu->ci_schedstate.spc_lwplock);

	/*
	 * If this was not the last LWP in the process, then adjust counters
	 * and unlock.  This is done differently for the last LWP in exit1().
	 */
	if (!last) {
		/*
		 * Add the LWP's run time to the process' base value.
		 * This needs to co-incide with coming off p_lwps.
		 */
		bintime_add(&p->p_rtime, &l->l_rtime);
		p->p_pctcpu += l->l_pctcpu;
		ru = &p->p_stats->p_ru;
		ruadd(ru, &l->l_ru);
		ru->ru_nvcsw += (l->l_ncsw - l->l_nivcsw);
		ru->ru_nivcsw += l->l_nivcsw;
		LIST_REMOVE(l, l_sibling);
		p->p_nlwps--;
		p->p_nzlwps--;
		if ((l->l_prflag & LPR_DETACHED) != 0)
			p->p_ndlwps--;

		/*
		 * Have any LWPs sleeping in lwp_wait() recheck for
		 * deadlock.
		 */
		cv_broadcast(&p->p_lwpcv);
		mutex_exit(p->p_lock);

		/* Free the LWP ID. */
		mutex_enter(&proc_lock);
		proc_free_lwpid(p, l->l_lid);
		mutex_exit(&proc_lock);
	}

	/*
	 * Destroy the LWP's remaining signal information.
	 */
	ksiginfo_queue_init(&kq);
	sigclear(&l->l_sigpend, NULL, &kq);
	ksiginfo_queue_drain(&kq);
	cv_destroy(&l->l_sigcv);
	cv_destroy(&l->l_waitcv);

	/*
	 * Free lwpctl structure and affinity.
	 */
	if (l->l_lwpctl) {
		lwp_ctl_free(l);
	}
	if (l->l_affinity) {
		kcpuset_unuse(l->l_affinity, NULL);
		l->l_affinity = NULL;
	}

	/*
	 * Free remaining data structures and the LWP itself unless the
	 * caller wants to recycle.
	 */
	if (l->l_name != NULL)
		kmem_free(l->l_name, MAXCOMLEN);

	kmsan_lwp_free(l);
	kcov_lwp_free(l);
	cpu_lwp_free2(l);
	uvm_lwp_exit(l);

	KASSERT(SLIST_EMPTY(&l->l_pi_lenders));
	KASSERT(l->l_inheritedprio == -1);
	KASSERT(l->l_blcnt == 0);
	kdtrace_thread_dtor(NULL, l);
	if (!recycle)
		pool_cache_put(lwp_cache, l);
}

/*
 * Migrate the LWP to the another CPU.  Unlocks the LWP.
 */
void
lwp_migrate(lwp_t *l, struct cpu_info *tci)
{
	struct schedstate_percpu *tspc;
	int lstat = l->l_stat;

	KASSERT(lwp_locked(l, NULL));
	KASSERT(tci != NULL);

	/* If LWP is still on the CPU, it must be handled like LSONPROC */
	if ((l->l_pflag & LP_RUNNING) != 0) {
		lstat = LSONPROC;
	}

	/*
	 * The destination CPU could be changed while previous migration
	 * was not finished.
	 */
	if (l->l_target_cpu != NULL) {
		l->l_target_cpu = tci;
		lwp_unlock(l);
		return;
	}

	/* Nothing to do if trying to migrate to the same CPU */
	if (l->l_cpu == tci) {
		lwp_unlock(l);
		return;
	}

	KASSERT(l->l_target_cpu == NULL);
	tspc = &tci->ci_schedstate;
	switch (lstat) {
	case LSRUN:
		l->l_target_cpu = tci;
		break;
	case LSSLEEP:
		l->l_cpu = tci;
		break;
	case LSIDL:
	case LSSTOP:
	case LSSUSPENDED:
		l->l_cpu = tci;
		if (l->l_wchan == NULL) {
			lwp_unlock_to(l, tspc->spc_lwplock);
			return;
		}
		break;
	case LSONPROC:
		l->l_target_cpu = tci;
		spc_lock(l->l_cpu);
		sched_resched_cpu(l->l_cpu, PRI_USER_RT, true);
		/* spc now unlocked */
		break;
	}
	lwp_unlock(l);
}

#define	lwp_find_exclude(l)					\
	((l)->l_stat == LSIDL || (l)->l_stat == LSZOMB)

/*
 * Find the LWP in the process.  Arguments may be zero, in such case,
 * the calling process and first LWP in the list will be used.
 * On success - returns proc locked.
 *
 * => pid == 0 -> look in curproc.
 * => pid == -1 -> match any proc.
 * => otherwise look up the proc.
 *
 * => lid == 0 -> first LWP in the proc
 * => otherwise specific LWP
 */
struct lwp *
lwp_find2(pid_t pid, lwpid_t lid)
{
	proc_t *p;
	lwp_t *l;

	/* First LWP of specified proc. */
	if (lid == 0) {
		switch (pid) {
		case -1:
			/* No lookup keys. */
			return NULL;
		case 0:
			p = curproc;
			mutex_enter(p->p_lock);
			break;
		default:
			mutex_enter(&proc_lock);
			p = proc_find(pid);
			if (__predict_false(p == NULL)) {
				mutex_exit(&proc_lock);
				return NULL;
			}
			mutex_enter(p->p_lock);
			mutex_exit(&proc_lock);
			break;
		}
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if (__predict_true(!lwp_find_exclude(l)))
				break;
		}
		goto out;
	}

	l = proc_find_lwp_acquire_proc(lid, &p);
	if (l == NULL)
		return NULL;
	KASSERT(p != NULL);
	KASSERT(mutex_owned(p->p_lock));

	if (__predict_false(lwp_find_exclude(l))) {
		l = NULL;
		goto out;
	}

	/* Apply proc filter, if applicable. */
	switch (pid) {
	case -1:
		/* Match anything. */
		break;
	case 0:
		if (p != curproc)
			l = NULL;
		break;
	default:
		if (p->p_pid != pid)
			l = NULL;
		break;
	}

 out:
	if (__predict_false(l == NULL)) {
		mutex_exit(p->p_lock);
	}
	return l;
}

/*
 * Look up a live LWP within the specified process.
 *
 * Must be called with p->p_lock held (as it looks at the radix tree,
 * and also wants to exclude idle and zombie LWPs).
 */
struct lwp *
lwp_find(struct proc *p, lwpid_t id)
{
	struct lwp *l;

	KASSERT(mutex_owned(p->p_lock));

	l = proc_find_lwp(p, id);
	KASSERT(l == NULL || l->l_lid == id);

	/*
	 * No need to lock - all of these conditions will
	 * be visible with the process level mutex held.
	 */
	if (__predict_false(l != NULL && lwp_find_exclude(l)))
		l = NULL;

	return l;
}

/*
 * Update an LWP's cached credentials to mirror the process' master copy.
 *
 * This happens early in the syscall path, on user trap, and on LWP
 * creation.  A long-running LWP can also voluntarily choose to update
 * its credentials by calling this routine.  This may be called from
 * LWP_CACHE_CREDS(), which checks l->l_cred != p->p_cred beforehand.
 */
void
lwp_update_creds(struct lwp *l)
{
	kauth_cred_t oc;
	struct proc *p;

	p = l->l_proc;
	oc = l->l_cred;

	mutex_enter(p->p_lock);
	kauth_cred_hold(p->p_cred);
	l->l_cred = p->p_cred;
	l->l_prflag &= ~LPR_CRMOD;
	mutex_exit(p->p_lock);
	if (oc != NULL)
		kauth_cred_free(oc);
}

/*
 * Verify that an LWP is locked, and optionally verify that the lock matches
 * one we specify.
 */
int
lwp_locked(struct lwp *l, kmutex_t *mtx)
{
	kmutex_t *cur = l->l_mutex;

	return mutex_owned(cur) && (mtx == cur || mtx == NULL);
}

/*
 * Lend a new mutex to an LWP.  The old mutex must be held.
 */
kmutex_t *
lwp_setlock(struct lwp *l, kmutex_t *mtx)
{
	kmutex_t *oldmtx = l->l_mutex;

	KASSERT(mutex_owned(oldmtx));

	atomic_store_release(&l->l_mutex, mtx);
	return oldmtx;
}

/*
 * Lend a new mutex to an LWP, and release the old mutex.  The old mutex
 * must be held.
 */
void
lwp_unlock_to(struct lwp *l, kmutex_t *mtx)
{
	kmutex_t *old;

	KASSERT(lwp_locked(l, NULL));

	old = l->l_mutex;
	atomic_store_release(&l->l_mutex, mtx);
	mutex_spin_exit(old);
}

int
lwp_trylock(struct lwp *l)
{
	kmutex_t *old;

	for (;;) {
		if (!mutex_tryenter(old = atomic_load_consume(&l->l_mutex)))
			return 0;
		if (__predict_true(atomic_load_relaxed(&l->l_mutex) == old))
			return 1;
		mutex_spin_exit(old);
	}
}

void
lwp_unsleep(lwp_t *l, bool unlock)
{

	KASSERT(mutex_owned(l->l_mutex));
	(*l->l_syncobj->sobj_unsleep)(l, unlock);
}

/*
 * Handle exceptions for mi_userret().  Called if a member of LW_USERRET is
 * set.
 */
void
lwp_userret(struct lwp *l)
{
	struct proc *p;
	int sig;

	KASSERT(l == curlwp);
	KASSERT(l->l_stat == LSONPROC);
	p = l->l_proc;

	/*
	 * It is safe to do this read unlocked on a MP system..
	 */
	while ((l->l_flag & LW_USERRET) != 0) {
		/*
		 * Process pending signals first, unless the process
		 * is dumping core or exiting, where we will instead
		 * enter the LW_WSUSPEND case below.
		 */
		if ((l->l_flag & (LW_PENDSIG | LW_WCORE | LW_WEXIT)) ==
		    LW_PENDSIG) {
			mutex_enter(p->p_lock);
			while ((sig = issignal(l)) != 0)
				postsig(sig);
			mutex_exit(p->p_lock);
		}

		/*
		 * Core-dump or suspend pending.
		 *
		 * In case of core dump, suspend ourselves, so that the kernel
		 * stack and therefore the userland registers saved in the
		 * trapframe are around for coredump() to write them out.
		 * We also need to save any PCU resources that we have so that
		 * they accessible for coredump().  We issue a wakeup on
		 * p->p_lwpcv so that sigexit() will write the core file out
		 * once all other LWPs are suspended.  
		 */
		if ((l->l_flag & LW_WSUSPEND) != 0) {
			pcu_save_all(l);
			mutex_enter(p->p_lock);
			p->p_nrlwps--;
			cv_broadcast(&p->p_lwpcv);
			lwp_lock(l);
			l->l_stat = LSSUSPENDED;
			lwp_unlock(l);
			mutex_exit(p->p_lock);
			lwp_lock(l);
			spc_lock(l->l_cpu);
			mi_switch(l);
		}

		/* Process is exiting. */
		if ((l->l_flag & LW_WEXIT) != 0) {
			lwp_exit(l);
			KASSERT(0);
			/* NOTREACHED */
		}

		/* update lwpctl processor (for vfork child_return) */
		if (l->l_flag & LW_LWPCTL) {
			lwp_lock(l);
			KASSERT(kpreempt_disabled());
			l->l_lwpctl->lc_curcpu = (int)cpu_index(l->l_cpu);
			l->l_lwpctl->lc_pctr++;
			l->l_flag &= ~LW_LWPCTL;
			lwp_unlock(l);
		}
	}
}

/*
 * Force an LWP to enter the kernel, to take a trip through lwp_userret().
 */
void
lwp_need_userret(struct lwp *l)
{

	KASSERT(!cpu_intr_p());
	KASSERT(lwp_locked(l, NULL));

	/*
	 * If the LWP is in any state other than LSONPROC, we know that it
	 * is executing in-kernel and will hit userret() on the way out. 
	 *
	 * If the LWP is curlwp, then we know we'll be back out to userspace
	 * soon (can't be called from a hardware interrupt here).
	 *
	 * Otherwise, we can't be sure what the LWP is doing, so first make
	 * sure the update to l_flag will be globally visible, and then
	 * force the LWP to take a trip through trap() where it will do
	 * userret().
	 */
	if (l->l_stat == LSONPROC && l != curlwp) {
		membar_producer();
		cpu_signotify(l);
	}
}

/*
 * Add one reference to an LWP.  This will prevent the LWP from
 * exiting, thus keep the lwp structure and PCB around to inspect.
 */
void
lwp_addref(struct lwp *l)
{
	KASSERT(mutex_owned(l->l_proc->p_lock));
	KASSERT(l->l_stat != LSZOMB);
	l->l_refcnt++;
}

/*
 * Remove one reference to an LWP.  If this is the last reference,
 * then we must finalize the LWP's death.
 */
void
lwp_delref(struct lwp *l)
{
	struct proc *p = l->l_proc;

	mutex_enter(p->p_lock);
	lwp_delref2(l);
	mutex_exit(p->p_lock);
}

/*
 * Remove one reference to an LWP.  If this is the last reference,
 * then we must finalize the LWP's death.  The proc mutex is held
 * on entry.
 */
void
lwp_delref2(struct lwp *l)
{
	struct proc *p = l->l_proc;

	KASSERT(mutex_owned(p->p_lock));
	KASSERT(l->l_stat != LSZOMB);
	KASSERT(l->l_refcnt > 0);

	if (--l->l_refcnt == 0)
		cv_broadcast(&p->p_lwpcv);
}

/*
 * Drain all references to the current LWP.  Returns true if
 * we blocked.
 */
bool
lwp_drainrefs(struct lwp *l)
{
	struct proc *p = l->l_proc;
	bool rv = false;

	KASSERT(mutex_owned(p->p_lock));

	l->l_prflag |= LPR_DRAINING;

	while (l->l_refcnt > 0) {
		rv = true;
		cv_wait(&p->p_lwpcv, p->p_lock);
	}
	return rv;
}

/*
 * Return true if the specified LWP is 'alive'.  Only p->p_lock need
 * be held.
 */
bool
lwp_alive(lwp_t *l)
{

	KASSERT(mutex_owned(l->l_proc->p_lock));

	switch (l->l_stat) {
	case LSSLEEP:
	case LSRUN:
	case LSONPROC:
	case LSSTOP:
	case LSSUSPENDED:
		return true;
	default:
		return false;
	}
}

/*
 * Return first live LWP in the process.
 */
lwp_t *
lwp_find_first(proc_t *p)
{
	lwp_t *l;

	KASSERT(mutex_owned(p->p_lock));

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		if (lwp_alive(l)) {
			return l;
		}
	}

	return NULL;
}

/*
 * Allocate a new lwpctl structure for a user LWP.
 */
int
lwp_ctl_alloc(vaddr_t *uaddr)
{
	lcproc_t *lp;
	u_int bit, i, offset;
	struct uvm_object *uao;
	int error;
	lcpage_t *lcp;
	proc_t *p;
	lwp_t *l;

	l = curlwp;
	p = l->l_proc;

	/* don't allow a vforked process to create lwp ctls */
	if (p->p_lflag & PL_PPWAIT)
		return EBUSY;

	if (l->l_lcpage != NULL) {
		lcp = l->l_lcpage;
		*uaddr = lcp->lcp_uaddr + (vaddr_t)l->l_lwpctl - lcp->lcp_kaddr;
		return 0;
	}

	/* First time around, allocate header structure for the process. */
	if ((lp = p->p_lwpctl) == NULL) {
		lp = kmem_alloc(sizeof(*lp), KM_SLEEP);
		mutex_init(&lp->lp_lock, MUTEX_DEFAULT, IPL_NONE);
		lp->lp_uao = NULL;
		TAILQ_INIT(&lp->lp_pages);
		mutex_enter(p->p_lock);
		if (p->p_lwpctl == NULL) {
			p->p_lwpctl = lp;
			mutex_exit(p->p_lock);
		} else {
			mutex_exit(p->p_lock);
			mutex_destroy(&lp->lp_lock);
			kmem_free(lp, sizeof(*lp));
			lp = p->p_lwpctl;
		}
	}

 	/*
 	 * Set up an anonymous memory region to hold the shared pages.
 	 * Map them into the process' address space.  The user vmspace
 	 * gets the first reference on the UAO.
 	 */
	mutex_enter(&lp->lp_lock);
	if (lp->lp_uao == NULL) {
		lp->lp_uao = uao_create(LWPCTL_UAREA_SZ, 0);
		lp->lp_cur = 0;
		lp->lp_max = LWPCTL_UAREA_SZ;
		lp->lp_uva = p->p_emul->e_vm_default_addr(p,
		     (vaddr_t)p->p_vmspace->vm_daddr, LWPCTL_UAREA_SZ,
		     p->p_vmspace->vm_map.flags & VM_MAP_TOPDOWN);
		error = uvm_map(&p->p_vmspace->vm_map, &lp->lp_uva,
		    LWPCTL_UAREA_SZ, lp->lp_uao, 0, 0, UVM_MAPFLAG(UVM_PROT_RW,
		    UVM_PROT_RW, UVM_INH_NONE, UVM_ADV_NORMAL, 0));
		if (error != 0) {
			uao_detach(lp->lp_uao);
			lp->lp_uao = NULL;
			mutex_exit(&lp->lp_lock);
			return error;
		}
	}

	/* Get a free block and allocate for this LWP. */
	TAILQ_FOREACH(lcp, &lp->lp_pages, lcp_chain) {
		if (lcp->lcp_nfree != 0)
			break;
	}
	if (lcp == NULL) {
		/* Nothing available - try to set up a free page. */
		if (lp->lp_cur == lp->lp_max) {
			mutex_exit(&lp->lp_lock);
			return ENOMEM;
		}
		lcp = kmem_alloc(LWPCTL_LCPAGE_SZ, KM_SLEEP);

		/*
		 * Wire the next page down in kernel space.  Since this
		 * is a new mapping, we must add a reference.
		 */
		uao = lp->lp_uao;
		(*uao->pgops->pgo_reference)(uao);
		lcp->lcp_kaddr = vm_map_min(kernel_map);
		error = uvm_map(kernel_map, &lcp->lcp_kaddr, PAGE_SIZE,
		    uao, lp->lp_cur, PAGE_SIZE,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
		    UVM_INH_NONE, UVM_ADV_RANDOM, 0));
		if (error != 0) {
			mutex_exit(&lp->lp_lock);
			kmem_free(lcp, LWPCTL_LCPAGE_SZ);
			(*uao->pgops->pgo_detach)(uao);
			return error;
		}
		error = uvm_map_pageable(kernel_map, lcp->lcp_kaddr,
		    lcp->lcp_kaddr + PAGE_SIZE, FALSE, 0);
		if (error != 0) {
			mutex_exit(&lp->lp_lock);
			uvm_unmap(kernel_map, lcp->lcp_kaddr,
			    lcp->lcp_kaddr + PAGE_SIZE);
			kmem_free(lcp, LWPCTL_LCPAGE_SZ);
			return error;
		}
		/* Prepare the page descriptor and link into the list. */
		lcp->lcp_uaddr = lp->lp_uva + lp->lp_cur;
		lp->lp_cur += PAGE_SIZE;
		lcp->lcp_nfree = LWPCTL_PER_PAGE;
		lcp->lcp_rotor = 0;
		memset(lcp->lcp_bitmap, 0xff, LWPCTL_BITMAP_SZ);
		TAILQ_INSERT_HEAD(&lp->lp_pages, lcp, lcp_chain);
	}
	for (i = lcp->lcp_rotor; lcp->lcp_bitmap[i] == 0;) {
		if (++i >= LWPCTL_BITMAP_ENTRIES)
			i = 0;
	}
	bit = ffs(lcp->lcp_bitmap[i]) - 1;
	lcp->lcp_bitmap[i] ^= (1U << bit);
	lcp->lcp_rotor = i;
	lcp->lcp_nfree--;
	l->l_lcpage = lcp;
	offset = (i << 5) + bit;
	l->l_lwpctl = (lwpctl_t *)lcp->lcp_kaddr + offset;
	*uaddr = lcp->lcp_uaddr + offset * sizeof(lwpctl_t);
	mutex_exit(&lp->lp_lock);

	KPREEMPT_DISABLE(l);
	l->l_lwpctl->lc_curcpu = (int)cpu_index(curcpu());
	KPREEMPT_ENABLE(l);

	return 0;
}

/*
 * Free an lwpctl structure back to the per-process list.
 */
void
lwp_ctl_free(lwp_t *l)
{
	struct proc *p = l->l_proc;
	lcproc_t *lp;
	lcpage_t *lcp;
	u_int map, offset;

	/* don't free a lwp context we borrowed for vfork */
	if (p->p_lflag & PL_PPWAIT) {
		l->l_lwpctl = NULL;
		return;
	}

	lp = p->p_lwpctl;
	KASSERT(lp != NULL);

	lcp = l->l_lcpage;
	offset = (u_int)((lwpctl_t *)l->l_lwpctl - (lwpctl_t *)lcp->lcp_kaddr);
	KASSERT(offset < LWPCTL_PER_PAGE);

	mutex_enter(&lp->lp_lock);
	lcp->lcp_nfree++;
	map = offset >> 5;
	lcp->lcp_bitmap[map] |= (1U << (offset & 31));
	if (lcp->lcp_bitmap[lcp->lcp_rotor] == 0)
		lcp->lcp_rotor = map;
	if (TAILQ_FIRST(&lp->lp_pages)->lcp_nfree == 0) {
		TAILQ_REMOVE(&lp->lp_pages, lcp, lcp_chain);
		TAILQ_INSERT_HEAD(&lp->lp_pages, lcp, lcp_chain);
	}
	mutex_exit(&lp->lp_lock);
}

/*
 * Process is exiting; tear down lwpctl state.  This can only be safely
 * called by the last LWP in the process.
 */
void
lwp_ctl_exit(void)
{
	lcpage_t *lcp, *next;
	lcproc_t *lp;
	proc_t *p;
	lwp_t *l;

	l = curlwp;
	l->l_lwpctl = NULL;
	l->l_lcpage = NULL;
	p = l->l_proc;
	lp = p->p_lwpctl;

	KASSERT(lp != NULL);
	KASSERT(p->p_nlwps == 1);

	for (lcp = TAILQ_FIRST(&lp->lp_pages); lcp != NULL; lcp = next) {
		next = TAILQ_NEXT(lcp, lcp_chain);
		uvm_unmap(kernel_map, lcp->lcp_kaddr,
		    lcp->lcp_kaddr + PAGE_SIZE);
		kmem_free(lcp, LWPCTL_LCPAGE_SZ);
	}

	if (lp->lp_uao != NULL) {
		uvm_unmap(&p->p_vmspace->vm_map, lp->lp_uva,
		    lp->lp_uva + LWPCTL_UAREA_SZ);
	}

	mutex_destroy(&lp->lp_lock);
	kmem_free(lp, sizeof(*lp));
	p->p_lwpctl = NULL;
}

/*
 * Return the current LWP's "preemption counter".  Used to detect
 * preemption across operations that can tolerate preemption without
 * crashing, but which may generate incorrect results if preempted.
 */
uint64_t
lwp_pctr(void)
{

	return curlwp->l_ncsw;
}

/*
 * Set an LWP's private data pointer.
 */
int
lwp_setprivate(struct lwp *l, void *ptr)
{
	int error = 0;

	l->l_private = ptr;
#ifdef __HAVE_CPU_LWP_SETPRIVATE
	error = cpu_lwp_setprivate(l, ptr);
#endif
	return error;
}

/*
 * Perform any thread-related cleanup on LWP exit.
 * N.B. l->l_proc->p_lock must be HELD on entry but will
 * be released before returning!
 */
void
lwp_thread_cleanup(struct lwp *l)
{

	KASSERT(mutex_owned(l->l_proc->p_lock));
	mutex_exit(l->l_proc->p_lock);

	/*
	 * If the LWP has robust futexes, release them all
	 * now.
	 */
	if (__predict_false(l->l_robust_head != 0)) {
		futex_release_all_lwp(l);
	}
}

#if defined(DDB)
#include <machine/pcb.h>

void
lwp_whatis(uintptr_t addr, void (*pr)(const char *, ...))
{
	lwp_t *l;

	LIST_FOREACH(l, &alllwp, l_list) {
		uintptr_t stack = (uintptr_t)KSTACK_LOWEST_ADDR(l);

		if (addr < stack || stack + KSTACK_SIZE <= addr) {
			continue;
		}
		(*pr)("%p is %p+%zu, LWP %p's stack\n",
		    (void *)addr, (void *)stack,
		    (size_t)(addr - stack), l);
	}
}
#endif /* defined(DDB) */
