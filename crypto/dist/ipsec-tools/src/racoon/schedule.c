/*	$NetBSD: schedule.c,v 1.7 2009/01/23 09:10:13 tteras Exp $	*/

/*	$KAME: schedule.c,v 1.19 2001/11/05 10:53:19 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * Copyright (C) 2008 Timo Teras.
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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "misc.h"
#include "plog.h"
#include "schedule.h"
#include "var.h"
#include "gcmalloc.h"

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(elm, head, field) \
        for (elm = TAILQ_FIRST(head); elm; elm = TAILQ_NEXT(elm, field))
#endif

static TAILQ_HEAD(_schedtree, sched) sctree;

void
sched_get_monotonic_time(tv)
	struct timeval *tv;
{
#ifdef HAVE_CLOCK_MONOTONIC
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / 1000;
#else
	gettimeofday(tv, NULL);
#endif
}

time_t
sched_monotonic_to_time_t(tv, now)
	struct timeval *tv, *now;
{
#ifdef HAVE_CLOCK_MONOTONIC
	struct timeval mynow, res;

	if (now == NULL) {
		sched_get_monotonic_time(&mynow);
		now = &mynow;
	}
	timersub(now, tv, &res);

	return time(NULL) + res.tv_sec;
#else
	return tv->tv_sec;
#endif
}

/*
 * schedule handler
 * OUT:
 *	time to block until next event.
 *	if no entry, NULL returned.
 */
struct timeval *
schedular()
{
	static struct timeval timeout;
	struct timeval now;
	struct sched *p;

	sched_get_monotonic_time(&now);
	while (!TAILQ_EMPTY(&sctree) &&
		timercmp(&TAILQ_FIRST(&sctree)->xtime, &now, <=)) {
		void (*func)(struct sched *);

		p = TAILQ_FIRST(&sctree);
		func = p->func;
		sched_cancel(p);
		func(p);
	}

	p = TAILQ_FIRST(&sctree);
	if (p == NULL)
		return NULL;

	timersub(&p->xtime, &now, &timeout);

	return &timeout;
}

/*
 * add new schedule to schedule table.
 */
void
sched_schedule(sc, tick, func)
	struct sched *sc;
	time_t tick;
	void (*func) __P((struct sched *));
{
	static long id = 1;
	struct sched *p;
	struct timeval now;

	sched_cancel(sc);

	sc->func = func;
	sc->id = id++;
	sc->tick.tv_sec = tick;
	sc->tick.tv_usec = 0;
	sched_get_monotonic_time(&now);
	timeradd(&now, &sc->tick, &sc->xtime);

	/* add to schedule table */
	TAILQ_FOREACH(p, &sctree, chain) {
		if (timercmp(&sc->xtime, &p->xtime, <))
			break;
	}
	if (p == NULL)
		TAILQ_INSERT_TAIL(&sctree, sc, chain);
	else
		TAILQ_INSERT_BEFORE(p, sc, chain);
}

/*
 * cancel scheduled callback
 */
void
sched_cancel(sc)
	struct sched *sc;
{
	if (sc->func != NULL) {
		TAILQ_REMOVE(&sctree, sc, chain);
		sc->func = NULL;
	}
}

/*
 * for debug
 */
int
sched_dump(buf, len)
	caddr_t *buf;
	int *len;
{
	caddr_t new;
	struct sched *p;
	struct scheddump *dst;
	struct timeval now, created;
	int cnt = 0;

	/* initialize */
	*len = 0;
	*buf = NULL;

	TAILQ_FOREACH(p, &sctree, chain)
		cnt++;

	/* no entry */
	if (cnt == 0)
		return -1;

	*len = cnt * sizeof(*dst);

	new = racoon_malloc(*len);
	if (new == NULL)
		return -1;
	dst = (struct scheddump *)new;

	sched_get_monotonic_time(&now);
	p = TAILQ_FIRST(&sctree);
	while (p) {
		timersub(&p->xtime, &p->tick, &created);
		dst->xtime = p->xtime.tv_sec;
		dst->id = p->id;
		dst->created = sched_monotonic_to_time_t(&created, &now);
		dst->tick = p->tick.tv_sec;

		p = TAILQ_NEXT(p, chain);
		if (p == NULL)
			break;
		dst++;
	}

	*buf = new;

	return 0;
}

/* initialize schedule table */
void
sched_init()
{
	TAILQ_INIT(&sctree);
}

#ifdef STEST
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <err.h>

void
test(tick)
	int *tick;
{
	printf("execute %d\n", *tick);
	racoon_free(tick);
}

void
getstdin()
{
	int *tick;
	char buf[16];

	read(0, buf, sizeof(buf));
	if (buf[0] == 'd') {
		struct scheddump *scbuf, *p;
		int len;
		sched_dump((caddr_t *)&scbuf, &len);
		if (scbuf == NULL)
			return;
		for (p = scbuf; len; p++) {
			printf("xtime=%ld\n", p->xtime);
			len -= sizeof(*p);
		}
		racoon_free(scbuf);
		return;
	}

	tick = (int *)racoon_malloc(sizeof(*tick));
	*tick = atoi(buf);
	printf("new queue tick = %d\n", *tick);
	sched_new(*tick, test, tick);
}

int
main()
{
	static fd_set mask0;
	int nfds = 0;
	fd_set rfds;
	struct timeval *timeout;
	int error;

	FD_ZERO(&mask0);
	FD_SET(0, &mask0);
	nfds = 1;

	/* initialize */
	sched_init();

	while (1) {
		rfds = mask0;

		timeout = schedular();

		error = select(nfds, &rfds, (fd_set *)0, (fd_set *)0, timeout);
		if (error < 0) {
			switch (errno) {
			case EINTR: continue;
			default:
				err(1, "select");
			}
			/*NOTREACHED*/
		}

		if (FD_ISSET(0, &rfds))
			getstdin();
	}
}
#endif
