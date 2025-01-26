/*	$NetBSD: tlsstream.c,v 1.4 2025/01/26 16:25:43 christos Exp $	*/

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

#include <errno.h>
#include <libgen.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <isc/async.h>
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
#include <isc/uv.h>

#include "../openssl_shim.h"
#include "netmgr-int.h"

#define TLS_BUF_SIZE (UINT16_MAX)

#define TLS_MAX_SEND_BUF_SIZE (UINT16_MAX + UINT16_MAX / 2)

#define MAX_DNS_MESSAGE_SIZE (UINT16_MAX)

#ifdef ISC_NETMGR_TRACE
ISC_ATTR_UNUSED static const char *
tls_status2str(int tls_status) {
	switch (tls_status) {
	case SSL_ERROR_NONE:
		return "SSL_ERROR_NONE";
	case SSL_ERROR_ZERO_RETURN:
		return "SSL_ERROR_ZERO_RETURN";
	case SSL_ERROR_WANT_WRITE:
		return "SSL_ERROR_WANT_WRITE";
	case SSL_ERROR_WANT_READ:
		return "SSL_ERROR_WANT_READ";
	case SSL_ERROR_SSL:
		return "SSL_ERROR_SSL";
	default:
		UNREACHABLE();
	}
}

ISC_ATTR_UNUSED static const char *
state2str(int state) {
	switch (state) {
	case TLS_INIT:
		return "TLS_INIT";
	case TLS_HANDSHAKE:
		return "TLS_HANDSHAKE";
	case TLS_IO:
		return "TLS_IO";
	case TLS_CLOSED:
		return "TLS_CLOSED";
	default:
		UNREACHABLE();
	}
}
#endif /* ISC_NETMGR_TRACE */

static isc_result_t
tls_error_to_result(const int tls_err, const int tls_state, isc_tls_t *tls) {
	switch (tls_err) {
	case SSL_ERROR_ZERO_RETURN:
		return ISC_R_EOF;
	case SSL_ERROR_SSL:
		if (tls != NULL && tls_state < TLS_IO &&
		    SSL_get_verify_result(tls) != X509_V_OK)
		{
			return ISC_R_TLSBADPEERCERT;
		}
		return ISC_R_TLSERROR;
	default:
		return ISC_R_UNEXPECTED;
	}
}

static void
tls_read_start(isc_nmsocket_t *restrict sock);

static void
tls_read_stop(isc_nmsocket_t *sock);

static void
tls_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result);

static void
tls_do_bio(isc_nmsocket_t *sock, isc_region_t *received_data,
	   isc__nm_uvreq_t *send_data, bool finish);

static void
tls_readcb(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *region,
	   void *cbarg);

static void
async_tls_do_bio(isc_nmsocket_t *sock);

static void
tls_init_listener_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *ctx);

static void
tls_cleanup_listener_tlsctx(isc_nmsocket_t *listener);

static isc_tlsctx_t *
tls_get_listener_tlsctx(isc_nmsocket_t *listener, const int tid);

static void
tls_keep_client_tls_session(isc_nmsocket_t *sock);

static void
tls_try_shutdown(isc_tls_t *tls, const bool quite);

static void
tls_try_to_enable_tcp_nodelay(isc_nmsocket_t *tlssock);

/*
 * The socket is closing, outerhandle has been detached, listener is
 * inactive, or the netmgr is closing: any operation on it should abort
 * with ISC_R_CANCELED.
 */
static bool
inactive(isc_nmsocket_t *sock) {
	return !isc__nmsocket_active(sock) || sock->closing ||
	       sock->outerhandle == NULL ||
	       !isc__nmsocket_active(sock->outerhandle->sock) ||
	       sock->outerhandle->sock->closing ||
	       isc__nm_closing(sock->worker);
}

static void
tls_call_connect_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
		    const isc_result_t result) {
	INSIST(sock->connect_cb != NULL);
	sock->connect_cb(handle, result, sock->connect_cbarg);
	if (result != ISC_R_SUCCESS) {
		isc__nmsocket_clearcb(handle->sock);
	}
}

static void
tls_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmsocket_tls_send_req_t *send_req =
		(isc_nmsocket_tls_send_req_t *)cbarg;
	isc_nmsocket_t *tlssock = NULL;
	bool finish = send_req->finish;
	isc_nm_cb_t send_cb = NULL;
	void *send_cbarg = NULL;
	isc_nmhandle_t *send_handle = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(VALID_NMSOCK(send_req->tlssock));

	tlssock = send_req->tlssock;
	send_req->tlssock = NULL;
	send_cb = send_req->cb;
	send_req->cb = NULL;
	send_cbarg = send_req->cbarg;
	send_req->cbarg = NULL;
	send_handle = send_req->handle;
	send_req->handle = NULL;

	if (finish) {
		tls_try_shutdown(tlssock->tlsstream.tls, true);
	}

	/* Try to keep the object to be reused later - to avoid an allocation */
	if (tlssock->tlsstream.send_req == NULL) {
		tlssock->tlsstream.send_req = send_req;
		/*
		 * We need to ensure that the buffer is not going to grow too
		 * large uncontrollably. We try to keep its size to be no more
		 * than TLS_MAX_SEND_BUF_SIZE. The constant should be larger
		 * than 64 KB for this to work efficiently when combined with
		 * DNS transports.
		 */
		if (isc_buffer_length(&send_req->data) > TLS_MAX_SEND_BUF_SIZE)
		{
			/* free the underlying buffer */
			isc_buffer_clearmctx(&send_req->data);
			isc_buffer_invalidate(&send_req->data);
			isc_buffer_init(&send_req->data, send_req->smallbuf,
					sizeof(send_req->smallbuf));
			isc_buffer_setmctx(&send_req->data,
					   handle->sock->worker->mctx);
		} else {
			isc_buffer_clear(&send_req->data);
		}
	} else {
		isc_buffer_clearmctx(&send_req->data);
		isc_buffer_invalidate(&send_req->data);
		isc_mem_put(handle->sock->worker->mctx, send_req,
			    sizeof(*send_req));
	}
	tlssock->tlsstream.nsending--;

	if (send_cb != NULL) {
		INSIST(VALID_NMHANDLE(tlssock->statichandle));
		send_cb(send_handle, eresult, send_cbarg);
		isc_nmhandle_detach(&send_handle);
		/* The last handle has been just detached: close the underlying
		 * socket. */
		if (tlssock->statichandle == NULL) {
			finish = true;
		}
	}

	if (finish) {
		/*
		 * If wrapping up, call tls_failed_read() - it will care of
		 * socket de-initialisation and calling the read callback, if
		 * necessary.
		 */
		tls_failed_read_cb(tlssock, ISC_R_EOF);
	} else if (eresult == ISC_R_SUCCESS) {
		tls_do_bio(tlssock, NULL, NULL, false);
	} else if (eresult != ISC_R_SUCCESS &&
		   tlssock->tlsstream.state <= TLS_HANDSHAKE &&
		   !tlssock->tlsstream.server)
	{
		/*
		 * We are still waiting for the handshake to complete, but
		 * it isn't going to happen. Call the connect callback,
		 * passing the error code there.
		 *
		 * (Note: tls_failed_read_cb() calls the connect
		 * rather than the read callback in this case.
		 * XXX: clarify?)
		 */
		tls_failed_read_cb(tlssock, eresult);
	}

	isc__nmsocket_detach(&tlssock);
}

