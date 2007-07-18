/*	$NetBSD: pthread_lock.c,v 1.20.2.1 2007/07/18 13:36:19 skrll Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_lock.c,v 1.20.2.1 2007/07/18 13:36:19 skrll Exp $");

#include <sys/types.h>
#include <sys/lock.h>
#include <sys/ras.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include "pthread.h"
#include "pthread_int.h"

#ifdef PTHREAD_SPIN_DEBUG_PRINT
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

/* This does not belong here. */
#if defined(i386) || defined(__x86_64__)
#define	smt_pause()	__asm __volatile("rep; nop" ::: "memory")
#else
#define	smt_pause()	/* nothing */
#endif

extern int pthread__nspins;
static int pthread__atomic;

RAS_DECL(pthread__lock);

void
pthread__simple_lock_init(__cpu_simple_lock_t *alp)
{

	if (pthread__atomic) {
		__cpu_simple_lock_init(alp);
		return;
	}

	__cpu_simple_lock_clear(alp);
}

int
pthread__simple_lock_try(__cpu_simple_lock_t *alp)
{
	__cpu_simple_lock_t old;
	__cpu_simple_lock_t locked = __SIMPLELOCK_LOCKED;

	if (pthread__atomic)
		return __cpu_simple_lock_try(alp);

	RAS_START(pthread__lock);
	old = *alp;
	*alp = locked;
	RAS_END(pthread__lock);

	return __SIMPLELOCK_UNLOCKED_P(&old);
}

inline void
pthread__simple_unlock(__cpu_simple_lock_t *alp)
{

	if (pthread__atomic) {
		__cpu_simple_unlock(alp);
		return;
	}

	__cpu_simple_lock_clear(alp);
}

/*
 * Initialize the locking primitives.  On uniprocessors, we always
 * use Restartable Atomic Sequences if they are available.  Otherwise,
 * we fall back onto machine-dependent atomic lock primitives.
 */
void
pthread__lockprim_init(int ncpu)
{

	if (ncpu != 1) {
		pthread__atomic = 1;
		return;
	}

	if (rasctl(RAS_ADDR(pthread__lock), RAS_SIZE(pthread__lock),
	    RAS_INSTALL) != 0) {
	    	pthread__atomic = 1;
	    	return;
	}
}

void
pthread_lockinit(pthread_spin_t *lock)
{

	pthread__simple_lock_init(lock);
}

void
pthread_spinlock(pthread_t thread, pthread_spin_t *lock)
{
	int count, ret;

	count = pthread__nspins;
	SDPRINTF(("(pthread_spinlock %p) spinlock %p (count %d)\n",
	    thread, lock, thread->pt_spinlocks));
#ifdef PTHREAD_SPIN_DEBUG
	pthread__assert(thread->pt_spinlocks >= 0);
#endif

	thread->pt_spinlocks++;
	if (__predict_true(pthread__simple_lock_try(lock))) {
		PTHREADD_ADD(PTHREADD_SPINLOCKS);
		return;
	}

	do {
		while ((ret = pthread__simple_lock_try(lock)) == 0 &&
		    --count) {
			smt_pause();
		}

		if (ret == 1)
			break;

		SDPRINTF(("(pthread_spinlock %p) retrying spinlock %p "
		    "(count %d)\n", thread, lock,
		    thread->pt_spinlocks));
		thread->pt_spinlocks--;

		/* XXXLWP far from ideal */
		sched_yield();
		count = pthread__nspins;
		thread->pt_spinlocks++;
	} while (/*CONSTCOND*/ 1);

	PTHREADD_ADD(PTHREADD_SPINLOCKS);
}

int
pthread_spintrylock(pthread_t thread, pthread_spin_t *lock)
{
	int ret;

	SDPRINTF(("(pthread_spintrylock %p) spinlock %p (count %d)\n",
	    thread, lock, thread->pt_spinlocks));

	thread->pt_spinlocks++;
	ret = pthread__simple_lock_try(lock);
	if (!ret)
		thread->pt_spinlocks--;

	return ret;
}

void
pthread_spinunlock(pthread_t thread, pthread_spin_t *lock)
{

	SDPRINTF(("(pthread_spinunlock %p) spinlock %p (count %d)\n",
	    thread, lock, thread->pt_spinlocks));

	pthread__simple_unlock(lock);
	thread->pt_spinlocks--;
#ifdef PTHREAD_SPIN_DEBUG
	pthread__assert(thread->pt_spinlocks >= 0);
#endif
	PTHREADD_ADD(PTHREADD_SPINUNLOCKS);
}


/* 
 * Public (POSIX-specified) spinlocks.
 */
int
pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{

#ifdef ERRORCHECK
	if (lock == NULL || (pshared != PTHREAD_PROCESS_PRIVATE &&
	    pshared != PTHREAD_PROCESS_SHARED))
		return EINVAL;
#endif
	lock->pts_magic = _PT_SPINLOCK_MAGIC;

	/*
	 * We don't actually use the pshared flag for anything;
	 * CPU simple locks have all the process-shared properties 
	 * that we want anyway.
	 */
	lock->pts_flags = pshared;
	pthread_lockinit(&lock->pts_spin);

	return 0;
}

int
pthread_spin_destroy(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if (lock == NULL || lock->pts_magic != _PT_SPINLOCK_MAGIC)
		return EINVAL;
	if (!__SIMPLELOCK_UNLOCKED_P(&lock->pts_spin))
		return EBUSY;
#endif

	lock->pts_magic = _PT_SPINLOCK_DEAD;

	return 0;
}

int
pthread_spin_lock(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if (lock == NULL || lock->pts_magic != _PT_SPINLOCK_MAGIC)
		return EINVAL;
#endif

	while (pthread__simple_lock_try(&lock->pts_spin) == 0) {
		smt_pause();
	}

	return 0;
}

int
pthread_spin_trylock(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if (lock == NULL || lock->pts_magic != _PT_SPINLOCK_MAGIC)
		return EINVAL;
#endif

	if (pthread__simple_lock_try(&lock->pts_spin) == 0)
		return EBUSY;

	return 0;
}

int
pthread_spin_unlock(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if (lock == NULL || lock->pts_magic != _PT_SPINLOCK_MAGIC)
		return EINVAL;
#endif

	pthread__simple_unlock(&lock->pts_spin);

	return 0;
}
