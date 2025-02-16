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

#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

static const char  path[] = "close_range";

ATF_TC_WITH_CLEANUP(close_range_basic);
ATF_TC_HEAD(close_range_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of close_range(2), #1");
}

ATF_TC_BODY(close_range_basic, tc)
{
	int fd, cur1, cur2;

	(void)close_range(STDERR_FILENO + 1, UINT_MAX, 0);

	fd = open(path, O_RDONLY | O_CREAT, 0400);
	ATF_REQUIRE(fd >= 0);

	cur1 = fcntl(0, F_MAXFD);

	ATF_REQUIRE(cur1 == STDERR_FILENO + 1);
	ATF_REQUIRE(close_range(cur1, UINT_MAX, 0) == 0);

	cur2 = fcntl(0, F_MAXFD);

	ATF_REQUIRE(cur1 - 1 == cur2);
	ATF_REQUIRE(close(fd) == -1);
	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(close_range_basic, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(close_range_buffer);
ATF_TC_HEAD(close_range_buffer, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of close_range(2), #2");
}

ATF_TC_BODY(close_range_buffer, tc)
{
	int buf[16], cur, half;
	size_t i;

	/*
	 * Open a buffer of descriptors, close the half of
	 * these and verify that the result is consistent.
	 */
	ATF_REQUIRE(close_range(STDERR_FILENO + 1, UINT_MAX, 0) == 0);

	cur = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur == STDERR_FILENO);

	for (i = 0; i < __arraycount(buf); i++) {
		buf[i] = open(path, O_RDWR | O_CREAT, 0600);
		ATF_REQUIRE(buf[i] >= 0);
	}

	cur = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur == __arraycount(buf) + STDERR_FILENO);

	half = STDERR_FILENO + __arraycount(buf) / 2;
	ATF_REQUIRE(close_range(half, UINT_MAX, 0) == 0);

	cur = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur == half - 1);

	for (i = 0; i < __arraycount(buf); i++)
		(void)close(buf[i]);
}

ATF_TC_CLEANUP(close_range_buffer, tc)
{
	(void)unlink(path);
}

ATF_TC(close_range_err);
ATF_TC_HEAD(close_range_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors from close_range(2)");
}

ATF_TC_BODY(close_range_err, tc)
{

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, close_range(UINT_MAX, 0, 0) == -1);
}

ATF_TC(close_range_one);
ATF_TC_HEAD(close_range_one, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test close_range(1, UINT_MAX, 0)");
}

ATF_TC_BODY(close_range_one, tc)
{
	pid_t pid;
	int sta;

	pid = fork();
	ATF_REQUIRE(pid >= 0);

	if (pid == 0) {

		if (close_range(1, UINT_MAX, 0) != 0)
			_exit(10);

		_exit(fcntl(0, F_MAXFD));
	}


	(void)wait(&sta);

	/*
	 * STDIN_FILENO should still be open; WEXITSTATUS(1) == 0.
	 */
	if (WIFEXITED(sta) == 0 || WEXITSTATUS(sta) != 0)
		atf_tc_fail("not all descriptors were closed");
}

ATF_TC_WITH_CLEANUP(close_range_cloexec);
ATF_TC_HEAD(close_range_cloexec, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test close_range(2) with CLOSE_RANGE_CLOEXEC");
}

ATF_TC_BODY(close_range_cloexec, tc)
{
	int fd, cur1, cur2;
	int flags;

	fd = open(path, O_RDONLY | O_CREAT, 0400);
	ATF_REQUIRE(fd >= 0);

	flags = fcntl(fd, F_GETFD);
	ATF_REQUIRE(flags != -1);
	ATF_REQUIRE(fcntl(fd, F_SETFD, flags | FD_CLOEXEC) != -1);

	cur1 = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur1 == STDERR_FILENO + 1);
	ATF_REQUIRE(close_range(cur1, UINT_MAX, CLOSE_RANGE_CLOEXEC) == 0);

	cur2 = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur1 == cur2);

	flags = fcntl(fd, F_GETFD);
	ATF_REQUIRE(flags != -1);
	ATF_CHECK((flags & FD_CLOEXEC) != 0);

	ATF_REQUIRE(close(fd) == 0);
	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(close_range_cloexec, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(close_range_invalid_flag);
ATF_TC_HEAD(close_range_invalid_flag, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test close_range(2) with an invalid flag");
}

ATF_TC_BODY(close_range_invalid_flag, tc)
{
	int cur1, cur2;
	int unused_flag = 0x40000000;

	cur1 = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur1 == STDERR_FILENO);

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, close_range(STDERR_FILENO + 1, UINT_MAX, unused_flag) == -1);

	cur2 = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur1 == cur2);
}

ATF_TC_CLEANUP(close_range_invalid_flag, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(close_range_above);
ATF_TC_HEAD(close_range_above, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test close_range(2) doesn't close above a limit");
}

ATF_TC_BODY(close_range_above, tc)
{
	int fd, fd2;
	int cur1, cur2;

	fd = open(path, O_RDONLY | O_CREAT, 0400);
	ATF_REQUIRE(fd >= 0);

	fd2 = dup2(fd, 777);
	ATF_REQUIRE(fd2 == 777);

	cur1 = fcntl(0, F_MAXFD);

	ATF_REQUIRE(close_range(fd + 1, fd2 - 1, 0) == 0);

	cur2 = fcntl(0, F_MAXFD);
	ATF_REQUIRE(cur1 == cur2);

	ATF_REQUIRE(fcntl(fd, F_GETFD) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE(fcntl(fd2, F_GETFD) != -1);
	ATF_REQUIRE(close(fd2) == 0);

	ATF_REQUIRE(unlink(path) == 0);
}

ATF_TC_CLEANUP(close_range_above, tc)
{
	(void)unlink(path);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, close_range_basic);
	ATF_TP_ADD_TC(tp, close_range_buffer);
	ATF_TP_ADD_TC(tp, close_range_err);
	ATF_TP_ADD_TC(tp, close_range_one);
	ATF_TP_ADD_TC(tp, close_range_cloexec);
	ATF_TP_ADD_TC(tp, close_range_invalid_flag);
	ATF_TP_ADD_TC(tp, close_range_above);

	return atf_no_error();
}
