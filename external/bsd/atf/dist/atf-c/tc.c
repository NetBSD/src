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
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "atf-c/config.h"
#include "atf-c/env.h"
#include "atf-c/error.h"
#include "atf-c/fs.h"
#include "atf-c/process.h"
#include "atf-c/sanity.h"
#include "atf-c/signals.h"
#include "atf-c/tc.h"
#include "atf-c/tcr.h"
#include "atf-c/text.h"
#include "atf-c/user.h"

/* ---------------------------------------------------------------------
 * Auxiliary types and functions.
 * --------------------------------------------------------------------- */

/* Parent-only stuff. */
struct timeout_data;
static atf_error_t body_parent(const atf_tc_t *, const atf_fs_path_t *,
                               pid_t, atf_tcr_t *);
static atf_error_t cleanup_parent(const atf_tc_t *, pid_t);
static atf_error_t fork_body(const atf_tc_t *, const atf_fs_path_t *,
                             atf_tcr_t *);
static atf_error_t fork_cleanup(const atf_tc_t *, const atf_fs_path_t *);
static atf_error_t get_tc_result(const atf_fs_path_t *, atf_tcr_t *);
static atf_error_t program_timeout(pid_t, const atf_tc_t *,
                                   struct timeout_data *);
static void unprogram_timeout(struct timeout_data *);
static void sigalrm_handler(int);

/* Child-only stuff. */
static void body_child(const atf_tc_t *, const atf_fs_path_t *)
            ATF_DEFS_ATTRIBUTE_NORETURN;
static atf_error_t check_arch(const char *, void *);
static atf_error_t check_config(const char *, void *);
static atf_error_t check_machine(const char *, void *);
static atf_error_t check_prog(const char *, void *);
static atf_error_t check_prog_in_dir(const char *, void *);
static atf_error_t check_requirements(const atf_tc_t *);
static void cleanup_child(const atf_tc_t *, const atf_fs_path_t *)
            ATF_DEFS_ATTRIBUTE_NORETURN;
static void fail_internal(const char *, int, const char *, const char *,
                          const char *, va_list,
                          void (*)(const char *, ...));
static void fatal_atf_error(const char *, atf_error_t)
            ATF_DEFS_ATTRIBUTE_NORETURN;
static void fatal_libc_error(const char *, int)
            ATF_DEFS_ATTRIBUTE_NORETURN;
static atf_error_t prepare_child(const atf_tc_t *, const atf_fs_path_t *);
static void write_tcr(const atf_tcr_t *);

/* ---------------------------------------------------------------------
 * The "atf_tc" type.
 * --------------------------------------------------------------------- */

/*
 * Constructors/destructors.
 */

atf_error_t
atf_tc_init(atf_tc_t *tc, const char *ident, atf_tc_head_t head,
            atf_tc_body_t body, atf_tc_cleanup_t cleanup,
            const atf_map_t *config)
{
    atf_error_t err;

    atf_object_init(&tc->m_object);

    tc->m_ident = ident;
    tc->m_head = head;
    tc->m_body = body;
    tc->m_cleanup = cleanup;
    tc->m_config = config;

    err = atf_map_init(&tc->m_vars);
    if (atf_is_error(err))
        goto err_object;

    err = atf_tc_set_md_var(tc, "ident", ident);
    if (atf_is_error(err))
        goto err_map;

    err = atf_tc_set_md_var(tc, "timeout", "300");
    if (atf_is_error(err))
        goto err_map;

    /* XXX Should the head be able to return error codes? */
    tc->m_head(tc);

    if (strcmp(atf_tc_get_md_var(tc, "ident"), ident) != 0)
        atf_tc_fail("Test case head modified the read-only 'ident' "
                    "property");

    INV(!atf_is_error(err));
    return err;

err_map:
    atf_map_fini(&tc->m_vars);
err_object:
    atf_object_fini(&tc->m_object);

    return err;
}

atf_error_t
atf_tc_init_pack(atf_tc_t *tc, const atf_tc_pack_t *pack,
                 const atf_map_t *config)
{
    return atf_tc_init(tc, pack->m_ident, pack->m_head, pack->m_body,
                       pack->m_cleanup, config);
}

void
atf_tc_fini(atf_tc_t *tc)
{
    atf_map_fini(&tc->m_vars);

    atf_object_fini(&tc->m_object);
}

