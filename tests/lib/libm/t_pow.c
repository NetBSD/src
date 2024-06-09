/* $NetBSD: t_pow.c,v 1.6 2024/06/09 16:53:25 riastradh Exp $ */

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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_pow.c,v 1.6 2024/06/09 16:53:25 riastradh Exp $");

#include <atf-c.h>
#include <math.h>

/*
 * pow(3)
 */
ATF_TC(pow_nan_x);
ATF_TC_HEAD(pow_nan_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(NaN, y) == NaN");
}

ATF_TC_BODY(pow_nan_x, tc)
{
	const volatile double x = 0.0 / 0.0;
	const double z = pow(x, 2.0);

	ATF_CHECK_MSG(isnan(z), "z=%a", z);
}

ATF_TC(pow_nan_y);
ATF_TC_HEAD(pow_nan_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(x, NaN) == NaN");
}

ATF_TC_BODY(pow_nan_y, tc)
{
	const volatile double y = 0.0 / 0.0;
	const double z = pow(2.0, y);

	ATF_CHECK_MSG(isnan(z), "z=%a", z);
}

ATF_TC(pow_inf_neg_x);
ATF_TC_HEAD(pow_inf_neg_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(-Inf, y) == +-Inf || +-0.0");
}

ATF_TC_BODY(pow_inf_neg_x, tc)
{
	const volatile double x = -1.0 / 0.0;
	double z;

	/*
	 * If y is odd, y > 0, and x is -Inf, -Inf is returned.
	 * If y is even, y > 0, and x is -Inf, +Inf is returned.
	 */
	z = pow(x, 3.0);
	ATF_CHECK_MSG(isinf(z), "x=%a z=%s=%a", x, "pow(x, 3.0)", z);
	ATF_CHECK_MSG(signbit(z) != 0, "x=%a z=%s=%a", x, "pow(x, 3.0)", z);

	z = pow(x, 4.0);
	ATF_CHECK_MSG(isinf(z), "x=%a z=%s=%a", x, "pow(x, 4.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "pow(x, 4.0)", z);

	/*
	 * If y is odd, y < 0, and x is -Inf, -0.0 is returned.
	 * If y is even, y < 0, and x is -Inf, +0.0 is returned.
	 */
	z = pow(x, -3.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "x=%a z=%s=%a", x, "pow(x, -3.0)", z);
	ATF_CHECK_MSG(signbit(z) != 0, "x=%a z=%s=%a", x, "pow(x, -3.0)", z);

	z = pow(x, -4.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "x=%a z=%s=%a", x, "pow(x, -4.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "pow(x, -4.0)", z);
}

ATF_TC(pow_inf_neg_y);
ATF_TC_HEAD(pow_inf_neg_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(x, -Inf) == +Inf || +0.0");
}

ATF_TC_BODY(pow_inf_neg_y, tc)
{
	const volatile double y = -1.0 / 0.0;
	double z;

	/*
	 * If |x| < 1 and y is -Inf, +Inf is returned.
	 * If |x| > 1 and y is -Inf, +0.0 is returned.
	 */
	z = pow(0.1, y);
	ATF_CHECK_MSG(isinf(z), "y=%a z=%s=%a", y, "pow(0.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "pow(0.1, y)", z);

	z = pow(1.1, y);
	ATF_CHECK_MSG(fabs(z) == 0.0, "y=%a z=%s=%a", y, "pow(1.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "pow(1.1, y)", z);
}

ATF_TC(pow_inf_pos_x);
ATF_TC_HEAD(pow_inf_pos_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(+Inf, y) == +Inf || +0.0");
}

ATF_TC_BODY(pow_inf_pos_x, tc)
{
	const volatile double x = 1.0 / 0.0;
	double z;

	/*
	 * For y < 0, if x is +Inf, +0.0 is returned.
	 * For y > 0, if x is +Inf, +Inf is returned.
	 */
	z = pow(x, -2.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "x=%a z=%s=%a", x, "pow(x, -2.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "pow(x, -2.0)", z);

	z = pow(x, 2.0);
	ATF_CHECK_MSG(isinf(z), "x=%a z=%s=%a", x, "pow(x, 2.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "pow(x, 2.0)", z);
}

ATF_TC(pow_inf_pos_y);
ATF_TC_HEAD(pow_inf_pos_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(x, +Inf) == +Inf || +0.0");
}

ATF_TC_BODY(pow_inf_pos_y, tc)
{
	const volatile double y = 1.0 / 0.0;
	double z;

	/*
	 * If |x| < 1 and y is +Inf, +0.0 is returned.
	 * If |x| > 1 and y is +Inf, +Inf is returned.
	 */
	z = pow(0.1, y);
	ATF_CHECK_MSG(fabs(z) == 0, "y=%a z=%s=%a", y, "pow(0.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "pow(0.1, y)", z);

	z = pow(1.1, y);
	ATF_CHECK_MSG(isinf(z), "y=%a z=%s=%a", y, "pow(1.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "pow(1.1, y)", z);
}

ATF_TC(pow_one_neg_x);
ATF_TC_HEAD(pow_one_neg_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(-1.0, +-Inf) == 1.0");
}

ATF_TC_BODY(pow_one_neg_x, tc)
{
	const volatile double infp = 1.0 / 0.0;
	const volatile double infn = -1.0 / 0.0;
	double z;

	/*
	 * If x is -1.0, and y is +-Inf, 1.0 shall be returned.
	 */
	ATF_REQUIRE_MSG(isinf(infp), "infp=%a", infp);
	ATF_REQUIRE_MSG(isinf(infn), "infn=%a", infn);

	ATF_CHECK_EQ_MSG((z = pow(-1.0, infp)), 1.0, "z=%a", z);
	ATF_CHECK_EQ_MSG((z = pow(-1.0, infn)), 1.0, "z=%a", z);
}

ATF_TC(pow_one_pos_x);
ATF_TC_HEAD(pow_one_pos_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(1.0, y) == 1.0");
}

ATF_TC_BODY(pow_one_pos_x, tc)
{
	const volatile double y[] =
	    { 0.0, 0.1, 2.0, -3.0, 99.0, 99.99, 9999999.9 };
	const volatile double z = 0.0 / 0.0;
	size_t i;

	/*
	 * For any value of y (including NaN),
	 * if x is 1.0, 1.0 shall be returned.
	 */
	ATF_CHECK_EQ_MSG(pow(1.0, z), 1.0, "z=%a pow(1.0, z)=%a",
	    z, pow(1.0, z));

	for (i = 0; i < __arraycount(y); i++) {
		ATF_CHECK_EQ_MSG(pow(1.0, y[i]), 1.0,
		    "i=%zu y[i]=%a pow(1.0, y[i])=%a",
		    i, y[i], pow(1.0, y[i]));
	}
}

ATF_TC(pow_zero_x);
ATF_TC_HEAD(pow_zero_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(+-0.0, y) == +-0.0 || HUGE");
}

ATF_TC_BODY(pow_zero_x, tc)
{
	double z;

	/*
	 * If x is +0.0 or -0.0, y > 0, and y
	 * is an odd integer, x is returned.
	 */
	z = pow(+0.0, 3.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) == 0, "z=%a", z);

	z = pow(-0.0, 3.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) != 0, "z=%a", z);

	/*
	 * If y > 0 and not an odd integer,
	 * if x is +0.0 or -0.0, +0.0 is returned.
	 */
	z = pow(+0.0, 4.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) == 0, "z=%a", z);

	z = pow(-0.0, 4.0);
	ATF_CHECK_MSG(fabs(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) == 0, "z=%a", z);

	/*
	 * If y < 0 and x is +0.0 or -0.0, either +-HUGE_VAL,
	 * +-HUGE_VALF, or +-HUGE_VALL shall be returned.
	 */
	z = pow(+0.0, -4.0);
	ATF_CHECK_EQ_MSG(z, HUGE_VAL, "z=%a", z);

	z = pow(-0.0, -4.0);
	ATF_CHECK_EQ_MSG(z, HUGE_VAL, "z=%a", z);

	z = pow(+0.0, -5.0);
	ATF_CHECK_EQ_MSG(z, HUGE_VAL, "z=%a", z);

	z = pow(-0.0, -5.0);
	ATF_CHECK_EQ_MSG(z, -HUGE_VAL, "z=%a", z);
}

ATF_TC(pow_zero_y);
ATF_TC_HEAD(pow_zero_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pow(x, +-0.0) == 1.0");
}

ATF_TC_BODY(pow_zero_y, tc)
{
	const volatile double x[] =  { 0.1, -3.0, 77.0, 99.99, 101.0000001 };
	const volatile double z = 0.0 / 0.0;
	size_t i;

	/*
	 * For any value of x (including NaN),
	 * if y is +0.0 or -0.0, 1.0 is returned.
	 */
	ATF_CHECK_EQ_MSG(pow(z, +0.0), 1.0, "z=%a pow(z, +0.0)=%a",
	    z, pow(z, +0.0));
	ATF_CHECK_EQ_MSG(pow(z, -0.0), 1.0, "z=%a pow(z, -0.0)=%a",
	    z, pow(z, -0.0));

	for (i = 0; i < __arraycount(x); i++) {
		ATF_CHECK_EQ_MSG(pow(x[i], +0.0), 1.0,
		    "i=%zu pow(%a, +0.0)=%a", i, x[i], pow(x[i], +0.0));
		ATF_CHECK_EQ_MSG(pow(x[i], -0.0), 1.0,
		    "i=%zu pow(%a, -0.0)=%a", i, x[i], pow(x[i], -0.0));
	}
}

/*
 * powf(3)
 */
ATF_TC(powf_nan_x);
ATF_TC_HEAD(powf_nan_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(NaN, y) == NaN");
}

ATF_TC_BODY(powf_nan_x, tc)
{
	const volatile float x = 0.0f / 0.0f;
	const float z = powf(x, 2.0f);

	ATF_CHECK_MSG(isnanf(z), "z=%a", z);
}

ATF_TC(powf_nan_y);
ATF_TC_HEAD(powf_nan_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(x, NaN) == NaN");
}

ATF_TC_BODY(powf_nan_y, tc)
{
	const volatile float y = 0.0f / 0.0f;
	const float z = powf(2.0f, y);

	ATF_CHECK_MSG(isnanf(z), "z=%a", z);
}

ATF_TC(powf_inf_neg_x);
ATF_TC_HEAD(powf_inf_neg_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(-Inf, y) == +-Inf || +-0.0");
}

ATF_TC_BODY(powf_inf_neg_x, tc)
{
	const volatile float x = -1.0f / 0.0f;
	float z;

	/*
	 * If y is odd, y > 0, and x is -Inf, -Inf is returned.
	 * If y is even, y > 0, and x is -Inf, +Inf is returned.
	 */
	z = powf(x, 3.0);
	ATF_CHECK_MSG(isinf(z), "x=%a z=%s=%a", x, "powf(x, 3.0)", z);
	ATF_CHECK_MSG(signbit(z) != 0, "x=%a z=%s=%a", x, "powf(x, 3.0)", z);

	z = powf(x, 4.0);
	ATF_CHECK_MSG(isinf(z), "x=%a z=%s=%a", x, "powf(x, 4.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "powf(x, 4.0)", z);

	/*
	 * If y is odd, y < 0, and x is -Inf, -0.0 is returned.
	 * If y is even, y < 0, and x is -Inf, +0.0 is returned.
	 */
	z = powf(x, -3.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "x=%a z=%s=%a", x, "powf(x, -3.0)", z);
	ATF_CHECK_MSG(signbit(z) != 0, "x=%a z=%s=%a", x, "powf(x, -3.0)", z);

	z = powf(x, -4.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "x=%a z=%s=%a", x, "powf(x, -4.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "powf(x, -4.0)", z);
}

ATF_TC(powf_inf_neg_y);
ATF_TC_HEAD(powf_inf_neg_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(x, -Inf) == +Inf || +0.0");
}

ATF_TC_BODY(powf_inf_neg_y, tc)
{
	const volatile float y = -1.0f / 0.0f;
	float z;

	/*
	 * If |x| < 1 and y is -Inf, +Inf is returned.
	 * If |x| > 1 and y is -Inf, +0.0 is returned.
	 */
	z = powf(0.1, y);
	ATF_CHECK_MSG(isinf(z), "y=%a z=%s=%a", y, "powf(0.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "powf(0.1, y)", z);

	z = powf(1.1, y);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "y=%a z=%s=%a", y, "powf(1.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "powf(1.1, y)", z);
}

ATF_TC(powf_inf_pos_x);
ATF_TC_HEAD(powf_inf_pos_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(+Inf, y) == +Inf || +0.0");
}

ATF_TC_BODY(powf_inf_pos_x, tc)
{
	const volatile float x = 1.0f / 0.0f;
	float z;

	/*
	 * For y < 0, if x is +Inf, +0.0 is returned.
	 * For y > 0, if x is +Inf, +Inf is returned.
	 */
	z = powf(x, -2.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "x=%a z=%s=%a", x, "powf(x, -2.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "powf(x, -2.0)", z);

	z = powf(x, 2.0);
	ATF_CHECK_MSG(isinf(z), "x=%a z=%s=%a", x, "powf(x, 2.0)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "x=%a z=%s=%a", x, "powf(x, 2.0)", z);
}

ATF_TC(powf_inf_pos_y);
ATF_TC_HEAD(powf_inf_pos_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(x, +Inf) == +Inf || +0.0");
}

ATF_TC_BODY(powf_inf_pos_y, tc)
{
	const float y = 1.0L / 0.0L;
	float z;

	/*
	 * If |x| < 1 and y is +Inf, +0.0 is returned.
	 * If |x| > 1 and y is +Inf, +Inf is returned.
	 */
	z = powf(0.1, y);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "y=%a z=%s=%a", y, "powf(0.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "powf(0.1, y)", z);

	z = powf(1.1, y);
	ATF_CHECK_MSG(isinf(z), "y=%a z=%s=%a", y, "powf(1.1, y)", z);
	ATF_CHECK_MSG(signbit(z) == 0, "y=%a z=%s=%a", y, "powf(1.1, y)", z);
}

ATF_TC(powf_one_neg_x);
ATF_TC_HEAD(powf_one_neg_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(-1.0, +-Inf) == 1.0");
}

ATF_TC_BODY(powf_one_neg_x, tc)
{
	const volatile float infp = 1.0f / 0.0f;
	const volatile float infn = -1.0f / 0.0f;
	double z;

	/*
	 * If x is -1.0, and y is +-Inf, 1.0 shall be returned.
	 */
	ATF_REQUIRE_MSG(isinf(infp), "infp=%a", infp);
	ATF_REQUIRE_MSG(isinf(infn), "infn=%a", infn);

	ATF_CHECK_EQ_MSG((z = powf(-1.0, infp)), 1.0, "z=%a", z);
	ATF_CHECK_EQ_MSG((z = powf(-1.0, infn)), 1.0, "z=%a", z);
}

ATF_TC(powf_one_pos_x);
ATF_TC_HEAD(powf_one_pos_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(1.0, y) == 1.0");
}

ATF_TC_BODY(powf_one_pos_x, tc)
{
	const volatile float y[] =
	    { 0.0, 0.1, 2.0, -3.0, 99.0, 99.99, 9999999.9 };
	const volatile float z = 0.0f / 0.0f;
	size_t i;

	/*
	 * For any value of y (including NaN),
	 * if x is 1.0, 1.0 shall be returned.
	 */
	ATF_CHECK_EQ_MSG(powf(1.0, z), 1.0, "z=%a powf(1.0, z)=%a",
	    z, powf(1.0, z));

	for (i = 0; i < __arraycount(y); i++) {
		ATF_CHECK_EQ_MSG(powf(1.0, y[i]), 1.0,
		    "i=%zu y[i]=%a powf(1.0, y[i])=%a",
		    i, y[i], powf(1.0, y[i]));
	}
}

ATF_TC(powf_zero_x);
ATF_TC_HEAD(powf_zero_x, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(+-0.0, y) == +-0.0 || HUGE");
}

ATF_TC_BODY(powf_zero_x, tc)
{
	float z;

	/*
	 * If x is +0.0 or -0.0, y > 0, and y
	 * is an odd integer, x is returned.
	 */
	z = powf(+0.0, 3.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) == 0, "z=%a", z);

	z = powf(-0.0, 3.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) != 0, "z=%a", z);

	/*
	 * If y > 0 and not an odd integer,
	 * if x is +0.0 or -0.0, +0.0 is returned.
	 */
	z = powf(+0.0, 4.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) == 0, "z=%a", z);

	z = powf(-0.0, 4.0);
	ATF_CHECK_MSG(fabsf(z) == 0.0, "z=%a", z);
	ATF_CHECK_MSG(signbit(z) == 0, "z=%a", z);

	/*
	 * If y < 0 and x is +0.0 or -0.0, either +-HUGE_VAL,
	 * +-HUGE_VALF, or +-HUGE_VALL shall be returned.
	 */
	z = powf(+0.0, -4.0);
	ATF_CHECK_EQ_MSG(z, HUGE_VALF, "z=%a", z);

	z = powf(-0.0, -4.0);
	ATF_CHECK_EQ_MSG(z, HUGE_VALF, "z=%a", z);

	z = powf(+0.0, -5.0);
	ATF_CHECK_EQ_MSG(z, HUGE_VALF, "z=%a", z);

	z = powf(-0.0, -5.0);
	ATF_CHECK_EQ_MSG(z, -HUGE_VALF, "z=%a", z);
}

ATF_TC(powf_zero_y);
ATF_TC_HEAD(powf_zero_y, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test powf(x, +-0.0) == 1.0");
}

ATF_TC_BODY(powf_zero_y, tc)
{
	const volatile float x[] =  { 0.1, -3.0, 77.0, 99.99, 101.0000001 };
	const volatile float z = 0.0f / 0.0f;
	size_t i;

	/*
	 * For any value of x (including NaN),
	 * if y is +0.0 or -0.0, 1.0 is returned.
	 */
	ATF_CHECK_EQ_MSG(powf(z, +0.0), 1.0, "z=%a powf(z, +0.0)=%a",
	    z, powf(z, +0.0));
	ATF_CHECK_EQ_MSG(powf(z, -0.0), 1.0, "z=%a powf(z, -0.0)=%a",
	    z, powf(z, -0.0));

	for (i = 0; i < __arraycount(x); i++) {
		ATF_CHECK_EQ_MSG(powf(x[i], +0.0), 1.0,
		    "i=%zu powf(%a, +0.0)=%a", i, x[i], powf(x[i], +0.0));
		ATF_CHECK_EQ_MSG(powf(x[i], -0.0), 1.0,
		    "i=%zu powf(%a, -0.0)=%a", i, x[i], powf(x[i], -0.0));
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, pow_nan_x);
	ATF_TP_ADD_TC(tp, pow_nan_y);
	ATF_TP_ADD_TC(tp, pow_inf_neg_x);
	ATF_TP_ADD_TC(tp, pow_inf_neg_y);
	ATF_TP_ADD_TC(tp, pow_inf_pos_x);
	ATF_TP_ADD_TC(tp, pow_inf_pos_y);
	ATF_TP_ADD_TC(tp, pow_one_neg_x);
	ATF_TP_ADD_TC(tp, pow_one_pos_x);
	ATF_TP_ADD_TC(tp, pow_zero_x);
	ATF_TP_ADD_TC(tp, pow_zero_y);

	ATF_TP_ADD_TC(tp, powf_nan_x);
	ATF_TP_ADD_TC(tp, powf_nan_y);
	ATF_TP_ADD_TC(tp, powf_inf_neg_x);
	ATF_TP_ADD_TC(tp, powf_inf_neg_y);
	ATF_TP_ADD_TC(tp, powf_inf_pos_x);
	ATF_TP_ADD_TC(tp, powf_inf_pos_y);
	ATF_TP_ADD_TC(tp, powf_one_neg_x);
	ATF_TP_ADD_TC(tp, powf_one_pos_x);
	ATF_TP_ADD_TC(tp, powf_zero_x);
	ATF_TP_ADD_TC(tp, powf_zero_y);

	return atf_no_error();
}
