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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/check.h"
#include "atf-c/config.h"
#include "atf-c/fs.h"
#include "atf-c/io.h"

#include "h_macros.h"

atf_error_t atf_check_result_init(atf_check_result_t *);

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
void
get_helpers_path(const atf_tc_t *tc, char buf[], size_t buflen)
{
    snprintf(buf, buflen, "%s/h_check", atf_tc_get_config_var(tc, "srcdir"));
}

static
void
do_exec(const atf_tc_t *tc, const char *helper_name, atf_check_result_t *r)
{
    char buf[1024];
    char *argv[3];

    get_helpers_path(tc, buf, sizeof(buf));

    argv[0] = buf;
    argv[1] = strdup(helper_name);
    argv[2] = NULL;
    printf("Executing %s %s\n", argv[0], argv[1]);
    RE(atf_check_exec(argv, r));
    free(argv[1]);
}

static
void
do_exec_with_arg(const atf_tc_t *tc, const char *helper_name, const char *arg,
                 atf_check_result_t *r)
{
    char buf[1024];
    char *argv[4];

    get_helpers_path(tc, buf, sizeof(buf));

    argv[0] = buf;
    argv[1] = strdup(helper_name);
    argv[2] = strdup(arg);
    argv[3] = NULL;
    printf("Executing %s %s %s\n", argv[0], argv[1], argv[2]);
    RE(atf_check_exec(argv, r));
    free(argv[2]);
    free(argv[1]);
}

static
void
check_line(int fd, const char *exp)
{
    atf_dynstr_t line;

    atf_dynstr_init(&line);
    RE(atf_io_readline(fd, &line));
    ATF_CHECK(atf_equal_dynstr_cstring(&line, exp));
    atf_dynstr_fini(&line);
}

/* ---------------------------------------------------------------------
 * Test cases for the "atf_check_result" type.
 * --------------------------------------------------------------------- */

ATF_TC(result_templates);
ATF_TC_HEAD(result_templates, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests that atf_check_result is "
                      "initialized with correct temporary file templates");
}
ATF_TC_BODY(result_templates, tc)
{
    atf_check_result_t result1, result2;
    const atf_fs_path_t *out1, *out2;
    const atf_fs_path_t *err1, *err2;

    RE(atf_check_result_init(&result1));
    RE(atf_check_result_init(&result2));

    out1 = atf_check_result_stdout(&result1);
    out2 = atf_check_result_stdout(&result2);
    err1 = atf_check_result_stderr(&result1);
    err2 = atf_check_result_stderr(&result2);

    ATF_CHECK(strstr(atf_fs_path_cstring(out1), "stdout.XXXXXX") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(out2), "stdout.XXXXXX") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err1), "stderr.XXXXXX") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err2), "stderr.XXXXXX") != NULL);

    ATF_CHECK(strcmp(atf_fs_path_cstring(out1),
                     atf_fs_path_cstring(out2)) == 0);
    ATF_CHECK(strcmp(atf_fs_path_cstring(err1),
                     atf_fs_path_cstring(err2)) == 0);

    atf_check_result_fini(&result2);
    atf_check_result_fini(&result1);
}

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(exec_cleanup);
ATF_TC_HEAD(exec_cleanup, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec properly "
                      "cleans up the temporary files it creates");
}
ATF_TC_BODY(exec_cleanup, tc)
{
    atf_fs_path_t out, err;
    atf_check_result_t result;
    bool exists;

    do_exec(tc, "exit-success", &result);
    RE(atf_fs_path_copy(&out, atf_check_result_stdout(&result)));
    RE(atf_fs_path_copy(&err, atf_check_result_stderr(&result)));

    RE(atf_fs_exists(&out, &exists)); ATF_CHECK(exists);
    RE(atf_fs_exists(&err, &exists)); ATF_CHECK(exists);
    atf_check_result_fini(&result);
    RE(atf_fs_exists(&out, &exists)); ATF_CHECK(!exists);
    RE(atf_fs_exists(&err, &exists)); ATF_CHECK(!exists);
}

