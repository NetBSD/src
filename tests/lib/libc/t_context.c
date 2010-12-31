/* $NetBSD: t_context.c,v 1.1 2010/12/31 14:36:11 pgoyette Exp $ */

/*-
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
__RCSID("$NetBSD: t_context.c,v 1.1 2010/12/31 14:36:11 pgoyette Exp $");

#include <ucontext.h>
#include <stdarg.h>
#include <stdlib.h>

#include <atf-c.h>

#define STACKSZ (10*1024)
#define DEPTH 3

static int calls;

static void
run(int n, ...)
{
	va_list va;
	int i, ia;

	ATF_REQUIRE_EQ(n, DEPTH - calls - 1);

	va_start(va, n);
	for (i = 0; i < 9; i++) {
		ia = va_arg(va, int);
		ATF_REQUIRE_EQ(i, ia);
	}
	va_end(va);

	calls++;
}

ATF_TC(context);

ATF_TC_HEAD(context, tc)
{

	atf_tc_set_md_var(tc, "descr",
	"Checks get/make/setcontext(), context linking via uc_link(), "
	    "and argument passing to the new context");
}

ATF_TC_BODY(context, tc)
{
	ucontext_t uc[DEPTH];
	ucontext_t save;
	volatile int i = 0; /* avoid longjmp clobbering */

	for (i = 0; i < DEPTH; ++i) {
		ATF_REQUIRE_EQ(getcontext(&uc[i]), 0);

		uc[i].uc_stack.ss_sp = malloc(STACKSZ);
		uc[i].uc_stack.ss_size = STACKSZ;
		uc[i].uc_link = (i > 0) ? &uc[i - 1] : &save;

		makecontext(&uc[i], (void *)run, 10, i,
			0, 1, 2, 3, 4, 5, 6, 7, 8);
	}

	ATF_REQUIRE_EQ(getcontext(&save), 0);

	if (calls == 0)
		ATF_REQUIRE_EQ(setcontext(&uc[DEPTH-1]), 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, context);

	return atf_no_error();
}
