/*	$NetBSD: drm_wait_netbsd.h,v 1.4.4.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _DRM_DRM_WAIT_NETBSD_H_
#define _DRM_DRM_WAIT_NETBSD_H_

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/cpu.h>		/* cpu_intr_p */
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <linux/mutex.h>
#include <linux/spinlock.h>

typedef kcondvar_t drm_waitqueue_t;

#define	DRM_HZ	hz		/* XXX Hurk...  */

#define	DRM_UDELAY	DELAY

static inline void
DRM_INIT_WAITQUEUE(drm_waitqueue_t *q, const char *name)
{
	cv_init(q, name);
}

static inline void
DRM_DESTROY_WAITQUEUE(drm_waitqueue_t *q)
{
	cv_destroy(q);
}

static inline bool
DRM_WAITERS_P(drm_waitqueue_t *q, struct mutex *interlock)
{
	KASSERT(mutex_is_locked(interlock));
	return cv_has_waiters(q);
}

static inline void
DRM_WAKEUP_ONE(drm_waitqueue_t *q, struct mutex *interlock)
{
	KASSERT(mutex_is_locked(interlock));
	cv_signal(q);
}

static inline void
DRM_WAKEUP_ALL(drm_waitqueue_t *q, struct mutex *interlock)
{
	KASSERT(mutex_is_locked(interlock));
	cv_broadcast(q);
}

static inline bool
DRM_SPIN_WAITERS_P(drm_waitqueue_t *q, spinlock_t *interlock)
{
	KASSERT(spin_is_locked(interlock));
	return cv_has_waiters(q);
}

static inline void
DRM_SPIN_WAKEUP_ONE(drm_waitqueue_t *q, spinlock_t *interlock)
{
	KASSERT(spin_is_locked(interlock));
	cv_signal(q);
}

static inline void
DRM_SPIN_WAKEUP_ALL(drm_waitqueue_t *q, spinlock_t *interlock)
{
	KASSERT(spin_is_locked(interlock));
	cv_broadcast(q);
}

/*
 * DRM_SPIN_WAIT_ON is a replacement for the legacy DRM_WAIT_ON
 * portability macro.  It requires a spin interlock, which may require
 * changes to the surrounding code so that the waits actually are
 * interlocked by a spin lock.  It also polls the condition at every
 * tick, which masks missing wakeups.  Since DRM_WAIT_ON is going away,
 * in favour of Linux's native wait_event* API, waits in new code
 * should be written to use the DRM_*WAIT*_UNTIL macros below.
 *
 * Like the legacy DRM_WAIT_ON, DRM_SPIN_WAIT_ON returns
 *
 * . -EBUSY if timed out (yes, -EBUSY, not -ETIMEDOUT or -EWOULDBLOCK),
 * . -EINTR/-ERESTART if interrupted by a signal, or
 * . 0 if the condition was true before or just after the timeout.
 *
 * Note that cv_timedwait* return -EWOULDBLOCK, not -EBUSY, on timeout.
 */

#define	DRM_SPIN_WAIT_ON(RET, Q, INTERLOCK, TICKS, CONDITION)	do	      \
{									      \
	unsigned _dswo_ticks = (TICKS);					      \
	unsigned _dswo_start, _dswo_end;				      \
									      \
	KASSERT(spin_is_locked((INTERLOCK)));				      \
	KASSERT(!cpu_intr_p());						      \
	KASSERT(!cpu_softintr_p());					      \
	KASSERT(!cold);							      \
									      \
	for (;;) {							      \
		if (CONDITION) {					      \
			(RET) = 0;					      \
			break;						      \
		}							      \
		if (_dswo_ticks == 0) {					      \
			(RET) = -EBUSY;		/* Match Linux...  */	      \
			break;						      \
		}							      \
		_dswo_start = hardclock_ticks;				      \
		/* XXX errno NetBSD->Linux */				      \
		(RET) = -cv_timedwait_sig((Q), &(INTERLOCK)->sl_lock, 1);     \
		_dswo_end = hardclock_ticks;				      \
		if (_dswo_end - _dswo_start < _dswo_ticks)		      \
			_dswo_ticks -= _dswo_end - _dswo_start;		      \
		else							      \
			_dswo_ticks = 0;				      \
		if (RET) {						      \
			if ((RET) == -EWOULDBLOCK)			      \
				/* Waited only one tick.  */		      \
				continue;				      \
			break;						      \
		}							      \
	}								      \
} while (0)

