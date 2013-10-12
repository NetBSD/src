/*	$NetBSD: t_tcp.c,v 1.1 2013/10/12 15:29:16 christos Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#ifdef __RCSID
__RCSID("$Id: t_tcp.c,v 1.1 2013/10/12 15:29:16 christos Exp $");
#endif

/* Example code. Should block; does with accept not paccept. */
/* Original by: Justin Cormack <justin@specialbusrvrervice.com> */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <stdlib.h>
#include <signal.h>

#ifdef TEST
#define FAIL(msg, ...)  err(EXIT_FAILURE, msg, ## __VA_ARGS__)
#else 
#include <atf-c.h> 
#define FAIL(msg, ...)  ATF_CHECK_MSG(0, msg, ## __VA_ARGS__)
#endif

static void
ding(int al)
{
}

static void 
paccept_reset_nonblock(void)
{
	int srvr, clnt, as;
	int ok, fl, n;
	char buf[10];
	struct sockaddr_in sin, ba;
	struct sigaction sa;

	srvr = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (srvr == -1)
		FAIL("socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
#ifdef BSD4_4
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_port = htons(0);
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ok = bind(srvr, (const struct sockaddr *)&sin, (socklen_t)sizeof(sin));
	if (ok == -1)
		FAIL("bind");

	socklen_t addrlen = sizeof(struct sockaddr_in);
	ok = getsockname(srvr, (struct sockaddr *)&ba, &addrlen);
	if (ok == -1)
		FAIL("getsockname");

	ok = listen(srvr, SOMAXCONN);
	if (ok == -1)
		FAIL("listen");

	clnt = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (clnt == -1)
		FAIL("socket");

	/* may not connect first time */
	ok = connect(clnt, (struct sockaddr *) &ba, addrlen);
	as = paccept(srvr, NULL, NULL, NULL, 0);
	ok = connect(clnt, (struct sockaddr *) &ba, addrlen);
	if (ok == -1 && errno != EISCONN)
		FAIL("both connects failed");

	fl = fcntl(srvr, F_GETFL, 0);
	if (fl == -1)
		FAIL("fnctl getfl");

	ok = fcntl(srvr, F_SETFL, fl & ~O_NONBLOCK);
	if (ok == -1)
		FAIL("fnctl setfl");

	if (as == -1) {		/* not true under NetBSD */
		as = paccept(srvr, NULL, NULL, NULL, 0);
		if (as == -1)
			FAIL("paccept");
	}
#ifdef notyet
	/*
	 * this has no effect, not possible to force blocking, same issue if
	 * removed. This is because fcntl does not work on sockets, fixme and
	 * add test.
	 */
	fl = fcntl(as, F_GETFL, 0);
	if (fl == -1)
		FAIL("fnctl");
	ok = fcntl(as, F_SETFL, fl & ~O_NONBLOCK);
	if (ok == -1)
		FAIL("fnctl setfl");

	fl = fcntl(as, F_GETFL, 0);
	if (fl & O_NONBLOCK) {
		printf("accepted sock nonblocking, something wrong\n");
	}
#endif
	sa.sa_handler = ding;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	alarm(1);
	n = read(as, buf, 10);
	if (n == -1 && errno != EINTR)
		FAIL("read");
}

#ifndef TEST

ATF_TC(paccept_reset_nonblock);
ATF_TC_HEAD(paccept_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that paccept(2) resets "
	    "the non-blocking flag on non-blocking sockets");
}

ATF_TC_BODY(paccept_reset_nonblock, tc)
{
	paccept_reset_nonblock();
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, paccept_reset_nonblock);
	return atf_no_error();
}
#else
int
main(int argc, char *argv[])
{
	paccept_reset_nonblock();
	return 0;
}
#endif
