/*	$NetBSD: t_ptrace_x86_wait.h,v 1.18 2020/01/08 17:23:34 mgorny Exp $	*/

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

volatile lwpid_t x86_the_lwp_id = 0;

static void __used
x86_lwp_main_func(void *arg)
{
	x86_the_lwp_id = _lwp_self();
	_lwp_exit();
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
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
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

		DPRINTF("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		DPRINTF("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, x86_lwp_main_func, NULL, NULL, stack,
		    ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, x86_the_lwp_id);
		FORKEE_ASSERT_EQ(lid, x86_the_lwp_id);

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
		"       movq $farjmp32, %%rax\n\t"
		"       ljmp *(%%rax)\n\t"
		"farjmp32:\n\t"
		"       .long trigger32\n\t"
		"       .word 0x73\n\t"
		"       .code32\n\t"
		"trigger32:\n\t"
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
		uint64_t a, b, c, d;
	} ymm;
	struct {
		uint64_t a, b;
	} xmm;
	uint64_t u64;
	uint32_t u32;
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
	FPREGS_MM,
	FPREGS_XMM,
	/* TEST_XSTATE */
	FPREGS_YMM
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
		"movq    %%r8, 0x00(%0)\n\t"
		"movq    %%r9, 0x20(%0)\n\t"
		"movq    %%r10, 0x40(%0)\n\t"
		"movq    %%r11, 0x60(%0)\n\t"
		"movq    %%r12, 0x80(%0)\n\t"
		"movq    %%r13, 0xA0(%0)\n\t"
		"movq    %%r14, 0xC0(%0)\n\t"
		"movq    %%r15, 0xE0(%0)\n\t"
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
		"movq    0x00(%0), %%r8\n\t"
		"movq    0x20(%0), %%r9\n\t"
		"movq    0x40(%0), %%r10\n\t"
		"movq    0x60(%0), %%r11\n\t"
		"movq    0x80(%0), %%r12\n\t"
		"movq    0xA0(%0), %%r13\n\t"
		"movq    0xC0(%0), %%r14\n\t"
		"movq    0xE0(%0), %%r15\n\t"
		"int3\n\t"
		:
		: "b"(data)
		: "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
	);
#else
	__unreachable();
#endif
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
		"movq    %%mm0, 0x00(%0)\n\t"
		"movq    %%mm1, 0x20(%0)\n\t"
		"movq    %%mm2, 0x40(%0)\n\t"
		"movq    %%mm3, 0x60(%0)\n\t"
		"movq    %%mm4, 0x80(%0)\n\t"
		"movq    %%mm5, 0xA0(%0)\n\t"
		"movq    %%mm6, 0xC0(%0)\n\t"
		"movq    %%mm7, 0xE0(%0)\n\t"
		:
		: "a"(out), "m"(fill)
		: "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
	);
}

