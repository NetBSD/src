/*	$NetBSD: cert.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DNS_CERT_H
#define DNS_CERT_H 1

/*! \file dns/cert.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_cert_fromtext(dns_cert_t *certp, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a certificate type.
 * The text may contain either a mnemonic type name or a decimal type number.
 *
 * Requires:
 *\li	'certp' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_RANGE			numeric type is out of range
 *\li	#DNS_R_UNKNOWN			mnemonic type is unknown
 */

isc_result_t
dns_cert_totext(dns_cert_t cert, isc_buffer_t *target);
/*%<
 * Put a textual representation of certificate type 'cert' into 'target'.
 *
 * Requires:
 *\li	'cert' is a valid cert.
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

ISC_LANG_ENDDECLS

#endif /* DNS_CERT_H */
