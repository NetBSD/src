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

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "atf-c/dynstr.h"
#include "atf-c/error.h"
#include "atf-c/fs.h"
#include "atf-c/io.h"
#include "atf-c/sanity.h"

atf_error_t
atf_io_readline(int fd, atf_dynstr_t *dest)
{
    char ch[2];
    ssize_t cnt;
    atf_error_t err;

    ch[1] = '\0';
    while ((cnt = read(fd, &ch[0], sizeof(ch[0]))) == sizeof(ch[0]) &&
           ch[0] != '\n') {
        atf_dynstr_append_fmt(dest, ch);
    }

    if (cnt == -1)
        err = atf_libc_error(errno, "Failed to read line from file "
                             "descriptor %d", fd);
    else
        err = atf_no_error();

    return err;
}

atf_error_t
atf_io_write_ap(int fd, const char *fmt, va_list ap)
{
    atf_error_t err;
    atf_dynstr_t str;

    err = atf_dynstr_init_ap(&str, fmt, ap);
    if (!atf_is_error(err)) {
        ssize_t cnt = write(fd, atf_dynstr_cstring(&str),
                            atf_dynstr_length(&str));

        if (cnt == -1)
            err = atf_libc_error(errno, "Failed to write '%s' to file "
                                 "descriptor %d", atf_dynstr_cstring(&str),
                                 fd);
        else {
            INV((size_t)cnt == atf_dynstr_length(&str));
            err = atf_no_error();
        }

        atf_dynstr_fini(&str);
    }

    return err;
}

atf_error_t
atf_io_write_fmt(int fd, const char *fmt, ...)
{
    atf_error_t err;
    va_list ap;

    va_start(ap, fmt);
    err = atf_io_write_ap(fd, fmt, ap);
    va_end(ap);

    return err;
}

atf_error_t
atf_io_cmp(int *result, const atf_fs_path_t *p1, const atf_fs_path_t *p2)
{
    int fd1, fd2, res;
    const char *path1, *path2;
    atf_error_t err;

    err = atf_no_error();
    res = 1;
    path1 = atf_fs_path_cstring(p1);
    path2 = atf_fs_path_cstring(p2);

    fd1 = open(path1, O_RDONLY);
    if (fd1 == -1) {
        err = atf_libc_error(errno, "Cannot open file: %s", path1);
        goto out_fd1;
    }

    fd2 = open(path2, O_RDONLY);
    if (fd2 == -1) {
        err = atf_libc_error(errno, "Cannot open file: %s", path2);
        goto out_fd2;
    }

    for (;;) {
        ssize_t r1, r2;
        char buf1[512], buf2[512];

        r1 = read(fd1, buf1, sizeof(buf1));
        if (r1 < 0) {
            err = atf_libc_error(errno, "Cannot read file: %s", path1);
            break;
        }

        r2 = read(fd2, buf2, sizeof(buf2));
        if (r2 < 0) {
            err = atf_libc_error(errno, "Cannot read file: %s", path2);
            break;
        }

        if ((r1 == 0) && (r2 == 0)) {
            res = 0;
            break;
        }

        if ((r1 != r2) || (memcmp(buf1, buf2, r1) != 0)) {
            break;
        }
    }

    close(fd2);

out_fd2:
    close(fd1);

out_fd1:

    if (err == atf_no_error())
        *result = res;

    return err;
}
