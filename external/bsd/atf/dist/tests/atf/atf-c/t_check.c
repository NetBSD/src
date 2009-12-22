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
#include "atf-c/tcr.h"

#include "h_lib.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
void
do_exec(const atf_tc_t *tc, const char *helper_name, atf_check_result_t *r)
{
    atf_fs_path_t h_processes;
    const char *argv[3];

    get_h_processes_path(tc, &h_processes);

    argv[0] = atf_fs_path_cstring(&h_processes);
    argv[1] = helper_name;
    argv[2] = NULL;
    printf("Executing %s %s\n", argv[0], argv[1]);
    RE(atf_check_exec_array(argv, r));

    atf_fs_path_fini(&h_processes);
}

static
void
do_exec_with_arg(const atf_tc_t *tc, const char *helper_name, const char *arg,
                 atf_check_result_t *r)
{
    atf_fs_path_t h_processes;
    const char *argv[4];

    get_h_processes_path(tc, &h_processes);

    argv[0] = atf_fs_path_cstring(&h_processes);
    argv[1] = helper_name;
    argv[2] = arg;
    argv[3] = NULL;
    printf("Executing %s %s %s\n", argv[0], argv[1], argv[2]);
    RE(atf_check_exec_array(argv, r));

    atf_fs_path_fini(&h_processes);
}

static
void
check_line(int fd, const char *exp)
{
    atf_dynstr_t line;
    bool eof;

    atf_dynstr_init(&line);
    RE(atf_io_readline(fd, &line, &eof));
    ATF_CHECK(!eof);
    ATF_CHECK_MSG(atf_equal_dynstr_cstring(&line, exp),
                  "read: '%s', expected: '%s'",
                  atf_dynstr_cstring(&line), exp);
    atf_dynstr_fini(&line);
}

