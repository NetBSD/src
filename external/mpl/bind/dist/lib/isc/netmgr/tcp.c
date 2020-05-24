/*	$NetBSD: tcp.c,v 1.1.1.1 2020/05/24 19:36:46 christos Exp $	*/

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

#include <libgen.h>
#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/quota.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/stdtime.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr-int.h"
#include "uv-compat.h"

static atomic_uint_fast32_t last_tcpquota_log = ATOMIC_VAR_INIT(0);

static bool
can_log_tcp_quota() {
	isc_stdtime_t now, last;

	isc_stdtime_get(&now);
	last = atomic_exchange_relaxed(&last_tcpquota_log, now);
	if (now != last) {
		return (true);
	}

	return (false);
}

static int
tcp_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);

static void
tcp_close_direct(isc_nmsocket_t *sock);

static isc_result_t
tcp_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);
static void
tcp_connect_cb(uv_connect_t *uvreq, int status);

static void
tcp_connection_cb(uv_stream_t *server, int status);

static void
read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

static void
tcp_close_cb(uv_handle_t *uvhandle);

static void
stoplistening(isc_nmsocket_t *sock);
static void
tcp_listenclose_cb(uv_handle_t *handle);

static int
tcp_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc__networker_t *worker = NULL;
	int r;

	REQUIRE(isc__nm_in_netthread());

	worker = &sock->mgr->workers[isc_nm_tid()];

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	if (r != 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPENFAIL]);
		return (r);
	}

	if (req->local.length != 0) {
		r = uv_tcp_bind(&sock->uv_handle.tcp, &req->local.type.sa, 0);
		if (r != 0) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_BINDFAIL]);
			tcp_close_direct(sock);
			return (r);
		}
	}
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	r = uv_tcp_connect(&req->uv_req.connect, &sock->uv_handle.tcp,
			   &req->peer.type.sa, tcp_connect_cb);
	return (r);
}

void
isc__nm_async_tcpconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpconnect_t *ievent =
		(isc__netievent_tcpconnect_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	int r;

	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(worker->id == ievent->req->sock->mgr->workers[isc_nm_tid()].id);

	r = tcp_connect_direct(sock, req);
	if (r != 0) {
		/* We need to issue callbacks ourselves */
		tcp_connect_cb(&req->uv_req.connect, r);
	}
}

static void
tcp_connect_cb(uv_connect_t *uvreq, int status) {
	isc__nm_uvreq_t *req = (isc__nm_uvreq_t *)uvreq->data;
	isc_nmsocket_t *sock = NULL;
	sock = uv_handle_get_data((uv_handle_t *)uvreq->handle);

	REQUIRE(VALID_UVREQ(req));

	if (status == 0) {
		isc_result_t result;
		isc_nmhandle_t *handle = NULL;
		struct sockaddr_storage ss;

		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CONNECT]);
		uv_tcp_getpeername(&sock->uv_handle.tcp, (struct sockaddr *)&ss,
				   &(int){ sizeof(ss) });
		result = isc_sockaddr_fromsockaddr(&sock->peer,
						   (struct sockaddr *)&ss);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		handle = isc__nmhandle_get(sock, NULL, NULL);
		req->cb.connect(handle, ISC_R_SUCCESS, req->cbarg);
	} else {
		/*
		 * TODO:
		 * Handle the connect error properly and free the socket.
		 */
		isc__nm_incstats(sock->mgr,
				 sock->statsindex[STATID_CONNECTFAIL]);
		req->cb.connect(NULL, isc__nm_uverr2result(status), req->cbarg);
	}

	isc__nm_uvreq_put(&req, sock);
}

