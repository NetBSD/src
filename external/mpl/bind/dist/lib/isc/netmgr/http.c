/*	$NetBSD: http.c,v 1.2.2.2 2024/02/25 15:47:24 martin Exp $	*/

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

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <nghttp2/nghttp2.h>
#include <signal.h>
#include <string.h>

#include <isc/base64.h>
#include <isc/log.h>
#include <isc/netmgr.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/tls.h>
#include <isc/url.h>
#include <isc/util.h>

#include "netmgr-int.h"

#define AUTHEXTRA 7

#define MAX_DNS_MESSAGE_SIZE (UINT16_MAX)

#define DNS_MEDIA_TYPE "application/dns-message"

/*
 * See https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cache-Control
 * for additional details. Basically it means "avoid caching by any
 * means."
 */
#define DEFAULT_CACHE_CONTROL "no-cache, no-store, must-revalidate"

/*
 * If server during request processing surpasses any of the limits
 * below, it will just reset the stream without returning any error
 * codes in a response.  Ideally, these parameters should be
 * configurable both globally and per every HTTP endpoint description
 * in the configuration file, but for now it should be enough.
 */

/*
 * 128K should be enough to encode 64K of data into base64url inside GET
 * request and have extra space for other headers
 */
#define MAX_ALLOWED_DATA_IN_HEADERS (MAX_DNS_MESSAGE_SIZE * 2)

#define MAX_ALLOWED_DATA_IN_POST \
	(MAX_DNS_MESSAGE_SIZE + MAX_DNS_MESSAGE_SIZE / 2)

#define HEADER_MATCH(header, name, namelen)   \
	(((namelen) == sizeof(header) - 1) && \
	 (strncasecmp((header), (const char *)(name), (namelen)) == 0))

#define MIN_SUCCESSFUL_HTTP_STATUS (200)
#define MAX_SUCCESSFUL_HTTP_STATUS (299)

/* This definition sets the upper limit of pending write buffer to an
 * adequate enough value. That is done mostly to fight a limitation
 * for a max TLS record size in flamethrower (2K).  In a perfect world
 * this constant should not be required, if we ever move closer to
 * that state, the constant, and corresponding code, should be
 * removed. For now the limit seems adequate enough to fight
 * "tinygrams" problem. */
#define FLUSH_HTTP_WRITE_BUFFER_AFTER (1536)

/* This switch is here mostly to test the code interoperability with
 * buggy implementations */
#define ENABLE_HTTP_WRITE_BUFFERING 1

#define SUCCESSFUL_HTTP_STATUS(code)             \
	((code) >= MIN_SUCCESSFUL_HTTP_STATUS && \
	 (code) <= MAX_SUCCESSFUL_HTTP_STATUS)

#define INITIAL_DNS_MESSAGE_BUFFER_SIZE (512)

typedef struct isc_nm_http_response_status {
	size_t code;
	size_t content_length;
	bool content_type_valid;
} isc_nm_http_response_status_t;

typedef struct http_cstream {
	isc_nm_recv_cb_t read_cb;
	void *read_cbarg;
	isc_nm_cb_t connect_cb;
	void *connect_cbarg;

	bool sending;
	bool reading;

	char *uri;
	isc_url_parser_t up;

	char *authority;
	size_t authoritylen;
	char *path;

	isc_buffer_t *rbuf;

	size_t pathlen;
	int32_t stream_id;

	bool post; /* POST or GET */
	isc_buffer_t *postdata;
	char *GET_path;
	size_t GET_path_len;

	isc_nm_http_response_status_t response_status;
	isc_nmsocket_t *httpsock;
	LINK(struct http_cstream) link;
} http_cstream_t;

#define HTTP2_SESSION_MAGIC    ISC_MAGIC('H', '2', 'S', 'S')
#define VALID_HTTP2_SESSION(t) ISC_MAGIC_VALID(t, HTTP2_SESSION_MAGIC)

typedef ISC_LIST(isc__nm_uvreq_t) isc__nm_http_pending_callbacks_t;

struct isc_nm_http_session {
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;

	size_t sending;
	bool reading;
	bool closed;
	bool closing;

	nghttp2_session *ngsession;
	bool client;

	ISC_LIST(http_cstream_t) cstreams;
	ISC_LIST(isc_nmsocket_h2_t) sstreams;
	size_t nsstreams;

	isc_nmhandle_t *handle;
	isc_nmhandle_t *client_httphandle;
	isc_nmsocket_t *serversocket;

	isc_buffer_t *buf;

	isc_tlsctx_t *tlsctx;
	uint32_t max_concurrent_streams;

	isc__nm_http_pending_callbacks_t pending_write_callbacks;
	isc_buffer_t *pending_write_data;
};

typedef enum isc_http_error_responses {
	ISC_HTTP_ERROR_SUCCESS,		       /* 200 */
	ISC_HTTP_ERROR_NOT_FOUND,	       /* 404 */
	ISC_HTTP_ERROR_PAYLOAD_TOO_LARGE,      /* 413 */
	ISC_HTTP_ERROR_URI_TOO_LONG,	       /* 414 */
	ISC_HTTP_ERROR_UNSUPPORTED_MEDIA_TYPE, /* 415 */
	ISC_HTTP_ERROR_BAD_REQUEST,	       /* 400 */
	ISC_HTTP_ERROR_NOT_IMPLEMENTED,	       /* 501 */
	ISC_HTTP_ERROR_GENERIC,		       /* 500 Internal Server Error */
	ISC_HTTP_ERROR_MAX
} isc_http_error_responses_t;

typedef struct isc_http_send_req {
	isc_nm_http_session_t *session;
	isc_nmhandle_t *transphandle;
	isc_nmhandle_t *httphandle;
	isc_nm_cb_t cb;
	void *cbarg;
	isc_buffer_t *pending_write_data;
	isc__nm_http_pending_callbacks_t pending_write_callbacks;
} isc_http_send_req_t;

#define HTTP_ENDPOINTS_MAGIC	ISC_MAGIC('H', 'T', 'E', 'P')
#define VALID_HTTP_ENDPOINTS(t) ISC_MAGIC_VALID(t, HTTP_ENDPOINTS_MAGIC)

static bool
http_send_outgoing(isc_nm_http_session_t *session, isc_nmhandle_t *httphandle,
		   isc_nm_cb_t cb, void *cbarg);

static void
http_do_bio(isc_nm_http_session_t *session, isc_nmhandle_t *send_httphandle,
	    isc_nm_cb_t send_cb, void *send_cbarg);

static void
failed_httpstream_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			  isc_nm_http_session_t *session);

static void
client_call_failed_read_cb(isc_result_t result, isc_nm_http_session_t *session);

static void
server_call_failed_read_cb(isc_result_t result, isc_nm_http_session_t *session);

static void
failed_read_cb(isc_result_t result, isc_nm_http_session_t *session);

static isc_result_t
server_send_error_response(const isc_http_error_responses_t error,
			   nghttp2_session *ngsession, isc_nmsocket_t *socket);

static isc_result_t
client_send(isc_nmhandle_t *handle, const isc_region_t *region);

static void
finish_http_session(isc_nm_http_session_t *session);

static void
http_transpost_tcp_nodelay(isc_nmhandle_t *transphandle);

static void
call_pending_callbacks(isc__nm_http_pending_callbacks_t pending_callbacks,
		       isc_result_t result);

static void
server_call_cb(isc_nmsocket_t *socket, isc_nm_http_session_t *session,
	       const isc_result_t result, isc_region_t *data);

static isc_nm_httphandler_t *
http_endpoints_find(const char *request_path,
		    const isc_nm_http_endpoints_t *restrict eps);

static void
http_init_listener_endpoints(isc_nmsocket_t *listener,
			     isc_nm_http_endpoints_t *epset);

static void
http_cleanup_listener_endpoints(isc_nmsocket_t *listener);

static isc_nm_http_endpoints_t *
http_get_listener_endpoints(isc_nmsocket_t *listener, const int tid);

static bool
http_session_active(isc_nm_http_session_t *session) {
	REQUIRE(VALID_HTTP2_SESSION(session));
	return (!session->closed && !session->closing);
}

static void *
http_malloc(size_t sz, isc_mem_t *mctx) {
	return (isc_mem_allocate(mctx, sz));
}

static void *
http_calloc(size_t n, size_t sz, isc_mem_t *mctx) {
	const size_t msize = n * sz;
	void *data = isc_mem_allocate(mctx, msize);

	memset(data, 0, msize);
	return (data);
}

static void *
http_realloc(void *p, size_t newsz, isc_mem_t *mctx) {
	return (isc_mem_reallocate(mctx, p, newsz));
}

static void
http_free(void *p, isc_mem_t *mctx) {
	if (p == NULL) { /* as standard free() behaves */
		return;
	}
	isc_mem_free(mctx, p);
}

static void
init_nghttp2_mem(isc_mem_t *mctx, nghttp2_mem *mem) {
	*mem = (nghttp2_mem){ .malloc = (nghttp2_malloc)http_malloc,
			      .calloc = (nghttp2_calloc)http_calloc,
			      .realloc = (nghttp2_realloc)http_realloc,
			      .free = (nghttp2_free)http_free,
			      .mem_user_data = mctx };
}

static void
new_session(isc_mem_t *mctx, isc_tlsctx_t *tctx,
	    isc_nm_http_session_t **sessionp) {
	isc_nm_http_session_t *session = NULL;

	REQUIRE(sessionp != NULL && *sessionp == NULL);
	REQUIRE(mctx != NULL);

	session = isc_mem_get(mctx, sizeof(isc_nm_http_session_t));
	*session = (isc_nm_http_session_t){ .magic = HTTP2_SESSION_MAGIC,
					    .tlsctx = tctx };
	isc_refcount_init(&session->references, 1);
	isc_mem_attach(mctx, &session->mctx);
	ISC_LIST_INIT(session->cstreams);
	ISC_LIST_INIT(session->sstreams);
	ISC_LIST_INIT(session->pending_write_callbacks);

	*sessionp = session;
}

