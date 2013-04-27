/*	$NetBSD: rumpuser_pth.c,v 1.14 2013/04/27 14:59:08 pooka Exp $	*/

/*
 * Copyright (c) 2007-2010 Antti Kantee.  All Rights Reserved.
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

#include "rumpuser_port.h"

#if !defined(lint)
__RCSID("$NetBSD: rumpuser_pth.c,v 1.14 2013/04/27 14:59:08 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
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

#define MTX_KMUTEX 0x1
#define MTX_ISSPIN 0x2
struct rumpuser_mtx {
	pthread_mutex_t pthmtx;
	struct lwp *owner;
	int flags;
};

#define RURW_AMWRITER(rw) (rw->writer == rumpuser_get_curlwp()		\
				&& rw->readers == -1)
#define RURW_HASREAD(rw)  (rw->readers > 0)

#define RURW_SETWRITE(rw)						\
do {									\
	assert(rw->readers == 0);					\
	rw->writer = rumpuser_get_curlwp();				\
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
	struct lwp *writer;
};

struct rumpuser_cv {
	pthread_cond_t pthcv;
	int nwaiters;
};

struct rumpuser_mtx rumpuser_aio_mtx;
struct rumpuser_cv rumpuser_aio_cv;
int rumpuser_aio_head, rumpuser_aio_tail;
struct rumpuser_aio rumpuser_aios[N_AIOS];

void
rumpuser__thrinit(void)
{

	pthread_mutex_init(&rumpuser_aio_mtx.pthmtx, NULL);
	pthread_cond_init(&rumpuser_aio_cv.pthcv, NULL);
	pthread_key_create(&curlwpkey, NULL);
}

void
/*ARGSUSED*/
rumpuser_biothread(void *arg)
{
	struct rumpuser_aio *rua;
	rump_biodone_fn biodone = arg;
	ssize_t rv;
	int error, dummy;

	/* unschedule from CPU.  we reschedule before running the interrupt */
	rumpuser__unschedule(0, &dummy, NULL);
	assert(dummy == 0);

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
		rumpuser__reschedule(0, NULL);
		biodone(rua->rua_bp, (size_t)rv, error);
		rumpuser__unschedule(0, &dummy, NULL);

		rua->rua_bp = NULL;

		NOFAIL_ERRNO(pthread_mutex_lock(&rumpuser_aio_mtx.pthmtx));
		rumpuser_aio_tail = (rumpuser_aio_tail+1) % N_AIOS;
		pthread_cond_signal(&rumpuser_aio_cv.pthcv);
	}

	/*NOTREACHED*/
	fprintf(stderr, "error: rumpuser_biothread reached unreachable\n");
	abort();
}

int
rumpuser_thread_create(void *(*f)(void *), void *arg, const char *thrname,
	int joinable, void **ptcookie)
{
	pthread_t ptid;
	pthread_t *ptidp;
	pthread_attr_t pattr;
	int rv;

	if ((rv = pthread_attr_init(&pattr)) != 0)
		return rv;

	if (joinable) {
		NOFAIL(ptidp = malloc(sizeof(*ptidp)));
		pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);
	} else {
		ptidp = &ptid;
		pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
	}

	rv = pthread_create(ptidp, &pattr, f, arg);
#if defined(__NetBSD__)
	if (rv == 0 && thrname)
		pthread_setname_np(ptid, thrname, NULL);
#elif defined(__linux__)
	/*
	 * The pthread_setname_np() call varies from one Linux distro to
	 * another.  Comment out the call pending autoconf support.
	 */
#if 0
	if (rv == 0 && thrname)
		pthread_setname_np(ptid, thrname);
#endif
#endif

	if (joinable) {
		assert(ptcookie);
		*ptcookie = ptidp;
	}

	pthread_attr_destroy(&pattr);

	return rv;
}

__dead void
rumpuser_thread_exit(void)
{

	pthread_exit(NULL);
}

int
rumpuser_thread_join(void *ptcookie)
{
	pthread_t *pt = ptcookie;
	int rv;

	KLOCK_WRAP((rv = pthread_join(*pt, NULL)));
	if (rv == 0)
		free(pt);

	return rv;
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
	(*mtx)->flags = MTX_ISSPIN;
}

