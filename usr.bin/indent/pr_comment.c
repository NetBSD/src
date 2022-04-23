/*	$NetBSD: pr_comment.c,v 1.127 2022/04/23 06:43:22 rillig Exp $	*/

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
static char sccsid[] = "@(#)pr_comment.c	8.1 (Berkeley) 6/6/93";
#endif

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__RCSID("$NetBSD: pr_comment.c,v 1.127 2022/04/23 06:43:22 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/pr_comment.c 334927 2018-06-10 16:44:18Z pstef $");
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "indent.h"

static void
com_add_char(char ch)
{
    if (1 >= com.l - com.e)
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

static void
com_terminate(void)
{
    if (1 >= com.l - com.e)
	buf_expand(&com, 1);
    *com.e = '\0';
}

static bool
fits_in_one_line(int com_ind, int max_line_length)
{
    for (const char *p = inp_p(); *p != '\n'; p++) {
	assert(*p != '\0');
	assert(inp_line_end() - p >= 2);
	if (!(p[0] == '*' && p[1] == '/'))
	    continue;

	int len = ind_add(com_ind + 3, inp_p(), p);
	len += ch_isblank(p[-1]) ? 2 : 3;
	return len <= max_line_length;
    }
    return false;
}

static void
analyze_comment(bool *p_may_wrap, bool *p_break_delim,
    int *p_adj_max_line_length)
{
    int adj_max_line_length;	/* Adjusted max_line_length for comments that
				 * spill over the right margin */
    bool break_delim = opt.comment_delimiter_on_blankline;
    int com_ind;

    adj_max_line_length = opt.max_line_length;
    bool may_wrap = true;

    if (ps.curr_col_1 && !opt.format_col1_comments) {
	may_wrap = false;
	break_delim = false;
	com_ind = 0;

    } else {
	if (inp_peek() == '-' || inp_peek() == '*' ||
		token.e[-1] == '/' ||
		(inp_peek() == '\n' && !opt.format_block_comments)) {
	    may_wrap = false;
	    break_delim = false;
	}

	/*
	 * XXX: This condition looks suspicious since it ignores the case
	 * where the end of the previous comment is still in 'com'.
	 *
	 * See test token_comment.c, keyword 'analyze_comment'.
	 */
	if (lab.s == lab.e && code.s == code.e) {
	    adj_max_line_length = opt.block_comment_max_line_length;
	    com_ind = (ps.ind_level - opt.unindent_displace) * opt.indent_size;
	    if (com_ind <= 0)
		com_ind = opt.format_col1_comments ? 0 : 1;

	} else {
	    break_delim = false;

	    int target_ind = code.s != code.e
		? ind_add(compute_code_indent(), code.s, code.e)
		: ind_add(compute_label_indent(), lab.s, lab.e);

	    com_ind = ps.decl_on_line || ps.ind_level == 0
		? opt.decl_comment_column - 1 : opt.comment_column - 1;
	    if (com_ind <= target_ind)
		com_ind = next_tab(target_ind);
	    if (com_ind + 25 > adj_max_line_length)
		adj_max_line_length = com_ind + 25;
	}
    }

    ps.com_ind = com_ind;

    if (!may_wrap) {
	/*
	 * Find out how much indentation there was originally, because that
	 * much will have to be ignored by output_complete_line.
	 */
	const char *start = inp_line_start();
	ps.n_comment_delta = -ind_add(0, start, inp_p() - 2);
    } else {
	ps.n_comment_delta = 0;
	while (ch_isblank(inp_peek()))
	    inp_skip();
    }

    ps.comment_delta = 0;
    com_add_char('/');
    com_add_char(token.e[-1]);	/* either '*' or '/' */

    /* TODO: Maybe preserve a single '\t' as well. */
    if (inp_peek() != ' ' && may_wrap)
	com_add_char(' ');

    if (break_delim && fits_in_one_line(com_ind, adj_max_line_length))
	break_delim = false;

    if (break_delim) {
	if (opt.blanklines_before_block_comments &&
		ps.prev_token != lsym_lbrace)
	    out.blank_line_before = true;
	output_line();
	com_add_delim();
    }

    *p_adj_max_line_length = adj_max_line_length;
    *p_break_delim = break_delim;
    *p_may_wrap = may_wrap;
}

/*
 * Copy characters from 'inp' to 'com'. Try to keep comments from going over
 * the maximum line length. To do that, remember where the last blank, tab, or
 * newline was. When a line is filled, print up to the last blank and continue
 * copying.
 */
static void
copy_comment_wrap(int adj_max_line_length, bool break_delim)
{
    ssize_t last_blank = -1;	/* index of the last blank in com.buf */

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
		last_blank = com.e - 1 - com.buf;
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

		if (break_delim) {
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
		com_terminate();
		return;

	    } else		/* handle isolated '*' */
		com_add_char('*');
	    break;

	default:		/* we have a random char */
	    ;
	    int now_len = ind_add(ps.com_ind, com.s, com.e);
	    for (;;) {
		char ch = inp_next();
		if (ch_isblank(ch))
		    last_blank = com.e - com.buf;
		com_add_char(ch);
		now_len++;
		if (memchr("*\n\r\b\t", inp_peek(), 6) != NULL)
		    break;
		if (now_len >= adj_max_line_length && last_blank != -1)
		    break;
	    }

	    ps.next_col_1 = false;

	    if (now_len <= adj_max_line_length)
		break;
	    if (ch_isspace(com.e[-1]))
		break;

	    if (last_blank == -1) {	/* only a single word in this line */
		output_line();
		com_add_delim();
		break;
	    }

	    const char *last_word_s = com.buf + last_blank + 1;
	    size_t last_word_len = (size_t)(com.e - last_word_s);
	    com.e = com.buf + last_blank;
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
		goto finish;

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
	    goto finish;
    }

finish:
    com_terminate();
}

/*
 * Scan, reformat and output a single comment, which is either a block comment
 * starting with '/' '*' or an end-of-line comment starting with '//'.
 */
void
process_comment(void)
{
    int adj_max_line_length;
    bool may_wrap, break_delim;

    ps.just_saw_decl = 0;
    ps.stats.comments++;

    int saved_just_saw_decl = ps.just_saw_decl;
    analyze_comment(&may_wrap, &break_delim, &adj_max_line_length);
    if (may_wrap)
	copy_comment_wrap(adj_max_line_length, break_delim);
    else
	copy_comment_nowrap();
    ps.just_saw_decl = saved_just_saw_decl;
}
