/*	$NetBSD: pthread_sleep.c,v 1.1 2003/02/15 22:15:50 nathanw Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#include <errno.h>
#include <sys/cdefs.h>
#include <sys/time.h>

#include "pthread.h"
#include "pthread_int.h"

int	_sys_nanosleep(const struct timespec *, struct timespec *);

extern int pthread__started;

/*
 * The point of this exercise is to avoid sleeping in the kernel for
 * each and every thread that wants to sleep. A surprising number of
 * applications create many threads that spend a lot of time sleeping.
 *
 * "Save a LWP, shoot a preppie."
 */

/* Queue of threads in nanosleep() */
struct pthread_queue_t pthread__nanosleeping;
static pthread_spin_t pt_nanosleep_lock;
/*
 * Nothing actually signals or waits on this lock, but the sleepobj
 * needs to point to something.
 */
static pthread_cond_t pt_nanosleep_cond = PTHREAD_COND_INITIALIZER;

static void pthread__nanosleep_callback(void *);


int
nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	int retval;
	pthread_t self;
	struct timespec sleeptime;
	struct timeval now;
	struct pt_alarm_t alarm;

	if ((rqtp->tv_sec) < 0 ||
	    (rqtp->tv_nsec < 0) || (rqtp->tv_nsec > 1000000000L))
		return EINVAL;

	retval = 0;

	self = pthread__self();

	if (pthread__started == 0) {
		pthread__testcancel(self);
		retval = _sys_nanosleep(rqtp, rmtp);
		pthread__testcancel(self);
		return retval;
	}

	/*
	 * Figure out the absolute time to sleep until. If the thread
	 * got suspended before this then the sleep will be longer
	 * than intended, but the same thing could happen just before
	 * the thread called sleep too, so it's not our problem.
	 */
	gettimeofday(&now, NULL);
	TIMEVAL_TO_TIMESPEC(&now, &sleeptime);
	timespecadd(&sleeptime, rqtp, &sleeptime);

	pthread_spinlock(self, &pt_nanosleep_lock);
	pthread_spinlock(self, &self->pt_statelock);
	if (self->pt_cancel) {
		pthread_spinunlock(self, &self->pt_statelock);
		pthread_spinunlock(self, &pt_nanosleep_lock);
		pthread_exit(PTHREAD_CANCELED);
	}
	pthread__alarm_add(self, &alarm, &sleeptime,
	    pthread__nanosleep_callback, self);
	    
	self->pt_state = PT_STATE_BLOCKED_QUEUE;
	self->pt_sleepobj = &pt_nanosleep_cond;
	self->pt_sleepq = &pthread__nanosleeping;
	self->pt_sleeplock = &pt_nanosleep_lock;
	pthread_spinunlock(self, &self->pt_statelock);

	PTQ_INSERT_TAIL(&pthread__nanosleeping, self, pt_sleep);
	pthread__block(self, &pt_nanosleep_lock);
	/* Spinlock is unlocked on return */
	pthread__alarm_del(self, &alarm);

	pthread__testcancel(self);

	if (!pthread__alarm_fired(&alarm)) {
		retval = -1;
		errno = EINTR;
		if (rmtp) {
			gettimeofday(&now, NULL);
			TIMEVAL_TO_TIMESPEC(&now, rmtp);
			timespecsub(&sleeptime, rmtp, rmtp);
		}
	}

	return retval;
}

static void
pthread__nanosleep_callback(void *arg)
{
	pthread_t self, thread;

	thread = arg;
	self = pthread__self();
	/*
	 * Don't dequeue and schedule the thread if it's already been
	 * queued up by a signal or broadcast (but hasn't yet run as far
	 * as pthread__alarm_del(), or we wouldn't be here, and hence can't
	 * have become blocked on some *other* queue).
	 */
	pthread_spinlock(self, &pt_nanosleep_lock);
	if (thread->pt_state == PT_STATE_BLOCKED_QUEUE) {
		PTQ_REMOVE(&pthread__nanosleeping, thread, pt_sleep);
		pthread__sched(self, thread);
	}
	pthread_spinunlock(self, &pt_nanosleep_lock);
}

__strong_alias(_nanosleep, nanosleep)
