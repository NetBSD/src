//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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
#include <sys/wait.h>
}

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <utility>

#include "atf-c++/application.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/check.hpp"
#include "atf-c++/config.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/text.hpp"
#include "atf-c++/utils.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
std::string
flatten_argv(char* const* argv)
{
    std::string cmdline;

    char* const* arg = &argv[0];
    while (*arg != NULL) {
        if (arg != &argv[0])
            cmdline += ' ';

        cmdline += *arg;

        arg++;
    }

    return cmdline;
}

static
atf::check::check_result
execute(const char* const* argv)
{
    std::cout << "Executing command [ ";
    for (int i = 0; argv[i] != NULL; ++i)
        std::cout << argv[i] << " ";
    std::cout << "]" << std::endl;

    atf::process::argv_array argva(argv);
    return atf::check::exec(argva);
}

static
atf::check::check_result
execute_with_shell(char* const* argv)
{
    const std::string cmd = flatten_argv(argv);

    const char* sh_argv[4];
    sh_argv[0] = atf::config::get("atf_shell").c_str();
    sh_argv[1] = "-c";
    sh_argv[2] = cmd.c_str();
    sh_argv[3] = NULL;
    return execute(sh_argv);
}

// ------------------------------------------------------------------------
// The "atf_check" application.
// ------------------------------------------------------------------------

class atf_check : public atf::application::app {
    enum output_check_t {
        oc_ignore,
        oc_inline,
        oc_file,
        oc_empty,
        oc_save
    };

    enum status_check_t {
        sc_equal,
        sc_not_equal,
        sc_ignore
    };

    bool m_xflag;

    output_check_t m_stdout_check;
    std::string m_stdout_arg;

    output_check_t m_stderr_check;
    std::string m_stderr_arg;

    status_check_t m_status_check;
    int m_status_arg;

    static const char* m_description;

    bool file_empty(const atf::fs::path&) const;
    void print_diff(const atf::fs::path&, const atf::fs::path&) const;
    void print_file(const atf::fs::path&) const;
    std::string decode(const std::string&) const;

    bool run_status_check(const atf::check::check_result&) const;
    bool run_output_check(const atf::check::check_result&,
                          const std::string&) const;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);
    void process_option_s(const std::string&);
    void process_option_o(const std::string&);
    void process_option_e(const std::string&);

public:
    atf_check(void);
    int main(void);
};

const char* atf_check::m_description =
    "atf-check executes given command and analyzes its results.";

atf_check::atf_check(void) :
    app(m_description, "atf-check(1)", "atf(7)"),
    m_xflag(false),
    m_stdout_check(oc_empty),
    m_stderr_check(oc_empty),
    m_status_check(sc_equal),
    m_status_arg(0)
{
}

bool
atf_check::file_empty(const atf::fs::path& p)
    const
{
    atf::fs::file_info f(p);

    return (f.get_size() == 0);
}

void
atf_check::print_diff(const atf::fs::path& p1, const atf::fs::path& p2)
    const
{
    const atf::process::status s =
        atf::process::exec(atf::fs::path("diff"),
                           atf::process::argv_array("diff", "-u", p1.c_str(),
                                                    p2.c_str(), NULL),
                           atf::process::stream_connect(STDOUT_FILENO,
                                                        STDERR_FILENO),
                           atf::process::stream_inherit());

    if (!s.exited())
        std::cerr << "Failed to run diff(3)" << std::endl;

    if (s.exitstatus() != 1)
        std::cerr << "Error while running diff(3)" << std::endl;
}

void
atf_check::print_file(const atf::fs::path& p)
    const
{
    std::ifstream f(p.c_str());
    f >> std::noskipws;
    std::istream_iterator< char > begin(f), end;
    std::ostream_iterator< char > out(std::cerr);
    std::copy(begin, end, out);
}

std::string
atf_check::decode(const std::string& s)
    const
{
    size_t i;
    std::string res;

    res.reserve(s.length());

    i = 0;
    while (i < s.length()) {
        char c = s[i++];

        if (c == '\\') {
            switch (s[i++]) {
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'c': break;
            case 'e': c = 033; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;
            case '\\': break;
            case '0':
                {
                    int count = 3;
                    c = 0;
                    while (--count >= 0 && (unsigned)(s[i] - '0') < 8)
                        c = (c << 3) + (s[i++] - '0');
                    break;
                }
            default:
                --i;
                break;
            }
        }

        res.push_back(c);
    }

    return res;
}

bool
atf_check::run_status_check(const atf::check::check_result& r)
    const
{
    bool retval = true;

    if (!r.exited()) {
        std::cerr << "Fail: program did not exit cleanly" << std::endl;
        retval = false;
    } else {
        const int& status = r.exitcode();

        if (m_status_check == sc_equal) {
            if (m_status_arg != status) {
                std::cerr << "Fail: incorrect exit status: "
                          << status << ", expected: "
                          << m_status_arg << std::endl;
                retval = false;
            }
        } else if (m_status_check == sc_not_equal) {
            if (m_status_arg == status) {
                std::cerr << "Fail: incorrect exit status: "
                          << status << ", expected: "
                          << "anything other" << std::endl;
                retval = false;
            }
        }
    }

    if (retval == false) {
        std::cerr << "stdout:" << std::endl;
        print_file(r.stdout_path());
        std::cerr << std::endl;

        std::cerr << "stderr:" << std::endl;
        print_file(r.stderr_path());
        std::cerr << std::endl;
    }

    return retval;
}

