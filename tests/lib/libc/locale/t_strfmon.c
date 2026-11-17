/* $NetBSD: t_strfmon.c,v 1.7 2026/11/17 00:03:57 kre Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (C) 2018 Conrad Meyer <cem@FreeBSD.org>
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
__RCSID("$NetBSD: t_strfmon.c,v 1.7 2026/11/17 00:03:57 kre Exp $");

/*
 * Caution: This test simply "knows" how monetary values should be
 * 	    formatted in a number of specific locales.   Should the
 *	    locale data for those locales alter, tests will usually fail.
 */

#include <atf-c.h>
#include <errno.h>
#include <locale.h>
#include <monetary.h>
#include <stdio.h>
#include <string.h>

ATF_TC(strfmon_locale);

ATF_TC_HEAD(strfmon_locale, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks strfmon_l under different locales");
}

ATF_TC_BODY(strfmon_locale, tc)
{
	const struct {
		const char *locale;
		const char *expected;
	} tests[] = {
	    { "de_DE.UTF-8",
	      "[ **1234,57 €] [ **1.234,57 EUR] %"
		  " [-  1.234,57] [(__1.234,57EUR)]",
	    },
	    { "en_GB.UTF-8",
	      "[ £**1234.57] [ GBP**1,234.57] %"
		  " [-  1,234.57] [(GBP__1,234.57)]",
	    },
	    { "x1_X1.UTF-8",
	      "[⊞¤ **1234。5670] [⊞X1Q ***12 34。5670] %"
		  " [⊟   12 34。5670] [( X1Q___12 34。5670)]",
	    },
	}, Ctests[] = {
	    { "C",
	      "[ **1235] [ **1235] % [-  1235] [(__1234.568)]"
	    },
	    { "de_DE.UTF-8",
	      "[ **1234,57 €] [ **1.234,57 EUR] %"
		  " [-  1.234,57] [(__1.234,568EUR)]",
	    },
	    { "en_GB.UTF-8",
	      "[ £**1234.57] [ GBP**1,234.57] %"
		  " [-  1,234.57] [(GBP__1,234.568)]",
	    },
	    { "x1_X1.UTF-8",
	      "[⊞¤ **1234。5678] [⊞X1Q ***12 34。5678] %"
		  " [⊟   12 34。5678] [( X1Q___12 34。568)]",
	    },
	};
	locale_t loc;
	size_t i;
	int res;
	char buf[120];

	/* we need a UTF-8 locale to be set, any locale will do */
	ATF_REQUIRE(setlocale(LC_ALL, "en_US.UTF-8") != NULL);

	for (i = 0; i < __arraycount(tests); ++i) {
		loc = newlocale(LC_MONETARY_MASK, tests[i].locale, 0);
		if (loc == 0) {
			ATF_CHECK_MSG(loc != 0,
			    "Locale %s not found", tests[i].locale);
			continue;
		}
		res = strfmon_l(buf, sizeof(buf), loc,
		    "[%^=*#6n] [%=*#6i] %% [%!+#6n] [%(=_#6i]",
		    1234.567, 1234.567, -1234.567, -1234.567);
		ATF_REQUIRE_MSG(res > 0, "Using %s res=%d errno=%d\n",
			tests[i].locale, res, errno);
		ATF_CHECK_STREQ_MSG(tests[i].expected, buf, "Using %s",
		    tests[i].locale);
		freelocale(loc);
	}

	/*
	 * The C locale cannot handle the above, as it has no sign strings
	 * hence any usage containing a '+' flag is required to fail.
	 * Repeat the tests without including that flag, with the C locale
	 * test included.   Also, the C locale has no mon_frac_digits (it
	 * has nothing at all) so no decimal places are possible unless
	 * there is an explicit (right-)precision in the format.
	 */
	for (i = 0; i < __arraycount(Ctests); ++i) {
		loc = newlocale(LC_MONETARY_MASK, Ctests[i].locale, 0);
		if (loc == 0) {
			ATF_CHECK_MSG(loc != 0,
			    "Locale %s not found", Ctests[i].locale);
			continue;
		}
		res = strfmon_l(buf, sizeof(buf), loc,
		    "[%^=*#6n] [%=*#6i] %% [%!#6n] [%(=_#6.3i]",
		    1234.5678, 1234.5678, -1234.5678, -1234.5678);
		ATF_REQUIRE_MSG(res > 0, "Using %s res=%d errno=%d\n",
			Ctests[i].locale, res, errno);
		ATF_CHECK_STREQ_MSG(Ctests[i].expected, buf, "Using %s",
		    Ctests[i].locale);
		freelocale(loc);
	}

	/* Now test that the C locale does fail with a + flag */
	loc = newlocale(LC_MONETARY_MASK, "C", 0);
	ATF_REQUIRE_MSG(loc != 0, "No C locale!");

	ATF_CHECK(strfmon_l(buf, sizeof buf, loc, "[%!+#6n]", 123.0) < 0);
	ATF_CHECK_ERRNO(EINVAL,
	    (strfmon_l(buf, sizeof buf, loc, "[%!+#6n]", 123.0) < 0));

	/* While here verify buffer does not overflow */
	buf[4] = '&';
	ATF_REQUIRE(strfmon_l(buf, 4, loc, "[%!#6n]", 123.0) < 0);
	ATF_CHECK_ERRNO(E2BIG, (strfmon_l(buf, 4, loc, "[%!#6n]", 123.0) < 0));
	ATF_REQUIRE(buf[4] == '&');
	freelocale(loc);
}

