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

#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "atf-c/build.h"
#include "atf-c/check.h"
#include "atf-c/config.h"
#include "atf-c/defs.h"
#include "atf-c/dynstr.h"
#include "atf-c/error.h"
#include "atf-c/fs.h"
#include "atf-c/list.h"
#include "atf-c/process.h"
#include "atf-c/sanity.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
atf_error_t
create_tmpdir(atf_fs_path_t *dir)
{
    atf_error_t err;

    err = atf_fs_path_init_fmt(dir, "%s/check.XXXXXX",
                               atf_config_get("atf_workdir"));
    if (atf_is_error(err))
        goto out;

    err = atf_fs_mkdtemp(dir);
    if (atf_is_error(err)) {
        atf_fs_path_fini(dir);
        goto out;
    }

    INV(!atf_is_error(err));
out:
    return err;
}

static
void
cleanup_tmpdir(const atf_fs_path_t *dir, const atf_fs_path_t *outfile,
               const atf_fs_path_t *errfile)
{
    {
        atf_error_t err = atf_fs_unlink(outfile);
        if (atf_is_error(err)) {
            INV(atf_error_is(err, "libc") &&
                atf_libc_error_code(err) == ENOENT);
            atf_error_free(err);
        } else
            INV(!atf_is_error(err));
    }

    {
        atf_error_t err = atf_fs_unlink(errfile);
        if (atf_is_error(err)) {
            INV(atf_error_is(err, "libc") &&
                atf_libc_error_code(err) == ENOENT);
            atf_error_free(err);
        } else
            INV(!atf_is_error(err));
    }

    {
        atf_error_t err = atf_fs_rmdir(dir);
        INV(!atf_is_error(err));
    }
}

static
int
const_execvp(const char *file, const char *const *argv)
{
#define UNCONST(a) ((void *)(unsigned long)(const void *)(a))
    return execvp(file, UNCONST(argv));
#undef UNCONST
}

static
atf_error_t
init_sb(const atf_fs_path_t *path, atf_process_stream_t *sb)
{
    atf_error_t err;

    if (path == NULL)
        err = atf_process_stream_init_inherit(sb);
    else
        err = atf_process_stream_init_redirect_path(sb, path);

    return err;
}

static
atf_error_t
init_sbs(const atf_fs_path_t *outfile, atf_process_stream_t *outsb,
         const atf_fs_path_t *errfile, atf_process_stream_t *errsb)
{
    atf_error_t err;

    err = init_sb(outfile, outsb);
    if (atf_is_error(err))
        goto out;

    err = init_sb(errfile, errsb);
    if (atf_is_error(err)) {
        atf_process_stream_fini(outsb);
        goto out;
    }

out:
    return err;
}

struct exec_data {
    const char *const *m_argv;
};

static void exec_child(void *) ATF_DEFS_ATTRIBUTE_NORETURN;

static
void
exec_child(void *v)
{
    struct exec_data *ea = v;

    atf_reset_exit_checks();
    const_execvp(ea->m_argv[0], ea->m_argv);
    fprintf(stderr, "execvp(%s) failed: %s\n", ea->m_argv[0], strerror(errno));
    exit(127);
}

static
atf_error_t
fork_and_wait(const char *const *argv, const atf_fs_path_t *outfile,
              const atf_fs_path_t *errfile, atf_process_status_t *status)
{
    atf_error_t err;
    atf_process_child_t child;
    atf_process_stream_t outsb, errsb;
    struct exec_data ea = { argv };

    err = init_sbs(outfile, &outsb, errfile, &errsb);
    if (atf_is_error(err))
        goto out;

    err = atf_process_fork(&child, exec_child, &outsb, &errsb, &ea);
    if (atf_is_error(err))
        goto out_sbs;

    err = atf_process_child_wait(&child, status);

out_sbs:
    atf_process_stream_fini(&errsb);
    atf_process_stream_fini(&outsb);
out:
    return err;
}

