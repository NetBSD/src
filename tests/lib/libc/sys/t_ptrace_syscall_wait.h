/*	$NetBSD: t_ptrace_syscall_wait.h,v 1.3 2023/03/20 11:19:30 hannken Exp $	*/

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


static int test_syscall_caught;

static void
syscall_sighand(int arg)
{

	DPRINTF("Caught a signal %d in process %d\n", arg, getpid());

	FORKEE_ASSERT_EQ(arg, SIGINFO);

	++test_syscall_caught;

	FORKEE_ASSERT_EQ(test_syscall_caught, 1);
}

static void
syscall_body(const char *op)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	if (strstr(op, "signal") != NULL) {
#if defined(TWAIT_HAVE_STATUS)
		atf_tc_expect_fail("XXX: behavior under investigation");
#else
		atf_tc_skip("PR lib/55087");
#endif
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		signal(SIGINFO, syscall_sighand);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		syscall(SYS_getpid);

		if (strstr(op, "signal") != NULL) {
			FORKEE_ASSERT_EQ(test_syscall_caught, 1);
		}

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t and lwpid\n");
	ATF_REQUIRE(info.psi_lwpid > 0);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_SCE);

	if (strstr(op, "killed") != NULL) {
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_signaled(status, SIGKILL, 0);
	} else {
		if (strstr(op, "signal") != NULL) {
			DPRINTF("Before resuming the child %d and sending a "
			    "signal SIGINFO\n", child);
			SYSCALL_REQUIRE(
			    ptrace(PT_CONTINUE, child, (void *)1, SIGINFO)
			    != -1);
		} else if (strstr(op, "detach") != NULL) {
			DPRINTF("Before detaching the child %d\n", child);
			SYSCALL_REQUIRE(
			    ptrace(PT_DETACH, child, (void *)1, 0) != -1);
		} else {
			DPRINTF("Before resuming the child process where it "
			    "left off and without signal to be sent\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

			DPRINTF("Before calling %s() for the child\n",
			    TWAIT_FNAME);
			TWAIT_REQUIRE_SUCCESS(
			    wpid = TWAIT_GENERIC(child, &status, 0), child);

			validate_status_stopped(status, SIGTRAP);

			DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO "
			    "for child\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
			    != -1);

			DPRINTF("Before checking siginfo_t and lwpid\n");
			ATF_REQUIRE(info.psi_lwpid > 0);
			ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
			ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_SCX);

			DPRINTF("Before resuming the child process where it "
			    "left off and without signal to be sent\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
		}

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_exited(status, exitval);
	}

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define SYSCALL_TEST(name,op)						\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "timeout", "15");				\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that getpid(2) can be traced with PT_SYSCALL %s",	\
	   #op );							\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	syscall_body(op);						\
}

SYSCALL_TEST(syscall, "")
SYSCALL_TEST(syscall_killed_on_sce, "and killed")
SYSCALL_TEST(syscall_signal_on_sce, "and signaled")
SYSCALL_TEST(syscall_detach_on_sce, "and detached")

/// ----------------------------------------------------------------------------

ATF_TC(syscallemu1);
ATF_TC_HEAD(syscallemu1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that exit(2) can be intercepted with PT_SYSCALLEMU");
}

ATF_TC_BODY(syscallemu1, tc)
{
	const int exitval = 5;
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

		syscall(SYS_exit, 100);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Set SYSCALLEMU for intercepted syscall\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALLEMU, child, (void *)1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define ATF_TP_ADD_TCS_PTRACE_WAIT_SYSCALL() \
	ATF_TP_ADD_TC(tp, syscall); \
	ATF_TP_ADD_TC(tp, syscall_killed_on_sce); \
	ATF_TP_ADD_TC(tp, syscall_signal_on_sce); \
	ATF_TP_ADD_TC(tp, syscall_detach_on_sce); \
	ATF_TP_ADD_TC(tp, syscallemu1);
