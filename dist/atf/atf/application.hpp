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

#if !defined(_ATF_APPLICATION_HPP_)
#define _ATF_APPLICATION_HPP_

#include <ostream>
#include <set>
#include <string>

#include <atf/exceptions.hpp>

namespace atf {

class application {
    void process_options(void);
    void usage(std::ostream&);

    bool inited(void);

protected:
    class option {
        char m_character;
        std::string m_argument;
        std::string m_description;

        friend class application;

    public:
        option(char, const std::string&, const std::string&);

        bool operator<(const option&) const;
    };
    typedef std::set< option > options_set;

    int m_argc;
    char* const* m_argv;

    const char* m_prog_name;
    std::string m_description;
    std::string m_manpage;

    options_set options(void);

    // To be redefined.
    virtual std::string specific_args(void) const;
    virtual options_set specific_options(void) const;
    virtual void process_option(int, const char*);
    virtual int main(void) = 0;

public:
    application(const std::string&, const std::string&);
    virtual ~application(void);

    int run(int, char* const* argv);
};

} // namespace atf

#endif // !defined(_ATF_APPLICATION_HPP_)
