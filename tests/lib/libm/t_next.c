/*	$NetBSD: t_next.c,v 1.7.4.3 2024/10/15 13:07:42 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_next.c,v 1.7.4.3 2024/10/15 13:07:42 martin Exp $");

#include <atf-c.h>
#include <float.h>
#include <math.h>

#ifdef __vax__		/* XXX PR 57881: vax libm is missing various symbols */

ATF_TC(vaxafter);
ATF_TC_HEAD(vaxafter, tc)
{

	atf_tc_set_md_var(tc, "descr", "vax nextafter/nexttoward reminder");
}
ATF_TC_BODY(vaxafter, tc)
{

	atf_tc_expect_fail("PR 57881: vax libm is missing various symbols");
	atf_tc_fail("missing nextafter{,f,l} and nexttoward{,f,l} on vax");
}

#else  /* !__vax__ */

#define	CHECK(i, next, x, d, y) do					      \
{									      \
	volatile __typeof__(x) check_x = (x);				      \
	volatile __typeof__(d) check_d = (d);				      \
	volatile __typeof__(y) check_y = (y);				      \
	const volatile __typeof__(y) check_tmp = (next)(check_x, check_d);    \
	ATF_CHECK_MSG(check_tmp == check_y,				      \
	    "[%u] %s(%s=%La=%Lg, %s=%La=%Lg)=%La=%Lg != %s=%La=%Lg",	      \
	    (i), #next,							      \
	    #x, (long double)check_x, (long double)check_x,		      \
	    #d, (long double)check_d, (long double)check_d,		      \
	    (long double)check_tmp, (long double)check_tmp,		      \
	    #y, (long double)check_y, (long double)check_y);		      \
} while (0)

/*
 * check(x, n)
 *
 *	x[0], x[1], ..., x[n - 1] are consecutive double floating-point
 *	numbers.  Verify nextafter and nexttoward follow exactly this
 *	sequence, forward and back, and in negative.
 */
static void
check(const double *x, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		CHECK(i, nextafter, x[i], x[i], x[i]);
		CHECK(i, nexttoward, x[i], x[i], x[i]);
		CHECK(i, nextafter, -x[i], -x[i], -x[i]);
		CHECK(i, nexttoward, -x[i], -x[i], -x[i]);
	}

	for (i = 0; i < n - 1; i++) {
		ATF_REQUIRE_MSG(x[i] < x[i + 1], "i=%u", i);

		if (isnormal(x[i])) {
			CHECK(i, nexttoward, x[i], x[i]*(1 + LDBL_EPSILON),
			    x[i + 1]);
		}

		CHECK(i, nextafter, x[i], x[i + 1], x[i + 1]);
		CHECK(i, nexttoward, x[i], x[i + 1], x[i + 1]);
		CHECK(i, nextafter, x[i], x[n - 1], x[i + 1]);
		CHECK(i, nexttoward, x[i], x[n - 1], x[i + 1]);
		CHECK(i, nextafter, x[i], INFINITY, x[i + 1]);
		CHECK(i, nexttoward, x[i], INFINITY, x[i + 1]);

		CHECK(i, nextafter, -x[i], -x[i + 1], -x[i + 1]);
		CHECK(i, nexttoward, -x[i], -x[i + 1], -x[i + 1]);
		CHECK(i, nextafter, -x[i], -x[n - 1], -x[i + 1]);
		CHECK(i, nexttoward, -x[i], -x[n - 1], -x[i + 1]);
		CHECK(i, nextafter, -x[i], -INFINITY, -x[i + 1]);
		CHECK(i, nexttoward, -x[i], -INFINITY, -x[i + 1]);
	}

	for (i = n; i --> 1;) {
		ATF_REQUIRE_MSG(x[i - 1] < x[i], "i=%u", i);

#ifdef __HAVE_LONG_DOUBLE
		if (isnormal(x[i])) {
			CHECK(i, nexttoward, x[i], x[i]*(1 - LDBL_EPSILON/2),
			    x[i - 1]);
		}
#endif

		CHECK(i, nextafter, x[i], x[i - 1], x[i - 1]);
		CHECK(i, nexttoward, x[i], x[i - 1], x[i - 1]);
		CHECK(i, nextafter, x[i], x[0], x[i - 1]);
		CHECK(i, nexttoward, x[i], x[0], x[i - 1]);
		CHECK(i, nextafter, x[i], +0., x[i - 1]);
		CHECK(i, nexttoward, x[i], +0., x[i - 1]);
		CHECK(i, nextafter, x[i], -0., x[i - 1]);
		CHECK(i, nexttoward, x[i], -0., x[i - 1]);
		CHECK(i, nextafter, x[i], -x[0], x[i - 1]);
		CHECK(i, nexttoward, x[i], -x[0], x[i - 1]);
		CHECK(i, nextafter, x[i], -x[i], x[i - 1]);
		CHECK(i, nexttoward, x[i], -x[i], x[i - 1]);
		CHECK(i, nextafter, x[i], -INFINITY, x[i - 1]);
		CHECK(i, nexttoward, x[i], -INFINITY, x[i - 1]);

		CHECK(i, nextafter, -x[i], -x[i - 1], -x[i - 1]);
		CHECK(i, nexttoward, -x[i], -x[i - 1], -x[i - 1]);
		CHECK(i, nextafter, -x[i], -x[0], -x[i - 1]);
		CHECK(i, nexttoward, -x[i], -x[0], -x[i - 1]);
		CHECK(i, nextafter, -x[i], -0., -x[i - 1]);
		CHECK(i, nexttoward, -x[i], -0., -x[i - 1]);
		CHECK(i, nextafter, -x[i], +0., -x[i - 1]);
		CHECK(i, nexttoward, -x[i], +0., -x[i - 1]);
		CHECK(i, nextafter, -x[i], x[0], -x[i - 1]);
		CHECK(i, nexttoward, -x[i], x[0], -x[i - 1]);
		CHECK(i, nextafter, -x[i], INFINITY, -x[i - 1]);
		CHECK(i, nexttoward, -x[i], INFINITY, -x[i - 1]);
	}
}