isc_result_t
isc_nm_listentcp(isc_nm_t *mgr, isc_nmiface_t *iface, isc_nm_cb_t cb,
		 void *cbarg, size_t extrahandlesize, int backlog,
		 isc_quota_t *quota, isc_nmsocket_t **sockp) {
	isc_nmsocket_t *nsock = NULL;
	isc__netievent_tcplisten_t *ievent = NULL;

	REQUIRE(VALID_NM(mgr));

	nsock = isc_mem_get(mgr->mctx, sizeof(*nsock));
	isc__nmsocket_init(nsock, mgr, isc_nm_tcplistener, iface);
	nsock->nchildren = mgr->nworkers;
	atomic_init(&nsock->rchildren, mgr->nworkers);
	nsock->children = isc_mem_get(mgr->mctx,
				      mgr->nworkers * sizeof(*nsock));
	memset(nsock->children, 0, mgr->nworkers * sizeof(*nsock));
	nsock->rcb.accept = cb;
	nsock->rcbarg = cbarg;
	nsock->extrahandlesize = extrahandlesize;
	nsock->backlog = backlog;
	nsock->result = ISC_R_SUCCESS;
	if (quota != NULL) {
		/*
		 * We don't attach to quota, just assign - to avoid
		 * increasing quota unnecessarily.
		 */
		nsock->pquota = quota;
	}

	ievent = isc__nm_get_ievent(mgr, netievent_tcplisten);
	ievent->sock = nsock;
	if (isc__nm_in_netthread()) {
		nsock->tid = isc_nm_tid();
		isc__nm_async_tcplisten(&mgr->workers[nsock->tid],
					(isc__netievent_t *)ievent);
		isc__nm_put_ievent(mgr, ievent);
	} else {
		nsock->tid = isc_random_uniform(mgr->nworkers);
		isc__nm_enqueue_ievent(&mgr->workers[nsock->tid],
				       (isc__netievent_t *)ievent);

		LOCK(&nsock->lock);
		while (!atomic_load(&nsock->listening) &&
		       !atomic_load(&nsock->listen_error)) {
			WAIT(&nsock->cond, &nsock->lock);
		}
		UNLOCK(&nsock->lock);
	}

	if (nsock->result == ISC_R_SUCCESS) {
		*sockp = nsock;
		return (ISC_R_SUCCESS);
	} else {
		isc_result_t result = nsock->result;
		isc_nmsocket_detach(&nsock);
		return (result);
	}
}

/*
 * For multi-threaded TCP listening, we create a single "parent" socket,
 * bind to it, and then pass its uv_handle to a set of child sockets, one
 * per worker. For thread safety, the passing of the socket's uv_handle has
 * to be done via IPC socket.
 *
 * This design pattern is ugly but it's what's recommended by the libuv
 * documentation.  (A prior version of libuv had uv_export() and
 * uv_import() functions which would have simplified this greatly, but
 * they have been deprecated and removed.)
 */
