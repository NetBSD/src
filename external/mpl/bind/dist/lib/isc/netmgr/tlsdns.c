/*	$NetBSD: tlsdns.c,v 1.5.2.2 2024/02/25 15:47:24 martin Exp $	*/

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
#include "openssl_shim.h"
#include "uv-compat.h"

static atomic_uint_fast32_t last_tlsdnsquota_log = 0;

static void
tls_error(isc_nmsocket_t *sock, isc_result_t result);

static isc_result_t
tlsdns_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);

static void
tlsdns_close_direct(isc_nmsocket_t *sock);

static isc_result_t
tlsdns_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);
static void
tlsdns_connect_cb(uv_connect_t *uvreq, int status);

static void
tlsdns_connection_cb(uv_stream_t *server, int status);

static void
tlsdns_close_cb(uv_handle_t *uvhandle);

static isc_result_t
accept_connection(isc_nmsocket_t *ssock, isc_quota_t *quota);

static void
quota_accept_cb(isc_quota_t *quota, void *sock0);

static void
stop_tlsdns_parent(isc_nmsocket_t *sock);
static void
stop_tlsdns_child(isc_nmsocket_t *sock);

static void
async_tlsdns_cycle(isc_nmsocket_t *sock) __attribute__((unused));

static isc_result_t
tls_cycle(isc_nmsocket_t *sock);

static void
call_pending_send_callbacks(isc_nmsocket_t *sock, const isc_result_t result);

static void
tlsdns_keep_client_tls_session(isc_nmsocket_t *sock);

static void
tlsdns_set_tls_shutdown(isc_tls_t *tls) {
	(void)SSL_set_shutdown(tls, SSL_SENT_SHUTDOWN);
}

static bool
peer_verification_has_failed(isc_nmsocket_t *sock) {
	if (sock->tls.tls != NULL && sock->tls.state == TLS_STATE_HANDSHAKE &&
	    SSL_get_verify_result(sock->tls.tls) != X509_V_OK)
	{
		return (true);
	}

	return (false);
}

static bool
can_log_tlsdns_quota(void) {
	isc_stdtime_t now, last;

	isc_stdtime_get(&now);
	last = atomic_exchange_relaxed(&last_tlsdnsquota_log, now);
	if (now != last) {
		return (true);
	}

	return (false);
}

static isc_result_t
tlsdns_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc__networker_t *worker = NULL;
	isc_result_t result = ISC_R_UNSET;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->tid == isc_nm_tid());

	worker = &sock->mgr->workers[sock->tid];

	atomic_store(&sock->connecting, true);

	/* 2 minute timeout */
	result = isc__nm_socket_connectiontimeout(sock->fd, 120 * 1000);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	if (isc__nm_closing(sock)) {
		result = ISC_R_SHUTTINGDOWN;
		goto error;
	}

	r = uv_tcp_open(&sock->uv_handle.tcp, sock->fd);
	if (r != 0) {
		isc__nm_closesocket(sock->fd);
		isc__nm_incstats(sock, STATID_OPENFAIL);
		goto done;
	}
	isc__nm_incstats(sock, STATID_OPEN);

	if (req->local.length != 0) {
		r = uv_tcp_bind(&sock->uv_handle.tcp, &req->local.type.sa, 0);
		/*
		 * In case of shared socket UV_EINVAL will be returned and needs
		 * to be ignored
		 */
		if (r != 0 && r != UV_EINVAL) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			goto done;
		}
	}

	isc__nm_set_network_buffers(sock->mgr, &sock->uv_handle.handle);

	uv_handle_set_data(&req->uv_req.handle, req);
	r = uv_tcp_connect(&req->uv_req.connect, &sock->uv_handle.tcp,
			   &req->peer.type.sa, tlsdns_connect_cb);
	if (r != 0) {
		isc__nm_incstats(sock, STATID_CONNECTFAIL);
		goto done;
	}

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
isc__nm_async_tlsdnsconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsconnect_t *ievent =
		(isc__netievent_tlsdnsconnect_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	isc_result_t result = ISC_R_SUCCESS;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->parent == NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	result = tlsdns_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		atomic_compare_exchange_enforced(&sock->connecting,
						 &(bool){ true }, false);
		isc__nmsocket_clearcb(sock);
		isc__nm_connectcb(sock, req, result, true);
		atomic_store(&sock->active, false);
		isc__nm_tlsdns_close(sock);
	}

	/*
	 * The sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&sock);
}

static void
tlsdns_connect_cb(uv_connect_t *uvreq, int status) {
	isc_result_t result = ISC_R_UNSET;
	isc__nm_uvreq_t *req = NULL;
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)uvreq->handle);
	struct sockaddr_storage ss;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	req = uv_handle_get_data((uv_handle_t *)uvreq);

	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMHANDLE(req->handle));

	if (atomic_load(&sock->timedout)) {
		result = ISC_R_TIMEDOUT;
		goto error;
	} else if (isc__nm_closing(sock)) {
		/* Network manager shutting down */
		result = ISC_R_SHUTTINGDOWN;
		goto error;
	} else if (isc__nmsocket_closing(sock)) {
		/* Connection canceled */
		result = ISC_R_CANCELED;
		goto error;
	} else if (status == UV_ETIMEDOUT) {
		/* Timeout status code here indicates hard error */
		result = ISC_R_TIMEDOUT;
		goto error;
	} else if (status == UV_EADDRINUSE) {
		/*
		 * On FreeBSD the TCP connect() call sometimes results in a
		 * spurious transient EADDRINUSE. Try a few more times before
		 * giving up.
		 */
		if (--req->connect_tries > 0) {
			r = uv_tcp_connect(
				&req->uv_req.connect, &sock->uv_handle.tcp,
				&req->peer.type.sa, tlsdns_connect_cb);
			if (r != 0) {
				result = isc__nm_uverr2result(r);
				goto error;
			}
			return;
		}
		result = isc__nm_uverr2result(status);
		goto error;
	} else if (status != 0) {
		result = isc__nm_uverr2result(status);
		goto error;
	}

	isc__nm_incstats(sock, STATID_CONNECT);
	r = uv_tcp_getpeername(&sock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc__nm_uverr2result(r);
		goto error;
	}

	sock->tls.state = TLS_STATE_NONE;
	sock->tls.tls = isc_tls_create(sock->tls.ctx);
	RUNTIME_CHECK(sock->tls.tls != NULL);

	/*
	 *
	 */
	r = BIO_new_bio_pair(&sock->tls.ssl_wbio, ISC_NETMGR_TCP_RECVBUF_SIZE,
			     &sock->tls.app_rbio, ISC_NETMGR_TCP_RECVBUF_SIZE);
	RUNTIME_CHECK(r == 1);

	r = BIO_new_bio_pair(&sock->tls.ssl_rbio, ISC_NETMGR_TCP_RECVBUF_SIZE,
			     &sock->tls.app_wbio, ISC_NETMGR_TCP_RECVBUF_SIZE);
	RUNTIME_CHECK(r == 1);

