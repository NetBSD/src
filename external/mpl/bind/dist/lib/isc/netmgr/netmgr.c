/*	$NetBSD: netmgr.c,v 1.5 2021/04/03 22:20:26 christos Exp $	*/

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

#include <inttypes.h>
#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/errno.h>
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
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr-int.h"
#include "uv-compat.h"

#ifdef NETMGR_TRACE
#include <execinfo.h>

#endif

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

ISC_THREAD_LOCAL int isc__nm_tid_v = ISC_NETMGR_TID_UNKNOWN;

static void
nmsocket_maybe_destroy(isc_nmsocket_t *sock FLARG);
static void
nmhandle_free(isc_nmsocket_t *sock, isc_nmhandle_t *handle);
static isc_threadresult_t
nm_thread(isc_threadarg_t worker0);
static void
async_cb(uv_async_t *handle);
static bool
process_queue(isc__networker_t *worker, isc_queue_t *queue);
static bool
process_priority_queue(isc__networker_t *worker);
static bool
process_normal_queue(isc__networker_t *worker);
static void
process_queues(isc__networker_t *worker);

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

isc_nm_t *
isc_nm_start(isc_mem_t *mctx, uint32_t workers) {
	isc_nm_t *mgr = NULL;
	char name[32];

#ifdef WIN32
	isc__nm_winsock_initialize();
#endif /* WIN32 */

	isc__nm_tls_initialize();

	mgr = isc_mem_get(mctx, sizeof(*mgr));
	*mgr = (isc_nm_t){ .nworkers = workers };

	isc_mem_attach(mctx, &mgr->mctx);
	isc_mutex_init(&mgr->lock);
	isc_condition_init(&mgr->wkstatecond);
	isc_refcount_init(&mgr->references, 1);
	atomic_init(&mgr->maxudp, 0);
	atomic_init(&mgr->interlocked, false);

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

	isc_mutex_init(&mgr->reqlock);
	isc_mempool_create(mgr->mctx, sizeof(isc__nm_uvreq_t), &mgr->reqpool);
	isc_mempool_setname(mgr->reqpool, "nm_reqpool");
	isc_mempool_setfreemax(mgr->reqpool, 4096);
	isc_mempool_associatelock(mgr->reqpool, &mgr->reqlock);
	isc_mempool_setfillcount(mgr->reqpool, 32);

	isc_mutex_init(&mgr->evlock);
	isc_mempool_create(mgr->mctx, sizeof(isc__netievent_storage_t),
			   &mgr->evpool);
	isc_mempool_setname(mgr->evpool, "nm_evpool");
	isc_mempool_setfreemax(mgr->evpool, 4096);
	isc_mempool_associatelock(mgr->evpool, &mgr->evlock);
	isc_mempool_setfillcount(mgr->evpool, 32);

	mgr->workers = isc_mem_get(mctx, workers * sizeof(isc__networker_t));
	for (size_t i = 0; i < workers; i++) {
		int r;
		isc__networker_t *worker = &mgr->workers[i];
		*worker = (isc__networker_t){
			.mgr = mgr,
			.id = i,
		};

		r = uv_loop_init(&worker->loop);
		RUNTIME_CHECK(r == 0);

		worker->loop.data = &mgr->workers[i];

		r = uv_async_init(&worker->loop, &worker->async, async_cb);
		RUNTIME_CHECK(r == 0);

		isc_mutex_init(&worker->lock);
		isc_condition_init(&worker->cond);

		worker->ievents = isc_queue_new(mgr->mctx, 128);
		worker->ievents_prio = isc_queue_new(mgr->mctx, 128);
		worker->recvbuf = isc_mem_get(mctx, ISC_NETMGR_RECVBUF_SIZE);

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
	return (mgr);
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

	for (size_t i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		isc__netievent_t *event = isc__nm_get_netievent_stop(mgr);
		isc__nm_enqueue_ievent(worker, event);
	}

	LOCK(&mgr->lock);
	while (mgr->workers_running > 0) {
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);

	for (size_t i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		isc__netievent_t *ievent = NULL;
		int r;

		/* Empty the async event queues */
		while ((ievent = (isc__netievent_t *)isc_queue_dequeue(
				worker->ievents)) != NULL)
		{
			isc_mempool_put(mgr->evpool, ievent);
		}

		while ((ievent = (isc__netievent_t *)isc_queue_dequeue(
				worker->ievents_prio)) != NULL)
		{
			isc_mempool_put(mgr->evpool, ievent);
		}

		r = uv_loop_close(&worker->loop);
		INSIST(r == 0);

		isc_queue_destroy(worker->ievents);
		isc_queue_destroy(worker->ievents_prio);
		isc_mutex_destroy(&worker->lock);
		isc_condition_destroy(&worker->cond);

		isc_mem_put(mgr->mctx, worker->recvbuf,
			    ISC_NETMGR_RECVBUF_SIZE);
		isc_thread_join(worker->thread, NULL);
	}

	if (mgr->stats != NULL) {
		isc_stats_detach(&mgr->stats);
	}

	isc_condition_destroy(&mgr->wkstatecond);
	isc_mutex_destroy(&mgr->lock);

	isc_mempool_destroy(&mgr->evpool);
	isc_mutex_destroy(&mgr->evlock);

	isc_mempool_destroy(&mgr->reqpool);
	isc_mutex_destroy(&mgr->reqlock);

	isc_mem_put(mgr->mctx, mgr->workers,
		    mgr->nworkers * sizeof(isc__networker_t));
	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(*mgr));

#ifdef WIN32
	isc__nm_winsock_destroy();
#endif /* WIN32 */
}

