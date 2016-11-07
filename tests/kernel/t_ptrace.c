/*	$NetBSD: t_ptrace.c,v 1.13 2016/11/07 21:09:03 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace.c,v 1.13 2016/11/07 21:09:03 kamil Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

#include "../h_macros.h"

/*
 * A child process cannot call atf functions and expect them to magically
 * work like in the parent.
 * The printf(3) messaging from a child will not work out of the box as well
 * without estabilishing a communication protocol with its parent. To not
 * overcomplicate the tests - do not log from a child and use err(3)/errx(3)
 * wrapped with FORKEE_ASSERT()/FORKEE_ASSERTX() as that is guaranteed to work.
 */
#define FORKEE_ASSERTX(x)							\
do {										\
	int ret = (x);								\
	if (!ret)								\
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",	\
		     __FILE__, __LINE__, __func__, #x);				\
} while (0)

#define FORKEE_ASSERT(x)							\
do {										\
	int ret = (x);								\
	if (!ret)								\
		err(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",	\
		     __FILE__, __LINE__, __func__, #x);				\
} while (0)

ATF_TC(attach_pid0);
ATF_TC_HEAD(attach_pid0, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that a debugger cannot attach to PID 0");
}

ATF_TC_BODY(attach_pid0, tc)
{
	errno = 0;
	ATF_REQUIRE_ERRNO(EPERM, ptrace(PT_ATTACH, 0, NULL, 0) == -1);
}

ATF_TC(attach_pid1);
ATF_TC_HEAD(attach_pid1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that a debugger cannot attach to PID 1 (as non-root)");

	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(attach_pid1, tc)
{
	ATF_REQUIRE_ERRNO(EPERM, ptrace(PT_ATTACH, 1, NULL, 0) == -1);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, attach_pid0);
	ATF_TP_ADD_TC(tp, attach_pid1);

	return atf_no_error();
}
