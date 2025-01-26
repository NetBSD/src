/*	$NetBSD: proxystream.c,v 1.2 2025/01/26 16:25:43 christos Exp $	*/

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

#include <isc/netmgr.h>

#include "netmgr-int.h"

/*
 * The idea behind the transport is simple after accepting the
 * connection or connecting to a remote server it enters PROXYv2
 * handling mode: that is, it either attempts to read (when accepting
 * the connection) or send (when establishing a connection) a PROXYv2
 * header. After that it works like a mere wrapper on top of the
 * underlying stream-based transport (TCP).
 */

typedef struct proxystream_send_req {
	isc_nm_cb_t cb;		     /* send callback */
	void *cbarg;		     /* send callback argument */
	isc_nmhandle_t *proxyhandle; /* PROXY Stream socket handle */
} proxystream_send_req_t;

static void
proxystream_on_header_data_cb(const isc_result_t result,
			      const isc_proxy2_command_t cmd,
			      const int socktype,
			      const isc_sockaddr_t *restrict src_addr,
			      const isc_sockaddr_t *restrict dst_addr,
			      const isc_region_t *restrict tlv_blob,
			      const isc_region_t *restrict extra, void *cbarg);

static isc_nmsocket_t *
proxystream_sock_new(isc__networker_t *worker, const isc_nmsocket_type_t type,
		     isc_sockaddr_t *addr, const bool is_server);

static isc_result_t
proxystream_accept_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);

static void
proxystream_connect_cb(isc_nmhandle_t *handle, isc_result_t result,
		       void *cbarg);

static void
proxystream_failed_read_cb_async(void *arg);

static void
proxystream_clear_proxy_header_data(isc_nmsocket_t *sock);

static void
proxystream_read_start(isc_nmsocket_t *sock);

static void
proxystream_read_stop(isc_nmsocket_t *sock);

static void
proxystream_try_close_unused(isc_nmsocket_t *sock);

static void
proxystream_call_connect_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
			    isc_result_t result);

static bool
proxystream_closing(isc_nmsocket_t *sock);

static void
proxystream_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result);

static void
proxystream_read_cb(isc_nmhandle_t *handle, isc_result_t result,
		    isc_region_t *region, void *cbarg);

static void
proxystream_read_extra_cb(void *arg);

static proxystream_send_req_t *
proxystream_get_send_req(isc_mem_t *mctx, isc_nmsocket_t *sock,
			 isc_nmhandle_t *proxyhandle, isc_nm_cb_t cb,
			 void *cbarg);

static void
proxystream_put_send_req(isc_mem_t *mctx, proxystream_send_req_t *send_req,
			 const bool force_destroy);

static void
proxystream_send_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);

static void
proxystream_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		 void *cbarg, const bool dnsmsg);

