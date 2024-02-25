/*	$NetBSD: dispatch.c,v 1.8.2.2 2024/02/25 15:46:48 martin Exp $	*/

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
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/atomic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/netmgr.h>
#include <isc/portset.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/dispatch.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/stats.h>
#include <dns/types.h>

typedef ISC_LIST(dns_dispentry_t) dns_displist_t;

typedef struct dns_qid {
	unsigned int magic;
	isc_mutex_t lock;
	unsigned int qid_nbuckets;  /*%< hash table size */
	unsigned int qid_increment; /*%< id increment on collision */
	dns_displist_t *qid_table;  /*%< the table itself */
} dns_qid_t;

struct dns_dispatchmgr {
	/* Unlocked. */
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	dns_acl_t *blackhole;
	isc_stats_t *stats;
	isc_nm_t *nm;

	/* Locked by "lock". */
	isc_mutex_t lock;
	ISC_LIST(dns_dispatch_t) list;

	dns_qid_t *qid;

	in_port_t *v4ports;    /*%< available ports for IPv4 */
	unsigned int nv4ports; /*%< # of available ports for IPv4 */
	in_port_t *v6ports;    /*%< available ports for IPv4 */
	unsigned int nv6ports; /*%< # of available ports for IPv4 */
};

typedef enum {
	DNS_DISPATCHSTATE_NONE = 0UL,
	DNS_DISPATCHSTATE_CONNECTING,
	DNS_DISPATCHSTATE_CONNECTED,
	DNS_DISPATCHSTATE_CANCELED,
} dns_dispatchstate_t;

struct dns_dispentry {
	unsigned int magic;
	isc_refcount_t references;
	dns_dispatch_t *disp;
	isc_nmhandle_t *handle; /*%< netmgr handle for UDP connection */
	dns_dispatchstate_t state;
	unsigned int bucket;
	unsigned int retries;
	unsigned int timeout;
	isc_time_t start;
	isc_sockaddr_t local;
	isc_sockaddr_t peer;
	in_port_t port;
	dns_messageid_t id;
	dispatch_cb_t connected;
	dispatch_cb_t sent;
	dispatch_cb_t response;
	void *arg;
	bool reading;
	isc_result_t result;
	ISC_LINK(dns_dispentry_t) link;
	ISC_LINK(dns_dispentry_t) alink;
	ISC_LINK(dns_dispentry_t) plink;
	ISC_LINK(dns_dispentry_t) rlink;
};

struct dns_dispatch {
	/* Unlocked. */
	unsigned int magic; /*%< magic */
	int tid;
	dns_dispatchmgr_t *mgr; /*%< dispatch manager */
	isc_nmhandle_t *handle; /*%< netmgr handle for TCP connection */
	isc_sockaddr_t local;	/*%< local address */
	in_port_t localport;	/*%< local UDP port */
	isc_sockaddr_t peer;	/*%< peer address (TCP) */

	/*% Locked by mgr->lock. */
	ISC_LINK(dns_dispatch_t) link;

	/* Locked by "lock". */
	isc_mutex_t lock; /*%< locks all below */
	isc_socktype_t socktype;
	dns_dispatchstate_t state;
	isc_refcount_t references;

	bool reading;

	dns_displist_t pending;
	dns_displist_t active;

	unsigned int requests; /*%< how many requests we have */

	unsigned int timedout;
};

#define QID_MAGIC    ISC_MAGIC('Q', 'i', 'd', ' ')
#define VALID_QID(e) ISC_MAGIC_VALID((e), QID_MAGIC)

#define RESPONSE_MAGIC	  ISC_MAGIC('D', 'r', 's', 'p')
#define VALID_RESPONSE(e) ISC_MAGIC_VALID((e), RESPONSE_MAGIC)

#define DISPSOCK_MAGIC	  ISC_MAGIC('D', 's', 'o', 'c')
#define VALID_DISPSOCK(e) ISC_MAGIC_VALID((e), DISPSOCK_MAGIC)

#define DISPATCH_MAGIC	  ISC_MAGIC('D', 'i', 's', 'p')
#define VALID_DISPATCH(e) ISC_MAGIC_VALID((e), DISPATCH_MAGIC)

#define DNS_DISPATCHMGR_MAGIC ISC_MAGIC('D', 'M', 'g', 'r')
#define VALID_DISPATCHMGR(e)  ISC_MAGIC_VALID((e), DNS_DISPATCHMGR_MAGIC)

/*%
 * Number of buckets in the QID hash table, and the value to
 * increment the QID by when attempting to avoid collisions.
 * The number of buckets should be prime, and the increment
 * should be the next higher prime number.
 */
#ifndef DNS_QID_BUCKETS
#define DNS_QID_BUCKETS 16411
#endif /* ifndef DNS_QID_BUCKETS */
#ifndef DNS_QID_INCREMENT
#define DNS_QID_INCREMENT 16433
#endif /* ifndef DNS_QID_INCREMENT */

#if DNS_DISPATCH_TRACE
#define dns_dispentry_ref(ptr) \
	dns_dispentry__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_dispentry_unref(ptr) \
	dns_dispentry__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_dispentry_attach(ptr, ptrp) \
	dns_dispentry__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_dispentry_detach(ptrp) \
	dns_dispentry__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_dispentry);
#else
ISC_REFCOUNT_DECL(dns_dispentry);
#endif

/*
 * Statics.
 */
static void
dispatchmgr_destroy(dns_dispatchmgr_t *mgr);

static dns_dispentry_t *
entry_search(dns_qid_t *, const isc_sockaddr_t *, dns_messageid_t, in_port_t,
	     unsigned int);
static void
udp_recv(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	 void *arg);
static void
tcp_recv(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	 void *arg);
static void
tcp_recv_done(dns_dispentry_t *resp, isc_result_t eresult,
	      isc_region_t *region);
static uint32_t
dns_hash(dns_qid_t *, const isc_sockaddr_t *, dns_messageid_t, in_port_t);
static void
dispentry_cancel(dns_dispentry_t *resp, isc_result_t result);
static isc_result_t
dispatch_createudp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		   dns_dispatch_t **dispp);
static void
qid_allocate(dns_dispatchmgr_t *mgr, dns_qid_t **qidp);
static void
qid_destroy(isc_mem_t *mctx, dns_qid_t **qidp);
static void
udp_startrecv(isc_nmhandle_t *handle, dns_dispentry_t *resp);
static void
udp_dispatch_connect(dns_dispatch_t *disp, dns_dispentry_t *resp);
static void
tcp_startrecv(isc_nmhandle_t *handle, dns_dispatch_t *disp,
	      dns_dispentry_t *resp);
static void
tcp_dispatch_getnext(dns_dispatch_t *disp, dns_dispentry_t *resp,
		     int32_t timeout);
static void
udp_dispatch_getnext(dns_dispentry_t *resp, int32_t timeout);

#define LVL(x) ISC_LOG_DEBUG(x)

static const char *
socktype2str(dns_dispentry_t *resp) {
	dns_dispatch_t *disp = resp->disp;

	switch (disp->socktype) {
	case isc_socktype_udp:
		return ("UDP");
	case isc_socktype_tcp:
		return ("TCP");
	default:
		return ("<unexpected>");
	}
}

static const char *
state2str(dns_dispatchstate_t state) {
	switch (state) {
	case DNS_DISPATCHSTATE_NONE:
		return ("none");
	case DNS_DISPATCHSTATE_CONNECTING:
		return ("connecting");
	case DNS_DISPATCHSTATE_CONNECTED:
		return ("connected");
	case DNS_DISPATCHSTATE_CANCELED:
		return ("canceled");
	default:
		return ("<unexpected>");
	}
}

static void
mgr_log(dns_dispatchmgr_t *mgr, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

static void
mgr_log(dns_dispatchmgr_t *mgr, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DISPATCH,
		      DNS_LOGMODULE_DISPATCH, level, "dispatchmgr %p: %s", mgr,
		      msgbuf);
}

static void
inc_stats(dns_dispatchmgr_t *mgr, isc_statscounter_t counter) {
	if (mgr->stats != NULL) {
		isc_stats_increment(mgr->stats, counter);
	}
}

static void
dec_stats(dns_dispatchmgr_t *mgr, isc_statscounter_t counter) {
	if (mgr->stats != NULL) {
		isc_stats_decrement(mgr->stats, counter);
	}
}

static void
dispatch_log(dns_dispatch_t *disp, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

static void
dispatch_log(dns_dispatch_t *disp, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;
	int r;

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	r = vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	if (r < 0) {
		msgbuf[0] = '\0';
	} else if ((unsigned int)r >= sizeof(msgbuf)) {
		/* Truncated */
		msgbuf[sizeof(msgbuf) - 1] = '\0';
	}
	va_end(ap);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DISPATCH,
		      DNS_LOGMODULE_DISPATCH, level, "dispatch %p: %s", disp,
		      msgbuf);
}

