/*	$NetBSD: t_ptrace_x86_wait.h,v 1.31 2020/10/27 08:32:36 mgorny Exp $	*/

/*-
 * Copyright (c) 2016, 2017, 2018, 2019 The NetBSD Foundation, Inc.
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

#if defined(__i386__) || defined(__x86_64__)
union u {
	unsigned long raw;
	struct {
		unsigned long local_dr0_breakpoint : 1;		/* 0 */
		unsigned long global_dr0_breakpoint : 1;	/* 1 */
		unsigned long local_dr1_breakpoint : 1;		/* 2 */
		unsigned long global_dr1_breakpoint : 1;	/* 3 */
		unsigned long local_dr2_breakpoint : 1;		/* 4 */
		unsigned long global_dr2_breakpoint : 1;	/* 5 */
		unsigned long local_dr3_breakpoint : 1;		/* 6 */
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

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);	

	DPRINTF("Call GETDBREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r, 0) != -1);

	DPRINTF("State of the debug registers:\n");
	for (i = 0; i < __arraycount(r.dr); i++)
		DPRINTF("r[%zu]=%" PRIxREGISTER "\n", i, r.dr[i]);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}


enum dbreg_preserve_mode {
	dbreg_preserve_mode_none,
	dbreg_preserve_mode_yield,
	dbreg_preserve_mode_continued
};

static void
dbreg_preserve(int reg, enum dbreg_preserve_mode mode)
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

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (mode == dbreg_preserve_mode_continued) {
			DPRINTF("Before raising %s from child\n",
			       strsignal(sigval));
			FORKEE_ASSERT(raise(sigval) == 0);
		}

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[reg] = (long)(intptr_t)&watchme;
	DPRINTF("Set DR0 (r1.dr[%d]) to new value %" PRIxREGISTER "\n",
	    reg, r1.dr[reg]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	switch (mode) {
	case dbreg_preserve_mode_none:
		break;
	case dbreg_preserve_mode_yield:
		DPRINTF("Yields a processor voluntarily and gives other "
		       "threads a chance to run without waiting for an "
		       "involuntary preemptive switch\n");
		sched_yield();
		break;
	case dbreg_preserve_mode_continued:
		DPRINTF("Call CONTINUE for the child process\n");
	        SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_stopped(status, sigval);
		break;
	}

	DPRINTF("Call GETDBREGS for the child process (r2)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	DPRINTF("Assert that (r1) and (r2) are the same\n");
	SYSCALL_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) == 0);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}


ATF_TC(dbregs_preserve_dr0);
ATF_TC_HEAD(dbregs_preserve_dr0, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr0, tc)
{
	dbreg_preserve(0, dbreg_preserve_mode_none);
}

ATF_TC(dbregs_preserve_dr1);
ATF_TC_HEAD(dbregs_preserve_dr1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr1, tc)
{
	dbreg_preserve(1, dbreg_preserve_mode_none);
}

ATF_TC(dbregs_preserve_dr2);
ATF_TC_HEAD(dbregs_preserve_dr2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr2, tc)
{
	dbreg_preserve(2, dbreg_preserve_mode_none);
}

ATF_TC(dbregs_preserve_dr3);
ATF_TC_HEAD(dbregs_preserve_dr3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls");
}

ATF_TC_BODY(dbregs_preserve_dr3, tc)
{
	dbreg_preserve(3, dbreg_preserve_mode_none);
}

ATF_TC(dbregs_preserve_dr0_yield);
ATF_TC_HEAD(dbregs_preserve_dr0_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr0_yield, tc)
{
	dbreg_preserve(0, dbreg_preserve_mode_yield);
}

ATF_TC(dbregs_preserve_dr1_yield);
ATF_TC_HEAD(dbregs_preserve_dr1_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr1_yield, tc)
{
	dbreg_preserve(0, dbreg_preserve_mode_yield);
}

ATF_TC(dbregs_preserve_dr2_yield);
ATF_TC_HEAD(dbregs_preserve_dr2_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr2_yield, tc)
{
	dbreg_preserve(0, dbreg_preserve_mode_yield);
}


ATF_TC(dbregs_preserve_dr3_yield);
ATF_TC_HEAD(dbregs_preserve_dr3_yield, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls with "
	    "scheduler yield");
}

ATF_TC_BODY(dbregs_preserve_dr3_yield, tc)
{
	dbreg_preserve(3, dbreg_preserve_mode_yield);
}

ATF_TC(dbregs_preserve_dr0_continued);
ATF_TC_HEAD(dbregs_preserve_dr0_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR0 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr0_continued, tc)
{
	dbreg_preserve(0, dbreg_preserve_mode_continued);
}

ATF_TC(dbregs_preserve_dr1_continued);
ATF_TC_HEAD(dbregs_preserve_dr1_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR1 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr1_continued, tc)
{
	dbreg_preserve(1, dbreg_preserve_mode_continued);
}

ATF_TC(dbregs_preserve_dr2_continued);
ATF_TC_HEAD(dbregs_preserve_dr2_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR2 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr2_continued, tc)
{
	dbreg_preserve(2, dbreg_preserve_mode_continued);
}

ATF_TC(dbregs_preserve_dr3_continued);
ATF_TC_HEAD(dbregs_preserve_dr3_continued, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting DR3 is preserved across ptrace(2) calls and "
	    "with continued child");
}

ATF_TC_BODY(dbregs_preserve_dr3_continued, tc)
{
	dbreg_preserve(3, dbreg_preserve_mode_continued);
}


static void
dbregs_trap_variable(int reg, int cond, int len, bool write)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct dbreg r1;
	size_t i;
	volatile int watchme = 0;
	union u dr7;

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	dr7.raw = 0;
	switch (reg) {
	case 0:
		dr7.bits.global_dr0_breakpoint = 1;
		dr7.bits.condition_dr0 = cond;
		dr7.bits.len_dr0 = len;
		break;
	case 1:
		dr7.bits.global_dr1_breakpoint = 1;
		dr7.bits.condition_dr1 = cond;
		dr7.bits.len_dr1 = len;
		break;
	case 2:
		dr7.bits.global_dr2_breakpoint = 1;
		dr7.bits.condition_dr2 = cond;
		dr7.bits.len_dr2 = len;
		break;
	case 3:
		dr7.bits.global_dr3_breakpoint = 1;
		dr7.bits.condition_dr3 = cond;
		dr7.bits.len_dr3 = len;
		break;
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (write)
			watchme = 1;
		else
			printf("watchme=%d\n", watchme);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[reg] = (long)(intptr_t)&watchme;
	DPRINTF("Set DR%d (r1.dr[%d]) to new value %" PRIxREGISTER "\n",
	    reg, reg, r1.dr[reg]);

	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(dbregs_dr0_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr0_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_writeonly_byte, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(0, 1, 0, true);
}

ATF_TC(dbregs_dr1_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr1_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_writeonly_byte, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(1, 1, 0, true);
}

ATF_TC(dbregs_dr2_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr2_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_writeonly_byte, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(2, 1, 0, true);
}

ATF_TC(dbregs_dr3_trap_variable_writeonly_byte);
ATF_TC_HEAD(dbregs_dr3_trap_variable_writeonly_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data writes only and 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_writeonly_byte, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(3, 1, 0, true);
} 

ATF_TC(dbregs_dr0_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_writeonly_2bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(0, 1, 1, true);
}

ATF_TC(dbregs_dr1_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_writeonly_2bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(1, 1, 1, true);
}

ATF_TC(dbregs_dr2_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_writeonly_2bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(2, 1, 1, true);
}

ATF_TC(dbregs_dr3_trap_variable_writeonly_2bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_writeonly_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data writes only and 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_writeonly_2bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(3, 1, 1, true);
}

ATF_TC(dbregs_dr0_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_writeonly_4bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(0, 1, 3, true);
}

ATF_TC(dbregs_dr1_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_writeonly_4bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(1, 1, 3, true);
}

ATF_TC(dbregs_dr2_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_writeonly_4bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(2, 1, 3, true);
}

ATF_TC(dbregs_dr3_trap_variable_writeonly_4bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_writeonly_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data writes only and 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_writeonly_4bytes, tc)
{
	/* 0b01 -- break on data write only */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(3, 1, 3, true);
}

ATF_TC(dbregs_dr0_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_write_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(0, 3, 0, true);
}

ATF_TC(dbregs_dr1_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_write_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(1, 3, 0, true);
}

ATF_TC(dbregs_dr2_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_write_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(2, 3, 0, true);
}

ATF_TC(dbregs_dr3_trap_variable_readwrite_write_byte);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_write_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in read 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_write_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(3, 3, 0, true);
}

ATF_TC(dbregs_dr0_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_write_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(0, 3, 1, true);
}

ATF_TC(dbregs_dr1_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_write_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(1, 3, 1, true);
}

ATF_TC(dbregs_dr2_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_write_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(2, 3, 1, true);
}

ATF_TC(dbregs_dr3_trap_variable_readwrite_write_2bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_write_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in read 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_write_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(3, 3, 1, true);
}

ATF_TC(dbregs_dr0_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_write_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(0, 3, 3, true);
}

ATF_TC(dbregs_dr1_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_write_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(1, 3, 3, true);
}

ATF_TC(dbregs_dr2_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_write_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(2, 3, 3, true);
}

ATF_TC(dbregs_dr3_trap_variable_readwrite_write_4bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_write_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in read 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_write_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(3, 3, 3, true);
}

ATF_TC(dbregs_dr0_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_read_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(0, 3, 0, false);
}

ATF_TC(dbregs_dr1_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_read_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(1, 3, 0, false);
}

ATF_TC(dbregs_dr2_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_read_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(2, 3, 0, false);
}

ATF_TC(dbregs_dr3_trap_variable_readwrite_read_byte);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_read_byte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in write 1 byte mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_read_byte, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b00 -- 1 byte */

	dbregs_trap_variable(3, 3, 0, false);
}

ATF_TC(dbregs_dr0_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_read_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(0, 3, 1, false);
}

ATF_TC(dbregs_dr1_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_read_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(1, 3, 1, false);
}

ATF_TC(dbregs_dr2_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_read_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(2, 3, 1, false);
}

ATF_TC(dbregs_dr3_trap_variable_readwrite_read_2bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_read_2bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in write 2 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_read_2bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b01 -- 2 bytes */

	dbregs_trap_variable(3, 3, 1, false);
}

ATF_TC(dbregs_dr0_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr0_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR0 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr0_trap_variable_readwrite_read_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(0, 3, 3, false);
}

ATF_TC(dbregs_dr1_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr1_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR1 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr1_trap_variable_readwrite_read_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(1, 3, 3, false);
}

ATF_TC(dbregs_dr2_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr2_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR2 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr2_trap_variable_readwrite_read_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(2, 3, 3, false);
}

ATF_TC(dbregs_dr3_trap_variable_readwrite_read_4bytes);
ATF_TC_HEAD(dbregs_dr3_trap_variable_readwrite_read_4bytes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that setting trap with DR3 triggers SIGTRAP "
	    "(break on data read/write trap in write 4 bytes mode)");
}

ATF_TC_BODY(dbregs_dr3_trap_variable_readwrite_read_4bytes, tc)
{
	/* 0b11 -- break on data write&read */
	/* 0b11 -- 4 bytes */

	dbregs_trap_variable(3, 3, 3, false);
}

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

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	dr7.raw = 0;
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr0 = 0;		/* 0b00 -- 1 byte */

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[0] = (long)(intptr_t)check_happy;
	DPRINTF("Set DR0 (r1.dr[0]) to new value %" PRIxREGISTER "\n",
	    r1.dr[0]);

	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	DPRINTF("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr0_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
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

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	dr7.raw = 0;
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr1 = 0;		/* 0b00 -- 1 byte */

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[1] = (long)(intptr_t)check_happy;
	DPRINTF("Set DR1 (r1.dr[1]) to new value %" PRIxREGISTER "\n",
	    r1.dr[1]);

	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	DPRINTF("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr1_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
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

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	dr7.raw = 0;
	dr7.bits.global_dr2_breakpoint = 1;
	dr7.bits.condition_dr2 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr2 = 0;		/* 0b00 -- 1 byte */

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[2] = (long)(intptr_t)check_happy;
	DPRINTF("Set DR2 (r1.dr[2]) to new value %" PRIxREGISTER "\n",
	    r1.dr[2]);

	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	DPRINTF("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr2_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
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

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	dr7.raw = 0;
	dr7.bits.global_dr3_breakpoint = 1;
	dr7.bits.condition_dr3 = 0;	/* 0b00 -- break on code execution */
	dr7.bits.len_dr3 = 0;		/* 0b00 -- 1 byte */

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		printf("check_happy(%d)=%d\n", watchme, check_happy(watchme));

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[3] = (long)(intptr_t)check_happy;
	DPRINTF("Set DR3 (r1.dr[3]) to new value %" PRIxREGISTER "\n",
	    r1.dr[3]);

	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_DBREG);

	DPRINTF("Remove code trap from check_happy=%p\n", check_happy);
	dr7.bits.global_dr3_breakpoint = 0;
	r1.dr[7] = dr7.raw;
	DPRINTF("Set DR7 (r1.dr[7]) to new value %" PRIxREGISTER "\n",
	    r1.dr[7]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Call CONTINUE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
#endif

static void * __used
x86_main_func(void *arg)
{

	return arg;
}

static void
dbregs_dont_inherit_lwp(int reg)
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
	pthread_t t;
	lwpid_t lid;
	size_t i;
	struct dbreg r1;
	struct dbreg r2;

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT(!pthread_create(&t, NULL, x86_main_func, NULL));

		DPRINTF("Before waiting for thread to exit\n");
		FORKEE_ASSERT(!pthread_join(t, NULL));

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_CREATE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[reg] = (long)(intptr_t)check_happy;
	DPRINTF("Set DR%d (r1.dr[%d]) to new value %" PRIxREGISTER "\n",
	    reg, reg, r1.dr[0]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	DPRINTF("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	DPRINTF("Call GETDBREGS for the child process new lwp (r2)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, lid) != -1);

	DPRINTF("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		DPRINTF("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	DPRINTF("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

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
	dbregs_dont_inherit_lwp(0);
}

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
	dbregs_dont_inherit_lwp(1);
}

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
	dbregs_dont_inherit_lwp(2);
}

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
	dbregs_dont_inherit_lwp(3);
}

static void
dbregs_dont_inherit_execve(int reg)
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

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r1, 0) != -1);

	DPRINTF("State of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	r1.dr[reg] = (long)(intptr_t)check_happy;
	DPRINTF("Set DR%d (r1.dr[%d]) to new value %" PRIxREGISTER "\n",
	    reg, reg, r1.dr[reg]);

	DPRINTF("New state of the debug registers (r1):\n");
	for (i = 0; i < __arraycount(r1.dr); i++)
		DPRINTF("r1[%zu]=%" PRIxREGISTER "\n", i, r1.dr[i]);

	DPRINTF("Call SETDBREGS for the child process (r1)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	DPRINTF("Call GETDBREGS for the child process after execve(2)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r2, 0) != -1);

	DPRINTF("State of the debug registers (r2):\n");
	for (i = 0; i < __arraycount(r2.dr); i++)
		DPRINTF("r2[%zu]=%" PRIxREGISTER "\n", i, r2.dr[i]);

	DPRINTF("Assert that (r1) and (r2) are not the same\n");
	ATF_REQUIRE(memcmp(&r1, &r2, sizeof(r1)) != 0);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(dbregs_dr0_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr0_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 0 is reset");
}

