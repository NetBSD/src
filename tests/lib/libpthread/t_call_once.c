/*	$NetBSD: t_call_once.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kamil Rytarowski.
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
__COPYRIGHT("@(#) Copyright (c) 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_call_once.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <threads.h>

#include <atf-c.h>

ATF_TC(call_once);
ATF_TC_HEAD(call_once, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 call_once(3)");
}

#define CO_THREADS 8

static int co_val;
static once_flag oflag = ONCE_FLAG_INIT;

static void
called_once(void)
{

	++co_val;
}

static int
co_func(void *arg __unused)
{

	call_once(&oflag, called_once);

	return 0;
}

ATF_TC_BODY(call_once, tc)
{
	thrd_t t[CO_THREADS];
	size_t i;

	for (i = 0; i < CO_THREADS; i++) {
		ATF_REQUIRE_EQ(thrd_create(&t[i], co_func, NULL), thrd_success);
	}

	for (i = 0; i < CO_THREADS; i++) {
		ATF_REQUIRE_EQ(thrd_join(t[i], NULL), thrd_success);
	}

	ATF_REQUIRE_EQ(co_val, 1);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, call_once);

	return atf_no_error();
}
