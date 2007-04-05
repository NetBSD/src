/*	$NetBSD: kern_sleepq.c,v 1.7.2.1 2007/04/05 21:38:37 ad Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Sleep queue implementation, used by turnstiles and general sleep/wakeup
 * interfaces.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_sleepq.c,v 1.7.2.1 2007/04/05 21:38:37 ad Exp $");

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/proc.h> 
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/sleepq.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <uvm/uvm_extern.h>

int	sleepq_sigtoerror(struct lwp *, int);
void	updatepri(struct lwp *);

/* General purpose sleep table, used by ltsleep() and condition variables. */
sleeptab_t	sleeptab;

/*
 * sleeptab_init:
 *
 *	Initialize a sleep table.
 */
void
sleeptab_init(sleeptab_t *st)
{
	sleepq_t *sq;
	int i;

	for (i = 0; i < SLEEPTAB_HASH_SIZE; i++) {
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
		sq = &st->st_queues[i].st_queue;
		mutex_init(&st->st_queues[i].st_mutex, MUTEX_SPIN, IPL_SCHED);
		sleepq_init(sq, &st->st_queues[i].st_mutex);
#else
		sq = &st->st_queues[i];
		sleepq_init(sq, &sched_mutex);
#endif
	}
}

/*
 * sleepq_init:
 *
 *	Prepare a sleep queue for use.
 */
void
sleepq_init(sleepq_t *sq, kmutex_t *mtx)
{

	sq->sq_waiters = 0;
	sq->sq_mutex = mtx;
	TAILQ_INIT(&sq->sq_queue);
}

/*
 * sleepq_remove:
 *
 *	Remove an LWP from a sleep queue and wake it up.  Return non-zero if
 *	the LWP is swapped out; if so the caller needs to awaken the swapper
 *	to bring the LWP into memory.
 */
int
sleepq_remove(sleepq_t *sq, struct lwp *l)
{
	struct cpu_info *ci;

	KASSERT(lwp_locked(l, sq->sq_mutex));
	KASSERT(sq->sq_waiters > 0);

	sq->sq_waiters--;
	TAILQ_REMOVE(&sq->sq_queue, l, l_sleepchain);

#ifdef DIAGNOSTIC
	if (sq->sq_waiters == 0)
		KASSERT(TAILQ_FIRST(&sq->sq_queue) == NULL);
	else
		KASSERT(TAILQ_FIRST(&sq->sq_queue) != NULL);
#endif

	l->l_syncobj = &sched_syncobj;
	l->l_wchan = NULL;
	l->l_sleepq = NULL;
	l->l_flag &= ~LW_SINTR;

	/*
	 * If not sleeping, the LWP must have been suspended.  Let whoever
	 * holds it stopped set it running again.
	 */
	if (l->l_stat != LSSLEEP) {
	 	KASSERT(l->l_stat == LSSTOP || l->l_stat == LSSUSPENDED);
		lwp_setlock(l, &sched_mutex);
		return 0;
	}

	sched_lock(1);
	lwp_setlock(l, &sched_mutex);

	/*
	 * If the LWP is still on the CPU, mark it as LSONPROC.  It may be
	 * about to call mi_switch(), in which case it will yield.
	 *
	 * XXXSMP Will need to change for preemption.
	 */
	ci = l->l_cpu;
#ifdef MULTIPROCESSOR
	if (ci->ci_curlwp == l) {
#else
	if (l == curlwp) {
#endif
		l->l_stat = LSONPROC;
		l->l_slptime = 0;
		sched_unlock(1);
		return 0;
	}

	/*
	 * Set it running.  We'll try to get the last CPU that ran
	 * this LWP to pick it up again.
	 */
	if (l->l_slptime > 1)
		updatepri(l);
	l->l_stat = LSRUN;
	l->l_slptime = 0;
	if ((l->l_flag & LW_INMEM) != 0) {
		setrunqueue(l);
		if (lwp_eprio(l) < ci->ci_schedstate.spc_curpriority)
			cpu_need_resched(ci);
		sched_unlock(1);
		return 0;
	}

	sched_unlock(1);
	return 1;
}

/*
 * sleepq_insert:
 *
 *	Insert an LWP into the sleep queue, optionally sorting by priority.
 */
inline void
sleepq_insert(sleepq_t *sq, struct lwp *l, syncobj_t *sobj)
{
	struct lwp *l2;
	const int pri = lwp_eprio(l);

	if ((sobj->sobj_flag & SOBJ_SLEEPQ_SORTED) != 0) {
		TAILQ_FOREACH(l2, &sq->sq_queue, l_sleepchain) {
			if (lwp_eprio(l2) > pri) {
				TAILQ_INSERT_BEFORE(l2, l, l_sleepchain);
				return;
			}
		}
	}

	TAILQ_INSERT_TAIL(&sq->sq_queue, l, l_sleepchain);
}

void
sleepq_enqueue(sleepq_t *sq, pri_t pri, wchan_t wchan, const char *wmesg,
    syncobj_t *sobj)
{
	struct lwp *l = curlwp;

	KASSERT(mutex_owned(sq->sq_mutex));
	KASSERT(l->l_stat == LSONPROC);
	KASSERT(l->l_wchan == NULL && l->l_sleepq == NULL);

	l->l_syncobj = sobj;
	l->l_wchan = wchan;
	l->l_sleepq = sq;
	l->l_wmesg = wmesg;
	l->l_slptime = 0;
	l->l_priority = pri;
	l->l_stat = LSSLEEP;
	l->l_sleeperr = 0;

	sq->sq_waiters++;
	sleepq_insert(sq, l, sobj);
}

void
sleepq_switch(int timo, int catch)
{
	struct lwp *l = curlwp;

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_CSW))
		ktrcsw(l, 1, 0);
