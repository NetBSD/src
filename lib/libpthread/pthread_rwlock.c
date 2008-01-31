/*	$NetBSD: pthread_rwlock.c,v 1.26 2008/01/31 11:50:40 ad Exp $ */

/*-
 * Copyright (c) 2002, 2006, 2007 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_rwlock.c,v 1.26 2008/01/31 11:50:40 ad Exp $");

#include <errno.h>

#include "pthread.h"
#include "pthread_int.h"

#ifndef	PTHREAD__HAVE_ATOMIC

__weak_alias(pthread_rwlock_held_np,_pthread_rwlock_held_np)
__weak_alias(pthread_rwlock_rdheld_np,_pthread_rwlock_rdheld_np)
__weak_alias(pthread_rwlock_wrheld_np,_pthread_rwlock_wrheld_np)

int	_pthread_rwlock_held_np(pthread_rwlock_t *);
int	_pthread_rwlock_rdheld_np(pthread_rwlock_t *);
int	_pthread_rwlock_wrheld_np(pthread_rwlock_t *);

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
	
	pthread__spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
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
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &rwlock->ptr_rblocked;
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		(void)pthread__park(self, &rwlock->ptr_interlock,
		    &rwlock->ptr_rblocked, NULL, 0, &rwlock->ptr_rblocked);
		pthread__spinlock(self, &rwlock->ptr_interlock);
	}
	
	rwlock->ptr_nreaders++;
	pthread__spinunlock(self, &rwlock->ptr_interlock);

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
	pthread__spinlock(self, &rwlock->ptr_interlock);
	/*
	 * Don't get a readlock if there is a writer or if there are waiting
	 * writers; i.e. prefer writers to readers. This strategy is dictated
	 * by SUSv3.
	 */
	if ((rwlock->ptr_writer != NULL) ||
	    (!PTQ_EMPTY(&rwlock->ptr_wblocked))) {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		return EBUSY;
	}

	rwlock->ptr_nreaders++;
	pthread__spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


int
pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	pthread_t self;
	extern int pthread__started;

#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();
	
	pthread__spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		return EDEADLK;
	}
#endif
	/*
	 * Prefer writers to readers here; permit writers even if there are
	 * waiting readers.
	 */
	while ((rwlock->ptr_nreaders > 0) || (rwlock->ptr_writer != NULL)) {
#ifdef ERRORCHECK
		if (pthread__started == 0) {
			pthread__spinunlock(self, &rwlock->ptr_interlock);
			return EDEADLK;
		}
#endif
	    	PTQ_INSERT_TAIL(&rwlock->ptr_wblocked, self, pt_sleep);
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &rwlock->ptr_wblocked;
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		(void)pthread__park(self, &rwlock->ptr_interlock,
		    &rwlock->ptr_wblocked, NULL, 0, &rwlock->ptr_wblocked);
		pthread__spinlock(self, &rwlock->ptr_interlock);
	}

	rwlock->ptr_writer = self;
	pthread__spinunlock(self, &rwlock->ptr_interlock);

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
	
	pthread__spinlock(self, &rwlock->ptr_interlock);
	/*
	 * Prefer writers to readers here; permit writers even if there are
	 * waiting readers.
	 */
	if ((rwlock->ptr_nreaders > 0) || (rwlock->ptr_writer != NULL)) {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		return EBUSY;
	}

	rwlock->ptr_writer = self;
	pthread__spinunlock(self, &rwlock->ptr_interlock);

	return 0;
}


int
pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
	    const struct timespec *abs_timeout)
{
	pthread_t self;
	int retval;

#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
	if (abs_timeout == NULL)
		return EINVAL;
#endif
	if ((abs_timeout->tv_nsec >= 1000000000) ||
	    (abs_timeout->tv_nsec < 0) ||
	    (abs_timeout->tv_sec < 0))
		return EINVAL;

	self = pthread__self();
	pthread__spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
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
	    	PTQ_INSERT_TAIL(&rwlock->ptr_rblocked, self, pt_sleep);
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &rwlock->ptr_rblocked;
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		retval = pthread__park(self, &rwlock->ptr_interlock,
		    &rwlock->ptr_rblocked, abs_timeout, 0,
		    &rwlock->ptr_rblocked);
		pthread__spinlock(self, &rwlock->ptr_interlock);
	}

	/* One last chance to get the lock, in case it was released between
	   the alarm firing and when this thread got rescheduled, or in case
	   a signal handler kept it busy */
	if ((rwlock->ptr_writer == NULL) &&
	    (PTQ_EMPTY(&rwlock->ptr_wblocked))) {
		rwlock->ptr_nreaders++;
		retval = 0;
	}
	pthread__spinunlock(self, &rwlock->ptr_interlock);

	return retval;
}


