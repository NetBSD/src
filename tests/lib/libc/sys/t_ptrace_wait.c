/*	$NetBSD: t_ptrace_wait.c,v 1.167 2020/03/01 18:22:00 kamil Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.167 2020/03/01 18:22:00 kamil Exp $");

#define __LEGACY_PT_LWPINFO

#include <sys/param.h>
#include <sys/types.h>
#include <sys/exec_elf.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <machine/reg.h>
#include <assert.h>
#include <elf.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <lwp.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <fenv.h>
#if (__arm__ && !__SOFTFP__) || __aarch64__
#include <ieeefp.h> /* only need for ARM Cortex/Neon hack */
#endif

#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#include <x86/cpu_extended_state.h>
#include <x86/specialreg.h>
#endif

#include <libelf.h>
#include <gelf.h>

#include <atf-c.h>

#ifdef ENABLE_TESTS

/* Assumptions in the kernel code that must be kept. */
static_assert(sizeof(((struct ptrace_state *)0)->pe_report_event) ==
    sizeof(((siginfo_t *)0)->si_pe_report_event),
    "pe_report_event and si_pe_report_event must be of the same size");
static_assert(sizeof(((struct ptrace_state *)0)->pe_other_pid) ==
    sizeof(((siginfo_t *)0)->si_pe_other_pid),
    "pe_other_pid and si_pe_other_pid must be of the same size");
static_assert(sizeof(((struct ptrace_state *)0)->pe_lwp) ==
    sizeof(((siginfo_t *)0)->si_pe_lwp),
    "pe_lwp and si_pe_lwp must be of the same size");
static_assert(sizeof(((struct ptrace_state *)0)->pe_other_pid) ==
    sizeof(((struct ptrace_state *)0)->pe_lwp),
    "pe_other_pid and pe_lwp must be of the same size");

#include "h_macros.h"

#include "t_ptrace_wait.h"
#include "msg.h"

#define PARENT_TO_CHILD(info, fds, msg) \
    SYSCALL_REQUIRE(msg_write_child(info " to child " # fds, &fds, &msg, \
	sizeof(msg)) == 0)

#define CHILD_FROM_PARENT(info, fds, msg) \
    FORKEE_ASSERT(msg_read_parent(info " from parent " # fds, &fds, &msg, \
	sizeof(msg)) == 0)

#define CHILD_TO_PARENT(info, fds, msg) \
    FORKEE_ASSERT(msg_write_parent(info " to parent " # fds, &fds, &msg, \
	sizeof(msg)) == 0)

#define PARENT_FROM_CHILD(info, fds, msg) \
    SYSCALL_REQUIRE(msg_read_child(info " from parent " # fds, &fds, &msg, \
	sizeof(msg)) == 0)

#define SYSCALL_REQUIRE(expr) ATF_REQUIRE_MSG(expr, "%s: %s", # expr, \
    strerror(errno))
#define SYSCALL_REQUIRE_ERRNO(res, exp) ATF_REQUIRE_MSG(res == exp, \
    "%d(%s) != %d", res, strerror(res), exp)

static int debug = 0;

#define DPRINTF(a, ...)	do  \
	if (debug) \
	printf("%s() %d.%d %s:%d " a, \
	__func__, getpid(), _lwp_self(), __FILE__, __LINE__,  ##__VA_ARGS__); \
    while (/*CONSTCOND*/0)

/// ----------------------------------------------------------------------------

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

		DPRINTF("Assert that PT_GET_PROCESS_STATE returns non-error");
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

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child");
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
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, FPE_INTDIV);
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

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child");
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
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, FPE_INTDIV);
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

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child");
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
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, FPE_INTDIV);
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

ATF_TC(traceme_pid1_parent);
ATF_TC_HEAD(traceme_pid1_parent, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PT_TRACE_ME is not allowed when our parent is PID1");
}

ATF_TC_BODY(traceme_pid1_parent, tc)
{
	struct msg_fds parent_child;
	int exitval_child1 = 1, exitval_child2 = 2;
	pid_t child1, child2, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	SYSCALL_REQUIRE(msg_open(&parent_child) == 0);

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child1 = fork()) != -1);
	if (child1 == 0) {
		DPRINTF("Before forking process PID=%d\n", getpid());
		SYSCALL_REQUIRE((child2 = fork()) != -1);
		if (child2 != 0) {
			DPRINTF("Parent process PID=%d, child2's PID=%d\n",
			    getpid(), child2);
			_exit(exitval_child1);
		}
		CHILD_FROM_PARENT("exit child1", parent_child, msg);

		DPRINTF("Assert that our parent is PID1 (initproc)\n");
		FORKEE_ASSERT_EQ(getppid(), 1);

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) == -1);
		SYSCALL_REQUIRE_ERRNO(errno, EPERM);

		CHILD_TO_PARENT("child2 exiting", parent_child, msg);

		_exit(exitval_child2);
	}
	DPRINTF("Parent process PID=%d, child1's PID=%d\n", getpid(), child1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(child1, &status, WEXITED), child1);

	validate_status_exited(status, exitval_child1);

	DPRINTF("Notify that child1 is dead\n");
	PARENT_TO_CHILD("exit child1", parent_child, msg);

	DPRINTF("Wait for exiting of child2\n");
	PARENT_FROM_CHILD("child2 exiting", parent_child, msg);
}

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
			FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, FPE_INTDIV);
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

#if defined(TWAIT_HAVE_PID)
static void
tracer_sees_terminaton_before_the_parent_raw(bool notimeout, bool unrelated,
                                             bool stopped)
{
	/*
	 * notimeout - disable timeout in await zombie function
	 * unrelated - attach from unrelated tracer reparented to initproc
	 * stopped - attach to a stopped process
	 */

	struct msg_fds parent_tracee, parent_tracer;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	/*
	 * Only a subset of options are supported.
	 */
	ATF_REQUIRE((!notimeout && !unrelated && !stopped) ||
	            (!notimeout && unrelated && !stopped) ||
	            (notimeout && !unrelated && !stopped) ||
	            (!notimeout && unrelated && stopped));

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		if (stopped) {
			DPRINTF("Stop self PID %d\n", getpid());
			raise(SIGSTOP);
		}

		// Wait for parent to let us exit
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);
		_exit(exitval_tracee);
	}

	DPRINTF("Spawn debugger\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		if(unrelated) {
			/* Fork again and drop parent to reattach to PID 1 */
			tracer = atf_utils_fork();
			if (tracer != 0)
				_exit(exitval_tracer);
		}

		if (stopped) {
			DPRINTF("Await for a stopped parent PID %d\n", tracee);
			await_stopped(tracee);
		}

		DPRINTF("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		CHILD_TO_PARENT("tracer ready", parent_tracer, msg);

		/* Wait for parent to tell use that tracee should have exited */
		CHILD_FROM_PARENT("wait for tracee exit", parent_tracer, msg);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);
		DPRINTF("Tracee %d exited with %d\n", tracee, exitval_tracee);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(unrelated ? 0 /* collect by initproc */ : exitval_tracer);
	}

	if (unrelated) {
		DPRINTF("Wait for the tracer process (direct child) to exit "
		    "calling %s()\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracer, &status, 0), tracer);

		validate_status_exited(status, exitval_tracer);

		DPRINTF("Wait for the non-exited tracee process with %s()\n",
		    TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, NULL, WNOHANG), 0);
	}

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("tracer ready", parent_tracer, msg);

	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("exit tracee", parent_tracee,  msg);

	DPRINTF("Detect that tracee is zombie\n");
	if (notimeout)
		await_zombie_raw(tracee, 0);
	else
		await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee %d - "
	    "Tracer must detect zombie first - calling %s()\n", tracee,
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	if (unrelated) {
		DPRINTF("Resume the tracer and let it detect exited tracee\n");
		PARENT_TO_CHILD("Message 2", parent_tracer, msg);
	} else {
		DPRINTF("Tell the tracer child should have exited\n");
		PARENT_TO_CHILD("wait for tracee exit", parent_tracer,  msg);
		DPRINTF("Wait for tracer to finish its job and exit - calling "
			"%s()\n", TWAIT_FNAME);

		DPRINTF("Wait from tracer child to complete waiting for "
			"tracee\n");
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
		    tracer);

		validate_status_exited(status, exitval_tracer);
	}

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

	validate_status_exited(status, exitval_tracee);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}

ATF_TC(tracer_sees_terminaton_before_the_parent);
ATF_TC_HEAD(tracer_sees_terminaton_before_the_parent, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees process termination before the parent");
}

ATF_TC_BODY(tracer_sees_terminaton_before_the_parent, tc)
{

	tracer_sees_terminaton_before_the_parent_raw(false, false, false);
}

ATF_TC(tracer_sysctl_lookup_without_duplicates);
ATF_TC_HEAD(tracer_sysctl_lookup_without_duplicates, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr",
	    "Assert that await_zombie() in attach1 always finds a single "
	    "process and no other error is reported");
}

ATF_TC_BODY(tracer_sysctl_lookup_without_duplicates, tc)
{
	time_t start, end;
	double diff;
	unsigned long N = 0;

	/*
	 * Reuse this test with tracer_sees_terminaton_before_the_parent_raw().
	 * This test body isn't specific to this race, however it's just good
	 * enough for this purposes, no need to invent a dedicated code flow.
	 */

	start = time(NULL);
	while (true) {
		DPRINTF("Step: %lu\n", N);
		tracer_sees_terminaton_before_the_parent_raw(true, false,
		                                             false);
		end = time(NULL);
		diff = difftime(end, start);
		if (diff >= 5.0)
			break;
		++N;
	}
	DPRINTF("Iterations: %lu\n", N);
}

ATF_TC(unrelated_tracer_sees_terminaton_before_the_parent);
ATF_TC_HEAD(unrelated_tracer_sees_terminaton_before_the_parent, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees process termination before the parent");
}

ATF_TC_BODY(unrelated_tracer_sees_terminaton_before_the_parent, tc)
{

	tracer_sees_terminaton_before_the_parent_raw(false, true, false);
}

ATF_TC(tracer_attach_to_unrelated_stopped_process);
ATF_TC_HEAD(tracer_attach_to_unrelated_stopped_process, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer can attach to an unrelated stopped process");
}

ATF_TC_BODY(tracer_attach_to_unrelated_stopped_process, tc)
{

	tracer_sees_terminaton_before_the_parent_raw(false, true, true);
}
#endif

/// ----------------------------------------------------------------------------

static void
parent_attach_to_its_child(bool stopped)
{
	struct msg_fds parent_tracee;
	const int exitval_tracee = 5;
	pid_t tracee, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		CHILD_FROM_PARENT("Message 1", parent_tracee, msg);
		DPRINTF("Parent should now attach to tracee\n");

		if (stopped) {
			DPRINTF("Stop self PID %d\n", getpid());
			SYSCALL_REQUIRE(raise(SIGSTOP) != -1);
		}

		CHILD_FROM_PARENT("Message 2", parent_tracee, msg);
		/* Wait for message from the parent */
		_exit(exitval_tracee);
	}
	PARENT_TO_CHILD("Message 1", parent_tracee, msg);

	if (stopped) {
		DPRINTF("Await for a stopped tracee PID %d\n", tracee);
		await_stopped(tracee);
	}

	DPRINTF("Before calling PT_ATTACH for tracee %d\n", tracee);
	SYSCALL_REQUIRE(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

	DPRINTF("Wait for the stopped tracee process with %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

	validate_status_stopped(status, SIGSTOP);

	DPRINTF("Resume tracee with PT_CONTINUE\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

	DPRINTF("Let the tracee exit now\n");
	PARENT_TO_CHILD("Message 2", parent_tracee, msg);

	DPRINTF("Wait for tracee to exit with %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

	validate_status_exited(status, exitval_tracee);

	DPRINTF("Before calling %s() for tracee\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD,
	    wpid = TWAIT_GENERIC(tracee, &status, 0));

	msg_close(&parent_tracee);
}

ATF_TC(parent_attach_to_its_child);
ATF_TC_HEAD(parent_attach_to_its_child, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer parent can PT_ATTACH to its child");
}

ATF_TC_BODY(parent_attach_to_its_child, tc)
{

	parent_attach_to_its_child(false);
}

ATF_TC(parent_attach_to_its_stopped_child);
ATF_TC_HEAD(parent_attach_to_its_stopped_child, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer parent can PT_ATTACH to its stopped child");
}

ATF_TC_BODY(parent_attach_to_its_stopped_child, tc)
{

	parent_attach_to_its_child(true);
}

/// ----------------------------------------------------------------------------

static void
child_attach_to_its_parent(bool stopped)
{
	struct msg_fds parent_tracee;
	const int exitval_tracer = 5;
	pid_t tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Spawn tracer\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* Wait for message from the parent */
		CHILD_FROM_PARENT("Message 1", parent_tracee, msg);

		if (stopped) {
			DPRINTF("Await for a stopped parent PID %d\n",
			        getppid());
			await_stopped(getppid());
		}

		DPRINTF("Attach to parent PID %d with PT_ATTACH from child\n",
		    getppid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, getppid(), NULL, 0) != -1);

		DPRINTF("Wait for the stopped parent process with %s()\n",
		    TWAIT_FNAME);
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(getppid(), &status, 0), getppid());

		forkee_status_stopped(status, SIGSTOP);

		DPRINTF("Resume parent with PT_DETACH\n");
		FORKEE_ASSERT(ptrace(PT_DETACH, getppid(), (void *)1, 0)
		    != -1);

		/* Tell parent we are ready */
		CHILD_TO_PARENT("Message 1", parent_tracee, msg);

		_exit(exitval_tracer);
	}

	DPRINTF("Wait for the tracer to become ready\n");
	PARENT_TO_CHILD("Message 1", parent_tracee, msg);

	if (stopped) {
		DPRINTF("Stop self PID %d\n", getpid());
		SYSCALL_REQUIRE(raise(SIGSTOP) != -1);
	}

	DPRINTF("Allow the tracer to exit now\n");
	PARENT_FROM_CHILD("Message 1", parent_tracee, msg);

	DPRINTF("Wait for tracer to exit with %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracer, &status, 0), tracer);

	validate_status_exited(status, exitval_tracer);

	DPRINTF("Before calling %s() for tracer\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD,
	    wpid = TWAIT_GENERIC(tracer, &status, 0));

	msg_close(&parent_tracee);
}

ATF_TC(child_attach_to_its_parent);
ATF_TC_HEAD(child_attach_to_its_parent, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer child can PT_ATTACH to its parent");
}

ATF_TC_BODY(child_attach_to_its_parent, tc)
{

	child_attach_to_its_parent(false);
}

ATF_TC(child_attach_to_its_stopped_parent);
ATF_TC_HEAD(child_attach_to_its_stopped_parent, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer child can PT_ATTACH to its stopped parent");
}

ATF_TC_BODY(child_attach_to_its_stopped_parent, tc)
{
	/*
	 * The ATF framework (atf-run) does not tolerate raise(SIGSTOP), as
	 * this causes a pipe (established from atf-run) to be broken.
	 * atf-run uses this mechanism to monitor whether a test is alive.
	 *
	 * As a workaround spawn this test as a subprocess.
	 */

	const int exitval = 15;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		child_attach_to_its_parent(true);
		_exit(exitval);
	} else {
		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_exited(status, exitval);

		DPRINTF("Before calling %s() for the exited child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
	}
}

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)

enum tracee_sees_its_original_parent_type {
	TRACEE_SEES_ITS_ORIGINAL_PARENT_GETPPID,
	TRACEE_SEES_ITS_ORIGINAL_PARENT_SYSCTL_KINFO_PROC2,
	TRACEE_SEES_ITS_ORIGINAL_PARENT_PROCFS_STATUS
};

static void
tracee_sees_its_original_parent(enum tracee_sees_its_original_parent_type type)
{
	struct msg_fds parent_tracer, parent_tracee;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	/* sysctl(3) - kinfo_proc2 */
	int name[CTL_MAXNAME];
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);
	unsigned int namelen;

	/* procfs - status  */
	FILE *fp;
	struct stat st;
	const char *fname = "/proc/curproc/status";
	char s_executable[MAXPATHLEN];
	int s_pid, s_ppid;
	int rv;

	if (type == TRACEE_SEES_ITS_ORIGINAL_PARENT_PROCFS_STATUS) {
		SYSCALL_REQUIRE(
		    (rv = stat(fname, &st)) == 0 || (errno == ENOENT));
		if (rv != 0)
			atf_tc_skip("/proc/curproc/status not found");
	}

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		parent = getppid();

		/* Emit message to the parent */
		CHILD_TO_PARENT("tracee ready", parent_tracee, msg);
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);

		switch (type) {
		case TRACEE_SEES_ITS_ORIGINAL_PARENT_GETPPID:
			FORKEE_ASSERT_EQ(parent, getppid());
			break;
		case TRACEE_SEES_ITS_ORIGINAL_PARENT_SYSCTL_KINFO_PROC2:
			namelen = 0;
			name[namelen++] = CTL_KERN;
			name[namelen++] = KERN_PROC2;
			name[namelen++] = KERN_PROC_PID;
			name[namelen++] = getpid();
			name[namelen++] = len;
			name[namelen++] = 1;

			FORKEE_ASSERT_EQ(
			    sysctl(name, namelen, &kp, &len, NULL, 0), 0);
			FORKEE_ASSERT_EQ(parent, kp.p_ppid);
			break;
		case TRACEE_SEES_ITS_ORIGINAL_PARENT_PROCFS_STATUS:
			/*
			 * Format:
			 *  EXECUTABLE PID PPID ...
			 */
			FORKEE_ASSERT((fp = fopen(fname, "r")) != NULL);
			fscanf(fp, "%s %d %d", s_executable, &s_pid, &s_ppid);
			FORKEE_ASSERT_EQ(fclose(fp), 0);
			FORKEE_ASSERT_EQ(parent, s_ppid);
			break;
		}

		_exit(exitval_tracee);
	}
	DPRINTF("Wait for child to record its parent identifier (pid)\n");
	PARENT_FROM_CHILD("tracee ready", parent_tracee, msg);

	DPRINTF("Spawn debugger\n");
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		DPRINTF("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		CHILD_TO_PARENT("tracer ready", parent_tracer, msg);

		/* Wait for parent to tell use that tracee should have exited */
		CHILD_FROM_PARENT("wait for tracee exit", parent_tracer, msg);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("tracer ready",  parent_tracer, msg);

	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("exit tracee",  parent_tracee, msg);

	DPRINTF("Detect that tracee is zombie\n");
	await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	DPRINTF("Tell the tracer child should have exited\n");
	PARENT_TO_CHILD("wait for tracee exit",  parent_tracer, msg);

	DPRINTF("Wait from tracer child to complete waiting for tracee\n");
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}

#define TRACEE_SEES_ITS_ORIGINAL_PARENT(test, type, descr)		\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Assert that tracee sees its original parent when being traced " \
	    "(check " descr ")");					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	tracee_sees_its_original_parent(type);				\
}

TRACEE_SEES_ITS_ORIGINAL_PARENT(
	tracee_sees_its_original_parent_getppid,
	TRACEE_SEES_ITS_ORIGINAL_PARENT_GETPPID,
	"getppid(2)");
TRACEE_SEES_ITS_ORIGINAL_PARENT(
	tracee_sees_its_original_parent_sysctl_kinfo_proc2,
	TRACEE_SEES_ITS_ORIGINAL_PARENT_SYSCTL_KINFO_PROC2,
	"sysctl(3) and kinfo_proc2");
TRACEE_SEES_ITS_ORIGINAL_PARENT(
	tracee_sees_its_original_parent_procfs_status,
	TRACEE_SEES_ITS_ORIGINAL_PARENT_PROCFS_STATUS,
	"the status file in procfs");