static void
proxystream_on_header_data_cb(const isc_result_t result,
			      const isc_proxy2_command_t cmd,
			      const int socktype,
			      const isc_sockaddr_t *restrict src_addr,
			      const isc_sockaddr_t *restrict dst_addr,
			      const isc_region_t *restrict tlvs,
			      const isc_region_t *restrict extra, void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;

	switch (result) {
	case ISC_R_SUCCESS: {
		isc_nmhandle_t *proxyhandle = NULL;
		isc_result_t accept_result = ISC_R_FAILURE;
		bool call_accept = false;
		bool is_unspec = false;

		/*
		 * After header has been processed - stop reading (thus,
		 * stopping the timer) and disable manual timer control as in
		 * the case of TCP it is disabled by default
		 */
		proxystream_read_stop(sock);
		isc__nmhandle_set_manual_timer(sock->outerhandle, false);

		sock->proxy.header_processed = true;
		if (extra == NULL) {
			sock->proxy.extra_processed = true;
		}

		/* Process header data */
		if (cmd == ISC_PROXY2_CMD_LOCAL) {
			is_unspec = true;
			call_accept = true;
		} else if (cmd == ISC_PROXY2_CMD_PROXY) {
			switch (socktype) {
			case 0:
				/*
				 * Treat unsupported addresses (aka AF_UNSPEC)
				 * as LOCAL.
				 */
				is_unspec = true;
				call_accept = true;
				break;
			case SOCK_DGRAM:
				/*
				 * In some cases proxies can do protocol
				 * conversion. In this case, the original
				 * request might have arrived over UDP-based
				 * transport and, thus, the PROXYv2 header can
				 * contain SOCK_DGRAM, while for TCP one would
				 * expect SOCK_STREAM. That might be unexpected,
				 * but, as the main idea behind PROXYv2 is to
				 * carry the original endpoint information to
				 * back-ends, that is fine.
				 *
				 * At least "dnsdist" does that when redirecting
				 * a UDP request to a TCP or TLS-only server.
				 */
			case SOCK_STREAM:
				INSIST(isc_sockaddr_pf(src_addr) ==
				       isc_sockaddr_pf(dst_addr));
				/* We will treat AF_UNIX as unspec */
				if (isc_sockaddr_pf(src_addr) == AF_UNIX) {
					is_unspec = true;
				}

				if (!is_unspec &&
				    !isc__nm_valid_proxy_addresses(src_addr,
								   dst_addr))
				{
					break;
				}

				call_accept = true;
				break;
			default:
				break;
			}
		}

		if (call_accept) {
			if (is_unspec) {
				proxyhandle = isc__nmhandle_get(
					sock, &sock->peer, &sock->iface);
			} else {
				INSIST(src_addr != NULL);
				INSIST(dst_addr != NULL);
				proxyhandle = isc__nmhandle_get(sock, src_addr,
								dst_addr);
			}
			proxyhandle->proxy_is_unspec = is_unspec;
			isc__nm_received_proxy_header_log(proxyhandle, cmd,
							  socktype, src_addr,
							  dst_addr, tlvs);
			accept_result = sock->accept_cb(proxyhandle, result,
							sock->accept_cbarg);
			isc_nmhandle_detach(&proxyhandle);
		}

		if (accept_result != ISC_R_SUCCESS) {
			isc__nmsocket_detach(&sock->listener);
			isc_nmhandle_detach(&sock->outerhandle);
			sock->closed = true;
		}

		sock->accepting = false;

		proxystream_try_close_unused(sock);
	} break;
	case ISC_R_NOMORE:
		/*
		 * That is fine, wait for more data to complete the PROXY
		 * header
		 */
		break;
	default:
		proxystream_failed_read_cb(sock, result);
		break;
	};
}

static void
proxystream_handle_incoming_header_data(isc_nmsocket_t *sock,
					isc_region_t *restrict data) {
	isc_proxy2_handler_t *restrict handler = sock->proxy.proxy2.handler;

	(void)isc_proxy2_handler_push(handler, data);
	proxystream_try_close_unused(sock);
}

static isc_nmsocket_t *
proxystream_sock_new(isc__networker_t *worker, const isc_nmsocket_type_t type,
		     isc_sockaddr_t *addr, const bool is_server) {
	isc_nmsocket_t *sock;
	INSIST(type == isc_nm_proxystreamsocket ||
	       type == isc_nm_proxystreamlistener);

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, type, addr, NULL);
	sock->result = ISC_R_UNSET;
	if (type == isc_nm_proxystreamsocket) {
		uint32_t initial = 0;
		isc_nm_gettimeouts(worker->netmgr, &initial, NULL, NULL, NULL);
		sock->read_timeout = initial;
		sock->client = !is_server;
		sock->connecting = !is_server;
		if (is_server) {
			/*
			 * Smallest TCP (over IPv6) segment size we required to
			 * support. An adequate value for both IPv4 and IPv6.
			 */
			sock->proxy.proxy2.handler = isc_proxy2_handler_new(
				worker->mctx, NM_MAXSEG,
				proxystream_on_header_data_cb, sock);
		} else {
			isc_buffer_allocate(worker->mctx,
					    &sock->proxy.proxy2.outbuf,
					    ISC_NM_PROXY2_DEFAULT_BUFFER_SIZE);
		}
	}

	return sock;
}

