/*	$NetBSD: t_mbrtoc16.c,v 1.3.2.2 2024/10/14 17:20:19 martin Exp $	*/

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
 * Test program for mbrtoc16() as specified by ISO/IEC 9899:2011.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_mbrtoc16.c,v 1.3.2.2 2024/10/14 17:20:19 martin Exp $");

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
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
static char16_t c16;

ATF_TC_WITHOUT_HEAD(mbrtoc16_c_locale_test);
ATF_TC_BODY(mbrtoc16_c_locale_test, tc)
{
	size_t n;

	require_lc_ctype("C");

	/* Null wide character, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 1, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 1, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc16(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A", 1, NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%"PRIx16" L'A'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%"PRIx16" L'A'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');

	/* Incomplete character sequence. */
	c16 = L'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'z', "c16=U+%"PRIx16" L'z'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'z');

	/* Check that mbrtoc16() doesn't access the buffer when n == 0. */
	c16 = L'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'z', "c16=U+%"PRIx16" L'z'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'z');

	/* Check that mbrtoc16() doesn't read ahead too aggressively. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "AB", 2, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%"PRIx16" L'A'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "C", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'C', "c16=U+%"PRIx16" L'C'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'C');
}

ATF_TC_WITHOUT_HEAD(mbrtoc16_iso2022jp_locale_test);
ATF_TC_BODY(mbrtoc16_iso2022jp_locale_test, tc)
{
	size_t n;

	require_lc_ctype("ja_JP.ISO-2022-JP");

	/* Null wide character, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 1, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 1, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc16(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A", 1, NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%04"PRIx16" L'A'=U+%04"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%04"PRIx16" L'A'=U+%04"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');

	/* Incomplete character sequence. */
	c16 = L'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'z', "c16=U+%04"PRIx16" L'z'=U+%04"PRIx16,
	    (uint16_t)c16, (uint16_t)L'z');

	/* Check that mbrtoc16() doesn't access the buffer when n == 0. */
	c16 = L'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'z', "c16=U+%04"PRIx16" L'z'=U+%04"PRIx16,
	    (uint16_t)c16, (uint16_t)L'z');

	/* Check that mbrtoc16() doesn't read ahead too aggressively. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "AB", 2, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%04"PRIx16" L'A'=U+%04"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "C", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'C', "c16=U+%04"PRIx16" L'C'=U+%04"PRIx16,
	    (uint16_t)c16, (uint16_t)L'C');

	/* Incomplete character sequence (shift sequence only). */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x1b(J", 3, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);

	/* Same as above, but complete (U+00A5 YEN SIGN). */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x1b(J\x5c", 4, &s)), 4,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xa5, "c16=U+%04"PRIx16, (uint16_t)c16);

	/* Test restarting behaviour. */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x1b(", 2, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "J\x5c", 2, &s)), 2, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xa5, "c16=U+%04"PRIx16, (uint16_t)c16);

	/*
	 * Test shift sequence state in various increments:
	 * 1. U+0042 LATIN CAPITAL LETTER A
	 * 2. (shift ISO/IEC 646:JP) U+00A5 YEN SIGN
	 * 3. U+00A5 YEN SIGN
	 * 4. (shift JIS X 0208) U+30A2 KATAKANA LETTER A
	 * 5. U+30A2 KATAKANA LETTER A
	 * 6. (shift to initial state) U+0000 NUL
	 */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A\x1b(J", 4, &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%04"PRIx16, (uint16_t)c16);
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x1b(J", 3, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x5c\x5c", 2, &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x00a5, "c16=U+%04"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x5c\x1b$", 3, &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x00a5, "c16=U+%04"PRIx16, (uint16_t)c16);
	c16 = 0x1234;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x1b", 1, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x1234, "c16=U+%04"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "$B\x25\x22", 4, &s)), 4,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x30a2, "c16=U+%04"PRIx16, (uint16_t)c16);
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x25", 1, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x22\x1b(B\x00", 5, &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x30a2, "c16=U+%04"PRIx16, (uint16_t)c16);
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x1b(", 2, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);
	c16 = 42;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "B\x00", 2, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%04"PRIx16, (uint16_t)c16);
}

ATF_TC_WITHOUT_HEAD(mbrtoc16_iso_8859_1_test);
ATF_TC_BODY(mbrtoc16_iso_8859_1_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.ISO8859-1");

	/* Currency sign. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xa4", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xa4, "c16=U+%"PRIx16, (uint16_t)c16);
}

ATF_TC_WITHOUT_HEAD(mbrtoc16_iso_8859_15_test);
ATF_TC_BODY(mbrtoc16_iso_8859_15_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.ISO8859-15");

	/* Euro sign. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xa4", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x20ac, "c16=U+%"PRIx16, (uint16_t)c16);
}

ATF_TC_WITHOUT_HEAD(mbrtoc16_utf_8_test);
ATF_TC_BODY(mbrtoc16_utf_8_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.UTF-8");

	/* Null wide character, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc16(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 1, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 1, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc16(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A", 1, NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%"PRIx16" L'A'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "A", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'A', "c16=U+%"PRIx16" L'A'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'A');

	/* Incomplete character sequence (zero length). */
	c16 = L'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, L'z', "c16=U+%"PRIx16" L'z'=U+%"PRIx16,
	    (uint16_t)c16, (uint16_t)L'z');

	/* Incomplete character sequence (truncated double-byte). */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xc3", 1, &s)), (size_t)-2,
	    "n=%zu", n);

	/* Same as above, but complete. */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xc3\x84", 2, &s)), 2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xc4, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Test restarting behaviour. */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xc3", 1, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0, "c16=U+%"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xb7", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xf7, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Surrogate pair. */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xf0\x9f\x92\xa9", 4, &s)), 4,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xd83d, "c16=U+%"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xdca9, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Letter e with acute, precomposed. */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xc3\xa9", 2, &s)), 2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0xe9, "c16=U+%"PRIx16, (uint16_t)c16);

	/* Letter e with acute, combined. */
	memset(&s, 0, sizeof(s));
	c16 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\x65\xcc\x81", 3, &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x65, "c16=U+%"PRIx16, (uint16_t)c16);
	ATF_CHECK_EQ_MSG((n = mbrtoc16(&c16, "\xcc\x81", 2, &s)), 2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c16, 0x301, "c16=U+%"PRIx16, (uint16_t)c16);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mbrtoc16_c_locale_test);
	ATF_TP_ADD_TC(tp, mbrtoc16_iso2022jp_locale_test);
	ATF_TP_ADD_TC(tp, mbrtoc16_iso_8859_1_test);
	ATF_TP_ADD_TC(tp, mbrtoc16_iso_8859_15_test);
	ATF_TP_ADD_TC(tp, mbrtoc16_utf_8_test);

	return (atf_no_error());
}
