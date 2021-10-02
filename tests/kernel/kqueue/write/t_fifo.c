/* $NetBSD: t_fifo.c,v 1.5 2021/10/02 18:39:15 thorpej Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__COPYRIGHT("@(#) Copyright (c) 2021\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_fifo.c,v 1.5 2021/10/02 18:39:15 thorpej Exp $");

#include <sys/event.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <atf-c.h>

static const char	fifo_path[] = "fifo";

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

ATF_TC_WITH_CLEANUP(fifo);
ATF_TC_HEAD(fifo, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks EVFILT_WRITE on fifo");
}
ATF_TC_BODY(fifo, tc)
{
	const struct timespec to = { 0, 0 };
	struct kevent event[1];
	char *buf;
	int rfd, wfd, kq;
	long pipe_buf;

	fifo_support();

	ATF_REQUIRE(mkfifo(fifo_path, 0600) == 0);
	ATF_REQUIRE((rfd = open(fifo_path, O_RDONLY | O_NONBLOCK)) >= 0);
	ATF_REQUIRE((wfd = open(fifo_path, O_WRONLY | O_NONBLOCK)) >= 0);
	ATF_REQUIRE((kq = kqueue()) >= 0);

	/* Get the maximum atomic pipe write size. */
	pipe_buf = fpathconf(wfd, _PC_PIPE_BUF);
	ATF_REQUIRE(pipe_buf > 1);

	buf = malloc(pipe_buf);
	ATF_REQUIRE(buf != NULL);

	EV_SET(&event[0], wfd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, 0);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	/* We expect the FIFO to be writable. */
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &to) == 1);
	ATF_REQUIRE(event[0].ident == (uintptr_t)wfd);
	ATF_REQUIRE(event[0].filter == EVFILT_WRITE);

	/*
	 * Write data into the FIFO until it is full, which is
	 * defined as insufficient buffer space to hold the
	 * maximum atomic pipe write size.
	 */
	while (write(wfd, buf, pipe_buf) != -1) {
		continue;
	}
	ATF_REQUIRE(errno == EAGAIN);

	/* We expect the FIFO to no longer be writable. */
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &to) == 0);

	/* Read a single byte of data from the FIFO. */
	ATF_REQUIRE(read(rfd, buf, 1) == 1);

	/*
	 * Because we have read only a single byte out, there will
	 * be insufficient space for a pipe_buf-sized message, so
	 * the FIFO should still not be writable.
	 */
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &to) == 0);

	/*
	 * Now read enough so that exactly pipe_buf space should be
	 * available.  The FIFO should be writable after that.
	 */
	ATF_REQUIRE(read(rfd, buf, pipe_buf - 1) == pipe_buf - 1);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &to) == 1);
	ATF_REQUIRE(event[0].ident == (uintptr_t)wfd);
	ATF_REQUIRE(event[0].filter == EVFILT_WRITE);

	(void)close(wfd);
	(void)close(rfd);
	(void)close(kq);
}

ATF_TC_CLEANUP(fifo, tc)
{
	(void)unlink(fifo_path);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, fifo);

	return atf_no_error();
}
