/*	$NetBSD: tcpdns.c,v 1.4 2021/02/19 16:42:20 christos Exp $	*/

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
#include "uv-compat.h"

#define TCPDNS_CLIENTS_PER_CONN 23
/*%<
 *
 * Maximum number of simultaneous handles in flight supported for a single
 * connected TCPDNS socket. This value was chosen arbitrarily, and may be
 * changed in the future.
 */

static atomic_uint_fast32_t last_tcpdnsquota_log = ATOMIC_VAR_INIT(0);

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

static void
tcpdns_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);

static void
resume_processing(void *arg);

static isc_result_t
tcpdns_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);

static void
tcpdns_close_direct(isc_nmsocket_t *sock);

static isc_result_t
tcpdns_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);
static void
tcpdns_connect_cb(uv_connect_t *uvreq, int status);

static void
tcpdns_connection_cb(uv_stream_t *server, int status);

static void
read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

static void
tcpdns_close_cb(uv_handle_t *uvhandle);

static isc_result_t
accept_connection(isc_nmsocket_t *ssock, isc_quota_t *quota);

static void
quota_accept_cb(isc_quota_t *quota, void *sock0);

static void
failed_accept_cb(isc_nmsocket_t *sock, isc_result_t eresult);

static void
failed_send_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
	       isc_result_t eresult);

static void
stop_tcpdns_parent(isc_nmsocket_t *sock);
static void
stop_tcpdns_child(isc_nmsocket_t *sock);

static void
start_sock_timer(isc_nmsocket_t *sock);

static void
process_sock_buffer(isc_nmsocket_t *sock);

static void
stop_reading(isc_nmsocket_t *sock);

static isc__nm_uvreq_t *
get_read_req(isc_nmsocket_t *sock);

/*
 * Regular TCP buffer, should suffice in most cases.
 */
#define NM_REG_BUF 4096
/*
 * Two full DNS packets with lengths.
 * netmgr receives 64k at most so there's no risk
 * of overrun.
 */
#define NM_BIG_BUF (65535 + 2) * 2
static inline void
alloc_dnsbuf(isc_nmsocket_t *sock, size_t len) {
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

static bool
inactive(isc_nmsocket_t *sock) {
	return (!isc__nmsocket_active(sock) || atomic_load(&sock->closing) ||
		atomic_load(&sock->mgr->closing) ||
		(sock->server != NULL && !isc__nmsocket_active(sock->server)));
}

static void
failed_accept_cb(isc_nmsocket_t *sock, isc_result_t eresult) {
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

static void
failed_connect_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
		  isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->connecting));
	REQUIRE(req->cb.connect != NULL);

	atomic_store(&sock->connecting, false);

	isc__nmsocket_clearcb(sock);
	isc__nm_connectcb(sock, req, eresult);

	isc__nmsocket_prep_destroy(sock);
}

static isc_result_t
tcpdns_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
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
			   &req->peer.type.sa, tcpdns_connect_cb);
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
isc__nm_async_tcpdnsconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsconnect_t *ievent =
		(isc__netievent_tcpdnsconnect_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	isc_result_t result = ISC_R_SUCCESS;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->iface != NULL);
	REQUIRE(sock->parent == NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	result = tcpdns_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		atomic_store(&sock->active, false);
		isc__nm_tcpdns_close(sock);
		isc__nm_uvreq_put(&req, sock);
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
	REQUIRE(atomic_load(&sock->connecting));

	req = uv_handle_get_data((uv_handle_t *)uvreq);

	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMHANDLE(req->handle));

	/* Socket was closed midflight by isc__nm_tcpdns_shutdown() */
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

	atomic_store(&sock->connecting, false);

	result = isc_sockaddr_fromsockaddr(&sock->peer, (struct sockaddr *)&ss);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc__nm_connectcb(sock, req, ISC_R_SUCCESS);

	return;

error:
	failed_connect_cb(sock, req, result);
}

