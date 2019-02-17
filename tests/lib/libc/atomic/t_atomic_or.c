/*	$NetBSD: t_atomic_or.c,v 1.1 2019/02/17 12:24:17 isaki Exp $	*/

/*
 * Copyright (C) 2019 Tetsuya Isaki. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_atomic_or.c,v 1.1 2019/02/17 12:24:17 isaki Exp $");

#include <atf-c.h>
#include <inttypes.h>
#include <sys/atomic.h>

/*
 * These tests don't examine the atomicity.
 */

#define DST    (0x1122334455667788UL)
#define SRC    (0xf0f0f0f0f0f0f0f0UL)
#define EXPECT (0xf1f2f3f4f5f6f7f8UL)

/*
 * atomic_or_*()
 */
#define atf_or(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE src; \
	TYPE exp; \
	val = (TYPE)DST; \
	src = (TYPE)SRC; \
	exp = (TYPE)EXPECT; \
	NAME(&val, src); \
	ATF_REQUIRE_MSG(val == exp, \
	    "val expects 0x%" FMT " but 0x%" FMT, exp, val); \
}

atf_or(atomic_or_32,    uint32_t,      PRIx32);
atf_or(atomic_or_uint,  unsigned int,  "x");
atf_or(atomic_or_ulong, unsigned long, "lx");
#if defined(__HAVE_ATOMIC64_OPS)
atf_or(atomic_or_64,    uint64_t,      PRIx64);
#endif

/*
 * atomic_or_*_nv()
 */
#define atf_or_nv(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE src; \
	TYPE res; \
	TYPE exp; \
	val = (TYPE)DST; \
	src = (TYPE)SRC; \
	exp = (TYPE)EXPECT; \
	res = NAME(&val, src); \
	ATF_REQUIRE_MSG(val == exp, \
	    "val expects 0x%" FMT " but 0x%" FMT, exp, val); \
	ATF_REQUIRE_MSG(res == exp, \
	    "res expects 0x%" FMT " but 0x%" FMT, exp, res); \
}

atf_or_nv(atomic_or_32_nv,    uint32_t,      PRIx32);
atf_or_nv(atomic_or_uint_nv,  unsigned int,  "x");
atf_or_nv(atomic_or_ulong_nv, unsigned long, "lx");
#if defined(__HAVE_ATOMIC64_OPS)
atf_or_nv(atomic_or_64_nv,    uint64_t,      PRIx64);
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, atomic_or_32);
	ATF_TP_ADD_TC(tp, atomic_or_uint);
	ATF_TP_ADD_TC(tp, atomic_or_ulong);
#if defined(__HAVE_ATOMIC64_OPS)
	ATF_TP_ADD_TC(tp, atomic_or_64);
#endif

	ATF_TP_ADD_TC(tp, atomic_or_32_nv);
	ATF_TP_ADD_TC(tp, atomic_or_uint_nv);
	ATF_TP_ADD_TC(tp, atomic_or_ulong_nv);
#if defined(__HAVE_ATOMIC64_OPS)
	ATF_TP_ADD_TC(tp, atomic_or_64_nv);
#endif

	return atf_no_error();
}
