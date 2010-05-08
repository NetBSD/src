//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2010 The NetBSD Foundation, Inc.
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

#include <fstream>

#include "atf-c++/atffile.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/expand.hpp"
#include "atf-c++/formats.hpp"
#include "atf-c++/sanity.hpp"

namespace impl = atf::atffile;

// ------------------------------------------------------------------------
// The "reader" helper class.
// ------------------------------------------------------------------------

class reader : public atf::formats::atf_atffile_reader {
    const atf::fs::directory& m_dir;
    atf::tests::vars_map m_conf, m_props;
    std::vector< std::string > m_tps;

    void
    got_tp(const std::string& name, bool isglob)
    {
        if (isglob) {
            std::vector< std::string > ms =
                atf::expand::expand_glob(name, m_dir.names());
            // Cannot use m_tps.insert(iterator, begin, end) here because it
            // does not work under Solaris.
            for (std::vector< std::string >::const_iterator iter = ms.begin();
                 iter != ms.end(); iter++)
                m_tps.push_back(*iter);
        } else {
            if (m_dir.find(name) == m_dir.end())
                throw atf::not_found_error< atf::fs::path >
                    ("Cannot locate the " + name + " file",
                     atf::fs::path(name));
            m_tps.push_back(name);
        }
    }

    void
    got_prop(const std::string& name, const std::string& val)
    {
        m_props[name] = val;
    }

    void
    got_conf(const std::string& var, const std::string& val)
    {
        m_conf[var] = val;
    }

public:
    reader(std::istream& is, const atf::fs::directory& dir) :
        atf::formats::atf_atffile_reader(is),
        m_dir(dir)
    {
    }

    const atf::tests::vars_map&
    conf(void)
        const
    {
        return m_conf;
    }

    const atf::tests::vars_map&
    props(void)
        const
    {
        return m_props;
    }

    const std::vector< std::string >&
    tps(void)
        const
    {
        return m_tps;
    }
};

// ------------------------------------------------------------------------
// The "atffile" class.
// ------------------------------------------------------------------------

impl::atffile::atffile(const atf::tests::vars_map& config_vars,
                       const std::vector< std::string >& test_program_names,
                       const atf::tests::vars_map& properties) :
    m_conf(config_vars),
    m_tps(test_program_names),
    m_props(properties)
{
    PRE(properties.find("test-suite") != properties.end());
}

const std::vector< std::string >&
impl::atffile::tps(void)
    const
{
    return m_tps;
}

const atf::tests::vars_map&
impl::atffile::conf(void)
    const
{
    return m_conf;
}

const atf::tests::vars_map&
impl::atffile::props(void)
    const
{
    return m_props;
}

// ------------------------------------------------------------------------
// Free functions.
// ------------------------------------------------------------------------

// XXX Glob expansion and file existance checks certainly do not belong in
// a *parser*.  This needs to be taken out...
impl::atffile
impl::read(const atf::fs::path& filename)
{
    // Scan the directory where the atffile lives in to gather a list of
    // all possible test programs in it.
    fs::directory dir(filename.branch_path());
    dir.erase(filename.leaf_name());
    fs::directory::iterator iter = dir.begin();
    while (iter != dir.end()) {
        const std::string& name = (*iter).first;
        const fs::file_info& fi = (*iter).second;

        // Discard hidden files and non-executable ones so that they are
        // not candidates for glob matching.
        if (name[0] == '.' || (!fi.is_owner_executable() &&
                               !fi.is_group_executable()))
            dir.erase(iter++);
        else
            iter++;
    }

    // Parse the atffile.
    std::ifstream is(filename.c_str());
    if (!is)
        throw atf::not_found_error< fs::path >
            ("Cannot open Atffile", filename);
    reader r(is, dir);
    r.read();
    is.close();

    // Sanity checks.
    if (r.props().find("test-suite") == r.props().end())
        throw atf::not_found_error< std::string >
            ("Undefined property `test-suite'", "test-suite");

    return atffile(r.conf(), r.tps(), r.props());
}
