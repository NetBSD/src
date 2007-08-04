/*	$NetBSD: pthread_cond.c,v 1.32.2.2 2007/08/04 13:37:50 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_cond.c,v 1.32.2.2 2007/08/04 13:37:50 ad Exp $");

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include "pthread.h"
#include "pthread_int.h"

int	_sys_nanosleep(const struct timespec *, struct timespec *);

extern int pthread__started;

static int pthread_cond_wait_nothread(pthread_t, pthread_mutex_t *,
    const struct timespec *);

__strong_alias(__libc_cond_init,pthread_cond_init)
__strong_alias(__libc_cond_signal,pthread_cond_signal)
__strong_alias(__libc_cond_broadcast,pthread_cond_broadcast)
__strong_alias(__libc_cond_wait,pthread_cond_wait)
__strong_alias(__libc_cond_timedwait,pthread_cond_timedwait)
__strong_alias(__libc_cond_destroy,pthread_cond_destroy)

int
pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    (attr == NULL) || (attr->ptca_magic == _PT_CONDATTR_MAGIC));

	cond->ptc_magic = _PT_COND_MAGIC;
	pthread_lockinit(&cond->ptc_lock);
	PTQ_INIT(&cond->ptc_waiters);
	cond->ptc_mutex = NULL;

	return 0;
}


int
pthread_cond_destroy(pthread_cond_t *cond)
{

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);
	pthread__error(EBUSY, "Destroying condition variable in use",
	    cond->ptc_mutex == NULL);

	cond->ptc_magic = _PT_COND_DEAD;

	return 0;
}


int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	pthread_t self;

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);
	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);
	pthread__error(EPERM, "Mutex not locked in condition wait",
	    mutex->ptm_lock == __SIMPLELOCK_LOCKED);

	self = pthread__self();
	PTHREADD_ADD(PTHREADD_COND_WAIT);

	/* Just hang out for a while if threads aren't running yet. */
	if (__predict_false(pthread__started == 0))
		return pthread_cond_wait_nothread(self, mutex, NULL);

	if (__predict_false(self->pt_cancel))
		pthread_exit(PTHREAD_CANCELED);

	/*
	 * Note this thread as waiting on the CV.  To ensure good
	 * performance it's critical that the spinlock is held for
	 * as short a time as possible - that means no system calls.
	 */ 
	pthread_spinlock(self, &cond->ptc_lock);
	if (cond->ptc_mutex == NULL)
		cond->ptc_mutex = mutex;
	else {
#ifdef ERRORCHECK
		pthread__error(EINVAL,
		    "Multiple mutexes used for condition wait", 
		    cond->ptc_mutex == mutex);
#endif
	}
	PTQ_INSERT_HEAD(&cond->ptc_waiters, self, pt_sleep);
	self->pt_signalled = 0;
	self->pt_sleeponq = 1;
	self->pt_sleepobj = &cond->ptc_waiters;
	pthread_spinunlock(self, &cond->ptc_lock);

	/* Once recorded as a waiter release the mutex and sleep. */
	pthread_mutex_unlock(mutex);
	(void)pthread__park(self, &cond->ptc_lock, &cond->ptc_waiters,
	    NULL, 1, &mutex->ptm_blocked);

	/*
	 * If we awoke abnormally the waiters list will have been
	 * made empty by the current thread (in pthread__park()),
	 * so we can check the value safely without locking.
	 *
	 * Otherwise, it will have been updated by whichever thread
	 * last issued a wakeup.
	 */
	if (PTQ_EMPTY(&cond->ptc_waiters) && cond->ptc_mutex != NULL) {
		pthread_spinlock(self, &cond->ptc_lock);
		if (PTQ_EMPTY(&cond->ptc_waiters))
			cond->ptc_mutex = NULL;
		pthread_spinunlock(self, &cond->ptc_lock);
	}

	/*
	 * Re-acquire the mutex and return to the caller.  If we
	 * have cancelled then exit.  POSIX dictates that the mutex
	 * must be held when we action the cancellation.
	 */
	pthread_mutex_lock(mutex);
	if (__predict_false(self->pt_cancel)) {
		if (self->pt_signalled)
			pthread_cond_signal(cond);
		pthread_exit(PTHREAD_CANCELED);
	}

	return 0;
}

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
    const struct timespec *abstime)
{
	pthread_t self;
	int retval;

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);
	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);
	pthread__error(EPERM, "Mutex not locked in condition wait",
	    mutex->ptm_lock == __SIMPLELOCK_LOCKED);
	pthread__error(EINVAL, "Invalid wait time", 
	    (abstime->tv_sec >= 0) &&
	    (abstime->tv_nsec >= 0) && (abstime->tv_nsec < 1000000000));

	self = pthread__self();
	PTHREADD_ADD(PTHREADD_COND_TIMEDWAIT);

	/* Just hang out for a while if threads aren't running yet. */
	if (__predict_false(pthread__started == 0))
		return pthread_cond_wait_nothread(self, mutex, abstime);

	if (__predict_false(self->pt_cancel))
		pthread_exit(PTHREAD_CANCELED);

	/*
	 * Note this thread as waiting on the CV.  To ensure good
	 * performance it's critical that the spinlock is held for
	 * as short a time as possible - that means no system calls.
	 */ 
	pthread_spinlock(self, &cond->ptc_lock);
	if (cond->ptc_mutex == NULL)
		cond->ptc_mutex = mutex;
	else {
#ifdef ERRORCHECK
		pthread__error(EINVAL,
		    "Multiple mutexes used for condition wait",
		    cond->ptc_mutex == mutex);
#endif
	}
	PTQ_INSERT_HEAD(&cond->ptc_waiters, self, pt_sleep);
	self->pt_signalled = 0;
	self->pt_sleeponq = 1;
	self->pt_sleepobj = &cond->ptc_waiters;
	pthread_spinunlock(self, &cond->ptc_lock);

	/* Once recorded as a waiter release the mutex and sleep. */
	pthread_mutex_unlock(mutex);
	retval = pthread__park(self, &cond->ptc_lock, &cond->ptc_waiters,
	    abstime, 1, &mutex->ptm_blocked);

	/*
	 * If we awoke abnormally the waiters list will have been
	 * made empty by the current thread (in pthread__park()),
	 * so we can check the value safely without locking.
	 *
	 * Otherwise, it will have been updated by whichever thread
	 * last issued a wakeup.
	 */
	if (PTQ_EMPTY(&cond->ptc_waiters) && cond->ptc_mutex != NULL) {
		pthread_spinlock(self, &cond->ptc_lock);
		if (PTQ_EMPTY(&cond->ptc_waiters))
			cond->ptc_mutex = NULL;
		pthread_spinunlock(self, &cond->ptc_lock);
	}

	/*
	 * Re-acquire the mutex and return to the caller.  If we
	 * have cancelled then exit.  POSIX dictates that the mutex
	 * must be held when we action the cancellation.
	 */
	pthread_mutex_lock(mutex);
	if (__predict_false(self->pt_cancel | retval)) {
		if (self->pt_signalled)
			pthread_cond_signal(cond);
		if (self->pt_cancel)
			pthread_exit(PTHREAD_CANCELED);
	}

	return retval;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
	pthread_t self, signaled;
	pthread_mutex_t *mutex;

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);
	PTHREADD_ADD(PTHREADD_COND_SIGNAL);

	if (PTQ_EMPTY(&cond->ptc_waiters))
		return 0;

	self = pthread__self();
	pthread_spinlock(self, &cond->ptc_lock);

	/*
	 * Find a thread that is still blocked (no pending wakeup).
	 * A wakeup can be pending if we have interrupted unpark_all
	 * as it releases the interlock.
	 */
	PTQ_FOREACH(signaled, &cond->ptc_waiters, pt_sleep) {	
		if (signaled->pt_sleepobj != NULL)
			break;
	}
	if (__predict_false(signaled == NULL)) {
		cond->ptc_mutex = NULL;
		pthread_spinunlock(self, &cond->ptc_lock);
		return 0;
	}

	/*
	 * Pull the thread off the queue, and set pt_signalled.
	 *
	 * After resuming execution, the thread must check to see if it
	 * has been restarted as a result of pthread_cond_signal().  If it
	 * has, but cannot take the wakeup (because of eg a timeout) then
	 * try to ensure that another thread sees it.  This is necessary
	 * because there may be multiple waiters, and at least one should
	 * take the wakeup if possible.
	 */
	PTQ_REMOVE(&cond->ptc_waiters, signaled, pt_sleep);
	mutex = cond->ptc_mutex;
	if (PTQ_EMPTY(&cond->ptc_waiters))
		cond->ptc_mutex = NULL;
	signaled->pt_signalled = 1;

	/*
	 * For all valid uses of pthread_cond_signal(), the caller will
	 * hold the mutex that the target is using to synchronize with.
	 * To avoid the target awakening and immediatley blocking on the
	 * mutex, transfer the thread to be awoken to the mutex's waiters
	 * list.  The waiter will be set running when the caller (this
	 * thread) releases the mutex.
	 */
	if (self->pt_mutexhint != NULL && self->pt_mutexhint == mutex) {
		pthread_spinunlock(self, &cond->ptc_lock);
		pthread_spinlock(self, &mutex->ptm_interlock);
		PTQ_INSERT_HEAD(&mutex->ptm_blocked, signaled, pt_sleep);
		pthread_spinunlock(self, &mutex->ptm_interlock);
	} else {
		pthread__unpark(self, &cond->ptc_lock,
		    &cond->ptc_waiters, signaled);
	}
	PTHREADD_ADD(PTHREADD_COND_WOKEUP);

	return 0;
}


