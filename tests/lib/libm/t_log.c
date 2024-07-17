/* $NetBSD: t_log.c,v 1.19 2024/07/17 14:52:13 riastradh Exp $ */

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
__RCSID("$NetBSD: t_log.c,v 1.19 2024/07/17 14:52:13 riastradh Exp $");

#include <sys/types.h>

#include <atf-c.h>

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define	CHECK_EQ(i, f, x, y)						      \
	ATF_CHECK_EQ_MSG(f(x), y,					      \
	    "[%u] %s(%a=%.17g)=%a=%.17g, expected %a=%.17g",		      \
	    (i), #f, (double)(x), (double)(x), f(x), f(x),		      \
	    (double)(y), (double)(y))

#define	CHECKL_EQ(i, f, x, y)						      \
	ATF_CHECK_EQ_MSG(f(x), y,					      \
	    "[%u] %s(%La=%.34Lg)=%La=%.34Lg, expected %La=%.34Lg",	      \
	    (i), #f, (long double)(x), (long double)(x), f(x), f(x),	      \
	    (long double)(y), (long double)(y))

#ifdef NAN

#define	CHECK_NAN(i, f, x)						      \
	ATF_CHECK_MSG(isnan(f(x)),					      \
	    "[%u] %s(%a=%.17g)=%a=%.17g, expected NaN",			      \
	    (i), #f, (x), (x), f(x), f(x))

#define	CHECKL_NAN(i, f, x)						      \
	ATF_CHECK_MSG(isnan(f(x)),					      \
	    "[%u] %s(%La=%.34Lg)=%La=%.34Lg, expected NaN",		      \
	    (i), #f, (long double)(x), (long double)(x), f(x), f(x))

#else  /* !defined(NAN) */

#define	CHECK_NAN(i, f, x) do						      \
{									      \
	int _checknan_error;						      \
	double _checknan_result;					      \
	errno = 0;							      \
	_checknan_result = f(x);					      \
	_checknan_error = errno;					      \
	ATF_CHECK_EQ_MSG(errno, EDOM,					      \
	    "[%u] %s(%a=%.17g)=%a=%.17g errno=%d, expected EDOM=%d",	      \
	    (i), #f, (double)(x), (double)(x),				      \
	    _checknan_result, _checknan_result,				      \
	    _checknan_error, EDOM);					      \
} while (0)

#define	CHECKL_NAN(i, f, x) do						      \
{									      \
	int _checknan_error;						      \
	long double _checknan_result;					      \
	errno = 0;							      \
	_checknan_result = f(x);					      \
	_checknan_error = errno;					      \
	ATF_CHECK_EQ_MSG(errno, EDOM,					      \
	    "[%u] %s(%La=%.34Lg)=%La=%.34Lg errno=%d, expected EDOM=%d",      \
	    (i), #f, (long double)(x), (long double)(x),		      \
	    _checknan_result, _checknan_result,				      \
	    _checknan_error, EDOM);					      \
} while (0)

#endif	/* NAN */

static const float logf_invalid[] = {
#ifdef NAN
	NAN,
#endif
	-HUGE_VALF,
	-FLT_MAX,
	-10,
	-1,
	-FLT_EPSILON,
	-FLT_MIN,
#ifdef FLT_DENORM_MIN
	-FLT_DENORM_MIN,
#endif
};

static const double log_invalid[] = {
#ifdef NAN
	NAN,
#endif
	-HUGE_VAL,
	-DBL_MAX,
	-10,
	-1,
	-DBL_EPSILON,
	-DBL_MIN,
#ifdef DBL_DENORM_MIN
	-DBL_DENORM_MIN,
#endif
};

static const long double logl_invalid[] = {
#ifdef NAN
	NAN,
#endif
	-HUGE_VALL,
	-LDBL_MAX,
	-10,
	-1,
	-LDBL_EPSILON,
	-LDBL_MIN,
#ifdef LDBL_DENORM_MIN
	-LDBL_DENORM_MIN,
#endif
};

static const float log1pf_invalid[] = {
#ifdef NAN
	NAN,
#endif
	-HUGE_VALF,
	-FLT_MAX,
	-10,
	-1 - FLT_EPSILON,
};

static const double log1p_invalid[] = {
#ifdef NAN
	NAN,
#endif
	-HUGE_VAL,
	-DBL_MAX,
	-10,
	-1 - DBL_EPSILON,
};

static const long double log1pl_invalid[] = {
#ifdef NAN
	NAN,
#endif
	-HUGE_VALL,
	-LDBL_MAX,
	-10,
	-1 - LDBL_EPSILON,
};

/*
 * log10(3)
 */
static const struct {
	float x, y;
} log10f_exact[] = {
	{ 1, 0 },
	{ 10, 1 },
	{ 100, 2 },
};

ATF_TC(log10_invalid);
ATF_TC_HEAD(log10_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log10/f/l on invalid inputs");
}
ATF_TC_BODY(log10_invalid, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(logf_invalid); i++) {
		CHECK_NAN(i, log10f, logf_invalid[i]);
		CHECK_NAN(i, log10, logf_invalid[i]);
		CHECKL_NAN(i, log10l, logf_invalid[i]);
	}

	for (i = 0; i < __arraycount(log_invalid); i++) {
		CHECK_NAN(i, log10, log_invalid[i]);
		CHECKL_NAN(i, log10l, log_invalid[i]);
	}

	for (i = 0; i < __arraycount(logl_invalid); i++) {
		CHECKL_NAN(i, log10l, logl_invalid[i]);
	}
}

ATF_TC(log10_zero);
ATF_TC_HEAD(log10_zero, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log10/f/l on zero");
}
ATF_TC_BODY(log10_zero, tc)
{

	CHECK_EQ(0, log10f, +0., -HUGE_VALF);
	CHECK_EQ(0, log10, +0., -HUGE_VAL);
	CHECKL_EQ(0, log10l, +0., -HUGE_VALL);

	CHECK_EQ(1, log10f, -0., -HUGE_VALF);
	CHECK_EQ(1, log10, -0., -HUGE_VAL);
	CHECKL_EQ(1, log10l, -0., -HUGE_VALL);
}

ATF_TC(log10_exact);
ATF_TC_HEAD(log10_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log10/f/l exact cases");
}
ATF_TC_BODY(log10_exact, tc)
{
	unsigned i;

	ATF_CHECK_EQ(signbit(log10f(1)), 0);
	ATF_CHECK_EQ(signbit(log10(1)), 0);
	ATF_CHECK_EQ(signbit(log10l(1)), 0);

	for (i = 0; i < __arraycount(log10f_exact); i++) {
		const float x = log10f_exact[i].x;
		const float y = log10f_exact[i].y;

		CHECK_EQ(i, log10f, x, y);
		CHECK_EQ(i, log10, x, y);
		CHECKL_EQ(i, log10l, x, y);
	}
}

ATF_TC(log10_approx);
ATF_TC_HEAD(log10_approx, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log10/f/l approximate cases");
}
ATF_TC_BODY(log10_approx, tc)
{
	volatile long double e =
	    2.7182818284590452353602874713526624977572470937L;
	volatile long double e2 =
	    7.3890560989306502272304274605750078131803155705519L;
	volatile long double log10e =
	    0.43429448190325182765112891891660508229439700580367L;
	volatile long double log10e2 =
	    2*0.43429448190325182765112891891660508229439700580367L;

	ATF_CHECK_MSG((fabsf((log10f(e) - (float)log10e)/(float)log10e) <
		2*FLT_EPSILON),
	    "log10f(e)=%a=%.8g expected %a=%.8g",
	    log10f(e), log10f(e), (float)log10e, (float)log10e);
	ATF_CHECK_MSG((fabs((log10(e) - (double)log10e)/(double)log10e) <
		2*DBL_EPSILON),
	    "log10(e)=%a=%.17g expected %a=%.17g",
	    log10(e), log10(e), (double)log10e, (double)log10e);
	ATF_CHECK_MSG((fabsl((log10l(e) - log10e)/log10e) < 2*LDBL_EPSILON),
	    "log10l(e)=%La=%.34Lg expected %La=%.34Lg",
	    log10l(e), log10l(e), log10e, log10e);

	ATF_CHECK_MSG((fabsf((log10f(e2) - (float)log10e2)/(float)log10e2) <
		2*FLT_EPSILON),
	    "log10f(e^2)=%a=%.8g expected %a=%.8g",
	    log10f(e2), log10f(e2), (float)log10e2, (float)log10e2);
	ATF_CHECK_MSG((fabs((log10(e2) - (double)log10e2)/(double)log10e2) <
		2*DBL_EPSILON),
	    "log10(e^2)=%a=%.17g expected %a=%.17g",
	    log10(e2), log10(e2), (double)log10e2, (double)log10e2);
	ATF_CHECK_MSG((fabsl((log10l(e2) - log10e2)/log10e2) < 2*LDBL_EPSILON),
	    "log10l(e^2)=%La=%.34Lg expected %La=%.34Lg",
	    log10l(e2), log10l(e2), log10e2, log10e2);
}

ATF_TC(log10_inf);
ATF_TC_HEAD(log10_inf, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log10/f/l on +infinity");
}
ATF_TC_BODY(log10_inf, tc)
{

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	CHECK_EQ(0, log10f, INFINITY, INFINITY);
	CHECK_EQ(0, log10, INFINITY, INFINITY);
	CHECKL_EQ(0, log10l, INFINITY, INFINITY);
}

/*
 * log1p(3)
 */

ATF_TC(log1p_invalid);
ATF_TC_HEAD(log1p_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log1p/f/l on invalid inputs");
}
ATF_TC_BODY(log1p_invalid, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(log1pf_invalid); i++) {
		CHECK_NAN(i, log1pf, log1pf_invalid[i]);
		CHECK_NAN(i, log1p, log1pf_invalid[i]);
		CHECKL_NAN(i, log1pl, log1pf_invalid[i]);
	}

	for (i = 0; i < __arraycount(log1p_invalid); i++) {
		CHECK_NAN(i, log1p, log1p_invalid[i]);
		CHECKL_NAN(i, log1pl, log1p_invalid[i]);
	}

	for (i = 0; i < __arraycount(log1pl_invalid); i++) {
		CHECKL_NAN(i, log1pl, log1pl_invalid[i]);
	}
}

