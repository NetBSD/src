/*	$NetBSD: udp.c,v 1.11 2023/01/25 21:43:31 christos Exp $	*/

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

#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/errno.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr-int.h"
#include "uv-compat.h"

static isc_result_t
udp_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
		isc_sockaddr_t *peer);

static void
udp_recv_cb(uv_udp_t *handle, ssize_t nrecv, const uv_buf_t *buf,
	    const struct sockaddr *addr, unsigned flags);

static void
udp_send_cb(uv_udp_send_t *req, int status);

static void
udp_close_cb(uv_handle_t *handle);

static void
read_timer_close_cb(uv_handle_t *handle);

static void
udp_close_direct(isc_nmsocket_t *sock);

static void
stop_udp_parent(isc_nmsocket_t *sock);
static void
stop_udp_child(isc_nmsocket_t *sock);

static uv_os_sock_t
isc__nm_udp_lb_socket(isc_nm_t *mgr, sa_family_t sa_family) {
	isc_result_t result;
	uv_os_sock_t sock;

	result = isc__nm_socket(sa_family, SOCK_DGRAM, 0, &sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	(void)isc__nm_socket_incoming_cpu(sock);
	(void)isc__nm_socket_disable_pmtud(sock, sa_family);

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
start_udp_child(isc_nm_t *mgr, isc_sockaddr_t *iface, isc_nmsocket_t *sock,
		uv_os_sock_t fd, int tid) {
	isc_nmsocket_t *csock;
	isc__netievent_udplisten_t *ievent = NULL;

	csock = &sock->children[tid];

	isc__nmsocket_init(csock, mgr, isc_nm_udpsocket, iface);
	csock->parent = sock;
	csock->iface = sock->iface;
	csock->reading = true;
	csock->recv_cb = sock->recv_cb;
	csock->recv_cbarg = sock->recv_cbarg;
	csock->extrahandlesize = sock->extrahandlesize;
	csock->tid = tid;

#ifdef _WIN32
	UNUSED(fd);
	csock->fd = isc__nm_udp_lb_socket(mgr, iface->type.sa.sa_family);
#else
	if (mgr->load_balance_sockets) {
		UNUSED(fd);
		csock->fd = isc__nm_udp_lb_socket(mgr,
						  iface->type.sa.sa_family);
	} else {
		csock->fd = dup(fd);
	}
#endif
	REQUIRE(csock->fd >= 0);

	ievent = isc__nm_get_netievent_udplisten(mgr, csock);
	isc__nm_maybe_enqueue_ievent(&mgr->workers[tid],
				     (isc__netievent_t *)ievent);
}

static void
enqueue_stoplistening(isc_nmsocket_t *sock) {
	isc__netievent_udpstop_t *ievent =
		isc__nm_get_netievent_udpstop(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

isc_result_t
isc_nm_listenudp(isc_nm_t *mgr, isc_sockaddr_t *iface, isc_nm_recv_cb_t cb,
		 void *cbarg, size_t extrahandlesize, isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	size_t children_size = 0;
	REQUIRE(VALID_NM(mgr));
	uv_os_sock_t fd = -1;

	/*
	 * We are creating mgr->nworkers duplicated sockets, one
	 * socket for each worker thread.
	 */
	sock = isc_mem_get(mgr->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(sock, mgr, isc_nm_udplistener, iface);

	atomic_init(&sock->rchildren, 0);
#if defined(WIN32)
	sock->nchildren = 1;
#else
	sock->nchildren = mgr->nworkers;
#endif

	children_size = sock->nchildren * sizeof(sock->children[0]);
	sock->children = isc_mem_get(mgr->mctx, children_size);
	memset(sock->children, 0, children_size);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->extrahandlesize = extrahandlesize;
	sock->result = ISC_R_UNSET;

	sock->tid = 0;
	sock->fd = -1;

#ifndef _WIN32
	if (!mgr->load_balance_sockets) {
		fd = isc__nm_udp_lb_socket(mgr, iface->type.sa.sa_family);
	}
#endif

	isc_barrier_init(&sock->startlistening, sock->nchildren);

	for (size_t i = 0; i < sock->nchildren; i++) {
		if ((int)i == isc_nm_tid()) {
			continue;
		}
		start_udp_child(mgr, iface, sock, fd, i);
	}

	if (isc__nm_in_netthread()) {
		start_udp_child(mgr, iface, sock, fd, isc_nm_tid());
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

/*
 * Asynchronous 'udplisten' call handler: start listening on a UDP socket.
 */
void
isc__nm_async_udplisten(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_udplisten_t *ievent = (isc__netievent_udplisten_t *)ev0;
	isc_nmsocket_t *sock = NULL;
	int r, uv_bind_flags = 0;
	int uv_init_flags = 0;
	sa_family_t sa_family;
	isc_result_t result = ISC_R_UNSET;
	isc_nm_t *mgr = NULL;

	REQUIRE(VALID_NMSOCK(ievent->sock));
	REQUIRE(ievent->sock->tid == isc_nm_tid());
	REQUIRE(VALID_NMSOCK(ievent->sock->parent));

	sock = ievent->sock;
	sa_family = sock->iface.type.sa.sa_family;
	mgr = sock->mgr;

	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->parent != NULL);
	REQUIRE(sock->tid == isc_nm_tid());

#if HAVE_DECL_UV_UDP_RECVMMSG
	uv_init_flags |= UV_UDP_RECVMMSG;
#endif
	r = uv_udp_init_ex(&worker->loop, &sock->uv_handle.udp, uv_init_flags);
	UV_RUNTIME_CHECK(uv_udp_init_ex, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	/* This keeps the socket alive after everything else is gone */
	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });

	r = uv_timer_init(&worker->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	LOCK(&sock->parent->lock);

	r = uv_udp_open(&sock->uv_handle.udp, sock->fd);
	if (r < 0) {
		isc__nm_closesocket(sock->fd);
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPENFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPEN]);

	if (sa_family == AF_INET6) {
		uv_bind_flags |= UV_UDP_IPV6ONLY;
	}

#ifdef _WIN32
	r = isc_uv_udp_freebind(&sock->uv_handle.udp,
				&sock->parent->iface.type.sa, uv_bind_flags);
	if (r < 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		goto done;
	}
#else
	if (mgr->load_balance_sockets) {
		r = isc_uv_udp_freebind(&sock->uv_handle.udp,
					&sock->parent->iface.type.sa,
					uv_bind_flags);
		if (r < 0) {
			isc__nm_incstats(sock->mgr,
					 sock->statsindex[STATID_BINDFAIL]);
			goto done;
		}
	} else {
		if (sock->parent->fd == -1) {
			/* This thread is first, bind the socket */
			r = isc_uv_udp_freebind(&sock->uv_handle.udp,
						&sock->parent->iface.type.sa,
						uv_bind_flags);
			if (r < 0) {
				isc__nm_incstats(sock->mgr, STATID_BINDFAIL);
				goto done;
			}
			sock->parent->uv_handle.udp.flags =
				sock->uv_handle.udp.flags;
			sock->parent->fd = sock->fd;
		} else {
			/* The socket is already bound, just copy the flags */
			sock->uv_handle.udp.flags =
				sock->parent->uv_handle.udp.flags;
		}
	}
#endif

#ifdef ISC_RECV_BUFFER_SIZE
	uv_recv_buffer_size(&sock->uv_handle.handle,
			    &(int){ ISC_RECV_BUFFER_SIZE });
#endif
#ifdef ISC_SEND_BUFFER_SIZE
	uv_send_buffer_size(&sock->uv_handle.handle,
			    &(int){ ISC_SEND_BUFFER_SIZE });
#endif
	r = uv_udp_recv_start(&sock->uv_handle.udp, isc__nm_alloc_cb,
			      udp_recv_cb);
	if (r != 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		goto done;
	}

	atomic_store(&sock->listening, true);

done:
	result = isc__nm_uverr2result(r);
	atomic_fetch_add(&sock->parent->rchildren, 1);
	if (sock->parent->result == ISC_R_UNSET) {
		sock->parent->result = result;
	}
	SIGNAL(&sock->parent->cond);
	UNLOCK(&sock->parent->lock);

	isc_barrier_wait(&sock->parent->startlistening);
}

void
isc__nm_udp_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udplistener);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		UNREACHABLE();
	}

	if (!isc__nm_in_netthread()) {
		enqueue_stoplistening(sock);
	} else {
		stop_udp_parent(sock);
	}
}

/*
 * Asynchronous 'udpstop' call handler: stop listening on a UDP socket.
 */
void
isc__nm_async_udpstop(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_udpstop_t *ievent = (isc__netievent_udpstop_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->parent != NULL) {
		stop_udp_child(sock);
		return;
	}

	stop_udp_parent(sock);
}

/*
 * udp_recv_cb handles incoming UDP packet from uv.  The buffer here is
 * reused for a series of packets, so we need to allocate a new one.
 * This new one can be reused to send the response then.
 */
static void
udp_recv_cb(uv_udp_t *handle, ssize_t nrecv, const uv_buf_t *buf,
	    const struct sockaddr *addr, unsigned flags) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)handle);
	isc__nm_uvreq_t *req = NULL;
	uint32_t maxudp;
	isc_sockaddr_t sockaddr;
	isc_result_t result;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->reading);

	/*
	 * When using recvmmsg(2), if no errors occur, there will be a final
	 * callback with nrecv set to 0, addr set to NULL and the buffer
	 * pointing at the initially allocated data with the UV_UDP_MMSG_CHUNK
	 * flag cleared and the UV_UDP_MMSG_FREE flag set.
	 */
