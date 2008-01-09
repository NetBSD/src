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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

extern "C" {
#include <unistd.h>
}

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "atf/application.hpp"
#include "atf/exceptions.hpp"
#include "atf/sanity.hpp"
#include "atf/ui.hpp"

atf::application::option::option(char ch,
                                 const std::string& a,
                                 const std::string& desc) :
    m_character(ch),
    m_argument(a),
    m_description(desc)
{
}

bool
atf::application::option::operator<(const atf::application::option& o)
    const
{
    return m_character < o.m_character;
}

atf::application::application(const std::string& description,
                              const std::string& manpage) :
    m_argc(-1),
    m_argv(NULL),
    m_prog_name(NULL),
    m_description(description),
    m_manpage(manpage)
{
}

atf::application::~application(void)
{
}

bool
atf::application::inited(void)
{
    return m_argc != -1;
}

atf::application::options_set
atf::application::options(void)
{
    options_set opts = specific_options();
    opts.insert(option('h', "", "Shows this help message"));
    return opts;
}

std::string
atf::application::specific_args(void)
    const
{
    return "";
}

atf::application::options_set
atf::application::specific_options(void)
    const
{
    return options_set();
}

void
atf::application::process_option(int ch, const char* arg)
{
}

void
atf::application::process_options(void)
{
    PRE(inited());

    std::string optstr(":");
    {
        options_set opts = options();
        for (options_set::const_iterator iter = opts.begin();
             iter != opts.end(); iter++) {
            const option& opt = (*iter);

            optstr += opt.m_character;
            if (!opt.m_argument.empty())
                optstr += ':';
        }
    }

    int ch;
    ::opterr = 0;
    while ((ch = ::getopt(m_argc, m_argv, optstr.c_str())) != -1) {
        switch (ch) {
            case 'h':
                usage(std::cout);
                ::exit(EXIT_SUCCESS);

            case ':':
                throw usage_error("Option -%c requires an argument.",
                                  ::optopt);

            case '?':
                throw usage_error("Unknown option -%c.", ::optopt);

            default:
                process_option(ch, ::optarg);
        }
    }
    m_argc -= ::optind;
    m_argv += ::optind;
}

void
atf::application::usage(std::ostream& os)
{
    PRE(inited());

    std::string args = specific_args();
    if (!args.empty())
        args = " " + args;
    os << ui::format_text_with_tag(std::string(m_prog_name) + " [options]" +
                                   args, "Usage: ", false) << std::endl
       << std::endl
       << ui::format_text(m_description) << std::endl
       << std::endl;

    options_set opts = options();
    INV(!opts.empty());
    os << "Available options:" << std::endl;
    size_t coldesc = 0;
    for (options_set::const_iterator iter = opts.begin();
         iter != opts.end(); iter++) {
        const option& opt = (*iter);

        if (opt.m_argument.length() + 1 > coldesc)
            coldesc = opt.m_argument.length() + 1;
    }
    for (options_set::const_iterator iter = opts.begin();
         iter != opts.end(); iter++) {
        const option& opt = (*iter);

        std::string tag = std::string("    -") + opt.m_character;
        if (opt.m_argument.empty())
            tag += "    ";
        else
            tag += " " + opt.m_argument + "    ";
        os << ui::format_text_with_tag(opt.m_description, tag, false,
                                       coldesc + 10)
           << std::endl;
    }
    os << std::endl;

    os << ui::format_text("For more details please see " + m_manpage +
                          " and atf(7).")
       << std::endl;
}

int
atf::application::run(int argc, char* const* argv)
{
    PRE(argc > 0);
    PRE(argv != NULL);

    m_argc = argc;
    m_argv = argv;

    m_prog_name = std::strrchr(m_argv[0], '/');
    if (m_prog_name == NULL)
        m_prog_name = m_argv[0];
    else
        m_prog_name++;

    const std::string bug =
        std::string("This is probably a bug in ") + m_prog_name +
        " or one of the libraries it uses.  Please report this problem to "
        PACKAGE_BUGREPORT " and provide as many details as possible "
        "describing how you got to this condition.";

    int errcode;
    try {
        process_options();
        errcode = main();
    } catch (const usage_error& e) {
        std::cerr << ui::format_error(m_prog_name, e.what())
                  << std::endl
                  << ui::format_info(m_prog_name, std::string("Type `") +
                                     m_prog_name + " -h' for more details.")
                  << std::endl;
        errcode = EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        std::cerr << ui::format_error(m_prog_name, std::string(e.what()))
                  << std::endl;
        errcode = EXIT_FAILURE;
    } catch (const atf::sanity::sanity_error& e) {
        std::cerr << ui::format_error(m_prog_name,
                                      std::string(e.what()) + "\n" + bug)
                  << std::endl;
        errcode = EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << ui::format_error(m_prog_name,
                                      std::string("Caught unexpected error: ")
                                      + e.what() + "\n" + bug)
                  << std::endl;
        errcode = EXIT_FAILURE;
    } catch (...) {
        std::cerr << ui::format_error(m_prog_name,
                                      std::string("Caught unknown error\n") +
                                      bug)
                  << std::endl;
        errcode = EXIT_FAILURE;
    }
    return errcode;
}
