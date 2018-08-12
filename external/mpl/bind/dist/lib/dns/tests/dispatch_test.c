/*	$NetBSD: dispatch_test.c,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/view.h>

#include "dnstest.h"

dns_dispatchmgr_t *dispatchmgr = NULL;
dns_dispatchset_t *dset = NULL;

static isc_result_t
make_dispatchset(unsigned int ndisps) {
	isc_result_t result;
	isc_sockaddr_t any;
	unsigned int attrs;
	dns_dispatch_t *disp = NULL;

	result = dns_dispatchmgr_create(mctx, NULL, &dispatchmgr);
	if (result != ISC_R_SUCCESS)
		return (result);

	isc_sockaddr_any(&any);
	attrs = DNS_DISPATCHATTR_IPV4 | DNS_DISPATCHATTR_UDP;
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr,
				     &any, 512, 6, 1024, 17, 19, attrs,
				     attrs, &disp);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_dispatchset_create(mctx, socketmgr, taskmgr, disp,
					&dset, ndisps);
	dns_dispatch_detach(&disp);

	return (result);
}

static void
teardown(void) {
	if (dset != NULL)
		dns_dispatchset_destroy(&dset);
	if (dispatchmgr != NULL)
		dns_dispatchmgr_destroy(&dispatchmgr);
}

/*
 * Individual unit tests
 */
ATF_TC(dispatchset_create);
ATF_TC_HEAD(dispatchset_create, tc) {
	atf_tc_set_md_var(tc, "descr", "create dispatch set");
}
ATF_TC_BODY(dispatchset_create, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = make_dispatchset(1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	teardown();

	result = make_dispatchset(10);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	teardown();

	dns_test_end();
}

ATF_TC(dispatchset_get);
ATF_TC_HEAD(dispatchset_get, tc) {
	atf_tc_set_md_var(tc, "descr", "test dispatch set round-robin");
}
ATF_TC_BODY(dispatchset_get, tc) {
	isc_result_t result;
	dns_dispatch_t *d1, *d2, *d3, *d4, *d5;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = make_dispatchset(1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	d1 = dns_dispatchset_get(dset);
	d2 = dns_dispatchset_get(dset);
	d3 = dns_dispatchset_get(dset);
	d4 = dns_dispatchset_get(dset);
	d5 = dns_dispatchset_get(dset);

	ATF_CHECK_EQ(d1, d2);
	ATF_CHECK_EQ(d2, d3);
	ATF_CHECK_EQ(d3, d4);
	ATF_CHECK_EQ(d4, d5);

	teardown();

	result = make_dispatchset(4);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	d1 = dns_dispatchset_get(dset);
	d2 = dns_dispatchset_get(dset);
	d3 = dns_dispatchset_get(dset);
	d4 = dns_dispatchset_get(dset);
	d5 = dns_dispatchset_get(dset);

	ATF_CHECK_EQ(d1, d5);
	ATF_CHECK(d1 != d2);
	ATF_CHECK(d2 != d3);
	ATF_CHECK(d3 != d4);
	ATF_CHECK(d4 != d5);

	teardown();
	dns_test_end();
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
	buf1[2] |= 0x80;	/* qr=1 */

	memmove(buf2, ev->region.base, 12);
	memset(buf2 + 12, 1, 4);
	buf2[2] |= 0x80;	/* qr=1 */

	/*
	 * send message to be discarded.
	 */
	region.base = buf1;
	region.length = sizeof(buf1);
	dummy = NULL;
	isc_socket_attach(sock, &dummy);
	result = isc_socket_sendto(sock, &region, task, senddone, sock,
				   &ev->address, NULL);
	if (result != ISC_R_SUCCESS)
		isc_socket_detach(&dummy);

	/*
	 * send nextitem message.
	 */
	region.base = buf2;
	region.length = sizeof(buf2);
	dummy = NULL;
	isc_socket_attach(sock, &dummy);
	result = isc_socket_sendto(sock, &region, task, senddone, sock,
				   &ev->address, NULL);
	if (result != ISC_R_SUCCESS)
		isc_socket_detach(&dummy);
	isc_event_free(&event);
}

static dns_dispatch_t *dispatch = NULL;
static dns_dispentry_t *dispentry = NULL;
static isc_boolean_t first = ISC_TRUE;
static isc_mutex_t lock;
static isc_sockaddr_t local;
static unsigned int responses = 0;

static void
response(isc_task_t *task, isc_event_t *event) {
	dns_dispatchevent_t *devent = (dns_dispatchevent_t *)event;
	isc_result_t result;
	isc_boolean_t wasfirst;

	UNUSED(task);

	LOCK(&lock);
	wasfirst = first;
	first = ISC_FALSE;
	responses++;
	UNLOCK(&lock);

	if (wasfirst) {
		result = dns_dispatch_getnext(dispentry, &devent);
		ATF_CHECK_EQ(result, ISC_R_SUCCESS);
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
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_event_free(&event);
}

ATF_TC(dispatch_getnext);
ATF_TC_HEAD(dispatch_getnext, tc) {
	atf_tc_set_md_var(tc, "descr", "test dispatch getnext");
}
ATF_TC_BODY(dispatch_getnext, tc) {
	isc_region_t region;
	isc_result_t result;
	isc_socket_t *sock = NULL;
	isc_task_t *task = NULL;
	isc_uint16_t id;
	struct in_addr ina;
	unsigned char message[12];
	unsigned int attrs;
	unsigned char rbuf[12];

	UNUSED(tc);

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_dispatchmgr_create(mctx, NULL, &dispatchmgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ina.s_addr = htonl(INADDR_LOOPBACK);
	isc_sockaddr_fromin(&local, &ina, 0);
	attrs = DNS_DISPATCHATTR_IPV4 | DNS_DISPATCHATTR_UDP;
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr,
				     &local, 512, 6, 1024, 17, 19, attrs,
				     attrs, &dispatch);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Create a local udp nameserver on the loopback.
	 */
	result = isc_socket_create(socketmgr, AF_INET, isc_sockettype_udp,
				   &sock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ina.s_addr = htonl(INADDR_LOOPBACK);
	isc_sockaddr_fromin(&local, &ina, 0);
	result = isc_socket_bind(sock, &local, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_socket_getsockname(sock, &local);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	first = ISC_TRUE;
	region.base = rbuf;
	region.length = sizeof(rbuf);
	result = isc_socket_recv(sock, &region, 1, task, nameserver, sock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_dispatch_addresponse(dispatch, &local, task, response,
					  NULL, &id, &dispentry);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);
	result = isc_app_onrun(mctx, task, startit, &region);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_app_run();
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_CHECK_EQ(responses, 2);

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

	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, dispatchset_create);
	ATF_TP_ADD_TC(tp, dispatchset_get);
	ATF_TP_ADD_TC(tp, dispatch_getnext);
	return (atf_no_error());
}
