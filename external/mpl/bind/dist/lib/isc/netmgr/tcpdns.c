/*	$NetBSD: tcpdns.c,v 1.3 2020/08/03 17:23:42 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
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

#define TCPDNS_CLIENTS_PER_CONN 23
/*%<
 *
 * Maximum number of simultaneous handles in flight supported for a single
 * connected TCPDNS socket. This value was chosen arbitrarily, and may be
 * changed in the future.
 */

static void
dnslisten_readcb(isc_nmhandle_t *handle, isc_region_t *region, void *arg);

static void
resume_processing(void *arg);

static void
tcpdns_close_direct(isc_nmsocket_t *sock);

static inline size_t
dnslen(unsigned char *base) {
	return ((base[0] << 8) + (base[1]));
}

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

static void
timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)uv_handle_get_data(handle);
	INSIST(VALID_NMSOCK(sock));
	isc_nmsocket_detach(&sock);
}

static void
dnstcp_readtimeout(uv_timer_t *timer) {
	isc_nmsocket_t *sock =
		(isc_nmsocket_t *)uv_handle_get_data((uv_handle_t *)timer);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());
	tcpdns_close_direct(sock);
}

/*
 * Accept callback for TCP-DNS connection.
 */
static isc_result_t
dnslisten_acceptcb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *dnslistensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *dnssock = NULL;

	REQUIRE(VALID_NMSOCK(dnslistensock));
	REQUIRE(dnslistensock->type == isc_nm_tcpdnslistener);

	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (dnslistensock->accept_cb.accept != NULL) {
		result = dnslistensock->accept_cb.accept(
			handle, ISC_R_SUCCESS, dnslistensock->accept_cbarg);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	/* We need to create a 'wrapper' dnssocket for this connection */
	dnssock = isc_mem_get(handle->sock->mgr->mctx, sizeof(*dnssock));
	isc__nmsocket_init(dnssock, handle->sock->mgr, isc_nm_tcpdnssocket,
			   handle->sock->iface);

	dnssock->extrahandlesize = dnslistensock->extrahandlesize;
	isc_nmsocket_attach(dnslistensock, &dnssock->listener);
	isc_nmsocket_attach(handle->sock, &dnssock->outer);
	dnssock->peer = handle->sock->peer;
	dnssock->read_timeout = handle->sock->mgr->init;
	dnssock->tid = isc_nm_tid();
	dnssock->closehandle_cb = resume_processing;

	uv_timer_init(&dnssock->mgr->workers[isc_nm_tid()].loop,
		      &dnssock->timer);
	dnssock->timer.data = dnssock;
	dnssock->timer_initialized = true;
	uv_timer_start(&dnssock->timer, dnstcp_readtimeout,
		       dnssock->read_timeout, 0);

	isc_nm_read(handle, dnslisten_readcb, dnssock);

	return (ISC_R_SUCCESS);
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
processbuffer(isc_nmsocket_t *dnssock, isc_nmhandle_t **handlep) {
	size_t len;

	REQUIRE(VALID_NMSOCK(dnssock));
	REQUIRE(handlep != NULL && *handlep == NULL);

	/*
	 * If we don't even have the length yet, we can't do
	 * anything.
	 */
	if (dnssock->buf_len < 2) {
		return (ISC_R_NOMORE);
	}

	/*
	 * Process the first packet from the buffer, leaving
	 * the rest (if any) for later.
	 */
	len = dnslen(dnssock->buf);
	if (len <= dnssock->buf_len - 2) {
		isc_nmhandle_t *dnshandle = isc__nmhandle_get(dnssock, NULL,
							      NULL);
		isc_nmsocket_t *listener = dnssock->listener;

		if (listener != NULL && listener->rcb.recv != NULL) {
			listener->rcb.recv(
				dnshandle,
				&(isc_region_t){ .base = dnssock->buf + 2,
						 .length = len },
				listener->rcbarg);
		}

		len += 2;
		dnssock->buf_len -= len;
		if (len > 0) {
			memmove(dnssock->buf, dnssock->buf + len,
				dnssock->buf_len);
		}

		*handlep = dnshandle;
		return (ISC_R_SUCCESS);
	}

	return (ISC_R_NOMORE);
}

/*
 * We've got a read on our underlying socket, need to check if we have
 * a complete DNS packet and, if so - call the callback
 */
static void
dnslisten_readcb(isc_nmhandle_t *handle, isc_region_t *region, void *arg) {
	isc_nmsocket_t *dnssock = (isc_nmsocket_t *)arg;
	unsigned char *base = NULL;
	bool done = false;
	size_t len;

	REQUIRE(VALID_NMSOCK(dnssock));
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(dnssock->tid == isc_nm_tid());

	if (region == NULL) {
		/* Connection closed */
		isc__nm_tcpdns_close(dnssock);
		return;
	}

	base = region->base;
	len = region->length;

	if (dnssock->buf_len + len > dnssock->buf_size) {
		alloc_dnsbuf(dnssock, dnssock->buf_len + len);
	}
	memmove(dnssock->buf + dnssock->buf_len, base, len);
	dnssock->buf_len += len;

	dnssock->read_timeout = (atomic_load(&dnssock->keepalive)
					 ? dnssock->mgr->keepalive
					 : dnssock->mgr->idle);

	do {
		isc_result_t result;
		isc_nmhandle_t *dnshandle = NULL;

		result = processbuffer(dnssock, &dnshandle);
		if (result != ISC_R_SUCCESS) {
			/*
			 * There wasn't anything in the buffer to process.
			 */
			return;
		}

		/*
		 * We have a packet: stop timeout timers
		 */
		atomic_store(&dnssock->outer->processing, true);
		if (dnssock->timer_initialized) {
			uv_timer_stop(&dnssock->timer);
		}

		if (atomic_load(&dnssock->sequential)) {
			/*
			 * We're in sequential mode and we processed
			 * one packet, so we're done until the next read
			 * completes.
			 */
			isc_nm_pauseread(dnssock->outer);
			done = true;
		} else {
			/*
			 * We're pipelining, so we now resume processing
			 * packets until the clients-per-connection limit
			 * is reached (as determined by the number of
			 * active handles on the socket). When the limit
			 * is reached, pause reading.
			 */
			if (atomic_load(&dnssock->ah) >=
			    TCPDNS_CLIENTS_PER_CONN) {
				isc_nm_pauseread(dnssock->outer);
				done = true;
			}
		}

		isc_nmhandle_unref(dnshandle);
	} while (!done);
}

/*
 * isc_nm_listentcpdns listens for connections and accepts
 * them immediately, then calls the cb for each incoming DNS packet
 * (with 2-byte length stripped) - just like for UDP packet.
 */
isc_result_t
isc_nm_listentcpdns(isc_nm_t *mgr, isc_nmiface_t *iface, isc_nm_recv_cb_t cb,
		    void *cbarg, isc_nm_accept_cb_t accept_cb,
		    void *accept_cbarg, size_t extrahandlesize, int backlog,
		    isc_quota_t *quota, isc_nmsocket_t **sockp) {
	/* A 'wrapper' socket object with outer set to true TCP socket */
	isc_nmsocket_t *dnslistensock = isc_mem_get(mgr->mctx,
						    sizeof(*dnslistensock));
	isc_result_t result;

	REQUIRE(VALID_NM(mgr));

	isc__nmsocket_init(dnslistensock, mgr, isc_nm_tcpdnslistener, iface);
	dnslistensock->rcb.recv = cb;
	dnslistensock->rcbarg = cbarg;
	dnslistensock->accept_cb.accept = accept_cb;
	dnslistensock->accept_cbarg = accept_cbarg;
	dnslistensock->extrahandlesize = extrahandlesize;

	/* We set dnslistensock->outer to a true listening socket */
	result = isc_nm_listentcp(mgr, iface, dnslisten_acceptcb, dnslistensock,
				  extrahandlesize, backlog, quota,
				  &dnslistensock->outer);
	if (result == ISC_R_SUCCESS) {
		atomic_store(&dnslistensock->listening, true);
		*sockp = dnslistensock;
		return (ISC_R_SUCCESS);
	} else {
		atomic_store(&dnslistensock->closed, true);
		isc_nmsocket_detach(&dnslistensock);
		return (result);
	}
}

void
isc__nm_tcpdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnslistener);

	atomic_store(&sock->listening, false);
	atomic_store(&sock->closed, true);
	sock->rcb.recv = NULL;
	sock->rcbarg = NULL;

	if (sock->outer != NULL) {
		isc_nm_stoplistening(sock->outer);
		isc_nmsocket_detach(&sock->outer);
	}
}

