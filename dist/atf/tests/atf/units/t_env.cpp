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

#include "atf/env.hpp"
#include "atf/macros.hpp"

ATF_TEST_CASE(has_get);
ATF_TEST_CASE_HEAD(has_get)
{
    set("descr", "Tests the has and get functions");
}
ATF_TEST_CASE_BODY(has_get)
{
    ATF_CHECK(atf::env::has("PATH"));
    ATF_CHECK(!atf::env::get("PATH").empty());

    ATF_CHECK(!atf::env::has("_UNDEFINED_VARIABLE_"));
}

ATF_TEST_CASE(set);
ATF_TEST_CASE_HEAD(set)
{
    set("descr", "Tests the set function");
}
ATF_TEST_CASE_BODY(set)
{
    ATF_CHECK(atf::env::has("PATH"));
    const std::string& oldval = atf::env::get("PATH");
    atf::env::set("PATH", "foo-bar");
    ATF_CHECK(atf::env::get("PATH") != oldval);
    ATF_CHECK_EQUAL(atf::env::get("PATH"), "foo-bar");

    ATF_CHECK(!atf::env::has("_UNDEFINED_VARIABLE_"));
    atf::env::set("_UNDEFINED_VARIABLE_", "foo2-bar2");
    ATF_CHECK_EQUAL(atf::env::get("_UNDEFINED_VARIABLE_"), "foo2-bar2");
}

ATF_TEST_CASE(unset);
ATF_TEST_CASE_HEAD(unset)
{
    set("descr", "Tests the unset function");
}
ATF_TEST_CASE_BODY(unset)
{
    ATF_CHECK(atf::env::has("PATH"));
    atf::env::unset("PATH");
    ATF_CHECK(!atf::env::has("PATH"));
}

ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, has_get);
    ATF_ADD_TEST_CASE(tcs, set);
    ATF_ADD_TEST_CASE(tcs, unset);
}