/*
 * Getters.
 */

const char *
atf_tc_get_ident(const atf_tc_t *tc)
{
    return tc->m_ident;
}

const char *
atf_tc_get_config_var(const atf_tc_t *tc, const char *name)
{
    const char *val;
    atf_map_citer_t iter;

    PRE(atf_tc_has_config_var(tc, name));
    iter = atf_map_find_c(tc->m_config, name);
    val = atf_map_citer_data(iter);
    INV(val != NULL);

    return val;
}

const char *
atf_tc_get_config_var_wd(const atf_tc_t *tc, const char *name,
                         const char *defval)
{
    const char *val;

    if (!atf_tc_has_config_var(tc, name))
        val = defval;
    else
        val = atf_tc_get_config_var(tc, name);

    return val;
}

const char *
atf_tc_get_md_var(const atf_tc_t *tc, const char *name)
{
    const char *val;
    atf_map_citer_t iter;

    PRE(atf_tc_has_md_var(tc, name));
    iter = atf_map_find_c(&tc->m_vars, name);
    val = atf_map_citer_data(iter);
    INV(val != NULL);

    return val;
}

bool
atf_tc_has_config_var(const atf_tc_t *tc, const char *name)
{
    bool found;
    atf_map_citer_t end, iter;

    if (tc->m_config == NULL)
        found = false;
    else {
        iter = atf_map_find_c(tc->m_config, name);
        end = atf_map_end_c(tc->m_config);
        found = !atf_equal_map_citer_map_citer(iter, end);
    }

    return found;
}

bool
atf_tc_has_md_var(const atf_tc_t *tc, const char *name)
{
    atf_map_citer_t end, iter;

    iter = atf_map_find_c(&tc->m_vars, name);
    end = atf_map_end_c(&tc->m_vars);
    return !atf_equal_map_citer_map_citer(iter, end);
}

/*
 * Modifiers.
 */

atf_error_t
atf_tc_set_md_var(atf_tc_t *tc, const char *name, const char *fmt, ...)
{
    atf_error_t err;
    char *value;
    va_list ap;

    va_start(ap, fmt);
    err = atf_text_format_ap(&value, fmt, ap);
    va_end(ap);

    if (!atf_is_error(err))
        err = atf_map_insert(&tc->m_vars, name, value, true);
    else
        free(value);

    return err;
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

atf_error_t
atf_tc_run(const atf_tc_t *tc, atf_tcr_t *tcr,
           const atf_fs_path_t *workdirbase)
{
    atf_error_t err, cleanuperr;
    atf_fs_path_t workdir;

    err = atf_fs_path_copy(&workdir, workdirbase);
    if (atf_is_error(err))
        goto out;

    err = atf_fs_path_append_fmt(&workdir, "atf.XXXXXX");
    if (atf_is_error(err))
        goto out_workdir;

    err = atf_fs_mkdtemp(&workdir);
    if (atf_is_error(err))
        goto out_workdir;

    err = fork_body(tc, &workdir, tcr);
    cleanuperr = fork_cleanup(tc, &workdir);
    if (!atf_is_error(cleanuperr))
        (void)atf_fs_cleanup(&workdir);
    if (!atf_is_error(err))
        err = cleanuperr;
    else if (atf_is_error(cleanuperr))
        atf_error_free(cleanuperr);

out_workdir:
    atf_fs_path_fini(&workdir);
out:
    return err;
}

/*
 * Parent-only stuff.
 */

static bool sigalrm_killed = false;
static pid_t sigalrm_pid = -1;

static
void
sigalrm_handler(int signo)
{
    INV(signo == SIGALRM);

    if (sigalrm_pid != -1) {
        killpg(sigalrm_pid, SIGTERM);
        sigalrm_killed = true;
    }
}

struct timeout_data {
    bool m_programmed;
    atf_signal_programmer_t m_sigalrm;
};

static
atf_error_t
program_timeout(pid_t pid, const atf_tc_t *tc, struct timeout_data *td)
{
    atf_error_t err;
    long timeout;

    err = atf_text_to_long(atf_tc_get_md_var(tc, "timeout"), &timeout);
    if (atf_is_error(err))
        goto out;

    if (timeout != 0) {
        sigalrm_pid = pid;
        sigalrm_killed = false;

        err = atf_signal_programmer_init(&td->m_sigalrm, SIGALRM,
                                         sigalrm_handler);
        if (atf_is_error(err))
            goto out;

        struct itimerval itv;
        timerclear(&itv.it_interval);
        timerclear(&itv.it_value);
        itv.it_value.tv_sec = timeout;
        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
            atf_signal_programmer_fini(&td->m_sigalrm);
            err = atf_libc_error(errno, "Failed to program timeout "
                                 "with %ld seconds", timeout);
        }

        td->m_programmed = !atf_is_error(err);
    } else
        td->m_programmed = false;

out:
    return err;
}

