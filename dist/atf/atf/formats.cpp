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

extern "C" {
#include <poll.h>
}

#include <map>
#include <sstream>
#include <utility>

#include "atf/formats.hpp"
#include "atf/parser.hpp"
#include "atf/sanity.hpp"
#include "atf/text.hpp"

namespace impl = atf::formats;
#define IMPL_NAME "atf::formats"

#define CALLBACK(parser, func) \
    do { \
        if (!(parser).has_errors()) \
            func; \
    } while (false);

// ------------------------------------------------------------------------
// The "format_error" class.
// ------------------------------------------------------------------------

impl::format_error::format_error(const std::string& w) :
    std::runtime_error(w.c_str())
{
}

// ------------------------------------------------------------------------
// The "header_entry" class.
// ------------------------------------------------------------------------

typedef std::map< std::string, std::string > attrs_map;

class header_entry {
    std::string m_name;
    std::string m_value;
    attrs_map m_attrs;

public:
    header_entry(void);
    header_entry(const std::string&, const std::string&,
                 attrs_map = attrs_map());

    const std::string& name(void) const;
    const std::string& value(void) const;
    const attrs_map& attrs(void) const;
    bool has_attr(const std::string&) const;
    const std::string& get_attr(const std::string&) const;
};

typedef std::map< std::string, header_entry > headers_map;

header_entry::header_entry(void)
{
}

header_entry::header_entry(const std::string& n,
                                 const std::string& v,
                                 attrs_map as) :
    m_name(n),
    m_value(v),
    m_attrs(as)
{
}

const std::string&
header_entry::name(void)
    const
{
    return m_name;
}

const std::string&
header_entry::value(void)
    const
{
    return m_value;
}

const attrs_map&
header_entry::attrs(void)
    const
{
    return m_attrs;
}

bool
header_entry::has_attr(const std::string& n)
    const
{
    return m_attrs.find(n) != m_attrs.end();
}

const std::string&
header_entry::get_attr(const std::string& n)
    const
{
    attrs_map::const_iterator iter = m_attrs.find(n);
    PRE(iter != m_attrs.end());
    return (*iter).second;
}

// ------------------------------------------------------------------------
// The header tokenizer.
// ------------------------------------------------------------------------

namespace header {

static const atf::parser::token_type& eof_type = 0;
static const atf::parser::token_type& nl_type = 1;
static const atf::parser::token_type& text_type = 2;
static const atf::parser::token_type& colon_type = 3;
static const atf::parser::token_type& semicolon_type = 4;
static const atf::parser::token_type& dblquote_type = 5;
static const atf::parser::token_type& equal_type = 6;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(';', semicolon_type);
        add_delim(':', colon_type);
        add_delim('=', equal_type);
        add_quote('"', dblquote_type);
    }
};

static
atf::parser::parser< header::tokenizer >&
read(atf::parser::parser< header::tokenizer >& p, header_entry& he)
{
    using namespace header;

    atf::parser::token t = p.expect(text_type, nl_type, "a header name");
    if (t.type() == nl_type) {
        he = header_entry();
        return p;
    }
    std::string hdr_name = t.text();

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "a textual value");
    std::string hdr_value = t.text();

    attrs_map attrs;

    for (;;) {
        t = p.expect(eof_type, semicolon_type, nl_type,
                     "eof, `;' or new line");
        if (t.type() == eof_type || t.type() == nl_type)
            break;

        t = p.expect(text_type, "an attribute name");
        std::string attr_name = t.text();

        t = p.expect(equal_type, "`='");

        t = p.expect(text_type, "word or quoted string");
        std::string attr_value = t.text();
        attrs[attr_name] = attr_value;
    }

    he = header_entry(hdr_name, hdr_value, attrs);

    return p;
}

static
std::ostream&
write(std::ostream& os, const header_entry& he)
{
    std::string line = he.name() + ": " + he.value();
    attrs_map as = he.attrs();
    for (attrs_map::const_iterator iter = as.begin();
         iter != as.end(); iter++) {
        PRE((*iter).second.find('\"') == std::string::npos);
        line += "; " + (*iter).first + "=\"" + (*iter).second + "\"";
    }

    os << line << std::endl;

    return os;
}

} // namespace header

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
size_t
string_to_size_t(const std::string& str)
{
    std::istringstream ss(str);
    size_t s;
    ss >> s;

    return s;
}

