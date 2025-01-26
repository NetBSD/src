/*	$NetBSD: udp.c,v 1.15 2025/01/26 16:25:44 christos Exp $	*/

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

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#if defined(RTM_VERSION) && defined(RTM_NEWADDR) && defined(RTM_DELADDR)
#define USE_ROUTE_SOCKET      1
#define ROUTE_SOCKET_PF	      PF_ROUTE
#define ROUTE_SOCKET_PROTOCOL 0
#define MSGHDR		      rt_msghdr
#define MSGTYPE		      rtm_type
#endif /* if defined(RTM_VERSION) && defined(RTM_NEWADDR) && \
	* defined(RTM_DELADDR) */
#endif /* ifdef HAVE_NET_ROUTE_H */

#if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#if defined(RTM_NEWADDR) && defined(RTM_DELADDR)
#define USE_ROUTE_SOCKET      1
#define USE_NETLINK	      1
#define ROUTE_SOCKET_PF	      PF_NETLINK
#define ROUTE_SOCKET_PROTOCOL NETLINK_ROUTE
#define MSGHDR		      nlmsghdr
#define MSGTYPE		      nlmsg_type
#endif /* if defined(RTM_NEWADDR) && defined(RTM_DELADDR) */
#endif /* if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H) \
	*/

static void
udp_send_cb(uv_udp_send_t *req, int status);

static void
udp_close_cb(uv_handle_t *handle);

static uv_os_sock_t
isc__nm_udp_lb_socket(isc_nm_t *mgr, sa_family_t sa_family) {
	isc_result_t result;
	uv_os_sock_t sock = -1;

	result = isc__nm_socket(sa_family, SOCK_DGRAM, 0, &sock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	(void)isc__nm_socket_disable_pmtud(sock, sa_family);
	(void)isc__nm_socket_v6only(sock, sa_family);

	result = isc__nm_socket_reuse(sock, 1);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (mgr->load_balance_sockets) {
		result = isc__nm_socket_reuse_lb(sock);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	return sock;
}

/*
 * Asynchronous 'udplisten' call handler: start listening on a UDP socket.
 */
static void
start_udp_child_job(void *arg) {
	isc_nmsocket_t *sock = arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_NMSOCK(sock->parent));
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->tid == isc_tid());

	int r, uv_bind_flags = 0;
	int uv_init_flags = 0;
	sa_family_t sa_family = sock->iface.type.sa.sa_family;
	isc_result_t result = ISC_R_UNSET;
	isc_nm_t *mgr = sock->worker->netmgr;
	isc_loop_t *loop = sock->worker->loop;

	(void)isc__nm_socket_min_mtu(sock->fd, sa_family);

#if HAVE_DECL_UV_UDP_RECVMMSG
	uv_init_flags |= UV_UDP_RECVMMSG;
#endif
	r = uv_udp_init_ex(&loop->loop, &sock->uv_handle.udp, uv_init_flags);
	UV_RUNTIME_CHECK(uv_udp_init_ex, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);
	/* This keeps the socket alive after everything else is gone */
	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });

	r = uv_timer_init(&loop->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	r = uv_udp_open(&sock->uv_handle.udp, sock->fd);
	if (r < 0) {
		isc__nm_closesocket(sock->fd);
		isc__nm_incstats(sock, STATID_OPENFAIL);
		goto done;
	}
	isc__nm_incstats(sock, STATID_OPEN);

	if (sa_family == AF_INET6) {
		uv_bind_flags |= UV_UDP_IPV6ONLY;
	}

	if (mgr->load_balance_sockets) {
		r = isc__nm_udp_freebind(&sock->uv_handle.udp,
					 &sock->parent->iface.type.sa,
					 uv_bind_flags);
		if (r < 0) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			goto done;
		}
	} else if (sock->tid == 0) {
		/* This thread is first, bind the socket */
		r = isc__nm_udp_freebind(&sock->uv_handle.udp,
					 &sock->parent->iface.type.sa,
					 uv_bind_flags);
		if (r < 0) {
			isc__nm_incstats(sock, STATID_BINDFAIL);
			goto done;
		}
		sock->parent->uv_handle.udp.flags = sock->uv_handle.udp.flags;
	} else {
		/* The socket is already bound, just copy the flags */
		sock->uv_handle.udp.flags = sock->parent->uv_handle.udp.flags;
	}

	isc__nm_set_network_buffers(mgr, &sock->uv_handle.handle);

	r = uv_udp_recv_start(&sock->uv_handle.udp, isc__nm_alloc_cb,
			      isc__nm_udp_read_cb);
	if (r != 0) {
		isc__nm_incstats(sock, STATID_BINDFAIL);
		goto done;
	}

