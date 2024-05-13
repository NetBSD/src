/* $NetBSD: t_hypot.c,v 1.8 2024/05/13 20:28:15 rillig Exp $ */

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

#include <atf-c.h>
#include <float.h>
#include <math.h>

#define	CHECK_EQ(i, hypot, a, b, c)					      \
	ATF_CHECK_MSG(hypot(a, b) == (c),				      \
	    "[%u] %s(%a, %a)=%a, expected %a",				      \
	    (i), #hypot, (a), (b), hypot(a, b), (c))

#define	CHECKL_EQ(i, hypot, a, b, c)					      \
	ATF_CHECK_MSG(hypot(a, b) == (c),				      \
	    "[%u] %s(%La, %La)=%La, expected %La",			      \
	    (i), #hypot, (long double)(a), (long double)(b), hypot(a, b),     \
	    (long double)(c))

#define	CHECK_NAN(i, hypot, a, b)					      \
	ATF_CHECK_MSG(isnan(hypot(a, b)),				      \
	    "[%u] %s(%a, %a)=%a, expected NaN",				      \
	    (i), #hypot, (a), (b), hypot(a, b))

#define	CHECKL_NAN(i, hypot, a, b)					      \
	ATF_CHECK_MSG(isnan(hypot(a, b)),				      \
	    "[%u] %s(%La, %La)=%La, expected NaN",			      \
	    (i), #hypot, (long double)(a), (long double)(b), hypot(a, b))

static const float trivial_casesf[] = {
	0,
#ifdef __FLT_HAS_DENORM__
	__FLT_DENORM_MIN__,
	2*__FLT_DENORM_MIN__,
	3*__FLT_DENORM_MIN__,
	FLT_MIN - 3*__FLT_DENORM_MIN__,
	FLT_MIN - 2*__FLT_DENORM_MIN__,
	FLT_MIN - __FLT_DENORM_MIN__,
#endif
	FLT_MIN,
	FLT_MIN*(1 + FLT_EPSILON),
	FLT_MIN*(1 + 2*FLT_EPSILON),
	2*FLT_MIN,
	FLT_EPSILON/2,
	FLT_EPSILON,
	2*FLT_EPSILON,
	1 - 3*FLT_EPSILON/2,
	1 - 2*FLT_EPSILON/2,
	1 - FLT_EPSILON/2,
	1,
	1 + FLT_EPSILON,
	1 + 2*FLT_EPSILON,
	1 + 3*FLT_EPSILON,
	1.5 - 3*FLT_EPSILON,
	1.5 - 2*FLT_EPSILON,
	1.5 - FLT_EPSILON,
	1.5,
	1.5 + FLT_EPSILON,
	1.5 + 2*FLT_EPSILON,
	1.5 + 3*FLT_EPSILON,
	2,
	0.5/FLT_EPSILON - 0.5,
	0.5/FLT_EPSILON,
	0.5/FLT_EPSILON + 0.5,
	1/FLT_EPSILON,
	FLT_MAX,
	INFINITY,
};

static const double trivial_cases[] = {
	0,
#ifdef __DBL_HAS_DENORM__
	__DBL_DENORM_MIN__,
	2*__DBL_DENORM_MIN__,
	3*__DBL_DENORM_MIN__,
	DBL_MIN - 3*__DBL_DENORM_MIN__,
	DBL_MIN - 2*__DBL_DENORM_MIN__,
	DBL_MIN - __DBL_DENORM_MIN__,
#endif
	DBL_MIN,
	DBL_MIN*(1 + DBL_EPSILON),
	DBL_MIN*(1 + 2*DBL_EPSILON),
	2*DBL_MIN,
	DBL_EPSILON/2,
	DBL_EPSILON,
	2*DBL_EPSILON,
	1 - 3*DBL_EPSILON/2,
	1 - 2*DBL_EPSILON/2,
	1 - DBL_EPSILON/2,
	1,
	1 + DBL_EPSILON,
	1 + 2*DBL_EPSILON,
	1 + 3*DBL_EPSILON,
	1.5 - 3*DBL_EPSILON,
	1.5 - 2*DBL_EPSILON,
	1.5 - DBL_EPSILON,
	1.5,
	1.5 + DBL_EPSILON,
	1.5 + 2*DBL_EPSILON,
	1.5 + 3*DBL_EPSILON,
	2,
	1/FLT_EPSILON - 0.5,
	1/FLT_EPSILON,
	1/FLT_EPSILON + 0.5,
	0.5/DBL_EPSILON - 0.5,
	0.5/DBL_EPSILON,
	0.5/DBL_EPSILON + 0.5,
	1/DBL_EPSILON,
	DBL_MAX,
	INFINITY,
};

static const long double trivial_casesl[] = {
	0,
#ifdef __LDBL_HAS_DENORM__
	__LDBL_DENORM_MIN__,
	2*__LDBL_DENORM_MIN__,
	3*__LDBL_DENORM_MIN__,
	LDBL_MIN - 3*__LDBL_DENORM_MIN__,
	LDBL_MIN - 2*__LDBL_DENORM_MIN__,
	LDBL_MIN - __LDBL_DENORM_MIN__,
#endif
	LDBL_MIN,
	LDBL_MIN*(1 + LDBL_EPSILON),
	LDBL_MIN*(1 + 2*LDBL_EPSILON),
	2*LDBL_MIN,
	LDBL_EPSILON/2,
	LDBL_EPSILON,
	2*LDBL_EPSILON,
	1 - 3*LDBL_EPSILON/2,
	1 - 2*LDBL_EPSILON/2,
	1 - LDBL_EPSILON/2,
	1,
	1 + LDBL_EPSILON,
	1 + 2*LDBL_EPSILON,
	1 + 3*LDBL_EPSILON,
	1.5 - 3*LDBL_EPSILON,
	1.5 - 2*LDBL_EPSILON,
	1.5 - LDBL_EPSILON,
	1.5,
	1.5 + LDBL_EPSILON,
	1.5 + 2*LDBL_EPSILON,
	1.5 + 3*LDBL_EPSILON,
	2,
	1/FLT_EPSILON - 0.5,
	1/FLT_EPSILON,
	1/FLT_EPSILON + 0.5,
#ifdef __HAVE_LONG_DOUBLE
	1/DBL_EPSILON - 0.5L,
	1/DBL_EPSILON,
	1/DBL_EPSILON + 0.5L,
#endif
	0.5/LDBL_EPSILON - 0.5,
	0.5/LDBL_EPSILON,
	0.5/LDBL_EPSILON + 0.5,
	1/LDBL_EPSILON,
	LDBL_MAX,
	INFINITY,
};

ATF_TC(hypotf_trivial);
ATF_TC_HEAD(hypotf_trivial, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypotf(x,0) and hypotf(0,x)");
}
ATF_TC_BODY(hypotf_trivial, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(trivial_casesf); i++) {
		volatile float x = trivial_casesf[i];

		CHECK_EQ(i, hypotf, x, +0., x);
		CHECK_EQ(i, hypotf, x, -0., x);
		CHECK_EQ(i, hypotf, +0., x, x);
		CHECK_EQ(i, hypotf, -0., x, x);
		CHECK_EQ(i, hypotf, -x, +0., x);
		CHECK_EQ(i, hypotf, -x, -0., x);
		CHECK_EQ(i, hypotf, +0., -x, x);
		CHECK_EQ(i, hypotf, -0., -x, x);

		if (isinf(INFINITY)) {
			CHECK_EQ(i, hypotf, x, +INFINITY, INFINITY);
			CHECK_EQ(i, hypotf, x, -INFINITY, INFINITY);
			CHECK_EQ(i, hypotf, +INFINITY, x, INFINITY);
			CHECK_EQ(i, hypotf, -INFINITY, x, INFINITY);
			CHECK_EQ(i, hypotf, -x, +INFINITY, INFINITY);
			CHECK_EQ(i, hypotf, -x, -INFINITY, INFINITY);
			CHECK_EQ(i, hypotf, +INFINITY, -x, INFINITY);
			CHECK_EQ(i, hypotf, -INFINITY, -x, INFINITY);
		}

#ifdef NAN
		if (isinf(x)) {
			CHECK_EQ(i, hypotf, x, NAN, INFINITY);
			CHECK_EQ(i, hypotf, NAN, x, INFINITY);
			CHECK_EQ(i, hypotf, -x, NAN, INFINITY);
			CHECK_EQ(i, hypotf, NAN, -x, INFINITY);
		} else {
			CHECK_NAN(i, hypotf, x, NAN);
			CHECK_NAN(i, hypotf, NAN, x);
			CHECK_NAN(i, hypotf, -x, NAN);
			CHECK_NAN(i, hypotf, NAN, -x);
		}
#endif
	}
}

ATF_TC(hypot_trivial);
ATF_TC_HEAD(hypot_trivial, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypot(x,0) and hypot(0,x)");
}
ATF_TC_BODY(hypot_trivial, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(trivial_casesf); i++) {
		volatile double x = trivial_casesf[i];

		CHECK_EQ(i, hypot, x, +0., x);
		CHECK_EQ(i, hypot, x, -0., x);
		CHECK_EQ(i, hypot, +0., x, x);
		CHECK_EQ(i, hypot, -0., x, x);
		CHECK_EQ(i, hypot, -x, +0., x);
		CHECK_EQ(i, hypot, -x, -0., x);
		CHECK_EQ(i, hypot, +0., -x, x);
		CHECK_EQ(i, hypot, -0., -x, x);

		if (isinf(INFINITY)) {
			CHECK_EQ(i, hypot, x, +INFINITY, INFINITY);
			CHECK_EQ(i, hypot, x, -INFINITY, INFINITY);
			CHECK_EQ(i, hypot, +INFINITY, x, INFINITY);
			CHECK_EQ(i, hypot, -INFINITY, x, INFINITY);
			CHECK_EQ(i, hypot, -x, +INFINITY, INFINITY);
			CHECK_EQ(i, hypot, -x, -INFINITY, INFINITY);
			CHECK_EQ(i, hypot, +INFINITY, -x, INFINITY);
			CHECK_EQ(i, hypot, -INFINITY, -x, INFINITY);
		}

#ifdef NAN
		if (isinf(x)) {
			CHECK_EQ(i, hypot, x, NAN, INFINITY);
			CHECK_EQ(i, hypot, NAN, x, INFINITY);
			CHECK_EQ(i, hypot, -x, NAN, INFINITY);
			CHECK_EQ(i, hypot, NAN, -x, INFINITY);
		} else {
			CHECK_NAN(i, hypot, x, NAN);
			CHECK_NAN(i, hypot, NAN, x);
			CHECK_NAN(i, hypot, -x, NAN);
			CHECK_NAN(i, hypot, NAN, -x);
		}
#endif
	}

	for (i = 0; i < __arraycount(trivial_cases); i++) {
		volatile double x = trivial_cases[i];

		CHECK_EQ(i, hypot, x, +0., x);
		CHECK_EQ(i, hypot, x, -0., x);
		CHECK_EQ(i, hypot, +0., x, x);
		CHECK_EQ(i, hypot, -0., x, x);
		CHECK_EQ(i, hypot, -x, +0., x);
		CHECK_EQ(i, hypot, -x, -0., x);
		CHECK_EQ(i, hypot, +0., -x, x);
		CHECK_EQ(i, hypot, -0., -x, x);

		if (isinf(INFINITY)) {
			CHECK_EQ(i, hypot, x, +INFINITY, INFINITY);
			CHECK_EQ(i, hypot, x, -INFINITY, INFINITY);
			CHECK_EQ(i, hypot, +INFINITY, x, INFINITY);
			CHECK_EQ(i, hypot, -INFINITY, x, INFINITY);
			CHECK_EQ(i, hypot, -x, +INFINITY, INFINITY);
			CHECK_EQ(i, hypot, -x, -INFINITY, INFINITY);
			CHECK_EQ(i, hypot, +INFINITY, -x, INFINITY);
			CHECK_EQ(i, hypot, -INFINITY, -x, INFINITY);
		}

#ifdef NAN
		if (isinf(x)) {
			CHECK_EQ(i, hypot, x, NAN, INFINITY);
			CHECK_EQ(i, hypot, NAN, x, INFINITY);
			CHECK_EQ(i, hypot, -x, NAN, INFINITY);
			CHECK_EQ(i, hypot, NAN, -x, INFINITY);
		} else {
			CHECK_NAN(i, hypot, x, NAN);
			CHECK_NAN(i, hypot, NAN, x);
			CHECK_NAN(i, hypot, -x, NAN);
			CHECK_NAN(i, hypot, NAN, -x);
		}
#endif
	}
}

ATF_TC(hypotl_trivial);
ATF_TC_HEAD(hypotl_trivial, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypotl(x,0) and hypotl(0,x)");
}
ATF_TC_BODY(hypotl_trivial, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(trivial_casesf); i++) {
		volatile long double x = trivial_casesf[i];

		CHECKL_EQ(i, hypotl, x, +0.L, x);
		CHECKL_EQ(i, hypotl, x, -0.L, x);
		CHECKL_EQ(i, hypotl, +0.L, x, x);
		CHECKL_EQ(i, hypotl, -0.L, x, x);
		CHECKL_EQ(i, hypotl, -x, +0.L, x);
		CHECKL_EQ(i, hypotl, -x, -0.L, x);
		CHECKL_EQ(i, hypotl, +0.L, -x, x);
		CHECKL_EQ(i, hypotl, -0.L, -x, x);

		if (isinf(INFINITY)) {
			CHECKL_EQ(i, hypotl, x, +INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, x, -INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, +INFINITY, x, INFINITY);
			CHECKL_EQ(i, hypotl, -INFINITY, x, INFINITY);
			CHECKL_EQ(i, hypotl, -x, +INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, -x, -INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, +INFINITY, -x, INFINITY);
			CHECKL_EQ(i, hypotl, -INFINITY, -x, INFINITY);
		}

#ifdef NAN
		if (isinf(x)) {
			CHECKL_EQ(i, hypotl, x, NAN, INFINITY);
			CHECKL_EQ(i, hypotl, NAN, x, INFINITY);
			CHECKL_EQ(i, hypotl, -x, NAN, INFINITY);
			CHECKL_EQ(i, hypotl, NAN, -x, INFINITY);
		} else {
			CHECKL_NAN(i, hypotl, x, NAN);
			CHECKL_NAN(i, hypotl, NAN, x);
			CHECKL_NAN(i, hypotl, -x, NAN);
			CHECKL_NAN(i, hypotl, NAN, -x);
		}
#endif
	}

	for (i = 0; i < __arraycount(trivial_cases); i++) {
		volatile long double x = trivial_cases[i];

		CHECKL_EQ(i, hypotl, x, +0.L, x);
		CHECKL_EQ(i, hypotl, x, -0.L, x);
		CHECKL_EQ(i, hypotl, +0.L, x, x);
		CHECKL_EQ(i, hypotl, -0.L, x, x);
		CHECKL_EQ(i, hypotl, -x, +0.L, x);
		CHECKL_EQ(i, hypotl, -x, -0.L, x);
		CHECKL_EQ(i, hypotl, +0.L, -x, x);
		CHECKL_EQ(i, hypotl, -0.L, -x, x);

		if (isinf(INFINITY)) {
			CHECKL_EQ(i, hypotl, x, +INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, x, -INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, +INFINITY, x, INFINITY);
			CHECKL_EQ(i, hypotl, -INFINITY, x, INFINITY);
			CHECKL_EQ(i, hypotl, -x, +INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, -x, -INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, +INFINITY, -x, INFINITY);
			CHECKL_EQ(i, hypotl, -INFINITY, -x, INFINITY);
		}

#ifdef NAN
		if (isinf(x)) {
			CHECKL_EQ(i, hypotl, x, NAN, INFINITY);
			CHECKL_EQ(i, hypotl, NAN, x, INFINITY);
			CHECKL_EQ(i, hypotl, -x, NAN, INFINITY);
			CHECKL_EQ(i, hypotl, NAN, -x, INFINITY);
		} else {
			CHECKL_NAN(i, hypotl, x, NAN);
			CHECKL_NAN(i, hypotl, NAN, x);
			CHECKL_NAN(i, hypotl, -x, NAN);
			CHECKL_NAN(i, hypotl, NAN, -x);
		}
#endif
	}

	for (i = 0; i < __arraycount(trivial_casesl); i++) {
		volatile long double x = trivial_casesl[i];

		CHECKL_EQ(i, hypotl, x, +0.L, x);
		CHECKL_EQ(i, hypotl, x, -0.L, x);
		CHECKL_EQ(i, hypotl, +0.L, x, x);
		CHECKL_EQ(i, hypotl, -0.L, x, x);
		CHECKL_EQ(i, hypotl, -x, +0.L, x);
		CHECKL_EQ(i, hypotl, -x, -0.L, x);
		CHECKL_EQ(i, hypotl, +0.L, -x, x);
		CHECKL_EQ(i, hypotl, -0.L, -x, x);

		if (isinf(INFINITY)) {
			CHECKL_EQ(i, hypotl, x, +INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, x, -INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, +INFINITY, x, INFINITY);
			CHECKL_EQ(i, hypotl, -INFINITY, x, INFINITY);
			CHECKL_EQ(i, hypotl, -x, +INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, -x, -INFINITY, INFINITY);
			CHECKL_EQ(i, hypotl, +INFINITY, -x, INFINITY);
			CHECKL_EQ(i, hypotl, -INFINITY, -x, INFINITY);
		}

#ifdef NAN
		if (isinf(x)) {
			CHECKL_EQ(i, hypotl, x, NAN, INFINITY);
			CHECKL_EQ(i, hypotl, NAN, x, INFINITY);
			CHECKL_EQ(i, hypotl, -x, NAN, INFINITY);
			CHECKL_EQ(i, hypotl, NAN, -x, INFINITY);
		} else {
			CHECKL_NAN(i, hypotl, x, NAN);
			CHECKL_NAN(i, hypotl, NAN, x);
			CHECKL_NAN(i, hypotl, -x, NAN);
			CHECKL_NAN(i, hypotl, NAN, -x);
		}
#endif
	}
}

