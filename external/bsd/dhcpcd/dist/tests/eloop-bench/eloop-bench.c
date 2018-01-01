/*
 * eloop benchmark
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/resource.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "eloop.h"

#ifndef timespecsub
#define timespecsub(tsp, usp, vsp)                                      \
        do {                                                            \
                (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;          \
                (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;       \
                if ((vsp)->tv_nsec < 0) {                               \
                        (vsp)->tv_sec--;                                \
                        (vsp)->tv_nsec += 1000000000L;                  \
                }                                                       \
        } while (/* CONSTCOND */ 0)
#endif

struct pipe {
	int fd[2];
};

static size_t good, bad, writes, fired;
static size_t npipes = 100, nwrites = 100, nactive = 1;
static struct pipe *pipes;
static struct eloop *e;

static void
read_cb(void *arg)
{
	struct pipe *p = arg;
	unsigned char c;
	ssize_t l;

	l = read(p->fd[0], &c, sizeof(c));
	if (l == -1)
		bad++;
	else
		good += (size_t)l;
	if (writes) {
		p = (struct pipe *)arg;
		l = write(p->fd[1], "e", 1);
		if (l != 1)
			bad++;
		else {
			writes -= (size_t)l;
			fired += (size_t)l;
		}
	}

	if (writes == 0) {
		if (good == fired)
			eloop_exit(e, EXIT_SUCCESS);
	}
}

static struct timespec *
runone(void)
{
	size_t i;
	struct pipe *p;
	static struct timespec _ts;
	struct timespec ts, te;

	writes = nwrites;
	fired = good = 0;

	for (i = 0, p = pipes; i < nactive; i++, p++) {
		if (write(p->fd[1], "e", 1) != 1)
			err(EXIT_FAILURE, "send");
	}

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
		err(EXIT_FAILURE, "clock_gettime");
	(void) eloop_start(e, NULL);
	if (clock_gettime(CLOCK_MONOTONIC, &te) == -1)
		err(EXIT_FAILURE, "clock_gettime");

	timespecsub(&te, &ts, &_ts);
	return &_ts;
}

int
main(int argc, char **argv)
{
	int c;
	size_t i, nruns = 25;
	struct pipe *p;
	struct timespec *ts;

	if ((e = eloop_new()) == NULL)
		err(EXIT_FAILURE, "eloop_init");

	while ((c = getopt(argc, argv, "a:n:r:w:")) != -1) {
		switch (c) {
		case 'a':
			nactive = (size_t)atoi(optarg);
			break;
		case 'n':
			npipes = (size_t)atoi(optarg);
			break;
		case 'r':
			nruns = (size_t)atoi(optarg);
			break;
		case 'w':
			nwrites = (size_t)atoi(optarg);
			break;
		default:
			errx(EXIT_FAILURE, "illegal argument `%c'", c);
		}
	}

	if (nactive > npipes)
		nactive = npipes;

	pipes = malloc(sizeof(*p) * npipes);
	if (pipes == NULL)
		err(EXIT_FAILURE, "malloc");

	for (i = 0, p = pipes; i < npipes; i++, p++) {
		if (pipe(p->fd) == -1)
			err(EXIT_FAILURE, "pipe");
		if (eloop_event_add(e, p->fd[0], read_cb, p) == -1)
			err(EXIT_FAILURE, "eloop_event_add");
	}

	for (i = 0; i < nruns; i++) {
		if ((ts = runone()) == NULL)
			err(EXIT_FAILURE, "runone");
		printf("%lld.%.9ld\n", (long long)ts->tv_sec, ts->tv_nsec);
	}

	eloop_free(e);
	free(pipes);
	exit(0);
}
