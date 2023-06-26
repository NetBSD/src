/*	$NetBSD: lexi.c,v 1.239 2023/06/26 20:23:40 rillig Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: lexi.c,v 1.239 2023/06/26 20:23:40 rillig Exp $");

#include <stdlib.h>
#include <string.h>

#include "indent.h"

/* must be sorted alphabetically, is used in binary search */
static const struct keyword {
	const char name[12];
	lexer_symbol lsym;
} keywords[] = {
	{"_Bool", lsym_type},
	{"_Complex", lsym_modifier},
	{"_Imaginary", lsym_modifier},
	{"auto", lsym_modifier},
	{"bool", lsym_type},
	{"break", lsym_word},
	{"case", lsym_case},
	{"char", lsym_type},
	{"complex", lsym_modifier},
	{"const", lsym_modifier},
	{"continue", lsym_word},
	{"default", lsym_default},
	{"do", lsym_do},
	{"double", lsym_type},
	{"else", lsym_else},
	{"enum", lsym_tag},
	{"extern", lsym_modifier},
	{"float", lsym_type},
	{"for", lsym_for},
	{"goto", lsym_word},
	{"if", lsym_if},
	{"imaginary", lsym_modifier},
	{"inline", lsym_modifier},
	{"int", lsym_type},
	{"long", lsym_type},
	{"offsetof", lsym_offsetof},
	{"register", lsym_modifier},
	{"restrict", lsym_word},
	{"return", lsym_return},
	{"short", lsym_type},
	{"signed", lsym_type},
	{"sizeof", lsym_sizeof},
	{"static", lsym_modifier},
	{"struct", lsym_tag},
	{"switch", lsym_switch},
	{"typedef", lsym_typedef},
	{"union", lsym_tag},
	{"unsigned", lsym_type},
	{"void", lsym_type},
	{"volatile", lsym_modifier},
	{"while", lsym_while}
};

static struct {
	const char **items;
	unsigned int len;
	unsigned int cap;
} typenames;

/*-
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

static const unsigned char lex_number_row[] = {
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


static bool
is_identifier_start(char ch)
{
	return ch_isalpha(ch) || ch == '_' || ch == '$';
}

static bool
is_identifier_part(char ch)
{
	return ch_isalnum(ch) || ch == '_' || ch == '$';
}

static void
token_add_char(char ch)
{
	buf_add_char(&token, ch);
}

static bool
skip_line_continuation(void)
{
	if (inp_p[0] == '\\' && inp_p[1] == '\n') {
		inp_p++;
		inp_skip();
		line_no++;
		return true;
	}
	return false;
}

static void
lex_number(void)
{
	for (unsigned char s = 'A'; s != 'f' && s != 'i' && s != 'u';) {
		unsigned char ch = (unsigned char)*inp_p;
		if (skip_line_continuation())
			continue;
		if (ch >= array_length(lex_number_row)
		    || lex_number_row[ch] == 0)
			break;

		unsigned char row = lex_number_row[ch];
		if (lex_number_state[row][s - 'A'] == ' ') {
			// lex_number_state[0][s - 'A'] now indicates the type:
			// f = floating, i = integer, u = unknown
			return;
		}

		s = lex_number_state[row][s - 'A'];
		token_add_char(inp_next());
	}
}

static void
lex_word(void)
{
	for (;;) {
		if (is_identifier_part(*inp_p))
			token_add_char(*inp_p++);
		else if (skip_line_continuation())
			continue;
		else
			return;
	}
}

static void
lex_char_or_string(void)
{
	for (char delim = token.s[token.len - 1];;) {
		if (*inp_p == '\n') {
			diag(1, "Unterminated literal");
			return;
		}

		token_add_char(*inp_p++);
		if (token.s[token.len - 1] == delim)
			return;

		if (token.s[token.len - 1] == '\\') {
			if (*inp_p == '\n')
				line_no++;
			token_add_char(inp_next());
		}
	}
}

/* Guess whether the current token is a declared type. */
static bool
probably_typename(void)
{
	if (ps.prev_lsym == lsym_modifier)
		return true;
	if (ps.in_init)
		return false;
	if (ps.in_stmt_or_decl)	/* XXX: this condition looks incorrect */
		return false;
	if (ps.prev_lsym == lsym_semicolon
	    || ps.prev_lsym == lsym_lbrace
	    || ps.prev_lsym == lsym_rbrace) {
		if (inp_p[0] == '*' && inp_p[1] != '=')
			return true;
		/* XXX: is_identifier_start */
		if (ch_isalpha(inp_p[0]))
			return true;
	}
	return false;
}

