/*	$NetBSD: udp_test.c,v 1.1.1.1 2021/02/19 16:37:17 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
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
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/mutex.h>
#include <isc/netmgr.h>
#include <isc/nonce.h>
#include <isc/os.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>

#include "uv_wrap.h"
#define KEEP_BEFORE

#include "../netmgr/netmgr-int.h"
#include "../netmgr/udp.c"
#include "../netmgr/uv-compat.c"
#include "../netmgr/uv-compat.h"
#include "isctest.h"

#define MAX_NM 2

static isc_sockaddr_t udp_listen_addr;

static uint64_t send_magic = 0;
static uint64_t stop_magic = 0;

static uv_buf_t send_msg = { .base = (char *)&stop_magic,
			     .len = sizeof(stop_magic) };

static uv_buf_t stop_msg = { .base = (char *)&stop_magic,
			     .len = sizeof(stop_magic) };

static atomic_uint_fast64_t nsends;

static atomic_uint_fast64_t ssends;
static atomic_uint_fast64_t sreads;

static atomic_uint_fast64_t cconnects;
static atomic_uint_fast64_t csends;
static atomic_uint_fast64_t creads;
static atomic_uint_fast64_t ctimeouts;

static unsigned int workers = 3;

#define NSENDS	100
#define NWRITES 10

/*
 * The UDP protocol doesn't protect against packet duplication, but instead of
 * inventing de-duplication, we just ignore the upper bound.
 */

#define CHECK_RANGE_FULL(v)                                             \
	{                                                               \
		int __v = atomic_load(&v);                              \
		assert_true(NSENDS *NWRITES * 20 / 100 <= __v);         \
		/* assert_true(__v <= NSENDS * NWRITES * 110 / 100); */ \
	}

#define CHECK_RANGE_HALF(v)                                            \
	{                                                              \
		int __v = atomic_load(&v);                             \
		assert_true(NSENDS *NWRITES * 10 / 100 <= __v);        \
		/* assert_true(__v <= NSENDS * NWRITES * 60 / 100); */ \
	}

/* Enable this to print values while running tests */
#undef PRINT_DEBUG
#ifdef PRINT_DEBUG
#define X(v) fprintf(stderr, #v " = %" PRIu64 "\n", atomic_load(&v))
#else
#define X(v)
#endif

/* MOCK */

static int
setup_ephemeral_port(isc_sockaddr_t *addr, sa_family_t family) {
	isc_result_t result;
	socklen_t addrlen = sizeof(*addr);
	int fd;
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

	result = isc__nm_socket_reuse(fd);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTIMPLEMENTED) {
		fprintf(stderr,
			"setup_ephemeral_port: isc__nm_socket_reuse(): %s",
			isc_result_totext(result));
		close(fd);
		return (-1);
	}

	result = isc__nm_socket_reuse_lb(fd);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTIMPLEMENTED) {
		fprintf(stderr,
			"setup_ephemeral_port: isc__nm_socket_reuse_lb(): %s",
			isc_result_totext(result));
		close(fd);
		return (-1);
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

static int
_setup(void **state) {
	UNUSED(state);

	/* workers = isc_os_ncpus(); */

	if (isc_test_begin(NULL, true, workers) != ISC_R_SUCCESS) {
		return (-1);
	}

	signal(SIGPIPE, SIG_IGN);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();

	return (0);
}

/* Generic */

static void
noop_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	     void *cbarg) {
	UNUSED(handle);
	UNUSED(eresult);
	UNUSED(region);
	UNUSED(cbarg);
}

static void
noop_connect_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	UNUSED(handle);
	UNUSED(result);
	UNUSED(cbarg);
}

