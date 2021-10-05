/*	$NetBSD: lexi.c,v 1.72 2021/10/05 22:22:46 rillig Exp $	*/

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
 */

#if 0
static char sccsid[] = "@(#)lexi.c	8.1 (Berkeley) 6/6/93";
#endif

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__RCSID("$NetBSD: lexi.c,v 1.72 2021/10/05 22:22:46 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/lexi.c 337862 2018-08-15 18:19:45Z pstef $");
#endif

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "indent.h"

/* must be sorted alphabetically, is used in binary search */
static const struct keyword {
    const char *name;
    enum keyword_kind kind;
} keywords[] = {
    {"_Bool", kw_type},
    {"_Complex", kw_type},
    {"_Imaginary", kw_type},
    {"auto", kw_storage_class},
    {"bool", kw_type},
    {"break", kw_jump},
    {"case", kw_case_or_default},
    {"char", kw_type},
    {"complex", kw_type},
    {"const", kw_type},
    {"continue", kw_jump},
    {"default", kw_case_or_default},
    {"do", kw_do_or_else},
    {"double", kw_type},
    {"else", kw_do_or_else},
    {"enum", kw_struct_or_union_or_enum},
    {"extern", kw_storage_class},
    {"float", kw_type},
    {"for", kw_for_or_if_or_while},
    {"global", kw_type},
    {"goto", kw_jump},
    {"if", kw_for_or_if_or_while},
    {"imaginary", kw_type},
    {"inline", kw_inline_or_restrict},
    {"int", kw_type},
    {"long", kw_type},
    {"offsetof", kw_offsetof},
    {"register", kw_storage_class},
    {"restrict", kw_inline_or_restrict},
    {"return", kw_jump},
    {"short", kw_type},
    {"signed", kw_type},
    {"sizeof", kw_sizeof},
    {"static", kw_storage_class},
    {"struct", kw_struct_or_union_or_enum},
    {"switch", kw_switch},
    {"typedef", kw_typedef},
    {"union", kw_struct_or_union_or_enum},
    {"unsigned", kw_type},
    {"void", kw_type},
    {"volatile", kw_type},
    {"while", kw_for_or_if_or_while}
};

struct {
    const char **items;
    unsigned int len;
    unsigned int cap;
} typenames;

/*
 * The transition table below was rewritten by hand from lx's output, given
 * the following definitions. lx is Katherine Flavel's lexer generator.
 *
 * O  = /[0-7]/;        D  = /[0-9]/;          NZ = /[1-9]/;
 * H  = /[a-f0-9]/i;    B  = /[0-1]/;          HP = /0x/i;
 * BP = /0b/i;          E  = /e[+\-]?/i D+;    P  = /p[+\-]?/i D+;
 * FS = /[fl]/i;        IS = /u/i /(l|L|ll|LL)/? | /(l|L|ll|LL)/ /u/i?;
 *
 * D+           E  FS? -> $float;
 * D*    "." D+ E? FS? -> $float;
 * D+    "."    E? FS? -> $float;    HP H+           IS? -> $int;
 * HP H+        P  FS? -> $float;    NZ D*           IS? -> $int;
 * HP H* "." H+ P  FS? -> $float;    "0" O*          IS? -> $int;
 * HP H+ "."    P  FS  -> $float;    BP B+           IS? -> $int;
 */
/* INDENT OFF */
static const char num_lex_state[][26] = {
    /*                examples:
                                     00
             s                      0xx
             t                    00xaa
             a     11       101100xxa..
             r   11ee0001101lbuuxx.a.pp
             t.01.e+008bLuxll0Ll.aa.p+0
    states:  ABCDEFGHIJKLMNOPQRSTUVWXYZ */
    [0] =   "uuiifuufiuuiiuiiiiiuiuuuuu",
    [1] =   "CEIDEHHHIJQ  U  Q  VUVVZZZ",
    [2] =   "DEIDEHHHIJQ  U  Q  VUVVZZZ",
    [3] =   "DEIDEHHHIJ   U     VUVVZZZ",
    [4] =   "DEJDEHHHJJ   U     VUVVZZZ",
    [5] =   "             U     VUVV   ",
    [6] =   "  K          U     VUVV   ",
    [7] =   "  FFF   FF   U     VUVV   ",
    [8] =   "    f  f     U     VUVV  f",
    [9] =   "  LLf  fL  PR   Li  L    f",
    [10] =  "  OOf  fO   S P O i O    f",
    [11] =  "                    FFX   ",
    [12] =  "  MM    M  i  iiM   M     ",
    [13] =  "  N                       ",
    [14] =  "     G                 Y  ",
    [15] =  "B EE    EE   T      W     ",
    /*       ABCDEFGHIJKLMNOPQRSTUVWXYZ */
};
/* INDENT ON */

