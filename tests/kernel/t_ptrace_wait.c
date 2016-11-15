/*	$NetBSD: t_ptrace_wait.c,v 1.15 2016/11/15 02:53:32 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.15 2016/11/15 02:53:32 kamil Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <atf-c.h>

#include "../h_macros.h"

/* Detect plain wait(2) use-case */
#if !defined(TWAIT_WAITPID) && \
    !defined(TWAIT_WAITID) && \
    !defined(TWAIT_WAIT3) && \
    !defined(TWAIT_WAIT4) && \
    !defined(TWAIT_WAIT6)
#define TWAIT_WAIT
#endif

/*
 * There are two classes of wait(2)-like functions:
 * - wait4(2)-like accepting pid_t, optional options parameter, struct rusage*
 * - wait6(2)-like accepting idtype_t, id_t, struct wrusage, mandatory options
 *
 * The TWAIT_FNAME value is to be used for convenience in debug messages.
 *
 * The TWAIT_GENERIC() macro is designed to reuse the same unmodified
 * code with as many wait(2)-like functions as possible.
 *
 * In a common use-case wait4(2) and wait6(2)-like function can work the almost
 * the same way, however there are few important differences:
 * wait6(2) must specify P_PID for idtype to match wpid from wait4(2).
 * To behave like wait4(2), wait6(2) the 'options' to wait must include
 * WEXITED|WTRUNCATED.
 *
 * There are two helper macros (they purpose it to mach more than one
 * wait(2)-like function):
 * The TWAIT_HAVE_STATUS - specifies whether a function can retrieve
 *                         status (as integer value).
 * The TWAIT_HAVE_PID    - specifies whether a function can request
 *                         exact process identifier
 * The TWAIT_HAVE_RUSAGE - specifies whether a function can request
 *                         the struct rusage value
 *
 */

#if defined(TWAIT_WAIT)
#	define TWAIT_FNAME			"wait"
#	define TWAIT_WAIT4TYPE(a,b,c,d)		wait((b))
#	define TWAIT_GENERIC(a,b,c)		wait((b))
#	define TWAIT_HAVE_STATUS		1
#elif defined(TWAIT_WAITPID)
#	define TWAIT_FNAME			"waitpid"
#	define TWAIT_WAIT4TYPE(a,b,c,d)		waitpid((a),(b),(c))
#	define TWAIT_GENERIC(a,b,c)		waitpid((a),(b),(c))
#	define TWAIT_HAVE_PID			1
#	define TWAIT_HAVE_STATUS		1
#elif defined(TWAIT_WAITID)
#	define TWAIT_FNAME			"waitid"
#	define TWAIT_GENERIC(a,b,c)		\
		waitid(P_PID,(a),NULL,(c)|WEXITED|WTRAPPED)
#	define TWAIT_WAIT6TYPE(a,b,c,d,e,f)	waitid((a),(b),(f),(d))
#	define TWAIT_HAVE_PID			1
#elif defined(TWAIT_WAIT3)
#	define TWAIT_FNAME			"wait3"
#	define TWAIT_WAIT4TYPE(a,b,c,d)		wait3((b),(c),(d))
#	define TWAIT_GENERIC(a,b,c)		wait3((b),(c),NULL)
#	define TWAIT_HAVE_STATUS		1
#	define TWAIT_HAVE_RUSAGE		1
#elif defined(TWAIT_WAIT4)
#	define TWAIT_FNAME			"wait4"
#	define TWAIT_WAIT4TYPE(a,b,c,d)		wait4((a),(b),(c),(d))
#	define TWAIT_GENERIC(a,b,c)		wait4((a),(b),(c),NULL)
#	define TWAIT_HAVE_PID			1
#	define TWAIT_HAVE_STATUS		1
#	define TWAIT_HAVE_RUSAGE		1
#elif defined(TWAIT_WAIT6)
#	define TWAIT_FNAME			"wait6"
#	define TWAIT_WAIT6TYPE(a,b,c,d,e,f)	wait6((a),(b),(c),(d),(e),(f))
#	define TWAIT_GENERIC(a,b,c)		\
		wait6(P_PID,(a),(b),(c)|WEXITED|WTRAPPED,NULL,NULL)
#	define TWAIT_HAVE_PID			1
#	define TWAIT_HAVE_STATUS		1
#endif