static
void
update_success_from_status(const char *progname,
                           const atf_process_status_t *status, bool *success)
{
    bool s = atf_process_status_exited(status) &&
             atf_process_status_exitstatus(status) == EXIT_SUCCESS;

    if (atf_process_status_exited(status)) {
        if (atf_process_status_exitstatus(status) == EXIT_SUCCESS)
            INV(s);
        else {
            INV(!s);
            fprintf(stderr, "%s failed with exit code %d\n", progname,
                    atf_process_status_exitstatus(status));
        }
    } else if (atf_process_status_signaled(status)) {
        INV(!s);
        fprintf(stderr, "%s failed due to signal %d%s\n", progname,
                atf_process_status_termsig(status),
                atf_process_status_coredump(status) ? " (core dumped)" : "");
    } else {
        INV(!s);
        fprintf(stderr, "%s failed due to unknown reason\n", progname);
    }

    *success = s;
}

static
atf_error_t
array_to_list(const char *const *a, atf_list_t *l)
{
    atf_error_t err;

    err = atf_list_init(l);
    if (atf_is_error(err))
        goto out;

    while (*a != NULL) {
        char *item = strdup(*a);
        if (item == NULL) {
            err = atf_no_memory_error();
            goto out;
        }

        err = atf_list_append(l, item, true);
        if (atf_is_error(err))
            goto out;

        a++;
    }

out:
    return err;
}

static
atf_error_t
list_to_array(const atf_list_t *l, const char ***ap)
{
    atf_error_t err;
    const char **a;

    a = (const char **)malloc((atf_list_size(l) + 1) * sizeof(const char *));
    if (a == NULL)
        err = atf_no_memory_error();
    else {
        const char **aiter;
        atf_list_citer_t liter;

        aiter = a;
        atf_list_for_each_c(liter, l) {
            *aiter = (const char *)atf_list_citer_data(liter);
            aiter++;
        }
        *aiter = NULL;

        err = atf_no_error();
        *ap = a;
    }

    return err;
}

static
void
print_list(const atf_list_t *l, const char *pfx)
{
    atf_list_citer_t iter;

    printf("%s", pfx);
    atf_list_for_each_c(iter, l)
        printf(" %s", (const char *)atf_list_citer_data(iter));
    printf("\n");
}

static
atf_error_t
fork_and_wait_list(const atf_list_t *argvl, const atf_fs_path_t *outfile,
                   const atf_fs_path_t *errfile, atf_process_status_t *status)
{
    atf_error_t err;
    const char **argva;

    err = list_to_array(argvl, &argva);
    if (atf_is_error(err))
        goto out;

    err = fork_and_wait(argva, outfile, errfile, status);

    free(argva);
out:
    return err;
}

static
atf_error_t
check_build_run(const atf_list_t *argv, bool *success)
{
    atf_error_t err;
    atf_process_status_t status;

    print_list(argv, ">");

    err = fork_and_wait_list(argv, NULL, NULL, &status);
    if (atf_is_error(err))
        goto out;

    update_success_from_status((const char *)atf_list_index_c(argv, 0),
                               &status, success);
    atf_process_status_fini(&status);

    INV(!atf_is_error(err));
out:
    return err;
}

/* ---------------------------------------------------------------------
 * The "atf_check_result" type.
 * --------------------------------------------------------------------- */

static
atf_error_t
atf_check_result_init(atf_check_result_t *r, const char *const *argv,
                      const atf_fs_path_t *dir)
{
    atf_error_t err;
    const char *workdir;

    atf_object_init(&r->m_object);

    workdir = atf_config_get("atf_workdir");

    err = array_to_list(argv, &r->m_argv);
    if (atf_is_error(err))
        goto out;

    err = atf_fs_path_copy(&r->m_dir, dir);
    if (atf_is_error(err))
        goto err_argv;

    err = atf_fs_path_init_fmt(&r->m_stdout, "%s/stdout",
                               atf_fs_path_cstring(dir));
    if (atf_is_error(err))
        goto err_dir;

    err = atf_fs_path_init_fmt(&r->m_stderr, "%s/stderr",
                               atf_fs_path_cstring(dir));
    if (atf_is_error(err))
        goto err_stdout;

    INV(!atf_is_error(err));
    goto out;

err_stdout:
    atf_fs_path_fini(&r->m_stdout);
err_dir:
    atf_fs_path_fini(&r->m_dir);
err_argv:
    atf_list_fini(&r->m_argv);
out:
    return err;
}

