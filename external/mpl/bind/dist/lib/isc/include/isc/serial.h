/*	$NetBSD: serial.h,v 1.2.2.3 2019/01/18 08:49:58 pgoyette Exp $	*/

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

#include <stdbool.h>
#include <inttypes.h>

#include <isc/lang.h>
#include <isc/types.h>

/*! \file isc/serial.h
 *	\brief Implement 32 bit serial space arithmetic comparison functions.
 *	Note: Undefined results are returned as false.
 */

/***
 ***	Functions
 ***/

ISC_LANG_BEGINDECLS

bool
isc_serial_lt(uint32_t a, uint32_t b);
/*%<
 *	Return true if 'a' < 'b' otherwise false.
 */

bool
isc_serial_gt(uint32_t a, uint32_t b);
/*%<
 *	Return true if 'a' > 'b' otherwise false.
 */

bool
isc_serial_le(uint32_t a, uint32_t b);
/*%<
 *	Return true if 'a' <= 'b' otherwise false.
 */

bool
isc_serial_ge(uint32_t a, uint32_t b);
/*%<
 *	Return true if 'a' >= 'b' otherwise false.
 */

bool
isc_serial_eq(uint32_t a, uint32_t b);
/*%<
 *	Return true if 'a' == 'b' otherwise false.
 */

bool
isc_serial_ne(uint32_t a, uint32_t b);
/*%<
 *	Return true if 'a' != 'b' otherwise false.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_SERIAL_H */
