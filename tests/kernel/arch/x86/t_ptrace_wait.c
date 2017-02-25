/*	$NetBSD: t_ptrace_wait.c,v 1.2 2017/02/25 16:45:24 christos Exp $	*/

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
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.2 2017/02/25 16:45:24 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <machine/reg.h>
#include <err.h>
#include <errno.h>
#include <lwp.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <atf-c.h>

#include "../../../h_macros.h"

#include "../../t_ptrace_wait.h"


union u {
	unsigned long raw;
	struct {
		unsigned long local_dr0_breakpoint : 1;q	/* 0 */
		unsigned long global_dr0_breakpoint : 1;	/* 1 */
		unsigned long local_dr1_breakpoint : 1;q	/* 2 */
		unsigned long global_dr1_breakpoint : 1;	/* 3 */
		unsigned long local_dr2_breakpoint : 1;q	/* 4 */
		unsigned long global_dr2_breakpoint : 1;	/* 5 */
		unsigned long local_dr3_breakpoint : 1;q	/* 6 */
		unsigned long global_dr3_breakpoint : 1;	/* 7 */
		unsigned long local_exact_breakpt : 1;		/* 8 */
		unsigned long global_exact_breakpt : 1;		/* 9 */
		unsigned long reserved_10 : 1;			/* 10 */
		unsigned long rest_trans_memory : 1;		/* 11 */
		unsigned long reserved_12 : 1;			/* 12 */
		unsigned long general_detect_enable : 1;	/* 13 */
		unsigned long reserved_14 : 1;			/* 14 */
		unsigned long reserved_15 : 1;			/* 15 */
		unsigned long condition_dr0 : 2;		/* 16-17 */
		unsigned long len_dr0 : 2;			/* 18-19 */
		unsigned long condition_dr1 : 2;		/* 20-21 */
		unsigned long len_dr1 : 2;			/* 22-23 */
		unsigned long condition_dr2 : 2;		/* 24-25 */
		unsigned long len_dr2 : 2;			/* 26-27 */
		unsigned long condition_dr3 : 2;		/* 28-29 */
		unsigned long len_dr3 : 2;			/* 30-31 */
	} bits;
};


#if defined(HAVE_DBREGS)
ATF_TC(dbregs_print);
ATF_TC_HEAD(dbregs_print, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETDBREGS with printing Debug Registers");
}

ATF_TC_BODY(dbregs_print, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r;
	size_t i;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r, 0) != -1);

	printf("State of the debug registers:\n");
	for (i = 0; i < __arraycount(r.dr); i++)
		printf("r[%zu]=%" PRIxREGISTER "\n", i, r.dr[i]);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr0);
ATF_TC_HEAD(dbregs_preserve_dr0, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr0, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr1);
ATF_TC_HEAD(dbregs_preserve_dr1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr2);
ATF_TC_HEAD(dbregs_preserve_dr2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr3);
ATF_TC_HEAD(dbregs_preserve_dr3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr0_yield);
ATF_TC_HEAD(dbregs_preserve_dr0_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr0_yield, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr1_yield);
ATF_TC_HEAD(dbregs_preserve_dr1_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr1_yield, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr2_yield);
ATF_TC_HEAD(dbregs_preserve_dr2_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr2_yield, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr3_yield);
ATF_TC_HEAD(dbregs_preserve_dr3_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr3_yield, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr0_continued);
ATF_TC_HEAD(dbregs_preserve_dr0_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr0_continued, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr1_continued);
ATF_TC_HEAD(dbregs_preserve_dr1_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr1_continued, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr2_continued);
ATF_TC_HEAD(dbregs_preserve_dr2_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr2_continued, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_preserve_dr3_continued);
ATF_TC_HEAD(dbregs_preserve_dr3_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr3_continued, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	size_t i;
	int watchme;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr0_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_writeonly_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr0 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr1_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_writeonly_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr1 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr2_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_writeonly_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr2 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr3_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_writeonly_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr3 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_writeonly_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr0 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_writeonly_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr1 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_writeonly_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr2 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_writeonly_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr3 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_writeonly_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr0 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_writeonly_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr1 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_writeonly_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr2 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_writeonly_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 1;	/* 0b01 -- break on data write only */
	dr7.bits.len_dr3 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_write_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_write_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_write_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_write_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_write_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_write_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_write_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_write_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_write_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_write_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_write_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_write_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		watchme = 1;

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_read_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_read_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_read_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_read_byte, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_read_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_read_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_read_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_read_2bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 1;		/* 0b01 -- 2 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_read_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_read_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_read_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_read_4bytes, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 3;		/* 0b11 -- 4 bytes */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("watchme=%d\n", watchme);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_trap_code);
ATF_TC_HEAD(dbregs_dr0_trap_code, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on code execution trap)");
}

