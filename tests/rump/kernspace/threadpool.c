/*	$NetBSD: threadpool.c,v 1.5 2019/01/04 05:35:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__RCSID("$NetBSD: threadpool.c,v 1.5 2019/01/04 05:35:24 thorpej Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/threadpool.h>

#include "kernspace.h"

void
rumptest_threadpool_unbound_lifecycle(void)
{
	struct threadpool *pool0, *pool1, *pool2;
	int error;

	error = threadpool_get(&pool0, PRI_NONE);
	KASSERT(error == 0);

	error = threadpool_get(&pool1, PRI_NONE);
	KASSERT(error == 0);

	KASSERT(pool0 == pool1);

	error = threadpool_get(&pool2, PRI_KERNEL_RT);
	KASSERT(error == 0);

	KASSERT(pool0 != pool2);

	threadpool_put(pool0, PRI_NONE);
	threadpool_put(pool1, PRI_NONE);
	threadpool_put(pool2, PRI_KERNEL_RT);
}

void
rumptest_threadpool_percpu_lifecycle(void)
{
	struct threadpool_percpu *pcpu0, *pcpu1, *pcpu2;
	int error;

	error = threadpool_percpu_get(&pcpu0, PRI_NONE);
	KASSERT(error == 0);

	error = threadpool_percpu_get(&pcpu1, PRI_NONE);
	KASSERT(error == 0);

	KASSERT(pcpu0 == pcpu1);

	error = threadpool_percpu_get(&pcpu2, PRI_KERNEL_RT);
	KASSERT(error == 0);

	KASSERT(pcpu0 != pcpu2);

	threadpool_percpu_put(pcpu0, PRI_NONE);
	threadpool_percpu_put(pcpu1, PRI_NONE);
	threadpool_percpu_put(pcpu2, PRI_KERNEL_RT);
}

struct test_job_data {
	kmutex_t mutex;
	kcondvar_t cond;
	unsigned int count;
	struct threadpool_job job;
};

#define	FINAL_COUNT	12345

static void
test_job_func_schedule(struct threadpool_job *job)
{
	struct test_job_data *data =
	    container_of(job, struct test_job_data, job);
	
	mutex_enter(&data->mutex);
	KASSERT(data->count != FINAL_COUNT);
	data->count++;
	cv_broadcast(&data->cond);
	threadpool_job_done(job);
	mutex_exit(&data->mutex);
}

static void
test_job_func_cancel(struct threadpool_job *job)
{
	struct test_job_data *data =
	    container_of(job, struct test_job_data, job);

	mutex_enter(&data->mutex);
	if (data->count == 0) {
		data->count = 1;
		cv_broadcast(&data->cond);
	}
	while (data->count != FINAL_COUNT - 1)
		cv_wait(&data->cond, &data->mutex);
	data->count = FINAL_COUNT;
	cv_broadcast(&data->cond);
	threadpool_job_done(job);
	mutex_exit(&data->mutex);
}

static void
init_test_job_data(struct test_job_data *data, threadpool_job_fn_t fn)
{
	mutex_init(&data->mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&data->cond, "testjob");
	threadpool_job_init(&data->job, fn, &data->mutex, "testjob");
	data->count = 0;
}

static void
fini_test_job_data(struct test_job_data *data)
{
	threadpool_job_destroy(&data->job);
	cv_destroy(&data->cond);
	mutex_destroy(&data->mutex);
}

void
rumptest_threadpool_unbound_schedule(void)
{
	struct test_job_data data;
	struct threadpool *pool;
	int error;

	error = threadpool_get(&pool, PRI_NONE);
	KASSERT(error == 0);

	init_test_job_data(&data, test_job_func_schedule);

	mutex_enter(&data.mutex);
	while (data.count != FINAL_COUNT) {
		threadpool_schedule_job(pool, &data.job);
		error = cv_timedwait(&data.cond, &data.mutex, hz * 2);
		KASSERT(error != EWOULDBLOCK);
	}
	mutex_exit(&data.mutex);

	fini_test_job_data(&data);

	threadpool_put(pool, PRI_NONE);
}

void
rumptest_threadpool_percpu_schedule(void)
{
	struct test_job_data data;
	struct threadpool_percpu *pcpu;
	struct threadpool *pool;
	int error;

	error = threadpool_percpu_get(&pcpu, PRI_NONE);
	KASSERT(error == 0);

	pool = threadpool_percpu_ref(pcpu);

	init_test_job_data(&data, test_job_func_schedule);

	mutex_enter(&data.mutex);
	while (data.count != FINAL_COUNT) {
		threadpool_schedule_job(pool, &data.job);
		error = cv_timedwait(&data.cond, &data.mutex, hz * 2);
		KASSERT(error != EWOULDBLOCK);
	}
	mutex_exit(&data.mutex);

	fini_test_job_data(&data);

	threadpool_percpu_put(pcpu, PRI_NONE);
}

void
rumptest_threadpool_job_cancel(void)
{
	struct test_job_data data;
	struct threadpool *pool;
	int error;
	bool rv;

	error = threadpool_get(&pool, PRI_NONE);
	KASSERT(error == 0);

	init_test_job_data(&data, test_job_func_cancel);

	mutex_enter(&data.mutex);
	threadpool_schedule_job(pool, &data.job);
	while (data.count == 0)
		cv_wait(&data.cond, &data.mutex);
	KASSERT(data.count == 1);

	/* Job is already running (and is not finished); this shold fail. */
	rv = threadpool_cancel_job_async(pool, &data.job);
	KASSERT(rv == false);

	data.count = FINAL_COUNT - 1;
	cv_broadcast(&data.cond);
	
	/* Now wait for the job to finish. */
	threadpool_cancel_job(pool, &data.job);
	KASSERT(data.count == FINAL_COUNT);
	mutex_exit(&data.mutex);

	fini_test_job_data(&data);

	threadpool_put(pool, PRI_NONE);
}

void
rumptest_threadpool_job_cancelthrash(void)
{
	struct test_job_data data;
	struct threadpool *pool;
	int i, error;

	error = threadpool_get(&pool, PRI_NONE);
	KASSERT(error == 0);

	init_test_job_data(&data, test_job_func_cancel);

	mutex_enter(&data.mutex);
	for (i = 0; i < 10000; i++) {
		threadpool_schedule_job(pool, &data.job);
		if ((i % 3) == 0) {
			mutex_exit(&data.mutex);
			mutex_enter(&data.mutex);
		}
		/*
		 * If the job managed to start, ensure that its exit
		 * condition is met so that we don't wait forever
		 * for the job to finish.
		 */
		data.count = FINAL_COUNT - 1;
		cv_broadcast(&data.cond);

		threadpool_cancel_job(pool, &data.job);

		/*
		 * After cancellation, either the job didn't start
		 * (data.count == FINAL_COUNT - 1, per above) or
		 * it finished (data.count == FINAL_COUNT).
		 */
		KASSERT(data.count == (FINAL_COUNT - 1) ||
		    data.count == FINAL_COUNT);

		/* Reset for the loop. */
		data.count = 0;
	}
	mutex_exit(&data.mutex);

	fini_test_job_data(&data);

	threadpool_put(pool, PRI_NONE);
}
