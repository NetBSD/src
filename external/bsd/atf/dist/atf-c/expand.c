/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

#include <regex.h>
#include <stdio.h>
#include <string.h>

#include "atf-c/dynstr.h"
#include "atf-c/error.h"
#include "atf-c/expand.h"
#include "atf-c/sanity.h"

/* REG_BASIC is just a synonym for 0, provided as a counterpart to
 * REG_EXTENDED to improve readability.  It is not provided by all
 * systems. */
#if !defined(REG_BASIC)
#define REG_BASIC 0
#endif /* !defined(REG_BASIC) */

/* ---------------------------------------------------------------------
 * The "pattern" error type.
 * --------------------------------------------------------------------- */

struct pattern_error_data {
    int m_errcode;
    regex_t m_preg;
};
typedef struct pattern_error_data pattern_error_data_t;

static
void
pattern_format(const atf_error_t err, char *buf, size_t buflen)
{
    const pattern_error_data_t *data;
    char tmp[4096];

    PRE(atf_error_is(err, "pattern"));

    data = atf_error_data(err);
    regerror(data->m_errcode, &data->m_preg, tmp, sizeof(tmp));
    snprintf(buf, buflen, "Regular expression error: %s", tmp);
}

static
atf_error_t
pattern_error(int errcode, const regex_t* preg)
{
    atf_error_t err;
    pattern_error_data_t data;

    data.m_errcode = errcode;
    data.m_preg = *preg;

    err = atf_error_new("pattern", &data, sizeof(data), pattern_format);

    return err;
}

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
atf_error_t
glob_to_regex(const char *glob, atf_dynstr_t *regex)
{
    atf_error_t err;

    err = atf_dynstr_init_fmt(regex, "^");

    for (; !atf_is_error(err) && *glob != '\0'; glob++) {
        if (*glob == '*')
            err = atf_dynstr_append_fmt(regex, ".*");
        else if (*glob == '?')
            err = atf_dynstr_append_fmt(regex, ".");
        else
            err = atf_dynstr_append_fmt(regex, "%c", *glob);
    }

    if (!atf_is_error(err))
        err = atf_dynstr_append_fmt(regex, "$");

    if (atf_is_error(err))
        atf_dynstr_fini(regex);

    return err;
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

bool
atf_expand_is_glob(const char *glob)
{
    /* NOTE: Keep this in sync with glob_to_regex! */
    return (strchr(glob, '*') != NULL) || (strchr(glob, '?') != NULL);
}

atf_error_t
atf_expand_matches_glob(const char *glob, const char *candidate,
                        bool *result)
{
    atf_dynstr_t regex;
    atf_error_t err;
    int res;
    regex_t preg;

    /* Special case: regcomp does not like empty patterns. */
    if (glob[0] == '\0') {
        *result = candidate[0] == '\0';
        err = atf_no_error();
        goto out;
    }

    /* Convert the glob pattern into a regular expression. */
    err = glob_to_regex(glob, &regex);
    if (atf_is_error(err))
        goto out;

    /* Compile the regular expression. */
    res = regcomp(&preg, atf_dynstr_cstring(&regex), REG_BASIC);
    if (res != 0) {
        err = pattern_error(res, &preg);
        goto out_regex;
    }

    /* Check match. */
    res = regexec(&preg, candidate, 0, NULL, 0);
    if (res != 0 && res != REG_NOMATCH)
        err = pattern_error(res, &preg);
    else {
        *result = (res == 0);
        INV(!atf_is_error(err));
    }

    regfree(&preg);
out_regex:
    atf_dynstr_fini(&regex);
out:
    return err;
}
