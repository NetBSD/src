/*	$NetBSD: io.c,v 1.184 2023/05/20 12:05:01 rillig Exp $	*/

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
__RCSID("$NetBSD: io.c,v 1.184 2023/05/20 12:05:01 rillig Exp $");

#include <stdio.h>

#include "indent.h"

struct buffer inp;
struct output_state out;
static unsigned wrote_newlines = 2;	/* 0 in the middle of a line, 1 after a
					 * single '\n', > 1 means there were (n
					 * - 1) blank lines above */
static int paren_indent;


void
inp_skip(void)
{
	inp.st++;
	if ((size_t)(inp.st - inp.mem) >= inp.len)
		inp_read_line();
}

char
inp_next(void)
{
	char ch = inp.st[0];
	inp_skip();
	return ch;
}

static void
inp_read_next_line(FILE *f)
{
	inp.st = inp.mem;
	inp.len = 0;

	for (;;) {
		int ch = getc(f);
		if (ch == EOF) {
			if (indent_enabled == indent_on) {
				buf_add_char(&inp, ' ');
				buf_add_char(&inp, '\n');
			}
			had_eof = true;
			break;
		}

		if (ch != '\0')
			buf_add_char(&inp, (char)ch);
		if (ch == '\n')
			break;
	}
}

static void
output_newline(void)
{
	fputc('\n', output);
	debug_println("output_newline");
	wrote_newlines++;
}

static void
output_range(const char *s, size_t len)
{
	fwrite(s, 1, len, output);
	debug_vis_range("output_range \"", s, len, "\"\n");
	for (size_t i = 0; i < len; i++)
		wrote_newlines = s[i] == '\n' ? wrote_newlines + 1 : 0;
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
			wrote_newlines = 0;
		}
	}

	for (; ind < new_ind; ind++) {
		fputc(' ', output);
		wrote_newlines = 0;
	}

	debug_println("output_indent %d", ind);
	return ind;
}

static bool
want_blank_line(void)
{
	debug_println("%s: %s -> %s", __func__,
	    line_kind_name[out.prev_line_kind], line_kind_name[out.line_kind]);

	if (ps.blank_line_after_decl && ps.declaration == decl_no) {
		ps.blank_line_after_decl = false;
		return true;
	}
	if (opt.blanklines_around_conditional_compilation) {
		if (out.prev_line_kind != lk_if && out.line_kind == lk_if)
			return true;
		if (out.prev_line_kind == lk_endif
		    && out.line_kind != lk_endif)
			return true;
	}
	if (opt.blanklines_after_procs && out.prev_line_kind == lk_func_end
	    && out.line_kind != lk_endif)
		return true;
	if (opt.blanklines_before_block_comments
	    && out.line_kind == lk_block_comment)
		return true;
	return false;
}

static int
output_line_label(void)
{
	int ind;

	while (lab.len > 0 && ch_isblank(lab.mem[lab.len - 1]))
		lab.len--;

	ind = output_indent(0, compute_label_indent());
	output_range(lab.st, lab.len);
	ind = ind_add(ind, lab.st, lab.len);

	ps.is_case_label = false;
	return ind;
}

static int
output_line_code(int ind)
{

	int target_ind = compute_code_indent();
	for (int i = 0; i < ps.nparen; i++) {
		int paren_ind = ps.paren[i].indent;
		if (paren_ind >= 0) {
			ps.paren[i].indent = -1 - (paren_ind + target_ind);
			debug_println(
			    "setting paren_indents[%d] from %d to %d "
			    "for column %d",
			    i, paren_ind, ps.paren[i].indent, target_ind + 1);
		}
	}

	ind = output_indent(ind, target_ind);
	output_range(code.st, code.len);
	return ind_add(ind, code.st, code.len);
}

static void
output_line_comment(int ind)
{
	int target_ind = ps.com_ind;
	const char *p = com.st;

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
		output_newline();
		ind = 0;
	}

	while (com.mem + com.len > p && ch_isspace(com.mem[com.len - 1]))
		com.len--;

	(void)output_indent(ind, target_ind);
	output_range(p, com.len - (size_t)(p - com.mem));

	ps.comment_delta = ps.n_comment_delta;
}

/*
 * Write a line of formatted source to the output file. The line consists of
 * the label, the code and the comment.
 *
 * Comments are written directly, bypassing this function.
 */
void
output_line(void)
{
	debug_printf("%s", __func__);
	debug_buffers();
	debug_println("");

	ps.is_function_definition = false;

	if (indent_enabled == indent_on) {
		if (want_blank_line() && wrote_newlines < 2
		    && (lab.len > 0 || code.len > 0 || com.len > 0))
			output_newline();

		if (ps.ind_level == 0)
			ps.in_stmt_cont = false;	/* this is a class A
							 * kludge */

		if (opt.blank_line_after_decl && ps.declaration == decl_end
		    && ps.tos > 1) {
			ps.declaration = decl_no;
			ps.blank_line_after_decl = true;
		}

		int ind = 0;
		if (lab.len > 0)
			ind = output_line_label();
		if (code.len > 0)
			ind = output_line_code(ind);
		if (com.len > 0)
			output_line_comment(ind);

		output_newline();
	}

	if (indent_enabled == indent_last_off_line) {
		indent_enabled = indent_on;
		output_range(out.indent_off_text.st, out.indent_off_text.len);
		out.indent_off_text.len = 0;
	}

	ps.decl_on_line = ps.in_decl;	/* for proper comment indentation */
	ps.in_stmt_cont = ps.in_stmt_or_decl && !ps.in_decl;
	ps.decl_indent_done = false;
	if (ps.extra_expr_indent == eei_last)
		ps.extra_expr_indent = eei_no;

	lab.len = 0;
	code.len = 0;
	com.len = 0;

	ps.ind_level = ps.ind_level_follow;
	ps.line_start_nparen = ps.nparen;

	if (ps.nparen > 0) {
		/* TODO: explain what negative indentation means */
		paren_indent = -1 - ps.paren[ps.nparen - 1].indent;
		debug_println("paren_indent is now %d", paren_indent);
	}

	ps.want_blank = false;
	out.prev_line_kind = out.line_kind;
	out.line_kind = lk_other;
}

static int
compute_code_indent_lineup(int base_ind)
{
	int ind = paren_indent;
	int overflow = ind_add(ind, code.st, code.len) - opt.max_line_length;
	if (overflow < 0)
		return ind;

	if (ind_add(base_ind, code.st, code.len) < opt.max_line_length) {
		ind -= overflow + 2;
		if (ind > base_ind)
			return ind;
		return base_ind;
	}

	return ind;
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

	if (ps.extra_expr_indent != eei_no)
		return base_ind + 2 * opt.continuation_indent;

	if (2 * opt.continuation_indent == opt.indent_size)
		return base_ind + opt.continuation_indent;
	else
		return base_ind +
		    opt.continuation_indent * ps.line_start_nparen;
}

int
compute_label_indent(void)
{
	if (ps.is_case_label)
		return (int)(case_ind * (float)opt.indent_size);
	if (lab.st[0] == '#')
		return 0;
	return opt.indent_size * (ps.ind_level - 2);
}

void
inp_read_line(void)
{
	if (indent_enabled == indent_on)
		out.indent_off_text.len = 0;
	buf_add_chars(&out.indent_off_text, inp.mem, inp.len);
	inp_read_next_line(input);
}