bool
atf_check::run_output_check(const atf::check::check_result& r,
                            const std::string& stdxxx)
    const
{
    atf::fs::path path("/");
    std::string arg;
    output_check_t check = m_stdout_check;

    if (stdxxx == "stdout") {
        path = r.stdout_path();
        arg = m_stdout_arg.c_str();
        check = m_stdout_check;
    } else if (stdxxx == "stderr") {
        path = r.stderr_path();
        arg = m_stderr_arg.c_str();
        check = m_stderr_check;
    } else
        UNREACHABLE;

    if (check == oc_empty) {
        if (!file_empty(path)) {
            std::cerr << "Fail: incorrect " << stdxxx << std::endl;
            print_diff(atf::fs::path("/dev/null"), path);

            return false;
        }
    } else if (check == oc_file) {
        if (atf::io::cmp(path, atf::fs::path(arg)) != 0) {
            std::cerr << "Fail: incorrect " << stdxxx << std::endl;
            print_diff(atf::fs::path(arg), path);

            return false;
        }
    } else if (check == oc_inline) {
        atf::fs::path path2 = atf::fs::path(atf::config::get("atf_workdir"))
                              / "inline.XXXXXX";
        atf::fs::temp_file temp(path2);
        temp << decode(arg);
        temp.close();

        if (atf::io::cmp(path, temp.get_path()) != 0) {
            std::cerr << "Fail: incorrect " << stdxxx << std::endl;
            print_diff(temp.get_path(), path);

            return false;
        }
    } else if (check == oc_save) {
        std::ifstream ifs(path.c_str(), std::fstream::binary);
        ifs >> std::noskipws;
        std::istream_iterator< char > begin(ifs), end;

        std::ofstream ofs(arg.c_str(), std::fstream::binary
                                     | std::fstream::trunc);
        std::ostream_iterator <char> obegin(ofs);

        std::copy(begin, end, obegin);
    }

    return true;
}

std::string
atf_check::specific_args(void)
    const
{
    return "<command>";
}

atf_check::options_set
atf_check::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;

    opts.insert(option('s', "qual:value", "Handle status. Qualifier "
                "must be one of: ignore eq:<num> ne:<num>"));
    opts.insert(option('o', "action:arg", "Handle stdout. Action must be "
                "one of: empty ignore file:<path> inline:<val> save:<path>"));
    opts.insert(option('e', "action:arg", "Handle stderr. Action must be "
                "one of: empty ignore file:<path> inline:<val> save:<path>"));
    opts.insert(option('x', "", "Execute command as a shell command"));

    return opts;
}

void
atf_check::process_option_s(const std::string& arg)
{
    using atf::application::usage_error;

    if (arg == "ignore") {
        m_status_check = sc_ignore;
        return;
    }

    std::string::size_type pos = arg.find(':');
    std::string action = arg.substr(0, pos);

    if (action == "eq")
        m_status_check = sc_equal;
    else if (action == "ne")
        m_status_check = sc_not_equal;
    else
        throw usage_error("Invalid value for -s option");

    std::string value = arg.substr(pos + 1);

    try {
        m_status_arg = atf::text::to_type< unsigned int >(value);
    } catch (std::runtime_error) {
        throw usage_error("Invalid value for -s option; must be an "
                          "integer in range 0-255");
    }

    if ((m_status_arg < 0) || (m_status_arg > 255))
        throw usage_error("Invalid value for -s option; must be an "
                          "integer in range 0-255");

}

void
atf_check::process_option_o(const std::string& arg)
{
    using atf::application::usage_error;

    std::string::size_type pos = arg.find(':');
    std::string action = arg.substr(0, pos);

    if (action == "empty")
        m_stdout_check = oc_empty;
    else if (action == "ignore")
        m_stdout_check = oc_ignore;
    else if (action == "save")
        m_stdout_check = oc_save;
    else if (action == "inline")
        m_stdout_check = oc_inline;
    else if (action == "file")
        m_stdout_check = oc_file;
    else
        throw usage_error("Invalid value for -o option");

    m_stdout_arg = arg.substr(pos + 1);
}

void
atf_check::process_option_e(const std::string& arg)
{
    using atf::application::usage_error;

    std::string::size_type pos = arg.find(':');
    std::string action = arg.substr(0, pos);

    if (action == "empty")
        m_stderr_check = oc_empty;
    else if (action == "ignore")
        m_stderr_check = oc_ignore;
    else if (action == "save")
        m_stderr_check = oc_save;
    else if (action == "inline")
        m_stderr_check = oc_inline;
    else if (action == "file")
        m_stderr_check = oc_file;
    else
        throw usage_error("Invalid value for -e option");

    m_stderr_arg = arg.substr(pos + 1);
}

void
atf_check::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 's':
        process_option_s(arg);
        break;

    case 'o':
        process_option_o(arg);
        break;

    case 'e':
        process_option_e(arg);
        break;

    case 'x':
        m_xflag = true;
        break;

    default:
        UNREACHABLE;
    }
}

int
atf_check::main(void)
{
    if (m_argc < 1)
        throw atf::application::usage_error("No command specified");

    int status = EXIT_FAILURE;

    atf::check::check_result r =
        m_xflag ? execute_with_shell(m_argv) : execute(m_argv);

    if ((run_status_check(r) == false) ||
        (run_output_check(r, "stderr") == false) ||
        (run_output_check(r, "stdout") == false))
        status = EXIT_FAILURE;
    else
        status = EXIT_SUCCESS;

    return status;
}

int
main(int argc, char* const* argv)
{
    return atf_check().run(argc, argv);
}