static
std::pair< size_t, headers_map >
read_headers(std::istream& is, size_t curline)
{
    using impl::format_error;

    headers_map hm;

    //
    // Grammar
    //
    // header = entry+ nl
    // entry = line nl
    // line = text colon text
    //        (semicolon (text equal (text | dblquote string dblquote)))*
    // string = quoted_string
    //

    header::tokenizer tkz(is, curline);
    atf::parser::parser< header::tokenizer > p(tkz);

    bool first = true;
    for (;;) {
        try {
            header_entry he;
            if (!header::read(p, he).good() || he.name().empty())
                break;

            if (first && he.name() != "Content-Type")
                throw format_error("Could not determine content type");
            else
                first = false;

            hm[he.name()] = he;
        } catch (const atf::parser::parse_error& pe) {
            p.add_error(pe);
            p.reset(header::nl_type);
        }
    }

    if (!is.good())
        throw format_error("Unexpected end of stream");

    return std::pair< size_t, headers_map >(tkz.lineno(), hm);
}

static
void
write_headers(const headers_map& hm,
              std::ostream& os)
{
    PRE(!hm.empty());
    headers_map::const_iterator ct = hm.find("Content-Type");
    PRE(ct != hm.end());
    header::write(os, (*ct).second);
    for (headers_map::const_iterator iter = hm.begin(); iter != hm.end();
         iter++) {
        if ((*iter).first != "Content-Type")
            header::write(os, (*iter).second);
    }
    os << std::endl;
}

static
void
validate_content_type(const headers_map& hm,
                      const std::string& fmt,
                      int version)
{
    using impl::format_error;

    headers_map::const_iterator iter = hm.find("Content-Type");
    if (iter == hm.end())
        throw format_error("Could not determine content type");

    const header_entry& he = (*iter).second;
    if (he.value() != fmt)
        throw format_error("Mismatched content type: expected `" + fmt +
                           "' but got `" + he.value() + "'");

    if (!he.has_attr("version"))
        throw format_error("Could not determine version");
    const std::string& vstr = atf::text::to_string(version);
    if (he.get_attr("version") != vstr)
        throw format_error("Mismatched version: expected `" +
                           vstr + "' but got `" +
                           he.get_attr("version") + "'");
}

// ------------------------------------------------------------------------
// The "atf_atffile" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_atffile {

static const atf::parser::token_type eof_type = 0;
static const atf::parser::token_type nl_type = 1;
static const atf::parser::token_type text_type = 2;
static const atf::parser::token_type colon_type = 3;
static const atf::parser::token_type conf_type = 4;
static const atf::parser::token_type dblquote_type = 5;
static const atf::parser::token_type equal_type = 6;
static const atf::parser::token_type hash_type = 7;
static const atf::parser::token_type prop_type = 8;
static const atf::parser::token_type tp_type = 9;
static const atf::parser::token_type tp_glob_type = 10;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(':', colon_type);
        add_delim('=', equal_type);
        add_delim('#', hash_type);
        add_quote('"', dblquote_type);
        add_keyword("conf", conf_type);
        add_keyword("prop", prop_type);
        add_keyword("tp", tp_type);
        add_keyword("tp-glob", tp_glob_type);
    }
};

} // namespace atf_atffile

// ------------------------------------------------------------------------
// The "atf_config" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_config {

static const atf::parser::token_type& eof_type = 0;
static const atf::parser::token_type& nl_type = 1;
static const atf::parser::token_type& text_type = 2;
static const atf::parser::token_type& dblquote_type = 3;
static const atf::parser::token_type& equal_type = 4;
static const atf::parser::token_type& hash_type = 5;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim('=', equal_type);
        add_delim('#', hash_type);
        add_quote('"', dblquote_type);
    }
};

} // namespace atf_config

// ------------------------------------------------------------------------
// The "atf_tcs" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_tcs {

