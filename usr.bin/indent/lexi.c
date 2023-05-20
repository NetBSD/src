/*	$NetBSD: lexi.c,v 1.202 2023/05/20 11:53:53 rillig Exp $	*/

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
__RCSID("$NetBSD: lexi.c,v 1.202 2023/05/20 11:53:53 rillig Exp $");

#include <stdlib.h>
#include <string.h>

#include "indent.h"

/* In lexi_alnum, this constant marks a type, independent of parentheses. */
#define lsym_type lsym_type_outside_parentheses
#define lsym_type_modifier lsym_storage_class

/* must be sorted alphabetically, is used in binary search */
static const struct keyword {
	const char name[12];
	lexer_symbol lsym;
} keywords[] = {
	{"_Bool", lsym_type},
	{"_Complex", lsym_type},
	{"_Imaginary", lsym_type},
	{"auto", lsym_storage_class},
	{"bool", lsym_type},
	{"break", lsym_word},
	{"case", lsym_case_label},
	{"char", lsym_type},
	{"complex", lsym_type},
	{"const", lsym_type_modifier},
	{"continue", lsym_word},
	{"default", lsym_case_label},
	{"do", lsym_do},
	{"double", lsym_type},
	{"else", lsym_else},
	{"enum", lsym_tag},
	{"extern", lsym_storage_class},
	{"float", lsym_type},
	{"for", lsym_for},
	{"goto", lsym_word},
	{"if", lsym_if},
	{"imaginary", lsym_type},
	{"inline", lsym_word},
	{"int", lsym_type},
	{"long", lsym_type},
	{"offsetof", lsym_offsetof},
	{"register", lsym_storage_class},
	{"restrict", lsym_word},
	{"return", lsym_return},
	{"short", lsym_type},
	{"signed", lsym_type},
	{"sizeof", lsym_sizeof},
	{"static", lsym_storage_class},
	{"struct", lsym_tag},
	{"switch", lsym_switch},
	{"typedef", lsym_typedef},
	{"union", lsym_tag},
	{"unsigned", lsym_type},
	{"void", lsym_type},
	{"volatile", lsym_type_modifier},
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

static void
token_add_char(char ch)
{
	buf_add_char(&token, ch);
}

static void
lex_number(void)
{
	for (unsigned char s = 'A'; s != 'f' && s != 'i' && s != 'u';) {
		unsigned char ch = (unsigned char)inp.st[0];
		if (ch == '\\' && inp.st[1] == '\n') {
			inp.st++;
			inp_skip();
			line_no++;
			continue;
		}
		if (ch >= array_length(lex_number_row)
		    || lex_number_row[ch] == 0)
			break;

		unsigned char row = lex_number_row[ch];
		if (lex_number_state[row][s - 'A'] == ' ') {
			/*-
		         * lex_number_state[0][s - 'A'] now indicates the type:
		         * f = floating, i = integer, u = unknown
		         */
			return;
		}

		s = lex_number_state[row][s - 'A'];
		token_add_char(inp_next());
	}
}

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
lex_word(void)
{
	for (;;) {
		if (is_identifier_part(inp.st[0]))
			token_add_char(*inp.st++);
		else if (inp.st[0] == '\\' && inp.st[1] == '\n') {
			inp.st++;
			inp_skip();
			line_no++;
		} else
			return;
	}
}

static void
lex_char_or_string(void)
{
	for (char delim = token.mem[token.len - 1];;) {
		if (inp.st[0] == '\n') {
			diag(1, "Unterminated literal");
			return;
		}

		token_add_char(*inp.st++);
		if (token.mem[token.len - 1] == delim)
			return;

		if (token.mem[token.len - 1] == '\\') {
			if (inp.st[0] == '\n')
				++line_no;
			token_add_char(inp_next());
		}
	}
}

/* Guess whether the current token is a declared type. */
static bool
probably_typename(void)
{
	if (ps.prev_token == lsym_storage_class)
		return true;
	if (ps.block_init)
		return false;
	if (ps.in_stmt_or_decl)	/* XXX: this condition looks incorrect */
		return false;
	if (inp.st[0] == '*' && inp.st[1] != '=')
		goto maybe;
	/* XXX: is_identifier_start */
	if (ch_isalpha(inp.st[0]))
		goto maybe;
	return false;
maybe:
	return ps.prev_token == lsym_semicolon ||
	    ps.prev_token == lsym_lbrace ||
	    ps.prev_token == lsym_rbrace;
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
	    token.len >= 2 && memcmp(token.mem + token.len - 2, "_t", 2) == 0)
		return true;

	return bsearch_typenames(token.st) >= 0;
}

