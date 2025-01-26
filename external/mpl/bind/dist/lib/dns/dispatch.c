/*	$NetBSD: dispatch.c,v 1.11 2025/01/26 16:25:22 christos Exp $	*/

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

#include <isc/async.h>
#include <isc/hash.h>
#include <isc/hashmap.h>
#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/netmgr.h>
#include <isc/portset.h>
#include <isc/random.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/tid.h>
#include <isc/time.h>
#include <isc/tls.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/dispatch.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/stats.h>
#include <dns/transport.h>
#include <dns/types.h>

typedef ISC_LIST(dns_dispentry_t) dns_displist_t;

struct dns_dispatchmgr {
	/* Unlocked. */
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	dns_acl_t *blackhole;
	isc_stats_t *stats;
	isc_nm_t *nm;

	uint32_t nloops;

	struct cds_lfht **tcps;

	struct cds_lfht *qids;

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
	isc_mem_t *mctx;
	dns_dispatch_t *disp;
	isc_loop_t *loop;
	isc_nmhandle_t *handle; /*%< netmgr handle for UDP connection */
	dns_dispatchstate_t state;
	dns_transport_t *transport;
	isc_tlsctx_cache_t *tlsctx_cache;
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
	ISC_LINK(dns_dispentry_t) alink;
	ISC_LINK(dns_dispentry_t) plink;
	ISC_LINK(dns_dispentry_t) rlink;

	struct cds_lfht_node ht_node;
	struct rcu_head rcu_head;
};

struct dns_dispatch {
	/* Unlocked. */
	unsigned int magic; /*%< magic */
	uint32_t tid;
	isc_socktype_t socktype;
	isc_refcount_t references;
	isc_mem_t *mctx;
	dns_dispatchmgr_t *mgr;	    /*%< dispatch manager */
	isc_nmhandle_t *handle;	    /*%< netmgr handle for TCP connection */
	isc_sockaddr_t local;	    /*%< local address */
	isc_sockaddr_t peer;	    /*%< peer address (TCP) */
	dns_transport_t *transport; /*%< TCP transport parameters */

	dns_dispatchopt_t options;
	dns_dispatchstate_t state;

	bool reading;

	dns_displist_t pending;
	dns_displist_t active;

	uint_fast32_t requests; /*%< how many requests we have */

	unsigned int timedout;

	struct cds_lfht_node ht_node;
	struct rcu_head rcu_head;
};

#define RESPONSE_MAGIC	  ISC_MAGIC('D', 'r', 's', 'p')
#define VALID_RESPONSE(e) ISC_MAGIC_VALID((e), RESPONSE_MAGIC)

#define DISPATCH_MAGIC	  ISC_MAGIC('D', 'i', 's', 'p')
#define VALID_DISPATCH(e) ISC_MAGIC_VALID((e), DISPATCH_MAGIC)

#define DNS_DISPATCHMGR_MAGIC ISC_MAGIC('D', 'M', 'g', 'r')
#define VALID_DISPATCHMGR(e)  ISC_MAGIC_VALID((e), DNS_DISPATCHMGR_MAGIC)

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
 * The number of attempts to find unique <addr, port, query_id> combination
 */
#define QID_MAX_TRIES 64

/*
 * Initial and minimum QID table sizes.
 */
#define QIDS_INIT_SIZE (1 << 4) /* Must be power of 2 */
#define QIDS_MIN_SIZE  (1 << 4) /* Must be power of 2 */

/*
 * Statics.
 */
static void
dispatchmgr_destroy(dns_dispatchmgr_t *mgr);

static void
udp_recv(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	 void *arg);
static void
tcp_recv(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	 void *arg);
static void
dispentry_cancel(dns_dispentry_t *resp, isc_result_t result);
static isc_result_t
dispatch_createudp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		   uint32_t tid, dns_dispatch_t **dispp);
static void
udp_startrecv(isc_nmhandle_t *handle, dns_dispentry_t *resp);
static void
udp_dispatch_connect(dns_dispatch_t *disp, dns_dispentry_t *resp);
static void
tcp_startrecv(dns_dispatch_t *disp, dns_dispentry_t *resp);
static void
tcp_dispatch_getnext(dns_dispatch_t *disp, dns_dispentry_t *resp,
		     int32_t timeout);
static void
udp_dispatch_getnext(dns_dispentry_t *resp, int32_t timeout);

static const char *
socktype2str(dns_dispentry_t *resp) {
	dns_transport_type_t transport_type = DNS_TRANSPORT_UDP;
	dns_dispatch_t *disp = resp->disp;

	if (disp->socktype == isc_socktype_tcp) {
		if (resp->transport != NULL) {
			transport_type =
				dns_transport_get_type(resp->transport);
		} else {
			transport_type = DNS_TRANSPORT_TCP;
		}
	}

	switch (transport_type) {
	case DNS_TRANSPORT_UDP:
		return "UDP";
	case DNS_TRANSPORT_TCP:
		return "TCP";
	case DNS_TRANSPORT_TLS:
		return "TLS";
	case DNS_TRANSPORT_HTTP:
		return "HTTP";
	default:
		return "<unexpected>";
	}
}

