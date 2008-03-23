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

#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "atf/application.hpp"
#include "atf/fs.hpp"
#include "atf/formats.hpp"
#include "atf/sanity.hpp"
#include "atf/text.hpp"
#include "atf/ui.hpp"

typedef std::auto_ptr< std::ostream > ostream_ptr;

ostream_ptr
open_outfile(const atf::fs::path& path)
{
    ostream_ptr osp;
    if (path.str() == "-")
        osp = ostream_ptr(new std::ofstream("/dev/stdout"));
    else
        osp = ostream_ptr(new std::ofstream(path.c_str()));
    if (!(*osp))
        throw std::runtime_error("Could not create file " + path.str());
    return osp;
}

// ------------------------------------------------------------------------
// The "writer" interface.
// ------------------------------------------------------------------------

//!
//! \brief A base class that defines an output format.
//!
//! The writer base class defines a generic interface to output formats.
//! This is meant to be subclassed, and each subclass can redefine any
//! method to format the information as it wishes.
//!
//! This class is not tied to a output stream nor a file because, depending
//! on the output format, we will want to write to a single file or to
//! multiple ones.
//!
class writer {
public:
    writer(void) {}
    virtual ~writer(void) {}

    virtual void write_info(const std::string&, const std::string&) {}
    virtual void write_ntps(size_t) {}
    virtual void write_tp_start(const std::string&, size_t) {}
    virtual void write_tp_end(const std::string&) {}
    virtual void write_tc_start(const std::string&) {}
    virtual void write_tc_stdout_line(const std::string&) {}
    virtual void write_tc_stderr_line(const std::string&) {}
    virtual void write_tc_end(const atf::tests::tcr&) {}
    virtual void write_eof(void) {}
};

// ------------------------------------------------------------------------
// The "csv_writer" class.
// ------------------------------------------------------------------------

//!
//! \brief A very simple plain-text output format.
//!
//! The csv_writer class implements a very simple plain-text output
//! format that summarizes the results of each executed test case.  The
//! results are meant to be easily parseable by third-party tools, hence
//! they are formatted as a CSV file.
//!
class csv_writer : public writer {
    ostream_ptr m_os;
    bool m_failed;

    std::string m_tpname;
    std::string m_tcname;

public:
    csv_writer(const atf::fs::path& p) :
        m_os(open_outfile(p))
    {
    }

    virtual
    void
    write_tp_start(const std::string& name, size_t ntcs)
    {
        m_tpname = name;
        m_failed = false;
    }

    virtual
    void
    write_tp_end(const std::string& reason)
    {
        if (!reason.empty())
            (*m_os) << "tp, " << m_tpname << ", bogus, " << reason
                    << std::endl;
        else if (m_failed)
            (*m_os) << "tp, " << m_tpname << ", failed" << std::endl;
        else
            (*m_os) << "tp, " << m_tpname << ", passed" << std::endl;
    }

    virtual
    void
    write_tc_start(const std::string& name)
    {
        m_tcname = name;
    }

    virtual
    void
    write_tc_end(const atf::tests::tcr& tcr)
    {
        std::string str = "tc, ";
        if (tcr.get_status() == atf::tests::tcr::status_passed) {
            str += m_tpname + ", " + m_tcname + ", passed";
        } else if (tcr.get_status() == atf::tests::tcr::status_failed) {
            str += m_tpname + ", " + m_tcname + ", failed, " +
                   tcr.get_reason();
            m_failed = true;
        } else if (tcr.get_status() == atf::tests::tcr::status_skipped) {
            str += m_tpname + ", " + m_tcname + ", skipped, " +
                   tcr.get_reason();
        } else
            UNREACHABLE;
        (*m_os) << str << std::endl;
    }
};

// ------------------------------------------------------------------------
// The "ticker_writer" class.
// ------------------------------------------------------------------------

//!
//! \brief A console-friendly output format.
//!
//! The ticker_writer class implements a formatter that is user-friendly
//! in the sense that it shows the execution of test cases in an easy to
//! read format.  It is not meant to be parseable and its format can
//! freely change across releases.
//!
class ticker_writer : public writer {
    ostream_ptr m_os;

