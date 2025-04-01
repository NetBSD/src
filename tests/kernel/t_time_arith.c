/*	$NetBSD: t_time_arith.c,v 1.3 2025/04/01 23:14:23 riastradh Exp $	*/

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
__RCSID("$NetBSD: t_time_arith.c,v 1.3 2025/04/01 23:14:23 riastradh Exp $");

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

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, itimer_transitions);

	return atf_no_error();
}

