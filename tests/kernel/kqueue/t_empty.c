/* $NetBSD: t_empty.c,v 1.1.2.1 2024/08/24 16:53:09 martin Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_empty.c,v 1.1.2.1 2024/08/24 16:53:09 martin Exp $");

#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

static void
test_empty(int readfd, int writefd, bool is_tcp)
{
	struct timespec ts = { 0, 0 };
	struct kevent event;
	int kq, error, sndbufsize;
	char buf[1024] = { 0 };
	ssize_t rv;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event, writefd, EVFILT_EMPTY, EV_ADD, 0, 0, NULL);
	ATF_REQUIRE(kevent(kq, &event, 1, NULL, 0, NULL) == 0);

	/* Check that EMPTY is true. */
	memset(&event, 0, sizeof(event));
	ATF_REQUIRE(kevent(kq, NULL, 0, &event, 1, &ts) == 1);
	ATF_REQUIRE(event.ident == (uintptr_t)writefd);
	ATF_REQUIRE(event.filter == EVFILT_EMPTY);

	if (is_tcp) {
		/*
		 * Get the write socket buffer size so that we can set
		 * the read socket buffer size to something larger
		 * later on.
		 */
		socklen_t slen = sizeof(sndbufsize);
		ATF_REQUIRE(getsockopt(writefd, SOL_SOCKET,
		    SO_SNDBUF, &sndbufsize, &slen) == 0);

		/*
		 * Set the receive buffer size to 1, slamming shut
		 * the TCP receive window, thus trapping all of the
		 * data in the sender's queue.
		 */
		int val = 1;
		ATF_REQUIRE(setsockopt(readfd, SOL_SOCKET,
		    SO_RCVBUF, &val, sizeof(val)) == 0);
	}

	/* Write until the write buffer is full. */
	for (rv = 0; rv != -1;) {
		rv = write(writefd, buf, sizeof(buf));
		error = errno;
		ATF_REQUIRE(rv > 0 || (rv == -1 && error == EAGAIN));
	}

	/* Check that EMPTY is false. */
	ATF_REQUIRE(kevent(kq, NULL, 0, &event, 1, &ts) == 0);

	if (is_tcp) {
		/*
		 * Set the receive buffer size to something larger than
		 * the sender's send buffer.
		 */
		int val = sndbufsize + 128;
		ATF_REQUIRE(setsockopt(readfd, SOL_SOCKET,
		    SO_RCVBUF, &val, sizeof(val)) == 0);
	}

	/* Read all of the data that's available. */
	for (rv = 0; rv != -1;) {
		rv = read(readfd, buf, sizeof(buf));
		error = errno;
		ATF_REQUIRE(rv > 0 || (rv == -1 && error == EAGAIN));
	}

	/*
	 * Check that EMPTY is true.  Check a few times (TCP might
	 * not drain immediately).
	 */
	if (is_tcp) {
		for (rv = 0; rv < 5; rv++) {
			if (kevent(kq, NULL, 0, &event, 1, &ts) == 1) {
				break;
			}
		}
		sleep(1);
	}
	memset(&event, 0, sizeof(event));
	ATF_REQUIRE(kevent(kq, NULL, 0, &event, 1, &ts) == 1);
	ATF_REQUIRE(event.ident == (uintptr_t)writefd);
	ATF_REQUIRE(event.filter == EVFILT_EMPTY);
}

ATF_TC(sock_tcp);
ATF_TC_HEAD(sock_tcp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test EVFILT_EMPTY with TCP sockets.");
}

ATF_TC_BODY(sock_tcp, tc)
{
	int readsock, writesock;
	socklen_t slen;

	ATF_REQUIRE((readsock =
	    socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP)) != -1);
	ATF_REQUIRE((writesock =
	    socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP)) != -1);

	struct sockaddr_in sin = {
		.sin_len = sizeof(sin),
		.sin_family = AF_INET,
		.sin_port = 0,		/* no need to swap 0 */
		.sin_addr = { .s_addr = htonl(INADDR_LOOPBACK) },
	};
	ATF_REQUIRE(bind(readsock, (struct sockaddr *)&sin,
	    sizeof(sin)) == 0);
	ATF_REQUIRE(listen(readsock, 1) == 0);
	slen = sizeof(sin);
	ATF_REQUIRE(getsockname(readsock, (struct sockaddr *)&sin, &slen) == 0);

	ATF_REQUIRE_ERRNO(EINPROGRESS,
	    connect(writesock, (struct sockaddr *)&sin, sizeof(sin)) == -1);

	/* XXX Avoid race between connect(2) and accept(2). */
	sleep(1);

	slen = sizeof(sin);
	ATF_REQUIRE((readsock = accept(readsock, (struct sockaddr *)&sin,
	    &slen)) != -1);

	test_empty(readsock, writesock, true);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, sock_tcp);

	return atf_no_error();
}