static const uint8_t num_lex_row[] = {
    ['0'] = 1,
    ['1'] = 2,
    ['2'] = 3, ['3'] = 3, ['4'] = 3, ['5'] = 3, ['6'] = 3, ['7'] = 3,
    ['8'] = 4, ['9'] = 4,
    ['A'] = 5, ['a'] = 5, ['C'] = 5, ['c'] = 5, ['D'] = 5, ['d'] = 5,
    ['B'] = 6, ['b'] = 6,
    ['E'] = 7, ['e'] = 7,
    ['F'] = 8, ['f'] = 8,
    ['L'] = 9,
    ['l'] = 10,
    ['P'] = 11, ['p'] = 11,
    ['U'] = 12, ['u'] = 12,
    ['X'] = 13, ['x'] = 13,
    ['+'] = 14, ['-'] = 14,
    ['.'] = 15,
};

static char
inbuf_peek(void)
{
    return *buf_ptr;
}

void
inbuf_skip(void)
{
    buf_ptr++;
    if (buf_ptr >= buf_end)
	fill_buffer();
}

char
inbuf_next(void)
{
    char ch = inbuf_peek();
    inbuf_skip();
    return ch;
}

static void
check_size_token(size_t desired_size)
{
    if (token.e + desired_size >= token.l)
	buf_expand(&token, desired_size);
}

static int
cmp_keyword_by_name(const void *key, const void *elem)
{
    return strcmp(key, ((const struct keyword *)elem)->name);
}

static int
cmp_type_by_name(const void *key, const void *elem)
{
    return strcmp(key, *((const char *const *)elem));
}

#ifdef debug
const char *
token_type_name(token_type ttype)
{
    static const char *const name[] = {
	"end_of_file", "newline", "lparen", "rparen", "unary_op",
	"binary_op", "postfix_op", "question", "case_label", "colon",
	"semicolon", "lbrace", "rbrace", "ident", "comma",
	"comment", "switch_expr", "preprocessing", "form_feed", "decl",
	"keyword_for_if_while", "keyword_do_else",
	"if_expr", "while_expr", "for_exprs",
	"stmt", "stmt_list", "keyword_else", "keyword_do", "do_stmt",
	"if_expr_stmt", "if_expr_stmt_else", "period", "string_prefix",
	"storage_class", "funcname", "type_def", "keyword_struct_union_enum"
    };

    assert(0 <= ttype && ttype < nitems(name));

    return name[ttype];
}

static void
debug_print_buf(const char *name, const struct buffer *buf)
{
    if (buf->s < buf->e) {
	debug_printf(" %s ", name);
	debug_vis_range("\"", buf->s, buf->e, "\"");
    }
}

static token_type
lexi_end(token_type ttype)
{
    debug_printf("in line %d, lexi returns '%s'",
	line_no, token_type_name(ttype));
    debug_print_buf("token", &token);
    debug_print_buf("label", &lab);
    debug_print_buf("code", &code);
    debug_print_buf("comment", &com);
    debug_printf("\n");

    return ttype;
}
#else
#define lexi_end(tk) (tk)
#endif

static void
lex_number(void)
{
    for (uint8_t s = 'A'; s != 'f' && s != 'i' && s != 'u';) {
	uint8_t ch = (uint8_t)*buf_ptr;
	if (ch >= nitems(num_lex_row) || num_lex_row[ch] == 0)
	    break;
	uint8_t row = num_lex_row[ch];
	if (num_lex_state[row][s - 'A'] == ' ') {
	    /*-
	     * num_lex_state[0][s - 'A'] now indicates the type:
	     * f = floating, ch = integer, u = unknown
	     */
	    break;
	}
	s = num_lex_state[row][s - 'A'];
	check_size_token(1);
	*token.e++ = inbuf_next();
    }
}