#endif

/// ----------------------------------------------------------------------------

static void
eventmask_preserved(int event)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_event_t set_event, get_event;
	const int len = sizeof(ptrace_event_t);

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

	set_event.pe_set_event = event;
	SYSCALL_REQUIRE(
	    ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
	DPRINTF("set_event=%#x get_event=%#x\n", set_event.pe_set_event,
	    get_event.pe_set_event);
	ATF_REQUIRE(memcmp(&set_event, &get_event, len) == 0);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define EVENTMASK_PRESERVED(test, event)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that eventmask " #event " is preserved");		\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	eventmask_preserved(event);					\
}

EVENTMASK_PRESERVED(eventmask_preserved_empty, 0)
EVENTMASK_PRESERVED(eventmask_preserved_fork, PTRACE_FORK)
EVENTMASK_PRESERVED(eventmask_preserved_vfork, PTRACE_VFORK)
EVENTMASK_PRESERVED(eventmask_preserved_vfork_done, PTRACE_VFORK_DONE)
EVENTMASK_PRESERVED(eventmask_preserved_lwp_create, PTRACE_LWP_CREATE)
EVENTMASK_PRESERVED(eventmask_preserved_lwp_exit, PTRACE_LWP_EXIT)
EVENTMASK_PRESERVED(eventmask_preserved_posix_spawn, PTRACE_POSIX_SPAWN)

/// ----------------------------------------------------------------------------

static void
fork_body(const char *fn, bool trackspawn, bool trackfork, bool trackvfork,
    bool trackvforkdone)
{
	const int exitval = 5;
	const int exitval2 = 0; /* This matched exit status from /bin/echo */
	const int sigval = SIGSTOP;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (strcmp(fn, "spawn") == 0) {
			FORKEE_ASSERT_EQ(posix_spawn(&child2,
			    arg[0], NULL, NULL, arg, NULL), 0);
		} else {
			if (strcmp(fn, "fork") == 0) {
				FORKEE_ASSERT((child2 = fork()) != -1);
			} else if (strcmp(fn, "vfork") == 0) {
				FORKEE_ASSERT((child2 = vfork()) != -1);
			}

			if (child2 == 0)
				_exit(exitval2);
		}
		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set 0%s%s%s%s in EVENT_MASK for the child %d\n",
	    trackspawn ? "|PTRACE_POSIX_SPAWN" : "",
	    trackfork ? "|PTRACE_FORK" : "",
	    trackvfork ? "|PTRACE_VFORK" : "",
	    trackvforkdone ? "|PTRACE_VFORK_DONE" : "", child);
	event.pe_set_event = 0;
	if (trackspawn)
		event.pe_set_event |= PTRACE_POSIX_SPAWN;
	if (trackfork)
		event.pe_set_event |= PTRACE_FORK;
	if (trackvfork)
		event.pe_set_event |= PTRACE_VFORK;
	if (trackvforkdone)
		event.pe_set_event |= PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

#if defined(TWAIT_HAVE_PID)
	if ((trackspawn && strcmp(fn, "spawn") == 0) ||
	    (trackfork && strcmp(fn, "fork") == 0) ||
	    (trackvfork && strcmp(fn, "vfork") == 0)) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		if (trackspawn && strcmp(fn, "spawn") == 0) {
			ATF_REQUIRE_EQ(
			    state.pe_report_event & PTRACE_POSIX_SPAWN,
			       PTRACE_POSIX_SPAWN);
		}
		if (trackfork && strcmp(fn, "fork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (trackvfork && strcmp(fn, "vfork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		child2 = state.pe_other_pid;
		DPRINTF("Reported ptrace event with forkee %d\n", child2);

		DPRINTF("Before calling %s() for the forkee %d of the child "
		    "%d\n", TWAIT_FNAME, child2, child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
		    child2);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
		if (trackspawn && strcmp(fn, "spawn") == 0) {
			ATF_REQUIRE_EQ(
			    state.pe_report_event & PTRACE_POSIX_SPAWN,
			       PTRACE_POSIX_SPAWN);
		}
		if (trackfork && strcmp(fn, "fork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (trackvfork && strcmp(fn, "vfork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		ATF_REQUIRE_EQ(state.pe_other_pid, child);

		DPRINTF("Before resuming the forkee process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}
#endif

	if (trackvforkdone && strcmp(fn, "vfork") == 0) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

		child2 = state.pe_other_pid;
		DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
		    child2);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

#if defined(TWAIT_HAVE_PID)
	if ((trackspawn && strcmp(fn, "spawn") == 0) ||
	    (trackfork && strcmp(fn, "fork") == 0) ||
	    (trackvfork && strcmp(fn, "vfork") == 0)) {
		DPRINTF("Before calling %s() for the forkee - expected exited"
		    "\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		validate_status_exited(status, exitval2);

		DPRINTF("Before calling %s() for the forkee - expected no "
		    "process\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD,
		    wpid = TWAIT_GENERIC(child2, &status, 0));
	}
#endif

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGCHLD\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGCHLD);

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

#define FORK_TEST(name,fun,tspawn,tfork,tvfork,tvforkdone)		\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify " fun "() "		\
	    "called with 0%s%s%s%s in EVENT_MASK",			\
	    tspawn ? "|PTRACE_POSIX_SPAWN" : "",			\
	    tfork ? "|PTRACE_FORK" : "",				\
	    tvfork ? "|PTRACE_VFORK" : "",				\
	    tvforkdone ? "|PTRACE_VFORK_DONE" : "");			\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	fork_body(fun, tspawn, tfork, tvfork, tvforkdone);		\
}

FORK_TEST(fork1, "fork", false, false, false, false)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(fork2, "fork", false, true, false, false)
FORK_TEST(fork3, "fork", false, false, true, false)
FORK_TEST(fork4, "fork", false, true, true, false)
#endif
FORK_TEST(fork5, "fork", false, false, false, true)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(fork6, "fork", false, true, false, true)
FORK_TEST(fork7, "fork", false, false, true, true)
FORK_TEST(fork8, "fork", false, true, true, true)
#endif
FORK_TEST(fork9, "fork", true, false, false, false)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(fork10, "fork", true, true, false, false)
FORK_TEST(fork11, "fork", true, false, true, false)
FORK_TEST(fork12, "fork", true, true, true, false)
#endif
FORK_TEST(fork13, "fork", true, false, false, true)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(fork14, "fork", true, true, false, true)
FORK_TEST(fork15, "fork", true, false, true, true)
FORK_TEST(fork16, "fork", true, true, true, true)
#endif

FORK_TEST(vfork1, "vfork", false, false, false, false)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(vfork2, "vfork", false, true, false, false)
FORK_TEST(vfork3, "vfork", false, false, true, false)
FORK_TEST(vfork4, "vfork", false, true, true, false)
#endif
FORK_TEST(vfork5, "vfork", false, false, false, true)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(vfork6, "vfork", false, true, false, true)
FORK_TEST(vfork7, "vfork", false, false, true, true)
FORK_TEST(vfork8, "vfork", false, true, true, true)
#endif
FORK_TEST(vfork9, "vfork", true, false, false, false)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(vfork10, "vfork", true, true, false, false)
FORK_TEST(vfork11, "vfork", true, false, true, false)
FORK_TEST(vfork12, "vfork", true, true, true, false)
#endif
FORK_TEST(vfork13, "vfork", true, false, false, true)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(vfork14, "vfork", true, true, false, true)
FORK_TEST(vfork15, "vfork", true, false, true, true)
FORK_TEST(vfork16, "vfork", true, true, true, true)
#endif

FORK_TEST(posix_spawn1, "spawn", false, false, false, false)
FORK_TEST(posix_spawn2, "spawn", false, true, false, false)
FORK_TEST(posix_spawn3, "spawn", false, false, true, false)
FORK_TEST(posix_spawn4, "spawn", false, true, true, false)
FORK_TEST(posix_spawn5, "spawn", false, false, false, true)
FORK_TEST(posix_spawn6, "spawn", false, true, false, true)
FORK_TEST(posix_spawn7, "spawn", false, false, true, true)
FORK_TEST(posix_spawn8, "spawn", false, true, true, true)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(posix_spawn9, "spawn", true, false, false, false)
FORK_TEST(posix_spawn10, "spawn", true, true, false, false)
FORK_TEST(posix_spawn11, "spawn", true, false, true, false)
FORK_TEST(posix_spawn12, "spawn", true, true, true, false)
FORK_TEST(posix_spawn13, "spawn", true, false, false, true)
FORK_TEST(posix_spawn14, "spawn", true, true, false, true)
FORK_TEST(posix_spawn15, "spawn", true, false, true, true)
FORK_TEST(posix_spawn16, "spawn", true, true, true, true)
#endif

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
unrelated_tracer_fork_body(const char *fn, bool trackspawn, bool trackfork,
    bool trackvfork, bool trackvforkdone)
{
	const int sigval = SIGSTOP;
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval = 10;
	const int exitval2 = 0; /* This matched exit status from /bin/echo */
	pid_t tracee, tracer, wpid;
	pid_t tracee2 = 0;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	struct ptrace_siginfo info;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		// Wait for parent to let us crash
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (strcmp(fn, "spawn") == 0) {
			FORKEE_ASSERT_EQ(posix_spawn(&tracee2,
			    arg[0], NULL, NULL, arg, NULL), 0);
		} else {
			if (strcmp(fn, "fork") == 0) {
				FORKEE_ASSERT((tracee2 = fork()) != -1);
			} else if (strcmp(fn, "vfork") == 0) {
				FORKEE_ASSERT((tracee2 = vfork()) != -1);
			}

			if (tracee2 == 0)
				_exit(exitval2);
		}
		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(tracee2, &status, 0), tracee2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
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

		DPRINTF("Set 0%s%s%s%s in EVENT_MASK for the child %d\n",
		    trackspawn ? "|PTRACE_POSIX_SPAWN" : "",
		    trackfork ? "|PTRACE_FORK" : "",
		    trackvfork ? "|PTRACE_VFORK" : "",
		    trackvforkdone ? "|PTRACE_VFORK_DONE" : "", tracee);
		event.pe_set_event = 0;
		if (trackspawn)
			event.pe_set_event |= PTRACE_POSIX_SPAWN;
		if (trackfork)
			event.pe_set_event |= PTRACE_FORK;
		if (trackvfork)
			event.pe_set_event |= PTRACE_VFORK;
		if (trackvforkdone)
			event.pe_set_event |= PTRACE_VFORK_DONE;
		SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, tracee, &event, elen)
		    != -1);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		if ((trackspawn && strcmp(fn, "spawn") == 0) ||
		    (trackfork && strcmp(fn, "fork") == 0) ||
		    (trackvfork && strcmp(fn, "vfork") == 0)) {
			DPRINTF("Before calling %s() for the tracee %d\n", TWAIT_FNAME,
			    tracee);
			TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0),
			    tracee);

			validate_status_stopped(status, SIGTRAP);

			SYSCALL_REQUIRE(
			    ptrace(PT_GET_PROCESS_STATE, tracee, &state, slen) != -1);
			if (trackspawn && strcmp(fn, "spawn") == 0) {
				ATF_REQUIRE_EQ(
				    state.pe_report_event & PTRACE_POSIX_SPAWN,
				       PTRACE_POSIX_SPAWN);
			}
			if (trackfork && strcmp(fn, "fork") == 0) {
				ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
				       PTRACE_FORK);
			}
			if (trackvfork && strcmp(fn, "vfork") == 0) {
				ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
				       PTRACE_VFORK);
			}

			tracee2 = state.pe_other_pid;
			DPRINTF("Reported ptrace event with forkee %d\n", tracee2);

			DPRINTF("Before calling %s() for the forkee %d of the tracee "
			    "%d\n", TWAIT_FNAME, tracee2, tracee);
			TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee2, &status, 0),
			    tracee2);

			validate_status_stopped(status, SIGTRAP);

			SYSCALL_REQUIRE(
			    ptrace(PT_GET_PROCESS_STATE, tracee2, &state, slen) != -1);
			if (trackspawn && strcmp(fn, "spawn") == 0) {
				ATF_REQUIRE_EQ(
				    state.pe_report_event & PTRACE_POSIX_SPAWN,
				       PTRACE_POSIX_SPAWN);
			}
			if (trackfork && strcmp(fn, "fork") == 0) {
				ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
				       PTRACE_FORK);
			}
			if (trackvfork && strcmp(fn, "vfork") == 0) {
				ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
				       PTRACE_VFORK);
			}

			ATF_REQUIRE_EQ(state.pe_other_pid, tracee);

			DPRINTF("Before resuming the forkee process where it left off "
			    "and without signal to be sent\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_CONTINUE, tracee2, (void *)1, 0) != -1);
		
			DPRINTF("Before resuming the tracee process where it left off "
			    "and without signal to be sent\n");
			SYSCALL_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);
		}

		if (trackvforkdone && strcmp(fn, "vfork") == 0) {
			DPRINTF("Before calling %s() for the tracee %d\n", TWAIT_FNAME,
			    tracee);
			TWAIT_REQUIRE_SUCCESS(
			    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

			validate_status_stopped(status, SIGTRAP);

			SYSCALL_REQUIRE(
			    ptrace(PT_GET_PROCESS_STATE, tracee, &state, slen) != -1);
			ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

			tracee2 = state.pe_other_pid;
			DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
			    tracee2);

			DPRINTF("Before resuming the tracee process where it left off "
			    "and without signal to be sent\n");
			SYSCALL_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);
		}


		if ((trackspawn && strcmp(fn, "spawn") == 0) ||
		    (trackfork && strcmp(fn, "fork") == 0) ||
		    (trackvfork && strcmp(fn, "vfork") == 0)) {
			DPRINTF("Before calling %s() for the forkee - expected exited"
			    "\n", TWAIT_FNAME);
			TWAIT_REQUIRE_SUCCESS(
			    wpid = TWAIT_GENERIC(tracee2, &status, 0), tracee2);

			validate_status_exited(status, exitval2);

			DPRINTF("Before calling %s() for the forkee - expected no "
			    "process\n", TWAIT_FNAME);
			TWAIT_REQUIRE_FAILURE(ECHILD,
			    wpid = TWAIT_GENERIC(tracee2, &status, 0));
		}

		DPRINTF("Before calling %s() for the tracee - expected stopped "
		    "SIGCHLD\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		validate_status_stopped(status, SIGCHLD);

		DPRINTF("Before resuming the tracee process where it left off and "
		    "without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		DPRINTF("Before calling %s() for the tracee - expected exited\n",
		    TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		validate_status_exited(status, exitval);

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

	validate_status_exited(status, exitval);

	DPRINTF("Await normal exit of tracer\n");
	PARENT_FROM_CHILD("tracer done", parent_tracer, msg);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}

#define UNRELATED_TRACER_FORK_TEST(name,fun,tspawn,tfork,tvfork,tvforkdone)\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify " fun "() "		\
	    "called with 0%s%s%s%s in EVENT_MASK",			\
	    tspawn ? "|PTRACE_POSIX_SPAWN" : "",			\
	    tfork ? "|PTRACE_FORK" : "",				\
	    tvfork ? "|PTRACE_VFORK" : "",				\
	    tvforkdone ? "|PTRACE_VFORK_DONE" : "");			\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	unrelated_tracer_fork_body(fun, tspawn, tfork, tvfork,		\
	    tvforkdone);						\
}

UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork1, "fork", false, false, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork2, "fork", false, true, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork3, "fork", false, false, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork4, "fork", false, true, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork5, "fork", false, false, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork6, "fork", false, true, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork7, "fork", false, false, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork8, "fork", false, true, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork9, "fork", true, false, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork10, "fork", true, true, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork11, "fork", true, false, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork12, "fork", true, true, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork13, "fork", true, false, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork14, "fork", true, true, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork15, "fork", true, false, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_fork16, "fork", true, true, true, true)

UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork1, "vfork", false, false, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork2, "vfork", false, true, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork3, "vfork", false, false, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork4, "vfork", false, true, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork5, "vfork", false, false, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork6, "vfork", false, true, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork7, "vfork", false, false, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork8, "vfork", false, true, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork9, "vfork", true, false, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork10, "vfork", true, true, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork11, "vfork", true, false, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork12, "vfork", true, true, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork13, "vfork", true, false, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork14, "vfork", true, true, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork15, "vfork", true, false, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_vfork16, "vfork", true, true, true, true)

UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn1, "spawn", false, false, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn2, "spawn", false, true, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn3, "spawn", false, false, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn4, "spawn", false, true, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn5, "spawn", false, false, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn6, "spawn", false, true, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn7, "spawn", false, false, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn8, "spawn", false, true, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn9, "spawn", true, false, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn10, "spawn", true, true, false, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn11, "spawn", true, false, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn12, "spawn", true, true, true, false)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn13, "spawn", true, false, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn14, "spawn", true, true, false, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn15, "spawn", true, false, true, true)
UNRELATED_TRACER_FORK_TEST(unrelated_tracer_posix_spawn16, "spawn", true, true, true, true)
#endif

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
fork_detach_forker_body(const char *fn, bool kill_process)
{
	const int exitval = 5;
	const int exitval2 = 0; /* Matches exit value from /bin/echo */
	const int sigval = SIGSTOP;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	int op;

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (strcmp(fn, "spawn") == 0) {
			FORKEE_ASSERT_EQ(posix_spawn(&child2,
			    arg[0], NULL, NULL, arg, NULL), 0);
		} else  {
			if (strcmp(fn, "fork") == 0) {
				FORKEE_ASSERT((child2 = fork()) != -1);
			} else {
				FORKEE_ASSERT((child2 = vfork()) != -1);
			}

			if (child2 == 0)
				_exit(exitval2);
		}

		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_POSIX_SPAWN | PTRACE_FORK | PTRACE_VFORK
		| PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME, child);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	SYSCALL_REQUIRE(
	    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	if (strcmp(fn, "spawn") == 0)
		op = PTRACE_POSIX_SPAWN;
	else if (strcmp(fn, "fork") == 0)
		op = PTRACE_FORK;
	else
		op = PTRACE_VFORK;

	ATF_REQUIRE_EQ(state.pe_report_event & op, op);

	child2 = state.pe_other_pid;
	DPRINTF("Reported ptrace event with forkee %d\n", child2);

	if (strcmp(fn, "spawn") == 0 || strcmp(fn, "fork") == 0 ||
	    strcmp(fn, "vfork") == 0)
		op = kill_process ? PT_KILL : PT_DETACH;
	else
		op = PT_CONTINUE;
	SYSCALL_REQUIRE(ptrace(op, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the forkee %d of the child %d\n",
	    TWAIT_FNAME, child2, child);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0), child2);

	validate_status_stopped(status, SIGTRAP);

	SYSCALL_REQUIRE(
	    ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
	if (strcmp(fn, "spawn") == 0)
		op = PTRACE_POSIX_SPAWN;
	else if (strcmp(fn, "fork") == 0)
		op = PTRACE_FORK;
	else
		op = PTRACE_VFORK;
	
	ATF_REQUIRE_EQ(state.pe_report_event & op, op);
	ATF_REQUIRE_EQ(state.pe_other_pid, child);

	DPRINTF("Before resuming the forkee process where it left off "
	    "and without signal to be sent\n");
 	SYSCALL_REQUIRE(
	    ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

	if (strcmp(fn, "vforkdone") == 0) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

		child2 = state.pe_other_pid;
		DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
		    child2);

		op = kill_process ? PT_KILL : PT_DETACH;
		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(op, child, (void *)1, 0) != -1);
	}

	DPRINTF("Before calling %s() for the forkee - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0), child2);

	validate_status_exited(status, exitval2);

	DPRINTF("Before calling %s() for the forkee - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child2, &status, 0));

	DPRINTF("Before calling %s() for the forkee - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	if (kill_process) {
		validate_status_signaled(status, SIGKILL, 0);
	} else {
		validate_status_exited(status, exitval);
	}

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define FORK_DETACH_FORKER(name,event,kprocess)				\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify %s " event,		\
	    kprocess ? "killed" : "detached");				\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	fork_detach_forker_body(event, kprocess);			\
}

FORK_DETACH_FORKER(posix_spawn_detach_spawner, "spawn", false)
FORK_DETACH_FORKER(fork_detach_forker, "fork", false)
FORK_DETACH_FORKER(vfork_detach_vforker, "vfork", false)
FORK_DETACH_FORKER(vfork_detach_vforkerdone, "vforkdone", false)

FORK_DETACH_FORKER(posix_spawn_kill_spawner, "spawn", true)
FORK_DETACH_FORKER(fork_kill_forker, "fork", true)
FORK_DETACH_FORKER(vfork_kill_vforker, "vfork", true)
FORK_DETACH_FORKER(vfork_kill_vforkerdone, "vforkdone", true)
#endif

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
unrelated_tracer_fork_detach_forker_body(const char *fn, bool kill_process)
{
	const int sigval = SIGSTOP;
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval = 10;
	const int exitval2 = 0; /* This matched exit status from /bin/echo */
	pid_t tracee, tracer, wpid;
	pid_t tracee2 = 0;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	int op;

	struct ptrace_siginfo info;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		// Wait for parent to let us crash
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (strcmp(fn, "spawn") == 0) {
			FORKEE_ASSERT_EQ(posix_spawn(&tracee2,
			    arg[0], NULL, NULL, arg, NULL), 0);
		} else  {
			if (strcmp(fn, "fork") == 0) {
				FORKEE_ASSERT((tracee2 = fork()) != -1);
			} else {
				FORKEE_ASSERT((tracee2 = vfork()) != -1);
			}

			if (tracee2 == 0)
				_exit(exitval2);
		}

		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(tracee2, &status, 0), tracee2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
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

		DPRINTF("Set EVENT_MASK for the child %d\n", tracee);
		event.pe_set_event = PTRACE_POSIX_SPAWN | PTRACE_FORK | PTRACE_VFORK
			| PTRACE_VFORK_DONE;
		SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, tracee, &event, elen) != -1);

		DPRINTF("Before resuming the child process where it left off and "
		    "without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME, tracee);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, tracee, &state, slen) != -1);

		if (strcmp(fn, "spawn") == 0)
			op = PTRACE_POSIX_SPAWN;
		else if (strcmp(fn, "fork") == 0)
			op = PTRACE_FORK;
		else
			op = PTRACE_VFORK;

		ATF_REQUIRE_EQ(state.pe_report_event & op, op);

		tracee2 = state.pe_other_pid;
		DPRINTF("Reported ptrace event with forkee %d\n", tracee2);
		if (strcmp(fn, "spawn") == 0 || strcmp(fn, "fork") == 0 ||
		    strcmp(fn, "vfork") == 0)
			op = kill_process ? PT_KILL : PT_DETACH;
		else
			op = PT_CONTINUE;
		SYSCALL_REQUIRE(ptrace(op, tracee, (void *)1, 0) != -1);

		DPRINTF("Before calling %s() for the forkee %d of the tracee %d\n",
		    TWAIT_FNAME, tracee2, tracee);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee2, &status, 0), tracee2);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, tracee2, &state, slen) != -1);
		if (strcmp(fn, "spawn") == 0)
			op = PTRACE_POSIX_SPAWN;
		else if (strcmp(fn, "fork") == 0)
			op = PTRACE_FORK;
		else
			op = PTRACE_VFORK;

		ATF_REQUIRE_EQ(state.pe_report_event & op, op);
		ATF_REQUIRE_EQ(state.pe_other_pid, tracee);

		DPRINTF("Before resuming the forkee process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_CONTINUE, tracee2, (void *)1, 0) != -1);

		if (strcmp(fn, "vforkdone") == 0) {
			DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
			    tracee);
			TWAIT_REQUIRE_SUCCESS(
			    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

			validate_status_stopped(status, SIGTRAP);

			SYSCALL_REQUIRE(
			    ptrace(PT_GET_PROCESS_STATE, tracee, &state, slen) != -1);
			ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

			tracee2 = state.pe_other_pid;
			DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
			    tracee2);

			op = kill_process ? PT_KILL : PT_DETACH;
			DPRINTF("Before resuming the child process where it left off "
			    "and without signal to be sent\n");
			SYSCALL_REQUIRE(ptrace(op, tracee, (void *)1, 0) != -1);
		}

		DPRINTF("Before calling %s() for the forkee - expected exited\n",
		    TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee2, &status, 0), tracee2);

		validate_status_exited(status, exitval2);

		DPRINTF("Before calling %s() for the forkee - expected no process\n",
		    TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(tracee2, &status, 0));

		if (kill_process) {
			DPRINTF("Before calling %s() for the forkee - expected signaled\n",
			    TWAIT_FNAME);
			TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

			validate_status_signaled(status, SIGKILL, 0);
		}

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

	if (kill_process) {
		validate_status_signaled(status, SIGKILL, 0);
	} else {
		validate_status_exited(status, exitval);
	}

	DPRINTF("Await normal exit of tracer\n");
	PARENT_FROM_CHILD("tracer done", parent_tracer, msg);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}

#define UNRELATED_TRACER_FORK_DETACH_FORKER(name,event,kprocess)	\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify %s " event,		\
	    kprocess ? "killed" : "detached");				\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	unrelated_tracer_fork_detach_forker_body(event, kprocess);	\
}

UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_posix_spawn_detach_spawner, "spawn", false)
UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_fork_detach_forker, "fork", false)
UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_vfork_detach_vforker, "vfork", false)
UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_vfork_detach_vforkerdone, "vforkdone", false)

UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_posix_spawn_kill_spawner, "spawn", true)
UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_fork_kill_forker, "fork", true)
UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_vfork_kill_vforker, "vfork", true)
UNRELATED_TRACER_FORK_DETACH_FORKER(unrelated_tracer_vfork_kill_vforkerdone, "vforkdone", true)
#endif

/// ----------------------------------------------------------------------------

static void
traceme_vfork_fork_body(pid_t (*fn)(void))
{
	const int exitval = 5;
	const int exitval2 = 15;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		FORKEE_ASSERT((child2 = (fn)()) != -1);

		if (child2 == 0)
			_exit(exitval2);

		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_FORK_TEST(name,fun)				\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify " #fun "(2) "		\
	    "called from vfork(2)ed child");				\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	traceme_vfork_fork_body(fun);					\
}

TRACEME_VFORK_FORK_TEST(traceme_vfork_fork, fork)
TRACEME_VFORK_FORK_TEST(traceme_vfork_vfork, vfork)

/// ----------------------------------------------------------------------------

enum bytes_transfer_type {
	BYTES_TRANSFER_DATA,
	BYTES_TRANSFER_DATAIO,
	BYTES_TRANSFER_TEXT,
	BYTES_TRANSFER_TEXTIO,
	BYTES_TRANSFER_AUXV
};

static int __used
bytes_transfer_dummy(int a, int b, int c, int d)
{
	int e, f, g, h;

	a *= 4;
	b += 3;
	c -= 2;
	d /= 1;

	e = strtol("10", NULL, 10);
	f = strtol("20", NULL, 10);
	g = strtol("30", NULL, 10);
	h = strtol("40", NULL, 10);

	return (a + b * c - d) + (e * f - g / h);
}

static void
bytes_transfer(int operation, size_t size, enum bytes_transfer_type type)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	bool skip = false;

	int lookup_me = 0;
	uint8_t lookup_me8 = 0;
	uint16_t lookup_me16 = 0;
	uint32_t lookup_me32 = 0;
	uint64_t lookup_me64 = 0;

	int magic = 0x13579246;
	uint8_t magic8 = 0xab;
	uint16_t magic16 = 0x1234;
	uint32_t magic32 = 0x98765432;
	uint64_t magic64 = 0xabcdef0123456789;

	struct ptrace_io_desc io;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	/* 513 is just enough, for the purposes of ATF it's good enough */
	AuxInfo ai[513], *aip;

	ATF_REQUIRE(size < sizeof(ai));

	/* Prepare variables for .TEXT transfers */
	switch (type) {
	case BYTES_TRANSFER_TEXT:
		memcpy(&magic, bytes_transfer_dummy, sizeof(magic));
		break;
	case BYTES_TRANSFER_TEXTIO:
		switch (size) {
		case 8:
			memcpy(&magic8, bytes_transfer_dummy, sizeof(magic8));
			break;
		case 16:
			memcpy(&magic16, bytes_transfer_dummy, sizeof(magic16));
			break;
		case 32:
			memcpy(&magic32, bytes_transfer_dummy, sizeof(magic32));
			break;
		case 64:
			memcpy(&magic64, bytes_transfer_dummy, sizeof(magic64));
			break;
		}
		break;
	default:
		break;
	}

	/* Prepare variables for PIOD and AUXV transfers */
	switch (type) {
	case BYTES_TRANSFER_TEXTIO:
	case BYTES_TRANSFER_DATAIO:
		io.piod_op = operation;
		switch (size) {
		case 8:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me8;
			io.piod_addr = &lookup_me8;
			io.piod_len = sizeof(lookup_me8);
			break;
		case 16:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me16;
			io.piod_addr = &lookup_me16;
			io.piod_len = sizeof(lookup_me16);
			break;
		case 32:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me32;
			io.piod_addr = &lookup_me32;
			io.piod_len = sizeof(lookup_me32);
			break;
		case 64:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me64;
			io.piod_addr = &lookup_me64;
			io.piod_len = sizeof(lookup_me64);
			break;
		default:
			break;
		}
		break;
	case BYTES_TRANSFER_AUXV:
		io.piod_op = operation;
		io.piod_offs = 0;
		io.piod_addr = ai;
		io.piod_len = size;
		break;
	default:
		break;
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		switch (type) {
		case BYTES_TRANSFER_DATA:
			switch (operation) {
			case PT_READ_D:
			case PT_READ_I:
				lookup_me = magic;
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_DATAIO:
			switch (operation) {
			case PIOD_READ_D:
			case PIOD_READ_I:
				switch (size) {
				case 8:
					lookup_me8 = magic8;
					break;
				case 16:
					lookup_me16 = magic16;
					break;
				case 32:
					lookup_me32 = magic32;
					break;
				case 64:
					lookup_me64 = magic64;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		default:
			break;
		}

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* Handle PIOD and PT separately as operation values overlap */
		switch (type) {
		case BYTES_TRANSFER_DATA:
			switch (operation) {
			case PT_WRITE_D:
			case PT_WRITE_I:
				FORKEE_ASSERT_EQ(lookup_me, magic);
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_DATAIO:
			switch (operation) {
			case PIOD_WRITE_D:
			case PIOD_WRITE_I:
				switch (size) {
				case 8:
					FORKEE_ASSERT_EQ(lookup_me8, magic8);
					break;
				case 16:
					FORKEE_ASSERT_EQ(lookup_me16, magic16);
					break;
				case 32:
					FORKEE_ASSERT_EQ(lookup_me32, magic32);
					break;
				case 64:
					FORKEE_ASSERT_EQ(lookup_me64, magic64);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_TEXT:
			FORKEE_ASSERT(memcmp(&magic, bytes_transfer_dummy,
			                     sizeof(magic)) == 0);
			break;
		case BYTES_TRANSFER_TEXTIO:
			switch (size) {
			case 8:
				FORKEE_ASSERT(memcmp(&magic8,
				                     bytes_transfer_dummy,
				                     sizeof(magic8)) == 0);
				break;
			case 16:
				FORKEE_ASSERT(memcmp(&magic16,
				                     bytes_transfer_dummy,
				                     sizeof(magic16)) == 0);
				break;
			case 32:
				FORKEE_ASSERT(memcmp(&magic32,
				                     bytes_transfer_dummy,
				                     sizeof(magic32)) == 0);
				break;
			case 64:
				FORKEE_ASSERT(memcmp(&magic64,
				                     bytes_transfer_dummy,
				                     sizeof(magic64)) == 0);
				break;
			}
			break;
		default:
			break;
		}

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	/* Check PaX MPROTECT */
	if (!can_we_write_to_text(child)) {
		switch (type) {
		case BYTES_TRANSFER_TEXTIO:
			switch (operation) {
			case PIOD_WRITE_D:
			case PIOD_WRITE_I:
				skip = true;
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_TEXT:
			switch (operation) {
			case PT_WRITE_D:
			case PT_WRITE_I:
				skip = true;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	/* Bailout cleanly killing the child process */
	if (skip) {
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, (void *)1, 0) != -1);
		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		                      child);

		validate_status_signaled(status, SIGKILL, 0);

		atf_tc_skip("PaX MPROTECT setup prevents writes to .text");
	}

	DPRINTF("Calling operation to transfer bytes between child=%d and "
	       "parent=%d\n", child, getpid());

	switch (type) {
	case BYTES_TRANSFER_TEXTIO:
	case BYTES_TRANSFER_DATAIO:
	case BYTES_TRANSFER_AUXV:
		switch (operation) {
		case PIOD_WRITE_D:
		case PIOD_WRITE_I:
			switch (size) {
			case 8:
				lookup_me8 = magic8;
				break;
			case 16:
				lookup_me16 = magic16;
				break;
			case 32:
				lookup_me32 = magic32;
				break;
			case 64:
				lookup_me64 = magic64;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);
		switch (operation) {
		case PIOD_READ_D:
		case PIOD_READ_I:
			switch (size) {
			case 8:
				ATF_REQUIRE_EQ(lookup_me8, magic8);
				break;
			case 16:
				ATF_REQUIRE_EQ(lookup_me16, magic16);
				break;
			case 32:
				ATF_REQUIRE_EQ(lookup_me32, magic32);
				break;
			case 64:
				ATF_REQUIRE_EQ(lookup_me64, magic64);
				break;
			default:
				break;
			}
			break;
		case PIOD_READ_AUXV:
			DPRINTF("Asserting that AUXV length (%zu) is > 0\n",
			        io.piod_len);
			ATF_REQUIRE(io.piod_len > 0);
			for (aip = ai; aip->a_type != AT_NULL; aip++)
				DPRINTF("a_type=%#llx a_v=%#llx\n",
				    (long long int)aip->a_type,
				    (long long int)aip->a_v);
			break;
		default:
			break;
		}
		break;
	case BYTES_TRANSFER_TEXT:
		switch (operation) {
		case PT_READ_D:
		case PT_READ_I:
			errno = 0;
			lookup_me = ptrace(operation, child,
			                   bytes_transfer_dummy, 0);
			ATF_REQUIRE_EQ(lookup_me, magic);
			SYSCALL_REQUIRE_ERRNO(errno, 0);
			break;
		case PT_WRITE_D:
		case PT_WRITE_I:
			SYSCALL_REQUIRE(ptrace(operation, child,
			                       bytes_transfer_dummy, magic)
			                != -1);
			break;
		default:
			break;
		}
		break;
	case BYTES_TRANSFER_DATA:
		switch (operation) {
		case PT_READ_D:
		case PT_READ_I:
			errno = 0;
			lookup_me = ptrace(operation, child, &lookup_me, 0);
			ATF_REQUIRE_EQ(lookup_me, magic);
			SYSCALL_REQUIRE_ERRNO(errno, 0);
			break;
		case PT_WRITE_D:
		case PT_WRITE_I:
			lookup_me = magic;
			SYSCALL_REQUIRE(ptrace(operation, child, &lookup_me,
			                       magic) != -1);
			break;
		default:
			break;
		}
		break;
	default:
		break;
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

#define BYTES_TRANSFER(test, operation, size, type)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify bytes transfer operation" #operation " and size " #size \
	    " of type " #type);						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	bytes_transfer(operation, size, BYTES_TRANSFER_##type);		\
}

// DATA

BYTES_TRANSFER(bytes_transfer_piod_read_d_8, PIOD_READ_D, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_16, PIOD_READ_D, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_32, PIOD_READ_D, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_64, PIOD_READ_D, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_piod_read_i_8, PIOD_READ_I, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_16, PIOD_READ_I, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_32, PIOD_READ_I, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_64, PIOD_READ_I, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_piod_write_d_8, PIOD_WRITE_D, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_16, PIOD_WRITE_D, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_32, PIOD_WRITE_D, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_64, PIOD_WRITE_D, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_piod_write_i_8, PIOD_WRITE_I, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_16, PIOD_WRITE_I, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_32, PIOD_WRITE_I, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_64, PIOD_WRITE_I, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_read_d, PT_READ_D, 32, DATA)
BYTES_TRANSFER(bytes_transfer_read_i, PT_READ_I, 32, DATA)
BYTES_TRANSFER(bytes_transfer_write_d, PT_WRITE_D, 32, DATA)
BYTES_TRANSFER(bytes_transfer_write_i, PT_WRITE_I, 32, DATA)

// TEXT

BYTES_TRANSFER(bytes_transfer_piod_read_d_8_text, PIOD_READ_D, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_16_text, PIOD_READ_D, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_32_text, PIOD_READ_D, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_64_text, PIOD_READ_D, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_piod_read_i_8_text, PIOD_READ_I, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_16_text, PIOD_READ_I, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_32_text, PIOD_READ_I, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_64_text, PIOD_READ_I, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_piod_write_d_8_text, PIOD_WRITE_D, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_16_text, PIOD_WRITE_D, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_32_text, PIOD_WRITE_D, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_64_text, PIOD_WRITE_D, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_piod_write_i_8_text, PIOD_WRITE_I, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_16_text, PIOD_WRITE_I, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_32_text, PIOD_WRITE_I, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_64_text, PIOD_WRITE_I, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_read_d_text, PT_READ_D, 32, TEXT)
BYTES_TRANSFER(bytes_transfer_read_i_text, PT_READ_I, 32, TEXT)
BYTES_TRANSFER(bytes_transfer_write_d_text, PT_WRITE_D, 32, TEXT)
BYTES_TRANSFER(bytes_transfer_write_i_text, PT_WRITE_I, 32, TEXT)

// AUXV

BYTES_TRANSFER(bytes_transfer_piod_read_auxv, PIOD_READ_AUXV, 4096, AUXV)

/// ----------------------------------------------------------------------------

static void
bytes_transfer_alignment(const char *operation)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	char *buffer;
	int vector;
	size_t len;
	size_t i;
	int op;

	struct ptrace_io_desc io;
	struct ptrace_siginfo info;

	memset(&io, 0, sizeof(io));
	memset(&info, 0, sizeof(info));

	/* Testing misaligned byte transfer crossing page boundaries */
	len = sysconf(_SC_PAGESIZE) * 2;
	buffer = malloc(len);
	ATF_REQUIRE(buffer != NULL);

	/* Initialize the buffer with random data */
	for (i = 0; i < len; i++)
		buffer[i] = i & 0xff;

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

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
		!= -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		"si_errno=%#x\n",
		info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	if (strcmp(operation, "PT_READ_I") == 0 ||
	    strcmp(operation, "PT_READ_D") == 0) {
		if (strcmp(operation, "PT_READ_I"))
			op = PT_READ_I;
		else
			op = PT_READ_D;

		for (i = 0; i <= (len - sizeof(int)); i++) {
			errno = 0;
			vector = ptrace(op, child, buffer + i, 0);
			ATF_REQUIRE_EQ(errno, 0);
			ATF_REQUIRE(!memcmp(&vector, buffer + i, sizeof(int)));
		}
	} else if (strcmp(operation, "PT_WRITE_I") == 0 ||
	           strcmp(operation, "PT_WRITE_D") == 0) {
		if (strcmp(operation, "PT_WRITE_I"))
			op = PT_WRITE_I;
		else
			op = PT_WRITE_D;

		for (i = 0; i <= (len - sizeof(int)); i++) {
			memcpy(&vector, buffer + i, sizeof(int));
			SYSCALL_REQUIRE(ptrace(op, child, buffer + 1, vector)
			    != -1);
		}
	} else if (strcmp(operation, "PIOD_READ_I") == 0 ||
	           strcmp(operation, "PIOD_READ_D") == 0) {
		if (strcmp(operation, "PIOD_READ_I"))
			op = PIOD_READ_I;
		else
			op = PIOD_READ_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);

		for (i = 0; i <= (len - sizeof(int)); i++) {
			io.piod_offs = buffer + i;

			SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io))
			                != -1);
			ATF_REQUIRE(!memcmp(&vector, buffer + i, sizeof(int)));
		}
	} else if (strcmp(operation, "PIOD_WRITE_I") == 0 ||
	           strcmp(operation, "PIOD_WRITE_D") == 0) {
		if (strcmp(operation, "PIOD_WRITE_I"))
			op = PIOD_WRITE_I;
		else
			op = PIOD_WRITE_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);

		for (i = 0; i <= (len - sizeof(int)); i++) {
			io.piod_offs = buffer + i;

			SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io))
			                != -1);
		}
	} else if (strcmp(operation, "PIOD_READ_AUXV") == 0) {
		io.piod_op = PIOD_READ_AUXV;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);

		errno = 0;
		i = 0;
		/* Read the whole AUXV vector, it has no clear length */
		while (io.piod_len > 0) {
			io.piod_offs = (void *)(intptr_t)i;
			SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io))
			                != -1 || (io.piod_len == 0 && i > 0));
			++i;
		}
	}

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define BYTES_TRANSFER_ALIGNMENT(test, operation)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify bytes transfer for potentially misaligned "		\
	    "operation " operation);					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	bytes_transfer_alignment(operation);				\
}

BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_read_i, "PT_READ_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_read_d, "PT_READ_D")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_write_i, "PT_WRITE_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_write_d, "PT_WRITE_D")

BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_read_i, "PIOD_READ_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_read_d, "PIOD_READ_D")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_write_i, "PIOD_WRITE_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_write_d, "PIOD_WRITE_D")

BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_read_auxv, "PIOD_READ_AUXV")

/// ----------------------------------------------------------------------------

static void
bytes_transfer_eof(const char *operation)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	FILE *fp;
	char *p;
	int vector;
	int op;

	struct ptrace_io_desc io;
	struct ptrace_siginfo info;

	memset(&io, 0, sizeof(io));
	memset(&info, 0, sizeof(info));

	vector = 0;

	fp = tmpfile();
	ATF_REQUIRE(fp != NULL);

	p = mmap(0, 1, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileno(fp), 0);
	ATF_REQUIRE(p != MAP_FAILED);

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

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
		!= -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		"si_errno=%#x\n",
		info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	if (strcmp(operation, "PT_READ_I") == 0 ||
	    strcmp(operation, "PT_READ_D") == 0) {
		if (strcmp(operation, "PT_READ_I"))
			op = PT_READ_I;
		else
			op = PT_READ_D;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(op, child, p, 0) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	} else if (strcmp(operation, "PT_WRITE_I") == 0 ||
	           strcmp(operation, "PT_WRITE_D") == 0) {
		if (strcmp(operation, "PT_WRITE_I"))
			op = PT_WRITE_I;
		else
			op = PT_WRITE_D;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(op, child, p, vector) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	} else if (strcmp(operation, "PIOD_READ_I") == 0 ||
	           strcmp(operation, "PIOD_READ_D") == 0) {
		if (strcmp(operation, "PIOD_READ_I"))
			op = PIOD_READ_I;
		else
			op = PIOD_READ_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);
		io.piod_offs = p;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io)) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	} else if (strcmp(operation, "PIOD_WRITE_I") == 0 ||
	           strcmp(operation, "PIOD_WRITE_D") == 0) {
		if (strcmp(operation, "PIOD_WRITE_I"))
			op = PIOD_WRITE_I;
		else
			op = PIOD_WRITE_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);
		io.piod_offs = p;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io)) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	}

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define BYTES_TRANSFER_EOF(test, operation)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify bytes EOF byte transfer for the " operation		\
	    " operation");						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	bytes_transfer_eof(operation);					\
}

BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_read_i, "PT_READ_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_read_d, "PT_READ_D")
BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_write_i, "PT_WRITE_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_write_d, "PT_WRITE_D")

BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_read_i, "PIOD_READ_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_read_d, "PIOD_READ_D")
BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_write_i, "PIOD_WRITE_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_write_d, "PIOD_WRITE_D")

/// ----------------------------------------------------------------------------

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
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_signaled(status, SIGKILL, 0);

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

/// ----------------------------------------------------------------------------

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

/// ----------------------------------------------------------------------------

static void
ptrace_kill(const char *type)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 &&
		    "Child should be terminated by a signal from its parent");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before killing the child process with %s\n", type);
	if (strcmp(type, "ptrace(PT_KILL)") == 0) {
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, (void*)1, 0) != -1);
	} else if (strcmp(type, "kill(SIGKILL)") == 0) {
		kill(child, SIGKILL);
	} else if (strcmp(type, "killpg(SIGKILL)") == 0) {
		setpgid(child, 0);
		killpg(getpgid(child), SIGKILL);
	}

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define PTRACE_KILL(test, type)						\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
        atf_tc_set_md_var(tc, "descr",					\
            "Verify killing the child with " type);			\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
        ptrace_kill(type);						\
}

// PT_CONTINUE with SIGKILL is covered by traceme_sendsignal_simple1
PTRACE_KILL(kill1, "ptrace(PT_KILL)")
PTRACE_KILL(kill2, "kill(SIGKILL)")
PTRACE_KILL(kill3, "killpg(SIGKILL)")

/// ----------------------------------------------------------------------------

static int lwpinfo_thread_sigmask[] = {SIGXCPU, SIGPIPE, SIGALRM, SIGURG};

static pthread_mutex_t lwpinfo_thread_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t lwpinfo_thread_cnd = PTHREAD_COND_INITIALIZER;
static volatile size_t lwpinfo_thread_done;

static void *
lwpinfo_thread(void *arg)
{
	sigset_t s;
	volatile void **tcb;

	tcb = (volatile void **)arg;

	*tcb = _lwp_getprivate();
	DPRINTF("Storing tcb[] = %p from thread %d\n", *tcb, _lwp_self());

	pthread_setname_np(pthread_self(), "thread %d",
	    (void *)(intptr_t)_lwp_self());

	sigemptyset(&s);
	pthread_mutex_lock(&lwpinfo_thread_mtx);
	sigaddset(&s, lwpinfo_thread_sigmask[lwpinfo_thread_done]);
	lwpinfo_thread_done++;
	pthread_sigmask(SIG_BLOCK, &s, NULL);
	pthread_cond_signal(&lwpinfo_thread_cnd);
	pthread_mutex_unlock(&lwpinfo_thread_mtx);

	return infinite_thread(NULL);
}

static void
traceme_lwpinfo(const size_t threads, const char *iter)
{
	const int sigval = SIGSTOP;
	const int sigval2 = SIGINT;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_lwpinfo lwp = {0, 0};
	struct ptrace_lwpstatus lwpstatus = {0};
	struct ptrace_siginfo info;
	void *private;
	char *name;
	char namebuf[PL_LNAMELEN];
	volatile void *tcb[4];
	bool found;
	sigset_t s;

	/* Maximum number of supported threads in this test */
	pthread_t t[__arraycount(tcb) - 1];
	size_t n, m;
	int rv;
	size_t bytes_read;

	struct ptrace_io_desc io;
	sigset_t sigmask;

	ATF_REQUIRE(__arraycount(t) >= threads);
	memset(tcb, 0, sizeof(tcb));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		tcb[0] = _lwp_getprivate();
		DPRINTF("Storing tcb[0] = %p\n", tcb[0]);

		pthread_setname_np(pthread_self(), "thread %d",
		    (void *)(intptr_t)_lwp_self());

		sigemptyset(&s);
		sigaddset(&s, lwpinfo_thread_sigmask[lwpinfo_thread_done]);
		pthread_sigmask(SIG_BLOCK, &s, NULL);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		for (n = 0; n < threads; n++) {
			rv = pthread_create(&t[n], NULL, lwpinfo_thread,
			    &tcb[n + 1]);
			FORKEE_ASSERT(rv == 0);
		}

		pthread_mutex_lock(&lwpinfo_thread_mtx);
		while (lwpinfo_thread_done < threads) {
			pthread_cond_wait(&lwpinfo_thread_cnd,
			    &lwpinfo_thread_mtx);
		}
		pthread_mutex_unlock(&lwpinfo_thread_mtx);

		DPRINTF("Before raising %s from child\n", strsignal(sigval2));
		FORKEE_ASSERT(raise(sigval2) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "Not reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	if (strstr(iter, "LWPINFO") != NULL) {
		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
		SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &lwp, sizeof(lwp))
		    != -1);

		DPRINTF("Assert that there exists a single thread only\n");
		ATF_REQUIRE(lwp.pl_lwpid > 0);

		DPRINTF("Assert that lwp thread %d received event "
		    "PL_EVENT_SIGNAL\n", lwp.pl_lwpid);
		FORKEE_ASSERT_EQ(lwp.pl_event, PL_EVENT_SIGNAL);

		if (strstr(iter, "LWPSTATUS") != NULL) {
			DPRINTF("Before calling ptrace(2) with PT_LWPSTATUS "
			    "for child\n");
			lwpstatus.pl_lwpid = lwp.pl_lwpid;
			SYSCALL_REQUIRE(ptrace(PT_LWPSTATUS, child, &lwpstatus,
			    sizeof(lwpstatus)) != -1);
		}

		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
		SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &lwp, sizeof(lwp))
		    != -1);

		DPRINTF("Assert that there exists a single thread only\n");
		ATF_REQUIRE_EQ(lwp.pl_lwpid, 0);
	} else {
		DPRINTF("Before calling ptrace(2) with PT_LWPNEXT for child\n");
		SYSCALL_REQUIRE(ptrace(PT_LWPNEXT, child, &lwpstatus,
		    sizeof(lwpstatus)) != -1);

		DPRINTF("Assert that there exists a single thread only %d\n", lwpstatus.pl_lwpid);
		ATF_REQUIRE(lwpstatus.pl_lwpid > 0);

		DPRINTF("Before calling ptrace(2) with PT_LWPNEXT for child\n");
		SYSCALL_REQUIRE(ptrace(PT_LWPNEXT, child, &lwpstatus,
		    sizeof(lwpstatus)) != -1);

		DPRINTF("Assert that there exists a single thread only\n");
		ATF_REQUIRE_EQ(lwpstatus.pl_lwpid, 0);
	}

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval2);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval2);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	memset(&lwp, 0, sizeof(lwp));
	memset(&lwpstatus, 0, sizeof(lwpstatus));

	memset(&io, 0, sizeof(io));

	bytes_read = 0;
	io.piod_op = PIOD_READ_D;
	io.piod_len = sizeof(tcb);

	do {
		io.piod_addr = (char *)&tcb + bytes_read;
		io.piod_offs = io.piod_addr;

		rv = ptrace(PT_IO, child, &io, sizeof(io));
		ATF_REQUIRE(rv != -1 && io.piod_len != 0);

		bytes_read += io.piod_len;
		io.piod_len = sizeof(tcb) - bytes_read;
	} while (bytes_read < sizeof(tcb));

	for (n = 0; n <= threads; n++) {
		if (strstr(iter, "LWPINFO") != NULL) {
			DPRINTF("Before calling ptrace(2) with PT_LWPINFO for "
			    "child\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_LWPINFO, child, &lwp, sizeof(lwp)) != -1);
			DPRINTF("LWP=%d\n", lwp.pl_lwpid);

			DPRINTF("Assert that the thread exists\n");
			ATF_REQUIRE(lwp.pl_lwpid > 0);

			DPRINTF("Assert that lwp thread %d received expected "
			    "event\n", lwp.pl_lwpid);
			FORKEE_ASSERT_EQ(lwp.pl_event,
			    info.psi_lwpid == lwp.pl_lwpid ?
			    PL_EVENT_SIGNAL : PL_EVENT_NONE);

			if (strstr(iter, "LWPSTATUS") != NULL) {
				DPRINTF("Before calling ptrace(2) with "
				    "PT_LWPSTATUS for child\n");
				lwpstatus.pl_lwpid = lwp.pl_lwpid;
				SYSCALL_REQUIRE(ptrace(PT_LWPSTATUS, child,
				    &lwpstatus, sizeof(lwpstatus)) != -1);

				goto check_lwpstatus;
			}
		} else {
			DPRINTF("Before calling ptrace(2) with PT_LWPNEXT for "
			    "child\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_LWPNEXT, child, &lwpstatus,
			    sizeof(lwpstatus)) != -1);
			DPRINTF("LWP=%d\n", lwpstatus.pl_lwpid);

			DPRINTF("Assert that the thread exists\n");
			ATF_REQUIRE(lwpstatus.pl_lwpid > 0);

		check_lwpstatus:

			if (strstr(iter, "pl_sigmask") != NULL) {
				sigmask = lwpstatus.pl_sigmask;

				DPRINTF("Retrieved sigmask: "
				    "%02x%02x%02x%02x\n",
				    sigmask.__bits[0], sigmask.__bits[1],
				    sigmask.__bits[2], sigmask.__bits[3]);

				found = false;
				for (m = 0;
				     m < __arraycount(lwpinfo_thread_sigmask);
				     m++) {
					if (sigismember(&sigmask,
					    lwpinfo_thread_sigmask[m])) {
						found = true;
						lwpinfo_thread_sigmask[m] = 0;
						break;
					}
				}
				ATF_REQUIRE(found == true);
			} else if (strstr(iter, "pl_name") != NULL) {
				name = lwpstatus.pl_name;

				DPRINTF("Retrieved thread name: "
				    "%s\n", name);

				snprintf(namebuf, sizeof namebuf, "thread %d",
				    lwpstatus.pl_lwpid);

				ATF_REQUIRE(strcmp(name, namebuf) == 0);
			} else if (strstr(iter, "pl_private") != NULL) {
				private = lwpstatus.pl_private;

				DPRINTF("Retrieved thread private pointer: "
				    "%p\n", private);

				found = false;
				for (m = 0; m < __arraycount(tcb); m++) {
					DPRINTF("Comparing %p and %p\n",
					    private, tcb[m]);
					if (private == tcb[m]) {
						found = true;
						break;
					}
				}
				ATF_REQUIRE(found == true);
			}
		}
	}

	if (strstr(iter, "LWPINFO") != NULL) {
		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for "
		    "child\n");
		SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &lwp, sizeof(lwp))
		    != -1);
		DPRINTF("LWP=%d\n", lwp.pl_lwpid);

		DPRINTF("Assert that there are no more threads\n");
		ATF_REQUIRE_EQ(lwp.pl_lwpid, 0);
	} else {
		DPRINTF("Before calling ptrace(2) with PT_LWPNEXT for child\n");
		SYSCALL_REQUIRE(ptrace(PT_LWPNEXT, child, &lwpstatus,
		    sizeof(lwpstatus)) != -1);

		DPRINTF("Assert that there exists a single thread only\n");
		ATF_REQUIRE_EQ(lwpstatus.pl_lwpid, 0);
	}

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, SIGKILL) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_LWPINFO(test, threads, iter)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify " iter " with the child with " #threads		\
	    " spawned extra threads");					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_lwpinfo(threads, iter);					\
}

TRACEME_LWPINFO(traceme_lwpinfo0, 0, "LWPINFO")
TRACEME_LWPINFO(traceme_lwpinfo1, 1, "LWPINFO")
TRACEME_LWPINFO(traceme_lwpinfo2, 2, "LWPINFO")
TRACEME_LWPINFO(traceme_lwpinfo3, 3, "LWPINFO")

TRACEME_LWPINFO(traceme_lwpinfo0_lwpstatus, 0, "LWPINFO+LWPSTATUS")
TRACEME_LWPINFO(traceme_lwpinfo1_lwpstatus, 1, "LWPINFO+LWPSTATUS")
TRACEME_LWPINFO(traceme_lwpinfo2_lwpstatus, 2, "LWPINFO+LWPSTATUS")
TRACEME_LWPINFO(traceme_lwpinfo3_lwpstatus, 3, "LWPINFO+LWPSTATUS")

TRACEME_LWPINFO(traceme_lwpinfo0_lwpstatus_pl_sigmask, 0,
    "LWPINFO+LWPSTATUS+pl_sigmask")
TRACEME_LWPINFO(traceme_lwpinfo1_lwpstatus_pl_sigmask, 1,
    "LWPINFO+LWPSTATUS+pl_sigmask")
TRACEME_LWPINFO(traceme_lwpinfo2_lwpstatus_pl_sigmask, 2,
    "LWPINFO+LWPSTATUS+pl_sigmask")
TRACEME_LWPINFO(traceme_lwpinfo3_lwpstatus_pl_sigmask, 3,
    "LWPINFO+LWPSTATUS+pl_sigmask")

TRACEME_LWPINFO(traceme_lwpinfo0_lwpstatus_pl_name, 0,
    "LWPINFO+LWPSTATUS+pl_name")
TRACEME_LWPINFO(traceme_lwpinfo1_lwpstatus_pl_name, 1,
    "LWPINFO+LWPSTATUS+pl_name")
TRACEME_LWPINFO(traceme_lwpinfo2_lwpstatus_pl_name, 2,
    "LWPINFO+LWPSTATUS+pl_name")
TRACEME_LWPINFO(traceme_lwpinfo3_lwpstatus_pl_name, 3,
    "LWPINFO+LWPSTATUS+pl_name")

TRACEME_LWPINFO(traceme_lwpinfo0_lwpstatus_pl_private, 0,
    "LWPINFO+LWPSTATUS+pl_private")
TRACEME_LWPINFO(traceme_lwpinfo1_lwpstatus_pl_private, 1,
    "LWPINFO+LWPSTATUS+pl_private")
TRACEME_LWPINFO(traceme_lwpinfo2_lwpstatus_pl_private, 2,
    "LWPINFO+LWPSTATUS+pl_private")
TRACEME_LWPINFO(traceme_lwpinfo3_lwpstatus_pl_private, 3,
    "LWPINFO+LWPSTATUS+pl_private")

TRACEME_LWPINFO(traceme_lwpnext0, 0, "LWPNEXT")
TRACEME_LWPINFO(traceme_lwpnext1, 1, "LWPNEXT")
TRACEME_LWPINFO(traceme_lwpnext2, 2, "LWPNEXT")
TRACEME_LWPINFO(traceme_lwpnext3, 3, "LWPNEXT")

TRACEME_LWPINFO(traceme_lwpnext0_pl_sigmask, 0, "LWPNEXT+pl_sigmask")
TRACEME_LWPINFO(traceme_lwpnext1_pl_sigmask, 1, "LWPNEXT+pl_sigmask")
TRACEME_LWPINFO(traceme_lwpnext2_pl_sigmask, 2, "LWPNEXT+pl_sigmask")
TRACEME_LWPINFO(traceme_lwpnext3_pl_sigmask, 3, "LWPNEXT+pl_sigmask")

