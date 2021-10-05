/*	$NetBSD: indent.h,v 1.29 2021/10/05 06:09:42 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001 Jens Schweikhardt
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.h 336333 2018-07-16 05:46:50Z pstef $");
#endif

#include <stdbool.h>

#include "indent_codes.h"
#include "indent_globs.h"

#ifndef nitems
#define nitems(array) (sizeof (array) / sizeof (array[0]))
#endif

void		add_typename(const char *);
int		compute_code_indent(void);
int		compute_label_indent(void);
int		indentation_after_range(int, const char *, const char *);
int		indentation_after(int, const char *);
#ifdef debug
void		debug_vis_range(const char *, const char *, const char *,
		    const char *);
void		debug_printf(const char *, ...) __printflike(1, 2);
void		debug_println(const char *, ...) __printflike(1, 2);
const char *	token_type_name(token_type);
#else
#define		debug_printf(fmt, ...) do { } while (false)
#define		debug_println(fmt, ...) do { } while (false)
#define		debug_vis_range(prefix, s, e, suffix) do { } while (false)
#endif
void		inbuf_skip(void);
char		inbuf_next(void);
token_type	lexi(struct parser_state *);
void		diag(int, const char *, ...) __printflike(2, 3);
void		dump_line(void);
void		fill_buffer(void);
void		parse(token_type);
void		process_comment(void);
void		set_option(const char *);
void		load_profiles(const char *);

void		*xmalloc(size_t);
void		*xrealloc(void *, size_t);
char		*xstrdup(const char *);

void		buf_expand(struct buffer *, size_t);

static inline bool
is_hspace(char ch)
{
    return ch == ' ' || ch == '\t';
}
