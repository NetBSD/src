/*	$NetBSD: netmgr_common.h,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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

#include <isc/atomic.h>
#include <isc/netmgr.h>
#include <isc/refcount.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr/netmgr-int.h"

/*
 * Pick unused port outside the ephemeral port range, so we don't clash with
 * connected sockets.
 */
#define UDP_TEST_PORT	      9153
#define TCP_TEST_PORT	      9154
#define TLS_TEST_PORT	      9155
#define TCPDNS_TEST_PORT      9156
#define TLSDNS_TEST_PORT      9157
#define PROXYSTREAM_TEST_PORT 9158
#define PROXYUDP_TEST_PORT    9159

typedef void (*stream_connect_function)(isc_nm_t *nm);
typedef void (*connect_func)(isc_nm_t *);

extern isc_nm_t *listen_nm;
extern isc_nm_t *connect_nm;

extern isc_sockaddr_t tcp_listen_addr;
extern isc_sockaddr_t tcp_connect_addr;
extern isc_tlsctx_t *tcp_listen_tlsctx;
extern isc_tlsctx_t *tcp_connect_tlsctx;
extern isc_tlsctx_client_session_cache_t *tcp_tlsctx_client_sess_cache;

extern isc_sockaddr_t udp_listen_addr;
extern isc_sockaddr_t udp_connect_addr;

extern uint64_t send_magic;
extern uint64_t stop_magic;

extern isc_region_t send_msg;
extern isc_region_t stop_msg;

extern atomic_bool do_send;

extern atomic_int_fast64_t nsends;
extern int_fast64_t esends; /* expected sends */

extern atomic_int_fast64_t ssends;
extern atomic_int_fast64_t sreads;
extern atomic_int_fast64_t saccepts;

extern atomic_int_fast64_t cconnects;
extern atomic_int_fast64_t csends;
extern atomic_int_fast64_t creads;
extern atomic_int_fast64_t ctimeouts;

extern int expected_ssends;
extern int expected_sreads;
extern int expected_csends;
extern int expected_cconnects;
extern int expected_creads;
extern int expected_ctimeouts;

extern bool ssends_shutdown;
extern bool sreads_shutdown;
extern bool csends_shutdown;
extern bool cconnects_shutdown;
extern bool creads_shutdown;
extern bool ctimeouts_shutdown;

#define have_expected_ssends(v) ((v) >= expected_ssends && expected_ssends >= 0)
#define have_expected_sreads(v) ((v) >= expected_sreads && expected_sreads >= 0)
#define have_expected_saccepts(v) \
	((v) >= expected_saccepts && expected_saccepts >= 0)
#define have_expected_csends(v) ((v) >= expected_csends && expected_csends >= 0)
#define have_expected_cconnects(v) \
	((v) >= expected_cconnects && expected_cconnects >= 0)
#define have_expected_creads(v) ((v) >= expected_creads && expected_creads >= 0)
#define have_expected_ctimeouts(v) \
	((v) >= expected_ctimeouts && expected_ctimeouts >= 0)

#define do_ssends_shutdown(lm)            \
	if (ssends_shutdown) {            \
		isc_loopmgr_shutdown(lm); \
	}
#define do_sreads_shutdown(lm)            \
	if (sreads_shutdown) {            \
		isc_loopmgr_shutdown(lm); \
	}
#define do_saccepts_shutdown(lm)          \
	if (saccepts_shutdown) {          \
		isc_loopmgr_shutdown(lm); \
	}
#define do_csends_shutdown(lm)            \
	if (csends_shutdown) {            \
		isc_loopmgr_shutdown(lm); \
	}
#define do_cconnects_shutdown(lm)         \
	if (cconnects_shutdown) {         \
		isc_loopmgr_shutdown(lm); \
	}
#define do_creads_shutdown(lm)            \
	if (creads_shutdown) {            \
		isc_loopmgr_shutdown(lm); \
	}
#define do_ctimeouts_shutdown(lm)         \
	if (ctimeouts_shutdown) {         \
		isc_loopmgr_shutdown(lm); \
	}

extern isc_refcount_t active_cconnects;
extern isc_refcount_t active_csends;
extern isc_refcount_t active_creads;
extern isc_refcount_t active_ssends;
extern isc_refcount_t active_sreads;

extern isc_nmsocket_t *listen_sock;

extern isc_quota_t listener_quota;
extern atomic_bool check_listener_quota;

extern bool allow_send_back;
extern bool noanswer;
extern bool stream_use_TLS;
extern bool stream_use_PROXY;
extern bool stream_PROXY_over_TLS;
extern bool stream;
extern in_port_t stream_port;

extern bool udp_use_PROXY;

extern isc_nm_recv_cb_t connect_readcb;

