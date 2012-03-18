/*	$NetBSD: t_kevent.c,v 1.2 2012/03/18 07:00:52 jruoho Exp $ */

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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_kevent.c,v 1.2 2012/03/18 07:00:52 jruoho Exp $");

#include <sys/types.h>
#include <sys/event.h>

#include <atf-c.h>
#include <time.h>
#include <stdio.h>

ATF_TC(kevent_zerotimer);
ATF_TC_HEAD(kevent_zerotimer, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that kevent with a 0 timer "
	    "does not crash the system (PR lib/45618)");
}

ATF_TC_BODY(kevent_zerotimer, tc)
{
	struct kevent ev;
	int kq;

	ATF_REQUIRE((kq = kqueue()) != -1);
	EV_SET(&ev, 1, EVFILT_TIMER, EV_ADD|EV_ENABLE, 0, 1, 0);
	ATF_REQUIRE(kevent(kq, &ev, 1, NULL, 0, NULL) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, &ev, 1, NULL) == 1);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, kevent_zerotimer);

	return atf_no_error();
}
