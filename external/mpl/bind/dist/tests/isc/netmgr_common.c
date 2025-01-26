/*	$NetBSD: netmgr_common.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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
#include <isc/nonce.h>
#include <isc/os.h>
#include <isc/quota.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>
#include <isc/util.h>
#include <isc/uv.h>
#define KEEP_BEFORE

#include "netmgr_common.h"

#include <tests/isc.h>

isc_nm_t *listen_nm = NULL;
isc_nm_t *connect_nm = NULL;

isc_sockaddr_t tcp_listen_addr;
isc_sockaddr_t tcp_connect_addr;
isc_tlsctx_t *tcp_listen_tlsctx = NULL;
isc_tlsctx_t *tcp_connect_tlsctx = NULL;
isc_tlsctx_client_session_cache_t *tcp_tlsctx_client_sess_cache = NULL;

isc_sockaddr_t udp_listen_addr;
isc_sockaddr_t udp_connect_addr;

uint64_t send_magic = 0;

isc_region_t send_msg = { .base = (unsigned char *)&send_magic,
			  .length = sizeof(send_magic) };

atomic_bool do_send = false;

atomic_int_fast64_t nsends = 0;
int_fast64_t esends = 0; /* expected sends */

atomic_int_fast64_t ssends = 0;
atomic_int_fast64_t sreads = 0;
atomic_int_fast64_t saccepts = 0;

atomic_int_fast64_t cconnects = 0;
atomic_int_fast64_t csends = 0;
atomic_int_fast64_t creads = 0;
atomic_int_fast64_t ctimeouts = 0;

int expected_ssends;
int expected_sreads;
int expected_csends;
int expected_cconnects;
int expected_creads;
int expected_saccepts;
int expected_ctimeouts;

bool ssends_shutdown;
bool sreads_shutdown;
bool saccepts_shutdown;
bool csends_shutdown;
bool cconnects_shutdown;
bool creads_shutdown;
bool ctimeouts_shutdown;

isc_refcount_t active_cconnects = 0;
isc_refcount_t active_csends = 0;
isc_refcount_t active_creads = 0;
isc_refcount_t active_ssends = 0;
isc_refcount_t active_sreads = 0;

isc_nmsocket_t *listen_sock = NULL;

isc_quota_t listener_quota;
atomic_bool check_listener_quota = false;

bool allow_send_back = false;
bool noanswer = false;
bool stream_use_TLS = false;
bool stream_use_PROXY = false;
bool stream_PROXY_over_TLS = false;
bool stream = false;
in_port_t stream_port = 0;

bool udp_use_PROXY = false;

isc_nm_recv_cb_t connect_readcb = NULL;

isc_nm_proxyheader_info_t proxy_info_data;
isc_nm_proxyheader_info_t *proxy_info = NULL;
isc_sockaddr_t proxy_src;
isc_sockaddr_t proxy_dst;

int
setup_netmgr_test(void **state) {
	struct in_addr in;
	tcp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_connect_addr, &in6addr_loopback, 0);

	tcp_listen_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcp_listen_addr, &in6addr_loopback, stream_port);

	RUNTIME_CHECK(inet_pton(AF_INET, "1.2.3.4", &in) == 1);
	isc_sockaddr_fromin(&proxy_src, &in, 1234);
	RUNTIME_CHECK(inet_pton(AF_INET, "4.3.2.1", &in) == 1);
	isc_sockaddr_fromin(&proxy_dst, &in, 4321);
	isc_nm_proxyheader_info_init(&proxy_info_data, &proxy_src, &proxy_dst,
				     NULL);

	esends = NSENDS * workers;

	atomic_store(&nsends, esends);

	atomic_store(&saccepts, 0);
	atomic_store(&sreads, 0);
	atomic_store(&ssends, 0);

	atomic_store(&cconnects, 0);
	atomic_store(&csends, 0);
	atomic_store(&creads, 0);
	atomic_store(&ctimeouts, 0);
	allow_send_back = false;

	expected_cconnects = -1;
	expected_csends = -1;
	expected_creads = -1;
	expected_sreads = -1;
	expected_ssends = -1;
	expected_saccepts = -1;
	expected_ctimeouts = -1;

	ssends_shutdown = true;
	sreads_shutdown = true;
	saccepts_shutdown = true;
	csends_shutdown = true;
	cconnects_shutdown = true;
	creads_shutdown = true;
	ctimeouts_shutdown = true;

	do_send = false;

	isc_refcount_init(&active_cconnects, 0);
	isc_refcount_init(&active_csends, 0);
	isc_refcount_init(&active_creads, 0);
	isc_refcount_init(&active_ssends, 0);
	isc_refcount_init(&active_sreads, 0);

	isc_nonce_buf(&send_magic, sizeof(send_magic));

	setup_loopmgr(state);
	isc_netmgr_create(mctx, loopmgr, &listen_nm);
	assert_non_null(listen_nm);
	isc_nm_settimeouts(listen_nm, T_INIT, T_IDLE, T_KEEPALIVE,
			   T_ADVERTISED);

	isc_netmgr_create(mctx, loopmgr, &connect_nm);
	assert_non_null(connect_nm);
	isc_nm_settimeouts(connect_nm, T_INIT, T_IDLE, T_KEEPALIVE,
			   T_ADVERTISED);

	isc_quota_init(&listener_quota, 0);
	atomic_store(&check_listener_quota, false);

	connect_readcb = connect_read_cb;
	noanswer = false;

	if (isc_tlsctx_createserver(NULL, NULL, &tcp_listen_tlsctx) !=
	    ISC_R_SUCCESS)
	{
		return -1;
	}
	if (isc_tlsctx_createclient(&tcp_connect_tlsctx) != ISC_R_SUCCESS) {
		return -1;
	}

	isc_tlsctx_enable_dot_client_alpn(tcp_connect_tlsctx);

	isc_tlsctx_client_session_cache_create(
		mctx, tcp_connect_tlsctx,
		ISC_TLSCTX_CLIENT_SESSION_CACHE_DEFAULT_SIZE,
		&tcp_tlsctx_client_sess_cache);

	return 0;
}

int
teardown_netmgr_test(void **state ISC_ATTR_UNUSED) {
	UNUSED(state);

	isc_tlsctx_client_session_cache_detach(&tcp_tlsctx_client_sess_cache);

	isc_tlsctx_free(&tcp_connect_tlsctx);
	isc_tlsctx_free(&tcp_listen_tlsctx);

	isc_netmgr_destroy(&connect_nm);
	assert_null(connect_nm);

	isc_netmgr_destroy(&listen_nm);
	assert_null(listen_nm);

	teardown_loopmgr(state);

	isc_refcount_destroy(&active_cconnects);
	isc_refcount_destroy(&active_csends);
	isc_refcount_destroy(&active_creads);
	isc_refcount_destroy(&active_ssends);
	isc_refcount_destroy(&active_sreads);

	proxy_info = NULL;

	return 0;
}

void
stop_listening(void *arg ISC_ATTR_UNUSED) {
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
}

/* Callbacks */

void
noop_recv_cb(isc_nmhandle_t *handle ISC_ATTR_UNUSED,
	     isc_result_t eresult ISC_ATTR_UNUSED,
	     isc_region_t *region ISC_ATTR_UNUSED,
	     void *cbarg ISC_ATTR_UNUSED) {
	F();
}

