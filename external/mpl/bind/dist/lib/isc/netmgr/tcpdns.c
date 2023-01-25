/*	$NetBSD: tcpdns.c,v 1.8 2023/01/25 21:43:31 christos Exp $	*/

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

#include <libgen.h>
#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/errno.h>
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

static atomic_uint_fast32_t last_tcpdnsquota_log = 0;

static bool
can_log_tcpdns_quota(void) {
	isc_stdtime_t now, last;

	isc_stdtime_get(&now);
	last = atomic_exchange_relaxed(&last_tcpdnsquota_log, now);
	if (now != last) {
		return (true);
	}

	return (false);
}

static isc_result_t
tcpdns_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);

static void
tcpdns_close_direct(isc_nmsocket_t *sock);

static void
tcpdns_connect_cb(uv_connect_t *uvreq, int status);

static void
tcpdns_connection_cb(uv_stream_t *server, int status);

static void
tcpdns_close_cb(uv_handle_t *uvhandle);

static isc_result_t
accept_connection(isc_nmsocket_t *ssock, isc_quota_t *quota);

static void
quota_accept_cb(isc_quota_t *quota, void *sock0);

static void
stop_tcpdns_parent(isc_nmsocket_t *sock);
static void
stop_tcpdns_child(isc_nmsocket_t *sock);

static isc_result_t
tcpdns_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc__networker_t *worker = NULL;
	isc_result_t result = ISC_R_UNSET;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->tid == isc_nm_tid());

	worker = &sock->mgr->workers[sock->tid];

	atomic_store(&sock->connecting, true);

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	if (isc__nm_closing(sock)) {
		result = ISC_R_CANCELED;
		goto error;
	}

	r = uv_tcp_open(&sock->uv_handle.tcp, sock->fd);
	if (r != 0) {
		isc__nm_closesocket(sock->fd);
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPENFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPEN]);

	if (req->local.length != 0) {
		r = uv_tcp_bind(&sock->uv_handle.tcp, &req->local.type.sa, 0);
		/*
		 * In case of shared socket UV_EINVAL will be returned and needs
		 * to be ignored
		 */
		if (r != 0 && r != UV_EINVAL) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_BINDFAIL]);
			goto done;
		}
	}

	uv_handle_set_data(&req->uv_req.handle, req);
	r = uv_tcp_connect(&req->uv_req.connect, &sock->uv_handle.tcp,
			   &req->peer.type.sa, tcpdns_connect_cb);
	if (r != 0) {
		isc__nm_incstats(sock->mgr,
				 sock->statsindex[STATID_CONNECTFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CONNECT]);

	uv_handle_set_data((uv_handle_t *)&sock->read_timer,
			   &req->uv_req.connect);
	isc__nmsocket_timer_start(sock);

	atomic_store(&sock->connected, true);

done:
	result = isc__nm_uverr2result(r);
error:
	LOCK(&sock->lock);
	sock->result = result;
	SIGNAL(&sock->cond);
	if (!atomic_load(&sock->active)) {
		WAIT(&sock->scond, &sock->lock);
	}
	INSIST(atomic_load(&sock->active));
	UNLOCK(&sock->lock);

	return (result);
}

void
isc__nm_async_tcpdnsconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsconnect_t *ievent =
		(isc__netievent_tcpdnsconnect_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	isc_result_t result = ISC_R_SUCCESS;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->parent == NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	result = tcpdns_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		isc__nmsocket_clearcb(sock);
		isc__nm_connectcb(sock, req, result, true);
		atomic_store(&sock->active, false);
		isc__nm_tcpdns_close(sock);
	}

	/*
	 * The sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&sock);
}

static void
tcpdns_connect_cb(uv_connect_t *uvreq, int status) {
	isc_result_t result;
	isc__nm_uvreq_t *req = NULL;
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)uvreq->handle);
	struct sockaddr_storage ss;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	isc__nmsocket_timer_stop(sock);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	req = uv_handle_get_data((uv_handle_t *)uvreq);

	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMHANDLE(req->handle));

	if (atomic_load(&sock->timedout)) {
		result = ISC_R_TIMEDOUT;
		goto error;
	}

	if (isc__nmsocket_closing(sock)) {
		/* Socket was closed midflight by isc__nm_tcpdns_shutdown() */
		result = ISC_R_CANCELED;
		goto error;
	} else if (status == UV_ETIMEDOUT) {
		/* Timeout status code here indicates hard error */
		result = ISC_R_TIMEDOUT;
		goto error;
	} else if (status != 0) {
		result = isc__nm_uverr2result(status);
		goto error;
	}

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CONNECT]);
	r = uv_tcp_getpeername(&sock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto error;
	}

	atomic_store(&sock->connecting, false);

	result = isc_sockaddr_fromsockaddr(&sock->peer, (struct sockaddr *)&ss);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc__nm_connectcb(sock, req, ISC_R_SUCCESS, false);

	return;

