/* $NetBSD: t_sprintf.c,v 1.9 2026/10/28 17:35:19 kre Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad Schroder.
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
__COPYRIGHT("@(#) Copyright (c) 2017\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_sprintf.c,v 1.9 2026/10/28 17:35:19 kre Exp $");

#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>

#include <atf-c.h>

static struct test {
	const char *locale;
	const int int_value;
	const char *int_result;
	const char *int_input;
	const unsigned long int ulong_value;
	const char *ulong_result;
	const char *ulong_input;
	const intmax_t max_value;
	const char *max_result;
	const char *max_input;
	const uintmax_t umax_value;
	const char *umax_result;
	const char *umax_input;
	const double double_value;
	const char *double_result;
	const char *double_input;
} tests[] = {
	{
		"en_US.UTF-8",
		-12345,
		"-12,345",
		"-12345",
		4294967197,
		"4,294,967,197",
		"4294967197",
		12345678901234,
		"+12,345,678,901,234",
		" 12345678901234",
		18446744073709551493U,
		"000018,446,744,073,709,551,493",
		"18446744073709551493",
		-12345.6789,
		"-12,345.678900",
		"-12345.67890",
	}, {
		"fr_FR.ISO8859-1",
		-12345,
		"-12\240345",
		"-12345",
		4294967197,
		"4\240294\240967\240197",
		"4294967197",
		12345678901234,
		"+12\240345\240678\240901\240234",
		" 12345678901234",
		18446744073709551493U,
		"000018\240446\240744\240073\240709\240551\240493",
		" 18446744073709551493",
		-12345.6789,
		"-12\240345,678900",
		"-12345,6789",
	}, {
		"it_IT.ISO8859-1",
		-54321,
		"-54.321",
		"-54321",
		4294967197,
		"4.294.967.197",
		"4294967197",
		12345678901234,
		"+12.345.678.901.234",
		" 12345678901234",
		18446744073709551493U,
		"000018.446.744.073.709.551.493",
		"+18446744073709551493",
		-98765.1234,
		"-98.765,123400",
		"-98765,12340",
	}, {
		"nb_NO.UTF-8",
		-54321,
		"-54\302\240321",
		"-54321",
		4294967197,
		"4\302\240294\302\240967\302\240197",
		"4294967197",
		12345678901234,
		"+12\302\240345\302\240678\302\240901\302\240234",
		" 12345678901234",
		18446744073709551493U,
		"18\302\240446\302\240744\302\240073\302\240709"
			"\302\240551\302\240493",
		"+18446744073709551493",
		-98765.1234,
		"-98\302\240765,123400",
		"-98765,12340",
	}, {
		"x1_X1.UTF-8",
		-1234567,
		"-1\331\2542345\331\25467",
		"-1234567",
		4294967197,
		"4\331\254294\331\2549671\331\25497",
		"4294967197",
		912345678901234,
		"+91234\331\2545\331\254678\331\2549012\331\25434",
		"912345678901234",
		18446744073709551493U,
		"001844674407\331\2543\331\254709\331\2545514\331\25493",
		" +18446744073709551493",
		-12345.6789,
		"-123\331\25445\342\227\216678900",
		"-12345\342\227\216678900",
	}, {
		"x1_X2.UTF-8",
		-12345,
		"-12,34,5",
		"-12345",
		4294967197,	/* between LONG_MAX and ULONG_MAX */
		"4294,967,19,7",
		"4294967197",
		-1234567890123456,
		"-1,23,45,6,7890,123,45,6",
		" -1234567890123456",
		18446744073709551493U, /* between INTMAX_MAX and UINTMAX_MAX */
		"01,84,46,74,40,7,3709,551,49,3",
		"18446744073709551493",
		-12312345.6789,
		"-12,312,34,5\340\274\232678900",
		"-12312345\340\274\23267890",
	}, {
		"x1_X3.UTF-8",
		-123456,
		"-123\342\270\26145\342\270\2616",
		"-123456",
		4294967197,	/* between LONG_MAX and ULONG_MAX */
		"4\342\270\261294\342\270\261967\342\270\26119\342\270\2617",
		"4294967197",
		-1234567890123456,
		"-1\342\270\261234\342\270\261567\342\270\261890"
			"\342\270\261123\342\270\26145\342\270\2616",
		" -1234567890123456",
		18446744073709551493U, /* between INTMAX_MAX and UINTMAX_MAX */
		"18\342\270\261446\342\270\261744\342\270\261073\342\270\261"
			"709\342\270\261551\342\270\26149\342\270\2613",
		"18446744073709551493",
		-12312345.6789,
		"-12\342\270\261312\342\270\26134\342\270\2615"
			"\342\270\263678900",
		"-12312345\342\270\26367890",
	}, {
		"POSIX",
		/*
		 * POSIX-1.2008 specifies that the C and POSIX
		 * locales shall be identical (section 7.2) and
		 * that the POSIX locale shall have an empty
		 * thousands separator and "<period>" as its
		 * decimal point (section 7.3.4).  *printf
		 * ought to honor these settings.
		 */
		-12345,
		"-12345",
		"-12345",
		4294967197,
		"4294967197",
		"4294967197",
		12345678901234,
		"+12345678901234",
		"12345678901234",
		18446744073709551493U,
		"000000000018446744073709551493",
		"18446744073709551493",
		-12345.6789,
		"-12345.678900",
		"-12345.6789",
	}, {
		NULL,
		0,
		NULL,
		NULL,
		0,
		NULL,
		NULL,
		0,
		NULL,
		NULL,
		0,
		NULL,
		NULL,
		0.0,
		NULL,
		NULL,
	}
};

