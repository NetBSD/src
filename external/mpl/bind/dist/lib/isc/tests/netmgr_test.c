/*	$NetBSD: netmgr_test.c,v 1.3 2022/09/23 12:15:34 christos Exp $	*/

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
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/nonce.h>
#include <isc/os.h>
#include <isc/quota.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "uv_wrap.h"
#define KEEP_BEFORE

#include "../netmgr/netmgr-int.h"
#include "../netmgr/udp.c"
#include "../netmgr/uv-compat.c"
#include "../netmgr/uv-compat.h"
#include "../netmgr_p.h"
#include "isctest.h"

typedef void (*stream_connect_function)(isc_nm_t *nm);

static void
connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);
static void
connect_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		isc_region_t *region, void *cbarg);

isc_nm_t *listen_nm = NULL;
isc_nm_t *connect_nm = NULL;

static isc_sockaddr_t udp_listen_addr;
static isc_sockaddr_t udp_connect_addr;

static isc_sockaddr_t tcp_listen_addr;
static isc_sockaddr_t tcp_connect_addr;

static uint64_t send_magic = 0;
static uint64_t stop_magic = 0;

static isc_region_t send_msg = { .base = (unsigned char *)&send_magic,
				 .length = sizeof(send_magic) };

static isc_region_t stop_msg = { .base = (unsigned char *)&stop_magic,
				 .length = sizeof(stop_magic) };

static atomic_bool do_send = false;
static unsigned int workers = 0;

static atomic_int_fast64_t nsends;
static int_fast64_t esends; /* expected sends */

static atomic_int_fast64_t ssends = 0;
static atomic_int_fast64_t sreads = 0;
static atomic_int_fast64_t saccepts = 0;

static atomic_int_fast64_t cconnects = 0;
static atomic_int_fast64_t csends = 0;
static atomic_int_fast64_t creads = 0;
static atomic_int_fast64_t ctimeouts = 0;

static isc_refcount_t active_cconnects;
static isc_refcount_t active_csends;
static isc_refcount_t active_creads;
static isc_refcount_t active_ssends;
static isc_refcount_t active_sreads;

static isc_quota_t listener_quota;
static atomic_bool check_listener_quota;

static bool skip_long_tests = false;

static bool allow_send_back = false;
static bool noanswer = false;

static isc_nm_recv_cb_t connect_readcb = NULL;

#define SKIP_IN_CI             \
	if (skip_long_tests) { \
		skip();        \
		return;        \
	}

#define NSENDS 100

/* Timeout for soft-timeout tests (0.05 seconds) */
#define T_SOFT 50

/* Timeouts in miliseconds */
#define T_INIT	     120 * 1000
#define T_IDLE	     120 * 1000
#define T_KEEPALIVE  120 * 1000
#define T_ADVERTISED 120 * 1000
#define T_CONNECT    30 * 1000

/* Wait for 1 second (1000 * 1000 microseconds) */
#define WAIT_REPEATS 1000
#define T_WAIT	     1000 /* In microseconds */

#define WAIT_FOR(v, op, val)                                \
	{                                                   \
		X(v);                                       \
		int_fast64_t __r = WAIT_REPEATS;            \
		int_fast64_t __o = 0;                       \
		do {                                        \
			int_fast64_t __l = atomic_load(&v); \
			if (__l op val) {                   \
				break;                      \
			};                                  \
			if (__o == __l) {                   \
				__r--;                      \
			} else {                            \
				__r = WAIT_REPEATS;         \
			}                                   \
			__o = __l;                          \
			isc_test_nap(T_WAIT);               \
		} while (__r > 0);                          \
		X(v);                                       \
		P(__r);                                     \
		assert_true(atomic_load(&v) op val);        \
	}

#define WAIT_FOR_EQ(v, val) WAIT_FOR(v, ==, val)
#define WAIT_FOR_NE(v, val) WAIT_FOR(v, !=, val)
#define WAIT_FOR_LE(v, val) WAIT_FOR(v, <=, val)
#define WAIT_FOR_LT(v, val) WAIT_FOR(v, <, val)
#define WAIT_FOR_GE(v, val) WAIT_FOR(v, >=, val)
#define WAIT_FOR_GT(v, val) WAIT_FOR(v, >, val)

#define DONE() atomic_store(&do_send, false);

#define CHECK_RANGE_FULL(v)                \
	{                                  \
		int __v = atomic_load(&v); \
		assert_true(__v > 1);      \
	}

#define CHECK_RANGE_HALF(v)                \
	{                                  \
		int __v = atomic_load(&v); \
		assert_true(__v > 1);      \
	}

