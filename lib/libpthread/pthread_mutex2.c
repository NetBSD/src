/*	$NetBSD: pthread_mutex2.c,v 1.2 2007/09/08 22:49:50 ad Exp $	*/

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
__RCSID("$NetBSD: pthread_mutex2.c,v 1.2 2007/09/08 22:49:50 ad Exp $");

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pthread.h"
#include "pthread_int.h"

#ifdef	PTHREAD__HAVE_ATOMIC

/*
 * Note that it's important to use the address of ptm_waiters as
 * the list head in order for the hint arguments to _lwp_park /
 * _lwp_unpark_all to match.
 */
#define	pt_nextwaiter		pt_sleep.ptqe_next
#define	ptm_waiters		ptm_blocked.ptqh_first

#define	MUTEX_WAITERS_BIT	(1UL)

#define	MUTEX_HAS_WAITERS(x)	((uintptr_t)(x) & MUTEX_WAITERS_BIT)
#define	MUTEX_OWNER(x)		((uintptr_t)(x) & ~MUTEX_WAITERS_BIT)

static void	pthread__mutex_wakeup(pthread_t, pthread_mutex_t *);
static int	pthread__mutex_lock_slow(pthread_mutex_t *, void *);

__strong_alias(__libc_mutex_init,pthread_mutex_init)
__strong_alias(__libc_mutex_lock,pthread_mutex_lock)
__strong_alias(__libc_mutex_trylock,pthread_mutex_trylock)
__strong_alias(__libc_mutex_unlock,pthread_mutex_unlock)
__strong_alias(__libc_mutex_destroy,pthread_mutex_destroy)

__strong_alias(__libc_mutexattr_init,pthread_mutexattr_init)
__strong_alias(__libc_mutexattr_destroy,pthread_mutexattr_destroy)
__strong_alias(__libc_mutexattr_settype,pthread_mutexattr_settype)

__strong_alias(__libc_thr_once,pthread_once)

static inline int
mutex_cas(volatile void *ptr, void **old, void *new)
{

	return pthread__atomic_cas_ptr(ptr, old, new);
}
	
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
pthread_mutex_init(pthread_mutex_t *ptm, const pthread_mutexattr_t *attr)
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

	ptm->ptm_magic = _PT_MUTEX_MAGIC;
	ptm->ptm_owner = NULL;
	ptm->ptm_waiters = NULL;
	pthread_lockinit(&ptm->ptm_lock);
	ptm->ptm_private = mp;

	return 0;
}


int
pthread_mutex_destroy(pthread_mutex_t *ptm)
{

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);
	pthread__error(EBUSY, "Destroying locked mutex",
	    ptm->ptm_owner == NULL);

	ptm->ptm_magic = _PT_MUTEX_DEAD;
	if (ptm->ptm_private != NULL &&
	    ptm->ptm_private != (const void *)&mutex_private_default)
		free(ptm->ptm_private);

	return 0;
}


/*
 * Note regarding memory visibility: Pthreads has rules about memory
 * visibility and mutexes. Very roughly: Memory a thread can see when
 * it unlocks a mutex can be seen by another thread that locks the
 * same mutex.
 * 
 * A memory barrier after a lock and before an unlock will provide
 * this behavior. This code relies on mutex_cas() to issue a barrier
 * after obtaining a lock, and on pthread__simple_unlock() to issue
 * a barrier before releasing a lock.
 */

int
pthread_mutex_lock(pthread_mutex_t *ptm)
{
	void *owner;
	pthread_t self;

	owner = NULL;
	self = pthread__self();
	if (__predict_true(mutex_cas(&ptm->ptm_owner, &owner, self)))
		return 0;
	return pthread__mutex_lock_slow(ptm, owner);
}

