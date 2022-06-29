/*	$NetBSD: kern_condvar.c,v 1.54 2022/06/29 22:27:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008, 2019, 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_condvar.c,v 1.54 2022/06/29 22:27:01 riastradh Exp $");

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
 *	cv_opaque[0]	sleepq_t
 *	cv_opaque[1]	description for ps(1)
 *
 * cv_opaque[0] is protected by the interlock passed to cv_wait() (enqueue
 * only), and the sleep queue lock acquired with sleepq_hashlock() (enqueue
 * and dequeue).
 *
 * cv_opaque[1] (the wmesg) is static and does not change throughout the life
 * of the CV.
 */
#define	CV_SLEEPQ(cv)		((sleepq_t *)(cv)->cv_opaque)
#define	CV_WMESG(cv)		((const char *)(cv)->cv_opaque[1])
#define	CV_SET_WMESG(cv, v) 	(cv)->cv_opaque[1] = __UNCONST(v)

#define	CV_DEBUG_P(cv)	(CV_WMESG(cv) != nodebug)
#define	CV_RA		((uintptr_t)__builtin_return_address(0))

static void		cv_unsleep(lwp_t *, bool);
static inline void	cv_wakeup_one(kcondvar_t *);
static inline void	cv_wakeup_all(kcondvar_t *);

