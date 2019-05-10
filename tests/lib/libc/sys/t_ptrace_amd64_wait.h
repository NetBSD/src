/*	$NetBSD: t_ptrace_amd64_wait.h,v 1.10 2019/05/10 16:24:35 mgorny Exp $	*/

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

#if defined(__x86_64__)

/// ----------------------------------------------------------------------------

ATF_TC(x86_64_regs1);
ATF_TC_HEAD(x86_64_regs1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Call PT_GETREGS and iterate over General Purpose registers");
}

ATF_TC_BODY(x86_64_regs1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct reg r;

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

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &r, 0) != -1);

	DPRINTF("RAX=%#" PRIxREGISTER "\n", r.regs[_REG_RAX]);
	DPRINTF("RBX=%#" PRIxREGISTER "\n", r.regs[_REG_RBX]);
	DPRINTF("RCX=%#" PRIxREGISTER "\n", r.regs[_REG_RCX]);
	DPRINTF("RDX=%#" PRIxREGISTER "\n", r.regs[_REG_RDX]);

	DPRINTF("RDI=%#" PRIxREGISTER "\n", r.regs[_REG_RDI]);
	DPRINTF("RSI=%#" PRIxREGISTER "\n", r.regs[_REG_RSI]);

	DPRINTF("GS=%#" PRIxREGISTER "\n", r.regs[_REG_GS]);
	DPRINTF("FS=%#" PRIxREGISTER "\n", r.regs[_REG_FS]);
	DPRINTF("ES=%#" PRIxREGISTER "\n", r.regs[_REG_ES]);
	DPRINTF("DS=%#" PRIxREGISTER "\n", r.regs[_REG_DS]);
	DPRINTF("CS=%#" PRIxREGISTER "\n", r.regs[_REG_CS]);
	DPRINTF("SS=%#" PRIxREGISTER "\n", r.regs[_REG_SS]);

	DPRINTF("RSP=%#" PRIxREGISTER "\n", r.regs[_REG_RSP]);
	DPRINTF("RIP=%#" PRIxREGISTER "\n", r.regs[_REG_RIP]);

	DPRINTF("RFLAGS=%#" PRIxREGISTER "\n", r.regs[_REG_RFLAGS]);

	DPRINTF("R8=%#" PRIxREGISTER "\n", r.regs[_REG_R8]);
	DPRINTF("R9=%#" PRIxREGISTER "\n", r.regs[_REG_R9]);
	DPRINTF("R10=%#" PRIxREGISTER "\n", r.regs[_REG_R10]);
	DPRINTF("R11=%#" PRIxREGISTER "\n", r.regs[_REG_R11]);
	DPRINTF("R12=%#" PRIxREGISTER "\n", r.regs[_REG_R12]);
	DPRINTF("R13=%#" PRIxREGISTER "\n", r.regs[_REG_R13]);
	DPRINTF("R14=%#" PRIxREGISTER "\n", r.regs[_REG_R14]);
	DPRINTF("R15=%#" PRIxREGISTER "\n", r.regs[_REG_R15]);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(x86_64_regs_gp_read);
ATF_TC_HEAD(x86_64_regs_gp_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set general-purpose reg values from debugged program and read "
		"them via PT_GETREGS, comparing values against expected.");
}

ATF_TC_BODY(x86_64_regs_gp_read, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint64_t rax = 0x0001020304050607;
	const uint64_t rbx = 0x1011121314151617;
	const uint64_t rcx = 0x2021222324252627;
	const uint64_t rdx = 0x3031323334353637;
	const uint64_t rsi = 0x4041424344454647;
	const uint64_t rdi = 0x5051525354555657;
	const uint64_t rsp = 0x6061626364656667;
	const uint64_t rbp = 0x7071727374757677;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

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
			: "a"(rax), "b"(rbx), "c"(rcx), "d"(rdx), "S"(rsi), "D"(rdi),
			  "i"(rsp), "i"(rbp)
			: "%r8", "%r9"
		);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RAX], rax);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RBX], rbx);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RCX], rcx);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RDX], rdx);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RSI], rsi);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RDI], rdi);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RSP], rsp);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_RBP], rbp);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(x86_64_regs_gp_write);
ATF_TC_HEAD(x86_64_regs_gp_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set general-purpose reg values into a debugged program via "
		"PT_SETREGS and compare the result against expected.");
}

