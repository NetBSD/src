/*	$NetBSD: t_mbrtoc8.c,v 1.1 2024/08/15 21:19:45 riastradh Exp $	*/

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
 * Test program for mbrtoc8() as specified by C23.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_mbrtoc8.c,v 1.1 2024/08/15 21:19:45 riastradh Exp $");

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
static char8_t c8;

ATF_TC_WITHOUT_HEAD(mbrtoc8_c_locale_test);
ATF_TC_BODY(mbrtoc8_c_locale_test, tc)
{
	size_t n;

	require_lc_ctype("C");

	/* Null wide character, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 1, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 1, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc8(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "A", 1, NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'A', "c8=0x%"PRIx8" 'A'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'A');

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "A", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'A', "c8=0x%"PRIx8" 'A'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'A');

	/* Incomplete character sequence. */
	c8 = 'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'z', "c8=0x%"PRIx8" 'z'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'z');

	/* Check that mbrtoc8() doesn't access the buffer when n == 0. */
	c8 = 'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'z', "c8=0x%"PRIx8" 'z'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'z');

	/* Check that mbrtoc8() doesn't read ahead too aggressively. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "AB", 2, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'A', "c8=0x%"PRIx8" 'A'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'A');
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "C", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'C', "c8=0x%"PRIx8" 'C'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'C');

}

ATF_TC_WITHOUT_HEAD(mbrtoc8_iso_8859_1_test);
ATF_TC_BODY(mbrtoc8_iso_8859_1_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.ISO8859-1");

	/* Currency sign. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xa4", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xc2, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xa4, "c8=0x%"PRIx8, (uint8_t)c8);
}

ATF_TC_WITHOUT_HEAD(mbrtoc8_iso_8859_15_test);
ATF_TC_BODY(mbrtoc8_iso_8859_15_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.ISO8859-15");

	/* Euro sign. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xa4", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xe2, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0x82, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xac, "c8=0x%"PRIx8, (uint8_t)c8);
}

ATF_TC_WITHOUT_HEAD(mbrtoc8_utf_8_test);
ATF_TC_BODY(mbrtoc8_utf_8_test, tc)
{
	size_t n;

	require_lc_ctype("en_US.UTF-8");

	/* Null wide character, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc8(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 1, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 1, &s)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Latin letter A, internal state. */
	ATF_CHECK_EQ_MSG((n = mbrtoc8(NULL, 0, 0, NULL)), 0, "n=%zu", n);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "A", 1, NULL)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'A', "c8=0x%"PRIx8" 'A'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'A');

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "A", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'A', "c8=0x%"PRIx8" 'A'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'A');

	/* Incomplete character sequence (zero length). */
	c8 = 'z';
	memset(&s, 0, sizeof(s));
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 'z', "c8=0x%"PRIx8" 'z'=0x%"PRIx8,
	    (uint8_t)c8, (uint8_t)'z');

	/* Incomplete character sequence (truncated double-byte). */
	memset(&s, 0, sizeof(s));
	c8 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xc3", 1, &s)), (size_t)-2,
	    "n=%zu", n);

	/* Same as above, but complete. */
	memset(&s, 0, sizeof(s));
	c8 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xc3\x84", 2, &s)), 2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xc3, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0x84, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Test restarting behaviour. */
	memset(&s, 0, sizeof(s));
	c8 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xc3", 1, &s)), (size_t)-2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xb7", 1, &s)), 1, "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xc3, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xb7, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Four-byte sequence. */
	memset(&s, 0, sizeof(s));
	c8 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xf0\x9f\x92\xa9", 4, &s)), 4,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xf0, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0x9f, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0x92, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xa9, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Letter e with acute, precomposed. */
	memset(&s, 0, sizeof(s));
	c8 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xc3\xa9", 2, &s)), 2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xc3, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xa9, "c8=0x%"PRIx8, (uint8_t)c8);

	/* Letter e with acute, combined. */
	memset(&s, 0, sizeof(s));
	c8 = 0;
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\x65\xcc\x81", 3, &s)), 1,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0x65, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "\xcc\x81", 2, &s)), 2,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0xcc, "c8=0x%"PRIx8, (uint8_t)c8);
	ATF_CHECK_EQ_MSG((n = mbrtoc8(&c8, "", 0, &s)), (size_t)-3,
	    "n=%zu", n);
	ATF_CHECK_EQ_MSG(c8, 0x81, "c8=0x%"PRIx8, (uint8_t)c8);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mbrtoc8_c_locale_test);
	ATF_TP_ADD_TC(tp, mbrtoc8_iso_8859_1_test);
	ATF_TP_ADD_TC(tp, mbrtoc8_iso_8859_15_test);
	ATF_TP_ADD_TC(tp, mbrtoc8_utf_8_test);

	return (atf_no_error());
}
