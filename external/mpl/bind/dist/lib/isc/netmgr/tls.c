/*	$NetBSD: tls.c,v 1.2 2021/02/19 16:42:20 christos Exp $	*/

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

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/once.h>
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

#define TLS_BUF_SIZE 65536

static isc_result_t
tls_error_to_result(int tls_err) {
	switch (tls_err) {
	case SSL_ERROR_ZERO_RETURN:
		return (ISC_R_EOF);
	default:
		return (ISC_R_UNEXPECTED);
	}
}

static void
tls_do_bio(isc_nmsocket_t *sock);

static void
tls_close_direct(isc_nmsocket_t *sock);

static void
async_tls_do_bio(isc_nmsocket_t *sock);

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

static void
tls_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)cbarg;

	UNUSED(handle);
	/*  XXXWPK TODO */
	UNUSED(eresult);

	isc_mem_put(sock->mgr->mctx, sock->tls.senddata.base,
		    sock->tls.senddata.length);
	sock->tls.senddata = (isc_region_t){ NULL, 0 };
	sock->tls.sending = false;

	async_tls_do_bio(sock);
}

static void
async_tls_do_bio(isc_nmsocket_t *sock) {
	isc__netievent_tlsdobio_t *ievent =
		isc__nm_get_netievent_tlsdobio(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

static void
tls_do_bio(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_SUCCESS;
	int pending, tls_err = 0;
	int rv;
	isc__nm_uvreq_t *req;

	REQUIRE(sock->tid == isc_nm_tid());
	/* We will resume read if TLS layer wants us to */
	isc_nm_pauseread(sock->outerhandle);

	if (inactive(sock)) {
		result = ISC_R_CANCELED;
		goto error;
	}

	if (sock->tls.state == TLS_INIT) {
		(void)SSL_do_handshake(sock->tls.ssl);
		sock->tls.state = TLS_HANDSHAKE;
	}

	if (sock->tls.state == TLS_ERROR) {
		result = ISC_R_FAILURE;
		goto error;
	}

	/* Data from TLS to client */
	char buf[1];
	if (sock->tls.state == TLS_IO && sock->recv_cb != NULL &&
	    !atomic_load(&sock->readpaused))
	{
		(void)SSL_peek(sock->tls.ssl, buf, 1);
		while ((pending = SSL_pending(sock->tls.ssl)) > 0) {
			if (pending > TLS_BUF_SIZE) {
				pending = TLS_BUF_SIZE;
			}
			isc_region_t region = {
				isc_mem_get(sock->mgr->mctx, pending), pending
			};
			isc_region_t dregion;
			memset(region.base, 0, region.length);
			rv = SSL_read(sock->tls.ssl, region.base,
				      region.length);
			/* Pending succeded, so should read */
			RUNTIME_CHECK(rv == pending);
			dregion = (isc_region_t){ region.base, rv };
			sock->recv_cb(sock->statichandle, ISC_R_SUCCESS,
				      &dregion, sock->recv_cbarg);
			isc_mem_put(sock->mgr->mctx, region.base,
				    region.length);
		}
	}

	/* Peek to move the session forward */
	(void)SSL_peek(sock->tls.ssl, buf, 1);

	/* Data from TLS to network */
	pending = BIO_pending(sock->tls.app_bio);
	if (!sock->tls.sending && pending > 0) {
		if (pending > TLS_BUF_SIZE) {
			pending = TLS_BUF_SIZE;
		}
		sock->tls.sending = true;
		sock->tls.senddata.base = isc_mem_get(sock->mgr->mctx, pending);
		sock->tls.senddata.length = pending;
		rv = BIO_read(sock->tls.app_bio, sock->tls.senddata.base,
			      pending);
		/* There's something pending, read must succed */
		RUNTIME_CHECK(rv == pending);
		isc_nm_send(sock->outerhandle, &sock->tls.senddata,
			    tls_senddone, sock);
		/* We'll continue in tls_senddone */
		return;
	}

	/* Get the potential error code */
	rv = SSL_peek(sock->tls.ssl, buf, 1);

	if (rv < 0) {
		tls_err = SSL_get_error(sock->tls.ssl, rv);
	}

	/* Only after doing the IO we can check if SSL handshake is done */
	if (sock->tls.state == TLS_HANDSHAKE &&
	    SSL_is_init_finished(sock->tls.ssl) == 1)
	{
		isc_nmhandle_t *tlshandle = isc__nmhandle_get(sock, NULL, NULL);
		if (sock->tls.server) {
			sock->listener->accept_cb(sock->statichandle,
						  ISC_R_SUCCESS,
						  sock->listener->accept_cbarg);
		} else {
			sock->connect_cb(tlshandle, ISC_R_SUCCESS,
					 sock->connect_cbarg);
		}
		isc_nmhandle_detach(&tlshandle);
		sock->tls.state = TLS_IO;
		async_tls_do_bio(sock);
		return;
	}

	switch (tls_err) {
	case 0:
		return;
	case SSL_ERROR_WANT_WRITE:
		if (!sock->tls.sending) {
			/*
			 * Launch tls_do_bio asynchronously. If we're sending
			 * already the send callback will call it.
			 */
			async_tls_do_bio(sock);
		} else {
			return;
		}
		break;
	case SSL_ERROR_WANT_READ:
		isc_nm_resumeread(sock->outerhandle);
		break;
	default:
		result = tls_error_to_result(tls_err);
		goto error;
	}

	while ((req = ISC_LIST_HEAD(sock->tls.sends)) != NULL) {
		INSIST(VALID_UVREQ(req));
		rv = SSL_write(sock->tls.ssl, req->uvbuf.base, req->uvbuf.len);
		if (rv < 0) {
			if (!sock->tls.sending) {
				async_tls_do_bio(sock);
			}
			return;
		}
		if (rv != (int)req->uvbuf.len) {
			sock->tls.state = TLS_ERROR;
			async_tls_do_bio(sock);
			return;
		}
		ISC_LIST_UNLINK(sock->tls.sends, req, link);
		req->cb.send(sock->statichandle, ISC_R_SUCCESS, req->cbarg);
		isc__nm_uvreq_put(&req, sock);
	}

	return;

error:
	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_ERROR, "SSL error in BIO: %d %s", tls_err,
		      isc_result_totext(result));
	if (ISC_LIST_HEAD(sock->tls.sends) != NULL) {
		while ((req = ISC_LIST_HEAD(sock->tls.sends)) != NULL) {
			req->cb.send(sock->statichandle, result, req->cbarg);
			ISC_LIST_UNLINK(sock->tls.sends, req, link);
			isc__nm_uvreq_put(&req, sock);
		}
	} else if (sock->recv_cb != NULL) {
		sock->recv_cb(sock->statichandle, result, NULL,
			      sock->recv_cbarg);
	} else {
		tls_close_direct(sock);
	}
}

static void
tls_readcb(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *region,
	   void *cbarg) {
	isc_nmsocket_t *tlssock = (isc_nmsocket_t *)cbarg;
	int rv;

	REQUIRE(VALID_NMSOCK(tlssock));
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(tlssock->tid == isc_nm_tid());
	if (result != ISC_R_SUCCESS) {
		/* Connection closed */
		/*
		 * TODO accept_cb should be called if we're not
		 * initialized yet!
		 */
		if (tlssock->recv_cb != NULL) {
			tlssock->recv_cb(tlssock->statichandle, result, region,
					 tlssock->recv_cbarg);
		}
		isc__nm_tls_close(tlssock);
		return;
	}
	rv = BIO_write(tlssock->tls.app_bio, region->base, region->length);

	if (rv != (int)region->length) {
		/* XXXWPK log it? */
		tlssock->tls.state = TLS_ERROR;
	}
	tls_do_bio(tlssock);
}

static isc_result_t
initialize_tls(isc_nmsocket_t *sock, bool server) {
	REQUIRE(sock->tid == isc_nm_tid());

	if (BIO_new_bio_pair(&(sock->tls.ssl_bio), TLS_BUF_SIZE,
			     &(sock->tls.app_bio), TLS_BUF_SIZE) != 1)
	{
		SSL_free(sock->tls.ssl);
		return (ISC_R_TLSERROR);
	}

	SSL_set_bio(sock->tls.ssl, sock->tls.ssl_bio, sock->tls.ssl_bio);
	if (server) {
		SSL_set_accept_state(sock->tls.ssl);
	} else {
		SSL_set_connect_state(sock->tls.ssl);
	}
	isc_nm_read(sock->outerhandle, tls_readcb, sock);
	tls_do_bio(sock);
	return (ISC_R_SUCCESS);
}

static isc_result_t
tlslisten_acceptcb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	REQUIRE(VALID_NMSOCK(cbarg));
	isc_nmsocket_t *tlslistensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *tlssock = NULL;
	int r;

	REQUIRE(tlslistensock->type == isc_nm_tlslistener);

	/* If accept() was unsuccessful we can't do anything */
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * We need to create a 'wrapper' tlssocket for this connection.
	 */
	tlssock = isc_mem_get(handle->sock->mgr->mctx, sizeof(*tlssock));
	isc__nmsocket_init(tlssock, handle->sock->mgr, isc_nm_tlssocket,
			   handle->sock->iface);

	tlssock->extrahandlesize = tlslistensock->extrahandlesize;
	isc__nmsocket_attach(tlslistensock, &tlssock->listener);
	isc_nmhandle_attach(handle, &tlssock->outerhandle);
	tlssock->peer = handle->sock->peer;
	tlssock->read_timeout = atomic_load(&handle->sock->mgr->init);
	tlssock->tid = isc_nm_tid();
	tlssock->tls.server = true;
	tlssock->tls.state = TLS_INIT;
	tlssock->tls.ctx = tlslistensock->tls.ctx;
	/* We need to initialize SSL now to reference SSL_CTX properly */
	tlssock->tls.ssl = SSL_new(tlssock->tls.ctx);
	ISC_LIST_INIT(tlssock->tls.sends);
	if (tlssock->tls.ssl == NULL) {
		atomic_store(&tlssock->closed, true);
		isc__nmsocket_detach(&tlssock);
		return (ISC_R_TLSERROR);
	}

	r = uv_timer_init(&tlssock->mgr->workers[isc_nm_tid()].loop,
			  &tlssock->timer);
	RUNTIME_CHECK(r == 0);

	tlssock->timer.data = tlssock;
	tlssock->timer_initialized = true;
	tlssock->tls.ctx = tlslistensock->tls.ctx;

	result = initialize_tls(tlssock, true);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	/* TODO: catch failure code, detach tlssock, and log the error */

	return (result);
}

