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

#if defined(HAVE_CONFIG_H)
#include "bconfig.h"
#endif

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "atf-c++/application.hpp"
#include "atf-c++/atffile.hpp"
#include "atf-c++/config.hpp"
#include "atf-c++/env.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/formats.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/parser.hpp"
#include "atf-c++/process.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/tests.hpp"
#include "atf-c++/text.hpp"

class config : public atf::formats::atf_config_reader {
    atf::tests::vars_map m_vars;

    void
    got_var(const std::string& var, const std::string& name)
    {
        m_vars[var] = name;
    }

public:
    config(std::istream& is) :
        atf::formats::atf_config_reader(is)
    {
    }

    const atf::tests::vars_map&
    get_vars(void)
        const
    {
        return m_vars;
    }
};

class muxer : public atf::formats::atf_tcs_reader {
    atf::fs::path m_tp;
    atf::formats::atf_tps_writer m_writer;

    bool m_inited, m_finalized;
    size_t m_ntcs;
    std::string m_tcname;

    // Counters for the test cases run by the test program.
    size_t m_passed, m_failed, m_skipped;

    void
    got_ntcs(size_t ntcs)
    {
        m_writer.start_tp(m_tp.str(), ntcs);
        m_inited = true;
        if (ntcs == 0)
            throw atf::formats::format_error("Bogus test program: reported "
                                             "0 test cases");
    }

    void
    got_tc_start(const std::string& tcname)
    {
        m_tcname = tcname;
        m_writer.start_tc(tcname);
    }

    void
    got_tc_end(const atf::tests::tcr& tcr)
    {
        const atf::tests::tcr::state& s = tcr.get_state();
        if (s == atf::tests::tcr::passed_state) {
            m_passed++;
        } else if (s == atf::tests::tcr::skipped_state) {
            m_skipped++;
        } else if (s == atf::tests::tcr::failed_state) {
            m_failed++;
        } else
            UNREACHABLE;

        m_writer.end_tc(tcr);
        m_tcname = "";
    }

    void
    got_stdout_line(const std::string& line)
    {
        m_writer.stdout_tc(line);
    }

    void
    got_stderr_line(const std::string& line)
    {
        m_writer.stderr_tc(line);
    }

public:
    muxer(const atf::fs::path& tp, atf::formats::atf_tps_writer& w,
           atf::io::pistream& is) :
        atf::formats::atf_tcs_reader(is),
        m_tp(tp),
        m_writer(w),
        m_inited(false),
        m_finalized(false),
        m_passed(0),
        m_failed(0),
        m_skipped(0)
    {
    }

    size_t
    failed(void)
        const
    {
        return m_failed;
    }

    void
    finalize(const std::string& reason = "")
    {
        PRE(!m_finalized);

        if (!m_inited)
            m_writer.start_tp(m_tp.str(), 0);
        if (!m_tcname.empty()) {
            INV(!reason.empty());
            got_tc_end(atf::tests::tcr(atf::tests::tcr::failed_state,
                                       "Bogus test program"));
        }

        m_writer.end_tp(reason);
        m_finalized = true;
    }

    ~muxer(void)
    {
        // The following is incorrect because we cannot throw an exception
        // from a destructor.  Let's just hope that this never happens.
        PRE(m_finalized);
    }
};

template< class K, class V >
void
merge_maps(std::map< K, V >& dest, const std::map< K, V >& src)
{
    for (typename std::map< K, V >::const_iterator iter = src.begin();
         iter != src.end(); iter++)
        dest[(*iter).first] = (*iter).second;
}

class atf_run : public atf::application::app {
    static const char* m_description;

    atf::tests::vars_map m_atffile_vars;
    atf::tests::vars_map m_cmdline_vars;
    atf::tests::vars_map m_config_vars;

    static atf::tests::vars_map::value_type parse_var(const std::string&);

    void process_option(int, const char*);
    std::string specific_args(void) const;
    options_set specific_options(void) const;

    void parse_vflag(const std::string&);

    void read_one_config(const atf::fs::path&);
    void read_config(const std::string&);
    std::vector< std::string > conf_args(void) const;

    size_t count_tps(std::vector< std::string >) const;

