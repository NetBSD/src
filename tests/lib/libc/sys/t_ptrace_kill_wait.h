/*	$NetBSD: t_ptrace_kill_wait.h,v 1.1 2020/05/04 21:55:12 kamil Exp $	*/

/*-
 * Copyright (c) 2016, 2017, 2018, 2019, 2020 The NetBSD Foundation, Inc.
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

static void
ptrace_kill(const char *type)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 &&
		    "Child should be terminated by a signal from its parent");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before killing the child process with %s\n", type);
	if (strcmp(type, "ptrace(PT_KILL)") == 0) {
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, (void*)1, 0) != -1);
	} else if (strcmp(type, "kill(SIGKILL)") == 0) {
		kill(child, SIGKILL);
	} else if (strcmp(type, "killpg(SIGKILL)") == 0) {
		setpgid(child, 0);
		killpg(getpgid(child), SIGKILL);
	}

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define PTRACE_KILL(test, type)						\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
        atf_tc_set_md_var(tc, "descr",					\
            "Verify killing the child with " type);			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
        ptrace_kill(type);						\
}

// PT_CONTINUE with SIGKILL is covered by traceme_sendsignal_simple1
PTRACE_KILL(kill1, "ptrace(PT_KILL)")
PTRACE_KILL(kill2, "kill(SIGKILL)")
PTRACE_KILL(kill3, "killpg(SIGKILL)")

#define ATF_TP_ADD_TCS_PTRACE_WAIT_KILL() \
	ATF_TP_ADD_TC(tp, kill1); \
	ATF_TP_ADD_TC(tp, kill2); \
	ATF_TP_ADD_TC(tp, kill3);
