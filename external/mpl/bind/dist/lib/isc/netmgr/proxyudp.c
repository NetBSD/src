/*	$NetBSD: proxyudp.c,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MP1 was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <isc/netmgr.h>

#include "netmgr-int.h"

typedef struct proxyudp_send_req {
	isc_nm_cb_t cb;		     /* send callback */
	void *cbarg;		     /* send callback argument */
	isc_nmhandle_t *proxyhandle; /* socket handle */
	isc_buffer_t *outbuf; /* PROXY header followed by data (client only) */
} proxyudp_send_req_t;

static bool
proxyudp_closing(isc_nmsocket_t *sock);

static void
proxyudp_stop_reading(isc_nmsocket_t *sock);

static void
proxyudp_on_header_data_cb(const isc_result_t result,
			   const isc_proxy2_command_t cmd, const int socktype,
			   const isc_sockaddr_t *restrict src_addr,
			   const isc_sockaddr_t *restrict dst_addr,
			   const isc_region_t *restrict tlvs,
			   const isc_region_t *restrict extra, void *cbarg);

static isc_nmsocket_t *
proxyudp_sock_new(isc__networker_t *worker, const isc_nmsocket_type_t type,
		  isc_sockaddr_t *addr, const bool is_server);

static void
proxyudp_read_cb(isc_nmhandle_t *handle, isc_result_t result,
		 isc_region_t *region, void *cbarg);

static void
proxyudp_call_connect_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
			 isc_result_t result);

static void
proxyudp_try_close_unused(isc_nmsocket_t *sock);

static void
proxyudp_connect_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);

static void
stop_proxyudp_child_job(void *arg);

static void
stop_proxyudp_child(isc_nmsocket_t *sock);

static void
proxyudp_clear_proxy_header_data(isc_nmsocket_t *sock);

static proxyudp_send_req_t *
proxyudp_get_send_req(isc_mem_t *mctx, isc_nmsocket_t *sock,
		      isc_nmhandle_t *proxyhandle, isc_region_t *client_data,
		      isc_nm_cb_t cb, void *cbarg);

static void
proxyudp_put_send_req(isc_mem_t *mctx, proxyudp_send_req_t *send_req,
		      const bool force_destroy);

static void
proxyudp_send_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);

static bool
proxyudp_closing(isc_nmsocket_t *sock) {
	return isc__nmsocket_closing(sock) ||
	       (sock->client && sock->outerhandle == NULL) ||
	       (sock->outerhandle != NULL &&
		isc__nmsocket_closing(sock->outerhandle->sock));
}

static void
proxyudp_stop_reading(isc_nmsocket_t *sock) {
	isc__nmsocket_timer_stop(sock);
	if (sock->outerhandle != NULL) {
		isc__nm_stop_reading(sock->outerhandle->sock);
	}
}

void
isc__nm_proxyudp_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result,
				const bool async) {
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
		proxyudp_stop_reading(sock);
	}

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
	}
}