static void
dispentry_log(dns_dispentry_t *resp, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

static void
dispentry_log(dns_dispentry_t *resp, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;
	int r;

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	r = vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	if (r < 0) {
		msgbuf[0] = '\0';
	} else if ((unsigned int)r >= sizeof(msgbuf)) {
		/* Truncated */
		msgbuf[sizeof(msgbuf) - 1] = '\0';
	}
	va_end(ap);

	dispatch_log(resp->disp, level, "%s response %p: %s",
		     socktype2str(resp), resp, msgbuf);
}

/*
 * Return a hash of the destination and message id.
 */
static uint32_t
dns_hash(dns_qid_t *qid, const isc_sockaddr_t *dest, dns_messageid_t id,
	 in_port_t port) {
	uint32_t ret;

	ret = isc_sockaddr_hash(dest, true);
	ret ^= ((uint32_t)id << 16) | port;
	ret %= qid->qid_nbuckets;

	INSIST(ret < qid->qid_nbuckets);

	return (ret);
}

/*%
 * Choose a random port number for a dispatch entry.
 * The caller must hold the disp->lock
 */
static isc_result_t
setup_socket(dns_dispatch_t *disp, dns_dispentry_t *resp,
	     const isc_sockaddr_t *dest, in_port_t *portp) {
	dns_dispatchmgr_t *mgr = disp->mgr;
	unsigned int nports;
	in_port_t *ports = NULL;
	in_port_t port = *portp;

	if (resp->retries++ > 5) {
		return (ISC_R_FAILURE);
	}

	if (isc_sockaddr_pf(&disp->local) == AF_INET) {
		nports = mgr->nv4ports;
		ports = mgr->v4ports;
	} else {
		nports = mgr->nv6ports;
		ports = mgr->v6ports;
	}
	if (nports == 0) {
		return (ISC_R_ADDRNOTAVAIL);
	}

	resp->local = disp->local;
	resp->peer = *dest;

	if (port == 0) {
		port = ports[isc_random_uniform(nports)];
		isc_sockaddr_setport(&resp->local, port);
		*portp = port;
	}
	resp->port = port;

	return (ISC_R_SUCCESS);
}

/*
 * Find an entry for query ID 'id', socket address 'dest', and port number
 * 'port'.
 * Return NULL if no such entry exists.
 */
static dns_dispentry_t *
entry_search(dns_qid_t *qid, const isc_sockaddr_t *dest, dns_messageid_t id,
	     in_port_t port, unsigned int bucket) {
	dns_dispentry_t *res = NULL;

	REQUIRE(VALID_QID(qid));
	REQUIRE(bucket < qid->qid_nbuckets);

	res = ISC_LIST_HEAD(qid->qid_table[bucket]);

	while (res != NULL) {
		if (res->id == id && isc_sockaddr_equal(dest, &res->peer) &&
		    res->port == port)
		{
			return (res);
		}
		res = ISC_LIST_NEXT(res, link);
	}

	return (NULL);
}

static void
dispentry_destroy(dns_dispentry_t *resp) {
	dns_dispatch_t *disp = resp->disp;

	/*
	 * We need to call this from here in case there's an external event that
	 * shuts down our dispatch (like ISC_R_SHUTTINGDOWN).
	 */
	dispentry_cancel(resp, ISC_R_CANCELED);

	LOCK(&disp->lock);
	INSIST(disp->requests > 0);
	disp->requests--;
	UNLOCK(&disp->lock);

	isc_refcount_destroy(&resp->references);

	resp->magic = 0;

	INSIST(!ISC_LINK_LINKED(resp, link));
	INSIST(!ISC_LINK_LINKED(resp, plink));
	INSIST(!ISC_LINK_LINKED(resp, alink));
	INSIST(!ISC_LINK_LINKED(resp, rlink));

	dispentry_log(resp, LVL(90), "destroying");

	if (resp->handle != NULL) {
		dispentry_log(resp, LVL(90), "detaching handle %p from %p",
			      resp->handle, &resp->handle);
		isc_nmhandle_detach(&resp->handle);
	}

	isc_mem_put(disp->mgr->mctx, resp, sizeof(*resp));

	dns_dispatch_detach(&disp); /* DISPATCH001 */
}

#if DNS_DISPATCH_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_dispentry, dispentry_destroy);
#else
ISC_REFCOUNT_IMPL(dns_dispentry, dispentry_destroy);
#endif

/*
 * How long in milliseconds has it been since this dispentry
 * started reading?
 */
static unsigned int
dispentry_runtime(dns_dispentry_t *resp, const isc_time_t *now) {
	if (isc_time_isepoch(&resp->start)) {
		return (0);
	}

	return (isc_time_microdiff(now, &resp->start) / 1000);
}

/*
 * General flow:
 *
 * If I/O result == CANCELED or error, free the buffer.
 *
 * If query, free the buffer, restart.
 *
 * If response:
 *	Allocate event, fill in details.
 *		If cannot allocate, free buffer, restart.
 *	find target.  If not found, free buffer, restart.
 *	if event queue is not empty, queue.  else, send.
 *	restart.
 */
static void
udp_recv(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	 void *arg) {
	dns_dispentry_t *resp = (dns_dispentry_t *)arg;
	dns_dispatch_t *disp = NULL;
	dns_messageid_t id;
	isc_result_t dres;
	isc_buffer_t source;
	unsigned int flags;
	isc_sockaddr_t peer;
	isc_netaddr_t netaddr;
	int match, timeout = 0;
	dispatch_cb_t response = NULL;
	isc_time_t now;

	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	disp = resp->disp;

	LOCK(&disp->lock);
	INSIST(resp->reading);
	resp->reading = false;

	response = resp->response;

	if (resp->state == DNS_DISPATCHSTATE_CANCELED) {
		/*
		 * Nobody is interested in the callback if the response
		 * has been canceled already.  Detach from the response
		 * and the handle.
		 */
		response = NULL;
		eresult = ISC_R_CANCELED;
	}

	dispentry_log(resp, LVL(90), "read callback:%s, requests %d",
		      isc_result_totext(eresult), disp->requests);

	if (eresult != ISC_R_SUCCESS) {
		/*
		 * This is most likely a network error on a connected
		 * socket, a timeout, or the query has been canceled.
		 * It makes no sense to check the address or parse the
		 * packet, but we can return the error to the caller.
		 */
		goto done;
	}

	peer = isc_nmhandle_peeraddr(handle);
	isc_netaddr_fromsockaddr(&netaddr, &peer);

	/*
	 * If this is from a blackholed address, drop it.
	 */
	if (disp->mgr->blackhole != NULL &&
	    dns_acl_match(&netaddr, NULL, disp->mgr->blackhole, NULL, &match,
			  NULL) == ISC_R_SUCCESS &&
	    match > 0)
	{
		if (isc_log_wouldlog(dns_lctx, LVL(10))) {
			char netaddrstr[ISC_NETADDR_FORMATSIZE];
			isc_netaddr_format(&netaddr, netaddrstr,
					   sizeof(netaddrstr));
			dispentry_log(resp, LVL(10),
				      "blackholed packet from %s", netaddrstr);
		}
		goto next;
	}

	/*
	 * Peek into the buffer to see what we can see.
	 */
	id = resp->id;
	isc_buffer_init(&source, region->base, region->length);
	isc_buffer_add(&source, region->length);
	dres = dns_message_peekheader(&source, &id, &flags);
	if (dres != ISC_R_SUCCESS) {
		char netaddrstr[ISC_NETADDR_FORMATSIZE];
		isc_netaddr_format(&netaddr, netaddrstr, sizeof(netaddrstr));
		dispentry_log(resp, LVL(10), "got garbage packet from %s",
			      netaddrstr);
		goto next;
	}

	dispentry_log(resp, LVL(92),
		      "got valid DNS message header, /QR %c, id %u",
		      (((flags & DNS_MESSAGEFLAG_QR) != 0) ? '1' : '0'), id);

	/*
	 * Look at the message flags.  If it's a query, ignore it.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) == 0) {
		goto next;
	}

	/*
	 * The QID and the address must match the expected ones.
	 */
	if (resp->id != id || !isc_sockaddr_equal(&peer, &resp->peer)) {
		dispentry_log(resp, LVL(90), "response doesn't match");
		inc_stats(disp->mgr, dns_resstatscounter_mismatch);
		goto next;
	}

	/*
	 * We have the right resp, so call the caller back.
	 */
	goto done;

next:
	/*
	 * This is the wrong response.  Check whether there is still enough
	 * time to wait for the correct one to arrive before the timeout fires.
	 */
	TIME_NOW(&now);
	timeout = resp->timeout - dispentry_runtime(resp, &now);
	if (timeout <= 0) {
		/*
		 * The time window for receiving the correct response is
		 * already closed, libuv has just not processed the socket
		 * timer yet.  Invoke the read callback, indicating a timeout.
		 */
		eresult = ISC_R_TIMEDOUT;
		goto done;
	}

	/*
	 * Do not invoke the read callback just yet and instead wait for the
	 * proper response to arrive until the original timeout fires.
	 */
	response = NULL;
	udp_dispatch_getnext(resp, timeout);

done:
	UNLOCK(&disp->lock);

	if (response != NULL) {
		dispentry_log(resp, LVL(90), "UDP read callback on %p: %s",
			      handle, isc_result_totext(eresult));
		response(eresult, region, resp->arg);
	}

	dns_dispentry_detach(&resp); /* DISPENTRY003 */
}

static isc_result_t
tcp_recv_oldest(dns_dispatch_t *disp, dns_dispentry_t **respp) {
	dns_dispentry_t *resp = NULL;
	resp = ISC_LIST_HEAD(disp->active);
	if (resp != NULL) {
		disp->timedout++;

		*respp = resp;
		return (ISC_R_TIMEDOUT);
	}

	return (ISC_R_NOTFOUND);
}

static isc_result_t
tcp_recv_success(dns_dispatch_t *disp, isc_region_t *region, dns_qid_t *qid,
		 isc_sockaddr_t *peer, dns_dispentry_t **respp) {
	isc_buffer_t source;
	dns_messageid_t id;
	unsigned int flags;
	unsigned int bucket;
	isc_result_t result = ISC_R_SUCCESS;
	dns_dispentry_t *resp = NULL;

	dispatch_log(disp, LVL(90), "TCP read success, length == %d, addr = %p",
		     region->length, region->base);

	/*
	 * Peek into the buffer to see what we can see.
	 */
	isc_buffer_init(&source, region->base, region->length);
	isc_buffer_add(&source, region->length);
	result = dns_message_peekheader(&source, &id, &flags);
	if (result != ISC_R_SUCCESS) {
		dispatch_log(disp, LVL(10), "got garbage packet");
		return (ISC_R_UNEXPECTED);
	}

	dispatch_log(disp, LVL(92),
		     "got valid DNS message header, /QR %c, id %u",
		     (((flags & DNS_MESSAGEFLAG_QR) != 0) ? '1' : '0'), id);

	/*
	 * Look at the message flags.  If it's a query, ignore it and keep
	 * reading.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) == 0) {
		dispatch_log(disp, LVL(10), "got DNS query instead of answer");
		return (ISC_R_UNEXPECTED);
	}

	/*
	 * We have a valid response; find the associated dispentry object
	 * and call the caller back.
	 */
	bucket = dns_hash(qid, peer, id, disp->localport);
	LOCK(&qid->lock);
	resp = entry_search(qid, peer, id, disp->localport, bucket);
	if (resp != NULL) {
		if (resp->reading) {
			*respp = resp;
		} else {
			/* We already got our DNS message. */
			result = ISC_R_UNEXPECTED;
		}
	} else {
		/* We are not expecting this DNS message */
		result = ISC_R_NOTFOUND;
	}
	dispatch_log(disp, LVL(90), "search for response in bucket %d: %s",
		     bucket, isc_result_totext(result));
	UNLOCK(&qid->lock);

	return (result);
}

static void
tcp_recv_add(dns_displist_t *resps, dns_dispentry_t *resp,
	     isc_result_t result) {
	dns_dispentry_ref(resp); /* DISPENTRY009 */
	ISC_LIST_UNLINK(resp->disp->active, resp, alink);
	ISC_LIST_APPEND(*resps, resp, rlink);
	INSIST(resp->reading);
	resp->reading = false;
	resp->result = result;
}

static void
tcp_recv_shutdown(dns_dispatch_t *disp, dns_displist_t *resps,
		  isc_result_t result) {
	dns_dispentry_t *resp = NULL, *next = NULL;

	/*
	 * If there are any active responses, shut them all down.
	 */
	for (resp = ISC_LIST_HEAD(disp->active); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, alink);
		tcp_recv_add(resps, resp, result);
	}
	disp->state = DNS_DISPATCHSTATE_CANCELED;
}

