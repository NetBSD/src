/*	$NetBSD: kern_condvar.c,v 1.11 2007/08/01 23:21:14 ad Exp $	*/

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
 * Kernel condition variable implementation, modeled after those found in
 * Solaris, a description of which can be found in:
 *
 *	Solaris Internals: Core Kernel Architecture, Jim Mauro and
 *	    Richard McDougall.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_condvar.c,v 1.11 2007/08/01 23:21:14 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/sleepq.h>

static void	cv_unsleep(lwp_t *);
static void	cv_changepri(lwp_t *, pri_t);

static syncobj_t cv_syncobj = {
	SOBJ_SLEEPQ_SORTED,
	cv_unsleep,
	cv_changepri,
	sleepq_lendpri,
	syncobj_noowner,
};

static const char deadcv[] = "deadcv";

/*
 * cv_init:
 *
 *	Initialize a condition variable for use.
 */
void
cv_init(kcondvar_t *cv, const char *wmesg)
{

	KASSERT(wmesg != NULL);

	cv->cv_wmesg = wmesg;
	cv->cv_waiters = 0;
}

/*
 * cv_destroy:
 *
 *	Tear down a condition variable.
 */
void
cv_destroy(kcondvar_t *cv)
{

#ifdef DIAGNOSTIC
	KASSERT(cv->cv_wmesg != deadcv && cv->cv_wmesg != NULL);
	KASSERT(cv->cv_waiters == 0);
	cv->cv_wmesg = deadcv;
#endif
}

/*
 * cv_enter:
 *
 *	Look up and lock the sleep queue corresponding to the given
 *	condition variable, and increment the number of waiters.
 */
static inline sleepq_t *
cv_enter(kcondvar_t *cv, kmutex_t *mtx, lwp_t *l)
{
	sleepq_t *sq;

	KASSERT(cv->cv_wmesg != deadcv && cv->cv_wmesg != NULL);
	KASSERT((l->l_flag & LW_INTR) == 0);

	l->l_cv_signalled = 0;
	sq = sleeptab_lookup(&sleeptab, cv);
	cv->cv_waiters++;
	sleepq_enter(sq, l);
	sleepq_enqueue(sq, sched_kpri(l), cv, cv->cv_wmesg, &cv_syncobj);
	mutex_exit(mtx);

	return sq;
}

/*
 * cv_exit:
 *
 *	After resuming execution, check to see if we have been restarted
 *	as a result of cv_signal().  If we have, but cannot take the
 *	wakeup (because of eg a pending Unix signal or timeout) then try
 *	to ensure that another LWP sees it.  This is necessary because
 *	there may be multiple waiters, and at least one should take the
 *	wakeup if possible.
 */
static inline int
cv_exit(kcondvar_t *cv, kmutex_t *mtx, lwp_t *l, const int error)
{

	mutex_enter(mtx);
	if (__predict_false(error != 0) && l->l_cv_signalled != 0)
		cv_signal(cv);

	KASSERT(cv->cv_wmesg != deadcv && cv->cv_wmesg != NULL);

	return error;
}

/*
 * cv_unsleep:
 *
 *	Remove an LWP from the condition variable and sleep queue.  This
 *	is called when the LWP has not been awoken normally but instead
 *	interrupted: for example, when a signal is received.  Must be
 *	called with the LWP locked, and must return it unlocked.
 */
static void
cv_unsleep(lwp_t *l)
{
	kcondvar_t *cv;

	KASSERT(l->l_wchan != NULL);
	KASSERT(lwp_locked(l, l->l_sleepq->sq_mutex));

	cv = (kcondvar_t *)(uintptr_t)l->l_wchan;
	KASSERT(cv->cv_wmesg != deadcv && cv->cv_wmesg != NULL);
	cv->cv_waiters--;

	sleepq_unsleep(l);
}

/*
 * cv_changepri:
 *
 *	Adjust the real (user) priority of an LWP blocked on a CV.
 */
static void
cv_changepri(lwp_t *l, pri_t pri)
{
	sleepq_t *sq = l->l_sleepq;
	pri_t opri;

	KASSERT(lwp_locked(l, sq->sq_mutex));

	opri = lwp_eprio(l);
	l->l_usrpri = pri;
	l->l_priority = sched_kpri(l);

	if (lwp_eprio(l) != opri) {
		TAILQ_REMOVE(&sq->sq_queue, l, l_sleepchain);
		sleepq_insert(sq, l, l->l_syncobj);
	}
}

/*
 * cv_wait:
 *
 *	Wait non-interruptably on a condition variable until awoken.
 */