#if HAVE_SSL_SET0_RBIO && HAVE_SSL_SET0_WBIO
	/*
	 * Note that if the rbio and wbio are the same then
	 * SSL_set0_rbio() and SSL_set0_wbio() each take ownership of
	 * one reference. Therefore it may be necessary to increment the
	 * number of references available using BIO_up_ref(3) before
	 * calling the set0 functions.
	 */
	SSL_set0_rbio(sock->tls.tls, sock->tls.ssl_rbio);
	SSL_set0_wbio(sock->tls.tls, sock->tls.ssl_wbio);
#else
	SSL_set_bio(sock->tls.tls, sock->tls.ssl_rbio, sock->tls.ssl_wbio);
#endif

	result = isc_sockaddr_fromsockaddr(&sock->peer, (struct sockaddr *)&ss);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (sock->tls.client_sess_cache != NULL) {
		isc_tlsctx_client_session_cache_reuse_sockaddr(
			sock->tls.client_sess_cache, &sock->peer,
			sock->tls.tls);
	}

	SSL_set_connect_state(sock->tls.tls);

	/* Setting pending req */
	sock->tls.pending_req = req;

	result = isc__nm_process_sock_buffer(sock);
	if (result != ISC_R_SUCCESS) {
		sock->tls.pending_req = NULL;
		goto error;
	}

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		sock->tls.pending_req = NULL;
		goto error;
	}

	return;

error:
	isc__nm_failed_connect_cb(sock, req, result, false);
}

void
isc_nm_tlsdnsconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		     isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
		     size_t extrahandlesize, isc_tlsctx_t *sslctx,
		     isc_tlsctx_client_session_cache_t *client_sess_cache) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tlsdnsconnect_t *ievent = NULL;
	isc__nm_uvreq_t *req = NULL;
	sa_family_t sa_family;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(local != NULL);
	REQUIRE(peer != NULL);
	REQUIRE(sslctx != NULL);

	sa_family = peer->type.sa.sa_family;

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tlsdnssocket, local);

	sock->extrahandlesize = extrahandlesize;
	sock->connect_timeout = timeout;
	sock->result = ISC_R_UNSET;
	isc_tlsctx_attach(sslctx, &sock->tls.ctx);
	atomic_init(&sock->client, true);
	atomic_init(&sock->connecting, true);

	req = isc__nm_uvreq_get(mgr, sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->peer = *peer;
	req->local = *local;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface);

	if (client_sess_cache != NULL) {
		INSIST(isc_tlsctx_client_session_cache_getctx(
			       client_sess_cache) == sslctx);
		isc_tlsctx_client_session_cache_attach(
			client_sess_cache, &sock->tls.client_sess_cache);
	}

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &sock->fd);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	if (isc__nm_closing(sock)) {
		result = ISC_R_SHUTTINGDOWN;
		goto failure;
	}

	(void)isc__nm_socket_min_mtu(sock->fd, sa_family);
	(void)isc__nm_socket_tcp_maxseg(sock->fd, NM_MAXSEG);

	/* 2 minute timeout */
	result = isc__nm_socket_connectiontimeout(sock->fd, 120 * 1000);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	ievent = isc__nm_get_netievent_tlsdnsconnect(mgr, sock, req);

	if (isc__nm_in_netthread()) {
		atomic_store(&sock->active, true);
		sock->tid = isc_nm_tid();
		isc__nm_async_tlsdnsconnect(&mgr->workers[sock->tid],
					    (isc__netievent_t *)ievent);
		isc__nm_put_netievent_tlsdnsconnect(mgr, ievent);
	} else {
		atomic_init(&sock->active, false);
		sock->tid = isc_random_uniform(mgr->nlisteners);
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
	return;

failure:
	if (isc__nm_in_netthread()) {
		sock->tid = isc_nm_tid();
	}

	atomic_compare_exchange_enforced(&sock->connecting, &(bool){ true },
					 false);
	isc__nmsocket_clearcb(sock);
	isc__nm_connectcb(sock, req, result, true);
	atomic_store(&sock->closed, true);
	isc__nmsocket_detach(&sock);
}