static void
tls_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(result != ISC_R_SUCCESS);

	/* This is TLS counterpart of isc__nm_failed_connect_cb() */
	if (!sock->tlsstream.server &&
	    (sock->tlsstream.state == TLS_INIT ||
	     sock->tlsstream.state == TLS_HANDSHAKE) &&
	    sock->connect_cb != NULL)
	{
		isc_nmhandle_t *handle = NULL;
		INSIST(sock->statichandle == NULL);
		handle = isc__nmhandle_get(sock, &sock->peer, &sock->iface);
		tls_call_connect_cb(sock, handle, result);
		isc__nmsocket_clearcb(sock);
		isc_nmhandle_detach(&handle);
		goto destroy;
	}

	isc__nmsocket_timer_stop(sock);

	/* Nobody is reading from the socket yet */
	if (sock->statichandle == NULL) {
		goto destroy;
	}

	/* This is TLS counterpart of isc__nmsocket_readtimeout_cb() */
	if (sock->client && result == ISC_R_TIMEDOUT) {
		INSIST(sock->statichandle != NULL);

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

	/*
	 * We don't need to check for .nsending, as the callbacks will be
	 * cleared at the time the tls_senddone() tries to call it for the
	 * second time.
	 */

	if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = isc__nm_get_read_req(sock, NULL);
		isc__nmsocket_clearcb(sock);
		isc__nm_readcb(sock, req, result, false);
	}

destroy:
	isc__nmsocket_prep_destroy(sock);
}

void
isc__nm_tls_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			   bool async ISC_ATTR_UNUSED) {
	if (!inactive(sock) && sock->tlsstream.state == TLS_IO) {
		tls_do_bio(sock, NULL, NULL, true);
		return;
	}

	tls_failed_read_cb(sock, result);
}

static void
tls_do_bio_cb(void *arg) {
	isc_nmsocket_t *sock = arg;

	REQUIRE(VALID_NMSOCK(sock));

	tls_do_bio(sock, NULL, NULL, false);

	isc__nmsocket_detach(&sock);
}

static void
async_tls_do_bio(isc_nmsocket_t *sock) {
	isc__nmsocket_attach(sock, &(isc_nmsocket_t *){ NULL });
	isc_async_run(sock->worker->loop, tls_do_bio_cb, sock);
}

static int
tls_send_outgoing(isc_nmsocket_t *sock, bool finish, isc_nmhandle_t *tlshandle,
		  isc_nm_cb_t cb, void *cbarg) {
	isc_nmsocket_tls_send_req_t *send_req = NULL;
	int pending;
	int rv;
	size_t len = 0;
	bool new_send_req = false;
	isc_region_t used_region = { 0 };
	bool shutting_down = isc__nm_closing(sock->worker);

	if (shutting_down || inactive(sock)) {
		if (cb != NULL) {
			isc_result_t result = shutting_down ? ISC_R_SHUTTINGDOWN
							    : ISC_R_CANCELED;
			INSIST(VALID_NMHANDLE(tlshandle));
			cb(tlshandle, result, cbarg);
		}
		return 0;
	}

	if (finish) {
		tls_try_shutdown(sock->tlsstream.tls, false);
		tls_keep_client_tls_session(sock);
	}

	pending = BIO_pending(sock->tlsstream.bio_out);
	if (pending <= 0) {
		return pending;
	}

	/* Try to reuse previously allocated object */
	if (sock->tlsstream.send_req != NULL) {
		send_req = sock->tlsstream.send_req;
		send_req->finish = finish;
		sock->tlsstream.send_req = NULL;
	} else {
		send_req = isc_mem_get(sock->worker->mctx, sizeof(*send_req));
		*send_req = (isc_nmsocket_tls_send_req_t){ .finish = finish };
		new_send_req = true;
	}

	if (new_send_req) {
		isc_buffer_init(&send_req->data, &send_req->smallbuf,
				sizeof(send_req->smallbuf));
		isc_buffer_setmctx(&send_req->data, sock->worker->mctx);
	}
	INSIST(isc_buffer_remaininglength(&send_req->data) == 0);

	isc__nmsocket_attach(sock, &send_req->tlssock);
	if (cb != NULL) {
		send_req->cb = cb;
		send_req->cbarg = cbarg;
		isc_nmhandle_attach(tlshandle, &send_req->handle);
	}

	RUNTIME_CHECK(isc_buffer_reserve(&send_req->data, pending) ==
		      ISC_R_SUCCESS);
	isc_buffer_add(&send_req->data, pending);
	rv = BIO_read_ex(sock->tlsstream.bio_out,
			 isc_buffer_base(&send_req->data), pending, &len);
	/* There's something pending, read must succeed */
	RUNTIME_CHECK(rv == 1 && len == (size_t)pending);

	INSIST(VALID_NMHANDLE(sock->outerhandle));

	sock->tlsstream.nsending++;
	isc_buffer_remainingregion(&send_req->data, &used_region);
	isc_nm_send(sock->outerhandle, &used_region, tls_senddone, send_req);

	return pending;
}

