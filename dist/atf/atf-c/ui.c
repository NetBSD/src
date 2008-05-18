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

#include <sys/ioctl.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "atf-c/dynstr.h"
#include "atf-c/env.h"
#include "atf-c/sanity.h"
#include "atf-c/text.h"
#include "atf-c/ui.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
size_t
terminal_width(void)
{
    static bool done = false;
    static size_t width = 0;

    if (!done) {
        if (atf_env_has("COLUMNS")) {
            const char *cols = atf_env_get("COLUMNS");
            if (strlen(cols) > 0) {
                width = atoi(cols); /* XXX No error checking */
            }
        } else {
            struct winsize ws;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1)
                width = ws.ws_col;
        }

        if (width >= 80)
            width -= 5;

        done = true;
    }

    return width;
}

static
atf_error_t
format_paragraph(atf_dynstr_t *dest,
                 const char *tag, bool repeat, size_t col, bool firstp,
                 char *str)
{
    const size_t maxcol = terminal_width();
    size_t curcol;
    char *last, *str2;
    atf_dynstr_t pad, fullpad;
    atf_error_t err;

    err = atf_dynstr_init_rep(&pad, col - strlen(tag), ' ');
    if (atf_is_error(err))
        goto out;

    err = atf_dynstr_init_rep(&fullpad, col, ' ');
    if (atf_is_error(err))
        goto out_pad;

    if (firstp || repeat)
        err = atf_dynstr_append_fmt(dest, "%s%s", tag,
                                    atf_dynstr_cstring(&pad));
    else
        err = atf_dynstr_append_fmt(dest, atf_dynstr_cstring(&fullpad));
    if (atf_is_error(err))
        goto out_pads;

    last = NULL; /* Silence GCC warning. */
    str2 = strtok_r(str, " ", &last);
    curcol = col;
    do {
        const bool firstw = (str == str2);

        if (!firstw) {
            if (maxcol > 0 && curcol + strlen(str2) + 1 > maxcol) {
                if (repeat)
                    err = atf_dynstr_append_fmt
                        (dest, "\n%s%s", tag, atf_dynstr_cstring(&pad));
                else
                    err = atf_dynstr_append_fmt
                        (dest, "\n%s", atf_dynstr_cstring(&fullpad));
                curcol = col;
            } else {
                err = atf_dynstr_append_fmt(dest, " ");
                curcol++;
            }
        }

        if (!atf_is_error(err)) {
            err = atf_dynstr_append_fmt(dest, str2);
            curcol += strlen(str2);

            str2 = strtok_r(NULL, " ", &last);
        }
    } while (!atf_is_error(err) && str2 != NULL);

out_pads:
    atf_dynstr_fini(&fullpad);
out_pad:
    atf_dynstr_fini(&pad);
out:
    return err;
}

static
atf_error_t
format_aux(atf_dynstr_t *dest,
           const char *tag, bool repeat, size_t col, char *str)
{
    char *last, *str2;
    atf_error_t err;

    PRE(col == 0 || col >= strlen(tag));
    if (col == 0)
        col = strlen(tag);

    str2 = str + strlen(str);
    while (str2 != str && *--str2 == '\n')
        *str2 = '\0';

    last = NULL; /* Silence GCC warning. */
    str2 = strtok_r(str, "\n", &last);
    do {
        const bool first = (str2 == str);
        err = format_paragraph(dest, tag, repeat, col, first, str2);
        str2 = strtok_r(NULL, "\n", &last);
        if (!atf_is_error(err) && str2 != NULL) {
            if (repeat)
                err = atf_dynstr_append_fmt(dest, "\n%s\n", tag);
            else
                err = atf_dynstr_append_fmt(dest, "\n\n");
        }
    } while (str2 != NULL && !atf_is_error(err));

    return 0;
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

atf_error_t
atf_ui_format_ap(atf_dynstr_t *dest,
                 const char *tag, bool repeat, size_t col,
                 const char *fmt, va_list ap)
{
    char *src;
    atf_error_t err;
    va_list ap2;

    va_copy(ap2, ap);
    err = atf_text_format_ap(&src, fmt, ap2);
    va_end(ap2);
    if (!atf_is_error(err)) {
        err = format_aux(dest, tag, repeat, col, src);
        free(src);
    }

    return err;
}

atf_error_t
atf_ui_format_fmt(atf_dynstr_t *dest,
                  const char *tag, bool repeat, size_t col,
                  const char *fmt, ...)
{
    va_list ap;
    atf_error_t err;

    va_start(ap, fmt);
    err = atf_ui_format_ap(dest, tag, repeat, col, fmt, ap);
    va_end(ap);

    return err;
}
