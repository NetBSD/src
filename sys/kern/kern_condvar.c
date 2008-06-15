/*	$NetBSD: kern_condvar.c,v 1.23 2008/06/15 09:56:18 chris Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_condvar.c,v 1.23 2008/06/15 09:56:18 chris Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>

#include <uvm/uvm_extern.h>

#define	CV_SLEEPQ(cv)	((sleepq_t *)(cv)->cv_opaque)
#define	CV_DEBUG_P(cv)	((cv)->cv_wmesg != nodebug)
#define	CV_RA		((uintptr_t)__builtin_return_address(0))

static u_int	cv_unsleep(lwp_t *, bool);
static void	cv_wakeup_one(kcondvar_t *);
static void	cv_wakeup_all(kcondvar_t *);

static syncobj_t cv_syncobj = {
	SOBJ_SLEEPQ_SORTED,
	cv_unsleep,
	sleepq_changepri,
	sleepq_lendpri,
	syncobj_noowner,
};

lockops_t cv_lockops = {
	"Condition variable",
	LOCKOPS_CV,
	NULL
};

static const char deadcv[] = "deadcv";
static const char nodebug[] = "nodebug";

/*
 * cv_init:
 *
 *	Initialize a condition variable for use.
 */
void
cv_init(kcondvar_t *cv, const char *wmesg)
{
#ifdef LOCKDEBUG
	bool dodebug;

	dodebug = LOCKDEBUG_ALLOC(cv, &cv_lockops,
	    (uintptr_t)__builtin_return_address(0));
	if (!dodebug) {
		/* XXX This will break vfs_lockf. */
		wmesg = nodebug;
	}
#endif
	KASSERT(wmesg != NULL);
	cv->cv_wmesg = wmesg;
	sleepq_init(CV_SLEEPQ(cv));
}

/*
 * cv_destroy:
 *
 *	Tear down a condition variable.
 */
void
cv_destroy(kcondvar_t *cv)
{

	LOCKDEBUG_FREE(CV_DEBUG_P(cv), cv);
#ifdef DIAGNOSTIC
	KASSERT(cv_is_valid(cv));
	cv->cv_wmesg = deadcv;
#endif
}

/*
 * cv_enter:
 *
 *	Look up and lock the sleep queue corresponding to the given
 *	condition variable, and increment the number of waiters.
 */
static inline void
cv_enter(kcondvar_t *cv, kmutex_t *mtx, lwp_t *l)
{
	sleepq_t *sq;
	kmutex_t *mp;

	KASSERT(cv_is_valid(cv));
	KASSERT((l->l_pflag & LP_INTR) == 0 || panicstr != NULL);

	LOCKDEBUG_LOCKED(CV_DEBUG_P(cv), cv, mtx, CV_RA, 0);

	l->l_kpriority = true;
	(void)sleeptab_lookup(&sleeptab, cv, &mp);
	sq = CV_SLEEPQ(cv);
	sleepq_enter(sq, l, mp);
	sleepq_enqueue(sq, cv, cv->cv_wmesg, &cv_syncobj);
	mutex_exit(mtx);
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
	if (__predict_false(error != 0))
		cv_signal(cv);

	LOCKDEBUG_UNLOCKED(CV_DEBUG_P(cv), cv, CV_RA, 0);
	KASSERT(cv_is_valid(cv));

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
static u_int
cv_unsleep(lwp_t *l, bool cleanup)
{
	kcondvar_t *cv;

	cv = (kcondvar_t *)(uintptr_t)l->l_wchan;

	KASSERT(l->l_wchan == (wchan_t)cv);
	KASSERT(l->l_sleepq == CV_SLEEPQ(cv));
	KASSERT(cv_is_valid(cv));
	KASSERT(!TAILQ_EMPTY(CV_SLEEPQ(cv)));

	return sleepq_unsleep(l, cleanup);
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

	KASSERT(mutex_owned(mtx));

	cv_enter(cv, mtx, l);
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
	int error;

	KASSERT(mutex_owned(mtx));

	cv_enter(cv, mtx, l);
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
	int error;

	KASSERT(mutex_owned(mtx));

	cv_enter(cv, mtx, l);
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
	int error;

	KASSERT(mutex_owned(mtx));

	cv_enter(cv, mtx, l);
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

	/* LOCKDEBUG_WAKEUP(CV_DEBUG_P(cv), cv, CV_RA); */
	KASSERT(cv_is_valid(cv));

	cv_wakeup_one(cv);
}

static void __noinline
cv_wakeup_one(kcondvar_t *cv)
{
	sleepq_t *sq;
	kmutex_t *mp;
	int swapin;
	lwp_t *l;

	KASSERT(cv_is_valid(cv));

	sq = CV_SLEEPQ(cv);
	(void)sleeptab_lookup(&sleeptab, cv, &mp);
	l = TAILQ_FIRST(sq);
	if (l == NULL) {
		mutex_spin_exit(mp);
		return;
	}
	KASSERT(l->l_sleepq == sq);
	KASSERT(l->l_mutex == mp);
	KASSERT(l->l_wchan == cv);
	swapin = sleepq_remove(sq, l);
	mutex_spin_exit(mp);

	/*
	 * If there are newly awakend threads that need to be swapped in,
	 * then kick the swapper into action.
	 */
	if (swapin)
		uvm_kick_scheduler();

	KASSERT(cv_is_valid(cv));
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

	/* LOCKDEBUG_WAKEUP(CV_DEBUG_P(cv), cv, CV_RA); */
	KASSERT(cv_is_valid(cv));

	cv_wakeup_all(cv);
}

static void __noinline
cv_wakeup_all(kcondvar_t *cv)
{
	sleepq_t *sq;
	kmutex_t *mp;
	int swapin;
	lwp_t *l, *next;

	KASSERT(cv_is_valid(cv));

	sq = CV_SLEEPQ(cv);
	(void)sleeptab_lookup(&sleeptab, cv, &mp);
	swapin = 0;
	for (l = TAILQ_FIRST(sq); l != NULL; l = next) {
		KASSERT(l->l_sleepq == sq);
		KASSERT(l->l_mutex == mp);
		KASSERT(l->l_wchan == cv);
		next = TAILQ_NEXT(l, l_sleepchain);
		swapin |= sleepq_remove(sq, l);
	}
	mutex_spin_exit(mp);

	/*
	 * If there are newly awakend threads that need to be swapped in,
	 * then kick the swapper into action.
	 */
	if (swapin)
		uvm_kick_scheduler();

	KASSERT(cv_is_valid(cv));
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

	cv_wakeup_all(cv);
	wakeup(cv);
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
	bool result;
	kmutex_t *mp;
	sleepq_t *sq;

	sq = CV_SLEEPQ(cv);
	(void)sleeptab_lookup(&sleeptab, cv, &mp);

	/* we can only get a valid result with the sleepq locked */
	result = !TAILQ_EMPTY(sq);

	mutex_spin_exit(mp);
	return result;
}

/*
 * cv_is_valid:
 *
 *	For diagnostic assertions: return non-zero if a condition
 *	variable appears to be valid.  No locks need be held.
 */
bool
cv_is_valid(kcondvar_t *cv)
{

	return cv->cv_wmesg != deadcv && cv->cv_wmesg != NULL;
}
