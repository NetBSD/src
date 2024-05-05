/*	$NetBSD: t_next.c,v 1.2 2024/05/05 14:29:38 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_next.c,v 1.2 2024/05/05 14:29:38 riastradh Exp $");

#include <atf-c.h>
#include <float.h>
#include <math.h>

#ifdef __vax__		/* XXX PR 57881: vax libm is missing various symbols */

ATF_TC(vaxafter)
ATF_TC_HEAD(vaxafter, tc)
{

	atf_expect_fail("PR 57881: vax libm is missing various symbols")
	atf_tc_fail("missing nextafter{,f,l} and nexttoward{,f,l} on vax");
}

#else  /* !__vax__ */

#define	CHECK(x, y)							      \
	ATF_CHECK_EQ_MSG((x), (y), #x"=%La=%Lg, "#y"=%La=%Lg",		      \
	    (long double)(x), (long double)(x),				      \
	    (long double)(y), (long double)(y))

ATF_TC(nextafter);
ATF_TC_HEAD(nextafter, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafter");
}
ATF_TC_BODY(nextafter, tc)
{

	CHECK(nextafter(1, 2), 1 + DBL_EPSILON);
	CHECK(nextafter(1 + DBL_EPSILON, 0), 1);
	CHECK(nextafter(1, 0), 1 - DBL_EPSILON/2);
	CHECK(nextafter(1 - DBL_EPSILON/2, 2), 1);
	CHECK(nextafter(-1, -2), -1 - DBL_EPSILON);
	CHECK(nextafter(-1 - DBL_EPSILON, 0), -1);
	CHECK(nextafter(-1, 0), -1 + DBL_EPSILON/2);
	CHECK(nextafter(-1 + DBL_EPSILON/2, -2), -1);
}

ATF_TC(nextafterf);
ATF_TC_HEAD(nextafterf, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterf");
}
ATF_TC_BODY(nextafterf, tc)
{

	CHECK(nextafterf(1, 2), 1 + FLT_EPSILON);
	CHECK(nextafterf(1 + FLT_EPSILON, 0), 1);
	CHECK(nextafterf(1, 0), 1 - FLT_EPSILON/2);
	CHECK(nextafterf(1 - FLT_EPSILON/2, 2), 1);
	CHECK(nextafterf(-1, -2), -1 - FLT_EPSILON);
	CHECK(nextafterf(-1 - FLT_EPSILON, 0), -1);
	CHECK(nextafterf(-1, 0), -1 + FLT_EPSILON/2);
	CHECK(nextafterf(-1 + FLT_EPSILON/2, -2), -1);
}

ATF_TC(nextafterl);
ATF_TC_HEAD(nextafterl, tc)
{
	atf_tc_set_md_var(tc, "descr", "nextafterl");
}
ATF_TC_BODY(nextafterl, tc)
{

	CHECK(nextafterl(1, 2), 1 + LDBL_EPSILON);
	CHECK(nextafterl(1 + LDBL_EPSILON, 0), 1);
	CHECK(nextafterl(1, 0), 1 - LDBL_EPSILON/2);
	CHECK(nextafterl(1 - LDBL_EPSILON/2, 2), 1);
	CHECK(nextafterl(-1, -2), -1 - LDBL_EPSILON);
	CHECK(nextafterl(-1 - LDBL_EPSILON, 0), -1);
	CHECK(nextafterl(-1, 0), -1 + LDBL_EPSILON/2);
	CHECK(nextafterl(-1 + LDBL_EPSILON/2, -2), -1);
}

ATF_TC(nexttoward);
ATF_TC_HEAD(nexttoward, tc)
{
	atf_tc_set_md_var(tc, "descr", "nexttoward");
}
ATF_TC_BODY(nexttoward, tc)
{

	CHECK(nexttoward(1, 2), 1 + DBL_EPSILON);
	CHECK(nexttoward(1 + DBL_EPSILON, 0), 1);
	CHECK(nexttoward(1, 0), 1 - DBL_EPSILON/2);
	CHECK(nexttoward(1 - DBL_EPSILON/2, 2), 1);
	CHECK(nexttoward(-1, -2), -1 - DBL_EPSILON);
	CHECK(nexttoward(-1 - DBL_EPSILON, 0), -1);
	CHECK(nexttoward(-1, 0), -1 + DBL_EPSILON/2);
	CHECK(nexttoward(-1 + DBL_EPSILON/2, -2), -1);
}

ATF_TC(nexttowardf);
ATF_TC_HEAD(nexttowardf, tc)
{
	atf_tc_set_md_var(tc, "descr", "nexttowardf");
}
ATF_TC_BODY(nexttowardf, tc)
{

	CHECK(nexttowardf(1, 2), 1 + FLT_EPSILON);
	CHECK(nexttowardf(1 + FLT_EPSILON, 0), 1);
	CHECK(nexttowardf(1, 0), 1 - FLT_EPSILON/2);
	CHECK(nexttowardf(1 - FLT_EPSILON/2, 2), 1);
	CHECK(nexttowardf(-1, -2), -1 - FLT_EPSILON);
	CHECK(nexttowardf(-1 - FLT_EPSILON, 0), -1);
	CHECK(nexttowardf(-1, 0), -1 + FLT_EPSILON/2);
	CHECK(nexttowardf(-1 + FLT_EPSILON/2, -2), -1);
}

ATF_TC(nexttowardl);
ATF_TC_HEAD(nexttowardl, tc)
{
	atf_tc_set_md_var(tc, "descr", "nexttowardl");
}
ATF_TC_BODY(nexttowardl, tc)
{

	CHECK(nexttowardl(1, 2), 1 + LDBL_EPSILON);
	CHECK(nexttowardl(1 + LDBL_EPSILON, 0), 1);
	CHECK(nexttowardl(1, 0), 1 - LDBL_EPSILON/2);
	CHECK(nexttowardl(1 - LDBL_EPSILON/2, 2), 1);
	CHECK(nexttowardl(-1, -2), -1 - LDBL_EPSILON);
	CHECK(nexttowardl(-1 - LDBL_EPSILON, 0), -1);
	CHECK(nexttowardl(-1, 0), -1 + LDBL_EPSILON/2);
	CHECK(nexttowardl(-1 + LDBL_EPSILON/2, -2), -1);
}

#endif	/* __vax__ */

ATF_TP_ADD_TCS(tp)
{

#ifdef __vax__
	ATF_TP_ADD_TC(tp, vaxafter);
#else
	ATF_TP_ADD_TC(tp, nextafter);
	ATF_TP_ADD_TC(tp, nextafterf);
	ATF_TP_ADD_TC(tp, nextafterl);
	ATF_TP_ADD_TC(tp, nexttoward);
	ATF_TP_ADD_TC(tp, nexttowardf);
	ATF_TP_ADD_TC(tp, nexttowardl);
#endif
	return atf_no_error();
}
