/*	$NetBSD: t_ptrace_i386_wait.h,v 1.2 2017/12/14 22:06:54 christos Exp $	*/

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
#define ATF_TP_ADD_TCS_PTRACE_WAIT_I386() \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, i386_regs1);
#else
#define ATF_TP_ADD_TCS_PTRACE_WAIT_I386()
#endif
