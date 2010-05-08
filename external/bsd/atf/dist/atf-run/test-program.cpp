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

#include <signal.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "atf-c++/env.hpp"
#include "atf-c++/formats.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/signals.hpp"
#include "atf-c++/text.hpp"

#include "config.hpp"
#include "test-program.hpp"
#include "timer.hpp"

namespace impl = atf::atf_run;

namespace {

class metadata_reader : public atf::formats::atf_tp_reader {
    impl::test_cases_map m_tcs;

    void got_tc(const std::string& ident, const atf::tests::vars_map& props)
    {
        if (m_tcs.find(ident) != m_tcs.end())
            throw(std::runtime_error("Duplicate test case " + ident +
                                     " in test program"));
        m_tcs[ident] = props;

        if (m_tcs[ident].find("timeout") == m_tcs[ident].end())
            m_tcs[ident].insert(std::make_pair("timeout", "300"));
    }

public:
    metadata_reader(std::istream& is) :
        atf::formats::atf_tp_reader(is)
    {
    }

    const impl::test_cases_map&
    get_tcs(void)
        const
    {
        return m_tcs;
    }
};

class output_muxer : public atf::io::std_muxer {
    atf::formats::atf_tps_writer& m_writer;

    void
    got_stdout_line(const std::string& line)
    {
        m_writer.stdout_tc(line);
    }

    void
    got_stderr_line(const std::string& line)
    {
        m_writer.stderr_tc(line);
    }

public:
    output_muxer(atf::formats::atf_tps_writer& writer) :
        m_writer(writer)
    {
    }
};

struct get_metadata_params {
    const atf::fs::path& executable;
    const atf::tests::vars_map& config;

    get_metadata_params(const atf::fs::path& p_executable,
                        const atf::tests::vars_map& p_config) :
        executable(p_executable),
        config(p_config)
    {
    }
};

struct test_case_params {
    const atf::fs::path& executable;
    const std::string& test_case_name;
    const std::string& test_case_part;
    const atf::tests::vars_map& metadata;
    const atf::tests::vars_map& config;
    const atf::fs::path& resfile;
    const atf::fs::path& workdir;

    test_case_params(const atf::fs::path& p_executable,
                     const std::string& p_test_case_name,
                     const std::string& p_test_case_part,
                     const atf::tests::vars_map& p_metadata,
                     const atf::tests::vars_map& p_config,
                     const atf::fs::path& p_resfile,
                     const atf::fs::path& p_workdir) :
        executable(p_executable),
        test_case_name(p_test_case_name),
        test_case_part(p_test_case_part),
        metadata(p_metadata),
        config(p_config),
        resfile(p_resfile),
        workdir(p_workdir)
    {
    }
};

static
void
append_to_vector(std::vector< std::string >& v1,
                 const std::vector< std::string >& v2)
{
    std::copy(v2.begin(), v2.end(),
              std::back_insert_iterator< std::vector< std::string > >(v1));
}

static
char**
vector_to_argv(const std::vector< std::string >& v)
{
    char** argv = new char*[v.size() + 1];
    for (std::vector< std::string >::size_type i = 0; i < v.size(); i++) {
        argv[i] = strdup(v[i].c_str());
    }
    argv[v.size()] = NULL;
    return argv;
}

static
void
exec_or_exit(const atf::fs::path& executable,
             const std::vector< std::string >& argv)
{
    // This leaks memory in case of a failure, but it is OK.  Exiting will
    // do the necessary cleanup.
    char* const* native_argv = vector_to_argv(argv);

    ::execv(executable.c_str(), native_argv);

    const std::string message = "Failed to execute '" + executable.str() +
        "': " + std::strerror(errno) + "\n";
    if (::write(STDERR_FILENO, message.c_str(), message.length()) == -1)
        std::abort();
    std::exit(EXIT_FAILURE);
}

static
void
create_timeout_resfile(const atf::fs::path& path, const unsigned int timeout)
{
    std::ofstream os(path.c_str());
    if (!os)
        throw std::runtime_error("Failed to create " + path.str());

    const std::string reason = "Test case timed out after " +
        atf::text::to_string(timeout) + " " +
        (timeout == 1 ? "second" : "seconds");

    atf::formats::atf_tcr_writer writer(os);
    writer.result("failed");
    writer.reason(reason);
}

static
std::vector< std::string >
config_to_args(const atf::tests::vars_map& config)
{
    std::vector< std::string > args;

    for (atf::tests::vars_map::const_iterator iter = config.begin();
         iter != config.end(); iter++)
        args.push_back("-v" + (*iter).first + "=" + (*iter).second);

    return args;
}

static
void
prepare_child(const atf::fs::path& workdir)
{
    const int ret = ::setpgid(::getpid(), 0);
    INV(ret != -1);

    ::umask(S_IWGRP | S_IWOTH);

    for (int i = 1; i <= atf::signals::last_signo; i++)
        atf::signals::reset(i);

    atf::env::set("HOME", workdir.str());
    atf::env::unset("LANG");
    atf::env::unset("LC_ALL");
    atf::env::unset("LC_COLLATE");
    atf::env::unset("LC_CTYPE");
    atf::env::unset("LC_MESSAGES");
    atf::env::unset("LC_MONETARY");
    atf::env::unset("LC_NUMERIC");
    atf::env::unset("LC_TIME");
    atf::env::unset("TZ");

    atf::fs::change_directory(workdir);
}

static
void
get_metadata_child(void* raw_params)
{
    const get_metadata_params* params =
        static_cast< const get_metadata_params* >(raw_params);

    std::vector< std::string > argv;
    argv.push_back(params->executable.leaf_name());
    argv.push_back("-l");
    argv.push_back("-s" + params->executable.branch_path().str());
    append_to_vector(argv, config_to_args(params->config));

    exec_or_exit(params->executable, argv);
}

void
run_test_case_child(void* raw_params)
{
    const test_case_params* params =
        static_cast< const test_case_params* >(raw_params);

    // The input 'tp' parameter may be relative and become invalid once
    // we change the current working directory.
    const atf::fs::path absolute_executable = params->executable.to_absolute();

    // Prepare the test program's arguments.  We use dynamic memory and
    // do not care to release it.  We are going to die anyway very soon,
    // either due to exec(2) or to exit(3).
    std::vector< std::string > argv;
    argv.push_back(absolute_executable.leaf_name());
    argv.push_back("-r" + params->resfile.str());
    argv.push_back("-s" + absolute_executable.branch_path().str());
    append_to_vector(argv, config_to_args(params->config));
    argv.push_back(params->test_case_name + ":" + params->test_case_part);

    prepare_child(params->workdir);
    exec_or_exit(absolute_executable, argv);
}

} // anonymous namespace

