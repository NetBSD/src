/*	$NetBSD: serial.c,v 1.3 2019/01/09 16:55:14 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stdbool.h>
#include <inttypes.h>

#include <isc/serial.h>

bool
isc_serial_lt(uint32_t a, uint32_t b) {
	/*
	 * Undefined => false
	 */
	if (a == (b ^ 0x80000000U))
		return (false);
	return (((int32_t)(a - b) < 0) ? true : false);
}

bool
isc_serial_gt(uint32_t a, uint32_t b) {
	return (((int32_t)(a - b) > 0) ? true : false);
}

bool
isc_serial_le(uint32_t a, uint32_t b) {
	return ((a == b) ? true : isc_serial_lt(a, b));
}

bool
isc_serial_ge(uint32_t a, uint32_t b) {
	return ((a == b) ? true : isc_serial_gt(a, b));
}

bool
isc_serial_eq(uint32_t a, uint32_t b) {
	return ((a == b) ? true : false);
}

bool
isc_serial_ne(uint32_t a, uint32_t b) {
	return ((a != b) ? true : false);
}
