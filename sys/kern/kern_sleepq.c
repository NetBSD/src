/*	$NetBSD: kern_sleepq.c,v 1.1.2.4 2006/10/24 21:10:21 ad Exp $	*/

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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_sleepq.c,v 1.1.2.4 2006/10/24 21:10:21 ad Exp $");

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

int	sleepq_sigtoerror(struct lwp *, int);
void	sleepq_exit(sleepq_t *, struct lwp *);
void	updatepri(struct lwp *);
void	sa_awaken(struct lwp *);

sleepq_t	sleeptab[SLEEPTAB_HASH_SIZE];
#ifdef MULTIPROCESSOR
kmutex_t	sleeptab_mutexes[SLEEPTAB_HASH_SIZE];
#endif

/*
 * sleeptab_init:
 *
 *	Initialize the general-purpose sleep queues.
 */
void
sleeptab_init(void)
{
	sleepq_t *sq;
	int i;

	for (i = 0; i < SLEEPTAB_HASH_SIZE; i++) {
		sq = &sleeptab[i];
#ifdef MULTIPROCESSOR
		mutex_init(&sleeptab_mutexes[i], MUTEX_SPIN, IPL_SCHED);
		sleepq_init(&sleeptab[i], &sleeptab_mutexes[i]);
#else
		sleepq_init(&sleeptab[i], &sched_mutex);
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
	LOCK_ASSERT(mutex_owned(&sched_mutex));
	KASSERT(sq->sq_waiters > 0);

	sq->sq_waiters--;
	TAILQ_REMOVE(&sq->sq_queue, l, l_sleepq);

#ifdef DIAGNOSTIC
	if (sq->sq_waiters == 0)
		KASSERT(TAILQ_FIRST(&sq->sq_queue) == NULL);
	else
		KASSERT(TAILQ_FIRST(&sq->sq_queue) != NULL);
#endif

	l->l_wchan = NULL;
	l->l_flag &= ~L_SINTR;

	/*
	 * If not sleeping, the LWP must have been suspended.  Let whoever
	 * holds it stopped set it running again.
	 */
	if (l->l_stat != LSSLEEP) {
		KASSERT(l->l_stat == LSSTOP || l->l_stat == LSSUSPENDED);
		return 0;
	}

	if (l->l_proc->p_sa)
		sa_awaken(l);

	lwp_setlock(l, &sched_mutex);

	/*
	 * If the LWP is still on the CPU, mark it as LSONPROC.  It may be
	 * about to call mi_switch(), in which case it will yield.
	 */
	if ((ci = l->l_cpu) != NULL && ci->ci_curlwp == l) {
		l->l_stat = LSONPROC;
		l->l_slptime = 0;
		return 0;
	}

	/*
	 * Set it running.  We'll try to get the last CPU that ran
	 * this LWP to pick it up again.
	 */
	l->l_stat = LSRUN;
	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;
	if ((l->l_flag & L_INMEM) != 0) {
		setrunqueue(l);
		cpu_need_resched(l->l_cpu);
		return 0;
	}

	return 1;
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
	     int catch)
{
	struct lwp *l = curlwp;

	LOCK_ASSERT(mutex_owned(sq->sq_mutex));

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(l, 1, 0);
#endif

	sq->sq_waiters++;
	TAILQ_INSERT_TAIL(&sq->sq_queue, l, l_sleepq);

	/*
	 * Acquire the per-LWP mutex.
	 */
	lwp_lock(l);

	KASSERT(l->l_wchan == NULL);

	l->l_wchan = wchan;
	l->l_wmesg = wmesg;
	l->l_slptime = 0;
	l->l_priority = pri & PRIMASK;
	l->l_flag &= ~L_CANCELLED;
	if (catch)
		l->l_flag |= L_SINTR;
	if (l->l_stat == LSONPROC)
		l->l_stat = LSSLEEP;
	l->l_nvcsw++;

	if (timo)
		callout_reset(&l->l_tsleep_ch, timo, sleepq_timeout, l);

	/*
	 * The LWP is now on the sleep queue.  Release its old mutex and
	 * lend it ours for the duration of the sleep.
	 */
	lwp_setlock_unlock(l, sq->sq_mutex);
}

/*
 * sleepq_exit:
 *
 *	Remove the current LWP from a sleep queue after the sleep has been
 *	interrupted.
 */
void
sleepq_exit(sleepq_t *sq, struct lwp *l)
{

	LOCK_ASSERT(lwp_locked(l, sq->sq_mutex));
	KASSERT(sq->sq_waiters > 0);
	KASSERT(l->l_stat == LSSLEEP);

	l->l_wchan = NULL;
	l->l_slptime = 0;
	l->l_flag &= ~L_SINTR;

	sq->sq_waiters--;
	TAILQ_REMOVE(&sq->sq_queue, l, l_sleepq);

#ifdef DIAGNOSTIC
	if (sq->sq_waiters == 0)
		KASSERT(TAILQ_FIRST(&sq->sq_queue) == NULL);
	else
		KASSERT(TAILQ_FIRST(&sq->sq_queue) != NULL);
#endif

	l->l_stat = LSONPROC;

	lwp_setlock_unlock(l, &sched_mutex);
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

	flag = l->l_flag;
	error = 0;

	/*
	 * If sleeping interruptably, check for pending signals, exits or
	 * core dump events.
	 */
	if ((flag & L_SINTR) != 0) {
		while ((l->l_flag & L_PENDSIG) != 0 && error == 0) {
			lwp_unlock(l);
			p = l->l_proc;
			mutex_enter(&p->p_smutex);
			if ((sig = issignal(l)) != 0)
				error = sleepq_sigtoerror(l, sig);
			mutex_exit(&p->p_smutex);
			lwp_lock(l);
		}

		if ((l->l_flag & (L_CANCELLED | L_WEXIT | L_WCORE)) != 0)
			error = EINTR;
	}

	if (error != 0 && l->l_stat == LSSLEEP)
		sleepq_exit(sq, l);
	else if (l->l_stat != LSONPROC) {
		if ((flag & L_SA) != 0) {
			sa_switch(l, sadata_upcall_alloc(0), SA_UPCALL_BLOCKED);
			/* XXXAD verify sa_switch restores SPL. */
		} else {
			mi_switch(l, NULL);
			l->l_cpu->ci_schedstate.spc_curpriority = l->l_usrpri;
		}
	}

	KASSERT(l->l_wchan == NULL);

	if (timo) {
		/*
		 * Even if the callout appears to have fired, we need to
		 * stop it in order to synchronise with other CPUs.
		 */
		expired = callout_expired(&l->l_tsleep_ch);
		callout_stop(&l->l_tsleep_ch);
		if (expired && error == 0)
			return EWOULDBLOCK;
	}

	if (error == 0 && (flag & L_SINTR) != 0) {
		if ((l->l_flag & (L_CANCELLED | L_WEXIT | L_WCORE)) != 0)
			error = EINTR;
		else if ((l->l_flag & L_PENDSIG) != 0) {
			p = l->l_proc;
			mutex_enter(&p->p_smutex);
			if ((sig = issignal(l)) != 0)
				error = sleepq_sigtoerror(l, sig);
			mutex_exit(&p->p_smutex);
		}
	}

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(l, 0, 0);
#endif
	return error;
}

/*
 * sleepq_wakeone:
 *
 *	Remove one LWP from the sleep queue and wake it.  We search among
 *	the higest priority LWPs waiting on a single wait channel, and pick
 *	the longest waiting one.
 */
void
sleepq_wakeone(sleepq_t *sq, wchan_t wchan)
{
	struct lwp *l, *bl;
	int bpri, swapin;

	LOCK_ASSERT(mutex_owned(sq->sq_mutex));

	swapin = 0;
	bpri = MAXPRI;
	bl = NULL;

	TAILQ_FOREACH(l, &sq->sq_queue, l_sleepq) {
		if (l->l_wchan != wchan || l->l_priority > bpri)
			continue;
		bl = l;
		bpri = l->l_priority;
	}

	if (bl != NULL) {
		sched_lock();
		swapin = sleepq_remove(sq, bl);
		sched_unlock();
	}

	mutex_exit(sq->sq_mutex);

	if (swapin)
		wakeup(&proc0);
}

/*
 * sleepq_wakeall:
 *
 *	Wake all LWPs blocked on a single wait channel.
 */
void
sleepq_wakeall(sleepq_t *sq, wchan_t wchan, u_int expected)
{
	struct lwp *l, *next;
	int swapin = 0;

	LOCK_ASSERT(mutex_owned(sq->sq_mutex));

	sched_lock();
	for (l = TAILQ_FIRST(&sq->sq_queue); l != NULL; l = next) {
		next = TAILQ_NEXT(l, l_sleepq);
		if (l->l_wchan != wchan)
			continue;
		swapin |= sleepq_remove(sq, l);
		if (--expected == 0)
			break;
	}
	sched_unlock();

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
	sleepq_t *sq;
	int swapin;

	sq = &sleeptab[SLEEPTAB_HASH(l->l_wchan)];
	KASSERT(l->l_wchan != NULL);
	KASSERT(l->l_mutex == sq->sq_mutex);

	sched_lock();
	swapin = sleepq_remove(sq, l);
	sched_unlock();

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

	sleepq_unsleep(arg);
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