#if HAVE_DECL_UV_UDP_MMSG_FREE
	if ((flags & UV_UDP_MMSG_FREE) == UV_UDP_MMSG_FREE) {
		INSIST(nrecv == 0);
		INSIST(addr == NULL);
		goto free;
	}
#else
	UNUSED(flags);
#endif

	/*
	 * - If we're simulating a firewall blocking UDP packets
	 *   bigger than 'maxudp' bytes for testing purposes.
	 */
	maxudp = atomic_load(&sock->mgr->maxudp);
	if ((maxudp != 0 && (uint32_t)nrecv > maxudp)) {
		/*
		 * We need to keep the read_cb intact in case, so the
		 * readtimeout_cb can trigger and not crash because of
		 * missing read_req.
		 */
		goto free;
	}

	/*
	 * - If addr == NULL, in which case it's the end of stream;
	 *   we can free the buffer and bail.
	 */
	if (addr == NULL) {
		isc__nm_failed_read_cb(sock, ISC_R_EOF, false);
		goto free;
	}

	/*
	 * - If the socket is no longer active.
	 */
	if (!isc__nmsocket_active(sock)) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED, false);
		goto free;
	}

	if (nrecv < 0) {
		isc__nm_failed_read_cb(sock, isc__nm_uverr2result(nrecv),
				       false);
		goto free;
	}

	result = isc_sockaddr_fromsockaddr(&sockaddr, addr);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	req = isc__nm_get_read_req(sock, &sockaddr);

	/*
	 * The callback will be called synchronously, because result is
	 * ISC_R_SUCCESS, so we are ok of passing the buf directly.
	 */
	req->uvbuf.base = buf->base;
	req->uvbuf.len = nrecv;

	sock->recv_read = false;

	REQUIRE(!sock->processing);
	sock->processing = true;
	isc__nm_readcb(sock, req, ISC_R_SUCCESS);
	sock->processing = false;