static const atf::parser::token_type& eof_type = 0;
static const atf::parser::token_type& nl_type = 1;
static const atf::parser::token_type& text_type = 2;
static const atf::parser::token_type& colon_type = 3;
static const atf::parser::token_type& comma_type = 4;
static const atf::parser::token_type& tcs_count_type = 5;
static const atf::parser::token_type& tc_start_type = 6;
static const atf::parser::token_type& tc_end_type = 7;
static const atf::parser::token_type& passed_type = 8;
static const atf::parser::token_type& failed_type = 9;
static const atf::parser::token_type& skipped_type = 10;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(':', colon_type);
        add_delim(',', comma_type);
        add_keyword("tcs-count", tcs_count_type);
        add_keyword("tc-start", tc_start_type);
        add_keyword("tc-end", tc_end_type);
        add_keyword("passed", passed_type);
        add_keyword("failed", failed_type);
        add_keyword("skipped", skipped_type);
    }
};

} // namespace atf_tcs

// ------------------------------------------------------------------------
// The "atf_tps" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_tps {

static const atf::parser::token_type& eof_type = 0;
static const atf::parser::token_type& nl_type = 1;
static const atf::parser::token_type& text_type = 2;
static const atf::parser::token_type& colon_type = 3;
static const atf::parser::token_type& comma_type = 4;
static const atf::parser::token_type& tps_count_type = 5;
static const atf::parser::token_type& tp_start_type = 6;
static const atf::parser::token_type& tp_end_type = 7;
static const atf::parser::token_type& tc_start_type = 8;
static const atf::parser::token_type& tc_so_type = 9;
static const atf::parser::token_type& tc_se_type = 10;
static const atf::parser::token_type& tc_end_type = 11;
static const atf::parser::token_type& passed_type = 12;
static const atf::parser::token_type& failed_type = 13;
static const atf::parser::token_type& skipped_type = 14;
static const atf::parser::token_type& info_type = 16;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(':', colon_type);
        add_delim(',', comma_type);
        add_keyword("tps-count", tps_count_type);
        add_keyword("tp-start", tp_start_type);
        add_keyword("tp-end", tp_end_type);
        add_keyword("tc-start", tc_start_type);
        add_keyword("tc-so", tc_so_type);
        add_keyword("tc-se", tc_se_type);
        add_keyword("tc-end", tc_end_type);
        add_keyword("passed", passed_type);
        add_keyword("failed", failed_type);
        add_keyword("skipped", skipped_type);
        add_keyword("info", info_type);
    }
};

} // namespace atf_tps

// ------------------------------------------------------------------------
// The "atf_atffile_reader" class.
// ------------------------------------------------------------------------

impl::atf_atffile_reader::atf_atffile_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_atffile_reader::~atf_atffile_reader(void)
{
}

void
impl::atf_atffile_reader::got_conf(const std::string& name,
                                   const std::string& val)
{
}

void
impl::atf_atffile_reader::got_prop(const std::string& name,
                                   const std::string& val)
{
}

void
impl::atf_atffile_reader::got_tp(const std::string& name, bool isglob)
{
}

void
impl::atf_atffile_reader::got_eof(void)
{
}

void
impl::atf_atffile_reader::read(void)
{
    using atf::parser::parse_error;
    using namespace atf_atffile;

    std::pair< size_t, headers_map > hml = read_headers(m_is, 1);
    validate_content_type(hml.second, "application/X-atf-atffile", 1);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    for (;;) {
        try {
            atf::parser::token t =
                p.expect(conf_type, hash_type, prop_type, tp_type,
                         tp_glob_type, nl_type, eof_type,
                         "conf, #, prop, tp, tp-glob, a new line or eof");
            if (t.type() == eof_type)
                break;

            if (t.type() == conf_type) {
                t = p.expect(colon_type, "`:'");

                t = p.expect(text_type, "variable name");
                std::string var = t.text();

                t = p.expect(equal_type, "equal sign");

                t = p.expect(text_type, "word or quoted string");
                CALLBACK(p, got_conf(var, t.text()));
            } else if (t.type() == hash_type) {
                (void)p.rest_of_line();
            } else if (t.type() == prop_type) {
                t = p.expect(colon_type, "`:'");

                t = p.expect(text_type, "property name");
                std::string name = t.text();

                t = p.expect(equal_type, "equale sign");

                t = p.expect(text_type, "word or quoted string");
                CALLBACK(p, got_prop(name, t.text()));
            } else if (t.type() == tp_type) {
                t = p.expect(colon_type, "`:'");

                t = p.expect(text_type, "word or quoted string");
                CALLBACK(p, got_tp(t.text(), false));
            } else if (t.type() == tp_glob_type) {
                t = p.expect(colon_type, "`:'");

                t = p.expect(text_type, "word or quoted string");
                CALLBACK(p, got_tp(t.text(), true));
            } else if (t.type() == nl_type) {
                continue;
            } else
                UNREACHABLE;

            t = p.expect(nl_type, hash_type, eof_type,
                         "new line or comment");
            if (t.type() == hash_type) {
                (void)p.rest_of_line();
                t = p.next();
            } else if (t.type() == eof_type)
                break;
        } catch (const parse_error& pe) {
            p.add_error(pe);
            p.reset(nl_type);
        }
    }

