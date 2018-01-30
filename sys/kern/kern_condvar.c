/*	$NetBSD: kern_condvar.c,v 1.41 2018/01/30 07:52:22 ozaki-r Exp $	*/

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
 * Kernel condition variable implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_condvar.c,v 1.41 2018/01/30 07:52:22 ozaki-r Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/condvar.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>
#include <sys/kernel.h>

/*
 * Accessors for the private contents of the kcondvar_t data type.
 *
 *	cv_opaque[0]	sleepq...
 *	cv_opaque[1]	...pointers
 *	cv_opaque[2]	description for ps(1)
 *
 * cv_opaque[0..1] is protected by the interlock passed to cv_wait() (enqueue
 * only), and the sleep queue lock acquired with sleeptab_lookup() (enqueue
 * and dequeue).
 *
 * cv_opaque[2] (the wmesg) is static and does not change throughout the life
 * of the CV.
 */
#define	CV_SLEEPQ(cv)		((sleepq_t *)(cv)->cv_opaque)
#define	CV_WMESG(cv)		((const char *)(cv)->cv_opaque[2])
#define	CV_SET_WMESG(cv, v) 	(cv)->cv_opaque[2] = __UNCONST(v)

#define	CV_DEBUG_P(cv)	(CV_WMESG(cv) != nodebug)
#define	CV_RA		((uintptr_t)__builtin_return_address(0))

static void		cv_unsleep(lwp_t *, bool);
static inline void	cv_wakeup_one(kcondvar_t *);
static inline void	cv_wakeup_all(kcondvar_t *);

static syncobj_t cv_syncobj = {
	.sobj_flag	= SOBJ_SLEEPQ_SORTED,
	.sobj_unsleep	= cv_unsleep,
	.sobj_changepri	= sleepq_changepri,
	.sobj_lendpri	= sleepq_lendpri,
	.sobj_owner	= syncobj_noowner,
};

lockops_t cv_lockops = {
	.lo_name = "Condition variable",
	.lo_type = LOCKOPS_CV,
	.lo_dump = NULL,
};

static const char deadcv[] = "deadcv";
#ifdef LOCKDEBUG
static const char nodebug[] = "nodebug";

#define CV_LOCKDEBUG_HANDOFF(l, cv) cv_lockdebug_handoff(l, cv)
#define CV_LOCKDEBUG_PROCESS(l, cv) cv_lockdebug_process(l, cv)

static inline void
cv_lockdebug_handoff(lwp_t *l, kcondvar_t *cv)
{

	if (CV_DEBUG_P(cv))
		l->l_flag |= LW_CVLOCKDEBUG;
}

static inline void
cv_lockdebug_process(lwp_t *l, kcondvar_t *cv)
{

	if ((l->l_flag & LW_CVLOCKDEBUG) == 0)
		return;

	l->l_flag &= ~LW_CVLOCKDEBUG;
	LOCKDEBUG_UNLOCKED(true, cv, CV_RA, 0);
}
#else
#define CV_LOCKDEBUG_HANDOFF(l, cv) __nothing
#define CV_LOCKDEBUG_PROCESS(l, cv) __nothing
#endif

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
	CV_SET_WMESG(cv, wmesg);
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
	CV_SET_WMESG(cv, deadcv);
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
	KASSERT(!cpu_intr_p());
	KASSERT((l->l_pflag & LP_INTR) == 0 || panicstr != NULL);

	LOCKDEBUG_LOCKED(CV_DEBUG_P(cv), cv, mtx, CV_RA, 0);

	l->l_kpriority = true;
	mp = sleepq_hashlock(cv);
	sq = CV_SLEEPQ(cv);
	sleepq_enter(sq, l, mp);
	sleepq_enqueue(sq, cv, CV_WMESG(cv), &cv_syncobj);
	mutex_exit(mtx);
	KASSERT(cv_has_waiters(cv));
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
static void
cv_unsleep(lwp_t *l, bool cleanup)
{
	kcondvar_t *cv __diagused;

	cv = (kcondvar_t *)(uintptr_t)l->l_wchan;

	KASSERT(l->l_wchan == (wchan_t)cv);
	KASSERT(l->l_sleepq == CV_SLEEPQ(cv));
	KASSERT(cv_is_valid(cv));
	KASSERT(cv_has_waiters(cv));

	sleepq_unsleep(l, cleanup);
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

	/*
	 * We can't use cv_exit() here since the cv might be destroyed before
	 * this thread gets a chance to run.  Instead, hand off the lockdebug
	 * responsibility to the thread that wakes us up.
	 */

	CV_LOCKDEBUG_HANDOFF(l, cv);
	(void)sleepq_block(0, false);
	mutex_enter(mtx);
}