void
isc_nm_tcpdns_sequential(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	if (handle->sock->type != isc_nm_tcpdnssocket ||
	    handle->sock->outer == NULL) {
		return;
	}

	/*
	 * We don't want pipelining on this connection. That means
	 * that we need to pause after reading each request, and
	 * resume only after the request has been processed. This
	 * is done in resume_processing(), which is the socket's
	 * closehandle_cb callback, called whenever a handle
	 * is released.
	 */
	isc_nm_pauseread(handle->sock->outer);
	atomic_store(&handle->sock->sequential, true);
}

void
isc_nm_tcpdns_keepalive(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	if (handle->sock->type != isc_nm_tcpdnssocket ||
	    handle->sock->outer == NULL) {
		return;
	}

	atomic_store(&handle->sock->keepalive, true);
	atomic_store(&handle->sock->outer->keepalive, true);
}

static void
resume_processing(void *arg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)arg;
	isc_result_t result;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->type != isc_nm_tcpdnssocket || sock->outer == NULL) {
		return;
	}

	if (atomic_load(&sock->ah) == 0) {
		/* Nothing is active; sockets can timeout now */
		atomic_store(&sock->outer->processing, false);
		if (sock->timer_initialized) {
			uv_timer_start(&sock->timer, dnstcp_readtimeout,
				       sock->read_timeout, 0);
		}
	}

	/*
	 * For sequential sockets: Process what's in the buffer, or
	 * if there aren't any messages buffered, resume reading.
	 */
	if (atomic_load(&sock->sequential)) {
		isc_nmhandle_t *handle = NULL;

		result = processbuffer(sock, &handle);
		if (result == ISC_R_SUCCESS) {
			atomic_store(&sock->outer->processing, true);
			if (sock->timer_initialized) {
				uv_timer_stop(&sock->timer);
			}
			isc_nmhandle_unref(handle);
		} else if (sock->outer != NULL) {
			isc_nm_resumeread(sock->outer);
		}

		return;
	}

	/*
	 * For pipelined sockets: If we're under the clients-per-connection
	 * limit, resume processing until we reach the limit again.
	 */
	do {
		isc_nmhandle_t *dnshandle = NULL;

		result = processbuffer(sock, &dnshandle);
		if (result != ISC_R_SUCCESS) {
			/*
			 * Nothing in the buffer; resume reading.
			 */
			if (sock->outer != NULL) {
				isc_nm_resumeread(sock->outer);
			}

			break;
		}

		if (sock->timer_initialized) {
			uv_timer_stop(&sock->timer);
		}
		atomic_store(&sock->outer->processing, true);
		isc_nmhandle_unref(dnshandle);
	} while (atomic_load(&sock->ah) < TCPDNS_CLIENTS_PER_CONN);
}

