/*	$NetBSD: request.c,v 1.11 2025/01/26 16:25:24 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/async.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/thread.h>
#include <isc/tls.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/compress.h>
#include <dns/dispatch.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/request.h>
#include <dns/transport.h>
#include <dns/tsig.h>

#define REQUESTMGR_MAGIC      ISC_MAGIC('R', 'q', 'u', 'M')
#define VALID_REQUESTMGR(mgr) ISC_MAGIC_VALID(mgr, REQUESTMGR_MAGIC)

#define REQUEST_MAGIC	       ISC_MAGIC('R', 'q', 'u', '!')
#define VALID_REQUEST(request) ISC_MAGIC_VALID(request, REQUEST_MAGIC)

typedef ISC_LIST(dns_request_t) dns_requestlist_t;

struct dns_requestmgr {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	isc_loopmgr_t *loopmgr;

	atomic_bool shuttingdown;

	dns_dispatchmgr_t *dispatchmgr;
	dns_dispatchset_t *dispatches4;
	dns_dispatchset_t *dispatches6;
	dns_requestlist_t *requests;
};

struct dns_request {
	unsigned int magic;
	isc_refcount_t references;

	isc_mem_t *mctx;
	int32_t flags;

	isc_loop_t *loop;
	unsigned int tid;

	isc_result_t result;
	isc_job_cb cb;
	void *arg;
	ISC_LINK(dns_request_t) link;
	isc_buffer_t *query;
	isc_buffer_t *answer;
	dns_dispatch_t *dispatch;
	dns_dispentry_t *dispentry;
	dns_requestmgr_t *requestmgr;
	isc_buffer_t *tsig;
	dns_tsigkey_t *tsigkey;
	isc_sockaddr_t destaddr;
	unsigned int timeout;
	unsigned int udpcount;
};

#define DNS_REQUEST_F_CONNECTING (1 << 0)
#define DNS_REQUEST_F_SENDING	 (1 << 1)
#define DNS_REQUEST_F_COMPLETE	 (1 << 2)
#define DNS_REQUEST_F_TCP	 (1 << 3)

#define DNS_REQUEST_CONNECTING(r) (((r)->flags & DNS_REQUEST_F_CONNECTING) != 0)
#define DNS_REQUEST_SENDING(r)	  (((r)->flags & DNS_REQUEST_F_SENDING) != 0)
#define DNS_REQUEST_COMPLETE(r)	  (((r)->flags & DNS_REQUEST_F_COMPLETE) != 0)

/***
 *** Forward
 ***/

static isc_result_t
req_render(dns_message_t *message, isc_buffer_t **buffer, unsigned int options,
	   isc_mem_t *mctx);
