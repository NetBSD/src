/*	$NetBSD: pthread_rwlock.c,v 1.44 2022/02/12 14:59:32 riastradh Exp $ */

/*-
 * Copyright (c) 2002, 2006, 2007, 2008, 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_rwlock.c,v 1.44 2022/02/12 14:59:32 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

#include <sys/types.h>
#include <sys/lwpctl.h>

#include <assert.h>
#include <time.h>
#include <errno.h>
#include <stddef.h>

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"

#define	_RW_LOCKED		0
#define	_RW_WANT_WRITE		1
#define	_RW_WANT_READ		2

#if __GNUC_PREREQ__(3, 0)
#define	NOINLINE		__attribute ((noinline))
#else
#define	NOINLINE		/* nothing */
#endif

static int pthread__rwlock_wrlock(pthread_rwlock_t *, const struct timespec *);
static int pthread__rwlock_rdlock(pthread_rwlock_t *, const struct timespec *);
static void pthread__rwlock_early(pthread_t, pthread_rwlock_t *,
    pthread_mutex_t *);

int	_pthread_rwlock_held_np(pthread_rwlock_t *);
int	_pthread_rwlock_rdheld_np(pthread_rwlock_t *);
int	_pthread_rwlock_wrheld_np(pthread_rwlock_t *);

#ifndef lint
__weak_alias(pthread_rwlock_held_np,_pthread_rwlock_held_np)
__weak_alias(pthread_rwlock_rdheld_np,_pthread_rwlock_rdheld_np)
__weak_alias(pthread_rwlock_wrheld_np,_pthread_rwlock_wrheld_np)
#endif

__strong_alias(__libc_rwlock_init,pthread_rwlock_init)
__strong_alias(__libc_rwlock_rdlock,pthread_rwlock_rdlock)
__strong_alias(__libc_rwlock_wrlock,pthread_rwlock_wrlock)
__strong_alias(__libc_rwlock_tryrdlock,pthread_rwlock_tryrdlock)
__strong_alias(__libc_rwlock_trywrlock,pthread_rwlock_trywrlock)
__strong_alias(__libc_rwlock_unlock,pthread_rwlock_unlock)
__strong_alias(__libc_rwlock_destroy,pthread_rwlock_destroy)

static inline uintptr_t
rw_cas(pthread_rwlock_t *ptr, uintptr_t o, uintptr_t n)
{

	return (uintptr_t)atomic_cas_ptr(&ptr->ptr_owner, (void *)o,
	    (void *)n);
}

int
pthread_rwlock_init(pthread_rwlock_t *ptr,
	    const pthread_rwlockattr_t *attr)
{
	if (__predict_false(__uselibcstub))
		return __libc_rwlock_init_stub(ptr, attr);

	pthread__error(EINVAL, "Invalid rwlock attribute",
	    attr == NULL || attr->ptra_magic == _PT_RWLOCKATTR_MAGIC);

	ptr->ptr_magic = _PT_RWLOCK_MAGIC;
	PTQ_INIT(&ptr->ptr_rblocked);
	PTQ_INIT(&ptr->ptr_wblocked);
	ptr->ptr_nreaders = 0;
	ptr->ptr_owner = NULL;

	return 0;
}


int
pthread_rwlock_destroy(pthread_rwlock_t *ptr)
{
	if (__predict_false(__uselibcstub))
		return __libc_rwlock_destroy_stub(ptr);

	pthread__error(EINVAL, "Invalid rwlock",
	    ptr->ptr_magic == _PT_RWLOCK_MAGIC);

	if ((!PTQ_EMPTY(&ptr->ptr_rblocked)) ||
	    (!PTQ_EMPTY(&ptr->ptr_wblocked)) ||
	    (ptr->ptr_nreaders != 0) ||
	    (ptr->ptr_owner != NULL))
		return EINVAL;
	ptr->ptr_magic = _PT_RWLOCK_DEAD;

	return 0;
}

/* We want function call overhead. */
NOINLINE static void
pthread__rwlock_pause(void)
{

	pthread__smt_pause();
}

