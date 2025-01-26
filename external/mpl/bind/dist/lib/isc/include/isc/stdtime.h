/*	$NetBSD: stdtime.h,v 1.3 2025/01/26 16:25:42 christos Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdlib.h>

#include <isc/lang.h>
#include <isc/time.h>

/*%
 * It's public information that 'isc_stdtime_t' is an unsigned integral type.
 * Applications that want maximum portability should not assume anything
 * about its size.
 */
typedef uint32_t isc_stdtime_t;

ISC_LANG_BEGINDECLS

isc_stdtime_t
isc_stdtime_now(void);
/*%<
 * Return the number of seconds since 00:00:00 UTC, January 1, 1970.
 */

void
isc_stdtime_tostring(isc_stdtime_t t, char *out, size_t outlen);
/*
 * Convert 't' into a null-terminated string of the form
 * "Wed Jun 30 21:49:08 1993". Store the string in the 'out'
 * buffer.
 *
 * Requires:
 *
 *	't' is a valid time.
 *	'out' is a valid pointer.
 *	'outlen' is at least 26.
 */

ISC_LANG_ENDDECLS