ATF_TC(log1p_neg_one);
ATF_TC_HEAD(log1p_neg_one, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log1p/f/l on -1");
}
ATF_TC_BODY(log1p_neg_one, tc)
{

	CHECK_EQ(0, log1pf, -1., -HUGE_VALF);
	CHECK_EQ(0, log1p, -1., -HUGE_VAL);
	CHECKL_EQ(0, log1pl, -1., -HUGE_VALL);
}

ATF_TC(log1p_exact);
ATF_TC_HEAD(log1p_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log1p/f/l exact cases");
}
ATF_TC_BODY(log1p_exact, tc)
{

	CHECK_EQ(0, log1pf, -FLT_MIN, -FLT_MIN);
	CHECK_EQ(0, log1p, -DBL_MIN, -DBL_MIN);
	CHECKL_EQ(01, log1pl, -LDBL_MIN, -LDBL_MIN);

	CHECK_EQ(1, log1pf, -0., 0);
	CHECK_EQ(1, log1p, -0., 0);
	CHECKL_EQ(1, log1pl, -0., 0);

	CHECK_EQ(2, log1pf, +0., 0);
	CHECK_EQ(2, log1p, +0., 0);
	CHECKL_EQ(2, log1pl, +0., 0);

#ifdef __i386__
	atf_tc_expect_fail("PR port-i386/58434: single-float functions"
	    " sometimes return surprisingly much precision");
#endif
	CHECK_EQ(3, log1pf, 1, logf(2));
#ifdef __i386__
	atf_tc_expect_pass();
#endif
	CHECK_EQ(3, log1p, 1, log(2));
	CHECKL_EQ(3, log1pl, 1, logl(2));
}

