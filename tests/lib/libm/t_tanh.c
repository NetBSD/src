/* $NetBSD: t_tanh.c,v 1.2 2011/04/08 06:37:50 jruoho Exp $ */

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
__RCSID("$NetBSD: t_tanh.c,v 1.2 2011/04/08 06:37:50 jruoho Exp $");

#include <math.h>

#include <atf-c.h>

ATF_TC(tanh_sign);
ATF_TC_HEAD(tanh_sign, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test signs in tanh(3) and tanhf(3)");
}

ATF_TC_BODY(tanh_sign, tc)
{
	double d;
	float f;

	d = 0.0;
	f = 0.0;

#ifndef __vax__
	/*
	 * PR lib/44057.
	 */
	d = tanh(-0.0);
	f = tanhf(-0.0);

	ATF_REQUIRE(signbit(d) != 0);
	ATF_REQUIRE(signbit(f) != 0);
#endif
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, tanh_sign);

	return atf_no_error();
}
