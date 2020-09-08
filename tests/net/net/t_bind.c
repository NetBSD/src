/*	$NetBSD: t_bind.c,v 1.1 2020/09/08 14:13:50 christos Exp $	*/

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
__RCSID("$Id: t_bind.c,v 1.1 2020/09/08 14:13:50 christos Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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

static int
mksocket(int sfam)
{
	int o, p;
	switch (sfam) {
	case AF_INET:
		o = IP_BINDANY;
		p = IPPROTO_IP;
		break;
	case AF_INET6:
		o = IPV6_BINDANY;
		p = IPPROTO_IPV6;
		break;
	default:
		FAIL("family");
	}
	int fd = socket(sfam, SOCK_STREAM, 0);
	if (fd == -1)
		FAIL("socket");
	int f = 1;
	if (setsockopt(fd, p, o, &f, sizeof(f)) == -1) {
		close(fd);
		FAIL("setsockopt");
	}
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
		inet_pton(sfam, "1.1.1.1", &sin->sin_addr);
		return sizeof(*sin);
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)ss;
		sin6->sin6_port = htons(12345);
#ifdef BSD4_4
		sin6->sin6_len = sizeof(*sin6);
#endif
		inet_pton(sfam, "2010:2020:2030:2040:2050:2060:2070:2080",
		    &sin6->sin6_addr);
		return sizeof(*sin6);
	default:
		FAIL("bad family");
	}
fail:
	return -1;
}

static int
test(int sfam)
{
	int sfd = -1;
	struct sockaddr_storage saddr;
	socklen_t slen;

	sfd = mksocket(sfam);
	slen = mkserver(sfam, &saddr);

	if (bind(sfd, (struct sockaddr *)&saddr, slen) == -1) {
		FAIL("bind");
	}
	return 0;
fail:
	return -1;
}

#ifndef TEST

ATF_TC(bindany_4);
ATF_TC_HEAD(bindany_4, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check AF_INET bindany");
}

ATF_TC_BODY(bindany_4, tc)
{
	test(AF_INET);
}

ATF_TC(bindany_6);
ATF_TC_HEAD(bindany_6, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check AF_INET6 bindany");
}

ATF_TC_BODY(bindany_6, tc)
{
	test(AF_INET6);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, bindany_4);
	ATF_TP_ADD_TC(tp, bindany_6);
	return atf_no_error();
}
#else
int
main(int argc, char *argv[])
{
	test(AF_INET);
	test(AF_INET6);
}
#endif
