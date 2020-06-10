/*	$NetBSD: pthread_cond.c,v 1.74 2020/06/10 22:45:15 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007, 2008, 2020 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_cond.c,v 1.74 2020/06/10 22:45:15 ad Exp $");

#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"

int	_sys___nanosleep50(const struct timespec *, struct timespec *);

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

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);

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
	cond->ptc_waiters = NULL;
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
	    cond->ptc_waiters == NULL);

	cond->ptc_magic = _PT_COND_DEAD;
	free(cond->ptc_private);

	return 0;
}

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
		       const struct timespec *abstime)
{
	struct pthread__waiter waiter, *next, *waiters;
	pthread_t self;
	int error, cancel;
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
	pthread__assert(self->pt_lid != 0);

	if (__predict_false(self->pt_cancel)) {
		pthread__cancelled();
	}

	/* Note this thread as waiting on the CV. */
	cond->ptc_mutex = mutex;
	for (waiters = cond->ptc_waiters;; waiters = next) {
		waiter.lid = self->pt_lid;
		waiter.next = waiters;
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
		membar_producer();
#endif
		next = atomic_cas_ptr(&cond->ptc_waiters, waiters, &waiter);
		if (__predict_true(next == waiters)) {
			break;
		}
	}

	/* Drop the interlock */
	pthread_mutex_unlock(mutex);
	error = 0;

	while (waiter.lid && !(cancel = self->pt_cancel)) {
		int rv = _lwp_park(clkid, TIMER_ABSTIME, __UNCONST(abstime),
		    0, NULL, NULL);
		if (rv == 0) {
			continue;
		}
		if (errno != EINTR && errno != EALREADY && errno != ESRCH) {
			error = errno;
			break;
		}
	}

	/*
	 * If this thread absorbed a wakeup from pthread_cond_signal() and
	 * cannot take the wakeup, we should ensure that another thread does.
	 *
	 * And if awoken early, we may still be on the waiter list and must
	 * remove self.
	 *
	 * In all cases do the wakeup without the mutex held otherwise:
	 *
	 * - wakeup could be deferred until mutex release
	 * - it would be mixing up two sets of waitpoints
	 */
	if (__predict_false(cancel | error)) {
		pthread_cond_broadcast(cond);

		/*
		 * Might have raced with another thread to do the wakeup. 
		 * Wait until released, otherwise "waiter" is still globally
		 * visible.
		 */
		while (__predict_false(waiter.lid)) {
			(void)_lwp_park(CLOCK_MONOTONIC, 0, NULL, 0, NULL,
			    NULL);
		}
	} else {
		pthread__assert(!waiter.lid);
	}

	/*
	 * If cancelled then exit.  POSIX dictates that the mutex must be
	 * held if this happens.
	 */
	pthread_mutex_lock(mutex);
	if (cancel) {
		pthread__cancelled();
	}

	return error;
}

int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	if (__predict_false(__uselibcstub))
		return __libc_cond_wait_stub(cond, mutex);

	return pthread_cond_timedwait(cond, mutex, NULL);
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
	struct pthread__waiter *waiter, *next;
	pthread_mutex_t *mutex;
	pthread_t self;

	if (__predict_false(__uselibcstub))
		return __libc_cond_signal_stub(cond);

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);

	/* Take ownership of one waiter. */
	self = pthread_self();
	mutex = cond->ptc_mutex;
	for (waiter = cond->ptc_waiters;; waiter = next) {
		if (waiter == NULL) {
			return 0;
		}
		membar_datadep_consumer(); /* for alpha */
		next = waiter->next;
		next = atomic_cas_ptr(&cond->ptc_waiters, waiter, next);
		if (__predict_true(next == waiter)) {
			break;
		}
	}

	/* Now transfer waiter to the mutex. */
	waiter->next = NULL;
	pthread__mutex_deferwake(self, mutex, waiter);
	return 0;
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	struct pthread__waiter *head;
	pthread_mutex_t *mutex;
	pthread_t self;

	if (__predict_false(__uselibcstub))
		return __libc_cond_broadcast_stub(cond);

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);

	if (cond->ptc_waiters == NULL)
		return 0;

	/* Take ownership of the current set of waiters. */
	self = pthread_self();
	mutex = cond->ptc_mutex;
	head = atomic_swap_ptr(&cond->ptc_waiters, NULL);
	if (head == NULL) {
		return 0;
	}
	membar_datadep_consumer(); /* for alpha */

	/* Now transfer waiters to the mutex. */
	pthread__mutex_deferwake(self, mutex, head);
	return 0;
}

int
_pthread_cond_has_waiters_np(pthread_cond_t *cond)
{

	return cond->ptc_waiters != NULL;
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

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    attr->ptca_magic == _PT_CONDATTR_MAGIC);

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
pthread_condattr_getclock(const pthread_condattr_t *__restrict attr,
    clockid_t *__restrict clock_id)
{

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    attr->ptca_magic == _PT_CONDATTR_MAGIC);

	if (attr == NULL || attr->ptca_private == NULL)
		return EINVAL;
	*clock_id = *(clockid_t *)attr->ptca_private;
	return 0;
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

#ifdef _PTHREAD_PSHARED
int
pthread_condattr_getpshared(const pthread_condattr_t * __restrict attr,
    int * __restrict pshared)
{

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    attr->ptca_magic == _PT_CONDATTR_MAGIC);

	*pshared = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

int
pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared)
{

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    attr->ptca_magic == _PT_CONDATTR_MAGIC);

	switch(pshared) {
	case PTHREAD_PROCESS_PRIVATE:
		return 0;
	case PTHREAD_PROCESS_SHARED:
		return ENOSYS;
	}
	return EINVAL;
}
#endif
