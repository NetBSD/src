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

#if !defined(ATF_C_TCR_H)
#define ATF_C_TCR_H

#include <stdarg.h>
#include <stdbool.h>

#include <atf-c/dynstr.h>
#include <atf-c/error_fwd.h>

struct atf_fs_path;

/* ---------------------------------------------------------------------
 * The "atf_tcr" type.
 * --------------------------------------------------------------------- */

typedef int atf_tcr_state_t;

struct atf_tcr {
    atf_tcr_state_t m_state;
    atf_dynstr_t m_reason;
};
typedef struct atf_tcr atf_tcr_t;

/* Constants. */
extern const atf_tcr_state_t atf_tcr_passed_state;
extern const atf_tcr_state_t atf_tcr_failed_state;
extern const atf_tcr_state_t atf_tcr_skipped_state;

/* Constructors/destructors. */
atf_error_t atf_tcr_init(atf_tcr_t *, int);
atf_error_t atf_tcr_init_reason_ap(atf_tcr_t *, atf_tcr_state_t,
                                   const char *, va_list);
atf_error_t atf_tcr_init_reason_fmt(atf_tcr_t *, atf_tcr_state_t,
                                    const char *, ...);
void atf_tcr_fini(atf_tcr_t *);

/* Getters. */
atf_tcr_state_t atf_tcr_get_state(const atf_tcr_t *);
const atf_dynstr_t *atf_tcr_get_reason(const atf_tcr_t *);
bool atf_tcr_has_reason(const atf_tcr_t *);

/* Operators. */
bool atf_equal_tcr_tcr(const atf_tcr_t *, const atf_tcr_t *);

/* Serialization. */
atf_error_t atf_tcr_serialize(const atf_tcr_t *, const int);
atf_error_t atf_tcr_deserialize(atf_tcr_t *, const int);
atf_error_t atf_tcr_write(const atf_tcr_t *, const struct atf_fs_path *);

#endif /* ATF_C_TCR_H */
