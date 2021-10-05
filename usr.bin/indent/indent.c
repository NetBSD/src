/*	$NetBSD: indent.c,v 1.108 2021/10/05 18:50:42 rillig Exp $	*/

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
__RCSID("$NetBSD: indent.c,v 1.108 2021/10/05 18:50:42 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.c 340138 2018-11-04 19:24:49Z oshogbo $");
#endif

#include <sys/param.h>
#if HAVE_CAPSICUM
#include <sys/capsicum.h>
#include <capsicum_helpers.h>
#endif
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "indent.h"

struct options opt = {
    .btype_2 = true,
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

struct buffer lab;
struct buffer code;
struct buffer com;
struct buffer token;

char *in_buffer;
char *in_buffer_limit;
char *buf_ptr;
char *buf_end;

char sc_buf[sc_size];
char *save_com;
static char *sc_end;		/* pointer into save_com buffer */

char *bp_save;
char *be_save;

bool found_err;
int next_blank_lines;
bool prefix_blankline_requested;
bool postfix_blankline_requested;
bool break_comma;
float case_ind;
bool had_eof;
int line_no;
bool inhibit_formatting;

static int ifdef_level;
static struct parser_state state_stack[5];

FILE *input;
FILE *output;

static void bakcopy(void);
static void indent_declaration(int, bool);

static const char *in_name = "Standard Input";
static const char *out_name = "Standard Output";
static const char *backup_suffix = ".BAK";
static char bakfile[MAXPATHLEN] = "";

static void
check_size_code(size_t desired_size)
{
    if (code.e + desired_size >= code.l)
	buf_expand(&code, desired_size);
}

static void
check_size_label(size_t desired_size)
{
    if (lab.e + desired_size >= lab.l)
	buf_expand(&lab, desired_size);
}

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
search_brace_newline(bool *force_nl)
{
    if (sc_end == NULL) {
	save_com = sc_buf;
	save_com[0] = save_com[1] = ' ';
	sc_end = &save_com[2];
    }
    *sc_end++ = '\n';

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
search_brace_comment(bool *comment_buffered)
{
    if (sc_end == NULL) {
	/*
	 * Copy everything from the start of the line, because
	 * process_comment() will use that to calculate original indentation
	 * of a boxed comment.
	 */
	memcpy(sc_buf, in_buffer, (size_t)(buf_ptr - in_buffer) - 4);
	save_com = sc_buf + (buf_ptr - in_buffer - 4);
	save_com[0] = save_com[1] = ' ';
	sc_end = &save_com[2];
    }
    *comment_buffered = true;
    *sc_end++ = '/';		/* copy in start of comment */
    *sc_end++ = '*';
    for (;;) {			/* loop until the end of the comment */
	*sc_end++ = inbuf_next();
	if (sc_end[-1] == '*' && *buf_ptr == '/')
	    break;		/* we are at end of comment */
	if (sc_end >= &save_com[sc_size]) {	/* check for temp buffer
						 * overflow */
	    diag(1, "Internal buffer overflow - Move big comment from right after if, while, or whatever");
	    fflush(output);
	    exit(1);
	}
    }
    *sc_end++ = '/';		/* add ending slash */
    inbuf_skip();		/* get past / in buffer */
}

static bool
search_brace_lbrace(void)
{
    /*
     * Put KNF-style lbraces before the buffered up tokens and jump out of
     * this loop in order to avoid copying the token again under the default
     * case of the switch below.
     */
    if (sc_end != NULL && opt.btype_2) {
	save_com[0] = '{';
	/*
	 * Originally the lbrace may have been alone on its own line, but it
	 * will be moved into "the else's line", so if there was a newline
	 * resulting from the "{" before, it must be scanned now and ignored.
	 */
	while (isspace((unsigned char)*buf_ptr)) {
	    inbuf_skip();
	    if (*buf_ptr == '\n')
		break;
	}
	return true;
    }
    return false;
}

static bool
search_brace_other(token_type ttype, bool *force_nl,
    bool comment_buffered, bool last_else)
{
    bool remove_newlines;

    remove_newlines =
	    /* "} else" */
	    (ttype == keyword_do_else && *token.s == 'e' &&
	     code.e != code.s && code.e[-1] == '}')
	    /* "else if" */
	    || (ttype == keyword_for_if_while &&
		*token.s == 'i' && last_else && opt.else_if);
    if (remove_newlines)
	*force_nl = false;
    if (sc_end == NULL) {	/* ignore buffering if comment wasn't saved
				 * up */
	ps.search_brace = false;
	return false;
    }
    while (sc_end > save_com && isblank((unsigned char)sc_end[-1])) {
	sc_end--;
    }
    if (opt.swallow_optional_blanklines ||
	(!comment_buffered && remove_newlines)) {
	*force_nl = !remove_newlines;
	while (sc_end > save_com && sc_end[-1] == '\n') {
	    sc_end--;
	}
    }
    if (*force_nl) {	/* if we should insert a nl here, put it into
				 * the buffer */
	*force_nl = false;
	--line_no;		/* this will be re-increased when the newline
				 * is read from the buffer */
	*sc_end++ = '\n';
	*sc_end++ = ' ';
	if (opt.verbose)	/* print error msg if the line was not already
				 * broken */
	    diag(0, "Line broken");
    }
    for (const char *t_ptr = token.s; *t_ptr != '\0'; ++t_ptr)
	*sc_end++ = *t_ptr;
    return true;
}

static void
switch_buffer(void)
{
    ps.search_brace = false;	/* stop looking for start of stmt */
    bp_save = buf_ptr;		/* save current input buffer */
    be_save = buf_end;
    buf_ptr = save_com;		/* fix so that subsequent calls to lexi will
				 * take tokens out of save_com */
    *sc_end++ = ' ';		/* add trailing blank, just in case */
    buf_end = sc_end;
    sc_end = NULL;
    debug_println("switched buf_ptr to save_com");
}

static void
search_brace_lookahead(token_type *ttype)
{
    /*
     * We must make this check, just in case there was an unexpected EOF.
     */
    if (*ttype != end_of_file) {
	/*
	 * The only intended purpose of calling lexi() below is to categorize
	 * the next token in order to decide whether to continue buffering
	 * forthcoming tokens. Once the buffering is over, lexi() will be
	 * called again elsewhere on all of the tokens - this time for normal
	 * processing.
	 *
	 * Calling it for this purpose is a bug, because lexi() also changes
	 * the parser state and discards leading whitespace, which is needed
	 * mostly for comment-related considerations.
	 *
	 * Work around the former problem by giving lexi() a copy of the
	 * current parser state and discard it if the call turned out to be
	 * just a look ahead.
	 *
	 * Work around the latter problem by copying all whitespace characters
	 * into the buffer so that the later lexi() call will read them.
	 */
	if (sc_end != NULL) {
	    while (is_hspace(*buf_ptr)) {
		*sc_end++ = *buf_ptr++;
		if (sc_end >= &save_com[sc_size]) {
		    errx(1, "input too long");
		}
	    }
	    if (buf_ptr >= buf_end)
		fill_buffer();
	}

	struct parser_state transient_state;
	transient_state = ps;
	*ttype = lexi(&transient_state);	/* read another token */
	if (*ttype != newline && *ttype != form_feed &&
	    *ttype != comment && !transient_state.search_brace) {
	    ps = transient_state;
	}
    }
}

static void
search_brace(token_type *ttype, bool *force_nl,
    bool *comment_buffered, bool *last_else)
{
    while (ps.search_brace) {
	switch (*ttype) {
	case newline:
	    search_brace_newline(force_nl);
	    break;
	case form_feed:
	    break;
	case comment:
	    search_brace_comment(comment_buffered);
	    break;
	case lbrace:
	    if (search_brace_lbrace())
		goto sw_buffer;
	    /* FALLTHROUGH */
	default:		/* it is the start of a normal statement */
	    if (!search_brace_other(*ttype, force_nl,
		    *comment_buffered, *last_else))
		return;
	sw_buffer:
	    switch_buffer();
	}
	search_brace_lookahead(ttype);
    }

    *last_else = false;
}

static void
buf_init(struct buffer *buf)
{
    buf->buf = xmalloc(bufsize);
    buf->buf[0] = ' ';		/* allow accessing buf->e[-1] */
    buf->buf[1] = '\0';
    buf->s = buf->buf + 1;
    buf->e = buf->s;
    buf->l = buf->buf + bufsize - 5;	/* safety margin, though unreliable */
}

void
buf_expand(struct buffer *buf, size_t desired_size)
{
    size_t nsize = buf->l - buf->s + 400 + desired_size;
    size_t code_len = buf->e - buf->s;
    buf->buf = xrealloc(buf->buf, nsize);
    buf->e = buf->buf + code_len + 1;
    buf->l = buf->buf + nsize - 5;
    buf->s = buf->buf + 1;
}

static void
main_init_globals(void)
{
    found_err = false;

    ps.p_stack[0] = stmt;	/* this is the parser's stack */
    ps.last_nl = true;		/* this is true if the last thing scanned was
				 * a newline */
    ps.last_token = semicolon;
    buf_init(&com);
    buf_init(&lab);
    buf_init(&code);
    buf_init(&token);
    opt.else_if = true;		/* XXX: redundant? */

    in_buffer = xmalloc(10);
    in_buffer_limit = in_buffer + 8;
    buf_ptr = buf_end = in_buffer;
    line_no = 1;
    had_eof = ps.in_decl = ps.decl_on_line = (break_comma = false);
    ps.in_or_st = false;
    ps.want_blank = ps.in_stmt = ps.ind_stmt = false;

    ps.pcase = false;
    sc_end = NULL;
    bp_save = NULL;
    be_save = NULL;

    output = NULL;

    const char *suffix = getenv("SIMPLE_BACKUP_SUFFIX");
    if (suffix != NULL)
	backup_suffix = suffix;
}

static void
main_parse_command_line(int argc, char **argv)
{
    int i;
    const char *profile_name = NULL;

    for (i = 1; i < argc; ++i)
	if (strcmp(argv[i], "-npro") == 0)
	    break;
	else if (argv[i][0] == '-' && argv[i][1] == 'P' && argv[i][2] != '\0')
	    profile_name = argv[i] + 2;	/* non-empty -P (set profile) */
    if (i >= argc)
	load_profiles(profile_name);

    for (i = 1; i < argc; ++i) {
	if (argv[i][0] == '-') {
	    set_option(argv[i]);

	} else if (input == NULL) {
	    in_name = argv[i];
	    input = fopen(in_name, "r");
	    if (input == NULL)
		err(1, "%s", in_name);

	} else if (output == NULL) {
	    out_name = argv[i];
	    if (strcmp(in_name, out_name) == 0)
		errx(1, "input and output files must be different");
	    output = fopen(out_name, "w");
	    if (output == NULL)
		err(1, "%s", out_name);

	} else
	    errx(1, "unknown parameter: %s", argv[i]);
    }

    if (input == NULL)
	input = stdin;
    if (output == NULL) {
	if (input == stdin)
	    output = stdout;
	else {
	    out_name = in_name;
	    bakcopy();
	}
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
    fill_buffer();		/* get first batch of stuff into input buffer */

    parse(semicolon);

    char *p = buf_ptr;
    int ind = 0;

    for (;;) {
	if (*p == ' ')
	    ind++;
	else if (*p == '\t')
	    ind = opt.tabsize * (1 + ind / opt.tabsize);
	else
	    break;
	p++;
    }
    if (ind >= opt.indent_size)
	ps.ind_level = ps.ind_level_follow = ind / opt.indent_size;
}

static void __attribute__((__noreturn__))
process_end_of_file(void)
{
    if (lab.s != lab.e || code.s != code.e || com.s != com.e)
	dump_line();

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
process_comment_in_code(token_type ttype, bool *force_nl)
{
    if (*force_nl &&
	ttype != semicolon &&
	(ttype != lbrace || !opt.btype_2)) {

	/* we should force a broken line here */
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();
	ps.want_blank = false;	/* don't insert blank at line start */
	*force_nl = false;
    }

    ps.in_stmt = true;		/* turn on flag which causes an extra level of
				 * indentation. this is turned off by a ';' or
				 * '}' */
    if (com.s != com.e) {	/* the turkey has embedded a comment in a
				 * line. fix it */
	size_t len = com.e - com.s;

	check_size_code(len + 3);
	*code.e++ = ' ';
	memcpy(code.e, com.s, len);
	code.e += len;
	*code.e++ = ' ';
	*code.e = '\0';
	ps.want_blank = false;
	com.e = com.s;
    }
}

static void
process_form_feed(void)
{
    ps.use_ff = true;		/* a form feed is treated much like a newline */
    dump_line();
    ps.want_blank = false;
}

static void
process_newline(void)
{
    if (ps.last_token != comma || ps.p_l_follow > 0 || opt.break_after_comma
	|| ps.block_init || !break_comma || com.s != com.e) {
	dump_line();
	ps.want_blank = false;
    }
    ++line_no;			/* keep track of input line number */
}

static bool
want_blank_before_lparen(void)
{
    if (!ps.want_blank)
	return false;
    if (ps.last_token == rparen)
	return false;
    if (ps.last_token != ident && ps.last_token != funcname)
	return true;
    if (opt.proc_calls_space)
	return true;
    if (ps.keyword == kw_sizeof)
	return opt.blank_after_sizeof;
    return ps.keyword != kw_0 && ps.keyword != kw_offsetof;
}

static void
process_lparen_or_lbracket(int decl_ind, bool tabs_to_var, bool sp_sw)
{
    if (++ps.p_l_follow == nitems(ps.paren_indents)) {
	diag(0, "Reached internal limit of %zu unclosed parens",
	    nitems(ps.paren_indents));
	ps.p_l_follow--;
    }
    if (token.s[0] == '(' && ps.in_decl
	&& !ps.block_init && !ps.dumped_decl_indent &&
	ps.procname[0] == '\0' && ps.paren_level == 0) {
	/* function pointer declarations */
	indent_declaration(decl_ind, tabs_to_var);
	ps.dumped_decl_indent = true;
    } else if (want_blank_before_lparen())
	*code.e++ = ' ';
    ps.want_blank = false;
    *code.e++ = token.s[0];

    ps.paren_indents[ps.p_l_follow - 1] =
	(short)indentation_after_range(0, code.s, code.e);
    debug_println("paren_indent[%d] is now %d",
	ps.p_l_follow - 1, ps.paren_indents[ps.p_l_follow - 1]);

    if (sp_sw && ps.p_l_follow == 1 && opt.extra_expression_indent
	    && ps.paren_indents[0] < 2 * opt.indent_size) {
	ps.paren_indents[0] = (short)(2 * opt.indent_size);
	debug_println("paren_indent[0] is now %d", ps.paren_indents[0]);
    }
    if (ps.in_or_st && *token.s == '(' && ps.tos <= 2) {
	/*
	 * this is a kluge to make sure that declarations will be aligned
	 * right if proc decl has an explicit type on it, i.e. "int a(x) {..."
	 */
	parse(semicolon);	/* I said this was a kluge... */
	ps.in_or_st = false;	/* turn off flag for structure decl or
				 * initialization */
    }
    /* parenthesized type following sizeof or offsetof is not a cast */
    if (ps.keyword == kw_offsetof || ps.keyword == kw_sizeof)
	ps.not_cast_mask |= 1 << ps.p_l_follow;
}

static void
process_rparen_or_rbracket(bool *sp_sw, bool *force_nl,
    token_type hd_type)
{
    if ((ps.cast_mask & (1 << ps.p_l_follow) & ~ps.not_cast_mask) != 0) {
	ps.last_u_d = true;
	ps.cast_mask &= (1 << ps.p_l_follow) - 1;
	ps.want_blank = opt.space_after_cast;
    } else
	ps.want_blank = true;
    ps.not_cast_mask &= (1 << ps.p_l_follow) - 1;

    if (--ps.p_l_follow < 0) {
	ps.p_l_follow = 0;
	diag(0, "Extra %c", *token.s);
    }

    if (code.e == code.s)	/* if the paren starts the line */
	ps.paren_level = ps.p_l_follow;	/* then indent it */

    *code.e++ = token.s[0];

    if (*sp_sw && (ps.p_l_follow == 0)) {	/* check for end of if (...),
						 * or some such */
	*sp_sw = false;
	*force_nl = true;	/* must force newline after if */
	ps.last_u_d = true;	/* inform lexi that a following operator is
				 * unary */
	ps.in_stmt = false;	/* don't use stmt continuation indentation */

	parse(hd_type);		/* let parser worry about if, or whatever */
    }
    ps.search_brace = opt.btype_2;	/* this should ensure that constructs
					 * such as main(){...} and int[]{...}
					 * have their braces put in the right
					 * place */
}

static void
process_unary_op(int decl_ind, bool tabs_to_var)
{
    if (!ps.dumped_decl_indent && ps.in_decl && !ps.block_init &&
	ps.procname[0] == '\0' && ps.paren_level == 0) {
	/* pointer declarations */
	indent_declaration(decl_ind - (int)strlen(token.s), tabs_to_var);
	ps.dumped_decl_indent = true;
    } else if (ps.want_blank)
	*code.e++ = ' ';

    {
	size_t len = token.e - token.s;

	check_size_code(len);
	memcpy(code.e, token.s, len);
	code.e += len;
    }
    ps.want_blank = false;
}

static void
process_binary_op(void)
{
    size_t len = token.e - token.s;

    check_size_code(len + 1);
    if (ps.want_blank)
	*code.e++ = ' ';
    memcpy(code.e, token.s, len);
    code.e += len;

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
process_question(int *seen_quest)
{
    (*seen_quest)++;		/* this will be used when a later colon
				 * appears, so we can distinguish the
				 * <c>?<n>:<n> construct */
    if (ps.want_blank)
	*code.e++ = ' ';
    *code.e++ = '?';
    ps.want_blank = true;
}

static void
process_colon(int *seen_quest, bool *force_nl, bool *seen_case)
{
    if (*seen_quest > 0) {	/* it is part of the <c>?<n>: <n> construct */
	--*seen_quest;
	if (ps.want_blank)
	    *code.e++ = ' ';
	*code.e++ = ':';
	ps.want_blank = true;
	return;
    }
    if (ps.in_or_st) {
	*code.e++ = ':';
	ps.want_blank = false;
	return;
    }
    ps.in_stmt = false;		/* seeing a label does not imply we are in a
				 * stmt */
    /*
     * turn everything so far into a label
     */
    {
	size_t len = code.e - code.s;

	check_size_label(len + 3);
	memcpy(lab.e, code.s, len);
	lab.e += len;
	*lab.e++ = ':';
	*lab.e = '\0';
	code.e = code.s;
    }
    ps.pcase = *seen_case;	/* will be used by dump_line to decide how to
				 * indent the label. */
    *force_nl = *seen_case;	/* will force a 'case n:' to be on a
				 * line by itself */
    *seen_case = false;
    ps.want_blank = false;
}

static void
process_semicolon(bool *seen_case, int *seen_quest, int decl_ind,
    bool tabs_to_var, bool *sp_sw,
    token_type hd_type,
    bool *force_nl)
{
    if (ps.decl_nest == 0)
	ps.in_or_st = false;	/* we are not in an initialization or
				 * structure declaration */
    *seen_case = false;		/* these will only need resetting in an error */
    *seen_quest = 0;
    if (ps.last_token == rparen)
	ps.in_parameter_declaration = false;
    ps.cast_mask = 0;
    ps.not_cast_mask = 0;
    ps.block_init = false;
    ps.block_init_level = 0;
    ps.just_saw_decl--;

    if (ps.in_decl && code.s == code.e && !ps.block_init &&
	!ps.dumped_decl_indent && ps.paren_level == 0) {
	/* indent stray semicolons in declarations */
	indent_declaration(decl_ind - 1, tabs_to_var);
	ps.dumped_decl_indent = true;
    }

    ps.in_decl = (ps.decl_nest > 0);	/* if we were in a first level
					 * structure declaration, we aren't
					 * anymore */

    if ((!*sp_sw || hd_type != for_exprs) && ps.p_l_follow > 0) {

	/*
	 * This should be true iff there were unbalanced parens in the stmt.
	 * It is a bit complicated, because the semicolon might be in a for
	 * stmt
	 */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	if (*sp_sw) {		/* this is a check for an if, while, etc. with
				 * unbalanced parens */
	    *sp_sw = false;
	    parse(hd_type);	/* don't lose the 'if', or whatever */
	}
    }
    *code.e++ = ';';
    ps.want_blank = true;
    ps.in_stmt = (ps.p_l_follow > 0);	/* we are no longer in the middle of a
					 * stmt */

    if (!*sp_sw) {		/* if not if for (;;) */
	parse(semicolon);	/* let parser know about end of stmt */
	*force_nl = true;	/* force newline after an end of stmt */
    }
}

static void
process_lbrace(bool *force_nl, bool *sp_sw, token_type hd_type,
    int *di_stack, int di_stack_cap, int *decl_ind)
{
    ps.in_stmt = false;		/* don't indent the {} */
    if (!ps.block_init)
	*force_nl = true;	/* force other stuff on same line as '{' onto
				 * new line */
    else if (ps.block_init_level <= 0)
	ps.block_init_level = 1;
    else
	ps.block_init_level++;

    if (code.s != code.e && !ps.block_init) {
	if (!opt.btype_2) {
	    dump_line();
	    ps.want_blank = false;
	} else if (ps.in_parameter_declaration && !ps.in_or_st) {
	    ps.ind_level_follow = 0;
	    if (opt.function_brace_split) {	/* dump the line prior to the
						 * brace ... */
		dump_line();
		ps.want_blank = false;
	    } else		/* add a space between the decl and brace */
		ps.want_blank = true;
	}
    }
    if (ps.in_parameter_declaration)
	prefix_blankline_requested = false;

    if (ps.p_l_follow > 0) {	/* check for preceding unbalanced parens */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	if (*sp_sw) {		/* check for unclosed if, for, etc. */
	    *sp_sw = false;
	    parse(hd_type);
	    ps.ind_level = ps.ind_level_follow;
	}
    }
    if (code.s == code.e)
	ps.ind_stmt = false;	/* don't put extra indentation on a line
				 * with '{' */
    if (ps.in_decl && ps.in_or_st) {	/* this is either a structure
					 * declaration or an init */
	di_stack[ps.decl_nest] = *decl_ind;
	if (++ps.decl_nest == di_stack_cap) {
	    diag(0, "Reached internal limit of %d struct levels",
		di_stack_cap);
	    ps.decl_nest--;
	}
    } else {
	ps.decl_on_line = false;	/* we can't be in the middle of a
					 * declaration, so don't do special
					 * indentation of comments */
	if (opt.blanklines_after_declarations_at_proctop
	    && ps.in_parameter_declaration)
	    postfix_blankline_requested = true;
	ps.in_parameter_declaration = false;
	ps.in_decl = false;
    }
    *decl_ind = 0;
    parse(lbrace);		/* let parser know about this */
    if (ps.want_blank)		/* put a blank before '{' if '{' is not at
				 * start of line */
	*code.e++ = ' ';
    ps.want_blank = false;
    *code.e++ = '{';
    ps.just_saw_decl = 0;
}

static void
process_rbrace(bool *sp_sw, int *decl_ind, const int *di_stack)
{
    if (ps.p_stack[ps.tos] == decl && !ps.block_init)	/* semicolons can be
							 * omitted in
							 * declarations */
	parse(semicolon);
    if (ps.p_l_follow != 0) {	/* check for unclosed if, for, else. */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	*sp_sw = false;
    }
    ps.just_saw_decl = 0;
    ps.block_init_level--;
    if (code.s != code.e && !ps.block_init) {	/* '}' must be first on line */
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();
    }
    *code.e++ = '}';
    ps.want_blank = true;
    ps.in_stmt = ps.ind_stmt = false;
    if (ps.decl_nest > 0) { /* we are in multi-level structure declaration */
	*decl_ind = di_stack[--ps.decl_nest];
	if (ps.decl_nest == 0 && !ps.in_parameter_declaration)
	    ps.just_saw_decl = 2;
	ps.in_decl = true;
    }
    prefix_blankline_requested = false;
    parse(rbrace);		/* let parser know about this */
    ps.search_brace = opt.cuddle_else
	&& ps.p_stack[ps.tos] == if_expr_stmt
	&& ps.il[ps.tos] >= ps.ind_level;
    if (ps.tos <= 1 && opt.blanklines_after_procs && ps.decl_nest <= 0)
	postfix_blankline_requested = true;
}

static void
process_keyword_do_else(bool *force_nl, bool *last_else)
{
    ps.in_stmt = false;
    if (*token.s == 'e') {
	if (code.e != code.s && (!opt.cuddle_else || code.e[-1] != '}')) {
	    if (opt.verbose)
		diag(0, "Line broken");
	    dump_line();	/* make sure this starts a line */
	    ps.want_blank = false;
	}
	*force_nl = true;	/* following stuff must go onto new line */
	*last_else = true;
	parse(keyword_else);
    } else {
	if (code.e != code.s) {	/* make sure this starts a line */
	    if (opt.verbose)
		diag(0, "Line broken");
	    dump_line();
	    ps.want_blank = false;
	}
	*force_nl = true;	/* following stuff must go onto new line */
	*last_else = false;
	parse(keyword_do);
    }
}

static void
process_decl(int *out_decl_ind, bool *out_tabs_to_var)
{
    parse(decl);		/* let parser worry about indentation */
    if (ps.last_token == rparen && ps.tos <= 1) {
	if (code.s != code.e) {
	    dump_line();
	    ps.want_blank = false;
	}
    }
    if (ps.in_parameter_declaration && opt.indent_parameters &&
	ps.decl_nest == 0) {
	ps.ind_level = ps.ind_level_follow = 1;
	ps.ind_stmt = false;
    }
    ps.in_or_st = true;		/* this might be a structure or initialization
				 * declaration */
    ps.in_decl = ps.decl_on_line = ps.last_token != type_def;
    if ( /* !ps.in_or_st && */ ps.decl_nest <= 0)
	ps.just_saw_decl = 2;
    prefix_blankline_requested = false;

    int len = (int)strlen(token.s) + 1;
    int ind = ps.ind_level == 0 || ps.decl_nest > 0
	    ? opt.decl_indent		/* global variable or local member */
	    : opt.local_decl_indent;	/* local variable */
    *out_decl_ind = ind > 0 ? ind : len;
    *out_tabs_to_var = opt.use_tabs ? ind > 0 : false;
}

static void
process_ident(token_type ttype, int decl_ind, bool tabs_to_var,
    bool *sp_sw, bool *force_nl, token_type hd_type)
{
    if (ps.in_decl) {
	if (ttype == funcname) {
	    ps.in_decl = false;
	    if (opt.procnames_start_line && code.s != code.e) {
		*code.e = '\0';
		dump_line();
	    } else if (ps.want_blank) {
		*code.e++ = ' ';
	    }
	    ps.want_blank = false;
	} else if (!ps.block_init && !ps.dumped_decl_indent &&
	    ps.paren_level == 0) {	/* if we are in a declaration, we must
					 * indent identifier */
	    indent_declaration(decl_ind, tabs_to_var);
	    ps.dumped_decl_indent = true;
	    ps.want_blank = false;
	}
    } else if (*sp_sw && ps.p_l_follow == 0) {
	*sp_sw = false;
	*force_nl = true;
	ps.last_u_d = true;
	ps.in_stmt = false;
	parse(hd_type);
    }
}

static void
copy_id(void)
{
    size_t len = token.e - token.s;

    check_size_code(len + 1);
    if (ps.want_blank)
	*code.e++ = ' ';
    memcpy(code.e, token.s, len);
    code.e += len;
}

static void
process_string_prefix(void)
{
    size_t len = token.e - token.s;

    check_size_code(len + 1);
    if (ps.want_blank)
	*code.e++ = ' ';
    memcpy(code.e, token.s, len);
    code.e += len;

    ps.want_blank = false;
}

static void
process_period(void)
{
    if (code.e[-1] == ',')
	*code.e++ = ' ';
    *code.e++ = '.';
    ps.want_blank = false;
}

static void
process_comma(int decl_ind, bool tabs_to_var, bool *force_nl)
{
    ps.want_blank = (code.s != code.e);	/* only put blank after comma if comma
					 * does not start the line */
    if (ps.in_decl && ps.procname[0] == '\0' && !ps.block_init &&
	!ps.dumped_decl_indent && ps.paren_level == 0) {
	/* indent leading commas and not the actual identifiers */
	indent_declaration(decl_ind - 1, tabs_to_var);
	ps.dumped_decl_indent = true;
    }
    *code.e++ = ',';
    if (ps.p_l_follow == 0) {
	if (ps.block_init_level <= 0)
	    ps.block_init = false;
	if (break_comma && (opt.break_after_comma ||
			    indentation_after_range(
				    compute_code_indent(), code.s, code.e)
			    >= opt.max_line_length - opt.tabsize))
	    *force_nl = true;
    }
}

static void
process_preprocessing(void)
{
    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	dump_line();
    check_size_label(1);
    *lab.e++ = '#';		/* move whole line to 'label' buffer */

    {
	bool in_comment = false;
	int com_start = 0;
	char quote = '\0';
	int com_end = 0;

	while (is_hspace(*buf_ptr))
	    inbuf_skip();

	while (*buf_ptr != '\n' || (in_comment && !had_eof)) {
	    check_size_label(2);
	    *lab.e++ = inbuf_next();
	    switch (lab.e[-1]) {
	    case '\\':
		if (!in_comment)
		    *lab.e++ = inbuf_next();
		break;
	    case '/':
		if (*buf_ptr == '*' && !in_comment && quote == '\0') {
		    in_comment = true;
		    *lab.e++ = *buf_ptr++;
		    com_start = (int)(lab.e - lab.s) - 2;
		}
		break;
	    case '"':
		if (quote == '"')
		    quote = '\0';
		else if (quote == '\0')
		    quote = '"';
		break;
	    case '\'':
		if (quote == '\'')
		    quote = '\0';
		else if (quote == '\0')
		    quote = '\'';
		break;
	    case '*':
		if (*buf_ptr == '/' && in_comment) {
		    in_comment = false;
		    *lab.e++ = *buf_ptr++;
		    com_end = (int)(lab.e - lab.s);
		}
		break;
	    }
	}

	while (lab.e > lab.s && is_hspace(lab.e[-1]))
	    lab.e--;
	if (lab.e - lab.s == com_end && bp_save == NULL) {
	    /* comment on preprocessor line */
	    if (sc_end == NULL) {	/* if this is the first comment, we
					 * must set up the buffer */
		save_com = sc_buf;
		sc_end = save_com;
	    } else {
		*sc_end++ = '\n';	/* add newline between comments */
		*sc_end++ = ' ';
		--line_no;
	    }
	    if (sc_end - save_com + com_end - com_start > sc_size)
		errx(1, "input too long");
	    memmove(sc_end, lab.s + com_start, (size_t)(com_end - com_start));
	    sc_end += com_end - com_start;
	    lab.e = lab.s + com_start;
	    while (lab.e > lab.s && is_hspace(lab.e[-1]))
		lab.e--;
	    bp_save = buf_ptr;	/* save current input buffer */
	    be_save = buf_end;
	    buf_ptr = save_com;	/* fix so that subsequent calls to lexi will
				 * take tokens out of save_com */
	    *sc_end++ = ' ';	/* add trailing blank, just in case */
	    buf_end = sc_end;
	    sc_end = NULL;
	    debug_println("switched buf_ptr to save_com");
	}
	check_size_label(1);
	*lab.e = '\0';		/* null terminate line */
	ps.pcase = false;
    }

    if (strncmp(lab.s, "#if", 3) == 0) {	/* also ifdef, ifndef */
	if ((size_t)ifdef_level < nitems(state_stack))
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
	postfix_blankline_requested = true;
	next_blank_lines = 0;
    } else {
	postfix_blankline_requested = false;
	prefix_blankline_requested = false;
    }

    /*
     * subsequent processing of the newline character will cause the line to
     * be printed
     */
}

static void __attribute__((__noreturn__))
main_loop(void)
{
    token_type ttype;
    bool force_nl;		/* when true, code must be broken */
    bool last_else = false;	/* true iff last keyword was an else */
    int decl_ind;		/* current indentation for declarations */
    int di_stack[20];		/* a stack of structure indentation levels */
    bool tabs_to_var;		/* true if using tabs to indent to var name */
    bool sp_sw;			/* when true, we are in the expression of
				 * if(...), while(...), etc. */
    token_type hd_type = end_of_file;	/* used to store type of stmt for if
					 * (...), for (...), etc */
    int seen_quest;		/* when this is positive, we have seen a '?'
				 * without the matching ':' in a <c>?<s>:<s>
				 * construct */
    bool seen_case;		/* set to true when we see a 'case', so we
				 * know what to do with the following colon */

    sp_sw = force_nl = false;
    decl_ind = 0;
    di_stack[ps.decl_nest = 0] = 0;
    seen_case = false;
    seen_quest = 0;
    tabs_to_var = false;

    for (;;) {			/* this is the main loop.  it will go until we
				 * reach eof */
	bool comment_buffered = false;

	ttype = lexi(&ps);	/* Read the next token.  The actual characters
				 * read are stored in "token". */

	/*
	 * The following code moves newlines and comments following an if (),
	 * while (), else, etc. up to the start of the following stmt to a
	 * buffer. This allows proper handling of both kinds of brace
	 * placement (-br, -bl) and cuddling "else" (-ce).
	 */
	search_brace(&ttype, &force_nl, &comment_buffered, &last_else);

	if (ttype == end_of_file) {
	    process_end_of_file();
	    /* NOTREACHED */
	}

	if (
		ttype != comment &&
		ttype != newline &&
		ttype != preprocessing &&
		ttype != form_feed) {
	    process_comment_in_code(ttype, &force_nl);

	} else if (ttype != comment)	/* preserve force_nl through a comment */
	    force_nl = false;	/* cancel forced newline after newline, form
				 * feed, etc */



	/*-----------------------------------------------------*\
	|	   do switch on type of token scanned		|
	\*-----------------------------------------------------*/
	check_size_code(3);	/* maximum number of increments of code.e
				 * before the next check_size_code or
				 * dump_line() is 2. After that there's the
				 * final increment for the null character. */
	switch (ttype) {

	case form_feed:
	    process_form_feed();
	    break;

	case newline:
	    process_newline();
	    break;

	case lparen:		/* got a '(' or '[' */
	    process_lparen_or_lbracket(decl_ind, tabs_to_var, sp_sw);
	    break;

	case rparen:		/* got a ')' or ']' */
	    process_rparen_or_rbracket(&sp_sw, &force_nl, hd_type);
	    break;

	case unary_op:		/* this could be any unary operation */
	    process_unary_op(decl_ind, tabs_to_var);
	    break;

	case binary_op:		/* any binary operation */
	    process_binary_op();
	    break;

	case postfix_op:	/* got a trailing ++ or -- */
	    process_postfix_op();
	    break;

	case question:		/* got a ? */
	    process_question(&seen_quest);
	    break;

	case case_label:	/* got word 'case' or 'default' */
	    seen_case = true;	/* so we can process the later colon properly */
	    goto copy_id;

	case colon:		/* got a ':' */
	    process_colon(&seen_quest, &force_nl, &seen_case);
	    break;

	case semicolon:		/* got a ';' */
	    process_semicolon(&seen_case, &seen_quest, decl_ind, tabs_to_var,
		&sp_sw, hd_type, &force_nl);
	    break;

	case lbrace:		/* got a '{' */
	    process_lbrace(&force_nl, &sp_sw, hd_type, di_stack,
		(int)nitems(di_stack), &decl_ind);
	    break;

	case rbrace:		/* got a '}' */
	    process_rbrace(&sp_sw, &decl_ind, di_stack);
	    break;

	case switch_expr:	/* got keyword "switch" */
	    sp_sw = true;
	    hd_type = switch_expr;	/* keep this for when we have seen the
					 * expression */
	    goto copy_id;	/* go move the token into buffer */

	case keyword_for_if_while:
	    sp_sw = true;	/* the interesting stuff is done after the
				 * expression is scanned */
	    hd_type = (*token.s == 'i' ? if_expr :
		(*token.s == 'w' ? while_expr : for_exprs));

	    /* remember the type of header for later use by parser */
	    goto copy_id;	/* copy the token into line */

	case keyword_do_else:
	    process_keyword_do_else(&force_nl, &last_else);
	    goto copy_id;	/* move the token into line */

	case type_def:
	case storage_class:
	    prefix_blankline_requested = false;
	    goto copy_id;

	case keyword_struct_union_enum:
	    if (ps.p_l_follow > 0)
		goto copy_id;
	    /* FALLTHROUGH */
	case decl:		/* we have a declaration type (int, etc.) */
	    process_decl(&decl_ind, &tabs_to_var);
	    goto copy_id;

	case funcname:
	case ident:		/* got an identifier or constant */
	    process_ident(ttype, decl_ind, tabs_to_var, &sp_sw, &force_nl,
		hd_type);
    copy_id:
	    copy_id();
	    if (ttype != funcname)
		ps.want_blank = true;
	    break;

	case string_prefix:
	    process_string_prefix();
	    break;

	case period:
	    process_period();
	    break;

	case comma:
	    process_comma(decl_ind, tabs_to_var, &force_nl);
	    break;

	case preprocessing:	/* '#' */
	    process_preprocessing();
	    break;
	case comment:		/* the initial '/' '*' or '//' of a comment */
	    process_comment();
	    break;

	default:
	    break;
	}

	*code.e = '\0';
	if (ttype != comment &&
	    ttype != newline &&
	    ttype != preprocessing)
	    ps.last_token = ttype;
    }
}

int
main(int argc, char **argv)
{
    main_init_globals();
    main_parse_command_line(argc, argv);
#if HAVE_CAPSICUM
    init_capsicum();
#endif
    main_prepare_parsing();
    main_loop();
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
indent_declaration(int cur_decl_ind, bool tabs_to_var)
{
    int pos = (int)(code.e - code.s);
    char *startpos = code.e;

    /*
     * get the tab math right for indentations that are not multiples of
     * tabsize
     */
    if ((ps.ind_level * opt.indent_size) % opt.tabsize != 0) {
	pos += (ps.ind_level * opt.indent_size) % opt.tabsize;
	cur_decl_ind += (ps.ind_level * opt.indent_size) % opt.tabsize;
    }
    if (tabs_to_var) {
	int tpos;

	check_size_code((size_t)(cur_decl_ind / opt.tabsize));
	while ((tpos = opt.tabsize * (1 + pos / opt.tabsize)) <= cur_decl_ind) {
	    *code.e++ = '\t';
	    pos = tpos;
	}
    }
    check_size_code((size_t)(cur_decl_ind - pos) + 1);
    while (pos < cur_decl_ind) {
	*code.e++ = ' ';
	pos++;
    }
    if (code.e == startpos && ps.want_blank) {
	*code.e++ = ' ';
	ps.want_blank = false;
    }
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
	if (isprint((unsigned char)*p) && *p != '\\' && *p != '"')
	    debug_printf("%c", *p);
	else if (*p == '\n')
	    debug_printf("\\n");
	else if (*p == '\t')
	    debug_printf("\\t");
	else
	    debug_printf("\\x%02x", *p);
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