/*
 * checkf(x, n)
 *
 *	x[0], x[1], ..., x[n - 1] are consecutive single floating-point
 *	numbers.  Verify nextafterf and nexttowardf follow exactly this
 *	sequence, forward and back, and in negative.
 */
static void
checkf(const float *x, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		CHECK(i, nextafterf, x[i], x[i], x[i]);
		CHECK(i, nexttowardf, x[i], x[i], x[i]);
		CHECK(i, nextafterf, -x[i], -x[i], -x[i]);
		CHECK(i, nexttowardf, -x[i], -x[i], -x[i]);
	}

	for (i = 0; i < n - 1; i++) {
		ATF_REQUIRE_MSG(x[i] < x[i + 1], "i=%u", i);

		if (isnormal(x[i])) {
			CHECK(i, nexttowardf, x[i], x[i]*(1 + LDBL_EPSILON),
			    x[i + 1]);
		}

		CHECK(i, nextafterf, x[i], x[i + 1], x[i + 1]);
		CHECK(i, nexttowardf, x[i], x[i + 1], x[i + 1]);
		CHECK(i, nextafterf, x[i], x[n - 1], x[i + 1]);
		CHECK(i, nexttowardf, x[i], x[n - 1], x[i + 1]);
		CHECK(i, nextafterf, x[i], INFINITY, x[i + 1]);
		CHECK(i, nexttowardf, x[i], INFINITY, x[i + 1]);

		CHECK(i, nextafterf, -x[i], -x[i + 1], -x[i + 1]);
		CHECK(i, nexttowardf, -x[i], -x[i + 1], -x[i + 1]);
		CHECK(i, nextafterf, -x[i], -x[n - 1], -x[i + 1]);
		CHECK(i, nexttowardf, -x[i], -x[n - 1], -x[i + 1]);
		CHECK(i, nextafterf, -x[i], -INFINITY, -x[i + 1]);
		CHECK(i, nexttowardf, -x[i], -INFINITY, -x[i + 1]);
	}

	for (i = n; i --> 1;) {
		ATF_REQUIRE_MSG(x[i - 1] < x[i], "i=%u", i);

		if (isnormal(x[i])) {
			CHECK(i, nexttowardf, x[i], x[i]*(1 - LDBL_EPSILON/2),
			    x[i - 1]);
		}

		CHECK(i, nextafterf, x[i], x[i - 1], x[i - 1]);
		CHECK(i, nexttowardf, x[i], x[i - 1], x[i - 1]);
		CHECK(i, nextafterf, x[i], x[0], x[i - 1]);
		CHECK(i, nexttowardf, x[i], x[0], x[i - 1]);
		CHECK(i, nextafterf, x[i], +0., x[i - 1]);
		CHECK(i, nexttowardf, x[i], +0., x[i - 1]);
		CHECK(i, nextafterf, x[i], -0., x[i - 1]);
		CHECK(i, nexttowardf, x[i], -0., x[i - 1]);
		CHECK(i, nextafterf, x[i], -x[0], x[i - 1]);
		CHECK(i, nexttowardf, x[i], -x[0], x[i - 1]);
		CHECK(i, nextafterf, x[i], -x[i], x[i - 1]);
		CHECK(i, nexttowardf, x[i], -x[i], x[i - 1]);
		CHECK(i, nextafterf, x[i], -INFINITY, x[i - 1]);
		CHECK(i, nexttowardf, x[i], -INFINITY, x[i - 1]);

		CHECK(i, nextafterf, -x[i], -x[i - 1], -x[i - 1]);
		CHECK(i, nexttowardf, -x[i], -x[i - 1], -x[i - 1]);
		CHECK(i, nextafterf, -x[i], -x[0], -x[i - 1]);
		CHECK(i, nexttowardf, -x[i], -x[0], -x[i - 1]);
		CHECK(i, nextafterf, -x[i], -0., -x[i - 1]);
		CHECK(i, nexttowardf, -x[i], -0., -x[i - 1]);
		CHECK(i, nextafterf, -x[i], +0., -x[i - 1]);
		CHECK(i, nexttowardf, -x[i], +0., -x[i - 1]);
		CHECK(i, nextafterf, -x[i], x[0], -x[i - 1]);
		CHECK(i, nexttowardf, -x[i], x[0], -x[i - 1]);
		CHECK(i, nextafterf, -x[i], INFINITY, -x[i - 1]);
		CHECK(i, nexttowardf, -x[i], INFINITY, -x[i - 1]);
	}
}

