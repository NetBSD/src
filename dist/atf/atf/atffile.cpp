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

#include "atf/atffile.hpp"
#include "atf/exceptions.hpp"
#include "atf/expand.hpp"
#include "atf/formats.hpp"

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
            std::set< std::string > ms =
                atf::expand::expand_glob(name, m_dir.names());
            // Cannot use m_tps.insert(iterator, begin, end) here because it
            // does not work under Solaris.
            for (std::set< std::string >::const_iterator iter = ms.begin();
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

atf::atffile::atffile(const atf::fs::path& filename)
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

    // Update the atffile with the data accumulated in the reader.
    m_conf = r.conf();
    m_props = r.props();
    m_tps = r.tps();

    // Sanity checks.
    if (m_props.find("test-suite") == m_props.end())
        throw std::runtime_error("Undefined property `test-suite'");
}

const std::vector< std::string >&
atf::atffile::tps(void)
    const
{
    return m_tps;
}

const atf::tests::vars_map&
atf::atffile::conf(void)
    const
{
    return m_conf;
}

const atf::tests::vars_map&
atf::atffile::props(void)
    const
{
    return m_props;
}