static int
tls_process_outgoing(isc_nmsocket_t *sock, bool finish,
		     isc__nm_uvreq_t *send_data) {
	int pending;

	bool received_shutdown = ((SSL_get_shutdown(sock->tlsstream.tls) &
				   SSL_RECEIVED_SHUTDOWN) != 0);
	bool sent_shutdown = ((SSL_get_shutdown(sock->tlsstream.tls) &
			       SSL_SENT_SHUTDOWN) != 0);

	if (received_shutdown && !sent_shutdown) {
		finish = true;
	}

	/* Data from TLS to network */
	if (send_data != NULL) {
		pending = tls_send_outgoing(sock, finish, send_data->handle,
					    send_data->cb.send,
					    send_data->cbarg);
	} else {
		pending = tls_send_outgoing(sock, finish, NULL, NULL, NULL);
	}

	return pending;
}

static int
tls_try_handshake(isc_nmsocket_t *sock, isc_result_t *presult) {
	REQUIRE(sock->tlsstream.state == TLS_HANDSHAKE);

	if (SSL_is_init_finished(sock->tlsstream.tls) == 1) {
		return 0;
	}

	int rv = SSL_do_handshake(sock->tlsstream.tls);
	if (rv == 1) {
		isc_nmhandle_t *tlshandle = NULL;
		isc_result_t result = ISC_R_SUCCESS;

		REQUIRE(sock->statichandle == NULL);
		INSIST(SSL_is_init_finished(sock->tlsstream.tls) == 1);

		isc__nmsocket_log_tls_session_reuse(sock, sock->tlsstream.tls);
		tlshandle = isc__nmhandle_get(sock, &sock->peer, &sock->iface);
		tls_read_stop(sock);

		if (isc__nm_closing(sock->worker)) {
			result = ISC_R_SHUTTINGDOWN;
		}

		if (sock->tlsstream.server) {
			/*
			 * The listening sockets are now closed from outer
			 * to inner order, which means that this function
			 * will never be called when the outer socket has
			 * stopped listening.
			 *
			 * Also see 'isc__nmsocket_stop()' - the function used
			 * to shut down the listening TLS socket - for more
			 * details.
			 */
			if (result == ISC_R_SUCCESS) {
				result = sock->accept_cb(tlshandle, result,
							 sock->accept_cbarg);
			}
		} else {
			tls_call_connect_cb(sock, tlshandle, result);
		}
		isc_nmhandle_detach(&tlshandle);
		sock->tlsstream.state = TLS_IO;

		if (presult != NULL) {
			*presult = result;
		}
	}

	return rv;
}

static bool
tls_try_to_close_unused_socket(isc_nmsocket_t *sock) {
	if (sock->tlsstream.state > TLS_HANDSHAKE &&
	    sock->statichandle == NULL && sock->tlsstream.nsending == 0)
	{
		/*
		 * It seems that no action on the socket has been
		 * scheduled on some point after the handshake, let's
		 * close the connection.
		 */
		isc__nmsocket_prep_destroy(sock);
		return true;
	}

	return false;
}

