/*	$NetBSD: pthread_alarms.c,v 1.12 2005/01/06 17:33:36 mycroft Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_alarms.c,v 1.12 2005/01/06 17:33:36 mycroft Exp $");

#include <err.h>
#include <sys/time.h>
#include <stdlib.h>

#include "pthread.h"
#include "pthread_int.h"

timer_t pthread_alarmtimer;
PTQ_HEAD(, pt_alarm_t) pthread_alarmqueue = PTQ_HEAD_INITIALIZER;
pthread_spin_t pthread_alarmqlock = __SIMPLELOCK_UNLOCKED;

#ifdef PTHREAD_ALARM_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

void
pthread__alarm_init(void)
{
	struct sigevent ev;
	int retval;

	ev.sigev_notify = SIGEV_SA;
	ev.sigev_signo = 0;
	ev.sigev_value.sival_int = (int)PT_ALARMTIMER_MAGIC;
	retval = timer_create(CLOCK_REALTIME, &ev, &pthread_alarmtimer);
	pthread__assert(retval == 0);

}

void
pthread__alarm_add(pthread_t self, struct pt_alarm_t *alarm, 
    const struct timespec *ts, void (*func)(void *), void *arg)
{
	struct pt_alarm_t *iterator, *prev;
	struct itimerspec it;
	int retval;

	pthread_lockinit(&alarm->pta_lock);
	alarm->pta_time = ts;
	alarm->pta_func = func;
	alarm->pta_arg = arg;
	alarm->pta_fired = 0;

	SDPRINTF(("(alarm_add %p) alarm %p for %d.%06ld\n",
	    self, alarm, ts->tv_sec, ts->tv_nsec/1000));
	prev = NULL;
	pthread_spinlock(self, &pthread_alarmqlock);
	PTQ_FOREACH(iterator, &pthread_alarmqueue, pta_next) {
		if (timespeccmp(ts, iterator->pta_time, <))
			break;
		prev = iterator;
	}

	if (iterator == PTQ_FIRST(&pthread_alarmqueue)) {
		PTQ_INSERT_HEAD(&pthread_alarmqueue, alarm, pta_next);
		timespecclear(&it.it_interval);
		/*
		 * A zero-valued timespec will disarm the timer, but is
		 * a legitimate value for the _timedwait functions to
		 * pass in. Correct for this here and in other timer-resetting
		 * code by setting it_value to a small value that is
		 * safely in the past.
		 */
 		if (timespecisset(ts)) {
 			it.it_value = *ts;
 		} else {
 			it.it_value.tv_sec = 1;
 			it.it_value.tv_nsec = 0;
 		}
		SDPRINTF(("(add %p) resetting alarm timer to %d.%06d\n",
		    self, it.it_value.tv_sec, it.it_value.tv_nsec/1000));
		retval = timer_settime(pthread_alarmtimer, TIMER_ABSTIME, 
		    &it, NULL);
		pthread__assert(retval == 0);
			
	} else {
		PTQ_INSERT_AFTER(&pthread_alarmqueue, prev, alarm, pta_next);
	}
	pthread_spinunlock(self, &pthread_alarmqlock);

}

void
pthread__alarm_del(pthread_t self, struct pt_alarm_t *alarm)
{
	struct pt_alarm_t *next;
	struct itimerspec it;
	int retval;

	next = NULL;
	pthread_spinlock(self, &pthread_alarmqlock);
	pthread_spinlock(self, &alarm->pta_lock);
	SDPRINTF(("(del %p) alarm %p\n", self, alarm));
	if (alarm->pta_fired == 0) {
		if (alarm == PTQ_FIRST(&pthread_alarmqueue)) {
			next = PTQ_NEXT(alarm, pta_next);
			timespecclear(&it.it_interval);
			if (next != NULL)
				/* See comment in pthread__alarm_add() */
				if (timespecisset(next->pta_time)) {
					it.it_value = *next->pta_time;
				} else {
					it.it_value.tv_sec = 1;
					it.it_value.tv_nsec = 0;
				}
			else
				timespecclear(&it.it_value);
			SDPRINTF(("(del %p) resetting alarm timer to %d.%06d\n",
			    self, it.it_value.tv_sec, it.it_value.tv_nsec/1000));
			retval = timer_settime(pthread_alarmtimer, TIMER_ABSTIME, &it, 
			    NULL);
			pthread__assert(retval == 0);
		}
		PTQ_REMOVE(&pthread_alarmqueue, alarm, pta_next);
	}
	pthread_spinunlock(self, &alarm->pta_lock);
	pthread_spinunlock(self, &pthread_alarmqlock);
}

int
pthread__alarm_fired(struct pt_alarm_t *alarm)
{

	return alarm->pta_fired;
}

void
/*ARGSUSED*/
pthread__alarm_process(pthread_t self, void *arg)
{
	struct timespec ts;
	struct itimerspec it;
	struct pt_alarm_t *iterator, *next;
	PTQ_HEAD(, pt_alarm_t) runq;
	int retval;

	clock_gettime(CLOCK_REALTIME, &ts);

	SDPRINTF(("(pro %p) alarm time %d.%06ld\n",
	    self, ts.tv_sec, ts.tv_nsec / 1000));

	PTQ_INIT(&runq);
	pthread_spinlock(self, &pthread_alarmqlock);

	/* 1. Collect a list of all alarms whose time has passed. */
	for (iterator = next = PTQ_FIRST(&pthread_alarmqueue);
	     iterator; 
	     iterator = next) {
		if (timespeccmp(&ts, iterator->pta_time, <=))
			break;
		pthread_spinlock(self, &iterator->pta_lock);
		next = PTQ_NEXT(iterator, pta_next);
		PTQ_REMOVE(&pthread_alarmqueue, iterator, pta_next);
		PTQ_INSERT_TAIL(&runq, iterator, pta_next);
		SDPRINTF(("(pro %p) collect alarm %p\n", self, iterator));
	}

	/* 2. Reset the timer for the next element in the queue. */
	if (next) {
		timespecclear(&it.it_interval);
		/* See comment in pthread__alarm_add() */
		if (timespecisset(next->pta_time)) {
			it.it_value = *next->pta_time;
		} else {
			it.it_value.tv_sec = 1;
			it.it_value.tv_nsec = 0;
		}
		SDPRINTF(("(pro %p) resetting alarm timer to %d.%09d\n", self,
		    it.it_value.tv_sec, it.it_value.tv_nsec));
		retval = timer_settime(pthread_alarmtimer, TIMER_ABSTIME, &it, NULL);
		pthread__assert(retval == 0);
	}
	pthread_spinunlock(self, &pthread_alarmqlock);

	/* 3. Call the functions for all passed alarms. */
	for (iterator = next = PTQ_FIRST(&runq);
	     iterator;
	     iterator = next) {
		SDPRINTF(("(pro %p) calling function for alarm %p\n", self, iterator));
		(*iterator->pta_func)(iterator->pta_arg);
		iterator->pta_fired = 1;
		next = PTQ_NEXT(iterator, pta_next);
		pthread_spinunlock(self, &iterator->pta_lock);
	}
	SDPRINTF(("(pro %p) done\n", self, iterator));
}
