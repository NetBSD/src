/*	$NetBSD: tlsdns.c,v 1.3 2021/04/29 17:26:12 christos Exp $	*/

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

#include <libgen.h>
#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
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

/*%<
 *
 * Maximum number of simultaneous handles in flight supported for a single
 * connected TLSDNS socket. This value was chosen arbitrarily, and may be
 * changed in the future.
 */

static atomic_uint_fast32_t last_tlsdnsquota_log = ATOMIC_VAR_INIT(0);

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
	isc_result_t result = ISC_R_DEFAULT;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->tid == isc_nm_tid());

	worker = &sock->mgr->workers[sock->tid];

	atomic_store(&sock->connecting, true);

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	RUNTIME_CHECK(r == 0);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop, &sock->timer);
	RUNTIME_CHECK(r == 0);
	uv_handle_set_data((uv_handle_t *)&sock->timer, sock);

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
			   &req->peer.type.sa, tlsdns_connect_cb);
	if (r != 0) {
		isc__nm_incstats(sock->mgr,
				 sock->statsindex[STATID_CONNECTFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CONNECT]);

	atomic_store(&sock->connected, true);

done:
	result = isc__nm_uverr2result(r);

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
	REQUIRE(sock->iface != NULL);
	REQUIRE(sock->parent == NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	result = tlsdns_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		atomic_store(&sock->active, false);
		isc__nm_tlsdns_close(sock);
		isc__nm_uvreq_put(&req, sock);
	}

	/*
	 * The sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&sock);
}

static void
tlsdns_connect_cb(uv_connect_t *uvreq, int status) {
	isc_result_t result;
	isc__nm_uvreq_t *req = NULL;
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)uvreq->handle);
	struct sockaddr_storage ss;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->connecting));

	req = uv_handle_get_data((uv_handle_t *)uvreq);

	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMHANDLE(req->handle));

	/* Socket was closed midflight by isc__nm_tlsdns_shutdown() */
	if (!isc__nmsocket_active(sock)) {
		result = ISC_R_CANCELED;
		goto error;
	}

	if (status != 0) {
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

	sock->tls.state = TLS_STATE_NONE;
	sock->tls.ssl = SSL_new(sock->tls.ctx);
	RUNTIME_CHECK(sock->tls.ssl != NULL);

	/*
	 *
	 */
	r = BIO_new_bio_pair(&sock->tls.ssl_wbio, ISC_NETMGR_TLSBUF_SIZE,
			     &sock->tls.app_rbio, ISC_NETMGR_TLSBUF_SIZE);
	RUNTIME_CHECK(r == 1);

	r = BIO_new_bio_pair(&sock->tls.ssl_rbio, ISC_NETMGR_TLSBUF_SIZE,
			     &sock->tls.app_wbio, ISC_NETMGR_TLSBUF_SIZE);
	RUNTIME_CHECK(r == 1);

#if HAVE_SSL_SET0_RBIO && HAVE_SSL_SET0_WBIO
	/*
	 * Note that if the rbio and wbio are the same then SSL_set0_rbio() and
	 * SSL_set0_wbio() each take ownership of one reference. Therefore it
	 * may be necessary to increment the number of references available
	 * using BIO_up_ref(3) before calling the set0 functions.
	 */
	SSL_set0_rbio(sock->tls.ssl, sock->tls.ssl_rbio);
	SSL_set0_wbio(sock->tls.ssl, sock->tls.ssl_wbio);
#else
	SSL_set_bio(sock->tls.ssl, sock->tls.ssl_rbio, sock->tls.ssl_wbio);
#endif

	SSL_set_connect_state(sock->tls.ssl);

	result = isc_sockaddr_fromsockaddr(&sock->peer, (struct sockaddr *)&ss);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	sock->tls.pending_req = req;

	isc__nm_process_sock_buffer(sock);

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	return;

error:
	isc__nm_failed_connect_cb(sock, req, result);
}

isc_result_t
isc_nm_tlsdnsconnect(isc_nm_t *mgr, isc_nmiface_t *local, isc_nmiface_t *peer,
		     isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
		     size_t extrahandlesize, isc_tlsctx_t *sslctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tlsdnsconnect_t *ievent = NULL;
	isc__nm_uvreq_t *req = NULL;
	sa_family_t sa_family;
	uv_os_sock_t fd;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(local != NULL);
	REQUIRE(peer != NULL);
	REQUIRE(sslctx != NULL);

	sa_family = peer->addr.type.sa.sa_family;

	/*
	 * The socket() call can fail spuriously on FreeBSD 12, so we need to
	 * handle the failure early and gracefully.
	 */
	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &fd);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tlsdnssocket, local);

	sock->extrahandlesize = extrahandlesize;
	sock->connect_timeout = timeout;
	sock->result = ISC_R_DEFAULT;
	sock->fd = fd;
	sock->tls.ctx = sslctx;

	atomic_init(&sock->client, true);

	result = isc__nm_socket_connectiontimeout(fd, timeout);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	req = isc__nm_uvreq_get(mgr, sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->peer = peer->addr;
	req->local = local->addr;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface->addr);

	ievent = isc__nm_get_netievent_tlsdnsconnect(mgr, sock, req);

	if (isc__nm_in_netthread()) {
		atomic_store(&sock->active, true);
		sock->tid = isc_nm_tid();
		isc__nm_async_tlsdnsconnect(&mgr->workers[sock->tid],
					    (isc__netievent_t *)ievent);
		isc__nm_put_netievent_tlsdnsconnect(mgr, ievent);
	} else {
		atomic_init(&sock->active, false);
		sock->tid = isc_random_uniform(mgr->nworkers);
		isc__nm_enqueue_ievent(&mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
	LOCK(&sock->lock);
	result = sock->result;
	while (result == ISC_R_DEFAULT) {
		WAIT(&sock->cond, &sock->lock);
		result = sock->result;
	}
	atomic_store(&sock->active, true);
	BROADCAST(&sock->scond);
	UNLOCK(&sock->lock);
	INSIST(result != ISC_R_DEFAULT);

	return (result);
}

static uv_os_sock_t
isc__nm_tlsdns_lb_socket(sa_family_t sa_family) {
	isc_result_t result;
	uv_os_sock_t sock;

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	(void)isc__nm_socket_incoming_cpu(sock);

	/* FIXME: set mss */

	result = isc__nm_socket_reuse(sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

#if HAVE_SO_REUSEPORT_LB
	result = isc__nm_socket_reuse_lb(sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
#endif

	return (sock);
}

isc_result_t
isc_nm_listentlsdns(isc_nm_t *mgr, isc_nmiface_t *iface,
		    isc_nm_recv_cb_t recv_cb, void *recv_cbarg,
		    isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		    size_t extrahandlesize, int backlog, isc_quota_t *quota,
		    isc_tlsctx_t *sslctx, isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	sa_family_t sa_family = iface->addr.type.sa.sa_family;
	size_t children_size = 0;
#if !HAVE_SO_REUSEPORT_LB && !defined(WIN32)
	uv_os_sock_t fd = -1;
#endif

	REQUIRE(VALID_NM(mgr));

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tlsdnslistener, iface);

	sock->rchildren = 0;
#if defined(WIN32)
	sock->nchildren = 1;
#else
	sock->nchildren = mgr->nworkers;
#endif
	children_size = sock->nchildren * sizeof(sock->children[0]);
	sock->children = isc_mem_get(mgr->mctx, children_size);
	memset(sock->children, 0, children_size);

	sock->result = ISC_R_DEFAULT;
	sock->tid = isc_random_uniform(sock->nchildren);
	sock->fd = -1;
	sock->tls.ctx = sslctx;

#if !HAVE_SO_REUSEPORT_LB && !defined(WIN32)
	fd = isc__nm_tlsdns_lb_socket(sa_family);
#endif

	for (size_t i = 0; i < sock->nchildren; i++) {
		isc__netievent_tlsdnslisten_t *ievent = NULL;
		isc_nmsocket_t *csock = &sock->children[i];

		isc__nmsocket_init(csock, mgr, isc_nm_tlsdnssocket, iface);
		csock->parent = sock;
		csock->accept_cb = accept_cb;
		csock->accept_cbarg = accept_cbarg;
		csock->recv_cb = recv_cb;
		csock->recv_cbarg = recv_cbarg;
		csock->extrahandlesize = extrahandlesize;
		csock->backlog = backlog;
		csock->tid = i;
		csock->tls.ctx = sslctx;

		/*
		 * We don't attach to quota, just assign - to avoid
		 * increasing quota unnecessarily.
		 */
		csock->pquota = quota;
		isc_quota_cb_init(&csock->quotacb, quota_accept_cb, csock);

#if HAVE_SO_REUSEPORT_LB || defined(WIN32)
		csock->fd = isc__nm_tlsdns_lb_socket(sa_family);
#else
		csock->fd = dup(fd);
#endif
		REQUIRE(csock->fd >= 0);

		ievent = isc__nm_get_netievent_tlsdnslisten(mgr, csock);
		isc__nm_enqueue_ievent(&mgr->workers[i],
				       (isc__netievent_t *)ievent);
	}

#if !HAVE_SO_REUSEPORT_LB && !defined(WIN32)
	isc__nm_closesocket(fd);
#endif

	LOCK(&sock->lock);
	while (sock->rchildren != sock->nchildren) {
		WAIT(&sock->cond, &sock->lock);
	}
	result = sock->result;
	atomic_store(&sock->active, true);
	BROADCAST(&sock->scond);
	UNLOCK(&sock->lock);
	INSIST(result != ISC_R_DEFAULT);

	if (result == ISC_R_SUCCESS) {
		REQUIRE(sock->rchildren == sock->nchildren);
		*sockp = sock;
	} else {
		atomic_store(&sock->active, false);
		isc__nm_tlsdns_stoplistening(sock);
		isc_nmsocket_close(&sock);
	}

	return (result);
}

void
isc__nm_async_tlsdnslisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnslisten_t *ievent =
		(isc__netievent_tlsdnslisten_t *)ev0;
	isc_nmiface_t *iface = NULL;
	sa_family_t sa_family;
	int r;
	int flags = 0;
	isc_nmsocket_t *sock = NULL;
	isc_result_t result = ISC_R_DEFAULT;

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());
	REQUIRE(VALID_NMSOCK(ievent->sock->parent));

	sock = ievent->sock;
	iface = sock->iface;
	sa_family = iface->addr.type.sa.sa_family;

	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->iface != NULL);
	REQUIRE(sock->parent != NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	/* TODO: set min mss */

	r = uv_tcp_init(&worker->loop, &sock->uv_handle.tcp);
	RUNTIME_CHECK(r == 0);
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	/* This keeps the socket alive after everything else is gone */
	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });

	r = uv_timer_init(&worker->loop, &sock->timer);
	RUNTIME_CHECK(r == 0);
	uv_handle_set_data((uv_handle_t *)&sock->timer, sock);

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

