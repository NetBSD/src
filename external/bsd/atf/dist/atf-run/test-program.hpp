//
// Automated Testing Framework (atf)
//
// Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <atf-c++/formats.hpp>
#include <atf-c++/fs.hpp>
#include <atf-c++/process.hpp>
#include <atf-c++/tests.hpp>

namespace atf {
namespace atf_run {

typedef std::map< std::string, atf::tests::vars_map > test_cases_map;

struct metadata {
    test_cases_map test_cases;

    metadata(void)
    {
    }

    metadata(const test_cases_map& p_test_cases) :
        test_cases(p_test_cases)
    {
    }
};

metadata get_metadata(const atf::fs::path&, const atf::tests::vars_map&);
std::pair< std::string, atf::process::status > run_test_case(const atf::fs::path&,
    const std::string&, const std::string&, const atf::tests::vars_map&,
    const atf::tests::vars_map&, const atf::fs::path&, const atf::fs::path&,
    atf::formats::atf_tps_writer&);

} // namespace atf_run
} // namespace atf