isc_result_t
isc_nm_tcpdnsconnect(isc_nm_t *mgr, isc_nmiface_t *local, isc_nmiface_t *peer,
		     isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
		     size_t extrahandlesize) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tcpdnsconnect_t *ievent = NULL;
	isc__nm_uvreq_t *req = NULL;
	sa_family_t sa_family;
	uv_os_sock_t fd;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(local != NULL);
	REQUIRE(peer != NULL);

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
	isc__nmsocket_init(sock, mgr, isc_nm_tcpdnssocket, local);

	sock->extrahandlesize = extrahandlesize;
	sock->connect_timeout = timeout;
	sock->result = ISC_R_DEFAULT;
	sock->fd = fd;
	atomic_init(&sock->client, true);

	result = isc__nm_socket_connectiontimeout(fd, timeout);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	req = isc__nm_uvreq_get(mgr, sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->peer = peer->addr;
	req->local = local->addr;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface->addr);

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
isc__nm_tcpdns_lb_socket(sa_family_t sa_family) {
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
isc_nm_listentcpdns(isc_nm_t *mgr, isc_nmiface_t *iface,
		    isc_nm_recv_cb_t recv_cb, void *recv_cbarg,
		    isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		    size_t extrahandlesize, int backlog, isc_quota_t *quota,
		    isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	sa_family_t sa_family = iface->addr.type.sa.sa_family;
	size_t children_size = 0;
#if !HAVE_SO_REUSEPORT_LB && !defined(WIN32)
	uv_os_sock_t fd = -1;
#endif

	REQUIRE(VALID_NM(mgr));

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_tcpdnslistener, iface);

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

#if !HAVE_SO_REUSEPORT_LB && !defined(WIN32)
	fd = isc__nm_tcpdns_lb_socket(sa_family);
#endif

	for (size_t i = 0; i < sock->nchildren; i++) {
		isc__netievent_tcpdnslisten_t *ievent = NULL;
		isc_nmsocket_t *csock = &sock->children[i];

		isc__nmsocket_init(csock, mgr, isc_nm_tcpdnssocket, iface);
		csock->parent = sock;
		csock->accept_cb = accept_cb;
		csock->accept_cbarg = accept_cbarg;
		csock->recv_cb = recv_cb;
		csock->recv_cbarg = recv_cbarg;
		csock->extrahandlesize = extrahandlesize;
		csock->backlog = backlog;
		csock->tid = i;
		/*
		 * We don't attach to quota, just assign - to avoid
		 * increasing quota unnecessarily.
		 */
		csock->pquota = quota;
		isc_quota_cb_init(&csock->quotacb, quota_accept_cb, csock);

#if HAVE_SO_REUSEPORT_LB || defined(WIN32)
		csock->fd = isc__nm_tcpdns_lb_socket(sa_family);
#else
		csock->fd = dup(fd);
#endif
		REQUIRE(csock->fd >= 0);

		ievent = isc__nm_get_netievent_tcpdnslisten(mgr, csock);
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
		isc__nm_tcpdns_stoplistening(sock);
		isc_nmsocket_close(&sock);
	}

	return (result);
}

void
isc__nm_async_tcpdnslisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnslisten_t *ievent =
		(isc__netievent_tcpdnslisten_t *)ev0;
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

	REQUIRE(sock->type == isc_nm_tcpdnssocket);
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

	if (inactive(ssock)) {
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
		    can_log_tcpdns_quota())
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
	isc__netievent_tcpdnsstop_t *ievent =
		isc__nm_get_netievent_tcpdnsstop(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_tcpdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnslistener);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		INSIST(0);
		ISC_UNREACHABLE();
	}
	enqueue_stoplistening(sock);
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

	/*
	 * If network manager is interlocked, re-enqueue the event for later.
	 */
	if (!isc__nm_acquire_interlocked(sock->mgr)) {
		enqueue_stoplistening(sock);
	} else {
		stop_tcpdns_parent(sock);
		isc__nm_drop_interlocked(sock->mgr);
	}
}

static void
failed_read_cb(isc_nmsocket_t *sock, isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	stop_reading(sock);

	if (!sock->recv_read) {
		goto destroy;
	}
	sock->recv_read = false;

	if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = get_read_req(sock);
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

static void
failed_send_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
	       isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	if (req->cb.send != NULL) {
		isc__nm_sendcb(sock, req, eresult);
	} else {
		isc__nm_uvreq_put(&req, sock);
	}
}

static isc__nm_uvreq_t *
get_read_req(isc_nmsocket_t *sock) {
	isc__nm_uvreq_t *req = NULL;

	req = isc__nm_uvreq_get(sock->mgr, sock);
	req->cb.recv = sock->recv_cb;
	req->cbarg = sock->recv_cbarg;

	if (atomic_load(&sock->client)) {
		isc_nmhandle_attach(sock->statichandle, &req->handle);
	} else {
		req->handle = isc__nmhandle_get(sock, NULL, NULL);
	}

	return req;
}

static void
readtimeout_cb(uv_timer_t *timer) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)timer);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->reading);

	/*
	 * Timeout; stop reading and process whatever we have.
	 */

	failed_read_cb(sock, ISC_R_TIMEDOUT);
}

static void
start_sock_timer(isc_nmsocket_t *sock) {
	if (sock->read_timeout > 0) {
		int r = uv_timer_start(&sock->timer, readtimeout_cb,
				       sock->read_timeout, 0);
		REQUIRE(r == 0);
	}
}

static void
stop_sock_timer(isc_nmsocket_t *sock) {
	int r = uv_timer_stop(&sock->timer);
	REQUIRE(r == 0);
}

