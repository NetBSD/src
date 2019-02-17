/*	$NetBSD: t_atomic_swap.c,v 1.1 2019/02/17 12:24:17 isaki Exp $	*/

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
__RCSID("$NetBSD: t_atomic_swap.c,v 1.1 2019/02/17 12:24:17 isaki Exp $");

#include <atf-c.h>
#include <inttypes.h>
#include <sys/atomic.h>

/*
 * These tests don't examine the atomicity.
 */

#define OLDVAL (0x1122334455667788UL)
#define NEWVAL (0x8090a0b0c0d0e0f0UL)

/*
 * atomic_swap_*()
 */
#define atf_swap(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE newval; \
	TYPE expval; \
	TYPE expres; \
	TYPE res; \
	val = (TYPE)OLDVAL; \
	newval = (TYPE)NEWVAL; \
	expval = (TYPE)NEWVAL; \
	expres = (TYPE)OLDVAL; \
	res = NAME(&val, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "val expects " FMT " but " FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "res expects " FMT " but " FMT, expres, res); \
}

atf_swap(atomic_swap_32,    uint32_t,      "0x%" PRIx32);
atf_swap(atomic_swap_uint,  unsigned int,  "0x%x");
atf_swap(atomic_swap_ulong, unsigned long, "0x%lx");
atf_swap(atomic_swap_ptr,   void *,        "%p");
#if defined(__HAVE_ATOMIC64_OPS)
atf_swap(atomic_swap_64,    uint64_t,      "0x%" PRIx64);
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, atomic_swap_32);
	ATF_TP_ADD_TC(tp, atomic_swap_uint);
	ATF_TP_ADD_TC(tp, atomic_swap_ulong);
	ATF_TP_ADD_TC(tp, atomic_swap_ptr);
#if defined(__HAVE_ATOMIC64_OPS)
	ATF_TP_ADD_TC(tp, atomic_swap_64);
#endif

	return atf_no_error();
}
