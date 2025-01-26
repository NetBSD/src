/*	$NetBSD: tcp.c,v 1.12 2025/01/26 16:25:43 christos Exp $	*/

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

#include <isc/async.h>
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
#include <isc/uv.h>

#include "../loop_p.h"
#include "netmgr-int.h"

static atomic_uint_fast32_t last_tcpquota_log = 0;

static bool
can_log_tcp_quota(void) {
	isc_stdtime_t last;
	isc_stdtime_t now = isc_stdtime_now();
	last = atomic_exchange_relaxed(&last_tcpquota_log, now);
	if (now != last) {
		return true;
	}

	return false;
}

static isc_result_t
tcp_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);

static isc_result_t
tcp_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req);
static void
tcp_connect_cb(uv_connect_t *uvreq, int status);
static void
tcp_stop_cb(uv_handle_t *handle);

static void
tcp_connection_cb(uv_stream_t *server, int status);

static void
tcp_close_cb(uv_handle_t *uvhandle);

static isc_result_t
accept_connection(isc_nmsocket_t *ssock);

static void
quota_accept_cb(void *arg);

static isc_result_t
tcp_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc__networker_t *worker = NULL;
	isc_result_t result = ISC_R_UNSET;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	REQUIRE(sock->tid == isc_tid());

	worker = sock->worker;

	sock->connecting = true;

	/* 2 minute timeout */
	result = isc__nm_socket_connectiontimeout(sock->fd, 120 * 1000);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	r = uv_tcp_init(&worker->loop->loop, &sock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	r = uv_tcp_open(&sock->uv_handle.tcp, sock->fd);
	if (r != 0) {
		isc__nm_closesocket(sock->fd);
		isc__nm_incstats(sock, STATID_OPENFAIL);
		return isc_uverr2result(r);
	}
	isc__nm_incstats(sock, STATID_OPEN);

	if (req->local.length != 0) {
		r = uv_tcp_bind(&sock->uv_handle.tcp, &req->local.type.sa, 0);
		if (r != 0) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			return isc_uverr2result(r);
		}
	}

	isc__nm_set_network_buffers(sock->worker->netmgr,
				    &sock->uv_handle.handle);

	uv_handle_set_data(&req->uv_req.handle, req);
	r = uv_tcp_connect(&req->uv_req.connect, &sock->uv_handle.tcp,
			   &req->peer.type.sa, tcp_connect_cb);
	if (r != 0) {
		isc__nm_incstats(sock, STATID_CONNECTFAIL);
		return isc_uverr2result(r);
	}

	uv_handle_set_data((uv_handle_t *)&sock->read_timer,
			   &req->uv_req.connect);
	isc__nmsocket_timer_start(sock);

	return ISC_R_SUCCESS;
}

static void
tcp_connect_cb(uv_connect_t *uvreq, int status) {
	isc_result_t result = ISC_R_UNSET;
	isc__nm_uvreq_t *req = NULL;
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)uvreq->handle);
	struct sockaddr_storage ss;
	isc__networker_t *worker = NULL;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	worker = sock->worker;

	req = uv_handle_get_data((uv_handle_t *)uvreq);

	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_NMHANDLE(req->handle));

	INSIST(sock->connecting);

	if (sock->timedout || status == UV_ETIMEDOUT) {
		/* Connection timed-out */
		result = ISC_R_TIMEDOUT;
		goto error;
	} else if (isc__nm_closing(worker)) {
		/* Network manager shutting down */
		result = ISC_R_SHUTTINGDOWN;
		goto error;
	} else if (isc__nmsocket_closing(sock)) {
		/* Connection canceled */
		result = ISC_R_CANCELED;
		goto error;
	} else if (status == UV_EADDRINUSE) {
		/*
		 * On FreeBSD the TCP connect() call sometimes results in a
		 * spurious transient EADDRINUSE. Try a few more times before
		 * giving up.
		 */
		if (--req->connect_tries > 0) {
			r = uv_tcp_connect(&req->uv_req.connect,
					   &sock->uv_handle.tcp,
					   &req->peer.type.sa, tcp_connect_cb);
			if (r != 0) {
				result = isc_uverr2result(r);
				goto error;
			}
			return;
		}
		result = isc_uverr2result(status);
		goto error;
	} else if (status != 0) {
		result = isc_uverr2result(status);
		goto error;
	}

	isc__nmsocket_timer_stop(sock);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	isc__nm_incstats(sock, STATID_CONNECT);
	r = uv_tcp_getpeername(&sock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc_uverr2result(r);
		goto error;
	}

	sock->connecting = false;
	sock->connected = true;

	result = isc_sockaddr_fromsockaddr(&sock->peer, (struct sockaddr *)&ss);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc__nm_connectcb(sock, req, ISC_R_SUCCESS, false);

	return;
