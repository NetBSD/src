/*	$NetBSD: stdtime.c,v 1.1.2.2 2024/02/24 13:07:31 martin Exp $	*/

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

/*! \file */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h> /* NULL */
#include <stdlib.h> /* NULL */
#include <syslog.h>
#include <time.h>

#include <isc/stdtime.h>
#include <isc/strerr.h>
#include <isc/util.h>

#define NS_PER_S 1000000000 /*%< Nanoseconds per second. */

#if defined(CLOCK_REALTIME_COARSE)
#define CLOCKSOURCE CLOCK_REALTIME_COARSE
#elif defined(CLOCK_REALTIME_FAST)
#define CLOCKSOURCE CLOCK_REALTIME_FAST
#else /* if defined(CLOCK_REALTIME_COARSE) */
#define CLOCKSOURCE CLOCK_REALTIME
#endif /* if defined(CLOCK_REALTIME_COARSE) */

void
isc_stdtime_get(isc_stdtime_t *t) {
	REQUIRE(t != NULL);

	struct timespec ts;

	if (clock_gettime(CLOCKSOURCE, &ts) == -1) {
		char strbuf[ISC_STRERRORSIZE];
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_error_fatal(__FILE__, __LINE__, "clock_gettime failed: %s",
				strbuf);
	}

	REQUIRE(ts.tv_sec > 0 && ts.tv_nsec >= 0 && ts.tv_nsec < NS_PER_S);

	*t = (isc_stdtime_t)ts.tv_sec;
}

void
isc_stdtime_tostring(isc_stdtime_t t, char *out, size_t outlen) {
	time_t when;

	REQUIRE(out != NULL);
	REQUIRE(outlen >= 26);

	UNUSED(outlen);

	/* time_t and isc_stdtime_t might be different sizes */
	when = t;
	INSIST((ctime_r(&when, out) != NULL));
	*(out + strlen(out) - 1) = '\0';
}
