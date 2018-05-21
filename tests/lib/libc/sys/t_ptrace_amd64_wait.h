/*	$NetBSD: t_ptrace_amd64_wait.h,v 1.2.2.1 2018/05/21 04:36:17 pgoyette Exp $	*/

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

/// ----------------------------------------------------------------------------

ATF_TC(x86_64_cve_2018_8897);
ATF_TC_HEAD(x86_64_cve_2018_8897, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify mitigation for CVE-2018-8897 (POP SS debug exception)");
}

#define CVE_2018_8897_PAGE 0x5000 /* page addressable by 32-bit registers */

static void
trigger_cve_2018_8897(void)
{
	/*
	 * A function to trigger the POP SS (CVE-2018-8897) vulnerability
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
}

ATF_TC_BODY(x86_64_cve_2018_8897, tc)
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

		trap_page = mmap((void *)CVE_2018_8897_PAGE,
		                 sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE,
		                 MAP_FIXED|MAP_ANON|MAP_PRIVATE, -1, 0);

		/* trigger page fault */
		memset(trap_page, 0, sysconf(_SC_PAGESIZE));

		/* SS selector (descriptor 9 (0x4f >> 3)) */
		*trap_page = 0x4f;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		trigger_cve_2018_8897();

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
	db.dr[0] = CVE_2018_8897_PAGE;
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

#undef CVE_2018_8897_PAGE

/// ----------------------------------------------------------------------------

#define ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64() \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_regs1); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, x86_64_cve_2018_8897);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64()
#endif
