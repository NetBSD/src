/* $NetBSD: t_snprintb.c,v 1.6 2019/04/29 07:55:38 kre Exp $ */

/*
 * Copyright (c) 2002, 2004, 2008, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Christos Zoulas.
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
__RCSID("$NetBSD: t_snprintb.c,v 1.6 2019/04/29 07:55:38 kre Exp $");

#include <string.h>
#include <util.h>

#include <atf-c.h>

static void
h_snprintb(const char *fmt, uint64_t val, const char *res)
{
	char buf[1024];
	int len, slen;

	len = snprintb(buf, sizeof(buf), fmt, val);
	slen = (int) strlen(res);

	ATF_REQUIRE_STREQ(res, buf);
	ATF_REQUIRE_EQ(len, slen);
}

ATF_TC(snprintb);
ATF_TC_HEAD(snprintb, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb(3)");
}
ATF_TC_BODY(snprintb, tc)
{
	h_snprintb("\10\2BITTWO\1BITONE", 3, "03<BITTWO,BITONE>");
	h_snprintb("\177\20b\0A\0\0", 0, "0x0");

	h_snprintb("\177\20b\05NOTBOOT\0b\06FPP\0b\013SDVMA\0b\015VIDEO\0"
		"b\020LORES\0b\021FPA\0b\022DIAG\0b\016CACHE\0"
		"b\017IOCACHE\0b\022LOOPBACK\0b\04DBGCACHE\0",
		0xe860, "0xe860<NOTBOOT,FPP,SDVMA,VIDEO,CACHE,IOCACHE>");

	h_snprintb("\177\20f\0\4FOO\0=\1ONE\0=\2TWO\0\0", 1,
	    "0x1<FOO=0x1=ONE>");
	h_snprintb("\177\20f\0\4X\0=\1ONE\0=\2TWO\0\0", 1,
	    "0x1<X=0x1=ONE>");
	h_snprintb("\177\020F\0\4\0:\1ONE\0:\2TWO\0\0", 1,
	    "0x1<ONE>");

	h_snprintb("\177\20f\0\4FOO\0=\1ONE\0=\2TWO\0\0", 2,
	    "0x2<FOO=0x2=TWO>");
	h_snprintb("\177\20f\0\4X\0=\1ONE\0=\2TWO\0\0", 2,
	    "0x2<X=0x2=TWO>");
	h_snprintb("\177\020F\0\4\0:\1ONE\0:\2TWO\0\0", 2,
	    "0x2<TWO>");

	h_snprintb("\177\20f\0\4FOO\0=\1ONE\0=\2TWO\0*=OTHER\0\0", 3,
	    "0x3<FOO=0x3=OTHER>");
	h_snprintb("\177\20f\0\4X\0=\1ONE\0=\2TWO\0*=Other(%d)\0\0", 3,
	    "0x3<X=0x3=Other(3)>");
	h_snprintb("\177\20f\0\x8X\0=\1ONE\0=\2TWO\0*=other(%o)\0\0", 0x20,
	    "0x20<X=0x20=other(40)>");
	h_snprintb("\177\020F\0\4\0:\1ONE\0:\2TWO\0\0", 3,
	    "0x3<>");

	h_snprintb("\177\20f\0\4Field_1\0=\1ONE\0=\2TWO\0"
		   "f\4\4Field_2\0=\1ONE\0=\2TWO\0\0", 0x12,
	    "0x12<Field_1=0x2=TWO,Field_2=0x1=ONE>");

	h_snprintb("\177\20f\0\4Field_1\0=\1ONE\0=\2TWO\0"
		   "F\x8\4\0*Field_3=%d\0"
		   "f\4\4Field_2\0:\1:ONE\0:\2:TWO\0\0", 0xD12,
	    "0xd12<Field_1=0x2=TWO,Field_3=13,Field_2=0x1:ONE>");
}

static void
h_snprintb_m(const char *fmt, uint64_t val, int line_max, const char *res,
	     int res_len)
{
	char buf[1024];
	int len;

	len = snprintb_m(buf, sizeof(buf), fmt, val, line_max);

	ATF_REQUIRE_EQ(len, res_len);
	ATF_REQUIRE_EQ(0, memcmp(res, buf, res_len + 1));
}

ATF_TC(snprintb_m);
ATF_TC_HEAD(snprintb_m, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb_m(3)");
}
ATF_TC_BODY(snprintb_m, tc)
{
	h_snprintb_m("\177\020b\0LSB\0b\1_BITONE\0f\4\4NIBBLE2\0"
			"f\x10\4BURST\0=\4FOUR\0=\xfSIXTEEN\0"
			"b\x1fMSB\0\0",
                     0x800f0701,
		     33,
		     "0x800f0701<LSB,NIBBLE2=0x0>\0"
			"0x800f0701<BURST=0xf=SIXTEEN,MSB>\0\0",
		     62);

	h_snprintb_m("\177\020b\0LSB\0b\1_BITONE\0f\4\4NIBBLE2\0"
			"f\x10\4BURST\0=\4FOUR\0=\xfSIXTEEN\0"
			"b\x1fMSB\0\0",
                     0x800f0701,
		     32,
		     "0x800f0701<LSB,NIBBLE2=0x0>\0"
			"0x800f0701<BURST=0xf=SIXTEEN>\0"
			"0x800f0701<MSB>\0\0",
		     74);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, snprintb);
	ATF_TP_ADD_TC(tp, snprintb_m);

	return atf_no_error();
}