static void
tls_do_bio(isc_nmsocket_t *sock, isc_region_t *received_data,
	   isc__nm_uvreq_t *send_data, bool finish) {
	isc_result_t result = ISC_R_SUCCESS;
	int pending, tls_status = SSL_ERROR_NONE;
	int rv = 0;
	size_t len = 0;
	int saved_errno = 0;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	/*
	 * Clear the TLS error queue so that SSL_get_error() and SSL I/O
	 * routine calls will not get affected by prior error statuses.
	 *
	 * See here:
	 * https://www.openssl.org/docs/man3.0/man3/SSL_get_error.html
	 *
	 * In particular, it mentions the following:
	 *
	 * The current thread's error queue must be empty before the
	 * TLS/SSL I/O operation is attempted, or SSL_get_error() will not
	 * work reliably.
	 *
	 * As we use the result of SSL_get_error() to decide on I/O
	 * operations, we need to ensure that it works reliably by
	 * cleaning the error queue.
	 *
	 * The sum of details: https://stackoverflow.com/a/37980911
	 */
	ERR_clear_error();

	if (sock->tlsstream.state == TLS_INIT) {
		INSIST(received_data == NULL && send_data == NULL);
		if (sock->tlsstream.server) {
			SSL_set_accept_state(sock->tlsstream.tls);
		} else {
			SSL_set_connect_state(sock->tlsstream.tls);
		}
		sock->tlsstream.state = TLS_HANDSHAKE;
		rv = tls_try_handshake(sock, NULL);
		INSIST(SSL_is_init_finished(sock->tlsstream.tls) == 0);
		isc__nmsocket_timer_restart(sock);
	} else if (sock->tlsstream.state == TLS_CLOSED) {
		return;
	} else { /* initialised and doing I/O */
		if (received_data != NULL) {
			INSIST(send_data == NULL);
			rv = BIO_write_ex(sock->tlsstream.bio_in,
					  received_data->base,
					  received_data->length, &len);
			if (rv <= 0 || len != received_data->length) {
				result = ISC_R_TLSERROR;
#if ISC_NETMGR_TRACE
				saved_errno = errno;
#endif
				goto error;
			}

			/*
			 * Only after doing the IO we can check whether SSL
			 * handshake is done.
			 */
			if (sock->tlsstream.state == TLS_HANDSHAKE) {
				isc_result_t hs_result = ISC_R_UNSET;
				rv = tls_try_handshake(sock, &hs_result);
				if (sock->tlsstream.state == TLS_IO &&
				    hs_result != ISC_R_SUCCESS)
				{
					/*
					 * The accept/connect callback has been
					 * called unsuccessfully. Let's try to
					 * shut down the TLS connection
					 * gracefully.
					 */
					INSIST(SSL_is_init_finished(
						       sock->tlsstream.tls) ==
					       1);
					finish = true;
				}
			}
		} else if (send_data != NULL) {
			INSIST(received_data == NULL);
			INSIST(sock->tlsstream.state > TLS_HANDSHAKE);
			bool received_shutdown =
				((SSL_get_shutdown(sock->tlsstream.tls) &
				  SSL_RECEIVED_SHUTDOWN) != 0);
			bool sent_shutdown =
				((SSL_get_shutdown(sock->tlsstream.tls) &
				  SSL_SENT_SHUTDOWN) != 0);
			bool write_failed = false;
			if (*(uint16_t *)send_data->tcplen != 0) {
				size_t sendlen = 0;
				uint8_t sendbuf[MAX_DNS_MESSAGE_SIZE +
						sizeof(uint16_t)];
				/*
				 * There is a DNS message length to write - do
				 * it.
				 */

				/*
				 * There's no SSL_writev(), so we need to use a
				 * local buffer to assemble the whole message
				 */
				INSIST(send_data->uvbuf.len <=
				       MAX_DNS_MESSAGE_SIZE);

				sendlen = send_data->uvbuf.len +
					  sizeof(uint16_t);
				memmove(sendbuf, send_data->tcplen,
					sizeof(uint16_t));
				memmove(sendbuf + sizeof(uint16_t),
					send_data->uvbuf.base,
					send_data->uvbuf.len);

				/* Write data */
				rv = SSL_write_ex(sock->tlsstream.tls, sendbuf,
						  sendlen, &len);
				if (rv != 1 || len != sendlen) {
					write_failed = true;
				}
			} else {
				/* Write data only */
				rv = SSL_write_ex(sock->tlsstream.tls,
						  send_data->uvbuf.base,
						  send_data->uvbuf.len, &len);
				if (rv != 1 || len != send_data->uvbuf.len) {
					write_failed = true;
				}
			}

			if (write_failed) {
				result = received_shutdown || sent_shutdown
						 ? ISC_R_CANCELED
						 : ISC_R_TLSERROR;
				send_data->cb.send(send_data->handle, result,
						   send_data->cbarg);
				send_data = NULL;
				return;
			}
		}

		/* Decrypt and pass data from network to client */
		if (sock->tlsstream.state >= TLS_IO && sock->recv_cb != NULL &&
		    sock->statichandle != NULL && sock->reading && !finish)
		{
			bool was_new_data = false;
			uint8_t recv_buf[TLS_BUF_SIZE];
			INSIST(sock->tlsstream.state > TLS_HANDSHAKE);
			while ((rv = SSL_read_ex(sock->tlsstream.tls, recv_buf,
						 TLS_BUF_SIZE, &len)) == 1)
			{
				isc_region_t region;
				region = (isc_region_t){ .base = &recv_buf[0],
							 .length = len };

				was_new_data = true;
				INSIST(VALID_NMHANDLE(sock->statichandle));
				sock->recv_cb(sock->statichandle, ISC_R_SUCCESS,
					      &region, sock->recv_cbarg);
				/* The handle could have been detached in
				 * sock->recv_cb, making the sock->statichandle
				 * nullified (it happens in netmgr.c). If it is
				 * the case, then it means that we are not
				 * interested in keeping the connection alive
				 * anymore. Let's shut down the SSL session,
				 * send what we have in the SSL buffers,
				 * and close the connection.
				 */
				if (sock->statichandle == NULL) {
					finish = true;
					break;
				} else if (sock->recv_cb == NULL) {
					/*
					 * The 'sock->recv_cb' might have been
					 * nullified during the call to
					 * 'sock->recv_cb'. That could happen,
					 * e.g. by an indirect call to
					 * 'isc_nmhandle_close()' from within
					 * the callback when wrapping up.
					 *
					 * In this case, let's close the TLS
					 * connection.
					 */
					finish = true;
					break;
				} else if (!sock->reading) {
					/*
					 * Reading has been paused from withing
					 * the context of read callback - stop
					 * processing incoming data.
					 */
					break;
				}
			}

			if (was_new_data && !sock->manual_read_timer) {
				/*
				 * Some data has been decrypted, it is the right
				 * time to stop the read timer as it will be
				 * restarted on the next read attempt.
				 */
				isc__nmsocket_timer_stop(sock);
			}
		}
	}

	/*
	 * Setting 'finish' to 'true' means that we are about to close the
	 * TLS stream (we intend to send TLS shutdown message to the
	 * remote side). After that no new data can be received, so we
	 * should stop the timer regardless of the
	 * 'sock->manual_read_timer' value.
	 */
	if (finish) {
		isc__nmsocket_timer_stop(sock);
	}

	errno = 0;
	tls_status = SSL_get_error(sock->tlsstream.tls, rv);
	saved_errno = errno;

	/* See "BUGS" section at:
	 * https://www.openssl.org/docs/man1.1.1/man3/SSL_get_error.html
	 *
	 * It is mentioned there that when TLS status equals
	 * SSL_ERROR_SYSCALL AND errno == 0 it means that underlying
	 * transport layer returned EOF prematurely.  However, we are
	 * managing the transport ourselves, so we should just resume
	 * reading from the TCP socket.
	 *
	 * It seems that this case has been handled properly on modern
	 * versions of OpenSSL. That being said, the situation goes in
	 * line with the manual: it is briefly mentioned there that
	 * SSL_ERROR_SYSCALL might be returned not only in a case of
	 * low-level errors (like system call failures).
	 */
	if (tls_status == SSL_ERROR_SYSCALL && saved_errno == 0 &&
	    received_data == NULL && send_data == NULL && finish == false)
	{
		tls_status = SSL_ERROR_WANT_READ;
	}

	pending = tls_process_outgoing(sock, finish, send_data);
	if (pending > 0 && tls_status != SSL_ERROR_SSL) {
		return;
	}

	switch (tls_status) {
	case SSL_ERROR_NONE:
	case SSL_ERROR_ZERO_RETURN:
		(void)tls_try_to_close_unused_socket(sock);
		return;
	case SSL_ERROR_WANT_WRITE:
		if (sock->tlsstream.nsending == 0) {
			/*
			 * Launch tls_do_bio asynchronously. If we're sending
			 * already the send callback will call it.
			 */
			async_tls_do_bio(sock);
		}
		return;
	case SSL_ERROR_WANT_READ:
		if (tls_try_to_close_unused_socket(sock) ||
		    sock->outerhandle == NULL)
		{
			return;
		} else if (sock->reading == false &&
			   sock->tlsstream.state == TLS_HANDSHAKE)
		{
			/*
			 * We need to read data when doing handshake even if
			 * 'sock->reading == false'. It will be stopped when
			 * handshake is completed.
			 */
			tls_read_start(sock);
			return;
		} else if (sock->reading == false) {
			return;
		}

		tls_read_start(sock);
		return;
	default:
		result = tls_error_to_result(tls_status, sock->tlsstream.state,
					     sock->tlsstream.tls);
		break;
	}

error:
#if ISC_NETMGR_TRACE
	isc__nmsocket_log(sock, ISC_LOG_NOTICE,
			  "SSL error in BIO: %d %s (errno: %d). Arguments: "
			  "received_data: %p, "
			  "send_data: %p, finish: %s",
			  tls_status, isc_result_totext(result), saved_errno,
			  received_data, send_data, finish ? "true" : "false");
#endif
	tls_failed_read_cb(sock, result);
}

