/*	$NetBSD: t_ptrace.c,v 1.5 2016/11/03 18:25:54 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace.c,v 1.5 2016/11/03 18:25:54 kamil Exp $");

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

	printf("1: Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		/* printf(3) messages from a child aren't intercepted by ATF */
		/* "2: Child process PID=%d\n", getpid() */

		/* "2: Before calling ptrace(PT_TRACE_ME, ...)\n" */
		if (ptrace(PT_TRACE_ME, 0, NULL, 0) == -1) {
			/* XXX: Is it safe to use ATF functions in a child? */
			err(EXIT_FAILURE, "2: ptrace(2) call failed with "
			    "status %s", sys_errlist[errno]);
		}

		/* "2: Before raising SIGSTOP\n" */
		raise(sigval);

		/* "2: Before calling _exit(%d)\n", exitval */
		_exit(exitval);
	} else {
		printf("1: Parent process PID=%d, child's PID=%d\n", getpid(),
		    child);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating child's PID (expected %d, got %d)\n",
		    child, wpid);
		ATF_REQUIRE(child == wpid);

		printf("1: Ensuring that the child has not been exited\n");
		ATF_REQUIRE(!WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has been stopped\n");
		ATF_REQUIRE(WIFSTOPPED(status));

		printf("1: Verifying that he child has been stopped with the"
		    " %s signal (received %s)\n", sys_signame[sigval],
		    sys_signame[WSTOPSIG(status)]);
		ATF_REQUIRE(WSTOPSIG(status) == sigval);

		printf("1: Before resuming the child process where it left "
		    "off and without signal to be sent\n");
		ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID is still there\n");
		ATF_REQUIRE(wpid == child);

		printf("1: Ensuring that the child has been exited\n");
		ATF_REQUIRE(WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has not been stopped\n");
		ATF_REQUIRE(!WIFSTOPPED(status));

		printf("1: Verifying that he child has exited with the "
		    "%d status (received %d)\n", exitval, WEXITSTATUS(status));
		ATF_REQUIRE(WEXITSTATUS(status) == exitval);

		printf("1: Before calling waitpid() for the exited child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID no longer exists\n");
		ATF_REQUIRE(wpid == -1);

		printf("1: Validating that errno is set to %s (got %s)\n",
		    sys_errlist[ECHILD], sys_errlist[errno]);
		ATF_REQUIRE(errno == ECHILD);
	}
}

ATF_TC(traceme2);
ATF_TC_HEAD(traceme2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by _exit(2) in a child");
}

static int traceme2_catched = 0;

static void
traceme2_sighandler(int sig)
{
	if (sig != SIGINT)
		errx(EXIT_FAILURE, "sighandler: Unexpected signal received: "
		    "%s, expected SIGINT", sys_signame[sig]);

	++traceme2_catched;
}

