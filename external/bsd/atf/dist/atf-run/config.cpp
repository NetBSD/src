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

#include <fstream>
#include <vector>

#include "atf-c++/config.hpp"
#include "atf-c++/env.hpp"
#include "atf-c++/formats.hpp"
#include "atf-c++/fs.hpp"

#include "config.hpp"

namespace impl = atf::atf_run;

namespace {

class config_reader : public atf::formats::atf_config_reader {
    atf::tests::vars_map m_vars;

    void
    got_var(const std::string& var, const std::string& name)
    {
        m_vars[var] = name;
    }

public:
    config_reader(std::istream& is) :
        atf::formats::atf_config_reader(is)
    {
    }

    const atf::tests::vars_map&
    get_vars(void)
        const
    {
        return m_vars;
    }
};

template< class K, class V >
static
void
merge_maps(std::map< K, V >& dest, const std::map< K, V >& src)
{
    for (typename std::map< K, V >::const_iterator iter = src.begin();
         iter != src.end(); iter++)
        dest[(*iter).first] = (*iter).second;
}

static
void
merge_config_file(const atf::fs::path& config_path,
                  atf::tests::vars_map& config)
{
    std::ifstream is(config_path.c_str());
    if (is) {
        config_reader reader(is);
        reader.read();
        merge_maps(config, reader.get_vars());
    }
}

static
std::vector< atf::fs::path >
get_config_dirs(void)
{
    std::vector< atf::fs::path > dirs;
    dirs.push_back(atf::fs::path(atf::config::get("atf_confdir")));
    if (atf::env::has("HOME"))
        dirs.push_back(atf::fs::path(atf::env::get("HOME")) / ".atf");
    return dirs;
}

} // anonymous namespace

atf::tests::vars_map
impl::merge_configs(const atf::tests::vars_map& lower,
                    const atf::tests::vars_map& upper)
{
    atf::tests::vars_map merged = lower;
    merge_maps(merged, upper);
    return merged;
}

atf::tests::vars_map
impl::read_config_files(const std::string& test_suite_name)
{
    atf::tests::vars_map config;

    const std::vector< atf::fs::path > dirs = get_config_dirs();
    for (std::vector< atf::fs::path >::const_iterator iter = dirs.begin();
         iter != dirs.end(); iter++) {
        merge_config_file((*iter) / "common.conf", config);
        merge_config_file((*iter) / (test_suite_name + ".conf"), config);
    }

    return config;
}
