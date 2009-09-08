/*	$NetBSD: rumpuser_pth.c,v 1.35 2009/09/08 20:04:03 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: rumpuser_pth.c,v 1.35 2009/09/08 20:04:03 pooka Exp $");
#endif /* !lint */

#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64
#endif

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <rump/rumpuser.h>

#include "rumpuser_int.h"

static pthread_key_t curlwpkey;

#define NOFAIL(a) do {if (!(a)) abort();} while (/*CONSTCOND*/0)
#define NOFAIL_ERRNO(a)							\
do {									\
	int fail_rv = (a);						\
	if (fail_rv) {							\
		printf("panic: rumpuser fatal failure %d (%s)\n",	\
		    fail_rv, strerror(fail_rv));			\
		    abort();						\
	}								\
} while (/*CONSTCOND*/0)

#define RUMTX_INCRECURSION(mtx) ((mtx)->recursion++)
#define RUMTX_DECRECURSION(mtx) ((mtx)->recursion--)
struct rumpuser_mtx {
	pthread_mutex_t pthmtx;
	pthread_t owner;
	unsigned recursion;
};

#define RURW_AMWRITER(rw) (pthread_equal(rw->writer, pthread_self())	\
				&& rw->readers == -1)
#define RURW_HASREAD(rw)  (rw->readers > 0)

#define RURW_SETWRITE(rw)						\
do {									\
	assert(rw->readers == 0);					\
	rw->writer = pthread_self();					\
	rw->readers = -1;						\
} while (/*CONSTCOND*/0)
#define RURW_CLRWRITE(rw)						\
do {									\
	assert(rw->readers == -1 && RURW_AMWRITER(rw));			\
	rw->readers = 0;						\
} while (/*CONSTCOND*/0)
#define RURW_INCREAD(rw)						\
do {									\
	pthread_spin_lock(&rw->spin);					\
	assert(rw->readers >= 0);					\
	++(rw)->readers;						\
	pthread_spin_unlock(&rw->spin);					\
} while (/*CONSTCOND*/0)
#define RURW_DECREAD(rw)						\
do {									\
	pthread_spin_lock(&rw->spin);					\
	assert(rw->readers > 0);					\
	--(rw)->readers;						\
	pthread_spin_unlock(&rw->spin);					\
} while (/*CONSTCOND*/0)

struct rumpuser_rw {
	pthread_rwlock_t pthrw;
	pthread_spinlock_t spin;
	int readers;
	pthread_t writer;
};

struct rumpuser_cv {
	pthread_cond_t pthcv;
	int nwaiters;
};

struct rumpuser_mtx rumpuser_aio_mtx;
struct rumpuser_cv rumpuser_aio_cv;
int rumpuser_aio_head, rumpuser_aio_tail;
struct rumpuser_aio rumpuser_aios[N_AIOS];

kernel_lockfn	rumpuser__klock;
kernel_unlockfn	rumpuser__kunlock;
int		rumpuser__wantthreads;

static void *
/*ARGSUSED*/
iothread(void *arg)
{
	struct rumpuser_aio *rua;
	rump_biodone_fn biodone = arg;
	ssize_t rv;
	int error;

	NOFAIL_ERRNO(pthread_mutex_lock(&rumpuser_aio_mtx.pthmtx));
	for (;;) {
		while (rumpuser_aio_head == rumpuser_aio_tail) {
			NOFAIL_ERRNO(pthread_cond_wait(&rumpuser_aio_cv.pthcv,
			    &rumpuser_aio_mtx.pthmtx));
		}

		rua = &rumpuser_aios[rumpuser_aio_tail];
		assert(rua->rua_bp != NULL);
		pthread_mutex_unlock(&rumpuser_aio_mtx.pthmtx);

		if (rua->rua_op & RUA_OP_READ) {
			error = 0;
			rv = pread(rua->rua_fd, rua->rua_data,
			    rua->rua_dlen, rua->rua_off);
			if (rv < 0) {
				rv = 0;
				error = errno;
			}
		} else {
			error = 0;
			rv = pwrite(rua->rua_fd, rua->rua_data,
			    rua->rua_dlen, rua->rua_off);
			if (rv < 0) {
				rv = 0;
				error = errno;
			} else if (rua->rua_op & RUA_OP_SYNC) {
#ifdef __NetBSD__
				fsync_range(rua->rua_fd, FDATASYNC,
				    rua->rua_off, rua->rua_dlen);
#else
				fsync(rua->rua_fd);
#endif
			}
		}
		biodone(rua->rua_bp, rv, error);
			
		rua->rua_bp = NULL;

		NOFAIL_ERRNO(pthread_mutex_lock(&rumpuser_aio_mtx.pthmtx));
		rumpuser_aio_tail = (rumpuser_aio_tail+1) % N_AIOS;
		pthread_cond_signal(&rumpuser_aio_cv.pthcv);
	}
}

