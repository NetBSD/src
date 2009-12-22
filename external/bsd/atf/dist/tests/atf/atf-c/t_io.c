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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/dynstr.h"
#include "atf-c/io.h"

#include "h_lib.h"

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(readline);
ATF_TC_HEAD(readline, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_io_readline function");
}
ATF_TC_BODY(readline, tc)
{
    const char *l1 = "First line with % formatting % characters %";
    const char *l2 = "Second line; much longer than the first one";
    const char *l3 = "Last line, without terminator";

    {
        FILE *f;

        f = fopen("test", "w");
        ATF_REQUIRE(f != NULL);
        fclose(f);
    }

    {
        int fd;
        atf_dynstr_t dest;
        bool eof;

        fd = open("test", O_RDONLY);
        ATF_REQUIRE(fd != -1);

        RE(atf_dynstr_init(&dest));
        RE(atf_io_readline(fd, &dest, &eof));
        ATF_REQUIRE(eof);
        atf_dynstr_fini(&dest);
    }

    {
        FILE *f;

        f = fopen("test", "w");
        ATF_REQUIRE(f != NULL);

        fprintf(f, "%s\n", l1);
        fprintf(f, "%s\n", l2);
        fprintf(f, "%s", l3);

        fclose(f);
    }

    {
        int fd;
        atf_dynstr_t dest;
        bool eof;

        fd = open("test", O_RDONLY);
        ATF_REQUIRE(fd != -1);

        RE(atf_dynstr_init(&dest));
        RE(atf_io_readline(fd, &dest, &eof));
        ATF_REQUIRE(!eof);
        printf("1st line: >%s<\n", atf_dynstr_cstring(&dest));
        ATF_REQUIRE(atf_equal_dynstr_cstring(&dest, l1));
        atf_dynstr_fini(&dest);

        RE(atf_dynstr_init(&dest));
        RE(atf_io_readline(fd, &dest, &eof));
        ATF_REQUIRE(!eof);
        printf("2nd line: >%s<\n", atf_dynstr_cstring(&dest));
        ATF_REQUIRE(atf_equal_dynstr_cstring(&dest, l2));
        atf_dynstr_fini(&dest);

        RE(atf_dynstr_init(&dest));
        RE(atf_io_readline(fd, &dest, &eof));
        ATF_REQUIRE(eof);
        printf("3rd line: >%s<\n", atf_dynstr_cstring(&dest));
        ATF_REQUIRE(atf_equal_dynstr_cstring(&dest, l3));
        atf_dynstr_fini(&dest);

        close(fd);
    }
}

ATF_TC(write_fmt);
ATF_TC_HEAD(write_fmt, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_io_write_fmt function");
}
ATF_TC_BODY(write_fmt, tc)
{
    {
        int fd;

        fd = open("test", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ATF_REQUIRE(fd != -1);

        RE(atf_io_write_fmt(fd, "Plain string\n"));
        RE(atf_io_write_fmt(fd, "Formatted %s %d\n", "string", 5));

        close(fd);
    }

    {
        int fd;
        char buf[1024];
        ssize_t cnt;

        fd = open("test", O_RDONLY);
        ATF_REQUIRE(fd != -1);

        cnt = read(fd, buf, sizeof(buf));
        ATF_REQUIRE(cnt != -1);
        buf[cnt] = '\0';
        ATF_REQUIRE(strcmp(buf, "Plain string\nFormatted string 5\n") == 0);

        close(fd);
    }
}

ATF_TC(write_fmt_fail);
ATF_TC_HEAD(write_fmt_fail, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_io_write_fmt function "
                      "writing to an invalid file descriptor");
}
ATF_TC_BODY(write_fmt_fail, tc)
{
    const int fd = 16;
    atf_error_t err;
    char buf[1024];

    close(fd);

    err = atf_io_write_fmt(fd, "Plain string\n");
    ATF_REQUIRE(atf_error_is(err, "libc"));
    ATF_REQUIRE_EQ(atf_libc_error_code(err), EBADF);
    atf_error_format(err, buf, sizeof(buf));
    ATF_REQUIRE(strstr(buf, "Plain string") != NULL);
    atf_error_free(err);
}

/* ---------------------------------------------------------------------
 * Tests cases for the header file.
 * --------------------------------------------------------------------- */

HEADER_TC(include, "atf-c/io.h", "d_include_io_h.c");

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, readline);
    ATF_TP_ADD_TC(tp, write_fmt);
    ATF_TP_ADD_TC(tp, write_fmt_fail);

    /* Add the test cases for the header file. */
    ATF_TP_ADD_TC(tp, include);

    return atf_no_error();
}
