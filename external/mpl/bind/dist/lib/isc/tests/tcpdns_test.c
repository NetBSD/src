/*	$NetBSD: tcpdns_test.c,v 1.3 2021/04/29 17:26:13 christos Exp $	*/

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

#include "../netmgr/netmgr-int.h"
#include "isctest.h"

#define MAX_NM 2

static isc_sockaddr_t tcpdns_listen_addr;

static uint64_t send_magic = 0;
static uint64_t stop_magic = 0;

static uv_buf_t send_msg = { .base = (char *)&send_magic,
			     .len = sizeof(send_magic) };

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

static bool reuse_supported = true;

#define NSENDS	100
#define NWRITES 10

#define CHECK_RANGE_FULL(v)                                       \
	{                                                         \
		int __v = atomic_load(&v);                        \
		assert_true(__v > NSENDS * NWRITES * 10 / 100);   \
		assert_true(__v <= NSENDS * NWRITES * 110 / 100); \
	}

#define CHECK_RANGE_HALF(v)                                      \
	{                                                        \
		int __v = atomic_load(&v);                       \
		assert_true(__v > NSENDS * NWRITES * 5 / 100);   \
		assert_true(__v <= NSENDS * NWRITES * 60 / 100); \
	}

/* Enable this to print values while running tests */
#undef PRINT_DEBUG
#ifdef PRINT_DEBUG
#define X(v) fprintf(stderr, #v " = %" PRIu64 "\n", atomic_load(&v))
#else
#define X(v)
#endif

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
	if (result == ISC_R_NOTIMPLEMENTED) {
		reuse_supported = false;
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

static unsigned int
noop_accept_cb(isc_nmhandle_t *handle, unsigned int result, void *cbarg) {
	UNUSED(handle);
	UNUSED(result);
	UNUSED(cbarg);

	return (0);
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
	int tcpdns_listen_sock = -1;
	isc_nm_t **nm = NULL;

	tcpdns_listen_addr = (isc_sockaddr_t){ .length = 0 };
	tcpdns_listen_sock = setup_ephemeral_port(&tcpdns_listen_addr,
						  SOCK_STREAM);
	if (tcpdns_listen_sock < 0) {
		return (-1);
	}
	close(tcpdns_listen_sock);
	tcpdns_listen_sock = -1;
	/* silence scan-build */
	UNUSED(tcpdns_listen_sock);

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

/* TCPDNS */

static void
tcpdns_connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       void *cbarg);

static void
tcpdns_connect_send(isc_nmhandle_t *handle);

static void
tcpdns_connect_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       void *cbarg) {
	assert_non_null(handle);

	UNUSED(cbarg);

	if (eresult == ISC_R_SUCCESS) {
		atomic_fetch_add(&csends, 1);
	} else {
		/* Send failed, we need to stop reading too */
		isc_nm_cancelread(handle);
	}
}

static void
tcpdns_connect_send(isc_nmhandle_t *handle) {
	uint_fast64_t sends = atomic_load(&nsends);

	/* Continue until we subtract or we are sent them all */
	while (sends > 0) {
		if (atomic_compare_exchange_weak(&nsends, &sends, sends - 1)) {
			sends--;
			break;
		}
	}

	if (sends == 0) {
		isc_nm_send(handle, (isc_region_t *)&stop_msg,
			    tcpdns_connect_send_cb, NULL);
	} else {
		isc_nm_send(handle, (isc_region_t *)&send_msg,
			    tcpdns_connect_send_cb, NULL);
	}
}

static void
tcpdns_connect_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		       isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	if (eresult != ISC_R_SUCCESS) {
		goto unref;
	}

	assert_int_equal(region->length, sizeof(magic));

	atomic_fetch_add(&creads, 1);

	memmove(&magic, region->base, sizeof(magic));

	assert_true(magic == stop_magic || magic == send_magic);

unref:
	isc_nmhandle_detach(&handle);
}

static void
tcpdns_connect_connect_cb(isc_nmhandle_t *handle, isc_result_t eresult,
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
				sends--;
				break;
			}
		}
		return;
	}

	atomic_fetch_add(&cconnects, 1);

	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, tcpdns_connect_read_cb, NULL);

	tcpdns_connect_send(handle);
}

