/*	$NetBSD: t_ptrace_wait.c,v 1.187 2020/05/05 00:33:37 kamil Exp $	*/

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
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.187 2020/05/05 00:33:37 kamil Exp $");

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
#include "t_ptrace_eventmask_wait.h"
#include "t_ptrace_lwp_wait.h"
#include "t_ptrace_exec_wait.h"
#include "t_ptrace_topology_wait.h"

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
	ATF_TP_ADD_TC(tp, siginfo_set_unmodified);
	ATF_TP_ADD_TC(tp, siginfo_set_faked);

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
	ATF_TP_ADD_TCS_PTRACE_WAIT_EVENTMASK();
	ATF_TP_ADD_TCS_PTRACE_WAIT_LWP();
	ATF_TP_ADD_TCS_PTRACE_WAIT_EXEC();
	ATF_TP_ADD_TCS_PTRACE_WAIT_TOPOLOGY();

	ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64();
	ATF_TP_ADD_TCS_PTRACE_WAIT_I386();
	ATF_TP_ADD_TCS_PTRACE_WAIT_X86();

#else
	ATF_TP_ADD_TC(tp, dummy);
#endif

	return atf_no_error();
}
