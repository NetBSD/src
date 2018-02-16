/*	$NetBSD: t_tcp.c,v 1.9 2018/02/16 19:24:16 christos Exp $	*/

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
__RCSID("$Id: t_tcp.c,v 1.9 2018/02/16 19:24:16 christos Exp $");
#endif

/* Example code. Should block; does with accept not paccept. */
/* Original by: Justin Cormack <justin@specialbusrvrervice.com> */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sched.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <stdlib.h>
#include <signal.h>


#include "test.h"

#ifdef __linux__
#define paccept(a, b, c, d, e) accept4((a), (b), (c), (e))
#endif

static void
ding(int al)
{
}

static void 
paccept_block(sa_family_t sfamily, sa_family_t cfamily,
    bool pacceptblock, bool fcntlblock)
{
	int srvr = -1, clnt = -1, as = -1;
	int ok, fl;
	int count = 5;
	ssize_t n;
	char buf[10];
	struct sockaddr_storage ss, bs;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	struct sigaction sa;
	socklen_t slen;
	uid_t euid;
	gid_t egid;

	srvr = socket(sfamily, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (srvr == -1)
		FAIL("socket");

	memset(&ss, 0, sizeof(ss));
	switch (ss.ss_family = sfamily) {
	case AF_INET:
		sin = (void *)&ss;
		slen = sizeof(*sin);
		sin->sin_port = htons(0);
		sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		break;
	case AF_INET6:
		sin6 = (void *)&ss;
		slen = sizeof(*sin6);
		sin6->sin6_port = htons(0);
		if (sfamily == AF_INET6 && cfamily == AF_INET) {
			sin6->sin6_addr = in6addr_any;
		} else {
			sin6->sin6_addr = in6addr_loopback;
		}
		break;
	default:
		errno = EINVAL;
		FAIL("bad family");
	}
#ifdef BSD4_4
	ss.ss_len = slen;
#endif
	if (sfamily == AF_INET6 && cfamily == AF_INET) {
		int off = 0;
		if (setsockopt(srvr, IPPROTO_IPV6, IPV6_V6ONLY,
		    (void *)&off, sizeof(off)) == -1)
			FAIL("setsockopt IPV6_V6ONLY");
	}

	ok = bind(srvr, (const struct sockaddr *)&ss, slen);
	if (ok == -1)
		FAIL("bind");

	socklen_t addrlen = slen;
	ok = getsockname(srvr, (struct sockaddr *)&bs, &addrlen);
	if (ok == -1)
		FAIL("getsockname");

	ok = listen(srvr, SOMAXCONN);
	if (ok == -1)
		FAIL("listen");

	clnt = socket(cfamily, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (clnt == -1)
		FAIL("socket");

	if (sfamily == AF_INET6 && cfamily == AF_INET) {
		ss = bs;
		sin6 = (void *)&ss;
		sin = (void *)&bs;
		addrlen = sizeof(*sin);
#ifdef BSD4_4
		sin->sin_len = sizeof(*sin);
#endif
		sin->sin_family = AF_INET;
		sin->sin_port = sin6->sin6_port;
		sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}

	/* may not connect first time */
	ok = connect(clnt, (struct sockaddr *) &bs, addrlen);
	if (ok != -1 || errno != EINPROGRESS)
		FAIL("expected connect to fail");
	as = paccept(srvr, NULL, NULL, NULL, pacceptblock ? 0 : SOCK_NONBLOCK);
again:
	ok = connect(clnt, (struct sockaddr *) &bs, addrlen);
	if (ok == -1 && errno != EISCONN) {
		if (count-- && errno == EALREADY) {
			fprintf(stderr, "retry\n");
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 1000000;
			nanosleep(&ts, NULL);
#if 0
			sched_yield();
#endif
			goto again;
		}
		FAIL("connect failed");
	}

#if 0
	fl = fcntl(srvr, F_GETFL, 0);
	if (fl == -1)
		FAIL("fnctl getfl");

	ok = fcntl(srvr, F_SETFL, fl & ~O_NONBLOCK);
	if (ok == -1)
		FAIL("fnctl setfl");
#endif

	/* This is supposed to only work on Unix sockets but returns garbage */
	if (getpeereid(clnt, &euid, &egid) != -1)
		FAIL("getpeereid(clnt)");
	/* This is supposed to only work on Unix sockets but returns garbage */
	if (getpeereid(srvr, &euid, &egid) != -1)
		FAIL("getpeereid(srvr)");

	if (as == -1) {		/* not true under NetBSD */
		as = paccept(srvr, NULL, NULL, NULL, pacceptblock ? 0 : SOCK_NONBLOCK);
		if (as == -1)
			FAIL("paccept");
	}
	if (fcntlblock) {
		fl = fcntl(as, F_GETFL, 0);
		if (fl == -1)
			FAIL("fnctl");
		if (fl != (O_RDWR|O_NONBLOCK))
			FAIL("fl 0x%x != 0x%x\n", fl, O_RDWR|O_NONBLOCK);
		ok = fcntl(as, F_SETFL, fl & ~O_NONBLOCK);
		if (ok == -1)
			FAIL("fnctl setfl");

		fl = fcntl(as, F_GETFL, 0);
		if (fl & O_NONBLOCK)
			FAIL("fl non blocking after reset");
	}
	sa.sa_handler = ding;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	alarm(1);
	n = read(as, buf, 10);

	if (pacceptblock || fcntlblock) {
		if (n == -1 && errno != EINTR)
			FAIL("read");
	} else {
		if (n != -1 || errno != EWOULDBLOCK)
			FAIL("read");
	}
	return;
fail:
	close(srvr);
	close(clnt);
	close(as);
}

#ifndef TEST

ATF_TC(paccept44_reset_nonblock);
ATF_TC_HEAD(paccept44_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that paccept(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv4->ipv4)");
}

ATF_TC_BODY(paccept44_reset_nonblock, tc)
{
	paccept_block(AF_INET, AF_INET, true, false);
}

ATF_TC(fcntl44_reset_nonblock);
ATF_TC_HEAD(fcntl44_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that fcntl(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv4->ipv4)");
}

ATF_TC_BODY(fcntl44_reset_nonblock, tc)
{
	paccept_block(AF_INET, AF_INET, false, true);
}

ATF_TC(paccept44_nonblock);
ATF_TC_HEAD(paccept44_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that fcntl(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv4->ipv4)");
}

ATF_TC_BODY(paccept44_nonblock, tc)
{
	paccept_block(AF_INET, AF_INET, false, false);
}

ATF_TC(paccept66_reset_nonblock);
ATF_TC_HEAD(paccept66_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that paccept(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv6->ipv6)");
}

ATF_TC_BODY(paccept66_reset_nonblock, tc)
{
	paccept_block(AF_INET6, AF_INET6, true, false);
}

ATF_TC(fcntl66_reset_nonblock);
ATF_TC_HEAD(fcntl66_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that fcntl(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv6->ipv6)");
}

ATF_TC_BODY(fcntl66_reset_nonblock, tc)
{
	paccept_block(AF_INET6, AF_INET6, false, true);
}

ATF_TC(paccept66_nonblock);
ATF_TC_HEAD(paccept66_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that fcntl(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv6->ipv6)");
}

ATF_TC_BODY(paccept66_nonblock, tc)
{
	paccept_block(AF_INET6, AF_INET6, false, false);
}

ATF_TC(paccept46_reset_nonblock);
ATF_TC_HEAD(paccept46_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that paccept(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv4->ipv6)");
}

ATF_TC_BODY(paccept46_reset_nonblock, tc)
{
	paccept_block(AF_INET6, AF_INET, true, false);
}

ATF_TC(fcntl46_reset_nonblock);
ATF_TC_HEAD(fcntl46_reset_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that fcntl(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv4->ipv6)");
}

ATF_TC_BODY(fcntl46_reset_nonblock, tc)
{
	paccept_block(AF_INET6, AF_INET, false, true);
}

ATF_TC(paccept46_nonblock);
ATF_TC_HEAD(paccept46_nonblock, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that fcntl(2) resets "
	    "the non-blocking flag on non-blocking sockets (ipv4->ipv6)");
}

ATF_TC_BODY(paccept46_nonblock, tc)
{
	paccept_block(AF_INET6, AF_INET, false, false);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, paccept44_reset_nonblock);
	ATF_TP_ADD_TC(tp, fcntl44_reset_nonblock);
	ATF_TP_ADD_TC(tp, paccept44_nonblock);
	ATF_TP_ADD_TC(tp, paccept66_reset_nonblock);
	ATF_TP_ADD_TC(tp, fcntl66_reset_nonblock);
	ATF_TP_ADD_TC(tp, paccept66_nonblock);
	ATF_TP_ADD_TC(tp, paccept46_reset_nonblock);
	ATF_TP_ADD_TC(tp, fcntl46_reset_nonblock);
	ATF_TP_ADD_TC(tp, paccept46_nonblock);
	return atf_no_error();
}
#else
int
main(int argc, char *argv[])
{
	paccept_block(AF_INET, AF_INET, false, false);
	paccept_block(AF_INET, AF_INET, true, false);
	return 0;
}
#endif
