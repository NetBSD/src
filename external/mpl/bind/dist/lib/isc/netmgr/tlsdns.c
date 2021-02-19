/*	$NetBSD: tlsdns.c,v 1.1.1.1 2021/02/19 16:37:17 christos Exp $	*/

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

#define TLSDNS_CLIENTS_PER_CONN 23
/*%<
 *
 * Maximum number of simultaneous handles in flight supported for a single
 * connected TLSDNS socket. This value was chosen arbitrarily, and may be
 * changed in the future.
 */

static void
dnslisten_readcb(isc_nmhandle_t *handle, isc_result_t eresult,
		 isc_region_t *region, void *arg);

static void
resume_processing(void *arg);

static void
tlsdns_close_direct(isc_nmsocket_t *sock);

static inline size_t
dnslen(unsigned char *base) {
	return ((base[0] << 8) + (base[1]));
}

/*
 * COMPAT CODE
 */

static void *
isc__nm_get_ievent(isc_nm_t *mgr, isc__netievent_type type) {
	isc__netievent_storage_t *event = isc_mempool_get(mgr->evpool);

	*event = (isc__netievent_storage_t){ .ni.type = type };
	return (event);
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

	REQUIRE(VALID_NMSOCK(sock));

	atomic_store(&sock->closed, true);
	tlsdns_close_direct(sock);
}

static void
dnstcp_readtimeout(uv_timer_t *timer) {
	isc_nmsocket_t *sock =
		(isc_nmsocket_t *)uv_handle_get_data((uv_handle_t *)timer);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	/* Close the TCP connection; its closure should fire ours. */
	if (sock->outerhandle != NULL) {
		isc_nmhandle_detach(&sock->outerhandle);
	}
}

/*
 * Accept callback for TCP-DNS connection.
 */
