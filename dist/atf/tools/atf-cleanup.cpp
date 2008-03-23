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

#include <cstdlib>
#include <iostream>

#include "atf/application.hpp"
#include "atf/fs.hpp"
#include "atf/ui.hpp"

class atf_cleanup : public atf::application::app {
    static const char* m_description;

    std::string specific_args(void) const;

public:
    atf_cleanup(void);

    int main(void);
};

const char* atf_cleanup::m_description =
    "atf-cleanup recursively removes a test case's work directory, "
    "unmounting any file systems it may contain and preventing to "
    "recurse into them if the unmounting fails.";

atf_cleanup::atf_cleanup(void) :
    app(m_description, "atf-cleanup(1)", "atf(7)")
{
}

std::string
atf_cleanup::specific_args(void)
    const
{
    return "[path1 [.. pathN]]";
}

int
atf_cleanup::main(void)
{
    for (int i = 0; i < m_argc; i++)
        atf::fs::cleanup(atf::fs::path(m_argv[i]));

    return EXIT_SUCCESS;
}

int
main(int argc, char* const* argv)
{
    return atf_cleanup().run(argc, argv);
}
