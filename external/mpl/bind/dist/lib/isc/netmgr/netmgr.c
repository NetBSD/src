/*	$NetBSD: netmgr.c,v 1.9 2023/01/25 21:43:31 christos Exp $	*/

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
#include <unistd.h>
#include <uv.h>
#ifdef HAVE_LIBCTRACE
#include <execinfo.h>
#endif /* ifdef HAVE_LIBCTRACE */

#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/errno.h>
#include <isc/list.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/print.h>
#include <isc/quota.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/stats.h>
#include <isc/strerr.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr-int.h"
#include "netmgr_p.h"
#include "openssl_shim.h"
#include "trampoline_p.h"
#include "uv-compat.h"

/*%
 * How many isc_nmhandles and isc_nm_uvreqs will we be
 * caching for reuse in a socket.
 */
#define ISC_NM_HANDLES_STACK_SIZE 600
#define ISC_NM_REQS_STACK_SIZE	  600

/*%
 * Shortcut index arrays to get access to statistics counters.
 */

static const isc_statscounter_t udp4statsindex[] = {
	isc_sockstatscounter_udp4open,
	isc_sockstatscounter_udp4openfail,
	isc_sockstatscounter_udp4close,
	isc_sockstatscounter_udp4bindfail,
	isc_sockstatscounter_udp4connectfail,
	isc_sockstatscounter_udp4connect,
	-1,
	-1,
	isc_sockstatscounter_udp4sendfail,
	isc_sockstatscounter_udp4recvfail,
	isc_sockstatscounter_udp4active
};

static const isc_statscounter_t udp6statsindex[] = {
	isc_sockstatscounter_udp6open,
	isc_sockstatscounter_udp6openfail,
	isc_sockstatscounter_udp6close,
	isc_sockstatscounter_udp6bindfail,
	isc_sockstatscounter_udp6connectfail,
	isc_sockstatscounter_udp6connect,
	-1,
	-1,
	isc_sockstatscounter_udp6sendfail,
	isc_sockstatscounter_udp6recvfail,
	isc_sockstatscounter_udp6active
};

static const isc_statscounter_t tcp4statsindex[] = {
	isc_sockstatscounter_tcp4open,	      isc_sockstatscounter_tcp4openfail,
	isc_sockstatscounter_tcp4close,	      isc_sockstatscounter_tcp4bindfail,
	isc_sockstatscounter_tcp4connectfail, isc_sockstatscounter_tcp4connect,
	isc_sockstatscounter_tcp4acceptfail,  isc_sockstatscounter_tcp4accept,
	isc_sockstatscounter_tcp4sendfail,    isc_sockstatscounter_tcp4recvfail,
	isc_sockstatscounter_tcp4active
};

static const isc_statscounter_t tcp6statsindex[] = {
	isc_sockstatscounter_tcp6open,	      isc_sockstatscounter_tcp6openfail,
	isc_sockstatscounter_tcp6close,	      isc_sockstatscounter_tcp6bindfail,
	isc_sockstatscounter_tcp6connectfail, isc_sockstatscounter_tcp6connect,
	isc_sockstatscounter_tcp6acceptfail,  isc_sockstatscounter_tcp6accept,
	isc_sockstatscounter_tcp6sendfail,    isc_sockstatscounter_tcp6recvfail,
	isc_sockstatscounter_tcp6active
};

#if 0
/* XXX: not currently used */
static const isc_statscounter_t unixstatsindex[] = {
	isc_sockstatscounter_unixopen,
	isc_sockstatscounter_unixopenfail,
	isc_sockstatscounter_unixclose,
	isc_sockstatscounter_unixbindfail,
	isc_sockstatscounter_unixconnectfail,
	isc_sockstatscounter_unixconnect,
	isc_sockstatscounter_unixacceptfail,
	isc_sockstatscounter_unixaccept,
	isc_sockstatscounter_unixsendfail,
	isc_sockstatscounter_unixrecvfail,
	isc_sockstatscounter_unixactive
};
#endif /* if 0 */

/*
 * libuv is not thread safe, but has mechanisms to pass messages
 * between threads. Each socket is owned by a thread. For UDP
 * sockets we have a set of sockets for each interface and we can
 * choose a sibling and send the message directly. For TCP, or if
 * we're calling from a non-networking thread, we need to pass the
 * request using async_cb.
 */

#if defined(HAVE_THREAD_LOCAL)
#include <threads.h>
static thread_local int isc__nm_tid_v = ISC_NETMGR_TID_UNKNOWN;
#elif defined(HAVE___THREAD)
static __thread int isc__nm_tid_v = ISC_NETMGR_TID_UNKNOWN;
#elif HAVE___DECLSPEC_THREAD
__declspec(thread) int isc__nm_tid_v = ISC_NETMGR_TID_UNKNOWN;
#endif /* if defined(HAVE_THREAD_LOCAL) */

static void
nmsocket_maybe_destroy(isc_nmsocket_t *sock FLARG);
static void
nmhandle_free(isc_nmsocket_t *sock, isc_nmhandle_t *handle);
static isc_threadresult_t
nm_thread(isc_threadarg_t worker0);
static void
async_cb(uv_async_t *handle);

static bool
process_netievent(isc__networker_t *worker, isc__netievent_t *ievent);
static isc_result_t
process_queue(isc__networker_t *worker, netievent_type_t type);
static void
wait_for_priority_queue(isc__networker_t *worker);
static void
drain_queue(isc__networker_t *worker, netievent_type_t type);

static void
isc__nm_async_stop(isc__networker_t *worker, isc__netievent_t *ev0);
static void
isc__nm_async_pause(isc__networker_t *worker, isc__netievent_t *ev0);
static void
isc__nm_async_resume(isc__networker_t *worker, isc__netievent_t *ev0);
static void
isc__nm_async_detach(isc__networker_t *worker, isc__netievent_t *ev0);
static void
isc__nm_async_close(isc__networker_t *worker, isc__netievent_t *ev0);

static void
isc__nm_threadpool_initialize(uint32_t workers);
static void
isc__nm_work_cb(uv_work_t *req);
static void
isc__nm_after_work_cb(uv_work_t *req, int status);

void
isc__nmsocket_reset(isc_nmsocket_t *sock);

/*%<
 * Issue a 'handle closed' callback on the socket.
 */

static void
nmhandle_detach_cb(isc_nmhandle_t **handlep FLARG);

int
isc_nm_tid(void) {
	return (isc__nm_tid_v);
}

bool
isc__nm_in_netthread(void) {
	return (isc__nm_tid_v >= 0);
}

#ifdef WIN32
static void
isc__nm_winsock_initialize(void) {
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int result;

	result = WSAStartup(wVersionRequested, &wsaData);
	if (result != 0) {
		char strbuf[ISC_STRERRORSIZE];
		strerror_r(result, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "WSAStartup() failed with error code %lu: %s",
				 result, strbuf);
	}

	/*
	 * Confirm that the WinSock DLL supports version 2.2.  Note that if the
	 * DLL supports versions greater than 2.2 in addition to 2.2, it will
	 * still return 2.2 in wVersion since that is the version we requested.
	 */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "Unusable WinSock DLL version: %u.%u",
				 LOBYTE(wsaData.wVersion),
				 HIBYTE(wsaData.wVersion));
	}
}

static void
isc__nm_winsock_destroy(void) {
	WSACleanup();
}
#endif /* WIN32 */

static void
isc__nm_threadpool_initialize(uint32_t workers) {
	char buf[11];
	int r = uv_os_getenv("UV_THREADPOOL_SIZE", buf,
			     &(size_t){ sizeof(buf) });
	if (r == UV_ENOENT) {
		snprintf(buf, sizeof(buf), "%" PRIu32, workers);
		uv_os_setenv("UV_THREADPOOL_SIZE", buf);
	}
}

#if HAVE_DECL_UV_UDP_LINUX_RECVERR
#define MINIMAL_UV_VERSION UV_VERSION(1, 42, 0)
#elif HAVE_DECL_UV_UDP_MMSG_FREE
#define MINIMAL_UV_VERSION UV_VERSION(1, 40, 0)
#elif HAVE_DECL_UV_UDP_RECVMMSG
#define MINIMAL_UV_VERSION UV_VERSION(1, 37, 0)
#elif HAVE_DECL_UV_UDP_MMSG_CHUNK
#define MINIMAL_UV_VERSION UV_VERSION(1, 35, 0)
#else
#define MINIMAL_UV_VERSION UV_VERSION(1, 0, 0)
#endif

void
isc__netmgr_create(isc_mem_t *mctx, uint32_t workers, isc_nm_t **netmgrp) {
	isc_nm_t *mgr = NULL;
	char name[32];

	REQUIRE(workers > 0);

	if (uv_version() < MINIMAL_UV_VERSION) {
		isc_error_fatal(__FILE__, __LINE__,
				"libuv version too old: running with libuv %s "
				"when compiled with libuv %s will lead to "
				"libuv failures because of unknown flags",
				uv_version_string(), UV_VERSION_STRING);
	}

#ifdef WIN32
	isc__nm_winsock_initialize();
#endif /* WIN32 */

	isc__nm_threadpool_initialize(workers);

	mgr = isc_mem_get(mctx, sizeof(*mgr));
	*mgr = (isc_nm_t){ .nworkers = workers };

	isc_mem_attach(mctx, &mgr->mctx);
	isc_mutex_init(&mgr->lock);
	isc_condition_init(&mgr->wkstatecond);
	isc_condition_init(&mgr->wkpausecond);
	isc_refcount_init(&mgr->references, 1);
	atomic_init(&mgr->maxudp, 0);
	atomic_init(&mgr->interlocked, ISC_NETMGR_NON_INTERLOCKED);
	atomic_init(&mgr->workers_paused, 0);
	atomic_init(&mgr->paused, false);
	atomic_init(&mgr->closing, false);
#if HAVE_SO_REUSEPORT_LB
	mgr->load_balance_sockets = true;
#else
	mgr->load_balance_sockets = false;
#endif

#ifdef NETMGR_TRACE
	ISC_LIST_INIT(mgr->active_sockets);
#endif

	/*
	 * Default TCP timeout values.
	 * May be updated by isc_nm_tcptimeouts().
	 */
	atomic_init(&mgr->init, 30000);
	atomic_init(&mgr->idle, 30000);
	atomic_init(&mgr->keepalive, 30000);
	atomic_init(&mgr->advertised, 30000);

	isc_barrier_init(&mgr->pausing, workers);
	isc_barrier_init(&mgr->resuming, workers);

	mgr->workers = isc_mem_get(mctx, workers * sizeof(isc__networker_t));
	for (size_t i = 0; i < workers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		int r;

		*worker = (isc__networker_t){
			.mgr = mgr,
			.id = i,
		};

		r = uv_loop_init(&worker->loop);
		UV_RUNTIME_CHECK(uv_loop_init, r);

		worker->loop.data = &mgr->workers[i];

		r = uv_async_init(&worker->loop, &worker->async, async_cb);
		UV_RUNTIME_CHECK(uv_async_init, r);

		for (size_t type = 0; type < NETIEVENT_MAX; type++) {
			isc_mutex_init(&worker->ievents[type].lock);
			isc_condition_init(&worker->ievents[type].cond);
			ISC_LIST_INIT(worker->ievents[type].list);
		}

		worker->recvbuf = isc_mem_get(mctx, ISC_NETMGR_RECVBUF_SIZE);
		worker->sendbuf = isc_mem_get(mctx, ISC_NETMGR_SENDBUF_SIZE);

		/*
		 * We need to do this here and not in nm_thread to avoid a
		 * race - we could exit isc_nm_start, launch nm_destroy,
		 * and nm_thread would still not be up.
		 */
		mgr->workers_running++;
		isc_thread_create(nm_thread, &mgr->workers[i], &worker->thread);

		snprintf(name, sizeof(name), "net-%zu", i);
		isc_thread_setname(worker->thread, name);
	}

	mgr->magic = NM_MAGIC;
	*netmgrp = mgr;
}

/*
 * Free the resources of the network manager.
 */
