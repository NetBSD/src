/*	$NetBSD: t_ptrace_signal_wait.h,v 1.5 2021/03/19 00:44:09 simonb Exp $	*/

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
traceme_raise(int sigval)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	ptrace_state_t state, zero_state;
	const int slen = sizeof(state);
	struct ptrace_siginfo info;
	memset(&zero_state, 0, sizeof(zero_state));
	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		switch (sigval) {
		case SIGKILL:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
			__unreachable();
		default:
			DPRINTF("Before exiting of the child process\n");
			_exit(exitval);
		}
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	switch (sigval) {
	case SIGKILL:
		validate_status_signaled(status, sigval, 0);
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) == -1);

		break;
	default:
		validate_status_stopped(status, sigval);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
			"child\n");
		SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info,
			sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
			"si_errno=%#x\n",
			info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
			info.psi_siginfo.si_errno);

		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

		DPRINTF("Assert that PT_GET_PROCESS_STATE returns non-error\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		ATF_REQUIRE(memcmp(&state, &zero_state, slen) == 0);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);
		break;
	}

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_RAISE(test, sig)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify " #sig " followed by _exit(2) in a child");		\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_raise(sig);						\
}

TRACEME_RAISE(traceme_raise1, SIGKILL) /* non-maskable */
TRACEME_RAISE(traceme_raise2, SIGSTOP) /* non-maskable */
TRACEME_RAISE(traceme_raise3, SIGABRT) /* regular abort trap */
TRACEME_RAISE(traceme_raise4, SIGHUP)  /* hangup */
TRACEME_RAISE(traceme_raise5, SIGCONT) /* continued? */
TRACEME_RAISE(traceme_raise6, SIGTRAP) /* crash signal */
TRACEME_RAISE(traceme_raise7, SIGBUS) /* crash signal */
TRACEME_RAISE(traceme_raise8, SIGILL) /* crash signal */
TRACEME_RAISE(traceme_raise9, SIGFPE) /* crash signal */
TRACEME_RAISE(traceme_raise10, SIGSEGV) /* crash signal */

/// ----------------------------------------------------------------------------

static void
traceme_raisesignal_ignored(int sigignored)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	struct sigaction sa;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);
		FORKEE_ASSERT(sigaction(sigignored, &sa, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before raising %s from child\n",
		    strsignal(sigignored));
		FORKEE_ASSERT(raise(sigignored) == 0);

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

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigignored);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigignored);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_RAISESIGNAL_IGNORED(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that ignoring (with SIG_IGN) " #sig " in tracee "	\
	    "does not stop tracer from catching this raised signal");	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_raisesignal_ignored(sig);				\
}

// A signal handler for SIGKILL and SIGSTOP cannot be ignored.
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored1, SIGABRT) /* abort */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored2, SIGHUP)  /* hangup */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored3, SIGCONT) /* cont. */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored4, SIGTRAP) /* crash */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored5, SIGBUS) /* crash */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored6, SIGILL) /* crash */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored7, SIGFPE) /* crash */
TRACEME_RAISESIGNAL_IGNORED(traceme_raisesignal_ignored8, SIGSEGV) /* crash */

/// ----------------------------------------------------------------------------

static void
traceme_raisesignal_masked(int sigmasked)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sigemptyset(&intmask);
		sigaddset(&intmask, sigmasked);
		sigprocmask(SIG_BLOCK, &intmask, NULL);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before raising %s breakpoint from child\n",
		    strsignal(sigmasked));
		FORKEE_ASSERT(raise(sigmasked) == 0);

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

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_RAISESIGNAL_MASKED(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that masking (with SIG_BLOCK) " #sig " in tracee "	\
	    "stops tracer from catching this raised signal");		\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_raisesignal_masked(sig);				\
}

// A signal handler for SIGKILL and SIGSTOP cannot be masked.
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked1, SIGABRT) /* abort trap */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked2, SIGHUP)  /* hangup */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked3, SIGCONT) /* continued? */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked4, SIGTRAP) /* crash sig. */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked5, SIGBUS) /* crash sig. */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked6, SIGILL) /* crash sig. */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked7, SIGFPE) /* crash sig. */
TRACEME_RAISESIGNAL_MASKED(traceme_raisesignal_masked8, SIGSEGV) /* crash sig. */

/// ----------------------------------------------------------------------------

