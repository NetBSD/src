/*	$NetBSD: t_getrandom.c,v 1.4 2022/03/18 23:35:37 riastradh Exp $	*/

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
__RCSID("$NetBSD: t_getrandom.c,v 1.4 2022/03/18 23:35:37 riastradh Exp $");

#include <sys/param.h>

#include <sys/random.h>

#include <atf-c.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static uint8_t buf[65536];
static uint8_t zero24[24];

static void
alarm_handler(int signo)
{

	fprintf(stderr, "timeout\n");
}

static void
install_alarm_handler(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = alarm_handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;	/* no SA_RESTART */

	ATF_CHECK_MSG(sigaction(SIGALRM, &sa, NULL) != -1,
	    "sigaction(SIGALRM): %s", strerror(errno));
}

/*
 * Probability of spurious failure is 1/2^192 for each of the memcmps.
 * As long as there are fewer than 2^64 of them, the probability of
 * spurious failure is at most 1/2^128, which is low enough that we
 * don't care about it.
 */

ATF_TC(getrandom_default);
ATF_TC_HEAD(getrandom_default, tc)
{
	atf_tc_set_md_var(tc, "descr", "getrandom(..., 0)");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getrandom_default, tc)
{
	ssize_t n;

	/* default */
	install_alarm_handler();
	alarm(1);
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, 0);
	if (n == -1) {
		ATF_CHECK_EQ(errno, EINTR);
	} else {
		ATF_CHECK(n >= (ssize_t)MIN(256, sizeof buf));
		ATF_CHECK((size_t)n <= sizeof buf);
		ATF_CHECK(memcmp(buf, zero24, 24) != 0);
		ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);
	}
	alarm(0);
}

ATF_TC(getrandom_nonblock);
ATF_TC_HEAD(getrandom_nonblock, tc)
{
	atf_tc_set_md_var(tc, "descr", "getrandom(..., GRND_NONBLOCK)");
}
ATF_TC_BODY(getrandom_nonblock, tc)
{
	ssize_t n;

	/* default, nonblocking */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_NONBLOCK);
	if (n == -1) {
		ATF_CHECK_EQ(errno, EAGAIN);
	} else {
		ATF_CHECK(n >= (ssize_t)MIN(256, sizeof buf));
		ATF_CHECK((size_t)n <= sizeof buf);
		ATF_CHECK(memcmp(buf, zero24, 24) != 0);
		ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);
	}
}

ATF_TC(getrandom_insecure);
ATF_TC_HEAD(getrandom_insecure, tc)
{
	atf_tc_set_md_var(tc, "descr", "getrandom(..., GRND_INSECURE)");
}
ATF_TC_BODY(getrandom_insecure, tc)
{
	ssize_t n;

	/* insecure */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_INSECURE);
	ATF_CHECK(n != -1);
	ATF_CHECK(n >= (ssize_t)MIN(256, sizeof buf));
	ATF_CHECK((size_t)n <= sizeof buf);
	ATF_CHECK(memcmp(buf, zero24, 24) != 0);
	ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);
}

ATF_TC(getrandom_insecure_nonblock);
ATF_TC_HEAD(getrandom_insecure_nonblock, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "getrandom(..., GRND_INSECURE|GRND_NONBLOCK)");
}
ATF_TC_BODY(getrandom_insecure_nonblock, tc)
{
	ssize_t n;

	/* insecure, nonblocking -- same as mere insecure */
	memset(buf, 0, sizeof buf);
	n = getrandom(buf, sizeof buf, GRND_INSECURE|GRND_NONBLOCK);
	ATF_CHECK(n != -1);
	ATF_CHECK(n >= (ssize_t)MIN(256, sizeof buf));
	ATF_CHECK((size_t)n <= sizeof buf);
	ATF_CHECK(memcmp(buf, zero24, 24) != 0);
	ATF_CHECK(memcmp(buf + sizeof buf - 24, zero24, 24) != 0);
}

ATF_TC(getrandom_random);
ATF_TC_HEAD(getrandom_random, tc)
{
	atf_tc_set_md_var(tc, "descr", "getrandom(..., GRND_RANDOM)");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getrandom_random, tc)
{
	ssize_t n;

	/* `random' (hokey) */
	install_alarm_handler();
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
}

ATF_TC(getrandom_random_nonblock);
ATF_TC_HEAD(getrandom_random_nonblock, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "getrandom(..., GRND_RANDOM|GRND_NONBLOCK)");
}
ATF_TC_BODY(getrandom_random_nonblock, tc)
{
	ssize_t n;

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
}

ATF_TC(getrandom_random_insecure);
ATF_TC_HEAD(getrandom_random_insecure, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "getrandom(..., GRND_RANDOM|GRND_INSECURE)");
}
ATF_TC_BODY(getrandom_random_insecure, tc)
{
	ssize_t n;

	/* random and insecure -- nonsensical */
	n = getrandom(buf, sizeof buf, GRND_RANDOM|GRND_INSECURE);
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EINVAL);
}

ATF_TC(getrandom_random_insecure_nonblock);
ATF_TC_HEAD(getrandom_random_insecure_nonblock, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "getrandom(..., GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK)");
}
ATF_TC_BODY(getrandom_random_insecure_nonblock, tc)
{
	ssize_t n;

	/* random and insecure, nonblocking -- nonsensical */
	n = getrandom(buf, sizeof buf,
	    GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK);
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EINVAL);
}

ATF_TC(getrandom_invalid);
ATF_TC_HEAD(getrandom_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr", "getrandom(..., <invalid>)");
}
ATF_TC_BODY(getrandom_invalid, tc)
{
	ssize_t n;

	/* invalid flags */
	__CTASSERT(~(GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK));
	n = getrandom(buf, sizeof buf,
	    ~(GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK));
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EINVAL);
}

ATF_TC(getrandom_fault);
ATF_TC_HEAD(getrandom_fault, tc)
{
	atf_tc_set_md_var(tc, "descr", "getrandom(NULL, ...)");
}
ATF_TC_BODY(getrandom_fault, tc)
{
	ssize_t n;

	/* unmapped */
	n = getrandom(NULL, sizeof buf, GRND_INSECURE|GRND_NONBLOCK);
	ATF_CHECK_EQ(n, -1);
	ATF_CHECK_EQ(errno, EFAULT);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, getrandom_default);
	ATF_TP_ADD_TC(tp, getrandom_nonblock);
	ATF_TP_ADD_TC(tp, getrandom_insecure);
	ATF_TP_ADD_TC(tp, getrandom_insecure_nonblock);
	ATF_TP_ADD_TC(tp, getrandom_random);
	ATF_TP_ADD_TC(tp, getrandom_random_nonblock);
	ATF_TP_ADD_TC(tp, getrandom_random_insecure);
	ATF_TP_ADD_TC(tp, getrandom_random_insecure_nonblock);
	ATF_TP_ADD_TC(tp, getrandom_invalid);
	ATF_TP_ADD_TC(tp, getrandom_fault);

	return atf_no_error();
}
