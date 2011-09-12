/* $NetBSD: t_ldexp.c,v 1.2 2011/09/12 15:47:14 jruoho Exp $ */

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
__RCSID("$NetBSD: t_ldexp.c,v 1.2 2011/09/12 15:47:14 jruoho Exp $");

#include <math.h>
#include <limits.h>

#include <atf-c.h>

static const int exps[] = { 0, 1, -1, 100, -100 };

/*
 * ldexp(3)
 */
ATF_TC(ldexp_nan);
ATF_TC_HEAD(ldexp_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test NaN with ldexp(3)");
}

ATF_TC_BODY(ldexp_nan, tc)
{
	const double x = 0.0L / 0.0L;
	double y;
	size_t i;

	ATF_REQUIRE(isnan(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = ldexp(x, exps[i]);
		ATF_CHECK(isnan(y) != 0);
	}
}

ATF_TC(ldexp_inf_neg);
ATF_TC_HEAD(ldexp_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test -Inf with ldexp(3)");
}

ATF_TC_BODY(ldexp_inf_neg, tc)
{
	const double x = -1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(ldexp(x, exps[i]) == x);
}

ATF_TC(ldexp_inf_pos);
ATF_TC_HEAD(ldexp_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test +Inf with ldexp(3)");
}

ATF_TC_BODY(ldexp_inf_pos, tc)
{
	const double x = 1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(ldexp(x, exps[i]) == x);
}

ATF_TC(ldexp_zero_neg);
ATF_TC_HEAD(ldexp_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test -0.0 with ldexp(3)");
}

ATF_TC_BODY(ldexp_zero_neg, tc)
{
	const double x = -0.0L;
	double y;
	size_t i;

	ATF_REQUIRE(signbit(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = ldexp(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) != 0);
	}
}

ATF_TC(ldexp_zero_pos);
ATF_TC_HEAD(ldexp_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test +0.0 with ldexp(3)");
}

ATF_TC_BODY(ldexp_zero_pos, tc)
{
	const double x = 0.0L;
	double y;
	size_t i;

	ATF_REQUIRE(signbit(x) == 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = ldexp(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) == 0);
	}
}

ATF_TC(ldexpf_nan);
ATF_TC_HEAD(ldexpf_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test NaN with ldexpf(3)");
}

ATF_TC_BODY(ldexpf_nan, tc)
{
	const float x = 0.0L / 0.0L;
	float y;
	size_t i;

	ATF_REQUIRE(isnan(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = ldexpf(x, exps[i]);
		ATF_CHECK(isnan(y) != 0);
	}
}

/*
 * ldexpf(3)
 */

ATF_TC(ldexpf_inf_neg);
ATF_TC_HEAD(ldexpf_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test -Inf with ldexpf(3)");
}

ATF_TC_BODY(ldexpf_inf_neg, tc)
{
	const float x = -1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(ldexpf(x, exps[i]) == x);
}

ATF_TC(ldexpf_inf_pos);
ATF_TC_HEAD(ldexpf_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test +Inf with ldexpf(3)");
}

ATF_TC_BODY(ldexpf_inf_pos, tc)
{
	const float x = 1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(ldexpf(x, exps[i]) == x);
}

ATF_TC(ldexpf_zero_neg);
ATF_TC_HEAD(ldexpf_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test -0.0 with ldexpf(3)");
}

ATF_TC_BODY(ldexpf_zero_neg, tc)
{
	const float x = -0.0L;
	float y;
	size_t i;

	ATF_REQUIRE(signbit(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = ldexpf(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) != 0);
	}
}

ATF_TC(ldexpf_zero_pos);
ATF_TC_HEAD(ldexpf_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test +0.0 with ldexpf(3)");
}

ATF_TC_BODY(ldexpf_zero_pos, tc)
{
	const float x = 0.0L;
	float y;
	size_t i;

	ATF_REQUIRE(signbit(x) == 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = ldexpf(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) == 0);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ldexp_nan);
	ATF_TP_ADD_TC(tp, ldexp_inf_neg);
	ATF_TP_ADD_TC(tp, ldexp_inf_pos);
	ATF_TP_ADD_TC(tp, ldexp_zero_neg);
	ATF_TP_ADD_TC(tp, ldexp_zero_pos);

	ATF_TP_ADD_TC(tp, ldexpf_nan);
	ATF_TP_ADD_TC(tp, ldexpf_inf_neg);
	ATF_TP_ADD_TC(tp, ldexpf_inf_pos);
	ATF_TP_ADD_TC(tp, ldexpf_zero_neg);
	ATF_TP_ADD_TC(tp, ldexpf_zero_pos);

	return atf_no_error();
}
