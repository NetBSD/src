//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009, 2010 The NetBSD Foundation, Inc.
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

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
}

#include <fstream>
#include <sstream>

#include <atf-c++.hpp>

#include "atf-c++/formats.hpp"
#include "atf-c++/parser.hpp"
#include "atf-c++/user.hpp"

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

ATF_TEST_CASE(tcr_read_passed);
ATF_TEST_CASE_HEAD(tcr_read_passed)
{
    set_md_var("descr", "Tests reading a passed test case result");
}
ATF_TEST_CASE_BODY(tcr_read_passed)
{
    using atf::tests::tcr;

    std::ofstream os("tcr.txt");
    ATF_CHECK(os);
    os << "Content-Type: application/X-atf-tcr; version=\"1\"\n\n";
    os << "result: passed\n";
    os.close();

    const tcr result = tcr::read(atf::fs::path("tcr.txt"));
    ATF_CHECK_EQUAL(tcr::passed_state, result.get_state());
}

ATF_TEST_CASE(tcr_read_failed);
ATF_TEST_CASE_HEAD(tcr_read_failed)
{
    set_md_var("descr", "Tests reading a failed test case result");
}
ATF_TEST_CASE_BODY(tcr_read_failed)
{
    using atf::tests::tcr;

    std::ofstream os("tcr.txt");
    ATF_CHECK(os);
    os << "Content-Type: application/X-atf-tcr; version=\"1\"\n\n";
    os << "result: failed\n";
    os << "reason: A test reason\n";
    os.close();

    const tcr result = tcr::read(atf::fs::path("tcr.txt"));
    ATF_CHECK_EQUAL(tcr::failed_state, result.get_state());
    ATF_CHECK_EQUAL("A test reason", result.get_reason());
}

ATF_TEST_CASE(tcr_read_skipped);
ATF_TEST_CASE_HEAD(tcr_read_skipped)
{
    set_md_var("descr", "Tests reading a skipped test case result");
}
ATF_TEST_CASE_BODY(tcr_read_skipped)
{
    using atf::tests::tcr;

    std::ofstream os("tcr.txt");
    ATF_CHECK(os);
    os << "Content-Type: application/X-atf-tcr; version=\"1\"\n\n";
    os << "result: skipped\n";
    os << "reason: Another test reason\n";
    os.close();

    const tcr result = tcr::read(atf::fs::path("tcr.txt"));
    ATF_CHECK_EQUAL(tcr::skipped_state, result.get_state());
    ATF_CHECK_EQUAL("Another test reason", result.get_reason());
}

ATF_TEST_CASE(tcr_read_missing_file_error);
ATF_TEST_CASE_HEAD(tcr_read_missing_file_error)
{
    set_md_var("descr", "Tests reading a test case result from a non-existent "
               "file");
}
ATF_TEST_CASE_BODY(tcr_read_missing_file_error)
{
    using atf::tests::tcr;

    ATF_CHECK_THROW(tcr::read(atf::fs::path("tcr.txt")), std::runtime_error);
}

ATF_TEST_CASE(tcr_read_format_error);
ATF_TEST_CASE_HEAD(tcr_read_format_error)
{
    set_md_var("descr", "Tests reading a test case result with a format error");
}
ATF_TEST_CASE_BODY(tcr_read_format_error)
{
    using atf::tests::tcr;

    std::ofstream os("tcr.txt");
    ATF_CHECK(os);
    os << "Content-Type: application/X-atf-tcr; version=\"50\"\n\n";
    os << "the body\n";
    os.close();

    ATF_CHECK_THROW(tcr::read(atf::fs::path("tcr.txt")),
                    atf::formats::format_error);
}

ATF_TEST_CASE(tcr_read_parse_error);
ATF_TEST_CASE_HEAD(tcr_read_parse_error)
{
    set_md_var("descr", "Tests reading a test case result with a parse error");
}
ATF_TEST_CASE_BODY(tcr_read_parse_error)
{
    using atf::tests::tcr;

    std::ofstream os("tcr.txt");
    ATF_CHECK(os);
    os << "Content-Type: application/X-atf-tcr; version=\"1\"\n\n";
    os << "result: foo\n";
    os.close();

    ATF_CHECK_THROW(tcr::read(atf::fs::path("tcr.txt")),
                    std::vector< atf::parser::parse_error >);
}