static int
nm_setup(void **state) {
	size_t nworkers = ISC_MAX(ISC_MIN(workers, 32), 1);
	int udp_listen_sock = -1;
	isc_nm_t **nm = NULL;

	udp_listen_addr = (isc_sockaddr_t){ .length = 0 };
	udp_listen_sock = setup_ephemeral_port(&udp_listen_addr, SOCK_DGRAM);
	if (udp_listen_sock < 0) {
		return (-1);
	}
	close(udp_listen_sock);
	udp_listen_sock = -1;
	/* silence scan-build */
	UNUSED(udp_listen_sock);

	atomic_store(&nsends, NSENDS * NWRITES);

	atomic_store(&csends, 0);
	atomic_store(&creads, 0);
	atomic_store(&sreads, 0);
	atomic_store(&ssends, 0);
	atomic_store(&ctimeouts, 0);
	atomic_store(&cconnects, 0);

	isc_nonce_buf(&send_magic, sizeof(send_magic));
	isc_nonce_buf(&stop_magic, sizeof(stop_magic));
	if (send_magic == stop_magic) {
		return (-1);
	}

	nm = isc_mem_get(test_mctx, MAX_NM * sizeof(nm[0]));
	for (size_t i = 0; i < MAX_NM; i++) {
		nm[i] = isc_nm_start(test_mctx, nworkers);
		assert_non_null(nm[i]);
	}

	*state = nm;

	return (0);
}

static int
nm_teardown(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;

	for (size_t i = 0; i < MAX_NM; i++) {
		isc_nm_destroy(&nm[i]);
		assert_null(nm[i]);
	}
	isc_mem_put(test_mctx, nm, MAX_NM * sizeof(nm[0]));

	return (0);
}

/* UDP */

static void
udp_listen_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	assert_non_null(handle);
	UNUSED(cbarg);

	if (eresult == ISC_R_SUCCESS) {
		atomic_fetch_add(&ssends, 1);
	}
}

static void
udp_listen_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		   isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	assert_null(cbarg);

	if (eresult != ISC_R_SUCCESS) {
		return;
	}

	assert_int_equal(region->length, sizeof(send_magic));
	atomic_fetch_add(&sreads, 1);
	magic = *(uint64_t *)region->base;

	assert_true(magic == stop_magic || magic == send_magic);
	isc_nm_send(handle, region, udp_listen_send_cb, NULL);
}

static void
mock_listenudp_uv_udp_open(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_udp_open, UV_ENOMEM);

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  noop_recv_cb, NULL, 0, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;
}

static void
mock_listenudp_uv_udp_bind(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_udp_bind, UV_EADDRINUSE);

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  noop_recv_cb, NULL, 0, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;
}

static void
mock_listenudp_uv_udp_recv_start(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_udp_recv_start, UV_EADDRINUSE);

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  noop_recv_cb, NULL, 0, &listen_sock);
	assert_int_not_equal(result, ISC_R_SUCCESS);
	assert_null(listen_sock);

	RESET_RETURN;
}

static void
mock_udpconnect_uv_udp_open(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_udp_open, UV_ENOMEM);

	result = isc_nm_udpconnect(connect_nm,
				   (isc_nmiface_t *)&udp_connect_addr,
				   (isc_nmiface_t *)&udp_listen_addr,
				   noop_connect_cb, NULL, 1000, 0);
	assert_int_not_equal(result, ISC_R_SUCCESS);

	isc_nm_closedown(connect_nm);

	RESET_RETURN;
}

static void
mock_udpconnect_uv_udp_bind(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_udp_bind, UV_ENOMEM);

	result = isc_nm_udpconnect(connect_nm,
				   (isc_nmiface_t *)&udp_connect_addr,
				   (isc_nmiface_t *)&udp_listen_addr,
				   noop_connect_cb, NULL, 1000, 0);
	assert_int_not_equal(result, ISC_R_SUCCESS);

	isc_nm_closedown(connect_nm);

	RESET_RETURN;
}

