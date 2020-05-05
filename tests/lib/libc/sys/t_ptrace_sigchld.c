/*	$NetBSD: t_ptrace_sigchld.c,v 1.3 2020/05/05 18:12:20 kamil Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_ptrace_sigchld.c,v 1.3 2020/05/05 18:12:20 kamil Exp $");

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

#include <atf-c.h>

#include "h_macros.h"
#include "msg.h"

#include "t_ptrace_wait.h"

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

static int expected_signo;
static int expected_code;
static int expected_status;
static pid_t expected_pid;

static void
sigchld_action(int sig, siginfo_t *info, void *ctx)
{

	FORKEE_ASSERT_EQ(info->si_signo, expected_signo);
	FORKEE_ASSERT_EQ(info->si_code, expected_code);
	FORKEE_ASSERT_EQ(info->si_uid, getuid());
	FORKEE_ASSERT_EQ(info->si_pid, expected_pid);

	if (WIFEXITED(info->si_status))
		ATF_REQUIRE_EQ(WEXITSTATUS(info->si_status), expected_status);
	else if (WIFSTOPPED(info->si_status))
		ATF_REQUIRE_EQ(WSTOPSIG(info->si_status), expected_status);
	else if (WIFSIGNALED(info->si_status))
		ATF_REQUIRE_EQ(WTERMSIG(info->si_status), expected_status);
/*
	else if (WIFCONTINUED(info->si_status))
		;
*/
}

static void
traceme_raise(int sigval)
{
	const int exitval = 5;
	struct sigaction sa;
	pid_t child;
	struct msg_fds parent_child;
	uint8_t msg = 0xde; /* dummy message for IPC based on pipe(2) */

	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = sigchld_action;
	sa.sa_flags = SA_SIGINFO | SA_NOCLDWAIT;
	sigemptyset(&sa.sa_mask);

	atf_tc_skip("XXX: zombie is not collected before tracer's death");

	SYSCALL_REQUIRE(sigaction(SIGCHLD, &sa, NULL) == 0);

	SYSCALL_REQUIRE(msg_open(&parent_child) == 0);

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		CHILD_FROM_PARENT("raise1 child", parent_child, msg);

		raise(sigval);

		CHILD_TO_PARENT("raise2 child", parent_child, msg);

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

	expected_signo = SIGCHLD;
	expected_pid = child;
	switch (sigval) {
	case SIGKILL:
		expected_code = CLD_KILLED;
		expected_status = SIGKILL;
		break;
	case SIGSTOP:
		expected_code = CLD_STOPPED;
		expected_status = SIGSTOP;
		break;
	default:
		break;
	}	

	PARENT_TO_CHILD("raise1 child", parent_child, msg);

	switch (sigval) {
	case SIGKILL:
		break;
	default:
		PARENT_FROM_CHILD("raise2 child", parent_child, msg);

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

		expected_code = CLD_EXITED;
		expected_status = exitval;

		SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0));

		break;
	}

	await_collected(child); /* XXX: Process is never collected */
}

#define TRACEME_RAISE(test, sig)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify " #sig " followed by _exit(2) in a child");		\
	atf_tc_set_md_var(tc, "timeout", "10");				\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	traceme_raise(sig);						\
}

TRACEME_RAISE(traceme_raise1, SIGKILL) /* non-maskable */
#if notyet
TRACEME_RAISE(traceme_raise2, SIGSTOP) /* non-maskable */
TRACEME_RAISE(traceme_raise3, SIGABRT) /* regular abort trap */
TRACEME_RAISE(traceme_raise4, SIGHUP)  /* hangup */
TRACEME_RAISE(traceme_raise5, SIGCONT) /* continued? */
TRACEME_RAISE(traceme_raise6, SIGTRAP) /* crash signal */
TRACEME_RAISE(traceme_raise7, SIGBUS) /* crash signal */
TRACEME_RAISE(traceme_raise8, SIGILL) /* crash signal */
TRACEME_RAISE(traceme_raise9, SIGFPE) /* crash signal */
TRACEME_RAISE(traceme_raise10, SIGSEGV) /* crash signal */
#endif

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	ATF_TP_ADD_TC(tp, traceme_raise1);
#if notyet
	ATF_TP_ADD_TC(tp, traceme_raise2);
	ATF_TP_ADD_TC(tp, traceme_raise3);
	ATF_TP_ADD_TC(tp, traceme_raise4);
	ATF_TP_ADD_TC(tp, traceme_raise5);
	ATF_TP_ADD_TC(tp, traceme_raise6);
	ATF_TP_ADD_TC(tp, traceme_raise7);
	ATF_TP_ADD_TC(tp, traceme_raise8);
	ATF_TP_ADD_TC(tp, traceme_raise9);
	ATF_TP_ADD_TC(tp, traceme_raise10);
#endif

	return atf_no_error();
}
