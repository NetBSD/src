/*	$NetBSD: t_backtrace_sandbox.c,v 1.2 2025/01/27 17:02:50 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 Kyle Evans <kevans@FreeBSD.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_backtrace_sandbox.c,v 1.2 2025/01/27 17:02:50 riastradh Exp $");

#include <sys/param.h>
#ifdef __FreeBSD__
#include <sys/capsicum.h>
#define __arraycount(a) nitems(a)
#endif

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
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

	frames = backtrace(addr, __arraycount(addr));
	ATF_REQUIRE(frames > 0);

	syms = backtrace_symbols_fmt(addr, frames, "%n");
	ATF_REQUIRE(strcmp(syms[0], "atfu_backtrace_sandbox_body") == 0);

	backtrace_sandbox_init();
#ifdef __FreeBSD__
	cap_enter();
#else
	ATF_REQUIRE(chroot("/tmp") == 0);
#endif

	syms = backtrace_symbols_fmt(addr, frames, "%n");
	ATF_REQUIRE(strcmp(syms[0], "atfu_backtrace_sandbox_body") == 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, backtrace_sandbox);

	return atf_no_error();
}