ATF_TC(log1p_approx);
ATF_TC_HEAD(log1p_approx, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log1p/f/l approximate cases");
}
ATF_TC_BODY(log1p_approx, tc)
{
	volatile long double em1 =	/* exp(1) - 1 */
	    1.7182818284590452353602874713526624977572470937L;
	volatile long double e2m1 =	/* exp(2) - 1 */
	    6.3890560989306502272304274605750078131803155705519L;

	/*
	 * Approximation is close enough that equality of the rounded
	 * output had better hold.
	 */
#ifdef FLT_DENORM_MIN
	CHECK_EQ(0, log1pf, -FLT_DENORM_MIN, -FLT_DENORM_MIN);
#endif
#ifdef DBL_DENORM_MIN
	CHECK_EQ(0, log1p, -DBL_DENORM_MIN, -DBL_DENORM_MIN);
#endif
#ifdef LDBL_DENORM_MIN
	CHECKL_EQ(0, log1pl, -LDBL_DENORM_MIN, -LDBL_DENORM_MIN);
#endif

	ATF_CHECK_MSG(fabsf((log1pf(em1) - 1)/1) < 2*FLT_EPSILON,
	    "log1pf(e)=%a=%.8g", log1pf(em1), log1pf(em1));
	ATF_CHECK_MSG(fabs((log1p(em1) - 1)/1) < 2*DBL_EPSILON,
	    "log1p(e)=%a=%.17g", log1p(em1), log1p(em1));
	ATF_CHECK_MSG(fabsl((log1pl(em1) - 1)/1) < 2*LDBL_EPSILON,
	    "log1pl(e)=%La=%.34Lg", log1pl(em1), log1pl(em1));

	ATF_CHECK_MSG(fabsf((log1pf(e2m1) - 2)/2) < 2*FLT_EPSILON,
	    "log1pf(e^2)=%a=%.8g", log1pf(em1), log1pf(em1));
	ATF_CHECK_MSG(fabs((log1p(e2m1) - 2)/2) < 2*DBL_EPSILON,
	    "log1p(e^2)=%a=%.17g", log1p(em1), log1p(em1));
	ATF_CHECK_MSG(fabsl((log1pl(e2m1) - 2)/2) < 2*LDBL_EPSILON,
	    "log1pl(e^2)=%La=%.34Lg", log1pl(em1), log1pl(em1));
}

