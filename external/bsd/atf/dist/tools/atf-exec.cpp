//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "atf-c++/application.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/signals.hpp"
#include "atf-c++/text.hpp"

namespace sigalarm {
    static bool happened = false;

    void
    handler(int signo)
    {
        happened = true;
    }
}

class atf_exec : public atf::application::app {
    static const char* m_description;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

    void process_option_t(const std::string&);
    unsigned int m_timeout_secs;
    std::string m_timeout_file;

public:
    atf_exec(void);

    int main(void);
};

const char* atf_exec::m_description =
    "atf-exec executes the given command in a controlled manner.";

atf_exec::atf_exec(void) :
    app(m_description, "atf-exec(1)", "atf(7)"),
    m_timeout_secs(0)
{
}

std::string
atf_exec::specific_args(void)
    const
{
    return "<command>";
}

atf_exec::options_set
atf_exec::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('t', "secs:file", "Kills the command after the "
                                         "specified amount of time and "
                                         "creates the given file"));
    return opts;
}

void
atf_exec::process_option_t(const std::string& arg)
{
    using atf::application::usage_error;

    std::string::size_type pos = arg.find(':');
    if (pos == std::string::npos)
        throw usage_error("Invalid value for -t option; must be of the "
                          "form secs:file");
    if (pos == 0)
        throw usage_error("Invalid value for -t option; secs cannot be "
                          "empty");
    if (pos == arg.length() - 1)
        throw usage_error("Invalid value for -t option; file cannot be "
                          "empty");

    m_timeout_secs = atf::text::to_type< unsigned int >(arg.substr(0, pos));
    m_timeout_file = arg.substr(pos + 1);
}

void
atf_exec::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 't':
        process_option_t(arg);
        break;

    default:
        UNREACHABLE;
    }
}

int
atf_exec::main(void)
{
    if (m_argc < 1)
        throw atf::application::usage_error("No command specified");

    int exitcode = EXIT_FAILURE;

    pid_t pid = atf::process::fork();
    if (pid == 0) {
        if (::setpgid(::getpid(), 0) == -1)
            throw atf::system_error("main", "setpgid failed", errno);

        char** argv = new char*[m_argc + 1];
        for (int i = 0; i < m_argc; i++)
            argv[i] = ::strdup(m_argv[i]);
        argv[m_argc] = NULL;

        ::execvp(m_argv[0], argv);
        return EXIT_FAILURE;
    } else {
        atf::signals::signal_programmer sp(SIGALRM, sigalarm::handler);

        if (m_timeout_secs > 0) {
            struct itimerval itv;

            timerclear(&itv.it_interval);
            timerclear(&itv.it_value);
            itv.it_value.tv_sec = m_timeout_secs;
            if (setitimer(ITIMER_REAL, &itv, NULL) == -1)
                throw atf::system_error("main", "setitimer failed", errno);
        }

        int status;
        if (::waitpid(pid, &status, 0) != pid) {
            if (!(errno == EINTR && sigalarm::happened))
                throw atf::system_error("main", "waitpid failed", errno);
        }

        if (sigalarm::happened) {
            INV(m_timeout_secs > 0);

            ::killpg(pid, SIGTERM);

            std::ofstream os(m_timeout_file.c_str());
            os.close();

            INV(exitcode == EXIT_FAILURE);
        } else {
            if (WIFEXITED(status))
                exitcode = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                ::kill(0, WTERMSIG(status));
            else
                UNREACHABLE;
        }
    }

    return exitcode;
}

int
main(int argc, char* const* argv)
{
    return atf_exec().run(argc, argv);
}
