/*	$NetBSD: rdatatype.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

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


#ifndef DNS_RDATATYPE_H
#define DNS_RDATATYPE_H 1

/*! \file dns/rdatatype.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rdatatype_fromtext(dns_rdatatype_t *typep, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNS rdata type.
 *
 * Requires:
 *\li	'typep' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	DNS_R_UNKNOWN			type is unknown
 */

isc_result_t
dns_rdatatype_totext(dns_rdatatype_t type, isc_buffer_t *target);
/*%<
 * Put a textual representation of type 'type' into 'target'.
 *
 * Requires:
 *\li	'type' is a valid type.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures,
 *	if the result is success:
 *\li		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

isc_result_t
dns_rdatatype_tounknowntext(dns_rdatatype_t type, isc_buffer_t *target);
/*%<
 * Put textual RFC3597 TYPEXXXX representation of type 'type' into
 * 'target'.
 *
 * Requires:
 *\li	'type' is a valid type.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures,
 *	if the result is success:
 *\li		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

void
dns_rdatatype_format(dns_rdatatype_t rdtype,
		     char *array, unsigned int size);
/*%<
 * Format a human-readable representation of the type 'rdtype'
 * into the character array 'array', which is of size 'size'.
 * The resulting string is guaranteed to be null-terminated.
 */

#define DNS_RDATATYPE_FORMATSIZE sizeof("NSEC3PARAM")

/*%<
 * Minimum size of array to pass to dns_rdatatype_format().
 * May need to be adjusted if a new RR type with a very long
 * name is defined.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RDATATYPE_H */