impl::metadata
impl::get_metadata(const atf::fs::path& executable,
                   const atf::tests::vars_map& config)
{
    get_metadata_params params(executable, config);
    atf::process::child child =
        atf::process::fork(get_metadata_child,
                           atf::process::stream_capture(),
                           atf::process::stream_inherit(),
                           static_cast< void * >(&params));

    atf::io::file_handle outfh = child.stdout_fd();
    atf::io::pistream outin(outfh);

    metadata_reader parser(outin);
    parser.read();

    const atf::process::status status = child.wait();
    if (!status.exited() || status.exitstatus() != EXIT_SUCCESS)
        throw atf::formats::format_error("Test program returned failure "
                                         "exit status for test case list");

    return metadata(parser.get_tcs());
}

atf::process::status
impl::run_test_case(const atf::fs::path& executable,
                    const std::string& test_case_name,
                    const std::string& test_case_part,
                    const atf::tests::vars_map& metadata,
                    const atf::tests::vars_map& config,
                    const atf::fs::path& resfile,
                    const atf::fs::path& workdir,
                    atf::formats::atf_tps_writer& writer)
{
    // TODO: Capture termination signals and deliver them to the subprocess
    // instead.  Or maybe do something else; think about it.

    test_case_params params(executable, test_case_name, test_case_part,
                            metadata, config, resfile, workdir);
    atf::process::child child =
        atf::process::fork(run_test_case_child,
                           atf::process::stream_capture(),
                           atf::process::stream_capture(),
                           static_cast< void * >(&params));

    const atf::tests::vars_map::const_iterator iter = metadata.find("timeout");
    INV(iter != metadata.end());
    const unsigned int timeout =
        atf::text::to_type< unsigned int >((*iter).second);
    const pid_t child_pid = child.pid();
    child_timer timeout_timer(timeout, child_pid);

    // Get the input stream of stdout and stderr.
    atf::io::file_handle outfh = child.stdout_fd();
    atf::io::unbuffered_istream outin(outfh);
    atf::io::file_handle errfh = child.stderr_fd();
    atf::io::unbuffered_istream errin(errfh);

    // Process the test case's output and multiplex it into our output
    // stream as we read it.
    try {
        output_muxer muxer(writer);
        muxer.read(outin, errin);
        outin.close();
        errin.close();
    } catch (...) {
        UNREACHABLE;
    }

    atf::process::status status = child.wait();
    ::killpg(child_pid, SIGKILL);

    if (timeout_timer.fired()) {
        INV(status.signaled());
        INV(status.termsig() == SIGKILL);
        create_timeout_resfile(resfile, timeout);
    }

    return status;
}
