/* $NetBSD: t_getrusage.c,v 1.1 2011/04/06 05:53:17 jruoho Exp $ */

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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_getrusage.c,v 1.1 2011/04/06 05:53:17 jruoho Exp $");

#include <sys/resource.h>

#include <atf-c.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>

static void	sighandler(int);

static const int who[] = {
	RUSAGE_SELF,
	RUSAGE_CHILDREN
};

static void
sighandler(int signo)
{
	/* Nothing. */
}

ATF_TC(getrusage_err);
ATF_TC_HEAD(getrusage_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test error conditions");
}

ATF_TC_BODY(getrusage_err, tc)
{
	struct rusage ru;
	size_t i;

	for (i = 0; i < __arraycount(who); i++) {

		errno = 0;

		ATF_REQUIRE(getrusage(INT_MAX, &ru) != 0);
		ATF_REQUIRE(errno == EINVAL);

		errno = 0;

		ATF_REQUIRE(getrusage(who[i], (void *)0) != 0);
		ATF_REQUIRE(errno == EFAULT);
	}
}

ATF_TC(getrusage_sig);
ATF_TC_HEAD(getrusage_sig, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test signal count with getrusage(2)");
}

ATF_TC_BODY(getrusage_sig, tc)
{
	struct rusage ru;
	const long n = 5;
	int i;

	/*
	 * Test that signals are recorded.
	 */
	ATF_REQUIRE(signal(SIGUSR1, sighandler) != SIG_ERR);

	for (i = 0; i < n; i++)
		ATF_REQUIRE(raise(SIGUSR1) == 0);

	(void)memset(&ru, 0, sizeof(struct rusage));
	ATF_REQUIRE(getrusage(RUSAGE_SELF, &ru) == 0);

	if (n != ru.ru_nsignals)
		atf_tc_fail("getrusage(2) did not record signals");
}

ATF_TC(getrusage_utime);
ATF_TC_HEAD(getrusage_utime, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test zero utime from getrusage(2)");
}

ATF_TC_BODY(getrusage_utime, tc)
{
	size_t n = UINT16_MAX * 100;
	struct rusage ru;

	/*
	 * Test that getrusage(2) does not return
	 * zero user time for the calling process;
	 * cf. PR port-xen/41734. Note that this
	 * does not trigger always.
	 */
	atf_tc_expect_fail("PR port-amd64/41734");

	while (n > 0) {
		 asm volatile("nop");	/* Do something. */
		 n--;
	}

	(void)memset(&ru, 0, sizeof(struct rusage));
	ATF_REQUIRE(getrusage(RUSAGE_SELF, &ru) == 0);

	if (ru.ru_utime.tv_sec == 0 && ru.ru_utime.tv_usec == 0)
		atf_tc_fail("zero user time from getrusage(2)");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, getrusage_err);
	ATF_TP_ADD_TC(tp, getrusage_sig);
	ATF_TP_ADD_TC(tp, getrusage_utime);

	return atf_no_error();
}