void
isc_nm_pause(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(!isc__nm_in_netthread());

	isc__nm_acquire_interlocked_force(mgr);

	for (size_t i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		isc__netievent_resume_t *event =
			isc__nm_get_netievent_pause(mgr);
		isc__nm_enqueue_ievent(worker, (isc__netievent_t *)event);
	}

	LOCK(&mgr->lock);
	while (mgr->workers_paused != mgr->workers_running) {
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);
}

void
isc_nm_resume(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(!isc__nm_in_netthread());

	for (size_t i = 0; i < mgr->nworkers; i++) {
		isc__networker_t *worker = &mgr->workers[i];
		isc__netievent_resume_t *event =
			isc__nm_get_netievent_resume(mgr);
		isc__nm_enqueue_ievent(worker, (isc__netievent_t *)event);
	}

	LOCK(&mgr->lock);
	while (mgr->workers_paused != 0) {
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);

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
isc_nm_closedown(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));

	atomic_store(&mgr->closing, true);
	for (size_t i = 0; i < mgr->nworkers; i++) {
		isc__netievent_t *event = NULL;
		event = isc__nm_get_netievent_shutdown(mgr);
		isc__nm_enqueue_ievent(&mgr->workers[i], event);
	}
}

void
isc_nm_destroy(isc_nm_t **mgr0) {
	isc_nm_t *mgr = NULL;
	int counter = 0;
	uint_fast32_t references;

	REQUIRE(mgr0 != NULL);
	REQUIRE(VALID_NM(*mgr0));

	mgr = *mgr0;

	/*
	 * Close active connections.
	 */
	isc_nm_closedown(mgr);

	/*
	 * Wait for the manager to be dereferenced elsewhere.
	 */
	while ((references = isc_refcount_current(&mgr->references)) > 1 &&
	       counter++ < 1000)
	{
#ifdef WIN32
		_sleep(10);
#else  /* ifdef WIN32 */
		usleep(10000);
#endif /* ifdef WIN32 */
	}

#ifdef NETMGR_TRACE
	isc__nm_dump_active(mgr);
#endif

	INSIST(references == 1);

	/*
	 * Detach final reference.
	 */
	isc_nm_detach(mgr0);
}

void
isc_nm_maxudp(isc_nm_t *mgr, uint32_t maxudp) {
	REQUIRE(VALID_NM(mgr));

	atomic_store(&mgr->maxudp, maxudp);
}

