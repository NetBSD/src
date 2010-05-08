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

#if !defined(ATF_C_TP_H)
#define ATF_C_TP_H

#include <atf-c/error_fwd.h>
#include <atf-c/list.h>
#include <atf-c/object.h>

struct atf_fs_path;
struct atf_map;
struct atf_tc;
struct atf_tcr;

/* ---------------------------------------------------------------------
 * The "atf_tp" type.
 * --------------------------------------------------------------------- */

struct atf_tp {
    atf_object_t m_object;

    atf_list_t m_tcs;
    const struct atf_map *m_config;
};
typedef struct atf_tp atf_tp_t;

/* Constructors/destructors. */
atf_error_t atf_tp_init(atf_tp_t *, struct atf_map *);
void atf_tp_fini(atf_tp_t *);

/* Getters. */
const struct atf_map *atf_tp_get_config(const atf_tp_t *);
bool atf_tp_has_tc(const atf_tp_t *, const char *);
const struct atf_tc *atf_tp_get_tc(const atf_tp_t *, const char *);
const atf_list_t *atf_tp_get_tcs(const atf_tp_t *);

/* Modifiers. */
atf_error_t atf_tp_add_tc(atf_tp_t *, struct atf_tc *);

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

atf_error_t atf_tp_run(const atf_tp_t *, const char *,
                       const struct atf_fs_path *);
atf_error_t atf_tp_cleanup(const atf_tp_t *, const char *);

#endif /* ATF_C_TP_H */