static isc_result_t
dnslisten_acceptcb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *dnslistensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *dnssock = NULL;
	isc_nmhandle_t *readhandle = NULL;
	isc_nm_accept_cb_t accept_cb;
	void *accept_cbarg;

	REQUIRE(VALID_NMSOCK(dnslistensock));
	REQUIRE(dnslistensock->type == isc_nm_tlsdnslistener);

	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	accept_cb = dnslistensock->accept_cb;
	accept_cbarg = dnslistensock->accept_cbarg;

	if (accept_cb != NULL) {
		result = accept_cb(handle, ISC_R_SUCCESS, accept_cbarg);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	/* We need to create a 'wrapper' dnssocket for this connection */
	dnssock = isc_mem_get(handle->sock->mgr->mctx, sizeof(*dnssock));
	isc__nmsocket_init(dnssock, handle->sock->mgr, isc_nm_tlsdnssocket,
			   handle->sock->iface);

	dnssock->extrahandlesize = dnslistensock->extrahandlesize;
	isc__nmsocket_attach(dnslistensock, &dnssock->listener);

	isc__nmsocket_attach(dnssock, &dnssock->self);

	isc_nmhandle_attach(handle, &dnssock->outerhandle);

	dnssock->peer = handle->sock->peer;
	dnssock->read_timeout = atomic_load(&handle->sock->mgr->init);
	dnssock->tid = isc_nm_tid();
	dnssock->closehandle_cb = resume_processing;

	uv_timer_init(&dnssock->mgr->workers[isc_nm_tid()].loop,
		      &dnssock->timer);
	dnssock->timer.data = dnssock;
	dnssock->timer_initialized = true;
	uv_timer_start(&dnssock->timer, dnstcp_readtimeout,
		       dnssock->read_timeout, 0);
	dnssock->timer_running = true;

	/*
	 * Add a reference to handle to keep it from being freed by
	 * the caller. It will be detached in dnslisted_readcb() when
	 * the connection is closed or there is no more data to be read.
	 */
	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(readhandle, dnslisten_readcb, dnssock);
	isc__nmsocket_detach(&dnssock);

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
	REQUIRE(dnssock->tid == isc_nm_tid());
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
		isc_nmhandle_t *dnshandle = NULL;
		isc_nmsocket_t *listener = NULL;
		isc_nm_recv_cb_t cb = NULL;
		void *cbarg = NULL;

		if (atomic_load(&dnssock->client) &&
		    dnssock->statichandle != NULL) {
			isc_nmhandle_attach(dnssock->statichandle, &dnshandle);
		} else {
			dnshandle = isc__nmhandle_get(dnssock, NULL, NULL);
		}

		listener = dnssock->listener;
		if (listener != NULL) {
			cb = listener->recv_cb;
			cbarg = listener->recv_cbarg;
		} else if (dnssock->recv_cb != NULL) {
			cb = dnssock->recv_cb;
			cbarg = dnssock->recv_cbarg;
			/*
			 * We need to clear the read callback *before*
			 * calling it, because it might make another
			 * call to isc_nm_read() and set up a new callback.
			 */
			isc__nmsocket_clearcb(dnssock);
		}

		if (cb != NULL) {
			cb(dnshandle, ISC_R_SUCCESS,
			   &(isc_region_t){ .base = dnssock->buf + 2,
					    .length = len },
			   cbarg);
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
 * We've got a read on our underlying socket. Check whether
 * we have a complete DNS packet and, if so, call the callback.
 */
static void
dnslisten_readcb(isc_nmhandle_t *handle, isc_result_t eresult,
		 isc_region_t *region, void *arg) {
	isc_nmsocket_t *dnssock = (isc_nmsocket_t *)arg;
	unsigned char *base = NULL;
	bool done = false;
	size_t len;

	REQUIRE(VALID_NMSOCK(dnssock));
	REQUIRE(dnssock->tid == isc_nm_tid());
	REQUIRE(VALID_NMHANDLE(handle));

	if (!isc__nmsocket_active(dnssock) || atomic_load(&dnssock->closing) ||
	    dnssock->outerhandle == NULL ||
	    (dnssock->listener != NULL &&
	     !isc__nmsocket_active(dnssock->listener)) ||
	    atomic_load(&dnssock->mgr->closing))
	{
		if (eresult == ISC_R_SUCCESS) {
			eresult = ISC_R_CANCELED;
		}
	}

	if (region == NULL || eresult != ISC_R_SUCCESS) {
		isc_nm_recv_cb_t cb = dnssock->recv_cb;
		void *cbarg = dnssock->recv_cbarg;

		/* Connection closed */
		dnssock->result = eresult;
		isc__nmsocket_clearcb(dnssock);
		if (atomic_load(&dnssock->client) && cb != NULL) {
			cb(dnssock->statichandle, eresult, NULL, cbarg);
		}

		if (dnssock->self != NULL) {
			isc__nmsocket_detach(&dnssock->self);
		}
		if (dnssock->outerhandle != NULL) {
			isc__nmsocket_clearcb(dnssock->outerhandle->sock);
			isc_nmhandle_detach(&dnssock->outerhandle);
		}
		if (dnssock->listener != NULL) {
			isc__nmsocket_detach(&dnssock->listener);
		}

		/*
		 * Server connections will hold two handle references when
		 * shut down, but client (tlsdnsconnect) connections have
		 * only one.
		 */
		if (!atomic_load(&dnssock->client)) {
			isc_nmhandle_detach(&handle);
		}
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
					 ? atomic_load(&dnssock->mgr->keepalive)
					 : atomic_load(&dnssock->mgr->idle));

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
		if (dnssock->timer_initialized) {
			uv_timer_stop(&dnssock->timer);
		}

		if (atomic_load(&dnssock->sequential) ||
		    dnssock->recv_cb == NULL) {
			/*
			 * There are two reasons we might want to pause here:
			 * - We're in sequential mode and we've received
			 *   a whole packet, so we're done until it's been
			 *   processed; or
			 * - We no longer have a read callback.
			 */
			isc_nm_pauseread(dnssock->outerhandle);
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
			    TLSDNS_CLIENTS_PER_CONN) {
				isc_nm_pauseread(dnssock->outerhandle);
				done = true;
			}
		}

		isc_nmhandle_detach(&dnshandle);
	} while (!done);
}

