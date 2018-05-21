/*	$NetBSD: t_ptrace_wait.c,v 1.23.2.4 2018/05/21 04:36:17 pgoyette Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.23.2.4 2018/05/21 04:36:17 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <machine/reg.h>
#include <elf.h>
#include <err.h>
#include <errno.h>
#include <lwp.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <atf-c.h>

#include "h_macros.h"

#include "t_ptrace_wait.h"
#include "msg.h"

#define PARENT_TO_CHILD(info, fds, msg) \
    SYSCALL_REQUIRE(msg_write_child(info " to child " # fds, &fds, &msg, sizeof(msg)) == 0)

#define CHILD_FROM_PARENT(info, fds, msg) \
    FORKEE_ASSERT(msg_read_parent(info " from parent " # fds, &fds, &msg, sizeof(msg)) == 0)

#define CHILD_TO_PARENT(info, fds, msg) \
    FORKEE_ASSERT(msg_write_parent(info " to parent " # fds, &fds, &msg, sizeof(msg)) == 0)

#define PARENT_FROM_CHILD(info, fds, msg) \
    SYSCALL_REQUIRE(msg_read_child(info " from parent " # fds, &fds, &msg, sizeof(msg)) == 0)

#define SYSCALL_REQUIRE(expr) ATF_REQUIRE_MSG(expr, "%s: %s", # expr, \
    strerror(errno))
#define SYSCALL_REQUIRE_ERRNO(res, exp) ATF_REQUIRE_MSG(res == exp, \
    "%d(%s) != %d", res, strerror(res), exp)

static int debug = 0;

#define DPRINTF(a, ...)	do  \
	if (debug) printf(a,  ##__VA_ARGS__); \
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

	struct ptrace_siginfo info;
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

#define TRACEME_RAISE(test, sig)						\
ATF_TC(test);									\
ATF_TC_HEAD(test, tc)								\
{										\
	atf_tc_set_md_var(tc, "descr",						\
	    "Verify " #sig " followed by _exit(2) in a child");			\
}										\
										\
ATF_TC_BODY(test, tc)								\
{										\
										\
	traceme_raise(sig);							\
}

TRACEME_RAISE(traceme_raise1, SIGKILL) /* non-maskable */
TRACEME_RAISE(traceme_raise2, SIGSTOP) /* non-maskable */
TRACEME_RAISE(traceme_raise3, SIGABRT) /* regular abort trap */
TRACEME_RAISE(traceme_raise4, SIGHUP)  /* hangup */
TRACEME_RAISE(traceme_raise5, SIGCONT) /* continued? */

/// ----------------------------------------------------------------------------

static void
traceme_sighandler_catch(int sigsent, void (*sah)(int arg), int *traceme_caught)
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
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
	                != -1);

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

#define TRACEME_SIGHANDLER_CATCH(test, sig)					\
ATF_TC(test);									\
ATF_TC_HEAD(test, tc)								\
{										\
	atf_tc_set_md_var(tc, "descr",						\
	    "Verify that a signal " #sig " emitted by a tracer to a child is "	\
	    "handled correctly and caught by a signal handler");		\
}										\
										\
static int test##_caught = 0;							\
										\
static void									\
test##_sighandler(int arg)							\
{										\
	FORKEE_ASSERT_EQ(arg, sig);						\
										\
	++ test##_caught;							\
}										\
										\
ATF_TC_BODY(test, tc)								\
{										\
										\
	traceme_sighandler_catch(sig, test##_sighandler, & test##_caught);	\
}

// A signal handler for SIGKILL and SIGSTOP cannot be registered.
TRACEME_SIGHANDLER_CATCH(traceme_sighandler_catch1, SIGABRT) /* abort trap */
TRACEME_SIGHANDLER_CATCH(traceme_sighandler_catch2, SIGHUP)  /* hangup */
TRACEME_SIGHANDLER_CATCH(traceme_sighandler_catch3, SIGCONT) /* continued? */

/// ----------------------------------------------------------------------------

static void
traceme_signal_nohandler(int sigsent)
{
	const int sigval = SIGSTOP;
	int exitval = 0;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
	int expect_core = (sigsent == SIGABRT) ? 1 : 0;
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
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
	                != -1);

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

#define TRACEME_SIGNAL_NOHANDLER(test, sig)					\
ATF_TC(test);									\
ATF_TC_HEAD(test, tc)								\
{										\
	atf_tc_set_md_var(tc, "descr",						\
	    "Verify that a signal " #sig " emitted by a tracer to a child is "	\
	    "handled correctly in a child without a signal handler");		\
}										\
										\
ATF_TC_BODY(test, tc)								\
{										\
										\
	traceme_signal_nohandler(sig);						\
}

TRACEME_SIGNAL_NOHANDLER(traceme_signal_nohandler1, SIGKILL) /* non-maskable */
TRACEME_SIGNAL_NOHANDLER(traceme_signal_nohandler2, SIGSTOP) /* non-maskable */
TRACEME_SIGNAL_NOHANDLER(traceme_signal_nohandler3, SIGABRT) /* abort trap */
TRACEME_SIGNAL_NOHANDLER(traceme_signal_nohandler4, SIGHUP)  /* hangup */
TRACEME_SIGNAL_NOHANDLER(traceme_signal_nohandler5, SIGCONT) /* continued? */

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
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child1, &status, WEXITED),
	                      child1);

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
	int expect_core = (sigval == SIGABRT) ? 1 : 0;
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
		TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(watcher,
		                                                   &status, 0));
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
			/* NOTREACHED */
			FORKEE_ASSERTX(0 && "This shall not be reached");
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

#define TRACEME_VFORK_RAISE(test, sig)						\
ATF_TC(test);									\
ATF_TC_HEAD(test, tc)								\
{										\
	atf_tc_set_md_var(tc, "descr",						\
	    "Verify PT_TRACE_ME followed by raise of " #sig " in a vfork(2)ed "	\
	    "child");								\
}										\
										\
ATF_TC_BODY(test, tc)								\
{										\
										\
	traceme_vfork_raise(sig);						\
}

TRACEME_VFORK_RAISE(traceme_vfork_raise1, SIGKILL) /* non-maskable */
TRACEME_VFORK_RAISE(traceme_vfork_raise2, SIGSTOP) /* non-maskable */
TRACEME_VFORK_RAISE(traceme_vfork_raise3, SIGTSTP) /* ignored in vfork(2) */
TRACEME_VFORK_RAISE(traceme_vfork_raise4, SIGTTIN) /* ignored in vfork(2) */
TRACEME_VFORK_RAISE(traceme_vfork_raise5, SIGTTOU) /* ignored in vfork(2) */
TRACEME_VFORK_RAISE(traceme_vfork_raise6, SIGABRT) /* regular abort trap */
TRACEME_VFORK_RAISE(traceme_vfork_raise7, SIGHUP)  /* hangup */
TRACEME_VFORK_RAISE(traceme_vfork_raise8, SIGCONT) /* continued? */

/// ----------------------------------------------------------------------------

ATF_TC(traceme_vfork_breakpoint);
ATF_TC_HEAD(traceme_vfork_breakpoint, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_TRACE_ME followed by a software breakpoint in a "
	    "vfork(2)ed child");
}

ATF_TC_BODY(traceme_vfork_breakpoint, tc)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before executing a software breakpoint\n");
#ifdef PTRACE_BREAKPOINT_ASM
		PTRACE_BREAKPOINT_ASM;
#else
		/* port me */
#endif

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGTRAP, 1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

/// ----------------------------------------------------------------------------

ATF_TC(traceme_vfork_exec);
ATF_TC_HEAD(traceme_vfork_exec, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_TRACE_ME followed by exec(3) in a vfork(2)ed child");
}

ATF_TC_BODY(traceme_vfork_exec, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = vfork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "Not reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

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

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach1);
ATF_TC_HEAD(attach1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees process termination before the parent");
}

static void
attach1_raw(bool raw)
{
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		// Wait for parent to let us exit
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);
		_exit(exitval_tracee);
	}

	DPRINTF("Spawn debugger\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
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
		_exit(exitval_tracer);
	}

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("tracer ready", parent_tracer, msg);

	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("exit tracee", parent_tracee,  msg);

	DPRINTF("Detect that tracee is zombie\n");
	if (raw)
		await_zombie_raw(tracee, 0);
	else
		await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee %d - "
	    "Tracer must detect zombie first - calling %s()\n", tracee,
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	DPRINTF("Tell the tracer child should have exited\n");
	PARENT_TO_CHILD("wait for tracee exit", parent_tracer,  msg);
	DPRINTF("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);

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

ATF_TC_BODY(attach1, tc)
{

	/* Reuse this test with race1 */
	attach1_raw(false);
}

#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach2);
ATF_TC_HEAD(attach2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that any tracer sees process termination before its "
	    "parent");
}