isc_result_t
isc_nm_listentls(isc_nm_t *mgr, isc_nmiface_t *iface,
		 isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		 size_t extrahandlesize, int backlog, isc_quota_t *quota,
		 SSL_CTX *sslctx, isc_nmsocket_t **sockp) {
	isc_result_t result;
	isc_nmsocket_t *tlssock = isc_mem_get(mgr->mctx, sizeof(*tlssock));

	isc__nmsocket_init(tlssock, mgr, isc_nm_tlslistener, iface);
	tlssock->accept_cb = accept_cb;
	tlssock->accept_cbarg = accept_cbarg;
	tlssock->extrahandlesize = extrahandlesize;
	tlssock->tls.ctx = sslctx;
	/* We need to initialize SSL now to reference SSL_CTX properly */
	tlssock->tls.ssl = SSL_new(tlssock->tls.ctx);
	if (tlssock->tls.ssl == NULL) {
		atomic_store(&tlssock->closed, true);
		isc__nmsocket_detach(&tlssock);
		return (ISC_R_TLSERROR);
	}

	/*
	 * tlssock will be a TLS 'wrapper' around an unencrypted stream.
	 * We set tlssock->outer to a socket listening for a TCP connection.
	 */
	result = isc_nm_listentcp(mgr, iface, tlslisten_acceptcb, tlssock,
				  extrahandlesize, backlog, quota,
				  &tlssock->outer);
	if (result == ISC_R_SUCCESS) {
		atomic_store(&tlssock->listening, true);
		*sockp = tlssock;
		return (ISC_R_SUCCESS);
	} else {
		atomic_store(&tlssock->closed, true);
		isc__nmsocket_detach(&tlssock);
		return (result);
	}
}