/* Enable this to print values while running tests */
#undef PRINT_DEBUG
#ifdef PRINT_DEBUG
#define X(v)                                                               \
	fprintf(stderr, "%s:%s:%d:%s = %" PRId64 "\n", __func__, __FILE__, \
		__LINE__, #v, atomic_load(&v))
#define P(v) fprintf(stderr, #v " = %" PRId64 "\n", v)
#define F()                                                   \
	fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle, \
		isc_result_totext(eresult), cbarg)
#else
#define X(v)
#define P(v)
#define F()
#endif

#define atomic_assert_int_eq(val, exp) assert_int_equal(atomic_load(&val), exp)
#define atomic_assert_int_ne(val, exp) \
	assert_int_not_equal(atomic_load(&val), exp)
#define atomic_assert_int_le(val, exp) assert_true(atomic_load(&val) <= exp)
#define atomic_assert_int_lt(val, exp) assert_true(atomic_load(&val) > exp)
#define atomic_assert_int_ge(val, exp) assert_true(atomic_load(&val) >= exp)
#define atomic_assert_int_gt(val, exp) assert_true(atomic_load(&val) > exp)

static int
_setup(void **state __attribute__((unused))) {
	char *p = NULL;

	if (workers == 0) {
		workers = isc_os_ncpus();
	}
	p = getenv("ISC_TASK_WORKERS");
	if (p != NULL) {
		workers = atoi(p);
	}
	INSIST(workers != 0);

	if (isc_test_begin(NULL, false, workers) != ISC_R_SUCCESS) {
		return (-1);
	}

	signal(SIGPIPE, SIG_IGN);

	if (getenv("CI") == NULL || getenv("CI_ENABLE_ALL_TESTS") != NULL) {
		esends = NSENDS * workers;
	} else {
		esends = workers;
		skip_long_tests = true;
	}

	return (0);
}

static int
_teardown(void **state __attribute__((unused))) {
	isc_test_end();

	return (0);
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
		return (-1);
	}

	r = bind(fd, (const struct sockaddr *)&addr->type.sa,
		 sizeof(addr->type.sin6));
	if (r != 0) {
		perror("setup_ephemeral_port: bind()");
		isc__nm_closesocket(fd);
		return (r);
	}

	r = getsockname(fd, (struct sockaddr *)&addr->type.sa, &addrlen);
	if (r != 0) {
		perror("setup_ephemeral_port: getsockname()");
		isc__nm_closesocket(fd);
		return (r);
	}

#if IPV6_RECVERR
#define setsockopt_on(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 1 }, sizeof(int))

	r = setsockopt_on(fd, IPPROTO_IPV6, IPV6_RECVERR);
	if (r != 0) {
		perror("setup_ephemeral_port");
		isc__nm_closesocket(fd);
		return (r);
	}
#endif

	return (fd);
}

static int
nm_setup(void **state __attribute__((unused))) {
	uv_os_sock_t tcp_listen_sock = -1;
	uv_os_sock_t udp_listen_sock = -1;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	udp_listen_addr = (isc_sockaddr_t){ .length = 0 };
	udp_listen_sock = setup_ephemeral_port(&udp_listen_addr, SOCK_DGRAM);
	if (udp_listen_sock < 0) {
		return (-1);
	}
	isc__nm_closesocket(udp_listen_sock);

	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_loopback, 0);

	tcp_listen_addr = (isc_sockaddr_t){ .length = 0 };
	tcp_listen_sock = setup_ephemeral_port(&tcp_listen_addr, SOCK_STREAM);
	if (tcp_listen_sock < 0) {
		return (-1);
	}
	isc__nm_closesocket(tcp_listen_sock);

	atomic_store(&do_send, true);
	atomic_store(&nsends, esends);

	atomic_store(&saccepts, 0);
	atomic_store(&sreads, 0);
	atomic_store(&ssends, 0);

	atomic_store(&cconnects, 0);
	atomic_store(&csends, 0);
	atomic_store(&creads, 0);
	atomic_store(&ctimeouts, 0);
	allow_send_back = false;

	isc_refcount_init(&active_cconnects, 0);
	isc_refcount_init(&active_csends, 0);
	isc_refcount_init(&active_creads, 0);
	isc_refcount_init(&active_ssends, 0);
	isc_refcount_init(&active_sreads, 0);

	isc_nonce_buf(&send_magic, sizeof(send_magic));
	isc_nonce_buf(&stop_magic, sizeof(stop_magic));
	if (send_magic == stop_magic) {
		return (-1);
	}

	isc__netmgr_create(test_mctx, workers, &listen_nm);
	assert_non_null(listen_nm);
	isc_nm_settimeouts(listen_nm, T_INIT, T_IDLE, T_KEEPALIVE,
			   T_ADVERTISED);

	isc__netmgr_create(test_mctx, workers, &connect_nm);
	assert_non_null(connect_nm);
	isc_nm_settimeouts(connect_nm, T_INIT, T_IDLE, T_KEEPALIVE,
			   T_ADVERTISED);

	isc_quota_init(&listener_quota, 0);
	atomic_store(&check_listener_quota, false);

	connect_readcb = connect_read_cb;
	noanswer = false;

	return (0);
}

