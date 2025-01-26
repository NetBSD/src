/*	$NetBSD: streamdns.c,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#include <limits.h>
#include <unistd.h>

#include <isc/async.h>
#include <isc/atomic.h>
#include <isc/result.h>
#include <isc/thread.h>

#include "netmgr-int.h"

/*
 * Stream DNS is a unified transport capable of serving both DNS over
 * TCP and DNS over TLS.  It is built on top of
 * 'isc_dnsstream_assembler_t' which is used for assembling DNS
 * messages in the format used for DNS over TCP out of incoming data.
 * It is built on top of 'isc_buffer_t' optimised for small (>= 512
 * bytes) DNS messages. For small messages it uses a small static
 * memory buffer, but it can automatically switch to a larger
 * dynamically allocated memory buffer for larger ones. This way we
 * avoid unnecessary memory allocation requests in most cases, as most
 * DNS messages are small.
 *
 * The use of 'isc_dnsstream_assembler_t' allows decoupling DNS
 * message assembling code from networking code itself, making it
 * easier to test.
 *
 * To understand how the part responsible for reading of data works,
 * start by looking at 'streamdns_on_dnsmessage_data_cb()' (the DNS
 * message data processing callback) and
 * 'streamdns_handle_incoming_data()' which passes incoming data to
 * the 'isc_dnsstream_assembler_t' object within the socket.
 *
 * The writing is done in a simpler manner due to the fact that we
 * have full control over the data. For each write request we attempt
 * to allocate a 'streamdns_send_req_t' structure, whose main purpose
 * is to keep the data required for the send request processing.
 *
 * When processing write requests there is an important optimisation:
 * we attempt to reuse 'streamdns_send_req_t' objects again, in order
 * to avoid memory allocations when requesting memory for the new
 * 'streamdns_send_req_t' object.
 *
 * To understand how sending is done, start by looking at
 * 'isc__nm_streamdns_send()'. Additionally also take a look at
 * 'streamdns_get_send_req()' and 'streamdns_put_send_req()' which are
 * responsible for send requests allocation/reuse and initialisation.
 *
 * The rest of the code is mostly wrapping code to expose the
 * functionality of the underlying transport, which at the moment
 * could be either TCP or TLS.
 */

typedef struct streamdns_send_req {
	isc_nm_cb_t cb;		   /* send callback */
	void *cbarg;		   /* send callback argument */
	isc_nmhandle_t *dnshandle; /* Stream DNS socket handle */
} streamdns_send_req_t;

static streamdns_send_req_t *
streamdns_get_send_req(isc_nmsocket_t *sock, isc_mem_t *mctx,
		       isc__nm_uvreq_t *req);

static void
streamdns_put_send_req(isc_mem_t *mctx, streamdns_send_req_t *send_req,
		       const bool force_destroy);

static void
streamdns_readcb(isc_nmhandle_t *handle, isc_result_t result,
		 isc_region_t *region, void *cbarg);

static void
streamdns_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result,
			 const bool async);

static void
streamdns_try_close_unused(isc_nmsocket_t *sock);

static bool
streamdns_closing(isc_nmsocket_t *sock);

static void
streamdns_resume_processing(void *arg);

static void
streamdns_resumeread(isc_nmsocket_t *sock, isc_nmhandle_t *transphandle) {
	if (!sock->streamdns.reading) {
		sock->streamdns.reading = true;
		isc_nm_read(transphandle, streamdns_readcb, (void *)sock);
	}
}

static void
streamdns_readmore(isc_nmsocket_t *sock, isc_nmhandle_t *transphandle) {
	streamdns_resumeread(sock, transphandle);

	/* Restart the timer only if there's a last single active handle */
	isc_nmhandle_t *handle = ISC_LIST_HEAD(sock->active_handles);
	INSIST(handle != NULL);
	if (ISC_LIST_NEXT(handle, active_link) == NULL) {
		isc__nmsocket_timer_start(sock);
	}
}

static void
streamdns_pauseread(isc_nmsocket_t *sock, isc_nmhandle_t *transphandle) {
	if (sock->streamdns.reading) {
		sock->streamdns.reading = false;
		isc_nm_read_stop(transphandle);
	}
}

