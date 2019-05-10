/*	$NetBSD: t_ptrace_i386_wait.h,v 1.8 2019/05/10 16:24:35 mgorny Exp $	*/

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

#if defined(__i386__)
ATF_TC(i386_regs1);
ATF_TC_HEAD(i386_regs1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Call PT_GETREGS and iterate over General Purpose registers");
}

ATF_TC_BODY(i386_regs1, tc)
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

	DPRINTF("EAX=%#" PRIxREGISTER "\n", r.r_eax);
	DPRINTF("EBX=%#" PRIxREGISTER "\n", r.r_ebx);
	DPRINTF("ECX=%#" PRIxREGISTER "\n", r.r_ecx);
	DPRINTF("EDX=%#" PRIxREGISTER "\n", r.r_edx);

	DPRINTF("ESP=%#" PRIxREGISTER "\n", r.r_esp);
	DPRINTF("EBP=%#" PRIxREGISTER "\n", r.r_ebp);

	DPRINTF("ESI=%#" PRIxREGISTER "\n", r.r_esi);
	DPRINTF("EDI=%#" PRIxREGISTER "\n", r.r_edi);

	DPRINTF("EIP=%#" PRIxREGISTER "\n", r.r_eip);

	DPRINTF("EFLAGS=%#" PRIxREGISTER "\n", r.r_eflags);

	DPRINTF("CS=%#" PRIxREGISTER "\n", r.r_cs);
	DPRINTF("SS=%#" PRIxREGISTER "\n", r.r_ss);
	DPRINTF("DS=%#" PRIxREGISTER "\n", r.r_ds);
	DPRINTF("ES=%#" PRIxREGISTER "\n", r.r_es);
	DPRINTF("FS=%#" PRIxREGISTER "\n", r.r_fs);
	DPRINTF("GS=%#" PRIxREGISTER "\n", r.r_gs);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(i386_regs_gp_read);
ATF_TC_HEAD(i386_regs_gp_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set general-purpose reg values from debugged program and read "
		"them via PT_GETREGS, comparing values against expected.");
}

ATF_TC_BODY(i386_regs_gp_read, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint32_t eax = 0x00010203;
	const uint32_t ebx = 0x10111213;
	const uint32_t ecx = 0x20212223;
	const uint32_t edx = 0x30313233;
	const uint32_t esi = 0x40414243;
	const uint32_t edi = 0x50515253;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

		__asm__ __volatile__(
			"int3\n\t"
			:
			: "a"(eax), "b"(ebx), "c"(ecx), "d"(edx), "S"(esi), "D"(edi)
			:
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

	ATF_CHECK_EQ((uint32_t)gpr.r_eax, eax);
	ATF_CHECK_EQ((uint32_t)gpr.r_ebx, ebx);
	ATF_CHECK_EQ((uint32_t)gpr.r_ecx, ecx);
	ATF_CHECK_EQ((uint32_t)gpr.r_edx, edx);
	ATF_CHECK_EQ((uint32_t)gpr.r_esi, esi);
	ATF_CHECK_EQ((uint32_t)gpr.r_edi, edi);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(i386_regs_gp_write);
ATF_TC_HEAD(i386_regs_gp_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set general-purpose reg values into a debugged program via "
		"PT_SETREGS and compare the result against expected.");
}

ATF_TC_BODY(i386_regs_gp_write, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint32_t eax = 0x00010203;
	const uint32_t ebx = 0x10111213;
	const uint32_t ecx = 0x20212223;
	const uint32_t edx = 0x30313233;
	const uint32_t esi = 0x40414243;
	const uint32_t edi = 0x50515253;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		const uint64_t fill = 0x0F0F0F0F;
		uint32_t v_eax, v_ebx, v_ecx, v_edx, v_esi, v_edi;

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

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
			: "=a"(v_eax), "=b"(v_ebx), "=c"(v_ecx), "=d"(v_edx), "=S"(v_esi),
			"=D"(v_edi)
			: "g"(fill)
			:
		);

		FORKEE_ASSERT_EQ(v_eax, eax);
		FORKEE_ASSERT_EQ(v_ebx, ebx);
		FORKEE_ASSERT_EQ(v_ecx, ecx);
		FORKEE_ASSERT_EQ(v_edx, edx);
		FORKEE_ASSERT_EQ(v_esi, esi);
		FORKEE_ASSERT_EQ(v_edi, edi);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

	gpr.r_eax = eax;
	gpr.r_ebx = ebx;
	gpr.r_ecx = ecx;
	gpr.r_edx = edx;
	gpr.r_esi = esi;
	gpr.r_edi = edi;

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

ATF_TC(i386_regs_ebp_esp_read);
ATF_TC_HEAD(i386_regs_ebp_esp_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set EBP & ESP reg values from debugged program and read "
		"them via PT_GETREGS, comparing values against expected.");
}

ATF_TC_BODY(i386_regs_ebp_esp_read, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint32_t esp = 0x60616263;
	const uint32_t ebp = 0x70717273;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

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
			: "ri"(esp), "ri"(ebp)
			: "%eax", "%ebx"
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

	ATF_CHECK_EQ((uint32_t)gpr.r_esp, esp);
	ATF_CHECK_EQ((uint32_t)gpr.r_ebp, ebp);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(i386_regs_ebp_esp_write);
ATF_TC_HEAD(i386_regs_ebp_esp_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Set EBP & ESP reg values into a debugged program via "
		"PT_SETREGS and compare the result against expected.");
}

ATF_TC_BODY(i386_regs_ebp_esp_write, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	struct reg gpr;

	const uint32_t esp = 0x60616263;
	const uint32_t ebp = 0x70717273;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		const uint64_t fill = 0x0F0F0F0F;
		uint32_t v_esp, v_ebp;

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before running assembly from child\n");

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
			: "=r"(v_esp), "=r"(v_ebp)
			: "g"(fill)
			:
		);

		FORKEE_ASSERT_EQ(v_esp, esp);
		FORKEE_ASSERT_EQ(v_ebp, ebp);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Call GETREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

	gpr.r_esp = esp;
	gpr.r_ebp = ebp;

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

#define ATF_TP_ADD_TCS_PTRACE_WAIT_I386() \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, i386_regs1); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, i386_regs_gp_read); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, i386_regs_gp_write); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, i386_regs_ebp_esp_read); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, i386_regs_ebp_esp_write);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_I386()
#endif