void
isc_nm_settimeouts(isc_nm_t *mgr, uint32_t init, uint32_t idle,
		   uint32_t keepalive, uint32_t advertised) {
	REQUIRE(VALID_NM(mgr));

	atomic_store(&mgr->init, init * 100);
	atomic_store(&mgr->idle, idle * 100);
	atomic_store(&mgr->keepalive, keepalive * 100);
	atomic_store(&mgr->advertised, advertised * 100);
}

void
isc_nm_gettimeouts(isc_nm_t *mgr, uint32_t *initial, uint32_t *idle,
		   uint32_t *keepalive, uint32_t *advertised) {
	REQUIRE(VALID_NM(mgr));

	if (initial != NULL) {
		*initial = atomic_load(&mgr->init) / 100;
	}

	if (idle != NULL) {
		*idle = atomic_load(&mgr->idle) / 100;
	}

	if (keepalive != NULL) {
		*keepalive = atomic_load(&mgr->keepalive) / 100;
	}

	if (advertised != NULL) {
		*advertised = atomic_load(&mgr->advertised) / 100;
	}
}

/*
 * nm_thread is a single worker thread, that runs uv_run event loop
 * until asked to stop.
 */
static isc_threadresult_t
nm_thread(isc_threadarg_t worker0) {
	isc__networker_t *worker = (isc__networker_t *)worker0;
	isc_nm_t *mgr = worker->mgr;

	isc__nm_tid_v = worker->id;
	isc_thread_setaffinity(isc__nm_tid_v);

	while (true) {
		int r = uv_run(&worker->loop, UV_RUN_DEFAULT);
		/* There's always the async handle until we are done */
		INSIST(r > 0 || worker->finished);

		if (worker->paused) {
			LOCK(&worker->lock);
			/* We need to lock the worker first otherwise
			 * isc_nm_resume() might slip in before WAIT() in the
			 * while loop starts and the signal never gets delivered
			 * and we are forever stuck in the paused loop.
			 */

			LOCK(&mgr->lock);
			mgr->workers_paused++;
			SIGNAL(&mgr->wkstatecond);
			UNLOCK(&mgr->lock);

			while (worker->paused) {
				WAIT(&worker->cond, &worker->lock);
				(void)process_priority_queue(worker);
			}

			LOCK(&mgr->lock);
			mgr->workers_paused--;
			SIGNAL(&mgr->wkstatecond);
			UNLOCK(&mgr->lock);

			UNLOCK(&worker->lock);
		}

		if (r == 0) {
			INSIST(worker->finished);
			break;
		}

		INSIST(!worker->finished);

		/*
		 * Empty the async queue.
		 */
		process_queues(worker);
	}

	LOCK(&mgr->lock);
	mgr->workers_running--;
	SIGNAL(&mgr->wkstatecond);
	UNLOCK(&mgr->lock);

	return ((isc_threadresult_t)0);
}

/*
 * async_cb is a universal callback for 'async' events sent to event loop.
 * It's the only way to safely pass data to the libuv event loop. We use a
 * single async event and a lockless queue of 'isc__netievent_t' structures
 * passed from other threads.
 */
static void
async_cb(uv_async_t *handle) {
	isc__networker_t *worker = (isc__networker_t *)handle->loop->data;
	process_queues(worker);
}

static void
isc__nm_async_stop(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	worker->finished = true;
	/* Close the async handler */
	uv_close((uv_handle_t *)&worker->async, NULL);
	/* uv_stop(&worker->loop); */
}

static void
isc__nm_async_pause(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	REQUIRE(worker->paused == false);
	worker->paused = true;
	uv_stop(&worker->loop);
}

static void
isc__nm_async_resume(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	REQUIRE(worker->paused == true);
	worker->paused = false;
}

static bool
process_priority_queue(isc__networker_t *worker) {
	return (process_queue(worker, worker->ievents_prio));
}

static bool
process_normal_queue(isc__networker_t *worker) {
	return (process_queue(worker, worker->ievents));
}

