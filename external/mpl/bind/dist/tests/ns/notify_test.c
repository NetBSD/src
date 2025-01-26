/*	$NetBSD: notify_test.c,v 1.3 2025/01/26 16:25:51 christos Exp $	*/

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
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/thread.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/rcode.h>
#include <dns/view.h>

#include <ns/client.h>
#include <ns/notify.h>

#include <tests/ns.h>

static void
check_response(isc_buffer_t *buf) {
	isc_result_t result;
	dns_message_t *message = NULL;
	char rcodebuf[20];
	isc_buffer_t b;

	dns_message_create(mctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE, &message);

	result = dns_message_parse(message, buf, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&b, rcodebuf, sizeof(rcodebuf));
	result = dns_rcode_totext(message->rcode, &b);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(message->rcode, dns_rcode_noerror);

	dns_message_detach(&message);
}

/* test ns_notify_start() */
ISC_LOOP_TEST_IMPL(notify_start) {
	isc_result_t result;
	ns_client_t *client = NULL;
	isc_nmhandle_t *handle = NULL;
	dns_message_t *nmsg = NULL;
	unsigned char ndata[4096];
	isc_buffer_t nbuf;
	size_t nsize;

	ns_test_getclient(NULL, false, &client);

	result = dns_test_makeview("view", false, false, &client->view);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = ns_test_serve_zone("example.com",
				    TESTS_DIR "/testdata/notify/zone1.db",
				    client->view);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Create a NOTIFY message by parsing a file in testdata.
	 * (XXX: use better message mocking method when available.)
	 */

	result = ns_test_getdata(TESTS_DIR "/testdata/notify/notify1.msg",
				 ndata, sizeof(ndata), &nsize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&nbuf, ndata, nsize);
	isc_buffer_add(&nbuf, nsize);

	dns_message_create(mctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE, &nmsg);

	result = dns_message_parse(nmsg, &nbuf, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Set up client object with this message and test the NOTIFY
	 * handler.
	 */
	if (client->message != NULL) {
		dns_message_detach(&client->message);
	}
	client->message = nmsg;
	nmsg = NULL;
	client->sendcb = check_response;
	ns_notify_start(client, client->handle);

	/*
	 * Clean up
	 */
	ns_test_cleanup_zone();

	handle = client->handle;
	isc_nmhandle_detach(&client->handle);
	isc_nmhandle_detach(&handle);

	isc_loop_teardown(mainloop, shutdown_interfacemgr, NULL);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(notify_start, setup_server, teardown_server)
ISC_TEST_LIST_END

ISC_TEST_MAIN
