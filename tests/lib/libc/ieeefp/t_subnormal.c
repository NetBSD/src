/* $NetBSD: t_subnormal.c,v 1.1 2011/01/02 03:51:21 pgoyette Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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
#include <atf-c/config.h>

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

ATF_TC(test_float);

ATF_TC_HEAD(test_float, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test float operations");
}

ATF_TC_BODY(test_float, tc)
{
	float d0, d1, d2, f, ip;
	int e, i;

	d0 = FLT_MIN;
	ATF_REQUIRE_EQ(fpclassify(d0), FP_NORMAL);
	f = frexpf(d0, &e);
	ATF_REQUIRE_EQ(e, FLT_MIN_EXP);
	ATF_REQUIRE_EQ(f, 0.5);
	d1 = d0;

	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (i = 1; i < FLT_MANT_DIG; i++) {
		d1 /= 2;
		ATF_REQUIRE_EQ(fpclassify(d1), FP_SUBNORMAL);
		ATF_REQUIRE(d1 > 0 && d1 < d0);

		d2 = ldexpf(d0, -i);
		ATF_REQUIRE_EQ(d2, d1);

		d2 = modff(d1, &ip);
		ATF_REQUIRE_EQ(d2, d1);
		ATF_REQUIRE_EQ(ip, 0);

		f = frexpf(d1, &e);
		ATF_REQUIRE_EQ(e, FLT_MIN_EXP - i);
		ATF_REQUIRE_EQ(f, 0.5);
	}

	d1 /= 2;
	ATF_REQUIRE_EQ(fpclassify(d1), FP_ZERO);
	f = frexpf(d1, &e);
	ATF_REQUIRE_EQ(e, 0);
	ATF_REQUIRE_EQ(f, 0);
}

ATF_TC(test_double);

ATF_TC_HEAD(test_double, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test double operations");
}

ATF_TC_BODY(test_double, tc)
{
	double d0, d1, d2, f, ip;
	int e, i;

	d0 = DBL_MIN;
	ATF_REQUIRE_EQ(fpclassify(d0), FP_NORMAL);
	f = frexp(d0, &e);
	ATF_REQUIRE_EQ(e, DBL_MIN_EXP);
	ATF_REQUIRE_EQ(f, 0.5);
	d1 = d0;

	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (i = 1; i < DBL_MANT_DIG; i++) {
		d1 /= 2;
		ATF_REQUIRE_EQ(fpclassify(d1), FP_SUBNORMAL);
		ATF_REQUIRE(d1 > 0 && d1 < d0);

		d2 = ldexp(d0, -i);
		ATF_REQUIRE_EQ(d2, d1);

		d2 = modf(d1, &ip);
		ATF_REQUIRE_EQ(d2, d1);
		ATF_REQUIRE_EQ(ip, 0);

		f = frexp(d1, &e);
		ATF_REQUIRE_EQ(e, DBL_MIN_EXP - i);
		ATF_REQUIRE_EQ(f, 0.5);
	}

	d1 /= 2;
	ATF_REQUIRE_EQ(fpclassify(d1), FP_ZERO);
	f = frexp(d1, &e);
	ATF_REQUIRE_EQ(e, 0);
	ATF_REQUIRE_EQ(f, 0);
}

/*
 * XXX NetBSD doesn't have long-double flavors of frexp, ldexp, and modf,
 * XXX so this test is disabled.
 */

#ifdef TEST_LONG_DOUBLE

ATF_TC(test_long_double);

ATF_TC_HEAD(test_long_double, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test long_double operations");
}

ATF_TC_BODY(test_long_double, tc)
{
	long double d0, d1, d2, f, ip;
	int e, i;

	d0 = LDBL_MIN;
	ATF_REQUIRE_EQ(fpclassify(d0), FP_NORMAL);
	f = frexpl(d0, &e);
	ATF_REQUIRE_EQ(e, LDBL_MIN_EXP);
	ATF_REQUIRE_EQ(f, 0.5);
	d1 = d0;

	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (i = 1; i < LDBL_MANT_DIG; i++) {
		d1 /= 2;
		ATF_REQUIRE_EQ(fpclassify(d1), FP_SUBNORMAL);
		ATF_REQUIRE(d1 > 0 && d1 < d0);

		d2 = ldexpl(d0, -i);
		ATF_REQUIRE_EQ(d2, d1);

		d2 = modfl(d1, &ip);
		ATF_REQUIRE_EQ(d2, d1);
		ATF_REQUIRE_EQ(ip, 0);

		f = frexpl(d1, &e);
		ATF_REQUIRE_EQ(e, LDBL_MIN_EXP - i);
		ATF_REQUIRE_EQ(f, 0.5);
	}

	d1 /= 2;
	ATF_REQUIRE_EQ(fpclassify(d1), FP_ZERO);
	f = frexpl(d1, &e);
	ATF_REQUIRE_EQ(e, 0);
	ATF_REQUIRE_EQ(f, 0);
}
#endif /* TEST_LONG_DOUBLE */

ATF_TP_ADD_TCS(tp)
{
	const char *arch;

	arch = atf_config_get("atf_arch");
	if (strcmp("vax", arch) == 0 || strcmp("m68000", arch) == 0)
		printf("Test not applicable on %s\n", arch);
	else {
		ATF_TP_ADD_TC(tp, test_float);
		ATF_TP_ADD_TC(tp, test_double);
#ifdef TEST_LONG_DOUBLE
		ATF_TP_ADD_TC(tp, test_long_double);
#endif /* TEST_LONG_DOUBLE */
	}

	return atf_no_error();
}
