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
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
}

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "atf/application.hpp"
#include "atf/config.hpp"
#include "atf/env.hpp"
#include "atf/exceptions.hpp"
#include "atf/expand.hpp"
#include "atf/formats.hpp"
#include "atf/fs.hpp"
#include "atf/io.hpp"
#include "atf/sanity.hpp"
#include "atf/signals.hpp"
#include "atf/tests.hpp"
#include "atf/text.hpp"
#include "atf/ui.hpp"
#include "atf/user.hpp"

namespace impl = atf::tests;
#define IMPL_NAME "atf::tests"

// ------------------------------------------------------------------------
// Auxiliary stuff for the timeout implementation.
// ------------------------------------------------------------------------

namespace timeout {
    static pid_t current_body = 0;
    static bool killed = false;

    void
    sigalrm_handler(int signo)
    {
        PRE(signo == SIGALRM);

        if (current_body != 0) {
            ::killpg(current_body, SIGTERM);
            killed = true;
        }
    }
} // namespace timeout

// ------------------------------------------------------------------------
// The "vars_map" class.
// ------------------------------------------------------------------------

impl::vars_map::vars_map(void)
{
}

const std::string&
impl::vars_map::get(const std::string& key)
    const
{
    const_iterator iter = find(key);
    if (iter == end())
        throw std::runtime_error("Could not find variable `" + key +
                                 "' in map");
    return (*iter).second;
}

const std::string&
impl::vars_map::get(const std::string& key, const std::string& def)
    const
{
    const_iterator iter = find(key);
    return (iter == end()) ? def : (*iter).second;
}

bool
impl::vars_map::get_bool(const std::string& key)
    const
{
    bool val;

    std::string str = text::to_lower(get(key));
    if (str == "yes" || str == "true")
        val = true;
    else if (str == "no" || str == "false")
        val = false;
    else
        throw std::runtime_error("Invalid value for boolean variable `" +
                                 key + "'");

    return val;
}

bool
impl::vars_map::get_bool(const std::string& key, bool def)
    const
{
    bool val;

    const_iterator iter = find(key);
    if (iter == end())
        val = def;
    else {
        std::string str = text::to_lower((*iter).second);
        if (str == "yes" || str == "true")
            val = true;
        else if (str == "no" || str == "false")
            val = false;
        else
            throw std::runtime_error("Invalid value for boolean "
                                     "variable `" + key + "'");
    }

    return val;
}

bool
impl::vars_map::has(const std::string& key)
    const
{
    return find(key) != end();
}

impl::vars_map::value_type
impl::vars_map::parse(const std::string& str)
{
    if (str.empty())
        throw std::runtime_error("-v requires a non-empty argument");

    std::vector< std::string > ws = atf::text::split(str, "=");
    if (ws.size() == 1 && str[str.length() - 1] == '=') {
        return impl::vars_map::value_type(ws[0], "");
    } else {
        if (ws.size() != 2)
            throw std::runtime_error("-v requires an argument of the form "
                                     "var=value");

        return impl::vars_map::value_type(ws[0], ws[1]);
    }
}

// ------------------------------------------------------------------------
// The "tcr" class.
// ------------------------------------------------------------------------

impl::tcr::tcr(void) :
    m_status(status_failed),
    m_reason("Uninitialized test case result")
{
}

impl::tcr::tcr(impl::tcr::status s, const std::string& r) :
    m_status(s)
{
    if (r.find('\n') == std::string::npos)
        m_reason = r;
    else {
        m_reason = "BOGUS REASON (THE ORIGINAL HAD NEWLINES): ";
        for (std::string::size_type i = 0; i < r.length(); i++) {
            if (r[i] == '\n')
                m_reason += "<<NEWLINE>>";
            else if (r[i] != '\r')
                m_reason += r[i];
        }
    }
}

impl::tcr
impl::tcr::passed(void)
{
    return tcr(status_passed, "");
}

