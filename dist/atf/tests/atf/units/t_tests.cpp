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

#include <sstream>

#include <atf.hpp>

// ------------------------------------------------------------------------
// Tests for the "tcr" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(tcr_default_ctor);
ATF_TEST_CASE_HEAD(tcr_default_ctor)
{
    set("descr", "Tests that the default constructor creates a failed "
                 "result.");
}
ATF_TEST_CASE_BODY(tcr_default_ctor)
{
    using atf::tests::tcr;

    tcr tcr;
    ATF_CHECK(tcr.get_status() == tcr::status_failed);
}

ATF_TEST_CASE(tcr_passed_ctor);
ATF_TEST_CASE_HEAD(tcr_passed_ctor)
{
    set("descr", "Tests that the passed pseudo-constructor works.");
}
ATF_TEST_CASE_BODY(tcr_passed_ctor)
{
    using atf::tests::tcr;

    tcr tcr = tcr::passed();
    ATF_CHECK(tcr.get_status() == tcr::status_passed);
}

ATF_TEST_CASE(tcr_skipped_ctor);
ATF_TEST_CASE_HEAD(tcr_skipped_ctor)
{
    set("descr", "Tests that the skipped pseudo-constructor works.");
}
ATF_TEST_CASE_BODY(tcr_skipped_ctor)
{
    using atf::tests::tcr;

    {
        tcr tcr = tcr::skipped("Reason 1");
        ATF_CHECK(tcr.get_status() == tcr::status_skipped);
        ATF_CHECK_EQUAL(tcr.get_reason(), "Reason 1");
    }

    {
        tcr tcr = tcr::skipped("Reason 2");
        ATF_CHECK(tcr.get_status() == tcr::status_skipped);
        ATF_CHECK_EQUAL(tcr.get_reason(), "Reason 2");
    }
}

ATF_TEST_CASE(tcr_failed_ctor);
ATF_TEST_CASE_HEAD(tcr_failed_ctor)
{
    set("descr", "Tests that the failed pseudo-constructor works.");
}
ATF_TEST_CASE_BODY(tcr_failed_ctor)
{
    using atf::tests::tcr;

    {
        tcr tcr = tcr::failed("Reason 1");
        ATF_CHECK(tcr.get_status() == tcr::status_failed);
        ATF_CHECK_EQUAL(tcr.get_reason(), "Reason 1");
    }

    {
        tcr tcr = tcr::failed("Reason 2");
        ATF_CHECK(tcr.get_status() == tcr::status_failed);
        ATF_CHECK_EQUAL(tcr.get_reason(), "Reason 2");
    }
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add tests for the "tcr" class.
    ATF_ADD_TEST_CASE(tcs, tcr_default_ctor);
    ATF_ADD_TEST_CASE(tcs, tcr_passed_ctor);
    ATF_ADD_TEST_CASE(tcs, tcr_skipped_ctor);
    ATF_ADD_TEST_CASE(tcs, tcr_failed_ctor);
}