static int
bsearch_typenames(const char *key)
{
	const char **arr = typenames.items;
	unsigned lo = 0;
	unsigned hi = typenames.len;

	while (lo < hi) {
		unsigned mid = (lo + hi) / 2;
		int cmp = strcmp(arr[mid], key);
		if (cmp < 0)
			lo = mid + 1;
		else if (cmp > 0)
			hi = mid;
		else
			return (int)mid;
	}
	return -1 - (int)lo;
}

static bool
is_typename(void)
{
	if (ps.prev_lsym == lsym_tag)
		return true;
	if (opt.auto_typedefs &&
	    token.len >= 2 && memcmp(token.s + token.len - 2, "_t", 2) == 0)
		return true;

	return bsearch_typenames(token.s) >= 0;
}

void
register_typename(const char *name)
{
	if (typenames.len >= typenames.cap) {
		typenames.cap = 16 + 2 * typenames.cap;
		typenames.items = nonnull(realloc(typenames.items,
			sizeof(typenames.items[0]) * typenames.cap));
	}

	int pos = bsearch_typenames(name);
	if (pos >= 0)
		return;		/* already in the list */

	pos = -1 - pos;
	memmove(typenames.items + pos + 1, typenames.items + pos,
	    sizeof(typenames.items[0]) * (typenames.len++ - (unsigned)pos));
	typenames.items[pos] = nonnull(strdup(name));
}

static int
cmp_keyword_by_name(const void *key, const void *elem)
{
	return strcmp(key, ((const struct keyword *)elem)->name);
}

/*
 * Looking at the '(', guess whether this starts a function definition or a
 * function declaration.
 */
static bool
probably_function_definition(const char *p)
{
	// TODO: Don't look at characters in comments, see lsym_funcname.c.
	int paren_level = 0;
	for (; *p != '\n'; p++) {
		if (*p == '(')
			paren_level++;
		if (*p == ')' && --paren_level == 0) {
			p++;

			while (*p != '\n'
			    && (ch_isspace(*p) || is_identifier_part(*p)))
				p++;	/* '__dead' or '__unused' */

			if (*p == '\n')	/* func(...) */
				break;
			if (*p == ';')	/* func(...); */
				return false;
			if (*p == ',')	/* double abs(), pi; */
				return false;
			if (*p == '(')	/* func(...) __attribute__((...)) */
				paren_level++;	/* func(...) __printflike(...)
						 */
			else
				break;	/* func(...) { ... */
		}

		if (paren_level == 1 && p[0] == '*' && p[1] == ',')
			return false;
	}

	/*
	 * To further reduce the cases where indent wrongly treats an
	 * incomplete function declaration as a function definition, thus
	 * adding a newline before the function name, it may be worth looking
	 * for parameter names, as these are often omitted in function
	 * declarations and only included in function definitions. Or just
	 * increase the lookahead to more than just the current line of input,
	 * until the next '{'.
	 */
	return true;
}

