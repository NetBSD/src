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
#include <unistd.h>
}

#include <fstream>
#include <iostream>

#include "atf-c++/fs.hpp"
#include "atf-c++/macros.hpp"

// ------------------------------------------------------------------------
// Helper tests for "t_config".
// ------------------------------------------------------------------------

ATF_TEST_CASE(config_unset);
ATF_TEST_CASE_HEAD(config_unset)
{
    set_md_var("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_unset)
{
    ATF_CHECK(!has_config_var("test"));
}

ATF_TEST_CASE(config_empty);
ATF_TEST_CASE_HEAD(config_empty)
{
    set_md_var("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_empty)
{
    ATF_CHECK_EQUAL(get_config_var("test"), "");
}

ATF_TEST_CASE(config_value);
ATF_TEST_CASE_HEAD(config_value)
{
    set_md_var("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_value)
{
    ATF_CHECK_EQUAL(get_config_var("test"), "foo");
}

ATF_TEST_CASE(config_multi_value);
ATF_TEST_CASE_HEAD(config_multi_value)
{
    set_md_var("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_multi_value)
{
    ATF_CHECK_EQUAL(get_config_var("test"), "foo bar");
}

// ------------------------------------------------------------------------
// Helper tests for "t_fork".
// ------------------------------------------------------------------------

ATF_TEST_CASE(fork_stop);
ATF_TEST_CASE_HEAD(fork_stop)
{
    set_md_var("descr", "Helper test case for the t_fork test program");
}
ATF_TEST_CASE_BODY(fork_stop)
{
    std::ofstream os(get_config_var("pidfile").c_str());
    os << ::getpid() << std::endl;
    os.close();
    std::cout << "Wrote pid file" << std::endl;
    std::cout << "Waiting for done file" << std::endl;
    while (::access(get_config_var("donefile").c_str(), F_OK) != 0)
        ::usleep(10000);
    std::cout << "Exiting" << std::endl;
}

// ------------------------------------------------------------------------
// Helper tests for "t_meta_data".
// ------------------------------------------------------------------------

ATF_TEST_CASE(metadata_no_descr);
ATF_TEST_CASE_HEAD(metadata_no_descr)
{
}
ATF_TEST_CASE_BODY(metadata_no_descr)
{
}

ATF_TEST_CASE_WITHOUT_HEAD(metadata_no_head);
ATF_TEST_CASE_BODY(metadata_no_head)
{
}

// ------------------------------------------------------------------------
// Helper tests for "t_srcdir".
// ------------------------------------------------------------------------

ATF_TEST_CASE(srcdir_exists);
ATF_TEST_CASE_HEAD(srcdir_exists)
{
    set_md_var("descr", "Helper test case for the t_srcdir test program");
}
ATF_TEST_CASE_BODY(srcdir_exists)
{
    if (!atf::fs::exists(atf::fs::path(get_config_var("srcdir")) /
        "datafile"))
        ATF_FAIL("Cannot find datafile");
}

// ------------------------------------------------------------------------
// Helper tests for "t_status".
// ------------------------------------------------------------------------

ATF_TEST_CASE(status_newlines_fail);
ATF_TEST_CASE_HEAD(status_newlines_fail)
{
    set_md_var("descr", "Helper test case for the t_status test program");
}
ATF_TEST_CASE_BODY(status_newlines_fail)
{
    ATF_FAIL("First line\nSecond line");
}

ATF_TEST_CASE(status_newlines_skip);
ATF_TEST_CASE_HEAD(status_newlines_skip)
{
    set_md_var("descr", "Helper test case for the t_status test program");
}
ATF_TEST_CASE_BODY(status_newlines_skip)
{
    ATF_SKIP("First line\nSecond line");
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add helper tests for t_config.
    ATF_ADD_TEST_CASE(tcs, config_unset);
    ATF_ADD_TEST_CASE(tcs, config_empty);
    ATF_ADD_TEST_CASE(tcs, config_value);
    ATF_ADD_TEST_CASE(tcs, config_multi_value);

    // Add helper tests for t_fork.
    ATF_ADD_TEST_CASE(tcs, fork_stop);

    // Add helper tests for t_meta_data.
    ATF_ADD_TEST_CASE(tcs, metadata_no_descr);
    ATF_ADD_TEST_CASE(tcs, metadata_no_head);

    // Add helper tests for t_srcdir.
    ATF_ADD_TEST_CASE(tcs, srcdir_exists);

    // Add helper tests for t_status.
    ATF_ADD_TEST_CASE(tcs, status_newlines_fail);
    ATF_ADD_TEST_CASE(tcs, status_newlines_skip);
}