ATF_TC(strfmon_pad);

ATF_TC_HEAD(strfmon_pad, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks strfmon padding");
}

ATF_TC_BODY(strfmon_pad, tc)
{
    char string[1024];

    ATF_REQUIRE(setlocale(LC_MONETARY, "en_US.UTF-8") != NULL);
    strfmon(string, sizeof(string), "[%10n] [%12i]", 123.45, 123.45);
    ATF_CHECK_STREQ(string, "[   $123.45] [   USD123.45]"); 
    strfmon(string, sizeof(string), "[%-10n] [%-12i]", 123.45, 123.45);
    ATF_REQUIRE_STREQ(string, "[$123.45   ] [USD123.45   ]"); 
}

ATF_TC(strfmon_locale_thousands);

ATF_TC_HEAD(strfmon_locale_thousands, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks strfmon locale thousands separator");
}

ATF_TC_BODY(strfmon_locale_thousands, tc)
{
	char actual[40], expected[40];
	struct lconv *lc;
	const char *ts;
	ssize_t res;
	size_t len;
	double n;

	ATF_REQUIRE(setlocale(LC_ALL, "sv_SE.UTF-8") != NULL);

	lc = localeconv();

	ts = lc->mon_thousands_sep;

	/* NULL value should be impossible */
	if (ts == NULL || strlen(ts) < 2)
		atf_tc_skip("multi-byte mon-thousands-separator not found");

	if (lc->mon_grouping == NULL ||
	    lc->mon_grouping[0] != 3 || lc->mon_grouping[1] != '\0')
		atf_tc_skip("monetary grouping is not '3'");

	n = 1234.56;
	res = strfmon(actual, sizeof(actual) - 1, "%!i", n);

	ATF_CHECK_MSG(res >= 0, "strfmon failed: %s", strerror(errno));

	strcpy(expected, "1");
	strlcat(expected, ts, sizeof(expected));
	strlcat(expected, "234", sizeof(expected));

	len = strlen(expected);
	ATF_CHECK_MSG((size_t)res > len, "strfmon returned %zd, expected > %zu",
	    res, len);

	/* We're just testing the thousands separator, not all of strfmon. */
	actual[len] = '\0';
	ATF_CHECK_STREQ(expected, actual);

	/* now do it all again with a bigger value */
	n = 12345678.9;
	res = strfmon(actual, sizeof(actual) - 1, "%!i", n);

	ATF_CHECK_MSG(res >= 0, "strfmon failed: %s", strerror(errno));

	strcpy(expected, "12");
	strlcat(expected, ts, sizeof(expected));
	strlcat(expected, "345", sizeof(expected));
	strlcat(expected, ts, sizeof(expected));
	strlcat(expected, "678", sizeof(expected));

	len = strlen(expected);
	ATF_CHECK_MSG((size_t)res > len, "strfmon returned %zd, expected > %zu",
	    res, len);

	actual[len] = '\0';
	ATF_CHECK_STREQ(expected, actual);
}

