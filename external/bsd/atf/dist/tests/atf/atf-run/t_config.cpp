//
// Automated Testing Framework (atf)
//
// Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include "atf-run/config.hpp"

#include "atf-c++/macros.hpp"

namespace impl = atf::atf_run;

using atf::tests::vars_map;

// -------------------------------------------------------------------------
// Tests.
// -------------------------------------------------------------------------

ATF_TEST_CASE(merge_configs_both_empty);
ATF_TEST_CASE_HEAD(merge_configs_both_empty) {}
ATF_TEST_CASE_BODY(merge_configs_both_empty) {
    vars_map lower, upper;

    ATF_CHECK(impl::merge_configs(lower, upper).empty());
}

ATF_TEST_CASE(merge_configs_lower_empty);
ATF_TEST_CASE_HEAD(merge_configs_lower_empty) {}
ATF_TEST_CASE_BODY(merge_configs_lower_empty) {
    vars_map lower, upper;
    upper["var"] = "value";

    vars_map merged = impl::merge_configs(lower, upper);
    ATF_CHECK_EQUAL("value", merged["var"]);
}

ATF_TEST_CASE(merge_configs_upper_empty);
ATF_TEST_CASE_HEAD(merge_configs_upper_empty) {}
ATF_TEST_CASE_BODY(merge_configs_upper_empty) {
    vars_map lower, upper;
    lower["var"] = "value";

    vars_map merged = impl::merge_configs(lower, upper);
    ATF_CHECK_EQUAL("value", merged["var"]);
}

ATF_TEST_CASE(merge_configs_mixed);
ATF_TEST_CASE_HEAD(merge_configs_mixed) {}
ATF_TEST_CASE_BODY(merge_configs_mixed) {
    vars_map lower, upper;
    lower["var1"] = "value1";
    lower["var2"] = "value2-l";
    upper["var2"] = "value2-u";
    upper["var3"] = "value3";

    vars_map merged = impl::merge_configs(lower, upper);
    ATF_CHECK_EQUAL("value1", merged["var1"]);
    ATF_CHECK_EQUAL("value2-u", merged["var2"]);
    ATF_CHECK_EQUAL("value3", merged["var3"]);
}

ATF_TEST_CASE(read_config_files_none);
ATF_TEST_CASE_HEAD(read_config_files_none) {}
ATF_TEST_CASE_BODY(read_config_files_none) {
    ATF_CHECK(vars_map() == impl::read_config_files("test-suite"));
}

// -------------------------------------------------------------------------
// Main.
// -------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, merge_configs_both_empty);
    ATF_ADD_TEST_CASE(tcs, merge_configs_lower_empty);
    ATF_ADD_TEST_CASE(tcs, merge_configs_upper_empty);
    ATF_ADD_TEST_CASE(tcs, merge_configs_mixed);

    ATF_ADD_TEST_CASE(tcs, read_config_files_none);
}
