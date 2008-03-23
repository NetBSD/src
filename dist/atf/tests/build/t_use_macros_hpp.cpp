//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008 The NetBSD Foundation, Inc.
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

#include <stdexcept>

#include <atf/macros.hpp>

void
atf_check_inside_if(void)
{
    // Make sure that ATF_CHECK can be used inside an if statement that
    // does not have braces.  Earlier versions of it generated an error
    // if there was an else clause because they confused the compiler
    // by defining an unprotected nested if.
    if (true)
        ATF_CHECK(true);
    else
        ATF_CHECK(true);
}

void
atf_check_equal_inside_if(void)
{
    // Make sure that ATF_CHECK_EQUAL can be used inside an if statement
    // that does not have braces.  Earlier versions of it generated an
    // error if there was an else clause because they confused the
    // compiler by defining an unprotected nested if.
    if (true)
        ATF_CHECK_EQUAL(true, true);
    else
        ATF_CHECK_EQUAL(true, true);
}

void
atf_check_throw_runtime_error(void)
{
    // Check that we can pass std::runtime_error to ATF_CHECK_THROW.
    // Earlier versions generated a warning because the macro's code also
    // attempted to capture this exception, and thus we had a duplicate
    // catch clause.
    ATF_CHECK_THROW((void)0, std::runtime_error);
}
