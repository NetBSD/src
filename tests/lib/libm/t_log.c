/* $NetBSD: t_log.c,v 1.2 2011/04/12 03:06:21 jruoho Exp $ */

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
__RCSID("$NetBSD: t_log.c,v 1.2 2011/04/12 03:06:21 jruoho Exp $");

#include <math.h>

#include <atf-c.h>

ATF_TC(log_nan);
ATF_TC_HEAD(log_nan, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test NaN from log(3)");
}

ATF_TC_BODY(log_nan, tc)
{
#ifndef __vax__

	double d;
	float f;

	/*
	 * If the argument is negative,
	 * the result should be NaN and
	 * a domain error should follow.
	 * Refer to the old PR lib/41931.
	 */
	d = log(-1);
	ATF_REQUIRE(isnan(d) != 0);

	d = log(-INFINITY);
	ATF_REQUIRE(isnan(d) != 0);

	f = logf(-1);
	ATF_REQUIRE(isnan(f) != 0);

	f = logf(-INFINITY);
	ATF_REQUIRE(isnan(f) != 0);
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, log_nan);

	return atf_no_error();
}