error:
	isc__nm_failed_connect_cb(sock, req, result, false);
}

void
isc_nm_tcpconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		  isc_nm_cb_t connect_cb, void *connect_cbarg,
		  unsigned int timeout) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__nm_uvreq_t *req = NULL;
	sa_family_t sa_family;
	isc__networker_t *worker = NULL;
	uv_os_sock_t fd = -1;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(local != NULL);
	REQUIRE(peer != NULL);

	worker = &mgr->workers[isc_tid()];

	if (isc__nm_closing(worker)) {
		connect_cb(NULL, ISC_R_SHUTTINGDOWN, connect_cbarg);
		return;
	}

	sa_family = peer->type.sa.sa_family;

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &fd);
	if (result != ISC_R_SUCCESS) {
		connect_cb(NULL, result, connect_cbarg);
		return;
	}

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, isc_nm_tcpsocket, local, NULL);

	sock->connect_timeout = timeout;
	sock->fd = fd;
	sock->client = true;

	req = isc__nm_uvreq_get(sock);
	req->cb.connect = connect_cb;
	req->cbarg = connect_cbarg;
	req->peer = *peer;
	req->local = *local;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface);

	(void)isc__nm_socket_min_mtu(sock->fd, sa_family);
	(void)isc__nm_socket_tcp_maxseg(sock->fd, NM_MAXSEG);

	sock->active = true;

	result = tcp_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		sock->active = false;
		isc__nm_tcp_close(sock);
		isc__nm_connectcb(sock, req, result, true);
	}

	/*
	 * The sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&sock);
}

static uv_os_sock_t
isc__nm_tcp_lb_socket(isc_nm_t *mgr, sa_family_t sa_family) {
	isc_result_t result;
	uv_os_sock_t sock;

	result = isc__nm_socket(sa_family, SOCK_STREAM, 0, &sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	(void)isc__nm_socket_v6only(sock, sa_family);

	/* FIXME: set mss */

	result = isc__nm_socket_reuse(sock, 1);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (mgr->load_balance_sockets) {
		result = isc__nm_socket_reuse_lb(sock);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	return sock;
}

static void
start_tcp_child_job(void *arg) {
	isc_nmsocket_t *sock = arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_NMSOCK(sock->parent));
	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(sock->tid == isc_tid());

	sa_family_t sa_family = sock->iface.type.sa.sa_family;
	int r, flags = 0;
	isc_result_t result = ISC_R_UNSET;
	isc_loop_t *loop = sock->worker->loop;
	struct sockaddr_storage ss;

	(void)isc__nm_socket_min_mtu(sock->fd, sa_family);
	(void)isc__nm_socket_tcp_maxseg(sock->fd, NM_MAXSEG);

	r = uv_tcp_init(&loop->loop, &sock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	/* This keeps the socket alive after everything else is gone */
	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });

	r = uv_timer_init(&loop->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

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

	if (sock->worker->netmgr->load_balance_sockets) {
		r = isc__nm_tcp_freebind(&sock->uv_handle.tcp,
					 &sock->iface.type.sa, flags);
		if (r < 0) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			goto done;
		}
	} else if (sock->tid == 0) {
		r = isc__nm_tcp_freebind(&sock->uv_handle.tcp,
					 &sock->iface.type.sa, flags);
		if (r < 0) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			goto done;
		}
		sock->parent->uv_handle.tcp.flags = sock->uv_handle.tcp.flags;
	} else {
		/* The socket is already bound, just copy the flags */
		sock->uv_handle.tcp.flags = sock->parent->uv_handle.tcp.flags;
	}

	isc__nm_set_network_buffers(sock->worker->netmgr,
				    &sock->uv_handle.handle);

	/*
	 * The callback will run in the same thread uv_listen() was called
	 * from, so a race with tcp_connection_cb() isn't possible.
	 */
	r = uv_listen((uv_stream_t *)&sock->uv_handle.tcp, sock->backlog,
		      tcp_connection_cb);
	if (r != 0) {
		isc__nmsocket_log(sock, ISC_LOG_ERROR, "uv_listen failed: %s",
				  isc_result_totext(isc_uverr2result(r)));
		isc__nm_incstats(sock, STATID_BINDFAIL);
		goto done;
	}

	if (sock->tid == 0) {
		r = uv_tcp_getsockname(&sock->uv_handle.tcp,
				       (struct sockaddr *)&ss,
				       &(int){ sizeof(ss) });
		if (r != 0) {
			goto done;
		}

		result = isc_sockaddr_fromsockaddr(&sock->parent->iface,
						   (struct sockaddr *)&ss);
		if (result != ISC_R_SUCCESS) {
			goto done_result;
		}
	}

