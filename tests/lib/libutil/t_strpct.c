/* $NetBSD: t_strpct.c,v 1.1 2025/05/02 19:52:02 rillig Exp $ */

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
__RCSID("$NetBSD: t_strpct.c,v 1.1 2025/05/02 19:52:02 rillig Exp $");

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

ATF_TC(strspct);
ATF_TC_HEAD(strspct, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks strspct(3)");
}
ATF_TC_BODY(strspct, tc)
{

	h_strspct(0, 0, 0, 0, "");
	h_strspct(1, 0, 0, 0, "");
	h_strspct(2, 0, 0, 0, "0");
	h_strspct(3, 0, 0, 0, "0");
	h_strspct(3, 0, 0, 1, "0.");
	h_strspct(4, 0, 0, 1, "0.0");
	h_strspct(4, 1, 5, 1, "20.");
	h_strspct(6, 1, 5, 1, "20.0");
	h_strspct(100, 1, 5, 5, "20.00000");

	// Percentages are always rounded towards zero.
	h_strspct(6, 1, 6, 1, "16.6");
	h_strspct(7, -1, 6, 1, "-16.6");
	h_strspct(7, 1, -6, 1, "-16.6");
	h_strspct(7, -1, -6, 1, "16.6");

	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 0, "100");
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 1, "100.0");
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 2, "100.00");
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 3, "100.000");
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 4, "100.0000");
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 5, "100.00000");
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 6, "100.000000");
	// FIXME: must be 100.0000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 7, "100.0000003");
	// FIXME: must be 100.00000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 8, "100.00002208");
	// FIXME: must be 100.000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 9, "100.000781031");
	// FIXME: must be 100.0000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 10, "100.0040337943");
	// FIXME: must be 100.00000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 11, "100.03657306783");
	// FIXME: must be 100.000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 12, "100.254043878856");
	// FIXME: must be 100.0000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 13, "102.4819115206086");
	// FIXME: must be 100.00000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 14, "92.23372036854775");
	// FIXME: must be 100.000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 15, "9.223372036854775");
	// FIXME: must be 100.0000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 16, "0.9223372036854775");
	// FIXME: must be 100.00000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 17, "0.09223372036854775");
	// FIXME: must be 100.000000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 18, "0.09223372036854775");
	// FIXME: must be 100.0000000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 19, "0.09223372036854775");
	// FIXME: must be 100.00000000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 20, "0.09223372036854775");
	// FIXME: must be 100.000000000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 21, "0.09223372036854775");
	// FIXME: must be 100.0000000000000000000000
	h_strspct(100, INTMAX_MAX / 1000, INTMAX_MAX / 1000, 22, "0.09223372036854775");

	// FIXME: must be 100.0
	h_strspct(100, INTMAX_MIN, INTMAX_MIN, 20, "92.23372036854775808");
	// FIXME: must be -100.0
	h_strspct(100, INTMAX_MIN, INTMAX_MAX, 20, "-92.23372036854775808");
	// FIXME: must be -100.0
	h_strspct(100, INTMAX_MAX, INTMAX_MIN, 20, "-92.23372036854775807");
	// FIXME: must be 100.0
	h_strspct(100, INTMAX_MAX, INTMAX_MAX, 20, "92.23372036854775807");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, strspct);

	return atf_no_error();
}
