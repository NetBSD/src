/*	$NetBSD: rumpuser_pth.c,v 1.6.6.2 2008/01/19 12:15:39 bouyer Exp $	*/

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

#include <sys/param.h>
#include <sys/lwp.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "rumpuser.h"

static pthread_key_t curlwpkey;
static pthread_key_t isintr;

#define NOFAIL(a) do {if (!(a)) abort();} while (/*CONSTCOND*/0)

struct rumpuser_mtx {
	pthread_mutex_t pthmtx;
};

struct rumpuser_rw {
	pthread_rwlock_t pthrw;
};

struct rumpuser_cv {
	pthread_cond_t pthcv;
};

struct rumpuser_mtx rua_mtx;
struct rumpuser_cv rua_cv;
int rua_head, rua_tail;
struct rumpuser_aio *rua_aios[N_AIOS];

struct rumpuser_rw rumpspl;

static void *
iothread(void *arg)
{
	struct rumpuser_aio *rua;

	NOFAIL(pthread_mutex_lock(&rua_mtx.pthmtx) == 0);
	for (;;) {
		while (rua_head == rua_tail) {
			NOFAIL(pthread_cond_wait(&rua_cv.pthcv,
			    &rua_mtx.pthmtx) == 0);
		}

		rua = rua_aios[rua_tail];
		rua_tail = (rua_tail+1) % (N_AIOS-1);
		pthread_mutex_unlock(&rua_mtx.pthmtx);

		if (rua->rua_op)
			rumpuser_read_bio(rua->rua_fd, rua->rua_data,
			    rua->rua_dlen, rua->rua_off, rua->rua_bp);
		else
			rumpuser_write_bio(rua->rua_fd, rua->rua_data,
			    rua->rua_dlen, rua->rua_off, rua->rua_bp);

		free(rua);
		NOFAIL(pthread_mutex_lock(&rua_mtx.pthmtx) == 0);
	}
}

int
rumpuser_thrinit()
{
	pthread_t iothr;

	pthread_mutex_init(&rua_mtx.pthmtx, NULL);
	pthread_cond_init(&rua_cv.pthcv, NULL);
	pthread_rwlock_init(&rumpspl.pthrw, NULL);

	pthread_key_create(&curlwpkey, NULL);
	pthread_key_create(&isintr, NULL);

	pthread_create(&iothr, NULL, iothread, NULL);

	return 0;
}

void
rumpuser_thrdestroy()
{

	pthread_key_delete(curlwpkey);
}

int
rumpuser_thread_create(void *(*f)(void *), void *arg)
{
	pthread_t ptid;

	return pthread_create(&ptid, NULL, f, arg);
}

void
rumpuser_thread_exit()
{

	pthread_exit(NULL);
}

void
rumpuser_mutex_init(struct rumpuser_mtx **mtx)
{
	NOFAIL(*mtx = malloc(sizeof(struct rumpuser_mtx)));
	NOFAIL(pthread_mutex_init(&((*mtx)->pthmtx), NULL) == 0);
}

void
rumpuser_mutex_recursive_init(struct rumpuser_mtx **mtx)
{
	pthread_mutexattr_t mattr;

	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);

	NOFAIL(*mtx = malloc(sizeof(struct rumpuser_mtx)));
	NOFAIL(pthread_mutex_init(&((*mtx)->pthmtx), &mattr) == 0);

	pthread_mutexattr_destroy(&mattr);
}

void
rumpuser_mutex_enter(struct rumpuser_mtx *mtx)
{

	NOFAIL(pthread_mutex_lock(&mtx->pthmtx) == 0);
}

int
rumpuser_mutex_tryenter(struct rumpuser_mtx *mtx)
{

	return pthread_mutex_trylock(&mtx->pthmtx) == 0;
}

void
rumpuser_mutex_exit(struct rumpuser_mtx *mtx)
{

	NOFAIL(pthread_mutex_unlock(&mtx->pthmtx) == 0);
}

