/*	$NetBSD: io.c,v 1.157 2023/05/13 12:31:02 rillig Exp $	*/

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
__RCSID("$NetBSD: io.c,v 1.157 2023/05/13 12:31:02 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/io.c 334927 2018-06-10 16:44:18Z pstef $");
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indent.h"

/*
 * The current line, ready to be split into tokens, terminated with '\n'. The
 * current read position is inp.s, and the invariant inp.s < inp.e holds.
 */
static struct buffer inp;

static int paren_indent;


void
inp_init(void)
{
    inp.mem = xmalloc(10);
    inp.limit = inp.mem + 8;
    inp.s = inp.mem;
    inp.e = inp.mem;
}

const char *
inp_p(void)
{
    assert(inp.s < inp.e);
    return inp.s;
}

const char *
inp_line_start(void)
{
    return inp.mem;
}

const char *
inp_line_end(void)
{
    return inp.e;
}

char
inp_peek(void)
{
    assert(inp.s < inp.e);
    return *inp.s;
}

char
inp_lookahead(size_t i)
{
    assert(i < (size_t)(inp.e - inp.s));
    return inp.s[i];
}

void
inp_skip(void)
{
    assert(inp.s < inp.e);
    inp.s++;
    if (inp.s >= inp.e)
	inp_read_line();
}

char
inp_next(void)
{
    char ch = inp_peek();
    inp_skip();
    return ch;
}

static void
inp_add(char ch)
{
    if (inp.e >= inp.limit) {
	size_t new_size = (size_t)(inp.limit - inp.mem) * 2 + 10;
	size_t e_offset = (size_t)(inp.e - inp.mem);
	inp.mem = xrealloc(inp.mem, new_size);
	inp.s = inp.mem;
	inp.e = inp.mem + e_offset;
	inp.limit = inp.mem + new_size - 2;
    }
    *inp.e++ = ch;
}

static void
inp_read_next_line(FILE *f)
{
    inp.s = inp.mem;
    inp.e = inp.mem;

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
    }

    while (com.e > p && ch_isspace(com.e[-1]))
	com.e--;

    (void)output_indent(ind, target_ind);
    output_range(p, com.e);

    ps.comment_delta = ps.n_comment_delta;
}

/*
 * Write a line of formatted source to the output file. The line consists of
 * the label, the code and the comment.
 *
 * Comments are written directly, bypassing this function.
 */
static void
output_complete_line(char line_terminator)
{
    debug_printf("%s", __func__);
    debug_buffers();
    debug_println("%s", line_terminator == '\f' ? " form_feed" : "");

    ps.is_function_definition = false;

    if (!inhibit_formatting) {
	if (ps.ind_level == 0)
	    ps.in_stmt_cont = false;	/* this is a class A kludge */

	int ind = 0;
	if (lab.e != lab.s)
	    ind = output_line_label();
	if (code.e != code.s)
	    ind = output_line_code(ind);
	if (com.e != com.s)
	    output_line_comment(ind);

	output_char(line_terminator);

	/* TODO: rename to blank_line_after_decl */
	if (ps.just_saw_decl == 1 && opt.blanklines_after_decl)
	    ps.just_saw_decl = 0;
    }

    ps.decl_on_line = ps.in_decl;	/* for proper comment indentation */
    ps.in_stmt_cont = ps.in_stmt_or_decl && !ps.in_decl;
    ps.decl_indent_done = false;

    *(lab.e = lab.s) = '\0';	/* reset buffers */
    *(code.e = code.s) = '\0';
    *(com.e = com.s = com.mem + 1) = '\0';

    ps.ind_level = ps.ind_level_follow;
    ps.line_start_nparen = ps.nparen;

    if (ps.nparen > 0) {
	/* TODO: explain what negative indentation means */
	paren_indent = -1 - ps.paren[ps.nparen - 1].indent;
	debug_println("paren_indent is now %d", paren_indent);
    }
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

    const char *p = inp.mem;

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
}

void
inp_read_line(void)
{
    inp_read_next_line(input);

    parse_indent_comment();

    if (inhibit_formatting)
	output_range(inp.s, inp.e);
}