impl::tcr
impl::tcr::skipped(const std::string& reason)
{
    return tcr(status_skipped, reason);
}

impl::tcr
impl::tcr::failed(const std::string& reason)
{
    return tcr(status_failed, reason);
}

impl::tcr::status
impl::tcr::get_status(void)
    const
{
    return m_status;
}

const std::string&
impl::tcr::get_reason(void)
    const
{
    return m_reason;
}

// ------------------------------------------------------------------------
// The "tc" class.
// ------------------------------------------------------------------------

impl::tc::tc(const std::string& ident) :
    m_ident(ident)
{
}

impl::tc::~tc(void)
{
}

void
impl::tc::ensure_boolean(const std::string& name)
{
    ensure_not_empty(name);

    std::string val = text::to_lower(get(name));
    if (val == "yes" || val == "true")
        set(name, "true");
    else if (val == "no" || val == "false")
        set(name, "false");
    else
        throw std::runtime_error("Invalid value for boolean variable `" +
                                 name + "'");
}

void
impl::tc::ensure_integral(const std::string& name)
{
    ensure_not_empty(name);

    const std::string& val = get(name);
    for (std::string::const_iterator iter = val.begin(); iter != val.end();
         iter++) {
        if (!std::isdigit(*iter))
            throw std::runtime_error("Invalid value for integral "
                                     "variable `" + name + "'");
    }
}

void
impl::tc::ensure_not_empty(const std::string& name)
{
    if (m_meta_data.find(name) == m_meta_data.end())
        throw atf::not_found_error< std::string >
            ("Undefined or empty variable", name);

    vars_map::const_iterator iter = m_meta_data.find(name);
    INV(iter != m_meta_data.end());

    const std::string& val = (*iter).second;
    if (val.empty())
        throw atf::not_found_error< std::string > // XXX Incorrect error
            ("Undefined or empty variable", name);
}

void
impl::tc::set(const std::string& var, const std::string& val)
{
    m_meta_data[var] = val;
}

const std::string&
impl::tc::get(const std::string& var)
    const
{
    vars_map::const_iterator iter = m_meta_data.find(var);
    PRE(iter != m_meta_data.end());
    return (*iter).second;
}

bool
impl::tc::get_bool(const std::string& var)
    const
{
    std::string val = get(var);

    if (val == "true")
        return true;
    else if (val == "false")
        return false;
    else
        UNREACHABLE;
}

bool
impl::tc::has(const std::string& var)
    const
{
    vars_map::const_iterator iter = m_meta_data.find(var);
    return (iter != m_meta_data.end());
}

const impl::vars_map&
impl::tc::config(void)
    const
{
    return m_config;
}

const std::string&
impl::tc::get_srcdir(void)
    const
{
    return m_srcdir;
}

void
impl::tc::init(const vars_map& c, const std::string& srcdir)
{
    PRE(m_meta_data.empty());

    m_config = c; // XXX Uh, deep copy.  Should be a reference...
    m_srcdir = srcdir;

    m_meta_data["ident"] = m_ident;
    m_meta_data["timeout"] = "300";
    head();
    ensure_not_empty("descr");
    ensure_not_empty("ident");
    ensure_integral("timeout");
    INV(m_meta_data["ident"] == m_ident);
}

impl::tcr
impl::tc::safe_run(void)
    const
{
    atf::fs::temp_dir workdir
        (atf::fs::path(m_config.get("workdir")) / "atf.XXXXXX");
    impl::tcr tcr = fork_body(workdir.get_path().str());
    fork_cleanup(workdir.get_path().str());
    return tcr;
}

static void
sanitize_process(const atf::fs::path& workdir)
{
    // Reset all signal handlers to their default behavior.
    struct sigaction sadfl;
    sadfl.sa_handler = SIG_DFL;
    sigemptyset(&sadfl.sa_mask);
    sadfl.sa_flags = 0;
    for (int i = 0; i < atf::signals::last_signo; i++)
        ::sigaction(i, &sadfl, NULL);

    // Reset critical environment variables to known values.
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

    // Reset the umask.
    ::umask(S_IWGRP | S_IWOTH);

    // Set the work directory.
    atf::fs::change_directory(workdir);
}

