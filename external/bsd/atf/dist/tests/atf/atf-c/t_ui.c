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

#include <stdio.h>
#include <string.h>

#include <atf-c.h>

#include "atf-c/dynstr.h"
#include "atf-c/env.h"
#include "atf-c/ui.h"

#include "h_lib.h"

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

struct test {
    const char *tc;
    const char *tag;
    bool repeat;
    size_t col;
    const char *fmt;
    const char *result;
} tests[] = {
    /*
     * wo_tag
     */

    {
        "wo_tag",
        "",
        false,
        0,
        "12345",
        "12345",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        "12345  ",
        "12345",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        "12345 7890",
        "12345 7890",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        "12345 789012 45",
        "12345 789012 45",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        "12345 789012 456",
        "12345 789012\n456",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        "1234567890123456",
        "1234567890123456",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        " 2345678901234567",
        "\n2345678901234567",
    },

    {
        "wo_tag",
        "",
        false,
        0,
        "12345 789012345 78",
        "12345 789012345\n78",
    },

    /*
     * wo_tag_col
     */

    {
        "wo_tag_col",
        "",
        false,
        10,
        "12345",
        "          12345",
    },

    {
        "wo_tag_col",
        "",
        false,
        10,
        "12345 7890",
        "          12345\n"
        "          7890",
    },

    {
        "wo_tag_col",
        "",
        false,
        10,
        "1 3 5 7 9",
        "          1 3 5\n"
        "          7 9",
    },

    /*
     * w_tag_no_repeat
     */

    {
        "w_tag_no_repeat",
        "1234: ",
        false,
        0,
        "789012345",
        "1234: 789012345",
    },

    {
        "w_tag_no_repeat",
        "1234: ",
        false,
        0,
        "789 1234 56789",
        "1234: 789 1234\n"
        "      56789",
    },

    {
        "w_tag_no_repeat",
        "1234: ",
        false,
        0,
        "789012345",
        "1234: 789012345",
    },

    {
        "w_tag_no_repeat",
        "1234: ",
        false,
        0,
        "789012345 7890",
        "1234: 789012345\n"
        "      7890",
    },

    /*
     * w_tag_repeat
     */

    {
        "w_tag_repeat",
        "1234: ",
        true,
        0,
        "789012345",
        "1234: 789012345",
    },

    {
        "w_tag_repeat",
        "1234: ",
        true,
        0,
        "789 1234 56789",
        "1234: 789 1234\n"
        "1234: 56789",
    },

    {
        "w_tag_repeat",
        "1234: ",
        true,
        0,
        "789012345",
        "1234: 789012345",
    },

    {
        "w_tag_no_repeat",
        "1234: ",
        true,
        0,
        "789012345 7890",
        "1234: 789012345\n"
        "1234: 7890",
    },

    /*
     * w_tag_col
     */

    {
        "w_tag_col",
        "1234:",
        false,
        10,
        "1 3 5",
        "1234:     1 3 5",
    },

    {
        "w_tag_col",
        "1234:",
        false,
        10,
        "1 3 5 7 9",
        "1234:     1 3 5\n"
        "          7 9",
    },

    {
        "w_tag_col",
        "1234:",
        true,
        10,
        "1 3 5 7 9",
        "1234:     1 3 5\n"
        "1234:     7 9",
    },

    /*
     * paragraphs
     */

    {
        "paragraphs",
        "",
        false,
        0,
        "1 3 5\n\n",
        "1 3 5"
    },

    {
        "paragraphs",
        "",
        false,
        0,
        "1 3 5\n2 4 6",
        "1 3 5\n\n2 4 6"
    },

    {
        "paragraphs",
        "",
        false,
        0,
        "1234 6789 123456\n2 4 6",
        "1234 6789\n123456\n\n2 4 6"
    },

    {
        "paragraphs",
        "12: ",
        false,
        0,
        "56789 123456\n2 4 6",
        "12: 56789\n    123456\n\n    2 4 6"
    },

    {
        "paragraphs",
        "12: ",
        true,
        0,
        "56789 123456\n2 4 6",
        "12: 56789\n12: 123456\n12: \n12: 2 4 6"
    },

    {
        "paragraphs",
        "12:",
        false,
        4,
        "56789 123456\n2 4 6",
        "12: 56789\n    123456\n\n    2 4 6"
    },

    {
        "paragraphs",
        "12:",
        true,
        4,
        "56789 123456\n2 4 6",
        "12: 56789\n12: 123456\n12:\n12: 2 4 6"
    },

    /*
     * end
     */

    {
        NULL,
        NULL,
        false,
        0,
        NULL,
        NULL,
    },
};

