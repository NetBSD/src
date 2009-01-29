/*	$NetBSD: nameibench.c,v 1.1 2009/01/29 21:24:19 ad Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <signal.h>

#define	MAXFILES	4096

const int primes[] = {
	233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283,
	293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359,
	367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431,
	433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491,
	499, 503, 509, 521, 523, 541, 
};

char	**sa;
int	sasize;
int	sacnt;
struct	timespec sts;
struct	timespec ets;
pthread_barrier_t	barrier;
pthread_mutex_t	lock;
volatile sig_atomic_t	stop;
double	nlookups;
double	ideal;

static void
makelist(void)
{
	char buf[MAXPATHLEN], *p;
	size_t l;
	FILE *fp;

	fp = popen("/usr/bin/locate x", "r");
	if (fp == NULL) {
		perror("popen");
		exit(EXIT_FAILURE);
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		l = strlen(buf) + 1;
		p = malloc(l);
		if (p == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		strcpy(p, buf);
		p[l - 2] = '\0';
		if (sacnt == sasize) {
			sasize += 256;
			sa = realloc(sa, sizeof(*sa) * sasize);
			if (sa == NULL) {
				perror("realloc");
				exit(EXIT_FAILURE);
			}
		}
		sa[sacnt++] = p;
		if (sacnt == MAXFILES) {
			break;
		}
	}

	fclose(fp);
}

static void
lookups(int idx)
{
	unsigned int p, c;

	for (c = 0, p = 0; !stop; c++) {
		p += primes[idx];
		if (p >= sacnt) {
			p %= sacnt;
		}
		(void)access(sa[p], F_OK);
		
	}

	pthread_mutex_lock(&lock);
	nlookups += c;
	pthread_mutex_unlock(&lock);
}

static void
start(void)
{

	(void)pthread_barrier_wait(&barrier);
	if (clock_gettime(CLOCK_MONOTONIC, &sts)) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
}

static void
end(void)
{

	if (clock_gettime(CLOCK_MONOTONIC, &ets)) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
	(void)pthread_barrier_wait(&barrier);
}

static void *
thread(void *cookie)
{

	start();
	lookups((int)(uintptr_t)cookie);
	end();

	return NULL;
}

static void
sigalrm(int junk)
{

	stop = (sig_atomic_t)1;
}

static void
run(int nt)
{
	pthread_t pt;
	double c;
	int i;
	long us;

	if (pthread_barrier_init(&barrier, NULL, nt + 1)) {
		fprintf(stderr, "pthread_barrier_init\n");
		exit(EXIT_FAILURE);
	}

	nlookups = 0;
	stop = 0;
	for (i = 0; i < nt; i++) {
		if (pthread_create(&pt, NULL, thread, (void *)(uintptr_t)i)) {
			fprintf(stderr, "pthread_create\n");
			exit(EXIT_FAILURE);
		}
	}
	start();
	alarm(10);
	end();
	us = (long)(ets.tv_sec * (uint64_t)1000000 + ets.tv_nsec / 1000);
	us -= (long)(sts.tv_sec * (uint64_t)1000000 + sts.tv_nsec / 1000);
	c = nlookups * 1000000.0 / us;
	if (ideal == 0) {
		ideal = c;
	}
	printf("%d\t%d\t%.0f\t%.0f\n", sacnt, nt, c, ideal * nt);

	if (pthread_barrier_destroy(&barrier)) {
		fprintf(stderr, "pthread_barrier_destroy\n");
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char **argv)
{
	int i, mt;

	(void)setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	if (argc < 2) {
		mt = sysconf(_SC_NPROCESSORS_ONLN);
	} else {
		mt = atoi(argv[1]);
		if (mt < 1) {
			mt = 1;
		}
	}
	if (pthread_mutex_init(&lock, NULL)) {
		fprintf(stderr, "pthread_mutex_init\n");
		exit(EXIT_FAILURE);
	}
	makelist();
	(void)signal(SIGALRM, sigalrm);
	printf("# nname\tnthr\tpersec\tideal\n");
	for (i = 1; i <= mt; i++) {
		run(i);	
	}
	exit(EXIT_SUCCESS);
}