static void
traceme_crash(int sig)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sig);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sig);
	switch (sig) {
	case SIGTRAP:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_BRKPT);
		break;
	case SIGSEGV:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SEGV_MAPERR);
		break;
	case SIGILL:
		ATF_REQUIRE(info.psi_siginfo.si_code >= ILL_ILLOPC &&
		            info.psi_siginfo.si_code <= ILL_BADSTK);
		break;
	case SIGFPE:
// XXXQEMU	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, FPE_FLTDIV);
		break;
	case SIGBUS:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, BUS_ADRERR);
		break;
	}

	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_CRASH(test, sig)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify crash signal " #sig " in a child after PT_TRACE_ME"); \
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_crash(sig);						\
}

TRACEME_CRASH(traceme_crash_trap, SIGTRAP)
TRACEME_CRASH(traceme_crash_segv, SIGSEGV)
TRACEME_CRASH(traceme_crash_ill, SIGILL)
TRACEME_CRASH(traceme_crash_fpe, SIGFPE)
TRACEME_CRASH(traceme_crash_bus, SIGBUS)

/// ----------------------------------------------------------------------------

static void
traceme_signalmasked_crash(int sig)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;
	sigset_t intmask;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigmask;

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sigemptyset(&intmask);
		sigaddset(&intmask, sig);
		sigprocmask(SIG_BLOCK, &intmask, NULL);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	name[0] = CTL_KERN,
	name[1] = KERN_PROC2,
	name[2] = KERN_PROC_PID;
	name[3] = child;
	name[4] = sizeof(kp);
	name[5] = 1;

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	kp_sigmask = kp.p_sigmask;

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sig);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	DPRINTF("kp_sigmask="
	    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
	    kp_sigmask.__bits[0], kp_sigmask.__bits[1], kp_sigmask.__bits[2],
	    kp_sigmask.__bits[3]);

	DPRINTF("kp.p_sigmask="
	    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
	    kp.p_sigmask.__bits[0], kp.p_sigmask.__bits[1],
	    kp.p_sigmask.__bits[2], kp.p_sigmask.__bits[3]);

	ATF_REQUIRE(!memcmp(&kp_sigmask, &kp.p_sigmask, sizeof(kp_sigmask)));

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sig);
	switch (sig) {
	case SIGTRAP:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_BRKPT);
		break;
	case SIGSEGV:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SEGV_MAPERR);
		break;
	case SIGILL:
		ATF_REQUIRE(info.psi_siginfo.si_code >= ILL_ILLOPC &&
		            info.psi_siginfo.si_code <= ILL_BADSTK);
		break;
	case SIGFPE:
// XXXQEMU	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, FPE_FLTDIV);
		break;
	case SIGBUS:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, BUS_ADRERR);
		break;
	}

	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_SIGNALMASKED_CRASH(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify masked crash signal " #sig " in a child after "	\
	    "PT_TRACE_ME is delivered to its tracer");			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_signalmasked_crash(sig);				\
}

TRACEME_SIGNALMASKED_CRASH(traceme_signalmasked_crash_trap, SIGTRAP)
TRACEME_SIGNALMASKED_CRASH(traceme_signalmasked_crash_segv, SIGSEGV)
TRACEME_SIGNALMASKED_CRASH(traceme_signalmasked_crash_ill, SIGILL)
TRACEME_SIGNALMASKED_CRASH(traceme_signalmasked_crash_fpe, SIGFPE)
TRACEME_SIGNALMASKED_CRASH(traceme_signalmasked_crash_bus, SIGBUS)

/// ----------------------------------------------------------------------------

static void
traceme_signalignored_crash(int sig)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct sigaction sa;
	struct ptrace_siginfo info;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigignore;

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sig, &sa, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	name[0] = CTL_KERN,
	name[1] = KERN_PROC2,
	name[2] = KERN_PROC_PID;
	name[3] = child;
	name[4] = sizeof(kp);
	name[5] = 1;

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	kp_sigignore = kp.p_sigignore;

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sig);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

	DPRINTF("kp_sigignore="
	    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
	    kp_sigignore.__bits[0], kp_sigignore.__bits[1],
	    kp_sigignore.__bits[2], kp_sigignore.__bits[3]);

	DPRINTF("kp.p_sigignore="
	    "%#02" PRIx32 "%02" PRIx32 "%02" PRIx32 "%02" PRIx32"\n",
	    kp.p_sigignore.__bits[0], kp.p_sigignore.__bits[1],
	    kp.p_sigignore.__bits[2], kp.p_sigignore.__bits[3]);

	ATF_REQUIRE(!memcmp(&kp_sigignore, &kp.p_sigignore, sizeof(kp_sigignore)));

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sig);
	switch (sig) {
	case SIGTRAP:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_BRKPT);
		break;
	case SIGSEGV:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SEGV_MAPERR);
		break;
	case SIGILL:
		ATF_REQUIRE(info.psi_siginfo.si_code >= ILL_ILLOPC &&
		            info.psi_siginfo.si_code <= ILL_BADSTK);
		break;
	case SIGFPE:
