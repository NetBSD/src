/*	$NetBSD: indent.c,v 1.302 2023/05/21 10:05:20 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 * Copyright (c) 1976 Board of Trustees of the University of Illinois.
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__RCSID("$NetBSD: indent.c,v 1.302 2023/05/21 10:05:20 rillig Exp $");

#include <sys/param.h>
#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "indent.h"

struct options opt = {
	.brace_same_line = true,
	.comment_delimiter_on_blankline = true,
	.cuddle_else = true,
	.comment_column = 33,
	.decl_indent = 16,
	.else_if = true,
	.function_brace_split = true,
	.format_col1_comments = true,
	.format_block_comments = true,
	.indent_parameters = true,
	.indent_size = 8,
	.local_decl_indent = -1,
	.lineup_to_parens = true,
	.procnames_start_line = true,
	.star_comment_cont = true,
	.tabsize = 8,
	.max_line_length = 78,
	.use_tabs = true,
};

struct parser_state ps;

struct buffer token;

struct buffer lab;
struct buffer code;
struct buffer com;

bool found_err;
bool break_comma;
float case_ind;
bool had_eof;
int line_no = 1;
enum indent_enabled indent_enabled;

static int ifdef_level;
static struct parser_state state_stack[5];

FILE *input;
FILE *output;

static const char *in_name = "Standard Input";
static const char *out_name = "Standard Output";
static const char *backup_suffix = ".BAK";
static char bakfile[MAXPATHLEN] = "";


void *
nonnull(void *p)
{
	if (p == NULL)
		err(EXIT_FAILURE, NULL);
	return p;
}

static void
buf_expand(struct buffer *buf, size_t add_size)
{
	buf->cap = buf->cap + add_size + 400;
	buf->mem = nonnull(realloc(buf->mem, buf->cap));
	buf->st = buf->mem;
}

void
buf_add_char(struct buffer *buf, char ch)
{
	if (buf->len == buf->cap)
		buf_expand(buf, 1);
	buf->mem[buf->len++] = ch;
}

void
buf_add_chars(struct buffer *buf, const char *s, size_t len)
{
	if (len == 0)
		return;
	if (len > buf->cap - buf->len)
		buf_expand(buf, len);
	memcpy(buf->mem + buf->len, s, len);
	buf->len += len;
}

static void
buf_add_buf(struct buffer *buf, const struct buffer *add)
{
	buf_add_chars(buf, add->st, add->len);
}

void
diag(int level, const char *msg, ...)
{
	va_list ap;

	if (level != 0)
		found_err = true;

	va_start(ap, msg);
	fprintf(stderr, "%s: %s:%d: ",
	    level == 0 ? "warning" : "error", in_name, line_no);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

/*
 * Compute the indentation from starting at 'ind' and adding the text starting
 * at 's'.
 */
int
ind_add(int ind, const char *s, size_t len)
{
	for (const char *p = s; len > 0; p++, len--) {
		if (*p == '\n')
			ind = 0;
		else if (*p == '\t')
			ind = next_tab(ind);
		else if (*p == '\b')
			--ind;
		else
			++ind;
	}
	return ind;
}

static void
init_globals(void)
{
	ps.s_sym[0] = psym_stmt_list;
	ps.prev_token = lsym_semicolon;
	ps.next_col_1 = true;

	const char *suffix = getenv("SIMPLE_BACKUP_SUFFIX");
	if (suffix != NULL)
		backup_suffix = suffix;
}

/*
 * Copy the input file to the backup file, then make the backup file the input
 * and the original input file the output.
 */
static void
bakcopy(void)
{
	ssize_t n;
	int bak_fd;
	char buff[8 * 1024];

	const char *last_slash = strrchr(in_name, '/');
	snprintf(bakfile, sizeof(bakfile), "%s%s",
	    last_slash != NULL ? last_slash + 1 : in_name, backup_suffix);

	/* copy in_name to backup file */
	bak_fd = creat(bakfile, 0600);
	if (bak_fd < 0)
		err(1, "%s", bakfile);

	while ((n = read(fileno(input), buff, sizeof(buff))) > 0)
		if (write(bak_fd, buff, (size_t)n) != n)
			err(1, "%s", bakfile);
	if (n < 0)
		err(1, "%s", in_name);

	close(bak_fd);
	(void)fclose(input);

	/* re-open backup file as the input file */
	input = fopen(bakfile, "r");
	if (input == NULL)
		err(1, "%s", bakfile);
	/* now the original input file will be the output */
	output = fopen(in_name, "w");
	if (output == NULL) {
		unlink(bakfile);
		err(1, "%s", in_name);
	}
}

