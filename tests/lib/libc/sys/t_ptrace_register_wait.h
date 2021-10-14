/*      $NetBSD: t_ptrace_register_wait.h,v 1.4 2021/10/14 13:50:36 gson Exp $   */

/*-
 * Copyright (c) 2016, 2017, 2018, 2019, 2020 The NetBSD Foundation, Inc.
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


#if defined(HAVE_GPREGS) || defined(HAVE_FPREGS)
static void
access_regs(const char *regset, const char *aux)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
#if defined(HAVE_GPREGS)
	struct reg gpr;
	register_t rgstr;
#endif
#if defined(HAVE_FPREGS)
	struct fpreg fpr;
#endif

#if !defined(HAVE_GPREGS)
	if (strcmp(regset, "regs") == 0)
		atf_tc_fail("Impossible test scenario!");
#endif

#if !defined(HAVE_FPREGS)
	if (strcmp(regset, "fpregs") == 0)
		atf_tc_fail("Impossible test scenario!");
#endif

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

#if defined(HAVE_GPREGS)
	if (strcmp(regset, "regs") == 0) {
		DPRINTF("Call GETREGS for the child process\n");
		SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &gpr, 0) != -1);

		if (strcmp(aux, "none") == 0) {
			DPRINTF("Retrieved registers\n");
		} else if (strcmp(aux, "pc") == 0) {
			rgstr = PTRACE_REG_PC(&gpr);
			DPRINTF("Retrieved %" PRIxREGISTER "\n", rgstr);
		} else if (strstr(aux, "set_pc") != NULL) {
			rgstr = PTRACE_REG_PC(&gpr);
			DPRINTF("Retrieved PC %" PRIxREGISTER "\n", rgstr);
			if (strstr(aux, "0x1") != NULL) {
				rgstr |= 0x1;
			} else if (strstr(aux, "0x3") != NULL) {
				rgstr |= 0x3;
			} else if (strstr(aux, "0x7") != NULL) {
				rgstr |= 0x7;
			}
			DPRINTF("Set PC %" PRIxREGISTER "\n", rgstr);
			PTRACE_REG_SET_PC(&gpr, rgstr);
			if (strcmp(aux, "set_pc") != 0) {
				/* This call can fail with EINVAL or similar. */
				ptrace(PT_SETREGS, child, &gpr, 0);
			}
		} else if (strcmp(aux, "sp") == 0) {
			rgstr = PTRACE_REG_SP(&gpr);
			DPRINTF("Retrieved %" PRIxREGISTER "\n", rgstr);
		} else if (strcmp(aux, "intrv") == 0) {
			rgstr = PTRACE_REG_INTRV(&gpr);
			DPRINTF("Retrieved %" PRIxREGISTER "\n", rgstr);
		} else if (strcmp(aux, "setregs") == 0) {
			DPRINTF("Call SETREGS for the child process\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_SETREGS, child, &gpr, 0) != -1);
		}
	}
#endif

#if defined(HAVE_FPREGS)
	if (strcmp(regset, "fpregs") == 0) {
		DPRINTF("Call GETFPREGS for the child process\n");
		SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &fpr, 0) != -1);

		if (strcmp(aux, "getfpregs") == 0) {
			DPRINTF("Retrieved FP registers\n");
		} else if (strcmp(aux, "setfpregs") == 0) {
			DPRINTF("Call SETFPREGS for the child\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_SETFPREGS, child, &fpr, 0) != -1);
		}
	}
#endif

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	if (strstr(aux, "unaligned") != NULL) {
		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");

		ptrace(PT_KILL, child, NULL, 0);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD,
		    wpid = TWAIT_GENERIC(child, &status, 0));
	} else {
		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_exited(status, exitval);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD,
		    wpid = TWAIT_GENERIC(child, &status, 0));
	}
}

#define ACCESS_REGS(test, regset, aux)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
        atf_tc_set_md_var(tc, "descr",					\
            "Verify " regset " with auxiliary operation: " aux);	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
        access_regs(regset, aux);					\
}
#endif

#if defined(HAVE_GPREGS)
ACCESS_REGS(access_regs1, "regs", "none")
ACCESS_REGS(access_regs2, "regs", "pc")
ACCESS_REGS(access_regs3, "regs", "set_pc")
ACCESS_REGS(access_regs4, "regs", "sp")
ACCESS_REGS(access_regs5, "regs", "intrv")
ACCESS_REGS(access_regs6, "regs", "setregs")
ACCESS_REGS(access_regs_set_unaligned_pc_0x1, "regs", "set_pc+unaligned+0x1")
ACCESS_REGS(access_regs_set_unaligned_pc_0x3, "regs", "set_pc+unaligned+0x3")
ACCESS_REGS(access_regs_set_unaligned_pc_0x7, "regs", "set_pc+unaligned+0x7")
#endif
#if defined(HAVE_FPREGS)
ACCESS_REGS(access_fpregs1, "fpregs", "getfpregs")
ACCESS_REGS(access_fpregs2, "fpregs", "setfpregs")
#endif

#define ATF_TP_ADD_TCS_PTRACE_WAIT_REGISTER() \
        ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs1); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs2); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs3); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs4); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs5); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs6); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs_set_unaligned_pc_0x1); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs_set_unaligned_pc_0x3); \
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs_set_unaligned_pc_0x7); \
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, access_fpregs1); \
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, access_fpregs2);
