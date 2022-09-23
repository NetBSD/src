/*	$NetBSD: notify_test.c,v 1.9 2022/09/23 12:15:36 christos Exp $	*/

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

#include <isc/util.h>

#if HAVE_CMOCKA

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

#include <isc/event.h>
#include <isc/print.h>
#include <isc/task.h>

#include <dns/acl.h>
#include <dns/rcode.h>
#include <dns/view.h>

#include <ns/client.h>
#include <ns/notify.h>

#include "nstest.h"

#if defined(USE_LIBTOOL) || LD_WRAP
static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = ns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	ns_test_end();

	return (0);
}

static void
check_response(isc_buffer_t *buf) {
	isc_result_t result;
	dns_message_t *message = NULL;
	char rcodebuf[20];
	isc_buffer_t b;

	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &message);

	result = dns_message_parse(message, buf, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&b, rcodebuf, sizeof(rcodebuf));
	result = dns_rcode_totext(message->rcode, &b);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(message->rcode, dns_rcode_noerror);

	dns_message_detach(&message);
}

/* test ns_notify_start() */
static void
notify_start(void **state) {
	isc_result_t result;
	ns_client_t *client = NULL;
	isc_nmhandle_t *handle = NULL;
	dns_message_t *nmsg = NULL;
	unsigned char ndata[4096];
	isc_buffer_t nbuf;
	size_t nsize;

	UNUSED(state);

	result = ns_test_getclient(NULL, false, &client);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = ns_test_makeview("view", false, &client->view);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = ns_test_serve_zone("example.com", "testdata/notify/zone1.db",
				    client->view);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Create a NOTIFY message by parsing a file in testdata.
	 * (XXX: use better message mocking method when available.)
	 */

	result = ns_test_getdata("testdata/notify/notify1.msg", ndata,
				 sizeof(ndata), &nsize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&nbuf, ndata, nsize);
	isc_buffer_add(&nbuf, nsize);

	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &nmsg);

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
}
#endif /* if defined(USE_LIBTOOL) || LD_WRAP */

int
main(void) {
#if defined(USE_LIBTOOL) || LD_WRAP
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(notify_start, _setup,
						_teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
#else  /* if defined(USE_LIBTOOL) || LD_WRAP */
	print_message("1..0 # Skip notify_test requires libtool or LD_WRAP\n");
#endif /* if defined(USE_LIBTOOL) || LD_WRAP */
}
#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");

	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* HAVE_CMOCKA */