/*
 * checkl(x, n)
 *
 *	x[0], x[1], ..., x[n - 1] are consecutive long double
 *	floating-point numbers.  Verify nextafterl and nexttowardl
 *	follow exactly this sequence, forward and back, and in
 *	negative.
 */
static void
checkl(const long double *x, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		CHECK(i, nextafterl, x[i], x[i], x[i]);
		CHECK(i, nexttowardl, x[i], x[i], x[i]);
		CHECK(i, nextafterl, -x[i], -x[i], -x[i]);
		CHECK(i, nexttowardl, -x[i], -x[i], -x[i]);
	}

	for (i = 0; i < n - 1; i++) {
		ATF_REQUIRE_MSG(x[i] < x[i + 1], "i=%u", i);

		CHECK(i, nextafterl, x[i], x[i + 1], x[i + 1]);
		CHECK(i, nexttowardl, x[i], x[i + 1], x[i + 1]);
		CHECK(i, nextafterl, x[i], x[n - 1], x[i + 1]);
		CHECK(i, nexttowardl, x[i], x[n - 1], x[i + 1]);
		CHECK(i, nextafterl, x[i], INFINITY, x[i + 1]);
		CHECK(i, nexttowardl, x[i], INFINITY, x[i + 1]);

		CHECK(i, nextafterl, -x[i], -x[i + 1], -x[i + 1]);
		CHECK(i, nexttowardl, -x[i], -x[i + 1], -x[i + 1]);
		CHECK(i, nextafterl, -x[i], -x[n - 1], -x[i + 1]);
		CHECK(i, nexttowardl, -x[i], -x[n - 1], -x[i + 1]);
		CHECK(i, nextafterl, -x[i], -INFINITY, -x[i + 1]);
		CHECK(i, nexttowardl, -x[i], -INFINITY, -x[i + 1]);
	}

	for (i = n; i --> 1;) {
		ATF_REQUIRE_MSG(x[i - 1] < x[i], "i=%u", i);

		CHECK(i, nextafterl, x[i], x[i - 1], x[i - 1]);
		CHECK(i, nexttowardl, x[i], x[i - 1], x[i - 1]);
		CHECK(i, nextafterl, x[i], x[0], x[i - 1]);
		CHECK(i, nexttowardl, x[i], x[0], x[i - 1]);
		CHECK(i, nextafterl, x[i], +0., x[i - 1]);
		CHECK(i, nexttowardl, x[i], +0., x[i - 1]);
		CHECK(i, nextafterl, x[i], -0., x[i - 1]);
		CHECK(i, nexttowardl, x[i], -0., x[i - 1]);
		CHECK(i, nextafterl, x[i], -x[0], x[i - 1]);
		CHECK(i, nexttowardl, x[i], -x[0], x[i - 1]);
		CHECK(i, nextafterl, x[i], -x[i], x[i - 1]);
		CHECK(i, nexttowardl, x[i], -x[i], x[i - 1]);
		CHECK(i, nextafterl, x[i], -INFINITY, x[i - 1]);
		CHECK(i, nexttowardl, x[i], -INFINITY, x[i - 1]);

		CHECK(i, nextafterl, -x[i], -x[i - 1], -x[i - 1]);
		CHECK(i, nexttowardl, -x[i], -x[i - 1], -x[i - 1]);
		CHECK(i, nextafterl, -x[i], -x[0], -x[i - 1]);
		CHECK(i, nexttowardl, -x[i], -x[0], -x[i - 1]);
		CHECK(i, nextafterl, -x[i], -0., -x[i - 1]);
		CHECK(i, nexttowardl, -x[i], -0., -x[i - 1]);
		CHECK(i, nextafterl, -x[i], +0., -x[i - 1]);
		CHECK(i, nexttowardl, -x[i], +0., -x[i - 1]);
		CHECK(i, nextafterl, -x[i], x[0], -x[i - 1]);
		CHECK(i, nexttowardl, -x[i], x[0], -x[i - 1]);
		CHECK(i, nextafterl, -x[i], INFINITY, -x[i - 1]);
		CHECK(i, nexttowardl, -x[i], INFINITY, -x[i - 1]);
	}
}