done:
	result = isc_uverr2result(r);

done_result:
	if (result != ISC_R_SUCCESS) {
		sock->pquota = NULL;
	}

	sock->result = result;

	REQUIRE(!loop->paused);

	if (sock->tid != 0) {
		isc_barrier_wait(&sock->parent->listen_barrier);
	}
}

static void
start_tcp_child(isc_nm_t *mgr, isc_sockaddr_t *iface, isc_nmsocket_t *sock,
		uv_os_sock_t fd, int tid) {
	isc_nmsocket_t *csock = &sock->children[tid];
	isc__networker_t *worker = &mgr->workers[tid];

	isc__nmsocket_init(csock, worker, isc_nm_tcpsocket, iface, sock);
	csock->accept_cb = sock->accept_cb;
	csock->accept_cbarg = sock->accept_cbarg;
	csock->backlog = sock->backlog;

	/*
	 * Quota isn't attached, just assigned.
	 */
	csock->pquota = sock->pquota;

	if (mgr->load_balance_sockets) {
		UNUSED(fd);
		csock->fd = isc__nm_tcp_lb_socket(mgr,
						  iface->type.sa.sa_family);
	} else {
		csock->fd = dup(fd);
	}
	REQUIRE(csock->fd >= 0);

	if (tid == 0) {
		start_tcp_child_job(csock);
	} else {
		isc_async_run(worker->loop, start_tcp_child_job, csock);
	}
}

isc_result_t
isc_nm_listentcp(isc_nm_t *mgr, uint32_t workers, isc_sockaddr_t *iface,
		 isc_nm_accept_cb_t accept_cb, void *accept_cbarg, int backlog,
		 isc_quota_t *quota, isc_nmsocket_t **sockp) {
	isc_nmsocket_t *sock = NULL;
	uv_os_sock_t fd = -1;
	isc_result_t result = ISC_R_UNSET;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(isc_tid() == 0);

	if (workers == 0) {
		workers = mgr->nloops;
	}
	REQUIRE(workers <= mgr->nloops);

	worker = &mgr->workers[0];
	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, isc_nm_tcplistener, iface, NULL);

	sock->nchildren = (workers == ISC_NM_LISTEN_ALL) ? (uint32_t)mgr->nloops
							 : workers;
	sock->children = isc_mem_cget(worker->mctx, sock->nchildren,
				      sizeof(sock->children[0]));

	isc__nmsocket_barrier_init(sock);

	sock->accept_cb = accept_cb;
	sock->accept_cbarg = accept_cbarg;
	sock->backlog = backlog;
	sock->pquota = quota;

	if (!mgr->load_balance_sockets) {
		fd = isc__nm_tcp_lb_socket(mgr, iface->type.sa.sa_family);
	}

	start_tcp_child(mgr, iface, sock, fd, 0);
	result = sock->children[0].result;
	INSIST(result != ISC_R_UNSET);

	for (size_t i = 1; i < sock->nchildren; i++) {
		start_tcp_child(mgr, iface, sock, fd, i);
	}

	isc_barrier_wait(&sock->listen_barrier);

	if (!mgr->load_balance_sockets) {
		isc__nm_closesocket(fd);
	}

	/*
	 * If any of the child sockets have failed then isc_nm_listentcp
	 * fails.
	 */
	for (size_t i = 1; i < sock->nchildren; i++) {
		if (result == ISC_R_SUCCESS &&
		    sock->children[i].result != ISC_R_SUCCESS)
		{
			result = sock->children[i].result;
		}
	}

	if (result != ISC_R_SUCCESS) {
		sock->active = false;
		isc__nm_tcp_stoplistening(sock);
		isc_nmsocket_close(&sock);

		return result;
	}

	sock->active = true;

	*sockp = sock;
	return ISC_R_SUCCESS;
}

