/* $NetBSD: t_scan.c,v 1.2 2021/10/10 19:17:31 wiz Exp $ */

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
__RCSID("$NetBSD: t_scan.c,v 1.2 2021/10/10 19:17:31 wiz Exp $");

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

/*
 * Each kevent thread will make this many kevent() calls, and if it
 * achieves this mark, we assume the race condition has not occurred
 * the declare the test passes.
 */
#define	NKEVENT_CALLS		10000

static int kq;

static void *
kevent_thread(void *arg)
{
	struct timespec ts = { 0, 0 };
	struct kevent event;
	int rv, i;

	for (i = 0; i < NKEVENT_CALLS; i++) {
		rv = kevent(kq, NULL, 0, &event, 1, &ts);
		if (rv < 0) {
			i = rv;
			break;
		}
		if (rv != 1) {
			break;
		}
	}

	return (void *)(intptr_t)i;
}

ATF_TC(scan1);
ATF_TC_HEAD(scan1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Exercises multi-threaded kevent() calls (PR kern/50094)");
}

ATF_TC_BODY(scan1, tc)
{
	pthread_t t1, t2;
	void *v1, *v2;
	intptr_t r1, r2;
	struct kevent event;
	int p[2];

	ATF_REQUIRE(pipe(p) == 0);
	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event, p[0], EVFILT_READ, EV_ADD, 0, 0, 0);
	ATF_REQUIRE(kevent(kq, &event, 1, NULL, 0, NULL) == 0);
	ATF_REQUIRE(write(p[1], p, 1) == 1);

	ATF_REQUIRE(pthread_create(&t1, NULL, kevent_thread, NULL) == 0);
	ATF_REQUIRE(pthread_create(&t2, NULL, kevent_thread, NULL) == 0);

	ATF_REQUIRE(pthread_join(t1, &v1) == 0);
	r1 = (intptr_t)v1;

	ATF_REQUIRE(pthread_join(t2, &v2) == 0);
	r2 = (intptr_t)v2;

	ATF_REQUIRE(r1 >= 0);
	ATF_REQUIRE(r2 >= 0);

	ATF_REQUIRE(r1 == NKEVENT_CALLS);
	ATF_REQUIRE(r2 == NKEVENT_CALLS);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, scan1);

	return atf_no_error();
}