ATF_TC(next_nan);
ATF_TC_HEAD(next_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter/nexttoward on NaN");
}
ATF_TC_BODY(next_nan, tc)
{
#ifdef NAN
	/* XXX verify the NaN is quiet */
	ATF_CHECK(isnan(nextafter(NAN, 0)));
	ATF_CHECK(isnan(nexttoward(NAN, 0)));
	ATF_CHECK(isnan(nextafter(0, NAN)));
	ATF_CHECK(isnan(nexttoward(0, NAN)));
#else
	atf_tc_skip("no NaNs on this architecture");
#endif
}

ATF_TC(next_signed_0);
ATF_TC_HEAD(next_signed_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter/nexttoward on signed 0");
}
ATF_TC_BODY(next_signed_0, tc)
{
	volatile double z_pos = +0.;
	volatile double z_neg = -0.;
#ifdef __DBL_HAS_DENORM__
	volatile double m = __DBL_DENORM_MIN__;
#else
	volatile double m = DBL_MIN;
#endif

	if (signbit(z_pos) == signbit(z_neg))
		atf_tc_skip("no signed zeroes on this architecture");

	/*
	 * `nextUp(x) is the least floating-point number in the format
	 *  of x that compares greater than x. [...] nextDown(x) is
	 *  -nextUp(-x).'
	 * --IEEE 754-2019, 5.3.1 General operations, p. 19
	 *
	 * Verify that nextafter and nexttoward, which implement the
	 * nextUp and nextDown operations, obey this rule and don't
	 * send -0 to +0 or +0 to -0, respectively.
	 */

	CHECK(0, nextafter, z_neg, +INFINITY, m);
	CHECK(1, nexttoward, z_neg, +INFINITY, m);
	CHECK(2, nextafter, z_pos, +INFINITY, m);
	CHECK(3, nexttoward, z_pos, +INFINITY, m);

	CHECK(4, nextafter, z_pos, -INFINITY, -m);
	CHECK(5, nexttoward, z_pos, -INFINITY, -m);
	CHECK(6, nextafter, z_neg, -INFINITY, -m);
	CHECK(7, nexttoward, z_neg, -INFINITY, -m);

	/*
	 * `If x is the negative number of least magnitude in x's
	 *  format, nextUp(x) is -0.'
	 * --IEEE 754-2019, 5.3.1 General operations, p. 19
	 *
	 * Verify that nextafter and nexttoward return the correctly
	 * signed zero.
	 */
	CHECK(8, nextafter, -m, +INFINITY, 0);
	CHECK(9, nexttoward, -m, +INFINITY, 0);
	ATF_CHECK(signbit(nextafter(-m, +INFINITY)) != 0);
	CHECK(10, nextafter, m, -INFINITY, 0);
	CHECK(11, nexttoward, m, -INFINITY, 0);
	ATF_CHECK(signbit(nextafter(m, -INFINITY)) == 0);
}

ATF_TC(next_near_0);
ATF_TC_HEAD(next_near_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter/nexttoward near 0");
}
ATF_TC_BODY(next_near_0, tc)
{
	static const double x[] = {
		[0] = 0,
#ifdef __DBL_HAS_DENORM__
		[1] = __DBL_DENORM_MIN__,
		[2] = 2*__DBL_DENORM_MIN__,
		[3] = 3*__DBL_DENORM_MIN__,
		[4] = 4*__DBL_DENORM_MIN__,
#else
		[1] = DBL_MIN,
		[2] = DBL_MIN*(1 + DBL_EPSILON),
		[3] = DBL_MIN*(1 + 2*DBL_EPSILON),
		[4] = DBL_MIN*(1 + 3*DBL_EPSILON),
#endif
	};

	check(x, __arraycount(x));
}

