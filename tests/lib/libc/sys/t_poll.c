/*	$NetBSD: t_poll.c,v 1.12 2025/02/10 02:41:34 riastradh Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
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

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "h_macros.h"

static int desc;

static void
child1(void)
{
	struct pollfd pfd;

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void)poll(&pfd, 1, 2000);
	(void)printf("child1 exit\n");
}

static void
child2(void)
{
	struct pollfd pfd;

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void)sleep(1);
	(void)poll(&pfd, 1, INFTIM);
	(void)printf("child2 exit\n");
}

static void
child3(void)
{
	struct pollfd pfd;

	(void)sleep(5);

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void)poll(&pfd, 1, INFTIM);
	(void)printf("child3 exit\n");
}

ATF_TC(3way);
ATF_TC_HEAD(3way, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr",
	    "Check for 3-way collision for descriptor. First child comes "
	    "and polls on descriptor, second child comes and polls, first "
	    "child times out and exits, third child comes and polls. When "
	    "the wakeup event happens, the two remaining children should "
	    "both be awaken. (kern/17517)");
}

ATF_TC_BODY(3way, tc)
{
	int pf[2];
	int status, i;
	pid_t pid;
	ssize_t nwrit;

	RL(pipe(pf));
	desc = pf[0];

	RL(pid = fork());
	if (pid == 0) {
		if (close(pf[1]) == -1)
			_exit(1);
		child1();
		_exit(0);
		/* NOTREACHED */
	}

	RL(pid = fork());
	if (pid == 0) {
		if (close(pf[1]) == -1)
			_exit(1);
		child2();
		_exit(0);
		/* NOTREACHED */
	}

	RL(pid = fork());
	if (pid == 0) {
		if (close(pf[1]) == -1)
			_exit(1);
		child3();
		_exit(0);
		/* NOTREACHED */
	}

	(void)sleep(10);

	(void)printf("parent write\n");

	RL(nwrit = write(pf[1], "konec\n", 6));
	ATF_REQUIRE_EQ_MSG(nwrit, 6, "nwrit=%zd", nwrit);

	for (i = 0; i < 3; i++)
		RL(wait(&status));

	(void)printf("parent terminated\n");
}

ATF_TC(basic);
ATF_TC_HEAD(basic, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr",
	    "Basis functionality test for poll(2)");
}

