/*	$NetBSD: kill1.c,v 1.1 2004/07/27 22:01:51 yamt Exp $	*/

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
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

#include <err.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define	NTHREAD	16
struct threadinfo {
	pthread_t id;
	int gotsignal;
} th[NTHREAD];

pthread_t mainthread;

void sighandler(int);
void *f(void *);
int main(int, char *[]);

void
sighandler(int signo)
{
	pthread_t self;
	int i;

	self = pthread_self();
	if (self == mainthread || signo != SIGUSR1)
		errx(EXIT_FAILURE, "unexpected signal");

	for (i = 0; i < NTHREAD; i++)
		if (self == th[i].id)
			break;

	if (i == NTHREAD)
		errx(EXIT_FAILURE, "unknown thread");

	th[i].gotsignal++;
}

void *
f(void *arg)
{
	struct threadinfo volatile *t = arg;

	while (t->gotsignal < 1) {
		/* busy loop */
	}

	return NULL;
}

int
main(int argc, char *argv[])
{
	int ret;
	int i;
	pthread_t self;

	mainthread = pthread_self();

	if (SIG_ERR == signal(SIGUSR1, sighandler))
		err(EXIT_FAILURE, "signal");

	for (i = 0; i < NTHREAD; i++) {
		ret = pthread_create(&th[i].id, NULL, f, &th[i]);
		if (ret)
			err(EXIT_FAILURE, "pthread_create");
	}

	sched_yield();

	self = pthread_self();
	if (self != mainthread)
		errx(EXIT_FAILURE, "thread id changed");

	for (i = 0; i < NTHREAD; i++) {
		ret = pthread_kill(th[i].id, SIGUSR1);
		if (ret)
			err(EXIT_FAILURE, "pthread_kill");
	}

	for (i = 0; i < NTHREAD; i++) {
		ret = pthread_join(th[i].id, NULL);
		if (ret)
			err(EXIT_FAILURE, "pthread_join");
	}

	exit(EXIT_SUCCESS);
}

