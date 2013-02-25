/*	$NetBSD: timer.c,v 1.10.6.1 2013/02/25 00:30:49 tls Exp $	*/
/*	$KAME: timer.c,v 1.11 2005/04/14 06:22:35 suz Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/queue.h>
#include <sys/time.h>

#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include "timer.h"

struct rtadvd_timer_head_t ra_timer = TAILQ_HEAD_INITIALIZER(ra_timer);

#define MILLION 1000000
#define TIMEVAL_EQUAL(t1,t2) ((t1)->tv_sec == (t2)->tv_sec &&\
 (t1)->tv_usec == (t2)->tv_usec)

static struct timeval tm_limit = {0x7fffffff, 0x7fffffff};
static struct timeval tm_max;

void
rtadvd_timer_init(void)
{

	TAILQ_INIT(&ra_timer);
	tm_max = tm_limit;
}

struct rtadvd_timer *
rtadvd_add_timer(struct rtadvd_timer *(*timeout) (void *),
    void (*update) (void *, struct timeval *),
    void *timeodata, void *updatedata)
{
	struct rtadvd_timer *newtimer;

	if ((newtimer = malloc(sizeof(*newtimer))) == NULL) {
		syslog(LOG_ERR,
		       "<%s> can't allocate memory", __func__);
		exit(1);
	}

	memset(newtimer, 0, sizeof(*newtimer));

	if (timeout == NULL) {
		syslog(LOG_ERR,
		       "<%s> timeout function unspecified", __func__);
		exit(1);
	}
	newtimer->expire = timeout;
	newtimer->update = update;
	newtimer->expire_data = timeodata;
	newtimer->update_data = updatedata;
	newtimer->tm = tm_max;

	/* link into chain */
	TAILQ_INSERT_TAIL(&ra_timer, newtimer, next);

	return(newtimer);
}

void
rtadvd_remove_timer(struct rtadvd_timer **timer)
{

	if (*timer) {
		TAILQ_REMOVE(&ra_timer, *timer, next);
		free(*timer);
		*timer = NULL;
	}
}

void
rtadvd_set_timer(struct timeval *tm, struct rtadvd_timer *timer)
{
	struct timeval now;

	/* reset the timer */
	gettimeofday(&now, NULL);

	TIMEVAL_ADD(&now, tm, &timer->tm);

	/* update the next expiration time */
	if (TIMEVAL_LT(timer->tm, tm_max))
		tm_max = timer->tm;

	return;
}

/*
 * Check expiration for each timer. If a timer expires,
 * call the expire function for the timer and update the timer.
 * Return the next interval for select() call.
 */
struct timeval *
rtadvd_check_timer(void)
{
	static struct timeval returnval;
	struct timeval now;
	struct rtadvd_timer *tm, *tmn;

	gettimeofday(&now, NULL);
	tm_max = tm_limit;

	TAILQ_FOREACH_SAFE(tm, &ra_timer, next, tmn) {
		if (TIMEVAL_LEQ(tm->tm, now)) {
			if ((*tm->expire)(tm->expire_data) == NULL)
				continue; /* the timer was removed */
			if (tm->update)
				(*tm->update)(tm->update_data, &tm->tm);
			TIMEVAL_ADD(&tm->tm, &now, &tm->tm);
		}

		if (TIMEVAL_LT(tm->tm, tm_max))
			tm_max = tm->tm;
	}

	if (TIMEVAL_EQUAL(&tm_max, &tm_limit)) {
		/* no need to timeout */
		return(NULL);
	} else if (TIMEVAL_LT(tm_max, now)) {
		/* this may occur when the interval is too small */
		returnval.tv_sec = returnval.tv_usec = 0;
	} else
		TIMEVAL_SUB(&tm_max, &now, &returnval);
	return(&returnval);
}

struct timeval *
rtadvd_timer_rest(struct rtadvd_timer *timer)
{
	static struct timeval returnval, now;

	gettimeofday(&now, NULL);
	if (TIMEVAL_LEQ(timer->tm, now)) {
		syslog(LOG_DEBUG,
		       "<%s> a timer must be expired, but not yet",
		       __func__);
		returnval.tv_sec = returnval.tv_usec = 0;
	}
	else
		TIMEVAL_SUB(&timer->tm, &now, &returnval);

	return(&returnval);
}

/* result = a + b */
void
TIMEVAL_ADD(struct timeval *a, struct timeval *b, struct timeval *result)
{
	long l;

	if ((l = a->tv_usec + b->tv_usec) < MILLION) {
		result->tv_usec = l;
		result->tv_sec = a->tv_sec + b->tv_sec;
	}
	else {
		result->tv_usec = l - MILLION;
		result->tv_sec = a->tv_sec + b->tv_sec + 1;
	}
}

/*
 * result = a - b
 * XXX: this function assumes that a >= b.
 */
void
TIMEVAL_SUB(struct timeval *a, struct timeval *b, struct timeval *result)
{
	long l;

	if ((l = a->tv_usec - b->tv_usec) >= 0) {
		result->tv_usec = l;
		result->tv_sec = a->tv_sec - b->tv_sec;
	}
	else {
		result->tv_usec = MILLION + l;
		result->tv_sec = a->tv_sec - b->tv_sec - 1;
	}
}