static bool
streamdns_on_complete_dnsmessage(isc_dnsstream_assembler_t *dnsasm,
				 isc_region_t *restrict region,
				 isc_nmsocket_t *sock,
				 isc_nmhandle_t *transphandle) {
	const bool last_datum = isc_dnsstream_assembler_remaininglength(
					dnsasm) == region->length;
	/*
	 * Stop after one message if a client connection.
	 */
	bool stop = sock->client;

	sock->reading = false;
	if (sock->recv_cb != NULL) {
		if (!sock->client) {
			/*
			 * We must allocate a new handle object, as we
			 * need to ensure that after processing of this
			 * message has been completed and the handle
			 * gets destroyed, 'nsock->closehandle_cb'
			 * (streamdns_resume_processing()) is invoked.
			 * That is required for pipelining support.
			 */
			isc_nmhandle_t *handle = isc__nmhandle_get(
				sock, &sock->peer, &sock->iface);
			sock->recv_cb(handle, ISC_R_SUCCESS, region,
				      sock->recv_cbarg);
			isc_nmhandle_detach(&handle);
		} else {
			/*
			 * As on the client side we are supposed to stop
			 * reading/processing after receiving one
			 * message, we can use the 'sock->recv_handle'
			 * from which we would need to detach before
			 * calling the read callback anyway.
			 */
			isc_nmhandle_t *recv_handle = sock->recv_handle;
			sock->recv_handle = NULL;
			sock->recv_cb(recv_handle, ISC_R_SUCCESS, region,
				      sock->recv_cbarg);
			isc_nmhandle_detach(&recv_handle);
		}

		if (streamdns_closing(sock)) {
			stop = true;
		}
	} else {
		stop = true;
	}

	if (sock->active_handles_max != 0 &&
	    (sock->active_handles_cur >= sock->active_handles_max))
	{
		stop = true;
	}
	INSIST(sock->active_handles_cur <= sock->active_handles_max);

	isc__nmsocket_timer_stop(sock);
	if (stop) {
		streamdns_pauseread(sock, transphandle);
	} else if (last_datum) {
		/*
		 * We have processed all data, need to read more.
		 * The call also restarts the timer.
		 */
		streamdns_readmore(sock, transphandle);
	} else {
		/*
		 * Process more DNS messages in the next loop tick.
		 */
		streamdns_pauseread(sock, transphandle);
		isc_async_run(sock->worker->loop, streamdns_resume_processing,
			      sock);
	}

	return false;
}

/*
 * This function, alongside 'streamdns_handle_incoming_data()',
 * connects networking code to the 'isc_dnsstream_assembler_t'. It is
 * responsible for making decisions regarding reading from the
 * underlying transport socket as well as controlling the read timer.
 */
static bool
streamdns_on_dnsmessage_data_cb(isc_dnsstream_assembler_t *dnsasm,
				const isc_result_t result,
				isc_region_t *restrict region, void *cbarg,
				void *userarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;
	isc_nmhandle_t *transphandle = (isc_nmhandle_t *)userarg;

	switch (result) {
	case ISC_R_SUCCESS:
		/*
		 * A complete DNS message has been assembled from the incoming
		 * data. Let's process it.
		 */
		return streamdns_on_complete_dnsmessage(dnsasm, region, sock,
							transphandle);
	case ISC_R_RANGE:
		/*
		 * It seems that someone attempts to send us some binary junk
		 * over the socket, as the beginning of the next message tells
		 * us the there is an empty (0-sized) DNS message to receive.
		 * We should treat it as a hard error.
		 */
		streamdns_failed_read_cb(sock, result, false);
		return false;
	case ISC_R_NOMORE:
		/*
		 * We do not have enough data to process the next message and
		 * thus we need to resume reading from the socket.
		 */
		if (sock->recv_handle != NULL) {
			streamdns_readmore(sock, transphandle);
		}
		return false;
	default:
		UNREACHABLE();
	};
}

static void
streamdns_handle_incoming_data(isc_nmsocket_t *sock,
			       isc_nmhandle_t *transphandle,
			       void *restrict data, size_t len) {
	isc_dnsstream_assembler_t *dnsasm = sock->streamdns.input;

	/*
	 * Try to process the received data or, when 'data == NULL' and
	 * 'len == 0', try to resume processing of the data within the
	 * internal buffers or resume reading, if there is no any.
	 */
	isc_dnsstream_assembler_incoming(dnsasm, transphandle, data, len);
	streamdns_try_close_unused(sock);
}

