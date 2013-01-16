/*	$NetBSD: rumpuser_pth_dummy.c,v 1.2.4.1 2013/01/16 05:32:28 yamt Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: rumpuser_pth_dummy.c,v 1.2.4.1 2013/01/16 05:32:28 yamt Exp $");
#endif /* !lint */

#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <rump/rumpuser.h>

#include "rumpuser_int.h"

static struct lwp *curlwp;

struct rumpuser_cv {};

struct rumpuser_mtx {
	int v;
	struct lwp *o;
};

struct rumpuser_rw {
	int v;
};

struct rumpuser_mtx rumpuser_aio_mtx;
struct rumpuser_cv rumpuser_aio_cv;
int rumpuser_aio_head, rumpuser_aio_tail;
struct rumpuser_aio rumpuser_aios[N_AIOS];

kernel_lockfn	rumpuser__klock;
kernel_unlockfn	rumpuser__kunlock;

/*ARGSUSED*/
void
rumpuser_thrinit(kernel_lockfn lockfn, kernel_unlockfn unlockfn, int threads)
{

	rumpuser__klock = lockfn;
	rumpuser__kunlock = unlockfn;
}

/*ARGSUSED*/
void
rumpuser_biothread(void *arg)
{

	fprintf(stderr, "rumpuser: threads not available\n");
	abort();
}

/*ARGSUSED*/
int
rumpuser_thread_create(void *(*f)(void *), void *arg, const char *thrname,
	int joinable, void **tptr)
{

	fprintf(stderr, "rumpuser: threads not available\n");
	abort();
	return 0;
}

void
rumpuser_thread_exit(void)
{

	fprintf(stderr, "rumpuser: threads not available\n");
	abort();
}

void
rumpuser_mutex_init(struct rumpuser_mtx **mtx)
{

	*mtx = calloc(1, sizeof(struct rumpuser_mtx));
}

void
rumpuser_mutex_init_kmutex(struct rumpuser_mtx **mtx)
{

	*mtx = calloc(1, sizeof(struct rumpuser_mtx));
}

void
rumpuser_mutex_enter(struct rumpuser_mtx *mtx)
{

	mtx->v++;
	mtx->o = curlwp;
}

void
rumpuser_mutex_enter_nowrap(struct rumpuser_mtx *mtx)
{

	rumpuser_mutex_enter(mtx);
}

int
rumpuser_mutex_tryenter(struct rumpuser_mtx *mtx)
{

	mtx->v++;
	return 1;
}

void
rumpuser_mutex_exit(struct rumpuser_mtx *mtx)
{

	assert(mtx->v > 0);
	if (--mtx->v == 0)
		mtx->o = NULL;
}

void
rumpuser_mutex_destroy(struct rumpuser_mtx *mtx)
{

	free(mtx);
}

struct lwp *
rumpuser_mutex_owner(struct rumpuser_mtx *mtx)
{

	return mtx->o;
}

void
rumpuser_rw_init(struct rumpuser_rw **rw)
{

	*rw = calloc(1, sizeof(struct rumpuser_rw));
}

void
rumpuser_rw_enter(struct rumpuser_rw *rw, int write)
{

	if (write) {
		rw->v++;
		assert(rw->v == 1);
	} else {
		assert(rw->v <= 0);
		rw->v--;
	}
}

int
rumpuser_rw_tryenter(struct rumpuser_rw *rw, int write)
{

	rumpuser_rw_enter(rw, write);
	return 1;
}

void
rumpuser_rw_exit(struct rumpuser_rw *rw)
{

	if (rw->v > 0) {
		assert(rw->v == 1);
		rw->v--;
	} else {
		rw->v++;
	}
}

void
rumpuser_rw_destroy(struct rumpuser_rw *rw)
{

	free(rw);
}

int
rumpuser_rw_held(struct rumpuser_rw *rw)
{

	return rw->v != 0;
}

int
rumpuser_rw_rdheld(struct rumpuser_rw *rw)
{

	return rw->v < 0;
}

int
rumpuser_rw_wrheld(struct rumpuser_rw *rw)
{

	return rw->v > 0;
}

/*ARGSUSED*/
void
rumpuser_cv_init(struct rumpuser_cv **cv)
{

}

/*ARGSUSED*/
void
rumpuser_cv_destroy(struct rumpuser_cv *cv)
{

}

/*ARGSUSED*/
void
rumpuser_cv_wait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx)
{

}

/*ARGSUSED*/
int
rumpuser_cv_timedwait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx,
	int64_t sec, int64_t nsec)
{
	struct timespec ts;

	/*LINTED*/
	ts.tv_sec = sec;
	/*LINTED*/
	ts.tv_nsec = nsec;

	nanosleep(&ts, NULL);
	return 0;
}

/*ARGSUSED*/
void
rumpuser_cv_signal(struct rumpuser_cv *cv)
{

}

/*ARGSUSED*/
void
rumpuser_cv_broadcast(struct rumpuser_cv *cv)
{

}

/*ARGSUSED*/
int
rumpuser_cv_has_waiters(struct rumpuser_cv *cv)
{

	return 0;
}

/*
 * curlwp
 */

void
rumpuser_set_curlwp(struct lwp *l)
{

	curlwp = l;
}

struct lwp *
rumpuser_get_curlwp(void)
{

	return curlwp;
}
