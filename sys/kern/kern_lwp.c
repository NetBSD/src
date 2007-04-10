/*	$NetBSD: kern_lwp.c,v 1.61.2.8 2007/04/10 13:26:39 ad Exp $	*/

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
 *	Lightweight processes (LWPs) are the basic unit (or thread) of
 *	execution within the kernel.  The core state of an LWP is described
 *	by "struct lwp".
 *
 *	Each LWP is contained within a process (described by "struct proc"),
 *	Every process contains at least one LWP, but may contain more.  The
 *	process describes attributes shared among all of its LWPs such as a
 *	private address space, global execution state (stopped, active,
 *	zombie, ...), signal disposition and so on.  On a multiprocessor
 *	machine, multiple LWPs be executing in kernel simultaneously.
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
 * 		Idle: the LWP has been created but has not yet executed. 
 * 		Whoever created the new LWP can be expected to set it to
 * 		another state shortly.
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
 *	sleeping, idle, or on a run queue. It is expected to take the
 *	necessary action to stop executing or become "running" again within
 *	a short timeframe.
 * 
 * 	LSZOMB:
 * 
 * 		Dead: the LWP has released most of its resources and is
 * 		about to switch away into oblivion.  When it switches away,
 * 		its few remaining resources will be collected.
 * 
 * 	LSSLEEP:
 * 
 * 		Sleeping: the LWP has entered itself onto a sleep queue, and
 * 		will switch away shortly to allow other LWPs to run on the
 * 		CPU.
 * 
 * 	LSSTOP:
 * 
 * 		Stopped: the LWP has been stopped as a result of a job
 * 		control signal, or as a result of the ptrace() interface. 
 * 		Stopped LWPs may run briefly within the kernel to handle
 * 		signals that they receive, but will not return to user space
 * 		until their process' state is changed away from stopped. 
 * 		Single LWPs within a process can not be set stopped
 * 		selectively: all actions that can stop or continue LWPs
 * 		occur at the process level.
 * 
 * State transitions
 *
 *	Note that the LSSTOP and LSSUSPENDED states may only be set
 *	when returning to user space in userret(), or when sleeping
 *	interruptably.  Before setting those states, we try to ensure
 *	that the LWPs will release all kernel locks that they hold,
 *	and at a minimum try to ensure that the LWP can be set runnable
 *	again by a signal.
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
 * Locking
 *
 *	The majority of fields in 'struct lwp' are covered by a single,
 *	general spin mutex pointed to by lwp::l_mutex.  The locks covering
 *	each field are documented in sys/lwp.h.
 *
 *	State transitions must be made with the LWP's general lock held.  In
 *	a multiprocessor kernel, state transitions may cause the LWP's lock
 *	pointer to change.  On uniprocessor kernels, most scheduler and
 *	synchronisation objects such as sleep queues and LWPs are protected
 *	by only one mutex (sched_mutex).  In this case, LWPs' lock pointers
 *	will never change and will always reference sched_mutex.
 *
 *	Manipulation of the general lock is not performed directly, but
 *	through calls to lwp_lock(), lwp_relock() and similar.
 *
 *	States and their associated locks:
 *
 *	LSIDL, LSZOMB
 *
 *		Always covered by sched_mutex.
 *
 *	LSONPROC, LSRUN:
 *
 *		Always covered by sched_mutex, which protects the run queues
 *		and other miscellaneous items.  If the scheduler is changed
 *		to use per-CPU run queues, this may become a per-CPU mutex.
 *
 *	LSSLEEP:
 *
 *		Covered by a mutex associated with the sleep queue that the
 *		LWP resides on, indirectly referenced by l_sleepq->sq_mutex.
 *
 *	LSSTOP, LSSUSPENDED:
 *	
 *		If the LWP was previously sleeping (l_wchan != NULL), then
 *		l_mutex references the sleep queue mutex.  If the LWP was
 *		runnable or on the CPU when halted, or has been removed from
 *		the sleep queue since halted, then the mutex is sched_mutex.
 *
 *	The lock order is as follows:
 *
 *		sleepq_t::sq_mutex  |---> sched_mutex
 *		tschain_t::tc_mutex |
 *
 *	Each process has an scheduler state mutex (proc::p_smutex), and a
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
__KERNEL_RCSID(0, "$NetBSD: kern_lwp.c,v 1.61.2.8 2007/04/10 13:26:39 ad Exp $");

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#define _LWP_API_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/syscall_stats.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