static void
tcpdnssend_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc__nm_uvreq_t *req = (isc__nm_uvreq_t *)cbarg;

	UNUSED(handle);

	req->cb.send(req->handle, result, req->cbarg);
	isc_mem_put(req->sock->mgr->mctx, req->uvbuf.base, req->uvbuf.len);
	isc__nm_uvreq_put(&req, req->handle->sock);
}

void
isc__nm_async_tcpdnssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc_result_t result;
	isc__netievent_tcpdnssend_t *ievent =
		(isc__netievent_tcpdnssend_t *)ev0;
	isc__nm_uvreq_t *req = ievent->req;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(worker->id == sock->tid);

	result = ISC_R_NOTCONNECTED;
	if (atomic_load(&sock->active)) {
		isc_region_t r;

		r.base = (unsigned char *)req->uvbuf.base;
		r.length = req->uvbuf.len;
		result = isc__nm_tcp_send(sock->outer->tcphandle, &r,
					  tcpdnssend_cb, req);
	}

	if (result != ISC_R_SUCCESS) {
		req->cb.send(req->handle, result, req->cbarg);
		isc_mem_put(sock->mgr->mctx, req->uvbuf.base, req->uvbuf.len);
		isc__nm_uvreq_put(&req, sock);
	}
}

/*
 * isc__nm_tcp_send sends buf to a peer on a socket.
 */
