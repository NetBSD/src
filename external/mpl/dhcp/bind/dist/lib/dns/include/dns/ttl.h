/*	$NetBSD: ttl.h,v 1.1.2.2 2024/02/24 13:07:08 martin Exp $	*/

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

#ifndef DNS_TTL_H
#define DNS_TTL_H 1

/*! \file dns/ttl.h */

/***
 ***	Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_ttl_totext(uint32_t src, bool verbose, bool upcase, isc_buffer_t *target);
/*%<
 * Output a TTL or other time interval in a human-readable form.
 * The time interval is given as a count of seconds in 'src'.
 * The text representation is appended to 'target'.
 *
 * If 'verbose' is false, use the terse BIND 8 style, like "1w2d3h4m5s".
 *
 * If 'verbose' is true, use a verbose style like the SOA comments
 * in "dig", like "1 week 2 days 3 hours 4 minutes 5 seconds".
 *
 * If 'upcase' is true, we conform to the BIND 8 style in which
 * the unit letter is capitalized if there is only a single unit
 * letter to print (for example, "1m30s", but "2M")
 *
 * If 'upcase' is false, unit letters are always in lower case.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOSPACE
 */

isc_result_t
dns_counter_fromtext(isc_textregion_t *source, uint32_t *ttl);
/*%<
 * Converts a counter from either a plain number or a BIND 8 style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_SYNTAX
 */

isc_result_t
dns_ttl_fromtext(isc_textregion_t *source, uint32_t *ttl);
/*%<
 * Converts a ttl from either a plain number or a BIND 8 style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_BADTTL
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TTL_H */