static const char *
state2str(dns_dispatchstate_t state) {
	switch (state) {
	case DNS_DISPATCHSTATE_NONE:
		return "none";
	case DNS_DISPATCHSTATE_CONNECTING:
		return "connecting";
	case DNS_DISPATCHSTATE_CONNECTED:
		return "connected";
	case DNS_DISPATCHSTATE_CANCELED:
		return "canceled";
	default:
		return "<unexpected>";
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

/*%
 * Choose a random port number for a dispatch entry.
 */
static isc_result_t
setup_socket(dns_dispatch_t *disp, dns_dispentry_t *resp,
	     const isc_sockaddr_t *dest, in_port_t *portp) {
	dns_dispatchmgr_t *mgr = disp->mgr;
	unsigned int nports;
	in_port_t *ports = NULL;
	in_port_t port = *portp;

	if (resp->retries++ > 5) {
		return ISC_R_FAILURE;
	}

	if (isc_sockaddr_pf(&disp->local) == AF_INET) {
		nports = mgr->nv4ports;
		ports = mgr->v4ports;
	} else {
		nports = mgr->nv6ports;
		ports = mgr->v6ports;
	}
	if (nports == 0) {
		return ISC_R_ADDRNOTAVAIL;
	}

	resp->local = disp->local;
	resp->peer = *dest;

	if (port == 0) {
		port = ports[isc_random_uniform(nports)];
		isc_sockaddr_setport(&resp->local, port);
		*portp = port;
	}
	resp->port = port;

	return ISC_R_SUCCESS;
}

static uint32_t
qid_hash(const dns_dispentry_t *dispentry) {
	isc_hash32_t hash;

	isc_hash32_init(&hash);

	isc_sockaddr_hash_ex(&hash, &dispentry->peer, true);
	isc_hash32_hash(&hash, &dispentry->id, sizeof(dispentry->id), true);
	isc_hash32_hash(&hash, &dispentry->port, sizeof(dispentry->port), true);

	return isc_hash32_finalize(&hash);
}

static int
qid_match(struct cds_lfht_node *node, const void *key0) {
	const dns_dispentry_t *dispentry =
		caa_container_of(node, dns_dispentry_t, ht_node);
	const dns_dispentry_t *key = key0;

	return dispentry->id == key->id && dispentry->port == key->port &&
	       isc_sockaddr_equal(&dispentry->peer, &key->peer);
}

static void
dispentry_destroy_rcu(struct rcu_head *rcu_head) {
	dns_dispentry_t *resp = caa_container_of(rcu_head, dns_dispentry_t,
						 rcu_head);
	isc_mem_putanddetach(&resp->mctx, resp, sizeof(*resp));
}

static void
dispentry_destroy(dns_dispentry_t *resp) {
	dns_dispatch_t *disp = resp->disp;

	/*
	 * We need to call this from here in case there's an external event that
	 * shuts down our dispatch (like ISC_R_SHUTTINGDOWN).
	 */
	dispentry_cancel(resp, ISC_R_CANCELED);

	INSIST(disp->requests > 0);
	disp->requests--;

	resp->magic = 0;

	INSIST(!ISC_LINK_LINKED(resp, plink));
	INSIST(!ISC_LINK_LINKED(resp, alink));
	INSIST(!ISC_LINK_LINKED(resp, rlink));

	dispentry_log(resp, ISC_LOG_DEBUG(90), "destroying");

	if (resp->handle != NULL) {
		dispentry_log(resp, ISC_LOG_DEBUG(90),
			      "detaching handle %p from %p", resp->handle,
			      &resp->handle);
		isc_nmhandle_detach(&resp->handle);
	}

	if (resp->tlsctx_cache != NULL) {
		isc_tlsctx_cache_detach(&resp->tlsctx_cache);
	}

	if (resp->transport != NULL) {
		dns_transport_detach(&resp->transport);
	}

	dns_dispatch_detach(&disp); /* DISPATCH001 */

	call_rcu(&resp->rcu_head, dispentry_destroy_rcu);
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
		return 0;
	}

	return isc_time_microdiff(now, &resp->start) / 1000;
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
	bool respond = true;
	isc_time_t now;

	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	disp = resp->disp;

	REQUIRE(disp->tid == isc_tid());
	INSIST(resp->reading);
	resp->reading = false;

	if (resp->state == DNS_DISPATCHSTATE_CANCELED) {
		/*
		 * Nobody is interested in the callback if the response
		 * has been canceled already.  Detach from the response
		 * and the handle.
		 */
		respond = false;
		eresult = ISC_R_CANCELED;
	}

	dispentry_log(resp, ISC_LOG_DEBUG(90),
		      "read callback:%s, requests %" PRIuFAST32,
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
		if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(10))) {
			char netaddrstr[ISC_NETADDR_FORMATSIZE];
			isc_netaddr_format(&netaddr, netaddrstr,
					   sizeof(netaddrstr));
			dispentry_log(resp, ISC_LOG_DEBUG(10),
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
		dispentry_log(resp, ISC_LOG_DEBUG(10),
			      "got garbage packet from %s", netaddrstr);
		goto next;
	}

	dispentry_log(resp, ISC_LOG_DEBUG(92),
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
		dispentry_log(resp, ISC_LOG_DEBUG(90),
			      "response doesn't match");
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
	now = isc_loop_now(resp->loop);
	if (resp->timeout > 0) {
		timeout = resp->timeout - dispentry_runtime(resp, &now);
		if (timeout <= 0) {
			/*
			 * The time window for receiving the correct response is
			 * already closed, libuv has just not processed the
			 * socket timer yet.  Invoke the read callback,
			 * indicating a timeout.
			 */
			eresult = ISC_R_TIMEDOUT;
			goto done;
		}
	}

	/*
	 * Do not invoke the read callback just yet and instead wait for the
	 * proper response to arrive until the original timeout fires.
	 */
	respond = false;
	udp_dispatch_getnext(resp, timeout);

done:
	if (respond) {
		dispentry_log(resp, ISC_LOG_DEBUG(90),
			      "UDP read callback on %p: %s", handle,
			      isc_result_totext(eresult));
		resp->response(eresult, region, resp->arg);
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
		return ISC_R_TIMEDOUT;
	}

	return ISC_R_NOTFOUND;
}

/*
 * NOTE: Must be RCU read locked!
 */
static isc_result_t
tcp_recv_success(dns_dispatch_t *disp, isc_region_t *region,
		 isc_sockaddr_t *peer, dns_dispentry_t **respp) {
	isc_buffer_t source;
	dns_messageid_t id;
	unsigned int flags;
	isc_result_t result = ISC_R_SUCCESS;

	dispatch_log(disp, ISC_LOG_DEBUG(90),
		     "TCP read success, length == %d, addr = %p",
		     region->length, region->base);

	/*
	 * Peek into the buffer to see what we can see.
	 */
	isc_buffer_init(&source, region->base, region->length);
	isc_buffer_add(&source, region->length);
	result = dns_message_peekheader(&source, &id, &flags);
	if (result != ISC_R_SUCCESS) {
		dispatch_log(disp, ISC_LOG_DEBUG(10), "got garbage packet");
		return ISC_R_UNEXPECTED;
	}

	dispatch_log(disp, ISC_LOG_DEBUG(92),
		     "got valid DNS message header, /QR %c, id %u",
		     (((flags & DNS_MESSAGEFLAG_QR) != 0) ? '1' : '0'), id);

	/*
	 * Look at the message flags.  If it's a query, ignore it and keep
	 * reading.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) == 0) {
		dispatch_log(disp, ISC_LOG_DEBUG(10),
			     "got DNS query instead of answer");
		return ISC_R_UNEXPECTED;
	}

	/*
	 * We have a valid response; find the associated dispentry object
	 * and call the caller back.
	 */
	dns_dispentry_t key = {
		.id = id,
		.peer = *peer,
		.port = isc_sockaddr_getport(&disp->local),
	};
	struct cds_lfht_iter iter;
	cds_lfht_lookup(disp->mgr->qids, qid_hash(&key), qid_match, &key,
			&iter);

	dns_dispentry_t *resp = cds_lfht_entry(cds_lfht_iter_get_node(&iter),
					       dns_dispentry_t, ht_node);

	/* Skip responses that are not ours */
	if (resp != NULL && resp->disp == disp) {
		if (!resp->reading) {
			/*
			 * We already got a message for this QID and weren't
			 * expecting any more.
			 */
			result = ISC_R_UNEXPECTED;
		} else {
			*respp = resp;
		}
	} else {
		result = ISC_R_NOTFOUND;
	}
	dispatch_log(disp, ISC_LOG_DEBUG(90),
		     "search for response in hashtable: %s",
		     isc_result_totext(result));

	return result;
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
tcp_recv_processall(dns_displist_t *resps, isc_region_t *region) {
	dns_dispentry_t *resp = NULL, *next = NULL;

	for (resp = ISC_LIST_HEAD(*resps); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, rlink);
		ISC_LIST_UNLINK(*resps, resp, rlink);

		dispentry_log(resp, ISC_LOG_DEBUG(90), "read callback: %s",
			      isc_result_totext(resp->result));
		resp->response(resp->result, region, resp->arg);
		dns_dispentry_detach(&resp); /* DISPENTRY009 */
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
	char buf[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_t peer;
	dns_displist_t resps = ISC_LIST_INITIALIZER;
	isc_time_t now;
	int timeout = 0;

	REQUIRE(VALID_DISPATCH(disp));

	REQUIRE(disp->tid == isc_tid());
	INSIST(disp->reading);
	disp->reading = false;

	dispatch_log(disp, ISC_LOG_DEBUG(90),
		     "TCP read:%s:requests %" PRIuFAST32,
		     isc_result_totext(result), disp->requests);

	peer = isc_nmhandle_peeraddr(handle);

	rcu_read_lock();
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
		result = tcp_recv_success(disp, region, &peer, &resp);
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
	 * have been timed out out already, but non-matching TCP reads have
	 * prevented this.
	 */
	resp = ISC_LIST_HEAD(disp->active);
	if (resp != NULL) {
		now = isc_loop_now(resp->loop);
	}
	while (resp != NULL) {
		dns_dispentry_t *next = ISC_LIST_NEXT(resp, alink);

		if (resp->timeout > 0) {
			timeout = resp->timeout - dispentry_runtime(resp, &now);
			if (timeout <= 0) {
				tcp_recv_add(&resps, resp, ISC_R_TIMEDOUT);
			}
		}

		resp = next;
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
		dispatch_log(disp, ISC_LOG_DEBUG(90),
			     "shutting down TCP: %s: %s", buf,
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
		if (resp->timeout > 0) {
			timeout = resp->timeout - dispentry_runtime(resp, &now);
			INSIST(timeout > 0);
		}
		tcp_startrecv(disp, resp);
		if (timeout > 0) {
			isc_nmhandle_settimeout(handle, timeout);
		}
	}

	rcu_read_unlock();

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
		v4ports = isc_mem_cget(mgr->mctx, nv4ports, sizeof(in_port_t));
	}
	v6ports = NULL;
	if (nv6ports != 0) {
		v6ports = isc_mem_cget(mgr->mctx, nv6ports, sizeof(in_port_t));
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
		isc_mem_cput(mgr->mctx, mgr->v4ports, mgr->nv4ports,
			     sizeof(in_port_t));
	}
	mgr->v4ports = v4ports;
	mgr->nv4ports = nv4ports;

	if (mgr->v6ports != NULL) {
		isc_mem_cput(mgr->mctx, mgr->v6ports, mgr->nv6ports,
			     sizeof(in_port_t));
	}
	mgr->v6ports = v6ports;
	mgr->nv6ports = nv6ports;

	return ISC_R_SUCCESS;
}

/*
 * Publics.
 */

isc_result_t
dns_dispatchmgr_create(isc_mem_t *mctx, isc_loopmgr_t *loopmgr, isc_nm_t *nm,
		       dns_dispatchmgr_t **mgrp) {
	dns_dispatchmgr_t *mgr = NULL;
	isc_portset_t *v4portset = NULL;
	isc_portset_t *v6portset = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(mgrp != NULL && *mgrp == NULL);

	mgr = isc_mem_get(mctx, sizeof(dns_dispatchmgr_t));
	*mgr = (dns_dispatchmgr_t){
		.magic = 0,
		.nloops = isc_loopmgr_nloops(loopmgr),
	};

#if DNS_DISPATCH_TRACE
	fprintf(stderr, "dns_dispatchmgr__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__, mgr);
#endif
	isc_refcount_init(&mgr->references, 1);

	isc_mem_attach(mctx, &mgr->mctx);
	isc_nm_attach(nm, &mgr->nm);

	mgr->tcps = isc_mem_cget(mgr->mctx, mgr->nloops, sizeof(mgr->tcps[0]));
	for (size_t i = 0; i < mgr->nloops; i++) {
		mgr->tcps[i] = cds_lfht_new(
			2, 2, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING,
			NULL);
	}

	create_default_portset(mgr->mctx, AF_INET, &v4portset);
	create_default_portset(mgr->mctx, AF_INET6, &v6portset);

	setavailports(mgr, v4portset, v6portset);

	isc_portset_destroy(mgr->mctx, &v4portset);
	isc_portset_destroy(mgr->mctx, &v6portset);

	mgr->qids = cds_lfht_new(QIDS_INIT_SIZE, QIDS_MIN_SIZE, 0,
				 CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING,
				 NULL);

	mgr->magic = DNS_DISPATCHMGR_MAGIC;

	*mgrp = mgr;
	return ISC_R_SUCCESS;
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
	return mgr->blackhole;
}

isc_result_t
dns_dispatchmgr_setavailports(dns_dispatchmgr_t *mgr, isc_portset_t *v4portset,
			      isc_portset_t *v6portset) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	return setavailports(mgr, v4portset, v6portset);
}

static void
dispatchmgr_destroy(dns_dispatchmgr_t *mgr) {
	REQUIRE(VALID_DISPATCHMGR(mgr));

	isc_refcount_destroy(&mgr->references);

	mgr->magic = 0;

	RUNTIME_CHECK(!cds_lfht_destroy(mgr->qids, NULL));

	for (size_t i = 0; i < mgr->nloops; i++) {
		RUNTIME_CHECK(!cds_lfht_destroy(mgr->tcps[i], NULL));
	}
	isc_mem_cput(mgr->mctx, mgr->tcps, mgr->nloops, sizeof(mgr->tcps[0]));

	if (mgr->blackhole != NULL) {
		dns_acl_detach(&mgr->blackhole);
	}

	if (mgr->stats != NULL) {
		isc_stats_detach(&mgr->stats);
	}

	if (mgr->v4ports != NULL) {
		isc_mem_cput(mgr->mctx, mgr->v4ports, mgr->nv4ports,
			     sizeof(in_port_t));
	}
	if (mgr->v6ports != NULL) {
		isc_mem_cput(mgr->mctx, mgr->v6ports, mgr->nv6ports,
			     sizeof(in_port_t));
	}

	isc_nm_detach(&mgr->nm);

	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(dns_dispatchmgr_t));
}

void
dns_dispatchmgr_setstats(dns_dispatchmgr_t *mgr, isc_stats_t *stats) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(mgr->stats == NULL);

	isc_stats_attach(stats, &mgr->stats);
}

/*
 * Allocate and set important limits.
 */
static void
dispatch_allocate(dns_dispatchmgr_t *mgr, isc_socktype_t type, uint32_t tid,
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
		.active = ISC_LIST_INITIALIZER,
		.pending = ISC_LIST_INITIALIZER,
		.tid = tid,
		.magic = DISPATCH_MAGIC,
	};

	isc_mem_attach(mgr->mctx, &disp->mctx);

	dns_dispatchmgr_attach(mgr, &disp->mgr);
#if DNS_DISPATCH_TRACE
	fprintf(stderr, "dns_dispatch__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__, disp);
#endif
	isc_refcount_init(&disp->references, 1); /* DISPATCH000 */

	*dispp = disp;
}

struct dispatch_key {
	const isc_sockaddr_t *local;
	const isc_sockaddr_t *peer;
	const dns_transport_t *transport;
};

static uint32_t
dispatch_hash(struct dispatch_key *key) {
	uint32_t hashval = isc_sockaddr_hash(key->peer, false);
	if (key->local) {
		hashval ^= isc_sockaddr_hash(key->local, true);
	}

	return hashval;
}

static int
dispatch_match(struct cds_lfht_node *node, const void *key0) {
	dns_dispatch_t *disp = caa_container_of(node, dns_dispatch_t, ht_node);
	const struct dispatch_key *key = key0;
	isc_sockaddr_t local;
	isc_sockaddr_t peer;

	if (disp->handle != NULL) {
		local = isc_nmhandle_localaddr(disp->handle);
		peer = isc_nmhandle_peeraddr(disp->handle);
	} else {
		local = disp->local;
		peer = disp->peer;
	}

	return isc_sockaddr_equal(&peer, key->peer) &&
	       disp->transport == key->transport &&
	       (key->local == NULL || isc_sockaddr_equal(&local, key->local));
}

isc_result_t
dns_dispatch_createtcp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		       const isc_sockaddr_t *destaddr,
		       dns_transport_t *transport, dns_dispatchopt_t options,
		       dns_dispatch_t **dispp) {
	dns_dispatch_t *disp = NULL;
	uint32_t tid = isc_tid();

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(destaddr != NULL);

	dispatch_allocate(mgr, isc_socktype_tcp, tid, &disp);

	disp->options = options;
	disp->peer = *destaddr;
	if (transport != NULL) {
		dns_transport_attach(transport, &disp->transport);
	}

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
	struct dispatch_key key = {
		.local = &disp->local,
		.peer = &disp->peer,
		.transport = transport,
	};

	if ((disp->options & DNS_DISPATCHOPT_UNSHARED) == 0) {
		rcu_read_lock();
		cds_lfht_add(mgr->tcps[tid], dispatch_hash(&key),
			     &disp->ht_node);
		rcu_read_unlock();
	}

	if (isc_log_wouldlog(dns_lctx, 90)) {
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(&disp->local, addrbuf,
				    ISC_SOCKADDR_FORMATSIZE);

		mgr_log(mgr, ISC_LOG_DEBUG(90),
			"dns_dispatch_createtcp: created TCP dispatch %p for "
			"%s",
			disp, addrbuf);
	}
	*dispp = disp;

	return ISC_R_SUCCESS;
}

isc_result_t
dns_dispatch_gettcp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *destaddr,
		    const isc_sockaddr_t *localaddr, dns_transport_t *transport,
		    dns_dispatch_t **dispp) {
	dns_dispatch_t *disp_connected = NULL;
	dns_dispatch_t *disp_fallback = NULL;
	isc_result_t result = ISC_R_NOTFOUND;
	uint32_t tid = isc_tid();

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(destaddr != NULL);
	REQUIRE(dispp != NULL && *dispp == NULL);

	struct dispatch_key key = {
		.local = localaddr,
		.peer = destaddr,
		.transport = transport,
	};

	rcu_read_lock();
	struct cds_lfht_iter iter;
	dns_dispatch_t *disp = NULL;
	cds_lfht_for_each_entry_duplicate(mgr->tcps[tid], dispatch_hash(&key),
					  dispatch_match, &key, &iter, disp,
					  ht_node) {
		INSIST(disp->tid == isc_tid());
		INSIST(disp->socktype == isc_socktype_tcp);

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

		if (disp_connected != NULL) {
			break;
		}
	}
	rcu_read_unlock();

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

	return result;
}

