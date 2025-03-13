/*	$NetBSD: t___sync_compare_and_swap.c,v 1.2 2025/03/13 14:00:50 riastradh Exp $	*/

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
__RCSID("$NetBSD: t___sync_compare_and_swap.c,v 1.2 2025/03/13 14:00:50 riastradh Exp $");

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

#define OLDVAL (0x1122334455667788UL)
#define NEWVAL (0x8090a0b0c0d0e0f0UL)

#define atf_sync_bool(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE oldval; \
	TYPE newval; \
	TYPE expval; \
	bool expres; \
	bool res; \
	/* If successful */ \
	val = (TYPE)OLDVAL; \
	oldval = (TYPE)OLDVAL; \
	newval = (TYPE)NEWVAL; \
	expval = (TYPE)NEWVAL; \
	expres = true; \
	res = NAME(&val, oldval, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "successful case: val expects 0x%" FMT " but 0x%" FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "successful case: res expects %d but %d", expres, res); \
	/* If failure */ \
	val = (TYPE)OLDVAL; \
	oldval = (TYPE)(OLDVAL + 1); \
	newval = (TYPE)NEWVAL; \
	expval = (TYPE)OLDVAL; \
	expres = false; \
	res = NAME(&val, oldval, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "failure case: val expects 0x%" FMT " but 0x%" FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "failure case: res expects %d but %d", expres, res); \
}

atf_sync_bool(__sync_bool_compare_and_swap_1, uint8_t,  PRIx8);
atf_sync_bool(__sync_bool_compare_and_swap_2, uint16_t, PRIx16);
atf_sync_bool(__sync_bool_compare_and_swap_4, uint32_t, PRIx32);
#ifdef __HAVE_ATOMIC64_OPS
atf_sync_bool(__sync_bool_compare_and_swap_8, uint64_t, PRIx64);
#endif

#if _BYTE_ORDER == _LITTLE_ENDIAN
#  define LSB	0
#  define MSB	1
#elif _BYTE_ORDER == _BIG_ENDIAN
#  define LSB	1
#  define MSB	0
#else
#  error Unknown byte order!
#endif

