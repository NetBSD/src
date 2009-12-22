//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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

extern "C" {
#include "atf-c/error.h"
#include "atf-c/object.h"
}

#include "atf-c++/application.hpp"
#include "atf-c++/config.hpp"
#include "atf-c++/env.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/expand.hpp"
#include "atf-c++/formats.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/signals.hpp"
#include "atf-c++/tests.hpp"
#include "atf-c++/text.hpp"
#include "atf-c++/ui.hpp"
#include "atf-c++/user.hpp"

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
// The "tcr" class.
// ------------------------------------------------------------------------

const impl::tcr::state impl::tcr::passed_state = atf_tcr_passed_state;
const impl::tcr::state impl::tcr::failed_state = atf_tcr_failed_state;
const impl::tcr::state impl::tcr::skipped_state = atf_tcr_skipped_state;

impl::tcr::tcr(state s)
{
    PRE(s == passed_state);

    atf_error_t err = atf_tcr_init(&m_tcr, s);
    if (atf_is_error(err))
        throw_atf_error(err);
}

impl::tcr::tcr(state s, const std::string& r)
{
    PRE(s == failed_state || s == skipped_state);
    PRE(!r.empty());

    atf_error_t err = atf_tcr_init_reason_fmt(&m_tcr, s, "%s", r.c_str());
    if (atf_is_error(err))
        throw_atf_error(err);
}

impl::tcr::tcr(const tcr& o)
{
    if (o.get_state() == passed_state)
        atf_tcr_init(&m_tcr, o.get_state());
    else
        atf_tcr_init_reason_fmt(&m_tcr, o.get_state(), "%s",
                                o.get_reason().c_str());
}

impl::tcr::~tcr(void)
{
    atf_tcr_fini(&m_tcr);
}

impl::tcr::state
impl::tcr::get_state(void)
    const
{
    return atf_tcr_get_state(&m_tcr);
}

const std::string
impl::tcr::get_reason(void)
    const
{
    const atf_dynstr_t* r = atf_tcr_get_reason(&m_tcr);
    return atf_dynstr_cstring(r);
}

impl::tcr&
impl::tcr::operator=(const tcr& o)
{
    if (this != &o) {
        atf_tcr_fini(&m_tcr);

        if (o.get_state() == passed_state)
            atf_tcr_init(&m_tcr, o.get_state());
        else
            atf_tcr_init_reason_fmt(&m_tcr, o.get_state(), "%s",
                                    o.get_reason().c_str());
    }
    return *this;
}

// ------------------------------------------------------------------------
// The "tc" class.
// ------------------------------------------------------------------------

static std::map< atf_tc_t*, impl::tc* > wraps;
static std::map< const atf_tc_t*, const impl::tc* > cwraps;

void
impl::tc::wrap_head(atf_tc_t *tc)
{
    std::map< atf_tc_t*, impl::tc* >::iterator iter = wraps.find(tc);
    INV(iter != wraps.end());
    (*iter).second->head();
}

void
impl::tc::wrap_body(const atf_tc_t *tc)
{
    std::map< const atf_tc_t*, const impl::tc* >::const_iterator iter =
        cwraps.find(tc);
    INV(iter != cwraps.end());
    (*iter).second->body();
}

void
impl::tc::wrap_cleanup(const atf_tc_t *tc)
{
    std::map< const atf_tc_t*, const impl::tc* >::const_iterator iter =
        cwraps.find(tc);
    INV(iter != cwraps.end());
    (*iter).second->cleanup();
}

impl::tc::tc(const std::string& ident) :
    m_ident(ident)
{
}

impl::tc::~tc(void)
{
    cwraps.erase(&m_tc);
    wraps.erase(&m_tc);

    atf_tc_fini(&m_tc);
    atf_map_fini(&m_config);
}

void
impl::tc::init(const vars_map& config)
{
    atf_error_t err;

    err = atf_map_init(&m_config);
    if (atf_is_error(err))
        throw_atf_error(err);

    for (vars_map::const_iterator iter = config.begin();
         iter != config.end(); iter++) {
        const char *var = (*iter).first.c_str();
        const char *val = (*iter).second.c_str();

        err = atf_map_insert(&m_config, var, ::strdup(val), true);
        if (atf_is_error(err)) {
            atf_map_fini(&m_config);
            throw_atf_error(err);
        }
    }