/*
 * The DRM_*WAIT*_UNTIL macros are replacements for the Linux
 * wait_event* macros.  Like DRM_SPIN_WAIT_ON, they add an interlock,
 * and so may require some changes to the surrounding code.  They have
 * a different return value convention from DRM_SPIN_WAIT_ON and a
 * different return value convention from cv_*wait*.
 *
 * The untimed DRM_*WAIT*_UNTIL macros return
 *
 * . -EINTR/-ERESTART if interrupted by a signal, or
 * . zero if the condition evaluated
 *
 * The timed DRM_*TIMED_WAIT*_UNTIL macros return
 *
 * . -EINTR/-ERESTART if interrupted by a signal,
 * . 0 if the condition was false after the timeout,
 * . 1 if the condition was true just after the timeout, or
 * . the number of ticks remaining if the condition was true before the
 * timeout.
 *
 * Contrast DRM_SPIN_WAIT_ON which returns -EINTR/-ERESTART on signal,
 * -EBUSY on timeout, and zero on success; and cv_*wait*, which return
 * -EINTR/-ERESTART on signal, -EWOULDBLOCK on timeout, and zero on
 * success.
 *
 * XXX In retrospect, giving the timed and untimed macros a different
 * return convention from one another to match Linux may have been a
 * bad idea.  All of this inconsistent timeout return convention logic
 * has been a consistent source of bugs.
 */

#define	_DRM_WAIT_UNTIL(RET, WAIT, Q, INTERLOCK, CONDITION) do		\
{									\
	KASSERT(mutex_is_locked((INTERLOCK)));				\
	ASSERT_SLEEPABLE();						\
	KASSERT(!cold);							\
	for (;;) {							\
		if (CONDITION) {					\
			(RET) = 0;					\
			break;						\
		}							\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -WAIT((Q), &(INTERLOCK)->mtx_lock);		\
		if (RET)						\
			break;						\
	}								\
} while (0)

#define	cv_wait_nointr(Q, I)	(cv_wait((Q), (I)), 0)

#define	DRM_WAIT_NOINTR_UNTIL(RET, Q, I, C)				\
	_DRM_WAIT_UNTIL(RET, cv_wait_nointr, Q, I, C)

#define	DRM_WAIT_UNTIL(RET, Q, I, C)				\
	_DRM_WAIT_UNTIL(RET, cv_wait_sig, Q, I, C)

#define	_DRM_TIMED_WAIT_UNTIL(RET, WAIT, Q, INTERLOCK, TICKS, CONDITION) do \
{									\
	unsigned _dtwu_ticks = (TICKS);					\
	unsigned _dtwu_start, _dtwu_end;				\
									\
	KASSERT(mutex_is_locked((INTERLOCK)));				\
	ASSERT_SLEEPABLE();						\
	KASSERT(!cold);							\
									\
	for (;;) {							\
		if (CONDITION) {					\
			(RET) = MAX(_dtwu_ticks, 1);			\
			break;						\
		}							\
		if (_dtwu_ticks == 0) {					\
			(RET) = 0;					\
			break;						\
		}							\
		_dtwu_start = hardclock_ticks;				\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -WAIT((Q), &(INTERLOCK)->mtx_lock,		\
		    MIN(_dtwu_ticks, INT_MAX/2));			\
		_dtwu_end = hardclock_ticks;				\
		if ((_dtwu_end - _dtwu_start) < _dtwu_ticks)		\
			_dtwu_ticks -= _dtwu_end - _dtwu_start;		\
		else							\
			_dtwu_ticks = 0;				\
		if (RET) {						\
			if ((RET) == -EWOULDBLOCK)			\
				(RET) = (CONDITION) ? 1 : 0;		\
			break;						\
		}							\
	}								\
} while (0)

