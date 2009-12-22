//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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

#if !defined(_ATF_CXX_TESTS_HPP_)
#define _ATF_CXX_TESTS_HPP_

#include <map>
#include <string>

extern "C" {
#include <atf-c/tc.h>
#include <atf-c/tcr.h>
}

#include <atf-c++/fs.hpp>
#include <atf-c++/utils.hpp>

namespace atf {
namespace tests {

// ------------------------------------------------------------------------
// The "vars_map" class.
// ------------------------------------------------------------------------

typedef std::map< std::string, std::string > vars_map;

// ------------------------------------------------------------------------
// The "tcr" class.
// ------------------------------------------------------------------------

//!
//! \brief Holds the results of a test case's execution.
//!
//! The tcr class holds the information that describes the results of a
//! test case's execution.  This is composed of an exit code and a reason
//! for that exit code.
//!
//! TODO: Complete documentation for this class.  Not done yet because it
//! is worth to investigate if this class could be rewritten as several
//! different classes, one for each status.
//!
class tcr {
    atf_tcr_t m_tcr;

public:
    typedef atf_tcr_state_t state;

    static const state passed_state;
    static const state failed_state;
    static const state skipped_state;

    tcr(state);
    tcr(state, const std::string&);
    tcr(const tcr&);
    ~tcr(void);

    state get_state(void) const;
    const std::string get_reason(void) const;

    tcr& operator=(const tcr&);
};

// ------------------------------------------------------------------------
// The "tc" class.
// ------------------------------------------------------------------------

class tc : utils::noncopyable {
    std::string m_ident;
    atf_map_t m_config;
    atf_tc_t m_tc;

protected:
    virtual void head(void) = 0;
    virtual void body(void) const = 0;
    virtual void cleanup(void) const;

    void require_prog(const std::string&) const;

    static void wrap_head(atf_tc_t *);
    static void wrap_body(const atf_tc_t *);
    static void wrap_cleanup(const atf_tc_t *);

public:
    tc(const std::string&);
    virtual ~tc(void);

    void init(const vars_map&);

    const std::string get_config_var(const std::string&) const;
    const std::string get_config_var(const std::string&, const std::string&)
        const;
    const std::string get_md_var(const std::string&) const;
    bool has_config_var(const std::string&) const;
    bool has_md_var(const std::string&) const;
    void set_md_var(const std::string&, const std::string&);

    tcr run(int, int, const fs::path&) const;

    // To be called from the child process only.
    static void pass(void);
    static void fail(const std::string&);
    static void skip(const std::string&);
};

} // namespace tests
} // namespace atf

#endif // !defined(_ATF_CXX_TESTS_HPP_)
