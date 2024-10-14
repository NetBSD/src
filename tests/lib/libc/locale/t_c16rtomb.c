/*	$NetBSD: t_c16rtomb.c,v 1.6.2.2 2024/10/14 17:20:19 martin Exp $	*/

/*-
 * Copyright (c) 2002 Tim J. Robbins
 * All rights reserved.
 *
 * Copyright (c) 2013 Ed Schouten <ed@FreeBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Test program for c16rtomb() as specified by ISO/IEC 9899:2011.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_c16rtomb.c,v 1.6.2.2 2024/10/14 17:20:19 martin Exp $");

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <uchar.h>

#include <atf-c.h>

static void
require_lc_ctype(const char *locale_name)
{
	char *lc_ctype_set;

	lc_ctype_set = setlocale(LC_CTYPE, locale_name);
	if (lc_ctype_set == NULL)
		atf_tc_fail("setlocale(LC_CTYPE, \"%s\") failed; errno=%d",
		    locale_name, errno);

	ATF_REQUIRE_EQ_MSG(strcmp(lc_ctype_set, locale_name), 0,
	    "lc_ctype_set=%s locale_name=%s", lc_ctype_set, locale_name);
}

static mbstate_t s;
static char buf[7*MB_LEN_MAX + 1];

ATF_TC_WITHOUT_HEAD(c16rtomb_c_locale_test);
ATF_TC_BODY(c16rtomb_c_locale_test, tc)
{
	size_t n;

	require_lc_ctype("C");

	/*
	 * If the buffer argument is NULL, c16 is implicitly 0,
	 * c16rtomb() resets its internal state.
	 */
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, L'\0', NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, 0xdc00, NULL)), 1, "n=%zu", n);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0, &s)), 1, "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == 0 &&
		(unsigned char)buf[1] == 0xcc),
	    "buf=[%02x %02x]", buf[0], buf[1]);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, L'\0', NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, L'A', NULL)), 1, "n=%zu", n);

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, L'A', &s)), 1, "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == 'A' &&
		(unsigned char)buf[1] == 0xcc),
	    "buf=[%02x %02x]", buf[0], buf[1]);

	/* Unicode character 'Pile of poo'. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xd83d, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xdca9, &s)), (size_t)-1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(errno, EILSEQ, "errno=%d", errno);
	ATF_CHECK_EQ_MSG((unsigned char)buf[0], 0xcc, "buf=[%02x]", buf[0]);

	/* Incomplete Unicode character 'Pile of poo', interrupted by NUL. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xd83d, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, L'\0', &s)), 1, "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == '\0' &&
		(unsigned char)buf[1] == 0xcc),
	    "buf=[%02x %02x]", buf[0], buf[1]);
}

ATF_TC_WITHOUT_HEAD(c16rtomb_iso2022jp_locale_test);
ATF_TC_BODY(c16rtomb_iso2022jp_locale_test, tc)
{
	char *p;
	size_t n;

	require_lc_ctype("ja_JP.ISO-2022-JP");

	/*
	 * If the buffer argument is NULL, c16 is implicitly 0,
	 * c16rtomb() resets its internal state.
	 */
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, L'\0', NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, 0xdc00, NULL)), 1, "n=%zu", n);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0, &s)), 1, "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == 0 &&
		(unsigned char)buf[1] == 0xcc),
	    "buf=[%02x %02x]", buf[0], buf[1]);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, L'\0', NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(NULL, L'A', NULL)), 1, "n=%zu", n);

	/*
	 * 1. U+0042 LATIN CAPITAL LETTER A
	 * 2. U+00A5 YEN SIGN
	 * 3. U+00A5 YEN SIGN (again, no shift needed)
	 * 4. U+30A2 KATAKANA LETTER A
	 * 5. U+30A2 KATAKANA LETTER A (again, no shift needed)
	 * 6. incomplete UTF-16 surrogate pair -- no output
	 * 7. U+0000 NUL (plus shift sequence to initial state)
	 */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	p = buf;
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, L'A', &s)), 1, "n=%zu", n); /* 1 */
	p += 1;
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, 0xa5, &s)), 4, "n=%zu", n); /* 2 */
	p += 4;
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, 0xa5, &s)), 1, "n=%zu", n); /* 3 */
	p += 1;
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, 0x30a2, &s)), 5, "n=%zu", n); /* 4 */
	p += 5;
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, 0x30a2, &s)), 2, "n=%zu", n); /* 5 */
	p += 2;
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, 0xd800, &s)), 0, "n=%zu", n); /* 6 */
	ATF_CHECK_EQ_MSG((n = c16rtomb(p, L'\0', &s)), 4, "n=%zu", n); /* 7 */
	p += 4;
	ATF_CHECK_MSG(((unsigned char)buf[0] == 'A' &&
		(unsigned char)buf[1] == 0x1b && /* shift ISO/IEC 646:JP */
		(unsigned char)buf[2] == '(' &&
		(unsigned char)buf[3] == 'J' &&
		(unsigned char)buf[4] == 0x5c && /* YEN SIGN */
		(unsigned char)buf[5] == 0x5c && /* YEN SIGN */
		(unsigned char)buf[6] == 0x1b && /* shift JIS X 0208 */
		(unsigned char)buf[7] == '$' &&
		(unsigned char)buf[8] == 'B' &&
		(unsigned char)buf[9] == 0x25 && /* KATAKANA LETTER A */
		(unsigned char)buf[10] == 0x22 &&
		(unsigned char)buf[11] == 0x25 && /* KATAKANA LETTER A */
		(unsigned char)buf[12] == 0x22 &&
		(unsigned char)buf[13] == 0x1b && /* shift US-ASCII */
		(unsigned char)buf[14] == '(' &&
		(unsigned char)buf[15] == 'B' &&
		(unsigned char)buf[16] == '\0' &&
		(unsigned char)buf[17] == 0xcc),
	    "buf=[%02x %02x %02x %02x  %02x %02x %02x %02x "
	    " %02x %02x %02x %02x  %02x %02x %02x %02x "
	    " %02x %02x]",
	    buf[0], buf[1], buf[2], buf[3],
	    buf[4], buf[5], buf[6], buf[7],
	    buf[8], buf[9], buf[10], buf[11],
	    buf[12], buf[13], buf[14], buf[15],
	    buf[16], buf[17]);
}

