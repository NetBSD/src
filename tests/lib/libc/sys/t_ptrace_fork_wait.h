/*	$NetBSD: t_ptrace_fork_wait.h,v 1.7 2020/06/09 00:28:57 kamil Exp $	*/

/*-
 * Copyright (c) 2016, 2017, 2018, 2020 The NetBSD Foundation, Inc.
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
fork_body(const char *fn, bool trackspawn, bool trackfork, bool trackvfork,
    bool trackvforkdone, bool newpgrp)
{
	const int exitval = 5;
	const int exitval2 = 0; /* This matched exit status from /bin/echo */
	const int sigval = SIGSTOP;
	pid_t child, child2 = 0, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	sigset_t set;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

	if (newpgrp)
		atf_tc_skip("kernel panic (pg_jobc going negative)");

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		if (newpgrp) {
			DPRINTF("Before entering new process group");
			setpgid(0, 0);
		}

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

	/*
	 * Ignore interception of the SIGCHLD signals.
	 *
	 * SIGCHLD once blocked is discarded by the kernel as it has the
	 * SA_IGNORE property. During the fork(2) operation all signals can be
	 * shortly blocked and missed (unless there is a registered signal
	 * handler in the traced child). This leads to a race in this test if
	 * there would be an intention to catch SIGCHLD.
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	SYSCALL_REQUIRE(ptrace(PT_SET_SIGPASS, child, &set, sizeof(set)) != -1);

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

	DPRINTF("Before calling %s() for the child - expected exited\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child - expected no process\n",
	    TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define FORK_TEST2(name,fun,tspawn,tfork,tvfork,tvforkdone,newpgrp)	\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify " fun "() "		\
	    "called with 0%s%s%s%s in EVENT_MASK%s",			\
	    tspawn ? "|PTRACE_POSIX_SPAWN" : "",			\
	    tfork ? "|PTRACE_FORK" : "",				\
	    tvfork ? "|PTRACE_VFORK" : "",				\
	    tvforkdone ? "|PTRACE_VFORK_DONE" : "",			\
	    newpgrp ? " and the traced processes call setpgrp(0,0)":"");\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	fork_body(fun, tspawn, tfork, tvfork, tvforkdone, newpgrp);	\
}

#define FORK_TEST(name,fun,tspawn,tfork,tvfork,tvforkdone)		\
	FORK_TEST2(name,fun,tspawn,tfork,tvfork,tvforkdone,false)

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

#if defined(TWAIT_HAVE_PID)
FORK_TEST2(fork_setpgid, "fork", true, true, true, true, true)
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

#if defined(TWAIT_HAVE_PID)
FORK_TEST2(vfork_setpgid, "vfork", true, true, true, true, true)
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

#if defined(TWAIT_HAVE_PID)
FORK_TEST2(posix_spawn_setpgid, "spawn", true, true, true, true, true)
#endif

/// ----------------------------------------------------------------------------

#if defined(TWAIT_HAVE_PID)
static void
unrelated_tracer_fork_body(const char *fn, bool trackspawn, bool trackfork,
    bool trackvfork, bool trackvforkdone, bool newpgrp)
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

	sigset_t set;
	struct ptrace_siginfo info;
	ptrace_state_t state;
	const int slen = sizeof(state);
	ptrace_event_t event;
	const int elen = sizeof(event);

	char * const arg[] = { __UNCONST("/bin/echo"), NULL };

	if (newpgrp)
		atf_tc_skip("kernel panic (pg_jobc going negative)");

	DPRINTF("Spawn tracee\n");
	SYSCALL_REQUIRE(msg_open(&parent_tracee) == 0);
	tracee = atf_utils_fork();
	if (tracee == 0) {
		if (newpgrp) {
			DPRINTF("Before entering new process group");
			setpgid(0, 0);
		}

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

		/*
		 * Ignore interception of the SIGCHLD signals.
		 *
		 * SIGCHLD once blocked is discarded by the kernel as it has the
		 * SA_IGNORE property. During the fork(2) operation all signals
		 * can be shortly blocked and missed (unless there is a
		 * registered signal handler in the traced child). This leads to
		 * a race in this test if there would be an intention to catch
		 * SIGCHLD.
		 */
		sigemptyset(&set);
		sigaddset(&set, SIGCHLD);
		SYSCALL_REQUIRE(ptrace(PT_SET_SIGPASS, tracee, &set,
		    sizeof(set)) != -1);

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