void
isc__nm_async_tlssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	int rv;
	isc__netievent_tcpsend_t *ievent = (isc__netievent_tcpsend_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	ievent->req = NULL;
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	if (inactive(sock)) {
		req->cb.send(req->handle, ISC_R_CANCELED, req->cbarg);
		isc__nm_uvreq_put(&req, sock);
		return;
	}
	if (!ISC_LIST_EMPTY(sock->tls.sends)) {
		/* We're not the first */
		ISC_LIST_APPEND(sock->tls.sends, req, link);
		tls_do_bio(sock);
		return;
	}

	rv = SSL_write(sock->tls.ssl, req->uvbuf.base, req->uvbuf.len);
	if (rv < 0) {
		/*
		 * We might need to read, we might need to write, or the
		 * TLS socket might be dead - in any case, we need to
		 * enqueue the uvreq and let the TLS BIO layer do the rest.
		 */
		ISC_LIST_APPEND(sock->tls.sends, req, link);
		tls_do_bio(sock);
		return;
	}
	if (rv != (int)req->uvbuf.len) {
		sock->tls.state = TLS_ERROR;
		async_tls_do_bio(sock);
		return;
	}
	req->cb.send(sock->statichandle, ISC_R_SUCCESS, req->cbarg);
	isc__nm_uvreq_put(&req, sock);
	tls_do_bio(sock);
	return;
}