static int
nm_teardown(void **state __attribute__((unused))) {
	UNUSED(state);

	isc__netmgr_destroy(&connect_nm);
	assert_null(connect_nm);

	isc__netmgr_destroy(&listen_nm);
	assert_null(listen_nm);

	WAIT_FOR_EQ(active_cconnects, 0);
	WAIT_FOR_EQ(active_csends, 0);
	WAIT_FOR_EQ(active_csends, 0);
	WAIT_FOR_EQ(active_ssends, 0);
	WAIT_FOR_EQ(active_sreads, 0);

	isc_refcount_destroy(&active_cconnects);
	isc_refcount_destroy(&active_csends);
	isc_refcount_destroy(&active_creads);
	isc_refcount_destroy(&active_ssends);
	isc_refcount_destroy(&active_sreads);

	return (0);
}

/* Callbacks */

static void
noop_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	     void *cbarg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(cbarg);
}

static unsigned int
noop_accept_cb(isc_nmhandle_t *handle, unsigned int result, void *cbarg) {
	UNUSED(handle);
	UNUSED(result);
	UNUSED(cbarg);

	return (0);
}

static void
connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

static void
connect_send(isc_nmhandle_t *handle);

static void
connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *sendhandle = handle;

	assert_non_null(sendhandle);

	UNUSED(cbarg);

	F();

	if (eresult != ISC_R_SUCCESS) {
		/* Send failed, we need to stop reading too */
		isc_nm_cancelread(handle);
		goto unref;
	}

	atomic_fetch_add(&csends, 1);
unref:
	isc_refcount_decrement(&active_csends);
	isc_nmhandle_detach(&sendhandle);
}

static void
connect_send(isc_nmhandle_t *handle) {
	isc_nmhandle_t *sendhandle = NULL;
	isc_refcount_increment0(&active_csends);
	isc_nmhandle_attach(handle, &sendhandle);
	isc_nmhandle_setwritetimeout(handle, T_IDLE);
	if (atomic_fetch_sub(&nsends, 1) > 1) {
		isc_nm_send(sendhandle, &send_msg, connect_send_cb, NULL);
	} else {
		isc_nm_send(sendhandle, &stop_msg, connect_send_cb, NULL);
	}
}

static void
connect_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	if (eresult != ISC_R_SUCCESS) {
		goto unref;
	}

	assert_true(region->length >= sizeof(magic));

	atomic_fetch_add(&creads, 1);

	memmove(&magic, region->base, sizeof(magic));

	assert_true(magic == stop_magic || magic == send_magic);

	if (magic == send_magic && allow_send_back) {
		connect_send(handle);
		return;
	}

unref:
	isc_refcount_decrement(&active_creads);
	isc_nmhandle_detach(&handle);
}

static void
connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	UNUSED(cbarg);

	F();

	isc_refcount_decrement(&active_cconnects);

	if (eresult != ISC_R_SUCCESS || connect_readcb == NULL) {
		return;
	}

	atomic_fetch_add(&cconnects, 1);

	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, connect_readcb, NULL);

	connect_send(handle);
}

static void
listen_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *sendhandle = handle;

	UNUSED(cbarg);
	UNUSED(eresult);

	assert_non_null(sendhandle);

	F();

	if (eresult != ISC_R_SUCCESS) {
		goto unref;
	}

	atomic_fetch_add(&ssends, 1);
unref:
	isc_nmhandle_detach(&sendhandle);
	isc_refcount_decrement(&active_ssends);
}

