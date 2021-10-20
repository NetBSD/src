/*	$NetBSD: pr_comment.c,v 1.80 2021/10/20 05:14:21 rillig Exp $	*/

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
__RCSID("$NetBSD: pr_comment.c,v 1.80 2021/10/20 05:14:21 rillig Exp $");
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
    if (com.e + 1 >= com.l)
	buf_expand(&com, 1);
    *com.e++ = ch;
}

static void
com_add_delim(void)
{
    if (!opt.star_comment_cont)
	return;
    size_t len = 3;
    if (com.e + len >= com.l)
	buf_expand(&com, len);
    memcpy(com.e, " * ", len);
    com.e += len;
}

static void
com_terminate(void)
{
    if (com.e + 1 >= com.l)
	buf_expand(&com, 1);
    *com.e = '\0';
}

static bool
fits_in_one_line(int max_line_length)
{
    for (const char *p = inp.s; *p != '\n'; p++) {
	assert(*p != '\0');
	assert(inp.e - p >= 2);
	if (!(p[0] == '*' && p[1] == '/'))
	    continue;

	int len = indentation_after_range(ps.com_ind + 3, inp.s, p);
	len += is_hspace(p[-1]) ? 2 : 3;
	if (len <= max_line_length)
	    return true;
    }
    return false;
}

/*
 * Scan, reformat and output a single comment, which is either a block comment
 * starting with '/' '*' or an end-of-line comment starting with '//'.
 *
 * Try to keep comments from going over the maximum line length.  If a line is
 * too long, move everything starting from the last blank to the next comment
 * line.  Blanks and tabs from the beginning of the input line are removed.
 *
 * ALGORITHM:
 *	1) Decide where the comment should be aligned, and if lines should
 *	   be broken.
 *	2) If lines should not be broken and filled, just copy up to end of
 *	   comment.
 *	3) If lines should be filled, then scan through the input buffer,
 *	   copying characters to com_buf.  Remember where the last blank,
 *	   tab, or newline was.  When line is filled, print up to last blank
 *	   and continue copying.
 */