isc_result_t
dns_dispatch_createudp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		       dns_dispatch_t **dispp) {
	isc_result_t result;
	dns_dispatch_t *disp = NULL;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(localaddr != NULL);
	REQUIRE(dispp != NULL && *dispp == NULL);

	result = dispatch_createudp(mgr, localaddr, isc_tid(), &disp);
	if (result == ISC_R_SUCCESS) {
		*dispp = disp;
	}

	return result;
}

static isc_result_t
dispatch_createudp(dns_dispatchmgr_t *mgr, const isc_sockaddr_t *localaddr,
		   uint32_t tid, dns_dispatch_t **dispp) {
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
			return result;
		}
	}

	dispatch_allocate(mgr, isc_socktype_udp, tid, &disp);

	if (isc_log_wouldlog(dns_lctx, 90)) {
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(localaddr, addrbuf,
				    ISC_SOCKADDR_FORMATSIZE);
		mgr_log(mgr, ISC_LOG_DEBUG(90),
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

	return result;
}

static void
dispatch_destroy_rcu(struct rcu_head *rcu_head) {
	dns_dispatch_t *disp = caa_container_of(rcu_head, dns_dispatch_t,
						rcu_head);

	isc_mem_putanddetach(&disp->mctx, disp, sizeof(*disp));
}

static void
dispatch_destroy(dns_dispatch_t *disp) {
	dns_dispatchmgr_t *mgr = disp->mgr;
	uint32_t tid = isc_tid();

	disp->magic = 0;

	if (disp->socktype == isc_socktype_tcp &&
	    (disp->options & DNS_DISPATCHOPT_UNSHARED) == 0)
	{
		(void)cds_lfht_del(mgr->tcps[tid], &disp->ht_node);
	}

	INSIST(disp->requests == 0);
	INSIST(ISC_LIST_EMPTY(disp->pending));
	INSIST(ISC_LIST_EMPTY(disp->active));

	dispatch_log(disp, ISC_LOG_DEBUG(90), "destroying dispatch %p", disp);

	if (disp->handle) {
		dispatch_log(disp, ISC_LOG_DEBUG(90),
			     "detaching TCP handle %p from %p", disp->handle,
			     &disp->handle);
		isc_nmhandle_detach(&disp->handle);
	}
	if (disp->transport != NULL) {
		dns_transport_detach(&disp->transport);
	}
	dns_dispatchmgr_detach(&disp->mgr);

	call_rcu(&disp->rcu_head, dispatch_destroy_rcu);
}

#if DNS_DISPATCH_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_dispatch, dispatch_destroy);
#else
ISC_REFCOUNT_IMPL(dns_dispatch, dispatch_destroy);
#endif

isc_result_t
dns_dispatch_add(dns_dispatch_t *disp, isc_loop_t *loop,
		 dns_dispatchopt_t options, unsigned int timeout,
		 const isc_sockaddr_t *dest, dns_transport_t *transport,
		 isc_tlsctx_cache_t *tlsctx_cache, dispatch_cb_t connected,
		 dispatch_cb_t sent, dispatch_cb_t response, void *arg,
		 dns_messageid_t *idp, dns_dispentry_t **respp) {
	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(dest != NULL);
	REQUIRE(respp != NULL && *respp == NULL);
	REQUIRE(idp != NULL);
	REQUIRE(disp->socktype == isc_socktype_tcp ||
		disp->socktype == isc_socktype_udp);
	REQUIRE(connected != NULL);
	REQUIRE(response != NULL);
	REQUIRE(sent != NULL);
	REQUIRE(loop != NULL);
	REQUIRE(disp->tid == isc_tid());
	REQUIRE(disp->transport == transport);

	if (disp->state == DNS_DISPATCHSTATE_CANCELED) {
		return ISC_R_CANCELED;
	}

	in_port_t localport = isc_sockaddr_getport(&disp->local);
	dns_dispentry_t *resp = isc_mem_get(disp->mctx, sizeof(*resp));
	*resp = (dns_dispentry_t){
		.timeout = timeout,
		.port = localport,
		.peer = *dest,
		.loop = loop,
		.connected = connected,
		.sent = sent,
		.response = response,
		.arg = arg,
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
			isc_mem_put(disp->mctx, resp, sizeof(*resp));
			inc_stats(disp->mgr, dns_resstatscounter_dispsockfail);
			return result;
		}
	}

	isc_result_t result = ISC_R_NOMORE;
	size_t i = 0;
	rcu_read_lock();
	do {
		/*
		 * Try somewhat hard to find a unique ID. Start with
		 * a random number unless DNS_DISPATCHOPT_FIXEDID is set,
		 * in which case we start with the ID passed in via *idp.
		 */
		resp->id = ((options & DNS_DISPATCHOPT_FIXEDID) != 0)
				   ? *idp
				   : (dns_messageid_t)isc_random16();

		struct cds_lfht_node *node =
			cds_lfht_add_unique(disp->mgr->qids, qid_hash(resp),
					    qid_match, resp, &resp->ht_node);

		if (node != &resp->ht_node) {
			if ((options & DNS_DISPATCHOPT_FIXEDID) != 0) {
				/*
				 * When using fixed ID, we either must
				 * use it or fail
				 */
				goto fail;
			}
		} else {
			result = ISC_R_SUCCESS;
			break;
		}
	} while (i++ < QID_MAX_TRIES);
fail:
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(disp->mctx, resp, sizeof(*resp));
		rcu_read_unlock();
		return result;
	}

	isc_mem_attach(disp->mctx, &resp->mctx);

	if (transport != NULL) {
		dns_transport_attach(transport, &resp->transport);
	}

	if (tlsctx_cache != NULL) {
		isc_tlsctx_cache_attach(tlsctx_cache, &resp->tlsctx_cache);
	}

	dns_dispatch_attach(disp, &resp->disp); /* DISPATCH001 */

	disp->requests++;

	inc_stats(disp->mgr, (disp->socktype == isc_socktype_udp)
				     ? dns_resstatscounter_disprequdp
				     : dns_resstatscounter_dispreqtcp);

	rcu_read_unlock();

	*idp = resp->id;
	*respp = resp;

	return ISC_R_SUCCESS;
}