/*
 * All primitive Pythagorean triples are generated from coprime
 * integers m > n > 0 by Euclid's formula,
 *
 *	a = m^2 - n^2
 *	b = 2 m n,
 *	c = m^2 + n^2.
 *
 * We test cases with various different numbers and positions of bits,
 * generated by this formula.
 *
 * If you're curious, you can recover m and n from a, b, and c by
 *
 *	m = sqrt((a + c)/2)
 *	n = b/2m.
 */

__CTASSERT(FLT_MANT_DIG >= 24);
static const struct {
	float a, b, c;
} exact_casesf[] = {
	{ 3, 4, 5 },
	{ 5, 12, 13 },
	{ 9, 12, 15 },
	{ 0x1001, 0x801000, 0x801001 },
	{ 4248257, 1130976, 4396225 },
};

__CTASSERT(DBL_MANT_DIG >= 53);
static const struct {
	double a, b, c;
} exact_cases[] = {
	{ 3378249543467007, 4505248894795776, 5631148868747265 },
	{ 0x7ffffff, 0x1ffffff8000000, 0x1ffffff8000001 },
#if DBL_MANT_DIG >= 56
	{ 13514123525517439, 18018830919909120, 22523538851237761 },
	{ 0x1fffffff, 0x1ffffffe0000000, 0x1ffffffe0000001 },
#endif
};