static void
listen_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
	       isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	assert_non_null(handle);

	F();

	if (eresult != ISC_R_SUCCESS) {
		goto unref;
	}

	atomic_fetch_add(&sreads, 1);

	assert_true(region->length >= sizeof(magic));

	memmove(&magic, region->base, sizeof(magic));
	assert_true(magic == stop_magic || magic == send_magic);

	if (magic == send_magic) {
		if (!noanswer) {
			isc_nmhandle_t *sendhandle = NULL;
			isc_nmhandle_attach(handle, &sendhandle);
			isc_refcount_increment0(&active_ssends);
			isc_nmhandle_setwritetimeout(sendhandle, T_IDLE);
			isc_nm_send(sendhandle, &send_msg, listen_send_cb,
				    cbarg);
		}
		return;
	}

unref:
	if (handle == cbarg) {
		isc_refcount_decrement(&active_sreads);
		isc_nmhandle_detach(&handle);
	}
}

static isc_result_t
listen_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	F();

	return (eresult);
}

static isc_result_t
stream_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	UNUSED(cbarg);

	F();

	if (eresult != ISC_R_SUCCESS) {
		return (eresult);
	}

	atomic_fetch_add(&saccepts, 1);

	isc_refcount_increment0(&active_sreads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, listen_read_cb, readhandle);

	return (ISC_R_SUCCESS);
}

typedef void (*connect_func)(isc_nm_t *);

static isc_threadresult_t
connect_thread(isc_threadarg_t arg) {
	connect_func connect = (connect_func)arg;
	isc_sockaddr_t connect_addr;

	connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&connect_addr, &in6addr_loopback, 0);

	while (atomic_load(&do_send)) {
		uint_fast32_t active =
			isc_refcount_increment0(&active_cconnects);
		if (active > workers) {
			/*
			 * If we have more active connections than workers,
			 * start slowing down the connections to prevent the
			 * thundering herd problem.
			 */
			isc_test_nap((active - workers) * 1000);
		}
		connect(connect_nm);
	}

	return ((isc_threadresult_t)0);
}

/* UDP */

static void
udp_connect(isc_nm_t *nm) {
	isc_nm_udpconnect(nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
}

static void
mock_listenudp_uv_udp_open(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	WILL_RETURN(uv_udp_open, UV_ENOMEM);

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, noop_recv_cb,
				  NULL, 0, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;
}

static void
mock_listenudp_uv_udp_bind(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	WILL_RETURN(uv_udp_bind, UV_EADDRINUSE);

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, noop_recv_cb,
				  NULL, 0, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;
}

static void
mock_listenudp_uv_udp_recv_start(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	WILL_RETURN(uv_udp_recv_start, UV_EADDRINUSE);

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, noop_recv_cb,
				  NULL, 0, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;
}

static void
mock_udpconnect_uv_udp_open(void **state __attribute__((unused))) {
	WILL_RETURN(uv_udp_open, UV_ENOMEM);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	RESET_RETURN;
}

static void
mock_udpconnect_uv_udp_bind(void **state __attribute__((unused))) {
	WILL_RETURN(uv_udp_bind, UV_ENOMEM);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	RESET_RETURN;
}

#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
static void
mock_udpconnect_uv_udp_connect(void **state __attribute__((unused))) {
	WILL_RETURN(uv_udp_connect, UV_ENOMEM);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	RESET_RETURN;
}
#endif

static void
mock_udpconnect_uv_recv_buffer_size(void **state __attribute__((unused))) {
	WILL_RETURN(uv_recv_buffer_size, UV_ENOMEM);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	RESET_RETURN;
}

static void
mock_udpconnect_uv_send_buffer_size(void **state __attribute__((unused))) {
	WILL_RETURN(uv_send_buffer_size, UV_ENOMEM);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	RESET_RETURN;
}

static void
udp_noop(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, noop_recv_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	atomic_assert_int_eq(cconnects, 0);
	atomic_assert_int_eq(csends, 0);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
}

static void
udp_noresponse(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, noop_recv_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_EQ(csends, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
}

static void
timeout_retry_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		 isc_region_t *region, void *cbarg) {
	UNUSED(region);
	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	if (eresult == ISC_R_TIMEDOUT && atomic_load(&csends) < 5) {
		isc_nmhandle_settimeout(handle, T_SOFT);
		connect_send(handle);
		return;
	}

	atomic_fetch_add(&ctimeouts, 1);

	isc_refcount_decrement(&active_creads);
	isc_nmhandle_detach(&handle);
}

static void
udp_timeout_recovery(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	SKIP_IN_CI;

	/*
	 * Listen using the noop callback so that client reads will time out.
	 */
	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, noop_recv_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Connect with client timeout set to 0.05 seconds, then sleep for at
	 * least a second for each 'tick'. timeout_retry_cb() will give up
	 * after five timeouts.
	 */
	connect_readcb = timeout_retry_cb;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_SOFT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_GE(csends, 1);
	WAIT_FOR_GE(csends, 2);
	WAIT_FOR_GE(csends, 3);
	WAIT_FOR_GE(csends, 4);
	WAIT_FOR_EQ(csends, 5);
	WAIT_FOR_EQ(ctimeouts, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);
}

static void
udp_recv_one(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	atomic_store(&nsends, 1);

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, listen_read_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_LE(nsends, 0);
	WAIT_FOR_EQ(csends, 1);
	WAIT_FOR_EQ(sreads, 1);
	WAIT_FOR_EQ(ssends, 0);
	WAIT_FOR_EQ(creads, 0);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 1);
	atomic_assert_int_eq(ssends, 0);
}

