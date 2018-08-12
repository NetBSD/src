/*	$NetBSD: rcode.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

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


#ifndef DNS_RCODE_H
#define DNS_RCODE_H 1

/*! \file dns/rcode.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t dns_rcode_fromtext(dns_rcode_t *rcodep, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNS error value.
 *
 * Requires:
 *\li	'rcodep' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#DNS_R_UNKNOWN			type is unknown
 */

isc_result_t dns_rcode_totext(dns_rcode_t rcode, isc_buffer_t *target);
/*%<
 * Put a textual representation of error 'rcode' into 'target'.
 *
 * Requires:
 *\li	'rcode' is a valid rcode.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures:
 *\li	If the result is success:
 *		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

isc_result_t dns_tsigrcode_fromtext(dns_rcode_t *rcodep,
				    isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a TSIG/TKEY error value.
 *
 * Requires:
 *\li	'rcodep' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#DNS_R_UNKNOWN			type is unknown
 */

isc_result_t dns_tsigrcode_totext(dns_rcode_t rcode, isc_buffer_t *target);
/*%<
 * Put a textual representation of TSIG/TKEY error 'rcode' into 'target'.
 *
 * Requires:
 *\li	'rcode' is a valid TSIG/TKEY error code.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures:
 *\li	If the result is success:
 *		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

isc_result_t
dns_hashalg_fromtext(unsigned char *hashalg, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a has algorithm value.
 *
 * Requires:
 *\li	'hashalg' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#DNS_R_UNKNOWN			type is unknown
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RCODE_H */
