/*	$NetBSD: client.c,v 1.23 2025/01/27 02:16:05 christos Exp $	*/

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

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>

#include <isc/async.h>
#include <isc/atomic.h>
#include <isc/formatcheck.h>
#include <isc/fuzz.h>
#include <isc/hmac.h>
#include <isc/log.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/random.h>
#include <isc/safe.h>
#include <isc/serial.h>
#include <isc/siphash.h>
#include <isc/stats.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/tid.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/badcache.h>
#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/dnstap.h>
#include <dns/edns.h>
#include <dns/message.h>
#include <dns/peer.h>
#include <dns/rcode.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/stats.h>
#include <dns/tsig.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <ns/client.h>
#include <ns/interfacemgr.h>
#include <ns/log.h>
#include <ns/notify.h>
#include <ns/server.h>
#include <ns/stats.h>
#include <ns/update.h>

/***
 *** Client
 ***/

/*! \file
 * Client Routines
 *
 * Important note!
 *
 * All client state changes, other than that from idle to listening, occur
 * as a result of events.  This guarantees serialization and avoids the
 * need for locking.
 *
 * If a routine is ever created that allows someone other than the client's
 * loop to change the client, then the client will have to be locked.
 */

#ifdef NS_CLIENT_TRACE
#define CTRACE(m)                                                         \
	ns_client_log(client, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_CLIENT, \
		      ISC_LOG_DEBUG(3), "%s", (m))
#define MTRACE(m)                                                          \
	isc_log_write(ns_lctx, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_CLIENT, \
		      ISC_LOG_DEBUG(3), "clientmgr @%p: %s", manager, (m))
#else /* ifdef NS_CLIENT_TRACE */
#define CTRACE(m) ((void)(m))
#define MTRACE(m) ((void)(m))
#endif /* ifdef NS_CLIENT_TRACE */

#define TCP_CLIENT(c) (((c)->attributes & NS_CLIENTATTR_TCP) != 0)

#define COOKIE_SIZE 24U /* 8 + 4 + 4 + 8 */
#define ECS_SIZE    20U /* 2 + 1 + 1 + [0..16] */

#define TCPBUFFERS_FILLCOUNT 1U
#define TCPBUFFERS_FREEMAX   8U

#define WANTNSID(x)	(((x)->attributes & NS_CLIENTATTR_WANTNSID) != 0)
#define WANTEXPIRE(x)	(((x)->attributes & NS_CLIENTATTR_WANTEXPIRE) != 0)
#define WANTPAD(x)	(((x)->attributes & NS_CLIENTATTR_WANTPAD) != 0)
#define USEKEEPALIVE(x) (((x)->attributes & NS_CLIENTATTR_USEKEEPALIVE) != 0)

#define MANAGER_MAGIC	 ISC_MAGIC('N', 'S', 'C', 'm')
#define VALID_MANAGER(m) ISC_MAGIC_VALID(m, MANAGER_MAGIC)

/*
 * Enable ns_client_dropport() by default.
 */
#ifndef NS_CLIENT_DROPPORT
#define NS_CLIENT_DROPPORT 1
#endif /* ifndef NS_CLIENT_DROPPORT */

#ifdef _LP64
atomic_uint_fast64_t ns_client_requests = 0;
#else
atomic_uint_fast32_t ns_client_requests = 0;
#endif

static atomic_uint_fast32_t last_sigchecks_quota_log = 0;

static bool
can_log_sigchecks_quota(void) {
	isc_stdtime_t last;
	isc_stdtime_t now = isc_stdtime_now();
	last = atomic_exchange_relaxed(&last_sigchecks_quota_log, now);
	if (now != last) {
		return true;
	}

	return false;
}

static void
clientmgr_destroy_cb(void *arg);
static void
ns_client_dumpmessage(ns_client_t *client, const char *reason);
static void
ns_client_request_continue(void *arg);
static void
compute_cookie(ns_client_t *client, uint32_t when, const unsigned char *secret,
	       isc_buffer_t *buf);

#ifdef HAVE_DNSTAP
static dns_transport_type_t
ns_client_transport_type(const ns_client_t *client) {
	/*
	 * Early escape hatch for libtest/ns.c
	 *
	 * When DoQ support this had to be removed to get correct DoQ entries.
	 */
	if (!TCP_CLIENT(client)) {
		return DNS_TRANSPORT_UDP;
	}

	INSIST(client->handle != NULL);

	switch (isc_nm_socket_type(client->handle)) {
	case isc_nm_udpsocket:
	case isc_nm_udplistener:
	case isc_nm_proxyudpsocket:
	case isc_nm_proxyudplistener:
		return DNS_TRANSPORT_UDP;
	case isc_nm_tlssocket:
	case isc_nm_tlslistener:
		return DNS_TRANSPORT_TLS;
	case isc_nm_httpsocket:
	case isc_nm_httplistener:
		return DNS_TRANSPORT_HTTP;
	case isc_nm_streamdnslistener:
	case isc_nm_streamdnssocket:
	case isc_nm_proxystreamlistener:
	case isc_nm_proxystreamsocket:
		/* If it isn't DoT, it is DNS-over-TCP */
		if (isc_nm_has_encryption(client->handle)) {
			return DNS_TRANSPORT_TLS;
		}
		FALLTHROUGH;
	case isc_nm_tcpsocket:
	case isc_nm_tcplistener:
		return DNS_TRANSPORT_TCP;
	case isc_nm_maxsocket:
	case isc_nm_nonesocket:
		UNREACHABLE();
	}

	return DNS_TRANSPORT_UDP;
}
#endif /* HAVE_DNSTAP */

void
ns_client_recursing(ns_client_t *client) {
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(client->state == NS_CLIENTSTATE_WORKING);

	LOCK(&client->manager->reclock);
	client->state = NS_CLIENTSTATE_RECURSING;
	ISC_LIST_APPEND(client->manager->recursing, client, rlink);
	UNLOCK(&client->manager->reclock);
}

void
ns_client_killoldestquery(ns_client_t *client) {
	ns_client_t *oldest;
	REQUIRE(NS_CLIENT_VALID(client));

	LOCK(&client->manager->reclock);
	oldest = ISC_LIST_HEAD(client->manager->recursing);
	if (oldest != NULL) {
		ISC_LIST_UNLINK(client->manager->recursing, oldest, rlink);
		ns_query_cancel(oldest);
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_reclimitdropped);
	}
	UNLOCK(&client->manager->reclock);
}

void
ns_client_settimeout(ns_client_t *client, unsigned int seconds) {
	UNUSED(client);
	UNUSED(seconds);
	/* XXXWPK TODO use netmgr to set timeout */
}

static void
client_extendederror_reset(ns_client_t *client) {
	if (client->ede == NULL) {
		return;
	}
	isc_mem_put(client->manager->mctx, client->ede->value,
		    client->ede->length);
	isc_mem_put(client->manager->mctx, client->ede, sizeof(dns_ednsopt_t));
	client->ede = NULL;
}

void
ns_client_extendederror(ns_client_t *client, uint16_t code, const char *text) {
	unsigned char ede[DNS_EDE_EXTRATEXT_LEN + 2];
	isc_buffer_t buf;
	uint16_t len = sizeof(uint16_t);

	REQUIRE(NS_CLIENT_VALID(client));

	if (client->ede != NULL) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "already have ede, ignoring %u %s", code,
			      text == NULL ? "(null)" : text);
		return;
	}

	ns_client_log(client, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(1), "set ede: info-code %u extra-text %s",
		      code, text == NULL ? "(null)" : text);

	isc_buffer_init(&buf, ede, sizeof(ede));
	isc_buffer_putuint16(&buf, code);
	if (text != NULL && strlen(text) > 0) {
		if (strlen(text) < DNS_EDE_EXTRATEXT_LEN) {
			isc_buffer_putstr(&buf, text);
			len += (uint16_t)(strlen(text));
		} else {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_WARNING,
				      "ede extra-text too long, ignoring");
		}
	}

	client->ede = isc_mem_get(client->manager->mctx, sizeof(dns_ednsopt_t));
	client->ede->code = DNS_OPT_EDE;
	client->ede->length = len;
	client->ede->value = isc_mem_get(client->manager->mctx, len);
	memmove(client->ede->value, ede, len);
}

static void
ns_client_endrequest(ns_client_t *client) {
	INSIST(client->state == NS_CLIENTSTATE_WORKING ||
	       client->state == NS_CLIENTSTATE_RECURSING);

	CTRACE("endrequest");

	if (client->state == NS_CLIENTSTATE_RECURSING) {
		LOCK(&client->manager->reclock);
		if (ISC_LINK_LINKED(client, rlink)) {
			ISC_LIST_UNLINK(client->manager->recursing, client,
					rlink);
		}
		UNLOCK(&client->manager->reclock);
	}

	if (client->cleanup != NULL) {
		(client->cleanup)(client);
		client->cleanup = NULL;
	}

	if (client->view != NULL) {
#ifdef ENABLE_AFL
		if (client->manager->sctx->fuzztype == isc_fuzz_resolver) {
			dns_adb_t *adb = NULL;
			dns_view_getadb(client->view, &adb);
			if (adb != NULL) {
				dns_adb_flush(adb);
				dns_adb_detach(&adb);
			}
		}
#endif /* ifdef ENABLE_AFL */
		dns_view_detach(&client->view);
	}
	if (client->opt != NULL) {
		INSIST(dns_rdataset_isassociated(client->opt));
		dns_rdataset_disassociate(client->opt);
		dns_message_puttemprdataset(client->message, &client->opt);
	}

	client_extendederror_reset(client);
	client->signer = NULL;
	client->udpsize = 512;
	client->extflags = 0;
	client->ednsversion = -1;
	client->additionaldepth = 0;
	dns_ecs_init(&client->ecs);
	dns_message_reset(client->message, DNS_MESSAGE_INTENTPARSE);

	/*
	 * Clear all client attributes that are specific to the request
	 */
	client->attributes = 0;
#ifdef ENABLE_AFL
	if (client->manager->sctx->fuzznotify != NULL &&
	    (client->manager->sctx->fuzztype == isc_fuzz_client ||
	     client->manager->sctx->fuzztype == isc_fuzz_tcpclient ||
	     client->manager->sctx->fuzztype == isc_fuzz_resolver))
	{
		client->manager->sctx->fuzznotify();
	}
#endif /* ENABLE_AFL */
}

void
ns_client_drop(ns_client_t *client, isc_result_t result) {
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(client->state == NS_CLIENTSTATE_WORKING ||
		client->state == NS_CLIENTSTATE_RECURSING);

	CTRACE("drop");
	if (result != ISC_R_SUCCESS) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request failed: %s", isc_result_totext(result));
	}
}

static void
client_senddone(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	ns_client_t *client = cbarg;

	REQUIRE(client->sendhandle == handle);

	CTRACE("senddone");

	/*
	 * Set sendhandle to NULL, but don't detach it immediately, in
	 * case we need to retry the send. If we do resend, then
	 * sendhandle will be reattached. Whether or not we resend,
	 * we will then detach the handle from *this* send by detaching
	 * 'handle' directly below.
	 */
	client->sendhandle = NULL;

	if (result != ISC_R_SUCCESS) {
		if (!TCP_CLIENT(client) && result == ISC_R_MAXSIZE) {
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
				      "send exceeded maximum size: truncating");
			client->query.attributes &= ~NS_QUERYATTR_ANSWERED;
			client->rcode_override = dns_rcode_noerror;
			ns_client_error(client, ISC_R_MAXSIZE);
		} else {
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
				      "send failed: %s",
				      isc_result_totext(result));
			isc_nm_bad_request(handle);
		}
	}

	isc_nmhandle_detach(&handle);
}

