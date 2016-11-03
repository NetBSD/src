/*	$NetBSD: t_ptrace.c,v 1.7 2016/11/03 20:24:18 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace.c,v 1.7 2016/11/03 20:24:18 kamil Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
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


ATF_TC(traceme1);
ATF_TC_HEAD(traceme1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by _exit(2) in a child");
}

ATF_TC_BODY(traceme1, tc)
{
	int status;
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		FORKEE_ASSERT(raise(sigval) == 0);

		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating child's PID (expected %d, got %d)\n", child, wpid);
	ATF_REQUIRE(child == wpid);

	printf("Ensuring that the child has not been exited\n");
	ATF_REQUIRE(!WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has been stopped\n");
	ATF_REQUIRE(WIFSTOPPED(status));

	printf("Verifying that he child has been stopped with the %s signal "
	    "(received %s)\n", sys_signame[sigval],
	    sys_signame[WSTOPSIG(status)]);
	ATF_REQUIRE(WSTOPSIG(status) == sigval);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID is still there\n");
	ATF_REQUIRE(wpid == child);

	printf("Ensuring that the child has been exited\n");
	ATF_REQUIRE(WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has not been stopped\n");
	ATF_REQUIRE(!WIFSTOPPED(status));

	printf("Verifying that he child has exited with the %d status "
	    "(received %d)\n", exitval, WEXITSTATUS(status));
	ATF_REQUIRE(WEXITSTATUS(status) == exitval);

	printf("Before calling waitpid() for the exited child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID no longer exists\n");
	ATF_REQUIRE(wpid == -1);

	printf("Validating that errno is set to %s (got %s)\n",
	    strerror(ECHILD), strerror(errno));
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(traceme2);
ATF_TC_HEAD(traceme2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by _exit(2) in a child");
}

static int traceme2_caught = 0;

static void
traceme2_sighandler(int sig)
{
	FORKEE_ASSERTX(sig == SIGINT);

	++traceme2_caught;
}

ATF_TC_BODY(traceme2, tc)
{
	int status;
	const int exitval = 5;
	const int sigval = SIGSTOP, sigsent = SIGINT;
	pid_t child, wpid;
	struct sigaction sa;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sa.sa_handler = traceme2_sighandler;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sigsent, &sa, NULL) != -1);

		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERTX(traceme2_caught == 1);

		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating child's PID (expected %d, got %d)\n", child, wpid);
	ATF_REQUIRE(child == wpid);

	printf("Ensuring that the child has not been exited\n");
	ATF_REQUIRE(!WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has been stopped\n");
	ATF_REQUIRE(WIFSTOPPED(status));

	printf("Verifying that he child has been stopped with the %s signal "
	    "(received %s)\n", sys_signame[sigval],
	    sys_signame[WSTOPSIG(status)]);
	ATF_REQUIRE(WSTOPSIG(status) == sigval);

	printf("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", sys_signame[sigsent]);
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID is still there\n");
	ATF_REQUIRE(wpid == child);

	printf("Ensuring that the child has been exited\n");
	ATF_REQUIRE(WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has not been stopped\n");
	ATF_REQUIRE(!WIFSTOPPED(status));

	printf("Verifying that he child has exited with the %d status "
	    "(received %d)\n", exitval, WEXITSTATUS(status));
	ATF_REQUIRE(WEXITSTATUS(status) == exitval);

	printf("Before calling waitpid() for the exited child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID no longer exists\n");
	ATF_REQUIRE(wpid == -1);

	printf("Validating that errno is set to %s (got %s)\n",
	    strerror(ECHILD), strerror(errno));
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(traceme3);
ATF_TC_HEAD(traceme3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by termination by a signal in a child");
}

ATF_TC_BODY(traceme3, tc)
{
	int status;
	const int sigval = SIGSTOP, sigsent = SIGINT /* Without core-dump */;
	pid_t child, wpid;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 &&
		    "Child should be terminated by a signal from its parent");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating child's PID (expected %d, got %d)\n", child, wpid);
	ATF_REQUIRE(child == wpid);

	printf("Ensuring that the child has not been exited\n");
	ATF_REQUIRE(!WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has been stopped\n");
	ATF_REQUIRE(WIFSTOPPED(status));

	printf("Verifying that he child has been stopped with the %s signal "
	    "(received %s)\n", sys_signame[sigval],
	    sys_signame[WSTOPSIG(status)]);
	ATF_REQUIRE(WSTOPSIG(status) == sigval);

	printf("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", sys_signame[sigsent]);
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID is still there\n");
	ATF_REQUIRE(wpid == child);

	printf("Ensuring that the child has not been exited\n");
	ATF_REQUIRE(!WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child hast been terminated with a "
	    "signal\n");
	ATF_REQUIRE(WIFSIGNALED(status));

	printf("Ensuring that the child has not been stopped\n");
	ATF_REQUIRE(!WIFSTOPPED(status));

	printf("Verifying that he child has not core dumped for the %s signal "
	    "(it is the default behavior)\n", sys_signame[sigsent]);
	ATF_REQUIRE(!WCOREDUMP(status));

	printf("Verifying that he child has been stopped with the %s "
	    "signal (received %s)\n", sys_signame[sigsent],
	    sys_signame[WTERMSIG(status)]);
	ATF_REQUIRE(WTERMSIG(status) == sigsent);

	printf("Before calling waitpid() for the exited child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID no longer exists\n");
	ATF_REQUIRE(wpid == -1);

	printf("Validating that errno is set to %s (got %s)\n",
	    strerror(ECHILD), strerror(errno));
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TC(traceme4);
ATF_TC_HEAD(traceme4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by SIGCONT and _exit(2) in a child");
}

ATF_TC_BODY(traceme4, tc)
{
	int status;
	const int exitval = 5;
	const int sigval = SIGSTOP, sigsent = SIGCONT;
	pid_t child, wpid;

	/* XXX: Linux&FreeBSD and NetBSD have different behavior here.
	 * Assume that they are right and NetBSD is wrong.
	 * The ptrace(2) interface is out of POSIX scope so there is no
	 * ultimate standard to verify it. */
	atf_tc_expect_fail("PR kern/51596");

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT(raise(sigsent) == 0);

		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(),child);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating child's PID (expected %d, got %d)\n", child, wpid);
	ATF_REQUIRE(child == wpid);

	printf("Ensuring that the child has not been exited\n");
	ATF_REQUIRE(!WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has been stopped\n");
	ATF_REQUIRE(WIFSTOPPED(status));

	printf("Verifying that he child has been stopped with the %s signal "
	    "(received %s)\n", sys_signame[sigval],
	    sys_signame[WSTOPSIG(status)]);
	ATF_REQUIRE(WSTOPSIG(status) == sigval);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, WALLSIG);

	printf("Validating that child's PID is still there\n");
	ATF_REQUIRE(wpid == child);

	printf("Ensuring that the child has not been exited\n");
	ATF_REQUIRE(!WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has not been stopped\n");
	ATF_REQUIRE(WIFSTOPPED(status));

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling waitpid() for the child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID is still there\n");
	ATF_REQUIRE(wpid == child);

	printf("Ensuring that the child has been exited\n");
	ATF_REQUIRE(WIFEXITED(status));

	printf("Ensuring that the child has not been continued\n");
	ATF_REQUIRE(!WIFCONTINUED(status));

	printf("Ensuring that the child has not been terminated with a "
	    "signal\n");
	ATF_REQUIRE(!WIFSIGNALED(status));

	printf("Ensuring that the child has not been stopped\n");
	ATF_REQUIRE(!WIFSTOPPED(status));

	printf("Verifying that he child has exited with the %d status "
	    "(received %d)\n", exitval, WEXITSTATUS(status));
	ATF_REQUIRE(WEXITSTATUS(status) == exitval);

	printf("Before calling waitpid() for the exited child\n");
	wpid = waitpid(child, &status, 0);

	printf("Validating that child's PID no longer exists\n");
	ATF_REQUIRE(wpid == -1);

	printf("Validating that errno is set to %s (got %s)\n",
	    strerror(ECHILD), strerror(errno));
	ATF_REQUIRE(errno == ECHILD);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, traceme1);
	ATF_TP_ADD_TC(tp, traceme2);
	ATF_TP_ADD_TC(tp, traceme3);
	ATF_TP_ADD_TC(tp, traceme4);

	return atf_no_error();
}
