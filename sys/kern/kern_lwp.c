/*	$NetBSD: kern_lwp.c,v 1.40.2.5 2006/11/18 21:39:22 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006 The NetBSD Foundation, Inc.
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
 *	Note that LWPs differ from kernel threads (kthreads) in that kernel
 *	threads are distinct processes (system processes) with no user space
 *	component, which themselves may contain one or more LWPs.
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
 * 	LSZOMB
 * 
 * 		Zombie: the LWP has exited, released all of its resources
 * 		and can execute no further.  It will persist until 'reaped'
 * 		by another LWP or process via the _lwp_wait() or wait()
 * 		system calls.
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
 * 	LSDEAD:
 * 
 * 		Dead: the LWP has released most of its resources and is
 * 		about to switch away into oblivion.  When it switches away,
 * 		its few remaining resources will be collected and the LWP
 * 		will enter the LSZOMB (zombie) state.
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
 *	 IDL -------> SUSPENDED		DEAD -------> ZOMBIE
 *		    > RUN
 *
 *	 RUN -------> ONPROC		ONPROC -----> RUN
 *	            > STOPPED			    > SLEEP
 *	            > SUSPENDED			    > STOPPED
 *						    > SUSPENDED
 *						    > DEAD
 *
 *	 STOPPED ---> RUN		SUSPENDED --> RUN
 *	            > SLEEP			    > SLEEP
 *
 *	 SLEEP -----> ONPROC
 *		    > RUN
 *		    > STOPPED
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
 *	LSIDL, LSDEAD, LSZOMB
 *
 *		Always covered by lwp_mutex (the idle mutex).
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
 *		the sleep queue since halted, then the mutex is lwp_mutex.
 *
 *	The lock order for the various mutexes is as follows:
 *
 *		sleepq_t::sq_mutex -> lwp_mutex -> sched_mutex
 *
 *	Each process has an scheduler state mutex (proc::p_smutex), and a
 *	number of counters on LWPs and their states: p_nzlwps, p_nrlwps, and
 *	so on.  When an LWP is to be entered into or removed from one of the
 *	following states, p_mutex must be held and the process wide counters
 *	adjusted:
 *
 *		LSIDL, LSDEAD, LSZOMB, LSSTOP, LSSUSPENDED
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
__KERNEL_RCSID(0, "$NetBSD: kern_lwp.c,v 1.40.2.5 2006/11/18 21:39:22 ad Exp $");

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#define _LWP_API_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>

#include <uvm/uvm_extern.h>

struct lwplist	alllwp;
kmutex_t	alllwp_mutex;
kmutex_t	lwp_mutex;

POOL_INIT(lwp_pool, sizeof(struct lwp), 16, 0, 0, "lwppl",
    &pool_allocator_nointr);
POOL_INIT(lwp_uc_pool, sizeof(ucontext_t), 0, 0, 0, "lwpucpl",
    &pool_allocator_nointr);

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
}

/*
 * Set an LWP halted or suspended.
 *
 * Must be called with p_smutex held, and the LWP locked.  Will unlock the
 * LWP before return.
 */