void
isc__nm_httpsession_attach(isc_nm_http_session_t *source,
			   isc_nm_http_session_t **targetp) {
	REQUIRE(VALID_HTTP2_SESSION(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
isc__nm_httpsession_detach(isc_nm_http_session_t **sessionp) {
	isc_nm_http_session_t *session = NULL;

	REQUIRE(sessionp != NULL);

	session = *sessionp;
	*sessionp = NULL;

	REQUIRE(VALID_HTTP2_SESSION(session));

	if (isc_refcount_decrement(&session->references) > 1) {
		return;
	}

	finish_http_session(session);

	INSIST(ISC_LIST_EMPTY(session->sstreams));
	INSIST(ISC_LIST_EMPTY(session->cstreams));

	if (session->ngsession != NULL) {
		nghttp2_session_del(session->ngsession);
		session->ngsession = NULL;
	}

	if (session->buf != NULL) {
		isc_buffer_free(&session->buf);
	}

	/* We need an acquire memory barrier here */
	(void)isc_refcount_current(&session->references);

	session->magic = 0;
	isc_mem_putanddetach(&session->mctx, session,
			     sizeof(isc_nm_http_session_t));
}

static http_cstream_t *
find_http_cstream(int32_t stream_id, isc_nm_http_session_t *session) {
	http_cstream_t *cstream = NULL;
	REQUIRE(VALID_HTTP2_SESSION(session));

	if (ISC_LIST_EMPTY(session->cstreams)) {
		return (NULL);
	}

	for (cstream = ISC_LIST_HEAD(session->cstreams); cstream != NULL;
	     cstream = ISC_LIST_NEXT(cstream, link))
	{
		if (cstream->stream_id == stream_id) {
			break;
		}
	}

	/* LRU-like behaviour */
	if (cstream && ISC_LIST_HEAD(session->cstreams) != cstream) {
		ISC_LIST_UNLINK(session->cstreams, cstream, link);
		ISC_LIST_PREPEND(session->cstreams, cstream, link);
	}

	return (cstream);
}

static isc_result_t
new_http_cstream(isc_nmsocket_t *sock, http_cstream_t **streamp) {
	isc_mem_t *mctx = sock->mgr->mctx;
	const char *uri = NULL;
	bool post;
	http_cstream_t *stream = NULL;
	isc_result_t result;

	uri = sock->h2.session->handle->sock->h2.connect.uri;
	post = sock->h2.session->handle->sock->h2.connect.post;

	stream = isc_mem_get(mctx, sizeof(http_cstream_t));
	*stream = (http_cstream_t){ .stream_id = -1,
				    .post = post,
				    .uri = isc_mem_strdup(mctx, uri) };
	ISC_LINK_INIT(stream, link);

	result = isc_url_parse(stream->uri, strlen(stream->uri), 0,
			       &stream->up);
	if (result != ISC_R_SUCCESS) {
		isc_mem_free(mctx, stream->uri);
		isc_mem_put(mctx, stream, sizeof(http_cstream_t));
		return (result);
	}

	isc__nmsocket_attach(sock, &stream->httpsock);
	stream->authoritylen = stream->up.field_data[ISC_UF_HOST].len;
	stream->authority = isc_mem_get(mctx, stream->authoritylen + AUTHEXTRA);
	memmove(stream->authority, &uri[stream->up.field_data[ISC_UF_HOST].off],
		stream->up.field_data[ISC_UF_HOST].len);

	if (stream->up.field_set & (1 << ISC_UF_PORT)) {
		stream->authoritylen += (size_t)snprintf(
			stream->authority +
				stream->up.field_data[ISC_UF_HOST].len,
			AUTHEXTRA, ":%u", stream->up.port);
	}

	/* If we don't have path in URI, we use "/" as path. */
	stream->pathlen = 1;
	if (stream->up.field_set & (1 << ISC_UF_PATH)) {
		stream->pathlen = stream->up.field_data[ISC_UF_PATH].len;
	}
	if (stream->up.field_set & (1 << ISC_UF_QUERY)) {
		/* +1 for '?' character */
		stream->pathlen +=
			(size_t)(stream->up.field_data[ISC_UF_QUERY].len + 1);
	}

	stream->path = isc_mem_get(mctx, stream->pathlen);
	if (stream->up.field_set & (1 << ISC_UF_PATH)) {
		memmove(stream->path,
			&uri[stream->up.field_data[ISC_UF_PATH].off],
			stream->up.field_data[ISC_UF_PATH].len);
	} else {
		stream->path[0] = '/';
	}

	if (stream->up.field_set & (1 << ISC_UF_QUERY)) {
		stream->path[stream->pathlen -
			     stream->up.field_data[ISC_UF_QUERY].len - 1] = '?';
		memmove(stream->path + stream->pathlen -
				stream->up.field_data[ISC_UF_QUERY].len,
			&uri[stream->up.field_data[ISC_UF_QUERY].off],
			stream->up.field_data[ISC_UF_QUERY].len);
	}

	isc_buffer_allocate(mctx, &stream->rbuf,
			    INITIAL_DNS_MESSAGE_BUFFER_SIZE);
	isc_buffer_setautorealloc(stream->rbuf, true);

	ISC_LIST_PREPEND(sock->h2.session->cstreams, stream, link);
	*streamp = stream;

	return (ISC_R_SUCCESS);
}

static void
put_http_cstream(isc_mem_t *mctx, http_cstream_t *stream) {
	isc_mem_put(mctx, stream->path, stream->pathlen);
	isc_mem_put(mctx, stream->authority,
		    stream->up.field_data[ISC_UF_HOST].len + AUTHEXTRA);
	isc_mem_free(mctx, stream->uri);
	if (stream->GET_path != NULL) {
		isc_mem_free(mctx, stream->GET_path);
		stream->GET_path = NULL;
		stream->GET_path_len = 0;
	}

	if (stream->postdata != NULL) {
		INSIST(stream->post);
		isc_buffer_free(&stream->postdata);
	}

	if (stream == stream->httpsock->h2.connect.cstream) {
		stream->httpsock->h2.connect.cstream = NULL;
	}
	if (ISC_LINK_LINKED(stream, link)) {
		ISC_LIST_UNLINK(stream->httpsock->h2.session->cstreams, stream,
				link);
	}
	isc__nmsocket_detach(&stream->httpsock);

	isc_buffer_free(&stream->rbuf);
	isc_mem_put(mctx, stream, sizeof(http_cstream_t));
}

static void
finish_http_session(isc_nm_http_session_t *session) {
	if (session->closed) {
		return;
	}

	if (session->handle != NULL) {
		if (!session->closed) {
			session->closed = true;
			isc_nm_cancelread(session->handle);
		}

		if (session->client) {
			client_call_failed_read_cb(ISC_R_UNEXPECTED, session);
		} else {
			server_call_failed_read_cb(ISC_R_UNEXPECTED, session);
		}

		call_pending_callbacks(session->pending_write_callbacks,
				       ISC_R_UNEXPECTED);
		ISC_LIST_INIT(session->pending_write_callbacks);

		if (session->pending_write_data != NULL) {
			isc_buffer_free(&session->pending_write_data);
		}

		isc_nmhandle_detach(&session->handle);
	}

	if (session->client_httphandle != NULL) {
		isc_nmhandle_detach(&session->client_httphandle);
	}

	INSIST(ISC_LIST_EMPTY(session->cstreams));

	/* detach from server socket */
	if (session->serversocket != NULL) {
		isc__nmsocket_detach(&session->serversocket);
	}
	session->closed = true;
}

static int
on_client_data_chunk_recv_callback(int32_t stream_id, const uint8_t *data,
				   size_t len, isc_nm_http_session_t *session) {
	http_cstream_t *cstream = find_http_cstream(stream_id, session);

	if (cstream != NULL) {
		size_t new_rbufsize = len;
		INSIST(cstream->rbuf != NULL);
		new_rbufsize += isc_buffer_usedlength(cstream->rbuf);
		if (new_rbufsize <= MAX_DNS_MESSAGE_SIZE &&
		    new_rbufsize <= cstream->response_status.content_length)
		{
			isc_buffer_putmem(cstream->rbuf, data, len);
		} else {
			return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
		}
	} else {
		return (NGHTTP2_ERR_CALLBACK_FAILURE);
	}

	return (0);
}

static int
on_server_data_chunk_recv_callback(int32_t stream_id, const uint8_t *data,
				   size_t len, isc_nm_http_session_t *session) {
	isc_nmsocket_h2_t *h2 = ISC_LIST_HEAD(session->sstreams);
	while (h2 != NULL) {
		if (stream_id == h2->stream_id) {
			if (isc_buffer_base(&h2->rbuf) == NULL) {
				isc_buffer_init(
					&h2->rbuf,
					isc_mem_allocate(session->mctx,
							 h2->content_length),
					MAX_DNS_MESSAGE_SIZE);
			}
			size_t new_bufsize = isc_buffer_usedlength(&h2->rbuf) +
					     len;
			if (new_bufsize <= MAX_DNS_MESSAGE_SIZE &&
			    new_bufsize <= h2->content_length)
			{
				isc_buffer_putmem(&h2->rbuf, data, len);
				break;
			}

			return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
		}
		h2 = ISC_LIST_NEXT(h2, link);
	}
	if (h2 == NULL) {
		return (NGHTTP2_ERR_CALLBACK_FAILURE);
	}

	return (0);
}

static int
on_data_chunk_recv_callback(nghttp2_session *ngsession, uint8_t flags,
			    int32_t stream_id, const uint8_t *data, size_t len,
			    void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	int rv;

	UNUSED(ngsession);
	UNUSED(flags);

	if (session->client) {
		rv = on_client_data_chunk_recv_callback(stream_id, data, len,
							session);
	} else {
		rv = on_server_data_chunk_recv_callback(stream_id, data, len,
							session);
	}

	return (rv);
}

static void
call_unlink_cstream_readcb(http_cstream_t *cstream,
			   isc_nm_http_session_t *session,
			   isc_result_t result) {
	isc_region_t read_data;
	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(cstream != NULL);
	ISC_LIST_UNLINK(session->cstreams, cstream, link);
	INSIST(VALID_NMHANDLE(session->client_httphandle));
	isc_buffer_usedregion(cstream->rbuf, &read_data);
	cstream->read_cb(session->client_httphandle, result, &read_data,
			 cstream->read_cbarg);
	put_http_cstream(session->mctx, cstream);
}

static int
on_client_stream_close_callback(int32_t stream_id,
				isc_nm_http_session_t *session) {
	http_cstream_t *cstream = find_http_cstream(stream_id, session);

	if (cstream != NULL) {
		isc_result_t result =
			SUCCESSFUL_HTTP_STATUS(cstream->response_status.code)
				? ISC_R_SUCCESS
				: ISC_R_FAILURE;
		call_unlink_cstream_readcb(cstream, session, result);
		if (ISC_LIST_EMPTY(session->cstreams)) {
			int rv = 0;
			rv = nghttp2_session_terminate_session(
				session->ngsession, NGHTTP2_NO_ERROR);
			if (rv != 0) {
				return (rv);
			}
			/* Mark the session as closing one to finish it on a
			 * subsequent call to http_do_bio() */
			session->closing = true;
		}
	} else {
		return (NGHTTP2_ERR_CALLBACK_FAILURE);
	}

	return (0);
}

static int
on_server_stream_close_callback(int32_t stream_id,
				isc_nm_http_session_t *session) {
	isc_nmsocket_t *sock = nghttp2_session_get_stream_user_data(
		session->ngsession, stream_id);
	int rv = 0;

	ISC_LIST_UNLINK(session->sstreams, &sock->h2, link);
	session->nsstreams--;

	/*
	 * By making a call to isc__nmsocket_prep_destroy(), we ensure that
	 * the socket gets marked as inactive, allowing the HTTP/2 data
	 * associated with it to be properly disposed of eventually.
	 *
	 * An HTTP/2 stream socket will normally be marked as inactive in
	 * the normal course of operation. However, when browsers terminate
	 * HTTP/2 streams prematurely (e.g. by sending RST_STREAM),
	 * corresponding sockets can remain marked as active, retaining
	 * references to the HTTP/2 data (most notably the session objects),
	 * preventing them from being correctly freed and leading to BIND
	 * hanging on shutdown.  Calling isc__nmsocket_prep_destroy()
	 * ensures that this will not happen.
	 */
	isc__nmsocket_prep_destroy(sock);
	isc__nmsocket_detach(&sock);
	return (rv);
}

static int
on_stream_close_callback(nghttp2_session *ngsession, int32_t stream_id,
			 uint32_t error_code, void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	int rv = 0;

	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(session->ngsession == ngsession);

	UNUSED(error_code);

	if (session->client) {
		rv = on_client_stream_close_callback(stream_id, session);
	} else {
		rv = on_server_stream_close_callback(stream_id, session);
	}

	return (rv);
}

static bool
client_handle_status_header(http_cstream_t *cstream, const uint8_t *value,
			    const size_t valuelen) {
	char tmp[32] = { 0 };
	const size_t tmplen = sizeof(tmp) - 1;

	strncpy(tmp, (const char *)value, ISC_MIN(tmplen, valuelen));
	cstream->response_status.code = strtoul(tmp, NULL, 10);

	if (SUCCESSFUL_HTTP_STATUS(cstream->response_status.code)) {
		return (true);
	}

	return (false);
}

static bool
client_handle_content_length_header(http_cstream_t *cstream,
				    const uint8_t *value,
				    const size_t valuelen) {
	char tmp[32] = { 0 };
	const size_t tmplen = sizeof(tmp) - 1;

	strncpy(tmp, (const char *)value, ISC_MIN(tmplen, valuelen));
	cstream->response_status.content_length = strtoul(tmp, NULL, 10);

	if (cstream->response_status.content_length == 0 ||
	    cstream->response_status.content_length > MAX_DNS_MESSAGE_SIZE)
	{
		return (false);
	}

	return (true);
}

static bool
client_handle_content_type_header(http_cstream_t *cstream, const uint8_t *value,
				  const size_t valuelen) {
	const char type_dns_message[] = DNS_MEDIA_TYPE;
	const size_t len = sizeof(type_dns_message) - 1;

	UNUSED(valuelen);

	if (strncasecmp((const char *)value, type_dns_message, len) == 0) {
		cstream->response_status.content_type_valid = true;
		return (true);
	}

	return (false);
}

static int
client_on_header_callback(nghttp2_session *ngsession,
			  const nghttp2_frame *frame, const uint8_t *name,
			  size_t namelen, const uint8_t *value, size_t valuelen,
			  uint8_t flags, void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	http_cstream_t *cstream = NULL;
	const char status[] = ":status";
	const char content_length[] = "Content-Length";
	const char content_type[] = "Content-Type";
	bool header_ok = true;

	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(session->client);

	UNUSED(flags);
	UNUSED(ngsession);

	cstream = find_http_cstream(frame->hd.stream_id, session);
	if (cstream == NULL) {
		/*
		 * This could happen in two cases:
		 * - the server sent us bad data, or
		 * - we closed the session prematurely before receiving all
		 *   responses (i.e., because of a belated or partial response).
		 */
		return (NGHTTP2_ERR_CALLBACK_FAILURE);
	}

	INSIST(!ISC_LIST_EMPTY(session->cstreams));

	switch (frame->hd.type) {
	case NGHTTP2_HEADERS:
		if (frame->headers.cat != NGHTTP2_HCAT_RESPONSE) {
			break;
		}

		if (HEADER_MATCH(status, name, namelen)) {
			header_ok = client_handle_status_header(cstream, value,
								valuelen);
		} else if (HEADER_MATCH(content_length, name, namelen)) {
			header_ok = client_handle_content_length_header(
				cstream, value, valuelen);
		} else if (HEADER_MATCH(content_type, name, namelen)) {
			header_ok = client_handle_content_type_header(
				cstream, value, valuelen);
		}
		break;
	}

	if (!header_ok) {
		return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
	}

	return (0);
}

static void
initialize_nghttp2_client_session(isc_nm_http_session_t *session) {
	nghttp2_session_callbacks *callbacks = NULL;
	nghttp2_option *option = NULL;
	nghttp2_mem mem;

	init_nghttp2_mem(session->mctx, &mem);
	RUNTIME_CHECK(nghttp2_session_callbacks_new(&callbacks) == 0);
	RUNTIME_CHECK(nghttp2_option_new(&option) == 0);

#if NGHTTP2_VERSION_NUM >= (0x010c00)
	nghttp2_option_set_max_send_header_block_length(
		option, MAX_ALLOWED_DATA_IN_HEADERS);
#endif

	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
		callbacks, on_data_chunk_recv_callback);

	nghttp2_session_callbacks_set_on_stream_close_callback(
		callbacks, on_stream_close_callback);

	nghttp2_session_callbacks_set_on_header_callback(
		callbacks, client_on_header_callback);

	RUNTIME_CHECK(nghttp2_session_client_new3(&session->ngsession,
						  callbacks, session, option,
						  &mem) == 0);

	nghttp2_option_del(option);
	nghttp2_session_callbacks_del(callbacks);
}

static bool
send_client_connection_header(isc_nm_http_session_t *session) {
	nghttp2_settings_entry iv[] = { { NGHTTP2_SETTINGS_ENABLE_PUSH, 0 } };
	int rv;

	rv = nghttp2_submit_settings(session->ngsession, NGHTTP2_FLAG_NONE, iv,
				     sizeof(iv) / sizeof(iv[0]));
	if (rv != 0) {
		return (false);
	}

	return (true);
}

#define MAKE_NV(NAME, VALUE, VALUELEN)                                       \
	{                                                                    \
		(uint8_t *)(uintptr_t)(NAME), (uint8_t *)(uintptr_t)(VALUE), \
			sizeof(NAME) - 1, VALUELEN, NGHTTP2_NV_FLAG_NONE     \
	}

#define MAKE_NV2(NAME, VALUE)                                                \
	{                                                                    \
		(uint8_t *)(uintptr_t)(NAME), (uint8_t *)(uintptr_t)(VALUE), \
			sizeof(NAME) - 1, sizeof(VALUE) - 1,                 \
			NGHTTP2_NV_FLAG_NONE                                 \
	}

static ssize_t
client_read_callback(nghttp2_session *ngsession, int32_t stream_id,
		     uint8_t *buf, size_t length, uint32_t *data_flags,
		     nghttp2_data_source *source, void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	http_cstream_t *cstream = NULL;

	REQUIRE(session->client);
	REQUIRE(!ISC_LIST_EMPTY(session->cstreams));

	UNUSED(ngsession);
	UNUSED(source);

	cstream = find_http_cstream(stream_id, session);
	if (!cstream || cstream->stream_id != stream_id) {
		/* We haven't found the stream, so we are not reading */
		return (NGHTTP2_ERR_CALLBACK_FAILURE);
	}

	if (cstream->post) {
		size_t len = isc_buffer_remaininglength(cstream->postdata);

		if (len > length) {
			len = length;
		}

		if (len > 0) {
			memmove(buf, isc_buffer_current(cstream->postdata),
				len);
			isc_buffer_forward(cstream->postdata, len);
		}

		if (isc_buffer_remaininglength(cstream->postdata) == 0) {
			*data_flags |= NGHTTP2_DATA_FLAG_EOF;
		}

		return (len);
	} else {
		*data_flags |= NGHTTP2_DATA_FLAG_EOF;
		return (0);
	}

	return (0);
}

/*
 * Send HTTP request to the remote peer.
 */
static isc_result_t
client_submit_request(isc_nm_http_session_t *session, http_cstream_t *stream) {
	int32_t stream_id;
	char *uri = stream->uri;
	isc_url_parser_t *up = &stream->up;
	nghttp2_data_provider dp;

	if (stream->post) {
		char p[64];
		snprintf(p, sizeof(p), "%u",
			 isc_buffer_usedlength(stream->postdata));
		nghttp2_nv hdrs[] = {
			MAKE_NV2(":method", "POST"),
			MAKE_NV(":scheme",
				&uri[up->field_data[ISC_UF_SCHEMA].off],
				up->field_data[ISC_UF_SCHEMA].len),
			MAKE_NV(":authority", stream->authority,
				stream->authoritylen),
			MAKE_NV(":path", stream->path, stream->pathlen),
			MAKE_NV2("content-type", DNS_MEDIA_TYPE),
			MAKE_NV2("accept", DNS_MEDIA_TYPE),
			MAKE_NV("content-length", p, strlen(p)),
			MAKE_NV2("cache-control", DEFAULT_CACHE_CONTROL)
		};

		dp = (nghttp2_data_provider){ .read_callback =
						      client_read_callback };
		stream_id = nghttp2_submit_request(
			session->ngsession, NULL, hdrs,
			sizeof(hdrs) / sizeof(hdrs[0]), &dp, stream);
	} else {
		INSIST(stream->GET_path != NULL);
		INSIST(stream->GET_path_len != 0);
		nghttp2_nv hdrs[] = {
			MAKE_NV2(":method", "GET"),
			MAKE_NV(":scheme",
				&uri[up->field_data[ISC_UF_SCHEMA].off],
				up->field_data[ISC_UF_SCHEMA].len),
			MAKE_NV(":authority", stream->authority,
				stream->authoritylen),
			MAKE_NV(":path", stream->GET_path,
				stream->GET_path_len),
			MAKE_NV2("accept", DNS_MEDIA_TYPE),
			MAKE_NV2("cache-control", DEFAULT_CACHE_CONTROL)
		};

		dp = (nghttp2_data_provider){ .read_callback =
						      client_read_callback };
		stream_id = nghttp2_submit_request(
			session->ngsession, NULL, hdrs,
			sizeof(hdrs) / sizeof(hdrs[0]), &dp, stream);
	}
	if (stream_id < 0) {
		return (ISC_R_FAILURE);
	}

	stream->stream_id = stream_id;

	return (ISC_R_SUCCESS);
}

/*
 * Read callback from TLS socket.
 */
static void
http_readcb(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *region,
	    void *data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)data;
	ssize_t readlen;

	REQUIRE(VALID_HTTP2_SESSION(session));

	UNUSED(handle);

	if (result != ISC_R_SUCCESS) {
		if (result != ISC_R_TIMEDOUT) {
			session->reading = false;
		}
		failed_read_cb(result, session);
		return;
	}

	readlen = nghttp2_session_mem_recv(session->ngsession, region->base,
					   region->length);
	if (readlen < 0) {
		failed_read_cb(ISC_R_UNEXPECTED, session);
		return;
	}

	if ((size_t)readlen < region->length) {
		size_t unread_size = region->length - readlen;
		if (session->buf == NULL) {
			isc_buffer_allocate(session->mctx, &session->buf,
					    unread_size);
			isc_buffer_setautorealloc(session->buf, true);
		}
		isc_buffer_putmem(session->buf, region->base + readlen,
				  unread_size);
		isc_nm_pauseread(session->handle);
	}

	/* We might have something to receive or send, do IO */
	http_do_bio(session, NULL, NULL, NULL);
}

static void
call_pending_callbacks(isc__nm_http_pending_callbacks_t pending_callbacks,
		       isc_result_t result) {
	isc__nm_uvreq_t *cbreq = ISC_LIST_HEAD(pending_callbacks);
	while (cbreq != NULL) {
		isc__nm_uvreq_t *next = ISC_LIST_NEXT(cbreq, link);
		ISC_LIST_UNLINK(pending_callbacks, cbreq, link);
		isc__nm_sendcb(cbreq->handle->sock, cbreq, result, false);
		cbreq = next;
	}
}

static void
http_writecb(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	isc_http_send_req_t *req = (isc_http_send_req_t *)arg;
	isc_nm_http_session_t *session = req->session;
	isc_nmhandle_t *transphandle = req->transphandle;

	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(VALID_NMHANDLE(handle));

	if (http_session_active(session)) {
		INSIST(session->handle == handle);
	}

	call_pending_callbacks(req->pending_write_callbacks, result);

	if (req->cb != NULL) {
		req->cb(req->httphandle, result, req->cbarg);
		isc_nmhandle_detach(&req->httphandle);
	}

	isc_buffer_free(&req->pending_write_data);
	isc_mem_put(session->mctx, req, sizeof(*req));

	session->sending--;
	http_do_bio(session, NULL, NULL, NULL);
	isc_nmhandle_detach(&transphandle);
	if (result != ISC_R_SUCCESS && session->sending == 0) {
		finish_http_session(session);
	}
	isc__nm_httpsession_detach(&session);
}

static void
move_pending_send_callbacks(isc_nm_http_session_t *session,
			    isc_http_send_req_t *send) {
	STATIC_ASSERT(
		sizeof(session->pending_write_callbacks) ==
			sizeof(send->pending_write_callbacks),
		"size of pending writes requests callbacks lists differs");
	memmove(&send->pending_write_callbacks,
		&session->pending_write_callbacks,
		sizeof(session->pending_write_callbacks));
	ISC_LIST_INIT(session->pending_write_callbacks);
}

static bool
http_send_outgoing(isc_nm_http_session_t *session, isc_nmhandle_t *httphandle,
		   isc_nm_cb_t cb, void *cbarg) {
	isc_http_send_req_t *send = NULL;
	size_t total = 0;
	isc_region_t send_data = { 0 };
	isc_nmhandle_t *transphandle = NULL;
#ifdef ENABLE_HTTP_WRITE_BUFFERING
	size_t max_total_write_size = 0;
#endif /* ENABLE_HTTP_WRITE_BUFFERING */

	if (!http_session_active(session) ||
	    (!nghttp2_session_want_write(session->ngsession) &&
	     session->pending_write_data == NULL))
	{
		return (false);
	}

	/* We need to attach to the session->handle earlier because as an
	 * indirect result of the nghttp2_session_mem_send() the session
	 * might get closed and the handle detached. However, there is
	 * still some outgoing data to handle and we need to call it
	 * anyway if only to get the write callback passed here to get
	 * called properly. */
	isc_nmhandle_attach(session->handle, &transphandle);

	while (nghttp2_session_want_write(session->ngsession)) {
		const uint8_t *data = NULL;
		const size_t pending =
			nghttp2_session_mem_send(session->ngsession, &data);
		const size_t new_total = total + pending;

		/* Sometimes nghttp2_session_mem_send() does not return any
		 * data to send even though nghttp2_session_want_write()
		 * returns success. */
		if (pending == 0 || data == NULL) {
			break;
		}

		/* reallocate buffer if required */
		if (session->pending_write_data == NULL) {
			isc_buffer_allocate(session->mctx,
					    &session->pending_write_data,
					    INITIAL_DNS_MESSAGE_BUFFER_SIZE);
			isc_buffer_setautorealloc(session->pending_write_data,
						  true);
		}
		isc_buffer_putmem(session->pending_write_data, data, pending);
		total = new_total;
	}

#ifdef ENABLE_HTTP_WRITE_BUFFERING
	if (session->pending_write_data != NULL) {
		max_total_write_size =
			isc_buffer_usedlength(session->pending_write_data);
	}

	/* Here we are trying to flush the pending writes buffer earlier
	 * to avoid hitting unnecessary limitations on a TLS record size
	 * within some tools (e.g. flamethrower). */
	if (max_total_write_size >= FLUSH_HTTP_WRITE_BUFFER_AFTER) {
		/* Case 1: We have equal or more than
		 * FLUSH_HTTP_WRITE_BUFFER_AFTER bytes to send. Let's flush it.
		 */
		total = max_total_write_size;
	} else if (session->sending > 0 && total > 0) {
		/* Case 2: There is one or more write requests in flight and
		 * we have some new data form nghttp2 to send. Let's put the
		 * write callback (if any) into the pending write callbacks
		 * list. Then let's return from the function: as soon as the
		 * "in-flight" write callback get's called or we have reached
		 * FLUSH_HTTP_WRITE_BUFFER_AFTER bytes in the write buffer, we
		 * will flush the buffer. */
		if (cb != NULL) {
			isc__nm_uvreq_t *newcb = isc__nm_uvreq_get(
				httphandle->sock->mgr, httphandle->sock);

			INSIST(VALID_NMHANDLE(httphandle));
			newcb->cb.send = cb;
			newcb->cbarg = cbarg;
			isc_nmhandle_attach(httphandle, &newcb->handle);
			ISC_LIST_APPEND(session->pending_write_callbacks, newcb,
					link);
		}
		goto nothing_to_send;
	} else if (session->sending == 0 && total == 0 &&
		   session->pending_write_data != NULL)
	{
		/* Case 3: There is no write in flight and we haven't got
		 * anything new from nghttp2, but there is some data pending
		 * in the write buffer. Let's flush the buffer. */
		isc_region_t region = { 0 };
		total = isc_buffer_usedlength(session->pending_write_data);
		INSIST(total > 0);
		isc_buffer_usedregion(session->pending_write_data, &region);
		INSIST(total == region.length);
	} else {
		/* The other cases are, uninteresting, fall-through ones. */
		/* In the following cases (4-6) we will just bail out. */
		/* Case 4: There is nothing new to send, nor anything in the
		 * write buffer. */
		/* Case 5: There is nothing new to send and there is write
		 * request(s) in flight. */
		/* Case 6: There is nothing new to send nor there are any
		 * write requests in flight. */

		/* Case 7: There is some new data to send and there are no any
		 * write requests in flight: Let's send the data.*/
		INSIST((total == 0 && session->pending_write_data == NULL) ||
		       (total == 0 && session->sending > 0) ||
		       (total == 0 && session->sending == 0) ||
		       (total > 0 && session->sending == 0));
	}
#else
	INSIST(ISC_LIST_EMPTY(session->pending_write_callbacks));
#endif /* ENABLE_HTTP_WRITE_BUFFERING */

	if (total == 0) {
		/* No data returned */
		goto nothing_to_send;
	}

	/* If we have reached the point it means that we need to send some
	 * data and flush the outgoing buffer. The code below does that. */
	send = isc_mem_get(session->mctx, sizeof(*send));

	*send = (isc_http_send_req_t){ .pending_write_data =
					       session->pending_write_data,
				       .cb = cb,
				       .cbarg = cbarg };
	session->pending_write_data = NULL;
	move_pending_send_callbacks(session, send);

	send->transphandle = transphandle;
	isc__nm_httpsession_attach(session, &send->session);

	if (cb != NULL) {
		INSIST(VALID_NMHANDLE(httphandle));
		isc_nmhandle_attach(httphandle, &send->httphandle);
	}

	session->sending++;
	isc_buffer_usedregion(send->pending_write_data, &send_data);
	isc_nm_send(transphandle, &send_data, http_writecb, send);
	return (true);
nothing_to_send:
	isc_nmhandle_detach(&transphandle);
	return (false);
}

static void
http_do_bio(isc_nm_http_session_t *session, isc_nmhandle_t *send_httphandle,
	    isc_nm_cb_t send_cb, void *send_cbarg) {
	REQUIRE(VALID_HTTP2_SESSION(session));

	if (session->closed) {
		return;
	} else if (session->closing) {
		/*
		 * There might be leftover callbacks waiting to be received
		 */
		if (session->sending == 0) {
			finish_http_session(session);
		}
		return;
	} else if (nghttp2_session_want_read(session->ngsession) == 0 &&
		   nghttp2_session_want_write(session->ngsession) == 0 &&
		   session->pending_write_data == NULL)
	{
		session->closing = true;
		return;
	}

	if (nghttp2_session_want_read(session->ngsession) != 0) {
		if (!session->reading) {
			/* We have not yet started reading from this handle */
			isc_nm_read(session->handle, http_readcb, session);
			session->reading = true;
		} else if (session->buf != NULL) {
			size_t remaining =
				isc_buffer_remaininglength(session->buf);
			/* Leftover data in the buffer, use it */
			size_t readlen = nghttp2_session_mem_recv(
				session->ngsession,
				isc_buffer_current(session->buf), remaining);

			if (readlen == remaining) {
				isc_buffer_free(&session->buf);
			} else {
				isc_buffer_forward(session->buf, readlen);
			}

			http_do_bio(session, send_httphandle, send_cb,
				    send_cbarg);
			return;
		} else {
			/* Resume reading, it's idempotent, wait for more */
			isc_nm_resumeread(session->handle);
		}
	} else {
		/* We don't want more data, stop reading for now */
		isc_nm_pauseread(session->handle);
	}

	if (send_cb != NULL) {
		INSIST(VALID_NMHANDLE(send_httphandle));
		(void)http_send_outgoing(session, send_httphandle, send_cb,
					 send_cbarg);
	} else {
		INSIST(send_httphandle == NULL);
		INSIST(send_cb == NULL);
		INSIST(send_cbarg == NULL);
		(void)http_send_outgoing(session, NULL, NULL, NULL);
	}

	return;
}

static isc_result_t
get_http_cstream(isc_nmsocket_t *sock, http_cstream_t **streamp) {
	http_cstream_t *cstream = sock->h2.connect.cstream;
	isc_result_t result;

	REQUIRE(streamp != NULL && *streamp == NULL);

	sock->h2.connect.cstream = NULL;

	if (cstream == NULL) {
		result = new_http_cstream(sock, &cstream);
		if (result != ISC_R_SUCCESS) {
			INSIST(cstream == NULL);
			return (result);
		}
	}

	*streamp = cstream;
	return (ISC_R_SUCCESS);
}

static void
http_call_connect_cb(isc_nmsocket_t *sock, isc_nm_http_session_t *session,
		     isc_result_t result) {
	isc__nm_uvreq_t *req = NULL;
	isc_nmhandle_t *httphandle = isc__nmhandle_get(sock, &sock->peer,
						       &sock->iface);

	REQUIRE(sock->connect_cb != NULL);

	if (result == ISC_R_SUCCESS) {
		req = isc__nm_uvreq_get(sock->mgr, sock);
		req->cb.connect = sock->connect_cb;
		req->cbarg = sock->connect_cbarg;
		if (session != NULL) {
			session->client_httphandle = httphandle;
			req->handle = NULL;
			isc_nmhandle_attach(httphandle, &req->handle);
		} else {
			req->handle = httphandle;
		}

		isc__nmsocket_clearcb(sock);
		isc__nm_connectcb(sock, req, result, true);
	} else {
		void *cbarg = sock->connect_cbarg;
		isc_nm_cb_t connect_cb = sock->connect_cb;

		isc__nmsocket_clearcb(sock);
		connect_cb(httphandle, result, cbarg);
		isc_nmhandle_detach(&httphandle);
	}
}

static void
transport_connect_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *http_sock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *transp_sock = NULL;
	isc_nm_http_session_t *session = NULL;
	http_cstream_t *cstream = NULL;
	isc_mem_t *mctx = NULL;

	REQUIRE(VALID_NMSOCK(http_sock));
	REQUIRE(VALID_NMHANDLE(handle));

	transp_sock = handle->sock;

	REQUIRE(VALID_NMSOCK(transp_sock));

	mctx = transp_sock->mgr->mctx;

	INSIST(http_sock->h2.connect.uri != NULL);

	http_sock->tid = transp_sock->tid;
	http_sock->h2.connect.tls_peer_verify_string =
		isc_nm_verify_tls_peer_result_string(handle);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	new_session(mctx, http_sock->h2.connect.tlsctx, &session);
	session->client = true;
	transp_sock->h2.session = session;
	http_sock->h2.connect.tlsctx = NULL;
	/* otherwise we will get some garbage output in DIG */
	http_sock->iface = handle->sock->iface;
	http_sock->peer = handle->sock->peer;

	transp_sock->h2.connect.post = http_sock->h2.connect.post;
	transp_sock->h2.connect.uri = http_sock->h2.connect.uri;
	http_sock->h2.connect.uri = NULL;
	isc__nm_httpsession_attach(session, &http_sock->h2.session);

	if (session->tlsctx != NULL) {
		const unsigned char *alpn = NULL;
		unsigned int alpnlen = 0;

		INSIST(transp_sock->type == isc_nm_tlssocket);

		isc_tls_get_selected_alpn(transp_sock->tlsstream.tls, &alpn,
					  &alpnlen);
		if (alpn == NULL || alpnlen != NGHTTP2_PROTO_VERSION_ID_LEN ||
		    memcmp(NGHTTP2_PROTO_VERSION_ID, alpn,
			   NGHTTP2_PROTO_VERSION_ID_LEN) != 0)
		{
			/*
			 * HTTP/2 negotiation error. Any sensible DoH
			 * client will fail if HTTP/2 cannot be
			 * negotiated via ALPN.
			 */
			result = ISC_R_HTTP2ALPNERROR;
			goto error;
		}
	}

	isc_nmhandle_attach(handle, &session->handle);

	initialize_nghttp2_client_session(session);
	if (!send_client_connection_header(session)) {
		goto error;
	}

	result = get_http_cstream(http_sock, &cstream);
	http_sock->h2.connect.cstream = cstream;
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	http_transpost_tcp_nodelay(handle);

	http_call_connect_cb(http_sock, session, result);

	http_do_bio(session, NULL, NULL, NULL);
	isc__nmsocket_detach(&http_sock);
	return;

error:
	http_call_connect_cb(http_sock, session, result);

	if (http_sock->h2.connect.uri != NULL) {
		isc_mem_free(mctx, http_sock->h2.connect.uri);
	}

	isc__nmsocket_prep_destroy(http_sock);
	isc__nmsocket_detach(&http_sock);
}

