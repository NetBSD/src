/* $NetBSD: t_oneshot.c,v 1.1 2021/10/23 18:46:26 thorpej Exp $ */

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
__RCSID("$NetBSD: t_oneshot.c,v 1.1 2021/10/23 18:46:26 thorpej Exp $");

#include <sys/types.h>
#include <sys/event.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <atf-c.h>

#define	MY_UDATA	((void *)0xbeefcafe)

ATF_TC(oneshot_udata);
ATF_TC_HEAD(oneshot_udata, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests that udata is round-tripped correctly for ONESHOT events.");
}

ATF_TC_BODY(oneshot_udata, tc)
{
	struct kevent event[1];
	int kq;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 1, MY_UDATA);

	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);
	memset(event, 0, sizeof(event));
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, NULL) == 1);
	ATF_REQUIRE(event[0].ident == 1);
	ATF_REQUIRE(event[0].data == 1);
	ATF_REQUIRE(event[0].udata == MY_UDATA);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, oneshot_udata);

	return atf_no_error();
}