free:
#if HAVE_DECL_UV_UDP_MMSG_CHUNK
	/*
	 * When using recvmmsg(2), chunks will have the UV_UDP_MMSG_CHUNK flag
	 * set, those must not be freed.
	 */
	if ((flags & UV_UDP_MMSG_CHUNK) == UV_UDP_MMSG_CHUNK) {
		return;
	}
#endif

	/*
	 * When using recvmmsg(2), if a UDP socket error occurs, nrecv will be <
	 * 0. In either scenario, the callee can now safely free the provided
	 * buffer.
	 */
	if (nrecv < 0) {
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
 * Send the data in 'region' to a peer via a UDP socket. We try to find
 * a proper sibling/child socket so that we won't have to jump to
 * another thread.
 */
void
isc__nm_udp_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = handle->sock;
	isc_nmsocket_t *rsock = NULL;
	isc_sockaddr_t *peer = &handle->peer;
	isc__nm_uvreq_t *uvreq = NULL;
	uint32_t maxudp = atomic_load(&sock->mgr->maxudp);
	int ntid;

	INSIST(sock->type == isc_nm_udpsocket);

	/*
	 * We're simulating a firewall blocking UDP packets bigger than
	 * 'maxudp' bytes, for testing purposes.
	 *
	 * The client would ordinarily have unreferenced the handle
	 * in the callback, but that won't happen in this case, so
	 * we need to do so here.
	 */
	if (maxudp != 0 && region->length > maxudp) {
		isc_nmhandle_detach(&handle);
		return;
	}

	if (atomic_load(&sock->client)) {
		/*
		 * When we are sending from the client socket, we directly use
		 * the socket provided.
		 */
		rsock = sock;
		goto send;
	} else {
		/*
		 * When we are sending from the server socket, we either use the
		 * socket associated with the network thread we are in, or we
		 * use the thread from the socket associated with the handle.
		 */
		INSIST(sock->parent != NULL);

#if defined(WIN32)
		/* On Windows, we have only a single listening listener */
		rsock = sock;
#else
		if (isc__nm_in_netthread()) {
			ntid = isc_nm_tid();
		} else {
			ntid = sock->tid;
		}
		rsock = &sock->parent->children[ntid];
#endif
	}

send:
	uvreq = isc__nm_uvreq_get(rsock->mgr, rsock);
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	isc_nmhandle_attach(handle, &uvreq->handle);

	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	if (isc_nm_tid() == rsock->tid) {
		REQUIRE(rsock->tid == isc_nm_tid());
		isc__netievent_udpsend_t ievent = { .sock = rsock,
						    .req = uvreq,
						    .peer = *peer };

		isc__nm_async_udpsend(NULL, (isc__netievent_t *)&ievent);
	} else {
		isc__netievent_udpsend_t *ievent =
			isc__nm_get_netievent_udpsend(sock->mgr, rsock);
		ievent->peer = *peer;
		ievent->req = uvreq;

		isc__nm_enqueue_ievent(&sock->mgr->workers[rsock->tid],
				       (isc__netievent_t *)ievent);
	}
}

