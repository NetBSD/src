/*	$NetBSD: helper.c,v 1.1.1.1 2025/01/26 16:12:30 christos Exp $	*/

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

#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/condition.h>
#include <isc/helper.h>
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
isc_helper_run(isc_loop_t *loop, isc_job_cb cb, void *cbarg) {
	REQUIRE(VALID_LOOP(loop));
	REQUIRE(cb != NULL);

	isc_loop_t *helper = &loop->loopmgr->helpers[loop->tid];

	isc_job_t *job = isc_mem_get(helper->mctx, sizeof(*job));
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
	if (!cds_wfcq_enqueue(&helper->async_jobs.head,
			      &helper->async_jobs.tail, &job->wfcq_node))
	{
		int r = uv_async_send(&helper->async_trigger);
		UV_RUNTIME_CHECK(uv_async_send, r);
	}
}