ATF_TC(strfmon_examples);
ATF_TC_HEAD(strfmon_examples, tc) {
	atf_tc_set_md_var(tc, "descr",
	    "Checks strfmon field formats");
}

ATF_TC_BODY(strfmon_examples, tc)
{
	const struct {
		const char *format;
		const char *expected;
	} tests[] = {
	    { "%n", "[$123.45] [-$123.45] [$3,456.78]" },
	    { "%11n", "[    $123.45] [   -$123.45] [  $3,456.78]" },
	    { "%#5n", "[ $   123.45] [-$   123.45] [ $ 3,456.78]" },
	    { "%=*#5n", "[ $***123.45] [-$***123.45] [ $*3,456.78]" },
	    { "%=0#5n", "[ $000123.45] [-$000123.45] [ $03,456.78]" },
	    { "%^#5n", "[ $  123.45] [-$  123.45] [ $ 3456.78]" },
	    { "%^#5.0n", "[ $  123] [-$  123] [ $ 3457]" },
	    { "%^#5.4n", "[ $  123.4500] [-$  123.4500] [ $ 3456.7810]" },
	    { "%(#5n", "[ $   123.45 ] [($   123.45)] [ $ 3,456.78 ]" },
	    { "%!(#5n", "[    123.45 ] [(   123.45)] [  3,456.78 ]" },
	    { "%-14#5.4n","[ $   123.4500 ] [-$   123.4500 ] [ $ 3,456.7810 ]"},
	    { "%14#5.4n","[  $   123.4500] [ -$   123.4500] [  $ 3,456.7810]"},
	};
	size_t i;
	char actual[100], format[50];

	if (setlocale(LC_MONETARY, "en_US.UTF-8") == NULL)
		atf_tc_skip("unable to setlocale(en_US.UTF-8)");

	for (i = 0; i < __arraycount(tests); ++i) {
		snprintf(format, sizeof(format), "[%s] [%s] [%s]",
		    tests[i].format, tests[i].format, tests[i].format);
		strfmon(actual, sizeof(actual) - 1, format,
		    123.45, -123.45, 3456.781);
		ATF_CHECK_STREQ_MSG(tests[i].expected, actual,
		    "[%s]", tests[i].format);
	}
}

struct otest {
	const char *expected;
};

static int
ordertests(const char *fmt, const struct otest *tests, size_t ntests,
    double val, int precedes, const char * const tname)
{
	size_t i, j;
	struct lconv *lc;
	char actual[100], buf[100];

	if (index(fmt, 'n') == NULL)
		atf_tc_fail("Only national currency tests supported");
	if (setlocale(LC_ALL, "en_US.UTF-8") == NULL) {
		atf_tc_skip("Cannot find locale en_US.UTF-8");
		return 0;
	}

	lc = localeconv();

	/*XXX
	 * Danger Will Robinson, Danger !!!
	 *
	 * Altering what localeconv() returns produces UB.
	 *	[the pointer might be to const (read only) memory]
	 * (yet it seems to work here the way we are using locales)
	 *XXX
	 */
	lc->n_cs_precedes = precedes;
	lc->p_cs_precedes = precedes;

	for (i = 0; i < ntests; ++i) {
		actual[0] = '\0';
		lc->n_sep_by_space = i;
		lc->p_sep_by_space = i;

		for (j = 0; j < 5; ++j) {
			if (val < 0 ) {
				lc->n_sign_posn = j;
				/*
				 * can't have both pos and neg using ()
				 * so set the one we aren't using to an
				 * arbitrary non-zero value (1 here) -- the
				 * value chosen is irrelevant, it isn't used,
				 * though it could affect alignment rules,
				 * but in practice, doesn't, as we know that
				 * the US domain has "" as + and "-" as -,
				 * which occupy the same number of columns,
				 * or less, than '(' or ')'.
				 */
				lc->p_sign_posn = j ? j : 1;
			} else {
				lc->p_sign_posn = j;
				lc->n_sign_posn = j ? j : 1;
			}

			strfmon(buf, sizeof(buf) - 1, fmt, val);
			strlcat(actual, buf, sizeof(actual));
		}

		actual[strlen(actual) - 1] = '\0';
		ATF_CHECK_STREQ_MSG(tests[i].expected, actual,
		    " [sign pos 0..4]   %stests fmt=\"%s\" "
		    "val=%.3f cs_precedes=%d sep_by_space(i)=%zu",
		    tname, fmt, val, precedes, i);
	}

	return 1;
}