ATF_TC(next_near_sub_normal);
ATF_TC_HEAD(next_near_sub_normal, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "nextafter/nexttoward near the subnormal/normal boundary");
}
ATF_TC_BODY(next_near_sub_normal, tc)
{
#ifdef __DBL_HAS_DENORM__
	static const double x[] = {
		[0] = DBL_MIN - 3*__DBL_DENORM_MIN__,
		[1] = DBL_MIN - 2*__DBL_DENORM_MIN__,
		[2] = DBL_MIN - __DBL_DENORM_MIN__,
		[3] = DBL_MIN,
		[4] = DBL_MIN + __DBL_DENORM_MIN__,
		[5] = DBL_MIN + 2*__DBL_DENORM_MIN__,
		[6] = DBL_MIN + 3*__DBL_DENORM_MIN__,
	};

	check(x, __arraycount(x));
#else  /* !__DBL_HAS_DENORM__ */
	atf_tc_skip("no subnormals on this architecture");
#endif	/* !__DBL_HAS_DENORM__ */
}

ATF_TC(next_near_1);
ATF_TC_HEAD(next_near_1, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter/nexttoward near 1");
}
ATF_TC_BODY(next_near_1, tc)
{
	static const double x[] = {
		[0] = 1 - 3*DBL_EPSILON/2,
		[1] = 1 - 2*DBL_EPSILON/2,
		[2] = 1 - DBL_EPSILON/2,
		[3] = 1,
		[4] = 1 + DBL_EPSILON,
		[5] = 1 + 2*DBL_EPSILON,
		[6] = 1 + 3*DBL_EPSILON,
	};

	check(x, __arraycount(x));
}

ATF_TC(next_near_1_5);
ATF_TC_HEAD(next_near_1_5, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter/nexttoward near 1.5");
}
ATF_TC_BODY(next_near_1_5, tc)
{
	static const double x[] = {
		[0] = 1.5 - 3*DBL_EPSILON,
		[1] = 1.5 - 2*DBL_EPSILON,
		[2] = 1.5 - DBL_EPSILON,
		[3] = 1.5,
		[4] = 1.5 + DBL_EPSILON,
		[5] = 1.5 + 2*DBL_EPSILON,
		[6] = 1.5 + 3*DBL_EPSILON,
	};

	check(x, __arraycount(x));
}

ATF_TC(next_near_infinity);
ATF_TC_HEAD(next_near_infinity, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter/nexttoward near infinity");
}
ATF_TC_BODY(next_near_infinity, tc)
{
	static const double x[] = {
		[0] = DBL_MAX,
		[1] = INFINITY,
	};
	volatile double t;

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	check(x, __arraycount(x));

	ATF_CHECK_EQ_MSG((t = nextafter(INFINITY, INFINITY)), INFINITY,
	    "t=%a=%g", t, t);
	ATF_CHECK_EQ_MSG((t = nextafter(-INFINITY, -INFINITY)), -INFINITY,
	    "t=%a=%g", t, t);
}

ATF_TC(nextf_nan);
ATF_TC_HEAD(nextf_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf/nexttowardf on NaN");
}
ATF_TC_BODY(nextf_nan, tc)
{
#ifdef NAN
	/* XXX verify the NaN is quiet */
	ATF_CHECK(isnan(nextafterf(NAN, 0)));
	ATF_CHECK(isnan(nexttowardf(NAN, 0)));
	ATF_CHECK(isnan(nextafterf(0, NAN)));
	ATF_CHECK(isnan(nexttowardf(0, NAN)));
#else
	atf_tc_skip("no NaNs on this architecture");
#endif
}