static void
req_response(isc_result_t result, isc_region_t *region, void *arg);
static void
req_senddone(isc_result_t eresult, isc_region_t *region, void *arg);
static void
req_cleanup(dns_request_t *request);
static void
req_sendevent(dns_request_t *request, isc_result_t result);
static void
req_connected(isc_result_t eresult, isc_region_t *region, void *arg);
static void
req_destroy(dns_request_t *request);
static void
req_log(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

/***
 *** Public
 ***/

isc_result_t
dns_requestmgr_create(isc_mem_t *mctx, isc_loopmgr_t *loopmgr,
		      dns_dispatchmgr_t *dispatchmgr,
		      dns_dispatch_t *dispatchv4, dns_dispatch_t *dispatchv6,
		      dns_requestmgr_t **requestmgrp) {
	REQUIRE(requestmgrp != NULL && *requestmgrp == NULL);
	REQUIRE(dispatchmgr != NULL);

	req_log(ISC_LOG_DEBUG(3), "%s", __func__);

	dns_requestmgr_t *requestmgr = isc_mem_get(mctx, sizeof(*requestmgr));
	*requestmgr = (dns_requestmgr_t){
		.magic = REQUESTMGR_MAGIC,
		.loopmgr = loopmgr,
	};
	isc_mem_attach(mctx, &requestmgr->mctx);

	uint32_t nloops = isc_loopmgr_nloops(requestmgr->loopmgr);
	requestmgr->requests = isc_mem_cget(requestmgr->mctx, nloops,
					    sizeof(requestmgr->requests[0]));
	for (size_t i = 0; i < nloops; i++) {
		ISC_LIST_INIT(requestmgr->requests[i]);

		/* unreferenced in requests_shutdown() */
		isc_loop_ref(isc_loop_get(requestmgr->loopmgr, i));
	}

	dns_dispatchmgr_attach(dispatchmgr, &requestmgr->dispatchmgr);

	if (dispatchv4 != NULL) {
		dns_dispatchset_create(requestmgr->mctx, dispatchv4,
				       &requestmgr->dispatches4,
				       isc_loopmgr_nloops(requestmgr->loopmgr));
	}
	if (dispatchv6 != NULL) {
		dns_dispatchset_create(requestmgr->mctx, dispatchv6,
				       &requestmgr->dispatches6,
				       isc_loopmgr_nloops(requestmgr->loopmgr));
	}

	isc_refcount_init(&requestmgr->references, 1);

	req_log(ISC_LOG_DEBUG(3), "%s: %p", __func__, requestmgr);

	*requestmgrp = requestmgr;
	return ISC_R_SUCCESS;
}

static void
requests_shutdown(void *arg) {
	dns_requestmgr_t *requestmgr = arg;
	dns_request_t *request = NULL, *next = NULL;
	uint32_t tid = isc_tid();

	ISC_LIST_FOREACH_SAFE (requestmgr->requests[tid], request, link, next) {
		req_log(ISC_LOG_DEBUG(3), "%s(%" PRIu32 ": request %p",
			__func__, tid, request);
		if (DNS_REQUEST_COMPLETE(request)) {
			/* The callback has been already scheduled */
			continue;
		}
		req_sendevent(request, ISC_R_SHUTTINGDOWN);
	}

	isc_loop_unref(isc_loop_get(requestmgr->loopmgr, tid));
	dns_requestmgr_detach(&requestmgr);
}

void
dns_requestmgr_shutdown(dns_requestmgr_t *requestmgr) {
	bool first;
	REQUIRE(VALID_REQUESTMGR(requestmgr));

	req_log(ISC_LOG_DEBUG(3), "%s: %p", __func__, requestmgr);

	rcu_read_lock();
	first = atomic_compare_exchange_strong(&requestmgr->shuttingdown,
					       &(bool){ false }, true);
	rcu_read_unlock();

	if (!first) {
		return;
	}

	/*
	 * Wait until all dns_request_create{raw}() are finished, so
	 * there will be no new requests added to the lists.
	 */
	synchronize_rcu();

	uint32_t tid = isc_tid();
	uint32_t nloops = isc_loopmgr_nloops(requestmgr->loopmgr);
	for (size_t i = 0; i < nloops; i++) {
		dns_requestmgr_ref(requestmgr);

		if (i == tid) {
			/* Run the current loop synchronously */
			requests_shutdown(requestmgr);
			continue;
		}

		isc_loop_t *loop = isc_loop_get(requestmgr->loopmgr, i);
		isc_async_run(loop, requests_shutdown, requestmgr);
	}
}

static void
requestmgr_destroy(dns_requestmgr_t *requestmgr) {
	req_log(ISC_LOG_DEBUG(3), "%s", __func__);

	INSIST(atomic_load(&requestmgr->shuttingdown));

	size_t nloops = isc_loopmgr_nloops(requestmgr->loopmgr);
	for (size_t i = 0; i < nloops; i++) {
		INSIST(ISC_LIST_EMPTY(requestmgr->requests[i]));
	}
	isc_mem_cput(requestmgr->mctx, requestmgr->requests, nloops,
		     sizeof(requestmgr->requests[0]));

	if (requestmgr->dispatches4 != NULL) {
		dns_dispatchset_destroy(&requestmgr->dispatches4);
	}
	if (requestmgr->dispatches6 != NULL) {
		dns_dispatchset_destroy(&requestmgr->dispatches6);
	}
	if (requestmgr->dispatchmgr != NULL) {
		dns_dispatchmgr_detach(&requestmgr->dispatchmgr);
	}
	requestmgr->magic = 0;
	isc_mem_putanddetach(&requestmgr->mctx, requestmgr,
			     sizeof(*requestmgr));
}

#if DNS_REQUEST_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_requestmgr, requestmgr_destroy);
#else
ISC_REFCOUNT_IMPL(dns_requestmgr, requestmgr_destroy);
#endif

static void
req_send(dns_request_t *request) {
	isc_region_t r;

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);

	REQUIRE(VALID_REQUEST(request));

	isc_buffer_usedregion(request->query, &r);

	request->flags |= DNS_REQUEST_F_SENDING;

	/* detached in req_senddone() */
	dns_request_ref(request);
	dns_dispatch_send(request->dispentry, &r);
}