done:
	result = isc_uverr2result(r);

	sock->result = result;

	REQUIRE(!loop->paused);

	if (sock->tid != 0) {
		isc_barrier_wait(&sock->parent->listen_barrier);
	}
}

static void
start_udp_child(isc_nm_t *mgr, isc_sockaddr_t *iface, isc_nmsocket_t *sock,
		uv_os_sock_t fd, int tid) {
	isc__networker_t *worker = &mgr->workers[tid];
	isc_nmsocket_t *csock = &sock->children[tid];

	isc__nmsocket_init(csock, worker, isc_nm_udpsocket, iface, sock);
	csock->recv_cb = sock->recv_cb;
	csock->recv_cbarg = sock->recv_cbarg;
	csock->inactive_handles_max = ISC_NM_NMHANDLES_MAX;

	if (mgr->load_balance_sockets) {
		csock->fd = isc__nm_udp_lb_socket(mgr,
						  iface->type.sa.sa_family);
	} else {
		csock->fd = dup(fd);
	}
	INSIST(csock->fd >= 0);

	if (tid == 0) {
		start_udp_child_job(csock);
	} else {
		isc_async_run(worker->loop, start_udp_child_job, csock);
	}
}

isc_result_t
isc_nm_listenudp(isc_nm_t *mgr, uint32_t workers, isc_sockaddr_t *iface,
		 isc_nm_recv_cb_t cb, void *cbarg, isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_UNSET;
	isc_nmsocket_t *sock = NULL;
	uv_os_sock_t fd = -1;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(isc_tid() == 0);

	worker = &mgr->workers[0];

	if (isc__nm_closing(worker)) {
		return ISC_R_SHUTTINGDOWN;
	}

	if (workers == 0) {
		workers = mgr->nloops;
	}
	REQUIRE(workers <= mgr->nloops);

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, isc_nm_udplistener, iface, NULL);

	sock->nchildren = (workers == ISC_NM_LISTEN_ALL) ? (uint32_t)mgr->nloops
							 : workers;
	sock->children = isc_mem_cget(worker->mctx, sock->nchildren,
				      sizeof(sock->children[0]));

	isc__nmsocket_barrier_init(sock);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;

	if (!mgr->load_balance_sockets) {
		fd = isc__nm_udp_lb_socket(mgr, iface->type.sa.sa_family);
	}

	start_udp_child(mgr, iface, sock, fd, 0);
	result = sock->children[0].result;
	INSIST(result != ISC_R_UNSET);

	for (size_t i = 1; i < sock->nchildren; i++) {
		start_udp_child(mgr, iface, sock, fd, i);
	}

	isc_barrier_wait(&sock->listen_barrier);

	if (!mgr->load_balance_sockets) {
		isc__nm_closesocket(fd);
	}

	/*
	 * If any of the child sockets have failed then isc_nm_listenudp
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
		isc__nm_udp_stoplistening(sock);
		isc_nmsocket_close(&sock);

		return result;
	}

	sock->active = true;

	*sockp = sock;
	return ISC_R_SUCCESS;
}

#ifdef USE_ROUTE_SOCKET
static isc_result_t
route_socket(uv_os_sock_t *fdp) {
	isc_result_t result;
	uv_os_sock_t fd = -1;
#ifdef USE_NETLINK
	struct sockaddr_nl sa;
	int r;
#endif

	result = isc__nm_socket(ROUTE_SOCKET_PF, SOCK_RAW,
				ROUTE_SOCKET_PROTOCOL, &fd);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

#ifdef USE_NETLINK
	sa.nl_family = PF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
	r = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
	if (r < 0) {
		isc__nm_closesocket(fd);
		return isc_errno_toresult(r);
	}
#endif

	*fdp = fd;
	return ISC_R_SUCCESS;
}

static isc_result_t
route_connect_direct(isc_nmsocket_t *sock) {
	isc__networker_t *worker = NULL;
	int r;

	REQUIRE(sock->tid == isc_tid());

	worker = sock->worker;

	sock->connecting = true;

	r = uv_udp_init(&worker->loop->loop, &sock->uv_handle.udp);
	UV_RUNTIME_CHECK(uv_udp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	if (isc__nm_closing(worker)) {
		return ISC_R_SHUTTINGDOWN;
	}

	r = uv_udp_open(&sock->uv_handle.udp, sock->fd);
	if (r != 0) {
		return isc_uverr2result(r);
	}

	isc__nm_set_network_buffers(sock->worker->netmgr,
				    &sock->uv_handle.handle);

	sock->connecting = false;
	sock->connected = true;

	return ISC_R_SUCCESS;
}

#endif /* USE_ROUTE_SOCKET */