error:
	isc__nm_failed_connect_cb(sock, req, result, false);
}

void
isc_nm_tcpdnsconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		     isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
		     size_t extrahandlesize) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tcpdnsconnect_t *ievent = NULL;
	isc__nm_uvreq_t *req = NULL;
	sa_family_t sa_family;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(local != NULL);
	REQUIRE(peer != NULL);

	sa_family = peer->type.sa.sa_family;

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tcpdnssocket, local);

	sock->extrahandlesize = extrahandlesize;
	sock->connect_timeout = timeout;
	sock->result = ISC_R_UNSET;
	atomic_init(&sock->client, true);

	req = isc__nm_uvreq_get(mgr, sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->peer = *peer;
	req->local = *local;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface);

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &sock->fd);
	if (result != ISC_R_SUCCESS) {
		if (isc__nm_in_netthread()) {
			sock->tid = isc_nm_tid();
		}
		isc__nmsocket_clearcb(sock);
		isc__nm_connectcb(sock, req, result, true);
		atomic_store(&sock->closed, true);
		isc__nmsocket_detach(&sock);
		return;
	}

	/* 2 minute timeout */
	result = isc__nm_socket_connectiontimeout(sock->fd, 120 * 1000);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	ievent = isc__nm_get_netievent_tcpdnsconnect(mgr, sock, req);

	if (isc__nm_in_netthread()) {
		atomic_store(&sock->active, true);
		sock->tid = isc_nm_tid();
		isc__nm_async_tcpdnsconnect(&mgr->workers[sock->tid],
					    (isc__netievent_t *)ievent);
		isc__nm_put_netievent_tcpdnsconnect(mgr, ievent);
	} else {
		atomic_init(&sock->active, false);
		sock->tid = isc_random_uniform(mgr->nworkers);
		isc__nm_enqueue_ievent(&mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}

	LOCK(&sock->lock);
	while (sock->result == ISC_R_UNSET) {
		WAIT(&sock->cond, &sock->lock);
	}
	atomic_store(&sock->active, true);
	BROADCAST(&sock->scond);
	UNLOCK(&sock->lock);
}