#if LDBL_MANT_DIG >= 64
static const struct {
	long double a, b, c;
} exact_casesl[] = {
	{ 3458976450080784639, 4611968592949214720, 5764960744407842561 },
	{ 0x200000000, 0x7ffffffffffffffe, 0x8000000000000002 },
#if LDBL_MANT_DIG >= 113
	{ 973555668229277869436257492279295.L,
	  1298074224305703705819019479072768.L,
	  1622592780382129686316970078625793.L },
	{ 0x1ffffffffffffff,
	  0x1fffffffffffffe00000000000000p0L,
	  0x1fffffffffffffe00000000000001p0L },
#endif
};
#endif

ATF_TC(hypotf_exact);
ATF_TC_HEAD(hypotf_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypotf on scaled Pythagorean triples");
}
ATF_TC_BODY(hypotf_exact, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(exact_casesf); i++) {
		int s;

		for (s = FLT_MIN_EXP;
		     s < FLT_MAX_EXP - FLT_MANT_DIG;
		     s += (FLT_MAX_EXP - FLT_MANT_DIG - FLT_MIN_EXP)/5) {
			volatile double a = ldexpf(exact_casesf[i].a, s);
			volatile double b = ldexpf(exact_casesf[i].b, s);
			float c = ldexpf(exact_casesf[i].c, s);

			CHECK_EQ(i, hypot, a, b, c);
			CHECK_EQ(i, hypot, b, a, c);
			CHECK_EQ(i, hypot, a, -b, c);
			CHECK_EQ(i, hypot, b, -a, c);
			CHECK_EQ(i, hypot, -a, b, c);
			CHECK_EQ(i, hypot, -b, a, c);
			CHECK_EQ(i, hypot, -a, -b, c);
			CHECK_EQ(i, hypot, -b, -a, c);
		}
	}
}