static
void
unprogram_timeout(struct timeout_data *td)
{
    if (td->m_programmed) {
        atf_signal_programmer_fini(&td->m_sigalrm);
        sigalrm_pid = -1;
        sigalrm_killed = false;
    }
}

static
atf_error_t
body_parent(const atf_tc_t *tc, const atf_fs_path_t *workdir, pid_t pid,
            atf_tcr_t *tcr)
{
    atf_error_t err;
    int state;
    struct timeout_data td;

    err = program_timeout(pid, tc, &td);
    if (atf_is_error(err)) {
        char buf[4096];

        atf_error_format(err, buf, sizeof(buf));
        fprintf(stderr, "Error programming test case's timeout: %s", buf);
        atf_error_free(err);
        killpg(pid, SIGKILL);
    }

    if (waitpid(pid, &state, 0) == -1) {
        if (errno == EINTR && sigalrm_killed)
            err = atf_tcr_init_reason_fmt(tcr, atf_tcr_failed_state,
                                          "Test case timed out after %s "
                                          "seconds",
                                          atf_tc_get_md_var(tc, "timeout"));
        else
            err = atf_libc_error(errno, "Error waiting for child process "
                                 "%d", pid);
    } else {
        if (!WIFEXITED(state) || WEXITSTATUS(state) != EXIT_SUCCESS) {
            if (WIFEXITED(state))
                err = atf_tcr_init_reason_fmt(tcr, atf_tcr_failed_state,
                                             "Test case did not exit cleanly; "
                                             "exit status was %d",
                                             WEXITSTATUS(state));
            else if (WIFSIGNALED(state))
                err = atf_tcr_init_reason_fmt(tcr, atf_tcr_failed_state,
                                             "Test case did not exit cleanly: "
                                             "%s%s",
                                             strsignal(WTERMSIG(state)),
                                             WCOREDUMP(state) ?
                                               " (core dumped)" : "");
            else
                err = atf_tcr_init_reason_fmt(tcr, atf_tcr_failed_state,
                                             "Test case did not exit cleanly; "
                                             "state was %d", state);
        } else
            err = get_tc_result(workdir, tcr);
    }

    unprogram_timeout(&td);

    return err;
}

static
atf_error_t
cleanup_parent(const atf_tc_t *tc, pid_t pid)
{
    atf_error_t err;
    int state;

    if (waitpid(pid, &state, 0) == -1) {
        err = atf_libc_error(errno, "Error waiting for child process "
                             "%d", pid);
        goto out;
    }

    if (!WIFEXITED(state) || WEXITSTATUS(state) != EXIT_SUCCESS)
        /* XXX Not really a libc error. */
        err = atf_libc_error(EINVAL, "Child process did not exit cleanly");
    else
        err = atf_no_error();

out:
    return err;
}

static
atf_error_t
fork_body(const atf_tc_t *tc, const atf_fs_path_t *workdir, atf_tcr_t *tcr)
{
    atf_error_t err;
    pid_t pid;

    err = atf_process_fork(&pid);
    if (atf_is_error(err))
        goto out;

    if (pid == 0) {
        body_child(tc, workdir);
        UNREACHABLE;
        abort();
    } else {
        err = body_parent(tc, workdir, pid, tcr);
    }

out:
    return err;
}

static
atf_error_t
fork_cleanup(const atf_tc_t *tc, const atf_fs_path_t *workdir)
{
    atf_error_t err;
    pid_t pid;

    if (tc->m_cleanup == NULL)
        err = atf_no_error();
    else {
        err = atf_process_fork(&pid);
        if (atf_is_error(err))
            goto out;

        if (pid == 0) {
            cleanup_child(tc, workdir);
            UNREACHABLE;
            abort();
        } else {
            err = cleanup_parent(tc, pid);
        }
    }

out:
    return err;
}

