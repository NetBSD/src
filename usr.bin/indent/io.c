/*	$NetBSD: io.c,v 1.99 2021/10/20 05:14:21 rillig Exp $	*/

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
__RCSID("$NetBSD: io.c,v 1.99 2021/10/20 05:14:21 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/io.c 334927 2018-06-10 16:44:18Z pstef $");
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "indent.h"

static int paren_indent;
static bool suppress_blanklines;

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

static inline void
output_string(const char *s)
{
    output_range(s, s + strlen(s));
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
dump_line_label(void)
{
    int ind;

    while (lab.e > lab.s && is_hspace(lab.e[-1]))
	lab.e--;
    *lab.e = '\0';

    ind = output_indent(0, compute_label_indent());

    if (lab.s[0] == '#' && (strncmp(lab.s, "#else", 5) == 0
	    || strncmp(lab.s, "#endif", 6) == 0)) {
	const char *s = lab.s;
	if (lab.e[-1] == '\n')
	    lab.e--;
	do {
	    output_char(*s++);
	} while (s < lab.e && 'a' <= *s && *s <= 'z');

	while (s < lab.e && is_hspace(*s))
	    s++;

	if (s < lab.e) {
	    if (s[0] == '/' && s[1] == '*') {
		output_char('\t');
		output_range(s, lab.e);
	    } else {
		output_string("\t/* ");
		output_range(s, lab.e);
		output_string(" */");
	    }
	}
    } else
	output_range(lab.s, lab.e);
    ind = indentation_after(ind, lab.s);

    ps.is_case_label = false;
    return ind;
}

static int
dump_line_code(int ind)
{

    int target_ind = compute_code_indent();
    for (int i = 0; i < ps.p_l_follow; i++) {
	if (ps.paren_indents[i] >= 0) {
	    int paren_ind = ps.paren_indents[i];
	    /* XXX: the '+ 1' smells like an off-by-one error. */
	    ps.paren_indents[i] = (short)-(paren_ind + target_ind + 1);
	    debug_println(
		"setting pi[%d] from %d to %d for column %d",
		i, paren_ind, ps.paren_indents[i], target_ind + 1);
	}
    }

    ind = output_indent(ind, target_ind);
    output_range(code.s, code.e);
    return indentation_after(ind, code.s);
}

static void
dump_line_comment(int ind)
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

    /* if comment can't fit on this line, put it on next line */
    if (ind > target_ind) {
	output_char('\n');
	ind = 0;
	ps.stats.lines++;
    }

    while (com.e > p && isspace((unsigned char)com.e[-1]))
	com.e--;

    (void)output_indent(ind, target_ind);
    output_range(p, com.e);

    ps.comment_delta = ps.n_comment_delta;
    ps.stats.comment_lines++;
}

/*
 * Write a line of formatted source to the output file. The line consists of a
 * label, the code and the comment.
 */
void
dump_line(void)
{
    static bool first_line = true;

    if (ps.procname[0] != '\0') {
	ps.ind_level = 0;
	ps.procname[0] = '\0';
    }

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
	    ps.ind_stmt = false;	/* this is a class A kludge. don't do
					 * additional statement indentation if
					 * we are at bracket level 0 */

	if (lab.e != lab.s || code.e != code.s)
	    ps.stats.code_lines++;

	int ind = 0;
	if (lab.e != lab.s)
	    ind = dump_line_label();
	if (code.e != code.s)
	    ind = dump_line_code(ind);
	if (com.e != com.s)
	    dump_line_comment(ind);

	if (ps.use_ff)
	    output_char('\f');
	else
	    output_char('\n');
	ps.stats.lines++;

	if (ps.just_saw_decl == 1 && opt.blanklines_after_decl) {
	    blank_line_before = true;
	    ps.just_saw_decl = 0;
	} else
	    blank_line_before = blank_line_after;
	blank_line_after = false;
    }

    ps.decl_on_line = ps.in_decl;	/* for proper comment indentation */
    ps.ind_stmt = ps.in_stmt && !ps.in_decl;
    ps.use_ff = false;
    ps.dumped_decl_indent = false;

    *(lab.e = lab.s) = '\0';	/* reset buffers */
    *(code.e = code.s) = '\0';
    *(com.e = com.s = com.buf + 1) = '\0';

    ps.ind_level = ps.ind_level_follow;
    ps.paren_level = ps.p_l_follow;

    if (ps.paren_level > 0) {
	/* TODO: explain what negative indentation means */
	paren_indent = -ps.paren_indents[ps.paren_level - 1];
	debug_println("paren_indent is now %d", paren_indent);
    }

    first_line = false;
}