int
lwp_halt(struct lwp *curl, struct lwp *t, int state)
{
	int error, want;

	LOCK_ASSERT(mutex_owned(&t->l_proc->p_smutex)); /* XXXAD what now? */
	LOCK_ASSERT(lwp_locked(t, NULL));

	KASSERT(curl != t || curl->l_stat == LSONPROC);

	/*
	 * If the current LWP has been told to exit, we must not suspend anyone
	 * else or deadlock could occur.  We won't return to userspace.
	 */
	if ((curl->l_stat & (L_WEXIT | L_WCORE)) != 0)
		return (EDEADLK);

	error = 0;

	want = (state == LSSUSPENDED ? L_WSUSPEND : 0);

	switch (t->l_stat) {
	case LSRUN:
	case LSONPROC:
		t->l_flag |= want;
		signotify(t);
		break;

	case LSSLEEP:
		t->l_stat |= want;

		/*
		 * Kick the LWP and try to get it to the kernel boundary
		 * so that it will release any locks that it holds.
		 * setrunnable() will release the lock.
		 */
		signotify(t);
		setrunnable(t);
		return 0;

	case LSSUSPENDED:
	case LSSTOP:
		t->l_flag |= want;
		break;

	case LSIDL:
	case LSZOMB:
	case LSDEAD:
		error = EINTR; /* It's what Solaris does..... */
		break;
	}

	lwp_unlock(t);

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

	LOCK_ASSERT(mutex_owned(&l->l_proc->p_smutex));
	LOCK_ASSERT(lwp_locked(l, NULL));

	DPRINTF(("lwp_continue of %d.%d (%s), state %d, wchan %p\n",
	    l->l_proc->p_pid, l->l_lid, l->l_proc->p_comm, l->l_stat,
	    l->l_wchan));

	/* If rebooting or not suspended, then just bail out. */
	if ((l->l_flag & L_WREBOOT) != 0) {
		lwp_unlock(l);
		return;
	}

	l->l_flag &= ~L_WSUSPEND;

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
	int nfound, error, wpri;
	static const char waitstr1[] = "lwpwait";
	static const char waitstr2[] = "lwpwait2";

	DPRINTF(("lwp_wait1: %d.%d waiting for %d.\n",
	    p->p_pid, l->l_lid, lid));

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	/*
	 * Check for deadlock:
	 *
	 * 1) If all other LWPs are waiting for exits or suspended.
	 * 2) If we are trying to wait on ourself.
	 *
	 * XXX we'd like to check for a cycle of waiting LWPs (specific LID
	 * waits, not any-LWP waits) and detect that sort of deadlock, but
	 * we don't have a good place to store the lwp that is being waited
	 * for. wchan is already filled with &p->p_nlwps, and putting the
	 * lwp address in there for deadlock tracing would require exiting
	 * LWPs to call wakeup on both their own address and &p->p_nlwps, to
	 * get threads sleeping on any LWP exiting.
	 */
	if (lwp_lastlive(p->p_nlwpwait) || lid == l->l_lid)
		return (EDEADLK);

	p->p_nlwpwait++;
	wpri = PWAIT;
	if ((flags & LWPWAIT_EXITCONTROL) == 0)
		wpri |= PCATCH;
 loop:
	nfound = 0;
	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
		if ((l2 == l) || (l2->l_flag & L_DETACHED) ||
		    ((lid != 0) && (lid != l2->l_lid)))
			continue;
		nfound++;
		if (l2->l_stat != LSZOMB)
			continue;

		if (departed)
			*departed = l2->l_lid;

		LIST_REMOVE(l2, l_sibling);
		p->p_nlwps--;
		p->p_nzlwps--;
		p->p_nlwpwait--;
		/* XXX decrement limits */
		pool_put(&lwp_pool, l2);
		return (0);
	}

	if (nfound == 0) {
		p->p_nlwpwait--;
		return (ESRCH);
	}

	if ((error = mtsleep(&p->p_nlwps, wpri,
	    (lid != 0) ? waitstr1 : waitstr2, 0, &p->p_smutex)) != 0)
		return (error);

	goto loop;
}

/*
 * Create a new LWP within process 'p2', using LWP 'l1' as a template.
 * The new LWP is created in state LSIDL and must be set running,
 * suspended, or stopped by the caller.
 */
