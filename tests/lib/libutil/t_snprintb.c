/* $NetBSD: t_snprintb.c,v 1.9 2024/01/25 18:13:14 rillig Exp $ */

/*
 * Copyright (c) 2002, 2004, 2008, 2010, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Christos Zoulas and
 * Roland Illig.
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
__COPYRIGHT("@(#) Copyright (c) 2008, 2010\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_snprintb.c,v 1.9 2024/01/25 18:13:14 rillig Exp $");

#include <stdio.h>
#include <string.h>
#include <util.h>
#include <vis.h>

#include <atf-c.h>

static const char *
vis_fmt(const char *fmt, size_t fmtlen)
{
	static char buf[4][1024];
	static size_t i;

	i = (i + 1) % 4;
	ATF_REQUIRE_MSG(
	    strnvisx(buf[i], sizeof(buf[i]), fmt, fmtlen, VIS_WHITE | VIS_OCTAL) >= 0,
	    "strnvisx failed for length %zu", fmtlen);
	return buf[i];
}

static void
h_snprintb(const char *file, size_t line, const char *fmt, size_t fmtlen, uint64_t val,
    const char *res, size_t reslen)
{
	char buf[1024];

	int rv = snprintb(buf, sizeof(buf), fmt, val);

	ATF_REQUIRE_MSG(rv > 0, "formatting %jx with '%s' returns error %d",
	    (uintmax_t)val, vis_fmt(fmt, fmtlen), rv);

	size_t buflen = rv;
	ATF_CHECK_MSG(
	    buflen == reslen && memcmp(buf, res, reslen) == 0,
	    "failed:\n"
	    "\ttest case: %s:%zu\n"
	    "\tformat: %s\n"
	    "\tvalue: %#jx\n"
	    "\twant: %3zu bytes %s\n"
	    "\thave: %3zu bytes %s\n",
	    file, line,
	    vis_fmt(fmt, fmtlen), (uintmax_t)val,
	    reslen, vis_fmt(res, reslen),
	    buflen, vis_fmt(buf, buflen));
}

#define	h_snprintb(fmt, val, res)					\
	h_snprintb(__FILE__, __LINE__, fmt, sizeof(fmt) - 1,		\
	    val, res, sizeof(res) - 1)

ATF_TC(snprintb);
ATF_TC_HEAD(snprintb, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb(3)");
}
ATF_TC_BODY(snprintb, tc)
{
	h_snprintb(
	    "\010"
	    "\002BITTWO"
	    "\001BITONE",
	    3,
	    "03<BITTWO,BITONE>");

	h_snprintb(
	    "\177\20"
	    "b\0A\0",
	    0,
	    "0");

	h_snprintb(
	    "\177\020"
	    "b\05NOTBOOT\0"
	    "b\06FPP\0"
	    "b\13SDVMA\0"
	    "b\15VIDEO\0"
	    "b\20LORES\0"
	    "b\21FPA\0"
	    "b\22DIAG\0"
	    "b\16CACHE\0"
	    "b\17IOCACHE\0"
	    "b\22LOOPBACK\0"
	    "b\04DBGCACHE\0",
	    0xe860,
	    "0xe860<NOTBOOT,FPP,SDVMA,VIDEO,CACHE,IOCACHE>");

	h_snprintb(
	    "\177\20"
	    "f\0\4FOO\0"
		"=\1ONE\0"
		"=\2TWO\0",
	    1,
	    "0x1<FOO=0x1=ONE>");
	h_snprintb(
	    "\177\20"
	    "f\0\4X\0"
		"=\1ONE\0"
		"=\2TWO\0",
	    1,
	    "0x1<X=0x1=ONE>");
	h_snprintb(
	    "\177\20"
	    "F\0\4\0"
		":\1ONE\0"
		":\2TWO\0",
	    1,
	    "0x1<ONE>");

	h_snprintb(
	    "\177\20"
	    "f\0\4FOO\0"
		"=\1ONE\0"
		"=\2TWO\0",
	    2,
	    "0x2<FOO=0x2=TWO>");
	h_snprintb(
	    "\177\20"
	    "f\0\4X\0"
		"=\1ONE\0"
		"=\2TWO\0",
	    2,
	    "0x2<X=0x2=TWO>");
	h_snprintb(
	    "\177\020"
	    "F\0\4\0"
		":\1ONE\0"
		":\2TWO\0",
	    2,
	    "0x2<TWO>");

	h_snprintb(
	    "\177\20"
	    "f\0\4FOO\0"
		"=\1ONE\0"
		"=\2TWO\0"
		"*=OTHER\0",
	    3,
	    "0x3<FOO=0x3=OTHER>");
	h_snprintb(
	    "\177\20"
	    "f\0\4X\0"
		"=\1ONE\0"
		"=\2TWO\0"
		"*=Other(%jd)\0",
	    3,
	    "0x3<X=0x3=Other(3)>");
	h_snprintb(
	    "\177\20"
	    "f\0\x8X\0"
		"=\1ONE\0"
		"=\2TWO\0"
		"*=other(%jo)\0",
	    0x20,
	    "0x20<X=0x20=other(40)>");
	h_snprintb(
	    "\177\020"
	    "F\0\4\0"
		":\1ONE\0"
		":\2TWO\0",
	    3,
	    "0x3<>");

	h_snprintb(
	    "\177\20"
	    "f\0\4Field_1\0"
		"=\1ONE\0"
		"=\2TWO\0"
	    "f\4\4Field_2\0"
		"=\1ONE\0"
		"=\2TWO\0",
	    0x12,
	    "0x12<Field_1=0x2=TWO,Field_2=0x1=ONE>");

	h_snprintb(
	    "\177\20"
	    "f\0\4Field_1\0"
		"=\1ONE\0"
		"=\2TWO\0"
	    "F\x8\4\0"
		"*Field_3=%jd\0"
	    "f\4\4Field_2\0"
		":\1:ONE\0"
		":\2:TWO\0",
	    0xD12,
	    "0xd12<Field_1=0x2=TWO,Field_3=13,Field_2=0x1:ONE>");

	// It is possible but cumbersome to implement a reduced variant of
	// rot13 using snprintb, shown here for lowercase letters only.
	for (char ch = 'A'; ch <= '~'; ch++) {
		char rot13 = ch >= 'a' && ch <= 'm' ? ch + 13
		    : ch >= 'n' && ch <= 'z' ? ch - 13
		    : '?';
		char expected[8];
		ATF_REQUIRE_EQ(7,
		    snprintf(expected, sizeof(expected), "%#x<%c>", ch, rot13));
		h_snprintb(
		    "\177\020"
		    "F\000\010\0"
		    ":an\0:bo\0:cp\0:dq\0:er\0:fs\0:gt\0:hu\0"
		    ":iv\0:jw\0:kx\0:ly\0:mz\0"
		    ":na\0:ob\0:pc\0:qd\0:re\0:sf\0:tg\0:uh\0"
		    ":vi\0:wj\0:xk\0:yl\0:zm\0"
		    // If snprintf accepted "%jc", it would be possible to
		    // echo the non-alphabetic characters instead of a
		    // catchall question mark.
		    "*?\0",
		    ch,
		    expected);
	}
}

static void
h_snprintb_m(const char *file, size_t line, const char *fmt, size_t fmtlen, uint64_t val, int line_max,
    const char *res, size_t reslen)
{
	char buf[1024];

	int rv = snprintb_m(buf, sizeof(buf), fmt, val, line_max);

	ATF_REQUIRE_MSG(rv >= 0, "formatting %jx with '%s' returns error %d",
	    (uintmax_t)val, vis_fmt(fmt, fmtlen), rv);

	size_t buflen = rv;
	ATF_CHECK_MSG(
	    buflen == reslen && memcmp(buf, res, reslen) == 0,
	    "failed:\n"
	    "\ttest case: %s:%zu\n"
	    "\tformat: %s\n"
	    "\tvalue: %#jx\n"
	    "\twant: %zu bytes %s\n"
	    "\thave: %zu bytes %s\n",
	    file, line,
	    vis_fmt(fmt, fmtlen), (uintmax_t)val,
	    reslen, vis_fmt(res, reslen),
	    buflen, vis_fmt(buf, buflen));
}

#define	h_snprintb_m(fmt, val, line_max, res)				\
	h_snprintb_m(__FILE__, __LINE__, fmt, sizeof(fmt) - 1,		\
	    val, line_max, res, sizeof(res) - 1)

ATF_TC(snprintb_m);
ATF_TC_HEAD(snprintb_m, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb_m(3)");
}
ATF_TC_BODY(snprintb_m, tc)
{
	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1_BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0"
		"=\04FOUR\0"
		"=\17FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    33,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN,MSB>\0");

	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1_BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0"
		"=\04FOUR\0"
		"=\17FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    32,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN>\0"
	    "0x800f0701<MSB>\0");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, snprintb);
	ATF_TP_ADD_TC(tp, snprintb_m);

	return atf_no_error();
}
