/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <atf-c.h>

#include <limits.h>
#include <time.h>

ATF_TC(timespec_getres);
ATF_TC_HEAD(timespec_getres, tc)
{
	atf_tc_set_md_var(tc, "descr", "Resolution tests for timespec_getres");
}

ATF_TC_BODY(timespec_getres, tc)
{
	struct timespec ts, ts2;

	ATF_REQUIRE_EQ(timespec_getres(&ts, TIME_MONOTONIC), TIME_MONOTONIC);
	ATF_REQUIRE_EQ(clock_getres(CLOCK_MONOTONIC, &ts2), 0);

	ATF_REQUIRE_EQ(ts.tv_sec, ts2.tv_sec);
	ATF_REQUIRE_EQ(ts.tv_nsec, ts2.tv_nsec);

	ATF_REQUIRE_EQ(timespec_getres(&ts, TIME_UTC), TIME_UTC);
	ATF_REQUIRE_EQ(clock_getres(CLOCK_REALTIME, &ts2), 0);

	ATF_REQUIRE_EQ(ts.tv_sec, ts2.tv_sec);
	ATF_REQUIRE_EQ(ts.tv_nsec, ts2.tv_nsec);

	/* now an invalid value */
	ATF_REQUIRE_EQ(timespec_getres(&ts, INT_MAX), 0);
}

ATF_TC(timespec_get);
ATF_TC_HEAD(timespec_get, tc)
{
	atf_tc_set_md_var(tc, "descr", "Basic tests for timespec_get");
}

ATF_TC_BODY(timespec_get, tc)
{
	struct timespec ts, ts2;

	ATF_REQUIRE_EQ(timespec_get(&ts, TIME_UTC), TIME_UTC);
	ATF_REQUIRE_EQ(clock_gettime(CLOCK_REALTIME, &ts2), 0);

	/*
	 * basically test that these clocks (which should be the same source)
	 * aren't too wildly apart...
	 */

	if (ts2.tv_sec >= ts.tv_sec) {
		ATF_REQUIRE((ts2.tv_sec - ts.tv_sec) < 86400);
	} else {
		ATF_REQUIRE((ts.tv_sec - ts2.tv_sec) < 86400);
	}

	/* now an invalid value */
	ATF_REQUIRE_EQ(timespec_get(&ts, INT_MAX), 0);
}

ATF_TC(timespec_get_monotonic);
ATF_TC_HEAD(timespec_get_monotonic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Monotonic tests for timespec_getres");
}

ATF_TC_BODY(timespec_get_monotonic, tc)
{
	struct timespec ts, ts2;

	ATF_REQUIRE_EQ(timespec_get(&ts, TIME_MONOTONIC), TIME_MONOTONIC);
	ATF_REQUIRE_EQ(clock_gettime(CLOCK_MONOTONIC, &ts2), 0);

	/*
	 * basically test that these clocks (which should be the same source)
	 * aren't too wildly apart...
	 */

	ATF_REQUIRE(ts2.tv_sec >= ts.tv_sec);
	ATF_REQUIRE((ts2.tv_sec - ts.tv_sec) < 86400);

	/* test that it's actually monotonic */
	ATF_REQUIRE_EQ(timespec_get(&ts2, TIME_MONOTONIC), TIME_MONOTONIC);
	ATF_REQUIRE(ts2.tv_sec >= ts.tv_sec);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, timespec_getres);
	ATF_TP_ADD_TC(tp, timespec_get);
	ATF_TP_ADD_TC(tp, timespec_get_monotonic);

	return atf_no_error();
}

