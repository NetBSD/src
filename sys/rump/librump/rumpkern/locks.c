/*	$NetBSD: locks.c,v 1.35 2009/11/26 17:29:34 pooka Exp $	*/

/*
 * Copyright (c) 2007, 2008 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: locks.c,v 1.35 2009/11/26 17:29:34 pooka Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * We map locks to pthread routines.  The difference between kernel
 * and rumpuser routines is that while the kernel uses static
 * storage, rumpuser allocates the object from the heap.  This
 * indirection is necessary because we don't know the size of
 * pthread objects here.  It is also benefitial, since we can
 * be easily compatible with the kernel ABI because all kernel
 * objects regardless of machine architecture are always at least
 * the size of a pointer.  The downside, of course, is a performance
 * penalty.
 */

#define RUMPMTX(mtx) (*(struct rumpuser_mtx **)(mtx))

void
mutex_init(kmutex_t *mtx, kmutex_type_t type, int ipl)
{

	CTASSERT(sizeof(kmutex_t) >= sizeof(void *));

	rumpuser_mutex_init((struct rumpuser_mtx **)mtx);
}

void
mutex_destroy(kmutex_t *mtx)
{

	rumpuser_mutex_destroy(RUMPMTX(mtx));
}

void
mutex_enter(kmutex_t *mtx)
{

	rumpuser_mutex_enter(RUMPMTX(mtx));
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

	return rumpuser_mutex_tryenter(RUMPMTX(mtx));
}

void
mutex_exit(kmutex_t *mtx)
{

	rumpuser_mutex_exit(RUMPMTX(mtx));
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

	return rumpuser_mutex_held(RUMPMTX(mtx));
}

#define RUMPRW(rw) (*(struct rumpuser_rw **)(rw))

/* reader/writer locks */

void
rw_init(krwlock_t *rw)
{

	CTASSERT(sizeof(krwlock_t) >= sizeof(void *));

	rumpuser_rw_init((struct rumpuser_rw **)rw);
}

void
rw_destroy(krwlock_t *rw)
{

	rumpuser_rw_destroy(RUMPRW(rw));
}

void
rw_enter(krwlock_t *rw, const krw_t op)
{

	rumpuser_rw_enter(RUMPRW(rw), op == RW_WRITER);
}

int
rw_tryenter(krwlock_t *rw, const krw_t op)
{

	return rumpuser_rw_tryenter(RUMPRW(rw), op == RW_WRITER);
}

void
rw_exit(krwlock_t *rw)
{

	rumpuser_rw_exit(RUMPRW(rw));
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

	return rumpuser_rw_wrheld(RUMPRW(rw));
}

int
rw_read_held(krwlock_t *rw)
{

	return rumpuser_rw_rdheld(RUMPRW(rw));
}

int
rw_lock_held(krwlock_t *rw)
{

	return rumpuser_rw_held(RUMPRW(rw));
}

/* curriculum vitaes */

#define RUMPCV(cv) (*(struct rumpuser_cv **)(cv))

void
cv_init(kcondvar_t *cv, const char *msg)
{

	CTASSERT(sizeof(kcondvar_t) >= sizeof(void *));

	rumpuser_cv_init((struct rumpuser_cv **)cv);
}

void
cv_destroy(kcondvar_t *cv)
{

	rumpuser_cv_destroy(RUMPCV(cv));
}

void
cv_wait(kcondvar_t *cv, kmutex_t *mtx)
{

	if (rump_threads == 0)
		panic("cv_wait without threads");
	rumpuser_cv_wait(RUMPCV(cv), RUMPMTX(mtx));
}

int
cv_wait_sig(kcondvar_t *cv, kmutex_t *mtx)
{

	rumpuser_cv_wait(RUMPCV(cv), RUMPMTX(mtx));
	return 0;
}

int
cv_timedwait(kcondvar_t *cv, kmutex_t *mtx, int ticks)
{
	struct timespec ts, tick;
	extern int hz;

	nanotime(&ts);
	tick.tv_sec = ticks / hz;
	tick.tv_nsec = (ticks % hz) * (1000000000/hz);
	timespecadd(&ts, &tick, &ts);

	if (ticks == 0) {
		cv_wait(cv, mtx);
		return 0;
	} else {
		if (rumpuser_cv_timedwait(RUMPCV(cv), RUMPMTX(mtx),
		    ts.tv_sec, ts.tv_nsec))
			return EWOULDBLOCK;
		else
			return 0;
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

/* this is not much of an attempt, but ... */
bool
cv_is_valid(kcondvar_t *cv)
{

	return RUMPCV(cv) != NULL;
}

/*
 * giant lock
 */

static volatile int lockcnt;

bool
kernel_biglocked()
{

	return rumpuser_mutex_held(rump_giantlock) && lockcnt > 0;
}

void
kernel_unlock_allbutone(int *countp)
{
	int minusone = lockcnt-1;

	KASSERT(kernel_biglocked());
	if (minusone) {
		_kernel_unlock(minusone, countp);
	}
	KASSERT(lockcnt == 1);
	*countp = minusone;

	/*
	 * We drop lockcnt to 0 since rumpuser doesn't know that the
	 * kernel biglock is being used as the interlock for cv in
	 * tsleep.
	 */
	lockcnt = 0;
}

void
kernel_ununlock_allbutone(int nlocks)
{

	KASSERT(rumpuser_mutex_held(rump_giantlock) && lockcnt == 0);
	lockcnt = 1;
	_kernel_lock(nlocks);
}

void
_kernel_lock(int nlocks)
{

	while (nlocks--) {
		if (!rumpuser_mutex_tryenter(rump_giantlock)) {
			struct lwp *l = curlwp;

			rump_unschedule_cpu(l);
			rumpuser_mutex_enter_nowrap(rump_giantlock);
			rump_schedule_cpu(l);
		}
		lockcnt++;
	}
}

void
_kernel_unlock(int nlocks, int *countp)
{

	if (!rumpuser_mutex_held(rump_giantlock)) {
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
		rumpuser_mutex_exit(rump_giantlock);
	}
}

void
rump_user_unschedule(int nlocks, int *countp)
{

	_kernel_unlock(nlocks, countp);
	rump_unschedule_cpu(curlwp);
}

void
rump_user_schedule(int nlocks)
{

	rump_schedule_cpu(curlwp);

	if (nlocks)
		_kernel_lock(nlocks);
}
