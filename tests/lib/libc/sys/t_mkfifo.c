/* $NetBSD: t_mkfifo.c,v 1.8 2026/09/25 15:57:31 riastradh Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
__RCSID("$NetBSD: t_mkfifo.c,v 1.8 2026/09/25 15:57:31 riastradh Exp $");

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "h_macros.h"

static const char	path[] = "fifo";
static pid_t		child = -1;
static void		support(void);

static void
support(void)
{

	errno = 0;

	if (mkfifo(path, 0600) == 0) {
		ATF_REQUIRE(unlink(path) == 0);
		return;
	}

	if (errno == EOPNOTSUPP)
		atf_tc_skip("the kernel does not support FIFOs");
	else {
		atf_tc_fail("mkfifo(2) failed");
	}
}

ATF_TC_WITH_CLEANUP(mkfifo_block);
ATF_TC_HEAD(mkfifo_block, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test that FIFOs block");
}

ATF_TC_BODY(mkfifo_block, tc)
{
	int sta, fd = -1;
	pid_t pid;

	support();

	ATF_REQUIRE(mkfifo(path, 0600) == 0);

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {

		/*
		 * If we open the FIFO as read-only (write-only),
		 * the call should block until another process
		 * opens the FIFO for writing (reading).
		 */
		fd = open(path, O_RDONLY);

		_exit(EXIT_FAILURE); /* NOTREACHED */
	}

	(void)sleep(1);

	ATF_REQUIRE(kill(pid, SIGKILL) == 0);

	(void)wait(&sta);

	if (WIFSIGNALED(sta) == 0 || WTERMSIG(sta) != SIGKILL)
		atf_tc_fail("FIFO did not block");

	(void)close(fd);
	(void)unlink(path);
}

ATF_TC_CLEANUP(mkfifo_block, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mkfifo_err);
ATF_TC_HEAD(mkfifo_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors from mkfifo(2)");
}

