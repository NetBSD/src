//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <cstring>

#include "atf/expand.hpp"
#include "atf/macros.hpp"

// ------------------------------------------------------------------------
// Test cases for the "pattern_error" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(pattern_error);
ATF_TEST_CASE_HEAD(pattern_error)
{
    set("descr", "Tests the pattern_error class.");
}
ATF_TEST_CASE_BODY(pattern_error)
{
    using atf::expand::pattern_error;

    atf::utils::auto_array< char > buf(new char[80]);
    std::strcpy(buf.get(), "Test string.");

    pattern_error pe1(buf);
    {
        pattern_error pe2(pe1);
        ATF_CHECK(std::strcmp(pe1.what(), pe2.what()) == 0);
    }
    ATF_CHECK(std::strcmp(pe1.what(), "Test string.") == 0);
}

// ------------------------------------------------------------------------
// Test cases for the free functions.
// ------------------------------------------------------------------------

ATF_TEST_CASE(is_glob);
ATF_TEST_CASE_HEAD(is_glob)
{
    set("descr", "Tests the is_glob function.");
}
ATF_TEST_CASE_BODY(is_glob)
{
    using atf::expand::is_glob;

    ATF_CHECK(!is_glob(""));
    ATF_CHECK(!is_glob("a"));
    ATF_CHECK(!is_glob("foo"));

    ATF_CHECK( is_glob("*"));
    ATF_CHECK( is_glob("a*"));
    ATF_CHECK( is_glob("*a"));
    ATF_CHECK( is_glob("a*b"));

    ATF_CHECK( is_glob("?"));
    ATF_CHECK( is_glob("a?"));
    ATF_CHECK( is_glob("?a"));
    ATF_CHECK( is_glob("a?b"));
}

ATF_TEST_CASE(matches_glob_plain);
ATF_TEST_CASE_HEAD(matches_glob_plain)
{
    set("descr", "Tests the matches_glob function by using plain text "
                 "strings as patterns only.");
}
ATF_TEST_CASE_BODY(matches_glob_plain)
{
    using atf::expand::matches_glob;

    ATF_CHECK( matches_glob("", ""));
    ATF_CHECK(!matches_glob("a", ""));
    ATF_CHECK(!matches_glob("", "a"));

    ATF_CHECK( matches_glob("ab", "ab"));
    ATF_CHECK(!matches_glob("abc", "ab"));
    ATF_CHECK(!matches_glob("ab", "abc"));
}

