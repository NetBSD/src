/*	$NetBSD: dispatch_test.c,v 1.3 2025/01/26 16:25:47 christos Exp $	*/

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

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/managers.h>
#include <isc/refcount.h>
#include <isc/tls.h>
#include <isc/util.h>
#include <isc/uv.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/view.h>

#include <tests/dns.h>

/* Timeouts in miliseconds */
#define T_SERVER_INIT	    (120 * 1000)
#define T_SERVER_IDLE	    (120 * 1000)
#define T_SERVER_KEEPALIVE  (120 * 1000)
#define T_SERVER_ADVERTISED (120 * 1000)

#define T_CLIENT_INIT	    (120 * 1000)
#define T_CLIENT_IDLE	    (120 * 1000)
#define T_CLIENT_KEEPALIVE  (120 * 1000)
#define T_CLIENT_ADVERTISED (120 * 1000)

#define T_CLIENT_CONNECT (30 * 1000)

/* For checks which are expected to timeout */
#define T_CLIENT_CONNECT_SHORT (10 * 1000)

/* dns_dispatchset_t *dset = NULL; */
static isc_sockaddr_t udp_server_addr;
static isc_sockaddr_t udp_connect_addr;
static isc_sockaddr_t tcp_server_addr;
static isc_sockaddr_t tcp_connect_addr;
static isc_sockaddr_t tls_server_addr;
static isc_sockaddr_t tls_connect_addr;

static isc_tlsctx_cache_t *tls_tlsctx_client_cache = NULL;
static isc_tlsctx_t *tls_listen_tlsctx = NULL;
static dns_name_t tls_name;
static const char *tls_name_str = "ephemeral";
static dns_transport_t *tls_transport = NULL;
static dns_transport_list_t *transport_list = NULL;

static isc_nmsocket_t *sock = NULL;

static isc_nm_t *connect_nm = NULL;

const struct in6_addr in6addr_blackhole = { { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 1 } } };

struct {
	uint8_t rbuf[12];
	isc_region_t region;
	uint8_t message[12];
} testdata;

typedef struct {
	dns_dispatchmgr_t *dispatchmgr;
	dns_dispatch_t *dispatch;
	dns_dispentry_t *dispentry;
	uint16_t id;
} test_dispatch_t;

static void
test_dispatch_done(test_dispatch_t *test) {
	dns_dispatch_done(&test->dispentry);
	dns_dispatch_detach(&test->dispatch);
	dns_dispatchmgr_detach(&test->dispatchmgr);
	isc_mem_put(mctx, test, sizeof(*test));
}

static void
test_dispatch_shutdown(test_dispatch_t *test) {
	test_dispatch_done(test);
	isc_loopmgr_shutdown(loopmgr);
}

static int
setup_ephemeral_port(isc_sockaddr_t *addr, sa_family_t family) {
	socklen_t addrlen = sizeof(*addr);
	uv_os_sock_t fd;
	int r;

	isc_sockaddr_fromin6(addr, &in6addr_loopback, 0);

	fd = socket(AF_INET6, family, 0);
	if (fd < 0) {
		perror("setup_ephemeral_port: socket()");
		return -1;
	}

	r = bind(fd, (const struct sockaddr *)&addr->type.sa,
		 sizeof(addr->type.sin6));
	if (r != 0) {
		perror("setup_ephemeral_port: bind()");
		close(fd);
		return r;
	}

	r = getsockname(fd, (struct sockaddr *)&addr->type.sa, &addrlen);
	if (r != 0) {
		perror("setup_ephemeral_port: getsockname()");
		close(fd);
		return r;
	}

#if IPV6_RECVERR
#define setsockopt_on(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 1 }, sizeof(int))

	r = setsockopt_on(fd, IPPROTO_IPV6, IPV6_RECVERR);
	if (r != 0) {
		perror("setup_ephemeral_port");
		close(fd);
		return r;
	}
#endif

	return fd;
}