#if HAVE_SO_REUSEPORT_LB || defined(WIN32)
	r = isc_uv_tcp_freebind(&sock->uv_handle.tcp,
				&sock->iface->addr.type.sa, flags);
	if (r < 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		goto done;
	}
#else
	if (sock->parent->fd == -1) {
		r = isc_uv_tcp_freebind(&sock->uv_handle.tcp,
					&sock->iface->addr.type.sa, flags);
		if (r < 0) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_BINDFAIL]);
			goto done;
		}
		sock->parent->uv_handle.tcp.flags = sock->uv_handle.tcp.flags;
		sock->parent->fd = sock->fd;
	} else {
		/* The socket is already bound, just copy the flags */
		sock->uv_handle.tcp.flags = sock->parent->uv_handle.tcp.flags;
	}
#endif

	/*
	 * The callback will run in the same thread uv_listen() was called
	 * from, so a race with tlsdns_connection_cb() isn't possible.
	 */
	r = uv_listen((uv_stream_t *)&sock->uv_handle.tcp, sock->backlog,
		      tlsdns_connection_cb);
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

	sock->parent->rchildren += 1;
	if (sock->parent->result == ISC_R_DEFAULT) {
		sock->parent->result = result;
	}
	SIGNAL(&sock->parent->cond);
	if (!atomic_load(&sock->parent->active)) {
		WAIT(&sock->parent->scond, &sock->parent->lock);
	}
	INSIST(atomic_load(&sock->parent->active));
	UNLOCK(&sock->parent->lock);
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

	if (isc__nm_inactive(ssock)) {
		result = ISC_R_CANCELED;
		goto done;
	}

	if (ssock->pquota != NULL) {
		result = isc_quota_attach_cb(ssock->pquota, &quota,
					     &ssock->quotacb);
		if (result == ISC_R_QUOTA) {
			isc__nm_incstats(ssock->mgr,
					 ssock->statsindex[STATID_ACCEPTFAIL]);
			return;
		}
	}

	result = accept_connection(ssock, quota);