static void
load_profiles(int argc, char **argv)
{
	const char *profile_name = NULL;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (strcmp(arg, "-npro") == 0)
			return;
		if (arg[0] == '-' && arg[1] == 'P' && arg[2] != '\0')
			profile_name = arg + 2;
	}

	load_profile_files(profile_name);
}

static void
parse_command_line(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (arg[0] == '-') {
			set_option(arg, "Command line");

		} else if (input == NULL) {
			in_name = arg;
			if ((input = fopen(in_name, "r")) == NULL)
				err(1, "%s", in_name);

		} else if (output == NULL) {
			out_name = arg;
			if (strcmp(in_name, out_name) == 0)
				errx(1, "input and output files "
				    "must be different");
			if ((output = fopen(out_name, "w")) == NULL)
				err(1, "%s", out_name);

		} else
			errx(1, "too many arguments: %s", arg);
	}

	if (input == NULL) {
		input = stdin;
		output = stdout;
	} else if (output == NULL) {
		out_name = in_name;
		bakcopy();
	}

	if (opt.comment_column <= 1)
		opt.comment_column = 2;	/* don't put normal comments in column
					 * 1, see opt.format_col1_comments */
	if (opt.block_comment_max_line_length <= 0)
		opt.block_comment_max_line_length = opt.max_line_length;
	if (opt.local_decl_indent < 0)
		opt.local_decl_indent = opt.decl_indent;
	if (opt.decl_comment_column <= 0)
		opt.decl_comment_column = opt.ljust_decl
		    ? (opt.comment_column <= 10 ? 2 : opt.comment_column - 8)
		    : opt.comment_column;
	if (opt.continuation_indent == 0)
		opt.continuation_indent = opt.indent_size;
}

static void
set_initial_indentation(void)
{
	inp_read_line();

	int ind = 0;
	for (const char *p = inp.st;; p++) {
		if (*p == ' ')
			ind++;
		else if (*p == '\t')
			ind = next_tab(ind);
		else
			break;
	}

	ps.ind_level = ps.ind_level_follow = ind / opt.indent_size;
}

static void
code_add_decl_indent(int decl_ind, bool tabs_to_var)
{
	int base = ps.ind_level * opt.indent_size;
	int ind = base + (int)code.len;
	int target = base + decl_ind;
	size_t orig_code_len = code.len;

	if (tabs_to_var)
		for (int next; (next = next_tab(ind)) <= target; ind = next)
			buf_add_char(&code, '\t');

	for (; ind < target; ind++)
		buf_add_char(&code, ' ');

	if (code.len == orig_code_len && ps.want_blank) {
		buf_add_char(&code, ' ');
		ps.want_blank = false;
	}
}

static int
process_eof(void)
{
	if (lab.len > 0 || code.len > 0 || com.len > 0)
		output_line();
	if (indent_enabled != indent_on) {
		indent_enabled = indent_last_off_line;
		output_line();
	}

	if (ps.tos > 1)		/* check for balanced braces */
		diag(1, "Stuff missing from end of file");

	fflush(output);
	return found_err ? EXIT_FAILURE : EXIT_SUCCESS;
}

static void
maybe_break_line(lexer_symbol lsym)
{
	if (!ps.force_nl)
		return;
	if (lsym == lsym_semicolon)
		return;
	if (lsym == lsym_lbrace && opt.brace_same_line)
		return;

	if (opt.verbose)
		diag(0, "Line broken");
	output_line();
	ps.force_nl = false;
}

static void
move_com_to_code(lexer_symbol lsym)
{
	if (ps.want_blank)
		buf_add_char(&code, ' ');
	buf_add_buf(&code, &com);
	if (lsym != lsym_rparen_or_rbracket)
		buf_add_char(&code, ' ');
	com.len = 0;
	ps.want_blank = false;
}