ATF_TC_BODY(basic, tc)
{
	int fds[2];
	struct pollfd pfds[2];
	int ret;
	ssize_t nwrit;

	RL(pipe(fds));

	pfds[0].fd = fds[0];
	pfds[0].events = POLLIN;
	pfds[1].fd = fds[1];
	pfds[1].events = POLLOUT;

	/*
	 * Check that we get a timeout waiting for data on the read end
	 * of our pipe.
	 */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	RL(ret = poll(&pfds[0], 1, 1));
	ATF_REQUIRE_EQ_MSG(ret, 0, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, 0, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, -1, "got: %d", pfds[1].revents);

	/* Check that the write end of the pipe as reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	RL(ret = poll(&pfds[1], 1, 1));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, -1, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",\
	    pfds[1].revents);

	/* Check that only the write end of the pipe as reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	RL(ret = poll(pfds, 2, 1));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, 0, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",
	    pfds[1].revents);

	/* Write data to our pipe. */
	RL(nwrit = write(fds[1], "", 1));
	ATF_REQUIRE_EQ_MSG(nwrit, 1, "nwrit=%zd", nwrit);

	/* Check that both ends of our pipe are reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	RL(ret = poll(pfds, 2, 1));
	ATF_REQUIRE_EQ_MSG(ret, 2, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, POLLIN, "got: %d",
	    pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",
	    pfds[1].revents);

	RL(close(fds[0]));
	RL(close(fds[1]));
}

ATF_TC(err);
ATF_TC_HEAD(err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check errors from poll(2)");
}

ATF_TC_BODY(err, tc)
{
	struct pollfd pfd;
	int fd = 0;

	pfd.fd = fd;
	pfd.events = POLLIN;

	errno = 0;
	ATF_REQUIRE_ERRNO(EFAULT, poll((struct pollfd *)-1, 1, -1) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, poll(&pfd, 1, -2) == -1);
}

static const char	fifo_path[] = "pollhup_fifo";

static void
fifo_support(void)
{

	errno = 0;
	if (mkfifo(fifo_path, 0600) == 0) {
		RL(unlink(fifo_path));
		return;
	}

	if (errno == EOPNOTSUPP) {
		atf_tc_skip("the kernel does not support FIFOs");
	} else {
		atf_tc_fail("mkfifo(2) failed");
	}
}

ATF_TC_WITH_CLEANUP(fifo_inout);
ATF_TC_HEAD(fifo_inout, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Check POLLIN/POLLOUT behavior with fifos");
}

ATF_TC_BODY(fifo_inout, tc)
{
	struct pollfd pfd[2];
	char *buf;
	int rfd, wfd;
	long pipe_buf;
	int ret;
	ssize_t nwrit, nread;

	fifo_support();

	RL(mkfifo(fifo_path, 0600));
	RL(rfd = open(fifo_path, O_RDONLY | O_NONBLOCK));
	RL(wfd = open(fifo_path, O_WRONLY | O_NONBLOCK));

	/* Get the maximum atomic pipe write size. */
	pipe_buf = fpathconf(wfd, _PC_PIPE_BUF);
	ATF_REQUIRE_MSG(pipe_buf > 1, "pipe_buf=%ld", pipe_buf);

	REQUIRE_LIBC(buf = malloc(pipe_buf), NULL);

	memset(&pfd, 0, sizeof(pfd));
	pfd[0].fd = rfd;
	pfd[0].events = POLLIN | POLLRDNORM;
	pfd[1].fd = wfd;
	pfd[1].events = POLLOUT | POLLWRNORM;

	/* We expect the FIFO to be writable but not readable. */
	RL(ret = poll(pfd, 2, 0));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfd[0].revents, 0,
	    "pfd[0].revents=0x%x", pfd[0].revents);
	ATF_REQUIRE_EQ_MSG(pfd[1].revents, POLLOUT|POLLWRNORM,
	    "pfd[1].revents=0x%x", pfd[1].revents);

	/* Write a single byte of data into the FIFO. */
	RL(nwrit = write(wfd, buf, 1));
	ATF_REQUIRE_EQ_MSG(nwrit, 1, "nwrit=%zd", nwrit);

	/* We expect the FIFO to be readable and writable. */
	RL(ret = poll(pfd, 2, 0));
	ATF_REQUIRE_EQ_MSG(ret, 2, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfd[0].revents, POLLIN|POLLRDNORM,
	    "pfd[0].revents=0x%x", pfd[0].revents);
	ATF_REQUIRE_EQ_MSG(pfd[1].revents, POLLOUT|POLLWRNORM,
	    "pfd[1].revents=0x%x", pfd[1].revents);

	/* Read that single byte back out. */
	RL(nread = read(rfd, buf, 1));
	ATF_REQUIRE_EQ_MSG(nread, 1, "nread=%zd", nread);

	/*
	 * Write data into the FIFO until it is full, which is
	 * defined as insufficient buffer space to hold a the
	 * maximum atomic pipe write size.
	 */
	while (write(wfd, buf, pipe_buf) != -1) {
		continue;
	}
	ATF_REQUIRE_EQ_MSG(errno, EAGAIN, "errno=%d", errno);

	/* We expect the FIFO to be readble but not writable. */
	RL(ret = poll(pfd, 2, 0));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfd[0].revents, POLLIN|POLLRDNORM,
	    "pfd[0].revents=0x%x", pfd[0].revents);
	ATF_REQUIRE_EQ_MSG(pfd[1].revents, 0,
	    "pfd[1].revents=0x%x", pfd[1].revents);

	/* Read a single byte of data from the FIFO. */
	RL(nread = read(rfd, buf, 1));
	ATF_REQUIRE_EQ_MSG(nread, 1, "nread=%zd", nread);

	/*
	 * Because we have read only a single byte out, there will
	 * be insufficient space for a pipe_buf-sized message, so
	 * the FIFO should still not be writable.
	 */
	RL(ret = poll(pfd, 2, 0));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfd[0].revents, POLLIN|POLLRDNORM,
	    "pfd[0].revents=0x%x", pfd[0].revents);
	ATF_REQUIRE_EQ_MSG(pfd[1].revents, 0,
	    "pfd[1].revents=0x%x", pfd[1].revents);

	/*
	 * Now read enough so that exactly pipe_buf space should
	 * be available.  The FIFO should be writable after that.
	 * N.B. we don't care if it's readable at this point.
	 */
	RL(nread = read(rfd, buf, pipe_buf - 1));
	ATF_REQUIRE_EQ_MSG(nread, pipe_buf - 1, "nread=%zd pipe_buf-1=%ld",
	    nread, pipe_buf - 1);
	RL(ret = poll(pfd, 2, 0));
	ATF_REQUIRE_MSG(ret >= 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfd[1].revents, POLLOUT|POLLWRNORM,
	    "pfd[1].revents=0x%x", pfd[1].revents);

	/*
	 * Now read all of the data out of the FIFO and ensure that
	 * we get back to the initial state.
	 */
	while (read(rfd, buf, pipe_buf) != -1) {
		continue;
	}
	ATF_REQUIRE_EQ_MSG(errno, EAGAIN, "errno=%d", errno);

	RL(ret = poll(pfd, 2, 0));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfd[0].revents, 0,
	    "pfd[0].revents=0x%x", pfd[0].revents);
	ATF_REQUIRE_EQ_MSG(pfd[1].revents, POLLOUT|POLLWRNORM,
	    "pfd[1].revents=0x%x", pfd[1].revents);

	RL(close(wfd));
	RL(close(rfd));
}

ATF_TC_CLEANUP(fifo_inout, tc)
{
	(void)unlink(fifo_path);
}

ATF_TC_WITH_CLEANUP(fifo_hup1);
ATF_TC_HEAD(fifo_hup1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Check POLLHUP behavior with fifos [1]");
}

ATF_TC_BODY(fifo_hup1, tc)
{
	struct pollfd pfd;
	int rfd, wfd;
	int ret;

	fifo_support();

	RL(mkfifo(fifo_path, 0600));
	RL(rfd = open(fifo_path, O_RDONLY | O_NONBLOCK));
	RL(wfd = open(fifo_path, O_WRONLY));

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = rfd;
	pfd.events = POLLIN;

	RL(close(wfd));

	RL(ret = poll(&pfd, 1, 0));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	ATF_REQUIRE_EQ_MSG((pfd.revents & (POLLHUP|POLLOUT)), POLLHUP,
	    "revents=0x%x expected POLLHUP=0x%x but not POLLOUT=0x%x",
	    pfd.revents, POLLHUP, POLLOUT);

	/*
	 * Check that POLLHUP is cleared when a writer re-connects.
	 * Since the writer will not put any data into the FIFO, we
	 * expect no events.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = rfd;
	pfd.events = POLLIN;

	RL(wfd = open(fifo_path, O_WRONLY));
	RL(ret = poll(&pfd, 1, 0));
	ATF_REQUIRE_EQ_MSG(ret, 0, "got: %d", ret);
}

ATF_TC_CLEANUP(fifo_hup1, tc)
{
	(void)unlink(fifo_path);
}

ATF_TC_WITH_CLEANUP(fifo_hup2);
ATF_TC_HEAD(fifo_hup2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Check POLLHUP behavior with fifos [2]");
}

ATF_TC_BODY(fifo_hup2, tc)
{
	struct pollfd pfd;
	int rfd, wfd;
	pid_t pid;
	struct timespec ts1, ts2;
	int ret;

	fifo_support();

	RL(mkfifo(fifo_path, 0600));
	RL(rfd = open(fifo_path, O_RDONLY | O_NONBLOCK));
	RL(wfd = open(fifo_path, O_WRONLY));

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = rfd;
	pfd.events = POLLIN;

	RL(pid = fork());
	if (pid == 0) {
		if (close(rfd))
			_exit(1);
		sleep(5);
		if (close(wfd))
			_exit(1);
		_exit(0);
	}
	RL(close(wfd));

	RL(clock_gettime(CLOCK_MONOTONIC, &ts1));
	RL(ret = poll(&pfd, 1, INFTIM));
	ATF_REQUIRE_EQ_MSG(ret, 1, "got: %d", ret);
	RL(clock_gettime(CLOCK_MONOTONIC, &ts2));

	/* Make sure at least a couple of seconds have elapsed. */
	ATF_REQUIRE_MSG(ts2.tv_sec - ts1.tv_sec >= 2,
	    "ts1=%lld.%09ld ts2=%lld.%09ld",
	    (long long)ts1.tv_sec, ts1.tv_nsec,
	    (long long)ts2.tv_sec, ts2.tv_nsec);

	ATF_REQUIRE_EQ_MSG((pfd.revents & (POLLHUP|POLLOUT)), POLLHUP,
	    "revents=0x%x expected POLLHUP=0x%x but not POLLOUT=0x%x",
	    pfd.revents, POLLHUP, POLLOUT);
}

