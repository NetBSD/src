/*	$NetBSD: io.c,v 1.146 2022/02/13 12:43:26 rillig Exp $	*/

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
static char sccsid[] = "@(#)io.c	8.1 (Berkeley) 6/6/93";
#endif

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__RCSID("$NetBSD: io.c,v 1.146 2022/02/13 12:43:26 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/io.c 334927 2018-06-10 16:44:18Z pstef $");
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indent.h"

/*
 * There are 3 modes for reading the input.
 *
 * default: In this mode, the input comes from the input file. The buffer
 * 'inp' contains the current line, terminated with '\n'. The current read
 * position is inp.s, and there is always inp.buf <= inp.s < inp.e. All other
 * pointers are null.
 *
 * copy-in: After reading 'if (expr)' or similar tokens, the input still comes
 * from 'inp', but instead of processing it, it is copied to 'save_com'. The
 * goal of this mode is to move the comments after the '{', that is to
 * transform 'if (expr) comment {' to 'if (expr) { comment'. When the next
 * token cannot be part of this transformation, switch to copy-out.
 *
 * copy-out: In this mode, the input comes from 'save_com', which contains the
 * tokens to be placed after the '{'. The input still comes from the range
 * [inp.s, inp.e), but these two members have been overwritten with pointers
 * into save_com_buf, so inp.buf and inp.s are unrelated, which is unusual.
 * In this mode, inp.e[-1] is usually not terminated with '\n'. After reading
 * all tokens from save_com, switch to default mode again.
 */
static struct {
    struct buffer inp;		/* one line of input, ready to be split into
				 * tokens; occasionally 's' and 'e' switch
				 * to save_com_buf */
    char save_com_buf[5000];	/* input text is saved here when looking for
				 * the brace after an if, while, etc */
    char *save_com_s;		/* start of the comment in save_com_buf, or
				 * null */
    char *save_com_e;		/* end of the comment in save_com_buf, or
				 * null */

    char *saved_inp_s;		/* saved value of inp.s when taking input from
				 * save_com, or null */
    char *saved_inp_e;		/* saved value of inp.e, or null */
} inbuf;

static int paren_indent;
static bool suppress_blanklines;


void
inp_init(void)
{
    inbuf.inp.buf = xmalloc(10);
    inbuf.inp.l = inbuf.inp.buf + 8;
    inbuf.inp.s = inbuf.inp.buf;
    inbuf.inp.e = inbuf.inp.buf;
}

const char *
inp_p(void)
{
    assert(inbuf.inp.s < inbuf.inp.e);
    return inbuf.inp.s;
}

const char *
inp_line_start(void)
{
    return inbuf.saved_inp_s != NULL ? inbuf.save_com_buf : inbuf.inp.buf;
}

const char *
inp_line_end(void)
{
    return inbuf.inp.e;
}

char
inp_peek(void)
{
    assert(inbuf.inp.s < inbuf.inp.e);
    return *inbuf.inp.s;
}

char
inp_lookahead(size_t i)
{
    assert(i < (size_t)(inbuf.inp.e - inbuf.inp.s));
    return inbuf.inp.s[i];
}

void
inp_skip(void)
{
    assert(inbuf.inp.s < inbuf.inp.e);
    inbuf.inp.s++;
    if (inbuf.inp.s >= inbuf.inp.e)
	inp_read_line();
}

char
inp_next(void)
{
    char ch = inp_peek();
    inp_skip();
    return ch;
}

#ifdef debug
static void
debug_inp_buf(const char *name, const char *s, const char *e)
{
    if (s != NULL && e != NULL) {
	debug_printf("    %-12s ", name);
	debug_vis_range("\"", s, e, "\"\n");
    }
}

void
debug_inp(const char *prefix)
{
    assert(inp_line_start() <= inbuf.inp.s);
    assert(inbuf.inp.s <= inbuf.inp.e);

    debug_println("%s %s:", __func__, prefix);
    if (inbuf.saved_inp_s == NULL)
	debug_inp_buf("inp.buf", inbuf.inp.buf, inbuf.inp.s);
    debug_inp_buf("inp", inbuf.inp.s, inbuf.inp.e);	/* never null */
    debug_inp_buf("save_com.buf", inbuf.save_com_buf, inbuf.save_com_s);
    debug_inp_buf("save_com", inbuf.save_com_s, inbuf.save_com_e);
    debug_inp_buf("saved_inp", inbuf.saved_inp_s, inbuf.saved_inp_e);
}
#endif

static void
inp_comment_check_size(size_t n)
{
    if ((size_t)(inbuf.save_com_e - inbuf.save_com_buf) + n <=
	array_length(inbuf.save_com_buf))
	return;

    diag(1, "Internal buffer overflow - "
	"Move big comment from right after if, while, or whatever");
    fflush(output);
    exit(1);
}

