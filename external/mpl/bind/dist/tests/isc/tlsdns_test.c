/*	$NetBSD: tlsdns_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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
#include <isc/loop.h>
#include <isc/nonce.h>
#include <isc/os.h>
#include <isc/quota.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>
#include <isc/util.h>
#include <isc/uv.h>

#include "uv_wrap.h"
#define KEEP_BEFORE

#include "netmgr_common.h"

#include <tests/isc.h>

static void
start_listening(uint32_t nworkers, isc_nm_accept_cb_t accept_cb,
		isc_nm_recv_cb_t recv_cb) {
	isc_result_t result = isc_nm_listenstreamdns(
		listen_nm, nworkers, &tcp_listen_addr, recv_cb, NULL, accept_cb,
		NULL, 128, NULL, tcp_listen_tlsctx, get_proxy_type(),
		&listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_loop_teardown(mainloop, stop_listening, listen_sock);
}

static void
tlsdns_connect(isc_nm_t *nm) {
	isc_nm_streamdnsconnect(
		nm, &tcp_connect_addr, &tcp_listen_addr, connect_connect_cb,
		tlsdns_connect, T_CONNECT, tcp_connect_tlsctx,
		tcp_tlsctx_client_sess_cache, get_proxy_type(), NULL);
}

ISC_LOOP_TEST_IMPL(tlsdns_noop) {
	start_listening(ISC_NM_LISTEN_ONE, noop_accept_cb, noop_recv_cb);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_streamdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
				connect_success_cb, tlsdns_connect, T_CONNECT,
				tcp_connect_tlsctx,
				tcp_tlsctx_client_sess_cache, get_proxy_type(),
				NULL);
}

ISC_LOOP_TEST_IMPL(tlsdns_noresponse) {
	start_listening(ISC_NM_LISTEN_ALL, noop_accept_cb, noop_recv_cb);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_streamdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
				connect_connect_cb, tlsdns_connect, T_CONNECT,
				tcp_connect_tlsctx,
				tcp_tlsctx_client_sess_cache, get_proxy_type(),
				NULL);
}

ISC_LOOP_TEST_IMPL(tlsdns_timeout_recovery) {
	/*
	 * Accept connections but don't send responses, forcing client
	 * reads to time out.
	 */
	noanswer = true;
	start_listening(ISC_NM_LISTEN_ONE, listen_accept_cb, listen_read_cb);

	/*
	 * Shorten all the TCP client timeouts to 0.05 seconds, connect,
	 * then sleep for at least a second for each 'tick'.
	 * timeout_retry_cb() will give up after five timeouts.
	 */
	connect_readcb = timeout_retry_cb;
	isc_nm_settimeouts(connect_nm, T_SOFT, T_SOFT, T_SOFT, T_SOFT);
	isc_refcount_increment0(&active_cconnects);
	isc_nm_streamdnsconnect(
		connect_nm, &tcp_connect_addr, &tcp_listen_addr,
		connect_connect_cb, tlsdns_connect, T_SOFT, tcp_connect_tlsctx,
		tcp_tlsctx_client_sess_cache, get_proxy_type(), NULL);
}

ISC_LOOP_TEST_IMPL(tlsdns_recv_one) {
	start_listening(ISC_NM_LISTEN_ONE, listen_accept_cb, listen_read_cb);

	isc_refcount_increment0(&active_cconnects);
	tlsdns_connect(connect_nm);
}

ISC_LOOP_TEST_IMPL(tlsdns_recv_two) {
	start_listening(ISC_NM_LISTEN_ONE, listen_accept_cb, listen_read_cb);

	isc_refcount_increment0(&active_cconnects);
	tlsdns_connect(connect_nm);

	isc_refcount_increment0(&active_cconnects);
	tlsdns_connect(connect_nm);
}

ISC_LOOP_TEST_IMPL(tlsdns_recv_send) {
	start_listening(ISC_NM_LISTEN_ALL, listen_accept_cb, listen_read_cb);

	for (size_t i = 0; i < workers; i++) {
		isc_async_run(isc_loop_get(loopmgr, i),
			      stream_recv_send_connect, tlsdns_connect);
	}
}

