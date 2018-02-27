/* $NetBSD: t_ucontext.c,v 1.4 2018/02/27 11:15:53 kamil Exp $ */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_ucontext.c,v 1.4 2018/02/27 11:15:53 kamil Exp $");

#include <atf-c.h>
#include <stdio.h>
#include <ucontext.h>

ATF_TC(ucontext_basic);
ATF_TC_HEAD(ucontext_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks {get,set}context(2)");
}

ATF_TC_BODY(ucontext_basic, tc)
{
	ucontext_t u, v, w;
	volatile int x, y;

	x = 0;
	y = 0;

	printf("Start\n");

	getcontext(&u);
	y++;

	printf("x == %d\n", x);

	getcontext(&v);

	if ( x < 20 ) {
		x++;
		getcontext(&w);
		setcontext(&u);
	}

	printf("End, y = %d\n", y);
	ATF_REQUIRE_EQ(y, 21);
}

ATF_TC(ucontext_sp);
ATF_TC_HEAD(ucontext_sp, tc)
{
	atf_tc_set_md_var(tc, "descr", "Retrieve _UC_MACHINE_SP()");
}

ATF_TC_BODY(ucontext_sp, tc)
{
	ucontext_t u;

	getcontext(&u);

	printf("_UC_MACHINE_SP(u)=%" PRIxREGISTER "\n", (register_t)_UC_MACHINE_SP(&u));
}

ATF_TC(ucontext_fp);
ATF_TC_HEAD(ucontext_fp, tc)
{
	atf_tc_set_md_var(tc, "descr", "Retrieve _UC_MACHINE_FP()");
}

ATF_TC_BODY(ucontext_fp, tc)
{
	ucontext_t u;

	getcontext(&u);

	printf("_UC_MACHINE_FP(u)=%" PRIxREGISTER "\n", (register_t)_UC_MACHINE_FP(&u));
}

ATF_TC(ucontext_pc);
ATF_TC_HEAD(ucontext_pc, tc)
{
	atf_tc_set_md_var(tc, "descr", "Retrieve _UC_MACHINE_PC()");
}

ATF_TC_BODY(ucontext_pc, tc)
{
	ucontext_t u;

	getcontext(&u);

	printf("_UC_MACHINE_PC(u)=%" PRIxREGISTER "\n", (register_t)_UC_MACHINE_PC(&u));
}

ATF_TC(ucontext_intrv);
ATF_TC_HEAD(ucontext_intrv, tc)
{
	atf_tc_set_md_var(tc, "descr", "Retrieve _UC_MACHINE_INTRV()");
}

ATF_TC_BODY(ucontext_intrv, tc)
{
	ucontext_t u;

	getcontext(&u);

	printf("_UC_MACHINE_INTRV(u)=%" PRIxREGISTER "\n", (register_t)_UC_MACHINE_INTRV(&u));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, ucontext_basic);
	ATF_TP_ADD_TC(tp, ucontext_sp);
	ATF_TP_ADD_TC(tp, ucontext_fp);
	ATF_TP_ADD_TC(tp, ucontext_pc);
	ATF_TP_ADD_TC(tp, ucontext_intrv);

	return atf_no_error();
}
