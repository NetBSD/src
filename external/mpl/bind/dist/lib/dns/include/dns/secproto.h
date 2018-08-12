/*	$NetBSD: secproto.h,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

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


#ifndef DNS_SECPROTO_H
#define DNS_SECPROTO_H 1

/*! \file dns/secproto.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_secproto_fromtext(dns_secproto_t *secprotop, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNSSEC security protocol value.
 * The text may contain either a mnemonic protocol name or a decimal protocol
 * number.
 *
 * Requires:
 *\li	'secprotop' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_RANGE			numeric type is out of range
 *\li	DNS_R_UNKNOWN			mnemonic type is unknown
 */

isc_result_t
dns_secproto_totext(dns_secproto_t secproto, isc_buffer_t *target);
/*%<
 * Put a textual representation of the DNSSEC security protocol 'secproto'
 * into 'target'.
 *
 * Requires:
 *\li	'secproto' is a valid secproto.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures,
 *	if the result is success:
 *	\li	The used space in 'target' is updated.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_NOSPACE			target buffer is too small
 */

ISC_LANG_ENDDECLS

#endif /* DNS_SECPROTO_H */