NOINLINE static int
pthread__rwlock_spin(uintptr_t owner)
{
	pthread_t thread;
	unsigned int i;

	if ((owner & ~RW_THREAD) != RW_WRITE_LOCKED)
		return 0;

	thread = (pthread_t)(owner & RW_THREAD);
	if (__predict_false(thread == NULL) ||
	    thread->pt_lwpctl->lc_curcpu == LWPCTL_CPU_NONE)
		return 0;

	for (i = 128; i != 0; i--)
		pthread__rwlock_pause();
	return 1;
}

static int
pthread__rwlock_rdlock(pthread_rwlock_t *ptr, const struct timespec *ts)
{
	uintptr_t owner, next;
	pthread_mutex_t *interlock;
	pthread_t self;
	int error;

	pthread__error(EINVAL, "Invalid rwlock",
	    ptr->ptr_magic == _PT_RWLOCK_MAGIC);

	for (owner = (uintptr_t)ptr->ptr_owner;; owner = next) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		if ((owner & (RW_WRITE_LOCKED | RW_WRITE_WANTED)) == 0) {
			next = rw_cas(ptr, owner, owner + RW_READ_INCR);
			if (owner == next) {
				/* Got it! */
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
				membar_enter();
#endif
				return 0;
			}

			/*
			 * Didn't get it -- spin around again (we'll
			 * probably sleep on the next iteration).
			 */
			continue;
		}

		self = pthread__self();
		if ((owner & RW_THREAD) == (uintptr_t)self)
			return EDEADLK;

		/* If held write locked and no waiters, spin. */
		if (pthread__rwlock_spin(owner)) {
			while (pthread__rwlock_spin(owner)) {
				owner = (uintptr_t)ptr->ptr_owner;
			}
			next = owner;
			continue;
		}

		/*
		 * Grab the interlock.  Once we have that, we
		 * can adjust the waiter bits and sleep queue.
		 */
		interlock = pthread__hashlock(ptr);
		pthread_mutex_lock(interlock);

		/*
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 */
		next = rw_cas(ptr, owner, owner | RW_HAS_WAITERS);
		if (owner != next) {
			pthread_mutex_unlock(interlock);
			continue;
		}

		/* The waiters bit is set - it's safe to sleep. */
	    	PTQ_INSERT_HEAD(&ptr->ptr_rblocked, self, pt_sleep);
	    	ptr->ptr_nreaders++;
		self->pt_rwlocked = _RW_WANT_READ;
		self->pt_sleepobj = &ptr->ptr_rblocked;
		error = pthread__park(self, interlock, &ptr->ptr_rblocked,
		    ts, 0);

		if (self->pt_sleepobj != NULL) {
			pthread__rwlock_early(self, ptr, interlock);
		}

		/* Did we get the lock? */
		if (self->pt_rwlocked == _RW_LOCKED) {
			membar_enter();
			return 0;
		}
		if (error != 0)
			return error;

		pthread__errorfunc(__FILE__, __LINE__, __func__,
		    "direct handoff failure");
	}
}


int
pthread_rwlock_tryrdlock(pthread_rwlock_t *ptr)
{
	uintptr_t owner, next;

	if (__predict_false(__uselibcstub))
		return __libc_rwlock_tryrdlock_stub(ptr);

	pthread__error(EINVAL, "Invalid rwlock",
	    ptr->ptr_magic == _PT_RWLOCK_MAGIC);

	/*
	 * Don't get a readlock if there is a writer or if there are waiting
	 * writers; i.e. prefer writers to readers. This strategy is dictated
	 * by SUSv3.
	 */
	for (owner = (uintptr_t)ptr->ptr_owner;; owner = next) {
		if ((owner & (RW_WRITE_LOCKED | RW_WRITE_WANTED)) != 0)
			return EBUSY;
		next = rw_cas(ptr, owner, owner + RW_READ_INCR);
		if (owner == next) {
			/* Got it! */
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
			membar_enter();
#endif
			return 0;
		}
	}
}

