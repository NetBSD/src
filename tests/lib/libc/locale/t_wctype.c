/* $NetBSD: t_wctype.c,v 1.3 2022/05/24 20:50:20 andvar Exp $ */

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
__RCSID("$NetBSD: t_wctype.c,v 1.3 2022/05/24 20:50:20 andvar Exp $");

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <vis.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>

#include <atf-c.h>

static const char *typenames[] = {
	"alnum",
	"alpha",
	"blank",
	"cntrl",
	"digit",
	"graph",
	"ideogram",
	"lower",
	"phonogram",
	"print",
	"punct",
	"rune",
	"space",
	"special",
	"upper",
	"xdigit",
};

static struct test {
	const char *locale;
	const struct {
		const char *in;
		const char *out;
	} classes[16];
} tests[] = {
	{
		"C",
		{
			{ /* alnum */ "abc123", "&^%$" },
			{ /* alpha */ "ABCabc", "123*&^" },
			{ /* blank */ " \t", "abc123%$^&\r\n" },
			{ /* cntrl */ "\a", "abc123^&*" },
			{ /* digit */ "123", "abc*()" },
			{ /* graph */ "abc123ABC[];?~", " " },
			{ /* ideogram */ "", "" },
			{ /* lower */ "abc", "ABC123&*(" },
			{ /* phonogram */ "", "" },
			{ /* print */ "abcABC123%^&* ", "\r\n\t" },
			{ /* punct */ ".,;:!?", "abc123 \n" },
			{ /* rune */ "", "" },
			{ /* space */ " \t\r\n", "abc123ABC&*(" },
			{ /* special */ "", "" },
			{ /* upper */ "ABC", "abc123&*(" },
			{ /* xdigit */ "123abcABC", "xyz" },
		},
	}, {
		"en_US.UTF-8",
		{
			{ /* alnum */ "abc123", "&^%$" },
			{ /* alpha */ "ABCabc", "123*&^" },
			{ /* blank */ " \t", "abc123%$^&\r\n" },
			{ /* cntrl */ "\a", "abc123^&*" },
			{ /* digit */ "123", "abc*()" },
			{ /* graph */ "abc123ABC[];?~", " " },
			{ /* ideogram */ "", "" },
			{ /* lower */ "abc", "ABC123&*(" },
			{ /* phonogram */ "", "" },
			{ /* print */ "abcABC123%^&* ", "\r\n\t" },
			{ /* punct */ ".,;:!?", "abc123 \n" },
			{ /* rune */ "", "" },
			{ /* space */ " \t\r\n", "abc123ABC&*(" },
			{ /* special */ "", "" },
			{ /* upper */ "ABC", "abc123&*(" },
			{ /* xdigit */ "123abcABC", "xyz" },
		},
	}, {
		"ru_RU.KOI8-R",
		{
			{ /* alnum */ "abc123i\xa3\xb3\xd6", "&^%$" },
			{ /* alpha */ "ABCabci\xa3\xb3\xd6", "123*&^" },
			{ /* blank */ " \t", "abc123%$^&\r\n" },
			{ /* cntrl */ "\a", "abc123^&*" },
			{ /* digit */ "123", "abc*()" },
			{ /* graph */ "abc123ABC[];?~i\xa3\xb3\xd6", " " },
			{ /* ideogram */ "", "" },
			{ /* lower */ "abci\xa3\xd6", "ABC123&*(\xb3\xf6" },
			{ /* phonogram */ "", "" },
			{ /* print */ "abcABC123%^&* \xa3\xb3\xd6\xbf", "\r\n\t" },
			{ /* punct */ ".,;:!?", "abc123 \n" },
			{ /* rune */ "", "" },
			{ /* space */ " \t\r\n", "abc123ABC&*(\xa3\xb3\xd6" },
			{ /* special */ "", "" },
			{ /* upper */ "ABC\xb3\xe0\xff", "abc123&*(\xa3\xc0\xdf" },
			{ /* xdigit */ "123abcABC", "xyz\xc1\xc5" },
		}
	}, {
		NULL,
		{ {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, }
	}
};

static void testall(int, int, wchar_t, wctype_t, int);

static void
h_ctype(const struct test *t)
{
	wctype_t wct;
	wchar_t wc;
	int i;
	const char *cp;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_CTYPE, t->locale) != NULL);
	printf("Using locale %s\n", setlocale(LC_CTYPE, NULL));

	for (i = 0; i < 16; i++) {
		/* Skip any type that has no chars */
		if (t->classes[i].in[0] == '\0' &&
		    t->classes[i].out[0] == '\0')
			continue;

		/* First examine wctype */
		wct = wctype(typenames[i]);
		ATF_REQUIRE(wct != 0);

		/* Convert in to wide and check that they match */
		for (cp = (const char *)(t->classes[i].in); *cp; ++cp) {
			wc = btowc((unsigned char)*cp);
			testall(i, (unsigned char)*cp, wc, wct, 1);
		}
		/* Convert out to wide and check that they do not match */
		for (cp = (const char *)(t->classes[i].out); *cp; ++cp) {
			wc = btowc((unsigned char)*cp);
			testall(i, (unsigned char)*cp, wc, wct, 0);
		}
	}
}