/*
 * isc_nm_listentlsdns works exactly as listentlsdns but on an SSL socket.
 */
isc_result_t
isc_nm_listentlsdns(isc_nm_t *mgr, isc_nmiface_t *iface, isc_nm_recv_cb_t cb,
		    void *cbarg, isc_nm_accept_cb_t accept_cb,
		    void *accept_cbarg, size_t extrahandlesize, int backlog,
		    isc_quota_t *quota, SSL_CTX *sslctx,
		    isc_nmsocket_t **sockp) {
	isc_nmsocket_t *dnslistensock = isc_mem_get(mgr->mctx,
						    sizeof(*dnslistensock));
	isc_result_t result;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(sslctx != NULL);

	isc__nmsocket_init(dnslistensock, mgr, isc_nm_tlsdnslistener, iface);
	dnslistensock->recv_cb = cb;
	dnslistensock->recv_cbarg = cbarg;
	dnslistensock->accept_cb = accept_cb;
	dnslistensock->accept_cbarg = accept_cbarg;
	dnslistensock->extrahandlesize = extrahandlesize;

	result = isc_nm_listentls(mgr, iface, dnslisten_acceptcb, dnslistensock,
				  extrahandlesize, backlog, quota, sslctx,
				  &dnslistensock->outer);
	if (result == ISC_R_SUCCESS) {
		atomic_store(&dnslistensock->listening, true);
		*sockp = dnslistensock;
		return (ISC_R_SUCCESS);
	} else {
		atomic_store(&dnslistensock->closed, true);
		isc__nmsocket_detach(&dnslistensock);
		return (result);
	}
}

void
isc__nm_async_tlsdnsstop(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsstop_t *ievent =
		(isc__netievent_tlsdnsstop_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	UNUSED(worker);

	REQUIRE(isc__nm_in_netthread());
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnslistener);
	REQUIRE(sock->tid == isc_nm_tid());

	atomic_store(&sock->listening, false);
	atomic_store(&sock->closed, true);

	isc__nmsocket_clearcb(sock);

	if (sock->outer != NULL) {
		isc__nm_tls_stoplistening(sock->outer);
		isc__nmsocket_detach(&sock->outer);
	}
}

void
isc__nm_tlsdns_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnslistener);

	isc__netievent_tlsdnsstop_t *ievent =
		isc__nm_get_ievent(sock->mgr, netievent_tlsdnsstop);
	isc__nmsocket_attach(sock, &ievent->sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc_nm_tlsdns_sequential(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));

	if (handle->sock->type != isc_nm_tlsdnssocket ||
	    handle->sock->outerhandle == NULL)
	{
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
	isc_nm_pauseread(handle->sock->outerhandle);
	atomic_store(&handle->sock->sequential, true);
}

void
isc_nm_tlsdns_keepalive(isc_nmhandle_t *handle, bool value) {
	REQUIRE(VALID_NMHANDLE(handle));

	if (handle->sock->type != isc_nm_tlsdnssocket ||
	    handle->sock->outerhandle == NULL)
	{
		return;
	}

	atomic_store(&handle->sock->keepalive, value);
	atomic_store(&handle->sock->outerhandle->sock->keepalive, value);
}

static void
resume_processing(void *arg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)arg;
	isc_result_t result;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->type != isc_nm_tlsdnssocket || sock->outerhandle == NULL) {
		return;
	}

	if (atomic_load(&sock->ah) == 0) {
		/* Nothing is active; sockets can timeout now */
		if (sock->timer_initialized) {
			uv_timer_start(&sock->timer, dnstcp_readtimeout,
				       sock->read_timeout, 0);
			sock->timer_running = true;
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
			if (sock->timer_initialized) {
				uv_timer_stop(&sock->timer);
			}
			isc_nmhandle_detach(&handle);
		} else if (sock->outerhandle != NULL) {
			isc_nm_resumeread(sock->outerhandle);
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
			if (sock->outerhandle != NULL) {
				isc_nm_resumeread(sock->outerhandle);
			}

			break;
		}

		if (sock->timer_initialized) {
			uv_timer_stop(&sock->timer);
		}
		isc_nmhandle_detach(&dnshandle);
	} while (atomic_load(&sock->ah) < TLSDNS_CLIENTS_PER_CONN);
}

