/*	$NetBSD: t_stddef.c,v 1.1 2025/04/01 00:33:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

/*
 * Include <stddef.h> first to verify it declares everything we need.
 */
#include <stddef.h>

#if __STDC_VERSION__ - 0 >= 202311L
#if __STDC_VERSION_STDDEF_H__ - 0 < 202311L
#error __STDC_VERSION_STDDEF_H__ not defined appropriately
#endif
#endif

typedef ptrdiff_t nbtest_ptrdiff_t;
typedef size_t nbtest_size_t;
#if __STDC_VERSION__ - 0 >= 201112L
typedef max_align_t nbtest_max_align_t;
#endif
typedef wchar_t nbtest_wchar_t;
#if __STDC_VERSION__ - 0 >= 202311L
typedef nullptr_t nbtest_nullptr_t;
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_stddef.c,v 1.1 2025/04/01 00:33:55 riastradh Exp $");

#include <atf-c.h>
#include <stdalign.h>

ATF_TC(macros);
ATF_TC_HEAD(macros, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test <stddef.h> macros work");
}
ATF_TC_BODY(macros, tc)
{
	void *volatile pNULL = NULL;
#if __STDC_VERSION__ - 0 >= 202311L
	void *volatile pnullptr = nullptr;
#endif
	struct s { char x[3], y; };
	size_t o;

	ATF_CHECK(!pNULL);
#if __STDC_VERSION__ - 0 >= 202311L
	ATF_CHECK(!pnullptr);
#endif

#if __STDC_VERSION__ - 0 >= 202311L
	volatile enum { A, B } x = A;
	switch (x) {
	case A:
		break;
	case B:
	default:
		unreachable();
	}
#endif

	ATF_CHECK_MSG((o = offsetof(struct s, y)) == 3,
	    "o=%zu", o);
}

ATF_TC(types);
ATF_TC_HEAD(types, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test <stddef.h> types are reasonable");
}
ATF_TC_BODY(types, tc)
{

#ifdef __GNUC__
	char *p, *q;
	ATF_CHECK(__builtin_types_compatible_p(ptrdiff_t, typeof(p - q)));
	ATF_CHECK(__builtin_types_compatible_p(size_t, typeof(sizeof(p))));
#if __STDC_VERSION__ - 0 >= 202311L
	ATF_CHECK(__builtin_types_compatible_p(nullptr_t, typeof(nullptr)));
#endif
#endif

#if __STDC_VERSION__ - 0 >= 201112L
	size_t a;
	ATF_CHECK_MSG((a = alignof(max_align_t)) >= alignof(long long),
	    "a=%zu", a);
	ATF_CHECK_MSG((a = alignof(max_align_t)) >= alignof(long double),
	    "a=%zu", a);
	ATF_CHECK_MSG((a = alignof(max_align_t)) >= alignof(void *),
	    "a=%zu", a);
	ATF_CHECK_MSG((a = alignof(max_align_t)) >= alignof(int (*)(void)),
	    "a=%zu", a);
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, macros);
	ATF_TP_ADD_TC(tp, types);

	return atf_no_error();
}
