/*	$NetBSD: notify.c,v 1.6.2.1 2024/02/25 15:47:34 martin Exp $	*/

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

#include <isc/log.h>
#include <isc/print.h>
#include <isc/result.h>

#include <dns/message.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/tsig.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zt.h>

#include <ns/log.h>
#include <ns/notify.h>
#include <ns/types.h>

/*! \file
 * \brief
 * This module implements notify as in RFC1996.
 */

static void
notify_log(ns_client_t *client, int level, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	ns_client_logv(client, DNS_LOGCATEGORY_NOTIFY, NS_LOGMODULE_NOTIFY,
		       level, fmt, ap);
	va_end(ap);
}

static void
respond(ns_client_t *client, isc_result_t result) {
	dns_rcode_t rcode;
	dns_message_t *message;
	isc_result_t msg_result;

	message = client->message;
	rcode = dns_result_torcode(result);

	msg_result = dns_message_reply(message, true);
	if (msg_result != ISC_R_SUCCESS) {
		msg_result = dns_message_reply(message, false);
	}
	if (msg_result != ISC_R_SUCCESS) {
		ns_client_drop(client, msg_result);
		isc_nmhandle_detach(&client->reqhandle);
		return;
	}
	message->rcode = rcode;
	if (rcode == dns_rcode_noerror) {
		message->flags |= DNS_MESSAGEFLAG_AA;
	} else {
		message->flags &= ~DNS_MESSAGEFLAG_AA;
	}

	ns_client_send(client);
	isc_nmhandle_detach(&client->reqhandle);
}

void
ns_notify_start(ns_client_t *client, isc_nmhandle_t *handle) {
	dns_message_t *request = client->message;
	isc_result_t result;
	dns_name_t *zonename;
	dns_rdataset_t *zone_rdataset;
	dns_zone_t *zone = NULL;
	char namebuf[DNS_NAME_FORMATSIZE];
	char tsigbuf[DNS_NAME_FORMATSIZE * 2 + sizeof(": TSIG '' ()")];
	dns_tsigkey_t *tsigkey;

	/*
	 * Attach to the request handle
	 */
	isc_nmhandle_attach(handle, &client->reqhandle);

	/*
	 * Interpret the question section.
	 */
	result = dns_message_firstname(request, DNS_SECTION_QUESTION);
	if (result != ISC_R_SUCCESS) {
		notify_log(client, ISC_LOG_NOTICE,
			   "notify question section empty");
		result = DNS_R_FORMERR;
		goto done;
	}

	/*
	 * The question section must contain exactly one question.
	 */
	zonename = NULL;
	dns_message_currentname(request, DNS_SECTION_QUESTION, &zonename);
	zone_rdataset = ISC_LIST_HEAD(zonename->list);
	if (ISC_LIST_NEXT(zone_rdataset, link) != NULL) {
		notify_log(client, ISC_LOG_NOTICE,
			   "notify question section contains multiple RRs");
		result = DNS_R_FORMERR;
		goto done;
	}

	/* The zone section must have exactly one name. */
	result = dns_message_nextname(request, DNS_SECTION_ZONE);
	if (result != ISC_R_NOMORE) {
		notify_log(client, ISC_LOG_NOTICE,
			   "notify question section contains multiple RRs");
		result = DNS_R_FORMERR;
		goto done;
	}

	/* The one rdataset must be an SOA. */
	if (zone_rdataset->type != dns_rdatatype_soa) {
		notify_log(client, ISC_LOG_NOTICE,
			   "notify question section contains no SOA");
		result = DNS_R_FORMERR;
		goto done;
	}

	tsigkey = dns_message_gettsigkey(request);
	if (tsigkey != NULL) {
		dns_name_format(&tsigkey->name, namebuf, sizeof(namebuf));

		if (tsigkey->generated) {
			char cnamebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(tsigkey->creator, cnamebuf,
					sizeof(cnamebuf));
			snprintf(tsigbuf, sizeof(tsigbuf), ": TSIG '%s' (%s)",
				 namebuf, cnamebuf);
		} else {
			snprintf(tsigbuf, sizeof(tsigbuf), ": TSIG '%s'",
				 namebuf);
		}
	} else {
		tsigbuf[0] = '\0';
	}

	dns_name_format(zonename, namebuf, sizeof(namebuf));
	result = dns_zt_find(client->view->zonetable, zonename, 0, NULL, &zone);
	if (result == ISC_R_SUCCESS) {
		dns_zonetype_t zonetype = dns_zone_gettype(zone);

		if ((zonetype == dns_zone_primary) ||
		    (zonetype == dns_zone_secondary) ||
		    (zonetype == dns_zone_mirror) ||
		    (zonetype == dns_zone_stub))
		{
			isc_sockaddr_t *from = ns_client_getsockaddr(client);
			isc_sockaddr_t *to = ns_client_getdestaddr(client);
			notify_log(client, ISC_LOG_INFO,
				   "received notify for zone '%s'%s", namebuf,
				   tsigbuf);
			result = dns_zone_notifyreceive(zone, from, to,
							request);
			goto done;
		}
	}

	notify_log(client, ISC_LOG_NOTICE,
		   "received notify for zone '%s'%s: not authoritative",
		   namebuf, tsigbuf);
	result = DNS_R_NOTAUTH;

done:
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}
	respond(client, result);
}
