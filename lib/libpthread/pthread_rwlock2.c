/*	$NetBSD: pthread_rwlock2.c,v 1.8 2007/12/24 14:46:29 ad Exp $ */

/*-
 * Copyright (c) 2002, 2006, 2007 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_rwlock2.c,v 1.8 2007/12/24 14:46:29 ad Exp $");

#include <errno.h>
#include <stddef.h>

#include "pthread.h"
#include "pthread_int.h"

#ifdef	PTHREAD__HAVE_ATOMIC

#define	_RW_LOCKED		0
#define	_RW_WANT_WRITE		1
#define	_RW_WANT_READ		2

static int pthread__rwlock_wrlock(pthread_rwlock_t *, const struct timespec *);
static int pthread__rwlock_rdlock(pthread_rwlock_t *, const struct timespec *);
static void pthread__rwlock_early(void *);

int	_pthread_rwlock_held_np(pthread_rwlock_t *);
int	_pthread_rwlock_rdheld_np(pthread_rwlock_t *);
int	_pthread_rwlock_wrheld_np(pthread_rwlock_t *);

#ifndef lint
__weak_alias(pthread_rwlock_held_np,_pthread_rwlock_held_np);
__weak_alias(pthread_rwlock_rdheld_np,_pthread_rwlock_rdheld_np);
__weak_alias(pthread_rwlock_wrheld_np,_pthread_rwlock_wrheld_np);
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

	return (uintptr_t)pthread__atomic_cas_ptr(&ptr->ptr_owner, (void *)o,
	    (void *)n);
}

int
pthread_rwlock_init(pthread_rwlock_t *ptr,
	    const pthread_rwlockattr_t *attr)
{

	if (attr && (attr->ptra_magic != _PT_RWLOCKATTR_MAGIC))
		return EINVAL;
	ptr->ptr_magic = _PT_RWLOCK_MAGIC;
	pthread_lockinit(&ptr->ptr_interlock);
	PTQ_INIT(&ptr->ptr_rblocked);
	PTQ_INIT(&ptr->ptr_wblocked);
	ptr->ptr_nreaders = 0;
	ptr->ptr_owner = NULL;

	return 0;
}


int
pthread_rwlock_destroy(pthread_rwlock_t *ptr)
{

	if ((ptr->ptr_magic != _PT_RWLOCK_MAGIC) ||
	    (!PTQ_EMPTY(&ptr->ptr_rblocked)) ||
	    (!PTQ_EMPTY(&ptr->ptr_wblocked)) ||
	    (ptr->ptr_nreaders != 0) ||
	    (ptr->ptr_owner != NULL))
		return EINVAL;
	ptr->ptr_magic = _PT_RWLOCK_DEAD;

	return 0;
}

static int
pthread__rwlock_rdlock(pthread_rwlock_t *ptr, const struct timespec *ts)
{
	uintptr_t owner, next;
	pthread_t self;
	int error;
	
	self = pthread__self();

#ifdef ERRORCHECK
	if (ptr->ptr_magic != _PT_RWLOCK_MAGIC)
		return EINVAL;
#endif

	for (owner = (uintptr_t)ptr->ptr_owner;; owner = next) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		if ((owner & (RW_WRITE_LOCKED | RW_WRITE_WANTED)) == 0) {
			next = rw_cas(ptr, owner, owner + RW_READ_INCR);
			if (owner == next) {
				/* Got it! */
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

		/*
		 * Grab the interlock.  Once we have that, we
		 * can adjust the waiter bits and sleep queue.
		 */
		pthread__spinlock(self, &ptr->ptr_interlock);

		/*
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 */
		next = rw_cas(ptr, owner, owner | RW_HAS_WAITERS);
		if (owner != next) {
			pthread__spinunlock(self, &ptr->ptr_interlock);
			continue;
		}

		/* The waiters bit is set - it's safe to sleep. */
	    	PTQ_INSERT_HEAD(&ptr->ptr_rblocked, self, pt_sleep);
	    	ptr->ptr_nreaders++;
		self->pt_rwlocked = _RW_WANT_READ;
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &ptr->ptr_rblocked;
		self->pt_early = pthread__rwlock_early;
		pthread__spinunlock(self, &ptr->ptr_interlock);

		error = pthread__park(self, &ptr->ptr_interlock,
		    &ptr->ptr_rblocked, ts, 0, &ptr->ptr_rblocked);

		/* Did we get the lock? */
		if (self->pt_rwlocked == _RW_LOCKED)
			return 0;
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

#ifdef ERRORCHECK
	if (ptr->ptr_magic != _PT_RWLOCK_MAGIC)
		return EINVAL;
#endif

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
			return 0;
		}
	}
}

static int
pthread__rwlock_wrlock(pthread_rwlock_t *ptr, const struct timespec *ts)
{
	uintptr_t owner, next;
	pthread_t self;
	int error;

	self = pthread__self();

#ifdef ERRORCHECK
	if (ptr->ptr_magic != _PT_RWLOCK_MAGIC)
		return EINVAL;
#endif

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

		/*
		 * Grab the interlock.  Once we have that, we
		 * can adjust the waiter bits and sleep queue.
		 */
		pthread__spinlock(self, &ptr->ptr_interlock);

		/*
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 */
		next = rw_cas(ptr, owner,
		    owner | RW_HAS_WAITERS | RW_WRITE_WANTED);
		if (owner != next) {
			pthread__spinunlock(self, &ptr->ptr_interlock);
			continue;
		}

		/* The waiters bit is set - it's safe to sleep. */
	    	PTQ_INSERT_TAIL(&ptr->ptr_wblocked, self, pt_sleep);
		self->pt_rwlocked = _RW_WANT_WRITE;
		self->pt_sleeponq = 1;
		self->pt_sleepobj = &ptr->ptr_wblocked;
		self->pt_early = pthread__rwlock_early;
		pthread__spinunlock(self, &ptr->ptr_interlock);

		error = pthread__park(self, &ptr->ptr_interlock,
		    &ptr->ptr_wblocked, ts, 0, &ptr->ptr_wblocked);

		/* Did we get the lock? */
		if (self->pt_rwlocked == _RW_LOCKED)
			return 0;
		if (error != 0)
			return error;

		pthread__errorfunc(__FILE__, __LINE__, __func__,
		    "direct handoff failure");
	}
}