isc_result_t
isc_nm_routeconnect(isc_nm_t *mgr, isc_nm_cb_t cb, void *cbarg) {
#ifdef USE_ROUTE_SOCKET
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	isc__nm_uvreq_t *req = NULL;
	isc__networker_t *worker = NULL;
	uv_os_sock_t fd = -1;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(isc_tid() == 0);

	worker = &mgr->workers[isc_tid()];

	if (isc__nm_closing(worker)) {
		return ISC_R_SHUTTINGDOWN;
	}

	result = route_socket(&fd);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, isc_nm_udpsocket, NULL, NULL);

	sock->connect_cb = cb;
	sock->connect_cbarg = cbarg;
	sock->client = true;
	sock->route_sock = true;
	sock->fd = fd;

	req = isc__nm_uvreq_get(sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->handle = isc__nmhandle_get(sock, NULL, NULL);

	sock->active = true;

	result = route_connect_direct(sock);
	if (result != ISC_R_SUCCESS) {
		sock->active = false;
		isc__nm_udp_close(sock);
	}

	isc__nm_connectcb(sock, req, result, true);

	isc__nmsocket_detach(&sock);

	return ISC_R_SUCCESS;
#else  /* USE_ROUTE_SOCKET */
	UNUSED(mgr);
	UNUSED(cb);
	UNUSED(cbarg);
	UNUSED(extrahandlesize);
	return ISC_R_NOTIMPLEMENTED;
#endif /* USE_ROUTE_SOCKET */
}

/*
 * Asynchronous 'udpstop' call handler: stop listening on a UDP socket.
 */
static void
stop_udp_child_job(void *arg) {
	isc_nmsocket_t *sock = arg;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->parent != NULL);

	sock->active = false;

	isc__nm_udp_close(sock);

	REQUIRE(!sock->worker->loop->paused);
	isc_barrier_wait(&sock->parent->stop_barrier);
}

static void
stop_udp_child(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (sock->tid == 0) {
		stop_udp_child_job(sock);
	} else {
		isc_async_run(sock->worker->loop, stop_udp_child_job, sock);
	}
}

void
isc__nm_udp_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udplistener);
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->tid == 0);
	REQUIRE(!sock->closing);

	sock->closing = true;

	/* Mark the parent socket inactive */
	sock->active = false;

	/* Stop all the other threads' children */
	for (size_t i = 1; i < sock->nchildren; i++) {
		stop_udp_child(&sock->children[i]);
	}

	/* Stop the child for the main thread */
	stop_udp_child(&sock->children[0]);

	/* Stop the parent */
	sock->closed = true;
	isc__nmsocket_prep_destroy(sock);
}

/*
 * udp_recv_cb handles incoming UDP packet from uv.  The buffer here is
 * reused for a series of packets, so we need to allocate a new one.
 * This new one can be reused to send the response then.
 */