#define UNRELATED_TRACER_FORK_TEST2(name,fun,tspawn,tfork,tvfork,tvforkdone,newpgrp)\
ATF_TC(name);								\
ATF_TC_HEAD(name, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr", "Verify " fun "() "		\
	    "called with 0%s%s%s%s in EVENT_MASK%s",			\
	    tspawn ? "|PTRACE_POSIX_SPAWN" : "",			\
	    tfork ? "|PTRACE_FORK" : "",				\
	    tvfork ? "|PTRACE_VFORK" : "",				\
	    tvforkdone ? "|PTRACE_VFORK_DONE" : "",			\
	    newpgrp ? " and the traced processes call setpgrp(0,0)":"");\
}									\
									\
ATF_TC_BODY(name, tc)							\
{									\
									\
	unrelated_tracer_fork_body(fun, tspawn, tfork, tvfork,		\
	    tvforkdone, newpgrp);					\
}

#define UNRELATED_TRACER_FORK_TEST(name,fun,tspawn,tfork,tvfork,tvforkdone)	\
	UNRELATED_TRACER_FORK_TEST2(name,fun,tspawn,tfork,tvfork,tvforkdone,false)

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

UNRELATED_TRACER_FORK_TEST2(unrelated_tracer_fork_setpgid, "fork", true, true, true, true, true)

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

UNRELATED_TRACER_FORK_TEST2(unrelated_tracer_vfork_setpgid, "vfork", true, true, true, true, true)

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

UNRELATED_TRACER_FORK_TEST2(unrelated_tracer_posix_spawn_setpgid, "spawn", true, true, true, true, true)
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
	sigset_t set;
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

	kp_sigmask = kp.p_sigmask;
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

	/*
	 * Ignore interception of the SIGCHLD signals.
	 *
	 * SIGCHLD once blocked is discarded by the kernel as it has the
	 * SA_IGNORE property. During the fork(2) operation all signals can be
	 * shortly blocked and missed (unless there is a registered signal
	 * handler in the traced child). This leads to a race in this test if
	 * there would be an intention to catch SIGCHLD.
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	SYSCALL_REQUIRE(ptrace(PT_SET_SIGPASS, child, &set, sizeof(set)) != -1);

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

			ATF_REQUIRE(sigismember((sigset_t *)&kp.p_sigmask,
			    SIGTRAP));
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

			ATF_REQUIRE(sigismember((sigset_t *)&kp.p_sigignore,
			    SIGTRAP));
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

			ATF_REQUIRE(sigismember((sigset_t *)&kp.p_sigmask,
			    SIGTRAP));
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

			ATF_REQUIRE(sigismember((sigset_t *)&kp.p_sigignore,
			    SIGTRAP));
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

			ATF_REQUIRE(sigismember((sigset_t *)&kp.p_sigmask,
			    SIGTRAP));
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

			ATF_REQUIRE(sigismember((sigset_t *)&kp.p_sigignore,
			    SIGTRAP));
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

FORK2_TEST(posix_spawn_signalmasked, "spawn", true, false)
FORK2_TEST(posix_spawn_signalignored, "spawn", false, true)
FORK2_TEST(fork_signalmasked, "fork", true, false)
FORK2_TEST(fork_signalignored, "fork", false, true)
FORK2_TEST(vfork_signalmasked, "vfork", true, false)
FORK2_TEST(vfork_signalignored, "vfork", false, true)
FORK2_TEST(vforkdone_signalmasked, "vforkdone", true, false)
FORK2_TEST(vforkdone_signalignored, "vforkdone", false, true)
#endif

#define ATF_TP_ADD_TCS_PTRACE_WAIT_FORK() \
	ATF_TP_ADD_TC(tp, fork1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork4); \
	ATF_TP_ADD_TC(tp, fork5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork8); \
	ATF_TP_ADD_TC(tp, fork9); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork10); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork11); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork12); \
	ATF_TP_ADD_TC(tp, fork13); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork14); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork15); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork16); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_setpgid); \
	ATF_TP_ADD_TC(tp, vfork1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork4); \
	ATF_TP_ADD_TC(tp, vfork5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork8); \
	ATF_TP_ADD_TC(tp, vfork9); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork10); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork11); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork12); \
	ATF_TP_ADD_TC(tp, vfork13); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork14); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork15); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork16); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_setpgid); \
	ATF_TP_ADD_TC(tp, posix_spawn1); \
	ATF_TP_ADD_TC(tp, posix_spawn2); \
	ATF_TP_ADD_TC(tp, posix_spawn3); \
	ATF_TP_ADD_TC(tp, posix_spawn4); \
	ATF_TP_ADD_TC(tp, posix_spawn5); \
	ATF_TP_ADD_TC(tp, posix_spawn6); \
	ATF_TP_ADD_TC(tp, posix_spawn7); \
	ATF_TP_ADD_TC(tp, posix_spawn8); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn9); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn10); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn11); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn12); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn13); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn14); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn15); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn16); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_setpgid); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork4); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork8); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork9); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork10); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork11); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork12); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork13); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork14); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork15); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork16); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork_setpgid); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork4); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork8); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork9); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork10); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork11); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork12); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork13); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork14); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork15); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork16); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_setpgid); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn4); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn8); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn9); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn10); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn11); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn12); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn13); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn14); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn15); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn16); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn_setpgid); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_detach_spawner); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_detach_forker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_detach_vforker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_detach_vforkerdone); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_kill_spawner); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_kill_forker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_kill_vforker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_kill_vforkerdone); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn_detach_spawner); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork_detach_forker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_detach_vforker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_detach_vforkerdone); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_posix_spawn_kill_spawner); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_fork_kill_forker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_kill_vforker); \
	ATF_TP_ADD_TC_HAVE_PID(tp, unrelated_tracer_vfork_kill_vforkerdone); \
	ATF_TP_ADD_TC(tp, traceme_vfork_fork); \
	ATF_TP_ADD_TC(tp, traceme_vfork_vfork); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, posix_spawn_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, fork_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vfork_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vforkdone_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, vforkdone_signalignored);