int
newlwp(struct lwp *l1, struct proc *p2, vaddr_t uaddr, boolean_t inmem,
    int flags, void *stack, size_t stacksize,
    void (*func)(void *), void *arg, struct lwp **rnewlwpp)
{
	struct lwp *l2;

	l2 = pool_get(&lwp_pool, PR_WAITOK);

	l2->l_stat = LSIDL;
	l2->l_forw = l2->l_back = NULL;
	l2->l_proc = p2;
	l2->l_refcnt = 1;

	lwp_initspecific(l2);

	memset(&l2->l_startzero, 0,
	       (unsigned) ((caddr_t)&l2->l_endzero -
			   (caddr_t)&l2->l_startzero));

	/* The copy here is unlocked, but is unlikely to pose a problem. */
	memcpy(&l2->l_startcopy, &l1->l_startcopy,
	       (unsigned) ((caddr_t)&l2->l_endcopy -
			   (caddr_t)&l2->l_startcopy));

#if !defined(MULTIPROCESSOR)
	/*
	 * In the single-processor case, all processes will always run
	 * on the same CPU.  So, initialize the child's CPU to the parent's
	 * now.  In the multiprocessor case, the child's CPU will be
	 * initialized in the low-level context switch code when the
	 * process runs.
	 */
	KASSERT(l1->l_cpu != NULL);
	l2->l_cpu = l1->l_cpu;
#else
	/*
	 * Zero child's CPU pointer so we don't get trash.
	 */
	l2->l_cpu = NULL;
#endif /* ! MULTIPROCESSOR */

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	l2->l_mutex = &lwp_mutex;
#else
	l2->l_mutex = &sched_mutex;
#endif

	l2->l_flag = inmem ? L_INMEM : 0;
	l2->l_flag |= (flags & LWP_DETACHED) ? L_DETACHED : 0;

	if (p2->p_flag & P_SYSTEM) {
		/*
		 * Mark it as a system process and not a candidate for
		 * swapping.
		 */
		l2->l_flag |= L_SYSTEM | L_INMEM;
	}

	lwp_update_creds(l2);
	callout_init(&l2->l_tsleep_ch);
	l2->l_ts = pool_cache_get(&turnstile_cache, PR_WAITOK);
	l2->l_syncobj = &sched_syncobj;

	if (rnewlwpp != NULL)
		*rnewlwpp = l2;

	l2->l_addr = UAREA_TO_USER(uaddr);
	uvm_lwp_fork(l1, l2, stack, stacksize, func,
	    (arg != NULL) ? arg : l2);

	mutex_enter(&p2->p_smutex);

	if ((p2->p_flag & P_SA) == 0) {
		l2->l_sigpend = &l2->l_sigstore.ss_pend;
		l2->l_sigmask = &l2->l_sigstore.ss_mask;
		l2->l_sigstk = &l2->l_sigstore.ss_stk;
		l2->l_sigmask = l1->l_sigmask;
		CIRCLEQ_INIT(&l2->l_sigpend->sp_info);
		sigemptyset(&l2->l_sigpend->sp_set);
	} else {
		l2->l_sigpend = &p2->p_sigstore.ss_pend;
		l2->l_sigmask = &p2->p_sigstore.ss_mask;
		l2->l_sigstk = &p2->p_sigstore.ss_stk;
	}

	l2->l_lid = ++p2->p_nlwpid;
	LIST_INSERT_HEAD(&p2->p_lwps, l2, l_sibling);
	p2->p_nlwps++;

	mutex_exit(&p2->p_smutex);

	mutex_enter(&alllwp_mutex);
	LIST_INSERT_HEAD(&alllwp, l2, l_list);
	mutex_exit(&alllwp_mutex);

	if (p2->p_emul->e_lwp_fork)
		(*p2->p_emul->e_lwp_fork)(l1, l2);

	return (0);
}

/*
 * Quit the process.  This will call cpu_exit, which will call cpu_switch,
 * so this can only be used meaningfully if you're willing to switch away. 
 * Calling with l!=curlwp would be weird.
 */
int
lwp_exit(struct lwp *l, int checksigs)
{
	struct proc *p = l->l_proc;

	DPRINTF(("lwp_exit: %d.%d exiting.\n", p->p_pid, l->l_lid));
	DPRINTF((" nlwps: %d nzlwps: %d\n", p->p_nlwps, p->p_nzlwps));

	mutex_enter(&p->p_smutex);

	/*
	 * If we've got pending signals that we haven't processed yet, make
	 * sure that we take them before exiting.
	 */
	if (checksigs && sigispending(l)) {
		mutex_exit(&p->p_smutex);
		return ERESTART;
	}

	if (p->p_emul->e_lwp_exit)
		(*p->p_emul->e_lwp_exit)(l);

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
	p->p_nzlwps++;
	if (p->p_nlwps - p->p_nzlwps == 0) {
		DPRINTF(("lwp_exit: %d.%d calling exit1()\n",
		    p->p_pid, l->l_lid));
		exit1(l, 0);
		/* NOTREACHED */
	}

	/* Delete the specificdata while it's still safe to sleep. */
	specificdata_fini(lwp_specificdata_domain, &l->l_specdataref);

	/*
	 * Release our cached credentials and collate accounting flags.
	 */
	kauth_cred_free(l->l_cred);
	mutex_enter(&p->p_mutex);
	p->p_acflag |= l->l_acflag;
	mutex_exit(&p->p_mutex);

	lwp_lock(l);
	if ((l->l_flag & L_DETACHED) != 0) {
		LIST_REMOVE(l, l_sibling);
		p->p_nlwps--;
		curlwp = NULL;
		l->l_proc = NULL;
	}
	l->l_stat = LSDEAD;
	lwp_unlock_to(l, &lwp_mutex);

	if ((p->p_flag & P_SA) == 0) {
		/*
		 * Clear any private, pending signals.   XXX We may loose
		 * process-wide signals that we didn't want to take.
		 */
		sigclear(l->l_sigpend, NULL);
	}

	mutex_exit(&p->p_smutex);

	/*
	 * Remove the LWP from the global list and from the parent process.
	 * Once done, mark it as dead.  Nothing should be able to find or
	 * update it past this point.
	 */
	mutex_enter(&alllwp_mutex);
	LIST_REMOVE(l, l_list);
	mutex_exit(&alllwp_mutex);

	/*
	 * Verify that we hold no locks other than the kernel mutex, and
	 * release our turnstile.  We should no longer sleep past this
	 * point.
	 */
	LOCKDEBUG_BARRIER(&kernel_lock, 0);
	pool_cache_put(&turnstile_cache, l->l_ts);

	/*
	 * Free MD LWP resources
	 */
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free(l, 0);
#endif
	pmap_deactivate(l);

	/*
	 * Release the kernel lock, and switch away into oblivion.
	 */
	(void)KERNEL_UNLOCK(0, l);	/* XXXSMP assert count == 1 */
	cpu_exit(l);

	/* NOTREACHED */
	return 0;
}