static dns_request_t *
new_request(isc_mem_t *mctx, isc_loop_t *loop, isc_job_cb cb, void *arg,
	    bool tcp, unsigned int timeout, unsigned int udptimeout,
	    unsigned int udpretries) {
	dns_request_t *request = isc_mem_get(mctx, sizeof(*request));
	*request = (dns_request_t){
		.magic = REQUEST_MAGIC,
		.loop = loop,
		.tid = isc_tid(),
		.cb = cb,
		.arg = arg,
		.link = ISC_LINK_INITIALIZER,
		.result = ISC_R_FAILURE,
		.udpcount = udpretries + 1,
	};

	isc_refcount_init(&request->references, 1);
	isc_mem_attach(mctx, &request->mctx);

	if (tcp) {
		request->timeout = timeout * 1000;
	} else {
		if (udptimeout == 0) {
			udptimeout = timeout / request->udpcount;
		}
		if (udptimeout == 0) {
			udptimeout = 1;
		}
		request->timeout = udptimeout * 1000;
	}

	return request;
}

static bool
isblackholed(dns_dispatchmgr_t *dispatchmgr, const isc_sockaddr_t *destaddr) {
	dns_acl_t *blackhole;
	isc_netaddr_t netaddr;
	char netaddrstr[ISC_NETADDR_FORMATSIZE];
	int match;
	isc_result_t result;

	blackhole = dns_dispatchmgr_getblackhole(dispatchmgr);
	if (blackhole == NULL) {
		return false;
	}

	isc_netaddr_fromsockaddr(&netaddr, destaddr);
	result = dns_acl_match(&netaddr, NULL, blackhole, NULL, &match, NULL);
	if (result != ISC_R_SUCCESS || match <= 0) {
		return false;
	}

	isc_netaddr_format(&netaddr, netaddrstr, sizeof(netaddrstr));
	req_log(ISC_LOG_DEBUG(10), "blackholed address %s", netaddrstr);

	return true;
}

static isc_result_t
tcp_dispatch(bool newtcp, dns_requestmgr_t *requestmgr,
	     const isc_sockaddr_t *srcaddr, const isc_sockaddr_t *destaddr,
	     dns_transport_t *transport, dns_dispatch_t **dispatchp) {
	isc_result_t result;

	if (!newtcp) {
		result = dns_dispatch_gettcp(requestmgr->dispatchmgr, destaddr,
					     srcaddr, transport, dispatchp);
		if (result == ISC_R_SUCCESS) {
			char peer[ISC_SOCKADDR_FORMATSIZE];

			isc_sockaddr_format(destaddr, peer, sizeof(peer));
			req_log(ISC_LOG_DEBUG(1),
				"attached to TCP connection to %s", peer);
			return result;
		}
	}

	result = dns_dispatch_createtcp(requestmgr->dispatchmgr, srcaddr,
					destaddr, transport, 0, dispatchp);
	return result;
}

static isc_result_t
udp_dispatch(dns_requestmgr_t *requestmgr, const isc_sockaddr_t *srcaddr,
	     const isc_sockaddr_t *destaddr, dns_dispatch_t **dispatchp) {
	dns_dispatch_t *disp = NULL;

	if (srcaddr == NULL) {
		switch (isc_sockaddr_pf(destaddr)) {
		case PF_INET:
			disp = dns_dispatchset_get(requestmgr->dispatches4);
			break;

		case PF_INET6:
			disp = dns_dispatchset_get(requestmgr->dispatches6);
			break;

		default:
			return ISC_R_NOTIMPLEMENTED;
		}
		if (disp == NULL) {
			return ISC_R_FAMILYNOSUPPORT;
		}
		dns_dispatch_attach(disp, dispatchp);
		return ISC_R_SUCCESS;
	}

	return dns_dispatch_createudp(requestmgr->dispatchmgr, srcaddr,
				      dispatchp);
}