struct lwplist	alllwp;

POOL_INIT(lwp_pool, sizeof(struct lwp), MIN_LWP_ALIGNMENT, 0, 0, "lwppl",
    &pool_allocator_nointr, IPL_NONE);
POOL_INIT(lwp_uc_pool, sizeof(ucontext_t), 0, 0, 0, "lwpucpl",
    &pool_allocator_nointr, IPL_NONE);

static specificdata_domain_t lwp_specificdata_domain;

#define LWP_DEBUG

#ifdef LWP_DEBUG
int lwp_debug = 0;
#define DPRINTF(x) if (lwp_debug) printf x
#else
#define DPRINTF(x)
#endif

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

	/*
	 * XXXLWP Wait for:
	 *
	 * o process exiting
	 * o target LWP suspended
	 * o target LWP not suspended and L_WSUSPEND clear
	 * o target LWP exited
	 */

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

	DPRINTF(("lwp_continue of %d.%d (%s), state %d, wchan %p\n",
	    l->l_proc->p_pid, l->l_lid, l->l_proc->p_comm, l->l_stat,
	    l->l_wchan));

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

	DPRINTF(("lwp_wait1: %d.%d waiting for %d.\n",
	    p->p_pid, l->l_lid, lid));

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
newlwp(struct lwp *l1, struct proc *p2, vaddr_t uaddr, bool inmem,
    int flags, void *stack, size_t stacksize,
    void (*func)(void *), void *arg, struct lwp **rnewlwpp)
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
		l2->l_ts = pool_cache_get(&turnstile_cache, PR_WAITOK);
		SLIST_INIT(&l2->l_pi_lenders);
	} else {
		l2 = isfree;
		ts = l2->l_ts;
		KASSERT(l2->l_inheritedprio == MAXPRI);
		KASSERT(SLIST_EMPTY(&l2->l_pi_lenders));
		memset(l2, 0, sizeof(*l2));
		l2->l_ts = ts;
	}

	l2->l_stat = LSIDL;
	l2->l_proc = p2;
	l2->l_refcnt = 1;
	l2->l_priority = l1->l_priority;
	l2->l_usrpri = l1->l_usrpri;
	l2->l_inheritedprio = MAXPRI;
	l2->l_mutex = &sched_mutex;
	l2->l_cpu = l1->l_cpu;
	l2->l_flag = inmem ? LW_INMEM : 0;
	lwp_initspecific(l2);
	TAILQ_INIT(&l2->l_selwait);

	if (p2->p_flag & PK_SYSTEM) {
		/*
		 * Mark it as a system process and not a candidate for
		 * swapping.
		 */
		l2->l_flag |= LW_SYSTEM;
	}

	lwp_update_creds(l2);
	callout_init(&l2->l_tsleep_ch);
	mutex_init(&l2->l_swaplock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&l2->l_sigcv, "sigwait");
	l2->l_syncobj = &sched_syncobj;

	if (rnewlwpp != NULL)
		*rnewlwpp = l2;

	l2->l_addr = UAREA_TO_USER(uaddr);
	KERNEL_LOCK(1, curlwp);
	uvm_lwp_fork(l1, l2, stack, stacksize, func,
	    (arg != NULL) ? arg : l2);
	KERNEL_UNLOCK_ONE(curlwp);

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
	mutex_enter(&proclist_mutex);
	LIST_INSERT_HEAD(&alllwp, l2, l_list);
	mutex_exit(&proclist_mutex);
	mutex_exit(&proclist_lock);

	SYSCALL_TIME_LWP_INIT(l2);

	if (p2->p_emul->e_lwp_fork)
		(*p2->p_emul->e_lwp_fork)(l1, l2);

	return (0);
}

/*
 * Quit the process.  This will call cpu_exit, which will call cpu_switch,
 * so this can only be used meaningfully if you're willing to switch away. 
 * Calling with l!=curlwp would be weird.
 */