ATF_TC_BODY(dbregs_dr0_dont_inherit_execve, tc)
{
	dbregs_dont_inherit_execve(0);
}

ATF_TC(dbregs_dr1_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr1_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 1 is reset");
}

ATF_TC_BODY(dbregs_dr1_dont_inherit_execve, tc)
{
	dbregs_dont_inherit_execve(1);
}

ATF_TC(dbregs_dr2_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr2_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 2 is reset");
}

ATF_TC_BODY(dbregs_dr2_dont_inherit_execve, tc)
{
	dbregs_dont_inherit_execve(2);
}

ATF_TC(dbregs_dr3_dont_inherit_execve);
ATF_TC_HEAD(dbregs_dr3_dont_inherit_execve, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that execve(2) is intercepted by tracer and Debug "
	    "Register 3 is reset");
}

ATF_TC_BODY(dbregs_dr3_dont_inherit_execve, tc)
{
	dbregs_dont_inherit_execve(3);
}

/// ----------------------------------------------------------------------------

ATF_TC(x86_cve_2018_8897);
ATF_TC_HEAD(x86_cve_2018_8897, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify mitigation for CVE-2018-8897 (POP SS debug exception)");
}

#define X86_CVE_2018_8897_PAGE 0x5000 /* page addressable by 32-bit registers */

static void
x86_cve_2018_8897_trigger(void)
{
	/*
	 * A function to trigger the POP SS (CVE-2018-8897) vulnerability
	 *
	 * ifdef __x86_64__
	 *
	 * We need to switch to 32-bit mode execution on 64-bit kernel.
	 * This is achieved with far jump instruction and GDT descriptor
	 * set to 32-bit CS selector. The 32-bit CS selector is kernel
	 * specific, in the NetBSD case registered as GUCODE32_SEL
	 * that is equal to (14 (decimal) << 3) with GDT and user
	 * privilege level (this makes it 0x73).
	 *
	 * In UNIX as(1) assembly x86_64 far jump is coded as ljmp.
	 * amd64 ljmp requires an indirect address with cs:RIP.
	 *
	 * When we are running in 32-bit mode, it's similar to the
	 * mode as if the binary had been launched in netbsd32.
	 *
	 * There are two versions of this exploit, one with RIP
	 * relative code and the other with static addresses.
	 * The first one is PIE code aware, the other no-PIE one.
	 *
	 *
	 * After switching to the 32-bit mode we can move on to the remaining
	 * part of the exploit.
	 *
	 * endif //  __x86_64__
	 *
	 * Set the stack pointer to the page we allocated earlier. Remember
	 * that we put an SS selector exactly at this address, so we can pop.
	 *
	 * movl    $0x5000,%esp
	 *
	 * Pop the SS selector off the stack. This reloads the SS selector,
	 * which is fine. Remember that we set DR0 at address 0x5000, which
	 * we are now reading. Therefore, on this instruction, the CPU will
	 * raise a #DB exception.
	 *
	 * But the "pop %ss" instruction is special: it blocks exceptions
	 * until the next instruction is executed. So the #DB that we just
	 * raised is actually blocked.
	 *
	 * pop %ss
	 *
	 * We are still here, and didn't receive the #DB. After we execute
	 * this instruction, the effect of "pop %ss" will disappear, and
	 * we will receive the #DB for real.
	 *
	 * int $4
	 *
	 * Here the bug happens. We executed "int $4", so we entered the
	 * kernel, with interrupts disabled. The #DB that was pending is
	 * received. But, it is received immediately in kernel mode, and is
	 * _NOT_ received when interrupts are enabled again.
	 *
	 * It means that, in the first instruction of the $4 handler, we
	 * think we are safe with interrupts disabled. But we aren't, and
	 * just got interrupted.													        
	 *
	 * The new interrupt handler doesn't handle this particular context:
	 * we are entered in kernel mode, the previous context was kernel
	 * mode too but it still had the user context loaded.
	 *
	 * We find ourselves not doing a 'swapgs'. At the end of the day, it
	 * means that we call trap() with a curcpu() that is fully
	 * controllable by userland. From then on, it is easy to escalate
	 * privileges.
	 *
	 * With SVS it also means we don't switch CR3, so this results in a
	 * triple fault, which this time cannot be turned to a privilege
	 * escalation.
	 */

#if __x86_64__
#if __PIE__
	void *csRIP;

	csRIP = malloc(sizeof(int) + sizeof(short));
	FORKEE_ASSERT(csRIP != NULL);

	__asm__ __volatile__(
		"	leal 24(%%eip), %%eax\n\t"
		"	movq %0, %%rdx\n\t"
		"	movl %%eax, (%%rdx)\n\t"
		"	movw $0x73, 4(%%rdx)\n\t"
		"	movq %1, %%rax\n\t"
		"	ljmp *(%%rax)\n\t"
		"	.code32\n\t"
		"	movl $0x5000, %%esp\n\t"
		"	pop %%ss\n\t"
		"	int $4\n\t"
		"	.code64\n\t"
		: "=m"(csRIP)
		: "m"(csRIP)
		: "%rax", "%rdx", "%rsp"
		);
#else /* !__PIE__ */
	__asm__ __volatile__(
		"       movq $farjmp32%=, %%rax\n\t"
		"       ljmp *(%%rax)\n\t"
		"farjmp32%=:\n\t"
		"       .long trigger32%=\n\t"
		"       .word 0x73\n\t"
		"       .code32\n\t"
		"trigger32%=:\n\t"
		"       movl $0x5000, %%esp\n\t"
		"       pop %%ss\n\t"
		"       int $4\n\t"
		"       .code64\n\t"
		:
		:
		: "%rax", "%rsp"
		);
#endif
#elif __i386__
	__asm__ __volatile__(
		"movl $0x5000, %%esp\n\t"
		"pop %%ss\n\t"
		"int $4\n\t"
		:
		:
		: "%esp"
		);
#endif
}

ATF_TC_BODY(x86_cve_2018_8897, tc)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	char *trap_page;
	struct dbreg db;
	

	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		trap_page = mmap((void *)X86_CVE_2018_8897_PAGE,
		                 sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE,
		                 MAP_FIXED|MAP_ANON|MAP_PRIVATE, -1, 0);

		/* trigger page fault */
		memset(trap_page, 0, sysconf(_SC_PAGESIZE));

		// kernel GDT
#if __x86_64__
		/* SS selector (descriptor 9 (0x4f >> 3)) */
		*trap_page = 0x4f;
#elif __i386__
		/* SS selector (descriptor 4 (0x23 >> 3)) */
		*trap_page = 0x23;
