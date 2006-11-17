/*	$NetBSD: kern_sleepq.c,v 1.1.2.6 2006/11/17 16:53:08 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_sleepq.c,v 1.1.2.6 2006/11/17 16:53:08 ad Exp $");

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
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/sleepq.h>

#ifdef KTRACE
#include <sys/ktrace.h>
#endif

int	sleepq_sigtoerror(struct lwp *, int);
void	updatepri(struct lwp *);
void	sa_awaken(struct lwp *);

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
		sq = &st->st_queues[i];
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
		mutex_init(&st->st_mutexes[i], MUTEX_SPIN, IPL_SCHED);
		sleepq_init(sq, &st->st_mutexes[i]);
#else
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

	LOCK_ASSERT(lwp_locked(l, sq->sq_mutex));
	KASSERT(sq->sq_waiters > 0);
	KASSERT(l->l_stat == LSSLEEP);

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
	l->l_flag &= ~L_SINTR;

	sched_lock(1);
	lwp_setlock(l, &sched_mutex);

	/*
	 * If the LWP is still on the CPU, mark it as LSONPROC.  It may be
	 * about to call mi_switch(), in which case it will yield.
	 */
	if ((ci = l->l_cpu) != NULL && ci->ci_curlwp == l) {
		l->l_stat = LSONPROC;
		l->l_slptime = 0;
		sched_unlock(1);
		return 0;
	}

	if (l->l_proc->p_sa)
		sa_awaken(l);

	/*
	 * Set it running.  We'll try to get the last CPU that ran
	 * this LWP to pick it up again.
	 */
	if (l->l_stat == LSSLEEP)
		l->l_stat = LSRUN;
	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;
	if ((l->l_flag & L_INMEM) != 0) {
		setrunqueue(l);
		if (l->l_priority < ci->ci_schedstate.spc_curpriority)
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
static inline void
sleepq_insert(sleepq_t *sq, struct lwp *l, int pri, syncobj_t *sobj)
{
	struct lwp *l2, *l3 = NULL;

	if ((sobj->sobj_flag & SOBJ_SLEEPQ_SORTED) != 0) {
		TAILQ_FOREACH(l2, &sq->sq_queue, l_sleepchain) {
			l3 = l2;
			if (l2->l_priority > pri)
				break;
		}
	}

	if (l3 == NULL)
		TAILQ_INSERT_HEAD(&sq->sq_queue, l, l_sleepchain);
	else
		TAILQ_INSERT_BEFORE(l3, l, l_sleepchain);
}

/*
 * sleepq_enter:
 *
 *	Enter an LWP into the sleep queue and prepare for sleep.  Any interlocking
 * 	step such as releasing a mutex or checking for signals may be safely done
 *	by the caller once on the sleep queue.
 */
void
sleepq_enter(sleepq_t *sq, int pri, wchan_t wchan, const char *wmesg, int timo,
	     int catch, syncobj_t *sobj)
{
	struct lwp *l = curlwp;

	LOCK_ASSERT(mutex_owned(sq->sq_mutex));
	KASSERT(l->l_stat == LSONPROC);
	KASSERT(l->l_wchan == NULL && l->l_sleepq == NULL);

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_CSW))
		ktrcsw(l, 1, 0);
#endif

	sq->sq_waiters++;

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/*
	 * Acquire the per-LWP mutex and sort it into the sleep queue.  Once
	 * we have that lock, we can release the kernel lock.  XXXSMP Not
	 * yet, since checking for signals may call pool_put().  Otherwise
	 * this is OK.
	 */
	lwp_lock(l);

#ifdef notyet
	l->l_biglocks = KERNEL_UNLOCK(0, l);
#endif
#endif

	l->l_syncobj = sobj;
	l->l_wchan = wchan;
	l->l_sleepq = sq;
	l->l_wmesg = wmesg;
	l->l_slptime = 0;
	l->l_priority = pri;
	l->l_stat = LSSLEEP;
	l->l_nvcsw++;

	if (catch)
		l->l_flag |= L_SINTR;

	sleepq_insert(sq, l, pri, sobj);

	if (timo)
		callout_reset(&l->l_tsleep_ch, timo, sleepq_timeout, l);

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/*
	 * The LWP is now on the sleep queue.  Release its old mutex and
	 * lend it ours for the duration of the sleep.
	 */
	lwp_unlock_to(l, sq->sq_mutex);
#endif
}

/*
 * sleepq_block:
 *
 *	The calling LWP has been entered into the sleep queue by
 *	sleepq_enter(), and now wants to block.  sleepq_block() may return
 *	early under exceptional conditions, for example if the LWP's process
 *	is exiting.  sleepq_block() must be called if sleepq_enter() has
 *	been called.
 */
