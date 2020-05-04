/*	$NetBSD: t_ptrace_wait.c,v 1.181 2020/05/04 23:49:31 kamil Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.181 2020/05/04 23:49:31 kamil Exp $");

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
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_report_event) ==
    sizeof(((siginfo_t *)0)->si_pe_report_event));
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_other_pid) ==
    sizeof(((siginfo_t *)0)->si_pe_other_pid));
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_lwp) ==
    sizeof(((siginfo_t *)0)->si_pe_lwp));
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_other_pid) ==
    sizeof(((struct ptrace_state *)0)->pe_lwp));

#include "h_macros.h"

#include "t_ptrace_wait.h"
#include "msg.h"

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
 *
 * If the note_name ends in '*', then we find the first note that matches
 * the note_name prefix up to the '*' character, e.g.:
 *
 *	NetBSD-CORE@*
 *
 * finds the first note whose name prefix matches "NetBSD-CORE@".
 */
static ssize_t core_find_note(const char *core_path,
    const char *note_name, uint64_t note_type, void *buf, size_t buf_len)
{
	int core_fd;
	Elf *core_elf;
	size_t core_numhdr, i;
	ssize_t ret = -1;
	size_t name_len = strlen(note_name);
	bool prefix_match = false;

	if (note_name[name_len - 1] == '*') {
		prefix_match = true;
		name_len--;
	} else {
		/* note: we assume note name will be null-terminated */
		name_len++;
	}

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
			if (((prefix_match &&
			      note_hdr.n_namesz > name_len) ||
			     (!prefix_match &&
			      note_hdr.n_namesz == name_len)) &&
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
	ATF_CHECK(procinfo.cpi_siglwp > 0);

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

#include "t_ptrace_register_wait.h"
#include "t_ptrace_syscall_wait.h"
#include "t_ptrace_step_wait.h"
#include "t_ptrace_kill_wait.h"
#include "t_ptrace_bytetransfer_wait.h"
#include "t_ptrace_clone_wait.h"
#include "t_ptrace_fork_wait.h"
#include "t_ptrace_signal_wait.h"

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
	ATF_TP_ADD_TC(tp, traceme_pid1_parent);

	ATF_TP_ADD_TC(tp, traceme_vfork_exec);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalmasked_exec);
	ATF_TP_ADD_TC(tp, traceme_vfork_signalignored_exec);

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

	ATF_TP_ADD_TC(tp, threads_and_exec);

	ATF_TP_ADD_TC(tp, suspend_no_deadlock);

	ATF_TP_ADD_TC(tp, resume);

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

	ATF_TP_ADD_TCS_PTRACE_WAIT_REGISTER();
	ATF_TP_ADD_TCS_PTRACE_WAIT_SYSCALL();
	ATF_TP_ADD_TCS_PTRACE_WAIT_STEP();
	ATF_TP_ADD_TCS_PTRACE_WAIT_KILL();
	ATF_TP_ADD_TCS_PTRACE_WAIT_BYTETRANSFER();
	ATF_TP_ADD_TCS_PTRACE_WAIT_CLONE();
	ATF_TP_ADD_TCS_PTRACE_WAIT_FORK();
	ATF_TP_ADD_TCS_PTRACE_WAIT_SIGNAL();

	ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64();
	ATF_TP_ADD_TCS_PTRACE_WAIT_I386();
	ATF_TP_ADD_TCS_PTRACE_WAIT_X86();

#else
	ATF_TP_ADD_TC(tp, dummy);
#endif

	return atf_no_error();
}
