/*	$NetBSD: ttl.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DNS_TTL_H
#define DNS_TTL_H 1

/*! \file dns/ttl.h */

/***
 ***	Imports
 ***/

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_ttl_totext(isc_uint32_t src, isc_boolean_t verbose,
	       isc_buffer_t *target);
isc_result_t
dns_ttl_totext2(isc_uint32_t src, isc_boolean_t verbose,
		isc_boolean_t upcase, isc_buffer_t *target);
/*%<
 * Output a TTL or other time interval in a human-readable form.
 * The time interval is given as a count of seconds in 'src'.
 * The text representation is appended to 'target'.
 *
 * If 'verbose' is ISC_FALSE, use the terse BIND 8 style, like "1w2d3h4m5s".
 *
 * If 'verbose' is ISC_TRUE, use a verbose style like the SOA comments
 * in "dig", like "1 week 2 days 3 hours 4 minutes 5 seconds".
 *
 * If 'upcase' is ISC_TRUE, we conform to the BIND 8 style in which
 * the unit letter is capitalized if there is only a single unit
 * letter to print (for example, "1m30s", but "2M")
 *
 * If 'upcase' is ISC_FALSE, unit letters are always in lower case.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOSPACE
 */

isc_result_t
dns_counter_fromtext(isc_textregion_t *source, isc_uint32_t *ttl);
/*%<
 * Converts a counter from either a plain number or a BIND 8 style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_SYNTAX
 */

isc_result_t
dns_ttl_fromtext(isc_textregion_t *source, isc_uint32_t *ttl);
/*%<
 * Converts a ttl from either a plain number or a BIND 8 style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_BADTTL
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TTL_H */
