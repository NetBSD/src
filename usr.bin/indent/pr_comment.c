/*	$NetBSD: pr_comment.c,v 1.58 2021/10/07 23:15:15 rillig Exp $	*/

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
__RCSID("$NetBSD: pr_comment.c,v 1.58 2021/10/07 23:15:15 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/pr_comment.c 334927 2018-06-10 16:44:18Z pstef $");
#endif

#include <stdio.h>
#include <string.h>

#include "indent.h"

static void
check_size_comment(size_t desired_size)
{
    if (com.e + desired_size >= com.l)
	buf_expand(&com, desired_size);
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
    char *t_ptr;		/* used for moving string */
    bool break_delim = opt.comment_delimiter_on_blankline;
    int l_just_saw_decl = ps.just_saw_decl;

    adj_max_line_length = opt.max_line_length;
    ps.just_saw_decl = 0;
    last_blank = -1;		/* no blanks found so far */
    ps.box_com = false;		/* at first, assume that we are not in a boxed
				 * comment or some other comment that should
				 * not be touched */
    ps.stats.comments++;

    /* Figure where to align and how to treat the comment */

    if (ps.col_1 && !opt.format_col1_comments) { /* if the comment starts in
				 * column 1, it should not be touched */
	ps.box_com = true;
	break_delim = false;
	ps.com_col = 1;

    } else {
	if (*inp.s == '-' || *inp.s == '*' || token.e[-1] == '/' ||
	    (*inp.s == '\n' && !opt.format_block_comments)) {
	    ps.box_com = true;	/* A comment with a '-' or '*' immediately
				 * after the /+* is assumed to be a boxed
				 * comment. A comment with a newline
				 * immediately after the /+* is assumed to be
				 * a block comment and is treated as a box
				 * comment unless format_block_comments is
				 * nonzero (the default). */
	    break_delim = false;
	}

	if (lab.s == lab.e && code.s == code.e) {
	    ps.com_col = (ps.ind_level - opt.unindent_displace) * opt.indent_size + 1;
	    adj_max_line_length = opt.block_comment_max_line_length;
	    if (ps.com_col <= 1)
		ps.com_col = 1 + (!opt.format_col1_comments ? 1 : 0);

	} else {
	    break_delim = false;

	    int target_col;
	    if (code.s != code.e)
		target_col = 1 + indentation_after(compute_code_indent(), code.s);
	    else if (lab.s != lab.e)
		target_col = 1 + indentation_after(compute_label_indent(), lab.s);
	    else
		target_col = 1;

	    ps.com_col = ps.decl_on_line || ps.ind_level == 0
		? opt.decl_comment_column : opt.comment_column;
	    if (ps.com_col <= target_col)
		ps.com_col = opt.tabsize * (1 + (target_col - 1) / opt.tabsize) + 1;
	    if (ps.com_col + 24 > adj_max_line_length)
		/* XXX: mismatch between column and length */
		adj_max_line_length = ps.com_col + 24;
	}
    }

    if (ps.box_com) {
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
    *com.e++ = '/';
    *com.e++ = token.e[-1];
    if (*inp.s != ' ' && !ps.box_com)
	*com.e++ = ' ';

    /* Don't put a break delimiter if this is a one-liner that won't wrap. */
    if (break_delim) {
	for (t_ptr = inp.s; *t_ptr != '\0' && *t_ptr != '\n'; t_ptr++) {
	    if (t_ptr >= inp.e)
		fill_buffer();
	    if (t_ptr[0] == '*' && t_ptr[1] == '/') {
		/*
		 * XXX: This computation ignores the leading " * ", as well as
		 * the trailing ' ' '*' '/'.  In simple cases, these cancel
		 * out since they are equally long.
		 */
		int right_margin = indentation_after_range(ps.com_col - 1,
		    inp.s, t_ptr + 2);
		if (right_margin < adj_max_line_length)
		    break_delim = false;
		break;
	    }
	}
    }

    if (break_delim) {
	char *t = com.e;
	com.e = com.s + 2;
	*com.e = '\0';
	if (opt.blanklines_before_blockcomments && ps.last_token != lbrace)
	    prefix_blankline_requested = true;
	dump_line();
	com.e = com.s = t;
	if (!ps.box_com && opt.star_comment_cont)
	    *com.e++ = ' ', *com.e++ = '*', *com.e++ = ' ';
    }

    /* Start to copy the comment */

    for (;;) {			/* this loop will go until the comment is
				 * copied */
	switch (*inp.s) {	/* this checks for various special cases */
	case '\f':
	    check_size_comment(3);
	    if (!ps.box_com) {	/* in a text comment, break the line here */
		ps.use_ff = true;
		dump_line();
		last_blank = -1;
		if (!ps.box_com && opt.star_comment_cont)
		    *com.e++ = ' ', *com.e++ = '*', *com.e++ = ' ';
		inp.s++;
		while (is_hspace(*inp.s))
		    inp.s++;
	    } else {
		inbuf_skip();
		*com.e++ = '\f';
	    }
	    break;

	case '\n':
	    if (token.e[-1] == '/') {
		++line_no;
		goto end_of_comment;
	    }

	    if (had_eof) {
		printf("Unterminated comment\n");
		dump_line();
		return;
	    }

	    last_blank = -1;
	    check_size_comment(4);
	    if (ps.box_com || ps.last_nl) {	/* if this is a boxed comment,
						 * we handle the newline */
		if (com.s == com.e)
		    *com.e++ = ' ';
		if (!ps.box_com && com.e - com.s > 3) {
		    dump_line();
		    if (opt.star_comment_cont)
			*com.e++ = ' ', *com.e++ = '*', *com.e++ = ' ';
		}
		dump_line();
		if (!ps.box_com && opt.star_comment_cont)
		    *com.e++ = ' ', *com.e++ = '*', *com.e++ = ' ';

	    } else {
		ps.last_nl = true;
		if (!is_hspace(com.e[-1]))
		    *com.e++ = ' ';
		last_blank = com.e - 1 - com.buf;
	    }
	    ++line_no;
	    if (!ps.box_com) {
		int asterisks_to_skip = 1;
		do {		/* flush any blanks and/or tabs at start of
				 * next line */
		    inbuf_skip();
		    if (*inp.s == '*' && --asterisks_to_skip >= 0) {
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
	    check_size_comment(4);
	    if (*inp.s == '/') {
	end_of_comment:
		inbuf_skip();

		if (break_delim) {
		    if (com.e > com.s + 3)
			dump_line();
		    else
			com.s = com.e;
		    *com.e++ = ' ';
		}

		if (!is_hspace(com.e[-1]) && !ps.box_com)
		    *com.e++ = ' ';	/* ensure blank before end */
		if (token.e[-1] == '/')
		    *com.e++ = '\n', *com.e = '\0';
		else
		    *com.e++ = '*', *com.e++ = '/', *com.e = '\0';

		ps.just_saw_decl = l_just_saw_decl;
		return;

	    } else		/* handle isolated '*' */
		*com.e++ = '*';
	    break;

	default:		/* we have a random char */
	    ;
	    int now_len = indentation_after_range(ps.com_col - 1, com.s, com.e);
	    do {
		check_size_comment(1);
		char ch = inbuf_next();
		if (is_hspace(ch))
		    last_blank = com.e - com.buf;
		*com.e++ = ch;
		now_len++;
	    } while (memchr("*\n\r\b\t", *inp.s, 6) == NULL &&
		(now_len < adj_max_line_length || last_blank == -1));

	    ps.last_nl = false;

	    /* XXX: signed character comparison '>' does not work for UTF-8 */
	    if (now_len > adj_max_line_length &&
		    !ps.box_com && com.e[-1] > ' ') {

		/* the comment is too long, it must be broken up */
		if (last_blank == -1) {
		    dump_line();
		    if (!ps.box_com && opt.star_comment_cont)
			*com.e++ = ' ', *com.e++ = '*', *com.e++ = ' ';
		    break;
		}

		*com.e = '\0';
		com.e = com.buf + last_blank;
		dump_line();

		if (!ps.box_com && opt.star_comment_cont)
		    *com.e++ = ' ', *com.e++ = '*', *com.e++ = ' ';

		for (t_ptr = com.buf + last_blank + 1;
		     is_hspace(*t_ptr); t_ptr++)
		    continue;
		last_blank = -1;

		/*
		 * t_ptr will be somewhere between com.e (dump_line() reset)
		 * and com.l. So it's safe to copy byte by byte from t_ptr to
		 * com.e without any check_size_comment().
		 */
		while (*t_ptr != '\0') {
		    if (is_hspace(*t_ptr))
			last_blank = com.e - com.buf;
		    *com.e++ = *t_ptr++;
		}
	    }
	    break;
	}
    }
}
