/*	$NetBSD: pthread_mutex.c,v 1.54.2.2 2014/08/20 00:02:20 tls Exp $	*/

/*-
 * Copyright (c) 2001, 2003, 2006, 2007, 2008 The NetBSD Foundation, Inc.
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

/*
 * To track threads waiting for mutexes to be released, we use lockless
 * lists built on atomic operations and memory barriers.
 *
 * A simple spinlock would be faster and make the code easier to
 * follow, but spinlocks are problematic in userspace.  If a thread is
 * preempted by the kernel while holding a spinlock, any other thread
 * attempting to acquire that spinlock will needlessly busy wait.
 *
 * There is no good way to know that the holding thread is no longer
 * running, nor to request a wake-up once it has begun running again. 
 * Of more concern, threads in the SCHED_FIFO class do not have a
 * limited time quantum and so could spin forever, preventing the
 * thread holding the spinlock from getting CPU time: it would never
 * be released.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_mutex.c,v 1.54.2.2 2014/08/20 00:02:20 tls Exp $");

#include <sys/types.h>
#include <sys/lwpctl.h>
#include <sys/lock.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"

#define	MUTEX_WAITERS_BIT		((uintptr_t)0x01)
#define	MUTEX_RECURSIVE_BIT		((uintptr_t)0x02)
#define	MUTEX_DEFERRED_BIT		((uintptr_t)0x04)
#define	MUTEX_THREAD			((uintptr_t)-16L)

#define	MUTEX_HAS_WAITERS(x)		((uintptr_t)(x) & MUTEX_WAITERS_BIT)
#define	MUTEX_RECURSIVE(x)		((uintptr_t)(x) & MUTEX_RECURSIVE_BIT)
#define	MUTEX_OWNER(x)			((uintptr_t)(x) & MUTEX_THREAD)

#if __GNUC_PREREQ__(3, 0)
#define	NOINLINE		__attribute ((noinline))
#else
#define	NOINLINE		/* nothing */
#endif

static void	pthread__mutex_wakeup(pthread_t, pthread_mutex_t *);
static int	pthread__mutex_lock_slow(pthread_mutex_t *);
static int	pthread__mutex_unlock_slow(pthread_mutex_t *);
static void	pthread__mutex_pause(void);

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

int
pthread_mutex_init(pthread_mutex_t *ptm, const pthread_mutexattr_t *attr)
{
	intptr_t type;

	if (__predict_false(__uselibcstub))
		return __libc_mutex_init_stub(ptm, attr);

	if (attr == NULL)
		type = PTHREAD_MUTEX_NORMAL;
	else
		type = (intptr_t)attr->ptma_private;

	switch (type) {
	case PTHREAD_MUTEX_ERRORCHECK:
		__cpu_simple_lock_set(&ptm->ptm_errorcheck);
		ptm->ptm_owner = NULL;
		break;
	case PTHREAD_MUTEX_RECURSIVE:
		__cpu_simple_lock_clear(&ptm->ptm_errorcheck);
		ptm->ptm_owner = (void *)MUTEX_RECURSIVE_BIT;
		break;
	default:
		__cpu_simple_lock_clear(&ptm->ptm_errorcheck);
		ptm->ptm_owner = NULL;
		break;
	}

	ptm->ptm_magic = _PT_MUTEX_MAGIC;
	ptm->ptm_waiters = NULL;
	ptm->ptm_recursed = 0;

	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *ptm)
{

	if (__predict_false(__uselibcstub))
		return __libc_mutex_destroy_stub(ptm);

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);
	pthread__error(EBUSY, "Destroying locked mutex",
	    MUTEX_OWNER(ptm->ptm_owner) == 0);

	ptm->ptm_magic = _PT_MUTEX_DEAD;
	return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *ptm)
{
	pthread_t self;
	void *val;

	if (__predict_false(__uselibcstub))
		return __libc_mutex_lock_stub(ptm);

	self = pthread__self();
	val = atomic_cas_ptr(&ptm->ptm_owner, NULL, self);
	if (__predict_true(val == NULL)) {
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
		membar_enter();
#endif
		return 0;
	}
	return pthread__mutex_lock_slow(ptm);
}

/* We want function call overhead. */
NOINLINE static void
pthread__mutex_pause(void)
{

	pthread__smt_pause();
}

/*
 * Spin while the holder is running.  'lwpctl' gives us the true
 * status of the thread.  pt_blocking is set by libpthread in order
 * to cut out system call and kernel spinlock overhead on remote CPUs
 * (could represent many thousands of clock cycles).  pt_blocking also
 * makes this thread yield if the target is calling sched_yield().
 */