ATF_TC(strfmon_cs_precedes_0);

ATF_TC_HEAD(strfmon_cs_precedes_0, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sep_by_space x sign_posn when cs_precedes = 0");
}

ATF_TC_BODY(strfmon_cs_precedes_0, tc)
{
	const struct otest ntests[] = {
	    /* sep_by_space(col) x sign_posn(row) */
	    { "[(789.12$)] [-789.12$] [789.12$-] [789.12-$] [789.12$-]" },
	    { "[(789.12 $)] [-789.12 $] [789.12 $-] [789.12 -$] [789.12 $-]" },
	    { "[(789.12$)] [- 789.12$] [789.12$ -] [789.12- $] [789.12$ -]" },
	}, ptests[] = {
	    { "[(789.12$)] [789.12$] [789.12$] [789.12$] [789.12$]" },
	    { "[(789.12 $)] [789.12 $] [789.12 $] [789.12 $] [789.12 $]" },
	    { "[(789.12$)] [ 789.12$] [789.12$ ] [789.12 $] [789.12$ ]" },
	}, patests[] = {
	    /*
	     * each row in patests should be perfectly aligned with the
	     * corresponding row in natests, all same width, digits aligned
	     * (the three rows in atests/natests are unrelated to each other)
	     */
	    { "[(789.12$)] [ 789.12$] [789.12$ ] [789.12$ ] [789.12$ ]" },
	    { "[(789.12 $)] [ 789.12 $] [789.12 $ ] [789.12 $ ] [789.12 $ ]" },
	    { "[(789.12$)] [  789.12$] [789.12$  ] [789.12 $ ] [789.12$  ]" },
	}, natests[] = {
	    { "[(789.12$)] [-789.12$] [789.12$-] [789.12-$] [789.12$-]" },
	    { "[(789.12 $)] [-789.12 $] [789.12 $-] [789.12 -$] [789.12 $-]" },
	    { "[(789.12$)] [- 789.12$] [789.12$ -] [789.12- $] [789.12$ -]" },
	};

	/*
	 * The format must end in a space, which is deleted from the
	 * last of the 5 ouputs produced for each test entry
	 */
	if (!ordertests("[%n] ", ntests, __arraycount(ntests), -789.12, 0, "n"))
		atf_tc_skip("unable to setlocale(en_US.UTF-8)");
	/*
	 * No need to test result hereafter, if setlocale() worked once,
	 * it will always - if it failed the atf_tc_skip() above happened
	 * and this code never runs.
	 */
	(void)ordertests("[%n] ", ptests, __arraycount(ptests), 789.12, 0, "p");

	/*
	 * the #1 left precision requests aligned results without extra pad
	 * this works only for values with the same number of pre-radix digits
	 * here, the value is always 789.12 so that is not an issue
	 */
	(void)ordertests("[%#1n] ", patests, __arraycount(patests), 789.12,
	    0, "pa");
	(void)ordertests("[%#1n] ", natests, __arraycount(natests), -789.12,
	    0, "na");
}

