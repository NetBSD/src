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

#if defined(HAVE_CONFIG_H)
#include "bconfig.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/dynstr.h"
#include "atf-c/io.h"
#include "atf-c/sanity.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

enum type { inv, pre, post, unreachable };

static
bool
grep(const atf_dynstr_t *line, const char *text)
{
    const char *l = atf_dynstr_cstring(line);
    bool found;

    found = false;

    if (strstr(l, text) != NULL)
        found = true;

    return found;
}

static
void
do_test(enum type t, bool cond)
{
    int fds[2];
    pid_t pid;

    ATF_CHECK(pipe(fds) != -1);

    ATF_CHECK((pid = fork()) != -1);
    if (pid == 0) {
        ATF_CHECK(dup2(fds[1], STDERR_FILENO) != -1);
        close(fds[0]);
        close(fds[1]);

        switch (t) {
        case inv:
            INV(cond);
            break;

        case pre:
            PRE(cond);
            break;

        case post:
            POST(cond);
            break;

        case unreachable:
            if (!cond)
                UNREACHABLE;
            break;
        }

        exit(EXIT_SUCCESS);
    } else {
        int ecode, i;
        atf_dynstr_t lines[3];
        atf_error_t err;

        close(fds[1]);

        i = 0;
        do {
            err = atf_dynstr_init(&lines[i]);
            if (!atf_is_error(err)) {
                err = atf_io_readline(fds[0], &lines[i]);
                if (!atf_is_error(err))
                    i++;
            }
        } while (i < 3 && !atf_is_error(err));
        ATF_CHECK(i == 3);
        i--;

        if (!cond) {
            switch (t) {
            case inv:
                ATF_CHECK(grep(&lines[0], "Invariant"));
                break;

            case pre:
                ATF_CHECK(grep(&lines[0], "Precondition"));
                break;

            case post:
                ATF_CHECK(grep(&lines[0], "Postcondition"));
                break;

            case unreachable:
                ATF_CHECK(grep(&lines[0], "Invariant"));
                break;
            }

            ATF_CHECK(grep(&lines[0], __FILE__));
            ATF_CHECK(grep(&lines[2], PACKAGE_BUGREPORT));
        }

        ATF_CHECK(waitpid(pid, &ecode, 0) != -1);
        if (!cond) {
            ATF_CHECK(WIFSIGNALED(ecode));
            ATF_CHECK(WTERMSIG(ecode) == SIGABRT);
        } else {
            ATF_CHECK(WIFEXITED(ecode));
            ATF_CHECK(WEXITSTATUS(ecode) == EXIT_SUCCESS);
        }
    }
}

static
void
require_ndebug(void)
{
#if defined(NDEBUG)
    atf_tc_skip("Sanity checks not available; code built with -DNDEBUG");
#endif
}

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(inv);
ATF_TC_HEAD(inv, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the INV macro");
}
ATF_TC_BODY(inv, tc)
{
    require_ndebug();

    do_test(inv, false);
    do_test(inv, true);
}

ATF_TC(pre);
ATF_TC_HEAD(pre, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the PRE macro");
}
ATF_TC_BODY(pre, tc)
{
    require_ndebug();

    do_test(pre, false);
    do_test(pre, true);
}

ATF_TC(post);
ATF_TC_HEAD(post, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the POST macro");
}
ATF_TC_BODY(post, tc)
{
    require_ndebug();

    do_test(post, false);
    do_test(post, true);
}

ATF_TC(unreachable);
ATF_TC_HEAD(unreachable, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the UNREACHABLE macro");
}
ATF_TC_BODY(unreachable, tc)
{
    require_ndebug();

    do_test(unreachable, false);
    do_test(unreachable, true);
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, inv);
    ATF_TP_ADD_TC(tp, pre);
    ATF_TP_ADD_TC(tp, post);
    ATF_TP_ADD_TC(tp, unreachable);

    return atf_no_error();
}