#endif

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		x86_cve_2018_8897_trigger();

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETDBREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &db, 0) != -1);

	/*
	 * Set up the dbregs. We put the 0x5000 address in DR0.
	 * It means that, the first time we touch this, the CPU will trigger a
	 * #DB exception.
	 */
	db.dr[0] = X86_CVE_2018_8897_PAGE;
	db.dr[7] = 0x30003;

	DPRINTF("Call SETDBREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &db, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	// In this test we receive SIGFPE, is this appropriate?
//	validate_status_stopped(status, SIGFPE);

	DPRINTF("Kill the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

/// ----------------------------------------------------------------------------

union x86_test_register {
	struct {
		uint64_t a, b, c, d, e, f, g, h;
	} zmm;
	struct {
		uint64_t a, b, c, d;
	} ymm;
	struct {
		uint64_t a, b;
	} xmm;
	uint64_t u64;
	uint32_t u32;
};

struct x86_test_fpu_registers {
	struct {
		uint64_t mantissa;
		uint16_t sign_exp;
	} __aligned(16) st[8];

	uint16_t cw;
	uint16_t sw;
	uint16_t tw;
	uint8_t tw_abridged;
	uint16_t opcode;
	union fp_addr ip;
	union fp_addr dp;
};

enum x86_test_regset {
	TEST_GPREGS,
	TEST_FPREGS,
	TEST_XMMREGS,
	TEST_XSTATE
};

/* Please keep them grouped by acceptable x86_test_regset. */
enum x86_test_registers {
	/* TEST_GPREGS */
	GPREGS_32,
	GPREGS_32_EBP_ESP,
	GPREGS_64,
	GPREGS_64_R8,
	/* TEST_FPREGS/TEST_XMMREGS */
	FPREGS_FPU,
	FPREGS_MM,
	FPREGS_XMM,
	/* TEST_XSTATE */
	FPREGS_YMM,
	FPREGS_ZMM
};

enum x86_test_regmode {
	TEST_GETREGS,
	TEST_SETREGS,
	TEST_COREDUMP
};

static __inline void get_gp32_regs(union x86_test_register out[])
{
#if defined(__i386__)
	const uint32_t fill = 0x0F0F0F0F;

	__asm__ __volatile__(
		/* fill registers with clobber pattern */
		"movl    %6, %%eax\n\t"
		"movl    %6, %%ebx\n\t"
		"movl    %6, %%ecx\n\t"
		"movl    %6, %%edx\n\t"
		"movl    %6, %%esi\n\t"
		"movl    %6, %%edi\n\t"
		"\n\t"
		"int3\n\t"
		: "=a"(out[0].u32), "=b"(out[1].u32), "=c"(out[2].u32),
		  "=d"(out[3].u32), "=S"(out[4].u32), "=D"(out[5].u32)
		: "g"(fill)
	);
#else
	__unreachable();
#endif
}

static __inline void set_gp32_regs(const union x86_test_register data[])
{
#if defined(__i386__)
	__asm__ __volatile__(
		"int3\n\t"
		:
		: "a"(data[0].u32), "b"(data[1].u32), "c"(data[2].u32),
		  "d"(data[3].u32), "S"(data[4].u32), "D"(data[5].u32)
		:
	);
#else
	__unreachable();
#endif
}

static __inline void get_gp32_ebp_esp_regs(union x86_test_register out[])
{
#if defined(__i386__)
	const uint32_t fill = 0x0F0F0F0F;

	__asm__ __volatile__(
		/* save original ebp & esp using our output registers */
		"movl    %%esp, %0\n\t"
		"movl    %%ebp, %1\n\t"
		/* fill them with clobber pattern */
		"movl    %2, %%esp\n\t"
		"movl    %2, %%ebp\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		/* restore ebp & esp, and save the result */
		"xchgl   %%esp, %0\n\t"
		"xchgl   %%ebp, %1\n\t"
		: "=r"(out[0].u32), "=r"(out[1].u32)
		: "g"(fill)
		:
	);
#else
	__unreachable();
#endif
}

static __inline void set_gp32_ebp_esp_regs(const union x86_test_register data[])
{
#if defined(__i386__)
	__asm__ __volatile__(
		/* ebp & ebp are a bit tricky, we must not clobber them */
		"movl    %%esp, %%eax\n\t"
		"movl    %%ebp, %%ebx\n\t"
		"movl    %0, %%esp\n\t"
		"movl    %1, %%ebp\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		"movl    %%eax, %%esp\n\t"
		"movl    %%ebx, %%ebp\n\t"
		:
		: "ri"(data[0].u32), "ri"(data[1].u32)
		: "%eax", "%ebx"
	);
#else
	__unreachable();
#endif
}

static __inline void get_gp64_regs(union x86_test_register out[])
{
#if defined(__x86_64__)
	const uint64_t fill = 0x0F0F0F0F0F0F0F0F;

	__asm__ __volatile__(
		/* save rsp & rbp */
		"movq    %%rsp, %6\n\t"
		"movq    %%rbp, %7\n\t"
		"\n\t"
		/* fill registers with clobber pattern */
		"movq    %8, %%rax\n\t"
		"movq    %8, %%rbx\n\t"
		"movq    %8, %%rcx\n\t"
		"movq    %8, %%rdx\n\t"
		"movq    %8, %%rsp\n\t"
		"movq    %8, %%rbp\n\t"
		"movq    %8, %%rsi\n\t"
		"movq    %8, %%rdi\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		/* swap saved & current rsp & rbp */
		"xchgq    %%rsp, %6\n\t"
		"xchgq    %%rbp, %7\n\t"
		: "=a"(out[0].u64), "=b"(out[1].u64), "=c"(out[2].u64),
		  "=d"(out[3].u64), "=S"(out[4].u64), "=D"(out[5].u64),
		  "=r"(out[6].u64), "=r"(out[7].u64)
		: "g"(fill)
	);
#else
	__unreachable();
#endif
}

static __inline void set_gp64_regs(const union x86_test_register data[])
{
#if defined(__x86_64__)
	__asm__ __volatile__(
		/* rbp & rbp are a bit tricky, we must not clobber them */
		"movq    %%rsp, %%r8\n\t"
		"movq    %%rbp, %%r9\n\t"
		"movq    %6, %%rsp\n\t"
		"movq    %7, %%rbp\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		"movq    %%r8, %%rsp\n\t"
		"movq    %%r9, %%rbp\n\t"
		:
		: "a"(data[0].u64), "b"(data[1].u64), "c"(data[2].u64),
		  "d"(data[3].u64), "S"(data[4].u64), "D"(data[5].u64),
		  "r"(data[6].u64), "r"(data[7].u64)
		: "%r8", "%r9"
	);
#else
	__unreachable();
#endif
}

static __inline void get_gp64_r8_regs(union x86_test_register out[])
{
#if defined(__x86_64__)
	const uint64_t fill = 0x0F0F0F0F0F0F0F0F;

	__asm__ __volatile__(
		/* fill registers with clobber pattern */
		"movq    %1, %%r8\n\t"
		"movq    %1, %%r9\n\t"
		"movq    %1, %%r10\n\t"
		"movq    %1, %%r11\n\t"
		"movq    %1, %%r12\n\t"
		"movq    %1, %%r13\n\t"
		"movq    %1, %%r14\n\t"
		"movq    %1, %%r15\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		"movq    %%r8, 0x000(%0)\n\t"
		"movq    %%r9, 0x040(%0)\n\t"
		"movq    %%r10, 0x080(%0)\n\t"
		"movq    %%r11, 0x0C0(%0)\n\t"
		"movq    %%r12, 0x100(%0)\n\t"
		"movq    %%r13, 0x140(%0)\n\t"
		"movq    %%r14, 0x180(%0)\n\t"
		"movq    %%r15, 0x1C0(%0)\n\t"
		:
		: "a"(out), "m"(fill)
		: "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
	);
#else
	__unreachable();
#endif
}

static __inline void set_gp64_r8_regs(const union x86_test_register data[])
{
#if defined(__x86_64__)
	__asm__ __volatile__(
		"movq    0x000(%0), %%r8\n\t"
		"movq    0x040(%0), %%r9\n\t"
		"movq    0x080(%0), %%r10\n\t"
		"movq    0x0C0(%0), %%r11\n\t"
		"movq    0x100(%0), %%r12\n\t"
		"movq    0x140(%0), %%r13\n\t"
		"movq    0x180(%0), %%r14\n\t"
		"movq    0x1C0(%0), %%r15\n\t"
		"int3\n\t"
		:
		: "b"(data)
		: "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
	);
#else
	__unreachable();
#endif
}

static __inline void get_fpu_regs(struct x86_test_fpu_registers *out)
{
	struct save87 fsave;
	struct fxsave fxsave;

	__CTASSERT(sizeof(out->st[0]) == 16);

	__asm__ __volatile__(
		"finit\n\t"
		"int3\n\t"
#if defined(__x86_64__)
		"fxsave64 %2\n\t"
#else
		"fxsave %2\n\t"
#endif
		"fnstenv %1\n\t"
		"fnclex\n\t"
		"fstpt 0x00(%0)\n\t"
		"fstpt 0x10(%0)\n\t"
		"fstpt 0x20(%0)\n\t"
		"fstpt 0x30(%0)\n\t"
		"fstpt 0x40(%0)\n\t"
		"fstpt 0x50(%0)\n\t"
		"fstpt 0x60(%0)\n\t"
		"fstpt 0x70(%0)\n\t"
		:
		: "a"(out->st), "m"(fsave), "m"(fxsave)
		: "st", "memory"
	);

	FORKEE_ASSERT(fsave.s87_cw == fxsave.fx_cw);
	FORKEE_ASSERT(fsave.s87_sw == fxsave.fx_sw);

	/* fsave contains full tw */
	out->cw = fsave.s87_cw;
	out->sw = fsave.s87_sw;
	out->tw = fsave.s87_tw;
	out->tw_abridged = fxsave.fx_tw;
	out->opcode = fxsave.fx_opcode;
	out->ip = fxsave.fx_ip;
	out->dp = fxsave.fx_dp;
}

/* used as single-precision float */
uint32_t x86_test_zero = 0;

static __inline void set_fpu_regs(const struct x86_test_fpu_registers *data)
{
	__CTASSERT(sizeof(data->st[0]) == 16);

	__asm__ __volatile__(
		"finit\n\t"
		"fldcw %1\n\t"
		/* load on stack in reverse order to make it easier to read */
		"fldt 0x70(%0)\n\t"
		"fldt 0x60(%0)\n\t"
		"fldt 0x50(%0)\n\t"
		"fldt 0x40(%0)\n\t"
		"fldt 0x30(%0)\n\t"
		"fldt 0x20(%0)\n\t"
		"fldt 0x10(%0)\n\t"
		"fldt 0x00(%0)\n\t"
		/* free st7 */
		"ffree %%st(7)\n\t"
		/* this should trigger a divide-by-zero */
		"fdivs (%2)\n\t"
		"int3\n\t"
		:
		: "a"(&data->st), "m"(data->cw), "b"(&x86_test_zero)
		: "st"
	);
}

__attribute__((target("mmx")))
static __inline void get_mm_regs(union x86_test_register out[])
{
	const uint64_t fill = 0x0F0F0F0F0F0F0F0F;

	__asm__ __volatile__(
		/* fill registers with clobber pattern */
		"movq    %1, %%mm0\n\t"
		"movq    %1, %%mm1\n\t"
		"movq    %1, %%mm2\n\t"
		"movq    %1, %%mm3\n\t"
		"movq    %1, %%mm4\n\t"
		"movq    %1, %%mm5\n\t"
		"movq    %1, %%mm6\n\t"
		"movq    %1, %%mm7\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		"movq    %%mm0, 0x000(%0)\n\t"
		"movq    %%mm1, 0x040(%0)\n\t"
		"movq    %%mm2, 0x080(%0)\n\t"
		"movq    %%mm3, 0x0C0(%0)\n\t"
		"movq    %%mm4, 0x100(%0)\n\t"
		"movq    %%mm5, 0x140(%0)\n\t"
		"movq    %%mm6, 0x180(%0)\n\t"
		"movq    %%mm7, 0x1C0(%0)\n\t"
		:
		: "a"(out), "m"(fill)
		: "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
	);
}

__attribute__((target("mmx")))
static __inline void set_mm_regs(const union x86_test_register data[])
{
	__asm__ __volatile__(
		"movq    0x000(%0), %%mm0\n\t"
		"movq    0x040(%0), %%mm1\n\t"
		"movq    0x080(%0), %%mm2\n\t"
		"movq    0x0C0(%0), %%mm3\n\t"
		"movq    0x100(%0), %%mm4\n\t"
		"movq    0x140(%0), %%mm5\n\t"
		"movq    0x180(%0), %%mm6\n\t"
		"movq    0x1C0(%0), %%mm7\n\t"
		"int3\n\t"
		:
		: "b"(data)
		: "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
	);
}

__attribute__((target("sse")))
static __inline void get_xmm_regs(union x86_test_register out[])
{
	union x86_test_register fill __aligned(32) = {
		.xmm={ 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F }
	};

	__asm__ __volatile__(
		/* fill registers with clobber pattern */
		"movaps  %1, %%xmm0\n\t"
		"movaps  %1, %%xmm1\n\t"
		"movaps  %1, %%xmm2\n\t"
		"movaps  %1, %%xmm3\n\t"
		"movaps  %1, %%xmm4\n\t"
		"movaps  %1, %%xmm5\n\t"
		"movaps  %1, %%xmm6\n\t"
		"movaps  %1, %%xmm7\n\t"
#if defined(__x86_64__)
		"movaps  %1, %%xmm8\n\t"
		"movaps  %1, %%xmm9\n\t"
		"movaps  %1, %%xmm10\n\t"
		"movaps  %1, %%xmm11\n\t"
		"movaps  %1, %%xmm12\n\t"
		"movaps  %1, %%xmm13\n\t"
		"movaps  %1, %%xmm14\n\t"
		"movaps  %1, %%xmm15\n\t"
#endif
		"\n\t"
		"int3\n\t"
		"\n\t"
		"movaps  %%xmm0, 0x000(%0)\n\t"
		"movaps  %%xmm1, 0x040(%0)\n\t"
		"movaps  %%xmm2, 0x080(%0)\n\t"
		"movaps  %%xmm3, 0x0C0(%0)\n\t"
		"movaps  %%xmm4, 0x100(%0)\n\t"
		"movaps  %%xmm5, 0x140(%0)\n\t"
		"movaps  %%xmm6, 0x180(%0)\n\t"
		"movaps  %%xmm7, 0x1C0(%0)\n\t"
#if defined(__x86_64__)
		"movaps  %%xmm8, 0x200(%0)\n\t"
		"movaps  %%xmm9, 0x240(%0)\n\t"
		"movaps  %%xmm10, 0x280(%0)\n\t"
		"movaps  %%xmm11, 0x2C0(%0)\n\t"
		"movaps  %%xmm12, 0x300(%0)\n\t"
		"movaps  %%xmm13, 0x340(%0)\n\t"
		"movaps  %%xmm14, 0x380(%0)\n\t"
		"movaps  %%xmm15, 0x3C0(%0)\n\t"
#endif
		:
		: "a"(out), "m"(fill)
		: "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"
#if defined(__x86_64__)
		, "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14",
		"%xmm15"
#endif
	);
}

__attribute__((target("sse")))
static __inline void set_xmm_regs(const union x86_test_register data[])
{
	__asm__ __volatile__(
		"movaps   0x000(%0), %%xmm0\n\t"
		"movaps   0x040(%0), %%xmm1\n\t"
		"movaps   0x080(%0), %%xmm2\n\t"
		"movaps   0x0C0(%0), %%xmm3\n\t"
		"movaps   0x100(%0), %%xmm4\n\t"
		"movaps   0x140(%0), %%xmm5\n\t"
		"movaps   0x180(%0), %%xmm6\n\t"
		"movaps   0x1C0(%0), %%xmm7\n\t"
#if defined(__x86_64__)
		"movaps   0x200(%0), %%xmm8\n\t"
		"movaps   0x240(%0), %%xmm9\n\t"
		"movaps   0x280(%0), %%xmm10\n\t"
		"movaps   0x2C0(%0), %%xmm11\n\t"
		"movaps   0x300(%0), %%xmm12\n\t"
		"movaps   0x340(%0), %%xmm13\n\t"
		"movaps   0x380(%0), %%xmm14\n\t"
		"movaps   0x3C0(%0), %%xmm15\n\t"
#endif
		"int3\n\t"
		:
		: "b"(data)
		: "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6",
		"%xmm7"
#if defined(__x86_64__)
		, "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13",
		"%xmm14", "%xmm15"
#endif
	);
}

__attribute__((target("avx")))
static __inline void get_ymm_regs(union x86_test_register out[])
{
	union x86_test_register fill __aligned(32) = {
		.ymm = {
			0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F,
			0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F
		}
	};

	__asm__ __volatile__(
		/* fill registers with clobber pattern */
		"vmovaps  %1, %%ymm0\n\t"
		"vmovaps  %1, %%ymm1\n\t"
		"vmovaps  %1, %%ymm2\n\t"
		"vmovaps  %1, %%ymm3\n\t"
		"vmovaps  %1, %%ymm4\n\t"
		"vmovaps  %1, %%ymm5\n\t"
		"vmovaps  %1, %%ymm6\n\t"
		"vmovaps  %1, %%ymm7\n\t"
#if defined(__x86_64__)
		"vmovaps  %1, %%ymm8\n\t"
		"vmovaps  %1, %%ymm9\n\t"
		"vmovaps  %1, %%ymm10\n\t"
		"vmovaps  %1, %%ymm11\n\t"
		"vmovaps  %1, %%ymm12\n\t"
		"vmovaps  %1, %%ymm13\n\t"
		"vmovaps  %1, %%ymm14\n\t"
		"vmovaps  %1, %%ymm15\n\t"
#endif
		"\n\t"
		"int3\n\t"
		"\n\t"
		"vmovaps %%ymm0,  0x000(%0)\n\t"
		"vmovaps %%ymm1,  0x040(%0)\n\t"
		"vmovaps %%ymm2,  0x080(%0)\n\t"
		"vmovaps %%ymm3,  0x0C0(%0)\n\t"
		"vmovaps %%ymm4,  0x100(%0)\n\t"
		"vmovaps %%ymm5,  0x140(%0)\n\t"
		"vmovaps %%ymm6,  0x180(%0)\n\t"
		"vmovaps %%ymm7,  0x1C0(%0)\n\t"
#if defined(__x86_64__)
		"vmovaps %%ymm8,  0x200(%0)\n\t"
		"vmovaps %%ymm9,  0x240(%0)\n\t"
		"vmovaps %%ymm10, 0x280(%0)\n\t"
		"vmovaps %%ymm11, 0x2C0(%0)\n\t"
		"vmovaps %%ymm12, 0x300(%0)\n\t"
		"vmovaps %%ymm13, 0x340(%0)\n\t"
		"vmovaps %%ymm14, 0x380(%0)\n\t"
		"vmovaps %%ymm15, 0x3C0(%0)\n\t"
#endif
		:
		: "a"(out), "m"(fill)
		: "%ymm0", "%ymm1", "%ymm2", "%ymm3", "%ymm4", "%ymm5", "%ymm6", "%ymm7"
#if defined(__x86_64__)
		, "%ymm8", "%ymm9", "%ymm10", "%ymm11", "%ymm12", "%ymm13", "%ymm14",
		"%ymm15"
#endif
	);
}

__attribute__((target("avx")))
static __inline void set_ymm_regs(const union x86_test_register data[])
{
	__asm__ __volatile__(
		"vmovaps  0x000(%0), %%ymm0\n\t"
		"vmovaps  0x040(%0), %%ymm1\n\t"
		"vmovaps  0x080(%0), %%ymm2\n\t"
		"vmovaps  0x0C0(%0), %%ymm3\n\t"
		"vmovaps  0x100(%0), %%ymm4\n\t"
		"vmovaps  0x140(%0), %%ymm5\n\t"
		"vmovaps  0x180(%0), %%ymm6\n\t"
		"vmovaps  0x1C0(%0), %%ymm7\n\t"
#if defined(__x86_64__)
		"vmovaps  0x200(%0), %%ymm8\n\t"
		"vmovaps  0x240(%0), %%ymm9\n\t"
		"vmovaps  0x280(%0), %%ymm10\n\t"
		"vmovaps  0x2C0(%0), %%ymm11\n\t"
		"vmovaps  0x300(%0), %%ymm12\n\t"
		"vmovaps  0x340(%0), %%ymm13\n\t"
		"vmovaps  0x380(%0), %%ymm14\n\t"
		"vmovaps  0x3C0(%0), %%ymm15\n\t"
#endif
		"int3\n\t"
		:
		: "b"(data)
		: "%ymm0", "%ymm1", "%ymm2", "%ymm3", "%ymm4", "%ymm5", "%ymm6",
		"%ymm7"
#if defined(__x86_64__)
		, "%ymm8", "%ymm9", "%ymm10", "%ymm11", "%ymm12", "%ymm13",
		"%ymm14", "%ymm15"
#endif
	);
}

__attribute__((target("avx512f")))
static __inline void get_zmm_regs(union x86_test_register out[])
{
	union x86_test_register fill __aligned(64) = {
		.zmm = {
			0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F,
			0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F,
			0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F,
			0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F
		}
	};

	__asm__ __volatile__(
		/* fill registers with clobber pattern */
		"vmovaps  %1, %%zmm0\n\t"
		"vmovaps  %1, %%zmm1\n\t"
		"vmovaps  %1, %%zmm2\n\t"
		"vmovaps  %1, %%zmm3\n\t"
		"vmovaps  %1, %%zmm4\n\t"
		"vmovaps  %1, %%zmm5\n\t"
		"vmovaps  %1, %%zmm6\n\t"
		"vmovaps  %1, %%zmm7\n\t"
#if defined(__x86_64__)
		"vmovaps  %1, %%zmm8\n\t"
		"vmovaps  %1, %%zmm9\n\t"
		"vmovaps  %1, %%zmm10\n\t"
		"vmovaps  %1, %%zmm11\n\t"
		"vmovaps  %1, %%zmm12\n\t"
		"vmovaps  %1, %%zmm13\n\t"
		"vmovaps  %1, %%zmm14\n\t"
		"vmovaps  %1, %%zmm15\n\t"
		"vmovaps  %1, %%zmm16\n\t"
		"vmovaps  %1, %%zmm17\n\t"
		"vmovaps  %1, %%zmm18\n\t"
		"vmovaps  %1, %%zmm19\n\t"
		"vmovaps  %1, %%zmm20\n\t"
		"vmovaps  %1, %%zmm21\n\t"
		"vmovaps  %1, %%zmm22\n\t"
		"vmovaps  %1, %%zmm23\n\t"
		"vmovaps  %1, %%zmm24\n\t"
		"vmovaps  %1, %%zmm25\n\t"
		"vmovaps  %1, %%zmm26\n\t"
		"vmovaps  %1, %%zmm27\n\t"
		"vmovaps  %1, %%zmm28\n\t"
		"vmovaps  %1, %%zmm29\n\t"
		"vmovaps  %1, %%zmm30\n\t"
		"vmovaps  %1, %%zmm31\n\t"
#endif
		"kmovq %1, %%k0\n\t"
		"kmovq %1, %%k1\n\t"
		"kmovq %1, %%k2\n\t"
		"kmovq %1, %%k3\n\t"
		"kmovq %1, %%k4\n\t"
		"kmovq %1, %%k5\n\t"
		"kmovq %1, %%k6\n\t"
		"kmovq %1, %%k7\n\t"
		"\n\t"
		"int3\n\t"
		"\n\t"
		"vmovaps %%zmm0,  0x000(%0)\n\t"
		"vmovaps %%zmm1,  0x040(%0)\n\t"
		"vmovaps %%zmm2,  0x080(%0)\n\t"
		"vmovaps %%zmm3,  0x0C0(%0)\n\t"
		"vmovaps %%zmm4,  0x100(%0)\n\t"
		"vmovaps %%zmm5,  0x140(%0)\n\t"
		"vmovaps %%zmm6,  0x180(%0)\n\t"
		"vmovaps %%zmm7,  0x1C0(%0)\n\t"
#if defined(__x86_64__)
		"vmovaps %%zmm8,  0x200(%0)\n\t"
		"vmovaps %%zmm9,  0x240(%0)\n\t"
		"vmovaps %%zmm10, 0x280(%0)\n\t"
		"vmovaps %%zmm11, 0x2C0(%0)\n\t"
		"vmovaps %%zmm12, 0x300(%0)\n\t"
		"vmovaps %%zmm13, 0x340(%0)\n\t"
		"vmovaps %%zmm14, 0x380(%0)\n\t"
		"vmovaps %%zmm15, 0x3C0(%0)\n\t"
		"vmovaps %%zmm16, 0x400(%0)\n\t"
		"vmovaps %%zmm17, 0x440(%0)\n\t"
		"vmovaps %%zmm18, 0x480(%0)\n\t"
		"vmovaps %%zmm19, 0x4C0(%0)\n\t"
		"vmovaps %%zmm20, 0x500(%0)\n\t"
		"vmovaps %%zmm21, 0x540(%0)\n\t"
		"vmovaps %%zmm22, 0x580(%0)\n\t"
		"vmovaps %%zmm23, 0x5C0(%0)\n\t"
		"vmovaps %%zmm24, 0x600(%0)\n\t"
		"vmovaps %%zmm25, 0x640(%0)\n\t"
		"vmovaps %%zmm26, 0x680(%0)\n\t"
		"vmovaps %%zmm27, 0x6C0(%0)\n\t"
		"vmovaps %%zmm28, 0x700(%0)\n\t"
		"vmovaps %%zmm29, 0x740(%0)\n\t"
		"vmovaps %%zmm30, 0x780(%0)\n\t"
		"vmovaps %%zmm31, 0x7C0(%0)\n\t"
#endif
		"kmovq %%k0, 0x800(%0)\n\t"
		"kmovq %%k1, 0x808(%0)\n\t"
		"kmovq %%k2, 0x810(%0)\n\t"
		"kmovq %%k3, 0x818(%0)\n\t"
		"kmovq %%k4, 0x820(%0)\n\t"
		"kmovq %%k5, 0x828(%0)\n\t"
		"kmovq %%k6, 0x830(%0)\n\t"
		"kmovq %%k7, 0x838(%0)\n\t"
		:
		: "a"(out), "m"(fill)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3", "%zmm4", "%zmm5", "%zmm6", "%zmm7"
#if defined(__x86_64__)
		, "%zmm8", "%zmm9", "%zmm10", "%zmm11", "%zmm12", "%zmm13", "%zmm14",
		  "%zmm15", "%zmm16", "%zmm17", "%zmm18", "%zmm19", "%zmm20", "%zmm21",
		  "%zmm22", "%zmm23", "%zmm24", "%zmm25", "%zmm26", "%zmm27", "%zmm28",
		  "%zmm29", "%zmm30", "%zmm31"
#endif
		, "%k0", "%k1", "%k2", "%k3", "%k4", "%k5", "%k6", "%k7"
	);
}

__attribute__((target("avx512f")))
static __inline void set_zmm_regs(const union x86_test_register data[])
{
	__asm__ __volatile__(
		"vmovaps  0x000(%0), %%zmm0\n\t"
		"vmovaps  0x040(%0), %%zmm1\n\t"
		"vmovaps  0x080(%0), %%zmm2\n\t"
		"vmovaps  0x0C0(%0), %%zmm3\n\t"
		"vmovaps  0x100(%0), %%zmm4\n\t"
		"vmovaps  0x140(%0), %%zmm5\n\t"
		"vmovaps  0x180(%0), %%zmm6\n\t"
		"vmovaps  0x1C0(%0), %%zmm7\n\t"
#if defined(__x86_64__)
		"vmovaps  0x200(%0), %%zmm8\n\t"
		"vmovaps  0x240(%0), %%zmm9\n\t"
		"vmovaps  0x280(%0), %%zmm10\n\t"
		"vmovaps  0x2C0(%0), %%zmm11\n\t"
		"vmovaps  0x300(%0), %%zmm12\n\t"
		"vmovaps  0x340(%0), %%zmm13\n\t"
		"vmovaps  0x380(%0), %%zmm14\n\t"
		"vmovaps  0x3C0(%0), %%zmm15\n\t"
		"vmovaps  0x400(%0), %%zmm16\n\t"
		"vmovaps  0x440(%0), %%zmm17\n\t"
		"vmovaps  0x480(%0), %%zmm18\n\t"
		"vmovaps  0x4C0(%0), %%zmm19\n\t"
		"vmovaps  0x500(%0), %%zmm20\n\t"
		"vmovaps  0x540(%0), %%zmm21\n\t"
		"vmovaps  0x580(%0), %%zmm22\n\t"
		"vmovaps  0x5C0(%0), %%zmm23\n\t"
		"vmovaps  0x600(%0), %%zmm24\n\t"
		"vmovaps  0x640(%0), %%zmm25\n\t"
		"vmovaps  0x680(%0), %%zmm26\n\t"
		"vmovaps  0x6C0(%0), %%zmm27\n\t"
		"vmovaps  0x700(%0), %%zmm28\n\t"
		"vmovaps  0x740(%0), %%zmm29\n\t"
		"vmovaps  0x780(%0), %%zmm30\n\t"
		"vmovaps  0x7C0(%0), %%zmm31\n\t"
#endif
		"kmovq 0x800(%0), %%k0\n\t"
		"kmovq 0x808(%0), %%k1\n\t"
		"kmovq 0x810(%0), %%k2\n\t"
		"kmovq 0x818(%0), %%k3\n\t"
		"kmovq 0x820(%0), %%k4\n\t"
		"kmovq 0x828(%0), %%k5\n\t"
		"kmovq 0x830(%0), %%k6\n\t"
		"kmovq 0x838(%0), %%k7\n\t"
		"int3\n\t"
		:
		: "b"(data)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3", "%zmm4", "%zmm5", "%zmm6", "%zmm7"
#if defined(__x86_64__)
		, "%zmm8", "%zmm9", "%zmm10", "%zmm11", "%zmm12", "%zmm13", "%zmm14",
		  "%zmm15", "%zmm16", "%zmm17", "%zmm18", "%zmm19", "%zmm20", "%zmm21",
		  "%zmm22", "%zmm23", "%zmm24", "%zmm25", "%zmm26", "%zmm27", "%zmm28",
		  "%zmm29", "%zmm30", "%zmm31"
#endif
		, "%k0", "%k1", "%k2", "%k3", "%k4",
		  "%k5", "%k6", "%k7"
	);
}

static void
x86_register_test(enum x86_test_regset regset, enum x86_test_registers regs,
    enum x86_test_regmode regmode)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;
	struct fpreg fpr;
#if defined(__i386__)
	struct xmmregs xmm;
#endif
	struct xstate xst;
	struct iovec iov;
	struct fxsave* fxs = NULL;
	uint64_t xst_flags = 0;
	char core_path[] = "/tmp/core.XXXXXX";
	int core_fd;

	const union x86_test_register expected[] __aligned(64) = {
		{{ 0x0706050403020100, 0x0F0E0D0C0B0A0908,
		   0x1716151413121110, 0x1F1E1D1C1B1A1918,
		   0x2726252423222120, 0x2F2E2D2C2B2A2928,
		   0x3736353433323130, 0x3F3E3D3C3B3A3938, }},
		{{ 0x0807060504030201, 0x100F0E0D0C0B0A09,
		   0x1817161514131211, 0x201F1E1D1C1B1A19,
		   0x2827262524232221, 0x302F2E2D2C2B2A29,
		   0x3837363534333231, 0x403F3E3D3C3B3A39, }},
		{{ 0x0908070605040302, 0x11100F0E0D0C0B0A,
		   0x1918171615141312, 0x21201F1E1D1C1B1A,
		   0x2928272625242322, 0x31302F2E2D2C2B2A,
		   0x3938373635343332, 0x41403F3E3D3C3B3A, }},
		{{ 0x0A09080706050403, 0x1211100F0E0D0C0B,
		   0x1A19181716151413, 0x2221201F1E1D1C1B,
		   0x2A29282726252423, 0x3231302F2E2D2C2B,
		   0x3A39383736353433, 0x4241403F3E3D3C3B, }},
		{{ 0x0B0A090807060504, 0x131211100F0E0D0C,
		   0x1B1A191817161514, 0x232221201F1E1D1C,
		   0x2B2A292827262524, 0x333231302F2E2D2C,
		   0x3B3A393837363534, 0x434241403F3E3D3C, }},
		{{ 0x0C0B0A0908070605, 0x14131211100F0E0D,
		   0x1C1B1A1918171615, 0x24232221201F1E1D,
		   0x2C2B2A2928272625, 0x34333231302F2E2D,
		   0x3C3B3A3938373635, 0x44434241403F3E3D, }},
		{{ 0x0D0C0B0A09080706, 0x1514131211100F0E,
		   0x1D1C1B1A19181716, 0x2524232221201F1E,
		   0x2D2C2B2A29282726, 0x3534333231302F2E,
		   0x3D3C3B3A39383736, 0x4544434241403F3E, }},
		{{ 0x0E0D0C0B0A090807, 0x161514131211100F,
		   0x1E1D1C1B1A191817, 0x262524232221201F,
		   0x2E2D2C2B2A292827, 0x363534333231302F,
		   0x3E3D3C3B3A393837, 0x464544434241403F, }},
		{{ 0x0F0E0D0C0B0A0908, 0x1716151413121110,
		   0x1F1E1D1C1B1A1918, 0x2726252423222120,
		   0x2F2E2D2C2B2A2928, 0x3736353433323130,
		   0x3F3E3D3C3B3A3938, 0x4746454443424140, }},
		{{ 0x100F0E0D0C0B0A09, 0x1817161514131211,
		   0x201F1E1D1C1B1A19, 0x2827262524232221,
		   0x302F2E2D2C2B2A29, 0x3837363534333231,
		   0x403F3E3D3C3B3A39, 0x4847464544434241, }},
		{{ 0x11100F0E0D0C0B0A, 0x1918171615141312,
		   0x21201F1E1D1C1B1A, 0x2928272625242322,
		   0x31302F2E2D2C2B2A, 0x3938373635343332,
		   0x41403F3E3D3C3B3A, 0x4948474645444342, }},
		{{ 0x1211100F0E0D0C0B, 0x1A19181716151413,
		   0x2221201F1E1D1C1B, 0x2A29282726252423,
		   0x3231302F2E2D2C2B, 0x3A39383736353433,
		   0x4241403F3E3D3C3B, 0x4A49484746454443, }},
		{{ 0x131211100F0E0D0C, 0x1B1A191817161514,
		   0x232221201F1E1D1C, 0x2B2A292827262524,
		   0x333231302F2E2D2C, 0x3B3A393837363534,
		   0x434241403F3E3D3C, 0x4B4A494847464544, }},
		{{ 0x14131211100F0E0D, 0x1C1B1A1918171615,
		   0x24232221201F1E1D, 0x2C2B2A2928272625,
		   0x34333231302F2E2D, 0x3C3B3A3938373635,
		   0x44434241403F3E3D, 0x4C4B4A4948474645, }},
		{{ 0x1514131211100F0E, 0x1D1C1B1A19181716,
		   0x2524232221201F1E, 0x2D2C2B2A29282726,
		   0x3534333231302F2E, 0x3D3C3B3A39383736,
		   0x4544434241403F3E, 0x4D4C4B4A49484746, }},
		{{ 0x161514131211100F, 0x1E1D1C1B1A191817,
		   0x262524232221201F, 0x2E2D2C2B2A292827,
		   0x363534333231302F, 0x3E3D3C3B3A393837,
		   0x464544434241403F, 0x4E4D4C4B4A494847, }},
		{{ 0x1716151413121110, 0x1F1E1D1C1B1A1918,
		   0x2726252423222120, 0x2F2E2D2C2B2A2928,
		   0x3736353433323130, 0x3F3E3D3C3B3A3938,
		   0x4746454443424140, 0x4F4E4D4C4B4A4948, }},
		{{ 0x1817161514131211, 0x201F1E1D1C1B1A19,
		   0x2827262524232221, 0x302F2E2D2C2B2A29,
		   0x3837363534333231, 0x403F3E3D3C3B3A39,
		   0x4847464544434241, 0x504F4E4D4C4B4A49, }},
		{{ 0x1918171615141312, 0x21201F1E1D1C1B1A,
		   0x2928272625242322, 0x31302F2E2D2C2B2A,
		   0x3938373635343332, 0x41403F3E3D3C3B3A,
		   0x4948474645444342, 0x51504F4E4D4C4B4A, }},
		{{ 0x1A19181716151413, 0x2221201F1E1D1C1B,
		   0x2A29282726252423, 0x3231302F2E2D2C2B,
		   0x3A39383736353433, 0x4241403F3E3D3C3B,
		   0x4A49484746454443, 0x5251504F4E4D4C4B, }},
		{{ 0x1B1A191817161514, 0x232221201F1E1D1C,
		   0x2B2A292827262524, 0x333231302F2E2D2C,
		   0x3B3A393837363534, 0x434241403F3E3D3C,
		   0x4B4A494847464544, 0x535251504F4E4D4C, }},
		{{ 0x1C1B1A1918171615, 0x24232221201F1E1D,
		   0x2C2B2A2928272625, 0x34333231302F2E2D,
		   0x3C3B3A3938373635, 0x44434241403F3E3D,
		   0x4C4B4A4948474645, 0x54535251504F4E4D, }},
		{{ 0x1D1C1B1A19181716, 0x2524232221201F1E,
		   0x2D2C2B2A29282726, 0x3534333231302F2E,
		   0x3D3C3B3A39383736, 0x4544434241403F3E,
		   0x4D4C4B4A49484746, 0x5554535251504F4E, }},
		{{ 0x1E1D1C1B1A191817, 0x262524232221201F,
		   0x2E2D2C2B2A292827, 0x363534333231302F,
		   0x3E3D3C3B3A393837, 0x464544434241403F,
		   0x4E4D4C4B4A494847, 0x565554535251504F, }},
		{{ 0x1F1E1D1C1B1A1918, 0x2726252423222120,
		   0x2F2E2D2C2B2A2928, 0x3736353433323130,
		   0x3F3E3D3C3B3A3938, 0x4746454443424140,
		   0x4F4E4D4C4B4A4948, 0x5756555453525150, }},
		{{ 0x201F1E1D1C1B1A19, 0x2827262524232221,
		   0x302F2E2D2C2B2A29, 0x3837363534333231,
		   0x403F3E3D3C3B3A39, 0x4847464544434241,
		   0x504F4E4D4C4B4A49, 0x5857565554535251, }},
		{{ 0x21201F1E1D1C1B1A, 0x2928272625242322,
		   0x31302F2E2D2C2B2A, 0x3938373635343332,
		   0x41403F3E3D3C3B3A, 0x4948474645444342,
		   0x51504F4E4D4C4B4A, 0x5958575655545352, }},
		{{ 0x2221201F1E1D1C1B, 0x2A29282726252423,
		   0x3231302F2E2D2C2B, 0x3A39383736353433,
		   0x4241403F3E3D3C3B, 0x4A49484746454443,
		   0x5251504F4E4D4C4B, 0x5A59585756555453, }},
		{{ 0x232221201F1E1D1C, 0x2B2A292827262524,
		   0x333231302F2E2D2C, 0x3B3A393837363534,
		   0x434241403F3E3D3C, 0x4B4A494847464544,
		   0x535251504F4E4D4C, 0x5B5A595857565554, }},
		{{ 0x24232221201F1E1D, 0x2C2B2A2928272625,
		   0x34333231302F2E2D, 0x3C3B3A3938373635,
		   0x44434241403F3E3D, 0x4C4B4A4948474645,
		   0x54535251504F4E4D, 0x5C5B5A5958575655, }},
		{{ 0x2524232221201F1E, 0x2D2C2B2A29282726,
		   0x3534333231302F2E, 0x3D3C3B3A39383736,
		   0x4544434241403F3E, 0x4D4C4B4A49484746,
		   0x5554535251504F4E, 0x5D5C5B5A59585756, }},
		{{ 0x262524232221201F, 0x2E2D2C2B2A292827,
		   0x363534333231302F, 0x3E3D3C3B3A393837,
		   0x464544434241403F, 0x4E4D4C4B4A494847,
		   0x565554535251504F, 0x5E5D5C5B5A595857, }},
		/* k0..k7 */
		{{ 0x2726252423222120, 0x2F2E2D2C2B2A2928,
		   0x3736353433323130, 0x3F3E3D3C3B3A3938,
		   0x4746454443424140, 0x4F4E4D4C4B4A4948,
		   0x5756555453525150, 0x5F5E5D5C5B5A5958, }},
	};

	const struct x86_test_fpu_registers expected_fpu = {
		.st = {
			{0x8000000000000000, 0x4000}, /* +2.0 */
			{0x3f00000000000000, 0x0000}, /* 1.654785e-4932 */
			{0x0000000000000000, 0x0000}, /* +0 */
			{0x0000000000000000, 0x8000}, /* -0 */
			{0x8000000000000000, 0x7fff}, /* +inf */
			{0x8000000000000000, 0xffff}, /* -inf */
			{0xc000000000000000, 0xffff}, /* nan */
			/* st(7) will be freed to test tag word better */
			{0x0000000000000000, 0x0000}, /* +0 */
		},
		/* 0000 0011 0111 1011
		 *             PU OZDI -- unmask divide-by-zero exc.
		 *           RR --------- reserved
		 *        PC ------------ 64-bit precision
		 *      RC -------------- round to nearest
		 *    I ----------------- allow interrupts (unused)
		 */
		.cw = 0x037b,
		/* 1000 0000 1000 0100
		 *            SPU OZDI -- divide-by-zero exception
		 *           I ---------- interrupt (exception handling)
		 *  C    CCC ------------ condition codes
		 *   TO P --------------- top register is 0
		 * B -------------------- FPU is busy
		 */
		.sw = 0x8084,
		/* 1110 1010 0101 1000
		 * R7R6 R5R4 R3R2 R1R0
		 *                  nz -- non-zero (+2.0)
		 *                sp ---- special (denormal)
		 *           zrzr ------- zeroes
		 *   sp spsp ------------ specials (NaN + infinities)
		 * em ------------------- empty register
		 */
		.tw = 0xea58,
		/* 0111 1111 -- registers 0 to 6 are used */
		.tw_abridged = 0x7f,
		/* FDIV */
		.opcode = 0x0033,
		/* random bits for IP/DP write test
		 * keep it below 48 bits since it can be truncated
		 */
		.ip = {.fa_64 = 0x00000a9876543210},
		.dp = {.fa_64 = 0x0000056789abcdef},
	};

	bool need_32 = false, need_64 = false, need_cpuid = false;

	switch (regs) {
	case GPREGS_32:
	case GPREGS_32_EBP_ESP:
		need_32 = true;
		break;
	case GPREGS_64:
	case GPREGS_64_R8:
		need_64 = true;
		break;
	case FPREGS_FPU:
		break;
	case FPREGS_MM:
	case FPREGS_XMM:
	case FPREGS_YMM:
	case FPREGS_ZMM:
		need_cpuid = true;
		break;
	}

	if (need_32) {
#if defined(__x86_64__)
		atf_tc_skip("Test requires 32-bit mode");
#endif
	}
	if (need_64) {
#if defined(__i386__)
		atf_tc_skip("Test requires 64-bit mode");
#endif
	}

	if (need_cpuid) {
		/* verify whether needed instruction sets are supported here */
		unsigned int eax, ebx, ecx, edx;
		unsigned int eax7, ebx7, ecx7, edx7;

		DPRINTF("Before invoking cpuid\n");
		if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
			atf_tc_skip("CPUID is not supported by the CPU");

		DPRINTF("cpuid[eax=1]: ECX = %08x, EDX = %08xd\n", ecx, edx);

		switch (regs) {
		case FPREGS_ZMM:
			/* ZMM is in EAX=7, ECX=0 */
			if (!__get_cpuid_count(7, 0, &eax7, &ebx7, &ecx7, &edx7))
				atf_tc_skip(
				    "AVX512F is not supported by the CPU");
			DPRINTF("cpuid[eax=7,ecx=0]: EBX = %08x\n", ebx7);
			if (!(ebx7 & bit_AVX512F))
				atf_tc_skip(
				    "AVX512F is not supported by the CPU");
			/*FALLTHROUGH*/
		case FPREGS_YMM:
			if (!(ecx & bit_AVX))
				atf_tc_skip("AVX is not supported by the CPU");
			/*FALLTHROUGH*/
		case FPREGS_XMM:
			if (!(edx & bit_SSE))
				atf_tc_skip("SSE is not supported by the CPU");
			break;
		case FPREGS_MM:
			if (!(edx & bit_MMX))
				atf_tc_skip("MMX is not supported by the CPU");
			break;
		case GPREGS_32:
		case GPREGS_32_EBP_ESP:
		case GPREGS_64:
		case GPREGS_64_R8:
		case FPREGS_FPU:
			__unreachable();
		}
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		union x86_test_register vals[__arraycount(expected)] __aligned(64);
		struct x86_test_fpu_registers vals_fpu;

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");
		switch (regmode) {
		case TEST_GETREGS:
		case TEST_COREDUMP:
			switch (regs) {
			case GPREGS_32:
				set_gp32_regs(expected);
				break;
			case GPREGS_32_EBP_ESP:
				set_gp32_ebp_esp_regs(expected);
				break;
			case GPREGS_64:
				set_gp64_regs(expected);
				break;
			case GPREGS_64_R8:
				set_gp64_r8_regs(expected);
				break;
			case FPREGS_FPU:
				set_fpu_regs(&expected_fpu);
				break;
			case FPREGS_MM:
				set_mm_regs(expected);
				break;
			case FPREGS_XMM:
				set_xmm_regs(expected);
				break;
			case FPREGS_YMM:
				set_ymm_regs(expected);
				break;
			case FPREGS_ZMM:
				set_zmm_regs(expected);
				break;
			}
			break;
		case TEST_SETREGS:
			switch (regs) {
			case GPREGS_32:
				get_gp32_regs(vals);
				break;
			case GPREGS_32_EBP_ESP:
				get_gp32_ebp_esp_regs(vals);
				break;
			case GPREGS_64:
				get_gp64_regs(vals);
				break;
			case GPREGS_64_R8:
				get_gp64_r8_regs(vals);
				break;
			case FPREGS_FPU:
				get_fpu_regs(&vals_fpu);
				break;
			case FPREGS_MM:
				get_mm_regs(vals);
				break;
			case FPREGS_XMM:
				get_xmm_regs(vals);
				break;
			case FPREGS_YMM:
				get_ymm_regs(vals);
				break;
			case FPREGS_ZMM:
				get_zmm_regs(vals);
				break;
			}

			DPRINTF("Before comparing results\n");
			switch (regs) {
			case GPREGS_32:
				FORKEE_ASSERT(!memcmp(&vals[5].u32,
				    &expected[5].u32, sizeof(vals->u32)));
				FORKEE_ASSERT(!memcmp(&vals[4].u32,
				    &expected[4].u32, sizeof(vals->u32)));
				FORKEE_ASSERT(!memcmp(&vals[3].u32,
				    &expected[3].u32, sizeof(vals->u32)));
				FORKEE_ASSERT(!memcmp(&vals[2].u32,
				    &expected[2].u32, sizeof(vals->u32)));
				/*FALLTHROUGH*/
			case GPREGS_32_EBP_ESP:
				FORKEE_ASSERT(!memcmp(&vals[1].u32,
				    &expected[1].u32, sizeof(vals->u32)));
				FORKEE_ASSERT(!memcmp(&vals[0].u32,
				    &expected[0].u32, sizeof(vals->u32)));
				break;
			case GPREGS_64:
			case GPREGS_64_R8:
			case FPREGS_MM:
				FORKEE_ASSERT(!memcmp(&vals[0].u64,
				    &expected[0].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[1].u64,
				    &expected[1].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[2].u64,
				    &expected[2].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[3].u64,
				    &expected[3].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[4].u64,
				    &expected[4].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[5].u64,
				    &expected[5].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[6].u64,
				    &expected[6].u64, sizeof(vals->u64)));
				FORKEE_ASSERT(!memcmp(&vals[7].u64,
				    &expected[7].u64, sizeof(vals->u64)));
				break;
			case FPREGS_FPU:
				FORKEE_ASSERT(vals_fpu.cw == expected_fpu.cw);
				FORKEE_ASSERT(vals_fpu.sw == expected_fpu.sw);
				FORKEE_ASSERT(vals_fpu.tw == expected_fpu.tw);
				FORKEE_ASSERT(vals_fpu.tw_abridged
				    == expected_fpu.tw_abridged);
				FORKEE_ASSERT(vals_fpu.ip.fa_64
				    == expected_fpu.ip.fa_64);
				FORKEE_ASSERT(vals_fpu.dp.fa_64
				    == expected_fpu.dp.fa_64);

				FORKEE_ASSERT(vals_fpu.st[0].sign_exp
				    == expected_fpu.st[0].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[0].mantissa
				    == expected_fpu.st[0].mantissa);
				FORKEE_ASSERT(vals_fpu.st[1].sign_exp
				    == expected_fpu.st[1].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[1].mantissa
				    == expected_fpu.st[1].mantissa);
				FORKEE_ASSERT(vals_fpu.st[2].sign_exp
				    == expected_fpu.st[2].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[2].mantissa
				    == expected_fpu.st[2].mantissa);
				FORKEE_ASSERT(vals_fpu.st[3].sign_exp
				    == expected_fpu.st[3].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[3].mantissa
				    == expected_fpu.st[3].mantissa);
				FORKEE_ASSERT(vals_fpu.st[4].sign_exp
				    == expected_fpu.st[4].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[4].mantissa
				    == expected_fpu.st[4].mantissa);
				FORKEE_ASSERT(vals_fpu.st[5].sign_exp
				    == expected_fpu.st[5].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[5].mantissa
				    == expected_fpu.st[5].mantissa);
				FORKEE_ASSERT(vals_fpu.st[6].sign_exp
				    == expected_fpu.st[6].sign_exp);
				FORKEE_ASSERT(vals_fpu.st[6].mantissa
				    == expected_fpu.st[6].mantissa);
				/* st(7) is left empty == undefined */
				break;
			case FPREGS_XMM:
				FORKEE_ASSERT(!memcmp(&vals[0].xmm,
				    &expected[0].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[1].xmm,
				    &expected[1].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[2].xmm,
				    &expected[2].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[3].xmm,
				    &expected[3].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[4].xmm,
				    &expected[4].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[5].xmm,
				    &expected[5].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[6].xmm,
				    &expected[6].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[7].xmm,
				    &expected[7].xmm, sizeof(vals->xmm)));
#if defined(__x86_64__)
				FORKEE_ASSERT(!memcmp(&vals[8].xmm,
				    &expected[8].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[9].xmm,
				    &expected[9].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[10].xmm,
				    &expected[10].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[11].xmm,
				    &expected[11].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[12].xmm,
				    &expected[12].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[13].xmm,
				    &expected[13].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[14].xmm,
				    &expected[14].xmm, sizeof(vals->xmm)));
				FORKEE_ASSERT(!memcmp(&vals[15].xmm,
				    &expected[15].xmm, sizeof(vals->xmm)));
#endif
				break;
			case FPREGS_YMM:
				FORKEE_ASSERT(!memcmp(&vals[0].ymm,
				    &expected[0].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[1].ymm,
				    &expected[1].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[2].ymm,
				    &expected[2].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[3].ymm,
				    &expected[3].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[4].ymm,
				    &expected[4].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[5].ymm,
				    &expected[5].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[6].ymm,
				    &expected[6].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[7].ymm,
				    &expected[7].ymm, sizeof(vals->ymm)));
#if defined(__x86_64__)
				FORKEE_ASSERT(!memcmp(&vals[8].ymm,
				    &expected[8].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[9].ymm,
				    &expected[9].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[10].ymm,
				    &expected[10].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[11].ymm,
				    &expected[11].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[12].ymm,
				    &expected[12].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[13].ymm,
				    &expected[13].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[14].ymm,
				    &expected[14].ymm, sizeof(vals->ymm)));
				FORKEE_ASSERT(!memcmp(&vals[15].ymm,
				    &expected[15].ymm, sizeof(vals->ymm)));
#endif
				break;
			case FPREGS_ZMM:
				FORKEE_ASSERT(!memcmp(&vals[0].zmm,
				    &expected[0].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[1].zmm,
				    &expected[1].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[2].zmm,
				    &expected[2].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[3].zmm,
				    &expected[3].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[4].zmm,
				    &expected[4].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[5].zmm,
				    &expected[5].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[6].zmm,
				    &expected[6].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[7].zmm,
				    &expected[7].zmm, sizeof(vals->zmm)));
#if defined(__x86_64__)
				FORKEE_ASSERT(!memcmp(&vals[8].zmm,
				    &expected[8].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[9].zmm,
				    &expected[9].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[10].zmm,
				    &expected[10].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[11].zmm,
				    &expected[11].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[12].zmm,
				    &expected[12].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[13].zmm,
				    &expected[13].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[14].zmm,
				    &expected[14].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[15].zmm,
				    &expected[15].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[16].zmm,
				    &expected[16].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[17].zmm,
				    &expected[17].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[18].zmm,
				    &expected[18].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[19].zmm,
				    &expected[19].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[20].zmm,
				    &expected[20].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[21].zmm,
				    &expected[21].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[22].zmm,
				    &expected[22].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[23].zmm,
				    &expected[23].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[24].zmm,
				    &expected[24].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[25].zmm,
				    &expected[25].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[26].zmm,
				    &expected[26].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[27].zmm,
				    &expected[27].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[28].zmm,
				    &expected[28].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[29].zmm,
				    &expected[29].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[30].zmm,
				    &expected[30].zmm, sizeof(vals->zmm)));
				FORKEE_ASSERT(!memcmp(&vals[31].zmm,
				    &expected[31].zmm, sizeof(vals->zmm)));
#endif
				/* k0..k7 */
				FORKEE_ASSERT(vals[32].zmm.a == expected[32].zmm.a);
				FORKEE_ASSERT(vals[32].zmm.b == expected[32].zmm.b);
				FORKEE_ASSERT(vals[32].zmm.c == expected[32].zmm.c);
				FORKEE_ASSERT(vals[32].zmm.d == expected[32].zmm.d);
				FORKEE_ASSERT(vals[32].zmm.e == expected[32].zmm.e);
				FORKEE_ASSERT(vals[32].zmm.f == expected[32].zmm.f);
				FORKEE_ASSERT(vals[32].zmm.g == expected[32].zmm.g);
				FORKEE_ASSERT(vals[32].zmm.h == expected[32].zmm.h);
				break;
			}
			break;
		}

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	if (regset == TEST_XSTATE) {
		switch (regs) {
		case FPREGS_FPU:
		case FPREGS_MM:
			xst_flags |= XCR0_X87;
			break;
		case FPREGS_ZMM:
			xst_flags |= XCR0_Opmask | XCR0_ZMM_Hi256;
#if defined(__x86_64__)
			xst_flags |= XCR0_Hi16_ZMM;
#endif
			/*FALLTHROUGH*/
		case FPREGS_YMM:
			xst_flags |= XCR0_YMM_Hi128;
			/*FALLTHROUGH*/
		case FPREGS_XMM:
			xst_flags |= XCR0_SSE;
			break;
		case GPREGS_32:
		case GPREGS_32_EBP_ESP:
		case GPREGS_64:
		case GPREGS_64_R8:
			__unreachable();
			break;
		}
	}

	switch (regmode) {
	case TEST_GETREGS:
	case TEST_SETREGS:
		if (regset == TEST_GPREGS || regs == FPREGS_FPU) {
			DPRINTF("Call GETREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0)
			    != -1);
		}

		switch (regset) {
		case TEST_GPREGS:
			/* already handled above */
			break;
		case TEST_XMMREGS:
#if defined(__i386__)
			ATF_REQUIRE(regs >= FPREGS_FPU && regs < FPREGS_YMM);
			DPRINTF("Call GETXMMREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_GETXMMREGS, child, &xmm, 0)
			    != -1);
			fxs = &xmm.fxstate;
			break;
#else
			/*FALLTHROUGH*/
#endif
		case TEST_FPREGS:
#if defined(__x86_64__)
			ATF_REQUIRE(regs >= FPREGS_FPU && regs < FPREGS_YMM);
			fxs = &fpr.fxstate;
#else
			ATF_REQUIRE(regs >= FPREGS_FPU && regs < FPREGS_XMM);
#endif
			DPRINTF("Call GETFPREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0)
			    != -1);
			break;
		case TEST_XSTATE:
			ATF_REQUIRE(regs >= FPREGS_FPU);
			iov.iov_base = &xst;
			iov.iov_len = sizeof(xst);

			DPRINTF("Call GETXSTATE for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_GETXSTATE, child, &iov, 0)
			    != -1);

			ATF_REQUIRE((xst.xs_rfbm & xst_flags) == xst_flags);
			switch (regmode) {
			case TEST_SETREGS:
				xst.xs_rfbm = xst_flags;
				xst.xs_xstate_bv = xst_flags;
				break;
			case TEST_GETREGS:
				ATF_REQUIRE((xst.xs_xstate_bv & xst_flags)
				    == xst_flags);
				break;
			case TEST_COREDUMP:
				__unreachable();
				break;
			}

			fxs = &xst.xs_fxsave;
			break;
		}
		break;
	case TEST_COREDUMP:
		SYSCALL_REQUIRE((core_fd = mkstemp(core_path)) != -1);
		close(core_fd);

		DPRINTF("Call DUMPCORE for the child process\n");
		SYSCALL_REQUIRE(ptrace(PT_DUMPCORE, child, core_path,
		    strlen(core_path)) != -1);

		if (regset == TEST_GPREGS || regs == FPREGS_FPU) {
			DPRINTF("Parse core file for PT_GETREGS\n");
			ATF_REQUIRE_EQ(core_find_note(core_path,
			    "NetBSD-CORE@*", PT_GETREGS, &gpr, sizeof(gpr)),
			    sizeof(gpr));
		}

		switch (regset) {
		case TEST_GPREGS:
			/* handled above */
			break;
		case TEST_XMMREGS:
#if defined(__i386__)
			ATF_REQUIRE(regs >= FPREGS_FPU && regs < FPREGS_YMM);
			unlink(core_path);
			atf_tc_skip("XMMREGS not supported in core dumps");
			break;
#else
			/*FALLTHROUGH*/
#endif
		case TEST_FPREGS:
#if defined(__x86_64__)
			ATF_REQUIRE(regs >= FPREGS_FPU && regs < FPREGS_YMM);
			fxs = &fpr.fxstate;
#else
			ATF_REQUIRE(regs >= FPREGS_FPU && regs < FPREGS_XMM);
#endif
			DPRINTF("Parse core file for PT_GETFPREGS\n");
			ATF_REQUIRE_EQ(core_find_note(core_path,
			    "NetBSD-CORE@*", PT_GETFPREGS, &fpr, sizeof(fpr)),
			    sizeof(fpr));
			break;
		case TEST_XSTATE:
			ATF_REQUIRE(regs >= FPREGS_FPU);
			DPRINTF("Parse core file for PT_GETXSTATE\n");
			ATF_REQUIRE_EQ(core_find_note(core_path,
			    "NetBSD-CORE@*", PT_GETXSTATE, &xst, sizeof(xst)),
			    sizeof(xst));
			ATF_REQUIRE((xst.xs_xstate_bv & xst_flags)
			    == xst_flags);
			fxs = &xst.xs_fxsave;
			break;
		}
		unlink(core_path);
	}

#if defined(__x86_64__)
#define ST_EXP(n) fxs->fx_87_ac[n].r.f87_exp_sign
#define ST_MAN(n) fxs->fx_87_ac[n].r.f87_mantissa
#else
#define ST_EXP(n) *(							\
    regset == TEST_FPREGS						\
    ? &fpr.fstate.s87_ac[n].f87_exp_sign				\
    : &fxs->fx_87_ac[n].r.f87_exp_sign					\
    )
#define ST_MAN(n) *(							\
    regset == TEST_FPREGS						\
    ? &fpr.fstate.s87_ac[n].f87_mantissa				\
    : &fxs->fx_87_ac[n].r.f87_mantissa					\
    )
#endif

	switch (regmode) {
	case TEST_GETREGS:
	case TEST_COREDUMP:
		switch (regs) {
		case GPREGS_32:
#if defined(__i386__)
			ATF_CHECK_EQ((uint32_t)gpr.r_eax, expected[0].u32);
			ATF_CHECK_EQ((uint32_t)gpr.r_ebx, expected[1].u32);
			ATF_CHECK_EQ((uint32_t)gpr.r_ecx, expected[2].u32);
			ATF_CHECK_EQ((uint32_t)gpr.r_edx, expected[3].u32);
			ATF_CHECK_EQ((uint32_t)gpr.r_esi, expected[4].u32);
			ATF_CHECK_EQ((uint32_t)gpr.r_edi, expected[5].u32);
#endif
			break;
		case GPREGS_32_EBP_ESP:
#if defined(__i386__)
			ATF_CHECK_EQ((uint32_t)gpr.r_esp, expected[0].u32);
			ATF_CHECK_EQ((uint32_t)gpr.r_ebp, expected[1].u32);
#endif
			break;
		case GPREGS_64:
#if defined(__x86_64__)
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RAX],
			    expected[0].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RBX],
			    expected[1].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RCX],
			    expected[2].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RDX],
			    expected[3].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RSI],
			    expected[4].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RDI],
			    expected[5].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RSP],
			    expected[6].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RBP],
			    expected[7].u64);