void
rumpuser_mutex_destroy(struct rumpuser_mtx *mtx)
{

	NOFAIL(pthread_mutex_destroy(&mtx->pthmtx) == 0);
	free(mtx);
}

void
rumpuser_rw_init(struct rumpuser_rw **rw)
{

	NOFAIL(*rw = malloc(sizeof(struct rumpuser_rw)));
	NOFAIL(pthread_rwlock_init(&((*rw)->pthrw), NULL) == 0);
}

void
rumpuser_rw_enter(struct rumpuser_rw *rw, int write)
{

	if (write)
		NOFAIL(pthread_rwlock_wrlock(&rw->pthrw) == 0);
	else
		NOFAIL(pthread_rwlock_rdlock(&rw->pthrw) == 0);
}

int
rumpuser_rw_tryenter(struct rumpuser_rw *rw, int write)
{

	if (write)
		return pthread_rwlock_trywrlock(&rw->pthrw) == 0;
	else
		return pthread_rwlock_tryrdlock(&rw->pthrw) == 0;
}

void
rumpuser_rw_exit(struct rumpuser_rw *rw)
{

	NOFAIL(pthread_rwlock_unlock(&rw->pthrw) == 0);
}

void
rumpuser_rw_destroy(struct rumpuser_rw *rw)
{

	NOFAIL(pthread_rwlock_destroy(&rw->pthrw) == 0);
	free(rw);
}

void
rumpuser_cv_init(struct rumpuser_cv **cv)
{

	NOFAIL(*cv = malloc(sizeof(struct rumpuser_cv)));
	NOFAIL(pthread_cond_init(&((*cv)->pthcv), NULL) == 0);
}

void
rumpuser_cv_destroy(struct rumpuser_cv *cv)
{

	NOFAIL(pthread_cond_destroy(&cv->pthcv) == 0);
	free(cv);
}

void
rumpuser_cv_wait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx)
{

	NOFAIL(pthread_cond_wait(&cv->pthcv, &mtx->pthmtx) == 0);
}

int
rumpuser_cv_timedwait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx,
	int stdticks)
{
	struct timespec ts;
	int rv;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec  += stdticks / 100;
	ts.tv_nsec += (stdticks % 100) * 10000000;
	ts.tv_sec  += ts.tv_nsec / 1000000000;
	ts.tv_nsec %= 1000000000;

	rv = pthread_cond_timedwait(&cv->pthcv, &mtx->pthmtx, &ts);
	if (rv != 0 && rv != ETIMEDOUT)
		abort();

	if (rv == ETIMEDOUT)
		rv = EWOULDBLOCK;
	return rv;
}

void
rumpuser_cv_signal(struct rumpuser_cv *cv)
{

	NOFAIL(pthread_cond_signal(&cv->pthcv) == 0);
}

void
rumpuser_cv_broadcast(struct rumpuser_cv *cv)
{

	NOFAIL(pthread_cond_broadcast(&cv->pthcv) == 0);
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
rumpuser_get_curlwp()
{

	return pthread_getspecific(curlwpkey);
}

/*
 * I am the interrupt
 */

void
rumpuser_set_ipl(int what)
{
	int cur;

	if (what == RUMPUSER_IPL_INTR) {
		pthread_setspecific(isintr, (void *)RUMPUSER_IPL_INTR);
	} else  {
		cur = (int)(intptr_t)pthread_getspecific(isintr);
		pthread_setspecific(isintr, (void *)(intptr_t)(cur+1));
	}
}

int
rumpuser_whatis_ipl()
{

	return (int)(intptr_t)pthread_getspecific(isintr);
}

void
rumpuser_clear_ipl(int what)
{
	int cur;

	if (what == RUMPUSER_IPL_INTR)
		cur = 1;
	else
		cur = (int)(intptr_t)pthread_getspecific(isintr);
	cur--;

	pthread_setspecific(isintr, (void *)(intptr_t)cur);
}