isc_result_t
dns_dispatch_getnext(dns_dispentry_t *resp) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;
	isc_result_t result = ISC_R_SUCCESS;
	int32_t timeout = 0;

	dispentry_log(resp, ISC_LOG_DEBUG(90), "getnext for QID %d", resp->id);

	if (resp->timeout > 0) {
		isc_time_t now = isc_loop_now(resp->loop);
		timeout = resp->timeout - dispentry_runtime(resp, &now);
		if (timeout <= 0) {
			return ISC_R_TIMEDOUT;
		}
	}

	REQUIRE(disp->tid == isc_tid());
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

	return result;
}

/*
 * NOTE: Must be RCU read locked!
 */
static void
udp_dispentry_cancel(dns_dispentry_t *resp, isc_result_t result) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));
	REQUIRE(VALID_DISPATCHMGR(resp->disp->mgr));

	dns_dispatch_t *disp = resp->disp;
	bool respond = false;

	REQUIRE(disp->tid == isc_tid());
	dispentry_log(resp, ISC_LOG_DEBUG(90),
		      "canceling response: %s, %s/%s (%s/%s), "
		      "requests %" PRIuFAST32,
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
			respond = true;
			dispentry_log(resp, ISC_LOG_DEBUG(90),
				      "canceling read on %p", resp->handle);
			isc_nm_cancelread(resp->handle);
		}
		break;

	case DNS_DISPATCHSTATE_CANCELED:
		goto unlock;

	default:
		UNREACHABLE();
	}

	dec_stats(disp->mgr, dns_resstatscounter_disprequdp);

	(void)cds_lfht_del(disp->mgr->qids, &resp->ht_node);

	resp->state = DNS_DISPATCHSTATE_CANCELED;