void
isc_nm_httpconnect(isc_nm_t *mgr, isc_sockaddr_t *local, isc_sockaddr_t *peer,
		   const char *uri, bool post, isc_nm_cb_t cb, void *cbarg,
		   isc_tlsctx_t *tlsctx,
		   isc_tlsctx_client_session_cache_t *client_sess_cache,
		   unsigned int timeout, size_t extrahandlesize) {
	isc_sockaddr_t local_interface;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(cb != NULL);
	REQUIRE(peer != NULL);
	REQUIRE(uri != NULL);
	REQUIRE(*uri != '\0');

	if (local == NULL) {
		isc_sockaddr_anyofpf(&local_interface, peer->type.sa.sa_family);
		local = &local_interface;
	}

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_httpsocket, local);

	sock->extrahandlesize = extrahandlesize;
	sock->connect_timeout = timeout;
	sock->result = ISC_R_UNSET;
	sock->connect_cb = cb;
	sock->connect_cbarg = cbarg;
	atomic_init(&sock->client, true);

	if (isc__nm_closing(sock)) {
		isc__nm_uvreq_t *req = isc__nm_uvreq_get(mgr, sock);

		req->cb.connect = cb;
		req->cbarg = cbarg;
		req->peer = *peer;
		req->local = *local;
		req->handle = isc__nmhandle_get(sock, &req->peer, &sock->iface);

		if (isc__nm_in_netthread()) {
			sock->tid = isc_nm_tid();
		}

		isc__nmsocket_clearcb(sock);
		isc__nm_connectcb(sock, req, ISC_R_SHUTTINGDOWN, true);
		isc__nmsocket_prep_destroy(sock);
		isc__nmsocket_detach(&sock);
		return;
	}

	sock->h2 = (isc_nmsocket_h2_t){ .connect.uri = isc_mem_strdup(mgr->mctx,
								      uri),
					.connect.post = post,
					.connect.tlsctx = tlsctx };
	ISC_LINK_INIT(&sock->h2, link);

	/*
	 * We need to prevent the interface object data from going out of
	 * scope too early.
	 */
	if (local == &local_interface) {
		sock->h2.connect.local_interface = local_interface;
		sock->iface = sock->h2.connect.local_interface;
	}

	if (tlsctx != NULL) {
		isc_nm_tlsconnect(mgr, local, peer, transport_connect_cb, sock,
				  tlsctx, client_sess_cache, timeout, 0);
	} else {
		isc_nm_tcpconnect(mgr, local, peer, transport_connect_cb, sock,
				  timeout, 0);
	}
}