static void
process_newline(void)
{
	if (ps.prev_token == lsym_comma && ps.nparen == 0 && !ps.block_init &&
	    !opt.break_after_comma && break_comma &&
	    com.len == 0)
		goto stay_in_line;

	output_line();

stay_in_line:
	++line_no;
}

static bool
is_function_pointer_declaration(void)
{
	return token.st[0] == '('
	    && ps.in_decl
	    && !ps.block_init
	    && !ps.decl_indent_done
	    && !ps.is_function_definition
	    && ps.line_start_nparen == 0;
}

static bool
want_blank_before_lparen(void)
{
	if (!ps.want_blank)
		return false;
	if (opt.proc_calls_space)
		return true;
	if (ps.prev_token == lsym_rparen_or_rbracket)
		return false;
	if (ps.prev_token == lsym_offsetof)
		return false;
	if (ps.prev_token == lsym_sizeof)
		return opt.blank_after_sizeof;
	if (ps.prev_token == lsym_word || ps.prev_token == lsym_funcname)
		return false;
	return true;
}

static bool
want_blank_before_lbracket(void)
{
	if (code.len == 0)
		return false;
	if (ps.prev_token == lsym_comma)
		return true;
	if (ps.prev_token == lsym_binary_op)
		return true;
	return false;
}

static void
process_lparen_or_lbracket(void)
{
	if (++ps.nparen == array_length(ps.paren)) {
		diag(0, "Reached internal limit of %zu unclosed parentheses",
		    array_length(ps.paren));
		ps.nparen--;
	}

	if (is_function_pointer_declaration()) {
		code_add_decl_indent(ps.decl_ind, ps.tabs_to_var);
		ps.decl_indent_done = true;
	} else if (token.st[0] == '('
	    ? want_blank_before_lparen() : want_blank_before_lbracket())
		buf_add_char(&code, ' ');
	ps.want_blank = false;
	buf_add_char(&code, token.st[0]);

	int indent = ind_add(0, code.st, code.len);
	enum paren_level_cast cast = cast_unknown;

	if (opt.extra_expr_indent && !opt.lineup_to_parens
	    && ps.spaced_expr_psym != psym_0 && ps.nparen == 1
	    && opt.continuation_indent == opt.indent_size)
		ps.extra_expr_indent = eei_yes;

	if (opt.extra_expr_indent && ps.spaced_expr_psym != psym_0
	    && ps.nparen == 1 && indent < 2 * opt.indent_size)
		indent = 2 * opt.indent_size;

	if (ps.init_or_struct && *token.st == '(' && ps.tos <= 2) {
		/* this is a kluge to make sure that declarations will be
		 * aligned right if proc decl has an explicit type on it, i.e.
		 * "int a(x) {..." */
		parse(psym_0);
		ps.init_or_struct = false;
	}

	if (ps.prev_token == lsym_offsetof || ps.prev_token == lsym_sizeof
	    || ps.is_function_definition)
		cast = cast_no;

	ps.paren[ps.nparen - 1].indent = indent;
	ps.paren[ps.nparen - 1].cast = cast;
	debug_println("paren_indents[%d] is now %s%d",
	    ps.nparen - 1, paren_level_cast_name[cast], indent);
}

static void
process_rparen_or_rbracket(void)
{
	if (ps.nparen == 0) {
		diag(0, "Extra '%c'", *token.st);
		goto unbalanced;
	}

	enum paren_level_cast cast = ps.paren[--ps.nparen].cast;
	if (ps.decl_on_line && !ps.block_init)
		cast = cast_no;

	if (cast == cast_maybe) {
		ps.next_unary = true;
		ps.want_blank = opt.space_after_cast;
	} else
		ps.want_blank = true;

	if (code.len == 0)	/* if the paren starts the line */
		ps.line_start_nparen = ps.nparen;	/* then indent it */

unbalanced:
	buf_add_char(&code, token.st[0]);

	if (ps.spaced_expr_psym != psym_0 && ps.nparen == 0) {
		if (ps.extra_expr_indent == eei_yes)
			ps.extra_expr_indent = eei_last;
		ps.force_nl = true;
		ps.next_unary = true;
		ps.in_stmt_or_decl = false;
		parse(ps.spaced_expr_psym);
		ps.spaced_expr_psym = psym_0;
		ps.want_blank = true;
	}
}

