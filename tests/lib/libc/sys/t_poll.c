/*	$NetBSD: t_poll.c,v 1.8 2021/10/02 17:32:55 thorpej Exp $	*/

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

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

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

	pipe(pf);
	desc = pf[0];

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		(void)close(pf[1]);
		child1();
		_exit(0);
		/* NOTREACHED */
	}

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		(void)close(pf[1]);
		child2();
		_exit(0);
		/* NOTREACHED */
	}

	pid = fork();
	ATF_REQUIRE( pid >= 0);

	if (pid == 0) {
		(void)close(pf[1]);
		child3();
		_exit(0);
		/* NOTREACHED */
	}

	(void)sleep(10);

	(void)printf("parent write\n");

	ATF_REQUIRE(write(pf[1], "konec\n", 6) == 6);

	for(i = 0; i < 3; ++i)
		(void)wait(&status);

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

	ATF_REQUIRE_EQ(pipe(fds), 0);

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
	ATF_REQUIRE_EQ_MSG(ret = poll(&pfds[0], 1, 1), 0,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, 0, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, -1, "got: %d", pfds[1].revents);

	/* Check that the write end of the pipe as reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = poll(&pfds[1], 1, 1), 1,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, -1, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",\
	    pfds[1].revents);

	/* Check that only the write end of the pipe as reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = poll(pfds, 2, 1), 1,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, 0, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",
	    pfds[1].revents);

	/* Write data to our pipe. */
	ATF_REQUIRE_EQ(write(fds[1], "", 1), 1);

	/* Check that both ends of our pipe are reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = poll(pfds, 2, 1), 2,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, POLLIN, "got: %d",
	    pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",
	    pfds[1].revents);

	ATF_REQUIRE_EQ(close(fds[0]), 0);
	ATF_REQUIRE_EQ(close(fds[1]), 0);
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
		ATF_REQUIRE(unlink(fifo_path) == 0);
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

	fifo_support();

	ATF_REQUIRE(mkfifo(fifo_path, 0600) == 0);
	ATF_REQUIRE((rfd = open(fifo_path, O_RDONLY | O_NONBLOCK)) >= 0);
	ATF_REQUIRE((wfd = open(fifo_path, O_WRONLY | O_NONBLOCK)) >= 0);

	/* Get the maximum atomic pipe write size. */
	pipe_buf = fpathconf(wfd, _PC_PIPE_BUF);
	ATF_REQUIRE(pipe_buf > 1);

	buf = malloc(pipe_buf);
	ATF_REQUIRE(buf != NULL);

	memset(&pfd, 0, sizeof(pfd));
	pfd[0].fd = rfd;
	pfd[0].events = POLLIN | POLLRDNORM;
	pfd[1].fd = wfd;
	pfd[1].events = POLLOUT | POLLWRNORM;

	/* We expect the FIFO to be writable but not readable. */
	ATF_REQUIRE(poll(pfd, 2, 0) == 1);
	ATF_REQUIRE(pfd[0].revents == 0);
	ATF_REQUIRE(pfd[1].revents == (POLLOUT | POLLWRNORM));

	/* Write a single byte of data into the FIFO. */
	ATF_REQUIRE(write(wfd, buf, 1) == 1);

	/* We expect the FIFO to be readable and writable. */
	ATF_REQUIRE(poll(pfd, 2, 0) == 2);
	ATF_REQUIRE(pfd[0].revents == (POLLIN | POLLRDNORM));
	ATF_REQUIRE(pfd[1].revents == (POLLOUT | POLLWRNORM));

	/* Read that single byte back out. */
	ATF_REQUIRE(read(rfd, buf, 1) == 1);

	/*
	 * Write data into the FIFO until it is full, which is
	 * defined as insufficient buffer space to hold a the
	 * maximum atomic pipe write size.
	 */
	while (write(wfd, buf, pipe_buf) != -1) {
		continue;
	}
	ATF_REQUIRE(errno == EAGAIN);

	/* We expect the FIFO to be readble but not writable. */
	ATF_REQUIRE(poll(pfd, 2, 0) == 1);
	ATF_REQUIRE(pfd[0].revents == (POLLIN | POLLRDNORM));
	ATF_REQUIRE(pfd[1].revents == 0);

	/* Read a single byte of data from the FIFO. */
	ATF_REQUIRE(read(rfd, buf, 1) == 1);

	/*
	 * Because we have read only a single byte out, there will
	 * be insufficient space for a pipe_buf-sized message, so
	 * the FIFO should still not be writable.
	 */
	ATF_REQUIRE(poll(pfd, 2, 0) == 1);
	ATF_REQUIRE(pfd[0].revents == (POLLIN | POLLRDNORM));
	ATF_REQUIRE(pfd[1].revents == 0);

	/*
	 * Now read enough so that exactly pipe_buf space should
	 * be available.  The FIFO should be writable after that.
	 * N.B. we don't care if it's readable at this point.
	 */
	ATF_REQUIRE(read(rfd, buf, pipe_buf - 1) == pipe_buf - 1);
	ATF_REQUIRE(poll(pfd, 2, 0) >= 1);
	ATF_REQUIRE(pfd[1].revents == (POLLOUT | POLLWRNORM));

	/*
	 * Now read all of the data out of the FIFO and ensure that
	 * we get back to the initial state.
	 */
	while (read(rfd, buf, pipe_buf) != -1) {
		continue;
	}
	ATF_REQUIRE(errno == EAGAIN);

	ATF_REQUIRE(poll(pfd, 2, 0) == 1);
	ATF_REQUIRE(pfd[0].revents == 0);
	ATF_REQUIRE(pfd[1].revents == (POLLOUT | POLLWRNORM));

	(void)close(wfd);
	(void)close(rfd);
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

	fifo_support();

	ATF_REQUIRE(mkfifo(fifo_path, 0600) == 0);
	ATF_REQUIRE((rfd = open(fifo_path, O_RDONLY | O_NONBLOCK)) >= 0);
	ATF_REQUIRE((wfd = open(fifo_path, O_WRONLY)) >= 0);

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = rfd;
	pfd.events = POLLIN;

	(void)close(wfd);

	ATF_REQUIRE(poll(&pfd, 1, 0) == 1);
	ATF_REQUIRE((pfd.revents & POLLHUP) != 0);

	/*
	 * Check that POLLHUP is cleared when a writer re-connects.
	 * Since the writer will not put any data into the FIFO, we
	 * expect no events.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = rfd;
	pfd.events = POLLIN;

	ATF_REQUIRE((wfd = open(fifo_path, O_WRONLY)) >= 0);
	ATF_REQUIRE(poll(&pfd, 1, 0) == 0);
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

	fifo_support();

	ATF_REQUIRE(mkfifo(fifo_path, 0600) == 0);
	ATF_REQUIRE((rfd = open(fifo_path, O_RDONLY | O_NONBLOCK)) >= 0);
	ATF_REQUIRE((wfd = open(fifo_path, O_WRONLY)) >= 0);

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = rfd;
	pfd.events = POLLIN;

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {
		(void)close(rfd);
		sleep(5);
		(void)close(wfd);
		_exit(0);
	}
	(void)close(wfd);

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ts1) == 0);
	ATF_REQUIRE(poll(&pfd, 1, INFTIM) == 1);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ts2) == 0);

	/* Make sure at least a couple of seconds have elapsed. */
	ATF_REQUIRE(ts2.tv_sec - ts1.tv_sec >= 2);

	ATF_REQUIRE((pfd.revents & POLLHUP) != 0);
}

ATF_TC_CLEANUP(fifo_hup2, tc)
{
	(void)unlink(fifo_path);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, 3way);
	ATF_TP_ADD_TC(tp, basic);
	ATF_TP_ADD_TC(tp, err);

	ATF_TP_ADD_TC(tp, fifo_inout);
	ATF_TP_ADD_TC(tp, fifo_hup1);
	ATF_TP_ADD_TC(tp, fifo_hup2);

	return atf_no_error();
}