static
atf_error_t
get_tc_result(const atf_fs_path_t *workdir, atf_tcr_t *tcr)
{
    atf_error_t err;
    int fd;
    atf_fs_path_t tcrfile;

    err = atf_fs_path_copy(&tcrfile, workdir);
    if (atf_is_error(err))
        goto out;

    err = atf_fs_path_append_fmt(&tcrfile, "tc-result");
    if (atf_is_error(err))
        goto out_tcrfile;

    fd = open(atf_fs_path_cstring(&tcrfile), O_RDONLY);
    if (fd == -1) {
        err = atf_libc_error(errno, "Cannot retrieve test case result");
        goto out_tcrfile;
    }

    err = atf_tcr_deserialize(tcr, fd);

    close(fd);
out_tcrfile:
    atf_fs_path_fini(&tcrfile);
out:
    return err;
}

/*
 * Child-only stuff.
 */

static const atf_tc_t *current_tc = NULL;
static const atf_fs_path_t *current_workdir = NULL;
static size_t current_tc_fail_count = 0;

static
atf_error_t
prepare_child(const atf_tc_t *tc, const atf_fs_path_t *workdir)
{
    atf_error_t err;
    int i, ret;

    current_tc = tc;
    current_workdir = workdir;
    current_tc_fail_count = 0;

    ret = setpgid(getpid(), 0);
    INV(ret != -1);

    umask(S_IWGRP | S_IWOTH);

    for (i = 1; i <= atf_signals_last_signo; i++)
        atf_signal_reset(i);

    err = atf_env_set("HOME", atf_fs_path_cstring(workdir));
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LANG");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_ALL");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_COLLATE");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_CTYPE");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_MESSAGES");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_MONETARY");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_NUMERIC");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("LC_TIME");
    if (atf_is_error(err))
        goto out;

    err = atf_env_unset("TZ");
    if (atf_is_error(err))
        goto out;

    if (chdir(atf_fs_path_cstring(workdir)) == -1) {
        err = atf_libc_error(errno, "Cannot enter work directory '%s'",
                             atf_fs_path_cstring(workdir));
        goto out;
    }

    err = atf_no_error();

out:
    return err;
}

static
void
body_child(const atf_tc_t *tc, const atf_fs_path_t *workdir)
{
    atf_error_t err;

    atf_disable_exit_checks();

    err = prepare_child(tc, workdir);
    if (atf_is_error(err))
        goto print_err;
    err = check_requirements(tc);
    if (atf_is_error(err))
        goto print_err;
    tc->m_body(tc);

    if (current_tc_fail_count == 0)
        atf_tc_pass();
    else
        atf_tc_fail("%d checks failed; see output for more details",
                    current_tc_fail_count);

    UNREACHABLE;
    abort();

print_err:
    INV(atf_is_error(err));
    {
        char buf[4096];

        atf_error_format(err, buf, sizeof(buf));
        atf_error_free(err);

        atf_tc_fail("Error while preparing child process: %s", buf);
    }

    UNREACHABLE;
    abort();
}

static
atf_error_t
check_arch(const char *arch, void *data)
{
    bool *found = data;

    if (strcmp(arch, atf_config_get("atf_arch")) == 0)
        *found = true;

    return atf_no_error();
}

static
atf_error_t
check_config(const char *var, void *data)
{
    if (!atf_tc_has_config_var(current_tc, var))
        atf_tc_skip("Required configuration variable %s not defined", var);

    return atf_no_error();
}

static
atf_error_t
check_machine(const char *machine, void *data)
{
    bool *found = data;

    if (strcmp(machine, atf_config_get("atf_machine")) == 0)
        *found = true;

    return atf_no_error();
}

struct prog_found_pair {
    const char *prog;
    bool found;
};