    size_t m_curtp, m_ntps;
    size_t m_tcs_passed, m_tcs_failed, m_tcs_skipped;
    std::string m_tcname, m_tpname;
    std::vector< std::string > m_failed_tcs;
    std::vector< std::string > m_failed_tps;

    void
    write_info(const std::string& what, const std::string& val)
    {
        if (what == "tests.root") {
            (*m_os) << "Tests root: " << val << std::endl
                    << std::endl;
        }
    }

    void
    write_ntps(size_t ntps)
    {
        m_curtp = 1;
        m_tcs_passed = 0;
        m_tcs_failed = 0;
        m_tcs_skipped = 0;
        m_ntps = ntps;
    }

    void
    write_tp_start(const std::string& tp, size_t ntcs)
    {
        using atf::text::to_string;
        using atf::ui::format_text;

        m_tpname = tp;

        (*m_os) << format_text(tp + " (" + to_string(m_curtp) +
                               "/" + to_string(m_ntps) + "): " +
                               to_string(ntcs) + " test cases")
                << std::endl;
        (*m_os).flush();
    }

    void
    write_tp_end(const std::string& reason)
    {
        using atf::ui::format_text_with_tag;

        m_curtp++;

        if (!reason.empty()) {
            (*m_os) << format_text_with_tag("BOGUS TEST PROGRAM: Cannot "
                                            "trust its results because "
                                            "of `" + reason + "'",
                                            m_tpname + ": ", false)
                    << std::endl;
            m_failed_tps.push_back(m_tpname);
        }
        (*m_os) << std::endl;
        (*m_os).flush();

        m_tpname.clear();
    }

    void
    write_tc_start(const std::string& tcname)
    {
        m_tcname = tcname;

        (*m_os) << "    " + tcname + ": ";
        (*m_os).flush();
    }

    void
    write_tc_end(const atf::tests::tcr& tcr)
    {
        std::string str;

        atf::tests::tcr::status s = tcr.get_status();
        if (s == atf::tests::tcr::status_passed) {
            str = "Passed.";
            m_tcs_passed++;
        } else if (s == atf::tests::tcr::status_failed) {
            str = "Failed: " + tcr.get_reason();
            m_tcs_failed++;
            m_failed_tcs.push_back(m_tpname + ":" + m_tcname);
        } else if (s == atf::tests::tcr::status_skipped) {
            str = "Skipped: " + tcr.get_reason();
            m_tcs_skipped++;
        } else
            UNREACHABLE;

        // XXX Wrap text.  format_text_with_tag does not currently allow
        // to specify the current column, which is needed because we have
        // already printed the tc's name.
        (*m_os) << str << std::endl;

        m_tcname = "";
    }

    void
    write_eof(void)
    {
        using atf::text::join;
        using atf::text::to_string;
        using atf::ui::format_text;
        using atf::ui::format_text_with_tag;

        if (!m_failed_tps.empty()) {
            (*m_os) << format_text("Failed (bogus) test programs:")
                    << std::endl;
            (*m_os) << format_text_with_tag(join(m_failed_tps, ", "),
                                            "    ", false) << std::endl
                    << std::endl;
        }

        if (!m_failed_tcs.empty()) {
            (*m_os) << format_text("Failed test cases:") << std::endl;
            (*m_os) << format_text_with_tag(join(m_failed_tcs, ", "),
                                            "    ", false) << std::endl
                    << std::endl;
        }

        (*m_os) << format_text("Summary for " + to_string(m_ntps) +
                               " test programs:")
                << std::endl;
        (*m_os) << format_text_with_tag(to_string(m_tcs_passed) +
                                        " passed test cases.",
                                        "    ", false)
                << std::endl;
        (*m_os) << format_text_with_tag(to_string(m_tcs_failed) +
                                        " failed test cases.",
                                        "    ", false)
                << std::endl;
        (*m_os) << format_text_with_tag(to_string(m_tcs_skipped) +
                                        " skipped test cases.",
                                        "    ", false)
                << std::endl;
    }

public:
    ticker_writer(const atf::fs::path& p) :
        m_os(open_outfile(p))
    {
    }
};