static void
client_setup_tcp_buffer(ns_client_t *client) {
	REQUIRE(client->tcpbuf == NULL);

	client->tcpbuf = client->manager->tcp_buffer;
	client->tcpbuf_size = NS_CLIENT_TCP_BUFFER_SIZE;
}

static void
client_put_tcp_buffer(ns_client_t *client) {
	if (client->tcpbuf == NULL) {
		return;
	}

	if (client->tcpbuf != client->manager->tcp_buffer) {
		isc_mem_put(client->manager->mctx, client->tcpbuf,
			    client->tcpbuf_size);
	}

	client->tcpbuf = NULL;
	client->tcpbuf_size = 0;
}

static void
client_allocsendbuf(ns_client_t *client, isc_buffer_t *buffer,
		    unsigned char **datap) {
	unsigned char *data;
	uint32_t bufsize;

	REQUIRE(datap != NULL);

	if (TCP_CLIENT(client)) {
		client_setup_tcp_buffer(client);
		data = client->tcpbuf;
		isc_buffer_init(buffer, data, client->tcpbuf_size);
	} else {
		data = client->sendbuf;
		if ((client->attributes & NS_CLIENTATTR_HAVECOOKIE) == 0) {
			if (client->view != NULL) {
				bufsize = client->view->nocookieudp;
			} else {
				bufsize = 512;
			}
		} else {
			bufsize = client->udpsize;
		}
		if (bufsize > client->udpsize) {
			bufsize = client->udpsize;
		}
		if (bufsize > NS_CLIENT_SEND_BUFFER_SIZE) {
			bufsize = NS_CLIENT_SEND_BUFFER_SIZE;
		}
		isc_buffer_init(buffer, data, bufsize);
	}
	*datap = data;
}

static void
client_sendpkg(ns_client_t *client, isc_buffer_t *buffer) {
	isc_result_t result;
	isc_region_t r;
	dns_ttl_t min_ttl = 0;

	REQUIRE(client->sendhandle == NULL);

	if (isc_buffer_base(buffer) == client->tcpbuf) {
		size_t used = isc_buffer_usedlength(buffer);
		INSIST(client->tcpbuf_size == NS_CLIENT_TCP_BUFFER_SIZE);

		/*
		 * Copy the data into a smaller buffer before sending,
		 * and keep the original big TCP send buffer for reuse
		 * by other clients.
		 */
		if (used > NS_CLIENT_SEND_BUFFER_SIZE) {
			/*
			 * We can save space by allocating a new buffer with a
			 * correct size and freeing the big buffer.
			 */
			unsigned char *new_tcpbuf =
				isc_mem_get(client->manager->mctx, used);
			memmove(new_tcpbuf, buffer->base, used);

			/*
			 * Put the big buffer so we can replace the pointer
			 * and the size with the new ones.
			 */
			client_put_tcp_buffer(client);

			/*
			 * Keep the new buffer's information so it can be freed.
			 */
			client->tcpbuf = new_tcpbuf;
			client->tcpbuf_size = used;

			r.base = new_tcpbuf;
		} else {
			/*
			 * The data fits in the available space in
			 * 'sendbuf', there is no need for a new buffer.
			 */
			memmove(client->sendbuf, buffer->base, used);

			/*
			 * Put the big buffer, we don't need a dynamic buffer.
			 */
			client_put_tcp_buffer(client);

			r.base = client->sendbuf;
		}
		r.length = used;
	} else {
		isc_buffer_usedregion(buffer, &r);
	}
	isc_nmhandle_attach(client->handle, &client->sendhandle);

	if (isc_nm_is_http_handle(client->handle)) {
		result = dns_message_response_minttl(client->message, &min_ttl);
		if (result == ISC_R_SUCCESS) {
			isc_nm_set_maxage(client->handle, min_ttl);
		}
	}
	isc_nm_send(client->handle, &r, client_senddone, client);
}

void
ns_client_sendraw(ns_client_t *client, dns_message_t *message) {
	isc_result_t result;
	unsigned char *data = NULL;
	isc_buffer_t buffer = { .magic = 0 };
	isc_region_t r;
	isc_region_t *mr = NULL;
#ifdef HAVE_DNSTAP
	dns_transport_type_t transport_type;
	dns_dtmsgtype_t dtmsgtype;
#endif

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE("sendraw");

	mr = dns_message_getrawmessage(message);
	if (mr == NULL) {
		result = ISC_R_UNEXPECTEDEND;
		goto done;
	}

	client_allocsendbuf(client, &buffer, &data);

	if (mr->length > isc_buffer_length(&buffer)) {
		result = ISC_R_NOSPACE;
		goto done;
	}

	/*
	 * Copy message to buffer and fixup id.
	 */
	isc_buffer_availableregion(&buffer, &r);
	result = isc_buffer_copyregion(&buffer, mr);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}
	r.base[0] = (client->message->id >> 8) & 0xff;
	r.base[1] = client->message->id & 0xff;

#ifdef HAVE_DNSTAP
	if (client->view != NULL) {
		transport_type = ns_client_transport_type(client);

		if (client->message->opcode == dns_opcode_update) {
			dtmsgtype = DNS_DTTYPE_UR;
		} else if ((client->message->flags & DNS_MESSAGEFLAG_RD) != 0) {
			dtmsgtype = DNS_DTTYPE_CR;
		} else {
			dtmsgtype = DNS_DTTYPE_AR;
		}
		dns_dt_send(client->view, dtmsgtype, &client->peeraddr,
			    &client->destsockaddr, transport_type, NULL,
			    &client->requesttime, NULL, &buffer);
	}
#endif

	client_sendpkg(client, &buffer);

	return;
done:
	if (client->tcpbuf != NULL) {
		client_put_tcp_buffer(client);
	}

	ns_client_drop(client, result);
}

void
ns_client_send(ns_client_t *client) {
	isc_result_t result;
	unsigned char *data = NULL;
	isc_buffer_t buffer = { .magic = 0 };
	isc_region_t r;
	dns_compress_t cctx;
	unsigned int compflags;
	bool cleanup_cctx = false;
	unsigned int render_opts;
	unsigned int preferred_glue;
	bool opt_included = false;
	size_t respsize;
	dns_aclenv_t *env = NULL;
#ifdef HAVE_DNSTAP
	unsigned char zone[DNS_NAME_MAXWIRE];
	dns_transport_type_t transport_type;
	dns_dtmsgtype_t dtmsgtype;
	isc_region_t zr;
#endif /* HAVE_DNSTAP */

	REQUIRE(NS_CLIENT_VALID(client));

	if ((client->query.attributes & NS_QUERYATTR_ANSWERED) != 0) {
		return;
	}

	/*
	 * XXXWPK TODO
	 * Delay the response according to the -T delay option
	 */

	env = client->manager->aclenv;

	CTRACE("send");

	if (client->message->opcode == dns_opcode_query &&
	    (client->attributes & NS_CLIENTATTR_RA) != 0)
	{
		client->message->flags |= DNS_MESSAGEFLAG_RA;
	}

	if ((client->attributes & NS_CLIENTATTR_WANTDNSSEC) != 0) {
		render_opts = 0;
	} else {
		render_opts = DNS_MESSAGERENDER_OMITDNSSEC;
	}

	preferred_glue = 0;
	if (client->view != NULL) {
		if (client->view->preferred_glue == dns_rdatatype_a) {
			preferred_glue = DNS_MESSAGERENDER_PREFER_A;
		} else if (client->view->preferred_glue == dns_rdatatype_aaaa) {
			preferred_glue = DNS_MESSAGERENDER_PREFER_AAAA;
		}
	}
	if (preferred_glue == 0) {
		if (isc_sockaddr_pf(&client->peeraddr) == AF_INET) {
			preferred_glue = DNS_MESSAGERENDER_PREFER_A;
		} else {
			preferred_glue = DNS_MESSAGERENDER_PREFER_AAAA;
		}
	}

	/*
	 * Create an OPT for our reply.
	 */
	if ((client->attributes & NS_CLIENTATTR_WANTOPT) != 0) {
		result = ns_client_addopt(client, client->message,
					  &client->opt);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	client_allocsendbuf(client, &buffer, &data);
	compflags = 0;
	if (client->peeraddr_valid && client->view != NULL) {
		isc_netaddr_t netaddr;
		dns_name_t *name = NULL;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		if (client->message->tsigkey != NULL) {
			name = client->message->tsigkey->name;
		}

		if (client->view->nocasecompress == NULL ||
		    !dns_acl_allowed(&netaddr, name,
				     client->view->nocasecompress, env))
		{
			compflags |= DNS_COMPRESS_CASE;
		}

		if (!client->view->msgcompression) {
			compflags = DNS_COMPRESS_DISABLED;
		}
	}
	dns_compress_init(&cctx, client->manager->mctx, compflags);
	cleanup_cctx = true;

	result = dns_message_renderbegin(client->message, &cctx, &buffer);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if (client->opt != NULL) {
		result = dns_message_setopt(client->message, client->opt);
		opt_included = true;
		client->opt = NULL;
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}
	result = dns_message_rendersection(client->message,
					   DNS_SECTION_QUESTION, 0);
	if (result == ISC_R_NOSPACE) {
		client->message->flags |= DNS_MESSAGEFLAG_TC;
		goto renderend;
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	/*
	 * Stop after the question if TC was set for rate limiting.
	 */
	if ((client->message->flags & DNS_MESSAGEFLAG_TC) != 0) {
		goto renderend;
	}
	result = dns_message_rendersection(client->message, DNS_SECTION_ANSWER,
					   DNS_MESSAGERENDER_PARTIAL |
						   render_opts);
	if (result == ISC_R_NOSPACE) {
		client->message->flags |= DNS_MESSAGEFLAG_TC;
		goto renderend;
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_rendersection(
		client->message, DNS_SECTION_AUTHORITY,
		DNS_MESSAGERENDER_PARTIAL | render_opts);
	if (result == ISC_R_NOSPACE) {
		client->message->flags |= DNS_MESSAGEFLAG_TC;
		goto renderend;
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_rendersection(client->message,
					   DNS_SECTION_ADDITIONAL,
					   preferred_glue | render_opts);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOSPACE) {
		goto cleanup;
	}
renderend:
	result = dns_message_renderend(client->message);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

#ifdef HAVE_DNSTAP
	memset(&zr, 0, sizeof(zr));
	if (((client->message->flags & DNS_MESSAGEFLAG_AA) != 0) &&
	    (client->query.authzone != NULL))
	{
		isc_result_t eresult;
		isc_buffer_t b;
		dns_name_t *zo = dns_zone_getorigin(client->query.authzone);

		isc_buffer_init(&b, zone, sizeof(zone));
		dns_compress_setpermitted(&cctx, false);
		eresult = dns_name_towire(zo, &cctx, &b, NULL);
		if (eresult == ISC_R_SUCCESS) {
			isc_buffer_usedregion(&b, &zr);
		}
	}

	if (client->message->opcode == dns_opcode_update) {
		dtmsgtype = DNS_DTTYPE_UR;
	} else if ((client->message->flags & DNS_MESSAGEFLAG_RD) != 0) {
		dtmsgtype = DNS_DTTYPE_CR;
	} else {
		dtmsgtype = DNS_DTTYPE_AR;
	}

	transport_type = ns_client_transport_type(client);
#endif /* HAVE_DNSTAP */

	if (cleanup_cctx) {
		dns_compress_invalidate(&cctx);
	}

	if (client->sendcb != NULL) {
		client->sendcb(&buffer);
	} else if (TCP_CLIENT(client)) {
		isc_buffer_usedregion(&buffer, &r);
#ifdef HAVE_DNSTAP
		if (client->view != NULL) {
			dns_dt_send(client->view, dtmsgtype, &client->peeraddr,
				    &client->destsockaddr, transport_type, &zr,
				    &client->requesttime, NULL, &buffer);
		}
#endif /* HAVE_DNSTAP */

		respsize = isc_buffer_usedlength(&buffer);

		client_sendpkg(client, &buffer);

		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_histomulti_inc(client->manager->sctx->tcpoutstats4,
					   DNS_SIZEHISTO_BUCKETOUT(respsize));
			break;
		case AF_INET6:
			isc_histomulti_inc(client->manager->sctx->tcpoutstats6,
					   DNS_SIZEHISTO_BUCKETOUT(respsize));
			break;
		default:
			UNREACHABLE();
		}
	} else {
#ifdef HAVE_DNSTAP
		/*
		 * Log dnstap data first, because client_sendpkg() may
		 * leave client->view set to NULL.
		 */
		if (client->view != NULL) {
			dns_dt_send(client->view, dtmsgtype, &client->peeraddr,
				    &client->destsockaddr, transport_type, &zr,
				    &client->requesttime, NULL, &buffer);
		}
#endif /* HAVE_DNSTAP */

		respsize = isc_buffer_usedlength(&buffer);

		client_sendpkg(client, &buffer);

		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_histomulti_inc(client->manager->sctx->udpoutstats4,
					   DNS_SIZEHISTO_BUCKETOUT(respsize));
			break;
		case AF_INET6:
			isc_histomulti_inc(client->manager->sctx->udpoutstats6,
					   DNS_SIZEHISTO_BUCKETOUT(respsize));
			break;
		default:
			UNREACHABLE();
		}
	}

	/* update statistics (XXXJT: is it okay to access message->xxxkey?) */
	ns_stats_increment(client->manager->sctx->nsstats,
			   ns_statscounter_response);

	dns_rcodestats_increment(client->manager->sctx->rcodestats,
				 client->message->rcode);
	if (opt_included) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_edns0out);
	}
	if (client->message->tsigkey != NULL) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_tsigout);
	}
	if (client->message->sig0key != NULL) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_sig0out);
	}
	if ((client->message->flags & DNS_MESSAGEFLAG_TC) != 0) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_truncatedresp);
	}

	client->query.attributes |= NS_QUERYATTR_ANSWERED;

	return;