int
compute_code_indent(void)
{
    int target_ind = opt.indent_size * ps.ind_level;

    if (ps.paren_level != 0) {
	if (!opt.lineup_to_parens) {
	    if (2 * opt.continuation_indent == opt.indent_size)
		target_ind += opt.continuation_indent;
	    else
		target_ind += opt.continuation_indent * ps.paren_level;

	} else if (opt.lineup_to_parens_always) {
	    /*
	     * XXX: where does this '- 1' come from?  It looks strange but is
	     * nevertheless needed for proper indentation, as demonstrated in
	     * the test opt-lpl.0.
	     */
	    target_ind = paren_indent - 1;

	} else {
	    int w;
	    int t = paren_indent;

	    if ((w = 1 + indentation_after(t - 1, code.s) - opt.max_line_length) > 0
		&& 1 + indentation_after(target_ind, code.s) <= opt.max_line_length) {
		t -= w + 1;
		if (t > target_ind + 1)
		    target_ind = t - 1;
	    } else
		target_ind = t - 1;
	}

    } else if (ps.ind_stmt)
	target_ind += opt.continuation_indent;

    return target_ind;
}

int
compute_label_indent(void)
{
    if (ps.is_case_label)
	return (int)(case_ind * (float)opt.indent_size);
    if (lab.s[0] == '#')
	return 0;
    return opt.indent_size * (ps.ind_level - label_offset);
}

static void
skip_hspace(const char **pp)
{
    while (is_hspace(**pp))
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

    const char *p = inp.buf;

    skip_hspace(&p);
    if (!skip_string(&p, "/*"))
	return;
    skip_hspace(&p);
    if (!skip_string(&p, "INDENT"))
	return;
    skip_hspace(&p);

    if (*p == '*' || skip_string(&p, "ON"))
	on = true;
    else if (skip_string(&p, "OFF"))
	on = false;
    else
	return;

    skip_hspace(&p);
    if (!skip_string(&p, "*/\n"))
	return;

    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	dump_line();

    inhibit_formatting = !on;
    if (on) {
	blank_lines_to_output = 0;
	blank_line_after = false;
	blank_line_before = false;
	suppress_blanklines = true;
    }
}

/*
 * Copyright (C) 1976 by the Board of Trustees of the University of Illinois
 *
 * All rights reserved
 */
void
inbuf_read_line(void)
{
    char *p;
    int ch;
    FILE *f = input;

    if (saved_inp_s != NULL) {	/* there is a partly filled input buffer left */
	inp.s = saved_inp_s;	/* do not read anything, just switch buffers */
	inp.e = saved_inp_e;
	saved_inp_s = saved_inp_e = NULL;
	debug_println("switched inp.s back to saved_inp_s");
	if (inp.s < inp.e)
	    return;		/* only return if there is really something in
				 * this buffer */
    }

    for (p = inp.buf;;) {
	if (p >= inp.l) {
	    size_t size = (size_t)(inp.l - inp.buf) * 2 + 10;
	    size_t offset = (size_t)(p - inp.buf);
	    inp.buf = xrealloc(inp.buf, size);
	    p = inp.buf + offset;
	    inp.l = inp.buf + size - 2;
	}

	if ((ch = getc(f)) == EOF) {
	    if (!inhibit_formatting) {
		*p++ = ' ';
		*p++ = '\n';
	    }
	    had_eof = true;
	    break;
	}

	if (ch != '\0')
	    *p++ = (char)ch;
	if (ch == '\n')
	    break;
    }

    inp.s = inp.buf;
    inp.e = p;

    if (p - inp.buf >= 3 && p[-3] == '*' && p[-2] == '/') {
	if (strncmp(inp.buf, "/**INDENT**", 11) == 0)
	    inbuf_read_line();	/* flush indent error message */
	else
	    parse_indent_comment();
    }

    if (inhibit_formatting)
	output_range(inp.s, inp.e);
}

int
indentation_after_range(int ind, const char *start, const char *end)
{
    for (const char *p = start; *p != '\0' && p != end; ++p) {
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

int
indentation_after(int ind, const char *s)
{
    return indentation_after_range(ind, s, NULL);
}

void
diag(int level, const char *msg, ...)
{
    va_list ap;
    const char *s, *e;

    if (level != 0)
	found_err = true;

    if (output == stdout) {
	s = "/**INDENT** ";
	e = " */";
    } else {
	s = e = "";
    }

    va_start(ap, msg);
    fprintf(stderr, "%s%s@%d: ", s, level == 0 ? "Warning" : "Error", line_no);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "%s\n", e);
    va_end(ap);
}