static uv_os_sock_t
isc__nm_tcpdns_lb_socket(isc_nm_t *mgr, sa_family_t sa_family) {
	isc_result_t result;
	uv_os_sock_t sock;

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	(void)isc__nm_socket_incoming_cpu(sock);

	/* FIXME: set mss */

	result = isc__nm_socket_reuse(sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

#ifndef _WIN32
	if (mgr->load_balance_sockets) {
		result = isc__nm_socket_reuse_lb(sock);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
#endif

	return (sock);
}

static void
enqueue_stoplistening(isc_nmsocket_t *sock) {
	isc__netievent_tcpdnsstop_t *ievent =
		isc__nm_get_netievent_tcpdnsstop(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

static void
start_tcpdns_child(isc_nm_t *mgr, isc_sockaddr_t *iface, isc_nmsocket_t *sock,
		   uv_os_sock_t fd, int tid) {
	isc__netievent_tcpdnslisten_t *ievent = NULL;
	isc_nmsocket_t *csock = &sock->children[tid];

	isc__nmsocket_init(csock, mgr, isc_nm_tcpdnssocket, iface);
	csock->parent = sock;
	csock->accept_cb = sock->accept_cb;
	csock->accept_cbarg = sock->accept_cbarg;
	csock->recv_cb = sock->recv_cb;
	csock->recv_cbarg = sock->recv_cbarg;
	csock->extrahandlesize = sock->extrahandlesize;
	csock->backlog = sock->backlog;
	csock->tid = tid;
	/*
	 * We don't attach to quota, just assign - to avoid
	 * increasing quota unnecessarily.
	 */
	csock->pquota = sock->pquota;
	isc_quota_cb_init(&csock->quotacb, quota_accept_cb, csock);

#ifdef _WIN32
	UNUSED(fd);
	csock->fd = isc__nm_tcpdns_lb_socket(mgr, iface->type.sa.sa_family);
#else
	if (mgr->load_balance_sockets) {
		UNUSED(fd);
		csock->fd = isc__nm_tcpdns_lb_socket(mgr,
						     iface->type.sa.sa_family);
	} else {
		csock->fd = dup(fd);
	}
#endif
	REQUIRE(csock->fd >= 0);

	ievent = isc__nm_get_netievent_tcpdnslisten(mgr, csock);
	isc__nm_maybe_enqueue_ievent(&mgr->workers[tid],
				     (isc__netievent_t *)ievent);
}
isc_result_t
isc_nm_listentcpdns(isc_nm_t *mgr, isc_sockaddr_t *iface,
		    isc_nm_recv_cb_t recv_cb, void *recv_cbarg,
		    isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		    size_t extrahandlesize, int backlog, isc_quota_t *quota,
		    isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	size_t children_size = 0;
	uv_os_sock_t fd = -1;

	REQUIRE(VALID_NM(mgr));

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tcpdnslistener, iface);

	atomic_init(&sock->rchildren, 0);
#if defined(WIN32)
	sock->nchildren = 1;
#else
	sock->nchildren = mgr->nworkers;
#endif
	children_size = sock->nchildren * sizeof(sock->children[0]);
	sock->children = isc_mem_get(mgr->mctx, children_size);
	memset(sock->children, 0, children_size);

	sock->result = ISC_R_UNSET;
	sock->accept_cb = accept_cb;
	sock->accept_cbarg = accept_cbarg;
	sock->recv_cb = recv_cb;
	sock->recv_cbarg = recv_cbarg;
	sock->extrahandlesize = extrahandlesize;
	sock->backlog = backlog;
	sock->pquota = quota;

	sock->tid = 0;
	sock->fd = -1;

#ifndef _WIN32
	if (!mgr->load_balance_sockets) {
		fd = isc__nm_tcpdns_lb_socket(mgr, iface->type.sa.sa_family);
	}
#endif

	isc_barrier_init(&sock->startlistening, sock->nchildren);

	for (size_t i = 0; i < sock->nchildren; i++) {
		if ((int)i == isc_nm_tid()) {
			continue;
		}
		start_tcpdns_child(mgr, iface, sock, fd, i);
	}

	if (isc__nm_in_netthread()) {
		start_tcpdns_child(mgr, iface, sock, fd, isc_nm_tid());
	}

#ifndef _WIN32
	if (!mgr->load_balance_sockets) {
		isc__nm_closesocket(fd);
	}
#endif

	LOCK(&sock->lock);
	while (atomic_load(&sock->rchildren) != sock->nchildren) {
		WAIT(&sock->cond, &sock->lock);
	}
	result = sock->result;
	atomic_store(&sock->active, true);
	UNLOCK(&sock->lock);

	INSIST(result != ISC_R_UNSET);

	if (result == ISC_R_SUCCESS) {
		REQUIRE(atomic_load(&sock->rchildren) == sock->nchildren);
		*sockp = sock;
	} else {
		atomic_store(&sock->active, false);
		enqueue_stoplistening(sock);
		isc_nmsocket_close(&sock);
	}

	return (result);
}

void
isc__nm_async_tcpdnslisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnslisten_t *ievent =
		(isc__netievent_tcpdnslisten_t *)ev0;
	sa_family_t sa_family;
	int r;
	int flags = 0;
	isc_nmsocket_t *sock = NULL;
	isc_result_t result = ISC_R_UNSET;
	isc_nm_t *mgr = NULL;

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());
	REQUIRE(VALID_NMSOCK(ievent->sock->parent));

	sock = ievent->sock;
	sa_family = sock->iface.type.sa.sa_family;
	mgr = sock->mgr;

	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->parent != NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	/* TODO: set min mss */

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	/* This keeps the socket alive after everything else is gone */
	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });

	r = uv_timer_init(&worker->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	LOCK(&sock->parent->lock);

	r = uv_tcp_open(&sock->uv_handle.tcp, sock->fd);
	if (r < 0) {
		isc__nm_closesocket(sock->fd);
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPENFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPEN]);

	if (sa_family == AF_INET6) {
		flags = UV_TCP_IPV6ONLY;
	}

#ifdef _WIN32
	r = isc_uv_tcp_freebind(&sock->uv_handle.tcp, &sock->iface.type.sa,
				flags);
	if (r < 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		goto done;
	}