#endif
			break;
		case GPREGS_64_R8:
#if defined(__x86_64__)
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R8],
			    expected[0].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R9],
			    expected[1].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R10],
			    expected[2].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R11],
			    expected[3].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R12],
			    expected[4].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R13],
			    expected[5].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R14],
			    expected[6].u64);
			ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R15],
			    expected[7].u64);
#endif
			break;
		case FPREGS_FPU:
#if defined(__i386__)
			if (regset == TEST_FPREGS) {
				/* GETFPREGS on i386 */
				ATF_CHECK_EQ(fpr.fstate.s87_cw,
				    expected_fpu.cw);
				ATF_CHECK_EQ(fpr.fstate.s87_sw,
				    expected_fpu.sw);
				ATF_CHECK_EQ(fpr.fstate.s87_tw,
				    expected_fpu.tw);
				ATF_CHECK_EQ(fpr.fstate.s87_opcode,
				    expected_fpu.opcode);
				ATF_CHECK_EQ(fpr.fstate.s87_ip.fa_32.fa_off,
				    (uint32_t)gpr.r_eip - 3);
				ATF_CHECK_EQ(fpr.fstate.s87_dp.fa_32.fa_off,
				    (uint32_t)&x86_test_zero);
				/* note: fa_seg is missing on newer CPUs */
			} else
