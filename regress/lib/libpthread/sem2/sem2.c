/*	$NetBSD: sem2.c,v 1.1 2004/11/03 15:18:35 yamt Exp $	*/

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
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

sem_t sem;

void sighandler(int);
void *f(void *);
int main(int, char *[]);

void
sighandler(int signo)
{

/*	printf("signal %d\n", signo); */

	if (signo != SIGALRM)
		errx(EXIT_FAILURE, "unexpected signal %d", signo);

	if (sem_post(&sem))
		err(EXIT_FAILURE, "sem_post");
}

void *
f(void *arg)
{
	int i;

	for (i = 0; i < 10; ) {
		int ret;

		if ((i & 1) != 0)
			ret = sem_wait(&sem);
		else
			ret = sem_trywait(&sem);
		if (ret) {
			if ((i & 1) !=0 || errno != EAGAIN)
				err(EXIT_FAILURE, "sem_trywait");
			continue;
		}
		printf("%s: %d\n", __func__, i);
		alarm(1);
		i++;
	}

	return NULL;
}

int
main(int argc, char *argv[])
{
#if defined(USE_PTHREAD)
	int ret;
	pthread_t t;
#endif

	alarm(1);

	if (sem_init(&sem, 0, 0))
		err(EXIT_FAILURE, "sem_init");

#if defined(USE_PTHREAD)
	ret = pthread_create(&t, NULL, f, NULL);
	if (ret)
		err(EXIT_FAILURE, "pthread_create");
#endif

	if (SIG_ERR == signal(SIGALRM, sighandler))
		err(EXIT_FAILURE, "signal");

#if defined(USE_PTHREAD)
	ret = pthread_join(t, NULL);
	if (ret)
		err(EXIT_FAILURE, "pthread_join");
#else
	f(NULL);
#endif

	exit(EXIT_SUCCESS);
}