unlock:
	if (respond) {
		dispentry_log(resp, ISC_LOG_DEBUG(90), "read callback: %s",
			      isc_result_totext(result));
		resp->response(result, NULL, resp->arg);
	}
}

/*
 * NOTE: Must be RCU read locked!
 */
static void
tcp_dispentry_cancel(dns_dispentry_t *resp, isc_result_t result) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));
	REQUIRE(VALID_DISPATCHMGR(resp->disp->mgr));

	dns_dispatch_t *disp = resp->disp;
	dns_displist_t resps = ISC_LIST_INITIALIZER;

	REQUIRE(disp->tid == isc_tid());
	dispentry_log(resp, ISC_LOG_DEBUG(90),
		      "canceling response: %s, %s/%s (%s/%s), "
		      "requests %" PRIuFAST32,
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
				dispentry_log(resp, ISC_LOG_DEBUG(90),
					      "final 1 second timeout on %p",
					      disp->handle);
				tcp_startrecv(disp, NULL);
			}
#else
			if (disp->reading) {
				dispentry_log(resp, ISC_LOG_DEBUG(90),
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

	(void)cds_lfht_del(disp->mgr->qids, &resp->ht_node);

	resp->state = DNS_DISPATCHSTATE_CANCELED;

unlock:

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

	rcu_read_lock();
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
	rcu_read_unlock();
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

	dispentry_log(resp, ISC_LOG_DEBUG(90), "attaching handle %p to %p",
		      handle, &resp->handle);
	isc_nmhandle_attach(handle, &resp->handle);
	dns_dispentry_ref(resp); /* DISPENTRY003 */
	dispentry_log(resp, ISC_LOG_DEBUG(90), "reading");
	isc_nm_read(resp->handle, udp_recv, resp);
	resp->reading = true;
}

static void
tcp_startrecv(dns_dispatch_t *disp, dns_dispentry_t *resp) {
	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(disp->socktype == isc_socktype_tcp);

	dns_dispatch_ref(disp); /* DISPATCH002 */
	if (resp != NULL) {
		dispentry_log(resp, ISC_LOG_DEBUG(90), "reading from %p",
			      disp->handle);
		INSIST(!isc_time_isepoch(&resp->start));
	} else {
		dispatch_log(disp, ISC_LOG_DEBUG(90),
			     "TCP reading without response from %p",
			     disp->handle);
	}
	isc_nm_read(disp->handle, tcp_recv, disp);
	disp->reading = true;
}

static void
resp_connected(void *arg) {
	dns_dispentry_t *resp = arg;
	dispentry_log(resp, ISC_LOG_DEBUG(90), "connect callback: %s",
		      isc_result_totext(resp->result));

	resp->connected(resp->result, NULL, resp->arg);
	dns_dispentry_detach(&resp); /* DISPENTRY005 */
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

		dispatch_log(disp, ISC_LOG_DEBUG(90),
			     "connected from %s to %s: %s", localbuf, peerbuf,
			     isc_result_totext(eresult));
	}

	REQUIRE(disp->tid == isc_tid());
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
			dispentry_log(resp, ISC_LOG_DEBUG(90), "start reading");
		} else {
			resp->state = DNS_DISPATCHSTATE_NONE;
		}
	}

	if (ISC_LIST_EMPTY(disp->active)) {
		/* All responses have been canceled */
		disp->state = DNS_DISPATCHSTATE_CANCELED;
	} else if (eresult == ISC_R_SUCCESS) {
		disp->state = DNS_DISPATCHSTATE_CONNECTED;
		isc_nmhandle_attach(handle, &disp->handle);
		tcp_startrecv(disp, resp);
	} else {
		disp->state = DNS_DISPATCHSTATE_NONE;
	}

	for (resp = ISC_LIST_HEAD(resps); resp != NULL; resp = next) {
		next = ISC_LIST_NEXT(resp, rlink);
		ISC_LIST_UNLINK(resps, resp, rlink);

		resp_connected(resp);
	}

	dns_dispatch_detach(&disp); /* DISPATCH003 */
}