static void
tcp_connection_cb(uv_stream_t *server, int status) {
	isc_nmsocket_t *ssock = uv_handle_get_data((uv_handle_t *)server);
	isc_result_t result;

	REQUIRE(ssock->accept_cb != NULL);

	if (status != 0) {
		result = isc_uverr2result(status);
		goto done;
	}

	REQUIRE(VALID_NMSOCK(ssock));
	REQUIRE(ssock->tid == isc_tid());

	if (isc__nmsocket_closing(ssock)) {
		result = ISC_R_CANCELED;
		goto done;
	}

	/* Prepare the child socket */
	isc_nmsocket_t *csock = isc_mempool_get(ssock->worker->nmsocket_pool);
	isc__nmsocket_init(csock, ssock->worker, isc_nm_tcpsocket,
			   &ssock->iface, NULL);
	isc__nmsocket_attach(ssock, &csock->server);

	if (csock->server->pquota != NULL) {
		result = isc_quota_acquire_cb(csock->server->pquota,
					      &csock->quotacb, quota_accept_cb,
					      csock);
		if (result == ISC_R_QUOTA) {
			csock->quota_accept_ts = isc_time_monotonic();
			isc__nm_incstats(ssock, STATID_ACCEPTFAIL);
			goto done;
		}
	}

	result = accept_connection(csock);
done:
	isc__nm_accept_connection_log(ssock, result, can_log_tcp_quota());
}

static void
stop_tcp_child_job(void *arg) {
	isc_nmsocket_t *sock = arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->parent != NULL);
	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(!sock->closing);

	sock->active = false;
	sock->closing = true;

	/*
	 * The order of the close operation is important here, the uv_close()
	 * gets scheduled in the reverse order, so we need to close the timer
	 * last, so its gone by the time we destroy the socket
	 */

	/* 2. close the listening socket */
	isc__nmsocket_clearcb(sock);
	isc__nm_stop_reading(sock);
	uv_close(&sock->uv_handle.handle, tcp_stop_cb);

	/* 1. close the read timer */
	isc__nmsocket_timer_stop(sock);
	uv_close(&sock->read_timer, NULL);

	REQUIRE(!sock->worker->loop->paused);
	isc_barrier_wait(&sock->parent->stop_barrier);
}

static void
stop_tcp_child(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (sock->tid == 0) {
		stop_tcp_child_job(sock);
	} else {
		isc_async_run(sock->worker->loop, stop_tcp_child_job, sock);
	}
}

void
isc__nm_tcp_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcplistener);
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->tid == 0);
	REQUIRE(!sock->closing);

	sock->closing = true;

	/* Mark the parent socket inactive */
	sock->active = false;

	/* Stop all the other threads' children */
	for (size_t i = 1; i < sock->nchildren; i++) {
		stop_tcp_child(&sock->children[i]);
	}

	/* Stop the child for the main thread */
	stop_tcp_child(&sock->children[0]);

	/* Stop the parent */
	sock->closed = true;

	isc__nmsocket_prep_destroy(sock);
}

static void
tcp_stop_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->closing);
	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(!sock->closed);

	sock->closed = true;

	isc__nm_incstats(sock, STATID_CLOSE);

	isc__nmsocket_detach(&sock);
}

void
isc__nm_tcp_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			   bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);
	sock->reading = false;

	if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
		isc__nmsocket_clearcb(sock);
		isc__nm_readcb(sock, req, result, async);
	}

	isc__nmsocket_prep_destroy(sock);
}

void
isc__nm_tcp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock;
	isc_nm_t *netmgr;
	isc_result_t result;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;
	netmgr = sock->worker->netmgr;

	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(sock->statichandle == handle);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;

	/* Initialize the timer */
	if (sock->read_timeout == 0) {
		sock->read_timeout =
			sock->keepalive
				? atomic_load_relaxed(&netmgr->keepalive)
				: atomic_load_relaxed(&netmgr->idle);
	}

	if (isc__nmsocket_closing(sock)) {
		result = ISC_R_CANCELED;
		goto failure;
	}

	if (!sock->reading_throttled) {
		result = isc__nm_start_reading(sock);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
	}

	sock->reading = true;

	if (!sock->manual_read_timer) {
		isc__nmsocket_timer_start(sock);
	}

	return;
