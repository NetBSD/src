/*	$NetBSD: pr_comment.c,v 1.171 2023/06/23 20:59:04 rillig Exp $	*/

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
__RCSID("$NetBSD: pr_comment.c,v 1.171 2023/06/23 20:59:04 rillig Exp $");

#include <string.h>

#include "indent.h"

static void
com_add_char(char ch)
{
	buf_add_char(&com, ch);
}

static void
com_add_star(void)
{
	if (opt.star_comment_cont)
		buf_add_chars(&com, " * ", 3);
}

static bool
fits_in_one_line(int max_line_length)
{
	for (const char *start = inp_p, *p = start; *p != '\n'; p++) {
		if (p[0] == '*' && p[1] == '/') {
			while (p - inp_p >= 2
			    && ch_isblank(p[-1])
			    && ch_isblank(p[-2]))
				p--;
			int ind = ind_add(ps.comment_ind + 3,
			    start, (size_t)(p - start));
			ind += p == start || ch_isblank(p[-1]) ? 2 : 3;
			return ind <= max_line_length;
		}
	}
	return false;
}

static bool
is_block_comment(void)
{
	const char *p = inp_p;
	while (*p == '*')
		p++;
	return *p == '\n';
}

static void
analyze_comment(bool *p_may_wrap, bool *p_delim, int *p_line_length)
{
	bool may_wrap = true;
	bool delim = false;	// only relevant if may_wrap
	int ind;
	int line_length = opt.max_line_length;

	if (inp_p - inp.s == 2 && !opt.format_col1_comments) {
		may_wrap = false;
		ind = 0;
	} else {
		if (inp_p[0] == '-' || inp_p[0] == '*' ||
		    token.s[token.len - 1] == '/' ||
		    (inp_p[0] == '\n' && !opt.format_block_comments))
			may_wrap = false;

		if (com.len > 0)
			output_line();
		if (lab.len == 0 && code.len == 0) {
			if (is_block_comment())
				out.line_kind = lk_block_comment;
			ind = (ps.ind_level - opt.unindent_displace)
			    * opt.indent_size;
			if (ind <= 0)
				ind = opt.format_col1_comments ? 0 : 1;
			line_length = opt.block_comment_max_line_length;
			if (may_wrap && inp_p[0] == '\n')
				delim = true;
			if (may_wrap && opt.comment_delimiter_on_blank_line)
				delim = true;
		} else {
			int min_ind = code.len > 0
			    ? ind_add(compute_code_indent(), code.s, code.len)
			    : ind_add(compute_label_indent(), lab.s, lab.len);

			ind = ps.line_has_decl || ps.ind_level == 0
			    ? opt.decl_comment_column - 1
			    : opt.comment_column - 1;
			if (ind <= min_ind)
				ind = next_tab(min_ind);
			if (ind + 25 > line_length)
				line_length = ind + 25;
		}
	}

	if (!may_wrap) {
		/* Find out how much indentation there was originally, because
		 * that much will have to be ignored by output_line. */
		size_t len = (size_t)(inp_p - 2 - inp.s);
		ps.comment_shift = -ind_add(0, inp.s, len);
	} else {
		ps.comment_shift = 0;
		if (!(inp_p[0] == '\t' && !ch_isblank(inp_p[1])))
			while (ch_isblank(inp_p[0]))
				inp_p++;
	}

	ps.comment_ind = ind;
	*p_may_wrap = may_wrap;
	*p_delim = delim;
	*p_line_length = line_length;
}

static void
copy_comment_start(bool may_wrap, bool *delim, int line_length)
{
	ps.comment_cont = false;
	buf_add_chars(&com, token.s, token.len);	// "/*" or "//"

	if (may_wrap) {
		if (!ch_isblank(inp_p[0]))
			com_add_char(' ');

		if (*delim && fits_in_one_line(line_length))
			*delim = false;
		if (*delim) {
			output_line();
			com_add_star();
		}
	}
}