#endif
			{
				/* amd64 or GETXSTATE on i386 */
				ATF_CHECK_EQ(fxs->fx_cw, expected_fpu.cw);
				ATF_CHECK_EQ(fxs->fx_sw, expected_fpu.sw);
				ATF_CHECK_EQ(fxs->fx_tw,
				    expected_fpu.tw_abridged);
				ATF_CHECK_EQ(fxs->fx_opcode,
				    expected_fpu.opcode);
#if defined(__x86_64__)
				ATF_CHECK_EQ(fxs->fx_ip.fa_64,
				    ((uint64_t)gpr.regs[_REG_RIP]) - 3);
				ATF_CHECK_EQ(fxs->fx_dp.fa_64,
				    (uint64_t)&x86_test_zero);
#else
				ATF_CHECK_EQ(fxs->fx_ip.fa_32.fa_off,
				    (uint32_t)gpr.r_eip - 3);
				ATF_CHECK_EQ(fxs->fx_dp.fa_32.fa_off,
				    (uint32_t)&x86_test_zero);
				/* note: fa_seg is missing on newer CPUs */
#endif
			}

			ATF_CHECK_EQ(ST_EXP(0), expected_fpu.st[0].sign_exp);
			ATF_CHECK_EQ(ST_MAN(0), expected_fpu.st[0].mantissa);
			ATF_CHECK_EQ(ST_EXP(1), expected_fpu.st[1].sign_exp);
			ATF_CHECK_EQ(ST_MAN(1), expected_fpu.st[1].mantissa);
			ATF_CHECK_EQ(ST_EXP(2), expected_fpu.st[2].sign_exp);
			ATF_CHECK_EQ(ST_MAN(2), expected_fpu.st[2].mantissa);
			ATF_CHECK_EQ(ST_EXP(3), expected_fpu.st[3].sign_exp);
			ATF_CHECK_EQ(ST_MAN(3), expected_fpu.st[3].mantissa);
			ATF_CHECK_EQ(ST_EXP(4), expected_fpu.st[4].sign_exp);
			ATF_CHECK_EQ(ST_MAN(4), expected_fpu.st[4].mantissa);
			ATF_CHECK_EQ(ST_EXP(5), expected_fpu.st[5].sign_exp);
			ATF_CHECK_EQ(ST_MAN(5), expected_fpu.st[5].mantissa);
			ATF_CHECK_EQ(ST_EXP(6), expected_fpu.st[6].sign_exp);
			ATF_CHECK_EQ(ST_MAN(6), expected_fpu.st[6].mantissa);
			ATF_CHECK_EQ(ST_EXP(7), expected_fpu.st[7].sign_exp);
			ATF_CHECK_EQ(ST_MAN(7), expected_fpu.st[7].mantissa);
			break;
		case FPREGS_MM:
			ATF_CHECK_EQ(ST_MAN(0), expected[0].u64);
			ATF_CHECK_EQ(ST_MAN(1), expected[1].u64);
			ATF_CHECK_EQ(ST_MAN(2), expected[2].u64);
			ATF_CHECK_EQ(ST_MAN(3), expected[3].u64);
			ATF_CHECK_EQ(ST_MAN(4), expected[4].u64);
			ATF_CHECK_EQ(ST_MAN(5), expected[5].u64);
			ATF_CHECK_EQ(ST_MAN(6), expected[6].u64);
			ATF_CHECK_EQ(ST_MAN(7), expected[7].u64);
			break;
		case FPREGS_ZMM:
			/* zmm0..zmm15 are split between xmm, ymm_hi128 and zmm_hi256 */
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[0],
			    &expected[0].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[1],
			    &expected[1].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[2],
			    &expected[2].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[3],
			    &expected[3].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[4],
			    &expected[4].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[5],
			    &expected[5].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[6],
			    &expected[6].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[7],
			    &expected[7].zmm.e, sizeof(expected->zmm)/2));