// XXXQEMU	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, FPE_FLTDIV);
		break;
	case SIGBUS:
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, BUS_ADRERR);
		break;
	}

	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_SIGNALIGNORED_CRASH(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify ignored crash signal " #sig " in a child after "	\
	    "PT_TRACE_ME is delivered to its tracer"); 			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_signalignored_crash(sig);				\
}

TRACEME_SIGNALIGNORED_CRASH(traceme_signalignored_crash_trap, SIGTRAP)
TRACEME_SIGNALIGNORED_CRASH(traceme_signalignored_crash_segv, SIGSEGV)
TRACEME_SIGNALIGNORED_CRASH(traceme_signalignored_crash_ill, SIGILL)
TRACEME_SIGNALIGNORED_CRASH(traceme_signalignored_crash_fpe, SIGFPE)
TRACEME_SIGNALIGNORED_CRASH(traceme_signalignored_crash_bus, SIGBUS)

/// ----------------------------------------------------------------------------

static void
traceme_sendsignal_handle(int sigsent, void (*sah)(int a), int *traceme_caught)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	struct sigaction sa;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sa.sa_handler = sah;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sigsent, &sa, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(*traceme_caught, 1);

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

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", strsignal(sigsent));
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_SENDSIGNAL_HANDLE(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that a signal " #sig " emitted by a tracer to a child is " \
	    "handled correctly and caught by a signal handler");	\
}									\
									\
static int test##_caught = 0;						\
									\
static void								\
test##_sighandler(int arg)						\
{									\
	FORKEE_ASSERT_EQ(arg, sig);					\
									\
	++ test##_caught;						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_sendsignal_handle(sig, test##_sighandler, & test##_caught); \
}

// A signal handler for SIGKILL and SIGSTOP cannot be registered.
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle1, SIGABRT) /* abort trap */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle2, SIGHUP)  /* hangup */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle3, SIGCONT) /* continued? */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle4, SIGTRAP) /* crash sig. */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle5, SIGBUS) /* crash sig. */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle6, SIGILL) /* crash sig. */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle7, SIGFPE) /* crash sig. */
TRACEME_SENDSIGNAL_HANDLE(traceme_sendsignal_handle8, SIGSEGV) /* crash sig. */

/// ----------------------------------------------------------------------------

static void
traceme_sendsignal_masked(int sigsent)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	sigset_t set;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sigemptyset(&set);
		sigaddset(&set, sigsent);
		FORKEE_ASSERT(sigprocmask(SIG_BLOCK, &set, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", strsignal(sigsent));
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_SENDSIGNAL_MASKED(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that a signal " #sig " emitted by a tracer to a child is " \
	    "handled correctly and the signal is masked by SIG_BLOCK");	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_sendsignal_masked(sig);					\
}

// A signal handler for SIGKILL and SIGSTOP cannot be masked.
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked1, SIGABRT) /* abort trap */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked2, SIGHUP)  /* hangup */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked3, SIGCONT) /* continued? */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked4, SIGTRAP) /* crash sig. */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked5, SIGBUS) /* crash sig. */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked6, SIGILL) /* crash sig. */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked7, SIGFPE) /* crash sig. */
TRACEME_SENDSIGNAL_MASKED(traceme_sendsignal_masked8, SIGSEGV) /* crash sig. */

/// ----------------------------------------------------------------------------