static void
udp_recv_two(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	atomic_store(&nsends, 2);

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, listen_read_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_udpconnect(connect_nm, &udp_connect_addr, &udp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 2);
	WAIT_FOR_LE(nsends, 0);
	WAIT_FOR_EQ(csends, 2);
	WAIT_FOR_EQ(sreads, 2);
	WAIT_FOR_EQ(ssends, 1);
	WAIT_FOR_EQ(creads, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 2);
	atomic_assert_int_eq(csends, 2);
	atomic_assert_int_eq(creads, 1);
	atomic_assert_int_eq(sreads, 2);
	atomic_assert_int_eq(ssends, 1);
}

static void
udp_recv_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, listen_read_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, udp_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends);
	WAIT_FOR_GE(csends, esends);
	WAIT_FOR_GE(sreads, esends);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc__netmgr_shutdown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_FULL(creads);
	CHECK_RANGE_FULL(sreads);
	CHECK_RANGE_FULL(ssends);
}

static void
udp_recv_half_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, listen_read_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, udp_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends / 2);
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc__netmgr_shutdown(connect_nm);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
udp_half_recv_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, listen_read_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, udp_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends / 2);
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	/* Try to send a little while longer */
	isc_test_nap((esends / 2) * 10000);

	isc__netmgr_shutdown(connect_nm);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
udp_half_recv_half_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listenudp(listen_nm, &udp_listen_addr, listen_read_cb,
				  NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, udp_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends / 2);
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc__netmgr_shutdown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

/* Common stream protocols code */

static isc_quota_t *
tcp_listener_init_quota(size_t nthreads) {
	isc_quota_t *quotap = NULL;
	if (atomic_load(&check_listener_quota)) {
		unsigned max_quota = ISC_MAX(nthreads / 2, 1);
		isc_quota_max(&listener_quota, max_quota);
		quotap = &listener_quota;
	}
	return (quotap);
}

static void
tcp_connect(isc_nm_t *nm) {
	isc_nm_tcpconnect(nm, &tcp_connect_addr, &tcp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT, 0);
}

static stream_connect_function
get_stream_connect_function(void) {
	return (tcp_connect);
}

static isc_result_t
stream_listen(isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
	      size_t extrahandlesize, int backlog, isc_quota_t *quota,
	      isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;

	result = isc_nm_listentcp(listen_nm, &tcp_listen_addr, accept_cb,
				  accept_cbarg, extrahandlesize, backlog, quota,
				  sockp);

	return (result);
}

static void
stream_connect(isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
	       size_t extrahandlesize) {
	isc_nm_tcpconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr, cb,
			  cbarg, timeout, extrahandlesize);
}