NOINLINE static void *
pthread__mutex_spin(pthread_mutex_t *ptm, pthread_t owner)
{
	pthread_t thread;
	unsigned int count, i;

	for (count = 2;; owner = ptm->ptm_owner) {
		thread = (pthread_t)MUTEX_OWNER(owner);
		if (thread == NULL)
			break;
		if (thread->pt_lwpctl->lc_curcpu == LWPCTL_CPU_NONE ||
		    thread->pt_blocking)
			break;
		if (count < 128) 
			count += count;
		for (i = count; i != 0; i--)
			pthread__mutex_pause();
	}

	return owner;
}

NOINLINE static void
pthread__mutex_setwaiters(pthread_t self, pthread_mutex_t *ptm)
{
	void *new, *owner;

	/*
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
	 * the value of ptm_owner/pt_mutexwait after we have entered
	 * the waiters list (the CAS itself must be atomic).
	 */
again:
	membar_consumer();
	owner = ptm->ptm_owner;

	if (MUTEX_OWNER(owner) == 0) {
		pthread__mutex_wakeup(self, ptm);
		return;
	}
	if (!MUTEX_HAS_WAITERS(owner)) {
		new = (void *)((uintptr_t)owner | MUTEX_WAITERS_BIT);
		if (atomic_cas_ptr(&ptm->ptm_owner, owner, new) != owner) {
			goto again;
		}
	}

	/*
	 * Note that pthread_mutex_unlock() can do a non-interlocked CAS.
	 * We cannot know if the presence of the waiters bit is stable
	 * while the holding thread is running.  There are many assumptions;
	 * see sys/kern/kern_mutex.c for details.  In short, we must spin if
	 * we see that the holder is running again.
	 */
	membar_sync();
	pthread__mutex_spin(ptm, owner);

	if (membar_consumer(), !MUTEX_HAS_WAITERS(ptm->ptm_owner)) {
		goto again;
	}
}

NOINLINE static int
pthread__mutex_lock_slow(pthread_mutex_t *ptm)
{
	void *waiters, *new, *owner, *next;
	pthread_t self;
	int serrno;

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);

	owner = ptm->ptm_owner;
	self = pthread__self();

	/* Recursive or errorcheck? */
	if (MUTEX_OWNER(owner) == (uintptr_t)self) {
		if (MUTEX_RECURSIVE(owner)) {
			if (ptm->ptm_recursed == INT_MAX)
				return EAGAIN;
			ptm->ptm_recursed++;
			return 0;
		}
		if (__SIMPLELOCK_LOCKED_P(&ptm->ptm_errorcheck))
			return EDEADLK;
	}

	serrno = errno;
	for (;; owner = ptm->ptm_owner) {
		/* Spin while the owner is running. */
		owner = pthread__mutex_spin(ptm, owner);

		/* If it has become free, try to acquire it again. */
		if (MUTEX_OWNER(owner) == 0) {
			do {
				new = (void *)
				    ((uintptr_t)self | (uintptr_t)owner);
				next = atomic_cas_ptr(&ptm->ptm_owner, owner,
				    new);
				if (next == owner) {
					errno = serrno;
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
					membar_enter();
#endif
					return 0;
				}
				owner = next;
			} while (MUTEX_OWNER(owner) == 0);
			/*
			 * We have lost the race to acquire the mutex.
			 * The new owner could be running on another
			 * CPU, in which case we should spin and avoid
			 * the overhead of blocking.
			 */
			continue;
		}

		/*
		 * Nope, still held.  Add thread to the list of waiters.
		 * Issue a memory barrier to ensure mutexwait/mutexnext
		 * are visible before we enter the waiters list.
		 */
		self->pt_mutexwait = 1;
		for (waiters = ptm->ptm_waiters;; waiters = next) {
			self->pt_mutexnext = waiters;
			membar_producer();
			next = atomic_cas_ptr(&ptm->ptm_waiters, waiters, self);
			if (next == waiters)
			    	break;
		}

		/* Set the waiters bit and block. */
		pthread__mutex_setwaiters(self, ptm);

		/*
		 * We may have been awoken by the current thread above,
		 * or will be awoken by the current holder of the mutex.
		 * The key requirement is that we must not proceed until
		 * told that we are no longer waiting (via pt_mutexwait
		 * being set to zero).  Otherwise it is unsafe to re-enter
		 * the thread onto the waiters list.
		 */
		while (self->pt_mutexwait) {
			self->pt_blocking++;
			(void)_lwp_park(CLOCK_REALTIME, TIMER_ABSTIME, NULL,
			    self->pt_unpark, __UNVOLATILE(&ptm->ptm_waiters),
			    __UNVOLATILE(&ptm->ptm_waiters));
			self->pt_unpark = 0;
			self->pt_blocking--;
			membar_sync();
		}
	}
}