ATF_TC(hypot_exact);
ATF_TC_HEAD(hypot_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypot on scaled Pythagorean triples");
}
ATF_TC_BODY(hypot_exact, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(exact_casesf); i++) {
		int s;

		for (s = DBL_MIN_EXP;
		     s < DBL_MAX_EXP - DBL_MANT_DIG;
		     s += (DBL_MAX_EXP - DBL_MANT_DIG - DBL_MIN_EXP)/5) {
			volatile double a = ldexp(exact_casesf[i].a, s);
			volatile double b = ldexp(exact_casesf[i].b, s);
			double c = ldexp(exact_casesf[i].c, s);

			CHECK_EQ(i, hypot, a, b, c);
			CHECK_EQ(i, hypot, b, a, c);
			CHECK_EQ(i, hypot, a, -b, c);
			CHECK_EQ(i, hypot, b, -a, c);
			CHECK_EQ(i, hypot, -a, b, c);
			CHECK_EQ(i, hypot, -b, a, c);
			CHECK_EQ(i, hypot, -a, -b, c);
			CHECK_EQ(i, hypot, -b, -a, c);
		}
	}

	for (i = 0; i < __arraycount(exact_cases); i++) {
		int s;

		for (s = DBL_MIN_EXP;
		     s < DBL_MAX_EXP - DBL_MANT_DIG;
		     s += (DBL_MAX_EXP - DBL_MANT_DIG - DBL_MIN_EXP)/5) {
			volatile double a = ldexp(exact_cases[i].a, s);
			volatile double b = ldexp(exact_cases[i].b, s);
			double c = ldexp(exact_cases[i].c, s);

			CHECK_EQ(i, hypot, a, b, c);
			CHECK_EQ(i, hypot, b, a, c);
			CHECK_EQ(i, hypot, a, -b, c);
			CHECK_EQ(i, hypot, b, -a, c);
			CHECK_EQ(i, hypot, -a, b, c);
			CHECK_EQ(i, hypot, -b, a, c);
			CHECK_EQ(i, hypot, -a, -b, c);
			CHECK_EQ(i, hypot, -b, -a, c);
		}
	}
}