ATF_TC_CLEANUP(fifo_hup2, tc)
{
	(void)unlink(fifo_path);
}

static void
fillpipebuf(int writefd)
{
	char buf[BUFSIZ] = {0};
	size_t n = 0;
	ssize_t nwrit;
	int flags;

	RL(flags = fcntl(writefd, F_GETFL));
	RL(fcntl(writefd, F_SETFL, flags|O_NONBLOCK));
	while ((nwrit = write(writefd, buf, sizeof(buf))) != -1)
		n += (size_t)nwrit;
	ATF_CHECK_EQ_MSG(errno, EAGAIN, "errno=%d", errno);
	RL(fcntl(writefd, F_SETFL, flags));
	fprintf(stderr, "filled %d with %zu bytes\n", writefd, n);
}

static void
check_write_fail(int writefd, int error)
{
	int flags;
	void (*sighandler)(int);
	char c = 0;
	ssize_t nwrit;

	RL(flags = fcntl(writefd, F_GETFL));
	RL(fcntl(writefd, F_SETFL, flags|O_NONBLOCK));

	REQUIRE_LIBC(sighandler = signal(SIGPIPE, SIG_IGN), SIG_ERR);
	ATF_CHECK_ERRNO(error, (nwrit = write(writefd, &c, 1)) == -1);
	ATF_CHECK_EQ_MSG(nwrit, -1, "nwrit=%zd", nwrit);
	REQUIRE_LIBC(signal(SIGPIPE, sighandler), SIG_ERR);

	RL(fcntl(writefd, F_SETFL, flags));
}

static void
check_read_eof(int readfd)
{
	int flags;
	char c;
	ssize_t nread;

	RL(flags = fcntl(readfd, F_GETFL));
	RL(fcntl(readfd, F_SETFL, flags|O_NONBLOCK));

	RL(nread = read(readfd, &c, 1));
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zu", nread);

	RL(fcntl(readfd, F_SETFL, flags));
}

static void
check_pollclosed_delayed_write(int writefd, int readfd,
    int expected, int writeerror)
{
	struct pollfd pfd = { .fd = writefd, .events = POLLOUT };
	struct timespec start, end, delta;
	int nfds;

	/*
	 * Don't let poll sleep for more than 3sec.  (The close delay
	 * will be 2sec, and we make sure that we sleep at least 1sec.)
	 */
	REQUIRE_LIBC(alarm(3), (unsigned)-1);

	/*
	 * Wait in poll(2) indefinitely (subject to the alarm) and
	 * measure how long we slept.
	 */
	fprintf(stderr, "poll %d\n", writefd);
	RL(clock_gettime(CLOCK_MONOTONIC, &start));
	RL(nfds = poll(&pfd, 1, INFTIM));
	RL(clock_gettime(CLOCK_MONOTONIC, &end));
	fprintf(stderr, "poll %d done nfds=%d\n", writefd, nfds);

	REQUIRE_LIBC(alarm(0), (unsigned)-1);

	/*
	 * The reader has been closed, so write will fail immediately
	 * with EPIPE/SIGPIPE, and thus POLLOUT must be set.  POLLHUP
	 * is only returned for reads, not for writes (and is mutually
	 * exclusive with POLLOUT).  Except we _do_ return POLLHUP
	 * instead of POLLOUT for terminals.
	 */
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.fd, writefd, "pfd.fd=%d writefd=%d",
	    pfd.fd, writefd);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)), expected,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, expected, POLLOUT, POLLHUP, POLLIN);

	/*
	 * We should have slept at least 1sec.
	 */
	timespecsub(&end, &start, &delta);
	ATF_CHECK_MSG(delta.tv_sec >= 1,
	    "slept only %lld.%09ld", (long long)delta.tv_sec, delta.tv_nsec);

	/*
	 * Write should fail with EPIPE/SIGPIPE now, or EIO for
	 * terminals -- and continue to do so.
	 */
	check_write_fail(writefd, writeerror);
	check_write_fail(writefd, writeerror);
}

static void
check_pollclosed_delayed_write_fifopipesocket(int writefd, int readfd)
{

	check_pollclosed_delayed_write(writefd, readfd, POLLOUT, EPIPE);
}

static void
check_pollclosed_delayed_write_terminal(int writefd, int readfd)
{

	check_pollclosed_delayed_write(writefd, readfd, POLLHUP, EIO);
}