ATF_TC_WITHOUT_HEAD(c16rtomb_iso_8859_1_test);
ATF_TC_BODY(c16rtomb_iso_8859_1_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.ISO8859-1");

	/* Unicode character 'Euro sign'. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0x20ac, &s)), (size_t)-1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(errno, EILSEQ, "errno=%d", errno);
	ATF_CHECK_EQ_MSG((unsigned char)buf[0], 0xcc, "buf=[%02x]", buf[0]);
}

ATF_TC_WITHOUT_HEAD(c16rtomb_iso_8859_15_test);
ATF_TC_BODY(c16rtomb_iso_8859_15_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.ISO8859-15");

	/* Unicode character 'Euro sign'. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0x20ac, &s)), 1, "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == 0xa4 &&
		(unsigned char)buf[1] == 0xcc),
	    "buf=[%02x %02x]", buf[0], buf[1]);
}

ATF_TC_WITHOUT_HEAD(c16rtomb_utf_8_test);
ATF_TC_BODY(c16rtomb_utf_8_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.UTF-8");

	/* Unicode character 'Pile of poo'. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xd83d, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xdca9, &s)), 4, "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == 0xf0 &&
		(unsigned char)buf[1] == 0x9f &&
		(unsigned char)buf[2] == 0x92 &&
		(unsigned char)buf[3] == 0xa9 &&
		(unsigned char)buf[4] == 0xcc),
	    "buf=[%02x %02x %02x %02x %02x]",
	    buf[0], buf[1], buf[2], buf[3], buf[4]);

	/* Invalid code; 'Pile of poo' without the trail surrogate. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xd83d, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, L'A', &s)), (size_t)-1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(errno, EILSEQ, "errno=%d", errno);
	ATF_CHECK_EQ_MSG((unsigned char)buf[0], 0xcc, "buf=[%02x]", buf[0]);

	/* Invalid code; 'Pile of poo' without the lead surrogate. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xdca9, &s)), (size_t)-1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(errno, EILSEQ, "errno=%d", errno);
	ATF_CHECK_EQ_MSG((unsigned char)buf[0], 0xcc, "buf=[%02x]", buf[0]);

	/* Incomplete Unicode character 'Pile of poo', interrupted by NUL. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, 0xd83d, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = c16rtomb(buf, L'\0', &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_MSG(((unsigned char)buf[0] == '\0' &&
		(unsigned char)buf[1] == 0xcc),
	    "buf=[%02x %02x]", buf[0], buf[1]);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, c16rtomb_c_locale_test);
	ATF_TP_ADD_TC(tp, c16rtomb_iso2022jp_locale_test);
	ATF_TP_ADD_TC(tp, c16rtomb_iso_8859_1_test);
	ATF_TP_ADD_TC(tp, c16rtomb_iso_8859_15_test);
	ATF_TP_ADD_TC(tp, c16rtomb_utf_8_test);

	return (atf_no_error());
}
