/*	$NetBSD: indent.c,v 1.61 2021/08/25 22:26:30 rillig Exp $	*/

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
#ifndef lint
static char sccsid[] = "@(#)indent.c	5.17 (Berkeley) 6/7/93";
#endif /* not lint */
#endif

#include <sys/cdefs.h>
#ifndef lint
#if defined(__NetBSD__)
__RCSID("$NetBSD: indent.c,v 1.61 2021/08/25 22:26:30 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.c 340138 2018-11-04 19:24:49Z oshogbo $");
#endif
#endif

#include <sys/param.h>
#if HAVE_CAPSICUM
#include <sys/capsicum.h>
#include <capsicum_helpers.h>
#endif
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "indent.h"

struct options opt;
struct parser_state ps;

char       *labbuf;
char       *s_lab;
char       *e_lab;
char       *l_lab;

char       *codebuf;
char       *s_code;
char       *e_code;
char       *l_code;

char       *combuf;
char       *s_com;
char       *e_com;
char       *l_com;

char       *tokenbuf;
char	   *s_token;
char       *e_token;
char	   *l_token;

char       *in_buffer;
char	   *in_buffer_limit;
char       *buf_ptr;
char       *buf_end;

char        sc_buf[sc_size];
char       *save_com;
char       *sc_end;

char       *bp_save;
char       *be_save;

int         found_err;
int         n_real_blanklines;
int         prefix_blankline_requested;
int         postfix_blankline_requested;
int         break_comma;
float       case_ind;
int         code_lines;
int         had_eof;
int         line_no;
int         inhibit_formatting;
int         suppress_blanklines;

int         ifdef_level;
struct parser_state state_stack[5];
struct parser_state match_state[5];

FILE       *input;
FILE       *output;

static void bakcopy(void);
static void indent_declaration(int, int);

const char *in_name = "Standard Input";	/* will always point to name of input
					 * file */
const char *out_name = "Standard Output";	/* will always point to name
						 * of output file */
const char *simple_backup_suffix = ".BAK";	/* Suffix to use for backup
						 * files */
char        bakfile[MAXPATHLEN] = "";

static void
check_size_code(size_t desired_size)
{
    if (e_code + (desired_size) < l_code)
        return;

    size_t nsize = l_code - s_code + 400 + desired_size;
    size_t code_len = e_code - s_code;
    codebuf = realloc(codebuf, nsize);
    if (codebuf == NULL)
	err(1, NULL);
    e_code = codebuf + code_len + 1;
    l_code = codebuf + nsize - 5;
    s_code = codebuf + 1;
}