syncobj_t cv_syncobj = {
	.sobj_flag	= SOBJ_SLEEPQ_SORTED,
	.sobj_unsleep	= cv_unsleep,
	.sobj_changepri	= sleepq_changepri,
	.sobj_lendpri	= sleepq_lendpri,
	.sobj_owner	= syncobj_noowner,
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

	sleepq_destroy(CV_SLEEPQ(cv));
#ifdef DIAGNOSTIC
	KASSERT(cv_is_valid(cv));
	KASSERT(!cv_has_waiters(cv));
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
cv_enter(kcondvar_t *cv, kmutex_t *mtx, lwp_t *l, bool catch_p)
{
	sleepq_t *sq;
	kmutex_t *mp;

	KASSERT(cv_is_valid(cv));
	KASSERT(!cpu_intr_p());
	KASSERT((l->l_pflag & LP_INTR) == 0 || panicstr != NULL);

	l->l_kpriority = true;
	mp = sleepq_hashlock(cv);
	sq = CV_SLEEPQ(cv);
	sleepq_enter(sq, l, mp);
	sleepq_enqueue(sq, cv, CV_WMESG(cv), &cv_syncobj, catch_p);
	mutex_exit(mtx);
	KASSERT(cv_has_waiters(cv));
}

/*
 * cv_unsleep:
 *
 *	Remove an LWP from the condition variable and sleep queue.  This
 *	is called when the LWP has not been awoken normally but instead
 *	interrupted: for example, when a signal is received.  Must be
 *	called with the LWP locked.  Will unlock if "unlock" is true.
 */
static void
cv_unsleep(lwp_t *l, bool unlock)
{
	kcondvar_t *cv __diagused;

	cv = (kcondvar_t *)(uintptr_t)l->l_wchan;

	KASSERT(l->l_wchan == (wchan_t)cv);
	KASSERT(l->l_sleepq == CV_SLEEPQ(cv));
	KASSERT(cv_is_valid(cv));
	KASSERT(cv_has_waiters(cv));

	sleepq_unsleep(l, unlock);
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

	cv_enter(cv, mtx, l, false);
	(void)sleepq_block(0, false, &cv_syncobj);
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

	cv_enter(cv, mtx, l, true);
	error = sleepq_block(0, true, &cv_syncobj);
	mutex_enter(mtx);
	return error;
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

	cv_enter(cv, mtx, l, false);
	error = sleepq_block(timo, false, &cv_syncobj);
	mutex_enter(mtx);
	return error;
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

	cv_enter(cv, mtx, l, true);
	error = sleepq_block(timo, true, &cv_syncobj);
	mutex_enter(mtx);
	return error;
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
	int timo;
	int error;

	KASSERTMSG(bt->sec >= 0, "negative timeout");
	KASSERTMSG(epsilon != NULL, "specify maximum requested delay");

	/* If there's nothing left to wait, time out.  */
	if (bt->sec == 0 && bt->frac == 0)
		return EWOULDBLOCK;

	/* Convert to ticks, but clamp to be >=1.  */
	timo = bintime2timo(bt);
	KASSERTMSG(timo >= 0, "negative ticks: %d", timo);
	if (timo == 0)
		timo = 1;

	/*
	 * getticks() is technically int, but nothing special
	 * happens instead of overflow, so we assume two's-complement
	 * wraparound and just treat it as unsigned.
	 */
	start = getticks();
	error = cv_timedwait(cv, mtx, timo);
	end = getticks();

	/*
	 * Set it to the time left, or zero, whichever is larger.  We
	 * do not fail with EWOULDBLOCK here because this may have been
	 * an explicit wakeup, so the caller needs to check before they
	 * give up or else cv_signal would be lost.
	 */
	slept = timo2bintime(end - start);
	if (bintimecmp(bt, &slept, <=)) {
		bt->sec = 0;
		bt->frac = 0;
	} else {
		/* bt := bt - slept */
		bintime_sub(bt, &slept);
	}

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
	int timo;
	int error;

	KASSERTMSG(bt->sec >= 0, "negative timeout");
	KASSERTMSG(epsilon != NULL, "specify maximum requested delay");

	/* If there's nothing left to wait, time out.  */
	if (bt->sec == 0 && bt->frac == 0)
		return EWOULDBLOCK;

	/* Convert to ticks, but clamp to be >=1.  */
	timo = bintime2timo(bt);
	KASSERTMSG(timo >= 0, "negative ticks: %d", timo);
	if (timo == 0)
		timo = 1;

	/*
	 * getticks() is technically int, but nothing special
	 * happens instead of overflow, so we assume two's-complement
	 * wraparound and just treat it as unsigned.
	 */
	start = getticks();
	error = cv_timedwait_sig(cv, mtx, timo);
	end = getticks();

	/*
	 * Set it to the time left, or zero, whichever is larger.  We
	 * do not fail with EWOULDBLOCK here because this may have been
	 * an explicit wakeup, so the caller needs to check before they
	 * give up or else cv_signal would be lost.
	 */
	slept = timo2bintime(end - start);
	if (bintimecmp(bt, &slept, <=)) {
		bt->sec = 0;
		bt->frac = 0;
	} else {
		/* bt := bt - slept */
		bintime_sub(bt, &slept);
	}

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

	KASSERT(cv_is_valid(cv));

	if (__predict_false(!LIST_EMPTY(CV_SLEEPQ(cv))))
		cv_wakeup_one(cv);
}

/*
 * cv_wakeup_one:
 *
 *	Slow path for cv_signal().  Deliberately marked __noinline to
 *	prevent the compiler pulling it in to cv_signal(), which adds
 *	extra prologue and epilogue code.
 */
static __noinline void
cv_wakeup_one(kcondvar_t *cv)
{
	sleepq_t *sq;
	kmutex_t *mp;
	lwp_t *l;

	/*
	 * Keep waking LWPs until a non-interruptable waiter is found.  An
	 * interruptable waiter could fail to do something useful with the
	 * wakeup due to an error return from cv_[timed]wait_sig(), and the
	 * caller of cv_signal() may not expect such a scenario.
	 *
	 * This isn't a problem for non-interruptable waits (untimed and
	 * timed), because if such a waiter is woken here it will not return
	 * an error.
	 */
	mp = sleepq_hashlock(cv);
	sq = CV_SLEEPQ(cv);
	while ((l = LIST_FIRST(sq)) != NULL) {
		KASSERT(l->l_sleepq == sq);
		KASSERT(l->l_mutex == mp);
		KASSERT(l->l_wchan == cv);
		if ((l->l_flag & LW_SINTR) == 0) {
			sleepq_remove(sq, l);
			break;
		} else
			sleepq_remove(sq, l);
	}
	mutex_spin_exit(mp);
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

	KASSERT(cv_is_valid(cv));

	if (__predict_false(!LIST_EMPTY(CV_SLEEPQ(cv))))  
		cv_wakeup_all(cv);
}

/*
 * cv_wakeup_all:
 *
 *	Slow path for cv_broadcast().  Deliberately marked __noinline to
 *	prevent the compiler pulling it in to cv_broadcast(), which adds
 *	extra prologue and epilogue code.
 */
static __noinline void
cv_wakeup_all(kcondvar_t *cv)
{
	sleepq_t *sq;
	kmutex_t *mp;
	lwp_t *l;

	mp = sleepq_hashlock(cv);
	sq = CV_SLEEPQ(cv);
	while ((l = LIST_FIRST(sq)) != NULL) {
		KASSERT(l->l_sleepq == sq);
		KASSERT(l->l_mutex == mp);
		KASSERT(l->l_wchan == cv);
		sleepq_remove(sq, l);
	}
	mutex_spin_exit(mp);
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

	return !LIST_EMPTY(CV_SLEEPQ(cv));
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
