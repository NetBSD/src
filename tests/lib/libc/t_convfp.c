/*	$NetBSD: t_convfp.c,v 1.1 2010/12/31 04:08:33 pgoyette Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#include <atf-c.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This value is representable as an unsigned int, but not as an int.
 * According to ISO C it must survive the convsion back from a double
 * to an unsigned int (everything > -1 and < UINT_MAX+1 has to)
 */ 
#define	UINT_TESTVALUE	(INT_MAX+42U)

/* The same for unsigned long */
#define ULONG_TESTVALUE	(LONG_MAX+42UL)

static void test1();
static void test2();
static void test3();

ATF_TC(test1);

ATF_TC_HEAD(test1, tc)
{

	atf_tc_set_md_var(tc, "descr", "test conversions to unsigned int/long");
}

ATF_TC_BODY(test1, tc)
{
	unsigned int ui;
	unsigned long ul;
	long double dt;
	double d;

	/* unsigned int test */
	d = UINT_TESTVALUE;
	ui = (unsigned int)d;

	if (ui != UINT_TESTVALUE)
		atf_tc_fail("FAILED: unsigned int %u (0x%x) != %u (0x%x)\n",
		    ui, ui, UINT_TESTVALUE, UINT_TESTVALUE);

	/* unsigned long vs. {long} double test */
	if (sizeof(d) > sizeof(ul)) {
		d = ULONG_TESTVALUE;
		ul = (unsigned long)d;
		printf("testing double vs. long\n");
	} else if (sizeof(dt) > sizeof(ul)) {
		dt = ULONG_TESTVALUE;
		ul = (unsigned long)dt;
		printf("testing long double vs. long\n");
	} else
		atf_tc_skip("no suitable {long} double type found, skipping "
		    "\"unsigned long\" test\n"
		    "sizeof(long) = %d, sizeof(double) = %d, "
		    "sizeof(long double) = %d\n", 
		    sizeof(ul), sizeof(d), sizeof(dt));

	if (ul != ULONG_TESTVALUE) {
		atf_tc_fail("unsigned long %lu (0x%lx) != %lu (0x%lx)\n",
		    ul, ul, ULONG_TESTVALUE, ULONG_TESTVALUE);
		exit(1);
	}
}

ATF_TC(test2);

ATF_TC_HEAD(test2, tc)
{

	atf_tc_set_md_var(tc, "descr", "test double to unsigned long cast");
}

ATF_TC_BODY(test2, tc)
{
	double nv;
	unsigned long uv;

	nv = 5.6;
	uv = (unsigned long)nv;

	ATF_REQUIRE_EQ_MSG(uv, 5,
	    "%.3f casted to unsigned long is %lu\n", nv, uv);
}

ATF_TC(test3);

ATF_TC_HEAD(test3, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "test double/long double casts to unsigned long");
}

ATF_TC_BODY(test3, tc)
{
	double dv = 1.9;
	long double ldv = dv;
	unsigned long l1 = dv;
	unsigned long l2 = ldv;

	ATF_REQUIRE_EQ_MSG(l1, 1,
	    "double 1.9 casted to unsigned long should be 1, but is %lu\n", l1);

	ATF_REQUIRE_EQ_MSG(l2, 1,
	    "long double 1.9 casted to unsigned long should be 1, but is %lu\n",
	    l2);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, test1);
	ATF_TP_ADD_TC(tp, test2);
	ATF_TP_ADD_TC(tp, test3);

	return atf_no_error();
}
