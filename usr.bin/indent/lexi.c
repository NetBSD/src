/*	$NetBSD: lexi.c,v 1.43 2021/08/26 07:03:00 rillig Exp $	*/

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
#ifndef lint
static char sccsid[] = "@(#)lexi.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */
#endif

#include <sys/cdefs.h>
#ifndef lint
#if defined(__NetBSD__)
__RCSID("$NetBSD: lexi.c,v 1.43 2021/08/26 07:03:00 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/lexi.c 337862 2018-08-15 18:19:45Z pstef $");
#endif
#endif

/*
 * Here we have the token scanner for indent.  It scans off one token and puts
 * it in the global variable "token".  It returns a code, indicating the type
 * of token scanned.
 */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "indent.h"

struct templ {
    const char *rwd;
    enum rwcode rwcode;
};

/*
 * This table has to be sorted alphabetically, because it'll be used in binary
 * search.
 */
const struct templ specials[] =
{
    {"_Bool", rw_type},
    {"_Complex", rw_type},
    {"_Imaginary", rw_type},
    {"auto", rw_storage_class},
    {"bool", rw_type},
    {"break", rw_jump},
    {"case", rw_case_or_default},
    {"char", rw_type},
    {"complex", rw_type},
    {"const", rw_type},
    {"continue", rw_jump},
    {"default", rw_case_or_default},
    {"do", rw_do_or_else},
    {"double", rw_type},
    {"else", rw_do_or_else},
    {"enum", rw_struct_or_union_or_enum},
    {"extern", rw_storage_class},
    {"float", rw_type},
    {"for", rw_for_or_if_or_while},
    {"global", rw_type},
    {"goto", rw_jump},
    {"if", rw_for_or_if_or_while},
    {"imaginary", rw_type},
    {"inline", rw_inline_or_restrict},
    {"int", rw_type},
    {"long", rw_type},
    {"offsetof", rw_offsetof},
    {"register", rw_storage_class},
    {"restrict", rw_inline_or_restrict},
    {"return", rw_jump},
    {"short", rw_type},
    {"signed", rw_type},
    {"sizeof", rw_sizeof},
    {"static", rw_storage_class},
    {"struct", rw_struct_or_union_or_enum},
    {"switch", rw_switch},
    {"typedef", rw_typedef},
    {"union", rw_struct_or_union_or_enum},
    {"unsigned", rw_type},
    {"void", rw_type},
    {"volatile", rw_type},
    {"while", rw_for_or_if_or_while}
};

const char **typenames;
int         typename_count;
int         typename_top = -1;

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
static char const *table[] = {
    /*                examples:
                                     00
             s                      0xx
             t                    00xaa
             a     11       101100xxa..
             r   11ee0001101lbuuxx.a.pp
             t.01.e+008bLuxll0Ll.aa.p+0
    states:  ABCDEFGHIJKLMNOPQRSTUVWXYZ */
    ['0'] = "CEIDEHHHIJQ  U  Q  VUVVZZZ",
    ['1'] = "DEIDEHHHIJQ  U  Q  VUVVZZZ",
    ['7'] = "DEIDEHHHIJ   U     VUVVZZZ",
    ['9'] = "DEJDEHHHJJ   U     VUVVZZZ",
    ['a'] = "             U     VUVV   ",
    ['b'] = "  K          U     VUVV   ",
    ['e'] = "  FFF   FF   U     VUVV   ",
    ['f'] = "    f  f     U     VUVV  f",
    ['u'] = "  MM    M  i  iiM   M     ",
    ['x'] = "  N                       ",
    ['p'] = "                    FFX   ",
    ['L'] = "  LLf  fL  PR   Li  L    f",
    ['l'] = "  OOf  fO   S P O i O    f",
    ['+'] = "     G                 Y  ",
    ['.'] = "B EE    EE   T      W     ",
    /*       ABCDEFGHIJKLMNOPQRSTUVWXYZ */
    [0]   = "uuiifuufiuuiiuiiiiiuiuuuuu",
};

/* Initialize constant transition table */
void
init_constant_tt(void)
{
    table['-'] = table['+'];
    table['8'] = table['9'];
    table['2'] = table['3'] = table['4'] = table['5'] = table['6'] = table['7'];
    table['A'] = table['C'] = table['D'] = table['c'] = table['d'] = table['a'];
    table['B'] = table['b'];
    table['E'] = table['e'];
    table['U'] = table['u'];
    table['X'] = table['x'];
    table['P'] = table['p'];
    table['F'] = table['f'];
}

static char
inbuf_peek(void)
{
    return *buf_ptr;
}

static void
inbuf_skip(void)
{
    buf_ptr++;
    if (buf_ptr >= buf_end)
	fill_buffer();
}

