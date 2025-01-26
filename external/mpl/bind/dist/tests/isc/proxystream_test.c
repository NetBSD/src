/*	$NetBSD: proxystream_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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

#include "netmgr/tcp.c"
#include "netmgr_common.h"

#include <tests/isc.h>

static isc_nm_proxyheader_info_t custom_info;

char complete_proxy_data[] = { 0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x0d, 0x0a,
			       0x51, 0x55, 0x49, 0x54, 0x0a, 0x21, 0x11,
			       0x00, 0x0c, 0x01, 0x02, 0x03, 0x04, 0x04,
			       0x03, 0x02, 0x01, 0x14, 0xe9, 0x14, 0xe9 };

/* TCP */
ISC_LOOP_TEST_IMPL(proxystream_noop) {
	stream_noop(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_noresponse) {
	stream_noresponse(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_shutdownconnect) {
	stream_shutdownconnect(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_shutdownread) {
	stream_shutdownread(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_timeout_recovery) {
	stream_timeout_recovery(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_recv_one) {
	stream_recv_one(arg);
	return;
}

static void
proxystream_recv_one_prerendered(void **arg ISC_ATTR_UNUSED) {
	isc_region_t header = { 0 };
	header.base = (unsigned char *)complete_proxy_data;
	header.length = sizeof(complete_proxy_data);

	isc_nm_proxyheader_info_init_complete(&custom_info, &header);

	set_proxyheader_info(&custom_info);

	stream_recv_one(arg);
}

ISC_LOOP_TEST_IMPL(proxystream_recv_one_prerendered) {
	proxystream_recv_one_prerendered(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_recv_two) {
	stream_recv_two(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_recv_send) {
	stream_recv_send(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_recv_send_sendback) {
	allow_send_back = true;
	stream_recv_send(arg);
	return;
}

/* TCP Quota */

ISC_LOOP_TEST_IMPL(proxystream_recv_one_quota) {
	atomic_store(&check_listener_quota, true);
	stream_recv_one(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_recv_two_quota) {
	atomic_store(&check_listener_quota, true);
	stream_recv_two(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystream_recv_send_quota) {
	atomic_store(&check_listener_quota, true);
	stream_recv_send(arg);
}

ISC_LOOP_TEST_IMPL(proxystream_recv_send_quota_sendback) {
	atomic_store(&check_listener_quota, true);
	allow_send_back = true;
	stream_recv_send(arg);
}

/* PROXY over TLS (as used by, e.g., dnsdist) */

/* TCP */
ISC_LOOP_TEST_IMPL(proxystreamtls_noop) {
	stream_noop(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_noresponse) {
	stream_noresponse(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_shutdownconnect) {
	stream_shutdownconnect(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_shutdownread) {
	stream_shutdownread(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_timeout_recovery) {
	stream_timeout_recovery(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_one) {
	stream_recv_one(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_one_prerendered) {
	proxystream_recv_one_prerendered(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_two) {
	stream_recv_two(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_send) {
	stream_recv_send(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_send_sendback) {
	allow_send_back = true;
	stream_recv_send(arg);
	return;
}

/* TCP Quota */

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_one_quota) {
	atomic_store(&check_listener_quota, true);
	stream_recv_one(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_two_quota) {
	atomic_store(&check_listener_quota, true);
	stream_recv_two(arg);
	return;
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_send_quota) {
	atomic_store(&check_listener_quota, true);
	stream_recv_send(arg);
}

ISC_LOOP_TEST_IMPL(proxystreamtls_recv_send_quota_sendback) {
	atomic_store(&check_listener_quota, true);
	allow_send_back = true;
	stream_recv_send(arg);
}

ISC_TEST_LIST_START

/* Stream */
ISC_TEST_ENTRY_CUSTOM(proxystream_noop, proxystream_noop_setup,
		      proxystream_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_noresponse, proxystream_noresponse_setup,
		      proxystream_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_shutdownconnect,
		      proxystream_shutdownconnect_setup,
		      proxystream_shutdownconnect_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_shutdownread, proxystream_shutdownread_setup,
		      proxystream_shutdownread_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_timeout_recovery,
		      proxystream_timeout_recovery_setup,
		      proxystream_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_one, proxystream_recv_one_setup,
		      proxystream_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_one_prerendered,
		      proxystream_recv_one_setup, proxystream_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_two, proxystream_recv_two_setup,
		      proxystream_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_send, proxystream_recv_send_setup,
		      proxystream_recv_send_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_send_sendback,
		      proxystream_recv_send_setup,
		      proxystream_recv_send_teardown)

/* Stream Quota */
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_one_quota, proxystream_recv_one_setup,
		      proxystream_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_two_quota, proxystream_recv_two_setup,
		      proxystream_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_send_quota, proxystream_recv_send_setup,
		      proxystream_recv_send_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystream_recv_send_quota_sendback,
		      proxystream_recv_send_setup,
		      proxystream_recv_send_teardown)

/* PROXY over TLS */

/* Stream */
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_noop, proxystreamtls_noop_setup,
		      proxystreamtls_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_noresponse,
		      proxystreamtls_noresponse_setup,
		      proxystreamtls_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_shutdownconnect,
		      proxystreamtls_shutdownconnect_setup,
		      proxystreamtls_shutdownconnect_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_shutdownread,
		      proxystreamtls_shutdownread_setup,
		      proxystreamtls_shutdownread_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_timeout_recovery,
		      proxystreamtls_timeout_recovery_setup,
		      proxystreamtls_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_one, proxystreamtls_recv_one_setup,
		      proxystreamtls_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_one_prerendered,
		      proxystreamtls_recv_one_setup,
		      proxystreamtls_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_two, proxystreamtls_recv_two_setup,
		      proxystreamtls_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_send, proxystreamtls_recv_send_setup,
		      proxystreamtls_recv_send_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_send_sendback,
		      proxystreamtls_recv_send_setup,
		      proxystreamtls_recv_send_teardown)

/* Stream Quota */
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_one_quota,
		      proxystreamtls_recv_one_setup,
		      proxystreamtls_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_two_quota,
		      proxystreamtls_recv_two_setup,
		      proxystreamtls_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_send_quota,
		      proxystreamtls_recv_send_setup,
		      proxystreamtls_recv_send_teardown)
ISC_TEST_ENTRY_CUSTOM(proxystreamtls_recv_send_quota_sendback,
		      proxystreamtls_recv_send_setup,
		      proxystreamtls_recv_send_teardown)

ISC_TEST_LIST_END

static int
proxystream_setup(void **state ISC_ATTR_UNUSED) {
	stream_port = PROXYSTREAM_TEST_PORT;
	stream_use_TLS = false;
	stream = true;

	return 0;
}

ISC_TEST_MAIN_CUSTOM(proxystream_setup, NULL)