isc_result_t
noop_accept_cb(isc_nmhandle_t *handle ISC_ATTR_UNUSED, isc_result_t eresult,
	       void *cbarg ISC_ATTR_UNUSED) {
	F();

	if (eresult == ISC_R_SUCCESS) {
		(void)atomic_fetch_add(&saccepts, 1);
	}

	return ISC_R_SUCCESS;
}

void
connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

void
connect_send(isc_nmhandle_t *handle);

void
connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *sendhandle = handle;

	assert_non_null(sendhandle);

	UNUSED(cbarg);

	F();

	switch (eresult) {
	case ISC_R_EOF:
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
	case ISC_R_CONNECTIONRESET:
		/* Abort */
		if (!stream) {
			isc_nm_cancelread(handle);
		}
		break;
	case ISC_R_SUCCESS:
		if (have_expected_csends(atomic_fetch_add(&csends, 1) + 1)) {
			do_csends_shutdown(loopmgr);
		}
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_refcount_decrement(&active_csends);
	isc_nmhandle_detach(&sendhandle);
}

void
connect_send(isc_nmhandle_t *handle) {
	isc_nmhandle_t *sendhandle = NULL;
	isc_refcount_increment0(&active_csends);
	isc_nmhandle_attach(handle, &sendhandle);
	isc_nmhandle_setwritetimeout(handle, T_IDLE);
	isc_nm_send(sendhandle, &send_msg, connect_send_cb, NULL);
}

void
connect_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	switch (eresult) {
	case ISC_R_SUCCESS:
		assert_true(region->length >= sizeof(magic));

		memmove(&magic, region->base, sizeof(magic));

		assert_true(magic == send_magic);

		if (have_expected_creads(atomic_fetch_add(&creads, 1) + 1)) {
			do_creads_shutdown(loopmgr);
		}

		if (magic == send_magic && allow_send_back) {
			connect_send(handle);
			return;
		}

		/* This will initiate one more read callback */
		if (stream) {
			isc_nmhandle_close(handle);
		}
		break;
	case ISC_R_TIMEDOUT:
	case ISC_R_EOF:
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
	case ISC_R_CONNECTIONRESET:
	case ISC_R_CONNREFUSED:
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_refcount_decrement(&active_creads);
	isc_nmhandle_detach(&handle);
}

void
connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	F();

	isc_refcount_decrement(&active_cconnects);

	if (eresult != ISC_R_SUCCESS || connect_readcb == NULL) {
		return;
	}

	if (stream_use_PROXY) {
		assert_true(isc_nm_is_proxy_handle(handle));
	}

	/* We are finished, initiate the shutdown */
	if (have_expected_cconnects(atomic_fetch_add(&cconnects, 1) + 1)) {
		do_cconnects_shutdown(loopmgr);
	} else if (do_send) {
		isc_async_current(stream_recv_send_connect,
				  (cbarg == NULL
					   ? get_stream_connect_function()
					   : (stream_connect_function)cbarg));
	}

	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, connect_readcb, NULL);

	connect_send(handle);
}

void
listen_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *sendhandle = handle;

	UNUSED(cbarg);
	UNUSED(eresult);

	assert_non_null(sendhandle);

	F();

	switch (eresult) {
	case ISC_R_CANCELED:
	case ISC_R_CONNECTIONRESET:
	case ISC_R_EOF:
	case ISC_R_SHUTTINGDOWN:
		break;
	case ISC_R_SUCCESS:
		if (have_expected_ssends(atomic_fetch_add(&ssends, 1) + 1)) {
			do_ssends_shutdown(loopmgr);
		}
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_refcount_decrement(&active_ssends);
	isc_nmhandle_detach(&sendhandle);
}

void
listen_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
	       isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	assert_non_null(handle);

	F();

	switch (eresult) {
	case ISC_R_SUCCESS:
		if (udp_use_PROXY || stream_use_PROXY) {
			assert_true(isc_nm_is_proxy_handle(handle));
			proxy_verify_endpoints(handle);
		}

		memmove(&magic, region->base, sizeof(magic));
		assert_true(magic == send_magic);

		if (have_expected_sreads(atomic_fetch_add(&sreads, 1) + 1)) {
			do_sreads_shutdown(loopmgr);
		}

		assert_true(region->length >= sizeof(magic));

		memmove(&magic, region->base, sizeof(magic));
		assert_true(magic == send_magic);

		if (!noanswer) {
			/* Answer and continue to listen */
			isc_nmhandle_t *sendhandle = NULL;
			isc_nmhandle_attach(handle, &sendhandle);
			isc_refcount_increment0(&active_ssends);
			isc_nmhandle_setwritetimeout(sendhandle, T_IDLE);
			isc_nm_send(sendhandle, &send_msg, listen_send_cb,
				    cbarg);
		}
		/* Continue to listen */
		return;
	case ISC_R_CANCELED:
	case ISC_R_CONNECTIONRESET:
	case ISC_R_EOF:
	case ISC_R_SHUTTINGDOWN:
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_refcount_decrement(&active_sreads);
	isc_nmhandle_detach(&handle);
}

isc_result_t
listen_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	F();

	if (eresult != ISC_R_SUCCESS) {
		return eresult;
	}

	if (have_expected_saccepts(atomic_fetch_add(&saccepts, 1) + 1)) {
		do_saccepts_shutdown(loopmgr);
	}

	isc_nmhandle_attach(handle, &(isc_nmhandle_t *){ NULL });
	isc_refcount_increment0(&active_sreads);

	return eresult;
}

isc_result_t
stream_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	UNUSED(cbarg);

	F();

	if (eresult != ISC_R_SUCCESS) {
		return eresult;
	}

	if (have_expected_saccepts(atomic_fetch_add(&saccepts, 1) + 1)) {
		do_saccepts_shutdown(loopmgr);
	}

	if (stream_use_PROXY) {
		assert_true(isc_nm_is_proxy_handle(handle));
		proxy_verify_endpoints(handle);
	}

	isc_refcount_increment0(&active_sreads);

	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, listen_read_cb, readhandle);

	return ISC_R_SUCCESS;
}

void
stream_recv_send_connect(void *arg) {
	connect_func connect = (connect_func)arg;
	isc_sockaddr_t connect_addr;

	connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&connect_addr, &in6addr_loopback, 0);

	isc_refcount_increment0(&active_cconnects);
	connect(connect_nm);
}

/* Common stream protocols code */

void
timeout_retry_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		 isc_region_t *region, void *cbarg) {
	UNUSED(region);
	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	if (eresult == ISC_R_TIMEDOUT &&
	    atomic_fetch_add(&ctimeouts, 1) + 1 < expected_ctimeouts)
	{
		isc_nmhandle_settimeout(handle, T_SOFT);
		connect_send(handle);
		return;
	}

	isc_refcount_decrement(&active_creads);
	isc_nmhandle_detach(&handle);

	isc_loopmgr_shutdown(loopmgr);
}

isc_quota_t *
tcp_listener_init_quota(size_t nthreads) {
	isc_quota_t *quotap = NULL;
	if (atomic_load(&check_listener_quota)) {
		unsigned int max_quota = ISC_MAX(nthreads / 2, 1);
		isc_quota_max(&listener_quota, max_quota);
		quotap = &listener_quota;
	}
	return quotap;
}

static void
tcp_connect(isc_nm_t *nm) {
	isc_nm_tcpconnect(nm, &tcp_connect_addr, &tcp_listen_addr,
			  connect_connect_cb, NULL, T_CONNECT);
}

