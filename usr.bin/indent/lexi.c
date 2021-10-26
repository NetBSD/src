/*	$NetBSD: lexi.c,v 1.105 2021/10/26 20:43:35 rillig Exp $	*/

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
__RCSID("$NetBSD: lexi.c,v 1.105 2021/10/26 20:43:35 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/lexi.c 337862 2018-08-15 18:19:45Z pstef $");
#endif

#include <sys/param.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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
    {"do", kw_do},
    {"double", kw_type},
    {"else", kw_else},
    {"enum", kw_struct_or_union_or_enum},
    {"extern", kw_storage_class},
    {"float", kw_type},
    {"for", kw_for},
    {"goto", kw_jump},
    {"if", kw_if},
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
    {"while", kw_while}
};

static struct {
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
static const unsigned char lex_number_state[][26] = {
    /*                examples:
                                     00
             s                      0xx
             t                    00xaa
             a     11       101100xxa..
             r   11ee0001101lbuuxx.a.pp
             t.01.e+008bLuxll0Ll.aa.p+0
    states:  ABCDEFGHIJKLMNOPQRSTUVWXYZ */
    [0] =   "uuiifuufiuuiiuiiiiiuiuuuuu",	/* (other) */
    [1] =   "CEIDEHHHIJQ  U  Q  VUVVZZZ",	/* 0 */
    [2] =   "DEIDEHHHIJQ  U  Q  VUVVZZZ",	/* 1 */
    [3] =   "DEIDEHHHIJ   U     VUVVZZZ",	/* 2 3 4 5 6 7 */
    [4] =   "DEJDEHHHJJ   U     VUVVZZZ",	/* 8 9 */
    [5] =   "             U     VUVV   ",	/* A a C c D d */
    [6] =   "  K          U     VUVV   ",	/* B b */
    [7] =   "  FFF   FF   U     VUVV   ",	/* E e */
    [8] =   "    f  f     U     VUVV  f",	/* F f */
    [9] =   "  LLf  fL  PR   Li  L    f",	/* L */
    [10] =  "  OOf  fO   S P O i O    f",	/* l */
    [11] =  "                    FFX   ",	/* P p */
    [12] =  "  MM    M  i  iiM   M     ",	/* U u */
    [13] =  "  N                       ",	/* X x */
    [14] =  "     G                 Y  ",	/* + - */
    [15] =  "B EE    EE   T      W     ",	/* . */
    /*       ABCDEFGHIJKLMNOPQRSTUVWXYZ */
};
/* INDENT ON */

static const uint8_t lex_number_row[] = {
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
    return *inp.s;
}

void
inbuf_skip(void)
{
    inp.s++;
    if (inp.s >= inp.e)
	inbuf_read_line();
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

static void
token_add_char(char ch)
{
    check_size_token(1);
    *token.e++ = ch;
}

static int
cmp_keyword_by_name(const void *key, const void *elem)
{
    return strcmp(key, ((const struct keyword *)elem)->name);
}

#ifdef debug
static const char *
lsym_name(lexer_symbol sym)
{
    static const char *const name[] = {
	"eof",
	"preprocessing",
	"newline",
	"form_feed",
	"comment",
	"lparen_or_lbracket",
	"rparen_or_rbracket",
	"lbrace",
	"rbrace",
	"period",
	"unary_op",
	"binary_op",
	"postfix_op",
	"question",
	"colon",
	"comma",
	"semicolon",
	"typedef",
	"storage_class",
	"type",
	"tag",
	"case_label",
	"string_prefix",
	"ident",
	"funcname",
	"do",
	"else",
	"for",
	"if",
	"switch",
	"while",
    };

    assert(array_length(name) == (int)lsym_while + 1);

    return name[sym];
}

static const char *
kw_name(enum keyword_kind kw)
{
    static const char *name[] = {
	"0",
	"offsetof",
	"sizeof",
	"struct_or_union_or_enum",
	"type",
	"for",
	"if",
	"while",
	"do",
	"else",
	"switch",
	"case_or_default",
	"jump",
	"storage_class",
	"typedef",
	"inline_or_restrict",
    };

    return name[kw];
}

static void
debug_print_buf(const char *name, const struct buffer *buf)
{
    if (buf->s < buf->e) {
	debug_printf("%s ", name);
	debug_vis_range("\"", buf->s, buf->e, "\"\n");
    }
}

static void
debug_lexi(const struct parser_state *state, lexer_symbol lsym)
{
    debug_println("");
    debug_printf("line %d\n", line_no);
    if (state != &ps)
	debug_println("transient state");
    debug_print_buf("label", &lab);
    debug_print_buf("code", &code);
    debug_print_buf("comment", &com);
    debug_printf("lexi returns '%s'", lsym_name(lsym));
    if (state->curr_keyword != kw_0)
	debug_printf(" keyword '%s'", kw_name(state->curr_keyword));
    if (state->prev_keyword != kw_0)
	debug_printf(" previous keyword '%s'", kw_name(state->prev_keyword));
    debug_println("");
    debug_print_buf("token", &token);
}
#endif

/* ARGSUSED */
static lexer_symbol
lexi_end(const struct parser_state *state, lexer_symbol lsym)
{
#ifdef debug
    debug_lexi(state, lsym);
#endif
    return lsym;
}

static void
lex_number(void)
{
    for (uint8_t s = 'A'; s != 'f' && s != 'i' && s != 'u';) {
	uint8_t ch = (uint8_t)*inp.s;
	if (ch >= array_length(lex_number_row) || lex_number_row[ch] == 0)
	    break;

	uint8_t row = lex_number_row[ch];
	if (lex_number_state[row][s - 'A'] == ' ') {
	    /*-
	     * lex_number_state[0][s - 'A'] now indicates the type:
	     * f = floating, i = integer, u = unknown
	     */
	    break;
	}

	s = lex_number_state[row][s - 'A'];
	token_add_char(inbuf_next());
    }
}

static void
lex_word(void)
{
    while (isalnum((unsigned char)*inp.s) ||
	    *inp.s == '\\' ||
	    *inp.s == '_' || *inp.s == '$') {

	if (*inp.s == '\\') {
	    if (inp.s[1] == '\n') {
		inp.s += 2;
		if (inp.s >= inp.e)
		    inbuf_read_line();
	    } else
		break;
	}

	token_add_char(inbuf_next());
    }
}

static void
lex_char_or_string(void)
{
    for (char delim = *token.s;;) {
	if (*inp.s == '\n') {
	    diag(1, "Unterminated literal");
	    return;
	}

	token_add_char(inbuf_next());
	if (token.e[-1] == delim)
	    return;

	if (token.e[-1] == '\\') {
	    if (*inp.s == '\n')
		++line_no;
	    token_add_char(inbuf_next());
	}
    }
}

/* Guess whether the current token is a declared type. */
static bool
probably_typename(const struct parser_state *state)
{
    if (state->p_l_follow != 0)
	return false;
    if (state->block_init || state->in_stmt)
	return false;
    if (inp.s[0] == '*' && inp.s[1] != '=')
	goto maybe;
    if (isalpha((unsigned char)*inp.s))
	goto maybe;
    return false;
maybe:
    return state->last_token == lsym_semicolon ||
	state->last_token == lsym_lbrace ||
	state->last_token == lsym_rbrace;
}

static int
bsearch_typenames(const char *key)
{
    const char **arr = typenames.items;
    int lo = 0;
    int hi = (int)typenames.len - 1;

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

static bool
is_typename(void)
{
    if (opt.auto_typedefs &&
	token.e - token.s >= 2 && memcmp(token.e - 2, "_t", 2) == 0)
	return true;

    return bsearch_typenames(token.s) >= 0;
}

/* Read an alphanumeric token into 'token', or return end_of_file. */
static lexer_symbol
lexi_alnum(struct parser_state *state)
{
    if (isdigit((unsigned char)*inp.s) ||
	(inp.s[0] == '.' && isdigit((unsigned char)inp.s[1]))) {
	lex_number();
    } else if (isalnum((unsigned char)*inp.s) ||
	    *inp.s == '_' || *inp.s == '$') {
	lex_word();
    } else
	return lsym_eof;	/* just as a placeholder */

    *token.e = '\0';

    if (token.s[0] == 'L' && token.s[1] == '\0' &&
	(*inp.s == '"' || *inp.s == '\''))
	return lsym_string_prefix;

    while (is_hspace(inbuf_peek()))
	inbuf_skip();

    if (state->last_token == lsym_tag && state->p_l_follow == 0) {
	state->next_unary = true;
	return lsym_type;
    }

    /* Operator after identifier is binary unless last token was 'struct'. */
    state->next_unary = state->last_token == lsym_tag;

    const struct keyword *kw = bsearch(token.s, keywords,
	array_length(keywords), sizeof(keywords[0]), cmp_keyword_by_name);
    if (kw == NULL) {
	if (is_typename()) {
	    state->curr_keyword = kw_type;
	    state->next_unary = true;
	    goto found_typename;
	}

    } else {			/* we have a keyword */
	state->curr_keyword = kw->kind;
	state->next_unary = true;

	switch (kw->kind) {
	case kw_switch:
	    return lsym_switch;

	case kw_case_or_default:
	    return lsym_case_label;

	case kw_struct_or_union_or_enum:
	case kw_type:
    found_typename:
	    if (state->p_l_follow != 0) {
		/* inside parens: cast, param list, offsetof or sizeof */
		state->cast_mask |= (1 << state->p_l_follow) & ~state->not_cast_mask;
	    }
	    if (state->last_token == lsym_period ||
		    state->last_token == lsym_unary_op)
		break;
	    if (kw != NULL && kw->kind == kw_struct_or_union_or_enum)
		return lsym_tag;
	    if (state->p_l_follow != 0)
		break;
	    return lsym_type;

	case kw_for:
	    return lsym_for;

	case kw_if:
	    return lsym_if;

	case kw_while:
	    return lsym_while;

	case kw_do:
	    return lsym_do;

	case kw_else:
	    return lsym_else;

	case kw_storage_class:
	    return lsym_storage_class;

	case kw_typedef:
	    return lsym_typedef;

	default:		/* all others are treated like any other
				 * identifier */
	    return lsym_ident;
	}
    }

    if (*inp.s == '(' && state->tos <= 1 && state->ind_level == 0 &&
	!state->in_parameter_declaration && !state->block_init) {

	for (const char *p = inp.s; p < inp.e;)
	    if (*p++ == ')' && (*p == ';' || *p == ','))
		goto not_proc;

	strncpy(state->procname, token.s, sizeof state->procname - 1);
	if (state->in_decl)
	    state->in_parameter_declaration = true;
	return lsym_funcname;
not_proc:;

    } else if (probably_typename(state)) {
	state->curr_keyword = kw_type;
	state->next_unary = true;
	return lsym_type;
    }

    if (state->last_token == lsym_type)	/* if this is a declared variable,
					 * then following sign is unary */
	state->next_unary = true;	/* will make "int a -1" work */

    return lsym_ident;		/* the ident is not in the list */
}

/* Reads the next token, placing it in the global variable "token". */
lexer_symbol
lexi(struct parser_state *state)
{
    token.e = token.s;
    state->col_1 = state->last_nl;
    state->last_nl = false;
    state->prev_keyword = state->curr_keyword;
    state->curr_keyword = kw_0;

    while (is_hspace(*inp.s)) {
	state->col_1 = false;
	inbuf_skip();
    }

    lexer_symbol alnum_lsym = lexi_alnum(state);
    if (alnum_lsym != lsym_eof)
	return lexi_end(state, alnum_lsym);

    /* Scan a non-alphanumeric token */

    check_size_token(3);	/* for things like "<<=" */
    *token.e++ = inbuf_next();
    *token.e = '\0';

    lexer_symbol lsym;
    bool unary_delim = false;	/* whether the current token forces a
				 * following operator to be unary */

    switch (*token.s) {
    case '\n':
	unary_delim = state->next_unary;
	state->last_nl = true;	/* remember that we just had a newline */
	/* if data has been exhausted, the newline is a dummy. */
	lsym = had_eof ? lsym_eof : lsym_newline;
	break;

    case '\'':
    case '"':
	lex_char_or_string();
	lsym = lsym_ident;
	break;

    case '(':
    case '[':
	unary_delim = true;
	lsym = lsym_lparen_or_lbracket;
	break;

    case ')':
    case ']':
	lsym = lsym_rparen_or_rbracket;
	break;

    case '#':
	unary_delim = state->next_unary;
	lsym = lsym_preprocessing;
	break;

    case '?':
	unary_delim = true;
	lsym = lsym_question;
	break;

    case ':':
	lsym = lsym_colon;
	unary_delim = true;
	break;

    case ';':
	unary_delim = true;
	lsym = lsym_semicolon;
	break;

    case '{':
	unary_delim = true;
	lsym = lsym_lbrace;
	break;

    case '}':
	unary_delim = true;
	lsym = lsym_rbrace;
	break;

    case '\f':
	unary_delim = state->next_unary;
	state->last_nl = true;	/* remember this, so we can set 'state->col_1'
				 * right */
	lsym = lsym_form_feed;
	break;

    case ',':
	unary_delim = true;
	lsym = lsym_comma;
	break;

    case '.':
	unary_delim = false;
	lsym = lsym_period;
	break;

    case '-':
    case '+':
	lsym = state->next_unary ? lsym_unary_op : lsym_binary_op;
	unary_delim = true;

	if (*inp.s == token.s[0]) {	/* ++, -- */
	    *token.e++ = *inp.s++;
	    if (state->last_token == lsym_ident ||
		    state->last_token == lsym_rparen_or_rbracket) {
		lsym = state->next_unary ? lsym_unary_op : lsym_postfix_op;
		unary_delim = false;
	    }

	} else if (*inp.s == '=') {	/* += */
	    *token.e++ = *inp.s++;

	} else if (*inp.s == '>') {	/* -> */
	    *token.e++ = *inp.s++;
	    unary_delim = false;
	    lsym = lsym_unary_op;
	    state->want_blank = false;
	}
	break;

    case '=':
	if (state->init_or_struct)
	    state->block_init = true;
	if (*inp.s == '=') {	/* == */
	    *token.e++ = *inp.s++;
	    *token.e = '\0';
	}
	lsym = lsym_binary_op;
	unary_delim = true;
	break;

    case '>':
    case '<':
    case '!':			/* ops like <, <<, <=, !=, etc */
	if (*inp.s == '>' || *inp.s == '<' || *inp.s == '=')
	    *token.e++ = inbuf_next();
	if (*inp.s == '=')
	    *token.e++ = *inp.s++;
	lsym = state->next_unary ? lsym_unary_op : lsym_binary_op;
	unary_delim = true;
	break;

    case '*':
	unary_delim = true;
	if (!state->next_unary) {
	    if (*inp.s == '=')
		*token.e++ = *inp.s++;
	    lsym = lsym_binary_op;
	    break;
	}

	while (*inp.s == '*' || isspace((unsigned char)*inp.s)) {
	    if (*inp.s == '*')
		token_add_char('*');
	    inbuf_skip();
	}

	if (ps.in_decl) {
	    char *tp = inp.s;

	    while (isalpha((unsigned char)*tp) ||
		    isspace((unsigned char)*tp)) {
		if (++tp >= inp.e)
		    inbuf_read_line();
	    }
	    if (*tp == '(')
		ps.procname[0] = ' ';
	}

	lsym = lsym_unary_op;
	break;

    default:
	if (token.s[0] == '/' && (*inp.s == '*' || *inp.s == '/')) {
	    /* it is start of comment */
	    *token.e++ = inbuf_next();

	    lsym = lsym_comment;
	    unary_delim = state->next_unary;
	    break;
	}

	while (token.e[-1] == *inp.s || *inp.s == '=') {
	    /* handle '||', '&&', etc., and also things as in 'int *****i' */
	    token_add_char(inbuf_next());
	}

	lsym = state->next_unary ? lsym_unary_op : lsym_binary_op;
	unary_delim = true;
    }

    if (inp.s >= inp.e)		/* check for input buffer empty */
	inbuf_read_line();

    state->next_unary = unary_delim;

    check_size_token(1);
    *token.e = '\0';

    return lexi_end(state, lsym);
}

void
add_typename(const char *name)
{
    if (typenames.len >= typenames.cap) {
	typenames.cap = 16 + 2 * typenames.cap;
	typenames.items = xrealloc(typenames.items,
	    sizeof(typenames.items[0]) * typenames.cap);
    }

    int pos = bsearch_typenames(name);
    if (pos >= 0)
	return;			/* already in the list */

    pos = -(pos + 1);
    memmove(typenames.items + pos + 1, typenames.items + pos,
	sizeof(typenames.items[0]) * (typenames.len++ - (unsigned)pos));
    typenames.items[pos] = xstrdup(name);
}
