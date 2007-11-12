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

#include <cctype>

#include "atf/text.hpp"

namespace impl = atf::text;
#define IMPL_NAME "atf::text"

std::string
impl::to_lower(const std::string& str)
{
    std::string lc;
    for (std::string::const_iterator iter = str.begin(); iter != str.end();
         iter++)
        lc += std::tolower(*iter);
    return lc;
}

std::vector< std::string >
impl::split(const std::string& str, const std::string& delim)
{
    std::vector< std::string > words;

    std::string::size_type pos = 0, newpos = 0;
    while (pos < str.length() && newpos != std::string::npos) {
        newpos = str.find(delim, pos);
        if (newpos != pos)
            words.push_back(str.substr(pos, newpos - pos));
        pos = newpos + delim.length();
    }

    return words;
}

std::string
impl::trim(const std::string& str)
{
    std::string::size_type pos1 = str.find_first_not_of(" \t");
    std::string::size_type pos2 = str.find_last_not_of(" \t");

    if (pos1 == std::string::npos && pos2 == std::string::npos)
        return "";
    else if (pos1 == std::string::npos)
        return str.substr(0, str.length() - pos2);
    else if (pos2 == std::string::npos)
        return str.substr(pos1);
    else
        return str.substr(pos1, pos2 - pos1 + 1);
}