cleanup:
	if (client->tcpbuf != NULL) {
		client_put_tcp_buffer(client);
	}

	if (cleanup_cctx) {
		dns_compress_invalidate(&cctx);
	}
}

#if NS_CLIENT_DROPPORT
#define DROPPORT_NO	  0
#define DROPPORT_REQUEST  1
#define DROPPORT_RESPONSE 2
/*%
 * ns_client_dropport determines if certain requests / responses
 * should be dropped based on the port number.
 *
 * Returns:
 * \li	0:	Don't drop.
 * \li	1:	Drop request.
 * \li	2:	Drop (error) response.
 */
static int
ns_client_dropport(in_port_t port) {
	switch (port) {
	case 7:	 /* echo */
	case 13: /* daytime */
	case 19: /* chargen */
	case 37: /* time */
		return DROPPORT_REQUEST;
	case 464: /* kpasswd */
		return DROPPORT_RESPONSE;
	}
	return DROPPORT_NO;
}
#endif /* if NS_CLIENT_DROPPORT */

void
ns_client_error(ns_client_t *client, isc_result_t result) {
	dns_message_t *message = NULL;
	dns_rcode_t rcode;
	bool trunc = false;

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE("error");

	message = client->message;

	if (client->rcode_override == -1) {
		rcode = dns_result_torcode(result);
	} else {
		rcode = (dns_rcode_t)(client->rcode_override & 0xfff);
	}

	if (result == ISC_R_MAXSIZE) {
		trunc = true;
	}

#if NS_CLIENT_DROPPORT
	/*
	 * Don't send FORMERR to ports on the drop port list.
	 */
	if (rcode == dns_rcode_formerr &&
	    ns_client_dropport(isc_sockaddr_getport(&client->peeraddr)) !=
		    DROPPORT_NO)
	{
		char buf[64];
		isc_buffer_t b;

		isc_buffer_init(&b, buf, sizeof(buf) - 1);
		if (dns_rcode_totext(rcode, &b) != ISC_R_SUCCESS) {
			isc_buffer_putstr(&b, "UNKNOWN RCODE");
		}
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped error (%.*s) response: suspicious port",
			      (int)isc_buffer_usedlength(&b), buf);
		ns_client_drop(client, ISC_R_SUCCESS);
		return;
	}
#endif /* if NS_CLIENT_DROPPORT */

	/*
	 * Try to rate limit error responses.
	 */
	if (client->view != NULL && client->view->rrl != NULL) {
		bool wouldlog;
		char log_buf[DNS_RRL_LOG_BUF_LEN];
		dns_rrl_result_t rrl_result;
		int loglevel;

		if ((client->manager->sctx->options & NS_SERVER_LOGQUERIES) !=
		    0)
		{
			loglevel = DNS_RRL_LOG_DROP;
		} else {
			loglevel = ISC_LOG_DEBUG(1);
		}
		wouldlog = isc_log_wouldlog(ns_lctx, loglevel);
		rrl_result = dns_rrl(client->view, NULL, &client->peeraddr,
				     TCP_CLIENT(client), dns_rdataclass_in,
				     dns_rdatatype_none, NULL, result,
				     client->now, wouldlog, log_buf,
				     sizeof(log_buf));
		if (rrl_result != DNS_RRL_RESULT_OK) {
			/*
			 * Log dropped errors in the query category
			 * so that they are not lost in silence.
			 * Starts of rate-limited bursts are logged in
			 * NS_LOGCATEGORY_RRL.
			 */
			if (wouldlog) {
				ns_client_log(client,
					      NS_LOGCATEGORY_QUERY_ERRORS,
					      NS_LOGMODULE_CLIENT, loglevel,
					      "%s", log_buf);
			}
			/*
			 * Some error responses cannot be 'slipped',
			 * so don't try to slip any error responses.
			 */
			if (!client->view->rrl->log_only) {
				ns_stats_increment(
					client->manager->sctx->nsstats,
					ns_statscounter_ratedropped);
				ns_stats_increment(
					client->manager->sctx->nsstats,
					ns_statscounter_dropped);
				ns_client_drop(client, DNS_R_DROP);
				return;
			}
		}
	}

	/*
	 * Message may be an in-progress reply that we had trouble
	 * with, in which case QR will be set.  We need to clear QR before
	 * calling dns_message_reply() to avoid triggering an assertion.
	 */
	message->flags &= ~DNS_MESSAGEFLAG_QR;
	/*
	 * AA and AD shouldn't be set.
	 */
	message->flags &= ~(DNS_MESSAGEFLAG_AA | DNS_MESSAGEFLAG_AD);
	result = dns_message_reply(message, true);
	if (result != ISC_R_SUCCESS) {
		/*
		 * It could be that we've got a query with a good header,
		 * but a bad question section, so we try again with
		 * want_question_section set to false.
		 */
		result = dns_message_reply(message, false);
		if (result != ISC_R_SUCCESS) {
			ns_client_drop(client, result);
			return;
		}
	}

	message->rcode = rcode;
	if (trunc) {
		message->flags |= DNS_MESSAGEFLAG_TC;
	}

	if (rcode == dns_rcode_formerr) {
		/*
		 * FORMERR loop avoidance:  If we sent a FORMERR message
		 * with the same ID to the same client less than two
		 * seconds ago, assume that we are in an infinite error
		 * packet dialog with a server for some protocol whose
		 * error responses look enough like DNS queries to
		 * elicit a FORMERR response.  Drop a packet to break
		 * the loop.
		 */
		if (isc_sockaddr_equal(&client->peeraddr,
				       &client->formerrcache.addr) &&
		    message->id == client->formerrcache.id &&
		    (isc_time_seconds(&client->requesttime) -
		     client->formerrcache.time) < 2)
		{
			/* Drop packet. */
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
				      "possible error packet loop, "
				      "FORMERR dropped");
			ns_client_drop(client, result);
			return;
		}
		client->formerrcache.addr = client->peeraddr;
		client->formerrcache.time =
			isc_time_seconds(&client->requesttime);
		client->formerrcache.id = message->id;
	} else if (rcode == dns_rcode_servfail && client->query.qname != NULL &&
		   client->view != NULL && client->view->fail_ttl != 0 &&
		   ((client->attributes & NS_CLIENTATTR_NOSETFC) == 0))
	{
		/*
		 * SERVFAIL caching: store qname/qtype of failed queries
		 */
		isc_time_t expire;
		isc_interval_t i;
		uint32_t flags = 0;

		if ((message->flags & DNS_MESSAGEFLAG_CD) != 0) {
			flags = NS_FAILCACHE_CD;
		}

		isc_interval_set(&i, client->view->fail_ttl, 0);
		result = isc_time_nowplusinterval(&expire, &i);
		if (result == ISC_R_SUCCESS) {
			dns_badcache_add(client->view->failcache,
					 client->query.qname,
					 client->query.qtype, flags,
					 isc_time_seconds(&expire));
		}
	}

	ns_client_send(client);
}