failure:
	isc__nm_tcp_failed_read_cb(sock, result, true);
}

void
isc__nm_tcp_read_stop(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;

	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);
	sock->reading = false;

	return;
}

void
isc__nm_tcp_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)stream);
	isc__nm_uvreq_t *req = NULL;
	isc_nm_t *netmgr = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(buf != NULL);

	netmgr = sock->worker->netmgr;

	if (isc__nmsocket_closing(sock)) {
		isc__nm_tcp_failed_read_cb(sock, ISC_R_CANCELED, false);
		goto free;
	}

	if (nread < 0) {
		if (nread != UV_EOF) {
			isc__nm_incstats(sock, STATID_RECVFAIL);
		}

		isc__nm_tcp_failed_read_cb(sock, isc_uverr2result(nread),
					   false);

		goto free;
	}

	req = isc__nm_get_read_req(sock, NULL);

	/*
	 * The callback will be called synchronously because the
	 * result is ISC_R_SUCCESS, so we don't need to retain
	 * the buffer
	 */
	req->uvbuf.base = buf->base;
	req->uvbuf.len = nread;

	if (!sock->client) {
		sock->read_timeout =
			sock->keepalive
				? atomic_load_relaxed(&netmgr->keepalive)
				: atomic_load_relaxed(&netmgr->idle);
	}

	isc__nm_readcb(sock, req, ISC_R_SUCCESS, false);

	if (!sock->client && sock->reading) {
		/*
		 * Stop reading if we have accumulated enough bytes in the send
		 * queue; this means that the TCP client is not reading back the
		 * data we sending to it, and there's no reason to continue
		 * processing more incoming DNS messages, if the client is not
		 * reading back the responses.
		 */
		size_t write_queue_size =
			uv_stream_get_write_queue_size(&sock->uv_handle.stream);

		if (write_queue_size >= ISC_NETMGR_TCP_SENDBUF_SIZE) {
			isc__nmsocket_log(
				sock, ISC_LOG_DEBUG(3),
				"throttling TCP connection, the other side is "
				"not reading the data (%zu)",
				write_queue_size);
			sock->reading_throttled = true;
			isc__nm_stop_reading(sock);
		}
	} else if (uv_is_active(&sock->uv_handle.handle) &&
		   !sock->manual_read_timer)
	{
		/* The readcb could have paused the reading */
		/* The timer will be updated */
		isc__nmsocket_timer_restart(sock);
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

/*
 * This is called after we get a quota_accept_cb() callback.
 */
static void
tcpaccept_cb(void *arg) {
	isc_nmsocket_t *csock = arg;
	isc_nmsocket_t *ssock = csock->server;

	REQUIRE(VALID_NMSOCK(csock));
	REQUIRE(csock->tid == isc_tid());

	isc_result_t result = accept_connection(csock);
	isc__nm_accept_connection_log(ssock, result, can_log_tcp_quota());
	isc__nmsocket_detach(&csock);
}

static void
quota_accept_cb(void *arg) {
	isc_nmsocket_t *csock = arg;
	isc_nmsocket_t *ssock = csock->server;

	REQUIRE(VALID_NMSOCK(csock));

	/*
	 * This needs to be asynchronous, because the quota might have been
	 * released by a different child socket.
	 */
	if (csock->tid == isc_tid()) {
		isc_result_t result = accept_connection(csock);
		isc__nm_accept_connection_log(ssock, result,
					      can_log_tcp_quota());
	} else {
		isc__nmsocket_attach(csock, &(isc_nmsocket_t *){ NULL });
		isc_async_run(csock->worker->loop, tcpaccept_cb, csock);
	}
}

static isc_result_t
accept_connection(isc_nmsocket_t *csock) {
	int r;
	isc_result_t result;
	struct sockaddr_storage ss;
	isc_sockaddr_t local;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(VALID_NMSOCK(csock));
	REQUIRE(VALID_NMSOCK(csock->server));
	REQUIRE(csock->tid == isc_tid());

	csock->accepting = true;
	csock->accept_cb = csock->server->accept_cb;
	csock->accept_cbarg = csock->server->accept_cbarg;
	csock->recv_cb = csock->server->recv_cb;
	csock->recv_cbarg = csock->server->recv_cbarg;
	csock->read_timeout = atomic_load_relaxed(&csock->worker->netmgr->init);

	r = uv_tcp_init(&csock->worker->loop->loop, &csock->uv_handle.tcp);
	UV_RUNTIME_CHECK(uv_tcp_init, r);
	uv_handle_set_data(&csock->uv_handle.handle, csock);

	r = uv_timer_init(&csock->worker->loop->loop, &csock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&csock->read_timer, csock);

	if (csock->server->pquota != NULL) {
		isc__nm_incstats(csock, STATID_CLIENTS);
	}

	/*
	 * We need to initialize the tcp and timer before failing because
	 * isc__nm_tcp_close() can't handle uninitalized TCP nmsocket.
	 */
	if (isc__nmsocket_closing(csock)) {
		result = ISC_R_CANCELED;
		goto failure;
	}

	r = uv_accept(&csock->server->uv_handle.stream,
		      &csock->uv_handle.stream);
	if (r != 0) {
		result = isc_uverr2result(r);
		goto failure;
	}

	/* Check if the connection is not expired */
	if (csock->quota_accept_ts != 0) {
		/* The timestamp is given in nanoseconds */
		const uint64_t time_elapsed_ms =
			(isc_time_monotonic() - csock->quota_accept_ts) /
			NS_PER_MS;

		if (time_elapsed_ms >= csock->read_timeout) {
			/*
			 * At this point we have received a connection from a
			 * queue of accepted connections (via uv_accept()), but
			 * it has expired. We cannot do anything better than
			 * drop it on the floor at this point.
			 */
			result = ISC_R_TIMEDOUT;
			goto failure;
		} else {
			/* Adjust the initial read timeout accordingly */
			csock->read_timeout -= time_elapsed_ms;
		}
	}

	r = uv_tcp_getpeername(&csock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc_uverr2result(r);
		goto failure;
	}

	result = isc_sockaddr_fromsockaddr(&csock->peer,
					   (struct sockaddr *)&ss);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	r = uv_tcp_getsockname(&csock->uv_handle.tcp, (struct sockaddr *)&ss,
			       &(int){ sizeof(ss) });
	if (r != 0) {
		result = isc_uverr2result(r);
		goto failure;
	}

	result = isc_sockaddr_fromsockaddr(&local, (struct sockaddr *)&ss);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	handle = isc__nmhandle_get(csock, NULL, &local);

	result = csock->accept_cb(handle, ISC_R_SUCCESS, csock->accept_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc_nmhandle_detach(&handle);
		goto failure;
	}

	csock->accepting = false;

	isc__nm_incstats(csock, STATID_ACCEPT);

	/*
	 * The acceptcb needs to attach to the handle if it wants to keep the
	 * connection alive
	 */
	isc_nmhandle_detach(&handle);

	/*
	 * sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&csock);

	return ISC_R_SUCCESS;

failure:
	csock->active = false;
	csock->accepting = false;

	if (result != ISC_R_NOTCONNECTED) {
		/* IGNORE: The client disconnected before we could accept */
		isc__nmsocket_log(csock, ISC_LOG_ERROR,
				  "Accepting TCP connection failed: %s",
				  isc_result_totext(result));
	}

	isc__nmsocket_prep_destroy(csock);

	isc__nmsocket_detach(&csock);

	return result;
}