static void
tls_connect(isc_nm_t *nm) {
	isc_nm_tlsconnect(nm, &tcp_connect_addr, &tcp_listen_addr,
			  connect_connect_cb, NULL, tcp_connect_tlsctx,
			  tcp_tlsctx_client_sess_cache, T_CONNECT,
			  stream_use_PROXY, NULL);
}

void
set_proxyheader_info(isc_nm_proxyheader_info_t *pi) {
	proxy_info = pi;
}

isc_nm_proxyheader_info_t *
get_proxyheader_info(void) {
	if (proxy_info != NULL) {
		return proxy_info;
	}

	/*
	 * There is 50% chance to get the info: so we can test LOCAL headers,
	 * too.
	 */
	if (isc_random_uniform(2)) {
		return &proxy_info_data;
	}

	return NULL;
}

static void
proxystream_connect(isc_nm_t *nm) {
	isc_tlsctx_t *tlsctx = stream_PROXY_over_TLS ? tcp_connect_tlsctx
						     : NULL;
	isc_tlsctx_client_session_cache_t *sess_cache =
		stream_PROXY_over_TLS ? tcp_tlsctx_client_sess_cache : NULL;

	isc_nm_proxystreamconnect(nm, &tcp_connect_addr, &tcp_listen_addr,
				  connect_connect_cb, NULL, T_CONNECT, tlsctx,
				  sess_cache, get_proxyheader_info());
}

stream_connect_function
get_stream_connect_function(void) {
	if (stream_use_TLS && !stream_PROXY_over_TLS) {
		return tls_connect;
	} else if (stream_use_PROXY) {
		return proxystream_connect;
	} else {
		return tcp_connect;
	}

	UNREACHABLE();
}

isc_result_t
stream_listen(isc_nm_accept_cb_t accept_cb, void *accept_cbarg, int backlog,
	      isc_quota_t *quota, isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;

	if (stream_use_TLS && !stream_PROXY_over_TLS) {
		result = isc_nm_listentls(
			listen_nm, ISC_NM_LISTEN_ALL, &tcp_listen_addr,
			accept_cb, accept_cbarg, backlog, quota,
			tcp_listen_tlsctx, stream_use_PROXY, sockp);
		return result;
	} else if (stream_use_PROXY) {
		isc_tlsctx_t *tlsctx = stream_PROXY_over_TLS ? tcp_listen_tlsctx
							     : NULL;
		result = isc_nm_listenproxystream(
			listen_nm, ISC_NM_LISTEN_ALL, &tcp_listen_addr,
			accept_cb, accept_cbarg, backlog, quota, tlsctx, sockp);
		return result;
	} else {
		result = isc_nm_listentcp(listen_nm, ISC_NM_LISTEN_ALL,
					  &tcp_listen_addr, accept_cb,
					  accept_cbarg, backlog, quota, sockp);
		return result;
	}

	UNREACHABLE();
}

void
stream_connect(isc_nm_cb_t cb, void *cbarg, unsigned int timeout) {
	isc_refcount_increment0(&active_cconnects);

	if (stream_use_TLS && !stream_PROXY_over_TLS) {
		isc_nm_tlsconnect(
			connect_nm, &tcp_connect_addr, &tcp_listen_addr, cb,
			cbarg, tcp_connect_tlsctx, tcp_tlsctx_client_sess_cache,
			timeout, stream_use_PROXY, NULL);
		return;
	} else if (stream_use_PROXY) {
		isc_tlsctx_t *tlsctx = stream_PROXY_over_TLS
					       ? tcp_connect_tlsctx
					       : NULL;
		isc_tlsctx_client_session_cache_t *sess_cache =
			stream_PROXY_over_TLS ? tcp_tlsctx_client_sess_cache
					      : NULL;
		isc_nm_proxystreamconnect(connect_nm, &tcp_connect_addr,
					  &tcp_listen_addr, cb, cbarg, timeout,
					  tlsctx, sess_cache,
					  get_proxyheader_info());
		return;
	} else {
		isc_nm_tcpconnect(connect_nm, &tcp_connect_addr,
				  &tcp_listen_addr, cb, cbarg, timeout);
		return;
	}
	UNREACHABLE();
}

isc_nm_proxy_type_t
get_proxy_type(void) {
	if (!stream_use_PROXY) {
		return ISC_NM_PROXY_NONE;
	} else if (stream_PROXY_over_TLS) {
		return ISC_NM_PROXY_ENCRYPTED;
	}

	return ISC_NM_PROXY_PLAIN;
}

void
connect_success_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	F();

	isc_refcount_decrement(&active_cconnects);
	assert_int_equal(eresult, ISC_R_SUCCESS);

	if (have_expected_cconnects(atomic_fetch_add(&cconnects, 1) + 1)) {
		do_cconnects_shutdown(loopmgr);
		return;
	}
}

int
stream_noop_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);
	expected_cconnects = 1;
	return r;
}

int
proxystream_noop_setup(void **state) {
	stream_use_PROXY = true;
	return stream_noop_setup(state);
}

int
proxystreamtls_noop_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_noop_setup(state);
}

void
stream_noop(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = ISC_R_SUCCESS;

	result = stream_listen(noop_accept_cb, NULL, 128, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	connect_readcb = NULL;
	stream_connect(connect_success_cb, NULL, T_CONNECT);
}

int
stream_noop_teardown(void **state ISC_ATTR_UNUSED) {
	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(csends, 0);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);
	return teardown_netmgr_test(state);
}

int
proxystream_noop_teardown(void **state) {
	int r = stream_noop_teardown(state);
	stream_use_PROXY = false;

	return r;
}

int
proxystreamtls_noop_teardown(void **state) {
	int r = proxystream_noop_teardown(state);
	stream_PROXY_over_TLS = false;

	return r;
}

static void
noresponse_readcb(isc_nmhandle_t *handle, isc_result_t eresult,
		  isc_region_t *region, void *cbarg) {
	UNUSED(handle);
	UNUSED(region);
	UNUSED(cbarg);

	F();

	assert_true(eresult == ISC_R_CANCELED ||
		    eresult == ISC_R_CONNECTIONRESET || eresult == ISC_R_EOF);

	isc_refcount_decrement(&active_creads);
	isc_nmhandle_detach(&handle);

	isc_loopmgr_shutdown(loopmgr);
}

static void
noresponse_sendcb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	UNUSED(cbarg);
	UNUSED(eresult);

	F();

	assert_non_null(handle);
	atomic_fetch_add(&csends, 1);
	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_csends);
}

static void
noresponse_connectcb(isc_nmhandle_t *handle, isc_result_t eresult,
		     void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;
	isc_nmhandle_t *sendhandle = NULL;

	F();

	isc_refcount_decrement(&active_cconnects);

	assert_int_equal(eresult, ISC_R_SUCCESS);

	atomic_fetch_add(&cconnects, 1);

	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, noresponse_readcb, NULL);

	isc_refcount_increment0(&active_csends);
	isc_nmhandle_attach(handle, &sendhandle);
	isc_nmhandle_setwritetimeout(handle, T_IDLE);

	isc_nm_send(handle, (isc_region_t *)&send_msg, noresponse_sendcb,
		    cbarg);
}

int
stream_noresponse_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);
	expected_cconnects = 1;
	expected_saccepts = 1;
	return r;
}

int
proxystream_noresponse_setup(void **state) {
	stream_use_PROXY = true;
	return stream_noresponse_setup(state);
}

int
proxystream_noresponse_teardown(void **state) {
	int r = stream_noresponse_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_noresponse_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_noresponse_setup(state);
}

