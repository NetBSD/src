/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(TESTS_ATF_ATF_C_H_LIB_H)
#   error "Cannot include h_lib.h more than once."
#else
#   define TESTS_ATF_ATF_C_H_LIB_H
#endif

#include <stdbool.h>

#include "atf-c/error_fwd.h"

struct atf_dynstr;
struct atf_fs_path;

#define CE(stm) ATF_CHECK(!atf_is_error(stm))
#define RE(stm) ATF_REQUIRE(!atf_is_error(stm))

#define HEADER_TC(name, hdrname, sfile) \
    BUILD_TC(name, sfile, \
             "Tests that the " hdrname " file can be included on " \
             "its own, without any prerequisites", \
             "Build of " sfile " failed; " hdrname " is not self-contained");

#define BUILD_TC(name, sfile, descr, failmsg) \
    ATF_TC(name); \
    ATF_TC_HEAD(name, tc) \
    { \
        atf_tc_set_md_var(tc, "descr", descr); \
    } \
    ATF_TC_BODY(name, tc) \
    { \
        build_check_c_o(tc, sfile, failmsg); \
    }

void build_check_c_o(const atf_tc_t *, const char *, const char *);
void get_h_processes_path(const atf_tc_t *, struct atf_fs_path *);
bool grep_string(const struct atf_dynstr *, const char *);
bool grep_file(const char *, const char *, ...);