static void
tcp_send(isc_nmhandle_t *handle, const isc_region_t *region, isc_nm_cb_t cb,
	 void *cbarg, const bool dnsmsg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;
	isc_result_t result;
	isc__nm_uvreq_t *uvreq = NULL;
	isc_nm_t *netmgr = sock->worker->netmgr;

	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(sock->tid == isc_tid());

	uvreq = isc__nm_uvreq_get(sock);
	if (dnsmsg) {
		*(uint16_t *)uvreq->tcplen = htons(region->length);
	}
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	isc_nmhandle_attach(handle, &uvreq->handle);

	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	if (sock->write_timeout == 0) {
		sock->write_timeout =
			sock->keepalive
				? atomic_load_relaxed(&netmgr->keepalive)
				: atomic_load_relaxed(&netmgr->idle);
	}

	result = tcp_send_direct(sock, uvreq);
	if (result != ISC_R_SUCCESS) {
		isc__nm_incstats(sock, STATID_SENDFAIL);
		isc__nm_failed_send_cb(sock, uvreq, result, true);
	}

	return;
}

void
isc__nm_tcp_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg) {
	tcp_send(handle, region, cb, cbarg, false);
}

void
isc__nm_tcp_senddns(isc_nmhandle_t *handle, const isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg) {
	tcp_send(handle, region, cb, cbarg, true);
}