static bool
want_blank_before_unary_op(void)
{
	if (ps.want_blank)
		return true;
	if (token.st[0] == '+' || token.st[0] == '-')
		return code.len > 0 && code.mem[code.len - 1] == token.st[0];
	return false;
}

static void
process_unary_op(void)
{
	if (!ps.decl_indent_done && ps.in_decl && !ps.block_init &&
	    !ps.is_function_definition && ps.line_start_nparen == 0) {
		/* pointer declarations */
		code_add_decl_indent(ps.decl_ind - (int)token.len,
		    ps.tabs_to_var);
		ps.decl_indent_done = true;
	} else if (want_blank_before_unary_op())
		buf_add_char(&code, ' ');

	buf_add_buf(&code, &token);
	ps.want_blank = false;
}

static void
process_binary_op(void)
{
	if (code.len > 0 && ps.want_blank)
		buf_add_char(&code, ' ');
	buf_add_buf(&code, &token);
	ps.want_blank = true;
}

static void
process_postfix_op(void)
{
	buf_add_buf(&code, &token);
	ps.want_blank = true;
}

static void
process_question(void)
{
	ps.quest_level++;
	if (code.len == 0) {
		ps.in_stmt_cont = true;
		ps.in_stmt_or_decl = true;
		ps.in_decl = false;
	}
	if (ps.want_blank)
		buf_add_char(&code, ' ');
	buf_add_char(&code, '?');
	ps.want_blank = true;
}

static void
process_colon(void)
{
	if (ps.quest_level > 0) {	/* part of a '?:' operator */
		ps.quest_level--;
		if (code.len == 0) {
			ps.in_stmt_cont = true;
			ps.in_stmt_or_decl = true;
			ps.in_decl = false;
		}
		if (ps.want_blank)
			buf_add_char(&code, ' ');
		buf_add_char(&code, ':');
		ps.want_blank = true;
		return;
	}

	if (ps.init_or_struct) {	/* bit-field */
		buf_add_char(&code, ':');
		ps.want_blank = false;
		return;
	}

	buf_add_buf(&lab, &code);	/* 'case' or 'default' or named label
					 */
	buf_add_char(&lab, ':');
	code.len = 0;

	ps.in_stmt_or_decl = false;
	ps.is_case_label = ps.seen_case;
	ps.force_nl = ps.seen_case;
	ps.seen_case = false;
	ps.want_blank = false;
}

static void
process_semicolon(void)
{
	if (ps.decl_level == 0)
		ps.init_or_struct = false;
	ps.seen_case = false;	/* only needs to be reset on error */
	ps.quest_level = 0;	/* only needs to be reset on error */
	if (ps.prev_token == lsym_rparen_or_rbracket)
		ps.in_func_def_params = false;
	ps.block_init = false;
	ps.block_init_level = 0;
	ps.declaration = ps.declaration == decl_begin ? decl_end : decl_no;

	if (ps.in_decl && code.len == 0 && !ps.block_init &&
	    !ps.decl_indent_done && ps.line_start_nparen == 0) {
		/* indent stray semicolons in declarations */
		code_add_decl_indent(ps.decl_ind - 1, ps.tabs_to_var);
		ps.decl_indent_done = true;
	}

	ps.in_decl = ps.decl_level > 0;	/* if we were in a first level
					 * structure declaration before, we
					 * aren't anymore */

	if (ps.nparen > 0 && ps.spaced_expr_psym != psym_for_exprs) {
		/* There were unbalanced parentheses in the statement. It is a
		 * bit complicated, because the semicolon might be in a for
		 * statement. */
		diag(1, "Unbalanced parentheses");
		ps.nparen = 0;
		if (ps.spaced_expr_psym != psym_0) {
			parse(ps.spaced_expr_psym);
			ps.spaced_expr_psym = psym_0;
		}
	}
	buf_add_char(&code, ';');
	ps.want_blank = true;
	ps.in_stmt_or_decl = ps.nparen > 0;

	if (ps.spaced_expr_psym == psym_0) {
		parse(psym_0);	/* let parser know about end of stmt */
		ps.force_nl = true;
	}
}