static void
check_pollclosed_delayed_read(int readfd, int writefd, int pollhup)
{
	struct pollfd pfd;
	struct timespec start, end, delta;
	int nfds;

	/*
	 * Don't let poll sleep for more than 3sec.  (The close delay
	 * will be 2sec, and we make sure that we sleep at least 1sec.)
	 */
	REQUIRE_LIBC(alarm(3), (unsigned)-1);

	/*
	 * Wait in poll(2) indefinitely (subject to the alarm) and
	 * measure how long we slept.
	 */
	pfd = (struct pollfd) { .fd = readfd, .events = POLLIN };
	fprintf(stderr, "poll %d\n", readfd);
	RL(clock_gettime(CLOCK_MONOTONIC, &start));
	RL(nfds = poll(&pfd, 1, INFTIM));
	RL(clock_gettime(CLOCK_MONOTONIC, &end));
	fprintf(stderr, "poll %d done nfds=%d\n", readfd, nfds);

	REQUIRE_LIBC(alarm(0), (unsigned)-1);

	/*
	 * Read will yield EOF without blocking, so POLLIN should be
	 * set, and the write side has been closed, so POLLHUP should
	 * also be set, unsolicited, if this is a pipe or FIFO -- but
	 * not if it's a socket, where POLLHUP is never set.  Since we
	 * didn't ask for POLLOUT, it should be clear.
	 */
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.fd, readfd, "pfd.fd=%d readfd=%d writefd=%d",
	    pfd.fd, readfd, writefd);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)),
	    pollhup|POLLIN,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, pollhup|POLLIN, POLLHUP, POLLIN, POLLOUT);

	/*
	 * We should have slept at least 1sec.
	 */
	timespecsub(&end, &start, &delta);
	ATF_CHECK_MSG(delta.tv_sec >= 1,
	    "slept only %lld.%09ld", (long long)delta.tv_sec, delta.tv_nsec);

	/*
	 * Read should return EOF now -- and continue to do so.
	 */
	check_read_eof(readfd);
	check_read_eof(readfd);

	/*
	 * POLLHUP|POLLIN state should be persistent (until the writer
	 * side is reopened if possible, as in a named pipe).
	 */
	pfd = (struct pollfd) { .fd = readfd, .events = POLLIN };
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.fd, readfd, "pfd.fd=%d readfd=%d writefd=%d",
	    pfd.fd, readfd, writefd);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)),
	    pollhup|POLLIN,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, pollhup|POLLIN, POLLHUP, POLLIN, POLLOUT);
}

static void
check_pollclosed_delayed_read_devfifopipe(int readfd, int writefd)
{

	check_pollclosed_delayed_read(readfd, writefd, POLLHUP);
}

static void
check_pollclosed_delayed_read_socket(int readfd, int writefd)
{

	check_pollclosed_delayed_read(readfd, writefd, /*no POLLHUP*/0);
}

static void
check_pollclosed_delayed_process(int pollfd, int closefd,
    void (*check_pollhup)(int, int))
{
	pid_t pid;
	int status;

	/*
	 * Fork a child to close closefd after a 2sec delay.
	 */
	RL(pid = fork());
	if (pid == 0) {
		sleep(2);
		fprintf(stderr, "[child] close %d\n", closefd);
		if (close(closefd) == -1)
			_exit(1);
		_exit(0);
	}

	/*
	 * Close closefd in the parent so the child has the last
	 * reference to it.
	 */
	fprintf(stderr, "[parent] close %d\n", closefd);
	RL(close(closefd));

	/*
	 * Test poll(2).
	 */
	(*check_pollhup)(pollfd, closefd);

	/*
	 * Wait for the child and make sure it exited successfully.
	 */
	RL(waitpid(pid, &status, 0));
	ATF_CHECK_EQ_MSG(status, 0, "child exited with status 0x%x", status);
}

static void *
check_pollclosed_thread(void *cookie)
{
	int *closefdp = cookie;

	sleep(2);
	fprintf(stderr, "[thread] close %d\n", *closefdp);
	RL(close(*closefdp));
	return NULL;
}

static void
check_pollclosed_delayed_thread(int pollfd, int closefd,
    void (*check_pollhup)(int, int))
{
	pthread_t t;

	/*
	 * Create a thread to close closefd (in this process, not a
	 * child) after a 2sec delay.
	 */
	RZ(pthread_create(&t, NULL, &check_pollclosed_thread, &closefd));

	/*
	 * Test poll(2).
	 */
	(*check_pollhup)(pollfd, closefd);

	/*
	 * Wait for the thread to complete.
	 */
	RZ(pthread_join(t, NULL));
}

static void
check_pollclosed_immediate_write(int writefd, int readfd, int expected,
    int writeerror)
{
	struct pollfd pfd = { .fd = writefd, .events = POLLOUT };
	int nfds;

	/*
	 * Close the reader side immediately.
	 */
	fprintf(stderr, "[immediate] close %d\n", readfd);
	RL(close(readfd));

	/*
	 * The reader has been closed, so write will fail immediately
	 * with EPIPE/SIGPIPE, and thus POLLOUT must be set.  POLLHUP
	 * is only returned for reads, not for writes (and is mutually
	 * exclusive with POLLOUT).  Except we _do_ return POLLHUP
	 * instead of POLLOUT for terminals.
	 */
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.fd, writefd, "pfd.fd=%d writefd=%d",
	    pfd.fd, writefd);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)), expected,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, expected, POLLOUT, POLLHUP, POLLIN);

	/*
	 * Write should fail with EPIPE/SIGPIPE now -- and continue to
	 * do so.
	 */
	check_write_fail(writefd, writeerror);
	check_write_fail(writefd, writeerror);
}

static void
check_pollclosed_immediate_readnone(int readfd, int writefd, int pollhup)
{
	struct pollfd pfd = { .fd = readfd, .events = POLLIN };
	int nfds;

	/*
	 * Close the writer side immediately.
	 */
	fprintf(stderr, "[immediate] close %d\n", writefd);
	RL(close(writefd));

	/*
	 * Read will yield EOF without blocking, so POLLIN should be
	 * set, and the write side has been closed, so POLLHUP should
	 * be set, unsolicited, if this is a pipe or FIFO -- but not if
	 * it's a socket, where POLLHUP is never set.  Since we didn't
	 * ask for POLLOUT, it should be clear.
	 */
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)),
	    pollhup|POLLIN,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, pollhup|POLLIN, POLLHUP, POLLIN, POLLOUT);

	/*
	 * Read should return EOF now -- and continue to do so.
	 */
	check_read_eof(readfd);
	check_read_eof(readfd);
}

