/*	$NetBSD: t_uchar.c,v 1.3.2.2 2024/10/14 17:20:19 martin Exp $	*/

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

/*
 * Include <uchar.h> first to verify it declares everything we need.
 */
#include <uchar.h>
typedef mbstate_t nbtest_mbstate_t;
typedef size_t nbtest_size_t;
typedef char8_t nbtest_char8_t;
typedef char16_t nbtest_char16_t;
typedef char32_t nbtest_char32_t;
static size_t (*nbtest_mbrtoc8)(char8_t *restrict, const char *restrict,
    size_t, mbstate_t *restrict) __unused = &mbrtoc8;
static size_t (*nbtest_c8rtomb)(char *restrict, char8_t,
    mbstate_t *restrict) __unused = &c8rtomb;
static size_t (*nbtest_mbrtoc16)(char16_t *restrict, const char *restrict,
    size_t, mbstate_t *restrict) __unused = &mbrtoc16;
static size_t (*nbtest_c16rtomb)(char *restrict, char16_t,
    mbstate_t *restrict) __unused = &c16rtomb;
static size_t (*nbtest_mbrtoc32)(char32_t *restrict, const char *restrict,
    size_t, mbstate_t *restrict) __unused = mbrtoc32;
static size_t (*nbtest_c32rtomb)(char *restrict, char32_t,
    mbstate_t *restrict) __unused = &c32rtomb;

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_uchar.c,v 1.3.2.2 2024/10/14 17:20:19 martin Exp $");

#include <atf-c.h>
#include <stdint.h>

ATF_TC(uchartypes);
ATF_TC_HEAD(uchartypes, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test <uchar.h> types are reasonable");
}
ATF_TC_BODY(uchartypes, tc)
{

	ATF_CHECK_EQ_MSG(sizeof(char8_t), sizeof(unsigned char),
	    "char8_t %zu, unsigned char %zu",
	    sizeof(char8_t), sizeof(unsigned char));
	ATF_CHECK_EQ_MSG(sizeof(char16_t), sizeof(uint_least16_t),
	    "char16_t %zu, uint_least16_t %zu",
	    sizeof(char16_t), sizeof(uint_least16_t));
	ATF_CHECK_EQ_MSG(sizeof(char32_t), sizeof(uint_least32_t),
	    "char32_t %zu, uint_least32_t %zu",
	    sizeof(char32_t), sizeof(uint_least32_t));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, uchartypes);
	return atf_no_error();
}