static isc_result_t
proxystream_accept_cb(isc_nmhandle_t *handle, isc_result_t result,
		      void *cbarg) {
	isc_nmsocket_t *listensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *nsock = NULL;
	isc_sockaddr_t iface;

	if (result != ISC_R_SUCCESS) {
		return result;
	}

	INSIST(VALID_NMHANDLE(handle));
	INSIST(VALID_NMSOCK(handle->sock));
	INSIST(VALID_NMSOCK(listensock));
	INSIST(listensock->type == isc_nm_proxystreamlistener);

	if (isc__nm_closing(handle->sock->worker)) {
		return ISC_R_SHUTTINGDOWN;
	} else if (isc__nmsocket_closing(handle->sock)) {
		return ISC_R_CANCELED;
	}

	iface = isc_nmhandle_localaddr(handle);
	nsock = proxystream_sock_new(handle->sock->worker,
				     isc_nm_proxystreamsocket, &iface, true);
	INSIST(listensock->accept_cb != NULL);
	nsock->accept_cb = listensock->accept_cb;
	nsock->accept_cbarg = listensock->accept_cbarg;

	nsock->peer = isc_nmhandle_peeraddr(handle);
	nsock->tid = isc_tid();
	nsock->accepting = true;
	nsock->active = true;

	isc__nmsocket_attach(listensock, &nsock->listener);
	isc_nmhandle_attach(handle, &nsock->outerhandle);
	handle->sock->proxy.sock = nsock;

	/*
	 * We need to control the timer manually as we do *not* want it to
	 * be reset on partial header data reads.
	 */
	isc__nmhandle_set_manual_timer(nsock->outerhandle, true);
	isc__nmsocket_timer_restart(nsock);

	proxystream_read_start(nsock);

	return ISC_R_SUCCESS;
}

isc_result_t
isc_nm_listenproxystream(isc_nm_t *mgr, uint32_t workers, isc_sockaddr_t *iface,
			 isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
			 int backlog, isc_quota_t *quota, isc_tlsctx_t *tlsctx,
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

	listener = proxystream_sock_new(worker, isc_nm_proxystreamlistener,
					iface, true);
	listener->accept_cb = accept_cb;
	listener->accept_cbarg = accept_cbarg;

	if (tlsctx == NULL) {
		result = isc_nm_listentcp(mgr, workers, iface,
					  proxystream_accept_cb, listener,
					  backlog, quota, &listener->outer);
	} else {
		result = isc_nm_listentls(
			mgr, workers, iface, proxystream_accept_cb, listener,
			backlog, quota, tlsctx, false, &listener->outer);
	}

	if (result != ISC_R_SUCCESS) {
		listener->closed = true;
		isc__nmsocket_detach(&listener);
		return result;
	}

	listener->active = true;
	listener->result = result;
	listener->nchildren = listener->outer->nchildren;

	*sockp = listener;

	return result;
}

static void
proxystream_try_close_unused(isc_nmsocket_t *sock) {
	/* try to close unused socket */
	if (sock->statichandle == NULL && sock->proxy.nsending == 0) {
		isc__nmsocket_prep_destroy(sock);
	}
}

static void
proxystream_call_connect_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
			    isc_result_t result) {
	sock->connecting = false;
	if (sock->connect_cb == NULL) {
		return;
	}

	if (result == ISC_R_SUCCESS) {
		sock->connected = true;
	}

	sock->connect_cb(handle, result, sock->connect_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc__nmsocket_clearcb(handle->sock);
	}
}