static void
process_lbrace(void)
{
	ps.in_stmt_or_decl = false;	/* don't indent the {} */

	if (!ps.block_init)
		ps.force_nl = true;
	else if (ps.block_init_level <= 0)
		ps.block_init_level = 1;
	else
		ps.block_init_level++;

	if (code.len > 0 && !ps.block_init) {
		if (!opt.brace_same_line)
			output_line();
		else if (ps.in_func_def_params && !ps.init_or_struct) {
			ps.ind_level_follow = 0;
			if (opt.function_brace_split)
				output_line();
			else
				ps.want_blank = true;
		}
	}

	if (ps.nparen > 0) {
		diag(1, "Unbalanced parentheses");
		ps.nparen = 0;
		if (ps.spaced_expr_psym != psym_0) {
			parse(ps.spaced_expr_psym);
			ps.spaced_expr_psym = psym_0;
			ps.ind_level = ps.ind_level_follow;
		}
	}

	if (code.len == 0)
		ps.in_stmt_cont = false;	/* don't indent the '{' itself
						 */
	if (ps.in_decl && ps.init_or_struct) {
		ps.di_stack[ps.decl_level] = ps.decl_ind;
		if (++ps.decl_level == (int)array_length(ps.di_stack)) {
			diag(0, "Reached internal limit of %d struct levels",
			    (int)array_length(ps.di_stack));
			ps.decl_level--;
		}
	} else {
		ps.decl_on_line = false;	/* we can't be in the middle of
						 * a declaration, so don't do
						 * special indentation of
						 * comments */
		ps.in_func_def_params = false;
		ps.in_decl = false;
	}

	ps.decl_ind = 0;
	parse(psym_lbrace);
	if (ps.want_blank)
		buf_add_char(&code, ' ');
	ps.want_blank = false;
	buf_add_char(&code, '{');
	ps.declaration = decl_no;
}

static void
process_rbrace(void)
{
	if (ps.nparen > 0) {	/* check for unclosed if, for, else. */
		diag(1, "Unbalanced parentheses");
		ps.nparen = 0;
		ps.spaced_expr_psym = psym_0;
	}

	ps.declaration = decl_no;
	ps.block_init_level--;

	if (code.len > 0 && !ps.block_init) {
		if (opt.verbose)
			diag(0, "Line broken");
		output_line();
	}

	buf_add_char(&code, '}');
	ps.want_blank = true;
	ps.in_stmt_or_decl = false;
	ps.in_stmt_cont = false;

	if (ps.decl_level > 0) {	/* multi-level structure declaration */
		ps.decl_ind = ps.di_stack[--ps.decl_level];
		if (ps.decl_level == 0 && !ps.in_func_def_params) {
			ps.declaration = decl_begin;
			ps.decl_ind = ps.ind_level == 0
			    ? opt.decl_indent : opt.local_decl_indent;
		}
		ps.in_decl = true;
	}

	if (ps.tos == 2)
		out.line_kind = lk_func_end;

	parse(psym_rbrace);
}

static void
process_do(void)
{
	ps.in_stmt_or_decl = false;

	if (code.len > 0) {	/* make sure this starts a line */
		if (opt.verbose)
			diag(0, "Line broken");
		output_line();
	}

	ps.force_nl = true;
	parse(psym_do);
}

static void
process_else(void)
{
	ps.in_stmt_or_decl = false;

	if (code.len > 0
	    && !(opt.cuddle_else && code.mem[code.len - 1] == '}')) {
		if (opt.verbose)
			diag(0, "Line broken");
		output_line();
	}

	ps.force_nl = true;
	parse(psym_else);
}

static void
process_type(void)
{
	parse(psym_decl);	/* let the parser worry about indentation */

	if (ps.prev_token == lsym_rparen_or_rbracket && ps.tos <= 1) {
		if (code.len > 0)
			output_line();
	}

	if (ps.in_func_def_params && opt.indent_parameters &&
	    ps.decl_level == 0) {
		ps.ind_level = ps.ind_level_follow = 1;
		ps.in_stmt_cont = false;
	}

	ps.init_or_struct = /* maybe */ true;
	ps.in_decl = ps.decl_on_line = ps.prev_token != lsym_typedef;
	if (ps.decl_level <= 0)
		ps.declaration = decl_begin;

	int len = (int)token.len + 1;
	int ind = ps.ind_level == 0 || ps.decl_level > 0
	    ? opt.decl_indent	/* global variable or local member */
	    : opt.local_decl_indent;	/* local variable */
	ps.decl_ind = ind > 0 ? ind : len;
	ps.tabs_to_var = opt.use_tabs && ind > 0;
}