ATF_TC(hypotl_exact);
ATF_TC_HEAD(hypotl_exact, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypotl on scaled Pythagorean triples");
}
ATF_TC_BODY(hypotl_exact, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(exact_casesf); i++) {
		int s;

		for (s = LDBL_MIN_EXP;
		     s < LDBL_MAX_EXP - LDBL_MANT_DIG;
		     s += (LDBL_MAX_EXP - LDBL_MANT_DIG - LDBL_MIN_EXP)/5) {
			volatile long double a = ldexpl(exact_casesf[i].a, s);
			volatile long double b = ldexpl(exact_casesf[i].b, s);
			long double c = ldexpl(exact_casesf[i].c, s);

			CHECKL_EQ(i, hypotl, a, b, c);
			CHECKL_EQ(i, hypotl, b, a, c);
			CHECKL_EQ(i, hypotl, a, -b, c);
			CHECKL_EQ(i, hypotl, b, -a, c);
			CHECKL_EQ(i, hypotl, -a, b, c);
			CHECKL_EQ(i, hypotl, -b, a, c);
			CHECKL_EQ(i, hypotl, -a, -b, c);
			CHECKL_EQ(i, hypotl, -b, -a, c);
		}
	}

	for (i = 0; i < __arraycount(exact_cases); i++) {
		int s;

		for (s = LDBL_MIN_EXP;
		     s < LDBL_MAX_EXP - LDBL_MANT_DIG;
		     s += (LDBL_MAX_EXP - LDBL_MANT_DIG - LDBL_MIN_EXP)/5) {
			volatile long double a = ldexpl(exact_cases[i].a, s);
			volatile long double b = ldexpl(exact_cases[i].b, s);
			long double c = ldexpl(exact_cases[i].c, s);

			CHECKL_EQ(i, hypotl, a, b, c);
			CHECKL_EQ(i, hypotl, b, a, c);
			CHECKL_EQ(i, hypotl, a, -b, c);
			CHECKL_EQ(i, hypotl, b, -a, c);
			CHECKL_EQ(i, hypotl, -a, b, c);
			CHECKL_EQ(i, hypotl, -b, a, c);
			CHECKL_EQ(i, hypotl, -a, -b, c);
			CHECKL_EQ(i, hypotl, -b, -a, c);
		}
	}

