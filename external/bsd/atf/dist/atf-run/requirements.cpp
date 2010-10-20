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

// TODO: We probably don't want to raise std::runtime_error for the errors
// detected in this file.
#include <stdexcept>

#include "atf-c++/config.hpp"

#include "atf-c++/detail/fs.hpp"
#include "atf-c++/detail/env.hpp"
#include "atf-c++/detail/sanity.hpp"
#include "atf-c++/detail/text.hpp"

#include "requirements.hpp"
#include "user.hpp"

namespace impl = atf::atf_run;

namespace {

static
bool
has_program(const atf::fs::path& program)
{
    bool found = false;

    if (program.is_absolute()) {
        found = atf::fs::is_executable(program);
    } else {
        if (program.str().find('/') != std::string::npos)
            throw std::runtime_error("Relative paths are not allowed "
                                     "when searching for a program (" +
                                     program.str() + ")");

        const std::vector< std::string > dirs = atf::text::split(
            atf::env::get("PATH"), ":");
        for (std::vector< std::string >::const_iterator iter = dirs.begin();
             !found && iter != dirs.end(); iter++) {
            const atf::fs::path& p = atf::fs::path(*iter) / program;
            if (atf::fs::is_executable(p))
                found = true;
        }
    }

    return found;
}

static
std::string
check_arch(const std::string& arches)
{
    const std::vector< std::string > v = atf::text::split(arches, " ");

    for (std::vector< std::string >::const_iterator iter = v.begin();
         iter != v.end(); iter++) {
        if ((*iter) == atf::config::get("atf_arch"))
            return "";
    }

    if (v.size() == 1)
        return "Requires the '" + arches + "' architecture";
    else
        return "Requires one of the '" + arches + "' architectures";
}

static
std::string
check_config(const std::string& variables, const atf::tests::vars_map& config)
{
    const std::vector< std::string > v = atf::text::split(variables, " ");
    for (std::vector< std::string >::const_iterator iter = v.begin();
         iter != v.end(); iter++) {
        if (config.find((*iter)) == config.end())
            return "Required configuration variable '" + (*iter) + "' not "
                "defined";
    }
    return "";
}

static
std::string
check_machine(const std::string& machines)
{
    const std::vector< std::string > v = atf::text::split(machines, " ");

    for (std::vector< std::string >::const_iterator iter = v.begin();
         iter != v.end(); iter++) {
        if ((*iter) == atf::config::get("atf_machine"))
            return "";
    }

    if (v.size() == 1)
        return "Requires the '" + machines + "' machine type";
    else
        return "Requires one of the '" + machines + "' machine types";
}

static
std::string
check_progs(const std::string& progs)
{
    const std::vector< std::string > v = atf::text::split(progs, " ");
    for (std::vector< std::string >::const_iterator iter = v.begin();
         iter != v.end(); iter++) {
        if (!has_program(atf::fs::path(*iter)))
            return "Required program '" + (*iter) + "' not found in the PATH";
    }
    return "";
}

static
std::string
check_user(const std::string& user)
{
    if (user == "root") {
        if (!impl::is_root())
            return "Requires root privileges";
        else
            return "";
    } else if (user == "unprivileged") {
        if (impl::is_root())
            return "Requires an unprivileged user";
        else
            return "";
    } else
        throw std::runtime_error("Invalid value '" + user + "' for property "
                                 "require.user");
}

} // anonymous namespace

std::string
impl::check_requirements(const atf::tests::vars_map& metadata,
                         const atf::tests::vars_map& config)
{
    std::string failure_reason = "";

    for (atf::tests::vars_map::const_iterator iter = metadata.begin();
         failure_reason.empty() && iter != metadata.end(); iter++) {
        const std::string& name = (*iter).first;
        const std::string& value = (*iter).second;
        INV(!value.empty()); // Enforced by application/X-atf-tp parser.

        if (name == "require.arch")
            failure_reason = check_arch(value);
        else if (name == "require.config")
            failure_reason = check_config(value, config);
        else if (name == "require.machine")
            failure_reason = check_machine(value);
        else if (name == "require.progs")
            failure_reason = check_progs(value);
        else if (name == "require.user")
            failure_reason = check_user(value);
        else {
            // Unknown require.* properties are forbidden by the
            // application/X-atf-tp parser.
            INV(failure_reason.find("require.") != 0);
        }
    }

    return failure_reason;
}
