/*	$NetBSD: kern_lwp.c,v 1.40.2.3 2006/10/24 21:10:21 ad Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_lwp.c,v 1.40.2.3 2006/10/24 21:10:21 ad Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>

#include <uvm/uvm_extern.h>

struct lwplist	alllwp;
kmutex_t	alllwp_mutex;
kmutex_t	lwp_mutex;

#define LWP_DEBUG

#ifdef LWP_DEBUG
int lwp_debug = 0;
#define DPRINTF(x) if (lwp_debug) printf x
#else
#define DPRINTF(x)
#endif

/*
 * Set an LWP halted or suspended.
 *
 * Must be called with p_smutex held, and the LWP locked.  Will unlock the
 * LWP before return.
 */
int
lwp_halt(struct lwp *curl, struct lwp *t, int state)
{
	struct proc *p = t->l_proc;
	int error;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	LOCK_ASSERT(lwp_locked(t, NULL));

	KASSERT(curl != t || curl->l_stat == LSONPROC);

	/*
	 * If the current LWP has been told to exit, we must not suspend anyone
	 * else or deadlock could occur.  We won't return to userspace.
	 */
	if ((curl->l_stat & (L_WEXIT | L_WCORE)) != 0)
		return (EDEADLK);

	error = 0;

	switch (t->l_stat) {
	case LSRUN:
		p->p_nrlwps--;
		t->l_stat = state;
		remrunqueue(t);
		break;
	case LSONPROC:
		p->p_nrlwps--;
		t->l_stat = state;
		if (t != curl) {
#ifdef MULTIPROCESSOR
			cpu_need_resched(t->l_cpu);
#elif defined(DIAGNOSTIC)
			panic("lwp_halt: onproc but not self");
#endif
		}
		break;
	case LSSLEEP:
		p->p_nrlwps--;
		/* FALLTHROUGH */
	case LSSUSPENDED:
	case LSSTOP:
		/* XXXAD What about restarting stopped -> suspended?? */
		t->l_stat = state;
		break;
	case LSIDL:
	case LSZOMB:
		error = EINTR; /* It's what Solaris does..... */
		break;
	}

	lwp_setlock_unlock(t, &lwp_mutex);

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
	LOCK_ASSERT(mutex_owned(l->l_mutex));

	DPRINTF(("lwp_continue of %d.%d (%s), state %d, wchan %p\n",
	    l->l_proc->p_pid, l->l_lid, l->l_proc->p_comm, l->l_stat,
	    l->l_wchan));

	if (l->l_stat != LSSUSPENDED) {
		lwp_unlock(l);
		return;
	}

	if (l->l_wchan == NULL) {
		/*
		 * LWP was runnable before being suspended.  setrunnable()
		 * will release the lock.
		 */
		setrunnable(l);
	} else {
		/* LWP was sleeping before being suspended. */
		l->l_proc->p_nrlwps++;
		l->l_stat = LSSLEEP;
		lwp_unlock(l);
	}
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

	memset(&l2->l_startzero, 0,
	       (unsigned) ((caddr_t)&l2->l_endzero -
			   (caddr_t)&l2->l_startzero));
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
	l2->l_mutex = &sched_mutex;
#else
	/*
	 * zero child's CPU pointer so we don't get trash.
	 */
	l2->l_cpu = NULL;
	l2->l_mutex = &lwp_mutex;
#endif /* ! MULTIPROCESSOR */

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
	l2->l_omutex = NULL;

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

	/* XXXAD verify */
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

	DPRINTF(("lwp_exit: %d.%d exiting.\n", p->p_pid, l->l_lid));
	DPRINTF((" nlwps: %d nzlwps: %d\n", p->p_nlwps, p->p_nzlwps));

	if (p->p_emul->e_lwp_exit)
		(*p->p_emul->e_lwp_exit)(l);

	/*
	 * If we are the last live LWP in a process, we need to exit the
	 * entire process.  We do so with an exit status of zero, because
	 * it's a "controlled" exit, and because that's what Solaris does.
	 *
	 * We are not quite a zombie yet, but for accounting purposes we
	 * must increment the count of zombies here.
	 */
	mutex_enter(&p->p_smutex);
	p->p_nzlwps++;
	if ((p->p_nlwps - p->p_nzlwps) == (p->p_stat == LSONPROC)) {
		DPRINTF(("lwp_exit: %d.%d calling exit1()\n",
		    p->p_pid, l->l_lid));
		exit1(l, 0);
		/* NOTREACHED */
	}
	mutex_exit(&p->p_smutex);

	/*
	 * Remove the LWP from the global list, from the parent process and
	 * then mark it as dead.  Nothing should be able to find or update
	 * it past this point.
	 */
	mutex_enter(&alllwp_mutex);
	LIST_REMOVE(l, l_list);
	mutex_exit(&alllwp_mutex);

	/*
	 * Mark us as dead (almost a zombie) and bin any pending signals
	 * that remain undelivered.
	 *
	 * XXX We should put whole-process signals back onto the process's
	 * pending set and find someone else to deliver them.
	 */
	mutex_enter(&p->p_smutex);
	lwp_lock(l);
	if ((l->l_flag & L_DETACHED) != 0) {
		LIST_REMOVE(l, l_sibling);
		p->p_nlwps--;
		curlwp = NULL;
		l->l_proc = NULL;
	}
	l->l_stat = LSDEAD;
	lwp_setlock_unlock(l, &lwp_mutex);
	if ((p->p_flag & P_SA) == 0)
		sigclear(l->l_sigpend, NULL);
	mutex_exit(&p->p_smutex);

	/*
	 * Release our cached credentials and collate accounting flags.
	 */
	kauth_cred_free(l->l_cred);
	mutex_enter(&p->p_crmutex);
	p->p_acflag |= l->l_acflag;
	mutex_exit(&p->p_crmutex);

	/*
	 * Verify that we hold no locks other than the kernel mutex, and
	 * release our turnstile.  We can no longer acquire sleep locks
	 * past this point.
	 */
	LOCKDEBUG_BARRIER(&kernel_mutex, 0);
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
	KERNEL_PROC_UNLOCK(l);
	cpu_exit(l);
}

