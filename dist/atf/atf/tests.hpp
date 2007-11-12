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

#if !defined(_ATF_TESTS_HPP_)
#define _ATF_TESTS_HPP_

#include <map>
#include <string>

namespace atf {
namespace tests {

// ------------------------------------------------------------------------
// The "vars_map" class.
// ------------------------------------------------------------------------

class vars_map : public std::map< std::string, std::string >
{
public:
    vars_map(void);

    const std::string& get(const std::string&) const;
    const std::string& get(const std::string&, const std::string&) const;
    bool get_bool(const std::string&) const;
    bool get_bool(const std::string&, bool) const;

    bool has(const std::string&) const;

    static value_type parse(const std::string&);
};

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
public:
    enum status { status_passed, status_skipped, status_failed };

    tcr(void);

    static tcr passed(void);
    static tcr skipped(const std::string&);
    static tcr failed(const std::string&);

    status get_status(void) const;
    const std::string& get_reason(void) const;

private:
    status m_status;
    std::string m_reason;

    tcr(status, const std::string&);
};

// ------------------------------------------------------------------------
// The "tc" class.
// ------------------------------------------------------------------------

class tc {
    std::string m_ident;
    vars_map m_meta_data;
    vars_map m_config;

    std::string m_srcdir;

    void ensure_boolean(const std::string&);
    void ensure_not_empty(const std::string&);

    tcr safe_run(void) const;
    tcr fork_body(const std::string&) const;
    void fork_cleanup(const std::string&) const;

    void check_requirements(void) const;

protected:
    virtual void head(void) = 0;
    virtual void body(void) const = 0;
    virtual void cleanup(void) const;

    void require_prog(const std::string&) const;

public:
    tc(const std::string&);
    virtual ~tc(void);

    const std::string& get(const std::string&) const;
    bool get_bool(const std::string&) const;
    bool has(const std::string&) const;
    void set(const std::string&, const std::string&);

    const vars_map& config(void) const;

    const std::string& get_srcdir(void) const;

    void init(const vars_map&, const std::string&);
    tcr run(void) const;
};

} // namespace tests
} // namespace atf

#endif // !defined(_ATF_TESTS_HPP_)
