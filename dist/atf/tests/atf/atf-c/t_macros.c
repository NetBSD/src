/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/fs.h"
#include "atf-c/tcr.h"
#include "atf-c/text.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

static
void
create_ctl_file(const atf_tc_t *tc, const char *name)
{
    atf_fs_path_t p;

    CE(atf_fs_path_init_fmt(&p, "%s/%s",
                            atf_tc_get_config_var(tc, "ctldir"), name));
    ATF_CHECK(open(atf_fs_path_cstring(&p),
                   O_CREAT | O_WRONLY | O_TRUNC, 0644) != -1);
    atf_fs_path_fini(&p);
}

static
bool
exists(const char *p)
{
    bool b;
    atf_fs_path_t pp;

    CE(atf_fs_path_init_fmt(&pp, "%s", p));
    CE(atf_fs_exists(&pp, &b));
    atf_fs_path_fini(&pp);

    return b;
}

static
void
init_config(atf_map_t *config)
{
    atf_fs_path_t cwd;

    CE(atf_map_init(config));

    CE(atf_fs_getcwd(&cwd));
    CE(atf_map_insert(config, "ctldir",
                      strdup(atf_fs_path_cstring(&cwd)), true));
    atf_fs_path_fini(&cwd);
}

static
void
run_here(const atf_tc_t *tc, atf_tcr_t *tcr)
{
    atf_fs_path_t cwd;

    CE(atf_fs_getcwd(&cwd));
    CE(atf_tc_run(tc, tcr, &cwd));
    atf_fs_path_fini(&cwd);
}

/* ---------------------------------------------------------------------
 * Helper test cases.
 * --------------------------------------------------------------------- */

ATF_TC_HEAD(h_check, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case");
}
ATF_TC_BODY(h_check, tc)
{
    bool condition;

    CE(atf_text_to_bool(atf_tc_get_config_var(tc, "condition"), &condition));

    create_ctl_file(tc, "before");
    ATF_CHECK(condition);
    create_ctl_file(tc, "after");
}

ATF_TC_HEAD(h_check_equal, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case");
}
ATF_TC_BODY(h_check_equal, tc)
{
    long v1, v2;

    CE(atf_text_to_long(atf_tc_get_config_var(tc, "v1"), &v1));
    CE(atf_text_to_long(atf_tc_get_config_var(tc, "v2"), &v2));

    create_ctl_file(tc, "before");
    ATF_CHECK_EQUAL(v1, v2);
    create_ctl_file(tc, "after");
}

/* ---------------------------------------------------------------------
 * Test cases for the macros.
 * --------------------------------------------------------------------- */

ATF_TC(check);
ATF_TC_HEAD(check, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_CHECK macro");
}
ATF_TC_BODY(check, tc)
{
    struct test {
        const char *cond;
        bool ok;
    } *t, tests[] = {
        { "false", false },
        { "true", true },
        { NULL, false }
    };

    for (t = &tests[0]; t->cond != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);
        CE(atf_map_insert(&config, "condition", strdup(t->cond), true));

        printf("Checking with a %s value\n", t->cond);

        CE(atf_tc_init(&tcaux, "h_check", ATF_TC_HEAD_NAME(h_check),
                       ATF_TC_BODY_NAME(h_check), NULL, &config));
        run_here(&tcaux, &tcr);
        atf_tc_fini(&tcaux);

        ATF_CHECK(exists("before"));
        if (t->ok) {
            ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_passed_state);
            ATF_CHECK(exists("after"));
        } else {
            const char *r;

            ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
            ATF_CHECK(!exists("after"));

            r = atf_dynstr_cstring(atf_tcr_get_reason(&tcr));
            ATF_CHECK(strstr(r, "condition not met") != NULL);
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);

        ATF_CHECK(unlink("before") != -1);
        if (t->ok)
            ATF_CHECK(unlink("after") != -1);
    }
}

ATF_TC(check_equal);
ATF_TC_HEAD(check_equal, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the ATF_CHECK_EQUAL macro");
}
ATF_TC_BODY(check_equal, tc)
{
    struct test {
        const char *v1;
        const char *v2;
        bool ok;
    } *t, tests[] = {
        { "1", "1", true },
        { "1", "2", false },
        { "2", "1", false },
        { "2", "2", true },
        { NULL, NULL, false }
    };

    for (t = &tests[0]; t->v1 != NULL; t++) {
        atf_map_t config;
        atf_tc_t tcaux;
        atf_tcr_t tcr;

        init_config(&config);
        CE(atf_map_insert(&config, "v1", strdup(t->v1), true));
        CE(atf_map_insert(&config, "v2", strdup(t->v2), true));

        printf("Checking with %s, %s and expecting %s\n", t->v1, t->v2,
               t->ok ? "true" : "false");

        CE(atf_tc_init(&tcaux, "h_check", ATF_TC_HEAD_NAME(h_check_equal),
                       ATF_TC_BODY_NAME(h_check_equal), NULL, &config));
        run_here(&tcaux, &tcr);
        atf_tc_fini(&tcaux);

        ATF_CHECK(exists("before"));
        if (t->ok) {
            ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_passed_state);
            ATF_CHECK(exists("after"));
        } else {
            const char *r;

            ATF_CHECK(atf_tcr_get_state(&tcr) == atf_tcr_failed_state);
            ATF_CHECK(!exists("after"));

            r = atf_dynstr_cstring(atf_tcr_get_reason(&tcr));
            ATF_CHECK(strstr(r, "v1 != v2") != NULL);
        }

        atf_tcr_fini(&tcr);
        atf_map_fini(&config);

        ATF_CHECK(unlink("before") != -1);
        if (t->ok)
            ATF_CHECK(unlink("after") != -1);
    }
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, check);
    ATF_TP_ADD_TC(tp, check_equal);

    return atf_no_error();
}
