/*	$NetBSD: pthread_barrier.c,v 1.6 2003/03/08 08:03:35 lukem Exp $	*/

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
__RCSID("$NetBSD: pthread_barrier.c,v 1.6 2003/03/08 08:03:35 lukem Exp $");

#include <errno.h>
#include <sys/cdefs.h>

#include "pthread.h"
#include "pthread_int.h"

#undef PTHREAD_BARRIER_DEBUG

#ifdef PTHREAD_BARRIER_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

int
pthread_barrier_init(pthread_barrier_t *barrier,
    const pthread_barrierattr_t *attr, unsigned int count)
{
	pthread_t self;

#ifdef ERRORCHECK
	if ((barrier == NULL) ||
	    (attr && (attr->ptba_magic != _PT_BARRIERATTR_MAGIC)))
		return EINVAL;
#endif

	if (count == 0)
		return EINVAL;

	self = pthread__self();

	if (barrier->ptb_magic == _PT_BARRIER_MAGIC) {
		/*
		 * We're simply reinitializing the barrier to a
		 * new count.
		 */
		pthread_spinlock(self, &barrier->ptb_lock);

		if (barrier->ptb_magic != _PT_BARRIER_MAGIC) {
			pthread_spinunlock(self, &barrier->ptb_lock);
			return EINVAL;
		}

		if (!PTQ_EMPTY(&barrier->ptb_waiters)) {
			pthread_spinunlock(self, &barrier->ptb_lock);
			return EBUSY;
		}

		barrier->ptb_initcount = count;
		barrier->ptb_curcount = 0;
		barrier->ptb_generation = 0;

		pthread_spinunlock(self, &barrier->ptb_lock);

		return 0;
	}

	barrier->ptb_magic = _PT_BARRIER_MAGIC;
	pthread_lockinit(&barrier->ptb_lock);
	PTQ_INIT(&barrier->ptb_waiters);
	barrier->ptb_initcount = count;
	barrier->ptb_curcount = 0;
	barrier->ptb_generation = 0;

	return 0;
}


int
pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	pthread_t self;

#ifdef ERRORCHECK
	if ((barrier == NULL) || (barrier->ptb_magic != _PT_BARRIER_MAGIC))
		return EINVAL;
#endif

	self = pthread__self();

	pthread_spinlock(self, &barrier->ptb_lock);

	if (barrier->ptb_magic != _PT_BARRIER_MAGIC) {
		pthread_spinunlock(self, &barrier->ptb_lock);
		return EINVAL;
	}

	if (!PTQ_EMPTY(&barrier->ptb_waiters)) {
		pthread_spinunlock(self, &barrier->ptb_lock);
		return EBUSY;
	}

	barrier->ptb_magic = _PT_BARRIER_DEAD;

	pthread_spinunlock(self, &barrier->ptb_lock);

	return 0;
}


int
pthread_barrier_wait(pthread_barrier_t *barrier)
{
	pthread_t self;
	unsigned int gen;

#ifdef ERRORCHECK
	if ((barrier == NULL) || (barrier->ptb_magic != _PT_BARRIER_MAGIC))
		return EINVAL;
#endif
	self = pthread__self();

	pthread_spinlock(self, &barrier->ptb_lock);

	/*
	 * A single arbitrary thread is supposed to return
	 * PTHREAD_BARRIER_SERIAL_THREAD, and everone else
	 * is supposed to return 0.  Since pthread_barrier_wait()
	 * is not a cancellation point, this is trivial; we
	 * simply elect that the thread that causes the barrier
	 * to be satisfied gets the special return value.  Note
	 * that this final thread does not actually need to block,
	 * but instead is responsible for waking everyone else up.
	 */
	if (barrier->ptb_curcount + 1 == barrier->ptb_initcount) {
		struct pthread_queue_t blockedq;

		SDPRINTF(("(barrier wait %p) Satisfied %p\n",
		    self, barrier));

		blockedq = barrier->ptb_waiters;
		PTQ_INIT(&barrier->ptb_waiters);
		barrier->ptb_curcount = 0;
		barrier->ptb_generation++;

		pthread__sched_sleepers(self, &blockedq);

		pthread_spinunlock(self, &barrier->ptb_lock);

		return PTHREAD_BARRIER_SERIAL_THREAD;
	}

	barrier->ptb_curcount++;
	gen = barrier->ptb_generation;
	while (gen == barrier->ptb_generation) {
		SDPRINTF(("(barrier wait %p) Waiting on %p\n",
		    self, barrier));

		pthread_spinlock(self, &self->pt_statelock);

		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = barrier;
		self->pt_sleepq = &barrier->ptb_waiters;
		self->pt_sleeplock = &barrier->ptb_lock;
		
		pthread_spinunlock(self, &self->pt_statelock);

		PTQ_INSERT_TAIL(&barrier->ptb_waiters, self, pt_sleep);

		pthread__block(self, &barrier->ptb_lock);
		SDPRINTF(("(barrier wait %p) Woke up on %p\n",
		    self, barrier));
		/* Spinlock is unlocked on return */
		pthread_spinlock(self, &barrier->ptb_lock);
	}
	pthread_spinunlock(self, &barrier->ptb_lock);

	return 0;
}


int
pthread_barrierattr_init(pthread_barrierattr_t *attr)
{

#ifdef ERRORCHECK
	if (attr == NULL)
		return EINVAL;
#endif

	attr->ptba_magic = _PT_BARRIERATTR_MAGIC;

	return 0;
}


int
pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{

#ifdef ERRORCHECK
	if ((attr == NULL) ||
	    (attr->ptba_magic != _PT_BARRIERATTR_MAGIC))
		return EINVAL;
#endif

	attr->ptba_magic = _PT_BARRIERATTR_DEAD;

	return 0;
}