ATF_TEST_CASE(matches_glob_star);
ATF_TEST_CASE_HEAD(matches_glob_star)
{
    set("descr", "Tests the matches_glob function by using the '*' meta-"
                 "character as part of the pattern.");
}
ATF_TEST_CASE_BODY(matches_glob_star)
{
    using atf::expand::matches_glob;

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

ATF_TEST_CASE(matches_glob_question);
ATF_TEST_CASE_HEAD(matches_glob_question)
{
    set("descr", "Tests the matches_glob function by using the '?' meta-"
                 "character as part of the pattern.");
}
ATF_TEST_CASE_BODY(matches_glob_question)
{
    using atf::expand::matches_glob;

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

ATF_TEST_CASE(expand_glob_base);
ATF_TEST_CASE_HEAD(expand_glob_base)
{
    set("descr", "Tests the expand_glob function with random patterns.");
}
ATF_TEST_CASE_BODY(expand_glob_base)
{
    using atf::expand::expand_glob;

    std::set< std::string > candidates;
    candidates.insert("foo");
    candidates.insert("bar");
    candidates.insert("baz");
    candidates.insert("foobar");
    candidates.insert("foobarbaz");
    candidates.insert("foobarbazfoo");

    std::set< std::string > exps;

    exps = expand_glob("foo", candidates);
    ATF_CHECK_EQUAL(exps.size(), 1);
    ATF_CHECK(exps.find("foo") != exps.end());

    exps = expand_glob("bar", candidates);
    ATF_CHECK_EQUAL(exps.size(), 1);
    ATF_CHECK(exps.find("bar") != exps.end());

    exps = expand_glob("foo*", candidates);
    ATF_CHECK_EQUAL(exps.size(), 4);
    ATF_CHECK(exps.find("foo") != exps.end());
    ATF_CHECK(exps.find("foobar") != exps.end());
    ATF_CHECK(exps.find("foobarbaz") != exps.end());
    ATF_CHECK(exps.find("foobarbazfoo") != exps.end());

    exps = expand_glob("*foo", candidates);
    ATF_CHECK_EQUAL(exps.size(), 2);
    ATF_CHECK(exps.find("foo") != exps.end());
    ATF_CHECK(exps.find("foobarbazfoo") != exps.end());

    exps = expand_glob("*foo*", candidates);
    ATF_CHECK_EQUAL(exps.size(), 4);
    ATF_CHECK(exps.find("foo") != exps.end());
    ATF_CHECK(exps.find("foobar") != exps.end());
    ATF_CHECK(exps.find("foobarbaz") != exps.end());
    ATF_CHECK(exps.find("foobarbazfoo") != exps.end());

    exps = expand_glob("ba", candidates);
    ATF_CHECK_EQUAL(exps.size(), 0);

    exps = expand_glob("ba*", candidates);
    ATF_CHECK_EQUAL(exps.size(), 2);
    ATF_CHECK(exps.find("bar") != exps.end());
    ATF_CHECK(exps.find("baz") != exps.end());

    exps = expand_glob("*ba", candidates);
    ATF_CHECK_EQUAL(exps.size(), 0);

    exps = expand_glob("*ba*", candidates);
    ATF_CHECK_EQUAL(exps.size(), 5);
    ATF_CHECK(exps.find("bar") != exps.end());
    ATF_CHECK(exps.find("baz") != exps.end());
    ATF_CHECK(exps.find("foobar") != exps.end());
    ATF_CHECK(exps.find("foobarbaz") != exps.end());
    ATF_CHECK(exps.find("foobarbazfoo") != exps.end());
}

ATF_TEST_CASE(expand_glob_tps);
ATF_TEST_CASE_HEAD(expand_glob_tps)
{
    set("descr", "Tests the expand_glob function with patterns that match "
                 "typical test program names.  This is just a subcase "
                 "of expand_base, but it is nice to make sure that "
                 "it really works.");
}
ATF_TEST_CASE_BODY(expand_glob_tps)
{
    using atf::expand::expand_glob;

    std::set< std::string > candidates;
    candidates.insert("Atffile");
    candidates.insert("h_foo");
    candidates.insert("t_foo");
    candidates.insert("t_bar");
    candidates.insert("t_baz");
    candidates.insert("foo_helper");
    candidates.insert("foo_test");
    candidates.insert("bar_test");
    candidates.insert("baz_test");

    std::set< std::string > exps;

    exps = expand_glob("t_*", candidates);
    ATF_CHECK_EQUAL(exps.size(), 3);
    ATF_CHECK(exps.find("t_foo") != exps.end());
    ATF_CHECK(exps.find("t_bar") != exps.end());
    ATF_CHECK(exps.find("t_baz") != exps.end());

    exps = expand_glob("*_test", candidates);
    ATF_CHECK_EQUAL(exps.size(), 3);
    ATF_CHECK(exps.find("foo_test") != exps.end());
    ATF_CHECK(exps.find("bar_test") != exps.end());
    ATF_CHECK(exps.find("baz_test") != exps.end());
}

ATF_INIT_TEST_CASES(tcs)
{
    // Add the tests for the "pattern_error" class.
    ATF_ADD_TEST_CASE(tcs, pattern_error);

    // Add the tests for the free functions.
    ATF_ADD_TEST_CASE(tcs, is_glob);
    ATF_ADD_TEST_CASE(tcs, matches_glob_plain);
    ATF_ADD_TEST_CASE(tcs, matches_glob_star);
    ATF_ADD_TEST_CASE(tcs, matches_glob_question);
    ATF_ADD_TEST_CASE(tcs, expand_glob_base);
    ATF_ADD_TEST_CASE(tcs, expand_glob_tps);
}