ATF_TC(strfmon_cs_precedes_1);

ATF_TC_HEAD(strfmon_cs_precedes_1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sep_by_space x sign_posn when cs_precedes = 1");
}

ATF_TC_BODY(strfmon_cs_precedes_1, tc)
{
	const struct otest ntests[] = {
	    /* sep_by_space x sign_posn */
	    { "[($789.12)] [-$789.12] [$789.12-] [-$789.12] [$-789.12]" },
	    { "[($ 789.12)] [-$ 789.12] [$ 789.12-] [-$ 789.12] [$- 789.12]" },
	    { "[($789.12)] [- $789.12] [$789.12 -] [- $789.12] [$ -789.12]" },
	}, ptests[] = {
	    { "[($789.12)] [$789.12] [$789.12] [$789.12] [$789.12]" },
	    { "[($ 789.12)] [$ 789.12] [$ 789.12] [$ 789.12] [$ 789.12]" },
	    { "[($789.12)] [ $789.12] [$789.12 ] [ $789.12] [$ 789.12]" },
	}, patests[] = {
	    { "[($789.12)] [ $789.12] [$789.12 ] [ $789.12] [ $789.12]" },
	    { "[($ 789.12)] [ $ 789.12] [$ 789.12 ] [ $ 789.12] [ $ 789.12]" },
	    { "[($789.12)] [  $789.12] [$789.12  ] [  $789.12] [ $ 789.12]" },
	}, natests[] = {
	    { "[($789.12)] [-$789.12] [$789.12-] [-$789.12] [$-789.12]" },
	    { "[($ 789.12)] [-$ 789.12] [$ 789.12-] [-$ 789.12] [$- 789.12]" },
	    { "[($789.12)] [- $789.12] [$789.12 -] [- $789.12] [$ -789.12]" },
	};

	if (!ordertests("[%n] ", ntests, __arraycount(ntests), -789.12, 1, "n"))
		atf_tc_skip("unable to setlocale(en_US.UTF-8)");
	(void)ordertests("[%n] ", ptests, __arraycount(ptests), 789.12, 1, "p");
	(void)ordertests("[%#1n] ", patests, __arraycount(patests), 789.12,
	    1, "pa");
	(void)ordertests("[%#1n] ", natests, __arraycount(natests), -789.12,
	    1, "na");
}

ATF_TC(strfmon_international_currency_code);
ATF_TC_HEAD(strfmon_international_currency_code, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "checks strfmon international currency code");
}

ATF_TC_BODY(strfmon_international_currency_code, tc)
{
	const struct {
		const char *locale;
		const char *expected;
	} tests[] = {
	    { "en_US.UTF-8", "[USD123.45]" },
	    { "de_DE.UTF-8", "[123,45 EUR]" },
	    { "C", "[123]" },
	    { "x1_X1.UTF-8", "[X1Q 123。4520]" },
	};
	size_t i;
	int res;
	char actual[100];

	for (i = 0; i < __arraycount(tests); ++i) {
		if (setlocale(LC_MONETARY, tests[i].locale) == NULL)
			atf_tc_skip("unable to setlocale(%s)",
			    tests[i].locale);

		res = strfmon(actual, sizeof(actual), "[%i]", 123.452);
		ATF_REQUIRE_MSG(res > 0, "Using %s res=%d errno=%d\n",
			tests[i].locale, res, errno);
		ATF_CHECK_STREQ_MSG(tests[i].expected, actual,
		   "Using %s (i == %zu)", tests[i].locale, i);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, strfmon_locale);
	ATF_TP_ADD_TC(tp, strfmon_pad);
	ATF_TP_ADD_TC(tp, strfmon_locale_thousands);
	ATF_TP_ADD_TC(tp, strfmon_examples);
	ATF_TP_ADD_TC(tp, strfmon_cs_precedes_0);
	ATF_TP_ADD_TC(tp, strfmon_cs_precedes_1);
	ATF_TP_ADD_TC(tp, strfmon_international_currency_code);

	return atf_no_error();
}