void
isc__nm_async_tcplisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcplisten_t *ievent = (isc__netievent_tcplisten_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	struct sockaddr_storage sname;
	int r, flags = 0, snamelen = sizeof(sname);

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->type == isc_nm_tcplistener);

	/* Initialize children now to make cleaning up easier */
	for (int i = 0; i < sock->nchildren; i++) {
		isc_nmsocket_t *csock = &sock->children[i];

		isc__nmsocket_init(csock, sock->mgr, isc_nm_tcpchildlistener,
				   sock->iface);
		csock->parent = sock;
		csock->tid = i;
		csock->pquota = sock->pquota;
		csock->backlog = sock->backlog;
		csock->extrahandlesize = sock->extrahandlesize;

		INSIST(csock->rcb.recv == NULL && csock->rcbarg == NULL);
		csock->rcb.accept = sock->rcb.accept;
		csock->rcbarg = sock->rcbarg;
		csock->fd = -1;
	}

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	if (r != 0) {
		/* It was never opened */
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPENFAIL]);
		atomic_store(&sock->closed, true);
		sock->result = isc__nm_uverr2result(r);
		atomic_store(&sock->listen_error, true);
		goto done;
	}

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPEN]);

	if (sock->iface->addr.type.sa.sa_family == AF_INET6) {
		flags = UV_TCP_IPV6ONLY;
	}

	r = uv_tcp_bind(&sock->uv_handle.tcp, &sock->iface->addr.type.sa,
			flags);
	if (r != 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		uv_close(&sock->uv_handle.handle, tcp_close_cb);
		sock->result = isc__nm_uverr2result(r);
		atomic_store(&sock->listen_error, true);
		goto done;
	}

	/*
	 * By doing this now, we can find out immediately whether bind()
	 * failed, and quit if so. (uv_bind() uses a delayed error,
	 * initially returning success even if bind() fails, and this
	 * could cause a deadlock later if we didn't check first.)
	 */
	r = uv_tcp_getsockname(&sock->uv_handle.tcp, (struct sockaddr *)&sname,
			       &snamelen);
	if (r != 0) {
		uv_close(&sock->uv_handle.handle, tcp_close_cb);
		sock->result = isc__nm_uverr2result(r);
		atomic_store(&sock->listen_error, true);
		goto done;
	}

	uv_handle_set_data(&sock->uv_handle.handle, sock);

	/*
	 * For each worker, we send a 'tcpchildlisten' event with
	 * the exported socket.
	 */
	for (int i = 0; i < sock->nchildren; i++) {
		isc_nmsocket_t *csock = &sock->children[i];
		isc__netievent_tcpchildlisten_t *event = NULL;

		event = isc__nm_get_ievent(csock->mgr,
					   netievent_tcpchildlisten);
		r = isc_uv_export(&sock->uv_handle.stream, &event->streaminfo);
		if (r != 0) {
			isc_log_write(
				isc_lctx, ISC_LOGCATEGORY_GENERAL,
				ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
				"uv_export failed: %s",
				isc_result_totext(isc__nm_uverr2result(r)));
			isc__nm_put_ievent(sock->mgr, event);
			continue;
		}
		event->sock = csock;
		if (csock->tid == isc_nm_tid()) {
			isc__nm_async_tcpchildlisten(&sock->mgr->workers[i],
						     (isc__netievent_t *)event);
			isc__nm_put_ievent(sock->mgr, event);
		} else {
			isc__nm_enqueue_ievent(&sock->mgr->workers[i],
					       (isc__netievent_t *)event);
		}
	}

	atomic_store(&sock->listening, true);

done:
	LOCK(&sock->lock);
	SIGNAL(&sock->cond);
	UNLOCK(&sock->lock);
	return;
}

/*
 * Connect to the parent socket and be ready to receive the uv_handle
 * for the socket we'll be listening on.
 */
void
isc__nm_async_tcpchildlisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpchildlisten_t *ievent =
		(isc__netievent_tcpchildlisten_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	int r;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->type == isc_nm_tcpchildlistener);

	worker = &sock->mgr->workers[isc_nm_tid()];

	uv_tcp_init(&worker->loop, (uv_tcp_t *)&sock->uv_handle.tcp);
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	r = isc_uv_import(&sock->uv_handle.stream, &ievent->streaminfo);
	if (r != 0) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
			      "uv_import failed: %s",
			      isc_result_totext(isc__nm_uverr2result(r)));
		return;
	}

	r = uv_listen((uv_stream_t *)&sock->uv_handle.tcp, sock->backlog,
		      tcp_connection_cb);

	if (r != 0) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
			      "uv_listen failed: %s",
			      isc_result_totext(isc__nm_uverr2result(r)));
		return;
	}
}

void
isc__nm_tcp_stoplistening(isc_nmsocket_t *sock) {
	isc__netievent_tcpstop_t *ievent = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(!isc__nm_in_netthread());

	ievent = isc__nm_get_ievent(sock->mgr, netievent_tcpstop);
	isc_nmsocket_attach(sock, &ievent->sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tcpstop(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpstop_t *ievent = (isc__netievent_tcpstop_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcplistener);

	/*
	 * If network manager is interlocked, re-enqueue the event for later.
	 */
	if (!isc__nm_acquire_interlocked(sock->mgr)) {
		isc__netievent_tcpstop_t *event = NULL;

		event = isc__nm_get_ievent(sock->mgr, netievent_tcpstop);
		event->sock = sock;
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)event);
	} else {
		stoplistening(sock);
		isc__nm_drop_interlocked(sock->mgr);
	}
}