int
proxystreamtls_noresponse_teardown(void **state) {
	int r = proxystream_noresponse_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_noresponse(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = ISC_R_SUCCESS;

	result = stream_listen(noop_accept_cb, NULL, 128, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	stream_connect(noresponse_connectcb, NULL, T_CONNECT);
}

int
stream_noresponse_teardown(void **state ISC_ATTR_UNUSED) {
	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	atomic_assert_int_eq(cconnects, 1);
	atomic_assert_int_eq(creads, 0);
	atomic_assert_int_eq(sreads, 0);
	atomic_assert_int_eq(ssends, 0);

	return teardown_netmgr_test(state);
}

int
stream_timeout_recovery_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);

	expected_ctimeouts = 4;
	ctimeouts_shutdown = false;

	expected_sreads = 5;
	sreads_shutdown = true;

	return r;
}

typedef struct proxy_addrs {
	isc_sockaddr_t src_addr;
	isc_sockaddr_t dst_addr;
} proxy_addrs_t;

static void
proxy2_handler_save_addrs_cb(const isc_result_t result,
			     const isc_proxy2_command_t cmd, const int socktype,
			     const isc_sockaddr_t *restrict src_addr,
			     const isc_sockaddr_t *restrict dst_addr,
			     const isc_region_t *restrict tlv_data,
			     const isc_region_t *restrict extra, void *cbarg) {
	proxy_addrs_t *addrs = (proxy_addrs_t *)cbarg;

	UNUSED(cmd);
	UNUSED(socktype);
	UNUSED(tlv_data);
	UNUSED(extra);

	REQUIRE(result == ISC_R_SUCCESS);

	if (src_addr != NULL) {
		addrs->src_addr = *src_addr;
	}

	if (dst_addr != NULL) {
		addrs->dst_addr = *dst_addr;
	}
}

void
proxy_verify_endpoints(isc_nmhandle_t *handle) {
	isc_sockaddr_t local, peer;
	peer = isc_nmhandle_peeraddr(handle);
	local = isc_nmhandle_localaddr(handle);

	if (isc_nm_is_proxy_unspec(handle)) {
		isc_sockaddr_t real_local, real_peer;
		real_peer = isc_nmhandle_real_peeraddr(handle);
		real_local = isc_nmhandle_real_localaddr(handle);

		assert_true(isc_sockaddr_equal(&peer, &real_peer));
		assert_true(isc_sockaddr_equal(&local, &real_local));
	} else if (proxy_info == NULL) {
		assert_true(isc_sockaddr_equal(&peer, &proxy_src));
		assert_true(isc_sockaddr_equal(&local, &proxy_dst));
	} else if (proxy_info != NULL && !proxy_info->complete) {
		assert_true(isc_sockaddr_equal(
			&peer, &proxy_info->proxy_info.src_addr));
		assert_true(isc_sockaddr_equal(
			&local, &proxy_info->proxy_info.dst_addr));
	} else if (proxy_info != NULL && proxy_info->complete) {
		proxy_addrs_t addrs = { 0 };
		RUNTIME_CHECK(isc_proxy2_header_handle_directly(
				      &proxy_info->complete_header,
				      proxy2_handler_save_addrs_cb,
				      &addrs) == ISC_R_SUCCESS);

		assert_true(isc_sockaddr_equal(&peer, &addrs.src_addr));
		assert_true(isc_sockaddr_equal(&local, &addrs.dst_addr));
	}
}

int
proxystream_timeout_recovery_setup(void **state) {
	stream_use_PROXY = true;
	return stream_timeout_recovery_setup(state);
}

int
proxystream_timeout_recovery_teardown(void **state) {
	int r = stream_timeout_recovery_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_timeout_recovery_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_timeout_recovery_setup(state);
}

int
proxystreamtls_timeout_recovery_teardown(void **state) {
	int r = proxystream_timeout_recovery_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_timeout_recovery(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = ISC_R_SUCCESS;

	/*
	 * Accept connections but don't send responses, forcing client
	 * reads to time out.
	 */
	noanswer = true;
	result = stream_listen(stream_accept_cb, NULL, 128, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	/*
	 * Shorten all the client timeouts to 0.05 seconds.
	 */
	isc_nm_settimeouts(connect_nm, T_SOFT, T_SOFT, T_SOFT, T_SOFT);
	connect_readcb = timeout_retry_cb;
	stream_connect(connect_connect_cb, NULL, T_CONNECT);
}

int
stream_timeout_recovery_teardown(void **state ISC_ATTR_UNUSED) {
	atomic_assert_int_eq(ctimeouts, expected_ctimeouts);
	return teardown_netmgr_test(state);
}

int
stream_recv_one_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);

	expected_cconnects = 1;
	cconnects_shutdown = false;

	expected_csends = 1;
	csends_shutdown = false;

	expected_saccepts = 1;
	saccepts_shutdown = false;

	expected_sreads = 1;
	sreads_shutdown = false;

	expected_ssends = 1;
	ssends_shutdown = false;

	expected_creads = 1;
	creads_shutdown = true;

	return r;
}

int
proxystream_recv_one_setup(void **state) {
	stream_use_PROXY = true;
	return stream_recv_one_setup(state);
}

int
proxystream_recv_one_teardown(void **state) {
	int r = stream_recv_one_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_recv_one_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_recv_one_setup(state);
}

int
proxystreamtls_recv_one_teardown(void **state) {
	int r = proxystream_recv_one_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_recv_one(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_quota_t *quotap = tcp_listener_init_quota(1);

	atomic_store(&nsends, 1);

	result = stream_listen(stream_accept_cb, NULL, 128, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	stream_connect(connect_connect_cb, NULL, T_CONNECT);
}

int
stream_recv_one_teardown(void **state ISC_ATTR_UNUSED) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(csends, expected_csends);
	atomic_assert_int_eq(saccepts, expected_saccepts);
	atomic_assert_int_eq(sreads, expected_sreads);
	atomic_assert_int_eq(ssends, expected_ssends);
	atomic_assert_int_eq(creads, expected_creads);

	return teardown_netmgr_test(state);
}

int
stream_recv_two_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);

	expected_cconnects = 2;
	cconnects_shutdown = false;

	expected_csends = 2;
	csends_shutdown = false;

	expected_saccepts = 2;
	saccepts_shutdown = false;

	expected_sreads = 2;
	sreads_shutdown = false;

	expected_ssends = 2;
	ssends_shutdown = false;

	expected_creads = 2;
	creads_shutdown = true;

	return r;
}

int
proxystream_recv_two_setup(void **state) {
	stream_use_PROXY = true;
	return stream_recv_two_setup(state);
}

int
proxystream_recv_two_teardown(void **state) {
	int r = stream_recv_two_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_recv_two_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_recv_two_setup(state);
}