isc_result_t
ns_client_addopt(ns_client_t *client, dns_message_t *message,
		 dns_rdataset_t **opt) {
	unsigned char ecs[ECS_SIZE];
	char nsid[_POSIX_HOST_NAME_MAX + 1], *nsidp = NULL;
	unsigned char cookie[COOKIE_SIZE];
	isc_result_t result;
	dns_view_t *view = NULL;
	uint16_t udpsize;
	dns_ednsopt_t ednsopts[DNS_EDNSOPTIONS];
	int count = 0;
	unsigned int flags;
	unsigned char expire[4];
	unsigned char advtimo[2];
	dns_aclenv_t *env = NULL;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(opt != NULL && *opt == NULL);
	REQUIRE(message != NULL);

	env = client->manager->aclenv;
	view = client->view;
	if (view != NULL) {
		udpsize = dns_view_getudpsize(view);
	} else {
		udpsize = client->manager->sctx->udpsize;
	}

	flags = client->extflags & DNS_MESSAGEEXTFLAG_REPLYPRESERVE;

	/* Set EDNS options if applicable */
	if (WANTNSID(client)) {
		if (client->manager->sctx->server_id != NULL) {
			nsidp = client->manager->sctx->server_id;
		} else if (client->manager->sctx->usehostname) {
			if (gethostname(nsid, sizeof(nsid)) != 0) {
				goto no_nsid;
			}
			nsidp = nsid;
		} else {
			goto no_nsid;
		}

		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_NSID;
		ednsopts[count].length = (uint16_t)strlen(nsidp);
		ednsopts[count].value = (unsigned char *)nsidp;
		count++;
	}
no_nsid:
	if ((client->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0) {
		isc_buffer_t buf;
		isc_stdtime_t now = isc_stdtime_now();

		isc_buffer_init(&buf, cookie, sizeof(cookie));

		compute_cookie(client, now, client->manager->sctx->secret,
			       &buf);

		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_COOKIE;
		ednsopts[count].length = COOKIE_SIZE;
		ednsopts[count].value = cookie;
		count++;
	}
	if ((client->attributes & NS_CLIENTATTR_HAVEEXPIRE) != 0) {
		isc_buffer_t buf;

		INSIST(count < DNS_EDNSOPTIONS);

		isc_buffer_init(&buf, expire, sizeof(expire));
		isc_buffer_putuint32(&buf, client->expire);
		ednsopts[count].code = DNS_OPT_EXPIRE;
		ednsopts[count].length = 4;
		ednsopts[count].value = expire;
		count++;
	}
	if (((client->attributes & NS_CLIENTATTR_HAVEECS) != 0) &&
	    (client->ecs.addr.family == AF_INET ||
	     client->ecs.addr.family == AF_INET6 ||
	     client->ecs.addr.family == AF_UNSPEC))
	{
		isc_buffer_t buf;
		uint8_t addr[16];
		uint32_t plen, addrl;
		uint16_t family = 0;

		/* Add CLIENT-SUBNET option. */

		plen = client->ecs.source;

		/* Round up prefix len to a multiple of 8 */
		addrl = (plen + 7) / 8;

		switch (client->ecs.addr.family) {
		case AF_UNSPEC:
			INSIST(plen == 0);
			family = 0;
			break;
		case AF_INET:
			INSIST(plen <= 32);
			family = 1;
			memmove(addr, &client->ecs.addr.type, addrl);
			break;
		case AF_INET6:
			INSIST(plen <= 128);
			family = 2;
			memmove(addr, &client->ecs.addr.type, addrl);
			break;
		default:
			UNREACHABLE();
		}

		isc_buffer_init(&buf, ecs, sizeof(ecs));
		/* family */
		isc_buffer_putuint16(&buf, family);
		/* source prefix-length */
		isc_buffer_putuint8(&buf, client->ecs.source);
		/* scope prefix-length */
		isc_buffer_putuint8(&buf, client->ecs.scope);

		/* address */
		if (addrl > 0) {
			/* Mask off last address byte */
			if ((plen % 8) != 0) {
				addr[addrl - 1] &= ~0U << (8 - (plen % 8));
			}
			isc_buffer_putmem(&buf, addr, (unsigned int)addrl);
		}

		ednsopts[count].code = DNS_OPT_CLIENT_SUBNET;
		ednsopts[count].length = addrl + 4;
		ednsopts[count].value = ecs;
		count++;
	}
	if (TCP_CLIENT(client) && USEKEEPALIVE(client)) {
		isc_buffer_t buf;
		uint32_t adv;

		INSIST(count < DNS_EDNSOPTIONS);

		isc_nm_gettimeouts(isc_nmhandle_netmgr(client->handle), NULL,
				   NULL, NULL, &adv);
		adv /= 100; /* units of 100 milliseconds */
		isc_buffer_init(&buf, advtimo, sizeof(advtimo));
		isc_buffer_putuint16(&buf, (uint16_t)adv);
		ednsopts[count].code = DNS_OPT_TCP_KEEPALIVE;
		ednsopts[count].length = 2;
		ednsopts[count].value = advtimo;
		count++;
	}

	if (client->ede != NULL) {
		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_EDE;
		ednsopts[count].length = client->ede->length;
		ednsopts[count].value = client->ede->value;
		count++;
	}

	/* Padding must be added last */
	if ((view != NULL) && (view->padding > 0) && WANTPAD(client) &&
	    (TCP_CLIENT(client) ||
	     ((client->attributes & NS_CLIENTATTR_HAVECOOKIE) != 0)))
	{
		isc_netaddr_t netaddr;
		int match;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		result = dns_acl_match(&netaddr, NULL, view->pad_acl, env,
				       &match, NULL);
		if (result == ISC_R_SUCCESS && match > 0) {
			INSIST(count < DNS_EDNSOPTIONS);

			ednsopts[count].code = DNS_OPT_PAD;
			ednsopts[count].length = 0;
			ednsopts[count].value = NULL;
			count++;

			dns_message_setpadding(message, view->padding);
		}
	}

	result = dns_message_buildopt(message, opt, 0, udpsize, flags, ednsopts,
				      count);
	return result;
}

static void
compute_cookie(ns_client_t *client, uint32_t when, const unsigned char *secret,
	       isc_buffer_t *buf) {
	unsigned char digest[ISC_MAX_MD_SIZE] ISC_NONSTRING = { 0 };
	STATIC_ASSERT(ISC_MAX_MD_SIZE >= ISC_SIPHASH24_TAG_LENGTH,
		      "You need to increase the digest buffer.");

	switch (client->manager->sctx->cookiealg) {
	case ns_cookiealg_siphash24: {
		unsigned char input[16 + 16] ISC_NONSTRING = { 0 };
		size_t inputlen = 0;
		isc_netaddr_t netaddr;
		unsigned char *cp;

		isc_buffer_putmem(buf, client->cookie, 8);
		isc_buffer_putuint8(buf, NS_COOKIE_VERSION_1);
		isc_buffer_putuint8(buf, 0);  /* Reserved */
		isc_buffer_putuint16(buf, 0); /* Reserved */
		isc_buffer_putuint32(buf, when);

		memmove(input, (unsigned char *)isc_buffer_used(buf) - 16, 16);

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		switch (netaddr.family) {
		case AF_INET:
			cp = (unsigned char *)&netaddr.type.in;
			memmove(input + 16, cp, 4);
			inputlen = 20;
			break;
		case AF_INET6:
			cp = (unsigned char *)&netaddr.type.in6;
			memmove(input + 16, cp, 16);
			inputlen = 32;
			break;
		default:
			UNREACHABLE();
		}

		isc_siphash24(secret, input, inputlen, true, digest);
		isc_buffer_putmem(buf, digest, 8);
		break;
	}
	default:
		UNREACHABLE();
	}
}

static void
process_cookie(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {
	ns_altsecret_t *altsecret;
	unsigned char dbuf[COOKIE_SIZE];
	unsigned char *old;
	isc_stdtime_t now;
	uint32_t when;
	isc_buffer_t db;

	/*
	 * If we have already seen a cookie option skip this cookie option.
	 */
	if ((!client->manager->sctx->answercookie) ||
	    (client->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0)
	{
		isc_buffer_forward(buf, (unsigned int)optlen);
		return;
	}

	client->attributes |= NS_CLIENTATTR_WANTCOOKIE;

	ns_stats_increment(client->manager->sctx->nsstats,
			   ns_statscounter_cookiein);

	if (optlen != COOKIE_SIZE) {
		/*
		 * Not our token.
		 */
		INSIST(optlen >= 8U);
		memmove(client->cookie, isc_buffer_current(buf), 8);
		isc_buffer_forward(buf, (unsigned int)optlen);

		if (optlen == 8U) {
			ns_stats_increment(client->manager->sctx->nsstats,
					   ns_statscounter_cookienew);
		} else {
			ns_stats_increment(client->manager->sctx->nsstats,
					   ns_statscounter_cookiebadsize);
			client->attributes |= NS_CLIENTATTR_BADCOOKIE;
		}
		return;
	}

	/*
	 * Process all of the incoming buffer.
	 */
	old = isc_buffer_current(buf);
	memmove(client->cookie, old, 8);
	isc_buffer_forward(buf, 8);
	isc_buffer_forward(buf, 4); /* version + reserved */
	when = isc_buffer_getuint32(buf);
	isc_buffer_forward(buf, 8);

	/*
	 * Allow for a 5 minute clock skew between servers sharing a secret.
	 * Only accept COOKIE if we have talked to the client in the last hour.
	 */
	now = isc_stdtime_now();
	if (isc_serial_gt(when, (now + 300)) /* In the future. */ ||
	    isc_serial_lt(when, (now - 3600)) /* In the past. */)
	{
		client->attributes |= NS_CLIENTATTR_BADCOOKIE;
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_cookiebadtime);
		return;
	}

	isc_buffer_init(&db, dbuf, sizeof(dbuf));
	compute_cookie(client, when, client->manager->sctx->secret, &db);

	if (isc_safe_memequal(old, dbuf, COOKIE_SIZE)) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_cookiematch);
		client->attributes |= NS_CLIENTATTR_HAVECOOKIE;
		return;
	}

	for (altsecret = ISC_LIST_HEAD(client->manager->sctx->altsecrets);
	     altsecret != NULL; altsecret = ISC_LIST_NEXT(altsecret, link))
	{
		isc_buffer_init(&db, dbuf, sizeof(dbuf));
		compute_cookie(client, when, altsecret->secret, &db);
		if (isc_safe_memequal(old, dbuf, COOKIE_SIZE)) {
			ns_stats_increment(client->manager->sctx->nsstats,
					   ns_statscounter_cookiematch);
			client->attributes |= NS_CLIENTATTR_HAVECOOKIE;
			return;
		}
	}

	client->attributes |= NS_CLIENTATTR_BADCOOKIE;
	ns_stats_increment(client->manager->sctx->nsstats,
			   ns_statscounter_cookienomatch);
}

static isc_result_t
process_ecs(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {
	uint16_t family;
	uint8_t addrlen, addrbytes, scope, *paddr;
	isc_netaddr_t caddr;

	/*
	 * If we have already seen a ECS option skip this ECS option.
	 */
	if ((client->attributes & NS_CLIENTATTR_HAVEECS) != 0) {
		isc_buffer_forward(buf, (unsigned int)optlen);
		return ISC_R_SUCCESS;
	}

	/*
	 * XXXMUKS: Is there any need to repeat these checks here
	 * (except query's scope length) when they are done in the OPT
	 * RDATA fromwire code?
	 */

	if (optlen < 4U) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option too short");
		return DNS_R_FORMERR;
	}

	family = isc_buffer_getuint16(buf);
	addrlen = isc_buffer_getuint8(buf);
	scope = isc_buffer_getuint8(buf);
	optlen -= 4;

	if (scope != 0U) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option: invalid scope");
		return DNS_R_OPTERR;
	}

	memset(&caddr, 0, sizeof(caddr));
	switch (family) {
	case 0:
		/*
		 * XXXMUKS: In queries, if FAMILY is set to 0, SOURCE
		 * PREFIX-LENGTH must be 0 and ADDRESS should not be
		 * present as the address and prefix lengths don't make
		 * sense because the family is unknown.
		 */
		if (addrlen != 0U) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
				      "EDNS client-subnet option: invalid "
				      "address length (%u) for FAMILY=0",
				      addrlen);
			return DNS_R_OPTERR;
		}
		caddr.family = AF_UNSPEC;
		break;
	case 1:
		if (addrlen > 32U) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
				      "EDNS client-subnet option: invalid "
				      "address length (%u) for IPv4",
				      addrlen);
			return DNS_R_OPTERR;
		}
		caddr.family = AF_INET;
		break;
	case 2:
		if (addrlen > 128U) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
				      "EDNS client-subnet option: invalid "
				      "address length (%u) for IPv6",
				      addrlen);
			return DNS_R_OPTERR;
		}
		caddr.family = AF_INET6;
		break;
	default:
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option: invalid family");
		return DNS_R_OPTERR;
	}

	addrbytes = (addrlen + 7) / 8;
	if (isc_buffer_remaininglength(buf) < addrbytes) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option: address too short");
		return DNS_R_OPTERR;
	}

	paddr = (uint8_t *)&caddr.type;
	if (addrbytes != 0U) {
		memmove(paddr, isc_buffer_current(buf), addrbytes);
		isc_buffer_forward(buf, addrbytes);
		optlen -= addrbytes;

		if ((addrlen % 8) != 0) {
			uint8_t bits = ~0U << (8 - (addrlen % 8));
			bits &= paddr[addrbytes - 1];
			if (bits != paddr[addrbytes - 1]) {
				return DNS_R_OPTERR;
			}
		}
	}

	memmove(&client->ecs.addr, &caddr, sizeof(caddr));
	client->ecs.source = addrlen;
	client->ecs.scope = 0;
	client->attributes |= NS_CLIENTATTR_HAVEECS;

	isc_buffer_forward(buf, (unsigned int)optlen);
	return ISC_R_SUCCESS;
}

