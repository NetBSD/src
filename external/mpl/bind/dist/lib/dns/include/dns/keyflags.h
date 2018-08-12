/*	$NetBSD: keyflags.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DNS_KEYFLAGS_H
#define DNS_KEYFLAGS_H 1

/*! \file dns/keyflags.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_keyflags_fromtext(dns_keyflags_t *flagsp, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNSSEC KEY flags value.
 * The text may contain either a set of flag mnemonics separated by
 * vertical bars or a decimal flags value.  For compatibility with
 * older versions of BIND and the DNSSEC signer, octal values
 * prefixed with a zero and hexadecimal values prefixed with "0x"
 * are also accepted.
 *
 * Requires:
 *\li	'flagsp' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_RANGE			numeric flag value is out of range
 *\li	DNS_R_UNKNOWN			mnemonic flag is unknown
 */

ISC_LANG_ENDDECLS

#endif /* DNS_KEYFLAGS_H */