static void
nm_destroy(isc_nm_t **mgr0) {
	REQUIRE(VALID_NM(*mgr0));
	REQUIRE(!isc__nm_in_netthread());

	isc_nm_t *mgr = *mgr0;
	*mgr0 = NULL;

	isc_refcount_destroy(&mgr->references);

	mgr->magic = 0;

	for (int i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		isc__netievent_t *event = isc__nm_get_netievent_stop(mgr);
		isc__nm_enqueue_ievent(worker, event);
	}

	LOCK(&mgr->lock);
	while (mgr->workers_running > 0) {
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);

	for (int i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		int r;

		r = uv_loop_close(&worker->loop);
		UV_RUNTIME_CHECK(uv_loop_close, r);

		for (size_t type = 0; type < NETIEVENT_MAX; type++) {
			INSIST(ISC_LIST_EMPTY(worker->ievents[type].list));
			isc_condition_destroy(&worker->ievents[type].cond);
			isc_mutex_destroy(&worker->ievents[type].lock);
		}

		isc_mem_put(mgr->mctx, worker->sendbuf,
			    ISC_NETMGR_SENDBUF_SIZE);
		isc_mem_put(mgr->mctx, worker->recvbuf,
			    ISC_NETMGR_RECVBUF_SIZE);
		isc_thread_join(worker->thread, NULL);
	}

	if (mgr->stats != NULL) {
		isc_stats_detach(&mgr->stats);
	}

	isc_barrier_destroy(&mgr->resuming);
	isc_barrier_destroy(&mgr->pausing);

	isc_condition_destroy(&mgr->wkstatecond);
	isc_condition_destroy(&mgr->wkpausecond);
	isc_mutex_destroy(&mgr->lock);

	isc_mem_put(mgr->mctx, mgr->workers,
		    mgr->nworkers * sizeof(isc__networker_t));
	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(*mgr));

#ifdef WIN32
	isc__nm_winsock_destroy();
#endif /* WIN32 */
}

static void
enqueue_pause(isc__networker_t *worker) {
	isc__netievent_pause_t *event =
		isc__nm_get_netievent_pause(worker->mgr);
	isc__nm_enqueue_ievent(worker, (isc__netievent_t *)event);
}

static void
isc__nm_async_pause(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	REQUIRE(worker->paused == false);

	worker->paused = true;
	uv_stop(&worker->loop);
}

void
isc_nm_pause(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(!atomic_load(&mgr->paused));

	isc__nm_acquire_interlocked_force(mgr);

	if (isc__nm_in_netthread()) {
		REQUIRE(isc_nm_tid() == 0);
	}

	for (int i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		if (i == isc_nm_tid()) {
			isc__nm_async_pause(worker, NULL);
		} else {
			enqueue_pause(worker);
		}
	}

	if (isc__nm_in_netthread()) {
		atomic_fetch_add(&mgr->workers_paused, 1);
		isc_barrier_wait(&mgr->pausing);
	}

	LOCK(&mgr->lock);
	while (atomic_load(&mgr->workers_paused) != mgr->workers_running) {
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);

	REQUIRE(atomic_compare_exchange_strong(&mgr->paused, &(bool){ false },
					       true));
}

static void
enqueue_resume(isc__networker_t *worker) {
	isc__netievent_resume_t *event =
		isc__nm_get_netievent_resume(worker->mgr);
	isc__nm_enqueue_ievent(worker, (isc__netievent_t *)event);
}

static void
isc__nm_async_resume(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	REQUIRE(worker->paused == true);

	worker->paused = false;
}

void
isc_nm_resume(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(atomic_load(&mgr->paused));

	if (isc__nm_in_netthread()) {
		REQUIRE(isc_nm_tid() == 0);
		drain_queue(&mgr->workers[isc_nm_tid()], NETIEVENT_PRIORITY);
	}

	for (int i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		if (i == isc_nm_tid()) {
			isc__nm_async_resume(worker, NULL);
		} else {
			enqueue_resume(worker);
		}
	}

	if (isc__nm_in_netthread()) {
		drain_queue(&mgr->workers[isc_nm_tid()], NETIEVENT_PRIVILEGED);

		atomic_fetch_sub(&mgr->workers_paused, 1);
		isc_barrier_wait(&mgr->resuming);
	}

	LOCK(&mgr->lock);
	while (atomic_load(&mgr->workers_paused) != 0) {
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);

	REQUIRE(atomic_compare_exchange_strong(&mgr->paused, &(bool){ true },
					       false));

	isc__nm_drop_interlocked(mgr);
}

void
isc_nm_attach(isc_nm_t *mgr, isc_nm_t **dst) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(dst != NULL && *dst == NULL);

	isc_refcount_increment(&mgr->references);

	*dst = mgr;
}

void
isc_nm_detach(isc_nm_t **mgr0) {
	isc_nm_t *mgr = NULL;

	REQUIRE(mgr0 != NULL);
	REQUIRE(VALID_NM(*mgr0));

	mgr = *mgr0;
	*mgr0 = NULL;

	if (isc_refcount_decrement(&mgr->references) == 1) {
		nm_destroy(&mgr);
	}
}

void
isc__netmgr_shutdown(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));

	atomic_store(&mgr->closing, true);
	for (int i = 0; i < mgr->nworkers; i++) {
		isc__netievent_t *event = NULL;
		event = isc__nm_get_netievent_shutdown(mgr);
		isc__nm_enqueue_ievent(&mgr->workers[i], event);
	}
}

void
isc__netmgr_destroy(isc_nm_t **netmgrp) {
	isc_nm_t *mgr = NULL;
	int counter = 0;

	REQUIRE(VALID_NM(*netmgrp));

	mgr = *netmgrp;

	/*
	 * Close active connections.
	 */
	isc__netmgr_shutdown(mgr);

	/*
	 * Wait for the manager to be dereferenced elsewhere.
	 */
	while (isc_refcount_current(&mgr->references) > 1 && counter++ < 1000) {
		uv_sleep(10);
	}

#ifdef NETMGR_TRACE
	if (isc_refcount_current(&mgr->references) > 1) {
		isc__nm_dump_active(mgr);
		UNREACHABLE();
	}
#endif

	/*
	 * Now just patiently wait
	 */
	while (isc_refcount_current(&mgr->references) > 1) {
		uv_sleep(10);
	}

	/*
	 * Detach final reference.
	 */
	isc_nm_detach(netmgrp);
}

void
isc_nm_maxudp(isc_nm_t *mgr, uint32_t maxudp) {
	REQUIRE(VALID_NM(mgr));

	atomic_store(&mgr->maxudp, maxudp);
}

void
isc_nmhandle_setwritetimeout(isc_nmhandle_t *handle, uint64_t write_timeout) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	handle->sock->write_timeout = write_timeout;
}

void
isc_nm_settimeouts(isc_nm_t *mgr, uint32_t init, uint32_t idle,
		   uint32_t keepalive, uint32_t advertised) {
	REQUIRE(VALID_NM(mgr));

	atomic_store(&mgr->init, init);
	atomic_store(&mgr->idle, idle);
	atomic_store(&mgr->keepalive, keepalive);
	atomic_store(&mgr->advertised, advertised);
}

bool
isc_nm_getloadbalancesockets(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));

	return (mgr->load_balance_sockets);
}

void
isc_nm_setloadbalancesockets(isc_nm_t *mgr, bool enabled) {
	REQUIRE(VALID_NM(mgr));

#if HAVE_SO_REUSEPORT_LB
	mgr->load_balance_sockets = enabled;
#else
	UNUSED(enabled);
#endif
}

void
isc_nm_gettimeouts(isc_nm_t *mgr, uint32_t *initial, uint32_t *idle,
		   uint32_t *keepalive, uint32_t *advertised) {
	REQUIRE(VALID_NM(mgr));

	if (initial != NULL) {
		*initial = atomic_load(&mgr->init);
	}

	if (idle != NULL) {
		*idle = atomic_load(&mgr->idle);
	}

	if (keepalive != NULL) {
		*keepalive = atomic_load(&mgr->keepalive);
	}

	if (advertised != NULL) {
		*advertised = atomic_load(&mgr->advertised);
	}
}

/*
 * nm_thread is a single worker thread, that runs uv_run event loop
 * until asked to stop.
 *
 * There are four queues for asynchronous events:
 *
 * 1. priority queue - netievents on the priority queue are run even when
 *    the taskmgr enters exclusive mode and the netmgr is paused.  This
 *    is needed to properly start listening on the interfaces, free
 *    resources on shutdown, or resume from a pause.
 *
 * 2. privileged task queue - only privileged tasks are queued here and
 *    this is the first queue that gets processed when network manager
 *    is unpaused using isc_nm_resume().  All netmgr workers need to
 *    clean the privileged task queue before they all proceed to normal
 *    operation.  Both task queues are processed when the workers are
 *    shutting down.
 *
 * 3. task queue - only (traditional) tasks are scheduled here, and this
 *    queue and the privileged task queue are both processed when the
 *    netmgr workers are finishing.  This is needed to process the task
 *    shutdown events.
 *
 * 4. normal queue - this is the queue with netmgr events, e.g. reading,
 *    sending, callbacks, etc.
 */

static isc_threadresult_t
nm_thread(isc_threadarg_t worker0) {
	isc__networker_t *worker = (isc__networker_t *)worker0;
	isc_nm_t *mgr = worker->mgr;

	isc__nm_tid_v = worker->id;

	while (true) {
		/*
		 * uv_run() runs async_cb() in a loop, which processes
		 * all four event queues until a "pause" or "stop" event
		 * is encountered. On pause, we process only priority and
		 * privileged events until resuming.
		 */
		int r = uv_run(&worker->loop, UV_RUN_DEFAULT);
		INSIST(r > 0 || worker->finished);

		if (worker->paused) {
			INSIST(atomic_load(&mgr->interlocked) != isc_nm_tid());

			atomic_fetch_add(&mgr->workers_paused, 1);
			if (isc_barrier_wait(&mgr->pausing) != 0) {
				LOCK(&mgr->lock);
				SIGNAL(&mgr->wkstatecond);
				UNLOCK(&mgr->lock);
			}

			while (worker->paused) {
				wait_for_priority_queue(worker);
			}

			/*
			 * All workers must drain the privileged event
			 * queue before we resume from pause.
			 */
			drain_queue(worker, NETIEVENT_PRIVILEGED);

			atomic_fetch_sub(&mgr->workers_paused, 1);
			if (isc_barrier_wait(&mgr->resuming) != 0) {
				LOCK(&mgr->lock);
				SIGNAL(&mgr->wkstatecond);
				UNLOCK(&mgr->lock);
			}
		}

		if (r == 0) {
			INSIST(worker->finished);
			break;
		}

		INSIST(!worker->finished);
	}

	/*
	 * We are shutting down.  Drain the queues.
	 */
	drain_queue(worker, NETIEVENT_PRIVILEGED);
	drain_queue(worker, NETIEVENT_TASK);

	for (size_t type = 0; type < NETIEVENT_MAX; type++) {
		LOCK(&worker->ievents[type].lock);
		INSIST(ISC_LIST_EMPTY(worker->ievents[type].list));
		UNLOCK(&worker->ievents[type].lock);
	}

	LOCK(&mgr->lock);
	mgr->workers_running--;
	SIGNAL(&mgr->wkstatecond);
	UNLOCK(&mgr->lock);

	return ((isc_threadresult_t)0);
}

static bool
process_all_queues(isc__networker_t *worker) {
	bool reschedule = false;
	/*
	 * The queue processing functions will return false when the
	 * system is pausing or stopping and we don't want to process
	 * the other queues in such case, but we need the async event
	 * to be rescheduled in the next uv_run().
	 */
	for (size_t type = 0; type < NETIEVENT_MAX; type++) {
		isc_result_t result = process_queue(worker, type);
		switch (result) {
		case ISC_R_SUSPEND:
			reschedule = true;
			break;
		case ISC_R_EMPTY:
			/* empty queue */
			break;
		case ISC_R_SUCCESS:
			reschedule = true;
			break;
		default:
			UNREACHABLE();
		}
	}

	return (reschedule);
}

/*
 * async_cb() is a universal callback for 'async' events sent to event loop.
 * It's the only way to safely pass data to the libuv event loop. We use a
 * single async event and a set of lockless queues of 'isc__netievent_t'
 * structures passed from other threads.
 */
static void
async_cb(uv_async_t *handle) {
	isc__networker_t *worker = (isc__networker_t *)handle->loop->data;

	if (process_all_queues(worker)) {
		/*
		 * If we didn't process all the events, we need to enqueue
		 * async_cb to be run in the next iteration of the uv_loop
		 */
		uv_async_send(handle);
	}
}

