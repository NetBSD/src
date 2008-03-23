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

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
}

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "atf/env.hpp"
#include "atf/fs.hpp"
#include "atf/macros.hpp"
#include "atf/text.hpp"

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
    set("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_pass)
{
    touch(config().get("tmpfile"));
}
ATF_TEST_CASE_CLEANUP(cleanup_pass)
{
    if (config().get_bool("cleanup"))
        atf::fs::remove(atf::fs::path(config().get("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_fail);
ATF_TEST_CASE_HEAD(cleanup_fail)
{
    set("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_fail)
{
    touch(config().get("tmpfile"));
    ATF_FAIL("On purpose");
}
ATF_TEST_CASE_CLEANUP(cleanup_fail)
{
    if (config().get_bool("cleanup"))
        atf::fs::remove(atf::fs::path(config().get("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_skip);
ATF_TEST_CASE_HEAD(cleanup_skip)
{
    set("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_skip)
{
    touch(config().get("tmpfile"));
    ATF_SKIP("On purpose");
}
ATF_TEST_CASE_CLEANUP(cleanup_skip)
{
    if (config().get_bool("cleanup"))
        atf::fs::remove(atf::fs::path(config().get("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_curdir);
ATF_TEST_CASE_HEAD(cleanup_curdir)
{
    set("descr", "Helper test case for the t_cleanup test program");
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
    set("descr", "Helper test case for the t_cleanup test program");
}
ATF_TEST_CASE_BODY(cleanup_sigterm)
{
    touch(config().get("tmpfile"));
    ::kill(::getpid(), SIGTERM);
    touch(config().get("tmpfile") + ".no");
}
ATF_TEST_CASE_CLEANUP(cleanup_sigterm)
{
    atf::fs::remove(atf::fs::path(config().get("tmpfile")));
}

ATF_TEST_CASE_WITH_CLEANUP(cleanup_fork);
ATF_TEST_CASE_HEAD(cleanup_fork)
{
    set("descr", "Helper test case for the t_cleanup test program");
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
    set("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_unset)
{
    ATF_CHECK(!config().has("test"));
}

ATF_TEST_CASE(config_empty);
ATF_TEST_CASE_HEAD(config_empty)
{
    set("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_empty)
{
    ATF_CHECK_EQUAL(config().get("test"), "");
}

ATF_TEST_CASE(config_value);
ATF_TEST_CASE_HEAD(config_value)
{
    set("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_value)
{
    ATF_CHECK_EQUAL(config().get("test"), "foo");
}

ATF_TEST_CASE(config_multi_value);
ATF_TEST_CASE_HEAD(config_multi_value)
{
    set("descr", "Helper test case for the t_config test program");
}
ATF_TEST_CASE_BODY(config_multi_value)
{
    ATF_CHECK_EQUAL(config().get("test"), "foo bar");
}

// ------------------------------------------------------------------------
// Helper tests for "t_env".
// ------------------------------------------------------------------------

ATF_TEST_CASE(env_home);
ATF_TEST_CASE_HEAD(env_home)
{
    set("descr", "Helper test case for the t_env test program");
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
    set("descr", "Helper test case for the t_env test program");
}
ATF_TEST_CASE_BODY(env_list)
{
    std::system("env");
}

// ------------------------------------------------------------------------
// Helper tests for "t_fork".
// ------------------------------------------------------------------------

ATF_TEST_CASE(fork_mangle_fds);
ATF_TEST_CASE_HEAD(fork_mangle_fds)
{
    set("descr", "Helper test case for the t_fork test program");
}
ATF_TEST_CASE_BODY(fork_mangle_fds)
{
    int resfd = std::atoi(config().get("resfd").c_str());

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
    set("descr", "Helper test case for the t_fork test program");
}
ATF_TEST_CASE_BODY(fork_stop)
{
    std::ofstream os(config().get("pidfile").c_str());
    os << ::getpid() << std::endl;
    os.close();
    std::cout << "Wrote pid file" << std::endl;
    std::cout << "Waiting for done file" << std::endl;
    while (::access(config().get("donefile").c_str(), F_OK) != 0)
        ::usleep(10000);
    std::cout << "Exiting" << std::endl;
}

ATF_TEST_CASE(fork_umask);
ATF_TEST_CASE_HEAD(fork_umask)
{
    set("descr", "Helper test case for the t_fork test program");
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
    set("descr", "Helper test case for the t_meta_data test program");
}
ATF_TEST_CASE_BODY(ident_1)
{
    ATF_CHECK_EQUAL(get("ident"), "ident_1");
}

ATF_TEST_CASE(ident_2);
ATF_TEST_CASE_HEAD(ident_2)
{
    set("descr", "Helper test case for the t_meta_data test program");
}
ATF_TEST_CASE_BODY(ident_2)
{
    ATF_CHECK_EQUAL(get("ident"), "ident_2");
}

ATF_TEST_CASE(require_arch);
ATF_TEST_CASE_HEAD(require_arch)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.arch", config().get("arch", "not-set"));
}
ATF_TEST_CASE_BODY(require_arch)
{
}

ATF_TEST_CASE(require_config);
ATF_TEST_CASE_HEAD(require_config)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.config", "var1 var2");
}
ATF_TEST_CASE_BODY(require_config)
{
    std::cout << "var1: " << config().get("var1") << std::endl;
    std::cout << "var2: " << config().get("var2") << std::endl;
}

ATF_TEST_CASE(require_machine);
ATF_TEST_CASE_HEAD(require_machine)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.machine", config().get("machine", "not-set"));
}
ATF_TEST_CASE_BODY(require_machine)
{
}

ATF_TEST_CASE(require_progs_body);
ATF_TEST_CASE_HEAD(require_progs_body)
{
    set("descr", "Helper test case for the t_meta_data test program");
}
ATF_TEST_CASE_BODY(require_progs_body)
{
    require_prog(config().get("progs"));
}

ATF_TEST_CASE(require_progs_head);
ATF_TEST_CASE_HEAD(require_progs_head)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.progs", config().get("progs", "not-set"));
}
ATF_TEST_CASE_BODY(require_progs_head)
{
}

ATF_TEST_CASE(require_user_root);
ATF_TEST_CASE_HEAD(require_user_root)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.user", "root");
}
ATF_TEST_CASE_BODY(require_user_root)
{
}

ATF_TEST_CASE(require_user_root2);
ATF_TEST_CASE_HEAD(require_user_root2)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.user", "root");
}
ATF_TEST_CASE_BODY(require_user_root2)
{
}

ATF_TEST_CASE(require_user_unprivileged);
ATF_TEST_CASE_HEAD(require_user_unprivileged)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.user", "unprivileged");
}
ATF_TEST_CASE_BODY(require_user_unprivileged)
{
}

ATF_TEST_CASE(require_user_unprivileged2);
ATF_TEST_CASE_HEAD(require_user_unprivileged2)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("require.user", "unprivileged");
}
ATF_TEST_CASE_BODY(require_user_unprivileged2)
{
}

ATF_TEST_CASE(timeout);
ATF_TEST_CASE_HEAD(timeout)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("timeout", config().get("timeout", "0"));
}
ATF_TEST_CASE_BODY(timeout)
{
    sleep(atf::text::to_type< int >(config().get("sleep")));
}

ATF_TEST_CASE(timeout2);
ATF_TEST_CASE_HEAD(timeout2)
{
    set("descr", "Helper test case for the t_meta_data test program");
    set("timeout", config().get("timeout2", "0"));
}
ATF_TEST_CASE_BODY(timeout2)
{
    sleep(atf::text::to_type< int >(config().get("sleep2")));
}

// ------------------------------------------------------------------------
// Helper tests for "t_srcdir".
// ------------------------------------------------------------------------

ATF_TEST_CASE(srcdir_exists);
ATF_TEST_CASE_HEAD(srcdir_exists)
{
    set("descr", "Helper test case for the t_srcdir test program");
}
ATF_TEST_CASE_BODY(srcdir_exists)
{
    if (!atf::fs::exists(atf::fs::path(get_srcdir()) / "datafile"))
        ATF_FAIL("Cannot find datafile");
}

// ------------------------------------------------------------------------
// Helper tests for "t_status".
// ------------------------------------------------------------------------

ATF_TEST_CASE(status_newlines_fail);
ATF_TEST_CASE_HEAD(status_newlines_fail)
{
    set("descr", "Helper test case for the t_status test program");
}
ATF_TEST_CASE_BODY(status_newlines_fail)
{
    ATF_FAIL("First line\nSecond line");
}

ATF_TEST_CASE(status_newlines_skip);
ATF_TEST_CASE_HEAD(status_newlines_skip)
{
    set("descr", "Helper test case for the t_status test program");
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
    set("descr", "Helper test case for the t_workdir test program");
}
ATF_TEST_CASE_BODY(workdir_path)
{
    const std::string& p = config().get("pathfile");

    std::ofstream os(p.c_str());
    if (!os)
        ATF_FAIL("Could not open " + p + " for writing");

    os << atf::fs::get_current_dir().str() << std::endl;

    os.close();
}

ATF_TEST_CASE(workdir_cleanup);
ATF_TEST_CASE_HEAD(workdir_cleanup)
{
    set("descr", "Helper test case for the t_workdir test program");
}
ATF_TEST_CASE_BODY(workdir_cleanup)
{
    const std::string& p = config().get("pathfile");

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
    ATF_ADD_TEST_CASE(tcs, require_user_root);
    ATF_ADD_TEST_CASE(tcs, require_user_root2);
    ATF_ADD_TEST_CASE(tcs, require_user_unprivileged);
    ATF_ADD_TEST_CASE(tcs, require_user_unprivileged2);
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