static void
lex_word(void)
{
    while (isalnum((unsigned char)*buf_ptr) ||
	   *buf_ptr == '\\' ||
	   *buf_ptr == '_' || *buf_ptr == '$') {
	/* fill_buffer() terminates buffer with newline */
	if (*buf_ptr == '\\') {
	    if (buf_ptr[1] == '\n') {
		buf_ptr += 2;
		if (buf_ptr >= buf_end)
		    fill_buffer();
	    } else
		break;
	}
	check_size_token(1);
	*token.e++ = inbuf_next();
    }
}

static void
lex_char_or_string(void)
{
    for (char delim = *token.s;;) {
	if (*buf_ptr == '\n') {
	    diag(1, "Unterminated literal");
	    return;
	}
	check_size_token(2);
	*token.e++ = inbuf_next();
	if (token.e[-1] == delim)
	    return;
	if (token.e[-1] == '\\') {
	    if (*buf_ptr == '\n')
		++line_no;
	    *token.e++ = inbuf_next();
	}
    }
}

/*
 * This hack attempts to guess whether the current token is in fact a
 * declaration keyword -- one that has been defined by typedef.
 */
static bool
probably_typedef(const struct parser_state *state)
{
    if (state->p_l_follow != 0)
	return false;
    if (state->block_init || state->in_stmt)
	return false;
    if (buf_ptr[0] == '*' && buf_ptr[1] != '=')
	goto maybe;
    if (isalpha((unsigned char)*buf_ptr))
	goto maybe;
    return false;
maybe:
    return state->last_token == semicolon ||
	state->last_token == lbrace ||
	state->last_token == rbrace;
}

static bool
is_typename(void)
{
    if (opt.auto_typedefs) {
	const char *u;
	if ((u = strrchr(token.s, '_')) != NULL && strcmp(u, "_t") == 0)
	    return true;
    }

    if (typenames.len == 0)
	return false;
    return bsearch(token.s, typenames.items, (size_t)typenames.len,
	sizeof(typenames.items[0]), cmp_type_by_name) != NULL;
}

