/*	$NetBSD: t_cloexec.c,v 1.1.2.2 2024/11/17 16:16:10 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_cloexec.c,v 1.1.2.2 2024/11/17 16:16:10 martin Exp $");

#include <sys/types.h>

#include <sys/bitops.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <unistd.h>

#include "h_macros.h"

/*
 * Test close-on-exec as set in various ways
 */

static int
open_via_accept4(void)
{
	static const union {
		struct sockaddr sa;
		struct sockaddr_un sun;
	} name = { .sun = {
		.sun_family = AF_LOCAL,
		.sun_path = "socket",
	} };
	int slisten, saccept, c;

	/*
	 * Create a listening server socket and bind it to the path.
	 */
	RL(slisten = socket(PF_LOCAL, SOCK_STREAM, 0));
	RL(bind(slisten, &name.sa, sizeof(name)));
	RL(listen(slisten, SOMAXCONN));

	/*
	 * Create an active client socket and connect it to the path --
	 * nonblocking, so we don't deadlock here.  If connect doesn't
	 * succeed immediately, it had better fail immediately with
	 * EINPROGRESS.
	 */
	RL(c = socket(PF_LOCAL, SOCK_STREAM|SOCK_NONBLOCK, 0));
	if (connect(c, &name.sa, sizeof(name)) == -1) {
		ATF_CHECK_EQ_MSG(errno, EINPROGRESS, "connect failed %d: %s",
		    errno, strerror(errno));
	}

	/*
	 * Accept a socket on the server side with SOCK_CLOEXEC.
	 */
	RL(saccept = accept4(slisten, /*addr*/NULL, /*addrlen*/NULL,
		SOCK_CLOEXEC));
	return saccept;
}

static int
open_via_clonedev(void)
{
	int fd;

	RL(fd = open("/dev/drvctl", O_RDONLY|O_CLOEXEC));

	return fd;
}

static int
open_via_dup3(void)
{
	int fd3;

	RL(fd3 = dup3(STDIN_FILENO, 3, O_CLOEXEC));
	ATF_REQUIRE_EQ_MSG(fd3, 3, "dup3(STDIN_FILENO, 3, ...)"
	    " failed to return 3: %d", fd3);

	return fd3;
}

static int
open_via_fcntldupfd(void)
{
	int fd;

	RL(fd = fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC, 0));

	return fd;
}

static int
open_via_kqueue(void)
{
	int fd;

	RL(fd = kqueue1(O_CLOEXEC));

	return fd;
}

static int
open_via_opencloexec(void)
{
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT|O_CLOEXEC, 0644));

	return fd;
}

static int
open_via_openfcntlcloexec(void)
{
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT, 0644));
	RL(fcntl(fd, F_SETFD, FD_CLOEXEC));

	return fd;
}

static int
open_via_openioctlfioclex(void)
{
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT, 0644));
	RL(ioctl(fd, FIOCLEX));

	return fd;
}

static int
open_via_pipe2rd(void)
{
	int fd[2];

	RL(pipe2(fd, O_CLOEXEC));

	return fd[0];
}

static int
open_via_pipe2wr(void)
{
	int fd[2];

	RL(pipe2(fd, O_CLOEXEC));

	return fd[1];
}

static int
open_via_paccept(void)
{
	static const union {
		struct sockaddr sa;
		struct sockaddr_un sun;
	} name = { .sun = {
		.sun_family = AF_LOCAL,
		.sun_path = "socket",
	} };
	int slisten, saccept, c;

	/*
	 * Create a listening server socket and bind it to the path.
	 */
	RL(slisten = socket(PF_LOCAL, SOCK_STREAM, 0));
	RL(bind(slisten, &name.sa, sizeof(name)));
	RL(listen(slisten, SOMAXCONN));

	/*
	 * Create an active client socket and connect it to the path --
	 * nonblocking, so we don't deadlock here.  If connect doesn't
	 * succeed immediately, it had better fail immediately with
	 * EINPROGRESS.
	 */
	RL(c = socket(PF_LOCAL, SOCK_STREAM|SOCK_NONBLOCK, 0));
	if (connect(c, &name.sa, sizeof(name)) == -1) {
		ATF_CHECK_EQ_MSG(errno, EINPROGRESS, "connect failed %d: %s",
		    errno, strerror(errno));
	}

	/*
	 * Accept a socket on the server side with SOCK_CLOEXEC.
	 */
	RL(saccept = paccept(slisten, /*addr*/NULL, /*addrlen*/NULL,
		/*sigmask*/NULL, SOCK_CLOEXEC));
	return saccept;
}

