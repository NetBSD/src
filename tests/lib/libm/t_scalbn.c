/* $NetBSD: t_scalbn.c,v 1.6 2011/09/13 06:50:41 jruoho Exp $ */

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
__RCSID("$NetBSD: t_scalbn.c,v 1.6 2011/09/13 06:50:41 jruoho Exp $");

#include <math.h>
#include <limits.h>

#include <atf-c.h>

static const int exps[] = { 0, 1, -1, 100, -100 };

/*
 * scalbn(3)
 */
ATF_TC(scalbn_nan);
ATF_TC_HEAD(scalbn_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbn(NaN) == NaN");
}

ATF_TC_BODY(scalbn_nan, tc)
{
#ifndef __vax__
	const double x = 0.0L / 0.0L;
	double y;
	size_t i;

	ATF_REQUIRE(isnan(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbn(x, exps[i]);
		ATF_CHECK(isnan(y) != 0);
	}
#endif
}

ATF_TC(scalbn_inf_neg);
ATF_TC_HEAD(scalbn_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbn(-Inf) == -Inf");
}

ATF_TC_BODY(scalbn_inf_neg, tc)
{
#ifndef __vax__
	const double x = -1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(scalbn(x, exps[i]) == x);
#endif
}

ATF_TC(scalbn_inf_pos);
ATF_TC_HEAD(scalbn_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbn(+Inf) == +Inf");
}

ATF_TC_BODY(scalbn_inf_pos, tc)
{
#ifndef __vax__
	const double x = 1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(scalbn(x, exps[i]) == x);
#endif
}

ATF_TC(scalbn_ldexp);
ATF_TC_HEAD(scalbn_ldexp, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbn(x) == ldexp(x)");
}

ATF_TC_BODY(scalbn_ldexp, tc)
{
#ifndef __vax__
#if FLT_RADIX == 2
	const double x = 2.91288191221812821;
	double y;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbn(x, exps[i]);
		ATF_CHECK(y == ldexp(x, exps[i]));
	}
#endif
#endif
}

ATF_TC(scalbn_zero_neg);
ATF_TC_HEAD(scalbn_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbn(-0.0) == -0.0");
}

ATF_TC_BODY(scalbn_zero_neg, tc)
{
#ifndef __vax__
	const double x = -0.0L;
	double y;
	size_t i;

	ATF_REQUIRE(signbit(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbn(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) != 0);
	}
#endif
}

ATF_TC(scalbn_zero_pos);
ATF_TC_HEAD(scalbn_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbn(+0.0) == +0.0");
}

ATF_TC_BODY(scalbn_zero_pos, tc)
{
#ifndef __vax__
	const double x = 0.0L;
	double y;
	size_t i;

	ATF_REQUIRE(signbit(x) == 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbn(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) == 0);
	}
#endif
}

/*
 * scalbnf(3)
 */
ATF_TC(scalbnf_nan);
ATF_TC_HEAD(scalbnf_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnf(NaN) == NaN");
}

ATF_TC_BODY(scalbnf_nan, tc)
{
#ifndef __vax__
	const float x = 0.0L / 0.0L;
	float y;
	size_t i;

	ATF_REQUIRE(isnan(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnf(x, exps[i]);
		ATF_CHECK(isnan(y) != 0);
	}
#endif
}

ATF_TC(scalbnf_inf_neg);
ATF_TC_HEAD(scalbnf_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnf(-Inf) == -Inf");
}

ATF_TC_BODY(scalbnf_inf_neg, tc)
{
#ifndef __vax__
	const float x = -1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(scalbnf(x, exps[i]) == x);
#endif
}

ATF_TC(scalbnf_inf_pos);
ATF_TC_HEAD(scalbnf_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnf(+Inf) == +Inf");
}

ATF_TC_BODY(scalbnf_inf_pos, tc)
{
#ifndef __vax__
	const float x = 1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(scalbnf(x, exps[i]) == x);
#endif
}

ATF_TC(scalbnf_ldexp);
ATF_TC_HEAD(scalbnf_ldexp, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnf(x) == ldexpf(x)");
}

ATF_TC_BODY(scalbnf_ldexp, tc)
{
#ifndef __vax__
#if FLT_RADIX == 2
	const float x = 2.91288191221812821;
	float y;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnf(x, exps[i]);
		ATF_CHECK(y == ldexpf(x, exps[i]));
	}
#endif
#endif
}

ATF_TC(scalbnf_zero_neg);
ATF_TC_HEAD(scalbnf_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnf(-0.0) == -0.0");
}

ATF_TC_BODY(scalbnf_zero_neg, tc)
{
#ifndef __vax__
	const float x = -0.0L;
	float y;
	size_t i;

	ATF_REQUIRE(signbit(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnf(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) != 0);
	}
#endif
}

