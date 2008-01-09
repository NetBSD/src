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

#if !defined(_ATF_UTILS_HPP_)
#define _ATF_UTILS_HPP_

#include <cstddef>

namespace atf {
namespace utils {

// ------------------------------------------------------------------------
// The "auto_array" class.
// ------------------------------------------------------------------------

template< class T >
class auto_array {
    T* m_ptr;

public:
    auto_array(T* = NULL) throw();
    auto_array(auto_array< T >&) throw();
    ~auto_array(void) throw();

    T* get(void) throw();
    T* release(void) throw();
    void reset(T* = NULL) throw();

    auto_array< T >& operator=(auto_array< T >&) throw();
};

template< class T >
auto_array< T >::auto_array(T* ptr)
    throw() :
    m_ptr(ptr)
{
}

template< class T >
auto_array< T >::auto_array(auto_array< T >& ptr)
    throw() :
    m_ptr(ptr.release())
{
}

template< class T >
auto_array< T >::~auto_array(void)
    throw()
{
    if (m_ptr != NULL)
        delete [] m_ptr;
}

template< class T >
T*
auto_array< T >::get(void)
    throw()
{
    return m_ptr;
}

template< class T >
T*
auto_array< T >::release(void)
    throw()
{
    T* ptr = m_ptr;
    m_ptr = NULL;
    return ptr;
}

template< class T >
void
auto_array< T >::reset(T* ptr)
    throw()
{
    if (m_ptr != NULL)
        delete [] m_ptr;
    m_ptr = ptr;
}

template< class T >
auto_array< T >&
auto_array< T >::operator=(auto_array< T >& ptr)
    throw()
{
    reset(ptr.release());
    return *this;
}

} // namespace utils
} // namespace atf

#endif // !defined(_ATF_UTILS_HPP_)
