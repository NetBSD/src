/*	$NetBSD: t_epoll.c,v 1.1 2023/07/28 18:19:01 christos Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Theodore Preduta.
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
__RCSID("$NetBSD: t_epoll.c,v 1.1 2023/07/28 18:19:01 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <errno.h>

#include <atf-c.h>

#include "h_macros.h"

ATF_TC(create_size);
ATF_TC_HEAD(create_size, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll_create requires a non-positive size");
}
ATF_TC_BODY(create_size, tc)
{
	ATF_REQUIRE_EQ_MSG(epoll_create(-1), -1,
	    "epoll_create succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EINVAL, true);

	ATF_REQUIRE_EQ_MSG(epoll_create(0), -1,
	    "epoll_create succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EINVAL, true);

	RL(epoll_create(1));
}

ATF_TC(bad_epfd);
ATF_TC_HEAD(bad_epfd, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll_ctl detects an invalid epfd");
}
ATF_TC_BODY(bad_epfd, tc)
{
	int fd;
	struct epoll_event event;

	RL(fd = epoll_create1(0));
	event.events = EPOLLIN;

	ATF_REQUIRE_EQ_MSG(epoll_ctl(-1, EPOLL_CTL_ADD, fd, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EBADF, true);
}

ATF_TC(bad_fd);
ATF_TC_HEAD(bad_fd, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll_ctl detects an invalid fd");
}
ATF_TC_BODY(bad_fd, tc)
{
	int epfd;
	struct epoll_event event;

	RL(epfd = epoll_create1(0));
	event.events = EPOLLIN;

	ATF_REQUIRE_EQ_MSG(epoll_ctl(epfd, EPOLL_CTL_ADD, -1, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EBADF, true);
}

ATF_TC(double_add);
ATF_TC_HEAD(double_add, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll_ctl detects if a fd has already been added");
}
ATF_TC_BODY(double_add, tc)
{
	int epfd, fd;
	struct epoll_event event;

	RL(epfd = epoll_create1(0));
	RL(fd = epoll_create1(0));
	event.events = EPOLLIN;

	RL(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event));

	ATF_REQUIRE_EQ_MSG(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EEXIST, true);
}

ATF_TC(not_added);
ATF_TC_HEAD(not_added, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll_ctl detects if a fd has not been added");
}
ATF_TC_BODY(not_added, tc)
{
	int epfd, fd;
	struct epoll_event event;

	RL(epfd = epoll_create1(0));
	RL(fd = epoll_create1(0));
	event.events = EPOLLIN;

	ATF_REQUIRE_EQ_MSG(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(ENOENT, true);

	ATF_REQUIRE_EQ_MSG(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(ENOENT, true);
}

ATF_TC(watching_self);
ATF_TC_HEAD(watching_self, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll disallows watching itself");
}
ATF_TC_BODY(watching_self, tc)
{
	int epfd;
	struct epoll_event event;

	RL(epfd = epoll_create1(0));
	ATF_REQUIRE_EQ_MSG(epoll_ctl(epfd, EPOLL_CTL_ADD, epfd, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EINVAL, true);
}

ATF_TC(watch_loops);
ATF_TC_HEAD(watch_loops, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks that epoll disallows loops");
}
ATF_TC_BODY(watch_loops, tc)
{
        int epfd1, epfd2;
	struct epoll_event event;

	event.events = EPOLLIN;
	RL(epfd1 = epoll_create1(0));
	RL(epfd2 = epoll_create1(0));
	RL(epoll_ctl(epfd1, EPOLL_CTL_ADD, epfd2, &event));
	ATF_REQUIRE_EQ_MSG(epoll_ctl(epfd2, EPOLL_CTL_ADD, epfd1, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(ELOOP, true);
}

ATF_TC(watch_depth);
ATF_TC_HEAD(watch_depth, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that epoll fails when the watch depth exceeds 5");
}
ATF_TC_BODY(watch_depth, tc)
{
	int epfd, tmp;
	struct epoll_event event;

	event.events = EPOLLIN;
	RL(epfd = epoll_create1(0));
	for (size_t i = 0; i < 4; i++) {
		RL(tmp = epoll_create1(0));
		RL(epoll_ctl(tmp, EPOLL_CTL_ADD, epfd, &event));
		epfd = tmp;
	}
	RL(tmp = epoll_create1(0));
	ATF_REQUIRE_EQ_MSG(epoll_ctl(tmp, EPOLL_CTL_ADD, epfd, &event), -1,
	    "epoll_ctl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EINVAL, true);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, create_size);
	ATF_TP_ADD_TC(tp, bad_epfd);
	ATF_TP_ADD_TC(tp, bad_fd);
	ATF_TP_ADD_TC(tp, not_added);
	ATF_TP_ADD_TC(tp, watching_self);
	ATF_TP_ADD_TC(tp, watch_loops);
	ATF_TP_ADD_TC(tp, watch_depth);

	return atf_no_error();
}