#if HAVE_UV_UDP_CONNECT
static void
mock_udpconnect_uv_udp_connect(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_udp_connect, UV_ENOMEM);

	result = isc_nm_udpconnect(connect_nm,
				   (isc_nmiface_t *)&udp_connect_addr,
				   (isc_nmiface_t *)&udp_listen_addr,
				   noop_connect_cb, NULL, 1000, 0);
	assert_int_not_equal(result, ISC_R_SUCCESS);

	isc_nm_closedown(connect_nm);

	RESET_RETURN;
}
#endif

static void
mock_udpconnect_uv_recv_buffer_size(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_recv_buffer_size, UV_ENOMEM);

	result = isc_nm_udpconnect(connect_nm,
				   (isc_nmiface_t *)&udp_connect_addr,
				   (isc_nmiface_t *)&udp_listen_addr,
				   noop_connect_cb, NULL, 1000, 0);
	assert_int_equal(result, ISC_R_SUCCESS); /* FIXME: should fail */

	isc_nm_closedown(connect_nm);

	RESET_RETURN;
}

static void
mock_udpconnect_uv_send_buffer_size(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	WILL_RETURN(uv_send_buffer_size, UV_ENOMEM);

	result = isc_nm_udpconnect(connect_nm,
				   (isc_nmiface_t *)&udp_connect_addr,
				   (isc_nmiface_t *)&udp_listen_addr,
				   noop_connect_cb, NULL, 1000, 0);
	assert_int_equal(result, ISC_R_SUCCESS); /* FIXME: should fail */

	isc_nm_closedown(connect_nm);

	RESET_RETURN;
}

static void
udp_noop(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  noop_recv_cb, NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	(void)isc_nm_udpconnect(connect_nm, (isc_nmiface_t *)&udp_connect_addr,
				(isc_nmiface_t *)&udp_listen_addr,
				noop_connect_cb, NULL, 1000, 0);

	isc_nm_closedown(connect_nm);

	assert_int_equal(0, atomic_load(&cconnects));
	assert_int_equal(0, atomic_load(&csends));
	assert_int_equal(0, atomic_load(&creads));
	assert_int_equal(0, atomic_load(&ctimeouts));
	assert_int_equal(0, atomic_load(&sreads));
	assert_int_equal(0, atomic_load(&ssends));
}

static void
udp_connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);
static void
udp_connect_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		    isc_region_t *region, void *cbarg);

static void
udp_connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	assert_non_null(handle);

	UNUSED(eresult);
	UNUSED(cbarg);

	atomic_fetch_add(&csends, 1);
}

static void
udp_connect_send(isc_nmhandle_t *handle, isc_region_t *region) {
	uint_fast64_t sends = atomic_load(&nsends);

	while (sends > 0) {
		/* Continue until we subtract or we are done */
		if (atomic_compare_exchange_weak(&nsends, &sends, sends - 1)) {
			break;
		}
	}

	isc_nm_send(handle, region, udp_connect_send_cb, NULL);
}

static void
udp_connect_recv_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		    isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	if (eresult != ISC_R_SUCCESS) {
		goto unref;
	}

	assert_int_equal(region->length, sizeof(magic));

	atomic_fetch_add(&creads, 1);

	magic = *(uint64_t *)region->base;

	assert_true(magic == stop_magic || magic == send_magic);

	if (magic == stop_magic) {
		goto unref;
	}

	if (isc_random_uniform(NWRITES) == 0) {
		udp_connect_send(handle, (isc_region_t *)&stop_msg);
	} else {
		udp_connect_send(handle, (isc_region_t *)&send_msg);
	}
unref:
	isc_nmhandle_detach(&handle);
}

static void
udp_connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	UNUSED(cbarg);

	if (eresult != ISC_R_SUCCESS) {
		uint_fast64_t sends = atomic_load(&nsends);

		/* We failed to connect; try again */
		while (sends > 0) {
			/* Continue until we subtract or we are done */
			if (atomic_compare_exchange_weak(&nsends, &sends,
							 sends - 1)) {
				break;
			}
		}
		return;
	}

	atomic_fetch_add(&cconnects, 1);

	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, udp_connect_recv_cb, NULL);

	udp_connect_send(handle, (isc_region_t *)&send_msg);
}