int
pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
	    const struct timespec *abs_timeout)
{
	pthread_t self;
	int retval;
	extern int pthread__started;

#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
	if (abs_timeout == NULL)
		return EINVAL;
#endif
	if ((abs_timeout->tv_nsec >= 1000000000) ||
	    (abs_timeout->tv_nsec < 0) ||
	    (abs_timeout->tv_sec < 0))
		return EINVAL;

	self = pthread__self();
	pthread__spinlock(self, &rwlock->ptr_interlock);
#ifdef ERRORCHECK
	if (rwlock->ptr_writer == self) {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		return EDEADLK;
	}
#endif
	/*
	 * Prefer writers to readers here; permit writers even if there are
	 * waiting readers.
	 */
	retval = 0;
	while (retval == 0 &&
	    ((rwlock->ptr_nreaders > 0) || (rwlock->ptr_writer != NULL))) {
#ifdef ERRORCHECK
		if (pthread__started == 0) {
			pthread__spinunlock(self, &rwlock->ptr_interlock);
			return EDEADLK;
		}
#endif
	    	PTQ_INSERT_TAIL(&rwlock->ptr_wblocked, self, pt_sleep);
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &rwlock->ptr_wblocked;
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		retval = pthread__park(self, &rwlock->ptr_interlock,
		    &rwlock->ptr_wblocked, abs_timeout, 0,
		    &rwlock->ptr_wblocked);
		pthread__spinlock(self, &rwlock->ptr_interlock);
	}

	if ((rwlock->ptr_nreaders == 0) && (rwlock->ptr_writer == NULL)) {
		rwlock->ptr_writer = self;
		retval = 0;
	}
	pthread__spinunlock(self, &rwlock->ptr_interlock);

	return retval;
}


int
pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	pthread_t self, writer;
#ifdef ERRORCHECK
	if ((rwlock == NULL) || (rwlock->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif
	writer = NULL;
	self = pthread__self();
	
	pthread__spinlock(self, &rwlock->ptr_interlock);
	if (rwlock->ptr_writer != NULL) {
		/* Releasing a write lock. */
#ifdef ERRORCHECK
		if (rwlock->ptr_writer != self) {
			pthread__spinunlock(self, &rwlock->ptr_interlock);
			return EPERM;
		}
#endif
		rwlock->ptr_writer = NULL;
		writer = PTQ_FIRST(&rwlock->ptr_wblocked);
		if (writer != NULL) {
			PTQ_REMOVE(&rwlock->ptr_wblocked, writer, pt_sleep);
		}
	} else
#ifdef ERRORCHECK
	if (rwlock->ptr_nreaders > 0)
#endif
	{
		/* Releasing a read lock. */
		rwlock->ptr_nreaders--;
		if (rwlock->ptr_nreaders == 0) {
			writer = PTQ_FIRST(&rwlock->ptr_wblocked);
			if (writer != NULL)
				PTQ_REMOVE(&rwlock->ptr_wblocked, writer,
				    pt_sleep);
		}
#ifdef ERRORCHECK
	} else {
		pthread__spinunlock(self, &rwlock->ptr_interlock);
		return EPERM;
#endif	
	}

	if (writer != NULL)
		pthread__unpark(self, &rwlock->ptr_interlock,
		    &rwlock->ptr_wblocked, writer);
	else
		pthread__unpark_all(self, &rwlock->ptr_interlock,
		    &rwlock->ptr_rblocked);

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

int
_pthread_rwlock_held_np(pthread_rwlock_t *ptr)
{

	return ptr->ptr_writer != NULL || ptr->ptr_nreaders != 0;
}

int
_pthread_rwlock_rdheld_np(pthread_rwlock_t *ptr)
{

	return ptr->ptr_nreaders != 0;
}

int
_pthread_rwlock_wrheld_np(pthread_rwlock_t *ptr)
{

	return ptr->ptr_writer == pthread__self();
}

#endif	/* !PTHREAD__HAVE_ATOMIC */
