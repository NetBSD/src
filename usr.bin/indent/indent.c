/*	$NetBSD: indent.c,v 1.241 2022/02/13 12:20:09 rillig Exp $	*/

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
__RCSID("$NetBSD: indent.c,v 1.241 2022/02/13 12:20:09 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.c 340138 2018-11-04 19:24:49Z oshogbo $");
#endif

#include <sys/param.h>
#if HAVE_CAPSICUM
#include <sys/capsicum.h>
#include <capsicum_helpers.h>
#endif
#include <assert.h>
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
int blank_lines_to_output;
bool blank_line_before;
bool blank_line_after;
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

#if HAVE_CAPSICUM
static void
init_capsicum(void)
{
    cap_rights_t rights;

    /* Restrict input/output descriptors and enter Capsicum sandbox. */
    cap_rights_init(&rights, CAP_FSTAT, CAP_WRITE);
    if (caph_rights_limit(fileno(output), &rights) < 0)
	err(EXIT_FAILURE, "unable to limit rights for %s", out_name);
    cap_rights_init(&rights, CAP_FSTAT, CAP_READ);
    if (caph_rights_limit(fileno(input), &rights) < 0)
	err(EXIT_FAILURE, "unable to limit rights for %s", in_name);
    if (caph_enter() < 0)
	err(EXIT_FAILURE, "unable to enter capability mode");
}
#endif

static void
buf_init(struct buffer *buf)
{
    size_t size = 200;
    buf->buf = xmalloc(size);
    buf->l = buf->buf + size - 5 /* safety margin */;
    buf->s = buf->buf + 1;	/* allow accessing buf->e[-1] */
    buf->e = buf->s;
    buf->buf[0] = ' ';
    buf->buf[1] = '\0';
}

static size_t
buf_len(const struct buffer *buf)
{
    return (size_t)(buf->e - buf->s);
}

void
buf_expand(struct buffer *buf, size_t add_size)
{
    size_t new_size = (size_t)(buf->l - buf->s) + 400 + add_size;
    size_t len = buf_len(buf);
    buf->buf = xrealloc(buf->buf, new_size);
    buf->l = buf->buf + new_size - 5;
    buf->s = buf->buf + 1;
    buf->e = buf->s + len;
    /* At this point, the buffer may not be null-terminated anymore. */
}

