/* $NetBSD: t_ceil.c,v 1.5 2011/09/12 16:48:48 jruoho Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
__RCSID("$NetBSD: t_ceil.c,v 1.5 2011/09/12 16:48:48 jruoho Exp $");

#include <atf-c.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>

#ifdef __vax__
#define SMALL_NUM	1.0e-38
#else
#define SMALL_NUM	1.0e-40
#endif

ATF_TC(ceil_basic);
ATF_TC_HEAD(ceil_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of ceil(3)");
}

ATF_TC_BODY(ceil_basic, tc)
{
	const double x = 0.999999999999999;
	const double y = 0.000000000000001;

	ATF_CHECK(fabs(ceil(x) - 1) < SMALL_NUM);
	ATF_CHECK(fabs(ceil(y) - 1) < SMALL_NUM);
}

ATF_TC(ceilf_basic);
ATF_TC_HEAD(ceilf_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of ceilf(3)");
}

ATF_TC_BODY(ceilf_basic, tc)
{
	const float x = 0.9999999;
	const float y = 0.0000001;

	ATF_CHECK(fabsf(ceilf(x) - 1) < SMALL_NUM);
	ATF_CHECK(fabsf(ceilf(y) - 1) < SMALL_NUM);
}

ATF_TC(floor_basic);
ATF_TC_HEAD(floor_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of floor(3)");
}

ATF_TC_BODY(floor_basic, tc)
{
	const double x = 0.999999999999999;
	const double y = 0.000000000000001;

	ATF_CHECK(floor(x) < SMALL_NUM);
	ATF_CHECK(floor(y) < SMALL_NUM);
}

ATF_TC(floorf_basic);
ATF_TC_HEAD(floorf_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of floorf(3)");
}

ATF_TC_BODY(floorf_basic, tc)
{
	const float x = 0.9999999;
	const float y = 0.0000001;

	ATF_CHECK(floorf(x) < SMALL_NUM);
	ATF_CHECK(floorf(y) < SMALL_NUM);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ceil_basic);
	ATF_TP_ADD_TC(tp, ceilf_basic);
	ATF_TP_ADD_TC(tp, floorf_basic);
	ATF_TP_ADD_TC(tp, floorf_basic);

	return atf_no_error();
}