static void
proxyudp_on_header_data_cb(const isc_result_t result,
			   const isc_proxy2_command_t cmd, const int socktype,
			   const isc_sockaddr_t *restrict src_addr,
			   const isc_sockaddr_t *restrict dst_addr,
			   const isc_region_t *restrict tlvs,
			   const isc_region_t *restrict extra, void *cbarg) {
	isc_nmhandle_t *proxyhandle = (isc_nmhandle_t *)cbarg;
	isc_nmsocket_t *proxysock = proxyhandle->sock;

	if (result != ISC_R_SUCCESS) {
		isc__nm_proxyudp_failed_read_cb(proxysock, result, false);
		return;
	} else if (extra == NULL) {
		/* a PROXYv2 header with no data is unexpected */
		goto unexpected;
	}

	/* Process header data */
	if (cmd == ISC_PROXY2_CMD_LOCAL) {
		proxyhandle->proxy_is_unspec = true;
	} else if (cmd == ISC_PROXY2_CMD_PROXY) {
		switch (socktype) {
		case 0:
			/*
			 * Treat unsupported addresses (aka AF_UNSPEC)
			 * as LOCAL.
			 */
			proxyhandle->proxy_is_unspec = true;
			break;
		case SOCK_STREAM:
			/*
			 * In some cases proxies can do protocol conversion. In
			 * this case, the original request might have arrived
			 * over TCP-based transport and, thus, the PROXYv2
			 * header can contain SOCK_STREAM, while for UDP one
			 * would expect SOCK_DGRAM. That might be unexpected,
			 * but, as the main idea behind PROXYv2 is to carry the
			 * original endpoint information to back-ends, that is
			 * fine.
			 */
		case SOCK_DGRAM:
			INSIST(isc_sockaddr_pf(src_addr) ==
			       isc_sockaddr_pf(dst_addr));
			/* We will treat AF_UNIX as unspec */
			if (isc_sockaddr_pf(src_addr) == AF_UNIX) {
				proxyhandle->proxy_is_unspec = true;
			} else {
				if (!isc__nm_valid_proxy_addresses(src_addr,
								   dst_addr))
				{
					goto unexpected;
				}
			}
			break;
		default:
			goto unexpected;
		}
	}

	if (!proxyhandle->proxy_is_unspec) {
		INSIST(src_addr != NULL);
		INSIST(dst_addr != NULL);
		proxyhandle->local = *dst_addr;
		proxyhandle->peer = *src_addr;
	}

	isc__nm_received_proxy_header_log(proxyhandle, cmd, socktype, src_addr,
					  dst_addr, tlvs);
	proxysock->recv_cb(proxyhandle, result, (isc_region_t *)extra,
			   proxysock->recv_cbarg);
	return;

unexpected:
	isc__nm_proxyudp_failed_read_cb(proxysock, ISC_R_UNEXPECTED, false);
}

static isc_nmsocket_t *
proxyudp_sock_new(isc__networker_t *worker, const isc_nmsocket_type_t type,
		  isc_sockaddr_t *addr, const bool is_server) {
	isc_nmsocket_t *sock;
	INSIST(type == isc_nm_proxyudpsocket ||
	       type == isc_nm_proxyudplistener);

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, type, addr, NULL);
	sock->result = ISC_R_UNSET;
	if (type == isc_nm_proxyudpsocket) {
		uint32_t initial = 0;
		isc_nm_gettimeouts(worker->netmgr, &initial, NULL, NULL, NULL);
		sock->read_timeout = initial;
		sock->client = !is_server;
		sock->connecting = !is_server;
		if (!is_server) {
			isc_buffer_allocate(worker->mctx,
					    &sock->proxy.proxy2.outbuf,
					    ISC_NM_PROXY2_DEFAULT_BUFFER_SIZE);
		}
	} else if (type == isc_nm_proxyudplistener) {
		size_t nworkers = worker->netmgr->nloops;
		sock->proxy.udp_server_socks_num = nworkers;
		sock->proxy.udp_server_socks = isc_mem_cget(
			worker->mctx, nworkers, sizeof(isc_nmsocket_t *));
	}

	return sock;
}

static void
proxyudp_read_cb(isc_nmhandle_t *handle, isc_result_t result,
		 isc_region_t *region, void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *proxysock = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_NMHANDLE(handle));

	if (sock->client) {
		proxysock = sock;
	} else {
		INSIST(sock->type == isc_nm_proxyudplistener);
		proxysock = sock->proxy.udp_server_socks[handle->sock->tid];
		if (proxysock->outerhandle == NULL) {
			isc_nmhandle_attach(handle, &proxysock->outerhandle);
		}

		proxysock->iface = isc_nmhandle_localaddr(handle);
		proxysock->peer = isc_nmhandle_peeraddr(handle);
	}

	INSIST(proxysock->tid == isc_tid());

	if (result != ISC_R_SUCCESS) {
		if (!proxysock->client) {
			goto failed;
		}

		if (result != ISC_R_TIMEDOUT) {
			goto failed;
		}
	}

	if (isc__nm_closing(proxysock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
		goto failed;
	} else if (proxyudp_closing(proxysock)) {
		result = ISC_R_CANCELED;
		goto failed;
	}

	/* Handle initial PROXY header data */
	if (!proxysock->client) {
		isc_nmhandle_t *proxyhandle = NULL;
		proxysock->reading = false;
		proxyhandle = isc__nmhandle_get(proxysock, &proxysock->peer,
						&proxysock->iface);
		isc_nmhandle_attach(handle, &proxyhandle->proxy_udphandle);
		(void)isc_proxy2_header_handle_directly(
			region, proxyudp_on_header_data_cb, proxyhandle);
		isc_nmhandle_detach(&proxyhandle);
	} else {
		isc_nm_recv_cb_t recv_cb = NULL;
		void *recv_cbarg = NULL;

		recv_cb = proxysock->recv_cb;
		recv_cbarg = proxysock->recv_cbarg;

		if (result != ISC_R_TIMEDOUT) {
			proxysock->reading = false;
			proxyudp_stop_reading(proxysock);
		}
		recv_cb(proxysock->statichandle, result, region, recv_cbarg);

		if (result == ISC_R_TIMEDOUT &&
		    !isc__nmsocket_timer_running(proxysock))

		{
			isc__nmsocket_clearcb(proxysock);
			goto failed;
		}
	}

	proxyudp_try_close_unused(proxysock);

	return;

failed:
	isc__nm_proxyudp_failed_read_cb(proxysock, result, false);
	return;
}