void
process_comment(void)
{
    int adj_max_line_length;	/* Adjusted max_line_length for comments that
				 * spill over the right margin */
    ssize_t last_blank;		/* index of the last blank in com.buf */
    bool break_delim = opt.comment_delimiter_on_blankline;
    int l_just_saw_decl = ps.just_saw_decl;
    int com_ind;

    adj_max_line_length = opt.max_line_length;
    ps.just_saw_decl = 0;
    last_blank = -1;		/* no blanks found so far */
    bool may_wrap = true;
    ps.stats.comments++;

    /* Figure where to align and how to treat the comment */

    if (ps.col_1 && !opt.format_col1_comments) {
	may_wrap = false;
	break_delim = false;
	com_ind = 0;

    } else {
	if (*inp.s == '-' || *inp.s == '*' || token.e[-1] == '/' ||
	    (*inp.s == '\n' && !opt.format_block_comments)) {
	    may_wrap = false;
	    break_delim = false;
	}

	if (lab.s == lab.e && code.s == code.e) {
	    com_ind = (ps.ind_level - opt.unindent_displace) * opt.indent_size;
	    adj_max_line_length = opt.block_comment_max_line_length;
	    if (com_ind <= 0)
		com_ind = opt.format_col1_comments ? 0 : 1;

	} else {
	    break_delim = false;

	    int target_ind;
	    if (code.s != code.e)
		target_ind = indentation_after(compute_code_indent(), code.s);
	    else if (lab.s != lab.e)
		target_ind = indentation_after(compute_label_indent(), lab.s);
	    else
		target_ind = 0;

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
	 * much will have to be ignored by dump_line(). This is a box comment,
	 * so nothing changes -- not even indentation.
	 *
	 * The comment we're about to read usually comes from inp.buf,
	 * unless it has been copied into save_com.
	 */
	const char *start;

	/*
	 * XXX: ordered comparison between pointers from different objects
	 * invokes undefined behavior (C99 6.5.8).
	 */
	start = inp.s >= save_com && inp.s < save_com + sc_size ?
	    sc_buf : inp.buf;
	ps.n_comment_delta = -indentation_after_range(0, start, inp.s - 2);
    } else {
	ps.n_comment_delta = 0;
	while (is_hspace(*inp.s))
	    inp.s++;
    }

    ps.comment_delta = 0;
    com_add_char('/');
    com_add_char(token.e[-1]);	/* either '*' or '/' */
    if (*inp.s != ' ' && may_wrap)
	com_add_char(' ');

    if (break_delim && fits_in_one_line(adj_max_line_length))
	break_delim = false;

    if (break_delim) {
	char *t = com.e;
	com.e = com.s + 2;
	*com.e = '\0';
	if (opt.blanklines_before_block_comments && ps.last_token != lbrace)
	    blank_line_before = true;
	dump_line();
	com.e = com.s = t;
	com_add_delim();
    }

    /* Start to copy the comment */

    for (;;) {			/* this loop will go until the comment is
				 * copied */
	switch (*inp.s) {	/* this checks for various special cases */
	case '\f':
	    if (may_wrap) {	/* in a text comment, break the line here */
		ps.use_ff = true;
		dump_line();
		last_blank = -1;
		com_add_delim();
		inp.s++;
		while (is_hspace(*inp.s))
		    inp.s++;
	    } else {
		inbuf_skip();
		com_add_char('\f');
	    }
	    break;

	case '\n':
	    if (token.e[-1] == '/')
		goto end_of_line_comment;

	    if (had_eof) {
		diag(1, "Unterminated comment");
		dump_line();
		return;
	    }

	    last_blank = -1;
	    if (!may_wrap || ps.last_nl) {	/* if this is a boxed comment,
						 * we handle the newline */
		if (com.s == com.e)
		    com_add_char(' ');
		if (may_wrap && com.e - com.s > 3) {
		    dump_line();
		    com_add_delim();
		}
		dump_line();
		if (may_wrap)
		    com_add_delim();

	    } else {
		ps.last_nl = true;
		if (!is_hspace(com.e[-1]))
		    com_add_char(' ');
		last_blank = com.e - 1 - com.buf;
	    }
	    ++line_no;
	    if (may_wrap) {
		bool skip_asterisk = true;
		do {		/* flush any blanks and/or tabs at start of
				 * next line */
		    inbuf_skip();
		    if (*inp.s == '*' && skip_asterisk) {
			skip_asterisk = false;
			inbuf_skip();
			if (*inp.s == '/')
			    goto end_of_comment;
		    }
		} while (is_hspace(*inp.s));
	    } else
		inbuf_skip();
	    break;		/* end of case for newline */

	case '*':
	    inbuf_skip();
	    if (*inp.s == '/') {
	end_of_comment:
		inbuf_skip();

	end_of_line_comment:
		if (break_delim) {
		    if (com.e > com.s + 3)
			dump_line();
		    else
			com.s = com.e;	/* XXX: why not e = s? */
		    com_add_char(' ');
		}

		if (!is_hspace(com.e[-1]) && may_wrap)
		    com_add_char(' ');
		if (token.e[-1] != '/') {
		    com_add_char('*');
		    com_add_char('/');
		}
		com_terminate();

		ps.just_saw_decl = l_just_saw_decl;
		return;

	    } else		/* handle isolated '*' */
		com_add_char('*');
	    break;

	default:		/* we have a random char */
	    ;
	    int now_len = indentation_after_range(ps.com_ind, com.s, com.e);
	    for (;;) {
		char ch = inbuf_next();
		if (is_hspace(ch))
		    last_blank = com.e - com.buf;
		com_add_char(ch);
		now_len++;
		if (memchr("*\n\r\b\t", *inp.s, 6) != NULL)
		    break;
		if (now_len >= adj_max_line_length && last_blank != -1)
		    break;
	    }

	    ps.last_nl = false;

	    /* XXX: signed character comparison '>' does not work for UTF-8 */
	    if (now_len > adj_max_line_length &&
		    may_wrap && com.e[-1] > ' ') {

		/* the comment is too long, it must be broken up */
		if (last_blank == -1) {
		    dump_line();
		    com_add_delim();
		    break;
		}

		com_terminate();	/* mark the end of the last word */
		com.e = com.buf + last_blank;
		dump_line();

		com_add_delim();

		const char *p = com.buf + last_blank + 1;
		while (is_hspace(*p))
		    p++;
		last_blank = -1;

		/*
		 * p still points to the last word from the previous line, in
		 * the same buffer that it is copied to, but to the right of
		 * the writing region [com.s, com.e). Calling dump_line only
		 * moved com.e back to com.s, it did not clear the contents of
		 * the buffer. This ensures that the buffer is already large
		 * enough.
		 */
		while (*p != '\0') {
		    assert(!is_hspace(*p));
		    *com.e++ = *p++;
		}
	    }
	    break;
	}
    }
}
