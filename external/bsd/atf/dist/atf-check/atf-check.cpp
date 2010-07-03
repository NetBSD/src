//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008, 2009, 2010 The NetBSD Foundation, Inc.
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

#include <limits.h>
#include <signal.h>
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

namespace {

enum status_check_t {
    sc_exit,
    sc_ignore,
    sc_signal,
};

struct status_check {
    status_check_t type;
    bool negated;
    int value;

    status_check(const status_check_t& p_type, const bool p_negated,
                 const int p_value) :
        type(p_type),
        negated(p_negated),
        value(p_value)
    {
    }
};

enum output_check_t {
    oc_ignore,
    oc_inline,
    oc_file,
    oc_empty,
    oc_match,
    oc_save
};

struct output_check {
    output_check_t type;
    bool negated;
    std::string value;

    output_check(const output_check_t& p_type, const bool p_negated,
                 const std::string& p_value) :
        type(p_type),
        negated(p_negated),
        value(p_value)
    {
    }
};

} // anonymous namespace

static int
parse_exit_code(const std::string& str)
{
    try {
        const int value = atf::text::to_type< int >(str);
        if (value < 0 || value > 255)
            throw std::runtime_error("Unused reason");
        return value;
    } catch (const std::runtime_error& e) {
        throw atf::application::usage_error("Invalid exit code for -s option; "
            "must be an integer in range 0-255");
    }
}

static struct name_number {
    const char *name;
    int signo;
} signal_names_to_numbers[] = {
    { "hup", SIGHUP },
    { "int", SIGINT },
    { "quit", SIGQUIT },
    { "trap", SIGTRAP },
    { "kill", SIGKILL },
    { "segv", SIGSEGV },
    { "pipe", SIGPIPE },
    { "alrm", SIGALRM },
    { "term", SIGTERM },
    { "usr1", SIGUSR1 },
    { "usr2", SIGUSR2 },
    { NULL, INT_MIN },
};

static int
signal_name_to_number(const std::string& str)
{
    struct name_number* iter = signal_names_to_numbers;
    int signo = INT_MIN;
    while (signo == INT_MIN && iter->name != NULL) {
        if (str == iter->name || str == std::string("sig") + iter->name)
            signo = iter->signo;
        else
            iter++;
    }
    return signo;
}

static int
parse_signal(const std::string& str)
{
    const int signo = signal_name_to_number(str);
    if (signo == INT_MIN) {
        try {
            return atf::text::to_type< int >(str);
        } catch (std::runtime_error) {
            throw atf::application::usage_error("Invalid signal name or number "
                "in -s option");
        }
    }
    INV(signo != INT_MIN);
    return signo;
}

static status_check
parse_status_check_arg(const std::string& arg)
{
    const std::string::size_type delimiter = arg.find(':');
    bool negated = (arg.compare(0, 4, "not-") == 0);
    const std::string action_str = arg.substr(0, delimiter);
    const std::string action = negated ? action_str.substr(4) : action_str;
    const std::string value_str = (
        delimiter == std::string::npos ? "" : arg.substr(delimiter + 1));
    int value;

    status_check_t type;
    if (action == "eq") {
        // Deprecated; use exit instead.  TODO: Remove after 0.10.
        type = sc_exit;
        if (negated)
            throw atf::application::usage_error("Cannot negate eq checker");
        negated = false;
        value = parse_exit_code(value_str);
    } else if (action == "exit") {
        type = sc_exit;
        if (value_str.empty())
            value = INT_MIN;
        else
            value = parse_exit_code(value_str);
    } else if (action == "ignore") {
        if (negated)
            throw atf::application::usage_error("Cannot negate ignore checker");
        type = sc_ignore;
        value = INT_MIN;
    } else if (action == "ne") {
        // Deprecated; use not-exit instead.  TODO: Remove after 0.10.
        type = sc_exit;
        if (negated)
            throw atf::application::usage_error("Cannot negate ne checker");
        negated = true;
        value = parse_exit_code(value_str);
    } else if (action == "signal") {
        type = sc_signal;
        if (value_str.empty())
            value = INT_MIN;
        else
            value = parse_signal(value_str);
    } else
        throw atf::application::usage_error("Invalid output checker");

    return status_check(type, negated, value);
}

static
output_check
parse_output_check_arg(const std::string& arg)
{
    const std::string::size_type delimiter = arg.find(':');
    const bool negated = (arg.compare(0, 4, "not-") == 0);
    const std::string action_str = arg.substr(0, delimiter);
    const std::string action = negated ? action_str.substr(4) : action_str;

    output_check_t type;
    if (action == "empty")
        type = oc_empty;
    else if (action == "file")
        type = oc_file;
    else if (action == "ignore") {
        if (negated)
            throw atf::application::usage_error("Cannot negate ignore checker");
        type = oc_ignore;
    } else if (action == "inline")
        type = oc_inline;
    else if (action == "match")
        type = oc_match;
    else if (action == "save") {
        if (negated)
            throw atf::application::usage_error("Cannot negate save checker");
        type = oc_save;
    } else
        throw atf::application::usage_error("Invalid output checker");

    return output_check(type, negated, arg.substr(delimiter + 1));
}

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