ATF_TC(log1p_inf);
ATF_TC_HEAD(log1p_inf, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log1p/f/l on +infinity");
}
ATF_TC_BODY(log1p_inf, tc)
{

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	CHECK_EQ(0, log1pf, INFINITY, INFINITY);
	CHECK_EQ(0, log1p, INFINITY, INFINITY);
	CHECKL_EQ(0, log1pl, INFINITY, INFINITY);
}

/*
 * log2(3)
 */
static const struct {
	float x, y;
} log2f_exact[] = {
#ifdef FLT_DENORM_MIN
	{ FLT_DENORM_MIN, FLT_MIN_EXP - FLT_MANT_DIG },
#endif
	{ FLT_MIN, FLT_MIN_EXP - 1 },
	{ 0.25, -2 },
	{ 0.5, -1 },
	{ 1, 0 },
	{ 2, 1 },
	{ 4, 2 },
	{ 8, 3 },
	{ 1 << FLT_MANT_DIG, FLT_MANT_DIG },
	{ (float)(1 << FLT_MANT_DIG) * (1 << FLT_MANT_DIG),
	  2*FLT_MANT_DIG },
};
static const struct {
	double x, y;
} log2_exact[] = {
#ifdef DBL_DENORM_MIN
	{ DBL_DENORM_MIN, DBL_MIN_EXP - DBL_MANT_DIG },
#endif
	{ DBL_MIN, DBL_MIN_EXP - 1 },
	{ (uint64_t)1 << DBL_MANT_DIG, DBL_MANT_DIG },
	{ ((double)((uint64_t)1 << DBL_MANT_DIG) *
		    ((uint64_t)1 << DBL_MANT_DIG)),
	  2*DBL_MANT_DIG },
};