static isc_result_t
client_send(isc_nmhandle_t *handle, const isc_region_t *region) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = handle->sock;
	isc_mem_t *mctx = sock->mgr->mctx;
	isc_nm_http_session_t *session = sock->h2.session;
	http_cstream_t *cstream = sock->h2.connect.cstream;

	REQUIRE(VALID_HTTP2_SESSION(handle->sock->h2.session));
	REQUIRE(session->client);
	REQUIRE(region != NULL);
	REQUIRE(region->base != NULL);
	REQUIRE(region->length <= MAX_DNS_MESSAGE_SIZE);

	if (session->closed) {
		return (ISC_R_CANCELED);
	}

	INSIST(cstream != NULL);

	if (cstream->post) {
		/* POST */
		isc_buffer_allocate(mctx, &cstream->postdata, region->length);
		isc_buffer_putmem(cstream->postdata, region->base,
				  region->length);
	} else {
		/* GET */
		size_t path_size = 0;
		char *base64url_data = NULL;
		size_t base64url_data_len = 0;
		isc_buffer_t *buf = NULL;
		isc_region_t data = *region;
		isc_region_t base64_region;
		size_t base64_len = ((4 * data.length / 3) + 3) & ~3;

		isc_buffer_allocate(mctx, &buf, base64_len);

		result = isc_base64_totext(&data, -1, "", buf);
		if (result != ISC_R_SUCCESS) {
			isc_buffer_free(&buf);
			goto error;
		}

		isc_buffer_usedregion(buf, &base64_region);
		INSIST(base64_region.length == base64_len);

		base64url_data = isc__nm_base64_to_base64url(
			mctx, (const char *)base64_region.base,
			base64_region.length, &base64url_data_len);
		isc_buffer_free(&buf);
		if (base64url_data == NULL) {
			goto error;
		}

		/* len("?dns=") + len(path) + len(base64url) + len("\0") */
		path_size = cstream->pathlen + base64url_data_len + 5 + 1;
		cstream->GET_path = isc_mem_allocate(mctx, path_size);
		cstream->GET_path_len = (size_t)snprintf(
			cstream->GET_path, path_size, "%.*s?dns=%s",
			(int)cstream->pathlen, cstream->path, base64url_data);

		INSIST(cstream->GET_path_len == (path_size - 1));
		isc_mem_free(mctx, base64url_data);
	}

	cstream->sending = true;

	sock->h2.connect.cstream = NULL;
	result = client_submit_request(session, cstream);
	if (result != ISC_R_SUCCESS) {
		put_http_cstream(session->mctx, cstream);
		goto error;
	}

error:
	return (result);
}

isc_result_t
isc__nm_http_request(isc_nmhandle_t *handle, isc_region_t *region,
		     isc_nm_recv_cb_t cb, void *cbarg) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nmsocket_t *sock = NULL;
	http_cstream_t *cstream = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->tid == isc_nm_tid());
	REQUIRE(atomic_load(&handle->sock->client));

	REQUIRE(cb != NULL);

	sock = handle->sock;

	isc__nm_http_read(handle, cb, cbarg);
	if (!http_session_active(handle->sock->h2.session)) {
		/* the callback was called by isc__nm_http_read() */
		return (ISC_R_CANCELED);
	}
	result = client_send(handle, region);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	return (ISC_R_SUCCESS);

error:
	cstream = sock->h2.connect.cstream;
	if (cstream->read_cb != NULL) {
		cstream->read_cb(handle, result, NULL, cstream->read_cbarg);
	}
	return (result);
}

static int
server_on_begin_headers_callback(nghttp2_session *ngsession,
				 const nghttp2_frame *frame, void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	isc_nmsocket_t *socket = NULL;

	if (frame->hd.type != NGHTTP2_HEADERS ||
	    frame->headers.cat != NGHTTP2_HCAT_REQUEST)
	{
		return (0);
	} else if (frame->hd.length > MAX_ALLOWED_DATA_IN_HEADERS) {
		return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
	}

	if (session->nsstreams >= session->max_concurrent_streams) {
		return (NGHTTP2_ERR_CALLBACK_FAILURE);
	}

	socket = isc_mem_get(session->mctx, sizeof(isc_nmsocket_t));
	isc__nmsocket_init(socket, session->serversocket->mgr,
			   isc_nm_httpsocket,
			   (isc_sockaddr_t *)&session->handle->sock->iface);
	socket->peer = session->handle->sock->peer;
	socket->h2 = (isc_nmsocket_h2_t){
		.psock = socket,
		.stream_id = frame->hd.stream_id,
		.headers_error_code = ISC_HTTP_ERROR_SUCCESS,
		.request_type = ISC_HTTP_REQ_UNSUPPORTED,
		.request_scheme = ISC_HTTP_SCHEME_UNSUPPORTED
	};
	isc_buffer_initnull(&socket->h2.rbuf);
	isc_buffer_initnull(&socket->h2.wbuf);
	session->nsstreams++;
	isc__nm_httpsession_attach(session, &socket->h2.session);
	socket->tid = session->handle->sock->tid;
	ISC_LINK_INIT(&socket->h2, link);
	ISC_LIST_APPEND(session->sstreams, &socket->h2, link);

	nghttp2_session_set_stream_user_data(ngsession, frame->hd.stream_id,
					     socket);
	return (0);
}

static isc_nm_httphandler_t *
find_server_request_handler(const char *request_path,
			    isc_nmsocket_t *serversocket, const int tid) {
	isc_nm_httphandler_t *handler = NULL;

	REQUIRE(VALID_NMSOCK(serversocket));

	if (atomic_load(&serversocket->listening)) {
		handler = http_endpoints_find(
			request_path,
			http_get_listener_endpoints(serversocket, tid));
	}
	return (handler);
}

static isc_http_error_responses_t
server_handle_path_header(isc_nmsocket_t *socket, const uint8_t *value,
			  const size_t valuelen) {
	isc_nm_httphandler_t *handler = NULL;
	const uint8_t *qstr = NULL;
	size_t vlen = valuelen;

	qstr = memchr(value, '?', valuelen);
	if (qstr != NULL) {
		vlen = qstr - value;
	}

	if (socket->h2.request_path != NULL) {
		isc_mem_free(socket->mgr->mctx, socket->h2.request_path);
	}
	socket->h2.request_path = isc_mem_strndup(
		socket->mgr->mctx, (const char *)value, vlen + 1);

	if (!isc_nm_http_path_isvalid(socket->h2.request_path)) {
		isc_mem_free(socket->mgr->mctx, socket->h2.request_path);
		socket->h2.request_path = NULL;
		return (ISC_HTTP_ERROR_BAD_REQUEST);
	}

	handler = find_server_request_handler(socket->h2.request_path,
					      socket->h2.session->serversocket,
					      socket->tid);
	if (handler != NULL) {
		socket->h2.cb = handler->cb;
		socket->h2.cbarg = handler->cbarg;
		socket->extrahandlesize = handler->extrahandlesize;
	} else {
		isc_mem_free(socket->mgr->mctx, socket->h2.request_path);
		socket->h2.request_path = NULL;
		return (ISC_HTTP_ERROR_NOT_FOUND);
	}

	if (qstr != NULL) {
		const char *dns_value = NULL;
		size_t dns_value_len = 0;

		if (isc__nm_parse_httpquery((const char *)qstr, &dns_value,
					    &dns_value_len))
		{
			const size_t decoded_size = dns_value_len / 4 * 3;
			if (decoded_size <= MAX_DNS_MESSAGE_SIZE) {
				if (socket->h2.query_data != NULL) {
					isc_mem_free(socket->mgr->mctx,
						     socket->h2.query_data);
				}
				socket->h2.query_data =
					isc__nm_base64url_to_base64(
						socket->mgr->mctx, dns_value,
						dns_value_len,
						&socket->h2.query_data_len);
			} else {
				socket->h2.query_too_large = true;
				return (ISC_HTTP_ERROR_PAYLOAD_TOO_LARGE);
			}
		} else {
			return (ISC_HTTP_ERROR_BAD_REQUEST);
		}
	}
	return (ISC_HTTP_ERROR_SUCCESS);
}

static isc_http_error_responses_t
server_handle_method_header(isc_nmsocket_t *socket, const uint8_t *value,
			    const size_t valuelen) {
	const char get[] = "GET";
	const char post[] = "POST";

	if (HEADER_MATCH(get, value, valuelen)) {
		socket->h2.request_type = ISC_HTTP_REQ_GET;
	} else if (HEADER_MATCH(post, value, valuelen)) {
		socket->h2.request_type = ISC_HTTP_REQ_POST;
	} else {
		return (ISC_HTTP_ERROR_NOT_IMPLEMENTED);
	}
	return (ISC_HTTP_ERROR_SUCCESS);
}

static isc_http_error_responses_t
server_handle_scheme_header(isc_nmsocket_t *socket, const uint8_t *value,
			    const size_t valuelen) {
	const char http[] = "http";
	const char http_secure[] = "https";

	if (HEADER_MATCH(http_secure, value, valuelen)) {
		socket->h2.request_scheme = ISC_HTTP_SCHEME_HTTP_SECURE;
	} else if (HEADER_MATCH(http, value, valuelen)) {
		socket->h2.request_scheme = ISC_HTTP_SCHEME_HTTP;
	} else {
		return (ISC_HTTP_ERROR_BAD_REQUEST);
	}
	return (ISC_HTTP_ERROR_SUCCESS);
}

