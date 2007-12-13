/*	$NetBSD: kern_lwp.c,v 1.85 2007/12/13 05:25:03 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
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
 * 	LSONPROC
 * 
 * 		On processor: the LWP is executing on a CPU, either in the
 * 		kernel or in user space.
 * 
 * 	LSRUN
 * 
 * 		Runnable: the LWP is parked on a run queue, and may soon be
 * 		chosen to run by a idle processor, or by a processor that
 * 		has been asked to preempt a currently runnning but lower
 * 		priority LWP.  If the LWP is not swapped in (L_INMEM == 0)
 *		then the LWP is not on a run queue, but may be soon.
 * 
 * 	LSIDL
 * 
 * 		Idle: the LWP has been created but has not yet executed,
 *		or it has ceased executing a unit of work and is waiting
 *		to be started again.
 * 
 * 	LSSUSPENDED:
 * 
 * 		Suspended: the LWP has had its execution suspended by
 *		another LWP in the same process using the _lwp_suspend()
 *		system call.  User-level LWPs also enter the suspended
 *		state when the system is shutting down.
 *
 *	The second set represent a "statement of intent" on behalf of the
 *	LWP.  The LWP may in fact be executing on a processor, may be
 *	sleeping or idle. It is expected to take the necessary action to
 *	stop executing or become "running" again within	a short timeframe.
 *	The LW_RUNNING flag in lwp::l_flag indicates that an LWP is running.
 *	Importantly, in indicates that its state is tied to a CPU.
 * 
 * 	LSZOMB:
 * 
 * 		Dead or dying: the LWP has released most of its resources
 *		and is a) about to switch away into oblivion b) has already
 *		switched away.  When it switches away, its few remaining
 *		resources can be collected.
 * 
 * 	LSSLEEP:
 * 
 * 		Sleeping: the LWP has entered itself onto a sleep queue, and
 * 		has switched away or will switch away shortly to allow other
 *		LWPs to run on the CPU.
 * 
 * 	LSSTOP:
 * 
 * 		Stopped: the LWP has been stopped as a result of a job
 * 		control signal, or as a result of the ptrace() interface. 
 *
 * 		Stopped LWPs may run briefly within the kernel to handle
 * 		signals that they receive, but will not return to user space
 * 		until their process' state is changed away from stopped. 
 *
 * 		Single LWPs within a process can not be set stopped
 * 		selectively: all actions that can stop or continue LWPs
 * 		occur at the process level.
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
 *	            > STOPPED			    > SLEEP
 *	            > SUSPENDED			    > STOPPED
 *						    > SUSPENDED
 *						    > ZOMB
 *
 *	 STOPPED ---> RUN		SUSPENDED --> RUN
 *	            > SLEEP			    > SLEEP
 *
 *	 SLEEP -----> ONPROC		IDL --------> RUN
 *		    > RUN		            > SUSPENDED
 *		    > STOPPED                       > STOPPED
 *		    > SUSPENDED
 *
 *	Other state transitions are possible with kernel threads (eg
 *	ONPROC -> IDL), but only happen under tightly controlled
 *	circumstances the side effects are understood.
 *
 * Locking
 *
 *	The majority of fields in 'struct lwp' are covered by a single,
 *	general spin lock pointed to by lwp::l_mutex.  The locks covering
 *	each field are documented in sys/lwp.h.
 *
 *	State transitions must be made with the LWP's general lock held,
 * 	and may cause the LWP's lock pointer to change. Manipulation of
 *	the general lock is not performed directly, but through calls to
 *	lwp_lock(), lwp_relock() and similar.
 *
 *	States and their associated locks:
 *
 *	LSONPROC, LSZOMB:
 *
 *		Always covered by spc_lwplock, which protects running LWPs.
 *		This is a per-CPU lock.
 *
 *	LSIDL, LSRUN:
 *
 *		Always covered by spc_mutex, which protects the run queues.
 *		This may be a per-CPU lock, depending on the scheduler.
 *
 *	LSSLEEP:
 *
 *		Covered by a lock associated with the sleep queue that the
 *		LWP resides on, indirectly referenced by l_sleepq->sq_mutex.
 *
 *	LSSTOP, LSSUSPENDED:
 *	
 *		If the LWP was previously sleeping (l_wchan != NULL), then
 *		l_mutex references the sleep queue lock.  If the LWP was
 *		runnable or on the CPU when halted, or has been removed from
 *		the sleep queue since halted, then the lock is spc_lwplock.
 *
 *	The lock order is as follows:
 *
 *		spc::spc_lwplock ->
 *		    sleepq_t::sq_mutex ->
 *			tschain_t::tc_mutex ->
 *			    spc::spc_mutex
 *
 *	Each process has an scheduler state lock (proc::p_smutex), and a
 *	number of counters on LWPs and their states: p_nzlwps, p_nrlwps, and
 *	so on.  When an LWP is to be entered into or removed from one of the
 *	following states, p_mutex must be held and the process wide counters
 *	adjusted:
 *
 *		LSIDL, LSZOMB, LSSTOP, LSSUSPENDED
 *
 *	Note that an LWP is considered running or likely to run soon if in
 *	one of the following states.  This affects the value of p_nrlwps:
 *
 *		LSRUN, LSONPROC, LSSLEEP
 *
 *	p_smutex does not need to be held when transitioning among these
 *	three states.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_lwp.c,v 1.85 2007/12/13 05:25:03 yamt Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

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
#include <sys/user.h>
#include <sys/lockdebug.h>
#include <sys/kmem.h>
#include <sys/intr.h>
#include <sys/lwpctl.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>

struct lwplist	alllwp = LIST_HEAD_INITIALIZER(alllwp);

POOL_INIT(lwp_pool, sizeof(struct lwp), MIN_LWP_ALIGNMENT, 0, 0, "lwppl",
    &pool_allocator_nointr, IPL_NONE);
POOL_INIT(lwp_uc_pool, sizeof(ucontext_t), 0, 0, 0, "lwpucpl",
    &pool_allocator_nointr, IPL_NONE);

static specificdata_domain_t lwp_specificdata_domain;

void
lwpinit(void)
{

	lwp_specificdata_domain = specificdata_domain_create();
	KASSERT(lwp_specificdata_domain != NULL);
	lwp_sys_init();
}

/*
 * Set an suspended.
 *
 * Must be called with p_smutex held, and the LWP locked.  Will unlock the
 * LWP before return.
 */