static char
inbuf_next(void)
{
    char ch = inbuf_peek();
    inbuf_skip();
    return ch;
}

static void
check_size_token(size_t desired_size)
{
    if (e_token + (desired_size) < l_token)
        return;

    size_t nsize = l_token - s_token + 400 + desired_size;
    size_t token_len = e_token - s_token;
    tokenbuf = realloc(tokenbuf, nsize);
    if (tokenbuf == NULL)
	err(1, NULL);
    e_token = tokenbuf + token_len + 1;
    l_token = tokenbuf + nsize - 5;
    s_token = tokenbuf + 1;
}

static int
compare_templ_array(const void *key, const void *elem)
{
    return strcmp(key, ((const struct templ *)elem)->rwd);
}

static int
compare_string_array(const void *key, const void *elem)
{
    return strcmp(key, *((const char *const *)elem));
}

#ifdef debug
const char *
token_type_name(token_type tk)
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

    assert(0 <= tk && tk < sizeof name / sizeof name[0]);

    return name[tk];
}

static void
print_buf(const char *name, const char *s, const char *e)
{
    if (s < e) {
	debug_printf(" %s ", name);
	debug_vis_range("\"", s, e, "\"");
    }
}

static token_type
lexi_end(token_type code)
{
    debug_printf("in line %d, lexi returns '%s'",
	line_no, token_type_name(code));
    print_buf("token", s_token, e_token);
    print_buf("label", s_lab, e_lab);
    print_buf("code", s_code, e_code);
    print_buf("comment", s_com, e_com);
    debug_printf("\n");

    return code;
}
#else
#  define lexi_end(tk) (tk)
#endif

static void
lex_number(void)
{
    char s;
    unsigned char i;

    for (s = 'A'; s != 'f' && s != 'i' && s != 'u'; ) {
	i = (unsigned char)*buf_ptr;
	if (i >= nitems(table) || table[i] == NULL ||
	    table[i][s - 'A'] == ' ') {
	    s = table[0][s - 'A'];
	    break;
	}
	s = table[i][s - 'A'];
	check_size_token(1);
	*e_token++ = inbuf_next();
    }
    /* s now indicates the type: f(loating), i(integer), u(nknown) */
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
	*e_token++ = inbuf_next();
    }
}

static void
lex_char_or_string(void)
{
    char delim;

    delim = *token;
    do {			/* copy the string */
	for (;;) {		/* move one character or [/<char>]<char> */
	    if (*buf_ptr == '\n') {
		diag(1, "Unterminated literal");
		return;
	    }
	    check_size_token(2);
	    *e_token = inbuf_next();
	    if (*e_token == '\\') {	/* if escape, copy extra char */
		if (*buf_ptr == '\n')	/* check for escaped newline */
		    ++line_no;
		*++e_token = inbuf_next();
		++e_token;	/* we must increment this again because we
				 * copied two chars */
	    } else
		break;		/* we copied one character */
	}			/* end of while (1) */
    } while (*e_token++ != delim);
}