static isc_http_error_responses_t
server_handle_content_length_header(isc_nmsocket_t *socket,
				    const uint8_t *value,
				    const size_t valuelen) {
	char tmp[32] = { 0 };
	const size_t tmplen = sizeof(tmp) - 1;

	strncpy(tmp, (const char *)value,
		valuelen > tmplen ? tmplen : valuelen);
	socket->h2.content_length = strtoul(tmp, NULL, 10);
	if (socket->h2.content_length > MAX_DNS_MESSAGE_SIZE) {
		return (ISC_HTTP_ERROR_PAYLOAD_TOO_LARGE);
	} else if (socket->h2.content_length == 0) {
		return (ISC_HTTP_ERROR_BAD_REQUEST);
	}
	return (ISC_HTTP_ERROR_SUCCESS);
}

static isc_http_error_responses_t
server_handle_content_type_header(isc_nmsocket_t *socket, const uint8_t *value,
				  const size_t valuelen) {
	const char type_dns_message[] = DNS_MEDIA_TYPE;
	isc_http_error_responses_t resp = ISC_HTTP_ERROR_SUCCESS;

	UNUSED(socket);

	if (!HEADER_MATCH(type_dns_message, value, valuelen)) {
		resp = ISC_HTTP_ERROR_UNSUPPORTED_MEDIA_TYPE;
	}
	return (resp);
}

static isc_http_error_responses_t
server_handle_header(isc_nmsocket_t *socket, const uint8_t *name,
		     size_t namelen, const uint8_t *value,
		     const size_t valuelen) {
	isc_http_error_responses_t code = ISC_HTTP_ERROR_SUCCESS;
	bool was_error;
	const char path[] = ":path";
	const char method[] = ":method";
	const char scheme[] = ":scheme";
	const char content_length[] = "Content-Length";
	const char content_type[] = "Content-Type";

	was_error = socket->h2.headers_error_code != ISC_HTTP_ERROR_SUCCESS;
	/*
	 * process Content-Length even when there was an error,
	 * to drop the connection earlier if required.
	 */
	if (HEADER_MATCH(content_length, name, namelen)) {
		code = server_handle_content_length_header(socket, value,
							   valuelen);
	} else if (!was_error && HEADER_MATCH(path, name, namelen)) {
		code = server_handle_path_header(socket, value, valuelen);
	} else if (!was_error && HEADER_MATCH(method, name, namelen)) {
		code = server_handle_method_header(socket, value, valuelen);
	} else if (!was_error && HEADER_MATCH(scheme, name, namelen)) {
		code = server_handle_scheme_header(socket, value, valuelen);
	} else if (!was_error && HEADER_MATCH(content_type, name, namelen)) {
		code = server_handle_content_type_header(socket, value,
							 valuelen);
	}

	return (code);
}

static int
server_on_header_callback(nghttp2_session *session, const nghttp2_frame *frame,
			  const uint8_t *name, size_t namelen,
			  const uint8_t *value, size_t valuelen, uint8_t flags,
			  void *user_data) {
	isc_nmsocket_t *socket = NULL;
	isc_http_error_responses_t code = ISC_HTTP_ERROR_SUCCESS;

	UNUSED(flags);
	UNUSED(user_data);

	socket = nghttp2_session_get_stream_user_data(session,
						      frame->hd.stream_id);
	if (socket == NULL) {
		return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
	}

	socket->h2.headers_data_processed += (namelen + valuelen);

	switch (frame->hd.type) {
	case NGHTTP2_HEADERS:
		if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
			break;
		}
		code = server_handle_header(socket, name, namelen, value,
					    valuelen);
		break;
	}

	INSIST(socket != NULL);

	if (socket->h2.headers_data_processed > MAX_ALLOWED_DATA_IN_HEADERS) {
		return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
	} else if (socket->h2.content_length > MAX_ALLOWED_DATA_IN_POST) {
		return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
	}

	if (code == ISC_HTTP_ERROR_SUCCESS) {
		return (0);
	} else {
		socket->h2.headers_error_code = code;
	}

	return (0);
}

static ssize_t
server_read_callback(nghttp2_session *ngsession, int32_t stream_id,
		     uint8_t *buf, size_t length, uint32_t *data_flags,
		     nghttp2_data_source *source, void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	isc_nmsocket_t *socket = (isc_nmsocket_t *)source->ptr;
	size_t buflen;

	REQUIRE(socket->h2.stream_id == stream_id);

	UNUSED(ngsession);
	UNUSED(session);

	buflen = isc_buffer_remaininglength(&socket->h2.wbuf);
	if (buflen > length) {
		buflen = length;
	}

	if (buflen > 0) {
		(void)memmove(buf, isc_buffer_current(&socket->h2.wbuf),
			      buflen);
		isc_buffer_forward(&socket->h2.wbuf, buflen);
	}

	if (isc_buffer_remaininglength(&socket->h2.wbuf) == 0) {
		*data_flags |= NGHTTP2_DATA_FLAG_EOF;
	}

	return (buflen);
}

static isc_result_t
server_send_response(nghttp2_session *ngsession, int32_t stream_id,
		     const nghttp2_nv *nva, size_t nvlen,
		     isc_nmsocket_t *socket) {
	nghttp2_data_provider data_prd;
	int rv;

	if (socket->h2.response_submitted) {
		/* NGHTTP2 will gladly accept new response (write request)
		 * from us even though we cannot send more than one over the
		 * same HTTP/2 stream. Thus, we need to handle this case
		 * manually. We will return failure code so that it will be
		 * passed to the write callback. */
		return (ISC_R_FAILURE);
	}

	data_prd.source.ptr = socket;
	data_prd.read_callback = server_read_callback;

	rv = nghttp2_submit_response(ngsession, stream_id, nva, nvlen,
				     &data_prd);
	if (rv != 0) {
		return (ISC_R_FAILURE);
	}

	socket->h2.response_submitted = true;
	return (ISC_R_SUCCESS);
}

#define MAKE_ERROR_REPLY(tag, code, desc)             \
	{                                             \
		tag, MAKE_NV2(":status", #code), desc \
	}

/*
 * Here we use roughly the same error codes that Unbound uses.
 * (https://blog.nlnetlabs.nl/dns-over-https-in-unbound/)
 */

static struct http_error_responses {
	const isc_http_error_responses_t type;
	const nghttp2_nv header;
	const char *desc;
} error_responses[] = {
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_BAD_REQUEST, 400, "Bad Request"),
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_NOT_FOUND, 404, "Not Found"),
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_PAYLOAD_TOO_LARGE, 413,
			 "Payload Too Large"),
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_URI_TOO_LONG, 414, "URI Too Long"),
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_UNSUPPORTED_MEDIA_TYPE, 415,
			 "Unsupported Media Type"),
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_GENERIC, 500, "Internal Server Error"),
	MAKE_ERROR_REPLY(ISC_HTTP_ERROR_NOT_IMPLEMENTED, 501, "Not Implemented")
};

static void
log_server_error_response(const isc_nmsocket_t *socket,
			  const struct http_error_responses *response) {
	const int log_level = ISC_LOG_DEBUG(1);
	isc_sockaddr_t client_addr;
	isc_sockaddr_t local_addr;
	char client_sabuf[ISC_SOCKADDR_FORMATSIZE];
	char local_sabuf[ISC_SOCKADDR_FORMATSIZE];

	if (!isc_log_wouldlog(isc_lctx, log_level)) {
		return;
	}

	client_addr = isc_nmhandle_peeraddr(socket->h2.session->handle);
	local_addr = isc_nmhandle_localaddr(socket->h2.session->handle);
	isc_sockaddr_format(&client_addr, client_sabuf, sizeof(client_sabuf));
	isc_sockaddr_format(&local_addr, local_sabuf, sizeof(local_sabuf));
	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      log_level, "HTTP/2 request from %s (on %s) failed: %s %s",
		      client_sabuf, local_sabuf, response->header.value,
		      response->desc);
}

static isc_result_t
server_send_error_response(const isc_http_error_responses_t error,
			   nghttp2_session *ngsession, isc_nmsocket_t *socket) {
	void *base;

	REQUIRE(error != ISC_HTTP_ERROR_SUCCESS);

	base = isc_buffer_base(&socket->h2.rbuf);
	if (base != NULL) {
		isc_mem_free(socket->h2.session->mctx, base);
		isc_buffer_initnull(&socket->h2.rbuf);
	}

	/* We do not want the error response to be cached anywhere. */
	socket->h2.min_ttl = 0;

	for (size_t i = 0;
	     i < sizeof(error_responses) / sizeof(error_responses[0]); i++)
	{
		if (error_responses[i].type == error) {
			log_server_error_response(socket, &error_responses[i]);
			return (server_send_response(
				ngsession, socket->h2.stream_id,
				&error_responses[i].header, 1, socket));
		}
	}

	return (server_send_error_response(ISC_HTTP_ERROR_GENERIC, ngsession,
					   socket));
}

static void
server_call_cb(isc_nmsocket_t *socket, isc_nm_http_session_t *session,
	       const isc_result_t result, isc_region_t *data) {
	isc_sockaddr_t addr;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(VALID_NMSOCK(socket));
	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(socket->h2.cb != NULL);

	addr = isc_nmhandle_peeraddr(session->handle);
	handle = isc__nmhandle_get(socket, &addr, NULL);
	socket->h2.cb(handle, result, data, socket->h2.cbarg);
	isc_nmhandle_detach(&handle);
}

void
isc__nm_http_bad_request(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	sock = handle->sock;
	REQUIRE(sock->type == isc_nm_httpsocket);
	REQUIRE(!atomic_load(&sock->client));
	REQUIRE(VALID_HTTP2_SESSION(sock->h2.session));

	(void)server_send_error_response(ISC_HTTP_ERROR_BAD_REQUEST,
					 sock->h2.session->ngsession, sock);
}

static int
server_on_request_recv(nghttp2_session *ngsession,
		       isc_nm_http_session_t *session, isc_nmsocket_t *socket) {
	isc_result_t result;
	isc_http_error_responses_t code = ISC_HTTP_ERROR_SUCCESS;
	isc_region_t data;
	uint8_t tmp_buf[MAX_DNS_MESSAGE_SIZE];

	code = socket->h2.headers_error_code;
	if (code != ISC_HTTP_ERROR_SUCCESS) {
		goto error;
	}

	if (socket->h2.request_path == NULL || socket->h2.cb == NULL) {
		code = ISC_HTTP_ERROR_NOT_FOUND;
	} else if (socket->h2.request_type == ISC_HTTP_REQ_POST &&
		   socket->h2.content_length == 0)
	{
		code = ISC_HTTP_ERROR_BAD_REQUEST;
	} else if (socket->h2.request_type == ISC_HTTP_REQ_POST &&
		   isc_buffer_usedlength(&socket->h2.rbuf) >
			   socket->h2.content_length)
	{
		code = ISC_HTTP_ERROR_PAYLOAD_TOO_LARGE;
	} else if (socket->h2.request_type == ISC_HTTP_REQ_POST &&
		   isc_buffer_usedlength(&socket->h2.rbuf) !=
			   socket->h2.content_length)
	{
		code = ISC_HTTP_ERROR_BAD_REQUEST;
	} else if (socket->h2.request_type == ISC_HTTP_REQ_POST &&
		   socket->h2.query_data != NULL)
	{
		/* The spec does not mention which value the query string for
		 * POST should have. For GET we use its value to decode a DNS
		 * message from it, for POST the message is transferred in the
		 * body of the request. Taking it into account, it is much safer
		 * to treat POST
		 * requests with query strings as malformed ones. */
		code = ISC_HTTP_ERROR_BAD_REQUEST;
	} else if (socket->h2.request_type == ISC_HTTP_REQ_GET &&
		   socket->h2.content_length > 0)
	{
		code = ISC_HTTP_ERROR_BAD_REQUEST;
	} else if (socket->h2.request_type == ISC_HTTP_REQ_GET &&
		   socket->h2.query_data == NULL)
	{
		/* A GET request without any query data - there is nothing to
		 * decode. */
		INSIST(socket->h2.query_data_len == 0);
		code = ISC_HTTP_ERROR_BAD_REQUEST;
	}

	if (code != ISC_HTTP_ERROR_SUCCESS) {
		goto error;
	}

	if (socket->h2.request_type == ISC_HTTP_REQ_GET) {
		isc_buffer_t decoded_buf;
		isc_buffer_init(&decoded_buf, tmp_buf, sizeof(tmp_buf));
		if (isc_base64_decodestring(socket->h2.query_data,
					    &decoded_buf) != ISC_R_SUCCESS)
		{
			code = ISC_HTTP_ERROR_BAD_REQUEST;
			goto error;
		}
		isc_buffer_usedregion(&decoded_buf, &data);
	} else if (socket->h2.request_type == ISC_HTTP_REQ_POST) {
		INSIST(socket->h2.content_length > 0);
		isc_buffer_usedregion(&socket->h2.rbuf, &data);
	} else {
		UNREACHABLE();
	}

	server_call_cb(socket, session, ISC_R_SUCCESS, &data);

	return (0);

error:
	result = server_send_error_response(code, ngsession, socket);
	if (result != ISC_R_SUCCESS) {
		return (NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE);
	}
	return (0);
}

void
isc__nm_http_send(isc_nmhandle_t *handle, const isc_region_t *region,
		  isc_nm_cb_t cb, void *cbarg) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_httpsend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(VALID_NMSOCK(sock));

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	isc_nmhandle_attach(handle, &uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	ievent = isc__nm_get_netievent_httpsend(sock->mgr, sock, uvreq);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

static void
failed_send_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
	       isc_result_t eresult) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));

	if (req->cb.send != NULL) {
		isc__nm_sendcb(sock, req, eresult, true);
	} else {
		isc__nm_uvreq_put(&req, sock);
	}
}

static void
client_httpsend(isc_nmhandle_t *handle, isc_nmsocket_t *sock,
		isc__nm_uvreq_t *req) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_nm_cb_t cb = req->cb.send;
	void *cbarg = req->cbarg;

	result = client_send(
		handle,
		&(isc_region_t){ (uint8_t *)req->uvbuf.base, req->uvbuf.len });
	if (result != ISC_R_SUCCESS) {
		failed_send_cb(sock, req, result);
		return;
	}

	http_do_bio(sock->h2.session, handle, cb, cbarg);
	isc__nm_uvreq_put(&req, sock);
}