isc_result_t
isc_nm_listenproxyudp(isc_nm_t *mgr, uint32_t workers, isc_sockaddr_t *iface,
		      isc_nm_recv_cb_t cb, void *cbarg,
		      isc_nmsocket_t **sockp) {
	isc_result_t result;
	isc_nmsocket_t *listener = NULL;
	isc__networker_t *worker = &mgr->workers[isc_tid()];

	REQUIRE(VALID_NM(mgr));
	REQUIRE(isc_tid() == 0);
	REQUIRE(sockp != NULL && *sockp == NULL);

	if (isc__nm_closing(worker)) {
		return ISC_R_SHUTTINGDOWN;
	}

	listener = proxyudp_sock_new(worker, isc_nm_proxyudplistener, iface,
				     true);
	listener->recv_cb = cb;
	listener->recv_cbarg = cbarg;

	for (size_t i = 0; i < listener->proxy.udp_server_socks_num; i++) {
		listener->proxy.udp_server_socks[i] = proxyudp_sock_new(
			&mgr->workers[i], isc_nm_proxyudpsocket, iface, true);

		listener->proxy.udp_server_socks[i]->recv_cb =
			listener->recv_cb;

		listener->proxy.udp_server_socks[i]->recv_cbarg =
			listener->recv_cbarg;

		isc__nmsocket_attach(
			listener,
			&listener->proxy.udp_server_socks[i]->listener);
	}

	result = isc_nm_listenudp(mgr, workers, iface, proxyudp_read_cb,
				  listener, &listener->outer);

	if (result == ISC_R_SUCCESS) {
		listener->active = true;
		listener->result = result;
		listener->nchildren = listener->outer->nchildren;
		*sockp = listener;
	} else {
		for (size_t i = 0; i < listener->proxy.udp_server_socks_num;
		     i++)
		{
			stop_proxyudp_child(
				listener->proxy.udp_server_socks[i]);
		}
		listener->closed = true;
		isc__nmsocket_detach(&listener);
	}

	return result;
}

static void
proxyudp_call_connect_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
			 isc_result_t result) {
	sock->connecting = false;
	if (sock->connect_cb == NULL) {
		return;
	}

	sock->connect_cb(handle, result, sock->connect_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc__nmsocket_clearcb(handle->sock);
	} else {
		sock->connected = true;
	}
}

static void
proxyudp_try_close_unused(isc_nmsocket_t *sock) {
	/* try to close unused socket */
	if (sock->statichandle == NULL && sock->proxy.nsending == 0) {
		if (sock->client) {
			isc__nmsocket_prep_destroy(sock);
		} else if (sock->outerhandle) {
			isc_nmhandle_detach(&sock->outerhandle);
		}
	}
}

static void
proxyudp_connect_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;
	isc_nmhandle_t *proxyhandle = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	sock->tid = isc_tid();

	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	INSIST(VALID_NMHANDLE(handle));

	sock->iface = isc_nmhandle_localaddr(handle);
	sock->peer = isc_nmhandle_peeraddr(handle);
	isc_nmhandle_attach(handle, &sock->outerhandle);
	handle->sock->proxy.sock = sock;
	sock->active = true;
	sock->connected = true;
	sock->connecting = false;

	proxyhandle = isc__nmhandle_get(sock, &sock->peer, &sock->iface);
	proxyudp_call_connect_cb(sock, proxyhandle, ISC_R_SUCCESS);
	isc_nmhandle_detach(&proxyhandle);

	proxyudp_try_close_unused(sock);

	isc__nmsocket_detach(&handle->sock->proxy.sock);

	return;