/*
 * There are 3 groups of tests:
 * - TWAIT_GENERIC()	(wait, wait2, waitpid, wait3, wait4, wait6)
 * - TWAIT_WAIT4TYPE()	(wait2, waitpid, wait3, wait4)
 * - TWAIT_WAIT6TYPE()	(waitid, wait6)
 *
 * Tests only in the above categories are allowed. However some tests are not
 * possible in the context requested functionality to be verified, therefore
 * there are helper macros:
 * - TWAIT_HAVE_PID	(wait2, waitpid, waitid, wait4, wait6)
 * - TWAIT_HAVE_STATUS	(wait, wait2, waitpid, wait3, wait4, wait6)
 * - TWAIT_HAVE_RUSAGE	(wait3, wait4)
 * - TWAIT_HAVE_RETPID	(wait, wait2, waitpid, wait3, wait4, wait6)
 *
 * If there is an intention to test e.g. wait6(2) specific features in the
 * ptrace(2) context, find the most matching group and with #ifdefs reduce
 * functionality of less featured than wait6(2) interface (TWAIT_WAIT6TYPE).
 *
 * For clarity never use negative preprocessor checks, like:
 *     #if !defined(TWAIT_WAIT4)
 * always refer to checks for positive values.
 */

/*
 * A child process cannot call atf functions and expect them to magically
 * work like in the parent.
 * The printf(3) messaging from a child will not work out of the box as well
 * without estabilishing a communication protocol with its parent. To not
 * overcomplicate the tests - do not log from a child and use err(3)/errx(3)
 * wrapped with FORKEE_ASSERT()/FORKEE_ASSERTX() as that is guaranteed to work.
 */