/*
 * Asynchronous 'udpsend' event handler: send a packet on a UDP socket.
 */
void
isc__nm_async_udpsend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc_result_t result;
	isc__netievent_udpsend_t *ievent = (isc__netievent_udpsend_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *uvreq = ievent->req;

	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	if (isc__nmsocket_closing(sock)) {
		isc__nm_failed_send_cb(sock, uvreq, ISC_R_CANCELED);
		return;
	}

	result = udp_send_direct(sock, uvreq, &ievent->peer);
	if (result != ISC_R_SUCCESS) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
		isc__nm_failed_send_cb(sock, uvreq, result);
	}
}

static void
udp_send_cb(uv_udp_send_t *req, int status) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__nm_uvreq_t *uvreq = uv_handle_get_data((uv_handle_t *)req);
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	sock = uvreq->sock;

	REQUIRE(sock->tid == isc_nm_tid());

	if (status < 0) {
		result = isc__nm_uverr2result(status);
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_SENDFAIL]);
	}

	isc__nm_sendcb(sock, uvreq, result, false);
}

/*
 * udp_send_direct sends buf to a peer on a socket. Sock has to be in
 * the same thread as the callee.
 */
static isc_result_t
udp_send_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
		isc_sockaddr_t *peer) {
	const struct sockaddr *sa = &peer->type.sa;
	int r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_udpsocket);

	if (isc__nmsocket_closing(sock)) {
		return (ISC_R_CANCELED);
	}

#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
	/*
	 * If we used uv_udp_connect() (and not the shim version for
	 * older versions of libuv), then the peer address has to be
	 * set to NULL or else uv_udp_send() could fail or assert,
	 * depending on the libuv version.
	 */
	if (atomic_load(&sock->connected)) {
		sa = NULL;
	}