error:
	proxyhandle = isc__nmhandle_get(sock, NULL, NULL);
	sock->closed = true;
	proxyudp_call_connect_cb(sock, proxyhandle, result);
	isc_nmhandle_detach(&proxyhandle);
	isc__nmsocket_detach(&sock);
}

void
isc_nm_proxyudpconnect(isc_nm_t *mgr, isc_sockaddr_t *local,
		       isc_sockaddr_t *peer, isc_nm_cb_t cb, void *cbarg,
		       unsigned int timeout,
		       isc_nm_proxyheader_info_t *proxy_info) {
	isc_result_t result = ISC_R_FAILURE;
	isc_nmsocket_t *nsock = NULL;
	isc__networker_t *worker = &mgr->workers[isc_tid()];

	REQUIRE(VALID_NM(mgr));

	if (isc__nm_closing(worker)) {
		cb(NULL, ISC_R_SHUTTINGDOWN, cbarg);
		return;
	}

	nsock = proxyudp_sock_new(worker, isc_nm_proxyudpsocket, local, false);
	nsock->connect_cb = cb;
	nsock->connect_cbarg = cbarg;
	nsock->read_timeout = timeout;
	nsock->connecting = true;

	if (proxy_info == NULL) {
		result = isc_proxy2_make_header(nsock->proxy.proxy2.outbuf,
						ISC_PROXY2_CMD_LOCAL, 0, NULL,
						NULL, NULL);
	} else if (proxy_info->complete) {
		isc_buffer_putmem(nsock->proxy.proxy2.outbuf,
				  proxy_info->complete_header.base,
				  proxy_info->complete_header.length);
		result = ISC_R_SUCCESS;
	} else if (!proxy_info->complete) {
		result = isc_proxy2_make_header(
			nsock->proxy.proxy2.outbuf, ISC_PROXY2_CMD_PROXY,
			SOCK_DGRAM, &proxy_info->proxy_info.src_addr,
			&proxy_info->proxy_info.dst_addr,
			&proxy_info->proxy_info.tlv_data);
	}
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc_nm_udpconnect(mgr, local, peer, proxyudp_connect_cb, nsock,
			  timeout);
}

/*
 * Asynchronous 'udpstop' call handler: stop listening on a UDP socket.
 */
static void
stop_proxyudp_child_job(void *arg) {
	isc_nmsocket_t *listener = NULL;
	isc_nmsocket_t *sock = arg;
	uint32_t tid = 0;

	if (sock == NULL) {
		return;
	}

	INSIST(VALID_NMSOCK(sock));
	INSIST(sock->tid == isc_tid());

	listener = sock->listener;
	sock->listener = NULL;

	INSIST(VALID_NMSOCK(listener));
	INSIST(listener->type == isc_nm_proxyudplistener);

	if (sock->outerhandle != NULL) {
		proxyudp_stop_reading(sock);
		isc_nmhandle_detach(&sock->outerhandle);
	}

	tid = sock->tid;
	isc__nmsocket_prep_destroy(sock);
	isc__nmsocket_detach(&listener->proxy.udp_server_socks[tid]);
	isc__nmsocket_detach(&listener);
}

static void
stop_proxyudp_child(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));

	if (sock->tid == 0) {
		stop_proxyudp_child_job(sock);
	} else {
		isc_async_run(sock->worker->loop, stop_proxyudp_child_job,
			      sock);
	}
}

void
isc__nm_proxyudp_stoplistening(isc_nmsocket_t *listener) {
	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(listener->type == isc_nm_proxyudplistener);
	REQUIRE(listener->proxy.sock == NULL);

	isc__nmsocket_stop(listener);

	listener->active = false;

	for (size_t i = 1; i < listener->proxy.udp_server_socks_num; i++) {
		stop_proxyudp_child(listener->proxy.udp_server_socks[i]);
	}

	stop_proxyudp_child(listener->proxy.udp_server_socks[0]);
}