static uv_os_sock_t
isc__nm_tlsdns_lb_socket(isc_nm_t *mgr, sa_family_t sa_family) {
	isc_result_t result;
	uv_os_sock_t sock;

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	(void)isc__nm_socket_incoming_cpu(sock);
	(void)isc__nm_socket_v6only(sock, sa_family);

	/* FIXME: set mss */

	result = isc__nm_socket_reuse(sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (mgr->load_balance_sockets) {
		result = isc__nm_socket_reuse_lb(sock);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	return (sock);
}

static void
start_tlsdns_child(isc_nm_t *mgr, isc_sockaddr_t *iface, isc_nmsocket_t *sock,
		   uv_os_sock_t fd, int tid) {
	isc__netievent_tlsdnslisten_t *ievent = NULL;
	isc_nmsocket_t *csock = &sock->children[tid];

	isc__nmsocket_init(csock, mgr, isc_nm_tlsdnssocket, iface);
	csock->parent = sock;
	csock->accept_cb = sock->accept_cb;
	csock->accept_cbarg = sock->accept_cbarg;
	csock->recv_cb = sock->recv_cb;
	csock->recv_cbarg = sock->recv_cbarg;
	csock->extrahandlesize = sock->extrahandlesize;
	csock->backlog = sock->backlog;
	csock->tid = tid;
	isc_tlsctx_attach(sock->tls.ctx, &csock->tls.ctx);

	/*
	 * We don't attach to quota, just assign - to avoid
	 * increasing quota unnecessarily.
	 */
	csock->pquota = sock->pquota;
	isc_quota_cb_init(&csock->quotacb, quota_accept_cb, csock);

	if (mgr->load_balance_sockets) {
		UNUSED(fd);
		csock->fd = isc__nm_tlsdns_lb_socket(mgr,
						     iface->type.sa.sa_family);
	} else {
		csock->fd = dup(fd);
	}
	REQUIRE(csock->fd >= 0);

	ievent = isc__nm_get_netievent_tlsdnslisten(mgr, csock);
	isc__nm_maybe_enqueue_ievent(&mgr->workers[tid],
				     (isc__netievent_t *)ievent);
}

static void
enqueue_stoplistening(isc_nmsocket_t *sock) {
	isc__netievent_tlsdnsstop_t *ievent =
		isc__nm_get_netievent_tlsdnsstop(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

isc_result_t
isc_nm_listentlsdns(isc_nm_t *mgr, isc_sockaddr_t *iface,
		    isc_nm_recv_cb_t recv_cb, void *recv_cbarg,
		    isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		    size_t extrahandlesize, int backlog, isc_quota_t *quota,
		    isc_tlsctx_t *sslctx, isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	size_t children_size = 0;
	uv_os_sock_t fd = -1;

	REQUIRE(VALID_NM(mgr));

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tlsdnslistener, iface);

	atomic_init(&sock->rchildren, 0);
	sock->nchildren = mgr->nlisteners;
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

	isc_tlsctx_attach(sslctx, &sock->tls.ctx);

	sock->tid = 0;
	sock->fd = -1;

	if (!mgr->load_balance_sockets) {
		fd = isc__nm_tlsdns_lb_socket(mgr, iface->type.sa.sa_family);
	}

	isc_barrier_init(&sock->startlistening, sock->nchildren);

	for (size_t i = 0; i < sock->nchildren; i++) {
		if ((int)i == isc_nm_tid()) {
			continue;
		}
		start_tlsdns_child(mgr, iface, sock, fd, i);
	}

	if (isc__nm_in_netthread()) {
		start_tlsdns_child(mgr, iface, sock, fd, isc_nm_tid());
	}

	if (!mgr->load_balance_sockets) {
		isc__nm_closesocket(fd);
	}

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
isc__nm_async_tlsdnslisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnslisten_t *ievent =
		(isc__netievent_tlsdnslisten_t *)ev0;
	sa_family_t sa_family;
	int r;
	int flags = 0;
	isc_nmsocket_t *sock = NULL;
	isc_result_t result = ISC_R_UNSET;
	isc_nm_t *mgr;

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());
	REQUIRE(VALID_NMSOCK(ievent->sock->parent));

	sock = ievent->sock;
	sa_family = sock->iface.type.sa.sa_family;
	mgr = sock->mgr;

	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->parent != NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	(void)isc__nm_socket_min_mtu(sock->fd, sa_family);
	(void)isc__nm_socket_tcp_maxseg(sock->fd, NM_MAXSEG);

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
		isc__nm_incstats(sock, STATID_OPENFAIL);
		goto done;
	}
	isc__nm_incstats(sock, STATID_OPEN);

	if (sa_family == AF_INET6) {
		flags = UV_TCP_IPV6ONLY;
	}

	if (mgr->load_balance_sockets) {
		r = isc_uv_tcp_freebind(&sock->uv_handle.tcp,
					&sock->iface.type.sa, flags);
		if (r < 0) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			goto done;
		}
	} else {
		if (sock->parent->fd == -1) {
			r = isc_uv_tcp_freebind(&sock->uv_handle.tcp,
						&sock->iface.type.sa, flags);
			if (r < 0) {
				isc__nm_incstats(sock, STATID_BINDFAIL);
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

	isc__nm_set_network_buffers(sock->mgr, &sock->uv_handle.handle);

	/*
	 * The callback will run in the same thread uv_listen() was
	 * called from, so a race with tlsdns_connection_cb() isn't
	 * possible.
	 */
	r = uv_listen((uv_stream_t *)&sock->uv_handle.tcp, sock->backlog,
		      tlsdns_connection_cb);
	if (r != 0) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
			      "uv_listen failed: %s",
			      isc_result_totext(isc__nm_uverr2result(r)));
		isc__nm_incstats(sock, STATID_BINDFAIL);
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
tlsdns_connection_cb(uv_stream_t *server, int status) {
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
			isc__nm_incstats(ssock, STATID_ACCEPTFAIL);
			goto done;
		}
	}

	result = accept_connection(ssock, quota);
done:
	isc__nm_accept_connection_log(result, can_log_tlsdns_quota());
}

void
isc__nm_tlsdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnslistener);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		UNREACHABLE();
	}

	if (!isc__nm_in_netthread()) {
		enqueue_stoplistening(sock);
	} else {
		stop_tlsdns_parent(sock);
	}
}

static void
tls_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	isc__netievent_tlsdnsshutdown_t *ievent =
		isc__nm_get_netievent_tlsdnsshutdown(sock->mgr, sock);
	isc__nm_maybe_enqueue_ievent(&sock->mgr->workers[sock->tid],
				     (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsdnsshutdown(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsshutdown_t *ievent =
		(isc__netievent_tlsdnsshutdown_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	int rv;
	int err;
	isc_result_t result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(ievent->sock));

	if (sock->tls.state != TLS_STATE_IO) {
		/* Nothing to do */
		return;
	}

	rv = SSL_shutdown(sock->tls.tls);

	if (rv == 1) {
		sock->tls.state = TLS_STATE_NONE;
		/* FIXME: continue closing the socket */
		return;
	}

	if (rv == 0) {
		result = tls_cycle(sock);
		if (result != ISC_R_SUCCESS) {
			tls_error(sock, result);
			return;
		}

		/* Reschedule closing the socket */
		tls_shutdown(sock);
		return;
	}

	err = SSL_get_error(sock->tls.tls, rv);

	switch (err) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_X509_LOOKUP:
		result = tls_cycle(sock);
		if (result != ISC_R_SUCCESS) {
			tls_error(sock, result);
			return;
		}

		/* Reschedule closing the socket */
		tls_shutdown(sock);
		return;
	case 0:
		UNREACHABLE();
	case SSL_ERROR_ZERO_RETURN:
		tls_error(sock, ISC_R_EOF);
		break;
	default:
		tls_error(sock, ISC_R_TLSERROR);
	}
	return;
}

void
isc__nm_async_tlsdnsstop(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsstop_t *ievent =
		(isc__netievent_tlsdnsstop_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->parent != NULL) {
		stop_tlsdns_child(sock);
		return;
	}

	stop_tlsdns_parent(sock);
}

void
isc__nm_tlsdns_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			      bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);

	if (sock->tls.pending_req != NULL) {
		isc_result_t failure_result = ISC_R_CANCELED;
		isc__nm_uvreq_t *req = sock->tls.pending_req;
		sock->tls.pending_req = NULL;

		if (peer_verification_has_failed(sock)) {
			/*
			 * Save error message as 'sock->tls' will get detached.
			 */
			sock->tls.tls_verify_errmsg =
				isc_tls_verify_peer_result_string(
					sock->tls.tls);
			failure_result = ISC_R_TLSBADPEERCERT;
		}
		isc__nm_failed_connect_cb(sock, req, failure_result, async);
	}

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
	call_pending_send_callbacks(sock, result);
	isc__nmsocket_prep_destroy(sock);

	/*
	 * We need to detach from quota after the read callback function
	 * had a chance to be executed.
	 */
	if (sock->quota != NULL) {
		isc_quota_detach(&sock->quota);
	}
}