static int
setup_test(void **state) {
	isc_buffer_t namesrc, namebuf;
	char namedata[DNS_NAME_FORMATSIZE + 1];

	uv_os_sock_t socket = -1;

	/* Create just 1 loop for this test */
	isc_loopmgr_create(mctx, 1, &loopmgr);
	mainloop = isc_loop_main(loopmgr);

	setup_netmgr(state);

	isc_netmgr_create(mctx, loopmgr, &connect_nm);

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_loopback, 0);

	tls_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tls_connect_addr, &in6addr_loopback, 0);

	udp_server_addr = (isc_sockaddr_t){ .length = 0 };
	socket = setup_ephemeral_port(&udp_server_addr, SOCK_DGRAM);
	if (socket < 0) {
		return -1;
	}
	close(socket);

	tcp_server_addr = (isc_sockaddr_t){ .length = 0 };
	socket = setup_ephemeral_port(&tcp_server_addr, SOCK_STREAM);
	if (socket < 0) {
		return -1;
	}
	close(socket);

	tls_server_addr = (isc_sockaddr_t){ .length = 0 };
	socket = setup_ephemeral_port(&tls_server_addr, SOCK_STREAM);
	if (socket < 0) {
		return -1;
	}
	close(socket);

	isc_nm_settimeouts(netmgr, T_SERVER_INIT, T_SERVER_IDLE,
			   T_SERVER_KEEPALIVE, T_SERVER_ADVERTISED);

	/*
	 * Use shorter client-side timeouts, to ensure that clients
	 * time out before the server.
	 */
	isc_nm_settimeouts(connect_nm, T_CLIENT_INIT, T_CLIENT_IDLE,
			   T_CLIENT_KEEPALIVE, T_CLIENT_ADVERTISED);

	memset(testdata.rbuf, 0, sizeof(testdata.rbuf));
	testdata.region.base = testdata.rbuf;
	testdata.region.length = sizeof(testdata.rbuf);
	memset(testdata.message, 0, sizeof(testdata.message));

	isc_tlsctx_cache_create(mctx, &tls_tlsctx_client_cache);

	if (isc_tlsctx_createserver(NULL, NULL, &tls_listen_tlsctx) !=
	    ISC_R_SUCCESS)
	{
		return -1;
	}

	dns_name_init(&tls_name, NULL);
	isc_buffer_constinit(&namesrc, tls_name_str, strlen(tls_name_str));
	isc_buffer_add(&namesrc, strlen(tls_name_str));
	isc_buffer_init(&namebuf, namedata, sizeof(namedata));
	if (dns_name_fromtext(&tls_name, &namesrc, dns_rootname,
			      DNS_NAME_DOWNCASE, &namebuf) != ISC_R_SUCCESS)
	{
		return -1;
	}
	transport_list = dns_transport_list_new(mctx);
	tls_transport = dns_transport_new(&tls_name, DNS_TRANSPORT_TLS,
					  transport_list);
	dns_transport_set_tlsname(tls_transport, tls_name_str);

	return 0;
}

static int
teardown_test(void **state) {
	dns_transport_list_detach(&transport_list);
	isc_tlsctx_cache_detach(&tls_tlsctx_client_cache);
	isc_tlsctx_free(&tls_listen_tlsctx);

	isc_netmgr_destroy(&connect_nm);

	teardown_netmgr(state);
	teardown_loopmgr(state);

	return 0;
}

static isc_result_t
make_dispatchset(dns_dispatchmgr_t *dispatchmgr, unsigned int ndisps,
		 dns_dispatchset_t **dsetp) {
	isc_result_t result;
	isc_sockaddr_t any;
	dns_dispatch_t *disp = NULL;

	isc_sockaddr_any(&any);
	result = dns_dispatch_createudp(dispatchmgr, &any, &disp);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = dns_dispatchset_create(mctx, disp, dsetp, ndisps);
	dns_dispatch_detach(&disp);

	return result;
}

/* create dispatch set */
ISC_LOOP_TEST_IMPL(dispatchset_create) {
	dns_dispatchset_t *dset = NULL;
	isc_result_t result;
	dns_dispatchmgr_t *dispatchmgr = NULL;

	UNUSED(arg);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = make_dispatchset(dispatchmgr, 1, &dset);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_dispatchset_destroy(&dset);

	result = make_dispatchset(dispatchmgr, 10, &dset);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_dispatchset_destroy(&dset);

	dns_dispatchmgr_detach(&dispatchmgr);

	isc_loopmgr_shutdown(loopmgr);
}