ATF_TC(nextf_signed_0);
ATF_TC_HEAD(nextf_signed_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf/nexttowardf on signed 0");
}
ATF_TC_BODY(nextf_signed_0, tc)
{
	volatile float z_pos = +0.;
	volatile float z_neg = -0.;
#ifdef __FLT_HAS_DENORM__
	volatile float m = __FLT_DENORM_MIN__;
#else
	volatile float m = FLT_MIN;
#endif

	if (signbit(z_pos) == signbit(z_neg))
		atf_tc_skip("no signed zeroes on this architecture");

	/*
	 * `nextUp(x) is the least floating-point number in the format
	 *  of x that compares greater than x. [...] nextDown(x) is
	 *  -nextUp(-x).'
	 * --IEEE 754-2019, 5.3.1 General operations, p. 19
	 *
	 * Verify that nextafterf and nexttowardf, which implement the
	 * nextUp and nextDown operations, obey this rule and don't
	 * send -0 to +0 or +0 to -0, respectively.
	 */

	CHECK(0, nextafterf, z_neg, +INFINITY, m);
	CHECK(1, nexttowardf, z_neg, +INFINITY, m);
	CHECK(2, nextafterf, z_pos, +INFINITY, m);
	CHECK(3, nexttowardf, z_pos, +INFINITY, m);

	CHECK(4, nextafterf, z_pos, -INFINITY, -m);
	CHECK(5, nexttowardf, z_pos, -INFINITY, -m);
	CHECK(6, nextafterf, z_neg, -INFINITY, -m);
	CHECK(7, nexttowardf, z_neg, -INFINITY, -m);

	/*
	 * `If x is the negative number of least magnitude in x's
	 *  format, nextUp(x) is -0.'
	 * --IEEE 754-2019, 5.3.1 General operations, p. 19
	 */
	CHECK(8, nextafterf, -m, +INFINITY, 0);
	CHECK(9, nexttowardf, -m, +INFINITY, 0);
	ATF_CHECK(signbit(nextafterf(-m, +INFINITY)) != 0);
	CHECK(10, nextafterf, m, -INFINITY, 0);
	CHECK(11, nexttowardf, m, -INFINITY, 0);
	ATF_CHECK(signbit(nextafterf(m, -INFINITY)) == 0);
}

ATF_TC(nextf_near_0);
ATF_TC_HEAD(nextf_near_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf/nexttowardf near 0");
}
ATF_TC_BODY(nextf_near_0, tc)
{
	static const float x[] = {
		[0] = 0,
#ifdef __FLT_HAS_DENORM__
		[1] = __FLT_DENORM_MIN__,
		[2] = 2*__FLT_DENORM_MIN__,
		[3] = 3*__FLT_DENORM_MIN__,
		[4] = 4*__FLT_DENORM_MIN__,
#else
		[1] = FLT_MIN,
		[2] = FLT_MIN*(1 + FLT_EPSILON),
		[3] = FLT_MIN*(1 + 2*FLT_EPSILON),
		[4] = FLT_MIN*(1 + 3*FLT_EPSILON),
#endif
	};

	checkf(x, __arraycount(x));
}

ATF_TC(nextf_near_sub_normal);
ATF_TC_HEAD(nextf_near_sub_normal, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "nextafterf/nexttowardf near the subnormal/normal boundary");
}
ATF_TC_BODY(nextf_near_sub_normal, tc)
{
#ifdef __FLT_HAS_DENORM__
	static const float x[] = {
		[0] = FLT_MIN - 3*__FLT_DENORM_MIN__,
		[1] = FLT_MIN - 2*__FLT_DENORM_MIN__,
		[2] = FLT_MIN - __FLT_DENORM_MIN__,
		[3] = FLT_MIN,
		[4] = FLT_MIN + __FLT_DENORM_MIN__,
		[5] = FLT_MIN + 2*__FLT_DENORM_MIN__,
		[6] = FLT_MIN + 3*__FLT_DENORM_MIN__,
	};

	checkf(x, __arraycount(x));
#else  /* !__FLT_HAS_DENORM__ */
	atf_tc_skip("no subnormals on this architecture");
#endif	/* !__FLT_HAS_DENORM__ */
}

ATF_TC(nextf_near_1);
ATF_TC_HEAD(nextf_near_1, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf/nexttowardf near 1");
}
ATF_TC_BODY(nextf_near_1, tc)
{
	static const float x[] = {
		[0] = 1 - 3*FLT_EPSILON/2,
		[1] = 1 - 2*FLT_EPSILON/2,
		[2] = 1 - FLT_EPSILON/2,
		[3] = 1,
		[4] = 1 + FLT_EPSILON,
		[5] = 1 + 2*FLT_EPSILON,
		[6] = 1 + 3*FLT_EPSILON,
	};

	checkf(x, __arraycount(x));
}

ATF_TC(nextf_near_1_5);
ATF_TC_HEAD(nextf_near_1_5, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf/nexttowardf near 1.5");
}
ATF_TC_BODY(nextf_near_1_5, tc)
{
	static const float x[] = {
		[0] = 1.5 - 3*FLT_EPSILON,
		[1] = 1.5 - 2*FLT_EPSILON,
		[2] = 1.5 - FLT_EPSILON,
		[3] = 1.5,
		[4] = 1.5 + FLT_EPSILON,
		[5] = 1.5 + 2*FLT_EPSILON,
		[6] = 1.5 + 3*FLT_EPSILON,
	};

	checkf(x, __arraycount(x));
}