static void
tlsdnssend_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc__nm_uvreq_t *req = (isc__nm_uvreq_t *)cbarg;
	REQUIRE(VALID_UVREQ(req));

	UNUSED(handle);

	req->cb.send(req->handle, result, req->cbarg);
	isc_mem_put(req->sock->mgr->mctx, req->uvbuf.base, req->uvbuf.len);
	isc__nm_uvreq_put(&req, req->handle->sock);
	isc_nmhandle_detach(&handle);
}

/*
 * The socket is closing, outerhandle has been detached, listener is
 * inactive, or the netmgr is closing: any operation on it should abort
 * with ISC_R_CANCELED.
 */
static bool
inactive(isc_nmsocket_t *sock) {
	return (!isc__nmsocket_active(sock) || atomic_load(&sock->closing) ||
		sock->outerhandle == NULL ||
		(sock->listener != NULL &&
		 !isc__nmsocket_active(sock->listener)) ||
		atomic_load(&sock->mgr->closing));
}

void
isc__nm_async_tlsdnssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnssend_t *ievent =
		(isc__netievent_tlsdnssend_t *)ev0;
	isc__nm_uvreq_t *req = ievent->req;
	isc_nmsocket_t *sock = ievent->sock;
	isc_nmhandle_t *sendhandle = NULL;
	isc_region_t r;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(worker->id == sock->tid);
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	if (inactive(sock)) {
		req->cb.send(req->handle, ISC_R_CANCELED, req->cbarg);
		isc_mem_put(sock->mgr->mctx, req->uvbuf.base, req->uvbuf.len);
		isc__nm_uvreq_put(&req, req->handle->sock);
		return;
	}

	r.base = (unsigned char *)req->uvbuf.base;
	r.length = req->uvbuf.len;
	isc_nmhandle_attach(sock->outerhandle, &sendhandle);
	isc_nm_send(sendhandle, &r, tlsdnssend_cb, req);
}

/*
 * isc__nm_tcp_send sends buf to a peer on a socket.
 */
void
isc__nm_tlsdns_send(isc_nmhandle_t *handle, isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg) {
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	isc_nmsocket_t *sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	if (inactive(sock)) {
		cb(handle, ISC_R_CANCELED, cbarg);
		return;
	}

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	isc_nmhandle_attach(handle, &uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	uvreq->uvbuf.base = isc_mem_get(sock->mgr->mctx, region->length + 2);
	uvreq->uvbuf.len = region->length + 2;
	*(uint16_t *)uvreq->uvbuf.base = htons(region->length);
	memmove(uvreq->uvbuf.base + 2, region->base, region->length);

	isc__netievent_tlsdnssend_t *ievent = NULL;

	ievent = isc__nm_get_ievent(sock->mgr, netievent_tlsdnssend);
	ievent->req = uvreq;
	isc__nmsocket_attach(sock, &ievent->sock);

	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

static void
tlsdns_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->timer_running) {
		uv_timer_stop(&sock->timer);
		sock->timer_running = false;
	}

	/* We don't need atomics here, it's all in single network thread */
	if (sock->self != NULL) {
		isc__nmsocket_detach(&sock->self);
	} else if (sock->timer_initialized) {
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
		if (sock->outerhandle != NULL) {
			isc__nmsocket_clearcb(sock->outerhandle->sock);
			isc_nmhandle_detach(&sock->outerhandle);
		}
		if (sock->listener != NULL) {
			isc__nmsocket_detach(&sock->listener);
		}
		atomic_store(&sock->closed, true);
		isc__nmsocket_prep_destroy(sock);
	}
}

