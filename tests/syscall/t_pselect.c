/*	$NetBSD: t_pselect.c,v 1.2 2011/05/18 03:15:12 christos Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundatiom
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

#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <atf-c.h>

static sig_atomic_t keep_going = 1;

static void
sig_handler(int signum)
{
	keep_going = 0;
}

static void __attribute__((__noreturn__))
child(void)
{
	struct sigaction sa;
	sigset_t set;
	int fd;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;
	if ((fd = open("/dev/null", O_RDONLY)) == -1)
		err(1, "open");

	if (sigaction(SIGTERM, &sa, NULL) == -1)
		err(1, "sigaction");

	sigfillset(&set);
	if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
		err(1, "procmask");

	sigemptyset(&set);

	for (;;) {
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		if (pselect(1, &rset, NULL, NULL, NULL, &set) == -1) {
			if(errno == EINTR) {
				if (!keep_going)
					exit(0);
			}
		}
       }
}

ATF_TC(pselect_signal_mask);
ATF_TC_HEAD(pselect_signal_mask, tc)
{

	/* Cf. PR lib/43625. */
	atf_tc_set_md_var(tc, "descr",
	    "Checks pselect's temporary mask setting");
}

ATF_TC_BODY(pselect_signal_mask, tc)
{
	pid_t pid;
	int status;

	switch (pid = fork()) {
	case 0:
		child();
	case -1:
		err(1, "fork");
	default:
		usleep(500);
		if (kill(pid, SIGTERM) == -1)
			err(1, "kill");
		usleep(500);
		switch (waitpid(pid, &status, WNOHANG)) {
		case -1:
			err(1, "wait");
		case 0:
			if (kill(pid, SIGKILL) == -1)
				err(1, "kill");
			atf_tc_fail("pselect() did not receive signal");
			break;
		default:
			break;
		}
	}
}


ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, pselect_signal_mask);

	return atf_no_error();
}