static void
stoplistening(isc_nmsocket_t *sock) {
	for (int i = 0; i < sock->nchildren; i++) {
		isc__netievent_tcpchildstop_t *event = NULL;

		/*
		 * We can ignore the overhead of event allocation because
		 * stoplistening is a rare event, and doing it this way
		 * simplifies sock reference counting.
		 */
		event = isc__nm_get_ievent(sock->mgr, netievent_tcpchildstop);
		isc_nmsocket_attach(&sock->children[i], &event->sock);

		if (isc_nm_tid() == sock->children[i].tid) {
			isc__nm_async_tcpchildstop(&sock->mgr->workers[i],
						   (isc__netievent_t *)event);
			isc__nm_put_ievent(sock->mgr, event);
		} else {
			isc__nm_enqueue_ievent(&sock->mgr->workers[i],
					       (isc__netievent_t *)event);
		}
	}

	LOCK(&sock->lock);
	while (atomic_load_relaxed(&sock->rchildren) > 0) {
		WAIT(&sock->cond, &sock->lock);
	}
	UNLOCK(&sock->lock);
	uv_close((uv_handle_t *)&sock->uv_handle.tcp, tcp_listenclose_cb);
}

void
isc__nm_async_tcpchildstop(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpchildstop_t *ievent =
		(isc__netievent_tcpchildstop_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(isc_nm_tid() == sock->tid);
	REQUIRE(sock->type == isc_nm_tcpchildlistener);
	REQUIRE(sock->parent != NULL);

	/*
	 * rchildren is atomic, but we still need to change it
	 * under a lock because the parent is waiting on conditional
	 * and without it we might deadlock.
	 */
	LOCK(&sock->parent->lock);
	atomic_fetch_sub(&sock->parent->rchildren, 1);
	UNLOCK(&sock->parent->lock);

	uv_close((uv_handle_t *)&sock->uv_handle.tcp, tcp_listenclose_cb);
	BROADCAST(&sock->parent->cond);
}

/*
 * This callback is used for closing both child and parent listening
 * sockets; that's why we need to choose the proper lock.
 */
static void
tcp_listenclose_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	isc_mutex_t *lock = ((sock->parent != NULL) ? &sock->parent->lock
						    : &sock->lock);

	LOCK(lock);
	atomic_store(&sock->closed, true);
	atomic_store(&sock->listening, false);
	sock->pquota = NULL;
	UNLOCK(lock);

	isc_nmsocket_detach(&sock);
}

static void
readtimeout_cb(uv_timer_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)handle);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	/*
	 * Socket is actively processing something, so restart the timer
	 * and return.
	 */
	if (atomic_load(&sock->processing)) {
		uv_timer_start(handle, readtimeout_cb, sock->read_timeout, 0);
		return;
	}

	/*
	 * Timeout; stop reading and process whatever we have.
	 */
	uv_read_stop(&sock->uv_handle.stream);
	if (sock->quota) {
		isc_quota_detach(&sock->quota);
	}
	sock->rcb.recv(sock->tcphandle, NULL, sock->rcbarg);
}

isc_result_t
isc__nm_tcp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_startread_t *ievent = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;
	sock->rcb.recv = cb;
	sock->rcbarg = cbarg;

	ievent = isc__nm_get_ievent(sock->mgr, netievent_tcpstartread);
	ievent->sock = sock;

	if (sock->tid == isc_nm_tid()) {
		isc__nm_async_tcp_startread(&sock->mgr->workers[sock->tid],
					    (isc__netievent_t *)ievent);
		isc__nm_put_ievent(sock->mgr, ievent);
	} else {
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}

	return (ISC_R_SUCCESS);
}

void
isc__nm_async_tcp_startread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_startread_t *ievent = (isc__netievent_startread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	int r;

	REQUIRE(worker->id == isc_nm_tid());
	if (sock->read_timeout != 0) {
		if (!sock->timer_initialized) {
			uv_timer_init(&worker->loop, &sock->timer);
			uv_handle_set_data((uv_handle_t *)&sock->timer, sock);
			sock->timer_initialized = true;
		}
		uv_timer_start(&sock->timer, readtimeout_cb, sock->read_timeout,
			       0);
	}

	r = uv_read_start(&sock->uv_handle.stream, isc__nm_alloc_cb, read_cb);
	if (r != 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_RECVFAIL]);
	}
}