__attribute__((target("mmx")))
static __inline void set_mm_regs(const union x86_test_register data[])
{
	__asm__ __volatile__(
		"movq    0x00(%0), %%mm0\n\t"
		"movq    0x20(%0), %%mm1\n\t"
		"movq    0x40(%0), %%mm2\n\t"
		"movq    0x60(%0), %%mm3\n\t"
		"movq    0x80(%0), %%mm4\n\t"
		"movq    0xA0(%0), %%mm5\n\t"
		"movq    0xC0(%0), %%mm6\n\t"
		"movq    0xE0(%0), %%mm7\n\t"
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
		"movaps  %%xmm1, 0x020(%0)\n\t"
		"movaps  %%xmm2, 0x040(%0)\n\t"
		"movaps  %%xmm3, 0x060(%0)\n\t"
		"movaps  %%xmm4, 0x080(%0)\n\t"
		"movaps  %%xmm5, 0x0A0(%0)\n\t"
		"movaps  %%xmm6, 0x0C0(%0)\n\t"
		"movaps  %%xmm7, 0x0E0(%0)\n\t"
#if defined(__x86_64__)
		"movaps  %%xmm8, 0x100(%0)\n\t"
		"movaps  %%xmm9, 0x120(%0)\n\t"
		"movaps  %%xmm10, 0x140(%0)\n\t"
		"movaps  %%xmm11, 0x160(%0)\n\t"
		"movaps  %%xmm12, 0x180(%0)\n\t"
		"movaps  %%xmm13, 0x1A0(%0)\n\t"
		"movaps  %%xmm14, 0x1C0(%0)\n\t"
		"movaps  %%xmm15, 0x1E0(%0)\n\t"
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
		"movaps   0x020(%0), %%xmm1\n\t"
		"movaps   0x040(%0), %%xmm2\n\t"
		"movaps   0x060(%0), %%xmm3\n\t"
		"movaps   0x080(%0), %%xmm4\n\t"
		"movaps   0x0A0(%0), %%xmm5\n\t"
		"movaps   0x0C0(%0), %%xmm6\n\t"
		"movaps   0x0E0(%0), %%xmm7\n\t"
#if defined(__x86_64__)
		"movaps   0x100(%0), %%xmm8\n\t"
		"movaps   0x120(%0), %%xmm9\n\t"
		"movaps   0x140(%0), %%xmm10\n\t"
		"movaps   0x160(%0), %%xmm11\n\t"
		"movaps   0x180(%0), %%xmm12\n\t"
		"movaps   0x1A0(%0), %%xmm13\n\t"
		"movaps   0x1C0(%0), %%xmm14\n\t"
		"movaps   0x1E0(%0), %%xmm15\n\t"
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
		{ 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F,
		  0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F }
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
		"vmovaps %%ymm1,  0x020(%0)\n\t"
		"vmovaps %%ymm2,  0x040(%0)\n\t"
		"vmovaps %%ymm3,  0x060(%0)\n\t"
		"vmovaps %%ymm4,  0x080(%0)\n\t"
		"vmovaps %%ymm5,  0x0A0(%0)\n\t"
		"vmovaps %%ymm6,  0x0C0(%0)\n\t"
		"vmovaps %%ymm7,  0x0E0(%0)\n\t"
#if defined(__x86_64__)
		"vmovaps %%ymm8,  0x100(%0)\n\t"
		"vmovaps %%ymm9,  0x120(%0)\n\t"
		"vmovaps %%ymm10, 0x140(%0)\n\t"
		"vmovaps %%ymm11, 0x160(%0)\n\t"
		"vmovaps %%ymm12, 0x180(%0)\n\t"
		"vmovaps %%ymm13, 0x1A0(%0)\n\t"
		"vmovaps %%ymm14, 0x1C0(%0)\n\t"
		"vmovaps %%ymm15, 0x1E0(%0)\n\t"
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
		"vmovaps  0x020(%0), %%ymm1\n\t"
		"vmovaps  0x040(%0), %%ymm2\n\t"
		"vmovaps  0x060(%0), %%ymm3\n\t"
		"vmovaps  0x080(%0), %%ymm4\n\t"
		"vmovaps  0x0A0(%0), %%ymm5\n\t"
		"vmovaps  0x0C0(%0), %%ymm6\n\t"
		"vmovaps  0x0E0(%0), %%ymm7\n\t"
#if defined(__x86_64__)
		"vmovaps  0x100(%0), %%ymm8\n\t"
		"vmovaps  0x120(%0), %%ymm9\n\t"
		"vmovaps  0x140(%0), %%ymm10\n\t"
		"vmovaps  0x160(%0), %%ymm11\n\t"
		"vmovaps  0x180(%0), %%ymm12\n\t"
		"vmovaps  0x1A0(%0), %%ymm13\n\t"
		"vmovaps  0x1C0(%0), %%ymm14\n\t"
		"vmovaps  0x1E0(%0), %%ymm15\n\t"
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

	const union x86_test_register expected[] __aligned(32) = {
		{{ 0x0706050403020100, 0x0F0E0D0C0B0A0908,
		   0x1716151413121110, 0x1F1E1D1C1B1A1918, }},
		{{ 0x0807060504030201, 0x100F0E0D0C0B0A09,
		   0x1817161514131211, 0x201F1E1D1C1B1A19, }},
		{{ 0x0908070605040302, 0x11100F0E0D0C0B0A,
		   0x1918171615141312, 0x21201F1E1D1C1B1A, }},
		{{ 0x0A09080706050403, 0x1211100F0E0D0C0B,
		   0x1A19181716151413, 0x2221201F1E1D1C1B, }},
		{{ 0x0B0A090807060504, 0x131211100F0E0D0C,
		   0x1B1A191817161514, 0x232221201F1E1D1C, }},
		{{ 0x0C0B0A0908070605, 0x14131211100F0E0D,
		   0x1C1B1A1918171615, 0x24232221201F1E1D, }},
		{{ 0x0D0C0B0A09080706, 0x1514131211100F0E,
		   0x1D1C1B1A19181716, 0x2524232221201F1E, }},
		{{ 0x0E0D0C0B0A090807, 0x161514131211100F,
		   0x1E1D1C1B1A191817, 0x262524232221201F, }},
		{{ 0x0F0E0D0C0B0A0908, 0x1716151413121110,
		   0x1F1E1D1C1B1A1918, 0x2726252423222120, }},
		{{ 0x100F0E0D0C0B0A09, 0x1817161514131211,
		   0x201F1E1D1C1B1A19, 0x2827262524232221, }},
		{{ 0x11100F0E0D0C0B0A, 0x1918171615141312,
		   0x21201F1E1D1C1B1A, 0x2928272625242322, }},
		{{ 0x1211100F0E0D0C0B, 0x1A19181716151413,
		   0x2221201F1E1D1C1B, 0x2A29282726252423, }},
		{{ 0x131211100F0E0D0C, 0x1B1A191817161514,
		   0x232221201F1E1D1C, 0x2B2A292827262524, }},
		{{ 0x14131211100F0E0D, 0x1C1B1A1918171615,
		   0x24232221201F1E1D, 0x2C2B2A2928272625, }},
		{{ 0x1514131211100F0E, 0x1D1C1B1A19181716,
		   0x2524232221201F1E, 0x2D2C2B2A29282726, }},
		{{ 0x161514131211100F, 0x1E1D1C1B1A191817,
		   0x262524232221201F, 0x2E2D2C2B2A292827, }},
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
	case FPREGS_MM:
	case FPREGS_XMM:
	case FPREGS_YMM:
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

		DPRINTF("Before invoking cpuid\n");
		if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
			atf_tc_skip("CPUID is not supported by the CPU");

		DPRINTF("cpuid: ECX = %08x, EDX = %08xd\n", ecx, edx);

		switch (regs) {
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
			__unreachable();
		}
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		union x86_test_register vals[16] __aligned(32);

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
			case FPREGS_MM:
				set_mm_regs(expected);
				break;
			case FPREGS_XMM:
				set_xmm_regs(expected);
				break;
			case FPREGS_YMM:
				set_ymm_regs(expected);
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
			case FPREGS_MM:
				get_mm_regs(vals);
				break;
			case FPREGS_XMM:
				get_xmm_regs(vals);
				break;
			case FPREGS_YMM:
				get_ymm_regs(vals);
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
		case FPREGS_MM:
			xst_flags |= XCR0_X87;
			break;
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
		switch (regset) {
		case TEST_GPREGS:
			ATF_REQUIRE(regs < FPREGS_MM);
			DPRINTF("Call GETREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0)
			    != -1);
			break;
		case TEST_XMMREGS:
#if defined(__i386__)
			ATF_REQUIRE(regs >= FPREGS_MM && regs < FPREGS_YMM);
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
			ATF_REQUIRE(regs >= FPREGS_MM && regs < FPREGS_YMM);
			fxs = &fpr.fxstate;
#else
			ATF_REQUIRE(regs >= FPREGS_MM && regs < FPREGS_XMM);
#endif
			DPRINTF("Call GETFPREGS for the child process\n");
			SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0)
			    != -1);
			break;
		case TEST_XSTATE:
			ATF_REQUIRE(regs >= FPREGS_MM);
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

		switch (regset) {
		case TEST_GPREGS:
			ATF_REQUIRE(regs < FPREGS_MM);
			DPRINTF("Parse core file for PT_GETREGS\n");
			ATF_REQUIRE_EQ(core_find_note(core_path,
			    "NetBSD-CORE@1", PT_GETREGS, &gpr, sizeof(gpr)),
			    sizeof(gpr));
			break;
		case TEST_XMMREGS:
#if defined(__i386__)
			ATF_REQUIRE(regs >= FPREGS_MM && regs < FPREGS_YMM);
			unlink(core_path);
			atf_tc_skip("XMMREGS not supported in core dumps");
			break;
#else
			/*FALLTHROUGH*/
#endif
		case TEST_FPREGS:
#if defined(__x86_64__)
			ATF_REQUIRE(regs >= FPREGS_MM && regs < FPREGS_YMM);
			fxs = &fpr.fxstate;
#else
			ATF_REQUIRE(regs >= FPREGS_MM && regs < FPREGS_XMM);
#endif
			DPRINTF("Parse core file for PT_GETFPREGS\n");
			ATF_REQUIRE_EQ(core_find_note(core_path,
			    "NetBSD-CORE@1", PT_GETFPREGS, &fpr, sizeof(fpr)),
			    sizeof(fpr));
			break;
		case TEST_XSTATE:
			ATF_REQUIRE(regs >= FPREGS_MM);
			DPRINTF("Parse core file for PT_GETXSTATE\n");
			ATF_REQUIRE_EQ(core_find_note(core_path,
			    "NetBSD-CORE@1", PT_GETXSTATE, &xst, sizeof(xst)),
			    sizeof(xst));
			ATF_REQUIRE((xst.xs_xstate_bv & xst_flags)
			    == xst_flags);
			fxs = &xst.xs_fxsave;
			break;
		}
		unlink(core_path);
	}

#if defined(__x86_64__)
#define MM_REG(n) fpr.fxstate.fx_87_ac[n].r.f87_mantissa
#else
#define MM_REG(n) fpr.fstate.s87_ac[n].f87_mantissa
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
		case FPREGS_MM:
			if (regset == TEST_FPREGS) {
				ATF_CHECK_EQ(MM_REG(0), expected[0].u64);
				ATF_CHECK_EQ(MM_REG(1), expected[1].u64);
				ATF_CHECK_EQ(MM_REG(2), expected[2].u64);
				ATF_CHECK_EQ(MM_REG(3), expected[3].u64);
				ATF_CHECK_EQ(MM_REG(4), expected[4].u64);
				ATF_CHECK_EQ(MM_REG(5), expected[5].u64);
				ATF_CHECK_EQ(MM_REG(6), expected[6].u64);
				ATF_CHECK_EQ(MM_REG(7), expected[7].u64);
			} else {
				ATF_CHECK_EQ(fxs->fx_87_ac[0].r.f87_mantissa,
			    expected[0].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[1].r.f87_mantissa,
			    expected[1].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[2].r.f87_mantissa,
			    expected[2].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[3].r.f87_mantissa,
			    expected[3].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[4].r.f87_mantissa,
			    expected[4].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[5].r.f87_mantissa,
			    expected[5].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[6].r.f87_mantissa,
			    expected[6].u64);
				ATF_CHECK_EQ(fxs->fx_87_ac[7].r.f87_mantissa,
			    expected[7].u64);
			}
			break;
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
		case FPREGS_MM:
			if (regset == TEST_FPREGS) {
				MM_REG(0) = expected[0].u64;
				MM_REG(1) = expected[1].u64;
				MM_REG(2) = expected[2].u64;
				MM_REG(3) = expected[3].u64;
				MM_REG(4) = expected[4].u64;
				MM_REG(5) = expected[5].u64;
				MM_REG(6) = expected[6].u64;
				MM_REG(7) = expected[7].u64;
			} else {
				fxs->fx_87_ac[0].r.f87_mantissa =
				    expected[0].u64;
				fxs->fx_87_ac[1].r.f87_mantissa =
				    expected[1].u64;
				fxs->fx_87_ac[2].r.f87_mantissa =
				    expected[2].u64;
				fxs->fx_87_ac[3].r.f87_mantissa =
				    expected[3].u64;
				fxs->fx_87_ac[4].r.f87_mantissa =
				    expected[4].u64;
				fxs->fx_87_ac[5].r.f87_mantissa =
				    expected[5].u64;
				fxs->fx_87_ac[6].r.f87_mantissa =
				    expected[6].u64;
				fxs->fx_87_ac[7].r.f87_mantissa =
				    expected[7].u64;
			}
			break;
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

#undef MM_REG

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
	ATF_TP_ADD_TC(tp, x86_fpregs_mm_read); \
	ATF_TP_ADD_TC(tp, x86_fpregs_mm_write); \
	ATF_TP_ADD_TC(tp, x86_fpregs_mm_core); \
	ATF_TP_ADD_TC(tp, x86_fpregs_xmm_read); \
	ATF_TP_ADD_TC(tp, x86_fpregs_xmm_write); \
	ATF_TP_ADD_TC(tp, x86_fpregs_xmm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_mm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_mm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_mm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_xmm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_xmm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_xmm_core); \
	ATF_TP_ADD_TC(tp, x86_xstate_ymm_read); \
	ATF_TP_ADD_TC(tp, x86_xstate_ymm_write); \
	ATF_TP_ADD_TC(tp, x86_xstate_ymm_core);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_X86()
#endif