static void
stream_noop(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	result = stream_listen(noop_accept_cb, NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	stream_connect(connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	atomic_assert_int_eq(cconnects, 0);
	atomic_assert_int_eq(csends, 0);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
}

static void
stream_noresponse(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	result = stream_listen(noop_accept_cb, NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	stream_connect(connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_EQ(csends, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
}

static void
stream_timeout_recovery(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	SKIP_IN_CI;

	/*
	 * Accept connections but don't send responses, forcing client
	 * reads to time out.
	 */
	noanswer = true;
	result = stream_listen(stream_accept_cb, NULL, 0, 0, NULL,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Shorten all the client timeouts to 0.05 seconds.
	 */
	isc_nm_settimeouts(connect_nm, T_SOFT, T_SOFT, T_SOFT, T_SOFT);
	connect_readcb = timeout_retry_cb;
	isc_refcount_increment0(&active_cconnects);
	stream_connect(connect_connect_cb, NULL, T_SOFT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_GE(csends, 1);
	WAIT_FOR_GE(csends, 2);
	WAIT_FOR_GE(csends, 3);
	WAIT_FOR_GE(csends, 4);
	WAIT_FOR_EQ(csends, 5);
	WAIT_FOR_EQ(ctimeouts, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);
}

static void
stream_recv_one(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_quota_t *quotap = tcp_listener_init_quota(1);

	atomic_store(&nsends, 1);

	result = stream_listen(stream_accept_cb, NULL, 0, 0, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	stream_connect(connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_LE(nsends, 0);
	WAIT_FOR_EQ(csends, 1);
	WAIT_FOR_EQ(sreads, 1);
	WAIT_FOR_EQ(ssends, 0);
	WAIT_FOR_EQ(creads, 0);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 1);
	atomic_assert_int_eq(ssends, 0);
}

static void
stream_recv_two(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_quota_t *quotap = tcp_listener_init_quota(1);

	atomic_store(&nsends, 2);

	result = stream_listen(stream_accept_cb, NULL, 0, 0, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	stream_connect(connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);

	isc_refcount_increment0(&active_cconnects);
	stream_connect(connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 2);
	WAIT_FOR_LE(nsends, 0);
	WAIT_FOR_EQ(csends, 2);
	WAIT_FOR_EQ(sreads, 2);
	WAIT_FOR_EQ(ssends, 1);
	WAIT_FOR_EQ(creads, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 2);
	atomic_assert_int_eq(csends, 2);
	atomic_assert_int_eq(creads, 1);
	atomic_assert_int_eq(sreads, 2);
	atomic_assert_int_eq(ssends, 1);
}

static void
stream_recv_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];
	isc_quota_t *quotap = tcp_listener_init_quota(workers);

	SKIP_IN_CI;

	result = stream_listen(stream_accept_cb, NULL, 0, 0, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, get_stream_connect_function(),
				  &threads[i]);
	}

	if (allow_send_back) {
		WAIT_FOR_GE(cconnects, 1);
	} else {
		WAIT_FOR_GE(cconnects, esends);
	}
	WAIT_FOR_GE(csends, esends);
	WAIT_FOR_GE(sreads, esends);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc__netmgr_shutdown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_FULL(creads);
	CHECK_RANGE_FULL(sreads);
	CHECK_RANGE_FULL(ssends);
}

static void
stream_recv_half_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];
	isc_quota_t *quotap = tcp_listener_init_quota(workers);

	SKIP_IN_CI;

	result = stream_listen(stream_accept_cb, NULL, 0, 0, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, get_stream_connect_function(),
				  &threads[i]);
	}

	if (allow_send_back) {
		WAIT_FOR_GE(cconnects, 1);
	} else {
		WAIT_FOR_GE(cconnects, esends / 2);
	}
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc__netmgr_shutdown(connect_nm);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
stream_half_recv_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];
	isc_quota_t *quotap = tcp_listener_init_quota(workers);

	SKIP_IN_CI;

	result = stream_listen(stream_accept_cb, NULL, 0, 0, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, get_stream_connect_function(),
				  &threads[i]);
	}

	if (allow_send_back) {
		WAIT_FOR_GE(cconnects, 1);
	} else {
		WAIT_FOR_GE(cconnects, esends / 2);
	}
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	/* Try to send a little while longer */
	isc_test_nap((esends / 2) * 10000);

	isc__netmgr_shutdown(connect_nm);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
stream_half_recv_half_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];
	isc_quota_t *quotap = tcp_listener_init_quota(workers);

	SKIP_IN_CI;

	result = stream_listen(stream_accept_cb, NULL, 0, 0, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, get_stream_connect_function(),
				  &threads[i]);
	}

	if (allow_send_back) {
		WAIT_FOR_GE(cconnects, 1);
	} else {
		WAIT_FOR_GE(cconnects, esends / 2);
	}
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc__netmgr_shutdown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

/* TCP */
static void
tcp_noop(void **state) {
	stream_noop(state);
}

static void
tcp_noresponse(void **state) {
	stream_noresponse(state);
}

static void
tcp_timeout_recovery(void **state) {
	stream_timeout_recovery(state);
}

static void
tcp_recv_one(void **state) {
	stream_recv_one(state);
}

static void
tcp_recv_two(void **state) {
	stream_recv_two(state);
}

static void
tcp_recv_send(void **state) {
	SKIP_IN_CI;
	stream_recv_send(state);
}

static void
tcp_recv_half_send(void **state) {
	SKIP_IN_CI;
	stream_recv_half_send(state);
}

static void
tcp_half_recv_send(void **state) {
	SKIP_IN_CI;
	stream_half_recv_send(state);
}

static void
tcp_half_recv_half_send(void **state) {
	SKIP_IN_CI;
	stream_half_recv_half_send(state);
}