isc_result_t
isc__nm_tcp_pauseread(isc_nmsocket_t *sock) {
	isc__netievent_pauseread_t *ievent = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	if (atomic_load(&sock->readpaused)) {
		return (ISC_R_SUCCESS);
	}

	atomic_store(&sock->readpaused, true);
	ievent = isc__nm_get_ievent(sock->mgr, netievent_tcppauseread);
	ievent->sock = sock;

	if (sock->tid == isc_nm_tid()) {
		isc__nm_async_tcp_pauseread(&sock->mgr->workers[sock->tid],
					    (isc__netievent_t *)ievent);
		isc__nm_put_ievent(sock->mgr, ievent);
	} else {
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}

	return (ISC_R_SUCCESS);
}

void
isc__nm_async_tcp_pauseread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_pauseread_t *ievent = (isc__netievent_pauseread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(worker->id == isc_nm_tid());

	if (sock->timer_initialized) {
		uv_timer_stop(&sock->timer);
	}
	uv_read_stop(&sock->uv_handle.stream);
}

isc_result_t
isc__nm_tcp_resumeread(isc_nmsocket_t *sock) {
	isc__netievent_startread_t *ievent = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->rcb.recv != NULL);

	if (!atomic_load(&sock->readpaused)) {
		return (ISC_R_SUCCESS);
	}

	atomic_store(&sock->readpaused, false);

	ievent = isc__nm_get_ievent(sock->mgr, netievent_tcpstartread);
	ievent->sock = sock;

	if (sock->tid == isc_nm_tid()) {
		isc__nm_async_tcp_startread(&sock->mgr->workers[sock->tid],
					    (isc__netievent_t *)ievent);
		isc__nm_put_ievent(sock->mgr, ievent);
	} else {
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}

	return (ISC_R_SUCCESS);
}

static void
read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)stream);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(buf != NULL);

	if (nread >= 0) {
		isc_region_t region = { .base = (unsigned char *)buf->base,
					.length = nread };

		if (sock->rcb.recv != NULL) {
			sock->rcb.recv(sock->tcphandle, &region, sock->rcbarg);
		}

		sock->read_timeout = (atomic_load(&sock->keepalive)
					      ? sock->mgr->keepalive
					      : sock->mgr->idle);

		if (sock->timer_initialized && sock->read_timeout != 0) {
			/* The timer will be updated */
			uv_timer_start(&sock->timer, readtimeout_cb,
				       sock->read_timeout, 0);
		}

		isc__nm_free_uvbuf(sock, buf);
		return;
	}

	isc__nm_free_uvbuf(sock, buf);

	/*
	 * This might happen if the inner socket is closing.  It means that
	 * it's detached, so the socket will be closed.
	 */
	if (sock->rcb.recv != NULL) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_RECVFAIL]);
		sock->rcb.recv(sock->tcphandle, NULL, sock->rcbarg);
	}

	/*
	 * We don't need to clean up now; the socket will be closed and
	 * resources and quota reclaimed when handle is freed in
	 * isc__nm_tcp_close().
	 */
}