TRACEME_LWPINFO(traceme_lwpnext0_pl_name, 0, "LWPNEXT+pl_name")
TRACEME_LWPINFO(traceme_lwpnext1_pl_name, 1, "LWPNEXT+pl_name")
TRACEME_LWPINFO(traceme_lwpnext2_pl_name, 2, "LWPNEXT+pl_name")
TRACEME_LWPINFO(traceme_lwpnext3_pl_name, 3, "LWPNEXT+pl_name")

TRACEME_LWPINFO(traceme_lwpnext0_pl_private, 0, "LWPNEXT+pl_private")
TRACEME_LWPINFO(traceme_lwpnext1_pl_private, 1, "LWPNEXT+pl_private")
TRACEME_LWPINFO(traceme_lwpnext2_pl_private, 2, "LWPNEXT+pl_private")
TRACEME_LWPINFO(traceme_lwpnext3_pl_private, 3, "LWPNEXT+pl_private")

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
attach_lwpinfo(const int threads)
{
	const int sigval = SIGINT;
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval_tracer = 10;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_lwpinfo lwp = {0, 0};
	struct ptrace_siginfo info;

	/* Maximum number of supported threads in this test */
	pthread_t t[3];
	int n, rv;

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		/* Wait for message from the parent */
		CHILD_TO_PARENT("tracee ready", parent_tracee, msg);

		CHILD_FROM_PARENT("spawn threads", parent_tracee, msg);

		for (n = 0; n < threads; n++) {
			rv = pthread_create(&t[n], NULL, infinite_thread, NULL);
			FORKEE_ASSERT(rv == 0);
		}

		CHILD_TO_PARENT("tracee exit", parent_tracee, msg);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "Not reached");
	}
	PARENT_FROM_CHILD("tracee ready", parent_tracee, msg);

	DPRINTF("Spawn debugger\n");
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* No IPC to communicate with the child */
		DPRINTF("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "tracee");
		FORKEE_ASSERT(
		    ptrace(PT_GET_SIGINFO, tracee, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n",
		    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		    info.psi_siginfo.si_errno);

		FORKEE_ASSERT_EQ(info.psi_siginfo.si_signo, SIGSTOP);
		FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, SI_USER);

		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
		FORKEE_ASSERT(ptrace(PT_LWPINFO, tracee, &lwp, sizeof(lwp))
		    != -1);

		DPRINTF("Assert that there exists a thread\n");
		FORKEE_ASSERTX(lwp.pl_lwpid > 0);

		DPRINTF("Assert that lwp thread %d received event "
		    "PL_EVENT_SIGNAL\n", lwp.pl_lwpid);
		FORKEE_ASSERT_EQ(lwp.pl_event, PL_EVENT_SIGNAL);

		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for "
		    "tracee\n");
		FORKEE_ASSERT(ptrace(PT_LWPINFO, tracee, &lwp, sizeof(lwp))
		    != -1);

		DPRINTF("Assert that there are no more lwp threads in "
		    "tracee\n");
		FORKEE_ASSERT_EQ(lwp.pl_lwpid, 0);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		CHILD_TO_PARENT("tracer ready", parent_tracer, msg);

		/* Wait for parent */
		CHILD_FROM_PARENT("tracer wait", parent_tracer, msg);

		/* Wait for tracee and assert that it raised a signal */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGINT);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "child");
		FORKEE_ASSERT(
		    ptrace(PT_GET_SIGINFO, tracee, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n",
		    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		    info.psi_siginfo.si_errno);

		FORKEE_ASSERT_EQ(info.psi_siginfo.si_signo, sigval);
		FORKEE_ASSERT_EQ(info.psi_siginfo.si_code, SI_LWP);

		memset(&lwp, 0, sizeof(lwp));

		for (n = 0; n <= threads; n++) {
			DPRINTF("Before calling ptrace(2) with PT_LWPINFO for "
			    "child\n");
			FORKEE_ASSERT(ptrace(PT_LWPINFO, tracee, &lwp,
			    sizeof(lwp)) != -1);
			DPRINTF("LWP=%d\n", lwp.pl_lwpid);

			DPRINTF("Assert that the thread exists\n");
			FORKEE_ASSERT(lwp.pl_lwpid > 0);

			DPRINTF("Assert that lwp thread %d received expected "
			    "event\n", lwp.pl_lwpid);
			FORKEE_ASSERT_EQ(lwp.pl_event,
			    info.psi_lwpid == lwp.pl_lwpid ?
			    PL_EVENT_SIGNAL : PL_EVENT_NONE);
		}
		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for "
		    "tracee\n");
		FORKEE_ASSERT(ptrace(PT_LWPINFO, tracee, &lwp, sizeof(lwp))
		    != -1);
		DPRINTF("LWP=%d\n", lwp.pl_lwpid);

		DPRINTF("Assert that there are no more threads\n");
		FORKEE_ASSERT_EQ(lwp.pl_lwpid, 0);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, SIGKILL)
		    != -1);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_signaled(status, SIGKILL, 0);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("tracer ready", parent_tracer, msg);

	DPRINTF("Resume the tracee and spawn threads\n");
	PARENT_TO_CHILD("spawn threads", parent_tracee, msg);

	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_FROM_CHILD("tracee exit", parent_tracee, msg);

	DPRINTF("Resume the tracer and let it detect multiple threads\n");
	PARENT_TO_CHILD("tracer wait", parent_tracer, msg);

	DPRINTF("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_signaled(status, SIGKILL, 0);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}

#define ATTACH_LWPINFO(test, threads)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify LWPINFO with the child with " #threads		\
	    " spawned extra threads (tracer is not the original "	\
	    "parent)");							\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	attach_lwpinfo(threads);					\
}

ATTACH_LWPINFO(attach_lwpinfo0, 0)
ATTACH_LWPINFO(attach_lwpinfo1, 1)
ATTACH_LWPINFO(attach_lwpinfo2, 2)
ATTACH_LWPINFO(attach_lwpinfo3, 3)
#endif

/// ----------------------------------------------------------------------------

static void
ptrace_siginfo(bool faked, void (*sah)(int a, siginfo_t *b, void *c), int *signal_caught)
{
	const int exitval = 5;
	const int sigval = SIGINT;
	const int sigfaked = SIGTRAP;
	const int sicodefaked = TRAP_BRKPT;
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

		sa.sa_sigaction = sah;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(faked ? sigfaked : sigval, &sa, NULL)
		    != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(*signal_caught, 1);

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

	if (faked) {
		DPRINTF("Before setting new faked signal to signo=%d "
		    "si_code=%d\n", sigfaked, sicodefaked);
		info.psi_siginfo.si_signo = sigfaked;
		info.psi_siginfo.si_code = sicodefaked;
	}

	DPRINTF("Before calling ptrace(2) with PT_SET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_SET_SIGINFO, child, &info, sizeof(info)) != -1);

	if (faked) {
		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "child\n");
		SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info,
		    sizeof(info)) != -1);

		DPRINTF("Before checking siginfo_t\n");
		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigfaked);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, sicodefaked);
	}

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1,
	    faked ? sigfaked : sigval) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define PTRACE_SIGINFO(test, faked)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify basic PT_GET_SIGINFO and PT_SET_SIGINFO calls"	\
	    "with%s setting signal to new value", faked ? "" : "out");	\
}									\
									\
static int test##_caught = 0;						\
									\
static void								\
test##_sighandler(int sig, siginfo_t *info, void *ctx)			\
{									\
	if (faked) {							\
		FORKEE_ASSERT_EQ(sig, SIGTRAP);				\
		FORKEE_ASSERT_EQ(info->si_signo, SIGTRAP);		\
		FORKEE_ASSERT_EQ(info->si_code, TRAP_BRKPT);		\
	} else {							\
		FORKEE_ASSERT_EQ(sig, SIGINT);				\
		FORKEE_ASSERT_EQ(info->si_signo, SIGINT);		\
		FORKEE_ASSERT_EQ(info->si_code, SI_LWP);		\
	}								\
									\
	++ test##_caught;						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	ptrace_siginfo(faked, test##_sighandler, & test##_caught); 	\
}

PTRACE_SIGINFO(siginfo_set_unmodified, false)
PTRACE_SIGINFO(siginfo_set_faked, true)

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

/// ----------------------------------------------------------------------------

#define TRACE_THREADS_NUM 100

static volatile int done;
pthread_mutex_t trace_threads_mtx = PTHREAD_MUTEX_INITIALIZER;

static void *
trace_threads_cb(void *arg __unused)
{

	pthread_mutex_lock(&trace_threads_mtx);
	done++;
	pthread_mutex_unlock(&trace_threads_mtx);

	while (done < TRACE_THREADS_NUM)
		sched_yield();

	return NULL;
}

static void
trace_threads(bool trace_create, bool trace_exit, bool masked)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	struct ptrace_siginfo info;

	sigset_t intmask;

	pthread_t t[TRACE_THREADS_NUM];
	int rv;
	size_t n;
	lwpid_t lid;

	/* Track created and exited threads */
	struct lwp_event_count traced_lwps[__arraycount(t)] = {{0, 0}};

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

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		for (n = 0; n < __arraycount(t); n++) {
			rv = pthread_create(&t[n], NULL, trace_threads_cb,
			    NULL);
			FORKEE_ASSERT(rv == 0);
		}

		for (n = 0; n < __arraycount(t); n++) {
			rv = pthread_join(t[n], NULL);
			FORKEE_ASSERT(rv == 0);
		}

		/*
		 * There is race between _exit() and pthread_join() detaching
		 * a thread. For simplicity kill the process after detecting
		 * LWP events.
		 */
		while (true)
			continue;

		FORKEE_ASSERT(0 && "Not reached");
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

	DPRINTF("Set LWP event mask for the child %d\n", child);
	memset(&event, 0, sizeof(event));
	if (trace_create)
		event.pe_set_event |= PTRACE_LWP_CREATE;
	if (trace_exit)
		event.pe_set_event |= PTRACE_LWP_EXIT;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	for (n = 0; n < (trace_create ? __arraycount(t) : 0); n++) {
		DPRINTF("Before calling %s() for the child - expected stopped "
		    "SIGTRAP\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_stopped(status, SIGTRAP);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "child\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n",
		    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		    info.psi_siginfo.si_errno);

		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_LWP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

		ATF_REQUIRE_EQ_MSG(state.pe_report_event, PTRACE_LWP_CREATE,
		    "%d != %d", state.pe_report_event, PTRACE_LWP_CREATE);

		lid = state.pe_lwp;
		DPRINTF("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

		*FIND_EVENT_COUNT(traced_lwps, lid) += 1;

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

	for (n = 0; n < (trace_exit ? __arraycount(t) : 0); n++) {
		DPRINTF("Before calling %s() for the child - expected stopped "
		    "SIGTRAP\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_stopped(status, SIGTRAP);

		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "child\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

		DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
		DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		    "si_errno=%#x\n",
		    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		    info.psi_siginfo.si_errno);

		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_LWP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

		ATF_REQUIRE_EQ_MSG(state.pe_report_event, PTRACE_LWP_EXIT,
		    "%d != %d", state.pe_report_event, PTRACE_LWP_EXIT);

		lid = state.pe_lwp;
		DPRINTF("Reported PTRACE_LWP_EXIT event with lid %d\n", lid);

		if (trace_create) {
			int *count = FIND_EVENT_COUNT(traced_lwps, lid);
			ATF_REQUIRE_EQ(*count, 1);
			*count = 0;
		}

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

	kill(child, SIGKILL);

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACE_THREADS(test, trace_create, trace_exit, mask)		\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
        atf_tc_set_md_var(tc, "descr",					\
            "Verify spawning threads with%s tracing LWP create and"	\
	    "with%s tracing LWP exit", trace_create ? "" : "out",	\
	    trace_exit ? "" : "out");					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
        trace_threads(trace_create, trace_exit, mask);			\
}

TRACE_THREADS(trace_thread_nolwpevents, false, false, false)
TRACE_THREADS(trace_thread_lwpexit, false, true, false)
TRACE_THREADS(trace_thread_lwpcreate, true, false, false)
TRACE_THREADS(trace_thread_lwpcreate_and_exit, true, true, false)

TRACE_THREADS(trace_thread_lwpexit_masked_sigtrap, false, true, true)
TRACE_THREADS(trace_thread_lwpcreate_masked_sigtrap, true, false, true)
TRACE_THREADS(trace_thread_lwpcreate_and_exit_masked_sigtrap, true, true, true)

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

#if defined(TWAIT_HAVE_PID)
static void
fork2_body(const char *fn, bool masked, bool ignored)
{
	const int exitval = 5;
	const int exitval2 = 0; /* Match exit status from /bin/echo */
	const int sigval = SIGSTOP;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	struct sigaction sa;
	struct ptrace_siginfo info;
	sigset_t intmask;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigmask;
	ki_sigset_t kp_sigignore;

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

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

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (strcmp(fn, "spawn") == 0) {
			FORKEE_ASSERT_EQ(posix_spawn(&child2,
			    arg[0], NULL, NULL, arg, NULL), 0);
		} else  {
			if (strcmp(fn, "fork") == 0) {
				FORKEE_ASSERT((child2 = fork()) != -1);
			} else {
				FORKEE_ASSERT((child2 = vfork()) != -1);
			}
			if (child2 == 0)
				_exit(exitval2);
		}

		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		forkee_status_exited(status, exitval2);

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

	DPRINTF("Set 0%s%s%s%s in EVENT_MASK for the child %d\n",
	    strcmp(fn, "spawn") == 0 ? "|PTRACE_POSIX_SPAWN" : "",
	    strcmp(fn, "fork") == 0 ? "|PTRACE_FORK" : "",
	    strcmp(fn, "vfork") == 0 ? "|PTRACE_VFORK" : "",
	    strcmp(fn, "vforkdone") == 0 ? "|PTRACE_VFORK_DONE" : "", child);
	event.pe_set_event = 0;
	if (strcmp(fn, "spawn") == 0)
		event.pe_set_event |= PTRACE_POSIX_SPAWN;
	if (strcmp(fn, "fork") == 0)
		event.pe_set_event |= PTRACE_FORK;
	if (strcmp(fn, "vfork") == 0)
		event.pe_set_event |= PTRACE_VFORK;
	if (strcmp(fn, "vforkdone") == 0)
		event.pe_set_event |= PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	if (strcmp(fn, "spawn") == 0 || strcmp(fn, "fork") == 0 ||
	    strcmp(fn, "vfork") == 0) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_stopped(status, SIGTRAP);

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

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		if (strcmp(fn, "spawn") == 0) {
			ATF_REQUIRE_EQ(
			    state.pe_report_event & PTRACE_POSIX_SPAWN,
			       PTRACE_POSIX_SPAWN);
		}
		if (strcmp(fn, "fork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (strcmp(fn, "vfork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		child2 = state.pe_other_pid;
		DPRINTF("Reported ptrace event with forkee %d\n", child2);

		DPRINTF("Before calling %s() for the forkee %d of the child "
		    "%d\n", TWAIT_FNAME, child2, child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
		    child2);

		validate_status_stopped(status, SIGTRAP);

		name[3] = child2;
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

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
		if (strcmp(fn, "spawn") == 0) {
			ATF_REQUIRE_EQ(
			    state.pe_report_event & PTRACE_POSIX_SPAWN,
			       PTRACE_POSIX_SPAWN);
		}
		if (strcmp(fn, "fork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (strcmp(fn, "vfork") == 0) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		ATF_REQUIRE_EQ(state.pe_other_pid, child);

		DPRINTF("Before resuming the forkee process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

	if (strcmp(fn, "vforkdone") == 0) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_stopped(status, SIGTRAP);

		name[3] = child;
		ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

		/*
		 * SIGCHLD is now pending in the signal queue and
		 * the kernel presents it to userland as a masked signal.
		 */
		sigdelset((sigset_t *)&kp.p_sigmask, SIGCHLD);

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

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

		child2 = state.pe_other_pid;
		DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
		    child2);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

	if (strcmp(fn, "spawn") == 0 || strcmp(fn, "fork") == 0 ||
	    strcmp(fn, "vfork") == 0) {
		DPRINTF("Before calling %s() for the forkee - expected exited"
		    "\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		validate_status_exited(status, exitval2);

		DPRINTF("Before calling %s() for the forkee - expected no "
		    "process\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD,
		    wpid = TWAIT_GENERIC(child2, &status, 0));
	}

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGCHLD\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGCHLD);

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

#define FORK2_TEST(name,fn,masked,ignored)				\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify that " fn " is caught "	\
	    "regardless of signal %s%s", 				\
	    masked ? "masked" : "", ignored ? "ignored" : "");		\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	fork2_body(fn, masked, ignored);				\
}

FORK2_TEST(posix_spawn_singalmasked, "spawn", true, false)
FORK2_TEST(posix_spawn_singalignored, "spawn", false, true)
FORK2_TEST(fork_singalmasked, "fork", true, false)
FORK2_TEST(fork_singalignored, "fork", false, true)
FORK2_TEST(vfork_singalmasked, "vfork", true, false)
FORK2_TEST(vfork_singalignored, "vfork", false, true)
FORK2_TEST(vforkdone_singalmasked, "vforkdone", true, false)
FORK2_TEST(vforkdone_singalignored, "vforkdone", false, true)
#endif

/// ----------------------------------------------------------------------------

static void *
thread_and_exec_thread_cb(void *arg __unused)
{

	execlp("/bin/echo", "/bin/echo", NULL);

	abort();
}

static void
threads_and_exec(void)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	struct ptrace_siginfo info;

	pthread_t t;
	lwpid_t lid;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT(pthread_create(&t, NULL,
		    thread_and_exec_thread_cb, NULL) == 0);

		for (;;)
			continue;

		FORKEE_ASSERT(0 && "Not reached");
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

	DPRINTF("Set LWP event mask for the child %d\n", child);
	memset(&event, 0, sizeof(event));
	event.pe_set_event |= PTRACE_LWP_CREATE | PTRACE_LWP_EXIT;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
	    "child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
	    "si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_LWP);

	SYSCALL_REQUIRE(
	    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ_MSG(state.pe_report_event, PTRACE_LWP_CREATE,
	    "%d != %d", state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	DPRINTF("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
	    "child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
	    "si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_LWP);

	SYSCALL_REQUIRE(
	    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ_MSG(state.pe_report_event, PTRACE_LWP_EXIT,
	    "%d != %d", state.pe_report_event, PTRACE_LWP_EXIT);

	lid = state.pe_lwp;
	DPRINTF("Reported PTRACE_LWP_EXIT event with lid %d\n", lid);

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
	    "child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
	    "si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_EXEC);

	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(threads_and_exec);
ATF_TC_HEAD(threads_and_exec, tc)
{
        atf_tc_set_md_var(tc, "descr",
            "Verify that multithreaded application on exec() will report "
	    "LWP_EXIT events");
}

ATF_TC_BODY(threads_and_exec, tc)
{

        threads_and_exec();
}

/// ----------------------------------------------------------------------------

ATF_TC(suspend_no_deadlock);
ATF_TC_HEAD(suspend_no_deadlock, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that the while the only thread within a process is "
	    "suspended, the whole process cannot be unstopped");
}

ATF_TC_BODY(suspend_no_deadlock, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo psi;

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

	DPRINTF("Before reading siginfo and lwpid_t\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &psi, sizeof(psi)) != -1);

	DPRINTF("Before suspending LWP %d\n", psi.psi_lwpid);
	SYSCALL_REQUIRE(ptrace(PT_SUSPEND, child, NULL, psi.psi_lwpid) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	ATF_REQUIRE_ERRNO(EDEADLK,
	    ptrace(PT_CONTINUE, child, (void *)1, 0) == -1);

	DPRINTF("Before resuming LWP %d\n", psi.psi_lwpid);
	SYSCALL_REQUIRE(ptrace(PT_RESUME, child, NULL, psi.psi_lwpid) != -1);

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

/// ----------------------------------------------------------------------------

static pthread_barrier_t barrier1_resume;
static pthread_barrier_t barrier2_resume;

static void *
resume_thread(void *arg)
{

	raise(SIGUSR1);

	pthread_barrier_wait(&barrier1_resume);

	/* Debugger will suspend the process here */

	pthread_barrier_wait(&barrier2_resume);

	raise(SIGUSR2);

	return infinite_thread(arg);
}

ATF_TC(resume);
ATF_TC_HEAD(resume, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that a thread can be suspended by a debugger and later "
	    "resumed by the debugger");
}

ATF_TC_BODY(resume, tc)
{
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	lwpid_t lid;
	struct ptrace_siginfo psi;
	pthread_t t;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		pthread_barrier_init(&barrier1_resume, NULL, 2);
		pthread_barrier_init(&barrier2_resume, NULL, 2);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before creating new thread in child\n");
		FORKEE_ASSERT(pthread_create(&t, NULL, resume_thread, NULL) == 0);

		pthread_barrier_wait(&barrier1_resume);

		pthread_barrier_wait(&barrier2_resume);

		infinite_thread(NULL);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGUSR1\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGUSR1);

	DPRINTF("Before reading siginfo and lwpid_t\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &psi, sizeof(psi)) != -1);

	DPRINTF("Before suspending LWP %d\n", psi.psi_lwpid);
	SYSCALL_REQUIRE(ptrace(PT_SUSPEND, child, NULL, psi.psi_lwpid) != -1);

	lid = psi.psi_lwpid;

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before suspending the parent for 1 second, we expect no signals\n");
	SYSCALL_REQUIRE(sleep(1) == 0);

#if defined(TWAIT_HAVE_OPTIONS)
	DPRINTF("Before calling %s() for the child - expected no status\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, WNOHANG), 0);
#endif

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_STOP, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGSTOP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGSTOP);

	DPRINTF("Before resuming LWP %d\n", lid);
	SYSCALL_REQUIRE(ptrace(PT_RESUME, child, NULL, lid) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGUSR2\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGUSR2);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_KILL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

/// ----------------------------------------------------------------------------

static int test_syscall_caught;

static void
syscall_sighand(int arg)
{

	DPRINTF("Caught a signal %d in process %d\n", arg, getpid());

	FORKEE_ASSERT_EQ(arg, SIGINFO);

	++test_syscall_caught;

	FORKEE_ASSERT_EQ(test_syscall_caught, 1);
}

static void
syscall_body(const char *op)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

#if defined(TWAIT_HAVE_STATUS)
	if (strstr(op, "signal") != NULL) {
		atf_tc_expect_fail("XXX: behavior under investigation");
	}
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		signal(SIGINFO, syscall_sighand);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		syscall(SYS_getpid);

		if (strstr(op, "signal") != NULL) {
			FORKEE_ASSERT_EQ(test_syscall_caught, 1);
		}

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t and lwpid\n");
	ATF_REQUIRE_EQ(info.psi_lwpid, 1);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_SCE);

	if (strstr(op, "killed") != NULL) {
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_signaled(status, SIGKILL, 0);
	} else {
		if (strstr(op, "signal") != NULL) {
			DPRINTF("Before resuming the child %d and sending a "
			    "signal SIGINFO\n", child);
			SYSCALL_REQUIRE(
			    ptrace(PT_CONTINUE, child, (void *)1, SIGINFO)
			    != -1);
		} else if (strstr(op, "detach") != NULL) {
			DPRINTF("Before detaching the child %d\n", child);
			SYSCALL_REQUIRE(
			    ptrace(PT_DETACH, child, (void *)1, 0) != -1);
		} else {
			DPRINTF("Before resuming the child process where it "
			    "left off and without signal to be sent\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

			DPRINTF("Before calling %s() for the child\n",
			    TWAIT_FNAME);
			TWAIT_REQUIRE_SUCCESS(
			    wpid = TWAIT_GENERIC(child, &status, 0), child);

			validate_status_stopped(status, SIGTRAP);

			DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO "
			    "for child\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
			    != -1);

			DPRINTF("Before checking siginfo_t and lwpid\n");
			ATF_REQUIRE_EQ(info.psi_lwpid, 1);
			ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
			ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_SCX);

			DPRINTF("Before resuming the child process where it "
			    "left off and without signal to be sent\n");
			SYSCALL_REQUIRE(
			    ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
		}

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_exited(status, exitval);
	}

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define SYSCALL_TEST(name,op)						\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify that getpid(2) can be traced with PT_SYSCALL %s",	\
	   #op );							\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	syscall_body(op);						\
}

SYSCALL_TEST(syscall, "")
SYSCALL_TEST(syscall_killed_on_sce, "and killed")
SYSCALL_TEST(syscall_signal_on_sce, "and signaled")
SYSCALL_TEST(syscall_detach_on_sce, "and detached")

/// ----------------------------------------------------------------------------

ATF_TC(syscallemu1);
ATF_TC_HEAD(syscallemu1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that exit(2) can be intercepted with PT_SYSCALLEMU");
}

ATF_TC_BODY(syscallemu1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

#if defined(__sparc__) && !defined(__sparc64__)
	/* syscallemu does not work on sparc (32-bit) */
	atf_tc_expect_fail("PR kern/52166");
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		syscall(SYS_exit, 100);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Set SYSCALLEMU for intercepted syscall\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALLEMU, child, (void *)1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_SYSCALL, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

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

static void
clone_body(int flags, bool trackfork, bool trackvfork,
    bool trackvforkdone)
{
	const int exitval = 5;
	const int exitval2 = 15;
	const int sigval = SIGSTOP;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	const size_t stack_size = 1024 * 1024;
	void *stack, *stack_base;

	stack = malloc(stack_size);
	ATF_REQUIRE(stack != NULL);

#ifdef __MACHINE_STACK_GROWS_UP
	stack_base = stack;
#else
	stack_base = (char *)stack + stack_size;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		SYSCALL_REQUIRE((child2 = __clone(clone_func, stack_base,
		    flags|SIGCHLD, (void *)(intptr_t)exitval2)) != -1);

		DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(),
		    child2);

		// XXX WALLSIG?
		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, WALLSIG), child2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set 0%s%s%s in EVENT_MASK for the child %d\n",
	    trackfork ? "|PTRACE_FORK" : "",
	    trackvfork ? "|PTRACE_VFORK" : "",
	    trackvforkdone ? "|PTRACE_VFORK_DONE" : "", child);
	event.pe_set_event = 0;
	if (trackfork)
		event.pe_set_event |= PTRACE_FORK;
	if (trackvfork)
		event.pe_set_event |= PTRACE_VFORK;
	if (trackvforkdone)
		event.pe_set_event |= PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

#if defined(TWAIT_HAVE_PID)
	if ((trackfork && !(flags & CLONE_VFORK)) ||
	    (trackvfork && (flags & CLONE_VFORK))) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		if (trackfork && !(flags & CLONE_VFORK)) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (trackvfork && (flags & CLONE_VFORK)) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		child2 = state.pe_other_pid;
		DPRINTF("Reported ptrace event with forkee %d\n", child2);

		DPRINTF("Before calling %s() for the forkee %d of the child "
		    "%d\n", TWAIT_FNAME, child2, child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
		    child2);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
		if (trackfork && !(flags & CLONE_VFORK)) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (trackvfork && (flags & CLONE_VFORK)) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		ATF_REQUIRE_EQ(state.pe_other_pid, child);

		DPRINTF("Before resuming the forkee process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(
		    ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}
#endif

	if (trackvforkdone && (flags & CLONE_VFORK)) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

		child2 = state.pe_other_pid;
		DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
		    child2);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

#if defined(TWAIT_HAVE_PID)
	if ((trackfork && !(flags & CLONE_VFORK)) ||
	    (trackvfork && (flags & CLONE_VFORK))) {
		DPRINTF("Before calling %s() for the forkee - expected exited"
		    "\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child2, &status, 0), child2);

		validate_status_exited(status, exitval2);

		DPRINTF("Before calling %s() for the forkee - expected no "
		    "process\n", TWAIT_FNAME);
		TWAIT_REQUIRE_FAILURE(ECHILD,
		    wpid = TWAIT_GENERIC(child2, &status, 0));
	}
#endif

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGCHLD\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGCHLD);

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

#define CLONE_TEST(name,flags,tfork,tvfork,tvforkdone)			\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify clone(%s) "		\
	    "called with 0%s%s%s in EVENT_MASK",			\
	    #flags,							\
	    tfork ? "|PTRACE_FORK" : "",				\
	    tvfork ? "|PTRACE_VFORK" : "",				\
	    tvforkdone ? "|PTRACE_VFORK_DONE" : "");			\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	clone_body(flags, tfork, tvfork, tvforkdone);			\
}

CLONE_TEST(clone1, 0, false, false, false)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone2, 0, true, false, false)
CLONE_TEST(clone3, 0, false, true, false)
CLONE_TEST(clone4, 0, true, true, false)
#endif
CLONE_TEST(clone5, 0, false, false, true)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone6, 0, true, false, true)
CLONE_TEST(clone7, 0, false, true, true)
CLONE_TEST(clone8, 0, true, true, true)
#endif

CLONE_TEST(clone_vm1, CLONE_VM, false, false, false)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_vm2, CLONE_VM, true, false, false)
CLONE_TEST(clone_vm3, CLONE_VM, false, true, false)
CLONE_TEST(clone_vm4, CLONE_VM, true, true, false)
#endif
CLONE_TEST(clone_vm5, CLONE_VM, false, false, true)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_vm6, CLONE_VM, true, false, true)
CLONE_TEST(clone_vm7, CLONE_VM, false, true, true)
CLONE_TEST(clone_vm8, CLONE_VM, true, true, true)
#endif

CLONE_TEST(clone_fs1, CLONE_FS, false, false, false)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_fs2, CLONE_FS, true, false, false)
CLONE_TEST(clone_fs3, CLONE_FS, false, true, false)
CLONE_TEST(clone_fs4, CLONE_FS, true, true, false)
#endif
CLONE_TEST(clone_fs5, CLONE_FS, false, false, true)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_fs6, CLONE_FS, true, false, true)
CLONE_TEST(clone_fs7, CLONE_FS, false, true, true)
CLONE_TEST(clone_fs8, CLONE_FS, true, true, true)
#endif