    wraps[&m_tc] = this;
    cwraps[&m_tc] = this;

    err = atf_tc_init(&m_tc, m_ident.c_str(), wrap_head, wrap_body,
                      wrap_cleanup, &m_config);
    if (atf_is_error(err)) {
        atf_map_fini(&m_config);
        throw_atf_error(err);
    }
}

bool
impl::tc::has_config_var(const std::string& var)
    const
{
    return atf_tc_has_config_var(&m_tc, var.c_str());
}

bool
impl::tc::has_md_var(const std::string& var)
    const
{
    return atf_tc_has_md_var(&m_tc, var.c_str());
}

const std::string
impl::tc::get_config_var(const std::string& var)
    const
{
    return atf_tc_get_config_var(&m_tc, var.c_str());
}

const std::string
impl::tc::get_config_var(const std::string& var, const std::string& defval)
    const
{
    return atf_tc_get_config_var_wd(&m_tc, var.c_str(), defval.c_str());
}

const std::string
impl::tc::get_md_var(const std::string& var)
    const
{
    return atf_tc_get_md_var(&m_tc, var.c_str());
}

void
impl::tc::set_md_var(const std::string& var, const std::string& val)
{
    atf_error_t err = atf_tc_set_md_var(&m_tc, var.c_str(), val.c_str());
    if (atf_is_error(err))
        throw_atf_error(err);
}

impl::tcr
impl::tc::run(int fdout, int fderr, const fs::path& workdirbase)
    const
{
    atf_tcr_t tcrc;
    tcr tcrr(tcr::failed_state, "UNINITIALIZED");

    atf_error_t err = atf_tc_run(&m_tc, &tcrc, fdout, fderr,
                                 workdirbase.c_path());
    if (atf_is_error(err))
        throw_atf_error(err);

    if (atf_tcr_has_reason(&tcrc)) {
        const atf_dynstr_t* r = atf_tcr_get_reason(&tcrc);
        tcrr = tcr(atf_tcr_get_state(&tcrc), atf_dynstr_cstring(r));
    } else {
        tcrr = tcr(atf_tcr_get_state(&tcrc));
    }

    atf_tcr_fini(&tcrc);
    return tcrr;
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
    atf_tc_require_prog(prog.c_str());
}

void
impl::tc::pass(void)
{
    atf_tc_pass();
}

void
impl::tc::fail(const std::string& reason)
{
    atf_tc_fail("%s", reason.c_str());
}

void
impl::tc::skip(const std::string& reason)
{
    atf_tc_skip("%s", reason.c_str());
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
    atf::fs::path m_workdir;
    std::vector< std::string > m_tcnames;

    atf::tests::vars_map m_vars;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

    void (*m_add_tcs)(tc_vector&);
    tc_vector m_tcs;

    void parse_vflag(const std::string&);
    void handle_srcdir(void);

    tc_vector init_tcs(void);
    static tc_vector filter_tcs(tc_vector,
                                const std::vector< std::string >&);

    std::ostream& results_stream(void);

    int list_tcs(void);
    int run_tcs(void);

public:
    tp(void (*)(tc_vector&));
    ~tp(void);

    int main(void);
};

const char* tp::m_description =
    "This is an independent atf test program.";

tp::tp(void (*add_tcs)(tc_vector&)) :
    app(m_description, "atf-test-program(1)", "atf(7)"),
    m_lflag(false),
    m_results_fd(STDOUT_FILENO),
    m_srcdir("."),
    m_workdir(atf::config::get("atf_workdir")),
    m_add_tcs(add_tcs)
{
}

tp::~tp(void)
{
    for (tc_vector::iterator iter = m_tcs.begin();
         iter != m_tcs.end(); iter++) {
        impl::tc* tc = *iter;

        delete tc;
    }
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
    opts.insert(option('w', "workdir", "Directory where the test's "
                                       "temporary files are located"));
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
        parse_vflag(arg);
        break;

    case 'w':
        m_workdir = atf::fs::path(arg);
        break;

    default:
        UNREACHABLE;
    }
}