static isc_result_t
accept_connection(isc_nmsocket_t *ssock) {
	isc_result_t result;
	isc_quota_t *quota = NULL;
	isc_nmsocket_t *csock = NULL;
	isc__networker_t *worker = NULL;
	isc_nmhandle_t *handle = NULL;
	struct sockaddr_storage ss;
	isc_sockaddr_t local;
	int r;
	bool overquota = false;

	REQUIRE(VALID_NMSOCK(ssock));
	REQUIRE(ssock->tid == isc_nm_tid());

	if (!atomic_load_relaxed(&ssock->active) ||
	    atomic_load_relaxed(&ssock->mgr->closing))
	{
		/* We're closing, bail */
		return (ISC_R_CANCELED);
	}

	if (ssock->pquota != NULL) {
		result = isc_quota_attach(ssock->pquota, &quota);

		/*
		 * We share the quota between all TCP sockets. Others
		 * may have used up all the quota slots, in which case
		 * this socket could starve. So we only fail here if we
		 * already had at least one active connection on this
		 * socket. This guarantees that we'll maintain some level
		 * of service while over quota, and will resume normal
		 * service when the quota comes back down.
		 */
		if (result != ISC_R_SUCCESS) {
			ssock->overquota++;
			overquota = true;
			if (ssock->conns > 0) {
				isc__nm_incstats(
					ssock->mgr,
					ssock->statsindex[STATID_ACCEPTFAIL]);
				return (result);
			}
		}
	}

	isc__nm_incstats(ssock->mgr, ssock->statsindex[STATID_ACCEPT]);

	csock = isc_mem_get(ssock->mgr->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(csock, ssock->mgr, isc_nm_tcpsocket, ssock->iface);
	csock->tid = isc_nm_tid();
	csock->extrahandlesize = ssock->extrahandlesize;
	csock->quota = quota;
	quota = NULL;

	worker = &ssock->mgr->workers[isc_nm_tid()];
	uv_tcp_init(&worker->loop, &csock->uv_handle.tcp);

	r = uv_accept(&ssock->uv_handle.stream, &csock->uv_handle.stream);
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto error;
	}

	r = uv_tcp_getpeername(&csock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto error;
	}

	result = isc_sockaddr_fromsockaddr(&csock->peer,
					   (struct sockaddr *)&ss);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	r = uv_tcp_getsockname(&csock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto error;
	}
	result = isc_sockaddr_fromsockaddr(&local, (struct sockaddr *)&ss);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	isc_nmsocket_attach(ssock, &csock->server);
	ssock->conns++;

	handle = isc__nmhandle_get(csock, NULL, &local);

	INSIST(ssock->rcb.accept != NULL);
	csock->read_timeout = ssock->mgr->init;
	ssock->rcb.accept(handle, ISC_R_SUCCESS, ssock->rcbarg);
	isc_nmsocket_detach(&csock);

	return (ISC_R_SUCCESS);

error:
	/*
	 * Detach it early to make room for other connections, otherwise
	 * it'd be detached later asynchronously clogging the quota.
	 */
	if (csock->quota != NULL) {
		isc_quota_detach(&csock->quota);
	}
	if (overquota) {
		ssock->overquota--;
	}
	/* We need to detach it properly to make sure uv_close is called. */
	isc_nmsocket_detach(&csock);
	return (result);
}

static void
tcp_connection_cb(uv_stream_t *server, int status) {
	isc_nmsocket_t *ssock = uv_handle_get_data((uv_handle_t *)server);
	isc_result_t result;

	UNUSED(status);

	result = accept_connection(ssock);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOCONN) {
		if ((result != ISC_R_QUOTA && result != ISC_R_SOFTQUOTA) ||
		    can_log_tcp_quota()) {
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
				      "TCP connection failed: %s",
				      isc_result_totext(result));
		}
	}
}

isc_result_t
isc__nm_tcp_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		 void *cbarg) {
	isc_nmsocket_t *sock = handle->sock;
	isc__netievent_tcpsend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(sock->type == isc_nm_tcpsocket);

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;
	uvreq->handle = handle;
	isc_nmhandle_ref(uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	if (sock->tid == isc_nm_tid()) {
		/*
		 * If we're in the same thread as the socket we can send the
		 * data directly
		 */
		return (tcp_send_direct(sock, uvreq));
	} else {
		/*
		 * We need to create an event and pass it using async channel
		 */
		ievent = isc__nm_get_ievent(sock->mgr, netievent_tcpsend);
		ievent->sock = sock;
		ievent->req = uvreq;
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
		return (ISC_R_SUCCESS);
	}

	return (ISC_R_UNEXPECTED);
}

static void
tcp_send_cb(uv_write_t *req, int status) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__nm_uvreq_t *uvreq = (isc__nm_uvreq_t *)req->data;

	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (status < 0) {
		result = isc__nm_uverr2result(status);
		isc__nm_incstats(uvreq->sock->mgr,
				 uvreq->sock->statsindex[STATID_SENDFAIL]);
	}

	uvreq->cb.send(uvreq->handle, result, uvreq->cbarg);
	isc_nmhandle_unref(uvreq->handle);
	isc__nm_uvreq_put(&uvreq, uvreq->handle->sock);
}