void
isc__nm_udp_read_cb(uv_udp_t *handle, ssize_t nrecv, const uv_buf_t *buf,
		    const struct sockaddr *addr, unsigned int flags) {
	isc_nmsocket_t *sock = uv_handle_get_data((uv_handle_t *)handle);
	isc__nm_uvreq_t *req = NULL;
	uint32_t maxudp;
	isc_result_t result;
	isc_sockaddr_t sockaddr, *sa = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

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
	 * Possible reasons to return now without processing:
	 *
	 * - If we're simulating a firewall blocking UDP packets
	 *   bigger than 'maxudp' bytes for testing purposes.
	 */
	maxudp = atomic_load_relaxed(&sock->worker->netmgr->maxudp);
	if (maxudp != 0 && (uint32_t)nrecv > maxudp) {
		/*
		 * We need to keep the read_cb intact in case, so the
		 * readtimeout_cb can trigger and not crash because of
		 * missing read_req.
		 */
		goto free;
	}

	/*
	 * - If there was a networking error.
	 */
	if (nrecv < 0) {
		isc__nm_failed_read_cb(sock, isc_uverr2result(nrecv), false);
		goto free;
	}

	/*
	 * - If the network manager is shutting down
	 */
	if (isc__nm_closing(sock->worker)) {
		isc__nm_failed_read_cb(sock, ISC_R_SHUTTINGDOWN, false);
		goto free;
	}

	/*
	 * - If the socket is no longer active.
	 */
	if (!isc__nmsocket_active(sock)) {
		isc__nm_failed_read_cb(sock, ISC_R_CANCELED, false);
		goto free;
	}

	/*
	 * End of the current (iteration) datagram stream, just free the buffer.
	 * The callback with nrecv == 0 and addr == NULL is called for both
	 * normal UDP sockets and recvmmsg sockets at the end of every event
	 * loop iteration.
	 */
	if (nrecv == 0 && addr == NULL) {
		INSIST(flags == 0);
		goto free;
	}

	/*
	 * We could receive an empty datagram in which case:
	 * nrecv == 0 and addr != NULL
	 */
	INSIST(addr != NULL);

	if (!sock->route_sock) {
		result = isc_sockaddr_fromsockaddr(&sockaddr, addr);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		sa = &sockaddr;
	}

	req = isc__nm_get_read_req(sock, sa);

	/*
	 * The callback will be called synchronously, because result is
	 * ISC_R_SUCCESS, so we are ok of passing the buf directly.
	 */
	req->uvbuf.base = buf->base;
	req->uvbuf.len = nrecv;

	sock->reading = false;

	/*
	 * The client isc_nm_read() expects just a single message, so we need to
	 * stop reading now.  The reading could be restarted in the read
	 * callback with another isc_nm_read() call.
	 */
	if (sock->client) {
		isc__nmsocket_timer_stop(sock);
		isc__nm_stop_reading(sock);
		isc__nmsocket_clearcb(sock);
	}

	REQUIRE(!sock->processing);
	sock->processing = true;
	isc__nm_readcb(sock, req, ISC_R_SUCCESS, false);
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

static void
udp_send_cb(uv_udp_send_t *req, int status) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__nm_uvreq_t *uvreq = uv_handle_get_data((uv_handle_t *)req);
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_UVREQ(uvreq));
	REQUIRE(VALID_NMHANDLE(uvreq->handle));

	sock = uvreq->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	if (status < 0) {
		isc__nm_incstats(sock, STATID_SENDFAIL);
		isc__nm_failed_send_cb(sock, uvreq, isc_uverr2result(status),
				       false);
		return;
	}

	isc__nm_sendcb(sock, uvreq, result, false);
}

static _Atomic(isc_stdtime_t) last_udpsends_log = 0;

static bool
can_log_udp_sends(void) {
	isc_stdtime_t now = isc_stdtime_now();
	isc_stdtime_t last = atomic_exchange_relaxed(&last_udpsends_log, now);
	if (now != last) {
		return true;
	}

	return false;
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
	const isc_sockaddr_t *peer = &handle->peer;
	const struct sockaddr *sa = NULL;
	isc__nm_uvreq_t *uvreq = NULL;
	isc__networker_t *worker = NULL;
	uint32_t maxudp;
	int r;
	isc_result_t result;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->tid == isc_tid());

	worker = sock->worker;
	maxudp = atomic_load(&worker->netmgr->maxudp);
	sa = sock->connected ? NULL : &peer->type.sa;

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

	uvreq = isc__nm_uvreq_get(sock);
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	isc_nmhandle_attach(handle, &uvreq->handle);

	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	if (isc__nm_closing(worker)) {
		result = ISC_R_SHUTTINGDOWN;
		goto fail;
	}

	if (isc__nmsocket_closing(sock)) {
		result = ISC_R_CANCELED;
		goto fail;
	}

	if (uv_udp_get_send_queue_size(&sock->uv_handle.udp) >
	    ISC_NETMGR_UDP_SENDBUF_SIZE)
	{
		/*
		 * The kernel UDP send queue is full, try sending the UDP
		 * response synchronously instead of just failing.
		 */
		r = uv_udp_try_send(&sock->uv_handle.udp, &uvreq->uvbuf, 1, sa);
		if (r < 0) {
			if (can_log_udp_sends()) {
				isc__netmgr_log(
					worker->netmgr, ISC_LOG_ERROR,
					"Sending UDP messages failed: %s",
					isc_result_totext(isc_uverr2result(r)));
			}

			isc__nm_incstats(sock, STATID_SENDFAIL);
			result = isc_uverr2result(r);
			goto fail;
		}

		RUNTIME_CHECK(r == (int)region->length);
		isc__nm_sendcb(sock, uvreq, ISC_R_SUCCESS, true);

	} else {
		/* Send the message asynchronously */
		r = uv_udp_send(&uvreq->uv_req.udp_send, &sock->uv_handle.udp,
				&uvreq->uvbuf, 1, sa, udp_send_cb);
		if (r < 0) {
			isc__nm_incstats(sock, STATID_SENDFAIL);
			result = isc_uverr2result(r);
			goto fail;
		}
	}
	return;
