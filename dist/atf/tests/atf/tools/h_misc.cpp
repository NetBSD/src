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

#include <iostream>

#include "atf/env.hpp"
#include "atf/macros.hpp"

// ------------------------------------------------------------------------
// Helper tests for "t_atf_run".
// ------------------------------------------------------------------------

ATF_TEST_CASE(atf_run_config);
ATF_TEST_CASE_HEAD(atf_run_config)
{
    set("descr", "Helper test case for the t_atf_run test program");
}
ATF_TEST_CASE_BODY(atf_run_config)
{
    std::cout << "1st: " << config().get("1st") << std::endl;
    std::cout << "2nd: " << config().get("2nd") << std::endl;
    std::cout << "3rd: " << config().get("3rd") << std::endl;
    std::cout << "4th: " << config().get("4th") << std::endl;
}

ATF_TEST_CASE(atf_run_fds);
ATF_TEST_CASE_HEAD(atf_run_fds)
{
    set("descr", "Helper test case for the t_atf_run test program");
}
ATF_TEST_CASE_BODY(atf_run_fds)
{
    std::cout << "msg1 to stdout" << std::endl;
    std::cout << "msg2 to stdout" << std::endl;
    std::cerr << "msg1 to stderr" << std::endl;
    std::cerr << "msg2 to stderr" << std::endl;
}

ATF_TEST_CASE(atf_run_testvar);
ATF_TEST_CASE_HEAD(atf_run_testvar)
{
    set("descr", "Helper test case for the t_atf_run test program");
}
ATF_TEST_CASE_BODY(atf_run_testvar)
{
    if (!config().has("testvar"))
        ATF_FAIL("testvar variable not defined");
    std::cout << "testvar: " << config().get("testvar") << std::endl;
}

// ------------------------------------------------------------------------
// Helper tests for "t_atf_report".
// ------------------------------------------------------------------------

ATF_TEST_CASE(atf_report_diff);
ATF_TEST_CASE_HEAD(atf_report_diff)
{
    set("descr", "Helper test case for the t_atf_report test program");
}
ATF_TEST_CASE_BODY(atf_report_diff)
{
    std::cout << "--- a	2007-11-04 14:00:41.000000000 +0100" << std::endl;
    std::cout << "+++ b	2007-11-04 14:00:48.000000000 +0100" << std::endl;
    std::cout << "@@ -1,7 +1,7 @@" << std::endl;
    std::cout << " This test is meant to simulate a diff." << std::endl;
    std::cout << " Blank space at beginning of context lines must be "
                 "preserved." << std::endl;
    std::cout << " " << std::endl;
    std::cout << "-First original line." << std::endl;
    std::cout << "-Second original line." << std::endl;
    std::cout << "+First modified line." << std::endl;
    std::cout << "+Second modified line." << std::endl;
    std::cout << " " << std::endl;
    std::cout << " EOF" << std::endl;
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    std::string which = atf::env::get("TESTCASE");

    // Add helper tests for t_atf_run.
    if (which == "atf_run_config")
        ATF_ADD_TEST_CASE(tcs, atf_run_config);
    if (which == "atf_run_fds")
        ATF_ADD_TEST_CASE(tcs, atf_run_fds);
    if (which == "atf_run_testvar")
        ATF_ADD_TEST_CASE(tcs, atf_run_testvar);

    // Add helper tests for t_atf_report.
    if (which == "atf_report_diff")
        ATF_ADD_TEST_CASE(tcs, atf_report_diff);
}
