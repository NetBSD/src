//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2010 The NetBSD Foundation, Inc.
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

#include <iostream>
#include <sstream>

#include "atf-c++/formats.hpp"
#include "atf-c++/macros.hpp"
#include "atf-c++/text.hpp"

static
void
print_indented(const std::string& str)
{
    std::vector< std::string > ws = atf::text::split(str, "\n");
    for (std::vector< std::string >::const_iterator iter = ws.begin();
         iter != ws.end(); iter++)
        std::cout << ">>" << *iter << "<<" << std::endl;
}

// XXX Should this string handling and verbosity level be part of the
// ATF_CHECK_EQUAL macro?  It may be hard to predict sometimes that a
// string can have newlines in it, and so the error message generated
// at the moment will be bogus if there are some.
static
void
check_equal(const std::string& str, const std::string& exp)
{
    if (str != exp) {
        std::cout << "String equality check failed." << std::endl
                  << "Adding >> and << to delimit the string boundaries "
                     "below." << std::endl;
        std::cout << "GOT:" << std::endl;
        print_indented(str);
        std::cout << "EXPECTED:" << std::endl;
        print_indented(exp);
        atf_tc_fail("Constructed string differs from the expected one");
    }
}

ATF_TEST_CASE(tp);
ATF_TEST_CASE_HEAD(tp)
{
    set_md_var("descr", "Verifies the application/X-atf-tp writer");
}
ATF_TEST_CASE_BODY(tp)
{
    std::ostringstream expss;
    std::ostringstream ss;

#define RESET \
    expss.str(""); \
    ss.str("")

#define CHECK \
    check_equal(ss.str(), expss.str())

    {
        RESET;

        atf::formats::atf_tp_writer w(ss);
        expss << "Content-Type: application/X-atf-tp; version=\"1\""
              << std::endl << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tp_writer w(ss);
        expss << "Content-Type: application/X-atf-tp; version=\"1\""
              << std::endl << std::endl;
        CHECK;

        w.start_tc("test1");
        expss << "ident: test1" << std::endl;
        CHECK;

        w.end_tc();
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tp_writer w(ss);
        expss << "Content-Type: application/X-atf-tp; version=\"1\""
              << std::endl << std::endl;
        CHECK;

        w.start_tc("test1");
        expss << "ident: test1" << std::endl;
        CHECK;

        w.end_tc();
        CHECK;

        w.start_tc("test2");
        expss << std::endl << "ident: test2" << std::endl;
        CHECK;

        w.end_tc();
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tp_writer w(ss);
        expss << "Content-Type: application/X-atf-tp; version=\"1\""
              << std::endl << std::endl;
        CHECK;

        w.start_tc("test1");
        expss << "ident: test1" << std::endl;
        CHECK;

        w.tc_meta_data("descr", "the description");
        expss << "descr: the description" << std::endl;
        CHECK;

        w.end_tc();
        CHECK;

        w.start_tc("test2");
        expss << std::endl << "ident: test2" << std::endl;
        CHECK;

        w.tc_meta_data("descr", "second test case");
        expss << "descr: second test case" << std::endl;
        CHECK;

        w.tc_meta_data("require.progs", "/bin/cp");
        expss << "require.progs: /bin/cp" << std::endl;
        CHECK;

        w.tc_meta_data("X-custom", "foo bar baz");
        expss << "X-custom: foo bar baz" << std::endl;
        CHECK;

        w.end_tc();
        CHECK;
    }

#undef CHECK
#undef RESET
}

ATF_TEST_CASE(tps);
ATF_TEST_CASE_HEAD(tps)
{
    set_md_var("descr", "Verifies the application/X-atf-tps writer");
}
ATF_TEST_CASE_BODY(tps)
{
    std::ostringstream expss;
    std::ostringstream ss;

#define RESET \
    expss.str(""); \
    ss.str("")

#define CHECK \
    check_equal(ss.str(), expss.str())

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.info("foo", "bar");
        expss << "info: foo, bar" << std::endl;
        CHECK;

        w.info("baz", "second info");
        expss << "info: baz, second info" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(0);
        expss << "tps-count: 0" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(123);
        expss << "tps-count: 123" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(2);
        expss << "tps-count: 2" << std::endl;
        CHECK;

        w.start_tp("foo", 0);
        expss << "tp-start: foo, 0" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: foo" << std::endl;
        CHECK;

        w.start_tp("bar", 0);
        expss << "tp-start: bar, 0" << std::endl;
        CHECK;

        w.end_tp("failed program");
        expss << "tp-end: bar, failed program" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("foo", 1);
        expss << "tp-start: foo, 1" << std::endl;
        CHECK;

        w.start_tc("brokentc");
        expss << "tc-start: brokentc" << std::endl;
        CHECK;

        w.end_tp("aborted");
        expss << "tp-end: foo, aborted" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("thetp", 3);
        expss << "tp-start: thetp, 3" << std::endl;
        CHECK;

        w.start_tc("passtc");
        expss << "tc-start: passtc" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::passed_state));
        expss << "tc-end: passtc, passed" << std::endl;
        CHECK;

        w.start_tc("failtc");
        expss << "tc-start: failtc" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::failed_state,
                                 "The reason"));
        expss << "tc-end: failtc, failed, The reason" << std::endl;
        CHECK;

        w.start_tc("skiptc");
        expss << "tc-start: skiptc" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::skipped_state,
                                 "The reason"));
        expss << "tc-end: skiptc, skipped, The reason" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: thetp" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("thetp", 1);
        expss << "tp-start: thetp, 1" << std::endl;
        CHECK;

        w.start_tc("thetc");
        expss << "tc-start: thetc" << std::endl;
        CHECK;

        w.stdout_tc("a line");
        expss << "tc-so:a line" << std::endl;
        CHECK;

        w.stdout_tc("another line");
        expss << "tc-so:another line" << std::endl;
        CHECK;

        w.stderr_tc("an error message");
        expss << "tc-se:an error message" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::passed_state));
        expss << "tc-end: thetc, passed" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: thetp" << std::endl;
        CHECK;
    }

    {
        RESET;

        atf::formats::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("thetp", 0);
        expss << "tp-start: thetp, 0" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: thetp" << std::endl;
        CHECK;

        w.info("foo", "bar");
        expss << "info: foo, bar" << std::endl;
        CHECK;

        w.info("baz", "second value");
        expss << "info: baz, second value" << std::endl;
        CHECK;
    }

#undef CHECK
#undef RESET
}

ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, tp);
    ATF_ADD_TEST_CASE(tcs, tps);
}