static isc_threadresult_t
udp_connect_thread(isc_threadarg_t arg) {
	isc_nm_t *connect_nm = (isc_nm_t *)arg;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	while (atomic_load(&nsends) > 0) {
		(void)isc_nm_udpconnect(connect_nm,
					(isc_nmiface_t *)&udp_connect_addr,
					(isc_nmiface_t *)&udp_listen_addr,
					udp_connect_connect_cb, NULL, 1000, 0);
	}
	return ((isc_threadresult_t)0);
}

static void
udp_noresponse(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t udp_connect_addr;

	udp_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&udp_connect_addr, &in6addr_loopback, 0);

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  noop_recv_cb, NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	(void)isc_nm_udpconnect(connect_nm, (isc_nmiface_t *)&udp_connect_addr,
				(isc_nmiface_t *)&udp_listen_addr,
				udp_connect_connect_cb, NULL, 1000, 0);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);
	isc_nm_closedown(connect_nm);

	while (atomic_load(&cconnects) != 1) {
		isc_thread_yield();
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(ctimeouts);
	X(sreads);
	X(ssends);

	assert_int_equal(1, atomic_load(&cconnects));
	assert_true(atomic_load(&csends) <= 1);
	assert_int_equal(0, atomic_load(&creads));
	assert_int_equal(0, atomic_load(&ctimeouts));
	assert_int_equal(0, atomic_load(&sreads));
	assert_int_equal(0, atomic_load(&ssends));
}

static void
udp_recv_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  udp_listen_recv_cb, NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(udp_connect_thread, connect_nm, &threads[i]);
	}

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	isc_nm_closedown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(ctimeouts);
	X(sreads);
	X(ssends);

	assert_true(atomic_load(&cconnects) >= (NSENDS - 1) * NWRITES);
	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_FULL(creads);
	CHECK_RANGE_FULL(sreads);
	CHECK_RANGE_FULL(ssends);
}

static void
udp_recv_half_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  udp_listen_recv_cb, NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(udp_connect_thread, connect_nm, &threads[i]);
	}

	while (atomic_load(&nsends) >= (NSENDS * NWRITES) / 2) {
		isc_thread_yield();
	}

	isc_nm_closedown(connect_nm);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(ctimeouts);
	X(sreads);
	X(ssends);

	assert_true(atomic_load(&cconnects) >= (NSENDS - 1) * NWRITES);
	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
udp_half_recv_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  udp_listen_recv_cb, NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(udp_connect_thread, connect_nm, &threads[i]);
	}

	while (atomic_load(&nsends) >= (NSENDS * NWRITES) / 2) {
		isc_thread_yield();
	}

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_closedown(connect_nm);

	X(cconnects);
	X(csends);
	X(creads);
	X(ctimeouts);
	X(sreads);
	X(ssends);

	assert_true(atomic_load(&cconnects) >= (NSENDS - 1) * NWRITES);
	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
udp_half_recv_half_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	result = isc_nm_listenudp(listen_nm, (isc_nmiface_t *)&udp_listen_addr,
				  udp_listen_recv_cb, NULL, 0, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(udp_connect_thread, connect_nm, &threads[i]);
	}

	while (atomic_load(&nsends) >= (NSENDS * NWRITES) / 2) {
		isc_thread_yield();
	}

	isc_nm_closedown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	X(cconnects);
	X(csends);
	X(creads);
	X(ctimeouts);
	X(sreads);
	X(ssends);

	assert_true(atomic_load(&cconnects) >= (NSENDS - 1) * NWRITES);
	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
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
#if HAVE_UV_UDP_CONNECT
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
		cmocka_unit_test_setup_teardown(udp_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_recv_half_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_half_recv_send, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(udp_half_recv_half_send,
						nm_setup, nm_teardown),
	};

	return (cmocka_run_group_tests(tests, _setup, _teardown));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif /* if HAVE_CMOCKA */
