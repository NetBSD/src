/* $NetBSD: t_ilogb.c,v 1.11 2024/05/14 15:31:42 riastradh Exp $ */

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maya Rashish.
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
__RCSID("$NetBSD: t_ilogb.c,v 1.11 2024/05/14 15:31:42 riastradh Exp $");

#include <atf-c.h>
#include <fenv.h>
#include <limits.h>
#include <math.h>

#ifndef __HAVE_FENV

# define ATF_CLEAR_EXCEPTS		__nothing
# define ATF_CHECK_RAISED_INVALID	__nothing
# define ATF_CHECK_RAISED_NOTHING	__nothing

#else  /* __HAVE_FENV */

# define ATF_CLEAR_EXCEPTS		(void)feclearexcept(FE_ALL_EXCEPT);

# define ATF_CHECK_RAISED_INVALID do { \
	int r = fetestexcept(FE_ALL_EXCEPT); \
	ATF_CHECK_MSG((r & FE_INVALID) != 0, \
	    "r & FE_INVALID == 0 (r=%#x, FE_INVALID=%#x)", \
	     r, FE_INVALID); \
	(void)feclearexcept(FE_ALL_EXCEPT); \
} while (/*CONSTCOND*/0)

# define ATF_CHECK_RAISED_NOTHING do { \
	int r = fetestexcept(FE_ALL_EXCEPT); \
	ATF_CHECK_MSG(r == 0, "r=%#x != 0", r); \
	(void)feclearexcept(FE_ALL_EXCEPT); \
} while (/*CONSTCOND*/0)

#endif	/* __HAVE_FENV */

ATF_TC(ilogb);
ATF_TC_HEAD(ilogb, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check ilogb family");
}

ATF_TC_BODY(ilogb, tc)
{

	ATF_CLEAR_EXCEPTS;

	ATF_CHECK(ilogbf(0) == FP_ILOGB0);
	ATF_CHECK_RAISED_INVALID;
	ATF_CHECK(ilogb(0) == FP_ILOGB0);
	ATF_CHECK_RAISED_INVALID;
	ATF_CHECK(ilogbl(0) == FP_ILOGB0);
	ATF_CHECK_RAISED_INVALID;

	ATF_CHECK(ilogbf(-0.) == FP_ILOGB0);
	ATF_CHECK_RAISED_INVALID;
	ATF_CHECK(ilogb(-0.) == FP_ILOGB0);
	ATF_CHECK_RAISED_INVALID;
	ATF_CHECK(ilogbl(-0.) == FP_ILOGB0);
	ATF_CHECK_RAISED_INVALID;

	if (isinf(INFINITY)) {
		ATF_CHECK(ilogbf(INFINITY) == INT_MAX);
		ATF_CHECK_RAISED_INVALID;
		ATF_CHECK(ilogb(INFINITY) == INT_MAX);
		ATF_CHECK_RAISED_INVALID;
		ATF_CHECK(ilogbl(INFINITY) == INT_MAX);
		ATF_CHECK_RAISED_INVALID;

		ATF_CHECK(ilogbf(-INFINITY) == INT_MAX);
		ATF_CHECK_RAISED_INVALID;
		ATF_CHECK(ilogb(-INFINITY) == INT_MAX);
		ATF_CHECK_RAISED_INVALID;
		ATF_CHECK(ilogbl(-INFINITY) == INT_MAX);
		ATF_CHECK_RAISED_INVALID;
	}

	ATF_CHECK(ilogbf(1024) == 10);
	ATF_CHECK_RAISED_NOTHING;
	ATF_CHECK(ilogb(1024) == 10);
	ATF_CHECK_RAISED_NOTHING;
	ATF_CHECK(ilogbl(1024) == 10);
	ATF_CHECK_RAISED_NOTHING;

#ifdef NAN
	ATF_CHECK(ilogbf(NAN) == FP_ILOGBNAN);
	ATF_CHECK_RAISED_INVALID;
	ATF_CHECK(ilogb(NAN) == FP_ILOGBNAN);
	ATF_CHECK_RAISED_INVALID;
	ATF_CHECK(ilogbl(NAN) == FP_ILOGBNAN);
	ATF_CHECK_RAISED_INVALID;
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ilogb);

	return atf_no_error();
}
