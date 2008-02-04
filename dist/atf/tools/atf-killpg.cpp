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
#include <signal.h>
}

#include <cerrno>
#include <cstdlib>
#include <iostream>

#include "atf/application.hpp"
#include "atf/exceptions.hpp"
#include "atf/sanity.hpp"
#include "atf/text.hpp"

class atf_killpg : public atf::application::app {
    static const char* m_description;

    int m_signo;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

public:
    atf_killpg(void);

    int main(void);
};

const char* atf_killpg::m_description =
    "atf-killpg sends a signal to a process group.";

atf_killpg::atf_killpg(void) :
    app(m_description, "atf-killpg(1)", "atf(7)"),
    m_signo(SIGTERM)
{
}

std::string
atf_killpg::specific_args(void)
    const
{
    return "<pid>";
}

atf_killpg::options_set
atf_killpg::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('s', "signal",
                       "The signal to send to processes (default SIGTERM)"));
    return opts;
}

void
atf_killpg::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 's':
        m_signo = atf::text::to_type< int >(arg);
        break;

    default:
        UNREACHABLE;
    }
}

int
atf_killpg::main(void)
{
    if (m_argc < 1)
        throw atf::application::usage_error("No PID specified");

    pid_t pid = atf::text::to_type< pid_t >(m_argv[0]);
    if (::killpg(pid, m_signo) == -1)
        throw atf::system_error("main", "killpg failed", errno);

    return EXIT_SUCCESS;
}

int
main(int argc, char* const* argv)
{
    return atf_killpg().run(argc, argv);
}
