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

extern "C" {
#include "atf-c/dynstr.h"
#include "atf-c/ui.h"
}

#include "atf-c++/env.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/text.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/ui.hpp"

namespace impl = atf::ui;
#define IMPL_NAME "atf::ui"

std::string
impl::format_error(const std::string& prog_name, const std::string& error)
{
    return format_text_with_tag("ERROR: " + error, prog_name + ": ", true);
}

std::string
impl::format_info(const std::string& prog_name, const std::string& msg)
{
    return format_text_with_tag(msg, prog_name + ": ", true);
}

std::string
impl::format_text(const std::string& text)
{
    return format_text_with_tag(text, "", false, 0);
}

std::string
impl::format_text_with_tag(const std::string& text, const std::string& tag,
                           bool repeat, size_t col)
{
    atf_dynstr_t dest;
    atf_error_t err;

    err = atf_dynstr_init(&dest);
    if (atf_is_error(err))
        throw_atf_error(err);

    try {
        err = atf_ui_format_fmt(&dest, tag.c_str(), repeat, col, "%s",
                                text.c_str());
        if (atf_is_error(err))
            throw_atf_error(err);

        std::string formatted(atf_dynstr_cstring(&dest));
        atf_dynstr_fini(&dest);
        return formatted;
    } catch (...) {
        atf_dynstr_fini(&dest);
        throw;
    }
}

std::string
impl::format_warning(const std::string& prog_name, const std::string& error)
{
    return format_text_with_tag("WARNING: " + error, prog_name + ": ", true);
}
