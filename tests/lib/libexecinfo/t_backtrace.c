/*	$NetBSD: t_backtrace.c,v 1.1 2012/05/27 18:47:18 christos Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
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
__RCSID("$NetBSD: t_backtrace.c,v 1.1 2012/05/27 18:47:18 christos Exp $");

#include <atf-c.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>

#ifndef __arraycount
#define __arraycount(a) (sizeof(a) / sizeof(a[0]))
#endif

static void __attribute__((__noinline__))
myfunc3(size_t ncalls)
{
	static const char *top[] = { "myfunc", "atfu_backtrace_fmt_basic_body",
	    "atf_tc_run", "atf_tp_main", "main", "___start" };
	size_t j, nptrs;
	void *buffer[ncalls + 10];
	char **strings;

	nptrs = backtrace(buffer, __arraycount(buffer));
	ATF_CHECK_EQ(nptrs, ncalls + 8);

	strings = backtrace_symbols_fmt(buffer, nptrs, "%n");

	ATF_CHECK(strings != NULL);
	ATF_CHECK_STREQ(strings[0], "myfunc3");
	ATF_CHECK_STREQ(strings[1], "myfunc2");

	for (j = 2; j < ncalls + 2; j++)
		ATF_CHECK_STREQ(strings[j], "myfunc1");

	for (size_t i = 0; j < nptrs; i++, j++)
		ATF_CHECK_STREQ(strings[j], top[i]);

	free(strings);
}

static void __attribute__((__noinline__))
myfunc2(size_t ncalls)
{
	myfunc3(ncalls);
}

static void __attribute__((__noinline__))
myfunc1(size_t origcalls, size_t ncalls)
{
	if (ncalls > 1)
		myfunc1(origcalls, ncalls - 1);
	else
		myfunc2(origcalls);
}

static void __attribute__((__noinline__))
myfunc(size_t ncalls)
{
	myfunc1(ncalls, ncalls);
}

ATF_TC(backtrace_fmt_basic);
ATF_TC_HEAD(backtrace_fmt_basic, tc)
{
        atf_tc_set_md_var(tc, "descr", "Test backtrace_fmt(3)");
}

ATF_TC_BODY(backtrace_fmt_basic, tc)
{
	myfunc(12);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, backtrace_fmt_basic);

	return atf_no_error();
}
