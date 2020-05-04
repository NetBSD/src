/*	$NetBSD: t_ptrace_step_wait.h,v 1.1 2020/05/04 21:33:20 kamil Exp $	*/

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

#if defined(PT_STEP)
static void
ptrace_step(int N, int setstep, bool masked, bool ignored)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	int happy;
	struct sigaction sa;
	struct ptrace_siginfo info;
	sigset_t intmask;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigmask;
	ki_sigset_t kp_sigignore;

#if defined(__arm__)
	/* PT_STEP not supported on arm 32-bit */
	atf_tc_expect_fail("PR kern/52119");
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		if (masked) {
			sigemptyset(&intmask);
			sigaddset(&intmask, SIGTRAP);
			sigprocmask(SIG_BLOCK, &intmask, NULL);
		}

		if (ignored) {
			memset(&sa, 0, sizeof(sa));
			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			FORKEE_ASSERT(sigaction(SIGTRAP, &sa, NULL) != -1);
		}

		happy = check_happy(999);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(happy, check_happy(999));

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	name[0] = CTL_KERN,
	name[1] = KERN_PROC2,
	name[2] = KERN_PROC_PID;
	name[3] = child;
	name[4] = sizeof(kp);
	name[5] = 1;

	FORKEE_ASSERT_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	if (masked)
		kp_sigmask = kp.p_sigmask;

	if (ignored)
		kp_sigignore = kp.p_sigignore;

	while (N --> 0) {
		if (setstep) {
			DPRINTF("Before resuming the child process where it "
			    "left off and without signal to be sent (use "
			    "PT_SETSTEP and PT_CONTINUE)\n");
			SYSCALL_REQUIRE(ptrace(PT_SETSTEP, child, 0, 0) != -1);
			SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0)
			    != -1);
		} else {
			DPRINTF("Before resuming the child process where it "
			    "left off and without signal to be sent (use "
			    "PT_STEP)\n");
			SYSCALL_REQUIRE(ptrace(PT_STEP, child, (void *)1, 0)
			    != -1);
		}

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_stopped(status, SIGTRAP);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

		DPRINTF("Before checking siginfo_t\n");
		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_TRACE);

		if (setstep) {
			SYSCALL_REQUIRE(ptrace(PT_CLEARSTEP, child, 0, 0) != -1);
		}

		ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

		if (masked) {
			DPRINTF("kp_sigmask="
			    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02"
			    PRIx32 "\n",
			    kp_sigmask.__bits[0], kp_sigmask.__bits[1],
			    kp_sigmask.__bits[2], kp_sigmask.__bits[3]);

			DPRINTF("kp.p_sigmask="
			    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02"
			    PRIx32 "\n",
			    kp.p_sigmask.__bits[0], kp.p_sigmask.__bits[1],
			    kp.p_sigmask.__bits[2], kp.p_sigmask.__bits[3]);

			ATF_REQUIRE(!memcmp(&kp_sigmask, &kp.p_sigmask,
			    sizeof(kp_sigmask)));
		}

		if (ignored) {
			DPRINTF("kp_sigignore="
			    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02"
			    PRIx32 "\n",
			    kp_sigignore.__bits[0], kp_sigignore.__bits[1],
			    kp_sigignore.__bits[2], kp_sigignore.__bits[3]);

			DPRINTF("kp.p_sigignore="
			    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02"
			    PRIx32 "\n",
			    kp.p_sigignore.__bits[0], kp.p_sigignore.__bits[1],
			    kp.p_sigignore.__bits[2], kp.p_sigignore.__bits[3]);

			ATF_REQUIRE(!memcmp(&kp_sigignore, &kp.p_sigignore,
			    sizeof(kp_sigignore)));
		}
	}

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define PTRACE_STEP(test, N, setstep)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
        atf_tc_set_md_var(tc, "descr",					\
            "Verify " #N " (PT_SETSTEP set to: " #setstep ")");		\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
        ptrace_step(N, setstep, false, false);				\
}

PTRACE_STEP(step1, 1, 0)
PTRACE_STEP(step2, 2, 0)
PTRACE_STEP(step3, 3, 0)
PTRACE_STEP(step4, 4, 0)
PTRACE_STEP(setstep1, 1, 1)
PTRACE_STEP(setstep2, 2, 1)
PTRACE_STEP(setstep3, 3, 1)
PTRACE_STEP(setstep4, 4, 1)

ATF_TC(step_signalmasked);
ATF_TC_HEAD(step_signalmasked, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify PT_STEP with masked SIGTRAP");
}

ATF_TC_BODY(step_signalmasked, tc)
{

	ptrace_step(1, 0, true, false);
}

ATF_TC(step_signalignored);
ATF_TC_HEAD(step_signalignored, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify PT_STEP with ignored SIGTRAP");
}

ATF_TC_BODY(step_signalignored, tc)
{

	ptrace_step(1, 0, false, true);
}
#endif

#define ATF_TP_ADD_TCS_PTRACE_WAIT_STEP() \
	ATF_TP_ADD_TC_PT_STEP(tp, step1); \
	ATF_TP_ADD_TC_PT_STEP(tp, step2); \
	ATF_TP_ADD_TC_PT_STEP(tp, step3); \
	ATF_TP_ADD_TC_PT_STEP(tp, step4); \
	ATF_TP_ADD_TC_PT_STEP(tp, setstep1); \
	ATF_TP_ADD_TC_PT_STEP(tp, setstep2); \
	ATF_TP_ADD_TC_PT_STEP(tp, setstep3); \
	ATF_TP_ADD_TC_PT_STEP(tp, setstep4); \
	ATF_TP_ADD_TC_PT_STEP(tp, step_signalmasked); \
	ATF_TP_ADD_TC_PT_STEP(tp, step_signalignored);
