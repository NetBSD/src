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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "atf/application.hpp"
#include "atf/atffile.hpp"
#include "atf/config.hpp"
#include "atf/env.hpp"
#include "atf/exceptions.hpp"
#include "atf/formats.hpp"
#include "atf/fs.hpp"
#include "atf/io.hpp"
#include "atf/parser.hpp"
#include "atf/sanity.hpp"
#include "atf/tests.hpp"
#include "atf/text.hpp"

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
        const atf::tests::tcr::status& s = tcr.get_status();
        if (s == atf::tests::tcr::status_passed) {
            m_passed++;
        } else if (s == atf::tests::tcr::status_skipped) {
            m_skipped++;
        } else if (s == atf::tests::tcr::status_failed) {
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
            got_tc_end(atf::tests::tcr::failed("Bogus test program"));
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

class atf_run : public atf::application {
    static const char* m_description;

    atf::tests::vars_map m_atffile_vars;
    atf::tests::vars_map m_cmdline_vars;
    atf::tests::vars_map m_config_vars;

    static atf::tests::vars_map::value_type parse_var(const std::string&);

    void process_option(int, const char*);
    std::string specific_args(void) const;
    options_set specific_options(void) const;

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

    void run_test_program_child(const atf::fs::path&,
                                atf::io::pipe&,
                                atf::io::pipe&,
                                atf::io::pipe&);
    int run_test_program_parent(const atf::fs::path&,
                                atf::formats::atf_tps_writer&,
                                atf::io::pipe&,
                                atf::io::pipe&,
                                atf::io::pipe&,
                                pid_t);

public:
    atf_run(void);

    int main(void);
};

const char* atf_run::m_description =
    "atf-run is a tool that runs tests programs and collects their "
    "results.";

atf_run::atf_run(void) :
    application(m_description, "atf-run(1)")
{
}

void
atf_run::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'v':
        {
            atf::tests::vars_map::value_type v =
                atf::tests::vars_map::parse(arg);
            m_cmdline_vars[v.first] = v.second;
        }
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
    options_set opts;
    opts.insert(option('v', "var=value", "Sets the configuration variable "
                                         "`var' to `value'; overrides "
                                         "values in configuration files"));
    return opts;
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
atf_run::run_test_program_child(const atf::fs::path& tp,
                                atf::io::pipe& outpipe,
                                atf::io::pipe& errpipe,
                                atf::io::pipe& respipe)
{
    // Remap stdout and stderr to point to the parent, who will capture
    // everything sent to these.
    outpipe.rend().close();
    outpipe.wend().posix_remap(STDOUT_FILENO);
    errpipe.rend().close();
    errpipe.wend().posix_remap(STDERR_FILENO);

    // Remap the results file descriptor to point to the parent too.
    // We use the 9th one (instead of a bigger one) because shell scripts
    // can only use the [0..9] file descriptors in their redirections.
    respipe.rend().close();
    respipe.wend().posix_remap(9);

    // Prepare the test program's arguments.  We use dynamic memory and
    // do not care to release it.  We are going to die anyway very soon,
    // either due to exec(2) or to exit(3).
    std::vector< std::string > confargs = conf_args();
    char* args[4 + confargs.size()];
    {
        // 0: Program name.
        const char* name = tp.leaf_name().c_str();
        args[0] = new char[std::strlen(name) + 1];
        std::strcpy(args[0], name);

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
                                 atf::io::pipe& outpipe,
                                 atf::io::pipe& errpipe,
                                 atf::io::pipe& respipe,
                                 pid_t pid)
{
    // Get the file descriptor and input stream of stdout.
    outpipe.wend().close();
    atf::io::unbuffered_istream outin(outpipe.rend());

    // Get the file descriptor and input stream of stderr.
    errpipe.wend().close();
    atf::io::unbuffered_istream errin(errpipe.rend());

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

    int code, status;
    if (::waitpid(pid, &status, 0) == -1) {
        m.finalize("waitpid(2) on the child process " +
                   atf::text::to_string(pid) + " failed" +
                   (fmterr.empty() ? "" : (".  " + fmterr)));
        code = EXIT_FAILURE;
    } else {
        if (WIFEXITED(status)) {
            code = WEXITSTATUS(status);
            if (m.failed() > 0 && code == EXIT_SUCCESS) {
                code = EXIT_FAILURE;
                m.finalize("Test program returned success but some test "
                           "cases failed" +
                           (fmterr.empty() ? "" : (".  " + fmterr)));
            } else {
                code = fmterr.empty() ? code : EXIT_FAILURE;
                m.finalize(fmterr);
            }
        } else if (WIFSIGNALED(status)) {
            code = EXIT_FAILURE;
            m.finalize("Test program received signal " +
                       atf::text::to_string(WTERMSIG(status)) +
                       (WCOREDUMP(status) ? " (core dumped)" : "") +
                       (fmterr.empty() ? "" : (".  " + fmterr)));
        } else if (WIFSTOPPED(status)) {
            code = EXIT_FAILURE;
            m.finalize("Test program stopped due to signal " +
                       atf::text::to_string(WSTOPSIG(status)) +
                       (fmterr.empty() ? "" : (".  " + fmterr)));
        } else
            throw std::runtime_error
                ("Child process " + atf::text::to_string(pid) +
                 " terminated with an unknown status condition " +
                 atf::text::to_string(status));
    }
    return code;
}

int
atf_run::run_test_program(const atf::fs::path& tp,
                          atf::formats::atf_tps_writer& w)
{
    int errcode;

    atf::io::pipe outpipe, errpipe, respipe;
    pid_t pid = ::fork();
    if (pid == -1) {
        throw atf::system_error("run_test_program",
                                "fork(2) failed", errno);
    } else if (pid == 0) {
        run_test_program_child(tp, outpipe, errpipe, respipe);
        UNREACHABLE;
    } else {
        errcode = run_test_program_parent(tp, w, outpipe, errpipe,
                                          respipe, pid);
    }

    return errcode;
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
    std::string sh = atf::config::get("atf_shell");
    atf::fs::path p = atf::fs::path(atf::config::get("atf_pkgdatadir")) /
                      (tool + ".hooks");
    std::string cmd = sh + " '" + p.str() + "' '" + hook + "'";
    std::system(cmd.c_str());
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