void
isc__nm_tlsdns_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;
	isc__netievent_tlsdnsread_t *ievent = NULL;

	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->statichandle == handle);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->recv_read = true;
	if (sock->read_timeout == 0) {
		sock->read_timeout =
			(atomic_load(&sock->keepalive)
				 ? atomic_load(&sock->mgr->keepalive)
				 : atomic_load(&sock->mgr->idle));
	}

	ievent = isc__nm_get_netievent_tlsdnsread(sock->mgr, sock);

	/*
	 * This MUST be done asynchronously, no matter which thread
	 * we're in. The callback function for isc_nm_read() often calls
	 * isc_nm_read() again; if we tried to do that synchronously
	 * we'd clash in processbuffer() and grow the stack
	 * indefinitely.
	 */
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);

	return;
}

void
isc__nm_async_tlsdnsread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsread_t *ievent =
		(isc__netievent_tlsdnsread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_result_t result = ISC_R_SUCCESS;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (isc__nmsocket_closing(sock)) {
		atomic_store(&sock->reading, true);
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED, false);
		return;
	}

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
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
isc__nm_tlsdns_processbuffer(isc_nmsocket_t *sock) {
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

	if (sock->recv_cb == NULL) {
		/*
		 * recv_cb has been cleared - there is
		 * nothing to do
		 */
		return (ISC_R_CANCELED);
	} else if (sock->statichandle == NULL &&
		   sock->tls.state == TLS_STATE_IO &&
		   atomic_load(&sock->connected) &&
		   !atomic_load(&sock->connecting))
	{
		/*
		 * It seems that some unexpected data (a DNS message) has
		 * arrived while we are wrapping up.
		 */
		return (ISC_R_CANCELED);
	}

	if (sock->client && !sock->recv_read) {
		/*
		 * We are not reading data - stop here.
		 */
		return (ISC_R_CANCELED);
	}

	req = isc__nm_get_read_req(sock, NULL);
	REQUIRE(VALID_UVREQ(req));

	/*
	 * We need to launch isc__nm_resume_processing() after the buffer
	 * has been consumed, thus we must delay detaching the handle.
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
	 * If isc__nm_tlsdns_read() was called, it will be satisfied by
	 * single DNS message in the next call.
	 */
	sock->recv_read = false;

	/*
	 * An assertion failure here means that there's an erroneous
	 * extra nmhandle detach happening in the callback and
	 * isc__nm_resume_processing() is called while we're
	 * processing the buffer.
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

	if (isc__nmsocket_closing(sock)) {
		tlsdns_set_tls_shutdown(sock->tls.tls);
		tlsdns_keep_client_tls_session(sock);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
tls_cycle_input(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_SUCCESS;
	int err = 0;
	int rv = 1;

	if (sock->tls.state == TLS_STATE_IO) {
		size_t len;

		for (;;) {
			(void)SSL_peek(sock->tls.tls, &(char){ '\0' }, 0);

			int pending = SSL_pending(sock->tls.tls);
			if (pending > (int)ISC_NETMGR_TCP_RECVBUF_SIZE) {
				pending = (int)ISC_NETMGR_TCP_RECVBUF_SIZE;
			}

			if (pending != 0) {
				if ((sock->buf_len + pending) > sock->buf_size)
				{
					isc__nm_alloc_dnsbuf(
						sock, sock->buf_len + pending);
				}

				len = 0;
				rv = SSL_read_ex(sock->tls.tls,
						 sock->buf + sock->buf_len,
						 sock->buf_size - sock->buf_len,
						 &len);
				if (rv != 1) {
					/*
					 * Process what's in the buffer so far
					 */
					result = isc__nm_process_sock_buffer(
						sock);
					if (result != ISC_R_SUCCESS) {
						goto failure;
					}
					/*
					 * FIXME: Should we call
					 * isc__nm_failed_read_cb()?
					 */
					break;
				}

				INSIST((size_t)pending == len);

				sock->buf_len += len;
			}
			result = isc__nm_process_sock_buffer(sock);
			if (result != ISC_R_SUCCESS) {
				goto failure;
			}

			if (pending == 0) {
				break;
			}
		}
	} else if (!SSL_is_init_finished(sock->tls.tls)) {
		if (SSL_is_server(sock->tls.tls)) {
			rv = SSL_accept(sock->tls.tls);
		} else {
			rv = SSL_connect(sock->tls.tls);
		}

	} else {
		rv = 1;
	}

	if (rv <= 0) {
		err = SSL_get_error(sock->tls.tls, rv);
	}

	switch (err) {
	case SSL_ERROR_WANT_READ:
		if (sock->tls.state == TLS_STATE_NONE &&
		    !SSL_is_init_finished(sock->tls.tls))
		{
			sock->tls.state = TLS_STATE_HANDSHAKE;
			result = isc__nm_process_sock_buffer(sock);
			if (result != ISC_R_SUCCESS) {
				goto failure;
			}
		}
		/* else continue reading */
		break;
	case SSL_ERROR_WANT_WRITE:
		async_tlsdns_cycle(sock);
		break;
	case SSL_ERROR_WANT_X509_LOOKUP:
		/* Continue reading/writing */
		break;
	case 0:
		/* Everything is ok, continue */
		break;
	case SSL_ERROR_ZERO_RETURN:
		return (ISC_R_EOF);
	default:
		return (ISC_R_TLSERROR);
	}

	/* Stop state after handshake */
	if (sock->tls.state == TLS_STATE_HANDSHAKE &&
	    SSL_is_init_finished(sock->tls.tls))
	{
		const unsigned char *alpn = NULL;
		unsigned int alpnlen = 0;

		isc__nmsocket_log_tls_session_reuse(sock, sock->tls.tls);

		isc_tls_get_selected_alpn(sock->tls.tls, &alpn, &alpnlen);
		if (alpn != NULL && alpnlen == ISC_TLS_DOT_PROTO_ALPN_ID_LEN &&
		    memcmp(ISC_TLS_DOT_PROTO_ALPN_ID, alpn,
			   ISC_TLS_DOT_PROTO_ALPN_ID_LEN) == 0)
		{
			sock->tls.alpn_negotiated = true;
		}

		sock->tls.state = TLS_STATE_IO;

		if (SSL_is_server(sock->tls.tls)) {
			REQUIRE(sock->recv_handle != NULL);
			result = sock->accept_cb(sock->recv_handle,
						 ISC_R_SUCCESS,
						 sock->accept_cbarg);

			if (result != ISC_R_SUCCESS) {
				isc_nmhandle_detach(&sock->recv_handle);
				goto failure;
			}
		} else {
			isc__nm_uvreq_t *req = sock->tls.pending_req;
			sock->tls.pending_req = NULL;

			isc__nmsocket_timer_stop(sock);
			uv_handle_set_data((uv_handle_t *)&sock->read_timer,
					   sock);

			atomic_compare_exchange_enforced(
				&sock->connecting, &(bool){ true }, false);
			isc__nm_connectcb(sock, req, ISC_R_SUCCESS, true);
		}
		async_tlsdns_cycle(sock);
	}