ATF_TC(nextf_near_infinity);
ATF_TC_HEAD(nextf_near_infinity, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf/nexttowardf near infinity");
}
ATF_TC_BODY(nextf_near_infinity, tc)
{
	static const float x[] = {
		[0] = FLT_MAX,
		[1] = INFINITY,
	};
	volatile float t;

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	checkf(x, __arraycount(x));

	ATF_CHECK_EQ_MSG((t = nextafterf(INFINITY, INFINITY)), INFINITY,
	    "t=%a=%g", t, t);
	ATF_CHECK_EQ_MSG((t = nextafterf(-INFINITY, -INFINITY)), -INFINITY,
	    "t=%a=%g", t, t);
}

ATF_TC(nextl_nan);
ATF_TC_HEAD(nextl_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl/nexttowardl on NaN");
}
ATF_TC_BODY(nextl_nan, tc)
{
#ifdef NAN
	/* XXX verify the NaN is quiet */
	ATF_CHECK(isnan(nextafterl(NAN, 0)));
	ATF_CHECK(isnan(nexttowardl(NAN, 0)));
	ATF_CHECK(isnan(nextafterl(0, NAN)));
	ATF_CHECK(isnan(nexttowardl(0, NAN)));
#else
	atf_tc_skip("no NaNs on this architecture");
#endif
}

ATF_TC(nextl_signed_0);
ATF_TC_HEAD(nextl_signed_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl/nexttowardl on signed 0");
}
ATF_TC_BODY(nextl_signed_0, tc)
{
	volatile long double z_pos = +0.;
	volatile long double z_neg = -0.;
#ifdef __LDBL_HAS_DENORM__
	volatile long double m = __LDBL_DENORM_MIN__;
#else
	volatile long double m = LDBL_MIN;
#endif

	if (signbit(z_pos) == signbit(z_neg))
		atf_tc_skip("no signed zeroes on this architecture");

	/*
	 * `nextUp(x) is the least floating-point number in the format
	 *  of x that compares greater than x. [...] nextDown(x) is
	 *  -nextUp(-x).'
	 * --IEEE 754-2019, 5.3.1 General operations, p. 19
	 *
	 * Verify that nextafterl and nexttowardl, which implement the
	 * nextUp and nextDown operations, obey this rule and don't
	 * send -0 to +0 or +0 to -0, respectively.
	 */

	CHECK(0, nextafterl, z_neg, +INFINITY, m);
	CHECK(1, nexttowardl, z_neg, +INFINITY, m);
	CHECK(2, nextafterl, z_pos, +INFINITY, m);
	CHECK(3, nexttowardl, z_pos, +INFINITY, m);

	CHECK(4, nextafterl, z_pos, -INFINITY, -m);
	CHECK(5, nexttowardl, z_pos, -INFINITY, -m);
	CHECK(6, nextafterl, z_neg, -INFINITY, -m);
	CHECK(7, nexttowardl, z_neg, -INFINITY, -m);

	/*
	 * `If x is the negative number of least magnitude in x's
	 *  format, nextUp(x) is -0.'
	 * --IEEE 754-2019, 5.3.1 General operations, p. 19
	 */
	CHECK(8, nextafterl, -m, +INFINITY, 0);
	CHECK(9, nexttowardl, -m, +INFINITY, 0);
	ATF_CHECK(signbit(nextafterl(-m, +INFINITY)) != 0);
	CHECK(10, nextafterl, m, -INFINITY, 0);
	CHECK(11, nexttowardl, m, -INFINITY, 0);
	ATF_CHECK(signbit(nextafterl(m, -INFINITY)) == 0);
}

ATF_TC(nextl_near_0);
ATF_TC_HEAD(nextl_near_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl/nexttowardl near 0");
}
ATF_TC_BODY(nextl_near_0, tc)
{
	static const long double x[] = {
		[0] = 0,
#ifdef __LDBL_HAS_DENORM__
		[1] = __LDBL_DENORM_MIN__,
		[2] = 2*__LDBL_DENORM_MIN__,
		[3] = 3*__LDBL_DENORM_MIN__,
		[4] = 4*__LDBL_DENORM_MIN__,
#else
		[1] = LDBL_MIN,
		[2] = LDBL_MIN*(1 + LDBL_EPSILON),
		[3] = LDBL_MIN*(1 + 2*LDBL_EPSILON),
		[4] = LDBL_MIN*(1 + 3*LDBL_EPSILON),
#endif
	};

	checkl(x, __arraycount(x));
}