#if defined(__x86_64__)
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[8],
			    &expected[8].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[9],
			    &expected[9].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[10],
			    &expected[10].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[11],
			    &expected[11].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[12],
			    &expected[12].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[13],
			    &expected[13].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[14],
			    &expected[14].zmm.e, sizeof(expected->zmm)/2));
			ATF_CHECK(!memcmp(&xst.xs_zmm_hi256.xs_zmm[15],
			    &expected[15].zmm.e, sizeof(expected->zmm)/2));
			/* zmm16..zmm31 are stored as a whole */
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[0],
			    &expected[16].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[1],
			    &expected[17].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[2],
				&expected[18].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[3],
				&expected[19].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[4],
				&expected[20].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[5],
				&expected[21].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[6],
				&expected[22].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[7],
				&expected[23].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[8],
				&expected[24].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[9],
				&expected[25].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[10],
				&expected[26].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[11],
				&expected[27].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[12],
				&expected[28].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[13],
				&expected[29].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[14],
				&expected[30].zmm, sizeof(expected->zmm)));
			ATF_CHECK(!memcmp(&xst.xs_hi16_zmm.xs_hi16_zmm[15],
				&expected[31].zmm, sizeof(expected->zmm)));
#endif
			/* k0..k7 */
			ATF_CHECK(xst.xs_opmask.xs_k[0] == expected[32].zmm.a);
			ATF_CHECK(xst.xs_opmask.xs_k[1] == expected[32].zmm.b);
			ATF_CHECK(xst.xs_opmask.xs_k[2] == expected[32].zmm.c);
			ATF_CHECK(xst.xs_opmask.xs_k[3] == expected[32].zmm.d);
			ATF_CHECK(xst.xs_opmask.xs_k[4] == expected[32].zmm.e);
			ATF_CHECK(xst.xs_opmask.xs_k[5] == expected[32].zmm.f);
			ATF_CHECK(xst.xs_opmask.xs_k[6] == expected[32].zmm.g);
			ATF_CHECK(xst.xs_opmask.xs_k[7] == expected[32].zmm.h);
			/*FALLTHROUGH*/
		case FPREGS_YMM:
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[0],
			    &expected[0].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[1],
			    &expected[1].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[2],
			    &expected[2].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[3],
			    &expected[3].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[4],
			    &expected[4].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[5],
			    &expected[5].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[6],
			    &expected[6].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[7],
			    &expected[7].ymm.c, sizeof(expected->ymm)/2));
