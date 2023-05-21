/*	$NetBSD: pr_comment.c,v 1.149 2023/05/21 10:18:44 rillig Exp $	*/

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
__RCSID("$NetBSD: pr_comment.c,v 1.149 2023/05/21 10:18:44 rillig Exp $");

#include <string.h>

#include "indent.h"

static void
com_add_char(char ch)
{
	buf_add_char(&com, ch);
}

static void
com_add_delim(void)
{
	if (!opt.star_comment_cont)
		return;
	buf_add_chars(&com, " * ", 3);
}

static bool
fits_in_one_line(int com_ind, int max_line_length)
{
	for (const char *start = inp.st, *p = start; *p != '\n'; p++) {
		if (p[0] == '*' && p[1] == '/') {
			int len = ind_add(com_ind + 3,
			    start, (size_t)(p - start));
			len += p == start || ch_isblank(p[-1]) ? 2 : 3;
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
		if (inp.st[0] == '-' || inp.st[0] == '*' ||
		    token.mem[token.len - 1] == '/' ||
		    (inp.st[0] == '\n' && !opt.format_block_comments)) {
			may_wrap = false;
			delim = false;
		}
		if (code.len == 0 && inp.st[strspn(inp.st, "*")] == '\n')
			out.line_kind = lk_block_comment;

		if (com.len > 0)
			output_line();
		if (lab.len == 0 && code.len == 0) {
			ind = (ps.ind_level - opt.unindent_displace)
			    * opt.indent_size;
			if (ind <= 0)
				ind = opt.format_col1_comments ? 0 : 1;
			line_length = opt.block_comment_max_line_length;
		} else {
			delim = false;

			int target_ind = code.len > 0
			    ? ind_add(compute_code_indent(), code.st, code.len)
			    : ind_add(compute_label_indent(), lab.st, lab.len);

			ind = ps.decl_on_line || ps.ind_level == 0
			    ? opt.decl_comment_column - 1
			    : opt.comment_column - 1;
			if (ind <= target_ind)
				ind = next_tab(target_ind);
			if (ind + 25 > line_length)
				line_length = ind + 25;
		}
	}

	ps.com_ind = ind;

	if (!may_wrap) {
		/* Find out how much indentation there was originally, because
		 * that much will have to be ignored by output_line. */
		size_t len = (size_t)(inp.st - 2 - inp.mem);
		ps.n_comment_delta = -ind_add(0, inp.mem, len);
	} else {
		ps.n_comment_delta = 0;
		if (!(inp.st[0] == '\t' && !ch_isblank(inp.st[1])))
			while (ch_isblank(inp.st[0]))
				inp.st++;
	}

	ps.comment_delta = 0;
	com_add_char('/');
	com_add_char(token.mem[token.len - 1]);	/* either '*' or '/' */

	if (may_wrap && !ch_isblank(inp.st[0]))
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
	ssize_t last_blank = -1;	/* index of the last blank in com.mem
					 */

	for (;;) {
		switch (inp.st[0]) {
		case '\n':
			if (had_eof) {
				diag(1, "Unterminated comment");
				output_line();
				return;
			}

			last_blank = -1;
			if (ps.next_col_1) {
				if (com.len == 0) {
					/* force empty line of output */
					com_add_char(' ');
				}
				if (com.len > 3) {
					output_line();
					com_add_delim();
				}
				output_line();
				com_add_delim();

			} else {
				ps.next_col_1 = true;
				if (!(com.len > 0
				    && ch_isblank(com.mem[com.len - 1])))
					com_add_char(' ');
				last_blank = (int)com.len - 1;
			}
			++line_no;

			bool skip_asterisk = true;
			do {	/* flush any blanks and/or tabs at start of
				 * next line */
				inp_skip();
				if (inp.st[0] == '*' && skip_asterisk) {
					skip_asterisk = false;
					inp.st++;
					if (inp.st[0] == '/')
						goto end_of_comment;
				}
			} while (ch_isblank(inp.st[0]));

			break;	/* end of case for newline */

		case '*':
			inp.st++;
			if (inp.st[0] == '/') {
		end_of_comment:
				inp.st++;

				if (delim) {
					if (com.len > 3)
						output_line();
					else
						com.len = 0;
					com_add_char(' ');
				} else {
					size_t len = com.len;
					while (ch_isblank(com.mem[len - 1]))
						len--;
					int now_len = ind_add(
					    ps.com_ind, com.st, len);
					if (now_len + 3 > line_length)
						output_line();
				}

				if (!(com.len > 0
				    && ch_isblank(com.mem[com.len - 1])))
					com_add_char(' ');
				com_add_char('*');
				com_add_char('/');
				return;

			} else	/* handle isolated '*' */
				com_add_char('*');
			break;

		default:
			;
			int now_len = ind_add(ps.com_ind, com.st, com.len);
			for (;;) {
				char ch = inp_next();
				if (ch_isblank(ch))
					last_blank = (ssize_t)com.len;
				com_add_char(ch);
				now_len++;
				if (memchr("*\n\r\b\t", inp.st[0], 6) != NULL)
					break;
				if (now_len >= line_length && last_blank != -1)
					break;
			}

			ps.next_col_1 = false;

			if (now_len <= line_length)
				break;
			if (ch_isspace(com.mem[com.len - 1]))
				break;

			if (last_blank == -1) {
				/* only a single word in this line */
				output_line();
				com_add_delim();
				break;
			}

			const char *last_word_s = com.mem + last_blank + 1;
			size_t last_word_len = com.len
			- (size_t)(last_blank + 1);
			com.len = (size_t)last_blank;
			output_line();
			com_add_delim();

			/* Assume that output_line and com_add_delim don't
			 * invalidate the "unused" part of the buffer beyond
			 * com.mem + com.len. */
			memmove(com.mem + com.len, last_word_s, last_word_len);
			com.len += last_word_len;
			last_blank = -1;
		}
	}
}

static void
copy_comment_nowrap(void)
{
	for (;;) {
		if (inp.st[0] == '\n') {
			if (token.mem[token.len - 1] == '/')
				return;

			if (had_eof) {
				diag(1, "Unterminated comment");
				output_line();
				return;
			}

			if (com.len == 0)
				com_add_char(' ');	/* force output of an
							 * empty line */
			output_line();
			++line_no;
			inp_skip();
			continue;
		}

		com_add_char(*inp.st++);
		if (com.mem[com.len - 2] == '*' && com.mem[com.len - 1] == '/'
		    && token.mem[token.len - 1] == '*')
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