/*
 * cv_wait_sig:
 *
 *	Wait on a condition variable until a awoken or a signal is received. 
 *	Will also return early if the process is exiting.  Returns zero if
 *	awoken normally, ERESTART if a signal was received and the system
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
 *
 *	timo is a timeout in ticks.  timo = 0 specifies an infinite timeout.
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
 *	exiting.  Returns zero if awoken normally, EWOULDBLOCK if the
 *	timeout expires, ERESTART if a signal was received and the system
 *	call is restartable, or EINTR otherwise.
 *
 *	timo is a timeout in ticks.  timo = 0 specifies an infinite timeout.
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
 * Given a number of seconds, sec, and 2^64ths of a second, frac, we
 * want a number of ticks for a timeout:
 *
 *	timo = hz*(sec + frac/2^64)
 *	     = hz*sec + hz*frac/2^64
 *	     = hz*sec + hz*(frachi*2^32 + fraclo)/2^64
 *	     = hz*sec + hz*frachi/2^32 + hz*fraclo/2^64,
 *
 * where frachi is the high 32 bits of frac and fraclo is the
 * low 32 bits.
 *
 * We assume hz < INT_MAX/2 < UINT32_MAX, so
 *
 *	hz*fraclo/2^64 < fraclo*2^32/2^64 <= 1,
 *
 * since fraclo < 2^32.
 *
 * We clamp the result at INT_MAX/2 for a timeout in ticks, since we
 * can't represent timeouts higher than INT_MAX in cv_timedwait, and
 * spurious wakeup is OK.  Moreover, we don't want to wrap around,
 * because we compute end - start in ticks in order to compute the
 * remaining timeout, and that difference cannot wrap around, so we use
 * a timeout less than INT_MAX.  Using INT_MAX/2 provides plenty of
 * margin for paranoia and will exceed most waits in practice by far.
 */
static unsigned
bintime2timo(const struct bintime *bt)
{

	KASSERT(hz < INT_MAX/2);
	CTASSERT(INT_MAX/2 < UINT32_MAX);
	if (bt->sec > ((INT_MAX/2)/hz))
		return INT_MAX/2;
	if ((hz*(bt->frac >> 32) >> 32) > (INT_MAX/2 - hz*bt->sec))
		return INT_MAX/2;

	return hz*bt->sec + (hz*(bt->frac >> 32) >> 32);
}

/*
 * timo is in units of ticks.  We want units of seconds and 2^64ths of
 * a second.  We know hz = 1 sec/tick, and 2^64 = 1 sec/(2^64th of a
 * second), from which we can conclude 2^64 / hz = 1 (2^64th of a
 * second)/tick.  So for the fractional part, we compute
 *
 *	frac = rem * 2^64 / hz
 *	     = ((rem * 2^32) / hz) * 2^32
 *
 * Using truncating integer division instead of real division will
 * leave us with only about 32 bits of precision, which means about
 * 1/4-nanosecond resolution, which is good enough for our purposes.
 */
static struct bintime
timo2bintime(unsigned timo)
{

	return (struct bintime) {
		.sec = timo / hz,
		.frac = (((uint64_t)(timo % hz) << 32)/hz << 32),
	};
}

/*
 * cv_timedwaitbt:
 *
 *	Wait on a condition variable until awoken or the specified
 *	timeout expires.  Returns zero if awoken normally or
 *	EWOULDBLOCK if the timeout expires.
 *
 *	On entry, bt is a timeout in bintime.  cv_timedwaitbt subtracts
 *	the time slept, so on exit, bt is the time remaining after
 *	sleeping, possibly negative if the complete time has elapsed.
 *	No infinite timeout; use cv_wait_sig instead.
 *
 *	epsilon is a requested maximum error in timeout (excluding
 *	spurious wakeups).  Currently not used, will be used in the
 *	future to choose between low- and high-resolution timers.
 *	Actual wakeup time will be somewhere in [t, t + max(e, r) + s)
 *	where r is the finest resolution of clock available and s is
 *	scheduling delays for scheduler overhead and competing threads.
 *	Time is measured by the interrupt source implementing the
 *	timeout, not by another timecounter.
 */