static void
proxystream_send_header_cb(isc_nmhandle_t *transphandle, isc_result_t result,
			   void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;
	isc_nmhandle_t *proxyhandle = NULL;

	REQUIRE(VALID_NMHANDLE(transphandle));
	REQUIRE(VALID_NMSOCK(sock));

	sock->proxy.nsending--;
	sock->proxy.header_processed = true;

	if (isc__nm_closing(transphandle->sock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	proxyhandle = isc__nmhandle_get(sock, &sock->peer, &sock->iface);
	proxystream_call_connect_cb(sock, proxyhandle, result);
	isc_nmhandle_detach(&proxyhandle);

	proxystream_try_close_unused(sock);
}

static void
proxystream_connect_cb(isc_nmhandle_t *handle, isc_result_t result,
		       void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;
	isc_nmhandle_t *proxyhandle = NULL;
	isc_region_t header = { 0 };

	REQUIRE(VALID_NMSOCK(sock));

	sock->tid = isc_tid();

	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	INSIST(VALID_NMHANDLE(handle));

	sock->iface = isc_nmhandle_localaddr(handle);
	sock->peer = isc_nmhandle_peeraddr(handle);
	if (isc__nm_closing(handle->sock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
		goto error;
	} else if (isc__nmsocket_closing(handle->sock)) {
		result = ISC_R_CANCELED;
		goto error;
	}

	isc_nmhandle_attach(handle, &sock->outerhandle);
	handle->sock->proxy.sock = sock;
	sock->active = true;

	isc_buffer_usedregion(sock->proxy.proxy2.outbuf, &header);
	sock->proxy.nsending++;
	isc_nm_send(handle, &header, proxystream_send_header_cb, sock);

	proxystream_try_close_unused(sock);

	return;
error:
	proxyhandle = isc__nmhandle_get(sock, NULL, NULL);
	sock->closed = true;
	proxystream_call_connect_cb(sock, proxyhandle, result);
	isc_nmhandle_detach(&proxyhandle);
	isc__nmsocket_detach(&sock);
}

void
isc_nm_proxystreamconnect(isc_nm_t *mgr, isc_sockaddr_t *local,
			  isc_sockaddr_t *peer, isc_nm_cb_t cb, void *cbarg,
			  unsigned int timeout, isc_tlsctx_t *tlsctx,
			  isc_tlsctx_client_session_cache_t *client_sess_cache,
			  isc_nm_proxyheader_info_t *proxy_info) {
	isc_result_t result = ISC_R_FAILURE;
	isc_nmsocket_t *nsock = NULL;
	isc__networker_t *worker = &mgr->workers[isc_tid()];

	REQUIRE(VALID_NM(mgr));

	if (isc__nm_closing(worker)) {
		cb(NULL, ISC_R_SHUTTINGDOWN, cbarg);
		return;
	}

	nsock = proxystream_sock_new(worker, isc_nm_proxystreamsocket, local,
				     false);
	nsock->connect_cb = cb;
	nsock->connect_cbarg = cbarg;
	nsock->connect_timeout = timeout;

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
			SOCK_STREAM, &proxy_info->proxy_info.src_addr,
			&proxy_info->proxy_info.dst_addr,
			&proxy_info->proxy_info.tlv_data);
	}
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (tlsctx == NULL) {
		isc_nm_tcpconnect(mgr, local, peer, proxystream_connect_cb,
				  nsock, nsock->connect_timeout);
	} else {
		isc_nm_tlsconnect(mgr, local, peer, proxystream_connect_cb,
				  nsock, tlsctx, client_sess_cache,
				  nsock->connect_timeout, false, NULL);
	}
}

static void
proxystream_failed_read_cb_async(void *arg) {
	isc__nm_uvreq_t *req = (isc__nm_uvreq_t *)arg;

	proxystream_failed_read_cb(req->sock, req->result);
	isc__nm_uvreq_put(&req);
}

void
isc__nm_proxystream_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
				   bool async) {
	proxystream_read_stop(sock);

	if (!async) {
		proxystream_failed_read_cb(sock, result);
	} else {
		isc__nm_uvreq_t *req = isc__nm_uvreq_get(sock);
		req->result = result;
		req->cbarg = sock;
		isc_job_run(sock->worker->loop, &req->job,
			    proxystream_failed_read_cb_async, req);
	}
}

void
isc__nm_proxystream_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamlistener);
	REQUIRE(sock->proxy.sock == NULL);

	isc__nmsocket_stop(sock);
}

static void
proxystream_clear_proxy_header_data(isc_nmsocket_t *sock) {
	if (!sock->client && sock->proxy.proxy2.handler != NULL) {
		isc_proxy2_handler_free(&sock->proxy.proxy2.handler);
	} else if (sock->client && sock->proxy.proxy2.outbuf != NULL) {
		isc_buffer_free(&sock->proxy.proxy2.outbuf);
	}
}

void
isc__nm_proxystream_cleanup_data(isc_nmsocket_t *sock) {
	switch (sock->type) {
	case isc_nm_tcpsocket:
	case isc_nm_tlssocket:
		if (sock->proxy.sock != NULL) {
			isc__nmsocket_detach(&sock->proxy.sock);
		}
		break;
	case isc_nm_proxystreamsocket:
		if (sock->proxy.send_req != NULL) {
			proxystream_put_send_req(
				sock->worker->mctx,
				(proxystream_send_req_t *)sock->proxy.send_req,
				true);
		}

		proxystream_clear_proxy_header_data(sock);
		break;
	default:
		break;
	};
}