void
impl::tc::check_requirements(void)
    const
{
    if (has("require.arch")) {
        const std::string& a = get("require.arch");
        std::vector< std::string > arches = text::split(a, " ");
        bool found = false;
        for (std::vector< std::string >::const_iterator iter = arches.begin();
             iter != arches.end() && !found; iter++) {
            if ((*iter) == atf::config::get("atf_arch"))
                found = true;
        }
        if (!a.empty() && !found)
            throw tcr::skipped("Requires one of the '" + a +
                               "' architectures");
    }

    if (has("require.machine")) {
        const std::string& m = get("require.machine");
        std::vector< std::string > machines = text::split(m, " ");
        bool found = false;
        for (std::vector< std::string >::const_iterator iter =
             machines.begin(); iter != machines.end() && !found; iter++) {
            if ((*iter) == atf::config::get("atf_machine"))
                found = true;
        }
        if (!m.empty() && !found)
            throw tcr::skipped("Requires one of the '" + m +
                               "' machine types");
    }

    if (has("require.config")) {
        const std::string& c = get("require.config");
        std::vector< std::string > vars = text::split(c, " ");
        for (std::vector< std::string >::const_iterator iter = vars.begin();
             iter != vars.end(); iter++) {
            if (!m_config.has(*iter))
                throw tcr::skipped("Required configuration variable " +
                                   *iter + " not defined");
        }
    }

    if (has("require.user")) {
        const std::string& u = get("require.user");
        if (u == "root") {
            if (!user::is_root())
                throw tcr::skipped("Requires root privileges");
        } else if (u == "unprivileged") {
            if (!user::is_unprivileged())
                throw tcr::skipped("Requires an unprivileged user");
        } else
            throw tcr::skipped("Invalid value in the require.user "
                               "property");
    }

    if (has("require.progs")) {
        std::vector< std::string > progs =
            text::split(get("require.progs"), " ");
        for (std::vector< std::string >::const_iterator iter =
             progs.begin(); iter != progs.end(); iter++)
            require_prog(*iter);
    }
}

static
void
program_timeout(pid_t pid, const std::string& tostr)
{
    PRE(pid != 0);

    INV(!tostr.empty());

    int timeout = atf::text::to_type< int >(tostr);

    if (timeout != 0) {
        struct itimerval itv;
        timerclear(&itv.it_interval);
        timerclear(&itv.it_value);
        itv.it_value.tv_sec = timeout;
        timeout::current_body = pid;
        ::setitimer(ITIMER_REAL, &itv, NULL);
    }
}

