/*	$NetBSD: stdtime.c,v 1.4 2020/08/03 17:23:43 christos Exp $	*/

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

#include <time.h>

#include <isc/assertions.h>
#include <isc/stdtime.h>
#include <isc/util.h>

void
isc_stdtime_get(isc_stdtime_t *t) {
	/*
	 * Set 't' to the number of seconds past 00:00:00 UTC, January 1, 1970.
	 */

	REQUIRE(t != NULL);

	(void)_time32(t);
}

void
isc_stdtime_tostring(isc_stdtime_t t, char *out, size_t outlen) {
	time_t when;

	REQUIRE(out != NULL);
	/* Minimum buffer as per ctime_r() specification. */
	REQUIRE(outlen >= 26);

	/* time_t and isc_stdtime_t might be different sizes */
	when = t;
	INSIST((ctime_s(out, outlen, &when) == 0));
	*(out + strlen(out) - 1) = '\0';
}