#if __GNUC_PREREQ__(3, 0)
__attribute ((noinline))
#endif
static int
pthread__mutex_lock_slow(pthread_mutex_t *ptm, void *owner)
{
	extern int pthread__started;
	struct mutex_private *mp;
	void *waiters, *new;
	pthread_t self;
	sigset_t ss;
	int count;

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);

	if (pthread__started == 0) {
		/* The spec says we must deadlock, so... */
		mp = ptm->ptm_private;
		pthread__assert(mp->type == PTHREAD_MUTEX_NORMAL);
		(void) sigprocmask(SIG_SETMASK, NULL, &ss);
		for (;;) {
			sigsuspend(&ss);
		}
		/*NOTREACHED*/
	}

	/* Recursive or errorcheck? */
	mp = ptm->ptm_private;
	self = pthread__self();
	if (MUTEX_OWNER(owner) == (uintptr_t)self && mp != NULL) {
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

	/* Spin for a while. */
	count = pthread__nspins;
	while (owner != NULL && --count > 0) {
		pthread__smt_pause();
		owner = ptm->ptm_owner;
	}

	for (;; owner = ptm->ptm_owner) {
		/* If it has become free, try to acquire it again. */
		while (owner == NULL) {
			if (mutex_cas(&ptm->ptm_owner, &owner, self))
				return 0;
		}

		/*
		 * Nope: still held.  Add us to the list of waiters.
		 * Issue memory barrier to ensure sleeponq/nextwaiter
		 * are visible before we enter the waiters list.
		 */
		self->pt_sleeponq = 1;
		for (waiters = ptm->ptm_waiters;;) {
			self->pt_nextwaiter = waiters;
			pthread__membar_producer();
			if (mutex_cas(&ptm->ptm_waiters, &waiters, self))
			    	break;
		}

		/*
		 * Set the waiters bit and block.
		 *
		 * Note that the mutex can become unlocked before we set
		 * the waiters bit.  If that happens it's not safe to sleep
		 * as we may never be awoken: we must remove the current
		 * thread from the waiters list and try again.
		 *
		 * Because we are doing this atomically, we can't remove
		 * one waiter: we must remove all waiters and awken them,
		 * then sleep in _lwp_park() until we have been awoken. 
		 *
		 * Issue a memory barrier to ensure that we are reading
		 * the value of ptm_owner/pt_sleeponq after we have entered
		 * the waiters list (the CAS itself must be atomic).
		 */
		pthread__membar_consumer();
		for (owner = ptm->ptm_owner;;) {
			if (MUTEX_HAS_WAITERS(owner))
				break;
			if (owner == NULL) {
				pthread__mutex_wakeup(self, ptm);
				break;
			}
			new = (void *)((uintptr_t)owner | MUTEX_WAITERS_BIT);
			if (mutex_cas(&ptm->ptm_owner, &owner, new))
				break;
		}

		/*
		 * We may be awoken by this thread, or some other thread.
		 * The key requirement is that we must not proceed until
		 * told that we are no longer waiting (via pt_sleeponq
		 * being set to zero),  Otherwise it is unsafe to re-enter
		 * the thread onto the waiters list.
		 */
		while (self->pt_sleeponq) {
			(void)_lwp_park(NULL, 0, &ptm->ptm_waiters, NULL);
		}
	}
}

int
pthread_mutex_trylock(pthread_mutex_t *ptm)
{
	struct mutex_private *mp;
	pthread_t self;
	void *value;

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);

	self = pthread__self();

	value = NULL;
	if (mutex_cas(&ptm->ptm_owner, &value, self))
		return 0;

	/*
	 * These tests can be performed without holding the
	 * interlock because these fields are only modified
	 * if we know we own the mutex.
	 */
	mp = ptm->ptm_private;
	if (mp != NULL && mp->type == PTHREAD_MUTEX_RECURSIVE &&
	    ptm->ptm_owner == self) {
		if (mp->recursecount == INT_MAX)
			return EAGAIN;
		mp->recursecount++;
		return 0;
	}
	return EBUSY;
}


