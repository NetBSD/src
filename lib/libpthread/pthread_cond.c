/*	$NetBSD: pthread_cond.c,v 1.5 2003/01/31 04:59:40 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#include <assert.h>
#include <errno.h>
#include <sys/cdefs.h>

#include "pthread.h"
#include "pthread_int.h"

#undef PTHREAD_COND_DEBUG

#ifdef PTHREAD_COND_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

static void pthread_cond_wait__callback(void *);

__strong_alias(__libc_cond_init,pthread_cond_init)
__strong_alias(__libc_cond_signal,pthread_cond_signal)
__strong_alias(__libc_cond_broadcast,pthread_cond_broadcast)
__strong_alias(__libc_cond_wait,pthread_cond_wait)
__strong_alias(__libc_cond_timedwait,pthread_cond_timedwait)
__strong_alias(__libc_cond_destroy,pthread_cond_destroy)

int
pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{

#ifdef ERRORCHECK
	if ((cond == NULL) ||
	    (attr && (attr->ptca_magic != _PT_CONDATTR_MAGIC)))
		return EINVAL;
#endif

	cond->ptc_magic = _PT_COND_MAGIC;
	pthread_lockinit(&cond->ptc_lock);
	PTQ_INIT(&cond->ptc_waiters);
	cond->ptc_mutex = NULL;

	return 0;
}


int
pthread_cond_destroy(pthread_cond_t *cond)
{

#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC) ||
	    (cond->ptc_mutex != NULL) ||
	    (cond->ptc_lock != __SIMPLELOCK_UNLOCKED))
		return EINVAL;
#endif

	cond->ptc_magic = _PT_COND_DEAD;

	return 0;
}


int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	pthread_t self;
#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC) ||
	    (mutex == NULL) || (mutex->ptm_magic != _PT_MUTEX_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	PTHREADD_ADD(PTHREADD_COND_WAIT);
	pthread_spinlock(self, &cond->ptc_lock);
#ifdef ERRORCHECK
	if (cond->ptc_mutex == NULL)
		cond->ptc_mutex = mutex;
	else {
		if (cond->ptc_mutex != mutex) {
			pthread_spinunlock(self, &cond->ptc_lock);
			return EINVAL;
		}
		/* Check the mutex is actually locked */
	        if (mutex->ptm_lock != __SIMPLELOCK_LOCKED) {
			pthread_spinunlock(self, &cond->ptc_lock);
			return EPERM;
		}
	}
#endif
	SDPRINTF(("(cond wait %p) Waiting on %p, mutex %p\n",
	    self, cond, mutex));
	pthread_spinlock(self, &self->pt_statelock);
	if (self->pt_cancel) {
		pthread_spinunlock(self, &self->pt_statelock);
		pthread_spinunlock(self, &cond->ptc_lock);
		pthread_exit(PTHREAD_CANCELED);
	}
	self->pt_state = PT_STATE_BLOCKED_QUEUE;
	self->pt_sleepobj = cond;
	self->pt_sleepq = &cond->ptc_waiters;
	self->pt_sleeplock = &cond->ptc_lock;
	pthread_spinunlock(self, &self->pt_statelock);

	PTQ_INSERT_TAIL(&cond->ptc_waiters, self, pt_sleep);
	pthread_mutex_unlock(mutex);

	pthread__block(self, &cond->ptc_lock);
	/* Spinlock is unlocked on return */
	pthread_mutex_lock(mutex);
	pthread__testcancel(self);
	SDPRINTF(("(cond wait %p) Woke up on %p, mutex %p\n",
	    self, cond, mutex));

	return 0;
}


struct pthread_cond__waitarg {
	pthread_t ptw_thread;
	pthread_cond_t *ptw_cond;
};

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
    const struct timespec *abstime)
{
	pthread_t self;
	struct pthread_cond__waitarg wait;
	struct pt_alarm_t alarm;
	int retval;

#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC) ||
	    (mutex == NULL) || (mutex->ptm_magic != _PT_MUTEX_MAGIC))
		return EINVAL;
	if ((abstime == NULL) || (abstime->tv_sec < 0 ||
	    abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000))
		return EINVAL;
#endif
	self = pthread__self();
	pthread_spinlock(self, &cond->ptc_lock);
#ifdef ERRORCHECK
	if (cond->ptc_mutex == NULL)
		cond->ptc_mutex = mutex;
	else {
		if (cond->ptc_mutex != mutex) {
			pthread_spinunlock(self, &cond->ptc_lock);
			return EINVAL;
		}
		/* Check the mutex is actually locked */
	        if (mutex->ptm_lock != __SIMPLELOCK_LOCKED) {
			pthread_spinunlock(self, &cond->ptc_lock);
			return EPERM;
		}
	}