#else
	if (mgr->load_balance_sockets) {
		r = isc_uv_tcp_freebind(&sock->uv_handle.tcp,
					&sock->iface.type.sa, flags);
		if (r < 0) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_BINDFAIL]);
			goto done;
		}
	} else {
		if (sock->parent->fd == -1) {
			r = isc_uv_tcp_freebind(&sock->uv_handle.tcp,
						&sock->iface.type.sa, flags);
			if (r < 0) {
				isc__nm_incstats(sock->mgr, STATID_BINDFAIL);
				goto done;
			}
			sock->parent->uv_handle.tcp.flags =
				sock->uv_handle.tcp.flags;
			sock->parent->fd = sock->fd;
		} else {
			/* The socket is already bound, just copy the flags */
			sock->uv_handle.tcp.flags =
				sock->parent->uv_handle.tcp.flags;
		}
	}
#endif

	/*
	 * The callback will run in the same thread uv_listen() was called
	 * from, so a race with tcpdns_connection_cb() isn't possible.
	 */
	r = uv_listen((uv_stream_t *)&sock->uv_handle.tcp, sock->backlog,
		      tcpdns_connection_cb);
	if (r != 0) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
			      "uv_listen failed: %s",
			      isc_result_totext(isc__nm_uverr2result(r)));
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		goto done;
	}

	atomic_store(&sock->listening, true);

done:
	result = isc__nm_uverr2result(r);
	if (result != ISC_R_SUCCESS) {
		sock->pquota = NULL;
	}

	atomic_fetch_add(&sock->parent->rchildren, 1);
	if (sock->parent->result == ISC_R_UNSET) {
		sock->parent->result = result;
	}
	SIGNAL(&sock->parent->cond);
	UNLOCK(&sock->parent->lock);

	isc_barrier_wait(&sock->parent->startlistening);
}

static void
tcpdns_connection_cb(uv_stream_t *server, int status) {
	isc_nmsocket_t *ssock = uv_handle_get_data((uv_handle_t *)server);
	isc_result_t result;
	isc_quota_t *quota = NULL;

	if (status != 0) {
		result = isc__nm_uverr2result(status);
		goto done;
	}

	REQUIRE(VALID_NMSOCK(ssock));
	REQUIRE(ssock->tid == isc_nm_tid());

	if (isc__nmsocket_closing(ssock)) {
		result = ISC_R_CANCELED;
		goto done;
	}

	if (ssock->pquota != NULL) {
		result = isc_quota_attach_cb(ssock->pquota, &quota,
					     &ssock->quotacb);
		if (result == ISC_R_QUOTA) {
			isc__nm_incstats(ssock->mgr,
					 ssock->statsindex[STATID_ACCEPTFAIL]);
			goto done;
		}
	}

	result = accept_connection(ssock, quota);
done:
	isc__nm_accept_connection_log(result, can_log_tcpdns_quota());
}

void
isc__nm_tcpdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnslistener);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		UNREACHABLE();
	}

	if (!isc__nm_in_netthread()) {
		enqueue_stoplistening(sock);
	} else {
		stop_tcpdns_parent(sock);
	}
}

void
isc__nm_async_tcpdnsstop(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsstop_t *ievent =
		(isc__netievent_tcpdnsstop_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->parent != NULL) {
		stop_tcpdns_child(sock);
		return;
	}

	stop_tcpdns_parent(sock);
}

void
isc__nm_tcpdns_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);

	if (!sock->recv_read) {
		goto destroy;
	}
	sock->recv_read = false;

	if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
		isc__nmsocket_clearcb(sock);
		isc__nm_readcb(sock, req, result);
	}

destroy:
	isc__nmsocket_prep_destroy(sock);

	/*
	 * We need to detach from quota after the read callback function had a
	 * chance to be executed.
	 */
	if (sock->quota != NULL) {
		isc_quota_detach(&sock->quota);
	}
}

void
isc__nm_tcpdns_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;
	isc__netievent_tcpdnsread_t *ievent = NULL;

	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->statichandle == handle);
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(!sock->recv_read);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->recv_read = true;
	if (sock->read_timeout == 0) {
		sock->read_timeout =
			(atomic_load(&sock->keepalive)
				 ? atomic_load(&sock->mgr->keepalive)
				 : atomic_load(&sock->mgr->idle));
	}

	ievent = isc__nm_get_netievent_tcpdnsread(sock->mgr, sock);

	/*
	 * This MUST be done asynchronously, no matter which thread we're
	 * in. The callback function for isc_nm_read() often calls
	 * isc_nm_read() again; if we tried to do that synchronously
	 * we'd clash in processbuffer() and grow the stack indefinitely.
	 */
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);

	return;
}

