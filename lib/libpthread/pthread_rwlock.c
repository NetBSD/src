/*	$NetBSD: pthread_rwlock.c,v 1.5 2003/03/08 08:03:35 lukem Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_rwlock.c,v 1.5 2003/03/08 08:03:35 lukem Exp $");

#include <errno.h>

#include "pthread.h"
#include "pthread_int.h"

static void pthread_rwlock__callback(void *);

__strong_alias(__libc_rwlock_init,pthread_rwlock_init)
__strong_alias(__libc_rwlock_rdlock,pthread_rwlock_rdlock)
__strong_alias(__libc_rwlock_wrlock,pthread_rwlock_wrlock)
__strong_alias(__libc_rwlock_tryrdlock,pthread_rwlock_tryrdlock)
__strong_alias(__libc_rwlock_trywrlock,pthread_rwlock_trywrlock)
__strong_alias(__libc_rwlock_unlock,pthread_rwlock_unlock)
__strong_alias(__libc_rwlock_destroy,pthread_rwlock_destroy)

int
pthread_rwlock_init(pthread_rwlock_t *rwlock,
	    const pthread_rwlockattr_t *attr)
{
#ifdef ERRORCHECK
	if ((rwlock == NULL) ||
	    (attr && (attr->ptra_magic != _PT_RWLOCKATTR_MAGIC)))
		return EINVAL;
#endif
	rwlock->ptr_magic = _PT_RWLOCK_MAGIC;
	pthread_lockinit(&rwlock->ptr_interlock);
	PTQ_INIT(&rwlock->ptr_rblocked);
	PTQ_INIT(&rwlock->ptr_wblocked);
	rwlock->ptr_nreaders = 0;
	rwlock->ptr_writer = NULL;

	return 0;
}


int
pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
#ifdef ERRORCHECK
	if ((rwlock == NULL) ||
	    (rwlock->ptr_magic != _PT_RWLOCK_MAGIC) ||
	    (!PTQ_EMPTY(&rwlock->ptr_rblocked)) ||
	    (!PTQ_EMPTY(&rwlock->ptr_wblocked)) ||
	    (rwlock->ptr_nreaders != 0) ||
	    (rwlock->ptr_writer != NULL))
		return EINVAL;
#endif
	rwlock->ptr_magic = _PT_RWLOCK_DEAD;

	return 0;
}


int
pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	pthread_t self;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread_spinunlock(self, &rwlock->ptr_interlock);
		return EDEADLK;
	}
#endif
	/*
	 * Don't get a readlock if there is a writer or if there are waiting
	 * writers; i.e. prefer writers to readers. This strategy is dictated
	 * by SUSv3.
	 */
	while ((rwlock->ptr_writer != NULL) ||
	    (!PTQ_EMPTY(&rwlock->ptr_wblocked))) {
		PTQ_INSERT_TAIL(&rwlock->ptr_rblocked, self, pt_sleep);
		/* Locking a rwlock is not a cancellation point; don't check */
		pthread_spinlock(self, &self->pt_statelock);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = rwlock;
		self->pt_sleepq = &rwlock->ptr_rblocked;
		self->pt_sleeplock = &rwlock->ptr_interlock;
		pthread_spinunlock(self, &self->pt_statelock);
		pthread__block(self, &rwlock->ptr_interlock);
		/* interlock is not held when we return */
		pthread_spinlock(self, &rwlock->ptr_interlock);
	}
	
	rwlock->ptr_nreaders++;
	pthread_spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


int
pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	pthread_t self;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread_spinunlock(self, &rwlock->ptr_interlock);
		return EDEADLK;
	}
#endif
	/*
	 * Don't get a readlock if there is a writer or if there are waiting
	 * writers; i.e. prefer writers to readers. This strategy is dictated
	 * by SUSv3.
	 */
	if ((rwlock->ptr_writer != NULL) ||
	    (!PTQ_EMPTY(&rwlock->ptr_wblocked))) {
		pthread_spinunlock(self, &rwlock->ptr_interlock);
		return EBUSY;
	}

	rwlock->ptr_nreaders++;
	pthread_spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