impl::tcr
impl::tc::fork_body(const std::string& workdir)
    const
{
    tcr tcr;

    fs::path result(fs::path(workdir) / "tc-result");

    pid_t pid = ::fork();
    if (pid == -1) {
        tcr = tcr::failed("Coult not fork to run test case");
    } else if (pid == 0) {
        int errcode;

        ::setpgid(::getpid(), 0);

        // Unexpected errors detected in the child process are mentioned
        // in stderr to give the user a chance to see what happened.
        // This is specially useful in those cases where these errors
        // cannot be directed to the parent process.
        try {
            sanitize_process(atf::fs::path(workdir));
            check_requirements();
            body();
            throw impl::tcr::passed();
        } catch (const impl::tcr& tcre) {
            std::ofstream os(result.c_str());
            if (!os) {
                std::cerr << "Could not open the temporary file " +
                             result.str() + " to leave the results in it"
                          << std::endl;
                errcode = EXIT_FAILURE;
            } else {
                if (tcre.get_status() == impl::tcr::status_passed)
                    os << "passed\n";
                else if (tcre.get_status() == impl::tcr::status_failed)
                    os << "failed\n" << tcre.get_reason() << '\n';
                else if (tcre.get_status() == impl::tcr::status_skipped)
                    os << "skipped\n" << tcre.get_reason() << '\n';
                os.close();
                errcode = EXIT_SUCCESS;
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Caught unexpected error: " << e.what() << std::endl;
            errcode = EXIT_FAILURE;
        } catch (...) {
            std::cerr << "Caught unexpected error" << std::endl;
            errcode = EXIT_FAILURE;
        }
        std::exit(errcode);
    } else {
        // Program the timeout handler.
        timeout::current_body = 0;
        timeout::killed = false;
        atf::signals::signal_programmer sigalrm(SIGALRM,
                                                timeout::sigalrm_handler);
        program_timeout(pid, get("timeout"));

        // Wait for the child and deal with its termination status.
        int status;
        if (::waitpid(pid, &status, 0) != pid) {
            if (errno == EINTR && timeout::killed)
                tcr = tcr::failed("Test case timed out after " +
                                  get("timeout") + " seconds");
            else
                tcr = tcr::failed("Error while waiting for process " +
                                  atf::text::to_string(pid) + ": " +
                                  ::strerror(errno));
        } else {
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == EXIT_SUCCESS) {
                    std::ifstream is(result.c_str());
                    if (!is) {
                        tcr = tcr::failed("Could not open results file for "
                                          "reading");
                    } else {
                        std::string line;
                        std::getline(is, line);
                        if (line == "passed") {
                            tcr = tcr::passed();
                        } else if (line == "failed") {
                            std::getline(is, line);
                            tcr = tcr::failed(line);
                        } else if (line == "skipped") {
                            std::getline(is, line);
                            tcr = tcr::skipped(line);
                        } else {
                            tcr = tcr::failed("Test case failed to report its "
                                              "status");
                        }
                        is.close();
                    }
                } else
                    tcr = tcr::failed("Test case returned non-OK status; "
                                      "see its stderr output for more "
                                      "details");
            } else if (WIFSIGNALED(status)) {
                tcr = tcr::failed("Test case received signal " +
                                  atf::text::to_string(WTERMSIG(status)));
            } else
                UNREACHABLE;
        }
    }

    return tcr;
}

void
impl::tc::fork_cleanup(const std::string& workdir)
    const
{
    pid_t pid = ::fork();
    if (pid == -1) {
        std::cerr << "WARNING: Could not fork to run test case's cleanup "
                     "routine for " << workdir << std::endl;
    } else if (pid == 0) {
        int errcode = EXIT_FAILURE;
        try {
            sanitize_process(atf::fs::path(workdir));
            cleanup();
            errcode = EXIT_SUCCESS;
        } catch (const std::exception& e) {
            std::cerr << "WARNING: Caught unexpected exception while "
                         "running the test case's cleanup routine: "
                      << e.what() << std::endl;
        } catch (...) {
            std::cerr << "WARNING: Caught unknown exception while "
                         "running the test case's cleanup routine"
                      << std::endl;
        }
        std::exit(errcode);
    } else {
        int status;
        if (::waitpid(pid, &status, 0) == -1)
            std::cerr << "WARNING: Error while waiting for cleanup process "
                      << atf::text::to_string(pid) << ": "
                      << ::strerror(errno) << std::endl;
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
            std::cerr << "WARNING: Cleanup process ended unexpectedly"
                      << std::endl;
    }
}

impl::tcr
impl::tc::run(void)
    const
{
    PRE(!m_meta_data.empty());

    tcr tcr;

    try {
        tcr = safe_run();
    } catch (const std::exception& e) {
        tcr = tcr::failed(std::string("Unhandled exception: ") +
                          e.what());
    } catch (...) {
        tcr = tcr::failed("Unknown unhandled exception");
    }

    return tcr;
}