void testall(int idx, int c, wchar_t wc, wctype_t wct, int inout)
{
	printf("Testing class %d (%s), char %c (0x%2.2x), wc %x, wct %ld, expect %d\n",
		idx, typenames[idx], c, c, wc, (long int)wct, inout);
	ATF_REQUIRE(!!iswctype(wc, wct) == inout);

	/* Switch based on wct and call is{,w}* to check */
	switch (idx) {
	case 0: /* alnum */
		ATF_REQUIRE_EQ(!!isalnum(c), inout);
		ATF_REQUIRE_EQ(!!iswalnum(wc), inout);
		break;
	case 1: /* alpha */
		ATF_REQUIRE_EQ(!!isalpha(c), inout);
		ATF_REQUIRE_EQ(!!iswalpha(wc), inout);
		break;
	case 2: /* blank */
		ATF_REQUIRE_EQ(!!isblank(c), inout);
		ATF_REQUIRE_EQ(!!iswblank(wc), inout);
		break;
	case 3: /* cntrl */
		ATF_REQUIRE_EQ(!!iscntrl(c), inout);
		ATF_REQUIRE_EQ(!!iswcntrl(wc), inout);
		break;
	case 4: /* digit */
		ATF_REQUIRE_EQ(!!isdigit(c), inout);
		ATF_REQUIRE_EQ(!!iswdigit(wc), inout);
		break;
	case 5: /* graph */
		ATF_REQUIRE_EQ(!!isgraph(c), inout);
		ATF_REQUIRE_EQ(!!iswgraph(wc), inout);
		break;
	case 6: /* ideogram */
#if 0
		ATF_REQUIRE_EQ(!!isideogram(c), inout);
		ATF_REQUIRE_EQ(!!iswideogram(wc), inout);
#endif
		break;
	case 7: /* lower */
		ATF_REQUIRE_EQ(!!islower(c), inout);
		ATF_REQUIRE_EQ(!!iswlower(wc), inout);
		break;
	case 8: /* phonogram */
#if 0
		ATF_REQUIRE_EQ(!!isphonogram(c), inout);
		ATF_REQUIRE_EQ(!!iswphonogram(wc), inout);
#endif
		break;
	case 9: /* print */
		ATF_REQUIRE_EQ(!!isprint(c), inout);
		ATF_REQUIRE_EQ(!!iswprint(wc), inout);
		break;
	case 10: /* punct */
		ATF_REQUIRE_EQ(!!ispunct(c), inout);
		ATF_REQUIRE_EQ(!!iswpunct(wc), inout);
		break;
	case 11: /* rune */
#if 0
		ATF_REQUIRE_EQ(!!isrune(c), inout);
		ATF_REQUIRE_EQ(!!iswrune(wc), inout);
#endif
		break;
	case 12: /* space */
		ATF_REQUIRE_EQ(!!isspace(c), inout);
		ATF_REQUIRE_EQ(!!iswspace(wc), inout);
		break;
	case 13: /* special */
#if 0
		ATF_REQUIRE_EQ(!!isspecial(c), inout);
		ATF_REQUIRE_EQ(!!iswspecial(wc), inout);
#endif
		break;
	case 14: /* upper */
		ATF_REQUIRE_EQ(!!isupper(c), inout);
		ATF_REQUIRE_EQ(!!iswupper(wc), inout);
		break;
	case 15: /* xdigit */
		ATF_REQUIRE_EQ(!!isxdigit(c), inout);
		ATF_REQUIRE_EQ(!!iswxdigit(wc), inout);
		break;
	}
}

ATF_TC(ctype);

ATF_TC_HEAD(ctype, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks is* and isw* under different locales");
}

ATF_TC_BODY(ctype, tc)
{
	struct test *t;

	for (t = &tests[0]; t->locale != NULL; ++t)
		h_ctype(t);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ctype);

	return atf_no_error();
}