#endif

	r = uv_udp_send(&req->uv_req.udp_send, &sock->uv_handle.udp,
			&req->uvbuf, 1, sa, udp_send_cb);
	if (r < 0) {
		return (isc__nm_uverr2result(r));
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
udp_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	isc__networker_t *worker = NULL;
	int uv_bind_flags = UV_UDP_REUSEADDR;
	isc_result_t result = ISC_R_UNSET;
	int tries = 3;
	int r;

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(sock->tid == isc_nm_tid());

	worker = &sock->mgr->workers[isc_nm_tid()];

	atomic_store(&sock->connecting, true);

	r = uv_udp_init(&worker->loop, &sock->uv_handle.udp);
	UV_RUNTIME_CHECK(uv_udp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	r = uv_udp_open(&sock->uv_handle.udp, sock->fd);
	if (r != 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPENFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_OPEN]);

	if (sock->iface.type.sa.sa_family == AF_INET6) {
		uv_bind_flags |= UV_UDP_IPV6ONLY;
	}

	r = uv_udp_bind(&sock->uv_handle.udp, &sock->iface.type.sa,
			uv_bind_flags);
	if (r != 0) {
		isc__nm_incstats(sock->mgr, sock->statsindex[STATID_BINDFAIL]);
		goto done;
	}

#ifdef ISC_RECV_BUFFER_SIZE
	uv_recv_buffer_size(&sock->uv_handle.handle,
			    &(int){ ISC_RECV_BUFFER_SIZE });
#endif
#ifdef ISC_SEND_BUFFER_SIZE
	uv_send_buffer_size(&sock->uv_handle.handle,
			    &(int){ ISC_SEND_BUFFER_SIZE });
#endif

	/*
	 * On FreeBSD the UDP connect() call sometimes results in a
	 * spurious transient EADDRINUSE. Try a few more times before
	 * giving up.
	 */
	do {
		r = isc_uv_udp_connect(&sock->uv_handle.udp,
				       &req->peer.type.sa);
	} while (r == UV_EADDRINUSE && --tries > 0);
	if (r != 0) {
		isc__nm_incstats(sock->mgr,
				 sock->statsindex[STATID_CONNECTFAIL]);
		goto done;
	}
	isc__nm_incstats(sock->mgr, sock->statsindex[STATID_CONNECT]);

	atomic_store(&sock->connecting, false);
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

/*
 * Asynchronous 'udpconnect' call handler: open a new UDP socket and
 * call the 'open' callback with a handle.
 */
void
isc__nm_async_udpconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_udpconnect_t *ievent =
		(isc__netievent_udpconnect_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	isc_result_t result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->parent == NULL);
	REQUIRE(sock->tid == isc_nm_tid());

	result = udp_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		atomic_store(&sock->active, false);
		isc__nm_udp_close(sock);
		isc__nm_connectcb(sock, req, result, true);
	} else {
		/*
		 * The callback has to be called after the socket has been
		 * initialized
		 */
		isc__nm_connectcb(sock, req, ISC_R_SUCCESS, true);
	}

	/*
	 * The sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&sock);
}

void
isc_nm_udpconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		  isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
		  size_t extrahandlesize) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__netievent_udpconnect_t *event = NULL;
	isc__nm_uvreq_t *req = NULL;
	sa_family_t sa_family;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(local != NULL);
	REQUIRE(peer != NULL);

	sa_family = peer->type.sa.sa_family;

	sock = isc_mem_get(mgr->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(sock, mgr, isc_nm_udpsocket, local);

	sock->connect_cb = cb;
	sock->connect_cbarg = cbarg;
	sock->read_timeout = timeout;
	sock->extrahandlesize = extrahandlesize;
	sock->peer = *peer;
	sock->result = ISC_R_UNSET;
	atomic_init(&sock->client, true);

	req = isc__nm_uvreq_get(mgr, sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->peer = *peer;
	req->local = *local;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface);

	result = isc__nm_socket(sa_family, SOCK_DGRAM, 0, &sock->fd);
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

	result = isc__nm_socket_reuse(sock->fd);
	RUNTIME_CHECK(result == ISC_R_SUCCESS ||
		      result == ISC_R_NOTIMPLEMENTED);

	result = isc__nm_socket_reuse_lb(sock->fd);
	RUNTIME_CHECK(result == ISC_R_SUCCESS ||
		      result == ISC_R_NOTIMPLEMENTED);

	(void)isc__nm_socket_incoming_cpu(sock->fd);

	(void)isc__nm_socket_disable_pmtud(sock->fd, sa_family);

	event = isc__nm_get_netievent_udpconnect(mgr, sock, req);

	if (isc__nm_in_netthread()) {
		atomic_store(&sock->active, true);
		sock->tid = isc_nm_tid();
		isc__nm_async_udpconnect(&mgr->workers[sock->tid],
					 (isc__netievent_t *)event);
		isc__nm_put_netievent_udpconnect(mgr, event);
	} else {
		atomic_init(&sock->active, false);
		sock->tid = isc_random_uniform(mgr->nworkers);
		isc__nm_enqueue_ievent(&mgr->workers[sock->tid],
				       (isc__netievent_t *)event);
	}
	LOCK(&sock->lock);
	while (sock->result == ISC_R_UNSET) {
		WAIT(&sock->cond, &sock->lock);
	}
	atomic_store(&sock->active, true);
	BROADCAST(&sock->scond);
	UNLOCK(&sock->lock);
}

void
isc__nm_udp_read_cb(uv_udp_t *handle, ssize_t nrecv, const uv_buf_t *buf,
		    const struct sockaddr *addr, unsigned flags) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)handle);
	REQUIRE(VALID_NMSOCK(sock));

	udp_recv_cb(handle, nrecv, buf, addr, flags);
	/*
	 * If a caller calls isc_nm_read() on a listening socket, we can
	 * get here, but we MUST NOT stop reading from the listener
	 * socket.  The only difference between listener and connected
	 * sockets is that the former has sock->parent set and later
	 * does not.
	 */
	if (!sock->parent) {
		isc__nm_stop_reading(sock);
	}
}