int
pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	pthread_t self;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
	/*
	 * Prefer writers to readers here; permit writers even if there are
	 * waiting readers.
	 */
	while ((rwlock->ptr_nreaders > 0) || (rwlock->ptr_writer != NULL)) {
		PTQ_INSERT_TAIL(&rwlock->ptr_wblocked, self, pt_sleep);
		/* Locking a rwlock is not a cancellation point; don't check */
		pthread_spinlock(self, &self->pt_statelock);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = rwlock;
		self->pt_sleepq = &rwlock->ptr_wblocked;
		self->pt_sleeplock = &rwlock->ptr_interlock;
		pthread_spinunlock(self, &self->pt_statelock);
		pthread__block(self, &rwlock->ptr_interlock);
		/* interlock is not held when we return */
		pthread_spinlock(self, &rwlock->ptr_interlock);
	}

	rwlock->ptr_writer = self;
	pthread_spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


int
pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	pthread_t self;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
	/*
	 * Prefer writers to readers here; permit writers even if there are
	 * waiting readers.
	 */
	if ((rwlock->ptr_nreaders > 0) || (rwlock->ptr_writer != NULL)) {
		pthread_spinunlock(self, &rwlock->ptr_interlock);
		return EBUSY;
	}

	rwlock->ptr_writer = self;
	pthread_spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


struct pthread_rwlock__waitarg {
	pthread_t ptw_thread;
	pthread_rwlock_t *ptw_rwlock;
	struct pthread_queue_t *ptw_queue;
};

int
pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
	    const struct timespec *abs_timeout)
{
	pthread_t self;
	struct pthread_rwlock__waitarg wait;
	struct pt_alarm_t alarm;
	int retval;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
	if ((abs_timeout == NULL) || (abs_timeout->tv_nsec >= 1000000000))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread_spinlock(self, &rwlock->ptr_interlock);
		return EDEADLK;
	}
#endif
	/*
	 * Don't get a readlock if there is a writer or if there are waiting
	 * writers; i.e. prefer writers to readers. This strategy is dictated
	 * by SUSv3.
	 */
	retval = 0;
	while ((retval == 0) && ((rwlock->ptr_writer != NULL) ||
	    (!PTQ_EMPTY(&rwlock->ptr_wblocked)))) {
		wait.ptw_thread = self;
		wait.ptw_rwlock = rwlock;
		wait.ptw_queue = &rwlock->ptr_rblocked;
		pthread__alarm_add(self, &alarm, abs_timeout,
		    pthread_rwlock__callback, &wait);
		PTQ_INSERT_TAIL(&rwlock->ptr_rblocked, self, pt_sleep);
		/* Locking a rwlock is not a cancellation point; don't check */
		pthread_spinlock(self, &self->pt_statelock);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = rwlock;
		self->pt_sleepq = &rwlock->ptr_rblocked;
		self->pt_sleeplock = &rwlock->ptr_interlock;
		pthread_spinunlock(self, &self->pt_statelock);
		pthread__block(self, &rwlock->ptr_interlock);
		/* interlock is not held when we return */
		pthread__alarm_del(self, &alarm);
		if (pthread__alarm_fired(&alarm))
			retval = ETIMEDOUT;
		pthread_spinlock(self, &rwlock->ptr_interlock);
	}

	if (retval == 0)
		rwlock->ptr_nreaders++;
	pthread_spinunlock(self, &rwlock->ptr_interlock);

	return retval;
}