static void
isc__nm_async_stop(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	worker->finished = true;
	/* Close the async handler */
	uv_close((uv_handle_t *)&worker->async, NULL);
}

void
isc_nm_task_enqueue(isc_nm_t *nm, isc_task_t *task, int threadid) {
	isc__netievent_t *event = NULL;
	int tid;
	isc__networker_t *worker = NULL;

	if (threadid == -1) {
		tid = (int)isc_random_uniform(nm->nworkers);
	} else {
		tid = threadid % nm->nworkers;
	}

	worker = &nm->workers[tid];

	if (isc_task_privileged(task)) {
		event = (isc__netievent_t *)
			isc__nm_get_netievent_privilegedtask(nm, task);
	} else {
		event = (isc__netievent_t *)isc__nm_get_netievent_task(nm,
								       task);
	}

	isc__nm_enqueue_ievent(worker, event);
}

#define isc__nm_async_privilegedtask(worker, ev0) \
	isc__nm_async_task(worker, ev0)

static void
isc__nm_async_task(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_task_t *ievent = (isc__netievent_task_t *)ev0;
	isc_result_t result;

	UNUSED(worker);

	result = isc_task_run(ievent->task);

	switch (result) {
	case ISC_R_QUOTA:
		isc_task_ready(ievent->task);
		return;
	case ISC_R_SUCCESS:
		return;
	default:
		UNREACHABLE();
	}
}

static void
wait_for_priority_queue(isc__networker_t *worker) {
	isc_condition_t *cond = &worker->ievents[NETIEVENT_PRIORITY].cond;
	isc_mutex_t *lock = &worker->ievents[NETIEVENT_PRIORITY].lock;
	isc__netievent_list_t *list =
		&(worker->ievents[NETIEVENT_PRIORITY].list);

	LOCK(lock);
	while (ISC_LIST_EMPTY(*list)) {
		WAIT(cond, lock);
	}
	UNLOCK(lock);

	drain_queue(worker, NETIEVENT_PRIORITY);
}

static void
drain_queue(isc__networker_t *worker, netievent_type_t type) {
	bool empty = false;
	while (!empty) {
		if (process_queue(worker, type) == ISC_R_EMPTY) {
			LOCK(&worker->ievents[type].lock);
			empty = ISC_LIST_EMPTY(worker->ievents[type].list);
			UNLOCK(&worker->ievents[type].lock);
		}
	}
}

/*
 * The two macros here generate the individual cases for the process_netievent()
 * function.  The NETIEVENT_CASE(type) macro is the common case, and
 * NETIEVENT_CASE_NOMORE(type) is a macro that causes the loop in the
 * process_queue() to stop, e.g. it's only used for the netievent that
 * stops/pauses processing the enqueued netievents.
 */
#define NETIEVENT_CASE(type)                                               \
	case netievent_##type: {                                           \
		isc__nm_async_##type(worker, ievent);                      \
		isc__nm_put_netievent_##type(                              \
			worker->mgr, (isc__netievent_##type##_t *)ievent); \
		return (true);                                             \
	}

#define NETIEVENT_CASE_NOMORE(type)                                \
	case netievent_##type: {                                   \
		isc__nm_async_##type(worker, ievent);              \
		isc__nm_put_netievent_##type(worker->mgr, ievent); \
		return (false);                                    \
	}

static bool
process_netievent(isc__networker_t *worker, isc__netievent_t *ievent) {
	REQUIRE(worker->id == isc_nm_tid());

	switch (ievent->type) {
		/* Don't process more ievents when we are stopping */
		NETIEVENT_CASE_NOMORE(stop);

		NETIEVENT_CASE(privilegedtask);
		NETIEVENT_CASE(task);

		NETIEVENT_CASE(udpconnect);
		NETIEVENT_CASE(udplisten);
		NETIEVENT_CASE(udpstop);
		NETIEVENT_CASE(udpsend);
		NETIEVENT_CASE(udpread);
		NETIEVENT_CASE(udpcancel);
		NETIEVENT_CASE(udpclose);

		NETIEVENT_CASE(tcpaccept);
		NETIEVENT_CASE(tcpconnect);
		NETIEVENT_CASE(tcplisten);
		NETIEVENT_CASE(tcpstartread);
		NETIEVENT_CASE(tcppauseread);
		NETIEVENT_CASE(tcpsend);
		NETIEVENT_CASE(tcpstop);
		NETIEVENT_CASE(tcpcancel);
		NETIEVENT_CASE(tcpclose);

		NETIEVENT_CASE(tcpdnsaccept);
		NETIEVENT_CASE(tcpdnslisten);
		NETIEVENT_CASE(tcpdnsconnect);
		NETIEVENT_CASE(tcpdnssend);
		NETIEVENT_CASE(tcpdnscancel);
		NETIEVENT_CASE(tcpdnsclose);
		NETIEVENT_CASE(tcpdnsread);
		NETIEVENT_CASE(tcpdnsstop);

		NETIEVENT_CASE(connectcb);
		NETIEVENT_CASE(readcb);
		NETIEVENT_CASE(sendcb);

		NETIEVENT_CASE(close);
		NETIEVENT_CASE(detach);

		NETIEVENT_CASE(shutdown);
		NETIEVENT_CASE(resume);
		NETIEVENT_CASE_NOMORE(pause);
	default:
		UNREACHABLE();
	}
	return (true);
}

static isc_result_t
process_queue(isc__networker_t *worker, netievent_type_t type) {
	isc__netievent_t *ievent = NULL;
	isc__netievent_list_t list;

	ISC_LIST_INIT(list);

	LOCK(&worker->ievents[type].lock);
	ISC_LIST_MOVE(list, worker->ievents[type].list);
	UNLOCK(&worker->ievents[type].lock);

	ievent = ISC_LIST_HEAD(list);
	if (ievent == NULL) {
		/* There's nothing scheduled */
		return (ISC_R_EMPTY);
	}

	while (ievent != NULL) {
		isc__netievent_t *next = ISC_LIST_NEXT(ievent, link);
		ISC_LIST_DEQUEUE(list, ievent, link);

		if (!process_netievent(worker, ievent)) {
			/* The netievent told us to stop */
			if (!ISC_LIST_EMPTY(list)) {
				/*
				 * Reschedule the rest of the unprocessed
				 * events.
				 */
				LOCK(&worker->ievents[type].lock);
				ISC_LIST_PREPENDLIST(worker->ievents[type].list,
						     list, link);
				UNLOCK(&worker->ievents[type].lock);
			}
			return (ISC_R_SUSPEND);
		}

		ievent = next;
	}

	/* We processed at least one */
	return (ISC_R_SUCCESS);
}

void *
isc__nm_get_netievent(isc_nm_t *mgr, isc__netievent_type type) {
	isc__netievent_storage_t *event = isc_mem_get(mgr->mctx,
						      sizeof(*event));

	*event = (isc__netievent_storage_t){ .ni.type = type };
	ISC_LINK_INIT(&(event->ni), link);
	return (event);
}

void
isc__nm_put_netievent(isc_nm_t *mgr, void *ievent) {
	isc_mem_put(mgr->mctx, ievent, sizeof(isc__netievent_storage_t));
}

NETIEVENT_SOCKET_DEF(tcpclose);
NETIEVENT_SOCKET_DEF(tcplisten);
NETIEVENT_SOCKET_DEF(tcppauseread);
NETIEVENT_SOCKET_DEF(tcpstartread);
NETIEVENT_SOCKET_DEF(tcpstop);
NETIEVENT_SOCKET_DEF(udpclose);
NETIEVENT_SOCKET_DEF(udplisten);
NETIEVENT_SOCKET_DEF(udpread);
NETIEVENT_SOCKET_DEF(udpsend);
NETIEVENT_SOCKET_DEF(udpstop);

NETIEVENT_SOCKET_DEF(tcpdnsclose);
NETIEVENT_SOCKET_DEF(tcpdnsread);
NETIEVENT_SOCKET_DEF(tcpdnsstop);
NETIEVENT_SOCKET_DEF(tcpdnslisten);
NETIEVENT_SOCKET_REQ_DEF(tcpdnsconnect);
NETIEVENT_SOCKET_REQ_DEF(tcpdnssend);
NETIEVENT_SOCKET_HANDLE_DEF(tcpdnscancel);
NETIEVENT_SOCKET_QUOTA_DEF(tcpdnsaccept);

NETIEVENT_SOCKET_REQ_DEF(tcpconnect);
NETIEVENT_SOCKET_REQ_DEF(tcpsend);
NETIEVENT_SOCKET_REQ_DEF(udpconnect);
NETIEVENT_SOCKET_REQ_RESULT_DEF(connectcb);
NETIEVENT_SOCKET_REQ_RESULT_DEF(readcb);
NETIEVENT_SOCKET_REQ_RESULT_DEF(sendcb);

NETIEVENT_SOCKET_DEF(detach);
NETIEVENT_SOCKET_HANDLE_DEF(tcpcancel);
NETIEVENT_SOCKET_HANDLE_DEF(udpcancel);

NETIEVENT_SOCKET_QUOTA_DEF(tcpaccept);

NETIEVENT_SOCKET_DEF(close);
NETIEVENT_DEF(pause);
NETIEVENT_DEF(resume);
NETIEVENT_DEF(shutdown);
NETIEVENT_DEF(stop);

NETIEVENT_TASK_DEF(task);
NETIEVENT_TASK_DEF(privilegedtask);

void
isc__nm_maybe_enqueue_ievent(isc__networker_t *worker,
			     isc__netievent_t *event) {
	/*
	 * If we are already in the matching nmthread, process the ievent
	 * directly.
	 */
	if (worker->id == isc_nm_tid()) {
		process_netievent(worker, event);
		return;
	}

	isc__nm_enqueue_ievent(worker, event);
}

void
isc__nm_enqueue_ievent(isc__networker_t *worker, isc__netievent_t *event) {
	netievent_type_t type;

	if (event->type > netievent_prio) {
		type = NETIEVENT_PRIORITY;
	} else {
		switch (event->type) {
		case netievent_prio:
			UNREACHABLE();
			break;
		case netievent_privilegedtask:
			type = NETIEVENT_PRIVILEGED;
			break;
		case netievent_task:
			type = NETIEVENT_TASK;
			break;
		default:
			type = NETIEVENT_NORMAL;
			break;
		}
	}

	/*
	 * We need to make sure this signal will be delivered and
	 * the queue will be processed.
	 */
	LOCK(&worker->ievents[type].lock);
	ISC_LIST_ENQUEUE(worker->ievents[type].list, event, link);
	if (type == NETIEVENT_PRIORITY) {
		SIGNAL(&worker->ievents[type].cond);
	}
	UNLOCK(&worker->ievents[type].lock);

	uv_async_send(&worker->async);
}

bool
isc__nmsocket_active(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	if (sock->parent != NULL) {
		return (atomic_load(&sock->parent->active));
	}

	return (atomic_load(&sock->active));
}

bool
isc__nmsocket_deactivate(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (sock->parent != NULL) {
		return (atomic_compare_exchange_strong(&sock->parent->active,
						       &(bool){ true }, false));
	}

	return (atomic_compare_exchange_strong(&sock->active, &(bool){ true },
					       false));
}

void
isc___nmsocket_attach(isc_nmsocket_t *sock, isc_nmsocket_t **target FLARG) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(target != NULL && *target == NULL);

	isc_nmsocket_t *rsock = NULL;

	if (sock->parent != NULL) {
		rsock = sock->parent;
		INSIST(rsock->parent == NULL); /* sanity check */
	} else {
		rsock = sock;
	}

	NETMGR_TRACE_LOG("isc__nmsocket_attach():%p->references = %" PRIuFAST32
			 "\n",
			 rsock, isc_refcount_current(&rsock->references) + 1);

	isc_refcount_increment0(&rsock->references);

	*target = sock;
}

/*
 * Free all resources inside a socket (including its children if any).
 */