    CALLBACK(p, got_eof());
}

// ------------------------------------------------------------------------
// The "atf_config_reader" class.
// ------------------------------------------------------------------------

impl::atf_config_reader::atf_config_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_config_reader::~atf_config_reader(void)
{
}

void
impl::atf_config_reader::got_var(const std::string& var,
                                 const std::string& val)
{
}

void
impl::atf_config_reader::got_eof(void)
{
}

void
impl::atf_config_reader::read(void)
{
    using atf::parser::parse_error;
    using namespace atf_config;

    std::pair< size_t, headers_map > hml = read_headers(m_is, 1);
    validate_content_type(hml.second, "application/X-atf-config", 1);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    for (;;) {
        try {
            atf::parser::token t = p.expect(eof_type, hash_type, text_type,
                                            nl_type,
                                            "eof, #, new line or text");
            if (t.type() == eof_type)
                break;

            if (t.type() == hash_type) {
                (void)p.rest_of_line();
                t = p.expect(nl_type, "new line");
            } else if (t.type() == text_type) {
                std::string name = t.text();

                t = p.expect(equal_type, "equal sign");

                t = p.expect(text_type, "word or quoted string");
                CALLBACK(p, got_var(name, t.text()));

                t = p.expect(nl_type, hash_type, "new line or comment");
                if (t.type() == hash_type) {
                    (void)p.rest_of_line();
                    t = p.expect(nl_type, "new line");
                }
            } else if (t.type() == nl_type) {
            } else
                UNREACHABLE;
        } catch (const parse_error& pe) {
            p.add_error(pe);
            p.reset(nl_type);
        }
    }

    CALLBACK(p, got_eof());
}

// ------------------------------------------------------------------------
// The "atf_tcs_reader" class.
// ------------------------------------------------------------------------

impl::atf_tcs_reader::atf_tcs_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_tcs_reader::~atf_tcs_reader(void)
{
}

void
impl::atf_tcs_reader::got_ntcs(size_t ntcs)
{
}

void
impl::atf_tcs_reader::got_tc_start(const std::string& tcname)
{
}

void
impl::atf_tcs_reader::got_tc_end(const atf::tests::tcr& tcr)
{
}

void
impl::atf_tcs_reader::got_stdout_line(const std::string& line)
{
}

void
impl::atf_tcs_reader::got_stderr_line(const std::string& line)
{
}

void
impl::atf_tcs_reader::got_eof(void)
{
}

void
impl::atf_tcs_reader::read_out_err(void* pptr,
                                   atf::io::unbuffered_istream& out,
                                   atf::io::unbuffered_istream& err)
{
    using namespace atf_tps;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    struct pollfd fds[2];
    fds[0].fd = out.get_fh().get();
    fds[0].events = POLLIN;
    fds[1].fd = err.get_fh().get();
    fds[1].events = POLLIN;

    do {
        fds[0].revents = 0;
        fds[1].revents = 0;
        if (::poll(fds, 2, -1) == -1)
            break;

        if (fds[0].revents & POLLIN) {
            std::string line;
            if (atf::io::getline(out, line).good()) {
                if (line == "__atf_tc_separator__")
                    fds[0].events &= ~POLLIN;
                else
                    CALLBACK(p, got_stdout_line(line));
            } else
                fds[0].events &= ~POLLIN;
        } else if (fds[0].revents & POLLHUP)
            fds[0].events &= ~POLLIN;

        if (fds[1].revents & POLLIN) {
            std::string line;
            if (atf::io::getline(err, line).good()) {
                if (line == "__atf_tc_separator__")
                    fds[1].events &= ~POLLIN;
                else
                    CALLBACK(p, got_stderr_line(line));
            } else
                fds[1].events &= ~POLLIN;
        } else if (fds[1].revents & POLLHUP)
            fds[1].events &= ~POLLIN;
    } while (fds[0].events & POLLIN || fds[1].events & POLLIN);
}

