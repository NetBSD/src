/*	$NetBSD: rumpuser_pth.c,v 1.20 2013/04/30 12:39:20 pooka Exp $	*/

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
__RCSID("$NetBSD: rumpuser_pth.c,v 1.20 2013/04/30 12:39:20 pooka Exp $");
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

void
rumpuser__thrinit(void)
{

	pthread_key_create(&curlwpkey, NULL);
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

	ET(rv);
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

	ET(rv);
}

void
rumpuser_mutex_init(struct rumpuser_mtx **mtx, int flags)
{
	pthread_mutexattr_t att;

	NOFAIL(*mtx = malloc(sizeof(struct rumpuser_mtx)));

	pthread_mutexattr_init(&att);
	pthread_mutexattr_settype(&att, PTHREAD_MUTEX_ERRORCHECK);
	NOFAIL_ERRNO(pthread_mutex_init(&((*mtx)->pthmtx), &att));
	pthread_mutexattr_destroy(&att);

	(*mtx)->owner = NULL;
	assert(flags != 0);
	(*mtx)->flags = flags;
}

static void
mtxenter(struct rumpuser_mtx *mtx)
{

	if (!(mtx->flags & RUMPUSER_MTX_KMUTEX))
		return;

	assert(mtx->owner == NULL);
	mtx->owner = rumpuser_get_curlwp();
}

static void
mtxexit(struct rumpuser_mtx *mtx)
{

	if (!(mtx->flags & RUMPUSER_MTX_KMUTEX))
		return;

	assert(mtx->owner != NULL);
	mtx->owner = NULL;
}

void
rumpuser_mutex_enter(struct rumpuser_mtx *mtx)
{

	if (mtx->flags & RUMPUSER_MTX_SPIN) {
		rumpuser_mutex_enter_nowrap(mtx);
		return;
	}

	assert(mtx->flags & RUMPUSER_MTX_KMUTEX);
	if (pthread_mutex_trylock(&mtx->pthmtx) != 0)
		KLOCK_WRAP(NOFAIL_ERRNO(pthread_mutex_lock(&mtx->pthmtx)));
	mtxenter(mtx);
}

void
rumpuser_mutex_enter_nowrap(struct rumpuser_mtx *mtx)
{

	assert(mtx->flags & RUMPUSER_MTX_SPIN);
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

	ET(rv);
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

void
rumpuser_mutex_owner(struct rumpuser_mtx *mtx, struct lwp **lp)
{

	if (__predict_false(!(mtx->flags & RUMPUSER_MTX_KMUTEX))) {
		printf("panic: rumpuser_mutex_held unsupported on non-kmtx\n");
		abort();
	}

	*lp = mtx->owner;
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

	ET(rv);
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

void
rumpuser_rw_held(struct rumpuser_rw *rw, int *rv)
{

	*rv = rw->readers != 0;
}

void
rumpuser_rw_rdheld(struct rumpuser_rw *rw, int *rv)
{

	*rv = RURW_HASREAD(rw);
}

void
rumpuser_rw_wrheld(struct rumpuser_rw *rw, int *rv)
{

	*rv = RURW_AMWRITER(rw);
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
	rumpkern_unsched(&nlocks, mtx);
	mtxexit(mtx);
	NOFAIL_ERRNO(pthread_cond_wait(&cv->pthcv, &mtx->pthmtx));
	mtxenter(mtx);
	rumpkern_sched(nlocks, mtx);
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

	/*
	 * Get clock already here, just in case we will be put to sleep
	 * after releasing the kernel context.
	 *
	 * The condition variables should use CLOCK_MONOTONIC, but since
	 * that's not available everywhere, leave it for another day.
	 */
	clock_gettime(CLOCK_REALTIME, &ts);

	cv->nwaiters++;
	rumpkern_unsched(&nlocks, mtx);
	mtxexit(mtx);

	ts.tv_sec += sec;
	ts.tv_nsec += nsec;
	if (ts.tv_nsec >= 1000*1000*1000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000*1000*1000;
	}
	rv = pthread_cond_timedwait(&cv->pthcv, &mtx->pthmtx, &ts);
	mtxenter(mtx);
	rumpkern_sched(nlocks, mtx);
	cv->nwaiters--;

	ET(rv);
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

void
rumpuser_cv_has_waiters(struct rumpuser_cv *cv, int *nwaiters)
{

	*nwaiters = cv->nwaiters;
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