static void
check_pollclosed_immediate_readsome(int readfd, int writefd, int pollhup)
{
	struct pollfd pfd;
	char buf[BUFSIZ];
	ssize_t nread;
	int nfds;

	/*
	 * Close the writer side immediately.
	 */
	fprintf(stderr, "[immediate] close %d\n", writefd);
	RL(close(writefd));

	/*
	 * Some data should be ready to read, so POLLIN should be set,
	 * and the write side has been closed, so POLLHUP should also
	 * be set, unsolicited, if this is a pipe or FIFO -- but not if
	 * it's a socket, where POLLHUP is never set.  Since we didn't
	 * ask for POLLOUT, it should be clear.
	 */
	pfd = (struct pollfd) { .fd = readfd, .events = POLLIN };
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)),
	    pollhup|POLLIN,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, pollhup|POLLIN, POLLHUP, POLLIN, POLLOUT);

	/*
	 * Read all the data.  Each read should complete instantly --
	 * no blocking, either because there's data to read or because
	 * the writer has hung up and we get EOF.
	 */
	do {
		REQUIRE_LIBC(alarm(1), (unsigned)-1);
		RL(nread = read(readfd, buf, sizeof(buf)));
		REQUIRE_LIBC(alarm(0), (unsigned)-1);
	} while (nread != 0);

	/*
	 * Read will yield EOF without blocking, so POLLIN should be
	 * set, and the write side has been closed, so POLLHUP should
	 * also be set, unsolicited, if this is a pipe or FIFO -- but
	 * not if it's a socket, where POLLHUP is never set.  Since we
	 * didn't ask for POLLOUT, it should be clear.
	 */
	pfd = (struct pollfd) { .fd = readfd, .events = POLLIN };
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)),
	    pollhup|POLLIN,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, pollhup|POLLIN, POLLHUP, POLLIN, POLLOUT);

	/*
	 * Read should return EOF now -- and continue to do so.
	 */
	check_read_eof(readfd);
	check_read_eof(readfd);

	/*
	 * POLLHUP|POLLIN state should be persistent (until the writer
	 * side is reopened if possible, as in a named pipe).
	 */
	pfd = (struct pollfd) { .fd = readfd, .events = POLLIN };
	RL(nfds = poll(&pfd, 1, 0));
	ATF_CHECK_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG((pfd.revents & (POLLHUP|POLLIN|POLLOUT)),
	    pollhup|POLLIN,
	    "revents=0x%x expected=0x%x"
	    " POLLHUP=0x%x POLLIN=0x%x POLLOUT=0x%x",
	    pfd.revents, pollhup|POLLIN, POLLHUP, POLLIN, POLLOUT);
}

static void *
pollclosed_fifo_writer_thread(void *cookie)
{
	int *pp = cookie;

	RL(*pp = open(fifo_path, O_WRONLY));
	return NULL;
}

static void *
pollclosed_fifo_reader_thread(void *cookie)
{
	int *pp = cookie;

	RL(*pp = open(fifo_path, O_RDONLY));
	return NULL;
}

static void
pollclosed_fifo0_setup(int *writefdp, int *readfdp)
{
	int p0, p1;
	pthread_t t;

	fifo_support();

	RL(mkfifo(fifo_path, 0600));
	RZ(pthread_create(&t, NULL, &pollclosed_fifo_reader_thread, &p0));
	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	RL(p1 = open(fifo_path, O_WRONLY));
	REQUIRE_LIBC(alarm(0), (unsigned)-1);
	RZ(pthread_join(t, NULL));

	*writefdp = p1;
	*readfdp = p0;
}

static void
pollclosed_fifo1_setup(int *writefdp, int *readfdp)
{
	int p0, p1;
	pthread_t t;

	fifo_support();

	RL(mkfifo(fifo_path, 0600));
	RZ(pthread_create(&t, NULL, &pollclosed_fifo_writer_thread, &p0));
	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	RL(p1 = open(fifo_path, O_RDONLY));
	REQUIRE_LIBC(alarm(0), (unsigned)-1);
	RZ(pthread_join(t, NULL));

	*writefdp = p0;
	*readfdp = p1;
}

static void
pollclosed_pipe_setup(int *writefdp, int *readfdp)
{
	int p[2];

	RL(pipe(p));

	*readfdp = p[0];	/* reader side */
	*writefdp = p[1];	/* writer side */
}

static void
pollclosed_ptyapp_setup(int *writefdp, int *readfdp)
{
	int hostfd, appfd;
	struct termios t;
	char *pts;

	RL(hostfd = posix_openpt(O_RDWR|O_NOCTTY));
	RL(grantpt(hostfd));
	RL(unlockpt(hostfd));
	REQUIRE_LIBC(pts = ptsname(hostfd), NULL);
	RL(appfd = open(pts, O_RDWR|O_NOCTTY));

	RL(tcgetattr(appfd, &t));
	t.c_lflag &= ~ICANON;	/* block rather than drop input */
	RL(tcsetattr(appfd, TCSANOW, &t));

	*readfdp = appfd;
	*writefdp = hostfd;
}

static void
pollclosed_ptyhost_setup(int *writefdp, int *readfdp)
{
	int hostfd, appfd;
	struct termios t;
	char *pts;

	RL(hostfd = posix_openpt(O_RDWR|O_NOCTTY));
	RL(grantpt(hostfd));
	RL(unlockpt(hostfd));
	REQUIRE_LIBC(pts = ptsname(hostfd), NULL);
	RL(appfd = open(pts, O_RDWR|O_NOCTTY));

	RL(tcgetattr(appfd, &t));
	t.c_lflag &= ~ICANON;	/* block rather than drop input */
	RL(tcsetattr(appfd, TCSANOW, &t));

	*writefdp = appfd;
	*readfdp = hostfd;
}

