/*	$NetBSD: t_regex_binary.c,v 1.1 2025/01/01 18:13:48 christos Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: t_regex_binary.c,v 1.1 2025/01/01 18:13:48 christos Exp $");

#include <atf-c.h>
#include <regex.h>

ATF_TC(negative_ranges);
ATF_TC_HEAD(negative_ranges, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test negative ranges compilation");
}
ATF_TC_BODY(negative_ranges, tc)
{
	regex_t re;
	char msg[1024];
	int e;

	if ((e = regcomp(&re, "[\xe0-\xf1][\xa0-\xd1].*", REG_EXTENDED)) != 0) {
		regerror(e, &re, msg, sizeof(msg));
		ATF_REQUIRE_MSG(0, "regcomp failed %s", msg);
	}
}

ATF_TC(negative_char);
ATF_TC_HEAD(negative_char, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test negative char in braces compilation");
}
ATF_TC_BODY(negative_char, tc)
{
	regex_t re;
	char msg[1024];
	int e;

	/* PR/58910 */
	if ((e = regcomp(&re, ": j:[]j:[]j:[\xd9j:[]", REG_EXTENDED)) != 0) {
		regerror(e, &re, msg, sizeof(msg));
		ATF_REQUIRE_MSG(0, "regcomp failed %s", msg);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, negative_ranges);
	ATF_TP_ADD_TC(tp, negative_char);
	return atf_no_error();
}