    int run_test(const atf::fs::path&,
                 atf::formats::atf_tps_writer&);
    int run_test_directory(const atf::fs::path&,
                           atf::formats::atf_tps_writer&);
    int run_test_program(const atf::fs::path&,
                         atf::formats::atf_tps_writer&);

    struct test_data {
        const atf_run* m_this;
        const atf::fs::path& m_tp;
        atf::io::pipe& m_respipe;

        test_data(const atf_run* t, const atf::fs::path& tp,
                  atf::io::pipe& respipe) :
            m_this(t),
            m_tp(tp),
            m_respipe(respipe)
        {
        }
    };

    static void route_run_test_program_child(void *);
    void run_test_program_child(const atf::fs::path&,
                                atf::io::pipe&) const;
    int run_test_program_parent(const atf::fs::path&,
                                atf::formats::atf_tps_writer&,
                                atf::process::child&,
                                atf::io::pipe&);

public:
    atf_run(void);

    int main(void);
};

const char* atf_run::m_description =
    "atf-run is a tool that runs tests programs and collects their "
    "results.";

atf_run::atf_run(void) :
    app(m_description, "atf-run(1)", "atf(7)")
{
}

void
atf_run::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'v':
        parse_vflag(arg);
        break;

    default:
        UNREACHABLE;
    }
}

std::string
atf_run::specific_args(void)
    const
{
    return "[test-program1 .. test-programN]";
}

atf_run::options_set
atf_run::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('v', "var=value", "Sets the configuration variable "
                                         "`var' to `value'; overrides "
                                         "values in configuration files"));
    return opts;
}

void
atf_run::parse_vflag(const std::string& str)
{
    if (str.empty())
        throw std::runtime_error("-v requires a non-empty argument");

    std::vector< std::string > ws = atf::text::split(str, "=");
    if (ws.size() == 1 && str[str.length() - 1] == '=') {
        m_cmdline_vars[ws[0]] = "";
    } else {
        if (ws.size() != 2)
            throw std::runtime_error("-v requires an argument of the form "
                                     "var=value");

        m_cmdline_vars[ws[0]] = ws[1];
    }
}

int
atf_run::run_test(const atf::fs::path& tp,
                  atf::formats::atf_tps_writer& w)
{
    atf::fs::file_info fi(tp);

    int errcode;
    if (fi.get_type() == atf::fs::file_info::dir_type)
        errcode = run_test_directory(tp, w);
    else
        errcode = run_test_program(tp, w);
    return errcode;
}