void
isc__nmhandle_proxystream_cleartimeout(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_cleartimeout(sock->outerhandle);
	}
}

void
isc__nmhandle_proxystream_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_settimeout(sock->outerhandle, timeout);
	}
}

void
isc__nmhandle_proxystream_keepalive(isc_nmhandle_t *handle, bool value) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nmhandle_keepalive(sock->outerhandle, value);
	}
}

void
isc__nmhandle_proxystream_setwritetimeout(isc_nmhandle_t *handle,
					  uint64_t write_timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nmhandle_setwritetimeout(sock->outerhandle, write_timeout);
	}
}

void
isc__nmsocket_proxystream_reset(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_reset(sock->outerhandle->sock);
	}
}

bool
isc__nmsocket_proxystream_timer_running(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		return isc__nmsocket_timer_running(sock->outerhandle->sock);
	}

	return false;
}

void
isc__nmsocket_proxystream_timer_restart(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_timer_restart(sock->outerhandle->sock);
	}
}

void
isc__nmsocket_proxystream_timer_stop(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_timer_stop(sock->outerhandle->sock);
	}
}

void
isc__nmhandle_proxystream_set_manual_timer(isc_nmhandle_t *handle,
					   const bool manual) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc__nmhandle_set_manual_timer(sock->outerhandle, manual);
	}
}

isc_result_t
isc__nmhandle_proxystream_set_tcp_nodelay(isc_nmhandle_t *handle,
					  const bool value) {
	isc_nmsocket_t *sock = NULL;
	isc_result_t result = ISC_R_FAILURE;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		result = isc_nmhandle_set_tcp_nodelay(sock->outerhandle, value);
	}

	return result;
}

static void
proxystream_read_start(isc_nmsocket_t *sock) {
	if (sock->proxy.reading == true) {
		return;
	}

	sock->proxy.reading = true;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nm_read(sock->outerhandle, proxystream_read_cb, sock);
	}
}

static void
proxystream_read_stop(isc_nmsocket_t *sock) {
	if (sock->proxy.reading == false) {
		return;
	}

	sock->proxy.reading = false;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nm_read_stop(sock->outerhandle);
	}
}

void
isc__nm_proxystream_read_stop(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	handle->sock->reading = false;
	proxystream_read_stop(handle->sock);
}

void
isc__nm_proxystream_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);
	REQUIRE(sock->tid == isc_tid());

	sock->closing = true;

	/*
	 * At this point we're certain that there are no
	 * external references, we can close everything.
	 */
	proxystream_read_stop(sock);
	if (sock->outerhandle != NULL) {
		sock->reading = false;
		isc_nm_read_stop(sock->outerhandle);
		isc_nmhandle_close(sock->outerhandle);
		isc_nmhandle_detach(&sock->outerhandle);
	}

	if (sock->listener != NULL) {
		isc__nmsocket_detach(&sock->listener);
	}

	/* Further cleanup performed in isc__nm_proxystream_cleanup_data() */
	sock->closed = true;
	sock->active = false;
}

static bool
proxystream_closing(isc_nmsocket_t *sock) {
	return isc__nmsocket_closing(sock) || sock->outerhandle == NULL ||
	       (sock->outerhandle != NULL &&
		isc__nmsocket_closing(sock->outerhandle->sock));
}

static void
proxystream_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	if (sock->client && sock->connect_cb != NULL && !sock->connected) {
		isc_nmhandle_t *handle = NULL;
		INSIST(sock->statichandle == NULL);
		handle = isc__nmhandle_get(sock, &sock->peer, &sock->iface);
		proxystream_call_connect_cb(sock, handle, result);
		isc__nmsocket_clearcb(sock);
		isc_nmhandle_detach(&handle);

		isc__nmsocket_prep_destroy(sock);
		return;
	}

	isc__nmsocket_timer_stop(sock);

	if (sock->statichandle == NULL) {
		isc__nmsocket_prep_destroy(sock);
		return;
	}

	/* See isc__nmsocket_readtimeout_cb() */
	if (sock->client && result == ISC_R_TIMEDOUT) {
		if (sock->recv_cb != NULL) {
			isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
			isc__nm_readcb(sock, req, result, false);
		}

		if (isc__nmsocket_timer_running(sock)) {
			/* Timer was restarted, bail-out */
			return;
		}

		isc__nmsocket_clearcb(sock);

		isc__nmsocket_prep_destroy(sock);
		return;
	}

	if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
		isc__nmsocket_clearcb(sock);
		isc__nm_readcb(sock, req, result, false);
	}

	isc__nmsocket_prep_destroy(sock);
}