static void
tcp_recv_send_sendback(void **state) {
	SKIP_IN_CI;
	stream_recv_send(state);
}

static void
tcp_recv_half_send_sendback(void **state) {
	SKIP_IN_CI;
	stream_recv_half_send(state);
}

static void
tcp_half_recv_send_sendback(void **state) {
	SKIP_IN_CI;
	stream_half_recv_send(state);
}

static void
tcp_half_recv_half_send_sendback(void **state) {
	SKIP_IN_CI;
	stream_half_recv_half_send(state);
}

/* TCP Quota */

static void
tcp_recv_one_quota(void **state) {
	atomic_store(&check_listener_quota, true);
	stream_recv_one(state);
}

static void
tcp_recv_two_quota(void **state) {
	atomic_store(&check_listener_quota, true);
	stream_recv_two(state);
}

static void
tcp_recv_send_quota(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	stream_recv_send(state);
}

static void
tcp_recv_half_send_quota(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	stream_recv_half_send(state);
}

static void
tcp_half_recv_send_quota(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	stream_half_recv_send(state);
}

static void
tcp_half_recv_half_send_quota(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	stream_half_recv_half_send(state);
}

static void
tcp_recv_send_quota_sendback(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	allow_send_back = true;
	stream_recv_send(state);
}

static void
tcp_recv_half_send_quota_sendback(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	allow_send_back = true;
	stream_recv_half_send(state);
}

static void
tcp_half_recv_send_quota_sendback(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	allow_send_back = true;
	stream_half_recv_send(state);
}

static void
tcp_half_recv_half_send_quota_sendback(void **state) {
	SKIP_IN_CI;
	atomic_store(&check_listener_quota, true);
	allow_send_back = true;
	stream_half_recv_half_send(state);
}

/* TCPDNS */

static void
tcpdns_connect(isc_nm_t *nm) {
	isc_nm_tcpdnsconnect(nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_CONNECT, 0);
}

static void
tcpdns_noop(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr, noop_recv_cb,
				     NULL, noop_accept_cb, NULL, 0, 0, NULL,
				     &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	connect_readcb = NULL;
	isc_refcount_increment0(&active_cconnects);
	isc_nm_tcpdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_CONNECT, 0);
	isc__netmgr_shutdown(connect_nm);

	atomic_assert_int_eq(cconnects, 0);
	atomic_assert_int_eq(csends, 0);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
}

static void
tcpdns_noresponse(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	isc_refcount_increment0(&active_cconnects);
	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr, noop_recv_cb,
				     NULL, noop_accept_cb, NULL, 0, 0, NULL,
				     &listen_sock);
	if (result != ISC_R_SUCCESS) {
		isc_refcount_decrement(&active_cconnects);
		isc_test_nap(1000);
	}
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_tcpdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_EQ(csends, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
}

static void
tcpdns_timeout_recovery(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	SKIP_IN_CI;

	/*
	 * Accept connections but don't send responses, forcing client
	 * reads to time out.
	 */
	noanswer = true;
	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Shorten all the TCP client timeouts to 0.05 seconds, connect,
	 * then sleep for at least a second for each 'tick'.
	 * timeout_retry_cb() will give up after five timeouts.
	 */
	connect_readcb = timeout_retry_cb;
	isc_nm_settimeouts(connect_nm, T_SOFT, T_SOFT, T_SOFT, T_SOFT);
	isc_refcount_increment0(&active_cconnects);
	isc_nm_tcpdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_SOFT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_GE(csends, 1);
	WAIT_FOR_GE(csends, 2);
	WAIT_FOR_GE(csends, 3);
	WAIT_FOR_GE(csends, 4);
	WAIT_FOR_EQ(csends, 5);
	WAIT_FOR_EQ(ctimeouts, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);
}

static void
tcpdns_recv_one(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	atomic_store(&nsends, 1);

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_tcpdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);
	WAIT_FOR_LE(nsends, 0);
	WAIT_FOR_EQ(csends, 1);
	WAIT_FOR_EQ(sreads, 1);
	WAIT_FOR_EQ(ssends, 0);
	WAIT_FOR_EQ(creads, 0);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 1);
	atomic_assert_int_eq(ssends, 0);
}