static void
server_httpsend(isc_nmhandle_t *handle, isc_nmsocket_t *sock,
		isc__nm_uvreq_t *req) {
	size_t content_len_buf_len, cache_control_buf_len;
	isc_result_t result = ISC_R_SUCCESS;
	isc_nm_cb_t cb = req->cb.send;
	void *cbarg = req->cbarg;
	if (isc__nmsocket_closing(sock) ||
	    !http_session_active(handle->httpsession))
	{
		failed_send_cb(sock, req, ISC_R_CANCELED);
		return;
	}

	INSIST(handle->httpsession->handle->sock->tid == isc_nm_tid());
	INSIST(VALID_NMHANDLE(handle->httpsession->handle));
	INSIST(VALID_NMSOCK(handle->httpsession->handle->sock));

	isc_buffer_init(&sock->h2.wbuf, req->uvbuf.base, req->uvbuf.len);
	isc_buffer_add(&sock->h2.wbuf, req->uvbuf.len);

	content_len_buf_len = snprintf(sock->h2.clenbuf,
				       sizeof(sock->h2.clenbuf), "%lu",
				       (unsigned long)req->uvbuf.len);
	if (sock->h2.min_ttl == 0) {
		cache_control_buf_len =
			snprintf(sock->h2.cache_control_buf,
				 sizeof(sock->h2.cache_control_buf), "%s",
				 DEFAULT_CACHE_CONTROL);
	} else {
		cache_control_buf_len =
			snprintf(sock->h2.cache_control_buf,
				 sizeof(sock->h2.cache_control_buf),
				 "max-age=%" PRIu32, sock->h2.min_ttl);
	}
	const nghttp2_nv hdrs[] = { MAKE_NV2(":status", "200"),
				    MAKE_NV2("Content-Type", DNS_MEDIA_TYPE),
				    MAKE_NV("Content-Length", sock->h2.clenbuf,
					    content_len_buf_len),
				    MAKE_NV("Cache-Control",
					    sock->h2.cache_control_buf,
					    cache_control_buf_len) };

	result = server_send_response(handle->httpsession->ngsession,
				      sock->h2.stream_id, hdrs,
				      sizeof(hdrs) / sizeof(nghttp2_nv), sock);

	if (result == ISC_R_SUCCESS) {
		http_do_bio(handle->httpsession, handle, cb, cbarg);
	} else {
		cb(handle, result, cbarg);
	}
	isc__nm_uvreq_put(&req, sock);
}

void
isc__nm_async_httpsend(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_httpsend_t *ievent = (isc__netievent_httpsend_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	isc_nmhandle_t *handle = NULL;
	isc_nm_http_session_t *session = NULL;

	UNUSED(worker);

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(VALID_HTTP2_SESSION(sock->h2.session));

	ievent->req = NULL;
	handle = req->handle;

	REQUIRE(VALID_NMHANDLE(handle));

	session = sock->h2.session;
	if (session != NULL && session->client) {
		client_httpsend(handle, sock, req);
	} else {
		server_httpsend(handle, sock, req);
	}
}

void
isc__nm_http_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	isc_result_t result;
	http_cstream_t *cstream = NULL;
	isc_nm_http_session_t *session = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	session = handle->sock->h2.session;
	if (!http_session_active(session)) {
		cb(handle, ISC_R_CANCELED, NULL, cbarg);
		return;
	}

	result = get_http_cstream(handle->sock, &cstream);
	if (result != ISC_R_SUCCESS) {
		return;
	}

	handle->sock->h2.connect.cstream = cstream;
	cstream->read_cb = cb;
	cstream->read_cbarg = cbarg;
	cstream->reading = true;

	if (cstream->sending) {
		result = client_submit_request(session, cstream);
		if (result != ISC_R_SUCCESS) {
			put_http_cstream(session->mctx, cstream);
			return;
		}

		http_do_bio(session, NULL, NULL, NULL);
	}
}

static int
server_on_frame_recv_callback(nghttp2_session *ngsession,
			      const nghttp2_frame *frame, void *user_data) {
	isc_nm_http_session_t *session = (isc_nm_http_session_t *)user_data;
	isc_nmsocket_t *socket = NULL;

	switch (frame->hd.type) {
	case NGHTTP2_DATA:
	case NGHTTP2_HEADERS:
		/* Check that the client request has finished */
		if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
			socket = nghttp2_session_get_stream_user_data(
				ngsession, frame->hd.stream_id);

			/*
			 * For DATA and HEADERS frame, this callback may be
			 * called after on_stream_close_callback. Check that
			 * the stream is still alive.
			 */
			if (socket == NULL) {
				return (0);
			}

			return (server_on_request_recv(ngsession, session,
						       socket));
		}
		break;
	default:
		break;
	}
	return (0);
}

static void
initialize_nghttp2_server_session(isc_nm_http_session_t *session) {
	nghttp2_session_callbacks *callbacks = NULL;
	nghttp2_mem mem;

	init_nghttp2_mem(session->mctx, &mem);

	RUNTIME_CHECK(nghttp2_session_callbacks_new(&callbacks) == 0);

	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
		callbacks, on_data_chunk_recv_callback);

	nghttp2_session_callbacks_set_on_stream_close_callback(
		callbacks, on_stream_close_callback);

	nghttp2_session_callbacks_set_on_header_callback(
		callbacks, server_on_header_callback);

	nghttp2_session_callbacks_set_on_begin_headers_callback(
		callbacks, server_on_begin_headers_callback);

	nghttp2_session_callbacks_set_on_frame_recv_callback(
		callbacks, server_on_frame_recv_callback);

	RUNTIME_CHECK(nghttp2_session_server_new3(&session->ngsession,
						  callbacks, session, NULL,
						  &mem) == 0);

	nghttp2_session_callbacks_del(callbacks);
}

static int
server_send_connection_header(isc_nm_http_session_t *session) {
	nghttp2_settings_entry iv[1] = {
		{ NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS,
		  session->max_concurrent_streams }
	};
	int rv;

	rv = nghttp2_submit_settings(session->ngsession, NGHTTP2_FLAG_NONE, iv,
				     1);
	if (rv != 0) {
		return (-1);
	}
	return (0);
}

/*
 * It is advisable to disable Nagle's algorithm for HTTP/2
 * connections because multiple HTTP/2 streams could be multiplexed
 * over one transport connection. Thus, delays when delivering small
 * packets could bring down performance for the whole session.
 * HTTP/2 is meant to be used this way.
 */
static void
http_transpost_tcp_nodelay(isc_nmhandle_t *transphandle) {
	isc_nmsocket_t *tcpsock = NULL;
	uv_os_fd_t tcp_fd = (uv_os_fd_t)-1;

	if (transphandle->sock->type == isc_nm_tlssocket) {
		tcpsock = transphandle->sock->outerhandle->sock;
	} else {
		tcpsock = transphandle->sock;
	}

	(void)uv_fileno((uv_handle_t *)&tcpsock->uv_handle.tcp, &tcp_fd);
	RUNTIME_CHECK(tcp_fd != (uv_os_fd_t)-1);
	(void)isc__nm_socket_tcp_nodelay((uv_os_sock_t)tcp_fd);
}

static isc_result_t
httplisten_acceptcb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *httplistensock = (isc_nmsocket_t *)cbarg;
	isc_nm_http_session_t *session = NULL;
	isc_nmsocket_t *listener = NULL, *httpserver = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	if (handle->sock->type == isc_nm_tlssocket) {
		REQUIRE(VALID_NMSOCK(handle->sock->listener));
		listener = handle->sock->listener;
		httpserver = listener->h2.httpserver;
	} else {
		REQUIRE(VALID_NMSOCK(handle->sock->server));
		listener = handle->sock->server;
		REQUIRE(VALID_NMSOCK(listener->parent));
		httpserver = listener->parent->h2.httpserver;
	}

	/*
	 * NOTE: HTTP listener socket might be destroyed by the time this
	 * function gets invoked, so we need to do extra sanity checks to
	 * detect this case.
	 */
	if (isc__nmsocket_closing(handle->sock) || httpserver == NULL) {
		return (ISC_R_CANCELED);
	}

	if (result != ISC_R_SUCCESS) {
		/* XXXWPK do nothing? */
		return (result);
	}

	REQUIRE(VALID_NMSOCK(httplistensock));
	INSIST(httplistensock == httpserver);

	if (isc__nmsocket_closing(httplistensock) ||
	    !atomic_load(&httplistensock->listening))
	{
		return (ISC_R_CANCELED);
	}

	http_transpost_tcp_nodelay(handle);

	new_session(httplistensock->mgr->mctx, NULL, &session);
	session->max_concurrent_streams =
		atomic_load(&httplistensock->h2.max_concurrent_streams);
	initialize_nghttp2_server_session(session);
	handle->sock->h2.session = session;

	isc_nmhandle_attach(handle, &session->handle);
	isc__nmsocket_attach(httplistensock, &session->serversocket);
	server_send_connection_header(session);

	/* TODO H2 */
	http_do_bio(session, NULL, NULL, NULL);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_nm_listenhttp(isc_nm_t *mgr, isc_sockaddr_t *iface, int backlog,
		  isc_quota_t *quota, isc_tlsctx_t *ctx,
		  isc_nm_http_endpoints_t *eps, uint32_t max_concurrent_streams,
		  isc_nmsocket_t **sockp) {
	isc_nmsocket_t *sock = NULL;
	isc_result_t result;

	REQUIRE(VALID_NM(mgr));
	REQUIRE(!ISC_LIST_EMPTY(eps->handlers));
	REQUIRE(!ISC_LIST_EMPTY(eps->handler_cbargs));
	REQUIRE(atomic_load(&eps->in_use) == false);

	sock = isc_mem_get(mgr->mctx, sizeof(*sock));
	isc__nmsocket_init(sock, mgr, isc_nm_httplistener, iface);
	atomic_init(&sock->h2.max_concurrent_streams,
		    NGHTTP2_INITIAL_MAX_CONCURRENT_STREAMS);

	isc_nmsocket_set_max_streams(sock, max_concurrent_streams);

	atomic_store(&eps->in_use, true);
	http_init_listener_endpoints(sock, eps);

	if (ctx != NULL) {
		result = isc_nm_listentls(mgr, iface, httplisten_acceptcb, sock,
					  sizeof(isc_nm_http_session_t),
					  backlog, quota, ctx, &sock->outer);
	} else {
		result = isc_nm_listentcp(mgr, iface, httplisten_acceptcb, sock,
					  sizeof(isc_nm_http_session_t),
					  backlog, quota, &sock->outer);
	}

	if (result != ISC_R_SUCCESS) {
		atomic_store(&sock->closed, true);
		isc__nmsocket_detach(&sock);
		return (result);
	}

	isc__nmsocket_attach(sock, &sock->outer->h2.httpserver);

	sock->nchildren = sock->outer->nchildren;
	sock->result = ISC_R_UNSET;
	sock->tid = 0;
	sock->fd = (uv_os_sock_t)-1;

	isc__nmsocket_barrier_init(sock);
	atomic_init(&sock->rchildren, sock->nchildren);

	atomic_store(&sock->listening, true);
	*sockp = sock;
	return (ISC_R_SUCCESS);
}

isc_nm_http_endpoints_t *
isc_nm_http_endpoints_new(isc_mem_t *mctx) {
	isc_nm_http_endpoints_t *restrict eps;
	REQUIRE(mctx != NULL);

	eps = isc_mem_get(mctx, sizeof(*eps));
	*eps = (isc_nm_http_endpoints_t){ .mctx = NULL };

	isc_mem_attach(mctx, &eps->mctx);
	ISC_LIST_INIT(eps->handler_cbargs);
	ISC_LIST_INIT(eps->handlers);
	isc_refcount_init(&eps->references, 1);
	atomic_init(&eps->in_use, false);
	eps->magic = HTTP_ENDPOINTS_MAGIC;

	return eps;
}

void
isc_nm_http_endpoints_detach(isc_nm_http_endpoints_t **restrict epsp) {
	isc_nm_http_endpoints_t *restrict eps;
	isc_mem_t *mctx;
	isc_nm_httphandler_t *handler = NULL;
	isc_nm_httpcbarg_t *httpcbarg = NULL;

	REQUIRE(epsp != NULL);
	eps = *epsp;
	REQUIRE(VALID_HTTP_ENDPOINTS(eps));

	if (isc_refcount_decrement(&eps->references) > 1) {
		*epsp = NULL;
		return;
	}

	mctx = eps->mctx;

	/* Delete all handlers */
	handler = ISC_LIST_HEAD(eps->handlers);
	while (handler != NULL) {
		isc_nm_httphandler_t *next = NULL;

		next = ISC_LIST_NEXT(handler, link);
		ISC_LIST_DEQUEUE(eps->handlers, handler, link);
		isc_mem_free(mctx, handler->path);
		isc_mem_put(mctx, handler, sizeof(*handler));
		handler = next;
	}

	httpcbarg = ISC_LIST_HEAD(eps->handler_cbargs);
	while (httpcbarg != NULL) {
		isc_nm_httpcbarg_t *next = NULL;

		next = ISC_LIST_NEXT(httpcbarg, link);
		ISC_LIST_DEQUEUE(eps->handler_cbargs, httpcbarg, link);
		isc_mem_put(mctx, httpcbarg, sizeof(isc_nm_httpcbarg_t));
		httpcbarg = next;
	}

	eps->magic = 0;

	isc_mem_putanddetach(&mctx, eps, sizeof(*eps));
	*epsp = NULL;
}

