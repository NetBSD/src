//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008, 2009, 2010 The NetBSD Foundation, Inc.
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

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "atf-c++/fs.hpp"
#include "atf-c++/macros.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/text.hpp"

#include "test_helpers.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
void
create_ctl_file(const atf::tests::tc& tc, const char *name)
{
    ATF_CHECK(open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644) != -1);
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
        ATF_CHECK_THROW(std::runtime_error, if (1) throw int(5));
    else if (get_config_var("what") == "throw_rt")
        ATF_CHECK_THROW(std::runtime_error,
                        if (1) throw std::runtime_error("e"));
    else if (get_config_var("what") == "no_throw_rt")
        ATF_CHECK_THROW(std::runtime_error,
                        if (0) throw std::runtime_error("e"));

    create_ctl_file(*this, "after");
}

static int
errno_fail_stub(const int raised_errno)
{
    errno = raised_errno;
    return -1;
}

static int
errno_ok_stub(void)
{
    return 0;
}

ATF_TEST_CASE(h_check_errno);
ATF_TEST_CASE_HEAD(h_check_errno)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_check_errno)
{
    create_ctl_file(*this, "before");

    if (get_config_var("what") == "no_error")
        ATF_CHECK_ERRNO(-1, errno_ok_stub() == -1);
    else if (get_config_var("what") == "errno_ok")
        ATF_CHECK_ERRNO(2, errno_fail_stub(2) == -1);
    else if (get_config_var("what") == "errno_fail")
        ATF_CHECK_ERRNO(3, errno_fail_stub(4) == -1);
    else
        UNREACHABLE;

    create_ctl_file(*this, "after");
}

ATF_TEST_CASE(h_require_errno);
ATF_TEST_CASE_HEAD(h_require_errno)
{
    set_md_var("descr", "Helper test case");
}
ATF_TEST_CASE_BODY(h_require_errno)
{
    create_ctl_file(*this, "before");

    if (get_config_var("what") == "no_error")
        ATF_REQUIRE_ERRNO(-1, errno_ok_stub() == -1);
    else if (get_config_var("what") == "errno_ok")
        ATF_REQUIRE_ERRNO(2, errno_fail_stub(2) == -1);
    else if (get_config_var("what") == "errno_fail")
        ATF_REQUIRE_ERRNO(3, errno_fail_stub(4) == -1);
    else
        UNREACHABLE;

    create_ctl_file(*this, "after");
}

// ------------------------------------------------------------------------
// Test cases for the macros.
// ------------------------------------------------------------------------