static void
nmsocket_cleanup(isc_nmsocket_t *sock, bool dofree FLARG) {
	isc_nmhandle_t *handle = NULL;
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(!isc__nmsocket_active(sock));

	NETMGR_TRACE_LOG("nmsocket_cleanup():%p->references = %" PRIuFAST32
			 "\n",
			 sock, isc_refcount_current(&sock->references));

	atomic_store(&sock->destroying, true);

	if (sock->parent == NULL && sock->children != NULL) {
		/*
		 * We shouldn't be here unless there are no active handles,
		 * so we can clean up and free the children.
		 */
		for (size_t i = 0; i < sock->nchildren; i++) {
			if (!atomic_load(&sock->children[i].destroying)) {
				nmsocket_cleanup(&sock->children[i],
						 false FLARG_PASS);
			}
		}

		/*
		 * This was a parent socket: destroy the listening
		 * barriers that synchronized the children.
		 */
		isc_barrier_destroy(&sock->startlistening);
		isc_barrier_destroy(&sock->stoplistening);

		/*
		 * Now free them.
		 */
		isc_mem_put(sock->mgr->mctx, sock->children,
			    sock->nchildren * sizeof(*sock));
		sock->children = NULL;
		sock->nchildren = 0;
	}
	if (sock->statsindex != NULL) {
		isc__nm_decstats(sock->mgr, sock->statsindex[STATID_ACTIVE]);
	}

	sock->statichandle = NULL;

	if (sock->outerhandle != NULL) {
		isc__nmhandle_detach(&sock->outerhandle FLARG_PASS);
	}

	if (sock->outer != NULL) {
		isc___nmsocket_detach(&sock->outer FLARG_PASS);
	}

	while ((handle = isc_astack_pop(sock->inactivehandles)) != NULL) {
		nmhandle_free(sock, handle);
	}

	if (sock->buf != NULL) {
		isc_mem_free(sock->mgr->mctx, sock->buf);
	}

	if (sock->quota != NULL) {
		isc_quota_detach(&sock->quota);
	}

	sock->pquota = NULL;

	isc_astack_destroy(sock->inactivehandles);

	while ((uvreq = isc_astack_pop(sock->inactivereqs)) != NULL) {
		isc_mem_put(sock->mgr->mctx, uvreq, sizeof(*uvreq));
	}

	isc_astack_destroy(sock->inactivereqs);
	sock->magic = 0;

	isc_condition_destroy(&sock->scond);
	isc_condition_destroy(&sock->cond);
	isc_mutex_destroy(&sock->lock);
#ifdef NETMGR_TRACE
	LOCK(&sock->mgr->lock);
	ISC_LIST_UNLINK(sock->mgr->active_sockets, sock, active_link);
	UNLOCK(&sock->mgr->lock);
#endif
	if (dofree) {
		isc_nm_t *mgr = sock->mgr;
		isc_mem_put(mgr->mctx, sock, sizeof(*sock));
		isc_nm_detach(&mgr);
	} else {
		isc_nm_detach(&sock->mgr);
	}
}

static void
nmsocket_maybe_destroy(isc_nmsocket_t *sock FLARG) {
	int active_handles;
	bool destroy = false;

	NETMGR_TRACE_LOG("%s():%p->references = %" PRIuFAST32 "\n", __func__,
			 sock, isc_refcount_current(&sock->references));

	if (sock->parent != NULL) {
		/*
		 * This is a child socket and cannot be destroyed except
		 * as a side effect of destroying the parent, so let's go
		 * see if the parent is ready to be destroyed.
		 */
		nmsocket_maybe_destroy(sock->parent FLARG_PASS);
		return;
	}

	/*
	 * This is a parent socket (or a standalone). See whether the
	 * children have active handles before deciding whether to
	 * accept destruction.
	 */
	LOCK(&sock->lock);
	if (atomic_load(&sock->active) || atomic_load(&sock->destroying) ||
	    !atomic_load(&sock->closed) || atomic_load(&sock->references) != 0)
	{
		UNLOCK(&sock->lock);
		return;
	}

	active_handles = atomic_load(&sock->ah);
	if (sock->children != NULL) {
		for (size_t i = 0; i < sock->nchildren; i++) {
			LOCK(&sock->children[i].lock);
			active_handles += atomic_load(&sock->children[i].ah);
			UNLOCK(&sock->children[i].lock);
		}
	}

	if (active_handles == 0 || sock->statichandle != NULL) {
		destroy = true;
	}

	NETMGR_TRACE_LOG("%s:%p->active_handles = %d, .statichandle = %p\n",
			 __func__, sock, active_handles, sock->statichandle);

	if (destroy) {
		atomic_store(&sock->destroying, true);
		UNLOCK(&sock->lock);
		nmsocket_cleanup(sock, true FLARG_PASS);
	} else {
		UNLOCK(&sock->lock);
	}
}

void
isc___nmsocket_prep_destroy(isc_nmsocket_t *sock FLARG) {
	REQUIRE(sock->parent == NULL);

	NETMGR_TRACE_LOG("isc___nmsocket_prep_destroy():%p->references = "
			 "%" PRIuFAST32 "\n",
			 sock, isc_refcount_current(&sock->references));

	/*
	 * The final external reference to the socket is gone. We can try
	 * destroying the socket, but we have to wait for all the inflight
	 * handles to finish first.
	 */
	atomic_store(&sock->active, false);

	/*
	 * If the socket has children, they'll need to be marked inactive
	 * so they can be cleaned up too.
	 */
	if (sock->children != NULL) {
		for (size_t i = 0; i < sock->nchildren; i++) {
			atomic_store(&sock->children[i].active, false);
		}
	}

	/*
	 * If we're here then we already stopped listening; otherwise
	 * we'd have a hanging reference from the listening process.
	 *
	 * If it's a regular socket we may need to close it.
	 */
	if (!atomic_load(&sock->closed)) {
		switch (sock->type) {
		case isc_nm_udpsocket:
			isc__nm_udp_close(sock);
			return;
		case isc_nm_tcpsocket:
			isc__nm_tcp_close(sock);
			return;
		case isc_nm_tcpdnssocket:
			isc__nm_tcpdns_close(sock);
			return;
		default:
			break;
		}
	}

	nmsocket_maybe_destroy(sock FLARG_PASS);
}

void
isc___nmsocket_detach(isc_nmsocket_t **sockp FLARG) {
	REQUIRE(sockp != NULL && *sockp != NULL);
	REQUIRE(VALID_NMSOCK(*sockp));

	isc_nmsocket_t *sock = *sockp, *rsock = NULL;
	*sockp = NULL;

	/*
	 * If the socket is a part of a set (a child socket) we are
	 * counting references for the whole set at the parent.
	 */
	if (sock->parent != NULL) {
		rsock = sock->parent;
		INSIST(rsock->parent == NULL); /* Sanity check */
	} else {
		rsock = sock;
	}

	NETMGR_TRACE_LOG("isc__nmsocket_detach():%p->references = %" PRIuFAST32
			 "\n",
			 rsock, isc_refcount_current(&rsock->references) - 1);

	if (isc_refcount_decrement(&rsock->references) == 1) {
		isc___nmsocket_prep_destroy(rsock FLARG_PASS);
	}
}

void
isc_nmsocket_close(isc_nmsocket_t **sockp) {
	REQUIRE(sockp != NULL);
	REQUIRE(VALID_NMSOCK(*sockp));
	REQUIRE((*sockp)->type == isc_nm_udplistener ||
		(*sockp)->type == isc_nm_tcplistener ||
		(*sockp)->type == isc_nm_tcpdnslistener);

	isc__nmsocket_detach(sockp);
}

void
isc___nmsocket_init(isc_nmsocket_t *sock, isc_nm_t *mgr, isc_nmsocket_type type,
		    isc_sockaddr_t *iface FLARG) {
	uint16_t family;

	REQUIRE(sock != NULL);
	REQUIRE(mgr != NULL);
	REQUIRE(iface != NULL);

	family = iface->type.sa.sa_family;

	*sock = (isc_nmsocket_t){ .type = type,
				  .iface = *iface,
				  .fd = -1,
				  .inactivehandles = isc_astack_new(
					  mgr->mctx, ISC_NM_HANDLES_STACK_SIZE),
				  .inactivereqs = isc_astack_new(
					  mgr->mctx, ISC_NM_REQS_STACK_SIZE) };

#if NETMGR_TRACE
	sock->backtrace_size = backtrace(sock->backtrace, TRACE_SIZE);
	ISC_LINK_INIT(sock, active_link);
	ISC_LIST_INIT(sock->active_handles);
	LOCK(&mgr->lock);
	ISC_LIST_APPEND(mgr->active_sockets, sock, active_link);
	UNLOCK(&mgr->lock);
#endif

	isc_nm_attach(mgr, &sock->mgr);
	sock->uv_handle.handle.data = sock;

	ISC_LINK_INIT(&sock->quotacb, link);

	switch (type) {
	case isc_nm_udpsocket:
	case isc_nm_udplistener:
		if (family == AF_INET) {
			sock->statsindex = udp4statsindex;
		} else {
			sock->statsindex = udp6statsindex;
		}
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_ACTIVE]);
		break;
	case isc_nm_tcpsocket:
	case isc_nm_tcplistener:
	case isc_nm_tcpdnssocket:
	case isc_nm_tcpdnslistener:
		if (family == AF_INET) {
			sock->statsindex = tcp4statsindex;
		} else {
			sock->statsindex = tcp6statsindex;
		}
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_ACTIVE]);
		break;
	default:
		break;
	}

	isc_mutex_init(&sock->lock);
	isc_condition_init(&sock->cond);
	isc_condition_init(&sock->scond);
	isc_refcount_init(&sock->references, 1);

	NETMGR_TRACE_LOG("isc__nmsocket_init():%p->references = %" PRIuFAST32
			 "\n",
			 sock, isc_refcount_current(&sock->references));

	atomic_init(&sock->active, true);
	atomic_init(&sock->sequential, false);
	atomic_init(&sock->readpaused, false);
	atomic_init(&sock->closing, false);
	atomic_init(&sock->listening, 0);
	atomic_init(&sock->closed, 0);
	atomic_init(&sock->destroying, 0);
	atomic_init(&sock->ah, 0);
	atomic_init(&sock->client, 0);
	atomic_init(&sock->connecting, false);
	atomic_init(&sock->keepalive, false);
	atomic_init(&sock->connected, false);
	atomic_init(&sock->timedout, false);

	atomic_init(&sock->active_child_connections, 0);

	sock->magic = NMSOCK_MAGIC;
}

void
isc__nmsocket_clearcb(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(!isc__nm_in_netthread() || sock->tid == isc_nm_tid());

	sock->recv_cb = NULL;
	sock->recv_cbarg = NULL;
	sock->accept_cb = NULL;
	sock->accept_cbarg = NULL;
	sock->connect_cb = NULL;
	sock->connect_cbarg = NULL;
}

void
isc__nm_free_uvbuf(isc_nmsocket_t *sock, const uv_buf_t *buf) {
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	worker = &sock->mgr->workers[sock->tid];
	REQUIRE(buf->base == worker->recvbuf);

	worker->recvbuf_inuse = false;
}

static isc_nmhandle_t *
alloc_handle(isc_nmsocket_t *sock) {
	isc_nmhandle_t *handle =
		isc_mem_get(sock->mgr->mctx,
			    sizeof(isc_nmhandle_t) + sock->extrahandlesize);

	*handle = (isc_nmhandle_t){ .magic = NMHANDLE_MAGIC };
#ifdef NETMGR_TRACE
	ISC_LINK_INIT(handle, active_link);
#endif
	isc_refcount_init(&handle->references, 1);

	return (handle);
}