void
impl::tc::cleanup(void)
    const
{
}

void
impl::tc::require_prog(const std::string& prog)
    const
{
    PRE(!prog.empty());

    fs::path p(prog);

    if (p.is_absolute()) {
        if (!fs::is_executable(p))
            throw impl::tcr::skipped("The required program " + prog +
                                     " could not be found");
    } else {
        INV(p.branch_path() == fs::path("."));
        if (fs::find_prog_in_path(prog).empty())
            throw impl::tcr::skipped("The required program " + prog +
                                     " could not be found in the PATH");
    }
}

// ------------------------------------------------------------------------
// The "tp" class.
// ------------------------------------------------------------------------

class tp : public atf::application::app {
public:
    typedef std::vector< impl::tc * > tc_vector;

private:
    static const char* m_description;

    bool m_lflag;
    int m_results_fd;
    std::auto_ptr< std::ostream > m_results_os;
    atf::fs::path m_srcdir;
    std::set< std::string > m_tcnames;

    atf::tests::vars_map m_vars;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

    tc_vector m_tcs;

    tc_vector init_tcs(void);
    static tc_vector filter_tcs(tc_vector, const std::set< std::string >&);

    std::ostream& results_stream(void);

    int list_tcs(void);
    int run_tcs(void);

public:
    tp(const tc_vector&);

    int main(void);
};

const char* tp::m_description =
    "This is an independent atf test program.";

tp::tp(const tc_vector& tcs) :
    app(m_description, "atf-test-program(1)", "atf(7)"),
    m_lflag(false),
    m_results_fd(STDOUT_FILENO),
    m_srcdir(atf::fs::get_current_dir()),
    m_tcs(tcs)
{
    m_vars["workdir"] = atf::config::get("atf_workdir");
}

std::string
tp::specific_args(void)
    const
{
    return "[test_case1 [.. test_caseN]]";
}

tp::options_set
tp::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('l', "", "List test cases and their purpose"));
    opts.insert(option('r', "fd", "The file descriptor to which the test "
                                  "program will send the results of the "
                                  "test cases"));
    opts.insert(option('s', "srcdir", "Directory where the test's data "
                                      "files are located"));
    opts.insert(option('v', "var=value", "Sets the configuration variable "
                                         "`var' to `value'"));
    return opts;
}

void
tp::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'l':
        m_lflag = true;
        break;

    case 'r':
        {
            std::istringstream ss(arg);
            ss >> m_results_fd;
        }
        break;

    case 's':
        m_srcdir = atf::fs::path(arg);
        break;

    case 'v':
        {
            atf::tests::vars_map::value_type v =
                atf::tests::vars_map::parse(arg);
            m_vars[v.first] = v.second;
        }
        break;

    default:
        UNREACHABLE;
    }
}

tp::tc_vector
tp::init_tcs(void)
{
    tc_vector tcs = m_tcs;

    for (tc_vector::iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        impl::tc* tc = *iter;

        tc->init(m_vars, m_srcdir.str());
    }

    return tcs;
}

//
// An auxiliary unary predicate that compares the given test case's
// identifier to the identifier stored in it.
//
class tc_equal_to_ident {
    const std::string& m_ident;

public:
    tc_equal_to_ident(const std::string& i) :
        m_ident(i)
    {
    }

    bool operator()(const impl::tc* tc)
    {
        return tc->get("ident") == m_ident;
    }
};