static int
cmp_keyword_by_name(const void *key, const void *elem)
{
	return strcmp(key, ((const struct keyword *)elem)->name);
}

/*
 * Looking at something like 'function_name(...)' in a line, guess whether
 * this starts a function definition or a declaration.
 */
static bool
probably_looking_at_definition(void)
{
	int paren_level = 0;
	for (const char *p = inp.st; *p != '\n'; p++) {
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
	}

	/* To further reduce the cases where indent wrongly treats an
	 * incomplete function declaration as a function definition, thus
	 * adding a newline before the function name, it may be worth looking
	 * for parameter names, as these are often omitted in function
	 * declarations and only included in function definitions. Or just
	 * increase the lookahead to more than just the current line of input,
	 * until the next '{'. */
	return true;
}

/* Read an alphanumeric token into 'token', or return lsym_eof. */
static lexer_symbol
lexi_alnum(void)
{
	if (ch_isdigit(inp.st[0]) ||
	    (inp.st[0] == '.' && ch_isdigit(inp.st[1]))) {
		lex_number();
	} else if (is_identifier_start(inp.st[0])) {
		lex_word();

		if (token.len == 1 && token.st[0] == 'L' &&
		    (inp.st[0] == '"' || inp.st[0] == '\'')) {
			token_add_char(*inp.st++);
			lex_char_or_string();
			ps.next_unary = false;
			return lsym_word;
		}
	} else
		return lsym_eof;	/* just as a placeholder */

	while (ch_isblank(inp.st[0]))
		inp.st++;

	ps.next_unary = ps.prev_token == lsym_tag
	    || ps.prev_token == lsym_typedef;

	if (ps.prev_token == lsym_tag && ps.nparen == 0)
		return lsym_type_outside_parentheses;

	token_add_char('\0');
	token.len--;
	const struct keyword *kw = bsearch(token.st, keywords,
	    array_length(keywords), sizeof(keywords[0]), cmp_keyword_by_name);
	lexer_symbol lsym = lsym_word;
	if (kw != NULL) {
		if (kw->lsym == lsym_type)
			lsym = lsym_type_in_parentheses;
		ps.next_unary = true;
		if (kw->lsym == lsym_tag || kw->lsym == lsym_type)
			goto found_typename;
		return kw->lsym;
	}

	if (is_typename()) {
		lsym = lsym_type_in_parentheses;
		ps.next_unary = true;
		if (ps.in_enum == in_enum_enum)
			ps.in_enum = in_enum_type;
found_typename:
		if (ps.nparen > 0) {
			/* inside parentheses: cast, param list, offsetof or
			 * sizeof */
			if (ps.paren[ps.nparen - 1].cast == cast_unknown)
				ps.paren[ps.nparen - 1].cast = cast_maybe;
		}
		if (ps.prev_token != lsym_period
		    && ps.prev_token != lsym_unary_op) {
			if (kw != NULL && kw->lsym == lsym_tag) {
				if (token.st[0] == 'e' /* enum */)
					ps.in_enum = in_enum_enum;
				return lsym_tag;
			}
			if (ps.nparen == 0)
				return lsym_type_outside_parentheses;
		}
	}

	if (inp.st[0] == '(' && ps.tos <= 1 && ps.ind_level == 0 &&
	    !ps.in_func_def_params && !ps.block_init) {

		if (ps.nparen == 0 && probably_looking_at_definition()) {
			ps.is_function_definition = true;
			if (ps.in_decl)
				ps.in_func_def_params = true;
			return lsym_funcname;
		}

	} else if (ps.nparen == 0 && probably_typename()) {
		ps.next_unary = true;
		return lsym_type_outside_parentheses;
	}

	return lsym;
}

