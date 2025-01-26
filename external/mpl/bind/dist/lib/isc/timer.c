/*	$NetBSD: timer.c,v 1.15 2025/01/26 16:25:39 christos Exp $	*/

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

/*! \file */

#include <stdbool.h>

#include <isc/async.h>
#include <isc/condition.h>
#include <isc/heap.h>
#include <isc/job.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/refcount.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>
#include <isc/uv.h>

#include "loop_p.h"

#define TIMER_MAGIC    ISC_MAGIC('T', 'I', 'M', 'R')
#define VALID_TIMER(t) ISC_MAGIC_VALID(t, TIMER_MAGIC)

struct isc_timer {
	unsigned int magic;
	isc_loop_t *loop;
	uv_timer_t timer;
	isc_job_cb cb;
	void *cbarg;
	uint64_t timeout;
	uint64_t repeat;
	atomic_bool running;
};

void
isc_timer_create(isc_loop_t *loop, isc_job_cb cb, void *cbarg,
		 isc_timer_t **timerp) {
	int r;
	isc_timer_t *timer;
	isc_loopmgr_t *loopmgr = NULL;

	REQUIRE(cb != NULL);
	REQUIRE(timerp != NULL && *timerp == NULL);

	REQUIRE(VALID_LOOP(loop));

	loopmgr = loop->loopmgr;

	REQUIRE(VALID_LOOPMGR(loopmgr));
	REQUIRE(loop == isc_loop());

	timer = isc_mem_get(loop->mctx, sizeof(*timer));
	*timer = (isc_timer_t){
		.cb = cb,
		.cbarg = cbarg,
		.magic = TIMER_MAGIC,
	};

	isc_loop_attach(loop, &timer->loop);

	r = uv_timer_init(&loop->loop, &timer->timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data(&timer->timer, timer);

	*timerp = timer;
}

void
isc_timer_stop(isc_timer_t *timer) {
	REQUIRE(VALID_TIMER(timer));

	if (!atomic_compare_exchange_strong_acq_rel(&timer->running,
						    &(bool){ true }, false))
	{
		/* Timer was already stopped */
		return;
	}

	/* Stop the timer, if the loops are matching */
	if (timer->loop == isc_loop()) {
		uv_timer_stop(&timer->timer);
	}
}

static void
timer_cb(uv_timer_t *handle) {
	isc_timer_t *timer = uv_handle_get_data(handle);

	REQUIRE(VALID_TIMER(timer));

	if (!atomic_load_acquire(&timer->running)) {
		uv_timer_stop(&timer->timer);
		return;
	}

	timer->cb(timer->cbarg);
}

void
isc_timer_start(isc_timer_t *timer, isc_timertype_t type,
		const isc_interval_t *interval) {
	isc_loopmgr_t *loopmgr = NULL;
	isc_loop_t *loop = NULL;
	int r;

	REQUIRE(VALID_TIMER(timer));
	REQUIRE(type == isc_timertype_ticker || type == isc_timertype_once);
	REQUIRE(timer->loop == isc_loop());

	loop = timer->loop;

	REQUIRE(VALID_LOOP(loop));

	loopmgr = loop->loopmgr;

	REQUIRE(VALID_LOOPMGR(loopmgr));

	switch (type) {
	case isc_timertype_once:
		timer->timeout = isc_interval_ms(interval);
		timer->repeat = 0;
		break;
	case isc_timertype_ticker:
		timer->timeout = timer->repeat = isc_interval_ms(interval);
		break;
	default:
		UNREACHABLE();
	}

	atomic_store_release(&timer->running, true);
	r = uv_timer_start(&timer->timer, timer_cb, timer->timeout,
			   timer->repeat);
	UV_RUNTIME_CHECK(uv_timer_start, r);
}

static void
timer_close(uv_handle_t *handle) {
	isc_timer_t *timer = uv_handle_get_data(handle);
	isc_loop_t *loop;

	REQUIRE(VALID_TIMER(timer));

	loop = timer->loop;

	isc_mem_put(loop->mctx, timer, sizeof(*timer));

	isc_loop_detach(&loop);
}

static void
timer_destroy(void *arg) {
	isc_timer_t *timer = arg;

	atomic_store_release(&timer->running, false);
	uv_timer_stop(&timer->timer);
	uv_close(&timer->timer, timer_close);
}

void
isc_timer_destroy(isc_timer_t **timerp) {
	isc_timer_t *timer = NULL;

	REQUIRE(timerp != NULL && VALID_TIMER(*timerp));

	timer = *timerp;
	*timerp = NULL;

	REQUIRE(timer->loop == isc_loop());

	timer_destroy(timer);
}

void
isc_timer_async_destroy(isc_timer_t **timerp) {
	isc_timer_t *timer = NULL;

	REQUIRE(timerp != NULL && VALID_TIMER(*timerp));

	timer = *timerp;
	*timerp = NULL;

	isc_timer_stop(timer);
	isc_async_run(timer->loop, timer_destroy, timer);
}
