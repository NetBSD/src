//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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

#include <sstream>

#include <atf-c++.hpp>

#include "h_lib.hpp"

// ------------------------------------------------------------------------
// Tests for the "tcr" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(tcr_ctor_wo_reason);
ATF_TEST_CASE_HEAD(tcr_ctor_wo_reason)
{
    set_md_var("descr", "Tests that the passed pseudo-constructor works.");
}
ATF_TEST_CASE_BODY(tcr_ctor_wo_reason)
{
    using atf::tests::tcr;

    tcr tcr(tcr::passed_state);
    ATF_CHECK(tcr.get_state() == tcr::passed_state);
}

ATF_TEST_CASE(tcr_ctor_w_reason);
ATF_TEST_CASE_HEAD(tcr_ctor_w_reason)
{
    set_md_var("descr", "Tests that the skipped pseudo-constructor works.");
}
ATF_TEST_CASE_BODY(tcr_ctor_w_reason)
{
    using atf::tests::tcr;

    {
        tcr t(tcr::skipped_state, "Reason 1");
        ATF_CHECK(t.get_state() == tcr::skipped_state);
        ATF_CHECK_EQUAL(t.get_reason(), "Reason 1");
    }

    {
        tcr t(tcr::skipped_state, "Reason 2");
        ATF_CHECK(t.get_state() == tcr::skipped_state);
        ATF_CHECK_EQUAL(t.get_reason(), "Reason 2");
    }

    {
        tcr t(tcr::failed_state, "Reason 1");
        ATF_CHECK(t.get_state() == tcr::failed_state);
        ATF_CHECK_EQUAL(t.get_reason(), "Reason 1");
    }

    {
        tcr t(tcr::failed_state, "Reason 2");
        ATF_CHECK(t.get_state() == tcr::failed_state);
        ATF_CHECK_EQUAL(t.get_reason(), "Reason 2");
    }
}

// ------------------------------------------------------------------------
// Tests cases for the header file.
// ------------------------------------------------------------------------

HEADER_TC(include, "atf-c++/tests.hpp", "d_include_tests_hpp.cpp");

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add tests for the "tcr" class.
    ATF_ADD_TEST_CASE(tcs, tcr_ctor_wo_reason);
    ATF_ADD_TEST_CASE(tcs, tcr_ctor_w_reason);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
}
