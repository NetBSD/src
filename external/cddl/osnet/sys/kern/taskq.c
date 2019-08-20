/*	$NetBSD: taskq.c,v 1.11 2019/08/20 08:12:50 hannken Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kcondvar.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/threadpool.h>

#include <sys/taskq.h>

struct taskq_executor {
	struct threadpool_job te_job;	/* Threadpool job serving the queue. */
	taskq_t *te_self;		/* Backpointer to the queue. */
	unsigned te_running:1;		/* True if the job is running. */
};

struct taskq {
	int tq_nthreads;		/* # of threads serving queue. */
	pri_t tq_pri;			/* Scheduling priority. */
	uint_t tq_flags;		/* Saved flags from taskq_create. */
	int tq_active;			/* # of tasks (queued or running). */
	int tq_running;			/* # of jobs currently running. */
	int tq_waiting;			/* # of jobs currently idle. */
	unsigned tq_destroyed:1;	/* True if queue gets destroyed. */
	kmutex_t tq_lock;		/* Queue and job lock. */
	kcondvar_t tq_cv;		/* Queue condvar. */
	struct taskq_executor *tq_executor; /* Array of jobs. */
	struct threadpool *tq_threadpool; /* Pool backing the jobs. */
	SIMPLEQ_HEAD(, taskq_ent) tq_list; /* Queue of tasks waiting. */
};

taskq_t *system_taskq;			/* General purpose task queue. */

static specificdata_key_t taskq_lwp_key; /* Null or taskq this thread runs. */

/*
 * Threadpool job to service tasks from task queue.
 * Runs until the task queue gets destroyed or the queue is empty for 10 secs.
 */
static void
task_executor(struct threadpool_job *job)
{
	struct taskq_executor *state = (struct taskq_executor *)job;
	taskq_t *tq = state->te_self;
	taskq_ent_t *tqe; 
	bool is_dynamic;
	int error;

	lwp_setspecific(taskq_lwp_key, tq);

	mutex_enter(&tq->tq_lock);
	while (!tq->tq_destroyed) {
		if (SIMPLEQ_EMPTY(&tq->tq_list)) {
			if (ISSET(tq->tq_flags, TASKQ_DYNAMIC))
				break;
			tq->tq_waiting++;
			error = cv_timedwait(&tq->tq_cv, &tq->tq_lock,
			    mstohz(10000));
			tq->tq_waiting--;
			if (SIMPLEQ_EMPTY(&tq->tq_list)) {
				if (error)
					break;
				continue;
			}
		}
		tqe = SIMPLEQ_FIRST(&tq->tq_list);
		KASSERT(tqe != NULL);
		SIMPLEQ_REMOVE_HEAD(&tq->tq_list, tqent_list);
		is_dynamic = tqe->tqent_dynamic;
		tqe->tqent_queued = 0;
		mutex_exit(&tq->tq_lock);

		(*tqe->tqent_func)(tqe->tqent_arg);

		mutex_enter(&tq->tq_lock);
		if (is_dynamic)
			kmem_free(tqe, sizeof(*tqe));
		tq->tq_active--;
	}
	state->te_running = 0;
	tq->tq_running--;
	threadpool_job_done(job);
	mutex_exit(&tq->tq_lock);

	lwp_setspecific(taskq_lwp_key, NULL);
}

void
taskq_init(void)
{

	lwp_specific_key_create(&taskq_lwp_key, NULL);
	system_taskq = taskq_create("system_taskq", ncpu * 4, PRI_KERNEL,
	    4, 512, TASKQ_DYNAMIC | TASKQ_PREPOPULATE);
	KASSERT(system_taskq != NULL);
}

void
taskq_fini(void)
{

	taskq_destroy(system_taskq);
	lwp_specific_key_delete(taskq_lwp_key);
}

/*
 * Dispatch a task entry creating executors as neeeded.
 */
static void
taskq_dispatch_common(taskq_t *tq, taskq_ent_t *tqe, uint_t flags)
{
	int i;

	KASSERT(mutex_owned(&tq->tq_lock));

	if (ISSET(flags, TQ_FRONT))
		SIMPLEQ_INSERT_HEAD(&tq->tq_list, tqe, tqent_list);
	else
		SIMPLEQ_INSERT_TAIL(&tq->tq_list, tqe, tqent_list);
	tqe->tqent_queued = 1;
	tq->tq_active++;
	if (tq->tq_waiting) {
		cv_signal(&tq->tq_cv);
		mutex_exit(&tq->tq_lock);
		return;
	}
	if (tq->tq_running < tq->tq_nthreads) {
		for (i = 0; i < tq->tq_nthreads; i++) {
			if (!tq->tq_executor[i].te_running) {
				tq->tq_executor[i].te_running = 1;
				tq->tq_running++;
				threadpool_schedule_job(tq->tq_threadpool,
				    &tq->tq_executor[i].te_job);
				break;
			}
		}
	}
	mutex_exit(&tq->tq_lock);
}

/*
 * Allocate and dispatch a task entry.
 */
