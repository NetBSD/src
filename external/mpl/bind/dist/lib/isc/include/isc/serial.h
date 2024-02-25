/*	$NetBSD: serial.h,v 1.6.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

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
