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

#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/fs.h"
#include "atf-c/io.h"
#include "atf-c/process.h"
#include "atf-c/tcr.h"
#include "atf-c/text.h"

#include "h_lib.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
void
create_ctl_file(const atf_tc_t *tc, const char *name)
{
    atf_fs_path_t p;

    RE(atf_fs_path_init_fmt(&p, "%s/%s",
                            atf_tc_get_config_var(tc, "ctldir"), name));
    ATF_REQUIRE(open(atf_fs_path_cstring(&p),
                   O_CREAT | O_WRONLY | O_TRUNC, 0644) != -1);
    atf_fs_path_fini(&p);
}

static
bool
exists(const char *p)
{
    bool b;
    atf_fs_path_t pp;

    RE(atf_fs_path_init_fmt(&pp, "%s", p));
    RE(atf_fs_exists(&pp, &b));
    atf_fs_path_fini(&pp);

    return b;
}

static
void
init_config(atf_map_t *config)
{
    atf_fs_path_t cwd;

    RE(atf_map_init(config));

    RE(atf_fs_getcwd(&cwd));
    RE(atf_map_insert(config, "ctldir",
                      strdup(atf_fs_path_cstring(&cwd)), true));
    atf_fs_path_fini(&cwd);
}

static
void
run_inherit(const atf_tc_t *tc, atf_tcr_t *tcr)
{
    atf_fs_path_t cwd;

    RE(atf_fs_getcwd(&cwd));
    RE(atf_tc_run(tc, tcr, STDOUT_FILENO, STDERR_FILENO, &cwd));
    atf_fs_path_fini(&cwd);
}

static
void
run_capture(const atf_tc_t *tc, const char *errname, atf_tcr_t *tcr)
{
    int errfile;
    atf_fs_path_t cwd;

    ATF_REQUIRE((errfile = open(errname, O_WRONLY | O_CREAT | O_TRUNC,
                                0644)) != -1);

    RE(atf_fs_getcwd(&cwd));
    RE(atf_tc_run(tc, tcr, STDOUT_FILENO, errfile, &cwd));
    atf_fs_path_fini(&cwd);

    close(errfile);
}

static
bool
grep_reason(const atf_tcr_t *tcr, const char *regex, ...)
{
    va_list ap;
    atf_dynstr_t formatted;
    bool found;

    va_start(ap, regex);
    RE(atf_dynstr_init_ap(&formatted, regex, ap));
    va_end(ap);

    found = grep_string(atf_tcr_get_reason(tcr),
                        atf_dynstr_cstring(&formatted));

    atf_dynstr_fini(&formatted);

    return found;
}

/* ---------------------------------------------------------------------
 * Helper test cases.
 * --------------------------------------------------------------------- */

