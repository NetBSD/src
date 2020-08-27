/*-
 * Copyright (c) 2010-2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf_worker.c,v 1.10 2020/08/27 18:49:36 riastradh Exp $");

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
	kmutex_t		lock;
	kcondvar_t		cv;
	kcondvar_t		exit_cv;
	bool			exit;
	LIST_HEAD(, npf)	instances;
	unsigned		worker_count;
	lwp_t *			worker[];
} npf_workerinfo_t;

#define	NPF_GC_MINWAIT		(10)		// 10 ms
#define	NPF_GC_MAXWAIT		(10 * 1000)	// 10 sec

/*
 * Flags for the npf_t::worker_flags field.
 */
#define	WFLAG_ACTIVE		0x01	// instance enqueued for workers
#define	WFLAG_INITED		0x02	// worker setup the instance
#define	WFLAG_REMOVE		0x04	// remove the instance

static void			npf_worker(void *)	__dead;
static npf_workerinfo_t *	worker_info		__read_mostly;

int
npf_worker_sysinit(unsigned nworkers)
{
	const size_t len = offsetof(npf_workerinfo_t, worker[nworkers]);
	npf_workerinfo_t *winfo;

	KASSERT(worker_info == NULL);

	if (!nworkers) {
		return 0;
	}

	winfo = kmem_zalloc(len, KM_SLEEP);
	winfo->worker_count = nworkers;
	mutex_init(&winfo->lock, MUTEX_DEFAULT, IPL_SOFTNET);
	cv_init(&winfo->exit_cv, "npfgcx");
	cv_init(&winfo->cv, "npfgcw");
	LIST_INIT(&winfo->instances);
	worker_info = winfo;

	for (unsigned i = 0; i < nworkers; i++) {
		if (kthread_create(PRI_NONE, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN,
		    NULL, npf_worker, winfo, &winfo->worker[i], "npfgc%u", i)) {
			npf_worker_sysfini();
			return ENOMEM;
		}
	}
	return 0;
}

void
npf_worker_sysfini(void)
{
	npf_workerinfo_t *winfo = worker_info;
	unsigned nworkers;

	if (!winfo) {
		return;
	}

	/* Notify the workers to exit. */
	mutex_enter(&winfo->lock);
	winfo->exit = true;
	cv_broadcast(&winfo->cv);
	mutex_exit(&winfo->lock);

	/* Wait for them to finish and then destroy. */
	nworkers = winfo->worker_count;
	for (unsigned i = 0; i < nworkers; i++) {
		lwp_t *worker;

		if ((worker = winfo->worker[i]) != NULL) {
			kthread_join(worker);
		}
	}
	cv_destroy(&winfo->cv);
	cv_destroy(&winfo->exit_cv);
	mutex_destroy(&winfo->lock);
	kmem_free(winfo, offsetof(npf_workerinfo_t, worker[nworkers]));
	worker_info = NULL;
}

int
npf_worker_addfunc(npf_t *npf, npf_workfunc_t work)
{
	KASSERTMSG(npf->worker_flags == 0,
	    "the task must be added before the npf_worker_enlist() call");

	for (unsigned i = 0; i < NPF_MAX_WORKS; i++) {
		if (npf->worker_funcs[i] == NULL) {
			npf->worker_funcs[i] = work;
			return 0;
		}
	}
	return -1;
}

void
npf_worker_signal(npf_t *npf)
{
	npf_workerinfo_t *winfo = worker_info;

	if ((npf->worker_flags & WFLAG_ACTIVE) == 0) {
		return;
	}
	KASSERT(winfo != NULL);

	mutex_enter(&winfo->lock);
	cv_signal(&winfo->cv);
	mutex_exit(&winfo->lock);
}

/*
 * npf_worker_enlist: add the NPF instance for worker(s) to process.
 */
void
npf_worker_enlist(npf_t *npf)
{
	npf_workerinfo_t *winfo = worker_info;

	KASSERT(npf->worker_flags == 0);
	if (!winfo) {
		return;
	}

	mutex_enter(&winfo->lock);
	LIST_INSERT_HEAD(&winfo->instances, npf, worker_entry);
	npf->worker_flags |= WFLAG_ACTIVE;
	mutex_exit(&winfo->lock);
}