ATF_TC_BODY(traceme2, tc)
{
	int status;
	const int exitval = 5;
	const int sigval = SIGSTOP, sigsent = SIGINT;
	pid_t child, wpid;
	struct sigaction sa;

	printf("1: Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		/* printf(3) messages from a child aren't intercepted by ATF */
		/* "2: Child process PID=%d\n", getpid() */

		/* "2: Before calling ptrace(PT_TRACE_ME, ...)\n" */
		if (ptrace(PT_TRACE_ME, 0, NULL, 0) == -1) {
			/* XXX: Is it safe to use ATF functions in a child? */
			err(EXIT_FAILURE, "2: ptrace(2) call failed with "
			    "status %s", sys_errlist[errno]);
		}

		/* "2: Setup sigaction(2) in the child" */
		sa.sa_handler = traceme2_sighandler;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		if (sigaction(sigsent, &sa, NULL) == -1)
			err(EXIT_FAILURE, "2: sigaction(2) call failed with "
			    "status %s", sys_errlist[errno]);

		/* "2: Before raising SIGSTOP\n" */
		raise(sigval);

		if (traceme2_catched != 1)
			errx(EXIT_FAILURE, "2: signal handler not called? "
			    "traceme2_catched (equal to %d) != 1",
			    traceme2_catched);

		/* "2: Before calling _exit(%d)\n", exitval */
		_exit(exitval);
	} else {
		printf("1: Parent process PID=%d, child's PID=%d\n", getpid(),
		    child);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating child's PID (expected %d, got %d)\n",
		    child, wpid);
		ATF_REQUIRE(child == wpid);

		printf("1: Ensuring that the child has not been exited\n");
		ATF_REQUIRE(!WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has been stopped\n");
		ATF_REQUIRE(WIFSTOPPED(status));

		printf("1: Verifying that he child has been stopped with the"
		    " %s signal (received %s)\n", sys_signame[sigval],
		    sys_signame[WSTOPSIG(status)]);
		ATF_REQUIRE(WSTOPSIG(status) == sigval);

		printf("1: Before resuming the child process where it left "
		    "off and with signal %s to be sent\n",
		    sys_signame[sigsent]);
		ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent)
		    != -1);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID is still there\n");
		ATF_REQUIRE(wpid == child);

		printf("1: Ensuring that the child has been exited\n");
		ATF_REQUIRE(WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has not been stopped\n");
		ATF_REQUIRE(!WIFSTOPPED(status));

		printf("1: Verifying that he child has exited with the "
		    "%d status (received %d)\n", exitval, WEXITSTATUS(status));
		ATF_REQUIRE(WEXITSTATUS(status) == exitval);

		printf("1: Before calling waitpid() for the exited child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID no longer exists\n");
		ATF_REQUIRE(wpid == -1);

		printf("1: Validating that errno is set to %s (got %s)\n",
		    sys_errlist[ECHILD], sys_errlist[errno]);
		ATF_REQUIRE(errno == ECHILD);
	}
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

	printf("1: Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		/* printf(3) messages from a child aren't intercepted by ATF */
		/* "2: Child process PID=%d\n", getpid() */

		/* "2: Before calling ptrace(PT_TRACE_ME, ...)\n" */
		if (ptrace(PT_TRACE_ME, 0, NULL, 0) == -1) {
			/* XXX: Is it safe to use ATF functions in a child? */
			err(EXIT_FAILURE, "2: ptrace(2) call failed with "
			    "status %s", sys_errlist[errno]);
		}

		/* "2: Before raising SIGSTOP\n" */
		raise(sigval);

		/* NOTREACHED */
		errx(EXIT_FAILURE, "2: Child should be terminated here by a "
		    "signal from parent");
	} else {
		printf("1: Parent process PID=%d, child's PID=%d\n", getpid(),
		    child);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating child's PID (expected %d, got %d)\n",
		    child, wpid);
		ATF_REQUIRE(child == wpid);

		printf("1: Ensuring that the child has not been exited\n");
		ATF_REQUIRE(!WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has been stopped\n");
		ATF_REQUIRE(WIFSTOPPED(status));

		printf("1: Verifying that he child has been stopped with the"
		    " %s signal (received %s)\n", sys_signame[sigval],
		    sys_signame[WSTOPSIG(status)]);
		ATF_REQUIRE(WSTOPSIG(status) == sigval);

		printf("1: Before resuming the child process where it left "
		    "off and with signal %s to be sent\n",
		    sys_signame[sigsent]);
		ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent)
		    != -1);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID is still there\n");
		ATF_REQUIRE(wpid == child);

		printf("1: Ensuring that the child has not been exited\n");
		ATF_REQUIRE(!WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child hast been terminated with "
		    "a signal\n");
		ATF_REQUIRE(WIFSIGNALED(status));

		printf("1: Ensuring that the child has not been stopped\n");
		ATF_REQUIRE(!WIFSTOPPED(status));

		printf("1: Verifying that he child has not core dumped "
		    "for the %s signal (it is the default behavior)\n",
		    sys_signame[sigsent]);
		ATF_REQUIRE(!WCOREDUMP(status));

		printf("1: Verifying that he child has been stopped with the"
		    " %s signal (received %s)\n", sys_signame[sigsent],
		    sys_signame[WTERMSIG(status)]);
		ATF_REQUIRE(WTERMSIG(status) == sigsent);

		printf("1: Before calling waitpid() for the exited child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID no longer exists\n");
		ATF_REQUIRE(wpid == -1);

		printf("1: Validating that errno is set to %s (got %s)\n",
		    sys_errlist[ECHILD], sys_errlist[errno]);
		ATF_REQUIRE(errno == ECHILD);
	}
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

	printf("1: Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		/* printf(3) messages from a child aren't intercepted by ATF */
		/* "2: Child process PID=%d\n", getpid() */

		/* "2: Before calling ptrace(PT_TRACE_ME, ...)\n" */
		if (ptrace(PT_TRACE_ME, 0, NULL, 0) == -1) {
			/* XXX: Is it safe to use ATF functions in a child? */
			err(EXIT_FAILURE, "2: ptrace(2) call failed with "
			    "status %s", sys_errlist[errno]);
		}

		/* "2: Before raising SIGSTOP\n" */
		raise(sigval);

		/* "2: Before raising SIGCONT\n" */
		raise(sigsent);

		/* "2: Before calling _exit(%d)\n", exitval */
		_exit(exitval);
	} else {
		printf("1: Parent process PID=%d, child's PID=%d\n", getpid(),
		    child);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating child's PID (expected %d, got %d)\n",
		    child, wpid);
		ATF_REQUIRE(child == wpid);

		printf("1: Ensuring that the child has not been exited\n");
		ATF_REQUIRE(!WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has been stopped\n");
		ATF_REQUIRE(WIFSTOPPED(status));

		printf("1: Verifying that he child has been stopped with the"
		    " %s signal (received %s)\n", sys_signame[sigval],
		    sys_signame[WSTOPSIG(status)]);
		ATF_REQUIRE(WSTOPSIG(status) == sigval);

		printf("1: Before resuming the child process where it left "
		    "off and without signal to be sent\n");
		ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0)
		    != -1);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, WALLSIG);

		printf("1: Validating that child's PID is still there\n");
		ATF_REQUIRE(wpid == child);

		printf("1: Ensuring that the child has not been exited\n");
		ATF_REQUIRE(!WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has not been stopped\n");
		ATF_REQUIRE(WIFSTOPPED(status));

		printf("1: Before resuming the child process where it left "
		    "off and without signal to be sent\n");
		ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

		printf("1: Before calling waitpid() for the child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID is still there\n");
		ATF_REQUIRE(wpid == child);

		printf("1: Ensuring that the child has been exited\n");
		ATF_REQUIRE(WIFEXITED(status));

		printf("1: Ensuring that the child has not been continued\n");
		ATF_REQUIRE(!WIFCONTINUED(status));

		printf("1: Ensuring that the child has not been terminated "
		    "with a signal\n");
		ATF_REQUIRE(!WIFSIGNALED(status));

		printf("1: Ensuring that the child has not been stopped\n");
		ATF_REQUIRE(!WIFSTOPPED(status));

		printf("1: Verifying that he child has exited with the "
		    "%d status (received %d)\n", exitval, WEXITSTATUS(status));
		ATF_REQUIRE(WEXITSTATUS(status) == exitval);

		printf("1: Before calling waitpid() for the exited child\n");
		wpid = waitpid(child, &status, 0);

		printf("1: Validating that child's PID no longer exists\n");
		ATF_REQUIRE(wpid == -1);

		printf("1: Validating that errno is set to %s (got %s)\n",
		    sys_errlist[ECHILD], sys_errlist[errno]);
		ATF_REQUIRE(errno == ECHILD);
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, traceme1);
	ATF_TP_ADD_TC(tp, traceme2);
	ATF_TP_ADD_TC(tp, traceme3);
	ATF_TP_ADD_TC(tp, traceme4);

	return atf_no_error();
}