static isc_result_t
get_dispatch(bool tcp, bool newtcp, dns_requestmgr_t *requestmgr,
	     const isc_sockaddr_t *srcaddr, const isc_sockaddr_t *destaddr,
	     dns_transport_t *transport, dns_dispatch_t **dispatchp) {
	isc_result_t result;

	if (tcp) {
		result = tcp_dispatch(newtcp, requestmgr, srcaddr, destaddr,
				      transport, dispatchp);
	} else {
		result = udp_dispatch(requestmgr, srcaddr, destaddr, dispatchp);
	}
	return result;
}

isc_result_t
dns_request_createraw(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		      const isc_sockaddr_t *srcaddr,
		      const isc_sockaddr_t *destaddr,
		      dns_transport_t *transport,
		      isc_tlsctx_cache_t *tlsctx_cache, unsigned int options,
		      unsigned int timeout, unsigned int udptimeout,
		      unsigned int udpretries, isc_loop_t *loop, isc_job_cb cb,
		      void *arg, dns_request_t **requestp) {
	dns_request_t *request = NULL;
	isc_result_t result;
	isc_mem_t *mctx = NULL;
	dns_messageid_t id;
	bool tcp = false;
	bool newtcp = false;
	isc_region_t r;
	unsigned int dispopt = 0;

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(msgbuf != NULL);
	REQUIRE(destaddr != NULL);
	REQUIRE(loop != NULL);
	REQUIRE(cb != NULL);
	REQUIRE(requestp != NULL && *requestp == NULL);
	REQUIRE(timeout > 0);
	REQUIRE(udpretries != UINT_MAX);

	if (srcaddr != NULL) {
		REQUIRE(isc_sockaddr_pf(srcaddr) == isc_sockaddr_pf(destaddr));
	}

	mctx = requestmgr->mctx;

	req_log(ISC_LOG_DEBUG(3), "%s", __func__);

	rcu_read_lock();

	if (atomic_load_acquire(&requestmgr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
		goto done;
	}

	if (isblackholed(requestmgr->dispatchmgr, destaddr)) {
		result = DNS_R_BLACKHOLED;
		goto done;
	}

	isc_buffer_usedregion(msgbuf, &r);
	if (r.length < DNS_MESSAGE_HEADERLEN || r.length > 65535) {
		result = DNS_R_FORMERR;
		goto done;
	}

	if ((options & DNS_REQUESTOPT_TCP) != 0 || r.length > 512) {
		tcp = true;
	}

	request = new_request(mctx, loop, cb, arg, tcp, timeout, udptimeout,
			      udpretries);

	isc_buffer_allocate(mctx, &request->query, r.length + (tcp ? 2 : 0));
	result = isc_buffer_copyregion(request->query, &r);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

again:
	result = get_dispatch(tcp, newtcp, requestmgr, srcaddr, destaddr,
			      transport, &request->dispatch);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if ((options & DNS_REQUESTOPT_FIXEDID) != 0) {
		id = (r.base[0] << 8) | r.base[1];
		dispopt |= DNS_DISPATCHOPT_FIXEDID;
	}

	result = dns_dispatch_add(
		request->dispatch, loop, dispopt, request->timeout, destaddr,
		transport, tlsctx_cache, req_connected, req_senddone,
		req_response, request, &id, &request->dispentry);
	if (result != ISC_R_SUCCESS) {
		if ((options & DNS_REQUESTOPT_FIXEDID) != 0 && !newtcp) {
			dns_dispatch_detach(&request->dispatch);
			newtcp = true;
			goto again;
		}

		goto cleanup;
	}

	/* Add message ID. */
	isc_buffer_usedregion(request->query, &r);
	r.base[0] = (id >> 8) & 0xff;
	r.base[1] = id & 0xff;

	request->destaddr = *destaddr;
	request->flags |= DNS_REQUEST_F_CONNECTING;
	if (tcp) {
		request->flags |= DNS_REQUEST_F_TCP;
	}

	dns_requestmgr_attach(requestmgr, &request->requestmgr);
	ISC_LIST_APPEND(requestmgr->requests[request->tid], request, link);

	dns_request_ref(request); /* detached in req_connected() */
	result = dns_dispatch_connect(request->dispentry);
	if (result != ISC_R_SUCCESS) {
		dns_request_unref(request);
		goto cleanup;
	}

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);
	*requestp = request;

cleanup:
	if (result != ISC_R_SUCCESS) {
		req_cleanup(request);
		dns_request_detach(&request);
		req_log(ISC_LOG_DEBUG(3), "%s: failed %s", __func__,
			isc_result_totext(result));
	}

done:
	rcu_read_unlock();
	return result;
}