static int
pthread__rwlock_wrlock(pthread_rwlock_t *ptr, const struct timespec *ts)
{
	uintptr_t owner, next;
	pthread_mutex_t *interlock;
	pthread_t self;
	int error;

	self = pthread__self();
	_DIAGASSERT(((uintptr_t)self & RW_FLAGMASK) == 0);

	pthread__error(EINVAL, "Invalid rwlock",
	    ptr->ptr_magic == _PT_RWLOCK_MAGIC);

	for (owner = (uintptr_t)ptr->ptr_owner;; owner = next) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		if ((owner & RW_THREAD) == 0) {
			next = rw_cas(ptr, owner,
			    (uintptr_t)self | RW_WRITE_LOCKED);
			if (owner == next) {
				/* Got it! */
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
				membar_enter();
#endif
				return 0;
			}

			/*
			 * Didn't get it -- spin around again (we'll
			 * probably sleep on the next iteration).
			 */
			continue;
		}

		if ((owner & RW_THREAD) == (uintptr_t)self)
			return EDEADLK;

		/* If held write locked and no waiters, spin. */
		if (pthread__rwlock_spin(owner)) {
			while (pthread__rwlock_spin(owner)) {
				owner = (uintptr_t)ptr->ptr_owner;
			}
			next = owner;
			continue;
		}

		/*
		 * Grab the interlock.  Once we have that, we
		 * can adjust the waiter bits and sleep queue.
		 */
		interlock = pthread__hashlock(ptr);
		pthread_mutex_lock(interlock);

		/*
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 */
		next = rw_cas(ptr, owner,
		    owner | RW_HAS_WAITERS | RW_WRITE_WANTED);
		if (owner != next) {
			pthread_mutex_unlock(interlock);
			continue;
		}

		/* The waiters bit is set - it's safe to sleep. */
	    	PTQ_INSERT_TAIL(&ptr->ptr_wblocked, self, pt_sleep);
		self->pt_rwlocked = _RW_WANT_WRITE;
		self->pt_sleepobj = &ptr->ptr_wblocked;
		error = pthread__park(self, interlock, &ptr->ptr_wblocked,
		    ts, 0);

		if (self->pt_sleepobj != NULL) {
			pthread__rwlock_early(self, ptr, interlock);
		}

		/* Did we get the lock? */
		if (self->pt_rwlocked == _RW_LOCKED) {
			membar_enter();
			return 0;
		}
		if (error != 0)
			return error;

		pthread__errorfunc(__FILE__, __LINE__, __func__,
		    "direct handoff failure: %d", errno);
	}
}

int
pthread_rwlock_trywrlock(pthread_rwlock_t *ptr)
{
	uintptr_t owner, next;
	pthread_t self;

	if (__predict_false(__uselibcstub))
		return __libc_rwlock_trywrlock_stub(ptr);

	pthread__error(EINVAL, "Invalid rwlock",
	    ptr->ptr_magic == _PT_RWLOCK_MAGIC);

	self = pthread__self();
	_DIAGASSERT(((uintptr_t)self & RW_FLAGMASK) == 0);

	for (owner = (uintptr_t)ptr->ptr_owner;; owner = next) {
		if (owner != 0)
			return EBUSY;
		next = rw_cas(ptr, owner, (uintptr_t)self | RW_WRITE_LOCKED);
		if (owner == next) {
			/* Got it! */
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
			membar_enter();
#endif
			return 0;
		}
	}
}

int
pthread_rwlock_rdlock(pthread_rwlock_t *ptr)
{
	if (__predict_false(__uselibcstub))
		return __libc_rwlock_rdlock_stub(ptr);

	return pthread__rwlock_rdlock(ptr, NULL);
}

int
pthread_rwlock_timedrdlock(pthread_rwlock_t *ptr,
			   const struct timespec *abs_timeout)
{
	if (abs_timeout == NULL)
		return EINVAL;
	if ((abs_timeout->tv_nsec >= 1000000000) ||
	    (abs_timeout->tv_nsec < 0) ||
	    (abs_timeout->tv_sec < 0))
		return EINVAL;

	return pthread__rwlock_rdlock(ptr, abs_timeout);
}

int
pthread_rwlock_wrlock(pthread_rwlock_t *ptr)
{
	if (__predict_false(__uselibcstub))
		return __libc_rwlock_wrlock_stub(ptr);

	return pthread__rwlock_wrlock(ptr, NULL);
}

