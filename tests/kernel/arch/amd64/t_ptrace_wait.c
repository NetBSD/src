/*	$NetBSD: t_ptrace_wait.c,v 1.15 2017/02/19 22:09:29 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.15 2017/02/19 22:09:29 kamil Exp $");

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
	long raw;
	struct {
		long local_dr0_breakpoint : 1;	/* 0 */
		long global_dr0_breakpoint : 1;	/* 1 */
		long local_dr1_breakpoint : 1;	/* 2 */
		long global_dr1_breakpoint : 1;	/* 3 */
		long local_dr2_breakpoint : 1;	/* 4 */
		long global_dr2_breakpoint : 1;	/* 5 */
		long local_dr3_breakpoint : 1;	/* 6 */
		long global_dr3_breakpoint : 1;	/* 7 */
		long local_exact_breakpt : 1;	/* 8 */
		long global_exact_breakpt : 1;	/* 9 */
		long reserved_10 : 1;		/* 10 */
		long rest_trans_memory : 1;	/* 11 */
		long reserved_12 : 1;		/* 12 */
		long general_detect_enable : 1;	/* 13 */
		long reserved_14 : 1;		/* 14 */
		long reserved_15 : 1;		/* 15 */
		long condition_dr0 : 2;		/* 16-17 */
		long len_dr0 : 2;		/* 18-19 */
		long condition_dr1 : 2;		/* 20-21 */
		long len_dr1 : 2;		/* 22-23 */
		long condition_dr2 : 2;		/* 24-25 */
		long len_dr2 : 2;		/* 26-27 */
		long condition_dr3 : 2;		/* 28-29 */
		long len_dr3 : 2;		/* 30-31 */
	} bits;
};

#if defined(HAVE_GPREGS)
ATF_TC(regs1);
ATF_TC_HEAD(regs1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Call PT_GETREGS and iterate over General Purpose registers");
}

ATF_TC_BODY(regs1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct reg r;

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

	printf("Call GETREGS for the child process\n");
	ATF_REQUIRE(ptrace(PT_GETREGS, child, &r, 0) != -1);

	printf("RAX=%#" PRIxREGISTER "\n", r.regs[_REG_RAX]);
	printf("RBX=%#" PRIxREGISTER "\n", r.regs[_REG_RBX]);
	printf("RCX=%#" PRIxREGISTER "\n", r.regs[_REG_RCX]);
	printf("RDX=%#" PRIxREGISTER "\n", r.regs[_REG_RDX]);

	printf("RDI=%#" PRIxREGISTER "\n", r.regs[_REG_RDI]);
	printf("RSI=%#" PRIxREGISTER "\n", r.regs[_REG_RSI]);

	printf("GS=%#" PRIxREGISTER "\n", r.regs[_REG_GS]);
	printf("FS=%#" PRIxREGISTER "\n", r.regs[_REG_FS]);
	printf("ES=%#" PRIxREGISTER "\n", r.regs[_REG_ES]);
	printf("DS=%#" PRIxREGISTER "\n", r.regs[_REG_DS]);
	printf("CS=%#" PRIxREGISTER "\n", r.regs[_REG_CS]);
	printf("SS=%#" PRIxREGISTER "\n", r.regs[_REG_SS]);

	printf("RSP=%#" PRIxREGISTER "\n", r.regs[_REG_RSP]);
	printf("RIP=%#" PRIxREGISTER "\n", r.regs[_REG_RIP]);

	printf("RFLAGS=%#" PRIxREGISTER "\n", r.regs[_REG_RFLAGS]);

	printf("R8=%#" PRIxREGISTER "\n", r.regs[_REG_R8]);
	printf("R9=%#" PRIxREGISTER "\n", r.regs[_REG_R9]);
	printf("R10=%#" PRIxREGISTER "\n", r.regs[_REG_R10]);
	printf("R11=%#" PRIxREGISTER "\n", r.regs[_REG_R11]);
	printf("R12=%#" PRIxREGISTER "\n", r.regs[_REG_R12]);
	printf("R13=%#" PRIxREGISTER "\n", r.regs[_REG_R13]);
	printf("R14=%#" PRIxREGISTER "\n", r.regs[_REG_R14]);
	printf("R15=%#" PRIxREGISTER "\n", r.regs[_REG_R15]);

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
		printf("r[%zu]=%#lx\n", i, r.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %#lx\n", r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %#lx\n", r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %#lx\n", r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %#lx\n", r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %#lx\n", r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %#lx\n", r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %#lx\n", r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %#lx\n", r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	printf("Call SETDBREGS for the child process (r1)\n");
	ATF_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	printf("Yields a processor voluntarily and gives other threads a "
	    "chance to run without waiting for an involuntary preemptive "
	    "switch\n");
	sched_yield();

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %#lx\n", r1.dr[0]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %#lx\n", r1.dr[1]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %#lx\n", r1.dr[2]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
	/* Number of available CPU Debug Registers on AMD64 */
	const size_t len = 16;
	size_t i;
	int watchme;

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %#lx\n", r1.dr[3]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
ATF_TC(dbregs_dr0_trap_variable);
ATF_TC_HEAD(dbregs_dr0_trap_variable, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP");
}

ATF_TC_BODY(dbregs_dr0_trap_variable, tc)
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
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr0 = 3;		/* 0b11 -- 4 bytes */

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)&watchme;
	printf("Set DR0 (r1.dr[0]) to new value %#lx\n", r1.dr[0]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %#lx\n", r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
ATF_TC(dbregs_dr1_trap_variable);
ATF_TC_HEAD(dbregs_dr1_trap_variable, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP");
}

ATF_TC_BODY(dbregs_dr1_trap_variable, tc)
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
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr1 = 3;		/* 0b11 -- 4 bytes */

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)&watchme;
	printf("Set DR1 (r1.dr[1]) to new value %#lx\n", r1.dr[1]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %#lx\n", r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
ATF_TC(dbregs_dr2_trap_variable);
ATF_TC_HEAD(dbregs_dr2_trap_variable, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP");
}

ATF_TC_BODY(dbregs_dr2_trap_variable, tc)
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
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr2 = 3;		/* 0b11 -- 4 bytes */

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)&watchme;
	printf("Set DR2 (r1.dr[2]) to new value %#lx\n", r1.dr[2]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %#lx\n", r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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
ATF_TC(dbregs_dr3_trap_variable);
ATF_TC_HEAD(dbregs_dr3_trap_variable, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP");
}

ATF_TC_BODY(dbregs_dr3_trap_variable, tc)
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
	volatile int watchme;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 3;	/* 0b11 -- break on data write&read */
	dr7.bits.len_dr3 = 3;		/* 0b11 -- 4 bytes */

	printf("Assert that known number of Debug Registers (%zu) is valid\n",
	    len);
	ATF_REQUIRE_EQ(__arraycount(r1.dr), len);
	ATF_REQUIRE_EQ(__arraycount(r2.dr), len);

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
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)&watchme;
	printf("Set DR3 (r1.dr[3]) to new value %#lx\n", r1.dr[3]);

	r1.dr[7] = dr7.raw;
	printf("Set DR7 (r1.dr[7]) to new value %#lx\n", r1.dr[7]);

	printf("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		printf("r1[%zu]=%#lx\n", i, r1.dr[i]);

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

	ATF_TP_ADD_TC_HAVE_GPREGS(tp, regs1);

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

	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable);
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable);

	return atf_no_error();
}