ATF_TC(nextl_near_sub_normal);
ATF_TC_HEAD(nextl_near_sub_normal, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "nextafterl/nexttowardl near the subnormal/normal boundary");
}
ATF_TC_BODY(nextl_near_sub_normal, tc)
{
#ifdef __LDBL_HAS_DENORM__
	static const long double x[] = {
		[0] = LDBL_MIN - 3*__LDBL_DENORM_MIN__,
		[1] = LDBL_MIN - 2*__LDBL_DENORM_MIN__,
		[2] = LDBL_MIN - __LDBL_DENORM_MIN__,
		[3] = LDBL_MIN,
		[4] = LDBL_MIN + __LDBL_DENORM_MIN__,
		[5] = LDBL_MIN + 2*__LDBL_DENORM_MIN__,
		[6] = LDBL_MIN + 3*__LDBL_DENORM_MIN__,
	};

	checkl(x, __arraycount(x));
#else  /* !__LDBL_HAS_DENORM__ */
	atf_tc_skip("no subnormals on this architecture");
#endif	/* !__LDBL_HAS_DENORM__ */
}

ATF_TC(nextl_near_1);
ATF_TC_HEAD(nextl_near_1, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl/nexttowardl near 1");
}
ATF_TC_BODY(nextl_near_1, tc)
{
	static const long double x[] = {
		[0] = 1 - 3*LDBL_EPSILON/2,
		[1] = 1 - 2*LDBL_EPSILON/2,
		[2] = 1 - LDBL_EPSILON/2,
		[3] = 1,
		[4] = 1 + LDBL_EPSILON,
		[5] = 1 + 2*LDBL_EPSILON,
		[6] = 1 + 3*LDBL_EPSILON,
	};

	checkl(x, __arraycount(x));
}

ATF_TC(nextl_near_1_5);
ATF_TC_HEAD(nextl_near_1_5, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl/nexttowardl near 1.5");
}
ATF_TC_BODY(nextl_near_1_5, tc)
{
	static const long double x[] = {
		[0] = 1.5 - 3*LDBL_EPSILON,
		[1] = 1.5 - 2*LDBL_EPSILON,
		[2] = 1.5 - LDBL_EPSILON,
		[3] = 1.5,
		[4] = 1.5 + LDBL_EPSILON,
		[5] = 1.5 + 2*LDBL_EPSILON,
		[6] = 1.5 + 3*LDBL_EPSILON,
	};

	checkl(x, __arraycount(x));
}

ATF_TC(nextl_near_infinity);
ATF_TC_HEAD(nextl_near_infinity, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl/nexttowardl near infinity");
}
ATF_TC_BODY(nextl_near_infinity, tc)
{
	static const long double x[] = {
		[0] = LDBL_MAX,
		[1] = INFINITY,
	};
	volatile long double t;

	if (!isinf(INFINITY))
		atf_tc_skip("no infinities on this architecture");

	checkl(x, __arraycount(x));

	ATF_CHECK_EQ_MSG((t = nextafterl(INFINITY, INFINITY)), INFINITY,
	    "t=%La=%Lg", t, t);
	ATF_CHECK_EQ_MSG((t = nextafterl(-INFINITY, -INFINITY)), -INFINITY,
	    "t=%La=%Lg", t, t);
}

#endif	/* __vax__ */

ATF_TP_ADD_TCS(tp)
{

#ifdef __vax__
	ATF_TP_ADD_TC(tp, vaxafter);
#else
	ATF_TP_ADD_TC(tp, next_nan);
	ATF_TP_ADD_TC(tp, next_near_0);
	ATF_TP_ADD_TC(tp, next_near_1);
	ATF_TP_ADD_TC(tp, next_near_1_5);
	ATF_TP_ADD_TC(tp, next_near_infinity);
	ATF_TP_ADD_TC(tp, next_near_sub_normal);
	ATF_TP_ADD_TC(tp, next_signed_0);
	ATF_TP_ADD_TC(tp, nextf_nan);
	ATF_TP_ADD_TC(tp, nextf_near_0);
	ATF_TP_ADD_TC(tp, nextf_near_1);
	ATF_TP_ADD_TC(tp, nextf_near_1_5);
	ATF_TP_ADD_TC(tp, nextf_near_infinity);
	ATF_TP_ADD_TC(tp, nextf_near_sub_normal);
	ATF_TP_ADD_TC(tp, nextf_signed_0);
	ATF_TP_ADD_TC(tp, nextl_nan);
	ATF_TP_ADD_TC(tp, nextl_near_0);
	ATF_TP_ADD_TC(tp, nextl_near_1);
	ATF_TP_ADD_TC(tp, nextl_near_1_5);
	ATF_TP_ADD_TC(tp, nextl_near_infinity);
	ATF_TP_ADD_TC(tp, nextl_near_sub_normal);
	ATF_TP_ADD_TC(tp, nextl_signed_0);
#endif
	return atf_no_error();
}
