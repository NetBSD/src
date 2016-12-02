/*	$NetBSD: t_ptrace_wait.c,v 1.2 2016/12/02 06:49:00 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.2 2016/12/02 06:49:00 kamil Exp $");

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
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <atf-c.h>

#include "../../../h_macros.h"

#include "../../t_ptrace_wait.h"

#if defined(HAVE_DBREGS)
ATF_TC(dbregs1);
ATF_TC_HEAD(dbregs1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETDBREGS with printing Debug Registers");
}

ATF_TC_BODY(dbregs1, tc)
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

	printf("Call GETDBREGS for the child process\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r, 0) != -1);

	printf("State of the debug registers:\n");
	for (i = 0; i < __arraycount(r.dbregs); i++)
		printf("r[%zu]=%#lx\n", i, r.dbregs[i]);

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
ATF_TC(dbregs2);
ATF_TC_HEAD(dbregs2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dbregs), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dbregs), len);

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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	r1.dbregs[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dbregs[0]) to new value %#lx\n", r1.dbregs[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, len) == 0);

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
ATF_TC(dbregs3);
ATF_TC_HEAD(dbregs3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dbregs), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dbregs), len);

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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	r1.dbregs[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dbregs[1]) to new value %#lx\n", r1.dbregs[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, len) == 0);

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
ATF_TC(dbregs4);
ATF_TC_HEAD(dbregs4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dbregs), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dbregs), len);

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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	r1.dbregs[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dbregs[2]) to new value %#lx\n", r1.dbregs[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, len) == 0);

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
ATF_TC(dbregs5);
ATF_TC_HEAD(dbregs5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs5, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	struct dbreg r2;
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dbregs), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dbregs), len);

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

	printf("Call GETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	printf("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	r1.dbregs[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dbregs[3]) to new value %#lx\n", r1.dbregs[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dbregs); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dbregs[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Call GETDBREGS for the child process (r2)\n");
	ATF_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	printf("Assert that (r1) and (r2) are the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, len) == 0);

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

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs1);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs2);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs3);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs4);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs5);

	return atf_no_error();
}
