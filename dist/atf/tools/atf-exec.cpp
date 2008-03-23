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
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "atf/application.hpp"
#include "atf/exceptions.hpp"
#include "atf/sanity.hpp"
#include "atf/text.hpp"

class atf_exec : public atf::application::app {
    static const char* m_description;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

    bool m_pgid;

public:
    atf_exec(void);

    int main(void);
};

const char* atf_exec::m_description =
    "atf-exec executes the given command after modifying its environment "
    "in different ways.";

atf_exec::atf_exec(void) :
    app(m_description, "atf-exec(1)", "atf(7)"),
    m_pgid(false)
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
    opts.insert(option('g', "", "Creates a new process group under which "
                                "to run the command"));
    return opts;
}

void
atf_exec::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'g':
        m_pgid = true;
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

    if (m_pgid) {
        if (::setpgid(::getpid(), 0) == -1)
            throw atf::system_error("main", "setpgid failed", errno);
    }

    int exitcode = EXIT_FAILURE;

    pid_t pid = ::fork();
    if (pid == 0) {
        char** argv = new char*[m_argc + 1];
        for (int i = 0; i < m_argc; i++)
            argv[i] = ::strdup(m_argv[i]);
        argv[m_argc] = NULL;

        ::execvp(m_argv[0], argv);
        return EXIT_FAILURE;
    } else {
        int status;
        if (::waitpid(pid, &status, 0) != pid)
            throw atf::system_error("main", "waitpid failed", errno);
        if (WIFEXITED(status))
            exitcode = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            ::kill(0, WTERMSIG(status));
        else
            UNREACHABLE;
    }

    return exitcode;
}

int
main(int argc, char* const* argv)
{
    return atf_exec().run(argc, argv);
}
