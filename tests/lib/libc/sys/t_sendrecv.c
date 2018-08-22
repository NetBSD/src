/*	$NetBSD: t_sendrecv.c,v 1.4 2018/08/22 06:31:37 christos Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: t_sendrecv.c,v 1.4 2018/08/22 06:31:37 christos Exp $");

#include <atf-c.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>

ATF_TC(sendrecv_basic);
ATF_TC_HEAD(sendrecv_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of sendrecv(2)");
}

#define COUNT 100

union packet {
	uint8_t buf[1316];
	uintmax_t seq;
};

static volatile sig_atomic_t rdied;

static void
handle_sigchld(__unused int pid)
{

	rdied = 1;
}

static void
sender(int fd)
{
	union packet p;
	ssize_t n;
	p.seq = 0;
	for (size_t i = 0; i < COUNT; i++) {
		for (; (n = send(fd, &p, sizeof(p), 0)) == sizeof(p);
		    p.seq++)
			continue;
		printf(">>%zd %d %ju\n", n, errno, p.seq);
		ATF_REQUIRE_MSG(errno == ENOBUFS, "send %s", strerror(errno));
		sched_yield();
	}
	printf("sender done\n");
}

static void
receiver(int fd)
{
	union packet p;
	ssize_t n;
	uintmax_t seq = 0;

	do {
		if (rdied)
			return;
		while ((n = recv(fd, &p, sizeof(p), 0), sizeof(p))
		    == sizeof(p))
		{
			if (rdied)
				return;
			if (p.seq != seq)
				printf("%ju != %ju\n", p.seq, seq);
			seq = p.seq + 1;
		}
		printf("<<%zd %d %ju\n", n, errno, seq);
		if (n == 0)
			return;
		ATF_REQUIRE_EQ(n, -1);
		ATF_REQUIRE_MSG(errno == ENOBUFS, "recv %s", strerror(errno));
	} while (p.seq < COUNT);
}

ATF_TC_BODY(sendrecv_basic, tc)
{
	int fd[2], error;
	struct sigaction sa;

//	atf_tc_fail("does not terminate");

	error = socketpair(AF_UNIX, SOCK_DGRAM, 0, fd);
//	error = pipe(fd);
	ATF_REQUIRE_MSG(error != -1, "socketpair failed (%s)", strerror(errno));

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = 0;
	sa.sa_handler = &handle_sigchld;
	sigemptyset(&sa.sa_mask);
	error = sigaction(SIGCHLD, &sa, 0);
	ATF_REQUIRE_MSG(error != -1, "sigaction failed (%s)",
	    strerror(errno));

	switch (fork()) {
	case -1:
		ATF_REQUIRE_MSG(errno == 0,
		    "socketpair failed (%s)", strerror(errno));
		/*NOTREACHED*/
	case 0:
		sched_yield();
		sender(fd[0]);
		close(fd[0]);
		exit(EXIT_SUCCESS);
		/*NOTREACHED*/
	default:
		receiver(fd[1]);
		return;
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, sendrecv_basic);

	return atf_no_error();
}