/*
 * npf_worker_discharge: remove the NPF instance the list for workers.
 *
 * => May block waiting for a worker to finish processing the instance.
 */
void
npf_worker_discharge(npf_t *npf)
{
	npf_workerinfo_t *winfo = worker_info;

	if ((npf->worker_flags & WFLAG_ACTIVE) == 0) {
		return;
	}
	KASSERT(winfo != NULL);

	/*
	 * Notify the worker(s) that we are removing this instance.
	 */
	mutex_enter(&winfo->lock);
	KASSERT(npf->worker_flags & WFLAG_ACTIVE);
	npf->worker_flags |= WFLAG_REMOVE;
	cv_broadcast(&winfo->cv);

	/* Wait for a worker to process this request. */
	while (npf->worker_flags & WFLAG_ACTIVE) {
		cv_wait(&winfo->exit_cv, &winfo->lock);
	}
	mutex_exit(&winfo->lock);
	KASSERT(npf->worker_flags == 0);
}

static void
remove_npf_instance(npf_workerinfo_t *winfo, npf_t *npf)
{
	KASSERT(mutex_owned(&winfo->lock));
	KASSERT(npf->worker_flags & WFLAG_ACTIVE);
	KASSERT(npf->worker_flags & WFLAG_REMOVE);

	/*
	 * Remove the NPF instance:
	 * - Release any structures owned by the worker.
	 * - Remove the instance from the list.
	 * - Notify any thread waiting for removal to complete.
	 */
	if (npf->worker_flags & WFLAG_INITED) {
		npfk_thread_unregister(npf);
	}
	LIST_REMOVE(npf, worker_entry);
	npf->worker_flags = 0;
	cv_broadcast(&winfo->exit_cv);
}

static unsigned
process_npf_instance(npf_workerinfo_t *winfo, npf_t *npf)
{
	npf_workfunc_t work;

	KASSERT(mutex_owned(&winfo->lock));

	if (npf->worker_flags & WFLAG_REMOVE) {
		remove_npf_instance(winfo, npf);
		return NPF_GC_MAXWAIT;
	}

	if ((npf->worker_flags & WFLAG_INITED) == 0) {
		npfk_thread_register(npf);
		npf->worker_flags |= WFLAG_INITED;
	}

	/* Run the jobs. */
	for (unsigned i = 0; i < NPF_MAX_WORKS; i++) {
		if ((work = npf->worker_funcs[i]) == NULL) {
			break;
		}
		work(npf);
	}

	return MAX(MIN(npf->worker_wait_time, NPF_GC_MAXWAIT), NPF_GC_MINWAIT);
}

/*
 * npf_worker: the main worker loop, processing enlisted NPF instances.
 *
 * XXX: Currently, npf_workerinfo_t::lock would serialize all workers,
 * so there is no point to have more than one worker; but there might
 * not be much point anyway.
 */
static void
npf_worker(void *arg)
{
	npf_workerinfo_t *winfo = arg;
	npf_t *npf;

	mutex_enter(&winfo->lock);
	for (;;) {
		unsigned wait_time = NPF_GC_MAXWAIT;

		/*
		 * Iterate all instances.  We do not use LIST_FOREACH here,
		 * since the instance can be removed.
		 */
		npf = LIST_FIRST(&winfo->instances);
		while (npf) {
			npf_t *next = LIST_NEXT(npf, worker_entry);
			unsigned i_wait_time = process_npf_instance(winfo, npf);
			wait_time = MIN(wait_time, i_wait_time);
			npf = next;
		}

		/*
		 * Sleep and periodically wake up, unless we get notified.
		 */
		if (winfo->exit) {
			break;
		}
		cv_timedwait(&winfo->cv, &winfo->lock, mstohz(wait_time));
	}
	mutex_exit(&winfo->lock);

	KASSERTMSG(LIST_EMPTY(&winfo->instances),
	    "NPF instances must be discharged before the npfk_sysfini() call");

	kthread_exit(0);
}