CLONE_TEST(clone_files1, CLONE_FILES, false, false, false)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_files2, CLONE_FILES, true, false, false)
CLONE_TEST(clone_files3, CLONE_FILES, false, true, false)
CLONE_TEST(clone_files4, CLONE_FILES, true, true, false)
#endif
CLONE_TEST(clone_files5, CLONE_FILES, false, false, true)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_files6, CLONE_FILES, true, false, true)
CLONE_TEST(clone_files7, CLONE_FILES, false, true, true)
CLONE_TEST(clone_files8, CLONE_FILES, true, true, true)
#endif

//CLONE_TEST(clone_sighand1, CLONE_SIGHAND, false, false, false)
#if defined(TWAIT_HAVE_PID)
//CLONE_TEST(clone_sighand2, CLONE_SIGHAND, true, false, false)
//CLONE_TEST(clone_sighand3, CLONE_SIGHAND, false, true, false)
//CLONE_TEST(clone_sighand4, CLONE_SIGHAND, true, true, false)
#endif
//CLONE_TEST(clone_sighand5, CLONE_SIGHAND, false, false, true)
#if defined(TWAIT_HAVE_PID)
//CLONE_TEST(clone_sighand6, CLONE_SIGHAND, true, false, true)
//CLONE_TEST(clone_sighand7, CLONE_SIGHAND, false, true, true)
//CLONE_TEST(clone_sighand8, CLONE_SIGHAND, true, true, true)
#endif

CLONE_TEST(clone_vfork1, CLONE_VFORK, false, false, false)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_vfork2, CLONE_VFORK, true, false, false)
CLONE_TEST(clone_vfork3, CLONE_VFORK, false, true, false)
CLONE_TEST(clone_vfork4, CLONE_VFORK, true, true, false)
#endif
CLONE_TEST(clone_vfork5, CLONE_VFORK, false, false, true)
#if defined(TWAIT_HAVE_PID)
CLONE_TEST(clone_vfork6, CLONE_VFORK, true, false, true)
CLONE_TEST(clone_vfork7, CLONE_VFORK, false, true, true)
CLONE_TEST(clone_vfork8, CLONE_VFORK, true, true, true)
#endif

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
clone_body2(int flags, bool masked, bool ignored)
{
	const int exitval = 5;
	const int exitval2 = 15;
	const int sigval = SIGSTOP;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	struct sigaction sa;
	struct ptrace_siginfo info;
	sigset_t intmask;
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);

	int name[6];
	const size_t namelen = __arraycount(name);
	ki_sigset_t kp_sigmask;
	ki_sigset_t kp_sigignore;

	const size_t stack_size = 1024 * 1024;
	void *stack, *stack_base;

	stack = malloc(stack_size);
	ATF_REQUIRE(stack != NULL);

#ifdef __MACHINE_STACK_GROWS_UP
	stack_base = stack;
#else
	stack_base = (char *)stack + stack_size;
#endif

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
		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before forking process PID=%d flags=%#x\n", getpid(),
		    flags);
		SYSCALL_REQUIRE((child2 = __clone(clone_func, stack_base,
		    flags|SIGCHLD, (void *)(intptr_t)exitval2)) != -1);

		DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(),
		    child2);

		// XXX WALLSIG?
		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, WALLSIG), child2);

		forkee_status_exited(status, exitval2);

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

	DPRINTF("Set PTRACE_FORK | PTRACE_VFORK | PTRACE_VFORK_DONE in "
	    "EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_FORK | PTRACE_VFORK | PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
	    child);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	validate_status_stopped(status, SIGTRAP);

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

	SYSCALL_REQUIRE(
	    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
	DPRINTF("state.pe_report_event=%#x pid=%d\n", state.pe_report_event,
	    child2);
	if (!(flags & CLONE_VFORK)) {
		ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
		       PTRACE_FORK);
	} else {
		ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
		       PTRACE_VFORK);
	}

	child2 = state.pe_other_pid;
	DPRINTF("Reported ptrace event with forkee %d\n", child2);

	DPRINTF("Before calling %s() for the forkee %d of the child "
	    "%d\n", TWAIT_FNAME, child2, child);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);

	validate_status_stopped(status, SIGTRAP);

	name[3] = child2;
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

	SYSCALL_REQUIRE(
	    ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
	if (!(flags & CLONE_VFORK)) {
		ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
		       PTRACE_FORK);
	} else {
		ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
		       PTRACE_VFORK);
	}

	ATF_REQUIRE_EQ(state.pe_other_pid, child);

	DPRINTF("Before resuming the forkee process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	if (flags & CLONE_VFORK) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		    child);
		TWAIT_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(child, &status, 0), child);

		validate_status_stopped(status, SIGTRAP);

		name[3] = child;
		ATF_REQUIRE_EQ(sysctl(name, namelen, &kp, &len, NULL, 0), 0);

		/*
		 * SIGCHLD is now pending in the signal queue and
		 * the kernel presents it to userland as a masked signal.
		 */
		sigdelset((sigset_t *)&kp.p_sigmask, SIGCHLD);

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

		SYSCALL_REQUIRE(
		    ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
		ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

		child2 = state.pe_other_pid;
		DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
		    child2);

		DPRINTF("Before resuming the child process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

	DPRINTF("Before calling %s() for the forkee - expected exited"
	    "\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(child2, &status, 0), child2);

	validate_status_exited(status, exitval2);

	DPRINTF("Before calling %s() for the forkee - expected no "
	    "process\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD,
	    wpid = TWAIT_GENERIC(child2, &status, 0));

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGCHLD\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGCHLD);

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

#define CLONE_TEST2(name,flags,masked,ignored)				\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify that clone(%s) is caught"\
	    " regardless of signal %s%s", 				\
	    #flags, masked ? "masked" : "", ignored ? "ignored" : "");	\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	clone_body2(flags, masked, ignored);				\
}

CLONE_TEST2(clone_signalignored, 0, true, false)
CLONE_TEST2(clone_signalmasked, 0, false, true)
CLONE_TEST2(clone_vm_signalignored, CLONE_VM, true, false)
CLONE_TEST2(clone_vm_signalmasked, CLONE_VM, false, true)
CLONE_TEST2(clone_fs_signalignored, CLONE_FS, true, false)
CLONE_TEST2(clone_fs_signalmasked, CLONE_FS, false, true)
CLONE_TEST2(clone_files_signalignored, CLONE_FILES, true, false)
CLONE_TEST2(clone_files_signalmasked, CLONE_FILES, false, true)
//CLONE_TEST2(clone_sighand_signalignored, CLONE_SIGHAND, true, false) // XXX
//CLONE_TEST2(clone_sighand_signalmasked, CLONE_SIGHAND, false, true)  // XXX
CLONE_TEST2(clone_vfork_signalignored, CLONE_VFORK, true, false)
CLONE_TEST2(clone_vfork_signalmasked, CLONE_VFORK, false, true)
#endif

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
traceme_vfork_clone_body(int flags)
{
	const int exitval = 5;
	const int exitval2 = 15;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	const size_t stack_size = 1024 * 1024;
	void *stack, *stack_base;

	stack = malloc(stack_size);
	ATF_REQUIRE(stack != NULL);

#ifdef __MACHINE_STACK_GROWS_UP
	stack_base = stack;
#else
	stack_base = (char *)stack + stack_size;
#endif

	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before forking process PID=%d flags=%#x\n", getpid(),
		    flags);
		SYSCALL_REQUIRE((child2 = __clone(clone_func, stack_base,
		    flags|SIGCHLD, (void *)(intptr_t)exitval2)) != -1);

		DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(),
		    child2);

		// XXX WALLSIG?
		FORKEE_REQUIRE_SUCCESS
		    (wpid = TWAIT_GENERIC(child2, &status, WALLSIG), child2);

		forkee_status_exited(status, exitval2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define TRACEME_VFORK_CLONE_TEST(name,flags)				\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify that clone(%s) is "	\
	    "handled correctly with vfork(2)ed tracer", 		\
	    #flags);							\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	traceme_vfork_clone_body(flags);				\
}

TRACEME_VFORK_CLONE_TEST(traceme_vfork_clone, 0)
TRACEME_VFORK_CLONE_TEST(traceme_vfork_clone_vm, CLONE_VM)
TRACEME_VFORK_CLONE_TEST(traceme_vfork_clone_fs, CLONE_FS)
TRACEME_VFORK_CLONE_TEST(traceme_vfork_clone_files, CLONE_FILES)
//TRACEME_VFORK_CLONE_TEST(traceme_vfork_clone_sighand, CLONE_SIGHAND)  // XXX
TRACEME_VFORK_CLONE_TEST(traceme_vfork_clone_vfork, CLONE_VFORK)
#endif

/// ----------------------------------------------------------------------------

static void
user_va0_disable(int operation)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	const int sigval = SIGSTOP;
	int rv;

	struct ptrace_siginfo info;

	if (get_user_va0_disable() == 0)
		atf_tc_skip("vm.user_va0_disable is set to 0");

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
		__unreachable();
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

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

	DPRINTF("Before resuming the child process in PC=0x0 "
	    "and without signal to be sent\n");
	errno = 0;
	rv = ptrace(operation, child, (void *)0, 0);
	ATF_REQUIRE_EQ(errno, EINVAL);
	ATF_REQUIRE_EQ(rv, -1);

	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);
	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define USER_VA0_DISABLE(test, operation)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify behavior of " #operation " with PC set to 0x0");	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	user_va0_disable(operation);					\
}