void
atf_check_result_fini(atf_check_result_t *r)
{
    atf_process_status_fini(&r->m_status);

    cleanup_tmpdir(&r->m_dir, &r->m_stdout, &r->m_stderr);
    atf_fs_path_fini(&r->m_stdout);
    atf_fs_path_fini(&r->m_stderr);
    atf_fs_path_fini(&r->m_dir);

    atf_list_fini(&r->m_argv);

    atf_object_fini(&r->m_object);
}

const atf_list_t *
atf_check_result_argv(const atf_check_result_t *r)
{
    return &r->m_argv;
}

const atf_fs_path_t *
atf_check_result_stdout(const atf_check_result_t *r)
{
    return &r->m_stdout;
}

const atf_fs_path_t *
atf_check_result_stderr(const atf_check_result_t *r)
{
    return &r->m_stderr;
}

bool
atf_check_result_exited(const atf_check_result_t *r)
{
    return atf_process_status_exited(&r->m_status);
}

int
atf_check_result_exitcode(const atf_check_result_t *r)
{
    return atf_process_status_exitstatus(&r->m_status);
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

/* XXX: This function shouldn't be in this module.  It messes with stdout
 * and stderr, and it provides a very high-end interface.  This belongs,
 * probably, somewhere related to test cases (such as in the tc module). */
atf_error_t
atf_check_build_c_o(const char *sfile,
                    const char *ofile,
                    const char *const optargs[],
                    bool *success)
{
    atf_error_t err;
    atf_list_t argv;

    err = atf_build_c_o(sfile, ofile, optargs, &argv);
    if (atf_is_error(err))
        goto out;

    err = check_build_run(&argv, success);

    atf_list_fini(&argv);
out:
    return err;
}

atf_error_t
atf_check_build_cpp(const char *sfile,
                    const char *ofile,
                    const char *const optargs[],
                    bool *success)
{
    atf_error_t err;
    atf_list_t argv;

    err = atf_build_cpp(sfile, ofile, optargs, &argv);
    if (atf_is_error(err))
        goto out;

    err = check_build_run(&argv, success);

    atf_list_fini(&argv);
out:
    return err;
}

atf_error_t
atf_check_build_cxx_o(const char *sfile,
                      const char *ofile,
                      const char *const optargs[],
                      bool *success)
{
    atf_error_t err;
    atf_list_t argv;

    err = atf_build_cxx_o(sfile, ofile, optargs, &argv);
    if (atf_is_error(err))
        goto out;

    err = check_build_run(&argv, success);

    atf_list_fini(&argv);
out:
    return err;
}

atf_error_t
atf_check_exec_array(const char *const *argv, atf_check_result_t *r)
{
    atf_error_t err;
    atf_fs_path_t dir;

    err = create_tmpdir(&dir);
    if (atf_is_error(err))
        goto out;

    err = atf_check_result_init(r, argv, &dir);
    if (atf_is_error(err))
        goto err_dir;

    err = fork_and_wait(argv, &r->m_stdout, &r->m_stderr, &r->m_status);
    if (atf_is_error(err))
        goto err_r;

    INV(!atf_is_error(err));
    goto out_dir;

err_r:
    atf_check_result_fini(r);
err_dir:
    {
        atf_error_t err2 = atf_fs_rmdir(&dir);
        INV(!atf_is_error(err2));
    }
out_dir:
    atf_fs_path_fini(&dir);
out:
    return err;
}

atf_error_t
atf_check_exec_list(const atf_list_t *argv, atf_check_result_t *r)
{
    atf_error_t err;
    const char **argv2;

    argv2 = NULL; /* Silence GCC warning. */
    err = list_to_array(argv, &argv2);
    if (atf_is_error(err))
        goto out;

    err = atf_check_exec_array(argv2, r);

    free(argv2);
out:
    return err;
}
