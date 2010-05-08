//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009, 2010 The NetBSD Foundation, Inc.
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
#include "atf-c++/env.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/formats.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/tests.hpp"
#include "atf-c++/text.hpp"
#include "atf-c++/ui.hpp"
#include "atf-c++/user.hpp"

namespace impl = atf::tests;
#define IMPL_NAME "atf::tests"

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

namespace {

class tcr_reader : public atf::formats::atf_tcr_reader {
    std::string m_result, m_reason;

    void
    got_result(const std::string& str)
    {
        m_result = str;
    }

    void
    got_reason(const std::string& str)
    {
        m_reason = str;
    }

public:
    tcr_reader(std::istream& is) :
        atf_tcr_reader(is)
    {
    }

    atf::tests::tcr
    get_tcr(void)
        const
    {
        using atf::tests::tcr;

        if (m_result == "passed")
            return tcr(tcr::passed_state);
        else if (m_result == "failed")
            return tcr(tcr::failed_state, m_reason);
        else if (m_result == "skipped")
            return tcr(tcr::skipped_state, m_reason);
        else {
            UNREACHABLE;
            return tcr(tcr::failed_state, "UNKNOWN");
        }
    }
};

} // anonymous namespace

impl::tcr
impl::tcr::read(const fs::path& p)
{
    std::ifstream is(p.c_str());
    if (!is)
        throw std::runtime_error("Cannot open results file `" + p.str() + "'");

    tcr_reader r(is);
    r.read();
    return r.get_tcr();
}

void
impl::tcr::write(const fs::path& p)
    const
{
    std::ofstream os(p.c_str());
    if (!os)
        throw std::runtime_error("Cannot create results file `" + p.str() +
                                 "'");

    atf::formats::atf_tcr_writer w(os);
    if (get_state() == impl::tcr::passed_state) {
        w.result("passed");
    } else if (get_state() == impl::tcr::failed_state) {
        w.result("failed");
        w.reason(get_reason());
    } else if (get_state() == impl::tcr::skipped_state) {
        w.result("skipped");
        w.reason(get_reason());
    }
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

const impl::vars_map
impl::tc::get_md_vars(void)
    const
{
    vars_map vars;

    atf_map_citer_t iter;
    atf_map_for_each_c(iter, atf_tc_get_md_vars(&m_tc)) {
        vars.insert(vars_map::value_type(atf_map_citer_key(iter),
                    static_cast< const char * >(atf_map_citer_data(iter))));
    }

    return vars;
}

void
impl::tc::set_md_var(const std::string& var, const std::string& val)
{
    atf_error_t err = atf_tc_set_md_var(&m_tc, var.c_str(), val.c_str());
    if (atf_is_error(err))
        throw_atf_error(err);
}

void
impl::tc::run(const fs::path& resfile)
    const
{
    atf_error_t err = atf_tc_run(&m_tc, resfile.c_path());
    if (atf_is_error(err))
        throw_atf_error(err);
}

void
impl::tc::run_cleanup(void)
    const
{
    atf_error_t err = atf_tc_cleanup(&m_tc);
    if (atf_is_error(err))
        throw_atf_error(err);
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
    atf_reset_exit_checks();
    atf_tc_pass();
}

void
impl::tc::fail(const std::string& reason)
{
    atf_reset_exit_checks();
    atf_tc_fail("%s", reason.c_str());
}

void
impl::tc::skip(const std::string& reason)
{
    atf_reset_exit_checks();
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
    atf::fs::path m_resfile;
    atf::fs::path m_srcdir;

    atf::tests::vars_map m_vars;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

    void (*m_add_tcs)(tc_vector&);
    tc_vector m_tcs;

    void parse_vflag(const std::string&);
    void handle_srcdir(void);

    tc_vector init_tcs(void);

    enum tc_part {
        BODY,
        CLEANUP,
    };

    void list_tcs(void);
    impl::tc* find_tc(tc_vector, const std::string&);
    static std::pair< std::string, tc_part > process_tcarg(const std::string&);
    int run_tc(const std::string&);

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
    m_resfile("resfile"), // XXX
    m_srcdir("."),
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
    return "test_case";
}

tp::options_set
tp::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('l', "", "List test cases and their purpose"));
    opts.insert(option('r', "resfile", "The file to which the test program "
                                       "will write the results of the "
                                       "executed test case"));
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
        m_resfile = atf::fs::path(arg);
        break;

    case 's':
        m_srcdir = atf::fs::path(arg);
        break;

    case 'v':
        parse_vflag(arg);
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

void
tp::list_tcs(void)
{
    tc_vector tcs = init_tcs();
    atf::formats::atf_tp_writer writer(std::cout);

    for (tc_vector::const_iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        const impl::vars_map vars = (*iter)->get_md_vars();

        {
            impl::vars_map::const_iterator iter2 = vars.find("ident");
            INV(iter2 != vars.end());
            writer.start_tc((*iter2).second);
        }

        for (impl::vars_map::const_iterator iter2 = vars.begin();
             iter2 != vars.end(); iter2++) {
            const std::string& key = (*iter2).first;
            if (key != "ident")
                writer.tc_meta_data(key, (*iter2).second);
        }

        writer.end_tc();
    }
}

impl::tc*
tp::find_tc(tc_vector tcs, const std::string& name)
{
    std::vector< std::string > ids;
    for (tc_vector::iterator iter = tcs.begin();
         iter != tcs.end(); iter++) {
        impl::tc* tc = *iter;

        if (tc->get_md_var("ident") == name)
            return tc;
    }
    throw atf::application::usage_error("Unknown test case `%s'",
                                        name.c_str());
}

std::pair< std::string, tp::tc_part >
tp::process_tcarg(const std::string& tcarg)
{
    const std::string::size_type pos = tcarg.find(':');
    if (pos == std::string::npos) {
        return std::make_pair(tcarg, BODY);
    } else {
        const std::string tcname = tcarg.substr(0, pos);

        const std::string partname = tcarg.substr(pos + 1);
        if (partname == "body")
            return std::make_pair(tcname, BODY);
        else if (partname == "cleanup")
            return std::make_pair(tcname, CLEANUP);
        else {
            using atf::application::usage_error;
            throw usage_error("Invalid test case part `%s'", partname.c_str());
        }
    }
}

int
tp::run_tc(const std::string& tcarg)
{
    const std::pair< std::string, tc_part > fields = process_tcarg(tcarg);

    impl::tc* tc = find_tc(init_tcs(), fields.first);

    try {
        switch (fields.second) {
        case BODY:
            tc->run(m_resfile);
            break;
        case CLEANUP:
            tc->run_cleanup();
            break;
        default:
            UNREACHABLE;
        }
        return EXIT_SUCCESS;
    } catch (const std::runtime_error& e) {
        // TODO: Handle error.
        return EXIT_FAILURE;
    }
}

int
tp::main(void)
{
    using atf::application::usage_error;

    int errcode;

    handle_srcdir();

    if (m_lflag) {
        if (m_argc > 0)
            throw usage_error("Cannot provide test case names with -l");

        list_tcs();
        errcode = EXIT_SUCCESS;
    } else {
        if (m_argc == 0)
            throw usage_error("Must provide a test case name");
        else if (m_argc > 1)
            throw usage_error("Cannot provide more than one test case name");
        INV(m_argc == 1);

        errcode = run_tc(m_argv[0]);
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