static isc_nmsocket_t *
streamdns_sock_new(isc__networker_t *worker, const isc_nmsocket_type_t type,
		   isc_sockaddr_t *addr, const bool is_server) {
	isc_nmsocket_t *sock;
	INSIST(type == isc_nm_streamdnssocket ||
	       type == isc_nm_streamdnslistener);

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, type, addr, NULL);
	sock->result = ISC_R_UNSET;
	if (type == isc_nm_streamdnssocket) {
		uint32_t initial = 0;
		isc_nm_gettimeouts(worker->netmgr, &initial, NULL, NULL, NULL);
		sock->read_timeout = initial;
		sock->client = !is_server;
		sock->connecting = !is_server;
		sock->streamdns.input = isc_dnsstream_assembler_new(
			sock->worker->mctx, streamdns_on_dnsmessage_data_cb,
			sock);
	}

	return sock;
}

static void
streamdns_call_connect_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
			  const isc_result_t result) {
	sock->connecting = false;
	INSIST(sock->connect_cb != NULL);
	sock->connect_cb(handle, result, sock->connect_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc__nmsocket_clearcb(handle->sock);
	} else {
		sock->connected = true;
	}
	streamdns_try_close_unused(sock);
}

static void
streamdns_save_alpn_status(isc_nmsocket_t *dnssock,
			   isc_nmhandle_t *transp_handle) {
	const unsigned char *alpn = NULL;
	unsigned int alpnlen = 0;

	isc__nmhandle_get_selected_alpn(transp_handle, &alpn, &alpnlen);
	if (alpn != NULL && alpnlen == ISC_TLS_DOT_PROTO_ALPN_ID_LEN &&
	    memcmp(ISC_TLS_DOT_PROTO_ALPN_ID, alpn,
		   ISC_TLS_DOT_PROTO_ALPN_ID_LEN) == 0)
	{
		dnssock->streamdns.dot_alpn_negotiated = true;
	}
}

static void
streamdns_transport_connected(isc_nmhandle_t *handle, isc_result_t result,
			      void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;
	isc_nmhandle_t *streamhandle = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	sock->tid = isc_tid();
	if (result == ISC_R_EOF) {
		/*
		 * The transport layer (probably TLS) has returned EOF during
		 * connection establishment. That means that connection has
		 * been "cancelled" (for compatibility with old transport
		 * behaviour).
		 */
		result = ISC_R_CANCELED;
		goto error;
	} else if (result == ISC_R_TLSERROR) {
		/*
		 * In some of the cases when the old code would return
		 * ISC_R_CANCELLED, the new code could return generic
		 * ISC_R_TLSERROR code. However, the old code does not expect
		 * that.
		 */
		result = ISC_R_CANCELED;
		goto error;
	} else if (result != ISC_R_SUCCESS) {
		goto error;
	}

	INSIST(VALID_NMHANDLE(handle));

	sock->iface = isc_nmhandle_localaddr(handle);
	sock->peer = isc_nmhandle_peeraddr(handle);
	if (isc__nmsocket_closing(handle->sock)) {
		result = ISC_R_SHUTTINGDOWN;
		goto error;
	}

	isc_nmhandle_attach(handle, &sock->outerhandle);
	sock->active = true;

	handle->sock->streamdns.sock = sock;

	streamdns_save_alpn_status(sock, handle);
	isc__nmhandle_set_manual_timer(sock->outerhandle, true);
	streamhandle = isc__nmhandle_get(sock, &sock->peer, &sock->iface);
	(void)isc_nmhandle_set_tcp_nodelay(sock->outerhandle, true);
	streamdns_call_connect_cb(sock, streamhandle, result);
	isc_nmhandle_detach(&streamhandle);

	return;
error:
	if (handle != NULL) {
		/*
		 * Let's save the error description (if any) so that
		 * e.g. 'dig' could produce a usable error message.
		 */
		INSIST(VALID_NMHANDLE(handle));
		sock->streamdns.tls_verify_error =
			isc_nm_verify_tls_peer_result_string(handle);
	}
	streamhandle = isc__nmhandle_get(sock, NULL, NULL);
	sock->closed = true;
	streamdns_call_connect_cb(sock, streamhandle, result);
	isc_nmhandle_detach(&streamhandle);
	isc__nmsocket_detach(&sock);
}

