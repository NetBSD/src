/*	$NetBSD: t_errhandling.c,v 1.3.2.2 2024/10/11 19:01:12 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#define	__TEST_FENV

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_errhandling.c,v 1.3.2.2 2024/10/11 19:01:12 martin Exp $");

#include <atf-c.h>
#include <errno.h>
#include <fenv.h>
#include <math.h>

ATF_TC(log);
ATF_TC_HEAD(log, tc)
{
	atf_tc_set_md_var(tc, "descr", "log of invalid");
}
ATF_TC_BODY(log, tc)
{
	static const struct {
#ifdef __HAVE_FENV
		double x;
		int e;
#define	C(x, e)		{ x, e }
#else
		double x;
#define	C(x, e)		{ x }
#endif
	} cases[] = {
		C(0, FE_DIVBYZERO),
		C(-0., FE_DIVBYZERO),
		C(-1, FE_INVALID),
		C(-HUGE_VAL, FE_INVALID),
	};
	volatile double y;
#ifdef __HAVE_FENV
	int except;
#endif
	unsigned i;

	for (i = 0; i < __arraycount(cases); i++) {
		const volatile double x = cases[i].x;

#ifdef __HAVE_FENV
		feclearexcept(FE_ALL_EXCEPT);
#endif
		errno = 0;
		y = log(x);
		if (math_errhandling & MATH_ERREXCEPT) {
#ifdef __HAVE_FENV
			ATF_CHECK_MSG(((except = fetestexcept(FE_ALL_EXCEPT)) &
				cases[i].e) != 0,
			    "expected=0x%x actual=0x%x", cases[i].e, except);
#else
			atf_tc_fail_nonfatal("MATH_ERREXCEPT but no fenv.h");
#endif
		}
		if (math_errhandling & MATH_ERRNO)
			ATF_CHECK_EQ_MSG(errno, EDOM, "errno=%d", errno);
	}

	__USE(y);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, log);

	return atf_no_error();
}