/*
 * We are called from cpu_exit() once it is safe to schedule the dead LWP's
 * resources to be freed (i.e., once we've switched to the idle PCB for the
 * current CPU).
 *
 * NOTE: One must be careful with locking in this routine.  It's called from
 * a critical section in machine-dependent code.
 */
void
lwp_exit2(struct lwp *l)
{
	struct proc *p;
	u_int refcnt;

	/*
	 * If someone holds a reference on the LWP, let them clean us up.
	 */
	lwp_lock(l);
	refcnt = --l->l_refcnt;
	lwp_unlock(l);
	if (refcnt != 0)
		return;

	KASSERT(l->l_stat == LSDEAD);
	KERNEL_LOCK(1, NULL);

	/*
	 * Free the VM resources we're still holding on to.
	 */
	uvm_lwp_exit(l);

	p = l->l_proc;

	if ((l->l_flag & L_DETACHED) != 0) {
		/*
		 * Nobody waits for detached LWPs.
		 */
		pool_put(&lwp_pool, l);
		(void)KERNEL_UNLOCK(1, NULL);

		/*
		 * If this is the last LWP in the process, wake up the
		 * parent so that it can reap us.
		 */
		mb_read();
		if (p->p_nlwps == 0) {
			KASSERT(p->p_stat == SDEAD);
			p->p_stat = SZOMB;
			mb_write();

			/* XXXSMP too much locking */
			mutex_enter(&proclist_mutex);
			mutex_enter(&proc_stop_mutex);
			p = p->p_pptr;
			p->p_nstopchild++;
			cv_broadcast(&p->p_waitcv);
			mutex_exit(&proc_stop_mutex);
			mutex_exit(&proclist_mutex);
		}
	} else {
		(void)KERNEL_UNLOCK(1, NULL);
		l->l_stat = LSZOMB;
		mb_write();
		mutex_enter(&p->p_smutex);
		wakeup(&p->p_nlwps);
		mutex_exit(&p->p_smutex);
	}
}

/*
 * Pick a LWP to represent the process for those operations which
 * want information about a "process" that is actually associated
 * with a LWP.
 *
 * Must be called with p->p_smutex held, and will return the LWP locked.
 * If 'locking' is false, no locking or lock checks are performed.  This
 * is intended for use by DDB.
 */