void
isc_nm_streamdnsconnect(isc_nm_t *mgr, isc_sockaddr_t *local,
			isc_sockaddr_t *peer, isc_nm_cb_t cb, void *cbarg,
			unsigned int timeout, isc_tlsctx_t *tlsctx,
			isc_tlsctx_client_session_cache_t *client_sess_cache,
			isc_nm_proxy_type_t proxy_type,
			isc_nm_proxyheader_info_t *proxy_info) {
	isc_nmsocket_t *nsock = NULL;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NM(mgr));

	worker = &mgr->workers[isc_tid()];

	if (isc__nm_closing(worker)) {
		cb(NULL, ISC_R_SHUTTINGDOWN, cbarg);
		return;
	}

	nsock = streamdns_sock_new(worker, isc_nm_streamdnssocket, local,
				   false);
	nsock->connect_cb = cb;
	nsock->connect_cbarg = cbarg;
	nsock->connect_timeout = timeout;

	switch (proxy_type) {
	case ISC_NM_PROXY_NONE:
		if (tlsctx == NULL) {
			INSIST(client_sess_cache == NULL);
			isc_nm_tcpconnect(mgr, local, peer,
					  streamdns_transport_connected, nsock,
					  nsock->connect_timeout);
		} else {
			isc_nm_tlsconnect(
				mgr, local, peer, streamdns_transport_connected,
				nsock, tlsctx, client_sess_cache,
				nsock->connect_timeout, false, proxy_info);
		}
		break;
	case ISC_NM_PROXY_PLAIN:
		if (tlsctx == NULL) {
			isc_nm_proxystreamconnect(mgr, local, peer,
						  streamdns_transport_connected,
						  nsock, nsock->connect_timeout,
						  NULL, NULL, proxy_info);
		} else {
			isc_nm_tlsconnect(
				mgr, local, peer, streamdns_transport_connected,
				nsock, tlsctx, client_sess_cache,
				nsock->connect_timeout, true, proxy_info);
		}
		break;
	case ISC_NM_PROXY_ENCRYPTED:
		INSIST(tlsctx != NULL);
		isc_nm_proxystreamconnect(mgr, local, peer,
					  streamdns_transport_connected, nsock,
					  nsock->connect_timeout, tlsctx,
					  client_sess_cache, proxy_info);
		break;
	default:
		UNREACHABLE();
	}
}

bool
isc__nmsocket_streamdns_timer_running(isc_nmsocket_t *sock) {
	isc_nmsocket_t *transp_sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);

	if (sock->outerhandle == NULL) {
		return false;
	}

	INSIST(VALID_NMHANDLE(sock->outerhandle));
	transp_sock = sock->outerhandle->sock;
	INSIST(VALID_NMSOCK(transp_sock));

	return isc__nmsocket_timer_running(transp_sock);
}

void
isc__nmsocket_streamdns_timer_stop(isc_nmsocket_t *sock) {
	isc_nmsocket_t *transp_sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);

	if (sock->outerhandle == NULL) {
		return;
	}

	INSIST(VALID_NMHANDLE(sock->outerhandle));
	transp_sock = sock->outerhandle->sock;
	INSIST(VALID_NMSOCK(transp_sock));

	isc__nmsocket_timer_stop(transp_sock);
}

void
isc__nmsocket_streamdns_timer_restart(isc_nmsocket_t *sock) {
	isc_nmsocket_t *transp_sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);

	if (sock->outerhandle == NULL) {
		return;
	}

	INSIST(VALID_NMHANDLE(sock->outerhandle));
	transp_sock = sock->outerhandle->sock;
	INSIST(VALID_NMSOCK(transp_sock));

	isc__nmsocket_timer_restart(transp_sock);
}

static void
streamdns_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result,
			 const bool async) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	/* Nobody is reading from the socket yet */
	if (sock->recv_handle == NULL) {
		goto destroy;
	}

	if (sock->client && result == ISC_R_TIMEDOUT) {
		if (sock->recv_cb != NULL) {
			isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
			isc__nm_readcb(sock, req, ISC_R_TIMEDOUT, false);
		}

		if (isc__nmsocket_timer_running(sock)) {
			/* Timer was restarted, bail-out */
			return;
		}

		isc__nmsocket_clearcb(sock);

		goto destroy;
	}

	isc_dnsstream_assembler_clear(sock->streamdns.input);

	/* Nobody expects the callback if isc_nm_read() wasn't called */
	if (!sock->client || sock->reading) {
		sock->reading = false;

		if (sock->recv_cb != NULL) {
			isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
			isc__nmsocket_clearcb(sock);
			isc__nm_readcb(sock, req, result, async);
		}
	}