int
pthread_mutex_unlock(pthread_mutex_t *ptm)
{
	struct mutex_private *mp;
	pthread_t self, owner;
	int weown;

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);

	/*
	 * These tests can be performed without holding the
	 * interlock because these fields are only modified
	 * if we know we own the mutex.
	 */
	self = pthread_self();
	weown = (MUTEX_OWNER(ptm->ptm_owner) == (uintptr_t)self);
	mp = ptm->ptm_private;

	if (mp == NULL) {
		if (__predict_false(!weown)) {
			pthread__error(EPERM, "Unlocking unlocked mutex",
			    (ptm->ptm_owner != 0));
			pthread__error(EPERM,
			    "Unlocking mutex owned by another thread", weown);
		}
	} else if (mp->type == PTHREAD_MUTEX_RECURSIVE) {
		if (!weown)
			return EPERM;
		if (mp->recursecount != 0) {
			mp->recursecount--;
			/*
			 * Wake any threads we deferred waking until
			 * mutex unlock.
			 */
			if (self->pt_nwaiters != 0) {
				(void)_lwp_unpark_all(self->pt_waiters,
				    self->pt_nwaiters, &ptm->ptm_waiters);
				self->pt_nwaiters = 0;
			}
			return 0;
		}
	} else if (mp->type == PTHREAD_MUTEX_ERRORCHECK) {
		if (!weown)
			return EPERM;
		if (__predict_false(!weown)) {
			pthread__error(EPERM, "Unlocking unlocked mutex",
			    (ptm->ptm_owner != 0));
			pthread__error(EPERM,
			    "Unlocking mutex owned by another thread", weown);
		}
	}

	/*
	 * Release the mutex.  If there appear to be waiters, then
	 * wake them up.
	 */
	owner = pthread__atomic_swap_ptr(&ptm->ptm_owner, NULL);
	if (MUTEX_HAS_WAITERS(owner) != 0) {
		pthread__mutex_wakeup(self, ptm);
		return 0;
	}

	/*
	 * There were no waiters, but we may have deferred waking
	 * other threads until mutex unlock - we must wake them now.
	 */
	if (self->pt_nwaiters != 0) {
		(void)_lwp_unpark_all(self->pt_waiters, self->pt_nwaiters,
		    &ptm->ptm_waiters);
		self->pt_nwaiters = 0;
	}

	return 0;
}

static void
pthread__mutex_wakeup(pthread_t self, pthread_mutex_t *ptm)
{
	pthread_t thread, next;
	ssize_t n, rv;

	/*
	 * Take ownership of the current set of waiters.  No
	 * need for a memory barrier following this, all loads
	 * are dependent upon 'thread'.
	 */
	thread = pthread__atomic_swap_ptr(&ptm->ptm_waiters, NULL);

	for (;; n = 0) {
		/*
		 * Pull waiters from the queue and add to our list.
		 * Use a memory barrier to ensure that we safely
		 * read the value of pt_nextwaiter before 'thread'
		 * sees pt_sleeponq being cleared.
		 */
		for (n = self->pt_nwaiters, self->pt_nwaiters = 0;
		    n < pthread__unpark_max && thread != NULL;
		    thread = next) {
		    	next = thread->pt_nextwaiter;
			self->pt_waiters[n++] = thread->pt_lid;
			pthread__membar_full();
			thread->pt_sleeponq = 0;
			/* No longer safe to touch 'thread' */
		}

		switch (n) {
		case 0:
			return;
		case 1:
			/*
			 * If the calling thread is about to block,
			 * defer unparking the target until _lwp_park()
			 * is called.
			 */
			if (self->pt_willpark && self->pt_unpark == 0) {
				self->pt_unpark = self->pt_waiters[0];
				self->pt_unparkhint = &ptm->ptm_waiters;
				return;
			}
			rv = (ssize_t)_lwp_unpark(self->pt_waiters[0],
			    &ptm->ptm_waiters);
			if (rv != 0 && errno != EALREADY && errno != EINTR) {
				pthread__errorfunc(__FILE__, __LINE__,
				    __func__, "_lwp_unpark failed");
			}
			return;
		default:
			rv = _lwp_unpark_all(self->pt_waiters, (size_t)n,
			    &ptm->ptm_waiters);
			if (rv != 0 && errno != EINTR) {
				pthread__errorfunc(__FILE__, __LINE__,
				    __func__, "_lwp_unpark_all failed");
			}
			break;
		}
	}
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
pthread__mutex_owned(pthread_t thread, pthread_mutex_t *ptm)
{

	return MUTEX_OWNER(ptm->ptm_owner) == (uintptr_t)thread;
}

#endif	/* PTHREAD__HAVE_ATOMIC */