static void
start_reading(isc_nmsocket_t *sock) {
	if (sock->reading) {
		return;
	}

	int r = uv_read_start(&sock->uv_handle.stream, tcpdns_alloc_cb,
			      read_cb);
	REQUIRE(r == 0);
	sock->reading = true;

	start_sock_timer(sock);
}

static void
stop_reading(isc_nmsocket_t *sock) {
	if (!sock->reading) {
		return;
	}

	int r = uv_read_stop(&sock->uv_handle.stream);
	REQUIRE(r == 0);
	sock->reading = false;

	stop_sock_timer(sock);
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

/*%<
 * Allocator for TCP read operations. Limited to size 2^16.
 *
 * Note this doesn't actually allocate anything, it just assigns the
 * worker's receive buffer to a socket, and marks it as "in use".
 */
static void
tcpdns_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(isc__nm_in_netthread());
	UNUSED(size);

	worker = &sock->mgr->workers[sock->tid];
	INSIST(!worker->recvbuf_inuse);

	buf->base = worker->recvbuf;
	buf->len = size;
	worker->recvbuf_inuse = true;
}

void
isc__nm_async_tcpdnsread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsread_t *ievent =
		(isc__netievent_tcpdnsread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	if (inactive(sock)) {
		sock->reading = true;
		failed_read_cb(sock, ISC_R_CANCELED);
		return;
	}

	process_sock_buffer(sock);
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
static isc_result_t
processbuffer(isc_nmsocket_t *sock) {
	size_t len;
	isc__nm_uvreq_t *req = NULL;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (inactive(sock)) {
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

	req = get_read_req(sock);
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
	if (len > 0) {
		memmove(sock->buf, sock->buf + len, sock->buf_len);
	}

	isc_nmhandle_detach(&handle);

	return (ISC_R_SUCCESS);
}

static void
read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)stream);
	uint8_t *base = NULL;
	size_t len;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->reading);
	REQUIRE(buf != NULL);

	if (inactive(sock)) {
		failed_read_cb(sock, ISC_R_CANCELED);
		goto free;
	}

	if (nread < 0) {
		if (nread != UV_EOF) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_RECVFAIL]);
		}

		failed_read_cb(sock, isc__nm_uverr2result(nread));

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
		alloc_dnsbuf(sock, sock->buf_len + len);
	}
	memmove(sock->buf + sock->buf_len, base, len);
	sock->buf_len += len;

	if (!atomic_load(&sock->client)) {
		sock->read_timeout = atomic_load(&sock->mgr->idle);
	}

	process_sock_buffer(sock);
free:
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
	if (result != ISC_R_SUCCESS && result != ISC_R_NOCONN) {
		if ((result != ISC_R_QUOTA && result != ISC_R_SOFTQUOTA) ||
		    can_log_tcpdns_quota())
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

	if (inactive(ssock)) {
		if (quota != NULL) {
			isc_quota_detach(&quota);
		}
		return (ISC_R_CANCELED);
	}

	REQUIRE(ssock->accept_cb != NULL);

	csock = isc_mem_get(ssock->mgr->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(csock, ssock->mgr, isc_nm_tcpdnssocket,
			   ssock->iface);
	csock->tid = ssock->tid;
	csock->extrahandlesize = ssock->extrahandlesize;
	isc__nmsocket_attach(ssock, &csock->server);
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

	csock->accepting = false;

	isc__nm_incstats(csock->mgr, csock->statsindex[STATID_ACCEPT]);

	csock->read_timeout = atomic_load(&csock->mgr->init);

	csock->closehandle_cb = resume_processing;

	/*
	 * We need to keep the handle alive until we fail to read or connection
	 * is closed by the other side, it will be detached via
	 * prep_destroy()->tcpdns_close_direct().
	 */
	isc_nmhandle_attach(handle, &csock->recv_handle);
	start_reading(csock);

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

	failed_accept_cb(csock, result);

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
	isc_nmsocket_t *sock = uvreq->sock;

	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	if (status < 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
		failed_send_cb(sock, uvreq, isc__nm_uverr2result(status));
		return;
	}

	isc__nm_sendcb(sock, uvreq, ISC_R_SUCCESS);
}

/*
 * Handle 'tcpsend' async event - send a packet on the socket
 */
void
isc__nm_async_tcpdnssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc_result_t result;
	isc__netievent_tcpdnssend_t *ievent =
		(isc__netievent_tcpdnssend_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;

	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	result = tcpdns_send_direct(sock, uvreq);
	if (result != ISC_R_SUCCESS) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
		failed_send_cb(sock, uvreq, result);
	}
}