static isc_result_t
process_keytag(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {
	if (optlen == 0 || (optlen % 2) != 0) {
		isc_buffer_forward(buf, (unsigned int)optlen);
		return DNS_R_OPTERR;
	}

	/* Silently drop additional keytag options. */
	if (client->keytag != NULL) {
		isc_buffer_forward(buf, (unsigned int)optlen);
		return ISC_R_SUCCESS;
	}

	client->keytag = isc_mem_get(client->manager->mctx, optlen);
	{
		client->keytag_len = (uint16_t)optlen;
		memmove(client->keytag, isc_buffer_current(buf), optlen);
	}
	isc_buffer_forward(buf, (unsigned int)optlen);
	return ISC_R_SUCCESS;
}

static isc_result_t
process_opt(ns_client_t *client, dns_rdataset_t *opt) {
	dns_rdata_t rdata;
	isc_buffer_t optbuf;
	isc_result_t result;
	uint16_t optcode;
	uint16_t optlen;

	/*
	 * Set the client's UDP buffer size.
	 */
	client->udpsize = opt->rdclass;

	/*
	 * If the requested UDP buffer size is less than 512,
	 * ignore it and use 512.
	 */
	if (client->udpsize < 512) {
		client->udpsize = 512;
	}

	/*
	 * Get the flags out of the OPT record.
	 */
	client->extflags = (uint16_t)(opt->ttl & 0xFFFF);

	/*
	 * Do we understand this version of EDNS?
	 *
	 * XXXRTH need library support for this!
	 */
	client->ednsversion = (opt->ttl & 0x00FF0000) >> 16;
	if (client->ednsversion > DNS_EDNS_VERSION) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_badednsver);
		result = ns_client_addopt(client, client->message,
					  &client->opt);
		if (result == ISC_R_SUCCESS) {
			result = DNS_R_BADVERS;
		}
		ns_client_error(client, result);
		return result;
	}

	/* Check for NSID request */
	result = dns_rdataset_first(opt);
	if (result == ISC_R_SUCCESS) {
		dns_rdata_init(&rdata);
		dns_rdataset_current(opt, &rdata);
		isc_buffer_init(&optbuf, rdata.data, rdata.length);
		isc_buffer_add(&optbuf, rdata.length);
		while (isc_buffer_remaininglength(&optbuf) >= 4) {
			optcode = isc_buffer_getuint16(&optbuf);
			optlen = isc_buffer_getuint16(&optbuf);
			switch (optcode) {
			case DNS_OPT_NSID:
				if (!WANTNSID(client)) {
					ns_stats_increment(
						client->manager->sctx->nsstats,
						ns_statscounter_nsidopt);
				}
				client->attributes |= NS_CLIENTATTR_WANTNSID;
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_COOKIE:
				process_cookie(client, &optbuf, optlen);
				break;
			case DNS_OPT_EXPIRE:
				if (!WANTEXPIRE(client)) {
					ns_stats_increment(
						client->manager->sctx->nsstats,
						ns_statscounter_expireopt);
				}
				client->attributes |= NS_CLIENTATTR_WANTEXPIRE;
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_CLIENT_SUBNET:
				result = process_ecs(client, &optbuf, optlen);
				if (result != ISC_R_SUCCESS) {
					ns_client_error(client, result);
					return result;
				}
				ns_stats_increment(
					client->manager->sctx->nsstats,
					ns_statscounter_ecsopt);
				break;
			case DNS_OPT_TCP_KEEPALIVE:
				if (!USEKEEPALIVE(client)) {
					ns_stats_increment(
						client->manager->sctx->nsstats,
						ns_statscounter_keepaliveopt);
				}
				client->attributes |=
					NS_CLIENTATTR_USEKEEPALIVE;
				isc_nmhandle_keepalive(client->handle, true);
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_PAD:
				client->attributes |= NS_CLIENTATTR_WANTPAD;
				ns_stats_increment(
					client->manager->sctx->nsstats,
					ns_statscounter_padopt);
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_KEY_TAG:
				result = process_keytag(client, &optbuf,
							optlen);
				if (result != ISC_R_SUCCESS) {
					ns_client_error(client, result);
					return result;
				}
				ns_stats_increment(
					client->manager->sctx->nsstats,
					ns_statscounter_keytagopt);
				break;
			default:
				ns_stats_increment(
					client->manager->sctx->nsstats,
					ns_statscounter_otheropt);
				isc_buffer_forward(&optbuf, optlen);
				break;
			}
		}
	}

	ns_stats_increment(client->manager->sctx->nsstats,
			   ns_statscounter_edns0in);
	client->attributes |= NS_CLIENTATTR_WANTOPT;

	return result;
}

static void
ns_client_async_reset(ns_client_t *client) {
	if (client->async) {
		client->async = false;
		if (client->handle != NULL) {
			isc_nmhandle_unref(client->handle);
		}
	}
}

void
ns__client_reset_cb(void *client0) {
	ns_client_t *client = client0;

	ns_client_log(client, DNS_LOGCATEGORY_SECURITY, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(3), "reset client");

	/*
	 * We never started processing this client, possible if we're
	 * shutting down, just exit.
	 */
	if (client->state == NS_CLIENTSTATE_READY) {
		return;
	}

	ns_client_endrequest(client);
	if (client->tcpbuf != NULL) {
		client_put_tcp_buffer(client);
	}

	if (client->keytag != NULL) {
		isc_mem_put(client->manager->mctx, client->keytag,
			    client->keytag_len);
		client->keytag_len = 0;
	}

	ns_client_async_reset(client);

	client->state = NS_CLIENTSTATE_READY;

#ifdef WANT_SINGLETRACE
	isc_log_setforcelog(false);
#endif /* WANT_SINGLETRACE */
}

void
ns__client_put_cb(void *client0) {
	ns_client_t *client = client0;
	ns_clientmgr_t *manager = NULL;

	REQUIRE(NS_CLIENT_VALID(client));

	manager = client->manager;

	ns_client_log(client, DNS_LOGCATEGORY_SECURITY, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(3), "freeing client");

	/*
	 * Call this first because it requires a valid client.
	 */
	ns_query_free(client);
	client_extendederror_reset(client);

	client->magic = 0;

	if (client->opt != NULL) {
		INSIST(dns_rdataset_isassociated(client->opt));
		dns_rdataset_disassociate(client->opt);
		dns_message_puttemprdataset(client->message, &client->opt);
	}

	ns_client_async_reset(client);

	dns_message_detach(&client->message);

	/*
	 * Destroy the fetchlock mutex that was created in
	 * ns_query_init().
	 */
	isc_mutex_destroy(&client->query.fetchlock);

	isc_mem_put(manager->mctx, client, sizeof(*client));

	ns_clientmgr_detach(&manager);
}

static isc_result_t
ns_client_setup_view(ns_client_t *client, isc_netaddr_t *netaddr) {
	isc_result_t result;

	client->sigresult = client->viewmatchresult = ISC_R_UNSET;

	if (client->async) {
		isc_nmhandle_ref(client->handle);
	}

	result = client->manager->sctx->matchingview(
		netaddr, &client->destaddr, client->message,
		client->manager->aclenv, client->manager->sctx,
		client->async ? client->manager->loop : NULL,
		ns_client_request_continue, client, &client->sigresult,
		&client->viewmatchresult, &client->view);

	/* Async mode. */
	if (result == DNS_R_WAIT) {
		INSIST(client->async == true);
		return DNS_R_WAIT;
	}

	/*
	 * matchingview() returning anything other than DNS_R_WAIT means it's
	 * not running in async mode, in which case 'result' must be equal to
	 * 'client->viewmatchresult'.
	 */
	INSIST(result == client->viewmatchresult);

	/* Non-async mode. */
	ns_client_async_reset(client);

	return result;
}

/*
 * Handle an incoming request event from the socket (UDP case)
 * or tcpmsg (TCP case).
 */
void
ns_client_request(isc_nmhandle_t *handle, isc_result_t eresult,
		  isc_region_t *region, void *arg) {
	ns_client_t *client = NULL;
	isc_result_t result;
	dns_rdataset_t *opt = NULL;
	isc_netaddr_t netaddr;
	int match;
	dns_messageid_t id;
	unsigned int flags;
	bool notimp;
	size_t reqsize;
	dns_aclenv_t *env = NULL;

	if (eresult != ISC_R_SUCCESS) {
		return;
	}

	client = isc_nmhandle_getdata(handle);
	if (client == NULL) {
		ns_interface_t *ifp = (ns_interface_t *)arg;
		ns_clientmgr_t *clientmgr =
			ns_interfacemgr_getclientmgr(ifp->mgr);

		INSIST(VALID_MANAGER(clientmgr));
		INSIST(clientmgr->tid == isc_tid());

		client = isc_mem_get(clientmgr->mctx, sizeof(*client));

		ns__client_setup(client, clientmgr, true);

		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "allocate new client");
	} else {
		ns__client_setup(client, NULL, false);
	}

	client->state = NS_CLIENTSTATE_READY;

	if (client->handle == NULL) {
		isc_nmhandle_setdata(handle, client, ns__client_reset_cb,
				     ns__client_put_cb);
		client->handle = handle;
	}

	if (isc_nmhandle_is_stream(handle)) {
		client->attributes |= NS_CLIENTATTR_TCP;
	}

	INSIST(client->state == NS_CLIENTSTATE_READY);

	(void)atomic_fetch_add_relaxed(&ns_client_requests, 1);

	isc_buffer_init(&client->tbuffer, region->base, region->length);
	isc_buffer_add(&client->tbuffer, region->length);
	client->buffer = &client->tbuffer;

	client->peeraddr = isc_nmhandle_peeraddr(handle);
	client->peeraddr_valid = true;

	reqsize = isc_buffer_usedlength(client->buffer);

	client->state = NS_CLIENTSTATE_WORKING;

	client->requesttime = isc_time_now();
	client->tnow = client->requesttime;
	client->now = isc_time_seconds(&client->tnow);

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

#if NS_CLIENT_DROPPORT
	if (ns_client_dropport(isc_sockaddr_getport(&client->peeraddr)) ==
	    DROPPORT_REQUEST)
	{
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped request: suspicious port");
		isc_nm_bad_request(handle);
		return;
	}
#endif /* if NS_CLIENT_DROPPORT */

	env = client->manager->aclenv;
	if (client->manager->sctx->blackholeacl != NULL &&
	    (dns_acl_match(&netaddr, NULL, client->manager->sctx->blackholeacl,
			   env, &match, NULL) == ISC_R_SUCCESS) &&
	    match > 0)
	{
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped request: blackholed peer");
		isc_nm_bad_request(handle);
		return;
	}

	ns_client_log(client, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(3), "%s request",
		      TCP_CLIENT(client) ? "TCP" : "UDP");

	result = dns_message_peekheader(client->buffer, &id, &flags);
	if (result != ISC_R_SUCCESS) {
		/*
		 * There isn't enough header to determine whether
		 * this was a request or a response.  Drop it.
		 */
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped request: invalid message header");
		isc_nm_bad_request(handle);
		return;
	}

#ifdef WANT_SINGLETRACE
	if (id == 0) {
		isc_log_setforcelog(true);
	}
