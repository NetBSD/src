/* $NetBSD: t_strfmon.c,v 1.2 2017/12/07 22:23:14 kre Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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
__RCSID("$NetBSD: t_strfmon.c,v 1.2 2017/12/07 22:23:14 kre Exp $");

#include <atf-c.h>
#include <locale.h>
#include <monetary.h>

ATF_TC(strfmon);

ATF_TC_HEAD(strfmon, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks strfmon_l under diferent locales");
}

ATF_TC_BODY(strfmon, tc)
{
	const struct {
		const char *locale;
		const char *expected;
	} tests[] = {
	    { "C", "[ **1234.57] [ **1234.57]" },
	    { "de_DE.UTF-8", "[ **1234,57 €] [ **1.234,57 EUR]" },
	    { "en_GB.UTF-8", "[ £**1234.57] [ GBP**1,234.57]" },
	};
	locale_t loc;
	size_t i;
	char buf[80];
	for (i = 0; i < __arraycount(tests); ++i) {
		loc = newlocale(LC_MONETARY_MASK, tests[i].locale, 0);
		ATF_REQUIRE(loc != 0);
		strfmon_l(buf, sizeof(buf), loc, "[%^=*#6n] [%=*#6i]",
		    1234.567, 1234.567);
		ATF_REQUIRE_STREQ(tests[i].expected, buf);
		freelocale(loc);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, strfmon);

	return atf_no_error();
}
