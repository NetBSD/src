/*	$NetBSD: io.c,v 1.73 2021/10/05 21:05:12 rillig Exp $	*/

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
__RCSID("$NetBSD: io.c,v 1.73 2021/10/05 21:05:12 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/io.c 334927 2018-06-10 16:44:18Z pstef $");
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "indent.h"

static bool comment_open;
static int paren_indent;
static int suppress_blanklines;

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

/*
 * dump_line is the routine that actually effects the printing of the new
 * source. It prints the label section, followed by the code section with
 * the appropriate nesting level, followed by any comments.
 */
void
dump_line(void)
{
    int cur_col;
    static bool not_first_line;

    if (ps.procname[0] != '\0') {
	ps.ind_level = 0;
	ps.procname[0] = '\0';
    }

    if (code.s == code.e && lab.s == lab.e && com.s == com.e) {
	if (suppress_blanklines > 0)
	    suppress_blanklines--;
	else
	    next_blank_lines++;
    } else if (!inhibit_formatting) {
	suppress_blanklines = 0;
	if (prefix_blankline_requested && not_first_line) {
	    if (opt.swallow_optional_blanklines) {
		if (next_blank_lines == 1)
		    next_blank_lines = 0;
	    } else {
		if (next_blank_lines == 0)
		    next_blank_lines = 1;
	    }
	}
	while (--next_blank_lines >= 0)
	    output_char('\n');
	next_blank_lines = 0;
	if (ps.ind_level == 0)
	    ps.ind_stmt = false;	/* this is a class A kludge. don't do
					 * additional statement indentation if
					 * we are at bracket level 0 */

	if (lab.e != lab.s || code.e != code.s)
	    ps.stats.code_lines++;


	if (lab.e != lab.s) {	/* print lab, if any */
	    if (comment_open) {
		comment_open = false;
		output_string(".*/\n");
	    }
	    while (lab.e > lab.s && is_hspace(lab.e[-1]))
		lab.e--;
	    *lab.e = '\0';
	    cur_col = 1 + output_indent(0, compute_label_indent());
	    if (lab.s[0] == '#' && (strncmp(lab.s, "#else", 5) == 0
		    || strncmp(lab.s, "#endif", 6) == 0)) {
		char *s = lab.s;
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
	    cur_col = 1 + indentation_after(cur_col - 1, lab.s);
	} else
	    cur_col = 1;	/* there is no label section */

	ps.pcase = false;

	if (code.s != code.e) {	/* print code section, if any */
	    if (comment_open) {
		comment_open = false;
		output_string(".*/\n");
	    }
	    int target_col = 1 + compute_code_indent();
	    {
		int i;

		for (i = 0; i < ps.p_l_follow; i++) {
		    if (ps.paren_indents[i] >= 0) {
			int ind = ps.paren_indents[i];
			/*
			 * XXX: this mix of 'indent' and 'column' smells like
			 * an off-by-one error.
			 */
			ps.paren_indents[i] = -(ind + target_col);
			debug_println(
			    "setting pi[%d] from %d to %d for column %d",
			    i, ind, ps.paren_indents[i], target_col);
		    }
		}
	    }
	    cur_col = 1 + output_indent(cur_col - 1, target_col - 1);
	    output_range(code.s, code.e);
	    cur_col = 1 + indentation_after(cur_col - 1, code.s);
	}
	if (com.s != com.e) {	/* print comment, if any */
	    int target_col = ps.com_col;
	    char *com_st = com.s;

	    target_col += ps.comment_delta;
	    while (*com_st == '\t')	/* consider original indentation in
					 * case this is a box comment */
		com_st++, target_col += opt.tabsize;
	    while (target_col <= 0)
		if (*com_st == ' ')
		    target_col++, com_st++;
		else if (*com_st == '\t') {
		    target_col = opt.tabsize * (1 + (target_col - 1) / opt.tabsize) + 1;
		    com_st++;
		} else
		    target_col = 1;
	    if (cur_col > target_col) {	/* if comment can't fit on this line,
					 * put it on next line */
		output_char('\n');
		cur_col = 1;
		ps.stats.lines++;
	    }
	    while (com.e > com_st && isspace((unsigned char)com.e[-1]))
		com.e--;
	    (void)output_indent(cur_col - 1, target_col - 1);
	    output_range(com_st, com.e);
	    ps.comment_delta = ps.n_comment_delta;
	    ps.stats.comment_lines++;
	}
	if (ps.use_ff)
	    output_char('\f');
	else
	    output_char('\n');
	ps.stats.lines++;
	if (ps.just_saw_decl == 1 && opt.blanklines_after_declarations) {
	    prefix_blankline_requested = true;
	    ps.just_saw_decl = 0;
	} else
	    prefix_blankline_requested = postfix_blankline_requested;
	postfix_blankline_requested = false;
    }

    /* keep blank lines after '//' comments */
    if (com.e - com.s > 1 && com.s[1] == '/'
	&& token.s < token.e && isspace((unsigned char)token.s[0]))
	output_range(token.s, token.e);

    ps.decl_on_line = ps.in_decl;	/* if we are in the middle of a
					 * declaration, remember that fact for
					 * proper comment indentation */
    ps.ind_stmt = ps.in_stmt && !ps.in_decl;	/* next line should be
						 * indented if we have not
						 * completed this stmt and if
						 * we are not in the middle of
						 * a declaration */
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
    not_first_line = true;
}

int
compute_code_indent(void)
{
    int target_ind = opt.indent_size * ps.ind_level;

    if (ps.paren_level != 0) {
	if (!opt.lineup_to_parens)
	    if (2 * opt.continuation_indent == opt.indent_size)
		target_ind += opt.continuation_indent;
	    else
		target_ind += opt.continuation_indent * ps.paren_level;
	else if (opt.lineup_to_parens_always)
	    /*
	     * XXX: where does this '- 1' come from?  It looks strange but is
	     * nevertheless needed for proper indentation, as demonstrated in
	     * the test opt-lpl.0.
	     */
	    target_ind = paren_indent - 1;
	else {
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
    if (ps.pcase)
	return (int)(case_ind * opt.indent_size);
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
    bool on_off;

    const char *p = in_buffer;

    skip_hspace(&p);
    if (!skip_string(&p, "/*"))
	return;
    skip_hspace(&p);
    if (!skip_string(&p, "INDENT"))
	return;
    skip_hspace(&p);
    if (*p == '*' || skip_string(&p, "ON"))
	on_off = true;
    else if (skip_string(&p, "OFF"))
	on_off = false;
    else
	return;

    skip_hspace(&p);
    if (!skip_string(&p, "*/\n"))
	return;

    if (com.s != com.e || lab.s != lab.e || code.s != code.e)
	dump_line();

    inhibit_formatting = !on_off;
    if (on_off) {
	next_blank_lines = 0;
	postfix_blankline_requested = false;
	prefix_blankline_requested = false;
	suppress_blanklines = 1;
    }
}

/*
 * Copyright (C) 1976 by the Board of Trustees of the University of Illinois
 *
 * All rights reserved
 *
 * FUNCTION: Reads one block of input into the input buffer
 */
void
fill_buffer(void)
{
    /* this routine reads stuff from the input */
    char *p;
    int ch;
    FILE *f = input;

    if (bp_save != NULL) {	/* there is a partly filled input buffer left */
	buf_ptr = bp_save;	/* do not read anything, just switch buffers */
	buf_end = be_save;
	bp_save = be_save = NULL;
	debug_println("switched buf_ptr back to bp_save");
	if (buf_ptr < buf_end)
	    return;		/* only return if there is really something in
				 * this buffer */
    }
    for (p = in_buffer;;) {
	if (p >= in_buffer_limit) {
	    size_t size = (in_buffer_limit - in_buffer) * 2 + 10;
	    size_t offset = p - in_buffer;
	    in_buffer = xrealloc(in_buffer, size);
	    p = in_buffer + offset;
	    in_buffer_limit = in_buffer + size - 2;
	}
	if ((ch = getc(f)) == EOF) {
	    *p++ = ' ';
	    *p++ = '\n';
	    had_eof = true;
	    break;
	}
	if (ch != '\0')
	    *p++ = (char)ch;
	if (ch == '\n')
	    break;
    }
    buf_ptr = in_buffer;
    buf_end = p;
    if (p - in_buffer > 2 && p[-2] == '/' && p[-3] == '*') {
	if (in_buffer[3] == 'I' && strncmp(in_buffer, "/**INDENT**", 11) == 0)
	    fill_buffer();	/* flush indent error message */
	else
	    parse_indent_comment();
    }
    if (inhibit_formatting) {
	p = in_buffer;
	do {
	    output_char(*p);
	} while (*p++ != '\n');
    }
}

int
indentation_after_range(int ind, const char *start, const char *end)
{
    for (const char *p = start; *p != '\0' && p != end; ++p) {
	if (*p == '\n' || *p == '\f')
	    ind = 0;
	else if (*p == '\t')
	    ind = opt.tabsize * (ind / opt.tabsize + 1);
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