static
void
run_tests(const char *tc)
{
    struct test *t;

    printf("Running tests for '%s'\n", tc);

    atf_env_set("COLUMNS", "15");

    for (t = &tests[0]; t->tc != NULL; t++) {
        if (strcmp(t->tc, tc) == 0) {
            atf_dynstr_t str;
            atf_error_t err;

            printf("\n");
            printf("Testing with tag '%s', %s, col '%zd'\n",
                   t->tag, t->repeat ? "repeat" : "no repeat", t->col);
            printf("Input: >>>%s<<<\n", t->fmt);
            printf("Expected output: >>>%s<<<\n", t->result);

            err = atf_dynstr_init(&str);
            ATF_REQUIRE(!atf_is_error(err));
            atf_ui_format_fmt(&str, t->tag, t->repeat, t->col, t->fmt);
            printf("Output         : >>>%s<<<\n", atf_dynstr_cstring(&str));
            ATF_REQUIRE(atf_equal_dynstr_cstring(&str, t->result));
            atf_dynstr_fini(&str);
        }
    }
}

ATF_TC(wo_tag);
ATF_TC_HEAD(wo_tag, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting without tags");
}
ATF_TC_BODY(wo_tag, tc)
{
    run_tests("wo_tag");
}

ATF_TC(wo_tag_col);
ATF_TC_HEAD(wo_tag_col, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting without tags and with "
                      "a non-zero starting column");
}
ATF_TC_BODY(wo_tag_col, tc)
{
    run_tests("wo_tag_col");
}

ATF_TC(w_tag_no_repeat);
ATF_TC_HEAD(w_tag_no_repeat, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting with a tag");
}
ATF_TC_BODY(w_tag_no_repeat, tc)
{
    run_tests("w_tag_no_repeat");
}

ATF_TC(w_tag_repeat);
ATF_TC_HEAD(w_tag_repeat, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting with a tag and "
                      "repeating it on each line");
}
ATF_TC_BODY(w_tag_repeat, tc)
{
    run_tests("w_tag_repeat");
}

ATF_TC(w_tag_col);
ATF_TC_HEAD(w_tag_col, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting with a tag and "
                      "starting at a column greater than its length");
}
ATF_TC_BODY(w_tag_col, tc)
{
    run_tests("w_tag_col");
}

ATF_TC(paragraphs);
ATF_TC_HEAD(paragraphs, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting a string that "
                      "contains multiple paragraphs");
}
ATF_TC_BODY(paragraphs, tc)
{
    run_tests("paragraphs");
}

ATF_TC(format);
ATF_TC_HEAD(format, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks formatting with a variable list "
                      "of arguments");
}
ATF_TC_BODY(format, tc)
{
    atf_dynstr_t str;
    atf_error_t err;

    err = atf_dynstr_init(&str);
    ATF_REQUIRE(!atf_is_error(err));
    atf_ui_format_fmt(&str, "", false, 0, "Test %s %d", "string", 1);
    ATF_REQUIRE(atf_equal_dynstr_cstring(&str, "Test string 1"));
    atf_dynstr_fini(&str);
}

/* ---------------------------------------------------------------------
 * Tests cases for the header file.
 * --------------------------------------------------------------------- */

HEADER_TC(include, "atf-c/ui.h", "d_include_ui_h.c");

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, wo_tag);
    ATF_TP_ADD_TC(tp, wo_tag_col);
    ATF_TP_ADD_TC(tp, w_tag_no_repeat);
    ATF_TP_ADD_TC(tp, w_tag_repeat);
    ATF_TP_ADD_TC(tp, w_tag_col);
    ATF_TP_ADD_TC(tp, paragraphs);
    ATF_TP_ADD_TC(tp, format);

    /* Add the test cases for the header file. */
    ATF_TP_ADD_TC(tp, include);

    return atf_no_error();
}
