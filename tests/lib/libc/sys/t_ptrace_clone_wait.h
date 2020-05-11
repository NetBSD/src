/*	$NetBSD: t_ptrace_clone_wait.h,v 1.3 2020/05/11 21:18:11 kamil Exp $	*/

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

	kp_sigmask = kp.p_sigmask;
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

#define ATF_TP_ADD_TCS_PTRACE_WAIT_CLONE() \
	ATF_TP_ADD_TC(tp, clone1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone4); \
	ATF_TP_ADD_TC(tp, clone5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone8); \
	ATF_TP_ADD_TC(tp, clone_vm1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm4); \
	ATF_TP_ADD_TC(tp, clone_vm5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm8); \
	ATF_TP_ADD_TC(tp, clone_fs1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs4); \
	ATF_TP_ADD_TC(tp, clone_fs5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs8); \
	ATF_TP_ADD_TC(tp, clone_files1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files4); \
	ATF_TP_ADD_TC(tp, clone_files5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files8); \
	ATF_TP_ADD_TC(tp, clone_vfork1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork3); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork4); \
	ATF_TP_ADD_TC(tp, clone_vfork5); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork6); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork7); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork8); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vm_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_fs_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_files_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork_signalignored); \
	ATF_TP_ADD_TC_HAVE_PID(tp, clone_vfork_signalmasked); \
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone); \
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_vm); \
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_fs); \
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_files); \
	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_vfork);

//	ATF_TP_ADD_TC(tp, clone_sighand1); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand2); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand3); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand4); // XXX
//	ATF_TP_ADD_TC(tp, clone_sighand5); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand6); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand7); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand8); // XXX

//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand_signalignored); // XXX
//	ATF_TP_ADD_TC_HAVE_PID(tp, clone_sighand_signalmasked); // XXX

//	ATF_TP_ADD_TC_HAVE_PID(tp, traceme_vfork_clone_sighand); // XXX
