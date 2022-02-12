/*	$NetBSD: pthread_barrier.c,v 1.23 2022/02/12 14:59:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2001, 2003, 2006, 2007, 2009, 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_barrier.c,v 1.23 2022/02/12 14:59:32 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

#include <errno.h>

#include "pthread.h"
#include "pthread_int.h"

int
pthread_barrier_init(pthread_barrier_t *barrier,
		     const pthread_barrierattr_t *attr, unsigned int count)
{

	pthread__error(EINVAL, "Invalid barrier attribute",
	    attr == NULL || attr->ptba_magic == _PT_BARRIERATTR_MAGIC);
	if (count == 0)
		return EINVAL;

	barrier->ptb_magic = _PT_BARRIER_MAGIC;
	PTQ_INIT(&barrier->ptb_waiters);
	barrier->ptb_initcount = count;
	barrier->ptb_curcount = 0;
	barrier->ptb_generation = 0;
	return 0;
}

int
pthread_barrier_destroy(pthread_barrier_t *barrier)
{

	pthread__error(EINVAL, "Invalid barrier",
	    barrier->ptb_magic == _PT_BARRIER_MAGIC);
	if (barrier->ptb_curcount != 0)
		return EBUSY;

	barrier->ptb_magic = _PT_BARRIER_DEAD;

	return 0;
}

int
pthread_barrier_wait(pthread_barrier_t *barrier)
{
	pthread_mutex_t *interlock;
	pthread_t self;
	unsigned int gen;

	pthread__error(EINVAL, "Invalid barrier",
	    barrier->ptb_magic == _PT_BARRIER_MAGIC);

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
	self = pthread__self();
	interlock = pthread__hashlock(barrier);
	pthread_mutex_lock(interlock);
	if (barrier->ptb_curcount + 1 == barrier->ptb_initcount) {
		barrier->ptb_generation++;
		barrier->ptb_curcount = 0;
		pthread__unpark_all(&barrier->ptb_waiters, self,
		    interlock);
		pthread_mutex_unlock(interlock);
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}
	barrier->ptb_curcount++;
	gen = barrier->ptb_generation;
	for (;;) {
		PTQ_INSERT_TAIL(&barrier->ptb_waiters, self, pt_sleep);
		self->pt_sleepobj = &barrier->ptb_waiters;
		(void)pthread__park(self, interlock, &barrier->ptb_waiters,
		    NULL, 0);
		if (__predict_true(gen != barrier->ptb_generation)) {
			break;
		}
		pthread_mutex_lock(interlock);
		if (gen != barrier->ptb_generation) {
			pthread_mutex_unlock(interlock);
			break;
		}
	}

	return 0;
}

#ifdef _PTHREAD_PSHARED
int
pthread_barrierattr_getpshared(const pthread_barrierattr_t * __restrict attr,
    int * __restrict pshared)
{

	pthread__error(EINVAL, "Invalid barrier attribute",
	    attr->ptba_magic == _PT_BARRIERATTR_MAGIC);

	*pshared = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

int
pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared)
{

	pthread__error(EINVAL, "Invalid barrier attribute",
	    attr->ptba_magic == _PT_BARRIERATTR_MAGIC);

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
pthread_barrierattr_init(pthread_barrierattr_t *attr)
{

	attr->ptba_magic = _PT_BARRIERATTR_MAGIC;
	return 0;
}

int
pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{

	pthread__error(EINVAL, "Invalid barrier attribute",
	    attr->ptba_magic == _PT_BARRIERATTR_MAGIC);
	attr->ptba_magic = _PT_BARRIERATTR_DEAD;
	return 0;
}