void
cv_wait(kcondvar_t *cv, kmutex_t *mtx)
{
	lwp_t *l = curlwp;
	sleepq_t *sq;

	KASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(mtx, 0);
		return;
	}

	sq = cv_enter(cv, mtx, l);
	(void)sleepq_block(0, false);
	(void)cv_exit(cv, mtx, l, 0);
}

/*
 * cv_wait_sig:
 *
 *	Wait on a condition variable until a awoken or a signal is received. 
 *	Will also return early if the process is exiting.  Returns zero if
 *	awoken normallly, ERESTART if a signal was received and the system
 *	call is restartable, or EINTR otherwise.
 */
int
cv_wait_sig(kcondvar_t *cv, kmutex_t *mtx)
{
	lwp_t *l = curlwp;
	sleepq_t *sq;
	int error;

	KASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, 0);

	sq = cv_enter(cv, mtx, l);
	error = sleepq_block(0, true);
	return cv_exit(cv, mtx, l, error);
}

/*
 * cv_timedwait:
 *
 *	Wait on a condition variable until awoken or the specified timeout
 *	expires.  Returns zero if awoken normally or EWOULDBLOCK if the
 *	timeout expired.
 */
int
cv_timedwait(kcondvar_t *cv, kmutex_t *mtx, int timo)
{
	lwp_t *l = curlwp;
	sleepq_t *sq;
	int error;

	KASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, 0);

	sq = cv_enter(cv, mtx, l);
	error = sleepq_block(timo, false);
	return cv_exit(cv, mtx, l, error);
}

/*
 * cv_timedwait_sig:
 *
 *	Wait on a condition variable until a timeout expires, awoken or a
 *	signal is received.  Will also return early if the process is
 *	exiting.  Returns zero if awoken normallly, EWOULDBLOCK if the
 *	timeout expires, ERESTART if a signal was received and the system
 *	call is restartable, or EINTR otherwise.
 */
int
cv_timedwait_sig(kcondvar_t *cv, kmutex_t *mtx, int timo)
{
	lwp_t *l = curlwp;
	sleepq_t *sq;
	int error;

	KASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, 0);

	sq = cv_enter(cv, mtx, l);
	error = sleepq_block(timo, true);
	return cv_exit(cv, mtx, l, error);
}

/*
 * cv_signal:
 *
 *	Wake the highest priority LWP waiting on a condition variable.
 *	Must be called with the interlocking mutex held.
 */
void
cv_signal(kcondvar_t *cv)
{
	lwp_t *l;
	sleepq_t *sq;

	if (cv->cv_waiters == 0)
		return;

	/*
	 * cv->cv_waiters may be stale and have dropped to zero, but
	 * while holding the interlock (the mutex passed to cv_wait()
	 * and similar) we will see non-zero values when it matters.
	 */

	sq = sleeptab_lookup(&sleeptab, cv);
	if (cv->cv_waiters != 0) {
		cv->cv_waiters--;
		l = sleepq_wake(sq, cv, 1);
		l->l_cv_signalled = 1;
	} else
		sleepq_unlock(sq);
}

/*
 * cv_broadcast:
 *
 *	Wake all LWPs waiting on a condition variable.  Must be called
 *	with the interlocking mutex held.
 */
void
cv_broadcast(kcondvar_t *cv)
{
	sleepq_t *sq;
	u_int cnt;

	if (cv->cv_waiters == 0)
		return;

	sq = sleeptab_lookup(&sleeptab, cv);
	if ((cnt = cv->cv_waiters) != 0) {
		cv->cv_waiters = 0;
		sleepq_wake(sq, cv, cnt);
	} else
		sleepq_unlock(sq);
}

/*
 * cv_wakeup:
 *
 *	Wake all LWPs waiting on a condition variable.  For cases
 *	where the address may be waited on by mtsleep()/tsleep().
 *	Not a documented call.
 */
void
cv_wakeup(kcondvar_t *cv)
{
	sleepq_t *sq;
	u_int cnt;

	sq = sleeptab_lookup(&sleeptab, cv);
	if ((cnt = cv->cv_waiters) != 0) {
		cv->cv_waiters = 0;
		sleepq_wake(sq, cv, cnt);
	} else
		sleepq_unlock(sq);
}

/*
 * cv_has_waiters:
 *
 *	For diagnostic assertions: return non-zero if a condition
 *	variable has waiters.
 */
bool
cv_has_waiters(kcondvar_t *cv)
{

	/* No need to interlock here */
	return cv->cv_waiters != 0;
}