void
isc__nm_tls_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		 void *cbarg) {
	isc__netievent_tlssend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;
	isc_nmsocket_t *sock = NULL;
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_tlssocket);

	if (inactive(sock)) {
		cb(handle, ISC_R_CANCELED, cbarg);
		return;
	}

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	isc_nmhandle_attach(handle, &uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	/*
	 * We need to create an event and pass it using async channel
	 */
	ievent = isc__nm_get_netievent_tlssend(sock->mgr, sock, uvreq);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsstartread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsstartread_t *ievent =
		(isc__netievent_tlsstartread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	tls_do_bio(sock);
}

void
isc__nm_tls_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->statichandle == handle);
	REQUIRE(handle->sock->tid == isc_nm_tid());

	isc__netievent_tlsstartread_t *ievent = NULL;
	isc_nmsocket_t *sock = handle->sock;

	if (inactive(sock)) {
		cb(handle, ISC_R_NOTCONNECTED, NULL, cbarg);
		return;
	}

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;

	ievent = isc__nm_get_netievent_tlsstartread(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_tls_pauseread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	isc_nmsocket_t *sock = handle->sock;

	atomic_store(&sock->readpaused, true);
}

void
isc__nm_tls_resumeread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	isc_nmsocket_t *sock = handle->sock;

	atomic_store(&sock->readpaused, false);
	async_tls_do_bio(sock);
}

static void
timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)uv_handle_get_data(handle);
	tls_close_direct(sock);
}

static void
tls_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	if (sock->timer_running) {
		uv_timer_stop(&sock->timer);
		sock->timer_running = false;
	}

	/* We don't need atomics here, it's all in single network thread
	 */
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
		 * At this point we're certain that there are no
		 * external references, we can close everything.
		 */
		if (sock->outerhandle != NULL) {
			isc_nm_pauseread(sock->outerhandle);
			isc_nmhandle_detach(&sock->outerhandle);
		}
		if (sock->listener != NULL) {
			isc__nmsocket_detach(&sock->listener);
		}
		if (sock->tls.ssl != NULL) {
			SSL_free(sock->tls.ssl);
			sock->tls.ssl = NULL;
			/* These are destroyed when we free SSL* */
			sock->tls.ctx = NULL;
			sock->tls.ssl_bio = NULL;
		}
		if (sock->tls.app_bio != NULL) {
			BIO_free(sock->tls.app_bio);
			sock->tls.app_bio = NULL;
		}
		atomic_store(&sock->closed, true);
		isc__nmsocket_detach(&sock);
	}
}

void
isc__nm_tls_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		tls_close_direct(sock);
	} else {
		isc__netievent_tlsclose_t *ievent =
			isc__nm_get_netievent_tlsclose(sock->mgr, sock);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tlsclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsclose_t *ievent = (isc__netievent_tlsclose_t *)ev0;

	REQUIRE(ievent->sock->tid == isc_nm_tid());
	UNUSED(worker);

	tls_close_direct(ievent->sock);
}

void
isc__nm_tls_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlslistener);

	atomic_store(&sock->listening, false);
	atomic_store(&sock->closed, true);
	sock->recv_cb = NULL;
	sock->recv_cbarg = NULL;
	if (sock->tls.ssl != NULL) {
		SSL_free(sock->tls.ssl);
		sock->tls.ssl = NULL;
		sock->tls.ctx = NULL;
	}

	if (sock->outer != NULL) {
		isc_nm_stoplistening(sock->outer);
		isc__nmsocket_detach(&sock->outer);
	}
}

