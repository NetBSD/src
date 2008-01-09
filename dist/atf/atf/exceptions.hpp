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

#if !defined(_ATF_EXCEPTIONS_HPP_)
#define _ATF_EXCEPTIONS_HPP_

#include <stdexcept>

namespace atf {

template< class T >
class not_found_error :
    public std::runtime_error
{
    T m_value;

public:
    not_found_error(const std::string& message, const T& value) throw();

    virtual ~not_found_error(void) throw();

    const T& get_value(void) const throw();
};

template< class T >
inline
not_found_error< T >::not_found_error(const std::string& message,
                                      const T& value)
    throw() :
    std::runtime_error(message),
    m_value(value)
{
}

template< class T >
inline
not_found_error< T >::~not_found_error(void)
    throw()
{
}

template< class T >
inline
const T&
not_found_error< T >::get_value(void)
    const
    throw()
{
    return m_value;
}

class system_error : public std::runtime_error {
    int m_sys_err;
    mutable std::string m_message;

public:
    system_error(const std::string&, const std::string&, int);
    ~system_error(void) throw();

    int code(void) const throw();
    const char* what(void) const throw();
};

class usage_error : public std::runtime_error {
    char m_text[4096];

public:
    usage_error(const char* fmt, ...) throw();
    ~usage_error(void) throw();

    const char* what(void) const throw();
};

} // namespace atf

#endif // !defined(_ATF_EXCEPTIONS_HPP_)
