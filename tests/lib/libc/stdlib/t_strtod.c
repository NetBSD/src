/*	$NetBSD: t_strtod.c,v 1.1 2011/04/05 06:15:30 jruoho Exp $ */

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

/* Public domain, Otto Moerbeek <otto@drijf.net>, 2006. */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_strtod.c,v 1.1 2011/04/05 06:15:30 jruoho Exp $");

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ATF_TC(strtod_basic);
ATF_TC_HEAD(strtod_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of strtod(3)");
}

ATF_TC_BODY(strtod_basic, tc)
{
	char buf[512];
	size_t i, n;
	double d;

	n = 1024 * 1000;

	for (i = 1; i < n; i = i + 1024) {

		(void)snprintf(buf, sizeof(buf), "%zu.%zu", i, i + 1);

		errno = 0;
		d = strtod(buf, NULL);

		ATF_REQUIRE(d > 0.0);
		ATF_REQUIRE(errno == 0);
	}
}

ATF_TC(strtod_underflow);
ATF_TC_HEAD(strtod_underflow, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test underflow in strtod(3)");
}

ATF_TC_BODY(strtod_underflow, tc)
{

	const char *tmp =
	    "0.0000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000000000000000"
	    "000000000000000002";

	double d;

	errno = 0;
	d = strtod(tmp, NULL);

	if (errno != ERANGE)
		atf_tc_fail("strtod(3) did not detect underflow");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, strtod_basic);
	ATF_TP_ADD_TC(tp, strtod_underflow);

	return atf_no_error();
}
