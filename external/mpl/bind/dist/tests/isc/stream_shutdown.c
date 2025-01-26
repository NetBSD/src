/*	$NetBSD: stream_shutdown.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * As a workaround, include an OpenSSL header file before including cmocka.h,
 * because OpenSSL 3.1.0 uses __attribute__(malloc), conflicting with a
 * redefined malloc in cmocka.h.
 */
#include <openssl/err.h>

#define UNIT_TESTING
#include <cmocka.h>

#include "netmgr_common.h"

#include <tests/isc.h>

/*
 * FIXME: This really needs two network managers, so there's predictable result
 * when shuttingdown the netmgr - right now there's a race whether the listening
 * or connecting sockets gets shutdown first
 */

static void
shutdownconnect_connectcb(isc_nmhandle_t *handle, isc_result_t eresult,
			  void *cbarg) {
	F();

	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SHUTTINGDOWN);
	assert_null(cbarg);

	isc_refcount_decrement(&active_cconnects);

	atomic_fetch_add(&cconnects, 1);
}

int
stream_shutdownconnect_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);
	return r;
}

int
proxystream_shutdownconnect_setup(void **state) {
	stream_use_PROXY = true;
	return stream_shutdownconnect_setup(state);
}

int
proxystream_shutdownconnect_teardown(void **state) {
	int r = stream_shutdownconnect_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_shutdownconnect_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_shutdownconnect_setup(state);
}

int
proxystreamtls_shutdownconnect_teardown(void **state) {
	int r = proxystream_shutdownconnect_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_shutdownconnect(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = stream_listen(stream_accept_cb, NULL, 128, NULL,
					    &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	/* Schedule the shutdown before the connect */
	isc_loopmgr_shutdown(loopmgr);

	stream_connect(shutdownconnect_connectcb, NULL, T_CONNECT);
}

int
stream_shutdownconnect_teardown(void **state ISC_ATTR_UNUSED) {
	X(cconnects);
	X(csends);
	X(creads);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 0);
	atomic_assert_int_eq(creads, 0);

	return teardown_netmgr_test(state);
}

/* Issue the shutdown before reading */

static void
shutdownread_readcb(isc_nmhandle_t *handle, isc_result_t eresult,
		    isc_region_t *region, void *cbarg) {
	F();
	assert_non_null(handle);
	assert_true(eresult == ISC_R_SHUTTINGDOWN ||
		    eresult == ISC_R_CONNECTIONRESET || eresult == ISC_R_EOF);
	assert_non_null(region);
	assert_null(cbarg);

	atomic_fetch_add(&creads, 1);
	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_creads);
}

static void
shutdownread_sendcb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	F();
	assert_non_null(handle);
	assert_true(eresult == ISC_R_SUCCESS || eresult == ISC_R_SHUTTINGDOWN ||
		    eresult == ISC_R_CONNECTIONRESET || eresult == ISC_R_EOF);
	assert_null(cbarg);

	atomic_fetch_add(&csends, 1);

	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_csends);
}

static void
shutdownread_connectcb(isc_nmhandle_t *handle, isc_result_t eresult,
		       void *cbarg) {
	F();

	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SUCCESS);
	assert_null(cbarg);

	isc_refcount_decrement(&active_cconnects);

	atomic_fetch_add(&cconnects, 1);

	/* Schedule the shutdown before read and send */
	isc_loopmgr_shutdown(loopmgr);

	isc_refcount_increment0(&active_creads);
	isc_nmhandle_ref(handle);
	isc_nm_read(handle, shutdownread_readcb, cbarg);

	isc_refcount_increment0(&active_csends);
	isc_nmhandle_ref(handle);
	isc_nm_send(handle, (isc_region_t *)&send_msg, shutdownread_sendcb,
		    cbarg);
}

int
stream_shutdownread_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);
	return r;
}

int
proxystream_shutdownread_setup(void **state) {
	stream_use_PROXY = true;
	return stream_shutdownread_setup(state);
}

int
proxystream_shutdownread_teardown(void **state) {
	int r = stream_shutdownread_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_shutdownread_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_shutdownread_setup(state);
}

int
proxystreamtls_shutdownread_teardown(void **state) {
	int r = proxystream_shutdownread_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_shutdownread(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = stream_listen(stream_accept_cb, NULL, 128, NULL,
					    &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	stream_connect(shutdownread_connectcb, NULL, T_CONNECT);
}

int
stream_shutdownread_teardown(void **state ISC_ATTR_UNUSED) {
	X(cconnects);
	X(csends);
	X(creads);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 1);

	return teardown_netmgr_test(state);
}