ATF_TEST_CASE(pass);
ATF_TEST_CASE_HEAD(pass)
{
    set_md_var("descr", "Tests the ATF_PASS macro");
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(pass)
{
    run_h_tc< ATF_TEST_CASE_NAME(h_pass) >();
    ATF_CHECK(grep_file("result", "^passed"));
    ATF_CHECK(atf::fs::exists(atf::fs::path("before")));
    ATF_CHECK(!atf::fs::exists(atf::fs::path("after")));
}

ATF_TEST_CASE(fail);
ATF_TEST_CASE_HEAD(fail)
{
    set_md_var("descr", "Tests the ATF_FAIL macro");
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(fail)
{
    run_h_tc< ATF_TEST_CASE_NAME(h_fail) >();
    ATF_CHECK(grep_file("result", "^failed: Failed on purpose"));
    ATF_CHECK(atf::fs::exists(atf::fs::path("before")));
    ATF_CHECK(!atf::fs::exists(atf::fs::path("after")));
}

ATF_TEST_CASE(skip);
ATF_TEST_CASE_HEAD(skip)
{
    set_md_var("descr", "Tests the ATF_SKIP macro");
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(skip)
{
    run_h_tc< ATF_TEST_CASE_NAME(h_skip) >();
    ATF_CHECK(grep_file("result", "^skipped: Skipped on purpose"));
    ATF_CHECK(atf::fs::exists(atf::fs::path("before")));
    ATF_CHECK(!atf::fs::exists(atf::fs::path("after")));
}

ATF_TEST_CASE(check);
ATF_TEST_CASE_HEAD(check)
{
    set_md_var("descr", "Tests the ATF_CHECK macro");
    set_md_var("use.fs", "true");
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
        atf::tests::vars_map config;
        config["condition"] = t->cond;

        std::cout << "Checking with a " << t->cond << " value" << std::endl;

        run_h_tc< ATF_TEST_CASE_NAME(h_check) >(config);

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(grep_file("result", "^passed"));
            ATF_CHECK(atf::fs::exists(after));
        } else {
            ATF_CHECK(grep_file("result", "^failed: .*condition not met"));
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
    set_md_var("use.fs", "true");
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
        atf::tests::vars_map config;
        config["v1"] = t->v1;
        config["v2"] = t->v2;

        std::cout << "Checking with " << t->v1 << ", " << t->v2
                  << " and expecting " << (t->ok ? "true" : "false")
                  << std::endl;

        run_h_tc< ATF_TEST_CASE_NAME(h_check_equal) >(config);

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(grep_file("result", "^passed"));
            ATF_CHECK(atf::fs::exists(after));
        } else {
            ATF_CHECK(grep_file("result", "^failed: .*v1 != v2"));
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
    set_md_var("use.fs", "true");
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
        atf::tests::vars_map config;
        config["what"] = t->what;

        std::cout << "Checking with " << t->what << " and expecting "
                  << (t->ok ? "true" : "false") << std::endl;

        run_h_tc< ATF_TEST_CASE_NAME(h_check_throw) >(config);

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(grep_file("result", "^passed"));
            ATF_CHECK(atf::fs::exists(after));
        } else {
            std::cout << "Checking that message contains '" << t->msg
                      << "'" << std::endl;
            std::string exp_result = std::string("^failed: .*") + t->msg;
            ATF_CHECK(grep_file("result", exp_result.c_str()));
            ATF_CHECK(!atf::fs::exists(after));
        }

        atf::fs::remove(before);
        if (t->ok)
            atf::fs::remove(after);
    }
}

ATF_TEST_CASE(check_errno);
ATF_TEST_CASE_HEAD(check_errno)
{
    set_md_var("descr", "Tests the ATF_CHECK_ERRNO macro");
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(check_errno)
{
    struct test {
        const char *what;
        bool ok;
        const char *msg;
    } *t, tests[] = {
        { "no_error", false,
          "Expected true value in errno_ok_stub\\(\\) == -1" },
        { "errno_ok", true, NULL },
        { "errno_fail", false,
          "Expected errno 3, got 4, in errno_fail_stub\\(4\\) == -1" },
        { NULL, false, NULL }
    };

    const atf::fs::path before("before");
    const atf::fs::path after("after");

    for (t = &tests[0]; t->what != NULL; t++) {
        atf::tests::vars_map config;
        config["what"] = t->what;

        run_h_tc< ATF_TEST_CASE_NAME(h_check_errno) >(config);

        ATF_CHECK(atf::fs::exists(before));
        ATF_CHECK(atf::fs::exists(after));

        if (t->ok) {
            ATF_CHECK(grep_file("result", "^passed"));
        } else {
            ATF_CHECK(grep_file("result", "^failed"));

            std::string exp_result = "macros_test.cpp:[0-9]+: " +
                std::string(t->msg) + "$";
            ATF_CHECK(grep_file("stderr", exp_result.c_str()));
        }

        atf::fs::remove(before);
        atf::fs::remove(after);
    }
}

ATF_TEST_CASE(require_errno);
ATF_TEST_CASE_HEAD(require_errno)
{
    set_md_var("descr", "Tests the ATF_REQUIRE_ERRNO macro");
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(require_errno)
{
    struct test {
        const char *what;
        bool ok;
        const char *msg;
    } *t, tests[] = {
        { "no_error", false,
          "Expected true value in errno_ok_stub\\(\\) == -1" },
        { "errno_ok", true, NULL },
        { "errno_fail", false,
          "Expected errno 3, got 4, in errno_fail_stub\\(4\\) == -1" },
        { NULL, false, NULL }
    };

    const atf::fs::path before("before");
    const atf::fs::path after("after");

    for (t = &tests[0]; t->what != NULL; t++) {
        atf::tests::vars_map config;
        config["what"] = t->what;

        run_h_tc< ATF_TEST_CASE_NAME(h_require_errno) >(config);

        ATF_CHECK(atf::fs::exists(before));
        if (t->ok) {
            ATF_CHECK(grep_file("result", "^passed"));
            ATF_CHECK(atf::fs::exists(after));
        } else {
            std::string exp_result = "^failed: .*macros_test.cpp:[0-9]+: " +
                std::string(t->msg) + "$";
            ATF_CHECK(grep_file("result", exp_result.c_str()));

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

HEADER_TC(include, "atf-c++/macros.hpp");
BUILD_TC(use, "macros_hpp_test.cpp",
         "Tests that the macros provided by the atf-c++/macros.hpp file "
         "do not cause syntax errors when used",
         "Build of macros_hpp_test.cpp failed; some macros in "
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
    ATF_ADD_TEST_CASE(tcs, check_errno);
    ATF_ADD_TEST_CASE(tcs, require_errno);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
    ATF_ADD_TEST_CASE(tcs, use);
}
