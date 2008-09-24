/*	$NetBSD: latency2.c,v 1.3.2.2 2008/09/24 04:08:38 wrstuden Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * Two threads are involved in the test.  Every 100ms the first thread
 * wakes the second.  The time taken for the wakeup to arrive is
 * measured.  Ctrl-C ends the test.
 */

#include <sys/mman.h>
#include <sys/resource.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>
#include <signal.h>
#include <err.h>
#include <lwp.h>

pthread_barrier_t	barrier;
struct timespec		start;
int 			done;
long			max = 0;
long			min = LONG_MAX;
lwpid_t			lid;
long			sum;
long			samples;

static void
sigint(int junk)
{

	done = 1;
}

static void *
reader(void *cookie)
{
	struct timespec end;
	long val;

	lid = _lwp_self();

	for (;;) {
		(void)pthread_barrier_wait(&barrier);
		while (_lwp_park(NULL, 0, &lid, NULL) != 0) {
			/* nothing */
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		timespecsub(&end, &start, &end);
		val = (long)end.tv_sec * 1000000000 + (long)end.tv_nsec;
		printf("%ld\n", val);
		if (val > max)
			max = val;
		if (val < min)
			min = val;
		sum += val;
		samples++;
	}
}

int
main(int argc, char *argv[])
{
	static struct timespec delay = { 0, 100000000 };	/* 100ms */
	struct sched_param sp;
	pthread_t pt;
	cpuset_t *cs;
	int cpuid;

	if (geteuid() != 0) {
		errx(1, "run me as root");
	}
	pthread_barrier_init(&barrier, NULL, 2);
	if (pthread_create(&pt, NULL, reader, NULL)) {
		errx(1, "pthread_create failed");
	}
	if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
		err(1, "mlockall");
	}
	if (signal(SIGINT, sigint)) {
		err(1, "siginal");
	}

	cs = calloc(sizeof(cpuset_t), 0);
	if (cs == NULL) {
		err(1, "malloc");
	}
	cpuid = pthread_curcpu_np();
	CPU_SET(cpuid, cs);
	if (_sched_setaffinity(0, 0, sizeof(cpuset_t), cs) < 0) {
		err(1, "_sched_setaffinity");
	}

	sp.sched_priority = sched_get_priority_max(SCHED_FIFO) - 2;
	if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)) {
		errx(1, "pthread_setschedparam");
	}
	sp.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if (pthread_setschedparam(pt, SCHED_FIFO, &sp)) {
		errx(1, "pthread_setschedparam");
	}

	do {
		(void)pthread_barrier_wait(&barrier);
		while (nanosleep(&delay, NULL) != 0) {
			/* nothing */
		}
		clock_gettime(CLOCK_MONOTONIC, &start);
		if (_lwp_unpark(lid, &lid) != 0) {
			err(1, "_lwp_unpark");
		}
	} while (!done);

	printf("\nmin=%ldns, max=%ldns, mean=%ldns\n", min, max, sum / samples);
	return 0;
}
