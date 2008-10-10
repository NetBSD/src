/*	$NetBSD: locks.c,v 1.20 2008/10/10 13:14:41 pooka Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/atomic.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

void
mutex_init(kmutex_t *mtx, kmutex_type_t type, int ipl)
{

	rumpuser_mutex_init(&mtx->kmtx_mtx);
}

void
mutex_destroy(kmutex_t *mtx)
{

	rumpuser_mutex_destroy(mtx->kmtx_mtx);
}

void
mutex_enter(kmutex_t *mtx)
{

	rumpuser_mutex_enter(mtx->kmtx_mtx);
}

void
mutex_spin_enter(kmutex_t *mtx)
{

	if (__predict_true(mtx != RUMP_LMUTEX_MAGIC))
		mutex_enter(mtx);
}

int
mutex_tryenter(kmutex_t *mtx)
{

	return rumpuser_mutex_tryenter(mtx->kmtx_mtx);
}

void
mutex_exit(kmutex_t *mtx)
{

	rumpuser_mutex_exit(mtx->kmtx_mtx);
}

void
mutex_spin_exit(kmutex_t *mtx)
{

	if (__predict_true(mtx != RUMP_LMUTEX_MAGIC))
		mutex_exit(mtx);
}

int
mutex_owned(kmutex_t *mtx)
{

	return rumpuser_mutex_held(mtx->kmtx_mtx);
}

/* reader/writer locks */

void
rw_init(krwlock_t *rw)
{

	rumpuser_rw_init(&rw->krw_pthlock);
}

void
rw_destroy(krwlock_t *rw)
{

	rumpuser_rw_destroy(rw->krw_pthlock);
}

void
rw_enter(krwlock_t *rw, const krw_t op)
{

	rumpuser_rw_enter(rw->krw_pthlock, op == RW_WRITER);
}

int
rw_tryenter(krwlock_t *rw, const krw_t op)
{

	return rumpuser_rw_tryenter(rw->krw_pthlock, op == RW_WRITER);
}

void
rw_exit(krwlock_t *rw)
{

	rumpuser_rw_exit(rw->krw_pthlock);
}

/* always fails */
int
rw_tryupgrade(krwlock_t *rw)
{

	return 0;
}

int
rw_write_held(krwlock_t *rw)
{

	return rumpuser_rw_wrheld(rw->krw_pthlock);
}

int
rw_read_held(krwlock_t *rw)
{

	return rumpuser_rw_rdheld(rw->krw_pthlock);
}

int
rw_lock_held(krwlock_t *rw)
{

	return rumpuser_rw_held(rw->krw_pthlock);
}

/* curriculum vitaes */

/* forgive me for I have sinned */
#define RUMPCV(a) ((struct rumpuser_cv *)(__UNCONST((a)->cv_wmesg)))

void
cv_init(kcondvar_t *cv, const char *msg)
{

	rumpuser_cv_init((struct rumpuser_cv **)__UNCONST(&cv->cv_wmesg));
}

void
cv_destroy(kcondvar_t *cv)
{

	rumpuser_cv_destroy(RUMPCV(cv));
}

void
cv_wait(kcondvar_t *cv, kmutex_t *mtx)
{

	rumpuser_cv_wait(RUMPCV(cv), mtx->kmtx_mtx);
}

int
cv_wait_sig(kcondvar_t *cv, kmutex_t *mtx)
{

	rumpuser_cv_wait(RUMPCV(cv), mtx->kmtx_mtx);
	return 0;
}

int
cv_timedwait(kcondvar_t *cv, kmutex_t *mtx, int ticks)
{
#ifdef DIAGNOSTIC
	extern int hz;
#endif

	if (ticks == 0) {
		cv_wait(cv, mtx);
		return 0;
	} else {
		KASSERT(hz == 100);
		return rumpuser_cv_timedwait(RUMPCV(cv), mtx->kmtx_mtx, ticks);
	}
}

int
cv_timedwait_sig(kcondvar_t *cv, kmutex_t *mtx, int ticks)
{

	return cv_timedwait(cv, mtx, ticks);
}

void
cv_signal(kcondvar_t *cv)
{

	rumpuser_cv_signal(RUMPCV(cv));
}

void
cv_broadcast(kcondvar_t *cv)
{

	rumpuser_cv_broadcast(RUMPCV(cv));
}

bool
cv_has_waiters(kcondvar_t *cv)
{

	return rumpuser_cv_has_waiters(RUMPCV(cv));
}

/*
 * giant lock
 */

static int lockcnt;
void
_kernel_lock(int nlocks)
{

	while (nlocks--) {
		mutex_enter(&rump_giantlock);
		lockcnt++;
	}
}

void
_kernel_unlock(int nlocks, int *countp)
{

	if (!mutex_owned(&rump_giantlock)) {
		KASSERT(nlocks == 0);
		if (countp)
			*countp = 0;
		return;
	}

	if (countp)
		*countp = lockcnt;
	if (nlocks == 0)
		nlocks = lockcnt;
	if (nlocks == -1) {
		KASSERT(lockcnt == 1);
		nlocks = 1;
	}
	KASSERT(nlocks <= lockcnt);
	while (nlocks--) {
		lockcnt--;
		mutex_exit(&rump_giantlock);
	}
}

struct kmutexobj {
	kmutex_t	mo_lock;
	u_int		mo_refcnt;
};

kmutex_t *
mutex_obj_alloc(kmutex_type_t type, int ipl)
{
	struct kmutexobj *mo;

	mo = kmem_alloc(sizeof(*mo), KM_SLEEP);
	mutex_init(&mo->mo_lock, type, ipl);
	mo->mo_refcnt = 1;

	return (kmutex_t *)mo;
}

void
mutex_obj_hold(kmutex_t *lock)
{
	struct kmutexobj *mo = (struct kmutexobj *)lock;

	atomic_inc_uint(&mo->mo_refcnt);
}

bool
mutex_obj_free(kmutex_t *lock)
{
	struct kmutexobj *mo = (struct kmutexobj *)lock;

	if (atomic_dec_uint_nv(&mo->mo_refcnt) > 0) {
		return false;
	}
	mutex_destroy(&mo->mo_lock);
	kmem_free(mo, sizeof(*mo));
	return true;
}
