/* $NetBSD: sem.c,v 1.2 2003/01/22 22:12:56 thorpej Exp $ */

/****************************************************************************
 *
 * Copyright (C) 2000 Jason Evans <jasone@freebsd.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************
 *
 * sem test.
 *
 * $FreeBSD: src/lib/libpthread/test/sem_d.c,v 1.2 2001/05/20 23:11:09 jasone Exp $
 *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>

#define NTHREADS 10

#define _LIBC_R_

static void *
entry(void * a_arg)
{
	pthread_t self = pthread_self();
	sem_t * sem = (sem_t *) a_arg;

#ifdef DEBUG
	printf("Thread %p waiting for semaphore...\n", self);
#endif
	sem_wait(sem);
#ifdef DEBUG
	printf("Thread %p got semaphore\n");
#endif
  
	return NULL;
}

static void ksem(void);
static void usem(void);

int
main()
{
#ifdef DEBUG
	printf("Test begin\n");
#endif
	usem();
	ksem();

#ifdef DEBUG
	printf("Test end\n");
#endif
	return 0;
}

static void
usem()
{
	sem_t sem_a, sem_b;
	pthread_t threads[NTHREADS];
	unsigned i;
	int val;
  
	/* userland sem */
	assert(0 == sem_init(&sem_b, 0, 0));
	assert(0 == sem_getvalue(&sem_b, &val));
	assert(0 == val);
  
	assert(0 == sem_post(&sem_b));
	assert(0 == sem_getvalue(&sem_b, &val));
	assert(1 == val);
  
	assert(0 == sem_wait(&sem_b));
	assert(-1 == sem_trywait(&sem_b));
	assert(EAGAIN == errno);
	assert(0 == sem_post(&sem_b));
	assert(0 == sem_trywait(&sem_b));
	assert(0 == sem_post(&sem_b));
	assert(0 == sem_wait(&sem_b));
	assert(0 == sem_post(&sem_b));


	assert(0 == sem_destroy(&sem_b));
  
	assert(0 == sem_init(&sem_a, 0, 0));

	for (i = 0; i < NTHREADS; i++) {
		pthread_create(&threads[i], NULL, entry, (void *) &sem_a);
	}

	for (i = 0; i < NTHREADS; i++) {
		sleep(1);
#ifdef DEBUG
		printf("main loop 1: posting...\n");
#endif
		assert(0 == sem_post(&sem_a));
	}
  
	for (i = 0; i < NTHREADS; i++) {
		pthread_join(threads[i], NULL);
	}
  
	for (i = 0; i < NTHREADS; i++) {
		pthread_create(&threads[i], NULL, entry, (void *) &sem_a);
	}

	for (i = 0; i < NTHREADS; i++) {
		sleep(1);
#ifdef DEBUG
		printf("main loop 2: posting...\n");
#endif
		assert(0 == sem_post(&sem_a));
	}
  
	for (i = 0; i < NTHREADS; i++) {
		pthread_join(threads[i], NULL);
	}
  
	assert(0 == sem_destroy(&sem_a));

}

static void
ksem()
{
	sem_t *sem;

	(void)sem_unlink("/foo");
	sem = sem_open("/foo", O_CREAT | O_EXCL, 0644, 0);
	assert(sem != SEM_FAILED);
	assert(-1 != sem_close(sem));
	assert(-1 != sem_unlink("/foo"));
}
