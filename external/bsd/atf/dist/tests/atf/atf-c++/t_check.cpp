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

extern "C" {
#include <fcntl.h>
#include <regex.h>
#include <unistd.h>
}

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include <atf-c++.hpp>

#include "atf-c++/check.hpp"
#include "atf-c++/config.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/text.hpp"
#include "atf-c++/utils.hpp"

#include "h_lib.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
atf::check::check_result
do_exec(const atf::tests::tc* tc, const char* helper_name)
{
    std::vector< std::string > argv;
    argv.push_back(tc->get_config_var("srcdir") + "/../atf-c/h_check");
    argv.push_back(helper_name);
    std::cout << "Executing " << argv[0] << " " << argv[1] << "\n";

    atf::process::argv_array argva(argv);
    return atf::check::exec(argva);
}

static
atf::check::check_result
do_exec(const atf::tests::tc* tc, const char* helper_name, const char *carg2)
{
    std::vector< std::string > argv;
    argv.push_back(tc->get_config_var("srcdir") + "/../atf-c/h_check");
    argv.push_back(helper_name);
    argv.push_back(carg2);
    std::cout << "Executing " << argv[0] << " " << argv[1] << " "
              << argv[2] << "\n";

    atf::process::argv_array argva(argv);
    return atf::check::exec(argva);
}

static
bool
grep_string(const std::string& str, const char* regex)
{
    int res;
    regex_t preg;

    std::cout << "Looking for '" << regex << "' in '" << str << "'\n";
    ATF_CHECK(::regcomp(&preg, regex, REG_EXTENDED) == 0);

    res = ::regexec(&preg, str.c_str(), 0, NULL, 0);
    ATF_CHECK(res == 0 || res == REG_NOMATCH);

    ::regfree(&preg);

    return res == 0;
}

static
bool
grep_file(const char* name, const char* regex)
{
    std::ifstream is(name);
    ATF_CHECK(is);

    bool found = false;

    std::string line;
    std::getline(is, line);
    while (!found && is.good()) {
        if (grep_string(line, regex))
            found = true;
        else
            std::getline(is, line);
    }

    return found;
}

// ------------------------------------------------------------------------
// Helper test cases for the free functions.
// ------------------------------------------------------------------------

static
void
run_h_tc(atf::tests::tc& tc, const char* outname, const char* errname)
{
    const int fdout = ::open(outname, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    ATF_CHECK(fdout != -1);

    const int fderr = ::open(errname, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    ATF_CHECK(fderr != -1);

    atf::tests::vars_map config;
    config["callerdir"] = atf::fs::get_current_dir().str();

    tc.init(config);
    const atf::tests::tcr tcr =
        tc.run(fdout, fderr, atf::fs::get_current_dir());

    ATF_CHECK_EQUAL(tcr.get_state(), atf::tests::tcr::passed_state);

    ::close(fderr);
    ::close(fdout);
}

ATF_TEST_CASE(h_build_c_o_ok);
ATF_TEST_CASE_HEAD(h_build_c_o_ok)
{
    set_md_var("descr", "Helper test case for build_c_o");
}
ATF_TEST_CASE_BODY(h_build_c_o_ok)
{
    std::ofstream sfile("test.c");
    sfile << "#include <stdio.h>\n";
    sfile.close();

    ATF_CHECK(atf::check::build_c_o(atf::fs::path("test.c"),
                                    atf::fs::path("test.o"),
                                    atf::process::argv_array()));
}

ATF_TEST_CASE(h_build_c_o_fail);
ATF_TEST_CASE_HEAD(h_build_c_o_fail)
{
    set_md_var("descr", "Helper test case for build_c_o");
}
ATF_TEST_CASE_BODY(h_build_c_o_fail)
{
    std::ofstream sfile("test.c");
    sfile << "void foo(void) { int a = UNDEFINED_SYMBOL; }\n";
    sfile.close();

    ATF_CHECK(!atf::check::build_c_o(atf::fs::path("test.c"),
                                     atf::fs::path("test.o"),
                                     atf::process::argv_array()));
}

ATF_TEST_CASE(h_build_cpp_ok);
ATF_TEST_CASE_HEAD(h_build_cpp_ok)
{
    set_md_var("descr", "Helper test case for build_cpp");
}
ATF_TEST_CASE_BODY(h_build_cpp_ok)
{
    std::ofstream sfile("test.c");
    sfile << "#define A foo\n";
    sfile << "#define B bar\n";
    sfile << "A B\n";
    sfile.close();

    const atf::fs::path testp =
        atf::fs::path(get_config_var("callerdir")) / "test.p";

    ATF_CHECK(atf::check::build_cpp(atf::fs::path("test.c"), testp,
                                    atf::process::argv_array()));
}

ATF_TEST_CASE(h_build_cpp_fail);
ATF_TEST_CASE_HEAD(h_build_cpp_fail)
{
    set_md_var("descr", "Helper test case for build_cpp");
}
ATF_TEST_CASE_BODY(h_build_cpp_fail)
{
    std::ofstream sfile("test.c");
    sfile << "#include \"./non-existent.h\"\n";
    sfile.close();

    ATF_CHECK(!atf::check::build_cpp(atf::fs::path("test.c"),
                                     atf::fs::path("test.p"),
                                     atf::process::argv_array()));
}

ATF_TEST_CASE(h_build_cxx_o_ok);
ATF_TEST_CASE_HEAD(h_build_cxx_o_ok)
{
    set_md_var("descr", "Helper test case for build_cxx_o");
}
ATF_TEST_CASE_BODY(h_build_cxx_o_ok)
{
    std::ofstream sfile("test.cpp");
    sfile << "#include <iostream>\n";
    sfile.close();

    ATF_CHECK(atf::check::build_cxx_o(atf::fs::path("test.cpp"),
                                      atf::fs::path("test.o"),
                                      atf::process::argv_array()));
}

ATF_TEST_CASE(h_build_cxx_o_fail);
ATF_TEST_CASE_HEAD(h_build_cxx_o_fail)
{
    set_md_var("descr", "Helper test case for build_cxx_o");
}
ATF_TEST_CASE_BODY(h_build_cxx_o_fail)
{
    std::ofstream sfile("test.cpp");
    sfile << "void foo(void) { int a = UNDEFINED_SYMBOL; }\n";
    sfile.close();

    ATF_CHECK(!atf::check::build_cxx_o(atf::fs::path("test.cpp"),
                                       atf::fs::path("test.o"),
                                       atf::process::argv_array()));
}

// ------------------------------------------------------------------------
// Test cases for the free functions.
// ------------------------------------------------------------------------

ATF_TEST_CASE(build_c_o);
ATF_TEST_CASE_HEAD(build_c_o)
{
    set_md_var("descr", "Tests the build_c_o function");
}
ATF_TEST_CASE_BODY(build_c_o)
{
    ATF_TEST_CASE_NAME(h_build_c_o_ok) h_build_c_o_ok;
    run_h_tc(h_build_c_o_ok, "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.c"));

    ATF_TEST_CASE_NAME(h_build_c_o_fail) h_build_c_o_fail;
    run_h_tc(h_build_c_o_fail, "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.c"));
    ATF_CHECK(grep_file("stderr", "test.c"));
    ATF_CHECK(grep_file("stderr", "UNDEFINED_SYMBOL"));
}