void
isc__nm_async_tcpdnsread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsread_t *ievent =
		(isc__netievent_tcpdnsread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_result_t result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (isc__nmsocket_closing(sock)) {
		result = ISC_R_CANCELED;
	} else {
		result = isc__nm_process_sock_buffer(sock);
	}

	if (result != ISC_R_SUCCESS) {
		sock->reading = true;
		isc__nm_failed_read_cb(sock, result, false);
	}
}

/*
 * Process a single packet from the incoming buffer.
 *
 * Return ISC_R_SUCCESS and attach 'handlep' to a handle if something
 * was processed; return ISC_R_NOMORE if there isn't a full message
 * to be processed.
 *
 * The caller will need to unreference the handle.
 */
isc_result_t
isc__nm_tcpdns_processbuffer(isc_nmsocket_t *sock) {
	size_t len;
	isc__nm_uvreq_t *req = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (isc__nmsocket_closing(sock)) {
		return (ISC_R_CANCELED);
	}

	/*
	 * If we don't even have the length yet, we can't do
	 * anything.
	 */
	if (sock->buf_len < 2) {
		return (ISC_R_NOMORE);
	}

	/*
	 * Process the first packet from the buffer, leaving
	 * the rest (if any) for later.
	 */
	len = ntohs(*(uint16_t *)sock->buf);
	if (len > sock->buf_len - 2) {
		return (ISC_R_NOMORE);
	}

	req = isc__nm_get_read_req(sock, NULL);
	REQUIRE(VALID_UVREQ(req));

	/*
	 * We need to launch the resume_processing after the buffer has
	 * been consumed, thus we need to delay the detaching the handle.
	 */
	isc_nmhandle_attach(req->handle, &handle);

	/*
	 * The callback will be called synchronously because the
	 * result is ISC_R_SUCCESS, so we don't need to have
	 * the buffer on the heap
	 */
	req->uvbuf.base = (char *)sock->buf + 2;
	req->uvbuf.len = len;

	/*
	 * If isc__nm_tcpdns_read() was called, it will be satisfied by single
	 * DNS message in the next call.
	 */
	sock->recv_read = false;

	/*
	 * The assertion failure here means that there's a errnoneous extra
	 * nmhandle detach happening in the callback and resume_processing gets
	 * called while we are still processing the buffer.
	 */
	REQUIRE(sock->processing == false);
	sock->processing = true;
	isc__nm_readcb(sock, req, ISC_R_SUCCESS);
	sock->processing = false;

	len += 2;
	sock->buf_len -= len;
	if (sock->buf_len > 0) {
		memmove(sock->buf, sock->buf + len, sock->buf_len);
	}

	isc_nmhandle_detach(&handle);

	return (ISC_R_SUCCESS);
}

void
isc__nm_tcpdns_read_cb(uv_stream_t *stream, ssize_t nread,
		       const uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)stream);
	uint8_t *base = NULL;
	size_t len;
	isc_result_t result;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->reading);
	REQUIRE(buf != NULL);

	if (isc__nmsocket_closing(sock)) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED, true);
		goto free;
	}

	if (nread < 0) {
		if (nread != UV_EOF) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_RECVFAIL]);
		}

		isc__nm_failed_read_cb(sock, isc__nm_uverr2result(nread), true);
		goto free;
	}

	base = (uint8_t *)buf->base;
	len = nread;

	/*
	 * FIXME: We can avoid the memmove here if we know we have received full
	 * packet; e.g. we should be smarter, a.s. there are just few situations
	 *
	 * The tcp_alloc_buf should be smarter and point the uv_read_start to
	 * the position where previous read has ended in the sock->buf, that way
	 * the data could be read directly into sock->buf.
	 */

	if (sock->buf_len + len > sock->buf_size) {
		isc__nm_alloc_dnsbuf(sock, sock->buf_len + len);
	}
	memmove(sock->buf + sock->buf_len, base, len);
	sock->buf_len += len;

	if (!atomic_load(&sock->client)) {
		sock->read_timeout = atomic_load(&sock->mgr->idle);
	}

	result = isc__nm_process_sock_buffer(sock);
	if (result != ISC_R_SUCCESS) {
		isc__nm_failed_read_cb(sock, result, true);
	}
free:
	if (nread < 0) {
		/*
		 * The buffer may be a null buffer on error.
		 */
		if (buf->base == NULL && buf->len == 0) {
			return;
		}
	}

	isc__nm_free_uvbuf(sock, buf);
}