static bool
is_asterisk_unary(void)
{
	if (ps.next_unary || ps.in_func_def_params)
		return true;
	if (ps.prev_token == lsym_word ||
	    ps.prev_token == lsym_rparen_or_rbracket)
		return false;
	return ps.in_decl && ps.nparen > 0;
}

static bool
probably_in_function_definition(void)
{
	for (const char *tp = inp.st; *tp != '\n';) {
		if (ch_isspace(*tp))
			tp++;
		else if (is_identifier_start(*tp)) {
			tp++;
			while (is_identifier_part(*tp))
				tp++;
		} else
			return *tp == '(';
	}
	return false;
}

static void
lex_asterisk_unary(void)
{
	while (inp.st[0] == '*' || ch_isspace(inp.st[0])) {
		if (inp.st[0] == '*')
			token_add_char('*');
		inp_skip();
	}

	if (ps.in_decl && probably_in_function_definition())
		ps.is_function_definition = true;
}

static void
skip_blank(const char **pp)
{
	while (ch_isblank(**pp))
		(*pp)++;
}

static bool
skip_string(const char **pp, const char *s)
{
	size_t len = strlen(s);
	if (strncmp(*pp, s, len) == 0) {
		*pp += len;
		return true;
	}
	return false;
}

static void
lex_indent_comment(void)
{
	const char *p = inp.mem;

	skip_blank(&p);
	if (!skip_string(&p, "/*"))
		return;
	skip_blank(&p);
	if (!skip_string(&p, "INDENT"))
		return;

	enum indent_enabled enabled;
	skip_blank(&p);
	if (*p == '*' || skip_string(&p, "ON"))
		enabled = indent_last_off_line;
	else if (skip_string(&p, "OFF"))
		enabled = indent_off;
	else
		return;

	skip_blank(&p);
	if (!skip_string(&p, "*/\n"))
		return;

	if (lab.len > 0 || code.len > 0 || com.len > 0)
		output_line();

	indent_enabled = enabled;
}