static const struct {
	long double x, y;
} log2l_exact[] = {
#ifdef LDBL_DENORM_MIN
	{ LDBL_DENORM_MIN, LDBL_MIN_EXP - LDBL_MANT_DIG },
#endif
	{ LDBL_MIN, LDBL_MIN_EXP - 1 },
	{ ((long double)((uint64_t)1 << (LDBL_MANT_DIG/2)) *
		    ((uint64_t)1 << ((LDBL_MANT_DIG + 1)/2))),
	  LDBL_MANT_DIG },
	{ (((long double)((uint64_t)1 << (LDBL_MANT_DIG/2)) *
			((uint64_t)1 << ((LDBL_MANT_DIG + 1)/2))) *
		    ((long double)((uint64_t)1 << (LDBL_MANT_DIG/2)) *
			((uint64_t)1 << ((LDBL_MANT_DIG + 1)/2)))),
	  2*LDBL_MANT_DIG },
};

ATF_TC(log2_invalid);
ATF_TC_HEAD(log2_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log2/f/l on invalid inputs");
}
ATF_TC_BODY(log2_invalid, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(logf_invalid); i++) {
		CHECK_NAN(i, log2f, logf_invalid[i]);
		CHECK_NAN(i, log2, logf_invalid[i]);
		CHECKL_NAN(i, log2l, logf_invalid[i]);
	}

	for (i = 0; i < __arraycount(log_invalid); i++) {
		CHECK_NAN(i, log2, log_invalid[i]);
		CHECKL_NAN(i, log2l, log_invalid[i]);
	}

	for (i = 0; i < __arraycount(logl_invalid); i++) {
		CHECKL_NAN(i, log2l, logl_invalid[i]);
	}
}

ATF_TC(log2_zero);
ATF_TC_HEAD(log2_zero, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log2/f/l on zero");
}
ATF_TC_BODY(log2_zero, tc)
{

	CHECK_EQ(0, log2f, +0., -HUGE_VALF);
	CHECK_EQ(0, log2, +0., -HUGE_VAL);
	CHECKL_EQ(0, log2l, +0., -HUGE_VALL);

	CHECK_EQ(1, log2f, -0., -HUGE_VALF);
	CHECK_EQ(1, log2, -0., -HUGE_VAL);
	CHECKL_EQ(1, log2l, -0., -HUGE_VALL);
}

ATF_TC(log2_exact);
ATF_TC_HEAD(log2_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log2/f/l exact cases");
}
ATF_TC_BODY(log2_exact, tc)
{
	unsigned i;

	ATF_CHECK_EQ(signbit(log2f(1)), 0);
	ATF_CHECK_EQ(signbit(log2(1)), 0);
	ATF_CHECK_EQ(signbit(log2l(1)), 0);

	for (i = 0; i < __arraycount(log2f_exact); i++) {
		const float x = log2f_exact[i].x;
		const float y = log2f_exact[i].y;

		CHECK_EQ(i, log2f, x, y);
		CHECK_EQ(i, log2, x, y);
		CHECKL_EQ(i, log2l, x, y);
	}

	for (i = 0; i < __arraycount(log2_exact); i++) {
		const double x = log2_exact[i].x;
		const double y = log2_exact[i].y;

		CHECK_EQ(i, log2, x, y);
		CHECKL_EQ(i, log2l, x, y);
	}

	for (i = 0; i < __arraycount(log2l_exact); i++) {
		const long double x = log2l_exact[i].x;
		const long double y = log2l_exact[i].y;

		CHECKL_EQ(i, log2l, x, y);
	}
}