static void
quota_accept_cb(isc_quota_t *quota, void *sock0) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)sock0;

	REQUIRE(VALID_NMSOCK(sock));

	/*
	 * Create a tcpdnsaccept event and pass it using the async channel.
	 */

	isc__netievent_tcpdnsaccept_t *ievent =
		isc__nm_get_netievent_tcpdnsaccept(sock->mgr, sock, quota);
	isc__nm_maybe_enqueue_ievent(&sock->mgr->workers[sock->tid],
				     (isc__netievent_t *)ievent);
}

/*
 * This is called after we get a quota_accept_cb() callback.
 */
void
isc__nm_async_tcpdnsaccept(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsaccept_t *ievent =
		(isc__netievent_tcpdnsaccept_t *)ev0;
	isc_result_t result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());

	result = accept_connection(ievent->sock, ievent->quota);
	isc__nm_accept_connection_log(result, can_log_tcpdns_quota());
}

static isc_result_t
accept_connection(isc_nmsocket_t *ssock, isc_quota_t *quota) {
	isc_nmsocket_t *csock = NULL;
	isc__networker_t *worker = NULL;
	int r;
	isc_result_t result;
	struct sockaddr_storage peer_ss;
	struct sockaddr_storage local_ss;
	isc_sockaddr_t local;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(VALID_NMSOCK(ssock));
	REQUIRE(ssock->tid == isc_nm_tid());

	if (isc__nmsocket_closing(ssock)) {
		if (quota != NULL) {
			isc_quota_detach(&quota);
		}
		return (ISC_R_CANCELED);
	}

	REQUIRE(ssock->accept_cb != NULL);

	csock = isc_mem_get(ssock->mgr->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(csock, ssock->mgr, isc_nm_tcpdnssocket,
			   &ssock->iface);
	csock->tid = ssock->tid;
	csock->extrahandlesize = ssock->extrahandlesize;
	isc__nmsocket_attach(ssock, &csock->server);
	csock->recv_cb = ssock->recv_cb;
	csock->recv_cbarg = ssock->recv_cbarg;
	csock->quota = quota;
	csock->accepting = true;

	worker = &csock->mgr->workers[csock->tid];

	r = uv_tcp_init(&worker->loop, &csock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&csock->uv_handle.handle, csock);

	r = uv_timer_init(&worker->loop, &csock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&csock->read_timer, csock);

	r = uv_accept(&ssock->uv_handle.stream, &csock->uv_handle.stream);
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto failure;
	}

	r = uv_tcp_getpeername(&csock->uv_handle.tcp,
			       (struct sockaddr *)&peer_ss,
			       &(int){ sizeof(peer_ss) });
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto failure;
	}

	result = isc_sockaddr_fromsockaddr(&csock->peer,
					   (struct sockaddr *)&peer_ss);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	r = uv_tcp_getsockname(&csock->uv_handle.tcp,
			       (struct sockaddr *)&local_ss,
			       &(int){ sizeof(local_ss) });
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto failure;
	}

	result = isc_sockaddr_fromsockaddr(&local,
					   (struct sockaddr *)&local_ss);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	/*
	 * The handle will be either detached on acceptcb failure or in the
	 * readcb.
	 */
	handle = isc__nmhandle_get(csock, NULL, &local);

	result = ssock->accept_cb(handle, ISC_R_SUCCESS, ssock->accept_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc_nmhandle_detach(&handle);
		goto failure;
	}

	csock->accepting = false;

	isc__nm_incstats(csock->mgr, csock->statsindex[STATID_ACCEPT]);

	csock->read_timeout = atomic_load(&csock->mgr->init);

	csock->closehandle_cb = isc__nm_resume_processing;

	/*
	 * We need to keep the handle alive until we fail to read or connection
	 * is closed by the other side, it will be detached via
	 * prep_destroy()->tcpdns_close_direct().
	 */
	isc_nmhandle_attach(handle, &csock->recv_handle);
	result = isc__nm_process_sock_buffer(csock);
	if (result != ISC_R_SUCCESS) {
		isc_nmhandle_detach(&csock->recv_handle);
		isc_nmhandle_detach(&handle);
		goto failure;
	}

	/*
	 * The initial timer has been set, update the read timeout for the next
	 * reads.
	 */
	csock->read_timeout = (atomic_load(&csock->keepalive)
				       ? atomic_load(&csock->mgr->keepalive)
				       : atomic_load(&csock->mgr->idle));

	isc_nmhandle_detach(&handle);

	/*
	 * sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&csock);

	return (ISC_R_SUCCESS);

failure:

	atomic_store(&csock->active, false);

	isc__nm_failed_accept_cb(csock, result);

	isc__nmsocket_prep_destroy(csock);

	isc__nmsocket_detach(&csock);

	return (result);
}

void
isc__nm_tcpdns_send(isc_nmhandle_t *handle, isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;
	isc__netievent_tcpdnssend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(sock->type == isc_nm_tcpdnssocket);

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	*(uint16_t *)uvreq->tcplen = htons(region->length);
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	isc_nmhandle_attach(handle, &uvreq->handle);

	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	ievent = isc__nm_get_netievent_tcpdnssend(sock->mgr, sock, uvreq);
	isc__nm_maybe_enqueue_ievent(&sock->mgr->workers[sock->tid],
				     (isc__netievent_t *)ievent);

	return;
}

static void
tcpdns_send_cb(uv_write_t *req, int status) {
	isc__nm_uvreq_t *uvreq = (isc__nm_uvreq_t *)req->data;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMSOCK(uvreq->sock));

	sock = uvreq->sock;

	isc_nm_timer_stop(uvreq->timer);
	isc_nm_timer_detach(&uvreq->timer);

	if (status < 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
		isc__nm_failed_send_cb(sock, uvreq,
				       isc__nm_uverr2result(status));
		return;
	}

	isc__nm_sendcb(sock, uvreq, ISC_R_SUCCESS, false);
}

/*
 * Handle 'tcpsend' async event - send a packet on the socket
 */
