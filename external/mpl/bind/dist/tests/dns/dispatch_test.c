/*	$NetBSD: dispatch_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/managers.h>
#include <isc/refcount.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/view.h>

#include <tests/dns.h>

uv_sem_t sem;

/* Timeouts in miliseconds */
#define T_SERVER_INIT	    5000
#define T_SERVER_IDLE	    5000
#define T_SERVER_KEEPALIVE  5000
#define T_SERVER_ADVERTISED 5000

#define T_CLIENT_INIT	    2000
#define T_CLIENT_IDLE	    2000
#define T_CLIENT_KEEPALIVE  2000
#define T_CLIENT_ADVERTISED 2000

#define T_CLIENT_CONNECT 1000

dns_dispatchmgr_t *dispatchmgr = NULL;
dns_dispatchset_t *dset = NULL;
isc_nm_t *connect_nm = NULL;
static isc_sockaddr_t udp_server_addr;
static isc_sockaddr_t udp_connect_addr;
static isc_sockaddr_t tcp_server_addr;
static isc_sockaddr_t tcp_connect_addr;

const struct in6_addr in6addr_blackhole = { { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 1 } } };

static int
setup_ephemeral_port(isc_sockaddr_t *addr, sa_family_t family) {
	socklen_t addrlen = sizeof(*addr);
	uv_os_sock_t fd;
	int r;

	isc_sockaddr_fromin6(addr, &in6addr_loopback, 0);

	fd = socket(AF_INET6, family, 0);
	if (fd < 0) {
		perror("setup_ephemeral_port: socket()");
		return (-1);
	}

	r = bind(fd, (const struct sockaddr *)&addr->type.sa,
		 sizeof(addr->type.sin6));
	if (r != 0) {
		perror("setup_ephemeral_port: bind()");
		close(fd);
		return (r);
	}

	r = getsockname(fd, (struct sockaddr *)&addr->type.sa, &addrlen);
	if (r != 0) {
		perror("setup_ephemeral_port: getsockname()");
		close(fd);
		return (r);
	}

#if IPV6_RECVERR
#define setsockopt_on(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 1 }, sizeof(int))

	r = setsockopt_on(fd, IPPROTO_IPV6, IPV6_RECVERR);
	if (r != 0) {
		perror("setup_ephemeral_port");
		close(fd);
		return (r);
	}
#endif

	return (fd);
}

static void
reset_testdata(void);

static int
_setup(void **state) {
	uv_os_sock_t sock = -1;
	int r;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_loopback, 0);

	udp_server_addr = (isc_sockaddr_t){ .length = 0 };
	sock = setup_ephemeral_port(&udp_server_addr, SOCK_DGRAM);
	if (sock < 0) {
		return (-1);
	}
	close(sock);

	tcp_server_addr = (isc_sockaddr_t){ .length = 0 };
	sock = setup_ephemeral_port(&tcp_server_addr, SOCK_STREAM);
	if (sock < 0) {
		return (-1);
	}
	close(sock);

	setup_managers(state);

	/* Create a secondary network manager */
	isc_managers_create(mctx, workers, 0, &connect_nm, NULL, NULL);

	isc_nm_settimeouts(netmgr, T_SERVER_INIT, T_SERVER_IDLE,
			   T_SERVER_KEEPALIVE, T_SERVER_ADVERTISED);

	/*
	 * Use shorter client-side timeouts, to ensure that clients
	 * time out before the server.
	 */
	isc_nm_settimeouts(connect_nm, T_CLIENT_INIT, T_CLIENT_IDLE,
			   T_CLIENT_KEEPALIVE, T_CLIENT_ADVERTISED);

	r = uv_sem_init(&sem, 0);
	assert_int_equal(r, 0);

	reset_testdata();

	return (0);
}

static int
_teardown(void **state) {
	uv_sem_destroy(&sem);

	isc_managers_destroy(&connect_nm, NULL, NULL);
	assert_null(connect_nm);

	teardown_managers(state);

	return (0);
}

static isc_result_t
make_dispatchset(unsigned int ndisps) {
	isc_result_t result;
	isc_sockaddr_t any;
	dns_dispatch_t *disp = NULL;

	result = dns_dispatchmgr_create(mctx, netmgr, &dispatchmgr);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	isc_sockaddr_any(&any);
	result = dns_dispatch_createudp(dispatchmgr, &any, &disp);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_dispatchset_create(mctx, disp, &dset, ndisps);
	dns_dispatch_detach(&disp);

	return (result);
}

static void
reset(void) {
	if (dset != NULL) {
		dns_dispatchset_destroy(&dset);
	}
	if (dispatchmgr != NULL) {
		dns_dispatchmgr_detach(&dispatchmgr);
	}
}