static void
pollclosed_socketpair0_setup(int *writefdp, int *readfdp)
{
	int s[2];

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, s));
	*readfdp = s[0];
	*writefdp = s[1];
}

static void
pollclosed_socketpair1_setup(int *writefdp, int *readfdp)
{
	int s[2];

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, s));
	*readfdp = s[1];
	*writefdp = s[0];
}

/*
 * Cartesian product of:
 *
 * 1. [fifo0] first fifo opener
 * 2. [fifo1] second fifo opener
 * 3. [pipe] pipe
 * 4. [ptyhost] host side of pty
 * 5. [ptyapp] application side of pty
 * 6. [socketpair0] first side of socket pair
 * 7. [socketpair1] second side of socket pair
 *
 * with
 *
 * 1. [immediate] closed before poll starts
 * 2. [delayed_thread] closed by another thread after poll starts
 * 3. [delayed_process] closed by another process after poll starts
 *
 * with
 *
 * 1. [writefull] close reader, poll for write when buffer full
 * 2. [writeempty] close reader, poll for write when buffer empty
 * 3. [readnone] close writer, poll for read when nothing to read
 * 4. [readsome] close writer, poll for read when something to read
 *
 * except that in the delayed cases we only do writefull [write] and
 * readnone [read], because there's no delay in the writeempty/readsome
 * cases.
 */

ATF_TC(pollclosed_fifo0_immediate_writefull);
ATF_TC_HEAD(pollclosed_fifo0_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_immediate_writefull, tc)
{
	int writefd, readfd;

	pollclosed_fifo0_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_fifo0_immediate_writeempty);
ATF_TC_HEAD(pollclosed_fifo0_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_immediate_writeempty, tc)
{
	int writefd, readfd;

	pollclosed_fifo0_setup(&writefd, &readfd);
	/* don't fill the pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_fifo0_immediate_readsome);
ATF_TC_HEAD(pollclosed_fifo0_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_immediate_readsome, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns nothing, when it is supposed to return
	 * POLLHUP|POLLIN.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_fifo1_setup(&writefd, &readfd); /* reverse r/w */
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_fifo0_immediate_readnone);
ATF_TC_HEAD(pollclosed_fifo0_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_immediate_readnone, tc)
{
	int writefd, readfd;

	pollclosed_fifo1_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill the pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_fifo0_delayed_process_write);
ATF_TC_HEAD(pollclosed_fifo0_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_delayed_process_write, tc)
{
	int writefd, readfd;

	pollclosed_fifo0_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_fifo0_delayed_process_read);
ATF_TC_HEAD(pollclosed_fifo0_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_delayed_process_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) wakes up with POLLHUP|POLLIN, but the state isn't
	 * persistent as it is supposed to be -- it returns nothing
	 * after that.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_fifo1_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_fifo0_delayed_thread_write);
ATF_TC_HEAD(pollclosed_fifo0_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_delayed_thread_write, tc)
{
	int writefd, readfd;

	pollclosed_fifo0_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_fifo0_delayed_thread_read);
ATF_TC_HEAD(pollclosed_fifo0_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo0_delayed_thread_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) wakes up with POLLHUP|POLLIN, but the state isn't
	 * persistent as it is supposed to be -- it returns nothing
	 * after that.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_fifo1_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_thread(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_fifo1_immediate_writefull);
ATF_TC_HEAD(pollclosed_fifo1_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_immediate_writefull, tc)
{
	int writefd, readfd;

	pollclosed_fifo1_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_fifo1_immediate_writeempty);
ATF_TC_HEAD(pollclosed_fifo1_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_immediate_writeempty, tc)
{
	int writefd, readfd;

	pollclosed_fifo1_setup(&writefd, &readfd);
	/* don't fill the pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_fifo1_immediate_readsome);
ATF_TC_HEAD(pollclosed_fifo1_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_immediate_readsome, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns nothing, when it is supposed to return
	 * POLLHUP|POLLIN.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_fifo0_setup(&writefd, &readfd); /* reverse r/w */
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_fifo1_immediate_readnone);
ATF_TC_HEAD(pollclosed_fifo1_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_immediate_readnone, tc)
{
	int writefd, readfd;

	pollclosed_fifo0_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill the pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_fifo1_delayed_process_write);
ATF_TC_HEAD(pollclosed_fifo1_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_delayed_process_write, tc)
{
	int writefd, readfd;

	pollclosed_fifo1_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_fifo1_delayed_process_read);
ATF_TC_HEAD(pollclosed_fifo1_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_delayed_process_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) wakes up with POLLHUP|POLLIN, but the state isn't
	 * persistent as it is supposed to be -- it returns nothing
	 * after that.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_fifo0_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_fifo1_delayed_thread_write);
ATF_TC_HEAD(pollclosed_fifo1_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_delayed_thread_write, tc)
{
	int writefd, readfd;

	pollclosed_fifo1_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_fifo1_delayed_thread_read);
ATF_TC_HEAD(pollclosed_fifo1_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second opener of a named pipe");
}
ATF_TC_BODY(pollclosed_fifo1_delayed_thread_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) wakes up with POLLHUP|POLLIN, but the state isn't
	 * persistent as it is supposed to be -- it returns nothing
	 * after that.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_fifo0_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_pipe_immediate_writefull);
ATF_TC_HEAD(pollclosed_pipe_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_immediate_writefull, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP|POLLOUT, which is forbidden --
	 * POLLHUP and POLLOUT are mutually exclusive.  And POLLHUP is
	 * only supposed to be returned by polling for read, not
	 * polling for write.  So it should be POLLOUT.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_pipe_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_pipe_immediate_writeempty);