static int
open_via_socket(void)
{
	int fd;

	RL(fd = socket(PF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0));

	return fd;
}

static int
open_via_socketpair0(void)
{
	int fd[2];

	RL(socketpair(PF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0, fd));

	return fd[0];
}

static int
open_via_socketpair1(void)
{
	int fd[2];

	RL(socketpair(PF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0, fd));

	return fd[1];
}

/*
 * XXX Close-on-exec paths still missing:
 * XXX
 * XXX compat_linux inotify
 * XXX compat_linux close_range
 * XXX drm i915_perf_open_ioctl
 * XXX drm dma_buf
 * XXX eventfd(2)
 * XXX memfd(2)
 * XXX timerfd(2)
 * XXX recvmsg/recvmmsg with MSG_CMSG_CLOEXEC
 */

static void
check_cloexec(const struct atf_tc *tc, int fd,
    pid_t (*execfn)(char *, char *const[]))
{
	char h_cloexec[PATH_MAX];
	char fdstr[(ilog2(INT_MAX) + 1)/(ilog2(10) - 1) + 1];
	char *const argv[] = {__UNCONST("h_cloexec"), fdstr, NULL};
	pid_t child, waitedpid;
	int status;

	/*
	 * Format the h_cloexec helper executable path, which lives in
	 * the test's directory (typically /usr/tests/kernel), and the
	 * argument of a file descriptor in decimal.
	 */
	snprintf(h_cloexec, sizeof(h_cloexec), "%s/h_cloexec",
	    atf_tc_get_config_var(tc, "srcdir"));
	snprintf(fdstr, sizeof(fdstr), "%d", fd);

	/*
	 * Execute h_cloexec as a subprocess.
	 */
	child = (*execfn)(h_cloexec, argv);

	/*
	 * Wait for the child to complete.
	 */
	RL(waitedpid = waitpid(child, &status, 0));
	ATF_CHECK_EQ_MSG(child, waitedpid, "waited for %jd, got %jd",
	    (intmax_t)child, (intmax_t)waitedpid);

	/*
	 * Verify the child exited normally.
	 */
	if (WIFSIGNALED(status)) {
		atf_tc_fail("subprocess terminated on signal %d",
		    WTERMSIG(status));
		return;
	} else if (!WIFEXITED(status)) {
		atf_tc_fail("subprocess failed to exit normally: status=0x%x",
		    status);
		return;
	}

	/*
	 * h_cloexec is supposed to exit status 0 if an operation on
	 * the fd failed with EBADFD, 1 if it unexpectedly succeeded,
	 * 127 if exec returned, or something else if anything else
	 * happened.
	 */
	switch (WEXITSTATUS(status)) {
	case 0:			/* success -- closed on exec */
		return;
	case 1:			/* fail -- not closed on exec */
		atf_tc_fail("fd was not closed on exec");
		return;
	case 127:		/* exec failed */
		atf_tc_fail("failed to exec h_cloexec");
		return;
	default:		/* something else went wong */
		atf_tc_fail("h_cloexec failed unexpectedly: %d",
		    WEXITSTATUS(status));
		return;
	}
}

static pid_t
exec_via_forkexecve(char *prog, char *const argv[])
{
	pid_t pid;

	RL(pid = fork());
	if (pid == 0) {		/* child */
		if (execve(prog, argv, /*envp*/NULL) == -1)
			_exit(127);
		abort();
	}

	/* parent */
	return pid;
}

static pid_t
exec_via_vforkexecve(char *prog, char *const argv[])
{
	pid_t pid;

	RL(pid = vfork());
	if (pid == 0) {		/* child */
		if (execve(prog, argv, /*envp*/NULL) == -1)
			_exit(127);
		abort();
	}

	/* parent */
	return pid;
}

static pid_t
exec_via_posixspawn(char *prog, char *const argv[])
{
	pid_t pid;

	RZ(posix_spawn(&pid, prog, /*file_actions*/NULL, /*attrp*/NULL, argv,
		/*envp*/NULL));

	return pid;
}

/*
 * Full cartesian product is not really important here -- the paths for
 * open and the paths for exec are independent.  So we try
 * pipe2(O_CLOEXEC) with each exec path, and we try each open path with
 * posix_spawn.
 */

#define	CLOEXEC_TEST(test, openvia, execvia, descr)			      \
ATF_TC(test);								      \
ATF_TC_HEAD(test, tc)							      \
{									      \
	atf_tc_set_md_var(tc, "descr", descr);				      \
}									      \
ATF_TC_BODY(test, tc)							      \
{									      \
	check_cloexec(tc, openvia(), &execvia);				      \
}