ATF_TC(scalbnf_zero_pos);
ATF_TC_HEAD(scalbnf_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnf(+0.0) == +0.0");
}

ATF_TC_BODY(scalbnf_zero_pos, tc)
{
#ifndef __vax__
	const float x = 0.0L;
	float y;
	size_t i;

	ATF_REQUIRE(signbit(x) == 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnf(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) == 0);
	}
#endif
}

/*
 * scalbnl(3)
 */
ATF_TC(scalbnl_nan);
ATF_TC_HEAD(scalbnl_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnl(NaN) == NaN");
}

ATF_TC_BODY(scalbnl_nan, tc)
{
#ifndef __vax__
#ifndef __HAVE_LONG_DOUBLE
	atf_tc_skip("Requires long double support");
#else
	const long double x = 0.0L / 0.0L;
	long double y;
	size_t i;

	if (isnan(x) == 0) {
		atf_tc_expect_fail("PR lib/45362");
		atf_tc_fail("(0.0L / 0.0L) != NaN");
	}

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnl(x, exps[i]);
		ATF_CHECK(isnan(y) != 0);
	}
#endif
#endif
}

ATF_TC(scalbnl_inf_neg);
ATF_TC_HEAD(scalbnl_inf_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnl(-Inf) == -Inf");
}

ATF_TC_BODY(scalbnl_inf_neg, tc)
{
#ifndef __vax__
#ifndef __HAVE_LONG_DOUBLE
	atf_tc_skip("Requires long double support");
#else
	const long double x = -1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(scalbnl(x, exps[i]) == x);
#endif
#endif
}

ATF_TC(scalbnl_inf_pos);
ATF_TC_HEAD(scalbnl_inf_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnl(+Inf) == +Inf");
}

ATF_TC_BODY(scalbnl_inf_pos, tc)
{
#ifndef __vax__
#ifndef __HAVE_LONG_DOUBLE
	atf_tc_skip("Requires long double support");
#else
	const long double x = 1.0L / 0.0L;
	size_t i;

	for (i = 0; i < __arraycount(exps); i++)
		ATF_CHECK(scalbnl(x, exps[i]) == x);
#endif
#endif
}

ATF_TC(scalbnl_zero_neg);
ATF_TC_HEAD(scalbnl_zero_neg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnl(-0.0) == -0.0");
}

ATF_TC_BODY(scalbnl_zero_neg, tc)
{
#ifndef __vax__
#ifndef __HAVE_LONG_DOUBLE
	atf_tc_skip("Requires long double support");
#else
	const long double x = -0.0L;
	long double y;
	size_t i;

	ATF_REQUIRE(signbit(x) != 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnl(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) != 0);
	}
#endif
#endif
}

ATF_TC(scalbnl_zero_pos);
ATF_TC_HEAD(scalbnl_zero_pos, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test scalbnl(+0.0) == +0.0");
}

ATF_TC_BODY(scalbnl_zero_pos, tc)
{
#ifndef __vax__
#ifndef __HAVE_LONG_DOUBLE
	atf_tc_skip("Requires long double support");
#else
	const long double x = 0.0L;
	long double y;
	size_t i;

	ATF_REQUIRE(signbit(x) == 0);

	for (i = 0; i < __arraycount(exps); i++) {
		y = scalbnl(x, exps[i]);
		ATF_CHECK(x == y);
		ATF_CHECK(signbit(y) == 0);
	}
#endif
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, scalbn_nan);
	ATF_TP_ADD_TC(tp, scalbn_inf_neg);
	ATF_TP_ADD_TC(tp, scalbn_inf_pos);
	ATF_TP_ADD_TC(tp, scalbn_ldexp);
	ATF_TP_ADD_TC(tp, scalbn_zero_neg);
	ATF_TP_ADD_TC(tp, scalbn_zero_pos);

	ATF_TP_ADD_TC(tp, scalbnf_nan);
	ATF_TP_ADD_TC(tp, scalbnf_inf_neg);
	ATF_TP_ADD_TC(tp, scalbnf_inf_pos);
	ATF_TP_ADD_TC(tp, scalbnf_ldexp);
	ATF_TP_ADD_TC(tp, scalbnf_zero_neg);
	ATF_TP_ADD_TC(tp, scalbnf_zero_pos);

	ATF_TP_ADD_TC(tp, scalbnl_nan);
	ATF_TP_ADD_TC(tp, scalbnl_inf_neg);
	ATF_TP_ADD_TC(tp, scalbnl_inf_pos);
/*	ATF_TP_ADD_TC(tp, scalbnl_ldexp);	*/
	ATF_TP_ADD_TC(tp, scalbnl_zero_neg);
	ATF_TP_ADD_TC(tp, scalbnl_zero_pos);

	return atf_no_error();
}
