/*	$NetBSD: request.c,v 1.6.2.2 2024/02/25 15:46:52 martin Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/compress.h>
#include <dns/dispatch.h>
#include <dns/events.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/request.h>
#include <dns/tsig.h>

#define REQUESTMGR_MAGIC      ISC_MAGIC('R', 'q', 'u', 'M')
#define VALID_REQUESTMGR(mgr) ISC_MAGIC_VALID(mgr, REQUESTMGR_MAGIC)

#define REQUEST_MAGIC	       ISC_MAGIC('R', 'q', 'u', '!')
#define VALID_REQUEST(request) ISC_MAGIC_VALID(request, REQUEST_MAGIC)

typedef ISC_LIST(dns_request_t) dns_requestlist_t;

#define DNS_REQUEST_NLOCKS 7

struct dns_requestmgr {
	unsigned int magic;
	isc_refcount_t references;

	isc_mutex_t lock;
	isc_mem_t *mctx;

	/* locked */
	isc_taskmgr_t *taskmgr;
	dns_dispatchmgr_t *dispatchmgr;
	dns_dispatch_t *dispatchv4;
	dns_dispatch_t *dispatchv6;
	atomic_bool exiting;
	isc_eventlist_t whenshutdown;
	unsigned int hash;
	isc_mutex_t locks[DNS_REQUEST_NLOCKS];
	dns_requestlist_t requests;
};

struct dns_request {
	unsigned int magic;
	isc_refcount_t references;

	unsigned int hash;
	isc_mem_t *mctx;
	int32_t flags;
	ISC_LINK(dns_request_t) link;
	isc_buffer_t *query;
	isc_buffer_t *answer;
	dns_requestevent_t *event;
	dns_dispatch_t *dispatch;
	dns_dispentry_t *dispentry;
	dns_requestmgr_t *requestmgr;
	isc_buffer_t *tsig;
	dns_tsigkey_t *tsigkey;
	isc_sockaddr_t destaddr;
	unsigned int timeout;
	unsigned int udpcount;
};

#define DNS_REQUEST_F_CONNECTING 0x0001
#define DNS_REQUEST_F_SENDING	 0x0002
#define DNS_REQUEST_F_CANCELED	 0x0004
#define DNS_REQUEST_F_TCP	 0x0010

#define DNS_REQUEST_CANCELED(r)	  (((r)->flags & DNS_REQUEST_F_CANCELED) != 0)
#define DNS_REQUEST_CONNECTING(r) (((r)->flags & DNS_REQUEST_F_CONNECTING) != 0)
#define DNS_REQUEST_SENDING(r)	  (((r)->flags & DNS_REQUEST_F_SENDING) != 0)

/***
 *** Forward
 ***/

static void
mgr_destroy(dns_requestmgr_t *requestmgr);
static unsigned int
mgr_gethash(dns_requestmgr_t *requestmgr);
static void
send_shutdown_events(dns_requestmgr_t *requestmgr);

static isc_result_t
req_render(dns_message_t *message, isc_buffer_t **buffer, unsigned int options,
	   isc_mem_t *mctx);
static void
req_response(isc_result_t result, isc_region_t *region, void *arg);
static void
req_senddone(isc_result_t eresult, isc_region_t *region, void *arg);
static void
req_sendevent(dns_request_t *request, isc_result_t result);
static void
req_connected(isc_result_t eresult, isc_region_t *region, void *arg);
static void
req_attach(dns_request_t *source, dns_request_t **targetp);
static void
req_detach(dns_request_t **requestp);
static void
req_destroy(dns_request_t *request);
static void
req_log(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);
void
request_cancel(dns_request_t *request);

/***
 *** Public
 ***/