/*
 * Handle 'tcpsend' async event - send a packet on the socket
 */
void
isc__nm_async_tcpsend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc_result_t result;
	isc__netievent_tcpsend_t *ievent = (isc__netievent_tcpsend_t *)ev0;

	REQUIRE(worker->id == ievent->sock->tid);

	if (!atomic_load(&ievent->sock->active)) {
		return;
	}

	result = tcp_send_direct(ievent->sock, ievent->req);
	if (result != ISC_R_SUCCESS) {
		ievent->req->cb.send(ievent->req->handle, result,
				     ievent->req->cbarg);
		isc__nm_uvreq_put(&ievent->req, ievent->req->handle->sock);
	}
}

static isc_result_t
tcp_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	int r;

	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tcpsocket);

	isc_nmhandle_ref(req->handle);
	r = uv_write(&req->uv_req.write, &sock->uv_handle.stream, &req->uvbuf,
		     1, tcp_send_cb);
	if (r < 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
		req->cb.send(NULL, isc__nm_uverr2result(r), req->cbarg);
		isc__nm_uvreq_put(&req, sock);
		return (isc__nm_uverr2result(r));
	}

	return (ISC_R_SUCCESS);
}

static void
tcp_close_cb(uv_handle_t *uvhandle) {
	isc_nmsocket_t *sock = uv_handle_get_data(uvhandle);

	REQUIRE(VALID_NMSOCK(sock));

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CLOSE]);
	atomic_store(&sock->closed, true);
	isc__nmsocket_prep_destroy(sock);
}

static void
timer_close_cb(uv_handle_t *uvhandle) {
	isc_nmsocket_t *sock = uv_handle_get_data(uvhandle);

	REQUIRE(VALID_NMSOCK(sock));

	isc_nmsocket_detach(&sock->server);
	uv_close(&sock->uv_handle.handle, tcp_close_cb);
}

static void
tcp_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tcpsocket);
	isc_nmsocket_t *ssock = sock->server;

	if (sock->quota != NULL) {
		isc_quota_detach(&sock->quota);
	}
	if (ssock != NULL) {
		ssock->conns--;
		while (ssock->conns == 0 && ssock->overquota > 0) {
			ssock->overquota--;
			isc_result_t result = accept_connection(ssock);
			if (result == ISC_R_SUCCESS || result == ISC_R_NOCONN) {
				continue;
			}
			if ((result != ISC_R_QUOTA &&
			     result != ISC_R_SOFTQUOTA) ||
			    can_log_tcp_quota()) {
				isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
					      ISC_LOGMODULE_NETMGR,
					      ISC_LOG_ERROR,
					      "TCP connection failed: %s",
					      isc_result_totext(result));
			}
		}
	}
	if (sock->timer_initialized) {
		sock->timer_initialized = false;
		uv_timer_stop(&sock->timer);
		uv_close((uv_handle_t *)&sock->timer, timer_close_cb);
	} else {
		if (sock->server != NULL) {
			isc_nmsocket_detach(&sock->server);
		}
		uv_close(&sock->uv_handle.handle, tcp_close_cb);
	}
}

void
isc__nm_tcp_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpsocket);

	if (sock->tid == isc_nm_tid()) {
		tcp_close_direct(sock);
	} else {
		/*
		 * We need to create an event and pass it using async channel
		 */
		isc__netievent_tcpclose_t *ievent =
			isc__nm_get_ievent(sock->mgr, netievent_tcpclose);

		ievent->sock = sock;
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tcpclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpclose_t *ievent = (isc__netievent_tcpclose_t *)ev0;

	REQUIRE(worker->id == ievent->sock->tid);

	tcp_close_direct(ievent->sock);
}

void
isc__nm_tcp_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (sock->type == isc_nm_tcpsocket && sock->tcphandle != NULL &&
	    sock->rcb.recv != NULL)
	{
		sock->rcb.recv(sock->tcphandle, NULL, sock->rcbarg);
	}
}
