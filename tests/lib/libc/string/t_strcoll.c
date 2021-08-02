/* $NetBSD: t_strcoll.c,v 1.2 2021/08/02 17:41:07 andvar Exp $ */

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
__RCSID("$NetBSD: t_strcoll.c,v 1.2 2021/08/02 17:41:07 andvar Exp $");

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <vis.h>

#include <atf-c.h>

static struct test {
	const char *locale;
	const char * const data[5];
} tests[] = {
	{
		"C",
		{ "aardvark", "absolution", "zyzygy", NULL },
	}, {
		"ru_RU.KOI8-R",
		{ "\xc5\xc4\xcf\xcb", "\xa3\xd6", "\xc5\xda\xc4\xc9\xd4\xd8", NULL },
	}, {
		NULL,
		{ NULL, NULL, NULL, NULL },
	}
};

static void
h_ordering(const struct test *t)
{
	const char * const *a;
	const char * const *b;
	char buf_a[1024], buf_b[1024];

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_COLLATE, t->locale) != NULL);

	for (a = t->data; *a != NULL; ++a) {
		strvis(buf_a, *a, VIS_WHITE | VIS_OCTAL);
		for (b = a + 1; *b != NULL; ++b) {
			strvis(buf_b, *b, VIS_WHITE | VIS_OCTAL);
			printf("Checking \"%s\" < \"%s\"\n", buf_a, buf_b);
			ATF_REQUIRE(strcoll(*a, *b) < 0);
			printf("...good\n");
		}
	}
}

ATF_TC(ordering);

ATF_TC_HEAD(ordering, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks collation ordering under different locales");
}

ATF_TC_BODY(ordering, tc)
{
	struct test *t;

	atf_tc_expect_fail("%s", "LC_COLLATE not supported");
	for (t = &tests[0]; t->locale != NULL; ++t)
		h_ordering(t);
	atf_tc_expect_pass();
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ordering);

	return atf_no_error();
}
