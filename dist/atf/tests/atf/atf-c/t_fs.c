/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/fs.h"
#include "atf-c/user.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

static
void
create_dir(const char *p, int mode)
{
    int fd;

    fd = mkdir(p, mode);
    if (fd == -1)
        atf_tc_fail("Could not create helper directory %s", p);
}

static
void
create_file(const char *p, int mode)
{
    int fd;

    fd = open(p, O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd == -1)
        atf_tc_fail("Could not create helper file %s", p);
}

static
bool
exists(const atf_fs_path_t *p)
{
    return access(atf_fs_path_cstring(p), F_OK) == 0;
}

static
bool
not_exists(const atf_fs_path_t *p)
{
    return access(atf_fs_path_cstring(p), F_OK) == -1 && errno == ENOENT;
}

/* ---------------------------------------------------------------------
 * Test cases for the "atf_fs_path" type.
 * --------------------------------------------------------------------- */

ATF_TC(path_normalize);
ATF_TC_HEAD(path_normalize, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the path's normalization");
}
ATF_TC_BODY(path_normalize, tc)
{
    struct test {
        const char *in;
        const char *out;
    } tests[] = {
        { ".", ".", },
        { "..", "..", },

        { "/", "/", },
        { "//", "/", }, /* NO_CHECK_STYLE */
        { "///", "/", }, /* NO_CHECK_STYLE */

        { "foo", "foo", },
        { "foo/", "foo", },
        { "foo/bar", "foo/bar", },
        { "foo/bar/", "foo/bar", },

        { "/foo", "/foo", },
        { "/foo/bar", "/foo/bar", },
        { "/foo/bar/", "/foo/bar", },

        { "///foo", "/foo", }, /* NO_CHECK_STYLE */
        { "///foo///bar", "/foo/bar", }, /* NO_CHECK_STYLE */
        { "///foo///bar///", "/foo/bar", }, /* NO_CHECK_STYLE */

        { NULL, NULL }
    };
    struct test *t;

    for (t = &tests[0]; t->in != NULL; t++) {
        atf_fs_path_t p;

        printf("Input          : >%s<\n", t->in);
        printf("Expected output: >%s<\n", t->out);

        CE(atf_fs_path_init_fmt(&p, "%s", t->in));
        printf("Output         : >%s<\n", atf_fs_path_cstring(&p));
        ATF_CHECK(strcmp(atf_fs_path_cstring(&p), t->out) == 0);
        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_copy);
ATF_TC_HEAD(path_copy, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_path_copy constructor");
}
ATF_TC_BODY(path_copy, tc)
{
    atf_fs_path_t str, str2;

    CE(atf_fs_path_init_fmt(&str, "foo"));
    CE(atf_fs_path_copy(&str2, &str));

    ATF_CHECK(atf_equal_fs_path_fs_path(&str, &str2));

    CE(atf_fs_path_append_fmt(&str2, "bar"));

    ATF_CHECK(!atf_equal_fs_path_fs_path(&str, &str2));

    atf_fs_path_fini(&str2);
    atf_fs_path_fini(&str);
}

ATF_TC(path_is_absolute);
ATF_TC_HEAD(path_is_absolute, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the path::is_absolute function");
}
ATF_TC_BODY(path_is_absolute, tc)
{
    struct test {
        const char *in;
        bool abs;
    } tests[] = {
        { "/", true },
        { "////", true }, /* NO_CHECK_STYLE */
        { "////a", true }, /* NO_CHECK_STYLE */
        { "//a//", true }, /* NO_CHECK_STYLE */
        { "a////", false }, /* NO_CHECK_STYLE */
        { "../foo", false },
        { NULL, false },
    };
    struct test *t;

    for (t = &tests[0]; t->in != NULL; t++) {
        atf_fs_path_t p;

        printf("Input          : %s\n", t->in);
        printf("Expected result: %s\n", t->abs ? "true" : "false");

        CE(atf_fs_path_init_fmt(&p, "%s", t->in));
        printf("Result         : %s\n",
               atf_fs_path_is_absolute(&p) ? "true" : "false");
        if (t->abs)
            ATF_CHECK(atf_fs_path_is_absolute(&p));
        else
            ATF_CHECK(!atf_fs_path_is_absolute(&p));
        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_is_root);
ATF_TC_HEAD(path_is_root, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the path::is_root function");
}
ATF_TC_BODY(path_is_root, tc)
{
    struct test {
        const char *in;
        bool root;
    } tests[] = {
        { "/", true },
        { "////", true }, /* NO_CHECK_STYLE */
        { "////a", false }, /* NO_CHECK_STYLE */
        { "//a//", false }, /* NO_CHECK_STYLE */
        { "a////", false }, /* NO_CHECK_STYLE */
        { "../foo", false },
        { NULL, false },
    };
    struct test *t;

    for (t = &tests[0]; t->in != NULL; t++) {
        atf_fs_path_t p;

        printf("Input          : %s\n", t->in);
        printf("Expected result: %s\n", t->root ? "true" : "false");

        CE(atf_fs_path_init_fmt(&p, "%s", t->in));
        printf("Result         : %s\n",
               atf_fs_path_is_root(&p) ? "true" : "false");
        if (t->root)
            ATF_CHECK(atf_fs_path_is_root(&p));
        else
            ATF_CHECK(!atf_fs_path_is_root(&p));
        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_branch_path);
ATF_TC_HEAD(path_branch_path, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_path_branch_path "
                      "function");
}
ATF_TC_BODY(path_branch_path, tc)
{
    struct test {
        const char *in;
        const char *branch;
    } tests[] = {
        { ".", "." },
        { "foo", "." },
        { "foo/bar", "foo" },
        { "/foo", "/" },
        { "/foo/bar", "/foo" },
        { NULL, NULL },
    };
    struct test *t;

    for (t = &tests[0]; t->in != NULL; t++) {
        atf_fs_path_t p, bp;

        printf("Input          : %s\n", t->in);
        printf("Expected output: %s\n", t->branch);

        CE(atf_fs_path_init_fmt(&p, "%s", t->in));
        CE(atf_fs_path_branch_path(&p, &bp));
        printf("Output         : %s\n", atf_fs_path_cstring(&bp));
        ATF_CHECK(strcmp(atf_fs_path_cstring(&bp), t->branch) == 0);
        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_leaf_name);
ATF_TC_HEAD(path_leaf_name, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_path_leaf_name "
                      "function");
}
ATF_TC_BODY(path_leaf_name, tc)
{
    struct test {
        const char *in;
        const char *leaf;
    } tests[] = {
        { ".", "." },
        { "foo", "foo" },
        { "foo/bar", "bar" },
        { "/foo", "foo" },
        { "/foo/bar", "bar" },
        { NULL, NULL },
    };
    struct test *t;

    for (t = &tests[0]; t->in != NULL; t++) {
        atf_fs_path_t p;
        atf_dynstr_t ln;

        printf("Input          : %s\n", t->in);
        printf("Expected output: %s\n", t->leaf);

        CE(atf_fs_path_init_fmt(&p, "%s", t->in));
        CE(atf_fs_path_leaf_name(&p, &ln));
        printf("Output         : %s\n", atf_dynstr_cstring(&ln));
        ATF_CHECK(atf_equal_dynstr_cstring(&ln, t->leaf));
        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_append);
ATF_TC_HEAD(path_append, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the concatenation of multiple "
                      "paths");
}
ATF_TC_BODY(path_append, tc)
{
    struct test {
        const char *in;
        const char *ap;
        const char *out;
    } tests[] = {
        { "foo", "bar", "foo/bar" },
        { "foo/", "/bar", "foo/bar" },
        { "foo/", "/bar/baz", "foo/bar/baz" },
        { "foo/", "///bar///baz", "foo/bar/baz" }, /* NO_CHECK_STYLE */

        { NULL, NULL, NULL }
    };
    struct test *t;

    for (t = &tests[0]; t->in != NULL; t++) {
        atf_fs_path_t p;

        printf("Input          : >%s<\n", t->in);
        printf("Append         : >%s<\n", t->ap);
        printf("Expected output: >%s<\n", t->out);

        CE(atf_fs_path_init_fmt(&p, "%s", t->in));

        CE(atf_fs_path_append_fmt(&p, "%s", t->ap));

        printf("Output         : >%s<\n", atf_fs_path_cstring(&p));
        ATF_CHECK(strcmp(atf_fs_path_cstring(&p), t->out) == 0);

        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_to_absolute);
ATF_TC_HEAD(path_to_absolute, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_path_to_absolute "
                      "function");
}
ATF_TC_BODY(path_to_absolute, tc)
{
    const char *names[] = { ".", "dir", NULL };
    const char **n;

    ATF_CHECK(mkdir("dir", 0755) != -1);

    for (n = names; *n != NULL; n++) {
        atf_fs_path_t p, p2;
        atf_fs_stat_t st1, st2;

        CE(atf_fs_path_init_fmt(&p, "%s", *n));
        CE(atf_fs_stat_init(&st1, &p));
        printf("Relative path: %s\n", atf_fs_path_cstring(&p));

        CE(atf_fs_path_to_absolute(&p, &p2));
        printf("Absolute path: %s\n", atf_fs_path_cstring(&p2));

        ATF_CHECK(atf_fs_path_is_absolute(&p2));
        CE(atf_fs_stat_init(&st2, &p2));

        ATF_CHECK_EQUAL(atf_fs_stat_get_device(&st1),
                        atf_fs_stat_get_device(&st2));
        ATF_CHECK_EQUAL(atf_fs_stat_get_inode(&st1),
                        atf_fs_stat_get_inode(&st2));

        atf_fs_stat_fini(&st2);
        atf_fs_stat_fini(&st1);
        atf_fs_path_fini(&p2);
        atf_fs_path_fini(&p);

        printf("\n");
    }
}

ATF_TC(path_equal);
ATF_TC_HEAD(path_equal, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the equality operators for paths");
}
ATF_TC_BODY(path_equal, tc)
{
    atf_fs_path_t p1, p2;

    CE(atf_fs_path_init_fmt(&p1, "foo"));

    CE(atf_fs_path_init_fmt(&p2, "foo"));
    ATF_CHECK(atf_equal_fs_path_fs_path(&p1, &p2));
    atf_fs_path_fini(&p2);

    CE(atf_fs_path_init_fmt(&p2, "bar"));
    ATF_CHECK(!atf_equal_fs_path_fs_path(&p1, &p2));
    atf_fs_path_fini(&p2);

    atf_fs_path_fini(&p1);
}

/* ---------------------------------------------------------------------
 * Test cases for the "atf_fs_stat" type.
 * --------------------------------------------------------------------- */

ATF_TC(stat_type);
ATF_TC_HEAD(stat_type, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_stat_get_type function "
                      "and, indirectly, the constructor");
}
ATF_TC_BODY(stat_type, tc)
{
    atf_fs_path_t p;
    atf_fs_stat_t st;

    create_dir("dir", 0755);
    create_file("reg", 0644);

    CE(atf_fs_path_init_fmt(&p, "dir"));
    CE(atf_fs_stat_init(&st, &p));
    ATF_CHECK_EQUAL(atf_fs_stat_get_type(&st), atf_fs_stat_dir_type);
    atf_fs_stat_fini(&st);
    atf_fs_path_fini(&p);

    CE(atf_fs_path_init_fmt(&p, "reg"));
    CE(atf_fs_stat_init(&st, &p));
    ATF_CHECK_EQUAL(atf_fs_stat_get_type(&st), atf_fs_stat_reg_type);
    atf_fs_stat_fini(&st);
    atf_fs_path_fini(&p);
}

ATF_TC(stat_perms);
ATF_TC_HEAD(stat_perms, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_stat_is_* functions");
}
ATF_TC_BODY(stat_perms, tc)
{
    atf_fs_path_t p;
    atf_fs_stat_t st;

    create_file("reg", 0);

    CE(atf_fs_path_init_fmt(&p, "reg"));

#define perms(ur, uw, ux, gr, gw, gx, othr, othw, othx) \
    { \
        CE(atf_fs_stat_init(&st, &p)); \
        ATF_CHECK(atf_fs_stat_is_owner_readable(&st) == ur); \
        ATF_CHECK(atf_fs_stat_is_owner_writable(&st) == uw); \
        ATF_CHECK(atf_fs_stat_is_owner_executable(&st) == ux); \
        ATF_CHECK(atf_fs_stat_is_group_readable(&st) == gr); \
        ATF_CHECK(atf_fs_stat_is_group_writable(&st) == gw); \
        ATF_CHECK(atf_fs_stat_is_group_executable(&st) == gx); \
        ATF_CHECK(atf_fs_stat_is_other_readable(&st) == othr); \
        ATF_CHECK(atf_fs_stat_is_other_writable(&st) == othw); \
        ATF_CHECK(atf_fs_stat_is_other_executable(&st) == othx); \
        atf_fs_stat_fini(&st); \
    }

    chmod("reg", 0000);
    perms(false, false, false, false, false, false, false, false, false);

    chmod("reg", 0001);
    perms(false, false, false, false, false, false, false, false, true);

    chmod("reg", 0010);
    perms(false, false, false, false, false, true, false, false, false);

    chmod("reg", 0100);
    perms(false, false, true, false, false, false, false, false, false);

    chmod("reg", 0002);
    perms(false, false, false, false, false, false, false, true, false);

    chmod("reg", 0020);
    perms(false, false, false, false, true, false, false, false, false);

    chmod("reg", 0200);
    perms(false, true, false, false, false, false, false, false, false);

    chmod("reg", 0004);
    perms(false, false, false, false, false, false, true, false, false);

    chmod("reg", 0040);
    perms(false, false, false, true, false, false, false, false, false);

    chmod("reg", 0400);
    perms(true, false, false, false, false, false, false, false, false);

    chmod("reg", 0644);
    perms(true, true, false, true, false, false, true, false, false);

    chmod("reg", 0755);
    perms(true, true, true, true, false, true, true, false, true);

    chmod("reg", 0777);
    perms(true, true, true, true, true, true, true, true, true);

#undef perms

    atf_fs_path_fini(&p);
}

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(cleanup);
ATF_TC_HEAD(cleanup, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_cleanup function");
}
ATF_TC_BODY(cleanup, tc)
{
    atf_fs_path_t root;

    create_dir ("root", 0755);
    create_dir ("root/dir", 0755);
    create_dir ("root/dir/1", 0755);
    create_file("root/dir/2", 0644);
    create_file("root/reg", 0644);

    CE(atf_fs_path_init_fmt(&root, "root"));
    CE(atf_fs_cleanup(&root));
    ATF_CHECK(not_exists(&root));
    atf_fs_path_fini(&root);

    /* TODO: Cleanup with mount points, just as in tools/t_atf_cleanup. */
}

ATF_TC(exists);
ATF_TC_HEAD(exists, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_exists function");
}
ATF_TC_BODY(exists, tc)
{
    atf_error_t err;
    atf_fs_path_t pdir, pfile;
    bool b;

    CE(atf_fs_path_init_fmt(&pdir, "dir"));
    CE(atf_fs_path_init_fmt(&pfile, "dir/file"));

    create_dir(atf_fs_path_cstring(&pdir), 0755);
    create_file(atf_fs_path_cstring(&pfile), 0644);

    printf("Checking existence of a directory\n");
    CE(atf_fs_exists(&pdir, &b));
    ATF_CHECK(b);

    printf("Checking existence of a file\n");
    CE(atf_fs_exists(&pfile, &b));
    ATF_CHECK(b);

    if (!atf_user_is_root()) {
        printf("Checking existence of a file inside a directory without "
               "permissions\n");
        ATF_CHECK(chmod(atf_fs_path_cstring(&pdir), 0000) != -1);
        err = atf_fs_exists(&pfile, &b);
        ATF_CHECK(atf_is_error(err));
        ATF_CHECK(atf_error_is(err, "libc"));
        ATF_CHECK(chmod(atf_fs_path_cstring(&pdir), 0755) != -1);
    }

    printf("Checking existence of a non-existent file\n");
    ATF_CHECK(unlink(atf_fs_path_cstring(&pfile)) != -1);
    CE(atf_fs_exists(&pfile, &b));
    ATF_CHECK(!b);
}

ATF_TC(eaccess);
ATF_TC_HEAD(eaccess, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_eaccess function");
}
ATF_TC_BODY(eaccess, tc)
{
    const int modes[] = { atf_fs_access_f, atf_fs_access_r, atf_fs_access_w,
                          atf_fs_access_x, 0 };
    const int *m;
    struct tests {
        mode_t fmode;
        int amode;
        int uerror;
        int rerror;
    } tests[] = {
        { 0000, atf_fs_access_r, EACCES, 0 },
        { 0000, atf_fs_access_w, EACCES, 0 },
        { 0000, atf_fs_access_x, EACCES, EACCES },

        { 0001, atf_fs_access_r, EACCES, 0 },
        { 0001, atf_fs_access_w, EACCES, 0 },
        { 0001, atf_fs_access_x, EACCES, 0 },
        { 0002, atf_fs_access_r, EACCES, 0 },
        { 0002, atf_fs_access_w, EACCES, 0 },
        { 0002, atf_fs_access_x, EACCES, EACCES },
        { 0004, atf_fs_access_r, EACCES, 0 },
        { 0004, atf_fs_access_w, EACCES, 0 },
        { 0004, atf_fs_access_x, EACCES, EACCES },

        { 0010, atf_fs_access_r, EACCES, 0 },
        { 0010, atf_fs_access_w, EACCES, 0 },
        { 0010, atf_fs_access_x, 0,      0 },
        { 0020, atf_fs_access_r, EACCES, 0 },
        { 0020, atf_fs_access_w, 0,      0 },
        { 0020, atf_fs_access_x, EACCES, EACCES },
        { 0040, atf_fs_access_r, 0,      0 },
        { 0040, atf_fs_access_w, EACCES, 0 },
        { 0040, atf_fs_access_x, EACCES, EACCES },

        { 0100, atf_fs_access_r, EACCES, 0 },
        { 0100, atf_fs_access_w, EACCES, 0 },
        { 0100, atf_fs_access_x, 0,      0 },
        { 0200, atf_fs_access_r, EACCES, 0 },
        { 0200, atf_fs_access_w, 0,      0 },
        { 0200, atf_fs_access_x, EACCES, EACCES },
        { 0400, atf_fs_access_r, 0,      0 },
        { 0400, atf_fs_access_w, EACCES, 0 },
        { 0400, atf_fs_access_x, EACCES, EACCES },

        { 0, 0, 0, 0 }
    };
    struct tests *t;
    atf_fs_path_t p;
    atf_error_t err;

    CE(atf_fs_path_init_fmt(&p, "the-file"));

    printf("Non-existent file checks\n");
    for (m = &modes[0]; *m != 0; m++) {
        err = atf_fs_eaccess(&p, *m);
        ATF_CHECK(atf_is_error(err));
        ATF_CHECK(atf_error_is(err, "libc"));
        ATF_CHECK_EQUAL(atf_libc_error_code(err), ENOENT);
        atf_error_free(err);
    }

    create_file(atf_fs_path_cstring(&p), 0000);
    ATF_CHECK(chown(atf_fs_path_cstring(&p), geteuid(), getegid()) != -1);

    for (t = &tests[0]; t->amode != 0; t++) {
        const int experr = atf_user_is_root() ? t->rerror : t->uerror;

        printf("\n");
        printf("File mode     : %04o\n", t->fmode);
        printf("Access mode   : 0x%02x\n", t->amode);

        ATF_CHECK(chmod(atf_fs_path_cstring(&p), t->fmode) != -1);

        /* First, existence check. */
        err = atf_fs_eaccess(&p, atf_fs_access_f);
        ATF_CHECK(!atf_is_error(err));

        /* Now do the specific test case. */
        printf("Expected error: %d\n", experr);
        err = atf_fs_eaccess(&p, t->amode);
        if (atf_is_error(err)) {
            if (atf_error_is(err, "libc"))
                printf("Error         : %d\n", atf_libc_error_code(err));
            else
                printf("Error         : Non-libc error\n");
        } else
                printf("Error         : None\n");
        if (experr == 0) {
            ATF_CHECK(!atf_is_error(err));
        } else {
            ATF_CHECK(atf_is_error(err));
            ATF_CHECK(atf_error_is(err, "libc"));
            ATF_CHECK_EQUAL(atf_libc_error_code(err), experr);
            atf_error_free(err);
        }
    }

    atf_fs_path_fini(&p);
}

ATF_TC(getcwd);
ATF_TC_HEAD(getcwd, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_getcwd function");
}
ATF_TC_BODY(getcwd, tc)
{
    atf_fs_path_t cwd1, cwd2;

    create_dir ("root", 0755);

    CE(atf_fs_getcwd(&cwd1));
    ATF_CHECK(chdir("root") != -1);
    CE(atf_fs_getcwd(&cwd2));

    CE(atf_fs_path_append_fmt(&cwd1, "root"));

    ATF_CHECK(atf_equal_fs_path_fs_path(&cwd1, &cwd2));

    atf_fs_path_fini(&cwd2);
    atf_fs_path_fini(&cwd1);
}

ATF_TC(mkdtemp);
ATF_TC_HEAD(mkdtemp, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_fs_mkdtemp function");
}
ATF_TC_BODY(mkdtemp, tc)
{
    atf_fs_path_t p1, p2;
    atf_fs_stat_t s1, s2;

    CE(atf_fs_path_init_fmt(&p1, "testdir.XXXXXX"));
    CE(atf_fs_path_init_fmt(&p2, "testdir.XXXXXX"));
    CE(atf_fs_mkdtemp(&p1));
    CE(atf_fs_mkdtemp(&p2));
    ATF_CHECK(!atf_equal_fs_path_fs_path(&p1, &p2));
    ATF_CHECK(exists(&p1));
    ATF_CHECK(exists(&p2));

    CE(atf_fs_stat_init(&s1, &p1));
    ATF_CHECK( atf_fs_stat_is_owner_readable(&s1));
    ATF_CHECK( atf_fs_stat_is_owner_writable(&s1));
    ATF_CHECK( atf_fs_stat_is_owner_executable(&s1));
    ATF_CHECK(!atf_fs_stat_is_group_readable(&s1));
    ATF_CHECK(!atf_fs_stat_is_group_writable(&s1));
    ATF_CHECK(!atf_fs_stat_is_group_executable(&s1));
    ATF_CHECK(!atf_fs_stat_is_other_readable(&s1));
    ATF_CHECK(!atf_fs_stat_is_other_writable(&s1));
    ATF_CHECK(!atf_fs_stat_is_other_executable(&s1));

    CE(atf_fs_stat_init(&s2, &p2));
    ATF_CHECK( atf_fs_stat_is_owner_readable(&s2));
    ATF_CHECK( atf_fs_stat_is_owner_writable(&s2));
    ATF_CHECK( atf_fs_stat_is_owner_executable(&s2));
    ATF_CHECK(!atf_fs_stat_is_group_readable(&s2));
    ATF_CHECK(!atf_fs_stat_is_group_writable(&s2));
    ATF_CHECK(!atf_fs_stat_is_group_executable(&s2));
    ATF_CHECK(!atf_fs_stat_is_other_readable(&s2));
    ATF_CHECK(!atf_fs_stat_is_other_writable(&s2));
    ATF_CHECK(!atf_fs_stat_is_other_executable(&s2));

    atf_fs_stat_fini(&s2);
    atf_fs_stat_fini(&s1);
    atf_fs_path_fini(&p2);
    atf_fs_path_fini(&p1);
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    /* Add the tests for the "atf_fs_path" type. */
    ATF_TP_ADD_TC(tp, path_normalize);
    ATF_TP_ADD_TC(tp, path_copy);
    ATF_TP_ADD_TC(tp, path_is_absolute);
    ATF_TP_ADD_TC(tp, path_is_root);
    ATF_TP_ADD_TC(tp, path_branch_path);
    ATF_TP_ADD_TC(tp, path_leaf_name);
    ATF_TP_ADD_TC(tp, path_append);
    ATF_TP_ADD_TC(tp, path_to_absolute);
    ATF_TP_ADD_TC(tp, path_equal);

    /* Add the tests for the "atf_fs_stat" type. */
    ATF_TP_ADD_TC(tp, stat_type);
    ATF_TP_ADD_TC(tp, stat_perms);

    /* Add the tests for the free functions. */
    ATF_TP_ADD_TC(tp, cleanup);
    ATF_TP_ADD_TC(tp, eaccess);
    ATF_TP_ADD_TC(tp, exists);
    ATF_TP_ADD_TC(tp, getcwd);
    ATF_TP_ADD_TC(tp, mkdtemp);

    return atf_no_error();
}