tp::tc_vector
tp::filter_tcs(tc_vector tcs, const std::set< std::string >& tcnames)
{
    tc_vector tcso;

    if (tcnames.empty()) {
        // Special case: added for efficiency because this is the most
        // typical situation.
        tcso = tcs;
    } else {
        // Collect all the test cases' identifiers.
        std::set< std::string > ids;
        for (tc_vector::iterator iter = tcs.begin();
             iter != tcs.end(); iter++) {
            impl::tc* tc = *iter;

            ids.insert(tc->get("ident"));
        }

        // Iterate over all names provided by the user and, for each one,
        // expand it as if it were a glob pattern.  Collect all expansions.
        std::set< std::string > exps;
        for (std::set< std::string >::const_iterator iter = tcnames.begin();
             iter != tcnames.end(); iter++) {
            const std::string& glob = *iter;

            std::set< std::string > ms = atf::expand::expand_glob(glob, ids);
            if (ms.empty())
                throw std::runtime_error("Unknown test case `" + glob + "'");
            exps.insert(ms.begin(), ms.end());
        }

        // For each expansion, locate its corresponding test case and add
        // it to the output set.
        for (std::set< std::string >::const_iterator iter = exps.begin();
             iter != exps.end(); iter++) {
            const std::string& name = *iter;

            tc_vector::iterator tciter =
                std::find_if(tcs.begin(), tcs.end(), tc_equal_to_ident(name));
            INV(tciter != tcs.end());
            tcso.push_back(*tciter);
        }
    }

    return tcso;
}

int
tp::list_tcs(void)
{
    tc_vector tcs = filter_tcs(init_tcs(), m_tcnames);

    std::string::size_type maxlen = 0;
    for (tc_vector::const_iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        const impl::tc* tc = *iter;

        if (maxlen < tc->get("ident").length())
            maxlen = tc->get("ident").length();
    }

    for (tc_vector::const_iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        const impl::tc* tc = *iter;

        std::cout << atf::ui::format_text_with_tag(tc->get("descr"),
                                                   tc->get("ident"),
                                                   false, maxlen + 4)
                  << std::endl;
    }

    return EXIT_SUCCESS;
}

std::ostream&
tp::results_stream(void)
{
    if (m_results_fd == STDOUT_FILENO)
        return std::cout;
    else if (m_results_fd == STDERR_FILENO)
        return std::cerr;
    else
        return *m_results_os;
}

int
tp::run_tcs(void)
{
    tc_vector tcs = filter_tcs(init_tcs(), m_tcnames);

    if (!atf::fs::exists(atf::fs::path(m_vars.get("workdir"))))
        throw std::runtime_error("Cannot find the work directory `" +
                                 m_vars.get("workdir") + "'");

    int errcode = EXIT_SUCCESS;

    atf::signals::signal_holder sighup(SIGHUP);
    atf::signals::signal_holder sigint(SIGINT);
    atf::signals::signal_holder sigterm(SIGTERM);

    atf::formats::atf_tcs_writer w(results_stream(), std::cout, std::cerr,
                                   tcs.size());
    for (tc_vector::iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        impl::tc* tc = *iter;

        w.start_tc(tc->get("ident"));
        impl::tcr tcr = tc->run();
        w.end_tc(tcr);

        sighup.process();
        sigint.process();
        sigterm.process();

        if (tcr.get_status() == impl::tcr::status_failed)
            errcode = EXIT_FAILURE;
    }

    return errcode;
}

int
tp::main(void)
{
    int errcode;

    if (!atf::fs::exists(m_srcdir / m_prog_name))
        throw std::runtime_error("Cannot find the test program in the "
                                 "source directory `" + m_srcdir.str() + "'");

    for (int i = 0; i < m_argc; i++)
        m_tcnames.insert(m_argv[i]);

    if (m_lflag)
        errcode = list_tcs();
    else {
        if (m_results_fd != STDOUT_FILENO && m_results_fd != STDERR_FILENO) {
            atf::io::file_handle fh(m_results_fd);
            m_results_os =
                std::auto_ptr< std::ostream >(new atf::io::postream(fh));
        }
        errcode = run_tcs();
    }

    return errcode;
}

namespace atf {
    namespace tests {
        int run_tp(int, char* const*, const tp::tc_vector&);
    }
}

int
impl::run_tp(int argc, char* const* argv, const tp::tc_vector& tcs)
{
    return tp(tcs).run(argc, argv);
}