static void
process_queues(isc__networker_t *worker) {
	if (!process_priority_queue(worker)) {
		return;
	}
	(void)process_normal_queue(worker);
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

		NETIEVENT_CASE(tlsstartread);
		NETIEVENT_CASE(tlssend);
		NETIEVENT_CASE(tlsclose);
		NETIEVENT_CASE(tlsconnect);
		NETIEVENT_CASE(tlsdobio);

		NETIEVENT_CASE(tlsdnssend);
		NETIEVENT_CASE(tlsdnscancel);
		NETIEVENT_CASE(tlsdnsclose);
		NETIEVENT_CASE(tlsdnsread);
		NETIEVENT_CASE(tlsdnsstop);

		NETIEVENT_CASE(connectcb);
		NETIEVENT_CASE(readcb);
		NETIEVENT_CASE(sendcb);

		NETIEVENT_CASE(close);
		NETIEVENT_CASE(detach);

		NETIEVENT_CASE(shutdown);
		NETIEVENT_CASE(resume);
		NETIEVENT_CASE_NOMORE(pause);

	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
	return (true);
}

static bool
process_queue(isc__networker_t *worker, isc_queue_t *queue) {
	isc__netievent_t *ievent = NULL;

	while ((ievent = (isc__netievent_t *)isc_queue_dequeue(queue)) != NULL)
	{
		if (!process_netievent(worker, ievent)) {
			return (false);
		}
	}
	return (true);
}

void *
isc__nm_get_netievent(isc_nm_t *mgr, isc__netievent_type type) {
	isc__netievent_storage_t *event = isc_mempool_get(mgr->evpool);

	*event = (isc__netievent_storage_t){ .ni.type = type };
	return (event);
}

void
isc__nm_put_netievent(isc_nm_t *mgr, void *ievent) {
	isc_mempool_put(mgr->evpool, ievent);
}

NETIEVENT_SOCKET_DEF(tcpclose);
NETIEVENT_SOCKET_DEF(tcplisten);
NETIEVENT_SOCKET_DEF(tcppauseread);
NETIEVENT_SOCKET_DEF(tcpstartread);
NETIEVENT_SOCKET_DEF(tcpstop);
NETIEVENT_SOCKET_DEF(tlsclose);
NETIEVENT_SOCKET_DEF(tlsconnect);
NETIEVENT_SOCKET_DEF(tlsdobio);
NETIEVENT_SOCKET_DEF(tlsstartread);
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

NETIEVENT_SOCKET_DEF(tlsdnsclose);
NETIEVENT_SOCKET_DEF(tlsdnsread);
NETIEVENT_SOCKET_DEF(tlsdnsstop);
NETIEVENT_SOCKET_REQ_DEF(tlsdnssend);
NETIEVENT_SOCKET_HANDLE_DEF(tlsdnscancel);