#if defined(__x86_64__)
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[8],
			    &expected[8].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[9],
			    &expected[9].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[10],
			    &expected[10].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[11],
			    &expected[11].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[12],
			    &expected[12].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[13],
			    &expected[13].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[14],
			    &expected[14].ymm.c, sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&xst.xs_ymm_hi128.xs_ymm[15],
			    &expected[15].ymm.c, sizeof(expected->ymm)/2));
#endif
			/*FALLTHROUGH*/
		case FPREGS_XMM:
			ATF_CHECK(!memcmp(&fxs->fx_xmm[0], &expected[0].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[1], &expected[1].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[2], &expected[2].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[3], &expected[3].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[4], &expected[4].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[5], &expected[5].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[6], &expected[6].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[7], &expected[7].ymm.a,
			    sizeof(expected->ymm)/2));
#if defined(__x86_64__)
			ATF_CHECK(!memcmp(&fxs->fx_xmm[8], &expected[8].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[9], &expected[9].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[10], &expected[10].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[11], &expected[11].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[12], &expected[12].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[13], &expected[13].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[14], &expected[14].ymm.a,
			    sizeof(expected->ymm)/2));
			ATF_CHECK(!memcmp(&fxs->fx_xmm[15], &expected[15].ymm.a,
			    sizeof(expected->ymm)/2));
#endif
			break;
		}
		break;
	case TEST_SETREGS:
		switch (regs) {
		case GPREGS_32:
#if defined(__i386__)
			gpr.r_eax = expected[0].u32;
			gpr.r_ebx = expected[1].u32;
			gpr.r_ecx = expected[2].u32;
			gpr.r_edx = expected[3].u32;
			gpr.r_esi = expected[4].u32;
			gpr.r_edi = expected[5].u32;
#endif
			break;
		case GPREGS_32_EBP_ESP:
#if defined(__i386__)
			gpr.r_esp = expected[0].u32;
			gpr.r_ebp = expected[1].u32;
#endif
			break;
		case GPREGS_64:
#if defined(__x86_64__)
			gpr.regs[_REG_RAX] = expected[0].u64;
			gpr.regs[_REG_RBX] = expected[1].u64;
			gpr.regs[_REG_RCX] = expected[2].u64;
			gpr.regs[_REG_RDX] = expected[3].u64;
			gpr.regs[_REG_RSI] = expected[4].u64;
			gpr.regs[_REG_RDI] = expected[5].u64;
			gpr.regs[_REG_RSP] = expected[6].u64;
			gpr.regs[_REG_RBP] = expected[7].u64;
#endif
			break;
		case GPREGS_64_R8:
#if defined(__x86_64__)
			gpr.regs[_REG_R8] = expected[0].u64;
			gpr.regs[_REG_R9] = expected[1].u64;
			gpr.regs[_REG_R10] = expected[2].u64;
			gpr.regs[_REG_R11] = expected[3].u64;
			gpr.regs[_REG_R12] = expected[4].u64;
			gpr.regs[_REG_R13] = expected[5].u64;
			gpr.regs[_REG_R14] = expected[6].u64;
			gpr.regs[_REG_R15] = expected[7].u64;
#endif
			break;
		case FPREGS_FPU:
#if defined(__i386__)
			if (regset == TEST_FPREGS) {
				/* SETFPREGS on i386 */
				fpr.fstate.s87_cw = expected_fpu.cw;
				fpr.fstate.s87_sw = expected_fpu.sw;
				fpr.fstate.s87_tw = expected_fpu.tw;
				fpr.fstate.s87_opcode = expected_fpu.opcode;
				fpr.fstate.s87_ip = expected_fpu.ip;
				fpr.fstate.s87_dp = expected_fpu.dp;
			} else
#endif /*defined(__i386__)*/
			{
				/* amd64 or SETXSTATE on i386 */
				fxs->fx_cw = expected_fpu.cw;
				fxs->fx_sw = expected_fpu.sw;
				fxs->fx_tw = expected_fpu.tw_abridged;
				fxs->fx_opcode = expected_fpu.opcode;
				fxs->fx_ip = expected_fpu.ip;
				fxs->fx_dp = expected_fpu.dp;
			}

			ST_EXP(0) = expected_fpu.st[0].sign_exp;
			ST_MAN(0) = expected_fpu.st[0].mantissa;
			ST_EXP(1) = expected_fpu.st[1].sign_exp;
			ST_MAN(1) = expected_fpu.st[1].mantissa;
			ST_EXP(2) = expected_fpu.st[2].sign_exp;
			ST_MAN(2) = expected_fpu.st[2].mantissa;
			ST_EXP(3) = expected_fpu.st[3].sign_exp;
			ST_MAN(3) = expected_fpu.st[3].mantissa;
			ST_EXP(4) = expected_fpu.st[4].sign_exp;
			ST_MAN(4) = expected_fpu.st[4].mantissa;
			ST_EXP(5) = expected_fpu.st[5].sign_exp;
			ST_MAN(5) = expected_fpu.st[5].mantissa;
			ST_EXP(6) = expected_fpu.st[6].sign_exp;
			ST_MAN(6) = expected_fpu.st[6].mantissa;
			ST_EXP(7) = expected_fpu.st[7].sign_exp;
			ST_MAN(7) = expected_fpu.st[7].mantissa;
			break;
		case FPREGS_MM:
			ST_MAN(0) = expected[0].u64;
			ST_MAN(1) = expected[1].u64;
			ST_MAN(2) = expected[2].u64;
			ST_MAN(3) = expected[3].u64;
			ST_MAN(4) = expected[4].u64;
			ST_MAN(5) = expected[5].u64;
			ST_MAN(6) = expected[6].u64;
			ST_MAN(7) = expected[7].u64;
			break;
		case FPREGS_ZMM:
			/* zmm0..zmm15 are split between xmm, ymm_hi128, zmm_hi256 */
			memcpy(&xst.xs_zmm_hi256.xs_zmm[0],
			    &expected[0].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[1],
			    &expected[1].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[2],
			    &expected[2].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[3],
			    &expected[3].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[4],
			    &expected[4].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[5],
			    &expected[5].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[6],
			    &expected[6].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[7],
			    &expected[7].zmm.e, sizeof(expected->zmm)/2);
#if defined(__x86_64__)
			memcpy(&xst.xs_zmm_hi256.xs_zmm[8],
			    &expected[8].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[9],
			    &expected[9].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[10],
			    &expected[10].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[11],
			    &expected[11].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[12],
			    &expected[12].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[13],
			    &expected[13].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[14],
			    &expected[14].zmm.e, sizeof(expected->zmm)/2);
			memcpy(&xst.xs_zmm_hi256.xs_zmm[15],
			    &expected[15].zmm.e, sizeof(expected->zmm)/2);
			/* zmm16..zmm31 are stored as a whole */
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[0],
			    &expected[16].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[1],
			    &expected[17].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[2],
			    &expected[18].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[3],
			    &expected[19].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[4],
			    &expected[20].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[5],
			    &expected[21].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[6],
			    &expected[22].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[7],
			    &expected[23].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[8],
			    &expected[24].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[9],
			    &expected[25].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[10],
			    &expected[26].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[11],
			    &expected[27].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[12],
			    &expected[28].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[13],
			    &expected[29].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[14],
			    &expected[30].zmm, sizeof(expected->zmm));
			memcpy(&xst.xs_hi16_zmm.xs_hi16_zmm[15],
			    &expected[31].zmm, sizeof(expected->zmm));
#endif
			/* k0..k7 */
			xst.xs_opmask.xs_k[0] = expected[32].zmm.a;
			xst.xs_opmask.xs_k[1] = expected[32].zmm.b;
			xst.xs_opmask.xs_k[2] = expected[32].zmm.c;
			xst.xs_opmask.xs_k[3] = expected[32].zmm.d;
			xst.xs_opmask.xs_k[4] = expected[32].zmm.e;
			xst.xs_opmask.xs_k[5] = expected[32].zmm.f;
			xst.xs_opmask.xs_k[6] = expected[32].zmm.g;
			xst.xs_opmask.xs_k[7] = expected[32].zmm.h;
			/*FALLTHROUGH*/
		case FPREGS_YMM:
			memcpy(&xst.xs_ymm_hi128.xs_ymm[0],
			    &expected[0].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[1],
			    &expected[1].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[2],
			    &expected[2].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[3],
			    &expected[3].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[4],
			    &expected[4].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[5],
			    &expected[5].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[6],
			    &expected[6].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[7],
			    &expected[7].ymm.c, sizeof(expected->ymm)/2);
#if defined(__x86_64__)
			memcpy(&xst.xs_ymm_hi128.xs_ymm[8],
			    &expected[8].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[9],
			    &expected[9].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[10],
			    &expected[10].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[11],
			    &expected[11].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[12],
			    &expected[12].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[13],
			    &expected[13].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[14],
			    &expected[14].ymm.c, sizeof(expected->ymm)/2);
			memcpy(&xst.xs_ymm_hi128.xs_ymm[15],
			    &expected[15].ymm.c, sizeof(expected->ymm)/2);
#endif
			/*FALLTHROUGH*/
		case FPREGS_XMM:
			memcpy(&fxs->fx_xmm[0], &expected[0].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[1], &expected[1].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[2], &expected[2].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[3], &expected[3].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[4], &expected[4].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[5], &expected[5].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[6], &expected[6].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[7], &expected[7].ymm.a,
			    sizeof(expected->ymm)/2);
#if defined(__x86_64__)
			memcpy(&fxs->fx_xmm[8], &expected[8].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[9], &expected[9].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[10], &expected[10].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[11], &expected[11].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[12], &expected[12].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[13], &expected[13].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[14], &expected[14].ymm.a,
			    sizeof(expected->ymm)/2);
			memcpy(&fxs->fx_xmm[15], &expected[15].ymm.a,
			    sizeof(expected->ymm)/2);
#endif
			break;
		}

		switch (regset) {
		case TEST_GPREGS:
			DPRINTF("Call SETREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_SETREGS, child, &gpr, 0)
			    != -1);
			break;
		case TEST_XMMREGS:
#if defined(__i386__)
			DPRINTF("Call SETXMMREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_SETXMMREGS, child, &xmm, 0)
			    != -1);
			break;
#else
			/*FALLTHROUGH*/
#endif
		case TEST_FPREGS:
			DPRINTF("Call SETFPREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_SETFPREGS, child, &fpr, 0)
			    != -1);
			break;
		case TEST_XSTATE:
			DPRINTF("Call SETXSTATE for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_SETXSTATE, child, &iov, 0)
			    != -1);
			break;
		}
		break;
	}

#undef ST_EXP
#undef ST_MAN

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define X86_REGISTER_TEST(test, regset, regs, regmode, descr)		\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", descr);				\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
	x86_register_test(regset, regs, regmode);			\
}

X86_REGISTER_TEST(x86_gpregs32_read, TEST_GPREGS, GPREGS_32, TEST_GETREGS,
    "Test reading basic 32-bit gp registers from debugged program "
    "via PT_GETREGS.");
X86_REGISTER_TEST(x86_gpregs32_write, TEST_GPREGS, GPREGS_32, TEST_SETREGS,
    "Test writing basic 32-bit gp registers into debugged program "
    "via PT_SETREGS.");
X86_REGISTER_TEST(x86_gpregs32_core, TEST_GPREGS, GPREGS_32, TEST_COREDUMP,
    "Test reading basic 32-bit gp registers from core dump.");
