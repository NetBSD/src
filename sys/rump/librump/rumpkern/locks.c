/*	$NetBSD: locks.c,v 1.10 2008/01/30 09:50:24 ad Exp $	*/

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

#include "rump_private.h"

#include "rumpuser.h"

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

	mutex_enter(mtx);
}

int
mutex_tryenter(kmutex_t *mtx)
{
	int rv;

	rv = rumpuser_mutex_tryenter(mtx->kmtx_mtx);
	if (rv)
		return 0;
	else
		return 1;
}

void
mutex_exit(kmutex_t *mtx)
{

	rumpuser_mutex_exit(mtx->kmtx_mtx);
}

void
mutex_spin_exit(kmutex_t *mtx)
{

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
	return 1;
}

int
rw_lock_held(krwlock_t *rw)
{

	return rumpuser_rw_held(rw->krw_pthlock);
	return 1;
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
	extern int hz;

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

/* kernel biglock, only for vnode_if */

void
_kernel_lock(int nlocks, struct lwp *l)
{

	KASSERT(nlocks == 1);
	mutex_enter(&rump_giantlock);
}

void
_kernel_unlock(int nlocks, struct lwp *l, int *countp)
{

	KASSERT(nlocks == 1);
	mutex_exit(&rump_giantlock);
	if (countp)
		*countp = 1;
}