#endif /* WANT_SINGLETRACE */

	/*
	 * The client object handles requests, not responses.
	 * If this is a UDP response, forward it to the dispatcher.
	 * If it's a TCP response, discard it here.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) != 0) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped request: unexpected response");
		isc_nm_bad_request(handle);
		return;
	}

	/*
	 * Update some statistics counters.  Don't count responses.
	 */
	if (isc_sockaddr_pf(&client->peeraddr) == PF_INET) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_requestv4);
	} else {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_requestv6);
	}
	if (TCP_CLIENT(client)) {
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_requesttcp);
		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_histomulti_inc(client->manager->sctx->tcpinstats4,
					   DNS_SIZEHISTO_BUCKETIN(reqsize));
			break;
		case AF_INET6:
			isc_histomulti_inc(client->manager->sctx->tcpinstats6,
					   DNS_SIZEHISTO_BUCKETIN(reqsize));
			break;
		default:
			UNREACHABLE();
		}
	} else {
		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_histomulti_inc(client->manager->sctx->udpinstats4,
					   DNS_SIZEHISTO_BUCKETIN(reqsize));
			break;
		case AF_INET6:
			isc_histomulti_inc(client->manager->sctx->udpinstats6,
					   DNS_SIZEHISTO_BUCKETIN(reqsize));
			break;
		default:
			UNREACHABLE();
		}
	}

	/*
	 * It's a request.  Parse it.
	 */
	result = dns_message_parse(client->message, client->buffer, 0);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Parsing the request failed.  Send a response
		 * (typically FORMERR or SERVFAIL).
		 */
		if (result == DNS_R_OPTERR) {
			(void)ns_client_addopt(client, client->message,
					       &client->opt);
		}

		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "message parsing failed: %s",
			      isc_result_totext(result));
		if (result == ISC_R_NOSPACE || result == DNS_R_BADTSIG) {
			result = DNS_R_FORMERR;
		}
		ns_client_error(client, result);
		return;
	}

	dns_opcodestats_increment(client->manager->sctx->opcodestats,
				  client->message->opcode);
	switch (client->message->opcode) {
	case dns_opcode_query:
	case dns_opcode_update:
	case dns_opcode_notify:
		notimp = false;
		break;
	case dns_opcode_iquery:
	default:
		notimp = true;
		break;
	}

	client->message->rcode = dns_rcode_noerror;
	client->ede = NULL;

	/*
	 * Deal with EDNS.
	 */
	if ((client->manager->sctx->options & NS_SERVER_NOEDNS) != 0) {
		opt = NULL;
	} else {
		opt = dns_message_getopt(client->message);
	}

	client->ecs.source = 0;
	client->ecs.scope = 0;

	if (opt != NULL) {
		/*
		 * Are returning FORMERR to all EDNS queries?
		 * Simulate a STD13 compliant server.
		 */
		if ((client->manager->sctx->options & NS_SERVER_EDNSFORMERR) !=
		    0)
		{
			ns_client_error(client, DNS_R_FORMERR);
			return;
		}

		/*
		 * Are returning NOTIMP to all EDNS queries?
		 */
		if ((client->manager->sctx->options & NS_SERVER_EDNSNOTIMP) !=
		    0)
		{
			ns_client_error(client, DNS_R_NOTIMP);
			return;
		}

		/*
		 * Are returning REFUSED to all EDNS queries?
		 */
		if ((client->manager->sctx->options & NS_SERVER_EDNSREFUSED) !=
		    0)
		{
			ns_client_error(client, DNS_R_REFUSED);
			return;
		}

		/*
		 * Are we dropping all EDNS queries?
		 */
		if ((client->manager->sctx->options & NS_SERVER_DROPEDNS) != 0)
		{
			ns_client_drop(client, ISC_R_SUCCESS);
			return;
		}

		result = process_opt(client, opt);
		if (result != ISC_R_SUCCESS) {
			return;
		}
	}

	if (client->message->rdclass == 0) {
		if ((client->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0 &&
		    client->message->opcode == dns_opcode_query &&
		    client->message->counts[DNS_SECTION_QUESTION] == 0U)
		{
			result = dns_message_reply(client->message, true);
			if (result != ISC_R_SUCCESS) {
				ns_client_error(client, result);
				return;
			}

			if (notimp) {
				client->message->rcode = dns_rcode_notimp;
			}

			ns_client_send(client);
			return;
		}

		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "message class could not be determined");
		ns_client_dumpmessage(client, "message class could not be "
					      "determined");
		ns_client_error(client, notimp ? DNS_R_NOTIMP : DNS_R_FORMERR);
		return;
	}

	client->destsockaddr = isc_nmhandle_localaddr(handle);
	isc_netaddr_fromsockaddr(&client->destaddr, &client->destsockaddr);

	/*
	 * Offload view matching only if we are going to check a SIG(0)
	 * signature.
	 */
	client->async = (client->message->tsigkey == NULL &&
			 client->message->tsig == NULL &&
			 client->message->sig0 != NULL);

	result = ns_client_setup_view(client, &netaddr);
	if (result == DNS_R_WAIT) {
		return;
	}

	ns_client_request_continue(client);
}

static void
ns_client_request_continue(void *arg) {
	ns_client_t *client = arg;
	const dns_name_t *signame = NULL;
	bool ra; /* Recursion available. */
	isc_result_t result = ISC_R_UNSET;
	static const char *ra_reasons[] = {
		"ACLs not processed yet",
		"no resolver in view",
		"recursion not enabled for view",
		"allow-recursion did not match",
		"allow-query-cache did not match",
		"allow-recursion-on did not match",
		"allow-query-cache-on did not match",
	};
	enum refusal_reasons {
		INVALID,
		NO_RESOLVER,
		RECURSION_DISABLED,
		ALLOW_RECURSION,
		ALLOW_QUERY_CACHE,
		ALLOW_RECURSION_ON,
		ALLOW_QUERY_CACHE_ON
	} ra_refusal_reason = INVALID;
#ifdef HAVE_DNSTAP
	dns_transport_type_t transport_type;
	dns_dtmsgtype_t dtmsgtype;
#endif /* ifdef HAVE_DNSTAP */

	INSIST(client->viewmatchresult != ISC_R_UNSET);

	/*
	 * This function could be running asynchronously, in which case update
	 * the current 'now' for correct timekeeping.
	 */
	if (client->async) {
		client->tnow = isc_time_now();
		client->now = isc_time_seconds(&client->tnow);
	}

	if (client->viewmatchresult != ISC_R_SUCCESS) {
		isc_buffer_t b;
		isc_region_t *r;

		/*
		 * Do a dummy TSIG verification attempt so that the
		 * response will have a TSIG if the query did, as
		 * required by RFC2845.
		 */
		dns_message_resetsig(client->message);
		r = dns_message_getrawmessage(client->message);
		isc_buffer_init(&b, r->base, r->length);
		isc_buffer_add(&b, r->length);
		(void)dns_tsig_verify(&b, client->message, NULL, NULL);

		if (client->viewmatchresult == ISC_R_QUOTA) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(5),
				      "SIG(0) checks quota reached");

			if (can_log_sigchecks_quota()) {
				ns_client_log(client, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_CLIENT, ISC_LOG_INFO,
					      "SIG(0) checks quota reached");
				ns_client_dumpmessage(
					client, "SIG(0) checks quota reached");
			}
		} else {
			char classname[DNS_RDATACLASS_FORMATSIZE];

			dns_rdataclass_format(client->message->rdclass,
					      classname, sizeof(classname));

			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
				      "no matching view in class '%s'",
				      classname);
			ns_client_dumpmessage(client,
					      "no matching view in class");
		}

		ns_client_extendederror(client, DNS_EDE_PROHIBITED, NULL);
		ns_client_error(client, DNS_R_REFUSED);

		goto cleanup;
	}

	if (isc_nm_is_proxy_handle(client->handle)) {
		char fmtbuf[ISC_SOCKADDR_FORMATSIZE] = { 0 };
		isc_netaddr_t real_local_addr, real_peer_addr;
		isc_sockaddr_t real_local, real_peer;
		int log_level = ISC_LOG_DEBUG(10);

		real_peer = isc_nmhandle_real_peeraddr(client->handle);
		isc_netaddr_fromsockaddr(&real_peer_addr, &real_peer);
		real_local = isc_nmhandle_real_localaddr(client->handle);
		isc_netaddr_fromsockaddr(&real_local_addr, &real_local);

		/* do not allow by default */
		if (ns_client_checkaclsilent(client, &real_peer_addr,
					     client->view->proxyacl,
					     false) != ISC_R_SUCCESS)
		{
			if (isc_log_wouldlog(ns_lctx, log_level)) {
				isc_sockaddr_format(&real_peer, fmtbuf,
						    sizeof(fmtbuf));
				ns_client_log(
					client, DNS_LOGCATEGORY_SECURITY,
					NS_LOGMODULE_CLIENT, log_level,
					"dropped request: PROXY is not allowed "
					"for that client (real client address: "
					"%s). Rejected by the 'allow-proxy' "
					"ACL",
					fmtbuf);
			}
			isc_nm_bad_request(client->handle);
			goto cleanup;
		}

		/* allow by default */
		if (ns_client_checkaclsilent(client, &real_local_addr,
					     client->view->proxyonacl,
					     true) != ISC_R_SUCCESS)
		{
			if (isc_log_wouldlog(ns_lctx, log_level)) {
				isc_sockaddr_format(&real_local, fmtbuf,
						    sizeof(fmtbuf));
				ns_client_log(
					client, DNS_LOGCATEGORY_SECURITY,
					NS_LOGMODULE_CLIENT, log_level,
					"dropped request: PROXY is not allowed "
					"on the interface (real interface "
					"address: %s). Rejected by the "
					"'allow-proxy-on' ACL",
					fmtbuf);
			}
			isc_nm_bad_request(client->handle);
			goto cleanup;
		}
	}

	ns_client_log(client, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(5), "using view '%s'", client->view->name);

	/*
	 * Check for a signature.  We log bad signatures regardless of
	 * whether they ultimately cause the request to be rejected or
	 * not.  We do not log the lack of a signature unless we are
	 * debugging.
	 */
	client->signer = NULL;
	dns_name_init(&client->signername, NULL);
	result = dns_message_signer(client->message, &client->signername);
	if (result != ISC_R_NOTFOUND) {
		signame = NULL;
		if (dns_message_gettsig(client->message, &signame) != NULL) {
			ns_stats_increment(client->manager->sctx->nsstats,
					   ns_statscounter_tsigin);
		} else {
			ns_stats_increment(client->manager->sctx->nsstats,
					   ns_statscounter_sig0in);
		}
	}
	if (result == ISC_R_SUCCESS) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(&client->signername, namebuf, sizeof(namebuf));
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request has valid signature: %s", namebuf);
		client->signer = &client->signername;
	} else if (result == ISC_R_NOTFOUND) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request is not signed");
	} else if (result == DNS_R_NOIDENTITY) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request is signed by a nonauthoritative key");
	} else {
		char tsigrcode[64];
		isc_buffer_t b;
		dns_rcode_t status;
		isc_result_t tresult;

		/* There is a signature, but it is bad. */
		ns_stats_increment(client->manager->sctx->nsstats,
				   ns_statscounter_invalidsig);
		signame = NULL;
		if (dns_message_gettsig(client->message, &signame) != NULL) {
			char namebuf[DNS_NAME_FORMATSIZE];

			status = client->message->tsigstatus;
			isc_buffer_init(&b, tsigrcode, sizeof(tsigrcode) - 1);
			tresult = dns_tsigrcode_totext(status, &b);
			INSIST(tresult == ISC_R_SUCCESS);
			tsigrcode[isc_buffer_usedlength(&b)] = '\0';
			if (client->message->tsigkey->generated) {
				dns_name_format(
					client->message->tsigkey->creator,
					namebuf, sizeof(namebuf));
			} else {
				dns_name_format(signame, namebuf,
						sizeof(namebuf));
			}
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_ERROR,
				      "request has invalid signature: "
				      "TSIG %s: %s (%s)",
				      namebuf, isc_result_totext(result),
				      tsigrcode);
		} else {
			status = client->message->sig0status;
			isc_buffer_init(&b, tsigrcode, sizeof(tsigrcode) - 1);
			tresult = dns_tsigrcode_totext(status, &b);
			INSIST(tresult == ISC_R_SUCCESS);
			tsigrcode[isc_buffer_usedlength(&b)] = '\0';
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_ERROR,
				      "request has invalid signature: %s (%s)",
				      isc_result_totext(result), tsigrcode);
		}

		/*
		 * Accept update messages signed by unknown keys so that
		 * update forwarding works transparently through slaves
		 * that don't have all the same keys as the primary.
		 */
		if (!(client->message->tsigstatus == dns_tsigerror_badkey &&
		      client->message->opcode == dns_opcode_update))
		{
			ns_client_error(client, client->sigresult);
			goto cleanup;
		}
	}

	/*
	 * Decide whether recursive service is available to this client.
	 * We do this here rather than in the query code so that we can
	 * set the RA bit correctly on all kinds of responses, not just
	 * responses to ordinary queries.  Note if you can't query the
	 * cache there is no point in setting RA.
	 */
	ra = false;

	/* must be initialized before ns_client_log uses it as index */
	if (client->view->resolver == NULL) {
		ra_refusal_reason = NO_RESOLVER;
	} else if (!client->view->recursion) {
		ra_refusal_reason = RECURSION_DISABLED;
	} else if (ns_client_checkaclsilent(client, NULL,
					    client->view->recursionacl,
					    true) != ISC_R_SUCCESS)
	{
		ra_refusal_reason = ALLOW_RECURSION;
	} else if (ns_client_checkaclsilent(client, NULL,
					    client->view->cacheacl,
					    true) != ISC_R_SUCCESS)
	{
		ra_refusal_reason = ALLOW_QUERY_CACHE;
	} else if (ns_client_checkaclsilent(client, &client->destaddr,
					    client->view->recursiononacl,
					    true) != ISC_R_SUCCESS)
	{
		ra_refusal_reason = ALLOW_RECURSION_ON;
	} else if (ns_client_checkaclsilent(client, &client->destaddr,
					    client->view->cacheonacl,
					    true) != ISC_R_SUCCESS)
	{
		ra_refusal_reason = ALLOW_QUERY_CACHE_ON;
	} else {
		ra = true;
		client->attributes |= NS_CLIENTATTR_RA;
	}

	ns_client_log(client, DNS_LOGCATEGORY_SECURITY, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(3),
		      ra ? "recursion available"
			 : "recursion not available (%s)",
		      ra_reasons[ra_refusal_reason]);

	/*
	 * Adjust maximum UDP response size for this client.
	 */
	if (client->udpsize > 512) {
		dns_peer_t *peer = NULL;
		uint16_t udpsize = client->view->maxudp;
		isc_netaddr_t netaddr;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		(void)dns_peerlist_peerbyaddr(client->view->peers, &netaddr,
					      &peer);
		if (peer != NULL) {
			dns_peer_getmaxudp(peer, &udpsize);
		}
		if (client->udpsize > udpsize) {
			client->udpsize = udpsize;
		}
	}