USER_VA0_DISABLE(user_va0_disable_pt_continue, PT_CONTINUE)
USER_VA0_DISABLE(user_va0_disable_pt_syscall, PT_SYSCALL)
USER_VA0_DISABLE(user_va0_disable_pt_detach, PT_DETACH)

/// ----------------------------------------------------------------------------

/*
 * Parse the core file and find the requested note.  If the reading or parsing
 * fails, the test is failed.  If the note is found, it is read onto buf, up to
 * buf_len.  The actual length of the note is returned (which can be greater
 * than buf_len, indicating that it has been truncated).  If the note is not
 * found, -1 is returned.
 */
static ssize_t core_find_note(const char *core_path,
    const char *note_name, uint64_t note_type, void *buf, size_t buf_len)
{
	int core_fd;
	Elf *core_elf;
	size_t core_numhdr, i;
	ssize_t ret = -1;
	/* note: we assume note name will be null-terminated */
	size_t name_len = strlen(note_name) + 1;

	SYSCALL_REQUIRE((core_fd = open(core_path, O_RDONLY)) != -1);
	SYSCALL_REQUIRE(elf_version(EV_CURRENT) != EV_NONE);
	SYSCALL_REQUIRE((core_elf = elf_begin(core_fd, ELF_C_READ, NULL)));

	SYSCALL_REQUIRE(elf_getphnum(core_elf, &core_numhdr) != 0);
	for (i = 0; i < core_numhdr && ret == -1; i++) {
		GElf_Phdr core_hdr;
		size_t offset;
		SYSCALL_REQUIRE(gelf_getphdr(core_elf, i, &core_hdr));
		if (core_hdr.p_type != PT_NOTE)
		    continue;

		for (offset = core_hdr.p_offset;
		    offset < core_hdr.p_offset + core_hdr.p_filesz;) {
			Elf64_Nhdr note_hdr;
			char name_buf[64];

			switch (gelf_getclass(core_elf)) {
			case ELFCLASS64:
				SYSCALL_REQUIRE(pread(core_fd, &note_hdr,
				    sizeof(note_hdr), offset)
				    == sizeof(note_hdr));
				offset += sizeof(note_hdr);
				break;
			case ELFCLASS32:
				{
				Elf32_Nhdr tmp_hdr;
				SYSCALL_REQUIRE(pread(core_fd, &tmp_hdr,
				    sizeof(tmp_hdr), offset)
				    == sizeof(tmp_hdr));
				offset += sizeof(tmp_hdr);
				note_hdr.n_namesz = tmp_hdr.n_namesz;
				note_hdr.n_descsz = tmp_hdr.n_descsz;
				note_hdr.n_type = tmp_hdr.n_type;
				}
				break;
			}

			/* indicates end of notes */
			if (note_hdr.n_namesz == 0 || note_hdr.n_descsz == 0)
				break;
			if (note_hdr.n_namesz == name_len &&
			    note_hdr.n_namesz <= sizeof(name_buf)) {
				SYSCALL_REQUIRE(pread(core_fd, name_buf,
				    note_hdr.n_namesz, offset)
				    == (ssize_t)(size_t)note_hdr.n_namesz);

				if (!strncmp(note_name, name_buf, name_len) &&
				    note_hdr.n_type == note_type)
					ret = note_hdr.n_descsz;
			}

			offset += note_hdr.n_namesz;
			/* fix to alignment */
			offset = roundup(offset, core_hdr.p_align);

			/* if name & type matched above */
			if (ret != -1) {
				ssize_t read_len = MIN(buf_len,
				    note_hdr.n_descsz);
				SYSCALL_REQUIRE(pread(core_fd, buf,
				    read_len, offset) == read_len);
				break;
			}

			offset += note_hdr.n_descsz;
			/* fix to alignment */
			offset = roundup(offset, core_hdr.p_align);
		}
	}

	elf_end(core_elf);
	close(core_fd);

	return ret;
}

ATF_TC(core_dump_procinfo);
ATF_TC_HEAD(core_dump_procinfo, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Trigger a core dump and verify its contents.");
}

ATF_TC_BODY(core_dump_procinfo, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	char core_path[] = "/tmp/core.XXXXXX";
	int core_fd;
	struct netbsd_elfcore_procinfo procinfo;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before triggering SIGTRAP\n");
		trigger_trap();

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	SYSCALL_REQUIRE((core_fd = mkstemp(core_path)) != -1);
	close(core_fd);

	DPRINTF("Call DUMPCORE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_DUMPCORE, child, core_path, strlen(core_path))
	    != -1);

	DPRINTF("Read core file\n");
	ATF_REQUIRE_EQ(core_find_note(core_path, "NetBSD-CORE",
	    ELF_NOTE_NETBSD_CORE_PROCINFO, &procinfo, sizeof(procinfo)),
	    sizeof(procinfo));

	ATF_CHECK_EQ(procinfo.cpi_version, 1);
	ATF_CHECK_EQ(procinfo.cpi_cpisize, sizeof(procinfo));
	ATF_CHECK_EQ(procinfo.cpi_signo, SIGTRAP);
	ATF_CHECK_EQ(procinfo.cpi_pid, child);
	ATF_CHECK_EQ(procinfo.cpi_ppid, getpid());
	ATF_CHECK_EQ(procinfo.cpi_pgrp, getpgid(child));
	ATF_CHECK_EQ(procinfo.cpi_sid, getsid(child));
	ATF_CHECK_EQ(procinfo.cpi_ruid, getuid());
	ATF_CHECK_EQ(procinfo.cpi_euid, geteuid());
	ATF_CHECK_EQ(procinfo.cpi_rgid, getgid());
	ATF_CHECK_EQ(procinfo.cpi_egid, getegid());
	ATF_CHECK_EQ(procinfo.cpi_nlwps, 1);
	ATF_CHECK_EQ(procinfo.cpi_siglwp, 1);

	unlink(core_path);

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

#if defined(TWAIT_HAVE_STATUS)

#define THREAD_CONCURRENT_BREAKPOINT_NUM 50
#define THREAD_CONCURRENT_SIGNALS_NUM 50
#define THREAD_CONCURRENT_WATCHPOINT_NUM 50

/* List of signals to use for the test */
const int thread_concurrent_signals_list[] = {
	SIGIO,
	SIGXCPU,
	SIGXFSZ,
	SIGVTALRM,
	SIGPROF,
	SIGWINCH,
	SIGINFO,
	SIGUSR1,
	SIGUSR2
};

enum thread_concurrent_signal_handling {
	/* the signal is discarded by debugger */
	TCSH_DISCARD,
	/* the handler is set to SIG_IGN */
	TCSH_SIG_IGN,
	/* an actual handler is used */
	TCSH_HANDLER
};

static pthread_barrier_t thread_concurrent_barrier;
static pthread_key_t thread_concurrent_key;
static uint32_t thread_concurrent_watchpoint_var = 0;

static void *
thread_concurrent_breakpoint_thread(void *arg)
{
	static volatile int watchme = 1;
	pthread_barrier_wait(&thread_concurrent_barrier);
	DPRINTF("Before entering breakpoint func from LWP %d\n", _lwp_self());
	check_happy(watchme);
	return NULL;
}

static void
thread_concurrent_sig_handler(int sig)
{
	void *tls_val = pthread_getspecific(thread_concurrent_key);
	DPRINTF("Before increment, LWP %d tls_val=%p\n", _lwp_self(), tls_val);
	FORKEE_ASSERT(pthread_setspecific(thread_concurrent_key,
	    (void*)((uintptr_t)tls_val + 1)) == 0);
}

static void *
thread_concurrent_signals_thread(void *arg)
{
	int sigval = thread_concurrent_signals_list[
	    _lwp_self() % __arraycount(thread_concurrent_signals_list)];
	enum thread_concurrent_signal_handling *signal_handle = arg;
	void *tls_val;

	pthread_barrier_wait(&thread_concurrent_barrier);
	DPRINTF("Before raising %s from LWP %d\n", strsignal(sigval),
		_lwp_self());
	pthread_kill(pthread_self(), sigval);
	if (*signal_handle == TCSH_HANDLER) {
	    tls_val = pthread_getspecific(thread_concurrent_key);
	    DPRINTF("After raising, LWP %d tls_val=%p\n", _lwp_self(), tls_val);
	    FORKEE_ASSERT(tls_val == (void*)1);
	}
	return NULL;
}

static void *
thread_concurrent_watchpoint_thread(void *arg)
{
	pthread_barrier_wait(&thread_concurrent_barrier);
	DPRINTF("Before modifying var from LWP %d\n", _lwp_self());
	thread_concurrent_watchpoint_var = 1;
	return NULL;
}

#if defined(__i386__) || defined(__x86_64__)
enum thread_concurrent_sigtrap_event {
	TCSE_UNKNOWN,
	TCSE_BREAKPOINT,
	TCSE_WATCHPOINT
};

static void
thread_concurrent_lwp_setup(pid_t child, lwpid_t lwpid);
static enum thread_concurrent_sigtrap_event
thread_concurrent_handle_sigtrap(pid_t child, ptrace_siginfo_t *info);
#endif

static void
thread_concurrent_test(enum thread_concurrent_signal_handling signal_handle,
    int breakpoint_threads, int signal_threads, int watchpoint_threads)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int status;
	struct lwp_event_count signal_counts[THREAD_CONCURRENT_SIGNALS_NUM]
	    = {{0, 0}};
	struct lwp_event_count bp_counts[THREAD_CONCURRENT_BREAKPOINT_NUM]
	    = {{0, 0}};
	struct lwp_event_count wp_counts[THREAD_CONCURRENT_BREAKPOINT_NUM]
	    = {{0, 0}};
	ptrace_event_t event;
	int i;

#if defined(HAVE_DBREGS)
	if (!can_we_set_dbregs()) {
		atf_tc_skip("Either run this test as root or set sysctl(3) "
		            "security.models.extensions.user_set_dbregs to 1");
        }
#endif

	atf_tc_skip("PR kern/54960");

	/* Protect against out-of-bounds array access. */
	ATF_REQUIRE(breakpoint_threads <= THREAD_CONCURRENT_BREAKPOINT_NUM);
	ATF_REQUIRE(signal_threads <= THREAD_CONCURRENT_SIGNALS_NUM);
	ATF_REQUIRE(watchpoint_threads <= THREAD_CONCURRENT_WATCHPOINT_NUM);

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		pthread_t bp_threads[THREAD_CONCURRENT_BREAKPOINT_NUM];
		pthread_t sig_threads[THREAD_CONCURRENT_SIGNALS_NUM];
		pthread_t wp_threads[THREAD_CONCURRENT_WATCHPOINT_NUM];

		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		if (signal_handle != TCSH_DISCARD) {
			struct sigaction sa;
			unsigned int j;

			memset(&sa, 0, sizeof(sa));
			if (signal_handle == TCSH_SIG_IGN)
				sa.sa_handler = SIG_IGN;
			else
				sa.sa_handler = thread_concurrent_sig_handler;
			sigemptyset(&sa.sa_mask);

			for (j = 0;
			    j < __arraycount(thread_concurrent_signals_list);
			    j++)
				FORKEE_ASSERT(sigaction(
				    thread_concurrent_signals_list[j], &sa, NULL)
				    != -1);
		}

		DPRINTF("Before starting threads from the child\n");
		FORKEE_ASSERT(pthread_barrier_init(
		    &thread_concurrent_barrier, NULL,
		    breakpoint_threads + signal_threads + watchpoint_threads)
		    == 0);
		FORKEE_ASSERT(pthread_key_create(&thread_concurrent_key, NULL)
		    == 0);

		for (i = 0; i < signal_threads; i++) {
			FORKEE_ASSERT(pthread_create(&sig_threads[i], NULL,
			    thread_concurrent_signals_thread,
			    &signal_handle) == 0);
		}
		for (i = 0; i < breakpoint_threads; i++) {
			FORKEE_ASSERT(pthread_create(&bp_threads[i], NULL,
			    thread_concurrent_breakpoint_thread, NULL) == 0);
		}
		for (i = 0; i < watchpoint_threads; i++) {
			FORKEE_ASSERT(pthread_create(&wp_threads[i], NULL,
			    thread_concurrent_watchpoint_thread, NULL) == 0);
		}

		DPRINTF("Before joining threads from the child\n");
		for (i = 0; i < watchpoint_threads; i++) {
			FORKEE_ASSERT(pthread_join(wp_threads[i], NULL) == 0);
		}
		for (i = 0; i < breakpoint_threads; i++) {
			FORKEE_ASSERT(pthread_join(bp_threads[i], NULL) == 0);
		}
		for (i = 0; i < signal_threads; i++) {
			FORKEE_ASSERT(pthread_join(sig_threads[i], NULL) == 0);
		}

		FORKEE_ASSERT(pthread_key_delete(thread_concurrent_key) == 0);
		FORKEE_ASSERT(pthread_barrier_destroy(
		    &thread_concurrent_barrier) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set LWP event mask for the child process\n");
	memset(&event, 0, sizeof(event));
	event.pe_set_event |= PTRACE_LWP_CREATE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, sizeof(event))
	    != -1);

	DPRINTF("Before resuming the child process where it left off\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before entering signal collection loop\n");
	while (1) {
		ptrace_siginfo_t info;

		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		    child);
		if (WIFEXITED(status))
			break;
		/* Note: we use validate_status_stopped() to get nice error
		 * message.  Signal is irrelevant since it won't be reached.
		 */
		else if (!WIFSTOPPED(status))
			validate_status_stopped(status, 0);

		DPRINTF("Before calling PT_GET_SIGINFO\n");
		SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info,
		    sizeof(info)) != -1);

		DPRINTF("Received signal %d from LWP %d (wait: %d)\n",
		    info.psi_siginfo.si_signo, info.psi_lwpid,
		    WSTOPSIG(status));

		ATF_CHECK_EQ_MSG(info.psi_siginfo.si_signo, WSTOPSIG(status),
		    "lwp=%d, WSTOPSIG=%d, psi_siginfo=%d", info.psi_lwpid,
		    WSTOPSIG(status), info.psi_siginfo.si_signo);

		if (WSTOPSIG(status) != SIGTRAP) {
			int expected_sig =
			    thread_concurrent_signals_list[info.psi_lwpid %
			    __arraycount(thread_concurrent_signals_list)];
			ATF_CHECK_EQ_MSG(WSTOPSIG(status), expected_sig,
				"lwp=%d, expected %d, got %d", info.psi_lwpid,
				expected_sig, WSTOPSIG(status));

			*FIND_EVENT_COUNT(signal_counts, info.psi_lwpid) += 1;
		} else if (info.psi_siginfo.si_code == TRAP_LWP) {
#if defined(__i386__) || defined(__x86_64__)
			thread_concurrent_lwp_setup(child, info.psi_lwpid);
#endif
		} else {
#if defined(__i386__) || defined(__x86_64__)
			switch (thread_concurrent_handle_sigtrap(child, &info)) {
				case TCSE_UNKNOWN:
					/* already reported inside the function */
					break;
				case TCSE_BREAKPOINT:
					*FIND_EVENT_COUNT(bp_counts,
					    info.psi_lwpid) += 1;
					break;
				case TCSE_WATCHPOINT:
					*FIND_EVENT_COUNT(wp_counts,
					    info.psi_lwpid) += 1;
					break;
			}
#else
			ATF_CHECK_MSG(0, "Unexpected SIGTRAP, si_code=%d\n",
			    info.psi_siginfo.si_code);
#endif
		}

		DPRINTF("Before resuming the child process\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1,
		     signal_handle != TCSH_DISCARD && WSTOPSIG(status) != SIGTRAP
		     ? WSTOPSIG(status) : 0) != -1);
	}

	for (i = 0; i < signal_threads; i++)
		ATF_CHECK_EQ_MSG(signal_counts[i].lec_count, 1,
		    "signal_counts[%d].lec_count=%d; lec_lwp=%d",
		    i, signal_counts[i].lec_count, signal_counts[i].lec_lwp);
	for (i = signal_threads; i < THREAD_CONCURRENT_SIGNALS_NUM; i++)
		ATF_CHECK_EQ_MSG(signal_counts[i].lec_count, 0,
		    "extraneous signal_counts[%d].lec_count=%d; lec_lwp=%d",
		    i, signal_counts[i].lec_count, signal_counts[i].lec_lwp);

	for (i = 0; i < breakpoint_threads; i++)
		ATF_CHECK_EQ_MSG(bp_counts[i].lec_count, 1,
		    "bp_counts[%d].lec_count=%d; lec_lwp=%d",
		    i, bp_counts[i].lec_count, bp_counts[i].lec_lwp);
	for (i = breakpoint_threads; i < THREAD_CONCURRENT_BREAKPOINT_NUM; i++)
		ATF_CHECK_EQ_MSG(bp_counts[i].lec_count, 0,
		    "extraneous bp_counts[%d].lec_count=%d; lec_lwp=%d",
		    i, bp_counts[i].lec_count, bp_counts[i].lec_lwp);

	for (i = 0; i < watchpoint_threads; i++)
		ATF_CHECK_EQ_MSG(wp_counts[i].lec_count, 1,
		    "wp_counts[%d].lec_count=%d; lec_lwp=%d",
		    i, wp_counts[i].lec_count, wp_counts[i].lec_lwp);
	for (i = watchpoint_threads; i < THREAD_CONCURRENT_WATCHPOINT_NUM; i++)
		ATF_CHECK_EQ_MSG(wp_counts[i].lec_count, 0,
		    "extraneous wp_counts[%d].lec_count=%d; lec_lwp=%d",
		    i, wp_counts[i].lec_count, wp_counts[i].lec_lwp);

	validate_status_exited(status, exitval);
}

#define THREAD_CONCURRENT_TEST(test, sig_hdl, bps, sigs, wps, descr)	\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", descr);				\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
	thread_concurrent_test(sig_hdl, bps, sigs, wps);		\
}

THREAD_CONCURRENT_TEST(thread_concurrent_signals, TCSH_DISCARD,
    0, THREAD_CONCURRENT_SIGNALS_NUM, 0,
    "Verify that concurrent signals issued to a single thread are reported "
    "correctly");
THREAD_CONCURRENT_TEST(thread_concurrent_signals_sig_ign, TCSH_SIG_IGN,
    0, THREAD_CONCURRENT_SIGNALS_NUM, 0,
    "Verify that concurrent signals issued to a single thread are reported "
    "correctly and passed back to SIG_IGN handler");
THREAD_CONCURRENT_TEST(thread_concurrent_signals_handler, TCSH_HANDLER,
    0, THREAD_CONCURRENT_SIGNALS_NUM, 0,
    "Verify that concurrent signals issued to a single thread are reported "
    "correctly and passed back to a handler function");

#if defined(__i386__) || defined(__x86_64__)
THREAD_CONCURRENT_TEST(thread_concurrent_breakpoints, TCSH_DISCARD,
    THREAD_CONCURRENT_BREAKPOINT_NUM, 0, 0,
    "Verify that concurrent breakpoints are reported correctly");