int
cv_timedwaitbt(kcondvar_t *cv, kmutex_t *mtx, struct bintime *bt,
    const struct bintime *epsilon __diagused)
{
	struct bintime slept;
	unsigned start, end;
	int error;

	KASSERTMSG(bt->sec >= 0, "negative timeout");
	KASSERTMSG(epsilon != NULL, "specify maximum requested delay");

	/*
	 * hardclock_ticks is technically int, but nothing special
	 * happens instead of overflow, so we assume two's-complement
	 * wraparound and just treat it as unsigned.
	 */
	start = hardclock_ticks;
	error = cv_timedwait(cv, mtx, bintime2timo(bt));
	end = hardclock_ticks;

	slept = timo2bintime(end - start);
	/* bt := bt - slept */
	bintime_sub(bt, &slept);

	return error;
}

/*
 * cv_timedwaitbt_sig:
 *
 *	Wait on a condition variable until awoken, the specified
 *	timeout expires, or interrupted by a signal.  Returns zero if
 *	awoken normally, EWOULDBLOCK if the timeout expires, or
 *	EINTR/ERESTART if interrupted by a signal.
 *
 *	On entry, bt is a timeout in bintime.  cv_timedwaitbt_sig
 *	subtracts the time slept, so on exit, bt is the time remaining
 *	after sleeping.  No infinite timeout; use cv_wait instead.
 *
 *	epsilon is a requested maximum error in timeout (excluding
 *	spurious wakeups).  Currently not used, will be used in the
 *	future to choose between low- and high-resolution timers.
 */
int
cv_timedwaitbt_sig(kcondvar_t *cv, kmutex_t *mtx, struct bintime *bt,
    const struct bintime *epsilon __diagused)
{
	struct bintime slept;
	unsigned start, end;
	int error;

	KASSERTMSG(bt->sec >= 0, "negative timeout");
	KASSERTMSG(epsilon != NULL, "specify maximum requested delay");

	/*
	 * hardclock_ticks is technically int, but nothing special
	 * happens instead of overflow, so we assume two's-complement
	 * wraparound and just treat it as unsigned.
	 */
	start = hardclock_ticks;
	error = cv_timedwait_sig(cv, mtx, bintime2timo(bt));
	end = hardclock_ticks;

	slept = timo2bintime(end - start);
	/* bt := bt - slept */
	bintime_sub(bt, &slept);

	return error;
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

	if (__predict_false(!TAILQ_EMPTY(CV_SLEEPQ(cv))))
		cv_wakeup_one(cv);
}

static inline void
cv_wakeup_one(kcondvar_t *cv)
{
	sleepq_t *sq;
	kmutex_t *mp;
	lwp_t *l;

	KASSERT(cv_is_valid(cv));

	mp = sleepq_hashlock(cv);
	sq = CV_SLEEPQ(cv);
	l = TAILQ_FIRST(sq);
	if (l == NULL) {
		mutex_spin_exit(mp);
		return;
	}
	KASSERT(l->l_sleepq == sq);
	KASSERT(l->l_mutex == mp);
	KASSERT(l->l_wchan == cv);
	CV_LOCKDEBUG_PROCESS(l, cv);
	sleepq_remove(sq, l);
	mutex_spin_exit(mp);

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

	if (__predict_false(!TAILQ_EMPTY(CV_SLEEPQ(cv))))  
		cv_wakeup_all(cv);
}

static inline void
cv_wakeup_all(kcondvar_t *cv)
{
	sleepq_t *sq;
	kmutex_t *mp;
	lwp_t *l, *next;

	KASSERT(cv_is_valid(cv));

	mp = sleepq_hashlock(cv);
	sq = CV_SLEEPQ(cv);
	for (l = TAILQ_FIRST(sq); l != NULL; l = next) {
		KASSERT(l->l_sleepq == sq);
		KASSERT(l->l_mutex == mp);
		KASSERT(l->l_wchan == cv);
		next = TAILQ_NEXT(l, l_sleepchain);
		CV_LOCKDEBUG_PROCESS(l, cv);
		sleepq_remove(sq, l);
	}
	mutex_spin_exit(mp);

	KASSERT(cv_is_valid(cv));
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

	return !TAILQ_EMPTY(CV_SLEEPQ(cv));
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

	return CV_WMESG(cv) != deadcv && CV_WMESG(cv) != NULL;
}
