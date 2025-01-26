/*	$NetBSD: stdtime.c,v 1.4 2025/01/26 16:25:38 christos Exp $	*/

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
#include <isc/time.h>
#include <isc/util.h>

#if defined(CLOCK_REALTIME_COARSE)
#define CLOCKSOURCE CLOCK_REALTIME_COARSE
#elif defined(CLOCK_REALTIME_FAST)
#define CLOCKSOURCE CLOCK_REALTIME_FAST
#else /* if defined(CLOCK_REALTIME_COARSE) */
#define CLOCKSOURCE CLOCK_REALTIME
#endif /* if defined(CLOCK_REALTIME_COARSE) */

isc_stdtime_t
isc_stdtime_now(void) {
	struct timespec ts;

	if (clock_gettime(CLOCKSOURCE, &ts) == -1) {
		FATAL_SYSERROR(errno, "clock_gettime()");
	}
	INSIST(ts.tv_sec > 0 && ts.tv_nsec >= 0 &&
	       ts.tv_nsec < (long)NS_PER_SEC);

	return (isc_stdtime_t)ts.tv_sec;
}

void
isc_stdtime_tostring(isc_stdtime_t t, char *out, size_t outlen) {
	time_t when;

	REQUIRE(out != NULL);
	REQUIRE(outlen >= 26);

	/* time_t and isc_stdtime_t might be different sizes */
	when = t;
	INSIST(ctime_r(&when, out) != NULL);
	*(out + strlen(out) - 1) = '\0';
}