destroy:
	isc__nmsocket_prep_destroy(sock);
}

void
isc__nm_streamdns_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
				 const bool async) {
	REQUIRE(result != ISC_R_SUCCESS);
	REQUIRE(sock->type == isc_nm_streamdnssocket);
	sock->streamdns.reading = false;
	streamdns_failed_read_cb(sock, result, async);
}

static void
streamdns_readcb(isc_nmhandle_t *handle, isc_result_t result,
		 isc_region_t *region, void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	if (result != ISC_R_SUCCESS) {
		streamdns_failed_read_cb(sock, result, false);
		return;
	} else if (streamdns_closing(sock)) {
		streamdns_failed_read_cb(sock, ISC_R_CANCELED, false);
		return;
	}

	streamdns_handle_incoming_data(sock, handle, region->base,
				       region->length);
}

static void
streamdns_try_close_unused(isc_nmsocket_t *sock) {
	if (sock->recv_handle == NULL && sock->streamdns.nsending == 0) {
		/*
		 * The socket is unused after calling the callback. Let's close
		 * the underlying connection.
		 */
		/* FIXME: call failed_read_cb(?) */
		if (sock->outerhandle != NULL) {
			isc_nmhandle_detach(&sock->outerhandle);
		}
		isc__nmsocket_prep_destroy(sock);
	}
}

static streamdns_send_req_t *
streamdns_get_send_req(isc_nmsocket_t *sock, isc_mem_t *mctx,
		       isc__nm_uvreq_t *req) {
	streamdns_send_req_t *send_req;

	if (sock->streamdns.send_req != NULL) {
		/*
		 * We have a previously allocated object - let's use that.
		 * That should help reducing stress on the memory allocator.
		 */
		send_req = (streamdns_send_req_t *)sock->streamdns.send_req;
		sock->streamdns.send_req = NULL;
	} else {
		/* Allocate a new object. */
		send_req = isc_mem_get(mctx, sizeof(*send_req));
		*send_req = (streamdns_send_req_t){ 0 };
	}

	/* Initialise the send request object */
	send_req->cb = req->cb.send;
	send_req->cbarg = req->cbarg;
	isc_nmhandle_attach(req->handle, &send_req->dnshandle);

	sock->streamdns.nsending++;

	return send_req;
}

