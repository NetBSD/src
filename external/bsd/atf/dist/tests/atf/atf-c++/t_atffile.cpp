//
// Automated Testing Framework (atf)
//
// Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
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

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
}

#include <algorithm>
#include <fstream>
#include <memory>

#include "atf-c++/atffile.hpp"
#include "atf-c++/macros.hpp"

#include "h_lib.hpp"

namespace impl = atf::atffile;

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

namespace {

static
std::auto_ptr< std::ofstream >
new_atffile(void)
{
    std::auto_ptr< std::ofstream > os(new std::ofstream("Atffile"));
    ATF_CHECK(*os);

    (*os) << "Content-Type: application/X-atf-atffile; version=\"1\"\n\n";
    return os;
}

static
void
touch_exec(const char* name)
{
    std::ofstream os(name);
    ATF_CHECK(os);
    os.close();
    ATF_CHECK(::chmod(name, S_IRWXU) != -1);
}

static inline
bool
is_in(const std::string& value, const std::vector< std::string >& v)
{
    return std::find(v.begin(), v.end(), value) != v.end();
}

} // anonymous namespace

// ------------------------------------------------------------------------
// Tests cases for the "atffile" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(atffile_getters);
ATF_TEST_CASE_HEAD(atffile_getters) {}
ATF_TEST_CASE_BODY(atffile_getters) {
    atf::tests::vars_map config_vars;
    config_vars["config-var-1"] = "value 1";

    std::vector< std::string > test_program_names;
    test_program_names.push_back("test-program-1");

    atf::tests::vars_map properties;
    properties["test-suite"] = "a test name";

    const impl::atffile atffile(config_vars, test_program_names, properties);
    ATF_CHECK(config_vars == atffile.conf());
    ATF_CHECK(test_program_names == atffile.tps());
    ATF_CHECK(properties == atffile.props());
}

// ------------------------------------------------------------------------
// Tests cases for the free functions.
// ------------------------------------------------------------------------

ATF_TEST_CASE(read_ok_simple);
ATF_TEST_CASE_HEAD(read_ok_simple) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_ok_simple) {
    std::auto_ptr< std::ofstream > os = new_atffile();
    (*os) << "prop: test-suite = foo\n";
    (*os) << "tp: tp-1\n";
    (*os) << "conf: var1 = value1\n";
    (*os) << "tp: tp-2\n";
    (*os) << "tp: tp-3\n";
    (*os) << "prop: prop1 = propvalue1\n";
    (*os) << "conf: var2 = value2\n";
    (*os).close();

    touch_exec("tp-1");
    touch_exec("tp-2");
    touch_exec("tp-3");

    const impl::atffile atffile = impl::read(atf::fs::path("Atffile"));
    ATF_CHECK_EQUAL(2, atffile.conf().size());
    ATF_CHECK_EQUAL("value1", atffile.conf().find("var1")->second);
    ATF_CHECK_EQUAL("value2", atffile.conf().find("var2")->second);
    ATF_CHECK_EQUAL(3, atffile.tps().size());
    ATF_CHECK(is_in("tp-1", atffile.tps()));
    ATF_CHECK(is_in("tp-2", atffile.tps()));
    ATF_CHECK(is_in("tp-3", atffile.tps()));
    ATF_CHECK_EQUAL(2, atffile.props().size());
    ATF_CHECK_EQUAL("foo", atffile.props().find("test-suite")->second);
    ATF_CHECK_EQUAL("propvalue1", atffile.props().find("prop1")->second);
}

ATF_TEST_CASE(read_ok_some_globs);
ATF_TEST_CASE_HEAD(read_ok_some_globs) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_ok_some_globs) {
    std::auto_ptr< std::ofstream > os = new_atffile();
    (*os) << "prop: test-suite = foo\n";
    (*os) << "tp: foo\n";
    (*os) << "tp-glob: *K*\n";
    (*os) << "tp: bar\n";
    (*os) << "tp-glob: t_*\n";
    (*os).close();

    touch_exec("foo");
    touch_exec("bar");
    touch_exec("aK");
    touch_exec("KKKKK");
    touch_exec("t_hello");
    touch_exec("zzzt_hello");

    const impl::atffile atffile = impl::read(atf::fs::path("Atffile"));
    ATF_CHECK_EQUAL(5, atffile.tps().size());
    ATF_CHECK(is_in("foo", atffile.tps()));
    ATF_CHECK(is_in("bar", atffile.tps()));
    ATF_CHECK(is_in("aK", atffile.tps()));
    ATF_CHECK(is_in("KKKKK", atffile.tps()));
    ATF_CHECK(is_in("t_hello", atffile.tps()));
}

ATF_TEST_CASE(read_missing_test_suite);
ATF_TEST_CASE_HEAD(read_missing_test_suite) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_missing_test_suite) {
    std::auto_ptr< std::ofstream > os = new_atffile();
    (*os).close();

    try {
        (void)impl::read(atf::fs::path("Atffile"));
        ATF_FAIL("Missing property 'test-suite' did not raise an error");
    } catch (const atf::not_found_error< std::string >& e) {
        ATF_CHECK_EQUAL("test-suite", e.get_value());
    }
}

ATF_TEST_CASE(read_missing_test_program);
ATF_TEST_CASE_HEAD(read_missing_test_program) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_missing_test_program) {
    std::auto_ptr< std::ofstream > os = new_atffile();
    (*os) << "tp: foo\n";
    (*os) << "tp: bar\n";
    (*os) << "tp: baz\n";
    (*os).close();

    touch_exec("foo");
    touch_exec("baz");

    try {
        (void)impl::read(atf::fs::path("Atffile"));
        ATF_FAIL("Missing file 'bar' did not raise an error");
    } catch (const atf::not_found_error< atf::fs::path >& e) {
        ATF_CHECK_EQUAL("bar", e.get_value().str());
    }
}

// ------------------------------------------------------------------------
// Tests cases for the header file.
// ------------------------------------------------------------------------

HEADER_TC(include, "atffile.hpp", "d_include_atffile_hpp.cpp");

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add the test cases for the atffile class.
    ATF_ADD_TEST_CASE(tcs, atffile_getters);

    // Add the test cases for the free functions.
    ATF_ADD_TEST_CASE(tcs, read_ok_simple);
    ATF_ADD_TEST_CASE(tcs, read_ok_some_globs);
    ATF_ADD_TEST_CASE(tcs, read_missing_test_suite);
    ATF_ADD_TEST_CASE(tcs, read_missing_test_program);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
}
