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

#if !defined(_ATF_SANITY_HPP_)
#define _ATF_SANITY_HPP_

#include <stdexcept>

namespace atf {
namespace sanity {

// ------------------------------------------------------------------------
// The "sanity_error" class.
// ------------------------------------------------------------------------

class sanity_error : public std::logic_error {
    const char* m_file;
    size_t m_line;
    const char* m_msg;

    mutable char m_buffer[1024];

protected:
    const char* format_message(const char*) const throw();

public:
    sanity_error(const char*, size_t, const char*) throw();
    virtual ~sanity_error(void) throw();

    const char* file(void) const throw();
    size_t line(void) const throw();

    virtual const char* what(void) const throw() = 0;
};

// ------------------------------------------------------------------------
// The "precondition_error" class.
// ------------------------------------------------------------------------

class precondition_error : public sanity_error {
public:
    precondition_error(const char*, size_t, const char*) throw();

    const char* what(void) const throw();
};

// ------------------------------------------------------------------------
// The "postcondition_error" class.
// ------------------------------------------------------------------------

class postcondition_error : public sanity_error {
public:
    postcondition_error(const char*, size_t, const char*) throw();

    const char* what(void) const throw();
};

// ------------------------------------------------------------------------
// The "invariant_error" class.
// ------------------------------------------------------------------------

class invariant_error : public sanity_error {
public:
    invariant_error(const char*, size_t, const char*) throw();

    const char* what(void) const throw();
};

// ------------------------------------------------------------------------
// Auxiliary macros.
// ------------------------------------------------------------------------

#define PRE(cond) \
    do { \
        if (!(cond)) \
            throw atf::sanity::precondition_error(__FILE__, __LINE__, \
                                                  #cond); \
    } while (false)

#define POST(cond) \
    do { \
        if (!(cond)) \
            throw atf::sanity::postcondition_error(__FILE__, __LINE__, \
                                                   #cond); \
    } while (false)

#define INV(cond) \
    do { \
        if (!(cond)) \
            throw atf::sanity::invariant_error(__FILE__, __LINE__, \
                                               #cond); \
    } while (false)

#define UNREACHABLE INV(false)

} // namespace sanity
} // namespace atf

#endif // !defined(_ATF_SANITY_HPP_)