isc_result_t
dns_requestmgr_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		      dns_dispatchmgr_t *dispatchmgr,
		      dns_dispatch_t *dispatchv4, dns_dispatch_t *dispatchv6,
		      dns_requestmgr_t **requestmgrp) {
	dns_requestmgr_t *requestmgr;
	int i;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_create");

	REQUIRE(requestmgrp != NULL && *requestmgrp == NULL);
	REQUIRE(taskmgr != NULL);
	REQUIRE(dispatchmgr != NULL);

	requestmgr = isc_mem_get(mctx, sizeof(*requestmgr));
	*requestmgr = (dns_requestmgr_t){ 0 };

	isc_taskmgr_attach(taskmgr, &requestmgr->taskmgr);
	dns_dispatchmgr_attach(dispatchmgr, &requestmgr->dispatchmgr);
	isc_mutex_init(&requestmgr->lock);

	for (i = 0; i < DNS_REQUEST_NLOCKS; i++) {
		isc_mutex_init(&requestmgr->locks[i]);
	}
	if (dispatchv4 != NULL) {
		dns_dispatch_attach(dispatchv4, &requestmgr->dispatchv4);
	}
	if (dispatchv6 != NULL) {
		dns_dispatch_attach(dispatchv6, &requestmgr->dispatchv6);
	}
	isc_mem_attach(mctx, &requestmgr->mctx);

	isc_refcount_init(&requestmgr->references, 1);

	ISC_LIST_INIT(requestmgr->whenshutdown);
	ISC_LIST_INIT(requestmgr->requests);

	atomic_init(&requestmgr->exiting, false);

	requestmgr->magic = REQUESTMGR_MAGIC;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_create: %p", requestmgr);

	*requestmgrp = requestmgr;
	return (ISC_R_SUCCESS);
}

void
dns_requestmgr_whenshutdown(dns_requestmgr_t *requestmgr, isc_task_t *task,
			    isc_event_t **eventp) {
	isc_task_t *tclone;
	isc_event_t *event;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_whenshutdown");

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(eventp != NULL);

	event = *eventp;
	*eventp = NULL;

	LOCK(&requestmgr->lock);

	if (atomic_load_acquire(&requestmgr->exiting)) {
		/*
		 * We're already shutdown.  Send the event.
		 */
		event->ev_sender = requestmgr;
		isc_task_send(task, &event);
	} else {
		tclone = NULL;
		isc_task_attach(task, &tclone);
		event->ev_sender = tclone;
		ISC_LIST_APPEND(requestmgr->whenshutdown, event, ev_link);
	}
	UNLOCK(&requestmgr->lock);
}

void
dns_requestmgr_shutdown(dns_requestmgr_t *requestmgr) {
	dns_request_t *request;

	REQUIRE(VALID_REQUESTMGR(requestmgr));

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_shutdown: %p", requestmgr);

	if (!atomic_compare_exchange_strong(&requestmgr->exiting,
					    &(bool){ false }, true))
	{
		return;
	}

	LOCK(&requestmgr->lock);
	for (request = ISC_LIST_HEAD(requestmgr->requests); request != NULL;
	     request = ISC_LIST_NEXT(request, link))
	{
		dns_request_cancel(request);
	}

	if (ISC_LIST_EMPTY(requestmgr->requests)) {
		send_shutdown_events(requestmgr);
	}

	UNLOCK(&requestmgr->lock);
}