static void
proxyudp_clear_proxy_header_data(isc_nmsocket_t *sock) {
	if (sock->client && sock->proxy.proxy2.outbuf != NULL) {
		isc_buffer_free(&sock->proxy.proxy2.outbuf);
	}
}

void
isc__nm_proxyudp_cleanup_data(isc_nmsocket_t *sock) {
	switch (sock->type) {
	case isc_nm_proxyudpsocket:
		if (sock->proxy.send_req != NULL) {
			proxyudp_put_send_req(sock->worker->mctx,
					      sock->proxy.send_req, true);
		}

		proxyudp_clear_proxy_header_data(sock);
		break;
	case isc_nm_proxyudplistener:
		isc_mem_cput(sock->worker->mctx, sock->proxy.udp_server_socks,
			     sock->proxy.udp_server_socks_num,
			     sizeof(isc_nmsocket_t *));
		break;
	case isc_nm_udpsocket:
		INSIST(sock->proxy.sock == NULL);
		break;
	default:
		break;
	};
}

void
isc__nmhandle_proxyudp_cleartimeout(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxyudpsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_cleartimeout(sock->outerhandle);
	}
}

void
isc__nmhandle_proxyudp_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxyudpsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_settimeout(sock->outerhandle, timeout);
	}
}

void
isc__nmhandle_proxyudp_setwritetimeout(isc_nmhandle_t *handle,
				       uint64_t write_timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxyudpsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nmhandle_setwritetimeout(sock->outerhandle, write_timeout);
	}
}

bool
isc__nmsocket_proxyudp_timer_running(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxyudpsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		return isc__nmsocket_timer_running(sock->outerhandle->sock);
	}

	return false;
}

void
isc__nmsocket_proxyudp_timer_restart(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxyudpsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_timer_restart(sock->outerhandle->sock);
	}
}

void
isc__nmsocket_proxyudp_timer_stop(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxyudpsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_timer_stop(sock->outerhandle->sock);
	}
}

void
isc__nm_proxyudp_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxyudpsocket);
	REQUIRE(sock->tid == isc_tid());

	sock->closing = true;

	/*
	 * At this point we're certain that there are no
	 * external references, we can close everything.
	 */
	proxyudp_stop_reading(sock);
	sock->reading = false;
	if (sock->outerhandle != NULL) {
		isc_nmhandle_close(sock->outerhandle);
		isc_nmhandle_detach(&sock->outerhandle);
	}

	if (sock->proxy.sock != NULL) {
		isc__nmsocket_detach(&sock->proxy.sock);
	}

	/* Further cleanup performed in isc__nm_proxyudp_cleanup_data() */
	sock->closed = true;
	sock->active = false;
}

void
isc__nm_proxyudp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb,
		      void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxyudpsocket);
	REQUIRE(sock->recv_handle == NULL);
	REQUIRE(sock->tid == isc_tid());

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->reading = true;

	if (isc__nm_closing(sock->worker)) {
		isc__nm_proxyudp_failed_read_cb(sock, ISC_R_SHUTTINGDOWN,
						false);
		return;
	} else if (proxyudp_closing(sock)) {
		isc__nm_proxyudp_failed_read_cb(sock, ISC_R_CANCELED, true);
		return;
	}

	isc_nm_read(sock->outerhandle, proxyudp_read_cb, sock);
}

static proxyudp_send_req_t *
proxyudp_get_send_req(isc_mem_t *mctx, isc_nmsocket_t *sock,
		      isc_nmhandle_t *proxyhandle, isc_region_t *client_data,
		      isc_nm_cb_t cb, void *cbarg) {
	proxyudp_send_req_t *send_req = NULL;

	if (sock->proxy.send_req != NULL) {
		/*
		 * We have a previously allocated object - let's use that.
		 * That should help reducing stress on the memory allocator.
		 */
		send_req = (proxyudp_send_req_t *)sock->proxy.send_req;
		sock->proxy.send_req = NULL;
	} else {
		/* Allocate a new object. */
		send_req = isc_mem_get(mctx, sizeof(*send_req));
		*send_req = (proxyudp_send_req_t){ 0 };
	}

	/* Initialise the send request object */
	send_req->cb = cb;
	send_req->cbarg = cbarg;
	isc_nmhandle_attach(proxyhandle, &send_req->proxyhandle);

	if (client_data != NULL) {
		isc_region_t header_region = { 0 };
		INSIST(sock->client);
		INSIST(sock->proxy.proxy2.outbuf != NULL);

		isc_buffer_usedregion(sock->proxy.proxy2.outbuf,
				      &header_region);

		INSIST(header_region.length > 0);

		/* allocate the buffer if it has not been allocated yet */
		if (send_req->outbuf == NULL) {
			isc_buffer_allocate(mctx, &send_req->outbuf,
					    client_data->length +
						    header_region.length);
		}

		isc_buffer_putmem(send_req->outbuf, header_region.base,
				  header_region.length);
		isc_buffer_putmem(send_req->outbuf, client_data->base,
				  client_data->length);
	}

	sock->proxy.nsending++;

	return send_req;
}

