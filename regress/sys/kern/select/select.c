/*	$NetBSD: select.c,v 1.3 2011/11/02 16:49:12 yamt Exp $	*/

/*-
 * Copyright (c)2008 YAMAMOTO Takashi,
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

#define	FD_SETSIZE	65536
#include <sys/select.h>
#include <sys/atomic.h>
#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	NPIPE	128
#define	NTHREAD	64
#define	NBALLS	5
#define	VERBOSE	0

#if !defined(RANDOM_MAX)
#define	RANDOM_MAX	((1UL << 31) - 1)
#endif

int fds[NPIPE][2];

volatile unsigned count;

pthread_barrier_t barrier;

static void
dowrite(void)
{
	char buf[1];
	int fd;
	int i;

	i = random() % NPIPE;
	fd = fds[i][1];
#if VERBOSE
	printf("[%p] write %d\n", (void *)pthread_self(), fd);
#endif
	if (write(fd, buf, sizeof(buf)) == -1) {
		perror("write");
		abort();
	}
}

static void *
f(void *dummy)
{

	pthread_barrier_wait(&barrier);

	for (;;) {
		struct timeval to;
		fd_set oset;
		fd_set set;
		int maxfd = -1;
		int nfd = 0;
		int ret;
		int fd;
		int i;

		FD_ZERO(&set);
		do {
			for (i = 0; i < NPIPE; i++) {
				fd = fds[i][0];
				if (fd > FD_SETSIZE) {
					fprintf(stderr,
					    "fd(%d) > FD_SETSIZE(%d)\n",
					    fd, FD_SETSIZE);
					abort();
				}
				if (random() & 1) {
					assert(!FD_ISSET(fd, &set));
					FD_SET(fd, &set);
					nfd++;
					if (fd > maxfd) {
						maxfd = fd;
					}
				}
			}
		} while (nfd == 0);
		memcpy(&oset, &set, sizeof(oset));
		memset(&to, 0, sizeof(to));
		to.tv_sec = random() % 10;
		to.tv_usec = random() % 1000000;
#if VERBOSE
		printf("[%p] select start to=%lu\n", (void *)pthread_self(),
		    (unsigned long)to.tv_sec);
#endif
		ret = select(maxfd + 1, &set, NULL, NULL, &to);
#if VERBOSE
		printf("[%p] select done ret=%d\n",
		    (void *)pthread_self(), ret);
#endif
		if (ret == -1) {
			perror("select");
			abort();
		}
		if (ret > nfd) {
			fprintf(stderr, "[%p] unexpected return value %d\n",
			    (void *)pthread_self(), ret);
			abort();
		}
		if (ret > NBALLS) {
			fprintf(stderr, "[%p] unexpected return value %d"
			    " > NBALLS\n",
			    (void *)pthread_self(), ret);
			abort();
		}
		nfd = 0;
		for (fd = 0; fd <= maxfd; fd++) {
			if (FD_ISSET(fd, &set)) {
				char buf[1];

#if VERBOSE
				printf("[%p] read %d\n",
				    (void *)pthread_self(), fd);
#endif
				if (!FD_ISSET(fd, &oset)) {
					fprintf(stderr, "[%p] unexpected\n",
					    (void *)pthread_self());
					abort();
				}
				if (read(fd, buf, sizeof(buf)) == -1) {
					if (errno != EAGAIN) {
						perror("read");
						abort();
					}
				} else {
					dowrite();
					atomic_inc_uint(&count);
				}
				nfd++;
			}
		}
		if (ret != nfd) {
			fprintf(stderr, "[%p] ret(%d) != nfd(%d)\n",
			    (void *)pthread_self(), ret, nfd);
			abort();
		}
	}
}

int
main(int argc, char *argv[])
{
	pthread_t pt[NTHREAD];
	int i;
	unsigned int secs;
	struct timeval start_tv;
	struct timeval end_tv;
	uint64_t usecs;
	unsigned int result;

	secs = atoi(argv[1]);

	for (i = 0; i < NPIPE; i++) {
		if (pipe(fds[i])) {
			perror("pipe");
			abort();
		}
		if (fcntl(fds[i][0], F_SETFL, O_NONBLOCK) == -1) {
			perror("fcntl");
			abort();
		}
	}
	pthread_barrier_init(&barrier, NULL, NTHREAD + 1);
	for (i = 0; i < NTHREAD; i++) {
		int error = pthread_create(&pt[i], NULL, f, NULL);
		if (error) {
			errno = error;
			perror("pthread_create");
			abort();
		}
	}
	pthread_barrier_wait(&barrier);
	gettimeofday(&start_tv, NULL);
	assert(count == 0);
	for (i = 0; i < NBALLS; i++) {
		dowrite();
	}
	sleep(secs);
	gettimeofday(&end_tv, NULL);
	result = count;
	usecs = (end_tv.tv_sec - start_tv.tv_sec) * 1000000
	    + end_tv.tv_usec - start_tv.tv_usec;
	printf("%u / %f = %f\n", result, (double)usecs / 1000000,
	    (double)result / usecs * 1000000);
	exit(EXIT_SUCCESS);
}