void
isc__nm_udp_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	if (atomic_load(&sock->client)) {
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
		return;
	}

	/*
	 * For UDP server socket, we don't have child socket via
	 * "accept", so we:
	 * - we continue to read
	 * - we don't clear the callbacks
	 * - we don't destroy it (only stoplistening could do that)
	 */
	if (!sock->recv_read) {
		return;
	}
	sock->recv_read = false;

	if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
		isc__nm_readcb(sock, req, result);
	}
}

/*
 * Asynchronous 'udpread' call handler: start or resume reading on a
 * socket; pause reading and call the 'recv' callback after each
 * datagram.
 */
void
isc__nm_async_udpread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_udpread_t *ievent = (isc__netievent_udpread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_result_t result;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (isc__nmsocket_closing(sock)) {
		result = ISC_R_CANCELED;
	} else {
		result = isc__nm_start_reading(sock);
	}

	if (result != ISC_R_SUCCESS) {
		sock->reading = true;
		isc__nm_failed_read_cb(sock, result, false);
		return;
	}

	isc__nmsocket_timer_start(sock);
}

void
isc__nm_udp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	isc_nmsocket_t *sock = handle->sock;

	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->statichandle == handle);
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(!sock->recv_read);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->recv_read = true;

	if (!sock->reading && sock->tid == isc_nm_tid()) {
		isc__netievent_udpread_t ievent = { .sock = sock };
		isc__nm_async_udpread(NULL, (isc__netievent_t *)&ievent);
	} else {
		isc__netievent_udpread_t *ievent =
			isc__nm_get_netievent_udpread(sock->mgr, sock);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

static void
udp_stop_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->closing));

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
udp_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

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
	atomic_store(&sock->listening, false);

	isc__nmsocket_prep_destroy(sock);
}

static void
read_timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	if (sock->parent) {
		uv_close(&sock->uv_handle.handle, udp_stop_cb);
	} else {
		uv_close(&sock->uv_handle.handle, udp_close_cb);
	}
}

static void
stop_udp_child(isc_nmsocket_t *sock) {
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->tid == isc_nm_tid());

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	udp_close_direct(sock);

	atomic_fetch_sub(&sock->parent->rchildren, 1);

	isc_barrier_wait(&sock->parent->stoplistening);
}

static void
stop_udp_parent(isc_nmsocket_t *sock) {
	isc_nmsocket_t *csock = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_udplistener);

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
	stop_udp_child(csock);

	atomic_store(&sock->closed, true);
	isc__nmsocket_prep_destroy(sock);
}

static void
udp_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);
	uv_close((uv_handle_t *)&sock->read_timer, read_timer_close_cb);
}

void
isc__nm_async_udpclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_udpclose_t *ievent = (isc__netievent_udpclose_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	udp_close_direct(sock);
}

void
isc__nm_udp_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(!isc__nmsocket_active(sock));

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		udp_close_direct(sock);
	} else {
		isc__netievent_udpclose_t *ievent =
			isc__nm_get_netievent_udpclose(sock->mgr, sock);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_udp_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_udpsocket);

	/*
	 * If the socket is active, mark it inactive and
	 * continue. If it isn't active, stop now.
	 */
	if (!isc__nmsocket_deactivate(sock)) {
		return;
	}

	/*
	 * If the socket is connecting, the cancel will happen in the
	 * async_udpconnect() due socket being inactive now.
	 */
	if (atomic_load(&sock->connecting)) {
		return;
	}

	/*
	 * When the client detaches the last handle, the
	 * sock->statichandle would be NULL, in that case, nobody is
	 * interested in the callback.
	 */
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
isc__nm_udp_cancelread(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_udpcancel_t *ievent = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udpsocket);

	ievent = isc__nm_get_netievent_udpcancel(sock->mgr, sock, handle);

	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_udpcancel(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_udpcancel_t *ievent = (isc__netievent_udpcancel_t *)ev0;
	isc_nmsocket_t *sock = NULL;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(ievent->sock));

	sock = ievent->sock;

	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->client));

	isc__nm_failed_read_cb(sock, ISC_R_EOF, false);
}