THREAD_CONCURRENT_TEST(thread_concurrent_watchpoints, TCSH_DISCARD,
    0, 0, THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent breakpoints are reported correctly");
THREAD_CONCURRENT_TEST(thread_concurrent_bp_wp, TCSH_DISCARD,
    THREAD_CONCURRENT_BREAKPOINT_NUM, 0, THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent breakpoints and watchpoints are reported "
    "correctly");

THREAD_CONCURRENT_TEST(thread_concurrent_bp_sig, TCSH_DISCARD,
    THREAD_CONCURRENT_BREAKPOINT_NUM, THREAD_CONCURRENT_SIGNALS_NUM, 0,
    "Verify that concurrent breakpoints and signals are reported correctly");
THREAD_CONCURRENT_TEST(thread_concurrent_bp_sig_sig_ign, TCSH_SIG_IGN,
    THREAD_CONCURRENT_BREAKPOINT_NUM, THREAD_CONCURRENT_SIGNALS_NUM, 0,
    "Verify that concurrent breakpoints and signals are reported correctly "
    "and passed back to SIG_IGN handler");
THREAD_CONCURRENT_TEST(thread_concurrent_bp_sig_handler, TCSH_HANDLER,
    THREAD_CONCURRENT_BREAKPOINT_NUM, THREAD_CONCURRENT_SIGNALS_NUM, 0,
    "Verify that concurrent breakpoints and signals are reported correctly "
    "and passed back to a handler function");

THREAD_CONCURRENT_TEST(thread_concurrent_wp_sig, TCSH_DISCARD,
    0, THREAD_CONCURRENT_SIGNALS_NUM, THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent watchpoints and signals are reported correctly");
THREAD_CONCURRENT_TEST(thread_concurrent_wp_sig_sig_ign, TCSH_SIG_IGN,
    0, THREAD_CONCURRENT_SIGNALS_NUM, THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent watchpoints and signals are reported correctly "
    "and passed back to SIG_IGN handler");
THREAD_CONCURRENT_TEST(thread_concurrent_wp_sig_handler, TCSH_HANDLER,
    0, THREAD_CONCURRENT_SIGNALS_NUM, THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent watchpoints and signals are reported correctly "
    "and passed back to a handler function");

THREAD_CONCURRENT_TEST(thread_concurrent_bp_wp_sig, TCSH_DISCARD,
    THREAD_CONCURRENT_BREAKPOINT_NUM, THREAD_CONCURRENT_SIGNALS_NUM,
    THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent breakpoints, watchpoints and signals are reported "
    "correctly");
THREAD_CONCURRENT_TEST(thread_concurrent_bp_wp_sig_sig_ign, TCSH_SIG_IGN,
    THREAD_CONCURRENT_BREAKPOINT_NUM, THREAD_CONCURRENT_SIGNALS_NUM,
    THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent breakpoints, watchpoints and signals are reported "
    "correctly and passed back to SIG_IGN handler");
THREAD_CONCURRENT_TEST(thread_concurrent_bp_wp_sig_handler, TCSH_HANDLER,
    THREAD_CONCURRENT_BREAKPOINT_NUM, THREAD_CONCURRENT_SIGNALS_NUM,
    THREAD_CONCURRENT_WATCHPOINT_NUM,
    "Verify that concurrent breakpoints, watchpoints and signals are reported "
    "correctly and passed back to a handler function");
#endif

#endif /*defined(TWAIT_HAVE_STATUS)*/

/// ----------------------------------------------------------------------------

#include "t_ptrace_amd64_wait.h"
#include "t_ptrace_i386_wait.h"
#include "t_ptrace_x86_wait.h"

/// ----------------------------------------------------------------------------

#else
ATF_TC(dummy);
ATF_TC_HEAD(dummy, tc)
{
	atf_tc_set_md_var(tc, "descr", "A dummy test");
}

ATF_TC_BODY(dummy, tc)
{

	// Dummy, skipped
	// The ATF framework requires at least a single defined test.
}
#endif

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

#ifdef ENABLE_TESTS
	ATF_TP_ADD_TC(tp, traceme_raise1);
	ATF_TP_ADD_TC(tp, traceme_raise2);
	ATF_TP_ADD_TC(tp, traceme_raise3);
	ATF_TP_ADD_TC(tp, traceme_raise4);
	ATF_TP_ADD_TC(tp, traceme_raise5);
	ATF_TP_ADD_TC(tp, traceme_raise6);
	ATF_TP_ADD_TC(tp, traceme_raise7);
	ATF_TP_ADD_TC(tp, traceme_raise8);
	ATF_TP_ADD_TC(tp, traceme_raise9);
	ATF_TP_ADD_TC(tp, traceme_raise10);

	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored1);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored2);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored3);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored4);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored5);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored6);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored7);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_ignored8);

	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked1);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked2);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked3);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked4);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked5);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked6);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked7);
	ATF_TP_ADD_TC(tp, traceme_raisesignal_masked8);

	ATF_TP_ADD_TC(tp, traceme_crash_trap);
	ATF_TP_ADD_TC(tp, traceme_crash_segv);
	ATF_TP_ADD_TC(tp, traceme_crash_ill);
	ATF_TP_ADD_TC(tp, traceme_crash_fpe);
	ATF_TP_ADD_TC(tp, traceme_crash_bus);

	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_trap);
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_segv);
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_ill);
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_fpe);
	ATF_TP_ADD_TC(tp, traceme_signalmasked_crash_bus);

	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_trap);
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_segv);
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_ill);
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_fpe);
	ATF_TP_ADD_TC(tp, traceme_signalignored_crash_bus);

	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle1);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle2);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle3);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle4);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle5);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle6);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle7);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_handle8);

	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked1);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked2);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked3);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked4);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked5);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked6);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked7);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_masked8);

	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored1);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored2);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored3);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored4);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored5);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored6);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored7);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_ignored8);

	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple1);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple2);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple3);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple4);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple5);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple6);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple7);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple8);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple9);
	ATF_TP_ADD_TC(tp, traceme_sendsignal_simple10);

	ATF_TP_ADD_TC(tp, traceme_pid1_parent);

	ATF_TP_ADD_TC(tp, traceme_vfork_raise1);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise2);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise3);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise4);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise5);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise6);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise7);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise8);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise9);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise10);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise11);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise12);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise13);

	ATF_TP_ADD_TC(tp, traceme_vfork_crash_trap);
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_segv);
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_ill);
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_fpe);
	ATF_TP_ADD_TC(tp, traceme_vfork_crash_bus);

	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_trap);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_segv);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_ill);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_fpe);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_crash_bus);

	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_trap);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_segv);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_ill);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_fpe);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_crash_bus);

	ATF_TP_ADD_TC(tp, traceme_vfork_exec);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_exec);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_exec);

	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_trap);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_segv);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_ill);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_fpe);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_sees_crash_bus);

	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalmasked_crash_trap);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalmasked_crash_segv);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalmasked_crash_ill);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalmasked_crash_fpe);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalmasked_crash_bus);

	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalignored_crash_trap);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalignored_crash_segv);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalignored_crash_ill);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalignored_crash_fpe);
	ATF_TP_ADD_TC_HAVE_PID(tp,
	    unrelated_tracer_sees_signalignored_crash_bus);

	ATF_TP_ADD_TC_HAVE_PID(tp, tracer_sees_terminaton_before_the_parent);
	ATF_TP_ADD_TC_HAVE_PID(tp, tracer_sysctl_lookup_without_duplicates);
	ATF_TP_ADD_TC_HAVE_PID(tp,
		unrelated_tracer_sees_terminaton_before_the_parent);
	ATF_TP_ADD_TC_HAVE_PID(tp, tracer_attach_to_unrelated_stopped_process);

	ATF_TP_ADD_TC(tp, parent_attach_to_its_child);
	ATF_TP_ADD_TC(tp, parent_attach_to_its_stopped_child);

	ATF_TP_ADD_TC(tp, child_attach_to_its_parent);
	ATF_TP_ADD_TC(tp, child_attach_to_its_stopped_parent);

	ATF_TP_ADD_TC_HAVE_PID(tp,
		tracee_sees_its_original_parent_getppid);
	ATF_TP_ADD_TC_HAVE_PID(tp,
		tracee_sees_its_original_parent_sysctl_kinfo_proc2);
	ATF_TP_ADD_TC_HAVE_PID(tp,
		tracee_sees_its_original_parent_procfs_status);

	ATF_TP_ADD_TC(tp, eventmask_preserved_empty);
	ATF_TP_ADD_TC(tp, eventmask_preserved_fork);
	ATF_TP_ADD_TC(tp, eventmask_preserved_vfork);
	ATF_TP_ADD_TC(tp, eventmask_preserved_vfork_done);
	ATF_TP_ADD_TC(tp, eventmask_preserved_lwp_create);
	ATF_TP_ADD_TC(tp, eventmask_preserved_lwp_exit);
	ATF_TP_ADD_TC(tp, eventmask_preserved_posix_spawn);

	ATF_TP_ADD_TC(tp, fork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork4);
	ATF_TP_ADD_TC(tp, fork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork8);
	ATF_TP_ADD_TC(tp, fork9);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork10);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork11);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork12);
	ATF_TP_ADD_TC(tp, fork13);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork14);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork15);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork16);

	ATF_TP_ADD_TC(tp, vfork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork4);
	ATF_TP_ADD_TC(tp, vfork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork8);
	ATF_TP_ADD_TC(tp, vfork9);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork10);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork11);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork12);
	ATF_TP_ADD_TC(tp, vfork13);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork14);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork15);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork16);

	ATF_TP_ADD_TC(tp, posix_spawn1);
	ATF_TP_ADD_TC(tp, posix_spawn2);
	ATF_TP_ADD_TC(tp, posix_spawn3);
	ATF_TP_ADD_TC(tp, posix_spawn4);
	ATF_TP_ADD_TC(tp, posix_spawn5);
	ATF_TP_ADD_TC(tp, posix_spawn6);
	ATF_TP_ADD_TC(tp, posix_spawn7);
	ATF_TP_ADD_TC(tp, posix_spawn8);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn9);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn10);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn11);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn12);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn13);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn14);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn15);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn16);

	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork4);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork8);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork9);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork10);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork11);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork12);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork13);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork14);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork15);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork16);

	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork4);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork8);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork9);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork10);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork11);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork12);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork13);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork14);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork15);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork16);

	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn1);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn2);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn3);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn4);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn5);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn6);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn7);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn8);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn9);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn10);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn11);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn12);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn13);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn14);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn15);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn16);

	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_detach_spawner);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_detach_forker);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_detach_vforker);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_detach_vforkerdone);

	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_kill_spawner);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_kill_forker);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_kill_vforker);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_kill_vforkerdone);

	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn_detach_spawner);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork_detach_forker);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_detach_vforker);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_detach_vforkerdone);

	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn_kill_spawner);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork_kill_forker);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_kill_vforker);
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_kill_vforkerdone);

	ATF_TP_ADD_TC(tp, traceme_vfork_fork);
	ATF_TP_ADD_TC(tp, traceme_vfork_vfork);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_8);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_16);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_32);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_64);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_8);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_16);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_32);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_64);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_8);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_16);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_32);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_64);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_8);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_16);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_32);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_64);

	ATF_TP_ADD_TC(tp, bytes_transfer_read_d);
	ATF_TP_ADD_TC(tp, bytes_transfer_read_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_write_d);
	ATF_TP_ADD_TC(tp, bytes_transfer_write_i);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_8_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_16_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_32_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_64_text);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_8_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_16_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_32_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_64_text);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_8_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_16_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_32_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_64_text);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_8_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_16_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_32_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_64_text);

	ATF_TP_ADD_TC(tp, bytes_transfer_read_d_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_read_i_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_write_d_text);
	ATF_TP_ADD_TC(tp, bytes_transfer_write_i_text);

	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_auxv);

	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_read_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_read_d);
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_write_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_write_d);

	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_read_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_read_d);
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_write_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_write_d);

	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_read_auxv);

	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_read_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_read_d);
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_write_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_write_d);

	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_read_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_read_d);
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_write_i);
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_write_d);

	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs1);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs2);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs3);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs4);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs5);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs6);

	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs_set_unaligned_pc_0x1);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs_set_unaligned_pc_0x3);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, access_regs_set_unaligned_pc_0x7);

	ATF_TP_ADD_TC_HAVE_FPREGS(tp, access_fpregs1);
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, access_fpregs2);

	ATF_TP_ADD_TC_PT_STEP(tp, step1);
	ATF_TP_ADD_TC_PT_STEP(tp, step2);
	ATF_TP_ADD_TC_PT_STEP(tp, step3);
	ATF_TP_ADD_TC_PT_STEP(tp, step4);

	ATF_TP_ADD_TC_PT_STEP(tp, setstep1);
	ATF_TP_ADD_TC_PT_STEP(tp, setstep2);
	ATF_TP_ADD_TC_PT_STEP(tp, setstep3);
	ATF_TP_ADD_TC_PT_STEP(tp, setstep4);

	ATF_TP_ADD_TC_PT_STEP(tp, step_signalmasked);
	ATF_TP_ADD_TC_PT_STEP(tp, step_signalignored);

	ATF_TP_ADD_TC(tp, kill1);
	ATF_TP_ADD_TC(tp, kill2);
	ATF_TP_ADD_TC(tp, kill3);

	ATF_TP_ADD_TC(tp, traceme_lwpinfo0);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3);

	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus);

	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus_pl_sigmask);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus_pl_sigmask);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus_pl_sigmask);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus_pl_sigmask);

	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus_pl_name);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus_pl_name);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus_pl_name);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus_pl_name);

	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus_pl_private);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus_pl_private);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus_pl_private);
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus_pl_private);

	ATF_TP_ADD_TC(tp, traceme_lwpnext0);
	ATF_TP_ADD_TC(tp, traceme_lwpnext1);
	ATF_TP_ADD_TC(tp, traceme_lwpnext2);
	ATF_TP_ADD_TC(tp, traceme_lwpnext3);

	ATF_TP_ADD_TC(tp, traceme_lwpnext0_pl_sigmask);
	ATF_TP_ADD_TC(tp, traceme_lwpnext1_pl_sigmask);
	ATF_TP_ADD_TC(tp, traceme_lwpnext2_pl_sigmask);
	ATF_TP_ADD_TC(tp, traceme_lwpnext3_pl_sigmask);

	ATF_TP_ADD_TC(tp, traceme_lwpnext0_pl_name);
	ATF_TP_ADD_TC(tp, traceme_lwpnext1_pl_name);
	ATF_TP_ADD_TC(tp, traceme_lwpnext2_pl_name);
	ATF_TP_ADD_TC(tp, traceme_lwpnext3_pl_name);

	ATF_TP_ADD_TC(tp, traceme_lwpnext0_pl_private);
	ATF_TP_ADD_TC(tp, traceme_lwpnext1_pl_private);
	ATF_TP_ADD_TC(tp, traceme_lwpnext2_pl_private);
	ATF_TP_ADD_TC(tp, traceme_lwpnext3_pl_private);

	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo0);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo1);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo2);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo3);

	ATF_TP_ADD_TC(tp, siginfo_set_unmodified);
	ATF_TP_ADD_TC(tp, siginfo_set_faked);

	ATF_TP_ADD_TC(tp, traceme_exec);
	ATF_TP_ADD_TC(tp, traceme_signalmasked_exec);
	ATF_TP_ADD_TC(tp, traceme_signalignored_exec);

	ATF_TP_ADD_TC(tp, trace_thread_nolwpevents);
	ATF_TP_ADD_TC(tp, trace_thread_lwpexit);
	ATF_TP_ADD_TC(tp, trace_thread_lwpcreate);
	ATF_TP_ADD_TC(tp, trace_thread_lwpcreate_and_exit);

	ATF_TP_ADD_TC(tp, trace_thread_lwpexit_masked_sigtrap);
	ATF_TP_ADD_TC(tp, trace_thread_lwpcreate_masked_sigtrap);
	ATF_TP_ADD_TC(tp, trace_thread_lwpcreate_and_exit_masked_sigtrap);

	ATF_TP_ADD_TC(tp, signal_mask_unrelated);

	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_singalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_singalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_singalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_singalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_singalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_singalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, vforkdone_singalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, vforkdone_singalignored);

	ATF_TP_ADD_TC(tp, threads_and_exec);

	ATF_TP_ADD_TC(tp, suspend_no_deadlock);

	ATF_TP_ADD_TC(tp, resume);

	ATF_TP_ADD_TC(tp, syscall);
	ATF_TP_ADD_TC(tp, syscall_killed_on_sce);
	ATF_TP_ADD_TC(tp, syscall_signal_on_sce);
	ATF_TP_ADD_TC(tp, syscall_detach_on_sce);

	ATF_TP_ADD_TC(tp, syscallemu1);

	ATF_TP_ADD_TC(tp, clone1);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone2);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone3);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone4);
	ATF_TP_ADD_TC(tp, clone5);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone6);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone7);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone8);

	ATF_TP_ADD_TC(tp, clone_vm1);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm2);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm3);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm4);
	ATF_TP_ADD_TC(tp, clone_vm5);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm6);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm7);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm8);

	ATF_TP_ADD_TC(tp, clone_fs1);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs2);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs3);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs4);
	ATF_TP_ADD_TC(tp, clone_fs5);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs6);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs7);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs8);

	ATF_TP_ADD_TC(tp, clone_files1);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files2);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files3);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files4);
	ATF_TP_ADD_TC(tp, clone_files5);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files6);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files7);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files8);

//	ATF_TP_ADD_TC(tp, clone_sighand1); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand2); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand3); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand4); // XXX
//	ATF_TP_ADD_TC(tp, clone_sighand5); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand6); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand7); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand8); // XXX

	ATF_TP_ADD_TC(tp, clone_vfork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork4);
	ATF_TP_ADD_TC(tp, clone_vfork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork8);

	ATF_TP_ADD_TC_HAVE_PID(tp, clone_signalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_signalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm_signalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm_signalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs_signalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs_signalmasked);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files_signalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files_signalmasked);
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand_signalignored); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand_signalmasked); // XXX
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork_signalignored);
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork_signalmasked);

	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone);
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_vm);
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_fs);
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_files);
//	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_sighand); // XXX
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_vfork);

	ATF_TP_ADD_TC(tp, user_va0_disable_pt_continue);
	ATF_TP_ADD_TC(tp, user_va0_disable_pt_syscall);
	ATF_TP_ADD_TC(tp, user_va0_disable_pt_detach);

	ATF_TP_ADD_TC(tp, core_dump_procinfo);

#if defined(TWAIT_HAVE_STATUS)
	ATF_TP_ADD_TC(tp, thread_concurrent_signals);
	ATF_TP_ADD_TC(tp, thread_concurrent_signals_sig_ign);
	ATF_TP_ADD_TC(tp, thread_concurrent_signals_handler);
#if defined(__i386__) || defined(__x86_64__)
	ATF_TP_ADD_TC(tp, thread_concurrent_breakpoints);
	ATF_TP_ADD_TC(tp, thread_concurrent_watchpoints);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_wp);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_sig);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_sig_sig_ign);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_sig_handler);
	ATF_TP_ADD_TC(tp, thread_concurrent_wp_sig);
	ATF_TP_ADD_TC(tp, thread_concurrent_wp_sig_sig_ign);
	ATF_TP_ADD_TC(tp, thread_concurrent_wp_sig_handler);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_wp_sig);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_wp_sig_sig_ign);
	ATF_TP_ADD_TC(tp, thread_concurrent_bp_wp_sig_handler);
#endif
#endif

	ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64();
	ATF_TP_ADD_TCS_PTRACE_WAIT_I386();
	ATF_TP_ADD_TCS_PTRACE_WAIT_X86();

#else
	ATF_TP_ADD_TC(tp, dummy);
#endif

	return atf_no_error();
}
