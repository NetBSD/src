/*	$NetBSD: parseint.h,v 1.1.2.2 2024/02/24 13:07:26 martin Exp $	*/

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

#ifndef ISC_PARSEINT_H
#define ISC_PARSEINT_H 1

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/types.h>

/*! \file isc/parseint.h
 * \brief Parse integers, in a saner way than atoi() or strtoul() do.
 */

/***
 ***	Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
isc_parse_uint32(uint32_t *uip, const char *string, int base);

isc_result_t
isc_parse_uint16(uint16_t *uip, const char *string, int base);

isc_result_t
isc_parse_uint8(uint8_t *uip, const char *string, int base);
/*%<
 * Parse the null-terminated string 'string' containing a base 'base'
 * integer, storing the result in '*uip'.
 * The base is interpreted
 * as in strtoul().  Unlike strtoul(), leading whitespace, minus or
 * plus signs are not accepted, and all errors (including overflow)
 * are reported uniformly through the return value.
 *
 * Requires:
 *\li	'string' points to a null-terminated string
 *\li	0 <= 'base' <= 36
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_BADNUMBER   The string is not numeric (in the given base)
 *\li	#ISC_R_RANGE	  The number is not representable as the requested type.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_PARSEINT_H */
