/*	$NetBSD: pthread_mutex.c,v 1.42 2008/01/25 01:09:18 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2003, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, by Jason R. Thorpe, and by Andrew Doran.
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
__RCSID("$NetBSD: pthread_mutex.c,v 1.42 2008/01/25 01:09:18 ad Exp $");

#include <sys/types.h>

#include <machine/lock.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "pthread.h"
#include "pthread_int.h"

#ifndef	PTHREAD__HAVE_ATOMIC

static int pthread_mutex_lock_slow(pthread_t, pthread_mutex_t *);
static void once_cleanup(void *);

int		_pthread_mutex_held_np(pthread_mutex_t *);
pthread_t	_pthread_mutex_owner_np(pthread_mutex_t *);

__weak_alias(pthread_mutex_held_np,_pthread_mutex_held_np)
__weak_alias(pthread_mutex_owner_np,_pthread_mutex_owner_np)

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

int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	struct mutexattr_private *map;
	struct mutex_private *mp;

	pthread__error(EINVAL, "Invalid mutex attribute",
	    (attr == NULL) || (attr->ptma_magic == _PT_MUTEXATTR_MAGIC));

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

	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);
	pthread__error(EBUSY, "Destroying locked mutex",
	    __SIMPLELOCK_UNLOCKED_P(&mutex->ptm_lock));

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
 * this behavior. This code relies on pthread__spintrylock() to issue
 * a barrier after obtaining a lock, and on pthread__spinunlock() to
 * issue a barrier before releasing a lock.
 */

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	pthread_t self;
	int error;

	self = pthread__self();

	/*
	 * Note that if we get the lock, we don't have to deal with any
	 * non-default lock type handling.
	 */
	if (__predict_false(pthread__spintrylock(self, &mutex->ptm_lock) == 0)) {
		error = pthread_mutex_lock_slow(self, mutex);
		if (error)
			return error;
	}

	/*
	 * We have the lock!
	 */
	mutex->ptm_owner = self;

	return 0;
}


static int
pthread_mutex_lock_slow(pthread_t self, pthread_mutex_t *mutex)
{
	extern int pthread__started;
	struct mutex_private *mp;
	sigset_t ss;
	int count;

	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);

	for (;;) {
		/* Spin for a while. */
		count = pthread__nspins;
		while (__SIMPLELOCK_LOCKED_P(&mutex->ptm_lock)  && --count > 0)
			pthread__smt_pause();
		if (count > 0) {
			if (pthread__spintrylock(self, &mutex->ptm_lock) != 0)
				break;
			continue;
		}

		/* Okay, didn't look free. Get the interlock... */
		pthread__spinlock(self, &mutex->ptm_interlock);

		/*
		 * The mutex_unlock routine will get the interlock
		 * before looking at the list of sleepers, so if the
		 * lock is held we can safely put ourselves on the
		 * sleep queue. If it's not held, we can try taking it
		 * again.
		 */
		PTQ_INSERT_HEAD(&mutex->ptm_blocked, self, pt_sleep);
		if (__SIMPLELOCK_UNLOCKED_P(&mutex->ptm_lock)) {
			PTQ_REMOVE(&mutex->ptm_blocked, self, pt_sleep);
			pthread__spinunlock(self, &mutex->ptm_interlock);
			continue;
		}

		mp = mutex->ptm_private;
		if (mutex->ptm_owner == self && mp != NULL) {
			switch (mp->type) {
			case PTHREAD_MUTEX_ERRORCHECK:
				PTQ_REMOVE(&mutex->ptm_blocked, self, pt_sleep);
				pthread__spinunlock(self, &mutex->ptm_interlock);
				return EDEADLK;

			case PTHREAD_MUTEX_RECURSIVE:
				/*
				 * It's safe to do this without
				 * holding the interlock, because
				 * we only modify it if we know we
				 * own the mutex.
				 */
				PTQ_REMOVE(&mutex->ptm_blocked, self, pt_sleep);
				pthread__spinunlock(self, &mutex->ptm_interlock);
				if (mp->recursecount == INT_MAX)
					return EAGAIN;
				mp->recursecount++;
				return 0;
			}
		}

		/*
		 * Locking a mutex is not a cancellation
		 * point, so we don't need to do the
		 * test-cancellation dance. We may get woken
		 * up spuriously by pthread_cancel or signals,
		 * but it's okay since we're just going to
		 * retry.
		 */
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &mutex->ptm_blocked;
		pthread__spinunlock(self, &mutex->ptm_interlock);
		(void)pthread__park(self, &mutex->ptm_interlock,
		    &mutex->ptm_blocked, NULL, 0, &mutex->ptm_blocked);
	}

	return 0;
}