NETIEVENT_SOCKET_REQ_DEF(tcpconnect);
NETIEVENT_SOCKET_REQ_DEF(tcpsend);
NETIEVENT_SOCKET_REQ_DEF(tlssend);
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
	if (event->type > netievent_prio) {
		/*
		 * We need to make sure this signal will be delivered and
		 * the queue will be processed.
		 */
		LOCK(&worker->lock);
		isc_queue_enqueue(worker->ievents_prio, (uintptr_t)event);
		SIGNAL(&worker->cond);
		UNLOCK(&worker->lock);
	} else {
		isc_queue_enqueue(worker->ievents, (uintptr_t)event);
	}
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

	NETMGR_TRACE_LOG("isc__nmsocket_attach():%p->references = %lu\n", rsock,
			 isc_refcount_current(&rsock->references) + 1);

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

	NETMGR_TRACE_LOG("nmsocket_cleanup():%p->references = %lu\n", sock,
			 isc_refcount_current(&sock->references));

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
		 * This was a parent socket; free the children.
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

	if (sock->timer_initialized) {
		sock->timer_initialized = false;
		/* We might be in timer callback */
		if (!uv_is_closing((uv_handle_t *)&sock->timer)) {
			uv_timer_stop(&sock->timer);
			uv_close((uv_handle_t *)&sock->timer, NULL);
		}
	}

	isc_astack_destroy(sock->inactivehandles);

	while ((uvreq = isc_astack_pop(sock->inactivereqs)) != NULL) {
		isc_mempool_put(sock->mgr->reqpool, uvreq);
	}

	isc_astack_destroy(sock->inactivereqs);
	sock->magic = 0;

	isc_mem_free(sock->mgr->mctx, sock->ah_frees);
	isc_mem_free(sock->mgr->mctx, sock->ah_handles);
	isc_mutex_destroy(&sock->lock);
	isc_condition_destroy(&sock->cond);
	isc_condition_destroy(&sock->scond);
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

	NETMGR_TRACE_LOG("isc___nmsocket_prep_destroy():%p->references = %lu\n",
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
		case isc_nm_tlssocket:
			isc__nm_tls_close(sock);
			break;
		case isc_nm_tlsdnssocket:
			isc__nm_tlsdns_close(sock);
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

	NETMGR_TRACE_LOG("isc__nmsocket_detach():%p->references = %lu\n", rsock,
			 isc_refcount_current(&rsock->references) - 1);

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
		(*sockp)->type == isc_nm_tcpdnslistener ||
		(*sockp)->type == isc_nm_tlsdnslistener);

	isc__nmsocket_detach(sockp);
}

