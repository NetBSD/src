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

#include <map>
#include <sstream>
#include <utility>

#include "atf-c++/formats.hpp"
#include "atf-c++/parser.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/text.hpp"

namespace impl = atf::formats;
#define IMPL_NAME "atf::formats"

#define CALLBACK(parser, func) \
    do { \
        if (!(parser).has_errors()) \
            func; \
    } while (false)

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
// The "atf_tcr_parser" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_tcr_parser {

static const atf::parser::token_type eof_type = 0;
static const atf::parser::token_type nl_type = 1;
static const atf::parser::token_type text_type = 2;
static const atf::parser::token_type colon_type = 3;
static const atf::parser::token_type result_type = 4;
static const atf::parser::token_type reason_type = 5;
static const atf::parser::token_type passed_type = 6;
static const atf::parser::token_type failed_type = 7;
static const atf::parser::token_type skipped_type = 8;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(':', colon_type);
        add_keyword("result", result_type);
        add_keyword("reason", reason_type);
        add_keyword("passed", passed_type);
        add_keyword("failed", failed_type);
        add_keyword("skipped", skipped_type);
    }
};

} // namespace atf_tcr_parser

// ------------------------------------------------------------------------
// The "atf_tp" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_tp {

static const atf::parser::token_type eof_type = 0;
static const atf::parser::token_type nl_type = 1;
static const atf::parser::token_type text_type = 2;
static const atf::parser::token_type colon_type = 3;
static const atf::parser::token_type dblquote_type = 4;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(':', colon_type);
        add_quote('"', dblquote_type);
    }
};

} // namespace atf_tp

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
// The "atf_tcr_reader" class.
// ------------------------------------------------------------------------

impl::atf_tcr_reader::atf_tcr_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_tcr_reader::~atf_tcr_reader(void)
{
}

void
impl::atf_tcr_reader::got_result(const std::string& str)
{
}

void
impl::atf_tcr_reader::got_reason(const std::string& str)
{
}

void
impl::atf_tcr_reader::got_eof(void)
{
}

void
impl::atf_tcr_reader::read(void)
{
    using atf::parser::parse_error;
    using namespace atf_tcr_parser;

    std::pair< size_t, headers_map > hml = read_headers(m_is, 1);
    validate_content_type(hml.second, "application/X-atf-tcr", 1);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    std::string result, reason;

    for (;;) {
        try {
            atf::parser::token t =
                p.expect(result_type, reason_type, nl_type, eof_type,
                         "result, reason, a new line or eof");
            if (t.type() == eof_type)
                break;

            if (t.type() == result_type) {
                t = p.expect(colon_type, "`:'");

                if (!result.empty())
                    throw parse_error(t.lineno(), "Result already specified");

                t = p.expect(passed_type, failed_type, skipped_type,
                             "passed, failed or skipped");
                result = t.text();
                CALLBACK(p, got_result(t.text()));
            } else if (t.type() == reason_type) {
                t = p.expect(colon_type, "`:'");

                if (result.empty())
                    throw parse_error(t.lineno(), "Reason must follow result");
                else if (result == "passed")
                    throw parse_error(t.lineno(), "Reason not allowed");

                if (!reason.empty())
                    throw parse_error(t.lineno(), "Reason already specified");

                reason = text::trim(p.rest_of_line());
                if (reason.empty())
                    throw parse_error(t.lineno(),
                                      "Empty reason for test case result");
                CALLBACK(p, got_reason(reason));
            } else if (t.type() == nl_type) {
                continue;
            } else
                UNREACHABLE;

            t = p.expect(nl_type, eof_type, "new line or comment");
            if (t.type() == eof_type)
                break;
        } catch (const parse_error& pe) {
            p.add_error(pe);
            p.reset(nl_type);
        }
    }

    atf::parser::token t = p.expect(eof_type, "end of stream");
    CALLBACK(p, got_eof());

    if (result.empty())
        p.add_error(parse_error(t.lineno(), "No result status specified"));
    else if (result != "passed" && reason.empty())
        p.add_error(parse_error(t.lineno(), "No reason specified"));
}

// ------------------------------------------------------------------------
// The "atf_tcr_writer" class.
// ------------------------------------------------------------------------

impl::atf_tcr_writer::atf_tcr_writer(std::ostream& os) :
    m_os(os)
{
    headers_map hm;
    attrs_map ct_attrs;
    ct_attrs["version"] = "1";
    hm["Content-Type"] =
        header_entry("Content-Type", "application/X-atf-tcr", ct_attrs);
    write_headers(hm, m_os);
}

void
impl::atf_tcr_writer::result(const std::string& str)
{
    m_os << "result: " << str << std::endl;
    m_os.flush();
}

void
impl::atf_tcr_writer::reason(const std::string& str)
{
    m_os << "reason: " << str << std::endl;
    m_os.flush();
}

// ------------------------------------------------------------------------
// The "atf_tp_reader" class.
// ------------------------------------------------------------------------

impl::atf_tp_reader::atf_tp_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_tp_reader::~atf_tp_reader(void)
{
}

