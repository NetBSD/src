/*	$NetBSD: pthread_mutex.c,v 1.6 2003/01/22 13:49:14 scw Exp $	*/

/*-
 * Copyright (c) 2001, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, and by Jason R. Thorpe.
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
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "pthread.h"
#include "pthread_int.h"

static int pthread_mutex_lock_slow(pthread_mutex_t *);

__strong_alias(__libc_mutex_init,pthread_mutex_init)
__strong_alias(__libc_mutex_lock,pthread_mutex_lock)
__strong_alias(__libc_mutex_trylock,pthread_mutex_trylock)
__strong_alias(__libc_mutex_unlock,pthread_mutex_unlock)
__strong_alias(__libc_mutex_destroy,pthread_mutex_destroy)

__strong_alias(__libc_mutexattr_init,pthread_mutexattr_init)
__strong_alias(__libc_mutexattr_destroy,pthread_mutexattr_destroy)
__strong_alias(__libc_mutexattr_settype,pthread_mutexattr_settype)

__strong_alias(__libc_thr_once,pthread_once)

struct mutex_private {
	int	type;
	int	recursecount;
};

static const struct mutex_private mutex_private_default = {
	PTHREAD_MUTEX_DEFAULT,
	0,
};

struct mutexattr_private {
	int	type;
};

static const struct mutexattr_private mutexattr_private_default = {
	PTHREAD_MUTEX_DEFAULT,
};

/*
 * If the mutex does not already have private data (i.e. was statically
 * initialized), then give it the default.
 */
#define	GET_MUTEX_PRIVATE(mutex, mp)					\
do {									\
	if (__predict_false((mp = (mutex)->ptm_private) == NULL)) {	\
		/* LINTED cast away const */				\
		mp = ((mutex)->ptm_private =				\
		    (void *)&mutex_private_default);			\
	}								\
} while (/*CONSTCOND*/0)

int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	struct mutexattr_private *map;
	struct mutex_private *mp;

#ifdef ERRORCHECK
	if ((mutex == NULL) || 
	    (attr && (attr->ptma_magic != _PT_MUTEXATTR_MAGIC)))
		return EINVAL;
#endif

	if (attr != NULL && (map = attr->ptma_private) != NULL &&
	    memcmp(map, &mutexattr_private_default, sizeof(*map)) != 0) {
		mp = malloc(sizeof(*mp));
		if (mp == NULL)
			return ENOMEM;

		mp->type = map->type;
		mp->recursecount = 0;
	} else {
		/* LINTED cast away const */
		mp = (struct mutex_private *) &mutex_private_default;
	}

	mutex->ptm_magic = _PT_MUTEX_MAGIC;
	mutex->ptm_owner = NULL;
	pthread_lockinit(&mutex->ptm_lock);
	pthread_lockinit(&mutex->ptm_interlock);
	PTQ_INIT(&mutex->ptm_blocked);
	mutex->ptm_private = mp;

	return 0;
}


int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{

#ifdef ERRORCHECK
	if ((mutex == NULL) ||
	    (mutex->ptm_magic != _PT_MUTEX_MAGIC) ||
	    (mutex->ptm_lock != __SIMPLELOCK_UNLOCKED))
		return EINVAL;
#endif

	mutex->ptm_magic = _PT_MUTEX_DEAD;
	if (mutex->ptm_private != NULL &&
	    mutex->ptm_private != (const void *)&mutex_private_default)
		free(mutex->ptm_private);

	return 0;
}


/*
 * Note regarding memory visibility: Pthreads has rules about memory
 * visibility and mutexes. Very roughly: Memory a thread can see when
 * it unlocks a mutex can be seen by another thread that locks the
 * same mutex.
 * 
 * A memory barrier after a lock and before an unlock will provide
 * this behavior. This code relies on pthread__simple_lock_try() to issue
 * a barrier after obtaining a lock, and on pthread__simple_unlock() to
 * issue a barrier before releasing a lock.
 */

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	int error;

#ifdef ERRORCHECK
	if ((mutex == NULL) || (mutex->ptm_magic != _PT_MUTEX_MAGIC))
		return EINVAL;
#endif

	/*
	 * Note that if we get the lock, we don't have to deal with any
	 * non-default lock type handling.
	 */
	if (__predict_false(pthread__simple_lock_try(&mutex->ptm_lock) == 0)) {
		error = pthread_mutex_lock_slow(mutex);
		if (error)
			return error;
	}

	/* We have the lock! */
	mutex->ptm_owner = pthread__self();

	return 0;
}


static int
pthread_mutex_lock_slow(pthread_mutex_t *mutex)
{
	pthread_t self;

	self = pthread__self();

	while (/*CONSTCOND*/1) {
		if (pthread__simple_lock_try(&mutex->ptm_lock))
			break; /* got it! */
		
		/* Okay, didn't look free. Get the interlock... */
		pthread_spinlock(self, &mutex->ptm_interlock);
		/*
		 * The mutex_unlock routine will get the interlock
		 * before looking at the list of sleepers, so if the
		 * lock is held we can safely put ourselves on the
		 * sleep queue. If it's not held, we can try taking it
		 * again.
		 */
		if (mutex->ptm_lock == __SIMPLELOCK_LOCKED) {
			struct mutex_private *mp;

			GET_MUTEX_PRIVATE(mutex, mp);

			if (mutex->ptm_owner == self) {
				switch (mp->type) {
				case PTHREAD_MUTEX_ERRORCHECK:
					pthread_spinunlock(self,
					    &mutex->ptm_interlock);
					return EDEADLK;

				case PTHREAD_MUTEX_RECURSIVE:
					/*
					 * It's safe to do this without
					 * holding the interlock, because
					 * we only modify it if we know we
					 * own the mutex.
					 */
					pthread_spinunlock(self,
					    &mutex->ptm_interlock);
					if (mp->recursecount == INT_MAX)
						return EAGAIN;
					mp->recursecount++;
					return 0;
				}
			}

			PTQ_INSERT_TAIL(&mutex->ptm_blocked, self, pt_sleep);
			/*
			 * Locking a mutex is not a cancellation
			 * point, so we don't need to do the
			 * test-cancellation dance. We may get woken
			 * up spuriously by pthread_cancel, though,
			 * but it's okay since we're just going to
			 * retry.
			 */
			pthread_spinlock(self, &self->pt_statelock);
			self->pt_state = PT_STATE_BLOCKED_QUEUE;
			self->pt_sleepobj = mutex;
			self->pt_sleepq = &mutex->ptm_blocked;
			self->pt_sleeplock = &mutex->ptm_interlock;
			pthread_spinunlock(self, &self->pt_statelock);

			pthread__block(self, &mutex->ptm_interlock);
			/* interlock is not held when we return */
		} else {
			pthread_spinunlock(self, &mutex->ptm_interlock);
		}
		/* Go around for another try. */
	}

	return 0;
}