static void
tcp_maybe_restart_reading(isc_nmsocket_t *sock) {
	if (!sock->client && sock->reading &&
	    !uv_is_active(&sock->uv_handle.handle))
	{
		/*
		 * Restart reading if we have less data in the send queue than
		 * the send buffer size, this means that the TCP client has
		 * started reading some data again.  Starting reading when we go
		 * under the limit instead of waiting for all data has been
		 * flushed allows faster recovery (in case there was a
		 * congestion and now there isn't).
		 */
		size_t write_queue_size =
			uv_stream_get_write_queue_size(&sock->uv_handle.stream);
		if (write_queue_size < ISC_NETMGR_TCP_SENDBUF_SIZE) {
			isc__nmsocket_log(
				sock, ISC_LOG_DEBUG(3),
				"resuming TCP connection, the other side  "
				"is reading the data again (%zu)",
				write_queue_size);
			isc__nm_start_reading(sock);
			sock->reading_throttled = false;
		}
	}
}

static void
tcp_send_cb(uv_write_t *req, int status) {
	isc__nm_uvreq_t *uvreq = (isc__nm_uvreq_t *)req->data;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMSOCK(uvreq->sock));

	sock = uvreq->sock;

	isc_nm_timer_stop(uvreq->timer);
	isc_nm_timer_detach(&uvreq->timer);

	if (status < 0) {
		isc__nm_incstats(sock, STATID_SENDFAIL);
		isc__nm_failed_send_cb(sock, uvreq, isc_uverr2result(status),
				       false);
		if (!sock->client && sock->reading) {
			/*
			 * As we are resuming reading, it is not throttled
			 * anymore (technically).
			 */
			sock->reading_throttled = false;
			isc__nm_start_reading(sock);
			isc__nmsocket_reset(sock);
		}
		return;
	}

	isc__nm_sendcb(sock, uvreq, ISC_R_SUCCESS, false);
	tcp_maybe_restart_reading(sock);
}

static isc_result_t
tcp_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->type == isc_nm_tcpsocket);

	int r;
	uv_buf_t bufs[2] = { { 0 }, { 0 } }; /* ugly, but required for old GCC
						versions */
	size_t nbufs = 1;

	if (isc__nmsocket_closing(sock)) {
		return ISC_R_CANCELED;
	}

	/* Check if we are not trying to send a DNS message */
	if (*(uint16_t *)req->tcplen == 0) {
		bufs[0].base = req->uvbuf.base;
		bufs[0].len = req->uvbuf.len;

		r = uv_try_write(&sock->uv_handle.stream, bufs, nbufs);

		if (r == (int)(bufs[0].len)) {
			/* Wrote everything */
			isc__nm_sendcb(sock, req, ISC_R_SUCCESS, true);
			tcp_maybe_restart_reading(sock);
			return ISC_R_SUCCESS;
		} else if (r > 0) {
			bufs[0].base += (size_t)r;
			bufs[0].len -= (size_t)r;
		} else if (!(r == UV_ENOSYS || r == UV_EAGAIN)) {
			return isc_uverr2result(r);
		}
	} else {
		nbufs = 2;
		bufs[0].base = req->tcplen;
		bufs[0].len = 2;
		bufs[1].base = req->uvbuf.base;
		bufs[1].len = req->uvbuf.len;

		r = uv_try_write(&sock->uv_handle.stream, bufs, nbufs);

		if (r == (int)(bufs[0].len + bufs[1].len)) {
			/* Wrote everything */
			isc__nm_sendcb(sock, req, ISC_R_SUCCESS, true);
			tcp_maybe_restart_reading(sock);
			return ISC_R_SUCCESS;
		} else if (r == 1) {
			/* Partial write of DNSMSG length */
			bufs[0].base = req->tcplen + 1;
			bufs[0].len = 1;
		} else if (r > 0) {
			/* Partial write of DNSMSG */
			nbufs = 1;
			bufs[0].base = req->uvbuf.base + (r - 2);
			bufs[0].len = req->uvbuf.len - (r - 2);
		} else if (!(r == UV_ENOSYS || r == UV_EAGAIN)) {
			return isc_uverr2result(r);
		}
	}

	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_DEBUG(3),
		      "throttling TCP connection, the other side is not "
		      "reading the data, switching to uv_write()");
	sock->reading_throttled = true;
	isc__nm_stop_reading(sock);

	r = uv_write(&req->uv_req.write, &sock->uv_handle.stream, bufs, nbufs,
		     tcp_send_cb);
	if (r < 0) {
		return isc_uverr2result(r);
	}

	isc_nm_timer_create(req->handle, isc__nmsocket_writetimeout_cb, req,
			    &req->timer);
	if (sock->write_timeout > 0) {
		isc_nm_timer_start(req->timer, sock->write_timeout);
	}

	return ISC_R_SUCCESS;
}

