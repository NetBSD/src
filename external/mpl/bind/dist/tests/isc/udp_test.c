/*	$NetBSD: udp_test.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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

#include <isc/async.h>
#include <isc/job.h>
#include <isc/nonce.h>
#include <isc/os.h>
#include <isc/quota.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "uv_wrap.h"
#define KEEP_BEFORE

#include "netmgr/socket.c"
#include "netmgr/udp.c"
#include "netmgr_common.h"
#include "uv.c"

#include <tests/isc.h>

/* Callbacks */

static void
mock_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	     void *cbarg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(cbarg);
}

static void
udp_connect_nomemory_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	isc_refcount_decrement(&active_cconnects);
	assert_int_equal(eresult, ISC_R_NOMEMORY);

	isc_loopmgr_shutdown(loopmgr);
}

/* UDP */

ISC_LOOP_TEST_IMPL(mock_listenudp_uv_udp_open) {
	isc_result_t result = ISC_R_SUCCESS;

	WILL_RETURN(uv_udp_open, UV_ENOMEM);

	result = isc_nm_listenudp(netmgr, ISC_NM_LISTEN_ALL, &udp_listen_addr,
				  mock_recv_cb, NULL, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(mock_listenudp_uv_udp_bind) {
	isc_result_t result = ISC_R_SUCCESS;

	WILL_RETURN(uv_udp_bind, UV_EADDRINUSE);

	result = isc_nm_listenudp(netmgr, ISC_NM_LISTEN_ALL, &udp_listen_addr,
				  mock_recv_cb, NULL, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(mock_listenudp_uv_udp_recv_start) {
	isc_result_t result = ISC_R_SUCCESS;

	WILL_RETURN(uv_udp_recv_start, UV_EADDRINUSE);

	result = isc_nm_listenudp(netmgr, ISC_NM_LISTEN_ALL, &udp_listen_addr,
				  mock_recv_cb, NULL, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(mock_udpconnect_uv_udp_open) {
	WILL_RETURN(uv_udp_open, UV_ENOMEM);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(netmgr, &udp_connect_addr, &udp_listen_addr,
			  udp_connect_nomemory_cb, NULL, UDP_T_CONNECT);
	isc_loopmgr_shutdown(loopmgr);

	RESET_RETURN;
}

ISC_LOOP_TEST_IMPL(mock_udpconnect_uv_udp_bind) {
	WILL_RETURN(uv_udp_bind, UV_ENOMEM);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(netmgr, &udp_connect_addr, &udp_listen_addr,
			  udp_connect_nomemory_cb, NULL, UDP_T_CONNECT);
	isc_loopmgr_shutdown(loopmgr);

	RESET_RETURN;
}

ISC_LOOP_TEST_IMPL(mock_udpconnect_uv_udp_connect) {
	WILL_RETURN(uv_udp_connect, UV_ENOMEM);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(netmgr, &udp_connect_addr, &udp_listen_addr,
			  udp_connect_nomemory_cb, NULL, UDP_T_CONNECT);
	isc_loopmgr_shutdown(loopmgr);

	RESET_RETURN;
}

ISC_LOOP_TEST_IMPL(mock_udpconnect_uv_recv_buffer_size) {
	WILL_RETURN(uv_recv_buffer_size, UV_ENOMEM);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(netmgr, &udp_connect_addr, &udp_listen_addr,
			  connect_success_cb, NULL, UDP_T_CONNECT);
	isc_loopmgr_shutdown(loopmgr);

	RESET_RETURN;
}

ISC_LOOP_TEST_IMPL(mock_udpconnect_uv_send_buffer_size) {
	WILL_RETURN(uv_send_buffer_size, UV_ENOMEM);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(netmgr, &udp_connect_addr, &udp_listen_addr,
			  connect_success_cb, NULL, UDP_T_CONNECT);
	isc_loopmgr_shutdown(loopmgr);

	RESET_RETURN;
}

ISC_LOOP_TEST_IMPL(udp_noop) { udp_noop(arg); }

ISC_LOOP_TEST_IMPL(udp_noresponse) { udp_noresponse(arg); }

ISC_LOOP_TEST_IMPL(udp_timeout_recovery) { udp_timeout_recovery(arg); }

ISC_LOOP_TEST_IMPL(udp_shutdown_connect) { udp_shutdown_connect(arg); }

ISC_LOOP_TEST_IMPL(udp_shutdown_read) { udp_shutdown_read(arg); }

ISC_LOOP_TEST_IMPL(udp_cancel_read) { udp_cancel_read(arg); }

ISC_LOOP_TEST_IMPL(udp_recv_one) { udp_recv_one(arg); }

ISC_LOOP_TEST_IMPL(udp_recv_two) { udp_recv_two(arg); }

ISC_LOOP_TEST_IMPL(udp_recv_send) { udp_recv_send(arg); }

ISC_LOOP_TEST_IMPL(udp_double_read) { udp_double_read(arg); }

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(mock_listenudp_uv_udp_open, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_listenudp_uv_udp_bind, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_listenudp_uv_udp_recv_start, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_udpconnect_uv_udp_open, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_udpconnect_uv_udp_bind, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_udpconnect_uv_udp_connect, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_udpconnect_uv_recv_buffer_size, setup_udp_test,
		      teardown_udp_test)
ISC_TEST_ENTRY_CUSTOM(mock_udpconnect_uv_send_buffer_size, setup_udp_test,
		      teardown_udp_test)

ISC_TEST_ENTRY_CUSTOM(udp_noop, udp_noop_setup, udp_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_noresponse, udp_noresponse_setup,
		      udp_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_timeout_recovery, udp_timeout_recovery_setup,
		      udp_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_shutdown_read, udp_shutdown_read_setup,
		      udp_shutdown_read_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_cancel_read, udp_cancel_read_setup,
		      udp_cancel_read_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_shutdown_connect, udp_shutdown_connect_setup,
		      udp_shutdown_connect_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_double_read, udp_double_read_setup,
		      udp_double_read_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_recv_one, udp_recv_one_setup, udp_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_recv_two, udp_recv_two_setup, udp_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(udp_recv_send, udp_recv_send_setup,
		      udp_recv_send_teardown)

ISC_TEST_LIST_END

ISC_TEST_MAIN