void
isc__nm_async_tcpdnssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnssend_t *ievent =
		(isc__netievent_tcpdnssend_t *)ev0;

	REQUIRE(VALID_UVREQ(ievent->req));
	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->type == isc_nm_tcpdnssocket);
	REQUIRE(ievent->sock->tid == isc_nm_tid());

	isc_result_t result;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;

	if (sock->write_timeout == 0) {
		sock->write_timeout =
			(atomic_load(&sock->keepalive)
				 ? atomic_load(&sock->mgr->keepalive)
				 : atomic_load(&sock->mgr->idle));
	}

	uv_buf_t bufs[2] = { { .base = uvreq->tcplen, .len = 2 },
			     { .base = uvreq->uvbuf.base,
			       .len = uvreq->uvbuf.len } };
	int nbufs = 2;
	int r;

	UNUSED(worker);

	if (isc__nmsocket_closing(sock)) {
		result = ISC_R_CANCELED;
		goto fail;
	}

	r = uv_try_write(&sock->uv_handle.stream, bufs, nbufs);

	if (r == (int)(bufs[0].len + bufs[1].len)) {
		/* Wrote everything */
		isc__nm_sendcb(sock, uvreq, ISC_R_SUCCESS, true);
		return;
	}

	if (r == 1) {
		/* Partial write of DNSMSG length */
		bufs[0].base = uvreq->tcplen + 1;
		bufs[0].len = 1;
	} else if (r > 0) {
		/* Partial write of DNSMSG */
		nbufs = 1;
		bufs[0].base = uvreq->uvbuf.base + (r - 2);
		bufs[0].len = uvreq->uvbuf.len - (r - 2);
	} else if (r == UV_ENOSYS || r == UV_EAGAIN) {
		/* uv_try_write not supported, send asynchronously */
	} else {
		/* error sending data */
		result = isc__nm_uverr2result(r);
		goto fail;
	}

	r = uv_write(&uvreq->uv_req.write, &sock->uv_handle.stream, bufs, nbufs,
		     tcpdns_send_cb);
	if (r < 0) {
		result = isc__nm_uverr2result(r);
		goto fail;
	}

	isc_nm_timer_create(uvreq->handle, isc__nmsocket_writetimeout_cb, uvreq,
			    &uvreq->timer);
	if (sock->write_timeout > 0) {
		isc_nm_timer_start(uvreq->timer, sock->write_timeout);
	}

	return;
fail:
	if (result != ISC_R_SUCCESS) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
		isc__nm_failed_send_cb(sock, uvreq, result);
	}
}

static void
tcpdns_stop_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

	uv_handle_set_data(handle, NULL);

	if (!atomic_compare_exchange_strong(&sock->closed, &(bool){ false },
					    true))
	{
		UNREACHABLE();
	}

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CLOSE]);

	atomic_store(&sock->listening, false);

	isc__nmsocket_detach(&sock);
}

static void
tcpdns_close_sock(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

	if (!atomic_compare_exchange_strong(&sock->closed, &(bool){ false },
					    true))
	{
		UNREACHABLE();
	}

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CLOSE]);

	if (sock->server != NULL) {
		isc__nmsocket_detach(&sock->server);
	}

	atomic_store(&sock->connected, false);

	isc__nmsocket_prep_destroy(sock);
}