int
lwp_suspend(struct lwp *curl, struct lwp *t)
{
	int error;

	KASSERT(mutex_owned(&t->l_proc->p_smutex));
	KASSERT(lwp_locked(t, NULL));

	KASSERT(curl != t || curl->l_stat == LSONPROC);

	/*
	 * If the current LWP has been told to exit, we must not suspend anyone
	 * else or deadlock could occur.  We won't return to userspace.
	 */
	if ((curl->l_stat & (LW_WEXIT | LW_WCORE)) != 0) {
		lwp_unlock(t);
		return (EDEADLK);
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
 * Must be called with p_smutex held, and the LWP locked.  Will unlock the
 * LWP before return.
 */
void
lwp_continue(struct lwp *l)
{

	KASSERT(mutex_owned(&l->l_proc->p_smutex));
	KASSERT(lwp_locked(l, NULL));

	/* If rebooting or not suspended, then just bail out. */
	if ((l->l_flag & LW_WREBOOT) != 0) {
		lwp_unlock(l);
		return;
	}

	l->l_flag &= ~LW_WSUSPEND;

	if (l->l_stat != LSSUSPENDED) {
		lwp_unlock(l);
		return;
	}

	/* setrunnable() will release the lock. */
	setrunnable(l);
}

/*
 * Wait for an LWP within the current process to exit.  If 'lid' is
 * non-zero, we are waiting for a specific LWP.
 *
 * Must be called with p->p_smutex held.
 */
int
lwp_wait1(struct lwp *l, lwpid_t lid, lwpid_t *departed, int flags)
{
	struct proc *p = l->l_proc;
	struct lwp *l2;
	int nfound, error;
	lwpid_t curlid;
	bool exiting;

	KASSERT(mutex_owned(&p->p_smutex));

	p->p_nlwpwait++;
	l->l_waitingfor = lid;
	curlid = l->l_lid;
	exiting = ((flags & LWPWAIT_EXITCONTROL) != 0);

	for (;;) {
		/*
		 * Avoid a race between exit1() and sigexit(): if the
		 * process is dumping core, then we need to bail out: call
		 * into lwp_userret() where we will be suspended until the
		 * deed is done.
		 */
		if ((p->p_sflag & PS_WCORE) != 0) {
			mutex_exit(&p->p_smutex);
			lwp_userret(l);
#ifdef DIAGNOSTIC
			panic("lwp_wait1");
#endif
			/* NOTREACHED */
		}

		/*
		 * First off, drain any detached LWP that is waiting to be
		 * reaped.
		 */
		while ((l2 = p->p_zomblwp) != NULL) {
			p->p_zomblwp = NULL;
			lwp_free(l2, false, false);/* releases proc mutex */
			mutex_enter(&p->p_smutex);
		}

		/*
		 * Now look for an LWP to collect.  If the whole process is
		 * exiting, count detached LWPs as eligible to be collected,
		 * but don't drain them here.
		 */
		nfound = 0;
		error = 0;
		LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
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
				if (l2->l_lid != lid)
					continue;
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
			mutex_enter(&p->p_smutex);
			return 0;
		}

		if (error != 0)
			break;
		if (nfound == 0) {
			error = ESRCH;
			break;
		}

		/*
		 * The kernel is careful to ensure that it can not deadlock
		 * when exiting - just keep waiting.
		 */
		if (exiting) {
			KASSERT(p->p_nlwps > 1);
			cv_wait(&p->p_lwpcv, &p->p_smutex);
			continue;
		}

		/*
		 * If all other LWPs are waiting for exits or suspends
		 * and the supply of zombies and potential zombies is
		 * exhausted, then we are about to deadlock.
		 *
		 * If the process is exiting (and this LWP is not the one
		 * that is coordinating the exit) then bail out now.
		 */
		if ((p->p_sflag & PS_WEXIT) != 0 ||
		    p->p_nrlwps + p->p_nzlwps - p->p_ndlwps <= p->p_nlwpwait) {
			error = EDEADLK;
			break;
		}

		/*
		 * Sit around and wait for something to happen.  We'll be 
		 * awoken if any of the conditions examined change: if an
		 * LWP exits, is collected, or is detached.
		 */
		if ((error = cv_wait_sig(&p->p_lwpcv, &p->p_smutex)) != 0)
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
		LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
			if (l2->l_lid == lid) {
				if (l2->l_waiter == curlid)
					l2->l_waiter = 0;
				break;
			}
		}
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
lwp_create(lwp_t *l1, proc_t *p2, vaddr_t uaddr, bool inmem, int flags,
	   void *stack, size_t stacksize, void (*func)(void *), void *arg,
	   lwp_t **rnewlwpp, int sclass)
{
	struct lwp *l2, *isfree;
	turnstile_t *ts;

	/*
	 * First off, reap any detached LWP waiting to be collected.
	 * We can re-use its LWP structure and turnstile.
	 */
	isfree = NULL;
	if (p2->p_zomblwp != NULL) {
		mutex_enter(&p2->p_smutex);
		if ((isfree = p2->p_zomblwp) != NULL) {
			p2->p_zomblwp = NULL;
			lwp_free(isfree, true, false);/* releases proc mutex */
		} else
			mutex_exit(&p2->p_smutex);
	}
	if (isfree == NULL) {
		l2 = pool_get(&lwp_pool, PR_WAITOK);
		memset(l2, 0, sizeof(*l2));
		l2->l_ts = pool_cache_get(turnstile_cache, PR_WAITOK);
		SLIST_INIT(&l2->l_pi_lenders);
	} else {
		l2 = isfree;
		ts = l2->l_ts;
		KASSERT(l2->l_inheritedprio == -1);
		KASSERT(SLIST_EMPTY(&l2->l_pi_lenders));
		memset(l2, 0, sizeof(*l2));
		l2->l_ts = ts;
	}

	l2->l_stat = LSIDL;
	l2->l_proc = p2;
	l2->l_refcnt = 1;
	l2->l_class = sclass;
	l2->l_kpriority = l1->l_kpriority;
	l2->l_kpribase = PRI_KERNEL;
	l2->l_priority = l1->l_priority;
	l2->l_inheritedprio = -1;
	l2->l_mutex = l1->l_cpu->ci_schedstate.spc_mutex;
	l2->l_cpu = l1->l_cpu;
	l2->l_flag = inmem ? LW_INMEM : 0;

	if (p2->p_flag & PK_SYSTEM) {
		/*
		 * Mark it as a system process and not a candidate for
		 * swapping.
		 */
		l2->l_flag |= LW_SYSTEM;
	} else {
		/* Look for a CPU to start */
		l2->l_cpu = sched_takecpu(l2);
		l2->l_mutex = l2->l_cpu->ci_schedstate.spc_mutex;
	}

	lwp_initspecific(l2);
	sched_lwp_fork(l1, l2);
	lwp_update_creds(l2);
	callout_init(&l2->l_timeout_ch, CALLOUT_MPSAFE);
	callout_setfunc(&l2->l_timeout_ch, sleepq_timeout, l2);
	mutex_init(&l2->l_swaplock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&l2->l_sigcv, "sigwait");
	l2->l_syncobj = &sched_syncobj;

	if (rnewlwpp != NULL)
		*rnewlwpp = l2;

	l2->l_addr = UAREA_TO_USER(uaddr);
	uvm_lwp_fork(l1, l2, stack, stacksize, func,
	    (arg != NULL) ? arg : l2);

	mutex_enter(&p2->p_smutex);

	if ((flags & LWP_DETACHED) != 0) {
		l2->l_prflag = LPR_DETACHED;
		p2->p_ndlwps++;
	} else
		l2->l_prflag = 0;

	l2->l_sigmask = l1->l_sigmask;
	CIRCLEQ_INIT(&l2->l_sigpend.sp_info);
	sigemptyset(&l2->l_sigpend.sp_set);

	p2->p_nlwpid++;
	if (p2->p_nlwpid == 0)
		p2->p_nlwpid++;
	l2->l_lid = p2->p_nlwpid;
	LIST_INSERT_HEAD(&p2->p_lwps, l2, l_sibling);
	p2->p_nlwps++;

	mutex_exit(&p2->p_smutex);

	mutex_enter(&proclist_lock);
	LIST_INSERT_HEAD(&alllwp, l2, l_list);
	mutex_exit(&proclist_lock);

	SYSCALL_TIME_LWP_INIT(l2);

	if (p2->p_emul->e_lwp_fork)
		(*p2->p_emul->e_lwp_fork)(l1, l2);

	return (0);
}

/*
 * Called by MD code when a new LWP begins execution.  Must be called
 * with the previous LWP locked (so at splsched), or if there is no
 * previous LWP, at splsched.
 */
void
lwp_startup(struct lwp *prev, struct lwp *new)
{

	if (prev != NULL) {
		/*
		 * Normalize the count of the spin-mutexes, it was
		 * increased in mi_switch().  Unmark the state of
		 * context switch - it is finished for previous LWP.
		 */
		curcpu()->ci_mtx_count++;
		membar_exit();
		prev->l_ctxswtch = 0;
	}
	spl0();
	pmap_activate(new);
	LOCKDEBUG_BARRIER(NULL, 0);
	if ((new->l_pflag & LP_MPSAFE) == 0) {
		KERNEL_LOCK(1, new);
	}
}

/*
 * Exit an LWP.
 */
void
lwp_exit(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct lwp *l2;
	bool current;

	current = (l == curlwp);

	KASSERT(current || l->l_stat == LSIDL);

	/*
	 * Verify that we hold no locks other than the kernel lock.
	 */
#ifdef MULTIPROCESSOR
	LOCKDEBUG_BARRIER(&kernel_lock, 0);
#else
	LOCKDEBUG_BARRIER(NULL, 0);
#endif

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
	mutex_enter(&p->p_smutex);
	if (p->p_nlwps - p->p_nzlwps == 1) {
		KASSERT(current == true);
		exit1(l, 0);
		/* NOTREACHED */
	}
	p->p_nzlwps++;
	mutex_exit(&p->p_smutex);

	if (p->p_emul->e_lwp_exit)
		(*p->p_emul->e_lwp_exit)(l);

	/* Delete the specificdata while it's still safe to sleep. */
	specificdata_fini(lwp_specificdata_domain, &l->l_specdataref);

	/*
	 * Release our cached credentials.
	 */
	kauth_cred_free(l->l_cred);
	callout_destroy(&l->l_timeout_ch);

	/*
	 * While we can still block, mark the LWP as unswappable to
	 * prevent conflicts with the with the swapper.
	 */
	if (current)
		uvm_lwp_hold(l);

	/*
	 * Remove the LWP from the global list.
	 */
	mutex_enter(&proclist_lock);
	mutex_enter(&proclist_mutex);
	LIST_REMOVE(l, l_list);
	mutex_exit(&proclist_mutex);
	mutex_exit(&proclist_lock);

	/*
	 * Get rid of all references to the LWP that others (e.g. procfs)
	 * may have, and mark the LWP as a zombie.  If the LWP is detached,
	 * mark it waiting for collection in the proc structure.  Note that
	 * before we can do that, we need to free any other dead, deatched
	 * LWP waiting to meet its maker.
	 *
	 * XXXSMP disable preemption.
	 */
	mutex_enter(&p->p_smutex);
	lwp_drainrefs(l);

	if ((l->l_prflag & LPR_DETACHED) != 0) {
		while ((l2 = p->p_zomblwp) != NULL) {
			p->p_zomblwp = NULL;
			lwp_free(l2, false, false);/* releases proc mutex */
			mutex_enter(&p->p_smutex);
			l->l_refcnt++;
			lwp_drainrefs(l);
		}
		p->p_zomblwp = l;
	}

	/*
	 * If we find a pending signal for the process and we have been
	 * asked to check for signals, then we loose: arrange to have
	 * all other LWPs in the process check for signals.
	 */
	if ((l->l_flag & LW_PENDSIG) != 0 &&
	    firstsig(&p->p_sigpend.sp_set) != 0) {
		LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
			lwp_lock(l2);
			l2->l_flag |= LW_PENDSIG;
			lwp_unlock(l2);
		}
	}

	lwp_lock(l);
	l->l_stat = LSZOMB;
	lwp_unlock(l);
	p->p_nrlwps--;
	cv_broadcast(&p->p_lwpcv);
	if (l->l_lwpctl != NULL)
		l->l_lwpctl->lc_curcpu = LWPCTL_CPU_EXITED;
	mutex_exit(&p->p_smutex);

	/*
	 * We can no longer block.  At this point, lwp_free() may already
	 * be gunning for us.  On a multi-CPU system, we may be off p_lwps.
	 *
	 * Free MD LWP resources.
	 */
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free(l, 0);
#endif

	if (current) {
		pmap_deactivate(l);

		/*
		 * Release the kernel lock, and switch away into
		 * oblivion.
		 */
#ifdef notyet
		/* XXXSMP hold in lwp_userret() */
		KERNEL_UNLOCK_LAST(l);
#else
		KERNEL_UNLOCK_ALL(l, NULL);
#endif
		lwp_exit_switchaway(l);
	}
}

void
lwp_exit_switchaway(struct lwp *l)
{
	struct cpu_info *ci;
	struct lwp *idlelwp;

	/* Unlocked, but is for statistics only. */
	uvmexp.swtch++;

	(void)splsched();
	l->l_flag &= ~LW_RUNNING;
	ci = curcpu();	
	idlelwp = ci->ci_data.cpu_idlelwp;
	idlelwp->l_stat = LSONPROC;

	/*
	 * cpu_onproc must be updated with the CPU locked, as
	 * aston() may try to set a AST pending on the LWP (and
	 * it does so with the CPU locked).  Otherwise, the LWP
	 * may be destroyed before the AST can be set, leading
	 * to a user-after-free.
	 */
	spc_lock(ci);
	ci->ci_data.cpu_onproc = idlelwp;
	spc_unlock(ci);
	cpu_switchto(NULL, idlelwp, false);
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
	ksiginfoq_t kq;

	/*
	 * If this was not the last LWP in the process, then adjust
	 * counters and unlock.
	 */
	if (!last) {
		/*
		 * Add the LWP's run time to the process' base value.
		 * This needs to co-incide with coming off p_lwps.
		 */
		timeradd(&l->l_rtime, &p->p_rtime, &p->p_rtime);
		p->p_pctcpu += l->l_pctcpu;
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
		mutex_exit(&p->p_smutex);
	}

#ifdef MULTIPROCESSOR
	/*
	 * In the unlikely event that the LWP is still on the CPU,
	 * then spin until it has switched away.  We need to release
	 * all locks to avoid deadlock against interrupt handlers on
	 * the target CPU.
	 */
	if ((l->l_flag & LW_RUNNING) != 0 || l->l_cpu->ci_curlwp == l) {
		int count;
		(void)count; /* XXXgcc */
		KERNEL_UNLOCK_ALL(curlwp, &count);
		while ((l->l_flag & LW_RUNNING) != 0 ||
		    l->l_cpu->ci_curlwp == l)
			SPINLOCK_BACKOFF_HOOK;
		KERNEL_LOCK(count, curlwp);
	}
#endif

	/*
	 * Destroy the LWP's remaining signal information.
	 */
	ksiginfo_queue_init(&kq);
	sigclear(&l->l_sigpend, NULL, &kq);
	ksiginfo_queue_drain(&kq);
	cv_destroy(&l->l_sigcv);
	mutex_destroy(&l->l_swaplock);

	/*
	 * Free the LWP's turnstile and the LWP structure itself unless the
	 * caller wants to recycle them.  Also, free the scheduler specific data.
	 *
	 * We can't return turnstile0 to the pool (it didn't come from it),
	 * so if it comes up just drop it quietly and move on.
	 *
	 * We don't recycle the VM resources at this time.
	 */
	KERNEL_LOCK(1, curlwp);		/* XXXSMP */

	if (l->l_lwpctl != NULL)
		lwp_ctl_free(l);
	sched_lwp_exit(l);

	if (!recycle && l->l_ts != &turnstile0)
		pool_cache_put(turnstile_cache, l->l_ts);
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free2(l);
#endif
	uvm_lwp_exit(l);
	KASSERT(SLIST_EMPTY(&l->l_pi_lenders));
	KASSERT(l->l_inheritedprio == -1);
	if (!recycle)
		pool_put(&lwp_pool, l);
	KERNEL_UNLOCK_ONE(curlwp);	/* XXXSMP */
}

/*
 * Pick a LWP to represent the process for those operations which
 * want information about a "process" that is actually associated
 * with a LWP.
 *
 * If 'locking' is false, no locking or lock checks are performed.
 * This is intended for use by DDB.
 *
 * We don't bother locking the LWP here, since code that uses this
 * interface is broken by design and an exact match is not required.
 */
struct lwp *
proc_representative_lwp(struct proc *p, int *nrlwps, int locking)
{
	struct lwp *l, *onproc, *running, *sleeping, *stopped, *suspended;
	struct lwp *signalled;
	int cnt;

	if (locking) {
		KASSERT(mutex_owned(&p->p_smutex));
	}

	/* Trivial case: only one LWP */
	if (p->p_nlwps == 1) {
		l = LIST_FIRST(&p->p_lwps);
		if (nrlwps)
			*nrlwps = (l->l_stat == LSONPROC || l->l_stat == LSRUN);
		return l;
	}

	cnt = 0;
	switch (p->p_stat) {
	case SSTOP:
	case SACTIVE:
		/* Pick the most live LWP */
		onproc = running = sleeping = stopped = suspended = NULL;
		signalled = NULL;
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if ((l->l_flag & LW_IDLE) != 0) {
				continue;
			}
			if (l->l_lid == p->p_sigctx.ps_lwp)
				signalled = l;
			switch (l->l_stat) {
			case LSONPROC:
				onproc = l;
				cnt++;
				break;
			case LSRUN:
				running = l;
				cnt++;
				break;
			case LSSLEEP:
				sleeping = l;
				break;
			case LSSTOP:
				stopped = l;
				break;
			case LSSUSPENDED:
				suspended = l;
				break;
			}
		}
		if (nrlwps)
			*nrlwps = cnt;
		if (signalled)
			l = signalled;
		else if (onproc)
			l = onproc;
		else if (running)
			l = running;
		else if (sleeping)
			l = sleeping;
		else if (stopped)
			l = stopped;
		else if (suspended)
			l = suspended;
		else
			break;
		return l;
#ifdef DIAGNOSTIC
	case SIDL:
	case SZOMB:
	case SDYING:
	case SDEAD:
		if (locking)
			mutex_exit(&p->p_smutex);
		/* We have more than one LWP and we're in SIDL?
		 * How'd that happen?
		 */
		panic("Too many LWPs in idle/dying process %d (%s) stat = %d",
		    p->p_pid, p->p_comm, p->p_stat);
		break;
	default:
		if (locking)
			mutex_exit(&p->p_smutex);
		panic("Process %d (%s) in unknown state %d",
		    p->p_pid, p->p_comm, p->p_stat);
#endif
	}

	if (locking)
		mutex_exit(&p->p_smutex);
	panic("proc_representative_lwp: couldn't find a lwp for process"
		" %d (%s)", p->p_pid, p->p_comm);
	/* NOTREACHED */
	return NULL;
}