#define atf_sync_bool_subword(NAME, SUBWIDTH, SUBTYPE, TYPE, SUBFMT, FMT) \
ATF_TC(NAME##_subword); \
ATF_TC_HEAD(NAME##_subword, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", "subword " #NAME); \
} \
ATF_TC_BODY(NAME##_subword, tc) \
{ \
	volatile union { \
		TYPE word; \
		SUBTYPE subword[2]; \
	} val; \
	/* If successful */ \
	val.subword[LSB] = -1; \
	val.subword[MSB] = 123; \
	ATF_CHECK(NAME(&val.subword[LSB], /*old*/-1, /*new*/-2)); \
	ATF_CHECK_EQ_MSG(val.subword[LSB], (SUBTYPE)-2, \
	    "val.subword[LSB] = 0x%" SUBFMT, val.subword[LSB]); \
	ATF_CHECK_EQ_MSG(val.subword[MSB], 123, \
	    "val.subword[MSB] = 0x%" SUBFMT, val.subword[MSB]); \
	ATF_CHECK_EQ_MSG(val.word, \
	    ((TYPE)123 << (SUBWIDTH)) | (TYPE)(SUBTYPE)-2, \
	    "val.word = 0x%" FMT, val.word); \
	/* If failed */ \
	val.subword[LSB] = -3; \
	val.subword[MSB] = 45; \
	ATF_CHECK(!NAME(&val.subword[LSB], /*old*/-1, /*new*/-2)); \
	ATF_CHECK_EQ_MSG(val.subword[LSB], (SUBTYPE)-3, \
	    "val.subword[LSB] = 0x%" SUBFMT, val.subword[LSB]); \
	ATF_CHECK_EQ_MSG(val.subword[MSB], 45, \
	    "val.subword[MSB] = 0x%" SUBFMT, val.subword[MSB]); \
	ATF_CHECK_EQ_MSG(val.word, \
	    ((TYPE)45 << (SUBWIDTH)) | (TYPE)(SUBTYPE)-3, \
	    "val.word = 0x%" FMT, val.word); \
}

atf_sync_bool_subword(__sync_bool_compare_and_swap_1, 8, uint8_t, uint16_t,
    PRIx8, PRIx16);
atf_sync_bool_subword(__sync_bool_compare_and_swap_2, 16, uint16_t, uint32_t,
    PRIx16, PRIx32);
atf_sync_bool_subword(__sync_bool_compare_and_swap_4, 32, uint32_t, uint64_t,
    PRIx32, PRIx64);

#define atf_sync_val(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE oldval; \
	TYPE newval; \
	TYPE expval; \
	TYPE expres; \
	TYPE res; \
	/* If successful */ \
	val = (TYPE)OLDVAL; \
	oldval = (TYPE)OLDVAL; \
	newval = (TYPE)NEWVAL; \
	expval = (TYPE)NEWVAL; \
	expres = (TYPE)OLDVAL; \
	res = NAME(&val, oldval, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "successful case: val expects 0x%" FMT " but 0x%" FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "successful case: res expects 0x%" FMT " but 0x%" FMT, expres, res); \
	/* If failure */ \
	val = (TYPE)OLDVAL; \
	oldval = (TYPE)(OLDVAL + 1); \
	newval = (TYPE)NEWVAL; \
	expval = (TYPE)OLDVAL; \
	expres = (TYPE)OLDVAL; \
	res = NAME(&val, oldval, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "failure case: val expects 0x%" FMT " but 0x%" FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "failure case: res expects 0x%" FMT " but 0x%" FMT, expres, res); \
}

atf_sync_val(__sync_val_compare_and_swap_1, uint8_t,  PRIx8);
atf_sync_val(__sync_val_compare_and_swap_2, uint16_t, PRIx16);
atf_sync_val(__sync_val_compare_and_swap_4, uint32_t, PRIx32);
#ifdef __HAVE_ATOMIC64_OPS
atf_sync_val(__sync_val_compare_and_swap_8, uint64_t, PRIx64);
#endif

#define atf_sync_val_subword(NAME, SUBWIDTH, SUBTYPE, TYPE, SUBFMT, FMT) \
ATF_TC(NAME##_subword); \
ATF_TC_HEAD(NAME##_subword, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", "subword " #NAME); \
} \
ATF_TC_BODY(NAME##_subword, tc) \
{ \
	volatile union { \
		TYPE word; \
		SUBTYPE subword[2]; \
	} val; \
	SUBTYPE rval; \
	/* If successful */ \
	val.subword[LSB] = -1; \
	val.subword[MSB] = 123; \
	ATF_CHECK_EQ_MSG((rval = NAME(&val.subword[LSB], \
		/*old*/-1, /*new*/-2)), (SUBTYPE)-1, \
	    "expected 0x%" SUBFMT ", got 0x%" SUBFMT, \
	    (SUBTYPE)-1, rval); \
	ATF_CHECK_EQ_MSG(val.subword[LSB], (SUBTYPE)-2, \
	    "val.subword[LSB] = 0x%" SUBFMT, val.subword[LSB]); \
	ATF_CHECK_EQ_MSG(val.subword[MSB], 123, \
	    "val.subword[MSB] = 0x%" SUBFMT, val.subword[MSB]); \
	ATF_CHECK_EQ_MSG(val.word, \
	    ((TYPE)123 << (SUBWIDTH)) | (TYPE)(SUBTYPE)-2, \
	    "val.word = 0x%" FMT, val.word); \
	/* If failed */ \
	val.subword[LSB] = -3; \
	val.subword[MSB] = 45; \
	ATF_CHECK_EQ_MSG((rval = NAME(&val.subword[LSB], \
		/*old*/-1, /*new*/-2)), (SUBTYPE)-3, \
	    "expected 0x%" SUBFMT ", got 0x%" SUBFMT, \
	    (SUBTYPE)-3, rval); \
	ATF_CHECK_EQ_MSG(val.subword[LSB], (SUBTYPE)-3, \
	    "val.subword[LSB] = 0x%" SUBFMT, val.subword[LSB]); \
	ATF_CHECK_EQ_MSG(val.subword[MSB], 45, \
	    "val.subword[MSB] = 0x%" SUBFMT, val.subword[MSB]); \
	ATF_CHECK_EQ_MSG(val.word, \
	    ((TYPE)45 << (SUBWIDTH)) | (TYPE)(SUBTYPE)-3, \
	    "val.word = 0x%" FMT, val.word); \
}

atf_sync_val_subword(__sync_val_compare_and_swap_1, 8, uint8_t, uint16_t,
    PRIx8, PRIx16);
atf_sync_val_subword(__sync_val_compare_and_swap_2, 16, uint16_t, uint32_t,
    PRIx16, PRIx32);
atf_sync_val_subword(__sync_val_compare_and_swap_4, 32, uint32_t, uint64_t,
    PRIx32, PRIx64);

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_1);
	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_2);
	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_4);
#ifdef __HAVE_ATOMIC64_OPS
	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_8);
#endif

	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_1_subword);
	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_2_subword);
	ATF_TP_ADD_TC(tp, __sync_bool_compare_and_swap_4_subword);

	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_1);
	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_2);
	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_4);
#ifdef __HAVE_ATOMIC64_OPS
	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_8);
#endif

	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_1_subword);
	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_2_subword);
	ATF_TP_ADD_TC(tp, __sync_val_compare_and_swap_4_subword);

	return atf_no_error();
}