static void
tcpdns_noop(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t tcpdns_connect_addr;

	tcpdns_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcpdns_connect_addr, &in6addr_loopback, 0);

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr, noop_recv_cb,
		NULL, noop_accept_cb, NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	(void)isc_nm_tcpdnsconnect(connect_nm,
				   (isc_nmiface_t *)&tcpdns_connect_addr,
				   (isc_nmiface_t *)&tcpdns_listen_addr,
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
tcpdns_noresponse(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t tcpdns_connect_addr;

	tcpdns_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcpdns_connect_addr, &in6addr_loopback, 0);

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr, noop_recv_cb,
		NULL, noop_accept_cb, NULL, 0, 0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	(void)isc_nm_tcpdnsconnect(connect_nm,
				   (isc_nmiface_t *)&tcpdns_connect_addr,
				   (isc_nmiface_t *)&tcpdns_listen_addr,
				   tcpdns_connect_connect_cb, NULL, 1000, 0);

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

	assert_true(atomic_load(&cconnects) <= 1);
	assert_true(atomic_load(&csends) <= 1);
	assert_int_equal(0, atomic_load(&creads));
	assert_int_equal(0, atomic_load(&ctimeouts));
	assert_int_equal(0, atomic_load(&sreads));
	assert_int_equal(0, atomic_load(&ssends));
}

static void
tcpdns_listen_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		      isc_region_t *region, void *cbarg);

static void
tcpdns_listen_send_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		      void *cbarg) {
	UNUSED(cbarg);
	UNUSED(eresult);

	assert_non_null(handle);

	if (eresult != ISC_R_SUCCESS) {
		return;
	}

	atomic_fetch_add(&ssends, 1);
}

static void
tcpdns_listen_read_cb(isc_nmhandle_t *handle, isc_result_t eresult,
		      isc_region_t *region, void *cbarg) {
	uint64_t magic = 0;

	UNUSED(cbarg);

	assert_non_null(handle);

	if (eresult != ISC_R_SUCCESS) {
		return;
	}

	atomic_fetch_add(&sreads, 1);

	assert_int_equal(region->length, sizeof(magic));

	memmove(&magic, region->base, sizeof(magic));
	assert_true(magic == stop_magic || magic == send_magic);

	if (magic == send_magic) {
		isc_nm_send(handle, region, tcpdns_listen_send_cb, NULL);
		return;
	} else if (magic == stop_magic) {
		/* We are done, we don't send anything back */
	}
}

static isc_result_t
tcpdns_listen_accept_cb(isc_nmhandle_t *handle, isc_result_t eresult,
			void *cbarg) {
	UNUSED(handle);
	UNUSED(cbarg);

	return (eresult);
}

static isc_threadresult_t
tcpdns_connect_thread(isc_threadarg_t arg) {
	isc_nm_t *connect_nm = (isc_nm_t *)arg;
	isc_sockaddr_t tcpdns_connect_addr;

	tcpdns_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcpdns_connect_addr, &in6addr_loopback, 0);

	while (atomic_load(&nsends) > 0) {
		(void)isc_nm_tcpdnsconnect(
			connect_nm, (isc_nmiface_t *)&tcpdns_connect_addr,
			(isc_nmiface_t *)&tcpdns_listen_addr,
			tcpdns_connect_connect_cb, NULL, 1000, 0);
	}

	return ((isc_threadresult_t)0);
}

static void
tcpdns_recv_one(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t tcpdns_connect_addr;

	tcpdns_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcpdns_connect_addr, &in6addr_loopback, 0);

	atomic_store(&nsends, 1);

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr,
		tcpdns_listen_read_cb, NULL, tcpdns_listen_accept_cb, NULL, 0,
		0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	(void)isc_nm_tcpdnsconnect(connect_nm,
				   (isc_nmiface_t *)&tcpdns_connect_addr,
				   (isc_nmiface_t *)&tcpdns_listen_addr,
				   tcpdns_connect_connect_cb, NULL, 1000, 0);

	while (atomic_load(&nsends) > 0) {
		isc_thread_yield();
	}

	while (atomic_load(&cconnects) != 1 || atomic_load(&ssends) != 0 ||
	       atomic_load(&sreads) != 1 || atomic_load(&creads) != 0 ||
	       atomic_load(&csends) != 1)
	{
		isc_thread_yield();
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

	assert_int_equal(atomic_load(&cconnects), 1);
	assert_int_equal(atomic_load(&csends), 1);
	assert_int_equal(atomic_load(&creads), 0);
	assert_int_equal(atomic_load(&ctimeouts), 0);
	assert_int_equal(atomic_load(&sreads), 1);
	assert_int_equal(atomic_load(&ssends), 0);
}