#define NSENDS 100

/* Timeout for soft-timeout tests (0.05 seconds) */
#define T_SOFT 50

/* Timeouts in miliseconds */
#define T_INIT	     120 * 1000
#define T_IDLE	     120 * 1000
#define T_KEEPALIVE  120 * 1000
#define T_ADVERTISED 120 * 1000
#define T_CONNECT    30 * 1000

/* Wait for 1 second (1000 milliseconds) */
#define WAIT_REPEATS 1000
#define T_WAIT	     1 /* 1 millisecond */

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
			uv_sleep(T_WAIT);                   \
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
#define F()                                                                 \
	fprintf(stderr, "%u:%s(%p, %s, %p)\n", isc_tid(), __func__, handle, \
		isc_result_totext(eresult), cbarg)

#define isc_loopmgr_shutdown(loopmgr)                                      \
	{                                                                  \
		fprintf(stderr, "%u:%s:%s:%d:isc_loopmgr_shutdown(%p)\n",  \
			isc_tid(), __func__, __FILE__, __LINE__, loopmgr); \
		isc_loopmgr_shutdown(loopmgr);                             \
	}
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

int
setup_netmgr_test(void **state);
int
teardown_netmgr_test(void **state ISC_ATTR_UNUSED);

void
noop_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	     void *cbarg);

isc_result_t
noop_accept_cb(isc_nmhandle_t *handle ISC_ATTR_UNUSED, isc_result_t result,
	       void *cbarg ISC_ATTR_UNUSED);

void
connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

void
connect_send(isc_nmhandle_t *handle);

void
connect_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		isc_region_t *region, void *cbarg);

void
connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);
void
connect_success_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

void
listen_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

void
listen_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
	       isc_region_t *region, void *cbarg);

isc_result_t
listen_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

isc_result_t
stream_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

void
timeout_retry_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		 isc_region_t *region, void *cbarg);

isc_quota_t *
tcp_listener_init_quota(size_t nthreads);

stream_connect_function
get_stream_connect_function(void);

isc_result_t
stream_listen(isc_nm_accept_cb_t accept_cb, void *accept_cbarg, int backlog,
	      isc_quota_t *quota, isc_nmsocket_t **sockp);

void
stream_connect(isc_nm_cb_t cb, void *cbarg, unsigned int timeout);

void
set_proxyheader_info(isc_nm_proxyheader_info_t *pi);

isc_nm_proxyheader_info_t *
get_proxyheader_info(void);

isc_nm_proxy_type_t
get_proxy_type(void);

void
proxy_verify_endpoints(isc_nmhandle_t *handle);

int
stream_noop_setup(void **state ISC_ATTR_UNUSED);
void
stream_noop(void **state ISC_ATTR_UNUSED);
int
stream_noop_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_noop_setup(void **state);
int
proxystream_noop_teardown(void **state);

int
proxystreamtls_noop_setup(void **state);
int
proxystreamtls_noop_teardown(void **state);

int
stream_noresponse_setup(void **state ISC_ATTR_UNUSED);
void
stream_noresponse(void **state ISC_ATTR_UNUSED);
int
stream_noresponse_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_noresponse_setup(void **state);
int
proxystream_noresponse_teardown(void **state);

int
proxystreamtls_noresponse_setup(void **state);
int
proxystreamtls_noresponse_teardown(void **state);

int
stream_timeout_recovery_setup(void **state ISC_ATTR_UNUSED);
void
stream_timeout_recovery(void **state ISC_ATTR_UNUSED);
int
stream_timeout_recovery_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_timeout_recovery_setup(void **state);
int
proxystream_timeout_recovery_teardown(void **state);

int
proxystreamtls_timeout_recovery_setup(void **state);
int
proxystreamtls_timeout_recovery_teardown(void **state);

int
stream_recv_one_setup(void **state ISC_ATTR_UNUSED);
void
stream_recv_one(void **state ISC_ATTR_UNUSED);
int
stream_recv_one_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_recv_one_setup(void **state);
int
proxystream_recv_one_teardown(void **state);

int
proxystreamtls_recv_one_setup(void **state);
int
proxystreamtls_recv_one_teardown(void **state);

int
stream_recv_two_setup(void **state ISC_ATTR_UNUSED);
void
stream_recv_two(void **state ISC_ATTR_UNUSED);
int
stream_recv_two_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_recv_two_setup(void **state);
int
proxystream_recv_two_teardown(void **state);

int
proxystreamtls_recv_two_setup(void **state);
int
proxystreamtls_recv_two_teardown(void **state);