static lexer_symbol
lexi_alnum(void)
{
	if (ch_isdigit(inp_p[0]) ||
	    (inp_p[0] == '.' && ch_isdigit(inp_p[1]))) {
		lex_number();
	} else if (is_identifier_start(inp_p[0])) {
		lex_word();

		if (token.len == 1 && token.s[0] == 'L' &&
		    (inp_p[0] == '"' || inp_p[0] == '\'')) {
			token_add_char(*inp_p++);
			lex_char_or_string();
			ps.next_unary = false;
			return lsym_word;
		}
	} else
		return lsym_eof;	/* just as a placeholder */

	while (ch_isblank(*inp_p))
		inp_p++;

	ps.next_unary = ps.prev_lsym == lsym_tag
	    || ps.prev_lsym == lsym_typedef
	    || (ps.prev_lsym == lsym_modifier && *inp_p == '*');

	if (ps.prev_lsym == lsym_tag && ps.paren.len == 0)
		return lsym_type;
	if (ps.spaced_expr_psym == psym_for_exprs
	    && ps.prev_lsym == lsym_lparen && ps.paren.len == 1
	    && *inp_p == '*') {
		ps.next_unary = true;
		return lsym_type;
	}

	token_add_char('\0');	// Terminate in non-debug mode as well.
	token.len--;
	const struct keyword *kw = bsearch(token.s, keywords,
	    array_length(keywords), sizeof(keywords[0]), cmp_keyword_by_name);
	lexer_symbol lsym = lsym_word;
	if (kw != NULL) {
		lsym = kw->lsym;
		ps.next_unary = true;
		if (lsym == lsym_tag || lsym == lsym_type)
			goto found_typename;
		return lsym;
	}

	if (is_typename()) {
		lsym = lsym_type;
		ps.next_unary = true;
found_typename:
		if (ps.prev_lsym != lsym_period
		    && ps.prev_lsym != lsym_unary_op) {
			if (lsym == lsym_tag)
				return lsym_tag;
			if (ps.paren.len == 0)
				return lsym_type;
		}
	}

	const char *p = inp_p;
	if (*p == ')')
		p++;
	if (*p == '(' && ps.psyms.len < 3 && ps.ind_level == 0 &&
	    !ps.in_func_def_params && !ps.in_init) {

		bool maybe_function_definition = *inp_p == ')'
		    ? ps.paren.len == 1 && ps.prev_lsym != lsym_unary_op
		    : ps.paren.len == 0;
		if (maybe_function_definition
		    && probably_function_definition(p)) {
			ps.line_has_func_def = true;
			if (ps.in_decl)
				ps.in_func_def_params = true;
			return lsym_funcname;
		}

	} else if (ps.paren.len == 0 && probably_typename()) {
		ps.next_unary = true;
		return lsym_type;
	}

	return lsym;
}

static void
check_parenthesized_function_definition(void)
{
	const char *p = inp_p;
	while (ch_isblank(*p))
		p++;
	if (is_identifier_start(*p))
		while (is_identifier_part(*p))
			p++;
	while (ch_isblank(*p))
		p++;
	if (*p == ')') {
		p++;
		while (ch_isblank(*p))
			p++;
		if (*p == '(' && probably_function_definition(p))
			ps.line_has_func_def = true;
	}
}

static bool
is_asterisk_unary(void)
{
	const char *p = inp_p;
	while (*p == '*' || ch_isblank(*p))
		p++;
	if (*p == ')')
		return true;
	if (ps.next_unary || ps.in_func_def_params)
		return true;
	if (ps.prev_lsym == lsym_word ||
	    ps.prev_lsym == lsym_rparen ||
	    ps.prev_lsym == lsym_rbracket)
		return false;
	return ps.in_decl && ps.paren.len > 0;
}

static bool
probably_in_function_definition(void)
{
	for (const char *p = inp_p; *p != '\n';) {
		if (ch_isspace(*p))
			p++;
		else if (is_identifier_start(*p)) {
			p++;
			while (is_identifier_part(*p))
				p++;
		} else
			return *p == '(';
	}
	return false;
}

static void
lex_asterisk_unary(void)
{
	while (*inp_p == '*' || ch_isspace(*inp_p)) {
		if (*inp_p == '*')
			token_add_char('*');
		inp_skip();
	}

	if (ps.in_decl && probably_in_function_definition())
		ps.line_has_func_def = true;
}

static bool
skip(const char **pp, const char *s)
{
	size_t len = strlen(s);
	while (ch_isblank(**pp))
		(*pp)++;
	if (strncmp(*pp, s, len) == 0) {
		*pp += len;
		return true;
	}
	return false;
}

static void
lex_indent_comment(void)
{
	const char *p = inp.s;
	if (skip(&p, "/*") && skip(&p, "INDENT")) {
		enum indent_enabled enabled;
		if (skip(&p, "ON") || *p == '*')
			enabled = indent_last_off_line;
		else if (skip(&p, "OFF"))
			enabled = indent_off;
		else
			return;
		if (skip(&p, "*/\n")) {
			if (lab.len > 0 || code.len > 0 || com.len > 0)
				output_line();
			indent_enabled = enabled;
		}
	}
}

