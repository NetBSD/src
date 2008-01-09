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

#include <sstream>

#include "atf/parser.hpp"

namespace impl = atf::parser;
#define IMPL_NAME "atf::parser"

// ------------------------------------------------------------------------
// The "parse_error" class.
// ------------------------------------------------------------------------

impl::parse_error::parse_error(size_t line, std::string msg) :
    std::runtime_error(msg),
    std::pair< size_t, std::string >(line, msg)
{
}

impl::parse_error::~parse_error(void)
    throw()
{
}

const char*
impl::parse_error::what(void)
    const throw()
{
    try {
        std::ostringstream oss;
        oss << "LONELY PARSE ERROR: " << first << ": " << second;
        m_msg = oss.str();
        return m_msg.c_str();
    } catch (...) {
        return "Could not format message for parsing error.";
    }
}

// ------------------------------------------------------------------------
// The "parse_errors" class.
// ------------------------------------------------------------------------

impl::parse_errors::parse_errors(void) :
    std::runtime_error("No parsing errors yet")
{
    m_msg.clear();
}

impl::parse_errors::~parse_errors(void)
    throw()
{
}

const char*
impl::parse_errors::what(void)
    const throw()
{
    try {
        std::ostringstream oss;
        for (const_iterator iter = begin(); iter != end(); iter++) {
            oss << (*iter).first << ": " << (*iter).second
                << std::endl;
        }
        m_msg = oss.str();
        return m_msg.c_str();
    } catch (...) {
        return "Could not format messages for parsing errors.";
    }
}

// ------------------------------------------------------------------------
// The "token" class.
// ------------------------------------------------------------------------

impl::token::token(void) :
    m_inited(false)
{
}

impl::token::token(size_t p_line,
                   const token_type& p_type,
                   const std::string& p_text) :
    m_inited(true),
    m_line(p_line),
    m_type(p_type),
    m_text(p_text)
{
}

size_t
impl::token::lineno(void)
    const
{
    return m_line;
}

const impl::token_type&
impl::token::type(void)
    const
{
    return m_type;
}

const std::string&
impl::token::text(void)
    const
{
    return m_text;
}

impl::token::operator bool(void)
    const
{
    return m_inited;
}

bool
impl::token::operator!(void)
    const
{
    return !m_inited;
}
