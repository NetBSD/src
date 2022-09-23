/*	$NetBSD: stdtime.c,v 1.1.1.5 2022/09/23 12:09:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
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