void
isc__nm_tlsdns_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		tlsdns_close_direct(sock);
	} else {
		isc__netievent_tlsdnsclose_t *ievent =
			isc__nm_get_ievent(sock->mgr, netievent_tlsdnsclose);

		isc__nmsocket_attach(sock, &ievent->sock);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tlsdnsclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnsclose_t *ievent =
		(isc__netievent_tlsdnsclose_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	UNUSED(worker);

	tlsdns_close_direct(ievent->sock);
}

typedef struct {
	isc_mem_t *mctx;
	isc_nm_cb_t cb;
	void *cbarg;
	size_t extrahandlesize;
} tcpconnect_t;

static void
tlsdnsconnect_cb(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	tcpconnect_t *conn = (tcpconnect_t *)arg;
	isc_nm_cb_t cb = conn->cb;
	void *cbarg = conn->cbarg;
	size_t extrahandlesize = conn->extrahandlesize;
	isc_nmsocket_t *dnssock = NULL;
	isc_nmhandle_t *readhandle = NULL;

	REQUIRE(result != ISC_R_SUCCESS || VALID_NMHANDLE(handle));

	isc_mem_putanddetach(&conn->mctx, conn, sizeof(*conn));

	dnssock = isc_mem_get(handle->sock->mgr->mctx, sizeof(*dnssock));
	isc__nmsocket_init(dnssock, handle->sock->mgr, isc_nm_tlsdnssocket,
			   handle->sock->iface);

	dnssock->extrahandlesize = extrahandlesize;
	isc_nmhandle_attach(handle, &dnssock->outerhandle);

	dnssock->peer = handle->sock->peer;
	dnssock->read_timeout = atomic_load(&handle->sock->mgr->init);
	dnssock->tid = isc_nm_tid();

	atomic_init(&dnssock->client, true);

	readhandle = isc__nmhandle_get(dnssock, NULL, NULL);

	if (result != ISC_R_SUCCESS) {
		cb(readhandle, result, cbarg);
		isc__nmsocket_detach(&dnssock);
		isc_nmhandle_detach(&readhandle);
		return;
	}

	INSIST(dnssock->statichandle != NULL);
	INSIST(dnssock->statichandle == readhandle);
	INSIST(readhandle->sock == dnssock);
	INSIST(dnssock->recv_cb == NULL);

	uv_timer_init(&dnssock->mgr->workers[isc_nm_tid()].loop,
		      &dnssock->timer);
	dnssock->timer.data = dnssock;
	dnssock->timer_initialized = true;
	uv_timer_start(&dnssock->timer, dnstcp_readtimeout,
		       dnssock->read_timeout, 0);
	dnssock->timer_running = true;

	/*
	 * The connection is now established; we start reading immediately,
	 * before we've been asked to. We'll read and buffer at most one
	 * packet.
	 */
	isc_nm_read(handle, dnslisten_readcb, dnssock);
	cb(readhandle, ISC_R_SUCCESS, cbarg);

	/*
	 * The sock is now attached to the handle.
	 */
	isc__nmsocket_detach(&dnssock);
}

isc_result_t
isc_nm_tlsdnsconnect(isc_nm_t *mgr, isc_nmiface_t *local, isc_nmiface_t *peer,
		     isc_nm_cb_t cb, void *cbarg, unsigned int timeout,
		     size_t extrahandlesize) {
	isc_result_t result = ISC_R_SUCCESS;
	tcpconnect_t *conn = isc_mem_get(mgr->mctx, sizeof(tcpconnect_t));
	SSL_CTX *ctx = NULL;

	*conn = (tcpconnect_t){ .cb = cb,
				.cbarg = cbarg,
				.extrahandlesize = extrahandlesize };
	isc_mem_attach(mgr->mctx, &conn->mctx);

	ctx = SSL_CTX_new(SSLv23_client_method());
	result = isc_nm_tlsconnect(mgr, local, peer, tlsdnsconnect_cb, conn,
				   ctx, timeout, 0);
	SSL_CTX_free(ctx);
	if (result != ISC_R_SUCCESS) {
		isc_mem_putanddetach(&conn->mctx, conn, sizeof(*conn));
	}
	return (result);
}

void
isc__nm_tlsdns_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tlsdnsread_t *ievent = NULL;
	isc_nmhandle_t *eventhandle = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(sock->statichandle == handle);
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->recv_cb == NULL);
	REQUIRE(sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&sock->client));

	if (inactive(sock)) {
		cb(handle, ISC_R_NOTCONNECTED, NULL, cbarg);
		return;
	}

	/*
	 * This MUST be done asynchronously, no matter which thread we're
	 * in. The callback function for isc_nm_read() often calls
	 * isc_nm_read() again; if we tried to do that synchronously
	 * we'd clash in processbuffer() and grow the stack indefinitely.
	 */
	ievent = isc__nm_get_ievent(sock->mgr, netievent_tlsdnsread);
	isc__nmsocket_attach(sock, &ievent->sock);

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;

	sock->read_timeout = (atomic_load(&sock->keepalive)
				      ? atomic_load(&sock->mgr->keepalive)
				      : atomic_load(&sock->mgr->idle));

	/*
	 * Add a reference to the handle to keep it from being freed by
	 * the caller; it will be detached in in isc__nm_async_tlsdnsread().
	 */
	isc_nmhandle_attach(handle, &eventhandle);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsdnsread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc_result_t result;
	isc__netievent_tlsdnsread_t *ievent =
		(isc__netievent_tlsdnsclose_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_nmhandle_t *handle = NULL, *newhandle = NULL;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(worker->id == sock->tid);
	REQUIRE(sock->tid == isc_nm_tid());

	handle = sock->statichandle;

	if (inactive(sock)) {
		isc_nm_recv_cb_t cb = sock->recv_cb;
		void *cbarg = sock->recv_cbarg;

		isc__nmsocket_clearcb(sock);
		if (cb != NULL) {
			cb(handle, ISC_R_NOTCONNECTED, NULL, cbarg);
		}

		isc_nmhandle_detach(&handle);
		return;
	}

	/*
	 * Maybe we have a packet already?
	 */
	result = processbuffer(sock, &newhandle);
	if (result == ISC_R_SUCCESS) {
		if (sock->timer_initialized) {
			uv_timer_stop(&sock->timer);
		}
		isc_nmhandle_detach(&newhandle);
	} else if (sock->outerhandle != NULL) {
		/* Restart reading, wait for the callback */
		if (sock->timer_initialized) {
			uv_timer_start(&sock->timer, dnstcp_readtimeout,
				       sock->read_timeout, 0);
			sock->timer_running = true;
		}
		isc_nm_resumeread(sock->outerhandle);
	} else {
		isc_nm_recv_cb_t cb = sock->recv_cb;
		void *cbarg = sock->recv_cbarg;

		isc__nmsocket_clearcb(sock);
		cb(handle, ISC_R_NOTCONNECTED, NULL, cbarg);
	}

	isc_nmhandle_detach(&handle);
}