failure:
	return (result);
}

static void
tls_error(isc_nmsocket_t *sock, isc_result_t result) {
	switch (sock->tls.state) {
	case TLS_STATE_HANDSHAKE:
	case TLS_STATE_IO:
		if (atomic_load(&sock->connecting)) {
			isc__nm_uvreq_t *req = sock->tls.pending_req;
			sock->tls.pending_req = NULL;

			isc__nm_failed_connect_cb(sock, req, result, false);
		} else {
			isc__nm_tlsdns_failed_read_cb(sock, result, false);
		}
		break;
	case TLS_STATE_ERROR:
		return;
	default:
		break;
	}

	sock->tls.state = TLS_STATE_ERROR;
	sock->tls.pending_error = result;

	isc__nmsocket_shutdown(sock);
}

static void
call_pending_send_callbacks(isc_nmsocket_t *sock, const isc_result_t result) {
	isc__nm_uvreq_t *cbreq = ISC_LIST_HEAD(sock->tls.sendreqs);
	while (cbreq != NULL) {
		isc__nm_uvreq_t *next = ISC_LIST_NEXT(cbreq, link);
		ISC_LIST_UNLINK(sock->tls.sendreqs, cbreq, link);
		INSIST(sock == cbreq->handle->sock);
		isc__nm_sendcb(sock, cbreq, result, false);
		cbreq = next;
	}
}

static void
free_senddata(isc_nmsocket_t *sock, const isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tls.senddata.base != NULL);
	REQUIRE(sock->tls.senddata.length > 0);

	isc_mem_put(sock->mgr->mctx, sock->tls.senddata.base,
		    sock->tls.senddata.length);
	sock->tls.senddata.base = NULL;
	sock->tls.senddata.length = 0;

	call_pending_send_callbacks(sock, result);
}

static void
tls_write_cb(uv_write_t *req, int status) {
	isc_result_t result = status != 0 ? isc__nm_uverr2result(status)
					  : ISC_R_SUCCESS;
	isc__nm_uvreq_t *uvreq = (isc__nm_uvreq_t *)req->data;
	isc_nmsocket_t *sock = uvreq->sock;

	isc_nm_timer_stop(uvreq->timer);
	isc_nm_timer_detach(&uvreq->timer);

	free_senddata(sock, result);

	isc__nm_uvreq_put(&uvreq, sock);

	if (status != 0) {
		tls_error(sock, result);
		return;
	}

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		tls_error(sock, result);
		return;
	}
}

static isc_result_t
tls_cycle_output(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_SUCCESS;
	int pending;

	while ((pending = BIO_pending(sock->tls.app_rbio)) > 0) {
		isc__nm_uvreq_t *req = NULL;
		size_t bytes;
		int rv;
		int r;

		if (sock->tls.senddata.base != NULL ||
		    sock->tls.senddata.length > 0)
		{
			break;
		}

		if (pending > (int)ISC_NETMGR_TCP_RECVBUF_SIZE) {
			pending = (int)ISC_NETMGR_TCP_RECVBUF_SIZE;
		}

		sock->tls.senddata.base = isc_mem_get(sock->mgr->mctx, pending);
		sock->tls.senddata.length = pending;

		/* It's a bit misnomer here, but it does the right thing */
		req = isc__nm_get_read_req(sock, NULL);
		req->uvbuf.base = (char *)sock->tls.senddata.base;
		req->uvbuf.len = sock->tls.senddata.length;

		rv = BIO_read_ex(sock->tls.app_rbio, req->uvbuf.base,
				 req->uvbuf.len, &bytes);

		RUNTIME_CHECK(rv == 1);
		INSIST((size_t)pending == bytes);

		r = uv_try_write(&sock->uv_handle.stream, &req->uvbuf, 1);

		if (r == pending) {
			/* Wrote everything, restart */
			isc__nm_uvreq_put(&req, sock);
			free_senddata(sock, ISC_R_SUCCESS);
			continue;
		}

		if (r > 0) {
			/* Partial write, send rest asynchronously */
			memmove(req->uvbuf.base, req->uvbuf.base + r,
				req->uvbuf.len - r);
			req->uvbuf.len = req->uvbuf.len - r;
		} else if (r == UV_ENOSYS || r == UV_EAGAIN) {
			/* uv_try_write is not supported, send
			 * asynchronously */
		} else {
			result = isc__nm_uverr2result(r);
			isc__nm_uvreq_put(&req, sock);
			free_senddata(sock, result);
			break;
		}

		r = uv_write(&req->uv_req.write, &sock->uv_handle.stream,
			     &req->uvbuf, 1, tls_write_cb);
		if (r < 0) {
			result = isc__nm_uverr2result(r);
			isc__nm_uvreq_put(&req, sock);
			free_senddata(sock, result);
			break;
		}

		isc_nm_timer_create(req->handle, isc__nmsocket_writetimeout_cb,
				    req, &req->timer);
		if (sock->write_timeout > 0) {
			isc_nm_timer_start(req->timer, sock->write_timeout);
		}

		break;
	}

	return (result);
}

static isc_result_t
tls_pop_error(isc_nmsocket_t *sock) {
	isc_result_t result;

	if (sock->tls.state != TLS_STATE_ERROR) {
		return (ISC_R_SUCCESS);
	}

	if (sock->tls.pending_error == ISC_R_SUCCESS) {
		return (ISC_R_TLSERROR);
	}

	result = sock->tls.pending_error;
	sock->tls.pending_error = ISC_R_SUCCESS;

	return (result);
}

static isc_result_t
tls_cycle(isc_nmsocket_t *sock) {
	isc_result_t result;

	/*
	 * Clear the TLS error queue so that SSL_get_error() and SSL I/O
	 * routine calls will not get affected by prior error statuses.
	 *
	 * See here:
	 * https://www.openssl.org/docs/man3.0/man3/SSL_get_error.html
	 *
	 * In particular, it mentions the following:
	 *
	 * The current thread's error queue must be empty before the
	 * TLS/SSL I/O operation is attempted, or SSL_get_error() will not
	 * work reliably.
	 *
	 * As we use the result of SSL_get_error() to decide on I/O
	 * operations, we need to ensure that it works reliably by
	 * cleaning the error queue.
	 *
	 * The sum of details: https://stackoverflow.com/a/37980911
	 */
	ERR_clear_error();

	if (isc__nmsocket_closing(sock)) {
		return (ISC_R_CANCELED);
	}

	result = tls_pop_error(sock);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	if (sock->tls.cycle) {
		return (ISC_R_SUCCESS);
	}

	sock->tls.cycle = true;
	result = tls_cycle_input(sock);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	result = tls_cycle_output(sock);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}