#define	DRM_TIMED_WAIT_NOINTR_UNTIL(RET, Q, I, T, C)			\
	_DRM_TIMED_WAIT_UNTIL(RET, cv_timedwait, Q, I, T, C)

#define	DRM_TIMED_WAIT_UNTIL(RET, Q, I, T, C)			\
	_DRM_TIMED_WAIT_UNTIL(RET, cv_timedwait_sig, Q, I, T, C)

/*
 * XXX Can't assert sleepable here because we hold a spin lock.  At
 * least we can assert that we're not in (soft) interrupt context, and
 * hope that nobody tries to use these with a sometimes quickly
 * satisfied condition while holding a different spin lock.
 */

#define	_DRM_SPIN_WAIT_UNTIL(RET, WAIT, Q, INTERLOCK, CONDITION) do	\
{									\
	KASSERT(spin_is_locked((INTERLOCK)));				\
	KASSERT(!cpu_intr_p());						\
	KASSERT(!cpu_softintr_p());					\
	KASSERT(!cold);							\
	(RET) = 0;							\
	while (!(CONDITION)) {						\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -WAIT((Q), &(INTERLOCK)->sl_lock);		\
		if (RET)						\
			break;						\
	}								\
} while (0)

#define	DRM_SPIN_WAIT_NOINTR_UNTIL(RET, Q, I, C)			\
	_DRM_SPIN_WAIT_UNTIL(RET, cv_wait_nointr, Q, I, C)

#define	DRM_SPIN_WAIT_UNTIL(RET, Q, I, C)				\
	_DRM_SPIN_WAIT_UNTIL(RET, cv_wait_sig, Q, I, C)

#define	_DRM_SPIN_TIMED_WAIT_UNTIL(RET, WAIT, Q, INTERLOCK, TICKS, CONDITION) \
	do								\
{									\
	unsigned _dstwu_ticks = (TICKS);				\
	unsigned _dstwu_start, _dstwu_end;				\
									\
	KASSERT(spin_is_locked((INTERLOCK)));				\
	KASSERT(!cpu_intr_p());						\
	KASSERT(!cpu_softintr_p());					\
	KASSERT(!cold);							\
									\
	for (;;) {							\
		if (CONDITION) {					\
			(RET) = MAX(_dstwu_ticks, 1);			\
			break;						\
		}							\
		if (_dstwu_ticks == 0) {				\
			(RET) = 0;					\
			break;						\
		}							\
		_dstwu_start = hardclock_ticks;				\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -WAIT((Q), &(INTERLOCK)->sl_lock,		\
		    MIN(_dstwu_ticks, INT_MAX/2));			\
		_dstwu_end = hardclock_ticks;				\
		if ((_dstwu_end - _dstwu_start) < _dstwu_ticks)		\
			_dstwu_ticks -= _dstwu_end - _dstwu_start;	\
		else							\
			_dstwu_ticks = 0;				\
		if (RET) {						\
			if ((RET) == -EWOULDBLOCK)			\
				(RET) = (CONDITION) ? 1 : 0;		\
			break;						\
		}							\
	}								\
} while (0)

#define	DRM_SPIN_TIMED_WAIT_NOINTR_UNTIL(RET, Q, I, T, C)		\
	_DRM_SPIN_TIMED_WAIT_UNTIL(RET, cv_timedwait, Q, I, T, C)

#define	DRM_SPIN_TIMED_WAIT_UNTIL(RET, Q, I, T, C)			\
	_DRM_SPIN_TIMED_WAIT_UNTIL(RET, cv_timedwait_sig, Q, I, T, C)

#endif  /* _DRM_DRM_WAIT_NETBSD_H_ */
