/*	$NetBSD: t_ptrace.c,v 1.7 2025/05/02 02:24:44 riastradh Exp $	*/

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
__RCSID("$NetBSD: t_ptrace.c,v 1.7 2025/05/02 02:24:44 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>

#include <atf-c.h>

#include "h_macros.h"

/*
 * A child process cannot call atf functions and expect them to magically
 * work like in the parent.
 * The printf(3) messaging from a child will not work out of the box as well
 * without establishing a communication protocol with its parent. To not
 * overcomplicate the tests - do not log from a child and use err(3)/errx(3)
 * wrapped with FORKEE_ASSERT()/FORKEE_ASSERTX() as that is guaranteed to work.
 */
#define FORKEE_ASSERTX(x)						      \
do {									      \
	int ret = (x);							      \
	if (!ret)							      \
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",    \
		    __FILE__, __LINE__, __func__, #x);			      \
} while (0)

#define FORKEE_ASSERT(x)						      \
do {									      \
	int ret = (x);							      \
	if (!ret)							      \
		err(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",     \
		    __FILE__, __LINE__, __func__, #x);			      \
} while (0)

#define FORKEE_ASSERT_EQ(x, y)						      \
do {									      \
	uintmax_t vx = (x);						      \
	uintmax_t vy = (y);						      \
	int ret = vx == vy;						      \
	if (!ret)							      \
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: "	      \
		    "%s(%ju) == %s(%ju)", __FILE__, __LINE__, __func__,	      \
		    #x, vx, #y, vy);					      \
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

ATF_TC(attach_pid1_securelevel);
ATF_TC_HEAD(attach_pid1_securelevel, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that a debugger cannot attach to PID 1 with "
	    "securelevel >= 0 (as root)");

	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(attach_pid1_securelevel, tc)
{
	int level;
	size_t len = sizeof(level);

	RL(sysctlbyname("kern.securelevel", &level, &len, NULL, 0));

	if (level < 0) {
		atf_tc_skip("Test must be run with securelevel >= 0");
	}

	ATF_REQUIRE_ERRNO(EPERM, ptrace(PT_ATTACH, 1, NULL, 0) == -1);
}

ATF_TC(attach_self);
ATF_TC_HEAD(attach_self, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that a debugger cannot attach to self (as it's nonsense)");
}

ATF_TC_BODY(attach_self, tc)
{
	ATF_REQUIRE_ERRNO(EINVAL, ptrace(PT_ATTACH, getpid(), NULL, 0) == -1);
}

ATF_TC(attach_chroot);
ATF_TC_HEAD(attach_chroot, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that a debugger cannot trace another process unless the "
	    "process's root directory is at or below the tracing process's "
	    "root");

	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(attach_chroot, tc)
{
	char buf[PATH_MAX];
	pid_t child;
	int fds_toparent[2], fds_fromparent[2];
	int rv;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */

	(void)memset(buf, '\0', sizeof(buf));
	REQUIRE_LIBC(getcwd(buf, sizeof(buf)), NULL);
	(void)strlcat(buf, "/dir", sizeof(buf));

	RL(mkdir(buf, 0500));
	RL(chdir(buf));

	RL(pipe(fds_toparent));
	RL(pipe(fds_fromparent));
	child = atf_utils_fork();
	if (child == 0) {
		FORKEE_ASSERT(close(fds_toparent[0]) == 0);
		FORKEE_ASSERT(close(fds_fromparent[1]) == 0);

		FORKEE_ASSERT(chroot(buf) == 0);

		FORKEE_ASSERT((rv = write(fds_toparent[1], &msg, sizeof(msg)))
		    != -1);
		FORKEE_ASSERT_EQ(rv, sizeof(msg));

		if (ptrace(PT_ATTACH, getppid(), NULL, 0) == 0) {
			errx(EXIT_FAILURE, "%s unexpectedly succeeded",
			    "ptrace(PT_ATTACH, getppid(), NULL, 0)");
		} else if (errno != EPERM) {
			err(EXIT_FAILURE, "%s failed but not with EPERM",
			    "ptrace(PT_ATTACH, getppid(), NULL, 0)");
		}

		FORKEE_ASSERT((rv = read(fds_fromparent[0], &msg, sizeof(msg)))
		    != -1);
		FORKEE_ASSERT_EQ(rv, sizeof(msg));

		_exit(0);
	}
	RL(close(fds_toparent[1]));
	RL(close(fds_fromparent[0]));

	printf("Waiting for chrooting of the child PID %d", child);
	RL(rv = read(fds_toparent[0], &msg, sizeof(msg)));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Child is ready, it will try to PT_ATTACH to parent\n");
	RL(rv = write(fds_fromparent[1], &msg, sizeof(msg)));
	ATF_REQUIRE(rv == sizeof(msg));

        printf("fds_fromparent is no longer needed - close it\n");
        RL(close(fds_fromparent[1]));

        printf("fds_toparent is no longer needed - close it\n");
        RL(close(fds_toparent[0]));
}

ATF_TC(traceme_twice);
ATF_TC_HEAD(traceme_twice, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that a process cannot mark its parent a debugger twice");
}

ATF_TC_BODY(traceme_twice, tc)
{

	printf("Mark the parent process (PID %d) a debugger of PID %d",
	       getppid(), getpid());
	RL(ptrace(PT_TRACE_ME, 0, NULL, 0));

	printf("Mark the parent process (PID %d) a debugger of PID %d again",
	       getppid(), getpid());
	ATF_REQUIRE_ERRNO(EBUSY, ptrace(PT_TRACE_ME, 0, NULL, 0) == -1);
}

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	ATF_TP_ADD_TC(tp, attach_pid0);
	ATF_TP_ADD_TC(tp, attach_pid1);
	ATF_TP_ADD_TC(tp, attach_pid1_securelevel);
	ATF_TP_ADD_TC(tp, attach_self);
	ATF_TP_ADD_TC(tp, attach_chroot);
	ATF_TP_ADD_TC(tp, traceme_twice);

	return atf_no_error();
}
