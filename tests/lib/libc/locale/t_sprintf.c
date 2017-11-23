/* $NetBSD: t_sprintf.c,v 1.4 2017/11/23 23:47:09 kre Exp $ */

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
__RCSID("$NetBSD: t_sprintf.c,v 1.4 2017/11/23 23:47:09 kre Exp $");

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include <atf-c.h>

static struct test {
	const char *locale;
	const int int_value;
	const char *int_result;
	const char *int_input;
	const double double_value;
	const char *double_result;
	const char *double_input;
} tests[] = {
	{
		"en_US.UTF-8",
		-12345,
		"-12,345",
		"-12345",
		-12345.6789,
		"-12,345.678900",
		"-12345.678900",
	}, {
		"fr_FR.ISO8859-1",
		-12345,
		"-12\240345",
		"-12345",
		-12345.6789,
		"-12\240345,678900",
		"-12345,678900",
	}, {
		"it_IT.ISO8859-1",
		-12345,
		"-12.345",
		"-12345",
		-12345.6789,
		"-12.345,678900",
		"-12345,678900",
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
		-12345.6789,
		"-12345.678900",
		"-12345.678900",
	}, {
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
	ATF_REQUIRE_STREQ(buf, t->double_result);

	sprintf(buf, "%'d", t->int_value);
	ATF_REQUIRE_STREQ(buf, t->int_result);

        atf_tc_expect_pass();
}

static void
h_strto(const struct test *t)
{
	double d;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_NUMERIC, t->locale) != NULL);

	ATF_REQUIRE_EQ((int)strtol(t->int_input, NULL, 10), t->int_value);
	d = strtod(t->double_input, NULL);
	ATF_REQUIRE_EQ_MSG(d, t->double_value, "In %s: "
	    "strtod(t->double_input[%s], NULL)[%g] != t->double_value[%g]",
	    t->locale, t->double_input, d, t->double_value);
}

static void
h_sscanf(const struct test *t)
{
	int int_reported;
	double double_reported;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_NUMERIC, t->locale) != NULL);

	sscanf(t->int_input, "%d", &int_reported);
	ATF_REQUIRE_EQ(int_reported, t->int_value);
	sscanf(t->double_input, "%lf", &double_reported);
	ATF_REQUIRE_EQ(double_reported, t->double_value);
}

ATF_TC(sprintf);
ATF_TC_HEAD(sprintf, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks sprintf %%'d and %%'f under diferent locales");
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
		"Checks strtol and strtod under diferent locales");
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
		"Checks sscanf under diferent locales");
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
