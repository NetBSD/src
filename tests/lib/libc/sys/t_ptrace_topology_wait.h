/*	$NetBSD: t_ptrace_topology_wait.h,v 1.1 2020/05/05 00:33:37 kamil Exp $	*/

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

#define ATF_TP_ADD_TCS_PTRACE_WAIT_TOPOLOGY() \
	ATF_TP_ADD_TC(tp, traceme_pid1_parent); \
	ATF_TP_ADD_TC_HAVE_PID(tp, tracer_sees_terminaton_before_the_parent); \
	ATF_TP_ADD_TC_HAVE_PID(tp, tracer_sysctl_lookup_without_duplicates); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
		unrelated_tracer_sees_terminaton_before_the_parent); \
	ATF_TP_ADD_TC_HAVE_PID(tp, tracer_attach_to_unrelated_stopped_process); \
	ATF_TP_ADD_TC(tp, parent_attach_to_its_child); \
	ATF_TP_ADD_TC(tp, parent_attach_to_its_stopped_child); \
	ATF_TP_ADD_TC(tp, child_attach_to_its_parent); \
	ATF_TP_ADD_TC(tp, child_attach_to_its_stopped_parent); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
		tracee_sees_its_original_parent_getppid); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
		tracee_sees_its_original_parent_sysctl_kinfo_proc2); \
	ATF_TP_ADD_TC_HAVE_PID(tp, \
		tracee_sees_its_original_parent_procfs_status);
