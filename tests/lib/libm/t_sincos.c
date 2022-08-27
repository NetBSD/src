/* $NetBSD: t_sincos.c,v 1.1 2022/08/27 08:31:58 christos Exp $ */

/*-
 * Copyright (c) 2011, 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen and Christos Zoulas
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

#include <assert.h>
#include <atf-c.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static const struct {
	int		angle;
	double		x;
	double		y;
	float		fy;
} sin_angles[] = {
//	{ -360, -6.283185307179586,  2.4492935982947064e-16, -1.7484555e-07 },
	{ -180, -3.141592653589793, -1.2246467991473532e-16, 8.7422777e-08 },
	{ -135, -2.356194490192345, -0.7071067811865476, 999 },
//	{  -90, -1.570796326794897, -1.0000000000000000, 999 },
	{  -45, -0.785398163397448, -0.7071067811865472, 999 },
	{    0,  0.000000000000000,  0.0000000000000000, 999 },
	{   30,  0.5235987755982989, 0.5000000000000000, 999 },
	{   45,  0.785398163397448,  0.7071067811865472, 999 },
//	{   60,  1.047197551196598,  0.8660254037844388, 999 },
	{   90,  1.570796326794897,  1.0000000000000000, 999 },
//	{  120,  2.094395102393195,  0.8660254037844389, 999 },
	{  135,  2.356194490192345,  0.7071067811865476, 999 },
	{  150,  2.617993877991494,  0.5000000000000003, 999 },
	{  180,  3.141592653589793,  1.2246467991473532e-16, -8.7422777e-08 },
	{  270,  4.712388980384690, -1.0000000000000000, 999 },
	{  360,  6.283185307179586, -2.4492935982947064e-16, 1.7484555e-07 },
};

static const struct {
	int		angle;
	double		x;
	double		y;
	float		fy;
} cos_angles[] = {
	{ -180, -3.141592653589793, -1.0000000000000000, 999 },
	{ -135, -2.356194490192345, -0.7071067811865476, 999 },
//	{  -90, -1.5707963267948966, 6.123233995736766e-17, -4.3711388e-08 },
//	{  -90, -1.5707963267948968, -1.6081226496766366e-16, -4.3711388e-08 },
	{  -45, -0.785398163397448,  0.7071067811865478, 999 },
	{    0,  0.000000000000000,  1.0000000000000000, 999 },
	{   30,  0.5235987755982989, 0.8660254037844386, 999 },
	{   45,  0.785398163397448,  0.7071067811865478, 999 },
//	{   60,  1.0471975511965976,  0.5000000000000001, 999 },
//	{   60,  1.0471975511965979,  0.4999999999999999, 999 },
	{   90,  1.570796326794897, -3.8285686989269494e-16, -4.3711388e-08 },
//	{  120,  2.0943951023931953, -0.4999999999999998, 999 },
//	{  120,  2.0943951023931957, -0.5000000000000002, 999 },
	{  135,  2.356194490192345, -0.7071067811865476, 999 },
	{  150,  2.617993877991494, -0.8660254037844386, 999 },
	{  180,  3.141592653589793, -1.0000000000000000, 999 },
	{  270,  4.712388980384690, -1.8369701987210297e-16, 1.1924881e-08 },
	{  360,  6.283185307179586,  1.0000000000000000, 999 },
};

#ifdef __HAVE_LONG_DOUBLE
/*
 * sincosl(3)
 */
ATF_TC(sincosl_angles);
ATF_TC_HEAD(sincosl_angles, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test some selected angles");
}

ATF_TC_BODY(sincosl_angles, tc)
{
	/*
	 * XXX The given data is for double, so take that
	 * into account and expect less precise results..
	 */
	const long double eps = DBL_EPSILON;
	size_t i;

	ATF_CHECK(__arraycount(sin_angles) == __arraycount(cos_angles));

	for (i = 0; i < __arraycount(sin_angles); i++) {
		ATF_CHECK_MSG(sin_angles[i].angle == cos_angles[i].angle,
		    "%zu %d %d", i, sin_angles[i].angle, cos_angles[i].angle);
		int deg = sin_angles[i].angle;
		ATF_CHECK_MSG(sin_angles[i].x == cos_angles[i].x,
		    "%zu %g %g", i, sin_angles[i].x, cos_angles[i].x);
		long double theta = sin_angles[i].x;
		long double sin_theta = sin_angles[i].y;
		long double cos_theta = cos_angles[i].y;
		long double s, c;

		sincosl(theta, &s, &c);

		if (fabsl((s - sin_theta)/sin_theta) > eps) {
			atf_tc_fail_nonfatal("sin(%d deg = %.17Lg) = %.17Lg"
			    " != %.17Lg",
			    deg, theta, s, sin_theta);
		}
		if (fabsl((c - cos_theta)/cos_theta) > eps) {
			atf_tc_fail_nonfatal("cos(%d deg = %.17Lg) = %.17Lg"
			    " != %.17Lg",
			    deg, theta, c, cos_theta);
		}
	}
}