#ifdef HAVE_DNSTAP
	transport_type = ns_client_transport_type(client);
#endif /* HAVE_DNSTAP */

	/*
	 * Dispatch the request.
	 */
	switch (client->message->opcode) {
	case dns_opcode_query:
		CTRACE("query");
#ifdef HAVE_DNSTAP
		if (ra && (client->message->flags & DNS_MESSAGEFLAG_RD) != 0) {
			dtmsgtype = DNS_DTTYPE_CQ;
		} else {
			dtmsgtype = DNS_DTTYPE_AQ;
		}

		dns_dt_send(client->view, dtmsgtype, &client->peeraddr,
			    &client->destsockaddr, transport_type, NULL,
			    &client->requesttime, NULL, client->buffer);
#endif /* HAVE_DNSTAP */

		ns_query_start(client, client->handle);
		break;
	case dns_opcode_update:
		CTRACE("update");
#ifdef HAVE_DNSTAP
		dns_dt_send(client->view, DNS_DTTYPE_UQ, &client->peeraddr,
			    &client->destsockaddr, transport_type, NULL,
			    &client->requesttime, NULL, client->buffer);
#endif /* HAVE_DNSTAP */
		ns_client_settimeout(client, 60);
		ns_update_start(client, client->handle, client->sigresult);
		break;
	case dns_opcode_notify:
		CTRACE("notify");
		ns_client_settimeout(client, 60);
		ns_notify_start(client, client->handle);
		break;
	case dns_opcode_iquery:
		CTRACE("iquery");
		ns_client_error(client, DNS_R_NOTIMP);
		break;
	default:
		CTRACE("unknown opcode");
		ns_client_error(client, DNS_R_NOTIMP);
	}

cleanup:
	ns_client_async_reset(client);
}

isc_result_t
ns__client_tcpconn(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	ns_interface_t *ifp = (ns_interface_t *)arg;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(ifp->mgr);
	ns_server_t *sctx = ns_interfacemgr_getserver(ifp->mgr);
	unsigned int tcpquota;
	isc_sockaddr_t peeraddr;
	isc_netaddr_t netaddr;
	int match;

	if (result != ISC_R_SUCCESS) {
		return result;
	}

	if (handle != NULL) {
		peeraddr = isc_nmhandle_peeraddr(handle);
		isc_netaddr_fromsockaddr(&netaddr, &peeraddr);

		if (sctx->blackholeacl != NULL &&
		    (dns_acl_match(&netaddr, NULL, sctx->blackholeacl, env,
				   &match, NULL) == ISC_R_SUCCESS) &&
		    match > 0)
		{
			return ISC_R_CONNREFUSED;
		}
	}

	tcpquota = isc_quota_getused(&sctx->tcpquota);
	ns_stats_update_if_greater(sctx->nsstats, ns_statscounter_tcphighwater,
				   tcpquota);

	return ISC_R_SUCCESS;
}

void
ns__client_setup(ns_client_t *client, ns_clientmgr_t *mgr, bool new) {
	/*
	 * Note: creating a client does not add the client to the
	 * manager's client list, the caller is responsible for that.
	 */

	if (new) {
		REQUIRE(VALID_MANAGER(mgr));
		REQUIRE(client != NULL);
		REQUIRE(mgr->tid == isc_tid());

		*client = (ns_client_t){ .magic = 0 };

		ns_clientmgr_attach(mgr, &client->manager);

		dns_message_create(client->manager->mctx,
				   client->manager->namepool,
				   client->manager->rdspool,
				   DNS_MESSAGE_INTENTPARSE, &client->message);

		/*
		 * Set magic earlier than usual because ns_query_init()
		 * and the functions it calls will require it.
		 */
		client->magic = NS_CLIENT_MAGIC;
		ns_query_init(client);
	} else {
		REQUIRE(NS_CLIENT_VALID(client));
		REQUIRE(client->manager->tid == isc_tid());

		/*
		 * Retain these values from the existing client, but
		 * zero every thing else.
		 */
		*client = (ns_client_t){
			.magic = 0,
			.manager = client->manager,
			.message = client->message,
			.query = client->query,
		};
	}

	client->query.attributes &= ~NS_QUERYATTR_ANSWERED;
	client->state = NS_CLIENTSTATE_INACTIVE;
	client->udpsize = 512;
	client->ednsversion = -1;
	dns_name_init(&client->signername, NULL);
	dns_ecs_init(&client->ecs);
	isc_sockaddr_any(&client->formerrcache.addr);
	client->formerrcache.time = 0;
	client->formerrcache.id = 0;
	ISC_LINK_INIT(client, rlink);
	client->rcode_override = -1; /* not set */

	client->magic = NS_CLIENT_MAGIC;

	CTRACE("client_setup");
}

/***
 *** Client Manager
 ***/

static void
clientmgr_destroy_cb(void *arg) {
	ns_clientmgr_t *manager = (ns_clientmgr_t *)arg;
	MTRACE("clientmgr_destroy");

	manager->magic = 0;

	isc_loop_detach(&manager->loop);

	dns_aclenv_detach(&manager->aclenv);

	isc_mutex_destroy(&manager->reclock);

	ns_server_detach(&manager->sctx);

	dns_message_destroypools(&manager->rdspool, &manager->namepool);

	isc_mem_putanddetach(&manager->mctx, manager, sizeof(*manager));
}

static void
clientmgr_destroy(ns_clientmgr_t *mgr) {
	isc_async_run(mgr->loop, clientmgr_destroy_cb, mgr);
}

ISC_REFCOUNT_IMPL(ns_clientmgr, clientmgr_destroy);

isc_result_t
ns_clientmgr_create(ns_server_t *sctx, isc_loopmgr_t *loopmgr,
		    dns_aclenv_t *aclenv, int tid, ns_clientmgr_t **managerp) {
	ns_clientmgr_t *manager = NULL;
	isc_mem_t *mctx = NULL;

	isc_mem_create(&mctx);
	isc_mem_setname(mctx, "clientmgr");

	manager = isc_mem_get(mctx, sizeof(*manager));
	*manager = (ns_clientmgr_t){
		.magic = 0,
		.mctx = mctx,
		.tid = tid,
		.recursing = ISC_LIST_INITIALIZER,
	};
	isc_loop_attach(isc_loop_get(loopmgr, tid), &manager->loop);
	isc_mutex_init(&manager->reclock);
	dns_aclenv_attach(aclenv, &manager->aclenv);
	isc_refcount_init(&manager->references, 1);
	ns_server_attach(sctx, &manager->sctx);

	dns_message_createpools(mctx, &manager->namepool, &manager->rdspool);

	manager->magic = MANAGER_MAGIC;

	MTRACE("create");

	*managerp = manager;

	return ISC_R_SUCCESS;
}

void
ns_clientmgr_shutdown(ns_clientmgr_t *manager) {
	ns_client_t *client;

	REQUIRE(VALID_MANAGER(manager));

	MTRACE("destroy");

	LOCK(&manager->reclock);
	for (client = ISC_LIST_HEAD(manager->recursing); client != NULL;
	     client = ISC_LIST_NEXT(client, rlink))
	{
		ns_query_cancel(client);
	}
	UNLOCK(&manager->reclock);
}

isc_sockaddr_t *
ns_client_getsockaddr(ns_client_t *client) {
	return &client->peeraddr;
}

isc_sockaddr_t *
ns_client_getdestaddr(ns_client_t *client) {
	return &client->destsockaddr;
}

isc_result_t
ns_client_checkaclsilent(ns_client_t *client, isc_netaddr_t *netaddr,
			 dns_acl_t *acl, bool default_allow) {
	isc_result_t result;
	dns_aclenv_t *env = client->manager->aclenv;
	isc_netaddr_t tmpnetaddr;
	int match;
	isc_sockaddr_t local;

	if (acl == NULL) {
		if (default_allow) {
			goto allow;
		} else {
			goto deny;
		}
	}

	if (netaddr == NULL) {
		isc_netaddr_fromsockaddr(&tmpnetaddr, &client->peeraddr);
		netaddr = &tmpnetaddr;
	}

	local = isc_nmhandle_localaddr(client->handle);
	result = dns_acl_match_port_transport(
		netaddr, isc_sockaddr_getport(&local),
		isc_nm_socket_type(client->handle),
		isc_nm_has_encryption(client->handle), client->signer, acl, env,
		&match, NULL);

	if (result != ISC_R_SUCCESS) {
		goto deny; /* Internal error, already logged. */
	}

	if (match > 0) {
		goto allow;
	}
	goto deny; /* Negative match or no match. */

allow:
	return ISC_R_SUCCESS;

deny:
	return DNS_R_REFUSED;
}

isc_result_t
ns_client_checkacl(ns_client_t *client, isc_sockaddr_t *sockaddr,
		   const char *opname, dns_acl_t *acl, bool default_allow,
		   int log_level) {
	isc_result_t result;
	isc_netaddr_t netaddr;

	if (sockaddr != NULL) {
		isc_netaddr_fromsockaddr(&netaddr, sockaddr);
	}

	result = ns_client_checkaclsilent(client, sockaddr ? &netaddr : NULL,
					  acl, default_allow);

	if (result == ISC_R_SUCCESS) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "%s approved", opname);
	} else {
		ns_client_extendederror(client, DNS_EDE_PROHIBITED, NULL);
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, log_level, "%s denied",
			      opname);
	}
	return result;
}

static void
ns_client_name(ns_client_t *client, char *peerbuf, size_t len) {
	if (client->peeraddr_valid) {
		isc_sockaddr_format(&client->peeraddr, peerbuf,
				    (unsigned int)len);
	} else {
		snprintf(peerbuf, len, "@%p", client);
	}
}