int
proxystreamtls_recv_two_teardown(void **state) {
	int r = proxystream_recv_two_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_recv_two(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_quota_t *quotap = tcp_listener_init_quota(1);

	atomic_store(&nsends, 2);

	result = stream_listen(stream_accept_cb, NULL, 128, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	stream_connect(connect_connect_cb, NULL, T_CONNECT);

	stream_connect(connect_connect_cb, NULL, T_CONNECT);
}

int
stream_recv_two_teardown(void **state ISC_ATTR_UNUSED) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(csends, expected_csends);
	atomic_assert_int_eq(sreads, expected_saccepts);
	atomic_assert_int_eq(sreads, expected_sreads);
	atomic_assert_int_eq(ssends, expected_ssends);
	atomic_assert_int_eq(creads, expected_creads);

	return teardown_netmgr_test(state);
}

int
stream_recv_send_setup(void **state ISC_ATTR_UNUSED) {
	int r = setup_netmgr_test(state);
	expected_cconnects = workers;
	cconnects_shutdown = false;
	nsends = expected_creads = workers;
	do_send = true;

	return r;
}

int
proxystream_recv_send_setup(void **state) {
	stream_use_PROXY = true;
	return stream_recv_send_setup(state);
}

int
proxystream_recv_send_teardown(void **state) {
	int r = stream_recv_send_teardown(state);
	stream_use_PROXY = false;
	return r;
}

int
proxystreamtls_recv_send_setup(void **state) {
	stream_PROXY_over_TLS = true;
	return proxystream_recv_send_setup(state);
}

int
proxystreamtls_recv_send_teardown(void **state) {
	int r = proxystream_recv_send_teardown(state);
	stream_PROXY_over_TLS = false;
	return r;
}

void
stream_recv_send(void **state ISC_ATTR_UNUSED) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_quota_t *quotap = tcp_listener_init_quota(workers);

	result = stream_listen(stream_accept_cb, NULL, 128, quotap,
			       &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_loop_teardown(mainloop, stop_listening, listen_sock);

	for (size_t i = 0; i < workers; i++) {
		isc_async_run(isc_loop_get(loopmgr, i),
			      stream_recv_send_connect,
			      get_stream_connect_function());
	}
}

int
stream_recv_send_teardown(void **state ISC_ATTR_UNUSED) {
	X(cconnects);
	X(csends);
	X(creads);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_FULL(creads);
	CHECK_RANGE_FULL(sreads);
	CHECK_RANGE_FULL(ssends);

	return teardown_netmgr_test(state);
}

int
setup_udp_test(void **state) {
	setup_loopmgr(state);
	setup_netmgr(state);

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	udp_listen_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_listen_addr, &in6addr_loopback,
			     udp_use_PROXY ? PROXYUDP_TEST_PORT
					   : UDP_TEST_PORT);

	atomic_store(&sreads, 0);
	atomic_store(&ssends, 0);

	atomic_store(&cconnects, 0);
	atomic_store(&csends, 0);
	atomic_store(&creads, 0);
	atomic_store(&ctimeouts, 0);

	isc_refcount_init(&active_cconnects, 0);
	isc_refcount_init(&active_csends, 0);
	isc_refcount_init(&active_creads, 0);
	isc_refcount_init(&active_ssends, 0);
	isc_refcount_init(&active_sreads, 0);

	expected_cconnects = -1;
	expected_csends = -1;
	expected_creads = -1;
	expected_sreads = -1;
	expected_ssends = -1;
	expected_ctimeouts = -1;

	ssends_shutdown = true;
	sreads_shutdown = true;
	csends_shutdown = true;
	cconnects_shutdown = true;
	creads_shutdown = true;

	isc_nonce_buf(&send_magic, sizeof(send_magic));

	connect_readcb = connect_read_cb;

	return 0;
}

int
teardown_udp_test(void **state) {
	UNUSED(state);

	isc_refcount_destroy(&active_cconnects);
	isc_refcount_destroy(&active_csends);
	isc_refcount_destroy(&active_creads);
	isc_refcount_destroy(&active_ssends);
	isc_refcount_destroy(&active_sreads);

	teardown_netmgr(state);
	teardown_loopmgr(state);

	return 0;
}

static void
udp_connect(isc_nm_cb_t cb, void *cbarg, unsigned int timeout) {
	if (udp_use_PROXY) {
		isc_nm_proxyudpconnect(netmgr, &udp_connect_addr,
				       &udp_listen_addr, cb, cbarg, timeout,
				       NULL);
	} else {
		isc_nm_udpconnect(netmgr, &udp_connect_addr, &udp_listen_addr,
				  cb, cbarg, timeout);
	}
}

static void
udp_listen_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		   isc_region_t *region, void *cbarg) {
	if (eresult != ISC_R_SUCCESS) {
		isc_refcount_increment0(&active_sreads);
	}
	listen_read_cb(handle, eresult, region, cbarg);
}

static void
udp_start_listening(uint32_t nworkers, isc_nm_recv_cb_t cb) {
	isc_result_t result;

	if (udp_use_PROXY) {
		result = isc_nm_listenproxyudp(netmgr, nworkers,
					       &udp_listen_addr, cb, NULL,
					       &listen_sock);
	} else {
		result = isc_nm_listenudp(netmgr, nworkers, &udp_listen_addr,
					  cb, NULL, &listen_sock);
	}

	assert_int_equal(result, ISC_R_SUCCESS);

	isc_loop_teardown(mainloop, stop_listening, listen_sock);
}

static void
udp__send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *sendhandle = handle;

	assert_non_null(sendhandle);

	F();

	switch (eresult) {
	case ISC_R_SUCCESS:
		if (have_expected_csends(atomic_fetch_add(&csends, 1) + 1)) {
			if (csends_shutdown) {
				isc_nm_cancelread(handle);
				isc_loopmgr_shutdown(loopmgr);
			}
		}
		break;
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_nmhandle_detach(&sendhandle);
	isc_refcount_decrement(&active_csends);
}

static void
udp__connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

static void
udp_enqueue_connect(void *arg ISC_ATTR_UNUSED) {
	isc_sockaddr_t connect_addr;

	connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&connect_addr, &in6addr_loopback, 0);

	isc_refcount_increment0(&active_cconnects);

	udp_connect(udp__connect_cb, NULL, T_CONNECT);
}

static void
udp__connect_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		     isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	assert_non_null(handle);

	F();

	switch (eresult) {
	case ISC_R_TIMEDOUT:
		/*
		 * We are operating on the localhost, UDP cannot get lost, but
		 * it could be delayed, so we read again until we get the
		 * answer.
		 */
		isc_nm_read(handle, connect_readcb, cbarg);
		return;
	case ISC_R_SUCCESS:
		assert_true(region->length >= sizeof(magic));

		memmove(&magic, region->base, sizeof(magic));

		assert_true(magic == send_magic);

		if (have_expected_creads(atomic_fetch_add(&creads, 1) + 1)) {
			do_creads_shutdown(loopmgr);
		}

		if (magic == send_magic && allow_send_back) {
			connect_send(handle);
			return;
		}

		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_refcount_decrement(&active_creads);

	isc_nmhandle_detach(&handle);
}

static void
udp__connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;
	isc_nmhandle_t *sendhandle = NULL;

	F();

	isc_refcount_decrement(&active_cconnects);

	switch (eresult) {
	case ISC_R_SUCCESS:
		if (udp_use_PROXY) {
			assert_true(isc_nm_is_proxy_handle(handle));
		}

		if (have_expected_cconnects(atomic_fetch_add(&cconnects, 1) +
					    1))
		{
			do_cconnects_shutdown(loopmgr);
		} else if (do_send) {
			isc_async_current(udp_enqueue_connect, cbarg);
		}

		isc_refcount_increment0(&active_creads);
		isc_nmhandle_attach(handle, &readhandle);
		isc_nm_read(handle, connect_readcb, cbarg);

		isc_refcount_increment0(&active_csends);
		isc_nmhandle_attach(handle, &sendhandle);
		isc_nmhandle_setwritetimeout(handle, T_IDLE);

		isc_nm_send(sendhandle, (isc_region_t *)&send_msg, udp__send_cb,
			    cbarg);

		break;
	case ISC_R_ADDRINUSE:
		/* Try again */
		udp_enqueue_connect(NULL);
		break;
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}
}