done:
	if (result != ISC_R_SUCCESS && result != ISC_R_NOCONN) {
		if ((result != ISC_R_QUOTA && result != ISC_R_SOFTQUOTA) ||
		    can_log_tlsdns_quota())
		{
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
				      "TCP connection failed: %s",
				      isc_result_totext(result));
		}
	}
}

static void
enqueue_stoplistening(isc_nmsocket_t *sock) {
	isc__netievent_tlsdnsstop_t *ievent =
		isc__nm_get_netievent_tlsdnsstop(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_tlsdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnslistener);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		INSIST(0);
		ISC_UNREACHABLE();
	}
	enqueue_stoplistening(sock);
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

	rv = SSL_shutdown(sock->tls.ssl);

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

	err = SSL_get_error(sock->tls.ssl, rv);

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
		INSIST(0);
		ISC_UNREACHABLE();
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

	/*
	 * If network manager is interlocked, re-enqueue the event for later.
	 */
	if (!isc__nm_acquire_interlocked(sock->mgr)) {
		enqueue_stoplistening(sock);
	} else {
		stop_tlsdns_parent(sock);
		isc__nm_drop_interlocked(sock->mgr);
	}
}

void
isc__nm_tlsdns_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);

	if (sock->tls.pending_req) {
		isc__nm_uvreq_t *req = sock->tls.pending_req;
		sock->tls.pending_req = NULL;
		isc__nm_failed_connect_cb(sock, req, ISC_R_CANCELED);
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
	isc__nmsocket_prep_destroy(sock);

	/* We need to detach from quota after the read callback function had a
	 * chance to be executed. */
	if (sock->quota) {
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

	ievent = isc__nm_get_netievent_tlsdnsread(sock->mgr, sock);

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
isc__nm_async_tlsdnsread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsread_t *ievent =
		(isc__netievent_tlsdnsread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_result_t result = ISC_R_SUCCESS;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (isc__nm_inactive(sock)) {
		sock->reading = true;
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED);
		return;
	}

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		isc__nm_failed_read_cb(sock, result);
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

	if (isc__nm_inactive(sock)) {
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
	 * If isc__nm_tlsdns_read() was called, it will be satisfied by single
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

static isc_result_t
tls_cycle_input(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_SUCCESS;
	int err = 0;
	int rv = 1;

	if (sock->tls.state == TLS_STATE_IO) {
		size_t len;

		for (;;) {
			(void)SSL_peek(sock->tls.ssl, &(char){ '\0' }, 0);

			int pending = SSL_pending(sock->tls.ssl);
			if (pending > ISC_NETMGR_TLSBUF_SIZE) {
				pending = ISC_NETMGR_TLSBUF_SIZE;
			}

			if ((sock->buf_len + pending) > sock->buf_size) {
				isc__nm_alloc_dnsbuf(sock,
						     sock->buf_len + pending);
			}

			len = 0;
			rv = SSL_read_ex(sock->tls.ssl,
					 sock->buf + sock->buf_len,
					 sock->buf_size - sock->buf_len, &len);
			if (rv != 1) {
				/* Process what's in the buffer so far */
				isc__nm_process_sock_buffer(sock);

				/* FIXME: Should we call failed_read_cb()? */
				break;
			}

			REQUIRE((size_t)pending == len);

			sock->buf_len += len;

			isc__nm_process_sock_buffer(sock);
		}
	} else if (!SSL_is_init_finished(sock->tls.ssl)) {
		if (SSL_is_server(sock->tls.ssl)) {
			rv = SSL_accept(sock->tls.ssl);
		} else {
			rv = SSL_connect(sock->tls.ssl);
		}

	} else {
		rv = 1;
	}

	if (rv <= 0) {
		err = SSL_get_error(sock->tls.ssl, rv);
	}

	switch (err) {
	case SSL_ERROR_WANT_READ:
		if (sock->tls.state == TLS_STATE_NONE &&
		    !SSL_is_init_finished(sock->tls.ssl)) {
			sock->tls.state = TLS_STATE_HANDSHAKE;
			isc__nm_process_sock_buffer(sock);
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
	    SSL_is_init_finished(sock->tls.ssl))
	{
		sock->tls.state = TLS_STATE_IO;

		if (SSL_is_server(sock->tls.ssl)) {
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

			atomic_store(&sock->connecting, false);

			isc__nm_connectcb(sock, req, ISC_R_SUCCESS);
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
		isc__nmsocket_timer_stop(sock);
		isc__nm_stop_reading(sock);
		break;
	case TLS_STATE_ERROR:
		return;
	default:
		break;
	}

	sock->tls.state = TLS_STATE_ERROR;
	sock->tls.pending_error = result;

	/* tlsdns_close_direct(sock); */
}

static void
free_senddata(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tls.senddata.base != NULL);
	REQUIRE(sock->tls.senddata.len > 0);

	isc_mem_put(sock->mgr->mctx, sock->tls.senddata.base,
		    sock->tls.senddata.len);
	sock->tls.senddata.base = NULL;
	sock->tls.senddata.len = 0;
}

static void
tls_write_cb(uv_write_t *req, int status) {
	isc_result_t result;
	isc__nm_uvreq_t *uvreq = (isc__nm_uvreq_t *)req->data;
	isc_nmsocket_t *sock = uvreq->sock;

	free_senddata(sock);

	if (status != 0) {
		tls_error(sock, isc__nm_uverr2result(status));
		return;
	}

	isc__nm_uvreq_put(&uvreq, sock);

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
		int err;

		if (sock->tls.senddata.base != NULL ||
		    sock->tls.senddata.len > 0) {
			break;
		}

		if (pending > ISC_NETMGR_TLSBUF_SIZE) {
			pending = ISC_NETMGR_TLSBUF_SIZE;
		}

		sock->tls.senddata.base = isc_mem_get(sock->mgr->mctx, pending);
		sock->tls.senddata.len = pending;

		rv = BIO_read_ex(sock->tls.app_rbio, sock->tls.senddata.base,
				 pending, &bytes);

		RUNTIME_CHECK(rv == 1);
		INSIST((size_t)pending == bytes);

		err = uv_try_write(&sock->uv_handle.stream, &sock->tls.senddata,
				   1);

		if (err == pending) {
			/* Wrote everything, restart */
			free_senddata(sock);
			continue;
		}

		if (err > 0) {
			/* Partial write, send rest asynchronously */
			memmove(sock->tls.senddata.base,
				sock->tls.senddata.base + err, pending - err);
			sock->tls.senddata.len = pending - err;
		} else if (err == UV_ENOSYS || err == UV_EAGAIN) {
			/* uv_try_write is not supported, send asynchronously */
		} else {
			result = isc__nm_uverr2result(err);
			free_senddata(sock);
			break;
		}

		req = isc__nm_uvreq_get(sock->mgr, sock);
		req->uvbuf.base = (char *)sock->tls.senddata.base;
		req->uvbuf.len = sock->tls.senddata.len;

		err = uv_write(&req->uv_req.write, &sock->uv_handle.stream,
			       &sock->tls.senddata, 1, tls_write_cb);

		INSIST(err == 0);

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
	REQUIRE(VALID_NMSOCK(sock));

	/* Socket was closed midflight by isc__nm_tlsdns_shutdown() */
	if (!isc__nmsocket_active(sock)) {
		return;
	}

	isc__netievent_tlsdnscycle_t *ievent =
		isc__nm_get_netievent_tlsdnscycle(sock->mgr, sock);
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
	REQUIRE(sock->reading);
	REQUIRE(buf != NULL);

	if (isc__nm_inactive(sock)) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED);
		goto free;
	}

	if (nread < 0) {
		if (nread != UV_EOF) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_RECVFAIL]);
		}

		isc__nm_failed_read_cb(sock, isc__nm_uverr2result(nread));

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
		isc__nm_failed_read_cb(sock, ISC_R_TLSERROR);
		goto free;
	}

	result = tls_cycle(sock);
	if (result != ISC_R_SUCCESS) {
		isc__nm_failed_read_cb(sock, result);
	}
