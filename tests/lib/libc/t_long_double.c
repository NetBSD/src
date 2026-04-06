/*	$NetBSD: t_long_double.c,v 1.5 2026/04/06 22:55:51 nat Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig.
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
__RCSID("$NetBSD: t_long_double.c,v 1.5 2026/04/06 22:55:51 nat Exp $");

#include <atf-c.h>

#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>

static void
test_ldbl_cmp(void)
{
	static const long double ns[] = {
		-0x1.0p63L,
		-0x1.0p62L,
		-0x1.0p60L,
		-0x1.0p10L,
		-0x1.0p1L,
		-0x1.0p0L,
		0.0L,
		+0x1.0p0L,
		+0x1.0p1L,
		+0x1.0p10L,
		+0x1.0p60L,
		+0x1.0p62L,
		+0x1.0p63L,
	};

	for (size_t i = 0; i < __arraycount(ns); i++) {
		for (size_t j = 0; j < __arraycount(ns); j++) {
			if (i < j)
				ATF_CHECK_MSG(ns[i] < ns[j],
				    "%s: want %Lf (%La) < %Lf (%La)",
				    __func__, ns[i], ns[i], ns[j], ns[j]);
			if (i <= j)
				ATF_CHECK_MSG(ns[i] <= ns[j],
				    "%s: want %Lf (%La) <= %Lf (%La)",
				   __func__,  ns[i], ns[i], ns[j], ns[j]);
			if (i == j)
				ATF_CHECK_MSG(ns[i] == ns[j],
				    "%s: want %Lf (%La) == %Lf (%La)",
				    __func__, ns[i], ns[i], ns[j], ns[j]);
			if (i != j)
				ATF_CHECK_MSG(ns[i] != ns[j],
				    "%s: want %Lf (%La) != %Lf (%La)",
				    __func__, ns[i], ns[i], ns[j], ns[j]);
			if (i >= j)
				ATF_CHECK_MSG(ns[i] >= ns[j],
				    "%s: want %Lf (%La) >= %Lf (%La)",
				    __func__, ns[i], ns[i], ns[j], ns[j]);
			if (i > j)
				ATF_CHECK_MSG(ns[i] > ns[j],
				    "%s: want %Lf (%La) > %Lf (%La)",
				    __func__, ns[i], ns[i], ns[j], ns[j]);
		}
	}
}

static void
test_int64_to_ldbl(void)
{
	static const struct testcase {
		int64_t arg;
		long double want;
	} testcases[] = {
		{ INT64_MIN, -0x1.0p63L },
		{ -(1LL << 62), -0x1.0p62L },
		{ -(1LL << 60), -0x1.0p60L },
		{ -(1LL << 40), -0x1.0p40L },
		{ -(1LL << 20), -0x1.0p20L },
		{ -(1LL << 10), -0x1.0p10L },
		{ -1, -1.0L },
		{ 0, +0.0L },
		{ +1, +1.0L },
		{ +(1LL << 10), +0x1.0p10L },
		{ +(1LL << 20), +0x1.0p20L },
		{ +(1LL << 40), +0x1.0p40L },
		{ +(1LL << 60), +0x1.0p60L },
		{ +(1LL << 62), +0x1.0p62L },
	};

	for (size_t i = 0; i < __arraycount(testcases); i++) {
		const struct testcase *c = testcases + i;
		long double have = (long double)c->arg;
		ATF_CHECK_MSG(have == c->want,
		    "%s: want %Lf (%La) for %" PRId64 " (0x%016" PRIx64 "), "
		    "have %Lf (%La)",
		    __func__, c->want, c->want, c->arg, c->arg, have, have);
	}
}

static void
test_ldbl_to_int64(void)
{
	static const struct testcase {
		long double arg;
		int64_t want;
	} testcases[] = {
		{ -0x1.0p63L, INT64_MIN },
		{ -1.0L, -1 },
		{ -0.0L, 0 },
		{ +0.0L, 0 },
		{ +1.0L, 1 },
		{ +0x1.0p62L, +1LL << 62 },
	};

	for (size_t i = 0; i < __arraycount(testcases); i++) {
		const struct testcase *c = testcases + i;
		int64_t have = (int64_t)c->arg;
		ATF_CHECK_MSG(have == c->want,
		    "%s: want %" PRId64 " (0x%016" PRIx64 ") for %Lf (%La), "
		    "have %" PRId64 " (0x%016" PRIx64 ")",
		    __func__, c->want, c->want, c->arg, c->arg, have, have);
	}
}

static void
test_uint64_to_ldbl(void)
{
	static const struct testcase {
		uint64_t arg;
		long double want;
	} testcases[] = {
		{ 0, +0.0L },
		{ +1, +1.0L },
		{ +1ULL << 63, +0x1.0p63L },
	};

	for (size_t i = 0; i < __arraycount(testcases); i++) {
		const struct testcase *c = testcases + i;
		long double have = (long double)c->arg;
		ATF_CHECK_MSG(have == c->want,
		    "%s: want %Lf (%La) for %" PRIu64 " (0x%016" PRIx64 "), "
		    "have %Lf (%La)",
		    __func__, c->want, c->want, c->arg, c->arg, have, have);
	}
}

static void
test_ldbl_to_uint64(void)
{
	static const struct testcase {
		long double arg;
		uint64_t want;
	} testcases[] = {
#if 1 /* Try some values outside the portable range. */
		// C23 6.3.1.4p1 says the portable range goes from
		// -1 + epsilon to (1<<64) - epsilon.