/* test dispatch set per-loop dispatch */
ISC_LOOP_TEST_IMPL(dispatchset_get) {
	isc_result_t result;
	dns_dispatchmgr_t *dispatchmgr = NULL;
	dns_dispatchset_t *dset = NULL;
	dns_dispatch_t *d1, *d2, *d3, *d4, *d5;
	uint32_t tid_saved;

	UNUSED(arg);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = make_dispatchset(dispatchmgr, 1, &dset);
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

	dns_dispatchset_destroy(&dset);

	result = make_dispatchset(dispatchmgr, 4, &dset);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Temporarily modify and then restore the current thread's
	 * ID value, in order to confirm that different threads get
	 * different dispatch sets but the same thread gets the same
	 * one.
	 */
	tid_saved = isc__tid_local;
	d1 = dns_dispatchset_get(dset);
	isc__tid_local++;
	d2 = dns_dispatchset_get(dset);
	isc__tid_local++;
	d3 = dns_dispatchset_get(dset);
	isc__tid_local++;
	d4 = dns_dispatchset_get(dset);
	isc__tid_local = tid_saved;
	d5 = dns_dispatchset_get(dset);

	assert_ptr_equal(d1, d5);
	assert_ptr_not_equal(d1, d2);
	assert_ptr_not_equal(d2, d3);
	assert_ptr_not_equal(d3, d4);
	assert_ptr_not_equal(d4, d5);

	dns_dispatchset_destroy(&dset);
	dns_dispatchmgr_detach(&dispatchmgr);
	isc_loopmgr_shutdown(loopmgr);
}

static atomic_bool first = true;

static void
server_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(arg);

	return;
}

static void
nameserver(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	   void *arg ISC_ATTR_UNUSED) {
	isc_region_t response1, response2;
	static unsigned char buf1[16];
	static unsigned char buf2[16];

	if (eresult != ISC_R_SUCCESS) {
		return;
	}

	memmove(buf1, region->base, 12);
	memset(buf1 + 12, 0, 4);
	buf1[2] |= 0x80; /* qr=1 */

	memmove(buf2, region->base, 12);
	memset(buf2 + 12, 1, 4);
	buf2[2] |= 0x80; /* qr=1 */

	/*
	 * send message to be discarded.
	 */
	response1.base = buf1;
	response1.length = sizeof(buf1);
	isc_nm_send(handle, &response1, server_senddone, NULL);

	/*
	 * send nextitem message.
	 */
	response2.base = buf2;
	response2.length = sizeof(buf2);
	isc_nm_send(handle, &response2, server_senddone, NULL);
}

static isc_result_t
accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	UNUSED(handle);
	UNUSED(arg);

	return eresult;
}

static void
noop_nameserver(isc_nmhandle_t *handle, isc_result_t eresult,
		isc_region_t *region, void *arg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(arg);
}

static void
response_getnext(isc_result_t result, isc_region_t *region ISC_ATTR_UNUSED,
		 void *arg) {
	test_dispatch_t *test = arg;

	if (atomic_compare_exchange_strong(&first, &(bool){ true }, false)) {
		result = dns_dispatch_getnext(test->dispentry);
		assert_int_equal(result, ISC_R_SUCCESS);
	} else {
		test_dispatch_shutdown(test);
	}
}

static void
response_noop(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
	      void *arg) {
	test_dispatch_t *test = arg;

	if (eresult == ISC_R_SUCCESS) {
		test_dispatch_done(test);
	}
}

static void
response_shutdown(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
		  void *arg) {
	test_dispatch_t *test = arg;

	assert_int_equal(eresult, ISC_R_SUCCESS);

	test_dispatch_shutdown(test);
}

static void
response_timeout(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
		 void *arg) {
	test_dispatch_t *test = arg;

	assert_int_equal(eresult, ISC_R_TIMEDOUT);

	test_dispatch_shutdown(test);
}

static void
client_senddone(isc_result_t eresult, isc_region_t *region, void *arg) {
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(arg);
}