int
atf_run::run_test_directory(const atf::fs::path& tp,
                            atf::formats::atf_tps_writer& w)
{
    atf::atffile af(tp / "Atffile");
    m_atffile_vars = af.conf();

    atf::tests::vars_map oldvars = m_config_vars;
    {
        atf::tests::vars_map::const_iterator iter =
            af.props().find("test-suite");
        INV(iter != af.props().end());
        read_config((*iter).second);
    }

    bool ok = true;
    for (std::vector< std::string >::const_iterator iter = af.tps().begin();
         iter != af.tps().end(); iter++)
        ok &= (run_test(tp / *iter, w) == EXIT_SUCCESS);

    m_config_vars = oldvars;

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

std::vector< std::string >
atf_run::conf_args(void) const
{
    using atf::tests::vars_map;

    atf::tests::vars_map vars;
    std::vector< std::string > args;

    merge_maps(vars, m_atffile_vars);
    merge_maps(vars, m_config_vars);
    merge_maps(vars, m_cmdline_vars);

    for (vars_map::const_iterator i = vars.begin(); i != vars.end(); i++)
        args.push_back("-v" + (*i).first + "=" + (*i).second);

    return args;
}

void
atf_run::route_run_test_program_child(void* v)
{
    test_data* td = static_cast< test_data* >(v);
    td->m_this->run_test_program_child(td->m_tp, td->m_respipe);
    UNREACHABLE;
}

void
atf_run::run_test_program_child(const atf::fs::path& tp,
                                atf::io::pipe& respipe)
    const
{
    // Remap the results file descriptor to point to the parent too.
    // We use the 9th one (instead of a bigger one) because shell scripts
    // can only use the [0..9] file descriptors in their redirections.
    respipe.rend().close();
    respipe.wend().posix_remap(9);

    // Prepare the test program's arguments.  We use dynamic memory and
    // do not care to release it.  We are going to die anyway very soon,
    // either due to exec(2) or to exit(3).
    std::vector< std::string > confargs = conf_args();
    char** args = new char*[4 + confargs.size()];
    {
        // 0: Program name.
        std::string progname = tp.leaf_name();
        args[0] = new char[progname.length() + 1];
        std::strcpy(args[0], progname.c_str());

        // 1: The file descriptor to which the results will be printed.
        args[1] = new char[4];
        std::strcpy(args[1], "-r9");

        // 2: The directory where the test program lives.
        atf::fs::path bp = tp.branch_path();
        if (!bp.is_absolute())
            bp = bp.to_absolute();
        const char* dir = bp.c_str();
        args[2] = new char[std::strlen(dir) + 3];
        std::strcpy(args[2], "-s");
        std::strcat(args[2], dir);

        // [3..last - 1]: Configuration arguments.
        std::vector< std::string >::size_type i;
        for (i = 0; i < confargs.size(); i++) {
            const char* str = confargs[i].c_str();
            args[3 + i] = new char[std::strlen(str) + 1];
            std::strcpy(args[3 + i], str);
        }

        // Last: Terminator.
        args[3 + i] = NULL;
    }

    // Do the real exec and report any errors to the parent through the
    // only mechanism we can use: stderr.
    // TODO Try to make this fail.
    ::execv(tp.c_str(), args);
    std::cerr << "Failed to execute `" << tp.str() << "': "
              << std::strerror(errno) << std::endl;
    std::exit(EXIT_FAILURE);
}

int
atf_run::run_test_program_parent(const atf::fs::path& tp,
                                 atf::formats::atf_tps_writer& w,
                                 atf::process::child& c,
                                 atf::io::pipe& respipe)
{
    // Get the input stream of stdout and stderr.
    atf::io::file_handle outfh = c.stdout_fd();
    atf::io::unbuffered_istream outin(outfh);
    atf::io::file_handle errfh = c.stderr_fd();
    atf::io::unbuffered_istream errin(errfh);

    // Get the file descriptor and input stream of the results channel.
    respipe.wend().close();
    atf::io::pistream resin(respipe.rend());

    // Process the test case's output and multiplex it into our output
    // stream as we read it.
    muxer m(tp, w, resin);
    std::string fmterr;
    try {
        m.read(outin, errin);
    } catch (const atf::parser::parse_errors& e) {
        fmterr = "There were errors parsing the output of the test "
                 "program:";
        for (atf::parser::parse_errors::const_iterator iter = e.begin();
             iter != e.end(); iter++) {
            fmterr += " Line " + atf::text::to_string((*iter).first) +
                      ": " + (*iter).second + ".";
        }
    } catch (const atf::formats::format_error& e) {
        fmterr = e.what();
    } catch (...) {
        UNREACHABLE;
    }

    try {
        outin.close();
        errin.close();
        resin.close();
    } catch (...) {
        UNREACHABLE;
    }

    const atf::process::status s = c.wait();

    int code;
    if (s.exited()) {
        code = s.exitstatus();
        if (m.failed() > 0 && code == EXIT_SUCCESS) {
            code = EXIT_FAILURE;
            m.finalize("Test program returned success but some test "
                       "cases failed" +
                       (fmterr.empty() ? "" : (".  " + fmterr)));
        } else {
            code = fmterr.empty() ? code : EXIT_FAILURE;
            m.finalize(fmterr);
        }
    } else if (s.signaled()) {
        code = EXIT_FAILURE;
        m.finalize("Test program received signal " +
                   atf::text::to_string(s.termsig()) +
                   (s.coredump() ? " (core dumped)" : "") +
                   (fmterr.empty() ? "" : (".  " + fmterr)));
    } else
        throw std::runtime_error
            ("Child process " + atf::text::to_string(c.pid()) +
             " terminated with an unknown status condition");
    return code;
}

int
atf_run::run_test_program(const atf::fs::path& tp,
                          atf::formats::atf_tps_writer& w)
{
    // XXX: This respipe is quite annoying.  The fact that we cannot
    // represent it as part of a portable fork API (which only supports
    // stdin, stdout and stderr) and not even in our own fork API means
    // that this will be a huge source of portability problems in the
    // future, should we ever want to port ATF to Win32.  I guess it'd
    // be worth revisiting the decision of using a third file descriptor
    // for results reporting sooner than later.  Alternative: use a
    // temporary file.
    atf::io::pipe respipe;
    test_data td(this, tp, respipe);
    atf::process::child c =
        atf::process::fork(route_run_test_program_child,
                           atf::process::stream_capture(),
                           atf::process::stream_capture(),
                           static_cast< void * >(&td));

    return run_test_program_parent(tp, w, c, respipe);
}

size_t
atf_run::count_tps(std::vector< std::string > tps)
    const
{
    size_t ntps = 0;

    for (std::vector< std::string >::const_iterator iter = tps.begin();
         iter != tps.end(); iter++) {
        atf::fs::path tp(*iter);
        atf::fs::file_info fi(tp);

        if (fi.get_type() == atf::fs::file_info::dir_type) {
            atf::atffile af = atf::atffile(tp / "Atffile");
            std::vector< std::string > aux = af.tps();
            for (std::vector< std::string >::iterator i2 = aux.begin();
                 i2 != aux.end(); i2++)
                *i2 = (tp / *i2).str();
            ntps += count_tps(aux);
        } else
            ntps++;
    }

    return ntps;
}

void
atf_run::read_one_config(const atf::fs::path& p)
{
    std::ifstream is(p.c_str());
    if (is) {
        config reader(is);
        reader.read();
        merge_maps(m_config_vars, reader.get_vars());
    }
}

void
atf_run::read_config(const std::string& name)
{
    std::vector< atf::fs::path > dirs;
    dirs.push_back(atf::fs::path(atf::config::get("atf_confdir")));
    if (atf::env::has("HOME"))
        dirs.push_back(atf::fs::path(atf::env::get("HOME")) / ".atf");

    m_config_vars.clear();
    for (std::vector< atf::fs::path >::const_iterator iter = dirs.begin();
         iter != dirs.end(); iter++) {
        read_one_config((*iter) / "common.conf");
        read_one_config((*iter) / (name + ".conf"));
    }
}

static
void
call_hook(const std::string& tool, const std::string& hook)
{
    const atf::fs::path sh(atf::config::get("atf_shell"));
    const atf::fs::path hooks =
        atf::fs::path(atf::config::get("atf_pkgdatadir")) / (tool + ".hooks");

    const atf::process::status s =
        atf::process::exec(sh,
                           atf::process::argv_array(sh.c_str(), hooks.c_str(),
                                                    hook.c_str(), NULL),
                           atf::process::stream_inherit(),
                           atf::process::stream_inherit());


    if (!s.exited() || s.exitstatus() != EXIT_SUCCESS)
        throw std::runtime_error("Failed to run the '" + hook + "' hook "
                                 "for '" + tool + "'");
}

int
atf_run::main(void)
{
    atf::atffile af(atf::fs::path("Atffile"));
    m_atffile_vars = af.conf();

    std::vector< std::string > tps;
    tps = af.tps();
    if (m_argc >= 1) {
        // TODO: Ensure that the given test names are listed in the
        // Atffile.  Take into account that the file can be using globs.
        tps.clear();
        for (int i = 0; i < m_argc; i++)
            tps.push_back(m_argv[i]);
    }

    // Read configuration data for this test suite.
    {
        atf::tests::vars_map::const_iterator iter =
            af.props().find("test-suite");
        INV(iter != af.props().end());
        read_config((*iter).second);
    }

    atf::formats::atf_tps_writer w(std::cout);
    call_hook("atf-run", "info_start_hook");
    w.ntps(count_tps(tps));

    bool ok = true;
    for (std::vector< std::string >::const_iterator iter = tps.begin();
         iter != tps.end(); iter++)
        ok &= (run_test(atf::fs::path(*iter), w) == EXIT_SUCCESS);

    call_hook("atf-run", "info_end_hook");

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

int
main(int argc, char* const* argv)
{
    return atf_run().run(argc, argv);
}