isc_nmhandle_t *
isc___nmhandle_get(isc_nmsocket_t *sock, isc_sockaddr_t *peer,
		   isc_sockaddr_t *local FLARG) {
	isc_nmhandle_t *handle = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	handle = isc_astack_pop(sock->inactivehandles);

	if (handle == NULL) {
		handle = alloc_handle(sock);
	} else {
		isc_refcount_init(&handle->references, 1);
		INSIST(VALID_NMHANDLE(handle));
	}

	NETMGR_TRACE_LOG(
		"isc__nmhandle_get():handle %p->references = %" PRIuFAST32 "\n",
		handle, isc_refcount_current(&handle->references));

	isc___nmsocket_attach(sock, &handle->sock FLARG_PASS);

#if NETMGR_TRACE
	handle->backtrace_size = backtrace(handle->backtrace, TRACE_SIZE);
#endif

	if (peer != NULL) {
		handle->peer = *peer;
	} else {
		handle->peer = sock->peer;
	}

	if (local != NULL) {
		handle->local = *local;
	} else {
		handle->local = sock->iface;
	}

	(void)atomic_fetch_add(&sock->ah, 1);

#ifdef NETMGR_TRACE
	LOCK(&sock->lock);
	ISC_LIST_APPEND(sock->active_handles, handle, active_link);
	UNLOCK(&sock->lock);
#endif

	switch (sock->type) {
	case isc_nm_udpsocket:
	case isc_nm_tcpdnssocket:
		if (!atomic_load(&sock->client)) {
			break;
		}
		FALLTHROUGH;
	case isc_nm_tcpsocket:
		INSIST(sock->statichandle == NULL);

		/*
		 * statichandle must be assigned, not attached;
		 * otherwise, if a handle was detached elsewhere
		 * it could never reach 0 references, and the
		 * handle and socket would never be freed.
		 */
		sock->statichandle = handle;
		break;
	default:
		break;
	}

	return (handle);
}

void
isc__nmhandle_attach(isc_nmhandle_t *handle, isc_nmhandle_t **handlep FLARG) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(handlep != NULL && *handlep == NULL);

	NETMGR_TRACE_LOG("isc__nmhandle_attach():handle %p->references = "
			 "%" PRIuFAST32 "\n",
			 handle, isc_refcount_current(&handle->references) + 1);

	isc_refcount_increment(&handle->references);
	*handlep = handle;
}

bool
isc_nmhandle_is_stream(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->sock->type == isc_nm_tcpsocket ||
		handle->sock->type == isc_nm_tcpdnssocket);
}

static void
nmhandle_free(isc_nmsocket_t *sock, isc_nmhandle_t *handle) {
	size_t extra = sock->extrahandlesize;

	isc_refcount_destroy(&handle->references);

	if (handle->dofree != NULL) {
		handle->dofree(handle->opaque);
	}

	*handle = (isc_nmhandle_t){ .magic = 0 };

	isc_mem_put(sock->mgr->mctx, handle, sizeof(isc_nmhandle_t) + extra);
}

static void
nmhandle_deactivate(isc_nmsocket_t *sock, isc_nmhandle_t *handle) {
	bool reuse = false;

	/*
	 * We do all of this under lock to avoid races with socket
	 * destruction.  We have to do this now, because at this point the
	 * socket is either unused or still attached to event->sock.
	 */
	LOCK(&sock->lock);

#ifdef NETMGR_TRACE
	ISC_LIST_UNLINK(sock->active_handles, handle, active_link);
#endif

	INSIST(atomic_fetch_sub(&sock->ah, 1) > 0);

#if !__SANITIZE_ADDRESS__ && !__SANITIZE_THREAD__
	if (atomic_load(&sock->active)) {
		reuse = isc_astack_trypush(sock->inactivehandles, handle);
	}
#endif /* !__SANITIZE_ADDRESS__ && !__SANITIZE_THREAD__ */
	if (!reuse) {
		nmhandle_free(sock, handle);
	}
	UNLOCK(&sock->lock);
}

void
isc__nmhandle_detach(isc_nmhandle_t **handlep FLARG) {
	isc_nmsocket_t *sock = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(handlep != NULL);
	REQUIRE(VALID_NMHANDLE(*handlep));

	handle = *handlep;
	*handlep = NULL;

	/*
	 * If the closehandle_cb is set, it needs to run asynchronously to
	 * ensure correct ordering of the isc__nm_process_sock_buffer().
	 */
	sock = handle->sock;
	if (sock->tid == isc_nm_tid() && sock->closehandle_cb == NULL) {
		nmhandle_detach_cb(&handle FLARG_PASS);
	} else {
		isc__netievent_detach_t *event =
			isc__nm_get_netievent_detach(sock->mgr, sock);
		/*
		 * we are using implicit "attach" as the last reference
		 * need to be destroyed explicitly in the async callback
		 */
		event->handle = handle;
		FLARG_IEVENT_PASS(event);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)event);
	}
}

void
isc__nmsocket_shutdown(isc_nmsocket_t *sock);

static void
nmhandle_detach_cb(isc_nmhandle_t **handlep FLARG) {
	isc_nmsocket_t *sock = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(handlep != NULL);
	REQUIRE(VALID_NMHANDLE(*handlep));

	handle = *handlep;
	*handlep = NULL;

	NETMGR_TRACE_LOG("isc__nmhandle_detach():%p->references = %" PRIuFAST32
			 "\n",
			 handle, isc_refcount_current(&handle->references) - 1);

	if (isc_refcount_decrement(&handle->references) > 1) {
		return;
	}

	/* We need an acquire memory barrier here */
	(void)isc_refcount_current(&handle->references);

	sock = handle->sock;
	handle->sock = NULL;

	if (handle->doreset != NULL) {
		handle->doreset(handle->opaque);
	}

	nmhandle_deactivate(sock, handle);

	/*
	 * The handle is gone now. If the socket has a callback configured
	 * for that (e.g., to perform cleanup after request processing),
	 * call it now, or schedule it to run asynchronously.
	 */
	if (sock->closehandle_cb != NULL) {
		if (sock->tid == isc_nm_tid()) {
			sock->closehandle_cb(sock);
		} else {
			isc__netievent_close_t *event =
				isc__nm_get_netievent_close(sock->mgr, sock);
			isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
					       (isc__netievent_t *)event);
		}
	}

	if (handle == sock->statichandle) {
		/* statichandle is assigned, not attached. */
		sock->statichandle = NULL;
	}

	isc___nmsocket_detach(&sock FLARG_PASS);
}

void *
isc_nmhandle_getdata(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->opaque);
}

int
isc_nmhandle_getfd(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->sock->fd);
}

void
isc_nmhandle_setdata(isc_nmhandle_t *handle, void *arg,
		     isc_nm_opaquecb_t doreset, isc_nm_opaquecb_t dofree) {
	REQUIRE(VALID_NMHANDLE(handle));

	handle->opaque = arg;
	handle->doreset = doreset;
	handle->dofree = dofree;
}

void
isc__nm_alloc_dnsbuf(isc_nmsocket_t *sock, size_t len) {
	REQUIRE(len <= NM_BIG_BUF);

	if (sock->buf == NULL) {
		/* We don't have the buffer at all */
		size_t alloc_len = len < NM_REG_BUF ? NM_REG_BUF : NM_BIG_BUF;
		sock->buf = isc_mem_allocate(sock->mgr->mctx, alloc_len);
		sock->buf_size = alloc_len;
	} else {
		/* We have the buffer but it's too small */
		sock->buf = isc_mem_reallocate(sock->mgr->mctx, sock->buf,
					       NM_BIG_BUF);
		sock->buf_size = NM_BIG_BUF;
	}
}

void
isc__nm_failed_send_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
		       isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	if (req->cb.send != NULL) {
		isc__nm_sendcb(sock, req, eresult, true);
	} else {
		isc__nm_uvreq_put(&req, sock);
	}
}

void
isc__nm_failed_accept_cb(isc_nmsocket_t *sock, isc_result_t eresult) {
	REQUIRE(sock->accepting);
	REQUIRE(sock->server);

	/*
	 * Detach the quota early to make room for other connections;
	 * otherwise it'd be detached later asynchronously, and clog
	 * the quota unnecessarily.
	 */
	if (sock->quota != NULL) {
		isc_quota_detach(&sock->quota);
	}

	isc__nmsocket_detach(&sock->server);

	sock->accepting = false;

	switch (eresult) {
	case ISC_R_NOTCONNECTED:
		/* IGNORE: The client disconnected before we could accept */
		break;
	default:
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
			      "Accepting TCP connection failed: %s",
			      isc_result_totext(eresult));
	}
}

void
isc__nm_failed_connect_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
			  isc_result_t eresult, bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(req->cb.connect != NULL);

	isc__nmsocket_timer_stop(sock);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	INSIST(atomic_compare_exchange_strong(&sock->connecting,
					      &(bool){ true }, false));

	isc__nmsocket_clearcb(sock);
	isc__nm_connectcb(sock, req, eresult, async);

	isc__nmsocket_prep_destroy(sock);
}

void
isc__nm_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result, bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	UNUSED(async);

	switch (sock->type) {
	case isc_nm_udpsocket:
		isc__nm_udp_failed_read_cb(sock, result);
		return;
	case isc_nm_tcpsocket:
		isc__nm_tcp_failed_read_cb(sock, result);
		return;
	case isc_nm_tcpdnssocket:
		isc__nm_tcpdns_failed_read_cb(sock, result);
		return;
	default:
		UNREACHABLE();
	}
}

void
isc__nmsocket_connecttimeout_cb(uv_timer_t *timer) {
	uv_connect_t *uvreq = uv_handle_get_data((uv_handle_t *)timer);
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)uvreq->handle);
	isc__nm_uvreq_t *req = uv_handle_get_data((uv_handle_t *)uvreq);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->connecting));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMHANDLE(req->handle));

	isc__nmsocket_timer_stop(sock);

	/*
	 * Mark the connection as timed out and shutdown the socket.
	 */

	INSIST(atomic_compare_exchange_strong(&sock->timedout, &(bool){ false },
					      true));
	isc__nmsocket_clearcb(sock);
	isc__nmsocket_shutdown(sock);
}

void
isc__nm_accept_connection_log(isc_result_t result, bool can_log_quota) {
	int level;

	switch (result) {
	case ISC_R_SUCCESS:
	case ISC_R_NOCONN:
		return;
	case ISC_R_QUOTA:
	case ISC_R_SOFTQUOTA:
		if (!can_log_quota) {
			return;
		}
		level = ISC_LOG_INFO;
		break;
	case ISC_R_NOTCONNECTED:
		level = ISC_LOG_INFO;
		break;
	default:
		level = ISC_LOG_ERROR;
	}

	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      level, "Accepting TCP connection failed: %s",
		      isc_result_totext(result));
}

void
isc__nmsocket_writetimeout_cb(void *data, isc_result_t eresult) {
	isc__nm_uvreq_t *req = data;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(eresult == ISC_R_TIMEDOUT);
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMSOCK(req->sock));

	sock = req->sock;

	isc__nmsocket_reset(sock);
}

void
isc__nmsocket_readtimeout_cb(uv_timer_t *timer) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)timer);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->reading);

	if (atomic_load(&sock->client)) {
		uv_timer_stop(timer);

		if (sock->recv_cb != NULL) {
			isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
			isc__nm_readcb(sock, req, ISC_R_TIMEDOUT);
		}

		if (!isc__nmsocket_timer_running(sock)) {
			isc__nmsocket_clearcb(sock);
			isc__nm_failed_read_cb(sock, ISC_R_CANCELED, false);
		}
	} else {
		isc__nm_failed_read_cb(sock, ISC_R_TIMEDOUT, false);
	}
}

void
isc__nmsocket_timer_restart(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (atomic_load(&sock->connecting)) {
		int r;

		if (sock->connect_timeout == 0) {
			return;
		}

		r = uv_timer_start(&sock->read_timer,
				   isc__nmsocket_connecttimeout_cb,
				   sock->connect_timeout + 10, 0);
		UV_RUNTIME_CHECK(uv_timer_start, r);

	} else {
		int r;

		if (sock->read_timeout == 0) {
			return;
		}

		r = uv_timer_start(&sock->read_timer,
				   isc__nmsocket_readtimeout_cb,
				   sock->read_timeout, 0);
		UV_RUNTIME_CHECK(uv_timer_start, r);
	}
}

bool
isc__nmsocket_timer_running(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	return (uv_is_active((uv_handle_t *)&sock->read_timer));
}

void
isc__nmsocket_timer_start(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (isc__nmsocket_timer_running(sock)) {
		return;
	}

	isc__nmsocket_timer_restart(sock);
}

void
isc__nmsocket_timer_stop(isc_nmsocket_t *sock) {
	int r;

	REQUIRE(VALID_NMSOCK(sock));

	/* uv_timer_stop() is idempotent, no need to check if running */

	r = uv_timer_stop(&sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_stop, r);
}

