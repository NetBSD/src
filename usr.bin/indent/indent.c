/*	$NetBSD: indent.c,v 1.264 2023/05/13 17:54:34 rillig Exp $	*/

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

#if 0
static char sccsid[] = "@(#)indent.c	5.17 (Berkeley) 6/7/93";
#endif

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__RCSID("$NetBSD: indent.c,v 1.264 2023/05/13 17:54:34 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.c 340138 2018-11-04 19:24:49Z oshogbo $");
#endif

#include <sys/param.h>
#include <err.h>
#include <errno.h>
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
bool inhibit_formatting;

static int ifdef_level;
static struct parser_state state_stack[5];

FILE *input;
FILE *output;

static const char *in_name = "Standard Input";
static const char *out_name = "Standard Output";
static const char *backup_suffix = ".BAK";
static char bakfile[MAXPATHLEN] = "";


static void
buf_init(struct buffer *buf)
{
    size_t size = 200;
    buf->mem = xmalloc(size);
    buf->limit = buf->mem + size - 5 /* safety margin */;
    buf->s = buf->mem + 1;	/* allow accessing buf->e[-1] */
    buf->e = buf->s;
    buf->mem[0] = ' ';
    buf->e[0] = '\0';
}

static size_t
buf_len(const struct buffer *buf)
{
    return (size_t)(buf->e - buf->s);
}

void
buf_expand(struct buffer *buf, size_t add_size)
{
    size_t new_size = (size_t)(buf->limit - buf->s) + 400 + add_size;
    size_t len = buf_len(buf);
    buf->mem = xrealloc(buf->mem, new_size);
    buf->limit = buf->mem + new_size - 5;
    buf->s = buf->mem + 1;
    buf->e = buf->s + len;
    /* At this point, the buffer may not be null-terminated anymore. */
}

static void
buf_reserve(struct buffer *buf, size_t n)
{
    if (n >= (size_t)(buf->limit - buf->e))
	buf_expand(buf, n);
}

void
buf_add_char(struct buffer *buf, char ch)
{
    buf_reserve(buf, 1);
    *buf->e++ = ch;
}

void
buf_add_range(struct buffer *buf, const char *s, const char *e)
{
    size_t len = (size_t)(e - s);
    buf_reserve(buf, len);
    memcpy(buf->e, s, len);
    buf->e += len;
}

static void
buf_add_buf(struct buffer *buf, const struct buffer *add)
{
    buf_add_range(buf, add->s, add->e);
}

static void
buf_terminate(struct buffer *buf)
{
    buf_reserve(buf, 1);
    *buf->e = '\0';
}