static void
tcpdns_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	uv_handle_set_data(handle, NULL);

	tcpdns_close_sock(sock);
}

static void
read_timer_close_cb(uv_handle_t *timer) {
	isc_nmsocket_t *sock = uv_handle_get_data(timer);
	uv_handle_set_data(timer, NULL);

	REQUIRE(VALID_NMSOCK(sock));

	if (sock->parent) {
		uv_close(&sock->uv_handle.handle, tcpdns_stop_cb);
	} else if (uv_is_closing(&sock->uv_handle.handle)) {
		tcpdns_close_sock(sock);
	} else {
		uv_close(&sock->uv_handle.handle, tcpdns_close_cb);
	}
}

static void
stop_tcpdns_child(isc_nmsocket_t *sock) {
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->tid == isc_nm_tid());

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	tcpdns_close_direct(sock);

	atomic_fetch_sub(&sock->parent->rchildren, 1);

	isc_barrier_wait(&sock->parent->stoplistening);
}

static void
stop_tcpdns_parent(isc_nmsocket_t *sock) {
	isc_nmsocket_t *csock = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tcpdnslistener);

	isc_barrier_init(&sock->stoplistening, sock->nchildren);

	for (size_t i = 0; i < sock->nchildren; i++) {
		csock = &sock->children[i];
		REQUIRE(VALID_NMSOCK(csock));

		if ((int)i == isc_nm_tid()) {
			/*
			 * We need to schedule closing the other sockets first
			 */
			continue;
		}

		atomic_store(&csock->active, false);
		enqueue_stoplistening(csock);
	}

	csock = &sock->children[isc_nm_tid()];
	atomic_store(&csock->active, false);
	stop_tcpdns_child(csock);

	atomic_store(&sock->closed, true);
	isc__nmsocket_prep_destroy(sock);
}

static void
tcpdns_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

	if (sock->quota != NULL) {
		isc_quota_detach(&sock->quota);
	}

	if (sock->recv_handle != NULL) {
		isc_nmhandle_detach(&sock->recv_handle);
	}

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);

	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);
	uv_close((uv_handle_t *)&sock->read_timer, read_timer_close_cb);
}

void
isc__nm_tcpdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(!isc__nmsocket_active(sock));

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		tcpdns_close_direct(sock);
	} else {
		/*
		 * We need to create an event and pass it using async channel
		 */
		isc__netievent_tcpdnsclose_t *ievent =
			isc__nm_get_netievent_tcpdnsclose(sock->mgr, sock);

		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tcpdnsclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsclose_t *ievent =
		(isc__netievent_tcpdnsclose_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	tcpdns_close_direct(sock);
}

static void
tcpdns_close_connect_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	REQUIRE(VALID_NMSOCK(sock));

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->tid == isc_nm_tid());

	isc__nmsocket_prep_destroy(sock);
	isc__nmsocket_detach(&sock);
}

void
isc__nm_tcpdns_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tcpdnssocket);

	/*
	 * If the socket is active, mark it inactive and
	 * continue. If it isn't active, stop now.
	 */
	if (!isc__nmsocket_deactivate(sock)) {
		return;
	}

	if (sock->accepting) {
		return;
	}

	if (atomic_load(&sock->connecting)) {
		isc_nmsocket_t *tsock = NULL;
		isc__nmsocket_attach(sock, &tsock);
		uv_close(&sock->uv_handle.handle, tcpdns_close_connect_cb);
		return;
	}

	if (sock->statichandle != NULL) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED, false);
		return;
	}

	/*
	 * Otherwise, we just send the socket to abyss...
	 */
	if (sock->parent == NULL) {
		isc__nmsocket_prep_destroy(sock);
	}
}

void
isc__nm_tcpdns_cancelread(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tcpdnscancel_t *ievent = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);

	ievent = isc__nm_get_netievent_tcpdnscancel(sock->mgr, sock, handle);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tcpdnscancel(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnscancel_t *ievent =
		(isc__netievent_tcpdnscancel_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	isc__nm_failed_read_cb(sock, ISC_R_EOF, false);
}

void
isc_nm_tcpdns_sequential(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tcpdnssocket);

	sock = handle->sock;

	/*
	 * We don't want pipelining on this connection. That means
	 * that we need to pause after reading each request, and
	 * resume only after the request has been processed. This
	 * is done in resume_processing(), which is the socket's
	 * closehandle_cb callback, called whenever a handle
	 * is released.
	 */

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);
	atomic_store(&sock->sequential, true);
}