static void
udp_connected(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	dns_dispentry_t *resp = (dns_dispentry_t *)arg;
	dns_dispatch_t *disp = resp->disp;

	dispentry_log(resp, ISC_LOG_DEBUG(90), "connected: %s",
		      isc_result_totext(eresult));

	REQUIRE(disp->tid == isc_tid());
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

	dispentry_log(resp, ISC_LOG_DEBUG(90), "connect callback: %s",
		      isc_result_totext(eresult));
	resp->connected(eresult, NULL, resp->arg);

detach:
	dns_dispentry_detach(&resp); /* DISPENTRY004 */
}

static void
udp_dispatch_connect(dns_dispatch_t *disp, dns_dispentry_t *resp) {
	REQUIRE(disp->tid == isc_tid());
	resp->state = DNS_DISPATCHSTATE_CONNECTING;
	resp->start = isc_loop_now(resp->loop);
	dns_dispentry_ref(resp); /* DISPENTRY004 */
	ISC_LIST_APPEND(disp->pending, resp, plink);

	isc_nm_udpconnect(disp->mgr->nm, &resp->local, &resp->peer,
			  udp_connected, resp, resp->timeout);
}

static isc_result_t
tcp_dispatch_connect(dns_dispatch_t *disp, dns_dispentry_t *resp) {
	dns_transport_type_t transport_type = DNS_TRANSPORT_TCP;
	isc_tlsctx_t *tlsctx = NULL;
	isc_tlsctx_client_session_cache_t *sess_cache = NULL;

	if (resp->transport != NULL) {
		transport_type = dns_transport_get_type(resp->transport);
	}

	if (transport_type == DNS_TRANSPORT_TLS) {
		isc_result_t result;

		result = dns_transport_get_tlsctx(
			resp->transport, &resp->peer, resp->tlsctx_cache,
			resp->mctx, &tlsctx, &sess_cache);

		if (result != ISC_R_SUCCESS) {
			return result;
		}
		INSIST(tlsctx != NULL);
	}

	/* Check whether the dispatch is already connecting or connected. */
	REQUIRE(disp->tid == isc_tid());
	switch (disp->state) {
	case DNS_DISPATCHSTATE_NONE:
		/* First connection, continue with connecting */
		disp->state = DNS_DISPATCHSTATE_CONNECTING;
		resp->state = DNS_DISPATCHSTATE_CONNECTING;
		resp->start = isc_loop_now(resp->loop);
		dns_dispentry_ref(resp); /* DISPENTRY005 */
		ISC_LIST_APPEND(disp->pending, resp, plink);

		char localbuf[ISC_SOCKADDR_FORMATSIZE];
		char peerbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(&disp->local, localbuf,
				    ISC_SOCKADDR_FORMATSIZE);
		isc_sockaddr_format(&disp->peer, peerbuf,
				    ISC_SOCKADDR_FORMATSIZE);

		dns_dispatch_ref(disp); /* DISPATCH003 */
		dispentry_log(resp, ISC_LOG_DEBUG(90),
			      "connecting from %s to %s, timeout %u", localbuf,
			      peerbuf, resp->timeout);

		isc_nm_streamdnsconnect(disp->mgr->nm, &disp->local,
					&disp->peer, tcp_connected, disp,
					resp->timeout, tlsctx, sess_cache,
					ISC_NM_PROXY_NONE, NULL);
		break;

	case DNS_DISPATCHSTATE_CONNECTING:
		/* Connection pending; add resp to the list */
		resp->state = DNS_DISPATCHSTATE_CONNECTING;
		resp->start = isc_loop_now(resp->loop);
		dns_dispentry_ref(resp); /* DISPENTRY005 */
		ISC_LIST_APPEND(disp->pending, resp, plink);
		break;

	case DNS_DISPATCHSTATE_CONNECTED:
		resp->state = DNS_DISPATCHSTATE_CONNECTED;
		resp->start = isc_loop_now(resp->loop);

		/* Add the resp to the reading list */
		ISC_LIST_APPEND(disp->active, resp, alink);
		dispentry_log(resp, ISC_LOG_DEBUG(90),
			      "already connected; attaching");
		resp->reading = true;

		if (!disp->reading) {
			/* Restart the reading */
			tcp_startrecv(disp, resp);
		}

		/* Already connected; call the connected cb asynchronously */
		dns_dispentry_ref(resp); /* DISPENTRY005 */
		isc_async_run(resp->loop, resp_connected, resp);
		break;

	default:
		UNREACHABLE();
	}

	return ISC_R_SUCCESS;
}

