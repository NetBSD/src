/* $NetBSD: t_remquo.c,v 1.2.2.2 2024/10/02 12:46:13 martin Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen and Greg Troxel.
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

#include <assert.h>
#include <atf-c.h>
#include <float.h>
#include <math.h>

/*
 * remquo(3)
 */
ATF_TC(remquo_args);
ATF_TC_HEAD(remquo_args, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test some selected arguments");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, remquo_args);

	return atf_no_error();
}

#ifdef __vax__

ATF_TC_BODY(remquo_args, tc)
{
        atf_tc_expect_fail("PR 57881: vax libm is missing various symbols");
        atf_tc_fail("missing remquo on vax");
}

#else /* !__vax__ */

static const struct {
	double		x;
	double		y;
	double		r;	/* expected */
	int		quo;	/* expected */
} args[] = {
	{ -135.0,	-90.0,		45.0,	2 },
	{ -45.0,	-90.0,		-45.0,	8 },
	{ 45.0,		-90.0,		45.0,	-8 },
	{ 135.0,	-90.0,		-45.0,	-2 },
	{ -180.0,	90.0,		-0.0,	-2 },
	{ -135.0,	90.0,		45.0,	-2 },
	{ -90.0,	90.0,		-0.0,	-1 },
	{ -45.0,	90.0,		-45.0,	-8 },
	{ 0.0,		90.0,		0.0,	0 },
	{ 45.0,		90.0,		45.0,	8 },
	{ 90.0,		90.0,		0.0,	1 },
	{ 135.0,	90.0,		-45.0,	2 },
	{ 180.0,	90.0,		0.0,	2 },
};

ATF_TC_BODY(remquo_args, tc)
{
	const double eps = DBL_EPSILON;
	size_t i;

	for (i = 0; i < __arraycount(args); i++) {
		double x = args[i].x;
		double y = args[i].y;
		double r;
		double r_exp = args[i].r;
		int quo;
		int quo_exp = args[i].quo;

		bool ok = true;

		r = remquo(x, y, &quo);

		ok &= (fabs(r - r_exp) <= eps);

		/*
		 * For now, consider 0 to have positive sign.
		 */
		if (quo_exp < 0 && quo >= 0)
			ok = false;
		if (quo_exp >= 0 && quo < 0)
			ok = false;

		/*
		 * The specification requires that quo be congruent to
		 * the integer remainder modulo 2^k for some k >=3.
		 */
		ok &= ((quo & 0x7) == (quo_exp & 0x7));

		if (!ok) {
			atf_tc_fail_nonfatal("remquo(%g, %g) "
			    "r/quo expected %g/%d != %g/%d",
			     x, y, r_exp, quo_exp, r, quo);
		}
	}
}

#endif /* !__vax__ */
