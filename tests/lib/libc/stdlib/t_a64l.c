/* $NetBSD: t_a64l.c,v 1.1 2020/07/01 07:16:37 jruoho Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_a64l.c,v 1.1 2020/07/01 07:16:37 jruoho Exp $");

#include <atf-c.h>
#include <stdio.h>
#include <stdlib.h>

struct test {
	const char	*str;
	size_t		 len;
	long		 val;
} t[] = {
	{ "",	     0, 0L	  },
	{ "///",     3, 4161L	  }, /* 1*64^0 + 1*64^1 + 1*64^2	    */
	{ "123",     3, 20739L	  }, /* 3*64^0 + 4*64^1 + 5*64^2	    */
	{ "zzz",     3, 262143L	  }, /* 63*64^0 + 63*64^1 + 63*64^2	    */
	{ "a64l",    4, 12870182L }, /* 38*64^0 + 8*64^1 + 6*64^2 + 49*64^3 */
	{ "a64l...", 4, 12870182L }, /* 38*64^0 + 8*64^1 + 6*64^2 + 49*64^3 */
};

#define MAXLEN 8

ATF_TC(a64l_basic);
ATF_TC_HEAD(a64l_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of a64l(3)");
}

ATF_TC_BODY(a64l_basic, tc)
{
	for (size_t i = 0; i < __arraycount(t); i++) {
		long val = a64l(t[i].str);
		ATF_REQUIRE(val == t[i].val);
	}

}

ATF_TC(l64a_basic);
ATF_TC_HEAD(l64a_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of l64a(3)");
}

ATF_TC_BODY(l64a_basic, tc)
{
	for (size_t i = 0; i < __arraycount(t); i++) {
		char *str = l64a(t[i].val);
		ATF_REQUIRE(strlen(str) == t[i].len);
		ATF_REQUIRE(strncmp(str, t[i].str, t[i].len) == 0);
	}
}

ATF_TC(l64a_r_basic);
ATF_TC_HEAD(l64a_r_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of l64a_r(3)");
}

ATF_TC_BODY(l64a_r_basic, tc)
{
	char buf[MAXLEN];

	for (size_t i = 0; i < __arraycount(t); i++) {

		(void)memset(buf, '\0', sizeof(buf));

		ATF_REQUIRE(l64a_r(t[i].val, buf, t[i].len + 1) == 0);
		ATF_REQUIRE(strlen(buf) == t[i].len);
		ATF_REQUIRE(strncmp(buf, t[i].str, t[i].len) == 0);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, a64l_basic);
	ATF_TP_ADD_TC(tp, l64a_basic);
	ATF_TP_ADD_TC(tp, l64a_r_basic);

	return atf_no_error();
}