/* Reads the next token, placing it in the global variable "token". */
lexer_symbol
lexi(void)
{
	buf_clear(&token);

	for (;;) {
		if (ch_isblank(*inp_p))
			inp_p++;
		else if (skip_line_continuation())
			continue;
		else
			break;
	}

	lexer_symbol alnum_lsym = lexi_alnum();
	if (alnum_lsym != lsym_eof)
		return alnum_lsym;

	/* Scan a non-alphanumeric token */

	token_add_char(inp_next());

	lexer_symbol lsym;
	bool next_unary;

	switch (token.s[token.len - 1]) {

	case '#':
		lsym = lsym_preprocessing;
		next_unary = ps.next_unary;
		break;

	case '\n':
		/* if data has been exhausted, the '\n' is a dummy. */
		lsym = had_eof ? lsym_eof : lsym_newline;
		next_unary = ps.next_unary;
		break;

	/* INDENT OFF */
	case ')':	lsym = lsym_rparen;	next_unary = false;	break;
	case '[':	lsym = lsym_lbracket;	next_unary = true;	break;
	case ']':	lsym = lsym_rbracket;	next_unary = false;	break;
	case '{':	lsym = lsym_lbrace;	next_unary = true;	break;
	case '}':	lsym = lsym_rbrace;	next_unary = true;	break;
	case '.':	lsym = lsym_period;	next_unary = false;	break;
	case '?':	lsym = lsym_question;	next_unary = true;	break;
	case ',':	lsym = lsym_comma;	next_unary = true;	break;
	case ';':	lsym = lsym_semicolon;	next_unary = true;	break;
	/* INDENT ON */

	case '(':
		if (inp_p == inp.s + 1)
			check_parenthesized_function_definition();
		lsym = lsym_lparen;
		next_unary = true;
		break;

	case '+':
	case '-':
		lsym = ps.next_unary ? lsym_unary_op : lsym_binary_op;
		next_unary = true;

		/* '++' or '--' */
		if (*inp_p == token.s[token.len - 1]) {
			token_add_char(*inp_p++);
			if (ps.prev_lsym == lsym_word ||
			    ps.prev_lsym == lsym_rparen ||
			    ps.prev_lsym == lsym_rbracket) {
				lsym = ps.next_unary
				    ? lsym_unary_op : lsym_postfix_op;
				next_unary = false;
			}

		} else if (*inp_p == '=') {	/* '+=' or '-=' */
			token_add_char(*inp_p++);

		} else if (*inp_p == '>') {	/* '->' */
			token_add_char(*inp_p++);
			lsym = lsym_unary_op;
			next_unary = false;
			ps.want_blank = false;
		}
		break;

	case ':':
		lsym = ps.quest_level > 0
		    ? (ps.quest_level--, lsym_question_colon)
		    : ps.in_var_decl ? lsym_other_colon : lsym_label_colon;
		next_unary = true;
		break;

	case '*':
		if (*inp_p == '=') {
			token_add_char(*inp_p++);
			lsym = lsym_binary_op;
		} else if (is_asterisk_unary()) {
			lex_asterisk_unary();
			lsym = lsym_unary_op;
		} else
			lsym = lsym_binary_op;
		next_unary = true;
		break;

	case '=':
		if (ps.in_var_decl)
			ps.in_init = true;
		if (*inp_p == '=')
			token_add_char(*inp_p++);
		lsym = lsym_binary_op;
		next_unary = true;
		break;

	case '>':
	case '<':
	case '!':		/* ops like <, <<, <=, !=, etc. */
		if (*inp_p == '>' || *inp_p == '<' || *inp_p == '=')
			token_add_char(*inp_p++);
		if (*inp_p == '=')
			token_add_char(*inp_p++);
		lsym = ps.next_unary ? lsym_unary_op : lsym_binary_op;
		next_unary = true;
		break;

	case '\'':
	case '"':
		lex_char_or_string();
		lsym = lsym_word;
		next_unary = false;
		break;

	default:
		if (token.s[token.len - 1] == '/'
		    && (*inp_p == '*' || *inp_p == '/')) {
			enum indent_enabled prev = indent_enabled;
			lex_indent_comment();
			if (prev == indent_on && indent_enabled == indent_off)
				buf_clear(&out.indent_off_text);
			token_add_char(*inp_p++);
			lsym = lsym_comment;
			next_unary = ps.next_unary;
			break;
		}

		/* punctuation like '%', '&&', '/', '^', '||', '~' */
		lsym = ps.next_unary ? lsym_unary_op : lsym_binary_op;
		if (*inp_p == token.s[token.len - 1])
			token_add_char(*inp_p++), lsym = lsym_binary_op;
		if (*inp_p == '=')
			token_add_char(*inp_p++), lsym = lsym_binary_op;

		next_unary = true;
	}

	ps.next_unary = next_unary;

	return lsym;
}