static void
traceme_sendsignal_ignored(int sigsent)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	struct sigaction sa;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);
		FORKEE_ASSERT(sigaction(sigsent, &sa, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", strsignal(sigsent));
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_SENDSIGNAL_IGNORED(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that a signal " #sig " emitted by a tracer to a child is " \
	    "handled correctly and the signal is masked by SIG_IGN");	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_sendsignal_ignored(sig);				\
}

// A signal handler for SIGKILL and SIGSTOP cannot be ignored.
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored1, SIGABRT) /* abort */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored2, SIGHUP)  /* hangup */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored3, SIGCONT) /* continued */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored4, SIGTRAP) /* crash s. */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored5, SIGBUS) /* crash s. */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored6, SIGILL) /* crash s. */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored7, SIGFPE) /* crash s. */
TRACEME_SENDSIGNAL_IGNORED(traceme_sendsignal_ignored8, SIGSEGV) /* crash s. */

/// ----------------------------------------------------------------------------

static void
traceme_sendsignal_simple(int sigsent)
{
	const int sigval = SIGSTOP;
	int exitval = 0;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
	int expect_core;

	switch (sigsent) {
	case SIGABRT:
	case SIGTRAP:
	case SIGBUS:
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
		expect_core = 1;
		break;
	default:
		expect_core = 0;
		break;
	}
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		switch (sigsent) {
		case SIGCONT:
		case SIGSTOP:
			_exit(exitval);
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and with "
	    "signal %s to be sent\n", strsignal(sigsent));
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	switch (sigsent) {
	case SIGSTOP:
		validate_status_stopped(status, sigsent);
		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "child\n");
		SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info,
		    sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n",
		    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		    info.psi_siginfo.si_errno);

		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

		DPRINTF("Before resuming the child process where it left off "
		    "and with signal %s to be sent\n", strsignal(sigsent));
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);
		/* FALLTHROUGH */
	case SIGCONT:
		validate_status_exited(status, exitval);
		break;
	default:
		validate_status_signaled(status, sigsent, expect_core);
		break;
	}

	DPRINTF("Before calling %s() for the exited child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_SENDSIGNAL_SIMPLE(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that a signal " #sig " emitted by a tracer to a child is " \
	    "handled correctly in a child without a signal handler");	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_sendsignal_simple(sig);					\
}

TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple1, SIGKILL) /* non-maskable*/
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple2, SIGSTOP) /* non-maskable*/
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple3, SIGABRT) /* abort trap */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple4, SIGHUP)  /* hangup */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple5, SIGCONT) /* continued? */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple6, SIGTRAP) /* crash sig. */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple7, SIGBUS) /* crash sig. */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple8, SIGILL) /* crash sig. */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple9, SIGFPE) /* crash sig. */
TRACEME_SENDSIGNAL_SIMPLE(traceme_sendsignal_simple10, SIGSEGV) /* crash sig. */

/// ----------------------------------------------------------------------------

static void
traceme_vfork_raise(int sigval)
{
	const int exitval = 5, exitval_watcher = 10;
	pid_t child, parent, watcher, wpid;
	int rv;
#if defined(TWAIT_HAVE_STATUS)
	int status;

	/* volatile workarounds GCC -Werror=clobbered */
	volatile int expect_core;

	switch (sigval) {
	case SIGABRT:
	case SIGTRAP:
	case SIGBUS:
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
		expect_core = 1;
		break;
	default:
		expect_core = 0;
		break;
	}
#endif

	/*
	 * Spawn a dedicated thread to watch for a stopped child and emit
	 * the SIGKILL signal to it.
	 *
	 * vfork(2) might clobber watcher, this means that it's safer and
	 * simpler to reparent this process to initproc and forget about it.
	 */
	if (sigval == SIGSTOP) {
		parent = getpid();

		watcher = fork();
		ATF_REQUIRE(watcher != 1);
		if (watcher == 0) {
			/* Double fork(2) trick to reparent to initproc */
			watcher = fork();
			FORKEE_ASSERT_NEQ(watcher, -1);
			if (watcher != 0)
				_exit(exitval_watcher);

			child = await_stopped_child(parent);

			errno = 0;
			rv = kill(child, SIGKILL);
			FORKEE_ASSERT_EQ(rv, 0);
			FORKEE_ASSERT_EQ(errno, 0);

			/* This exit value will be collected by initproc */
			_exit(0);
		}
		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(watcher, &status, 0),
		    watcher);

		validate_status_exited(status, exitval_watcher);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD,
		    wpid = TWAIT_GENERIC(watcher, &status, 0));
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		switch (sigval) {
		case SIGSTOP:
		case SIGKILL:
		case SIGABRT:
		case SIGHUP:
		case SIGTRAP:
		case SIGBUS:
		case SIGILL:
		case SIGFPE:
		case SIGSEGV:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
			__unreachable();
		default:
			DPRINTF("Before exiting of the child process\n");
			_exit(exitval);
		}
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	switch (sigval) {
	case SIGKILL:
	case SIGABRT:
	case SIGHUP:
	case SIGTRAP:
	case SIGBUS:
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
		validate_status_signaled(status, sigval, expect_core);
		break;
	case SIGSTOP:
		validate_status_signaled(status, SIGKILL, 0);
		break;
	case SIGCONT:
	case SIGTSTP:
	case SIGTTIN:
	case SIGTTOU:
		validate_status_exited(status, exitval);
		break;
	default:
		/* NOTREACHED */
		ATF_REQUIRE(0 && "NOT IMPLEMENTED");
		break;
	}

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_RAISE(test, sig)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify PT_TRACE_ME followed by raise of " #sig " in a "	\
	    "vfork(2)ed child");					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_vfork_raise(sig);					\
}

