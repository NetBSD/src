/* $NetBSD: barriers.c,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@NetBSD.org>
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

#include <sys/cdefs.h>

__RCSID("$NetBSD: barriers.c,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $");

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/param.h>

#include "barriers.h"
#include "arrayalloc.h"

/* An implementation of thread barriers, using POSIX semaphores */
/* We use a semaphore for barriers because it works for fork(2) as
 * well as pthread(3)
 */
struct barrier {
	char *semfile; /* File name of the semaphore */
	sem_t *lock; /* operates as a mutex */
	volatile size_t stile;
};


static bool
spinwait(sem_t *sem)
{
	if (sem == NULL) {
		return false;
	}
	while (sem_trywait(sem)) { /* spinwait */
		switch (errno) {
		case EAGAIN:
			continue;
		case EINVAL: /* Invalid sem */
			return false;
		default:
			assert(!"Unknown sem error");
		}
	}
	return true;
}

struct barrier *
vm_barrier_create(size_t ncpus)
{
	char _semfile[] = "/semXXXX", *semfile = _semfile;

	/* upto UCHAR_MAX barriers allowed. counting starts at 0 */

	static unsigned char sno = (unsigned char) -1; 
	sno++;
	assert(sno < UCHAR_MAX);

	struct barrier *bt;

	bt = arrayalloc(sizeof *bt);

	if (bt == NULL) {
		fprintf(stderr, "arrayalloc() failed\n");
		return NULL;
	}

	/* unique semaphore */
	if (sprintf(semfile, "/sem%04d", sno) < 0) {
		fprintf(stderr, "Unable to get unique filename for semaphore\n");
		return NULL;
	}

	bt->lock = sem_open(semfile, O_CREAT, 0600, 1);

	if (bt->lock == SEM_FAILED) {
		fprintf(stderr, "Unable to open semaphore\n");
		perror("sem_open():");
		return NULL;
	}

	bt->semfile = malloc(sizeof semfile);
	if (bt->semfile == NULL) {
		fprintf(stderr, "Unable to malloc() for filename\n");
		sem_close(bt->lock);
		sem_unlink(semfile);
		free(bt);
		return NULL;
	}

	strncpy(bt->semfile, _semfile, sizeof _semfile);
	bt->stile = ncpus;

	return bt;
}

void
vm_barrier_reset(struct barrier *bt)
{
	assert(bt != NULL);
	if (!spinwait(bt->lock)) { /* Lock is invalid */
		abort();
	}
	bt->stile = 0;
	sem_post(bt->lock);
}

void
vm_barrier_destroy(struct barrier *bt)
{
	assert(bt != NULL);
	assert(bt->lock != NULL);
	assert(bt->semfile != NULL);
	assert(bt->stile == 0);	

	sem_close(bt->lock);
	sem_unlink(bt->semfile);
	free(bt->semfile);

	arrayfree(bt, sizeof *bt);
}

void
vm_barrier_hold(struct barrier *bt)
{
	size_t stile;

	assert(bt != NULL);
	assert(bt->lock != NULL);
	assert(bt->semfile != NULL);

	if (!spinwait(bt->lock)) { /* Lock is invalid */
		abort();
	}

	if (bt->stile == 0) {
		sem_post(bt->lock);
		return;
	}

	bt->stile--;
	sem_post(bt->lock);

	do {
		if (!spinwait(bt->lock)) return; /* Bail out on an invalid lock */
		stile = bt->stile;
		sem_post(bt->lock);
	} while (stile);
}
