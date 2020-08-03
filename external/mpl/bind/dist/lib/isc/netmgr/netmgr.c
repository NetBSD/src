/*	$NetBSD: netmgr.c,v 1.3 2020/08/03 17:23:42 christos Exp $	*/

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

#include <inttypes.h>
#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
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
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr-int.h"
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

ISC_THREAD_LOCAL int isc__nm_tid_v = ISC_NETMGR_TID_UNKNOWN;

static void
nmsocket_maybe_destroy(isc_nmsocket_t *sock);
static void
nmhandle_free(isc_nmsocket_t *sock, isc_nmhandle_t *handle);
static isc_threadresult_t
nm_thread(isc_threadarg_t worker0);
static void
async_cb(uv_async_t *handle);
static void
process_queue(isc__networker_t *worker, isc_queue_t *queue);

int
isc_nm_tid() {
	return (isc__nm_tid_v);
}

bool
isc__nm_in_netthread() {
	return (isc__nm_tid_v >= 0);
}

isc_nm_t *
isc_nm_start(isc_mem_t *mctx, uint32_t workers) {
	isc_nm_t *mgr = NULL;
	char name[32];

	mgr = isc_mem_get(mctx, sizeof(*mgr));
	*mgr = (isc_nm_t){ .nworkers = workers };

	isc_mem_attach(mctx, &mgr->mctx);
	isc_mutex_init(&mgr->lock);
	isc_condition_init(&mgr->wkstatecond);
	isc_refcount_init(&mgr->references, 1);
	atomic_init(&mgr->workers_running, 0);
	atomic_init(&mgr->workers_paused, 0);
	atomic_init(&mgr->maxudp, 0);
	atomic_init(&mgr->paused, false);
	atomic_init(&mgr->interlocked, false);

	/*
	 * Default TCP timeout values.
	 * May be updated by isc_nm_tcptimeouts().
	 */
	mgr->init = 30000;
	mgr->idle = 30000;
	mgr->keepalive = 30000;
	mgr->advertised = 30000;

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
		atomic_fetch_add_explicit(&mgr->workers_running, 1,
					  memory_order_relaxed);
		isc_thread_create(nm_thread, &mgr->workers[i], &worker->thread);

		snprintf(name, sizeof(name), "isc-net-%04zu", i);
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
		isc__netievent_t *event = NULL;

		LOCK(&mgr->workers[i].lock);
		mgr->workers[i].finished = true;
		UNLOCK(&mgr->workers[i].lock);
		event = isc__nm_get_ievent(mgr, netievent_stop);
		isc__nm_enqueue_ievent(&mgr->workers[i], event);
	}

	LOCK(&mgr->lock);
	while (atomic_load(&mgr->workers_running) > 0) {
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
}

void
isc_nm_pause(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(!isc__nm_in_netthread());

	atomic_store(&mgr->paused, true);
	isc__nm_acquire_interlocked_force(mgr);

	for (size_t i = 0; i < mgr->nworkers; i++) {
		isc__netievent_t *event = NULL;

		LOCK(&mgr->workers[i].lock);
		mgr->workers[i].paused = true;
		UNLOCK(&mgr->workers[i].lock);

		/*
		 * We have to issue a stop, otherwise the uv_run loop will
		 * run indefinitely!
		 */
		event = isc__nm_get_ievent(mgr, netievent_stop);
		isc__nm_enqueue_ievent(&mgr->workers[i], event);
	}

	LOCK(&mgr->lock);
	while (atomic_load_relaxed(&mgr->workers_paused) !=
	       atomic_load_relaxed(&mgr->workers_running))
	{
		WAIT(&mgr->wkstatecond, &mgr->lock);
	}
	UNLOCK(&mgr->lock);
}

void
isc_nm_resume(isc_nm_t *mgr) {
	REQUIRE(VALID_NM(mgr));
	REQUIRE(!isc__nm_in_netthread());

	for (size_t i = 0; i < mgr->nworkers; i++) {
		LOCK(&mgr->workers[i].lock);
		mgr->workers[i].paused = false;
		SIGNAL(&mgr->workers[i].cond);
		UNLOCK(&mgr->workers[i].lock);
	}
	isc__nm_drop_interlocked(mgr);

	/*
	 * We're not waiting for all the workers to come back to life;
	 * they eventually will, we don't care.
	 */
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
		event = isc__nm_get_ievent(mgr, netievent_shutdown);
		isc__nm_enqueue_ievent(&mgr->workers[i], event);
	}
}