static void
tcpdns_recv_two(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	isc_sockaddr_t tcpdns_connect_addr;

	tcpdns_connect_addr = (isc_sockaddr_t){ .length = 0 };
	isc_sockaddr_fromin6(&tcpdns_connect_addr, &in6addr_loopback, 0);

	atomic_store(&nsends, 2);

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr,
		tcpdns_listen_read_cb, NULL, tcpdns_listen_accept_cb, NULL, 0,
		0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_nm_tcpdnsconnect(connect_nm,
				      (isc_nmiface_t *)&tcpdns_connect_addr,
				      (isc_nmiface_t *)&tcpdns_listen_addr,
				      tcpdns_connect_connect_cb, NULL, 1000, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_nm_settimeouts(connect_nm, 1000, 1000, 1000, 1000);

	result = isc_nm_tcpdnsconnect(connect_nm,
				      (isc_nmiface_t *)&tcpdns_connect_addr,
				      (isc_nmiface_t *)&tcpdns_listen_addr,
				      tcpdns_connect_connect_cb, NULL, 1000, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	while (atomic_load(&nsends) > 0) {
		isc_thread_yield();
	}

	while (atomic_load(&sreads) != 2 || atomic_load(&ssends) != 1 ||
	       atomic_load(&csends) != 2 || atomic_load(&creads) != 1)
	{
		isc_thread_yield();
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

	assert_int_equal(atomic_load(&cconnects), 2);
	assert_int_equal(atomic_load(&csends), 2);
	assert_int_equal(atomic_load(&creads), 1);
	assert_int_equal(atomic_load(&ctimeouts), 0);
	assert_int_equal(atomic_load(&sreads), 2);
	assert_int_equal(atomic_load(&ssends), 1);
}

static void
tcpdns_recv_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	if (!reuse_supported) {
		skip();
		return;
	}

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr,
		tcpdns_listen_read_cb, NULL, tcpdns_listen_accept_cb, NULL, 0,
		0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(tcpdns_connect_thread, connect_nm,
				  &threads[i]);
	}

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	isc_nm_closedown(connect_nm);
	isc_nm_stoplistening(listen_sock);
	isc_nmsocket_close(&listen_sock);
	assert_null(listen_sock);

	X(cconnects);
	X(csends);
	X(creads);
	X(ctimeouts);
	X(sreads);
	X(ssends);

	CHECK_RANGE_FULL(csends);
	CHECK_RANGE_FULL(creads);
	CHECK_RANGE_FULL(sreads);
	CHECK_RANGE_FULL(ssends);
}

static void
tcpdns_recv_half_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	if (!reuse_supported) {
		skip();
		return;
	}

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr,
		tcpdns_listen_read_cb, NULL, tcpdns_listen_accept_cb, NULL, 0,
		0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(tcpdns_connect_thread, connect_nm,
				  &threads[i]);
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

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
tcpdns_half_recv_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	if (!reuse_supported) {
		skip();
		return;
	}

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr,
		tcpdns_listen_read_cb, NULL, tcpdns_listen_accept_cb, NULL, 0,
		0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(tcpdns_connect_thread, connect_nm,
				  &threads[i]);
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

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

static void
tcpdns_half_recv_half_send(void **state) {
	isc_nm_t **nm = (isc_nm_t **)*state;
	isc_nm_t *listen_nm = nm[0];
	isc_nm_t *connect_nm = nm[1];
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *listen_sock = NULL;
	size_t nthreads = ISC_MAX(ISC_MIN(workers, 32), 1);
	isc_thread_t threads[32] = { 0 };

	if (!reuse_supported) {
		skip();
		return;
	}

	result = isc_nm_listentcpdns(
		listen_nm, (isc_nmiface_t *)&tcpdns_listen_addr,
		tcpdns_listen_read_cb, NULL, tcpdns_listen_accept_cb, NULL, 0,
		0, NULL, &listen_sock);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nthreads; i++) {
		isc_thread_create(tcpdns_connect_thread, connect_nm,
				  &threads[i]);
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

	CHECK_RANGE_HALF(csends);
	CHECK_RANGE_HALF(creads);
	CHECK_RANGE_HALF(sreads);
	CHECK_RANGE_HALF(ssends);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(tcpdns_recv_one, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_recv_two, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_noop, nm_setup,
						nm_teardown),
		cmocka_unit_test_setup_teardown(tcpdns_noresponse, nm_setup,
						nm_teardown),
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