ATF_TC_HEAD(pollclosed_pipe_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_immediate_writeempty, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP|POLLOUT, which is forbidden --
	 * POLLHUP and POLLOUT are mutually exclusive.  And POLLHUP is
	 * only supposed to be returned by polling for read, not
	 * polling for write.  So it should be POLLOUT.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_pipe_setup(&writefd, &readfd);
	/* don't fill pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_pipe_immediate_readsome);
ATF_TC_HEAD(pollclosed_pipe_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_immediate_readsome, tc)
{
	int writefd, readfd;

	pollclosed_pipe_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_pipe_immediate_readnone);
ATF_TC_HEAD(pollclosed_pipe_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_immediate_readnone, tc)
{
	int writefd, readfd;

	pollclosed_pipe_setup(&writefd, &readfd);
	/* don't fill pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_pipe_delayed_process_write);
ATF_TC_HEAD(pollclosed_pipe_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_delayed_process_write, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP|POLLOUT, which is forbidden --
	 * POLLHUP and POLLOUT are mutually exclusive.  And POLLHUP is
	 * only supposed to be returned by polling for read, not
	 * polling for write.  So it should be POLLOUT.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_pipe_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_pipe_delayed_process_read);
ATF_TC_HEAD(pollclosed_pipe_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_delayed_process_read, tc)
{
	int writefd, readfd;

	pollclosed_pipe_setup(&writefd, &readfd);
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_pipe_delayed_thread_write);
ATF_TC_HEAD(pollclosed_pipe_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_delayed_thread_write, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP|POLLOUT, which is forbidden --
	 * POLLHUP and POLLOUT are mutually exclusive.  And POLLHUP is
	 * only supposed to be returned by polling for read, not
	 * polling for write.  So it should be POLLOUT.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_pipe_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_pipe_delayed_thread_read);
ATF_TC_HEAD(pollclosed_pipe_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with a closed pipe");
}
ATF_TC_BODY(pollclosed_pipe_delayed_thread_read, tc)
{
	int writefd, readfd;

	pollclosed_pipe_setup(&writefd, &readfd);
	/* don't fill pipe buf */
	check_pollclosed_delayed_thread(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_ptyapp_immediate_writefull);
ATF_TC_HEAD(pollclosed_ptyapp_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_immediate_writefull, tc)
{
	int writefd, readfd;

	pollclosed_ptyapp_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLHUP, EIO);
}

ATF_TC(pollclosed_ptyapp_immediate_writeempty);
ATF_TC_HEAD(pollclosed_ptyapp_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_immediate_writeempty, tc)
{
	int writefd, readfd;

	pollclosed_ptyapp_setup(&writefd, &readfd);
	/* don't fill the pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLHUP, EIO);
}

ATF_TC(pollclosed_ptyapp_immediate_readsome);
ATF_TC_HEAD(pollclosed_ptyapp_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_immediate_readsome, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyhost_setup(&writefd, &readfd); /* reverse r/w */
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_ptyapp_immediate_readnone);
ATF_TC_HEAD(pollclosed_ptyapp_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_immediate_readnone, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyhost_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill the pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_ptyapp_delayed_process_write);
ATF_TC_HEAD(pollclosed_ptyapp_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_delayed_process_write, tc)
{
	int writefd, readfd;

	/*
	 * The poll(2) call is not woken by the concurrent close(2)
	 * call.
	 */
	atf_tc_expect_signal(SIGALRM, "PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyapp_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_terminal);
}

ATF_TC(pollclosed_ptyapp_delayed_process_read);
ATF_TC_HEAD(pollclosed_ptyapp_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_delayed_process_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyhost_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_ptyapp_delayed_thread_write);
ATF_TC_HEAD(pollclosed_ptyapp_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_delayed_thread_write, tc)
{
	int writefd, readfd;

	/*
	 * The poll(2) call is not woken by the concurrent close(2)
	 * call.
	 */
	atf_tc_expect_signal(SIGALRM, "PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyapp_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_terminal);
}

ATF_TC(pollclosed_ptyapp_delayed_thread_read);
ATF_TC_HEAD(pollclosed_ptyapp_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty application side");
}
ATF_TC_BODY(pollclosed_ptyapp_delayed_thread_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyhost_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_ptyhost_immediate_writefull);
ATF_TC_HEAD(pollclosed_ptyhost_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_immediate_writefull, tc)
{
	int writefd, readfd;

	pollclosed_ptyhost_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLHUP, EIO);
}

ATF_TC(pollclosed_ptyhost_immediate_writeempty);
ATF_TC_HEAD(pollclosed_ptyhost_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_immediate_writeempty, tc)
{
	int writefd, readfd;

	pollclosed_ptyhost_setup(&writefd, &readfd);
	/* don't fill the pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLHUP, EIO);
}

ATF_TC(pollclosed_ptyhost_immediate_readsome);
ATF_TC_HEAD(pollclosed_ptyhost_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_immediate_readsome, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyapp_setup(&writefd, &readfd); /* reverse r/w */
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_ptyhost_immediate_readnone);
ATF_TC_HEAD(pollclosed_ptyhost_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_immediate_readnone, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyapp_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill the pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, POLLHUP);
}

ATF_TC(pollclosed_ptyhost_delayed_process_write);
ATF_TC_HEAD(pollclosed_ptyhost_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_delayed_process_write, tc)
{
	int writefd, readfd;

	pollclosed_ptyhost_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_terminal);
}

ATF_TC(pollclosed_ptyhost_delayed_process_read);
ATF_TC_HEAD(pollclosed_ptyhost_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_delayed_process_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyapp_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_ptyhost_delayed_thread_write);
ATF_TC_HEAD(pollclosed_ptyhost_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_delayed_thread_write, tc)
{
	int writefd, readfd;

	pollclosed_ptyhost_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_terminal);
}

ATF_TC(pollclosed_ptyhost_delayed_thread_read);
ATF_TC_HEAD(pollclosed_ptyhost_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the pty host side");
}
ATF_TC_BODY(pollclosed_ptyhost_delayed_thread_read, tc)
{
	int writefd, readfd;

	/*
	 * poll(2) returns POLLHUP but not POLLIN even though read(2)
	 * would return EOF without blocking.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	pollclosed_ptyapp_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_thread(readfd, writefd,
	    &check_pollclosed_delayed_read_devfifopipe);
}

ATF_TC(pollclosed_socketpair0_immediate_writefull);
ATF_TC_HEAD(pollclosed_socketpair0_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_immediate_writefull, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_socketpair0_immediate_writeempty);
ATF_TC_HEAD(pollclosed_socketpair0_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_immediate_writeempty, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd);
	/* don't fill the pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_socketpair0_immediate_readsome);
ATF_TC_HEAD(pollclosed_socketpair0_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_immediate_readsome, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd); /* reverse r/w */
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, /*no POLLHUP*/0);
}

