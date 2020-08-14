/*	$NetBSD: t_getrandom.c,v 1.1 2020/08/14 00:53:16 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__RCSID("$NetBSD: t_getrandom.c,v 1.1 2020/08/14 00:53:16 riastradh Exp $");

#include <sys/random.h>

#include <atf-c.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

static uint8_t buf[65536];
static uint8_t zero24[24];

static void
alarm_handler(int signo)
{
}

ATF_TC(getrandom);
ATF_TC_HEAD(getrandom, tc)
{

	atf_tc_set_md_var(tc, "descr", "getrandom(2)");
}

/*
 * Probability of spurious failure is 1/2^192 for each of the memcmps.
 * As long as there are fewer than 2^64 of them, the probability of
 * spurious failure is at most 1/2^128, which is low enough that we
 * don't care about it.
 */

ATF_TC_BODY(getrandom, tc)
{
	ssize_t n;

	ATF_REQUIRE(signal(SIGALRM, &alarm_handler) != SIG_ERR);

	/* default */
	alarm(1);
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, 0);
	if (n == -1) {
		ATF_CHECK_EQ(errno, EINTR);
	} else {
		ATF_CHECK_EQ((size_t)n, sizeof buf);
		ATF_CHECK(memcmp(buf, zero24, 24) != 0);
		ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);
	}
	alarm(0);

	/* default, nonblocking */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_NONBLOCK);
	if (n == -1) {
		ATF_CHECK_EQ(errno, EAGAIN);
	} else {
		ATF_CHECK_EQ((size_t)n, sizeof buf);
		ATF_CHECK(memcmp(buf, zero24, 24) != 0);
		ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);
	}

	/* insecure */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_INSECURE);
	ATF_CHECK(n != -1);
	ATF_CHECK_EQ((size_t)n, sizeof buf);
	ATF_CHECK(memcmp(buf, zero24, 24) != 0);
	ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);

	/* insecure, nonblocking -- same as mere insecure */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_INSECURE|GRND_NONBLOCK);
	ATF_CHECK(n != -1);
	ATF_CHECK_EQ((size_t)n, sizeof buf);
	ATF_CHECK(memcmp(buf, zero24, 24) != 0);
	ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);

	/* `random' (hokey) */
	alarm(1);
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_RANDOM);
	if (n == -1) {
		ATF_CHECK_EQ(errno, EINTR);
	} else {
		ATF_CHECK(n != 0);
		ATF_CHECK((size_t)n <= sizeof buf);
		if ((size_t)n >= 24) {
			ATF_CHECK(memcmp(buf, zero24, 24) != 0);
			ATF_CHECK(memcmp(buf + n - 24, zero24, 24) != 0);
		}
	}
	alarm(0);

	/* `random' (hokey), nonblocking */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_RANDOM|GRND_NONBLOCK);
	if (n == -1) {
		ATF_CHECK_EQ(errno, EAGAIN);
	} else {
		ATF_CHECK(n != 0);
		ATF_CHECK((size_t)n <= sizeof buf);
		if ((size_t)n >= 24) {
			ATF_CHECK(memcmp(buf, zero24, 24) != 0);
			ATF_CHECK(memcmp(buf + n - 24, zero24, 24) != 0);
		}
	}

	/* random and insecure -- nonsensical */
	n = getrandom(buf, sizeof buf, GRND_RANDOM|GRND_INSECURE);
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EINVAL);

	/* random and insecure, nonblocking -- nonsensical */
	n = getrandom(buf, sizeof buf,
	    GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK);
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EINVAL);

	/* invalid flags */
	__CTASSERT(~(GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK));
	n = getrandom(buf, sizeof buf,
	    ~(GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK));
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EINVAL);

	/* unmapped */
	n = getrandom(NULL, sizeof buf, GRND_INSECURE|GRND_NONBLOCK);
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EFAULT);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, getrandom);

	return atf_no_error();
}