int
udp_noop_setup(void **state) {
	setup_udp_test(state);
	expected_cconnects = 1;
	cconnects_shutdown = true;
	return 0;
}

int
udp_noop_teardown(void **state) {
	atomic_assert_int_eq(cconnects, 1);
	teardown_udp_test(state);
	return 0;
}

void
udp_noop(void **arg ISC_ATTR_UNUSED) {
	/* isc_result_t result = ISC_R_SUCCESS; */

	/* result = isc_nm_listenudp(netmgr, ISC_NM_LISTEN_ALL,
	 * &udp_listen_addr, */
	/* 			  mock_recv_cb, NULL, &listen_sock); */
	/* assert_int_equal(result, ISC_R_SUCCESS); */

	/* isc_nm_stoplistening(listen_sock); */
	/* isc_nmsocket_close(&listen_sock); */
	/* assert_null(listen_sock); */

	isc_refcount_increment0(&active_cconnects);
	udp_connect(connect_success_cb, NULL, UDP_T_CONNECT);
}

int
proxyudp_noop_setup(void **state) {
	udp_use_PROXY = true;
	return udp_noop_setup(state);
}

int
proxyudp_noop_teardown(void **state) {
	int ret = udp_noop_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

static void
udp_noresponse_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       isc_region_t *region, void *cbarg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(cbarg);
}

static void
udp_noresponse_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       isc_region_t *region, void *cbarg) {
	UNUSED(region);
	UNUSED(cbarg);

	assert_int_equal(eresult, ISC_R_TIMEDOUT);

	isc_refcount_decrement(&active_creads);

	atomic_fetch_add(&creads, 1);

	isc_nmhandle_detach(&handle);

	isc_loopmgr_shutdown(loopmgr);
}

static void
udp_noresponse_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       void *cbarg) {
	UNUSED(cbarg);

	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SUCCESS);
	atomic_fetch_add(&csends, 1);
	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_csends);
}

static void
udp_noresponse_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			  void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;
	isc_nmhandle_t *sendhandle = NULL;

	isc_refcount_decrement(&active_cconnects);

	assert_int_equal(eresult, ISC_R_SUCCESS);

	/* Read */
	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, udp_noresponse_read_cb, cbarg);

	/* Send */
	isc_refcount_increment0(&active_csends);
	isc_nmhandle_attach(handle, &sendhandle);
	isc_nmhandle_setwritetimeout(handle, T_IDLE);

	isc_nm_send(sendhandle, (isc_region_t *)&send_msg,
		    udp_noresponse_send_cb, cbarg);

	atomic_fetch_add(&cconnects, 1);
}

int
udp_noresponse_setup(void **state) {
	setup_udp_test(state);
	expected_csends = 1;
	return 0;
}

int
udp_noresponse_teardown(void **state) {
	atomic_assert_int_eq(csends, expected_csends);
	teardown_udp_test(state);
	return 0;
}

void
udp_noresponse(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ONE, udp_noresponse_recv_cb);

	isc_refcount_increment0(&active_cconnects);
	udp_connect(udp_noresponse_connect_cb, listen_sock, UDP_T_SOFT);
}

int
proxyudp_noresponse_setup(void **state) {
	udp_use_PROXY = true;
	return udp_noresponse_setup(state);
}

int
proxyudp_noresponse_teardown(void **state) {
	int ret = udp_noresponse_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

static void
udp_timeout_recovery_ssend_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			      void *cbarg) {
	UNUSED(cbarg);

	isc_refcount_decrement(&active_ssends);
	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SUCCESS);
	atomic_fetch_add(&ssends, 1);
	isc_nmhandle_detach(&handle);
}

static void
udp_timeout_recovery_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			     isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;
	isc_nmhandle_t *sendhandle = NULL;
	int _creads = atomic_fetch_add(&creads, 1) + 1;

	assert_non_null(handle);

	assert_int_equal(eresult, ISC_R_SUCCESS);

	assert_true(region->length == sizeof(magic));

	memmove(&magic, region->base, sizeof(magic));
	assert_true(magic == send_magic);
	assert_true(_creads < 6);

	if (_creads == 5) {
		isc_nmhandle_attach(handle, &sendhandle);
		isc_refcount_increment0(&active_ssends);
		isc_nmhandle_setwritetimeout(sendhandle, T_IDLE);
		isc_nm_send(sendhandle, (isc_region_t *)&send_msg,
			    udp_timeout_recovery_ssend_cb, cbarg);
	}
}

static void
udp_timeout_recovery_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			     isc_region_t *region, void *cbarg) {
	UNUSED(region);
	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	if (eresult == ISC_R_TIMEDOUT &&
	    atomic_fetch_add(&ctimeouts, 1) + 1 < expected_ctimeouts)
	{
		isc_nmhandle_settimeout(handle, T_SOFT);
		return;
	}

	isc_refcount_decrement(&active_creads);
	isc_nmhandle_detach(&handle);

	atomic_fetch_add(&creads, 1);
	isc_loopmgr_shutdown(loopmgr);
}

static void
udp_timeout_recovery_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			     void *cbarg) {
	UNUSED(cbarg);

	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SUCCESS);
	atomic_fetch_add(&csends, 1);

	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_csends);
}

static void
udp_timeout_recovery_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
				void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;
	isc_nmhandle_t *sendhandle = NULL;

	F();

	isc_refcount_decrement(&active_cconnects);

	assert_int_equal(eresult, ISC_R_SUCCESS);

	/* Read */
	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, udp_timeout_recovery_read_cb, cbarg);

	/* Send */
	isc_refcount_increment0(&active_csends);
	isc_nmhandle_attach(handle, &sendhandle);
	isc_nmhandle_setwritetimeout(handle, T_IDLE);
	isc_nm_send(sendhandle, (isc_region_t *)&send_msg,
		    udp_timeout_recovery_send_cb, cbarg);

	atomic_fetch_add(&cconnects, 1);
}

int
udp_timeout_recovery_setup(void **state) {
	setup_udp_test(state);
	expected_cconnects = 1;
	expected_csends = 1;
	expected_creads = 1;
	expected_ctimeouts = 4;
	return 0;
}

int
udp_timeout_recovery_teardown(void **state) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(csends, expected_csends);
	atomic_assert_int_eq(csends, expected_creads);
	atomic_assert_int_eq(ctimeouts, expected_ctimeouts);
	teardown_udp_test(state);
	return 0;
}

void
udp_timeout_recovery(void **arg ISC_ATTR_UNUSED) {
	/*
	 * Listen using the noop callback so that client reads will time out.
	 */
	udp_start_listening(ISC_NM_LISTEN_ONE, udp_timeout_recovery_recv_cb);

	/*
	 * Connect with client timeout set to 0.05 seconds, then sleep for at
	 * least a second for each 'tick'. timeout_retry_cb() will give up
	 * after five timeouts.
	 */
	isc_refcount_increment0(&active_cconnects);
	udp_connect(udp_timeout_recovery_connect_cb, listen_sock, UDP_T_SOFT);
}

int
proxyudp_timeout_recovery_setup(void **state) {
	udp_use_PROXY = true;
	return udp_timeout_recovery_setup(state);
}

