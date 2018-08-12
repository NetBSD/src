/*	$NetBSD: stdtime.h,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

/*! \file */

#include <isc/lang.h>
#include <isc/int.h>

/*%
 * It's public information that 'isc_stdtime_t' is an unsigned integral type.
 * Applications that want maximum portability should not assume anything
 * about its size.
 */
typedef isc_uint32_t isc_stdtime_t;

/* but this flag helps... */
#define STDTIME_ON_32BITS	1

/*
 * isc_stdtime32_t is a 32-bit version of isc_stdtime_t.  A variable of this
 * type should only be used as an opaque integer (e.g.,) to compare two
 * time values.
 */
typedef isc_uint32_t isc_stdtime32_t;

ISC_LANG_BEGINDECLS
/* */
void
isc_stdtime_get(isc_stdtime_t *t);
/*%<
 * Set 't' to the number of seconds since 00:00:00 UTC, January 1, 1970.
 *
 * Requires:
 *
 *\li	't' is a valid pointer.
 */

#define isc_stdtime_convert32(t, t32p) (*(t32p) = t)
/*
 * Convert the standard time to its 32-bit version.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_STDTIME_H */
