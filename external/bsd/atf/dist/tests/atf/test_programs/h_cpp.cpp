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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
}

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "atf-c++/env.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/macros.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/text.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
void
safe_mkdir(const char* path)
{
    if (::mkdir(path, 0755) == -1)
        ATF_FAIL(std::string("mkdir(2) of ") + path + " failed");
}

static
void
touch(const std::string& path)
{
    std::ofstream os(path.c_str());
    if (!os)
        ATF_FAIL("Could not create file " + path);
    os.close();
}

// ------------------------------------------------------------------------
// Helper tests for "t_cleanup".
// ------------------------------------------------------------------------

ATF_TEST_CASE_WITH_CLEANUP(cleanup_pass);
ATF_TEST_CASE_HEAD(cleanup_pass)
{
    set_md_var("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_pass)
{
    touch(get_config_var("tmpfile"));
}
ATF_TEST_CASE_CLEANUP(cleanup_pass)
{
    if (atf::text::to_bool(get_config_var("cleanup")))
        atf::fs::remove(atf::fs::path(get_config_var("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_fail);
ATF_TEST_CASE_HEAD(cleanup_fail)
{
    set_md_var("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_fail)
{
    touch(get_config_var("tmpfile"));
    ATF_FAIL("On purpose");
}
ATF_TEST_CASE_CLEANUP(cleanup_fail)
{
    if (atf::text::to_bool(get_config_var("cleanup")))
        atf::fs::remove(atf::fs::path(get_config_var("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_skip);
ATF_TEST_CASE_HEAD(cleanup_skip)
{
    set_md_var("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_skip)
{
    touch(get_config_var("tmpfile"));
    ATF_SKIP("On purpose");
}
ATF_TEST_CASE_CLEANUP(cleanup_skip)
{
    if (atf::text::to_bool(get_config_var("cleanup")))
        atf::fs::remove(atf::fs::path(get_config_var("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_curdir);
ATF_TEST_CASE_HEAD(cleanup_curdir)
{
    set_md_var("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_curdir)
{
    std::ofstream os("oldvalue");
    if (!os)
        ATF_FAIL("Failed to create oldvalue file");
    os << 1234;
    os.close();
}
ATF_TEST_CASE_CLEANUP(cleanup_curdir)
{
    std::ifstream is("oldvalue");
    if (is) {
        int i;
        is >> i;
        std::cout << "Old value: " << i << std::endl;
        is.close();
    }
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_sigterm);
ATF_TEST_CASE_HEAD(cleanup_sigterm)
{
    set_md_var("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_sigterm)
{
    touch(get_config_var("tmpfile"));
    ::kill(::getpid(), SIGTERM);
    touch(get_config_var("tmpfile") + ".no");
}
ATF_TEST_CASE_CLEANUP(cleanup_sigterm)
{
    atf::fs::remove(atf::fs::path(get_config_var("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_fork);
ATF_TEST_CASE_HEAD(cleanup_fork)
{
    set_md_var("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_fork)
{
}
ATF_TEST_CASE_CLEANUP(cleanup_fork)
{
    ::close(STDOUT_FILENO);
    ::close(STDERR_FILENO);
    ::close(3);
}

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
// Helper tests for "t_env".
// ------------------------------------------------------------------------

ATF_TEST_CASE(env_home);
ATF_TEST_CASE_HEAD(env_home)
{
    set_md_var("descr", "Helper test case for the t_env test program");
}
ATF_TEST_CASE_BODY(env_home)
{
    ATF_CHECK(atf::env::has("HOME"));
    atf::fs::path p(atf::env::get("HOME"));
    atf::fs::file_info fi1(p);
    atf::fs::file_info fi2(atf::fs::get_current_dir());
    ATF_CHECK_EQUAL(fi1.get_device(), fi2.get_device());
    ATF_CHECK_EQUAL(fi1.get_inode(), fi2.get_inode());
}

ATF_TEST_CASE(env_list);
ATF_TEST_CASE_HEAD(env_list)
{
    set_md_var("descr", "Helper test case for the t_env test program");
}
ATF_TEST_CASE_BODY(env_list)
{
    const atf::process::status s =
        atf::process::exec(atf::fs::path("env"),
                           atf::process::argv_array("env", NULL),
                           atf::process::stream_inherit(),
                           atf::process::stream_inherit());
    ATF_CHECK(s.exited());
    ATF_CHECK(s.exitstatus() == EXIT_SUCCESS);
}

// ------------------------------------------------------------------------
// Helper tests for "t_fork".
// ------------------------------------------------------------------------

ATF_TEST_CASE(fork_mangle_fds);
ATF_TEST_CASE_HEAD(fork_mangle_fds)
{
    set_md_var("descr", "Helper test case for the t_fork test program");
}
ATF_TEST_CASE_BODY(fork_mangle_fds)
{
    int resfd = std::atoi(get_config_var("resfd").c_str());

    if (::close(STDIN_FILENO) == -1)
        ATF_FAIL("Failed to close stdin");
    if (::close(STDOUT_FILENO) == -1)
        ATF_FAIL("Failed to close stdout");
    if (::close(STDERR_FILENO) == -1)
        ATF_FAIL("Failed to close stderr");
    if (::close(resfd) == -1)
        ATF_FAIL("Failed to close results descriptor");

#if defined(F_CLOSEM)
    if (::fcntl(0, F_CLOSEM) == -1)
        ATF_FAIL("Failed to close everything");
#endif
}

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

ATF_TEST_CASE(fork_umask);
ATF_TEST_CASE_HEAD(fork_umask)
{
    set_md_var("descr", "Helper test case for the t_fork test program");
}
ATF_TEST_CASE_BODY(fork_umask)
{
    mode_t m = ::umask(0);
    std::cout << "umask: " << std::setw(4) << std::setfill('0')
              << std::oct << m << std::endl;
    (void)::umask(m);
}

// ------------------------------------------------------------------------
// Helper tests for "t_meta_data".
// ------------------------------------------------------------------------

ATF_TEST_CASE(ident_1);
ATF_TEST_CASE_HEAD(ident_1)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
}
ATF_TEST_CASE_BODY(ident_1)
{
    ATF_CHECK_EQUAL(get_md_var("ident"), "ident_1");
}

ATF_TEST_CASE(ident_2);
ATF_TEST_CASE_HEAD(ident_2)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
}
ATF_TEST_CASE_BODY(ident_2)
{
    ATF_CHECK_EQUAL(get_md_var("ident"), "ident_2");
}

ATF_TEST_CASE(require_arch);
ATF_TEST_CASE_HEAD(require_arch)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.arch", get_config_var("arch", "not-set"));
}
ATF_TEST_CASE_BODY(require_arch)
{
}

ATF_TEST_CASE(require_config);
ATF_TEST_CASE_HEAD(require_config)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.config", "var1 var2");
}
ATF_TEST_CASE_BODY(require_config)
{
    std::cout << "var1: " << get_config_var("var1") << std::endl;
    std::cout << "var2: " << get_config_var("var2") << std::endl;
}

ATF_TEST_CASE(require_machine);
ATF_TEST_CASE_HEAD(require_machine)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.machine", get_config_var("machine", "not-set"));
}
ATF_TEST_CASE_BODY(require_machine)
{
}

ATF_TEST_CASE(require_progs_body);
ATF_TEST_CASE_HEAD(require_progs_body)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
}
ATF_TEST_CASE_BODY(require_progs_body)
{
    require_prog(get_config_var("progs"));
}

ATF_TEST_CASE(require_progs_head);
ATF_TEST_CASE_HEAD(require_progs_head)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.progs", get_config_var("progs", "not-set"));
}
ATF_TEST_CASE_BODY(require_progs_head)
{
}

ATF_TEST_CASE(require_user);
ATF_TEST_CASE_HEAD(require_user)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.user", get_config_var("user", "not-set"));
}
ATF_TEST_CASE_BODY(require_user)
{
}

ATF_TEST_CASE(require_user2);
ATF_TEST_CASE_HEAD(require_user2)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.user", get_config_var("user2", "not-set"));
}
ATF_TEST_CASE_BODY(require_user2)
{
}

ATF_TEST_CASE(require_user3);
ATF_TEST_CASE_HEAD(require_user3)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("require.user", get_config_var("user3", "not-set"));
}
ATF_TEST_CASE_BODY(require_user3)
{
}

ATF_TEST_CASE(timeout);
ATF_TEST_CASE_HEAD(timeout)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("timeout", get_config_var("timeout", "0"));
}
ATF_TEST_CASE_BODY(timeout)
{
    sleep(atf::text::to_type< int >(get_config_var("sleep")));
}

ATF_TEST_CASE(timeout2);
ATF_TEST_CASE_HEAD(timeout2)
{
    set_md_var("descr", "Helper test case for the t_meta_data test program");
    set_md_var("timeout", get_config_var("timeout2", "0"));
}
ATF_TEST_CASE_BODY(timeout2)
{
    sleep(atf::text::to_type< int >(get_config_var("sleep2")));
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
// Helper tests for "t_workdir".
// ------------------------------------------------------------------------

ATF_TEST_CASE(workdir_path);
ATF_TEST_CASE_HEAD(workdir_path)
{
    set_md_var("descr", "Helper test case for the t_workdir test program");
}
ATF_TEST_CASE_BODY(workdir_path)
{
    const std::string& p = get_config_var("pathfile");

    std::ofstream os(p.c_str());
    if (!os)
        ATF_FAIL("Could not open " + p + " for writing");

    os << atf::fs::get_current_dir().str() << std::endl;

    os.close();
}

ATF_TEST_CASE(workdir_cleanup);
ATF_TEST_CASE_HEAD(workdir_cleanup)
{
    set_md_var("descr", "Helper test case for the t_workdir test program");
}
ATF_TEST_CASE_BODY(workdir_cleanup)
{
    const std::string& p = get_config_var("pathfile");

    std::ofstream os(p.c_str());
    if (!os)
        ATF_FAIL("Could not open " + p + " for writing");

    os << atf::fs::get_current_dir().str() << std::endl;

    os.close();

    safe_mkdir("1");
    safe_mkdir("1/1");
    safe_mkdir("1/2");
    safe_mkdir("1/3");
    safe_mkdir("1/3/1");
    safe_mkdir("1/3/2");
    safe_mkdir("2");
    touch("2/1");
    touch("2/2");
    safe_mkdir("2/3");
    touch("2/3/1");
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add helper tests for t_cleanup.
    ATF_ADD_TEST_CASE(tcs, cleanup_pass);
    ATF_ADD_TEST_CASE(tcs, cleanup_fail);
    ATF_ADD_TEST_CASE(tcs, cleanup_skip);
    ATF_ADD_TEST_CASE(tcs, cleanup_curdir);
    ATF_ADD_TEST_CASE(tcs, cleanup_sigterm);
    ATF_ADD_TEST_CASE(tcs, cleanup_fork);

    // Add helper tests for t_config.
    ATF_ADD_TEST_CASE(tcs, config_unset);
    ATF_ADD_TEST_CASE(tcs, config_empty);
    ATF_ADD_TEST_CASE(tcs, config_value);
    ATF_ADD_TEST_CASE(tcs, config_multi_value);

    // Add helper tests for t_env.
    ATF_ADD_TEST_CASE(tcs, env_home);
    ATF_ADD_TEST_CASE(tcs, env_list);

    // Add helper tests for t_fork.
    ATF_ADD_TEST_CASE(tcs, fork_mangle_fds);
    ATF_ADD_TEST_CASE(tcs, fork_stop);
    ATF_ADD_TEST_CASE(tcs, fork_umask);

    // Add helper tests for t_meta_data.
    ATF_ADD_TEST_CASE(tcs, ident_1);
    ATF_ADD_TEST_CASE(tcs, ident_2);
    ATF_ADD_TEST_CASE(tcs, require_arch);
    ATF_ADD_TEST_CASE(tcs, require_config);
    ATF_ADD_TEST_CASE(tcs, require_machine);
    ATF_ADD_TEST_CASE(tcs, require_progs_body);
    ATF_ADD_TEST_CASE(tcs, require_progs_head);
    ATF_ADD_TEST_CASE(tcs, require_user);
    ATF_ADD_TEST_CASE(tcs, require_user2);
    ATF_ADD_TEST_CASE(tcs, require_user3);
    ATF_ADD_TEST_CASE(tcs, timeout);
    ATF_ADD_TEST_CASE(tcs, timeout2);

    // Add helper tests for t_srcdir.
    ATF_ADD_TEST_CASE(tcs, srcdir_exists);

    // Add helper tests for t_status.
    ATF_ADD_TEST_CASE(tcs, status_newlines_fail);
    ATF_ADD_TEST_CASE(tcs, status_newlines_skip);

    // Add helper tests for t_workdir.
    ATF_ADD_TEST_CASE(tcs, workdir_path);
    ATF_ADD_TEST_CASE(tcs, workdir_cleanup);
}
