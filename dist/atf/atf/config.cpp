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

#include <map>

#include "atf/config.hpp"
#include "atf/env.hpp"
#include "atf/sanity.hpp"

static std::map< std::string, std::string > m_variables;

//
// Adds all predefined standard build-time variables to the m_variables
// map, considering the values a user may have provided in the environment.
//
// Can only be called once during the program's lifetime.
//
static
void
init_variables(void)
{
    PRE(m_variables.empty());

    if (atf::env::has("ATF_CONFDIR")) {
        const std::string& val = atf::env::get("ATF_CONFDIR");
        if (!val.empty())
            m_variables["atf_confdir"] = val;
        else
            m_variables["atf_confdir"] = ATF_CONFDIR;
    } else
        m_variables["atf_confdir"] = ATF_CONFDIR;

    if (atf::env::has("ATF_LIBEXECDIR")) {
        const std::string& val = atf::env::get("ATF_LIBEXECDIR");
        if (!val.empty())
            m_variables["atf_libexecdir"] = val;
        else
            m_variables["atf_libexecdir"] = ATF_LIBEXECDIR;
    } else
        m_variables["atf_libexecdir"] = ATF_LIBEXECDIR;

    if (atf::env::has("ATF_PKGDATADIR")) {
        const std::string& val = atf::env::get("ATF_PKGDATADIR");
        if (!val.empty())
            m_variables["atf_pkgdatadir"] = val;
        else
            m_variables["atf_pkgdatadir"] = ATF_PKGDATADIR;
    } else
        m_variables["atf_pkgdatadir"] = ATF_PKGDATADIR;

    if (atf::env::has("ATF_SHELL")) {
        const std::string& val = atf::env::get("ATF_SHELL");
        if (!val.empty())
            m_variables["atf_shell"] = val;
        else
            m_variables["atf_shell"] = ATF_SHELL;
    } else
        m_variables["atf_shell"] = ATF_SHELL;

    if (atf::env::has("ATF_WORKDIR")) {
        const std::string& val = atf::env::get("ATF_WORKDIR");
        if (!val.empty())
            m_variables["atf_workdir"] = val;
        else
            m_variables["atf_workdir"] = ATF_WORKDIR;
    } else
        m_variables["atf_workdir"] = ATF_WORKDIR;

    POST(!m_variables.empty());
}

const std::string&
atf::config::get(const std::string& varname)
{
    if (m_variables.empty())
        init_variables();

    PRE(has(varname));
    return m_variables[varname];
}

const std::map< std::string, std::string >&
atf::config::get_all(void)
{
    if (m_variables.empty())
        init_variables();

    return m_variables;
}

bool
atf::config::has(const std::string& varname)
{
    if (m_variables.empty())
        init_variables();

    return m_variables.find(varname) != m_variables.end();
}

namespace atf {
namespace config {
//
// Auxiliary function for the t_config test program so that it can
// revert the configuration's global status to an empty state and
// do new tests from there on.
//
// Ideally this shouldn't be part of the production library... but
// this is so small that it does not matter.
//
void
__reinit(void)
{
    m_variables.clear();
}
} // namespace config
} // namespace atf