ATF_TEST_CASE(build_cpp);
ATF_TEST_CASE_HEAD(build_cpp)
{
    set_md_var("descr", "Tests the build_cpp function");
}
ATF_TEST_CASE_BODY(build_cpp)
{
    ATF_TEST_CASE_NAME(h_build_cpp_ok) h_build_cpp_ok;
    run_h_tc(h_build_cpp_ok, "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o.*test.p"));
    ATF_CHECK(grep_file("stdout", "test.c"));
    ATF_CHECK(grep_file("test.p", "foo bar"));

    ATF_TEST_CASE_NAME(h_build_cpp_fail) h_build_cpp_fail;
    run_h_tc(h_build_cpp_fail, "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.p"));
    ATF_CHECK(grep_file("stdout", "test.c"));
    ATF_CHECK(grep_file("stderr", "test.c"));
    ATF_CHECK(grep_file("stderr", "non-existent.h"));
}

ATF_TEST_CASE(build_cxx_o);
ATF_TEST_CASE_HEAD(build_cxx_o)
{
    set_md_var("descr", "Tests the build_cxx_o function");
}
ATF_TEST_CASE_BODY(build_cxx_o)
{
    ATF_TEST_CASE_NAME(h_build_cxx_o_ok) h_build_cxx_o_ok;
    run_h_tc(h_build_cxx_o_ok, "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.cpp"));

    ATF_TEST_CASE_NAME(h_build_cxx_o_fail) h_build_cxx_o_fail;
    run_h_tc(h_build_cxx_o_fail, "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.cpp"));
    ATF_CHECK(grep_file("stderr", "test.cpp"));
    ATF_CHECK(grep_file("stderr", "UNDEFINED_SYMBOL"));
}

ATF_TEST_CASE(exec_argv);
ATF_TEST_CASE_HEAD(exec_argv)
{
    set_md_var("descr", "Tests that exec preserves the provided argv");
}
ATF_TEST_CASE_BODY(exec_argv)
{
    const atf::check::check_result r = do_exec(this, "exit-success");
    ATF_CHECK_EQUAL(r.argv().size(), 2);
    ATF_CHECK_EQUAL(r.argv()[0],
                    get_config_var("srcdir") + "/../atf-c/h_check");
    ATF_CHECK_EQUAL(r.argv()[1], "exit-success");
}

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

    ATF_CHECK(out1.str().find("stdout.") != std::string::npos);
    ATF_CHECK(out2.str().find("stdout.") != std::string::npos);
    ATF_CHECK(err1.str().find("stderr.") != std::string::npos);
    ATF_CHECK(err2.str().find("stderr.") != std::string::npos);

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
    std::vector< std::string > argv;
    argv.push_back(atf::config::get("atf_workdir") + "/non-existent");

    atf::process::argv_array argva(argv);
    const atf::check::check_result r = atf::check::exec(argva);
    ATF_CHECK(r.exited());
    ATF_CHECK_EQUAL(r.exitcode(), 127);
}

// ------------------------------------------------------------------------
// Tests cases for the header file.
// ------------------------------------------------------------------------

HEADER_TC(include, "atf-c++/check.hpp", "d_include_check_hpp.cpp");

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add the test cases for the free functions.
    ATF_ADD_TEST_CASE(tcs, build_c_o);
    ATF_ADD_TEST_CASE(tcs, build_cpp);
    ATF_ADD_TEST_CASE(tcs, build_cxx_o);
    ATF_ADD_TEST_CASE(tcs, exec_argv);
    ATF_ADD_TEST_CASE(tcs, exec_cleanup);
    ATF_ADD_TEST_CASE(tcs, exec_exitstatus);
    ATF_ADD_TEST_CASE(tcs, exec_stdout_stderr);
    ATF_ADD_TEST_CASE(tcs, exec_unknown);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
}