int
pthread_mutex_trylock(pthread_mutex_t *ptm)
{
	pthread_t self;
	void *val, *new, *next;

	if (__predict_false(__uselibcstub))
		return __libc_mutex_trylock_stub(ptm);

	self = pthread__self();
	val = atomic_cas_ptr(&ptm->ptm_owner, NULL, self);
	if (__predict_true(val == NULL)) {
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
		membar_enter();
#endif
		return 0;
	}

	if (MUTEX_RECURSIVE(val)) {
		if (MUTEX_OWNER(val) == 0) {
			new = (void *)((uintptr_t)self | (uintptr_t)val);
			next = atomic_cas_ptr(&ptm->ptm_owner, val, new);
			if (__predict_true(next == val)) {
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
				membar_enter();
#endif
				return 0;
			}
		}
		if (MUTEX_OWNER(val) == (uintptr_t)self) {
			if (ptm->ptm_recursed == INT_MAX)
				return EAGAIN;
			ptm->ptm_recursed++;
			return 0;
		}
	}

	return EBUSY;
}

int
pthread_mutex_unlock(pthread_mutex_t *ptm)
{
	pthread_t self;
	void *value;

	if (__predict_false(__uselibcstub))
		return __libc_mutex_unlock_stub(ptm);

	/*
	 * Note this may be a non-interlocked CAS.  See lock_slow()
	 * above and sys/kern/kern_mutex.c for details.
	 */
#ifndef PTHREAD__ATOMIC_IS_MEMBAR
	membar_exit();
#endif
	self = pthread__self();
	value = atomic_cas_ptr_ni(&ptm->ptm_owner, self, NULL);
	if (__predict_true(value == self)) {
		pthread__smt_wake();
		return 0;
	}
	return pthread__mutex_unlock_slow(ptm);
}

NOINLINE static int
pthread__mutex_unlock_slow(pthread_mutex_t *ptm)
{
	pthread_t self, owner, new;
	int weown, error, deferred;

	pthread__error(EINVAL, "Invalid mutex",
	    ptm->ptm_magic == _PT_MUTEX_MAGIC);

	self = pthread__self();
	owner = ptm->ptm_owner;
	weown = (MUTEX_OWNER(owner) == (uintptr_t)self);
	deferred = (int)((uintptr_t)owner & MUTEX_DEFERRED_BIT);
	error = 0;

	if (__SIMPLELOCK_LOCKED_P(&ptm->ptm_errorcheck)) {
		if (!weown) {
			error = EPERM;
			new = owner;
		} else {
			new = NULL;
		}
	} else if (MUTEX_RECURSIVE(owner)) {
		if (!weown) {
			error = EPERM;
			new = owner;
		} else if (ptm->ptm_recursed) {
			ptm->ptm_recursed--;
			new = owner;
		} else {
			new = (pthread_t)MUTEX_RECURSIVE_BIT;
		}
	} else {
		pthread__error(EPERM,
		    "Unlocking unlocked mutex", (owner != NULL));
		pthread__error(EPERM,
		    "Unlocking mutex owned by another thread", weown);
		new = NULL;
	}

	/*
	 * Release the mutex.  If there appear to be waiters, then
	 * wake them up.
	 */
	if (new != owner) {
		owner = atomic_swap_ptr(&ptm->ptm_owner, new);
		if (MUTEX_HAS_WAITERS(owner) != 0) {
			pthread__mutex_wakeup(self, ptm);
			return 0;
		}
	}

	/*
	 * There were no waiters, but we may have deferred waking
	 * other threads until mutex unlock - we must wake them now.
	 */
	if (!deferred)
		return error;

	if (self->pt_nwaiters == 1) {
		/*
		 * If the calling thread is about to block, defer
		 * unparking the target until _lwp_park() is called.
		 */
		if (self->pt_willpark && self->pt_unpark == 0) {
			self->pt_unpark = self->pt_waiters[0];
		} else {
			(void)_lwp_unpark(self->pt_waiters[0],
			    __UNVOLATILE(&ptm->ptm_waiters));
		}
	} else {
		(void)_lwp_unpark_all(self->pt_waiters, self->pt_nwaiters,
		    __UNVOLATILE(&ptm->ptm_waiters));
	}
	self->pt_nwaiters = 0;

	return error;
}