int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	struct mutex_private *mp;
	pthread_t self;

	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);

	self = pthread__self();

	if (pthread__spintrylock(self, &mutex->ptm_lock) == 0) {
		/*
		 * These tests can be performed without holding the
		 * interlock because these fields are only modified
		 * if we know we own the mutex.
		 */
		mp = mutex->ptm_private;
		if (mp != NULL && mp->type == PTHREAD_MUTEX_RECURSIVE &&
		    mutex->ptm_owner == self) {
			if (mp->recursecount == INT_MAX)
				return EAGAIN;
			mp->recursecount++;
			return 0;
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
	pthread_t self;
	int weown;

	pthread__error(EINVAL, "Invalid mutex",
	    mutex->ptm_magic == _PT_MUTEX_MAGIC);

	/*
	 * These tests can be performed without holding the
	 * interlock because these fields are only modified
	 * if we know we own the mutex.
	 */
	self = pthread__self();
	weown = (mutex->ptm_owner == self);
	mp = mutex->ptm_private;

	if (mp == NULL) {
		if (__predict_false(!weown)) {
			pthread__error(EPERM, "Unlocking unlocked mutex",
			    (mutex->ptm_owner != 0));
			pthread__error(EPERM,
			    "Unlocking mutex owned by another thread", weown);
		}
	} else if (mp->type == PTHREAD_MUTEX_RECURSIVE) {
		if (!weown)
			return EPERM;
		if (mp->recursecount != 0) {
			mp->recursecount--;
			return 0;
		}
	} else if (mp->type == PTHREAD_MUTEX_ERRORCHECK) {
		if (!weown)
			return EPERM;
		if (__predict_false(!weown)) {
			pthread__error(EPERM, "Unlocking unlocked mutex",
			    (mutex->ptm_owner != 0));
			pthread__error(EPERM,
			    "Unlocking mutex owned by another thread", weown);
		}
	}

	mutex->ptm_owner = NULL;
	pthread__spinunlock(self, &mutex->ptm_lock);

	/*
	 * Do a double-checked locking dance to see if there are any
	 * waiters.  If we don't see any waiters, we can exit, because
	 * we've already released the lock. If we do see waiters, they
	 * were probably waiting on us... there's a slight chance that
	 * they are waiting on a different thread's ownership of the
	 * lock that happened between the unlock above and this
	 * examination of the queue; if so, no harm is done, as the
	 * waiter will loop and see that the mutex is still locked.
	 */
	pthread__spinlock(self, &mutex->ptm_interlock);
	pthread__unpark_all(self, &mutex->ptm_interlock, &mutex->ptm_blocked);
	return 0;
}

int
pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	struct mutexattr_private *map;

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

	pthread__error(EINVAL, "Invalid mutex attribute",
	    attr->ptma_magic == _PT_MUTEXATTR_MAGIC);

	attr->ptma_magic = _PT_MUTEXATTR_DEAD;
	if (attr->ptma_private != NULL)
		free(attr->ptma_private);

	return 0;
}


int
pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *typep)
{
	struct mutexattr_private *map;

	pthread__error(EINVAL, "Invalid mutex attribute",
	    attr->ptma_magic == _PT_MUTEXATTR_MAGIC);

	map = attr->ptma_private;

	*typep = map->type;

	return 0;
}


int
pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	struct mutexattr_private *map;

	pthread__error(EINVAL, "Invalid mutex attribute",
	    attr->ptma_magic == _PT_MUTEXATTR_MAGIC);

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


static void
once_cleanup(void *closure)
{

       pthread_mutex_unlock((pthread_mutex_t *)closure);
}


int
pthread_once(pthread_once_t *once_control, void (*routine)(void))
{

	if (once_control->pto_done == 0) {
		pthread_mutex_lock(&once_control->pto_mutex);
		pthread_cleanup_push(&once_cleanup, &once_control->pto_mutex);
		if (once_control->pto_done == 0) {
			routine();
			once_control->pto_done = 1;
		}
		pthread_cleanup_pop(1);
	}

	return 0;
}

int
pthread__mutex_deferwake(pthread_t thread, pthread_mutex_t *mutex)
{

	return mutex->ptm_owner == thread;
}

int
_pthread_mutex_held_np(pthread_mutex_t *mutex)
{

	return mutex->ptm_owner == pthread__self();
}

pthread_t
_pthread_mutex_owner_np(pthread_mutex_t *mutex)
{

	return (pthread_t)mutex->ptm_owner;
}

#endif	/* !PTHREAD__HAVE_ATOMIC */
