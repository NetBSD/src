/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <atf-c.h>

#include "atf-c/expand.h"

#include "h_lib.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
bool
matches_glob(const char *glob, const char *candidate)
{
    bool result;

    RE(atf_expand_matches_glob(glob, candidate, &result));

    return result;
}

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(is_glob);
ATF_TC_HEAD(is_glob, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function");
}
ATF_TC_BODY(is_glob, tc)
{
    ATF_REQUIRE(!atf_expand_is_glob(""));
    ATF_REQUIRE(!atf_expand_is_glob("a"));
    ATF_REQUIRE(!atf_expand_is_glob("foo"));

    ATF_REQUIRE( atf_expand_is_glob("*"));
    ATF_REQUIRE( atf_expand_is_glob("a*"));
    ATF_REQUIRE( atf_expand_is_glob("*a"));
    ATF_REQUIRE( atf_expand_is_glob("a*b"));

    ATF_REQUIRE( atf_expand_is_glob("?"));
    ATF_REQUIRE( atf_expand_is_glob("a?"));
    ATF_REQUIRE( atf_expand_is_glob("?a"));
    ATF_REQUIRE( atf_expand_is_glob("a?b"));
}

ATF_TC(matches_glob_plain);
ATF_TC_HEAD(matches_glob_plain, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function "
                      "by using plain text strings as globs");
}
ATF_TC_BODY(matches_glob_plain, tc)
{
    ATF_REQUIRE( matches_glob("", ""));
    ATF_REQUIRE(!matches_glob("a", ""));
    ATF_REQUIRE(!matches_glob("", "a"));

    ATF_REQUIRE( matches_glob("ab", "ab"));
    ATF_REQUIRE(!matches_glob("abc", "ab"));
    ATF_REQUIRE(!matches_glob("ab", "abc"));
}

ATF_TC(matches_glob_star);
ATF_TC_HEAD(matches_glob_star, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function "
                      "by using the '*' glob-character");
}
ATF_TC_BODY(matches_glob_star, tc)
{
    ATF_REQUIRE( matches_glob("*", ""));
    ATF_REQUIRE( matches_glob("*", "a"));
    ATF_REQUIRE( matches_glob("*", "ab"));

    ATF_REQUIRE(!matches_glob("a*", ""));
    ATF_REQUIRE( matches_glob("a*", "a"));
    ATF_REQUIRE( matches_glob("a*", "aa"));
    ATF_REQUIRE( matches_glob("a*", "ab"));
    ATF_REQUIRE( matches_glob("a*", "abc"));
    ATF_REQUIRE(!matches_glob("a*", "ba"));

    ATF_REQUIRE( matches_glob("*a", "a"));
    ATF_REQUIRE( matches_glob("*a", "ba"));
    ATF_REQUIRE(!matches_glob("*a", "bc"));
    ATF_REQUIRE(!matches_glob("*a", "bac"));

    ATF_REQUIRE( matches_glob("*ab", "ab"));
    ATF_REQUIRE( matches_glob("*ab", "aab"));
    ATF_REQUIRE( matches_glob("*ab", "aaab"));
    ATF_REQUIRE( matches_glob("*ab", "bab"));
    ATF_REQUIRE(!matches_glob("*ab", "bcb"));
    ATF_REQUIRE(!matches_glob("*ab", "bacb"));

    ATF_REQUIRE( matches_glob("a*b", "ab"));
    ATF_REQUIRE( matches_glob("a*b", "acb"));
    ATF_REQUIRE( matches_glob("a*b", "acdeb"));
    ATF_REQUIRE(!matches_glob("a*b", "acdebz"));
    ATF_REQUIRE(!matches_glob("a*b", "zacdeb"));
}

ATF_TC(matches_glob_question);
ATF_TC_HEAD(matches_glob_question, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function  "
                      "by using the '?' glob-character");
}
ATF_TC_BODY(matches_glob_question, tc)
{
    ATF_REQUIRE(!matches_glob("?", ""));
    ATF_REQUIRE( matches_glob("?", "a"));
    ATF_REQUIRE(!matches_glob("?", "ab"));

    ATF_REQUIRE( matches_glob("?", "b"));
    ATF_REQUIRE( matches_glob("?", "c"));

    ATF_REQUIRE( matches_glob("a?", "ab"));
    ATF_REQUIRE( matches_glob("a?", "ac"));
    ATF_REQUIRE(!matches_glob("a?", "ca"));

    ATF_REQUIRE( matches_glob("???", "abc"));
    ATF_REQUIRE( matches_glob("???", "def"));
    ATF_REQUIRE(!matches_glob("???", "a"));
    ATF_REQUIRE(!matches_glob("???", "ab"));
    ATF_REQUIRE(!matches_glob("???", "abcd"));
}

/* ---------------------------------------------------------------------
 * Tests cases for the header file.
 * --------------------------------------------------------------------- */

HEADER_TC(include, "atf-c/expand.h", "d_include_expand_h.c");

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, is_glob);
    ATF_TP_ADD_TC(tp, matches_glob_plain);
    ATF_TP_ADD_TC(tp, matches_glob_star);
    ATF_TP_ADD_TC(tp, matches_glob_question);

    /* Add the test cases for the header file. */
    ATF_TP_ADD_TC(tp, include);

    return atf_no_error();
}