static void
proxyudp_put_send_req(isc_mem_t *mctx, proxyudp_send_req_t *send_req,
		      const bool force_destroy) {
	if (send_req->outbuf != NULL) {
		/* clear the buffer to reuse it further */
		isc_buffer_clear(send_req->outbuf);
	}
	/*
	 * Attempt to put the object for reuse later if we are not
	 * wrapping up.
	 */
	if (!force_destroy) {
		isc_nmsocket_t *sock = send_req->proxyhandle->sock;
		sock->proxy.nsending--;
		isc_nmhandle_detach(&send_req->proxyhandle);
		if (sock->proxy.send_req == NULL) {
			sock->proxy.send_req = send_req;
			/*
			 * An object has been recycled,
			 * if not - we are going to destroy it.
			 */
			return;
		}
	} else {
		if (send_req->outbuf != NULL) {
			isc_buffer_free(&send_req->outbuf);
		}
	}

	isc_mem_put(mctx, send_req, sizeof(*send_req));
}

static void
proxyudp_send_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	proxyudp_send_req_t *send_req = (proxyudp_send_req_t *)cbarg;
	isc_mem_t *mctx;
	isc_nm_cb_t cb;
	void *send_cbarg;
	isc_nmhandle_t *proxyhandle = NULL;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMHANDLE(send_req->proxyhandle));
	REQUIRE(VALID_NMSOCK(send_req->proxyhandle->sock));
	REQUIRE(send_req->proxyhandle->sock->tid == isc_tid());

	mctx = send_req->proxyhandle->sock->worker->mctx;
	cb = send_req->cb;
	send_cbarg = send_req->cbarg;

	isc_nmhandle_attach(send_req->proxyhandle, &proxyhandle);
	isc__nmsocket_attach(proxyhandle->sock, &sock);

	/* try to keep the send request object for reuse */
	proxyudp_put_send_req(mctx, send_req, false);
	cb(proxyhandle, result, send_cbarg);
	isc_nmhandle_detach(&proxyhandle);

	/*
	 * Try to close the client socket when we do not need it
	 * anymore. In the case of server socket - detach the underlying
	 * (UDP) handle when the socket is not being used anymore.
	 */
	proxyudp_try_close_unused(sock);
	isc__nmsocket_detach(&sock);
}

void
isc__nm_proxyudp_send(isc_nmhandle_t *handle, isc_region_t *region,
		      isc_nm_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	proxyudp_send_req_t *send_req = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_proxyudpsocket);

	if (isc__nm_closing(sock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
	} else if (proxyudp_closing(sock)) {
		result = ISC_R_CANCELED;
	}

	if (result != ISC_R_SUCCESS) {
		isc__nm_uvreq_t *uvreq = isc__nm_uvreq_get(sock);
		isc_nmhandle_attach(handle, &uvreq->handle);
		uvreq->cb.send = cb;
		uvreq->cbarg = cbarg;

		isc__nm_failed_send_cb(sock, uvreq, result, true);
		return;
	}

	send_req = proxyudp_get_send_req(sock->worker->mctx, sock, handle,
					 (sock->client ? region : NULL), cb,
					 cbarg);
	if (sock->client) {
		isc_region_t send_data = { 0 };
		isc_buffer_usedregion(send_req->outbuf, &send_data);
		isc_nm_send(sock->outerhandle, &send_data, proxyudp_send_cb,
			    send_req);
	} else {
		isc_nm_send(handle->proxy_udphandle, region, proxyudp_send_cb,
			    send_req);
	}
}
