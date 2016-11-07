/* $NetBSD: t_wait_noproc.c,v 1.2 2016/11/07 02:23:43 kamil Exp $ */

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_wait_noproc.c,v 1.2 2016/11/07 02:23:43 kamil Exp $");

#include <sys/wait.h>
#include <sys/resource.h>

#include <errno.h>

#include <atf-c.h>

#ifndef TWAIT_OPTION
#define TWAIT_OPTION 0
#endif

#if TWAIT_OPTION == 0
ATF_TC(wait);
ATF_TC_HEAD(wait, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that wait(2) returns ECHILD for no child");
}

ATF_TC_BODY(wait, tc)
{
	errno = 0;
	ATF_REQUIRE(wait(NULL) == -1);
	ATF_REQUIRE(errno == ECHILD);
}
#endif

ATF_TC(waitpid);
ATF_TC_HEAD(waitpid, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that waitpid(2) returns ECHILD for WAIT_ANY and option %s",
	    ___STRING(TWAIT_OPTION));
}

ATF_TC_BODY(waitpid, tc)
{
#if TWAIT_OPTION
	/* wait4() (and friends) with WNOHANG and no children does not error */
	atf_tc_expect_fail("PR standards/51606");
#endif
	errno = 0;
	ATF_REQUIRE(waitpid(WAIT_ANY, NULL, TWAIT_OPTION) == -1);
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(waitid);
ATF_TC_HEAD(waitid, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that waitid(2) returns ECHILD for P_ALL and option %s",
	    ___STRING(TWAIT_OPTION));
}

ATF_TC_BODY(waitid, tc)
{
#if TWAIT_OPTION
	/* wait4() (and friends) with WNOHANG and no children does not error */
	atf_tc_expect_fail("PR standards/51606");
#endif
	errno = 0;
	ATF_REQUIRE(waitid(P_ALL, 0, NULL, WEXITED | TWAIT_OPTION) == -1);
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(wait3);
ATF_TC_HEAD(wait3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that wait3(2) returns ECHILD for no child");
}

ATF_TC_BODY(wait3, tc)
{
#if TWAIT_OPTION
	/* wait4() (and friends) with WNOHANG and no children does not error */
	atf_tc_expect_fail("PR standards/51606");
#endif
	errno = 0;
	ATF_REQUIRE(wait3(NULL, TWAIT_OPTION, NULL) == -1);
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(wait4);
ATF_TC_HEAD(wait4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that wait4(2) returns ECHILD for WAIT_ANY and option %s",
	    ___STRING(TWAIT_OPTION));
}

ATF_TC_BODY(wait4, tc)
{
#if TWAIT_OPTION
	/* wait4() (and friends) with WNOHANG and no children does not error */
	atf_tc_expect_fail("PR standards/51606");
#endif
	errno = 0;
	ATF_REQUIRE(wait4(WAIT_ANY, NULL, TWAIT_OPTION, NULL) == -1);
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(wait6);
ATF_TC_HEAD(wait6, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test that wait6(2) returns ECHILD for P_ALL and option %s",
	    ___STRING(TWAIT_OPTION));
}

ATF_TC_BODY(wait6, tc)
{
#if TWAIT_OPTION
	/* wait4() (and friends) with WNOHANG and no children does not error */
	atf_tc_expect_fail("PR standards/51606");
#endif
	errno = 0;
	ATF_REQUIRE(wait6(P_ALL, 0, NULL, WEXITED | TWAIT_OPTION, NULL, NULL) == -1);
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TP_ADD_TCS(tp)
{

#if TWAIT_OPTION == 0
	ATF_TP_ADD_TC(tp, wait);
#endif
	ATF_TP_ADD_TC(tp, waitpid);
	ATF_TP_ADD_TC(tp, waitid);
	ATF_TP_ADD_TC(tp, wait3);
	ATF_TP_ADD_TC(tp, wait4);
	ATF_TP_ADD_TC(tp, wait6);

	return atf_no_error();
}
