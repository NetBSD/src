/*	$NetBSD: stdtime.h,v 1.1.1.2 2019/01/09 16:48:20 christos Exp $	*/

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


#ifndef ISC_STDTIME_H
#define ISC_STDTIME_H 1

#include <isc/lang.h>
#include <inttypes.h>

/*
 * It's public information that 'isc_stdtime_t' is an unsigned integral type.
 * Applications that want maximum portability should not assume anything
 * about its size.
 */
typedef uint32_t isc_stdtime_t;

ISC_LANG_BEGINDECLS

void
isc_stdtime_get(isc_stdtime_t *t);
/*
 * Set 't' to the number of seconds since 00:00:00 UTC, January 1, 1970.
 *
 * Requires:
 *
 *	't' is a valid pointer.
 */

#define isc_stdtime_convert32(t, t32p) (*(t32p) = t)
/*
 * Convert the standard time to its 32-bit version.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_STDTIME_H */