/*
 * Look up a live LWP within the speicifed process, and return it locked.
 *
 * Must be called with p->p_smutex held.
 */
struct lwp *
lwp_find(struct proc *p, int id)
{
	struct lwp *l;

	KASSERT(mutex_owned(&p->p_smutex));

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		if (l->l_lid == id)
			break;
	}

	/*
	 * No need to lock - all of these conditions will
	 * be visible with the process level mutex held.
	 */
	if (l != NULL && (l->l_stat == LSIDL || l->l_stat == LSZOMB))
		l = NULL;

	return l;
}

/*
 * Update an LWP's cached credentials to mirror the process' master copy.
 *
 * This happens early in the syscall path, on user trap, and on LWP
 * creation.  A long-running LWP can also voluntarily choose to update
 * it's credentials by calling this routine.  This may be called from
 * LWP_CACHE_CREDS(), which checks l->l_cred != p->p_cred beforehand.
 */
void
lwp_update_creds(struct lwp *l)
{
	kauth_cred_t oc;
	struct proc *p;

	p = l->l_proc;
	oc = l->l_cred;

	mutex_enter(&p->p_mutex);
	kauth_cred_hold(p->p_cred);
	l->l_cred = p->p_cred;
	mutex_exit(&p->p_mutex);
	if (oc != NULL) {
		KERNEL_LOCK(1, l);	/* XXXSMP */
		kauth_cred_free(oc);
		KERNEL_UNLOCK_ONE(l);	/* XXXSMP */
	}
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
 * Lock an LWP.
 */
void
lwp_lock_retry(struct lwp *l, kmutex_t *old)
{

	/*
	 * XXXgcc ignoring kmutex_t * volatile on i386
	 *
	 * gcc version 4.1.2 20061021 prerelease (NetBSD nb1 20061021)
	 */
#if 1
	while (l->l_mutex != old) {
#else
	for (;;) {
#endif
		mutex_spin_exit(old);
		old = l->l_mutex;
		mutex_spin_enter(old);

		/*
		 * mutex_enter() will have posted a read barrier.  Re-test
		 * l->l_mutex.  If it has changed, we need to try again.
		 */
#if 1
	}
#else
	} while (__predict_false(l->l_mutex != old));
#endif
}

/*
 * Lend a new mutex to an LWP.  The old mutex must be held.
 */
void
lwp_setlock(struct lwp *l, kmutex_t *new)
{

	KASSERT(mutex_owned(l->l_mutex));

	membar_producer();
	l->l_mutex = new;
}

/*
 * Lend a new mutex to an LWP, and release the old mutex.  The old mutex
 * must be held.
 */
void
lwp_unlock_to(struct lwp *l, kmutex_t *new)
{
	kmutex_t *old;

	KASSERT(mutex_owned(l->l_mutex));

	old = l->l_mutex;
	membar_producer();
	l->l_mutex = new;
	mutex_spin_exit(old);
}

/*
 * Acquire a new mutex, and donate it to an LWP.  The LWP must already be
 * locked.
 */
void
lwp_relock(struct lwp *l, kmutex_t *new)
{
	kmutex_t *old;

	KASSERT(mutex_owned(l->l_mutex));

	old = l->l_mutex;
	if (old != new) {
		mutex_spin_enter(new);
		l->l_mutex = new;
		mutex_spin_exit(old);
	}
}

int
lwp_trylock(struct lwp *l)
{
	kmutex_t *old;

	for (;;) {
		if (!mutex_tryenter(old = l->l_mutex))
			return 0;
		if (__predict_true(l->l_mutex == old))
			return 1;
		mutex_spin_exit(old);
	}
}

/*
 * Handle exceptions for mi_userret().  Called if a member of LW_USERRET is
 * set.
 */
void
lwp_userret(struct lwp *l)
{
	struct proc *p;
	void (*hook)(void);
	int sig;

	p = l->l_proc;

#ifndef __HAVE_FAST_SOFTINTS
	/* Run pending soft interrupts. */
	if (l->l_cpu->ci_data.cpu_softints != 0)
		softint_overlay();
#endif

	/*
	 * It should be safe to do this read unlocked on a multiprocessor
	 * system..
	 */
	while ((l->l_flag & LW_USERRET) != 0) {
		/*
		 * Process pending signals first, unless the process
		 * is dumping core or exiting, where we will instead
		 * enter the L_WSUSPEND case below.
		 */
		if ((l->l_flag & (LW_PENDSIG | LW_WCORE | LW_WEXIT)) ==
		    LW_PENDSIG) {
			KERNEL_LOCK(1, l);	/* XXXSMP pool_put() below */
			mutex_enter(&p->p_smutex);
			while ((sig = issignal(l)) != 0)
				postsig(sig);
			mutex_exit(&p->p_smutex);
			KERNEL_UNLOCK_LAST(l);	/* XXXSMP */
		}

		/*
		 * Core-dump or suspend pending.
		 *
		 * In case of core dump, suspend ourselves, so that the
		 * kernel stack and therefore the userland registers saved
		 * in the trapframe are around for coredump() to write them
		 * out.  We issue a wakeup on p->p_lwpcv so that sigexit()
		 * will write the core file out once all other LWPs are
		 * suspended.
		 */
		if ((l->l_flag & LW_WSUSPEND) != 0) {
			mutex_enter(&p->p_smutex);
			p->p_nrlwps--;
			cv_broadcast(&p->p_lwpcv);
			lwp_lock(l);
			l->l_stat = LSSUSPENDED;
			mutex_exit(&p->p_smutex);
			mi_switch(l);
		}

		/* Process is exiting. */
		if ((l->l_flag & LW_WEXIT) != 0) {
			KERNEL_LOCK(1, l);
			lwp_exit(l);
			KASSERT(0);
			/* NOTREACHED */
		}

		/* Call userret hook; used by Linux emulation. */
		if ((l->l_flag & LW_WUSERRET) != 0) {
			lwp_lock(l);
			l->l_flag &= ~LW_WUSERRET;
			lwp_unlock(l);
			hook = p->p_userret;
			p->p_userret = NULL;
			(*hook)();
		}
	}
}

/*
 * Force an LWP to enter the kernel, to take a trip through lwp_userret().
 */
void
lwp_need_userret(struct lwp *l)
{
	KASSERT(lwp_locked(l, NULL));

	/*
	 * Since the tests in lwp_userret() are done unlocked, make sure
	 * that the condition will be seen before forcing the LWP to enter
	 * kernel mode.
	 */
	membar_producer();
	cpu_signotify(l);
}

/*
 * Add one reference to an LWP.  This will prevent the LWP from
 * exiting, thus keep the lwp structure and PCB around to inspect.
 */
void
lwp_addref(struct lwp *l)
{

	KASSERT(mutex_owned(&l->l_proc->p_smutex));
	KASSERT(l->l_stat != LSZOMB);
	KASSERT(l->l_refcnt != 0);

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

	mutex_enter(&p->p_smutex);
	KASSERT(l->l_stat != LSZOMB);
	KASSERT(l->l_refcnt > 0);
	if (--l->l_refcnt == 0)
		cv_broadcast(&p->p_lwpcv);
	mutex_exit(&p->p_smutex);
}

/*
 * Drain all references to the current LWP.
 */
void
lwp_drainrefs(struct lwp *l)
{
	struct proc *p = l->l_proc;

	KASSERT(mutex_owned(&p->p_smutex));
	KASSERT(l->l_refcnt != 0);

	l->l_refcnt--;
	while (l->l_refcnt != 0)
		cv_wait(&p->p_lwpcv, &p->p_smutex);
}

/*
 * lwp_specific_key_create --
 *	Create a key for subsystem lwp-specific data.
 */
int
lwp_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return (specificdata_key_create(lwp_specificdata_domain, keyp, dtor));
}

