/* $NetBSD: t_random.c,v 1.2 2012/03/28 10:38:00 jruoho Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_random.c,v 1.2 2012/03/28 10:38:00 jruoho Exp $");

#include <atf-c.h>
#include <stdlib.h>

/*
 * TODO: Add some general RNG tests (cf. the famous "diehard" tests?).
 */

ATF_TC(random_zero);
ATF_TC_HEAD(random_zero, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that random(3) does not always return "
	    "zero when the seed is initialized to zero");
}

ATF_TC_BODY(random_zero, tc)
{
	const size_t n = 1000000;
	size_t i, j;
	long x;

	/*
	 * See CVE-2012-1577.
	 */
	srandom(0);

	for (i = j = 0; i < n; i++) {

		if ((x = random()) == 0)
			j++;
	}

	ATF_REQUIRE(j != n);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, random_zero);

	return atf_no_error();
}
