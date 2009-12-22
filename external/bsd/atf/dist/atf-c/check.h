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

#if !defined(ATF_C_CHECK_H)
#define ATF_C_CHECK_H

#include <atf-c/error_fwd.h>
#include <atf-c/fs.h>
#include <atf-c/list.h>
#include <atf-c/object.h>
#include <atf-c/process.h>

/* ---------------------------------------------------------------------
 * The "atf_check_result" type.
 * --------------------------------------------------------------------- */

struct atf_check_result {
    atf_object_t m_object;

    atf_list_t m_argv;
    atf_fs_path_t m_dir;
    atf_fs_path_t m_stdout;
    atf_fs_path_t m_stderr;
    atf_process_status_t m_status;
};
typedef struct atf_check_result atf_check_result_t;

/* Construtors and destructors */
void atf_check_result_fini(atf_check_result_t *);

/* Getters */
const atf_list_t *atf_check_result_argv(const atf_check_result_t *);
const atf_fs_path_t *atf_check_result_stdout(const atf_check_result_t *);
const atf_fs_path_t *atf_check_result_stderr(const atf_check_result_t *);
bool atf_check_result_exited(const atf_check_result_t *);
int atf_check_result_exitcode(const atf_check_result_t *);

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

atf_error_t atf_check_build_c_o(const char *, const char *,
                                const char *const [],
                                bool *);
atf_error_t atf_check_build_cpp(const char *, const char *,
                                const char *const [],
                                bool *);
atf_error_t atf_check_build_cxx_o(const char *, const char *,
                                  const char *const [],
                                  bool *);
atf_error_t atf_check_exec_array(const char *const *, atf_check_result_t *);
atf_error_t atf_check_exec_list(const atf_list_t *, atf_check_result_t *);

#endif /* ATF_C_CHECK_H */
