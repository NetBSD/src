/*	$NetBSD: t_ptrace_eventmask_wait.h,v 1.1 2020/05/05 00:01:14 kamil Exp $	*/

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

#define ATF_TP_ADD_TCS_PTRACE_WAIT_EVENTMASK() \
	ATF_TP_ADD_TC(tp, eventmask_preserved_empty); \
	ATF_TP_ADD_TC(tp, eventmask_preserved_fork); \
	ATF_TP_ADD_TC(tp, eventmask_preserved_vfork); \
	ATF_TP_ADD_TC(tp, eventmask_preserved_vfork_done); \
	ATF_TP_ADD_TC(tp, eventmask_preserved_lwp_create); \
	ATF_TP_ADD_TC(tp, eventmask_preserved_lwp_exit); \
	ATF_TP_ADD_TC(tp, eventmask_preserved_posix_spawn);
