/*	$NetBSD: t_ptrace_lwp_wait.h,v 1.1 2020/05/05 00:15:45 kamil Exp $	*/

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

#define ATF_TP_ADD_TCS_PTRACE_WAIT_LWP() \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo0); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo0_lwpstatus_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo1_lwpstatus_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo2_lwpstatus_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpinfo3_lwpstatus_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext0); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext1); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext2); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext3); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext0_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext1_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext2_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext3_pl_sigmask); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext0_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext1_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext2_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext3_pl_name); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext0_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext1_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext2_pl_private); \
	ATF_TP_ADD_TC(tp, traceme_lwpnext3_pl_private); \
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo0); \
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo1); \
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, attach_lwpinfo3);
