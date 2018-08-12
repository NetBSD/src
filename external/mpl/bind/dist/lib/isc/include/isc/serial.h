/*	$NetBSD: serial.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_SERIAL_H
#define ISC_SERIAL_H 1

#include <isc/lang.h>
#include <isc/types.h>

/*! \file isc/serial.h
 *	\brief Implement 32 bit serial space arithmetic comparison functions.
 *	Note: Undefined results are returned as ISC_FALSE.
 */

/***
 ***	Functions
 ***/

ISC_LANG_BEGINDECLS

isc_boolean_t
isc_serial_lt(isc_uint32_t a, isc_uint32_t b);
/*%<
 *	Return true if 'a' < 'b' otherwise false.
 */

isc_boolean_t
isc_serial_gt(isc_uint32_t a, isc_uint32_t b);
/*%<
 *	Return true if 'a' > 'b' otherwise false.
 */

isc_boolean_t
isc_serial_le(isc_uint32_t a, isc_uint32_t b);
/*%<
 *	Return true if 'a' <= 'b' otherwise false.
 */

isc_boolean_t
isc_serial_ge(isc_uint32_t a, isc_uint32_t b);
/*%<
 *	Return true if 'a' >= 'b' otherwise false.
 */

isc_boolean_t
isc_serial_eq(isc_uint32_t a, isc_uint32_t b);
/*%<
 *	Return true if 'a' == 'b' otherwise false.
 */

isc_boolean_t
isc_serial_ne(isc_uint32_t a, isc_uint32_t b);
/*%<
 *	Return true if 'a' != 'b' otherwise false.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_SERIAL_H */