static
atf_error_t
check_prog(const char *prog, void *data)
{
    atf_error_t err;
    atf_fs_path_t p;

    err = atf_fs_path_init_fmt(&p, "%s", prog);
    if (atf_is_error(err))
        goto out;

    if (atf_fs_path_is_absolute(&p)) {
        if (atf_is_error(atf_fs_eaccess(&p, atf_fs_access_x)))
            atf_tc_skip("The required program %s could not be found", prog);
    } else {
        const char *path = atf_env_get("PATH");
        struct prog_found_pair pf;
        atf_fs_path_t bp;

        err = atf_fs_path_branch_path(&p, &bp);
        if (atf_is_error(err))
            goto out_p;

        if (strcmp(atf_fs_path_cstring(&bp), ".") != 0)
            atf_tc_fail("Relative paths are not allowed when searching for "
                        "a program (%s)", prog);

        pf.prog = prog;
        pf.found = false;
        err = atf_text_for_each_word(path, ":", check_prog_in_dir, &pf);
        if (atf_is_error(err))
            goto out_bp;

        if (!pf.found)
            atf_tc_skip("The required program %s could not be found in "
                        "the PATH", prog);

out_bp:
        atf_fs_path_fini(&bp);
    }

out_p:
    atf_fs_path_fini(&p);
out:
    return err;
}

static
atf_error_t
check_prog_in_dir(const char *dir, void *data)
{
    struct prog_found_pair *pf = data;
    atf_error_t err;

    if (pf->found)
        err = atf_no_error();
    else {
        atf_fs_path_t p;

        err = atf_fs_path_init_fmt(&p, "%s/%s", dir, pf->prog);
        if (atf_is_error(err))
            goto out_p;

        if (!atf_is_error(atf_fs_eaccess(&p, atf_fs_access_x)))
            pf->found = true;

out_p:
        atf_fs_path_fini(&p);
    }

    return err;
}

static
atf_error_t
check_requirements(const atf_tc_t *tc)
{
    atf_error_t err;

    err = atf_no_error();

    if (atf_tc_has_md_var(tc, "require.arch")) {
        const char *arches = atf_tc_get_md_var(tc, "require.arch");
        bool found = false;

        if (strlen(arches) == 0)
            atf_tc_fail("Invalid value in the require.arch property");
        else {
            err = atf_text_for_each_word(arches, " ", check_arch, &found);
            if (atf_is_error(err))
                goto out;

            if (!found)
                atf_tc_skip("Requires one of the '%s' architectures",
                            arches);
        }
    }

    if (atf_tc_has_md_var(tc, "require.config")) {
        const char *vars = atf_tc_get_md_var(tc, "require.config");

        if (strlen(vars) == 0)
            atf_tc_fail("Invalid value in the require.config property");
        else {
            err = atf_text_for_each_word(vars, " ", check_config, NULL);
            if (atf_is_error(err))
                goto out;
        }
    }

    if (atf_tc_has_md_var(tc, "require.machine")) {
        const char *machines = atf_tc_get_md_var(tc, "require.machine");
        bool found = false;

        if (strlen(machines) == 0)
            atf_tc_fail("Invalid value in the require.machine property");
        else {
            err = atf_text_for_each_word(machines, " ", check_machine,
                                         &found);
            if (atf_is_error(err))
                goto out;

            if (!found)
                atf_tc_skip("Requires one of the '%s' machine types",
                            machines);
        }
    }

    if (atf_tc_has_md_var(tc, "require.progs")) {
        const char *progs = atf_tc_get_md_var(tc, "require.progs");

        if (strlen(progs) == 0)
            atf_tc_fail("Invalid value in the require.progs property");
        else {
            err = atf_text_for_each_word(progs, " ", check_prog, NULL);
            if (atf_is_error(err))
                goto out;
        }
    }

    if (atf_tc_has_md_var(tc, "require.user")) {
        const char *u = atf_tc_get_md_var(tc, "require.user");

        if (strcmp(u, "root") == 0) {
            if (!atf_user_is_root())
                atf_tc_skip("Requires root privileges");
        } else if (strcmp(u, "unprivileged") == 0) {
            if (atf_user_is_root())
                atf_tc_skip("Requires an unprivileged user");
        } else
            atf_tc_fail("Invalid value in the require.user property");
    }

    INV(!atf_is_error(err));
out:
    return err;
}

static
void
cleanup_child(const atf_tc_t *tc, const atf_fs_path_t *workdir)
{
    atf_error_t err;

    atf_disable_exit_checks();

    err = prepare_child(tc, workdir);
    if (atf_is_error(err))
        exit(EXIT_FAILURE);
    else {
        tc->m_cleanup(tc);
        exit(EXIT_SUCCESS);
    }

    UNREACHABLE;
    abort();
}

