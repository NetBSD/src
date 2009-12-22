//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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
#include <fcntl.h>
#include <unistd.h>
}

#include <iostream>
#include <stdexcept>

#include "atf-c++/fs.hpp"
#include "atf-c++/macros.hpp"
#include "atf-c++/text.hpp"

#include "h_lib.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
void
create_ctl_file(const atf::tests::tc& tc, const char *name)
{
    atf::fs::path p = atf::fs::path(tc.get_config_var("ctldir")) / name;
    ATF_CHECK(open(p.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644) != -1);
}

static
atf::tests::vars_map
init_config(void)
{
    atf::tests::vars_map config;
    config["ctldir"] = atf::fs::get_current_dir().str();
    return config;
}

// ------------------------------------------------------------------------
// Auxiliary test cases.
// ------------------------------------------------------------------------

ATF_TEST_CASE(h_pass);
ATF_TEST_CASE_HEAD(h_pass)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_pass)
{
    create_ctl_file(*this, "before");
    ATF_PASS();
    create_ctl_file(*this, "after");
}

ATF_TEST_CASE(h_fail);
ATF_TEST_CASE_HEAD(h_fail)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_fail)
{
    create_ctl_file(*this, "before");
    ATF_FAIL("Failed on purpose");
    create_ctl_file(*this, "after");
}

ATF_TEST_CASE(h_skip);
ATF_TEST_CASE_HEAD(h_skip)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_skip)
{
    create_ctl_file(*this, "before");
    ATF_SKIP("Skipped on purpose");
    create_ctl_file(*this, "after");
}

ATF_TEST_CASE(h_check);
ATF_TEST_CASE_HEAD(h_check)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_check)
{
    bool condition = atf::text::to_bool(get_config_var("condition"));

    create_ctl_file(*this, "before");
    ATF_CHECK(condition);
    create_ctl_file(*this, "after");
}

ATF_TEST_CASE(h_check_equal);
ATF_TEST_CASE_HEAD(h_check_equal)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_check_equal)
{
    long v1 = atf::text::to_type< long >(get_config_var("v1"));
    long v2 = atf::text::to_type< long >(get_config_var("v2"));

    create_ctl_file(*this, "before");
    ATF_CHECK_EQUAL(v1, v2);
    create_ctl_file(*this, "after");
}

ATF_TEST_CASE(h_check_throw);
ATF_TEST_CASE_HEAD(h_check_throw)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_check_throw)
{
    create_ctl_file(*this, "before");

    if (get_config_var("what") == "throw_int")
        ATF_CHECK_THROW(if (1) throw int(5),
                        std::runtime_error);
    else if (get_config_var("what") == "throw_rt")
        ATF_CHECK_THROW(if (1) throw std::runtime_error("e"),
                        std::runtime_error);
    else if (get_config_var("what") == "no_throw_rt")
        ATF_CHECK_THROW(if (0) throw std::runtime_error("e"),
                        std::runtime_error);

    create_ctl_file(*this, "after");
}

// ------------------------------------------------------------------------
// Test cases for the macros.
// ------------------------------------------------------------------------

ATF_TEST_CASE(pass);
ATF_TEST_CASE_HEAD(pass)
{
    set_md_var("descr", "Tests the ATF_PASS macro");
}
ATF_TEST_CASE_BODY(pass)
{
    ATF_TEST_CASE_NAME(h_pass) tcaux;
    tcaux.init(init_config());
    atf::tests::tcr tcr =
        tcaux.run(STDOUT_FILENO, STDERR_FILENO, atf::fs::get_current_dir());

    ATF_CHECK(tcr.get_state() == atf::tests::tcr::passed_state);
    ATF_CHECK(atf::fs::exists(atf::fs::path("before")));
    ATF_CHECK(!atf::fs::exists(atf::fs::path("after")));
}

ATF_TEST_CASE(fail);
ATF_TEST_CASE_HEAD(fail)
{
    set_md_var("descr", "Tests the ATF_FAIL macro");
}
ATF_TEST_CASE_BODY(fail)
{
    ATF_TEST_CASE_NAME(h_fail) tcaux;
    tcaux.init(init_config());
    atf::tests::tcr tcr =
        tcaux.run(STDOUT_FILENO, STDERR_FILENO, atf::fs::get_current_dir());

    ATF_CHECK(tcr.get_state() == atf::tests::tcr::failed_state);
    ATF_CHECK(tcr.get_reason() == "Failed on purpose");
    ATF_CHECK(atf::fs::exists(atf::fs::path("before")));
    ATF_CHECK(!atf::fs::exists(atf::fs::path("after")));
}

ATF_TEST_CASE(skip);
ATF_TEST_CASE_HEAD(skip)
{
    set_md_var("descr", "Tests the ATF_SKIP macro");
}
ATF_TEST_CASE_BODY(skip)
{
    ATF_TEST_CASE_NAME(h_skip) tcaux;
    tcaux.init(init_config());
    atf::tests::tcr tcr =
        tcaux.run(STDOUT_FILENO, STDERR_FILENO, atf::fs::get_current_dir());

    ATF_CHECK(tcr.get_state() == atf::tests::tcr::skipped_state);
    ATF_CHECK(tcr.get_reason() == "Skipped on purpose");
    ATF_CHECK(atf::fs::exists(atf::fs::path("before")));
    ATF_CHECK(!atf::fs::exists(atf::fs::path("after")));
}

