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

#include <cstdio>

#include "atf/sanity.hpp"

namespace std {
#if !defined(HAVE_SNPRINTF_IN_STD)
    using ::snprintf;
#endif
}

namespace impl = atf::sanity;
#define IMPL_NAME "atf::sanity"

// ------------------------------------------------------------------------
// The "sanity_error" class.
// ------------------------------------------------------------------------

impl::sanity_error::sanity_error(const char* p_file,
                                 size_t p_line,
                                 const char* p_msg)
    throw() :
    std::logic_error(p_msg),
    m_file(p_file),
    m_line(p_line),
    m_msg(p_msg)
{
    m_buffer[0] = '\0';
}

impl::sanity_error::~sanity_error(void)
    throw()
{
}

const char*
impl::sanity_error::file(void)
    const
    throw()
{
    return m_file;
}

size_t
impl::sanity_error::line(void)
    const
    throw()
{
    return m_line;
}

const char*
impl::sanity_error::format_message(const char* type)
    const
    throw()
{
    if (m_buffer[0] == '\0') {
        std::snprintf(m_buffer, sizeof(m_buffer),
                      "%s check failed at %s:%zu: %s",
                      type, m_file, m_line, m_msg);
    }
    return m_buffer;
}

// ------------------------------------------------------------------------
// The "precondition_error" class.
// ------------------------------------------------------------------------

impl::precondition_error::precondition_error(const char* p_file,
                                             size_t p_line,
                                             const char* p_msg)
    throw() :
    sanity_error(p_file, p_line, p_msg)
{
}

const char*
impl::precondition_error::what(void)
    const
    throw()
{
    return format_message("Precondition");
}

// ------------------------------------------------------------------------
// The "postcondition_error" class.
// ------------------------------------------------------------------------

impl::postcondition_error::postcondition_error(const char* p_file,
                                               size_t p_line,
                                               const char* p_msg)
    throw() :
    sanity_error(p_file, p_line, p_msg)
{
}

const char*
impl::postcondition_error::what(void)
    const
    throw()
{
    return format_message("Postcondition");
}

// ------------------------------------------------------------------------
// The "invariant_error" class.
// ------------------------------------------------------------------------

impl::invariant_error::invariant_error(const char* p_file,
                                       size_t p_line,
                                       const char* p_msg)
    throw() :
    sanity_error(p_file, p_line, p_msg)
{
}

const char*
impl::invariant_error::what(void)
    const
    throw()
{
    return format_message("Invariant");
}