/*
 * We are called from cpu_exit() once it is safe to schedule the
 * dead process's resources to be freed (i.e., once we've switched to
 * the idle PCB for the current CPU).
 *
 * NOTE: One must be careful with locking in this routine.  It's
 * called from a critical section in machine-dependent code, so
 * we should refrain from changing any interrupt state.
 */
void
lwp_exit2(struct lwp *l)
{

	KERNEL_LOCK(LK_EXCLUSIVE);

	/*
	 * Free the VM resources we're still holding on to.
	 */
	uvm_lwp_exit(l);

	if (l->l_flag & L_DETACHED) {
		/* Nobody waits for detached LWPs. */
		pool_put(&lwp_pool, l);
		KERNEL_UNLOCK();
	} else {
		KERNEL_UNLOCK();
		l->l_stat = LSZOMB;
		mb_write();
		wakeup(&l->l_proc->p_nlwps);
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

	if (locking)
		LOCK_ASSERT(mutex_owned(&p->p_smutex));

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

	mutex_enter(&p->p_crmutex);
	kauth_cred_hold(p->p_cred);
	l->l_cred = p->p_cred;
	mutex_exit(&p->p_crmutex);
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
#ifdef MULTIPROCESSOR
	kmutex_t *cur = l->l_mutex;

	return mutex_owned(cur) && (mtx == cur || mtx == NULL);
#else
	return mutex_owned(l->l_mutex);
#endif
}

/*
 * Lock an LWP.
 */
void
lwp_lock(struct lwp *l)
{
#ifdef MULTIPROCESSOR
	kmutex_t *old;

	for (;;) {
		mutex_enter(old = l->l_mutex);

		/*
		 * mutex_enter() will have posted a read barrier.  Re-test
		 * l->l_mutex.  If it has changed, we need to try again.
		 */
		if (__predict_true(l->l_mutex == old)) {
			LOCK_ASSERT(l->l_omutex == NULL);
			return;
		}

		mutex_exit(old);
	}
#else
	mutex_enter(l->l_mutex);
#endif
}

/*
 * Unlock an LWP.  If the LWP has been relocked, release the new mutex
 * first, then the old mutex.
 */
void
lwp_unlock(struct lwp *l)
{
#ifdef MULTIPROCESSOR
	kmutex_t *old;

	LOCK_ASSERT(mutex_owned(l->l_mutex));

	if (__predict_true((old = l->l_omutex) == NULL)) {
		mutex_exit(l->l_mutex);
		return;
	}

	l->l_omutex = NULL;
	mutex_exit(l->l_mutex);
	mutex_exit(old);
#else
	LOCK_ASSERT(mutex_owned(l->l_mutex));

	mutex_exit(l->l_mutex);
#endif
}

/*
 * Lend a new mutex to an LWP.  Both the old and new mutexes must be held.
 */
void
lwp_setlock(struct lwp *l, kmutex_t *new)
{
	LOCK_ASSERT(mutex_owned(l->l_mutex));
	LOCK_ASSERT(mutex_owned(new));
	LOCK_ASSERT(l->l_omutex == NULL);

#ifdef MULTIPROCESSOR
	mb_write();
	l->l_mutex = new;
#endif
}

/*
 * Lend a new mutex to an LWP, and release the old mutex.  The old mutex
 * must be held.
 */
void
lwp_setlock_unlock(struct lwp *l, kmutex_t *new)
{
	kmutex_t *old;

	LOCK_ASSERT(mutex_owned(l->l_mutex));
	LOCK_ASSERT(l->l_omutex == NULL);

	old = l->l_mutex;
#ifdef MULTIPROCESSOR
	mb_write();
	l->l_mutex = new;
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

	LOCK_ASSERT(mutex_owned(l->l_mutex));
	LOCK_ASSERT(l->l_omutex == NULL);

#ifdef MULTIPROCESSOR
	mutex_enter(new);
	l->l_omutex = l->l_mutex;
	mb_write();
	l->l_mutex = new;
#endif
}

/*
 * Handle exceptions for mi_userret().  Called if L_USERRET is set.
 *
 * Must be called with the LWP locked.
 */
void
lwp_userret(struct lwp *l)
{
	struct proc *p;
	int sig, flag;

	p = l->l_proc;
	flag = l->l_flag;

#ifdef MULTIPROCESSOR
	LOCK_ASSERT(lwp_locked(l, NULL));
	lwp_unlock(l);
#endif

	/* Signals must be processed first. */
	if ((flag & L_PENDSIG) != 0) {
		mutex_enter(&p->p_smutex);
		while ((sig = issignal(l)) != 0)
			postsig(sig);
		mutex_exit(&p->p_smutex);
	}

	if ((flag & L_WCORE) != 0) {
		/*
		 * Suspend ourselves, so that the kernel stack and therefore
		 * the userland registers saved in the trapframe are around
		 * for coredump() to write them out.  We issue a wakeup() on
		 * p->p_nrlwps so that sigexit() will write the core file out
		 * once all other LWPs are suspended.
		 */
		KERNEL_PROC_LOCK(l);
		mutex_enter(&p->p_smutex);
		p->p_nrlwps--;
		wakeup(&p->p_nrlwps);
		lwp_lock(l);
		l->l_flag &= ~L_DETACHED;
		l->l_stat = LSSUSPENDED;
		mutex_exit_linked(&p->p_smutex, l->l_mutex);
		mi_switch(l, NULL);
		lwp_exit(l);
		/* NOTREACHED */
	}

	if ((flag & L_WEXIT) != 0) {
		KERNEL_PROC_LOCK(l);
		lwp_exit(l);
		/* NOTREACHED */
	}

#ifdef MULTIPROCESSOR
	lwp_lock(l);
#endif
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