isc_result_t
dns_dispatch_connect(dns_dispentry_t *resp) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;

	switch (disp->socktype) {
	case isc_socktype_tcp:
		return tcp_dispatch_connect(disp, resp);

	case isc_socktype_udp:
		udp_dispatch_connect(disp, resp);
		return ISC_R_SUCCESS;

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

	dispentry_log(resp, ISC_LOG_DEBUG(90), "sent: %s",
		      isc_result_totext(result));

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

	dispentry_log(resp, ISC_LOG_DEBUG(90), "continue reading");

	if (!resp->reading) {
		ISC_LIST_APPEND(disp->active, resp, alink);
		resp->reading = true;
	}

	if (disp->reading) {
		return;
	}

	if (timeout > 0) {
		isc_nmhandle_settimeout(disp->handle, timeout);
	}

	dns_dispatch_ref(disp); /* DISPATCH002 */
	isc_nm_read(disp->handle, tcp_recv, disp);
	disp->reading = true;
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

	dispentry_log(resp, ISC_LOG_DEBUG(90), "continue reading");

	dns_dispentry_ref(resp); /* DISPENTRY003 */
	isc_nm_read(resp->handle, udp_recv, resp);
	resp->reading = true;
}

void
dns_dispatch_resume(dns_dispentry_t *resp, uint16_t timeout) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;

	dispentry_log(resp, ISC_LOG_DEBUG(90), "resume");

	REQUIRE(disp->tid == isc_tid());
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
}