static void
tcp_recv_done(dns_dispentry_t *resp, isc_result_t eresult,
	      isc_region_t *region) {
	dispentry_log(resp, LVL(90), "read callback: %s",
		      isc_result_totext(eresult));

	resp->response(eresult, region, resp->arg);
	dns_dispentry_detach(&resp); /* DISPENTRY009 */
}

static void
tcp_recv_processall(dns_displist_t *resps, isc_region_t *region) {
	dns_dispentry_t *resp = NULL, *next = NULL;

	for (resp = ISC_LIST_HEAD(*resps); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, rlink);
		ISC_LIST_UNLINK(*resps, resp, rlink);
		tcp_recv_done(resp, resp->result, region);
	}
}

/*
 * General flow:
 *
 * If I/O result == CANCELED, EOF, or error, notify everyone as the
 * various queues drain.
 *
 * If response:
 *	Allocate event, fill in details.
 *		If cannot allocate, restart.
 *	find target.  If not found, restart.
 *	if event queue is not empty, queue.  else, send.
 *	restart.
 */
static void
tcp_recv(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *region,
	 void *arg) {
	dns_dispatch_t *disp = (dns_dispatch_t *)arg;
	dns_dispentry_t *resp = NULL;
	dns_qid_t *qid = NULL;
	char buf[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_t peer;
	dns_displist_t resps = ISC_LIST_INITIALIZER;
	isc_time_t now;
	int timeout;

	REQUIRE(VALID_DISPATCH(disp));

	qid = disp->mgr->qid;

	TIME_NOW(&now);

	LOCK(&disp->lock);
	INSIST(disp->reading);
	disp->reading = false;

	dispatch_log(disp, LVL(90), "TCP read:%s:requests %u",
		     isc_result_totext(result), disp->requests);

	peer = isc_nmhandle_peeraddr(handle);

	/*
	 * Phase 1: Process timeout and success.
	 */
	switch (result) {
	case ISC_R_TIMEDOUT:
		/*
		 * Time out the oldest response in the active queue.
		 */
		result = tcp_recv_oldest(disp, &resp);
		break;
	case ISC_R_SUCCESS:
		/* We got an answer */
		result = tcp_recv_success(disp, region, qid, &peer, &resp);
		break;

	default:
		break;
	}

	if (resp != NULL) {
		tcp_recv_add(&resps, resp, result);
	}

	/*
	 * Phase 2: Look if we timed out before.
	 */

	if (result == ISC_R_NOTFOUND) {
		if (disp->timedout > 0) {
			/* There was active query that timed-out before */
			disp->timedout--;
		} else {
			result = ISC_R_UNEXPECTED;
		}
	}

	/*
	 * Phase 3: Trigger timeouts.  It's possible that the responses would
	 * have been timedout out already, but non-matching TCP reads have
	 * prevented this.
	 */
	dns_dispentry_t *next = NULL;
	for (resp = ISC_LIST_HEAD(disp->active); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, alink);

		timeout = resp->timeout - dispentry_runtime(resp, &now);
		if (timeout <= 0) {
			tcp_recv_add(&resps, resp, ISC_R_TIMEDOUT);
		}
	}

	/*
	 * Phase 4: log if we errored out.
	 */
	switch (result) {
	case ISC_R_SUCCESS:
	case ISC_R_TIMEDOUT:
	case ISC_R_NOTFOUND:
		break;

	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
	case ISC_R_EOF:
	case ISC_R_CONNECTIONRESET:
		isc_sockaddr_format(&peer, buf, sizeof(buf));
		dispatch_log(disp, LVL(90), "shutting down TCP: %s: %s", buf,
			     isc_result_totext(result));
		tcp_recv_shutdown(disp, &resps, result);
		break;
	default:
		isc_sockaddr_format(&peer, buf, sizeof(buf));
		dispatch_log(disp, ISC_LOG_ERROR,
			     "shutting down due to TCP "
			     "receive error: %s: %s",
			     buf, isc_result_totext(result));
		tcp_recv_shutdown(disp, &resps, result);
		break;
	}

	/*
	 * Phase 5: Resume reading if there are still active responses
	 */
	resp = ISC_LIST_HEAD(disp->active);
	if (resp != NULL) {
		timeout = resp->timeout - dispentry_runtime(resp, &now);
		INSIST(timeout > 0);
		tcp_startrecv(NULL, disp, resp);
		isc_nmhandle_settimeout(handle, timeout);
	}

	UNLOCK(&disp->lock);

	/*
	 * Phase 6: Process all scheduled callbacks.
	 */
	tcp_recv_processall(&resps, region);

	dns_dispatch_detach(&disp); /* DISPATCH002 */
}