int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	pthread_t self = pthread__self();

#ifdef ERRORCHECK
	if ((mutex == NULL) || (mutex->ptm_magic != _PT_MUTEX_MAGIC))
		return EINVAL;
#endif

	if (pthread__simple_lock_try(&mutex->ptm_lock) == 0) {
		struct mutex_private *mp;

		GET_MUTEX_PRIVATE(mutex, mp);

		/*
		 * These tests can be performed without holding the
		 * interlock because these fields are only modified
		 * if we know we own the mutex.
		 */
		if (mutex->ptm_owner == self) {
			switch (mp->type) {
			case PTHREAD_MUTEX_ERRORCHECK:
				return EDEADLK;

			case PTHREAD_MUTEX_RECURSIVE:
				if (mp->recursecount == INT_MAX)
					return EAGAIN;
				mp->recursecount++;
				return 0;
			}
		}

		return EBUSY;
	}

	mutex->ptm_owner = self;

	return 0;
}


int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	struct mutex_private *mp;
	pthread_t self, blocked; 

	self = pthread__self();

#ifdef ERRORCHECK
	if ((mutex == NULL) || (mutex->ptm_magic != _PT_MUTEX_MAGIC))
		return EINVAL;

	if (mutex->ptm_lock != __SIMPLELOCK_LOCKED)
		return EPERM; /* Not exactly the right error. */
#endif

	GET_MUTEX_PRIVATE(mutex, mp);

	/*
	 * These tests can be performed without holding the
	 * interlock because these fields are only modified
	 * if we know we own the mutex.
	 */
	switch (mp->type) {
	case PTHREAD_MUTEX_ERRORCHECK:
		if (mutex->ptm_owner != self)
			return EPERM;
		break;

	case PTHREAD_MUTEX_RECURSIVE:
		if (mutex->ptm_owner != self)
			return EPERM;
		if (mp->recursecount != 0) {
			mp->recursecount--;
			return 0;
		}
		break;
	}

	pthread_spinlock(self, &mutex->ptm_interlock);
	blocked = PTQ_FIRST(&mutex->ptm_blocked);
	if (blocked)
		PTQ_REMOVE(&mutex->ptm_blocked, blocked, pt_sleep);
	mutex->ptm_owner = NULL;
	pthread__simple_unlock(&mutex->ptm_lock);
	pthread_spinunlock(self, &mutex->ptm_interlock);

	/* Give the head of the blocked queue another try. */
	if (blocked)
		pthread__sched(self, blocked);

	return 0;
}

int
pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	struct mutexattr_private *map;

#ifdef ERRORCHECK
	if (attr == NULL)
		return EINVAL;
#endif

	map = malloc(sizeof(*map));
	if (map == NULL)
		return ENOMEM;

	*map = mutexattr_private_default;

	attr->ptma_magic = _PT_MUTEXATTR_MAGIC;
	attr->ptma_private = map;

	return 0;
}


int
pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{

#ifdef ERRORCHECK
	if ((attr == NULL) ||
	    (attr->ptma_magic != _PT_MUTEXATTR_MAGIC))
		return EINVAL;
#endif

	attr->ptma_magic = _PT_MUTEXATTR_DEAD;
	if (attr->ptma_private != NULL)
		free(attr->ptma_private);

	return 0;
}


int
pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *typep)
{
	struct mutexattr_private *map;

#ifdef ERRORCHECK
	if ((attr == NULL) ||
	    (attr->ptma_magic != _PT_MUTEXATTR_MAGIC) ||
	    (typep == NULL))
		return EINVAL;
#endif

	map = attr->ptma_private;

	*typep = map->type;

	return 0;
}


int
pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	struct mutexattr_private *map;

#ifdef ERRORCHECK
	if ((attr == NULL) ||
	    (attr->ptma_magic != _PT_MUTEXATTR_MAGIC))
		return EINVAL;
#endif
	map = attr->ptma_private;

	switch (type) {
	case PTHREAD_MUTEX_NORMAL:
	case PTHREAD_MUTEX_ERRORCHECK:
	case PTHREAD_MUTEX_RECURSIVE:
		map->type = type;
		break;

	default:
		return EINVAL;
	}

	return 0;
}


int
pthread_once(pthread_once_t *once_control, void (*routine)(void))
{

	if (once_control->pto_done == 0) {
		pthread_mutex_lock(&once_control->pto_mutex);
		if (once_control->pto_done == 0) {
			routine();
			once_control->pto_done = 1;
		}
		pthread_mutex_unlock(&once_control->pto_mutex);
	}

	return 0;
}
