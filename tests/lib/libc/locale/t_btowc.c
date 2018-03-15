/* $NetBSD: t_btowc.c,v 1.1.2.1 2018/03/15 09:55:23 martin Exp $ */

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
__RCSID("$NetBSD: t_btowc.c,v 1.1.2.1 2018/03/15 09:55:23 martin Exp $");

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>

#include <atf-c.h>

struct test {
	const char *locale;
	const char *illegal; /* Illegal single-byte characters, if any */
	const char *legal;   /* Legal single-byte characters */
	/* The next two are only used if __STDC_ISO_10646__ is defined */
	const wchar_t wlegal[8]; /* The same characters, but in ISO-10646 */
	const wchar_t willegal[8]; /* ISO-10646 that do not map into charset */
} tests[] = {
	{
		"en_US.UTF-8",
		"\200",
		"ABC123@\t",
		{ 'A', 'B', 'C', '1', '2', '3', '@', '\t' },
		{ 0xfdd0, 0x10fffe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
	},
	{
                "ru_RU.KOI8-R",
		"", /* No illegal characters in KOI8-R */
                "A\xc2\xd7\xc7\xc4\xc5\xa3",
		{ 'A', 0x0431, 0x432, 0x0433, 0x0434, 0x0435, 0x0451, 0x0 },
		{ 0x00c5, 0x00e6, 0x00fe, 0x0630, 0x06fc, 0x56cd, 0x0, 0x0 }
	},
	{
		NULL,
                NULL,
                NULL,
		{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
		{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
	},
};

#ifdef __STDC_ISO_10646__
static void
h_iso10646(struct test *t)
{
	const char *cp;
	int c, wc;
	char *str;
	const wchar_t *wcp;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale: %s\n", t->locale);
	ATF_REQUIRE(setlocale(LC_CTYPE, t->locale) != NULL);
	ATF_REQUIRE((str = setlocale(LC_ALL, NULL)) != NULL);
	(void)printf("Using locale: %s\n", str);

	/* These should have valid wchar representations */
	for (cp = t->legal, wcp = t->wlegal; *cp != '\0'; ++cp, ++wcp) {
		c = (int)(unsigned char)*cp;
		printf("Checking legal character 0x%x\n", c);
		wc = btowc(c);

		if (errno != 0)
			printf(" btowc() failed with errno=%d\n", errno);

		/* It should map to the known Unicode equivalent */
		printf("btowc(0x%2.2x) = 0x%x, expecting 0x%x\n",
		       c, wc, *wcp);
		ATF_REQUIRE(btowc(c) == *wcp);
	}

	/* These are invalid characters in the target set */
	for (wcp = t->willegal; *wcp != '\0'; ++wcp) {
		printf("Checking illegal wide character 0x%lx\n",
			(unsigned long)*wcp);
		ATF_REQUIRE_EQ(wctob(*wcp), EOF);
	}
}
#endif

static void
h_btowc(struct test *t)
{
	const char *cp;
	unsigned char c;
	char *str;
	const wchar_t *wcp;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale: %s\n", t->locale);
	ATF_REQUIRE(setlocale(LC_CTYPE, t->locale) != NULL);
	ATF_REQUIRE((str = setlocale(LC_ALL, NULL)) != NULL);
	(void)printf("Using locale: %s\n", str);

	/* btowc(EOF) -> WEOF */
	ATF_REQUIRE_EQ(btowc(EOF), WEOF);

	/* wctob(WEOF) -> EOF */
	ATF_REQUIRE_EQ(wctob(WEOF), EOF);

	/* Invalid in initial shift state -> WEOF */
	for (cp = t->illegal; *cp != '\0'; ++cp) {
		printf("Checking illegal character 0x%x\n",
			(unsigned char)*cp);
		ATF_REQUIRE_EQ(btowc(*cp), WEOF);
	}

	/* These should have valid wchar representations */
	for (cp = t->legal; *cp != '\0'; ++cp) {
		c = (unsigned char)*cp;
		printf("Checking legal character 0x%x\n", c);

		/* A legal character never maps to EOF */
		ATF_REQUIRE(btowc(c) != WEOF);

		/* And the mapping should be reversible */
		printf("0x%x -> wide 0x%x -> 0x%x\n",
			c, btowc(c), (unsigned char)wctob(btowc(c)));
		ATF_REQUIRE_EQ(wctob(btowc(c)), c);
	}
}

ATF_TC(btowc);
ATF_TC_HEAD(btowc, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks btowc(3) and wctob(3)");
}
ATF_TC_BODY(btowc, tc)
{
	struct test *t;

	for (t = tests; t->locale != NULL; ++t)
		h_btowc(t);
}

ATF_TC(stdc_iso_10646);
ATF_TC_HEAD(stdc_iso_10646, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks btowc(3) conversion to ISO10646");
}
ATF_TC_BODY(stdc_iso_10646, tc)
{
	struct test *t;

#ifdef __STDC_ISO_10646__
	for (t = tests; t->locale != NULL; ++t)
		h_iso10646(t);
#else /* ! __STDC_ISO_10646__ */
	atf_tc_skip("__STDC_ISO_10646__ not defined");
#endif /* ! __STDC_ISO_10646__ */
}

ATF_TC(btowc_posix);
ATF_TC_HEAD(btowc_posix, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks btowc(3) and wctob(3) for POSIX locale");
}
ATF_TC_BODY(btowc_posix, tc)
{
	const char *cp;
	unsigned char c;
	char *str;
	const wchar_t *wcp;
	int i;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "POSIX"), "POSIX");

	/* btowc(EOF) -> WEOF */
	ATF_REQUIRE_EQ(btowc(EOF), WEOF);

	/* wctob(WEOF) -> EOF */
	ATF_REQUIRE_EQ(wctob(WEOF), EOF);

	/* All characters from 0 to 255, inclusive, map
	   onto their unsigned char equivalent */
	for (i = 0; i <= 255; i++) {
		ATF_REQUIRE_EQ(btowc(i), (wchar_t)(unsigned char)(i));
		ATF_REQUIRE_EQ((unsigned char)wctob(i), (wchar_t)i);
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, btowc);
	ATF_TP_ADD_TC(tp, btowc_posix);
	ATF_TP_ADD_TC(tp, stdc_iso_10646);

	return atf_no_error();
}
