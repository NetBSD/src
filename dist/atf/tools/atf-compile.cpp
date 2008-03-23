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
}

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "atf/application.hpp"
#include "atf/config.hpp"
#include "atf/sanity.hpp"

static
void
cat_file(std::ostream& os, const std::string& path)
{
    std::ifstream is(path.c_str());
    if (!is)
        throw std::runtime_error("Cannot open " + path);
    std::string line;
    while (std::getline(is, line))
        os << line << std::endl;
    is.close();
}

class atf_compile : public atf::application::app {
    static const char* m_description;

    std::string m_outfile;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int ch, const char*);

    void compile(std::ostream&);

public:
    atf_compile(void);

    int main(void);
};

const char* atf_compile::m_description =
    "atf-compile is a tool that ``compiles'' test cases written in the "
    "POSIX shell language, generating an executable.";

atf_compile::atf_compile(void) :
    app(m_description, "atf-compile(1)", "atf(7)")
{
}

std::string
atf_compile::specific_args(void)
    const
{
    return "<test-program>";
}

atf_compile::options_set
atf_compile::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('o', "out-file", "Name of the output file"));
    return opts;
}

void
atf_compile::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'o':
        m_outfile = arg;
        break;

    default:
        UNREACHABLE;
    }
}

void
atf_compile::compile(std::ostream& os)
{
    os << "#! " << atf::config::get("atf_shell") << std::endl;
    cat_file(os, atf::config::get("atf_pkgdatadir") + "/atf.init.subr");
    os << std::endl;
    os << ". ${Atf_Pkgdatadir}/atf.header.subr" << std::endl;
    os << std::endl;
    for (int i = 0; i < m_argc; i++) {
        cat_file(os, m_argv[i]);
        os << std::endl;
    }
    os << ". ${Atf_Pkgdatadir}/atf.footer.subr" << std::endl;
    os << std::endl;
    os << "main \"${@}\"" << std::endl;
}

int
atf_compile::main(void)
{
    if (m_argc < 1)
        throw atf::application::usage_error("No test program specified");

    if (m_outfile.empty())
        throw atf::application::usage_error("No output file specified");

    std::ofstream os(m_outfile.c_str());
    if (!os)
        throw std::runtime_error("Cannot open output file `" +
                                 m_outfile + "'");
    compile(os);
    os.close();

    mode_t umask = ::umask(S_IRWXU | S_IRWXG | S_IRWXO);
    ::umask(umask);

    if (::chmod(m_outfile.c_str(),
                (S_IRWXU | S_IRWXG | S_IRWXO) & ~umask) == -1)
        throw std::runtime_error("Cannot set executable permissions "
                                 "on `" + m_outfile + "'");

    return EXIT_SUCCESS;
}

int
main(int argc, char* const* argv)
{
    return atf_compile().run(argc, argv);
}
