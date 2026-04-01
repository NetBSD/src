/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig.
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
__RCSID("$NetBSD: t_long_double.c,v 1.1 2026/04/01 21:00:53 rillig Exp $");

#include <atf-c.h>

#include <stdio.h>
#include <stdlib.h>

ATF_TC(long_double);

ATF_TC_HEAD(long_double, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test 'long double' operations");
}

#define	P_ldbl_m_1_63	"%La"
#define	P_ldbl_p_1_63	"%La"
#define	P_dbl_m_1_63	"%a"
#define	P_dbl_p_1_63	"%a"
#define	P_ll_m_1_63	"%#016llx"
#define	P_ll_p_1_63	"%#016llx"
#define	P_ull_1_63	"%#016llx"

ATF_TC_BODY(long_double, tc)
{
	volatile long double ldbl_m_1_63 = -0x1.0p63L;
	volatile long double ldbl_p_1_63 = +0x1.0p63L;
	volatile double dbl_m_1_63 = -0x1.0p63;
	volatile double dbl_p_1_63 = +0x1.0p63;
	volatile long long ll_m_1_63 = (long long)(-1ULL << 63);
	volatile long long ll_p_1_63 = (long long)(+1ULL << 63);
	volatile unsigned long long ull_1_63 = +1ULL << 63;

#define	EXPECT_CMP(lhs, op, rhs) \
	ATF_CHECK_MSG(lhs op rhs, \
	    P_##lhs " !" #op " " P_##rhs, \
	    lhs, rhs)

	// _Qp_fge
	EXPECT_CMP(ldbl_m_1_63, >=, ldbl_m_1_63);
	EXPECT_CMP(ldbl_p_1_63, >=, ldbl_p_1_63);

	// _Qp_fle
	EXPECT_CMP(ldbl_m_1_63, <=, ldbl_m_1_63);
	EXPECT_CMP(ldbl_p_1_63, <=, ldbl_p_1_63);

	// _Qp_feq
	EXPECT_CMP(ldbl_m_1_63, ==, ldbl_m_1_63);
	EXPECT_CMP(ldbl_p_1_63, ==, ldbl_p_1_63);

	// _Qp_fne
	EXPECT_CMP(ldbl_m_1_63, !=, ldbl_p_1_63);
	EXPECT_CMP(ldbl_p_1_63, !=, ldbl_m_1_63);

	// _Qp_xtoq
	EXPECT_CMP(ldbl_m_1_63, ==, ll_m_1_63);
	EXPECT_CMP(ll_m_1_63, ==, ldbl_m_1_63);
	EXPECT_CMP(ldbl_p_1_63, !=, ll_p_1_63);
	EXPECT_CMP(ll_p_1_63, !=, ldbl_p_1_63);

	// ?
	EXPECT_CMP(ldbl_m_1_63, ==, dbl_m_1_63);
	EXPECT_CMP(dbl_m_1_63, ==, ldbl_m_1_63);
	EXPECT_CMP(ldbl_p_1_63, ==, dbl_p_1_63);
	EXPECT_CMP(dbl_p_1_63, ==, ldbl_p_1_63);

	// _Qp_uxtoq
	EXPECT_CMP(ldbl_p_1_63, ==, ull_1_63);

	// _Qp_qtox
	ATF_CHECK_MSG((long long)ldbl_m_1_63 == ll_m_1_63,
	    "%#llx != %#llx", (long long)ldbl_m_1_63, ll_m_1_63);

	// _Qp_qtoux
	ATF_CHECK_MSG((unsigned long long)ldbl_m_1_63 == ull_1_63,
	    "%#llx != %#llx", (unsigned long long)ldbl_m_1_63, ull_1_63);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, long_double);

	return atf_no_error();
}