void
lwp_exit(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct lwp *l2;

	DPRINTF(("lwp_exit: %d.%d exiting.\n", p->p_pid, l->l_lid));
	DPRINTF((" nlwps: %d nzlwps: %d\n", p->p_nlwps, p->p_nzlwps));

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
		DPRINTF(("lwp_exit: %d.%d calling exit1()\n",
		    p->p_pid, l->l_lid));
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

	/*
	 * While we can still block, mark the LWP as unswappable to
	 * prevent conflicts with the with the swapper.
	 */
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
	pmap_deactivate(l);

	/*
	 * Release the kernel lock, signal another LWP to collect us,
	 * and switch away into oblivion.
	 */
#ifdef notyet
	/* XXXSMP hold in lwp_userret() */
	KERNEL_UNLOCK_LAST(l);
#else
	KERNEL_UNLOCK_ALL(l, NULL);
#endif

	cpu_exit(l);
}

/*
 * We are called from cpu_exit() once it is safe to schedule the dead LWP's
 * resources to be freed (i.e., once we've switched to the idle PCB for the
 * current CPU).
 */
void
lwp_exit2(struct lwp *l)
{
	/* XXXSMP re-enable preemption */
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
	if (l->l_cpu->ci_curlwp == l) {
		int count;
		KERNEL_UNLOCK_ALL(curlwp, &count);
		while (l->l_cpu->ci_curlwp == l)
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
	 * caller wants to recycle them. 
	 *
	 * We can't return turnstile0 to the pool (it didn't come from it),
	 * so if it comes up just drop it quietly and move on.
	 *
	 * We don't recycle the VM resources at this time.
	 */
	if (!recycle && l->l_ts != &turnstile0)
		pool_cache_put(&turnstile_cache, l->l_ts);
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free2(l);
#endif
	uvm_lwp_exit(l);
	KASSERT(SLIST_EMPTY(&l->l_pi_lenders));
	KASSERT(l->l_inheritedprio == MAXPRI);
	if (!recycle)
		pool_put(&lwp_pool, l);
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
			*nrlwps = (l->l_stat == LSONPROC || LSRUN);
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
		if (nrlwps)
			*nrlwps = 0;
		l = LIST_FIRST(&p->p_lwps);
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

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	return mutex_owned(cur) && (mtx == cur || mtx == NULL);
#else
	return mutex_owned(cur);
#endif
}

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
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
#endif

/*
 * Lend a new mutex to an LWP.  The old mutex must be held.
 */
void
lwp_setlock(struct lwp *l, kmutex_t *new)
{

	KASSERT(mutex_owned(l->l_mutex));

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	mb_write();
	l->l_mutex = new;
#else
	(void)new;
#endif
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
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	mb_write();
	l->l_mutex = new;
#else
	(void)new;
#endif
	mutex_spin_exit(old);
}

/*
 * Acquire a new mutex, and donate it to an LWP.  The LWP must already be
 * locked.
 */
void
lwp_relock(struct lwp *l, kmutex_t *new)
{
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	kmutex_t *old;
#endif

	KASSERT(mutex_owned(l->l_mutex));

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	old = l->l_mutex;
	if (old != new) {
		mutex_spin_enter(new);
		l->l_mutex = new;
		mutex_spin_exit(old);
	}
#else
	(void)new;
#endif
}

int
lwp_trylock(struct lwp *l)
{
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	kmutex_t *old;

	for (;;) {
		if (!mutex_tryenter(old = l->l_mutex))
			return 0;
		if (__predict_true(l->l_mutex == old))
			return 1;
		mutex_spin_exit(old);
	}
#else
	return mutex_tryenter(l->l_mutex);
#endif
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
			mutex_enter(&p->p_smutex);
			while ((sig = issignal(l)) != 0)
				postsig(sig);
			mutex_exit(&p->p_smutex);
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
			mi_switch(l, NULL);
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
	mb_write();
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
	if (--l->l_refcnt == 0)
		cv_broadcast(&p->p_refcv);
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
		cv_wait(&p->p_refcv, &p->p_smutex);
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
