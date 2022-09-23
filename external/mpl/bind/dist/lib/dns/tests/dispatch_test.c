/*	$NetBSD: dispatch_test.c,v 1.9 2022/09/23 12:15:32 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/refcount.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/view.h>

#include "dnstest.h"

dns_dispatchmgr_t *dispatchmgr = NULL;
dns_dispatchset_t *dset = NULL;

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

static isc_result_t
make_dispatchset(unsigned int ndisps) {
	isc_result_t result;
	isc_sockaddr_t any;
	unsigned int attrs;
	dns_dispatch_t *disp = NULL;

	result = dns_dispatchmgr_create(dt_mctx, &dispatchmgr);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	isc_sockaddr_any(&any);
	attrs = DNS_DISPATCHATTR_IPV4 | DNS_DISPATCHATTR_UDP;
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr, &any, 512,
				     6, 1024, 17, 19, attrs, attrs, &disp);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_dispatchset_create(dt_mctx, socketmgr, taskmgr, disp,
					&dset, ndisps);
	dns_dispatch_detach(&disp);

	return (result);
}

static void
reset(void) {
	if (dset != NULL) {
		dns_dispatchset_destroy(&dset);
	}
	if (dispatchmgr != NULL) {
		dns_dispatchmgr_destroy(&dispatchmgr);
	}
}

/* create dispatch set */
static void
dispatchset_create(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = make_dispatchset(1);
	assert_int_equal(result, ISC_R_SUCCESS);
	reset();

	result = make_dispatchset(10);
	assert_int_equal(result, ISC_R_SUCCESS);
	reset();
}

/* test dispatch set round-robin */
static void
dispatchset_get(void **state) {
	isc_result_t result;
	dns_dispatch_t *d1, *d2, *d3, *d4, *d5;

	UNUSED(state);

	result = make_dispatchset(1);
	assert_int_equal(result, ISC_R_SUCCESS);

	d1 = dns_dispatchset_get(dset);
	d2 = dns_dispatchset_get(dset);
	d3 = dns_dispatchset_get(dset);
	d4 = dns_dispatchset_get(dset);
	d5 = dns_dispatchset_get(dset);

	assert_ptr_equal(d1, d2);
	assert_ptr_equal(d2, d3);
	assert_ptr_equal(d3, d4);
	assert_ptr_equal(d4, d5);

	reset();

	result = make_dispatchset(4);
	assert_int_equal(result, ISC_R_SUCCESS);

	d1 = dns_dispatchset_get(dset);
	d2 = dns_dispatchset_get(dset);
	d3 = dns_dispatchset_get(dset);
	d4 = dns_dispatchset_get(dset);
	d5 = dns_dispatchset_get(dset);

	assert_ptr_equal(d1, d5);
	assert_ptr_not_equal(d1, d2);
	assert_ptr_not_equal(d2, d3);
	assert_ptr_not_equal(d3, d4);
	assert_ptr_not_equal(d4, d5);

	reset();
}

static void
senddone(isc_task_t *task, isc_event_t *event) {
	isc_socket_t *sock = event->ev_arg;

	UNUSED(task);

	isc_socket_detach(&sock);
	isc_event_free(&event);
}

static void
nameserver(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_region_t region;
	isc_socket_t *dummy;
	isc_socket_t *sock = event->ev_arg;
	isc_socketevent_t *ev = (isc_socketevent_t *)event;
	static unsigned char buf1[16];
	static unsigned char buf2[16];

	memmove(buf1, ev->region.base, 12);
	memset(buf1 + 12, 0, 4);
	buf1[2] |= 0x80; /* qr=1 */

	memmove(buf2, ev->region.base, 12);
	memset(buf2 + 12, 1, 4);
	buf2[2] |= 0x80; /* qr=1 */

	/*
	 * send message to be discarded.
	 */
	region.base = buf1;
	region.length = sizeof(buf1);
	dummy = NULL;
	isc_socket_attach(sock, &dummy);
	result = isc_socket_sendto(sock, &region, task, senddone, sock,
				   &ev->address, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_socket_detach(&dummy);
	}

	/*
	 * send nextitem message.
	 */
	region.base = buf2;
	region.length = sizeof(buf2);
	dummy = NULL;
	isc_socket_attach(sock, &dummy);
	result = isc_socket_sendto(sock, &region, task, senddone, sock,
				   &ev->address, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_socket_detach(&dummy);
	}
	isc_event_free(&event);
}

static dns_dispatch_t *dispatch = NULL;
static dns_dispentry_t *dispentry = NULL;
static atomic_bool first = true;
static isc_sockaddr_t local;
static atomic_uint_fast32_t responses;

static void
response(isc_task_t *task, isc_event_t *event) {
	dns_dispatchevent_t *devent = (dns_dispatchevent_t *)event;
	bool exp_true = true;

	UNUSED(task);

	atomic_fetch_add_relaxed(&responses, 1);
	if (atomic_compare_exchange_strong(&first, &exp_true, false)) {
		isc_result_t result = dns_dispatch_getnext(dispentry, &devent);
		assert_int_equal(result, ISC_R_SUCCESS);
	} else {
		dns_dispatch_removeresponse(&dispentry, &devent);
		isc_app_shutdown();
	}
}

static void
startit(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_socket_t *sock = NULL;

	isc_socket_attach(dns_dispatch_getsocket(dispatch), &sock);
	result = isc_socket_sendto(sock, event->ev_arg, task, senddone, sock,
				   &local, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_event_free(&event);
}

/* test dispatch getnext */
static void
dispatch_getnext(void **state) {
	isc_region_t region;
	isc_result_t result;
	isc_socket_t *sock = NULL;
	isc_task_t *task = NULL;
	uint16_t id;
	struct in_addr ina;
	unsigned char message[12];
	unsigned int attrs;
	unsigned char rbuf[12];

	UNUSED(state);

	atomic_init(&responses, 0);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatchmgr_create(dt_mctx, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	ina.s_addr = htonl(INADDR_LOOPBACK);
	isc_sockaddr_fromin(&local, &ina, 0);
	attrs = DNS_DISPATCHATTR_IPV4 | DNS_DISPATCHATTR_UDP;
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr, &local,
				     512, 6, 1024, 17, 19, attrs, attrs,
				     &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Create a local udp nameserver on the loopback.
	 */
	result = isc_socket_create(socketmgr, AF_INET, isc_sockettype_udp,
				   &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	ina.s_addr = htonl(INADDR_LOOPBACK);
	isc_sockaddr_fromin(&local, &ina, 0);
	result = isc_socket_bind(sock, &local, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_socket_getsockname(sock, &local);
	assert_int_equal(result, ISC_R_SUCCESS);

	region.base = rbuf;
	region.length = sizeof(rbuf);
	result = isc_socket_recv(sock, &region, 1, task, nameserver, sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_addresponse(dispatch, 0, &local, task, response,
					  NULL, &id, &dispentry, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);
	result = isc_app_onrun(dt_mctx, task, startit, &region);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_app_run();
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(atomic_load_acquire(&responses), 2);

	/*
	 * Shutdown nameserver.
	 */
	isc_socket_cancel(sock, task, ISC_SOCKCANCEL_RECV);
	isc_socket_detach(&sock);
	isc_task_detach(&task);

	/*
	 * Shutdown the dispatch.
	 */
	dns_dispatch_detach(&dispatch);
	dns_dispatchmgr_destroy(&dispatchmgr);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(dispatchset_create, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(dispatchset_get, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(dispatch_getnext, _setup,
						_teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