#define H_DEF(id, macro) \
    ATF_TC_HEAD(h_ ## id, tc) \
    { \
        atf_tc_set_md_var(tc, "descr", "Helper test case"); \
    } \
    ATF_TC_BODY(h_ ## id, tc) \
    { \
        create_ctl_file(tc, "before"); \
        macro; \
        create_ctl_file(tc, "after"); \
    }

#define H_CHECK_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_check_ ## id)
#define H_CHECK_BODY_NAME(id) ATF_TC_BODY_NAME(h_check_ ## id)
#define H_CHECK(id, condition) \
    H_DEF(check_ ## id, ATF_CHECK(condition))

#define H_CHECK_MSG_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_check_msg_ ## id)
#define H_CHECK_MSG_BODY_NAME(id) ATF_TC_BODY_NAME(h_check_msg_ ## id)
#define H_CHECK_MSG(id, condition, msg) \
    H_DEF(check_msg_ ## id, ATF_CHECK_MSG(condition, msg))

#define H_CHECK_EQ_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_check_eq_ ## id)
#define H_CHECK_EQ_BODY_NAME(id) ATF_TC_BODY_NAME(h_check_eq_ ## id)
#define H_CHECK_EQ(id, v1, v2) \
    H_DEF(check_eq_ ## id, ATF_CHECK_EQ(v1, v2))

#define H_CHECK_STREQ_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_check_streq_ ## id)
#define H_CHECK_STREQ_BODY_NAME(id) ATF_TC_BODY_NAME(h_check_streq_ ## id)
#define H_CHECK_STREQ(id, v1, v2) \
    H_DEF(check_streq_ ## id, ATF_CHECK_STREQ(v1, v2))

#define H_CHECK_EQ_MSG_HEAD_NAME(id) \
    ATF_TC_HEAD_NAME(h_check_eq_msg_ ## id)
#define H_CHECK_EQ_MSG_BODY_NAME(id) \
    ATF_TC_BODY_NAME(h_check_eq_msg_ ## id)
#define H_CHECK_EQ_MSG(id, v1, v2, msg) \
    H_DEF(check_eq_msg_ ## id, ATF_CHECK_EQ_MSG(v1, v2, msg))

#define H_CHECK_STREQ_MSG_HEAD_NAME(id) \
    ATF_TC_HEAD_NAME(h_check_streq_msg_ ## id)
#define H_CHECK_STREQ_MSG_BODY_NAME(id) \
    ATF_TC_BODY_NAME(h_check_streq_msg_ ## id)
#define H_CHECK_STREQ_MSG(id, v1, v2, msg) \
    H_DEF(check_streq_msg_ ## id, ATF_CHECK_STREQ_MSG(v1, v2, msg))

#define H_REQUIRE_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_require_ ## id)
#define H_REQUIRE_BODY_NAME(id) ATF_TC_BODY_NAME(h_require_ ## id)
#define H_REQUIRE(id, condition) \
    H_DEF(require_ ## id, ATF_REQUIRE(condition))

#define H_REQUIRE_MSG_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_require_msg_ ## id)
#define H_REQUIRE_MSG_BODY_NAME(id) ATF_TC_BODY_NAME(h_require_msg_ ## id)
#define H_REQUIRE_MSG(id, condition, msg) \
    H_DEF(require_msg_ ## id, ATF_REQUIRE_MSG(condition, msg))

#define H_REQUIRE_EQ_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_require_eq_ ## id)
#define H_REQUIRE_EQ_BODY_NAME(id) ATF_TC_BODY_NAME(h_require_eq_ ## id)
#define H_REQUIRE_EQ(id, v1, v2) \
    H_DEF(require_eq_ ## id, ATF_REQUIRE_EQ(v1, v2))

#define H_REQUIRE_STREQ_HEAD_NAME(id) ATF_TC_HEAD_NAME(h_require_streq_ ## id)
#define H_REQUIRE_STREQ_BODY_NAME(id) ATF_TC_BODY_NAME(h_require_streq_ ## id)
#define H_REQUIRE_STREQ(id, v1, v2) \
    H_DEF(require_streq_ ## id, ATF_REQUIRE_STREQ(v1, v2))

#define H_REQUIRE_EQ_MSG_HEAD_NAME(id) \
    ATF_TC_HEAD_NAME(h_require_eq_msg_ ## id)
#define H_REQUIRE_EQ_MSG_BODY_NAME(id) \
    ATF_TC_BODY_NAME(h_require_eq_msg_ ## id)
#define H_REQUIRE_EQ_MSG(id, v1, v2, msg) \
    H_DEF(require_eq_msg_ ## id, ATF_REQUIRE_EQ_MSG(v1, v2, msg))

#define H_REQUIRE_STREQ_MSG_HEAD_NAME(id) \
    ATF_TC_HEAD_NAME(h_require_streq_msg_ ## id)
#define H_REQUIRE_STREQ_MSG_BODY_NAME(id) \
    ATF_TC_BODY_NAME(h_require_streq_msg_ ## id)
#define H_REQUIRE_STREQ_MSG(id, v1, v2, msg) \
    H_DEF(require_streq_msg_ ## id, ATF_REQUIRE_STREQ_MSG(v1, v2, msg))

/* ---------------------------------------------------------------------
 * Test cases for the ATF_CHECK and ATF_CHECK_MSG macros.
 * --------------------------------------------------------------------- */

H_CHECK(0, 0);
H_CHECK(1, 1);
H_CHECK_MSG(0, 0, "expected a false value");
H_CHECK_MSG(1, 1, "expected a true value");

ATF_TC(check);
ATF_TC_HEAD(check, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_CHECK and "
                      "ATF_CHECK_MSG macros");
}
ATF_TC_BODY(check, tc)
{
    struct test {
        void (*head)(atf_tc_t *);
        void (*body)(const atf_tc_t *);
        bool value;
        const char *msg;
        bool ok;
    } *t, tests[] = {
        { H_CHECK_HEAD_NAME(0), H_CHECK_BODY_NAME(0), 0,
          "0 not met", false },
        { H_CHECK_HEAD_NAME(1), H_CHECK_BODY_NAME(1), 1,
          "1 not met", true },
        { H_CHECK_MSG_HEAD_NAME(0), H_CHECK_MSG_BODY_NAME(0), 0,
          "expected a false value", false },
        { H_CHECK_MSG_HEAD_NAME(1), H_CHECK_MSG_BODY_NAME(1), 1,
          "expected a true value", true },
        { NULL, NULL, false, NULL, false }
    };

    for (t = &tests[0]; t->head != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);

        printf("Checking with a %d value\n", t->value);

        RE(atf_tc_init(&tcaux, "h_check", t->head, t->body, NULL, &config));
        run_capture(&tcaux, "error", &tcr);
        atf_tc_fini(&tcaux);

        ATF_REQUIRE(exists("before"));
        ATF_REQUIRE(exists("after"));

        if (t->ok) {
            ATF_REQUIRE(atf_tcr_get_state(&tcr) == atf_tcr_passed_state);
        } else {
            ATF_REQUIRE(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
            ATF_REQUIRE(grep_file("error", "t_macros.c:[0-9]+: "
                                  "Check failed: %s$", t->msg));
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);

        ATF_REQUIRE(unlink("before") != -1);
        ATF_REQUIRE(unlink("after") != -1);
    }
}

/* ---------------------------------------------------------------------
 * Test cases for the ATF_CHECK_*EQ_ macros.
 * --------------------------------------------------------------------- */

struct check_eq_test {
    void (*head)(atf_tc_t *);
    void (*body)(const atf_tc_t *);
    const char *v1;
    const char *v2;
    const char *msg;
    bool ok;
};

static
void
do_check_eq_tests(const struct check_eq_test *tests)
{
    const struct check_eq_test *t;

    for (t = &tests[0]; t->head != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);

        printf("Checking with %s, %s and expecting %s\n", t->v1, t->v2,
               t->ok ? "true" : "false");

        RE(atf_tc_init(&tcaux, "h_check", t->head, t->body, NULL, &config));
        run_capture(&tcaux, "error", &tcr);
        atf_tc_fini(&tcaux);

        ATF_CHECK(exists("before"));
        ATF_CHECK(exists("after"));

        if (t->ok) {
            ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_passed_state);
        } else {
            ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
            ATF_CHECK(grep_file("error", "t_macros.c:[0-9]+: "
                                "Check failed: %s$", t->msg));
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);

        ATF_CHECK(unlink("before") != -1);
        ATF_CHECK(unlink("after") != -1);
    }
}

H_CHECK_EQ(1_1, 1, 1);
H_CHECK_EQ(1_2, 1, 2);
H_CHECK_EQ(2_1, 2, 1);
H_CHECK_EQ(2_2, 2, 2);
H_CHECK_EQ_MSG(1_1, 1, 1, "1 does not match 1");
H_CHECK_EQ_MSG(1_2, 1, 2, "1 does not match 2");
H_CHECK_EQ_MSG(2_1, 2, 1, "2 does not match 1");
H_CHECK_EQ_MSG(2_2, 2, 2, "2 does not match 2");

ATF_TC(check_eq);
ATF_TC_HEAD(check_eq, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_CHECK_EQ and "
                      "ATF_CHECK_EQ_MSG macros");
}
ATF_TC_BODY(check_eq, tc)
{
    struct check_eq_test tests[] = {
        { H_CHECK_EQ_HEAD_NAME(1_1), H_CHECK_EQ_BODY_NAME(1_1),
          "1", "1", "1 != 1", true },
        { H_CHECK_EQ_HEAD_NAME(1_2), H_CHECK_EQ_BODY_NAME(1_2),
          "1", "2", "1 != 2", false },
        { H_CHECK_EQ_HEAD_NAME(2_1), H_CHECK_EQ_BODY_NAME(2_1),
          "2", "1", "2 != 1", false },
        { H_CHECK_EQ_HEAD_NAME(2_2), H_CHECK_EQ_BODY_NAME(2_2),
          "2", "2", "2 != 2", true },
        { H_CHECK_EQ_MSG_HEAD_NAME(1_1), H_CHECK_EQ_MSG_BODY_NAME(1_1),
          "1", "1", "1 != 1: 1 does not match 1", true },
        { H_CHECK_EQ_MSG_HEAD_NAME(1_2), H_CHECK_EQ_MSG_BODY_NAME(1_2),
          "1", "2", "1 != 2: 1 does not match 2", false },
        { H_CHECK_EQ_MSG_HEAD_NAME(2_1), H_CHECK_EQ_MSG_BODY_NAME(2_1),
          "2", "1", "2 != 1: 2 does not match 1", false },
        { H_CHECK_EQ_MSG_HEAD_NAME(2_2), H_CHECK_EQ_MSG_BODY_NAME(2_2),
          "2", "2", "2 != 2: 2 does not match 2", true },
        { NULL, NULL, 0, 0, "", false }
    };
    do_check_eq_tests(tests);
}

H_CHECK_STREQ(1_1, "1", "1");
H_CHECK_STREQ(1_2, "1", "2");
H_CHECK_STREQ(2_1, "2", "1");
H_CHECK_STREQ(2_2, "2", "2");
H_CHECK_STREQ_MSG(1_1, "1", "1", "1 does not match 1");
H_CHECK_STREQ_MSG(1_2, "1", "2", "1 does not match 2");
H_CHECK_STREQ_MSG(2_1, "2", "1", "2 does not match 1");
H_CHECK_STREQ_MSG(2_2, "2", "2", "2 does not match 2");
#define CHECK_STREQ_VAR1 "5"
#define CHECK_STREQ_VAR2 "9"
const const char *check_streq_var1 = CHECK_STREQ_VAR1;
const const char *check_streq_var2 = CHECK_STREQ_VAR2;
H_CHECK_STREQ(vars, check_streq_var1, check_streq_var2);

ATF_TC(check_streq);
ATF_TC_HEAD(check_streq, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_CHECK_STREQ and "
                      "ATF_CHECK_STREQ_MSG macros");
}
ATF_TC_BODY(check_streq, tc)
{
    struct check_eq_test tests[] = {
        { H_CHECK_STREQ_HEAD_NAME(1_1), H_CHECK_STREQ_BODY_NAME(1_1),
          "1", "1", "\"1\" != \"1\" \\(1 != 1\\)", true },
        { H_CHECK_STREQ_HEAD_NAME(1_2), H_CHECK_STREQ_BODY_NAME(1_2),
          "1", "2", "\"1\" != \"2\" \\(1 != 2\\)", false },
        { H_CHECK_STREQ_HEAD_NAME(2_1), H_CHECK_STREQ_BODY_NAME(2_1),
          "2", "1", "\"2\" != \"1\" \\(2 != 1\\)", false },
        { H_CHECK_STREQ_HEAD_NAME(2_2), H_CHECK_STREQ_BODY_NAME(2_2),
          "2", "2", "\"2\" != \"2\" \\(2 != 2\\)", true },
        { H_CHECK_STREQ_MSG_HEAD_NAME(1_1),
          H_CHECK_STREQ_MSG_BODY_NAME(1_1),
          "1", "1", "\"1\" != \"1\" \\(1 != 1\\): 1 does not match 1", true },
        { H_CHECK_STREQ_MSG_HEAD_NAME(1_2),
          H_CHECK_STREQ_MSG_BODY_NAME(1_2),
          "1", "2", "\"1\" != \"2\" \\(1 != 2\\): 1 does not match 2", false },
        { H_CHECK_STREQ_MSG_HEAD_NAME(2_1),
          H_CHECK_STREQ_MSG_BODY_NAME(2_1),
          "2", "1", "\"2\" != \"1\" \\(2 != 1\\): 2 does not match 1", false },
        { H_CHECK_STREQ_MSG_HEAD_NAME(2_2),
          H_CHECK_STREQ_MSG_BODY_NAME(2_2),
          "2", "2", "\"2\" != \"2\" \\(2 != 2\\): 2 does not match 2", true },
        { H_CHECK_STREQ_HEAD_NAME(vars), H_CHECK_STREQ_BODY_NAME(vars),
          check_streq_var1, check_streq_var2,
          "check_streq_var1 != check_streq_var2 \\("
          CHECK_STREQ_VAR1 " != " CHECK_STREQ_VAR2 "\\)", false },
        { NULL, NULL, 0, 0, "", false }
    };
    do_check_eq_tests(tests);
}

/* ---------------------------------------------------------------------
 * Test cases for the ATF_REQUIRE and ATF_REQUIRE_MSG macros.
 * --------------------------------------------------------------------- */

H_REQUIRE(0, 0);
H_REQUIRE(1, 1);
H_REQUIRE_MSG(0, 0, "expected a false value");
H_REQUIRE_MSG(1, 1, "expected a true value");

ATF_TC(require);
ATF_TC_HEAD(require, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_REQUIRE and "
                      "ATF_REQUIRE_MSG macros");
}
ATF_TC_BODY(require, tc)
{
    struct test {
        void (*head)(atf_tc_t *);
        void (*body)(const atf_tc_t *);
        bool value;
        const char *msg;
        bool ok;
    } *t, tests[] = {
        { H_REQUIRE_HEAD_NAME(0), H_REQUIRE_BODY_NAME(0), 0,
          "0 not met", false },
        { H_REQUIRE_HEAD_NAME(1), H_REQUIRE_BODY_NAME(1), 1,
          "1 not met", true },
        { H_REQUIRE_MSG_HEAD_NAME(0), H_REQUIRE_MSG_BODY_NAME(0), 0,
          "expected a false value", false },
        { H_REQUIRE_MSG_HEAD_NAME(1), H_REQUIRE_MSG_BODY_NAME(1), 1,
          "expected a true value", true },
        { NULL, NULL, false, NULL, false }
    };

    for (t = &tests[0]; t->head != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);

        printf("Checking with a %d value\n", t->value);

        RE(atf_tc_init(&tcaux, "h_require", t->head, t->body, NULL, &config));
        run_inherit(&tcaux, &tcr);
        atf_tc_fini(&tcaux);

        ATF_REQUIRE(exists("before"));
        if (t->ok) {
            ATF_REQUIRE(atf_tcr_get_state(&tcr) == atf_tcr_passed_state);
            ATF_REQUIRE(exists("after"));
        } else {
            ATF_REQUIRE(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
            ATF_REQUIRE(!exists("after"));
            ATF_REQUIRE(grep_reason(&tcr, "t_macros.c:[0-9]+: Requirement "
                                    "failed: %s$", t->msg));
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);

        ATF_REQUIRE(unlink("before") != -1);
        if (t->ok)
            ATF_REQUIRE(unlink("after") != -1);
    }
}

/* ---------------------------------------------------------------------
 * Test cases for the ATF_REQUIRE_*EQ_ macros.
 * --------------------------------------------------------------------- */

struct require_eq_test {
    void (*head)(atf_tc_t *);
    void (*body)(const atf_tc_t *);
    const char *v1;
    const char *v2;
    const char *msg;
    bool ok;
};

static
void
do_require_eq_tests(const struct require_eq_test *tests)
{
    const struct require_eq_test *t;

    for (t = &tests[0]; t->head != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);

        printf("Checking with %s, %s and expecting %s\n", t->v1, t->v2,
               t->ok ? "true" : "false");

        RE(atf_tc_init(&tcaux, "h_require", t->head, t->body, NULL, &config));
        run_inherit(&tcaux, &tcr);
        atf_tc_fini(&tcaux);

        ATF_REQUIRE(exists("before"));
        if (t->ok) {
            ATF_REQUIRE(atf_tcr_get_state(&tcr) == atf_tcr_passed_state);
            ATF_REQUIRE(exists("after"));
        } else {
            ATF_REQUIRE(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
            ATF_REQUIRE(!exists("after"));
            ATF_REQUIRE(grep_reason(&tcr, "t_macros.c:[0-9]+: Requirement "
                                    "failed: %s$", t->msg));
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);

        ATF_REQUIRE(unlink("before") != -1);
        if (t->ok)
            ATF_REQUIRE(unlink("after") != -1);
    }
}

H_REQUIRE_EQ(1_1, 1, 1);
H_REQUIRE_EQ(1_2, 1, 2);
H_REQUIRE_EQ(2_1, 2, 1);
H_REQUIRE_EQ(2_2, 2, 2);
H_REQUIRE_EQ_MSG(1_1, 1, 1, "1 does not match 1");
H_REQUIRE_EQ_MSG(1_2, 1, 2, "1 does not match 2");
H_REQUIRE_EQ_MSG(2_1, 2, 1, "2 does not match 1");
H_REQUIRE_EQ_MSG(2_2, 2, 2, "2 does not match 2");

ATF_TC(require_eq);
ATF_TC_HEAD(require_eq, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_REQUIRE_EQ and "
                      "ATF_REQUIRE_EQ_MSG macros");
}
ATF_TC_BODY(require_eq, tc)
{
    struct require_eq_test tests[] = {
        { H_REQUIRE_EQ_HEAD_NAME(1_1), H_REQUIRE_EQ_BODY_NAME(1_1),
          "1", "1", "1 != 1", true },
        { H_REQUIRE_EQ_HEAD_NAME(1_2), H_REQUIRE_EQ_BODY_NAME(1_2),
          "1", "2", "1 != 2", false },
        { H_REQUIRE_EQ_HEAD_NAME(2_1), H_REQUIRE_EQ_BODY_NAME(2_1),
          "2", "1", "2 != 1", false },
        { H_REQUIRE_EQ_HEAD_NAME(2_2), H_REQUIRE_EQ_BODY_NAME(2_2),
          "2", "2", "2 != 2", true },
        { H_REQUIRE_EQ_MSG_HEAD_NAME(1_1), H_REQUIRE_EQ_MSG_BODY_NAME(1_1),
          "1", "1", "1 != 1: 1 does not match 1", true },
        { H_REQUIRE_EQ_MSG_HEAD_NAME(1_2), H_REQUIRE_EQ_MSG_BODY_NAME(1_2),
          "1", "2", "1 != 2: 1 does not match 2", false },
        { H_REQUIRE_EQ_MSG_HEAD_NAME(2_1), H_REQUIRE_EQ_MSG_BODY_NAME(2_1),
          "2", "1", "2 != 1: 2 does not match 1", false },
        { H_REQUIRE_EQ_MSG_HEAD_NAME(2_2), H_REQUIRE_EQ_MSG_BODY_NAME(2_2),
          "2", "2", "2 != 2: 2 does not match 2", true },
        { NULL, NULL, 0, 0, "", false }
    };
    do_require_eq_tests(tests);
}

H_REQUIRE_STREQ(1_1, "1", "1");
H_REQUIRE_STREQ(1_2, "1", "2");
H_REQUIRE_STREQ(2_1, "2", "1");
H_REQUIRE_STREQ(2_2, "2", "2");
H_REQUIRE_STREQ_MSG(1_1, "1", "1", "1 does not match 1");
H_REQUIRE_STREQ_MSG(1_2, "1", "2", "1 does not match 2");
H_REQUIRE_STREQ_MSG(2_1, "2", "1", "2 does not match 1");
H_REQUIRE_STREQ_MSG(2_2, "2", "2", "2 does not match 2");
#define REQUIRE_STREQ_VAR1 "5"
#define REQUIRE_STREQ_VAR2 "9"
const const char *require_streq_var1 = REQUIRE_STREQ_VAR1;
const const char *require_streq_var2 = REQUIRE_STREQ_VAR2;
H_REQUIRE_STREQ(vars, require_streq_var1, require_streq_var2);

ATF_TC(require_streq);
ATF_TC_HEAD(require_streq, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_REQUIRE_STREQ and "
                      "ATF_REQUIRE_STREQ_MSG macros");
}
ATF_TC_BODY(require_streq, tc)
{
    struct require_eq_test tests[] = {
        { H_REQUIRE_STREQ_HEAD_NAME(1_1), H_REQUIRE_STREQ_BODY_NAME(1_1),
          "1", "1", "\"1\" != \"1\" \\(1 != 1\\)", true },
        { H_REQUIRE_STREQ_HEAD_NAME(1_2), H_REQUIRE_STREQ_BODY_NAME(1_2),
          "1", "2", "\"1\" != \"2\" \\(1 != 2\\)", false },
        { H_REQUIRE_STREQ_HEAD_NAME(2_1), H_REQUIRE_STREQ_BODY_NAME(2_1),
          "2", "1", "\"2\" != \"1\" \\(2 != 1\\)", false },
        { H_REQUIRE_STREQ_HEAD_NAME(2_2), H_REQUIRE_STREQ_BODY_NAME(2_2),
          "2", "2", "\"2\" != \"2\" \\(2 != 2\\)", true },
        { H_REQUIRE_STREQ_MSG_HEAD_NAME(1_1),
          H_REQUIRE_STREQ_MSG_BODY_NAME(1_1),
          "1", "1", "\"1\" != \"1\" \\(1 != 1\\): 1 does not match 1", true },
        { H_REQUIRE_STREQ_MSG_HEAD_NAME(1_2),
          H_REQUIRE_STREQ_MSG_BODY_NAME(1_2),
          "1", "2", "\"1\" != \"2\" \\(1 != 2\\): 1 does not match 2", false },
        { H_REQUIRE_STREQ_MSG_HEAD_NAME(2_1),
          H_REQUIRE_STREQ_MSG_BODY_NAME(2_1),
          "2", "1", "\"2\" != \"1\" \\(2 != 1\\): 2 does not match 1", false },
        { H_REQUIRE_STREQ_MSG_HEAD_NAME(2_2),
          H_REQUIRE_STREQ_MSG_BODY_NAME(2_2),
          "2", "2", "\"2\" != \"2\" \\(2 != 2\\): 2 does not match 2", true },
        { H_REQUIRE_STREQ_HEAD_NAME(vars), H_REQUIRE_STREQ_BODY_NAME(vars),
          require_streq_var1, require_streq_var2,
          "require_streq_var1 != require_streq_var2 \\("
          REQUIRE_STREQ_VAR1 " != " REQUIRE_STREQ_VAR2 "\\)", false },
        { NULL, NULL, 0, 0, "", false }
    };
    do_require_eq_tests(tests);
}

/* ---------------------------------------------------------------------
 * Miscellaneous test cases covering several macros.
 * --------------------------------------------------------------------- */

static
bool
aux_bool(const char *fmt)
{
    return false;
}

static
const char *
aux_str(const char *fmt)
{
    return "foo";
}

H_CHECK(msg, aux_bool("%d"));
H_REQUIRE(msg, aux_bool("%d"));
H_CHECK_STREQ(msg, aux_str("%d"), "");
H_REQUIRE_STREQ(msg, aux_str("%d"), "");

ATF_TC(msg_embedded_fmt);
ATF_TC_HEAD(msg_embedded_fmt, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests that format strings passed "
                      "as part of the automatically-generated messages "
                      "do not get expanded");
}
ATF_TC_BODY(msg_embedded_fmt, tc)
{
    struct test {
        void (*head)(atf_tc_t *);
        void (*body)(const atf_tc_t *);
        bool fatal;
        const char *msg;
    } *t, tests[] = {
       {  H_CHECK_HEAD_NAME(msg), H_CHECK_BODY_NAME(msg), false,
          "aux_bool(\"%d\")" },
       {  H_REQUIRE_HEAD_NAME(msg), H_REQUIRE_BODY_NAME(msg), true,
          "aux_bool(\"%d\")" },
       {  H_CHECK_STREQ_HEAD_NAME(msg), H_CHECK_STREQ_BODY_NAME(msg), false,
          "aux_str(\"%d\") != \"\" (foo != )" },
       {  H_REQUIRE_STREQ_HEAD_NAME(msg), H_REQUIRE_STREQ_BODY_NAME(msg), true,
          "aux_str(\"%d\") != \"\" (foo != )" },
       { NULL, NULL, false, NULL }
    };

    for (t = &tests[0]; t->head != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);

        printf("Checking with an expected '%s' message\n", t->msg);

        RE(atf_tc_init(&tcaux, "h_check", t->head, t->body, NULL, &config));
        run_capture(&tcaux, "error", &tcr);
        atf_tc_fini(&tcaux);

        ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
        if (t->fatal) {
            bool matched =
                grep_reason(&tcr, "t_macros.c:[0-9]+: "
                            "Requirement failed: %s$", t->msg);
            ATF_CHECK_MSG(matched, "couldn't find error string in result");
        } else {
            bool matched =
                grep_file("error", "t_macros.c:[0-9]+: "
                          "Check failed: %s$", t->msg);
            ATF_CHECK_MSG(matched, "couldn't find error string in output");
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);
    }
}

/* ---------------------------------------------------------------------
 * Tests cases for the header file.
 * --------------------------------------------------------------------- */

HEADER_TC(include, "atf-c/macros.h", "d_include_macros_h.c");
BUILD_TC(use, "d_use_macros_h.c",
         "Tests that the macros provided by the atf-c/macros.h file "
         "do not cause syntax errors when used",
         "Build of d_use_macros_h.c failed; some macros in atf-c/macros.h "
         "are broken");

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, check);
    ATF_TP_ADD_TC(tp, check_eq);
    ATF_TP_ADD_TC(tp, check_streq);

    ATF_TP_ADD_TC(tp, require);
    ATF_TP_ADD_TC(tp, require_eq);
    ATF_TP_ADD_TC(tp, require_streq);

    ATF_TP_ADD_TC(tp, msg_embedded_fmt);

    /* Add the test cases for the header file. */
    ATF_TP_ADD_TC(tp, include);
    ATF_TP_ADD_TC(tp, use);

    return atf_no_error();
}
