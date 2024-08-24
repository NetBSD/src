/* $NetBSD: t_sig.c,v 1.4.2.1 2024/08/24 16:35:46 martin Exp $ */

/*-
 * Copyright (c) 2002, 2008, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn and Jaromir Dolecek.
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
__RCSID("$NetBSD: t_sig.c,v 1.4.2.1 2024/08/24 16:35:46 martin Exp $");

#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

#include "h_macros.h"

#define NSIGNALS 5

ATF_TC(sig);
ATF_TC_HEAD(sig, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks EVFILT_SIGNAL");
}
ATF_TC_BODY(sig, tc)
{
	struct timespec	timeout;
	struct kfilter_mapping km;
	struct kevent event[1];
	char namebuf[32];
	pid_t pid, child;
	int kq, n, num, status;

	pid = getpid();
	(void)printf("my pid: %d\n", pid);

	/* fork a child to send signals */
	RL(child = fork());
	if (child == 0) {
		int i;
		(void)sleep(2);
		for(i = 0; i < NSIGNALS; ++i) {
			(void)kill(pid, SIGUSR1);
			(void)sleep(2);
		}
		_exit(0);
		/* NOTREACHED */
	}

	RL(kq = kqueue());

	(void)strlcpy(namebuf, "EVFILT_SIGNAL", sizeof(namebuf));
	km.name = namebuf;
	RL(ioctl(kq, KFILTER_BYNAME, &km));
	(void)printf("got %d as filter number for `%s'.\n", km.filter, km.name);

	/* ignore the signal to avoid taking it for real */
	REQUIRE_LIBC(signal(SIGUSR1, SIG_IGN), SIG_ERR);

	event[0].ident = SIGUSR1;
	event[0].filter = km.filter;
	event[0].flags = EV_ADD | EV_ENABLE;

	RL(kevent(kq, event, 1, NULL, 0, NULL));

	(void)sleep(1);

	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;

	for (num = 0; num < NSIGNALS; num += n) {
		struct timeval then, now, diff;

		RL(gettimeofday(&then, NULL));
		RL(n = kevent(kq, NULL, 0, event, 1, &timeout));
		RL(gettimeofday(&now, NULL));
		timersub(&now, &then, &diff);

		(void)printf("sig: kevent returned %d in %lld.%06ld\n",
		    n, (long long)diff.tv_sec, (long)diff.tv_usec);

		if (n == 0)
			continue;

		(void)printf("sig: kevent flags: 0x%x, data: %" PRId64 " (# "
		    "times signal posted)\n", event[0].flags, event[0].data);
	}

	(void)waitpid(child, &status, 0);
	(void)printf("sig: finished successfully\n");
}

/*
 * This test case exercises code paths in the kernel that KASSERT()
 * some assumptions about EVFILT_SIGNAL and EVFILT_PROC implementation
 * details.
 */
ATF_TC(sig_and_proc);
ATF_TC_HEAD(sig_and_proc, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Validates implementation detail assumptions about "
	    "EVFILT_SIGNAL and EVFILT_PROC");
}
ATF_TC_BODY(sig_and_proc, tc)
{
	struct kevent events[3];
	pid_t pid;
	int kq;

	pid = fork();
	ATF_REQUIRE(pid != -1);

	if (pid == 0) {
		/*
		 * Child: create a kqueue and attach signal knotes
		 * to curproc->p_klist.
		 */
		kq = kqueue();
		ATF_REQUIRE(kq >= 0);

		ATF_REQUIRE(signal(SIGUSR1, SIG_IGN) != SIG_ERR);
		ATF_REQUIRE(signal(SIGUSR2, SIG_IGN) != SIG_ERR);

		EV_SET(&events[0], SIGUSR1, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
		EV_SET(&events[1], SIGUSR2, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);

		ATF_REQUIRE(kevent(kq, events, 2, &events[2], 1, NULL) == 1);
		ATF_REQUIRE(events[2].filter == EVFILT_SIGNAL);
		ATF_REQUIRE(events[2].ident == SIGUSR1);

		/*
		 * When we exit here, the kernel will close all of
		 * its file descriptors (including our kq), which
		 * will in turn remove the signal notes from
		 * curproc->p_klist.
		 *
		 * Then, later on, the kernel will post a NOTE_EXIT
		 * on our parent's kqueue using the proc note that
		 * our parent attached to (our) curproc->p_klist.
		 * That code path KASSERT()s that the signal knotes
		 * have already been removed.
		 */
		_exit(0);
	}

	/*
	 * Parent: create a kqueue and attach a proc note to
	 * child->p_klist.
	 */
	kq = kqueue();
	ATF_REQUIRE(kq >= 0);

	EV_SET(&events[0], pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);

	ATF_REQUIRE(kevent(kq, events, 1, NULL, 0, NULL) == 0);

	/* Ensure we never see stale data. */
	memset(events, 0, sizeof(events));

	/* Signal child to exit. */
	ATF_REQUIRE(kill(pid, SIGUSR1) == 0);

	ATF_REQUIRE(kevent(kq, NULL, 0, events, 1, NULL) == 1);
	ATF_REQUIRE(events[0].filter == EVFILT_PROC);
	ATF_REQUIRE(events[0].ident == (uintptr_t)pid);
	ATF_REQUIRE(events[0].fflags == NOTE_EXIT);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, sig);
	ATF_TP_ADD_TC(tp, sig_and_proc);

	return atf_no_error();
}
