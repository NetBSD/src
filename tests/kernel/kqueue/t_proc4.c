/* $NetBSD: t_proc4.c,v 1.1 2021/10/10 17:43:15 thorpej Exp $ */

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
__RCSID("$NetBSD: t_proc4.c,v 1.1 2021/10/10 17:43:15 thorpej Exp $");

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <err.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

/* 10 children each create 10 grandchildren. */
#define	TOTAL_CHILDREN		10
#define	TOTAL_GRANDCHILDREN	(TOTAL_CHILDREN * TOTAL_CHILDREN)
#define	TOTAL_DESCENDENTS	(TOTAL_CHILDREN + TOTAL_GRANDCHILDREN)

#define	EXIT_CHILD		66
#define	EXIT_GRANDCHILD		76

static void
child(void)
{
	int i, status;
	pid_t pid;

	for (i = 0; i < TOTAL_CHILDREN; i++) {
		ATF_REQUIRE((pid = fork()) != -1);
		if (pid == 0) {
			/* grandchild */
			_exit(EXIT_GRANDCHILD);
		}
	}

	for (i = 0; i < TOTAL_CHILDREN; i++) {
		ATF_REQUIRE((pid = wait(&status)) != -1);
	}
	_exit(EXIT_CHILD);
}

ATF_TC(proc4);
ATF_TC_HEAD(proc4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Exercises EVFILT_PROC + NOTE_TRACK");
}

ATF_TC_BODY(proc4, tc)
{
	int total_tracks = 0;
	int child_exits = 0;
	int grandchild_exits = 0;
	pid_t pid;
	int kq, status;
	struct kevent event;
	struct timespec ts = { 0, 0 };
	int i, rv;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event, getpid(), EVFILT_PROC, EV_ADD,
	    NOTE_TRACK | NOTE_EXIT, 0, 0);

	ATF_REQUIRE(kevent(kq, &event, 1, NULL, 0, NULL) == 0);

	for (i = 0; i < TOTAL_CHILDREN; i++) {
		ATF_REQUIRE((pid = fork()) != -1);
		if (pid == 0) {
			child();
			/* NOTREACHED */
		}
	}

	for (;;) {
		if (total_tracks == TOTAL_DESCENDENTS &&
		    child_exits == TOTAL_CHILDREN &&
		    grandchild_exits == TOTAL_GRANDCHILDREN) {
			break;
		}
		memset(&event, 0, sizeof(event));
		rv = kevent(kq, NULL, 0, &event, 1, NULL);
		ATF_REQUIRE(rv != -1);
		ATF_REQUIRE(rv == 1);
		if (event.fflags & NOTE_CHILD) {
			total_tracks++;
			ATF_REQUIRE(total_tracks <= TOTAL_DESCENDENTS);
		} else if (event.fflags & NOTE_EXIT) {
			ATF_REQUIRE(event.flags & EV_EOF);
			ATF_REQUIRE(event.data >= 0);
			ATF_REQUIRE(event.data <= UINT_MAX);
			status = (int)event.data;
			ATF_REQUIRE(WIFEXITED(status));
			ATF_REQUIRE(WEXITSTATUS(status) == EXIT_CHILD ||
				    WEXITSTATUS(status) == EXIT_GRANDCHILD);
			if (WEXITSTATUS(status) == EXIT_CHILD) {
				child_exits++;
				ATF_REQUIRE(child_exits <= TOTAL_CHILDREN);
				ATF_REQUIRE(wait4((pid_t)event.ident,
						  &status,
						  WNOHANG,
						  NULL) != -1);
				ATF_REQUIRE(status == (int)event.data);
			} else {
				grandchild_exits++;
				ATF_REQUIRE(grandchild_exits <=
					    TOTAL_GRANDCHILDREN);
			}
		} else {
			/*
			 * We didn't ask for NOTE_FORK, so we don't
			 * expect to ever see it, even though we are
			 * getting NOTE_CHILD as the result of the
			 * NOTE_TRACK.
			 */
			ATF_REQUIRE((event.fflags & NOTE_FORK) == 0);

			/*
			 * Bomb out of we are getting a TRACKERR.
			 * XXX This could legitimately happen if
			 * the kernel is low on memory because the
			 * code path involved specifically chooses
			 * not to block when allocating memory.
			 */
			ATF_REQUIRE((event.fflags & NOTE_TRACKERR) == 0);
		}
	}

	ATF_REQUIRE(kevent(kq, NULL, 0, &event, 1, &ts) == 0);

	(void)close(kq);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, proc4);

	return atf_no_error();
}