static void
tls_readcb(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *region,
	   void *cbarg) {
	isc_nmsocket_t *tlssock = (isc_nmsocket_t *)cbarg;

	REQUIRE(VALID_NMSOCK(tlssock));
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(tlssock->tid == isc_tid());

	if (result != ISC_R_SUCCESS) {
		tls_failed_read_cb(tlssock, result);
		return;
	} else if (isc__nmsocket_closing(handle->sock)) {
		tls_failed_read_cb(tlssock, ISC_R_CANCELED);
		return;
	}

	REQUIRE(handle == tlssock->outerhandle);
	tls_do_bio(tlssock, region, NULL, false);
}

static isc_result_t
initialize_tls(isc_nmsocket_t *sock, bool server) {
	REQUIRE(sock->tid == isc_tid());

	sock->tlsstream.bio_in = BIO_new(BIO_s_mem());
	if (sock->tlsstream.bio_in == NULL) {
		isc_tls_free(&sock->tlsstream.tls);
		return ISC_R_TLSERROR;
	}
	sock->tlsstream.bio_out = BIO_new(BIO_s_mem());
	if (sock->tlsstream.bio_out == NULL) {
		BIO_free_all(sock->tlsstream.bio_in);
		sock->tlsstream.bio_in = NULL;
		isc_tls_free(&sock->tlsstream.tls);
		return ISC_R_TLSERROR;
	}

	if (BIO_set_mem_eof_return(sock->tlsstream.bio_in, EOF) != 1 ||
	    BIO_set_mem_eof_return(sock->tlsstream.bio_out, EOF) != 1)
	{
		goto error;
	}

	SSL_set_bio(sock->tlsstream.tls, sock->tlsstream.bio_in,
		    sock->tlsstream.bio_out);
	sock->tlsstream.server = server;
	sock->tlsstream.nsending = 0;
	sock->tlsstream.state = TLS_INIT;
	return ISC_R_SUCCESS;
error:
	isc_tls_free(&sock->tlsstream.tls);
	sock->tlsstream.bio_out = sock->tlsstream.bio_in = NULL;
	return ISC_R_TLSERROR;
}

static void
tls_try_to_enable_tcp_nodelay(isc_nmsocket_t *tlssock) {
	/*
	 * Try to enable TCP_NODELAY for TLS connections by default to speed up
	 * the handshakes, just like other software (e.g. NGINX) does.
	 */
	isc_result_t result = isc_nmhandle_set_tcp_nodelay(tlssock->outerhandle,
							   true);
	tlssock->tlsstream.tcp_nodelay_value = (result == ISC_R_SUCCESS);
}

static isc_result_t
tlslisten_acceptcb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *tlslistensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *tlssock = NULL;
	isc_tlsctx_t *tlsctx = NULL;
	isc_sockaddr_t local;

	/* If accept() was unsuccessful we can't do anything */
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(VALID_NMSOCK(tlslistensock));
	REQUIRE(tlslistensock->type == isc_nm_tlslistener);

	if (isc__nm_closing(handle->sock->worker)) {
		return ISC_R_SHUTTINGDOWN;
	} else if (isc__nmsocket_closing(handle->sock)) {
		return ISC_R_CANCELED;
	}

	local = isc_nmhandle_localaddr(handle);
	/*
	 * We need to create a 'wrapper' tlssocket for this connection.
	 */
	tlssock = isc_mempool_get(handle->sock->worker->nmsocket_pool);
	isc__nmsocket_init(tlssock, handle->sock->worker, isc_nm_tlssocket,
			   &local, NULL);
	isc__nmsocket_attach(tlslistensock, &tlssock->server);

	/* We need to initialize SSL now to reference SSL_CTX properly */
	tlsctx = tls_get_listener_tlsctx(tlslistensock, isc_tid());
	RUNTIME_CHECK(tlsctx != NULL);
	isc_tlsctx_attach(tlsctx, &tlssock->tlsstream.ctx);
	tlssock->tlsstream.tls = isc_tls_create(tlssock->tlsstream.ctx);
	if (tlssock->tlsstream.tls == NULL) {
		tlssock->closed = true;
		isc_tlsctx_free(&tlssock->tlsstream.ctx);
		isc__nmsocket_detach(&tlssock);
		return ISC_R_TLSERROR;
	}

	tlssock->accept_cb = tlslistensock->accept_cb;
	tlssock->accept_cbarg = tlslistensock->accept_cbarg;
	isc__nmsocket_attach(handle->sock, &tlssock->listener);
	isc_nmhandle_attach(handle, &tlssock->outerhandle);
	tlssock->peer = isc_nmhandle_peeraddr(handle);
	tlssock->read_timeout =
		atomic_load_relaxed(&handle->sock->worker->netmgr->init);

	/*
	 * Hold a reference to tlssock in the TCP socket: it will
	 * detached in isc__nm_tls_cleanup_data().
	 */
	handle->sock->tlsstream.tlssocket = tlssock;

	result = initialize_tls(tlssock, true);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	/* TODO: catch failure code, detach tlssock, and log the error */

	tls_try_to_enable_tcp_nodelay(tlssock);

	isc__nmhandle_set_manual_timer(tlssock->outerhandle, true);
	tls_do_bio(tlssock, NULL, NULL, false);
	return result;
}

