/*	$NetBSD: t_wcsrtombs.c,v 1.1 2019/07/28 13:46:45 christos Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_wcsrtombs.c,v 1.1 2019/07/28 13:46:45 christos Exp $");

#include <atf-c.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>

ATF_TC(wcsrtombs_advance);
ATF_TC_HEAD(wcsrtombs_advance, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test wcsrtombs(3) advances "
	    "the source pointer to the first illegal byte");
}

ATF_TC_BODY(wcsrtombs_advance, tc)
{
	wchar_t label[] = L"L" L"\u0403" L"bel";
	const wchar_t *wp = label;
	char lbuf[128];
	mbstate_t mbstate;
	size_t n;

	memset(&mbstate, 0, sizeof(mbstate));
	memset(lbuf, 0, sizeof(lbuf));
	n = wcsrtombs(lbuf, &wp, sizeof(lbuf), &mbstate);

	ATF_REQUIRE_EQ(n, (size_t)-1);
	ATF_REQUIRE_EQ(errno, EILSEQ);
	ATF_REQUIRE_EQ(wp - label, 1);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, wcsrtombs_advance);
	return atf_no_error();
}