void
dns_dispatch_send(dns_dispentry_t *resp, isc_region_t *r) {
	REQUIRE(VALID_RESPONSE(resp));
	REQUIRE(VALID_DISPATCH(resp->disp));

	dns_dispatch_t *disp = resp->disp;
	isc_nmhandle_t *sendhandle = NULL;

	dispentry_log(resp, ISC_LOG_DEBUG(90), "sending");
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
		return ISC_R_SUCCESS;
	}
	return ISC_R_NOTIMPLEMENTED;
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
		return ISC_R_SUCCESS;
	case isc_socktype_udp:
		*addrp = isc_nmhandle_localaddr(resp->handle);
		return ISC_R_SUCCESS;
	default:
		UNREACHABLE();
	}
}

dns_dispatch_t *
dns_dispatchset_get(dns_dispatchset_t *dset) {
	uint32_t tid = isc_tid();

	/* check that dispatch set is configured */
	if (dset == NULL || dset->ndisp == 0) {
		return NULL;
	}

	INSIST(tid < dset->ndisp);

	return dset->dispatches[tid];
}

isc_result_t
dns_dispatchset_create(isc_mem_t *mctx, dns_dispatch_t *source,
		       dns_dispatchset_t **dsetp, uint32_t ndisp) {
	isc_result_t result;
	dns_dispatchset_t *dset = NULL;
	dns_dispatchmgr_t *mgr = NULL;
	size_t i;

	REQUIRE(VALID_DISPATCH(source));
	REQUIRE(source->socktype == isc_socktype_udp);
	REQUIRE(dsetp != NULL && *dsetp == NULL);

	mgr = source->mgr;

	dset = isc_mem_get(mctx, sizeof(dns_dispatchset_t));
	*dset = (dns_dispatchset_t){ .ndisp = ndisp };

	isc_mem_attach(mctx, &dset->mctx);

	dset->dispatches = isc_mem_cget(dset->mctx, ndisp,
					sizeof(dns_dispatch_t *));

	dset->dispatches[0] = NULL;
	dns_dispatch_attach(source, &dset->dispatches[0]); /* DISPATCH004 */

	for (i = 1; i < dset->ndisp; i++) {
		result = dispatch_createudp(mgr, &source->local, i,
					    &dset->dispatches[i]);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
	}

	*dsetp = dset;

	return ISC_R_SUCCESS;

fail:
	for (size_t j = 0; j < i; j++) {
		dns_dispatch_detach(&(dset->dispatches[j])); /* DISPATCH004 */
	}
	isc_mem_cput(dset->mctx, dset->dispatches, ndisp,
		     sizeof(dns_dispatch_t *));

	isc_mem_putanddetach(&dset->mctx, dset, sizeof(dns_dispatchset_t));
	return result;
}

void
dns_dispatchset_destroy(dns_dispatchset_t **dsetp) {
	REQUIRE(dsetp != NULL && *dsetp != NULL);

	dns_dispatchset_t *dset = *dsetp;
	*dsetp = NULL;

	for (size_t i = 0; i < dset->ndisp; i++) {
		dns_dispatch_detach(&(dset->dispatches[i])); /* DISPATCH004 */
	}
	isc_mem_cput(dset->mctx, dset->dispatches, dset->ndisp,
		     sizeof(dns_dispatch_t *));
	isc_mem_putanddetach(&dset->mctx, dset, sizeof(dns_dispatchset_t));
}

isc_result_t
dns_dispatch_checkperm(dns_dispatch_t *disp) {
	REQUIRE(VALID_DISPATCH(disp));

	if (disp->handle == NULL || disp->socktype == isc_socktype_udp) {
		return ISC_R_NOPERM;
	}

	return isc_nm_xfr_checkperm(disp->handle);
}