isc_result_t
isc_nm_tlsconnect(isc_nm_t *mgr, isc_nmiface_t *local, isc_nmiface_t *peer,
		  isc_nm_cb_t cb, void *cbarg, SSL_CTX *ctx,
		  unsigned int timeout, size_t extrahandlesize) {
	isc_nmsocket_t *nsock = NULL;
	isc__netievent_tlsconnect_t *ievent = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_NM(mgr));

	nsock = isc_mem_get(mgr->mctx, sizeof(*nsock));
	isc__nmsocket_init(nsock, mgr, isc_nm_tlssocket, local);
	nsock->extrahandlesize = extrahandlesize;
	nsock->result = ISC_R_SUCCESS;
	nsock->connect_cb = cb;
	nsock->connect_cbarg = cbarg;
	nsock->connect_timeout = timeout;
	nsock->tls.ctx = ctx;
	/* We need to initialize SSL now to reference SSL_CTX properly
	 */
	nsock->tls.ssl = SSL_new(nsock->tls.ctx);
	if (nsock->tls.ssl == NULL) {
		atomic_store(&nsock->closed, true);
		isc__nmsocket_detach(&nsock);
		return (ISC_R_TLSERROR);
	}

	ievent = isc__nm_get_netievent_tlsconnect(mgr, nsock);
	ievent->local = local->addr;
	ievent->peer = peer->addr;
	ievent->ctx = ctx;

	if (isc__nm_in_netthread()) {
		nsock->tid = isc_nm_tid();
		isc__nm_async_tlsconnect(&mgr->workers[nsock->tid],
					 (isc__netievent_t *)ievent);
		isc__nm_put_netievent_tlsconnect(mgr, ievent);
	} else {
		nsock->tid = isc_random_uniform(mgr->nworkers);
		isc__nm_enqueue_ievent(&mgr->workers[nsock->tid],
				       (isc__netievent_t *)ievent);
	}

	return (result);
}

static void
tls_connect_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *tlssock = (isc_nmsocket_t *)cbarg;

	REQUIRE(VALID_NMSOCK(tlssock));

	if (result != ISC_R_SUCCESS) {
		tlssock->connect_cb(handle, result, tlssock->connect_cbarg);
		LOCK(&tlssock->parent->lock);
		tlssock->parent->result = result;
		UNLOCK(&tlssock->parent->lock);
		tls_close_direct(tlssock);
		return;
	}

	INSIST(VALID_NMHANDLE(handle));

	tlssock->peer = isc_nmhandle_peeraddr(handle);
	isc_nmhandle_attach(handle, &tlssock->outerhandle);
	result = initialize_tls(tlssock, false);
	if (result != ISC_R_SUCCESS) {
		tlssock->connect_cb(handle, result, tlssock->connect_cbarg);
		LOCK(&tlssock->parent->lock);
		tlssock->parent->result = result;
		UNLOCK(&tlssock->parent->lock);
		tls_close_direct(tlssock);
		return;
	}
}
void
isc__nm_async_tlsconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsconnect_t *ievent =
		(isc__netievent_tlsconnect_t *)ev0;
	isc_nmsocket_t *tlssock = ievent->sock;
	isc_result_t result;
	int r;

	UNUSED(worker);

	tlssock->tid = isc_nm_tid();
	r = uv_timer_init(&tlssock->mgr->workers[isc_nm_tid()].loop,
			  &tlssock->timer);
	RUNTIME_CHECK(r == 0);

	tlssock->timer.data = tlssock;
	tlssock->timer_initialized = true;
	tlssock->tls.state = TLS_INIT;

	result = isc_nm_tcpconnect(worker->mgr, (isc_nmiface_t *)&ievent->local,
				   (isc_nmiface_t *)&ievent->peer,
				   tls_connect_cb, tlssock,
				   tlssock->connect_timeout, 0);
	if (result != ISC_R_SUCCESS) {
		/* FIXME: We need to pass valid handle */
		tlssock->connect_cb(NULL, result, tlssock->connect_cbarg);
		LOCK(&tlssock->parent->lock);
		tlssock->parent->result = result;
		UNLOCK(&tlssock->parent->lock);
		tls_close_direct(tlssock);
		return;
	}
}