fail:
	isc__nm_failed_send_cb(sock, uvreq, result, true);
}

static isc_result_t
udp_connect_direct(isc_nmsocket_t *sock, isc__nm_uvreq_t *req) {
	int uv_bind_flags = 0;
	int r;
	isc__networker_t *worker = sock->worker;
	isc_result_t result;

	r = uv_udp_init(&worker->loop->loop, &sock->uv_handle.udp);
	UV_RUNTIME_CHECK(uv_udp_init, r);
	uv_handle_set_data(&sock->uv_handle.handle, sock);

	r = uv_timer_init(&worker->loop->loop, &sock->read_timer);
	UV_RUNTIME_CHECK(uv_timer_init, r);
	uv_handle_set_data((uv_handle_t *)&sock->read_timer, sock);

	r = uv_udp_open(&sock->uv_handle.udp, sock->fd);
	if (r != 0) {
		isc__nm_incstats(sock, STATID_OPENFAIL);
		return isc_uverr2result(r);
	}
	isc__nm_incstats(sock, STATID_OPEN);

	/*
	 * uv_udp_open() enables REUSE_ADDR, we need to disable it again.
	 */
	result = isc__nm_socket_reuse(sock->fd, 0);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (sock->iface.type.sa.sa_family == AF_INET6) {
		uv_bind_flags |= UV_UDP_IPV6ONLY;
	}

#if HAVE_DECL_UV_UDP_LINUX_RECVERR
	uv_bind_flags |= UV_UDP_LINUX_RECVERR;
#endif

	r = uv_udp_bind(&sock->uv_handle.udp, &sock->iface.type.sa,
			uv_bind_flags);
	if (r != 0) {
		isc__nm_incstats(sock, STATID_BINDFAIL);
		return isc_uverr2result(r);
	}

	isc__nm_set_network_buffers(sock->worker->netmgr,
				    &sock->uv_handle.handle);

	/*
	 * On FreeBSD the UDP connect() call sometimes results in a
	 * spurious transient EADDRINUSE. Try a few more times before
	 * giving up.
	 */
	do {
		r = uv_udp_connect(&sock->uv_handle.udp, &req->peer.type.sa);
	} while (r == UV_EADDRINUSE && --req->connect_tries > 0);
	if (r != 0) {
		isc__nm_incstats(sock, STATID_CONNECTFAIL);
		return isc_uverr2result(r);
	}
	isc__nm_incstats(sock, STATID_CONNECT);

	return ISC_R_SUCCESS;
}