CLOEXEC_TEST(pipe2rd_forkexecve, open_via_pipe2rd, exec_via_forkexecve,
    "pipe2(O_CLOEXEC) reader is closed in child on fork/exec")
CLOEXEC_TEST(pipe2rd_vforkexecve, open_via_pipe2rd, exec_via_vforkexecve,
    "pipe2(O_CLOEXEC) reader is closed in child on vfork/exec")
CLOEXEC_TEST(pipe2rd_posixspawn, open_via_pipe2rd, exec_via_posixspawn,
    "pipe2(O_CLOEXEC) reader is closed in child on posix_spawn")

CLOEXEC_TEST(accept4_posixspawn, open_via_accept4, exec_via_posixspawn,
    "accept4(SOCK_CLOEXEC) is closed in child on posix_spawn");
CLOEXEC_TEST(clonedev_posixspawn, open_via_clonedev, exec_via_posixspawn,
    "open(\"/dev/drvctl\") is closed in child on posix_spawn");
CLOEXEC_TEST(dup3_posixspawn, open_via_dup3, exec_via_posixspawn,
    "dup3(..., O_CLOEXEC) is closed in child on posix_spawn");
CLOEXEC_TEST(fcntldupfd_posixspawn, open_via_fcntldupfd, exec_via_posixspawn,
    "fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC) is closed in child on posix_spawn");
CLOEXEC_TEST(kqueue_posixspawn, open_via_kqueue, exec_via_posixspawn,
    "kqueue1(O_CLOEXEC) is closed in child on posix_spawn");
CLOEXEC_TEST(opencloexec_posixspawn, open_via_opencloexec, exec_via_posixspawn,
    "open(O_CLOEXEC) is closed in child on posix_spawn");
CLOEXEC_TEST(openfcntlcloexec_posixspawn, open_via_openfcntlcloexec,
    exec_via_posixspawn,
    "fcntl(open(...), F_SETFD, O_CLOEXEC) is closed in child on posix_spawn");
CLOEXEC_TEST(openioctlfioclex_posixspawn, open_via_openioctlfioclex,
    exec_via_posixspawn,
    "ioctl(open(...), FIOCLEX) is closed in child on posix_spawn");
#if 0				/* already done above */
CLOEXEC_TEST(pipe2rd_posixspawn, open_via_pipe2rd, exec_via_posixspawn,
    "pipe2(O_CLOEXEC) reader is closed in child on posix_spawn")
#endif
CLOEXEC_TEST(pipe2wr_posixspawn, open_via_pipe2wr, exec_via_posixspawn,
    "pipe2(O_CLOEXEC) writer is closed in child on posix_spawn")
CLOEXEC_TEST(paccept_posixspawn, open_via_paccept, exec_via_posixspawn,
    "paccept(..., SOCK_CLOEXEC) is closed in child on posix_spawn")
CLOEXEC_TEST(socket_posixspawn, open_via_socket, exec_via_posixspawn,
    "socket(SOCK_CLOEXEC) is closed in child on posix_spawn")
CLOEXEC_TEST(socketpair0_posixspawn, open_via_socketpair0, exec_via_posixspawn,
    "socketpair(SOCK_CLOEXEC) side 0 is closed in child on posix_spawn")
CLOEXEC_TEST(socketpair1_posixspawn, open_via_socketpair1, exec_via_posixspawn,
    "socketpair(SOCK_CLOEXEC) side 1 is closed in child on posix_spawn")

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, accept4_posixspawn);
	ATF_TP_ADD_TC(tp, clonedev_posixspawn);
	ATF_TP_ADD_TC(tp, dup3_posixspawn);
	ATF_TP_ADD_TC(tp, fcntldupfd_posixspawn);
	ATF_TP_ADD_TC(tp, kqueue_posixspawn);
	ATF_TP_ADD_TC(tp, opencloexec_posixspawn);
	ATF_TP_ADD_TC(tp, openfcntlcloexec_posixspawn);
	ATF_TP_ADD_TC(tp, openioctlfioclex_posixspawn);
	ATF_TP_ADD_TC(tp, paccept_posixspawn);
	ATF_TP_ADD_TC(tp, pipe2rd_forkexecve);
	ATF_TP_ADD_TC(tp, pipe2rd_posixspawn);
	ATF_TP_ADD_TC(tp, pipe2rd_vforkexecve);
	ATF_TP_ADD_TC(tp, pipe2wr_posixspawn);
	ATF_TP_ADD_TC(tp, socket_posixspawn);
	ATF_TP_ADD_TC(tp, socketpair0_posixspawn);
	ATF_TP_ADD_TC(tp, socketpair1_posixspawn);

	return atf_no_error();
}