void
isc___nmsocket_init(isc_nmsocket_t *sock, isc_nm_t *mgr, isc_nmsocket_type type,
		    isc_nmiface_t *iface FLARG) {
	uint16_t family;

	REQUIRE(sock != NULL);
	REQUIRE(mgr != NULL);
	REQUIRE(iface != NULL);

	family = iface->addr.type.sa.sa_family;

	*sock = (isc_nmsocket_t){ .type = type,
				  .iface = iface,
				  .fd = -1,
				  .ah_size = 32,
				  .inactivehandles = isc_astack_new(
					  mgr->mctx, ISC_NM_HANDLES_STACK_SIZE),
				  .inactivereqs = isc_astack_new(
					  mgr->mctx, ISC_NM_REQS_STACK_SIZE) };

#ifdef NETMGR_TRACE
	sock->backtrace_size = backtrace(sock->backtrace, TRACE_SIZE);
	ISC_LINK_INIT(sock, active_link);
	ISC_LIST_INIT(sock->active_handles);
	LOCK(&mgr->lock);
	ISC_LIST_APPEND(mgr->active_sockets, sock, active_link);
	UNLOCK(&mgr->lock);
#endif

	isc_nm_attach(mgr, &sock->mgr);
	sock->uv_handle.handle.data = sock;

	sock->ah_frees = isc_mem_allocate(mgr->mctx,
					  sock->ah_size * sizeof(size_t));
	sock->ah_handles = isc_mem_allocate(
		mgr->mctx, sock->ah_size * sizeof(isc_nmhandle_t *));
	ISC_LINK_INIT(&sock->quotacb, link);
	for (size_t i = 0; i < 32; i++) {
		sock->ah_frees[i] = i;
		sock->ah_handles[i] = NULL;
	}

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
	case isc_nm_tlsdnssocket:
	case isc_nm_tlsdnslistener:
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

	NETMGR_TRACE_LOG("isc__nmsocket_init():%p->references = %lu\n", sock,
			 isc_refcount_current(&sock->references));

	atomic_init(&sock->active, true);
	atomic_init(&sock->sequential, false);
	atomic_init(&sock->readpaused, false);
	atomic_init(&sock->closing, false);

	atomic_store(&sock->active_child_connections, 0);

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
	if (buf->base == NULL) {
		/* Empty buffer: might happen in case of error. */
		return;
	}
	worker = &sock->mgr->workers[sock->tid];

	REQUIRE(worker->recvbuf_inuse);
	if (sock->type == isc_nm_udpsocket && buf->base > worker->recvbuf &&
	    buf->base <= worker->recvbuf + ISC_NETMGR_RECVBUF_SIZE)
	{
		/* Can happen in case of out-of-order recvmmsg in libuv1.36 */
		return;
	}
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
	size_t handlenum;
	int pos;

	REQUIRE(VALID_NMSOCK(sock));

	handle = isc_astack_pop(sock->inactivehandles);

	if (handle == NULL) {
		handle = alloc_handle(sock);
	} else {
		isc_refcount_init(&handle->references, 1);
		INSIST(VALID_NMHANDLE(handle));
	}

	NETMGR_TRACE_LOG("isc__nmhandle_get():handle %p->references = %lu\n",
			 handle, isc_refcount_current(&handle->references));

	isc___nmsocket_attach(sock, &handle->sock FLARG_PASS);

#ifdef NETMGR_TRACE
	handle->backtrace_size = backtrace(handle->backtrace, TRACE_SIZE);
#endif

	if (peer != NULL) {
		memmove(&handle->peer, peer, sizeof(isc_sockaddr_t));
	} else {
		memmove(&handle->peer, &sock->peer, sizeof(isc_sockaddr_t));
	}

	if (local != NULL) {
		memmove(&handle->local, local, sizeof(isc_sockaddr_t));
	} else if (sock->iface != NULL) {
		memmove(&handle->local, &sock->iface->addr,
			sizeof(isc_sockaddr_t));
	} else {
		INSIST(0);
		ISC_UNREACHABLE();
	}

	LOCK(&sock->lock);
	/* We need to add this handle to the list of active handles */
	if ((size_t)atomic_load(&sock->ah) == sock->ah_size) {
		sock->ah_frees =
			isc_mem_reallocate(sock->mgr->mctx, sock->ah_frees,
					   sock->ah_size * 2 * sizeof(size_t));
		sock->ah_handles = isc_mem_reallocate(
			sock->mgr->mctx, sock->ah_handles,
			sock->ah_size * 2 * sizeof(isc_nmhandle_t *));

		for (size_t i = sock->ah_size; i < sock->ah_size * 2; i++) {
			sock->ah_frees[i] = i;
			sock->ah_handles[i] = NULL;
		}

		sock->ah_size *= 2;
	}

	handlenum = atomic_fetch_add(&sock->ah, 1);
	pos = sock->ah_frees[handlenum];

	INSIST(sock->ah_handles[pos] == NULL);
	sock->ah_handles[pos] = handle;
	handle->ah_pos = pos;
#ifdef NETMGR_TRACE
	ISC_LIST_APPEND(sock->active_handles, handle, active_link);
#endif
	UNLOCK(&sock->lock);

	if (sock->type == isc_nm_tcpsocket || sock->type == isc_nm_tlssocket ||
	    (sock->type == isc_nm_udpsocket && atomic_load(&sock->client)) ||
	    (sock->type == isc_nm_tcpdnssocket && atomic_load(&sock->client)) ||
	    (sock->type == isc_nm_tlsdnssocket && atomic_load(&sock->client)))
	{
		INSIST(sock->statichandle == NULL);

		/*
		 * statichandle must be assigned, not attached;
		 * otherwise, if a handle was detached elsewhere
		 * it could never reach 0 references, and the
		 * handle and socket would never be freed.
		 */
		sock->statichandle = handle;
	}

	return (handle);
}

void
isc__nmhandle_attach(isc_nmhandle_t *handle, isc_nmhandle_t **handlep FLARG) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(handlep != NULL && *handlep == NULL);

	NETMGR_TRACE_LOG("isc__nmhandle_attach():handle %p->references = %lu\n",
			 handle, isc_refcount_current(&handle->references) + 1);

	isc_refcount_increment(&handle->references);
	*handlep = handle;
}

