/*	$NetBSD: t_exect.c,v 1.4 2016/12/09 08:34:37 kamil Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

ATF_TC(t_exect_null);

ATF_TC_HEAD(t_exect_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests an empty exect(2) executing");
}

static sig_atomic_t caught = 0;

static void
sigtrap_handler(int sig, siginfo_t *info, void *ctx)
{
	ATF_REQUIRE_EQ(sig, SIGTRAP);
	ATF_REQUIRE_EQ(info->si_code, TRAP_TRACE);

	++caught;
}

ATF_TC_BODY(t_exect_null, tc)
{
	struct sigaction act;

#if defined(__x86_64__)
	/*
	 * exect(NULL,NULL,NULL) generates 15859 times SIGTRAP on amd64
	 */
	atf_tc_expect_fail("PR port-amd64/51700");
#endif

	ATF_REQUIRE(sigemptyset(&act.sa_mask) == 0);
	act.sa_sigaction = sigtrap_handler;
	act.sa_flags = SA_SIGINFO;

	ATF_REQUIRE(sigaction(SIGTRAP, &act, 0) == 0);

	ATF_REQUIRE_ERRNO(EFAULT, exect(NULL, NULL, NULL) == -1);

	ATF_REQUIRE_EQ_MSG(caught, 1, "expected caught (1) != received (%d)",
	    (int)caught);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_exect_null);

	return atf_no_error();
}