void
isc__nm_async_tlsdobio(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(worker);
	isc__netievent_tlsdobio_t *ievent = (isc__netievent_tlsdobio_t *)ev0;
	tls_do_bio(ievent->sock);
}

isc_result_t
isc_nm_tls_create_server_ctx(const char *keyfile, const char *certfile,
			     SSL_CTX **ctxp) {
	INSIST(ctxp != NULL);
	INSIST(*ctxp == NULL);
	int rv;
	unsigned long err;
	bool ephemeral = (keyfile == NULL && certfile == NULL);
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;
	BIGNUM *bn = NULL;
	RSA *rsa = NULL;

	if (ephemeral) {
		INSIST(keyfile == NULL);
		INSIST(certfile == NULL);
	} else {
		INSIST(keyfile != NULL);
		INSIST(certfile != NULL);
	}

#ifdef HAVE_TLS_SERVER_METHOD
	const SSL_METHOD *method = TLS_server_method();
#else
	const SSL_METHOD *method = SSLv23_server_method();
#endif

	SSL_CTX *ctx = SSL_CTX_new(method);
	RUNTIME_CHECK(ctx != NULL);
	if (ephemeral) {
		rsa = RSA_new();
		if (rsa == NULL) {
			goto ssl_error;
		}
		bn = BN_new();
		if (bn == NULL) {
			goto ssl_error;
		}
		BN_set_word(bn, RSA_F4);
		rv = RSA_generate_key_ex(rsa, 4096, bn, NULL);
		if (rv != 1) {
			goto ssl_error;
		}
		cert = X509_new();
		if (cert == NULL) {
			goto ssl_error;
		}
		pkey = EVP_PKEY_new();
		if (pkey == NULL) {
			goto ssl_error;
		}

		/*
		 * EVP_PKEY_assign_*() set the referenced key to key
		 * however these use the supplied key internally and so
		 * key will be freed when the parent pkey is freed.
		 */
		EVP_PKEY_assign(pkey, EVP_PKEY_RSA, rsa);
		rsa = NULL;
		ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

		X509_gmtime_adj(X509_get_notBefore(cert), 0);
		/*
		 * We set the vailidy for 10 years.
		 */
		X509_gmtime_adj(X509_get_notAfter(cert), 3650 * 24 * 3600);

		X509_set_pubkey(cert, pkey);

		X509_NAME *name = X509_get_subject_name(cert);

		X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC,
					   (const unsigned char *)"AQ", -1, -1,
					   0);
		X509_NAME_add_entry_by_txt(
			name, "O", MBSTRING_ASC,
			(const unsigned char *)"BIND9 ephemeral "
					       "certificate",
			-1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
					   (const unsigned char *)"bind9.local",
					   -1, -1, 0);

		X509_set_issuer_name(cert, name);
		X509_sign(cert, pkey, EVP_sha256());
		rv = SSL_CTX_use_certificate(ctx, cert);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = SSL_CTX_use_PrivateKey(ctx, pkey);
		if (rv != 1) {
			goto ssl_error;
		}

		X509_free(cert);
		EVP_PKEY_free(pkey);
		BN_free(bn);
	} else {
		rv = SSL_CTX_use_certificate_file(ctx, certfile,
						  SSL_FILETYPE_PEM);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = SSL_CTX_use_PrivateKey_file(ctx, keyfile,
						 SSL_FILETYPE_PEM);
		if (rv != 1) {
			goto ssl_error;
		}
	}
	*ctxp = ctx;
	return (ISC_R_SUCCESS);

ssl_error:
	err = ERR_get_error();
	char errbuf[256];
	ERR_error_string_n(err, errbuf, 256);
	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_ERROR, "Error initializing TLS context: %s",
		      errbuf);
	if (ctx != NULL) {
		SSL_CTX_free(ctx);
	}
	if (cert != NULL) {
		X509_free(cert);
	}
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (bn != NULL) {
		BN_free(bn);
	}
	if (rsa != NULL) {
		RSA_free(rsa);
	}

	return (ISC_R_TLSERROR);
}

void
isc__nm_tls_initialize() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_library_init();
#else
	OPENSSL_init_ssl(0, NULL);
#endif
}
