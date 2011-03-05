/* $NetBSD: t_ldexp.c,v 1.3.2.1 2011/03/05 15:10:55 bouyer Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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

#include <atf-c.h>
#include <atf-c/config.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SKIP	9999
#define FORMAT  "%23.23lg"

struct ldexp_test {
	double x;
	int exp1;
	int exp2;
	const char *result;
};

struct ldexp_test basics[] = {
	{ 1.0,	5,	SKIP,	"                     32" },
	{ 1.0,	1022,	SKIP,	"4.4942328371557897693233e+307" },
	{ 1.0,	1023,	-1,	"4.4942328371557897693233e+307" },
	{ 1.0,	1023,	SKIP,	"8.9884656743115795386465e+307" },
	{ 1.0,	1022,	1,	"8.9884656743115795386465e+307" },
	{ 1.0,	-1022,	2045,	"8.9884656743115795386465e+307" },
	{ 1.0,	-5,	SKIP,	"                0.03125" },
	{ 1.0,	-1021,	SKIP,	"4.4501477170144027661805e-308" },
	{ 1.0,	-1022,	1,	"4.4501477170144027661805e-308" },
	{ 1.0,	-1022,	SKIP,	"2.2250738585072013830902e-308" },
	{ 1.0,	-1021,	-1,	"2.2250738585072013830902e-308" },
	{ 1.0,	1023,	-2045,	"2.2250738585072013830902e-308" },
	{ 1.0,	1023,	-1023,	"                      1" },
	{ 1.0,	-1022,	1022,	"                      1" },
	{ 0,	0,	0,	NULL }
};

struct ldexp_test zero[] = {
	{ 0.0,	-1,	SKIP,	"                      0" },
	{ 0.0,	0,	SKIP,	"                      0" },
	{ 0.0,	1,	SKIP,	"                      0" },
	{ 0.0,	1024,	SKIP,	"                      0" },
	{ 0.0,	1025,	SKIP,	"                      0" },
	{ 0.0,	-1023,	SKIP,	"                      0" },
	{ 0.0,	-1024,	SKIP,	"                      0" },
	{ 0,	0,	0,	NULL }
};

struct ldexp_test infinity[] = {
	{ 1.0,	1024,	-1,	"                    inf" },
	{ 1.0,	1024,	0,	"                    inf" },
	{ 1.0,	1024,	1,	"                    inf" },
	{ -1.0,	1024,	-1,	"                   -inf" },
	{ -1.0,	1024,	0,	"                   -inf" },
	{ -1.0,	1024,	1,	"                   -inf" },
	{ 0,	0,	0,	NULL }
};

struct ldexp_test overflow[] = {
	{ 1.0,	1024,	SKIP,	"                    inf" },
	{ 1.0,	1023,	1,	"                    inf" },
	{ 1.0,	-1022,	2046,	"                    inf" },
	{ 1.0,	1025,	SKIP,	"                    inf" },
	{ -1.0,	1024,	SKIP,	"                   -inf" },
	{ -1.0,	1023,	1,	"                   -inf" },
	{ -1.0,	-1022,	2046,	"                   -inf" },
	{ -1.0,	1025,	SKIP,	"                   -inf" },
	{ 0,	0,	0,	NULL }
};

struct ldexp_test denormal[] = {
	{ 1.0,	-1023,	SKIP,	"1.1125369292536006915451e-308" },
	{ 1.0,	-1022,	-1,	"1.1125369292536006915451e-308" },
	{ 1.0,	1023,	-2046,	"1.1125369292536006915451e-308" },
	{ 1.0,	-1024,	SKIP,	"5.5626846462680034577256e-309" },
	{ 1.0,	-1074,	SKIP,	"4.9406564584124654417657e-324" },
	{ -1.0,	-1023,	SKIP,	"-1.1125369292536006915451e-308" },
	{ -1.0,	-1022,	-1,	"-1.1125369292536006915451e-308" },
	{ -1.0,	1023,	-2046,	"-1.1125369292536006915451e-308" },
	{ -1.0,	-1024,	SKIP,	"-5.5626846462680034577256e-309" },
	{ -1.0,	-1074,	SKIP,	"-4.9406564584124654417657e-324" },
	{ 0,	0,	0,	NULL }
};

struct ldexp_test underflow[] = {
	{ 1.0,	-1075,	SKIP,	"                      0" },
	{ 1.0,	-1074,	-1,	"                      0" },
	{ 1.0,	1023,	-2098,	"                      0" },
	{ 1.0,	-1076,	SKIP,	"                      0" },
	{ -1.0,	-1075,	SKIP,	"                     -0" },
	{ -1.0,	-1074,	-1,	"                     -0" },
	{ -1.0,	1023,	-2098,	"                     -0" },
	{ -1.0,	-1076,	SKIP,	"                     -0" },
	{ 0,	0,	0,	NULL }
};

struct ldexp_test denormal_large_exp[] = {
	{ 1.0,	-1028,	1024,	"                 0.0625" },
	{ 1.0,	-1028,	1025,	"                  0.125" },
	{ 1.0,	-1028,	1026,	"                   0.25" },
	{ 1.0,	-1028,	1027,	"                    0.5" },
	{ 1.0,	-1028,	1028,	"                      1" },
	{ 1.0,	-1028,	1029,	"                      2" },
	{ 1.0,	-1028,	1030,	"                      4" },
	{ 1.0,	-1028,	1040,	"                   4096" },
	{ 1.0,	-1028,	1050,	"                4194304" },
	{ 1.0,	-1028,	1060,	"             4294967296" },
	{ 1.0,	-1028,	1100,	" 4722366482869645213696" },
	{ 1.0,	-1028,	1200,	"5.9863107065073783529623e+51" },
	{ 1.0,	-1028,	1300,	"7.5885503602567541832791e+81" },
	{ 1.0,	-1028,	1400,	"9.6196304190416209014353e+111" },
	{ 1.0,	-1028,	1500,	"1.2194330274671844653834e+142" },
	{ 1.0,	-1028,	1600,	"1.5458150092069033378781e+172" },
	{ 1.0,	-1028,	1700,	"1.9595533242629369747791e+202" },
	{ 1.0,	-1028,	1800,	"2.4840289476811342962384e+232" },
	{ 1.0,	-1028,	1900,	"3.1488807865122869393369e+262" },
	{ 1.0,	-1028,	2000,	"3.9916806190694396233127e+292" },
	{ 1.0,	-1028,	2046,	"2.808895523222368605827e+306" },
	{ 1.0,	-1028,	2047,	"5.6177910464447372116541e+306" },
	{ 1.0,	-1028,	2048,	"1.1235582092889474423308e+307" },
	{ 1.0,	-1028,	2049,	"2.2471164185778948846616e+307" },
	{ 1.0,	-1028,	2050,	"4.4942328371557897693233e+307" },
	{ 1.0,	-1028,	2051,	"8.9884656743115795386465e+307" },
	{ 0,	0,	0,	NULL }
};

static void
run_test(struct ldexp_test *table)
{
	int i;
	double v;
	char outbuf[64];

	for (i = 0; table->result != NULL; table++, i++) {
		v = ldexp(table->x, table->exp1);
		if (table->exp2 != SKIP)
			v = ldexp(v, table->exp2);
			snprintf(outbuf, sizeof(outbuf), FORMAT, v);
			ATF_CHECK_STREQ_MSG(table->result, outbuf,
			    "Entry %d:\n\tExp: \"%s\"\n\tAct: \"%s\"",
			    i, table->result, outbuf);
	}
}

#define TEST(name)							\
	ATF_TC(name);							\
	ATF_TC_HEAD(name, tc)						\
	{								\
									\
		atf_tc_set_md_var(tc, "descr",				\
		    "Test ldexp(3) for " ___STRING(name));		\
	}								\
	ATF_TC_BODY(name, tc)						\
	{								\
		const char *machine;					\
									\
		machine = atf_config_get("atf_machine");		\
		if (strcmp("vax", machine) == 0)			\
			atf_tc_skip("Test not valid for %s", machine);	\
		run_test(name);						\
	}

TEST(basics)
TEST(zero)
TEST(infinity)
TEST(overflow)
TEST(denormal)
TEST(underflow)
TEST(denormal_large_exp)

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, basics);
	ATF_TP_ADD_TC(tp, zero);
	ATF_TP_ADD_TC(tp, infinity);
	ATF_TP_ADD_TC(tp, overflow);
	ATF_TP_ADD_TC(tp, denormal);
	ATF_TP_ADD_TC(tp, underflow);
	ATF_TP_ADD_TC(tp, denormal_large_exp);

	return atf_no_error();
}
