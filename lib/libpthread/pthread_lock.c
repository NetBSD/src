/*	$NetBSD: pthread_lock.c,v 1.35 2022/02/12 14:59:32 riastradh Exp $	*/

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
__RCSID("$NetBSD: pthread_lock.c,v 1.35 2022/02/12 14:59:32 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

#include <sys/types.h>
#include <sys/ras.h>

#include <machine/lock.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pthread.h"
#include "pthread_int.h"

/* How many times to try acquiring spin locks on MP systems. */
#define	PTHREAD__NSPINS         64

RAS_DECL(pthread__lock);

static void 	pthread__spinlock_slow(pthread_spin_t *);

#ifdef PTHREAD__ASM_RASOPS

void pthread__ras_simple_lock_init(__cpu_simple_lock_t *);
int pthread__ras_simple_lock_try(__cpu_simple_lock_t *);
void pthread__ras_simple_unlock(__cpu_simple_lock_t *);

#else

static void
pthread__ras_simple_lock_init(__cpu_simple_lock_t *alp)
{

	__cpu_simple_lock_clear(alp);
}

static int
pthread__ras_simple_lock_try(__cpu_simple_lock_t *alp)
{
	int locked;

	RAS_START(pthread__lock);
	locked = __SIMPLELOCK_LOCKED_P(alp);
	__cpu_simple_lock_set(alp);
	RAS_END(pthread__lock);

	return !locked;
}

static void
pthread__ras_simple_unlock(__cpu_simple_lock_t *alp)
{

	__cpu_simple_lock_clear(alp);
}

#endif /* PTHREAD__ASM_RASOPS */

static const struct pthread_lock_ops pthread__lock_ops_ras = {
	pthread__ras_simple_lock_init,
	pthread__ras_simple_lock_try,
	pthread__ras_simple_unlock,
	pthread__spinlock_slow,
};

static void
pthread__atomic_simple_lock_init(__cpu_simple_lock_t *alp)
{

	__cpu_simple_lock_init(alp);
}

static int
pthread__atomic_simple_lock_try(__cpu_simple_lock_t *alp)
{

	return (__cpu_simple_lock_try(alp));
}

static void
pthread__atomic_simple_unlock(__cpu_simple_lock_t *alp)
{

	__cpu_simple_unlock(alp);
}

static const struct pthread_lock_ops pthread__lock_ops_atomic = {
	pthread__atomic_simple_lock_init,
	pthread__atomic_simple_lock_try,
	pthread__atomic_simple_unlock,
	pthread__spinlock_slow,
};

/*
 * We default to pointing to the RAS primitives; we might need to use
 * locks early, but before main() starts.  This is safe, since no other
 * threads will be active for the process, so atomicity will not be
 * required.
 */
const struct pthread_lock_ops *pthread__lock_ops = &pthread__lock_ops_ras;

/*
 * Prevent this routine from being inlined.  The common case is no
 * contention and it's better to not burden the instruction decoder.
 */
static void 
pthread__spinlock_slow(pthread_spin_t *lock)
{
	pthread_t self;
	int count;

	self = pthread__self();

	do {
		count = pthread__nspins;
		while (__SIMPLELOCK_LOCKED_P(lock) && --count > 0)
			pthread__smt_pause();
		if (count > 0) {
			if ((*self->pt_lockops.plo_try)(lock))
				break;
			continue;
		}
		sched_yield();
	} while (/*CONSTCOND*/ 1);
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

	if ((p = pthread__getenv("PTHREAD_NSPINS")) != NULL)
		pthread__nspins = atoi(p);
	else if (pthread__concurrency != 1)
		pthread__nspins = PTHREAD__NSPINS;
	else
		pthread__nspins = 1;

	if (pthread__concurrency != 1) {
		pthread__lock_ops = &pthread__lock_ops_atomic;
		return;
	}

	if (rasctl(RAS_ADDR(pthread__lock), RAS_SIZE(pthread__lock),
	    RAS_INSTALL) != 0) {
		pthread__lock_ops = &pthread__lock_ops_atomic;
		return;
	}
}

void
pthread_lockinit(pthread_spin_t *lock)
{

	pthread__simple_lock_init(lock);
}