ATF_TC_BODY(mkfifo_err, tc)
{
	char buf[PATH_MAX + 1];

	support();

	(void)memset(buf, 'x', sizeof(buf));
	ATF_REQUIRE(mkfifo(path, 0600) == 0);

	errno = 0;
	ATF_REQUIRE_ERRNO(EFAULT, mkfifo((char *)-1, 0600) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EEXIST, mkfifo("/etc/passwd", 0600) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EEXIST, mkfifo(path, 0600) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(ENAMETOOLONG, mkfifo(buf, 0600) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(ENOENT, mkfifo("/a/b/c/d/e/f/g", 0600) == -1);

	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(mkfifo_err, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mkfifo_nonblock);
ATF_TC_HEAD(mkfifo_nonblock, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test O_NONBLOCK with FIFOs");
}

ATF_TC_BODY(mkfifo_nonblock, tc)
{
	int fd, sta;
	pid_t pid;

	support();

	fd = -1;
	ATF_REQUIRE(mkfifo(path, 0600) == 0);

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {

		/*
		 * If we open the FIFO as O_NONBLOCK, the O_RDONLY
		 * call should return immediately, whereas the call
		 * for write-only should fail with ENXIO.
		 */
		fd = open(path, O_RDONLY | O_NONBLOCK);

		if (fd >= 0)
			_exit(EXIT_SUCCESS);

		(void)pause();	/* NOTREACHED */
	}

	(void)sleep(1);

	errno = 0;
	ATF_REQUIRE_ERRNO(ENXIO, open(path, O_WRONLY | O_NONBLOCK) == -1);

	(void)kill(pid, SIGKILL);
	(void)wait(&sta);

	if (WIFSIGNALED(sta) != 0 || WTERMSIG(sta) == SIGKILL)
		atf_tc_fail("FIFO blocked for O_NONBLOCK open(2)");

	(void)close(fd);
	(void)unlink(path);
}

ATF_TC_CLEANUP(mkfifo_nonblock, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mkfifo_perm);
ATF_TC_HEAD(mkfifo_perm, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test permissions with mkfifo(2)");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(mkfifo_perm, tc)
{

	support();

	errno = 0;
	ATF_REQUIRE_ERRNO(EACCES, mkfifo("/root/fifo", 0600) == -1);

	ATF_REQUIRE(mkfifo(path, 0600) == 0);

	/*
	 * For some reason this fails with EFTYPE...
	 */
	errno = 0;
	ATF_REQUIRE_ERRNO(EFTYPE, chmod(path, 1777) == -1);

	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(mkfifo_perm, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mkfifo_stat);
ATF_TC_HEAD(mkfifo_stat, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mkfifo(2) with stat");
}

ATF_TC_BODY(mkfifo_stat, tc)
{
	struct stat st;

	support();

	(void)memset(&st, 0, sizeof(struct stat));

	ATF_REQUIRE(mkfifo(path, 0600) == 0);
	ATF_REQUIRE(stat(path, &st) == 0);

	if (S_ISFIFO(st.st_mode) == 0)
		atf_tc_fail("invalid mode from mkfifo(2)");

	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(mkfifo_stat, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mknod_s_ififo);
ATF_TC_HEAD(mknod_s_ififo, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mknod(2) with S_IFIFO");
}

ATF_TC_BODY(mknod_s_ififo, tc)
{
	struct stat st;

	support();

	(void)memset(&st, 0, sizeof(struct stat));

	ATF_REQUIRE(mknod(path, S_IFIFO | 0600, 0) == 0);
	ATF_REQUIRE(stat(path, &st) == 0);

	if (S_ISFIFO(st.st_mode) == 0)
		atf_tc_fail("invalid mode from mknod(2) with S_IFIFO");

	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(mknod_s_ififo, tc)
{
	(void)unlink(path);
}

static int
tryread(int fd, void *buf, size_t len)
{
	ssize_t nread;

	while ((nread = read(fd, buf, len)) == -1) {
		if (errno == EAGAIN)
			return -1;
		if (errno != EINTR)
			atf_tc_fail_errno("read");
	}
	if (nread == 0)
		atf_tc_fail("unexpected eof");
	return 0;
}

static void
cleanup_sigopen_race(void)
{
	if (child != -1) {
		(void)kill(child, SIGKILL);
		(void)waitpid(child, NULL, 0);
	}
	(void)unlink(path);
}

static void
on_sigalrm(int signo)
{
}

static void
test_sigopen_race(int parent_flags, int child_flags)
{
	struct sigaction sa;
	const struct timespec timeout = {5,0};
	struct timespec starttime, deadline, now;
	uint64_t nintr = 0;	/* 64-bit counter can't overflow */
	uint64_t niter = 0;

	support();

	/*
	 * Create a fifo to work with.
	 */
	RL(mkfifo(path, 0600));

	/*
	 * Set up a signal handler for SIGALRM to interrupt system
	 * calls with EINTR.
	 */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &on_sigalrm;
	sa.sa_flags = 0;	/* no SA_RESTART -- want EINTR in open() */
	RL(sigemptyset(&sa.sa_mask));
	RL(sigaction(SIGALRM, &sa, NULL));

	/*
	 * Compute a deadline and keep rerunning the test until we've
	 * passed the deadline.
	 */
	RL(clock_gettime(CLOCK_MONOTONIC, &starttime));
	timespecadd(&starttime, &timeout, &deadline);
	for (;; niter++) {
		const struct itimerval timer = {
			.it_interval = {0,1},
			.it_value = {0,1},
		};
		const struct itimerval notimer = {
			.it_interval = {0,0},
			.it_value = {0,0},
		};
		int readyfd[2], donefd[2];
		int flags, fd, status;
		char ch = 0;

		/*
		 * Create a pair of pipes:
		 *
		 * - Parent writes to readyfd when it's ready to start
		 *   the race; child reads to start the race.
		 *
		 * - Child writes to donefd when it has completed an
		 *   open/close cycle; parent stops trying to race once
		 *   it can read from donefd.
		 */
		RL(pipe(readyfd));
		RL(pipe(donefd));

		/*
		 * Assume we didn't get fds 0/1/2, to keep it simple in
		 * the child.
		 */
		ATF_REQUIRE(readyfd[1] > STDERR_FILENO);
		ATF_REQUIRE(donefd[0] > STDERR_FILENO);

		/*
		 * Fork a child to open and close the fifo.  This
		 * should match up with a single successful open of the
		 * parent, but with the bug of PR kern/59578, the
		 * child's open may succeed and close in quick
		 * succession while the parent's open fails with EINTR
		 * and never returns with success matching the child's
		 * successful open.
		 */
		RL(child = fork());
		if (child == 0) {
			/*
			 * Move the pipe endpoints we will be using to
			 * stdin/stdout, and close everything else.
			 */
			if (dup2(readyfd[0], STDIN_FILENO) == -1) {
				warn("dup2 to stdin");
				_exit(1);
			}
			if (dup2(donefd[1], STDOUT_FILENO) == -1) {
				warn("dup2 to stdin");
				_exit(2);
			}
			if (closefrom(STDERR_FILENO + 1) == -1) {
				warn("closefrom");
				_exit(3);
			}

			/*
			 * Wait until the parent has said it's ready.
			 */
			if (read(STDIN_FILENO, &ch, 1) == -1) {
				warn("read");
				_exit(4);
			}

			/*
			 * Wait a smidge more for the parent to start
			 * sleeping in open.  XXX Should really
			 * busy-wait on the parent process's status,
			 * and _then_ sleep a tick longer so the parent
			 * gets an EINTR at least once to confirm.
			 */
			sleep(1);

			/*
			 * Make sure we give up within 1sec.
			 */
			if (alarm(1) == (unsigned)-1) {
				warn("alarm");
				_exit(6);
			}

			/*
			 * Open and close the fifo in quick succession.
			 * This should make open succeed in the parent,
			 * but with the bug of PR kern/59578, if we do
			 * this fast enough while the parent is
			 * handling a signal, the parent might fail
			 * with EINTR and restart even though the child
			 * succeeded without a matching open on the
			 * other side of the fifo.
			 */
			if ((fd = open(path, child_flags)) == -1) {
				warn("open");
				_exit(7);
			}
			if (close(fd) == -1) {
				warn("close");
				_exit(8);
			}

			/*
			 * Notify the parent that we're done, so if it
			 * lost the race, it will promptly notice the
			 * fact and fail.
			 */
			if (write(STDOUT_FILENO, &ch, 1) == -1) {
				warn("write");
				_exit(9);
			}
			_exit(0);
		}
		RL(close(readyfd[0]));
		RL(close(donefd[1]));

		/*
		 * Make donefd nonblocking so the parent can use it to
		 * test, without blocking, whether the child is done.
		 */
		RL(flags = fcntl(donefd[0], F_GETFL));
		RL(fcntl(donefd[0], F_SETFL, flags | O_NONBLOCK));

		/*
		 * Arrange to deliver SIGALRM as fast as we can while
		 * we race with the child, and then notify the child
		 * that it's ready to go.  We must be prepared to
		 * handle EINTR for all blocking system calls while the
		 * timer is set up.
		 *
		 * If the open was interrupted in the parent but the
		 * child has already finished an open/close cycle,
		 * fail.
		 */
		RL(setitimer(ITIMER_REAL, &timer, NULL));
		while (write(readyfd[1], &ch, 1) == -1) {
			if (errno != EINTR)
				atf_tc_fail_errno("write");
		}
		for (;;) {
			if ((fd = open(path, parent_flags)) != -1)
				break;
			if (errno != EINTR)
				atf_tc_fail_errno("open");
			nintr++;
			if (tryread(donefd[0], &ch, 1) == 0)
				atf_tc_fail("child opened without parent");
		}
		RL(setitimer(ITIMER_REAL, &notimer, NULL));
		RL(close(fd));

		/*
		 * Parent and child successfully handed off a matching
		 * pair of fifo opens.  Wait for the the child and make
		 * sure it didn't crash.
		 */
		RL(waitpid(child, &status, 0));
		child = -1;
		ATF_REQUIRE_MSG(!WIFSIGNALED(status),
		    "child terminated on signal %d (%s)",
		    WTERMSIG(status), strsignal(WTERMSIG(status)));
		ATF_REQUIRE_MSG(WIFEXITED(status),
		    "child exited mysteriously, status=0x%x", status);
		ATF_REQUIRE_MSG(WEXITSTATUS(status) == 0,
		    "child exited with code %d", WEXITSTATUS(status));

		/*
		 * Close the pipes; we'll open fresh ones on the next
		 * iteration.  (Could reuse them but in case they had
		 * any state left over, let's just keep it simpler by
		 * opening fresh ones.)
		 */
		RL(close(readyfd[1]));
		RL(close(donefd[0]));

		/*
		 * If we've run long enough that we think the bug isn't
		 * there, stop.
		 */
		RL(clock_gettime(CLOCK_MONOTONIC, &now));
		if (timespeccmp(&deadline, &now, <=))
			break;
	}

	/*
	 * We are supposed to test interrupting open(2) in the parent.
	 * If it never got interrupted, the test is broken.
	 */
	fprintf(stderr, "interrupted %"PRIu64" times"
	    " in %"PRIu64" iterations\n", nintr, niter);
	ATF_REQUIRE(nintr > 0);
}

ATF_TC_WITH_CLEANUP(mkfifo_sigopenreader_race);
ATF_TC_HEAD(mkfifo_sigopenreader_race, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test interrupting open(fifo, O_RDONLY) by a signal");
}
ATF_TC_BODY(mkfifo_sigopenreader_race, tc)
{
	test_sigopen_race(O_RDONLY, O_WRONLY);
}
ATF_TC_CLEANUP(mkfifo_sigopenreader_race, tc)
{
	cleanup_sigopen_race();
}

ATF_TC_WITH_CLEANUP(mkfifo_sigopenwriter_race);
ATF_TC_HEAD(mkfifo_sigopenwriter_race, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test interrupting open(fifo, O_WRONLY) by a signal");
}
ATF_TC_BODY(mkfifo_sigopenwriter_race, tc)
{
	test_sigopen_race(O_WRONLY, O_RDONLY);
}
ATF_TC_CLEANUP(mkfifo_sigopenwriter_race, tc)
{
	cleanup_sigopen_race();
}

ATF_TC_WITH_CLEANUP(mkfifo_readeof_block);
ATF_TC_HEAD(mkfifo_readeof_block, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test read with no writers returns EOF instead of blocking");
}
ATF_TC_BODY(mkfifo_readeof_block, tc)
{
	int rd, wr, flags;
	char ch;
	ssize_t nread, nwrit;

	support();

	RL(mkfifo(path, 0600));

	atf_tc_expect_fail("PR kern/60789:"
	    " read on fifo without writer may block");

	/*
	 * Open nonblocking so we open immediately, but then switch to
	 * blocking to test the blocking path.
	 */
	fprintf(stderr, "1. open reader\n");
	RL(rd = open(path, O_RDONLY|O_NONBLOCK));
	RL(flags = fcntl(rd, F_GETFL));
	RL(fcntl(rd, F_SETFL, flags & ~O_NONBLOCK));

	/*
	 * No data and writers, so read should return EOF.
	 */
	fprintf(stderr, "2. read expect eof\n");
	alarm(1);
	RL(nread = read(rd, &ch, 1));
	alarm(0);
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	fprintf(stderr, "3. read expect eof\n");
	alarm(1);
	RL(nread = read(rd, &ch, 1));
	alarm(0);
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	/*
	 * Reader is open, so writer should be able to write a byte.
	 */
	fprintf(stderr, "4. open writer\n");
	alarm(1);
	RL(wr = open(path, O_WRONLY));
	alarm(0);

	fprintf(stderr, "5. write byte\n");
	ch = 123;
	alarm(1);
	RL(nwrit = write(wr, &ch, 1));
	alarm(0);
	ATF_CHECK_EQ_MSG(nwrit, 1, "nwrit=%zd", nwrit);

	fprintf(stderr, "6. close writer\n");
	alarm(1);
	RL(close(wr));
	alarm(0);

	/*
	 * No writers but data available, so read should return data.
	 */
	fprintf(stderr, "7. read expect byte\n");
	alarm(1);
	RL(nread = read(rd, &ch, 1));
	alarm(0);
	ATF_CHECK_EQ_MSG(ch, 123, "ch=%d", ch);

	/*
	 * No data and writers, so read should return EOF again.
	 */
	fprintf(stderr, "8. read expect eof\n");
	alarm(1);
	RL(nread = read(rd, &ch, 1));
	alarm(0);
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	fprintf(stderr, "9. read expect eof\n");
	alarm(1);
	RL(nread = read(rd, &ch, 1));
	alarm(0);
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	fprintf(stderr, "10. tada\n");
}
ATF_TC_CLEANUP(mkfifo_readeof_block, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(mkfifo_readeof_nonblock);
ATF_TC_HEAD(mkfifo_readeof_nonblock, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test read with no writers returns EOF instead of EAGAIN");
}
ATF_TC_BODY(mkfifo_readeof_nonblock, tc)
{
	int rd, wr;
	char ch;
	ssize_t nread, nwrit;

	support();

	RL(mkfifo(path, 0600));

	/*
	 * Open reader nonblocking.
	 */
	fprintf(stderr, "1. open reader\n");
	RL(rd = open(path, O_RDONLY|O_NONBLOCK));

	/*
	 * No data and writers, so read should return EOF.
	 */
	fprintf(stderr, "2. read expect eof\n");
	RL(nread = read(rd, &ch, 1));
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	fprintf(stderr, "3. read expect eof\n");
	RL(nread = read(rd, &ch, 1));
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	/*
	 * Reader is open, so writer should be able to write a byte.
	 */
	fprintf(stderr, "4. open writer\n");
	RL(wr = open(path, O_WRONLY|O_NONBLOCK));

	fprintf(stderr, "5. write byte\n");
	ch = 123;
	RL(nwrit = write(wr, &ch, 1));
	ATF_CHECK_EQ_MSG(nwrit, 1, "nwrit=%zd", nwrit);

	fprintf(stderr, "6. close writer\n");
	RL(close(wr));

	/*
	 * No writers but data available, so read should return data.
	 */
	fprintf(stderr, "7. read expect byte\n");
	RL(nread = read(rd, &ch, 1));
	ATF_CHECK_EQ_MSG(ch, 123, "ch=%d", ch);

	/*
	 * No data and writers, so read should return EOF again.
	 */
	fprintf(stderr, "8. read expect eof\n");
	RL(nread = read(rd, &ch, 1));

	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	fprintf(stderr, "9. read expect eof\n");
	RL(nread = read(rd, &ch, 1));
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zd", nread);

	fprintf(stderr, "10. tada\n");
}
ATF_TC_CLEANUP(mkfifo_readeof_nonblock, tc)
{
	(void)unlink(path);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mkfifo_block);
	ATF_TP_ADD_TC(tp, mkfifo_err);
	ATF_TP_ADD_TC(tp, mkfifo_nonblock);
	ATF_TP_ADD_TC(tp, mkfifo_perm);
	ATF_TP_ADD_TC(tp, mkfifo_readeof_block);
	ATF_TP_ADD_TC(tp, mkfifo_readeof_nonblock);
	ATF_TP_ADD_TC(tp, mkfifo_sigopenreader_race);
	ATF_TP_ADD_TC(tp, mkfifo_sigopenwriter_race);
	ATF_TP_ADD_TC(tp, mkfifo_stat);
	ATF_TP_ADD_TC(tp, mknod_s_ififo);

	return atf_no_error();
}
