/*	$NetBSD: pthread_cond.c,v 1.1.2.3 2001/08/06 20:51:41 nathanw Exp $	*/

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

#include "pthread.h"
#include "pthread_int.h"


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
	pthread_spinlock(self, &cond->ptc_lock);
	if (cond->ptc_mutex == NULL)
		cond->ptc_mutex = mutex;
	else
		if (cond->ptc_mutex != mutex) {
			pthread_spinunlock(self, &cond->ptc_lock);
			return EINVAL;
		}

	PTQ_INSERT_TAIL(&cond->ptc_waiters, self, pt_sleep);
	pthread_mutex_unlock(mutex);
	pthread__block(self, &cond->ptc_lock);
	/* Spinlock is unlocked on return */
	pthread_mutex_lock(mutex);

	return 0;
}


int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
    const struct timespec *abstime)
{

	return EINVAL;
}


int
pthread_cond_signal(pthread_cond_t *cond)
{
	pthread_t self, signaled;
#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC))
		return EINVAL;
#endif

	self = pthread__self();

	pthread_spinlock(self, &cond->ptc_lock);
	signaled = PTQ_FIRST(&cond->ptc_waiters);
	if (signaled != NULL)
		PTQ_REMOVE(&cond->ptc_waiters, signaled, pt_sleep);
	if (PTQ_EMPTY(&cond->ptc_waiters))
		cond->ptc_mutex = NULL;
	pthread_spinunlock(self, &cond->ptc_lock);

	if (signaled != NULL)
		pthread__sched(self, signaled);

	return 0;
}


int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	pthread_t self, signaled;
	struct pt_queue_t blockedq, nullq = PTQ_HEAD_INITIALIZER;
#ifdef ERRORCHECK
	if ((cond == NULL) || (cond->ptc_magic != _PT_COND_MAGIC))
		return EINVAL;
#endif

	self = pthread__self();

	pthread_spinlock(self, &cond->ptc_lock);
	blockedq = cond->ptc_waiters;
	cond->ptc_waiters = nullq;
	cond->ptc_mutex = NULL;
	pthread_spinunlock(self, &cond->ptc_lock);
	
	PTQ_FOREACH(signaled, &blockedq, pt_sleep)
	    pthread__sched(self, signaled);

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