static void
streamdns_put_send_req(isc_mem_t *mctx, streamdns_send_req_t *send_req,
		       const bool force_destroy) {
	/*
	 * Attempt to put the object for reuse later if we are not
	 * wrapping up.
	 */
	if (!force_destroy) {
		isc_nmsocket_t *sock = send_req->dnshandle->sock;
		sock->streamdns.nsending--;
		isc_nmhandle_detach(&send_req->dnshandle);
		if (sock->streamdns.send_req == NULL) {
			sock->streamdns.send_req = send_req;
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
streamdns_writecb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	streamdns_send_req_t *send_req = (streamdns_send_req_t *)cbarg;
	isc_mem_t *mctx;
	isc_nm_cb_t cb;
	void *send_cbarg;
	isc_nmhandle_t *dnshandle = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMHANDLE(send_req->dnshandle));
	REQUIRE(VALID_NMSOCK(send_req->dnshandle->sock));
	REQUIRE(send_req->dnshandle->sock->tid == isc_tid());

	mctx = send_req->dnshandle->sock->worker->mctx;
	cb = send_req->cb;
	send_cbarg = send_req->cbarg;

	isc_nmhandle_attach(send_req->dnshandle, &dnshandle);
	/* try to keep the send request object for reuse */
	streamdns_put_send_req(mctx, send_req, false);
	cb(dnshandle, result, send_cbarg);
	streamdns_try_close_unused(dnshandle->sock);
	isc_nmhandle_detach(&dnshandle);
}

static bool
streamdns_closing(isc_nmsocket_t *sock) {
	return isc__nmsocket_closing(sock) || isc__nm_closing(sock->worker) ||
	       sock->outerhandle == NULL ||
	       (sock->outerhandle != NULL &&
		isc__nmsocket_closing(sock->outerhandle->sock));
}

static void
streamdns_resume_processing(void *arg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(!sock->client);

	if (streamdns_closing(sock)) {
		return;
	}

	if (sock->active_handles_max != 0 &&
	    (sock->active_handles_cur >= sock->active_handles_max))
	{
		return;
	}

	streamdns_handle_incoming_data(sock, sock->outerhandle, NULL, 0);
}

static isc_result_t
streamdns_accept_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *listensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *nsock;
	isc_sockaddr_t iface;
	int tid = isc_tid();
	uint32_t initial = 0;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	if (isc__nm_closing(handle->sock->worker)) {
		return ISC_R_SHUTTINGDOWN;
	} else if (result != ISC_R_SUCCESS) {
		return result;
	}

	REQUIRE(VALID_NMSOCK(listensock));
	REQUIRE(listensock->type == isc_nm_streamdnslistener);

	iface = isc_nmhandle_localaddr(handle);
	nsock = streamdns_sock_new(handle->sock->worker, isc_nm_streamdnssocket,
				   &iface, true);
	nsock->recv_cb = listensock->recv_cb;
	nsock->recv_cbarg = listensock->recv_cbarg;

	nsock->peer = isc_nmhandle_peeraddr(handle);
	nsock->tid = tid;
	isc_nm_gettimeouts(handle->sock->worker->netmgr, &initial, NULL, NULL,
			   NULL);
	nsock->read_timeout = initial;
	nsock->accepting = true;
	nsock->active = true;

	isc__nmsocket_attach(handle->sock, &nsock->listener);
	isc_nmhandle_attach(handle, &nsock->outerhandle);
	handle->sock->streamdns.sock = nsock;

	streamdns_save_alpn_status(nsock, handle);

	nsock->recv_handle = isc__nmhandle_get(nsock, NULL, &iface);
	INSIST(listensock->accept_cb != NULL);
	result = listensock->accept_cb(nsock->recv_handle, result,
				       listensock->accept_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc_nmhandle_detach(&nsock->recv_handle);
		isc__nmsocket_detach(&nsock->listener);
		isc_nmhandle_detach(&nsock->outerhandle);
		nsock->closed = true;
		goto exit;
	}

	nsock->closehandle_cb = streamdns_resume_processing;
	isc__nmhandle_set_manual_timer(nsock->outerhandle, true);
	isc_nm_gettimeouts(nsock->worker->netmgr, &initial, NULL, NULL, NULL);
	/* settimeout restarts the timer */
	isc_nmhandle_settimeout(nsock->outerhandle, initial);
	(void)isc_nmhandle_set_tcp_nodelay(nsock->outerhandle, true);
	streamdns_handle_incoming_data(nsock, nsock->outerhandle, NULL, 0);

exit:
	nsock->accepting = false;

	return result;
}

isc_result_t
isc_nm_listenstreamdns(isc_nm_t *mgr, uint32_t workers, isc_sockaddr_t *iface,
		       isc_nm_recv_cb_t recv_cb, void *recv_cbarg,
		       isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		       int backlog, isc_quota_t *quota, isc_tlsctx_t *tlsctx,
		       isc_nm_proxy_type_t proxy_type, isc_nmsocket_t **sockp) {
	isc_result_t result = ISC_R_FAILURE;
	isc_nmsocket_t *listener = NULL;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(isc_tid() == 0);

	worker = &mgr->workers[isc_tid()];

	if (isc__nm_closing(worker)) {
		return ISC_R_SHUTTINGDOWN;
	}

	listener = streamdns_sock_new(worker, isc_nm_streamdnslistener, iface,
				      true);
	listener->accept_cb = accept_cb;
	listener->accept_cbarg = accept_cbarg;
	listener->recv_cb = recv_cb;
	listener->recv_cbarg = recv_cbarg;

	switch (proxy_type) {
	case ISC_NM_PROXY_NONE:
		if (tlsctx == NULL) {
			result = isc_nm_listentcp(
				mgr, workers, iface, streamdns_accept_cb,
				listener, backlog, quota, &listener->outer);
		} else {
			result = isc_nm_listentls(mgr, workers, iface,
						  streamdns_accept_cb, listener,
						  backlog, quota, tlsctx, false,
						  &listener->outer);
		}
		break;
	case ISC_NM_PROXY_PLAIN:
		if (tlsctx == NULL) {
			result = isc_nm_listenproxystream(
				mgr, workers, iface, streamdns_accept_cb,
				listener, backlog, quota, NULL,
				&listener->outer);
		} else {
			result = isc_nm_listentls(mgr, workers, iface,
						  streamdns_accept_cb, listener,
						  backlog, quota, tlsctx, true,
						  &listener->outer);
		}
		break;
	case ISC_NM_PROXY_ENCRYPTED:
		INSIST(tlsctx != NULL);
		result = isc_nm_listenproxystream(
			mgr, workers, iface, streamdns_accept_cb, listener,
			backlog, quota, tlsctx, &listener->outer);
		break;
	default:
		UNREACHABLE();
	};

	if (result != ISC_R_SUCCESS) {
		listener->closed = true;
		isc__nmsocket_detach(&listener);
		return result;
	}

	/* copy the actual port we're listening on into sock->iface */
	if (isc_sockaddr_getport(iface) == 0) {
		listener->iface = listener->outer->iface;
	}

	listener->result = result;
	listener->active = true;
	INSIST(listener->outer->streamdns.listener == NULL);
	listener->nchildren = listener->outer->nchildren;
	isc__nmsocket_attach(listener, &listener->outer->streamdns.listener);

	*sockp = listener;

	return result;
}