ATF_TC_BODY(attach2, tc)
{
	struct msg_fds parent_tracer, parent_tracee;
	const int exitval_tracee = 5;
	const int exitval_tracer1 = 10, exitval_tracer2 = 20;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		/* Wait for message from the parent */
		CHILD_FROM_PARENT("Message 1", parent_tracee, msg);
		_exit(exitval_tracee);
	}

	DPRINTF("Spawn debugger\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracer = atf_utils_fork();
	if (tracer == 0) {
		/* Fork again and drop parent to reattach to PID 1 */
		tracer = atf_utils_fork();
		if (tracer != 0)
			_exit(exitval_tracer1);

		DPRINTF("Before calling PT_ATTACH from tracee %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_ATTACH, tracee, NULL, 0) != -1);

		/* Wait for tracee and assert that it was stopped w/ SIGSTOP */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_stopped(status, SIGSTOP);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		CHILD_TO_PARENT("Message 1", parent_tracer, msg);
		CHILD_FROM_PARENT("Message 2", parent_tracer, msg);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(exitval_tracer2);
	}
	DPRINTF("Wait for the tracer process (direct child) to exit calling "
	    "%s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracer, &status, 0), tracer);

	validate_status_exited(status, exitval_tracer1);

	DPRINTF("Wait for the non-exited tracee process with %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, NULL, WNOHANG), 0);

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("Message 1", parent_tracer, msg);
	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("Message 1", parent_tracee, msg);

	DPRINTF("Detect that tracee is zombie\n");
	await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	DPRINTF("Resume the tracer and let it detect exited tracee\n");
	PARENT_TO_CHILD("Message 2", parent_tracer, msg);

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, 0),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}
#endif

ATF_TC(attach3);
ATF_TC_HEAD(attach3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer parent can PT_ATTACH to its child");
}

ATF_TC_BODY(attach3, tc)
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

		CHILD_FROM_PARENT("Message 2", parent_tracee, msg);
		/* Wait for message from the parent */
		_exit(exitval_tracee);
	}
	PARENT_TO_CHILD("Message 1", parent_tracee, msg);
	
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

ATF_TC(attach4);
ATF_TC_HEAD(attach4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer child can PT_ATTACH to its parent");
}

ATF_TC_BODY(attach4, tc)
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

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach5);
ATF_TC_HEAD(attach5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees its parent when attached to tracer "
	    "(check getppid(2))");
}

ATF_TC_BODY(attach5, tc)
{
	struct msg_fds parent_tracer, parent_tracee;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		parent = getppid();

		/* Emit message to the parent */
		CHILD_TO_PARENT("tracee ready", parent_tracee, msg);
		CHILD_FROM_PARENT("exit tracee", parent_tracee, msg);

		FORKEE_ASSERT_EQ(parent, getppid());

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
#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach6);
ATF_TC_HEAD(attach6, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees its parent when attached to tracer "
	    "(check sysctl(7) and struct kinfo_proc2)");
}

ATF_TC_BODY(attach6, tc)
{
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	int name[CTL_MAXNAME];
	struct kinfo_proc2 kp;
	size_t len = sizeof(kp);
	unsigned int namelen;

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		parent = getppid();

		/* Emit message to the parent */
		CHILD_TO_PARENT("Message 1", parent_tracee, msg);
		CHILD_FROM_PARENT("Message 2", parent_tracee, msg);

		namelen = 0;
		name[namelen++] = CTL_KERN;
		name[namelen++] = KERN_PROC2;
		name[namelen++] = KERN_PROC_PID;
		name[namelen++] = getpid();
		name[namelen++] = len;
		name[namelen++] = 1;

		FORKEE_ASSERT(sysctl(name, namelen, &kp, &len, NULL, 0) == 0);
		FORKEE_ASSERT_EQ(parent, kp.p_ppid);

		_exit(exitval_tracee);
	}

	DPRINTF("Wait for child to record its parent identifier (pid)\n");
	PARENT_FROM_CHILD("Message 1", parent_tracee, msg);

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
		CHILD_TO_PARENT("Message 1", parent_tracer, msg);

		CHILD_FROM_PARENT("Message 2", parent_tracer, msg);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("Message 1", parent_tracer, msg);

	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("Message 1", parent_tracee, msg);

	DPRINTF("Detect that tracee is zombie\n");
	await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	DPRINTF("Resume the tracer and let it detect exited tracee\n");
	PARENT_TO_CHILD("Message 2", parent_tracer, msg);

	DPRINTF("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	msg_close(&parent_tracee);
	msg_close(&parent_tracer);
}
#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(attach7);
ATF_TC_HEAD(attach7, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that tracer sees its parent when attached to tracer "
	    "(check /proc/curproc/status 3rd column)");
}

ATF_TC_BODY(attach7, tc)
{
	struct msg_fds parent_tracee, parent_tracer;
	int rv;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t parent, tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	FILE *fp;
	struct stat st;
	const char *fname = "/proc/curproc/status";
	char s_executable[MAXPATHLEN];
	int s_pid, s_ppid;
	/*
	 * Format:
	 *  EXECUTABLE PID PPID ...
	 */

	SYSCALL_REQUIRE((rv = stat(fname, &st)) == 0 || (errno == ENOENT));
	if (rv != 0) {
		atf_tc_skip("/proc/curproc/status not found");
	}

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		parent = getppid();

		// Wait for parent to let us exit
		CHILD_TO_PARENT("tracee ready", parent_tracee, msg);
		CHILD_FROM_PARENT("tracee exit", parent_tracee, msg);

		FORKEE_ASSERT((fp = fopen(fname, "r")) != NULL);
		fscanf(fp, "%s %d %d", s_executable, &s_pid, &s_ppid);
		FORKEE_ASSERT(fclose(fp) == 0);
		FORKEE_ASSERT_EQ(parent, s_ppid);

		_exit(exitval_tracee);
	}

	DPRINTF("Wait for child to record its parent identifier (pid)\n");
	PARENT_FROM_CHILD("tracee ready", parent_tracee, msg);

	DPRINTF("Spawn debugger\n");
	tracer = atf_utils_fork();
	if (tracer == 0) {
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
	PARENT_FROM_CHILD("tracer ready", parent_tracer, msg);
	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("tracee exit", parent_tracee, msg);

	DPRINTF("Detect that tracee is zombie\n");
	await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	DPRINTF("Resume the tracer and let it detect exited tracee\n");
	PARENT_TO_CHILD("Message 2", parent_tracer, msg);

	DPRINTF("Wait for tracer to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracer, &status, 0),
	    tracer);

	validate_status_exited(status, exitval_tracer);

	DPRINTF("Wait for tracee to finish its job and exit - calling %s()\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(tracee, &status, WNOHANG),
	    tracee);

	validate_status_exited(status, exitval_tracee);

	msg_close(&parent_tracee);
	msg_close(&parent_tracer);
}
#endif

ATF_TC(eventmask1);
ATF_TC_HEAD(eventmask1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that empty EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask1, tc)
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

	set_event.pe_set_event = 0;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
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

ATF_TC(eventmask2);
ATF_TC_HEAD(eventmask2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PTRACE_FORK in EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask2, tc)
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

	set_event.pe_set_event = PTRACE_FORK;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
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

ATF_TC(eventmask3);
ATF_TC_HEAD(eventmask3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PTRACE_VFORK in EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask3, tc)
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

	set_event.pe_set_event = PTRACE_VFORK;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
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

ATF_TC(eventmask4);
ATF_TC_HEAD(eventmask4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PTRACE_VFORK_DONE in EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask4, tc)
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

	set_event.pe_set_event = PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
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

ATF_TC(eventmask5);
ATF_TC_HEAD(eventmask5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PTRACE_LWP_CREATE in EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask5, tc)
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

	set_event.pe_set_event = PTRACE_LWP_CREATE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
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

ATF_TC(eventmask6);
ATF_TC_HEAD(eventmask6, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PTRACE_LWP_EXIT in EVENT_MASK is preserved");
}

ATF_TC_BODY(eventmask6, tc)
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

	set_event.pe_set_event = PTRACE_LWP_EXIT;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &set_event, len) != -1);
	SYSCALL_REQUIRE(ptrace(PT_GET_EVENT_MASK, child, &get_event, len) != -1);
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

static void
fork_body(pid_t (*fn)(void), bool trackfork, bool trackvfork,
          bool trackvforkdone, bool detachchild, bool detachparent)
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

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

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
	if ((trackfork && fn == fork) || (trackvfork && fn == vfork)) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		        child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		                      child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state,
		                       slen) != -1);
		if (trackfork && fn == fork) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (trackvfork && fn == vfork) {
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

		SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child2, &state,
		                       slen) != -1);
		if (trackfork && fn == fork) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_FORK,
			       PTRACE_FORK);
		}
		if (trackvfork && fn == vfork) {
			ATF_REQUIRE_EQ(state.pe_report_event & PTRACE_VFORK,
			       PTRACE_VFORK);
		}

		ATF_REQUIRE_EQ(state.pe_other_pid, child);

		DPRINTF("Before resuming the forkee process where it left off "
		    "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child2, (void *)1, 0)
		                != -1);

		DPRINTF("Before resuming the child process where it left off "
		        "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}
#endif

	if (trackvforkdone && fn == vfork) {
		DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME,
		        child);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		                      child);

		validate_status_stopped(status, SIGTRAP);

		SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state,
		                       slen) != -1);
		ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

		child2 = state.pe_other_pid;
		DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n",
		        child2);

		DPRINTF("Before resuming the child process where it left off "
		        "and without signal to be sent\n");
		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
	}