isc_result_t
isc_nm_listentls(isc_nm_t *mgr, uint32_t workers, isc_sockaddr_t *iface,
		 isc_nm_accept_cb_t accept_cb, void *accept_cbarg, int backlog,
		 isc_quota_t *quota, SSL_CTX *sslctx, bool proxy,
		 isc_nmsocket_t **sockp) {
	isc_result_t result;
	isc_nmsocket_t *tlssock = NULL;
	isc_nmsocket_t *tsock = NULL;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(isc_tid() == 0);

	worker = &mgr->workers[isc_tid()];

	if (isc__nm_closing(worker)) {
		return ISC_R_SHUTTINGDOWN;
	}

	if (workers == 0) {
		workers = mgr->nloops;
	}
	REQUIRE(workers <= mgr->nloops);

	tlssock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(tlssock, worker, isc_nm_tlslistener, iface, NULL);
	tlssock->accept_cb = accept_cb;
	tlssock->accept_cbarg = accept_cbarg;
	tls_init_listener_tlsctx(tlssock, sslctx);
	tlssock->tlsstream.tls = NULL;

	/*
	 * tlssock will be a TLS 'wrapper' around an unencrypted stream.
	 * We set tlssock->outer to a socket listening for a TCP connection.
	 */
	if (proxy) {
		result = isc_nm_listenproxystream(
			mgr, workers, iface, tlslisten_acceptcb, tlssock,
			backlog, quota, NULL, &tlssock->outer);
	} else {
		result = isc_nm_listentcp(mgr, workers, iface,
					  tlslisten_acceptcb, tlssock, backlog,
					  quota, &tlssock->outer);
	}
	if (result != ISC_R_SUCCESS) {
		tlssock->closed = true;
		isc__nmsocket_detach(&tlssock);
		return result;
	}

	/* copy the actual port we're listening on into sock->iface */
	if (isc_sockaddr_getport(iface) == 0) {
		tlssock->iface = tlssock->outer->iface;
	}

	/* wait for listen result */
	isc__nmsocket_attach(tlssock->outer, &tsock);
	tlssock->result = result;
	tlssock->active = true;
	INSIST(tlssock->outer->tlsstream.tlslistener == NULL);
	isc__nmsocket_attach(tlssock, &tlssock->outer->tlsstream.tlslistener);
	isc__nmsocket_detach(&tsock);
	INSIST(result != ISC_R_UNSET);
	tlssock->nchildren = tlssock->outer->nchildren;

	if (result == ISC_R_SUCCESS) {
		*sockp = tlssock;
	}

	return result;
}

static void
tls_send_direct(void *arg) {
	isc__nm_uvreq_t *req = arg;

	REQUIRE(VALID_UVREQ(req));

	isc_nmsocket_t *sock = req->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_tid());

	if (isc__nm_closing(sock->worker)) {
		req->cb.send(req->handle, ISC_R_SHUTTINGDOWN, req->cbarg);
		goto done;
	} else if (inactive(sock)) {
		req->cb.send(req->handle, ISC_R_CANCELED, req->cbarg);
		goto done;
	}

	tls_do_bio(sock, NULL, req, false);
done:
	isc__nm_uvreq_put(&req);
	return;
}

static void
tls_send(isc_nmhandle_t *handle, const isc_region_t *region, isc_nm_cb_t cb,
	 void *cbarg, const bool dnsmsg) {
	isc__nm_uvreq_t *uvreq = NULL;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_tlssocket);

	uvreq = isc__nm_uvreq_get(sock);
	isc_nmhandle_attach(handle, &uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;
	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;
	if (dnsmsg) {
		*(uint16_t *)uvreq->tcplen = htons(region->length);
	}

	isc_job_run(sock->worker->loop, &uvreq->job, tls_send_direct, uvreq);
}

void
isc__nm_tls_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg) {
	tls_send(handle, region, cb, cbarg, false);
}

void
isc__nm_tls_senddns(isc_nmhandle_t *handle, const isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg) {
	tls_send(handle, region, cb, cbarg, true);
}

void
isc__nm_tls_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->statichandle == handle);
	REQUIRE(sock->tid == isc_tid());

	if (isc__nm_closing(sock->worker)) {
		cb(handle, ISC_R_SHUTTINGDOWN, NULL, cbarg);
		return;
	} else if (inactive(sock)) {
		cb(handle, ISC_R_CANCELED, NULL, cbarg);
		return;
	}

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;
	sock->reading = true;

	async_tls_do_bio(sock);
}

static void
tls_read_start(isc_nmsocket_t *restrict sock) {
	if (sock->tlsstream.reading) {
		return;
	}
	sock->tlsstream.reading = true;

	INSIST(VALID_NMHANDLE(sock->outerhandle));

	isc_nm_read(sock->outerhandle, tls_readcb, sock);
	if (!sock->manual_read_timer) {
		isc__nmsocket_timer_start(sock);
	}
}

static void
tls_read_stop(isc_nmsocket_t *sock) {
	sock->tlsstream.reading = false;
	if (sock->outerhandle != NULL) {
		isc_nm_read_stop(sock->outerhandle);
	}
}

void
isc__nm_tls_read_stop(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	handle->sock->reading = false;

	tls_read_stop(handle->sock);
}

void
isc__nm_tls_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);
	REQUIRE(!sock->closing);
	REQUIRE(sock->tid == isc_tid());
	REQUIRE(!sock->closed);
	REQUIRE(!sock->closing);

	sock->closing = true;

	/*
	 * At this point we're certain that there are no
	 * external references, we can close everything.
	 */
	tls_read_stop(sock);
	if (sock->outerhandle != NULL) {
		isc_nm_read_stop(sock->outerhandle);
		isc_nmhandle_close(sock->outerhandle);
		isc_nmhandle_detach(&sock->outerhandle);
	}

	if (sock->listener != NULL) {
		isc__nmsocket_detach(&sock->listener);
	}

	if (sock->server != NULL) {
		isc__nmsocket_detach(&sock->server);
	}

	/* Further cleanup performed in isc__nm_tls_cleanup_data() */
	sock->closed = true;
	sock->active = false;
	sock->tlsstream.state = TLS_CLOSED;
}

