/*	$NetBSD: pthread_cond.c,v 1.57.2.3 2014/08/20 00:02:20 tls Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Andrew Doran.
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
 * We assume that there will be no contention on pthread_cond_t::ptc_lock
 * because functioning applications must call both the wait and wakeup
 * functions while holding the same application provided mutex.  The
 * spinlock is present only to prevent libpthread causing the application
 * to crash or malfunction as a result of corrupted data structures, in
 * the event that the application is buggy.
 *
 * If there is contention on spinlock when real-time threads are in use,
 * it could cause a deadlock due to priority inversion: the thread holding
 * the spinlock may not get CPU time to make forward progress and release
 * the spinlock to a higher priority thread that is waiting for it.
 * Contention on the spinlock will only occur with buggy applications,
 * so at the time of writing it's not considered a major bug in libpthread.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_cond.c,v 1.57.2.3 2014/08/20 00:02:20 tls Exp $");

#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"

int	_sys___nanosleep50(const struct timespec *, struct timespec *);

extern int pthread__started;

static int pthread_cond_wait_nothread(pthread_t, pthread_mutex_t *,
    pthread_cond_t *, const struct timespec *);

int	_pthread_cond_has_waiters_np(pthread_cond_t *);

__weak_alias(pthread_cond_has_waiters_np,_pthread_cond_has_waiters_np)

__strong_alias(__libc_cond_init,pthread_cond_init)
__strong_alias(__libc_cond_signal,pthread_cond_signal)
__strong_alias(__libc_cond_broadcast,pthread_cond_broadcast)
__strong_alias(__libc_cond_wait,pthread_cond_wait)
__strong_alias(__libc_cond_timedwait,pthread_cond_timedwait)
__strong_alias(__libc_cond_destroy,pthread_cond_destroy)

static clockid_t
pthread_cond_getclock(const pthread_cond_t *cond)
{
	return cond->ptc_private ? 
	    *(clockid_t *)cond->ptc_private : CLOCK_REALTIME;
}

int
pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	if (__predict_false(__uselibcstub))
		return __libc_cond_init_stub(cond, attr);

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    (attr == NULL) || (attr->ptca_magic == _PT_CONDATTR_MAGIC));

	cond->ptc_magic = _PT_COND_MAGIC;
	pthread_lockinit(&cond->ptc_lock);
	PTQ_INIT(&cond->ptc_waiters);
	cond->ptc_mutex = NULL;
	if (attr && attr->ptca_private) {
		cond->ptc_private = malloc(sizeof(clockid_t));
		if (cond->ptc_private == NULL)
			return errno;
		*(clockid_t *)cond->ptc_private =
		    *(clockid_t *)attr->ptca_private;
	} else
		cond->ptc_private = NULL;

	return 0;
}


int
pthread_cond_destroy(pthread_cond_t *cond)
{
	if (__predict_false(__uselibcstub))
		return __libc_cond_destroy_stub(cond);

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);
	pthread__error(EBUSY, "Destroying condition variable in use",
	    cond->ptc_mutex == NULL);

	cond->ptc_magic = _PT_COND_DEAD;
	free(cond->ptc_private);

	return 0;
}

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
		       const struct timespec *abstime)
{
	pthread_t self;
	int retval;
	clockid_t clkid = pthread_cond_getclock(cond);

	if (__predict_false(__uselibcstub))
		return __libc_cond_timedwait_stub(cond, mutex, abstime);

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);
	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);
	pthread__error(EPERM, "Mutex not locked in condition wait",
	    mutex->ptm_owner != NULL);

	self = pthread__self();

	/* Just hang out for a while if threads aren't running yet. */
	if (__predict_false(pthread__started == 0)) {
		return pthread_cond_wait_nothread(self, mutex, cond, abstime);
	}
	if (__predict_false(self->pt_cancel)) {
		pthread__cancelled();
	}

	/* Note this thread as waiting on the CV. */
	pthread__spinlock(self, &cond->ptc_lock);
	cond->ptc_mutex = mutex;
	PTQ_INSERT_HEAD(&cond->ptc_waiters, self, pt_sleep);
	self->pt_sleepobj = cond;
	pthread__spinunlock(self, &cond->ptc_lock);

	do {
		self->pt_willpark = 1;
		pthread_mutex_unlock(mutex);
		self->pt_willpark = 0;
		self->pt_blocking++;
		do {
			retval = _lwp_park(clkid, TIMER_ABSTIME, abstime,
			    self->pt_unpark, __UNVOLATILE(&mutex->ptm_waiters),
			    __UNVOLATILE(&mutex->ptm_waiters));
			self->pt_unpark = 0;
		} while (retval == -1 && errno == ESRCH);
		self->pt_blocking--;
		membar_sync();
		pthread_mutex_lock(mutex);

		/*
		 * If we have cancelled then exit.  POSIX dictates that
		 * the mutex must be held when we action the cancellation.
		 *
		 * If we absorbed a pthread_cond_signal() and cannot take
		 * the wakeup, we must ensure that another thread does.
		 *
		 * If awoke early, we may still be on the sleep queue and
		 * must remove ourself.
		 */
		if (__predict_false(retval != 0)) {
			switch (errno) {
			case EINTR:
			case EALREADY:
				retval = 0;
				break;
			default:
				retval = errno;
				break;
			}
		}
		if (__predict_false(self->pt_cancel | retval)) {
			pthread_cond_signal(cond);
			if (self->pt_cancel) {
				pthread__cancelled();
			}
			break;
		}
	} while (self->pt_sleepobj != NULL);

	return retval;
}