struct lwp *
proc_representative_lwp(struct proc *p, int *nrlwps, int locking)
{
	struct lwp *l, *onproc, *running, *sleeping, *stopped, *suspended;
	struct lwp *signalled;
	int cnt;

	if (locking) {
		LOCK_ASSERT(mutex_owned(&p->p_smutex));
	}

	/* Trivial case: only one LWP */
	if (p->p_nlwps == 1) {
		l = LIST_FIRST(&p->p_lwps);
		if (nrlwps)
			*nrlwps = (l->l_stat == LSONPROC || LSRUN);
		if (locking)
			lwp_lock(l);
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
			if (locking)
				lwp_lock(l);
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
			if (locking)
				lwp_unlock(l);
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
		if (locking)
			lwp_lock(l);
		return l;
	case SZOMB:
		/* Doesn't really matter... */
		if (nrlwps)
			*nrlwps = 0;
		l = LIST_FIRST(&p->p_lwps);
		if (locking)
			lwp_lock(l);
		return l;
#ifdef DIAGNOSTIC
	case SIDL:
		if (locking)
			mutex_exit(&p->p_smutex);
		/* We have more than one LWP and we're in SIDL?
		 * How'd that happen?
		 */
		panic("Too many LWPs in SIDL process %d (%s)",
		    p->p_pid, p->p_comm);
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
lwp_byid(struct proc *p, int id)
{
	struct lwp *l;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		if (l->l_lid == id)
			break;
	}

	if (l != NULL) {
		lwp_lock(l);
		if (l->l_stat == LSIDL || l->l_stat == LSZOMB ||
		    l->l_stat == LSDEAD) {
			lwp_unlock(l);
			l = NULL;
		}
	}

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

	for (;;) {
		mutex_exit(old);
		old = l->l_mutex;
		mutex_enter(old);

		/*
		 * mutex_enter() will have posted a read barrier.  Re-test
		 * l->l_mutex.  If it has changed, we need to try again.
		 */
	} while (__predict_false(l->l_mutex != old));
}
#endif

/*
 * Lend a new mutex to an LWP.  The old mutex must be held.
 */
void
lwp_setlock(struct lwp *l, kmutex_t *new)
{

	LOCK_ASSERT(mutex_owned(l->l_mutex));

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

	LOCK_ASSERT(mutex_owned(l->l_mutex));

	old = l->l_mutex;
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	mb_write();
	l->l_mutex = new;
#else
	(void)new;
#endif
	mutex_exit(old);
}

/*
 * Acquire a new mutex, and dontate it to an LWP.  The LWP must already be
 * locked.
 */
void
lwp_relock(struct lwp *l, kmutex_t *new)
{
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	kmutex_t *old;
#endif

	LOCK_ASSERT(mutex_owned(l->l_mutex));

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	old = l->l_mutex;
	if (old != new) {
		mutex_enter(new);
		l->l_mutex = new;
		mutex_exit(old);
	}
#else
	(void)new;
#endif
}

/*
 * Handle exceptions for mi_userret().  Called if L_USERRET is set.
 */
void
lwp_userret(struct lwp *l)
{
	struct proc *p;
	int sig;

	p = l->l_proc;

	do {
		/* Process pending signals first. */
		if ((l->l_flag & L_PENDSIG) != 0) {
			KERNEL_LOCK(1, l);	/* XXXSMP pool_put() below */
			mutex_enter(&p->p_smutex);
			while ((sig = issignal(l)) != 0)
				postsig(sig);
			mutex_exit(&p->p_smutex);
			(void)KERNEL_UNLOCK(0, l);	/* XXXSMP */
		}

		/* Core-dump or suspend pending. */
		if ((l->l_flag & L_WSUSPEND) != 0) {
			/*
			 * Suspend ourselves, so that the kernel stack and
			 * therefore the userland registers saved in the
			 * trapframe are around for coredump() to write them
			 * out.  We issue a wakeup() on p->p_nrlwps so that
			 * sigexit() will write the core file out once all
			 * other LWPs are suspended.
			 */
			mutex_enter(&p->p_smutex);
			lwp_lock(l);
			lwp_relock(l, &lwp_mutex);
			p->p_nrlwps--;
			wakeup(&p->p_nrlwps);
			l->l_stat = LSSUSPENDED;
			mutex_exit(&p->p_smutex);
			mi_switch(l, NULL);
			lwp_lock(l);
		}

		/* Process is exiting. */
		if ((l->l_flag & L_WEXIT) != 0) {
			KERNEL_LOCK(1, l);
			(void)lwp_exit(l, 0);
			KASSERT(0);
			/* NOTREACHED */
		}
	} while ((l->l_flag & L_USERRET) != 0);
}

/*
 * Return non-zero if this the last live LWP in the process.  Called when
 * exiting, dumping core, waiting for other LWPs to exit, etc.  Accepts a
 * 'bias' value for deadlock detection.
 *
 * Must be called with p->p_smutex held.
 */
int
lwp_lastlive(int bias)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	KASSERT(l->l_stat == LSONPROC || l->l_stat == LSSTOP);

	return p->p_nrlwps - bias - (l->l_stat == LSONPROC) == 0;
}

/*
 * Add one reference to an LWP.  This will prevent the LWP from
 * transitioning from the LSDEAD state into LSZOMB, and thus keep
 * the lwp structure and PCB around to inspect.
 */
void
lwp_addref(struct lwp *l)
{

	LOCK_ASSERT(lwp_locked(l, NULL));
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

	lwp_exit2(l);
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