void
isc__nm_streamdns_cleanup_data(isc_nmsocket_t *sock) {
	switch (sock->type) {
	case isc_nm_streamdnssocket:
		isc_dnsstream_assembler_free(&sock->streamdns.input);
		INSIST(sock->streamdns.nsending == 0);
		if (sock->streamdns.send_req != NULL) {
			isc_mem_t *mctx = sock->worker->mctx;
			streamdns_put_send_req(mctx,
					       (streamdns_send_req_t *)
						       sock->streamdns.send_req,
					       true);
		}
		break;
	case isc_nm_streamdnslistener:
		if (sock->outer) {
			isc__nmsocket_detach(&sock->outer);
		}
		break;
	case isc_nm_tlslistener:
	case isc_nm_tcplistener:
	case isc_nm_proxystreamlistener:
		if (sock->streamdns.listener != NULL) {
			isc__nmsocket_detach(&sock->streamdns.listener);
		}
		break;
	case isc_nm_tlssocket:
	case isc_nm_tcpsocket:
	case isc_nm_proxystreamsocket:
		if (sock->streamdns.sock != NULL) {
			isc__nmsocket_detach(&sock->streamdns.sock);
		}
		break;
	default:
		return;
	}
}

static void
streamdns_read_cb(void *arg) {
	isc_nmsocket_t *sock = arg;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	if (streamdns_closing(sock)) {
		streamdns_failed_read_cb(sock, ISC_R_CANCELED, false);
		goto detach;
	}

	if (sock->streamdns.reading) {
		goto detach;
	}

	INSIST(VALID_NMHANDLE(sock->outerhandle));
	streamdns_handle_incoming_data(sock, sock->outerhandle, NULL, 0);
detach:
	isc__nmsocket_detach(&sock);
}

void
isc__nm_streamdns_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb,
		       void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	bool closing = false;

	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);
	REQUIRE(sock->recv_handle == handle || sock->recv_handle == NULL);
	REQUIRE(sock->tid == isc_tid());

	closing = streamdns_closing(sock);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->reading = true;
	if (sock->recv_handle == NULL) {
		isc_nmhandle_attach(handle, &sock->recv_handle);
	}

	/*
	 * In some cases there is little sense in making the operation
	 * asynchronous as we just want to start reading from the
	 * underlying transport.
	 */
	if (!closing && isc_dnsstream_assembler_result(sock->streamdns.input) ==
				ISC_R_UNSET)
	{
		isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });
		streamdns_read_cb(sock);
		return;
	}

	/*
	 * We want the read operation to be asynchronous in most cases
	 * because:
	 *
	 * 1. A read operation might be initiated from within the read
	 *    callback itself.
	 *
	 * 2. Due to the above, we need to make the operation
	 *    asynchronous to keep the socket state consistent.
	 */

	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });
	isc_job_run(sock->worker->loop, &sock->job, streamdns_read_cb, sock);
}