/*%
 * Create a temporary port list to set the initial default set of dispatch
 * ephemeral ports.  This is almost meaningless as the application will
 * normally set the ports explicitly, but is provided to fill some minor corner
 * cases.
 */
static void
create_default_portset(isc_mem_t *mctx, int family, isc_portset_t **portsetp) {
	in_port_t low, high;

	isc_net_getudpportrange(family, &low, &high);

	isc_portset_create(mctx, portsetp);
	isc_portset_addrange(*portsetp, low, high);
}

static isc_result_t
setavailports(dns_dispatchmgr_t *mgr, isc_portset_t *v4portset,
	      isc_portset_t *v6portset) {
	in_port_t *v4ports, *v6ports, p = 0;
	unsigned int nv4ports, nv6ports, i4 = 0, i6 = 0;

	nv4ports = isc_portset_nports(v4portset);
	nv6ports = isc_portset_nports(v6portset);

	v4ports = NULL;
	if (nv4ports != 0) {
		v4ports = isc_mem_get(mgr->mctx, sizeof(in_port_t) * nv4ports);
	}
	v6ports = NULL;
	if (nv6ports != 0) {
		v6ports = isc_mem_get(mgr->mctx, sizeof(in_port_t) * nv6ports);
	}

	do {
		if (isc_portset_isset(v4portset, p)) {
			INSIST(i4 < nv4ports);
			v4ports[i4++] = p;
		}
		if (isc_portset_isset(v6portset, p)) {
			INSIST(i6 < nv6ports);
			v6ports[i6++] = p;
		}
	} while (p++ < 65535);
	INSIST(i4 == nv4ports && i6 == nv6ports);

	if (mgr->v4ports != NULL) {
		isc_mem_put(mgr->mctx, mgr->v4ports,
			    mgr->nv4ports * sizeof(in_port_t));
	}
	mgr->v4ports = v4ports;
	mgr->nv4ports = nv4ports;

	if (mgr->v6ports != NULL) {
		isc_mem_put(mgr->mctx, mgr->v6ports,
			    mgr->nv6ports * sizeof(in_port_t));
	}
	mgr->v6ports = v6ports;
	mgr->nv6ports = nv6ports;

	return (ISC_R_SUCCESS);
}

/*
 * Publics.
 */

isc_result_t
dns_dispatchmgr_create(isc_mem_t *mctx, isc_nm_t *nm,
		       dns_dispatchmgr_t **mgrp) {
	dns_dispatchmgr_t *mgr = NULL;
	isc_portset_t *v4portset = NULL;
	isc_portset_t *v6portset = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(mgrp != NULL && *mgrp == NULL);

	mgr = isc_mem_get(mctx, sizeof(dns_dispatchmgr_t));
	*mgr = (dns_dispatchmgr_t){ .magic = 0 };

#if DNS_DISPATCH_TRACE
	fprintf(stderr, "dns_dispatchmgr__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__, mgr);
#endif
	isc_refcount_init(&mgr->references, 1);

	isc_mem_attach(mctx, &mgr->mctx);
	isc_nm_attach(nm, &mgr->nm);

	isc_mutex_init(&mgr->lock);

	ISC_LIST_INIT(mgr->list);

	create_default_portset(mctx, AF_INET, &v4portset);
	create_default_portset(mctx, AF_INET6, &v6portset);

	setavailports(mgr, v4portset, v6portset);

	isc_portset_destroy(mctx, &v4portset);
	isc_portset_destroy(mctx, &v6portset);

	qid_allocate(mgr, &mgr->qid);
	mgr->magic = DNS_DISPATCHMGR_MAGIC;

	*mgrp = mgr;
	return (ISC_R_SUCCESS);
}

#if DNS_DISPATCH_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_dispatchmgr, dispatchmgr_destroy);
#else
ISC_REFCOUNT_IMPL(dns_dispatchmgr, dispatchmgr_destroy);
#endif

void
dns_dispatchmgr_setblackhole(dns_dispatchmgr_t *mgr, dns_acl_t *blackhole) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	if (mgr->blackhole != NULL) {
		dns_acl_detach(&mgr->blackhole);
	}
	dns_acl_attach(blackhole, &mgr->blackhole);
}

dns_acl_t *
dns_dispatchmgr_getblackhole(dns_dispatchmgr_t *mgr) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	return (mgr->blackhole);
}

isc_result_t
dns_dispatchmgr_setavailports(dns_dispatchmgr_t *mgr, isc_portset_t *v4portset,
			      isc_portset_t *v6portset) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	return (setavailports(mgr, v4portset, v6portset));
}

static void
dispatchmgr_destroy(dns_dispatchmgr_t *mgr) {
	REQUIRE(VALID_DISPATCHMGR(mgr));

	isc_refcount_destroy(&mgr->references);

	mgr->magic = 0;
	isc_mutex_destroy(&mgr->lock);

	qid_destroy(mgr->mctx, &mgr->qid);

	if (mgr->blackhole != NULL) {
		dns_acl_detach(&mgr->blackhole);
	}

	if (mgr->stats != NULL) {
		isc_stats_detach(&mgr->stats);
	}

	if (mgr->v4ports != NULL) {
		isc_mem_put(mgr->mctx, mgr->v4ports,
			    mgr->nv4ports * sizeof(in_port_t));
	}
	if (mgr->v6ports != NULL) {
		isc_mem_put(mgr->mctx, mgr->v6ports,
			    mgr->nv6ports * sizeof(in_port_t));
	}

	isc_nm_detach(&mgr->nm);

	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(dns_dispatchmgr_t));
}

void
dns_dispatchmgr_setstats(dns_dispatchmgr_t *mgr, isc_stats_t *stats) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(ISC_LIST_EMPTY(mgr->list));
	REQUIRE(mgr->stats == NULL);

	isc_stats_attach(stats, &mgr->stats);
}

static void
qid_allocate(dns_dispatchmgr_t *mgr, dns_qid_t **qidp) {
	dns_qid_t *qid = NULL;
	unsigned int i;

	REQUIRE(qidp != NULL && *qidp == NULL);

	qid = isc_mem_get(mgr->mctx, sizeof(*qid));
	*qid = (dns_qid_t){ .qid_nbuckets = DNS_QID_BUCKETS,
			    .qid_increment = DNS_QID_INCREMENT };

	qid->qid_table = isc_mem_get(mgr->mctx,
				     DNS_QID_BUCKETS * sizeof(dns_displist_t));
	for (i = 0; i < qid->qid_nbuckets; i++) {
		ISC_LIST_INIT(qid->qid_table[i]);
	}

	isc_mutex_init(&qid->lock);
	qid->magic = QID_MAGIC;
	*qidp = qid;
}

static void
qid_destroy(isc_mem_t *mctx, dns_qid_t **qidp) {
	dns_qid_t *qid = NULL;

	REQUIRE(qidp != NULL);
	qid = *qidp;
	*qidp = NULL;

	REQUIRE(VALID_QID(qid));

	qid->magic = 0;
	isc_mem_put(mctx, qid->qid_table,
		    qid->qid_nbuckets * sizeof(dns_displist_t));
	isc_mutex_destroy(&qid->lock);
	isc_mem_put(mctx, qid, sizeof(*qid));
}

/*
 * Allocate and set important limits.
 */