X86_REGISTER_TEST(x86_gpregs32_ebp_esp_read, TEST_GPREGS, GPREGS_32_EBP_ESP,
    TEST_GETREGS, "Test reading ebp & esp registers from debugged program "
    "via PT_GETREGS.");
X86_REGISTER_TEST(x86_gpregs32_ebp_esp_write, TEST_GPREGS, GPREGS_32_EBP_ESP,
    TEST_SETREGS, "Test writing ebp & esp registers into debugged program "
    "via PT_SETREGS.");
X86_REGISTER_TEST(x86_gpregs32_ebp_esp_core, TEST_GPREGS, GPREGS_32_EBP_ESP,
    TEST_COREDUMP, "Test reading ebp & esp registers from core dump.");

X86_REGISTER_TEST(x86_gpregs64_read, TEST_GPREGS, GPREGS_64, TEST_GETREGS,
    "Test reading basic 64-bit gp registers from debugged program "
    "via PT_GETREGS.");
X86_REGISTER_TEST(x86_gpregs64_write, TEST_GPREGS, GPREGS_64, TEST_SETREGS,
    "Test writing basic 64-bit gp registers into debugged program "
    "via PT_SETREGS.");
X86_REGISTER_TEST(x86_gpregs64_core, TEST_GPREGS, GPREGS_64, TEST_COREDUMP,
    "Test reading basic 64-bit gp registers from core dump.");
X86_REGISTER_TEST(x86_gpregs64_r8_read, TEST_GPREGS, GPREGS_64_R8, TEST_GETREGS,
    "Test reading r8..r15 registers from debugged program via PT_GETREGS.");
X86_REGISTER_TEST(x86_gpregs64_r8_write, TEST_GPREGS, GPREGS_64_R8,
    TEST_SETREGS, "Test writing r8..r15 registers into debugged program "
    "via PT_SETREGS.");
X86_REGISTER_TEST(x86_gpregs64_r8_core, TEST_GPREGS, GPREGS_64_R8,
    TEST_COREDUMP, "Test reading r8..r15 registers from core dump.");

X86_REGISTER_TEST(x86_fpregs_fpu_read, TEST_FPREGS, FPREGS_FPU, TEST_GETREGS,
    "Test reading base FPU registers from debugged program via PT_GETFPREGS.");
X86_REGISTER_TEST(x86_fpregs_fpu_write, TEST_FPREGS, FPREGS_FPU, TEST_SETREGS,
    "Test writing base FPU registers into debugged program via PT_SETFPREGS.");
X86_REGISTER_TEST(x86_fpregs_fpu_core, TEST_FPREGS, FPREGS_FPU, TEST_COREDUMP,
    "Test reading base FPU registers from coredump.");
X86_REGISTER_TEST(x86_fpregs_mm_read, TEST_FPREGS, FPREGS_MM, TEST_GETREGS,
    "Test reading mm0..mm7 registers from debugged program "
    "via PT_GETFPREGS.");
X86_REGISTER_TEST(x86_fpregs_mm_write, TEST_FPREGS, FPREGS_MM, TEST_SETREGS,
    "Test writing mm0..mm7 registers into debugged program "
    "via PT_SETFPREGS.");
X86_REGISTER_TEST(x86_fpregs_mm_core, TEST_FPREGS, FPREGS_MM, TEST_COREDUMP,
    "Test reading mm0..mm7 registers from coredump.");
X86_REGISTER_TEST(x86_fpregs_xmm_read, TEST_XMMREGS, FPREGS_XMM, TEST_GETREGS,
    "Test reading xmm0..xmm15 (..xmm7 on i386) from debugged program "
    "via PT_GETFPREGS (PT_GETXMMREGS on i386).");
X86_REGISTER_TEST(x86_fpregs_xmm_write, TEST_XMMREGS, FPREGS_XMM, TEST_SETREGS,
    "Test writing xmm0..xmm15 (..xmm7 on i386) into debugged program "
    "via PT_SETFPREGS (PT_SETXMMREGS on i386).");
X86_REGISTER_TEST(x86_fpregs_xmm_core, TEST_XMMREGS, FPREGS_XMM, TEST_COREDUMP,
    "Test reading xmm0..xmm15 (..xmm7 on i386) from coredump.");

X86_REGISTER_TEST(x86_xstate_fpu_read, TEST_XSTATE, FPREGS_FPU, TEST_GETREGS,
    "Test reading base FPU registers from debugged program via PT_GETXSTATE.");
X86_REGISTER_TEST(x86_xstate_fpu_write, TEST_XSTATE, FPREGS_FPU, TEST_SETREGS,
    "Test writing base FPU registers into debugged program via PT_SETXSTATE.");
X86_REGISTER_TEST(x86_xstate_fpu_core, TEST_XSTATE, FPREGS_FPU, TEST_COREDUMP,
    "Test reading base FPU registers from core dump via XSTATE note.");
X86_REGISTER_TEST(x86_xstate_mm_read, TEST_XSTATE, FPREGS_MM, TEST_GETREGS,
    "Test reading mm0..mm7 registers from debugged program "
    "via PT_GETXSTATE.");
X86_REGISTER_TEST(x86_xstate_mm_write, TEST_XSTATE, FPREGS_MM, TEST_SETREGS,
    "Test writing mm0..mm7 registers into debugged program "
    "via PT_SETXSTATE.");
X86_REGISTER_TEST(x86_xstate_mm_core, TEST_XSTATE, FPREGS_MM, TEST_COREDUMP,
    "Test reading mm0..mm7 registers from core dump via XSTATE note.");
X86_REGISTER_TEST(x86_xstate_xmm_read, TEST_XSTATE, FPREGS_XMM, TEST_GETREGS,
    "Test reading xmm0..xmm15 (..xmm7 on i386) from debugged program "
    "via PT_GETXSTATE.");
X86_REGISTER_TEST(x86_xstate_xmm_write, TEST_XSTATE, FPREGS_XMM, TEST_SETREGS,
    "Test writing xmm0..xmm15 (..xmm7 on i386) into debugged program "
    "via PT_SETXSTATE.");
X86_REGISTER_TEST(x86_xstate_xmm_core, TEST_XSTATE, FPREGS_XMM, TEST_COREDUMP,
    "Test reading xmm0..xmm15 (..xmm7 on i386) from coredump via XSTATE note.");
X86_REGISTER_TEST(x86_xstate_ymm_read, TEST_XSTATE, FPREGS_YMM, TEST_GETREGS,
    "Test reading ymm0..ymm15 (..ymm7 on i386) from debugged program "
    "via PT_GETXSTATE.");
X86_REGISTER_TEST(x86_xstate_ymm_write, TEST_XSTATE, FPREGS_YMM, TEST_SETREGS,
    "Test writing ymm0..ymm15 (..ymm7 on i386) into debugged program "
    "via PT_SETXSTATE.");
X86_REGISTER_TEST(x86_xstate_ymm_core, TEST_XSTATE, FPREGS_YMM, TEST_COREDUMP,
    "Test reading ymm0..ymm15 (..ymm7 on i386) from coredump via XSTATE note.");
X86_REGISTER_TEST(x86_xstate_zmm_read, TEST_XSTATE, FPREGS_ZMM, TEST_GETREGS,
    "Test reading zmm0..zmm31 (..zmm7 on i386), k0..k7 from debugged program "
    "via PT_GETXSTATE.");
X86_REGISTER_TEST(x86_xstate_zmm_write, TEST_XSTATE, FPREGS_ZMM, TEST_SETREGS,
    "Test writing zmm0..zmm31 (..zmm7 on i386), k0..k7 into debugged program "
    "via PT_SETXSTATE.");
X86_REGISTER_TEST(x86_xstate_zmm_core, TEST_XSTATE, FPREGS_ZMM, TEST_COREDUMP,
    "Test reading zmm0..zmm31 (..zmm7 on i386), k0..k7 from coredump "
    "via XSTATE note.");

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_STATUS)

static void
thread_concurrent_lwp_setup(pid_t child, lwpid_t lwpid)
{
	struct dbreg r;
	union u dr7;

	/* We need to set debug registers for every child */
	DPRINTF("Call GETDBREGS for LWP %d\n", lwpid);
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r, lwpid) != -1);

	dr7.raw = 0;
	/* should be set to 1 according to Intel manual, 17.2 */
	dr7.bits.reserved_10 = 1;
	dr7.bits.local_exact_breakpt = 1;
	dr7.bits.global_exact_breakpt = 1;
	/* use DR0 for breakpoints */
	dr7.bits.global_dr0_breakpoint = 1;
	dr7.bits.condition_dr0 = 0; /* exec */
	dr7.bits.len_dr0 = 0;
	/* use DR1 for watchpoints */
	dr7.bits.global_dr1_breakpoint = 1;
	dr7.bits.condition_dr1 = 1; /* write */
	dr7.bits.len_dr1 = 3; /* 4 bytes */
	r.dr[7] = dr7.raw;
	r.dr[0] = (long)(intptr_t)check_happy;
	r.dr[1] = (long)(intptr_t)&thread_concurrent_watchpoint_var;
	DPRINTF("dr0=%" PRIxREGISTER "\n", r.dr[0]);
	DPRINTF("dr1=%" PRIxREGISTER "\n", r.dr[1]);
	DPRINTF("dr7=%" PRIxREGISTER "\n", r.dr[7]);

	DPRINTF("Call SETDBREGS for LWP %d\n", lwpid);
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r, lwpid) != -1);
}

static enum thread_concurrent_sigtrap_event
thread_concurrent_handle_sigtrap(pid_t child, ptrace_siginfo_t *info)
{
	enum thread_concurrent_sigtrap_event ret = TCSE_UNKNOWN;
	struct dbreg r;
	union u dr7;

	ATF_CHECK_EQ_MSG(info->psi_siginfo.si_code, TRAP_DBREG,
	    "lwp=%d, expected TRAP_DBREG (%d), got %d", info->psi_lwpid,
	    TRAP_DBREG, info->psi_siginfo.si_code);

	DPRINTF("Call GETDBREGS for LWP %d\n", info->psi_lwpid);
	SYSCALL_REQUIRE(ptrace(PT_GETDBREGS, child, &r, info->psi_lwpid) != -1);
	DPRINTF("dr6=%" PRIxREGISTER ", dr7=%" PRIxREGISTER "\n",
	    r.dr[6], r.dr[7]);

	ATF_CHECK_MSG(r.dr[6] & 3, "lwp=%d, got DR6=%" PRIxREGISTER,
	    info->psi_lwpid, r.dr[6]);

	/* Handle only one event at a time, we should get
	 * a separate SIGTRAP for the other one.
	 */
	if (r.dr[6] & 1) {
		r.dr[6] &= ~1;

		/* We need to disable the breakpoint to move
		 * past it.
		 *
		 * TODO: single-step and reenable it?
		 */
		dr7.raw = r.dr[7];
		dr7.bits.global_dr0_breakpoint = 0;
		r.dr[7] = dr7.raw;

		ret = TCSE_BREAKPOINT;
	} else if (r.dr[6] & 2) {
		r.dr[6] &= ~2;
		ret = TCSE_WATCHPOINT;
	}

	DPRINTF("Call SETDBREGS for LWP %d\n", info->psi_lwpid);
	DPRINTF("dr6=%" PRIxREGISTER ", dr7=%" PRIxREGISTER "\n",
		r.dr[6], r.dr[7]);
	SYSCALL_REQUIRE(ptrace(PT_SETDBREGS, child, &r, info->psi_lwpid) != -1);

	return ret;
}

#endif /*defined(TWAIT_HAVE_STATUS)*/

/// ----------------------------------------------------------------------------

#define ATF_TP_ADD_TCS_PTRACE_WAIT_X86() \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_print); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr0); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr1); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr2); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr3); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr0_yield); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr1_yield); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr2_yield); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr3_yield); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr0_continued); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr1_continued); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr2_continued); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_preserve_dr3_continued); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_writeonly_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_writeonly_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_writeonly_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_writeonly_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_writeonly_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_writeonly_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_writeonly_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_writeonly_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_writeonly_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_writeonly_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_writeonly_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_writeonly_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_write_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_write_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_write_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_write_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_write_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_write_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_write_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_write_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_write_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_write_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_write_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_write_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_read_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_read_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_read_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_read_byte); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_read_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_read_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_read_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_read_2bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_variable_readwrite_read_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_variable_readwrite_read_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_variable_readwrite_read_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_variable_readwrite_read_4bytes); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_trap_code); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_trap_code); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_trap_code); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_trap_code); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_dont_inherit_lwp); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_dont_inherit_lwp); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_dont_inherit_lwp); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_dont_inherit_lwp); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr0_dont_inherit_execve); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr1_dont_inherit_execve); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr2_dont_inherit_execve); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, dbregs_dr3_dont_inherit_execve); \
	ATF_TP_ADD_TC_HAVE_DBREGS(tp, x86_cve_2018_8897); \
	ATF_TP_ADD_TC(tp, x86_gpregs32_read); \
	ATF_TP_ADD_TC(tp, x86_gpregs32_write); \
	ATF_TP_ADD_TC(tp, x86_gpregs32_core); \
	ATF_TP_ADD_TC(tp, x86_gpregs32_ebp_esp_read); \
	ATF_TP_ADD_TC(tp, x86_gpregs32_ebp_esp_write); \
	ATF_TP_ADD_TC(tp, x86_gpregs32_ebp_esp_core); \
	ATF_TP_ADD_TC(tp, x86_gpregs64_read); \
	ATF_TP_ADD_TC(tp, x86_gpregs64_write); \
	ATF_TP_ADD_TC(tp, x86_gpregs64_core); \
	ATF_TP_ADD_TC(tp, x86_gpregs64_r8_read); \
	ATF_TP_ADD_TC(tp, x86_gpregs64_r8_write); \
	ATF_TP_ADD_TC(tp, x86_gpregs64_r8_core); \
	ATF_TP_ADD_TC(tp, x86_fpregs_fpu_read); \
	ATF_TP_ADD_TC(tp, x86_fpregs_fpu_write); \
	ATF_TP_ADD_TC(tp, x86_fpregs_fpu_core); \
	ATF_TP_ADD_TC(tp, x86_fpregs_mm_read); \
	ATF_TP_ADD_TC(tp, x86_fpregs_mm_write); \
	ATF_TP_ADD_TC(tp, x86_fpregs_mm_core); \
	ATF_TP_ADD_TC(tp, x86_fpregs_xmm_read); \
	ATF_TP_ADD_TC(tp, x86_fpregs_xmm_write); \
	ATF_TP_ADD_TC(tp, x86_fpregs_xmm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_fpu_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_fpu_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_fpu_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_mm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_mm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_mm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_xmm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_xmm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_xmm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_ymm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_ymm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_ymm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_zmm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_zmm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_zmm_core);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_X86()
#endif
