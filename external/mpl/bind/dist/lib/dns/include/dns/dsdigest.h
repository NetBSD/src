/*	$NetBSD: dsdigest.h,v 1.5.2.1 2024/02/25 15:46:56 martin Exp $	*/

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

/*! \file dns/dsdigest.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_dsdigest_fromtext(dns_dsdigest_t *dsdigestp, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DS digest type value.
 * The text may contain either a mnemonic digest name or a decimal
 * digest number.
 *
 * Requires:
 *\li	'dsdigestp' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_RANGE			numeric type is out of range
 *\li	DNS_R_UNKNOWN			mnemonic type is unknown
 */

isc_result_t
dns_dsdigest_totext(dns_dsdigest_t dsdigest, isc_buffer_t *target);
/*%<
 * Put a textual representation of the DS digest type 'dsdigest'
 * into 'target'.
 *
 * Requires:
 *\li	'dsdigest' is a valid dsdigest.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures,
 *	if the result is success:
 *\li		The used space in 'target' is updated.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_NOSPACE			target buffer is too small
 */

#define DNS_DSDIGEST_FORMATSIZE 20
void
dns_dsdigest_format(dns_dsdigest_t typ, char *cp, unsigned int size);
/*%<
 * Wrapper for dns_dsdigest_totext(), writing text into 'cp'
 */

ISC_LANG_ENDDECLS