void
dns_requestmgr_attach(dns_requestmgr_t *source, dns_requestmgr_t **targetp) {
	uint_fast32_t ref;

	REQUIRE(VALID_REQUESTMGR(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	REQUIRE(!atomic_load_acquire(&source->exiting));

	ref = isc_refcount_increment(&source->references);

	req_log(ISC_LOG_DEBUG(3),
		"dns_requestmgr_attach: %p: references = %" PRIuFAST32, source,
		ref + 1);

	*targetp = source;
}

void
dns_requestmgr_detach(dns_requestmgr_t **requestmgrp) {
	dns_requestmgr_t *requestmgr = NULL;
	uint_fast32_t ref;

	REQUIRE(requestmgrp != NULL && VALID_REQUESTMGR(*requestmgrp));

	requestmgr = *requestmgrp;
	*requestmgrp = NULL;

	ref = isc_refcount_decrement(&requestmgr->references);

	req_log(ISC_LOG_DEBUG(3),
		"dns_requestmgr_detach: %p: references = %" PRIuFAST32,
		requestmgr, ref - 1);

	if (ref == 1) {
		INSIST(ISC_LIST_EMPTY(requestmgr->requests));
		mgr_destroy(requestmgr);
	}
}

/* FIXME */
static void
send_shutdown_events(dns_requestmgr_t *requestmgr) {
	isc_event_t *event, *next_event;
	isc_task_t *etask;

	req_log(ISC_LOG_DEBUG(3), "send_shutdown_events: %p", requestmgr);

	/*
	 * Caller must be holding the manager lock.
	 */
	for (event = ISC_LIST_HEAD(requestmgr->whenshutdown); event != NULL;
	     event = next_event)
	{
		next_event = ISC_LIST_NEXT(event, ev_link);
		ISC_LIST_UNLINK(requestmgr->whenshutdown, event, ev_link);
		etask = event->ev_sender;
		event->ev_sender = requestmgr;
		isc_task_sendanddetach(&etask, &event);
	}
}

static void
mgr_destroy(dns_requestmgr_t *requestmgr) {
	int i;

	req_log(ISC_LOG_DEBUG(3), "mgr_destroy");

	isc_refcount_destroy(&requestmgr->references);

	isc_mutex_destroy(&requestmgr->lock);
	for (i = 0; i < DNS_REQUEST_NLOCKS; i++) {
		isc_mutex_destroy(&requestmgr->locks[i]);
	}
	if (requestmgr->dispatchv4 != NULL) {
		dns_dispatch_detach(&requestmgr->dispatchv4);
	}
	if (requestmgr->dispatchv6 != NULL) {
		dns_dispatch_detach(&requestmgr->dispatchv6);
	}
	if (requestmgr->dispatchmgr != NULL) {
		dns_dispatchmgr_detach(&requestmgr->dispatchmgr);
	}
	if (requestmgr->taskmgr != NULL) {
		isc_taskmgr_detach(&requestmgr->taskmgr);
	}
	requestmgr->magic = 0;
	isc_mem_putanddetach(&requestmgr->mctx, requestmgr,
			     sizeof(*requestmgr));
}

static unsigned int
mgr_gethash(dns_requestmgr_t *requestmgr) {
	req_log(ISC_LOG_DEBUG(3), "mgr_gethash");
	/*
	 * Locked by caller.
	 */
	requestmgr->hash++;
	return (requestmgr->hash % DNS_REQUEST_NLOCKS);
}

static void
req_send(dns_request_t *request) {
	isc_region_t r;

	req_log(ISC_LOG_DEBUG(3), "req_send: request %p", request);

	REQUIRE(VALID_REQUEST(request));

	isc_buffer_usedregion(request->query, &r);

	request->flags |= DNS_REQUEST_F_SENDING;

	/* detached in req_senddone() */
	req_attach(request, &(dns_request_t *){ NULL });
	dns_dispatch_send(request->dispentry, &r);
}

static isc_result_t
new_request(isc_mem_t *mctx, dns_request_t **requestp) {
	dns_request_t *request = NULL;

	request = isc_mem_get(mctx, sizeof(*request));
	*request = (dns_request_t){ 0 };
	ISC_LINK_INIT(request, link);

	isc_refcount_init(&request->references, 1);
	isc_mem_attach(mctx, &request->mctx);

	request->magic = REQUEST_MAGIC;
	*requestp = request;
	return (ISC_R_SUCCESS);
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
		return (false);
	}

	isc_netaddr_fromsockaddr(&netaddr, destaddr);
	result = dns_acl_match(&netaddr, NULL, blackhole, NULL, &match, NULL);
	if (result != ISC_R_SUCCESS || match <= 0) {
		return (false);
	}

	isc_netaddr_format(&netaddr, netaddrstr, sizeof(netaddrstr));
	req_log(ISC_LOG_DEBUG(10), "blackholed address %s", netaddrstr);

	return (true);
}

static isc_result_t
tcp_dispatch(bool newtcp, dns_requestmgr_t *requestmgr,
	     const isc_sockaddr_t *srcaddr, const isc_sockaddr_t *destaddr,
	     dns_dispatch_t **dispatchp) {
	isc_result_t result;

	if (!newtcp) {
		result = dns_dispatch_gettcp(requestmgr->dispatchmgr, destaddr,
					     srcaddr, dispatchp);
		if (result == ISC_R_SUCCESS) {
			char peer[ISC_SOCKADDR_FORMATSIZE];

			isc_sockaddr_format(destaddr, peer, sizeof(peer));
			req_log(ISC_LOG_DEBUG(1),
				"attached to TCP connection to %s", peer);
			return (result);
		}
	}

	result = dns_dispatch_createtcp(requestmgr->dispatchmgr, srcaddr,
					destaddr, dispatchp);
	return (result);
}

static isc_result_t
udp_dispatch(dns_requestmgr_t *requestmgr, const isc_sockaddr_t *srcaddr,
	     const isc_sockaddr_t *destaddr, dns_dispatch_t **dispatchp) {
	dns_dispatch_t *disp = NULL;

	if (srcaddr == NULL) {
		switch (isc_sockaddr_pf(destaddr)) {
		case PF_INET:
			disp = requestmgr->dispatchv4;
			break;

		case PF_INET6:
			disp = requestmgr->dispatchv6;
			break;

		default:
			return (ISC_R_NOTIMPLEMENTED);
		}
		if (disp == NULL) {
			return (ISC_R_FAMILYNOSUPPORT);
		}
		dns_dispatch_attach(disp, dispatchp);
		return (ISC_R_SUCCESS);
	}

	return (dns_dispatch_createudp(requestmgr->dispatchmgr, srcaddr,
				       dispatchp));
}

static isc_result_t
get_dispatch(bool tcp, bool newtcp, dns_requestmgr_t *requestmgr,
	     const isc_sockaddr_t *srcaddr, const isc_sockaddr_t *destaddr,
	     dns_dispatch_t **dispatchp) {
	isc_result_t result;

	if (tcp) {
		result = tcp_dispatch(newtcp, requestmgr, srcaddr, destaddr,
				      dispatchp);
	} else {
		result = udp_dispatch(requestmgr, srcaddr, destaddr, dispatchp);
	}
	return (result);
}

isc_result_t
dns_request_createraw(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		      const isc_sockaddr_t *srcaddr,
		      const isc_sockaddr_t *destaddr, unsigned int options,
		      unsigned int timeout, unsigned int udptimeout,
		      unsigned int udpretries, isc_task_t *task,
		      isc_taskaction_t action, void *arg,
		      dns_request_t **requestp) {
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
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);
	REQUIRE(requestp != NULL && *requestp == NULL);
	REQUIRE(timeout > 0);
	REQUIRE(udpretries != UINT_MAX);

	if (srcaddr != NULL) {
		REQUIRE(isc_sockaddr_pf(srcaddr) == isc_sockaddr_pf(destaddr));
	}

	mctx = requestmgr->mctx;

	req_log(ISC_LOG_DEBUG(3), "dns_request_createraw");

	if (atomic_load_acquire(&requestmgr->exiting)) {
		return (ISC_R_SHUTTINGDOWN);
	}

	if (isblackholed(requestmgr->dispatchmgr, destaddr)) {
		return (DNS_R_BLACKHOLED);
	}

	/* detached in dns_request_destroy() */
	result = new_request(mctx, &request);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	request->udpcount = udpretries + 1;

	request->event = (dns_requestevent_t *)isc_event_allocate(
		mctx, task, DNS_EVENT_REQUESTDONE, action, arg,
		sizeof(dns_requestevent_t));
	isc_task_attach(task, &(isc_task_t *){ NULL });
	request->event->ev_sender = task;
	request->event->request = request;
	request->event->result = ISC_R_FAILURE;

	isc_buffer_usedregion(msgbuf, &r);
	if (r.length < DNS_MESSAGE_HEADERLEN || r.length > 65535) {
		result = DNS_R_FORMERR;
		goto cleanup;
	}

	if ((options & DNS_REQUESTOPT_TCP) != 0 || r.length > 512) {
		tcp = true;
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

	isc_buffer_allocate(mctx, &request->query, r.length + (tcp ? 2 : 0));
	result = isc_buffer_copyregion(request->query, &r);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/* detached in req_connected() */
	req_attach(request, &(dns_request_t *){ NULL });

again:

	result = get_dispatch(tcp, newtcp, requestmgr, srcaddr, destaddr,
			      &request->dispatch);
	if (result != ISC_R_SUCCESS) {
		goto detach;
	}

	if ((options & DNS_REQUESTOPT_FIXEDID) != 0) {
		id = (r.base[0] << 8) | r.base[1];
		dispopt |= DNS_DISPATCHOPT_FIXEDID;
	}

	result = dns_dispatch_add(request->dispatch, dispopt, request->timeout,
				  destaddr, req_connected, req_senddone,
				  req_response, request, &id,
				  &request->dispentry);
	if (result != ISC_R_SUCCESS) {
		if ((options & DNS_REQUESTOPT_FIXEDID) != 0 && !newtcp) {
			newtcp = true;
			dns_dispatch_detach(&request->dispatch);
			goto again;
		}

		goto detach;
	}

	/* Add message ID. */
	isc_buffer_usedregion(request->query, &r);
	r.base[0] = (id >> 8) & 0xff;
	r.base[1] = id & 0xff;

	LOCK(&requestmgr->lock);
	dns_requestmgr_attach(requestmgr, &request->requestmgr);
	request->hash = mgr_gethash(requestmgr);
	ISC_LIST_APPEND(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

	request->destaddr = *destaddr;

	request->flags |= DNS_REQUEST_F_CONNECTING;
	if (tcp) {
		request->flags |= DNS_REQUEST_F_TCP;
	}

	result = dns_dispatch_connect(request->dispentry);
	if (result != ISC_R_SUCCESS) {
		goto unlink;
	}

	req_log(ISC_LOG_DEBUG(3), "dns_request_createraw: request %p", request);
	*requestp = request;
	return (ISC_R_SUCCESS);

unlink:
	LOCK(&requestmgr->lock);
	ISC_LIST_UNLINK(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

detach:
	/* connect failed, detach here */
	req_detach(&(dns_request_t *){ request });

cleanup:
	isc_task_detach(&(isc_task_t *){ task });
	/* final detach to shut down request */
	req_detach(&request);
	req_log(ISC_LOG_DEBUG(3), "dns_request_createraw: failed %s",
		isc_result_totext(result));
	return (result);
}

isc_result_t
dns_request_create(dns_requestmgr_t *requestmgr, dns_message_t *message,
		   const isc_sockaddr_t *srcaddr,
		   const isc_sockaddr_t *destaddr, unsigned int options,
		   dns_tsigkey_t *key, unsigned int timeout,
		   unsigned int udptimeout, unsigned int udpretries,
		   isc_task_t *task, isc_taskaction_t action, void *arg,
		   dns_request_t **requestp) {
	dns_request_t *request = NULL;
	isc_result_t result;
	isc_mem_t *mctx = NULL;
	dns_messageid_t id;
	bool tcp = false;
	bool connected = false;

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(message != NULL);
	REQUIRE(destaddr != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);
	REQUIRE(requestp != NULL && *requestp == NULL);
	REQUIRE(timeout > 0);
	REQUIRE(udpretries != UINT_MAX);

	mctx = requestmgr->mctx;

	req_log(ISC_LOG_DEBUG(3), "dns_request_create");

	if (atomic_load_acquire(&requestmgr->exiting)) {
		return (ISC_R_SHUTTINGDOWN);
	}

	if (srcaddr != NULL &&
	    isc_sockaddr_pf(srcaddr) != isc_sockaddr_pf(destaddr))
	{
		return (ISC_R_FAMILYMISMATCH);
	}

	if (isblackholed(requestmgr->dispatchmgr, destaddr)) {
		return (DNS_R_BLACKHOLED);
	}

	/* detached in dns_request_destroy() */
	result = new_request(mctx, &request);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	request->udpcount = udpretries + 1;

	request->event = (dns_requestevent_t *)isc_event_allocate(
		mctx, task, DNS_EVENT_REQUESTDONE, action, arg,
		sizeof(dns_requestevent_t));
	isc_task_attach(task, &(isc_task_t *){ NULL });
	request->event->ev_sender = task;
	request->event->request = request;
	request->event->result = ISC_R_FAILURE;

	if (key != NULL) {
		dns_tsigkey_attach(key, &request->tsigkey);
	}

	result = dns_message_settsigkey(message, request->tsigkey);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if ((options & DNS_REQUESTOPT_TCP) != 0) {
		tcp = true;
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

	/* detached in req_connected() */
	req_attach(request, &(dns_request_t *){ NULL });

again:
	result = get_dispatch(tcp, false, requestmgr, srcaddr, destaddr,
			      &request->dispatch);
	if (result != ISC_R_SUCCESS) {
		goto detach;
	}

	result = dns_dispatch_add(
		request->dispatch, 0, request->timeout, destaddr, req_connected,
		req_senddone, req_response, request, &id, &request->dispentry);
	if (result != ISC_R_SUCCESS) {
		goto detach;
	}

	message->id = id;
	result = req_render(message, &request->query, options, mctx);
	if (result == DNS_R_USETCP && !tcp) {
		/*
		 * Try again using TCP.
		 */
		dns_message_renderreset(message);
		dns_dispatch_done(&request->dispentry);
		dns_dispatch_detach(&request->dispatch);
		options |= DNS_REQUESTOPT_TCP;
		tcp = true;
		goto again;
	}
	if (result != ISC_R_SUCCESS) {
		goto detach;
	}

	result = dns_message_getquerytsig(message, mctx, &request->tsig);
	if (result != ISC_R_SUCCESS) {
		goto detach;
	}

	LOCK(&requestmgr->lock);
	dns_requestmgr_attach(requestmgr, &request->requestmgr);
	request->hash = mgr_gethash(requestmgr);
	ISC_LIST_APPEND(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

	request->destaddr = *destaddr;
	if (tcp && connected) {
		req_send(request);

		/* no need to call req_connected(), detach here */
		req_detach(&(dns_request_t *){ request });
	} else {
		request->flags |= DNS_REQUEST_F_CONNECTING;
		if (tcp) {
			request->flags |= DNS_REQUEST_F_TCP;
		}

		result = dns_dispatch_connect(request->dispentry);
		if (result != ISC_R_SUCCESS) {
			goto unlink;
		}
	}

	req_log(ISC_LOG_DEBUG(3), "dns_request_create: request %p", request);
	*requestp = request;
	return (ISC_R_SUCCESS);

unlink:
	LOCK(&requestmgr->lock);
	ISC_LIST_UNLINK(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

detach:
	/* connect failed, detach here */
	req_detach(&(dns_request_t *){ request });

cleanup:
	isc_task_detach(&(isc_task_t *){ task });
	/* final detach to shut down request */
	req_detach(&request);
	req_log(ISC_LOG_DEBUG(3), "dns_request_create: failed %s",
		isc_result_totext(result));
	return (result);
}

static isc_result_t
req_render(dns_message_t *message, isc_buffer_t **bufferp, unsigned int options,
	   isc_mem_t *mctx) {
	isc_buffer_t *buf1 = NULL;
	isc_buffer_t *buf2 = NULL;
	isc_result_t result;
	isc_region_t r;
	dns_compress_t cctx;
	bool cleanup_cctx = false;

	REQUIRE(bufferp != NULL && *bufferp == NULL);

	req_log(ISC_LOG_DEBUG(3), "request_render");

	/*
	 * Create buffer able to hold largest possible message.
	 */
	isc_buffer_allocate(mctx, &buf1, 65535);

	result = dns_compress_init(&cctx, -1, mctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	cleanup_cctx = true;

	if ((options & DNS_REQUESTOPT_CASE) != 0) {
		dns_compress_setsensitive(&cctx, true);
	}

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

	dns_compress_invalidate(&cctx);
	cleanup_cctx = false;

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
	isc_buffer_free(&buf1);
	*bufferp = buf2;
	return (ISC_R_SUCCESS);

cleanup:
	dns_message_renderreset(message);
	if (buf1 != NULL) {
		isc_buffer_free(&buf1);
	}
	if (buf2 != NULL) {
		isc_buffer_free(&buf2);
	}
	if (cleanup_cctx) {
		dns_compress_invalidate(&cctx);
	}
	return (result);
}

void
request_cancel(dns_request_t *request) {
	if (!DNS_REQUEST_CANCELED(request)) {
		req_log(ISC_LOG_DEBUG(3), "request_cancel: request %p",
			request);

		request->flags |= DNS_REQUEST_F_CANCELED;
		request->flags &= ~DNS_REQUEST_F_CONNECTING;

		if (request->dispentry != NULL) {
			dns_dispatch_done(&request->dispentry);
		}

		dns_dispatch_detach(&request->dispatch);
	}
}

void
dns_request_cancel(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "dns_request_cancel: request %p", request);
	LOCK(&request->requestmgr->locks[request->hash]);
	request_cancel(request);
	req_sendevent(request, ISC_R_CANCELED);
	UNLOCK(&request->requestmgr->locks[request->hash]);
}

isc_result_t
dns_request_getresponse(dns_request_t *request, dns_message_t *message,
			unsigned int options) {
	isc_result_t result;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->answer != NULL);

	req_log(ISC_LOG_DEBUG(3), "dns_request_getresponse: request %p",
		request);

	result = dns_message_setquerytsig(message, request->tsig);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	result = dns_message_settsigkey(message, request->tsigkey);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	result = dns_message_parse(message, request->answer, options);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	if (request->tsigkey != NULL) {
		result = dns_tsig_verify(request->answer, message, NULL, NULL);
	}
	return (result);
}

isc_buffer_t *
dns_request_getanswer(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	return (request->answer);
}

bool
dns_request_usedtcp(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	return ((request->flags & DNS_REQUEST_F_TCP) != 0);
}

void
dns_request_destroy(dns_request_t **requestp) {
	dns_request_t *request;

	REQUIRE(requestp != NULL && VALID_REQUEST(*requestp));

	request = *requestp;
	*requestp = NULL;

	req_log(ISC_LOG_DEBUG(3), "dns_request_destroy: request %p", request);

	LOCK(&request->requestmgr->lock);
	LOCK(&request->requestmgr->locks[request->hash]);
	ISC_LIST_UNLINK(request->requestmgr->requests, request, link);
	UNLOCK(&request->requestmgr->locks[request->hash]);
	UNLOCK(&request->requestmgr->lock);

	/*
	 * These should have been cleaned up before the completion
	 * event was sent.
	 */
	INSIST(request->dispentry == NULL);
	INSIST(request->dispatch == NULL);

	/* final detach to shut down request */
	req_detach(&request);
}

static void
req_connected(isc_result_t eresult, isc_region_t *region, void *arg) {
	dns_request_t *request = (dns_request_t *)arg;

	UNUSED(region);

	req_log(ISC_LOG_DEBUG(3), "req_connected: request %p: %s", request,
		isc_result_totext(eresult));

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(DNS_REQUEST_CONNECTING(request) ||
		DNS_REQUEST_CANCELED(request));

	LOCK(&request->requestmgr->locks[request->hash]);
	request->flags &= ~DNS_REQUEST_F_CONNECTING;

	if (eresult == ISC_R_TIMEDOUT) {
		dns_dispatch_done(&request->dispentry);
		dns_dispatch_detach(&request->dispatch);
		req_sendevent(request, eresult);
	} else if (DNS_REQUEST_CANCELED(request)) {
		req_sendevent(request, ISC_R_CANCELED);
	} else if (eresult == ISC_R_SUCCESS) {
		req_send(request);
	} else {
		request_cancel(request);
		req_sendevent(request, ISC_R_CANCELED);
	}
	UNLOCK(&request->requestmgr->locks[request->hash]);

	/* attached in dns_request_create/_createraw() */
	req_detach(&(dns_request_t *){ request });
}

static void
req_senddone(isc_result_t eresult, isc_region_t *region, void *arg) {
	dns_request_t *request = (dns_request_t *)arg;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(DNS_REQUEST_SENDING(request));

	UNUSED(region);

	req_log(ISC_LOG_DEBUG(3), "req_senddone: request %p", request);

	LOCK(&request->requestmgr->locks[request->hash]);
	request->flags &= ~DNS_REQUEST_F_SENDING;

	if (DNS_REQUEST_CANCELED(request)) {
		if (eresult == ISC_R_TIMEDOUT) {
			req_sendevent(request, eresult);
		} else {
			req_sendevent(request, ISC_R_CANCELED);
		}
	} else if (eresult != ISC_R_SUCCESS) {
		request_cancel(request);
		req_sendevent(request, ISC_R_CANCELED);
	}

	UNLOCK(&request->requestmgr->locks[request->hash]);

	/* attached in req_send() */
	req_detach(&request);
}

static void
req_response(isc_result_t result, isc_region_t *region, void *arg) {
	dns_request_t *request = (dns_request_t *)arg;

	if (result == ISC_R_CANCELED) {
		return;
	}

	req_log(ISC_LOG_DEBUG(3), "req_response: request %p: %s", request,
		isc_result_totext(result));

	REQUIRE(VALID_REQUEST(request));

	if (result == ISC_R_TIMEDOUT) {
		LOCK(&request->requestmgr->locks[request->hash]);
		if (request->udpcount > 1 &&
		    (request->flags & DNS_REQUEST_F_TCP) == 0)
		{
			request->udpcount -= 1;
			dns_dispatch_resume(request->dispentry,
					    request->timeout);
			if (!DNS_REQUEST_SENDING(request)) {
				req_send(request);
			}
			UNLOCK(&request->requestmgr->locks[request->hash]);
			return;
		}

		/* The lock is unlocked below */
		goto done;
	}

	LOCK(&request->requestmgr->locks[request->hash]);

	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	/*
	 * Copy region to request.
	 */
	isc_buffer_allocate(request->mctx, &request->answer, region->length);
	result = isc_buffer_copyregion(request->answer, region);
	if (result != ISC_R_SUCCESS) {
		isc_buffer_free(&request->answer);
	}

done:
	/*
	 * Cleanup.
	 */
	if (request->dispentry != NULL) {
		dns_dispatch_done(&request->dispentry);
	}
	request_cancel(request);

	/*
	 * Send completion event.
	 */
	req_sendevent(request, result);
	UNLOCK(&request->requestmgr->locks[request->hash]);
}

static void
req_sendevent(dns_request_t *request, isc_result_t result) {
	isc_task_t *task = NULL;

	REQUIRE(VALID_REQUEST(request));

	if (request->event == NULL) {
		return;
	}

	req_log(ISC_LOG_DEBUG(3), "req_sendevent: request %p", request);

	/*
	 * Lock held by caller.
	 */
	task = request->event->ev_sender;
	request->event->ev_sender = request;
	request->event->result = result;
	isc_task_sendanddetach(&task, (isc_event_t **)(void *)&request->event);
}

static void
req_attach(dns_request_t *source, dns_request_t **targetp) {
	REQUIRE(VALID_REQUEST(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
req_detach(dns_request_t **requestp) {
	dns_request_t *request = NULL;
	uint_fast32_t ref;

	REQUIRE(requestp != NULL && VALID_REQUEST(*requestp));

	request = *requestp;
	*requestp = NULL;

	ref = isc_refcount_decrement(&request->references);

	if (request->requestmgr != NULL &&
	    atomic_load_acquire(&request->requestmgr->exiting))
	{
		/* We are shutting down and this was last request */
		LOCK(&request->requestmgr->lock);
		if (ISC_LIST_EMPTY(request->requestmgr->requests)) {
			send_shutdown_events(request->requestmgr);
		}
		UNLOCK(&request->requestmgr->lock);
	}

	if (ref == 1) {
		req_destroy(request);
	}
}

static void
req_destroy(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "req_destroy: request %p", request);

	isc_refcount_destroy(&request->references);

	request->magic = 0;
	if (request->query != NULL) {
		isc_buffer_free(&request->query);
	}
	if (request->answer != NULL) {
		isc_buffer_free(&request->answer);
	}
	if (request->event != NULL) {
		isc_event_free((isc_event_t **)(void *)&request->event);
	}
	if (request->dispentry != NULL) {
		dns_dispatch_done(&request->dispentry);
	}
	if (request->dispatch != NULL) {
		dns_dispatch_detach(&request->dispatch);
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

static void
req_log(int level, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_REQUEST,
		       level, fmt, ap);
	va_end(ap);
}