free:
	async_tlsdns_cycle(sock);
	isc__nm_free_uvbuf(sock, buf);
}

static void
quota_accept_cb(isc_quota_t *quota, void *sock0) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)sock0;

	REQUIRE(VALID_NMSOCK(sock));

	/*
	 * Create a tlsdnsaccept event and pass it using the async channel.
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
	if (result != ISC_R_SUCCESS && result != ISC_R_NOCONN) {
		if ((result != ISC_R_QUOTA && result != ISC_R_SOFTQUOTA) ||
		    can_log_tlsdns_quota())
		{
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_NETMGR, ISC_LOG_ERROR,
				      "TCP connection failed: %s",
				      isc_result_totext(result));
		}
	}
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

	if (isc__nm_inactive(ssock)) {
		if (quota != NULL) {
			isc_quota_detach(&quota);
		}
		return (ISC_R_CANCELED);
	}

	REQUIRE(ssock->accept_cb != NULL);

	csock = isc_mem_get(ssock->mgr->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(csock, ssock->mgr, isc_nm_tlsdnssocket,
			   ssock->iface);
	csock->tid = ssock->tid;
	csock->extrahandlesize = ssock->extrahandlesize;
	isc__nmsocket_attach(ssock, &csock->server);
	csock->accept_cb = ssock->accept_cb;
	csock->accept_cbarg = ssock->accept_cbarg;
	csock->recv_cb = ssock->recv_cb;
	csock->recv_cbarg = ssock->recv_cbarg;
	csock->quota = quota;
	csock->accepting = true;

	worker = &csock->mgr->workers[csock->tid];

	r = uv_tcp_init(&worker->loop, &csock->uv_handle.tcp);
	RUNTIME_CHECK(r == 0);
	uv_handle_set_data(&csock->uv_handle.handle, csock);

	r = uv_timer_init(&worker->loop, &csock->timer);
	RUNTIME_CHECK(r == 0);
	uv_handle_set_data((uv_handle_t *)&csock->timer, csock);

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

	csock->tls.state = TLS_STATE_NONE;

	csock->tls.ssl = SSL_new(ssock->tls.ctx);

	if (csock->tls.ssl == NULL) {
		char errbuf[256];
		unsigned long err = ERR_get_error();

		ERR_error_string_n(err, errbuf, sizeof(errbuf));
		fprintf(stderr, "%s:SSL_new(%p) -> %s\n", __func__,
			ssock->tls.ctx, errbuf);
	}

	RUNTIME_CHECK(csock->tls.ssl != NULL);

	r = BIO_new_bio_pair(&csock->tls.ssl_wbio, ISC_NETMGR_TLSBUF_SIZE,
			     &csock->tls.app_rbio, ISC_NETMGR_TLSBUF_SIZE);
	RUNTIME_CHECK(r == 1);

	r = BIO_new_bio_pair(&csock->tls.ssl_rbio, ISC_NETMGR_TLSBUF_SIZE,
			     &csock->tls.app_wbio, ISC_NETMGR_TLSBUF_SIZE);
	RUNTIME_CHECK(r == 1);

#if HAVE_SSL_SET0_RBIO && HAVE_SSL_SET0_WBIO
	/*
	 * Note that if the rbio and wbio are the same then SSL_set0_rbio() and
	 * SSL_set0_wbio() each take ownership of one reference. Therefore it
	 * may be necessary to increment the number of references available
	 * using BIO_up_ref(3) before calling the set0 functions.
	 */
	SSL_set0_rbio(csock->tls.ssl, csock->tls.ssl_rbio);
	SSL_set0_wbio(csock->tls.ssl, csock->tls.ssl_wbio);