done:
	sock->tls.cycle = false;

	return (result);
}

static void
async_tlsdns_cycle(isc_nmsocket_t *sock) {
	isc__netievent_tlsdnscycle_t *ievent = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	/* Socket was closed midflight by isc__nm_tlsdns_shutdown() */
	if (isc__nmsocket_closing(sock)) {
		return;
	}

	ievent = isc__nm_get_netievent_tlsdnscycle(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsdnscycle(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnscycle_t *ievent =
		(isc__netievent_tlsdnscycle_t *)ev0;
	isc_result_t result;
	isc_nmsocket_t *sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());

	sock = ievent->sock;

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		tls_error(sock, result);
	}
}

void
isc__nm_tlsdns_read_cb(uv_stream_t *stream, ssize_t nread,
		       const uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)stream);
	size_t len;
	isc_result_t result;
	int rv;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->reading));
	REQUIRE(buf != NULL);

	if (isc__nmsocket_closing(sock)) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED, true);
		goto free;
	}

	if (nread < 0) {
		if (nread != UV_EOF) {
			isc__nm_incstats(sock, STATID_RECVFAIL);
		}

		isc__nm_failed_read_cb(sock, isc__nm_uverr2result(nread), true);

		goto free;
	}

	if (!atomic_load(&sock->client)) {
		sock->read_timeout = atomic_load(&sock->mgr->idle);
	}

	/*
	 * The input has to be fed into BIO
	 */
	rv = BIO_write_ex(sock->tls.app_wbio, buf->base, (size_t)nread, &len);

	if (rv <= 0 || (size_t)nread != len) {
		isc__nm_failed_read_cb(sock, ISC_R_TLSERROR, true);
		goto free;
	}

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		isc__nm_failed_read_cb(sock, result, true);
	}
free:
	async_tlsdns_cycle(sock);

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
	 * Create a tlsdnsaccept event and pass it using the async
	 * channel.
	 */

	isc__netievent_tlsdnsaccept_t *ievent =
		isc__nm_get_netievent_tlsdnsaccept(sock->mgr, sock, quota);
	isc__nm_maybe_enqueue_ievent(&sock->mgr->workers[sock->tid],
				     (isc__netievent_t *)ievent);
}

/*
 * This is called after we get a quota_accept_cb() callback.
 */
void
isc__nm_async_tlsdnsaccept(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsaccept_t *ievent =
		(isc__netievent_tlsdnsaccept_t *)ev0;
	isc_result_t result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());

	result = accept_connection(ievent->sock, ievent->quota);
	isc__nm_accept_connection_log(result, can_log_tlsdns_quota());
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
	isc__nmsocket_init(csock, ssock->mgr, isc_nm_tlsdnssocket,
			   &ssock->iface);
	csock->tid = ssock->tid;
	csock->extrahandlesize = ssock->extrahandlesize;
	isc__nmsocket_attach(ssock, &csock->server);
	csock->accept_cb = ssock->accept_cb;
	csock->accept_cbarg = ssock->accept_cbarg;
	csock->recv_cb = ssock->recv_cb;
	csock->recv_cbarg = ssock->recv_cbarg;
	csock->quota = quota;
	atomic_init(&csock->accepting, true);

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

	csock->tls.state = TLS_STATE_NONE;

	csock->tls.tls = isc_tls_create(ssock->tls.ctx);
	RUNTIME_CHECK(csock->tls.tls != NULL);

	r = BIO_new_bio_pair(&csock->tls.ssl_wbio, ISC_NETMGR_TCP_RECVBUF_SIZE,
			     &csock->tls.app_rbio, ISC_NETMGR_TCP_RECVBUF_SIZE);
	RUNTIME_CHECK(r == 1);

	r = BIO_new_bio_pair(&csock->tls.ssl_rbio, ISC_NETMGR_TCP_RECVBUF_SIZE,
			     &csock->tls.app_wbio, ISC_NETMGR_TCP_RECVBUF_SIZE);
	RUNTIME_CHECK(r == 1);

#if HAVE_SSL_SET0_RBIO && HAVE_SSL_SET0_WBIO
	/*
	 * Note that if the rbio and wbio are the same then
	 * SSL_set0_rbio() and SSL_set0_wbio() each take ownership of
	 * one reference. Therefore it may be necessary to increment the
	 * number of references available using BIO_up_ref(3) before
	 * calling the set0 functions.
	 */
	SSL_set0_rbio(csock->tls.tls, csock->tls.ssl_rbio);
	SSL_set0_wbio(csock->tls.tls, csock->tls.ssl_wbio);
#else
	SSL_set_bio(csock->tls.tls, csock->tls.ssl_rbio, csock->tls.ssl_wbio);
#endif

	SSL_set_accept_state(csock->tls.tls);

	/* FIXME: Set SSL_MODE_RELEASE_BUFFERS */

	atomic_store(&csock->accepting, false);

	isc__nm_incstats(csock, STATID_ACCEPT);

	csock->read_timeout = atomic_load(&csock->mgr->init);

	csock->closehandle_cb = isc__nm_resume_processing;

	/*
	 * We need to keep the handle alive until we fail to read or
	 * connection is closed by the other side, it will be detached
	 * via prep_destroy()->tlsdns_close_direct().
	 *
	 * The handle will be either detached on acceptcb failure or in
	 * the readcb.
	 */
	csock->recv_handle = isc__nmhandle_get(csock, NULL, &local);

	/*
	 * The initial timer has been set, update the read timeout for
	 * the next reads.
	 */
	csock->read_timeout = (atomic_load(&csock->keepalive)
				       ? atomic_load(&csock->mgr->keepalive)
				       : atomic_load(&csock->mgr->idle));

	result = isc__nm_process_sock_buffer(csock);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

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
isc__nm_tlsdns_send(isc_nmhandle_t *handle, isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg) {
	isc__netievent_tlsdnssend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	*(uint16_t *)uvreq->tcplen = htons(region->length);
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	isc_nmhandle_attach(handle, &uvreq->handle);

	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	ievent = isc__nm_get_netievent_tlsdnssend(sock->mgr, sock, uvreq);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
	return;
}

/*
 * Handle 'tcpsend' async event - send a packet on the socket
 */
void
isc__nm_async_tlsdnssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc_result_t result;
	isc__netievent_tlsdnssend_t *ievent =
		(isc__netievent_tlsdnssend_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;

	UNUSED(worker);

	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->write_timeout == 0) {
		sock->write_timeout =
			(atomic_load(&sock->keepalive)
				 ? atomic_load(&sock->mgr->keepalive)
				 : atomic_load(&sock->mgr->idle));
	}

	result = tlsdns_send_direct(sock, uvreq);
	if (result != ISC_R_SUCCESS) {
		isc__nm_incstats(sock, STATID_SENDFAIL);
		isc__nm_failed_send_cb(sock, uvreq, result);
	}
}