static
void
fatal_atf_error(const char *prefix, atf_error_t err)
{
    char buf[1024];

    INV(atf_is_error(err));

    atf_error_format(err, buf, sizeof(buf));
    atf_error_free(err);

    fprintf(stderr, "%s: %s", prefix, buf);

    abort();
}

static
void
fatal_libc_error(const char *prefix, int err)
{
    fprintf(stderr, "%s: %s", prefix, strerror(err));

    abort();
}

static
void
write_tcr(const atf_tcr_t *tcr)
{
    atf_error_t err;
    int fd;
    atf_fs_path_t tcrfile;

    err = atf_fs_path_copy(&tcrfile, current_workdir);
    if (atf_is_error(err))
        fatal_atf_error("Cannot write test case results", err);

    err = atf_fs_path_append_fmt(&tcrfile, "tc-result");
    if (atf_is_error(err))
        fatal_atf_error("Cannot write test case results", err);

    fd = open(atf_fs_path_cstring(&tcrfile),
              O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd == -1)
        fatal_libc_error("Cannot write test case results", errno);

    err = atf_tcr_serialize(tcr, fd);
    if (atf_is_error(err))
        fatal_atf_error("Cannot write test case results", err);

    close(fd);
}

void
atf_tc_fail(const char *fmt, ...)
{
    va_list ap;
    atf_tcr_t tcr;
    atf_error_t err;

    PRE(current_tc != NULL);

    va_start(ap, fmt);
    err = atf_tcr_init_reason_ap(&tcr, atf_tcr_failed_state, fmt, ap);
    va_end(ap);
    if (atf_is_error(err))
        abort();

    write_tcr(&tcr);

    atf_tcr_fini(&tcr);

    exit(EXIT_SUCCESS);
}

void
atf_tc_fail_nonfatal(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");

    current_tc_fail_count++;
}

void
atf_tc_fail_check(const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fail_internal(file, line, "Check failed", "*** ", fmt, ap,
                  atf_tc_fail_nonfatal);
    va_end(ap);
}

void
atf_tc_fail_requirement(const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fail_internal(file, line, "Requirement failed", "", fmt, ap,
                  atf_tc_fail);
    va_end(ap);

    UNREACHABLE;
    abort();
}

static
void
fail_internal(const char *file, int line, const char *reason,
              const char *prefix, const char *fmt, va_list ap,
              void (*failfunc)(const char *, ...))
{
    va_list ap2;
    atf_error_t err;
    atf_dynstr_t msg;

    err = atf_dynstr_init_fmt(&msg, "%s%s:%d: %s: ", prefix, file, line,
                              reason);
    if (atf_is_error(err))
        goto backup;

    va_copy(ap2, ap);
    err = atf_dynstr_append_ap(&msg, fmt, ap2);
    va_end(ap2);
    if (atf_is_error(err)) {
        atf_dynstr_fini(&msg);
        goto backup;
    }

    va_copy(ap2, ap);
    failfunc("%s", atf_dynstr_cstring(&msg));
    atf_dynstr_fini(&msg);
    return;

backup:
    va_copy(ap2, ap);
    failfunc(fmt, ap2);
    va_end(ap2);
}

void
atf_tc_pass(void)
{
    atf_tcr_t tcr;
    atf_error_t err;

    PRE(current_tc != NULL);

    err = atf_tcr_init(&tcr, atf_tcr_passed_state);
    if (atf_is_error(err))
        abort();

    write_tcr(&tcr);

    atf_tcr_fini(&tcr);

    exit(EXIT_SUCCESS);
}

void
atf_tc_require_prog(const char *prog)
{
    atf_error_t err;

    err = check_prog(prog, NULL);
    if (atf_is_error(err))
        atf_tc_fail("atf_tc_require_prog failed"); /* XXX Correct? */
}

void
atf_tc_skip(const char *fmt, ...)
{
    va_list ap;
    atf_tcr_t tcr;
    atf_error_t err;

    PRE(current_tc != NULL);

    va_start(ap, fmt);
    err = atf_tcr_init_reason_ap(&tcr, atf_tcr_skipped_state, fmt, ap);
    va_end(ap);
    if (atf_is_error(err))
        abort();

    write_tcr(&tcr);

    atf_tcr_fini(&tcr);

    exit(EXIT_SUCCESS);
}
