/*	$NetBSD: job.c,v 1.1.1.1 2025/01/26 16:12:30 christos Exp $	*/

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
#include <isc/job.h>
#include <isc/list.h>
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

#include "job_p.h"
#include "loop_p.h"
#include "probes.h"

/*
 * Public: #include <isc/job.h>
 */

void
isc_job_run(isc_loop_t *loop, isc_job_t *job, isc_job_cb cb, void *cbarg) {
	if (ISC_LIST_EMPTY(loop->run_jobs)) {
		uv_idle_start(&loop->run_trigger, isc__job_cb);
	}

	job->cb = cb;
	job->cbarg = cbarg;
	ISC_LINK_INIT(job, link);

	ISC_LIST_APPEND(loop->run_jobs, job, link);
}

/*
 * Protected: #include <job_p.h>
 */

void
isc__job_cb(uv_idle_t *handle) {
	isc_loop_t *loop = uv_handle_get_data(handle);
	ISC_LIST(isc_job_t) jobs = ISC_LIST_INITIALIZER;

	ISC_LIST_MOVE(jobs, loop->run_jobs);

	isc_job_t *job, *next;
	for (job = ISC_LIST_HEAD(jobs),
	    next = (job != NULL) ? ISC_LIST_NEXT(job, link) : NULL;
	     job != NULL;
	     job = next, next = job ? ISC_LIST_NEXT(job, link) : NULL)
	{
		isc_job_cb cb = job->cb;
		void *cbarg = job->cbarg;
		ISC_LIST_UNLINK(jobs, job, link);
		LIBISC_JOB_CB_BEFORE(job, cb, cbarg);
		cb(cbarg);
		LIBISC_JOB_CB_AFTER(job, cb, cbarg);
	}

	if (ISC_LIST_EMPTY(loop->run_jobs)) {
		uv_idle_stop(&loop->run_trigger);
	}
}

void
isc__job_close(uv_handle_t *handle) {
	isc_loop_t *loop = uv_handle_get_data(handle);

	isc__job_cb(&loop->run_trigger);
}