static void
tcp_close_sock(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->closing);
	REQUIRE(!sock->closed);

	sock->closed = true;
	sock->connected = false;

	isc__nm_incstats(sock, STATID_CLOSE);

	if (sock->server != NULL) {
		if (sock->server->pquota != NULL) {
			isc__nm_decstats(sock, STATID_CLIENTS);
			isc_quota_release(sock->server->pquota);
		}
		isc__nmsocket_detach(&sock->server);
	}

	isc__nmsocket_prep_destroy(sock);
}

static void
tcp_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	tcp_close_sock(sock);
}

void
isc__nm_tcp_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(!isc__nmsocket_active(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->parent == NULL);
	REQUIRE(!sock->closing);

	sock->closing = true;

	/*
	 * The order of the close operation is important here, the uv_close()
	 * gets scheduled in the reverse order, so we need to close the timer
	 * last, so its gone by the time we destroy the socket
	 */

	if (!uv_is_closing(&sock->uv_handle.handle)) {
		/* Normal order of operation */

		/* 2. close the socket + destroy the socket in callback */
		isc__nmsocket_clearcb(sock);
		isc__nm_stop_reading(sock);
		sock->reading = false;
		uv_close(&sock->uv_handle.handle, tcp_close_cb);

		/* 1. close the timer */
		isc__nmsocket_timer_stop(sock);
		uv_close((uv_handle_t *)&sock->read_timer, NULL);
	} else {
		/* The socket was already closed elsewhere */

		/* 1. close the timer + destroy the socket in callback */
		isc__nmsocket_timer_stop(sock);
		uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);
		uv_close((uv_handle_t *)&sock->read_timer, tcp_close_cb);
	}
}

static void
tcp_close_connect_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);

	REQUIRE(VALID_NMSOCK(sock));

	REQUIRE(sock->tid == isc_tid());

	isc__nmsocket_prep_destroy(sock);
	isc__nmsocket_detach(&sock);
}

void
isc__nm_tcp_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->type == isc_nm_tcpsocket);

	/*
	 * If the socket is active, mark it inactive and
	 * continue. If it isn't active, stop now.
	 */
	if (!sock->active) {
		return;
	}
	sock->active = false;

	INSIST(!sock->accepting);

	if (sock->connecting) {
		isc_nmsocket_t *tsock = NULL;
		isc__nmsocket_attach(sock, &tsock);
		uv_close(&sock->uv_handle.handle, tcp_close_connect_cb);
		return;
	}

	/* There's a handle attached to the socket (from accept or connect) */
	if (sock->statichandle) {
		isc__nm_failed_read_cb(sock, ISC_R_SHUTTINGDOWN, false);
		return;
	}

	/* Destroy the non-listening socket */
	if (sock->parent == NULL) {
		isc__nmsocket_prep_destroy(sock);
		return;
	}

	/* Destroy the listening socket if on the same loop */
	if (sock->tid == sock->parent->tid) {
		isc__nmsocket_prep_destroy(sock->parent);
	}
}

void
isc__nmhandle_tcp_set_manual_timer(isc_nmhandle_t *handle, const bool manual) {
	isc_nmsocket_t *sock;

	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpsocket);
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(!uv_is_active(&sock->uv_handle.handle));

	sock->manual_read_timer = manual;
}
