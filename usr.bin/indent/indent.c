/*	$NetBSD: indent.c,v 1.162 2021/10/26 21:37:27 rillig Exp $	*/

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
__RCSID("$NetBSD: indent.c,v 1.162 2021/10/26 21:37:27 rillig Exp $");
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

struct buffer lab;
struct buffer code;
struct buffer com;
struct buffer token;

struct buffer inp;

char sc_buf[sc_size];
char *save_com;
static char *sc_end;		/* pointer into save_com buffer */

char *saved_inp_s;
char *saved_inp_e;

bool found_err;
int blank_lines_to_output;
bool blank_line_before;
bool blank_line_after;
bool break_comma;
float case_ind;
bool had_eof;
int line_no;
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

static void
search_stmt_newline(bool *force_nl)
{
    if (sc_end == NULL) {
	save_com = sc_buf;
	save_com[0] = save_com[1] = ' ';
	sc_end = &save_com[2];
    }
    *sc_end++ = '\n';

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
search_stmt_comment(bool *comment_buffered)
{
    if (sc_end == NULL) {
	/*
	 * Copy everything from the start of the line, because
	 * process_comment() will use that to calculate original indentation
	 * of a boxed comment.
	 */
	memcpy(sc_buf, inp.buf, (size_t)(inp.s - inp.buf) - 4);
	save_com = sc_buf + (inp.s - inp.buf - 4);
	save_com[0] = save_com[1] = ' ';
	sc_end = &save_com[2];
    }

    *comment_buffered = true;
    *sc_end++ = '/';		/* copy in start of comment */
    *sc_end++ = '*';

    for (;;) {			/* loop until the end of the comment */
	*sc_end++ = inbuf_next();
	if (sc_end[-1] == '*' && *inp.s == '/')
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
search_stmt_lbrace(void)
{
    /*
     * Put KNF-style lbraces before the buffered up tokens and jump out of
     * this loop in order to avoid copying the token again.
     */
    if (sc_end != NULL && opt.brace_same_line) {
	save_com[0] = '{';
	/*
	 * Originally the lbrace may have been alone on its own line, but it
	 * will be moved into "the else's line", so if there was a newline
	 * resulting from the "{" before, it must be scanned now and ignored.
	 */
	while (isspace((unsigned char)*inp.s)) {
	    inbuf_skip();
	    if (*inp.s == '\n')
		break;
	}
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

    if (sc_end == NULL) {	/* ignore buffering if comment wasn't saved
				 * up */
	ps.search_stmt = false;
	return false;
    }

    while (sc_end > save_com && isblank((unsigned char)sc_end[-1]))
	sc_end--;

    if (opt.swallow_optional_blanklines ||
	(!comment_buffered && remove_newlines)) {
	*force_nl = !remove_newlines;
	while (sc_end > save_com && sc_end[-1] == '\n') {
	    sc_end--;
	}
    }

    if (*force_nl) {		/* if we should insert a nl here, put it into
				 * the buffer */
	*force_nl = false;
	--line_no;		/* this will be re-increased when the newline
				 * is read from the buffer */
	*sc_end++ = '\n';
	*sc_end++ = ' ';
	if (opt.verbose)	/* warn if the line was not already broken */
	    diag(0, "Line broken");
    }

    /* XXX: buffer overflow? This is essentially a strcpy. */
    for (const char *t_ptr = token.s; *t_ptr != '\0'; ++t_ptr)
	*sc_end++ = *t_ptr;
    return true;
}

static void
switch_buffer(void)
{
    ps.search_stmt = false;
    saved_inp_s = inp.s;	/* save current input buffer */
    saved_inp_e = inp.e;
    inp.s = save_com;		/* fix so that subsequent calls to lexi will
				 * take tokens out of save_com */
    *sc_end++ = ' ';		/* add trailing blank, just in case */
    inp.e = sc_end;
    sc_end = NULL;
    debug_println("switched inp.s to save_com");
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
    if (sc_end != NULL) {
	while (is_hspace(*inp.s)) {
	    *sc_end++ = *inp.s++;
	    if (sc_end >= &save_com[sc_size])
		errx(1, "input too long");
	}
	if (inp.s >= inp.e)
	    inbuf_read_line();
    }

    struct parser_state transient_state = ps;
    *lsym = lexi(&transient_state);	/* read another token */
    if (*lsym != lsym_newline && *lsym != lsym_form_feed &&
	*lsym != lsym_comment && !transient_state.search_stmt) {
	ps = transient_state;
    }
}

static void
search_stmt(lexer_symbol *lsym, bool *force_nl,
    bool *comment_buffered, bool *last_else)
{
    while (ps.search_stmt) {
	switch (*lsym) {
	case lsym_newline:
	    search_stmt_newline(force_nl);
	    break;
	case lsym_form_feed:
	    break;
	case lsym_comment:
	    search_stmt_comment(comment_buffered);
	    break;
	case lsym_lbrace:
	    if (search_stmt_lbrace())
		goto switch_buffer;
	    /* FALLTHROUGH */
	default:		/* it is the start of a normal statement */
	    if (!search_stmt_other(*lsym, force_nl,
		    *comment_buffered, *last_else))
		return;
    switch_buffer:
	    switch_buffer();
	}
	search_stmt_lookahead(lsym);
    }

    *last_else = false;
}

static void
buf_init(struct buffer *buf)
{
    size_t size = 200;
    buf->buf = xmalloc(size);
    buf->buf[0] = ' ';		/* allow accessing buf->e[-1] */
    buf->buf[1] = '\0';
    buf->s = buf->buf + 1;
    buf->e = buf->s;
    buf->l = buf->buf + size - 5;	/* safety margin */
}

static size_t
buf_len(const struct buffer *buf)
{
    return (size_t)(buf->e - buf->s);
}

void
buf_expand(struct buffer *buf, size_t desired_size)
{
    size_t nsize = (size_t)(buf->l - buf->s) + 400 + desired_size;
    size_t len = buf_len(buf);
    buf->buf = xrealloc(buf->buf, nsize);
    buf->e = buf->buf + len + 1;
    buf->l = buf->buf + nsize - 5;
    buf->s = buf->buf + 1;
}

static void
buf_reserve(struct buffer *buf, size_t n)
{
    if (buf->e + n >= buf->l)
	buf_expand(buf, n);
}

static void
buf_add_char(struct buffer *buf, char ch)
{
    buf_reserve(buf, 1);
    *buf->e++ = ch;
}

static void
buf_add_buf(struct buffer *buf, const struct buffer *add)
{
    size_t len = buf_len(add);
    buf_reserve(buf, len);
    memcpy(buf->e, add->s, len);
    buf->e += len;
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

static void
main_init_globals(void)
{
    found_err = false;

    ps.s_sym[0] = psym_stmt;
    ps.last_nl = true;
    ps.last_token = lsym_semicolon;
    buf_init(&com);
    buf_init(&lab);
    buf_init(&code);
    buf_init(&token);

    opt.else_if = true;		/* XXX: redundant? */

    inp.buf = xmalloc(10);
    inp.l = inp.buf + 8;
    inp.s = inp.e = inp.buf;
    line_no = 1;
    had_eof = ps.in_decl = ps.decl_on_line = break_comma = false;

    ps.init_or_struct = false;
    ps.want_blank = ps.in_stmt = ps.ind_stmt = false;
    ps.is_case_label = false;

    sc_end = NULL;
    saved_inp_s = NULL;
    saved_inp_e = NULL;

    output = NULL;

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
	    set_option(argv[i], "Command line");

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
    inbuf_read_line();

    parse(psym_semicolon);

    int ind = 0;
    for (const char *p = inp.s;; p++) {
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
indent_declaration(int cur_decl_ind, bool tabs_to_var)
{
    int ind = (int)buf_len(&code);
    char *orig_code_e = code.e;

    /*
     * get the tab math right for indentations that are not multiples of
     * tabsize
     */
    if ((ps.ind_level * opt.indent_size) % opt.tabsize != 0) {
	ind += (ps.ind_level * opt.indent_size) % opt.tabsize;
	cur_decl_ind += (ps.ind_level * opt.indent_size) % opt.tabsize;
    }

    if (tabs_to_var) {
	for (int next; (next = next_tab(ind)) <= cur_decl_ind; ind = next)
	    buf_add_char(&code, '\t');
    }

    for (; ind < cur_decl_ind; ind++)
	buf_add_char(&code, ' ');

    if (code.e == orig_code_e && ps.want_blank) {
	*code.e++ = ' ';
	ps.want_blank = false;
    }
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
process_comment_in_code(lexer_symbol lsym, bool *force_nl)
{
    if (*force_nl &&
	lsym != lsym_semicolon &&
	(lsym != lsym_lbrace || !opt.brace_same_line)) {

	/* we should force a broken line here */
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();
	ps.want_blank = false;	/* don't insert blank at line start */
	*force_nl = false;
    }

    /* add an extra level of indentation; turned off again by a ';' or '}' */
    ps.in_stmt = true;

    if (com.s != com.e) {	/* a comment embedded in a line */
	buf_add_char(&code, ' ');
	buf_add_buf(&code, &com);
	buf_add_char(&code, ' ');
	buf_terminate(&code);
	buf_reset(&com);
	ps.want_blank = false;
    }
}

static void
process_form_feed(void)
{
    dump_line_ff();
    ps.want_blank = false;
}

static void
process_newline(void)
{
    if (ps.last_token == lsym_comma && ps.p_l_follow == 0 && !ps.block_init &&
	!opt.break_after_comma && break_comma &&
	com.s == com.e)
	goto stay_in_line;

    dump_line();
    ps.want_blank = false;

stay_in_line:
    ++line_no;
}

static bool
want_blank_before_lparen(void)
{
    if (!ps.want_blank)
	return false;
    if (ps.last_token == lsym_rparen_or_rbracket)
	return false;
    if (ps.last_token != lsym_ident && ps.last_token != lsym_funcname)
	return true;
    if (opt.proc_calls_space)
	return true;
    if (ps.prev_keyword == kw_sizeof)
	return opt.blank_after_sizeof;
    return ps.prev_keyword != kw_0 && ps.prev_keyword != kw_offsetof;
}

static void
process_lparen_or_lbracket(int decl_ind, bool tabs_to_var, bool spaced_expr)
{
    if (++ps.p_l_follow == array_length(ps.paren_indents)) {
	diag(0, "Reached internal limit of %zu unclosed parens",
	    array_length(ps.paren_indents));
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
    debug_println("paren_indents[%d] is now %d",
	ps.p_l_follow - 1, ps.paren_indents[ps.p_l_follow - 1]);

    if (spaced_expr && ps.p_l_follow == 1 && opt.extra_expr_indent
	    && ps.paren_indents[0] < 2 * opt.indent_size) {
	ps.paren_indents[0] = (short)(2 * opt.indent_size);
	debug_println("paren_indents[0] is now %d", ps.paren_indents[0]);
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
    if (ps.prev_keyword == kw_offsetof || ps.prev_keyword == kw_sizeof)
	ps.not_cast_mask |= 1 << ps.p_l_follow;
}

static void
process_rparen_or_rbracket(bool *spaced_expr, bool *force_nl, stmt_head hd)
{
    if ((ps.cast_mask & (1 << ps.p_l_follow) & ~ps.not_cast_mask) != 0) {
	ps.next_unary = true;
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

    if (*spaced_expr && ps.p_l_follow == 0) {	/* check for end of 'if
						 * (...)', or some such */
	*spaced_expr = false;
	*force_nl = true;	/* must force newline after if */
	ps.next_unary = true;
	ps.in_stmt = false;	/* don't use stmt continuation indentation */

	parse_hd(hd);		/* let parser worry about if, or whatever */
    }

    /*
     * This should ensure that constructs such as main(){...} and int[]{...}
     * have their braces put in the right place.
     */
    ps.search_stmt = opt.brace_same_line;
}

static void
process_unary_op(int decl_ind, bool tabs_to_var)
{
    if (!ps.dumped_decl_indent && ps.in_decl && !ps.block_init &&
	ps.procname[0] == '\0' && ps.paren_level == 0) {
	/* pointer declarations */
	indent_declaration(decl_ind - (int)buf_len(&token), tabs_to_var);
	ps.dumped_decl_indent = true;
    } else if (ps.want_blank)
	*code.e++ = ' ';

    buf_add_buf(&code, &token);
    ps.want_blank = false;
}

static void
process_binary_op(void)
{
    if (ps.want_blank)
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

    ps.in_stmt = false;
    ps.is_case_label = *seen_case;
    *force_nl = *seen_case;
    *seen_case = false;
    ps.want_blank = false;
}

static void
process_semicolon(bool *seen_case, int *quest_level, int decl_ind,
    bool tabs_to_var, bool *spaced_expr, stmt_head hd, bool *force_nl)
{
    if (ps.decl_nest == 0)
	ps.init_or_struct = false;
    *seen_case = false;		/* these will only need resetting in an error */
    *quest_level = 0;
    if (ps.last_token == lsym_rparen_or_rbracket)
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

    ps.in_decl = ps.decl_nest > 0;	/* if we were in a first level
					 * structure declaration, we aren't
					 * anymore */

    if ((!*spaced_expr || hd != hd_for) && ps.p_l_follow > 0) {

	/*
	 * There were unbalanced parens in the statement. It is a bit
	 * complicated, because the semicolon might be in a for statement.
	 */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	if (*spaced_expr) {	/* 'if', 'while', etc. with unbalanced
				 * parentheses */
	    *spaced_expr = false;
	    parse_hd(hd);	/* don't lose the 'if', or whatever */
	}
    }
    *code.e++ = ';';
    ps.want_blank = true;
    ps.in_stmt = ps.p_l_follow > 0;	/* we are no longer in the middle of a
					 * stmt */

    if (!*spaced_expr) {	/* if not if for (;;) */
	parse(psym_semicolon);	/* let parser know about end of stmt */
	*force_nl = true;	/* force newline after an end of stmt */
    }
}

static void
process_lbrace(bool *force_nl, bool *spaced_expr, stmt_head hd,
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
	if (!opt.brace_same_line) {
	    dump_line();
	    ps.want_blank = false;
	} else if (ps.in_parameter_declaration && !ps.init_or_struct) {
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
	blank_line_before = false;

    if (ps.p_l_follow > 0) {	/* check for preceding unbalanced parens */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	if (*spaced_expr) {	/* check for unclosed 'if', 'for', etc. */
	    *spaced_expr = false;
	    parse_hd(hd);
	    ps.ind_level = ps.ind_level_follow;
	}
    }

    if (code.s == code.e)
	ps.ind_stmt = false;	/* don't indent the '{' itself */
    if (ps.in_decl && ps.init_or_struct) {
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
	if (opt.blanklines_after_decl_at_top && ps.in_parameter_declaration)
	    blank_line_after = true;
	ps.in_parameter_declaration = false;
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

    if (ps.p_l_follow != 0) {	/* check for unclosed if, for, else. */
	diag(1, "Unbalanced parens");
	ps.p_l_follow = 0;
	*spaced_expr = false;
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
	if (ps.decl_nest == 0 && !ps.in_parameter_declaration) {
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

    if (ps.tos <= 1 && opt.blanklines_after_procs && ps.decl_nest <= 0)
	blank_line_after = true;
}

static void
process_keyword_do(bool *force_nl, bool *last_else)
{
    ps.in_stmt = false;

    if (code.e != code.s) {	/* make sure this starts a line */
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();
	ps.want_blank = false;
    }

    *force_nl = true;		/* following stuff must go onto new line */
    *last_else = false;
    parse(psym_do);
}

static void
process_keyword_else(bool *force_nl, bool *last_else)
{
    ps.in_stmt = false;

    if (code.e != code.s && (!opt.cuddle_else || code.e[-1] != '}')) {
	if (opt.verbose)
	    diag(0, "Line broken");
	dump_line();		/* make sure this starts a line */
	ps.want_blank = false;
    }

    *force_nl = true;		/* following stuff must go onto new line */
    *last_else = true;
    parse(psym_else);
}

static void
process_decl(int *decl_ind, bool *tabs_to_var)
{
    parse(psym_decl);		/* let the parser worry about indentation */

    if (ps.last_token == lsym_rparen_or_rbracket && ps.tos <= 1) {
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

    ps.init_or_struct = /* maybe */ true;
    ps.in_decl = ps.decl_on_line = ps.last_token != lsym_typedef;
    if (ps.decl_nest <= 0)
	ps.just_saw_decl = 2;

    blank_line_before = false;

    int len = (int)buf_len(&token) + 1;
    int ind = ps.ind_level == 0 || ps.decl_nest > 0
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

    } else if (*spaced_expr && ps.p_l_follow == 0) {
	*spaced_expr = false;
	*force_nl = true;
	ps.next_unary = true;
	ps.in_stmt = false;
	parse_hd(hd);
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
process_string_prefix(void)
{
    copy_token();
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
    ps.want_blank = code.s != code.e;	/* only put blank after comma if comma
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
		indentation_after_range(compute_code_indent(), code.s, code.e)
		>= opt.max_line_length - opt.tabsize))
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

    while (is_hspace(*inp.s))
	inbuf_skip();

    while (*inp.s != '\n' || (state == COMM && !had_eof)) {
	buf_reserve(&lab, 2);
	*lab.e++ = inbuf_next();
	switch (lab.e[-1]) {
	case '\\':
	    if (state != COMM)
		*lab.e++ = inbuf_next();
	    break;
	case '/':
	    if (*inp.s == '*' && state == PLAIN) {
		state = COMM;
		*lab.e++ = *inp.s++;
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
	    if (*inp.s == '/' && state == COMM) {
		state = PLAIN;
		*lab.e++ = *inp.s++;
		com_end = (int)buf_len(&lab);
	    }
	    break;
	}
    }

    while (lab.e > lab.s && is_hspace(lab.e[-1]))
	lab.e--;
    if (lab.e - lab.s == com_end && saved_inp_s == NULL) {
	/* comment on preprocessor line */
	if (sc_end == NULL) {	/* if this is the first comment, we must set
				 * up the buffer */
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
	saved_inp_s = inp.s;	/* save current input buffer */
	saved_inp_e = inp.e;
	inp.s = save_com;	/* fix so that subsequent calls to lexi will
				 * take tokens out of save_com */
	*sc_end++ = ' ';	/* add trailing blank, just in case */
	inp.e = sc_end;
	sc_end = NULL;
	debug_println("switched inp.s to save_com");
    }
    buf_terminate(&lab);
}

static void
process_preprocessing(void)
{
    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	dump_line();

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

    di_stack[ps.decl_nest = 0] = 0;

    for (;;) {			/* this is the main loop.  it will go until we
				 * reach eof */
	bool comment_buffered = false;

	lexer_symbol lsym = lexi(&ps);	/* Read the next token.  The actual
					 * characters read are stored in
					 * "token". */

	/*
	 * Move newlines and comments following an if (), while (), else, etc.
	 * up to the start of the following stmt to a buffer. This allows
	 * proper handling of both kinds of brace placement (-br, -bl) and
	 * cuddling "else" (-ce).
	 */
	search_stmt(&lsym, &force_nl, &comment_buffered, &last_else);

	if (lsym == lsym_eof) {
	    process_end_of_file();
	    /* NOTREACHED */
	}

	if (lsym == lsym_newline || lsym == lsym_form_feed ||
		lsym == lsym_preprocessing)
	    force_nl = false;
	else if (lsym != lsym_comment)
	    process_comment_in_code(lsym, &force_nl);

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

	case lsym_case_label:	/* got word 'case' or 'default' */
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
				 * by parser */
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
	    process_keyword_do(&force_nl, &last_else);
	    goto copy_token;

	case lsym_else:
	    process_keyword_else(&force_nl, &last_else);
	    goto copy_token;

	case lsym_typedef:
	case lsym_storage_class:
	    blank_line_before = false;
	    goto copy_token;

	case lsym_tag:
	    if (ps.p_l_follow > 0)
		goto copy_token;
	    /* FALLTHROUGH */
	case lsym_type:
	    process_decl(&decl_ind, &tabs_to_var);
	    goto copy_token;

	case lsym_funcname:
	case lsym_ident:	/* an identifier, constant or string */
	    process_ident(lsym, decl_ind, tabs_to_var, &spaced_expr,
		&force_nl, hd);
    copy_token:
	    copy_token();
	    if (lsym != lsym_funcname)
		ps.want_blank = true;
	    break;

	case lsym_string_prefix:
	    process_string_prefix();
	    break;

	case lsym_period:
	    process_period();
	    break;

	case lsym_comma:
	    process_comma(decl_ind, tabs_to_var, &force_nl);
	    break;

	case lsym_preprocessing:	/* the initial '#' */
	    process_preprocessing();
	    break;

	case lsym_comment:	/* the initial '/' '*' or '//' of a comment */
	    process_comment();
	    break;

	default:
	    break;
	}

	*code.e = '\0';
	if (lsym != lsym_comment && lsym != lsym_newline &&
		lsym != lsym_preprocessing)
	    ps.last_token = lsym;
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