// ------------------------------------------------------------------------
// The "xml" class.
// ------------------------------------------------------------------------

//!
//! \brief A single-file XML output format.
//!
//! The xml_writer class implements a formatter that prints the results
//! of test cases in an XML format easily parseable later on by other
//! utilities.
//!
class xml_writer : public writer {
    ostream_ptr m_os;

    size_t m_curtp, m_ntps;
    size_t m_tcs_passed, m_tcs_failed, m_tcs_skipped;
    std::string m_tcname, m_tpname;
    std::vector< std::string > m_failed_tcs;
    std::vector< std::string > m_failed_tps;

    static
    std::string
    attrval(const std::string& str)
    {
        return str;
    }

    static
    std::string
    elemval(const std::string& str)
    {
        std::string ostr;
        for (std::string::const_iterator iter = str.begin();
             iter != str.end(); iter++) {
            switch (*iter) {
            case '&': ostr += "&amp;"; break;
            case '<': ostr += "&lt;"; break;
            case '>': ostr += "&gt;"; break;
            default:  ostr += *iter;
            }
        }
        return ostr;
    }

    void
    write_info(const std::string& what, const std::string& val)
    {
        (*m_os) << "<info class=\"" << what << "\">" << val << "</info>"
                << std::endl;
    }

    void
    write_tp_start(const std::string& tp, size_t ntcs)
    {
        (*m_os) << "<tp id=\"" << attrval(tp) << "\">" << std::endl;
    }

    void
    write_tp_end(const std::string& reason)
    {
        if (!reason.empty())
            (*m_os) << "<failed>" << elemval(reason) << "</failed>"
                    << std::endl;
        (*m_os) << "</tp>" << std::endl;
    }

    void
    write_tc_start(const std::string& tcname)
    {
        (*m_os) << "<tc id=\"" << attrval(tcname) << "\">" << std::endl;
    }

    void
    write_tc_stdout_line(const std::string& line)
    {
        (*m_os) << "<so>" << elemval(line) << "</so>" << std::endl;
    }

    void
    write_tc_stderr_line(const std::string& line)
    {
        (*m_os) << "<se>" << elemval(line) << "</se>" << std::endl;
    }

    void
    write_tc_end(const atf::tests::tcr& tcr)
    {
        std::string str;

        atf::tests::tcr::status s = tcr.get_status();
        if (s == atf::tests::tcr::status_passed) {
            (*m_os) << "<passed />" << std::endl;
        } else if (s == atf::tests::tcr::status_failed) {
            (*m_os) << "<failed>" << elemval(tcr.get_reason())
                    << "</failed>" << std::endl;
        } else if (s == atf::tests::tcr::status_skipped) {
            (*m_os) << "<skipped>" << elemval(tcr.get_reason())
                    << "</skipped>" << std::endl;
        } else
            UNREACHABLE;
        (*m_os) << "</tc>" << std::endl;
    }

    void
    write_eof(void)
    {
        (*m_os) << "</tests-results>" << std::endl;
    }

public:
    xml_writer(const atf::fs::path& p) :
        m_os(open_outfile(p))
    {
        (*m_os) << "<?xml version=\"1.0\"?>" << std::endl
                << "<!DOCTYPE tests-results PUBLIC "
                   "\"-//NetBSD//DTD ATF Tests Results 0.1//EN\" "
                   "\"http://www.NetBSD.org/XML/atf/tests-results.dtd\">"
                << std::endl
                << std::endl
                << "<tests-results>" << std::endl;
    }
};

// ------------------------------------------------------------------------
// The "converter" class.
// ------------------------------------------------------------------------

//!
//! \brief A reader that redirects events to multiple writers.
//!
//! The converter class implements an atf_tps_reader that, for each event
//! raised by the parser, redirects it to multiple writers so that they
//! can reformat it according to their output rules.
//!
class converter : public atf::formats::atf_tps_reader {
    typedef std::vector< writer* > outs_vector;
    outs_vector m_outs;