// TODO: The tests below suggest that we'd benefit from a generic macro
// to check for differences in generated files and golden contents.

ATF_TEST_CASE(tcr_write_passed);
ATF_TEST_CASE_HEAD(tcr_write_passed)
{
    set_md_var("descr", "Tests writing a passed test case result");
}
ATF_TEST_CASE_BODY(tcr_write_passed)
{
    using atf::tests::tcr;

    tcr result(tcr::passed_state);
    result.write(atf::fs::path("tcr.txt"));

    std::ifstream is("tcr.txt");
    ATF_CHECK(is);
    std::string line;
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("Content-Type: application/X-atf-tcr; version=\"1\"", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("result: passed", line);
    ATF_CHECK(!std::getline(is, line).good());
}

ATF_TEST_CASE(tcr_write_failed);
ATF_TEST_CASE_HEAD(tcr_write_failed)
{
    set_md_var("descr", "Tests writing a failed test case result");
}
ATF_TEST_CASE_BODY(tcr_write_failed)
{
    using atf::tests::tcr;

    tcr result(tcr::failed_state, "A test reason");
    result.write(atf::fs::path("tcr.txt"));

    std::ifstream is("tcr.txt");
    ATF_CHECK(is);
    std::string line;
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("Content-Type: application/X-atf-tcr; version=\"1\"", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("result: failed", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("reason: A test reason", line);
    ATF_CHECK(!std::getline(is, line).good());
}

ATF_TEST_CASE(tcr_write_skipped);
ATF_TEST_CASE_HEAD(tcr_write_skipped)
{
    set_md_var("descr", "Tests writing a skipped test case result");
}
ATF_TEST_CASE_BODY(tcr_write_skipped)
{
    using atf::tests::tcr;

    tcr result(tcr::skipped_state, "Another test reason");
    result.write(atf::fs::path("tcr.txt"));

    std::ifstream is("tcr.txt");
    ATF_CHECK(is);
    std::string line;
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("Content-Type: application/X-atf-tcr; version=\"1\"", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("result: skipped", line);
    ATF_CHECK(std::getline(is, line).good());
    ATF_CHECK_EQUAL("reason: Another test reason", line);
    ATF_CHECK(!std::getline(is, line).good());
}

ATF_TEST_CASE(tcr_write_create_file_error);
ATF_TEST_CASE_HEAD(tcr_write_create_file_error)
{
    set_md_var("descr", "Tests writing a test case result to an unwritable "
               "file");
}
ATF_TEST_CASE_BODY(tcr_write_create_file_error)
{
    using atf::tests::tcr;

    const int fd = ::open("tcr.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    ATF_CHECK(fd != -1);
    ATF_CHECK(::close(fd) != -1);
    ATF_CHECK(::chmod("tcr.txt", 0) != -1);

    tcr result(tcr::passed_state);
    if (atf::user::is_root()) {
        result.write(atf::fs::path("tcr.txt"));
    } else {
        ATF_CHECK_THROW(result.write(atf::fs::path("tcr.txt")),
                        std::runtime_error);
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
    ATF_ADD_TEST_CASE(tcs, tcr_read_passed);
    ATF_ADD_TEST_CASE(tcs, tcr_read_failed);
    ATF_ADD_TEST_CASE(tcs, tcr_read_skipped);
    ATF_ADD_TEST_CASE(tcs, tcr_read_missing_file_error);
    ATF_ADD_TEST_CASE(tcs, tcr_read_format_error);
    ATF_ADD_TEST_CASE(tcs, tcr_read_parse_error);
    ATF_ADD_TEST_CASE(tcs, tcr_write_passed);
    ATF_ADD_TEST_CASE(tcs, tcr_write_failed);
    ATF_ADD_TEST_CASE(tcs, tcr_write_skipped);
    ATF_ADD_TEST_CASE(tcs, tcr_write_create_file_error);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
}
