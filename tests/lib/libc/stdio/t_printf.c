/* $NetBSD: t_printf.c,v 1.10 2023/04/04 19:39:38 he Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/resource.h>
#include <atf-c.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

ATF_TC(snprintf_c99);
ATF_TC_HEAD(snprintf_c99, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Test printf(3) C99 conformance (PR lib/22019)");
}

ATF_TC_BODY(snprintf_c99, tc)
{
	char s[4];

	(void)memset(s, '\0', sizeof(s));
	(void)snprintf(s, sizeof(s), "%#.o", 0);
	(void)printf("printf = %#.o\n", 0);
	(void)fprintf(stderr, "snprintf = %s", s);

	ATF_REQUIRE(strlen(s) == 1);
	ATF_REQUIRE(s[0] == '0');
}

ATF_TC(snprintf_dotzero);
ATF_TC_HEAD(snprintf_dotzero, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "PR lib/32951: %%.0f formats (0.0,0.5] to \"0.\"");
}

ATF_TC_BODY(snprintf_dotzero, tc)
{
	char s[4];

	ATF_CHECK(snprintf(s, sizeof(s), "%.0f", 0.1) == 1);
	ATF_REQUIRE_STREQ(s, "0");
}

ATF_TC(snprintf_posarg);
ATF_TC_HEAD(snprintf_posarg, tc)
{

	atf_tc_set_md_var(tc, "descr", "test for positional arguments");
}

ATF_TC_BODY(snprintf_posarg, tc)
{
	char s[16];

	ATF_CHECK(snprintf(s, sizeof(s), "%1$d", -23) == 3);
	ATF_REQUIRE_STREQ(s, "-23");
}

ATF_TC(snprintf_posarg_width);
ATF_TC_HEAD(snprintf_posarg_width, tc)
{

	atf_tc_set_md_var(tc, "descr", "test for positional arguments with "
	    "field width");
}

ATF_TC_BODY(snprintf_posarg_width, tc)
{
	char s[16];

	ATF_CHECK(snprintf(s, sizeof(s), "%1$*2$d", -23, 4) == 4);
	ATF_REQUIRE_STREQ(s, " -23");
}

ATF_TC(snprintf_posarg_error);
ATF_TC_HEAD(snprintf_posarg_error, tc)
{

	atf_tc_set_md_var(tc, "descr", "test for positional arguments out "
	    "of bounds");
}

ATF_TC_BODY(snprintf_posarg_error, tc)
{
	char s[16], fmt[32];

	snprintf(fmt, sizeof(fmt), "%%%zu$d", SIZE_MAX / sizeof(size_t));

	ATF_CHECK(snprintf(s, sizeof(s), fmt, -23) == -1);
}

ATF_TC(snprintf_float);
ATF_TC_HEAD(snprintf_float, tc)
{

	atf_tc_set_md_var(tc, "descr", "test that floating conversions don't"
	    " leak memory");
}

ATF_TC_BODY(snprintf_float, tc)
{
	union {
		double d;
		uint64_t bits;
	} u;
	uint32_t ul, uh;
	time_t now;
	char buf[1000];
	struct rlimit rl;

	rl.rlim_cur = rl.rlim_max = 1 * 1024 * 1024;
	ATF_CHECK(setrlimit(RLIMIT_AS, &rl) != -1);
	rl.rlim_cur = rl.rlim_max = 1 * 1024 * 1024;
	ATF_CHECK(setrlimit(RLIMIT_DATA, &rl) != -1);

	time(&now);
	srand(now);
	for (size_t i = 0; i < 10000; i++) {
		ul = rand();
		uh = rand();
		u.bits = (uint64_t)uh << 32 | ul;
		ATF_CHECK(snprintf(buf, sizeof buf, " %.2f", u.d) != -1);
	}
}

ATF_TC(sprintf_zeropad);
ATF_TC_HEAD(sprintf_zeropad, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test output format zero padding (PR lib/44113)");
}

ATF_TC_BODY(sprintf_zeropad, tc)
{
	char str[1024];

	ATF_CHECK(sprintf(str, "%010f", 0.0) == 10);
	ATF_REQUIRE_STREQ(str, "000.000000");

	/* ieeefp */
#ifndef __vax__
	/* printf(3) should ignore zero padding for nan/inf */
	ATF_CHECK(sprintf(str, "%010f", NAN) == 10);
	ATF_REQUIRE_STREQ(str, "       nan");
	ATF_CHECK(sprintf(str, "%010f", INFINITY) == 10);
	ATF_REQUIRE_STREQ(str, "       inf");
#endif
}

ATF_TC(snprintf_double_a);
ATF_TC_HEAD(snprintf_double_a, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test printf a format");
}

ATF_TC_BODY(snprintf_double_a, tc)
{
	char buf[1000];

	snprintf(buf, sizeof buf, "%.3a", (double)10.6);
	ATF_REQUIRE_STREQ("0x1.533p+3", buf);
}

/* is "long double" and "double" different? */
#if (__LDBL_MANT_DIG__ != __DBL_MANT_DIG__) || \
    (__LDBL_MAX_EXP__ != __DBL_MAX_EXP__)
#define WIDE_DOUBLE
#endif

#ifndef WIDE_DOUBLE
ATF_TC(pr57250_fix);
ATF_TC_HEAD(pr57250_fix, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test for PR57250");
}

ATF_TC_BODY(pr57250_fix, tc)
{
	char *eptr;
	char buf[1000];
	long double ld;

	errno = 0;
	ld = strtold("1e309", &eptr);
	ATF_CHECK(errno != 0);
	ld = (double)ld;
	ATF_CHECK(isfinite(ld) == 0);
	snprintf(buf, sizeof buf, "%Lf\n", ld);
	ATF_REQUIRE_STREQ(buf, "inf\n");
}
#endif


ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, snprintf_c99);
	ATF_TP_ADD_TC(tp, snprintf_dotzero);
	ATF_TP_ADD_TC(tp, snprintf_posarg);
	ATF_TP_ADD_TC(tp, snprintf_posarg_width);
	ATF_TP_ADD_TC(tp, snprintf_posarg_error);
	ATF_TP_ADD_TC(tp, snprintf_float);
	ATF_TP_ADD_TC(tp, sprintf_zeropad);
	ATF_TP_ADD_TC(tp, snprintf_double_a);
#ifndef WIDE_DOUBLE
	ATF_TP_ADD_TC(tp, pr57250_fix);
#endif

	return atf_no_error();
}