static void
dispatch_allocate(dns_dispatchmgr_t *mgr, isc_socktype_t type,
		  dns_dispatch_t **dispp) {
	dns_dispatch_t *disp = NULL;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(dispp != NULL && *dispp == NULL);

	/*
	 * Set up the dispatcher, mostly.  Don't bother setting some of
	 * the options that are controlled by tcp vs. udp, etc.
	 */

	disp = isc_mem_get(mgr->mctx, sizeof(*disp));
	*disp = (dns_dispatch_t){
		.socktype = type,
		.link = ISC_LINK_INITIALIZER,
		.active = ISC_LIST_INITIALIZER,
		.pending = ISC_LIST_INITIALIZER,
		.tid = isc_nm_tid(),
		.magic = DISPATCH_MAGIC,
	};

	dns_dispatchmgr_attach(mgr, &disp->mgr);
#if DNS_DISPATCH_TRACE
	fprintf(stderr, "dns_dispatch__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__, disp);
#endif
	isc_refcount_init(&disp->references, 1); /* DISPATCH000 */
	isc_mutex_init(&disp->lock);

	*dispp = disp;
}

isc_result_t
dns_dispatch_createtcp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		       const isc_sockaddr_t *destaddr, dns_dispatch_t **dispp) {
	dns_dispatch_t *disp = NULL;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(destaddr != NULL);

	LOCK(&mgr->lock);

	dispatch_allocate(mgr, isc_socktype_tcp, &disp);

	disp->peer = *destaddr;

	if (localaddr != NULL) {
		disp->local = *localaddr;
	} else {
		int pf;
		pf = isc_sockaddr_pf(destaddr);
		isc_sockaddr_anyofpf(&disp->local, pf);
		isc_sockaddr_setport(&disp->local, 0);
	}

	/*
	 * Append it to the dispatcher list.
	 */

	/* FIXME: There should be a lookup hashtable here */
	ISC_LIST_APPEND(mgr->list, disp, link);
	UNLOCK(&mgr->lock);

	if (isc_log_wouldlog(dns_lctx, 90)) {
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(&disp->local, addrbuf,
				    ISC_SOCKADDR_FORMATSIZE);

		mgr_log(mgr, LVL(90),
			"dns_dispatch_createtcp: created TCP dispatch %p for "
			"%s",
			disp, addrbuf);
	}
	*dispp = disp;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dispatch_gettcp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *destaddr,
		    const isc_sockaddr_t *localaddr, dns_dispatch_t **dispp) {
	dns_dispatch_t *disp_connected = NULL;
	dns_dispatch_t *disp_fallback = NULL;
	isc_result_t result = ISC_R_NOTFOUND;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(destaddr != NULL);
	REQUIRE(dispp != NULL && *dispp == NULL);

	LOCK(&mgr->lock);

	for (dns_dispatch_t *disp = ISC_LIST_HEAD(mgr->list); disp != NULL;
	     disp = ISC_LIST_NEXT(disp, link))
	{
		isc_sockaddr_t sockname;
		isc_sockaddr_t peeraddr;

		LOCK(&disp->lock);

		if (disp->tid != isc_nm_tid()) {
			UNLOCK(&disp->lock);
			continue;
		}

		if (disp->handle != NULL) {
			sockname = isc_nmhandle_localaddr(disp->handle);
			peeraddr = isc_nmhandle_peeraddr(disp->handle);
		} else {
			sockname = disp->local;
			peeraddr = disp->peer;
		}

		/*
		 * The conditions match:
		 * 1. socktype is TCP
		 * 2. destination address is same
		 * 3. local address is either NULL or same
		 */
		if (disp->socktype != isc_socktype_tcp ||
		    !isc_sockaddr_equal(destaddr, &peeraddr) ||
		    (localaddr != NULL &&
		     !isc_sockaddr_eqaddr(localaddr, &sockname)))
		{
			UNLOCK(&disp->lock);
			continue;
		}

		switch (disp->state) {
		case DNS_DISPATCHSTATE_NONE:
			/* A dispatch in indeterminate state, skip it */
			break;
		case DNS_DISPATCHSTATE_CONNECTED:
			if (ISC_LIST_EMPTY(disp->active)) {
				/* Ignore dispatch with no responses */
				break;
			}
			/* We found a connected dispatch */
			dns_dispatch_attach(disp, &disp_connected);
			break;
		case DNS_DISPATCHSTATE_CONNECTING:
			if (ISC_LIST_EMPTY(disp->pending)) {
				/* Ignore dispatch with no responses */
				break;
			}
			/* We found "a" dispatch, store it for later */
			if (disp_fallback == NULL) {
				dns_dispatch_attach(disp, &disp_fallback);
			}
			break;
		case DNS_DISPATCHSTATE_CANCELED:
			/* A canceled dispatch, skip it. */
			break;
		default:
			UNREACHABLE();
		}

		UNLOCK(&disp->lock);

		if (disp_connected != NULL) {
			break;
		}
	}

	if (disp_connected != NULL) {
		/* We found connected dispatch */
		INSIST(disp_connected->handle != NULL);

		*dispp = disp_connected;
		disp_connected = NULL;

		result = ISC_R_SUCCESS;

		if (disp_fallback != NULL) {
			dns_dispatch_detach(&disp_fallback);
		}
	} else if (disp_fallback != NULL) {
		*dispp = disp_fallback;

		result = ISC_R_SUCCESS;
	}

	UNLOCK(&mgr->lock);

	return (result);
}

isc_result_t
dns_dispatch_createudp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		       dns_dispatch_t **dispp) {
	isc_result_t result;
	dns_dispatch_t *disp = NULL;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(localaddr != NULL);
	REQUIRE(dispp != NULL && *dispp == NULL);

	LOCK(&mgr->lock);
	result = dispatch_createudp(mgr, localaddr, &disp);
	if (result == ISC_R_SUCCESS) {
		*dispp = disp;
	}
	UNLOCK(&mgr->lock);

	return (result);
}

static isc_result_t
dispatch_createudp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		   dns_dispatch_t **dispp) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_dispatch_t *disp = NULL;
	isc_sockaddr_t sa_any;

	/*
	 * Check whether this address/port is available locally.
	 */
	isc_sockaddr_anyofpf(&sa_any, isc_sockaddr_pf(localaddr));
	if (!isc_sockaddr_eqaddr(&sa_any, localaddr)) {
		result = isc_nm_checkaddr(localaddr, isc_socktype_udp);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	dispatch_allocate(mgr, isc_socktype_udp, &disp);

	if (isc_log_wouldlog(dns_lctx, 90)) {
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(localaddr, addrbuf,
				    ISC_SOCKADDR_FORMATSIZE);
		mgr_log(mgr, LVL(90),
			"dispatch_createudp: created UDP dispatch %p for %s",
			disp, addrbuf);
	}

	disp->local = *localaddr;

	/*
	 * Don't append it to the dispatcher list, we don't care about UDP, only
	 * TCP should be searched
	 *
	 * ISC_LIST_APPEND(mgr->list, disp, link);
	 */

	*dispp = disp;

	return (result);
}

static void
dispatch_destroy(dns_dispatch_t *disp) {
	dns_dispatchmgr_t *mgr = disp->mgr;

	isc_refcount_destroy(&disp->references);
	disp->magic = 0;

	LOCK(&mgr->lock);
	if (ISC_LINK_LINKED(disp, link)) {
		ISC_LIST_UNLINK(disp->mgr->list, disp, link);
	}
	UNLOCK(&mgr->lock);

	INSIST(disp->requests == 0);
	INSIST(ISC_LIST_EMPTY(disp->pending));
	INSIST(ISC_LIST_EMPTY(disp->active));

	INSIST(!ISC_LINK_LINKED(disp, link));

	dispatch_log(disp, LVL(90), "destroying dispatch %p", disp);

	if (disp->handle) {
		dispatch_log(disp, LVL(90), "detaching TCP handle %p from %p",
			     disp->handle, &disp->handle);
		isc_nmhandle_detach(&disp->handle);
	}

	isc_mutex_destroy(&disp->lock);

	isc_mem_put(mgr->mctx, disp, sizeof(*disp));

	/*
	 * Because dispatch uses mgr->mctx, we must detach after freeing
	 * dispatch, not before.
	 */
	dns_dispatchmgr_detach(&mgr);
}

#if DNS_DISPATCH_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_dispatch, dispatch_destroy);
#else
ISC_REFCOUNT_IMPL(dns_dispatch, dispatch_destroy);
#endif

