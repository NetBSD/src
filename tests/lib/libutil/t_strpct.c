/* $NetBSD: t_strpct.c,v 1.2 2025/05/03 07:22:52 rillig Exp $ */

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Roland Illig.
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
__COPYRIGHT("@(#) Copyright (c) 2025\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_strpct.c,v 1.2 2025/05/03 07:22:52 rillig Exp $");

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util.h>

#include <atf-c.h>

static void
check_strspct(const char *file, unsigned line,
    size_t bufsiz, intmax_t num, intmax_t den, size_t digits,
    const char *want)
{
	char buf[128];

	ATF_REQUIRE_MSG(bufsiz < sizeof(buf) - 2, "bufsiz too large");
	memset(buf, '>', sizeof(buf));
	buf[0] = '<';
	buf[sizeof(buf) - 1] = '\0';

	const char *have = strspct(buf + 1, bufsiz, num, den, digits);

	ATF_REQUIRE_MSG(buf[0] == '<',
	    "out-of-bounds write before");
	ATF_REQUIRE_MSG(buf[1 + bufsiz] == '>',
	    "out-of-bounds write after");
	ATF_REQUIRE_MSG(have == buf + 1,
	    "have != buf");
	ATF_CHECK_MSG(bufsiz > 0 ? strcmp(have, want) == 0 : true,
	    "%s:%u: want \"%s\", have \"%s\"",
	    file, line, want, have);
}

#define h_strspct(bufsiz, num, den, digits, want) \
	check_strspct(__FILE__, __LINE__, bufsiz, num, den, digits, want)

static void
check_strpct(const char *file, unsigned line,
    size_t bufsiz, uintmax_t num, uintmax_t den, size_t digits,
    const char *want)
{
	char buf[128];

	ATF_REQUIRE_MSG(bufsiz < sizeof(buf) - 2, "bufsiz too large");
	memset(buf, '>', sizeof(buf));
	buf[0] = '<';
	buf[sizeof(buf) - 1] = '\0';

	const char *have = strpct(buf + 1, bufsiz, num, den, digits);

	ATF_REQUIRE_MSG(buf[0] == '<',
	    "out-of-bounds write before");
	ATF_REQUIRE_MSG(buf[1 + bufsiz] == '>',
	    "out-of-bounds write after");
	ATF_REQUIRE_MSG(have == buf + 1,
	    "have != buf");
	ATF_CHECK_MSG(bufsiz > 0 ? strcmp(have, want) == 0 : true,
	    "%s:%u: want \"%s\", have \"%s\"",
	    file, line, want, have);
}

#define h_strpct(bufsiz, num, den, digits, want) \
	check_strpct(__FILE__, __LINE__, bufsiz, num, den, digits, want)

ATF_TC(strspct);
ATF_TC_HEAD(strspct, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks strspct(3)");
}
ATF_TC_BODY(strspct, tc)
{

	// Very small buffers.
	h_strspct(0, 0, 0, 0, "");
	h_strspct(1, 0, 0, 0, "");

	// Small buffers.
	h_strspct(2, 1, 40, 0, "2");
	h_strspct(3, 1, 40, 0, "2");
	h_strspct(3, 1, 40, 1, "2.");
	h_strspct(4, 1, 40, 1, "2.5");
	h_strspct(4, 8, 40, 1, "20.");
	h_strspct(6, 1,  5, 1, "20.0");
	h_strspct(100, 1, 5, 5, "20.00000");
	h_strspct( 5, 11223344, 100, 10, "1122");
	h_strspct(10, 11223344, 100, 10, "11223344.");
	h_strspct(11, 11223344, 100, 10, "11223344.0");

	// Small buffers with negative numbers.
	h_strspct(1, -1, 40, 0, "");
	h_strspct(2, -1, 40, 0, "-");
	h_strspct(3, -1, 40, 0, "-2");
	h_strspct(3, -1, 40, 1, "-2");
	h_strspct(4, -1, 40, 1, "-2.");
	h_strspct(5, -1, 40, 1, "-2.5");
	h_strspct(4, -8, 40, 1, "-20");
	h_strspct(5, -8, 40, 1, "-20.");
	h_strspct(6, -1,  5, 1, "-20.0");
	h_strspct(100, -1, 5, 5, "-20.00000");
	h_strspct( 5, -11223344, 100, 10, "-112");
	h_strspct(10, -11223344, 100, 10, "-11223344");
	h_strspct(11, -11223344, 100, 10, "-11223344.");
	h_strspct(12, -11223344, 100, 10, "-11223344.0");

	// Percentages are always rounded towards zero.
	h_strspct(6, 1, 6, 1, "16.6");
	h_strspct(7, -1, 6, 1, "-16.6");
	h_strspct(7, 1, -6, 1, "-16.6");
	h_strspct(7, -1, -6, 1, "16.6");
	h_strspct(100, 1, 7, 20, "14.28571428571428571428");

	// Big numbers.
	h_strspct(100, INTMAX_MAX, INTMAX_MAX,  0, "100");
	h_strspct(100, INTMAX_MIN, INTMAX_MIN, 25, "100.0000000000000000000000000");
	h_strspct(100, INTMAX_MIN, INTMAX_MAX, 25, "-100.0000000000000000108420217");
	h_strspct(100, INTMAX_MAX, INTMAX_MIN, 25, "-99.9999999999999999891579782");
	h_strspct(100, INTMAX_MAX, INTMAX_MAX, 25, "100.0000000000000000000000000");
}

ATF_TC(strpct);
ATF_TC_HEAD(strpct, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks strpct(3)");
}
ATF_TC_BODY(strpct, tc)
{

	// Small buffers.
	h_strpct(0, 0, 0, 0, "");
	h_strpct(1, 0, 0, 0, "");
	h_strpct(2, 0, 0, 0, "0");
	h_strpct(3, 0, 0, 0, "0");
	h_strpct(3, 0, 0, 1, "0.");
	h_strpct(4, 0, 0, 1, "0.0");
	h_strpct(4, 1, 5, 1, "20.");
	h_strpct(6, 1, 5, 1, "20.0");
	h_strpct(100, 1, 5, 5, "20.00000");

	h_strpct(100, 1, 7, 20, "14.28571428571428571428");

	h_strpct( 5, 11223344, 100, 10, "1122");
	h_strpct(10, 11223344, 100, 10, "11223344.");
	h_strpct(11, 11223344, 100, 10, "11223344.0");

	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX,  0, "100");
	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX,  1, "100.0");
	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX,  5, "100.00000");
	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX, 10, "100.0000000000");
	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX, 15, "100.000000000000000");
	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX, 20, "100.00000000000000000000");
	h_strpct(100, UINTMAX_MAX, UINTMAX_MAX, 25, "100.0000000000000000000000000");

	h_strpct(100, UINTMAX_MAX - 1, UINTMAX_MAX, 25, "99.9999999999999999945789891");
	h_strpct(100, 1, (UINTMAX_MAX >> 1) + 1, 70,
	    "0.0000000000000000108420217248550443400745280086994171142578125000000000");
	h_strpct(100, UINTMAX_MAX, 1, 10, "1844674407370955161500.0000000000");
	h_strpct(100, 1, UINTMAX_MAX, 30, "0.000000000000000005421010862427");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, strspct);
	ATF_TP_ADD_TC(tp, strpct);

	return atf_no_error();
}