/*
 * lwp_specific_key_delete --
 *	Delete a key for subsystem lwp-specific data.
 */
void
lwp_specific_key_delete(specificdata_key_t key)
{

	specificdata_key_delete(lwp_specificdata_domain, key);
}

/*
 * lwp_initspecific --
 *	Initialize an LWP's specificdata container.
 */
void
lwp_initspecific(struct lwp *l)
{
	int error;

	error = specificdata_init(lwp_specificdata_domain, &l->l_specdataref);
	KASSERT(error == 0);
}

/*
 * lwp_finispecific --
 *	Finalize an LWP's specificdata container.
 */
void
lwp_finispecific(struct lwp *l)
{

	specificdata_fini(lwp_specificdata_domain, &l->l_specdataref);
}

/*
 * lwp_getspecific --
 *	Return lwp-specific data corresponding to the specified key.
 *
 *	Note: LWP specific data is NOT INTERLOCKED.  An LWP should access
 *	only its OWN SPECIFIC DATA.  If it is necessary to access another
 *	LWP's specifc data, care must be taken to ensure that doing so
 *	would not cause internal data structure inconsistency (i.e. caller
 *	can guarantee that the target LWP is not inside an lwp_getspecific()
 *	or lwp_setspecific() call).
 */
void *
lwp_getspecific(specificdata_key_t key)
{

	return (specificdata_getspecific_unlocked(lwp_specificdata_domain,
						  &curlwp->l_specdataref, key));
}