static void
tlsdns_send_enqueue(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc__netievent_tlsdnssend_t *ievent =
		isc__nm_get_netievent_tlsdnssend(sock->mgr, sock, req);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

static isc_result_t
tlsdns_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc_result_t result;
	int err = 0;
	int rv;
	size_t bytes = 0;
	size_t sendlen;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	result = tls_pop_error(sock);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (isc__nmsocket_closing(sock)) {
		return (ISC_R_CANCELED);
	}

	/* Writes won't succeed until handshake end */
	if (!SSL_is_init_finished(sock->tls.tls)) {
		goto requeue;
	}

	/*
	 * Try to send any pending data before trying to call SSL_write_ex().
	 * Otherwise, it could fail with SSL_ERROR_WANT_WRITE error.
	 *
	 * It is important to stress that we want to avoid this happening
	 * due to how SSL_write_ex() works - mainly to avoid partial
	 * writes.
	 *
	 * Although the documentation for these functions is vague, it is
	 * not stated that partial writes are not possible. On the
	 * contrary, one can deduce that it is possible and recovering
	 * from this situation is complicated and unreasonably hard to
	 * implement to the point when it is better to avoid this
	 * situation altogether.
	 *
	 * In particular, the following can be found in the documentation:
	 *
	 * "The write functions will only return with success when the
	 * complete contents of buf of length num has been written. This
	 * default behaviour can be changed with the
	 * SSL_MODE_ENABLE_PARTIAL_WRITE option of
	 * SSL_CTX_set_mode(3). When this flag is set, the write functions
	 * will also return with success when a partial write has been
	 * successfully completed. In this case, the write function
	 * operation is considered completed. The bytes are sent, and a
	 * new write call with a new buffer (with the already sent bytes
	 * removed) must be started. A partial write is performed with the
	 * size of a message block, which is 16kB."

	 * That is, it is said that success is returned only when the
	 * complete chunk of data is written (encrypted), but it does not
	 * mention that partial writes are not possible (the behaviour can
	 * be changed using SSL_MODE_ENABLE_PARTIAL_WRITE). Another
	 * important aspect of this passage is that a partial write of up
	 * to 16 kilobytes can happen, and the call still can
	 * fail. Needless to say, this amount of data may include more
	 * than one DNS message.
	 *
	 * One could expect that SSL_write_ex() should return the number
	 * of bytes written, but no, that is not guaranteed (emphasis is
	 * mine): "*On success* SSL_write_ex() will store the number of
	 * bytes written in *written."
	 *
	 * Moreover, we can find the following guidance on how to handle
	 * the SSL_ERROR_WANT_WRITE error in the "Warnings" section of the
	 * documentation:

	 * "When a write function call has to be repeated because
	 * SSL_get_error(3) returned SSL_ERROR_WANT_READ or
	 * SSL_ERROR_WANT_WRITE, it must be repeated with the same
	 * arguments. The data that was passed might have been partially
	 * processed. When SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER was set
	 * using SSL_CTX_set_mode(3) the pointer can be different, but the
	 * data and length should still be the same."
	 *
	 * That is, when a call to SSL_write_ex() fails with
	 * SSL_ERROR_WANT_WRITE, we must attempt to make the next call to
	 * the function exactly with the same arguments. Of course, the
	 * code is structured in such a way that we cannot guarantee that
	 * (and keeping track of that would be unreasonably complicated to
	 * implement). The best we can do to avoid this error is to get
	 * (and send) the outgoing data from the SSL buffer ASAP before
	 * processing the subsequent write request. We can achieve that by
	 * calling tls_cycle() and rescheduling the write request for
	 * being processed later.
	 */
	if (BIO_pending(sock->tls.app_rbio) > 0) {
		/* Handle any pending data and requeue the write request. */
		goto cycle;
	}

	/*
	 * There's no SSL_writev(), so we need to use a local buffer to
	 * assemble the whole message
	 */
	worker = &sock->mgr->workers[sock->tid];
	sendlen = req->uvbuf.len + sizeof(uint16_t);
	memmove(worker->sendbuf, req->tcplen, sizeof(uint16_t));
	memmove(worker->sendbuf + sizeof(uint16_t), req->uvbuf.base,
		req->uvbuf.len);

	rv = SSL_write_ex(sock->tls.tls, worker->sendbuf, sendlen, &bytes);
	if (rv > 0) {
		INSIST(sendlen == bytes);

		ISC_LIST_APPEND(sock->tls.sendreqs, req, link);
		async_tlsdns_cycle(sock);
		return (ISC_R_SUCCESS);
	}

	/* Nothing was written, maybe enqueue? */
	err = SSL_get_error(sock->tls.tls, rv);

	switch (err) {
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_READ:
		break;
	case 0:
		UNREACHABLE();
	default:
		return (ISC_R_TLSERROR);
	}

cycle:
	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

requeue:
	tlsdns_send_enqueue(sock, req);

	return (result);
}

static void
tlsdns_stop_cb(uv_handle_t *handle) {
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

	isc__nm_incstats(sock, STATID_CLOSE);

	atomic_store(&sock->listening, false);

	BIO_free_all(sock->tls.app_rbio);
	BIO_free_all(sock->tls.app_wbio);

	if (sock->tls.ctx != NULL) {
		isc_tlsctx_free(&sock->tls.ctx);
	}

	isc__nmsocket_detach(&sock);
}

static void
tlsdns_close_sock(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

	if (!atomic_compare_exchange_strong(&sock->closed, &(bool){ false },
					    true))
	{
		UNREACHABLE();
	}

	isc__nm_incstats(sock, STATID_CLOSE);

	if (sock->server != NULL) {
		isc__nmsocket_detach(&sock->server);
	}

	atomic_store(&sock->connected, false);

	if (sock->tls.tls != NULL) {
		/*
		 * Let's shutdown the TLS session properly so that the session
		 * will remain resumable, if required.
		 */
		tlsdns_set_tls_shutdown(sock->tls.tls);
		tlsdns_keep_client_tls_session(sock);
		isc_tls_free(&sock->tls.tls);
	}

	BIO_free_all(sock->tls.app_rbio);
	BIO_free_all(sock->tls.app_wbio);

	if (sock->tls.ctx != NULL) {
		isc_tlsctx_free(&sock->tls.ctx);
	}

	isc__nmsocket_prep_destroy(sock);
}

static void
tlsdns_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	tlsdns_close_sock(sock);
}