isc_result_t
isc__nm_tcpdns_send(isc_nmhandle_t *handle, isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg) {
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	isc_nmsocket_t *sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	uvreq->handle = handle;
	isc_nmhandle_ref(uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	uvreq->uvbuf.base = isc_mem_get(sock->mgr->mctx, region->length + 2);
	uvreq->uvbuf.len = region->length + 2;
	*(uint16_t *)uvreq->uvbuf.base = htons(region->length);
	memmove(uvreq->uvbuf.base + 2, region->base, region->length);

	if (sock->tid == isc_nm_tid()) {
		isc_region_t r;

		r.base = (unsigned char *)uvreq->uvbuf.base;
		r.length = uvreq->uvbuf.len;

		return (isc__nm_tcp_send(sock->outer->tcphandle, &r,
					 tcpdnssend_cb, uvreq));
	} else {
		isc__netievent_tcpdnssend_t *ievent = NULL;

		ievent = isc__nm_get_ievent(sock->mgr, netievent_tcpdnssend);
		ievent->req = uvreq;
		ievent->sock = sock;

		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);

		return (ISC_R_SUCCESS);
	}

	return (ISC_R_UNEXPECTED);
}

static void
tcpdns_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(sock->tid == isc_nm_tid());
	/* We don't need atomics here, it's all in single network thread */

	if (sock->timer_initialized) {
		/*
		 * We need to fire the timer callback to clean it up,
		 * it will then call us again (via detach) so that we
		 * can finally close the socket.
		 */
		sock->timer_initialized = false;
		uv_timer_stop(&sock->timer);
		uv_close((uv_handle_t *)&sock->timer, timer_close_cb);
	} else {
		/*
		 * At this point we're certain that there are no external
		 * references, we can close everything.
		 */
		if (sock->outer != NULL) {
			sock->outer->rcb.recv = NULL;
			isc_nmsocket_detach(&sock->outer);
		}
		if (sock->listener != NULL) {
			isc_nmsocket_detach(&sock->listener);
		}
		atomic_store(&sock->closed, true);
	}
}

void
isc__nm_tcpdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tcpdnssocket);

	if (sock->tid == isc_nm_tid()) {
		tcpdns_close_direct(sock);
	} else {
		isc__netievent_tcpdnsclose_t *ievent =
			isc__nm_get_ievent(sock->mgr, netievent_tcpdnsclose);

		ievent->sock = sock;
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tcpdnsclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tcpdnsclose_t *ievent =
		(isc__netievent_tcpdnsclose_t *)ev0;

	REQUIRE(worker->id == ievent->sock->tid);

	tcpdns_close_direct(ievent->sock);
}