void
impl::atf_tcs_reader::read(atf::io::unbuffered_istream& out,
                           atf::io::unbuffered_istream& err)
{
    using atf::parser::parse_error;
    using namespace atf_tcs;

    std::pair< size_t, headers_map > hml = read_headers(m_is, 1);
    validate_content_type(hml.second, "application/X-atf-tcs", 1);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    try {
        atf::parser::token t = p.expect(tcs_count_type, "tcs-count field");
        t = p.expect(colon_type, "`:'");

        t = p.expect(text_type, "number of test cases");
        size_t ntcs = string_to_size_t(t.text());
        CALLBACK(p, got_ntcs(ntcs));

        t = p.expect(nl_type, "new line");

        size_t i = 0;
        while (m_is.good() && i < ntcs) {
            try {
                t = p.expect(tc_start_type, "start of test case");

                t = p.expect(colon_type, "`:'");

                t = p.expect(text_type, "test case name");
                std::string tcname = t.text();
                CALLBACK(p, got_tc_start(tcname));

                t = p.expect(nl_type, "new line");

                read_out_err(&p, out, err);
                if (i < ntcs - 1 && (!out.good() || !err.good()))
                    p.add_error(parse_error(0, "Missing terminators in "
                                               "stdout or stderr"));

                t = p.expect(tc_end_type, "end of test case");

                t = p.expect(colon_type, "`:'");

                t = p.expect(text_type, "test case name");
                if (t.text() != tcname)
                    throw parse_error(t.lineno(), "Test case name used in "
                                                  "terminator does not match "
                                                  "opening");

                t = p.expect(comma_type, "`,'");

                t = p.expect(passed_type, skipped_type, failed_type,
                             "passed, failed or skipped");
                if (t.type() == passed_type) {
                    CALLBACK(p, got_tc_end(tests::tcr::passed()));
                } else if (t.type() == failed_type) {
                    t = p.expect(comma_type, "`,'");
                    std::string reason = text::trim(p.rest_of_line());
                    if (reason.empty())
                        throw parse_error(t.lineno(),
                                          "Empty reason for failed "
                                          "test case result");
                    CALLBACK(p, got_tc_end(tests::tcr::failed(reason)));
                } else if (t.type() == skipped_type) {
                    t = p.expect(comma_type, "`,'");
                    std::string reason = text::trim(p.rest_of_line());
                    if (reason.empty())
                        throw parse_error(t.lineno(),
                                          "Empty reason for skipped "
                                          "test case result");
                    CALLBACK(p, got_tc_end(tests::tcr::skipped(reason)));
                } else
                    UNREACHABLE;

                t = p.expect(nl_type, "new line");
                i++;
            } catch (const parse_error& pe) {
                p.add_error(pe);
                p.reset(nl_type);
            }
        }

        t = p.expect(eof_type, "end of stream");
        CALLBACK(p, got_eof());
    } catch (const parse_error& pe) {
        p.add_error(pe);
        p.reset(nl_type);
    }
}

// ------------------------------------------------------------------------
// The "atf_tcs_writer" class.
// ------------------------------------------------------------------------

impl::atf_tcs_writer::atf_tcs_writer(std::ostream& os,
                                     std::ostream& p_cout,
                                     std::ostream& p_cerr,
                                     size_t ntcs) :
    m_os(os),
    m_cout(p_cout),
    m_cerr(p_cerr),
    m_ntcs(ntcs),
    m_curtc(0)
{
    headers_map hm;
    attrs_map ct_attrs;
    ct_attrs["version"] = "1";
    hm["Content-Type"] =
        header_entry("Content-Type", "application/X-atf-tcs", ct_attrs);
    write_headers(hm, m_os);

    m_os << "tcs-count: " << ntcs << std::endl;
    m_os.flush();
}

void
impl::atf_tcs_writer::start_tc(const std::string& tcname)
{
    m_tcname = tcname;
    m_os << "tc-start: " << tcname << std::endl;
    m_os.flush();
}