#else
	SSL_set_bio(csock->tls.ssl, csock->tls.ssl_rbio, csock->tls.ssl_wbio);
#endif

	SSL_set_accept_state(csock->tls.ssl);

	/* FIXME: Set SSL_MODE_RELEASE_BUFFERS */

	csock->accepting = false;

	isc__nm_incstats(csock->mgr, csock->statsindex[STATID_ACCEPT]);

	csock->read_timeout = atomic_load(&csock->mgr->init);

	csock->closehandle_cb = isc__nm_resume_processing;

	/*
	 * We need to keep the handle alive until we fail to read or connection
	 * is closed by the other side, it will be detached via
	 * prep_destroy()->tlsdns_close_direct().
	 */
	isc_nmhandle_attach(handle, &csock->recv_handle);

	/*
	 * The initial timer has been set, update the read timeout for the next
	 * reads.
	 */
	csock->read_timeout = (atomic_load(&csock->keepalive)
				       ? atomic_load(&csock->mgr->keepalive)
				       : atomic_load(&csock->mgr->idle));

	isc_nmhandle_detach(&handle);

	isc__nm_process_sock_buffer(csock);

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
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;
	isc__netievent_tlsdnssend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;

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

	result = tlsdns_send_direct(sock, uvreq);
	if (result != ISC_R_SUCCESS) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
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

	if (isc__nm_inactive(sock)) {
		return (ISC_R_CANCELED);
	}

	/* Writes won't succeed until handshake end */
	if (!SSL_is_init_finished(sock->tls.ssl)) {
		goto requeue;
	}

	/*
	 * There's no SSL_writev(), so we need to use a local buffer to assemble
	 * the whole message
	 */
	worker = &sock->mgr->workers[sock->tid];
	sendlen = req->uvbuf.len + sizeof(uint16_t);
	memmove(worker->sendbuf, req->tcplen, sizeof(uint16_t));
	memmove(worker->sendbuf + sizeof(uint16_t), req->uvbuf.base,
		req->uvbuf.len);

	rv = SSL_write_ex(sock->tls.ssl, worker->sendbuf, sendlen, &bytes);
	if (rv > 0) {
		/* SSL_write_ex() doesn't do partial writes */
		INSIST(sendlen == bytes);

		isc__nm_sendcb(sock, req, ISC_R_SUCCESS, true);
		async_tlsdns_cycle(sock);
		return (ISC_R_SUCCESS);
	}

	/* Nothing was written, maybe enqueue? */
	err = SSL_get_error(sock->tls.ssl, rv);

	switch (err) {
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_READ:
		break;
	case 0:
		INSIST(0);
		ISC_UNREACHABLE();
	default:
		return (ISC_R_TLSERROR);
	}

	result = tls_cycle(sock);

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
					    true)) {
		INSIST(0);
		ISC_UNREACHABLE();
	}

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CLOSE]);

	atomic_store(&sock->listening, false);

	BIO_free_all(sock->tls.app_rbio);
	BIO_free_all(sock->tls.app_wbio);

	sock->tls.ctx = NULL;

	isc__nmsocket_detach(&sock);
}