int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	pthread_t self, signaled, next;
	pthread_mutex_t *mutex;

	pthread__error(EINVAL, "Invalid condition variable",
	    cond->ptc_magic == _PT_COND_MAGIC);

	PTHREADD_ADD(PTHREADD_COND_BROADCAST);

	if (PTQ_EMPTY(&cond->ptc_waiters))
		return 0;

	self = pthread__self();
	pthread_spinlock(self, &cond->ptc_lock);
	mutex = cond->ptc_mutex;
	cond->ptc_mutex = NULL;

	/*
	 * Try to transfer waiters to the mutex's waiters list (see
	 * pthread_cond_signal()).  Only transfer waiters for which
	 * there is no pending wakeup.
	 */
	if (self->pt_mutexhint != NULL && self->pt_mutexhint == mutex) {
		pthread_spinlock(self, &mutex->ptm_interlock);
		for (signaled = PTQ_FIRST(&cond->ptc_waiters);
		    signaled != NULL;
		    signaled = next) {	
		    	next = PTQ_NEXT(signaled, pt_sleep);
		    	if (__predict_false(signaled->pt_sleepobj == NULL))
		    		continue;
		    	PTQ_REMOVE(&cond->ptc_waiters, signaled, pt_sleep);
		    	PTQ_INSERT_HEAD(&mutex->ptm_blocked, signaled,
		    	    pt_sleep);
		}
		pthread_spinunlock(self, &mutex->ptm_interlock);
		pthread_spinunlock(self, &cond->ptc_lock);
	} else
		pthread__unpark_all(self, &cond->ptc_lock, &cond->ptc_waiters);

	PTHREADD_ADD(PTHREADD_COND_WOKEUP);

	return 0;

}


