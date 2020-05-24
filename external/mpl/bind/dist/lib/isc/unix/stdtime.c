/*	$NetBSD: stdtime.c,v 1.4 2020/05/24 19:46:27 christos Exp $	*/

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
