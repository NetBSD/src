/*	$NetBSD: sem.c,v 1.2 2003/02/28 05:29:48 matt Exp $	*/

/*
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
 */

#include <sys/wait.h>
#include <assert.h>
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define NCHILDREN 10

static void
child(sem_t *sem)
{

#ifdef DEBUG
	printf("PID %d waiting for semaphore...\n", getpid());
#endif
	if (sem_wait(sem))
		err(1, "sem_wait");
#ifdef DEBUG
	printf("PID %d got semaphore\n", getpid());
#endif
}

static sem_t *
create_sem(const char *name)
{
	sem_t *sem;

	(void)sem_unlink(name);
	sem = sem_open(name, O_CREAT | O_EXCL, 0644, 0);
	assert(sem != SEM_FAILED);

	return (sem);
}

static void
delete_sem(sem_t *sem, const char *name)
{

	assert(0 == sem_close(sem));
	assert(0 == sem_unlink(name));
}

static void
dosem(void)
{
	sem_t *sem_a, *sem_b;
	pid_t children[NCHILDREN];
	unsigned i;
	int val, status;
	pid_t pid;

	sem_b = create_sem("/sem_b");
	assert(0 == sem_getvalue(sem_b, &val));
	assert(0 == val);
  
	assert(0 == sem_post(sem_b));
	assert(0 == sem_getvalue(sem_b, &val));
	assert(1 == val);
  
	assert(0 == sem_wait(sem_b));
	assert(-1 == sem_trywait(sem_b));
	assert(EAGAIN == errno);
	assert(0 == sem_post(sem_b));
	assert(0 == sem_trywait(sem_b));
	assert(0 == sem_post(sem_b));
	assert(0 == sem_wait(sem_b));
	assert(0 == sem_post(sem_b));

	delete_sem(sem_b, "/sem_b");

	sem_a = create_sem("/sem_a");

	for (i = 0; i < NCHILDREN; i++) {
		switch ((pid = fork())) {
		case -1:
			abort();
		case 0:
			child(sem_a);
			_exit(0);
		default:
			children[i] = pid;
			break;
		}
	}

	for (i = 0; i < NCHILDREN; i++) {
		sleep(1);
#ifdef DEBUG
		printf("main loop 1: posting...\n");
#endif
		assert(0 == sem_post(sem_a));
	}
  
	for (i = 0; i < NCHILDREN; i++) {
		assert(children[i] == waitpid(children[i], &status, 0));
		assert(WIFEXITED(status));
		assert(0 == WEXITSTATUS(status));
	}
  
	for (i = 0; i < NCHILDREN; i++) {
		switch ((pid = fork())) {
		case -1:
			abort();
		case 0:
			child(sem_a);
			_exit(0);
		default:
			children[i] = pid;
			break;
		}
	}

	for (i = 0; i < NCHILDREN; i++) {
		sleep(1);
#ifdef DEBUG
		printf("main loop 2: posting...\n");
#endif
		assert(0 == sem_post(sem_a));
	}
  
	for (i = 0; i < NCHILDREN; i++) {
		assert(children[i] == waitpid(children[i], &status, 0));
		assert(WIFEXITED(status));
		assert(0 == WEXITSTATUS(status));
	}
  
	delete_sem(sem_a, "/sem_a");
}

int
main(int argc, char *argv[])
{
#ifdef DEBUG
	printf("Test begin\n");
#endif
	dosem();

#ifdef DEBUG
	printf("Test end\n");
#endif
	return 0;
}
