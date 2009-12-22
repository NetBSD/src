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

#if !defined(ATF_C_TC_H)
#define ATF_C_TC_H

#include <atf-c/defs.h>
#include <atf-c/error_fwd.h>
#include <atf-c/map.h>
#include <atf-c/object.h>

struct atf_dynstr;
struct atf_fs_path;
struct atf_tc;
struct atf_tcr;

typedef void (*atf_tc_head_t)(struct atf_tc *);
typedef void (*atf_tc_body_t)(const struct atf_tc *);
typedef void (*atf_tc_cleanup_t)(const struct atf_tc *);

/* ---------------------------------------------------------------------
 * The "atf_tc_pack" type.
 * --------------------------------------------------------------------- */

/* For static initialization only. */
struct atf_tc_pack {
    const char *m_ident;

    const atf_map_t *m_config;

    atf_tc_head_t m_head;
    atf_tc_body_t m_body;
    atf_tc_cleanup_t m_cleanup;
};
typedef const struct atf_tc_pack atf_tc_pack_t;

/* ---------------------------------------------------------------------
 * The "atf_tc" type.
 * --------------------------------------------------------------------- */

struct atf_tc {
    atf_object_t m_object;

    const char *m_ident;

    atf_map_t m_vars;
    const atf_map_t *m_config;

    atf_tc_head_t m_head;
    atf_tc_body_t m_body;
    atf_tc_cleanup_t m_cleanup;
};
typedef struct atf_tc atf_tc_t;

/* Constructors/destructors. */
atf_error_t atf_tc_init(atf_tc_t *, const char *, atf_tc_head_t,
                        atf_tc_body_t, atf_tc_cleanup_t,
                        const atf_map_t *);
atf_error_t atf_tc_init_pack(atf_tc_t *, atf_tc_pack_t *,
                             const atf_map_t *);
void atf_tc_fini(atf_tc_t *);

/* Getters. */
const char *atf_tc_get_ident(const atf_tc_t *);
const char *atf_tc_get_config_var(const atf_tc_t *, const char *);
const char *atf_tc_get_config_var_wd(const atf_tc_t *, const char *,
                                     const char *);
const char *atf_tc_get_md_var(const atf_tc_t *, const char *);
bool atf_tc_has_config_var(const atf_tc_t *, const char *);
bool atf_tc_has_md_var(const atf_tc_t *, const char *);

/* Modifiers. */
atf_error_t atf_tc_set_md_var(atf_tc_t *, const char *, const char *, ...);

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

atf_error_t atf_tc_run(const atf_tc_t *, struct atf_tcr *,
                       int, int, const struct atf_fs_path *);

/* To be run from test case bodies only. */
void atf_tc_fail(const char *, ...) ATF_DEFS_ATTRIBUTE_NORETURN;
void atf_tc_fail_nonfatal(const char *, ...);
void atf_tc_pass(void) ATF_DEFS_ATTRIBUTE_NORETURN;
void atf_tc_require_prog(const char *);
void atf_tc_skip(const char *, ...) ATF_DEFS_ATTRIBUTE_NORETURN;

/* To be run from test case bodies only; internal to macros.h. */
void atf_tc_fail_check(const char *, int, const char *, ...);
void atf_tc_fail_requirement(const char *, int, const char *, ...)
     ATF_DEFS_ATTRIBUTE_NORETURN;

#endif /* ATF_C_TC_H */