void
isc_nm_destroy(isc_nm_t **mgr0) {
	isc_nm_t *mgr = NULL;

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
	while (isc_refcount_current(&mgr->references) > 1) {
		/*
		 * Sometimes libuv gets stuck, pausing and unpausing
		 * netmgr goes over all events in async queue for all
		 * the workers, and since it's done only on shutdown it
		 * doesn't cost us anything.
		 */
		isc_nm_pause(mgr);
		isc_nm_resume(mgr);
#ifdef WIN32
		_sleep(1000);
#else  /* ifdef WIN32 */
		usleep(1000000);
#endif /* ifdef WIN32 */
	}

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
isc_nm_tcp_settimeouts(isc_nm_t *mgr, uint32_t init, uint32_t idle,
		       uint32_t keepalive, uint32_t advertised) {
	REQUIRE(VALID_NM(mgr));

	mgr->init = init * 100;
	mgr->idle = idle * 100;
	mgr->keepalive = keepalive * 100;
	mgr->advertised = advertised * 100;
}

void
isc_nm_tcp_gettimeouts(isc_nm_t *mgr, uint32_t *initial, uint32_t *idle,
		       uint32_t *keepalive, uint32_t *advertised) {
	REQUIRE(VALID_NM(mgr));

	if (initial != NULL) {
		*initial = mgr->init / 100;
	}

	if (idle != NULL) {
		*idle = mgr->idle / 100;
	}

	if (keepalive != NULL) {
		*keepalive = mgr->keepalive / 100;
	}

	if (advertised != NULL) {
		*advertised = mgr->advertised / 100;
	}
}

/*
 * nm_thread is a single worker thread, that runs uv_run event loop
 * until asked to stop.
 */
static isc_threadresult_t
nm_thread(isc_threadarg_t worker0) {
	isc__networker_t *worker = (isc__networker_t *)worker0;

	isc__nm_tid_v = worker->id;
	isc_thread_setaffinity(isc__nm_tid_v);

	while (true) {
		int r = uv_run(&worker->loop, UV_RUN_DEFAULT);
		bool pausing = false;

		/*
		 * or there's nothing to do. In the first case - wait
		 * for condition. In the latter - timedwait
		 */
		LOCK(&worker->lock);
		while (worker->paused) {
			LOCK(&worker->mgr->lock);
			if (!pausing) {
				atomic_fetch_add_explicit(
					&worker->mgr->workers_paused, 1,
					memory_order_acquire);
				pausing = true;
			}

			SIGNAL(&worker->mgr->wkstatecond);
			UNLOCK(&worker->mgr->lock);

			WAIT(&worker->cond, &worker->lock);

			/* Process priority events */
			process_queue(worker, worker->ievents_prio);
		}
		if (pausing) {
			uint32_t wp = atomic_fetch_sub_explicit(
				&worker->mgr->workers_paused, 1,
				memory_order_release);
			if (wp == 1) {
				atomic_store(&worker->mgr->paused, false);
			}
		}
		bool finished = worker->finished;
		UNLOCK(&worker->lock);

		if (finished) {
			/*
			 * We need to launch the loop one more time
			 * in UV_RUN_NOWAIT mode to make sure that
			 * worker->async is closed, so that we can
			 * close the loop cleanly.  We don't care
			 * about the callback, as in this case we can
			 * be certain that uv_run() will eat the event.
			 *
			 * XXX: We may need to take steps here to ensure
			 * that all netmgr handles are freed.
			 */
			uv_close((uv_handle_t *)&worker->async, NULL);
			uv_run(&worker->loop, UV_RUN_NOWAIT);
			break;
		}

		if (r == 0) {
			/*
			 * XXX: uv_run() in UV_RUN_DEFAULT mode returns
			 * zero if there are still active uv_handles.
			 * This shouldn't happen, but if it does, we just
			 * keep checking until they're done. We nap for a
			 * tenth of a second on each loop so as not to burn
			 * CPU. (We do a conditional wait instead, but it
			 * seems like overkill for this case.)
			 */
#ifdef WIN32
			_sleep(100);
#else  /* ifdef WIN32 */
			usleep(100000);
#endif /* ifdef WIN32 */
		}

		/*
		 * Empty the async queue.
		 */
		process_queue(worker, worker->ievents_prio);
		process_queue(worker, worker->ievents);
	}

	LOCK(&worker->mgr->lock);
	atomic_fetch_sub_explicit(&worker->mgr->workers_running, 1,
				  memory_order_relaxed);
	SIGNAL(&worker->mgr->wkstatecond);
	UNLOCK(&worker->mgr->lock);

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
	process_queue(worker, worker->ievents_prio);
	process_queue(worker, worker->ievents);
}

static void
process_queue(isc__networker_t *worker, isc_queue_t *queue) {
	isc__netievent_t *ievent = NULL;

	while ((ievent = (isc__netievent_t *)isc_queue_dequeue(queue)) != NULL)
	{
		switch (ievent->type) {
		case netievent_stop:
			uv_stop(&worker->loop);
			isc_mempool_put(worker->mgr->evpool, ievent);
			return;
		case netievent_udplisten:
			isc__nm_async_udplisten(worker, ievent);
			break;
		case netievent_udpstop:
			isc__nm_async_udpstop(worker, ievent);
			break;
		case netievent_udpsend:
			isc__nm_async_udpsend(worker, ievent);
			break;
		case netievent_tcpconnect:
			isc__nm_async_tcpconnect(worker, ievent);
			break;
		case netievent_tcplisten:
			isc__nm_async_tcplisten(worker, ievent);
			break;
		case netievent_tcpchildaccept:
			isc__nm_async_tcpchildaccept(worker, ievent);
			break;
		case netievent_tcpaccept:
			isc__nm_async_tcpaccept(worker, ievent);
			break;
		case netievent_tcpstartread:
			isc__nm_async_tcp_startread(worker, ievent);
			break;
		case netievent_tcppauseread:
			isc__nm_async_tcp_pauseread(worker, ievent);
			break;
		case netievent_tcpsend:
			isc__nm_async_tcpsend(worker, ievent);
			break;
		case netievent_tcpdnssend:
			isc__nm_async_tcpdnssend(worker, ievent);
			break;
		case netievent_tcpstop:
			isc__nm_async_tcpstop(worker, ievent);
			break;
		case netievent_tcpclose:
			isc__nm_async_tcpclose(worker, ievent);
			break;
		case netievent_tcpdnsclose:
			isc__nm_async_tcpdnsclose(worker, ievent);
			break;
		case netievent_closecb:
			isc__nm_async_closecb(worker, ievent);
			break;
		case netievent_shutdown:
			isc__nm_async_shutdown(worker, ievent);
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}

		isc__nm_put_ievent(worker->mgr, ievent);
	}
}

void *
isc__nm_get_ievent(isc_nm_t *mgr, isc__netievent_type type) {
	isc__netievent_storage_t *event = isc_mempool_get(mgr->evpool);

	*event = (isc__netievent_storage_t){ .ni.type = type };
	return (event);
}

void
isc__nm_put_ievent(isc_nm_t *mgr, void *ievent) {
	isc_mempool_put(mgr->evpool, ievent);
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

void
isc_nmsocket_attach(isc_nmsocket_t *sock, isc_nmsocket_t **target) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(target != NULL && *target == NULL);

	if (sock->parent != NULL) {
		INSIST(sock->parent->parent == NULL); /* sanity check */
		isc_refcount_increment0(&sock->parent->references);
	} else {
		isc_refcount_increment0(&sock->references);
	}

	*target = sock;
}

/*
 * Free all resources inside a socket (including its children if any).
 */
static void
nmsocket_cleanup(isc_nmsocket_t *sock, bool dofree) {
	isc_nmhandle_t *handle = NULL;
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(!isc__nmsocket_active(sock));

	atomic_store(&sock->destroying, true);

	if (sock->parent == NULL && sock->children != NULL) {
		/*
		 * We shouldn't be here unless there are no active handles,
		 * so we can clean up and free the children.
		 */
		for (int i = 0; i < sock->nchildren; i++) {
			if (!atomic_load(&sock->children[i].destroying)) {
				nmsocket_cleanup(&sock->children[i], false);
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

	if (sock->tcphandle != NULL) {
		isc_nmhandle_unref(sock->tcphandle);
		sock->tcphandle = NULL;
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

	if (dofree) {
		isc_nm_t *mgr = sock->mgr;
		isc_mem_put(mgr->mctx, sock, sizeof(*sock));
		isc_nm_detach(&mgr);
	} else {
		isc_nm_detach(&sock->mgr);
	}
}

static void
nmsocket_maybe_destroy(isc_nmsocket_t *sock) {
	int active_handles;
	bool destroy = false;

	if (sock->parent != NULL) {
		/*
		 * This is a child socket and cannot be destroyed except
		 * as a side effect of destroying the parent, so let's go
		 * see if the parent is ready to be destroyed.
		 */
		nmsocket_maybe_destroy(sock->parent);
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
		for (int i = 0; i < sock->nchildren; i++) {
			LOCK(&sock->children[i].lock);
			active_handles += atomic_load(&sock->children[i].ah);
			UNLOCK(&sock->children[i].lock);
		}
	}

	if (active_handles == 0 || sock->tcphandle != NULL) {
		destroy = true;
	}

	if (destroy) {
		atomic_store(&sock->destroying, true);
		UNLOCK(&sock->lock);
		nmsocket_cleanup(sock, true);
	} else {
		UNLOCK(&sock->lock);
	}
}

void
isc__nmsocket_prep_destroy(isc_nmsocket_t *sock) {
	REQUIRE(sock->parent == NULL);

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
		for (int i = 0; i < sock->nchildren; i++) {
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
		case isc_nm_tcpsocket:
			isc__nm_tcp_close(sock);
			break;
		case isc_nm_tcpdnssocket:
			isc__nm_tcpdns_close(sock);
			break;
		default:
			break;
		}
	}

	nmsocket_maybe_destroy(sock);
}

void
isc_nmsocket_detach(isc_nmsocket_t **sockp) {
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

	if (isc_refcount_decrement(&rsock->references) == 1) {
		isc__nmsocket_prep_destroy(rsock);
	}
}

void
isc__nmsocket_init(isc_nmsocket_t *sock, isc_nm_t *mgr, isc_nmsocket_type type,
		   isc_nmiface_t *iface) {
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
	isc_refcount_init(&sock->references, 1);

	atomic_init(&sock->active, true);
	atomic_init(&sock->sequential, false);
	atomic_init(&sock->overlimit, false);
	atomic_init(&sock->processing, false);
	atomic_init(&sock->readpaused, false);

	sock->magic = NMSOCK_MAGIC;
}

void
isc__nm_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(isc__nm_in_netthread());
	REQUIRE(size <= ISC_NETMGR_RECVBUF_SIZE);

	worker = &sock->mgr->workers[sock->tid];
	INSIST(!worker->recvbuf_inuse);

	buf->base = worker->recvbuf;
	worker->recvbuf_inuse = true;
	buf->len = ISC_NETMGR_RECVBUF_SIZE;
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
	if (buf->base > worker->recvbuf &&
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
	isc_refcount_init(&handle->references, 1);

	return (handle);
}

isc_nmhandle_t *
isc__nmhandle_get(isc_nmsocket_t *sock, isc_sockaddr_t *peer,
		  isc_sockaddr_t *local) {
	isc_nmhandle_t *handle = NULL;
	size_t handlenum;
	int pos;

	REQUIRE(VALID_NMSOCK(sock));

	handle = isc_astack_pop(sock->inactivehandles);

	if (handle == NULL) {
		handle = alloc_handle(sock);
	} else {
		INSIST(VALID_NMHANDLE(handle));
		isc_refcount_increment0(&handle->references);
	}

	handle->sock = sock;
	if (peer != NULL) {
		memcpy(&handle->peer, peer, sizeof(isc_sockaddr_t));
	} else {
		memcpy(&handle->peer, &sock->peer, sizeof(isc_sockaddr_t));
	}

	if (local != NULL) {
		memcpy(&handle->local, local, sizeof(isc_sockaddr_t));
	} else if (sock->iface != NULL) {
		memcpy(&handle->local, &sock->iface->addr,
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
	UNLOCK(&sock->lock);

	if (sock->type == isc_nm_tcpsocket) {
		INSIST(sock->tcphandle == NULL);
		sock->tcphandle = handle;
	}

	return (handle);
}

void
isc_nmhandle_ref(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	isc_refcount_increment(&handle->references);
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
	/*
	 * We do all of this under lock to avoid races with socket
	 * destruction.  We have to do this now, because at this point the
	 * socket is either unused or still attached to event->sock.
	 */
	LOCK(&sock->lock);

	INSIST(sock->ah_handles[handle->ah_pos] == handle);
	INSIST(sock->ah_size > handle->ah_pos);
	INSIST(atomic_load(&sock->ah) > 0);

	sock->ah_handles[handle->ah_pos] = NULL;
	size_t handlenum = atomic_fetch_sub(&sock->ah, 1) - 1;
	sock->ah_frees[handlenum] = handle->ah_pos;
	handle->ah_pos = 0;
	bool reuse = false;
	if (atomic_load(&sock->active)) {
		reuse = isc_astack_trypush(sock->inactivehandles, handle);
	}
	if (!reuse) {
		nmhandle_free(sock, handle);
	}
	UNLOCK(&sock->lock);
}

void
isc_nmhandle_unref(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL, *tmp = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

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

	/*
	 * Temporarily reference the socket to ensure that it can't
	 * be deleted by another thread while we're deactivating the
	 * handle.
	 */
	isc_nmsocket_attach(sock, &tmp);
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
			isc__netievent_closecb_t *event = isc__nm_get_ievent(
				sock->mgr, netievent_closecb);
			/*
			 * The socket will be finally detached by the closecb
			 * event handler.
			 */
			isc_nmsocket_attach(sock, &event->sock);
			isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
					       (isc__netievent_t *)event);
		}
	}

	isc_nmsocket_detach(&tmp);
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
isc__nm_uvreq_get(isc_nm_t *mgr, isc_nmsocket_t *sock) {
	isc__nm_uvreq_t *req = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(VALID_NMSOCK(sock));

	if (sock != NULL && atomic_load(&sock->active)) {
		/* Try to reuse one */
		req = isc_astack_pop(sock->inactivereqs);
	}

	if (req == NULL) {
		req = isc_mempool_get(mgr->reqpool);
	}

	*req = (isc__nm_uvreq_t){ .magic = 0 };
	req->uv_req.req.data = req;
	isc_nmsocket_attach(sock, &req->sock);
	req->magic = UVREQ_MAGIC;

	return (req);
}

void
isc__nm_uvreq_put(isc__nm_uvreq_t **req0, isc_nmsocket_t *sock) {
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

	if (!atomic_load(&sock->active) ||
	    !isc_astack_trypush(sock->inactivereqs, req)) {
		isc_mempool_put(sock->mgr->reqpool, req);
	}

	if (handle != NULL) {
		isc_nmhandle_unref(handle);
	}

	isc_nmsocket_detach(&sock);
}

isc_result_t
isc_nm_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
	    void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));

	switch (handle->sock->type) {
	case isc_nm_udpsocket:
	case isc_nm_udplistener:
		return (isc__nm_udp_send(handle, region, cb, cbarg));
	case isc_nm_tcpsocket:
		return (isc__nm_tcp_send(handle, region, cb, cbarg));
	case isc_nm_tcpdnssocket:
		return (isc__nm_tcpdns_send(handle, region, cb, cbarg));
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

isc_result_t
isc_nm_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));

	switch (handle->sock->type) {
	case isc_nm_tcpsocket:
		return (isc__nm_tcp_read(handle, cb, cbarg));
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

isc_result_t
isc_nm_pauseread(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	switch (sock->type) {
	case isc_nm_tcpsocket:
		return (isc__nm_tcp_pauseread(sock));
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

isc_result_t
isc_nm_resumeread(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	switch (sock->type) {
	case isc_nm_tcpsocket:
		return (isc__nm_tcp_resumeread(sock));
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
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

void
isc__nm_async_closecb(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_closecb_t *ievent = (isc__netievent_closecb_t *)ev0;

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());
	REQUIRE(ievent->sock->closehandle_cb != NULL);

	UNUSED(worker);

	ievent->sock->closehandle_cb(ievent->sock);
	isc_nmsocket_detach(&ievent->sock);
}

static void
shutdown_walk_cb(uv_handle_t *handle, void *arg) {
	UNUSED(arg);

	switch (handle->type) {
	case UV_TCP:
		isc__nm_tcp_shutdown(uv_handle_get_data(handle));
		break;
	default:
		break;
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