static void
process_ident(lexer_symbol lsym)
{
	if (ps.in_decl) {
		if (lsym == lsym_funcname) {
			ps.in_decl = false;
			if (opt.procnames_start_line && code.len > 0)
				output_line();
			else if (ps.want_blank)
				buf_add_char(&code, ' ');
			ps.want_blank = false;

		} else if (!ps.block_init && !ps.decl_indent_done &&
		    ps.line_start_nparen == 0) {
			if (opt.decl_indent == 0
			    && code.len > 0 && code.mem[code.len - 1] == '}')
				ps.decl_ind =
				    ind_add(0, code.st, code.len) + 1;
			code_add_decl_indent(ps.decl_ind, ps.tabs_to_var);
			ps.decl_indent_done = true;
			ps.want_blank = false;
		}

	} else if (ps.spaced_expr_psym != psym_0 && ps.nparen == 0) {
		ps.force_nl = true;
		ps.next_unary = true;
		ps.in_stmt_or_decl = false;
		parse(ps.spaced_expr_psym);
		ps.spaced_expr_psym = psym_0;
	}
}

static void
process_period(void)
{
	if (code.len > 0 && code.mem[code.len - 1] == ',')
		buf_add_char(&code, ' ');
	buf_add_char(&code, '.');
	ps.want_blank = false;
}

static void
process_comma(void)
{
	ps.want_blank = code.len > 0;	/* only put blank after comma if comma
					 * does not start the line */

	if (ps.in_decl && !ps.is_function_definition && !ps.block_init &&
	    !ps.decl_indent_done && ps.line_start_nparen == 0) {
		/* indent leading commas and not the actual identifiers */
		code_add_decl_indent(ps.decl_ind - 1, ps.tabs_to_var);
		ps.decl_indent_done = true;
	}

	buf_add_char(&code, ',');

	if (ps.nparen == 0) {
		if (ps.block_init_level <= 0)
			ps.block_init = false;
		int typical_varname_length = 8;
		if (break_comma && (opt.break_after_comma ||
		    ind_add(compute_code_indent(), code.st, code.len)
		    >= opt.max_line_length - typical_varname_length))
			ps.force_nl = true;
	}
}

/* move the whole line to the 'label' buffer */
static void
read_preprocessing_line(void)
{
	enum {
		PLAIN, STR, CHR, COMM
	} state = PLAIN;

	buf_add_char(&lab, '#');

	while (ch_isblank(inp.st[0]))
		buf_add_char(&lab, *inp.st++);

	while (inp.st[0] != '\n' || (state == COMM && !had_eof)) {
		buf_add_char(&lab, inp_next());
		switch (lab.mem[lab.len - 1]) {
		case '\\':
			if (state != COMM)
				buf_add_char(&lab, inp_next());
			break;
		case '/':
			if (inp.st[0] == '*' && state == PLAIN) {
				state = COMM;
				buf_add_char(&lab, *inp.st++);
			}
			break;
		case '"':
			if (state == STR)
				state = PLAIN;
			else if (state == PLAIN)
				state = STR;
			break;
		case '\'':
			if (state == CHR)
				state = PLAIN;
			else if (state == PLAIN)
				state = CHR;
			break;
		case '*':
			if (inp.st[0] == '/' && state == COMM) {
				state = PLAIN;
				buf_add_char(&lab, *inp.st++);
			}
			break;
		}
	}

	while (lab.len > 0 && ch_isblank(lab.mem[lab.len - 1]))
		lab.len--;
}