void
isc_nm_udpconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		  isc_nm_cb_t cb, void *cbarg, unsigned int timeout) {
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
		cb(NULL, ISC_R_SHUTTINGDOWN, cbarg);
		return;
	}

	sa_family = peer->type.sa.sa_family;

	result = isc__nm_socket(sa_family, SOCK_DGRAM, 0, &fd);
	if (result != ISC_R_SUCCESS) {
		cb(NULL, result, cbarg);
		return;
	}

	/* Initialize the new socket */
	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, isc_nm_udpsocket, local, NULL);

	sock->connect_cb = cb;
	sock->connect_cbarg = cbarg;
	sock->read_timeout = timeout;
	sock->peer = *peer;
	sock->client = true;

	sock->fd = fd;

	(void)isc__nm_socket_disable_pmtud(sock->fd, sa_family);

	(void)isc__nm_socket_min_mtu(sock->fd, sa_family);

	/* Initialize the request */
	req = isc__nm_uvreq_get(sock);
	req->cb.connect = cb;
	req->cbarg = cbarg;
	req->peer = *peer;
	req->local = *local;
	req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface);

	sock->active = true;
	sock->connecting = true;

	result = udp_connect_direct(sock, req);
	if (result != ISC_R_SUCCESS) {
		sock->active = false;
		isc__nm_failed_connect_cb(sock, req, result, true);
		isc__nmsocket_detach(&sock);
		return;
	}

	sock->connecting = false;
	sock->connected = true;

	isc__nm_connectcb(sock, req, ISC_R_SUCCESS, true);
	isc__nmsocket_detach(&sock);
}

void
isc__nm_udp_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			   bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);
	REQUIRE(sock->tid == isc_tid());

	/*
	 * For UDP server socket, we don't have child socket via
	 * "accept", so we:
	 * - we continue to read
	 * - we don't clear the callbacks
	 * - we don't destroy it (only stoplistening could do that)
	 */

	if (sock->client) {
		isc__nmsocket_timer_stop(sock);
		isc__nm_stop_reading(sock);
	}

	/* Nobody expects the callback if isc_nm_read() wasn't called */
	if (sock->reading) {
		sock->reading = false;

		if (sock->recv_cb != NULL) {
			isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
			isc__nm_readcb(sock, req, result, async);
		}
	}

	if (sock->client) {
		isc__nmsocket_clearcb(sock);
		isc__nmsocket_prep_destroy(sock);
		return;
	}
}

void
isc__nm_udp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	isc_result_t result;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->statichandle == handle);
	REQUIRE(sock->tid == isc_tid());

	/*
	 * We need to initialize the callback before checking for shutdown
	 * conditions, so the callback is always called even on error condition.
	 */
	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->reading = true;

	if (isc__nm_closing(sock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
		goto fail;
	}

	if (isc__nmsocket_closing(sock)) {
		result = ISC_R_CANCELED;
		goto fail;
	}

	result = isc__nm_start_reading(sock);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	isc__nmsocket_timer_restart(sock);
	return;

fail:
	sock->reading = true; /* required by the next call */
	isc__nm_failed_read_cb(sock, result, true);
}

static void
udp_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = uv_handle_get_data(handle);
	uv_handle_set_data(handle, NULL);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->closing);
	REQUIRE(!sock->closed);

	sock->closed = true;

	isc__nm_incstats(sock, STATID_CLOSE);

	if (sock->parent != NULL) {
		/* listening socket (listen) */
		isc__nmsocket_detach(&sock);
	} else {
		/* client and server sockets */
		sock->connected = false;
		isc__nmsocket_prep_destroy(sock);
	}
}

void
isc__nm_udp_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_udpsocket);
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(!sock->closing);

	sock->closing = true;

	isc__nmsocket_clearcb(sock);
	isc__nmsocket_timer_stop(sock);
	isc__nm_stop_reading(sock);

	/*
	 * The order of the close operation is important here, the uv_close()
	 * gets scheduled in the reverse order, so we need to close the timer
	 * last, so its gone by the time we destroy the socket
	 */

	/* 2. close the listening socket */
	isc__nmsocket_clearcb(sock);
	isc__nm_stop_reading(sock);
	uv_close(&sock->uv_handle.handle, udp_close_cb);

	/* 1. close the read timer */
	isc__nmsocket_timer_stop(sock);
	uv_close((uv_handle_t *)&sock->read_timer, NULL);
}

void
isc__nm_udp_shutdown(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(sock->type == isc_nm_udpsocket);

	/*
	 * If the socket is active, mark it inactive and
	 * continue. If it isn't active, stop now.
	 */
	if (!sock->active) {
		return;
	}
	sock->active = false;

	/* uv_udp_connect is synchronous, we can't be in connected state */
	REQUIRE(!sock->connecting);

	/*
	 * When the client detaches the last handle, the
	 * sock->statichandle would be NULL, in that case, nobody is
	 * interested in the callback.
	 */
	if (sock->statichandle != NULL) {
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
