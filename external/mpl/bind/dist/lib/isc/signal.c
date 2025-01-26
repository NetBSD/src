/*	$NetBSD: signal.c,v 1.1.1.1 2025/01/26 16:12:30 christos Exp $	*/

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

#include <isc/loop.h>
#include <isc/signal.h>
#include <isc/uv.h>

#include "loop_p.h"

isc_signal_t *
isc_signal_new(isc_loopmgr_t *loopmgr, isc_signal_cb cb, void *cbarg,
	       int signum) {
	isc_loop_t *loop = NULL;
	isc_signal_t *signal = NULL;
	int r;

	loop = DEFAULT_LOOP(loopmgr);

	signal = isc_mem_get(isc_loop_getmctx(loop), sizeof(*signal));
	*signal = (isc_signal_t){
		.magic = SIGNAL_MAGIC,
		.cb = cb,
		.cbarg = cbarg,
		.signum = signum,
	};

	isc_loop_attach(loop, &signal->loop);

	r = uv_signal_init(&loop->loop, &signal->signal);
	UV_RUNTIME_CHECK(uv_signal_init, r);

	uv_handle_set_data((uv_handle_t *)&signal->signal, signal);

	return signal;
}

static void
isc__signal_destroy_cb(uv_handle_t *handle) {
	isc_signal_t *signal = uv_handle_get_data(handle);
	isc_loop_t *loop = NULL;

	REQUIRE(VALID_SIGNAL(signal));

	loop = signal->loop;
	isc_mem_put(loop->mctx, signal, sizeof(*signal));
	isc_loop_detach(&loop);
}

void
isc_signal_destroy(isc_signal_t **signalp) {
	isc_signal_t *signal = NULL;

	REQUIRE(signalp != NULL);
	REQUIRE(VALID_SIGNAL(*signalp));

	signal = *signalp;
	*signalp = NULL;

	uv_close((uv_handle_t *)&signal->signal, isc__signal_destroy_cb);
}

void
isc_signal_stop(isc_signal_t *signal) {
	int r;

	REQUIRE(VALID_SIGNAL(signal));
	r = uv_signal_stop(&signal->signal);
	UV_RUNTIME_CHECK(uv_signal_stop, r);
}

static void
isc__signal_cb(uv_signal_t *handle, int signum) {
	isc_signal_t *signal = uv_handle_get_data((uv_handle_t *)handle);

	REQUIRE(VALID_SIGNAL(signal));
	REQUIRE(signum == signal->signum);

	signal->cb(signal->cbarg, signum);
}

void
isc_signal_start(isc_signal_t *signal) {
	int r;

	REQUIRE(VALID_SIGNAL(signal));
	r = uv_signal_start(&signal->signal, isc__signal_cb, signal->signum);
	UV_RUNTIME_CHECK(uv_signal_start, r);
}
