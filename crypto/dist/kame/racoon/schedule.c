/*	$KAME: schedule.c,v 1.19 2001/11/05 10:53:19 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/queue.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"
#include "plog.h"
#include "schedule.h"
#include "var.h"
#include "gcmalloc.h"

#define FIXY2038PROBLEM

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(elm, head, field) \
        for (elm = TAILQ_FIRST(head); elm; elm = TAILQ_NEXT(elm, field))
#endif

static struct timeval timeout;

#ifdef FIXY2038PROBLEM
#define Y2038TIME_T	0x7fffffff
static time_t launched;		/* time when the program launched. */
static time_t deltaY2038;
#endif

static TAILQ_HEAD(_schedtree, sched) sctree;

static void sched_add __P((struct sched *));
static time_t current_time __P((void));

/*
 * schedule handler
 * OUT:
 *	time to block until next event.
 *	if no entry, NULL returned.
 */
struct timeval *
schedular()
{
	time_t now, delta;
	struct sched *p, *next = NULL;

	now = current_time();

        for (p = TAILQ_FIRST(&sctree); p; p = next) {
		/* if the entry has been daed, remove it */
		if (p->dead)
			goto next_schedule;

		/* if the time hasn't come, proceed to the next entry */
		if (now < p->xtime) {
			next = TAILQ_NEXT(p, chain);
			continue;
		}

		/* mark it with dead. and call the function. */
		p->dead = 1;
		if (p->func != NULL)
			(p->func)(p->param);

	   next_schedule:
		next = TAILQ_NEXT(p, chain);
		TAILQ_REMOVE(&sctree, p, chain);
		racoon_free(p);
	}

	p = TAILQ_FIRST(&sctree);
	if (p == NULL)
		return NULL;

	now = current_time();

	delta = p->xtime - now;
	timeout.tv_sec = delta < 0 ? 0 : delta;
	timeout.tv_usec = 0;

	return &timeout;
}

/*
 * add new schedule to schedule table.
 */
struct sched *
sched_new(tick, func, param)
	time_t tick;
	void (*func) __P((void *));
	void *param;
{
	static long id = 1;
	struct sched *new;

	new = (struct sched *)racoon_malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	memset(new, 0, sizeof(*new));
	new->func = func;
	new->param = param;

	new->id = id++;
	time(&new->created);
	new->tick = tick;

	new->xtime = current_time() + tick;
	new->dead = 0;

	/* add to schedule table */
	sched_add(new);

	return(new);
}

/* add new schedule to schedule table */
static void
sched_add(sc)
	struct sched *sc;
{
	struct sched *p;

	TAILQ_FOREACH(p, &sctree, chain) {
		if (sc->xtime < p->xtime) {
			TAILQ_INSERT_BEFORE(p, sc, chain);
			return;
		}
	}
	if (p == NULL)
		TAILQ_INSERT_TAIL(&sctree, sc, chain);

	return;
}

/* get current time.
 * if defined FIXY2038PROBLEM, base time is the time when called sched_init().
 * Otherwise, conform to time(3).
 */
static time_t
current_time()
{
	time_t n;
#ifdef FIXY2038PROBLEM
	time_t t;

	time(&n);
	t = n - launched;
	if (t < 0)
		t += deltaY2038;

	return t;
#else
	return time(&n);
#endif
}

void
sched_kill(sc)
	struct sched *sc;
{
	sc->dead = 1;

	return;
}

/* XXX this function is probably unnecessary. */
void
sched_scrub_param(param)
	void *param;
{
	struct sched *sc;

	TAILQ_FOREACH(sc, &sctree, chain) {
		if (sc->param == param) {
			if (!sc->dead) {
				plog(LLV_DEBUG, LOCATION, NULL,
				    "an undead schedule has been deleted.\n");
			}
			sched_kill(sc);
		}
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

        p = TAILQ_FIRST(&sctree);
	while (p) {
		dst->xtime = p->xtime;
		dst->id = p->id;
		dst->created = p->created;
		dst->tick = p->tick;

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
#ifdef FIXY2038PROBLEM
	time(&launched);

	deltaY2038 = Y2038TIME_T - launched;
#endif

	TAILQ_INIT(&sctree);

	return;
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
		if (buf == NULL)
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