static void
h_sprintf(const struct test *t)
{
	char buf[1024];

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_NUMERIC, t->locale) != NULL);
	printf("Using locale: %s\n", setlocale(LC_ALL, NULL));

	sprintf(buf, "%'f", t->double_value);
	ATF_CHECK_STREQ_MSG(buf, t->double_result,
	    "In %s: sprintf(\"%%'f\", %g) -> \"%s\", expected \"%s\"",
		t->locale, t->double_value, buf, t->double_result);

	sprintf(buf, "%'d", t->int_value);
	ATF_CHECK_STREQ_MSG(buf, t->int_result,
	    "In %s: sprintf(buf, \"%%'d\", %d) -> \"%s\", expected \"%s\"",
		t->locale, t->int_value, buf, t->int_result); 

	sprintf(buf, "%'lu", t->ulong_value);
	ATF_CHECK_STREQ_MSG(buf, t->ulong_result,
	    "In %s: sprintf(buf, \"%%'lu\", %lu) -> \"%s\", expected \"%s\"",
		t->locale, t->ulong_value, buf, t->ulong_result);

	sprintf(buf, "%'+jd", t->max_value);
	ATF_CHECK_STREQ_MSG(buf, t->max_result,
	    "In %s: sprintf(buf, \"%%'+jd\", %jd) -> \"%s\", expected \"%s\"",
		t->locale, t->max_value, buf, t->max_result);

	sprintf(buf, "%'030ju", t->umax_value);
	ATF_CHECK_STREQ_MSG(buf, t->umax_result,
	    "In %s: sprintf(buf, \"%%'030ju\", %ju) -> \"%s\", expected \"%s\"",
		t->locale, t->umax_value, buf, t->umax_result);
}