void
impl::atf_tcs_writer::end_tc(const atf::tests::tcr& tcr)
{
    PRE(m_curtc < m_ntcs);
    m_curtc++;
    if (m_curtc < m_ntcs) {
        m_cout << "__atf_tc_separator__\n";
        m_cerr << "__atf_tc_separator__\n";
    }
    m_cout.flush();
    m_cerr.flush();

    std::string end = "tc-end: " + m_tcname + ", ";
    switch (tcr.get_status()) {
    case tests::tcr::status_passed:
        end += "passed";
        break;

    case tests::tcr::status_failed:
        end += "failed, " + tcr.get_reason();
        break;

    case tests::tcr::status_skipped:
        end += "skipped, " + tcr.get_reason();
        break;

    default:
        UNREACHABLE;
    }
    m_os << end << std::endl;
    m_os.flush();
}

// ------------------------------------------------------------------------
// The "atf_tps_reader" class.
// ------------------------------------------------------------------------

impl::atf_tps_reader::atf_tps_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_tps_reader::~atf_tps_reader(void)
{
}

void
impl::atf_tps_reader::got_info(const std::string& what,
                               const std::string& val)
{
}

void
impl::atf_tps_reader::got_ntps(size_t ntps)
{
}

void
impl::atf_tps_reader::got_tp_start(const std::string& tp, size_t ntcs)
{
}

void
impl::atf_tps_reader::got_tp_end(const std::string& reason)
{
}

void
impl::atf_tps_reader::got_tc_start(const std::string& tcname)
{
}

void
impl::atf_tps_reader::got_tc_stdout_line(const std::string& line)
{
}

void
impl::atf_tps_reader::got_tc_stderr_line(const std::string& line)
{
}

void
impl::atf_tps_reader::got_tc_end(const atf::tests::tcr& tcr)
{
}

void
impl::atf_tps_reader::got_eof(void)
{
}

void
impl::atf_tps_reader::read_info(void* pptr)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    (void)p.expect(colon_type, "`:'");

    atf::parser::token t = p.expect(text_type, "info property name");
    (void)p.expect(comma_type, "`,'");
    got_info(t.text(), atf::text::trim(p.rest_of_line()));

    (void)p.expect(nl_type, "new line");
}

void
impl::atf_tps_reader::read_tp(void* pptr)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    atf::parser::token t = p.expect(tp_start_type,
                                    "start of test program");

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test program name");
    std::string tpname = t.text();

    t = p.expect(comma_type, "`,'");

    t = p.expect(text_type, "number of test programs");
    size_t ntcs = string_to_size_t(t.text());

    t = p.expect(nl_type, "new line");

    CALLBACK(p, got_tp_start(tpname, ntcs));

    size_t i = 0;
    while (p.good() && i < ntcs) {
        try {
            read_tc(&p);
            i++;
        } catch (const parse_error& pe) {
            p.add_error(pe);
            p.reset(nl_type);
        }
    }
    t = p.expect(tp_end_type, "end of test program");

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test program name");
    if (t.text() != tpname)
        throw parse_error(t.lineno(), "Test program name used in "
                                      "terminator does not match "
                                      "opening");

    t = p.expect(nl_type, comma_type,
                 "new line or comma_type");
    std::string reason;
    if (t.type() == comma_type) {
        reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for failed test program");
        t = p.next();
    }

    CALLBACK(p, got_tp_end(reason));
}