ATF_TC_BODY(x86_64_regs_gp_write, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint64_t rax = 0x0001020304050607;
	const uint64_t rbx = 0x1011121314151617;
	const uint64_t rcx = 0x2021222324252627;
	const uint64_t rdx = 0x3031323334353637;
	const uint64_t rsi = 0x4041424344454647;
	const uint64_t rdi = 0x5051525354555657;
	const uint64_t rsp = 0x6061626364656667;
	const uint64_t rbp = 0x7071727374757677;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		const uint64_t fill = 0x0F0F0F0F0F0F0F0F;
		uint64_t v_rax, v_rbx, v_rcx, v_rdx, v_rsi, v_rdi, v_rsp, v_rbp;

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

		__asm__ __volatile__(
			/* save rsp & rbp */
			"movq    %%rsp, %4\n\t"
			"movq    %%rbp, %5\n\t"
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
			"xchgq    %%rsp, %4\n\t"
			"xchgq    %%rbp, %5\n\t"
			: "=a"(v_rax), "=b"(v_rbx), "=c"(v_rcx), "=d"(v_rdx), "=r"(v_rsp),
			"=r"(v_rbp), "=S"(v_rsi), "=D"(v_rdi)
			: "g"(fill)
			:
		);

		FORKEE_ASSERT_EQ(v_rax, rax);
		FORKEE_ASSERT_EQ(v_rbx, rbx);
		FORKEE_ASSERT_EQ(v_rcx, rcx);
		FORKEE_ASSERT_EQ(v_rdx, rdx);
		FORKEE_ASSERT_EQ(v_rsi, rsi);
		FORKEE_ASSERT_EQ(v_rdi, rdi);
		FORKEE_ASSERT_EQ(v_rsp, rsp);
		FORKEE_ASSERT_EQ(v_rbp, rbp);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

	gpr.regs[_REG_RAX] = rax;
	gpr.regs[_REG_RBX] = rbx;
	gpr.regs[_REG_RCX] = rcx;
	gpr.regs[_REG_RDX] = rdx;
	gpr.regs[_REG_RSI] = rsi;
	gpr.regs[_REG_RDI] = rdi;
	gpr.regs[_REG_RSP] = rsp;
	gpr.regs[_REG_RBP] = rbp;

	DPRINTF("Call SETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_SETREGS, child, &gpr, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(x86_64_regs_r8_read);
ATF_TC_HEAD(x86_64_regs_r8_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set r8..r15 reg values from debugged program and read "
		"them via PT_GETREGS, comparing values against expected.");
}

ATF_TC_BODY(x86_64_regs_r8_read, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint64_t r8[] = {
		0x0001020304050607,
		0x1011121314151617,
		0x2021222324252627,
		0x3031323334353637,
		0x4041424344454647,
		0x5051525354555657,
		0x6061626364656667,
		0x7071727374757677,
	};

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

		__asm__ __volatile__(
			"movq    0x00(%%rbx), %%r8\n\t"
			"movq    0x08(%%rbx), %%r9\n\t"
			"movq    0x10(%%rbx), %%r10\n\t"
			"movq    0x18(%%rbx), %%r11\n\t"
			"movq    0x20(%%rbx), %%r12\n\t"
			"movq    0x28(%%rbx), %%r13\n\t"
			"movq    0x30(%%rbx), %%r14\n\t"
			"movq    0x38(%%rbx), %%r15\n\t"
			"int3\n\t"
			:
			: "b"(r8)
			: "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
		);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R8], r8[0]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R9], r8[1]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R10], r8[2]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R11], r8[3]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R12], r8[4]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R13], r8[5]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R14], r8[6]);
	ATF_CHECK_EQ((uint64_t)gpr.regs[_REG_R15], r8[7]);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(x86_64_regs_r8_write);
ATF_TC_HEAD(x86_64_regs_r8_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set r8..r15 reg values into a debugged program via "
		"PT_SETREGS and compare the result against expected.");
}

ATF_TC_BODY(x86_64_regs_r8_write, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint64_t r8[] = {
		0x0001020304050607,
		0x1011121314151617,
		0x2021222324252627,
		0x3031323334353637,
		0x4041424344454647,
		0x5051525354555657,
		0x6061626364656667,
		0x7071727374757677,
	};

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		const uint64_t fill = 0x0F0F0F0F0F0F0F0F;
		uint64_t v_r8[8];

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

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
			"movq    %%r9, 0x08(%0)\n\t"
			"movq    %%r10, 0x10(%0)\n\t"
			"movq    %%r11, 0x18(%0)\n\t"
			"movq    %%r12, 0x20(%0)\n\t"
			"movq    %%r13, 0x28(%0)\n\t"
			"movq    %%r14, 0x30(%0)\n\t"
			"movq    %%r15, 0x38(%0)\n\t"
			:
			: "a"(v_r8), "m"(fill)
			: "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
		);

		FORKEE_ASSERT_EQ(v_r8[0], r8[0]);
		FORKEE_ASSERT_EQ(v_r8[1], r8[1]);
		FORKEE_ASSERT_EQ(v_r8[2], r8[2]);
		FORKEE_ASSERT_EQ(v_r8[3], r8[3]);
		FORKEE_ASSERT_EQ(v_r8[4], r8[4]);
		FORKEE_ASSERT_EQ(v_r8[5], r8[5]);
		FORKEE_ASSERT_EQ(v_r8[6], r8[6]);
		FORKEE_ASSERT_EQ(v_r8[7], r8[7]);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

	gpr.regs[_REG_R8] = r8[0];
	gpr.regs[_REG_R9] = r8[1];
	gpr.regs[_REG_R10] = r8[2];
	gpr.regs[_REG_R11] = r8[3];
	gpr.regs[_REG_R12] = r8[4];
	gpr.regs[_REG_R13] = r8[5];
	gpr.regs[_REG_R14] = r8[6];
	gpr.regs[_REG_R15] = r8[7];

	DPRINTF("Call SETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_SETREGS, child, &gpr, 0) != -1);

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


#define ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64() \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_regs1); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_regs_gp_read); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_regs_gp_write); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_regs_r8_read); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_regs_r8_write);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64()
#endif