void
isc__nm_streamdns_send(isc_nmhandle_t *handle, const isc_region_t *region,
		       isc_nm_cb_t cb, void *cbarg) {
	isc__nm_uvreq_t *uvreq = NULL;
	isc_nmsocket_t *sock = NULL;
	streamdns_send_req_t *send_req;
	isc_mem_t *mctx;
	isc_region_t data = { 0 };

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(region->length <= UINT16_MAX);

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_streamdnssocket);
	REQUIRE(sock->tid == isc_tid());

	uvreq = isc__nm_uvreq_get(sock);
	isc_nmhandle_attach(handle, &uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	if (streamdns_closing(sock)) {
		isc__nm_failed_send_cb(sock, uvreq, ISC_R_CANCELED, true);
		return;
	}

	/*
	 * As when sending, we, basically, handing data to the underlying
	 * transport, we can treat the operation synchronously, as the
	 * transport code will take care of the asynchronicity if required.
	 */
	mctx = sock->worker->mctx;
	send_req = streamdns_get_send_req(sock, mctx, uvreq);
	data.base = (unsigned char *)uvreq->uvbuf.base;
	data.length = uvreq->uvbuf.len;
	isc__nm_senddns(sock->outerhandle, &data, streamdns_writecb,
			(void *)send_req);

	isc__nm_uvreq_put(&uvreq);
}

static void
streamdns_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	if (sock->outerhandle != NULL) {
		sock->streamdns.reading = false;
		isc_nm_read_stop(sock->outerhandle);
		isc_nmhandle_close(sock->outerhandle);
		isc_nmhandle_detach(&sock->outerhandle);
	}

	if (sock->listener != NULL) {
		isc__nmsocket_detach(&sock->listener);
	}

	if (sock->recv_handle != NULL) {
		isc_nmhandle_detach(&sock->recv_handle);
	}

	/* Further cleanup performed in isc__nm_streamdns_cleanup_data() */
	isc_dnsstream_assembler_clear(sock->streamdns.input);
	sock->closed = true;
	sock->active = false;
}

void
isc__nm_streamdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(!sock->closing);

	sock->closing = true;

	streamdns_close_direct(sock);
}

void
isc__nm_streamdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnslistener);

	isc__nmsocket_stop(sock);
}

void
isc__nmhandle_streamdns_cleartimeout(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_streamdnssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_cleartimeout(sock->outerhandle);
	}
}

void
isc__nmhandle_streamdns_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_streamdnssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_settimeout(sock->outerhandle, timeout);
	}
}

void
isc__nmhandle_streamdns_keepalive(isc_nmhandle_t *handle, bool value) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_streamdnssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_keepalive(sock->outerhandle, value);
	}
}

void
isc__nmhandle_streamdns_setwritetimeout(isc_nmhandle_t *handle,
					uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_streamdnssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_setwritetimeout(sock->outerhandle, timeout);
	}
}

bool
isc__nm_streamdns_has_encryption(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_streamdnssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		return isc_nm_has_encryption(sock->outerhandle);
	}

	return false;
}

const char *
isc__nm_streamdns_verify_tls_peer_result_string(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_streamdnssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		return isc_nm_verify_tls_peer_result_string(sock->outerhandle);
	} else if (sock->streamdns.tls_verify_error != NULL) {
		return sock->streamdns.tls_verify_error;
	}

	return NULL;
}

void
isc__nm_streamdns_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx) {
	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(listener->type == isc_nm_streamdnslistener);

	if (listener->outer != NULL) {
		INSIST(VALID_NMSOCK(listener->outer));
		isc_nmsocket_set_tlsctx(listener->outer, tlsctx);
	}
}

isc_result_t
isc__nm_streamdns_xfr_checkperm(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_NOPERM;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);

	if (sock->outerhandle != NULL) {
		if (isc_nm_has_encryption(sock->outerhandle) &&
		    !sock->streamdns.dot_alpn_negotiated)
		{
			result = ISC_R_DOTALPNERROR;
		} else {
			result = ISC_R_SUCCESS;
		}
	}

	return result;
}

void
isc__nmsocket_streamdns_reset(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_streamdnssocket);

	if (sock->outerhandle == NULL) {
		return;
	}

	INSIST(VALID_NMHANDLE(sock->outerhandle));
	isc__nmsocket_reset(sock->outerhandle->sock);
}