int
proxyudp_timeout_recovery_teardown(void **state) {
	int ret = udp_timeout_recovery_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

static void
udp_shutdown_connect_async_cb(void *arg ISC_ATTR_UNUSED);

static void
udp_shutdown_connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
				void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	isc_refcount_decrement(&active_cconnects);

	/*
	 * The first UDP connect is faster than asynchronous shutdown procedure,
	 * restart the UDP connect again and expect the failure only in the
	 * second loop.
	 */
	if (atomic_fetch_add(&cconnects, 1) == 0) {
		assert_int_equal(eresult, ISC_R_SUCCESS);
		isc_async_current(udp_shutdown_connect_async_cb, netmgr);
	} else {
		assert_int_equal(eresult, ISC_R_SHUTTINGDOWN);
	}
}

static void
udp_shutdown_connect_async_cb(void *arg ISC_ATTR_UNUSED) {
	isc_refcount_increment0(&active_cconnects);
	udp_connect(udp_shutdown_connect_connect_cb, NULL, T_SOFT);
}

int
udp_shutdown_connect_setup(void **state) {
	setup_udp_test(state);
	expected_cconnects = 2;
	return 0;
}

int
udp_shutdown_connect_teardown(void **state) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	teardown_udp_test(state);
	return 0;
}

void
udp_shutdown_connect(void **arg ISC_ATTR_UNUSED) {
	isc_loopmgr_shutdown(loopmgr);
	/*
	 * isc_nm_udpconnect() is synchronous, so we need to launch this on the
	 * async loop.
	 */
	isc_async_current(udp_shutdown_connect_async_cb, netmgr);
}

int
proxyudp_shutdown_connect_setup(void **state) {
	udp_use_PROXY = true;
	return udp_shutdown_connect_setup(state);
}

int
proxyudp_shutdown_connect_teardown(void **state) {
	int ret = udp_shutdown_connect_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

static void
udp_shutdown_read_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			  isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	assert_int_equal(eresult, ISC_R_SUCCESS);

	assert_true(region->length == sizeof(magic));

	memmove(&magic, region->base, sizeof(magic));
	assert_true(magic == send_magic);
}

static void
udp_shutdown_read_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			  void *cbarg) {
	UNUSED(cbarg);

	F();

	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SUCCESS);

	atomic_fetch_add(&csends, 1);

	isc_loopmgr_shutdown(loopmgr);

	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_csends);
}

static void
udp_shutdown_read_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			  isc_region_t *region, void *cbarg) {
	UNUSED(region);
	UNUSED(cbarg);

	assert_true(eresult == ISC_R_SHUTTINGDOWN || eresult == ISC_R_TIMEDOUT);

	isc_refcount_decrement(&active_creads);

	atomic_fetch_add(&creads, 1);

	isc_nmhandle_detach(&handle);
}

static void
udp_shutdown_read_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			     void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;
	isc_nmhandle_t *sendhandle = NULL;

	isc_refcount_decrement(&active_cconnects);

	assert_int_equal(eresult, ISC_R_SUCCESS);

	/* Read */
	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, udp_shutdown_read_read_cb, cbarg);
	assert_true(handle->sock->reading);

	/* Send */
	isc_refcount_increment0(&active_csends);
	isc_nmhandle_attach(handle, &sendhandle);
	isc_nmhandle_setwritetimeout(handle, T_IDLE);
	isc_nm_send(sendhandle, (isc_region_t *)&send_msg,
		    udp_shutdown_read_send_cb, cbarg);

	atomic_fetch_add(&cconnects, 1);
}

int
udp_shutdown_read_setup(void **state) {
	setup_udp_test(state);
	expected_cconnects = 1;
	expected_creads = 1;
	return 0;
}

int
udp_shutdown_read_teardown(void **state) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(creads, expected_creads);
	teardown_udp_test(state);
	return 0;
}

void
udp_shutdown_read(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ONE, udp_shutdown_read_recv_cb);

	isc_refcount_increment0(&active_cconnects);
	udp_connect(udp_shutdown_read_connect_cb, NULL, UDP_T_SOFT);
}

int
proxyudp_shutdown_read_setup(void **state) {
	udp_use_PROXY = true;
	return udp_shutdown_read_setup(state);
}

int
proxyudp_shutdown_read_teardown(void **state) {
	int ret = udp_shutdown_read_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

static void
udp_cancel_read_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	F();

	assert_int_equal(eresult, ISC_R_SUCCESS);

	assert_true(region->length == sizeof(magic));

	memmove(&magic, region->base, sizeof(magic));
	assert_true(magic == send_magic);
}

static void
udp_cancel_read_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			void *cbarg) {
	UNUSED(cbarg);

	F();

	assert_non_null(handle);
	assert_int_equal(eresult, ISC_R_SUCCESS);

	atomic_fetch_add(&csends, 1);

	isc_nm_cancelread(handle);

	isc_nmhandle_detach(&handle);
	isc_refcount_decrement(&active_csends);
}

static void
udp_cancel_read_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			isc_region_t *region, void *cbarg) {
	isc_nmhandle_t *sendhandle = NULL;
	isc_nmhandle_t *readhandle = NULL;

	UNUSED(region);

	F();

	switch (eresult) {
	case ISC_R_TIMEDOUT:

		/* Read again */
		isc_refcount_increment0(&active_creads);
		isc_nmhandle_attach(handle, &readhandle);
		isc_nm_read(handle, udp_cancel_read_read_cb, cbarg);

		/* Send only once */
		if (isc_refcount_increment0(&active_csends) == 0) {
			isc_nmhandle_attach(handle, &sendhandle);
			isc_nmhandle_setwritetimeout(handle, T_IDLE);
			isc_nm_send(sendhandle, (isc_region_t *)&send_msg,
				    udp_cancel_read_send_cb, cbarg);
		}
		break;
	case ISC_R_CANCELED:
		/* The read has been canceled */
		atomic_fetch_add(&creads, 1);
		isc_loopmgr_shutdown(loopmgr);
		break;
	default:
		UNREACHABLE();
	}

	isc_refcount_decrement(&active_creads);

	isc_nmhandle_detach(&handle);
}

static void
udp_cancel_read_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			   void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	isc_refcount_decrement(&active_cconnects);

	assert_int_equal(eresult, ISC_R_SUCCESS);

	isc_refcount_increment0(&active_creads);
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, udp_cancel_read_read_cb, cbarg);

	atomic_fetch_add(&cconnects, 1);
}

int
udp_cancel_read_setup(void **state) {
	setup_udp_test(state);
	expected_cconnects = 1;
	expected_creads = 1;
	return 0;
}

int
udp_cancel_read_teardown(void **state) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(creads, expected_creads);
	teardown_udp_test(state);
	return 0;
}

void
udp_cancel_read(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ONE, udp_cancel_read_recv_cb);

	isc_refcount_increment0(&active_cconnects);
	udp_connect(udp_cancel_read_connect_cb, NULL, UDP_T_SOFT);
}

int
proxyudp_cancel_read_setup(void **state) {
	udp_use_PROXY = true;
	return udp_cancel_read_setup(state);
}

