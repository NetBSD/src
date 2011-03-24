/*	$NetBSD: t_raise.c,v 1.1 2011/03/24 16:56:38 jruoho Exp $ */

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

#include <atf-c.h>

#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static bool	fail;
static void	handler(int);
static int	sig[] = { SIGALRM, SIGIO, SIGUSR1, SIGUSR2, SIGPWR };

static void
handler(int signo)
{
	size_t i;

	for (i = 0; i < __arraycount(sig); i++) {

		if (sig[i] == signo) {
			fail = false;
			break;
		}
	}
}

ATF_TC(raise_1);
ATF_TC_HEAD(raise_1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test raise(3) for invalid parameters");
}

ATF_TC_BODY(raise_1, tc)
{
	int i = 0;

	while (i < 10) {

		ATF_REQUIRE(raise(10240 + i) == -1);

		i++;
	}
}

ATF_TC(raise_2);
ATF_TC_HEAD(raise_2, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of raise(3)");
}

ATF_TC_BODY(raise_2, tc)
{
	struct timespec tv, tr;
	struct sigaction sa;
	size_t i;

	for (i = 0; i < __arraycount(sig); i++) {

		(void)memset(&sa, 0, sizeof(struct sigaction));

		fail = true;

		tv.tv_sec = 0;
		tv.tv_nsec = 2;

		sa.sa_flags = 0;
		sa.sa_handler = handler;

		ATF_REQUIRE(sigemptyset(&sa.sa_mask) == 0);
		ATF_REQUIRE(sigaction(sig[i], &sa, 0) == 0);

		ATF_REQUIRE(raise(sig[i]) == 0);
		ATF_REQUIRE(nanosleep(&tv, &tr) == 0);

		if (fail != false)
			atf_tc_fail("raise(3) did not raise a signal");
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, raise_1);
	ATF_TP_ADD_TC(tp, raise_2);

	return atf_no_error();
}