static void
connected(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
	  void *arg) {
	test_dispatch_t *test = arg;

	switch (eresult) {
	case ISC_R_CONNECTIONRESET:
		/* Don't send any data if the connection failed */
		test_dispatch_shutdown(test);
		return;
	default:
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	dns_dispatch_send(test->dispentry, &testdata.region);
}

static void
connected_shutdown(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
		   void *arg) {
	test_dispatch_t *test = arg;

	switch (eresult) {
	case ISC_R_CONNECTIONRESET:
		/* Skip */
		break;
	default:
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	test_dispatch_shutdown(test);
}

static void
connected_gettcp(isc_result_t eresult ISC_ATTR_UNUSED,
		 isc_region_t *region ISC_ATTR_UNUSED, void *arg) {
	test_dispatch_t *test1 = arg;

	/* Client 2 */
	isc_result_t result;
	test_dispatch_t *test2 = isc_mem_get(mctx, sizeof(*test2));
	*test2 = (test_dispatch_t){
		.dispatchmgr = dns_dispatchmgr_ref(test1->dispatchmgr),
	};

	result = dns_dispatch_gettcp(test2->dispatchmgr, &tcp_server_addr,
				     &tcp_connect_addr, NULL, &test2->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_ptr_equal(test1->dispatch, test2->dispatch);

	result = dns_dispatch_add(test2->dispatch, isc_loop_main(loopmgr), 0,
				  T_CLIENT_CONNECT, &tcp_server_addr, NULL,
				  NULL, connected_shutdown, client_senddone,
				  response_noop, test2, &test2->id,
				  &test2->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatch_connect(test2->dispentry);

	test_dispatch_done(test1);
}

static void
connected_newtcp(isc_result_t eresult ISC_ATTR_UNUSED,
		 isc_region_t *region ISC_ATTR_UNUSED, void *arg) {
	test_dispatch_t *test3 = arg;

	/* Client - unshared */
	isc_result_t result;
	test_dispatch_t *test4 = isc_mem_get(mctx, sizeof(*test4));
	*test4 = (test_dispatch_t){
		.dispatchmgr = dns_dispatchmgr_ref(test3->dispatchmgr),
	};
	result = dns_dispatch_gettcp(test4->dispatchmgr, &tcp_server_addr,
				     &tcp_connect_addr, NULL, &test4->dispatch);
	assert_int_equal(result, ISC_R_NOTFOUND);

	result = dns_dispatch_createtcp(
		test4->dispatchmgr, &tcp_connect_addr, &tcp_server_addr, NULL,
		DNS_DISPATCHOPT_UNSHARED, &test4->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_ptr_not_equal(test3->dispatch, test4->dispatch);

	result = dns_dispatch_add(test4->dispatch, isc_loop_main(loopmgr), 0,
				  T_CLIENT_CONNECT, &tcp_server_addr, NULL,
				  NULL, connected_shutdown, client_senddone,
				  response_noop, test4, &test4->id,
				  &test4->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatch_connect(test4->dispentry);

	test_dispatch_done(test3);
}

static void
timeout_connected(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
		  void *arg) {
	test_dispatch_t *test = arg;

	switch (eresult) {
	case ISC_R_ADDRNOTAVAIL:
	case ISC_R_CONNREFUSED:
		/* Skip */
		break;
	default:
		assert_int_equal(eresult, ISC_R_TIMEDOUT);
	}

	test_dispatch_shutdown(test);
}

ISC_LOOP_TEST_IMPL(dispatch_timeout_tcp_connect) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Client */
	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_blackhole, 0);

	testdata.region.base = testdata.message;
	testdata.region.length = sizeof(testdata.message);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(test->dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, NULL, 0,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(test->dispatch, isc_loop_main(loopmgr), 0,
				  T_CLIENT_CONNECT_SHORT, &tcp_server_addr,
				  NULL, NULL, timeout_connected,
				  client_senddone, response_timeout, test,
				  &test->id, &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	testdata.message[0] = (test->id >> 8) & 0xff;
	testdata.message[1] = test->id & 0xff;

	dns_dispatch_connect(test->dispentry);
}

static void
stop_listening(void *arg) {
	UNUSED(arg);

	isc_nm_stoplistening(sock);
	isc_nmsocket_close(&sock);
	assert_null(sock);
}

ISC_LOOP_TEST_IMPL(dispatch_timeout_tcp_response) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenstreamdns(
		netmgr, ISC_NM_LISTEN_ONE, &tcp_server_addr, noop_nameserver,
		NULL, accept_cb, NULL, 0, NULL, NULL, ISC_NM_PROXY_NONE, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* ensure we stop listening after the test is done */
	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	/* Client */
	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(test->dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, NULL, 0,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(test->dispatch, isc_loop_main(loopmgr), 0,
				  T_CLIENT_CONNECT_SHORT, &tcp_server_addr,
				  NULL, NULL, connected, client_senddone,
				  response_timeout, test, &test->id,
				  &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatch_connect(test->dispentry);
}

ISC_LOOP_TEST_IMPL(dispatch_tcp_response) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenstreamdns(
		netmgr, ISC_NM_LISTEN_ONE, &tcp_server_addr, nameserver, NULL,
		accept_cb, NULL, 0, NULL, NULL, ISC_NM_PROXY_NONE, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	/* Client */
	testdata.region.base = testdata.message;
	testdata.region.length = sizeof(testdata.message);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(test->dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, NULL, 0,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(
		test->dispatch, isc_loop_main(loopmgr), 0, T_CLIENT_CONNECT,
		&tcp_server_addr, NULL, NULL, connected, client_senddone,
		response_shutdown, test, &test->id, &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	testdata.message[0] = (test->id >> 8) & 0xff;
	testdata.message[1] = test->id & 0xff;

	dns_dispatch_connect(test->dispentry);
}

ISC_LOOP_TEST_IMPL(dispatch_tls_response) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenstreamdns(
		netmgr, ISC_NM_LISTEN_ONE, &tls_server_addr, nameserver, NULL,
		accept_cb, NULL, 0, NULL, tls_listen_tlsctx, ISC_NM_PROXY_NONE,
		&sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	/* Client */
	testdata.region.base = testdata.message;
	testdata.region.length = sizeof(testdata.message);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(test->dispatchmgr, &tls_connect_addr,
					&tls_server_addr, tls_transport, 0,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(test->dispatch, isc_loop_main(loopmgr), 0,
				  T_CLIENT_CONNECT, &tls_server_addr,
				  tls_transport, tls_tlsctx_client_cache,
				  connected, client_senddone, response_shutdown,
				  test, &test->id, &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	testdata.message[0] = (test->id >> 8) & 0xff;
	testdata.message[1] = test->id & 0xff;

	dns_dispatch_connect(test->dispentry);
}

ISC_LOOP_TEST_IMPL(dispatch_timeout_udp_response) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenudp(netmgr, ISC_NM_LISTEN_ONE, &udp_server_addr,
				  noop_nameserver, NULL, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* ensure we stop listening after the test is done */
	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	/* Client */
	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createudp(test->dispatchmgr, &udp_connect_addr,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(test->dispatch, isc_loop_main(loopmgr), 0,
				  T_CLIENT_CONNECT_SHORT, &udp_server_addr,
				  NULL, NULL, connected, client_senddone,
				  response_timeout, test, &test->id,
				  &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatch_connect(test->dispentry);
}

/* test dispatch getnext */
ISC_LOOP_TEST_IMPL(dispatch_getnext) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenudp(netmgr, ISC_NM_LISTEN_ONE, &udp_server_addr,
				  nameserver, NULL, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	/* Client */
	testdata.region.base = testdata.message;
	testdata.region.length = sizeof(testdata.message);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createudp(test->dispatchmgr, &udp_connect_addr,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(
		test->dispatch, isc_loop_main(loopmgr), 0, T_CLIENT_CONNECT,
		&udp_server_addr, NULL, NULL, connected, client_senddone,
		response_getnext, test, &test->id, &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	testdata.message[0] = (test->id >> 8) & 0xff;
	testdata.message[1] = test->id & 0xff;

	dns_dispatch_connect(test->dispentry);
}

ISC_LOOP_TEST_IMPL(dispatch_gettcp) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenstreamdns(
		netmgr, ISC_NM_LISTEN_ONE, &tcp_server_addr, nameserver, NULL,
		accept_cb, NULL, 0, NULL, NULL, ISC_NM_PROXY_NONE, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* ensure we stop listening after the test is done */
	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Client */
	result = dns_dispatch_createtcp(test->dispatchmgr, &tcp_connect_addr,
					&tcp_server_addr, NULL, 0,
					&test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(
		test->dispatch, isc_loop_main(loopmgr), 0, T_CLIENT_CONNECT,
		&tcp_server_addr, NULL, NULL, connected_gettcp, client_senddone,
		response_noop, test, &test->id, &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatch_connect(test->dispentry);
}

ISC_LOOP_TEST_IMPL(dispatch_newtcp) {
	isc_result_t result;
	test_dispatch_t *test = isc_mem_get(mctx, sizeof(*test));
	*test = (test_dispatch_t){ 0 };

	/* Server */
	result = isc_nm_listenstreamdns(
		netmgr, ISC_NM_LISTEN_ONE, &tcp_server_addr, nameserver, NULL,
		accept_cb, NULL, 0, NULL, NULL, ISC_NM_PROXY_NONE, &sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* ensure we stop listening after the test is done */
	isc_loop_teardown(isc_loop_main(loopmgr), stop_listening, sock);

	/* Client - unshared */
	result = dns_dispatchmgr_create(mctx, loopmgr, connect_nm,
					&test->dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_createtcp(
		test->dispatchmgr, &tcp_connect_addr, &tcp_server_addr, NULL,
		DNS_DISPATCHOPT_UNSHARED, &test->dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatch_add(
		test->dispatch, isc_loop_main(loopmgr), 0, T_CLIENT_CONNECT,
		&tcp_server_addr, NULL, NULL, connected_newtcp, client_senddone,
		response_noop, test, &test->id, &test->dispentry);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatch_connect(test->dispentry);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(dispatch_gettcp, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_newtcp, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_timeout_udp_response, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatchset_create, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatchset_get, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_timeout_tcp_response, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_timeout_tcp_connect, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_tcp_response, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_tls_response, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(dispatch_getnext, setup_test, teardown_test)
ISC_TEST_LIST_END

ISC_TEST_MAIN