static
void
cat_file(const atf::fs::path& path)
{
    std::ifstream stream(path.c_str());
    if (!stream)
        throw std::runtime_error("Failed to open " + path.str());

    stream >> std::noskipws;
    std::istream_iterator< char > begin(stream), end;
    std::ostream_iterator< char > out(std::cerr);
    std::copy(begin, end, out);

    stream.close();
}

static
bool
grep_file(const atf::fs::path& path, const std::string& regexp)
{
    std::ifstream stream(path.c_str());
    if (!stream)
        throw std::runtime_error("Failed to open " + path.str());

    bool found = false;

    std::string line;
    while (!found && std::getline(stream, line).good()) {
        if (atf::text::match(line, regexp))
            found = true;
    }

    stream.close();

    return found;
}

static
bool
file_empty(const atf::fs::path& p)
{
    atf::fs::file_info f(p);

    return (f.get_size() == 0);
}

static
void
print_diff(const atf::fs::path& p1, const atf::fs::path& p2)
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

static
std::string
decode(const std::string& s)
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

static
bool
run_status_check(const status_check& sc, const atf::check::check_result& cr)
{
    bool result;

    if (sc.type == sc_exit) {
        if (cr.exited() && sc.value != INT_MIN) {
            const int status = cr.exitcode();

            if (!sc.negated && sc.value != status) {
                std::cerr << "Fail: incorrect exit status: "
                          << status << ", expected: "
                          << sc.value << std::endl;
                result = false;
            } else if (sc.negated && sc.value == status) {
                std::cerr << "Fail: incorrect exit status: "
                          << status << ", expected: "
                          << "anything else" << std::endl;
                result = false;
            } else
                result = true;
        } else if (cr.exited() && sc.value == INT_MIN) {
            result = true;
        } else {
            std::cerr << "Fail: program did not exit cleanly" << std::endl;
            result = false;
        }
    } else if (sc.type == sc_ignore) {
        result = true;
    } else if (sc.type == sc_signal) {
        if (cr.signaled() && sc.value != INT_MIN) {
            const int status = cr.termsig();

            if (!sc.negated && sc.value != status) {
                std::cerr << "Fail: incorrect signal received: "
                          << status << ", expected: "
                          << sc.value << std::endl;
                result = false;
            } else if (sc.negated && sc.value == status) {
                std::cerr << "Fail: incorrect signal received: "
                          << status << ", expected: "
                          << "anything else" << std::endl;
                result = false;
            } else
                result = true;
        } else if (cr.signaled() && sc.value == INT_MIN) {
            result = true;
        } else {
            std::cerr << "Fail: program did not receive a signal" << std::endl;
            result = false;
        }
    } else {
        UNREACHABLE;
        result = false;
    }

    if (result == false) {
        std::cerr << "stdout:" << std::endl;
        cat_file(cr.stdout_path());
        std::cerr << std::endl;

        std::cerr << "stderr:" << std::endl;
        cat_file(cr.stderr_path());
        std::cerr << std::endl;
    }

    return result;
}

static
bool
run_status_checks(const std::vector< status_check >& checks,
                  const atf::check::check_result& result)
{
    bool ok = false;

    for (std::vector< status_check >::const_iterator iter = checks.begin();
         !ok && iter != checks.end(); iter++) {
         ok |= run_status_check(*iter, result);
    }

    return ok;
}

