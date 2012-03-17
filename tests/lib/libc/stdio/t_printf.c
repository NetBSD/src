/* $NetBSD: t_printf.c,v 1.2.4.1 2012/03/17 17:49:55 bouyer Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
#include <math.h>
#include <stdio.h>
#include <string.h>

ATF_TC(snprintf_dotzero);
ATF_TC_HEAD(snprintf_dotzero, tc)
{

	atf_tc_set_md_var(tc, "descr", \
	    "PR lib/32951: %%.0f formats (0.0,0.5] to \"0.\"");
}

ATF_TC_BODY(snprintf_dotzero, tc)
{
	char s[4];

	ATF_CHECK(snprintf(s, sizeof(s), "%.0f", 0.1) == 1);
	ATF_REQUIRE_STREQ(s, "0");
}

ATF_TC(sprintf_zeropad);
ATF_TC_HEAD(sprintf_zeropad, tc)
{

	atf_tc_set_md_var(tc, "descr", "output format zero padding");
}

ATF_TC_BODY(sprintf_zeropad, tc)
{
	char str[1024];

	ATF_CHECK(sprintf(str, "%010f", 0.0) == 10);
	ATF_REQUIRE_STREQ(str, "000.000000");

	/* ieeefp */
#ifndef __vax__
	/* PR/44113: printf(3) should ignore zero padding for nan/inf */
	ATF_CHECK(sprintf(str, "%010f", NAN) == 10);
	ATF_REQUIRE_STREQ(str, "       nan");
	ATF_CHECK(sprintf(str, "%010f", INFINITY) == 10);
	ATF_REQUIRE_STREQ(str, "       inf");
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, snprintf_dotzero);
	ATF_TP_ADD_TC(tp, sprintf_zeropad);

	return atf_no_error();
}