/* Reads the next token, placing it in the global variable "token". */
lexer_symbol
lexi(void)
{
	token.len = 0;
	ps.curr_col_1 = ps.next_col_1;
	ps.next_col_1 = false;

	for (;;) {
		if (ch_isblank(inp.st[0])) {
			ps.curr_col_1 = false;
			inp.st++;
		} else if (inp.st[0] == '\\' && inp.st[1] == '\n') {
			inp.st++;
			inp_skip();
			line_no++;
		} else
			break;
	}

	lexer_symbol alnum_lsym = lexi_alnum();
	if (alnum_lsym != lsym_eof) {
		debug_parser_state(alnum_lsym);
		return alnum_lsym;
	}

	/* Scan a non-alphanumeric token */

	token_add_char(inp_next());

	lexer_symbol lsym;
	bool next_unary;

	switch (token.mem[token.len - 1]) {

    /* INDENT OFF */
    case '(':
    case '[':	lsym = lsym_lparen_or_lbracket;	next_unary = true;	break;
    case ')':
    case ']':	lsym = lsym_rparen_or_rbracket;	next_unary = false;	break;
    case '?':	lsym = lsym_question;		next_unary = true;	break;
    case ':':	lsym = lsym_colon;		next_unary = true;	break;
    case ';':	lsym = lsym_semicolon;		next_unary = true;	break;
    case '{':	lsym = lsym_lbrace;		next_unary = true;	break;
    case '}':	lsym = lsym_rbrace;		next_unary = true;	break;
    case ',':	lsym = lsym_comma;		next_unary = true;	break;
    case '.':	lsym = lsym_period;		next_unary = false;	break;
    /* INDENT ON */

	case '\n':
		/* if data has been exhausted, the '\n' is a dummy. */
		lsym = had_eof ? lsym_eof : lsym_newline;
		next_unary = ps.next_unary;
		ps.next_col_1 = true;
		break;

	case '#':
		lsym = lsym_preprocessing;
		next_unary = ps.next_unary;
		break;

	case '\'':
	case '"':
		lex_char_or_string();
		lsym = lsym_word;
		next_unary = false;
		break;

	case '-':
	case '+':
		lsym = ps.next_unary ? lsym_unary_op : lsym_binary_op;
		next_unary = true;

		/* '++' or '--' */
		if (inp.st[0] == token.mem[token.len - 1]) {
			token_add_char(*inp.st++);
			if (ps.prev_token == lsym_word ||
			    ps.prev_token == lsym_rparen_or_rbracket) {
				lsym = ps.next_unary
				    ? lsym_unary_op : lsym_postfix_op;
				next_unary = false;
			}

		} else if (inp.st[0] == '=') {	/* '+=' or '-=' */
			token_add_char(*inp.st++);

		} else if (inp.st[0] == '>') {	/* '->' */
			token_add_char(*inp.st++);
			lsym = lsym_unary_op;
			next_unary = false;
			ps.want_blank = false;
		}
		break;

	case '=':
		if (ps.init_or_struct)
			ps.block_init = true;
		if (inp.st[0] == '=')
			token_add_char(*inp.st++);
		lsym = lsym_binary_op;
		next_unary = true;
		break;

	case '>':
	case '<':
	case '!':		/* ops like <, <<, <=, !=, etc */
		if (inp.st[0] == '>' || inp.st[0] == '<' || inp.st[0] == '=')
			token_add_char(*inp.st++);
		if (inp.st[0] == '=')
			token_add_char(*inp.st++);
		lsym = ps.next_unary ? lsym_unary_op : lsym_binary_op;
		next_unary = true;
		break;

	case '*':
		if (is_asterisk_unary()) {
			lex_asterisk_unary();
			lsym = lsym_unary_op;
			next_unary = true;
		} else {
			if (inp.st[0] == '=')
				token_add_char(*inp.st++);
			lsym = lsym_binary_op;
			next_unary = true;
		}
		break;

	default:
		if (token.mem[token.len - 1] == '/'
		    && (inp.st[0] == '*' || inp.st[0] == '/')) {
			enum indent_enabled prev = indent_enabled;
			lex_indent_comment();
			if (prev == indent_on && indent_enabled == indent_off)
				out.indent_off_text.len = 0;
			token_add_char(*inp.st++);
			lsym = lsym_comment;
			next_unary = ps.next_unary;
			break;
		}

		/* things like '||', '&&', '<<=', 'int *****i' */
		while (inp.st[0] == token.mem[token.len - 1]
		    || inp.st[0] == '=')
			token_add_char(*inp.st++);

		lsym = ps.next_unary ? lsym_unary_op : lsym_binary_op;
		next_unary = true;
	}

	if (ps.in_enum == in_enum_enum || ps.in_enum == in_enum_type)
		ps.in_enum = lsym == lsym_lbrace ? in_enum_brace : in_enum_no;
	if (lsym == lsym_rbrace)
		ps.in_enum = in_enum_no;

	ps.next_unary = next_unary;

	debug_parser_state(lsym);
	return lsym;
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

	pos = -(pos + 1);
	memmove(typenames.items + pos + 1, typenames.items + pos,
	    sizeof(typenames.items[0]) * (typenames.len++ - (unsigned)pos));
	typenames.items[pos] = nonnull(strdup(name));
}
