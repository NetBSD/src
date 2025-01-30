/*	$NetBSD: t_backtrace_sandbox.c,v 1.3 2025/01/30 16:13:51 christos Exp $	*/

/*-
 * Copyright (c) 2025 Kyle Evans <kevans@FreeBSD.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_backtrace_sandbox.c,v 1.3 2025/01/30 16:13:51 christos Exp $");

#include <sys/param.h>
#include <sys/wait.h>
#ifdef __FreeBSD__
#include <sys/capsicum.h>
#define __arraycount(a) nitems(a)
#endif

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

#define	BT_FUNCTIONS		10

ATF_TC(backtrace_sandbox);
ATF_TC_HEAD(backtrace_sandbox, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Test backtrace_sandbox_init(3) in sandbox");
#ifndef __FreeBSD__
	atf_tc_set_md_var(tc, "require.user", "root");
#endif
}

ATF_TC_BODY(backtrace_sandbox, tc)
{
	void *addr[BT_FUNCTIONS];
	char **syms;
	size_t frames;
	pid_t pid;
	int status;

	frames = backtrace(addr, __arraycount(addr));
	ATF_REQUIRE(frames > 0);

	syms = backtrace_symbols_fmt(addr, frames, "%n");
	ATF_REQUIRE(strcmp(syms[0], "atfu_backtrace_sandbox_body") == 0);

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {

		backtrace_sandbox_init();
#ifdef __FreeBSD__
		cap_enter();
#else
		if (chroot("/tmp") != 0)
			_exit(EXIT_FAILURE);
#endif

		syms = backtrace_symbols_fmt(addr, frames, "%n");
		if (strcmp(syms[0], "atfu_backtrace_sandbox_body") != 0)
			_exit(EXIT_FAILURE);

		backtrace_sandbox_fini();

		_exit(EXIT_SUCCESS);
	}

	(void)wait(&status);

	if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
		atf_tc_fail("resolving symbols in chroot failed");

}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, backtrace_sandbox);

	return atf_no_error();
}