void
isc_nm_http_endpoints_attach(isc_nm_http_endpoints_t *source,
			     isc_nm_http_endpoints_t **targetp) {
	REQUIRE(VALID_HTTP_ENDPOINTS(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static isc_nm_httphandler_t *
http_endpoints_find(const char *request_path,
		    const isc_nm_http_endpoints_t *restrict eps) {
	isc_nm_httphandler_t *handler = NULL;

	REQUIRE(VALID_HTTP_ENDPOINTS(eps));

	if (request_path == NULL || *request_path == '\0') {
		return (NULL);
	}

	for (handler = ISC_LIST_HEAD(eps->handlers); handler != NULL;
	     handler = ISC_LIST_NEXT(handler, link))
	{
		if (!strcmp(request_path, handler->path)) {
			break;
		}
	}

	return (handler);
}

/*
 * In DoH we just need to intercept the request - the response can be sent
 * to the client code via the nmhandle directly as it's always just the
 * http content.
 */
static void
http_callback(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *data,
	      void *arg) {
	isc_nm_httpcbarg_t *httpcbarg = arg;

	REQUIRE(VALID_NMHANDLE(handle));

	if (result != ISC_R_SUCCESS) {
		/* Shut down the client, then ourselves */
		httpcbarg->cb(handle, result, NULL, httpcbarg->cbarg);
		/* XXXWPK FREE */
		return;
	}
	httpcbarg->cb(handle, result, data, httpcbarg->cbarg);
}

isc_result_t
isc_nm_http_endpoints_add(isc_nm_http_endpoints_t *restrict eps,
			  const char *uri, const isc_nm_recv_cb_t cb,
			  void *cbarg, const size_t extrahandlesize) {
	isc_mem_t *mctx;
	isc_nm_httphandler_t *restrict handler = NULL;
	isc_nm_httpcbarg_t *restrict httpcbarg = NULL;
	bool newhandler = false;

	REQUIRE(VALID_HTTP_ENDPOINTS(eps));
	REQUIRE(isc_nm_http_path_isvalid(uri));
	REQUIRE(atomic_load(&eps->in_use) == false);

	mctx = eps->mctx;

	httpcbarg = isc_mem_get(mctx, sizeof(isc_nm_httpcbarg_t));
	*httpcbarg = (isc_nm_httpcbarg_t){ .cb = cb, .cbarg = cbarg };
	ISC_LINK_INIT(httpcbarg, link);

	if (http_endpoints_find(uri, eps) == NULL) {
		handler = isc_mem_get(mctx, sizeof(*handler));
		*handler = (isc_nm_httphandler_t){
			.cb = http_callback,
			.cbarg = httpcbarg,
			.extrahandlesize = extrahandlesize,
			.path = isc_mem_strdup(mctx, uri)
		};
		ISC_LINK_INIT(handler, link);

		newhandler = true;
	}

	if (newhandler) {
		ISC_LIST_APPEND(eps->handlers, handler, link);
	}
	ISC_LIST_APPEND(eps->handler_cbargs, httpcbarg, link);
	return (ISC_R_SUCCESS);
}

void
isc__nm_http_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_httplistener);

	isc__nmsocket_stop(sock);
}

static void
http_close_direct(isc_nmsocket_t *sock) {
	isc_nm_http_session_t *session = NULL;

	REQUIRE(VALID_NMSOCK(sock));

	atomic_store(&sock->closed, true);
	atomic_store(&sock->active, false);
	session = sock->h2.session;

	if (session != NULL && session->sending == 0 && !session->reading) {
		/*
		 * The socket is going to be closed too early without been
		 * used even once (might happen in a case of low level
		 * error).
		 */
		finish_http_session(session);
	} else if (session != NULL && session->handle) {
		http_do_bio(session, NULL, NULL, NULL);
	}
}

void
isc__nm_http_close(isc_nmsocket_t *sock) {
	bool destroy = false;
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_httpsocket);
	REQUIRE(!isc__nmsocket_active(sock));

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true))
	{
		return;
	}

	if (sock->h2.session != NULL && sock->h2.session->closed &&
	    sock->tid == isc_nm_tid())
	{
		isc__nm_httpsession_detach(&sock->h2.session);
		destroy = true;
	} else if (sock->h2.session == NULL && sock->tid == isc_nm_tid()) {
		destroy = true;
	}

	if (destroy) {
		http_close_direct(sock);
		isc__nmsocket_prep_destroy(sock);
		return;
	}

	isc__netievent_httpclose_t *ievent =
		isc__nm_get_netievent_httpclose(sock->mgr, sock);

	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_httpclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_httpclose_t *ievent = (isc__netievent_httpclose_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	UNUSED(worker);

	http_close_direct(sock);
}

static void
failed_httpstream_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			  isc_nm_http_session_t *session) {
	isc_region_t data;
	REQUIRE(VALID_NMSOCK(sock));
	INSIST(sock->type == isc_nm_httpsocket);

	if (sock->h2.request_path == NULL) {
		return;
	}

	INSIST(sock->h2.cbarg != NULL);

	(void)nghttp2_submit_rst_stream(
		session->ngsession, NGHTTP2_FLAG_END_STREAM, sock->h2.stream_id,
		NGHTTP2_REFUSED_STREAM);
	isc_buffer_usedregion(&sock->h2.rbuf, &data);
	server_call_cb(sock, session, result, &data);
}

static void
client_call_failed_read_cb(isc_result_t result,
			   isc_nm_http_session_t *session) {
	http_cstream_t *cstream = NULL;

	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(result != ISC_R_SUCCESS);

	cstream = ISC_LIST_HEAD(session->cstreams);
	while (cstream != NULL) {
		http_cstream_t *next = ISC_LIST_NEXT(cstream, link);

		/*
		 * read_cb could be NULL if cstream was allocated and added
		 * to the tracking list, but was not properly initialized due
		 * to a low-level error. It is safe to get rid of the object
		 * in such a case.
		 */
		if (cstream->read_cb != NULL) {
			isc_region_t read_data;
			isc_buffer_usedregion(cstream->rbuf, &read_data);
			cstream->read_cb(session->client_httphandle, result,
					 &read_data, cstream->read_cbarg);
		}

		if (result != ISC_R_TIMEDOUT || cstream->read_cb == NULL ||
		    !isc__nmsocket_timer_running(session->handle->sock))
		{
			ISC_LIST_DEQUEUE(session->cstreams, cstream, link);
			put_http_cstream(session->mctx, cstream);
		}

		cstream = next;
	}
}

static void
server_call_failed_read_cb(isc_result_t result,
			   isc_nm_http_session_t *session) {
	isc_nmsocket_h2_t *h2data = NULL; /* stream socket */

	REQUIRE(VALID_HTTP2_SESSION(session));
	REQUIRE(result != ISC_R_SUCCESS);

	for (h2data = ISC_LIST_HEAD(session->sstreams); h2data != NULL;
	     h2data = ISC_LIST_NEXT(h2data, link))
	{
		failed_httpstream_read_cb(h2data->psock, result, session);
	}

	h2data = ISC_LIST_HEAD(session->sstreams);
	while (h2data != NULL) {
		isc_nmsocket_h2_t *next = ISC_LIST_NEXT(h2data, link);
		ISC_LIST_DEQUEUE(session->sstreams, h2data, link);
		/* Cleanup socket in place */
		atomic_store(&h2data->psock->active, false);
		atomic_store(&h2data->psock->closed, true);
		isc__nmsocket_detach(&h2data->psock);

		h2data = next;
	}
}

static void
failed_read_cb(isc_result_t result, isc_nm_http_session_t *session) {
	if (session->client) {
		client_call_failed_read_cb(result, session);
		/*
		 * If result was ISC_R_TIMEDOUT and the timer was reset,
		 * then we still have active streams and should not close
		 * the session.
		 */
		if (ISC_LIST_EMPTY(session->cstreams)) {
			finish_http_session(session);
		}
	} else {
		server_call_failed_read_cb(result, session);
		/*
		 * All streams are now destroyed; close the session.
		 */
		finish_http_session(session);
	}
}

void
isc__nm_http_set_maxage(isc_nmhandle_t *handle, const uint32_t ttl) {
	isc_nm_http_session_t *session;
	isc_nmsocket_t *sock;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;
	session = sock->h2.session;

	INSIST(VALID_HTTP2_SESSION(session));
	INSIST(!session->client);

	sock->h2.min_ttl = ttl;
}

bool
isc__nm_http_has_encryption(const isc_nmhandle_t *handle) {
	isc_nm_http_session_t *session;
	isc_nmsocket_t *sock;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;
	session = sock->h2.session;

	INSIST(VALID_HTTP2_SESSION(session));

	return (isc_nm_socket_type(session->handle) == isc_nm_tlssocket);
}

const char *
isc__nm_http_verify_tls_peer_result_string(const isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;
	isc_nm_http_session_t *session;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_httpsocket);

	sock = handle->sock;
	session = sock->h2.session;

	/*
	 * In the case of a low-level error the session->handle is not
	 * attached nor session object is created.
	 */
	if (session == NULL && sock->h2.connect.tls_peer_verify_string != NULL)
	{
		return (sock->h2.connect.tls_peer_verify_string);
	}

	if (session == NULL) {
		return (NULL);
	}

	INSIST(VALID_HTTP2_SESSION(session));

	return (isc_nm_verify_tls_peer_result_string(session->handle));
}

void
isc__nm_http_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx) {
	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(listener->type == isc_nm_httplistener);

	isc_nmsocket_set_tlsctx(listener->outer, tlsctx);
}

void
isc__nm_http_set_max_streams(isc_nmsocket_t *listener,
			     const uint32_t max_concurrent_streams) {
	uint32_t max_streams = NGHTTP2_INITIAL_MAX_CONCURRENT_STREAMS;

	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(listener->type == isc_nm_httplistener);

	if (max_concurrent_streams > 0 &&
	    max_concurrent_streams < NGHTTP2_INITIAL_MAX_CONCURRENT_STREAMS)
	{
		max_streams = max_concurrent_streams;
	}

	atomic_store(&listener->h2.max_concurrent_streams, max_streams);
}

void
isc_nm_http_set_endpoints(isc_nmsocket_t *listener,
			  isc_nm_http_endpoints_t *eps) {
	size_t nlisteners;

	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(listener->type == isc_nm_httplistener);
	REQUIRE(VALID_HTTP_ENDPOINTS(eps));

	atomic_store(&eps->in_use, true);

	nlisteners = (size_t)listener->mgr->nlisteners;
	for (size_t i = 0; i < nlisteners; i++) {
		isc__netievent__http_eps_t *ievent =
			isc__nm_get_netievent_httpendpoints(listener->mgr,
							    listener, eps);
		isc__nm_enqueue_ievent(&listener->mgr->workers[i],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_httpendpoints(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent__http_eps_t *ievent = (isc__netievent__http_eps_t *)ev0;
	const int tid = isc_nm_tid();
	isc_nmsocket_t *listener = ievent->sock;
	isc_nm_http_endpoints_t *eps = ievent->endpoints;
	UNUSED(worker);

	isc_nm_http_endpoints_detach(&listener->h2.listener_endpoints[tid]);
	isc_nm_http_endpoints_attach(eps,
				     &listener->h2.listener_endpoints[tid]);
}

static void
http_init_listener_endpoints(isc_nmsocket_t *listener,
			     isc_nm_http_endpoints_t *epset) {
	size_t nlisteners;

	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(VALID_NM(listener->mgr));
	REQUIRE(VALID_HTTP_ENDPOINTS(epset));

	nlisteners = (size_t)listener->mgr->nlisteners;
	INSIST(nlisteners > 0);

	listener->h2.listener_endpoints =
		isc_mem_get(listener->mgr->mctx,
			    sizeof(isc_nm_http_endpoints_t *) * nlisteners);
	listener->h2.n_listener_endpoints = nlisteners;
	for (size_t i = 0; i < nlisteners; i++) {
		listener->h2.listener_endpoints[i] = NULL;
		isc_nm_http_endpoints_attach(
			epset, &listener->h2.listener_endpoints[i]);
	}
}

static void
http_cleanup_listener_endpoints(isc_nmsocket_t *listener) {
	REQUIRE(VALID_NM(listener->mgr));

	if (listener->h2.listener_endpoints == NULL) {
		return;
	}

	for (size_t i = 0; i < listener->h2.n_listener_endpoints; i++) {
		isc_nm_http_endpoints_detach(
			&listener->h2.listener_endpoints[i]);
	}
	isc_mem_put(listener->mgr->mctx, listener->h2.listener_endpoints,
		    sizeof(isc_nm_http_endpoints_t *) *
			    listener->h2.n_listener_endpoints);
	listener->h2.n_listener_endpoints = 0;
}

static isc_nm_http_endpoints_t *
http_get_listener_endpoints(isc_nmsocket_t *listener, const int tid) {
	isc_nm_http_endpoints_t *eps;
	REQUIRE(VALID_NMSOCK(listener));
	REQUIRE(tid >= 0);
	REQUIRE((size_t)tid < listener->h2.n_listener_endpoints);

	eps = listener->h2.listener_endpoints[tid];
	INSIST(eps != NULL);
	return (eps);
}

static const bool base64url_validation_table[256] = {
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  false, false, true,  true,
	true,  true,  true,  true,  true,  true,  true,	 true,	false, false,
	false, false, false, false, false, true,  true,	 true,	true,  true,
	true,  true,  true,  true,  true,  true,  true,	 true,	true,  true,
	true,  true,  true,  true,  true,  true,  true,	 true,	true,  true,
	true,  false, false, false, false, true,  false, true,	true,  true,
	true,  true,  true,  true,  true,  true,  true,	 true,	true,  true,
	true,  true,  true,  true,  true,  true,  true,	 true,	true,  true,
	true,  true,  true,  false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false
};

char *
isc__nm_base64url_to_base64(isc_mem_t *mem, const char *base64url,
			    const size_t base64url_len, size_t *res_len) {
	char *res = NULL;
	size_t i, k, len;

	if (mem == NULL || base64url == NULL || base64url_len == 0) {
		return (NULL);
	}

	len = base64url_len % 4 ? base64url_len + (4 - base64url_len % 4)
				: base64url_len;
	res = isc_mem_allocate(mem, len + 1); /* '\0' */

	for (i = 0; i < base64url_len; i++) {
		switch (base64url[i]) {
		case '-':
			res[i] = '+';
			break;
		case '_':
			res[i] = '/';
			break;
		default:
			if (base64url_validation_table[(size_t)base64url[i]]) {
				res[i] = base64url[i];
			} else {
				isc_mem_free(mem, res);
				return (NULL);
			}
			break;
		}
	}

	if (base64url_len % 4 != 0) {
		for (k = 0; k < (4 - base64url_len % 4); k++, i++) {
			res[i] = '=';
		}
	}

	INSIST(i == len);

	if (res_len != NULL) {
		*res_len = len;
	}

	res[len] = '\0';

	return (res);
}

char *
isc__nm_base64_to_base64url(isc_mem_t *mem, const char *base64,
			    const size_t base64_len, size_t *res_len) {
	char *res = NULL;
	size_t i;

	if (mem == NULL || base64 == NULL || base64_len == 0) {
		return (NULL);
	}

	res = isc_mem_allocate(mem, base64_len + 1); /* '\0' */

	for (i = 0; i < base64_len; i++) {
		switch (base64[i]) {
		case '+':
			res[i] = '-';
			break;
		case '/':
			res[i] = '_';
			break;
		case '=':
			goto end;
			break;
		default:
			/*
			 * All other characters from the alphabet are the same
			 * for both base64 and base64url, so we can reuse the
			 * validation table for the rest of the characters.
			 */
			if (base64[i] != '-' && base64[i] != '_' &&
			    base64url_validation_table[(size_t)base64[i]])
			{
				res[i] = base64[i];
			} else {
				isc_mem_free(mem, res);
				return (NULL);
			}
			break;
		}
	}
end:
	if (res_len) {
		*res_len = i;
	}

	res[i] = '\0';

	return (res);
}

void
isc__nm_http_initsocket(isc_nmsocket_t *sock) {
	REQUIRE(sock != NULL);

	sock->h2 = (isc_nmsocket_h2_t){
		.request_type = ISC_HTTP_REQ_UNSUPPORTED,
		.request_scheme = ISC_HTTP_SCHEME_UNSUPPORTED,
	};
}

void
isc__nm_http_cleanup_data(isc_nmsocket_t *sock) {
	if ((sock->type == isc_nm_tcplistener ||
	     sock->type == isc_nm_tlslistener) &&
	    sock->h2.httpserver != NULL)
	{
		isc__nmsocket_detach(&sock->h2.httpserver);
	}

	if (sock->type == isc_nm_httplistener ||
	    sock->type == isc_nm_httpsocket)
	{
		if (sock->type == isc_nm_httplistener &&
		    sock->h2.listener_endpoints != NULL)
		{
			/* Delete all handlers */
			http_cleanup_listener_endpoints(sock);
		}

		if (sock->h2.request_path != NULL) {
			isc_mem_free(sock->mgr->mctx, sock->h2.request_path);
			sock->h2.request_path = NULL;
		}

		if (sock->h2.query_data != NULL) {
			isc_mem_free(sock->mgr->mctx, sock->h2.query_data);
			sock->h2.query_data = NULL;
		}

		INSIST(sock->h2.connect.cstream == NULL);

		if (isc_buffer_base(&sock->h2.rbuf) != NULL) {
			void *base = isc_buffer_base(&sock->h2.rbuf);
			isc_mem_free(sock->mgr->mctx, base);
			isc_buffer_initnull(&sock->h2.rbuf);
		}
	}

	if ((sock->type == isc_nm_httplistener ||
	     sock->type == isc_nm_httpsocket ||
	     sock->type == isc_nm_tcpsocket ||
	     sock->type == isc_nm_tlssocket) &&
	    sock->h2.session != NULL)
	{
		if (sock->h2.connect.uri != NULL) {
			isc_mem_free(sock->mgr->mctx, sock->h2.connect.uri);
			sock->h2.connect.uri = NULL;
		}
		isc__nm_httpsession_detach(&sock->h2.session);
	}
}

void
isc__nm_http_cleartimeout(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_httpsocket);

	sock = handle->sock;
	if (sock->h2.session != NULL && sock->h2.session->handle != NULL) {
		INSIST(VALID_HTTP2_SESSION(sock->h2.session));
		INSIST(VALID_NMHANDLE(sock->h2.session->handle));
		isc_nmhandle_cleartimeout(sock->h2.session->handle);
	}
}

void
isc__nm_http_settimeout(isc_nmhandle_t *handle, uint32_t timeout) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_httpsocket);

	sock = handle->sock;
	if (sock->h2.session != NULL && sock->h2.session->handle != NULL) {
		INSIST(VALID_HTTP2_SESSION(sock->h2.session));
		INSIST(VALID_NMHANDLE(sock->h2.session->handle));
		isc_nmhandle_settimeout(sock->h2.session->handle, timeout);
	}
}