void
inp_comment_init_newline(void)
{
    if (inbuf.save_com_e != NULL)
	return;

    inbuf.save_com_s = inbuf.save_com_buf;
    inbuf.save_com_s[0] = ' ';	/* see inp_comment_insert_lbrace */
    inbuf.save_com_s[1] = ' ';	/* see inp_comment_insert_lbrace */
    inbuf.save_com_e = &inbuf.save_com_s[2];
    debug_inp(__func__);
}

void
inp_comment_init_comment(void)
{
    if (inbuf.save_com_e != NULL)
	return;

    /*
     * Copy everything from the start of the line, because process_comment()
     * will use that to calculate the original indentation of a boxed comment.
     */
    /*
     * TODO: Don't store anything in the memory range [input.inp.buf,
     * input.inp.s), as that data can easily get lost.
     */
    /*
     * FIXME: The '4' below is completely wrong. For example, in the snippet
     * 'if(expr)/''*comment', the 'r)' of the code is not copied. If there is
     * an additional line break before the ')', memcpy tries to copy
     * (size_t)-1 bytes.
     *
     * The original author of this magic number doesn't remember its purpose
     * anymore, so there is no point in keeping it. The existing tests must
     * still pass though.
     */
    assert((size_t)(inbuf.inp.s - inbuf.inp.buf) >= 4);
    size_t line_len = (size_t)(inbuf.inp.s - inbuf.inp.buf) - 4;
    assert(line_len < array_length(inbuf.save_com_buf));

    memcpy(inbuf.save_com_buf, inbuf.inp.buf, line_len);
    inbuf.save_com_s = inbuf.save_com_buf + line_len;

    inbuf.save_com_s[0] = ' ';	/* see inp_comment_insert_lbrace */
    inbuf.save_com_s[1] = ' ';	/* see inp_comment_insert_lbrace */
    inbuf.save_com_e = &inbuf.save_com_s[2];

    debug_vis_range("search_stmt_comment: before save_com is \"",
	inbuf.save_com_buf, inbuf.save_com_s, "\"\n");
    debug_vis_range("search_stmt_comment: save_com is \"",
	inbuf.save_com_s, inbuf.save_com_e, "\"\n");
}

void
inp_comment_init_preproc(void)
{
    if (inbuf.save_com_e == NULL) {	/* if this is the first comment, we
					 * must set up the buffer */
	/*
	 * XXX: No space is reserved for a potential '{' here, unlike in
	 * inp_comment_init_comment.
	 */
	inbuf.save_com_s = inbuf.save_com_buf;
	inbuf.save_com_e = inbuf.save_com_s;
    } else {
	inp_comment_add_char('\n');	/* add newline between comments */
	inp_comment_add_char(' ');
	--line_no;
    }
}

void
inp_comment_add_char(char ch)
{
    inp_comment_check_size(1);
    *inbuf.save_com_e++ = ch;
}

void
inp_comment_add_range(const char *s, const char *e)
{
    size_t len = (size_t)(e - s);
    inp_comment_check_size(len);
    memcpy(inbuf.save_com_e, s, len);
    inbuf.save_com_e += len;
}

bool
inp_comment_complete_block(void)
{
    return inbuf.save_com_e[-2] == '*' && inbuf.save_com_e[-1] == '/';
}

bool
inp_comment_seen(void)
{
    return inbuf.save_com_e != NULL;
}

void
inp_comment_rtrim_blank(void)
{
    while (inbuf.save_com_e > inbuf.save_com_s &&
	    ch_isblank(inbuf.save_com_e[-1]))
	inbuf.save_com_e--;
}

void
inp_comment_rtrim_newline(void)
{
    while (inbuf.save_com_e > inbuf.save_com_s &&
	    inbuf.save_com_e[-1] == '\n')
	inbuf.save_com_e--;
}

/*
 * Switch the input to come from save_com, replaying the copied tokens while
 * looking for the next '{'.
 */
void
inp_from_comment(void)
{
    debug_inp("before inp_from_comment");
    inbuf.saved_inp_s = inbuf.inp.s;
    inbuf.saved_inp_e = inbuf.inp.e;

    inbuf.inp.s = inbuf.save_com_s;
    inbuf.inp.e = inbuf.save_com_e;
    inbuf.save_com_s = NULL;
    inbuf.save_com_e = NULL;
    debug_inp("after inp_from_comment");
}

/*
 * After having read from save_com, continue with the rest of the input line
 * before reading the next line from the input file.
 */