void
impl::atf_tp_reader::got_tc(const std::string& ident,
                            const std::map< std::string, std::string >& md)
{
}

void
impl::atf_tp_reader::got_eof(void)
{
}

void
impl::atf_tp_reader::validate_and_insert(const std::string& name,
                                         const std::string& value,
                                         const size_t lineno,
                                         std::map< std::string, std::string >&
                                         md)
{
    using atf::parser::parse_error;

    if (value.empty())
        throw parse_error(lineno, "The value for '" + name +"' cannot be "
                          "empty");

    const std::string ident_regex = "^[_A-Za-z0-9]+$";
    const std::string integer_regex = "^[0-9]+$";

    if (name == "descr") {
        // Any non-empty value is valid.
    } else if (name == "ident") {
        if (!atf::text::match(value, ident_regex))
            throw parse_error(lineno, "The identifier must match " +
                              ident_regex + "; was '" + value + "'");
    } else if (name == "require.arch") {
    } else if (name == "require.config") {
    } else if (name == "require.machine") {
    } else if (name == "require.progs") {
    } else if (name == "require.user") {
    } else if (name == "timeout") {
        if (!atf::text::match(value, integer_regex))
            throw parse_error(lineno, "The timeout property requires an integer"
                              " value");
    } else if (name.size() > 2 && name[0] == 'X' && name[1] == '-') {
        // Any non-empty value is valid.
    } else {
        throw parse_error(lineno, "Unknown property '" + name + "'");
    }

    md.insert(std::make_pair(name, value));
}

void
impl::atf_tp_reader::read(void)
{
    using atf::parser::parse_error;
    using namespace atf_tp;

    std::pair< size_t, headers_map > hml = read_headers(m_is, 1);
    validate_content_type(hml.second, "application/X-atf-tp", 1);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    try {
        atf::parser::token t = p.expect(text_type, "property name");
        if (t.text() != "ident")
            throw parse_error(t.lineno(), "First property of a test case "
                              "must be 'ident'");

        std::map< std::string, std::string > props;
        do {
            const std::string name = t.text();
            t = p.expect(colon_type, "`:'");
            const std::string value = atf::text::trim(p.rest_of_line());
            t = p.expect(nl_type, "new line");
            validate_and_insert(name, value, t.lineno(), props);

            t = p.expect(eof_type, nl_type, text_type, "property name, new "
                         "line or eof");
            if (t.type() == nl_type || t.type() == eof_type) {
                const std::map< std::string, std::string >::const_iterator
                    iter = props.find("ident");
                if (iter == props.end())
                    throw parse_error(t.lineno(), "Test case definition did "
                                      "not define an 'ident' property");
                CALLBACK(p, got_tc((*iter).second, props));
                props.clear();

                if (t.type() == nl_type) {
                    t = p.expect(text_type, "property name");
                    if (t.text() != "ident")
                        throw parse_error(t.lineno(), "First property of a "
                                          "test case must be 'ident'");
                }
            }
        } while (t.type() != eof_type);
        CALLBACK(p, got_eof());
    } catch (const parse_error& pe) {
        p.add_error(pe);
        p.reset(nl_type);
    }
}

// ------------------------------------------------------------------------
// The "atf_tp_writer" class.
// ------------------------------------------------------------------------

impl::atf_tp_writer::atf_tp_writer(std::ostream& os) :
    m_os(os),
    m_is_first(true)
{
    headers_map hm;
    attrs_map ct_attrs;
    ct_attrs["version"] = "1";
    hm["Content-Type"] =
        header_entry("Content-Type", "application/X-atf-tp", ct_attrs);
    write_headers(hm, m_os);
}

void
impl::atf_tp_writer::start_tc(const std::string& ident)
{
    if (!m_is_first)
        m_os << std::endl;
    m_os << "ident: " << ident << std::endl;
    m_os.flush();
}

void
impl::atf_tp_writer::end_tc(void)
{
    if (m_is_first)
        m_is_first = false;
}

void
impl::atf_tp_writer::tc_meta_data(const std::string& name,
                                  const std::string& value)
{
    PRE(name != "ident");
    m_os << name << ": " << value << std::endl;
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
    using atf::tests::tcr;

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
        CALLBACK(p, got_tc_end(tcr(tcr::passed_state)));
    } else if (t.type() == failed_type) {
        t = p.expect(comma_type, "`,'");
        std::string reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for failed test case result");
        CALLBACK(p, got_tc_end(tcr(tcr::failed_state, reason)));
    } else if (t.type() == skipped_type) {
        t = p.expect(comma_type, "`,'");
        std::string reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for skipped test case result");
        CALLBACK(p, got_tc_end(tcr(tcr::skipped_state, reason)));
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
    if (tcr.get_state() == atf::tests::tcr::passed_state)
        str += "passed";
    else if (tcr.get_state() == atf::tests::tcr::skipped_state)
        str += "skipped, " + tcr.get_reason();
    else if (tcr.get_state() == atf::tests::tcr::failed_state)
        str += "failed, " + tcr.get_reason();
    else
        UNREACHABLE;
    m_os << str << std::endl;
    m_os.flush();
}
