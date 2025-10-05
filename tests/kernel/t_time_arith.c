/*	$NetBSD: t_time_arith.c,v 1.4 2025/10/05 18:46:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2024-2025 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_time_arith.c,v 1.4 2025/10/05 18:46:26 riastradh Exp $");

#include <sys/timearith.h>

#include <atf-c.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include "h_macros.h"

enum { HZ = 100 };

int hz = HZ;
int tick = 1000000/HZ;

static sig_atomic_t jmp_en;
static int jmp_sig;
static jmp_buf jmp;

static void
handle_signal(int signo)
{
	const int errno_save = errno;
	char buf[32];

	snprintf_ss(buf, sizeof(buf), "signal %d\n", signo);
	(void)write(STDERR_FILENO, buf, strlen(buf));

	errno = errno_save;

	if (jmp_en) {
		jmp_sig = signo;
		jmp_en = 0;
		longjmp(jmp, 1);
	} else {
		raise_default_signal(signo);
	}
}

const struct itimer_transition {
	struct itimerspec	it_time;
	struct timespec		it_now;
	struct timespec		it_next;
	int			it_overruns;
	const char		*it_xfail;
} itimer_transitions[] = {
	/*
	 * Fired more than one interval early -- treat clock as wound
	 * backwards, not counting overruns.  Advance to the next
	 * integral multiple of it_interval starting from it_value.
	 */
	[0] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {0,1}, {1,0}, 0,
	       NULL},
	[1] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {0,500000000}, {1,0}, 0,
	       NULL},
	[2] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {0,999999999}, {1,0}, 0,
	       NULL},
	[3] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {1,0}, {2,0}, 0,
	       NULL},
	[4] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {1,1}, {2,0}, 0,
	       NULL},
	[5] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {1,500000000}, {2,0}, 0,
	       NULL},
	[6] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {1,999999999}, {2,0}, 0,
	       NULL},

	/*
	 * Fired exactly one interval early.  Treat this too as clock
	 * wound backwards.
	 */
	[7] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {2,0}, {3,0}, 0,
	       NULL},

	/*
	 * Fired less than one interval early -- callouts and real-time
	 * clock might not be perfectly synced, counted as zero
	 * overruns.  Advance by one interval from the scheduled time.
	 */
	[8] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {2,1}, {3,0}, 0,
	       NULL},
	[9] = {{.it_value = {3,0}, .it_interval = {1,0}},
	       {2,500000000}, {3,0}, 0,
	       NULL},
	[10] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{2,999999999}, {3,0}, 0,
		NULL},

	/*
	 * Fired exactly on time.  Advance by one interval.
	 */
	[11] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{3,0}, {4,0}, 0, NULL},

	/*
	 * Fired late by less than one interval -- callouts and
	 * real-time clock might not be prefectly synced, counted as
	 * zero overruns.  Advance by one interval from the scheduled
	 * time (even if it's very close to a full interval).
	 */
	[12] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{3,1}, {4,0}, 0, NULL},
	[14] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{3,500000000}, {4,0}, 0, NULL},
	[15] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{3,999999999}, {4,0}, 0, NULL},

	/*
	 * Fired late by exactly one interval -- treat it as overrun.
	 */
	[16] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{4,0}, {5,0}, 1,
		NULL},

	/*
	 * Fired late by more than one interval but less than two --
	 * overrun.
	 */
	[17] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{4,1}, {5,0}, 1,
		NULL},
	[18] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{4,500000000}, {5,0}, 1,
		NULL},
	[19] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{4,999999999}, {5,0}, 1,
		NULL},

	/*
	 * Fired late by exactly two intervals -- two overruns.
	 */
	[20] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{5,0}, {6,0}, 2,
		NULL},

	/*
	 * Fired late by more intervals plus slop, up to 32.
	 *
	 * XXX Define DELAYTIMER_MAX so we can write it in terms of
	 * that.
	 */
	[21] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{13,123456789}, {14,0}, 10,
		NULL},
	[22] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{34,999999999}, {35,0}, 31,
		NULL},

	/*
	 * Fired late by roughly INT_MAX intervals.
	 */
	[23] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{(time_t)3 + INT_MAX - 1, 0},
		{(time_t)3 + INT_MAX, 0},
		INT_MAX - 1,
		NULL},
	[24] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{(time_t)3 + INT_MAX, 0},
		{(time_t)3 + INT_MAX + 1, 0},
		INT_MAX,
		NULL},
	[25] = {{.it_value = {3,0}, .it_interval = {1,0}},
		{(time_t)3 + INT_MAX + 1, 0},
		{(time_t)3 + INT_MAX + 2, 0},
		INT_MAX,
		NULL},

	/* (2^63 - 1) ns */
	[26] = {{.it_value = {3,0}, .it_interval = {9223372036,854775807}},
		{3,1}, {9223372039,854775807}, 0, NULL},
	/* 2^63 ns */
	[27] = {{.it_value = {3,0}, .it_interval = {9223372036,854775808}},
		{3,1}, {9223372039,854775808}, 0, NULL},
	/* (2^63 + 1) ns */
	[28] = {{.it_value = {3,0}, .it_interval = {9223372036,854775809}},
		{3,1}, {9223372039,854775809}, 0, NULL},

	/*
	 * Overflows -- we should (XXX but currently don't) reject
	 * intervals of at least 2^64 nanoseconds up front, since this
	 * is more time than it is reasonable to wait (more than 584
	 * years).
	 */

	/* (2^64 - 1) ns */
	[29] = {{.it_value = {3,0}, .it_interval = {18446744073,709551615}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* 2^64 ns */
	[30] = {{.it_value = {3,0}, .it_interval = {18446744073,709551616}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* (2^64 + 1) ns */
	[31] = {{.it_value = {3,0}, .it_interval = {18446744073,709551617}},
		{2,999999999}, {0,0}, 0,
		NULL},

	/* (2^63 - 1) us */
	[32] = {{.it_value = {3,0}, .it_interval = {9223372036854,775807}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* 2^63 us */
	[33] = {{.it_value = {3,0}, .it_interval = {9223372036854,775808}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* (2^63 + 1) us */
	[34] = {{.it_value = {3,0}, .it_interval = {9223372036854,775809}},
		{2,999999999}, {0,0}, 0,
		NULL},

	/* (2^64 - 1) us */
	[35] = {{.it_value = {3,0}, .it_interval = {18446744073709,551615}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* 2^64 us */
	[36] = {{.it_value = {3,0}, .it_interval = {18446744073709,551616}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* (2^64 + 1) us */
	[37] = {{.it_value = {3,0}, .it_interval = {18446744073709,551617}},
		{2,999999999}, {0,0}, 0,
		NULL},

	/* (2^63 - 1) ms */
	[38] = {{.it_value = {3,0}, .it_interval = {9223372036854775,807}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* 2^63 ms */
	[39] = {{.it_value = {3,0}, .it_interval = {9223372036854775,808}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* (2^63 + 1) ms */
	[40] = {{.it_value = {3,0}, .it_interval = {9223372036854775,809}},
		{2,999999999}, {0,0}, 0,
		NULL},

	/* (2^64 - 1) ms */
	[41] = {{.it_value = {3,0}, .it_interval = {18446744073709551,615}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* 2^64 ms */
	[42] = {{.it_value = {3,0}, .it_interval = {18446744073709551,616}},
		{2,999999999}, {0,0}, 0,
		NULL},
	/* (2^64 + 1) ms */
	[43] = {{.it_value = {3,0}, .it_interval = {18446744073709551,617}},
		{2,999999999}, {0,0}, 0,
		NULL},

	/* invalid intervals */
	[44] = {{.it_value = {3,0}, .it_interval = {-1,0}},
		{3,1}, {0,0}, 0, NULL},
	[45] = {{.it_value = {3,0}, .it_interval = {0,-1}},
		{3,1}, {0,0}, 0, NULL},
	[46] = {{.it_value = {3,0}, .it_interval = {0,1000000000}},
		{3,1}, {0,0}, 0, NULL},

	/*
	 * Overflow nanosecond arithmetic.  The magic interval number
	 * here is ceiling(INT64_MAX/2) nanoseconds.  The interval
	 * start value will be rounded to an integral number of ticks,
	 * so rather than write exactly `4611686018,427387905', just
	 * round up the `now' value to the next second.  This forces an
	 * overrun _and_ triggers int64_t arithmetic overflow.
	 */
	[47] = {{.it_value = {0,1},
		 .it_interval = {4611686018,427387904}},
		/* XXX needless overflow */
		{4611686019,0}, {0,0}, 1,
		NULL},

	/* interval ~ 1/4 * (2^63 - 1) ns, now ~ 3/4 * (2^63 - 1) ns */
	[48] = {{.it_value = {0,1},
		 .it_interval = {2305843009,213693952}},
		/* XXX needless overflow */
		{6917529028,0}, {0,0}, 3,
		NULL},
	[49] = {{.it_value = {6917529027,0},
		 .it_interval = {2305843009,213693952}},
		{6917529028,0}, {9223372036,213693952}, 0, NULL},
	[50] = {{.it_value = {6917529029,0},
		 .it_interval = {2305843009,213693952}},
		{6917529028,0}, {6917529029,0}, 0,
		NULL},

	/* interval ~ 1/2 * (2^63 - 1) ns, now ~ 3/4 * (2^63 - 1) ns */
	[51] = {{.it_value = {0,1},
		 .it_interval = {4611686018,427387904}},
		/* XXX needless overflow */
		{6917529028,0}, {0,0}, 1,
		NULL},
	[52] = {{.it_value = {2305843009,213693951}, /* ~1/4 * (2^63 - 1) */
		 .it_interval = {4611686018,427387904}},
		/* XXX needless overflow */
		{6917529028,0}, {0,0}, 1,
		NULL},
	[54] = {{.it_value = {6917529027,0},
		 .it_interval = {4611686018,427387904}},
		{6917529028,0}, {11529215045,427387904}, 0, NULL},
	[55] = {{.it_value = {6917529029,0},
		 .it_interval = {4611686018,427387904}},
		{6917529028,0}, {6917529029,0}, 0,
		NULL},

	[56] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		/* XXX needless overflow */
		{INT64_MAX - 2,999999999}, {0,0}, 0,
		NULL},
	[57] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		{INT64_MAX - 1,0}, {INT64_MAX,0}, 0, NULL},
	[58] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		{INT64_MAX - 1,1}, {INT64_MAX,0}, 0, NULL},
	[59] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		{INT64_MAX - 1,999999999}, {INT64_MAX,0}, 0, NULL},
	[60] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		{INT64_MAX,0}, {0,0}, 0,
		NULL},
	[61] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		{INT64_MAX,1}, {0,0}, 0,
		NULL},
	[62] = {{.it_value = {INT64_MAX - 1,0}, .it_interval = {1,0}},
		{INT64_MAX,999999999}, {0,0}, 0,
		NULL},

	[63] = {{.it_value = {INT64_MAX,0}, .it_interval = {1,0}},
		{INT64_MAX - 1,1}, {0,0}, 0,
		NULL},
	[64] = {{.it_value = {INT64_MAX,0}, .it_interval = {1,0}},
		{INT64_MAX - 1,999999999}, {0,0}, 0,
		NULL},
	[65] = {{.it_value = {INT64_MAX,0}, .it_interval = {1,0}},
		{INT64_MAX,0}, {0,0}, 0,
		NULL},
	[66] = {{.it_value = {INT64_MAX,0}, .it_interval = {1,0}},
		{INT64_MAX,1}, {0,0}, 0,
		NULL},
	[67] = {{.it_value = {INT64_MAX,0}, .it_interval = {1,0}},
		{INT64_MAX,999999999}, {0,0}, 0,
		NULL},
};

ATF_TC(itimer_transitions);
ATF_TC_HEAD(itimer_transitions, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests interval timer transitions");
}
ATF_TC_BODY(itimer_transitions, tc)
{
	volatile unsigned i;

	REQUIRE_LIBC(signal(SIGFPE, handle_signal), SIG_ERR);
	REQUIRE_LIBC(signal(SIGABRT, handle_signal), SIG_ERR);

	for (i = 0; i < __arraycount(itimer_transitions); i++) {
		struct itimer_transition it = itimer_transitions[i];
		struct timespec next;
		int overruns;
		volatile bool aborted = true;
		volatile bool expect_abort = false;

		fprintf(stderr, "case %u\n", i);

		if (it.it_xfail)
			atf_tc_expect_fail("%s", it.it_xfail);

		if (itimespecfix(&it.it_time.it_value) != 0 ||
		    itimespecfix(&it.it_time.it_interval) != 0) {
			fprintf(stderr, "rejected by itimerspecfix\n");
			expect_abort = true;
		}

		if (setjmp(jmp) == 0) {
			jmp_en = 1;
			itimer_transition(&it.it_time, &it.it_now,
			    &next, &overruns);
			jmp_en = 0;
			aborted = false;
		}
		ATF_CHECK(!jmp_en);
		jmp_en = 0;	/* paranoia */
		if (expect_abort) {
			fprintf(stderr, "expected abort\n");
			ATF_CHECK_MSG(aborted,
			    "[%u] missing invariant assertion", i);
			ATF_CHECK_MSG(jmp_sig == SIGABRT,
			    "[%u] missing invariant assertion", i);
		} else {
			ATF_CHECK_MSG(!aborted, "[%u] raised signal %d: %s", i,
			    jmp_sig, strsignal(jmp_sig));
		}

		ATF_CHECK_MSG((next.tv_sec == it.it_next.tv_sec &&
			next.tv_nsec == it.it_next.tv_nsec),
		    "[%u] periodic intervals of %lld.%09d from %lld.%09d"
		    " last expired at %lld.%09d:"
		    " next expiry at %lld.%09d, expected %lld.%09d", i,
		    (long long)it.it_time.it_interval.tv_sec,
		    (int)it.it_time.it_interval.tv_nsec,
		    (long long)it.it_time.it_value.tv_sec,
		    (int)it.it_time.it_value.tv_nsec,
		    (long long)it.it_now.tv_sec, (int)it.it_now.tv_nsec,
		    (long long)next.tv_sec, (int)next.tv_nsec,
		    (long long)it.it_next.tv_sec, (int)it.it_next.tv_nsec);
		ATF_CHECK_EQ_MSG(overruns, it.it_overruns,
		    "[%u] periodic intervals of %lld.%09d from %lld.%09d"
		    " last expired at %lld.%09d:"
		    " overruns %d, expected %d", i,
		    (long long)it.it_time.it_interval.tv_sec,
		    (int)it.it_time.it_interval.tv_nsec,
		    (long long)it.it_time.it_value.tv_sec,
		    (int)it.it_time.it_value.tv_nsec,
		    (long long)it.it_now.tv_sec, (int)it.it_now.tv_nsec,
		    overruns, it.it_overruns);

		if (it.it_xfail)
			atf_tc_expect_pass();
	}
}

/*
 *                        { 0,                  if t <= 0;
 * tvtohz(t sec) @ f Hz = { ceil(t/(1/f)),      if that's below INT_MAX;
 *                        { INT_MAX,            otherwise.
 */

const struct tvtohz_case {
	int tv_hz;
	struct timeval tv_tv;
	int tv_ticks;
	const char *tv_xfail;
} tvtohz_cases[] = {
	/*
	 * hz = 10
	 */

	/* negative inputs yield 0 ticks */
	[0] = {10, {.tv_sec = -1, .tv_usec = 0}, 0, NULL},
	[1] = {10, {.tv_sec = -1, .tv_usec = 999999}, 0, NULL},

	/* zero input yields 0 ticks */
	[2] = {10, {.tv_sec = 0, .tv_usec = 0}, 0, NULL},

	/*
	 * Nonzero input always yields >=2 ticks, because the time from
	 * now until the next tick may be arbitrarily short, and we
	 * need to wait one full tick, so we have to wait for two
	 * ticks.
	 */
	[3] = {10, {.tv_sec = 0, .tv_usec = 1}, 2, NULL},
	[4] = {10, {.tv_sec = 0, .tv_usec = 2}, 2, NULL},
	[5] = {10, {.tv_sec = 0, .tv_usec = 99999}, 2, NULL},
	[6] = {10, {.tv_sec = 0, .tv_usec = 100000}, 2, NULL},
	[7] = {10, {.tv_sec = 0, .tv_usec = 100001}, 3, NULL},
	[8] = {10, {.tv_sec = 0, .tv_usec = 100002}, 3, NULL},
	[9] = {10, {.tv_sec = 0, .tv_usec = 199999}, 3, NULL},
	[10] = {10, {.tv_sec = 0, .tv_usec = 200000}, 3, NULL},
	[11] = {10, {.tv_sec = 0, .tv_usec = 200001}, 4, NULL},
	[12] = {10, {.tv_sec = 0, .tv_usec = 200002}, 4, NULL},
	[13] = {10, {.tv_sec = 0, .tv_usec = 999999}, 11, NULL},
	[14] = {10, {.tv_sec = 1, .tv_usec = 0}, 11, NULL},
	[15] = {10, {.tv_sec = 1, .tv_usec = 1}, 12, NULL},
	[16] = {10, {.tv_sec = 1, .tv_usec = 2}, 12, NULL},
	/* .tv_sec ~ INT32_MAX/1000000 */
	[17] = {10, {.tv_sec = 2147, .tv_usec = 999999}, 21481, NULL},
	[18] = {10, {.tv_sec = 2148, .tv_usec = 0}, 21481, NULL},
	[19] = {10, {.tv_sec = 2148, .tv_usec = 1}, 21482, NULL},
	[20] = {10, {.tv_sec = 2148, .tv_usec = 2}, 21482, NULL},
	/* .tv_sec ~ INT32_MAX/hz */
	[21] = {10, {.tv_sec = 214748364, .tv_usec = 499999}, 2147483646,
		NULL},
	/* saturate at INT_MAX = 2^31 - 1 ticks */
	[22] = {10, {.tv_sec = 214748364, .tv_usec = 500000}, 2147483646,
		NULL},
	[23] = {10, {.tv_sec = 214748364, .tv_usec = 500001}, 2147483647,
		NULL},
	[24] = {10, {.tv_sec = 214748364, .tv_usec = 500002}, 2147483647,
		NULL},
	[25] = {10, {.tv_sec = 214748364, .tv_usec = 599999}, 2147483647,
		NULL},
	[26] = {10, {.tv_sec = 214748364, .tv_usec = 600000}, 2147483647,
		NULL},
	[27] = {10, {.tv_sec = 214748364, .tv_usec = 999999}, 2147483647,
		NULL},
	[28] = {10, {.tv_sec = 214748365, .tv_usec = 0}, 2147483647,
		NULL},
	[29] = {10, {.tv_sec = 214748365, .tv_usec = 1}, 2147483647,
		NULL},
	[30] = {10, {.tv_sec = 214748365, .tv_usec = 2}, 2147483647,
		NULL},
	[31] = {10, {.tv_sec = (time_t)INT_MAX + 1, .tv_usec = 123456},
		INT_MAX, NULL},
	/* .tv_sec ~ INT64_MAX/1000000, overflows to INT_MAX ticks */
	[32] = {10, {.tv_sec = 9223372036854, .tv_usec = 999999},
		INT_MAX, NULL},
	[33] = {10, {.tv_sec = 9223372036855, .tv_usec = 0},
		INT_MAX, NULL},
	[34] = {10, {.tv_sec = 9223372036855, .tv_usec = 1},
		INT_MAX, NULL},
	[35] = {10, {.tv_sec = 9223372036855, .tv_usec = 2},
		INT_MAX, NULL},
	/* .tv_sec ~ INT64_MAX/hz, overflows to INT_MAX ticks */
	[36] = {10, {.tv_sec = 922337203685477580, .tv_usec = 999999},
		INT_MAX, NULL},
	[37] = {10, {.tv_sec = 922337203685477581, .tv_usec = 0},
		INT_MAX, NULL},
	[38] = {10, {.tv_sec = 922337203685477581, .tv_usec = 1},
		INT_MAX, NULL},
	[39] = {10, {.tv_sec = 922337203685477581, .tv_usec = 2},
		INT_MAX, NULL},
	[40] = {10, {.tv_sec = (time_t)INT_MAX + 1, .tv_usec = 123456},
		INT_MAX, NULL},

	/*
	 * hz = 100
	 */

	[41] = {100, {.tv_sec = -1, .tv_usec = 0}, 0, NULL},
	[42] = {100, {.tv_sec = -1, .tv_usec = 999999}, 0, NULL},
	[43] = {100, {.tv_sec = 0, .tv_usec = 0}, 0, NULL},
	[44] = {100, {.tv_sec = 0, .tv_usec = 1}, 2, NULL},
	[45] = {100, {.tv_sec = 0, .tv_usec = 2}, 2, NULL},
	[46] = {100, {.tv_sec = 0, .tv_usec = 9999}, 2, NULL},
	[47] = {100, {.tv_sec = 0, .tv_usec = 10000}, 2, NULL},
	[48] = {100, {.tv_sec = 0, .tv_usec = 10001}, 3, NULL},
	[49] = {100, {.tv_sec = 0, .tv_usec = 10002}, 3, NULL},
	[50] = {100, {.tv_sec = 0, .tv_usec = 19999}, 3, NULL},
	[51] = {100, {.tv_sec = 0, .tv_usec = 20000}, 3, NULL},
	[52] = {100, {.tv_sec = 0, .tv_usec = 20001}, 4, NULL},
	[53] = {100, {.tv_sec = 0, .tv_usec = 20002}, 4, NULL},
	[54] = {100, {.tv_sec = 0, .tv_usec = 99999}, 11, NULL},
	[55] = {100, {.tv_sec = 0, .tv_usec = 100000}, 11, NULL},
	[56] = {100, {.tv_sec = 0, .tv_usec = 100001}, 12, NULL},
	[57] = {100, {.tv_sec = 0, .tv_usec = 100002}, 12, NULL},
	[58] = {100, {.tv_sec = 0, .tv_usec = 999999}, 101, NULL},
	[59] = {100, {.tv_sec = 1, .tv_usec = 0}, 101, NULL},
	[60] = {100, {.tv_sec = 1, .tv_usec = 1}, 102, NULL},
	[61] = {100, {.tv_sec = 1, .tv_usec = 2}, 102, NULL},
	/* .tv_sec ~ INT32_MAX/1000000 */
	[62] = {100, {.tv_sec = 2147, .tv_usec = 999999}, 214801, NULL},
	[63] = {100, {.tv_sec = 2148, .tv_usec = 0}, 214801, NULL},
	[64] = {100, {.tv_sec = 2148, .tv_usec = 1}, 214802, NULL},
	[65] = {100, {.tv_sec = 2148, .tv_usec = 2}, 214802, NULL},
	/* .tv_sec ~ INT32_MAX/hz */
	[66] = {100, {.tv_sec = 21474836, .tv_usec = 439999}, 2147483645,
		NULL},
	[67] = {100, {.tv_sec = 21474836, .tv_usec = 440000}, 2147483645,
		NULL},
	[68] = {100, {.tv_sec = 21474836, .tv_usec = 440001}, 2147483646,
		NULL},
	[69] = {100, {.tv_sec = 21474836, .tv_usec = 440002}, 2147483646,
		NULL},
	[70] = {100, {.tv_sec = 21474836, .tv_usec = 449999}, 2147483646,
		NULL},
	[71] = {100, {.tv_sec = 21474836, .tv_usec = 450000}, 2147483646,
		NULL},
	/* saturate at INT_MAX = 2^31 - 1 ticks */
	[72] = {100, {.tv_sec = 21474836, .tv_usec = 450001}, 2147483647,
		NULL},
	[73] = {100, {.tv_sec = 21474836, .tv_usec = 450002}, 2147483647,
		NULL},
	[74] = {100, {.tv_sec = 21474836, .tv_usec = 459999}, 2147483647,
		NULL},
	[75] = {100, {.tv_sec = 21474836, .tv_usec = 460000}, 2147483647,
		NULL},
	[76] = {100, {.tv_sec = 21474836, .tv_usec = 460001}, 2147483647,
		NULL},
	[77] = {100, {.tv_sec = 21474836, .tv_usec = 460002}, 2147483647,
		NULL},
	[78] = {100, {.tv_sec = 21474836, .tv_usec = 999999}, 2147483647,
		NULL},
	[79] = {100, {.tv_sec = 21474837, .tv_usec = 0}, 2147483647,
		NULL},
	[80] = {100, {.tv_sec = 21474837, .tv_usec = 1}, 2147483647,
		NULL},
	[81] = {100, {.tv_sec = 21474837, .tv_usec = 2}, 2147483647,
		NULL},
	[82] = {100, {.tv_sec = 21474837, .tv_usec = 2}, 2147483647,
		NULL},
	/* .tv_sec ~ INT64_MAX/1000000 */
	[83] = {100, {.tv_sec = 9223372036854, .tv_usec = 999999},
		INT_MAX, NULL},
	[84] = {100, {.tv_sec = 9223372036855, .tv_usec = 0},
		INT_MAX, NULL},
	[85] = {100, {.tv_sec = 9223372036855, .tv_usec = 1},
		INT_MAX, NULL},
	[86] = {100, {.tv_sec = 9223372036855, .tv_usec = 2},
		INT_MAX, NULL},
	/* .tv_sec ~ INT64_MAX/hz, overflows to INT_MAX ticks */
	[87] = {100, {.tv_sec = 92233720368547758, .tv_usec = 999999},
		INT_MAX, NULL},
	[88] = {100, {.tv_sec = 92233720368547758, .tv_usec = 0},
		INT_MAX, NULL},
	[89] = {100, {.tv_sec = 92233720368547758, .tv_usec = 1},
		INT_MAX, NULL},
	[90] = {100, {.tv_sec = 92233720368547758, .tv_usec = 2},
		INT_MAX, NULL},
	[91] = {100, {.tv_sec = (time_t)INT_MAX + 1, .tv_usec = 123456},
		INT_MAX, NULL},

	/*
	 * hz = 1000
	 */

	[92] = {1000, {.tv_sec = -1, .tv_usec = 0}, 0, NULL},
	[93] = {1000, {.tv_sec = -1, .tv_usec = 999999}, 0, NULL},
	[94] = {1000, {.tv_sec = 0, .tv_usec = 0}, 0, NULL},
	[95] = {1000, {.tv_sec = 0, .tv_usec = 1}, 2, NULL},
	[96] = {1000, {.tv_sec = 0, .tv_usec = 2}, 2, NULL},
	[97] = {1000, {.tv_sec = 0, .tv_usec = 999}, 2, NULL},
	[98] = {1000, {.tv_sec = 0, .tv_usec = 1000}, 2, NULL},
	[99] = {1000, {.tv_sec = 0, .tv_usec = 1001}, 3, NULL},
	[100] = {1000, {.tv_sec = 0, .tv_usec = 1002}, 3, NULL},
	[101] = {1000, {.tv_sec = 0, .tv_usec = 1999}, 3, NULL},
	[102] = {1000, {.tv_sec = 0, .tv_usec = 2000}, 3, NULL},
	[103] = {1000, {.tv_sec = 0, .tv_usec = 2001}, 4, NULL},
	[104] = {1000, {.tv_sec = 0, .tv_usec = 2002}, 4, NULL},
	[105] = {1000, {.tv_sec = 0, .tv_usec = 999999}, 1001, NULL},
	[106] = {1000, {.tv_sec = 1, .tv_usec = 0}, 1001, NULL},
	[107] = {1000, {.tv_sec = 1, .tv_usec = 1}, 1002, NULL},
	[108] = {1000, {.tv_sec = 1, .tv_usec = 2}, 1002, NULL},
	/* .tv_sec ~ INT_MAX/1000000 */
	[109] = {1000, {.tv_sec = 2147, .tv_usec = 999999}, 2148001, NULL},
	[110] = {1000, {.tv_sec = 2148, .tv_usec = 0}, 2148001, NULL},
	[111] = {1000, {.tv_sec = 2148, .tv_usec = 1}, 2148002, NULL},
	[112] = {1000, {.tv_sec = 2148, .tv_usec = 2}, 2148002, NULL},
	/* .tv_sec ~ INT_MAX/hz */
	[113] = {1000, {.tv_sec = 2147483, .tv_usec = 643999}, 2147483645,
		NULL},
	[114] = {1000, {.tv_sec = 2147483, .tv_usec = 644000}, 2147483645,
		NULL},
	[115] = {1000, {.tv_sec = 2147483, .tv_usec = 644001}, 2147483646,
		NULL},
	[116] = {1000, {.tv_sec = 2147483, .tv_usec = 644002}, 2147483646,
		NULL},
	[117] = {1000, {.tv_sec = 2147483, .tv_usec = 644999}, 2147483646,
		NULL},
	[118] = {1000, {.tv_sec = 2147483, .tv_usec = 645000}, 2147483646,
		NULL},
	/* saturate at INT_MAX = 2^31 - 1 ticks */
	[119] = {1000, {.tv_sec = 2147483, .tv_usec = 645001}, 2147483647,
		NULL},
	[120] = {1000, {.tv_sec = 2147483, .tv_usec = 645002}, 2147483647,
		NULL},
	[121] = {1000, {.tv_sec = 2147483, .tv_usec = 645999}, 2147483647,
		NULL},
	[122] = {1000, {.tv_sec = 2147483, .tv_usec = 646000}, 2147483647,
		NULL},
	[123] = {1000, {.tv_sec = 2147483, .tv_usec = 646001}, 2147483647,
		NULL},
	[124] = {1000, {.tv_sec = 2147483, .tv_usec = 646002}, 2147483647,
		NULL},
	[125] = {1000, {.tv_sec = 2147483, .tv_usec = 699999}, 2147483647,
		NULL},
	[126] = {1000, {.tv_sec = 2147484, .tv_usec = 0}, 2147483647,
		NULL},
	[127] = {1000, {.tv_sec = 2147484, .tv_usec = 1}, 2147483647,
		NULL},
	[128] = {1000, {.tv_sec = 2147484, .tv_usec = 2}, 2147483647,
		NULL},
	[129] = {1000, {.tv_sec = 2147484, .tv_usec = 2}, 2147483647,
		NULL},
	/* .tv_sec ~ INT64_MAX/1000000, overflows to INT_MAX ticks */
	[130] = {1000, {.tv_sec = 9223372036854, .tv_usec = 999999},
		INT_MAX, NULL},
	[131] = {1000, {.tv_sec = 9223372036855, .tv_usec = 0},
		INT_MAX, NULL},
	[132] = {1000, {.tv_sec = 9223372036855, .tv_usec = 1},
		INT_MAX, NULL},
	[133] = {1000, {.tv_sec = 9223372036855, .tv_usec = 2},
		INT_MAX, NULL},
	/* .tv_sec ~ INT64_MAX/hz, overflows to INT_MAX ticks */
	[134] = {1000, {.tv_sec = 92233720368547758, .tv_usec = 999999},
		INT_MAX, NULL},
	[135] = {1000, {.tv_sec = 92233720368547758, .tv_usec = 0},
		INT_MAX, NULL},
	[136] = {1000, {.tv_sec = 92233720368547758, .tv_usec = 1},
		INT_MAX, NULL},
	[137] = {1000, {.tv_sec = 92233720368547758, .tv_usec = 2},
		INT_MAX, NULL},
	[138] = {1000, {.tv_sec = (time_t)INT_MAX + 1, .tv_usec = 123456},
		INT_MAX, NULL},

	/*
	 * hz = 8191, prime non-divisor of 10^k or 2^k
	 */

	[139] = {8191, {.tv_sec = -1, .tv_usec = 0}, 0, NULL},
	[140] = {8191, {.tv_sec = -1, .tv_usec = 999999}, 0, NULL},
	[141] = {8191, {.tv_sec = 0, .tv_usec = 0}, 0, NULL},
	[142] = {8191, {.tv_sec = 0, .tv_usec = 1}, 2, NULL},
	[143] = {8191, {.tv_sec = 0, .tv_usec = 2}, 2, NULL},
	[144] = {8191, {.tv_sec = 0, .tv_usec = 121}, 2, NULL},
	[145] = {8191, {.tv_sec = 0, .tv_usec = 122}, 2, NULL},
	[146] = {8191, {.tv_sec = 0, .tv_usec = 123}, 3, NULL},
	[147] = {8191, {.tv_sec = 0, .tv_usec = 242}, 3, NULL},
	[148] = {8191, {.tv_sec = 0, .tv_usec = 243}, 3, NULL},
	[149] = {8191, {.tv_sec = 0, .tv_usec = 244}, 3, NULL},
	[150] = {8191, {.tv_sec = 0, .tv_usec = 245}, 4, NULL},
	[151] = {8191, {.tv_sec = 0, .tv_usec = 246}, 4, NULL},
	[152] = {8191, {.tv_sec = 0, .tv_usec = 999999}, 8192, NULL},
	[153] = {8191, {.tv_sec = 1, .tv_usec = 0}, 8192, NULL},
	[154] = {8191, {.tv_sec = 1, .tv_usec = 1}, 8193, NULL},
	[155] = {8191, {.tv_sec = 1, .tv_usec = 2}, 8193, NULL},
	/* .tv_sec ~ INT_MAX/1000000 */
	[156] = {8191, {.tv_sec = 2147, .tv_usec = 999999}, 17594269, NULL},
	[157] = {8191, {.tv_sec = 2148, .tv_usec = 0}, 17594269, NULL},
	[158] = {8191, {.tv_sec = 2148, .tv_usec = 1}, 17594270, NULL},
	[159] = {8191, {.tv_sec = 2148, .tv_usec = 2}, 17594270, NULL},
	/* .tv_sec ~ INT_MAX/hz */
	[160] = {8191, {.tv_sec = 262176, .tv_usec = 3540}, 2147483646,
		NULL},
	[161] = {8191, {.tv_sec = 262176, .tv_usec = 3541}, 2147483647,
		NULL},
	[162] = {8191, {.tv_sec = 262176, .tv_usec = 3542}, 2147483647,
		NULL},
	/* saturate at INT_MAX = 2^31 - 1 ticks */
	[163] = {8191, {.tv_sec = 262176, .tv_usec = 3662}, 2147483647,
		NULL},
	[164] = {8191, {.tv_sec = 262176, .tv_usec = 3663}, 2147483647,
		NULL},
	[165] = {8191, {.tv_sec = 262176, .tv_usec = 3664}, 2147483647,
		NULL},
	[166] = {8191, {.tv_sec = 262176, .tv_usec = 999999}, 2147483647,
		NULL},
	/* .tv_sec ~ INT64_MAX/1000000, overflows to INT_MAX ticks */
	[167] = {8191, {.tv_sec = 9223372036854, .tv_usec = 999999},
		INT_MAX, NULL},
	[168] = {8191, {.tv_sec = 9223372036855, .tv_usec = 0},
		INT_MAX, NULL},
	[169] = {8191, {.tv_sec = 9223372036855, .tv_usec = 1},
		INT_MAX, NULL},
	[170] = {8191, {.tv_sec = 9223372036855, .tv_usec = 2},
		INT_MAX, NULL},
	/* .tv_sec ~ INT64_MAX/hz, overflows to INT_MAX ticks */
	[171] = {8191, {.tv_sec = 92233720368547758, .tv_usec = 999999},
		INT_MAX, NULL},
	[172] = {8191, {.tv_sec = 92233720368547758, .tv_usec = 0},
		INT_MAX, NULL},
	[173] = {8191, {.tv_sec = 92233720368547758, .tv_usec = 1},
		INT_MAX, NULL},
	[174] = {8191, {.tv_sec = 92233720368547758, .tv_usec = 2},
		INT_MAX, NULL},
	[175] = {8191, {.tv_sec = (time_t)INT_MAX + 1, .tv_usec = 123456},
		INT_MAX, NULL},
};

ATF_TC(tvtohz);
ATF_TC_HEAD(tvtohz, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test tvtohz(9)");
}
ATF_TC_BODY(tvtohz, tc)
{
	size_t i;

	for (i = 0; i < __arraycount(tvtohz_cases); i++) {
		const struct tvtohz_case *tv = &tvtohz_cases[i];
		int ticks;

		/* set system parameters */
		hz = tv->tv_hz;
		tick = 1000000/hz;

		ticks = tvtohz(&tv->tv_tv);
		if (tv->tv_xfail)
			atf_tc_expect_fail("%s", tv->tv_xfail);

		/*
		 * Allow some slop of one part per thousand in the
		 * arithmetic, but ensure we round up, not down.
		 *
		 * XXX Analytically determine error bounds on the
		 * formulae we use and assess them.
		 */
		ATF_CHECK_MSG(((unsigned)(ticks - tv->tv_ticks) <=
			(unsigned)tv->tv_ticks/1000),
		    "[%zu] tvtohz(%lld.%06ld sec) @ %d Hz:"
		    " expected %d, got %d",
		    i,
		    (long long)tv->tv_tv.tv_sec,
		    (long)tv->tv_tv.tv_usec,
		    tv->tv_hz,
		    tv->tv_ticks,
		    ticks);
		if (tv->tv_xfail)
			atf_tc_expect_pass();
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, itimer_transitions);
	ATF_TP_ADD_TC(tp, tvtohz);

	return atf_no_error();
}

