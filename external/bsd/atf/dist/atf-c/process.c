/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "atf-c/error.h"
#include "atf-c/process.h"
#include "atf-c/sanity.h"

/* ---------------------------------------------------------------------
 * The "child" error type.
 * --------------------------------------------------------------------- */

struct child_error_data {
    char m_cmd[4096];
    int m_state;
};
typedef struct child_error_data child_error_data_t;

static
void
child_format(const atf_error_t err, char *buf, size_t buflen)
{
    const child_error_data_t *data;

    PRE(atf_error_is(err, "child"));

    data = atf_error_data(err);
    snprintf(buf, buflen, "Unknown error while executing \"%s\"; "
             "exit state was %d", data->m_cmd, data->m_state);
}

static
atf_error_t
child_error(const char *cmd, int state)
{
    atf_error_t err;
    child_error_data_t data;

    snprintf(data.m_cmd, sizeof(data.m_cmd), "%s", cmd);
    data.m_state = state;

    err = atf_error_new("child", &data, sizeof(data), child_format);

    return err;
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

atf_error_t
atf_process_fork(pid_t *outpid)
{
    atf_error_t err;
    pid_t pid;

    pid = fork();
    if (pid == -1) {
        err = atf_libc_error(errno, "Failed to fork");
        goto out;
    }

    *outpid = pid;
    err = atf_no_error();

out:
    return err;
}

atf_error_t
atf_process_system(const char *cmdline)
{
    atf_error_t err;
    int state;

    state = system(cmdline);
    if (state == -1)
        err = atf_libc_error(errno, "Failed to run \"%s\"", cmdline);
    else if (!WIFEXITED(state) || WEXITSTATUS(state) != EXIT_SUCCESS)
        err = child_error(cmdline, state);
    else
        err = atf_no_error();

    return err;
}
