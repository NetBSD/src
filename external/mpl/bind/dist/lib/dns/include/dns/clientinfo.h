/*	$NetBSD: clientinfo.h,v 1.1.1.7 2024/02/21 21:54:53 christos Exp $	*/

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

#pragma once

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

#include <inttypes.h>

#include <isc/sockaddr.h>
#include <isc/types.h>

#include <dns/ecs.h>

ISC_LANG_BEGINDECLS

/*****
***** Types
*****/

#define DNS_CLIENTINFO_VERSION 3
/*
 * Any updates to this structure should also be applied in
 * contrib/modules/dlz/dlz_minmal.h.
 */
typedef struct dns_clientinfo {
	uint16_t  version;
	void	 *data;
	void	 *dbversion;
	dns_ecs_t ecs;
} dns_clientinfo_t;

typedef isc_result_t (*dns_clientinfo_sourceip_t)(dns_clientinfo_t *client,
						  isc_sockaddr_t  **addrp);

#define DNS_CLIENTINFOMETHODS_VERSION 2
#define DNS_CLIENTINFOMETHODS_AGE     1

/*
 * Any updates to this structure should also be applied in
 * contrib/modules/dlz/dlz_minmal.h.
 */
typedef struct dns_clientinfomethods {
	uint16_t		  version;
	uint16_t		  age;
	dns_clientinfo_sourceip_t sourceip;
} dns_clientinfomethods_t;

/*****
***** Methods
*****/
void
dns_clientinfomethods_init(dns_clientinfomethods_t  *methods,
			   dns_clientinfo_sourceip_t sourceip);

void
dns_clientinfo_init(dns_clientinfo_t *ci, void *data, void *versionp);
/*%<
 * Initialize a clientinfo object, setting the data to 'data' and the
 * database version to 'versionp'. ECS data is initialized to 0/0/0.
 */

void
dns_clientinfo_setecs(dns_clientinfo_t *ci, dns_ecs_t *ecs);
/*%<
 * Set the ECS client data associated with a clientinfo object 'ci'.
 * If 'ecs' is NULL, initialize ci->ecs to 0/0/0; otherwise copy it.
 */

ISC_LANG_ENDDECLS