int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	if (__predict_false(__uselibcstub))
		return __libc_cond_wait_stub(cond, mutex);

	return pthread_cond_timedwait(cond, mutex, NULL);
}

static int __noinline
pthread__cond_wake_one(pthread_cond_t *cond)
{
	pthread_t self, signaled;
	pthread_mutex_t *mutex;
	lwpid_t lid;

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);

	/*
	 * Pull the first thread off the queue.  If the current thread
	 * is associated with the condition variable, remove it without
	 * awakening (error case in pthread_cond_timedwait()).
	 */
	self = pthread__self();
	pthread__spinlock(self, &cond->ptc_lock);
	if (self->pt_sleepobj == cond) {
		PTQ_REMOVE(&cond->ptc_waiters, self, pt_sleep);
		self->pt_sleepobj = NULL;
	}
	signaled = PTQ_FIRST(&cond->ptc_waiters);
	if (__predict_false(signaled == NULL)) {
		cond->ptc_mutex = NULL;
		pthread__spinunlock(self, &cond->ptc_lock);
		return 0;
	}
	mutex = cond->ptc_mutex;
	if (PTQ_NEXT(signaled, pt_sleep) == NULL) {
		cond->ptc_mutex = NULL;
		PTQ_INIT(&cond->ptc_waiters);
	} else {
		PTQ_REMOVE(&cond->ptc_waiters, signaled, pt_sleep);
	}
	signaled->pt_sleepobj = NULL;
	lid = signaled->pt_lid;
	pthread__spinunlock(self, &cond->ptc_lock);

	/*
	 * For all valid uses of pthread_cond_signal(), the caller will
	 * hold the mutex that the target is using to synchronize with.
	 * To avoid the target awakening and immediately blocking on the
	 * mutex, transfer the thread to be awoken to the current thread's
	 * deferred wakeup list.  The waiter will be set running when the
	 * caller (this thread) releases the mutex.
	 */
	if (__predict_false(self->pt_nwaiters == (size_t)pthread__unpark_max)) {
		(void)_lwp_unpark_all(self->pt_waiters, self->pt_nwaiters,
		    __UNVOLATILE(&mutex->ptm_waiters));
		self->pt_nwaiters = 0;
	}
	self->pt_waiters[self->pt_nwaiters++] = lid;
	pthread__mutex_deferwake(self, mutex);
	return 0;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{

	if (__predict_false(__uselibcstub))
		return __libc_cond_signal_stub(cond);

	if (__predict_true(PTQ_EMPTY(&cond->ptc_waiters)))
		return 0;
	return pthread__cond_wake_one(cond);
}