#endif
	PTHREADD_ADD(PTHREADD_COND_TIMEDWAIT);
	wait.ptw_thread = self;
	wait.ptw_cond = cond;
	retval = 0;
	SDPRINTF(("(cond timed wait %p) Waiting on %p until %d.%06ld\n",
	    self, cond, abstime->tv_sec, abstime->tv_nsec/1000));

	pthread_spinlock(self, &self->pt_statelock);
	if (self->pt_cancel) {
		pthread_spinunlock(self, &self->pt_statelock);
		pthread_spinunlock(self, &cond->ptc_lock);
		pthread_exit(PTHREAD_CANCELED);
	}
	pthread__alarm_add(self, &alarm, abstime, pthread_cond_wait__callback,
	    &wait);
	self->pt_state = PT_STATE_BLOCKED_QUEUE;
	self->pt_sleepobj = cond;
	self->pt_sleepq = &cond->ptc_waiters;
	self->pt_sleeplock = &cond->ptc_lock;
	pthread_spinunlock(self, &self->pt_statelock);

	PTQ_INSERT_TAIL(&cond->ptc_waiters, self, pt_sleep);
	pthread_mutex_unlock(mutex);

	pthread__block(self, &cond->ptc_lock);
	/* Spinlock is unlocked on return */
	SDPRINTF(("(cond timed wait %p) Woke up on %p, mutex %p\n",
	    self, cond));
	pthread__alarm_del(self, &alarm);
	if (pthread__alarm_fired(&alarm))
		retval = ETIMEDOUT;
	SDPRINTF(("(cond timed wait %p) %s\n",
	    self, (retval == ETIMEDOUT) ? "(timed out)" : ""));
	pthread_mutex_lock(mutex);
	pthread__testcancel(self);

	return retval;
}

static void
pthread_cond_wait__callback(void *arg)
{
	struct pthread_cond__waitarg *a;
	pthread_t self;

	a = arg;
	self = pthread__self();

	/*
	 * Don't dequeue and schedule the thread if it's already been
	 * queued up by a signal or broadcast (but hasn't yet run as far
	 * as pthread__alarm_del(), or we wouldn't be here, and hence can't
	 * have become blocked on some *other* queue).
	 */
	pthread_spinlock(self, &a->ptw_cond->ptc_lock);
	if (a->ptw_thread->pt_state == PT_STATE_BLOCKED_QUEUE) {
		PTQ_REMOVE(&a->ptw_cond->ptc_waiters, a->ptw_thread, pt_sleep);
#ifdef ERRORCHECK
		if (PTQ_EMPTY(&a->ptw_cond->ptc_waiters))
			a->ptw_cond->ptc_mutex = NULL;
#endif
		pthread__sched(self, a->ptw_thread);
	}
	pthread_spinunlock(self, &a->ptw_cond->ptc_lock);
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
	pthread_t self, signaled;
#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC))
		return EINVAL;
#endif
	PTHREADD_ADD(PTHREADD_COND_SIGNAL);

	SDPRINTF(("(cond signal %p) Signaling %p\n",
	    pthread__self(), cond));

	if (!PTQ_EMPTY(&cond->ptc_waiters)) {
		self = pthread__self();
		pthread_spinlock(self, &cond->ptc_lock);
		signaled = PTQ_FIRST(&cond->ptc_waiters);
		if (signaled != NULL)
			PTQ_REMOVE(&cond->ptc_waiters, signaled, pt_sleep);
#ifdef ERRORCHECK
		if (PTQ_EMPTY(&cond->ptc_waiters))
			cond->ptc_mutex = NULL;
#endif
		pthread_spinunlock(self, &cond->ptc_lock);
		if (signaled != NULL)
			pthread__sched(self, signaled);
	}

	return 0;
}


int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	pthread_t self;
	struct pthread_queue_t blockedq;
#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC))
		return EINVAL;
#endif

	PTHREADD_ADD(PTHREADD_COND_BROADCAST);
	SDPRINTF(("(cond signal %p) Broadcasting %p\n",
	    pthread__self(), cond));

	if (!PTQ_EMPTY(&cond->ptc_waiters)) {
		self = pthread__self();
		pthread_spinlock(self, &cond->ptc_lock);
		blockedq = cond->ptc_waiters;
		PTQ_INIT(&cond->ptc_waiters);
#ifdef ERRORCHECK
		cond->ptc_mutex = NULL;
#endif
		pthread_spinunlock(self, &cond->ptc_lock);
		pthread__sched_sleepers(self, &blockedq);
	}

	return 0;

}


int
pthread_condattr_init(pthread_condattr_t *attr)
{

#ifdef ERRORCHECK
	if (attr == NULL)
		return EINVAL;
#endif

	attr->ptca_magic = _PT_CONDATTR_MAGIC;

	return 0;
}


int
pthread_condattr_destroy(pthread_condattr_t *attr)
{

#ifdef ERRORCHECK
	if ((attr == NULL) ||
	    (attr->ptca_magic != _PT_CONDATTR_MAGIC))
		return EINVAL;
#endif

	attr->ptca_magic = _PT_CONDATTR_DEAD;

	return 0;
}
