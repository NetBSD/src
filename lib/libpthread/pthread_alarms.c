/*	$NetBSD: pthread_alarms.c,v 1.1.2.3 2002/02/19 23:56:08 nathanw Exp $	*/

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

#include <err.h>
#include <sys/time.h>
#include <stdlib.h>

#include "pthread.h"
#include "pthread_int.h"


struct pt_alarm_t {
	PTQ_ENTRY(pt_alarm_t)	pta_next;
	const struct timespec	*pta_time;
	void	(*pta_func)(void *);
	void	*pta_arg;
	int	pta_fired;
};

timer_t pthread_alarmtimer;
static PTQ_HEAD(, pt_alarm_t) pthread_alarmqueue = PTQ_HEAD_INITIALIZER;
static pthread_spin_t pthread_alarmqlock;

void
pthread__alarm_init(void)
{
	struct sigevent ev;
	int retval;

	ev.sigev_notify = SIGEV_SA;
	ev.sigev_signo = 0;
	ev.sigev_value.sival_int = PT_ALARMTIMER_MAGIC;
	retval = timer_create(CLOCK_REALTIME, &ev, &pthread_alarmtimer);
	if (retval)
		err(1, "timer_create");

}

void *
pthread__alarm_add(pthread_t self, const struct timespec *ts, 
    void (*func)(void *), void *arg)
{
	struct pt_alarm_t *alarm, *iterator, *prev;
	struct itimerspec it;
	int retval;

	alarm = malloc(sizeof(struct pt_alarm_t));
	alarm->pta_time = ts;
	alarm->pta_func = func;
	alarm->pta_arg = arg;
	alarm->pta_fired = 0;

	prev = NULL;
	pthread_spinlock(self, &pthread_alarmqlock);
	iterator = PTQ_FIRST(&pthread_alarmqueue);
	PTQ_FOREACH(iterator, &pthread_alarmqueue, pta_next) {
		if (timespeccmp(ts, iterator->pta_time, <))
			break;
		prev = iterator;
	}

	if (iterator == PTQ_FIRST(&pthread_alarmqueue)) {
		PTQ_INSERT_HEAD(&pthread_alarmqueue, alarm, pta_next);
		timespecclear(&it.it_interval);
		it.it_value = *ts;
		retval = timer_settime(pthread_alarmtimer, TIMER_ABSTIME, &it, NULL);
		if (retval)
			err(1, "timer_settime");
			
	} else {
		PTQ_INSERT_AFTER(&pthread_alarmqueue, prev, alarm, pta_next);
	}
	pthread_spinunlock(self, &pthread_alarmqlock);

	return alarm;
}

void
pthread__alarm_del(pthread_t self, void *arg)
{
	struct pt_alarm_t *alarm, *next;
	struct itimerspec it;

	alarm = arg;

	pthread_spinlock(self, &pthread_alarmqlock);
	if (alarm->pta_fired == 0) {
		if (alarm == PTQ_FIRST(&pthread_alarmqueue)) {
			next = PTQ_NEXT(alarm, pta_next);
			PTQ_REMOVE(&pthread_alarmqueue, alarm, pta_next);
			timespecclear(&it.it_interval);
			if (next != NULL)
				it.it_value = *next->pta_time;
			else
				timespecclear(&it.it_value);
			timer_settime(pthread_alarmtimer, TIMER_ABSTIME, &it, 
			    NULL);
		} else {
			PTQ_REMOVE(&pthread_alarmqueue, alarm, pta_next);
		}
	}
	pthread_spinunlock(self, &pthread_alarmqlock);

	free(alarm);
}

int
pthread__alarm_fired(void *arg)
{
	struct pt_alarm_t *alarm;
	
	alarm = arg;

	return alarm->pta_fired;
}

void
pthread__alarm_process(pthread_t self, void *arg)
{
	struct timeval tv;
	struct timespec ts;
	struct pt_alarm_t *iterator, *next;
	PTQ_HEAD(, pt_alarm_t) runq;

	gettimeofday(&tv, NULL);
	TIMEVAL_TO_TIMESPEC(&tv, &ts);

	PTQ_INIT(&runq);
	pthread_spinlock(self, &pthread_alarmqlock);
	
	iterator = PTQ_FIRST(&pthread_alarmqueue);
	while (iterator) {
		if (timespeccmp(&ts, iterator->pta_time, <))
			break;
		next = PTQ_NEXT(iterator, pta_next);
		PTQ_REMOVE(&pthread_alarmqueue, iterator, pta_next);
		PTQ_INSERT_TAIL(&runq, iterator, pta_next);
		iterator = next;
		
	}
	pthread_spinunlock(self, &pthread_alarmqlock);

	PTQ_FOREACH(iterator, &runq, pta_next) {
		(*iterator->pta_func)(iterator->pta_arg);
		iterator->pta_fired = 1;
	}

}