int
pthread_rwlock_timedwrlock(pthread_rwlock_t *ptr,
			   const struct timespec *abs_timeout)
{
	if (abs_timeout == NULL)
		return EINVAL;
	if ((abs_timeout->tv_nsec >= 1000000000) ||
	    (abs_timeout->tv_nsec < 0) ||
	    (abs_timeout->tv_sec < 0))
		return EINVAL;

	return pthread__rwlock_wrlock(ptr, abs_timeout);
}


int
pthread_rwlock_unlock(pthread_rwlock_t *ptr)
{
	uintptr_t owner, decr, new, next;
	pthread_mutex_t *interlock;
	pthread_t self, thread;

	if (__predict_false(__uselibcstub))
		return __libc_rwlock_unlock_stub(ptr);

	pthread__error(EINVAL, "Invalid rwlock",
	    ptr->ptr_magic == _PT_RWLOCK_MAGIC);

#ifndef PTHREAD__ATOMIC_IS_MEMBAR
	membar_exit();
#endif

	/*
	 * Since we used an add operation to set the required lock
	 * bits, we can use a subtract to clear them, which makes
	 * the read-release and write-release path similar.
	 */
	owner = (uintptr_t)ptr->ptr_owner;
	if ((owner & RW_WRITE_LOCKED) != 0) {
		self = pthread__self();
		decr = (uintptr_t)self | RW_WRITE_LOCKED;
		if ((owner & RW_THREAD) != (uintptr_t)self) {
			return EPERM;
		}
	} else {
		decr = RW_READ_INCR;
		if (owner == 0) {
			return EPERM;
		}
	}

	for (;; owner = next) {
		/*
		 * Compute what we expect the new value of the lock to be.
		 * Only proceed to do direct handoff if there are waiters,
		 * and if the lock would become unowned.
		 */
		new = (owner - decr);
		if ((new & (RW_THREAD | RW_HAS_WAITERS)) != RW_HAS_WAITERS) {
			next = rw_cas(ptr, owner, new);
			if (owner == next) {
				/* Released! */
				return 0;
			}
			continue;
		}

		/*
		 * Grab the interlock.  Once we have that, we can adjust
		 * the waiter bits.  We must check to see if there are
		 * still waiters before proceeding.
		 */
		interlock = pthread__hashlock(ptr);
		pthread_mutex_lock(interlock);
		owner = (uintptr_t)ptr->ptr_owner;
		if ((owner & RW_HAS_WAITERS) == 0) {
			pthread_mutex_unlock(interlock);
			next = owner;
			continue;
		}

		/*
		 * Give the lock away.  SUSv3 dictates that we must give
		 * preference to writers.
		 */
		self = pthread__self();
		if ((thread = PTQ_FIRST(&ptr->ptr_wblocked)) != NULL) {
			_DIAGASSERT(((uintptr_t)thread & RW_FLAGMASK) == 0);
			new = (uintptr_t)thread | RW_WRITE_LOCKED;

			if (PTQ_NEXT(thread, pt_sleep) != NULL)
				new |= RW_HAS_WAITERS | RW_WRITE_WANTED;
			else if (ptr->ptr_nreaders != 0)
				new |= RW_HAS_WAITERS;

			/*
			 * Set in the new value.  The lock becomes owned
			 * by the writer that we are about to wake.
			 */
			(void)atomic_swap_ptr(&ptr->ptr_owner, (void *)new);
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
			membar_exit();
#endif

			/* Wake the writer. */
			thread->pt_rwlocked = _RW_LOCKED;
			pthread__unpark(&ptr->ptr_wblocked, self,
			    interlock);
		} else {
			new = 0;
			PTQ_FOREACH(thread, &ptr->ptr_rblocked, pt_sleep) {
				/*
				 * May have already been handed the lock,
				 * since pthread__unpark_all() can release
				 * our interlock before awakening all
				 * threads.
				 */
				if (thread->pt_sleepobj == NULL)
					continue;
				new += RW_READ_INCR;
				membar_exit();
				thread->pt_rwlocked = _RW_LOCKED;
			}

			/*
			 * Set in the new value.  The lock becomes owned
			 * by the readers that we are about to wake.
			 */
			(void)atomic_swap_ptr(&ptr->ptr_owner, (void *)new);

			/* Wake up all sleeping readers. */
			ptr->ptr_nreaders = 0;
			pthread__unpark_all(&ptr->ptr_rblocked, self,
			    interlock);
		}
		pthread_mutex_unlock(interlock);

		return 0;
	}
}