/* create dispatch set */
ISC_RUN_TEST_IMPL(dispatchset_create) {
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
ISC_RUN_TEST_IMPL(dispatchset_get) {
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

struct {
	atomic_uint_fast32_t responses;
	atomic_uint_fast32_t result;
} testdata;

static dns_dispatch_t *dispatch = NULL;
static dns_dispentry_t *dispentry = NULL;
static atomic_bool first = true;

static void
reset_testdata(void) {
	atomic_init(&testdata.responses, 0);
	atomic_init(&testdata.result, ISC_R_UNSET);
}

static void
server_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(cbarg);

	return;
}

static void
nameserver(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	   void *cbarg) {
	isc_region_t response;
	static unsigned char buf1[16];
	static unsigned char buf2[16];

	UNUSED(eresult);
	UNUSED(cbarg);

	memmove(buf1, region->base, 12);
	memset(buf1 + 12, 0, 4);
	buf1[2] |= 0x80; /* qr=1 */

	memmove(buf2, region->base, 12);
	memset(buf2 + 12, 1, 4);
	buf2[2] |= 0x80; /* qr=1 */

	/*
	 * send message to be discarded.
	 */
	response.base = buf1;
	response.length = sizeof(buf1);
	isc_nm_send(handle, &response, server_senddone, NULL);

	/*
	 * send nextitem message.
	 */
	response.base = buf2;
	response.length = sizeof(buf2);
	isc_nm_send(handle, &response, server_senddone, NULL);
}

static isc_result_t
accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	return (eresult);
}

static void
noop_nameserver(isc_nmhandle_t *handle, isc_result_t eresult,
		isc_region_t *region, void *cbarg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(cbarg);
}

static void
response_getnext(isc_result_t result, isc_region_t *region, void *arg) {
	UNUSED(region);
	UNUSED(arg);

	atomic_fetch_add_relaxed(&testdata.responses, 1);

	if (atomic_compare_exchange_strong(&first, &(bool){ true }, false)) {
		result = dns_dispatch_getnext(dispentry);
		assert_int_equal(result, ISC_R_SUCCESS);
	} else {
		uv_sem_post(&sem);
	}
}

static void
response(isc_result_t eresult, isc_region_t *region, void *arg) {
	UNUSED(region);
	UNUSED(arg);

	switch (eresult) {
	case ISC_R_EOF:
	case ISC_R_CANCELED:
	case ISC_R_SHUTTINGDOWN:
		break;
	default:
		atomic_fetch_add_relaxed(&testdata.responses, 1);
		atomic_store_relaxed(&testdata.result, eresult);
	}

	uv_sem_post(&sem);
}

static void
response_timeout(isc_result_t eresult, isc_region_t *region, void *arg) {
	UNUSED(region);
	UNUSED(arg);

	atomic_store_relaxed(&testdata.result, eresult);

	uv_sem_post(&sem);
}

static void
connected(isc_result_t eresult, isc_region_t *region, void *cbarg) {
	isc_region_t *r = (isc_region_t *)cbarg;

	UNUSED(eresult);
	UNUSED(region);

	dns_dispatch_send(dispentry, r);
}

static void
client_senddone(isc_result_t eresult, isc_region_t *region, void *cbarg) {
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(cbarg);

	return;
}

static void
timeout_connected(isc_result_t eresult, isc_region_t *region, void *cbarg) {
	UNUSED(region);
	UNUSED(cbarg);

	atomic_store_relaxed(&testdata.result, eresult);

	uv_sem_post(&sem);
}

ISC_RUN_TEST_IMPL(dispatch_timeout_tcp_connect) {
	isc_result_t result;
	isc_region_t region;
	unsigned char rbuf[12] = { 0 };
	unsigned char message[12] = { 0 };
	uint16_t id;

	UNUSED(state);

	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_blackhole, 0);

	result = dns_dispatchmgr_create(mctx, connect_nm, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	region.base = rbuf;
	region.length = sizeof(rbuf);

	result = dns_dispatch_add(dispatch, 0, T_CLIENT_CONNECT,
				  &tcp_server_addr, timeout_connected,
				  client_senddone, response, &region, &id,
				  &dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);

	dns_dispatch_connect(dispentry);

	uv_sem_wait(&sem);

	dns_dispatch_done(&dispentry);

	dns_dispatch_detach(&dispatch);
	dns_dispatchmgr_detach(&dispatchmgr);

	/* Skip if the IPv6 is not available or not blackholed */

	result = atomic_load_acquire(&testdata.result);
	if (result == ISC_R_ADDRNOTAVAIL || result == ISC_R_CONNREFUSED) {
		skip();
		return;
	}

	assert_int_equal(result, ISC_R_TIMEDOUT);
}