void
impl::atf_tps_reader::read_tc(void* pptr)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    atf::parser::token t = p.expect(tc_start_type, "start of test case");

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test case name");
    std::string tcname = t.text();
    CALLBACK(p, got_tc_start(tcname));

    t = p.expect(nl_type, "new line");

    t = p.expect(tc_end_type, tc_so_type, tc_se_type,
                 "end of test case or test case's stdout/stderr line");
    while (t.type() != tc_end_type &&
           (t.type() == tc_so_type || t.type() == tc_se_type)) {
        atf::parser::token t2 = t;

        t = p.expect(colon_type, "`:'");

        std::string line = p.rest_of_line();

        if (t2.type() == tc_so_type) {
            CALLBACK(p, got_tc_stdout_line(line));
        } else {
            INV(t2.type() == tc_se_type);
            CALLBACK(p, got_tc_stderr_line(line));
        }

        t = p.expect(nl_type, "new line");

        t = p.expect(tc_end_type, tc_so_type, tc_se_type,
                     "end of test case or test case's stdout/stderr line");
    }

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test case name");
    if (t.text() != tcname)
        throw parse_error(t.lineno(),
                          "Test case name used in terminator does not "
                          "match opening");

    t = p.expect(comma_type, "`,'");

    t = p.expect(passed_type, failed_type, skipped_type,
                 "passed, failed or skipped");
    if (t.type() == passed_type) {
        CALLBACK(p, got_tc_end(tests::tcr::passed()));
    } else if (t.type() == failed_type) {
        t = p.expect(comma_type, "`,'");
        std::string reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for failed test case result");
        CALLBACK(p, got_tc_end(tests::tcr::failed(reason)));
    } else if (t.type() == skipped_type) {
        t = p.expect(comma_type, "`,'");
        std::string reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for skipped test case result");
        CALLBACK(p, got_tc_end(tests::tcr::skipped(reason)));
    } else
        UNREACHABLE;

    t = p.expect(nl_type, "new line");
}

void
impl::atf_tps_reader::read(void)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    std::pair< size_t, headers_map > hml = read_headers(m_is, 1);
    validate_content_type(hml.second, "application/X-atf-tps", 2);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    try {
        atf::parser::token t;

        while ((t = p.expect(tps_count_type, info_type, "tps-count or info "
                             "field")).type() == info_type)
            read_info(&p);

        t = p.expect(colon_type, "`:'");

        t = p.expect(text_type, "number of test programs");
        size_t ntps = string_to_size_t(t.text());
        CALLBACK(p, got_ntps(ntps));

        t = p.expect(nl_type, "new line");

        size_t i = 0;
        while (p.good() && i < ntps) {
            try {
                read_tp(&p);
                i++;
            } catch (const parse_error& pe) {
                p.add_error(pe);
                p.reset(nl_type);
            }
        }

        while ((t = p.expect(eof_type, info_type, "end of stream or info "
                             "field")).type() == info_type)
            read_info(&p);
        CALLBACK(p, got_eof());
    } catch (const parse_error& pe) {
        p.add_error(pe);
        p.reset(nl_type);
    }
}

// ------------------------------------------------------------------------
// The "atf_tps_writer" class.
// ------------------------------------------------------------------------

impl::atf_tps_writer::atf_tps_writer(std::ostream& os) :
    m_os(os)
{
    headers_map hm;
    attrs_map ct_attrs;
    ct_attrs["version"] = "2";
    hm["Content-Type"] =
        header_entry("Content-Type", "application/X-atf-tps", ct_attrs);
    write_headers(hm, m_os);
}

void
impl::atf_tps_writer::info(const std::string& what, const std::string& val)
{
    m_os << "info: " << what << ", " << val << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::ntps(size_t p_ntps)
{
    m_os << "tps-count: " << p_ntps << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::start_tp(const std::string& tp, size_t ntcs)
{
    m_tpname = tp;
    m_os << "tp-start: " << tp << ", " << ntcs << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::end_tp(const std::string& reason)
{
    PRE(reason.find('\n') == std::string::npos);
    if (reason.empty())
        m_os << "tp-end: " << m_tpname << std::endl;
    else
        m_os << "tp-end: " << m_tpname << ", " << reason << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::start_tc(const std::string& tcname)
{
    m_tcname = tcname;
    m_os << "tc-start: " << tcname << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::stdout_tc(const std::string& line)
{
    m_os << "tc-so:" << line << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::stderr_tc(const std::string& line)
{
    m_os << "tc-se:" << line << std::endl;
    m_os.flush();
}

void
impl::atf_tps_writer::end_tc(const atf::tests::tcr& tcr)
{
    std::string str = "tc-end: " + m_tcname + ", ";
    switch (tcr.get_status()) {
    case atf::tests::tcr::status_passed:
        str += "passed";
        break;

    case atf::tests::tcr::status_skipped:
        str += "skipped, " + tcr.get_reason();
        break;

    case atf::tests::tcr::status_failed:
        str += "failed, " + tcr.get_reason();
        break;

    default:
        UNREACHABLE;
    }
    m_os << str << std::endl;
    m_os.flush();
}