TRACEME_VFORK_RAISE(traceme_vfork_raise1, SIGKILL) /* non-maskable */
TRACEME_VFORK_RAISE(traceme_vfork_raise2, SIGSTOP) /* non-maskable */
TRACEME_VFORK_RAISE(traceme_vfork_raise3, SIGTSTP) /* ignored in vfork(2) */
TRACEME_VFORK_RAISE(traceme_vfork_raise4, SIGTTIN) /* ignored in vfork(2) */
TRACEME_VFORK_RAISE(traceme_vfork_raise5, SIGTTOU) /* ignored in vfork(2) */
TRACEME_VFORK_RAISE(traceme_vfork_raise6, SIGABRT) /* regular abort trap */
TRACEME_VFORK_RAISE(traceme_vfork_raise7, SIGHUP)  /* hangup */
TRACEME_VFORK_RAISE(traceme_vfork_raise8, SIGCONT) /* continued? */
TRACEME_VFORK_RAISE(traceme_vfork_raise9, SIGTRAP) /* crash signal */
TRACEME_VFORK_RAISE(traceme_vfork_raise10, SIGBUS) /* crash signal */
TRACEME_VFORK_RAISE(traceme_vfork_raise11, SIGILL) /* crash signal */
TRACEME_VFORK_RAISE(traceme_vfork_raise12, SIGFPE) /* crash signal */
TRACEME_VFORK_RAISE(traceme_vfork_raise13, SIGSEGV) /* crash signal */

/// ----------------------------------------------------------------------------

static void
traceme_vfork_crash(int sig)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, sig, 1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_CRASH(test, sig)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify PT_TRACE_ME followed by a crash signal " #sig " in a " \
	    "vfork(2)ed child");					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_vfork_crash(sig);					\
}

TRACEME_VFORK_CRASH(traceme_vfork_crash_trap, SIGTRAP)
TRACEME_VFORK_CRASH(traceme_vfork_crash_segv, SIGSEGV)
TRACEME_VFORK_CRASH(traceme_vfork_crash_ill, SIGILL)
TRACEME_VFORK_CRASH(traceme_vfork_crash_fpe, SIGFPE)
TRACEME_VFORK_CRASH(traceme_vfork_crash_bus, SIGBUS)

/// ----------------------------------------------------------------------------

static void
traceme_vfork_signalmasked_crash(int sig)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sigemptyset(&intmask);
		sigaddset(&intmask, sig);
		sigprocmask(SIG_BLOCK, &intmask, NULL);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, sig, 1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_SIGNALMASKED_CRASH(test, sig)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify PT_TRACE_ME followed by a crash signal " #sig " in a " \
	    "vfork(2)ed child with a masked signal");			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_vfork_signalmasked_crash(sig);				\
}

TRACEME_VFORK_SIGNALMASKED_CRASH(traceme_vfork_signalmasked_crash_trap, SIGTRAP)
TRACEME_VFORK_SIGNALMASKED_CRASH(traceme_vfork_signalmasked_crash_segv, SIGSEGV)
TRACEME_VFORK_SIGNALMASKED_CRASH(traceme_vfork_signalmasked_crash_ill, SIGILL)
TRACEME_VFORK_SIGNALMASKED_CRASH(traceme_vfork_signalmasked_crash_fpe, SIGFPE)
TRACEME_VFORK_SIGNALMASKED_CRASH(traceme_vfork_signalmasked_crash_bus, SIGBUS)