#if defined(TWAIT_HAVE_PID)
	if ((trackfork && fn == fork) || (trackvfork && fn == vfork)) {
		DPRINTF("Before calling %s() for the forkee - expected exited"
		        "\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
		    child2);

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

#define FORK_TEST(name,descr,fun,tfork,tvfork,tvforkdone,detchild,detparent)	\
ATF_TC(name);									\
ATF_TC_HEAD(name, tc)								\
{										\
	atf_tc_set_md_var(tc, "descr", descr);					\
}										\
										\
ATF_TC_BODY(name, tc)								\
{										\
										\
	fork_body(fun, tfork, tvfork, tvforkdone, detchild, detparent);		\
}

#define F false
#define T true

#define F_IF__0(x)
#define F_IF__1(x) x
#define F_IF__(x,y) F_IF__ ## x (y)
#define F_IF_(x,y) F_IF__(x,y)
#define F_IF(x,y) F_IF_(x,y)

#define DSCR(function,forkbit,vforkbit,vforkdonebit,dchildbit,dparentbit)	\
        "Verify " #function "(2) called with 0"					\
        F_IF(forkbit,"|PTRACE_FORK")						\
        F_IF(vforkbit,"|PTRACE_VFORK")						\
        F_IF(vforkdonebit,"|PTRACE_VFORK_DONE")					\
        " in EVENT_MASK."							\
        F_IF(dchildbit," Detach child in this test.")				\
        F_IF(dparentbit," Detach parent in this test.")

FORK_TEST(fork1, DSCR(fork,0,0,0,0,0), fork, F, F, F, F, F)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(fork2, DSCR(fork,1,0,0,0,0), fork, T, F, F, F, F)
FORK_TEST(fork3, DSCR(fork,0,1,0,0,0), fork, F, T, F, F, F)
FORK_TEST(fork4, DSCR(fork,1,1,0,0,0), fork, T, T, F, F, F)
#endif
FORK_TEST(fork5, DSCR(fork,0,0,1,0,0), fork, F, F, T, F, F)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(fork6, DSCR(fork,1,0,1,0,0), fork, T, F, T, F, F)
FORK_TEST(fork7, DSCR(fork,0,1,1,0,0), fork, F, T, T, F, F)
FORK_TEST(fork8, DSCR(fork,1,1,1,0,0), fork, T, T, T, F, F)
#endif

FORK_TEST(vfork1, DSCR(vfork,0,0,0,0,0), vfork, F, F, F, F, F)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(vfork2, DSCR(vfork,1,0,0,0,0), vfork, T, F, F, F, F)
FORK_TEST(vfork3, DSCR(vfork,0,1,0,0,0), vfork, F, T, F, F, F)
FORK_TEST(vfork4, DSCR(vfork,1,1,0,0,0), vfork, T, T, F, F, F)
#endif
FORK_TEST(vfork5, DSCR(vfork,0,0,1,0,0), vfork, F, F, T, F, F)
#if defined(TWAIT_HAVE_PID)
FORK_TEST(vfork6, DSCR(vfork,1,0,1,0,0), vfork, T, F, T, F, F)
FORK_TEST(vfork7, DSCR(vfork,0,1,1,0,0), vfork, F, T, T, F, F)
FORK_TEST(vfork8, DSCR(vfork,1,1,1,0,0), vfork, T, T, T, F, F)
#endif




ATF_TC(io_read_d1);
ATF_TC_HEAD(io_read_d1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_D and len = sizeof(uint8_t)");
}

ATF_TC_BODY(io_read_d1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint8_t lookup_me = 0;
	const uint8_t magic = 0xab;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me = magic;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx8 " != expected %" PRIx8, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_d2);
ATF_TC_HEAD(io_read_d2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_D and len = sizeof(uint16_t)");
}

ATF_TC_BODY(io_read_d2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint16_t lookup_me = 0;
	const uint16_t magic = 0x1234;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me = magic;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx16 " != expected %" PRIx16, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_d3);
ATF_TC_HEAD(io_read_d3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_D and len = sizeof(uint32_t)");
}

ATF_TC_BODY(io_read_d3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint32_t lookup_me = 0;
	const uint32_t magic = 0x1234abcd;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me = magic;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx32 " != expected %" PRIx32, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_d4);
ATF_TC_HEAD(io_read_d4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_D and len = sizeof(uint64_t)");
}

ATF_TC_BODY(io_read_d4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint64_t lookup_me = 0;
	const uint64_t magic = 0x1234abcd9876dcfa;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me = magic;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx64 " != expected %" PRIx64, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_write_d1);
ATF_TC_HEAD(io_write_d1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_WRITE_D and len = sizeof(uint8_t)");
}

ATF_TC_BODY(io_write_d1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint8_t lookup_me = 0;
	const uint8_t magic = 0xab;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_WRITE_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		FORKEE_ASSERT_EQ(lookup_me, magic);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	lookup_me = magic;

	DPRINTF("Write new lookup_me to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_write_d2);
ATF_TC_HEAD(io_write_d2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_WRITE_D and len = sizeof(uint16_t)");
}

ATF_TC_BODY(io_write_d2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint16_t lookup_me = 0;
	const uint16_t magic = 0xab12;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_WRITE_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		FORKEE_ASSERT_EQ(lookup_me, magic);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	lookup_me = magic;

	DPRINTF("Write new lookup_me to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_write_d3);
ATF_TC_HEAD(io_write_d3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_WRITE_D and len = sizeof(uint32_t)");
}

ATF_TC_BODY(io_write_d3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint32_t lookup_me = 0;
	const uint32_t magic = 0xab127643;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_WRITE_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		FORKEE_ASSERT_EQ(lookup_me, magic);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	lookup_me = magic;

	DPRINTF("Write new lookup_me to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_write_d4);
ATF_TC_HEAD(io_write_d4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_WRITE_D and len = sizeof(uint64_t)");
}

ATF_TC_BODY(io_write_d4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint64_t lookup_me = 0;
	const uint64_t magic = 0xab12764376490123;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_WRITE_D,
		.piod_offs = &lookup_me,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		FORKEE_ASSERT_EQ(lookup_me, magic);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	lookup_me = magic;

	DPRINTF("Write new lookup_me to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_auxv1);
ATF_TC_HEAD(io_read_auxv1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_AUXV called for tracee");
}

ATF_TC_BODY(io_read_auxv1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	AuxInfo ai[100], *aip;
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_AUXV,
		.piod_offs = 0,
		.piod_addr = ai,
		.piod_len = sizeof(ai)
	};

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

	DPRINTF("Read new AUXV from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	DPRINTF("Asserting that AUXV length (%zu) is > 0\n", io.piod_len);
	ATF_REQUIRE(io.piod_len > 0);

	for (aip = ai; aip->a_type != AT_NULL; aip++)
		DPRINTF("a_type=%#llx a_v=%#llx\n",
		    (long long int)aip->a_type, (long long int)aip->a_v);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_d1);
ATF_TC_HEAD(read_d1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_D called once");
}

ATF_TC_BODY(read_d1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me = 0;
	const int magic = (int)random();
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me = magic;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me = ptrace(PT_READ_D, child, &lookup_me, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %#x != expected %#x", lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_d2);
ATF_TC_HEAD(read_d2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_D called twice");
}

ATF_TC_BODY(read_d2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	const int magic1 = (int)random();
	const int magic2 = (int)random();
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me1 = magic1;
		lookup_me2 = magic2;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me1 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me1 = ptrace(PT_READ_D, child, &lookup_me1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me1, magic1,
	    "got value %#x != expected %#x", lookup_me1, magic1);

	DPRINTF("Read new lookup_me2 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me2 = ptrace(PT_READ_D, child, &lookup_me2, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me2, magic2,
	    "got value %#x != expected %#x", lookup_me2, magic2);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_d3);
ATF_TC_HEAD(read_d3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_D called three times");
}

ATF_TC_BODY(read_d3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int lookup_me3 = 0;
	const int magic1 = (int)random();
	const int magic2 = (int)random();
	const int magic3 = (int)random();
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me1 = magic1;
		lookup_me2 = magic2;
		lookup_me3 = magic3;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me1 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me1 = ptrace(PT_READ_D, child, &lookup_me1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me1, magic1,
	    "got value %#x != expected %#x", lookup_me1, magic1);

	DPRINTF("Read new lookup_me2 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me2 = ptrace(PT_READ_D, child, &lookup_me2, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me2, magic2,
	    "got value %#x != expected %#x", lookup_me2, magic2);

	DPRINTF("Read new lookup_me3 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me3 = ptrace(PT_READ_D, child, &lookup_me3, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me3, magic3,
	    "got value %#x != expected %#x", lookup_me3, magic3);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_d4);
ATF_TC_HEAD(read_d4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_D called four times");
}

ATF_TC_BODY(read_d4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int lookup_me3 = 0;
	int lookup_me4 = 0;
	const int magic1 = (int)random();
	const int magic2 = (int)random();
	const int magic3 = (int)random();
	const int magic4 = (int)random();
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me1 = magic1;
		lookup_me2 = magic2;
		lookup_me3 = magic3;
		lookup_me4 = magic4;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me1 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me1 = ptrace(PT_READ_D, child, &lookup_me1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me1, magic1,
	    "got value %#x != expected %#x", lookup_me1, magic1);

	DPRINTF("Read new lookup_me2 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me2 = ptrace(PT_READ_D, child, &lookup_me2, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me2, magic2,
	    "got value %#x != expected %#x", lookup_me2, magic2);

	DPRINTF("Read new lookup_me3 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me3 = ptrace(PT_READ_D, child, &lookup_me3, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me3, magic3,
	    "got value %#x != expected %#x", lookup_me3, magic3);

	DPRINTF("Read new lookup_me4 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me4 = ptrace(PT_READ_D, child, &lookup_me4, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me4, magic4,
	    "got value %#x != expected %#x", lookup_me4, magic4);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(write_d1);
ATF_TC_HEAD(write_d1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_WRITE_D called once");
}

ATF_TC_BODY(write_d1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me = 0;
	const int magic = (int)random();
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

		FORKEE_ASSERT_EQ(lookup_me, magic);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Write new lookup_me to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me, magic) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(write_d2);
ATF_TC_HEAD(write_d2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_WRITE_D called twice");
}

ATF_TC_BODY(write_d2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	const int magic1 = (int)random();
	const int magic2 = (int)random();
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

		FORKEE_ASSERT_EQ(lookup_me1, magic1);
		FORKEE_ASSERT_EQ(lookup_me2, magic2);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Write new lookup_me1 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me1, magic1) != -1);

	DPRINTF("Write new lookup_me2 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me2, magic2) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(write_d3);
ATF_TC_HEAD(write_d3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_WRITE_D called three times");
}

ATF_TC_BODY(write_d3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int lookup_me3 = 0;
	const int magic1 = (int)random();
	const int magic2 = (int)random();
	const int magic3 = (int)random();
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

		FORKEE_ASSERT_EQ(lookup_me1, magic1);
		FORKEE_ASSERT_EQ(lookup_me2, magic2);
		FORKEE_ASSERT_EQ(lookup_me3, magic3);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Write new lookup_me1 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me1, magic1) != -1);

	DPRINTF("Write new lookup_me2 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me2, magic2) != -1);

	DPRINTF("Write new lookup_me3 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me3, magic3) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(write_d4);
ATF_TC_HEAD(write_d4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_WRITE_D called four times");
}

ATF_TC_BODY(write_d4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int lookup_me3 = 0;
	int lookup_me4 = 0;
	const int magic1 = (int)random();
	const int magic2 = (int)random();
	const int magic3 = (int)random();
	const int magic4 = (int)random();
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

		FORKEE_ASSERT_EQ(lookup_me1, magic1);
		FORKEE_ASSERT_EQ(lookup_me2, magic2);
		FORKEE_ASSERT_EQ(lookup_me3, magic3);
		FORKEE_ASSERT_EQ(lookup_me4, magic4);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Write new lookup_me1 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me1, magic1) != -1);

	DPRINTF("Write new lookup_me2 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me2, magic2) != -1);

	DPRINTF("Write new lookup_me3 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me3, magic3) != -1);

	DPRINTF("Write new lookup_me4 to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, &lookup_me4, magic4) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_d_write_d_handshake1);
ATF_TC_HEAD(io_read_d_write_d_handshake1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_D and PIOD_WRITE_D handshake");
}

ATF_TC_BODY(io_read_d_write_d_handshake1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint8_t lookup_me_fromtracee = 0;
	const uint8_t magic_fromtracee = (uint8_t)random();
	uint8_t lookup_me_totracee = 0;
	const uint8_t magic_totracee = (uint8_t)random();
	struct ptrace_io_desc io_fromtracee = {
		.piod_op = PIOD_READ_D,
		.piod_offs = &lookup_me_fromtracee,
		.piod_addr = &lookup_me_fromtracee,
		.piod_len = sizeof(lookup_me_fromtracee)
	};
	struct ptrace_io_desc io_totracee = {
		.piod_op = PIOD_WRITE_D,
		.piod_offs = &lookup_me_totracee,
		.piod_addr = &lookup_me_totracee,
		.piod_len = sizeof(lookup_me_totracee)
	};
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me_fromtracee = magic_fromtracee;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(lookup_me_totracee, magic_totracee);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me_fromtracee PID=%d by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io_fromtracee, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me_fromtracee, magic_fromtracee,
	    "got value %" PRIx8 " != expected %" PRIx8, lookup_me_fromtracee,
	    magic_fromtracee);

	lookup_me_totracee = magic_totracee;

	DPRINTF("Write lookup_me_totracee to PID=%d by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io_totracee, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me_totracee, magic_totracee,
	    "got value %" PRIx8 " != expected %" PRIx8, lookup_me_totracee,
	    magic_totracee);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_d_write_d_handshake2);
ATF_TC_HEAD(io_read_d_write_d_handshake2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_WRITE_D and PIOD_READ_D handshake");
}

ATF_TC_BODY(io_read_d_write_d_handshake2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint8_t lookup_me_fromtracee = 0;
	const uint8_t magic_fromtracee = (uint8_t)random();
	uint8_t lookup_me_totracee = 0;
	const uint8_t magic_totracee = (uint8_t)random();
	struct ptrace_io_desc io_fromtracee = {
		.piod_op = PIOD_READ_D,
		.piod_offs = &lookup_me_fromtracee,
		.piod_addr = &lookup_me_fromtracee,
		.piod_len = sizeof(lookup_me_fromtracee)
	};
	struct ptrace_io_desc io_totracee = {
		.piod_op = PIOD_WRITE_D,
		.piod_offs = &lookup_me_totracee,
		.piod_addr = &lookup_me_totracee,
		.piod_len = sizeof(lookup_me_totracee)
	};
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me_fromtracee = magic_fromtracee;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(lookup_me_totracee, magic_totracee);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	lookup_me_totracee = magic_totracee;

	DPRINTF("Write lookup_me_totracee to PID=%d by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io_totracee, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me_totracee, magic_totracee,
	    "got value %" PRIx8 " != expected %" PRIx8, lookup_me_totracee,
	    magic_totracee);

	DPRINTF("Read lookup_me_fromtracee PID=%d by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io_fromtracee, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me_fromtracee, magic_fromtracee,
	    "got value %" PRIx8 " != expected %" PRIx8, lookup_me_fromtracee,
	    magic_fromtracee);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_d_write_d_handshake1);
ATF_TC_HEAD(read_d_write_d_handshake1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_D with PT_WRITE_D handshake");
}

ATF_TC_BODY(read_d_write_d_handshake1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me_fromtracee = 0;
	const int magic_fromtracee = (int)random();
	int lookup_me_totracee = 0;
	const int magic_totracee = (int)random();
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me_fromtracee = magic_fromtracee;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(lookup_me_totracee, magic_totracee);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me_fromtracee PID=%d by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me_fromtracee =
	    ptrace(PT_READ_D, child, &lookup_me_fromtracee, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me_fromtracee, magic_fromtracee,
	    "got value %#x != expected %#x", lookup_me_fromtracee,
	    magic_fromtracee);

	DPRINTF("Write new lookup_me_totracee to PID=%d from tracer (PID=%d)\n",
	    child, getpid());
	ATF_REQUIRE
	    (ptrace(PT_WRITE_D, child, &lookup_me_totracee, magic_totracee)
	    != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_d_write_d_handshake2);
ATF_TC_HEAD(read_d_write_d_handshake2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_WRITE_D with PT_READ_D handshake");
}

ATF_TC_BODY(read_d_write_d_handshake2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me_fromtracee = 0;
	const int magic_fromtracee = (int)random();
	int lookup_me_totracee = 0;
	const int magic_totracee = (int)random();
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		lookup_me_fromtracee = magic_fromtracee;

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(lookup_me_totracee, magic_totracee);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Write new lookup_me_totracee to PID=%d from tracer (PID=%d)\n",
	    child, getpid());
	ATF_REQUIRE
	    (ptrace(PT_WRITE_D, child, &lookup_me_totracee, magic_totracee)
	    != -1);

	DPRINTF("Read new lookup_me_fromtracee PID=%d by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me_fromtracee =
	    ptrace(PT_READ_D, child, &lookup_me_fromtracee, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me_fromtracee, magic_fromtracee,
	    "got value %#x != expected %#x", lookup_me_fromtracee,
	    magic_fromtracee);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

/* These dummy functions are used to be copied with ptrace(2) calls */
static int __used
dummy_fn1(int a, int b, int c, int d)
{

	a *= 1;
	b += 2;
	c -= 3;
	d /= 4;

	return a + b * c - d;
}

static int __used
dummy_fn2(int a, int b, int c, int d)
{

	a *= 4;
	b += 3;
	c -= 2;
	d /= 1;

	return a + b * c - d;
}

static int __used
dummy_fn3(int a, int b, int c, int d)
{

	a *= 10;
	b += 20;
	c -= 30;
	d /= 40;

	return a + b * c - d;
}

static int __used
dummy_fn4(int a, int b, int c, int d)
{

	a *= 40;
	b += 30;
	c -= 20;
	d /= 10;

	return a + b * c - d;
}

ATF_TC(io_read_i1);
ATF_TC_HEAD(io_read_i1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_I and len = sizeof(uint8_t)");
}

ATF_TC_BODY(io_read_i1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint8_t lookup_me = 0;
	uint8_t magic;
	memcpy(&magic, dummy_fn1, sizeof(magic));
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_I,
		.piod_offs = dummy_fn1,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx8 " != expected %" PRIx8, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_i2);
ATF_TC_HEAD(io_read_i2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_I and len = sizeof(uint16_t)");
}

ATF_TC_BODY(io_read_i2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint16_t lookup_me = 0;
	uint16_t magic;
	memcpy(&magic, dummy_fn1, sizeof(magic));
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_I,
		.piod_offs = dummy_fn1,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx16 " != expected %" PRIx16, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_i3);
ATF_TC_HEAD(io_read_i3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_I and len = sizeof(uint32_t)");
}

ATF_TC_BODY(io_read_i3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint32_t lookup_me = 0;
	uint32_t magic;
	memcpy(&magic, dummy_fn1, sizeof(magic));
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_I,
		.piod_offs = dummy_fn1,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx32 " != expected %" PRIx32, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(io_read_i4);
ATF_TC_HEAD(io_read_i4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_IO with PIOD_READ_I and len = sizeof(uint64_t)");
}

ATF_TC_BODY(io_read_i4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint64_t lookup_me = 0;
	uint64_t magic;
	memcpy(&magic, dummy_fn1, sizeof(magic));
	struct ptrace_io_desc io = {
		.piod_op = PIOD_READ_I,
		.piod_offs = dummy_fn1,
		.piod_addr = &lookup_me,
		.piod_len = sizeof(lookup_me)
	};
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %" PRIx64 " != expected %" PRIx64, lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_i1);
ATF_TC_HEAD(read_i1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_I called once");
}

ATF_TC_BODY(read_i1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me = 0;
	int magic;
	memcpy(&magic, dummy_fn1, sizeof(magic));
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me = ptrace(PT_READ_I, child, dummy_fn1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me, magic,
	    "got value %#x != expected %#x", lookup_me, magic);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_i2);
ATF_TC_HEAD(read_i2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_I called twice");
}

ATF_TC_BODY(read_i2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int magic1;
	int magic2;
	memcpy(&magic1, dummy_fn1, sizeof(magic1));
	memcpy(&magic2, dummy_fn2, sizeof(magic2));
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me1 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me1 = ptrace(PT_READ_I, child, dummy_fn1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me1, magic1,
	    "got value %#x != expected %#x", lookup_me1, magic1);

	DPRINTF("Read new lookup_me2 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me2 = ptrace(PT_READ_I, child, dummy_fn2, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me2, magic2,
	    "got value %#x != expected %#x", lookup_me2, magic2);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_i3);
ATF_TC_HEAD(read_i3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_I called three times");
}

ATF_TC_BODY(read_i3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int lookup_me3 = 0;
	int magic1;
	int magic2;
	int magic3;
	memcpy(&magic1, dummy_fn1, sizeof(magic1));
	memcpy(&magic2, dummy_fn2, sizeof(magic2));
	memcpy(&magic3, dummy_fn3, sizeof(magic3));
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me1 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me1 = ptrace(PT_READ_I, child, dummy_fn1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me1, magic1,
	    "got value %#x != expected %#x", lookup_me1, magic1);

	DPRINTF("Read new lookup_me2 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me2 = ptrace(PT_READ_I, child, dummy_fn2, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me2, magic2,
	    "got value %#x != expected %#x", lookup_me2, magic2);

	DPRINTF("Read new lookup_me3 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me3 = ptrace(PT_READ_I, child, dummy_fn3, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me3, magic3,
	    "got value %#x != expected %#x", lookup_me3, magic3);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(read_i4);
ATF_TC_HEAD(read_i4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_READ_I called four times");
}

ATF_TC_BODY(read_i4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	int lookup_me1 = 0;
	int lookup_me2 = 0;
	int lookup_me3 = 0;
	int lookup_me4 = 0;
	int magic1;
	int magic2;
	int magic3;
	int magic4;
	memcpy(&magic1, dummy_fn1, sizeof(magic1));
	memcpy(&magic2, dummy_fn2, sizeof(magic2));
	memcpy(&magic3, dummy_fn3, sizeof(magic3));
	memcpy(&magic4, dummy_fn4, sizeof(magic4));
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

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Read new lookup_me1 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me1 = ptrace(PT_READ_I, child, dummy_fn1, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me1, magic1,
	    "got value %#x != expected %#x", lookup_me1, magic1);

	DPRINTF("Read new lookup_me2 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me2 = ptrace(PT_READ_I, child, dummy_fn2, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me2, magic2,
	    "got value %#x != expected %#x", lookup_me2, magic2);

	DPRINTF("Read new lookup_me3 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me3 = ptrace(PT_READ_I, child, dummy_fn3, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me3, magic3,
	    "got value %#x != expected %#x", lookup_me3, magic3);

	DPRINTF("Read new lookup_me4 from tracee (PID=%d) by tracer (PID=%d)\n",
	    child, getpid());
	errno = 0;
	lookup_me4 = ptrace(PT_READ_I, child, dummy_fn4, 0);
	SYSCALL_REQUIRE_ERRNO(errno, 0);

	ATF_REQUIRE_EQ_MSG(lookup_me4, magic4,
	    "got value %#x != expected %#x", lookup_me4, magic4);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(HAVE_GPREGS)
ATF_TC(regs1);
ATF_TC_HEAD(regs1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETREGS call without further steps");
}

ATF_TC_BODY(regs1, tc)
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

#if defined(HAVE_GPREGS)
ATF_TC(regs2);
ATF_TC_HEAD(regs2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETREGS call and retrieve PC");
}

ATF_TC_BODY(regs2, tc)
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

	DPRINTF("Retrieved PC=%" PRIxREGISTER "\n", PTRACE_REG_PC(&r));

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

#if defined(HAVE_GPREGS)
ATF_TC(regs3);
ATF_TC_HEAD(regs3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETREGS call and retrieve SP");
}

ATF_TC_BODY(regs3, tc)
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

	DPRINTF("Retrieved SP=%" PRIxREGISTER "\n", PTRACE_REG_SP(&r));

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

#if defined(HAVE_GPREGS)
ATF_TC(regs4);
ATF_TC_HEAD(regs4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETREGS call and retrieve INTRV");
}

ATF_TC_BODY(regs4, tc)
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

	DPRINTF("Retrieved INTRV=%" PRIxREGISTER "\n", PTRACE_REG_INTRV(&r));

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

#if defined(HAVE_GPREGS)
ATF_TC(regs5);
ATF_TC_HEAD(regs5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_GETREGS and PT_SETREGS calls without changing regs");
}

ATF_TC_BODY(regs5, tc)
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

	DPRINTF("Call SETREGS for the child process (without changed regs)\n");
	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &r, 0) != -1);

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

#if defined(HAVE_FPREGS)
ATF_TC(fpregs1);
ATF_TC_HEAD(fpregs1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify plain PT_GETFPREGS call without further steps");
}

ATF_TC_BODY(fpregs1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct fpreg r;

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

	DPRINTF("Call GETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &r, 0) != -1);

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

#if defined(HAVE_FPREGS)
ATF_TC(fpregs2);
ATF_TC_HEAD(fpregs2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_GETFPREGS and PT_SETFPREGS calls without changing "
	    "regs");
}

ATF_TC_BODY(fpregs2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct fpreg r;

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

	DPRINTF("Call GETFPREGS for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_GETFPREGS, child, &r, 0) != -1);

	DPRINTF("Call SETFPREGS for the child (without changed regs)\n");
	SYSCALL_REQUIRE(ptrace(PT_SETFPREGS, child, &r, 0) != -1);

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

#if defined(PT_STEP)
static void
ptrace_step(int N, int setstep)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	int happy;

#if defined(__arm__)
	/* PT_STEP not supported on arm 32-bit */
	atf_tc_expect_fail("PR kern/52119");
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

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

		if (setstep) {
			SYSCALL_REQUIRE(ptrace(PT_CLEARSTEP, child, 0, 0) != -1);
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
#endif

#if defined(PT_STEP)
ATF_TC(step1);
ATF_TC_HEAD(step1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify single PT_STEP call");
}

ATF_TC_BODY(step1, tc)
{
	ptrace_step(1, 0);
}
#endif

#if defined(PT_STEP)
ATF_TC(step2);
ATF_TC_HEAD(step2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_STEP called twice");
}

ATF_TC_BODY(step2, tc)
{
	ptrace_step(2, 0);
}
#endif

#if defined(PT_STEP)
ATF_TC(step3);
ATF_TC_HEAD(step3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_STEP called three times");
}

ATF_TC_BODY(step3, tc)
{
	ptrace_step(3, 0);
}
#endif

#if defined(PT_STEP)
ATF_TC(step4);
ATF_TC_HEAD(step4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_STEP called four times");
}

ATF_TC_BODY(step4, tc)
{
	ptrace_step(4, 0);
}
#endif

#if defined(PT_STEP)
ATF_TC(setstep1);
ATF_TC_HEAD(setstep1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify single PT_SETSTEP call");
}

ATF_TC_BODY(setstep1, tc)
{
	ptrace_step(1, 1);
}
#endif

#if defined(PT_STEP)
ATF_TC(setstep2);
ATF_TC_HEAD(setstep2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_SETSTEP called twice");
}

ATF_TC_BODY(setstep2, tc)
{
	ptrace_step(2, 1);
}
#endif

#if defined(PT_STEP)
ATF_TC(setstep3);
ATF_TC_HEAD(setstep3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_SETSTEP called three times");
}

ATF_TC_BODY(setstep3, tc)
{
	ptrace_step(3, 1);
}
#endif

#if defined(PT_STEP)
ATF_TC(setstep4);
ATF_TC_HEAD(setstep4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify PT_SETSTEP called four times");
}

ATF_TC_BODY(setstep4, tc)
{
	ptrace_step(4, 1);
}
#endif

ATF_TC(kill1);
ATF_TC_HEAD(kill1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PT_CONTINUE with SIGKILL terminates child");
}

ATF_TC_BODY(kill1, tc)
{
	const int sigval = SIGSTOP, sigsent = SIGKILL;
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

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigsent) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, sigsent, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(kill2);
ATF_TC_HEAD(kill2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that PT_KILL terminates child");
}

ATF_TC_BODY(kill2, tc)
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

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_KILL, child, (void*)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(lwpinfo1);
ATF_TC_HEAD(lwpinfo1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify basic LWPINFO call for single thread (PT_TRACE_ME)");
}

ATF_TC_BODY(lwpinfo1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_lwpinfo info = {0, 0};

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

	DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Assert that there exists a thread\n");
	ATF_REQUIRE(info.pl_lwpid > 0);

	DPRINTF("Assert that lwp thread %d received event PL_EVENT_SIGNAL\n",
	    info.pl_lwpid);
	ATF_REQUIRE_EQ_MSG(info.pl_event, PL_EVENT_SIGNAL,
	    "Received event %d != expected event %d",
	    info.pl_event, PL_EVENT_SIGNAL);

	DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Assert that there are no more lwp threads in child\n");
	ATF_REQUIRE_EQ(info.pl_lwpid, 0);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(TWAIT_HAVE_PID)
ATF_TC(lwpinfo2);
ATF_TC_HEAD(lwpinfo2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify basic LWPINFO call for single thread (PT_ATTACH from "
	    "tracer)");
}

ATF_TC_BODY(lwpinfo2, tc)
{
	struct msg_fds parent_tracee, parent_tracer;
	const int exitval_tracee = 5;
	const int exitval_tracer = 10;
	pid_t tracee, tracer, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_lwpinfo info = {0, 0};

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	SYSCALL_REQUIRE(msg_open(&parent_tracer) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {

		/* Wait for message from the parent */
		CHILD_TO_PARENT("tracee ready", parent_tracee, msg);
		CHILD_FROM_PARENT("tracee exit", parent_tracee, msg);

		_exit(exitval_tracee);
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

		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
		FORKEE_ASSERT(ptrace(PT_LWPINFO, tracee, &info, sizeof(info))
		    != -1);

		DPRINTF("Assert that there exists a thread\n");
		FORKEE_ASSERTX(info.pl_lwpid > 0);

		DPRINTF("Assert that lwp thread %d received event "
		    "PL_EVENT_SIGNAL\n", info.pl_lwpid);
		FORKEE_ASSERT_EQ(info.pl_event, PL_EVENT_SIGNAL);

		DPRINTF("Before calling ptrace(2) with PT_LWPINFO for child\n");
		FORKEE_ASSERT(ptrace(PT_LWPINFO, tracee, &info, sizeof(info))
		    != -1);

		DPRINTF("Assert that there are no more lwp threads in child\n");
		FORKEE_ASSERTX(info.pl_lwpid == 0);

		/* Resume tracee with PT_CONTINUE */
		FORKEE_ASSERT(ptrace(PT_CONTINUE, tracee, (void *)1, 0) != -1);

		/* Inform parent that tracer has attached to tracee */
		CHILD_TO_PARENT("tracer ready", parent_tracer, msg);
		/* Wait for parent */
		CHILD_FROM_PARENT("tracer wait", parent_tracer, msg);

		/* Wait for tracee and assert that it exited */
		FORKEE_REQUIRE_SUCCESS(
		    wpid = TWAIT_GENERIC(tracee, &status, 0), tracee);

		forkee_status_exited(status, exitval_tracee);

		DPRINTF("Before exiting of the tracer process\n");
		_exit(exitval_tracer);
	}

	DPRINTF("Wait for the tracer to attach to the tracee\n");
	PARENT_FROM_CHILD("tracer ready", parent_tracer, msg);

	DPRINTF("Resume the tracee and let it exit\n");
	PARENT_TO_CHILD("tracee exit", parent_tracee, msg);

	DPRINTF("Detect that tracee is zombie\n");
	await_zombie(tracee);

	DPRINTF("Assert that there is no status about tracee - "
	    "Tracer must detect zombie first - calling %s()\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(
	    wpid = TWAIT_GENERIC(tracee, &status, WNOHANG), 0);

	DPRINTF("Resume the tracer and let it detect exited tracee\n");
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

	validate_status_exited(status, exitval_tracee);

	msg_close(&parent_tracer);
	msg_close(&parent_tracee);
}
#endif

ATF_TC(siginfo1);
ATF_TC_HEAD(siginfo1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify basic PT_GET_SIGINFO call for SIGTRAP from tracee");
}

ATF_TC_BODY(siginfo1, tc)
{
	const int exitval = 5;
	const int sigval = SIGTRAP;
	pid_t child, wpid;
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
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(siginfo2);
ATF_TC_HEAD(siginfo2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify basic PT_GET_SIGINFO and PT_SET_SIGINFO calls without "
	    "modification of SIGINT from tracee");
}

static int siginfo2_caught = 0;

static void
siginfo2_sighandler(int sig)
{
	FORKEE_ASSERT_EQ(sig, SIGINT);

	++siginfo2_caught;
}

ATF_TC_BODY(siginfo2, tc)
{
	const int exitval = 5;
	const int sigval = SIGINT;
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

		sa.sa_handler = siginfo2_sighandler;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sigval, &sa, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(siginfo2_caught, 1);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before calling ptrace(2) with PT_SET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_SET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigval) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(siginfo3);
ATF_TC_HEAD(siginfo3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify basic PT_GET_SIGINFO and PT_SET_SIGINFO calls with "
	    "setting signal to new value");
}

static int siginfo3_caught = 0;

static void
siginfo3_sigaction(int sig, siginfo_t *info, void *ctx)
{
	FORKEE_ASSERT_EQ(sig, SIGTRAP);

	FORKEE_ASSERT_EQ(info->si_signo, SIGTRAP);
	FORKEE_ASSERT_EQ(info->si_code, TRAP_BRKPT);

	++siginfo3_caught;
}

ATF_TC_BODY(siginfo3, tc)
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

		sa.sa_sigaction = siginfo3_sigaction;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(sigfaked, &sa, NULL) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(siginfo3_caught, 1);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	DPRINTF("Before setting new faked signal to signo=%d si_code=%d\n",
	    sigfaked, sicodefaked);
	info.psi_siginfo.si_signo = sigfaked;
	info.psi_siginfo.si_code = sicodefaked;

	DPRINTF("Before calling ptrace(2) with PT_SET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_SET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigfaked);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, sicodefaked);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, sigfaked) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(siginfo4);
ATF_TC_HEAD(siginfo4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Detect SIGTRAP TRAP_EXEC from tracee");
}

ATF_TC_BODY(siginfo4, tc)
{
	const int sigval = SIGTRAP;
	pid_t child, wpid;
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

		DPRINTF("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

		FORKEE_ASSERT(0 && "Not reached");
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

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

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(TWAIT_HAVE_PID)
ATF_TC(siginfo5);
ATF_TC_HEAD(siginfo5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that fork(2) is intercepted by ptrace(2) with EVENT_MASK "
	    "set to PTRACE_FORK and reports correct signal information");
}

ATF_TC_BODY(siginfo5, tc)
{
	const int exitval = 5;
	const int exitval2 = 15;
	const int sigval = SIGSTOP;
	pid_t child, child2, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	struct ptrace_siginfo info;

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT((child2 = fork()) != -1);

		if (child2 == 0)
			_exit(exitval2);

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
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Enable PTRACE_FORK in EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_FORK;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
        DPRINTF("We expect two SIGTRAP events, for child %d (TRAP_CHLD, "
               "pe_report_event=PTRACE_FORK, state.pe_other_pid=child2) and "
               "for child2 (TRAP_CHLD, pe_report_event=PTRACE_FORK, "
                "state.pe_other_pid=child)\n", child);
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child %d\n", TWAIT_FNAME, child);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_CHLD);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_FORK);

	child2 = state.pe_other_pid;
	DPRINTF("Reported PTRACE_FORK event with forkee %d\n", child2);

	DPRINTF("Before calling %s() for the forkee %d of the child %d\n",
	    TWAIT_FNAME, child2, child);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_CHLD);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_FORK);
	ATF_REQUIRE_EQ(state.pe_other_pid, child);

	DPRINTF("Before resuming the forkee process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the forkee - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);

	validate_status_exited(status, exitval2);

	DPRINTF("Before calling %s() for the forkee - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD,
	    wpid = TWAIT_GENERIC(child2, &status, 0));

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGCHLD\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGCHLD);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGCHLD);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, CLD_EXITED);

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
#endif

#if defined(PT_STEP)
ATF_TC(siginfo6);
ATF_TC_HEAD(siginfo6, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify single PT_STEP call with signal information check");
}

ATF_TC_BODY(siginfo6, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	int happy;
	struct ptrace_siginfo info;

#if defined(__arm__)
	/* PT_STEP not supported on arm 32-bit */
	atf_tc_expect_fail("PR kern/52119");
#endif

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		happy = check_happy(100);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(happy, check_happy(100));

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent (use PT_STEP)\n");
	SYSCALL_REQUIRE(ptrace(PT_STEP, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Before checking siginfo_t\n");
	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, SIGTRAP);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_TRACE);

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

volatile lwpid_t the_lwp_id = 0;

static void
lwp_main_func(void *arg)
{
	the_lwp_id = _lwp_self();
	_lwp_exit();
}

ATF_TC(lwp_create1);
ATF_TC_HEAD(lwp_create1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_CREATE");
}

ATF_TC_BODY(lwp_create1, tc)
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
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

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

ATF_TC(lwp_exit1);
ATF_TC_HEAD(lwp_exit1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that 1 LWP creation is intercepted by ptrace(2) with "
	    "EVENT_MASK set to PTRACE_LWP_EXIT");
}

ATF_TC_BODY(lwp_exit1, tc)
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
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_EXIT;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_EXIT);

	lid = state.pe_lwp;
	DPRINTF("Reported PTRACE_LWP_EXIT event with lid %d\n", lid);

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

ATF_TC(signal1);
ATF_TC_HEAD(signal1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking single unrelated signal does not stop tracer "
	    "from catching other signals");
}

ATF_TC_BODY(signal1, tc)
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

ATF_TC(signal2);
ATF_TC_HEAD(signal2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee stops tracer from "
	    "catching this raised signal");
}

ATF_TC_BODY(signal2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
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

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

ATF_TC(signal3);
ATF_TC_HEAD(signal3, tc)
{
	atf_tc_set_md_var(tc, "timeout", "5");
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching software breakpoints");
}

ATF_TC_BODY(signal3, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
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

		DPRINTF("Before raising software breakpoint from child\n");

#ifdef PTRACE_BREAKPOINT_ASM
		PTRACE_BREAKPOINT_ASM;
#else
		/* port me */
#endif

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

	validate_status_stopped(status, sigmasked);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(PT_STEP)
ATF_TC(signal4);
ATF_TC_HEAD(signal4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching single step trap");
}

ATF_TC_BODY(signal4, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	int happy;

#if defined(__arm__)
	/* PT_STEP not supported on arm 32-bit */
	atf_tc_expect_fail("PR kern/51918 PR kern/52119");
#endif

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		happy = check_happy(100);

		sigemptyset(&intmask);
		sigaddset(&intmask, sigmasked);
		sigprocmask(SIG_BLOCK, &intmask, NULL);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(happy, check_happy(100));

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_STEP, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigmasked);

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

ATF_TC(signal5);
ATF_TC_HEAD(signal5, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching exec() breakpoint");
}

ATF_TC_BODY(signal5, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;

	atf_tc_expect_fail("wrong signal");

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

		DPRINTF("Before calling execve(2) from child\n");
		execlp("/bin/echo", "/bin/echo", NULL);

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

	validate_status_stopped(status, sigmasked);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#if defined(TWAIT_HAVE_PID)
ATF_TC(signal6);
ATF_TC_HEAD(signal6, tc)
{
	atf_tc_set_md_var(tc, "timeout", "5");
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching PTRACE_FORK breakpoint");
}

ATF_TC_BODY(signal6, tc)
{
	const int exitval = 5;
	const int exitval2 = 15;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, child2, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	atf_tc_expect_fail("PR kern/51918");

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

		FORKEE_ASSERT((child2 = fork()) != -1);

		if (child2 == 0)
			_exit(exitval2);

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

	DPRINTF("Enable PTRACE_FORK in EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_FORK;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigmasked);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_FORK);

	child2 = state.pe_other_pid;
	DPRINTF("Reported PTRACE_FORK event with forkee %d\n", child2);

	DPRINTF("Before calling %s() for the child2\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);

	validate_status_stopped(status, SIGTRAP);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_FORK);
	ATF_REQUIRE_EQ(state.pe_other_pid, child);

	DPRINTF("Before resuming the forkee process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the forkee - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);                                                                                                                                         

	validate_status_exited(status, exitval2);

	DPRINTF("Before calling %s() for the forkee - expected no process\n",
	    TWAIT_FNAME);
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
#endif

#if defined(TWAIT_HAVE_PID)
ATF_TC(signal7);
ATF_TC_HEAD(signal7, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching PTRACE_VFORK breakpoint");
}

ATF_TC_BODY(signal7, tc)
{
	const int exitval = 5;
	const int exitval2 = 15;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, child2, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	atf_tc_expect_fail("PR kern/51918");

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

		FORKEE_ASSERT((child2 = fork()) != -1);

		if (child2 == 0)
			_exit(exitval2);

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

	DPRINTF("Enable PTRACE_VFORK in EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_VFORK;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1 || errno == ENOTSUP);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigmasked);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK);

	child2 = state.pe_other_pid;
	DPRINTF("Reported PTRACE_VFORK event with forkee %d\n", child2);

	DPRINTF("Before calling %s() for the child2\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);

	validate_status_stopped(status, SIGTRAP);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child2, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK);
	ATF_REQUIRE_EQ(state.pe_other_pid, child);

	DPRINTF("Before resuming the forkee process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child2, (void *)1, 0) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the forkee - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child2, &status, 0),
	    child2);                                                                                                                                         

	validate_status_exited(status, exitval2);

	DPRINTF("Before calling %s() for the forkee - expected no process\n",
	    TWAIT_FNAME);
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
#endif

ATF_TC(signal8);
ATF_TC_HEAD(signal8, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching PTRACE_VFORK_DONE breakpoint");
}

ATF_TC_BODY(signal8, tc)
{
	const int exitval = 5;
	const int exitval2 = 15;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, child2, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	atf_tc_expect_fail("PR kern/51918");

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

		FORKEE_ASSERT((child2 = vfork()) != -1);

		if (child2 == 0)
			_exit(exitval2);

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

	DPRINTF("Enable PTRACE_VFORK_DONE in EVENT_MASK for the child %d\n",
	    child);
	event.pe_set_event = PTRACE_VFORK_DONE;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigmasked);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);
	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_VFORK_DONE);

	child2 = state.pe_other_pid;
	DPRINTF("Reported PTRACE_VFORK_DONE event with forkee %d\n", child2);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

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

ATF_TC(signal9);
ATF_TC_HEAD(signal9, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching PTRACE_LWP_CREATE breakpoint");
}

ATF_TC_BODY(signal9, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;

	atf_tc_expect_fail("PR kern/51918");

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

		DPRINTF("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		DPRINTF("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

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

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigmasked);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_CREATE);

	lid = state.pe_lwp;
	DPRINTF("Reported PTRACE_LWP_CREATE event with lid %d\n", lid);

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

ATF_TC(signal10);
ATF_TC_HEAD(signal10, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that masking SIGTRAP in tracee does not stop tracer from "
	    "catching PTRACE_LWP_EXIT breakpoint");
}

ATF_TC_BODY(signal10, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	const int sigmasked = SIGTRAP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t intmask;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;

	atf_tc_expect_fail("PR kern/51918");

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

		DPRINTF("Before allocating memory for stack in child\n");
		FORKEE_ASSERT((stack = malloc(ssize)) != NULL);

		DPRINTF("Before making context for new lwp in child\n");
		_lwp_makecontext(&uc, lwp_main_func, NULL, NULL, stack, ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Set empty EVENT_MASK for the child %d\n", child);
	event.pe_set_event = PTRACE_LWP_EXIT;
	SYSCALL_REQUIRE(ptrace(PT_SET_EVENT_MASK, child, &event, elen) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigmasked);

	SYSCALL_REQUIRE(ptrace(PT_GET_PROCESS_STATE, child, &state, slen) != -1);

	ATF_REQUIRE_EQ(state.pe_report_event, PTRACE_LWP_EXIT);

	lid = state.pe_lwp;
	DPRINTF("Reported PTRACE_LWP_EXIT event with lid %d\n", lid);

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

static void
lwp_main_stop(void *arg)
{
	the_lwp_id = _lwp_self();

	raise(SIGTRAP);

	_lwp_exit();
}

ATF_TC(suspend1);
ATF_TC_HEAD(suspend1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that a thread can be suspended by a debugger and later "
	    "resumed by a tracee");
}

ATF_TC_BODY(suspend1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	struct ptrace_lwpinfo pl;
	struct ptrace_siginfo psi;
	volatile int go = 0;

	// Feature pending for refactoring
	atf_tc_expect_fail("PR kern/51995");

	// Hangs with qemu
	ATF_REQUIRE(0 && "In order to get reliable failure, abort");

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
		_lwp_makecontext(&uc, lwp_main_stop, NULL, NULL, stack, ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		while (go == 0)
			continue;

		raise(SIGINT);

		FORKEE_ASSERT(_lwp_continue(lid) == 0);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

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

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before reading siginfo and lwpid_t\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &psi, sizeof(psi)) != -1);

	DPRINTF("Before suspending LWP %d\n", psi.psi_lwpid);
	SYSCALL_REQUIRE(ptrace(PT_SUSPEND, child, NULL, psi.psi_lwpid) != -1);

        DPRINTF("Write new go to tracee (PID=%d) from tracer (PID=%d)\n",
	    child, getpid());
	SYSCALL_REQUIRE(ptrace(PT_WRITE_D, child, __UNVOLATILE(&go), 1) != -1);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGINT\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGINT);

	pl.pl_lwpid = 0;

	SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &pl, sizeof(pl)) != -1);
	while (pl.pl_lwpid != 0) {

		SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &pl, sizeof(pl)) != -1);
		switch (pl.pl_lwpid) {
		case 1:
			ATF_REQUIRE_EQ(pl.pl_event, PL_EVENT_SIGNAL);
			break;
		case 2:
			ATF_REQUIRE_EQ(pl.pl_event, PL_EVENT_SUSPENDED);
			break;
		}
	}

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

ATF_TC(suspend2);
ATF_TC_HEAD(suspend2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that the while the only thread within a process is "
	    "suspended, the whole process cannot be unstopped");
}

ATF_TC_BODY(suspend2, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo psi;

	// Feature pending for refactoring
	atf_tc_expect_fail("PR kern/51995");

	// Hangs with qemu
	ATF_REQUIRE(0 && "In order to get reliable failure, abort");

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

ATF_TC(resume1);
ATF_TC_HEAD(resume1, tc)
{
	atf_tc_set_md_var(tc, "timeout", "5");
	atf_tc_set_md_var(tc, "descr",
	    "Verify that a thread can be suspended by a debugger and later "
	    "resumed by the debugger");
}

ATF_TC_BODY(resume1, tc)
{
	struct msg_fds fds;
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	ucontext_t uc;
	lwpid_t lid;
	static const size_t ssize = 16*1024;
	void *stack;
	struct ptrace_lwpinfo pl;
	struct ptrace_siginfo psi;

	// Feature pending for refactoring
	atf_tc_expect_fail("PR kern/51995");

	// Hangs with qemu
	ATF_REQUIRE(0 && "In order to get reliable failure, abort");

	SYSCALL_REQUIRE(msg_open(&fds) == 0);

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
		_lwp_makecontext(&uc, lwp_main_stop, NULL, NULL, stack, ssize);

		DPRINTF("Before creating new in child\n");
		FORKEE_ASSERT(_lwp_create(&uc, 0, &lid) == 0);

		CHILD_TO_PARENT("Message", fds, msg);

		raise(SIGINT);

		DPRINTF("Before waiting for lwp %d to exit\n", lid);
		FORKEE_ASSERT(_lwp_wait(lid, NULL) == 0);

		DPRINTF("Before verifying that reported %d and running lid %d "
		    "are the same\n", lid, the_lwp_id);
		FORKEE_ASSERT_EQ(lid, the_lwp_id);

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

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGTRAP\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGTRAP);

	DPRINTF("Before reading siginfo and lwpid_t\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &psi, sizeof(psi)) != -1);

	DPRINTF("Before suspending LWP %d\n", psi.psi_lwpid);
	SYSCALL_REQUIRE(ptrace(PT_SUSPEND, child, NULL, psi.psi_lwpid) != -1);

	PARENT_FROM_CHILD("Message", fds, msg);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child - expected stopped "
	    "SIGINT\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, SIGINT);

	pl.pl_lwpid = 0;

	SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &pl, sizeof(pl)) != -1);
	while (pl.pl_lwpid != 0) {
		SYSCALL_REQUIRE(ptrace(PT_LWPINFO, child, &pl, sizeof(pl)) != -1);
		switch (pl.pl_lwpid) {
		case 1:
			ATF_REQUIRE_EQ(pl.pl_event, PL_EVENT_SIGNAL);
			break;
		case 2:
			ATF_REQUIRE_EQ(pl.pl_event, PL_EVENT_SUSPENDED);
			break;
		}
	}

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

	msg_close(&fds);

	DPRINTF("XXX: Test worked this time but for consistency timeout it\n");
	sleep(10);
}

ATF_TC(syscall1);
ATF_TC_HEAD(syscall1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify that getpid(2) can be traced with PT_SYSCALL");
}

ATF_TC_BODY(syscall1, tc)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
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

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		syscall(SYS_getpid);

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
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, TRAP_SCX);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

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

#if defined(TWAIT_HAVE_PID)
ATF_TC(race1);
ATF_TC_HEAD(race1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Assert that await_zombie() in attach1 always finds a single "
	    "process and no other error is reported");
}

ATF_TC_BODY(race1, tc)
{
	time_t start, end;
	double diff;
	unsigned long N = 0;

	/* Reuse this test with attach1 */

	start = time(NULL);
	while (true) {
		DPRINTF("Step: %lu\n", N);
		attach1_raw(true);
		end = time(NULL);
		diff = difftime(end, start);
		if (diff >= 5.0)
			break;
		++N;
	}
	DPRINTF("Iterations: %lu\n", N);
}
#endif

#include "t_ptrace_amd64_wait.h"
#include "t_ptrace_i386_wait.h"
#include "t_ptrace_x86_wait.h"

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	ATF_TP_ADD_TC(tp, traceme_raise1);
	ATF_TP_ADD_TC(tp, traceme_raise2);
	ATF_TP_ADD_TC(tp, traceme_raise3);
	ATF_TP_ADD_TC(tp, traceme_raise4);
	ATF_TP_ADD_TC(tp, traceme_raise5);

	ATF_TP_ADD_TC(tp, traceme_sighandler_catch1);
	ATF_TP_ADD_TC(tp, traceme_sighandler_catch2);
	ATF_TP_ADD_TC(tp, traceme_sighandler_catch3);

	ATF_TP_ADD_TC(tp, traceme_signal_nohandler1);
	ATF_TP_ADD_TC(tp, traceme_signal_nohandler2);
	ATF_TP_ADD_TC(tp, traceme_signal_nohandler3);
	ATF_TP_ADD_TC(tp, traceme_signal_nohandler4);
	ATF_TP_ADD_TC(tp, traceme_signal_nohandler5);

	ATF_TP_ADD_TC(tp, traceme_pid1_parent);

	ATF_TP_ADD_TC(tp, traceme_vfork_raise1);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise2);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise3);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise4);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise5);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise6);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise7);
	ATF_TP_ADD_TC(tp, traceme_vfork_raise8);

	ATF_TP_ADD_TC(tp, traceme_vfork_breakpoint);

	ATF_TP_ADD_TC(tp, traceme_vfork_exec);

	ATF_TP_ADD_TC_HAVE_PID(tp, attach1);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach2);
	ATF_TP_ADD_TC(tp, attach3);
	ATF_TP_ADD_TC(tp, attach4);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach5);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach6);
	ATF_TP_ADD_TC_HAVE_PID(tp, attach7);

	ATF_TP_ADD_TC(tp, eventmask1);
	ATF_TP_ADD_TC(tp, eventmask2);
	ATF_TP_ADD_TC(tp, eventmask3);
	ATF_TP_ADD_TC(tp, eventmask4);
	ATF_TP_ADD_TC(tp, eventmask5);
	ATF_TP_ADD_TC(tp, eventmask6);

	ATF_TP_ADD_TC(tp, fork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork4);
	ATF_TP_ADD_TC(tp, fork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, fork8);

	ATF_TP_ADD_TC(tp, vfork1);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork2);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork3);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork4);
	ATF_TP_ADD_TC(tp, vfork5);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork6);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork7);
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork8);

	ATF_TP_ADD_TC(tp, io_read_d1);
	ATF_TP_ADD_TC(tp, io_read_d2);
	ATF_TP_ADD_TC(tp, io_read_d3);
	ATF_TP_ADD_TC(tp, io_read_d4);

	ATF_TP_ADD_TC(tp, io_write_d1);
	ATF_TP_ADD_TC(tp, io_write_d2);
	ATF_TP_ADD_TC(tp, io_write_d3);
	ATF_TP_ADD_TC(tp, io_write_d4);

	ATF_TP_ADD_TC(tp, read_d1);
	ATF_TP_ADD_TC(tp, read_d2);
	ATF_TP_ADD_TC(tp, read_d3);
	ATF_TP_ADD_TC(tp, read_d4);

	ATF_TP_ADD_TC(tp, write_d1);
	ATF_TP_ADD_TC(tp, write_d2);
	ATF_TP_ADD_TC(tp, write_d3);
	ATF_TP_ADD_TC(tp, write_d4);

	ATF_TP_ADD_TC(tp, io_read_d_write_d_handshake1);
	ATF_TP_ADD_TC(tp, io_read_d_write_d_handshake2);

	ATF_TP_ADD_TC(tp, read_d_write_d_handshake1);
	ATF_TP_ADD_TC(tp, read_d_write_d_handshake2);

	ATF_TP_ADD_TC(tp, io_read_i1);
	ATF_TP_ADD_TC(tp, io_read_i2);
	ATF_TP_ADD_TC(tp, io_read_i3);
	ATF_TP_ADD_TC(tp, io_read_i4);

	ATF_TP_ADD_TC(tp, read_i1);
	ATF_TP_ADD_TC(tp, read_i2);
	ATF_TP_ADD_TC(tp, read_i3);
	ATF_TP_ADD_TC(tp, read_i4);

	ATF_TP_ADD_TC(tp, io_read_auxv1);

	ATF_TP_ADD_TC_HAVE_GPREGS(tp, regs1);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, regs2);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, regs3);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, regs4);
	ATF_TP_ADD_TC_HAVE_GPREGS(tp, regs5);

	ATF_TP_ADD_TC_HAVE_FPREGS(tp, fpregs1);
	ATF_TP_ADD_TC_HAVE_FPREGS(tp, fpregs2);

	ATF_TP_ADD_TC_PT_STEP(tp, step1);
	ATF_TP_ADD_TC_PT_STEP(tp, step2);
	ATF_TP_ADD_TC_PT_STEP(tp, step3);
	ATF_TP_ADD_TC_PT_STEP(tp, step4);

	ATF_TP_ADD_TC_PT_STEP(tp, setstep1);
	ATF_TP_ADD_TC_PT_STEP(tp, setstep2);
	ATF_TP_ADD_TC_PT_STEP(tp, setstep3);
	ATF_TP_ADD_TC_PT_STEP(tp, setstep4);

	ATF_TP_ADD_TC(tp, kill1);
	ATF_TP_ADD_TC(tp, kill2);

	ATF_TP_ADD_TC(tp, lwpinfo1);
	ATF_TP_ADD_TC_HAVE_PID(tp, lwpinfo2);

	ATF_TP_ADD_TC(tp, siginfo1);
	ATF_TP_ADD_TC(tp, siginfo2);
	ATF_TP_ADD_TC(tp, siginfo3);
	ATF_TP_ADD_TC(tp, siginfo4);
	ATF_TP_ADD_TC_HAVE_PID(tp, siginfo5);
	ATF_TP_ADD_TC_PT_STEP(tp, siginfo6);

	ATF_TP_ADD_TC(tp, lwp_create1);

	ATF_TP_ADD_TC(tp, lwp_exit1);

	ATF_TP_ADD_TC(tp, signal1);
	ATF_TP_ADD_TC(tp, signal2);
	ATF_TP_ADD_TC(tp, signal3);
	ATF_TP_ADD_TC_PT_STEP(tp, signal4);
	ATF_TP_ADD_TC(tp, signal5);
	ATF_TP_ADD_TC_HAVE_PID(tp, signal6);
	ATF_TP_ADD_TC_HAVE_PID(tp, signal7);
	ATF_TP_ADD_TC(tp, signal8);
	ATF_TP_ADD_TC(tp, signal9);
	ATF_TP_ADD_TC(tp, signal10);

	ATF_TP_ADD_TC(tp, suspend1);
	ATF_TP_ADD_TC(tp, suspend2);

	ATF_TP_ADD_TC(tp, resume1);

	ATF_TP_ADD_TC(tp, syscall1);

	ATF_TP_ADD_TC(tp, syscallemu1);

	ATF_TP_ADD_TC_HAVE_PID(tp, race1);

	ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64();
	ATF_TP_ADD_TCS_PTRACE_WAIT_I386();
	ATF_TP_ADD_TCS_PTRACE_WAIT_X86();

	return atf_no_error();
}