isc_result_t
dns_request_create(dns_requestmgr_t *requestmgr, dns_message_t *message,
		   const isc_sockaddr_t *srcaddr,
		   const isc_sockaddr_t *destaddr, dns_transport_t *transport,
		   isc_tlsctx_cache_t *tlsctx_cache, unsigned int options,
		   dns_tsigkey_t *key, unsigned int timeout,
		   unsigned int udptimeout, unsigned int udpretries,
		   isc_loop_t *loop, isc_job_cb cb, void *arg,
		   dns_request_t **requestp) {
	dns_request_t *request = NULL;
	isc_result_t result;
	isc_mem_t *mctx = NULL;
	dns_messageid_t id;
	bool tcp = false;

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(message != NULL);
	REQUIRE(destaddr != NULL);
	REQUIRE(loop != NULL);
	REQUIRE(cb != NULL);
	REQUIRE(requestp != NULL && *requestp == NULL);
	REQUIRE(timeout > 0);
	REQUIRE(udpretries != UINT_MAX);

	if (srcaddr != NULL &&
	    isc_sockaddr_pf(srcaddr) != isc_sockaddr_pf(destaddr))
	{
		return ISC_R_FAMILYMISMATCH;
	}

	mctx = requestmgr->mctx;

	req_log(ISC_LOG_DEBUG(3), "%s", __func__);

	rcu_read_lock();

	if (atomic_load_acquire(&requestmgr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
		goto done;
	}

	if (isblackholed(requestmgr->dispatchmgr, destaddr)) {
		result = DNS_R_BLACKHOLED;
		goto done;
	}

	if ((options & DNS_REQUESTOPT_TCP) != 0) {
		tcp = true;
	}

	request = new_request(mctx, loop, cb, arg, tcp, timeout, udptimeout,
			      udpretries);

	if (key != NULL) {
		dns_tsigkey_attach(key, &request->tsigkey);
	}

	result = dns_message_settsigkey(message, request->tsigkey);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

again:
	result = get_dispatch(tcp, false, requestmgr, srcaddr, destaddr,
			      transport, &request->dispatch);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_dispatch_add(request->dispatch, loop, 0, request->timeout,
				  destaddr, transport, tlsctx_cache,
				  req_connected, req_senddone, req_response,
				  request, &id, &request->dispentry);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	message->id = id;
	result = req_render(message, &request->query, options, mctx);
	if (result == DNS_R_USETCP && !tcp) {
		/* Try again using TCP. */
		dns_message_renderreset(message);
		dns_dispatch_done(&request->dispentry);
		dns_dispatch_detach(&request->dispatch);
		options |= DNS_REQUESTOPT_TCP;
		tcp = true;
		goto again;
	} else if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_message_getquerytsig(message, mctx, &request->tsig);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	request->destaddr = *destaddr;
	request->flags |= DNS_REQUEST_F_CONNECTING;
	if (tcp) {
		request->flags |= DNS_REQUEST_F_TCP;
	}

	dns_requestmgr_attach(requestmgr, &request->requestmgr);
	ISC_LIST_APPEND(requestmgr->requests[request->tid], request, link);

	dns_request_ref(request); /* detached in req_connected() */
	result = dns_dispatch_connect(request->dispentry);
	if (result != ISC_R_SUCCESS) {
		dns_request_unref(request);
		goto cleanup;
	}

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);
	*requestp = request;

cleanup:
	if (result != ISC_R_SUCCESS) {
		req_cleanup(request);
		dns_request_detach(&request);
		req_log(ISC_LOG_DEBUG(3), "%s: failed %s", __func__,
			isc_result_totext(result));
	}
done:
	rcu_read_unlock();

	return result;
}