static bool
inp_from_file(void)
{
    if (inbuf.saved_inp_s == NULL)
	return false;

    inbuf.inp.s = inbuf.saved_inp_s;
    inbuf.inp.e = inbuf.saved_inp_e;
    inbuf.saved_inp_s = inbuf.saved_inp_e = NULL;
    debug_println("switched inp.s back to saved_inp_s");
    return inbuf.inp.s < inbuf.inp.e;
}

void
inp_comment_insert_lbrace(void)
{
    assert(inbuf.save_com_s[0] == ' ');	/* see inp_comment_init_newline */
    inbuf.save_com_s[0] = '{';
}

static void
inp_add(char ch)
{
    if (inbuf.inp.e >= inbuf.inp.l) {
	size_t new_size = (size_t)(inbuf.inp.l - inbuf.inp.buf) * 2 + 10;
	size_t offset = (size_t)(inbuf.inp.e - inbuf.inp.buf);
	inbuf.inp.buf = xrealloc(inbuf.inp.buf, new_size);
	inbuf.inp.s = inbuf.inp.buf;
	inbuf.inp.e = inbuf.inp.buf + offset;
	inbuf.inp.l = inbuf.inp.buf + new_size - 2;
    }
    *inbuf.inp.e++ = ch;
}

static void
inp_read_next_line(FILE *f)
{
    inbuf.inp.s = inbuf.inp.buf;
    inbuf.inp.e = inbuf.inp.buf;

    for (;;) {
	int ch = getc(f);
	if (ch == EOF) {
	    if (!inhibit_formatting) {
		inp_add(' ');
		inp_add('\n');
	    }
	    had_eof = true;
	    break;
	}

	if (ch != '\0')
	    inp_add((char)ch);
	if (ch == '\n')
	    break;
    }
}

static void
output_char(char ch)
{
    fputc(ch, output);
    debug_vis_range("output_char '", &ch, &ch + 1, "'\n");
}

static void
output_range(const char *s, const char *e)
{
    fwrite(s, 1, (size_t)(e - s), output);
    debug_vis_range("output_range \"", s, e, "\"\n");
}

static int
output_indent(int old_ind, int new_ind)
{
    int ind = old_ind;

    if (opt.use_tabs) {
	int tabsize = opt.tabsize;
	int n = new_ind / tabsize - ind / tabsize;
	if (n > 0)
	    ind -= ind % tabsize;
	for (int i = 0; i < n; i++) {
	    fputc('\t', output);
	    ind += tabsize;
	}
    }

    for (; ind < new_ind; ind++)
	fputc(' ', output);

    debug_println("output_indent %d", ind);
    return ind;
}

static int
output_line_label(void)
{
    int ind;

    while (lab.e > lab.s && ch_isblank(lab.e[-1]))
	lab.e--;
    *lab.e = '\0';

    ind = output_indent(0, compute_label_indent());
    output_range(lab.s, lab.e);
    ind = ind_add(ind, lab.s, lab.e);

    ps.is_case_label = false;
    return ind;
}

static int
output_line_code(int ind)
{

    int target_ind = compute_code_indent();
    for (int i = 0; i < ps.nparen; i++) {
	if (ps.paren[i].indent >= 0) {
	    int paren_ind = ps.paren[i].indent;
	    ps.paren[i].indent = (short)(-1 - (paren_ind + target_ind));
	    debug_println(
		"setting paren_indents[%d] from %d to %d for column %d",
		i, paren_ind, ps.paren[i].indent, target_ind + 1);
	}
    }

    ind = output_indent(ind, target_ind);
    output_range(code.s, code.e);
    return ind_add(ind, code.s, code.e);
}

static void
output_line_comment(int ind)
{
    int target_ind = ps.com_ind;
    const char *p = com.s;

    target_ind += ps.comment_delta;

    /* consider original indentation in case this is a box comment */
    for (; *p == '\t'; p++)
	target_ind += opt.tabsize;

    for (; target_ind < 0; p++) {
	if (*p == ' ')
	    target_ind++;
	else if (*p == '\t')
	    target_ind = next_tab(target_ind);
	else {
	    target_ind = 0;
	    break;
	}
    }

    /* if comment can't fit on this line, put it on the next line */
    if (ind > target_ind) {
	output_char('\n');
	ind = 0;
	ps.stats.lines++;
    }

    while (com.e > p && ch_isspace(com.e[-1]))
	com.e--;

    (void)output_indent(ind, target_ind);
    output_range(p, com.e);

    ps.comment_delta = ps.n_comment_delta;
    ps.stats.comment_lines++;
}

/*
 * Write a line of formatted source to the output file. The line consists of
 * the label, the code and the comment.
 */