void *
_lwp_getspecific_by_lwp(struct lwp *l, specificdata_key_t key)
{

	return (specificdata_getspecific_unlocked(lwp_specificdata_domain,
						  &l->l_specdataref, key));
}

/*
 * lwp_setspecific --
 *	Set lwp-specific data corresponding to the specified key.
 */
void
lwp_setspecific(specificdata_key_t key, void *data)
{

	specificdata_setspecific(lwp_specificdata_domain,
				 &curlwp->l_specdataref, key, data);
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

	if (l->l_lcpage != NULL) {
		lcp = l->l_lcpage;
		*uaddr = lcp->lcp_uaddr + (vaddr_t)l->l_lwpctl - lcp->lcp_kaddr;
		return (EINVAL);
	}

	/* First time around, allocate header structure for the process. */
	if ((lp = p->p_lwpctl) == NULL) {
		lp = kmem_alloc(sizeof(*lp), KM_SLEEP);
		mutex_init(&lp->lp_lock, MUTEX_DEFAULT, IPL_NONE);
		lp->lp_uao = NULL;
		TAILQ_INIT(&lp->lp_pages);
		mutex_enter(&p->p_mutex);
		if (p->p_lwpctl == NULL) {
			p->p_lwpctl = lp;
			mutex_exit(&p->p_mutex);
		} else {
			mutex_exit(&p->p_mutex);
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
		     (vaddr_t)p->p_vmspace->vm_daddr, LWPCTL_UAREA_SZ);
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
		if (lcp == NULL) {
			mutex_exit(&lp->lp_lock);
			return ENOMEM;
		}
		/*
		 * Wire the next page down in kernel space.  Since this
		 * is a new mapping, we must add a reference.
		 */
		uao = lp->lp_uao;
		(*uao->pgops->pgo_reference)(uao);
		error = uvm_map(kernel_map, &lcp->lcp_kaddr, PAGE_SIZE,
		    uao, lp->lp_cur, PAGE_SIZE,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
		    UVM_INH_NONE, UVM_ADV_RANDOM, 0));
		if (error == 0)
			error = uvm_map_pageable(kernel_map, lcp->lcp_kaddr,
			    lcp->lcp_kaddr + PAGE_SIZE, FALSE, 0);
		if (error != 0) {
			mutex_exit(&lp->lp_lock);
			kmem_free(lcp, LWPCTL_LCPAGE_SZ);
			(*uao->pgops->pgo_detach)(uao);
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
	lcp->lcp_bitmap[i] ^= (1 << bit);
	lcp->lcp_rotor = i;
	lcp->lcp_nfree--;
	l->l_lcpage = lcp;
	offset = (i << 5) + bit;
	l->l_lwpctl = (lwpctl_t *)lcp->lcp_kaddr + offset;
	*uaddr = lcp->lcp_uaddr + offset * sizeof(lwpctl_t);
	mutex_exit(&lp->lp_lock);

	l->l_lwpctl->lc_curcpu = (short)curcpu()->ci_data.cpu_index;

	return 0;
}

/*
 * Free an lwpctl structure back to the per-process list.
 */
void
lwp_ctl_free(lwp_t *l)
{
	lcproc_t *lp;
	lcpage_t *lcp;
	u_int map, offset;

	lp = l->l_proc->p_lwpctl;
	KASSERT(lp != NULL);

	lcp = l->l_lcpage;
	offset = (u_int)((lwpctl_t *)l->l_lwpctl - (lwpctl_t *)lcp->lcp_kaddr);
	KASSERT(offset < LWPCTL_PER_PAGE);

	mutex_enter(&lp->lp_lock);
	lcp->lcp_nfree++;
	map = offset >> 5;
	lcp->lcp_bitmap[map] |= (1 << (offset & 31));
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

#if defined(DDB)
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
