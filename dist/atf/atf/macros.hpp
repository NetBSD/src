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

#if !defined(_ATF_MACROS_HPP_)
#define _ATF_MACROS_HPP_

#include <sstream>
#include <stdexcept>
#include <vector>

#include <atf/tests.hpp>

#define ATF_TEST_CASE(name) \
    class atf_tc_ ## name : public atf::tests::tc { \
        void head(void); \
        void body(void) const; \
    public: \
        atf_tc_ ## name(void) : atf::tests::tc(#name) {} \
    }; \
    static atf_tc_ ## name atf_tc_ ## name;

#define ATF_TEST_CASE_WITH_CLEANUP(name) \
    class atf_tc_ ## name : public atf::tests::tc { \
        void head(void); \
        void body(void) const; \
        void cleanup(void) const; \
    public: \
        atf_tc_ ## name(void) : atf::tests::tc(#name) {} \
    }; \
    static atf_tc_ ## name atf_tc_ ## name;

#define ATF_TEST_CASE_HEAD(name) \
    void \
    atf_tc_ ## name::head(void)

#define ATF_TEST_CASE_BODY(name) \
    void \
    atf_tc_ ## name::body(void) \
        const

#define ATF_TEST_CASE_CLEANUP(name) \
    void \
    atf_tc_ ## name::cleanup(void) \
        const

#define ATF_FAIL(reason) \
    throw atf::tests::tcr::failed(reason)

#define ATF_SKIP(reason) \
    throw atf::tests::tcr::skipped(reason)

#define ATF_PASS() \
    throw atf::tests::tcr::passed()

#define ATF_CHECK(x) \
    do { \
        if (!(x)) { \
            std::ostringstream __atf_ss; \
            __atf_ss << "Line " << __LINE__ << ": " << #x << " not met"; \
            throw atf::tests::tcr::failed(__atf_ss.str()); \
        } \
    } while (false)

#define ATF_CHECK_EQUAL(x, y) \
    do { \
        if ((x) != (y)) { \
            std::ostringstream __atf_ss; \
            __atf_ss << "Line " << __LINE__ << ": " << #x << " != " << #y \
                     << " (" << (x) << " != " << (y) << ")"; \
            throw atf::tests::tcr::failed(__atf_ss.str()); \
        } \
    } while (false)

#define ATF_CHECK_THROW(x, e) \
    try { \
        x; \
        std::ostringstream __atf_ss; \
        __atf_ss << "Line " << __LINE__ << ": " #x " did not throw " \
                    #e " as expected"; \
        throw atf::tests::tcr::failed(__atf_ss.str()); \
    } catch (const e& __atf_eo) { \
    } catch (const atf::tests::tcr&) { \
        throw; \
    } catch (const std::exception& __atf_e) { \
        std::ostringstream __atf_ss; \
        __atf_ss << "Line " << __LINE__ << ": " #x " threw an " \
                    "unexpected error (not " #e "): " << __atf_e.what(); \
        throw atf::tests::tcr::failed(__atf_ss.str()); \
    } catch (...) { \
        std::ostringstream __atf_ss; \
        __atf_ss << "Line " << __LINE__ << ": " #x " threw an " \
                    "unexpected error (not " #e ")"; \
        throw atf::tests::tcr::failed(__atf_ss.str()); \
    }

#define ATF_INIT_TEST_CASES(tcs) \
    namespace atf { \
        namespace tests { \
            int run_tp(int, char* const*, \
                       const std::vector< atf::tests::tc * >&); \
        } \
    } \
    \
    int \
    main(int argc, char* const* argv) \
    { \
        void __atf_init_tcs(std::vector< atf::tests::tc * >&); \
        std::vector< atf::tests::tc * > tcs; \
        __atf_init_tcs(tcs); \
        return atf::tests::run_tp(argc, argv, tcs); \
    } \
    \
    void \
    __atf_init_tcs(std::vector< atf::tests::tc * >& tcs)

#define ATF_ADD_TEST_CASE(tcs, tc) \
    (tcs).push_back(&(atf_tc_ ## tc));

#endif // !defined(_ATF_MACROS_HPP_)