/* ---------------------------------------------------------------------
 * Helper test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(h_build_c_o_ok);
ATF_TC_HEAD(h_build_c_o_ok, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case for build_c_o");
}
ATF_TC_BODY(h_build_c_o_ok, tc)
{
    FILE *sfile;
    bool success;

    ATF_REQUIRE((sfile = fopen("test.c", "w")) != NULL);
    fprintf(sfile, "#include <stdio.h>\n");
    fclose(sfile);

    RE(atf_check_build_c_o("test.c", "test.o", NULL, &success));
    ATF_REQUIRE(success);
}

ATF_TC(h_build_c_o_fail);
ATF_TC_HEAD(h_build_c_o_fail, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case for build_c_o");
}
ATF_TC_BODY(h_build_c_o_fail, tc)
{
    FILE *sfile;
    bool success;

    ATF_REQUIRE((sfile = fopen("test.c", "w")) != NULL);
    fprintf(sfile, "void foo(void) { int a = UNDEFINED_SYMBOL; }\n");
    fclose(sfile);

    RE(atf_check_build_c_o("test.c", "test.o", NULL, &success));
    ATF_REQUIRE(!success);
}

ATF_TC(h_build_cpp_ok);
ATF_TC_HEAD(h_build_cpp_ok, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case for build_cpp");
}
ATF_TC_BODY(h_build_cpp_ok, tc)
{
    FILE *sfile;
    bool success;
    atf_fs_path_t test_p;

    RE(atf_fs_path_init_fmt(&test_p, "%s/test.p",
                            atf_tc_get_config_var(tc, "callerdir")));

    ATF_REQUIRE((sfile = fopen("test.c", "w")) != NULL);
    fprintf(sfile, "#define A foo\n");
    fprintf(sfile, "#define B bar\n");
    fprintf(sfile, "A B\n");
    fclose(sfile);

    RE(atf_check_build_cpp("test.c", atf_fs_path_cstring(&test_p), NULL,
                           &success));
    ATF_REQUIRE(success);

    atf_fs_path_fini(&test_p);
}

ATF_TC(h_build_cpp_fail);
ATF_TC_HEAD(h_build_cpp_fail, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case for build_cpp");
}
ATF_TC_BODY(h_build_cpp_fail, tc)
{
    FILE *sfile;
    bool success;

    ATF_REQUIRE((sfile = fopen("test.c", "w")) != NULL);
    fprintf(sfile, "#include \"./non-existent.h\"\n");
    fclose(sfile);

    RE(atf_check_build_cpp("test.c", "test.p", NULL, &success));
    ATF_REQUIRE(!success);
}

ATF_TC(h_build_cxx_o_ok);
ATF_TC_HEAD(h_build_cxx_o_ok, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case for build_cxx_o");
}
ATF_TC_BODY(h_build_cxx_o_ok, tc)
{
    FILE *sfile;
    bool success;

    ATF_REQUIRE((sfile = fopen("test.cpp", "w")) != NULL);
    fprintf(sfile, "#include <iostream>\n");
    fclose(sfile);

    RE(atf_check_build_cxx_o("test.cpp", "test.o", NULL, &success));
    ATF_REQUIRE(success);
}

ATF_TC(h_build_cxx_o_fail);
ATF_TC_HEAD(h_build_cxx_o_fail, tc)
{
    atf_tc_set_md_var(tc, "descr", "Helper test case for build_cxx_o");
}
ATF_TC_BODY(h_build_cxx_o_fail, tc)
{
    FILE *sfile;
    bool success;

    ATF_REQUIRE((sfile = fopen("test.cpp", "w")) != NULL);
    fprintf(sfile, "void foo(void) { int a = UNDEFINED_SYMBOL; }\n");
    fclose(sfile);

    RE(atf_check_build_cxx_o("test.cpp", "test.o", NULL, &success));
    ATF_REQUIRE(!success);
}

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

static
void
run_h_tc(atf_tc_t *tc, const atf_tc_pack_t *tcpack,
         const char *outname, const char *errname)
{
    atf_fs_path_t cwd;
    atf_map_t config;
    atf_tcr_t tcr;
    int fdout, fderr;

    RE(atf_fs_getcwd(&cwd));
    RE(atf_map_init(&config));
    RE(atf_map_insert(&config, "callerdir",
                      strdup(atf_fs_path_cstring(&cwd)), true));

    ATF_REQUIRE((fdout = open(outname, O_CREAT | O_WRONLY | O_TRUNC,
                              0600)) != -1);
    ATF_REQUIRE((fderr = open(errname, O_CREAT | O_WRONLY | O_TRUNC,
                              0600)) != -1);

    RE(atf_tc_init_pack(tc, tcpack, &config));
    RE(atf_tc_run(tc, &tcr, fdout, fderr, &cwd));
    atf_tc_fini(tc);

    ATF_CHECK_EQ(atf_tcr_get_state(&tcr), atf_tcr_passed_state);
    atf_tcr_fini(&tcr);

    close(fderr);
    close(fdout);

    atf_map_fini(&config);
    atf_fs_path_fini(&cwd);
}

ATF_TC(build_c_o);
ATF_TC_HEAD(build_c_o, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_check_build_c_o "
                      "function");
}
ATF_TC_BODY(build_c_o, tc)
{
    run_h_tc(&ATF_TC_NAME(h_build_c_o_ok),
             &ATF_TC_PACK_NAME(h_build_c_o_ok), "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.c"));

    run_h_tc(&ATF_TC_NAME(h_build_c_o_fail),
             &ATF_TC_PACK_NAME(h_build_c_o_fail), "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.c"));
    ATF_CHECK(grep_file("stderr", "test.c"));
    ATF_CHECK(grep_file("stderr", "UNDEFINED_SYMBOL"));
}

ATF_TC(build_cpp);
ATF_TC_HEAD(build_cpp, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_check_build_cpp "
                      "function");
}
ATF_TC_BODY(build_cpp, tc)
{
    run_h_tc(&ATF_TC_NAME(h_build_cpp_ok),
             &ATF_TC_PACK_NAME(h_build_cpp_ok), "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o.*test.p"));
    ATF_CHECK(grep_file("stdout", "test.c"));
    ATF_CHECK(grep_file("test.p", "foo bar"));

    run_h_tc(&ATF_TC_NAME(h_build_cpp_fail),
             &ATF_TC_PACK_NAME(h_build_cpp_fail), "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.p"));
    ATF_CHECK(grep_file("stdout", "test.c"));
    ATF_CHECK(grep_file("stderr", "test.c"));
    ATF_CHECK(grep_file("stderr", "non-existent.h"));
}

ATF_TC(build_cxx_o);
ATF_TC_HEAD(build_cxx_o, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_check_build_cxx_o "
                      "function");
}
ATF_TC_BODY(build_cxx_o, tc)
{
    run_h_tc(&ATF_TC_NAME(h_build_cxx_o_ok),
             &ATF_TC_PACK_NAME(h_build_cxx_o_ok), "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.cpp"));

    run_h_tc(&ATF_TC_NAME(h_build_cxx_o_fail),
             &ATF_TC_PACK_NAME(h_build_cxx_o_fail), "stdout", "stderr");
    ATF_CHECK(grep_file("stdout", "-o test.o"));
    ATF_CHECK(grep_file("stdout", "-c test.cpp"));
    ATF_CHECK(grep_file("stderr", "test.cpp"));
    ATF_CHECK(grep_file("stderr", "UNDEFINED_SYMBOL"));
}

ATF_TC(exec_argv);
ATF_TC_HEAD(exec_argv, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec_array "
                      "preserves the provided argv");
}
ATF_TC_BODY(exec_argv, tc)
{
    atf_fs_path_t h_processes;
    atf_check_result_t result;

    get_h_processes_path(tc, &h_processes);
    do_exec(tc, "exit-success", &result);

    const atf_list_t *argv = atf_check_result_argv(&result);
    ATF_REQUIRE_EQ(atf_list_size(argv), 2);
    ATF_CHECK_STREQ((const char *)atf_list_index_c(argv, 0),
                    atf_fs_path_cstring(&h_processes));
    ATF_CHECK_STREQ((const char *)atf_list_index_c(argv, 1), "exit-success");

    atf_check_result_fini(&result);
    atf_fs_path_fini(&h_processes);
}

ATF_TC(exec_cleanup);
ATF_TC_HEAD(exec_cleanup, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec_array "
                      "properly cleans up the temporary files it creates");
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

    atf_fs_path_fini(&err);
    atf_fs_path_fini(&out);
}

ATF_TC(exec_exitstatus);
ATF_TC_HEAD(exec_exitstatus, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec_array "
                      "properly captures the exit status of the executed "
                      "command");
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

ATF_TC(exec_list);
ATF_TC_HEAD(exec_list, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec_list "
                      "works properly; assumes that this method is "
                      "backed by atf_check_exec_array, so the tests "
                      "are not exhaustive");
}
ATF_TC_BODY(exec_list, tc)
{
    atf_fs_path_t h_processes;
    atf_list_t argv;
    atf_check_result_t result;

    RE(atf_list_init(&argv));

    get_h_processes_path(tc, &h_processes);
    atf_list_append(&argv, strdup(atf_fs_path_cstring(&h_processes)), true);
    atf_list_append(&argv, strdup("echo"), true);
    atf_list_append(&argv, strdup("test-message"), true);
    RE(atf_check_exec_list(&argv, &result));
    atf_list_fini(&argv);

    ATF_CHECK(atf_check_result_exited(&result));
    ATF_CHECK(atf_check_result_exitcode(&result) == EXIT_SUCCESS);

    {
        const atf_fs_path_t *path = atf_check_result_stdout(&result);
        int fd = open(atf_fs_path_cstring(path), O_RDONLY);
        ATF_CHECK(fd != -1);
        check_line(fd, "test-message");
        close(fd);
    }

    atf_check_result_fini(&result);
    atf_fs_path_fini(&h_processes);
}

ATF_TC(exec_stdout_stderr);
ATF_TC_HEAD(exec_stdout_stderr, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec_array "
                      "properly captures the stdout and stderr streams "
                      "of the child process");
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

    ATF_CHECK(strstr(atf_fs_path_cstring(out1), "check.XXXXXX") == NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(out2), "check.XXXXXX") == NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err1), "check.XXXXXX") == NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err2), "check.XXXXXX") == NULL);

    ATF_CHECK(strstr(atf_fs_path_cstring(out1), "/check") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(out2), "/check") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err1), "/check") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err2), "/check") != NULL);

    ATF_CHECK(strstr(atf_fs_path_cstring(out1), "/stdout") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(out2), "/stdout") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err1), "/stderr") != NULL);
    ATF_CHECK(strstr(atf_fs_path_cstring(err2), "/stderr") != NULL);

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

ATF_TC(exec_umask);
ATF_TC_HEAD(exec_umask, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks that atf_check_exec_array "
                      "correctly reports an error if the umask is too "
                      "restrictive to create temporary files");
}
ATF_TC_BODY(exec_umask, tc)
{
    atf_check_result_t result;
    atf_fs_path_t h_processes;
    const char *argv[3];

    get_h_processes_path(tc, &h_processes);
    argv[0] = atf_fs_path_cstring(&h_processes);
    argv[1] = "exit-success";
    argv[2] = NULL;

    umask(0222);
    atf_error_t err = atf_check_exec_array(argv, &result);
    ATF_CHECK(atf_is_error(err));
    ATF_CHECK(atf_error_is(err, "invalid_umask"));
    atf_error_free(err);

    atf_fs_path_fini(&h_processes);
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

    const char *argv[2];
    argv[0] = buf;
    argv[1] = NULL;

    atf_check_result_t result;
    RE(atf_check_exec_array(argv, &result));
    ATF_CHECK(atf_check_result_exited(&result));
    ATF_CHECK(atf_check_result_exitcode(&result) == 127);
    atf_check_result_fini(&result);
}

/* ---------------------------------------------------------------------
 * Tests cases for the header file.
 * --------------------------------------------------------------------- */

HEADER_TC(include, "atf-c/check.h", "d_include_check_h.c");

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    /* Add the test cases for the free functions. */
    ATF_TP_ADD_TC(tp, build_c_o);
    ATF_TP_ADD_TC(tp, build_cpp);
    ATF_TP_ADD_TC(tp, build_cxx_o);
    ATF_TP_ADD_TC(tp, exec_argv);
    ATF_TP_ADD_TC(tp, exec_cleanup);
    ATF_TP_ADD_TC(tp, exec_exitstatus);
    ATF_TP_ADD_TC(tp, exec_list);
    ATF_TP_ADD_TC(tp, exec_stdout_stderr);
    ATF_TP_ADD_TC(tp, exec_umask);
    ATF_TP_ADD_TC(tp, exec_unknown);

    /* Add the test cases for the header file. */
    ATF_TP_ADD_TC(tp, include);

    return atf_no_error();
}