ATF_TC(log2_approx);
ATF_TC_HEAD(log2_approx, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log2/f/l approximate cases");
}
ATF_TC_BODY(log2_approx, tc)
{
	volatile long double e =
	    2.7182818284590452353602874713526624977572470937L;
	volatile long double e2 =
	    7.3890560989306502272304274605750078131803155705519L;
	volatile long double log2e =
	    1.442695040888963407359924681001892137426645954153L;
	volatile long double log2e2 =
	    2*1.442695040888963407359924681001892137426645954153L;

	ATF_CHECK_MSG((fabsf((log2f(e) - (float)log2e)/(float)log2e) <
		2*FLT_EPSILON),
	    "log2f(e)=%a=%.8g expected %a=%.8g",
	    log2f(e), log2f(e), (float)log2e, (float)log2e);
	ATF_CHECK_MSG((fabs((log2(e) - (double)log2e)/(double)log2e) <
		2*DBL_EPSILON),
	    "log2(e)=%a=%.17g expected %a=%.17g",
	    log2(e), log2(e), (double)log2e, (double)log2e);
	ATF_CHECK_MSG((fabsl((log2l(e) - log2e)/log2e) < 2*LDBL_EPSILON),
	    "log2l(e)=%La=%.34Lg expected %La=%.34Lg",
	    log2l(e), log2l(e), log2e, log2e);

	ATF_CHECK_MSG((fabsf((log2f(e2) - (float)log2e2)/(float)log2e2) <
		2*FLT_EPSILON),
	    "log2f(e^2)=%a=%.8g expected %a=%.8g",
	    log2f(e2), log2f(e2), (float)log2e2, (float)log2e2);
	ATF_CHECK_MSG((fabs((log2(e2) - (double)log2e2)/(double)log2e2) <
		2*DBL_EPSILON),
	    "log2(e^2)=%a=%.17g expected %a=%.17g",
	    log2(e2), log2(e2), (double)log2e2, (double)log2e2);
	ATF_CHECK_MSG((fabsl((log2l(e2) - log2e2)/log2e2) < 2*LDBL_EPSILON),
	    "log2l(e^2)=%La=%.34Lg expected %La=%.34Lg",
	    log2l(e2), log2l(e2), log2e2, log2e2);
}

ATF_TC(log2_inf);
ATF_TC_HEAD(log2_inf, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log2/f/l on +infinity");
}
ATF_TC_BODY(log2_inf, tc)
{

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	CHECK_EQ(0, log2f, INFINITY, INFINITY);
	CHECK_EQ(0, log2, INFINITY, INFINITY);
	CHECKL_EQ(0, log2l, INFINITY, INFINITY);
}

/*
 * log(3)
 */

ATF_TC(log_invalid);
ATF_TC_HEAD(log_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log/f/l on invalid inputs");
}
ATF_TC_BODY(log_invalid, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(logf_invalid); i++) {
		CHECK_NAN(i, logf, logf_invalid[i]);
		CHECK_NAN(i, log, logf_invalid[i]);
		CHECKL_NAN(i, logl, logf_invalid[i]);
	}

	for (i = 0; i < __arraycount(log_invalid); i++) {
		CHECK_NAN(i, log, log_invalid[i]);
		CHECKL_NAN(i, logl, log_invalid[i]);
	}

	for (i = 0; i < __arraycount(logl_invalid); i++) {
		CHECKL_NAN(i, logl, logl_invalid[i]);
	}
}

ATF_TC(log_zero);
ATF_TC_HEAD(log_zero, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log/f/l on zero");
}
ATF_TC_BODY(log_zero, tc)
{

	CHECK_EQ(0, logf, +0., -HUGE_VALF);
	CHECK_EQ(0, log, +0., -HUGE_VAL);
	CHECKL_EQ(0, logl, +0., -HUGE_VALL);

	CHECK_EQ(1, logf, -0., -HUGE_VALF);
	CHECK_EQ(1, log, -0., -HUGE_VAL);
	CHECKL_EQ(1, logl, -0., -HUGE_VALL);
}

ATF_TC(log_exact);
ATF_TC_HEAD(log_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log/f/l exact cases");
}
ATF_TC_BODY(log_exact, tc)
{

	CHECK_EQ(0, logf, 1, 0);
	CHECK_EQ(0, log, 1, 0);
	CHECKL_EQ(0, logl, 1, 0);

	ATF_CHECK_EQ(signbit(logf(1)), 0);
	ATF_CHECK_EQ(signbit(log(1)), 0);
	ATF_CHECK_EQ(signbit(logl(1)), 0);
}