isc__nm_uvreq_t *
isc__nm_get_read_req(isc_nmsocket_t *sock, isc_sockaddr_t *sockaddr) {
	isc__nm_uvreq_t *req = NULL;

	req = isc__nm_uvreq_get(sock->mgr, sock);
	req->cb.recv = sock->recv_cb;
	req->cbarg = sock->recv_cbarg;

	switch (sock->type) {
	case isc_nm_tcpsocket:
		isc_nmhandle_attach(sock->statichandle, &req->handle);
		break;
	default:
		if (atomic_load(&sock->client)) {
			isc_nmhandle_attach(sock->statichandle, &req->handle);
		} else {
			req->handle = isc__nmhandle_get(sock, sockaddr, NULL);
		}
		break;
	}

	return (req);
}

/*%<
 * Allocator callback for read operations.
 *
 * Note this doesn't actually allocate anything, it just assigns the
 * worker's receive buffer to a socket, and marks it as "in use".
 */
void
isc__nm_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(isc__nm_in_netthread());
	/*
	 * The size provided by libuv is only suggested size, and it always
	 * defaults to 64 * 1024 in the current versions of libuv (see
	 * src/unix/udp.c and src/unix/stream.c).
	 */
	UNUSED(size);

	worker = &sock->mgr->workers[sock->tid];
	INSIST(!worker->recvbuf_inuse);
	INSIST(worker->recvbuf != NULL);

	switch (sock->type) {
	case isc_nm_udpsocket:
		buf->len = ISC_NETMGR_UDP_RECVBUF_SIZE;
		break;
	case isc_nm_tcpsocket:
	case isc_nm_tcpdnssocket:
		buf->len = ISC_NETMGR_TCP_RECVBUF_SIZE;
		break;
	default:
		UNREACHABLE();
	}

	REQUIRE(buf->len <= ISC_NETMGR_RECVBUF_SIZE);
	buf->base = worker->recvbuf;

	worker->recvbuf_inuse = true;
}

isc_result_t
isc__nm_start_reading(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_SUCCESS;
	int r;

	if (sock->reading) {
		return (ISC_R_SUCCESS);
	}

	switch (sock->type) {
	case isc_nm_udpsocket:
		r = uv_udp_recv_start(&sock->uv_handle.udp, isc__nm_alloc_cb,
				      isc__nm_udp_read_cb);
		break;
	case isc_nm_tcpsocket:
		r = uv_read_start(&sock->uv_handle.stream, isc__nm_alloc_cb,
				  isc__nm_tcp_read_cb);
		break;
	case isc_nm_tcpdnssocket:
		r = uv_read_start(&sock->uv_handle.stream, isc__nm_alloc_cb,
				  isc__nm_tcpdns_read_cb);
		break;
	default:
		UNREACHABLE();
	}

	if (r != 0) {
		result = isc__nm_uverr2result(r);
	} else {
		sock->reading = true;
	}

	return (result);
}

void
isc__nm_stop_reading(isc_nmsocket_t *sock) {
	int r;

	if (!sock->reading) {
		return;
	}

	switch (sock->type) {
	case isc_nm_udpsocket:
		r = uv_udp_recv_stop(&sock->uv_handle.udp);
		UV_RUNTIME_CHECK(uv_udp_recv_stop, r);
		break;
	case isc_nm_tcpsocket:
	case isc_nm_tcpdnssocket:
		r = uv_read_stop(&sock->uv_handle.stream);
		UV_RUNTIME_CHECK(uv_read_stop, r);
		break;
	default:
		UNREACHABLE();
	}
	sock->reading = false;
}

bool
isc__nm_closing(isc_nmsocket_t *sock) {
	return (atomic_load(&sock->mgr->closing));
}

bool
isc__nmsocket_closing(isc_nmsocket_t *sock) {
	return (!isc__nmsocket_active(sock) || atomic_load(&sock->closing) ||
		atomic_load(&sock->mgr->closing) ||
		(sock->server != NULL && !isc__nmsocket_active(sock->server)));
}

static isc_result_t
processbuffer(isc_nmsocket_t *sock) {
	switch (sock->type) {
	case isc_nm_tcpdnssocket:
		return (isc__nm_tcpdns_processbuffer(sock));
	default:
		UNREACHABLE();
	}
}

/*
 * Process a DNS message.
 *
 * If we only have an incomplete DNS message, we don't touch any
 * timers. If we do have a full message, reset the timer.
 *
 * Stop reading if this is a client socket, or if the server socket
 * has been set to sequential mode, or the number of queries we are
 * processing simultaneously has reached the clients-per-connection
 * limit. In this case we'll be called again by resume_processing()
 * later.
 */
isc_result_t
isc__nm_process_sock_buffer(isc_nmsocket_t *sock) {
	for (;;) {
		int_fast32_t ah = atomic_load(&sock->ah);
		isc_result_t result = processbuffer(sock);
		switch (result) {
		case ISC_R_NOMORE:
			/*
			 * Don't reset the timer until we have a
			 * full DNS message.
			 */
			result = isc__nm_start_reading(sock);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}
			/*
			 * Start the timer only if there are no externally used
			 * active handles, there's always one active handle
			 * attached internally to sock->recv_handle in
			 * accept_connection()
			 */
			if (ah == 1) {
				isc__nmsocket_timer_start(sock);
			}
			goto done;
		case ISC_R_CANCELED:
			isc__nmsocket_timer_stop(sock);
			isc__nm_stop_reading(sock);
			goto done;
		case ISC_R_SUCCESS:
			/*
			 * Stop the timer on the successful message read, this
			 * also allows to restart the timer when we have no more
			 * data.
			 */
			isc__nmsocket_timer_stop(sock);

			if (atomic_load(&sock->client) ||
			    atomic_load(&sock->sequential) ||
			    ah >= STREAM_CLIENTS_PER_CONN)
			{
				isc__nm_stop_reading(sock);
				goto done;
			}
			break;
		default:
			UNREACHABLE();
		}
	}
done:
	return (ISC_R_SUCCESS);
}

void
isc__nm_resume_processing(void *arg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(!atomic_load(&sock->client));

	if (isc__nmsocket_closing(sock)) {
		return;
	}

	isc__nm_process_sock_buffer(sock);
}

void
isc_nmhandle_cleartimeout(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	switch (handle->sock->type) {
	default:
		handle->sock->read_timeout = 0;

		if (uv_is_active((uv_handle_t *)&handle->sock->read_timer)) {
			isc__nmsocket_timer_stop(handle->sock);
		}
	}
}

void
isc_nmhandle_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	switch (handle->sock->type) {
	default:
		handle->sock->read_timeout = timeout;
		isc__nmsocket_timer_restart(handle->sock);
	}
}

void
isc_nmhandle_keepalive(isc_nmhandle_t *handle, bool value) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;

	switch (sock->type) {
	case isc_nm_tcpsocket:
	case isc_nm_tcpdnssocket:
		atomic_store(&sock->keepalive, value);
		sock->read_timeout = value ? atomic_load(&sock->mgr->keepalive)
					   : atomic_load(&sock->mgr->idle);
		sock->write_timeout = value ? atomic_load(&sock->mgr->keepalive)
					    : atomic_load(&sock->mgr->idle);
		break;
	default:
		/*
		 * For any other protocol, this is a no-op.
		 */
		return;
	}
}

void *
isc_nmhandle_getextra(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->extra);
}

isc_sockaddr_t
isc_nmhandle_peeraddr(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->peer);
}

isc_sockaddr_t
isc_nmhandle_localaddr(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->local);
}

isc_nm_t *
isc_nmhandle_netmgr(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	return (handle->sock->mgr);
}

isc__nm_uvreq_t *
isc___nm_uvreq_get(isc_nm_t *mgr, isc_nmsocket_t *sock FLARG) {
	isc__nm_uvreq_t *req = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(VALID_NMSOCK(sock));

	if (sock != NULL && isc__nmsocket_active(sock)) {
		/* Try to reuse one */
		req = isc_astack_pop(sock->inactivereqs);
	}

	if (req == NULL) {
		req = isc_mem_get(mgr->mctx, sizeof(*req));
	}

	*req = (isc__nm_uvreq_t){ .magic = 0 };
	ISC_LINK_INIT(req, link);
	req->uv_req.req.data = req;
	isc___nmsocket_attach(sock, &req->sock FLARG_PASS);
	req->magic = UVREQ_MAGIC;

	return (req);
}

void
isc___nm_uvreq_put(isc__nm_uvreq_t **req0, isc_nmsocket_t *sock FLARG) {
	isc__nm_uvreq_t *req = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(req0 != NULL);
	REQUIRE(VALID_UVREQ(*req0));

	req = *req0;
	*req0 = NULL;

	INSIST(sock == req->sock);

	req->magic = 0;

	/*
	 * We need to save this first to make sure that handle,
	 * sock, and the netmgr won't all disappear.
	 */
	handle = req->handle;
	req->handle = NULL;

#if !__SANITIZE_ADDRESS__ && !__SANITIZE_THREAD__
	if (!isc__nmsocket_active(sock) ||
	    !isc_astack_trypush(sock->inactivereqs, req))
	{
		isc_mem_put(sock->mgr->mctx, req, sizeof(*req));
	}
#else  /* !__SANITIZE_ADDRESS__ && !__SANITIZE_THREAD__ */
	isc_mem_put(sock->mgr->mctx, req, sizeof(*req));
#endif /* !__SANITIZE_ADDRESS__ && !__SANITIZE_THREAD__ */

	if (handle != NULL) {
		isc__nmhandle_detach(&handle FLARG_PASS);
	}

	isc___nmsocket_detach(&sock FLARG_PASS);
}

void
isc_nm_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
	    void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));

	switch (handle->sock->type) {
	case isc_nm_udpsocket:
	case isc_nm_udplistener:
		isc__nm_udp_send(handle, region, cb, cbarg);
		break;
	case isc_nm_tcpsocket:
		isc__nm_tcp_send(handle, region, cb, cbarg);
		break;
	case isc_nm_tcpdnssocket:
		isc__nm_tcpdns_send(handle, region, cb, cbarg);
		break;
	default:
		UNREACHABLE();
	}
}

void
isc_nm_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));

	/*
	 * This is always called via callback (from accept or connect), and
	 * caller must attach to the handle, so the references always need to be
	 * at least 2.
	 */
	REQUIRE(isc_refcount_current(&handle->references) >= 2);

	switch (handle->sock->type) {
	case isc_nm_udpsocket:
		isc__nm_udp_read(handle, cb, cbarg);
		break;
	case isc_nm_tcpsocket:
		isc__nm_tcp_read(handle, cb, cbarg);
		break;
	case isc_nm_tcpdnssocket:
		isc__nm_tcpdns_read(handle, cb, cbarg);
		break;
	default:
		UNREACHABLE();
	}
}

void
isc_nm_cancelread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	switch (handle->sock->type) {
	case isc_nm_udpsocket:
		isc__nm_udp_cancelread(handle);
		break;
	case isc_nm_tcpsocket:
		isc__nm_tcp_cancelread(handle);
		break;
	case isc_nm_tcpdnssocket:
		isc__nm_tcpdns_cancelread(handle);
		break;
	default:
		UNREACHABLE();
	}
}

void
isc_nm_pauseread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	isc_nmsocket_t *sock = handle->sock;

	switch (sock->type) {
	case isc_nm_tcpsocket:
		isc__nm_tcp_pauseread(handle);
		break;
	default:
		UNREACHABLE();
	}
}

void
isc_nm_resumeread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	isc_nmsocket_t *sock = handle->sock;

	switch (sock->type) {
	case isc_nm_tcpsocket:
		isc__nm_tcp_resumeread(handle);
		break;
	default:
		UNREACHABLE();
	}
}

void
isc_nm_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	switch (sock->type) {
	case isc_nm_udplistener:
		isc__nm_udp_stoplistening(sock);
		break;
	case isc_nm_tcpdnslistener:
		isc__nm_tcpdns_stoplistening(sock);
		break;
	case isc_nm_tcplistener:
		isc__nm_tcp_stoplistening(sock);
		break;
	default:
		UNREACHABLE();
	}
}

void
isc__nm_connectcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
		  isc_result_t eresult, bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (!async) {
		isc__netievent_connectcb_t ievent = { .sock = sock,
						      .req = uvreq,
						      .result = eresult };
		isc__nm_async_connectcb(NULL, (isc__netievent_t *)&ievent);
	} else {
		isc__netievent_connectcb_t *ievent =
			isc__nm_get_netievent_connectcb(sock->mgr, sock, uvreq,
							eresult);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_connectcb(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_connectcb_t *ievent = (isc__netievent_connectcb_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;
	isc_result_t eresult = ievent->result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));
	REQUIRE(ievent->sock->tid == isc_nm_tid());
	REQUIRE(uvreq->cb.connect != NULL);

	uvreq->cb.connect(uvreq->handle, eresult, uvreq->cbarg);

	isc__nm_uvreq_put(&uvreq, sock);
}