/// ----------------------------------------------------------------------------

static void
traceme_vfork_signalignored_crash(int sig)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct sigaction sa;

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sig, &sa, NULL) != -1);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, sig, 1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_SIGNALIGNORED_CRASH(test, sig)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify PT_TRACE_ME followed by a crash signal " #sig " in a " \
	    "vfork(2)ed child with ignored signal");			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_vfork_signalignored_crash(sig);				\
}

TRACEME_VFORK_SIGNALIGNORED_CRASH(traceme_vfork_signalignored_crash_trap,
    SIGTRAP)
TRACEME_VFORK_SIGNALIGNORED_CRASH(traceme_vfork_signalignored_crash_segv,
    SIGSEGV)
TRACEME_VFORK_SIGNALIGNORED_CRASH(traceme_vfork_signalignored_crash_ill,
    SIGILL)
TRACEME_VFORK_SIGNALIGNORED_CRASH(traceme_vfork_signalignored_crash_fpe,
    SIGFPE)
TRACEME_VFORK_SIGNALIGNORED_CRASH(traceme_vfork_signalignored_crash_bus,
    SIGBUS)

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
unrelated_tracer_sees_crash(int sig, bool masked, bool ignored)
{
	const int sigval = SIGSTOP;
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval = 10;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
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

#ifndef PTRACE_ILLEGAL_ASM
	if (sig == SIGILL)
		atf_tc_skip("PTRACE_ILLEGAL_ASM not defined");
#endif

	if (sig == SIGFPE && !are_fpu_exceptions_supported())
		atf_tc_skip("FP exceptions are not supported");

	memset(&info, 0, sizeof(info));

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		// Wait for parent to let us crash
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);

		if (masked) {
			sigemptyset(&intmask);
			sigaddset(&intmask, sig);
			sigprocmask(SIG_BLOCK, &intmask, NULL);
		}

		if (ignored) {
			memset(&sa, 0, sizeof(sa));
			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			FORKEE_ASSERT(sigaction(sig, &sa, NULL) != -1);
		}

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before executing a trap\n");
		switch (sig) {
		case SIGTRAP:
			trigger_trap();
			break;
		case SIGSEGV:
			trigger_segv();
			break;
		case SIGILL:
			trigger_ill();
			break;
		case SIGFPE:
			trigger_fpe();
			break;
		case SIGBUS:
			trigger_bus();
			break;
		default:
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
		}

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}

	DPRINTF("Spawn debugger\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* Fork again and drop parent to reattach to PID 1 */
		tracer = atf_utils_fork();
		if (tracer != 0)
			_exit(exitval);

		DPRINTF("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for the "
		    "traced process\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_SIGINFO, tracee, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n", info.psi_siginfo.si_signo,
		    info.psi_siginfo.si_code, info.psi_siginfo.si_errno);

		FORKEE_ASSERT_EQ(info.psi_siginfo.si_signo, SIGSTOP);
		FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, SI_USER);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		CHILD_TO_PARENT("tracer ready", parent_tracer, msg);

		/* Wait for parent to tell use that tracee should have exited */
		CHILD_FROM_PARENT("wait for tracee exit", parent_tracer, msg);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, sigval);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for the "
		    "traced process\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_SIGINFO, tracee, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n", info.psi_siginfo.si_signo,
		    info.psi_siginfo.si_code, info.psi_siginfo.si_errno);

		FORKEE_ASSERT_EQ(info.psi_siginfo.si_signo, sigval);
		FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, SI_LWP);

		name[0] = CTL_KERN,
		name[1] = KERN_PROC2,
		name[2] = KERN_PROC_PID;
		name[3] = tracee;
		name[4] = sizeof(kp);
		name[5] = 1;

		FORKEE_ASSERT_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

		if (masked)
			kp_sigmask = kp.p_sigmask;

		if (ignored)
			kp_sigignore = kp.p_sigignore;

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, sig);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for the "
		    "traced process\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_SIGINFO, tracee, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n", info.psi_siginfo.si_signo,
		    info.psi_siginfo.si_code, info.psi_siginfo.si_errno);

		FORKEE_ASSERT_EQ(info.psi_siginfo.si_signo, sig);

		FORKEE_ASSERT_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

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

			FORKEE_ASSERTX(!memcmp(&kp_sigmask, &kp.p_sigmask,
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

			FORKEE_ASSERTX(!memcmp(&kp_sigignore, &kp.p_sigignore,
			    sizeof(kp_sigignore)));
		}

		switch (sig) {
		case SIGTRAP:
			FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, TRAP_BRKPT);
			break;
		case SIGSEGV:
			FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, SEGV_MAPERR);
			break;
		case SIGILL:
			FORKEE_ASSERT(info.psi_siginfo.si_code >= ILL_ILLOPC &&
			            info.psi_siginfo.si_code <= ILL_BADSTK);
			break;
		case SIGFPE:
// XXXQEMU		FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, FPE_FLTDIV);
			break;
		case SIGBUS:
			FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, BUS_ADRERR);
			break;
		}

		FORKEE_ASSERT(ptrace(PT_KILL, tracee, NULL, 0) != -1);
		DPRINTF("Before calling %s() for the tracee\n", TWAIT_FNAME);
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_signaled(status, SIGKILL, 0);

		/* Inform parent that tracer is exiting normally */
		CHILD_TO_PARENT("tracer done", parent_tracer, msg);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(0 /* collect by initproc */);
	}

	DPRINTF("Wait for the tracer process (direct child) to exit "
	    "calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracer, &status, 0), tracer);

	validate_status_exited(status, exitval);

	DPRINTF("Wait for the non-exited tracee process with %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, NULL, WNOHANG), 0);

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("tracer ready", parent_tracer, msg);

	DPRINTF("Resume the tracee and let it crash\n");
	PARENT_TO_CHILD("exit tracee", parent_tracee,  msg);

	DPRINTF("Resume the tracer and let it detect crashed tracee\n");
	PARENT_TO_CHILD("Message 2", parent_tracer, msg);

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Await normal exit of tracer\n");
	PARENT_FROM_CHILD("tracer done", parent_tracer, msg);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}

#define UNRELATED_TRACER_SEES_CRASH(test, sig)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Assert that an unrelated tracer sees crash signal from "	\
	    "the debuggee");						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	unrelated_tracer_sees_crash(sig, false, false);			\
}

UNRELATED_TRACER_SEES_CRASH(unrelated_tracer_sees_crash_trap, SIGTRAP)
UNRELATED_TRACER_SEES_CRASH(unrelated_tracer_sees_crash_segv, SIGSEGV)
UNRELATED_TRACER_SEES_CRASH(unrelated_tracer_sees_crash_ill, SIGILL)
UNRELATED_TRACER_SEES_CRASH(unrelated_tracer_sees_crash_fpe, SIGFPE)
UNRELATED_TRACER_SEES_CRASH(unrelated_tracer_sees_crash_bus, SIGBUS)

#define UNRELATED_TRACER_SEES_SIGNALMASKED_CRASH(test, sig)		\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Assert that an unrelated tracer sees crash signal from "	\
	    "the debuggee with masked signal");				\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	unrelated_tracer_sees_crash(sig, true, false);			\
}

UNRELATED_TRACER_SEES_SIGNALMASKED_CRASH(
    unrelated_tracer_sees_signalmasked_crash_trap, SIGTRAP)
UNRELATED_TRACER_SEES_SIGNALMASKED_CRASH(
    unrelated_tracer_sees_signalmasked_crash_segv, SIGSEGV)
UNRELATED_TRACER_SEES_SIGNALMASKED_CRASH(
    unrelated_tracer_sees_signalmasked_crash_ill, SIGILL)
UNRELATED_TRACER_SEES_SIGNALMASKED_CRASH(
    unrelated_tracer_sees_signalmasked_crash_fpe, SIGFPE)
UNRELATED_TRACER_SEES_SIGNALMASKED_CRASH(
    unrelated_tracer_sees_signalmasked_crash_bus, SIGBUS)

#define UNRELATED_TRACER_SEES_SIGNALIGNORED_CRASH(test, sig)		\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Assert that an unrelated tracer sees crash signal from "	\
	    "the debuggee with signal ignored");			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	unrelated_tracer_sees_crash(sig, false, true);			\
}

