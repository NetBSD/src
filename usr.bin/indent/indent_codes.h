/*	$NetBSD: indent_codes.h,v 1.12 2021/09/27 16:56:35 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)indent_codes.h	8.1 (Berkeley) 6/6/93
 * $FreeBSD: head/usr.bin/indent/indent_codes.h 334564 2018-06-03 16:21:15Z pstef $
 */

typedef enum token_type {
    end_of_file,
    newline,
    lparen,			/* '(' or '[' */
    rparen,			/* ')' or ']' */
    unary_op,			/* e.g. '+' or '&' */
    binary_op,			/* e.g. '<<' or '+' or '&&' or '/=' */
    postfix_op,			/* trailing '++' or '--' */
    question,			/* the '?' from a '?:' expression */
    case_label,
    colon,
    semicolon,
    lbrace,
    rbrace,
    ident,			/* identifier, constant or string */
    comma,
    comment,
    switch_expr,		/* 'switch' '(' <expr> ')' */
    preprocessing,		/* '#' */
    form_feed,
    decl,
    keyword_for_if_while,	/* 'for', 'if' or 'while' */
    keyword_do_else,		/* 'do' or 'else' */
    if_expr,			/* 'if' '(' <expr> ')' */
    while_expr,			/* 'while' '(' <expr> ')' */
    for_exprs,			/* 'for' '(' ... ')' */
    stmt,
    stmt_list,
    keyword_else,		/* 'else' */
    keyword_do,			/* 'do' */
    do_stmt,			/* 'do' <stmt> */
    if_expr_stmt,		/* 'if' '(' <expr> ')' <stmt> */
    if_expr_stmt_else,		/* 'if' '(' <expr> ')' <stmt> 'else' */
    period,
    string_prefix,		/* 'L' */
    storage_class,
    funcname,
    type_def,
    keyword_struct_union_enum
} token_type;
