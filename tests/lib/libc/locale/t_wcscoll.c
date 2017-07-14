/* $NetBSD: t_wcscoll.c,v 1.1 2017/07/14 14:57:43 perseant Exp $ */

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
__RCSID("$NetBSD: t_wcscoll.c,v 1.1 2017/07/14 14:57:43 perseant Exp $");

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>

#include <atf-c.h>

static struct test {
	const char *locale;
	const wchar_t *in_order[10];
} tests[] = {
	{
		"C", {
			L"A string beginning with a"
			L"Capital Letter",
			L"always comes before",
			L"another beginning lowercase",
			L"assuming ASCII of course",
			NULL }
	} , {
		"en_US.UTF-8", {
			L"A string beginning with a"
			L"Capital Letter",
			L"always comes before",
			L"another beginning lowercase",
			L"assuming ASCII of course",
			NULL }
	},
		/*
		 * The rest of these examples are from Wikipedia.
		 */
	{
		"de_DE.UTF-8", {
			L"Arg",
			L"Ärgerlich",
			L"Arm",
			L"Assistent",
			L"Aßlar",
			L"Assoziation",
			NULL }
	}, {
		"ru_RU.KOI-8", {
			L"едок",
			L"ёж",
			L"ездить",
			NULL }
	}, { /* Old-style Spanish collation, expect fail with DUCET */
		"es_ES.UTF-8", {
			L"cinco",
			L"credo",
			L"chispa",
			L"lomo",
			L"luz",
			L"llama",
			NULL }
	}, { /* We don't have Slovak locale files, expect fail */
		"sk_SK.UTF-8", {
			L"baa",
			L"baá",
			L"báa",
			L"bab",
			L"báb",
			L"bac",
			L"bác",
			L"bač",
			L"báč",
			NULL }
	}, {
		NULL,
		{ NULL }
	}
};

static void
h_wcscoll(const struct test *t)
{
	const wchar_t * const *wcp;
	const wchar_t * const *owcp;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_COLLATE, t->locale) != NULL);
	printf("Using locale: %s\n", setlocale(LC_ALL, NULL));
	
	for (wcp = &t->in_order[0], owcp = wcp++;
	     *wcp != NULL; owcp = wcp++) {
		printf("Check L\"%S\" < L\"%S\"\n", *owcp, *wcp);
		ATF_CHECK(wcscoll(*owcp, *wcp) < 0);
	}
}

ATF_TC(wcscoll);
ATF_TC_HEAD(wcscoll, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks collation using wcscoll(3) under different locales");
}
ATF_TC_BODY(wcscoll, tc)
{
	struct test *t;

	atf_tc_expect_fail("LC_COLLATE support is not yet fully implemented");
	for (t = &tests[0]; t->locale != NULL; ++t)
		h_wcscoll(t);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, wcscoll);

	return atf_no_error();
}
