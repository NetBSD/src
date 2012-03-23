/* $NetBSD: t_acos.c,v 1.3 2012/03/23 23:45:31 matt Exp $ */

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
 * acos(3)
 */
ATF_TC(acos_nan);
ATF_TC_HEAD(acos_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acos(NaN) == NaN");
}

ATF_TC_BODY(acos_nan, tc)
{
#ifndef __vax__
	const double x = 0.0L / 0.0L;

	if (isnan(acos(x)) == 0)
		atf_tc_fail_nonfatal("acos(NaN) != NaN");
#endif
}

ATF_TC(acos_inf_neg);
ATF_TC_HEAD(acos_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acos(-Inf) == NaN");
}

ATF_TC_BODY(acos_inf_neg, tc)
{
#ifndef __vax__
	const double x = -1.0L / 0.0L;

	if (isnan(acos(x)) == 0)
		atf_tc_fail_nonfatal("acos(-Inf) != NaN");
#endif
}

ATF_TC(acos_inf_pos);
ATF_TC_HEAD(acos_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acos(+Inf) == NaN");
}

ATF_TC_BODY(acos_inf_pos, tc)
{
#ifndef __vax__
	const double x = 1.0L / 0.0L;

	if (isnan(acos(x)) == 0)
		atf_tc_fail_nonfatal("acos(+Inf) != NaN");
#endif
}

ATF_TC(acos_one_pos);
ATF_TC_HEAD(acos_one_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acos(1.0) == +0.0");
}

ATF_TC_BODY(acos_one_pos, tc)
{
#ifndef __vax__
	const double y = acos(1.0);

	if (fabs(y) > 0.0 || signbit(y) != 0)
		atf_tc_fail_nonfatal("acos(1.0) != +0.0");
#endif
}

ATF_TC(acos_range);
ATF_TC_HEAD(acos_range, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acos(x) == NaN, x < -1, x > 1");
}

ATF_TC_BODY(acos_range, tc)
{
#ifndef __vax__
	const double x[] = { -1.1, -1.000000001, 1.1, 1.000000001 };
	size_t i;

	for (i = 0; i < __arraycount(x); i++) {

		if (isnan(acos(x[i])) == 0)
			atf_tc_fail_nonfatal("acos(%f) != NaN", x[i]);
	}
#endif
}

ATF_TC(acos_cos);
ATF_TC_HEAD(acos_cos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acos(cos(x)) == x");
}

ATF_TC_BODY(acos_cos, tc)
{
#ifndef __vax__
	const double x[] = { 0.0, 1.0, M_PI / 2, M_PI / 3, M_PI / 6 };
	const double eps = 1.0e-15;
	double y;
	size_t i;

	for (i = 0; i < __arraycount(x); i++) {

		y = acos(cos(x[i]));

		if (fabs(y - x[i]) > eps)
			atf_tc_fail_nonfatal(
			    "acos(cos(%0.03f)) != %0.03f (eps=%0.03e)",
			    x[i], x[i], fabs(y - x[i]));
	}
#endif
}

/*
 * acosf(3)
 */
ATF_TC(acosf_nan);
ATF_TC_HEAD(acosf_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acosf(NaN) == NaN");
}

ATF_TC_BODY(acosf_nan, tc)
{
#ifndef __vax__
	const float x = 0.0L / 0.0L;

	if (isnan(acosf(x)) == 0)
		atf_tc_fail_nonfatal("acosf(NaN) != NaN");
#endif
}

ATF_TC(acosf_inf_neg);
ATF_TC_HEAD(acosf_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acosf(-Inf) == NaN");
}

ATF_TC_BODY(acosf_inf_neg, tc)
{
#ifndef __vax__
	const float x = -1.0L / 0.0L;

	if (isnan(acosf(x)) == 0)
		atf_tc_fail_nonfatal("acosf(-Inf) != NaN");
#endif
}

ATF_TC(acosf_inf_pos);
ATF_TC_HEAD(acosf_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acosf(+Inf) == NaN");
}

ATF_TC_BODY(acosf_inf_pos, tc)
{
#ifndef __vax__
	const float x = 1.0L / 0.0L;

	if (isnan(acosf(x)) == 0)
		atf_tc_fail_nonfatal("acosf(+Inf) != NaN");
#endif
}

ATF_TC(acosf_one_pos);
ATF_TC_HEAD(acosf_one_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acosf(1.0) == +0.0");
}

ATF_TC_BODY(acosf_one_pos, tc)
{
#ifndef __vax__
	const float y = acosf(1.0);

	if (fabsf(y) > 0.0 || signbit(y) != 0)
		atf_tc_fail_nonfatal("acosf(1.0) != +0.0");
#endif
}

ATF_TC(acosf_range);
ATF_TC_HEAD(acosf_range, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acosf(x) == NaN, x < -1, x > 1");
}

ATF_TC_BODY(acosf_range, tc)
{
#ifndef __vax__
	const float x[] = { -1.1, -1.0000001, 1.1, 1.0000001 };
	size_t i;

	for (i = 0; i < __arraycount(x); i++) {

		if (isnan(acosf(x[i])) == 0)
			atf_tc_fail_nonfatal("acosf(%f) != NaN", x[i]);
	}
#endif
}

ATF_TC(acosf_cosf);
ATF_TC_HEAD(acosf_cosf, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test acosf(cosf(x)) == x");
}

ATF_TC_BODY(acosf_cosf, tc)
{
#ifndef __vax__
	const float x[] = { 0.0, 1.0, M_PI / 2, M_PI / 3, M_PI / 6 };
	const float eps = 1.0e-5;
	float y;
	size_t i;

	for (i = 0; i < __arraycount(x); i++) {

		y = acosf(cosf(x[i]));

		if (fabsf(y - x[i]) > eps)
			atf_tc_fail_nonfatal(
			    "acosf(cosf(%0.03f)) != %0.03f (eps=%0.03e)",
			    x[i], x[i], fabs(y - x[i]));
	}
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, acos_nan);
	ATF_TP_ADD_TC(tp, acos_inf_neg);
	ATF_TP_ADD_TC(tp, acos_inf_pos);
	ATF_TP_ADD_TC(tp, acos_one_pos);
	ATF_TP_ADD_TC(tp, acos_range);
	ATF_TP_ADD_TC(tp, acos_cos);

	ATF_TP_ADD_TC(tp, acosf_nan);
	ATF_TP_ADD_TC(tp, acosf_inf_neg);
	ATF_TP_ADD_TC(tp, acosf_inf_pos);
	ATF_TP_ADD_TC(tp, acosf_one_pos);
	ATF_TP_ADD_TC(tp, acosf_range);
	ATF_TP_ADD_TC(tp, acosf_cosf);

	return atf_no_error();
}
