/*	$NetBSD: clientinfo.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


#ifndef DNS_CLIENTINFO_H
#define DNS_CLIENTINFO_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/clientinfo.h
 * \brief
 * The DNS clientinfo interface allows libdns to retrieve information
 * about the client from the caller.
 *
 * The clientinfo interface is used by the DNS DB and DLZ interfaces;
 * it allows databases to modify their answers on the basis of information
 * about the client, such as source IP address.
 *
 * dns_clientinfo_t contains a pointer to an opaque structure containing
 * client information in some form.  dns_clientinfomethods_t contains a
 * list of methods which operate on that opaque structure to return
 * potentially useful data.  Both structures also contain versioning
 * information.
 */

/*****
 ***** Imports
 *****/

#include <isc/sockaddr.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/*****
 ***** Types
 *****/

#define DNS_CLIENTINFO_VERSION 2
typedef struct dns_clientinfo {
	isc_uint16_t version;
	void *data;
	void *dbversion;
} dns_clientinfo_t;

typedef isc_result_t (*dns_clientinfo_sourceip_t)(dns_clientinfo_t *client,
						  isc_sockaddr_t **addrp);

#define DNS_CLIENTINFOMETHODS_VERSION 1
#define DNS_CLIENTINFOMETHODS_AGE 0

typedef struct dns_clientinfomethods {
	isc_uint16_t version;
	isc_uint16_t age;
	dns_clientinfo_sourceip_t sourceip;
} dns_clientinfomethods_t;

/*****
 ***** Methods
 *****/
void
dns_clientinfomethods_init(dns_clientinfomethods_t *methods,
			   dns_clientinfo_sourceip_t sourceip);

void
dns_clientinfo_init(dns_clientinfo_t *ci, void *data, void *versionp);

ISC_LANG_ENDDECLS

#endif /* DNS_CLIENTINFO_H */
