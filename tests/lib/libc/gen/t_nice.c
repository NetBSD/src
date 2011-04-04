/*	$NetBSD: t_nice.c,v 1.1 2011/04/04 09:52:18 jruoho Exp $ */

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
__RCSID("$NetBSD: t_nice.c,v 1.1 2011/04/04 09:52:18 jruoho Exp $");

#include <sys/resource.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

ATF_TC(nice_err);
ATF_TC_HEAD(nice_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test nice(3) for invalid parameters");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(nice_err, tc)
{
	int i;

	/*
	 * The call should fail with EPERM if the
	 * supplied parameter is negative and the
	 * caller does not have privileges. Note
	 * that the errno is thus "wrong" in NetBSD.
	 */
	for (i = -20; i < 0; i++) {

		errno = 0;

		ATF_REQUIRE(nice(i) == -1);
		ATF_REQUIRE(errno == EACCES);
	}
}

ATF_TC(nice_priority);
ATF_TC_HEAD(nice_priority, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test nice(3) vs. getpriority(2)");
}

ATF_TC_BODY(nice_priority, tc)
{
	int i, pri, nic;
	pid_t pid;
	int sta;

	for (i = 0; i <= 20; i++) {

		nic = nice(i);
		ATF_REQUIRE(nic != -1);

		errno = 0;
		pri = getpriority(PRIO_PROCESS, 0);
		ATF_REQUIRE(errno == 0);

		if (nic != pri)
			atf_tc_fail("nice(3) and getpriority(2) conflict");

		/*
		 * Also verify that the nice(3) values
		 * are inherited by child processes.
		 */
		pid = fork();
		ATF_REQUIRE(pid >= 0);

		if (pid == 0) {

			errno = 0;
			pri = getpriority(PRIO_PROCESS, 0);
			ATF_REQUIRE(errno == 0);

			if (nic != pri)
				exit(EXIT_FAILURE);

			exit(EXIT_SUCCESS);
		}

		(void)wait(&sta);

		if (WIFEXITED(sta) == 0 || WEXITSTATUS(sta) != EXIT_SUCCESS)
			atf_tc_fail("nice(3) value was not inherited");
	}
}

ATF_TC(nice_root);
ATF_TC_HEAD(nice_root, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test that nice(3) works");
	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(nice_root, tc)
{
	int i;

	for (i = -20; i <= 20; i++) {

		ATF_REQUIRE(nice(i) != -1);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, nice_err);
	ATF_TP_ADD_TC(tp, nice_priority);
	ATF_TP_ADD_TC(tp, nice_root);

	return atf_no_error();
}