void
rumpuser_thrinit(kernel_lockfn lockfn, kernel_unlockfn unlockfn, int threads)
{

	pthread_mutex_init(&rumpuser_aio_mtx.pthmtx, NULL);
	pthread_cond_init(&rumpuser_aio_cv.pthcv, NULL);

	pthread_key_create(&curlwpkey, NULL);

	rumpuser__klock = lockfn;
	rumpuser__kunlock = unlockfn;
	rumpuser__wantthreads = threads;
}

int
rumpuser_bioinit(rump_biodone_fn biodone)
{
	pthread_t iothr;

	if (rumpuser__wantthreads)
		pthread_create(&iothr, NULL, iothread, biodone);

	return 0;
}

#if 0
void
rumpuser__thrdestroy(void)
{

	pthread_key_delete(curlwpkey);
}
#endif

int
rumpuser_thread_create(void *(*f)(void *), void *arg, const char *thrname)
{
	pthread_t ptid;
	int rv;

	rv = pthread_create(&ptid, NULL, f, arg);
#ifdef __NetBSD__
	if (rv == 0 && thrname)
		pthread_setname_np(ptid, thrname, NULL);
#endif

	return rv;
}

__dead void
rumpuser_thread_exit(void)
{

	pthread_exit(NULL);
}

void
rumpuser_mutex_init(struct rumpuser_mtx **mtx)
{
	pthread_mutexattr_t att;

	NOFAIL(*mtx = malloc(sizeof(struct rumpuser_mtx)));

	pthread_mutexattr_init(&att);
	pthread_mutexattr_settype(&att, PTHREAD_MUTEX_ERRORCHECK);
	NOFAIL_ERRNO(pthread_mutex_init(&((*mtx)->pthmtx), &att));
	pthread_mutexattr_destroy(&att);

	(*mtx)->owner = NULL;
	(*mtx)->recursion = 0;
}

void
rumpuser_mutex_recursive_init(struct rumpuser_mtx **mtx)
{
	pthread_mutexattr_t mattr;

	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);

	NOFAIL(*mtx = malloc(sizeof(struct rumpuser_mtx)));
	NOFAIL_ERRNO(pthread_mutex_init(&((*mtx)->pthmtx), &mattr));
	(*mtx)->owner = NULL;
	(*mtx)->recursion = 0;

	pthread_mutexattr_destroy(&mattr);
}

static void
mtxenter(struct rumpuser_mtx *mtx)
{

	if (mtx->recursion++ == 0) {
		assert(mtx->owner == NULL);
		mtx->owner = pthread_self();
	} else {
		assert(pthread_equal(mtx->owner, pthread_self()));
	}
}

static void
mtxexit(struct rumpuser_mtx *mtx)
{

	assert(mtx->owner != NULL);
	if (--mtx->recursion == 0)
		mtx->owner = NULL;
}

void
rumpuser_mutex_enter(struct rumpuser_mtx *mtx)
{

	KLOCK_WRAP(NOFAIL_ERRNO(pthread_mutex_lock(&mtx->pthmtx)));
	mtxenter(mtx);
}

int
rumpuser_mutex_tryenter(struct rumpuser_mtx *mtx)
{
	int rv;

	rv = pthread_mutex_trylock(&mtx->pthmtx);
	if (rv == 0) {
		mtxenter(mtx);
	}

	return rv == 0;
}

void
rumpuser_mutex_exit(struct rumpuser_mtx *mtx)
{

	mtxexit(mtx);
	NOFAIL_ERRNO(pthread_mutex_unlock(&mtx->pthmtx));
}

void
rumpuser_mutex_destroy(struct rumpuser_mtx *mtx)
{

	NOFAIL_ERRNO(pthread_mutex_destroy(&mtx->pthmtx));
	free(mtx);
}

int
rumpuser_mutex_held(struct rumpuser_mtx *mtx)
{

	return mtx->recursion && pthread_equal(mtx->owner, pthread_self());
}

