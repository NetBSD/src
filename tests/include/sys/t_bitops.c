/*	$NetBSD: t_bitops.c,v 1.1 2011/03/19 06:39:17 jruoho Exp $ */

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

#include <atf-c.h>

#include <sys/bitops.h>
#include <math.h>

ATF_TC(ilog2_1);
ATF_TC_HEAD(ilog2_1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test ilog2(3) for correctness");
}

ATF_TC_BODY(ilog2_1, tc)
{
	uint64_t i, x;

	for (i = x = 0; i < 64; i++) {

		x = (uint64_t)1 << i;

		ATF_REQUIRE(i == (uint64_t)ilog2(x));
	}
}

ATF_TC(ilog2_2);
ATF_TC_HEAD(ilog2_2, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test log2(3) vs. ilog2(3)");
}

ATF_TC_BODY(ilog2_2, tc)
{
	double  x, y;
	uint64_t i;

	for (i = 1; i < UINT32_MAX; i += UINT16_MAX) {

		x = log2(i);
		y = (double)(ilog2(i));

		ATF_REQUIRE(ceil(x) >= y);
		ATF_REQUIRE(fabs(floor(x) - y) < 1.0e-40);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ilog2_1);
	ATF_TP_ADD_TC(tp, ilog2_2);

	return atf_no_error();
}