#if LDBL_MANT_DIG >= 64
	for (i = 0; i < __arraycount(exact_casesl); i++) {
		int s;

		for (s = LDBL_MIN_EXP;
		     s < LDBL_MAX_EXP - LDBL_MANT_DIG;
		     s += (LDBL_MAX_EXP - LDBL_MANT_DIG - LDBL_MIN_EXP)/5) {
			volatile long double a = ldexpl(exact_casesl[i].a, s);
			volatile long double b = ldexpl(exact_casesl[i].b, s);
			long double c = ldexpl(exact_casesl[i].c, s);

			CHECKL_EQ(i, hypotl, a, b, c);
			CHECKL_EQ(i, hypotl, b, a, c);
			CHECKL_EQ(i, hypotl, a, -b, c);
			CHECKL_EQ(i, hypotl, b, -a, c);
			CHECKL_EQ(i, hypotl, -a, b, c);
			CHECKL_EQ(i, hypotl, -b, a, c);
			CHECKL_EQ(i, hypotl, -a, -b, c);
			CHECKL_EQ(i, hypotl, -b, -a, c);
		}
	}
#endif
}

ATF_TC(hypot_nan);
ATF_TC_HEAD(hypot_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "hypot/hypotf/hypotl(NAN, NAN)");
}
ATF_TC_BODY(hypot_nan, tc)
{
#ifdef NAN
	CHECK_NAN(0, hypot, NAN, NAN);
	CHECK_NAN(1, hypotf, NAN, NAN);
	CHECKL_NAN(2, hypotl, NAN, NAN);
#else
	atf_tc_skip("no NaNs on this architecture");
#endif
}

ATF_TC(pr50698);
ATF_TC_HEAD(pr50698, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check for the bug of PR lib/50698");
}

ATF_TC_BODY(pr50698, tc)
{
	volatile float a = 1e-18f;
	float val = hypotf(a, a);
	ATF_CHECK(!isinf(val));
	ATF_CHECK(!isnan(val));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, hypot_exact);
	ATF_TP_ADD_TC(tp, hypot_nan);
	ATF_TP_ADD_TC(tp, hypot_trivial);
	ATF_TP_ADD_TC(tp, hypotf_exact);
	ATF_TP_ADD_TC(tp, hypotf_trivial);
	ATF_TP_ADD_TC(tp, hypotl_exact);
	ATF_TP_ADD_TC(tp, hypotl_trivial);
	ATF_TP_ADD_TC(tp, pr50698);

	return atf_no_error();
}