int
stream_recv_send_setup(void **state ISC_ATTR_UNUSED);
void
stream_recv_send(void **state ISC_ATTR_UNUSED);
int
stream_recv_send_teardown(void **state ISC_ATTR_UNUSED);
void
stream_recv_send_connect(void *arg);

int
proxystream_recv_send_setup(void **state);
int
proxystream_recv_send_teardown(void **state);

int
proxystreamtls_recv_send_setup(void **state);
int
proxystreamtls_recv_send_teardown(void **state);

int
stream_shutdownconnect_setup(void **state ISC_ATTR_UNUSED);
void
stream_shutdownconnect(void **state ISC_ATTR_UNUSED);
int
stream_shutdownconnect_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_shutdownconnect_setup(void **state);
int
proxystream_shutdownconnect_teardown(void **state);

int
proxystreamtls_shutdownconnect_setup(void **state);
int
proxystreamtls_shutdownconnect_teardown(void **state);

int
stream_shutdownread_setup(void **state ISC_ATTR_UNUSED);
void
stream_shutdownread(void **state ISC_ATTR_UNUSED);
int
stream_shutdownread_teardown(void **state ISC_ATTR_UNUSED);

int
proxystream_shutdownread_setup(void **state);
int
proxystream_shutdownread_teardown(void **state);

int
proxystreamtls_shutdownread_setup(void **state);
int
proxystreamtls_shutdownread_teardown(void **state);

void
stop_listening(void *arg ISC_ATTR_UNUSED);

/* UDP */

/* Timeout for soft-timeout tests (0.05 seconds) */
#define UDP_T_SOFT 50

/* Timeouts in miliseconds */
#define UDP_T_INIT	 120 * 1000
#define UDP_T_IDLE	 120 * 1000
#define UDP_T_KEEPALIVE	 120 * 1000
#define UDP_T_ADVERTISED 120 * 1000
#define UDP_T_CONNECT	 30 * 1000

int
setup_udp_test(void **state);

int
teardown_udp_test(void **state);

int
udp_noop_setup(void **state);

int
udp_noop_teardown(void **state);

void
udp_noop(void **arg ISC_ATTR_UNUSED);

int
proxyudp_noop_setup(void **state);

int
proxyudp_noop_teardown(void **state);

int
udp_noresponse_setup(void **state);

int
udp_noresponse_teardown(void **state);

void
udp_noresponse(void **arg ISC_ATTR_UNUSED);

int
proxyudp_noresponse_setup(void **state);

int
proxyudp_noresponse_teardown(void **state);

int
udp_timeout_recovery_setup(void **state);

int
udp_timeout_recovery_teardown(void **state);

void
udp_timeout_recovery(void **arg ISC_ATTR_UNUSED);

int
proxyudp_timeout_recovery_setup(void **state);

int
proxyudp_timeout_recovery_teardown(void **state);

int
udp_shutdown_connect_setup(void **state);

int
udp_shutdown_connect_teardown(void **state);

void
udp_shutdown_connect(void **arg ISC_ATTR_UNUSED);

int
proxyudp_shutdown_connect_setup(void **state);

int
proxyudp_shutdown_connect_teardown(void **state);

int
udp_shutdown_read_setup(void **state);

int
udp_shutdown_read_teardown(void **state);

void
udp_shutdown_read(void **arg ISC_ATTR_UNUSED);

int
proxyudp_shutdown_read_setup(void **state);

int
proxyudp_shutdown_read_teardown(void **state);

int
udp_cancel_read_setup(void **state);

int
udp_cancel_read_teardown(void **state);

void
udp_cancel_read(void **arg ISC_ATTR_UNUSED);

int
proxyudp_cancel_read_setup(void **state);

int
proxyudp_cancel_read_teardown(void **state);

int
udp_recv_one_setup(void **state);

int
udp_recv_one_teardown(void **state);

void
udp_recv_one(void **arg ISC_ATTR_UNUSED);

int
proxyudp_recv_one_setup(void **state);

int
proxyudp_recv_one_teardown(void **state);

int
udp_recv_two_setup(void **state);

int
udp_recv_two_teardown(void **state);

void
udp_recv_two(void **arg ISC_ATTR_UNUSED);

int
proxyudp_recv_two_setup(void **state);

int
proxyudp_recv_two_teardown(void **state);

int
udp_recv_send_setup(void **state);

int
udp_recv_send_teardown(void **state);

void
udp_recv_send(void **arg ISC_ATTR_UNUSED);

int
proxyudp_recv_send_setup(void **state);

int
proxyudp_recv_send_teardown(void **state);

int
udp_double_read_setup(void **state);

int
udp_double_read_teardown(void **state);

void
udp_double_read(void **arg ISC_ATTR_UNUSED);

int
proxyudp_double_read_setup(void **state);

int
proxyudp_double_read_teardown(void **state);