void
isc__nm_readcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
	       isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (eresult == ISC_R_SUCCESS || eresult == ISC_R_TIMEDOUT) {
		isc__netievent_readcb_t ievent = { .sock = sock,
						   .req = uvreq,
						   .result = eresult };

		isc__nm_async_readcb(NULL, (isc__netievent_t *)&ievent);
	} else {
		isc__netievent_readcb_t *ievent = isc__nm_get_netievent_readcb(
			sock->mgr, sock, uvreq, eresult);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_readcb(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_readcb_t *ievent = (isc__netievent_readcb_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;
	isc_result_t eresult = ievent->result;
	isc_region_t region;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));
	REQUIRE(sock->tid == isc_nm_tid());

	region.base = (unsigned char *)uvreq->uvbuf.base;
	region.length = uvreq->uvbuf.len;

	uvreq->cb.recv(uvreq->handle, eresult, &region, uvreq->cbarg);

	isc__nm_uvreq_put(&uvreq, sock);
}

void
isc__nm_sendcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
	       isc_result_t eresult, bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (!async) {
		isc__netievent_sendcb_t ievent = { .sock = sock,
						   .req = uvreq,
						   .result = eresult };
		isc__nm_async_sendcb(NULL, (isc__netievent_t *)&ievent);
		return;
	}

	isc__netievent_sendcb_t *ievent =
		isc__nm_get_netievent_sendcb(sock->mgr, sock, uvreq, eresult);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_sendcb(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_sendcb_t *ievent = (isc__netievent_sendcb_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;
	isc_result_t eresult = ievent->result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));
	REQUIRE(sock->tid == isc_nm_tid());

	uvreq->cb.send(uvreq->handle, eresult, uvreq->cbarg);

	isc__nm_uvreq_put(&uvreq, sock);
}

static void
isc__nm_async_close(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_close_t *ievent = (isc__netievent_close_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->closehandle_cb != NULL);

	UNUSED(worker);

	ievent->sock->closehandle_cb(sock);
}

void
isc__nm_async_detach(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_detach_t *ievent = (isc__netievent_detach_t *)ev0;
	FLARG_IEVENT(ievent);

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(VALID_NMHANDLE(ievent->handle));
	REQUIRE(ievent->sock->tid == isc_nm_tid());

	UNUSED(worker);

	nmhandle_detach_cb(&ievent->handle FLARG_PASS);
}

static void
reset_shutdown(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	isc__nmsocket_shutdown(sock);
	isc__nmsocket_detach(&sock);
}

void
isc__nmsocket_reset(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	switch (sock->type) {
	case isc_nm_tcpsocket:
	case isc_nm_tcpdnssocket:
		/*
		 * This can be called from the TCP write timeout.
		 */
		REQUIRE(sock->parent == NULL);
		break;
	default:
		UNREACHABLE();
		break;
	}

	if (!uv_is_closing(&sock->uv_handle.handle) &&
	    uv_is_active(&sock->uv_handle.handle))
	{
		/*
		 * The real shutdown will be handled in the respective
		 * close functions.
		 */
		isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });
		int r = uv_tcp_close_reset(&sock->uv_handle.tcp,
					   reset_shutdown);
		UV_RUNTIME_CHECK(uv_tcp_close_reset, r);
	} else {
		isc__nmsocket_shutdown(sock);
	}
}

void
isc__nmsocket_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	switch (sock->type) {
	case isc_nm_udpsocket:
		isc__nm_udp_shutdown(sock);
		break;
	case isc_nm_tcpsocket:
		isc__nm_tcp_shutdown(sock);
		break;
	case isc_nm_tcpdnssocket:
		isc__nm_tcpdns_shutdown(sock);
		break;
	case isc_nm_udplistener:
	case isc_nm_tcplistener:
	case isc_nm_tcpdnslistener:
		return;
	default:
		UNREACHABLE();
	}
}

static void
shutdown_walk_cb(uv_handle_t *handle, void *arg) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	UNUSED(arg);

	if (uv_is_closing(handle)) {
		return;
	}

	switch (handle->type) {
	case UV_UDP:
		isc__nmsocket_shutdown(sock);
		return;
	case UV_TCP:
		switch (sock->type) {
		case isc_nm_tcpsocket:
		case isc_nm_tcpdnssocket:
			if (sock->parent == NULL) {
				/* Reset the TCP connections on shutdown */
				isc__nmsocket_reset(sock);
				return;
			}
			FALLTHROUGH;
		default:
			isc__nmsocket_shutdown(sock);
		}

		return;
	default:
		return;
	}
}

void
isc__nm_async_shutdown(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);

	uv_walk(&worker->loop, shutdown_walk_cb, NULL);
}

bool
isc__nm_acquire_interlocked(isc_nm_t *mgr) {
	if (!isc__nm_in_netthread()) {
		return (false);
	}

	LOCK(&mgr->lock);
	bool success = atomic_compare_exchange_strong(
		&mgr->interlocked, &(int){ ISC_NETMGR_NON_INTERLOCKED },
		isc_nm_tid());

	UNLOCK(&mgr->lock);
	return (success);
}

void
isc__nm_drop_interlocked(isc_nm_t *mgr) {
	if (!isc__nm_in_netthread()) {
		return;
	}

	LOCK(&mgr->lock);
	int tid = atomic_exchange(&mgr->interlocked,
				  ISC_NETMGR_NON_INTERLOCKED);
	INSIST(tid != ISC_NETMGR_NON_INTERLOCKED);
	BROADCAST(&mgr->wkstatecond);
	UNLOCK(&mgr->lock);
}

void
isc__nm_acquire_interlocked_force(isc_nm_t *mgr) {
	if (!isc__nm_in_netthread()) {
		return;
	}

	LOCK(&mgr->lock);
	while (!atomic_compare_exchange_strong(
		&mgr->interlocked, &(int){ ISC_NETMGR_NON_INTERLOCKED },
		isc_nm_tid()))
	{
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);
}

void
isc_nm_setstats(isc_nm_t *mgr, isc_stats_t *stats) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(mgr->stats == NULL);
	REQUIRE(isc_stats_ncounters(stats) == isc_sockstatscounter_max);

	isc_stats_attach(stats, &mgr->stats);
}

void
isc__nm_incstats(isc_nm_t *mgr, isc_statscounter_t counterid) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(counterid != -1);

	if (mgr->stats != NULL) {
		isc_stats_increment(mgr->stats, counterid);
	}
}

void
isc__nm_decstats(isc_nm_t *mgr, isc_statscounter_t counterid) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(counterid != -1);

	if (mgr->stats != NULL) {
		isc_stats_decrement(mgr->stats, counterid);
	}
}

isc_result_t
isc__nm_socket(int domain, int type, int protocol, uv_os_sock_t *sockp) {
#ifdef WIN32
	SOCKET sock;
	sock = socket(domain, type, protocol);
	if (sock == INVALID_SOCKET) {
		char strbuf[ISC_STRERRORSIZE];
		DWORD socket_errno = WSAGetLastError();
		switch (socket_errno) {
		case WSAEMFILE:
		case WSAENOBUFS:
			return (ISC_R_NORESOURCES);

		case WSAEPROTONOSUPPORT:
		case WSAEPFNOSUPPORT:
		case WSAEAFNOSUPPORT:
			return (ISC_R_FAMILYNOSUPPORT);
		default:
			strerror_r(socket_errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(
				__FILE__, __LINE__,
				"socket() failed with error code %lu: %s",
				socket_errno, strbuf);
			return (ISC_R_UNEXPECTED);
		}
	}
#else
	int sock = socket(domain, type, protocol);
	if (sock < 0) {
		return (isc_errno_toresult(errno));
	}
#endif
	*sockp = (uv_os_sock_t)sock;
	return (ISC_R_SUCCESS);
}

void
isc__nm_closesocket(uv_os_sock_t sock) {
#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif
}

#define setsockopt_on(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 1 }, sizeof(int))

#define setsockopt_off(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 0 }, sizeof(int))

isc_result_t
isc__nm_socket_freebind(uv_os_sock_t fd, sa_family_t sa_family) {
	/*
	 * Set the IP_FREEBIND (or equivalent option) on the uv_handle.
	 */
#ifdef IP_FREEBIND
	UNUSED(sa_family);
	if (setsockopt_on(fd, IPPROTO_IP, IP_FREEBIND) == -1) {
		return (ISC_R_FAILURE);
	}
	return (ISC_R_SUCCESS);
#elif defined(IP_BINDANY) || defined(IPV6_BINDANY)
	if (sa_family == AF_INET) {
#if defined(IP_BINDANY)
		if (setsockopt_on(fd, IPPROTO_IP, IP_BINDANY) == -1) {
			return (ISC_R_FAILURE);
		}
		return (ISC_R_SUCCESS);
#endif
	} else if (sa_family == AF_INET6) {
#if defined(IPV6_BINDANY)
		if (setsockopt_on(fd, IPPROTO_IPV6, IPV6_BINDANY) == -1) {
			return (ISC_R_FAILURE);
		}
		return (ISC_R_SUCCESS);
#endif
	}
	return (ISC_R_NOTIMPLEMENTED);
#elif defined(SO_BINDANY)
	UNUSED(sa_family);
	if (setsockopt_on(fd, SOL_SOCKET, SO_BINDANY) == -1) {
		return (ISC_R_FAILURE);
	}
	return (ISC_R_SUCCESS);
#else
	UNUSED(fd);
	UNUSED(sa_family);
	return (ISC_R_NOTIMPLEMENTED);
#endif
}

isc_result_t
isc__nm_socket_reuse(uv_os_sock_t fd) {
	/*
	 * Generally, the SO_REUSEADDR socket option allows reuse of
	 * local addresses.
	 *
	 * On the BSDs, SO_REUSEPORT implies SO_REUSEADDR but with some
	 * additional refinements for programs that use multicast.
	 *
	 * On Linux, SO_REUSEPORT has different semantics: it _shares_ the port
	 * rather than steal it from the current listener, so we don't use it
	 * here, but rather in isc__nm_socket_reuse_lb().
	 *
	 * On Windows, it also allows a socket to forcibly bind to a port in use
	 * by another socket.
	 */

#if defined(SO_REUSEPORT) && !defined(__linux__)
	if (setsockopt_on(fd, SOL_SOCKET, SO_REUSEPORT) == -1) {
		return (ISC_R_FAILURE);
	}
	return (ISC_R_SUCCESS);
#elif defined(SO_REUSEADDR)
	if (setsockopt_on(fd, SOL_SOCKET, SO_REUSEADDR) == -1) {
		return (ISC_R_FAILURE);
	}
	return (ISC_R_SUCCESS);
#else
	UNUSED(fd);
	return (ISC_R_NOTIMPLEMENTED);
#endif
}

isc_result_t
isc__nm_socket_reuse_lb(uv_os_sock_t fd) {
	/*
	 * On FreeBSD 12+, SO_REUSEPORT_LB socket option allows sockets to be
	 * bound to an identical socket address. For UDP sockets, the use of
	 * this option can provide better distribution of incoming datagrams to
	 * multiple processes (or threads) as compared to the traditional
	 * technique of having multiple processes compete to receive datagrams
	 * on the same socket.
	 *
	 * On Linux, the same thing is achieved simply with SO_REUSEPORT.
	 */
#if defined(SO_REUSEPORT_LB)
	if (setsockopt_on(fd, SOL_SOCKET, SO_REUSEPORT_LB) == -1) {
		return (ISC_R_FAILURE);
	} else {
		return (ISC_R_SUCCESS);
	}
#elif defined(SO_REUSEPORT) && defined(__linux__)
	if (setsockopt_on(fd, SOL_SOCKET, SO_REUSEPORT) == -1) {
		return (ISC_R_FAILURE);
	} else {
		return (ISC_R_SUCCESS);
	}
#else
	UNUSED(fd);
	return (ISC_R_NOTIMPLEMENTED);
#endif
}

isc_result_t
isc__nm_socket_incoming_cpu(uv_os_sock_t fd) {
#ifdef SO_INCOMING_CPU
	if (setsockopt_on(fd, SOL_SOCKET, SO_INCOMING_CPU) == -1) {
		return (ISC_R_FAILURE);
	} else {
		return (ISC_R_SUCCESS);
	}
#else
	UNUSED(fd);
#endif
	return (ISC_R_NOTIMPLEMENTED);
}

isc_result_t
isc__nm_socket_disable_pmtud(uv_os_sock_t fd, sa_family_t sa_family) {
	/*
	 * Disable the Path MTU Discovery on IP packets
	 */
	if (sa_family == AF_INET6) {
#if defined(IPV6_DONTFRAG)
		if (setsockopt_off(fd, IPPROTO_IPV6, IPV6_DONTFRAG) == -1) {
			return (ISC_R_FAILURE);
		} else {
			return (ISC_R_SUCCESS);
		}
#elif defined(IPV6_MTU_DISCOVER) && defined(IP_PMTUDISC_OMIT)
		if (setsockopt(fd, IPPROTO_IPV6, IPV6_MTU_DISCOVER,
			       &(int){ IP_PMTUDISC_OMIT }, sizeof(int)) == -1)
		{
			return (ISC_R_FAILURE);
		} else {
			return (ISC_R_SUCCESS);
		}
#else
		UNUSED(fd);
#endif
	} else if (sa_family == AF_INET) {
#if defined(IP_DONTFRAG)
		if (setsockopt_off(fd, IPPROTO_IP, IP_DONTFRAG) == -1) {
			return (ISC_R_FAILURE);
		} else {
			return (ISC_R_SUCCESS);
		}
#elif defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_OMIT)
		if (setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER,
			       &(int){ IP_PMTUDISC_OMIT }, sizeof(int)) == -1)
		{
			return (ISC_R_FAILURE);
		} else {
			return (ISC_R_SUCCESS);
		}
#else
		UNUSED(fd);
#endif
	} else {
		return (ISC_R_FAMILYNOSUPPORT);
	}

	return (ISC_R_NOTIMPLEMENTED);
}