void
ns_client_logv(ns_client_t *client, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *fmt,
	       va_list ap) {
	char msgbuf[4096];
	char signerbuf[DNS_NAME_FORMATSIZE], qnamebuf[DNS_NAME_FORMATSIZE];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	const char *viewname = "";
	const char *sep1 = "", *sep2 = "", *sep3 = "", *sep4 = "";
	const char *signer = "", *qname = "";
	dns_name_t *q = NULL;

	REQUIRE(client != NULL);

	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);

	if (client->signer != NULL) {
		dns_name_format(client->signer, signerbuf, sizeof(signerbuf));
		sep1 = "/key ";
		signer = signerbuf;
	}

	q = client->query.origqname != NULL ? client->query.origqname
					    : client->query.qname;
	if (q != NULL) {
		dns_name_format(q, qnamebuf, sizeof(qnamebuf));
		sep2 = " (";
		sep3 = ")";
		qname = qnamebuf;
	}

	if (client->view != NULL && strcmp(client->view->name, "_bind") != 0 &&
	    strcmp(client->view->name, "_default") != 0)
	{
		sep4 = ": view ";
		viewname = client->view->name;
	}

	if (client->peeraddr_valid) {
		isc_sockaddr_format(&client->peeraddr, peerbuf,
				    sizeof(peerbuf));
	} else {
		snprintf(peerbuf, sizeof(peerbuf), "(no-peer)");
	}

	isc_log_write(ns_lctx, category, module, level,
		      "client @%p %s%s%s%s%s%s%s%s: %s", client, peerbuf, sep1,
		      signer, sep2, qname, sep3, sep4, viewname, msgbuf);
}

void
ns_client_log(ns_client_t *client, isc_logcategory_t *category,
	      isc_logmodule_t *module, int level, const char *fmt, ...) {
	va_list ap;

	if (!isc_log_wouldlog(ns_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	ns_client_logv(client, category, module, level, fmt, ap);
	va_end(ap);
}

void
ns_client_aclmsg(const char *msg, const dns_name_t *name, dns_rdatatype_t type,
		 dns_rdataclass_t rdclass, char *buf, size_t len) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];

	dns_name_format(name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));
	dns_rdataclass_format(rdclass, classbuf, sizeof(classbuf));
	(void)snprintf(buf, len, "%s '%s/%s/%s'", msg, namebuf, typebuf,
		       classbuf);
}

static void
ns_client_dumpmessage(ns_client_t *client, const char *reason) {
	isc_buffer_t buffer;
	char *buf = NULL;
	int len = 1024;
	isc_result_t result;

	if (!isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(1))) {
		return;
	}

	/*
	 * Note that these are multiline debug messages.  We want a newline
	 * to appear in the log after each message.
	 */

	do {
		buf = isc_mem_get(client->manager->mctx, len);
		isc_buffer_init(&buffer, buf, len);
		result = dns_message_totext(
			client->message, &dns_master_style_debug, 0, &buffer);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(client->manager->mctx, buf, len);
			len += 1024;
		} else if (result == ISC_R_SUCCESS) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
				      "%s\n%.*s", reason,
				      (int)isc_buffer_usedlength(&buffer), buf);
		}
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL) {
		isc_mem_put(client->manager->mctx, buf, len);
	}
}

void
ns_client_dumprecursing(FILE *f, ns_clientmgr_t *manager) {
	ns_client_t *client;
	char namebuf[DNS_NAME_FORMATSIZE];
	char original[DNS_NAME_FORMATSIZE];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
	const char *name;
	const char *sep;
	const char *origfor;
	dns_rdataset_t *rdataset;

	REQUIRE(VALID_MANAGER(manager));

	LOCK(&manager->reclock);
	client = ISC_LIST_HEAD(manager->recursing);
	while (client != NULL) {
		INSIST(client->state == NS_CLIENTSTATE_RECURSING);

		ns_client_name(client, peerbuf, sizeof(peerbuf));
		if (client->view != NULL &&
		    strcmp(client->view->name, "_bind") != 0 &&
		    strcmp(client->view->name, "_default") != 0)
		{
			name = client->view->name;
			sep = ": view ";
		} else {
			name = "";
			sep = "";
		}

		LOCK(&client->query.fetchlock);
		INSIST(client->query.qname != NULL);
		dns_name_format(client->query.qname, namebuf, sizeof(namebuf));
		if (client->query.qname != client->query.origqname &&
		    client->query.origqname != NULL)
		{
			origfor = " for ";
			dns_name_format(client->query.origqname, original,
					sizeof(original));
		} else {
			origfor = "";
			original[0] = '\0';
		}
		rdataset = ISC_LIST_HEAD(client->query.qname->list);
		if (rdataset == NULL && client->query.origqname != NULL) {
			rdataset = ISC_LIST_HEAD(client->query.origqname->list);
		}
		if (rdataset != NULL) {
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			dns_rdataclass_format(rdataset->rdclass, classbuf,
					      sizeof(classbuf));
		} else {
			strlcpy(typebuf, "-", sizeof(typebuf));
			strlcpy(classbuf, "-", sizeof(classbuf));
		}
		UNLOCK(&client->query.fetchlock);
		fprintf(f,
			"; client %s%s%s: id %u '%s/%s/%s'%s%s "
			"requesttime %u\n",
			peerbuf, sep, name, client->message->id, namebuf,
			typebuf, classbuf, origfor, original,
			isc_time_seconds(&client->requesttime));
		client = ISC_LIST_NEXT(client, rlink);
	}
	UNLOCK(&manager->reclock);
}

void
ns_client_qnamereplace(ns_client_t *client, dns_name_t *name) {
	LOCK(&client->query.fetchlock);
	if (client->query.restarts > 0) {
		/*
		 * client->query.qname was dynamically allocated.
		 */
		dns_message_puttempname(client->message, &client->query.qname);
	}
	client->query.qname = name;
	client->query.attributes &= ~NS_QUERYATTR_REDIRECT;
	UNLOCK(&client->query.fetchlock);
}

isc_result_t
ns_client_sourceip(dns_clientinfo_t *ci, isc_sockaddr_t **addrp) {
	ns_client_t *client = (ns_client_t *)ci->data;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(addrp != NULL);

	*addrp = &client->peeraddr;
	return ISC_R_SUCCESS;
}

dns_rdataset_t *
ns_client_newrdataset(ns_client_t *client) {
	dns_rdataset_t *rdataset;

	REQUIRE(NS_CLIENT_VALID(client));

	rdataset = NULL;
	dns_message_gettemprdataset(client->message, &rdataset);

	return rdataset;
}

void
ns_client_putrdataset(ns_client_t *client, dns_rdataset_t **rdatasetp) {
	dns_rdataset_t *rdataset;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(rdatasetp != NULL);

	rdataset = *rdatasetp;

	if (rdataset != NULL) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		dns_message_puttemprdataset(client->message, rdatasetp);
	}
}

isc_result_t
ns_client_newnamebuf(ns_client_t *client) {
	isc_buffer_t *dbuf = NULL;

	CTRACE("ns_client_newnamebuf");

	isc_buffer_allocate(client->manager->mctx, &dbuf, 1024);
	ISC_LIST_APPEND(client->query.namebufs, dbuf, link);

	CTRACE("ns_client_newnamebuf: done");
	return ISC_R_SUCCESS;
}

dns_name_t *
ns_client_newname(ns_client_t *client, isc_buffer_t *dbuf, isc_buffer_t *nbuf) {
	dns_name_t *name = NULL;
	isc_region_t r;

	REQUIRE((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED) == 0);

	CTRACE("ns_client_newname");

	dns_message_gettempname(client->message, &name);
	isc_buffer_availableregion(dbuf, &r);
	isc_buffer_init(nbuf, r.base, r.length);
	dns_name_setbuffer(name, NULL);
	dns_name_setbuffer(name, nbuf);
	client->query.attributes |= NS_QUERYATTR_NAMEBUFUSED;

	CTRACE("ns_client_newname: done");
	return name;
}

isc_buffer_t *
ns_client_getnamebuf(ns_client_t *client) {
	isc_buffer_t *dbuf;
	isc_region_t r;

	CTRACE("ns_client_getnamebuf");

	/*%
	 * Return a name buffer with space for a maximal name, allocating
	 * a new one if necessary.
	 */
	if (ISC_LIST_EMPTY(client->query.namebufs)) {
		ns_client_newnamebuf(client);
	}

	dbuf = ISC_LIST_TAIL(client->query.namebufs);
	INSIST(dbuf != NULL);
	isc_buffer_availableregion(dbuf, &r);
	if (r.length < DNS_NAME_MAXWIRE) {
		ns_client_newnamebuf(client);
		dbuf = ISC_LIST_TAIL(client->query.namebufs);
		isc_buffer_availableregion(dbuf, &r);
		INSIST(r.length >= 255);
	}
	CTRACE("ns_client_getnamebuf: done");
	return dbuf;
}

void
ns_client_keepname(ns_client_t *client, dns_name_t *name, isc_buffer_t *dbuf) {
	isc_region_t r;

	CTRACE("ns_client_keepname");

	/*%
	 * 'name' is using space in 'dbuf', but 'dbuf' has not yet been
	 * adjusted to take account of that.  We do the adjustment.
	 */
	REQUIRE((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED) != 0);

	dns_name_toregion(name, &r);
	isc_buffer_add(dbuf, r.length);
	dns_name_setbuffer(name, NULL);
	client->query.attributes &= ~NS_QUERYATTR_NAMEBUFUSED;
}

void
ns_client_releasename(ns_client_t *client, dns_name_t **namep) {
	/*%
	 * 'name' is no longer needed.  Return it to our pool of temporary
	 * names.  If it is using a name buffer, relinquish its exclusive
	 * rights on the buffer.
	 */

	CTRACE("ns_client_releasename");
	client->query.attributes &= ~NS_QUERYATTR_NAMEBUFUSED;
	dns_message_puttempname(client->message, namep);
	CTRACE("ns_client_releasename: done");
}

isc_result_t
ns_client_newdbversion(ns_client_t *client, unsigned int n) {
	unsigned int i;
	ns_dbversion_t *dbversion = NULL;

	for (i = 0; i < n; i++) {
		dbversion = isc_mem_get(client->manager->mctx,
					sizeof(*dbversion));
		*dbversion = (ns_dbversion_t){ 0 };
		ISC_LIST_INITANDAPPEND(client->query.freeversions, dbversion,
				       link);
	}

	return ISC_R_SUCCESS;
}

static ns_dbversion_t *
client_getdbversion(ns_client_t *client) {
	ns_dbversion_t *dbversion = NULL;

	if (ISC_LIST_EMPTY(client->query.freeversions)) {
		ns_client_newdbversion(client, 1);
	}
	dbversion = ISC_LIST_HEAD(client->query.freeversions);
	INSIST(dbversion != NULL);
	ISC_LIST_UNLINK(client->query.freeversions, dbversion, link);

	return dbversion;
}

ns_dbversion_t *
ns_client_findversion(ns_client_t *client, dns_db_t *db) {
	ns_dbversion_t *dbversion;

	for (dbversion = ISC_LIST_HEAD(client->query.activeversions);
	     dbversion != NULL; dbversion = ISC_LIST_NEXT(dbversion, link))
	{
		if (dbversion->db == db) {
			break;
		}
	}

	if (dbversion == NULL) {
		/*
		 * This is a new zone for this query.  Add it to
		 * the active list.
		 */
		dbversion = client_getdbversion(client);
		if (dbversion == NULL) {
			return NULL;
		}
		dns_db_attach(db, &dbversion->db);
		dns_db_currentversion(db, &dbversion->version);
		dbversion->acl_checked = false;
		dbversion->queryok = false;
		ISC_LIST_APPEND(client->query.activeversions, dbversion, link);
	}

	return dbversion;
}
