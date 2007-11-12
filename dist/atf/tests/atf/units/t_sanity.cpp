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

#include <cstring>

#include "atf/macros.hpp"
#include "atf/sanity.hpp"

template< class E >
void
test_getters(void)
{
    E error("a-file.c", 123, "Test message");
    ATF_CHECK(std::strcmp(error.file(), "a-file.c") == 0);
    ATF_CHECK_EQUAL(error.line(), 123);
}

// ------------------------------------------------------------------------
// Tests for the "precondition_error" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(precondition_error_getters);
ATF_TEST_CASE_HEAD(precondition_error_getters)
{
    set("descr", "Tests the precondition_error's getters");
}
ATF_TEST_CASE_BODY(precondition_error_getters)
{
    test_getters< atf::sanity::precondition_error >();
}

ATF_TEST_CASE(precondition_error_macro);
ATF_TEST_CASE_HEAD(precondition_error_macro)
{
    set("descr", "Tests the precondition_error's PRE macro");
}
ATF_TEST_CASE_BODY(precondition_error_macro)
{
    PRE(true);
    ATF_CHECK_THROW(PRE(false), atf::sanity::precondition_error);
}

// ------------------------------------------------------------------------
// Tests for the "postcondition_error" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(postcondition_error_getters);
ATF_TEST_CASE_HEAD(postcondition_error_getters)
{
    set("descr", "Tests the postcondition_error's getters");
}
ATF_TEST_CASE_BODY(postcondition_error_getters)
{
    test_getters< atf::sanity::postcondition_error >();
}

ATF_TEST_CASE(postcondition_error_macro);
ATF_TEST_CASE_HEAD(postcondition_error_macro)
{
    set("descr", "Tests the postcondition_error's POST macro");
}
ATF_TEST_CASE_BODY(postcondition_error_macro)
{
    POST(true);
    ATF_CHECK_THROW(POST(false), atf::sanity::postcondition_error);
}

// ------------------------------------------------------------------------
// Tests for the "invariant_error" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(invariant_error_getters);
ATF_TEST_CASE_HEAD(invariant_error_getters)
{
    set("descr", "Tests the invariant_error's getters");
}
ATF_TEST_CASE_BODY(invariant_error_getters)
{
    test_getters< atf::sanity::invariant_error >();
}

ATF_TEST_CASE(invariant_error_macro);
ATF_TEST_CASE_HEAD(invariant_error_macro)
{
    set("descr", "Tests the invariant_error's INV macro");
}
ATF_TEST_CASE_BODY(invariant_error_macro)
{
    INV(true);
    ATF_CHECK_THROW(INV(false), atf::sanity::invariant_error);
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // We do not check the sanity_error class directly because it is just
    // provided as a base class.  It is indirectly tested through all of
    // its children.

    // Add the tests for the "precondition_error" class.
    ATF_ADD_TEST_CASE(tcs, precondition_error_getters);

    // Add the tests for the "postcondition_error" class.
    ATF_ADD_TEST_CASE(tcs, postcondition_error_getters);

    // Add the tests for the "invariant_error" class.
    ATF_ADD_TEST_CASE(tcs, invariant_error_getters);
    ATF_ADD_TEST_CASE(tcs, invariant_error_macro);
}
