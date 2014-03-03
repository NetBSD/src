/* $NetBSD: t_acos.c,v 1.8 2014/03/03 18:21:33 dsl Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
#include <math.h>

/*
 * Check result of fn(arg) is correct within the bounds.
 * Should be ok to do the checks using 'double' for 'float' functions.
 */
#define T_LIBM_CHECK(subtest, fn, arg, expect, epsilon) do { \
	double r = fn(arg); \
	double e = fabs(r - expect); \
	if (e > epsilon) \
		atf_tc_fail_nonfatal( \
		    "subtest %zu: " #fn "(%g) is %g not %g (error %g > %g)", \
		    subtest, arg, r, expect, e, epsilon); \
    } while (0)

/* Check that the result of fn(arg) is NaN */
#ifndef __vax__
#define T_LIBM_CHECK_NAN(subtest, fn, arg) do { \
	double r = fn(arg); \
	if (!isnan(r)) \
		atf_tc_fail_nonfatal("subtest %zu: " #fn "(%g) is %g not NaN", \
		    subtest, arg, r); \
    } while (0)
#else
/* vax doesn't support NaN */
#define T_LIBM_CHECK_NAN(subtest, fn, arg) (void)(arg)
#endif

#define AFT_LIBM_TEST(name, description) \
ATF_TC(name); \
ATF_TC_HEAD(name, tc) { atf_tc_set_md_var(tc, "descr", description); } \
ATF_TC_BODY(name, tc)

/*
 * acos(3) and acosf(3)
 */

AFT_LIBM_TEST(acos_nan, "Test acos/acosf(x) == NaN, x = NaN, +/-Inf, ![-1..1]")
{
	static const double x[] = {
	    -1.000000001, 1.000000001,
	    -1.0000001, 1.0000001,
	    -1.1, 1.1,
#ifndef __vax__
	    0.0L / 0.0L,  /* NAN */
	    -1.0L / 0.0L, /* -Inf */
	    +1.0L / 0.0L, /* +Inf */
#endif
	};
	size_t i;

	for (i = 0; i < __arraycount(x); i++) {
		T_LIBM_CHECK_NAN(i, acos, x[i]);
		if (i < 2)
			/* Values are too small for float */
			continue;
		T_LIBM_CHECK_NAN(i, acosf, x[i]);
	}
}

AFT_LIBM_TEST(acos_inrange, "Test acos/acosf(x) for some valid values")
{
	static const struct {
		double x;
		double y;
	} values[] = {
		{ -1,    M_PI,              },
		{ -0.99, 3.000053180265366, },
		{ -0.5,  2.094395102393195, },
		{ -0.1,  1.670963747956456, },
		{  0,    M_PI / 2,          },
		{  0.1,  1.470628905633337, },
		{  0.5,  1.047197551196598, },
		{  0.99, 0.141539473324427, },
	};
	size_t i;

	/*
	 * Note that acos(x) might be calculated as atan2(sqrt(1-x*x),x).
	 * This means that acos(-1) is atan2(+0,-1), if the sign is wrong
	 * the value will be -M_PI (atan2(-0,-1)) not M_PI.
	 */

	for (i = 0; i < __arraycount(values); i++) {
		T_LIBM_CHECK(i, acos, values[i].x, values[i].y, 1.0e-15);
		T_LIBM_CHECK(i, acosf, values[i].x, values[i].y, 1.0e-5);
	}
}

AFT_LIBM_TEST(acos_one_pos, "Test acos(1.0) == +0.0")
{
	const double y = acos(1.0);

	if (fabs(y) > 0.0 || signbit(y) != 0)
		atf_tc_fail_nonfatal("acos(1.0) != +0.0");
}

AFT_LIBM_TEST(acosf_one_pos, "Test acosf(1.0) == +0.0")
{
	const float y = acosf(1.0);

	if (fabsf(y) > 0.0 || signbit(y) != 0)
		atf_tc_fail_nonfatal("acosf(1.0) != +0.0");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, acos_nan);
	ATF_TP_ADD_TC(tp, acos_inrange);
	ATF_TP_ADD_TC(tp, acos_one_pos);
	ATF_TP_ADD_TC(tp, acosf_one_pos);

	return atf_no_error();
}
