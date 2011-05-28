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

#include <sys/time.h>

#include <assert.h>
#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <atf-c.h>

ATF_TC(pollts);
ATF_TC_HEAD(pollts, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr",
	    "Basis functionality test for pollts(2)");
}

ATF_TC(pollts_sigmask);
ATF_TC_HEAD(pollts_sigmask, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr",
	    "Check that pollts_sigmask(2) restores the signal mask");
}

ATF_TC_BODY(pollts, tc)
{
	int fds[2];
	struct pollfd pfds[2];
	struct timespec timeout;
	int ret;

	assert(pipe(fds) == 0);

	pfds[0].fd = fds[0];
	pfds[0].events = POLLIN;
	pfds[1].fd = fds[1];
	pfds[1].events = POLLOUT;

	/* Use a timeout of 1 second. */
	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;

	/*
	 * Check that we get a timeout waiting for data on the read end
	 * of our pipe.
	 */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = pollts(&pfds[0], 1, &timeout, NULL), 0,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, 0, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, -1, "got: %d", pfds[1].revents);

	/* Check that the write end of the pipe as reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = pollts(&pfds[1], 1, &timeout, NULL), 1,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, -1, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",\
	    pfds[1].revents);

	/* Check that only the write end of the pipe as reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = pollts(pfds, 2, &timeout, NULL), 1,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, 0, "got: %d", pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",
	    pfds[1].revents);

	/* Write data to our pipe. */
	assert(write(fds[1], "", 1) == 1);

	/* Check that both ends of our pipe are reported as ready. */
	pfds[0].revents = -1;
	pfds[1].revents = -1;
	ATF_REQUIRE_EQ_MSG(ret = pollts(pfds, 2, &timeout, NULL), 2,
	    "got: %d", ret);
	ATF_REQUIRE_EQ_MSG(pfds[0].revents, POLLIN, "got: %d",
	    pfds[0].revents);
	ATF_REQUIRE_EQ_MSG(pfds[1].revents, POLLOUT, "got: %d",
	    pfds[1].revents);

	assert(close(fds[0]) == 0);
	assert(close(fds[1]) == 0);
}

ATF_TC_BODY(pollts_sigmask, tc)
{
	int fd;
	struct pollfd pfd;
	struct timespec timeout;
	sigset_t mask;
	int ret;

	/* Cf kern/44986 */

	fd = open(_PATH_DEVNULL, O_RDONLY);
	assert(fd >= 0);

	pfd.fd = fd;
	pfd.events = POLLIN;

	/* Use a timeout of 1 second. */
	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;

	/* Unblock all signals. */
	assert(sigfillset(&mask) == 0);
	assert(sigprocmask(SIG_UNBLOCK, &mask, NULL) == 0);

	/* 
	 * Check that pollts(2) immediately returns. We block *all*
	 * signals during pollts(2).
	 */
	ATF_REQUIRE_EQ_MSG(ret = pollts(&pfd, 1, &timeout, &mask), 1,
	    "got: %d", ret);

	/* Check that signals are now longer blocked. */
	assert(sigprocmask(SIG_SETMASK, NULL, &mask) == 0);
	ATF_REQUIRE_MSG(sigismember(&mask, SIGUSR1) == 0,
	    "signal mask was changed.");

	assert(close(fd) == 0);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, pollts);
	ATF_TP_ADD_TC(tp, pollts_sigmask);

	return atf_no_error();
}