static void
buf_reset(struct buffer *buf)
{
    buf->e = buf->s;
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
 * Compute the indentation from starting at 'ind' and adding the text from
 * 'start' to 'end'.
 */
int
ind_add(int ind, const char *start, const char *end)
{
    for (const char *p = start; p != end; ++p) {
	if (*p == '\n' || *p == '\f')
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
main_init_globals(void)
{
    inp_init();

    buf_init(&token);

    buf_init(&lab);
    buf_init(&code);
    buf_init(&com);

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
main_load_profiles(int argc, char **argv)
{
    const char *profile_name = NULL;

    for (int i = 1; i < argc; ++i) {
	const char *arg = argv[i];

	if (strcmp(arg, "-npro") == 0)
	    return;
	if (arg[0] == '-' && arg[1] == 'P' && arg[2] != '\0')
	    profile_name = arg + 2;
    }
    load_profiles(profile_name);
}

static void
main_parse_command_line(int argc, char **argv)
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
		errx(1, "input and output files must be different");
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
	opt.comment_column = 2;	/* don't put normal comments before column 2 */
    if (opt.block_comment_max_line_length <= 0)
	opt.block_comment_max_line_length = opt.max_line_length;
    if (opt.local_decl_indent < 0)	/* if not specified by user, set this */
	opt.local_decl_indent = opt.decl_indent;
    if (opt.decl_comment_column <= 0)	/* if not specified by user, set this */
	opt.decl_comment_column = opt.ljust_decl
	    ? (opt.comment_column <= 10 ? 2 : opt.comment_column - 8)
	    : opt.comment_column;
    if (opt.continuation_indent == 0)
	opt.continuation_indent = opt.indent_size;
}

static void
main_prepare_parsing(void)
{
    inp_read_line();

    int ind = 0;
    for (const char *p = inp_p();; p++) {
	if (*p == ' ')
	    ind++;
	else if (*p == '\t')
	    ind = next_tab(ind);
	else
	    break;
    }

    if (ind >= opt.indent_size)
	ps.ind_level = ps.ind_level_follow = ind / opt.indent_size;
}

static void
code_add_decl_indent(int decl_ind, bool tabs_to_var)
{
    int base_ind = ps.ind_level * opt.indent_size;
    int ind = base_ind + (int)buf_len(&code);
    int target_ind = base_ind + decl_ind;
    char *orig_code_e = code.e;

    if (tabs_to_var)
	for (int next; (next = next_tab(ind)) <= target_ind; ind = next)
	    buf_add_char(&code, '\t');

    for (; ind < target_ind; ind++)
	buf_add_char(&code, ' ');

    if (code.e == orig_code_e && ps.want_blank) {
	buf_add_char(&code, ' ');
	ps.want_blank = false;
    }
}

static int
process_eof(void)
{
    if (lab.s != lab.e || code.s != code.e || com.s != com.e)
	output_line();

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
move_com_to_code(void)
{
    if (lab.e != lab.s || code.e != code.s)
	buf_add_char(&code, ' ');
    buf_add_buf(&code, &com);
    buf_add_char(&code, ' ');
    buf_terminate(&code);
    buf_reset(&com);
    ps.want_blank = false;
}

static void
process_form_feed(void)
{
    output_line_ff();
    ps.want_blank = false;
}

static void
process_newline(void)
{
    if (ps.prev_token == lsym_comma && ps.nparen == 0 && !ps.block_init &&
	!opt.break_after_comma && break_comma &&
	com.s == com.e)
	goto stay_in_line;

    output_line();

stay_in_line:
    ++line_no;
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

static void
process_lparen_or_lbracket(void)
{
    if (++ps.nparen == array_length(ps.paren)) {
	diag(0, "Reached internal limit of %zu unclosed parentheses",
	    array_length(ps.paren));
	ps.nparen--;
    }

    if (token.s[0] == '(' && ps.in_decl
	&& !ps.block_init && !ps.decl_indent_done &&
	!ps.is_function_definition && ps.line_start_nparen == 0) {
	/* function pointer declarations */
	code_add_decl_indent(ps.decl_ind, ps.tabs_to_var);
	ps.decl_indent_done = true;
    } else if (want_blank_before_lparen())
	*code.e++ = ' ';
    ps.want_blank = false;
    *code.e++ = token.s[0];

    ps.paren[ps.nparen - 1].indent = (short)ind_add(0, code.s, code.e);
    debug_println("paren_indents[%d] is now %d",
	ps.nparen - 1, ps.paren[ps.nparen - 1].indent);

    if (opt.extra_expr_indent && ps.spaced_expr_psym != psym_0
	    && ps.nparen == 1 && ps.paren[0].indent < 2 * opt.indent_size) {
	ps.paren[0].indent = (short)(2 * opt.indent_size);
	debug_println("paren_indents[0] is now %d", ps.paren[0].indent);
    }

    if (ps.init_or_struct && *token.s == '(' && ps.tos <= 2) {
	/*
	 * this is a kluge to make sure that declarations will be aligned
	 * right if proc decl has an explicit type on it, i.e. "int a(x) {..."
	 */
	parse(psym_0);
	ps.init_or_struct = false;
    }

    /* parenthesized type following sizeof or offsetof is not a cast */
    if (ps.prev_token == lsym_offsetof || ps.prev_token == lsym_sizeof)
	ps.paren[ps.nparen - 1].no_cast = true;
}

static void
process_rparen_or_rbracket(void)
{
    if (ps.nparen == 0) {
	diag(0, "Extra '%c'", *token.s);
	goto unbalanced;	/* TODO: better exit immediately */
    }

    if (ps.paren[ps.nparen - 1].maybe_cast &&
	!ps.paren[ps.nparen - 1].no_cast) {
	ps.next_unary = true;
	ps.paren[ps.nparen - 1].maybe_cast = false;
	ps.want_blank = opt.space_after_cast;
    } else
	ps.want_blank = true;
    ps.paren[ps.nparen - 1].no_cast = false;

    if (ps.nparen > 0)
	ps.nparen--;

    if (code.e == code.s)	/* if the paren starts the line */
	ps.line_start_nparen = ps.nparen;	/* then indent it */

unbalanced:
    *code.e++ = token.s[0];

    if (ps.spaced_expr_psym != psym_0 && ps.nparen == 0) {
	ps.force_nl = true;
	ps.next_unary = true;
	ps.in_stmt_or_decl = false;
	parse(ps.spaced_expr_psym);
	ps.spaced_expr_psym = psym_0;
    }
}

static bool
want_blank_before_unary_op(void)
{
    if (ps.want_blank)
	return true;
    if (token.s[0] == '+' || token.s[0] == '-')
	return code.e > code.s && code.e[-1] == token.s[0];
    return false;
}

static void
process_unary_op(void)
{
    if (!ps.decl_indent_done && ps.in_decl && !ps.block_init &&
	!ps.is_function_definition && ps.line_start_nparen == 0) {
	/* pointer declarations */
	code_add_decl_indent(ps.decl_ind - (int)buf_len(&token),
			     ps.tabs_to_var);
	ps.decl_indent_done = true;
    } else if (want_blank_before_unary_op())
	*code.e++ = ' ';

    buf_add_buf(&code, &token);
    ps.want_blank = false;
}

static void
process_binary_op(void)
{
    if (buf_len(&code) > 0)
	buf_add_char(&code, ' ');
    buf_add_buf(&code, &token);
    ps.want_blank = true;
}

static void
process_postfix_op(void)
{
    *code.e++ = token.s[0];
    *code.e++ = token.s[1];
    ps.want_blank = true;
}

static void
process_question(void)
{
    ps.quest_level++;
    if (ps.want_blank)
	*code.e++ = ' ';
    *code.e++ = '?';
    ps.want_blank = true;
}

static void
process_colon(void)
{
    if (ps.quest_level > 0) {	/* part of a '?:' operator */
	ps.quest_level--;
	if (ps.want_blank)
	    *code.e++ = ' ';
	*code.e++ = ':';
	ps.want_blank = true;
	return;
    }

    if (ps.init_or_struct) {	/* bit-field */
	*code.e++ = ':';
	ps.want_blank = false;
	return;
    }

    buf_add_buf(&lab, &code);	/* 'case' or 'default' or named label */
    buf_add_char(&lab, ':');
    buf_terminate(&lab);
    buf_reset(&code);

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
    ps.seen_case = false;	/* these will only need resetting in an error */
    ps.quest_level = 0;
    if (ps.prev_token == lsym_rparen_or_rbracket)
	ps.in_func_def_params = false;
    ps.block_init = false;
    ps.block_init_level = 0;
    ps.declaration = ps.declaration == decl_begin ? decl_end : decl_no;

    if (ps.in_decl && code.s == code.e && !ps.block_init &&
	!ps.decl_indent_done && ps.line_start_nparen == 0) {
	/* indent stray semicolons in declarations */
	code_add_decl_indent(ps.decl_ind - 1, ps.tabs_to_var);
	ps.decl_indent_done = true;
    }

    ps.in_decl = ps.decl_level > 0;	/* if we were in a first level
					 * structure declaration before, we
					 * aren't anymore */

    if (ps.nparen > 0 && ps.spaced_expr_psym != psym_for_exprs) {
	/*
	 * There were unbalanced parentheses in the statement. It is a bit
	 * complicated, because the semicolon might be in a for statement.
	 */
	diag(1, "Unbalanced parentheses");
	ps.nparen = 0;
	if (ps.spaced_expr_psym != psym_0) {
	    parse(ps.spaced_expr_psym);
	    ps.spaced_expr_psym = psym_0;
	}
    }
    *code.e++ = ';';
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

    if (code.s != code.e && !ps.block_init) {
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

    if (code.s == code.e)
	ps.in_stmt_cont = false;	/* don't indent the '{' itself */
    if (ps.in_decl && ps.init_or_struct) {
	ps.di_stack[ps.decl_level] = ps.decl_ind;
	if (++ps.decl_level == (int)array_length(ps.di_stack)) {
	    diag(0, "Reached internal limit of %d struct levels",
		 (int)array_length(ps.di_stack));
	    ps.decl_level--;
	}
    } else {
	ps.decl_on_line = false;	/* we can't be in the middle of a
					 * declaration, so don't do special
					 * indentation of comments */
	ps.in_func_def_params = false;
	ps.in_decl = false;
    }

    ps.decl_ind = 0;
    parse(psym_lbrace);
    if (ps.want_blank)
	*code.e++ = ' ';
    ps.want_blank = false;
    *code.e++ = '{';
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

    if (code.s != code.e && !ps.block_init) {	/* '}' must be first on line */
	if (opt.verbose)
	    diag(0, "Line broken");
	output_line();
    }

    *code.e++ = '}';
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

    parse(psym_rbrace);
}

static void
process_do(void)
{
    ps.in_stmt_or_decl = false;

    if (code.e != code.s) {	/* make sure this starts a line */
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

    if (code.e > code.s && !(opt.cuddle_else && code.e[-1] == '}')) {
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
    parse(psym_decl);		/* let the parser worry about indentation */

    if (ps.prev_token == lsym_rparen_or_rbracket && ps.tos <= 1) {
	if (code.s != code.e)
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

    int len = (int)buf_len(&token) + 1;
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
	    if (opt.procnames_start_line && code.s != code.e) {
		*code.e = '\0';
		output_line();
	    } else if (ps.want_blank) {
		*code.e++ = ' ';
	    }
	    ps.want_blank = false;

	} else if (!ps.block_init && !ps.decl_indent_done &&
		ps.line_start_nparen == 0) {
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
copy_token(void)
{
    if (ps.want_blank)
	buf_add_char(&code, ' ');
    buf_add_buf(&code, &token);
}

static void
process_period(void)
{
    if (code.e > code.s && code.e[-1] == ',')
	*code.e++ = ' ';
    *code.e++ = '.';
    ps.want_blank = false;
}

static void
process_comma(void)
{
    ps.want_blank = code.s != code.e;	/* only put blank after comma if comma
					 * does not start the line */

    if (ps.in_decl && !ps.is_function_definition && !ps.block_init &&
	    !ps.decl_indent_done && ps.line_start_nparen == 0) {
	/* indent leading commas and not the actual identifiers */
	code_add_decl_indent(ps.decl_ind - 1, ps.tabs_to_var);
	ps.decl_indent_done = true;
    }

    *code.e++ = ',';

    if (ps.nparen == 0) {
	if (ps.block_init_level <= 0)
	    ps.block_init = false;
	int varname_len = 8;	/* rough estimate for the length of a typical
				 * variable name */
	if (break_comma && (opt.break_after_comma ||
		ind_add(compute_code_indent(), code.s, code.e)
		>= opt.max_line_length - varname_len))
	    ps.force_nl = true;
    }
}

/* move the whole line to the 'label' buffer */
static void
read_preprocessing_line(void)
{
    enum {
	PLAIN, STR, CHR, COMM
    } state;

    buf_add_char(&lab, '#');

    state = PLAIN;

    while (ch_isblank(inp_peek()))
	buf_add_char(&lab, inp_next());

    while (inp_peek() != '\n' || (state == COMM && !had_eof)) {
	buf_reserve(&lab, 2);
	*lab.e++ = inp_next();
	switch (lab.e[-1]) {
	case '\\':
	    if (state != COMM)
		*lab.e++ = inp_next();
	    break;
	case '/':
	    if (inp_peek() == '*' && state == PLAIN) {
		state = COMM;
		*lab.e++ = inp_next();
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
	    if (inp_peek() == '/' && state == COMM) {
		state = PLAIN;
		*lab.e++ = inp_next();
	    }
	    break;
	}
    }

    while (lab.e > lab.s && ch_isblank(lab.e[-1]))
	lab.e--;
    buf_terminate(&lab);
}

typedef struct {
    const char *s;
    const char *e;
} substring;

static bool
substring_equals(substring ss, const char *str)
{
    size_t len = (size_t)(ss.e - ss.s);
    return len == strlen(str) && memcmp(ss.s, str, len) == 0;
}

static bool
substring_starts_with(substring ss, const char *prefix)
{
    while (ss.s < ss.e && *prefix != '\0' && *ss.s == *prefix)
	ss.s++, prefix++;
    return *prefix == '\0';
}

static void
process_preprocessing(void)
{
    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	output_line();

    read_preprocessing_line();

    ps.is_case_label = false;

    substring dir;
    dir.s = lab.s + 1;
    while (dir.s < lab.e && ch_isblank(*dir.s))
	dir.s++;
    dir.e = dir.s;
    while (dir.e < lab.e && ch_isalpha(*dir.e))
	dir.e++;

    if (substring_starts_with(dir, "if")) {	/* also ifdef, ifndef */
	if ((size_t)ifdef_level < array_length(state_stack))
	    state_stack[ifdef_level++] = ps;
	else
	    diag(1, "#if stack overflow");

    } else if (substring_starts_with(dir, "el")) {	/* else, elif */
	if (ifdef_level <= 0)
	    diag(1, dir.s[2] == 'i' ? "Unmatched #elif" : "Unmatched #else");
	else
	    ps = state_stack[ifdef_level - 1];

    } else if (substring_equals(dir, "endif")) {
	if (ifdef_level <= 0)
	    diag(1, "Unmatched #endif");
	else
	    ifdef_level--;

    } else {
	if (!substring_equals(dir, "pragma") &&
	    !substring_equals(dir, "error") &&
	    !substring_equals(dir, "line") &&
	    !substring_equals(dir, "undef") &&
	    !substring_equals(dir, "define") &&
	    !substring_equals(dir, "include")) {
	    diag(1, "Unrecognized cpp directive \"%.*s\"",
		 (int)(dir.e - dir.s), dir.s);
	    return;
	}
    }

    /*
     * subsequent processing of the newline character will cause the line to
     * be printed
     */
}

static int
main_loop(void)
{

    ps.di_stack[ps.decl_level = 0] = 0;

    for (;;) {			/* loop until we reach eof */
	lexer_symbol lsym = lexi();

	if (lsym == lsym_if && ps.prev_token == lsym_else && opt.else_if)
	    ps.force_nl = false;

	if (lsym == lsym_eof)
	    return process_eof();

	if (lsym == lsym_newline || lsym == lsym_form_feed ||
		lsym == lsym_preprocessing)
	    ps.force_nl = false;
	else if (lsym != lsym_comment) {
	    maybe_break_line(lsym);
	    ps.in_stmt_or_decl = true;	/* add an extra level of indentation;
					 * turned off again by a ';' or '}' */
	    if (com.s != com.e)
		move_com_to_code();
	}

	buf_reserve(&code, 3);	/* space for 2 characters plus '\0' */

	switch (lsym) {

	case lsym_form_feed:
	    process_form_feed();
	    break;

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
	    copy_token();
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

	*code.e = '\0';
	if (lsym != lsym_comment && lsym != lsym_newline &&
		lsym != lsym_preprocessing)
	    ps.prev_token = lsym;
    }
}

int
main(int argc, char **argv)
{
    main_init_globals();
    main_load_profiles(argc, argv);
    main_parse_command_line(argc, argv);
    main_prepare_parsing();
    return main_loop();
}

#ifdef debug
void
debug_printf(const char *fmt, ...)
{
    FILE *f = output == stdout ? stderr : stdout;
    va_list ap;

    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
}

void
debug_println(const char *fmt, ...)
{
    FILE *f = output == stdout ? stderr : stdout;
    va_list ap;

    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
}

void
debug_vis_range(const char *prefix, const char *s, const char *e,
    const char *suffix)
{
    debug_printf("%s", prefix);
    for (const char *p = s; p < e; p++) {
	if (*p == '\\' || *p == '"')
	    debug_printf("\\%c", *p);
	else if (isprint((unsigned char)*p))
	    debug_printf("%c", *p);
	else if (*p == '\n')
	    debug_printf("\\n");
	else if (*p == '\t')
	    debug_printf("\\t");
	else
	    debug_printf("\\x%02x", (unsigned char)*p);
    }
    debug_printf("%s", suffix);
}
#endif

static void *
nonnull(void *p)
{
    if (p == NULL)
	err(EXIT_FAILURE, NULL);
    return p;
}

void *
xmalloc(size_t size)
{
    return nonnull(malloc(size));
}

void *
xrealloc(void *p, size_t new_size)
{
    return nonnull(realloc(p, new_size));
}

char *
xstrdup(const char *s)
{
    return nonnull(strdup(s));
}
