/*	$NetBSD: pthread_lock.c,v 1.26 2007/09/07 14:09:27 ad Exp $	*/

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

/*
 * libpthread internal spinlock routines.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_lock.c,v 1.26 2007/09/07 14:09:27 ad Exp $");

#include <sys/types.h>
#include <sys/lock.h>
#include <sys/ras.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pthread.h"
#include "pthread_int.h"

/* How many times to try acquiring spin locks on MP systems. */
#define	PTHREAD__NSPINS		1024

#ifdef PTHREAD_SPIN_DEBUG_PRINT
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

static void pthread_spinlock_slow(pthread_spin_t *);

static int pthread__atomic;

RAS_DECL(pthread__lock);
RAS_DECL(pthread__lock2);

void
pthread__simple_lock_init(__cpu_simple_lock_t *alp)
{

	if (pthread__atomic) {
		__cpu_simple_lock_init(alp);
		return;
	}

	*alp = __SIMPLELOCK_UNLOCKED;
}

int
pthread__simple_lock_try(__cpu_simple_lock_t *alp)
{
	__cpu_simple_lock_t old;

	if (pthread__atomic)
		return __cpu_simple_lock_try(alp);

	RAS_START(pthread__lock);
	old = *alp;
	*alp = __SIMPLELOCK_LOCKED;
	RAS_END(pthread__lock);

	return old == __SIMPLELOCK_UNLOCKED;
}

inline void
pthread__simple_unlock(__cpu_simple_lock_t *alp)
{

#ifdef PTHREAD__CHEAP_UNLOCK
	__cpu_simple_unlock(alp);
#else
	if (pthread__atomic) {
		__cpu_simple_unlock(alp);
		return;
	}
	*alp = __SIMPLELOCK_UNLOCKED;
#endif
}

void
pthread_spinlock(pthread_spin_t *lock)
{
#ifdef PTHREAD_SPIN_DEBUG
	pthread_t thread = pthread__self();

	SDPRINTF(("(pthread_spinlock %p) spinlock %p (count %d)\n",
	    thread, lock, thread->pt_spinlocks));
	pthread__assert(thread->pt_spinlocks >= 0);
	thread->pt_spinlocks++;
	PTHREADD_ADD(PTHREADD_SPINLOCKS);
#endif

	if (pthread__atomic) {
		if (__predict_true(__cpu_simple_lock_try(lock)))
			return;
	} else {
		__cpu_simple_lock_t old;

		RAS_START(pthread__lock2);
		old = *lock;
		*lock = __SIMPLELOCK_LOCKED;
		RAS_END(pthread__lock2);

		if (__predict_true(old == __SIMPLELOCK_UNLOCKED))
			return;
	}

	pthread_spinlock_slow(lock);
}

/*
 * Prevent this routine from being inlined.  The common case is no
 * contention and it's better to not burden the instruction decoder.
 */
#if __GNUC_PREREQ__(3, 0)
__attribute ((noinline))
#endif
static void 
pthread_spinlock_slow(pthread_spin_t *lock)
{
	int count;
#ifdef PTHREAD_SPIN_DEBUG
	pthread_t thread = pthread__self();
#endif

	do {
		count = pthread__nspins;
		while (*lock == __SIMPLELOCK_LOCKED && --count > 0)
			pthread__smt_pause();
		if (count > 0) {
			if (pthread__simple_lock_try(lock))
				break;
			continue;
		}

#ifdef PTHREAD_SPIN_DEBUG
		SDPRINTF(("(pthread_spinlock %p) retrying spinlock %p "
		    "(count %d)\n", thread, lock,
		    thread->pt_spinlocks));
		thread->pt_spinlocks--;
		/* XXXLWP far from ideal */
		sched_yield();
		thread->pt_spinlocks++;
#else
		/* XXXLWP far from ideal */
		sched_yield();
#endif
	} while (/*CONSTCOND*/ 1);
}

int
pthread_spintrylock(pthread_spin_t *lock)
{
#ifdef PTHREAD_SPIN_DEBUG
	pthread_t thread = pthread__self();
	int ret;

	SDPRINTF(("(pthread_spintrylock %p) spinlock %p (count %d)\n",
	    thread, lock, thread->pt_spinlocks));
	thread->pt_spinlocks++;
	ret = pthread__simple_lock_try(lock);
	if (!ret)
		thread->pt_spinlocks--;
	return ret;
#else
	return pthread__simple_lock_try(lock);
#endif
}

void
pthread_spinunlock(pthread_spin_t *lock)
{
#ifdef PTHREAD_SPIN_DEBUG
	pthread_t thread = pthread__self();

	SDPRINTF(("(pthread_spinunlock %p) spinlock %p (count %d)\n",
	    thread, lock, thread->pt_spinlocks));

	pthread__simple_unlock(lock);
	thread->pt_spinlocks--;
	pthread__assert(thread->pt_spinlocks >= 0);
	PTHREADD_ADD(PTHREADD_SPINUNLOCKS);
#else
	pthread__simple_unlock(lock);
#endif
}

/*
 * Initialize the locking primitives.  On uniprocessors, we always
 * use Restartable Atomic Sequences if they are available.  Otherwise,
 * we fall back onto machine-dependent atomic lock primitives.
 */
void
pthread__lockprim_init(void)
{
	char *p;

	if ((p = getenv("PTHREAD_NSPINS")) != NULL)
		pthread__nspins = atoi(p);
	else if (pthread__concurrency != 1)
		pthread__nspins = PTHREAD__NSPINS;
	else
		pthread__nspins = 1;

	if (pthread__concurrency != 1) {
		pthread__atomic = 1;
		return;
	}

	if (rasctl(RAS_ADDR(pthread__lock), RAS_SIZE(pthread__lock),
	    RAS_INSTALL) != 0) {
	    	pthread__atomic = 1;
	    	return;
	}

	if (rasctl(RAS_ADDR(pthread__lock2), RAS_SIZE(pthread__lock2),
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