UNRELATED_TRACER_SEES_SIGNALIGNORED_CRASH(
    unrelated_tracer_sees_signalignored_crash_trap, SIGTRAP)
UNRELATED_TRACER_SEES_SIGNALIGNORED_CRASH(
    unrelated_tracer_sees_signalignored_crash_segv, SIGSEGV)
UNRELATED_TRACER_SEES_SIGNALIGNORED_CRASH(
    unrelated_tracer_sees_signalignored_crash_ill, SIGILL)
UNRELATED_TRACER_SEES_SIGNALIGNORED_CRASH(
    unrelated_tracer_sees_signalignored_crash_fpe, SIGFPE)
UNRELATED_TRACER_SEES_SIGNALIGNORED_CRASH(
    unrelated_tracer_sees_signalignored_crash_bus, SIGBUS)
#endif

/// ----------------------------------------------------------------------------

ATF_TC(signal_mask_unrelated);
ATF_TC_HEAD(signal_mask_unrelated, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking single unrelated signal does not stop tracer "
	    "from catching other signals");
}

ATF_TC_BODY(signal_mask_unrelated, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	const int signotmasked = SIGINT;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sigemptyset(&intmask);
		sigaddset(&intmask, sigmasked);
		sigprocmask(SIG_BLOCK, &intmask, NULL);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before raising %s from child\n",
		    strsignal(signotmasked));
		FORKEE_ASSERT(raise(signotmasked) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, signotmasked);

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

#define ATF_TP_ADD_TCS_PTRACE_WAIT_SIGNAL() \
	ATF_TP_ADD_TC(tp, traceme_raise1); \
	ATF_TP_ADD_TC(tp, traceme_raise2); \
	ATF_TP_ADD_TC(tp, traceme_raise3); \
	ATF_TP_ADD_TC(tp, traceme_raise4); \
	ATF_TP_ADD_TC(tp, traceme_raise5); \
	ATF_TP_ADD_TC(tp, traceme_raise6); \
	ATF_TP_ADD_TC(tp, traceme_raise7); \
	ATF_TP_ADD_TC(tp, traceme_raise8); \
	ATF_TP_ADD_TC(tp, traceme_raise9); \
	ATF_TP_ADD_TC(tp, traceme_raise10); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored1); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored2); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored3); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored4); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored5); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored6); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored7); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored8); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked1); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked2); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked3); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked4); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked5); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked6); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked7); \
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked8); \
	ATF_TP_ADD_TC(tp, traceme_crash_trap); \
	ATF_TP_ADD_TC(tp, traceme_crash_segv); \
	ATF_TP_ADD_TC(tp, traceme_crash_ill); \
	ATF_TP_ADD_TC(tp, traceme_crash_fpe); \
	ATF_TP_ADD_TC(tp, traceme_crash_bus); \
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_trap); \
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_segv); \
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_ill); \
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_fpe); \
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_bus); \
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_trap); \
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_segv); \
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_ill); \
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_fpe); \
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_bus); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle1); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle2); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle3); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle4); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle5); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle6); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle7); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle8); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked1); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked2); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked3); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked4); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked5); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked6); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked7); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked8); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored1); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored2); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored3); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored4); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored5); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored6); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored7); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored8); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple1); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple2); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple3); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple4); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple5); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple6); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple7); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple8); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple9); \
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple10); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise1); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise2); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise3); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise4); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise5); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise6); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise7); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise8); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise9); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise10); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise11); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise12); \
	ATF_TP_ADD_TC(tp, traceme_vfork_raise13); \
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_trap); \
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_segv); \
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_ill); \
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_fpe); \
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_bus); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_trap); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_segv); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_ill); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_fpe); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_bus); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_trap); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_segv); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_ill); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_fpe); \
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_bus); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_trap); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_segv); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_ill); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_fpe); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_bus); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalmasked_crash_trap); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalmasked_crash_segv); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalmasked_crash_ill); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalmasked_crash_fpe); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalmasked_crash_bus); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalignored_crash_trap); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalignored_crash_segv); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalignored_crash_ill); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalignored_crash_fpe); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
	    unrelated_tracer_sees_signalignored_crash_bus); \
	ATF_TP_ADD_TC(tp, signal_mask_unrelated);