/* Reads the next token, placing it in the global variable "token". */
token_type
lexi(struct parser_state *state)
{
    bool unary_delim;		/* whether the current token forces a
				 * following operator to be unary */
    token_type ttype;

    token.e = token.s;		/* point to start of place to save token */
    unary_delim = false;
    state->col_1 = state->last_nl;	/* tell world that this token started
					 * in column 1 iff the last thing
					 * scanned was a newline */
    state->last_nl = false;

    while (is_hspace(*buf_ptr)) {
	state->col_1 = false;
	inbuf_skip();
    }

    /* Scan an alphanumeric token */
    if (isalnum((unsigned char)*buf_ptr) ||
	*buf_ptr == '_' || *buf_ptr == '$' ||
	(buf_ptr[0] == '.' && isdigit((unsigned char)buf_ptr[1]))) {
	struct keyword *kw;

	if (isdigit((unsigned char)*buf_ptr) ||
	    (buf_ptr[0] == '.' && isdigit((unsigned char)buf_ptr[1]))) {
	    lex_number();
	} else {
	    lex_word();
	}
	*token.e = '\0';

	if (token.s[0] == 'L' && token.s[1] == '\0' &&
	    (*buf_ptr == '"' || *buf_ptr == '\''))
	    return lexi_end(string_prefix);

	while (is_hspace(inbuf_peek()))
	    inbuf_skip();
	state->keyword = kw_0;

	if (state->last_token == keyword_struct_union_enum &&
		state->p_l_follow == 0) {
	    state->last_u_d = true;
	    return lexi_end(decl);
	}
	/*
	 * Operator after identifier is binary unless last token was 'struct'
	 */
	state->last_u_d = (state->last_token == keyword_struct_union_enum);

	kw = bsearch(token.s, keywords, nitems(keywords),
	    sizeof(keywords[0]), cmp_keyword_by_name);
	if (kw == NULL) {
	    if (is_typename()) {
		state->keyword = kw_type;
		state->last_u_d = true;
		goto found_typename;
	    }
	} else {		/* we have a keyword */
	    state->keyword = kw->kind;
	    state->last_u_d = true;
	    switch (kw->kind) {
	    case kw_switch:
		return lexi_end(switch_expr);
	    case kw_case_or_default:
		return lexi_end(case_label);
	    case kw_struct_or_union_or_enum:
	    case kw_type:
	found_typename:
		if (state->p_l_follow != 0) {
		    /* inside parens: cast, param list, offsetof or sizeof */
		    state->cast_mask |= (1 << state->p_l_follow) & ~state->not_cast_mask;
		}
		if (state->last_token == period || state->last_token == unary_op) {
		    state->keyword = kw_0;
		    break;
		}
		if (kw != NULL && kw->kind == kw_struct_or_union_or_enum)
		    return lexi_end(keyword_struct_union_enum);
		if (state->p_l_follow != 0)
		    break;
		return lexi_end(decl);

	    case kw_for_or_if_or_while:
		return lexi_end(keyword_for_if_while);

	    case kw_do_or_else:
		return lexi_end(keyword_do_else);

	    case kw_storage_class:
		return lexi_end(storage_class);

	    case kw_typedef:
		return lexi_end(type_def);

	    default:		/* all others are treated like any other
				 * identifier */
		return lexi_end(ident);
	    }			/* end of switch */
	}			/* end of if (found_it) */
	if (*buf_ptr == '(' && state->tos <= 1 && state->ind_level == 0 &&
	    !state->in_parameter_declaration && !state->block_init) {
	    char *tp = buf_ptr;
	    while (tp < buf_end)
		if (*tp++ == ')' && (*tp == ';' || *tp == ','))
		    goto not_proc;
	    strncpy(state->procname, token.s, sizeof state->procname - 1);
	    if (state->in_decl)
		state->in_parameter_declaration = true;
	    return lexi_end(funcname);
    not_proc:;
	} else if (probably_typedef(state)) {
	    state->keyword = kw_type;
	    state->last_u_d = true;
	    return lexi_end(decl);
	}
	if (state->last_token == decl)	/* if this is a declared variable,
					 * then following sign is unary */
	    state->last_u_d = true;	/* will make "int a -1" work */
	return lexi_end(ident);	/* the ident is not in the list */
    }				/* end of procesing for alpanum character */

    /* Scan a non-alphanumeric token */

    check_size_token(3);	/* things like "<<=" */
    *token.e++ = inbuf_next();	/* if it is only a one-character token, it is
				 * moved here */
    *token.e = '\0';

    switch (*token.s) {
    case '\n':
	unary_delim = state->last_u_d;
	state->last_nl = true;	/* remember that we just had a newline */
	/* if data has been exhausted, the newline is a dummy. */
	ttype = had_eof ? end_of_file : newline;
	break;

    case '\'':
    case '"':
	lex_char_or_string();
	ttype = ident;
	break;

    case '(':
    case '[':
	unary_delim = true;
	ttype = lparen;
	break;

    case ')':
    case ']':
	ttype = rparen;
	break;

    case '#':
	unary_delim = state->last_u_d;
	ttype = preprocessing;
	break;

    case '?':
	unary_delim = true;
	ttype = question;
	break;

    case ':':
	ttype = colon;
	unary_delim = true;
	break;

    case ';':
	unary_delim = true;
	ttype = semicolon;
	break;

    case '{':
	unary_delim = true;
	ttype = lbrace;
	break;

    case '}':
	unary_delim = true;
	ttype = rbrace;
	break;

    case '\f':
	unary_delim = state->last_u_d;
	state->last_nl = true;	/* remember this so we can set 'state->col_1'
				 * right */
	ttype = form_feed;
	break;

    case ',':
	unary_delim = true;
	ttype = comma;
	break;

    case '.':
	unary_delim = false;
	ttype = period;
	break;

    case '-':
    case '+':			/* check for -, +, --, ++ */
	ttype = state->last_u_d ? unary_op : binary_op;
	unary_delim = true;

	if (*buf_ptr == token.s[0]) {
	    /* check for doubled character */
	    *token.e++ = *buf_ptr++;
	    /* buffer overflow will be checked at end of loop */
	    if (state->last_token == ident || state->last_token == rparen) {
		ttype = state->last_u_d ? unary_op : postfix_op;
		/* check for following ++ or -- */
		unary_delim = false;
	    }
	} else if (*buf_ptr == '=')
	    /* check for operator += */
	    *token.e++ = *buf_ptr++;
	else if (*buf_ptr == '>') {
	    /* check for operator -> */
	    *token.e++ = *buf_ptr++;
	    unary_delim = false;
	    ttype = unary_op;
	    state->want_blank = false;
	}
	break;			/* buffer overflow will be checked at end of
				 * switch */

    case '=':
	if (state->in_or_st)
	    state->block_init = true;
	if (*buf_ptr == '=') {	/* == */
	    *token.e++ = '=';	/* Flip =+ to += */
	    buf_ptr++;
	    *token.e = '\0';
	}
	ttype = binary_op;
	unary_delim = true;
	break;
	/* can drop thru!!! */

    case '>':
    case '<':
    case '!':			/* ops like <, <<, <=, !=, etc */
	if (*buf_ptr == '>' || *buf_ptr == '<' || *buf_ptr == '=')
	    *token.e++ = inbuf_next();
	if (*buf_ptr == '=')
	    *token.e++ = *buf_ptr++;
	ttype = state->last_u_d ? unary_op : binary_op;
	unary_delim = true;
	break;

    case '*':
	unary_delim = true;
	if (!state->last_u_d) {
	    if (*buf_ptr == '=')
		*token.e++ = *buf_ptr++;
	    ttype = binary_op;
	    break;
	}
	while (*buf_ptr == '*' || isspace((unsigned char)*buf_ptr)) {
	    if (*buf_ptr == '*') {
		check_size_token(1);
		*token.e++ = *buf_ptr;
	    }
	    inbuf_skip();
	}
	if (ps.in_decl) {
	    char *tp = buf_ptr;

	    while (isalpha((unsigned char)*tp) ||
		   isspace((unsigned char)*tp)) {
		if (++tp >= buf_end)
		    fill_buffer();
	    }
	    if (*tp == '(')
		ps.procname[0] = ' ';
	}
	ttype = unary_op;
	break;

    default:
	if (token.s[0] == '/' && (*buf_ptr == '*' || *buf_ptr == '/')) {
	    /* it is start of comment */
	    *token.e++ = inbuf_next();

	    ttype = comment;
	    unary_delim = state->last_u_d;
	    break;
	}
	while (token.e[-1] == *buf_ptr || *buf_ptr == '=') {
	    /*
	     * handle ||, &&, etc, and also things as in int *****i
	     */
	    check_size_token(1);
	    *token.e++ = inbuf_next();
	}
	ttype = state->last_u_d ? unary_op : binary_op;
	unary_delim = true;
    }

    if (buf_ptr >= buf_end)	/* check for input buffer empty */
	fill_buffer();
    state->last_u_d = unary_delim;
    check_size_token(1);
    *token.e = '\0';
    return lexi_end(ttype);
}

static int
insert_pos(const char *key, const char **arr, unsigned int len)
{
    int lo = 0;
    int hi = (int)len - 1;

    while (lo <= hi) {
	int mid = (int)((unsigned)(lo + hi) >> 1);
	int cmp = strcmp(arr[mid], key);
	if (cmp < 0)
	    lo = mid + 1;
	else if (cmp > 0)
	    hi = mid - 1;
	else
	    return mid;
    }
    return -(lo + 1);
}

void
add_typename(const char *name)
{
    if (typenames.len >= typenames.cap) {
	typenames.cap = 16 + 2 * typenames.cap;
	typenames.items = xrealloc(typenames.items,
	    sizeof(typenames.items[0]) * typenames.cap);
    }

    int pos = insert_pos(name, typenames.items, typenames.len);
    if (pos >= 0)
	return;			/* already in the list */
    pos = -(pos + 1);
    memmove(typenames.items + pos + 1, typenames.items + pos,
	sizeof(typenames.items[0]) * (typenames.len++ - pos));
    typenames.items[pos] = xstrdup(name);
}