static void
proxystream_read_cb(isc_nmhandle_t *handle, isc_result_t result,
		    isc_region_t *region, void *cbarg) {
	isc_nmsocket_t *proxysock = (isc_nmsocket_t *)cbarg;

	REQUIRE(VALID_NMSOCK(proxysock));
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(proxysock->tid == isc_tid());

	if (result != ISC_R_SUCCESS) {
		goto failed;

	} else if (isc__nm_closing(proxysock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
		goto failed;
	} else if (isc__nmsocket_closing(handle->sock)) {
		result = ISC_R_CANCELED;
		goto failed;
	}

	/* Handle initial PROXY header data */
	if (!proxysock->client && !proxysock->proxy.header_processed) {
		proxystream_handle_incoming_header_data(proxysock, region);
		return;
	}

	proxysock->recv_cb(proxysock->statichandle, ISC_R_SUCCESS, region,
			   proxysock->recv_cbarg);

	proxystream_try_close_unused(proxysock);

	return;
failed:
	proxystream_failed_read_cb(proxysock, result);
}

static void
proxystream_read_extra_cb(void *arg) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__nm_uvreq_t *req = arg;
	isc_region_t extra_data = { 0 }; /* data past PROXY header */

	REQUIRE(VALID_UVREQ(req));

	isc_nmsocket_t *sock = req->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	sock->proxy.extra_processed = true;

	if (isc__nm_closing(sock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
	} else if (proxystream_closing(sock)) {
		result = ISC_R_CANCELED;
	}

	if (result == ISC_R_SUCCESS) {
		extra_data.base = (uint8_t *)req->uvbuf.base;
		extra_data.length = req->uvbuf.len;

		INSIST(extra_data.length > 0);

		req->cb.recv(req->handle, result, &extra_data, req->cbarg);

		if (sock->reading) {
			proxystream_read_start(sock);
		}
	} else {
		isc__nm_proxystream_failed_read_cb(sock, result, false);
	}

	isc__nm_uvreq_put(&req);
}

void
isc__nm_proxystream_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb,
			 void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	isc_region_t extra_data = { 0 }; /* data past PROXY header */

	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);
	REQUIRE(sock->recv_handle == NULL);
	REQUIRE(sock->tid == isc_tid());

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->reading = true;

	if (isc__nm_closing(sock->worker)) {
		isc__nm_proxystream_failed_read_cb(sock, ISC_R_SHUTTINGDOWN,
						   false);
		return;
	} else if (proxystream_closing(sock)) {
		isc__nm_proxystream_failed_read_cb(sock, ISC_R_CANCELED, true);
		return;
	}

	/* check if there is extra data on the server */
	if (!sock->client && sock->proxy.header_processed &&
	    !sock->proxy.extra_processed &&
	    isc_proxy2_handler_extra(sock->proxy.proxy2.handler, &extra_data) >
		    0)
	{
		isc__nm_uvreq_t *req = isc__nm_uvreq_get(sock);
		isc_nmhandle_attach(handle, &req->handle);
		req->cb.recv = sock->recv_cb;
		req->cbarg = sock->recv_cbarg;

		req->uvbuf.base = (char *)extra_data.base;
		req->uvbuf.len = extra_data.length;

		isc_job_run(sock->worker->loop, &req->job,
			    proxystream_read_extra_cb, req);
		return;
	}

	proxystream_read_start(sock);
}

static proxystream_send_req_t *
proxystream_get_send_req(isc_mem_t *mctx, isc_nmsocket_t *sock,
			 isc_nmhandle_t *proxyhandle, isc_nm_cb_t cb,
			 void *cbarg) {
	proxystream_send_req_t *send_req = NULL;

	if (sock->proxy.send_req != NULL) {
		/*
		 * We have a previously allocated object - let's use that.
		 * That should help reducing stress on the memory allocator.
		 */
		send_req = (proxystream_send_req_t *)sock->proxy.send_req;
		sock->proxy.send_req = NULL;
	} else {
		/* Allocate a new object. */
		send_req = isc_mem_get(mctx, sizeof(*send_req));
		*send_req = (proxystream_send_req_t){ 0 };
	}

	/* Initialise the send request object */
	send_req->cb = cb;
	send_req->cbarg = cbarg;
	isc_nmhandle_attach(proxyhandle, &send_req->proxyhandle);

	sock->proxy.nsending++;

	return send_req;
}