ATF_TC_BODY(dbregs_dr0_trap_code, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr0 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)check_happy;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr0_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_trap_code);
ATF_TC_HEAD(dbregs_dr1_trap_code, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on code execution trap)");
}

ATF_TC_BODY(dbregs_dr1_trap_code, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr1 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)check_happy;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr1_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_trap_code);
ATF_TC_HEAD(dbregs_dr2_trap_code, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on code execution trap)");
}

ATF_TC_BODY(dbregs_dr2_trap_code, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr2 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)check_happy;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr2_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_trap_code);
ATF_TC_HEAD(dbregs_dr3_trap_code, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on code execution trap)");
}

ATF_TC_BODY(dbregs_dr3_trap_code, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 1;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr3 = 0;		/* 0b00 -- 1 byte */

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)check_happy;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	printf("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	printf("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr3_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call CONTINUE for the child process\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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
#endif

volatile lwpid_t the_lwp_id = 0;

static void __used
lwp_main_func(void *arg)
{
	the_lwp_id = _lwp_self();
	_lwp_exit();
}

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_dont_inherit_lwp);
ATF_TC_HEAD(dbregs_dr0_dont_inherit_lwp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE and Debug Register 0 from "
	    "the forker thread is not inherited");
}

ATF_TC_BODY(dbregs_dr0_dont_inherit_lwp, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		printf("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		printf("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		printf("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		printf("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)check_happy;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	ATF_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	printf("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	printf("Call GETDBREGS for the child process new lwp (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_dont_inherit_lwp);
ATF_TC_HEAD(dbregs_dr1_dont_inherit_lwp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE and Debug Register 1 from "
	    "the forker thread is not inherited");
}

ATF_TC_BODY(dbregs_dr1_dont_inherit_lwp, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		printf("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		printf("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		printf("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		printf("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)check_happy;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	ATF_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	printf("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	printf("Call GETDBREGS for the child process new lwp (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_dont_inherit_lwp);
ATF_TC_HEAD(dbregs_dr2_dont_inherit_lwp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE and Debug Register 2 from "
	    "the forker thread is not inherited");
}

ATF_TC_BODY(dbregs_dr2_dont_inherit_lwp, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		printf("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		printf("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		printf("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		printf("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)check_happy;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	ATF_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	printf("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	printf("Call GETDBREGS for the child process new lwp (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_dont_inherit_lwp);
ATF_TC_HEAD(dbregs_dr3_dont_inherit_lwp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE and Debug Register 3 from "
	    "the forker thread is not inherited");
}

ATF_TC_BODY(dbregs_dr3_dont_inherit_lwp, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		printf("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		printf("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		printf("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		printf("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)check_happy;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	ATF_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	printf("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	printf("Call GETDBREGS for the child process new lwp (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr6_dont_inherit_lwp);
ATF_TC_HEAD(dbregs_dr6_dont_inherit_lwp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE and Debug Register 6 from "
	    "the forker thread is not inherited");
}

ATF_TC_BODY(dbregs_dr6_dont_inherit_lwp, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		printf("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		printf("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		printf("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		printf("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[6] = 1; /* breakpoint condition dr0 detected */
	printf("Set DR6 (r1.dr[6]) to new value %" PRIxREGISTER "\n",
	    r1.dr[6]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	ATF_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	printf("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	printf("Call GETDBREGS for the child process new lwp (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr7_dont_inherit_lwp);
ATF_TC_HEAD(dbregs_dr7_dont_inherit_lwp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE and Debug Register 7 from "
	    "the forker thread is not inherited");
}

ATF_TC_BODY(dbregs_dr7_dont_inherit_lwp, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		printf("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		printf("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		printf("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		printf("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		printf("Before exiting of the child process\n");
		_exit(exitval);
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	ATF_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[7] = __BIT(16); /* break on data writes dr0 */
	printf("Set DR6 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	ATF_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	printf("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	printf("Call GETDBREGS for the child process new lwp (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	printf("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr0_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr0_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 0 is reset");
}

ATF_TC_BODY(dbregs_dr0_dont_inherit_execve, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)check_happy;
	printf("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	printf("Call GETDBREGS for the child process after execve(2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr1_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr1_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 1 is reset");
}

ATF_TC_BODY(dbregs_dr1_dont_inherit_execve, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)check_happy;
	printf("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	printf("Call GETDBREGS for the child process after execve(2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr2_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr2_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 2 is reset");
}

ATF_TC_BODY(dbregs_dr2_dont_inherit_execve, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)check_happy;
	printf("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	printf("Call GETDBREGS for the child process after execve(2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr3_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr3_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 3 is reset");
}

ATF_TC_BODY(dbregs_dr3_dont_inherit_execve, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)check_happy;
	printf("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	printf("Call GETDBREGS for the child process after execve(2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr6_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr6_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 6 is reset");
}

ATF_TC_BODY(dbregs_dr6_dont_inherit_execve, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[6] = 1; /* breakpoint condition dr0 detected */
	printf("Set DR6 (r1.dr[6]) to new value %" PRIxREGISTER "\n",
	    r1.dr[6]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	printf("Call GETDBREGS for the child process after execve(2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

#if defined(HAVE_DBREGS)
ATF_TC(dbregs_dr7_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr7_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 7 is reset");
}

ATF_TC_BODY(dbregs_dr7_dont_inherit_execve, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	printf("Before forking process PID=%d\n", getpid());
	ATF_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		printf("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		printf("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	printf("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[7] = __BIT(16); /* break on data writes dr0 */
	printf("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	printf("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	ATF_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	printf("Signal traced to lwpid=%d\n", info.psi_lwpid);
	printf("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	printf("Call GETDBREGS for the child process after execve(2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		printf("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	printf("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	printf("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	printf("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_print);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr0);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr1);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr2);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr3);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr0_yield);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr1_yield);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr2_yield);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr3_yield);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr0_continued);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr1_continued);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr2_continued);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr3_continued);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_writeonly_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_writeonly_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_writeonly_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_writeonly_byte);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_writeonly_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_writeonly_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_writeonly_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_writeonly_2bytes);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_writeonly_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_writeonly_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_writeonly_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_writeonly_4bytes);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_write_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_write_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_write_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_write_byte);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_write_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_write_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_write_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_write_2bytes);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_write_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_write_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_write_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_write_4bytes);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_read_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_read_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_read_byte);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_read_byte);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_read_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_read_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_read_2bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_read_2bytes);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_read_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_read_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_read_4bytes);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_read_4bytes);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_code);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_code);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_code);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_code);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_dont_inherit_lwp);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_dont_inherit_lwp);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_dont_inherit_lwp);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_dont_inherit_lwp);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr6_dont_inherit_lwp);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr7_dont_inherit_lwp);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_dont_inherit_execve);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_dont_inherit_execve);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_dont_inherit_execve);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_dont_inherit_execve);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr6_dont_inherit_execve);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr7_dont_inherit_execve);

	return atf_no_error();
}