ATF_TC(log_approx);
ATF_TC_HEAD(log_approx, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log/f/l approximate cases");
}
ATF_TC_BODY(log_approx, tc)
{
	volatile long double e =
	    2.7182818284590452353602874713526624977572470937L;
	volatile long double e2 =
	    7.3890560989306502272304274605750078131803155705519L;
	volatile long double log_2 =
	    0.69314718055994530941723212145817656807550013436025L;
	volatile long double log_10 =
	    2.30258509299404568401799145468436420760110148862875L;

	ATF_CHECK_MSG(fabsf((logf(2) - log_2)/log_2) < 2*FLT_EPSILON,
	    "logf(2)=%a=%.8g expected %a=%.8g",
	    logf(2), logf(2), (float)log_2, (float)log_2);
	ATF_CHECK_MSG(fabs((log(2) - log_2)/log_2) < 2*DBL_EPSILON,
	    "log(2)=%a=%.17g expected %a=%.17g",
	    log(2), log(2), (double)log_2, (double)log_2);
	ATF_CHECK_MSG(fabsl((logl(2) - log_2)/log_2) < 2*LDBL_EPSILON,
	    "logl(2)=%La=%.34Lg expected %La=%.34Lg",
	    logl(2), logl(2), log_2, log_2);

	ATF_CHECK_MSG(fabsf((logf(e) - 1)/1) < 2*FLT_EPSILON,
	    "logf(e)=%a=%.8g", logf(e), logf(e));
	ATF_CHECK_MSG(fabs((log(e) - 1)/1) < 2*DBL_EPSILON,
	    "log(e)=%a=%.17g", log(e), log(e));
	ATF_CHECK_MSG(fabsl((logl(e) - 1)/1) < 2*LDBL_EPSILON,
	    "logl(e)=%La=%.34Lg", logl(e), logl(e));

	ATF_CHECK_MSG(fabsf((logf(e2) - 2)/2) < 2*FLT_EPSILON,
	    "logf(e)=%a=%.8g", logf(e2), logf(e2));
	ATF_CHECK_MSG(fabs((log(e2) - 2)/2) < 2*DBL_EPSILON,
	    "log(e)=%a=%.17g", log(e2), log(e2));
	ATF_CHECK_MSG(fabsl((logl(e2) - 2)/2) < 2*LDBL_EPSILON,
	    "logl(e)=%La=%.34Lg", logl(e2), logl(e2));

	ATF_CHECK_MSG(fabsf((logf(10) - log_10)/log_10) < 2*FLT_EPSILON,
	    "logf(10)=%a=%.8g expected %a=%.8g",
	    logf(10), logf(10), (float)log_10, (float)log_10);
	ATF_CHECK_MSG(fabs((log(10) - log_10)/log_10) < 2*DBL_EPSILON,
	    "log(10)=%a=%.17g expected %a=%.17g",
	    log(10), log(10), (double)log_10, (double)log_10);
	ATF_CHECK_MSG(fabsl((logl(10) - log_10)/log_10) < 2*LDBL_EPSILON,
	    "logl(10)=%La=%.34Lg expected %La=%.34Lg",
	    logl(10), logl(10), log_10, log_10);
}

ATF_TC(log_inf);
ATF_TC_HEAD(log_inf, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log/f/l on +infinity");
}
ATF_TC_BODY(log_inf, tc)
{

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	CHECK_EQ(0, logf, INFINITY, INFINITY);
	CHECK_EQ(0, log, INFINITY, INFINITY);
	CHECKL_EQ(0, logl, INFINITY, INFINITY);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, log10_invalid);
	ATF_TP_ADD_TC(tp, log10_zero);
	ATF_TP_ADD_TC(tp, log10_exact);
	ATF_TP_ADD_TC(tp, log10_approx);
	ATF_TP_ADD_TC(tp, log10_inf);

	ATF_TP_ADD_TC(tp, log1p_invalid);
	ATF_TP_ADD_TC(tp, log1p_neg_one);
	ATF_TP_ADD_TC(tp, log1p_exact);
	ATF_TP_ADD_TC(tp, log1p_approx);
	ATF_TP_ADD_TC(tp, log1p_inf);

	ATF_TP_ADD_TC(tp, log2_invalid);
	ATF_TP_ADD_TC(tp, log2_zero);
	ATF_TP_ADD_TC(tp, log2_exact);
	ATF_TP_ADD_TC(tp, log2_approx);
	ATF_TP_ADD_TC(tp, log2_inf);

	ATF_TP_ADD_TC(tp, log_invalid);
	ATF_TP_ADD_TC(tp, log_zero);
	ATF_TP_ADD_TC(tp, log_exact);
	ATF_TP_ADD_TC(tp, log_approx);
	ATF_TP_ADD_TC(tp, log_inf);

	return atf_no_error();
}