static void
check_size_label(size_t desired_size)
{
    if (e_lab + (desired_size) < l_lab)
        return;

    size_t nsize = l_lab - s_lab + 400 + desired_size;
    size_t label_len = e_lab - s_lab;
    labbuf = realloc(labbuf, nsize);
    if (labbuf == NULL)
	err(1, NULL);
    e_lab = labbuf + label_len + 1;
    l_lab = labbuf + nsize - 5;
    s_lab = labbuf + 1;
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
search_brace(token_type *inout_type_code, int *inout_force_nl,
	     int *inout_comment_buffered, int *inout_last_else)
{
    while (ps.search_brace) {
	switch (*inout_type_code) {
	case newline:
	    if (sc_end == NULL) {
		save_com = sc_buf;
		save_com[0] = save_com[1] = ' ';
		sc_end = &save_com[2];
	    }
	    *sc_end++ = '\n';
	    /*
	     * We may have inherited a force_nl == true from the previous
	     * token (like a semicolon). But once we know that a newline
	     * has been scanned in this loop, force_nl should be false.
	     *
	     * However, the force_nl == true must be preserved if newline
	     * is never scanned in this loop, so this assignment cannot be
	     * done earlier.
	     */
	    *inout_force_nl = false;
	    break;
	case form_feed:
	    break;
	case comment:
	    if (sc_end == NULL) {
		/*
		 * Copy everything from the start of the line, because
		 * process_comment() will use that to calculate original
		 * indentation of a boxed comment.
		 */
		memcpy(sc_buf, in_buffer, (size_t)(buf_ptr - in_buffer) - 4);
		save_com = sc_buf + (buf_ptr - in_buffer - 4);
		save_com[0] = save_com[1] = ' ';
		sc_end = &save_com[2];
	    }
	    *inout_comment_buffered = true;
	    *sc_end++ = '/';	/* copy in start of comment */
	    *sc_end++ = '*';
	    for (;;) {		/* loop until the end of the comment */
		*sc_end = *buf_ptr++;
		if (buf_ptr >= buf_end)
		    fill_buffer();
		if (*sc_end++ == '*' && *buf_ptr == '/')
		    break;	/* we are at end of comment */
		if (sc_end >= &save_com[sc_size]) {	/* check for temp buffer
							 * overflow */
		    diag(1, "Internal buffer overflow - Move big comment from right after if, while, or whatever");
		    fflush(output);
		    exit(1);
		}
	    }
	    *sc_end++ = '/';	/* add ending slash */
	    if (++buf_ptr >= buf_end)	/* get past / in buffer */
		fill_buffer();
	    break;
	case lbrace:
	    /*
	     * Put KNF-style lbraces before the buffered up tokens and
	     * jump out of this loop in order to avoid copying the token
	     * again under the default case of the switch below.
	     */
	    if (sc_end != NULL && opt.btype_2) {
		save_com[0] = '{';
		/*
		 * Originally the lbrace may have been alone on its own
		 * line, but it will be moved into "the else's line", so
		 * if there was a newline resulting from the "{" before,
		 * it must be scanned now and ignored.
		 */
		while (isspace((unsigned char)*buf_ptr)) {
		    if (++buf_ptr >= buf_end)
			fill_buffer();
		    if (*buf_ptr == '\n')
			break;
		}
		goto sw_buffer;
	    }
	    /* FALLTHROUGH */
	default:		/* it is the start of a normal statement */
	{
	    int remove_newlines;

	    remove_newlines =
		    /* "} else" */
		    (*inout_type_code == keyword_do_else && *token == 'e' &&
		     e_code != s_code && e_code[-1] == '}')
		    /* "else if" */
		    || (*inout_type_code == keyword_for_if_while &&
			*token == 'i' && *inout_last_else && opt.else_if);
	    if (remove_newlines)
		*inout_force_nl = false;
	    if (sc_end == NULL) {	/* ignore buffering if
					 * comment wasn't saved up */
		ps.search_brace = false;
		return;
	    }
	    while (sc_end > save_com && isblank((unsigned char)sc_end[-1])) {
		sc_end--;
	    }
	    if (opt.swallow_optional_blanklines ||
		(!*inout_comment_buffered && remove_newlines)) {
		*inout_force_nl = !remove_newlines;
		while (sc_end > save_com && sc_end[-1] == '\n') {
		    sc_end--;
		}
	    }
	    if (*inout_force_nl) {	/* if we should insert a nl here, put
					 * it into the buffer */
		*inout_force_nl = false;
		--line_no;	/* this will be re-increased when the
				 * newline is read from the buffer */
		*sc_end++ = '\n';
		*sc_end++ = ' ';
		if (opt.verbose) /* print error msg if the line was
				 * not already broken */
		    diag(0, "Line broken");
	    }
	    for (const char *t_ptr = token; *t_ptr; ++t_ptr)
		*sc_end++ = *t_ptr;

	    sw_buffer:
	    ps.search_brace = false;	/* stop looking for start of stmt */
	    bp_save = buf_ptr;	/* save current input buffer */
	    be_save = buf_end;
	    buf_ptr = save_com;	/* fix so that subsequent calls to
				 * lexi will take tokens out of save_com */
	    *sc_end++ = ' ';	/* add trailing blank, just in case */
	    buf_end = sc_end;
	    sc_end = NULL;
	    debug_println("switched buf_ptr to save_com");
	    break;
	}
	}			/* end of switch */
	/*
	 * We must make this check, just in case there was an unexpected
	 * EOF.
	 */
	if (*inout_type_code != end_of_file) {
	    /*
	     * The only intended purpose of calling lexi() below is to
	     * categorize the next token in order to decide whether to
	     * continue buffering forthcoming tokens. Once the buffering
	     * is over, lexi() will be called again elsewhere on all of
	     * the tokens - this time for normal processing.
	     *
	     * Calling it for this purpose is a bug, because lexi() also
	     * changes the parser state and discards leading whitespace,
	     * which is needed mostly for comment-related considerations.
	     *
	     * Work around the former problem by giving lexi() a copy of
	     * the current parser state and discard it if the call turned
	     * out to be just a look ahead.
	     *
	     * Work around the latter problem by copying all whitespace
	     * characters into the buffer so that the later lexi() call
	     * will read them.
	     */
	    if (sc_end != NULL) {
		while (*buf_ptr == ' ' || *buf_ptr == '\t') {
		    *sc_end++ = *buf_ptr++;
		    if (sc_end >= &save_com[sc_size]) {
			errx(1, "input too long");
		    }
		}
		if (buf_ptr >= buf_end) {
		    fill_buffer();
		}
	    }

	    struct parser_state transient_state;
	    transient_state = ps;
	    *inout_type_code = lexi(&transient_state);	/* read another token */
	    if (*inout_type_code != newline && *inout_type_code != form_feed &&
		*inout_type_code != comment && !transient_state.search_brace) {
		ps = transient_state;
	    }
	}
    }

    *inout_last_else = 0;
}

static void
main_init_globals(void)
{
    found_err = 0;

    ps.p_stack[0] = stmt;	/* this is the parser's stack */
    ps.last_nl = true;		/* this is true if the last thing scanned was
				 * a newline */
    ps.last_token = semicolon;
    combuf = malloc(bufsize);
    if (combuf == NULL)
	err(1, NULL);
    labbuf = malloc(bufsize);
    if (labbuf == NULL)
	err(1, NULL);
    codebuf = malloc(bufsize);
    if (codebuf == NULL)
	err(1, NULL);
    tokenbuf = malloc(bufsize);
    if (tokenbuf == NULL)
	err(1, NULL);
    alloc_typenames();
    init_constant_tt();
    l_com = combuf + bufsize - 5;
    l_lab = labbuf + bufsize - 5;
    l_code = codebuf + bufsize - 5;
    l_token = tokenbuf + bufsize - 5;
    combuf[0] = codebuf[0] = labbuf[0] = ' ';	/* set up code, label, and
						 * comment buffers */
    combuf[1] = codebuf[1] = labbuf[1] = tokenbuf[1] = '\0';
    opt.else_if = 1;		/* Default else-if special processing to on */
    s_lab = e_lab = labbuf + 1;
    s_code = e_code = codebuf + 1;
    s_com = e_com = combuf + 1;
    s_token = e_token = tokenbuf + 1;

    in_buffer = malloc(10);
    if (in_buffer == NULL)
	err(1, NULL);
    in_buffer_limit = in_buffer + 8;
    buf_ptr = buf_end = in_buffer;
    line_no = 1;
    had_eof = ps.in_decl = ps.decl_on_line = break_comma = false;
    ps.in_or_st = false;
    ps.bl_line = true;
    ps.want_blank = ps.in_stmt = ps.ind_stmt = false;

    ps.pcase = false;
    sc_end = NULL;
    bp_save = NULL;
    be_save = NULL;

    output = NULL;

    const char *suffix = getenv("SIMPLE_BACKUP_SUFFIX");
    if (suffix != NULL)
	simple_backup_suffix = suffix;
}

static void
main_parse_command_line(int argc, char **argv)
{
    int i;
    const char *profile_name = NULL;

#if 0
    max_line_length = 78;	/* -l78 */
    lineup_to_parens = 1;	/* -lp */
    lineup_to_parens_always = 0; /* -nlpl */
    ps.ljust_decl = 0;		/* -ndj */
    ps.com_ind = 33;		/* -c33 */
    star_comment_cont = 1;	/* -sc */
    ps.ind_size = 8;		/* -i8 */
    verbose = 0;
    ps.decl_indent = 16;	/* -di16 */
    ps.local_decl_indent = -1;	/* if this is not set to some nonnegative value
				 * by an arg, we will set this equal to
				 * ps.decl_ind */
    ps.indent_parameters = 1;	/* -ip */
    ps.decl_com_ind = 0;	/* if this is not set to some positive value
				 * by an arg, we will set this equal to
				 * ps.com_ind */
    btype_2 = 1;		/* -br */
    cuddle_else = 1;		/* -ce */
    ps.unindent_displace = 0;	/* -d0 */
    ps.case_indent = 0;		/* -cli0 */
    format_block_comments = 1;	/* -fcb */
    format_col1_comments = 1;	/* -fc1 */
    procnames_start_line = 1;	/* -psl */
    proc_calls_space = 0;	/* -npcs */
    comment_delimiter_on_blankline = 1;	/* -cdb */
    ps.leave_comma = 1;		/* -nbc */
#endif

    for (i = 1; i < argc; ++i)
	if (strcmp(argv[i], "-npro") == 0)
	    break;
	else if (argv[i][0] == '-' && argv[i][1] == 'P' && argv[i][2] != '\0')
	    profile_name = argv[i];	/* non-empty -P (set profile) */
    set_defaults();
    if (i >= argc)
	set_profile(profile_name);

    for (i = 1; i < argc; ++i) {

	/*
	 * look thru args (if any) for changes to defaults
	 */
	if (argv[i][0] != '-') {/* no flag on parameter */
	    if (input == NULL) {	/* we must have the input file */
		in_name = argv[i];	/* remember name of input file */
		input = fopen(in_name, "r");
		if (input == NULL)	/* check for open error */
			err(1, "%s", in_name);
		continue;
	    } else if (output == NULL) {	/* we have the output file */
		out_name = argv[i];	/* remember name of output file */
		if (strcmp(in_name, out_name) == 0) {	/* attempt to overwrite
							 * the file */
		    errx(1, "input and output files must be different");
		}
		output = fopen(out_name, "w");
		if (output == NULL)	/* check for create error */
			err(1, "%s", out_name);
		continue;
	    }
	    errx(1, "unknown parameter: %s", argv[i]);
	} else
	    set_option(argv[i]);
    }				/* end of for */
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
    if (opt.local_decl_indent < 0) /* if not specified by user, set this */
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
    int col = 1;

    for (;;) {
	if (*p == ' ')
	    col++;
	else if (*p == '\t')
	    col = opt.tabsize * (1 + (col - 1) / opt.tabsize) + 1;
	else
	    break;
	p++;
    }
    if (col > opt.indent_size)
	ps.ind_level = ps.i_l_follow = col / opt.indent_size;
}

static void __attribute__((__noreturn__))
process_end_of_file(void)
{
    if (s_lab != e_lab || s_code != e_code || s_com != e_com)
	dump_line();

    if (ps.tos > 1)		/* check for balanced braces */
	diag(1, "Stuff missing from end of file");

    if (opt.verbose) {
	printf("There were %d output lines and %d comments\n",
	       ps.out_lines, ps.out_coms);
	printf("(Lines with comments)/(Lines with code): %6.3f\n",
	       (1.0 * ps.com_lines) / code_lines);
    }

    fflush(output);
    exit(found_err);
}

static void
process_comment_in_code(token_type type_code, int *inout_force_nl)
{
    if (*inout_force_nl &&
	type_code != semicolon &&
	(type_code != lbrace || !opt.btype_2)) {

	/* we should force a broken line here */
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();
	ps.want_blank = false;	/* dont insert blank at line start */
	*inout_force_nl = false;
    }

    ps.in_stmt = true;		/* turn on flag which causes an extra level of
				 * indentation. this is turned off by a ; or
				 * '}' */
    if (s_com != e_com) {	/* the turkey has embedded a comment
				 * in a line. fix it */
	size_t len = e_com - s_com;

	check_size_code(len + 3);
	*e_code++ = ' ';
	memcpy(e_code, s_com, len);
	e_code += len;
	*e_code++ = ' ';
	*e_code = '\0';		/* null terminate code sect */
	ps.want_blank = false;
	e_com = s_com;
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
    if (ps.last_token != comma || ps.p_l_follow > 0
	|| !opt.leave_comma || ps.block_init || !break_comma || s_com != e_com) {
	dump_line();
	ps.want_blank = false;
    }
    ++line_no;			/* keep track of input line number */
}

static void
process_lparen_or_lbracket(int dec_ind, int tabs_to_var, int sp_sw)
{
    /* count parens to make Healy happy */
    if (++ps.p_l_follow == nitems(ps.paren_indents)) {
	diag(0, "Reached internal limit of %zu unclosed parens",
	    nitems(ps.paren_indents));
	ps.p_l_follow--;
    }
    if (*token == '[')
	/* not a function pointer declaration or a function call */;
    else if (ps.in_decl && !ps.block_init && !ps.dumped_decl_indent &&
	ps.procname[0] == '\0' && ps.paren_level == 0) {
	/* function pointer declarations */
	indent_declaration(dec_ind, tabs_to_var);
	ps.dumped_decl_indent = true;
    } else if (ps.want_blank &&
	    ((ps.last_token != ident && ps.last_token != funcname) ||
	    opt.proc_calls_space ||
	    (ps.keyword == rw_sizeof ? opt.Bill_Shannon :
	    ps.keyword != rw_0 && ps.keyword != rw_offsetof)))
	*e_code++ = ' ';
    ps.want_blank = false;
    *e_code++ = token[0];

    ps.paren_indents[ps.p_l_follow - 1] =
	indentation_after_range(0, s_code, e_code);
    debug_println("paren_indent[%d] is now %d",
	ps.p_l_follow - 1, ps.paren_indents[ps.p_l_follow - 1]);

    if (sp_sw && ps.p_l_follow == 1 && opt.extra_expression_indent
	    && ps.paren_indents[0] < 2 * opt.indent_size) {
	ps.paren_indents[0] = 2 * opt.indent_size;
	debug_println("paren_indent[0] is now %d", ps.paren_indents[0]);
    }
    if (ps.in_or_st && *token == '(' && ps.tos <= 2) {
	/*
	 * this is a kluge to make sure that declarations will be
	 * aligned right if proc decl has an explicit type on it, i.e.
	 * "int a(x) {..."
	 */
	parse(semicolon);	/* I said this was a kluge... */
	ps.in_or_st = false;	/* turn off flag for structure decl or
				 * initialization */
    }
    /* parenthesized type following sizeof or offsetof is not a cast */
    if (ps.keyword == rw_offsetof || ps.keyword == rw_sizeof)
	ps.not_cast_mask |= 1 << ps.p_l_follow;
}

static void
process_rparen_or_rbracket(int *inout_sp_sw, int *inout_force_nl,
			 token_type hd_type)
{
    if (ps.cast_mask & (1 << ps.p_l_follow) & ~ps.not_cast_mask) {
	ps.last_u_d = true;
	ps.cast_mask &= (1 << ps.p_l_follow) - 1;
	ps.want_blank = opt.space_after_cast;
    } else
	ps.want_blank = true;
    ps.not_cast_mask &= (1 << ps.p_l_follow) - 1;

    if (--ps.p_l_follow < 0) {
	ps.p_l_follow = 0;
	diag(0, "Extra %c", *token);
    }

    if (e_code == s_code)	/* if the paren starts the line */
	ps.paren_level = ps.p_l_follow;	/* then indent it */

    *e_code++ = token[0];

    if (*inout_sp_sw && (ps.p_l_follow == 0)) {	/* check for end of if
				 * (...), or some such */
	*inout_sp_sw = false;
	*inout_force_nl = true;	/* must force newline after if */
	ps.last_u_d = true;	/* inform lexi that a following
				 * operator is unary */
	ps.in_stmt = false;	/* dont use stmt continuation indentation */

	parse(hd_type);		/* let parser worry about if, or whatever */
    }
    ps.search_brace = opt.btype_2; /* this should ensure that constructs such
				 * as main(){...} and int[]{...} have their
				 * braces put in the right place */
}

static void
process_unary_op(int dec_ind, int tabs_to_var)
{
    if (!ps.dumped_decl_indent && ps.in_decl && !ps.block_init &&
	ps.procname[0] == '\0' && ps.paren_level == 0) {
	/* pointer declarations */

	/*
	 * if this is a unary op in a declaration, we should indent
	 * this token
	 */
	int i;
	for (i = 0; token[i]; ++i)
	    /* find length of token */;
	indent_declaration(dec_ind - i, tabs_to_var);
	ps.dumped_decl_indent = true;
    } else if (ps.want_blank)
	*e_code++ = ' ';

    {
	size_t len = e_token - s_token;

	check_size_code(len);
	memcpy(e_code, token, len);
	e_code += len;
    }
    ps.want_blank = false;
}

static void
process_binary_op(void)
{
    size_t len = e_token - s_token;

    check_size_code(len + 1);
    if (ps.want_blank)
	*e_code++ = ' ';
    memcpy(e_code, token, len);
    e_code += len;

    ps.want_blank = true;
}

static void
process_postfix_op(void)
{
    *e_code++ = token[0];
    *e_code++ = token[1];
    ps.want_blank = true;
}

static void
process_question(int *inout_squest)
{
    (*inout_squest)++;		/* this will be used when a later colon
				 * appears so we can distinguish the
				 * <c>?<n>:<n> construct */
    if (ps.want_blank)
	*e_code++ = ' ';
    *e_code++ = '?';
    ps.want_blank = true;
}

static void
process_colon(int *inout_squest, int *inout_force_nl, int *inout_scase)
{
    if (*inout_squest > 0) {	/* it is part of the <c>?<n>: <n> construct */
	--*inout_squest;
	if (ps.want_blank)
	    *e_code++ = ' ';
	*e_code++ = ':';
	ps.want_blank = true;
	return;
    }
    if (ps.in_or_st) {
	*e_code++ = ':';
	ps.want_blank = false;
	return;
    }
    ps.in_stmt = false;		/* seeing a label does not imply we are in a
				 * stmt */
    /*
     * turn everything so far into a label
     */
    {
	size_t len = e_code - s_code;

	check_size_label(len + 3);
	memcpy(e_lab, s_code, len);
	e_lab += len;
	*e_lab++ = ':';
	*e_lab = '\0';
	e_code = s_code;
    }
    *inout_force_nl = ps.pcase = *inout_scase;	/* ps.pcase will be used by
						 * dump_line to decide how to
						 * indent the label. force_nl
						 * will force a case n: to be
						 * on a line by itself */
    *inout_scase = false;
    ps.want_blank = false;
}

static void
process_semicolon(int *inout_scase, int *inout_squest, int const dec_ind,
		  int const tabs_to_var, int *inout_sp_sw,
		  token_type const hd_type,
		  int *inout_force_nl)
{
    if (ps.dec_nest == 0)
	ps.in_or_st = false;	/* we are not in an initialization or
				 * structure declaration */
    *inout_scase = false; /* these will only need resetting in an error */
    *inout_squest = 0;
    if (ps.last_token == rparen)
	ps.in_parameter_declaration = 0;
    ps.cast_mask = 0;
    ps.not_cast_mask = 0;
    ps.block_init = 0;
    ps.block_init_level = 0;
    ps.just_saw_decl--;

    if (ps.in_decl && s_code == e_code && !ps.block_init &&
	!ps.dumped_decl_indent && ps.paren_level == 0) {
	/* indent stray semicolons in declarations */
	indent_declaration(dec_ind - 1, tabs_to_var);
	ps.dumped_decl_indent = true;
    }

    ps.in_decl = (ps.dec_nest > 0);	/* if we were in a first level
						 * structure declaration, we
						 * arent any more */

    if ((!*inout_sp_sw || hd_type != for_exprs) && ps.p_l_follow > 0) {

	/*
	 * This should be true iff there were unbalanced parens in the
	 * stmt.  It is a bit complicated, because the semicolon might
	 * be in a for stmt
	 */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	if (*inout_sp_sw) {	/* this is a check for an if, while, etc. with
				 * unbalanced parens */
	    *inout_sp_sw = false;
	    parse(hd_type);	/* dont lose the if, or whatever */
	}
    }
    *e_code++ = ';';
    ps.want_blank = true;
    ps.in_stmt = (ps.p_l_follow > 0);	/* we are no longer in the
				 * middle of a stmt */

    if (!*inout_sp_sw) {	/* if not if for (;;) */
	parse(semicolon);	/* let parser know about end of stmt */
	*inout_force_nl = true;/* force newline after an end of stmt */
    }
}

static void
process_lbrace(int *inout_force_nl, int *inout_sp_sw, token_type hd_type,
	       int *di_stack, int di_stack_cap, int *inout_dec_ind)
{
    ps.in_stmt = false;	/* dont indent the {} */
    if (!ps.block_init)
	*inout_force_nl = true;	/* force other stuff on same line as '{' onto
				 * new line */
    else if (ps.block_init_level <= 0)
	ps.block_init_level = 1;
    else
	ps.block_init_level++;

    if (s_code != e_code && !ps.block_init) {
	if (!opt.btype_2) {
	    dump_line();
	    ps.want_blank = false;
	} else if (ps.in_parameter_declaration && !ps.in_or_st) {
	    ps.i_l_follow = 0;
	    if (opt.function_brace_split) { /* dump the line prior
				 * to the brace ... */
		dump_line();
		ps.want_blank = false;
	    } else		/* add a space between the decl and brace */
		ps.want_blank = true;
	}
    }
    if (ps.in_parameter_declaration)
	prefix_blankline_requested = 0;

    if (ps.p_l_follow > 0) {	/* check for preceding unbalanced
				 * parens */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	if (*inout_sp_sw) {	/* check for unclosed if, for, etc. */
	    *inout_sp_sw = false;
	    parse(hd_type);
	    ps.ind_level = ps.i_l_follow;
	}
    }
    if (s_code == e_code)
	ps.ind_stmt = false;	/* dont put extra indentation on line
				 * with '{' */
    if (ps.in_decl && ps.in_or_st) {	/* this is either a structure
				 * declaration or an init */
	di_stack[ps.dec_nest] = *inout_dec_ind;
	if (++ps.dec_nest == di_stack_cap) {
	    diag(0, "Reached internal limit of %d struct levels",
		 di_stack_cap);
	    ps.dec_nest--;
	}
	/* ?		dec_ind = 0; */
    } else {
	ps.decl_on_line = false;	/* we can't be in the middle of
						 * a declaration, so don't do
						 * special indentation of
						 * comments */
	if (opt.blanklines_after_declarations_at_proctop
	    && ps.in_parameter_declaration)
	    postfix_blankline_requested = 1;
	ps.in_parameter_declaration = 0;
	ps.in_decl = false;
    }
    *inout_dec_ind = 0;
    parse(lbrace);	/* let parser know about this */
    if (ps.want_blank)	/* put a blank before '{' if '{' is not at
				 * start of line */
	*e_code++ = ' ';
    ps.want_blank = false;
    *e_code++ = '{';
    ps.just_saw_decl = 0;
}

static void
process_rbrace(int *inout_sp_sw, int *inout_dec_ind, const int *di_stack)
{
    if (ps.p_stack[ps.tos] == decl && !ps.block_init)	/* semicolons can be
				 * omitted in declarations */
	parse(semicolon);
    if (ps.p_l_follow) {	/* check for unclosed if, for, else. */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	*inout_sp_sw = false;
    }
    ps.just_saw_decl = 0;
    ps.block_init_level--;
    if (s_code != e_code && !ps.block_init) {	/* '}' must be first on line */
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();
    }
    *e_code++ = '}';
    ps.want_blank = true;
    ps.in_stmt = ps.ind_stmt = false;
    if (ps.dec_nest > 0) { /* we are in multi-level structure declaration */
	*inout_dec_ind = di_stack[--ps.dec_nest];
	if (ps.dec_nest == 0 && !ps.in_parameter_declaration)
	    ps.just_saw_decl = 2;
	ps.in_decl = true;
    }
    prefix_blankline_requested = 0;
    parse(rbrace);		/* let parser know about this */
    ps.search_brace = opt.cuddle_else
		      && ps.p_stack[ps.tos] == if_expr_stmt
		      && ps.il[ps.tos] >= ps.ind_level;
    if (ps.tos <= 1 && opt.blanklines_after_procs && ps.dec_nest <= 0)
	postfix_blankline_requested = 1;
}

static void
process_keyword_do_else(int *inout_force_nl, int *inout_last_else)
{
    ps.in_stmt = false;
    if (*token == 'e') {
	if (e_code != s_code && (!opt.cuddle_else || e_code[-1] != '}')) {
	    if (opt.verbose)
		diag(0, "Line broken");
	    dump_line();	/* make sure this starts a line */
	    ps.want_blank = false;
	}
	*inout_force_nl = true;/* also, following stuff must go onto new line */
	*inout_last_else = 1;
	parse(keyword_else);
    } else {
	if (e_code != s_code) {	/* make sure this starts a line */
	    if (opt.verbose)
		diag(0, "Line broken");
	    dump_line();
	    ps.want_blank = false;
	}
	*inout_force_nl = true;/* also, following stuff must go onto new line */
	*inout_last_else = 0;
	parse(keyword_do);
    }
}

static void
process_decl(int *out_dec_ind, int *out_tabs_to_var)
{
    parse(decl);		/* let parser worry about indentation */
    if (ps.last_token == rparen && ps.tos <= 1) {
	if (s_code != e_code) {
	    dump_line();
	    ps.want_blank = 0;
	}
    }
    if (ps.in_parameter_declaration && opt.indent_parameters && ps.dec_nest == 0) {
	ps.ind_level = ps.i_l_follow = 1;
	ps.ind_stmt = 0;
    }
    ps.in_or_st = true;		/* this might be a structure or initialization
				 * declaration */
    ps.in_decl = ps.decl_on_line = ps.last_token != type_def;
    if ( /* !ps.in_or_st && */ ps.dec_nest <= 0)
	ps.just_saw_decl = 2;
    prefix_blankline_requested = 0;
    int i;
    for (i = 0; token[i++];);	/* get length of token */

    if (ps.ind_level == 0 || ps.dec_nest > 0) {
	/* global variable or struct member in local variable */
	*out_dec_ind = opt.decl_indent > 0 ? opt.decl_indent : i;
	*out_tabs_to_var = (opt.use_tabs ? opt.decl_indent > 0 : 0);
    } else {
	/* local variable */
	*out_dec_ind = opt.local_decl_indent > 0 ? opt.local_decl_indent : i;
	*out_tabs_to_var = (opt.use_tabs ? opt.local_decl_indent > 0 : 0);
    }
}

static void
process_ident(token_type type_code, int dec_ind, int tabs_to_var,
	      int *inout_sp_sw, int *inout_force_nl, token_type hd_type)
{
    if (ps.in_decl) {
	if (type_code == funcname) {
	    ps.in_decl = false;
	    if (opt.procnames_start_line && s_code != e_code) {
		*e_code = '\0';
		dump_line();
	    } else if (ps.want_blank) {
		*e_code++ = ' ';
	    }
	    ps.want_blank = false;
	} else if (!ps.block_init && !ps.dumped_decl_indent &&
		   ps.paren_level == 0) { /* if we are in a declaration, we
					    * must indent identifier */
	    indent_declaration(dec_ind, tabs_to_var);
	    ps.dumped_decl_indent = true;
	    ps.want_blank = false;
	}
    } else if (*inout_sp_sw && ps.p_l_follow == 0) {
	*inout_sp_sw = false;
	*inout_force_nl = true;
	ps.last_u_d = true;
	ps.in_stmt = false;
	parse(hd_type);
    }
}

static void
copy_id(void)
{
    size_t len = e_token - s_token;

    check_size_code(len + 1);
    if (ps.want_blank)
	*e_code++ = ' ';
    memcpy(e_code, s_token, len);
    e_code += len;
}

static void
process_string_prefix(void)
{
    size_t len = e_token - s_token;

    check_size_code(len + 1);
    if (ps.want_blank)
	*e_code++ = ' ';
    memcpy(e_code, token, len);
    e_code += len;

    ps.want_blank = false;
}

static void
process_period(void)
{
    *e_code++ = '.';		/* move the period into line */
    ps.want_blank = false;	/* dont put a blank after a period */
}

static void
process_comma(int dec_ind, int tabs_to_var, int *inout_force_nl)
{
    ps.want_blank = (s_code != e_code);	/* only put blank after comma
				 * if comma does not start the line */
    if (ps.in_decl && ps.procname[0] == '\0' && !ps.block_init &&
	!ps.dumped_decl_indent && ps.paren_level == 0) {
	/* indent leading commas and not the actual identifiers */
	indent_declaration(dec_ind - 1, tabs_to_var);
	ps.dumped_decl_indent = true;
    }
    *e_code++ = ',';
    if (ps.p_l_follow == 0) {
	if (ps.block_init_level <= 0)
	    ps.block_init = 0;
	if (break_comma && (!opt.leave_comma ||
			    indentation_after_range(
				    compute_code_indent(), s_code, e_code)
			    >= opt.max_line_length - opt.tabsize))
	    *inout_force_nl = true;
    }
}

static void
process_preprocessing(void)
{
    if (s_com != e_com || s_lab != e_lab || s_code != e_code)
	dump_line();
    check_size_label(1);
    *e_lab++ = '#';	/* move whole line to 'label' buffer */

    {
	int         in_comment = 0;
	int         com_start = 0;
	char        quote = '\0';
	int         com_end = 0;

	while (*buf_ptr == ' ' || *buf_ptr == '\t') {
	    buf_ptr++;
	    if (buf_ptr >= buf_end)
		fill_buffer();
	}
	while (*buf_ptr != '\n' || (in_comment && !had_eof)) {
	    check_size_label(2);
	    *e_lab = *buf_ptr++;
	    if (buf_ptr >= buf_end)
		fill_buffer();
	    switch (*e_lab++) {
	    case '\\':
		if (!in_comment) {
		    *e_lab++ = *buf_ptr++;
		    if (buf_ptr >= buf_end)
			fill_buffer();
		}
		break;
	    case '/':
		if (*buf_ptr == '*' && !in_comment && quote == '\0') {
		    in_comment = 1;
		    *e_lab++ = *buf_ptr++;
		    com_start = (int)(e_lab - s_lab) - 2;
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
		    in_comment = 0;
		    *e_lab++ = *buf_ptr++;
		    com_end = (int)(e_lab - s_lab);
		}
		break;
	    }
	}

	while (e_lab > s_lab && (e_lab[-1] == ' ' || e_lab[-1] == '\t'))
	    e_lab--;
	if (e_lab - s_lab == com_end && bp_save == NULL) {
	    /* comment on preprocessor line */
	    if (sc_end == NULL) {	/* if this is the first comment,
						 * we must set up the buffer */
		save_com = sc_buf;
		sc_end = &save_com[0];
	    } else {
		*sc_end++ = '\n';	/* add newline between
						 * comments */
		*sc_end++ = ' ';
		--line_no;
	    }
	    if (sc_end - save_com + com_end - com_start > sc_size)
		errx(1, "input too long");
	    memmove(sc_end, s_lab + com_start, (size_t)(com_end - com_start));
	    sc_end += com_end - com_start;
	    e_lab = s_lab + com_start;
	    while (e_lab > s_lab && (e_lab[-1] == ' ' || e_lab[-1] == '\t'))
		e_lab--;
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
	*e_lab = '\0';	/* null terminate line */
	ps.pcase = false;
    }

    if (strncmp(s_lab, "#if", 3) == 0) { /* also ifdef, ifndef */
	if ((size_t)ifdef_level < nitems(state_stack)) {
	    match_state[ifdef_level].tos = -1;
	    state_stack[ifdef_level++] = ps;
	} else
	    diag(1, "#if stack overflow");
    } else if (strncmp(s_lab, "#el", 3) == 0) { /* else, elif */
	if (ifdef_level <= 0)
	    diag(1, s_lab[3] == 'i' ? "Unmatched #elif" : "Unmatched #else");
	else {
	    match_state[ifdef_level - 1] = ps;
	    ps = state_stack[ifdef_level - 1];
	}
    } else if (strncmp(s_lab, "#endif", 6) == 0) {
	if (ifdef_level <= 0)
	    diag(1, "Unmatched #endif");
	else
	    ifdef_level--;
    } else {
	if (strncmp(s_lab + 1, "pragma", 6) != 0 &&
	    strncmp(s_lab + 1, "error", 5) != 0 &&
	    strncmp(s_lab + 1, "line", 4) != 0 &&
	    strncmp(s_lab + 1, "undef", 5) != 0 &&
	    strncmp(s_lab + 1, "define", 6) != 0 &&
	    strncmp(s_lab + 1, "include", 7) != 0) {
	    diag(1, "Unrecognized cpp directive");
	    return;
	}
    }
    if (opt.blanklines_around_conditional_compilation) {
	postfix_blankline_requested++;
	n_real_blanklines = 0;
    } else {
	postfix_blankline_requested = 0;
	prefix_blankline_requested = 0;
    }

    /*
     * subsequent processing of the newline character will cause the line to
     * be printed
     */
}

static void __attribute__((__noreturn__))
main_loop(void)
{
    token_type type_code;
    int force_nl;		/* when true, code must be broken */
    int last_else = false;	/* true iff last keyword was an else */
    int         dec_ind;	/* current indentation for declarations */
    int         di_stack[20];	/* a stack of structure indentation levels */
    int		tabs_to_var;	/* true if using tabs to indent to var name */
    int         sp_sw;		/* when true, we are in the expression of
				 * if(...), while(...), etc. */
    token_type  hd_type = end_of_file; /* used to store type of stmt
				 * for if (...), for (...), etc */
    int         squest;		/* when this is positive, we have seen a ?
				 * without the matching : in a <c>?<s>:<s>
				 * construct */
    int         scase;		/* set to true when we see a case, so we will
				 * know what to do with the following colon */

    sp_sw = force_nl = false;
    dec_ind = 0;
    di_stack[ps.dec_nest = 0] = 0;
    scase = false;
    squest = 0;
    tabs_to_var = 0;

    for (;;) {			/* this is the main loop.  it will go until we
				 * reach eof */
	int comment_buffered = false;

	type_code = lexi(&ps);	/* lexi reads one token.  The actual
				 * characters read are stored in "token". lexi
				 * returns a code indicating the type of token */

	/*
	 * The following code moves newlines and comments following an if (),
	 * while (), else, etc. up to the start of the following stmt to
	 * a buffer. This allows proper handling of both kinds of brace
	 * placement (-br, -bl) and cuddling "else" (-ce).
	 */
	search_brace(&type_code, &force_nl, &comment_buffered, &last_else);

	if (type_code == end_of_file) {
	    process_end_of_file();
	    /* NOTREACHED */
	}

	if (
		type_code != comment &&
		type_code != newline &&
		type_code != preprocessing &&
		type_code != form_feed) {
	    process_comment_in_code(type_code, &force_nl);

	} else if (type_code != comment) /* preserve force_nl thru a comment */
	    force_nl = false;	/* cancel forced newline after newline, form
				 * feed, etc */



	/*-----------------------------------------------------*\
	|	   do switch on type of token scanned		|
	\*-----------------------------------------------------*/
	check_size_code(3);	/* maximum number of increments of e_code
				 * before the next check_size_code or
				 * dump_line() is 2. After that there's the
				 * final increment for the null character. */
	switch (type_code) {	/* now, decide what to do with the token */

	case form_feed:		/* found a form feed in line */
	    process_form_feed();
	    break;

	case newline:
	    process_newline();
	    break;

	case lparen:		/* got a '(' or '[' */
	    process_lparen_or_lbracket(dec_ind, tabs_to_var, sp_sw);
	    break;

	case rparen:		/* got a ')' or ']' */
	    process_rparen_or_rbracket(&sp_sw, &force_nl, hd_type);
	    break;

	case unary_op:		/* this could be any unary operation */
	    process_unary_op(dec_ind, tabs_to_var);
	    break;

	case binary_op:		/* any binary operation */
	    process_binary_op();
	    break;

	case postfix_op:	/* got a trailing ++ or -- */
	    process_postfix_op();
	    break;

	case question:		/* got a ? */
	    process_question(&squest);
	    break;

	case case_label:	/* got word 'case' or 'default' */
	    scase = true;	/* so we can process the later colon properly */
	    goto copy_id;

	case colon:		/* got a ':' */
	    process_colon(&squest, &force_nl, &scase);
	    break;

	case semicolon:		/* got a ';' */
	    process_semicolon(&scase, &squest, dec_ind, tabs_to_var, &sp_sw,
		hd_type, &force_nl);
	    break;

	case lbrace:		/* got a '{' */
	    process_lbrace(&force_nl, &sp_sw, hd_type, di_stack,
		(int)nitems(di_stack), &dec_ind);
	    break;

	case rbrace:		/* got a '}' */
	    process_rbrace(&sp_sw, &dec_ind, di_stack);
	    break;

	case switch_expr:	/* got keyword "switch" */
	    sp_sw = true;
	    hd_type = switch_expr; /* keep this for when we have seen the
				 * expression */
	    goto copy_id;	/* go move the token into buffer */

	case keyword_for_if_while:
	    sp_sw = true;	/* the interesting stuff is done after the
				 * expression is scanned */
	    hd_type = (*token == 'i' ? if_expr :
		       (*token == 'w' ? while_expr : for_exprs));

	    /* remember the type of header for later use by parser */
	    goto copy_id;	/* copy the token into line */

	case keyword_do_else:
	    process_keyword_do_else(&force_nl, &last_else);
	    goto copy_id;	/* move the token into line */

	case type_def:
	case storage_class:
	    prefix_blankline_requested = 0;
	    goto copy_id;

	case keyword_struct_union_enum:
	    if (ps.p_l_follow > 0)
		goto copy_id;
	    /* FALLTHROUGH */
	case decl:		/* we have a declaration type (int, etc.) */
	    process_decl(&dec_ind, &tabs_to_var);
	    goto copy_id;

	case funcname:
	case ident:		/* got an identifier or constant */
	    process_ident(type_code, dec_ind, tabs_to_var, &sp_sw, &force_nl,
		hd_type);
    copy_id:
	    copy_id();
	    if (type_code != funcname)
		ps.want_blank = true;
	    break;

	case string_prefix:
	    process_string_prefix();
	    break;

	case period:
	    process_period();
	    break;

	case comma:
	    process_comma(dec_ind, tabs_to_var, &force_nl);
	    break;

	case preprocessing:	/* '#' */
	    process_preprocessing();
	    break;
	case comment:		/* the initial '/' '*' or '//' of a comment */
	    process_comment();
	    break;

	default:
	    break;
	}			/* end of big switch stmt */

	*e_code = '\0';		/* make sure code section is null terminated */
	if (type_code != comment &&
	    type_code != newline &&
	    type_code != preprocessing)
	    ps.last_token = type_code;
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
 * copy input file to backup file if in_name is /blah/blah/blah/file, then
 * backup file will be ".Bfile" then make the backup file the input and
 * original input file the output
 */
static void
bakcopy(void)
{
    ssize_t n;
    int bakchn;
    char buff[8 * 1024];
    const char *p;

    /* construct file name .Bfile */
    for (p = in_name; *p; p++);	/* skip to end of string */
    while (p > in_name && *p != '/')	/* find last '/' */
	p--;
    if (*p == '/')
	p++;
    sprintf(bakfile, "%s%s", p, simple_backup_suffix);

    /* copy in_name to backup file */
    bakchn = creat(bakfile, 0600);
    if (bakchn < 0)
	err(1, "%s", bakfile);
    while ((n = read(fileno(input), buff, sizeof(buff))) > 0)
	if (write(bakchn, buff, (size_t)n) != n)
	    err(1, "%s", bakfile);
    if (n < 0)
	err(1, "%s", in_name);
    close(bakchn);
    fclose(input);

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
indent_declaration(int cur_dec_ind, int tabs_to_var)
{
    int pos = (int)(e_code - s_code);
    char *startpos = e_code;

    /*
     * get the tab math right for indentations that are not multiples of tabsize
     */
    if ((ps.ind_level * opt.indent_size) % opt.tabsize != 0) {
	pos += (ps.ind_level * opt.indent_size) % opt.tabsize;
	cur_dec_ind += (ps.ind_level * opt.indent_size) % opt.tabsize;
    }
    if (tabs_to_var) {
	int tpos;

	check_size_code((size_t)(cur_dec_ind / opt.tabsize));
	while ((tpos = opt.tabsize * (1 + pos / opt.tabsize)) <= cur_dec_ind) {
	    *e_code++ = '\t';
	    pos = tpos;
	}
    }
    check_size_code((size_t)(cur_dec_ind - pos + 1));
    while (pos < cur_dec_ind) {
	*e_code++ = ' ';
	pos++;
    }
    if (e_code == startpos && ps.want_blank) {
	*e_code++ = ' ';
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
