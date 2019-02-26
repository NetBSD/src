/*	$NetBSD: t___sync_xor.c,v 1.1 2019/02/26 10:01:41 isaki Exp $	*/

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
__RCSID("$NetBSD: t___sync_xor.c,v 1.1 2019/02/26 10:01:41 isaki Exp $");

#include <atf-c.h>
#include <inttypes.h>
#include <machine/types.h>	// for __HAVE_ATOMIC64_OPS

/*
 * These tests don't examine the atomicity.
 */

/* XXX
 * Depending on a combination of arch and compiler, __sync_* is
 * implemented as compiler's builtin function.  In that case, even
 * if libc exports the function symbol, it is not used.  As a result
 * this tests will examine compiler's builtin functions.
 * It's better to run only when target is actually in libc.
 */

#define DST    (0x1122334455667788UL)
#define SRC    (0xf0f0f0f0f0f0f0f0UL)
#define EXPECT (0xe1d2c3b4a5968778UL)

#define atf_sync_prefetch(NAME, TYPE, FMT) \
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
	TYPE expval; \
	TYPE expres; \
	val = (TYPE)DST; \
	src = (TYPE)SRC; \
	expval = (TYPE)EXPECT; \
	expres = (TYPE)DST; \
	res = NAME(&val, src); \
	ATF_REQUIRE_MSG(val == expval, \
	    "val expects 0x%" FMT " but 0x%" FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "res expects 0x%" FMT " but 0x%" FMT, expres, res); \
}

atf_sync_prefetch(__sync_fetch_and_xor_1, uint8_t,  PRIx8);
atf_sync_prefetch(__sync_fetch_and_xor_2, uint16_t, PRIx16);
atf_sync_prefetch(__sync_fetch_and_xor_4, uint32_t, PRIx32);
#if defined(__HAVE_ATOMIC64_OPS)
atf_sync_prefetch(__sync_fetch_and_xor_8, uint64_t, PRIx64);
#endif

#define atf_sync_postfetch(NAME, TYPE, FMT) \
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

atf_sync_postfetch(__sync_xor_and_fetch_1, uint8_t,  PRIx8);
atf_sync_postfetch(__sync_xor_and_fetch_2, uint16_t, PRIx16);
atf_sync_postfetch(__sync_xor_and_fetch_4, uint32_t, PRIx32);
#ifdef __HAVE_ATOMIC64_OPS
atf_sync_postfetch(__sync_xor_and_fetch_8, uint64_t, PRIx64);
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, __sync_fetch_and_xor_1);
	ATF_TP_ADD_TC(tp, __sync_fetch_and_xor_2);
	ATF_TP_ADD_TC(tp, __sync_fetch_and_xor_4);
#ifdef __HAVE_ATOMIC64_OPS
	ATF_TP_ADD_TC(tp, __sync_fetch_and_xor_8);
#endif

	ATF_TP_ADD_TC(tp, __sync_xor_and_fetch_1);
	ATF_TP_ADD_TC(tp, __sync_xor_and_fetch_2);
	ATF_TP_ADD_TC(tp, __sync_xor_and_fetch_4);
#ifdef __HAVE_ATOMIC64_OPS
	ATF_TP_ADD_TC(tp, __sync_xor_and_fetch_8);
#endif

	return atf_no_error();
}
