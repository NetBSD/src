/*	$NetBSD: npf_worker.c,v 1.4 2017/12/10 01:18:21 rmind Exp $	*/

/*-
 * Copyright (c) 2010-2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_worker.c,v 1.4 2017/12/10 01:18:21 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/cprng.h>
#endif

#include "npf_impl.h"

typedef struct npf_worker {
	kmutex_t		worker_lock;
	kcondvar_t		worker_cv;
	npf_workfunc_t		work_funcs[NPF_MAX_WORKS];
	bool			worker_exit;
	lwp_t *			worker_lwp;
	npf_t *			instances;
} npf_worker_t;

#define	W_INTERVAL		mstohz(1 * 1000)

static void			npf_worker(void *) __dead;

static npf_worker_t *		npf_workers		__read_mostly;
static unsigned			npf_worker_count	__read_mostly;

int
npf_worker_sysinit(unsigned nworkers)
{
	npf_workers = kmem_zalloc(sizeof(npf_worker_t) * nworkers, KM_SLEEP);
	npf_worker_count = nworkers;

	for (unsigned i = 0; i < nworkers; i++) {
		npf_worker_t *wrk = &npf_workers[i];

		mutex_init(&wrk->worker_lock, MUTEX_DEFAULT, IPL_SOFTNET);
		cv_init(&wrk->worker_cv, "npfgccv");

		if (kthread_create(PRI_NONE, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN,
		    NULL, npf_worker, wrk, &wrk->worker_lwp, "npfgc-%u", i)) {
			npf_worker_sysfini();
			return ENOMEM;
		}
	}
	return 0;
}

void
npf_worker_sysfini(void)
{
	for (unsigned i = 0; i < npf_worker_count; i++) {
		npf_worker_t *wrk = &npf_workers[i];

		/* Notify the worker and wait for the exit. */
		mutex_enter(&wrk->worker_lock);
		wrk->worker_exit = true;
		cv_broadcast(&wrk->worker_cv);
		mutex_exit(&wrk->worker_lock);

		if (wrk->worker_lwp) {
			kthread_join(wrk->worker_lwp);
		}

		/* LWP has exited, destroy the structures. */
		cv_destroy(&wrk->worker_cv);
		mutex_destroy(&wrk->worker_lock);
	}
	kmem_free(npf_workers, sizeof(npf_worker_t) * npf_worker_count);
}

void
npf_worker_signal(npf_t *npf)
{
	const unsigned idx = npf->worker_id;
	npf_worker_t *wrk = &npf_workers[idx];

	mutex_enter(&wrk->worker_lock);
	cv_signal(&wrk->worker_cv);
	mutex_exit(&wrk->worker_lock);
}

static bool
npf_worker_testset(npf_worker_t *wrk, npf_workfunc_t find, npf_workfunc_t set)
{
	for (u_int i = 0; i < NPF_MAX_WORKS; i++) {
		if (wrk->work_funcs[i] == find) {
			wrk->work_funcs[i] = set;
			return true;
		}
	}
	return false;
}

void
npf_worker_register(npf_t *npf, npf_workfunc_t func)
{
	const unsigned idx = cprng_fast32() % npf_worker_count;
	npf_worker_t *wrk = &npf_workers[idx];

	mutex_enter(&wrk->worker_lock);

	npf->worker_id = idx;
	npf->worker_entry = wrk->instances;
	wrk->instances = npf;

	npf_worker_testset(wrk, NULL, func);
	mutex_exit(&wrk->worker_lock);
}

void
npf_worker_unregister(npf_t *npf, npf_workfunc_t func)
{
	const unsigned idx = npf->worker_id;
	npf_worker_t *wrk;
	npf_t *instance;

	if (!npf_worker_count)
		return;
	wrk = &npf_workers[idx];
	mutex_enter(&wrk->worker_lock);
	npf_worker_testset(wrk, func, NULL);
	if ((instance = wrk->instances) == npf) {
		wrk->instances = instance->worker_entry;
	} else while (instance) {
		if (instance->worker_entry == npf) {
			instance->worker_entry = npf->worker_entry;
			break;
		}
		instance = instance->worker_entry;
	}
	mutex_exit(&wrk->worker_lock);
}

static void
npf_worker(void *arg)
{
	npf_worker_t *wrk = arg;

	KASSERT(wrk != NULL);
	KASSERT(!wrk->worker_exit);

	while (!wrk->worker_exit) {
		npf_t *npf;

		npf = wrk->instances;
		while (npf) {
			u_int i = NPF_MAX_WORKS;
			npf_workfunc_t work;

			/* Run the jobs. */
			while (i--) {
				if ((work = wrk->work_funcs[i]) != NULL) {
					work(npf);
				}
			}
			/* Next .. */
			npf = npf->worker_entry;
		}
		if (wrk->worker_exit)
			break;

		/* Sleep and periodically wake up, unless we get notified. */
		mutex_enter(&wrk->worker_lock);
		cv_timedwait(&wrk->worker_cv, &wrk->worker_lock, W_INTERVAL);
		mutex_exit(&wrk->worker_lock);
	}
	kthread_exit(0);
}
