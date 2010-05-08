/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2008, 2009, 2010 The NetBSD Foundation, Inc.
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
#include <regex.h>
#include <unistd.h>

#include "atf-c/build.h"
#include "atf-c/check.h"
#include "atf-c/config.h"
#include "atf-c/dynstr.h"
#include "atf-c/error.h"
#include "atf-c/fs.h"
#include "atf-c/io.h"
#include "atf-c/macros.h"

#include "h_lib.h"

void
build_check_c_o(const atf_tc_t *tc, const char *sfile, const char *failmsg)
{
    bool success;
    atf_fs_path_t path;
    atf_dynstr_t iflag;
    const char *optargs[2];

    RE(atf_dynstr_init_fmt(&iflag, "-I%s", atf_config_get("atf_includedir")));

    optargs[0] = atf_dynstr_cstring(&iflag);
    optargs[1] = NULL;

    RE(atf_fs_path_init_fmt(&path, "%s/%s",
                            atf_tc_get_config_var(tc, "srcdir"), sfile));

    RE(atf_check_build_c_o(atf_fs_path_cstring(&path), "test.o", optargs,
                           &success));

    atf_fs_path_fini(&path);
    atf_dynstr_fini(&iflag);

    if (!success)
        atf_tc_fail(failmsg);
}

void
get_h_processes_path(const atf_tc_t *tc, atf_fs_path_t *path)
{
    RE(atf_fs_path_init_fmt(path, "%s/h_processes",
                            atf_tc_get_config_var(tc, "srcdir")));
}

bool
grep_string(const atf_dynstr_t *str, const char *regex)
{
    int res;
    regex_t preg;

    printf("Looking for '%s' in '%s'\n", regex, atf_dynstr_cstring(str));
    ATF_REQUIRE(regcomp(&preg, regex, REG_EXTENDED) == 0);

    res = regexec(&preg, atf_dynstr_cstring(str), 0, NULL, 0);
    ATF_REQUIRE(res == 0 || res == REG_NOMATCH);

    regfree(&preg);

    return res == 0;
}

bool
grep_file(const char *file, const char *regex, ...)
{
    bool done, found;
    int fd;
    va_list ap;
    atf_dynstr_t formatted;

    va_start(ap, regex);
    RE(atf_dynstr_init_ap(&formatted, regex, ap));
    va_end(ap);

    done = false;
    found = false;
    ATF_REQUIRE((fd = open(file, O_RDONLY)) != -1);
    do {
        atf_error_t err;
        atf_dynstr_t line;
        bool eof;

        RE(atf_dynstr_init(&line));

        err = atf_io_readline(fd, &line, &eof);
        done = atf_is_error(err) || eof;
        if (!done)
            found = grep_string(&line, atf_dynstr_cstring(&formatted));

        atf_dynstr_fini(&line);
    } while (!found && !done);
    close(fd);

    atf_dynstr_fini(&formatted);

    return found;
}

struct run_h_tc_data {
    atf_tc_t *m_tc;
    const char *m_resname;
};

static
void
run_h_tc_child(void *v)
{
    struct run_h_tc_data *data = (struct run_h_tc_data *)v;
    atf_fs_path_t respath;

    RE(atf_fs_path_init_fmt(&respath, data->m_resname));
    RE(atf_tc_run(data->m_tc, &respath));
    atf_fs_path_fini(&respath);
}

/* TODO: Investigate if it's worth to add this functionality as part of
 * the public API.  I.e. a function to easily run a test case body in a
 * subprocess. */
void
run_h_tc(atf_tc_t *tc, const char *outname, const char *errname,
         const char *resname)
{
    atf_fs_path_t outpath, errpath;
    atf_process_stream_t outb, errb;
    atf_process_child_t child;
    atf_process_status_t status;

    RE(atf_fs_path_init_fmt(&outpath, outname));
    RE(atf_fs_path_init_fmt(&errpath, errname));

    struct run_h_tc_data data = { tc, resname };

    RE(atf_process_stream_init_redirect_path(&outb, &outpath));
    RE(atf_process_stream_init_redirect_path(&errb, &errpath));
    RE(atf_process_fork(&child, run_h_tc_child, &outb, &errb, &data));
    atf_process_stream_fini(&errb);
    atf_process_stream_fini(&outb);

    RE(atf_process_child_wait(&child, &status));
    ATF_CHECK(atf_process_status_exited(&status));
    atf_process_status_fini(&status);

    atf_fs_path_fini(&errpath);
    atf_fs_path_fini(&outpath);
}