isc_result_t
dns_dispatch_add(dns_dispatch_t *disp, unsigned int options,
		 unsigned int timeout, const isc_sockaddr_t *dest,
		 dispatch_cb_t connected, dispatch_cb_t sent,
		 dispatch_cb_t response, void *arg, dns_messageid_t *idp,
		 dns_dispentry_t **respp) {
	dns_dispentry_t *resp = NULL;
	dns_qid_t *qid = NULL;
	in_port_t localport;
	dns_messageid_t id;
	unsigned int bucket;
	bool ok = false;
	int i = 0;

	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(dest != NULL);
	REQUIRE(respp != NULL && *respp == NULL);
	REQUIRE(idp != NULL);
	REQUIRE(disp->socktype == isc_socktype_tcp ||
		disp->socktype == isc_socktype_udp);
	REQUIRE(connected != NULL);
	REQUIRE(response != NULL);
	REQUIRE(sent != NULL);

	LOCK(&disp->lock);

	if (disp->state == DNS_DISPATCHSTATE_CANCELED) {
		UNLOCK(&disp->lock);
		return (ISC_R_CANCELED);
	}

	qid = disp->mgr->qid;

	localport = isc_sockaddr_getport(&disp->local);

	resp = isc_mem_get(disp->mgr->mctx, sizeof(*resp));
	*resp = (dns_dispentry_t){
		.port = localport,
		.timeout = timeout,
		.peer = *dest,
		.connected = connected,
		.sent = sent,
		.response = response,
		.arg = arg,
		.link = ISC_LINK_INITIALIZER,
		.alink = ISC_LINK_INITIALIZER,
		.plink = ISC_LINK_INITIALIZER,
		.rlink = ISC_LINK_INITIALIZER,
		.magic = RESPONSE_MAGIC,
	};

#if DNS_DISPATCH_TRACE
	fprintf(stderr, "dns_dispentry__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__, resp);
#endif
	isc_refcount_init(&resp->references, 1); /* DISPENTRY000 */

	if (disp->socktype == isc_socktype_udp) {
		isc_result_t result = setup_socket(disp, resp, dest,
						   &localport);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(disp->mgr->mctx, resp, sizeof(*resp));
			UNLOCK(&disp->lock);
			inc_stats(disp->mgr, dns_resstatscounter_dispsockfail);
			return (result);
		}
	}

	/*
	 * Try somewhat hard to find a unique ID. Start with
	 * a random number unless DNS_DISPATCHOPT_FIXEDID is set,
	 * in which case we start with the ID passed in via *idp.
	 */
	if ((options & DNS_DISPATCHOPT_FIXEDID) != 0) {
		id = *idp;
	} else {
		id = (dns_messageid_t)isc_random16();
	}

	LOCK(&qid->lock);
	do {
		dns_dispentry_t *entry = NULL;
		bucket = dns_hash(qid, dest, id, localport);
		entry = entry_search(qid, dest, id, localport, bucket);
		if (entry == NULL) {
			ok = true;
			break;
		}
		if ((options & DNS_DISPATCHOPT_FIXEDID) != 0) {
			/* When using fixed ID, we either must use it or fail */
			break;
		}
		id += qid->qid_increment;
		id &= 0x0000ffff;
	} while (i++ < 64);

	if (ok) {
		resp->id = id;
		resp->bucket = bucket;
		ISC_LIST_APPEND(qid->qid_table[bucket], resp, link);
	}
	UNLOCK(&qid->lock);

	if (!ok) {
		isc_mem_put(disp->mgr->mctx, resp, sizeof(*resp));
		UNLOCK(&disp->lock);
		return (ISC_R_NOMORE);
	}

	dns_dispatch_attach(disp, &resp->disp); /* DISPATCH001 */

	disp->requests++;

	inc_stats(disp->mgr, (disp->socktype == isc_socktype_udp)
				     ? dns_resstatscounter_disprequdp
				     : dns_resstatscounter_dispreqtcp);

	UNLOCK(&disp->lock);

	*idp = id;
	*respp = resp;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dispatch_getnext(dns_dispentry_t *resp) {
	isc_time_t now;

	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;
	isc_result_t result = ISC_R_SUCCESS;
	int32_t timeout = -1;

	dispentry_log(resp, LVL(90), "getnext for QID %d", resp->id);

	TIME_NOW(&now);
	timeout = resp->timeout - dispentry_runtime(resp, &now);
	if (timeout <= 0) {
		return (ISC_R_TIMEDOUT);
	}

	LOCK(&disp->lock);
	switch (disp->socktype) {
	case isc_socktype_udp:
		udp_dispatch_getnext(resp, timeout);
		break;
	case isc_socktype_tcp:
		tcp_dispatch_getnext(disp, resp, timeout);
		break;
	default:
		UNREACHABLE();
	}
	UNLOCK(&disp->lock);

	return (result);
}

static void
udp_dispentry_cancel(dns_dispentry_t *resp, isc_result_t result) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));
	REQUIRE(VALID_DISPATCHMGR(resp->disp->mgr));

	dns_dispatch_t *disp = resp->disp;
	dns_dispatchmgr_t *mgr = disp->mgr;
	dns_qid_t *qid = mgr->qid;
	dispatch_cb_t response = NULL;

	LOCK(&disp->lock);
	dispentry_log(resp, LVL(90),
		      "canceling response: %s, %s/%s (%s/%s), "
		      "requests %u",
		      isc_result_totext(result), state2str(resp->state),
		      resp->reading ? "reading" : "not reading",
		      state2str(disp->state),
		      disp->reading ? "reading" : "not reading",
		      disp->requests);

	if (ISC_LINK_LINKED(resp, alink)) {
		ISC_LIST_UNLINK(disp->active, resp, alink);
	}

	switch (resp->state) {
	case DNS_DISPATCHSTATE_NONE:
		break;

	case DNS_DISPATCHSTATE_CONNECTING:
		break;

	case DNS_DISPATCHSTATE_CONNECTED:
		if (resp->reading) {
			dns_dispentry_ref(resp); /* DISPENTRY003 */
			response = resp->response;

			dispentry_log(resp, LVL(90), "canceling read on %p",
				      resp->handle);
			isc_nm_cancelread(resp->handle);
		}
		break;

	case DNS_DISPATCHSTATE_CANCELED:
		goto unlock;

	default:
		UNREACHABLE();
	}

	dec_stats(disp->mgr, dns_resstatscounter_disprequdp);

	LOCK(&qid->lock);
	ISC_LIST_UNLINK(qid->qid_table[resp->bucket], resp, link);
	UNLOCK(&qid->lock);
	resp->state = DNS_DISPATCHSTATE_CANCELED;

unlock:
	UNLOCK(&disp->lock);

	if (response) {
		dispentry_log(resp, LVL(90), "read callback: %s",
			      isc_result_totext(result));
		response(result, NULL, resp->arg);
		dns_dispentry_detach(&resp); /* DISPENTRY003 */
	}
}

static void
tcp_dispentry_cancel(dns_dispentry_t *resp, isc_result_t result) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));
	REQUIRE(VALID_DISPATCHMGR(resp->disp->mgr));

	dns_dispatch_t *disp = resp->disp;
	dns_dispatchmgr_t *mgr = disp->mgr;
	dns_qid_t *qid = mgr->qid;
	dns_displist_t resps = ISC_LIST_INITIALIZER;

	LOCK(&disp->lock);
	dispentry_log(resp, LVL(90),
		      "canceling response: %s, %s/%s (%s/%s), "
		      "requests %u",
		      isc_result_totext(result), state2str(resp->state),
		      resp->reading ? "reading" : "not reading",
		      state2str(disp->state),
		      disp->reading ? "reading" : "not reading",
		      disp->requests);

	switch (resp->state) {
	case DNS_DISPATCHSTATE_NONE:
		break;

	case DNS_DISPATCHSTATE_CONNECTING:
		break;

	case DNS_DISPATCHSTATE_CONNECTED:
		if (resp->reading) {
			tcp_recv_add(&resps, resp, ISC_R_CANCELED);
		}

		INSIST(!ISC_LINK_LINKED(resp, alink));

		if (ISC_LIST_EMPTY(disp->active)) {
			INSIST(disp->handle != NULL);

#if DISPATCH_TCP_KEEPALIVE
			/*
			 * This is an experimental code that keeps the TCP
			 * connection open for 1 second before it is finally
			 * closed.  By keeping the TCP connection open, it can
			 * be reused by dns_request that uses
			 * dns_dispatch_gettcp() to join existing TCP
			 * connections.
			 *
			 * It is disabled for now, because it changes the
			 * behaviour, but I am keeping the code here for future
			 * reference when we improve the dns_dispatch to reuse
			 * the TCP connections also in the resolver.
			 *
			 * The TCP connection reuse should be seamless and not
			 * require any extra handling on the client side though.
			 */
			isc_nmhandle_cleartimeout(disp->handle);
			isc_nmhandle_settimeout(disp->handle, 1000);

			if (!disp->reading) {
				dispentry_log(resp, LVL(90),
					      "final 1 second timeout on %p",
					      disp->handle);
				tcp_startrecv(NULL, disp, NULL);
			}
#else
			if (disp->reading) {
				dispentry_log(resp, LVL(90),
					      "canceling read on %p",
					      disp->handle);
				isc_nm_cancelread(disp->handle);
			}
#endif
		}
		break;

	case DNS_DISPATCHSTATE_CANCELED:
		goto unlock;

	default:
		UNREACHABLE();
	}

	dec_stats(disp->mgr, dns_resstatscounter_dispreqtcp);

	LOCK(&qid->lock);
	ISC_LIST_UNLINK(qid->qid_table[resp->bucket], resp, link);
	UNLOCK(&qid->lock);
	resp->state = DNS_DISPATCHSTATE_CANCELED;

unlock:
	UNLOCK(&disp->lock);

	/*
	 * NOTE: Calling the response callback directly from here should be done
	 * asynchronously, as the dns_dispatch_done() is usually called directly
	 * from the response callback, so there's a slight chance that the call
	 * stack will get higher here, but it's mitigated by the ".reading"
	 * flag, so we don't ever go into a loop.
	 */

	tcp_recv_processall(&resps, NULL);
}