/*
 * pthread__mutex_wakeup: unpark threads waiting for us
 *
 * unpark threads on the ptm->ptm_waiters list and self->pt_waiters.
 */

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
	thread = atomic_swap_ptr(&ptm->ptm_waiters, NULL);
	pthread__smt_wake();

	for (;;) {
		/*
		 * Pull waiters from the queue and add to our list.
		 * Use a memory barrier to ensure that we safely
		 * read the value of pt_mutexnext before 'thread'
		 * sees pt_mutexwait being cleared.
		 */
		for (n = self->pt_nwaiters, self->pt_nwaiters = 0;
		    n < pthread__unpark_max && thread != NULL;
		    thread = next) {
		    	next = thread->pt_mutexnext;
		    	if (thread != self) {
				self->pt_waiters[n++] = thread->pt_lid;
				membar_sync();
			}
			thread->pt_mutexwait = 0;
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
				return;
			}
			rv = (ssize_t)_lwp_unpark(self->pt_waiters[0],
			    __UNVOLATILE(&ptm->ptm_waiters));
			if (rv != 0 && errno != EALREADY && errno != EINTR &&
			    errno != ESRCH) {
				pthread__errorfunc(__FILE__, __LINE__,
				    __func__, "_lwp_unpark failed");
			}
			return;
		default:
			rv = _lwp_unpark_all(self->pt_waiters, (size_t)n,
			    __UNVOLATILE(&ptm->ptm_waiters));
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
	if (__predict_false(__uselibcstub))
		return __libc_mutexattr_init_stub(attr);

	attr->ptma_magic = _PT_MUTEXATTR_MAGIC;
	attr->ptma_private = (void *)PTHREAD_MUTEX_DEFAULT;
	return 0;
}

int
pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	if (__predict_false(__uselibcstub))
		return __libc_mutexattr_destroy_stub(attr);

	pthread__error(EINVAL, "Invalid mutex attribute",
	    attr->ptma_magic == _PT_MUTEXATTR_MAGIC);

	return 0;
}

int
pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *typep)
{
	pthread__error(EINVAL, "Invalid mutex attribute",
	    attr->ptma_magic == _PT_MUTEXATTR_MAGIC);

	*typep = (int)(intptr_t)attr->ptma_private;
	return 0;
}

int
pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	if (__predict_false(__uselibcstub))
		return __libc_mutexattr_settype_stub(attr, type);

	pthread__error(EINVAL, "Invalid mutex attribute",
	    attr->ptma_magic == _PT_MUTEXATTR_MAGIC);

	switch (type) {
	case PTHREAD_MUTEX_NORMAL:
	case PTHREAD_MUTEX_ERRORCHECK:
	case PTHREAD_MUTEX_RECURSIVE:
		attr->ptma_private = (void *)(intptr_t)type;
		return 0;
	default:
		return EINVAL;
	}
}

/*
 * pthread__mutex_deferwake: try to defer unparking threads in self->pt_waiters
 *
 * In order to avoid unnecessary contention on the interlocking mutex,
 * we defer waking up threads until we unlock the mutex.  The threads will
 * be woken up when the calling thread (self) releases the first mutex with
 * MUTEX_DEFERRED_BIT set.  It likely be the mutex 'ptm', but no problem
 * even if it isn't.
 */

void
pthread__mutex_deferwake(pthread_t self, pthread_mutex_t *ptm)
{

	if (__predict_false(ptm == NULL ||
	    MUTEX_OWNER(ptm->ptm_owner) != (uintptr_t)self)) {
	    	(void)_lwp_unpark_all(self->pt_waiters, self->pt_nwaiters,
	    	    __UNVOLATILE(&ptm->ptm_waiters));
	    	self->pt_nwaiters = 0;
	} else {
		atomic_or_ulong((volatile unsigned long *)
		    (uintptr_t)&ptm->ptm_owner,
		    (unsigned long)MUTEX_DEFERRED_BIT);
	}
}

int
_pthread_mutex_held_np(pthread_mutex_t *ptm)
{

	return MUTEX_OWNER(ptm->ptm_owner) == (uintptr_t)pthread__self();
}

pthread_t
_pthread_mutex_owner_np(pthread_mutex_t *ptm)
{

	return (pthread_t)MUTEX_OWNER(ptm->ptm_owner);
}