void
isc__nm_tlsdns_cancelread(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tlsdnscancel_t *ievent = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_tlsdnssocket);

	ievent = isc__nm_get_ievent(sock->mgr, netievent_tlsdnscancel);
	isc__nmsocket_attach(sock, &ievent->sock);
	isc_nmhandle_attach(handle, &ievent->handle);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsdnscancel(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsdnscancel_t *ievent =
		(isc__netievent_tlsdnscancel_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_nmhandle_t *handle = ievent->handle;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(worker->id == sock->tid);
	REQUIRE(sock->tid == isc_nm_tid());

	if (atomic_load(&sock->client)) {
		isc_nm_recv_cb_t cb;
		void *cbarg = NULL;

		cb = sock->recv_cb;
		cbarg = sock->recv_cbarg;
		isc__nmsocket_clearcb(sock);

		if (cb != NULL) {
			cb(handle, ISC_R_EOF, NULL, cbarg);
		}

		isc__nm_tcp_cancelread(sock->outerhandle);
	}
}

void
isc__nm_tlsdns_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	if (sock->outerhandle != NULL) {
		isc__nm_tcp_settimeout(sock->outerhandle, timeout);
	}

	sock->read_timeout = timeout;
	if (sock->timer_running) {
		uv_timer_start(&sock->timer, dnstcp_readtimeout,
			       sock->read_timeout, 0);
	}
}