static void
dispentry_cancel(dns_dispentry_t *resp, isc_result_t result) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;

	switch (disp->socktype) {
	case isc_socktype_udp:
		udp_dispentry_cancel(resp, result);
		break;
	case isc_socktype_tcp:
		tcp_dispentry_cancel(resp, result);
		break;
	default:
		UNREACHABLE();
	}
}

void
dns_dispatch_done(dns_dispentry_t **respp) {
	REQUIRE(VALID_RESPONSE(*respp));

	dns_dispentry_t *resp = *respp;
	*respp = NULL;

	dispentry_cancel(resp, ISC_R_CANCELED);
	dns_dispentry_detach(&resp); /* DISPENTRY000 */
}

static void
udp_startrecv(isc_nmhandle_t *handle, dns_dispentry_t *resp) {
	REQUIRE(VALID_RESPONSE(resp));

	dispentry_log(resp, LVL(90), "attaching handle %p to %p", handle,
		      &resp->handle);
	isc_nmhandle_attach(handle, &resp->handle);
	dns_dispentry_ref(resp); /* DISPENTRY003 */
	dispentry_log(resp, LVL(90), "reading");
	isc_nm_read(resp->handle, udp_recv, resp);
	resp->reading = true;
}

static void
tcp_startrecv(isc_nmhandle_t *handle, dns_dispatch_t *disp,
	      dns_dispentry_t *resp) {
	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(disp->socktype == isc_socktype_tcp);

	if (handle != NULL) {
		isc_nmhandle_attach(handle, &disp->handle);
	}
	dns_dispatch_ref(disp); /* DISPATCH002 */
	if (resp != NULL) {
		dispentry_log(resp, LVL(90), "reading from %p", disp->handle);
		INSIST(!isc_time_isepoch(&resp->start));
	} else {
		dispatch_log(disp, LVL(90),
			     "TCP reading without response from %p",
			     disp->handle);
	}
	isc_nm_read(disp->handle, tcp_recv, disp);
	disp->reading = true;
}

static void
tcp_connected(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	dns_dispatch_t *disp = (dns_dispatch_t *)arg;
	dns_dispentry_t *resp = NULL;
	dns_dispentry_t *next = NULL;
	dns_displist_t resps = ISC_LIST_INITIALIZER;

	if (isc_log_wouldlog(dns_lctx, 90)) {
		char localbuf[ISC_SOCKADDR_FORMATSIZE];
		char peerbuf[ISC_SOCKADDR_FORMATSIZE];
		if (handle != NULL) {
			isc_sockaddr_t local = isc_nmhandle_localaddr(handle);
			isc_sockaddr_t peer = isc_nmhandle_peeraddr(handle);

			isc_sockaddr_format(&local, localbuf,
					    ISC_SOCKADDR_FORMATSIZE);
			isc_sockaddr_format(&peer, peerbuf,
					    ISC_SOCKADDR_FORMATSIZE);
		} else {
			isc_sockaddr_format(&disp->local, localbuf,
					    ISC_SOCKADDR_FORMATSIZE);
			isc_sockaddr_format(&disp->peer, peerbuf,
					    ISC_SOCKADDR_FORMATSIZE);
		}

		dispatch_log(disp, LVL(90), "connected from %s to %s: %s",
			     localbuf, peerbuf, isc_result_totext(eresult));
	}

	LOCK(&disp->lock);
	INSIST(disp->state == DNS_DISPATCHSTATE_CONNECTING);

	/*
	 * If there are pending responses, call the connect
	 * callbacks for all of them.
	 */
	for (resp = ISC_LIST_HEAD(disp->pending); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, plink);
		ISC_LIST_UNLINK(disp->pending, resp, plink);
		ISC_LIST_APPEND(resps, resp, rlink);
		resp->result = eresult;

		if (resp->state == DNS_DISPATCHSTATE_CANCELED) {
			resp->result = ISC_R_CANCELED;
		} else if (eresult == ISC_R_SUCCESS) {
			resp->state = DNS_DISPATCHSTATE_CONNECTED;
			ISC_LIST_APPEND(disp->active, resp, alink);
			resp->reading = true;
			dispentry_log(resp, LVL(90), "start reading");
		} else {
			resp->state = DNS_DISPATCHSTATE_NONE;
		}
	}

	if (ISC_LIST_EMPTY(disp->active)) {
		/* All responses have been canceled */
		disp->state = DNS_DISPATCHSTATE_CANCELED;
	} else if (eresult == ISC_R_SUCCESS) {
		disp->state = DNS_DISPATCHSTATE_CONNECTED;
		tcp_startrecv(handle, disp, resp);
	} else {
		disp->state = DNS_DISPATCHSTATE_NONE;
	}

	UNLOCK(&disp->lock);

	for (resp = ISC_LIST_HEAD(resps); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, rlink);
		ISC_LIST_UNLINK(resps, resp, rlink);

		dispentry_log(resp, LVL(90), "connect callback: %s",
			      isc_result_totext(resp->result));
		resp->connected(resp->result, NULL, resp->arg);
		dns_dispentry_detach(&resp); /* DISPENTRY005 */
	}

	dns_dispatch_detach(&disp); /* DISPATCH003 */
}

static void
udp_connected(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	dns_dispentry_t *resp = (dns_dispentry_t *)arg;
	dns_dispatch_t *disp = resp->disp;

	dispentry_log(resp, LVL(90), "connected: %s",
		      isc_result_totext(eresult));

	LOCK(&disp->lock);

	switch (resp->state) {
	case DNS_DISPATCHSTATE_CANCELED:
		eresult = ISC_R_CANCELED;
		ISC_LIST_UNLINK(disp->pending, resp, plink);
		goto unlock;
	case DNS_DISPATCHSTATE_CONNECTING:
		ISC_LIST_UNLINK(disp->pending, resp, plink);
		break;
	default:
		UNREACHABLE();
	}

	switch (eresult) {
	case ISC_R_CANCELED:
		break;
	case ISC_R_SUCCESS:
		resp->state = DNS_DISPATCHSTATE_CONNECTED;
		udp_startrecv(handle, resp);
		break;
	case ISC_R_NOPERM:
	case ISC_R_ADDRINUSE: {
		in_port_t localport = isc_sockaddr_getport(&disp->local);
		isc_result_t result;

		/* probably a port collision; try a different one */
		result = setup_socket(disp, resp, &resp->peer, &localport);
		if (result == ISC_R_SUCCESS) {
			UNLOCK(&disp->lock);
			udp_dispatch_connect(disp, resp);
			goto detach;
		}
		resp->state = DNS_DISPATCHSTATE_NONE;
		break;
	}
	default:
		resp->state = DNS_DISPATCHSTATE_NONE;
		break;
	}
unlock:
	UNLOCK(&disp->lock);

	dispentry_log(resp, LVL(90), "connect callback: %s",
		      isc_result_totext(eresult));
	resp->connected(eresult, NULL, resp->arg);

detach:
	dns_dispentry_detach(&resp); /* DISPENTRY004 */
}

static void
udp_dispatch_connect(dns_dispatch_t *disp, dns_dispentry_t *resp) {
	LOCK(&disp->lock);
	resp->state = DNS_DISPATCHSTATE_CONNECTING;
	TIME_NOW(&resp->start);
	dns_dispentry_ref(resp); /* DISPENTRY004 */
	ISC_LIST_APPEND(disp->pending, resp, plink);
	UNLOCK(&disp->lock);

	isc_nm_udpconnect(disp->mgr->nm, &resp->local, &resp->peer,
			  udp_connected, resp, resp->timeout, 0);
}