void
isc__nm_tls_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlslistener);
	REQUIRE(sock->tlsstream.tls == NULL);
	REQUIRE(sock->tlsstream.ctx == NULL);

	isc__nmsocket_stop(sock);
}

static void
tcp_connected(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);

void
isc_nm_tlsconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		  isc_nm_cb_t connect_cb, void *connect_cbarg,
		  isc_tlsctx_t *ctx,
		  isc_tlsctx_client_session_cache_t *client_sess_cache,
		  unsigned int timeout, bool proxy,
		  isc_nm_proxyheader_info_t *proxy_info) {
	isc_nmsocket_t *sock = NULL;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NM(mgr));

	worker = &mgr->workers[isc_tid()];

	if (isc__nm_closing(worker)) {
		connect_cb(NULL, ISC_R_SHUTTINGDOWN, connect_cbarg);
		return;
	}

	sock = isc_mempool_get(worker->nmsocket_pool);
	isc__nmsocket_init(sock, worker, isc_nm_tlssocket, local, NULL);
	sock->connect_cb = connect_cb;
	sock->connect_cbarg = connect_cbarg;
	sock->connect_timeout = timeout;
	isc_tlsctx_attach(ctx, &sock->tlsstream.ctx);
	sock->client = true;
	if (client_sess_cache != NULL) {
		INSIST(isc_tlsctx_client_session_cache_getctx(
			       client_sess_cache) == ctx);
		isc_tlsctx_client_session_cache_attach(
			client_sess_cache, &sock->tlsstream.client_sess_cache);
	}

	if (proxy) {
		isc_nm_proxystreamconnect(mgr, local, peer, tcp_connected, sock,
					  sock->connect_timeout, NULL, NULL,
					  proxy_info);
	} else {
		isc_nm_tcpconnect(mgr, local, peer, tcp_connected, sock,
				  sock->connect_timeout);
	}
}

static void
tcp_connected(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *tlssock = (isc_nmsocket_t *)cbarg;
	isc_nmhandle_t *tlshandle = NULL;
	isc__networker_t *worker = NULL;

	REQUIRE(VALID_NMSOCK(tlssock));

	worker = tlssock->worker;

	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	INSIST(VALID_NMHANDLE(handle));

	tlssock->iface = isc_nmhandle_localaddr(handle);
	tlssock->peer = isc_nmhandle_peeraddr(handle);
	if (isc__nm_closing(worker)) {
		result = ISC_R_SHUTTINGDOWN;
		goto error;
	} else if (isc__nmsocket_closing(handle->sock)) {
		result = ISC_R_CANCELED;
		goto error;
	}

	/*
	 * We need to initialize SSL now to reference SSL_CTX properly.
	 */
	tlssock->tlsstream.tls = isc_tls_create(tlssock->tlsstream.ctx);
	if (tlssock->tlsstream.tls == NULL) {
		result = ISC_R_TLSERROR;
		goto error;
	}

	result = initialize_tls(tlssock, false);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}
	tlssock->peer = isc_nmhandle_peeraddr(handle);
	isc_nmhandle_attach(handle, &tlssock->outerhandle);
	tlssock->active = true;

	if (tlssock->tlsstream.client_sess_cache != NULL) {
		isc_tlsctx_client_session_cache_reuse_sockaddr(
			tlssock->tlsstream.client_sess_cache, &tlssock->peer,
			tlssock->tlsstream.tls);
	}

	/*
	 * Hold a reference to tlssock in the TCP socket: it will
	 * detached in isc__nm_tls_cleanup_data().
	 */
	handle->sock->tlsstream.tlssocket = tlssock;

	tls_try_to_enable_tcp_nodelay(tlssock);

	isc__nmhandle_set_manual_timer(tlssock->outerhandle, true);
	tls_do_bio(tlssock, NULL, NULL, false);
	return;
error:
	tlshandle = isc__nmhandle_get(tlssock, NULL, NULL);
	tlssock->closed = true;
	tls_call_connect_cb(tlssock, tlshandle, result);
	isc_nmhandle_detach(&tlshandle);
	isc__nmsocket_detach(&tlssock);
}

void
isc__nm_tls_cleanup_data(isc_nmsocket_t *sock) {
	if ((sock->type == isc_nm_tcplistener ||
	     sock->type == isc_nm_proxystreamlistener) &&
	    sock->tlsstream.tlslistener != NULL)
	{
		isc__nmsocket_detach(&sock->tlsstream.tlslistener);
	} else if (sock->type == isc_nm_tlslistener) {
		tls_cleanup_listener_tlsctx(sock);
	} else if (sock->type == isc_nm_tlssocket) {
		if (sock->tlsstream.tls != NULL) {
			/*
			 * Let's shut down the TLS session properly so that
			 * the session will remain resumable, if required.
			 */
			tls_try_shutdown(sock->tlsstream.tls, true);
			tls_keep_client_tls_session(sock);
			isc_tls_free(&sock->tlsstream.tls);
			/* These are destroyed when we free SSL */
			sock->tlsstream.bio_out = NULL;
			sock->tlsstream.bio_in = NULL;
		}
		if (sock->tlsstream.ctx != NULL) {
			isc_tlsctx_free(&sock->tlsstream.ctx);
		}
		if (sock->tlsstream.client_sess_cache != NULL) {
			INSIST(sock->client);
			isc_tlsctx_client_session_cache_detach(
				&sock->tlsstream.client_sess_cache);
		}

		if (sock->tlsstream.send_req != NULL) {
			isc_buffer_clearmctx(&sock->tlsstream.send_req->data);
			isc_buffer_invalidate(&sock->tlsstream.send_req->data);
			isc_mem_put(sock->worker->mctx,
				    sock->tlsstream.send_req,
				    sizeof(*sock->tlsstream.send_req));
		}
	} else if ((sock->type == isc_nm_tcpsocket ||
		    sock->type == isc_nm_proxystreamsocket) &&
		   sock->tlsstream.tlssocket != NULL)
	{
		/*
		 * The TLS socket can't be destroyed until its underlying TCP
		 * socket is, to avoid possible use-after-free errors.
		 */
		isc__nmsocket_detach(&sock->tlsstream.tlssocket);
	}
}