bool
isc_nmhandle_is_stream(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	return (handle->sock->type == isc_nm_tcpsocket ||
		handle->sock->type == isc_nm_tcpdnssocket ||
		handle->sock->type == isc_nm_tlssocket ||
		handle->sock->type == isc_nm_tlsdnssocket);
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
	size_t handlenum;
	bool reuse = false;

	/*
	 * We do all of this under lock to avoid races with socket
	 * destruction.  We have to do this now, because at this point the
	 * socket is either unused or still attached to event->sock.
	 */
	LOCK(&sock->lock);

	INSIST(sock->ah_handles[handle->ah_pos] == handle);
	INSIST(sock->ah_size > handle->ah_pos);
	INSIST(atomic_load(&sock->ah) > 0);

#ifdef NETMGR_TRACE
	ISC_LIST_UNLINK(sock->active_handles, handle, active_link);
#endif

	sock->ah_handles[handle->ah_pos] = NULL;
	handlenum = atomic_fetch_sub(&sock->ah, 1) - 1;
	sock->ah_frees[handlenum] = handle->ah_pos;
	handle->ah_pos = 0;
	if (atomic_load(&sock->active)) {
		reuse = isc_astack_trypush(sock->inactivehandles, handle);
	}
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

	sock = handle->sock;
	if (sock->tid == isc_nm_tid()) {
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

static void
nmhandle_detach_cb(isc_nmhandle_t **handlep FLARG) {
	isc_nmsocket_t *sock = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(handlep != NULL);
	REQUIRE(VALID_NMHANDLE(*handlep));

	handle = *handlep;
	*handlep = NULL;

	NETMGR_TRACE_LOG("isc__nmhandle_detach():%p->references = %lu\n",
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
isc_nmhandle_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	REQUIRE(VALID_NMHANDLE(handle));

	switch (handle->sock->type) {
	case isc_nm_udpsocket:
		isc__nm_udp_settimeout(handle, timeout);
		break;
	case isc_nm_tcpsocket:
		isc__nm_tcp_settimeout(handle, timeout);
		break;
	case isc_nm_tcpdnssocket:
		isc__nm_tcpdns_settimeout(handle, timeout);
		break;
	case isc_nm_tlsdnssocket:
		isc__nm_tlsdns_settimeout(handle, timeout);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
		req = isc_mempool_get(mgr->reqpool);
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

	if (!isc__nmsocket_active(sock) ||
	    !isc_astack_trypush(sock->inactivereqs, req)) {
		isc_mempool_put(sock->mgr->reqpool, req);
	}

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
	case isc_nm_tlssocket:
		isc__nm_tls_send(handle, region, cb, cbarg);
		break;
	case isc_nm_tlsdnssocket:
		isc__nm_tlsdns_send(handle, region, cb, cbarg);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	case isc_nm_tlssocket:
		isc__nm_tls_read(handle, cb, cbarg);
		break;
	case isc_nm_tlsdnssocket:
		isc__nm_tlsdns_read(handle, cb, cbarg);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	case isc_nm_tlsdnssocket:
		isc__nm_tlsdns_cancelread(handle);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	case isc_nm_tlssocket:
		isc__nm_tls_pauseread(handle);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	case isc_nm_tlssocket:
		isc__nm_tls_resumeread(handle);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	case isc_nm_tlslistener:
		isc__nm_tls_stoplistening(sock);
		break;
	case isc_nm_tlsdnslistener:
		isc__nm_tlsdns_stoplistening(sock);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

void
isc__nm_connectcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
		  isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (eresult == ISC_R_SUCCESS) {
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

	if (eresult == ISC_R_SUCCESS) {
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
	isc_region_t region = { .base = (unsigned char *)uvreq->uvbuf.base,
				.length = uvreq->uvbuf.len };

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));
	REQUIRE(sock->tid == isc_nm_tid());

	uvreq->cb.recv(uvreq->handle, eresult, &region, uvreq->cbarg);

	isc__nm_uvreq_put(&uvreq, sock);
}

void
isc__nm_sendcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
	       isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (eresult == ISC_R_SUCCESS) {
		isc__netievent_sendcb_t ievent = { .sock = sock,
						   .req = uvreq,
						   .result = eresult };
		isc__nm_async_sendcb(NULL, (isc__netievent_t *)&ievent);
	} else {
		isc__netievent_sendcb_t *ievent = isc__nm_get_netievent_sendcb(
			sock->mgr, sock, uvreq, eresult);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
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
shutdown_walk_cb(uv_handle_t *handle, void *arg) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	UNUSED(arg);

	if (uv_is_closing(handle)) {
		return;
	}

	switch (handle->type) {
	case UV_UDP:
	case UV_TCP:
		break;
	default:
		return;
	}

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
	case isc_nm_tlsdnssocket:
		/* dummy now */
		break;
	case isc_nm_udplistener:
	case isc_nm_tcplistener:
	case isc_nm_tcpdnslistener:
	case isc_nm_tlsdnslistener:
		return;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

void
isc__nm_async_shutdown(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(ev0);
	uv_walk(&worker->loop, shutdown_walk_cb, NULL);
}

bool
isc__nm_acquire_interlocked(isc_nm_t *mgr) {
	LOCK(&mgr->lock);
	bool success = atomic_compare_exchange_strong(&mgr->interlocked,
						      &(bool){ false }, true);
	UNLOCK(&mgr->lock);
	return (success);
}

void
isc__nm_drop_interlocked(isc_nm_t *mgr) {
	LOCK(&mgr->lock);
	bool success = atomic_compare_exchange_strong(&mgr->interlocked,
						      &(bool){ true }, false);
	INSIST(success);
	BROADCAST(&mgr->wkstatecond);
	UNLOCK(&mgr->lock);
}

void
isc__nm_acquire_interlocked_force(isc_nm_t *mgr) {
	LOCK(&mgr->lock);
	while (!atomic_compare_exchange_strong(&mgr->interlocked,
					       &(bool){ false }, true))
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
isc__nm_socket_dontfrag(uv_os_sock_t fd, sa_family_t sa_family) {
	/*
	 * Set the Don't Fragment flag on IP packets
	 */
	if (sa_family == AF_INET6) {
#if defined(IPV6_DONTFRAG)
		if (setsockopt_on(fd, IPPROTO_IPV6, IPV6_DONTFRAG) == -1) {
			return (ISC_R_FAILURE);
		} else {
			return (ISC_R_SUCCESS);
		}
#elif defined(IPV6_MTU_DISCOVER)
		if (setsockopt(fd, IPPROTO_IPV6, IPV6_MTU_DISCOVER,
			       &(int){ IP_PMTUDISC_DO }, sizeof(int)) == -1)
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
		if (setsockopt_on(fd, IPPROTO_IP, IP_DONTFRAG) == -1) {
			return (ISC_R_FAILURE);
		} else {
			return (ISC_R_SUCCESS);
		}
#elif defined(IP_MTU_DISCOVER)
		if (setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER,
			       &(int){ IP_PMTUDISC_DO }, sizeof(int)) == -1)
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
	case isc_nm_tlssocket:
		return ("isc_nm_tlssocket");
	case isc_nm_tlslistener:
		return ("isc_nm_tlslistener");
	case isc_nm_tlsdnslistener:
		return ("isc_nm_tlsdnslistener");
	case isc_nm_tlsdnssocket:
		return ("isc_nm_tlsdnssocket");
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

static void
nmhandle_dump(isc_nmhandle_t *handle) {
	fprintf(stderr, "Active handle %p, refs %lu\n", handle,
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
	fprintf(stderr, "Active %s socket %p, type %s, refs %lu\n",
		sock->client ? "client" : "server", sock,
		nmsocket_type_totext(sock->type),
		isc_refcount_current(&sock->references));
	fprintf(stderr,
		"Parent %p, listener %p, server %p, statichandle = %p\n",
		sock->parent, sock->listener, sock->server, sock->statichandle);
	fprintf(stderr, "Flags:%s%s%s%s%s\n", sock->active ? " active" : "",
		sock->closing ? " closing" : "",
		sock->destroying ? " destroying" : "",
		sock->connecting ? " connecting" : "",
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
