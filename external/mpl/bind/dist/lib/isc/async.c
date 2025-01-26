/*	$NetBSD: async.c,v 1.2 2025/01/26 16:25:36 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/async.h>
#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/condition.h>
#include <isc/job.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/signal.h>
#include <isc/strerr.h>
#include <isc/thread.h>
#include <isc/util.h>
#include <isc/uv.h>
#include <isc/work.h>

#include "async_p.h"
#include "job_p.h"
#include "loop_p.h"

void
isc_async_run(isc_loop_t *loop, isc_job_cb cb, void *cbarg) {
	REQUIRE(VALID_LOOP(loop));
	REQUIRE(cb != NULL);

	isc_job_t *job = isc_mem_get(loop->mctx, sizeof(*job));
	*job = (isc_job_t){
		.cb = cb,
		.cbarg = cbarg,
	};

	cds_wfcq_node_init(&job->wfcq_node);

	/*
	 * cds_wfcq_enqueue() is non-blocking and enqueues the job to async
	 * queue.
	 *
	 * The function returns 'false' in case the queue was empty - in such
	 * case we need to trigger the async callback.
	 */
	if (!cds_wfcq_enqueue(&loop->async_jobs.head, &loop->async_jobs.tail,
			      &job->wfcq_node))
	{
		int r = uv_async_send(&loop->async_trigger);
		UV_RUNTIME_CHECK(uv_async_send, r);
	}
}

void
isc__async_cb(uv_async_t *handle) {
	isc_loop_t *loop = uv_handle_get_data(handle);
	isc_jobqueue_t jobs;

	REQUIRE(VALID_LOOP(loop));

	/* Initialize local wfcqueue */
	__cds_wfcq_init(&jobs.head, &jobs.tail);

	/*
	 * Move all the elements from loop->async_jobs to a local jobs queue.
	 *
	 * __cds_wfcq_splice_blocking() assumes that synchronization is
	 * done externally - there's no internal locking, unlike
	 * cds_wfcq_splice_blocking(), and we do not need to check whether
	 * it needs to block, unlike __cds_wfcq_splice_nonblocking().
	 *
	 * The reason we can use __cds_wfcq_splice_blocking() is that the
	 * only other function we use is cds_wfcq_enqueue() which doesn't
	 * require any synchronization (see the table in urcu/wfcqueue.h
	 * for more details).
	 */
	enum cds_wfcq_ret ret = __cds_wfcq_splice_blocking(
		&jobs.head, &jobs.tail, &loop->async_jobs.head,
		&loop->async_jobs.tail);
	INSIST(ret != CDS_WFCQ_RET_WOULDBLOCK);
	if (ret == CDS_WFCQ_RET_SRC_EMPTY) {
		/*
		 * Nothing to do, the source queue was empty - most
		 * probably we were called from isc__async_close() below.
		 */
		return;
	}

	/*
	 * Walk through the local queue which has now all the members copied
	 * locally, and call the callbacks and free all the isc_job_t(s).
	 */
	struct cds_wfcq_node *node, *next;
	__cds_wfcq_for_each_blocking_safe(&jobs.head, &jobs.tail, node, next) {
		isc_job_t *job = caa_container_of(node, isc_job_t, wfcq_node);

		job->cb(job->cbarg);

		isc_mem_put(loop->mctx, job, sizeof(*job));
	}
}

void
isc__async_close(uv_handle_t *handle) {
	isc_loop_t *loop = uv_handle_get_data(handle);

	isc__async_cb(&loop->async_trigger);
}