static void
output_complete_line(char line_terminator)
{
    static bool first_line = true;

    ps.is_function_definition = false;

    if (code.s == code.e && lab.s == lab.e && com.s == com.e) {
	if (suppress_blanklines)
	    suppress_blanklines = false;
	else
	    blank_lines_to_output++;

    } else if (!inhibit_formatting) {
	suppress_blanklines = false;
	if (blank_line_before && !first_line) {
	    if (opt.swallow_optional_blanklines) {
		if (blank_lines_to_output == 1)
		    blank_lines_to_output = 0;
	    } else {
		if (blank_lines_to_output == 0)
		    blank_lines_to_output = 1;
	    }
	}

	for (; blank_lines_to_output > 0; blank_lines_to_output--)
	    output_char('\n');

	if (ps.ind_level == 0)
	    ps.in_stmt_cont = false;	/* this is a class A kludge */

	if (lab.e != lab.s || code.e != code.s)
	    ps.stats.code_lines++;

	int ind = 0;
	if (lab.e != lab.s)
	    ind = output_line_label();
	if (code.e != code.s)
	    ind = output_line_code(ind);
	if (com.e != com.s)
	    output_line_comment(ind);

	output_char(line_terminator);
	ps.stats.lines++;

	/* TODO: rename to blank_line_after_decl */
	if (ps.just_saw_decl == 1 && opt.blanklines_after_decl) {
	    blank_line_before = true;
	    ps.just_saw_decl = 0;
	} else
	    blank_line_before = blank_line_after;
	blank_line_after = false;
    }

    ps.decl_on_line = ps.in_decl;	/* for proper comment indentation */
    ps.in_stmt_cont = ps.in_stmt_or_decl && !ps.in_decl;
    ps.decl_indent_done = false;

    *(lab.e = lab.s) = '\0';	/* reset buffers */
    *(code.e = code.s) = '\0';
    *(com.e = com.s = com.buf + 1) = '\0';

    ps.ind_level = ps.ind_level_follow;
    ps.line_start_nparen = ps.nparen;

    if (ps.line_start_nparen > 0) {
	/* TODO: explain what negative indentation means */
	paren_indent = -1 - ps.paren[ps.line_start_nparen - 1].indent;
	debug_println("paren_indent is now %d", paren_indent);
    }

    first_line = false;
}

void
output_line(void)
{
    output_complete_line('\n');
}

void
output_line_ff(void)
{
    output_complete_line('\f');
}

static int
compute_code_indent_lineup(int base_ind)
{
    int ti = paren_indent;
    int overflow = ind_add(ti, code.s, code.e) - opt.max_line_length;
    if (overflow < 0)
	return ti;

    if (ind_add(base_ind, code.s, code.e) < opt.max_line_length) {
	ti -= overflow + 2;
	if (ti > base_ind)
	    return ti;
	return base_ind;
    }

    return ti;
}

int
compute_code_indent(void)
{
    int base_ind = ps.ind_level * opt.indent_size;

    if (ps.line_start_nparen == 0) {
	if (ps.in_stmt_cont && ps.in_enum != in_enum_brace)
	    return base_ind + opt.continuation_indent;
	return base_ind;
    }

    if (opt.lineup_to_parens) {
	if (opt.lineup_to_parens_always)
	    return paren_indent;
	return compute_code_indent_lineup(base_ind);
    }

    if (2 * opt.continuation_indent == opt.indent_size)
	return base_ind + opt.continuation_indent;
    else
	return base_ind + opt.continuation_indent * ps.line_start_nparen;
}

int
compute_label_indent(void)
{
    if (ps.is_case_label)
	return (int)(case_ind * (float)opt.indent_size);
    if (lab.s[0] == '#')
	return 0;
    return opt.indent_size * (ps.ind_level - 2);
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
parse_indent_comment(void)
{
    bool on;

    const char *p = inbuf.inp.buf;

    skip_blank(&p);
    if (!skip_string(&p, "/*"))
	return;
    skip_blank(&p);
    if (!skip_string(&p, "INDENT"))
	return;

    skip_blank(&p);
    if (*p == '*' || skip_string(&p, "ON"))
	on = true;
    else if (skip_string(&p, "OFF"))
	on = false;
    else
	return;

    skip_blank(&p);
    if (!skip_string(&p, "*/\n"))
	return;

    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	output_line();

    inhibit_formatting = !on;
    if (on) {
	/*
	 * XXX: Does this make sense? Is the handling of blank lines above
	 * INDENT OFF comments essentially the same?
	 */
	blank_lines_to_output = 0;
	blank_line_after = false;
	blank_line_before = false;
	suppress_blanklines = true;
    }
}

void
inp_read_line(void)
{
    if (inp_from_file())
	return;

    inp_read_next_line(input);

    parse_indent_comment();

    if (inhibit_formatting)
	output_range(inbuf.inp.s, inbuf.inp.e);
}