ISC_RUN_TEST_IMPL(dispatch_timeout_tcp_response) {
	isc_result_t result;
	isc_region_t region;
	unsigned char rbuf[12] = { 0 };
	unsigned char message[12] = { 0 };
	uint16_t id;
	isc_nmsocket_t *sock = NULL;

	UNUSED(state);

	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_loopback, 0);

	result = dns_dispatchmgr_create(mctx, connect_nm, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_nm_listentcpdns(netmgr, &tcp_server_addr, noop_nameserver,
				     NULL, accept_cb, NULL, 0, 0, NULL, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	region.base = rbuf;
	region.length = sizeof(rbuf);

	result = dns_dispatch_add(dispatch, 0, T_CLIENT_CONNECT,
				  &tcp_server_addr, connected, client_senddone,
				  response_timeout, &region, &id, &dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);

	dns_dispatch_connect(dispentry);

	uv_sem_wait(&sem);

	assert_int_equal(atomic_load_acquire(&testdata.result), ISC_R_TIMEDOUT);

	isc_nm_stoplistening(sock);
	isc_nmsocket_close(&sock);
	assert_null(sock);

	dns_dispatch_done(&dispentry);

	dns_dispatch_detach(&dispatch);
	dns_dispatchmgr_detach(&dispatchmgr);
}

ISC_RUN_TEST_IMPL(dispatch_tcp_response) {
	isc_result_t result;
	isc_region_t region;
	unsigned char rbuf[12] = { 0 };
	unsigned char message[12] = { 0 };
	uint16_t id;
	isc_nmsocket_t *sock = NULL;

	UNUSED(state);

	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_loopback, 0);

	result = dns_dispatchmgr_create(mctx, connect_nm, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_nm_listentcpdns(netmgr, &tcp_server_addr, nameserver, NULL,
				     accept_cb, NULL, 0, 0, NULL, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	region.base = rbuf;
	region.length = sizeof(rbuf);

	result = dns_dispatch_add(dispatch, 0, T_CLIENT_CONNECT,
				  &tcp_server_addr, connected, client_senddone,
				  response, &region, &id, &dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);

	dns_dispatch_connect(dispentry);

	uv_sem_wait(&sem);

	assert_in_range(atomic_load_acquire(&testdata.responses), 1, 2);
	assert_int_equal(atomic_load_acquire(&testdata.result), ISC_R_SUCCESS);

	/* Cleanup */

	isc_nm_stoplistening(sock);
	isc_nmsocket_close(&sock);
	assert_null(sock);

	dns_dispatch_done(&dispentry);

	dns_dispatch_detach(&dispatch);
	dns_dispatchmgr_detach(&dispatchmgr);
}

ISC_RUN_TEST_IMPL(dispatch_timeout_udp_response) {
	isc_result_t result;
	isc_region_t region;
	unsigned char rbuf[12] = { 0 };
	unsigned char message[12] = { 0 };
	uint16_t id;
	isc_nmsocket_t *sock = NULL;

	UNUSED(state);

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	result = dns_dispatchmgr_create(mctx, connect_nm, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createudp(dispatchmgr, &tcp_connect_addr,
					&dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_nm_listenudp(netmgr, &udp_server_addr, noop_nameserver,
				  NULL, 0, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	region.base = rbuf;
	region.length = sizeof(rbuf);

	result = dns_dispatch_add(dispatch, 0, T_CLIENT_CONNECT,
				  &udp_server_addr, connected, client_senddone,
				  response_timeout, &region, &id, &dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);

	dns_dispatch_connect(dispentry);

	uv_sem_wait(&sem);

	assert_int_equal(atomic_load_acquire(&testdata.result), ISC_R_TIMEDOUT);

	isc_nm_stoplistening(sock);
	isc_nmsocket_close(&sock);
	assert_null(sock);

	dns_dispatch_done(&dispentry);

	dns_dispatch_detach(&dispatch);
	dns_dispatchmgr_detach(&dispatchmgr);
}

/* test dispatch getnext */
ISC_RUN_TEST_IMPL(dispatch_getnext) {
	isc_result_t result;
	isc_region_t region;
	isc_nmsocket_t *sock = NULL;
	unsigned char message[12] = { 0 };
	unsigned char rbuf[12] = { 0 };
	uint16_t id;

	UNUSED(state);

	result = dns_dispatchmgr_create(mctx, connect_nm, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createudp(dispatchmgr, &udp_connect_addr,
					&dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Create a local udp nameserver on the loopback.
	 */
	result = isc_nm_listenudp(netmgr, &udp_server_addr, nameserver, NULL, 0,
				  &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	region.base = rbuf;
	region.length = sizeof(rbuf);
	result = dns_dispatch_add(dispatch, 0, T_CLIENT_CONNECT,
				  &udp_server_addr, connected, client_senddone,
				  response_getnext, &region, &id, &dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(message, 0, sizeof(message));
	message[0] = (id >> 8) & 0xff;
	message[1] = id & 0xff;

	region.base = message;
	region.length = sizeof(message);

	dns_dispatch_connect(dispentry);

	uv_sem_wait(&sem);

	assert_int_equal(atomic_load_acquire(&testdata.responses), 2);

	/* Cleanup */
	isc_nm_stoplistening(sock);
	isc_nmsocket_close(&sock);
	assert_null(sock);

	dns_dispatch_done(&dispentry);
	dns_dispatch_detach(&dispatch);
	dns_dispatchmgr_detach(&dispatchmgr);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(dispatch_timeout_tcp_connect, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dispatch_timeout_tcp_response, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dispatch_tcp_response, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dispatch_timeout_udp_response, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dispatchset_create, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dispatchset_get, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dispatch_getnext, _setup, _teardown)

ISC_TEST_LIST_END

ISC_TEST_MAIN
