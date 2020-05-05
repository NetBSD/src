/*	$NetBSD: t_ptrace_exec_wait.h,v 1.1 2020/05/05 00:23:12 kamil Exp $	*/

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

static void
traceme_vfork_exec(bool masked, bool ignored)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct sigaction sa;
	struct ptrace_siginfo info;
	sigset_t intmask;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigmask;
	ki_sigset_t kp_sigignore;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		if (masked) {
			sigemptyset(&intmask);
			sigaddset(&intmask, sigval);
			sigprocmask(SIG_BLOCK, &intmask, NULL);
		}

		if (ignored) {
			memset(&sa, 0, sizeof(sa));
			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			FORKEE_ASSERT(sigaction(sigval, &sa, NULL) != -1);
		}

		DPRINTF("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "Not reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	name[0] = CTL_KERN,
	name[1] = KERN_PROC2,
	name[2] = KERN_PROC_PID;
	name[3] = getpid();
	name[4] = sizeof(kp);
	name[5] = 1;

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	if (masked)
		kp_sigmask = kp.p_sigmask;

	if (ignored)
		kp_sigignore = kp.p_sigignore;

	name[3] = getpid();

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	if (masked) {
		DPRINTF("kp_sigmask="
		    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
		    kp_sigmask.__bits[0], kp_sigmask.__bits[1],
		    kp_sigmask.__bits[2], kp_sigmask.__bits[3]);

	        DPRINTF("kp.p_sigmask="
	            "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
	            kp.p_sigmask.__bits[0], kp.p_sigmask.__bits[1],
	            kp.p_sigmask.__bits[2], kp.p_sigmask.__bits[3]);

		ATF_REQUIRE(!memcmp(&kp_sigmask, &kp.p_sigmask,
		    sizeof(kp_sigmask)));
	}

	if (ignored) {
		DPRINTF("kp_sigignore="
		    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
		    kp_sigignore.__bits[0], kp_sigignore.__bits[1],
		    kp_sigignore.__bits[2], kp_sigignore.__bits[3]);

	        DPRINTF("kp.p_sigignore="
	            "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
	            kp.p_sigignore.__bits[0], kp.p_sigignore.__bits[1],
	            kp.p_sigignore.__bits[2], kp.p_sigignore.__bits[3]);

		ATF_REQUIRE(!memcmp(&kp_sigignore, &kp.p_sigignore,
		    sizeof(kp_sigignore)));
	}

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_EXEC(test, masked, ignored)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify PT_TRACE_ME followed by exec(3) in a vfork(2)ed "	\
	    "child%s%s", masked ? " with masked signal" : "",		\
	    masked ? " with ignored signal" : "");			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_vfork_exec(masked, ignored);				\
}

TRACEME_VFORK_EXEC(traceme_vfork_exec, false, false)
TRACEME_VFORK_EXEC(traceme_vfork_signalmasked_exec, true, false)
TRACEME_VFORK_EXEC(traceme_vfork_signalignored_exec, false, true)

/// ----------------------------------------------------------------------------

static void
traceme_exec(bool masked, bool ignored)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct sigaction sa;
	struct ptrace_siginfo info;
	sigset_t intmask;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigmask;
	ki_sigset_t kp_sigignore;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		if (masked) {
			sigemptyset(&intmask);
			sigaddset(&intmask, sigval);
			sigprocmask(SIG_BLOCK, &intmask, NULL);
		}

		if (ignored) {
			memset(&sa, 0, sizeof(sa));
			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			FORKEE_ASSERT(sigaction(sigval, &sa, NULL) != -1);
		}

		DPRINTF("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	name[0] = CTL_KERN,
	name[1] = KERN_PROC2,
	name[2] = KERN_PROC_PID;
	name[3] = getpid();
	name[4] = sizeof(kp);
	name[5] = 1;

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	if (masked)
		kp_sigmask = kp.p_sigmask;

	if (ignored)
		kp_sigignore = kp.p_sigignore;

	name[3] = getpid();

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	if (masked) {
		DPRINTF("kp_sigmask="
		    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
		    kp_sigmask.__bits[0], kp_sigmask.__bits[1],
		    kp_sigmask.__bits[2], kp_sigmask.__bits[3]);

		DPRINTF("kp.p_sigmask="
		    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
		    kp.p_sigmask.__bits[0], kp.p_sigmask.__bits[1],
		    kp.p_sigmask.__bits[2], kp.p_sigmask.__bits[3]);

		ATF_REQUIRE(!memcmp(&kp_sigmask, &kp.p_sigmask,
		    sizeof(kp_sigmask)));
	}

	if (ignored) {
		DPRINTF("kp_sigignore="
		    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
		    kp_sigignore.__bits[0], kp_sigignore.__bits[1],
		    kp_sigignore.__bits[2], kp_sigignore.__bits[3]);

		DPRINTF("kp.p_sigignore="
		    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
		    kp.p_sigignore.__bits[0], kp.p_sigignore.__bits[1],
		    kp.p_sigignore.__bits[2], kp.p_sigignore.__bits[3]);

		ATF_REQUIRE(!memcmp(&kp_sigignore, &kp.p_sigignore,
		    sizeof(kp_sigignore)));
	}

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_EXEC(test, masked, ignored)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
       atf_tc_set_md_var(tc, "descr",					\
           "Detect SIGTRAP TRAP_EXEC from "				\
           "child%s%s", masked ? " with masked signal" : "",		\
           masked ? " with ignored signal" : "");			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
       traceme_exec(masked, ignored);					\
}

TRACEME_EXEC(traceme_exec, false, false)
TRACEME_EXEC(traceme_signalmasked_exec, true, false)
TRACEME_EXEC(traceme_signalignored_exec, false, true)

#define ATF_TP_ADD_TCS_PTRACE_WAIT_EXEC() \
	ATF_TP_ADD_TC(tp, traceme_vfork_exec); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_exec); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_exec); \
	ATF_TP_ADD_TC(tp, traceme_exec); \
	ATF_TP_ADD_TC(tp, traceme_signalmasked_exec); \
	ATF_TP_ADD_TC(tp, traceme_signalignored_exec);