static int __noinline
pthread__cond_wake_all(pthread_cond_t *cond)
{
	pthread_t self, signaled;
	pthread_mutex_t *mutex;
	u_int max;
	size_t nwaiters;

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);

	/*
	 * Try to defer waking threads (see pthread_cond_signal()).
	 * Only transfer waiters for which there is no pending wakeup.
	 */
	self = pthread__self();
	pthread__spinlock(self, &cond->ptc_lock);
	max = pthread__unpark_max;
	mutex = cond->ptc_mutex;
	nwaiters = self->pt_nwaiters;
	PTQ_FOREACH(signaled, &cond->ptc_waiters, pt_sleep) {
		if (__predict_false(nwaiters == max)) {
			/* Overflow. */
			(void)_lwp_unpark_all(self->pt_waiters,
			    nwaiters, __UNVOLATILE(&mutex->ptm_waiters));
			nwaiters = 0;
		}
		signaled->pt_sleepobj = NULL;
		self->pt_waiters[nwaiters++] = signaled->pt_lid;
	}
	PTQ_INIT(&cond->ptc_waiters);
	self->pt_nwaiters = nwaiters;
	cond->ptc_mutex = NULL;
	pthread__spinunlock(self, &cond->ptc_lock);
	pthread__mutex_deferwake(self, mutex);

	return 0;
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	if (__predict_false(__uselibcstub))
		return __libc_cond_broadcast_stub(cond);

	if (__predict_true(PTQ_EMPTY(&cond->ptc_waiters)))
		return 0;
	return pthread__cond_wake_all(cond);
}

int
_pthread_cond_has_waiters_np(pthread_cond_t *cond)
{

	return !PTQ_EMPTY(&cond->ptc_waiters);
}

int
pthread_condattr_init(pthread_condattr_t *attr)
{

	attr->ptca_magic = _PT_CONDATTR_MAGIC;
	attr->ptca_private = NULL;

	return 0;
}

int
pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clck)
{
	switch (clck) {
	case CLOCK_MONOTONIC:
	case CLOCK_REALTIME:
		if (attr->ptca_private == NULL)
			attr->ptca_private = malloc(sizeof(clockid_t));
		if (attr->ptca_private == NULL)
			return errno;
		*(clockid_t *)attr->ptca_private = clck;
		return 0;
	default:
		return EINVAL;
	}
}

int
pthread_condattr_destroy(pthread_condattr_t *attr)
{

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    attr->ptca_magic == _PT_CONDATTR_MAGIC);

	attr->ptca_magic = _PT_CONDATTR_DEAD;
	free(attr->ptca_private);

	return 0;
}

/* Utility routine to hang out for a while if threads haven't started yet. */
static int
pthread_cond_wait_nothread(pthread_t self, pthread_mutex_t *mutex,
    pthread_cond_t *cond, const struct timespec *abstime)
{
	struct timespec now, diff;
	int retval;

	if (abstime == NULL) {
		diff.tv_sec = 99999999;
		diff.tv_nsec = 0;
	} else {
		clockid_t clck = pthread_cond_getclock(cond);
		clock_gettime(clck, &now);
		if  (timespeccmp(abstime, &now, <))
			timespecclear(&diff);
		else
			timespecsub(abstime, &now, &diff);
	}

	do {
		pthread__testcancel(self);
		pthread_mutex_unlock(mutex);
		retval = _sys___nanosleep50(&diff, NULL);
		pthread_mutex_lock(mutex);
	} while (abstime == NULL && retval == 0);
	pthread__testcancel(self);

	if (retval == 0)
		return ETIMEDOUT;
	else
		/* spurious wakeup */
		return 0;
}
