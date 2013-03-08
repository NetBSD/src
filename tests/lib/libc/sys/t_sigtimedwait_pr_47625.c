/* $NetBSD: t_sigtimedwait_pr_47625.c,v 1.1 2013/03/08 10:33:51 martin Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_sigtimedwait_pr_47625.c,v 1.1 2013/03/08 10:33:51 martin Exp $");

#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <atf-c.h>


ATF_TC(sigtimedwait);

ATF_TC_HEAD(sigtimedwait, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr", "Test for PR kern/47625: sigtimedwait"
	    " with a timeout value of all zero should return imediately");
}

ATF_TC_BODY(sigtimedwait, tc)
{
	sigset_t block;
	struct timespec ts;
	siginfo_t info;
	int r;

	sigemptyset(&block);
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	r = sigtimedwait(&block, &info, &ts);
	ATF_REQUIRE(r == -1);
	ATF_REQUIRE_ERRNO(EAGAIN, errno);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, sigtimedwait);

	return atf_no_error();
}