#endif

	/*
	 * If sleeping interruptably, check for pending signals, exits or
	 * core dump events.
	 */
	if (catch) {
		l->l_flag |= LW_SINTR;
		if ((l->l_flag & LW_PENDSIG) != 0 && sigispending(l, 0)) {
			l->l_sleeperr = EPASSTHROUGH;
			/* lwp_unsleep() will release the lock */
			lwp_unsleep(l);
			return;
		}
		if ((l->l_flag & (LW_CANCELLED|LW_WEXIT|LW_WCORE)) != 0) {
			l->l_flag &= ~LW_CANCELLED;
			l->l_sleeperr = EINTR;
			/* lwp_unsleep() will release the lock */
			lwp_unsleep(l);
			return;
		}
	}

	if (timo)
		callout_reset(&l->l_tsleep_ch, timo, sleepq_timeout, l);

	mi_switch(l, NULL);
	l->l_cpu->ci_schedstate.spc_curpriority = l->l_usrpri;

	/*
	 * When we reach this point, the LWP and sleep queue are unlocked.
	 */
	KASSERT(l->l_wchan == NULL && l->l_sleepq == NULL);
}

/*
 * sleepq_block:
 *
 *	Enter an LWP into the sleep queue and prepare for sleep.  The sleep
 *	queue must already be locked, and any interlock (such as the kernel
 *	lock) must have be released (see sleeptab_lookup(), sleepq_enter()).
 *
 * 	sleepq_block() may return early under exceptional conditions, for
 * 	example if the LWP's containing process is exiting.
 */
void
sleepq_block(sleepq_t *sq, pri_t pri, wchan_t wchan, const char *wmesg,
	     int timo, int catch, syncobj_t *sobj)
{

	sleepq_enqueue(sq, pri, wchan, wmesg, sobj);
	sleepq_switch(timo, catch);
}

/*
 * sleepq_unblock:
 *
 *	After any intermediate step such as updating statistics, re-acquire
 *	the kernel lock and record the switch for ktrace.  Note that we are
 *	no longer on the sleep queue at this point.
 *
 *	This is split out from sleepq_block() in expectation that at some
 *	point in the future, LWPs may awake on different kernel stacks than
 *	those they went asleep on.
 */
int
sleepq_unblock(int timo, int catch)
{
	int error, expired, sig;
	struct proc *p;
	struct lwp *l;

	l = curlwp;
	error = l->l_sleeperr;

	if (timo) {
		/*
		 * Even if the callout appears to have fired, we need to
		 * stop it in order to synchronise with other CPUs.
		 */
		expired = callout_expired(&l->l_tsleep_ch);
		callout_stop(&l->l_tsleep_ch);
		if (expired && error == 0)
			error = EWOULDBLOCK;
	}

	if (catch && (error == 0 || error == EPASSTHROUGH)) {
		l->l_sleeperr = 0;
		p = l->l_proc;
		if ((l->l_flag & (LW_CANCELLED | LW_WEXIT | LW_WCORE)) != 0)
			error = EINTR;
		else if ((l->l_flag & LW_PENDSIG) != 0) {
			KERNEL_LOCK(1, l);	/* XXXSMP pool_put() */
			mutex_enter(&p->p_smutex);
			if ((sig = issignal(l)) != 0)
				error = sleepq_sigtoerror(l, sig);
			mutex_exit(&p->p_smutex);
			KERNEL_UNLOCK_LAST(l);
		}
		if (error == EPASSTHROUGH) {
			/* Raced */
			error = EINTR;
		}
	}

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_CSW))
		ktrcsw(l, 0, 0);
