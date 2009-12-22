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

#if defined(TESTS_ATF_ATF_CXX_HPP_LIB_H)
#   error "Cannot include h_lib.hpp more than once."
#else
#   define TESTS_ATF_ATF_CXX_HPP_LIB_H
#endif

#define HEADER_TC(name, hdrname, sfile) \
    BUILD_TC(name, sfile, \
             "Tests that the " hdrname " file can be included on " \
             "its own, without any prerequisites", \
             "Build of " sfile " failed; " hdrname " is not self-contained");

#define BUILD_TC(name, sfile, descr, failmsg) \
    ATF_TEST_CASE(name); \
    ATF_TEST_CASE_HEAD(name) \
    { \
        set_md_var("descr", descr); \
    } \
    ATF_TEST_CASE_BODY(name) \
    { \
        build_check_cxx_o(*this, sfile, failmsg); \
    }

namespace atf {
namespace tests {
class tc;
}
}

void build_check_cxx_o(const atf::tests::tc&, const char*, const char*);
atf::fs::path get_h_processes_path(const atf::tests::tc&);
