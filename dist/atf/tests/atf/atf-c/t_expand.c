/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

static
bool
matches_glob(const char *glob, const char *candidate)
{
    bool result;

    CE(atf_expand_matches_glob(glob, candidate, &result));

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
    ATF_CHECK(!atf_expand_is_glob(""));
    ATF_CHECK(!atf_expand_is_glob("a"));
    ATF_CHECK(!atf_expand_is_glob("foo"));

    ATF_CHECK( atf_expand_is_glob("*"));
    ATF_CHECK( atf_expand_is_glob("a*"));
    ATF_CHECK( atf_expand_is_glob("*a"));
    ATF_CHECK( atf_expand_is_glob("a*b"));

    ATF_CHECK( atf_expand_is_glob("?"));
    ATF_CHECK( atf_expand_is_glob("a?"));
    ATF_CHECK( atf_expand_is_glob("?a"));
    ATF_CHECK( atf_expand_is_glob("a?b"));
}

ATF_TC(matches_glob_plain);
ATF_TC_HEAD(matches_glob_plain, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function "
                      "by using plain text strings as globs");
}
ATF_TC_BODY(matches_glob_plain, tc)
{
    ATF_CHECK( matches_glob("", ""));
    ATF_CHECK(!matches_glob("a", ""));
    ATF_CHECK(!matches_glob("", "a"));

    ATF_CHECK( matches_glob("ab", "ab"));
    ATF_CHECK(!matches_glob("abc", "ab"));
    ATF_CHECK(!matches_glob("ab", "abc"));
}

ATF_TC(matches_glob_star);
ATF_TC_HEAD(matches_glob_star, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function "
                      "by using the '*' glob-character");
}
ATF_TC_BODY(matches_glob_star, tc)
{
    ATF_CHECK( matches_glob("*", ""));
    ATF_CHECK( matches_glob("*", "a"));
    ATF_CHECK( matches_glob("*", "ab"));

    ATF_CHECK(!matches_glob("a*", ""));
    ATF_CHECK( matches_glob("a*", "a"));
    ATF_CHECK( matches_glob("a*", "aa"));
    ATF_CHECK( matches_glob("a*", "ab"));
    ATF_CHECK( matches_glob("a*", "abc"));
    ATF_CHECK(!matches_glob("a*", "ba"));

    ATF_CHECK( matches_glob("*a", "a"));
    ATF_CHECK( matches_glob("*a", "ba"));
    ATF_CHECK(!matches_glob("*a", "bc"));
    ATF_CHECK(!matches_glob("*a", "bac"));

    ATF_CHECK( matches_glob("*ab", "ab"));
    ATF_CHECK( matches_glob("*ab", "aab"));
    ATF_CHECK( matches_glob("*ab", "aaab"));
    ATF_CHECK( matches_glob("*ab", "bab"));
    ATF_CHECK(!matches_glob("*ab", "bcb"));
    ATF_CHECK(!matches_glob("*ab", "bacb"));

    ATF_CHECK( matches_glob("a*b", "ab"));
    ATF_CHECK( matches_glob("a*b", "acb"));
    ATF_CHECK( matches_glob("a*b", "acdeb"));
    ATF_CHECK(!matches_glob("a*b", "acdebz"));
    ATF_CHECK(!matches_glob("a*b", "zacdeb"));
}

ATF_TC(matches_glob_question);
ATF_TC_HEAD(matches_glob_question, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_expand_is_glob function  "
                      "by using the '?' glob-character");
}
ATF_TC_BODY(matches_glob_question, tc)
{
    ATF_CHECK(!matches_glob("?", ""));
    ATF_CHECK( matches_glob("?", "a"));
    ATF_CHECK(!matches_glob("?", "ab"));

    ATF_CHECK( matches_glob("?", "b"));
    ATF_CHECK( matches_glob("?", "c"));

    ATF_CHECK( matches_glob("a?", "ab"));
    ATF_CHECK( matches_glob("a?", "ac"));
    ATF_CHECK(!matches_glob("a?", "ca"));

    ATF_CHECK( matches_glob("???", "abc"));
    ATF_CHECK( matches_glob("???", "def"));
    ATF_CHECK(!matches_glob("???", "a"));
    ATF_CHECK(!matches_glob("???", "ab"));
    ATF_CHECK(!matches_glob("???", "abcd"));
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, is_glob);
    ATF_TP_ADD_TC(tp, matches_glob_plain);
    ATF_TP_ADD_TC(tp, matches_glob_star);
    ATF_TP_ADD_TC(tp, matches_glob_question);

    return atf_no_error();
}