taskqid_t
taskq_dispatch(taskq_t *tq, task_func_t func, void *arg, uint_t flags)
{
	taskq_ent_t *tqe;

	KASSERT(!ISSET(flags, ~(TQ_SLEEP | TQ_NOSLEEP | TQ_NOQUEUE)));
	KASSERT(ISSET(tq->tq_flags, TASKQ_DYNAMIC) ||
	    !ISSET(flags, TQ_NOQUEUE));

	if (ISSET(flags, (TQ_SLEEP | TQ_NOSLEEP)) == TQ_NOSLEEP)
		tqe = kmem_alloc(sizeof(*tqe), KM_NOSLEEP);
	else
		tqe = kmem_alloc(sizeof(*tqe), KM_SLEEP);
	if (tqe == NULL)
		return (taskqid_t) NULL;

	mutex_enter(&tq->tq_lock);
	if (ISSET(flags, TQ_NOQUEUE) && tq->tq_active == tq->tq_nthreads) {
		mutex_exit(&tq->tq_lock);
		kmem_free(tqe, sizeof(*tqe));
		return (taskqid_t) NULL;
	}
	tqe->tqent_dynamic = 1;
	tqe->tqent_queued = 0;
	tqe->tqent_func = func;
	tqe->tqent_arg = arg;
	taskq_dispatch_common(tq, tqe, flags);

	return (taskqid_t) tqe;
}

/*
 * Dispatch a preallocated task entry.
 * Assume caller zeroed it.
 */
void
taskq_dispatch_ent(taskq_t *tq, task_func_t func, void *arg, uint_t flags,
    taskq_ent_t *tqe)
{

	KASSERT(!ISSET(flags, ~(TQ_FRONT)));

	tqe->tqent_func = func;
	tqe->tqent_arg = arg;
	mutex_enter(&tq->tq_lock);
	taskq_dispatch_common(tq, tqe, flags);
}

/*
 * Wait until all tasks have completed.
 */
void
taskq_wait(taskq_t *tq)
{

	KASSERT(!taskq_member(tq, curlwp));

	mutex_enter(&tq->tq_lock);
	while (tq->tq_active)
		kpause("qwait", false, 1, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);
}

/*
 * True if the current thread is an executor for this queue.
 */
int
taskq_member(taskq_t *tq, kthread_t *thread)
{

	KASSERT(thread == curlwp);

	return (lwp_getspecific(taskq_lwp_key) == tq);
}

/*
 * Create a task queue.
 * Allocation hints are ignored.
 */
taskq_t *
taskq_create(const char *name, int nthreads, pri_t pri, int minalloc,
    int maxalloc, uint_t flags)
{
	int i;
	struct threadpool *threadpool;
	taskq_t *tq;

	KASSERT(!ISSET(flags,
	    ~(TASKQ_DYNAMIC | TASKQ_PREPOPULATE | TASKQ_THREADS_CPU_PCT)));

	if (threadpool_get(&threadpool, pri) != 0)
		return NULL;

	if (ISSET(flags, TASKQ_THREADS_CPU_PCT))
		nthreads = MAX((ncpu * nthreads) / 100, 1);

	tq = kmem_zalloc(sizeof(*tq), KM_SLEEP);
	tq->tq_nthreads = nthreads;
	tq->tq_pri = pri;
	tq->tq_flags = flags;
	mutex_init(&tq->tq_lock, NULL, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&tq->tq_cv, NULL, CV_DEFAULT, NULL);
	SIMPLEQ_INIT(&tq->tq_list);
	tq->tq_executor = kmem_alloc(sizeof(*tq->tq_executor) * nthreads,
	    KM_SLEEP);
	for (i = 0; i < nthreads; i++) {
		threadpool_job_init(&tq->tq_executor[i].te_job, task_executor,
		    &tq->tq_lock, "%s/%d", name, i);
		tq->tq_executor[i].te_self = tq;
		tq->tq_executor[i].te_running = 0;
	}
	tq->tq_threadpool = threadpool;

	return tq;
}

taskq_t *
taskq_create_proc(const char *name, int nthreads, pri_t pri, int minalloc,
    int maxalloc, struct proc *proc, uint_t flags)
{

	return taskq_create(name, nthreads, pri, minalloc, maxalloc, flags);
}

/*
 * Destroy a task queue.
 */
void
taskq_destroy(taskq_t *tq)
{
	int i;
	taskq_ent_t *tqe;

	KASSERT(!taskq_member(tq, curlwp));

	/* Wait for tasks to complete. */
	taskq_wait(tq);

	/* Mark destroyed and ask running executors to quit. */
	mutex_enter(&tq->tq_lock);
	tq->tq_destroyed = 1;
	cv_broadcast(&tq->tq_cv);

	/* Wait for all executors to quit. */
	while (tq->tq_running > 0)
		kpause("tqdestroy", false, 1, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);

	for (i = 0; i < tq->tq_nthreads; i++)
		threadpool_job_destroy(&tq->tq_executor[i].te_job);
	threadpool_put(tq->tq_threadpool, tq->tq_pri);
	mutex_destroy(&tq->tq_lock);
	cv_destroy(&tq->tq_cv);
	kmem_free(tq->tq_executor, sizeof(*tq->tq_executor) * tq->tq_nthreads);
	kmem_free(tq, sizeof(*tq));
}