static void
proxystream_put_send_req(isc_mem_t *mctx, proxystream_send_req_t *send_req,
			 const bool force_destroy) {
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
	}

	isc_mem_put(mctx, send_req, sizeof(*send_req));
}

static void
proxystream_send_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	proxystream_send_req_t *send_req = (proxystream_send_req_t *)cbarg;
	isc_mem_t *mctx;
	isc_nm_cb_t cb;
	void *send_cbarg;
	isc_nmhandle_t *proxyhandle = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMHANDLE(send_req->proxyhandle));
	REQUIRE(VALID_NMSOCK(send_req->proxyhandle->sock));
	REQUIRE(send_req->proxyhandle->sock->tid == isc_tid());

	mctx = send_req->proxyhandle->sock->worker->mctx;
	cb = send_req->cb;
	send_cbarg = send_req->cbarg;

	isc_nmhandle_attach(send_req->proxyhandle, &proxyhandle);
	/* try to keep the send request object for reuse */
	proxystream_put_send_req(mctx, send_req, false);
	cb(proxyhandle, result, send_cbarg);
	proxystream_try_close_unused(proxyhandle->sock);
	isc_nmhandle_detach(&proxyhandle);
}

static void
proxystream_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		 void *cbarg, const bool dnsmsg) {
	isc_nmsocket_t *sock = NULL;
	proxystream_send_req_t *send_req = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	bool fail_async = true;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_proxystreamsocket);

	if (isc__nm_closing(sock->worker)) {
		result = ISC_R_SHUTTINGDOWN;
		fail_async = false;
	} else if (proxystream_closing(sock)) {
		result = ISC_R_CANCELED;
		fail_async = true;
	}

	if (result != ISC_R_SUCCESS) {
		isc__nm_uvreq_t *uvreq = isc__nm_uvreq_get(sock);
		isc_nmhandle_attach(handle, &uvreq->handle);
		uvreq->cb.send = cb;
		uvreq->cbarg = cbarg;

		isc__nm_failed_send_cb(sock, uvreq, result, fail_async);
		return;
	}

	send_req = proxystream_get_send_req(sock->worker->mctx, sock, handle,
					    cb, cbarg);
	if (dnsmsg) {
		isc__nm_senddns(sock->outerhandle, region, proxystream_send_cb,
				send_req);
	} else {
		isc_nm_send(sock->outerhandle, region, proxystream_send_cb,
			    send_req);
	}
}

void
isc__nm_proxystream_send(isc_nmhandle_t *handle, isc_region_t *region,
			 isc_nm_cb_t cb, void *cbarg) {
	proxystream_send(handle, region, cb, cbarg, false);
}

void
isc__nm_proxystream_senddns(isc_nmhandle_t *handle, isc_region_t *region,
			    isc_nm_cb_t cb, void *cbarg) {
	proxystream_send(handle, region, cb, cbarg, true);
}

void
isc__nm_proxystream_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx) {
	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(listener->type == isc_nm_proxystreamlistener);

	if (listener->outer != NULL) {
		INSIST(VALID_NMSOCK(listener->outer));
		isc_nmsocket_set_tlsctx(listener->outer, tlsctx);
	}
}

bool
isc__nm_proxystream_has_encryption(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		return isc_nm_has_encryption(sock->outerhandle);
	}

	return false;
}

const char *
isc__nm_proxystream_verify_tls_peer_result_string(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_proxystreamsocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		return isc_nm_verify_tls_peer_result_string(sock->outerhandle);
	}

	return NULL;
}

void
isc__nmhandle_proxystream_get_selected_alpn(isc_nmhandle_t *handle,
					    const unsigned char **alpn,
					    unsigned int *alpnlen) {
	isc_nmsocket_t *sock;

	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_proxystreamsocket);
	REQUIRE(sock->tid == isc_tid());

	isc__nmhandle_get_selected_alpn(sock->outerhandle, alpn, alpnlen);
}