static isc_result_t
tcp_dispatch_connect(dns_dispatch_t *disp, dns_dispentry_t *resp) {
	/* Check whether the dispatch is already connecting or connected. */
	LOCK(&disp->lock);
	switch (disp->state) {
	case DNS_DISPATCHSTATE_NONE:
		/* First connection, continue with connecting */
		disp->state = DNS_DISPATCHSTATE_CONNECTING;
		resp->state = DNS_DISPATCHSTATE_CONNECTING;
		TIME_NOW(&resp->start);
		dns_dispentry_ref(resp); /* DISPENTRY005 */
		ISC_LIST_APPEND(disp->pending, resp, plink);
		UNLOCK(&disp->lock);

		char localbuf[ISC_SOCKADDR_FORMATSIZE];
		char peerbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(&disp->local, localbuf,
				    ISC_SOCKADDR_FORMATSIZE);
		isc_sockaddr_format(&disp->peer, peerbuf,
				    ISC_SOCKADDR_FORMATSIZE);

		dns_dispatch_ref(disp); /* DISPATCH003 */
		dispentry_log(resp, LVL(90),
			      "connecting from %s to %s, timeout %u", localbuf,
			      peerbuf, resp->timeout);

		isc_nm_tcpdnsconnect(disp->mgr->nm, &disp->local, &disp->peer,
				     tcp_connected, disp, resp->timeout, 0);
		break;

	case DNS_DISPATCHSTATE_CONNECTING:
		/* Connection pending; add resp to the list */
		resp->state = DNS_DISPATCHSTATE_CONNECTING;
		TIME_NOW(&resp->start);
		dns_dispentry_ref(resp); /* DISPENTRY005 */
		ISC_LIST_APPEND(disp->pending, resp, plink);
		UNLOCK(&disp->lock);
		break;

	case DNS_DISPATCHSTATE_CONNECTED:
		resp->state = DNS_DISPATCHSTATE_CONNECTED;
		TIME_NOW(&resp->start);

		/* Add the resp to the reading list */
		ISC_LIST_APPEND(disp->active, resp, alink);
		dispentry_log(resp, LVL(90), "already connected; attaching");
		resp->reading = true;

		if (!disp->reading) {
			/* Restart the reading */
			tcp_startrecv(NULL, disp, resp);
		}

		UNLOCK(&disp->lock);
		/* We are already connected; call the connected cb */
		dispentry_log(resp, LVL(90), "connect callback: %s",
			      isc_result_totext(ISC_R_SUCCESS));
		resp->connected(ISC_R_SUCCESS, NULL, resp->arg);
		break;

	default:
		UNREACHABLE();
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dispatch_connect(dns_dispentry_t *resp) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;

	switch (disp->socktype) {
	case isc_socktype_tcp:
		return (tcp_dispatch_connect(disp, resp));

	case isc_socktype_udp:
		udp_dispatch_connect(disp, resp);
		return (ISC_R_SUCCESS);

	default:
		UNREACHABLE();
	}
}

static void
send_done(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	dns_dispentry_t *resp = (dns_dispentry_t *)cbarg;

	REQUIRE(VALID_RESPONSE(resp));

	dns_dispatch_t *disp = resp->disp;

	REQUIRE(VALID_DISPATCH(disp));

	dispentry_log(resp, LVL(90), "sent: %s", isc_result_totext(result));

	resp->sent(result, NULL, resp->arg);

	if (result != ISC_R_SUCCESS) {
		dispentry_cancel(resp, result);
	}

	dns_dispentry_detach(&resp); /* DISPENTRY007 */
	isc_nmhandle_detach(&handle);
}

static void
tcp_dispatch_getnext(dns_dispatch_t *disp, dns_dispentry_t *resp,
		     int32_t timeout) {
	REQUIRE(timeout <= INT16_MAX);

	if (disp->reading) {
		return;
	}

	if (timeout > 0) {
		isc_nmhandle_settimeout(disp->handle, timeout);
	}

	dispentry_log(resp, LVL(90), "continue reading");

	dns_dispatch_ref(disp); /* DISPATCH002 */
	isc_nm_read(disp->handle, tcp_recv, disp);
	disp->reading = true;

	ISC_LIST_APPEND(disp->active, resp, alink);
	resp->reading = true;
}

static void
udp_dispatch_getnext(dns_dispentry_t *resp, int32_t timeout) {
	REQUIRE(timeout <= INT16_MAX);

	if (resp->reading) {
		return;
	}

	if (timeout > 0) {
		isc_nmhandle_settimeout(resp->handle, timeout);
	}

	dispentry_log(resp, LVL(90), "continue reading");

	dns_dispentry_ref(resp); /* DISPENTRY003 */
	isc_nm_read(resp->handle, udp_recv, resp);
	resp->reading = true;
}

void
dns_dispatch_resume(dns_dispentry_t *resp, uint16_t timeout) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;

	LOCK(&disp->lock);
	switch (disp->socktype) {
	case isc_socktype_udp: {
		udp_dispatch_getnext(resp, timeout);
		break;
	}
	case isc_socktype_tcp:
		INSIST(disp->timedout > 0);
		disp->timedout--;
		tcp_dispatch_getnext(disp, resp, timeout);
		break;
	default:
		UNREACHABLE();
	}

	UNLOCK(&disp->lock);
}

void
dns_dispatch_send(dns_dispentry_t *resp, isc_region_t *r) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;
	isc_nmhandle_t *sendhandle = NULL;

	dispentry_log(resp, LVL(90), "sending");
	switch (disp->socktype) {
	case isc_socktype_udp:
		isc_nmhandle_attach(resp->handle, &sendhandle);
		break;
	case isc_socktype_tcp:
		isc_nmhandle_attach(disp->handle, &sendhandle);
		break;
	default:
		UNREACHABLE();
	}
	dns_dispentry_ref(resp); /* DISPENTRY007 */
	isc_nm_send(sendhandle, r, send_done, resp);
}

isc_result_t
dns_dispatch_getlocaladdress(dns_dispatch_t *disp, isc_sockaddr_t *addrp) {
	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(addrp != NULL);

	if (disp->socktype == isc_socktype_udp) {
		*addrp = disp->local;
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_NOTIMPLEMENTED);
}

isc_result_t
dns_dispentry_getlocaladdress(dns_dispentry_t *resp, isc_sockaddr_t *addrp) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));
	REQUIRE(addrp != NULL);

	dns_dispatch_t *disp = resp->disp;

	switch (disp->socktype) {
	case isc_socktype_tcp:
		*addrp = disp->local;
		return (ISC_R_SUCCESS);
	case isc_socktype_udp:
		*addrp = isc_nmhandle_localaddr(resp->handle);
		return (ISC_R_SUCCESS);
	default:
		UNREACHABLE();
	}
}

dns_dispatch_t *
dns_dispatchset_get(dns_dispatchset_t *dset) {
	dns_dispatch_t *disp = NULL;

	/* check that dispatch set is configured */
	if (dset == NULL || dset->ndisp == 0) {
		return (NULL);
	}

	LOCK(&dset->lock);
	disp = dset->dispatches[dset->cur];
	dset->cur++;
	if (dset->cur == dset->ndisp) {
		dset->cur = 0;
	}
	UNLOCK(&dset->lock);

	return (disp);
}

isc_result_t
dns_dispatchset_create(isc_mem_t *mctx, dns_dispatch_t *source,
		       dns_dispatchset_t **dsetp, int n) {
	isc_result_t result;
	dns_dispatchset_t *dset = NULL;
	dns_dispatchmgr_t *mgr = NULL;
	int i, j;

	REQUIRE(VALID_DISPATCH(source));
	REQUIRE(source->socktype == isc_socktype_udp);
	REQUIRE(dsetp != NULL && *dsetp == NULL);

	mgr = source->mgr;

	dset = isc_mem_get(mctx, sizeof(dns_dispatchset_t));
	*dset = (dns_dispatchset_t){ .ndisp = n };

	isc_mutex_init(&dset->lock);

	dset->dispatches = isc_mem_get(mctx, sizeof(dns_dispatch_t *) * n);

	isc_mem_attach(mctx, &dset->mctx);

	dset->dispatches[0] = NULL;
	dns_dispatch_attach(source, &dset->dispatches[0]); /* DISPATCH004 */

	LOCK(&mgr->lock);
	for (i = 1; i < n; i++) {
		dset->dispatches[i] = NULL;
		result = dispatch_createudp(mgr, &source->local,
					    &dset->dispatches[i]);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
	}

	UNLOCK(&mgr->lock);
	*dsetp = dset;

	return (ISC_R_SUCCESS);

fail:
	UNLOCK(&mgr->lock);

	for (j = 0; j < i; j++) {
		dns_dispatch_detach(&(dset->dispatches[j])); /* DISPATCH004 */
	}
	isc_mem_put(mctx, dset->dispatches, sizeof(dns_dispatch_t *) * n);
	if (dset->mctx == mctx) {
		isc_mem_detach(&dset->mctx);
	}

	isc_mutex_destroy(&dset->lock);
	isc_mem_put(mctx, dset, sizeof(dns_dispatchset_t));
	return (result);
}

void
dns_dispatchset_destroy(dns_dispatchset_t **dsetp) {
	dns_dispatchset_t *dset = NULL;
	int i;

	REQUIRE(dsetp != NULL && *dsetp != NULL);

	dset = *dsetp;
	*dsetp = NULL;
	for (i = 0; i < dset->ndisp; i++) {
		dns_dispatch_detach(&(dset->dispatches[i])); /* DISPATCH004 */
	}
	isc_mem_put(dset->mctx, dset->dispatches,
		    sizeof(dns_dispatch_t *) * dset->ndisp);
	isc_mutex_destroy(&dset->lock);
	isc_mem_putanddetach(&dset->mctx, dset, sizeof(dns_dispatchset_t));
}