token_type
lexi(struct parser_state *state)
{
    int         unary_delim;	/* this is set to 1 if the current token
				 * forces a following operator to be unary */
    token_type  code;		/* internal code to be returned */

    e_token = s_token;		/* point to start of place to save token */
    unary_delim = false;
    state->col_1 = state->last_nl;	/* tell world that this token started
					 * in column 1 iff the last thing
					 * scanned was a newline */
    state->last_nl = false;

    while (*buf_ptr == ' ' || *buf_ptr == '\t') {	/* get rid of blanks */
	state->col_1 = false;	/* leading blanks imply token is not in column
				 * 1 */
	inbuf_skip();
    }

    /* Scan an alphanumeric token */
    if (isalnum((unsigned char)*buf_ptr) ||
	*buf_ptr == '_' || *buf_ptr == '$' ||
	(buf_ptr[0] == '.' && isdigit((unsigned char)buf_ptr[1]))) {
	/*
	 * we have a letter or number
	 */
	struct templ *p;

	if (isdigit((unsigned char)*buf_ptr) ||
	    (buf_ptr[0] == '.' && isdigit((unsigned char)buf_ptr[1]))) {
	    lex_number();
	} else {
	    lex_word();
	}
	*e_token = '\0';

	if (s_token[0] == 'L' && s_token[1] == '\0' &&
	      (*buf_ptr == '"' || *buf_ptr == '\''))
	    return lexi_end(string_prefix);

	while (*buf_ptr == ' ' || *buf_ptr == '\t')	/* get rid of blanks */
	    inbuf_next();
	state->keyword = rw_0;
	if (state->last_token == keyword_struct_union_enum &&
	    !state->p_l_follow) {
	    /* if last token was 'struct' and we're not in parentheses, then
	     * this token should be treated as a declaration */
	    state->last_u_d = true;
	    return lexi_end(decl);
	}
	/*
	 * Operator after identifier is binary unless last token was 'struct'
	 */
	state->last_u_d = (state->last_token == keyword_struct_union_enum);

	p = bsearch(s_token, specials, sizeof specials / sizeof specials[0],
	    sizeof specials[0], compare_templ_array);
	if (p == NULL) {	/* not a special keyword... */
	    char *u;

	    /* ... so maybe a type_t or a typedef */
	    if ((opt.auto_typedefs && ((u = strrchr(s_token, '_')) != NULL) &&
	        strcmp(u, "_t") == 0) || (typename_top >= 0 &&
		  bsearch(s_token, typenames, (size_t)typename_top + 1,
		    sizeof typenames[0], compare_string_array))) {
		state->keyword = rw_type;
		state->last_u_d = true;
	        goto found_typename;
	    }
	} else {			/* we have a keyword */
	    state->keyword = p->rwcode;
	    state->last_u_d = true;
	    switch (p->rwcode) {
	    case rw_switch:
		return lexi_end(switch_expr);
	    case rw_case_or_default:
		return lexi_end(case_label);
	    case rw_struct_or_union_or_enum:
	    case rw_type:
	    found_typename:
		if (state->p_l_follow) {
		    /* inside parens: cast, param list, offsetof or sizeof */
		    state->cast_mask |= (1 << state->p_l_follow) & ~state->not_cast_mask;
		}
		if (state->last_token == period || state->last_token == unary_op) {
		    state->keyword = rw_0;
		    break;
		}
		if (p != NULL && p->rwcode == rw_struct_or_union_or_enum)
		    return lexi_end(keyword_struct_union_enum);
		if (state->p_l_follow)
		    break;
		return lexi_end(decl);

	    case rw_for_or_if_or_while:
		return lexi_end(keyword_for_if_while);

	    case rw_do_or_else:
		return lexi_end(keyword_do_else);

	    case rw_storage_class:
		return lexi_end(storage_class);

	    case rw_typedef:
		return lexi_end(type_def);

	    default:		/* all others are treated like any other
				 * identifier */
		return lexi_end(ident);
	    }			/* end of switch */
	}			/* end of if (found_it) */
	if (*buf_ptr == '(' && state->tos <= 1 && state->ind_level == 0 &&
	    state->in_parameter_declaration == 0 && state->block_init == 0) {
	    char *tp = buf_ptr;
	    while (tp < buf_end)
		if (*tp++ == ')' && (*tp == ';' || *tp == ','))
		    goto not_proc;
	    strncpy(state->procname, token, sizeof state->procname - 1);
	    if (state->in_decl)
		state->in_parameter_declaration = 1;
	    return lexi_end(funcname);
    not_proc:;
	}
	/*
	 * The following hack attempts to guess whether or not the current
	 * token is in fact a declaration keyword -- one that has been
	 * typedefd
	 */
	else if (!state->p_l_follow && !state->block_init &&
	    !state->in_stmt &&
	    ((*buf_ptr == '*' && buf_ptr[1] != '=') ||
		isalpha((unsigned char)*buf_ptr)) &&
	    (state->last_token == semicolon || state->last_token == lbrace ||
		state->last_token == rbrace)) {
	    state->keyword = rw_type;
	    state->last_u_d = true;
	    return lexi_end(decl);
	}
	if (state->last_token == decl)	/* if this is a declared variable,
					 * then following sign is unary */
	    state->last_u_d = true;	/* will make "int a -1" work */
	return lexi_end(ident);		/* the ident is not in the list */
    }				/* end of procesing for alpanum character */

    /* Scan a non-alphanumeric token */

    check_size_token(3);	/* things like "<<=" */
    *e_token++ = inbuf_next();	/* if it is only a one-character token, it is
				 * moved here */
    *e_token = '\0';

    switch (*token) {
    case '\n':
	unary_delim = state->last_u_d;
	state->last_nl = true;	/* remember that we just had a newline */
	code = (had_eof ? end_of_file : newline);

	/*
	 * if data has been exhausted, the newline is a dummy, and we should
	 * return code to stop
	 */
	break;

    case '\'':
    case '"':
    	lex_char_or_string();
	code = ident;
	break;

    case '(':
    case '[':
	unary_delim = true;
	code = lparen;
	break;

    case ')':
    case ']':
	code = rparen;
	break;

    case '#':
	unary_delim = state->last_u_d;
	code = preprocessing;
	break;

    case '?':
	unary_delim = true;
	code = question;
	break;

    case ':':
	code = colon;
	unary_delim = true;
	break;

    case ';':
	unary_delim = true;
	code = semicolon;
	break;

    case '{':
	unary_delim = true;

	/*
	 * if (state->in_or_st) state->block_init = 1;
	 */
	/* ?	code = state->block_init ? lparen : lbrace; */
	code = lbrace;
	break;

    case '}':
	unary_delim = true;
	/* ?	code = state->block_init ? rparen : rbrace; */
	code = rbrace;
	break;

    case 014:			/* a form feed */
	unary_delim = state->last_u_d;
	state->last_nl = true;	/* remember this so we can set 'state->col_1'
				 * right */
	code = form_feed;
	break;

    case ',':
	unary_delim = true;
	code = comma;
	break;

    case '.':
	unary_delim = false;
	code = period;
	break;

    case '-':
    case '+':			/* check for -, +, --, ++ */
	code = (state->last_u_d ? unary_op : binary_op);
	unary_delim = true;

	if (*buf_ptr == token[0]) {
	    /* check for doubled character */
	    *e_token++ = *buf_ptr++;
	    /* buffer overflow will be checked at end of loop */
	    if (state->last_token == ident || state->last_token == rparen) {
		code = (state->last_u_d ? unary_op : postfix_op);
		/* check for following ++ or -- */
		unary_delim = false;
	    }
	} else if (*buf_ptr == '=')
	    /* check for operator += */
	    *e_token++ = *buf_ptr++;
	else if (*buf_ptr == '>') {
	    /* check for operator -> */
	    *e_token++ = *buf_ptr++;
	    unary_delim = false;
	    code = unary_op;
	    state->want_blank = false;
	}
	break;			/* buffer overflow will be checked at end of
				 * switch */

    case '=':
	if (state->in_or_st)
	    state->block_init = 1;
	if (*buf_ptr == '=') {	/* == */
	    *e_token++ = '=';	/* Flip =+ to += */
	    buf_ptr++;
	    *e_token = 0;
	}
	code = binary_op;
	unary_delim = true;
	break;
	/* can drop thru!!! */

    case '>':
    case '<':
    case '!':			/* ops like <, <<, <=, !=, etc */
	if (*buf_ptr == '>' || *buf_ptr == '<' || *buf_ptr == '=')
	    *e_token++ = inbuf_next();
	if (*buf_ptr == '=')
	    *e_token++ = *buf_ptr++;
	code = (state->last_u_d ? unary_op : binary_op);
	unary_delim = true;
	break;

    case '*':
	unary_delim = true;
	if (!state->last_u_d) {
	    if (*buf_ptr == '=')
		*e_token++ = *buf_ptr++;
	    code = binary_op;
	    break;
	}
	while (*buf_ptr == '*' || isspace((unsigned char)*buf_ptr)) {
	    if (*buf_ptr == '*') {
		check_size_token(1);
		*e_token++ = *buf_ptr;
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
	code = unary_op;
	break;

    default:
	if (token[0] == '/' && (*buf_ptr == '*' || *buf_ptr == '/')) {
	    /* it is start of comment */
	    *e_token++ = inbuf_next();

	    code = comment;
	    unary_delim = state->last_u_d;
	    break;
	}
	while (e_token[-1] == *buf_ptr || *buf_ptr == '=') {
	    /*
	     * handle ||, &&, etc, and also things as in int *****i
	     */
	    check_size_token(1);
	    *e_token++ = inbuf_next();
	}
	code = (state->last_u_d ? unary_op : binary_op);
	unary_delim = true;


    }				/* end of switch */
    if (buf_ptr >= buf_end)	/* check for input buffer empty */
	fill_buffer();
    state->last_u_d = unary_delim;
    check_size_token(1);
    *e_token = '\0';		/* null terminate the token */
    return lexi_end(code);
}

void
alloc_typenames(void)
{

    typenames = malloc(sizeof(typenames[0]) * (typename_count = 16));
    if (typenames == NULL)
	err(1, NULL);
}

void
add_typename(const char *key)
{
    int comparison;
    const char *copy;

    if (typename_top + 1 >= typename_count) {
	typenames = realloc((void *)typenames,
	    sizeof(typenames[0]) * (typename_count *= 2));
	if (typenames == NULL)
	    err(1, NULL);
    }
    if (typename_top == -1)
	typenames[++typename_top] = copy = strdup(key);
    else if ((comparison = strcmp(key, typenames[typename_top])) >= 0) {
	/* take advantage of sorted input */
	if (comparison == 0)	/* remove duplicates */
	    return;
	typenames[++typename_top] = copy = strdup(key);
    } else {
	int p;

	for (p = 0; (comparison = strcmp(key, typenames[p])) > 0; p++)
	    /* find place for the new key */;
	if (comparison == 0)	/* remove duplicates */
	    return;
	memmove(&typenames[p + 1], &typenames[p],
	    sizeof(typenames[0]) * (++typename_top - p));
	typenames[p] = copy = strdup(key);
    }

    if (copy == NULL)
	err(1, NULL);
}
