/* $NetBSD: t_floor.c,v 1.4 2011/03/25 10:42:38 jruoho Exp $ */

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
__RCSID("$NetBSD: t_floor.c,v 1.4 2011/03/25 10:42:38 jruoho Exp $");

#include <math.h>
#include <limits.h>
#include <stdlib.h>

#include <atf-c.h>

ATF_TC(floor);
ATF_TC_HEAD(floor, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of floor(3)");
}

ATF_TC_BODY(floor, tc)
{
	const int n = 10240;
	double x, y;
	int i;

	/*
	 * This may fail under QEMU; see PR misc/44767.
	 */
	if (system("cpuctl identify 0 | grep -q QEMU") == 0)
		atf_tc_expect_fail("PR misc/44767");

	for (i = 0; i < n; i++) {

		x = i + 0.999999999;
		y = i + 0.000000001;

		ATF_REQUIRE(fabs(floor(x) - (double)i) < 1.0e-40);
		ATF_REQUIRE(fabs(floor(y) - (double)i) < 1.0e-40);
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, floor);

	return atf_no_error();
}