ATF_TC(sincosl_nan);
ATF_TC_HEAD(sincosl_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosl(NaN) == (NaN, NaN)");
}

ATF_TC_BODY(sincosl_nan, tc)
{
	const long double x = 0.0L / 0.0L;
	long double s, c;

	sincosl(x, &s, &c);
	ATF_CHECK(isnan(x) && isnan(s) && isnan(c));
}

ATF_TC(sincosl_inf_neg);
ATF_TC_HEAD(sincosl_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosl(-Inf) == (NaN, NaN)");
}

ATF_TC_BODY(sincosl_inf_neg, tc)
{
	const long double x = -1.0L / 0.0L;
	long double s, c;

	sincosl(x, &s, &c);
	ATF_CHECK(isnan(s) && isnan(c));
}

ATF_TC(sincosl_inf_pos);
ATF_TC_HEAD(sincosl_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosl(+Inf) == (NaN, NaN)");
}

ATF_TC_BODY(sincosl_inf_pos, tc)
{
	const long double x = 1.0L / 0.0L;
	long double s, c;

	sincosl(x, &s, &c);
	ATF_CHECK(isnan(s) && isnan(c));
}


ATF_TC(sincosl_zero_neg);
ATF_TC_HEAD(sincosl_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosl(-0.0) == (0.0, 1.0)");
}

ATF_TC_BODY(sincosl_zero_neg, tc)
{
	const long double x = -0.0L;
	long double s, c;

	sincosl(x, &s, &c);
	ATF_CHECK(s == 0.0 && c == 1.0);
}

ATF_TC(sincosl_zero_pos);
ATF_TC_HEAD(sincosl_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosl(+0.0) == (0.0, 1.0)");
}

ATF_TC_BODY(sincosl_zero_pos, tc)
{
	const long double x = 0.0L;
	long double s, c;

	sincosl(x, &s, &c);
	ATF_CHECK(s == 0.0 && c == 1.0);
}
#endif

/*
 * sincos(3)
 */
ATF_TC(sincos_angles);
ATF_TC_HEAD(sincos_angles, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test some selected angles");
}

ATF_TC_BODY(sincos_angles, tc)
{
	const double eps = DBL_EPSILON;
	size_t i;

	for (i = 0; i < __arraycount(sin_angles); i++) {
		ATF_CHECK_MSG(sin_angles[i].angle == cos_angles[i].angle,
		    "%zu %d %d", i, sin_angles[i].angle, cos_angles[i].angle);
		int deg = sin_angles[i].angle;
		ATF_CHECK_MSG(sin_angles[i].x == cos_angles[i].x,
		    "%zu %g %g", i, sin_angles[i].x, cos_angles[i].x);
		double theta = sin_angles[i].x;
		double sin_theta = sin_angles[i].y;
		double cos_theta = cos_angles[i].y;
		double s, c;

		sincos(theta, &s, &c);

		if (fabs((s - sin_theta)/sin_theta) > eps) {
			atf_tc_fail_nonfatal("sin(%d deg = %.17g) = %.17g"
			    " != %.17g",
			    deg, theta, s, sin_theta);
		}
		if (fabs((c - cos_theta)/cos_theta) > eps) {
			atf_tc_fail_nonfatal("cos(%d deg = %.17g) = %.17g"
			    " != %.17g",
			    deg, theta, c, cos_theta);
		}
	}
}

ATF_TC(sincos_nan);
ATF_TC_HEAD(sincos_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincos(NaN) == (NaN, NaN)");
}

ATF_TC_BODY(sincos_nan, tc)
{
	const double x = 0.0L / 0.0L;
	double s, c;

	sincos(x, &s, &c);
	ATF_CHECK(isnan(x) && isnan(s) && isnan(c));
}

ATF_TC(sincos_inf_neg);
ATF_TC_HEAD(sincos_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincos(-Inf) == (NaN, NaN)");
}

ATF_TC_BODY(sincos_inf_neg, tc)
{
	const double x = -1.0L / 0.0L;
	double s, c;

	sincos(x, &s, &c);
	ATF_CHECK(isnan(s) && isnan(c));
}

ATF_TC(sincos_inf_pos);
ATF_TC_HEAD(sincos_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincos(+Inf) == (NaN, NaN)");
}

ATF_TC_BODY(sincos_inf_pos, tc)
{
	const double x = 1.0L / 0.0L;
	double s, c;

	sincos(x, &s, &c);
	ATF_CHECK(isnan(s) && isnan(c));
}


ATF_TC(sincos_zero_neg);
ATF_TC_HEAD(sincos_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincos(-0.0) == (0.0, 1.0)");
}

ATF_TC_BODY(sincos_zero_neg, tc)
{
	const double x = -0.0L;
	double s, c;

	sincos(x, &s, &c);
	ATF_CHECK(s == 0 && c == 1.0);
}

ATF_TC(sincos_zero_pos);
ATF_TC_HEAD(sincos_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test cos(+0.0) == (0.0, 1.0)");
}