void
isc__nmhandle_http_keepalive(isc_nmhandle_t *handle, bool value) {
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->type == isc_nm_httpsocket);

	sock = handle->sock;
	if (sock->h2.session != NULL && sock->h2.session->handle) {
		INSIST(VALID_HTTP2_SESSION(sock->h2.session));
		INSIST(VALID_NMHANDLE(sock->h2.session->handle));

		isc_nmhandle_keepalive(sock->h2.session->handle, value);
	}
}

void
isc_nm_http_makeuri(const bool https, const isc_sockaddr_t *sa,
		    const char *hostname, const uint16_t http_port,
		    const char *abs_path, char *outbuf,
		    const size_t outbuf_len) {
	char saddr[INET6_ADDRSTRLEN] = { 0 };
	int family;
	bool ipv6_addr = false;
	struct sockaddr_in6 sa6;
	uint16_t host_port = http_port;
	const char *host = NULL;

	REQUIRE(outbuf != NULL);
	REQUIRE(outbuf_len != 0);
	REQUIRE(isc_nm_http_path_isvalid(abs_path));

	/* If hostname is specified, use that. */
	if (hostname != NULL && hostname[0] != '\0') {
		/*
		 * The host name could be an IPv6 address. If so,
		 * wrap it between [ and ].
		 */
		if (inet_pton(AF_INET6, hostname, &sa6) == 1 &&
		    hostname[0] != '[')
		{
			ipv6_addr = true;
		}
		host = hostname;
	} else {
		/*
		 * A hostname was not specified; build one from
		 * the given IP address.
		 */
		INSIST(sa != NULL);
		family = ((const struct sockaddr *)&sa->type.sa)->sa_family;
		host_port = ntohs(family == AF_INET ? sa->type.sin.sin_port
						    : sa->type.sin6.sin6_port);
		ipv6_addr = family == AF_INET6;
		(void)inet_ntop(
			family,
			family == AF_INET
				? (const struct sockaddr *)&sa->type.sin.sin_addr
				: (const struct sockaddr *)&sa->type.sin6
					  .sin6_addr,
			saddr, sizeof(saddr));
		host = saddr;
	}

	/*
	 * If the port number was not specified, the default
	 * depends on whether we're using encryption or not.
	 */
	if (host_port == 0) {
		host_port = https ? 443 : 80;
	}

	(void)snprintf(outbuf, outbuf_len, "%s://%s%s%s:%u%s",
		       https ? "https" : "http", ipv6_addr ? "[" : "", host,
		       ipv6_addr ? "]" : "", host_port, abs_path);
}

/*
 * DoH GET Query String Scanner-less Recursive Descent Parser/Verifier
 *
 * It is based on the following grammar (using WSN/EBNF):
 *
 * S                = query-string.
 * query-string     = ['?'] { key-value-pair } EOF.
 * key-value-pair   = key '=' value [ '&' ].
 * key              = ('_' | alpha) { '_' | alnum}.
 * value            = value-char {value-char}.
 * value-char       = unreserved-char | percent-charcode.
 * unreserved-char  = alnum |'_' | '.' | '-' | '~'. (* RFC3986, Section 2.3 *)
 * percent-charcode = '%' hexdigit hexdigit.
 * ...
 *
 * Should be good enough.
 */
typedef struct isc_httpparser_state {
	const char *str;

	const char *last_key;
	size_t last_key_len;

	const char *last_value;
	size_t last_value_len;

	bool query_found;
	const char *query;
	size_t query_len;
} isc_httpparser_state_t;

#define MATCH(ch)      (st->str[0] == (ch))
#define MATCH_ALPHA()  isalpha((unsigned char)(st->str[0]))
#define MATCH_DIGIT()  isdigit((unsigned char)(st->str[0]))
#define MATCH_ALNUM()  isalnum((unsigned char)(st->str[0]))
#define MATCH_XDIGIT() isxdigit((unsigned char)(st->str[0]))
#define ADVANCE()      st->str++
#define GETP()	       (st->str)

static bool
rule_query_string(isc_httpparser_state_t *st);

bool
isc__nm_parse_httpquery(const char *query_string, const char **start,
			size_t *len) {
	isc_httpparser_state_t state;

	REQUIRE(start != NULL);
	REQUIRE(len != NULL);

	if (query_string == NULL || query_string[0] == '\0') {
		return (false);
	}

	state = (isc_httpparser_state_t){ .str = query_string };
	if (!rule_query_string(&state)) {
		return (false);
	}

	if (!state.query_found) {
		return (false);
	}

	*start = state.query;
	*len = state.query_len;

	return (true);
}

static bool
rule_key_value_pair(isc_httpparser_state_t *st);

static bool
rule_key(isc_httpparser_state_t *st);

static bool
rule_value(isc_httpparser_state_t *st);

static bool
rule_value_char(isc_httpparser_state_t *st);

static bool
rule_percent_charcode(isc_httpparser_state_t *st);

static bool
rule_unreserved_char(isc_httpparser_state_t *st);

static bool
rule_query_string(isc_httpparser_state_t *st) {
	if (MATCH('?')) {
		ADVANCE();
	}

	while (rule_key_value_pair(st)) {
		/* skip */;
	}

	if (!MATCH('\0')) {
		return (false);
	}

	ADVANCE();
	return (true);
}

static bool
rule_key_value_pair(isc_httpparser_state_t *st) {
	if (!rule_key(st)) {
		return (false);
	}

	if (MATCH('=')) {
		ADVANCE();
	} else {
		return (false);
	}

	if (rule_value(st)) {
		const char dns[] = "dns";
		if (st->last_key_len == sizeof(dns) - 1 &&
		    memcmp(st->last_key, dns, sizeof(dns) - 1) == 0)
		{
			st->query_found = true;
			st->query = st->last_value;
			st->query_len = st->last_value_len;
		}
	} else {
		return (false);
	}

	if (MATCH('&')) {
		ADVANCE();
	}

	return (true);
}

static bool
rule_key(isc_httpparser_state_t *st) {
	if (MATCH('_') || MATCH_ALPHA()) {
		st->last_key = GETP();
		ADVANCE();
	} else {
		return (false);
	}

	while (MATCH('_') || MATCH_ALNUM()) {
		ADVANCE();
	}

	st->last_key_len = GETP() - st->last_key;
	return (true);
}

static bool
rule_value(isc_httpparser_state_t *st) {
	const char *s = GETP();
	if (!rule_value_char(st)) {
		return (false);
	}

	st->last_value = s;
	while (rule_value_char(st)) {
		/* skip */;
	}
	st->last_value_len = GETP() - st->last_value;
	return (true);
}

static bool
rule_value_char(isc_httpparser_state_t *st) {
	if (rule_unreserved_char(st)) {
		return (true);
	}

	return (rule_percent_charcode(st));
}

static bool
rule_unreserved_char(isc_httpparser_state_t *st) {
	if (MATCH_ALNUM() || MATCH('_') || MATCH('.') || MATCH('-') ||
	    MATCH('~'))
	{
		ADVANCE();
		return (true);
	}
	return (false);
}

static bool
rule_percent_charcode(isc_httpparser_state_t *st) {
	if (MATCH('%')) {
		ADVANCE();
	} else {
		return (false);
	}

	if (!MATCH_XDIGIT()) {
		return (false);
	}
	ADVANCE();

	if (!MATCH_XDIGIT()) {
		return (false);
	}
	ADVANCE();

	return (true);
}

/*
 * DoH URL Location Verifier. Based on the following grammar (EBNF/WSN
 * notation):
 *
 * S             = path_absolute.
 * path_absolute = '/' [ segments ] '\0'.
 * segments      = segment_nz { slash_segment }.
 * slash_segment = '/' segment.
 * segment       = { pchar }.
 * segment_nz    = pchar { pchar }.
 * pchar         = unreserved | pct_encoded | sub_delims | ':' | '@'.
 * unreserved    = ALPHA | DIGIT | '-' | '.' | '_' | '~'.
 * pct_encoded   = '%' XDIGIT XDIGIT.
 * sub_delims    = '!' | '$' | '&' | '\'' | '(' | ')' | '*' | '+' |
 *                 ',' | ';' | '='.
 *
 * The grammar is extracted from RFC 3986. It is slightly modified to
 * aid in parser creation, but the end result is the same
 * (path_absolute is defined slightly differently - split into
 * multiple productions).
 *
 * https://datatracker.ietf.org/doc/html/rfc3986#appendix-A
 */

typedef struct isc_http_location_parser_state {
	const char *str;
} isc_http_location_parser_state_t;

static bool
rule_loc_path_absolute(isc_http_location_parser_state_t *);

static bool
rule_loc_segments(isc_http_location_parser_state_t *);

static bool
rule_loc_slash_segment(isc_http_location_parser_state_t *);

static bool
rule_loc_segment(isc_http_location_parser_state_t *);

static bool
rule_loc_segment_nz(isc_http_location_parser_state_t *);

static bool
rule_loc_pchar(isc_http_location_parser_state_t *);

static bool
rule_loc_unreserved(isc_http_location_parser_state_t *);

static bool
rule_loc_pct_encoded(isc_http_location_parser_state_t *);

static bool
rule_loc_sub_delims(isc_http_location_parser_state_t *);

static bool
rule_loc_path_absolute(isc_http_location_parser_state_t *st) {
	if (MATCH('/')) {
		ADVANCE();
	} else {
		return (false);
	}

	(void)rule_loc_segments(st);

	if (MATCH('\0')) {
		ADVANCE();
	} else {
		return (false);
	}

	return (true);
}

static bool
rule_loc_segments(isc_http_location_parser_state_t *st) {
	if (!rule_loc_segment_nz(st)) {
		return (false);
	}

	while (rule_loc_slash_segment(st)) {
		/* zero or more */;
	}

	return (true);
}

static bool
rule_loc_slash_segment(isc_http_location_parser_state_t *st) {
	if (MATCH('/')) {
		ADVANCE();
	} else {
		return (false);
	}

	return (rule_loc_segment(st));
}

static bool
rule_loc_segment(isc_http_location_parser_state_t *st) {
	while (rule_loc_pchar(st)) {
		/* zero or more */;
	}

	return (true);
}

static bool
rule_loc_segment_nz(isc_http_location_parser_state_t *st) {
	if (!rule_loc_pchar(st)) {
		return (false);
	}

	while (rule_loc_pchar(st)) {
		/* zero or more */;
	}

	return (true);
}

static bool
rule_loc_pchar(isc_http_location_parser_state_t *st) {
	if (rule_loc_unreserved(st)) {
		return (true);
	} else if (rule_loc_pct_encoded(st)) {
		return (true);
	} else if (rule_loc_sub_delims(st)) {
		return (true);
	} else if (MATCH(':') || MATCH('@')) {
		ADVANCE();
		return (true);
	}

	return (false);
}

static bool
rule_loc_unreserved(isc_http_location_parser_state_t *st) {
	if (MATCH_ALPHA() | MATCH_DIGIT() | MATCH('-') | MATCH('.') |
	    MATCH('_') | MATCH('~'))
	{
		ADVANCE();
		return (true);
	}
	return (false);
}

static bool
rule_loc_pct_encoded(isc_http_location_parser_state_t *st) {
	if (!MATCH('%')) {
		return (false);
	}
	ADVANCE();

	if (!MATCH_XDIGIT()) {
		return (false);
	}
	ADVANCE();

	if (!MATCH_XDIGIT()) {
		return (false);
	}
	ADVANCE();

	return (true);
}

static bool
rule_loc_sub_delims(isc_http_location_parser_state_t *st) {
	if (MATCH('!') | MATCH('$') | MATCH('&') | MATCH('\'') | MATCH('(') |
	    MATCH(')') | MATCH('*') | MATCH('+') | MATCH(',') | MATCH(';') |
	    MATCH('='))
	{
		ADVANCE();
		return (true);
	}

	return (false);
}

bool
isc_nm_http_path_isvalid(const char *path) {
	isc_http_location_parser_state_t state = { 0 };

	REQUIRE(path != NULL);

	state.str = path;

	return (rule_loc_path_absolute(&state));
}
