//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "atf/application.hpp"
#include "atf/sanity.hpp"
#include "atf/ui.hpp"

class atf_format : public atf::application {
    static const char* m_description;

    size_t m_length;
    std::string m_tag;
    bool m_repeat;

    void process_option(int, const char*);
    std::string specific_args(void) const;
    options_set specific_options(void) const;

public:
    atf_format(void);

    int main(void);
};

const char* atf_format::m_description =
    "atf-format is a tool that formats text messages to fit inside "
    "the terminal's size.  Messages can be fed either through the "
    "standard input or as arguments to the program.  Each independent "
    "line is treated as a different paragraph.";

atf_format::atf_format(void) :
    application(m_description, "atf-format(1)"),
    m_length(0),
    m_repeat(false)
{
}

void
atf_format::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'l':
        {
            std::istringstream ss(arg);
            ss >> m_length;
            if (m_length < m_tag.length())
                throw std::runtime_error("Length must be greater than "
                                         "tag's length");
        }
        break;

    case 'r':
        m_repeat = true;
        break;

    case 't':
        m_tag = arg;
        if (m_length < m_tag.length())
            m_length = m_tag.length();
        break;

    default:
        UNREACHABLE;
    }
}

std::string
atf_format::specific_args(void)
    const
{
    return "[str1 [.. strN]]";
}

atf_format::options_set
atf_format::specific_options(void)
    const
{
    options_set opts;
    opts.insert(option('l', "length", "Tag length"));
    opts.insert(option('r', "", "Repeat tag on each line"));
    opts.insert(option('t', "tag", "Tag to use for printing"));
    return opts;
}

int
atf_format::main(void)
{
    std::string str;

    if (m_argc > 0) {
        for (int i = 0; i < m_argc; i++) {
            str += m_argv[i];
            if (i < m_argc - 1)
                str += ' ';
        }
        str += '\n';
    } else {
        std::string line;
        while (std::getline(std::cin, line))
            str += line + '\n';
    }

    std::cout << atf::ui::format_text_with_tag(str, m_tag, m_repeat, m_length)
              << std::endl;

    return EXIT_SUCCESS;
}

int
main(int argc, char* const* argv)
{
    return atf_format().run(argc, argv);
}