static void
tcpdns_recv_two(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;

	atomic_store(&nsends, 2);

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_tcpdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 1);

	isc_refcount_increment0(&active_cconnects);
	isc_nm_tcpdnsconnect(connect_nm, &tcp_connect_addr, &tcp_listen_addr,
			     connect_connect_cb, NULL, T_CONNECT, 0);

	WAIT_FOR_EQ(cconnects, 2);

	WAIT_FOR_LE(nsends, 0);
	WAIT_FOR_EQ(csends, 2);
	WAIT_FOR_EQ(sreads, 2);
	WAIT_FOR_EQ(ssends, 1);
	WAIT_FOR_EQ(creads, 1);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc__netmgr_shutdown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 2);
	atomic_assert_int_eq(csends, 2);
	atomic_assert_int_eq(creads, 1);
	atomic_assert_int_eq(sreads, 2);
	atomic_assert_int_eq(ssends, 1);
}

static void
tcpdns_recv_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, tcpdns_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends);
	WAIT_FOR_GE(csends, esends);
	WAIT_FOR_GE(sreads, esends);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc__netmgr_shutdown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_FULL(creads);
	CHECK_RANGE_FULL(sreads);
	CHECK_RANGE_FULL(ssends);
}

static void
tcpdns_recv_half_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, tcpdns_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends / 2);
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc__netmgr_shutdown(connect_nm);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
tcpdns_half_recv_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, tcpdns_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends / 2);
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	/* Try to send a little while longer */
	isc_test_nap((esends / 2) * 10000);

	isc__netmgr_shutdown(connect_nm);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
tcpdns_half_recv_half_send(void **state __attribute__((unused))) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_thread_t threads[workers];

	SKIP_IN_CI;

	result = isc_nm_listentcpdns(listen_nm, &tcp_listen_addr,
				     listen_read_cb, NULL, listen_accept_cb,
				     NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(threads, 0, sizeof(threads));
	for (size_t i = 0; i < workers; i++) {
		isc_thread_create(connect_thread, tcpdns_connect, &threads[i]);
	}

	WAIT_FOR_GE(cconnects, esends / 2);
	WAIT_FOR_GE(csends, esends / 2);
	WAIT_FOR_GE(sreads, esends / 2);
	WAIT_FOR_GE(ssends, esends / 2);
	WAIT_FOR_GE(creads, esends / 2);

	isc__netmgr_shutdown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	DONE();
	for (size_t i = 0; i < workers; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		/* UDP */
		cmocka_unit_test_setup_teardown(mock_listenudp_uv_udp_open,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(mock_listenudp_uv_udp_bind,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(
			mock_listenudp_uv_udp_recv_start, nm_setup,
			nm_teardown),
		cmocka_unit_test_setup_teardown(mock_udpconnect_uv_udp_open,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(mock_udpconnect_uv_udp_bind,
						nm_setup, nm_teardown),
#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
		cmocka_unit_test_setup_teardown(mock_udpconnect_uv_udp_connect,
						nm_setup, nm_teardown),
#endif
		cmocka_unit_test_setup_teardown(
			mock_udpconnect_uv_recv_buffer_size, nm_setup,
			nm_teardown),
		cmocka_unit_test_setup_teardown(
			mock_udpconnect_uv_send_buffer_size, nm_setup,
			nm_teardown),
		cmocka_unit_test_setup_teardown(udp_noop, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_noresponse, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_timeout_recovery, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_recv_one, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_recv_two, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_recv_half_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_half_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_half_recv_half_send,
						nm_setup, nm_teardown),

		/* TCP */
		cmocka_unit_test_setup_teardown(tcp_noop, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_noresponse, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_timeout_recovery, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_one, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_two, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_half_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_half_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_half_recv_half_send,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_send_sendback,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_half_send_sendback,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_half_recv_send_sendback,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(
			tcp_half_recv_half_send_sendback, nm_setup,
			nm_teardown),

		/* TCP Quota */
		cmocka_unit_test_setup_teardown(tcp_recv_one_quota, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_two_quota, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_send_quota, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_half_send_quota,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_half_recv_send_quota,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_half_recv_half_send_quota,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcp_recv_send_quota_sendback,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(
			tcp_recv_half_send_quota_sendback, nm_setup,
			nm_teardown),
		cmocka_unit_test_setup_teardown(
			tcp_half_recv_send_quota_sendback, nm_setup,
			nm_teardown),
		cmocka_unit_test_setup_teardown(
			tcp_half_recv_half_send_quota_sendback, nm_setup,
			nm_teardown),

		/* TCPDNS */
		cmocka_unit_test_setup_teardown(tcpdns_recv_one, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_recv_two, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_noop, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_noresponse, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_timeout_recovery,
						nm_setup, nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_recv_half_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_half_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_half_recv_half_send,
						nm_setup, nm_teardown),

	};

	return (cmocka_run_group_tests(tests, _setup, _teardown));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
