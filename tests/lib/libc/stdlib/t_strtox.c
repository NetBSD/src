/*	$NetBSD: t_strtox.c,v 1.2 2010/12/06 17:30:07 pooka Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_strtox.c,v 1.2 2010/12/06 17:30:07 pooka Exp $");

#include <atf-c.h>
#include <stdlib.h>

ATF_TC(hexadecimal);

ATF_TC_HEAD(hexadecimal, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test strtod with hexdecimal numbers");
}

ATF_TC_BODY(hexadecimal, tc)
{
	const char *str;
	char *end;

	str = "-0x0";
	ATF_REQUIRE(strtod(str, &end) == -0.0 && end == str+4);

	atf_tc_expect_fail("PR lib/44189");
	str = "-0x";
	ATF_REQUIRE(strtod(str, &end) == -0.0 && end == str+2);
	atf_tc_expect_pass();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, hexadecimal);

	return atf_no_error();
}
