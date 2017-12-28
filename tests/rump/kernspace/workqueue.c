/*	$NetBSD: workqueue.c,v 1.6 2017/12/28 07:46:34 ozaki-r Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: workqueue.c,v 1.6 2017/12/28 07:46:34 ozaki-r Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/workqueue.h>

#include "kernspace.h"

struct test_softc {
	kmutex_t mtx;
	kcondvar_t cv;
	struct workqueue *wq;
	struct work wk;
	int counter;
};	
	
static void
rump_work1(struct work *wk, void *arg)
{
	struct test_softc *sc = arg;

	mutex_enter(&sc->mtx);
	++sc->counter;
	cv_broadcast(&sc->cv);
	mutex_exit(&sc->mtx);
}

static struct test_softc *
create_sc(void)
{
	int rv;
	struct test_softc *sc;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	mutex_init(&sc->mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->cv, "rumpwqcv");
	rv = workqueue_create(&sc->wq, "rumpwq",
	    rump_work1, sc, PRI_SOFTNET, IPL_SOFTNET, 0);
	if (rv)
		panic("workqueue creation failed: %d", rv);

	sc->counter = 0;

	return sc;
}

static void
destroy_sc(struct test_softc *sc)
{

	cv_destroy(&sc->cv);
	mutex_destroy(&sc->mtx);
	workqueue_destroy(sc->wq);
}

void
rumptest_workqueue1()
{
	struct test_softc *sc;

	sc = create_sc();

#define ITERATIONS 12435
	for (int i = 0; i < ITERATIONS; ++i) {
		int e;
		mutex_enter(&sc->mtx);
		workqueue_enqueue(sc->wq, &sc->wk, NULL);
		e = cv_timedwait(&sc->cv, &sc->mtx, hz * 2);
		if (e != 0)
			panic("cv_timedwait timed out (i=%d)", i);
		mutex_exit(&sc->mtx);
	}

	KASSERT(sc->counter == ITERATIONS);

	destroy_sc(sc);
#undef ITERATIONS
}

void
rumptest_workqueue_wait(void)
{
	struct test_softc *sc;
	struct work dummy;

	sc = create_sc();

#define ITERATIONS 12435
	for (size_t i = 0; i < ITERATIONS; ++i) {
		KASSERT(sc->counter == i);
		workqueue_enqueue(sc->wq, &sc->wk, NULL);
		workqueue_wait(sc->wq, &sc->wk);
		KASSERT(sc->counter == (i + 1));
	}

	KASSERT(sc->counter == ITERATIONS);

	/* Wait for a work that is not enqueued. Just return immediately. */
	workqueue_wait(sc->wq, &dummy);

	destroy_sc(sc);
#undef ITERATIONS
}
