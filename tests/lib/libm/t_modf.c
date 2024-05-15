/*	$NetBSD: t_modf.c,v 1.6 2024/05/15 00:02:57 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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
__RCSID("$NetBSD: t_modf.c,v 1.6 2024/05/15 00:02:57 riastradh Exp $");

#include <atf-c.h>
#include <float.h>
#include <math.h>

__CTASSERT(FLT_RADIX == 2);

static const struct {
	float x, i, f;
} casesf[] = {
	{ 0, 0, 0 },
	{ FLT_MIN, 0, FLT_MIN },
	{ 0.5, 0, 0.5 },
	{ 1 - FLT_EPSILON/2, 0, 1 - FLT_EPSILON/2 },
	{ 1, 1, 0 },
	{ 1 + FLT_EPSILON, 1, FLT_EPSILON },
	{ 0.5/FLT_EPSILON - 0.5, 0.5/FLT_EPSILON - 1, 0.5 },
	{ 0.5/FLT_EPSILON, 0.5/FLT_EPSILON, 0 },
	{ 0.5/FLT_EPSILON + 0.5, 0.5/FLT_EPSILON, 0.5 },
	{ 1/FLT_EPSILON, 1/FLT_EPSILON, 0 },
};

static const struct {
	double x, i, f;
} cases[] = {
	{ 0, 0, 0 },
	{ DBL_MIN, 0, DBL_MIN },
	{ 0.5, 0, 0.5 },
	{ 1 - DBL_EPSILON/2, 0, 1 - DBL_EPSILON/2 },
	{ 1, 1, 0 },
	{ 1 + DBL_EPSILON, 1, DBL_EPSILON },
	{ 1/FLT_EPSILON + 0.5, 1/FLT_EPSILON, 0.5 },
	{ 0.5/DBL_EPSILON - 0.5, 0.5/DBL_EPSILON - 1, 0.5 },
	{ 0.5/DBL_EPSILON, 0.5/DBL_EPSILON, 0 },
	{ 0.5/DBL_EPSILON + 0.5, 0.5/DBL_EPSILON, 0.5 },
	{ 1/DBL_EPSILON, 1/DBL_EPSILON, 0 },
};

#ifdef __HAVE_LONG_DOUBLE
static const struct {
	long double x, i, f;
} casesl[] = {
	{ 0, 0, 0 },
	{ LDBL_MIN, 0, LDBL_MIN },
	{ 0.5, 0, 0.5 },
	{ 1 - LDBL_EPSILON/2, 0, 1 - LDBL_EPSILON/2 },
	{ 1, 1, 0 },
	{ 1 + LDBL_EPSILON, 1, LDBL_EPSILON },
	{ 1.0L/DBL_EPSILON + 0.5L, 1.0L/DBL_EPSILON, 0.5 },
	{ 0.5/LDBL_EPSILON - 0.5L, 0.5/LDBL_EPSILON - 1, 0.5 },
	{ 0.5/LDBL_EPSILON, 0.5/LDBL_EPSILON, 0 },
	{ 0.5/LDBL_EPSILON + 0.5L, 0.5/LDBL_EPSILON, 0.5 },
	{ 1/LDBL_EPSILON, 1/LDBL_EPSILON, 0 },
};
#endif	/* __HAVE_LONG_DOUBLE */

