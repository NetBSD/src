/*	$NetBSD: timer.c,v 1.2.2.2 2024/02/25 15:47:24 martin Exp $	*/

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

#include <uv.h>

#include <isc/netmgr.h>
#include <isc/util.h>

#include "netmgr-int.h"

struct isc_nm_timer {
	isc_refcount_t references;
	uv_timer_t timer;
	isc_nmhandle_t *handle;
	isc_nm_timer_cb cb;
	void *cbarg;
};

void
isc_nm_timer_create(isc_nmhandle_t *handle, isc_nm_timer_cb cb, void *cbarg,
		    isc_nm_timer_t **timerp) {
	isc__networker_t *worker = NULL;
	isc_nmsocket_t *sock = NULL;
	isc_nm_timer_t *timer = NULL;
	int r;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;
	worker = &sock->mgr->workers[isc_nm_tid()];

	/* TODO: per-loop object cache */
	timer = isc_mem_get(sock->mgr->mctx, sizeof(*timer));
	*timer = (isc_nm_timer_t){ .cb = cb, .cbarg = cbarg };
	isc_refcount_init(&timer->references, 1);
	isc_nmhandle_attach(handle, &timer->handle);

	r = uv_timer_init(&worker->loop, &timer->timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);

	uv_handle_set_data((uv_handle_t *)&timer->timer, timer);

	*timerp = timer;
}

void
isc_nm_timer_attach(isc_nm_timer_t *timer, isc_nm_timer_t **timerp) {
	REQUIRE(timer != NULL);
	REQUIRE(timerp != NULL && *timerp == NULL);

	isc_refcount_increment(&timer->references);
	*timerp = timer;
}

static void
timer_destroy(uv_handle_t *uvhandle) {
	isc_nm_timer_t *timer = uv_handle_get_data(uvhandle);
	isc_nmhandle_t *handle = timer->handle;
	isc_mem_t *mctx = timer->handle->sock->mgr->mctx;

	isc_mem_put(mctx, timer, sizeof(*timer));

	isc_nmhandle_detach(&handle);
}

void
isc_nm_timer_detach(isc_nm_timer_t **timerp) {
	isc_nm_timer_t *timer = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(timerp != NULL && *timerp != NULL);

	timer = *timerp;
	*timerp = NULL;

	handle = timer->handle;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	if (isc_refcount_decrement(&timer->references) == 1) {
		int r = uv_timer_stop(&timer->timer);
		UV_RUNTIME_CHECK(uv_timer_stop, r);
		uv_close((uv_handle_t *)&timer->timer, timer_destroy);
	}
}

static void
timer_cb(uv_timer_t *uvtimer) {
	isc_nm_timer_t *timer = uv_handle_get_data((uv_handle_t *)uvtimer);

	REQUIRE(timer->cb != NULL);

	timer->cb(timer->cbarg, ISC_R_TIMEDOUT);
}

void
isc_nm_timer_start(isc_nm_timer_t *timer, uint64_t timeout) {
	int r = uv_timer_start(&timer->timer, timer_cb, timeout, 0);
	UV_RUNTIME_CHECK(uv_timer_start, r);
}

void
isc_nm_timer_stop(isc_nm_timer_t *timer) {
	int r = uv_timer_stop(&timer->timer);
	UV_RUNTIME_CHECK(uv_timer_stop, r);
}
