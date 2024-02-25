/*	$NetBSD: httpd.c,v 1.7.2.2 2024/02/25 15:47:16 martin Exp $	*/

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

/*! \file */

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <isc/buffer.h>
#include <isc/httpd.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/url.h>
#include <isc/util.h>

#include "netmgr/netmgr-int.h"
#include "picohttpparser.h"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif /* ifdef HAVE_ZLIB */

#define CHECK(m)                               \
	do {                                   \
		result = (m);                  \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

/*
 * Size the recv buffer to hold at maximum two full buffers from isc_nm_read(),
 * so we don't have to handle the truncation.
 */
#define HTTP_RECVLEN	     ISC_NETMGR_TCP_RECVBUF_SIZE * 2
#define HTTP_SENDLEN	     ISC_NETMGR_TCP_RECVBUF_SIZE
#define HTTP_HEADERS_NUM     100
#define HTTP_MAX_REQUEST_LEN 4096

typedef enum httpd_flags {
	CONNECTION_CLOSE = 1 << 0,	/* connection must close */
	CONNECTION_KEEP_ALIVE = 1 << 1, /* response needs a keep-alive header */
	ACCEPT_DEFLATE = 1 << 2,	/* response can be compressed */
} httpd_flags_t;

#define HTTPD_MAGIC    ISC_MAGIC('H', 't', 'p', 'd')
#define VALID_HTTPD(m) ISC_MAGIC_VALID(m, HTTPD_MAGIC)

#define HTTPDMGR_MAGIC	  ISC_MAGIC('H', 'p', 'd', 'm')
#define VALID_HTTPDMGR(m) ISC_MAGIC_VALID(m, HTTPDMGR_MAGIC)

/*%
 * HTTP methods.
 */
typedef enum { METHOD_UNKNOWN = 0, METHOD_GET = 1, METHOD_POST = 2 } method_t;

/*%
 * HTTP urls.  These are the URLs we manage, and the function to call to
 * provide the data for it.  We pass in the base url (so the same function
 * can handle multiple requests), and a structure to fill in to return a
 * result to the client.  We also pass in a pointer to be filled in for
 * the data cleanup function.
 */
struct isc_httpdurl {
	char *url;
	isc_httpdaction_t *action;
	void *action_arg;
	bool isstatic;
	isc_time_t loadtime;
	ISC_LINK(isc_httpdurl_t) link;
};

/*% http client */
struct isc_httpd {
	unsigned int magic; /* HTTPD_MAGIC */

	isc_httpdmgr_t *mgr; /*%< our parent */
	ISC_LINK(isc_httpd_t) link;

	isc_nmhandle_t *handle;	    /* Permanent pointer to handle */
	isc_nmhandle_t *readhandle; /* Waiting for a read callback */

	/*%
	 * Received data state.
	 */
	char recvbuf[HTTP_RECVLEN]; /*%< receive buffer */
	size_t recvlen;		    /*%< length recv'd */
	size_t consume;		    /*%< length of last command */

	method_t method;
	int minor_version;
	httpd_flags_t flags;
	const char *path;
	isc_url_parser_t up;
	isc_time_t if_modified_since;
};

struct isc_httpdmgr {
	unsigned int magic; /* HTTPDMGR_MAGIC */
	isc_refcount_t references;
	isc_mem_t *mctx;
	isc_nmsocket_t *sock;

	isc_httpdclientok_t *client_ok;	 /*%< client validator */
	isc_httpdondestroy_t *ondestroy; /*%< cleanup callback */
	void *cb_arg;			 /*%< argument for the above */

	unsigned int flags;
	ISC_LIST(isc_httpd_t) running; /*%< running clients */

	isc_mutex_t lock;

	ISC_LIST(isc_httpdurl_t) urls; /*%< urls we manage */
	isc_httpdaction_t *render_404;
	isc_httpdaction_t *render_500;
};

typedef struct isc_httpd_sendreq {
	isc_mem_t *mctx;
	isc_httpd_t *httpd;
	isc_nmhandle_t *handle;

	/*%
	 * Transmit data state.
	 *
	 * This is the data buffer we will transmit.
	 *
	 * This free function pointer is filled in by the rendering function
	 * we call.  The free function is called after the data is transmitted
	 * to the client.
	 *
	 * We currently use three buffers total:
	 *
	 * sendbuffer - gets filled as we gather the data
	 *
	 * bodybuffer - for the client to fill in (which it manages, it provides
	 * the space for it, etc) -- we will pass that buffer structure back to
	 * the caller, who is responsible for managing the space it may have
	 * allocated as backing store for it.  we only allocate the buffer
	 * itself, not the backing store.
	 *
	 * compbuffer - managed by us, that contains the compressed HTTP data,
	 * if compression is used.
	 */
	isc_buffer_t *sendbuffer;
	isc_buffer_t *compbuffer;

	isc_buffer_t bodybuffer;

	const char *mimetype;
	unsigned int retcode;
	const char *retmsg;
	isc_httpdfree_t *freecb;
	void *freecb_arg;

} isc_httpd_sendreq_t;

static isc_result_t
httpd_newconn(isc_nmhandle_t *, isc_result_t, void *);
static void
httpd_request(isc_nmhandle_t *, isc_result_t, isc_region_t *, void *);
static void
httpd_senddone(isc_nmhandle_t *, isc_result_t, void *);
static void
httpd_reset(void *);
static void
httpd_put(void *);

static void
httpd_addheader(isc_httpd_sendreq_t *, const char *, const char *);
static void
httpd_addheaderuint(isc_httpd_sendreq_t *, const char *, int);
static void
httpd_endheaders(isc_httpd_sendreq_t *);
static void
httpd_response(isc_httpd_t *, isc_httpd_sendreq_t *);

static isc_result_t
process_request(isc_httpd_t *, size_t);

static isc_httpdaction_t render_404;
static isc_httpdaction_t render_500;

#if ENABLE_AFL
static void (*finishhook)(void) = NULL;
#endif /* ENABLE_AFL */

static void
destroy_httpdmgr(isc_httpdmgr_t *);

static void
httpdmgr_attach(isc_httpdmgr_t *, isc_httpdmgr_t **);
static void
httpdmgr_detach(isc_httpdmgr_t **);

isc_result_t
isc_httpdmgr_create(isc_nm_t *nm, isc_mem_t *mctx, isc_sockaddr_t *addr,
		    isc_httpdclientok_t *client_ok,
		    isc_httpdondestroy_t *ondestroy, void *cb_arg,
		    isc_httpdmgr_t **httpdmgrp) {
	isc_result_t result;
	isc_httpdmgr_t *httpdmgr = NULL;

	REQUIRE(nm != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(httpdmgrp != NULL && *httpdmgrp == NULL);

	httpdmgr = isc_mem_get(mctx, sizeof(isc_httpdmgr_t));
	*httpdmgr = (isc_httpdmgr_t){ .client_ok = client_ok,
				      .ondestroy = ondestroy,
				      .cb_arg = cb_arg,
				      .render_404 = render_404,
				      .render_500 = render_500 };

	isc_mutex_init(&httpdmgr->lock);
	isc_mem_attach(mctx, &httpdmgr->mctx);

	ISC_LIST_INIT(httpdmgr->running);
	ISC_LIST_INIT(httpdmgr->urls);

	isc_refcount_init(&httpdmgr->references, 1);

	CHECK(isc_nm_listentcp(nm, addr, httpd_newconn, httpdmgr,
			       sizeof(isc_httpd_t), 5, NULL, &httpdmgr->sock));

	httpdmgr->magic = HTTPDMGR_MAGIC;
	*httpdmgrp = httpdmgr;

	return (ISC_R_SUCCESS);

cleanup:
	httpdmgr->magic = 0;
	isc_refcount_decrementz(&httpdmgr->references);
	isc_refcount_destroy(&httpdmgr->references);
	isc_mem_detach(&httpdmgr->mctx);
	isc_mutex_destroy(&httpdmgr->lock);
	isc_mem_put(mctx, httpdmgr, sizeof(isc_httpdmgr_t));

	return (result);
}

static void
httpdmgr_attach(isc_httpdmgr_t *source, isc_httpdmgr_t **targetp) {
	REQUIRE(VALID_HTTPDMGR(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
httpdmgr_detach(isc_httpdmgr_t **httpdmgrp) {
	isc_httpdmgr_t *httpdmgr = NULL;

	REQUIRE(httpdmgrp != NULL);
	REQUIRE(VALID_HTTPDMGR(*httpdmgrp));

	httpdmgr = *httpdmgrp;
	*httpdmgrp = NULL;

	if (isc_refcount_decrement(&httpdmgr->references) == 1) {
		destroy_httpdmgr(httpdmgr);
	}
}

static void
destroy_httpdmgr(isc_httpdmgr_t *httpdmgr) {
	isc_httpdurl_t *url;

	isc_refcount_destroy(&httpdmgr->references);

	LOCK(&httpdmgr->lock);

	REQUIRE((httpdmgr->flags & ISC_HTTPDMGR_SHUTTINGDOWN) != 0);
	REQUIRE(ISC_LIST_EMPTY(httpdmgr->running));

	httpdmgr->magic = 0;

	if (httpdmgr->sock != NULL) {
		isc_nmsocket_close(&httpdmgr->sock);
	}

	/*
	 * Clear out the list of all actions we know about.  Just free the
	 * memory.
	 */
	url = ISC_LIST_HEAD(httpdmgr->urls);
	while (url != NULL) {
		isc_mem_free(httpdmgr->mctx, url->url);
		ISC_LIST_UNLINK(httpdmgr->urls, url, link);
		isc_mem_put(httpdmgr->mctx, url, sizeof(isc_httpdurl_t));
		url = ISC_LIST_HEAD(httpdmgr->urls);
	}

	UNLOCK(&httpdmgr->lock);
	isc_mutex_destroy(&httpdmgr->lock);

	if (httpdmgr->ondestroy != NULL) {
		(httpdmgr->ondestroy)(httpdmgr->cb_arg);
	}
	isc_mem_putanddetach(&httpdmgr->mctx, httpdmgr, sizeof(isc_httpdmgr_t));
}

static bool
name_match(const struct phr_header *header, const char *match) {
	size_t match_len = strlen(match);
	if (match_len != header->name_len) {
		return (false);
	}
	return (strncasecmp(header->name, match, match_len) == 0);
}

static bool
value_match(const struct phr_header *header, const char *match) {
	size_t match_len = strlen(match);
	size_t limit;

	if (match_len > header->value_len) {
		return (false);
	}

	limit = header->value_len - match_len + 1;

	for (size_t i = 0; i < limit; i++) {
		if (isspace((unsigned char)header->value[i])) {
			while (i < limit &&
			       isspace((unsigned char)header->value[i]))
			{
				i++;
			}
			continue;
		}

		if (strncasecmp(&header->value[i], match, match_len) == 0) {
			i += match_len;
			/*
			 * Sanity check; f.e. for 'deflate' match only
			 * 'deflate[,;]', but not 'deflateyou'
			 */
			if (i == header->value_len || header->value[i] == ',' ||
			    header->value[i] == ';')
			{
				return (true);
			}
		}

		while (i < limit && header->value[i] != ',') {
			i++;
		}
	}
	return (false);
}

static isc_result_t
process_request(isc_httpd_t *httpd, size_t last_len) {
	int pret;
	const char *method = NULL;
	size_t method_len = 0;
	const char *path;
	size_t path_len = 0;
	struct phr_header headers[HTTP_HEADERS_NUM];
	size_t num_headers;
	isc_result_t result;

	num_headers = ARRAY_SIZE(headers);

	pret = phr_parse_request(httpd->recvbuf, httpd->recvlen, &method,
				 &method_len, &path, &path_len,
				 &httpd->minor_version, headers, &num_headers,
				 last_len);

	if (pret == -1) {
		/* Parse Error */
		return (ISC_R_UNEXPECTED);
	} else if (pret == -2) {
		/* Need more data */
		return (ISC_R_NOMORE);
	}

	INSIST(pret > 0);

	if (pret > HTTP_MAX_REQUEST_LEN) {
		return (ISC_R_RANGE);
	}

	httpd->consume = pret;

	/*
	 * Determine if this is a POST or GET method.  Any other values will
	 * cause an error to be returned.
	 */
	if (strncmp(method, "GET ", method_len) == 0) {
		httpd->method = METHOD_GET;
	} else if (strncmp(method, "POST ", method_len) == 0) {
		httpd->method = METHOD_POST;
	} else {
		return (ISC_R_RANGE);
	}

	/*
	 * Parse the URL
	 */
	result = isc_url_parse(path, path_len, 0, &httpd->up);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	httpd->path = path;

	/*
	 * Examine headers that can affect this request's response
	 */
	httpd->flags = 0;

	size_t content_len = 0;
	bool keep_alive = false;
	bool host_header = false;

	isc_time_set(&httpd->if_modified_since, 0, 0);

	for (size_t i = 0; i < num_headers; i++) {
		struct phr_header *header = &headers[i];

		if (name_match(header, "Content-Length")) {
			char *endptr;
			long val = strtol(header->value, &endptr, 10);

			errno = 0;

			/* ensure we consumed all digits */
			if ((header->value + header->value_len) != endptr) {
				return (ISC_R_BADNUMBER);
			}
			/* ensure there was no minus sign */
			if (val < 0) {
				return (ISC_R_BADNUMBER);
			}
			/* ensure it did not overflow */
			if (errno != 0) {
				return (ISC_R_RANGE);
			}
			content_len = val;
		} else if (name_match(header, "Connection")) {
			/* multiple fields in a connection header are allowed */
			if (value_match(header, "close")) {
				httpd->flags |= CONNECTION_CLOSE;
			}
			if (value_match(header, "keep-alive")) {
				keep_alive = true;
			}
		} else if (name_match(header, "Host")) {
			host_header = true;
		} else if (name_match(header, "Accept-Encoding")) {
			if (value_match(header, "deflate")) {
				httpd->flags |= ACCEPT_DEFLATE;
			}
		} else if (name_match(header, "If-Modified-Since") &&
			   header->value_len < ISC_FORMATHTTPTIMESTAMP_SIZE)
		{
			char timestamp[ISC_FORMATHTTPTIMESTAMP_SIZE + 1];
			memmove(timestamp, header->value, header->value_len);
			timestamp[header->value_len] = 0;

			/* Ignore the value if it can't be parsed */
			(void)isc_time_parsehttptimestamp(
				timestamp, &httpd->if_modified_since);
		}
	}

	/*
	 * The Content-Length is optional in an HTTP request.
	 * For a GET the length must be zero.
	 */
	if (httpd->method == METHOD_GET && content_len != 0) {
		return (ISC_R_BADNUMBER);
	}

	if (content_len >= HTTP_MAX_REQUEST_LEN) {
		return (ISC_R_RANGE);
	}

	size_t consume = httpd->consume + content_len;
	if (consume > httpd->recvlen) {
		/* The request data isn't complete yet. */
		return (ISC_R_NOMORE);
	}

	/* Consume the request's data, which we do not use. */
	httpd->consume = consume;

	switch (httpd->minor_version) {
	case 0:
		/*
		 * RFC 9112 section 9.3 says close takes priority if
		 * keep-alive is also present
		 */
		if ((httpd->flags & CONNECTION_CLOSE) == 0 && keep_alive) {
			httpd->flags |= CONNECTION_KEEP_ALIVE;
		} else {
			httpd->flags |= CONNECTION_CLOSE;
		}
		break;
	case 1:
		if (!host_header) {
			return (ISC_R_RANGE);
		}
		break;
	default:
		return (ISC_R_UNEXPECTED);
	}

	/*
	 * Looks like a a valid request, so now we know we won't have
	 * to process this buffer again. We can NULL-terminate the
	 * URL for the caller's benefit, and set recvlen to 0 so
	 * the next read will overwrite this one instead of appending
	 * to the buffer.
	 */

	return (ISC_R_SUCCESS);
}

static void
httpd_reset(void *arg) {
	isc_httpd_t *httpd = (isc_httpd_t *)arg;
	isc_httpdmgr_t *httpdmgr = NULL;

	REQUIRE(VALID_HTTPD(httpd));

	httpdmgr = httpd->mgr;

	REQUIRE(VALID_HTTPDMGR(httpdmgr));

	LOCK(&httpdmgr->lock);
	ISC_LIST_UNLINK(httpdmgr->running, httpd, link);
	UNLOCK(&httpdmgr->lock);

	httpd->recvbuf[0] = 0;
	httpd->recvlen = 0;
	httpd->consume = 0;
	httpd->method = METHOD_UNKNOWN;
	httpd->flags = 0;

	httpd->minor_version = -1;
	httpd->path = NULL;
	httpd->up = (isc_url_parser_t){ 0 };
	isc_time_set(&httpd->if_modified_since, 0, 0);
}

static void
isc__httpd_sendreq_free(isc_httpd_sendreq_t *req) {
	/* Clean up buffers */

	isc_buffer_free(&req->sendbuffer);

	isc_mem_putanddetach(&req->mctx, req, sizeof(*req));
}

static isc_httpd_sendreq_t *
isc__httpd_sendreq_new(isc_httpd_t *httpd) {
	isc_httpdmgr_t *httpdmgr = httpd->mgr;
	isc_httpd_sendreq_t *req;

	REQUIRE(VALID_HTTPDMGR(httpdmgr));

	req = isc_mem_get(httpdmgr->mctx, sizeof(*req));
	*req = (isc_httpd_sendreq_t){ 0 };

	isc_mem_attach(httpdmgr->mctx, &req->mctx);

	/*
	 * Initialize the buffer for our headers.
	 */
	isc_buffer_allocate(req->mctx, &req->sendbuffer, HTTP_SENDLEN);
	isc_buffer_clear(req->sendbuffer);
	isc_buffer_setautorealloc(req->sendbuffer, true);

	isc_buffer_initnull(&req->bodybuffer);

	return (req);
}

static void
httpd_put(void *arg) {
	isc_httpd_t *httpd = (isc_httpd_t *)arg;
	isc_httpdmgr_t *mgr = NULL;

	REQUIRE(VALID_HTTPD(httpd));

	mgr = httpd->mgr;
	REQUIRE(VALID_HTTPDMGR(mgr));

	httpd->magic = 0;
	httpd->mgr = NULL;

	isc_mem_put(mgr->mctx, httpd, sizeof(*httpd));

	httpdmgr_detach(&mgr);

#if ENABLE_AFL
	if (finishhook != NULL) {
		finishhook();
	}
#endif /* ENABLE_AFL */
}

static void
new_httpd(isc_httpdmgr_t *httpdmgr, isc_nmhandle_t *handle) {
	isc_httpd_t *httpd = NULL;

	REQUIRE(VALID_HTTPDMGR(httpdmgr));

	httpd = isc_nmhandle_getdata(handle);
	if (httpd == NULL) {
		httpd = isc_mem_get(httpdmgr->mctx, sizeof(*httpd));
		*httpd = (isc_httpd_t){ .handle = NULL };
		httpdmgr_attach(httpdmgr, &httpd->mgr);
	}

	if (httpd->handle == NULL) {
		isc_nmhandle_setdata(handle, httpd, httpd_reset, httpd_put);
		httpd->handle = handle;
	} else {
		INSIST(httpd->handle == handle);
	}

	ISC_LINK_INIT(httpd, link);

	httpd->magic = HTTPD_MAGIC;

	LOCK(&httpdmgr->lock);
	ISC_LIST_APPEND(httpdmgr->running, httpd, link);
	UNLOCK(&httpdmgr->lock);

	isc_nmhandle_attach(httpd->handle, &httpd->readhandle);
	isc_nm_read(handle, httpd_request, httpdmgr);
}

static isc_result_t
httpd_newconn(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	isc_httpdmgr_t *httpdmgr = (isc_httpdmgr_t *)arg;
	isc_sockaddr_t peeraddr;

	REQUIRE(VALID_HTTPDMGR(httpdmgr));

	if ((httpdmgr->flags & ISC_HTTPDMGR_SHUTTINGDOWN) != 0) {
		return (ISC_R_CANCELED);
	} else if (result == ISC_R_CANCELED) {
		isc_httpdmgr_shutdown(&httpdmgr);
		return (result);
	} else if (result != ISC_R_SUCCESS) {
		return (result);
	}

	peeraddr = isc_nmhandle_peeraddr(handle);
	if (httpdmgr->client_ok != NULL &&
	    !(httpdmgr->client_ok)(&peeraddr, httpdmgr->cb_arg))
	{
		return (ISC_R_FAILURE);
	}

	new_httpd(httpdmgr, handle);

	return (ISC_R_SUCCESS);
}

static isc_result_t
render_404(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo, void *arg,
	   unsigned int *retcode, const char **retmsg, const char **mimetype,
	   isc_buffer_t *b, isc_httpdfree_t **freecb, void **freecb_args) {
	static char msg[] = "No such URL.\r\n";

	UNUSED(httpd);
	UNUSED(urlinfo);
	UNUSED(arg);

	*retcode = 404;
	*retmsg = "No such URL";
	*mimetype = "text/plain";
	isc_buffer_reinit(b, msg, strlen(msg));
	isc_buffer_add(b, strlen(msg));
	*freecb = NULL;
	*freecb_args = NULL;

	return (ISC_R_SUCCESS);
}

static isc_result_t
render_500(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo, void *arg,
	   unsigned int *retcode, const char **retmsg, const char **mimetype,
	   isc_buffer_t *b, isc_httpdfree_t **freecb, void **freecb_args) {
	static char msg[] = "Internal server failure.\r\n";

	UNUSED(httpd);
	UNUSED(urlinfo);
	UNUSED(arg);

	*retcode = 500;
	*retmsg = "Internal server failure";
	*mimetype = "text/plain";
	isc_buffer_reinit(b, msg, strlen(msg));
	isc_buffer_add(b, strlen(msg));
	*freecb = NULL;
	*freecb_args = NULL;

	return (ISC_R_SUCCESS);
}

#ifdef HAVE_ZLIB
/*%<
 * Tries to compress httpd->bodybuffer to httpd->compbuffer, extending it
 * if necessary.
 *
 * Requires:
 *\li	httpd a valid isc_httpd_t object
 *
 * Returns:
 *\li	#ISC_R_SUCCESS	  -- all is well.
 *\li	#ISC_R_NOMEMORY	  -- not enough memory to compress data
 *\li	#ISC_R_FAILURE	  -- error during compression or compressed
 *			     data would be larger than input data
 */
static isc_result_t
httpd_compress(isc_httpd_sendreq_t *req) {
	z_stream zstr;
	int ret, inputlen;

	/*
	 * We're setting output buffer size to input size so it fails if the
	 * compressed data size would be bigger than the input size.
	 */
	inputlen = isc_buffer_usedlength(&req->bodybuffer);
	if (inputlen == 0) {
		return (ISC_R_FAILURE);
	}

	isc_buffer_allocate(req->mctx, &req->compbuffer, inputlen);
	isc_buffer_clear(req->compbuffer);

	zstr = (z_stream){
		.total_in = inputlen,
		.avail_out = inputlen,
		.avail_in = inputlen,
		.next_in = isc_buffer_base(&req->bodybuffer),
		.next_out = isc_buffer_base(req->compbuffer),
	};

	ret = deflateInit(&zstr, Z_DEFAULT_COMPRESSION);
	if (ret == Z_OK) {
		ret = deflate(&zstr, Z_FINISH);
	}
	deflateEnd(&zstr);
	if (ret == Z_STREAM_END) {
		isc_buffer_add(req->compbuffer, zstr.total_out);
		return (ISC_R_SUCCESS);
	} else {
		isc_buffer_free(&req->compbuffer);
		return (ISC_R_FAILURE);
	}
}
#endif /* ifdef HAVE_ZLIB */

static void
prepare_response(isc_httpdmgr_t *mgr, isc_httpd_t *httpd,
		 isc_httpd_sendreq_t **reqp) {
	isc_httpd_sendreq_t *req = NULL;
	isc_time_t now;
	char datebuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
	const char *path = "/";
	size_t path_len = 1;
	bool is_compressed = false;
	isc_httpdurl_t *url = NULL;
	isc_result_t result;

	REQUIRE(VALID_HTTPD(httpd));
	REQUIRE(reqp != NULL && *reqp == NULL);

	isc_time_now(&now);
	isc_time_formathttptimestamp(&now, datebuf, sizeof(datebuf));

	if (httpd->up.field_set & (1 << ISC_UF_PATH)) {
		path = &httpd->path[httpd->up.field_data[ISC_UF_PATH].off];
		path_len = httpd->up.field_data[ISC_UF_PATH].len;
	}

	LOCK(&mgr->lock);
	url = ISC_LIST_HEAD(mgr->urls);
	while (url != NULL) {
		if (strncmp(path, url->url, path_len) == 0) {
			break;
		}
		url = ISC_LIST_NEXT(url, link);
	}
	UNLOCK(&mgr->lock);

	req = isc__httpd_sendreq_new(httpd);

	if (url == NULL) {
		result = mgr->render_404(httpd, NULL, NULL, &req->retcode,
					 &req->retmsg, &req->mimetype,
					 &req->bodybuffer, &req->freecb,
					 &req->freecb_arg);
	} else {
		result = url->action(httpd, url, url->action_arg, &req->retcode,
				     &req->retmsg, &req->mimetype,
				     &req->bodybuffer, &req->freecb,
				     &req->freecb_arg);
	}
	if (result != ISC_R_SUCCESS) {
		result = mgr->render_500(httpd, url, NULL, &req->retcode,
					 &req->retmsg, &req->mimetype,
					 &req->bodybuffer, &req->freecb,
					 &req->freecb_arg);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

#ifdef HAVE_ZLIB
	if ((httpd->flags & ACCEPT_DEFLATE) != 0) {
		result = httpd_compress(req);
		if (result == ISC_R_SUCCESS) {
			is_compressed = true;
		}
	}
#endif /* ifdef HAVE_ZLIB */

	httpd_response(httpd, req);
	/* RFC 9112 § 9.6: SHOULD send Connection: close in last response */
	if ((httpd->flags & CONNECTION_CLOSE) != 0) {
		httpd_addheader(req, "Connection", "close");
	} else if ((httpd->flags & CONNECTION_KEEP_ALIVE) != 0) {
		httpd_addheader(req, "Connection", "Keep-Alive");
	}
	httpd_addheader(req, "Content-Type", req->mimetype);
	httpd_addheader(req, "Date", datebuf);
	httpd_addheader(req, "Expires", datebuf);

	if (url != NULL && url->isstatic) {
		char loadbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
		isc_time_formathttptimestamp(&url->loadtime, loadbuf,
					     sizeof(loadbuf));
		httpd_addheader(req, "Last-Modified", loadbuf);
		httpd_addheader(req, "Cache-Control: public", NULL);
	} else {
		httpd_addheader(req, "Last-Modified", datebuf);
		httpd_addheader(req, "Pragma: no-cache", NULL);
		httpd_addheader(req, "Cache-Control: no-cache", NULL);
	}

	httpd_addheader(req, "Server: libisc", NULL);

	if (is_compressed) {
		httpd_addheader(req, "Content-Encoding", "deflate");
		httpd_addheaderuint(req, "Content-Length",
				    isc_buffer_usedlength(req->compbuffer));
	} else {
		httpd_addheaderuint(req, "Content-Length",
				    isc_buffer_usedlength(&req->bodybuffer));
	}

	httpd_endheaders(req); /* done */

	/*
	 * Append either the compressed or the non-compressed response body to
	 * the response headers and store the result in httpd->sendbuffer.
	 */
	if (is_compressed) {
		isc_buffer_putmem(req->sendbuffer,
				  isc_buffer_base(req->compbuffer),
				  isc_buffer_usedlength(req->compbuffer));
		isc_buffer_free(&req->compbuffer);
	} else {
		isc_buffer_putmem(req->sendbuffer,
				  isc_buffer_base(&req->bodybuffer),
				  isc_buffer_usedlength(&req->bodybuffer));
	}

	/* Free the bodybuffer */
	if (req->freecb != NULL && isc_buffer_length(&req->bodybuffer) > 0) {
		req->freecb(&req->bodybuffer, req->freecb_arg);
	}

	/* Consume the request from the recv buffer. */
	INSIST(httpd->consume != 0);
	INSIST(httpd->consume <= httpd->recvlen);
	if (httpd->consume < httpd->recvlen) {
		memmove(httpd->recvbuf, httpd->recvbuf + httpd->consume,
			httpd->recvlen - httpd->consume);
	}
	httpd->recvlen -= httpd->consume;
	httpd->consume = 0;

	/*
	 * We don't need to attach to httpd here because it gets only cleaned
	 * when the last handle has been detached
	 */
	req->httpd = httpd;

	*reqp = req;
}

static void
httpd_request(isc_nmhandle_t *handle, isc_result_t eresult,
	      isc_region_t *region, void *arg) {
	isc_result_t result;
	isc_httpdmgr_t *mgr = arg;
	isc_httpd_t *httpd = NULL;
	isc_httpd_sendreq_t *req = NULL;
	isc_region_t r;
	size_t last_len = 0;

	httpd = isc_nmhandle_getdata(handle);

	REQUIRE(VALID_HTTPD(httpd));

	REQUIRE(httpd->handle == handle);

	if (httpd->readhandle == NULL) {
		/* The channel has been already closed, just bail out */
		return;
	}

	if (eresult != ISC_R_SUCCESS) {
		goto close_readhandle;
	}

	REQUIRE(httpd->readhandle == handle);

	isc_nm_pauseread(httpd->readhandle);

	/*
	 * If we are being called from httpd_senddone(), the last HTTP request
	 * was processed successfully, reset the last_len to 0, even if there's
	 * data in the httpd->recvbuf.
	 */
	last_len = (region == NULL) ? 0 : httpd->recvlen;

	/* Store the received data into the recvbuf */
	if (region != NULL) {
		if (httpd->recvlen + region->length > sizeof(httpd->recvbuf)) {
			goto close_readhandle;
		}

		memmove(httpd->recvbuf + httpd->recvlen, region->base,
			region->length);
		httpd->recvlen += region->length;
	}

	result = process_request(httpd, last_len);

	if (result == ISC_R_NOMORE) {
		if (httpd->recvlen > HTTP_MAX_REQUEST_LEN) {
			goto close_readhandle;
		}

		/* Wait for more data, the readhandle is still attached */
		isc_nm_resumeread(httpd->readhandle);
		return;
	}

	/* XXXFANF it would be more polite to reply 400 bad request */
	if (result != ISC_R_SUCCESS) {
		goto close_readhandle;
	}

	prepare_response(mgr, httpd, &req);

	/*
	 * Determine total response size.
	 */
	isc_buffer_usedregion(req->sendbuffer, &r);

	isc_nmhandle_attach(httpd->handle, &req->handle);
	isc_nm_send(httpd->handle, &r, httpd_senddone, req);
	return;

close_readhandle:
	isc_nm_pauseread(httpd->readhandle);
	isc_nmhandle_detach(&httpd->readhandle);
}

void
isc_httpdmgr_shutdown(isc_httpdmgr_t **httpdmgrp) {
	isc_httpdmgr_t *httpdmgr;
	isc_httpd_t *httpd;

	REQUIRE(httpdmgrp != NULL);
	REQUIRE(VALID_HTTPDMGR(*httpdmgrp));

	httpdmgr = *httpdmgrp;
	*httpdmgrp = NULL;

	isc_nm_stoplistening(httpdmgr->sock);

	LOCK(&httpdmgr->lock);
	httpdmgr->flags |= ISC_HTTPDMGR_SHUTTINGDOWN;

	httpd = ISC_LIST_HEAD(httpdmgr->running);
	while (httpd != NULL) {
		isc_nm_cancelread(httpd->readhandle);
		httpd = ISC_LIST_NEXT(httpd, link);
	}
	UNLOCK(&httpdmgr->lock);

	isc_nmsocket_close(&httpdmgr->sock);

	httpdmgr_detach(&httpdmgr);
}

static void
httpd_response(isc_httpd_t *httpd, isc_httpd_sendreq_t *req) {
	isc_result_t result;

	result = isc_buffer_printf(req->sendbuffer, "HTTP/1.%u %03u %s\r\n",
				   httpd->minor_version, req->retcode,
				   req->retmsg);

	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
httpd_addheader(isc_httpd_sendreq_t *req, const char *name, const char *val) {
	isc_result_t result;

	if (val != NULL) {
		result = isc_buffer_printf(req->sendbuffer, "%s: %s\r\n", name,
					   val);
	} else {
		result = isc_buffer_printf(req->sendbuffer, "%s\r\n", name);
	}

	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
httpd_endheaders(isc_httpd_sendreq_t *req) {
	isc_result_t result;

	result = isc_buffer_printf(req->sendbuffer, "\r\n");

	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
httpd_addheaderuint(isc_httpd_sendreq_t *req, const char *name, int val) {
	isc_result_t result;

	result = isc_buffer_printf(req->sendbuffer, "%s: %d\r\n", name, val);

	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
httpd_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	isc_httpd_sendreq_t *req = (isc_httpd_sendreq_t *)arg;
	isc_httpd_t *httpd = req->httpd;

	REQUIRE(VALID_HTTPD(httpd));

	if (httpd->readhandle == NULL) {
		goto detach;
	}

	if (eresult == ISC_R_SUCCESS && (httpd->flags & CONNECTION_CLOSE) == 0)
	{
		/*
		 * Calling httpd_request() with region NULL restarts
		 * reading.
		 */
		httpd_request(handle, ISC_R_SUCCESS, NULL, httpd->mgr);
	} else {
		isc_nm_cancelread(httpd->readhandle);
	}

detach:
	isc_nmhandle_detach(&handle);

	isc__httpd_sendreq_free(req);
}

isc_result_t
isc_httpdmgr_addurl(isc_httpdmgr_t *httpdmgr, const char *url, bool isstatic,
		    isc_httpdaction_t *func, void *arg) {
	isc_httpdurl_t *item;

	REQUIRE(VALID_HTTPDMGR(httpdmgr));

	if (url == NULL) {
		httpdmgr->render_404 = func;
		return (ISC_R_SUCCESS);
	}

	item = isc_mem_get(httpdmgr->mctx, sizeof(isc_httpdurl_t));

	item->url = isc_mem_strdup(httpdmgr->mctx, url);

	item->action = func;
	item->action_arg = arg;
	item->isstatic = isstatic;
	isc_time_now(&item->loadtime);

	ISC_LINK_INIT(item, link);

	LOCK(&httpdmgr->lock);
	ISC_LIST_APPEND(httpdmgr->urls, item, link);
	UNLOCK(&httpdmgr->lock);

	return (ISC_R_SUCCESS);
}

void
isc_httpd_setfinishhook(void (*fn)(void)) {
#if ENABLE_AFL
	finishhook = fn;
#else  /* ENABLE_AFL */
	UNUSED(fn);
#endif /* ENABLE_AFL */
}

bool
isc_httpdurl_isstatic(const isc_httpdurl_t *url) {
	return (url->isstatic);
}

const isc_time_t *
isc_httpdurl_loadtime(const isc_httpdurl_t *url) {
	return (&url->loadtime);
}

const isc_time_t *
isc_httpd_if_modified_since(const isc_httpd_t *httpd) {
	return ((const isc_time_t *)&httpd->if_modified_since);
}
