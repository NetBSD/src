/*	$NetBSD: kernel2.c,v 1.1 2009/08/07 20:57:56 haad Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: kernel2.c,v 1.1 2009/08/07 20:57:56 haad Exp $");

#include <sys/zfs_context.h>

#include <assert.h>
#include <err.h>

#define	GET(ptr)		(*(void **)ptr)
#define	SET(ptr, val)		(*(void **)ptr = val)

void
mutex_init(kmutex_t *mtx, void *junk1, kmutex_type_t type, void *junk2)
{
	pthread_mutex_t *mutex;
	int rv;

	mutex = malloc(sizeof(*mutex));
	if (mutex == NULL) {
		err(1, "mutex_init: malloc\n");
	}
	rv = pthread_mutex_init(mutex, NULL);
	if (rv != 0) {
		errx(1, "mutex_init: err %d\n", rv);
	}
	SET(mtx, mutex);
}

void
mutex_destroy(kmutex_t *mtx)
{
	pthread_mutex_t *mutex = GET(mtx);

	pthread_mutex_destroy(mutex);
	free(mutex);
}

void
mutex_enter(kmutex_t *mtx)
{

	pthread_mutex_lock(GET(mtx));
}

int
mutex_tryenter(kmutex_t *mtx)
{

	return (pthread_mutex_trylock(GET(mtx)) == 0);
}

void
mutex_exit(kmutex_t *mtx)
{

	pthread_mutex_unlock(GET(mtx));
}

void *
mutex_owner(kmutex_t *mtx)
{

	return pthread_mutex_owner_np(GET(mtx));
}

int
mutex_owned(kmutex_t *mtx)
{

	return pthread_mutex_held_np(GET(mtx));
}

void
rw_init(krwlock_t *rw, char *name, int type, void *arg)
{
	pthread_rwlock_t *rwlock;
	int rv;

	rwlock = malloc(sizeof(*rwlock));
	if (rwlock == NULL) {
		err(1, "rw_init: malloc\n");
	}
	rv = pthread_rwlock_init(rwlock, NULL);
	if (rv != 0) {
		errx(1, "rw_init: err %d\n", rv);
	}
	SET(rw, rwlock);
}

void
rw_destroy(krwlock_t *rw)
{
	pthread_rwlock_t *rwlock = GET(rw);
	int rv;

	rv = pthread_rwlock_destroy(rwlock);
	if (rv != 0) {
		errx(1, "rw_destroy: err %d\n", rv);
	}
	free(rwlock);
}

void
rw_enter(krwlock_t *rw, const krw_t op)
{
	pthread_rwlock_t *rwlock = GET(rw);
	int rv;

	if (op == RW_WRITER) {
		rv = pthread_rwlock_wrlock(rwlock);
		if (rv != 0) {
			errx(1, "rw_enter(RW_WRITER) err %d\n", rv);
		}
	} else {
		rv = pthread_rwlock_rdlock(rwlock);
		if (rv != 0) {
			errx(1, "rw_enter(RW_READER) err %d\n", rv);
		}
	}
}

int
rw_tryenter(krwlock_t *rw, const krw_t op)
{
	pthread_rwlock_t *rwlock = GET(rw);
	int rv;

	if (op == RW_WRITER) {
		rv = pthread_rwlock_trywrlock(rwlock);
	} else {
		rv = pthread_rwlock_tryrdlock(rwlock);
	}

	return rv == 0;
}

void
rw_exit(krwlock_t *rw)
{
	int rv;

	rv = pthread_rwlock_unlock(GET(rw));
	if (rv != 0) {
		errx(1, "rw_exit: err %d\n", rv);
	}
}

int
rw_tryupgrade(krwlock_t *rw)
{

	return 0;
}

int
rw_lock_held(krwlock_t *rw)
{

	return pthread_rwlock_held_np(GET(rw));
}

int
rw_read_held(krwlock_t *rw)
{

	return pthread_rwlock_rdheld_np(GET(rw));
}

int
rw_write_held(krwlock_t *rw)
{

	return pthread_rwlock_wrheld_np(GET(rw));
}

void
cv_init(kcondvar_t *cv, char *name, int type, void *arg)
{
	pthread_cond_t *cond;

	cond = malloc(sizeof(*cond));
	pthread_cond_init(cond, NULL);
	SET(cv, cond);
}

void
cv_destroy(kcondvar_t *cv)
{
	pthread_cond_t *cond = GET(cv);

	if (cond != NULL) {
		pthread_cond_destroy(cond);
		free(cond);
	}
}

void
cv_wait(kcondvar_t *cv, kmutex_t *mtx)
{

	if (GET(cv) == NULL) {
		cv_init(cv, NULL, 0, NULL);
	}

	pthread_cond_wait(GET(cv), GET(mtx));
}

clock_t
cv_timedwait(kcondvar_t *cv, kmutex_t *mp, clock_t abstime)
{
	struct timespec ts;
	uint64_t when;
	int error;

	if (GET(cv) == NULL) {
		cv_init(cv, NULL, 0, NULL);
	}

	/* convert back from 119hz to nanoseconds. */
	when = abstime << 23;
	ts.tv_sec = (long)(abstime / 1000000000);
	ts.tv_nsec = (long)(abstime % 1000000000);
	
	do {
		error = pthread_cond_timedwait(GET(cv), GET(mp), &ts);
	} while (error == EINTR);

	if (error == ETIMEDOUT)
		return (-1);
	assert(error == 0);

	return (1);
}

void
cv_signal(kcondvar_t *cv)
{

	if (GET(cv) == NULL) {
		cv_init(cv, NULL, 0, NULL);
	}
	pthread_cond_signal(GET(cv));
}

void
cv_broadcast(kcondvar_t *cv)
{

	if (GET(cv) == NULL) {
		cv_init(cv, NULL, 0, NULL);
	}
	pthread_cond_broadcast(GET(cv));
}
