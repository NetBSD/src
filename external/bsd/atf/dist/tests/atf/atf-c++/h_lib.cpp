//
// Automated Testing Framework (atf)
//
// Copyright (c) 2009 The NetBSD Foundation, Inc.
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

#include <string>
#include <vector>

#include "atf-c++/check.hpp"
#include "atf-c++/config.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/macros.hpp"
#include "atf-c++/process.hpp"

#include "h_lib.hpp"

void
build_check_cxx_o(const atf::tests::tc& tc, const char* sfile,
                  const char* failmsg)
{
    std::vector< std::string > optargs;
    optargs.push_back("-I" + atf::config::get("atf_includedir"));

    const atf::fs::path sfilepath =
        atf::fs::path(tc.get_config_var("srcdir")) / sfile;

    if (!atf::check::build_cxx_o(sfilepath, atf::fs::path("test.o"),
                                 atf::process::argv_array(optargs)))
        ATF_FAIL(failmsg);
}

atf::fs::path
get_h_processes_path(const atf::tests::tc& tc)
{
    return atf::fs::path(tc.get_config_var("srcdir")) /
           ".." / "atf-c" / "h_processes";
}