#endif

	KERNEL_LOCK(l->l_biglocks, l);
	return error;
}

/*
 * sleepq_wake:
 *
 *	Wake zero or more LWPs blocked on a single wait channel.
 */
void
sleepq_wake(sleepq_t *sq, wchan_t wchan, u_int expected)
{
	struct lwp *l, *next;
	int swapin = 0;

	KASSERT(mutex_owned(sq->sq_mutex));

	for (l = TAILQ_FIRST(&sq->sq_queue); l != NULL; l = next) {
		KASSERT(l->l_sleepq == sq);
		next = TAILQ_NEXT(l, l_sleepchain);
		if (l->l_wchan != wchan)
			continue;
		swapin |= sleepq_remove(sq, l);
		if (--expected == 0)
			break;
	}

	sleepq_unlock(sq);

	/*
	 * If there are newly awakend threads that need to be swapped in,
	 * then kick the swapper into action.
	 */
	if (swapin)
		uvm_kick_scheduler();
}

/*
 * sleepq_unsleep:
 *
 *	Remove an LWP from its sleep queue and set it runnable again. 
 *	sleepq_unsleep() is called with the LWP's mutex held, and will
 *	always release it.
 */
void
sleepq_unsleep(struct lwp *l)
{
	sleepq_t *sq = l->l_sleepq;
	int swapin;

	KASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_wchan != NULL);
	KASSERT(l->l_mutex == sq->sq_mutex);

	swapin = sleepq_remove(sq, l);
	sleepq_unlock(sq);

	if (swapin)
		uvm_kick_scheduler();
}

/*
 * sleepq_timeout:
 *
 *	Entered via the callout(9) subsystem to time out an LWP that is on a
 *	sleep queue.
 */
void
sleepq_timeout(void *arg)
{
	struct lwp *l = arg;

	/*
	 * Lock the LWP.  Assuming it's still on the sleep queue, its
	 * current mutex will also be the sleep queue mutex.
	 */
	lwp_lock(l);

	if (l->l_wchan == NULL) {
		/* Somebody beat us to it. */
		lwp_unlock(l);
		return;
	}

	lwp_unsleep(l);
}

/*
 * sleepq_sigtoerror:
 *
 *	Given a signal number, interpret and return an error code.
 */
int
sleepq_sigtoerror(struct lwp *l, int sig)
{
	struct proc *p = l->l_proc;
	int error;

	KASSERT(mutex_owned(&p->p_smutex));

	/*
	 * If this sleep was canceled, don't let the syscall restart.
	 */
	if ((SIGACTION(p, sig).sa_flags & SA_RESTART) == 0)
		error = EINTR;
	else
		error = ERESTART;

	return error;
}

/*
 * sleepq_abort:
 *
 *	After a panic or during autoconfiguration, lower the interrupt
 *	priority level to give pending interrupts a chance to run, and
 *	then return.  Called if sleepq_dontsleep() returns non-zero, and
 *	always returns zero.
 */
int
sleepq_abort(kmutex_t *mtx, int unlock)
{ 
	extern int safepri;
	int s;

	s = splhigh();
	splx(safepri);
	splx(s);
	if (mtx != NULL && unlock != 0)
		mutex_exit(mtx);

	return 0;
}

/*
 * sleepq_changepri:
 *
 *	Adjust the priority of an LWP residing on a sleepq.  This method
 *	will only alter the user priority; the effective priority is
 *	assumed to have been fixed at the time of insertion into the queue.
 */
void
sleepq_changepri(struct lwp *l, pri_t pri)
{

	KASSERT(lwp_locked(l, l->l_sleepq->sq_mutex));
	l->l_usrpri = pri;
}

void
sleepq_lendpri(struct lwp *l, pri_t pri)
{
	sleepq_t *sq = l->l_sleepq;
	pri_t opri;

	KASSERT(lwp_locked(l, sq->sq_mutex));

	opri = lwp_eprio(l);
	l->l_inheritedprio = pri;

	if (lwp_eprio(l) != opri &&
	    (l->l_syncobj->sobj_flag & SOBJ_SLEEPQ_SORTED) != 0) {
		TAILQ_REMOVE(&sq->sq_queue, l, l_sleepchain);
		sleepq_insert(sq, l, l->l_syncobj);
	}
}