static void
h_strto(const struct test *t)
{
	double d, diff;
	int iv;
	unsigned long ulv;
	intmax_t mxv;
	uintmax_t umxv;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_NUMERIC, t->locale) != NULL);

	ATF_CHECK_EQ_MSG((iv = (int)strtol(t->int_input, NULL, 10)),
	    t->int_value,
	    "In %s: (int)strtol(\"%s\", NULL, 10) -> %d, expected %d",
		t->locale, t->int_input, iv, t->int_value);
	ATF_CHECK_EQ_MSG((ulv = strtoul(t->ulong_input, NULL, 10)),
	    t->ulong_value,
	    "In %s: strtoul(\"%s\", NULL, 10) -> %lu, expected %lu",
		t->locale, t->ulong_input, ulv, t->ulong_value);
	ATF_CHECK_EQ_MSG((mxv = strtoimax(t->max_input, NULL, 10)),
	    t->max_value,
	    "In %s: strtoimax(\"%s\", NULL, 10) -> %jd, expected %jd",
		t->locale, t->max_input, mxv, t->max_value);
	ATF_CHECK_EQ_MSG((umxv = strtoumax(t->umax_input, NULL, 10)),
	    t->umax_value,
	    "In %s: strtoumax(\"%s\", NULL, 10) -> %ju, expected %ju",
		t->locale, t->umax_input, umxv, t->umax_value);

	/*
	 * Note that the C standard permits function values to be
	 * returned with more precision than is expected by (floating)
	 * data types, and on i386 (and potentially other implementations)
	 * that is exactly what happens, meaning that the result from
	 * strtod() is not identical to the expected value - it turns out
	 * that it is the same if the value is constrained to the number
	 * of mantissa bits in a double (so the %a values printed below
	 * show the exact same bit patterns) and on i386 -ffloat-store
	 * will cause gcc to constrain the result that way, but nothing
	 * demands that be true, so instead, we simply test that the
	 * value returned is very very close to that expected.
	 *
	 * 1e-12 is chosen as the allowable delta, as we know (from
	 * the data in the "struct test" earlier in this file) that
	 * its magnitude is ~ 10^5, with values of that magnitude,
	 * 10^-12 difference is a 10^-17 relative difference, and
	 * with a 56 bit mantissa (standard IEEE "double") a difference
	 * that small vanishes (requires at least 57 mantissa bits to
	 * be representable).   If the data values were to change, then
	 * so might this delta (if they were not all the same, we would
	 * move the delta into the struct rather than having it a constant
	 * here.).
	 *
	 * Finally, note that our purpose here is not to test floating
	 * point arithmetic, we're testing locale dependent string to
	 * binary conversions.
	 */

	d = (double)strtod(t->double_input, NULL);
	diff = fabs(d - t->double_value);
	if (diff >= 1e-12)
		ATF_CHECK_EQ_MSG(d, t->double_value, "In %s: " 
		    "d=strtod(t->double_input[%s], NULL)[%.12g = %a] != "
		    "t->double_value[%.12g = %a]: diff=%g", t->locale,
		    t->double_input, d, d, t->double_value, t->double_value,
		    diff);
}

static void
h_sscanf(const struct test *t)
{
	int int_reported;
	unsigned long ulong_reported;
	intmax_t max_reported;
	uintmax_t umax_reported;
	double double_reported, diff;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_NUMERIC, t->locale) != NULL);

	sscanf(t->int_input, "%d", &int_reported);
	ATF_CHECK_EQ_MSG(int_reported, t->int_value,
	  "In %s: int scanned=%d expected=%d", t->locale,
	  int_reported, t->int_value);
	sscanf(t->ulong_input, "%lu", &ulong_reported);
	ATF_CHECK_EQ_MSG(ulong_reported, t->ulong_value,
	  "In %s: ulong scanned=%lu expected=%lu", t->locale,
	  ulong_reported, t->ulong_value);
	sscanf(t->max_input, "%jd", &max_reported);
	ATF_CHECK_EQ_MSG(max_reported, t->max_value,
	  "In %s: intmax scanned=%jd expected=%jd", t->locale,
	  max_reported, t->max_value);
	sscanf(t->umax_input, "%ju", &umax_reported);
	ATF_CHECK_EQ_MSG(umax_reported, t->umax_value,
	  "In %s: umax scanned=%ju expected=%ju", t->locale,
	  umax_reported, t->umax_value);

	sscanf(t->double_input, "%lf", &double_reported);
	/*
	 * Test for approximately equal, the same way that
	 * is done in the strto tests.
	 */
	diff = fabs(double_reported - t->double_value);
	if (diff >= 1e-12)
		ATF_CHECK_EQ_MSG(double_reported, t->double_value, "In %s: " 
		    "sscanf(t->double_input, \"%%lf\", &result) result "
		    "[%.12g = %a] != t->double_value[%.12g = %a]: diff=%g",
		    t->locale, double_reported, double_reported,
		    t->double_value, t->double_value, diff);
}

ATF_TC(sprintf);
ATF_TC_HEAD(sprintf, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks sprintf %%'d and %%'f under different locales");
}
ATF_TC_BODY(sprintf, tc)
{
	struct test *t;

	for (t = &tests[0]; t->locale != NULL; ++t)
		h_sprintf(t);
}

ATF_TC(strto);
ATF_TC_HEAD(strto, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks strtol and strtod under different locales");
}
ATF_TC_BODY(strto, tc)
{
	struct test *t;

	for (t = &tests[0]; t->locale != NULL; ++t)
		h_strto(t);
}

ATF_TC(sscanf);
ATF_TC_HEAD(sscanf, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks sscanf under different locales");
}
ATF_TC_BODY(sscanf, tc)
{
	struct test *t;

	for (t = &tests[0]; t->locale != NULL; ++t)
		h_sscanf(t);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, sprintf);
	ATF_TP_ADD_TC(tp, sscanf);
	ATF_TP_ADD_TC(tp, strto);

	return atf_no_error();
}