ATF_TC(pollclosed_socketpair0_immediate_readnone);
ATF_TC_HEAD(pollclosed_socketpair0_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_immediate_readnone, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill the pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, /*no POLLHUP*/0);
}

ATF_TC(pollclosed_socketpair0_delayed_process_write);
ATF_TC_HEAD(pollclosed_socketpair0_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_delayed_process_write, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_socketpair0_delayed_process_read);
ATF_TC_HEAD(pollclosed_socketpair0_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_delayed_process_read, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_socket);
}

ATF_TC(pollclosed_socketpair0_delayed_thread_write);
ATF_TC_HEAD(pollclosed_socketpair0_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_delayed_thread_write, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_socketpair0_delayed_thread_read);
ATF_TC_HEAD(pollclosed_socketpair0_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the first half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair0_delayed_thread_read, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_thread(readfd, writefd,
	    &check_pollclosed_delayed_read_socket);
}

ATF_TC(pollclosed_socketpair1_immediate_writefull);
ATF_TC_HEAD(pollclosed_socketpair1_immediate_writefull, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_immediate_writefull, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_socketpair1_immediate_writeempty);
ATF_TC_HEAD(pollclosed_socketpair1_immediate_writeempty, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_immediate_writeempty, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd);
	/* don't fill the pipe buf */
	check_pollclosed_immediate_write(writefd, readfd, POLLOUT, EPIPE);
}

ATF_TC(pollclosed_socketpair1_immediate_readsome);
ATF_TC_HEAD(pollclosed_socketpair1_immediate_readsome, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_immediate_readsome, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd); /* reverse r/w */
	fillpipebuf(writefd);
	check_pollclosed_immediate_readsome(readfd, writefd, /*no POLLHUP*/0);
}

ATF_TC(pollclosed_socketpair1_immediate_readnone);
ATF_TC_HEAD(pollclosed_socketpair1_immediate_readnone, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_immediate_readnone, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill the pipe buf */
	check_pollclosed_immediate_readnone(readfd, writefd, /*no POLLHUP*/0);
}

ATF_TC(pollclosed_socketpair1_delayed_process_write);
ATF_TC_HEAD(pollclosed_socketpair1_delayed_process_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_delayed_process_write, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_process(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_socketpair1_delayed_process_read);
ATF_TC_HEAD(pollclosed_socketpair1_delayed_process_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_delayed_process_read, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_socket);
}

ATF_TC(pollclosed_socketpair1_delayed_thread_write);
ATF_TC_HEAD(pollclosed_socketpair1_delayed_thread_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_delayed_thread_write, tc)
{
	int writefd, readfd;

	pollclosed_socketpair1_setup(&writefd, &readfd);
	fillpipebuf(writefd);
	check_pollclosed_delayed_thread(writefd, readfd,
	    &check_pollclosed_delayed_write_fifopipesocket);
}

ATF_TC(pollclosed_socketpair1_delayed_thread_read);
ATF_TC_HEAD(pollclosed_socketpair1_delayed_thread_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks POLLHUP with closing the second half of a socketpair");
}
ATF_TC_BODY(pollclosed_socketpair1_delayed_thread_read, tc)
{
	int writefd, readfd;

	pollclosed_socketpair0_setup(&writefd, &readfd); /* reverse r/w */
	/* don't fill pipe buf */
	check_pollclosed_delayed_process(readfd, writefd,
	    &check_pollclosed_delayed_read_socket);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, 3way);
	ATF_TP_ADD_TC(tp, basic);
	ATF_TP_ADD_TC(tp, err);

	ATF_TP_ADD_TC(tp, fifo_inout);
	ATF_TP_ADD_TC(tp, fifo_hup1);
	ATF_TP_ADD_TC(tp, fifo_hup2);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_immediate_writefull);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_immediate_writefull);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_immediate_writefull);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_immediate_writefull);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_immediate_writefull);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_immediate_writefull);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_immediate_writefull);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_immediate_writeempty);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_immediate_writeempty);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_immediate_writeempty);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_immediate_writeempty);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_immediate_writeempty);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_immediate_writeempty);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_immediate_writeempty);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_immediate_readsome);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_immediate_readsome);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_immediate_readsome);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_immediate_readsome);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_immediate_readsome);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_immediate_readsome);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_immediate_readsome);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_immediate_readnone);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_immediate_readnone);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_immediate_readnone);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_immediate_readnone);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_immediate_readnone);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_immediate_readnone);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_immediate_readnone);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_delayed_process_write);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_delayed_process_write);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_delayed_process_write);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_delayed_process_write);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_delayed_process_write);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_delayed_process_write);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_delayed_process_write);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_delayed_process_read);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_delayed_process_read);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_delayed_process_read);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_delayed_process_read);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_delayed_process_read);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_delayed_process_read);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_delayed_process_read);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_delayed_thread_write);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_delayed_thread_write);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_delayed_thread_write);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_delayed_thread_write);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_delayed_thread_write);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_delayed_thread_write);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_delayed_thread_write);

	ATF_TP_ADD_TC(tp, pollclosed_fifo0_delayed_thread_read);
	ATF_TP_ADD_TC(tp, pollclosed_fifo1_delayed_thread_read);
	ATF_TP_ADD_TC(tp, pollclosed_pipe_delayed_thread_read);
	ATF_TP_ADD_TC(tp, pollclosed_ptyapp_delayed_thread_read);
	ATF_TP_ADD_TC(tp, pollclosed_ptyhost_delayed_thread_read);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair0_delayed_thread_read);
	ATF_TP_ADD_TC(tp, pollclosed_socketpair1_delayed_thread_read);

	return atf_no_error();
}