static isc_result_t
req_render(dns_message_t *message, isc_buffer_t **bufferp, unsigned int options,
	   isc_mem_t *mctx) {
	isc_buffer_t *buf1 = NULL;
	isc_buffer_t *buf2 = NULL;
	isc_result_t result;
	isc_region_t r;
	dns_compress_t cctx;
	unsigned int compflags;

	REQUIRE(bufferp != NULL && *bufferp == NULL);

	req_log(ISC_LOG_DEBUG(3), "%s", __func__);

	/*
	 * Create buffer able to hold largest possible message.
	 */
	isc_buffer_allocate(mctx, &buf1, 65535);

	compflags = 0;
	if ((options & DNS_REQUESTOPT_LARGE) != 0) {
		compflags |= DNS_COMPRESS_LARGE;
	}
	if ((options & DNS_REQUESTOPT_CASE) != 0) {
		compflags |= DNS_COMPRESS_CASE;
	}
	dns_compress_init(&cctx, mctx, compflags);

	/*
	 * Render message.
	 */
	result = dns_message_renderbegin(message, &cctx, buf1);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_rendersection(message, DNS_SECTION_QUESTION, 0);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_rendersection(message, DNS_SECTION_ANSWER, 0);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_rendersection(message, DNS_SECTION_AUTHORITY, 0);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_rendersection(message, DNS_SECTION_ADDITIONAL, 0);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_renderend(message);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * Copy rendered message to exact sized buffer.
	 */
	isc_buffer_usedregion(buf1, &r);
	if ((options & DNS_REQUESTOPT_TCP) == 0 && r.length > 512) {
		result = DNS_R_USETCP;
		goto cleanup;
	}
	isc_buffer_allocate(mctx, &buf2, r.length);
	result = isc_buffer_copyregion(buf2, &r);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * Cleanup and return.
	 */
	dns_compress_invalidate(&cctx);
	isc_buffer_free(&buf1);
	*bufferp = buf2;
	return ISC_R_SUCCESS;

cleanup:
	dns_message_renderreset(message);
	dns_compress_invalidate(&cctx);
	if (buf1 != NULL) {
		isc_buffer_free(&buf1);
	}
	if (buf2 != NULL) {
		isc_buffer_free(&buf2);
	}
	return result;
}

static void
request_cancel(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());

	if (DNS_REQUEST_COMPLETE(request)) {
		/* The request callback was already called */
		return;
	}

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);
	req_sendevent(request, ISC_R_CANCELED); /* call asynchronously */
}

static void
req_cancel_cb(void *arg) {
	dns_request_t *request = arg;

	request_cancel(request);
	dns_request_unref(request);
}

void
dns_request_cancel(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	if (request->tid == isc_tid()) {
		request_cancel(request);
	} else {
		dns_request_ref(request);
		isc_async_run(request->loop, req_cancel_cb, request);
	}
}

isc_result_t
dns_request_getresponse(dns_request_t *request, dns_message_t *message,
			unsigned int options) {
	isc_result_t result;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());
	REQUIRE(request->answer != NULL);

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);

	dns_message_setquerytsig(message, request->tsig);
	result = dns_message_settsigkey(message, request->tsigkey);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	result = dns_message_parse(message, request->answer, options);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	if (request->tsigkey != NULL) {
		result = dns_tsig_verify(request->answer, message, NULL, NULL);
	}
	return result;
}

isc_buffer_t *
dns_request_getanswer(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());

	return request->answer;
}

bool
dns_request_usedtcp(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());

	return (request->flags & DNS_REQUEST_F_TCP) != 0;
}

void
dns_request_destroy(dns_request_t **requestp) {
	REQUIRE(requestp != NULL && VALID_REQUEST(*requestp));

	dns_request_t *request = *requestp;
	*requestp = NULL;

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);

	if (DNS_REQUEST_COMPLETE(request)) {
		dns_request_cancel(request);
	}

	/* final detach to shut down request */
	dns_request_detach(&request);
}

static void
req_connected(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
	      void *arg) {
	dns_request_t *request = (dns_request_t *)arg;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());
	REQUIRE(DNS_REQUEST_CONNECTING(request));

	req_log(ISC_LOG_DEBUG(3), "%s: request %p: %s", __func__, request,
		isc_result_totext(eresult));

	request->flags &= ~DNS_REQUEST_F_CONNECTING;

	if (DNS_REQUEST_COMPLETE(request)) {
		/* The request callback was already called */
		goto detach;
	}

	if (eresult == ISC_R_SUCCESS) {
		req_send(request);
	} else {
		req_sendevent(request, eresult);
	}

detach:
	/* attached in dns_request_create/_createraw() */
	dns_request_unref(request);
}

