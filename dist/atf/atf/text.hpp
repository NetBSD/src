//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

#if !defined(_ATF_TEXT_HPP_)
#define _ATF_TEXT_HPP_

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace atf {
namespace text {

//!
//! \brief Joins multiple words into a string.
//!
//! Joins a list of words into a string, separating them using the provided
//! separator.  Empty words are not omitted.
//!
template< class T >
std::string
join(const T& words, const std::string& separator)
{
    std::string str;

    typename T::const_iterator iter = words.begin();
    bool done = iter == words.end();
    while (!done) {
        str += *iter;
        iter++;
        if (iter != words.end())
            str += separator;
        else
            done = true;
    }

    return str;
}

//!
//! \brief Splits a string into words.
//!
//! Splits the given string into multiple words, all separated by the
//! given delimiter.  Multiple occurrences of the same delimiter are
//! not condensed so that rejoining the words later on using the same
//! delimiter results in the original string.
//!
std::vector< std::string > split(const std::string&, const std::string&);

//!
//! \brief Removes whitespace from the beginning and end of a string.
//!
std::string trim(const std::string&);

//!
//! \brief Changes the case of a string to lowercase.
//!
//! Returns a new string that is a lowercased version of the original
//! one.
//!
std::string to_lower(const std::string&);

//!
//! \brief Converts the given object to a string.
//!
//! Returns a string with the representation of the given object.  There
//! must exist an operator<< method for that object.
//!
template< class T >
std::string
to_string(const T& ob)
{
    std::ostringstream ss;
    ss << ob;
    return ss.str();
}

//!
//! \brief Converts the given string to another type.
//!
//! Attempts to convert the given string to the requested type.  Throws
//! an exception if the conversion failed.
//!
template< class T >
T
to_type(const std::string& str)
{
    std::istringstream ss(str);
    T value;
    ss >> value;
    if (!ss.eof() || (!ss.eof() && !ss.good()))
        throw std::runtime_error("Cannot convert string to requested type");
    return value;
}

} // namespace text
} // namespace atf

#endif // !defined(_ATF_TEXT_HPP_)