void
isc__nm_tls_cleartimeout(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_cleartimeout(sock->outerhandle);
	}
}

void
isc__nm_tls_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nmhandle_settimeout(sock->outerhandle, timeout);
	}
}

void
isc__nmhandle_tls_keepalive(isc_nmhandle_t *handle, bool value) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nmhandle_keepalive(sock->outerhandle, value);
	}
}

void
isc__nmhandle_tls_setwritetimeout(isc_nmhandle_t *handle,
				  uint64_t write_timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		isc_nmhandle_setwritetimeout(sock->outerhandle, write_timeout);
	}
}

void
isc__nmsocket_tls_reset(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_reset(sock->outerhandle->sock);
	}
}

bool
isc__nmsocket_tls_timer_running(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		return isc__nmsocket_timer_running(sock->outerhandle->sock);
	}

	return false;
}

void
isc__nmsocket_tls_timer_restart(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_timer_restart(sock->outerhandle->sock);
	}
}

void
isc__nmsocket_tls_timer_stop(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);

	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		REQUIRE(VALID_NMSOCK(sock->outerhandle->sock));
		isc__nmsocket_timer_stop(sock->outerhandle->sock);
	}
}

const char *
isc__nm_tls_verify_tls_peer_result_string(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlssocket);

	sock = handle->sock;
	if (sock->tlsstream.tls == NULL) {
		return NULL;
	}

	return isc_tls_verify_peer_result_string(sock->tlsstream.tls);
}

static void
tls_init_listener_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *ctx) {
	size_t nworkers;

	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(ctx != NULL);

	nworkers =
		(size_t)isc_loopmgr_nloops(listener->worker->netmgr->loopmgr);
	INSIST(nworkers > 0);

	listener->tlsstream.listener_tls_ctx = isc_mem_cget(
		listener->worker->mctx, nworkers, sizeof(isc_tlsctx_t *));
	listener->tlsstream.n_listener_tls_ctx = nworkers;
	for (size_t i = 0; i < nworkers; i++) {
		listener->tlsstream.listener_tls_ctx[i] = NULL;
		isc_tlsctx_attach(ctx,
				  &listener->tlsstream.listener_tls_ctx[i]);
	}
}

static void
tls_cleanup_listener_tlsctx(isc_nmsocket_t *listener) {
	REQUIRE(VALID_NMSOCK(listener));

	if (listener->tlsstream.listener_tls_ctx == NULL) {
		return;
	}

	for (size_t i = 0; i < listener->tlsstream.n_listener_tls_ctx; i++) {
		isc_tlsctx_free(&listener->tlsstream.listener_tls_ctx[i]);
	}
	isc_mem_cput(
		listener->worker->mctx, listener->tlsstream.listener_tls_ctx,
		listener->tlsstream.n_listener_tls_ctx, sizeof(isc_tlsctx_t *));
	listener->tlsstream.n_listener_tls_ctx = 0;
}

static isc_tlsctx_t *
tls_get_listener_tlsctx(isc_nmsocket_t *listener, const int tid) {
	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(tid >= 0);

	if (listener->tlsstream.listener_tls_ctx == NULL) {
		return NULL;
	}

	return listener->tlsstream.listener_tls_ctx[tid];
}

void
isc__nm_async_tls_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx,
			     const int tid) {
	REQUIRE(tid >= 0);

	isc_tlsctx_free(&listener->tlsstream.listener_tls_ctx[tid]);
	isc_tlsctx_attach(tlsctx, &listener->tlsstream.listener_tls_ctx[tid]);
}

static void
tls_keep_client_tls_session(isc_nmsocket_t *sock) {
	/*
	 * Ensure that the isc_tls_t is being accessed from
	 * within the worker thread the socket is bound to.
	 */
	REQUIRE(sock->tid == isc_tid());
	if (sock->tlsstream.client_sess_cache != NULL &&
	    sock->tlsstream.client_session_saved == false)
	{
		INSIST(sock->client);
		isc_tlsctx_client_session_cache_keep_sockaddr(
			sock->tlsstream.client_sess_cache, &sock->peer,
			sock->tlsstream.tls);
		sock->tlsstream.client_session_saved = true;
	}
}

static void
tls_try_shutdown(isc_tls_t *tls, const bool force) {
	if (force) {
		(void)SSL_set_shutdown(tls, SSL_SENT_SHUTDOWN);
	} else if ((SSL_get_shutdown(tls) & SSL_SENT_SHUTDOWN) == 0) {
		(void)SSL_shutdown(tls);
	}
}

void
isc__nmhandle_tls_set_manual_timer(isc_nmhandle_t *handle, const bool manual) {
	isc_nmsocket_t *sock;

	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);
	REQUIRE(sock->tid == isc_tid());

	sock->manual_read_timer = manual;
}

void
isc__nmhandle_tls_get_selected_alpn(isc_nmhandle_t *handle,
				    const unsigned char **alpn,
				    unsigned int *alpnlen) {
	isc_nmsocket_t *sock;

	REQUIRE(VALID_NMHANDLE(handle));
	sock = handle->sock;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);
	REQUIRE(sock->tid == isc_tid());

	isc_tls_get_selected_alpn(sock->tlsstream.tls, alpn, alpnlen);
}

isc_result_t
isc__nmhandle_tls_set_tcp_nodelay(isc_nmhandle_t *handle, const bool value) {
	isc_nmsocket_t *sock = NULL;
	isc_result_t result = ISC_R_FAILURE;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_tlssocket);

	sock = handle->sock;
	if (sock->outerhandle != NULL) {
		INSIST(VALID_NMHANDLE(sock->outerhandle));

		if (value == sock->tlsstream.tcp_nodelay_value) {
			result = ISC_R_SUCCESS;
		} else {
			result = isc_nmhandle_set_tcp_nodelay(sock->outerhandle,
							      value);
			if (result == ISC_R_SUCCESS) {
				sock->tlsstream.tcp_nodelay_value = value;
			}
		}
	}

	return result;
}
