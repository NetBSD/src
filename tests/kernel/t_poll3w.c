/* $NetBSD: t_poll3w.c,v 1.1.2.2 2009/05/13 19:19:33 jym Exp $ */

/*-
 * Copyright (c) 2002, 2008 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_poll3w.c,v 1.1.2.2 2009/05/13 19:19:33 jym Exp $");

#include <sys/types.h>
#include <sys/wait.h>

#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "../h_macros.h"

int desc;

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

ATF_TC(poll);
ATF_TC_HEAD(poll, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr",
	    "Check for 3-way collision for descriptor. First child comes "
	    "and polls on descriptor, second child comes and polls, first "
	    "child times out and exits, third child comes and polls. When "
	    "the wakeup event happens, the two remaining children should "
	    "both be awaken. (kern/17517)");
}
ATF_TC_BODY(poll, tc)
{
	int pf[2];
	int status, i;
	pid_t pid;

	pipe(pf);
	desc = pf[0];

	RL((pid = fork()));
	if (pid == 0) {
		(void)close(pf[1]);
		child1();
		_exit(0);
		/* NOTREACHED */
	}

	RL((pid = fork()));
	if (pid == 0) {
		(void)close(pf[1]);
		child2();
		_exit(0);
		/* NOTREACHED */
	}

	RL((pid = fork()));
	if (pid == 0) {
		(void)close(pf[1]);
		child3();
		_exit(0);
		/* NOTREACHED */
	}

	(void)sleep(10);

	(void)printf("parent write\n");

	RL(write(pf[1], "konec\n", 6));

	for(i = 0; i < 3; ++i)
		RL(wait(&status));

	(void)printf("parent terminated\n");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, poll);

	return atf_no_error();
}