#if defined(_WIN32)
#define TIMEOUT_TYPE	DWORD
#define TIMEOUT_DIV	1000
#define TIMEOUT_OPTNAME TCP_MAXRT
#elif defined(TCP_CONNECTIONTIMEOUT)
#define TIMEOUT_TYPE	int
#define TIMEOUT_DIV	1000
#define TIMEOUT_OPTNAME TCP_CONNECTIONTIMEOUT
#elif defined(TCP_RXT_CONNDROPTIME)
#define TIMEOUT_TYPE	int
#define TIMEOUT_DIV	1000
#define TIMEOUT_OPTNAME TCP_RXT_CONNDROPTIME
#elif defined(TCP_USER_TIMEOUT)
#define TIMEOUT_TYPE	unsigned int
#define TIMEOUT_DIV	1
#define TIMEOUT_OPTNAME TCP_USER_TIMEOUT
#elif defined(TCP_KEEPINIT)
#define TIMEOUT_TYPE	int
#define TIMEOUT_DIV	1000
#define TIMEOUT_OPTNAME TCP_KEEPINIT
#endif

isc_result_t
isc__nm_socket_connectiontimeout(uv_os_sock_t fd, int timeout_ms) {
#if defined(TIMEOUT_OPTNAME)
	TIMEOUT_TYPE timeout = timeout_ms / TIMEOUT_DIV;

	if (timeout == 0) {
		timeout = 1;
	}

	if (setsockopt(fd, IPPROTO_TCP, TIMEOUT_OPTNAME, &timeout,
		       sizeof(timeout)) == -1)
	{
		return (ISC_R_FAILURE);
	}

	return (ISC_R_SUCCESS);
#else
	UNUSED(fd);
	UNUSED(timeout_ms);

	return (ISC_R_SUCCESS);
#endif
}

isc_result_t
isc__nm_socket_tcp_nodelay(uv_os_sock_t fd) {
#ifdef TCP_NODELAY
	if (setsockopt_on(fd, IPPROTO_TCP, TCP_NODELAY) == -1) {
		return (ISC_R_FAILURE);
	} else {
		return (ISC_R_SUCCESS);
	}
#else
	UNUSED(fd);
	return (ISC_R_SUCCESS);
#endif
}

static isc_threadresult_t
isc__nm_work_run(isc_threadarg_t arg) {
	isc__nm_work_t *work = (isc__nm_work_t *)arg;

	work->cb(work->data);

	return ((isc_threadresult_t)0);
}

static void
isc__nm_work_cb(uv_work_t *req) {
	isc__nm_work_t *work = uv_req_get_data((uv_req_t *)req);

	if (isc_tid_v == SIZE_MAX) {
		isc__trampoline_t *trampoline_arg =
			isc__trampoline_get(isc__nm_work_run, work);
		(void)isc__trampoline_run(trampoline_arg);
	} else {
		(void)isc__nm_work_run((isc_threadarg_t)work);
	}
}

static void
isc__nm_after_work_cb(uv_work_t *req, int status) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__nm_work_t *work = uv_req_get_data((uv_req_t *)req);
	isc_nm_t *netmgr = work->netmgr;

	if (status != 0) {
		result = isc__nm_uverr2result(status);
	}

	work->after_cb(work->data, result);

	isc_mem_put(netmgr->mctx, work, sizeof(*work));

	isc_nm_detach(&netmgr);
}

void
isc_nm_work_offload(isc_nm_t *netmgr, isc_nm_workcb_t work_cb,
		    isc_nm_after_workcb_t after_work_cb, void *data) {
	isc__networker_t *worker = NULL;
	isc__nm_work_t *work = NULL;
	int r;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NM(netmgr));

	worker = &netmgr->workers[isc_nm_tid()];

	work = isc_mem_get(netmgr->mctx, sizeof(*work));
	*work = (isc__nm_work_t){
		.cb = work_cb,
		.after_cb = after_work_cb,
		.data = data,
	};

	isc_nm_attach(netmgr, &work->netmgr);

	uv_req_set_data((uv_req_t *)&work->req, work);

	r = uv_queue_work(&worker->loop, &work->req, isc__nm_work_cb,
			  isc__nm_after_work_cb);
	UV_RUNTIME_CHECK(uv_queue_work, r);
}

void
isc_nm_timer_create(isc_nmhandle_t *handle, isc_nm_timer_cb cb, void *cbarg,
		    isc_nm_timer_t **timerp) {
	isc__networker_t *worker = NULL;
	isc_nmsocket_t *sock = NULL;
	isc_nm_timer_t *timer = NULL;
	int r;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;
	worker = &sock->mgr->workers[isc_nm_tid()];

	timer = isc_mem_get(sock->mgr->mctx, sizeof(*timer));
	*timer = (isc_nm_timer_t){ .cb = cb, .cbarg = cbarg };
	isc_refcount_init(&timer->references, 1);
	isc_nmhandle_attach(handle, &timer->handle);

	r = uv_timer_init(&worker->loop, &timer->timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);

	uv_handle_set_data((uv_handle_t *)&timer->timer, timer);

	*timerp = timer;
}

void
isc_nm_timer_attach(isc_nm_timer_t *timer, isc_nm_timer_t **timerp) {
	REQUIRE(timer != NULL);
	REQUIRE(timerp != NULL && *timerp == NULL);

	isc_refcount_increment(&timer->references);
	*timerp = timer;
}

static void
timer_destroy(uv_handle_t *uvhandle) {
	isc_nm_timer_t *timer = uv_handle_get_data(uvhandle);
	isc_nmhandle_t *handle = timer->handle;
	isc_mem_t *mctx = timer->handle->sock->mgr->mctx;

	isc_mem_put(mctx, timer, sizeof(*timer));

	isc_nmhandle_detach(&handle);
}

void
isc_nm_timer_detach(isc_nm_timer_t **timerp) {
	isc_nm_timer_t *timer = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(timerp != NULL && *timerp != NULL);

	timer = *timerp;
	*timerp = NULL;

	handle = timer->handle;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	if (isc_refcount_decrement(&timer->references) == 1) {
		int r = uv_timer_stop(&timer->timer);
		UV_RUNTIME_CHECK(uv_timer_stop, r);
		uv_close((uv_handle_t *)&timer->timer, timer_destroy);
	}
}

static void
timer_cb(uv_timer_t *uvtimer) {
	isc_nm_timer_t *timer = uv_handle_get_data((uv_handle_t *)uvtimer);

	REQUIRE(timer->cb != NULL);

	timer->cb(timer->cbarg, ISC_R_TIMEDOUT);
}

void
isc_nm_timer_start(isc_nm_timer_t *timer, uint64_t timeout) {
	int r = uv_timer_start(&timer->timer, timer_cb, timeout, 0);
	UV_RUNTIME_CHECK(uv_timer_start, r);
}

void
isc_nm_timer_stop(isc_nm_timer_t *timer) {
	int r = uv_timer_stop(&timer->timer);
	UV_RUNTIME_CHECK(uv_timer_stop, r);
}

#ifdef NETMGR_TRACE
/*
 * Dump all active sockets in netmgr. We output to stderr
 * as the logger might be already shut down.
 */

static const char *
nmsocket_type_totext(isc_nmsocket_type type) {
	switch (type) {
	case isc_nm_udpsocket:
		return ("isc_nm_udpsocket");
	case isc_nm_udplistener:
		return ("isc_nm_udplistener");
	case isc_nm_tcpsocket:
		return ("isc_nm_tcpsocket");
	case isc_nm_tcplistener:
		return ("isc_nm_tcplistener");
	case isc_nm_tcpdnslistener:
		return ("isc_nm_tcpdnslistener");
	case isc_nm_tcpdnssocket:
		return ("isc_nm_tcpdnssocket");
	default:
		UNREACHABLE();
	}
}

static void
nmhandle_dump(isc_nmhandle_t *handle) {
	fprintf(stderr, "Active handle %p, refs %" PRIuFAST32 "\n", handle,
		isc_refcount_current(&handle->references));
	fprintf(stderr, "Created by:\n");
	backtrace_symbols_fd(handle->backtrace, handle->backtrace_size,
			     STDERR_FILENO);
	fprintf(stderr, "\n\n");
}

static void
nmsocket_dump(isc_nmsocket_t *sock) {
	isc_nmhandle_t *handle = NULL;

	LOCK(&sock->lock);
	fprintf(stderr, "\n=================\n");
	fprintf(stderr, "Active %s socket %p, type %s, refs %" PRIuFAST32 "\n",
		atomic_load(&sock->client) ? "client" : "server", sock,
		nmsocket_type_totext(sock->type),
		isc_refcount_current(&sock->references));
	fprintf(stderr,
		"Parent %p, listener %p, server %p, statichandle = "
		"%p\n",
		sock->parent, sock->listener, sock->server, sock->statichandle);
	fprintf(stderr, "Flags:%s%s%s%s%s\n",
		atomic_load(&sock->active) ? " active" : "",
		atomic_load(&sock->closing) ? " closing" : "",
		atomic_load(&sock->destroying) ? " destroying" : "",
		atomic_load(&sock->connecting) ? " connecting" : "",
		sock->accepting ? " accepting" : "");
	fprintf(stderr, "Created by:\n");
	backtrace_symbols_fd(sock->backtrace, sock->backtrace_size,
			     STDERR_FILENO);
	fprintf(stderr, "\n");

	for (handle = ISC_LIST_HEAD(sock->active_handles); handle != NULL;
	     handle = ISC_LIST_NEXT(handle, active_link))
	{
		static bool first = true;
		if (first) {
			fprintf(stderr, "Active handles:\n");
			first = false;
		}
		nmhandle_dump(handle);
	}

	fprintf(stderr, "\n");
	UNLOCK(&sock->lock);
}

void
isc__nm_dump_active(isc_nm_t *nm) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NM(nm));

	LOCK(&nm->lock);
	for (sock = ISC_LIST_HEAD(nm->active_sockets); sock != NULL;
	     sock = ISC_LIST_NEXT(sock, active_link))
	{
		static bool first = true;
		if (first) {
			fprintf(stderr, "Outstanding sockets\n");
			first = false;
		}
		nmsocket_dump(sock);
	}
	UNLOCK(&nm->lock);
}
#endif
