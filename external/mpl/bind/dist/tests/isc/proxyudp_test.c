/*	$NetBSD: proxyudp_test.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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

static isc_nm_proxyheader_info_t custom_info;

char complete_proxy_data[] = { 0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x0d, 0x0a,
			       0x51, 0x55, 0x49, 0x54, 0x0a, 0x21, 0x12,
			       0x00, 0x0c, 0x01, 0x02, 0x03, 0x04, 0x04,
			       0x03, 0x02, 0x01, 0x14, 0xe9, 0x14, 0xe9 };

ISC_LOOP_TEST_IMPL(proxyudp_noop) { udp_noop(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_noresponse) { udp_noresponse(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_timeout_recovery) { udp_timeout_recovery(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_shutdown_connect) { udp_shutdown_connect(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_shutdown_read) { udp_shutdown_read(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_cancel_read) { udp_cancel_read(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_recv_one) { udp_recv_one(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_recv_one_prerendered) {
	isc_region_t header = { 0 };
	header.base = (unsigned char *)complete_proxy_data;
	header.length = sizeof(complete_proxy_data);

	isc_nm_proxyheader_info_init_complete(&custom_info, &header);

	set_proxyheader_info(&custom_info);

	udp_recv_one(arg);
}

ISC_LOOP_TEST_IMPL(proxyudp_recv_two) { udp_recv_two(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_recv_send) { udp_recv_send(arg); }

ISC_LOOP_TEST_IMPL(proxyudp_double_read) { udp_double_read(arg); }

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(proxyudp_noop, proxyudp_noop_setup,
		      proxyudp_noop_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_noresponse, proxyudp_noresponse_setup,
		      proxyudp_noresponse_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_timeout_recovery,
		      proxyudp_timeout_recovery_setup,
		      proxyudp_timeout_recovery_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_shutdown_read, proxyudp_shutdown_read_setup,
		      proxyudp_shutdown_read_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_cancel_read, proxyudp_cancel_read_setup,
		      proxyudp_cancel_read_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_shutdown_connect,
		      proxyudp_shutdown_connect_setup,
		      proxyudp_shutdown_connect_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_double_read, proxyudp_double_read_setup,
		      proxyudp_double_read_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_recv_one, proxyudp_recv_one_setup,
		      proxyudp_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_recv_one_prerendered, proxyudp_recv_one_setup,
		      proxyudp_recv_one_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_recv_two, proxyudp_recv_two_setup,
		      proxyudp_recv_two_teardown)
ISC_TEST_ENTRY_CUSTOM(proxyudp_recv_send, proxyudp_recv_send_setup,
		      proxyudp_recv_send_teardown)

ISC_TEST_LIST_END

ISC_TEST_MAIN