static
bool
run_output_check(const output_check oc, const atf::fs::path& path,
                 const std::string& stdxxx)
{
    bool result;

    if (oc.type == oc_empty) {
        const bool is_empty = file_empty(path);
        if (!oc.negated && !is_empty) {
            std::cerr << "Fail: " << stdxxx << " not empty" << std::endl;
            print_diff(atf::fs::path("/dev/null"), path);
            result = false;
        } else if (oc.negated && is_empty) {
            std::cerr << "Fail: " << stdxxx << " is empty" << std::endl;
            result = false;
        } else
            result = true;
    } else if (oc.type == oc_file) {
        const bool equals = atf::io::cmp(path, atf::fs::path(oc.value));
        if (!oc.negated && !equals) {
            std::cerr << "Fail: " << stdxxx << " does not match golden output"
                      << std::endl;
            print_diff(atf::fs::path(oc.value), path);
            result = false;
        } else if (oc.negated && equals) {
            std::cerr << "Fail: " << stdxxx << " matches golden output"
                      << std::endl;
            cat_file(atf::fs::path(oc.value));
            result = false;
        } else
            result = true;
    } else if (oc.type == oc_ignore) {
        result = true;
    } else if (oc.type == oc_inline) {
        atf::fs::path path2 = atf::fs::path(atf::config::get("atf_workdir"))
                              / "inline.XXXXXX";
        atf::fs::temp_file temp(path2);
        temp << decode(oc.value);
        temp.close();

        const bool equals = atf::io::cmp(path, temp.get_path());
        if (!oc.negated && !equals) {
            std::cerr << "Fail: " << stdxxx << " does not match expected value"
                      << std::endl;
            print_diff(temp.get_path(), path);
            result = false;
        } else if (oc.negated && equals) {
            std::cerr << "Fail: " << stdxxx << " matches expected value"
                      << std::endl;
            cat_file(temp.get_path());
            result = false;
        } else
            result = true;
    } else if (oc.type == oc_match) {
        const bool matches = grep_file(path, oc.value);
        if (!oc.negated && !matches) {
            std::cerr << "Fail: regexp " + oc.value + " not in " << stdxxx
                      << std::endl;
            cat_file(path);
            result = false;
        } else if (oc.negated && matches) {
            std::cerr << "Fail: regexp " + oc.value + " is in " << stdxxx
                      << std::endl;
            cat_file(path);
            result = false;
        } else
            result = true;
    } else if (oc.type == oc_save) {
        INV(!oc.negated);
        std::ifstream ifs(path.c_str(), std::fstream::binary);
        ifs >> std::noskipws;
        std::istream_iterator< char > begin(ifs), end;

        std::ofstream ofs(oc.value.c_str(), std::fstream::binary
                                     | std::fstream::trunc);
        std::ostream_iterator <char> obegin(ofs);

        std::copy(begin, end, obegin);
        result = true;
    } else {
        UNREACHABLE;
        result = false;
    }

    return result;
}

static
bool
run_output_checks(const std::vector< output_check >& checks,
                  const atf::fs::path& path, const std::string& stdxxx)
{
    bool ok = true;

    for (std::vector< output_check >::const_iterator iter = checks.begin();
         iter != checks.end(); iter++) {
         ok &= run_output_check(*iter, path, stdxxx);
    }

    return ok;
}

// ------------------------------------------------------------------------
// The "atf_check" application.
// ------------------------------------------------------------------------

class atf_check : public atf::application::app {
    bool m_xflag;

    std::vector< status_check > m_status_checks;
    std::vector< output_check > m_stdout_checks;
    std::vector< output_check > m_stderr_checks;

    static const char* m_description;

    bool run_output_checks(const atf::check::check_result&,
                           const std::string&) const;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);
    void process_option_s(const std::string&);

public:
    atf_check(void);
    int main(void);
};

const char* atf_check::m_description =
    "atf-check executes given command and analyzes its results.";

atf_check::atf_check(void) :
    app(m_description, "atf-check(1)", "atf(7)"),
    m_xflag(false)
{
}

bool
atf_check::run_output_checks(const atf::check::check_result& r,
                             const std::string& stdxxx)
    const
{
    if (stdxxx == "stdout") {
        return ::run_output_checks(m_stdout_checks, r.stdout_path(), "stdout");
    } else if (stdxxx == "stderr") {
        return ::run_output_checks(m_stderr_checks, r.stderr_path(), "stderr");
    } else {
        UNREACHABLE;
        return false;
    }
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
                "must be one of: ignore exit:<num> signal:<name|num>"));
    opts.insert(option('o', "action:arg", "Handle stdout. Action must be "
                "one of: empty ignore file:<path> inline:<val> match:regexp "
                "save:<path>"));
    opts.insert(option('e', "action:arg", "Handle stderr. Action must be "
                "one of: empty ignore file:<path> inline:<val> match:regexp "
                "save:<path>"));
    opts.insert(option('x', "", "Execute command as a shell command"));

    return opts;
}

void
atf_check::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 's':
        m_status_checks.push_back(parse_status_check_arg(arg));
        break;

    case 'o':
        m_stdout_checks.push_back(parse_output_check_arg(arg));
        break;

    case 'e':
        m_stderr_checks.push_back(parse_output_check_arg(arg));
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

    if (m_status_checks.empty())
        m_status_checks.push_back(status_check(sc_exit, false, EXIT_SUCCESS));
    else if (m_status_checks.size() > 1) {
        // TODO: Remove this restriction.
        throw atf::application::usage_error("Cannot specify -s more than once");
    }

    if (m_stdout_checks.empty())
        m_stdout_checks.push_back(output_check(oc_empty, false, ""));
    if (m_stderr_checks.empty())
        m_stderr_checks.push_back(output_check(oc_empty, false, ""));

    if ((run_status_checks(m_status_checks, r) == false) ||
        (run_output_checks(r, "stderr") == false) ||
        (run_output_checks(r, "stdout") == false))
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