int
proxyudp_cancel_read_teardown(void **state) {
	int ret = udp_cancel_read_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

int
udp_recv_one_setup(void **state) {
	setup_udp_test(state);

	connect_readcb = udp__connect_read_cb;

	expected_cconnects = 1;
	cconnects_shutdown = false;

	expected_csends = 1;
	csends_shutdown = false;

	expected_sreads = 1;
	sreads_shutdown = false;

	expected_ssends = 1;
	ssends_shutdown = false;

	expected_creads = 1;
	creads_shutdown = true;

	return 0;
}

int
udp_recv_one_teardown(void **state) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(csends, expected_csends);
	atomic_assert_int_eq(sreads, expected_sreads);
	atomic_assert_int_eq(ssends, expected_ssends);
	atomic_assert_int_eq(creads, expected_creads);

	teardown_udp_test(state);

	return 0;
}

void
udp_recv_one(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ONE, udp_listen_read_cb);

	udp_enqueue_connect(NULL);
}

int
proxyudp_recv_one_setup(void **state) {
	udp_use_PROXY = true;
	return udp_recv_one_setup(state);
}

int
proxyudp_recv_one_teardown(void **state) {
	int ret = udp_recv_one_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

int
udp_recv_two_setup(void **state) {
	setup_udp_test(state);

	connect_readcb = udp__connect_read_cb;

	expected_cconnects = 2;
	cconnects_shutdown = false;

	expected_csends = 2;
	csends_shutdown = false;

	expected_sreads = 2;
	sreads_shutdown = false;

	expected_ssends = 2;
	ssends_shutdown = false;

	expected_creads = 2;
	creads_shutdown = true;

	return 0;
}

int
udp_recv_two_teardown(void **state) {
	atomic_assert_int_eq(cconnects, expected_cconnects);
	atomic_assert_int_eq(csends, expected_csends);
	atomic_assert_int_eq(sreads, expected_sreads);
	atomic_assert_int_eq(ssends, expected_ssends);
	atomic_assert_int_eq(creads, expected_creads);

	teardown_udp_test(state);
	return 0;
}

void
udp_recv_two(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ONE, udp_listen_read_cb);

	udp_enqueue_connect(NULL);
	udp_enqueue_connect(NULL);
}

int
proxyudp_recv_two_setup(void **state) {
	udp_use_PROXY = true;
	return udp_recv_two_setup(state);
}

int
proxyudp_recv_two_teardown(void **state) {
	int ret = udp_recv_two_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

int
udp_recv_send_setup(void **state) {
	setup_udp_test(state);

	/* Allow some leeway (+1) as datagram service is unreliable */
	expected_cconnects = (workers + 1) * NSENDS;
	cconnects_shutdown = false;

	expected_creads = workers * NSENDS;
	do_send = true;

	return 0;
}

int
udp_recv_send_teardown(void **state) {
	atomic_assert_int_ge(cconnects, expected_creads);
	atomic_assert_int_ge(csends, expected_creads);
	atomic_assert_int_ge(sreads, expected_creads);
	atomic_assert_int_ge(ssends, expected_creads);
	atomic_assert_int_ge(creads, expected_creads);

	teardown_udp_test(state);
	return 0;
}

void
udp_recv_send(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ALL, udp_listen_read_cb);

	for (size_t i = 0; i < workers; i++) {
		isc_async_run(isc_loop_get(loopmgr, i), udp_enqueue_connect,
			      NULL);
	}
}

int
proxyudp_recv_send_setup(void **state) {
	udp_use_PROXY = true;
	return udp_recv_send_setup(state);
}

int
proxyudp_recv_send_teardown(void **state) {
	int ret = udp_recv_send_teardown(state);
	udp_use_PROXY = false;
	return ret;
}

static void
udp_double_read_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			void *cbarg) {
	assert_non_null(handle);

	F();

	isc_refcount_decrement(&active_ssends);

	switch (eresult) {
	case ISC_R_SUCCESS:
		if (have_expected_ssends(atomic_fetch_add(&ssends, 1) + 1)) {
			do_ssends_shutdown(loopmgr);
		} else {
			isc_nmhandle_t *sendhandle = NULL;
			isc_nmhandle_attach(handle, &sendhandle);
			isc_nmhandle_setwritetimeout(sendhandle, T_IDLE);
			isc_refcount_increment0(&active_ssends);
			isc_nm_send(sendhandle, &send_msg,
				    udp_double_read_send_cb, cbarg);
			break;
		}
		break;
	case ISC_R_CANCELED:
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_nmhandle_detach(&handle);
}

static void
udp_double_read_listen_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			  isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	assert_non_null(handle);

	F();

	switch (eresult) {
	case ISC_R_EOF:
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
		break;
	case ISC_R_SUCCESS:
		memmove(&magic, region->base, sizeof(magic));
		assert_true(magic == send_magic);

		assert_true(region->length >= sizeof(magic));

		memmove(&magic, region->base, sizeof(magic));
		assert_true(magic == send_magic);

		isc_nmhandle_t *sendhandle = NULL;
		isc_nmhandle_attach(handle, &sendhandle);
		isc_nmhandle_setwritetimeout(sendhandle, T_IDLE);
		isc_refcount_increment0(&active_ssends);
		isc_nm_send(sendhandle, &send_msg, udp_double_read_send_cb,
			    cbarg);
		return;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	isc_refcount_decrement(&active_sreads);

	isc_nmhandle_detach(&handle);
}

static void
udp_double_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		   isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;
	bool detach = false;

	assert_non_null(handle);

	F();

	switch (eresult) {
	case ISC_R_TIMEDOUT:
		/*
		 * We are operating on the localhost, UDP cannot get lost, but
		 * it could be delayed, so we read again until we get the
		 * answer.
		 */
		detach = false;
		break;
	case ISC_R_SUCCESS:
		assert_true(region->length >= sizeof(magic));

		memmove(&magic, region->base, sizeof(magic));

		assert_true(magic == send_magic);

		if (have_expected_creads(atomic_fetch_add(&creads, 1) + 1)) {
			do_creads_shutdown(loopmgr);
			detach = true;
		}

		if (magic == send_magic && allow_send_back) {
			connect_send(handle);
			return;
		}

		break;
	case ISC_R_EOF:
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
	case ISC_R_CONNECTIONRESET:
		detach = true;
		break;
	default:
		fprintf(stderr, "%s(%p, %s, %p)\n", __func__, handle,
			isc_result_totext(eresult), cbarg);
		assert_int_equal(eresult, ISC_R_SUCCESS);
	}

	if (detach) {
		isc_refcount_decrement(&active_creads);
		isc_nmhandle_detach(&handle);
	} else {
		isc_nm_read(handle, connect_readcb, cbarg);
	}
}

int
udp_double_read_setup(void **state) {
	setup_udp_test(state);

	expected_cconnects = 1;
	cconnects_shutdown = false;

	expected_csends = 1;
	csends_shutdown = false;

	expected_sreads = 1;
	sreads_shutdown = false;

	expected_ssends = 2;
	ssends_shutdown = false;
	expected_creads = 2;
	creads_shutdown = true;

	connect_readcb = udp_double_read_cb;

	return 0;
}

int
udp_double_read_teardown(void **state) {
	atomic_assert_int_eq(creads, expected_creads);

	teardown_udp_test(state);

	return 0;
}

void
udp_double_read(void **arg ISC_ATTR_UNUSED) {
	udp_start_listening(ISC_NM_LISTEN_ALL, udp_double_read_listen_cb);

	udp_enqueue_connect(NULL);
}

int
proxyudp_double_read_setup(void **state) {
	udp_use_PROXY = true;
	return udp_double_read_setup(state);
}

int
proxyudp_double_read_teardown(void **state) {
	int ret = udp_double_read_teardown(state);
	udp_use_PROXY = false;
	return ret;
}
