/*	$NetBSD: work.c,v 1.1.1.1 2025/01/26 16:12:30 christos Exp $	*/

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

#include <isc/job.h>
#include <isc/loop.h>
#include <isc/urcu.h>
#include <isc/uv.h>
#include <isc/work.h>

#include "loop_p.h"

static void
isc__work_cb(uv_work_t *req) {
	isc_work_t *work = uv_req_get_data((uv_req_t *)req);

	rcu_register_thread();

	work->work_cb(work->cbarg);

	rcu_unregister_thread();
}

static void
isc__after_work_cb(uv_work_t *req, int status) {
	isc_work_t *work = uv_req_get_data((uv_req_t *)req);
	isc_loop_t *loop = work->loop;

	UV_RUNTIME_CHECK(uv_after_work_cb, status);

	work->after_work_cb(work->cbarg);

	isc_mem_put(loop->mctx, work, sizeof(*work));

	isc_loop_detach(&loop);
}

void
isc_work_enqueue(isc_loop_t *loop, isc_work_cb work_cb,
		 isc_after_work_cb after_work_cb, void *cbarg) {
	isc_work_t *work = NULL;
	int r;

	REQUIRE(VALID_LOOP(loop));
	REQUIRE(work_cb != NULL);
	REQUIRE(after_work_cb != NULL);

	work = isc_mem_get(loop->mctx, sizeof(*work));
	*work = (isc_work_t){
		.work_cb = work_cb,
		.after_work_cb = after_work_cb,
		.cbarg = cbarg,
	};

	isc_loop_attach(loop, &work->loop);

	uv_req_set_data((uv_req_t *)&work->work, work);

	r = uv_queue_work(&loop->loop, &work->work, isc__work_cb,
			  isc__after_work_cb);
	UV_RUNTIME_CHECK(uv_queue_work, r);
}