    void
    got_info(const std::string& what, const std::string& val)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_info(what, val);
    }

    void
    got_ntps(size_t ntps)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_ntps(ntps);
    }

    void
    got_tp_start(const std::string& tp, size_t ntcs)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_tp_start(tp, ntcs);
    }

    void
    got_tp_end(const std::string& reason)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_tp_end(reason);
    }

    void
    got_tc_start(const std::string& tcname)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_tc_start(tcname);
    }

    void
    got_tc_stdout_line(const std::string& line)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_tc_stdout_line(line);
    }

    void
    got_tc_stderr_line(const std::string& line)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_tc_stderr_line(line);
    }

    void
    got_tc_end(const atf::tests::tcr& tcr)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_tc_end(tcr);
    }

    void
    got_eof(void)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            (*iter)->write_eof();
    }

public:
    converter(std::istream& is) :
        atf::formats::atf_tps_reader(is)
    {
    }

    ~converter(void)
    {
        for (outs_vector::iterator iter = m_outs.begin();
             iter != m_outs.end(); iter++)
            delete *iter;
    }

    void
    add_output(const std::string& fmt, const atf::fs::path& p)
    {
        if (fmt == "csv") {
            m_outs.push_back(new csv_writer(p));
        } else if (fmt == "ticker") {
            m_outs.push_back(new ticker_writer(p));
        } else if (fmt == "xml") {
            m_outs.push_back(new xml_writer(p));
        } else
            throw std::runtime_error("Unknown format `" + fmt + "'");
    }
};

// ------------------------------------------------------------------------
// The "atf_report" class.
// ------------------------------------------------------------------------

class atf_report : public atf::application::app {
    static const char* m_description;

    typedef std::pair< std::string, atf::fs::path > fmt_path_pair;
    std::vector< fmt_path_pair > m_oflags;

    void process_option(int, const char*);
    options_set specific_options(void) const;

public:
    atf_report(void);

    int main(void);
};

const char* atf_report::m_description =
    "atf-report is a tool that parses the output of atf-run and "
    "generates user-friendly reports in multiple different formats.";

atf_report::atf_report(void) :
    app(m_description, "atf-report(1)", "atf(7)")
{
}

void
atf_report::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 'o':
        {
            std::string str(arg);
            std::string::size_type pos = str.find(':');
            if (pos == std::string::npos)
                throw std::runtime_error("Syntax error in -o option");
            else {
                std::string fmt = str.substr(0, pos);
                atf::fs::path path = atf::fs::path(str.substr(pos + 1));
                m_oflags.push_back(fmt_path_pair(fmt, path));
            }
        }
        break;

    default:
        UNREACHABLE;
    }
}

atf_report::options_set
atf_report::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('o', "fmt:path", "Adds a new output file; multiple "
                                        "ones can be specified, and a - "
                                        "path means stdout"));
    return opts;
}

int
atf_report::main(void)
{
    if (m_oflags.empty())
        m_oflags.push_back(fmt_path_pair("ticker", atf::fs::path("-")));

    // Look for path duplicates.
    std::set< atf::fs::path > paths;
    for (std::vector< fmt_path_pair >::const_iterator iter = m_oflags.begin();
         iter != m_oflags.end(); iter++) {
        atf::fs::path p = (*iter).second;
        if (p == atf::fs::path("/dev/stdout"))
            p = atf::fs::path("-");
        if (paths.find(p) != paths.end())
            throw std::runtime_error("The file `" + p.str() + "' was "
                                     "specified more than once");
        paths.insert((*iter).second);
    }

    // Generate the output files.
    converter cnv(std::cin);
    for (std::vector< fmt_path_pair >::const_iterator iter = m_oflags.begin();
         iter != m_oflags.end(); iter++)
        cnv.add_output((*iter).first, (*iter).second);
    cnv.read();

    return EXIT_SUCCESS;
}

int
main(int argc, char* const* argv)
{
    return atf_report().run(argc, argv);
}