static void
process_preprocessing(void)
{
	if (lab.len > 0 || code.len > 0 || com.len > 0)
		output_line();

	read_preprocessing_line();

	ps.is_case_label = false;

	const char *end = lab.mem + lab.len;
	const char *dir = lab.st + 1;
	while (dir < end && ch_isblank(*dir))
		dir++;
	size_t dir_len = 0;
	while (dir + dir_len < end && ch_isalpha(dir[dir_len]))
		dir_len++;

	if (dir_len >= 2 && memcmp(dir, "if", 2) == 0) {
		if ((size_t)ifdef_level < array_length(state_stack))
			state_stack[ifdef_level++] = ps;
		else
			diag(1, "#if stack overflow");
		out.line_kind = lk_if;

	} else if (dir_len >= 2 && memcmp(dir, "el", 2) == 0) {
		if (ifdef_level <= 0)
			diag(1, dir[2] == 'i'
			    ? "Unmatched #elif" : "Unmatched #else");
		else
			ps = state_stack[ifdef_level - 1];

	} else if (dir_len == 5 && memcmp(dir, "endif", 5) == 0) {
		if (ifdef_level <= 0)
			diag(1, "Unmatched #endif");
		else
			ifdef_level--;
		out.line_kind = lk_endif;
	}

	/* subsequent processing of the newline character will cause the line
	 * to be printed */
}

static int
indent(void)
{
	for (;;) {		/* loop until we reach eof */
		lexer_symbol lsym = lexi();

		if (lsym == lsym_eof)
			return process_eof();

		if (lsym == lsym_if && ps.prev_token == lsym_else
		    && opt.else_if)
			ps.force_nl = false;

		if (lsym == lsym_newline || lsym == lsym_preprocessing)
			ps.force_nl = false;
		else if (lsym != lsym_comment) {
			maybe_break_line(lsym);
			/*
			 * Add an extra level of indentation; turned off again
			 * by a ';' or '}'.
			 */
			ps.in_stmt_or_decl = true;
			if (com.len > 0)
				move_com_to_code(lsym);
		}

		switch (lsym) {

		case lsym_newline:
			process_newline();
			break;

		case lsym_lparen_or_lbracket:
			process_lparen_or_lbracket();
			break;

		case lsym_rparen_or_rbracket:
			process_rparen_or_rbracket();
			break;

		case lsym_unary_op:
			process_unary_op();
			break;

		case lsym_binary_op:
			process_binary_op();
			break;

		case lsym_postfix_op:
			process_postfix_op();
			break;

		case lsym_question:
			process_question();
			break;

		case lsym_case_label:
			ps.seen_case = true;
			goto copy_token;

		case lsym_colon:
			process_colon();
			break;

		case lsym_semicolon:
			process_semicolon();
			break;

		case lsym_lbrace:
			process_lbrace();
			break;

		case lsym_rbrace:
			process_rbrace();
			break;

		case lsym_switch:
			ps.spaced_expr_psym = psym_switch_expr;
			goto copy_token;

		case lsym_for:
			ps.spaced_expr_psym = psym_for_exprs;
			goto copy_token;

		case lsym_if:
			ps.spaced_expr_psym = psym_if_expr;
			goto copy_token;

		case lsym_while:
			ps.spaced_expr_psym = psym_while_expr;
			goto copy_token;

		case lsym_do:
			process_do();
			goto copy_token;

		case lsym_else:
			process_else();
			goto copy_token;

		case lsym_typedef:
		case lsym_storage_class:
			goto copy_token;

		case lsym_tag:
			if (ps.nparen > 0)
				goto copy_token;
			/* FALLTHROUGH */
		case lsym_type_outside_parentheses:
			process_type();
			goto copy_token;

		case lsym_type_in_parentheses:
		case lsym_offsetof:
		case lsym_sizeof:
		case lsym_word:
		case lsym_funcname:
		case lsym_return:
			process_ident(lsym);
	copy_token:
			if (ps.want_blank)
				buf_add_char(&code, ' ');
			buf_add_buf(&code, &token);
			if (lsym != lsym_funcname)
				ps.want_blank = true;
			break;

		case lsym_period:
			process_period();
			break;

		case lsym_comma:
			process_comma();
			break;

		case lsym_preprocessing:
			process_preprocessing();
			break;

		case lsym_comment:
			process_comment();
			break;

		default:
			break;
		}

		if (lsym != lsym_comment && lsym != lsym_newline &&
		    lsym != lsym_preprocessing)
			ps.prev_token = lsym;
	}
}

int
main(int argc, char **argv)
{
	init_globals();
	load_profiles(argc, argv);
	parse_command_line(argc, argv);
	set_initial_indentation();
	return indent();
}