int
pthread_condattr_init(pthread_condattr_t *attr)
{

	attr->ptca_magic = _PT_CONDATTR_MAGIC;

	return 0;
}


int
pthread_condattr_destroy(pthread_condattr_t *attr)
{

	pthread__error(EINVAL, "Invalid condition variable attribute",
	    attr->ptca_magic == _PT_CONDATTR_MAGIC);

	attr->ptca_magic = _PT_CONDATTR_DEAD;

	return 0;
}

/* Utility routine to hang out for a while if threads haven't started yet. */
static int
pthread_cond_wait_nothread(pthread_t self, pthread_mutex_t *mutex,
    const struct timespec *abstime)
{
	struct timespec now, diff;
	int retval;

	if (abstime == NULL) {
		diff.tv_sec = 99999999;
		diff.tv_nsec = 0;
	} else {
		clock_gettime(CLOCK_REALTIME, &now);
		if  (timespeccmp(abstime, &now, <))
			timespecclear(&diff);
		else
			timespecsub(abstime, &now, &diff);
	}

	do {
		pthread__testcancel(self);
		pthread_mutex_unlock(mutex);
		retval = _sys_nanosleep(&diff, NULL);
		pthread_mutex_lock(mutex);
	} while (abstime == NULL && retval == 0);
	pthread__testcancel(self);

	if (retval == 0)
		return ETIMEDOUT;
	else
		/* spurious wakeup */
		return 0;
}