static isc_result_t
tcpdns_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tcpdnssocket);

	int r;
	uv_buf_t bufs[2] = { { .base = req->tcplen, .len = 2 },
			     { .base = req->uvbuf.base,
			       .len = req->uvbuf.len } };

	if (inactive(sock)) {
		return (ISC_R_CANCELED);
	}

	r = uv_write(&req->uv_req.write, &sock->uv_handle.stream, bufs, 2,
		     tcpdns_send_cb);
	if (r < 0) {
		return (isc__nm_uverr2result(r));
	}

	return (ISC_R_SUCCESS);
}

static void
tcpdns_stop_cb(uv_handle_t *handle) {
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

	atomic_store(&sock->listening, false);

	isc__nmsocket_detach(&sock);
}

static void
tcpdns_close_cb(uv_handle_t *handle) {
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

	isc__nmsocket_prep_destroy(sock);
}

static void
timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	if (sock->parent) {
		uv_close(&sock->uv_handle.handle, tcpdns_stop_cb);
	} else {
		uv_close(&sock->uv_handle.handle, tcpdns_close_cb);
	}
}

static void
stop_tcpdns_child(isc_nmsocket_t *sock) {
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(sock->tid == isc_nm_tid());

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		return;
	}

	tcpdns_close_direct(sock);

	LOCK(&sock->parent->lock);
	sock->parent->rchildren -= 1;
	UNLOCK(&sock->parent->lock);
	BROADCAST(&sock->parent->cond);
}

static void
stop_tcpdns_parent(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnslistener);

	for (size_t i = 0; i < sock->nchildren; i++) {
		isc__netievent_tcpdnsstop_t *ievent = NULL;
		isc_nmsocket_t *csock = &sock->children[i];
		REQUIRE(VALID_NMSOCK(csock));

		atomic_store(&csock->active, false);

		if (csock->tid == isc_nm_tid()) {
			stop_tcpdns_child(csock);
			continue;
		}

		ievent = isc__nm_get_netievent_tcpdnsstop(sock->mgr, csock);
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

	stop_reading(sock);
	uv_close((uv_handle_t *)&sock->timer, timer_close_cb);
}

void
isc__nm_tcpdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(!isc__nmsocket_active(sock));

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
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

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	UNUSED(worker);

	tcpdns_close_direct(sock);
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

	if (atomic_load(&sock->connecting) || sock->accepting) {
		return;
	}

	if (sock->statichandle) {
		failed_read_cb(sock, ISC_R_CANCELED);
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

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	failed_read_cb(sock, ISC_R_EOF);
}

void
isc__nm_tcpdns_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;

	sock->read_timeout = timeout;
	if (uv_is_active((uv_handle_t *)&sock->timer)) {
		start_sock_timer(sock);
	}
}

void
isc_nm_tcpdns_sequential(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tcpdnssocket);

	isc_nmsocket_t *sock = handle->sock;

	/*
	 * We don't want pipelining on this connection. That means
	 * that we need to pause after reading each request, and
	 * resume only after the request has been processed. This
	 * is done in resume_processing(), which is the socket's
	 * closehandle_cb callback, called whenever a handle
	 * is released.
	 */

	stop_reading(sock);
	atomic_store(&sock->sequential, true);
}

void
isc_nm_tcpdns_keepalive(isc_nmhandle_t *handle, bool value) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type != isc_nm_tcpdnssocket);

	isc_nmsocket_t *sock = handle->sock;

	atomic_store(&sock->keepalive, value);
}

static void
process_sock_buffer(isc_nmsocket_t *sock) {
	/*
	 * 1. When process_buffer receives incomplete DNS message,
	 *    we don't touch any timers
	 *
	 * 2. When we receive at least one full DNS message, we stop the timers
	 *    until resume_processing calls this function again and restarts the
	 *    reading and the timers
	 */

	/*
	 * Process a DNS messages.  Stop if this is client socket, or the server
	 * socket has been set to sequential mode or the number of queries we
	 * are processing simultaneously have reached the clients-per-connection
	 * limit.
	 */
	for (;;) {
		isc_result_t result = processbuffer(sock);
		switch (result) {
		case ISC_R_NOMORE:
			start_reading(sock);
			return;
		case ISC_R_CANCELED:
			stop_reading(sock);
			return;
		case ISC_R_SUCCESS:
			if (atomic_load(&sock->client) ||
			    atomic_load(&sock->sequential) ||
			    atomic_load(&sock->ah) >= TCPDNS_CLIENTS_PER_CONN)
			{
				stop_reading(sock);
				return;
			}
			break;
		default:
			INSIST(0);
		}
	}
}

static void
resume_processing(void *arg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tcpdnssocket);
	REQUIRE(!atomic_load(&sock->client));

	if (inactive(sock)) {
		return;
	}

	if (atomic_load(&sock->ah) == 0) {
		/* Nothing is active; sockets can timeout now */
		start_sock_timer(sock);
	}

	process_sock_buffer(sock);
}