ATF_TC(modff);
ATF_TC_HEAD(modff, tc)
{
	atf_tc_set_md_var(tc, "descr", "modff(3)");
}
ATF_TC_BODY(modff, tc)
{
	unsigned n;

	for (n = 0; n < __arraycount(casesf); n++) {
		float x, i, f;

		x = casesf[n].x;
		f = modff(x, &i);
		ATF_CHECK_EQ_MSG(i, casesf[n].i,
		    "casesf[%u]: modff %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
		ATF_CHECK_EQ_MSG(f, casesf[n].f,
		    "casesf[%u]: modff %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);

		f = modff(-x, &i);
		ATF_CHECK_EQ_MSG(i, -casesf[n].i,
		    "casesf[%u]: modff %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
		ATF_CHECK_EQ_MSG(f, -casesf[n].f,
		    "casesf[%u]: modff %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
	}

	if (isinf(INFINITY)) {
		float x, i, f;

		x = INFINITY;
		f = modff(x, &i);
		ATF_CHECK_MSG(f == 0,
		    "modff +inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
		ATF_CHECK_MSG(isinf(i) && i > 0,
		    "modff +inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);

		x = -INFINITY;
		f = modff(x, &i);
		ATF_CHECK_MSG(f == 0,
		    "modff -inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
		ATF_CHECK_MSG(isinf(i) && i < 0,
		    "modff -inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
	}

#ifdef NAN
	{
		float x, i, f;

		x = NAN;
		f = modff(x, &i);
		ATF_CHECK_MSG(isnan(f),
		    "modff NaN returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
		ATF_CHECK_MSG(isnan(i),
		    "modff NaN returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
	}
#endif	/* NAN */
}

ATF_TC(modf);
ATF_TC_HEAD(modf, tc)
{
	atf_tc_set_md_var(tc, "descr", "modf(3)");
}
ATF_TC_BODY(modf, tc)
{
	unsigned n;

	for (n = 0; n < __arraycount(casesf); n++) {
		double x, i, f;

		x = casesf[n].x;
		f = modf(x, &i);
		ATF_CHECK_EQ_MSG(i, casesf[n].i,
		    "casesf[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
		ATF_CHECK_EQ_MSG(f, casesf[n].f,
		    "casesf[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);

		f = modf(-x, &i);
		ATF_CHECK_EQ_MSG(i, -casesf[n].i,
		    "casesf[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
		ATF_CHECK_EQ_MSG(f, -casesf[n].f,
		    "casesf[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
	}

	for (n = 0; n < __arraycount(cases); n++) {
		double x, i, f;

		x = cases[n].x;
		f = modf(x, &i);
		ATF_CHECK_EQ_MSG(i, cases[n].i,
		    "cases[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);
		ATF_CHECK_EQ_MSG(f, cases[n].f,
		    "cases[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);

		f = modf(-x, &i);
		ATF_CHECK_EQ_MSG(i, -cases[n].i,
		    "cases[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);
		ATF_CHECK_EQ_MSG(f, -cases[n].f,
		    "cases[%u]: modf %g=%a"
		    " returned integer %g=%a, frac %g=%a;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);
	}

	if (isinf(INFINITY)) {
		double x, i, f;

		x = INFINITY;
		f = modf(x, &i);
		ATF_CHECK_MSG(f == 0,
		    "modf +inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
		ATF_CHECK_MSG(isinf(i) && i > 0,
		    "modf +inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);

		x = -INFINITY;
		f = modf(x, &i);
		ATF_CHECK_MSG(f == 0,
		    "modf -inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
		ATF_CHECK_MSG(isinf(i) && i < 0,
		    "modf -inf returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
	}

#ifdef NAN
	{
		double x, i, f;

		x = NAN;
		f = modf(x, &i);
		ATF_CHECK_MSG(isnan(f),
		    "modf NaN returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
		ATF_CHECK_MSG(isnan(i),
		    "modf NaN returned integer %g=%a, frac %g=%a",
		    i, i, f, f);
	}
#endif	/* NAN */
}

ATF_TC(modfl);
ATF_TC_HEAD(modfl, tc)
{
	atf_tc_set_md_var(tc, "descr", "modfl(3)");
}
ATF_TC_BODY(modfl, tc)
{
	unsigned n;

	for (n = 0; n < __arraycount(casesf); n++) {
		long double x, i, f;

		x = casesf[n].x;
		f = modfl(x, &i);
		ATF_CHECK_EQ_MSG(i, casesf[n].i,
		    "casesf[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
		ATF_CHECK_EQ_MSG(f, casesf[n].f,
		    "casesf[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);

		f = modfl(-x, &i);
		ATF_CHECK_EQ_MSG(i, -casesf[n].i,
		    "casesf[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
		ATF_CHECK_EQ_MSG(f, -casesf[n].f,
		    "casesf[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    casesf[n].i, casesf[n].i, casesf[n].f, casesf[n].f);
	}

	for (n = 0; n < __arraycount(cases); n++) {
		long double x, i, f;

		x = cases[n].x;
		f = modfl(x, &i);
		ATF_CHECK_EQ_MSG(i, cases[n].i,
		    "cases[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);
		ATF_CHECK_EQ_MSG(f, cases[n].f,
		    "cases[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);

		f = modfl(-x, &i);
		ATF_CHECK_EQ_MSG(i, -cases[n].i,
		    "cases[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);
		ATF_CHECK_EQ_MSG(f, -cases[n].f,
		    "cases[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %g=%a, frac %g=%a",
		    n, x, x, i, i, f, f,
		    cases[n].i, cases[n].i, cases[n].f, cases[n].f);
	}

#ifdef __HAVE_LONG_DOUBLE
	for (n = 0; n < __arraycount(casesl); n++) {
		long double x, i, f;

		x = casesl[n].x;
		f = modfl(x, &i);
		ATF_CHECK_EQ_MSG(i, casesl[n].i,
		    "casesl[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %Lg=%La, frac %Lg=%La",
		    n, x, x, i, i, f, f,
		    casesl[n].i, casesl[n].i, casesl[n].f, casesl[n].f);
		ATF_CHECK_EQ_MSG(f, casesl[n].f,
		    "casesl[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %Lg=%La, frac %Lg=%La",
		    n, x, x, i, i, f, f,
		    casesl[n].i, casesl[n].i, casesl[n].f, casesl[n].f);

		f = modfl(-x, &i);
		ATF_CHECK_EQ_MSG(i, -casesl[n].i,
		    "casesl[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %Lg=%La, frac %Lg=%La",
		    n, x, x, i, i, f, f,
		    casesl[n].i, casesl[n].i, casesl[n].f, casesl[n].f);
		ATF_CHECK_EQ_MSG(f, -casesl[n].f,
		    "casesl[%u]: modfl %Lg=%La"
		    " returned integer %Lg=%La, frac %Lg=%La;"
		    " expected integer %Lg=%La, frac %Lg=%La",
		    n, x, x, i, i, f, f,
		    casesl[n].i, casesl[n].i, casesl[n].f, casesl[n].f);
	}
#endif	/* __HAVE_LONG_DOUBLE */

	if (isinf(INFINITY)) {
		long double x, i, f;

		x = INFINITY;
		f = modfl(x, &i);
		ATF_CHECK_MSG(f == 0,
		    "modfl +inf returned integer %Lg=%La, frac %Lg=%La",
		    i, i, f, f);
		ATF_CHECK_MSG(isinf(i) && i > 0,
		    "modfl +inf returned integer %Lg=%La, frac %Lg=%La",
		    i, i, f, f);

		x = -INFINITY;
		f = modfl(x, &i);
		ATF_CHECK_MSG(f == 0,
		    "modfl -inf returned integer %Lg=%La, frac %Lg=%La",
		    i, i, f, f);
		ATF_CHECK_MSG(isinf(i) && i < 0,
		    "modfl -inf returned integer %Lg=%La, frac %Lg=%La",
		    i, i, f, f);
	}

#ifdef NAN
	{
		long double x, i, f;

		x = NAN;
		f = modfl(x, &i);
		ATF_CHECK_MSG(isnan(f),
		    "modfl NaN returned integer %Lg=%La, frac %Lg=%La",
		    i, i, f, f);
		ATF_CHECK_MSG(isnan(i),
		    "modfl NaN returned integer %Lg=%La, frac %Lg=%La",
		    i, i, f, f);
	}
#endif	/* NAN */
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, modff);
	ATF_TP_ADD_TC(tp, modf);
	ATF_TP_ADD_TC(tp, modfl);

	return atf_no_error();
}