ATF_TEST_CASE(check);
ATF_TEST_CASE_HEAD(check)
{
    set_md_var("descr", "Tests the ATF_CHECK macro");
}
ATF_TEST_CASE_BODY(check)
{
    struct test {
        const char *cond;
        bool ok;
    } *t, tests[] = {
        { "false", false },
        { "true", true },
        { NULL, false }
    };

    const atf::fs::path before("before");
    const atf::fs::path after("after");

    for (t = &tests[0]; t->cond != NULL; t++) {
        atf::tests::vars_map config = init_config();
        config["condition"] = t->cond;

        std::cout << "Checking with a " << t->cond << " value" << std::endl;

        ATF_TEST_CASE_NAME(h_check) tcaux;
        tcaux.init(config);
        atf::tests::tcr tcr = tcaux.run(STDOUT_FILENO, STDERR_FILENO,
                                        atf::fs::get_current_dir());

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(tcr.get_state() == atf::tests::tcr::passed_state);
            ATF_CHECK(atf::fs::exists(after));
        } else {
            ATF_CHECK(tcr.get_state() == atf::tests::tcr::failed_state);
            ATF_CHECK(tcr.get_reason().find("condition not met") !=
                      std::string::npos);
            ATF_CHECK(!atf::fs::exists(after));
        }

        atf::fs::remove(before);
        if (t->ok)
            atf::fs::remove(after);
    }
}

ATF_TEST_CASE(check_equal);
ATF_TEST_CASE_HEAD(check_equal)
{
    set_md_var("descr", "Tests the ATF_CHECK_EQUAL macro");
}
ATF_TEST_CASE_BODY(check_equal)
{
    struct test {
        const char *v1;
        const char *v2;
        bool ok;
    } *t, tests[] = {
        { "1", "1", true },
        { "1", "2", false },
        { "2", "1", false },
        { "2", "2", true },
        { NULL, NULL, false }
    };

    const atf::fs::path before("before");
    const atf::fs::path after("after");

    for (t = &tests[0]; t->v1 != NULL; t++) {
        atf::tests::vars_map config = init_config();
        config["v1"] = t->v1;
        config["v2"] = t->v2;

        std::cout << "Checking with " << t->v1 << ", " << t->v2
                  << " and expecting " << (t->ok ? "true" : "false")
                  << std::endl;

        ATF_TEST_CASE_NAME(h_check_equal) tcaux;
        tcaux.init(config);
        atf::tests::tcr tcr = tcaux.run(STDOUT_FILENO, STDERR_FILENO,
                                        atf::fs::get_current_dir());

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(tcr.get_state() == atf::tests::tcr::passed_state);
            ATF_CHECK(atf::fs::exists(after));
        } else {
            ATF_CHECK(tcr.get_state() == atf::tests::tcr::failed_state);
            ATF_CHECK(tcr.get_reason().find("v1 != v2") !=
                      std::string::npos);
            ATF_CHECK(!atf::fs::exists(after));
        }

        atf::fs::remove(before);
        if (t->ok)
            atf::fs::remove(after);
    }
}

ATF_TEST_CASE(check_throw);
ATF_TEST_CASE_HEAD(check_throw)
{
    set_md_var("descr", "Tests the ATF_CHECK_THROW macro");
}
ATF_TEST_CASE_BODY(check_throw)
{
    struct test {
        const char *what;
        bool ok;
        const char *msg;
    } *t, tests[] = {
        { "throw_int", false, "unexpected error" },
        { "throw_rt", true, NULL },
        { "no_throw_rt", false, "did not throw" },
        { NULL, false, NULL }
    };

    const atf::fs::path before("before");
    const atf::fs::path after("after");

    for (t = &tests[0]; t->what != NULL; t++) {
        atf::tests::vars_map config = init_config();
        config["what"] = t->what;

        std::cout << "Checking with " << t->what << " and expecting "
                  << (t->ok ? "true" : "false") << std::endl;

        ATF_TEST_CASE_NAME(h_check_throw) tcaux;
        tcaux.init(config);
        atf::tests::tcr tcr = tcaux.run(STDOUT_FILENO, STDERR_FILENO,
                                        atf::fs::get_current_dir());

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(tcr.get_state() == atf::tests::tcr::passed_state);
            ATF_CHECK(atf::fs::exists(after));
        } else {
            ATF_CHECK(tcr.get_state() == atf::tests::tcr::failed_state);
            std::cout << "Checking that message contains '" << t->msg
                      << "'" << std::endl;
            ATF_CHECK(tcr.get_reason().find(t->msg) != std::string::npos);
            ATF_CHECK(!atf::fs::exists(after));
        }

        atf::fs::remove(before);
        if (t->ok)
            atf::fs::remove(after);
    }
}

// ------------------------------------------------------------------------
// Tests cases for the header file.
// ------------------------------------------------------------------------

HEADER_TC(include, "atf-c++/macros.hpp", "d_include_macros_hpp.cpp");
BUILD_TC(use, "d_use_macros_hpp.cpp",
         "Tests that the macros provided by the atf-c++/macros.hpp file "
         "do not cause syntax errors when used",
         "Build of d_use_macros_hpp.cpp failed; some macros in "
         "atf-c++/macros.hpp are broken");

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add the test cases for the macros.
    ATF_ADD_TEST_CASE(tcs, pass);
    ATF_ADD_TEST_CASE(tcs, fail);
    ATF_ADD_TEST_CASE(tcs, skip);
    ATF_ADD_TEST_CASE(tcs, check);
    ATF_ADD_TEST_CASE(tcs, check_equal);
    ATF_ADD_TEST_CASE(tcs, check_throw);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
    ATF_ADD_TEST_CASE(tcs, use);
}