static void
read_timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	REQUIRE(VALID_NMSOCK(sock));

	if (sock->parent) {
		uv_close(&sock->uv_handle.handle, tlsdns_stop_cb);
	} else if (uv_is_closing(&sock->uv_handle.handle)) {
		tlsdns_close_sock(sock);
	} else {
		uv_close(&sock->uv_handle.handle, tlsdns_close_cb);
	}
}

static void
stop_tlsdns_child(isc_nmsocket_t *sock) {
	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->tid == isc_nm_tid());

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	tlsdns_close_direct(sock);

	atomic_fetch_sub(&sock->parent->rchildren, 1);

	isc_barrier_wait(&sock->parent->stoplistening);
}

static void
stop_tlsdns_parent(isc_nmsocket_t *sock) {
	isc_nmsocket_t *csock = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tlsdnslistener);

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
	stop_tlsdns_child(csock);

	atomic_store(&sock->closed, true);
	isc__nmsocket_prep_destroy(sock);
}

static void
tlsdns_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

	REQUIRE(sock->tls.pending_req == NULL);

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
isc__nm_tlsdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(!isc__nmsocket_active(sock));

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		tlsdns_close_direct(sock);
	} else {
		/*
		 * We need to create an event and pass it using async
		 * channel
		 */
		isc__netievent_tlsdnsclose_t *ievent =
			isc__nm_get_netievent_tlsdnsclose(sock->mgr, sock);

		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tlsdnsclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsclose_t *ievent =
		(isc__netievent_tlsdnsclose_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	tlsdns_close_direct(sock);
}

static void
tlsdns_close_connect_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	REQUIRE(VALID_NMSOCK(sock));

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->tid == isc_nm_tid());

	isc__nmsocket_prep_destroy(sock);
	isc__nmsocket_detach(&sock);
}

void
isc__nm_tlsdns_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	/*
	 * If the socket is active, mark it inactive and
	 * continue. If it isn't active, stop now.
	 */
	if (!isc__nmsocket_deactivate(sock)) {
		return;
	}

	if (sock->tls.tls) {
		/* Shutdown any active TLS connections */
		tlsdns_set_tls_shutdown(sock->tls.tls);
	}

	if (atomic_load(&sock->accepting)) {
		return;
	}

	/* TLS handshake hasn't been completed yet */
	if (atomic_load(&sock->connecting)) {
		isc_nmsocket_t *tsock = NULL;

		/*
		 * TCP connection has been established, now waiting on
		 * TLS handshake to complete
		 */
		if (sock->tls.pending_req != NULL) {
			isc_result_t result = ISC_R_CANCELED;
			isc__nm_uvreq_t *req = sock->tls.pending_req;
			sock->tls.pending_req = NULL;

			if (peer_verification_has_failed(sock)) {
				/*
				 * Save error message as 'sock->tls' will get
				 * detached.
				 */
				sock->tls.tls_verify_errmsg =
					isc_tls_verify_peer_result_string(
						sock->tls.tls);
				result = ISC_R_TLSBADPEERCERT;
			}
			isc__nm_failed_connect_cb(sock, req, result, false);
			return;
		}

		/* The TCP connection hasn't been established yet */
		isc__nmsocket_attach(sock, &tsock);
		uv_close(&sock->uv_handle.handle, tlsdns_close_connect_cb);
		return;
	}

	if (sock->statichandle != NULL) {
		if (isc__nm_closing(sock)) {
			isc__nm_failed_read_cb(sock, ISC_R_SHUTTINGDOWN, false);
		} else {
			isc__nm_failed_read_cb(sock, ISC_R_CANCELED, false);
		}
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
isc__nm_tlsdns_cancelread(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tlsdnscancel_t *ievent = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	ievent = isc__nm_get_netievent_tlsdnscancel(sock->mgr, sock, handle);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsdnscancel(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnscancel_t *ievent =
		(isc__netievent_tlsdnscancel_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	isc__nm_failed_read_cb(sock, ISC_R_EOF, false);
}

/* Zone transfers/updates over TLS are allowed only when "dot" ALPN
 * was negotiated.
 *
 * Per the XoT spec, we must also check that the TLS version is >=
 * 1.3. The check could be added here. However, we still need to
 * support platforms where no cryptographic library with TLSv1.3
 * support is available. As a result of this we cannot be too strict
 * regarding the minimal TLS protocol version in order to make it
 * possible to do encrypted zone transfers over TLSv1.2, as it would
 * not be right to leave users on these platforms without means for
 * encrypted zone transfers using BIND only.
 *
 * The ones requiring strict compatibility with the specification
 * could disable TLSv1.2 in the configuration file.
 */
isc_result_t
isc__nm_tlsdns_xfr_checkperm(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	if (!sock->tls.alpn_negotiated) {
		return (ISC_R_DOTALPNERROR);
	}

	return (ISC_R_SUCCESS);
}

const char *
isc__nm_tlsdns_verify_tls_peer_result_string(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlsdnssocket);

	sock = handle->sock;
	if (sock->tls.tls == NULL) {
		return (sock->tls.tls_verify_errmsg);
	}

	return (isc_tls_verify_peer_result_string(sock->tls.tls));
}

void
isc__nm_async_tlsdns_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx,
				const int tid) {
	REQUIRE(tid >= 0);

	isc_tlsctx_free(&listener->children[tid].tls.ctx);
	isc_tlsctx_attach(tlsctx, &listener->children[tid].tls.ctx);
}

void
isc__nm_tlsdns_cleanup_data(isc_nmsocket_t *sock) {
	if (sock->type == isc_nm_tlsdnslistener ||
	    sock->type == isc_nm_tlsdnssocket)
	{
		if (sock->tls.client_sess_cache != NULL) {
			INSIST(atomic_load(&sock->client));
			INSIST(sock->type == isc_nm_tlsdnssocket);
			isc_tlsctx_client_session_cache_detach(
				&sock->tls.client_sess_cache);
		}
		if (sock->tls.ctx != NULL) {
			INSIST(ISC_LIST_EMPTY(sock->tls.sendreqs));
			isc_tlsctx_free(&sock->tls.ctx);
		}
	}
}

static void
tlsdns_keep_client_tls_session(isc_nmsocket_t *sock) {
	/*
	 * Ensure that the isc_tls_t is being accessed from
	 * within the worker thread the socket is bound to.
	 */
	REQUIRE(sock->tid == isc_nm_tid());
	if (sock->tls.client_sess_cache != NULL &&
	    sock->tls.client_session_saved == false)
	{
		INSIST(atomic_load(&sock->client));
		isc_tlsctx_client_session_cache_keep_sockaddr(
			sock->tls.client_sess_cache, &sock->peer,
			sock->tls.tls);
		sock->tls.client_session_saved = true;
	}
}
