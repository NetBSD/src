/*	$NetBSD: ede.h,v 1.2 2025/05/21 14:48:04 christos Exp $	*/

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

#include <isc/mem.h>

#include <dns/message.h>

/*%< EDNS0 extended DNS errors */
#define DNS_EDE_OTHER		     0	/*%< Other Error */
#define DNS_EDE_DNSKEYALG	     1	/*%< Unsupported DNSKEY Algorithm */
#define DNS_EDE_DSDIGESTTYPE	     2	/*%< Unsupported DS Digest Type */
#define DNS_EDE_STALEANSWER	     3	/*%< Stale Answer */
#define DNS_EDE_FORGEDANSWER	     4	/*%< Forged Answer */
#define DNS_EDE_DNSSECINDETERMINATE  5	/*%< DNSSEC Indeterminate */
#define DNS_EDE_DNSSECBOGUS	     6	/*%< DNSSEC Bogus */
#define DNS_EDE_SIGNATUREEXPIRED     7	/*%< Signature Expired */
#define DNS_EDE_SIGNATURENOTYETVALID 8	/*%< Signature Not Yet Valid */
#define DNS_EDE_DNSKEYMISSING	     9	/*%< DNSKEY Missing */
#define DNS_EDE_RRSIGSMISSING	     10 /*%< RRSIGs Missing */
#define DNS_EDE_NOZONEKEYBITSET	     11 /*%< No Zone Key Bit Set */
#define DNS_EDE_NSECMISSING	     12 /*%< NSEC Missing */
#define DNS_EDE_CACHEDERROR	     13 /*%< Cached Error */
#define DNS_EDE_NOTREADY	     14 /*%< Not Ready */
#define DNS_EDE_BLOCKED		     15 /*%< Blocked */
#define DNS_EDE_CENSORED	     16 /*%< Censored */
#define DNS_EDE_FILTERED	     17 /*%< Filtered */
#define DNS_EDE_PROHIBITED	     18 /*%< Prohibited */
#define DNS_EDE_STALENXANSWER	     19 /*%< Stale NXDomain Answer */
#define DNS_EDE_NOTAUTH		     20 /*%< Not Authoritative */
#define DNS_EDE_NOTSUPPORTED	     21 /*%< Not Supported */
#define DNS_EDE_NOREACHABLEAUTH	     22 /*%< No Reachable Authority */
#define DNS_EDE_NETWORKERROR	     23 /*%< Network Error */
#define DNS_EDE_INVALIDDATA	     24 /*%< Invalid Data */

#define DNS_EDE_MAX_CODE DNS_EDE_INVALIDDATA

/*
 * From RFC 8914:
 * Because long EXTRA-TEXT fields may trigger truncation (which is undesirable
 * given the supplemental nature of EDE), implementers and operators creating
 * EDE options SHOULD avoid lengthy EXTRA-TEXT contents.
 *
 * Following this advice we limit the EXTRA-TEXT length to 64 characters.
 */
#define DNS_EDE_EXTRATEXT_LEN 64

#define DNS_EDE_MAX_ERRORS 3

typedef struct dns_edectx dns_edectx_t;
struct dns_edectx {
	int	       magic;
	isc_mem_t     *mctx;
	dns_ednsopt_t *ede[DNS_EDE_MAX_ERRORS];
	uint32_t       edeused;
	size_t	       nextede;
};
/*%<
 * Multiple extended DNS errors (EDE) (defined in RFC 8914) can be raised during
 * a DNS resolution and in various area of the code base. "dns_edectx_t" object
 * abstract and holds pending EDE and the set of dns_ede_ API enable to
 * manipulate its state (adding EDE, transfer to another context, etc.). EDE are
 * internally stored in the wire format, so it can be directly consumed to build
 * the response client message.
 */

void
dns_ede_init(isc_mem_t *mctx, dns_edectx_t *edectx);
/*%<
 * Initialize "edectx" so it is valid to use. Can be called after
 * dns_ede_invalidate" is being called to reuse the object.
 *
 * Requires:
 *
 * \li "mctx" to be valid
 * \li "edectx" to be valid
 */

void
dns_ede_reset(dns_edectx_t *edectx);
/*%<
 * Reset "edectx" internal state and free all its EDE from memory. "edectx" is
 * still valid to use, in the same state than after dns_ede_init is called.
 *
 * Requires:
 *
 * \li "edectx" to be valid
 */

void
dns_ede_invalidate(dns_edectx_t *edectx);
/*%<
 * Reset "edectx" and remove its memory context as well as its magic number. It
 * is not valid to use anymore.
 *
 * Requires:
 *
 * \li "edectx" to be valid
 */

void
dns_ede_add(dns_edectx_t *edectx, uint16_t code, const char *text);
/*%<
 * Add a new extended error in "edectx". "code" must be one of the INFO-CODE
 * values defined in RFC8914, see DNS_EDE_ macros above. "text" is optional, it
 * is immediately copied internally if provided.
 *
 * Rules:
 *
 * \li If "text" is non NULL, it must be NULL terminated. If its length is more
 * than DNS_EDE_EXTRATEXT_LEN, it is trucated.
 *
 * \li If an EDE with the same code has already been added to "edectx", the
 * following ones with the same code are ignored.
 *
 * \li If more than DNS_EDE_MAX_ERRORS EDE have been already added to this
 * context, the following ones are ignored.
 *
 * Requires:
 *
 * \li "edectx" to be valid
 * \li "code" to be one of the INFO-CODE defied in RFC8914, see DNS_EDE_ macros.
 */

void
dns_ede_copy(dns_edectx_t *edectx_to, const dns_edectx_t *edectx_from);
/*%<
 * Copy all EDE from "edectx_from" into "edectx_to". If "edectx_to" reaches the
 * maximum number of EDE (see DNS_EDE_MAX_ERRORS), the copy stops and
 * remaining EDE in "edectx_from" are not copied.
 *
 * Rules defined in "dns_ede_add" applies.
 *
 * Requires:
 *
 * \li "edectx_from" to be valid
 * \li "edectx_to" to be valid
 */
