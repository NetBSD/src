//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>

#include <atf-c++.hpp>

#include "atf-c++/check.hpp"
#include "atf-c++/config.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/text.hpp"
#include "atf-c++/utils.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

extern "C" {
atf_error_t atf_check_result_init(atf_check_result_t *);
}

namespace atf {
namespace check {

check_result
test_constructor(void)
{
    atf_check_result_t r;
    atf_check_result_init(&r);

    return atf::check::check_result(&r);
}

} // namespace check
} // namespace atf

static
atf::check::check_result
do_exec(const atf::tests::tc* tc, const char* helper_name)
{
    const std::string hp = tc->get_config_var("srcdir") + "/../atf-c/h_check";

    atf::utils::auto_array< char > arg0(atf::text::duplicate(hp.c_str()));
    atf::utils::auto_array< char > arg1(atf::text::duplicate(helper_name));

    char *argv[3];
    argv[0] = arg0.get();
    argv[1] = arg1.get();
    argv[2] = NULL;
    printf("Executing %s %s\n", argv[0], argv[1]);
    return atf::check::exec(argv);
}

static
atf::check::check_result
do_exec(const atf::tests::tc* tc, const char* helper_name, const char *carg2)
{
    const std::string hp = tc->get_config_var("srcdir") + "/../atf-c/h_check";

    atf::utils::auto_array< char > arg0(atf::text::duplicate(hp.c_str()));
    atf::utils::auto_array< char > arg1(atf::text::duplicate(helper_name));
    atf::utils::auto_array< char > arg2(atf::text::duplicate(carg2));

    char *argv[4];
    argv[0] = arg0.get();
    argv[1] = arg1.get();
    argv[2] = arg2.get();
    argv[3] = NULL;
    printf("Executing %s %s %s\n", argv[0], argv[1], argv[2]);
    return atf::check::exec(argv);
}

// ------------------------------------------------------------------------
// Tests for the "atf_check_result" type.
// ------------------------------------------------------------------------

ATF_TEST_CASE(result_templates);
ATF_TEST_CASE_HEAD(result_templates)
{
    set_md_var("descr", "Tests that check_result is initialized with "
               "correct temporary file templates");
}
ATF_TEST_CASE_BODY(result_templates)
{
    const atf::check::check_result result1 = atf::check::test_constructor();
    const atf::check::check_result result2 = atf::check::test_constructor();

    const atf::fs::path& out1 = result1.stdout_path();
    const atf::fs::path& err1 = result1.stderr_path();
    const atf::fs::path& out2 = result2.stdout_path();
    const atf::fs::path& err2 = result2.stderr_path();

    ATF_CHECK(out1.str().find("stdout.XXXXXX") != std::string::npos);
    ATF_CHECK(err1.str().find("stderr.XXXXXX") != std::string::npos);
    ATF_CHECK(out2.str().find("stdout.XXXXXX") != std::string::npos);
    ATF_CHECK(err2.str().find("stderr.XXXXXX") != std::string::npos);

    ATF_CHECK(out1 == out2);
    ATF_CHECK(err1 == err2);
}

// ------------------------------------------------------------------------
// Test cases for the free functions.
// ------------------------------------------------------------------------

ATF_TEST_CASE(exec_cleanup);
ATF_TEST_CASE_HEAD(exec_cleanup)
{
    set_md_var("descr", "Tests that exec properly cleans up the temporary "
               "files it creates");
}
ATF_TEST_CASE_BODY(exec_cleanup)
{
    std::auto_ptr< atf::fs::path > out;
    std::auto_ptr< atf::fs::path > err;

    {
        const atf::check::check_result r = do_exec(this, "exit-success");
        out.reset(new atf::fs::path(r.stdout_path()));
        err.reset(new atf::fs::path(r.stderr_path()));
        ATF_CHECK(atf::fs::exists(*out.get()));
        ATF_CHECK(atf::fs::exists(*err.get()));
    }
    ATF_CHECK(!atf::fs::exists(*out.get()));
    ATF_CHECK(!atf::fs::exists(*err.get()));
}

