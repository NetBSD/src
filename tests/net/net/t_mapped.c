/*	$NetBSD: t_mapped.c,v 1.1 2020/07/06 18:45:25 christos Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#define _GNU_SOURCE

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$Id: t_mapped.c,v 1.1 2020/07/06 18:45:25 christos Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <stdlib.h>
#include <signal.h>
#include "util.h"

#include "test.h"

static char mymsg[] = "hi mom!";

static void
print(const char *msg, const struct sockaddr *addr)
{
	char buf[1024];

	sockaddr_snprintf(buf, sizeof(buf), "%a:%p", addr);
	printf("%s: %s\n", msg, buf);
}

static int
mksocket(int sfam)
{
	int fd = socket(sfam, SOCK_STREAM, 0);
	if (fd ==  -1)
		FAIL("socket");
	if (sfam != AF_INET6)
		return fd;
	int f = 0;
#if 0
	/* crashes the kernel, should not be allowed kernel only? */
	if (setsockopt(fd, IPPROTO_IPV6, 24 /* IPV6_2292RTHDR */,
	    &f, sizeof(f)) == -1)
		FAIL("setsockopt");
#endif
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &f, sizeof(f)) == -1)
		FAIL("setsockopt");
	return fd;
fail:
	return -1;
}

static socklen_t
mkserver(int sfam, struct sockaddr_storage *ss)
{
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	memset(ss, 0, sizeof(*ss));
	ss->ss_family = sfam;
	switch (sfam) {
	case AF_INET:
		sin = (struct sockaddr_in *)ss;
		sin->sin_port = htons(12345);
#ifdef BSD4_4
		sin->sin_len = sizeof(*sin);
#endif
		return sizeof(*sin);
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)ss;
		sin6->sin6_port = htons(12345);
#ifdef BSD4_4
		sin6->sin6_len = sizeof(*sin6);
#endif
		return sizeof(*sin6);
	default:
		FAIL("bad family");
	}
fail:
	return -1;
}

#if 0
static const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
#endif
static socklen_t
mkclient(int sfam, struct sockaddr_storage *ss)
{
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	memset(ss, 0, sizeof(*ss));
	ss->ss_family = sfam;
	switch (sfam) {
	case AF_INET:
		sin = (struct sockaddr_in *)ss;
		sin->sin_port = htons(12345);
#ifdef BSD4_4
		sin->sin_len = sizeof(*sin);
#endif
		sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		return sizeof(*sin);
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)ss;
		sin6->sin6_port = htons(12345);
#ifdef BSD4_4
		sin6->sin6_len = sizeof(*sin6);
#endif
#if 1
		sin6->sin6_addr = in6addr_loopback;
#else
		struct in6_addr *a = &sin6->sin6_addr;
		a->__u6_addr.__u6_addr32[0] == 0;
		a->__u6_addr.__u6_addr32[1] == 0;
	        a->__u6_addr.__u6_addr32[2] == ntohl(0x0000ffff);
	        a->__u6_addr.__u6_addr32[3] == ntohl(INADDR_LOOPBACK);
#endif
		return sizeof(*sin6);
	default:
		FAIL("bad family");
	}
fail:
	return -1;
}

static int
test(int forkit, int sfam, int cfam)
{
	int sfd = -1, cfd = -1, afd = -1;
	pid_t sfdpid, cfdpid;
	struct sockaddr_storage saddr, caddr, paddr;
	socklen_t slen, clen, plen;

	sfdpid = cfdpid = getpid();

	sfd = mksocket(sfam);
	slen = mkserver(sfam, &saddr);

	if (bind(sfd, (struct sockaddr *)&saddr, slen) == -1) {
		FAIL("bind");
	}

	if (listen(sfd, SOMAXCONN) == -1)
		FAIL("listen");

	if (forkit) {
		switch (cfdpid = fork()) {
		case 0:	/* child */
			sfdpid = getppid();
			cfdpid = getpid();
			break;
		case -1:
			FAIL("fork");
		default:
			break;
		}
	}

	if (cfdpid == getpid()) {
		cfd = mksocket(cfam);
		clen = mkclient(cfam, &caddr);

		if (connect(cfd, (const struct sockaddr *)&caddr, clen) == -1)
			FAIL("connect");
	}

	if (sfdpid == getpid()) {
		plen = sizeof(paddr);
		afd = accept(sfd, (struct sockaddr *)&paddr, &plen);
		if (afd == -1)
			FAIL("accept");

		print("peer", (const struct sockaddr *)&paddr);
	}

	if (cfdpid == getpid()) {
		if (write(cfd, mymsg, sizeof(mymsg)) != sizeof(mymsg))
			FAIL("write");
		(void)close(cfd);
	}

	if (sfdpid == getpid()) {
		char buf[1024];
		if (read(afd, buf, sizeof(mymsg)) != sizeof(mymsg))
			FAIL("write");

		if (strcmp(buf, mymsg) != 0)
			FAIL("compare");

		(void)close(afd);
		(void)close(sfd);
		if (forkit && waitpid(cfdpid, NULL, 0) == -1)
			FAIL("waitpid");
	}
	(void)close(cfd);

	return 0;
fail:
	if (sfdpid == getpid()) {
		(void)close(afd);
		(void)close(sfd);
	}
	if (cfdpid == getpid()) {
		(void)close(cfd);
	}
	return -1;
}

#ifndef TEST

ATF_TC(mapped_4_4);
ATF_TC_HEAD(mapped_4_4, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check AF_INET <- AF_INET connections");
}

ATF_TC_BODY(mapped_4_4, tc)
{
	test(0, AF_INET, AF_INET);
}

ATF_TC(mapped_6_4);
ATF_TC_HEAD(mapped_6_4, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check AF_INET6 <- AF_INET connections");
}

ATF_TC_BODY(mapped_6_4, tc)
{
	test(0, AF_INET6, AF_INET);
}

#if 0
ATF_TC(mapped_4_6);
ATF_TC_HEAD(mapped_4_6, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check AF_INET <- AF_INET6 connections");
}

ATF_TC_BODY(mapped_4_6, tc)
{
	test(0, AF_INET, AF_INET6);
}
#endif

ATF_TC(mapped_6_6);
ATF_TC_HEAD(mapped_6_6, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check AF_INET6 <- AF_INET6 connections");
}

ATF_TC_BODY(mapped_6_6, tc)
{
	test(0, AF_INET6, AF_INET6);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mapped_4_4);
#if 0
	ATF_TP_ADD_TC(tp, mapped_4_6);
#endif
	ATF_TP_ADD_TC(tp, mapped_6_4);
	ATF_TP_ADD_TC(tp, mapped_6_6);
	return atf_no_error();
}
#else
int
main(int argc, char *argv[])
{
	test(0, AF_INET, AF_INET);
//	test(0, AF_INET, AF_INET6);
	test(0, AF_INET6, AF_INET);
	test(0, AF_INET6, AF_INET6);
}
#endif