static void
copy_comment_wrap_text(int line_length, ssize_t *last_blank)
{
	int ind = ind_add(ps.comment_ind, com.s, com.len);
	for (;;) {
		char ch = inp_next();
		if (ch_isblank(ch))
			*last_blank = (ssize_t)com.len;
		com_add_char(ch);
		ind++;
		if (memchr("*\n\r\t", inp_p[0], 5) != NULL)
			break;
		if (ind >= line_length && *last_blank != -1)
			break;
	}

	if (ind <= line_length)
		return;
	if (ch_isspace(com.s[com.len - 1]))
		return;

	if (*last_blank == -1) {	/* only a single word in this line */
		output_line();
		com_add_star();
		return;
	}

	// Move the overlong word to the next line.
	const char *last_word = com.s + *last_blank + 1;
	size_t last_word_len = com.len - (size_t)(*last_blank + 1);
	com.len = (size_t)*last_blank;
	buf_terminate(&com);
	output_line();
	com_add_star();

	/* Assume that output_line and com_add_delim left the "unused" part of
	 * the now truncated buffer beyond com.s + com.len as-is. */
	memmove(com.s + com.len, last_word, last_word_len);
	com.len += last_word_len;
	buf_terminate(&com);
	*last_blank = -1;
}

/* In a comment that is re-wrapped, handle a single newline character. */
static bool
copy_comment_wrap_newline(ssize_t *last_blank, bool seen_newline)
{
	*last_blank = -1;
	if (seen_newline) {
		if (com.len > 3) {
			output_line();
			com_add_star();
		}
		output_line();
		com_add_star();
	} else {
		if (!(com.len > 0 && ch_isblank(com.s[com.len - 1])))
			com_add_char(' ');
		*last_blank = (int)com.len - 1;
	}
	line_no++;

	/* flush any blanks and/or tabs at start of next line */
	inp_skip();		/* '\n' */
	while (ch_isblank(inp_p[0]))
		inp_p++;
	if (inp_p[0] == '*' && inp_p[1] == '/')
		return false;
	if (inp_p[0] == '*') {
		inp_p++;
		while (ch_isblank(inp_p[0]))
			inp_p++;
	}

	return true;
}

static void
copy_comment_wrap_finish(int line_length, bool delim)
{
	if (delim) {
		if (com.len > 3)
			output_line();
		buf_clear(&com);
	} else {
		size_t len = com.len;
		// XXX: This loop differs from the one below.
		while (ch_isblank(com.s[len - 1]))
			len--;
		if (ind_add(ps.comment_ind, com.s, len) + 3 > line_length)
			output_line();
	}

	while (com.len >= 2
	    && ch_isblank(com.s[com.len - 1])
	    && ch_isblank(com.s[com.len - 2]))
		com.len--;
	buf_terminate(&com);

	inp_p += 2;
	if (com.len > 0 && ch_isblank(com.s[com.len - 1]))
		buf_add_chars(&com, "*/", 2);
	else
		buf_add_chars(&com, " */", 3);
}

static void
copy_comment_wrap(int line_length, bool delim)
{
	ssize_t last_blank = -1;	/* index of the last blank in 'com' */
	bool seen_newline = false;

	for (;;) {
		if (inp_p[0] == '\n') {
			if (had_eof)
				goto unterminated_comment;
			if (!copy_comment_wrap_newline(&last_blank,
				seen_newline))
				break;
			seen_newline = true;
		} else if (inp_p[0] == '*' && inp_p[1] == '/')
			break;
		else {
			copy_comment_wrap_text(line_length, &last_blank);
			seen_newline = false;
		}
	}

	copy_comment_wrap_finish(line_length, delim);
	return;

unterminated_comment:
	diag(1, "Unterminated comment");
	output_line();
}

static void
copy_comment_nowrap(void)
{
	char kind = token.s[token.len - 1];

	for (;;) {
		if (inp_p[0] == '\n') {
			if (kind == '/')
				return;

			if (had_eof) {
				diag(1, "Unterminated comment");
				output_line();
				return;
			}

			output_line();
			line_no++;
			inp_skip();
			continue;
		}

		if (kind == '*' && inp_p[0] == '*' && inp_p[1] == '/') {
			com_add_char(*inp_p++);
			com_add_char(*inp_p++);
			return;
		}

		com_add_char(*inp_p++);
	}
}

void
process_comment(void)
{
	bool may_wrap, delim;
	int line_length;

	analyze_comment(&may_wrap, &delim, &line_length);
	copy_comment_start(may_wrap, &delim, line_length);
	if (may_wrap)
		copy_comment_wrap(line_length, delim);
	else
		copy_comment_nowrap();
}