static void
tlsdns_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

	if (!atomic_compare_exchange_strong(&sock->closed, &(bool){ false },
					    true)) {
		INSIST(0);
		ISC_UNREACHABLE();
	}

	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CLOSE]);

	if (sock->server != NULL) {
		isc__nmsocket_detach(&sock->server);
	}

	atomic_store(&sock->connected, false);

	if (sock->tls.ssl) {
		SSL_free(sock->tls.ssl);
		sock->tls.ssl = NULL;
	}

	BIO_free_all(sock->tls.app_rbio);
	BIO_free_all(sock->tls.app_wbio);

	sock->tls.ctx = NULL;

	isc__nmsocket_prep_destroy(sock);
}

static void
timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	uv_handle_set_data(handle, NULL);

	if (sock->parent) {
		uv_close(&sock->uv_handle.handle, tlsdns_stop_cb);
	} else {
		uv_close(&sock->uv_handle.handle, tlsdns_close_cb);
	}
}

static void
stop_tlsdns_child(isc_nmsocket_t *sock) {
	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(sock->tid == isc_nm_tid());

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		return;
	}

	tlsdns_close_direct(sock);

	LOCK(&sock->parent->lock);
	sock->parent->rchildren -= 1;
	UNLOCK(&sock->parent->lock);
	BROADCAST(&sock->parent->cond);
}