void
rumpuser_mutex_init_kmutex(struct rumpuser_mtx **mtx, int isspin)
{

	rumpuser_mutex_init(mtx);
	(*mtx)->flags = MTX_KMUTEX | (isspin ? MTX_ISSPIN : 0);
}

static void
mtxenter(struct rumpuser_mtx *mtx)
{

	if (!(mtx->flags & MTX_KMUTEX))
		return;

	assert(mtx->owner == NULL);
	mtx->owner = rumpuser_get_curlwp();
}

static void
mtxexit(struct rumpuser_mtx *mtx)
{

	if (!(mtx->flags & MTX_KMUTEX))
		return;

	assert(mtx->owner != NULL);
	mtx->owner = NULL;
}

void
rumpuser_mutex_enter(struct rumpuser_mtx *mtx)
{

	if (mtx->flags & MTX_ISSPIN) {
		rumpuser_mutex_enter_nowrap(mtx);
		return;
	}

	if (pthread_mutex_trylock(&mtx->pthmtx) != 0)
		KLOCK_WRAP(NOFAIL_ERRNO(pthread_mutex_lock(&mtx->pthmtx)));
	mtxenter(mtx);
}

void
rumpuser_mutex_enter_nowrap(struct rumpuser_mtx *mtx)
{

	assert(mtx->flags & MTX_ISSPIN);
	NOFAIL_ERRNO(pthread_mutex_lock(&mtx->pthmtx));
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

struct lwp *
rumpuser_mutex_owner(struct rumpuser_mtx *mtx)
{

	if (__predict_false(!(mtx->flags & MTX_KMUTEX))) {
		printf("panic: rumpuser_mutex_held unsupported on non-kmtx\n");
		abort();
	}

	return mtx->owner;
}

void
rumpuser_rw_init(struct rumpuser_rw **rw)
{

	NOFAIL(*rw = malloc(sizeof(struct rumpuser_rw)));
	NOFAIL_ERRNO(pthread_rwlock_init(&((*rw)->pthrw), NULL));
	NOFAIL_ERRNO(pthread_spin_init(&((*rw)->spin),PTHREAD_PROCESS_PRIVATE));
	(*rw)->readers = 0;
	(*rw)->writer = NULL;
}

void
rumpuser_rw_enter(struct rumpuser_rw *rw, int iswrite)
{

	if (iswrite) {
		if (pthread_rwlock_trywrlock(&rw->pthrw) != 0)
			KLOCK_WRAP(NOFAIL_ERRNO(
			    pthread_rwlock_wrlock(&rw->pthrw)));
		RURW_SETWRITE(rw);
	} else {
		if (pthread_rwlock_tryrdlock(&rw->pthrw) != 0)
			KLOCK_WRAP(NOFAIL_ERRNO(
			    pthread_rwlock_rdlock(&rw->pthrw)));
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
	int nlocks;

	cv->nwaiters++;
	rumpuser__unschedule(0, &nlocks, mtx);
	mtxexit(mtx);
	NOFAIL_ERRNO(pthread_cond_wait(&cv->pthcv, &mtx->pthmtx));
	mtxenter(mtx);
	rumpuser__reschedule(nlocks, mtx);
	cv->nwaiters--;
}

void
rumpuser_cv_wait_nowrap(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx)
{

	cv->nwaiters++;
	mtxexit(mtx);
	NOFAIL_ERRNO(pthread_cond_wait(&cv->pthcv, &mtx->pthmtx));
	mtxenter(mtx);
	cv->nwaiters--;
}

int
rumpuser_cv_timedwait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx,
	int64_t sec, int64_t nsec)
{
	struct timespec ts;
	int rv, nlocks;

	/* LINTED */
	ts.tv_sec = sec; ts.tv_nsec = nsec;

	cv->nwaiters++;
	rumpuser__unschedule(0, &nlocks, mtx);
	mtxexit(mtx);
	rv = pthread_cond_timedwait(&cv->pthcv, &mtx->pthmtx, &ts);
	mtxenter(mtx);
	rumpuser__reschedule(nlocks, mtx);
	cv->nwaiters--;
	if (rv != 0 && rv != ETIMEDOUT)
		abort();

	return rv == ETIMEDOUT;
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