#define FORKEE_ASSERT_EQ(x, y)						\
do {									\
	int ret = (x) == (y);						\
	if (!ret)							\
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: "	\
		    "%s(%d) == %s(%d)", __FILE__, __LINE__, __func__,	\
		    #x, (int)x, #y, (int)y);				\
} while (/*CONSTCOND*/0)

#define FORKEE_ASSERTX(x)						\
do {									\
	int ret = (x);							\
	if (!ret)							\
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",\
		     __FILE__, __LINE__, __func__, #x);			\
} while (/*CONSTCOND*/0)

#define FORKEE_ASSERT(x)						\
do {									\
	int ret = (x);							\
	if (!ret)							\
		err(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",\
		     __FILE__, __LINE__, __func__, #x);			\
} while (/*CONSTCOND*/0)

/*
 * If waitid(2) returns because one or more processes have a state change to
 * report, 0 is returned.  If an error is detected, a value of -1 is returned
 * and errno is set to indicate the error. If WNOHANG is specified and there
 * are no stopped, continued or exited children, 0 is returned.
 */
#if defined(TWAIT_WAITID)
#define TWAIT_REQUIRE_SUCCESS(a,b)	ATF_REQUIRE((a) == 0)
#define TWAIT_REQUIRE_FAILURE(a,b)	ATF_REQUIRE_ERRNO((a),(b) == -1)
#define FORKEE_REQUIRE_SUCCESS(a,b)	FORKEE_ASSERTX((a) == 0)
#define FORKEE_REQUIRE_FAILURE(a,b)	\
	FORKEE_ASSERTX(((a) == errno) && ((b) == -1))
#else
#define TWAIT_REQUIRE_SUCCESS(a,b)	ATF_REQUIRE((a) == (b))
#define TWAIT_REQUIRE_FAILURE(a,b)	ATF_REQUIRE_ERRNO((a),(b) == -1)
#define FORKEE_REQUIRE_SUCCESS(a,b)	FORKEE_ASSERTX((a) == (b))
#define FORKEE_REQUIRE_FAILURE(a,b)	\
	FORKEE_ASSERTX(((a) == errno) && ((b) == -1))
#endif

/*
 * Helper tools to verify whether status reports exited value
 */
#if TWAIT_HAVE_STATUS
static void __used
validate_status_exited(int status, int expected)
{
        ATF_REQUIRE_MSG(WIFEXITED(status), "Reported exited process");
        ATF_REQUIRE_MSG(!WIFCONTINUED(status), "Reported continued process");
        ATF_REQUIRE_MSG(!WIFSIGNALED(status), "Reported signaled process");
        ATF_REQUIRE_MSG(!WIFSTOPPED(status), "Reported stopped process");

	ATF_REQUIRE_EQ_MSG(WEXITSTATUS(status), expected,
	    "The process has exited with invalid value %d != %d",
	    WEXITSTATUS(status), expected);
}

static void __used
forkee_status_exited(int status, int expected)
{
	FORKEE_ASSERTX(WIFEXITED(status));
	FORKEE_ASSERTX(!WIFCONTINUED(status));
	FORKEE_ASSERTX(!WIFSIGNALED(status));
	FORKEE_ASSERTX(!WIFSTOPPED(status));

	FORKEE_ASSERT_EQ(WEXITSTATUS(status), expected);
}

static void __used
validate_status_continued(int status)
{
	ATF_REQUIRE_MSG(!WIFEXITED(status), "Reported exited process");
	ATF_REQUIRE_MSG(WIFCONTINUED(status), "Reported continued process");
	ATF_REQUIRE_MSG(!WIFSIGNALED(status), "Reported signaled process");
	ATF_REQUIRE_MSG(!WIFSTOPPED(status), "Reported stopped process");
}

static void __used
forkee_status_continued(int status)
{
	FORKEE_ASSERTX(!WIFEXITED(status));
	FORKEE_ASSERTX(WIFCONTINUED(status));
	FORKEE_ASSERTX(!WIFSIGNALED(status));
	FORKEE_ASSERTX(!WIFSTOPPED(status));
}

static void __used
validate_status_signaled(int status, int expected_termsig, int expected_core)
{
	ATF_REQUIRE_MSG(!WIFEXITED(status), "Reported exited process");
	ATF_REQUIRE_MSG(!WIFCONTINUED(status), "Reported continued process");
	ATF_REQUIRE_MSG(WIFSIGNALED(status), "Reported signaled process");
	ATF_REQUIRE_MSG(!WIFSTOPPED(status), "Reported stopped process");

	ATF_REQUIRE_EQ_MSG(WTERMSIG(status), expected_termsig,
	    "Unexpected signal received");

	ATF_REQUIRE_EQ_MSG(WCOREDUMP(status), expected_core,
	    "Unexpectedly core file %s generated", expected_core ? "not" : "");
}

static void __used
forkee_status_signaled(int status, int expected_termsig, int expected_core)
{
	FORKEE_ASSERTX(!WIFEXITED(status));
	FORKEE_ASSERTX(!WIFCONTINUED(status));
	FORKEE_ASSERTX(WIFSIGNALED(status));
	FORKEE_ASSERTX(!WIFSTOPPED(status));

	FORKEE_ASSERT_EQ(WTERMSIG(status), expected_termsig);
	FORKEE_ASSERT_EQ(WCOREDUMP(status), expected_core);
}

static void __used
validate_status_stopped(int status, int expected)
{
	ATF_REQUIRE_MSG(!WIFEXITED(status), "Reported exited process");
	ATF_REQUIRE_MSG(!WIFCONTINUED(status), "Reported continued process");
	ATF_REQUIRE_MSG(!WIFSIGNALED(status), "Reported signaled process");
	ATF_REQUIRE_MSG(WIFSTOPPED(status), "Reported stopped process");

	char st[128], ex[128];
	strlcpy(st, strsignal(WSTOPSIG(status)), sizeof(st));
	strlcpy(ex, strsignal(expected), sizeof(ex));

	ATF_REQUIRE_EQ_MSG(WSTOPSIG(status), expected,
	    "Unexpected stop signal received [%s] != [%s]", st, ex);
}

static void __used
forkee_status_stopped(int status, int expected)
{
	FORKEE_ASSERTX(!WIFEXITED(status));
	FORKEE_ASSERTX(!WIFCONTINUED(status));
	FORKEE_ASSERTX(!WIFSIGNALED(status));
	FORKEE_ASSERTX(WIFSTOPPED(status));

	FORKEE_ASSERT_EQ(WSTOPSIG(status), expected);
}
#else
#define validate_status_exited(a,b)
#define forkee_status_exited(a,b)
#define validate_status_continued(a,b)
#define forkee_status_continued(a,b)
#define validate_status_signaled(a,b,c)
#define forkee_status_signaled(a,b,c)
#define validate_status_stopped(a,b)
#define forkee_status_stopped(a,b)
#endif

#if defined(TWAIT_HAVE_PID)
/* This function is currently designed to be run in the main/parent process */
static void
await_zombie(pid_t process)
{
	struct kinfo_proc2 p;
	size_t len = sizeof(p);

	const int name[] = {
		[0] = CTL_KERN,
		[1] = KERN_PROC2,
		[2] = KERN_PROC_PID,
		[3] = process,
		[4] = sizeof(p),
		[5] = 1
	};

	const size_t namelen = __arraycount(name);

	/* Await the process becoming a zombie */
	while(1) {
		ATF_REQUIRE(sysctl(name, namelen, &p, &len, NULL, 0) == 0);

		if (p.p_stat == LSZOMB)
			break;

		ATF_REQUIRE(usleep(1000) == 0);
	}
}
#endif

ATF_TC(traceme1);
ATF_TC_HEAD(traceme1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by _exit(2) in a child");
}

ATF_TC_BODY(traceme1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Before forking process PID=%d\n", getpid());
	child = atf_utils_fork();
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
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
	FORKEE_ASSERT_EQ(sig, SIGINT);

	++traceme2_caught;
}

ATF_TC_BODY(traceme2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP, sigsent = SIGINT;
	pid_t child, wpid;
	struct sigaction sa;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Before forking process PID=%d\n", getpid());
	child = atf_utils_fork();
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sa.sa_handler = traceme2_sighandler;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sigsent, &sa, NULL) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(traceme2_caught, 1);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", strsignal(sigsent));
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(traceme3);
ATF_TC_HEAD(traceme3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by termination by a signal in a child");
}

ATF_TC_BODY(traceme3, tc)
{
	const int sigval = SIGSTOP, sigsent = SIGINT /* Without core-dump */;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Before forking process PID=%d\n", getpid());
	child = atf_utils_fork();
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 &&
		    "Child should be terminated by a signal from its parent");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", strsignal(sigsent));
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, sigsent, 0);

	printf("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(traceme4);
ATF_TC_HEAD(traceme4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify SIGSTOP followed by SIGCONT and _exit(2) in a child");
}

ATF_TC_BODY(traceme4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP, sigsent = SIGCONT;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Before forking process PID=%d\n", getpid());
	child = atf_utils_fork();
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before raising %s from child\n", strsignal(sigsent));
		FORKEE_ASSERT(raise(sigsent) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(),child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigsent);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach1);
ATF_TC_HEAD(attach1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees process termination before the parent");
}

ATF_TC_BODY(attach1, tc)
{
	int fds_totracee[2], fds_totracer[2], fds_fromtracer[2];
	int rv;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Spawn tracee\n");
	ATF_REQUIRE(pipe(fds_totracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);

		/* Wait for message from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		_exit(exitval_tracee);
	}
	ATF_REQUIRE(close(fds_totracee[0]) == 0);

	printf("Spawn debugger\n");
	ATF_REQUIRE(pipe(fds_totracer) == 0);
	ATF_REQUIRE(pipe(fds_fromtracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);

		FORKEE_ASSERT(close(fds_totracer[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracer[0]) == 0);

		printf("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		rv = write(fds_fromtracer[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		printf("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}
	ATF_REQUIRE(close(fds_totracer[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracer[1]) == 0);

	printf("Wait for the tracer to attach to the tracee\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Resume the tracee and let it exit\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("fds_totracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracee[1]) == 0);

	printf("Detect that tracee is zombie\n");
	await_zombie(tracee);

	printf("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	printf("Resume the tracer and let it detect exited tracee\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	printf("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	printf("fds_fromtracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracer[0]) == 0);

	printf("fds_totracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracer[1]) == 0);
}
#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach2);
ATF_TC_HEAD(attach2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that any tracer sees process termination before its "
	    "parent");
}

ATF_TC_BODY(attach2, tc)
{
	int fds_totracee[2], fds_totracer[2], fds_fromtracer[2];
	int rv;
	const int exitval_tracee = 5;
	const int exitval_tracer1 = 10, exitval_tracer2 = 20;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Spawn tracee\n");
	ATF_REQUIRE(pipe(fds_totracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);

		/* Wait for message from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		_exit(exitval_tracee);
	}
	ATF_REQUIRE(close(fds_totracee[0]) == 0);

	printf("Spawn debugger\n");
	ATF_REQUIRE(pipe(fds_totracer) == 0);
	ATF_REQUIRE(pipe(fds_fromtracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);

		FORKEE_ASSERT(close(fds_totracer[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracer[0]) == 0);

		/* Fork again and drop parent to reattach to PID 1 */
		tracer = atf_utils_fork();
		if (tracer != 0)
			_exit(exitval_tracer1);

		printf("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		rv = write(fds_fromtracer[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		printf("Before exiting of the tracer process\n");
		_exit(exitval_tracer2);
	}
	ATF_REQUIRE(close(fds_totracer[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracer[1]) == 0);

	printf("Wait for the tracer process (direct child) to exit calling "
	    "%s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracer, &status, 0), tracer);

	validate_status_exited(status, exitval_tracer1);

	printf("Wait for the non-exited tracee process with %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, NULL, WNOHANG), 0);

	printf("Wait for the tracer to attach to the tracee\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Resume the tracee and let it exit\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("fds_totracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracee[1]) == 0);

	printf("Detect that tracee is zombie\n");
	await_zombie(tracee);

	printf("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	printf("Resume the tracer and let it detect exited tracee\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracer to exit (pipe broken expected)\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == 0);

	printf("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	printf("fds_fromtracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracer[0]) == 0);

	printf("fds_totracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracer[1]) == 0);
}
#endif

ATF_TC(attach3);
ATF_TC_HEAD(attach3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer parent can PT_ATTACH to its child");
}

ATF_TC_BODY(attach3, tc)
{
	int fds_totracee[2], fds_fromtracee[2];
	int rv;
	const int exitval_tracee = 5;
	pid_t tracee, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Spawn tracee\n");
	ATF_REQUIRE(pipe(fds_totracee) == 0);
	ATF_REQUIRE(pipe(fds_fromtracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		/* Wait for message 1 from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		/* Send response to parent */
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		/* Wait for message 2 from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		printf("Parent should now attach to tracee\n");

		/* Write to pearent we are ok */
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		/* Wait for message from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		_exit(exitval_tracee);
	}
	ATF_REQUIRE(close(fds_totracee[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracee[1]) == 0);

	printf("Send message 1 to tracee\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));
	printf("Wait for response from tracee\n");
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));
	printf("Send message 2 to tracee\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Before calling PT_ATTACH for tracee %d\n", tracee);
	ATF_REQUIRE(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

	printf("Wait for the stopped tracee process with %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

	validate_status_stopped(status, SIGSTOP);

	printf("Resume tracee with PT_CONTINUE\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

	printf("Let the tracee exit now\n");
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracee to exit with %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

	validate_status_exited(status, exitval_tracee);

	printf("Before calling %s() for tracee\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD,
	    wpid = TWAIT_GENERIC(tracee, &status, 0));

	printf("fds_fromtracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracee[0]) == 0);

	printf("fds_totracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracee[1]) == 0);
}

ATF_TC(attach4);
ATF_TC_HEAD(attach4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer child can PT_ATTACH to its parent");
}

ATF_TC_BODY(attach4, tc)
{
	int fds_totracer[2], fds_fromtracer[2];
	int rv;
	const int exitval_tracer = 5;
	pid_t tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Spawn tracer\n");
	ATF_REQUIRE(pipe(fds_totracer) == 0);
	ATF_REQUIRE(pipe(fds_fromtracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		FORKEE_ASSERT(close(fds_totracer[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracer[0]) == 0);

		/* Wait for message from the parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		printf("Attach to parent PID %d with PT_ATTACH from child\n",
		    getppid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, getppid(), NULL, 0) != -1);

		printf("Wait for the stopped parent process with %s()\n",
		    TWAIT_FNAME);
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(getppid(), &status, 0), getppid());

		forkee_status_stopped(status, SIGSTOP);

		printf("Resume parent with PT_DETACH\n");
		FORKEE_ASSERT(ptrace(PT_DETACH, getppid(), (void *)1, 0)
		    != -1);

		/* Tell parent we are ready */
		rv = write(fds_fromtracer[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		/* Wait for message from the parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		_exit(exitval_tracer);
	}
	ATF_REQUIRE(close(fds_totracer[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracer[1]) == 0);

	printf("Wait for the tracer to become ready\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for the tracer to resume\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));
	printf("Allow the tracer to exit now\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracer to exit with %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracer, &status, 0), tracer);

	validate_status_exited(status, exitval_tracer);

	printf("Before calling %s() for tracer\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD,
	    wpid = TWAIT_GENERIC(tracer, &status, 0));

	printf("fds_fromtracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracer[0]) == 0);

	printf("fds_totracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracer[1]) == 0);
}

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach5);
ATF_TC_HEAD(attach5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees its parent when attached to tracer "
	    "(check getppid(2))");
}

ATF_TC_BODY(attach5, tc)
{
	int fds_totracee[2], fds_fromtracee[2];
	int fds_totracer[2], fds_fromtracer[2];
	int rv;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	printf("Spawn tracee\n");
	ATF_REQUIRE(pipe(fds_totracee) == 0);
	ATF_REQUIRE(pipe(fds_fromtracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		parent = getppid();

		/* Emit message to the parent */
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for message from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		FORKEE_ASSERT_EQ(parent, getppid());

		_exit(exitval_tracee);
	}
	ATF_REQUIRE(close(fds_totracee[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracee[1]) == 0);

	printf("Wait for child to record its parent identifier (pid)\n");
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));

	printf("Spawn debugger\n");
	ATF_REQUIRE(pipe(fds_totracer) == 0);
	ATF_REQUIRE(pipe(fds_fromtracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		FORKEE_ASSERT(close(fds_totracer[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracer[0]) == 0);

		printf("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		rv = write(fds_fromtracer[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		printf("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}
	ATF_REQUIRE(close(fds_totracer[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracer[1]) == 0);

	printf("Wait for the tracer to attach to the tracee\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Resume the tracee and let it exit\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("fds_totracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracee[1]) == 0);

	printf("fds_fromtracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracee[0]) == 0);

	printf("Detect that tracee is zombie\n");
	await_zombie(tracee);

	printf("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	printf("Resume the tracer and let it detect exited tracee\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	printf("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	printf("fds_fromtracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracer[0]) == 0);

	printf("fds_totracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracer[1]) == 0);
}
#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach6);
ATF_TC_HEAD(attach6, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees its parent when attached to tracer "
	    "(check sysctl(7) and struct kinfo_proc2)");
}

ATF_TC_BODY(attach6, tc)
{
	int fds_totracee[2], fds_fromtracee[2];
	int fds_totracer[2], fds_fromtracer[2];
	int rv;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	int name[CTL_MAXNAME];
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);
	unsigned int namelen;

	printf("Spawn tracee\n");
	ATF_REQUIRE(pipe(fds_totracee) == 0);
	ATF_REQUIRE(pipe(fds_fromtracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		parent = getppid();

		/* Emit message to the parent */
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for message from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		namelen = 0;
		name[namelen++] = CTL_KERN;
		name[namelen++] = KERN_PROC2;
		name[namelen++] = KERN_PROC_PID;
		name[namelen++] = getpid();
		name[namelen++] = len;
		name[namelen++] = 1;

		FORKEE_ASSERT(sysctl(name, namelen, &kp, &len, NULL, 0) == 0);
		FORKEE_ASSERT_EQ(parent, kp.p_ppid);

		_exit(exitval_tracee);
	}
	ATF_REQUIRE(close(fds_totracee[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracee[1]) == 0);

	printf("Wait for child to record its parent identifier (pid)\n");
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));

	printf("Spawn debugger\n");
	ATF_REQUIRE(pipe(fds_totracer) == 0);
	ATF_REQUIRE(pipe(fds_fromtracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		FORKEE_ASSERT(close(fds_totracer[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracer[0]) == 0);

		printf("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		rv = write(fds_fromtracer[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		printf("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}
	ATF_REQUIRE(close(fds_totracer[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracer[1]) == 0);

	printf("Wait for the tracer to attach to the tracee\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Resume the tracee and let it exit\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("fds_totracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracee[1]) == 0);

	printf("fds_fromtracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracee[0]) == 0);

	printf("Detect that tracee is zombie\n");
	await_zombie(tracee);

	printf("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	printf("Resume the tracer and let it detect exited tracee\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	printf("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	printf("fds_fromtracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracer[0]) == 0);

	printf("fds_totracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracer[1]) == 0);
}
#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach7);
ATF_TC_HEAD(attach7, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees its parent when attached to tracer "
	    "(check /proc/curproc/status 3rd column)");
}

ATF_TC_BODY(attach7, tc)
{
	int fds_totracee[2], fds_fromtracee[2];
	int fds_totracer[2], fds_fromtracer[2];
	int rv;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	FILE *fp;
	struct stat st;
	const char *fname = "/proc/curproc/status";
	char s_executable[MAXPATHLEN];
	int s_pid, s_ppid;
	/*
	 * Format:
	 *  EXECUTABLE PID PPID ...
	 */

	ATF_REQUIRE((rv = stat(fname, &st)) == 0 || (errno == ENOENT));
	if (rv != 0) {
		atf_tc_skip("/proc/curproc/status not found");
	}

	printf("Spawn tracee\n");
	ATF_REQUIRE(pipe(fds_totracee) == 0);
	ATF_REQUIRE(pipe(fds_fromtracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		parent = getppid();

		/* Emit message to the parent */
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));
		rv = write(fds_fromtracee[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for message from the parent */
		rv = read(fds_totracee[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		FORKEE_ASSERT((fp = fopen(fname, "r")) != NULL);
		fscanf(fp, "%s %d %d", s_executable, &s_pid, &s_ppid);
		FORKEE_ASSERT(fclose(fp) == 0);
		FORKEE_ASSERT_EQ(parent, s_ppid);

		_exit(exitval_tracee);
	}
	ATF_REQUIRE(close(fds_totracee[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracee[1]) == 0);

	printf("Wait for child to record its parent identifier (pid)\n");
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));
	rv = read(fds_fromtracee[0], &msg, sizeof(msg));
	FORKEE_ASSERT(rv == sizeof(msg));

	printf("Spawn debugger\n");
	ATF_REQUIRE(pipe(fds_totracer) == 0);
	ATF_REQUIRE(pipe(fds_fromtracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		FORKEE_ASSERT(close(fds_totracee[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracee[0]) == 0);

		FORKEE_ASSERT(close(fds_totracer[1]) == 0);
		FORKEE_ASSERT(close(fds_fromtracer[0]) == 0);

		printf("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		rv = write(fds_fromtracer[1], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for parent */
		rv = read(fds_totracer[0], &msg, sizeof(msg));
		FORKEE_ASSERT(rv == sizeof(msg));

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		printf("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}
	ATF_REQUIRE(close(fds_totracer[0]) == 0);
	ATF_REQUIRE(close(fds_fromtracer[1]) == 0);

	printf("Wait for the tracer to attach to the tracee\n");
	rv = read(fds_fromtracer[0], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Resume the tracee and let it exit\n");
	rv = write(fds_totracee[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("fds_totracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracee[1]) == 0);

	printf("fds_fromtracee is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracee[0]) == 0);

	printf("Detect that tracee is zombie\n");
	await_zombie(tracee);

	printf("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	printf("Resume the tracer and let it detect exited tracee\n");
	rv = write(fds_totracer[1], &msg, sizeof(msg));
	ATF_REQUIRE(rv == sizeof(msg));

	printf("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	printf("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	printf("fds_fromtracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_fromtracer[0]) == 0);

	printf("fds_totracer is no longer needed - close it\n");
	ATF_REQUIRE(close(fds_totracer[1]) == 0);
}
#endif

ATF_TC(eventmask1);
ATF_TC_HEAD(eventmask1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that empty EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_event_t set_event, get_event;
	const int len = sizeof(ptrace_event_t);

	printf("Before forking process PID=%d\n", getpid());
	child = atf_utils_fork();
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	set_event.pe_set_event = 0;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	ATF_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
	ATF_REQUIRE(memcmp(&set_event, &get_event, len) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(eventmask2);
ATF_TC_HEAD(eventmask2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PTRACE_FORK in EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_event_t set_event, get_event;
	const int len = sizeof(ptrace_event_t);

	printf("Before forking process PID=%d\n", getpid());
	child = atf_utils_fork();
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	set_event.pe_set_event = PTRACE_FORK;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	ATF_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
	ATF_REQUIRE(memcmp(&set_event, &get_event, len) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(TWAIT_HAVE_PID)
#define ATF_TP_ADD_TC_HAVE_PID(a,b)	ATF_TP_ADD_TC(a,b)
#else
#define ATF_TP_ADD_TC_HAVE_PID(a,b)
#endif

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	ATF_TP_ADD_TC(tp, traceme1);
	ATF_TP_ADD_TC(tp, traceme2);
	ATF_TP_ADD_TC(tp, traceme3);
	ATF_TP_ADD_TC(tp, traceme4);

	ATF_TP_ADD_TC_HAVE_PID(tp, attach1);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach2);
	ATF_TP_ADD_TC(tp, attach3);
	ATF_TP_ADD_TC(tp, attach4);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach5);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach6);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach7);

	ATF_TP_ADD_TC(tp, eventmask1);
	ATF_TP_ADD_TC(tp, eventmask2);

	return atf_no_error();
}