ATF_TC(exec_exitstatus);
ATF_TC_HEAD(exec_exitstatus, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec properly "
                      "captures the exit status of the executed command");
}
ATF_TC_BODY(exec_exitstatus, tc)
{
    {
        atf_check_result_t result;
        do_exec(tc, "exit-success", &result);
        ATF_CHECK(atf_check_result_exited(&result));
        ATF_CHECK(atf_check_result_exitcode(&result) == EXIT_SUCCESS);
        atf_check_result_fini(&result);
    }

    {
        atf_check_result_t result;
        do_exec(tc, "exit-failure", &result);
        ATF_CHECK(atf_check_result_exited(&result));
        ATF_CHECK(atf_check_result_exitcode(&result) == EXIT_FAILURE);
        atf_check_result_fini(&result);
    }

    {
        atf_check_result_t result;
        do_exec(tc, "exit-signal", &result);
        ATF_CHECK(!atf_check_result_exited(&result));
        atf_check_result_fini(&result);
    }
}

ATF_TC(exec_stdout_stderr);
ATF_TC_HEAD(exec_stdout_stderr, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec properly "
                      "captures the stdout and stderr streams of the "
                      "child process");
}
ATF_TC_BODY(exec_stdout_stderr, tc)
{
    atf_check_result_t result1, result2;
    const atf_fs_path_t *out1, *out2;
    const atf_fs_path_t *err1, *err2;

    do_exec_with_arg(tc, "stdout-stderr", "result1", &result1);
    ATF_CHECK(atf_check_result_exited(&result1));
    ATF_CHECK(atf_check_result_exitcode(&result1) == EXIT_SUCCESS);

    do_exec_with_arg(tc, "stdout-stderr", "result2", &result2);
    ATF_CHECK(atf_check_result_exited(&result2));
    ATF_CHECK(atf_check_result_exitcode(&result2) == EXIT_SUCCESS);

    out1 = atf_check_result_stdout(&result1);
    out2 = atf_check_result_stdout(&result2);
    err1 = atf_check_result_stderr(&result1);
    err2 = atf_check_result_stderr(&result2);

    ATF_CHECK(strstr(atf_fs_path_cstring(out1), "stdout.XXXXXX") == NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(out2), "stdout.XXXXXX") == NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err1), "stderr.XXXXXX") == NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err2), "stderr.XXXXXX") == NULL);

    ATF_CHECK(strcmp(atf_fs_path_cstring(out1),
                     atf_fs_path_cstring(out2)) != 0);
    ATF_CHECK(strcmp(atf_fs_path_cstring(err1),
                     atf_fs_path_cstring(err2)) != 0);

#define CHECK_LINES(path, outname, resname) \
    do { \
        int fd = open(atf_fs_path_cstring(path), O_RDONLY); \
        ATF_CHECK(fd != -1); \
        check_line(fd, "Line 1 to " outname " for " resname); \
        check_line(fd, "Line 2 to " outname " for " resname); \
        close(fd); \
    } while (false)

    CHECK_LINES(out1, "stdout", "result1");
    CHECK_LINES(out2, "stdout", "result2");
    CHECK_LINES(err1, "stderr", "result1");
    CHECK_LINES(err2, "stderr", "result2");

#undef CHECK_LINES

    atf_check_result_fini(&result2);
    atf_check_result_fini(&result1);
}

ATF_TC(exec_unknown);
ATF_TC_HEAD(exec_unknown, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that running a non-existing "
                      "binary is handled correctly");
}
ATF_TC_BODY(exec_unknown, tc)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s/non-existent",
             atf_config_get("atf_workdir"));

    char *argv[2];
    argv[0] = buf;
    argv[1] = NULL;

    atf_check_result_t result;
    RE(atf_check_exec(argv, &result));
    ATF_CHECK(atf_check_result_exited(&result));
    ATF_CHECK(atf_check_result_exitcode(&result) == 127);
    atf_check_result_fini(&result);
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, result_templates);
    ATF_TP_ADD_TC(tp, exec_cleanup);
    ATF_TP_ADD_TC(tp, exec_exitstatus);
    ATF_TP_ADD_TC(tp, exec_stdout_stderr);
    ATF_TP_ADD_TC(tp, exec_unknown);

    return atf_no_error();
}
