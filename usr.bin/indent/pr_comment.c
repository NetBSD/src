/*	$NetBSD: pr_comment.c,v 1.139 2023/05/14 22:26:37 rillig Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pr_comment.c,v 1.139 2023/05/14 22:26:37 rillig Exp $");

#include <string.h>

#include "indent.h"

static void
com_add_char(char ch)
{
    if (1 >= com.limit - com.e)
	buf_expand(&com, 1);
    *com.e++ = ch;
}

static void
com_add_delim(void)
{
    if (!opt.star_comment_cont)
	return;
    const char *delim = " * ";
    buf_add_range(&com, delim, delim + 3);
}

static bool
fits_in_one_line(int com_ind, int max_line_length)
{
    for (const char *start = inp_p(), *p = start; *p != '\n'; p++) {
	if (p[0] == '*' && p[1] == '/') {
	    int len = ind_add(com_ind + 3, start, p);
	    len += ch_isblank(p[-1]) ? 2 : 3;
	    return len <= max_line_length;
	}
    }
    return false;
}

static void
analyze_comment(bool *p_may_wrap, bool *p_delim, int *p_line_length)
{
    bool may_wrap = true;
    bool delim = opt.comment_delimiter_on_blankline;
    int ind;
    int line_length = opt.max_line_length;

    if (ps.curr_col_1 && !opt.format_col1_comments) {
	may_wrap = false;
	delim = false;
	ind = 0;

    } else {
	if (inp_peek() == '-' || inp_peek() == '*' ||
		token.e[-1] == '/' ||
		(inp_peek() == '\n' && !opt.format_block_comments)) {
	    may_wrap = false;
	    delim = false;
	}

	if (com.e != com.s)
	    output_line();
	if (lab.s == lab.e && code.s == code.e) {
	    ind = (ps.ind_level - opt.unindent_displace) * opt.indent_size;
	    if (ind <= 0)
		ind = opt.format_col1_comments ? 0 : 1;
	    line_length = opt.block_comment_max_line_length;
	} else {
	    delim = false;

	    int target_ind = code.s != code.e
		? ind_add(compute_code_indent(), code.s, code.e)
		: ind_add(compute_label_indent(), lab.s, lab.e);

	    ind = ps.decl_on_line || ps.ind_level == 0
		? opt.decl_comment_column - 1 : opt.comment_column - 1;
	    if (ind <= target_ind)
		ind = next_tab(target_ind);
	    if (ind + 25 > line_length)
		line_length = ind + 25;
	}
    }

    ps.com_ind = ind;

    if (!may_wrap) {
	/*
	 * Find out how much indentation there was originally, because that
	 * much will have to be ignored by output_complete_line.
	 */
	ps.n_comment_delta = -ind_add(0, inp_line_start(), inp_p() - 2);
    } else {
	ps.n_comment_delta = 0;
	if (!(inp_peek() == '\t' && !ch_isblank(inp_lookahead(1))))
	    while (ch_isblank(inp_peek()))
		inp_skip();
    }

    ps.comment_delta = 0;
    com_add_char('/');
    com_add_char(token.e[-1]);	/* either '*' or '/' */

    if (may_wrap && !ch_isblank(inp_peek()))
	com_add_char(' ');

    if (delim && fits_in_one_line(ind, line_length))
	delim = false;

    if (delim) {
	output_line();
	com_add_delim();
    }

    *p_line_length = line_length;
    *p_delim = delim;
    *p_may_wrap = may_wrap;
}

/*
 * Copy characters from 'inp' to 'com'. Try to keep comments from going over
 * the maximum line length. To do that, remember where the last blank, tab, or
 * newline was. When a line is filled, print up to the last blank and continue
 * copying.
 */
static void
copy_comment_wrap(int line_length, bool delim)
{
    ssize_t last_blank = -1;	/* index of the last blank in com.mem */

    for (;;) {
	switch (inp_peek()) {
	case '\f':
	    output_line_ff();
	    last_blank = -1;
	    com_add_delim();
	    inp_skip();
	    while (ch_isblank(inp_peek()))
		inp_skip();
	    break;

	case '\n':
	    if (had_eof) {
		diag(1, "Unterminated comment");
		output_line();
		return;
	    }

	    last_blank = -1;
	    if (ps.next_col_1) {
		if (com.s == com.e)
		    com_add_char(' ');	/* force empty line of output */
		if (com.e - com.s > 3) {
		    output_line();
		    com_add_delim();
		}
		output_line();
		com_add_delim();

	    } else {
		ps.next_col_1 = true;
		if (!(com.e > com.s && ch_isblank(com.e[-1])))
		    com_add_char(' ');
		last_blank = com.e - 1 - com.mem;
	    }
	    ++line_no;

	    bool skip_asterisk = true;
	    do {		/* flush any blanks and/or tabs at start of
				 * next line */
		inp_skip();
		if (inp_peek() == '*' && skip_asterisk) {
		    skip_asterisk = false;
		    inp_skip();
		    if (inp_peek() == '/')
			goto end_of_comment;
		}
	    } while (ch_isblank(inp_peek()));

	    break;		/* end of case for newline */

	case '*':
	    inp_skip();
	    if (inp_peek() == '/') {
	end_of_comment:
		inp_skip();

		if (delim) {
		    if (com.e - com.s > 3)
			output_line();
		    else
			com.e = com.s;
		    com_add_char(' ');
		}

		if (!(com.e > com.s && ch_isblank(com.e[-1])))
		    com_add_char(' ');
		com_add_char('*');
		com_add_char('/');
		return;

	    } else		/* handle isolated '*' */
		com_add_char('*');
	    break;

	default:
	    ;
	    int now_len = ind_add(ps.com_ind, com.s, com.e);
	    for (;;) {
		char ch = inp_next();
		if (ch_isblank(ch))
		    last_blank = com.e - com.mem;
		com_add_char(ch);
		now_len++;
		if (memchr("*\n\r\b\t", inp_peek(), 6) != NULL)
		    break;
		if (now_len >= line_length && last_blank != -1)
		    break;
	    }

	    ps.next_col_1 = false;

	    if (now_len <= line_length)
		break;
	    if (ch_isspace(com.e[-1]))
		break;

	    if (last_blank == -1) {	/* only a single word in this line */
		output_line();
		com_add_delim();
		break;
	    }

	    const char *last_word_s = com.mem + last_blank + 1;
	    size_t last_word_len = (size_t)(com.e - last_word_s);
	    com.e = com.mem + last_blank;
	    output_line();
	    com_add_delim();

	    memcpy(com.e, last_word_s, last_word_len);
	    com.e += last_word_len;
	    last_blank = -1;
	}
    }
}

static void
copy_comment_nowrap(void)
{
    for (;;) {
	if (inp_peek() == '\n') {
	    if (token.e[-1] == '/')
		return;

	    if (had_eof) {
		diag(1, "Unterminated comment");
		output_line();
		return;
	    }

	    if (com.s == com.e)
		com_add_char(' ');	/* force output of an empty line */
	    output_line();
	    ++line_no;
	    inp_skip();
	    continue;
	}

	com_add_char(inp_next());
	if (com.e[-2] == '*' && com.e[-1] == '/' && token.e[-1] == '*')
	    return;
    }
}

/*
 * Scan, reformat and output a single comment, which is either a block comment
 * starting with '/' '*' or an end-of-line comment starting with '//'.
 */
void
process_comment(void)
{
    int line_length;
    bool may_wrap, delim;

    analyze_comment(&may_wrap, &delim, &line_length);
    if (may_wrap)
	copy_comment_wrap(line_length, delim);
    else
	copy_comment_nowrap();
}