int
pthread_rwlock_trywrlock(pthread_rwlock_t *ptr)
{
	uintptr_t owner, next;
	pthread_t self;

#ifdef ERRORCHECK
	if (ptr->ptr_magic != _PT_RWLOCK_MAGIC)
		return EINVAL;
#endif

	self = pthread__self();

	for (owner = (uintptr_t)ptr->ptr_owner;; owner = next) {
		if (owner != 0)
			return EBUSY;
		next = rw_cas(ptr, owner, (uintptr_t)self | RW_WRITE_LOCKED);
		if (owner == next) {
			/* Got it! */
			return 0;
		}
	}
}

int
pthread_rwlock_rdlock(pthread_rwlock_t *ptr)
{

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
	pthread_t self, thread;

#ifdef ERRORCHECK
	if ((ptr == NULL) || (ptr->ptr_magic != _PT_RWLOCK_MAGIC))
		return EINVAL;
#endif

	self = pthread__self();

	/*
	 * Since we used an add operation to set the required lock
	 * bits, we can use a subtract to clear them, which makes
	 * the read-release and write-release path similar.
	 */
	owner = (uintptr_t)ptr->ptr_owner;
	if ((owner & RW_WRITE_LOCKED) != 0) {
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
		pthread__spinlock(self, &ptr->ptr_interlock);
		owner = (uintptr_t)ptr->ptr_owner;
		if ((owner & RW_HAS_WAITERS) == 0) {
			pthread__spinunlock(self, &ptr->ptr_interlock);
			next = owner;
			continue;
		}

		/*
		 * Give the lock away.  SUSv3 dictates that we must give
		 * preference to writers.
		 */
		if ((thread = PTQ_FIRST(&ptr->ptr_wblocked)) != NULL) {
			new = (uintptr_t)thread | RW_WRITE_LOCKED;

			if (PTQ_NEXT(thread, pt_sleep) != NULL)
				new |= RW_HAS_WAITERS | RW_WRITE_WANTED;
			else if (ptr->ptr_nreaders != 0)
				new |= RW_HAS_WAITERS;

			/*
			 * Set in the new value.  The lock becomes owned
			 * by the writer that we are about to wake.
			 */
			(void)pthread__atomic_swap_ptr(&ptr->ptr_owner,
			    (void *)new);

			/* Wake the writer. */
			PTQ_REMOVE(&ptr->ptr_wblocked, thread, pt_sleep);
			thread->pt_rwlocked = _RW_LOCKED;
			pthread__unpark(self, &ptr->ptr_interlock,
			    &ptr->ptr_wblocked, thread);
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
				thread->pt_rwlocked = _RW_LOCKED;
			}

			/*
			 * Set in the new value.  The lock becomes owned
			 * by the readers that we are about to wake.
			 */
			(void)pthread__atomic_swap_ptr(&ptr->ptr_owner,
			    (void *)new);

			/* Wake up all sleeping readers. */
			ptr->ptr_nreaders = 0;
			pthread__unpark_all(self, &ptr->ptr_interlock,
			    &ptr->ptr_rblocked);
		}

		return 0;
	}
}

/*
 * Called when a timedlock awakens early to adjust the waiter bits.
 * The rwlock's interlock is held on entry, and the caller has been
 * removed from the waiters lists.
 */
static void
pthread__rwlock_early(void *obj)
{
	uintptr_t owner, set, new, next;
	pthread_rwlock_t *ptr;
	pthread_t self;
	u_int off;

	self = pthread__self();

	switch (self->pt_rwlocked) {
	case _RW_WANT_READ:
		off = offsetof(pthread_rwlock_t, ptr_rblocked);
		break;
	case _RW_WANT_WRITE:
		off = offsetof(pthread_rwlock_t, ptr_wblocked);
		break;
	default:
		pthread__errorfunc(__FILE__, __LINE__, __func__,
		    "bad value of pt_rwlocked");
		off = 0;
		/* NOTREACHED */
		break;
	}

	/* LINTED mind your own business */
	ptr = (pthread_rwlock_t *)((uint8_t *)obj - off);
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
		new = (owner & ~(RW_HAS_WAITERS | RW_WRITE_WANTED)) | set;
		next = rw_cas(ptr, owner, new);
		if (owner == next)
			break;
	}
}

int
_pthread_rwlock_held_np(pthread_rwlock_t *ptr)
{
	uintptr_t owner = (uintptr_t)ptr->ptr_owner;

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

	return (owner & RW_THREAD) != 0 && (owner & RW_WRITE_LOCKED) != 0;
}

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

	if ((attr == NULL) ||
	    (attr->ptra_magic != _PT_RWLOCKATTR_MAGIC))
		return EINVAL;
	attr->ptra_magic = _PT_RWLOCKATTR_DEAD;

	return 0;
}

#endif	/* PTHREAD__HAVE_ATOMIC */