ATF_TEST_CASE(exec_exitstatus);
ATF_TEST_CASE_HEAD(exec_exitstatus)
{
    set_md_var("descr", "Tests that exec properly captures the exit "
               "status of the executed command");
}
ATF_TEST_CASE_BODY(exec_exitstatus)
{
    {
        atf::check::check_result r = do_exec(this, "exit-success");
        ATF_CHECK(r.exited());
        ATF_CHECK_EQUAL(r.exitcode(), EXIT_SUCCESS);
    }

    {
        atf::check::check_result r = do_exec(this, "exit-failure");
        ATF_CHECK(r.exited());
        ATF_CHECK_EQUAL(r.exitcode(), EXIT_FAILURE);
    }

    {
        atf::check::check_result r = do_exec(this, "exit-signal");
        ATF_CHECK(!r.exited());
    }
}

static
void
check_lines(const atf::fs::path& path, const char* outname,
            const char* resname)
{
    std::ifstream f(path.c_str());
    ATF_CHECK(f);

    std::string line;
    std::getline(f, line);
    ATF_CHECK_EQUAL(line, std::string("Line 1 to ") + outname + " for " +
                    resname);
    std::getline(f, line);
    ATF_CHECK_EQUAL(line, std::string("Line 2 to ") + outname + " for " +
                    resname);
}

ATF_TEST_CASE(exec_stdout_stderr);
ATF_TEST_CASE_HEAD(exec_stdout_stderr)
{
    set_md_var("descr", "Tests that exec properly captures the stdout "
               "and stderr streams of the child process");
}
ATF_TEST_CASE_BODY(exec_stdout_stderr)
{
    const atf::check::check_result r1 = do_exec(this, "stdout-stderr",
                                                "result1");
    ATF_CHECK(r1.exited());
    ATF_CHECK_EQUAL(r1.exitcode(), EXIT_SUCCESS);

    const atf::check::check_result r2 = do_exec(this, "stdout-stderr",
                                                "result2");
    ATF_CHECK(r2.exited());
    ATF_CHECK_EQUAL(r2.exitcode(), EXIT_SUCCESS);

    const atf::fs::path& out1 = r1.stdout_path();
    const atf::fs::path& out2 = r2.stdout_path();
    const atf::fs::path& err1 = r1.stderr_path();
    const atf::fs::path& err2 = r2.stderr_path();

    ATF_CHECK(out1.str().find("stdout.XXXXXX") == std::string::npos);
    ATF_CHECK(out2.str().find("stdout.XXXXXX") == std::string::npos);
    ATF_CHECK(err1.str().find("stderr.XXXXXX") == std::string::npos);
    ATF_CHECK(err2.str().find("stderr.XXXXXX") == std::string::npos);

    ATF_CHECK(out1 != out2);
    ATF_CHECK(err1 != err2);

    check_lines(out1, "stdout", "result1");
    check_lines(out2, "stdout", "result2");
    check_lines(err1, "stderr", "result1");
    check_lines(err2, "stderr", "result2");
}

ATF_TEST_CASE(exec_unknown);
ATF_TEST_CASE_HEAD(exec_unknown)
{
    set_md_var("descr", "Tests that running a non-existing binary "
               "is handled correctly");
}
ATF_TEST_CASE_BODY(exec_unknown)
{
    const std::string path = atf::config::get("atf_workdir") + "/non-existent";
    atf::utils::auto_array< char > arg(atf::text::duplicate(path.c_str()));

    char* argv[2];
    argv[0] = arg.get();
    argv[1] = NULL;

    const atf::check::check_result r = atf::check::exec(argv);
    ATF_CHECK(r.exited());
    ATF_CHECK_EQUAL(r.exitcode(), 127);
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, result_templates);
    ATF_ADD_TEST_CASE(tcs, exec_cleanup);
    ATF_ADD_TEST_CASE(tcs, exec_exitstatus);
    ATF_ADD_TEST_CASE(tcs, exec_stdout_stderr);
    ATF_ADD_TEST_CASE(tcs, exec_unknown);
}