static void
buf_reserve(struct buffer *buf, size_t n)
{
    if (n >= (size_t)(buf->l - buf->e))
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
search_stmt_newline(bool *force_nl)
{
    inp_comment_init_newline();
    inp_comment_add_char('\n');
    debug_inp(__func__);

    line_no++;

    /*
     * We may have inherited a force_nl == true from the previous token (like
     * a semicolon). But once we know that a newline has been scanned in this
     * loop, force_nl should be false.
     *
     * However, the force_nl == true must be preserved if newline is never
     * scanned in this loop, so this assignment cannot be done earlier.
     */
    *force_nl = false;
}

static void
search_stmt_comment(void)
{
    inp_comment_init_comment();
    inp_comment_add_range(token.s, token.e);
    if (token.e[-1] == '/') {
	while (inp_peek() != '\n')
	    inp_comment_add_char(inp_next());
	debug_inp("search_stmt_comment: end of C99 comment");
    } else {
	while (!inp_comment_complete_block())
	    inp_comment_add_char(inp_next());
	debug_inp("search_stmt_comment: end of block comment");
    }
}

static bool
search_stmt_lbrace(void)
{
    /*
     * Put KNF-style lbraces before the buffered up tokens and jump out of
     * this loop in order to avoid copying the token again.
     */
    if (inp_comment_seen() && opt.brace_same_line) {
	inp_comment_insert_lbrace();
	/*
	 * Originally the lbrace may have been alone on its own line, but it
	 * will be moved into "the else's line", so if there was a newline
	 * resulting from the "{" before, it must be scanned now and ignored.
	 */
	while (ch_isspace(inp_peek())) {
	    inp_skip();
	    if (inp_peek() == '\n')
		break;
	}
	debug_inp(__func__);
	return true;
    }
    return false;
}

static bool
search_stmt_other(lexer_symbol lsym, bool *force_nl,
    bool comment_buffered, bool last_else)
{
    bool remove_newlines;

    remove_newlines =
	/* "} else" */
	(lsym == lsym_else && code.e != code.s && code.e[-1] == '}')
	/* "else if" */
	|| (lsym == lsym_if && last_else && opt.else_if);
    if (remove_newlines)
	*force_nl = false;

    if (!inp_comment_seen()) {
	ps.search_stmt = false;
	return false;
    }

    debug_inp(__func__);
    inp_comment_rtrim_blank();

    if (opt.swallow_optional_blanklines ||
	(!comment_buffered && remove_newlines)) {
	*force_nl = !remove_newlines;
	inp_comment_rtrim_newline();
    }

    if (*force_nl) {		/* if we should insert a newline here, put it
				 * into the buffer */
	*force_nl = false;
	--line_no;		/* this will be re-increased when the newline
				 * is read from the buffer */
	inp_comment_add_char('\n');
	inp_comment_add_char(' ');
	if (opt.verbose)	/* warn if the line was not already broken */
	    diag(0, "Line broken");
    }

    inp_comment_add_range(token.s, token.e);

    debug_inp("search_stmt_other end");
    return true;
}

static void
search_stmt_lookahead(lexer_symbol *lsym)
{
    if (*lsym == lsym_eof)
	return;

    /*
     * The only intended purpose of calling lexi() below is to categorize the
     * next token in order to decide whether to continue buffering forthcoming
     * tokens. Once the buffering is over, lexi() will be called again
     * elsewhere on all of the tokens - this time for normal processing.
     *
     * Calling it for this purpose is a bug, because lexi() also changes the
     * parser state and discards leading whitespace, which is needed mostly
     * for comment-related considerations.
     *
     * Work around the former problem by giving lexi() a copy of the current
     * parser state and discard it if the call turned out to be just a
     * lookahead.
     *
     * Work around the latter problem by copying all whitespace characters
     * into the buffer so that the later lexi() call will read them.
     */
    if (inp_comment_seen()) {
	while (ch_isblank(inp_peek()))
	    inp_comment_add_char(inp_next());
	debug_inp(__func__);
    }

    struct parser_state backup_ps = ps;
    debug_println("backed up parser state");
    *lsym = lexi();
    if (*lsym == lsym_newline || *lsym == lsym_form_feed ||
	*lsym == lsym_comment || ps.search_stmt) {
	ps = backup_ps;
	debug_println("restored parser state");
    }
}

/*
 * Move newlines and comments following an 'if (expr)', 'while (expr)',
 * 'else', etc. up to the start of the following statement to a buffer. This
 * allows proper handling of both kinds of brace placement (-br, -bl) and
 * "cuddling else" (-ce).
 */
static void
search_stmt(lexer_symbol *lsym, bool *force_nl, bool *last_else)
{
    bool comment_buffered = false;

    while (ps.search_stmt) {
	switch (*lsym) {
	case lsym_newline:
	    search_stmt_newline(force_nl);
	    break;
	case lsym_form_feed:
	    /* XXX: Is simply removed from the source code. */
	    break;
	case lsym_comment:
	    search_stmt_comment();
	    comment_buffered = true;
	    break;
	case lsym_lbrace:
	    if (search_stmt_lbrace())
		goto switch_buffer;
	    /* FALLTHROUGH */
	default:
	    if (!search_stmt_other(*lsym, force_nl, comment_buffered,
		    *last_else))
		return;
    switch_buffer:
	    ps.search_stmt = false;
	    inp_comment_add_char(' ');	/* add trailing blank, just in case */
	    inp_from_comment();
	}
	search_stmt_lookahead(lsym);
    }

    *last_else = false;
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

static void __attribute__((__noreturn__))
process_eof(void)
{
    if (lab.s != lab.e || code.s != code.e || com.s != com.e)
	output_line();

    if (ps.tos > 1)		/* check for balanced braces */
	diag(1, "Stuff missing from end of file");

    if (opt.verbose) {
	printf("There were %d output lines and %d comments\n",
	    ps.stats.lines, ps.stats.comments);
	printf("(Lines with comments)/(Lines with code): %6.3f\n",
	    (1.0 * ps.stats.comment_lines) / ps.stats.code_lines);
    }

    fflush(output);
    exit(found_err ? EXIT_FAILURE : EXIT_SUCCESS);
}

static void
maybe_break_line(lexer_symbol lsym, bool *force_nl)
{
    if (!*force_nl)
	return;
    if (lsym == lsym_semicolon)
	return;
    if (lsym == lsym_lbrace && opt.brace_same_line)
	return;

    if (opt.verbose)
	diag(0, "Line broken");
    output_line();
    ps.want_blank = false;
    *force_nl = false;
}

static void
move_com_to_code(void)
{
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
    if (ps.prev_token == lsym_comma && ps.p_l_follow == 0 && !ps.block_init &&
	!opt.break_after_comma && break_comma &&
	com.s == com.e)
	goto stay_in_line;

    output_line();
    ps.want_blank = false;

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
process_lparen_or_lbracket(int decl_ind, bool tabs_to_var, bool spaced_expr)
{
    if (++ps.p_l_follow == array_length(ps.paren)) {
	diag(0, "Reached internal limit of %zu unclosed parentheses",
	    array_length(ps.paren));
	ps.p_l_follow--;
    }

    if (token.s[0] == '(' && ps.in_decl
	&& !ps.block_init && !ps.decl_indent_done &&
	!ps.is_function_definition && ps.paren_level == 0) {
	/* function pointer declarations */
	code_add_decl_indent(decl_ind, tabs_to_var);
	ps.decl_indent_done = true;
    } else if (want_blank_before_lparen())
	*code.e++ = ' ';
    ps.want_blank = false;
    *code.e++ = token.s[0];

    ps.paren[ps.p_l_follow - 1].indent = (short)ind_add(0, code.s, code.e);
    debug_println("paren_indents[%d] is now %d",
	ps.p_l_follow - 1, ps.paren[ps.p_l_follow - 1].indent);

    if (spaced_expr && ps.p_l_follow == 1 && opt.extra_expr_indent
	    && ps.paren[0].indent < 2 * opt.indent_size) {
	ps.paren[0].indent = (short)(2 * opt.indent_size);
	debug_println("paren_indents[0] is now %d", ps.paren[0].indent);
    }

    if (ps.init_or_struct && *token.s == '(' && ps.tos <= 2) {
	/*
	 * this is a kluge to make sure that declarations will be aligned
	 * right if proc decl has an explicit type on it, i.e. "int a(x) {..."
	 */
	parse(psym_semicolon);	/* I said this was a kluge... */
	ps.init_or_struct = false;
    }

    /* parenthesized type following sizeof or offsetof is not a cast */
    if (ps.prev_token == lsym_offsetof || ps.prev_token == lsym_sizeof)
	ps.paren[ps.p_l_follow - 1].no_cast = true;
}

static void
process_rparen_or_rbracket(bool *spaced_expr, bool *force_nl, stmt_head hd)
{
    if (ps.paren[ps.p_l_follow - 1].maybe_cast &&
	!ps.paren[ps.p_l_follow - 1].no_cast) {
	ps.next_unary = true;
	ps.paren[ps.p_l_follow - 1].maybe_cast = false;
	ps.want_blank = opt.space_after_cast;
    } else
	ps.want_blank = true;
    ps.paren[ps.p_l_follow - 1].no_cast = false;

    if (ps.p_l_follow > 0)
	ps.p_l_follow--;
    else
	diag(0, "Extra '%c'", *token.s);

    if (code.e == code.s)	/* if the paren starts the line */
	ps.paren_level = ps.p_l_follow;	/* then indent it */

    *code.e++ = token.s[0];

    if (*spaced_expr && ps.p_l_follow == 0) {	/* check for end of 'if
						 * (...)', or some such */
	*spaced_expr = false;
	*force_nl = true;	/* must force newline after if */
	ps.next_unary = true;
	ps.in_stmt_or_decl = false;	/* don't use stmt continuation
					 * indentation */

	parse_stmt_head(hd);
    }

    /*
     * This should ensure that constructs such as main(){...} and int[]{...}
     * have their braces put in the right place.
     */
    ps.search_stmt = opt.brace_same_line;
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
process_unary_op(int decl_ind, bool tabs_to_var)
{
    if (!ps.decl_indent_done && ps.in_decl && !ps.block_init &&
	!ps.is_function_definition && ps.paren_level == 0) {
	/* pointer declarations */
	code_add_decl_indent(decl_ind - (int)buf_len(&token), tabs_to_var);
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
process_question(int *quest_level)
{
    (*quest_level)++;
    if (ps.want_blank)
	*code.e++ = ' ';
    *code.e++ = '?';
    ps.want_blank = true;
}

static void
process_colon(int *quest_level, bool *force_nl, bool *seen_case)
{
    if (*quest_level > 0) {	/* part of a '?:' operator */
	--*quest_level;
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
    ps.is_case_label = *seen_case;
    *force_nl = *seen_case;
    *seen_case = false;
    ps.want_blank = false;
}

static void
process_semicolon(bool *seen_case, int *quest_level, int decl_ind,
    bool tabs_to_var, bool *spaced_expr, stmt_head hd, bool *force_nl)
{
    if (ps.decl_level == 0)
	ps.init_or_struct = false;
    *seen_case = false;		/* these will only need resetting in an error */
    *quest_level = 0;
    if (ps.prev_token == lsym_rparen_or_rbracket)
	ps.in_func_def_params = false;
    ps.block_init = false;
    ps.block_init_level = 0;
    ps.just_saw_decl--;

    if (ps.in_decl && code.s == code.e && !ps.block_init &&
	!ps.decl_indent_done && ps.paren_level == 0) {
	/* indent stray semicolons in declarations */
	code_add_decl_indent(decl_ind - 1, tabs_to_var);
	ps.decl_indent_done = true;
    }

    ps.in_decl = ps.decl_level > 0;	/* if we were in a first level
					 * structure declaration before, we
					 * aren't anymore */

    if ((!*spaced_expr || hd != hd_for) && ps.p_l_follow > 0) {

	/*
	 * There were unbalanced parentheses in the statement. It is a bit
	 * complicated, because the semicolon might be in a for statement.
	 */
	diag(1, "Unbalanced parentheses");
	ps.p_l_follow = 0;
	if (*spaced_expr) {	/* 'if', 'while', etc. */
	    *spaced_expr = false;
	    parse_stmt_head(hd);
	}
    }
    *code.e++ = ';';
    ps.want_blank = true;
    ps.in_stmt_or_decl = ps.p_l_follow > 0;

    if (!*spaced_expr) {	/* if not if for (;;) */
	parse(psym_semicolon);	/* let parser know about end of stmt */
	*force_nl = true;	/* force newline after an end of stmt */
    }
}

static void
process_lbrace(bool *force_nl, bool *spaced_expr, stmt_head hd,
    int *di_stack, int di_stack_cap, int *decl_ind)
{
    ps.in_stmt_or_decl = false;	/* don't indent the {} */

    if (!ps.block_init)
	*force_nl = true;	/* force other stuff on same line as '{' onto
				 * new line */
    else if (ps.block_init_level <= 0)
	ps.block_init_level = 1;
    else
	ps.block_init_level++;

    if (code.s != code.e && !ps.block_init) {
	if (!opt.brace_same_line) {
	    output_line();
	    ps.want_blank = false;
	} else if (ps.in_func_def_params && !ps.init_or_struct) {
	    ps.ind_level_follow = 0;
	    if (opt.function_brace_split) {	/* dump the line prior to the
						 * brace ... */
		output_line();
		ps.want_blank = false;
	    } else		/* add a space between the decl and brace */
		ps.want_blank = true;
	}
    }

    if (ps.in_func_def_params)
	blank_line_before = false;

    if (ps.p_l_follow > 0) {
	diag(1, "Unbalanced parentheses");
	ps.p_l_follow = 0;
	if (*spaced_expr) {	/* check for unclosed 'if', 'for', etc. */
	    *spaced_expr = false;
	    parse_stmt_head(hd);
	    ps.ind_level = ps.ind_level_follow;
	}
    }

    if (code.s == code.e)
	ps.in_stmt_cont = false;	/* don't indent the '{' itself */
    if (ps.in_decl && ps.init_or_struct) {
	di_stack[ps.decl_level] = *decl_ind;
	if (++ps.decl_level == di_stack_cap) {
	    diag(0, "Reached internal limit of %d struct levels",
		di_stack_cap);
	    ps.decl_level--;
	}
    } else {
	ps.decl_on_line = false;	/* we can't be in the middle of a
					 * declaration, so don't do special
					 * indentation of comments */
	if (opt.blanklines_after_decl_at_top && ps.in_func_def_params)
	    blank_line_after = true;
	ps.in_func_def_params = false;
	ps.in_decl = false;
    }

    *decl_ind = 0;
    parse(psym_lbrace);
    if (ps.want_blank)
	*code.e++ = ' ';
    ps.want_blank = false;
    *code.e++ = '{';
    ps.just_saw_decl = 0;
}

static void
process_rbrace(bool *spaced_expr, int *decl_ind, const int *di_stack)
{
    if (ps.s_sym[ps.tos] == psym_decl && !ps.block_init) {
	/* semicolons can be omitted in declarations */
	parse(psym_semicolon);
    }

    if (ps.p_l_follow > 0) {	/* check for unclosed if, for, else. */
	diag(1, "Unbalanced parentheses");
	ps.p_l_follow = 0;
	*spaced_expr = false;
    }

    ps.just_saw_decl = 0;
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
	*decl_ind = di_stack[--ps.decl_level];
	if (ps.decl_level == 0 && !ps.in_func_def_params) {
	    ps.just_saw_decl = 2;
	    *decl_ind = ps.ind_level == 0
		? opt.decl_indent : opt.local_decl_indent;
	}
	ps.in_decl = true;
    }

    blank_line_before = false;
    parse(psym_rbrace);
    ps.search_stmt = opt.cuddle_else
	&& ps.s_sym[ps.tos] == psym_if_expr_stmt
	&& ps.s_ind_level[ps.tos] >= ps.ind_level;

    if (ps.tos <= 1 && opt.blanklines_after_procs && ps.decl_level <= 0)
	blank_line_after = true;
}

static void
process_do(bool *force_nl, bool *last_else)
{
    ps.in_stmt_or_decl = false;

    if (code.e != code.s) {	/* make sure this starts a line */
	if (opt.verbose)
	    diag(0, "Line broken");
	output_line();
	ps.want_blank = false;
    }

    *force_nl = true;		/* following stuff must go onto new line */
    *last_else = false;
    parse(psym_do);
}

static void
process_else(bool *force_nl, bool *last_else)
{
    ps.in_stmt_or_decl = false;

    if (code.e > code.s && !(opt.cuddle_else && code.e[-1] == '}')) {
	if (opt.verbose)
	    diag(0, "Line broken");
	output_line();		/* make sure this starts a line */
	ps.want_blank = false;
    }

    *force_nl = true;		/* following stuff must go onto new line */
    *last_else = true;
    parse(psym_else);
}

static void
process_type(int *decl_ind, bool *tabs_to_var)
{
    parse(psym_decl);		/* let the parser worry about indentation */

    if (ps.prev_token == lsym_rparen_or_rbracket && ps.tos <= 1) {
	if (code.s != code.e) {
	    output_line();
	    ps.want_blank = false;
	}
    }

    if (ps.in_func_def_params && opt.indent_parameters &&
	    ps.decl_level == 0) {
	ps.ind_level = ps.ind_level_follow = 1;
	ps.in_stmt_cont = false;
    }

    ps.init_or_struct = /* maybe */ true;
    ps.in_decl = ps.decl_on_line = ps.prev_token != lsym_typedef;
    if (ps.decl_level <= 0)
	ps.just_saw_decl = 2;

    blank_line_before = false;

    int len = (int)buf_len(&token) + 1;
    int ind = ps.ind_level == 0 || ps.decl_level > 0
	? opt.decl_indent	/* global variable or local member */
	: opt.local_decl_indent;	/* local variable */
    *decl_ind = ind > 0 ? ind : len;
    *tabs_to_var = opt.use_tabs && ind > 0;
}

static void
process_ident(lexer_symbol lsym, int decl_ind, bool tabs_to_var,
    bool *spaced_expr, bool *force_nl, stmt_head hd)
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
	    ps.paren_level == 0) {
	    code_add_decl_indent(decl_ind, tabs_to_var);
	    ps.decl_indent_done = true;
	    ps.want_blank = false;
	}

    } else if (*spaced_expr && ps.p_l_follow == 0) {
	*spaced_expr = false;
	*force_nl = true;
	ps.next_unary = true;
	ps.in_stmt_or_decl = false;
	parse_stmt_head(hd);
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
process_comma(int decl_ind, bool tabs_to_var, bool *force_nl)
{
    ps.want_blank = code.s != code.e;	/* only put blank after comma if comma
					 * does not start the line */

    if (ps.in_decl && !ps.is_function_definition && !ps.block_init &&
	!ps.decl_indent_done && ps.paren_level == 0) {
	/* indent leading commas and not the actual identifiers */
	code_add_decl_indent(decl_ind - 1, tabs_to_var);
	ps.decl_indent_done = true;
    }

    *code.e++ = ',';

    if (ps.p_l_follow == 0) {
	if (ps.block_init_level <= 0)
	    ps.block_init = false;
	int varname_len = 8;	/* rough estimate for the length of a typical
				 * variable name */
	if (break_comma && (opt.break_after_comma ||
		ind_add(compute_code_indent(), code.s, code.e)
		>= opt.max_line_length - varname_len))
	    *force_nl = true;
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
    int com_start = 0, com_end = 0;

    while (ch_isblank(inp_peek()))
	inp_skip();

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
		com_start = (int)buf_len(&lab) - 2;
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
		com_end = (int)buf_len(&lab);
	    }
	    break;
	}
    }

    while (lab.e > lab.s && ch_isblank(lab.e[-1]))
	lab.e--;
    if (lab.e - lab.s == com_end && !inp_comment_seen()) {
	/* comment on preprocessor line */
	inp_comment_init_preproc();
	inp_comment_add_range(lab.s + com_start, lab.s + com_end);
	lab.e = lab.s + com_start;
	while (lab.e > lab.s && ch_isblank(lab.e[-1]))
	    lab.e--;
	inp_comment_add_char(' ');	/* add trailing blank, just in case */
	inp_from_comment();
    }
    buf_terminate(&lab);
}

static void
process_preprocessing(void)
{
    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	output_line();

    read_preprocessing_line();

    ps.is_case_label = false;

    if (strncmp(lab.s, "#if", 3) == 0) {	/* also ifdef, ifndef */
	if ((size_t)ifdef_level < array_length(state_stack))
	    state_stack[ifdef_level++] = ps;
	else
	    diag(1, "#if stack overflow");

    } else if (strncmp(lab.s, "#el", 3) == 0) {	/* else, elif */
	if (ifdef_level <= 0)
	    diag(1, lab.s[3] == 'i' ? "Unmatched #elif" : "Unmatched #else");
	else
	    ps = state_stack[ifdef_level - 1];

    } else if (strncmp(lab.s, "#endif", 6) == 0) {
	if (ifdef_level <= 0)
	    diag(1, "Unmatched #endif");
	else
	    ifdef_level--;

    } else {
	if (strncmp(lab.s + 1, "pragma", 6) != 0 &&
	    strncmp(lab.s + 1, "error", 5) != 0 &&
	    strncmp(lab.s + 1, "line", 4) != 0 &&
	    strncmp(lab.s + 1, "undef", 5) != 0 &&
	    strncmp(lab.s + 1, "define", 6) != 0 &&
	    strncmp(lab.s + 1, "include", 7) != 0) {
	    diag(1, "Unrecognized cpp directive");
	    return;
	}
    }

    if (opt.blanklines_around_conditional_compilation) {
	blank_line_after = true;
	blank_lines_to_output = 0;
    } else {
	blank_line_after = false;
	blank_line_before = false;
    }

    /*
     * subsequent processing of the newline character will cause the line to
     * be printed
     */
}

static void __attribute__((__noreturn__))
main_loop(void)
{
    bool force_nl = false;	/* when true, code must be broken */
    bool last_else = false;	/* true iff last keyword was an else */
    int decl_ind = 0;		/* current indentation for declarations */
    int di_stack[20];		/* a stack of structure indentation levels */
    bool tabs_to_var = false;	/* true if using tabs to indent to var name */
    bool spaced_expr = false;	/* whether we are in the expression of
				 * if(...), while(...), etc. */
    stmt_head hd = hd_0;	/* the type of statement for 'if (...)', 'for
				 * (...)', etc */
    int quest_level = 0;	/* when this is positive, we have seen a '?'
				 * without the matching ':' in a '?:'
				 * expression */
    bool seen_case = false;	/* set to true when we see a 'case', so we
				 * know what to do with the following colon */

    di_stack[ps.decl_level = 0] = 0;

    for (;;) {			/* loop until we reach eof */
	lexer_symbol lsym = lexi();

	search_stmt(&lsym, &force_nl, &last_else);

	if (lsym == lsym_eof) {
	    process_eof();
	    /* NOTREACHED */
	}

	if (lsym == lsym_newline || lsym == lsym_form_feed ||
		lsym == lsym_preprocessing)
	    force_nl = false;
	else if (lsym != lsym_comment) {
	    maybe_break_line(lsym, &force_nl);
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
	    process_lparen_or_lbracket(decl_ind, tabs_to_var, spaced_expr);
	    break;

	case lsym_rparen_or_rbracket:
	    process_rparen_or_rbracket(&spaced_expr, &force_nl, hd);
	    break;

	case lsym_unary_op:
	    process_unary_op(decl_ind, tabs_to_var);
	    break;

	case lsym_binary_op:
	    process_binary_op();
	    break;

	case lsym_postfix_op:
	    process_postfix_op();
	    break;

	case lsym_question:
	    process_question(&quest_level);
	    break;

	case lsym_case_label:
	    seen_case = true;
	    goto copy_token;

	case lsym_colon:
	    process_colon(&quest_level, &force_nl, &seen_case);
	    break;

	case lsym_semicolon:
	    process_semicolon(&seen_case, &quest_level, decl_ind, tabs_to_var,
		&spaced_expr, hd, &force_nl);
	    break;

	case lsym_lbrace:
	    process_lbrace(&force_nl, &spaced_expr, hd, di_stack,
		(int)array_length(di_stack), &decl_ind);
	    break;

	case lsym_rbrace:
	    process_rbrace(&spaced_expr, &decl_ind, di_stack);
	    break;

	case lsym_switch:
	    spaced_expr = true;	/* the interesting stuff is done after the
				 * expressions are scanned */
	    hd = hd_switch;	/* remember the type of header for later use
				 * by the parser */
	    goto copy_token;

	case lsym_for:
	    spaced_expr = true;
	    hd = hd_for;
	    goto copy_token;

	case lsym_if:
	    spaced_expr = true;
	    hd = hd_if;
	    goto copy_token;

	case lsym_while:
	    spaced_expr = true;
	    hd = hd_while;
	    goto copy_token;

	case lsym_do:
	    process_do(&force_nl, &last_else);
	    goto copy_token;

	case lsym_else:
	    process_else(&force_nl, &last_else);
	    goto copy_token;

	case lsym_typedef:
	case lsym_storage_class:
	    blank_line_before = false;
	    goto copy_token;

	case lsym_tag:
	    if (ps.p_l_follow > 0)
		goto copy_token;
	    /* FALLTHROUGH */
	case lsym_type_outside_parentheses:
	    process_type(&decl_ind, &tabs_to_var);
	    goto copy_token;

	case lsym_type_in_parentheses:
	case lsym_offsetof:
	case lsym_sizeof:
	case lsym_word:
	case lsym_funcname:
	case lsym_return:
	    process_ident(lsym, decl_ind, tabs_to_var, &spaced_expr,
		&force_nl, hd);
    copy_token:
	    copy_token();
	    if (lsym != lsym_funcname)
		ps.want_blank = true;
	    break;

	case lsym_period:
	    process_period();
	    break;

	case lsym_comma:
	    process_comma(decl_ind, tabs_to_var, &force_nl);
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
#if HAVE_CAPSICUM
    init_capsicum();
#endif
    main_prepare_parsing();
    main_loop();
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