void
rumpuser_rw_init(struct rumpuser_rw **rw)
{

	NOFAIL(*rw = malloc(sizeof(struct rumpuser_rw)));
	NOFAIL_ERRNO(pthread_rwlock_init(&((*rw)->pthrw), NULL));
	NOFAIL_ERRNO(pthread_spin_init(&((*rw)->spin), PTHREAD_PROCESS_SHARED));
	(*rw)->readers = 0;
	(*rw)->writer = NULL;
}

void
rumpuser_rw_enter(struct rumpuser_rw *rw, int iswrite)
{

	if (iswrite) {
		KLOCK_WRAP(NOFAIL_ERRNO(pthread_rwlock_wrlock(&rw->pthrw)));
		RURW_SETWRITE(rw);
	} else {
		KLOCK_WRAP(NOFAIL_ERRNO(pthread_rwlock_rdlock(&rw->pthrw)));
		RURW_INCREAD(rw);
	}
}

int
rumpuser_rw_tryenter(struct rumpuser_rw *rw, int iswrite)
{
	int rv;

	if (iswrite) {
		rv = pthread_rwlock_trywrlock(&rw->pthrw);
		if (rv == 0)
			RURW_SETWRITE(rw);
	} else {
		rv = pthread_rwlock_tryrdlock(&rw->pthrw);
		if (rv == 0)
			RURW_INCREAD(rw);
	}

	return rv == 0;
}

void
rumpuser_rw_exit(struct rumpuser_rw *rw)
{

	if (RURW_HASREAD(rw))
		RURW_DECREAD(rw);
	else
		RURW_CLRWRITE(rw);
	NOFAIL_ERRNO(pthread_rwlock_unlock(&rw->pthrw));
}

void
rumpuser_rw_destroy(struct rumpuser_rw *rw)
{

	NOFAIL_ERRNO(pthread_rwlock_destroy(&rw->pthrw));
	NOFAIL_ERRNO(pthread_spin_destroy(&rw->spin));
	free(rw);
}

int
rumpuser_rw_held(struct rumpuser_rw *rw)
{

	return rw->readers != 0;
}

int
rumpuser_rw_rdheld(struct rumpuser_rw *rw)
{

	return RURW_HASREAD(rw);
}

int
rumpuser_rw_wrheld(struct rumpuser_rw *rw)
{

	return RURW_AMWRITER(rw);
}

void
rumpuser_cv_init(struct rumpuser_cv **cv)
{

	NOFAIL(*cv = malloc(sizeof(struct rumpuser_cv)));
	NOFAIL_ERRNO(pthread_cond_init(&((*cv)->pthcv), NULL));
	(*cv)->nwaiters = 0;
}

void
rumpuser_cv_destroy(struct rumpuser_cv *cv)
{

	NOFAIL_ERRNO(pthread_cond_destroy(&cv->pthcv));
	free(cv);
}

void
rumpuser_cv_wait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx)
{

	cv->nwaiters++;
	assert(mtx->recursion == 1);
	mtxexit(mtx);
	KLOCK_WRAP(NOFAIL_ERRNO(pthread_cond_wait(&cv->pthcv, &mtx->pthmtx)));
	mtxenter(mtx);
	cv->nwaiters--;
}

int
rumpuser_cv_timedwait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx,
	struct timespec *ts)
{
	int rv;

	cv->nwaiters++;
	mtxexit(mtx);
	KLOCK_WRAP(rv = pthread_cond_timedwait(&cv->pthcv, &mtx->pthmtx, ts));
	mtxenter(mtx);
	cv->nwaiters--;
	if (rv != 0 && rv != ETIMEDOUT)
		abort();

	if (rv == ETIMEDOUT)
		rv = EWOULDBLOCK;
	return rv;
}

void
rumpuser_cv_signal(struct rumpuser_cv *cv)
{

	NOFAIL_ERRNO(pthread_cond_signal(&cv->pthcv));
}

void
rumpuser_cv_broadcast(struct rumpuser_cv *cv)
{

	NOFAIL_ERRNO(pthread_cond_broadcast(&cv->pthcv));
}

int
rumpuser_cv_has_waiters(struct rumpuser_cv *cv)
{

	return cv->nwaiters;
}

/*
 * curlwp
 */

void
rumpuser_set_curlwp(struct lwp *l)
{

	assert(pthread_getspecific(curlwpkey) == NULL || l == NULL);
	pthread_setspecific(curlwpkey, l);
}

struct lwp *
rumpuser_get_curlwp(void)
{

	return pthread_getspecific(curlwpkey);
}