int
sleepq_block(sleepq_t *sq, int timo)
{
	int error, flag, expired, sig;
	struct lwp *l = curlwp;
	struct proc *p;

	LOCK_ASSERT(lwp_locked(l, sq->sq_mutex));

	p = l->l_proc;
	flag = l->l_flag;
	error = 0;

	/*
	 * If sleeping interruptably, check for pending signals, exits or
	 * core dump events.
	 */
	if ((flag & L_SINTR) != 0) {
		while ((l->l_flag & L_PENDSIG) != 0 && error == 0) {
			lwp_unlock(l);
			mutex_enter(&p->p_smutex);
			if ((sig = issignal(l)) != 0)
				error = sleepq_sigtoerror(l, sig);
			mutex_exit(&p->p_smutex);
			lwp_lock(l);
		}

		if ((l->l_flag & (L_CANCELLED | L_WEXIT | L_WCORE)) != 0) {
			l->l_flag &= ~L_CANCELLED;
			error = EINTR;
		}

		if (l->l_wchan != NULL) {
			if (error != 0) {
				KASSERT(l->l_stat == LSSLEEP);
				sleepq_remove(sq, l);
				mutex_exit(sq->sq_mutex);
			}
		} else {
			KASSERT(l->l_stat == LSONPROC);
			lwp_unlock(l);
		}
	}

	if (l->l_stat == LSSLEEP) {
		KASSERT(l->l_wchan != NULL);

		if ((flag & L_SA) != 0)
			sa_switch(l, sadata_upcall_alloc(0), SA_UPCALL_BLOCKED);
		else {
			mi_switch(l, NULL);
			l->l_cpu->ci_schedstate.spc_curpriority = l->l_usrpri;
		}
	}

	/* When we reach this point, the LWP is unlocked. */

	KASSERT(l->l_wchan == NULL && l->l_sleepq == NULL);

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

	if (error == 0 && (flag & L_SINTR) != 0) {
		if ((l->l_flag & (L_CANCELLED | L_WEXIT | L_WCORE)) != 0)
			error = EINTR;
		else if ((l->l_flag & L_PENDSIG) != 0) {
			mutex_enter(&p->p_smutex);
			if ((sig = issignal(l)) != 0)
				error = sleepq_sigtoerror(l, sig);
			mutex_exit(&p->p_smutex);
		}
	}

	return error;
}

/*
 * sleepq_unblock:
 *
 *	After any intermediate step such as updating statistics, re-acquire
 *	the kernel lock and record the switch for ktrace.  Note that we are
 *	no longer on the sleep queue at this point.
 */
void
sleepq_unblock(void)
{
	struct lwp *l = curlwp;

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#ifdef notyet
	/*
	 * Re-acquire the kernel lock.  XXXSMP Let mi_switch() take care of
	 * it, until we can release the lock in sleepq_block().
	 */
	KERNEL_LOCK(l->l_biglocks, l);
#endif
#endif

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_CSW))
		ktrcsw(l, 0, 0);
#endif
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

	LOCK_ASSERT(mutex_owned(sq->sq_mutex));

	for (l = TAILQ_FIRST(&sq->sq_queue); l != NULL; l = next) {
		KASSERT(l->l_sleepq == sq);
		next = TAILQ_NEXT(l, l_sleepchain);
		if (l->l_wchan != wchan)
			continue;
		swapin |= sleepq_remove(sq, l);
		if (--expected == 0)
			break;
	}

	LOCK_ASSERT(mutex_owned(sq->sq_mutex));
	mutex_exit(sq->sq_mutex);

	/*
	 * If there are newly awakend threads that need to be swapped in,
	 * then kick the swapper into action.
	 */
	if (swapin)
		wakeup(&proc0);
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

	LOCK_ASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_wchan != NULL);
	KASSERT(l->l_mutex == sq->sq_mutex);

	swapin = sleepq_remove(sq, l);
	mutex_exit(sq->sq_mutex);

	if (swapin)
		wakeup(&proc0);
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

	sleepq_unsleep(l);
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

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

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
 *	Adjust the priority of an LWP residing on a sleepq.
 */
void
sleepq_changepri(struct lwp *l, int pri)
{
	sleepq_t *sq = l->l_sleepq;

	KASSERT(lwp_locked(l, sq->sq_mutex));

	TAILQ_REMOVE(&sq->sq_queue, l, l_sleepchain);
	sleepq_insert(sq, l, pri, l->l_syncobj);
}
