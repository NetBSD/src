/*	$NetBSD: t_atomic_cas.c,v 1.1 2019/02/17 12:24:17 isaki Exp $	*/

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
__RCSID("$NetBSD: t_atomic_cas.c,v 1.1 2019/02/17 12:24:17 isaki Exp $");

#include <atf-c.h>
#include <inttypes.h>
#include <sys/atomic.h>

/*
 * These tests don't examine the atomicity.
 */

#define OLDVAL (0x1122334455667788UL)
#define NEWVAL (0x8090a0b0c0d0e0f0UL)

/*
 * atomic_cas_*{,_ni}()
 */
#define atf_cas(NAME, TYPE, FMT) \
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
	    "successful case: val expects " FMT " but " FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "successful case: res expects " FMT " but " FMT, expres, res); \
	/* If failure */ \
	val = (TYPE)OLDVAL; \
	oldval = (TYPE)(OLDVAL + 1); \
	newval = (TYPE)NEWVAL; \
	expval = (TYPE)OLDVAL; \
	expres = (TYPE)OLDVAL; \
	res = NAME(&val, oldval, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "failure case: val expects " FMT " but " FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "failure case: res expects " FMT " but " FMT, expres, res); \
}

atf_cas(atomic_cas_32,       uint32_t,      "0x%" PRIx32);
atf_cas(atomic_cas_uint,     unsigned int,  "0x%x");
atf_cas(atomic_cas_ulong,    unsigned long, "0x%lx");
atf_cas(atomic_cas_ptr,      void *,        "%p");
#if defined(__HAVE_ATOMIC64_OPS)
atf_cas(atomic_cas_64,       uint64_t,      "0x%" PRIx64);
#endif

atf_cas(atomic_cas_32_ni,    uint32_t,      "0x%" PRIx32);
atf_cas(atomic_cas_uint_ni,  unsigned int,  "0x%x");
atf_cas(atomic_cas_ulong_ni, unsigned long, "0x%lx");
atf_cas(atomic_cas_ptr_ni,   void *,        "%p");
#if defined(__HAVE_ATOMIC64_OPS)
atf_cas(atomic_cas_64_ni,    uint64_t,      "0x%" PRIx64);
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, atomic_cas_32);
	ATF_TP_ADD_TC(tp, atomic_cas_uint);
	ATF_TP_ADD_TC(tp, atomic_cas_ulong);
	ATF_TP_ADD_TC(tp, atomic_cas_ptr);
#if defined(__HAVE_ATOMIC64_OPS)
	ATF_TP_ADD_TC(tp, atomic_cas_64);
#endif

	ATF_TP_ADD_TC(tp, atomic_cas_32_ni);
	ATF_TP_ADD_TC(tp, atomic_cas_uint_ni);
	ATF_TP_ADD_TC(tp, atomic_cas_ulong_ni);
	ATF_TP_ADD_TC(tp, atomic_cas_ptr_ni);
#if defined(__HAVE_ATOMIC64_OPS)
	ATF_TP_ADD_TC(tp, atomic_cas_64_ni);
#endif

	return atf_no_error();
}