int
pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
	    const struct timespec *abs_timeout)
{
	struct pthread_rwlock__waitarg wait;
	struct pt_alarm_t alarm;
	int retval;
	pthread_t self;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
	/*
	 * Prefer writers to readers here; permit writers even if there are
	 * waiting readers.
	 */
	retval = 0;
	while (retval == 0 &&
	    ((rwlock->ptr_nreaders > 0) || (rwlock->ptr_writer != NULL))) {
		wait.ptw_thread = self;
		wait.ptw_rwlock = rwlock;
		wait.ptw_queue = &rwlock->ptr_wblocked;
		pthread__alarm_add(self, &alarm, abs_timeout,
		    pthread_rwlock__callback, &wait);
		PTQ_INSERT_TAIL(&rwlock->ptr_wblocked, self, pt_sleep);
		/* Locking a rwlock is not a cancellation point; don't check */
		pthread_spinlock(self, &self->pt_statelock);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = rwlock;
		self->pt_sleepq = &rwlock->ptr_wblocked;
		self->pt_sleeplock = &rwlock->ptr_interlock;
		pthread_spinunlock(self, &self->pt_statelock);
		pthread__block(self, &rwlock->ptr_interlock);
		/* interlock is not held when we return */
		pthread__alarm_del(self, &alarm);
		if (pthread__alarm_fired(&alarm))
			retval = ETIMEDOUT;
		pthread_spinlock(self, &rwlock->ptr_interlock);
	}

	if (retval == 0)
		rwlock->ptr_writer = self;
	pthread_spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


static void
pthread_rwlock__callback(void *arg)
{
	struct pthread_rwlock__waitarg *a;
	pthread_t self;

	a = arg;
	self = pthread__self();

	pthread_spinlock(self, &a->ptw_rwlock->ptr_interlock);
	/*
	 * Don't dequeue and schedule the thread if it's already been
	 * queued up by a signal or broadcast (but hasn't yet run as far
	 * as pthread__alarm_del(), or we wouldn't be here, and hence can't
	 * have become blocked on some *other* queue).
	 */
	if (a->ptw_thread->pt_state == PT_STATE_BLOCKED_QUEUE) {
		PTQ_REMOVE(a->ptw_queue, a->ptw_thread, pt_sleep);
		pthread__sched(self, a->ptw_thread);
	}
	pthread_spinunlock(self, &a->ptw_rwlock->ptr_interlock);

}


int
pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	pthread_t self, writer;
	struct pthread_queue_t blockedq;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	writer = NULL;
	PTQ_INIT(&blockedq);
	self = pthread__self();
	
	pthread_spinlock(self, &rwlock->ptr_interlock);
	if (rwlock->ptr_writer != NULL) {
		/* Releasing a write lock. */
#ifdef ERRORCHECK
		if (rwlock->ptr_writer != self) {
			pthread_spinunlock(self, &rwlock->ptr_interlock);
			return EPERM;
		}
#endif
		rwlock->ptr_writer = NULL;
		writer = PTQ_FIRST(&rwlock->ptr_wblocked);
		if (writer != NULL) {
			PTQ_REMOVE(&rwlock->ptr_wblocked, writer, pt_sleep);
		} else {
			blockedq = rwlock->ptr_rblocked;
			PTQ_INIT(&rwlock->ptr_rblocked);
		}
	} else {
		/* Releasing a read lock. */
		rwlock->ptr_nreaders--;
		if (rwlock->ptr_nreaders == 0) {
			writer = PTQ_FIRST(&rwlock->ptr_wblocked);
			if (writer != NULL)
				PTQ_REMOVE(&rwlock->ptr_wblocked, writer,
				    pt_sleep);
		}
	}

	pthread_spinunlock(self, &rwlock->ptr_interlock);

	if (writer != NULL)
		pthread__sched(self, writer);
	else
		pthread__sched_sleepers(self, &blockedq);
	
	return 0;
}


int
pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
#ifdef ERRORCHECK
	if (attr == NULL)
		return EINVAL;
#endif
	attr->ptra_magic = _PT_RWLOCKATTR_MAGIC;

	return 0;
}


int
pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
#ifdef ERRORCHECK
	if ((attr == NULL) ||
	    (attr->ptra_magic != _PT_RWLOCKATTR_MAGIC))
		return EINVAL;
#endif
	attr->ptra_magic = _PT_RWLOCKATTR_DEAD;

	return 0;
}