/* PROXY tests */

ISC_LOOP_TEST_IMPL(proxy_tlsdns_noop) { loop_test_tlsdns_noop(arg); }

ISC_LOOP_TEST_IMPL(proxy_tlsdns_noresponse) {
	loop_test_tlsdns_noresponse(arg);
}

ISC_LOOP_TEST_IMPL(proxy_tlsdns_timeout_recovery) {
	loop_test_tlsdns_timeout_recovery(arg);
}

ISC_LOOP_TEST_IMPL(proxy_tlsdns_recv_one) { loop_test_tlsdns_recv_one(arg); }

ISC_LOOP_TEST_IMPL(proxy_tlsdns_recv_two) { loop_test_tlsdns_recv_two(arg); }

ISC_LOOP_TEST_IMPL(proxy_tlsdns_recv_send) { loop_test_tlsdns_recv_send(arg); }

/* PROXY over TLS tests */

ISC_LOOP_TEST_IMPL(proxytls_tlsdns_noop) { loop_test_tlsdns_noop(arg); }

ISC_LOOP_TEST_IMPL(proxytls_tlsdns_noresponse) {
	loop_test_tlsdns_noresponse(arg);
}

ISC_LOOP_TEST_IMPL(proxytls_tlsdns_timeout_recovery) {
	loop_test_tlsdns_timeout_recovery(arg);
}

ISC_LOOP_TEST_IMPL(proxytls_tlsdns_recv_one) { loop_test_tlsdns_recv_one(arg); }

ISC_LOOP_TEST_IMPL(proxytls_tlsdns_recv_two) { loop_test_tlsdns_recv_two(arg); }

ISC_LOOP_TEST_IMPL(proxytls_tlsdns_recv_send) {
	loop_test_tlsdns_recv_send(arg);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(tlsdns_noop, stream_noop_setup, stream_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(tlsdns_noresponse, stream_noresponse_setup,
		      stream_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(tlsdns_timeout_recovery, stream_timeout_recovery_setup,
		      stream_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(tlsdns_recv_one, stream_recv_one_setup,
		      stream_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(tlsdns_recv_two, stream_recv_two_setup,
		      stream_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(tlsdns_recv_send, stream_recv_send_setup,
		      stream_recv_send_teardown)

/* FIXME: Re-add the noalpn tests */

/* PROXY */

ISC_TEST_ENTRY_CUSTOM(proxy_tlsdns_noop, proxystream_noop_setup,
		      proxystream_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(proxy_tlsdns_noresponse, proxystream_noresponse_setup,
		      proxystream_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(proxy_tlsdns_timeout_recovery,
		      proxystream_timeout_recovery_setup,
		      proxystream_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(proxy_tlsdns_recv_one, proxystream_recv_one_setup,
		      proxystream_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxy_tlsdns_recv_two, proxystream_recv_two_setup,
		      proxystream_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxy_tlsdns_recv_send, proxystream_recv_send_setup,
		      proxystream_recv_send_teardown)

/* PROXY over TLS */

ISC_TEST_ENTRY_CUSTOM(proxytls_tlsdns_noop, proxystreamtls_noop_setup,
		      proxystreamtls_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(proxytls_tlsdns_noresponse,
		      proxystreamtls_noresponse_setup,
		      proxystreamtls_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(proxytls_tlsdns_timeout_recovery,
		      proxystreamtls_timeout_recovery_setup,
		      proxystreamtls_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(proxytls_tlsdns_recv_one, proxystreamtls_recv_one_setup,
		      proxystreamtls_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxytls_tlsdns_recv_two, proxystreamtls_recv_two_setup,
		      proxystreamtls_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxytls_tlsdns_recv_send, proxystreamtls_recv_send_setup,
		      proxystreamtls_recv_send_teardown)

ISC_TEST_LIST_END

static int
tlsdns_setup(void **state ISC_ATTR_UNUSED) {
	stream_port = TLSDNS_TEST_PORT;

	return 0;
}

ISC_TEST_MAIN_CUSTOM(tlsdns_setup, NULL)
