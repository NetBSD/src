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

#include <stdbool.h>

#include "atf-c/error.h"
#include "atf-c/io.h"
#include "atf-c/sanity.h"
#include "atf-c/tcr.h"

/* ---------------------------------------------------------------------
 * Auxiliary types and functions.
 * --------------------------------------------------------------------- */

static
bool
state_allows_reason(atf_tcr_state_t state)
{
    return state == atf_tcr_failed_state || state == atf_tcr_skipped_state;
}

static
atf_error_t
format_reason(atf_dynstr_t *reason, const char *fmt, va_list ap)
{
    atf_error_t err;
    atf_dynstr_t tmp;
    va_list ap2;

    va_copy(ap2, ap);
    err = atf_dynstr_init_ap(&tmp, fmt, ap2);
    va_end(ap2);
    if (atf_is_error(err))
        goto out;

    /* There is no reason for calling rfind instead of find other than
     * find is not implemented. */
    if (atf_dynstr_rfind_ch(&tmp, '\n') == atf_dynstr_npos) {
        err = atf_dynstr_copy(reason, &tmp);
    } else {
        const char *iter;

        err = atf_dynstr_init_fmt(reason, "BOGUS REASON (THE ORIGINAL "
                                  "HAD NEWLINES): ");
        if (atf_is_error(err))
            goto out_tmp;

        for (iter = atf_dynstr_cstring(&tmp); *iter != '\0'; iter++) {
            if (*iter == '\n')
                err = atf_dynstr_append_fmt(reason, "<<NEWLINE>>");
            else
                err = atf_dynstr_append_fmt(reason, "%c", *iter);

            if (atf_is_error(err)) {
                atf_dynstr_fini(reason);
                break;
            }
        }
    }

out_tmp:
    atf_dynstr_fini(&tmp);
out:
    return err;
}

/* ---------------------------------------------------------------------
 * The "atf_tcr" type.
 * --------------------------------------------------------------------- */

/*
 * Constants.
 */
const atf_tcr_state_t atf_tcr_passed_state = 0;
const atf_tcr_state_t atf_tcr_failed_state = 1;
const atf_tcr_state_t atf_tcr_skipped_state = 2;

/*
 * Constructors/destructors.
 */

atf_error_t
atf_tcr_init(atf_tcr_t *tcr, atf_tcr_state_t state)
{
    atf_error_t err;

    PRE(!state_allows_reason(state));

    atf_object_init(&tcr->m_object);

    tcr->m_state = state;

    err = atf_dynstr_init(&tcr->m_reason);
    if (atf_is_error(err))
        goto err_object;

    INV(!atf_is_error(err));
    return err;

err_object:
    atf_object_fini(&tcr->m_object);

    return err;
}

atf_error_t
atf_tcr_init_reason_ap(atf_tcr_t *tcr, atf_tcr_state_t state,
                       const char *fmt, va_list ap)
{
    va_list ap2;
    atf_error_t err;

    PRE(state_allows_reason(state));

    atf_object_init(&tcr->m_object);

    tcr->m_state = state;

    va_copy(ap2, ap);
    err = format_reason(&tcr->m_reason, fmt, ap2);
    va_end(ap2);
    if (atf_is_error(err))
        goto err_object;

    INV(!atf_is_error(err));
    return err;

err_object:
    atf_object_fini(&tcr->m_object);

    return err;
}

atf_error_t
atf_tcr_init_reason_fmt(atf_tcr_t *tcr, atf_tcr_state_t state,
                        const char *fmt, ...)
{
    va_list ap;
    atf_error_t err;

    va_start(ap, fmt);
    err = atf_tcr_init_reason_ap(tcr, state, fmt, ap);
    va_end(ap);

    return err;
}

void
atf_tcr_fini(atf_tcr_t *tcr)
{
    atf_dynstr_fini(&tcr->m_reason);

    atf_object_fini(&tcr->m_object);
}

/*
 * Getters.
 */

atf_tcr_state_t
atf_tcr_get_state(const atf_tcr_t *tcr)
{
    return tcr->m_state;
}

const atf_dynstr_t *
atf_tcr_get_reason(const atf_tcr_t *tcr)
{
    PRE(state_allows_reason(tcr->m_state));
    return &tcr->m_reason;
}

bool
atf_tcr_has_reason(const atf_tcr_t *tcr)
{
    return state_allows_reason(tcr->m_state);
}

/*
 * Operators.
 */

bool
atf_equal_tcr_tcr(const atf_tcr_t *tcr1, const atf_tcr_t *tcr2)
{
    const bool ar1 = state_allows_reason(tcr1->m_state);
    const bool ar2 = state_allows_reason(tcr2->m_state);

    bool equal;

    if (ar1 && ar2) {
        equal = tcr1->m_state == tcr2->m_state &&
                atf_equal_dynstr_dynstr(&tcr1->m_reason, &tcr2->m_reason);
    } else if (ar1 && !ar2) {
        equal = false;
    } else if (!ar1 && ar2) {
        equal = false;
    } else {
        INV(!ar1 && !ar2);
        equal = tcr1->m_state == tcr2->m_state;
    }

    return equal;
}

/*
 * Serialization.
 */

atf_error_t
atf_tcr_serialize(const atf_tcr_t *tcr, const int fd)
{
    atf_error_t err;
    const atf_tcr_state_t s = atf_tcr_get_state(tcr);
    const char *str;

    if (s == atf_tcr_passed_state)
        str = "passed";
    else if (s == atf_tcr_failed_state)
        str = "failed";
    else if (s == atf_tcr_skipped_state)
        str = "skipped";
    else {
        UNREACHABLE;
        str = NULL;
    }

    err = atf_io_write_fmt(fd, "%s\n", str);
    if (atf_is_error(err))
        goto out;

    if (atf_tcr_has_reason(tcr)) {
        const atf_dynstr_t *r = atf_tcr_get_reason(tcr);
        err = atf_io_write_fmt(fd, "%s\n", atf_dynstr_cstring(r));
        if (atf_is_error(err))
            goto out;
    }

    INV(!atf_is_error(err));

out:
    return err;
}

atf_error_t
atf_tcr_deserialize(atf_tcr_t *tcr, const int fd)
{
    atf_error_t err;
    atf_dynstr_t state, reason;
    bool eof;

    err = atf_dynstr_init(&state);
    if (atf_is_error(err))
        goto out;

    err = atf_dynstr_init(&reason);
    if (atf_is_error(err))
        goto out_state;

    err = atf_io_readline(fd, &state, &eof);
    if (atf_is_error(err))
        goto out_reason;
    INV(!eof);

    if (atf_equal_dynstr_cstring(&state, "passed"))
        err = atf_tcr_init(tcr, atf_tcr_passed_state);
    else {
        err = atf_io_readline(fd, &reason, &eof);
        if (atf_is_error(err))
            goto out_reason;
        INV(!eof);

        if (atf_equal_dynstr_cstring(&state, "failed"))
            err = atf_tcr_init_reason_fmt(tcr, atf_tcr_failed_state, "%s",
                                          atf_dynstr_cstring(&reason));
        else if (atf_equal_dynstr_cstring(&state, "skipped"))
            err = atf_tcr_init_reason_fmt(tcr, atf_tcr_skipped_state, "%s",
                                          atf_dynstr_cstring(&reason));
        else
            UNREACHABLE;
    }

out_reason:
    atf_dynstr_fini(&reason);
out_state:
    atf_dynstr_fini(&state);
out:
    return err;
}