void
tp::parse_vflag(const std::string& str)
{
    if (str.empty())
        throw std::runtime_error("-v requires a non-empty argument");

    std::vector< std::string > ws = atf::text::split(str, "=");
    if (ws.size() == 1 && str[str.length() - 1] == '=') {
        m_vars[ws[0]] = "";
    } else {
        if (ws.size() != 2)
            throw std::runtime_error("-v requires an argument of the form "
                                     "var=value");

        m_vars[ws[0]] = ws[1];
    }
}

void
tp::handle_srcdir(void)
{
    if (!atf::fs::exists(m_srcdir / m_prog_name))
        throw std::runtime_error("Cannot find the test program in the "
                                 "source directory `" + m_srcdir.str() + "'");

    if (!m_srcdir.is_absolute())
        m_srcdir = m_srcdir.to_absolute();

    m_vars["srcdir"] = m_srcdir.str();
}

tp::tc_vector
tp::init_tcs(void)
{
    m_add_tcs(m_tcs);
    for (tc_vector::iterator iter = m_tcs.begin();
         iter != m_tcs.end(); iter++) {
        impl::tc* tc = *iter;

        tc->init(m_vars);
    }
    return m_tcs;
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
        return tc->get_md_var("ident") == m_ident;
    }
};

tp::tc_vector
tp::filter_tcs(tc_vector tcs, const std::vector< std::string >& tcnames)
{
    tc_vector tcso;

    if (tcnames.empty()) {
        // Special case: added for efficiency because this is the most
        // typical situation.
        tcso = tcs;
    } else {
        // Collect all the test cases' identifiers.
        std::vector< std::string > ids;
        for (tc_vector::iterator iter = tcs.begin();
             iter != tcs.end(); iter++) {
            impl::tc* tc = *iter;

            ids.push_back(tc->get_md_var("ident"));
        }

        // Iterate over all names provided by the user and, for each one,
        // expand it as if it were a glob pattern.  Collect all expansions.
        std::vector< std::string > exps;
        for (std::vector< std::string >::const_iterator iter = tcnames.begin();
             iter != tcnames.end(); iter++) {
            const std::string& glob = *iter;

            std::vector< std::string > ms =
                atf::expand::expand_glob(glob, ids);
            if (ms.empty())
                throw std::runtime_error("Unknown test case `" + glob + "'");
            exps.insert(exps.end(), ms.begin(), ms.end());
        }

        // For each expansion, locate its corresponding test case and add
        // it to the output set.
        for (std::vector< std::string >::const_iterator iter = exps.begin();
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

        if (maxlen < tc->get_md_var("ident").length())
            maxlen = tc->get_md_var("ident").length();
    }

    for (tc_vector::const_iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        const impl::tc* tc = *iter;

        std::cout << atf::ui::format_text_with_tag(tc->get_md_var("descr"),
                                                   tc->get_md_var("ident"),
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

    if (!atf::fs::exists(m_workdir))
        throw std::runtime_error("Cannot find the work directory `" +
                                 m_workdir.str() + "'");

    int errcode = EXIT_SUCCESS;

    atf::signals::signal_holder sighup(SIGHUP);
    atf::signals::signal_holder sigint(SIGINT);
    atf::signals::signal_holder sigterm(SIGTERM);

    atf::formats::atf_tcs_writer w(results_stream(), std::cout, std::cerr,
                                   tcs.size());
    for (tc_vector::iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        impl::tc* tc = *iter;

        w.start_tc(tc->get_md_var("ident"));
        impl::tcr tcr = tc->run(STDOUT_FILENO, STDERR_FILENO, m_workdir);
        w.end_tc(tcr);

        sighup.process();
        sigint.process();
        sigterm.process();

        if (tcr.get_state() == impl::tcr::failed_state)
            errcode = EXIT_FAILURE;
    }

    return errcode;
}

int
tp::main(void)
{
    int errcode;

    handle_srcdir();

    for (int i = 0; i < m_argc; i++)
        m_tcnames.push_back(m_argv[i]);

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
        int run_tp(int, char* const*, void (*)(tp::tc_vector&));
    }
}

int
impl::run_tp(int argc, char* const* argv, void (*add_tcs)(tp::tc_vector&))
{
    return tp(add_tcs).run(argc, argv);
}
