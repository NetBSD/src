/*	$NetBSD: time.h,v 1.3.2.2 2019/06/10 22:04:37 christos Exp $	*/

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


#ifndef DNS_TIME_H
#define DNS_TIME_H 1

/*! \file dns/time.h */

/***
 ***	Imports
 ***/

#include <inttypes.h>

#include <isc/buffer.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_time64_fromtext(const char *source, int64_t *target);
/*%<
 * Convert a date and time in YYYYMMDDHHMMSS text format at 'source'
 * into to a 64-bit count of seconds since Jan 1 1970 0:00 GMT.
 * Store the count at 'target'.
 */

isc_result_t
dns_time32_fromtext(const char *source, uint32_t *target);
/*%<
 * Like dns_time64_fromtext, but returns the second count modulo 2^32
 * as per RFC2535.
 */


isc_result_t
dns_time64_totext(int64_t value, isc_buffer_t *target);
/*%<
 * Convert a 64-bit count of seconds since Jan 1 1970 0:00 GMT into
 * a YYYYMMDDHHMMSS text representation and append it to 'target'.
 */

isc_result_t
dns_time32_totext(uint32_t value, isc_buffer_t *target);
/*%<
 * Like dns_time64_totext, but for a 32-bit cyclic time value.
 * Of those dates whose counts of seconds since Jan 1 1970 0:00 GMT
 * are congruent with 'value' modulo 2^32, the one closest to the
 * current date is chosen.
 */

int64_t
dns_time64_from32(uint32_t value);
/*%<
 * Covert a 32-bit cyclic time value into a 64 bit time stamp.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TIME_H */
