/* $NetBSD: t_toupper.c,v 1.1.4.1 2018/01/23 03:12:11 perseant Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad Schroder
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
__RCSID("$NetBSD: t_toupper.c,v 1.1.4.1 2018/01/23 03:12:11 perseant Exp $");

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <vis.h>
#include <ctype.h>

#include <atf-c.h>

static struct test {
	const char *locale;
	const char *lower;
	const char *upper;
} tests[] = {
	{
		"C",
		"abcde12345",
		"ABCDE12345",
	}, {
		"ru_RU.KOI8-R",
		"abcde12345\xc1\xc2\xd7\xc7\xc4\xc5\xa3",
		"ABCDE12345\xe1\xe2\xf7\xe7\xe4\xe5\xb3",
	}, {
		NULL,
		NULL,
		NULL,
	}
};

static void
h_swapcase(const struct test *t, int upperp)
{
	unsigned int i;
	unsigned char answer, reported;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_CTYPE, t->locale) != NULL);
	printf("Using locale: %s\n", setlocale(LC_ALL, NULL));

	for (i = 0; i < strlen(t->lower); i++) {
		printf("Comparing char %d, lower %2.2x, with upper %2.2x\n",
			i, (unsigned char)t->lower[i], (unsigned char)t->upper[i]);
		if (upperp) {
			answer = t->upper[i];
			reported = toupper((int)(unsigned char)t->lower[i]);
		} else {
			answer = t->lower[i];
			reported = tolower((int)(unsigned char)t->upper[i]);
		}
		printf("  expecting %2.2x, reported %2.2x\n", answer, reported);
		ATF_REQUIRE_EQ(reported, answer);
	}
}

ATF_TC(toupper);

ATF_TC_HEAD(toupper, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks toupper under diferent locales");
}

ATF_TC_BODY(toupper, tc)
{
	struct test *t;

	for (t = &tests[0]; t->locale != NULL; ++t)
		h_swapcase(t, 1);
}

ATF_TC(tolower);

ATF_TC_HEAD(tolower, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks tolower under diferent locales");
}

ATF_TC_BODY(tolower, tc)
{
	struct test *t;

	/* atf_tc_expect_fail("%s", "LC_COLLATE not supported"); */
	for (t = &tests[0]; t->locale != NULL; ++t)
		h_swapcase(t, 0);
	/* atf_tc_expect_pass(); */
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, toupper);
	ATF_TP_ADD_TC(tp, tolower);

	return atf_no_error();
}