/*
 * Called when a timedlock awakens early to adjust the waiter bits.
 * The rwlock's interlock is held on entry, and the caller has been
 * removed from the waiters lists.
 */
static void
pthread__rwlock_early(pthread_t self, pthread_rwlock_t *ptr,
    pthread_mutex_t *interlock)
{
	uintptr_t owner, set, newval, next;
	pthread_queue_t *queue;

	pthread_mutex_lock(interlock);
	if ((queue = self->pt_sleepobj) == NULL) {
		pthread_mutex_unlock(interlock);
		return;
	}
	PTQ_REMOVE(queue, self, pt_sleep);
	self->pt_sleepobj = NULL;
	owner = (uintptr_t)ptr->ptr_owner;

	if ((owner & RW_THREAD) == 0) {
		pthread__errorfunc(__FILE__, __LINE__, __func__,
		    "lock not held");
	}

	if (!PTQ_EMPTY(&ptr->ptr_wblocked))
		set = RW_HAS_WAITERS | RW_WRITE_WANTED;
	else if (ptr->ptr_nreaders != 0)
		set = RW_HAS_WAITERS;
	else
		set = 0;

	for (;; owner = next) {
		newval = (owner & ~(RW_HAS_WAITERS | RW_WRITE_WANTED)) | set;
		next = rw_cas(ptr, owner, newval);
		if (owner == next)
			break;
	}
	pthread_mutex_unlock(interlock);
}

int
_pthread_rwlock_held_np(pthread_rwlock_t *ptr)
{
	uintptr_t owner = (uintptr_t)ptr->ptr_owner;

	if ((owner & RW_WRITE_LOCKED) != 0)
		return (owner & RW_THREAD) == (uintptr_t)pthread__self();
	return (owner & RW_THREAD) != 0;
}

int
_pthread_rwlock_rdheld_np(pthread_rwlock_t *ptr)
{
	uintptr_t owner = (uintptr_t)ptr->ptr_owner;

	return (owner & RW_THREAD) != 0 && (owner & RW_WRITE_LOCKED) == 0;
}

int
_pthread_rwlock_wrheld_np(pthread_rwlock_t *ptr)
{
	uintptr_t owner = (uintptr_t)ptr->ptr_owner;

	return (owner & (RW_THREAD | RW_WRITE_LOCKED)) ==
	    ((uintptr_t)pthread__self() | RW_WRITE_LOCKED);
}

#ifdef _PTHREAD_PSHARED
int
pthread_rwlockattr_getpshared(const pthread_rwlockattr_t * __restrict attr,
    int * __restrict pshared)
{

	pthread__error(EINVAL, "Invalid rwlock attribute",
	    ptr->ptra_magic == _PT_RWLOCKATTR_MAGIC);

	*pshared = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

int
pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared)
{

	pthread__error(EINVAL, "Invalid rwlock attribute",
	    ptr->ptra_magic == _PT_RWLOCKATTR_MAGIC);

	switch(pshared) {
	case PTHREAD_PROCESS_PRIVATE:
		return 0;
	case PTHREAD_PROCESS_SHARED:
		return ENOSYS;
	}
	return EINVAL;
}
#endif

int
pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{

	if (attr == NULL)
		return EINVAL;
	attr->ptra_magic = _PT_RWLOCKATTR_MAGIC;

	return 0;
}


int
pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{

	pthread__error(EINVAL, "Invalid rwlock attribute",
	    attr->ptra_magic == _PT_RWLOCKATTR_MAGIC);

	attr->ptra_magic = _PT_RWLOCKATTR_DEAD;

	return 0;
}