ATF_TC_BODY(sincos_zero_pos, tc)
{
	const double x = 0.0L;
	double s, c;

	sincos(x, &s, &c);
	ATF_CHECK(s == 0 && c == 1.0);
}

/*
 * sincosf(3)
 */
ATF_TC(sincosf_angles);
ATF_TC_HEAD(sincosf_angles, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test some selected angles");
}

ATF_TC_BODY(sincosf_angles, tc)
{
	const float eps = FLT_EPSILON;
	size_t i;

	for (i = 0; i < __arraycount(sin_angles); i++) {
		ATF_CHECK_MSG(sin_angles[i].angle == cos_angles[i].angle,
		    "%zu %d %d", i, sin_angles[i].angle, cos_angles[i].angle);
		int deg = sin_angles[i].angle;
		ATF_CHECK_MSG(sin_angles[i].x == cos_angles[i].x,
		    "%zu %g %g", i, sin_angles[i].x, cos_angles[i].x);
		float theta = sin_angles[i].x;
		float sin_theta = sin_angles[i].fy;
		float cos_theta = cos_angles[i].fy;
		float s, c;

		sincosf(theta, &s, &c);
		if (cos_theta == 999)
			cos_theta = cos_angles[i].y;
		if (sin_theta == 999)
			sin_theta = sin_angles[i].y;

		if (fabs((s - sin_theta)/sin_theta) > eps) {
			atf_tc_fail_nonfatal("sin(%d deg = %.8g) = %.8g"
			    " != %.8g",
			    deg, theta, s, sin_theta);
		}
		if (fabs((c - cos_theta)/cos_theta) > eps) {
			atf_tc_fail_nonfatal("cos(%d deg = %.8g) = %.8g"
			    " != %.8g",
			    deg, theta, c, cos_theta);
		}
	}
}

ATF_TC(sincosf_nan);
ATF_TC_HEAD(sincosf_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test cosf(NaN) == (NaN, NaN)");
}

ATF_TC_BODY(sincosf_nan, tc)
{
	const float x = 0.0L / 0.0L;
	float s, c;

	sincosf(x, &s, &c);
	ATF_CHECK(isnan(x) && isnan(s) && isnan(c));
}

ATF_TC(sincosf_inf_neg);
ATF_TC_HEAD(sincosf_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test cosf(-Inf) == (NaN, NaN)");
}

ATF_TC_BODY(sincosf_inf_neg, tc)
{
	const float x = -1.0L / 0.0L;
	float s, c;

	sincosf(x, &s, &c);
	ATF_CHECK(isnan(s) && isnan(c));

}

ATF_TC(sincosf_inf_pos);
ATF_TC_HEAD(sincosf_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosf(+Inf) == (NaN, NaN)");
}

ATF_TC_BODY(sincosf_inf_pos, tc)
{
	const float x = 1.0L / 0.0L;
	float s, c;

	sincosf(x, &s, &c);
	ATF_CHECK(isnan(s) && isnan(c));
}


ATF_TC(sincosf_zero_neg);
ATF_TC_HEAD(sincosf_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosf(-0.0) == (0.0, 1.0)");
}

ATF_TC_BODY(sincosf_zero_neg, tc)
{
	const float x = -0.0L;
	float s, c;

	sincosf(x, &s, &c);

	ATF_CHECK(s == 0.0 && c == 1.0);
}

ATF_TC(sincosf_zero_pos);
ATF_TC_HEAD(sincosf_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sincosf(+0.0) == (0.0, 1.0)");
}

ATF_TC_BODY(sincosf_zero_pos, tc)
{
	const float x = 0.0L;

	float s, c;

	sincosf(x, &s, &c);

	ATF_CHECK(s == 0 && c == 1.0);
}

ATF_TP_ADD_TCS(tp)
{
#ifdef __HAVE_LONG_DOUBLE
	ATF_TP_ADD_TC(tp, sincosl_angles);
	ATF_TP_ADD_TC(tp, sincosl_nan);
	ATF_TP_ADD_TC(tp, sincosl_inf_neg);
	ATF_TP_ADD_TC(tp, sincosl_inf_pos);
	ATF_TP_ADD_TC(tp, sincosl_zero_neg);
	ATF_TP_ADD_TC(tp, sincosl_zero_pos);
#endif

	ATF_TP_ADD_TC(tp, sincos_angles);
	ATF_TP_ADD_TC(tp, sincos_nan);
	ATF_TP_ADD_TC(tp, sincos_inf_neg);
	ATF_TP_ADD_TC(tp, sincos_inf_pos);
	ATF_TP_ADD_TC(tp, sincos_zero_neg);
	ATF_TP_ADD_TC(tp, sincos_zero_pos);

	ATF_TP_ADD_TC(tp, sincosf_angles);
	ATF_TP_ADD_TC(tp, sincosf_nan);
	ATF_TP_ADD_TC(tp, sincosf_inf_neg);
	ATF_TP_ADD_TC(tp, sincosf_inf_pos);
	ATF_TP_ADD_TC(tp, sincosf_zero_neg);
	ATF_TP_ADD_TC(tp, sincosf_zero_pos);

	return atf_no_error();
}