#if __m68k__
		// Negative values below 0 saturate.
		{ -0xf.0p60L, 0 },
		{ -0x8.0p60L, 0 },
		{ -0x4.0p60L, 0 },
		{ -1.0L, 0 },
#else
		// Negative values below INT64_MIN saturate.
		{ -0xf.0p60L, 0x8ULL << 60 },
		{ -0x8.0p60L, 0x8ULL << 60 },
		{ -0x4.0p60L, 0xcULL << 60 },
		// The above results were taken on amd64,
		// other platforms may differ.
		{ -1.0L, (uint64_t)-1 },
#endif
#endif
		{ -0.0L, 0 },
		{ +0.0L, 0 },
		{ +1.0L, 1 },
		{ +0x1.0p63L, 1ULL << 63 },
	};

	for (size_t i = 0; i < __arraycount(testcases); i++) {
		const struct testcase *c = testcases + i;
		uint64_t have = (uint64_t)c->arg;
		ATF_CHECK_MSG(have == c->want,
		    "%s: want %" PRIu64 " (0x%016" PRIx64 ") for %Lf (%La), "
		    "have %" PRIu64 " (0x%016" PRIx64 ")",
		    __func__, c->want, c->want, c->arg, c->arg, have, have);
	}
}

static void
test_dbl_to_ldbl(void)
{
	static const struct testcase {
		double arg;
		long double want;
	} testcases[] = {
		{ -DBL_MAX, -DBL_MAX },
		{ -1.0, -1.0L },
		{ -DBL_MIN, -DBL_MIN },
		{ -0.0, -0.0L },
		{ +0.0, +0.0L },
		{ +DBL_MIN, +DBL_MIN },
		{ +1.0, +1.0L },
		{ +DBL_MAX, +DBL_MAX },
	};

	for (size_t i = 0; i < __arraycount(testcases); i++) {
		const struct testcase *c = testcases + i;
		long double have = (long double)c->arg;
		ATF_CHECK_MSG(have == c->want,
		    "%s: want %Lf (%La) for %f (%a), have %Lf (%La)",
		    __func__, c->want, c->want, c->arg, c->arg, have, have);
	}
}

static void
test_ldbl_to_dbl(void)
{
	static const struct testcase {
		long double arg;
		double want;
	} testcases[] = {
		{ -DBL_MAX, -DBL_MAX },
		{ -1.0L, -1.0 },
		{ -DBL_MIN, -DBL_MIN },
		{ -0.0L, -0.0 },
		{ +0.0L, +0.0 },
		{ +DBL_MIN, +DBL_MIN },
		{ +1.0L, +1.0 },
		{ +DBL_MAX, +DBL_MAX },
	};

	for (size_t i = 0; i < __arraycount(testcases); i++) {
		const struct testcase *c = testcases + i;
		double have = (double)c->arg;
		ATF_CHECK_MSG(have == c->want,
		    "%s: want %f (%a) for %Lf (%La), have %f (%a)",
		    __func__, c->want, c->want, c->arg, c->arg, have, have);
	}
}

ATF_TC(long_double);

ATF_TC_HEAD(long_double, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test 'long double' operations");
}

ATF_TC_BODY(long_double, tc)
{
	test_ldbl_cmp();
	test_int64_to_ldbl();
	test_ldbl_to_int64();
	test_uint64_to_ldbl();
	test_ldbl_to_uint64();
	test_dbl_to_ldbl();
	test_ldbl_to_dbl();
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, long_double);

	return atf_no_error();
}