static void
req_senddone(isc_result_t eresult, isc_region_t *region ISC_ATTR_UNUSED,
	     void *arg) {
	dns_request_t *request = (dns_request_t *)arg;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());
	REQUIRE(DNS_REQUEST_SENDING(request));

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);

	request->flags &= ~DNS_REQUEST_F_SENDING;

	if (DNS_REQUEST_COMPLETE(request)) {
		/* The request callback was already called */
		goto detach;
	}

	if (eresult != ISC_R_SUCCESS) {
		req_sendevent(request, eresult);
	}

detach:
	/* attached in req_send() */
	dns_request_unref(request);
}

static void
req_response(isc_result_t eresult, isc_region_t *region, void *arg) {
	dns_request_t *request = (dns_request_t *)arg;

	if (eresult == ISC_R_CANCELED) {
		return;
	}

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());

	req_log(ISC_LOG_DEBUG(3), "%s: request %p: %s", __func__, request,
		isc_result_totext(eresult));

	if (DNS_REQUEST_COMPLETE(request)) {
		/* The request callback was already called */
		return;
	}

	switch (eresult) {
	case ISC_R_TIMEDOUT:
		if (request->udpcount > 1 && !dns_request_usedtcp(request)) {
			request->udpcount -= 1;
			dns_dispatch_resume(request->dispentry,
					    request->timeout);
			if (!DNS_REQUEST_SENDING(request)) {
				req_send(request);
			}
			return;
		}
		break;
	case ISC_R_SUCCESS:
		/* Copy region to request. */
		isc_buffer_allocate(request->mctx, &request->answer,
				    region->length);
		eresult = isc_buffer_copyregion(request->answer, region);
		if (eresult != ISC_R_SUCCESS) {
			isc_buffer_free(&request->answer);
		}
		break;
	default:
		break;
	}

	req_sendevent(request, eresult);
}

static void
req_sendevent_cb(void *arg) {
	dns_request_t *request = arg;

	request->cb(request);
	dns_request_unref(request);
}

static void
req_cleanup(dns_request_t *request) {
	if (ISC_LINK_LINKED(request, link)) {
		ISC_LIST_UNLINK(request->requestmgr->requests[request->tid],
				request, link);
	}
	if (request->dispentry != NULL) {
		dns_dispatch_done(&request->dispentry);
	}
	if (request->dispatch != NULL) {
		dns_dispatch_detach(&request->dispatch);
	}
}

static void
req_sendevent(dns_request_t *request, isc_result_t result) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());
	REQUIRE(!DNS_REQUEST_COMPLETE(request));

	request->flags |= DNS_REQUEST_F_COMPLETE;

	req_cleanup(request);

	req_log(ISC_LOG_DEBUG(3), "%s: request %p: %s", __func__, request,
		isc_result_totext(result));

	request->result = result;

	/*
	 * Do not call request->cb directly as it introduces a dead lock
	 * between dns_zonemgr_shutdown and sendtoprimary in lib/dns/zone.c
	 * zone->lock.
	 */
	dns_request_ref(request);
	isc_async_run(request->loop, req_sendevent_cb, request);
}

static void
req_destroy(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());
	REQUIRE(!ISC_LINK_LINKED(request, link));

	req_log(ISC_LOG_DEBUG(3), "%s: request %p", __func__, request);

	/*
	 * These should have been cleaned up before the
	 * completion event was sent.
	 */
	INSIST(!ISC_LINK_LINKED(request, link));
	INSIST(request->dispentry == NULL);
	INSIST(request->dispatch == NULL);

	request->magic = 0;
	if (request->query != NULL) {
		isc_buffer_free(&request->query);
	}
	if (request->answer != NULL) {
		isc_buffer_free(&request->answer);
	}
	if (request->tsig != NULL) {
		isc_buffer_free(&request->tsig);
	}
	if (request->tsigkey != NULL) {
		dns_tsigkey_detach(&request->tsigkey);
	}
	if (request->requestmgr != NULL) {
		dns_requestmgr_detach(&request->requestmgr);
	}
	isc_mem_putanddetach(&request->mctx, request, sizeof(*request));
}

void *
dns_request_getarg(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());

	return request->arg;
}

isc_result_t
dns_request_getresult(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->tid == isc_tid());

	return request->result;
}

#if DNS_REQUEST_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_request, req_destroy);
#else
ISC_REFCOUNT_IMPL(dns_request, req_destroy);
#endif

static void
req_log(int level, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_REQUEST,
		       level, fmt, ap);
	va_end(ap);
}