static void
stop_tlsdns_parent(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnslistener);

	for (size_t i = 0; i < sock->nchildren; i++) {
		isc__netievent_tlsdnsstop_t *ievent = NULL;
		isc_nmsocket_t *csock = &sock->children[i];

		REQUIRE(VALID_NMSOCK(csock));

		atomic_store(&csock->active, false);

		if (csock->tid == isc_nm_tid()) {
			stop_tlsdns_child(csock);
			continue;
		}

		ievent = isc__nm_get_netievent_tlsdnsstop(sock->mgr, csock);
		isc__nm_enqueue_ievent(&sock->mgr->workers[csock->tid],
				       (isc__netievent_t *)ievent);
	}

	LOCK(&sock->lock);
	while (sock->rchildren > 0) {
		WAIT(&sock->cond, &sock->lock);
	}
	atomic_store(&sock->closed, true);
	UNLOCK(&sock->lock);

	isc__nmsocket_prep_destroy(sock);
}

static void
tlsdns_close_direct(isc_nmsocket_t *sock) {
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
	uv_close((uv_handle_t *)&sock->timer, timer_close_cb);
}

void
isc__nm_tlsdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);
	REQUIRE(!isc__nmsocket_active(sock));

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		tlsdns_close_direct(sock);
	} else {
		/*
		 * We need to create an event and pass it using async channel
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

	if (atomic_load(&sock->connecting) || sock->accepting) {
		return;
	}

	if (sock->tls.pending_req) {
		isc__nm_uvreq_t *req = sock->tls.pending_req;
		sock->tls.pending_req = NULL;
		isc__nm_failed_connect_cb(sock, req, ISC_R_CANCELED);
	}

	if (sock->statichandle) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED);
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

	isc__nm_failed_read_cb(sock, ISC_R_EOF);
}

void
isc_nm_tlsdns_sequential(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlsdnssocket);

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

void
isc_nm_tlsdns_keepalive(isc_nmhandle_t *handle, bool value) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type != isc_nm_tlsdnssocket);

	sock = handle->sock;

	atomic_store(&sock->keepalive, value);
}
