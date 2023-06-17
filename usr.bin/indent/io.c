/*	$NetBSD: io.c,v 1.229 2023/06/17 23:03:20 rillig Exp $	*/

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
__RCSID("$NetBSD: io.c,v 1.229 2023/06/17 23:03:20 rillig Exp $");

#include <stdio.h>

#include "indent.h"

struct buffer inp;
const char *inp_p;

struct output_state out;
enum indent_enabled indent_enabled;
static int out_ind;		/* width of the line that is being written */
static unsigned newlines = 2;	/* the total of written and buffered newlines;
				 * 0 in the middle of a line, 1 after a single
				 * finished line, anything > 1 are trailing
				 * blank lines */
static unsigned buffered_newlines;	/* not yet written */
static int paren_indent;	/* total indentation when parenthesized */


static void
inp_read_next_line(void)
{
	buf_clear(&inp);

	for (;;) {
		int ch = getc(input);
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
	buf_terminate(&inp);
	inp_p = inp.s;
}

void
inp_read_line(void)
{
	if (indent_enabled == indent_on)
		buf_clear(&out.indent_off_text);
	buf_add_chars(&out.indent_off_text, inp.s, inp.len);
	inp_read_next_line();
}

void
inp_skip(void)
{
	inp_p++;
	if ((size_t)(inp_p - inp.s) >= inp.len)
		inp_read_line();
}

char
inp_next(void)
{
	char ch = inp_p[0];
	inp_skip();
	return ch;
}


static void
add_buffered_newline(void)
{
	buffered_newlines++;
	newlines++;
	out_ind = 0;
}

static void
write_buffered_newlines(void)
{
	for (; buffered_newlines > 0; buffered_newlines--) {
		fputc('\n', output);
		debug_println("write_newline");
	}
}

static void
write_range(const char *s, size_t len)
{
	write_buffered_newlines();
	fwrite(s, 1, len, output);
	debug_printf("write_range ");
	debug_vis_range(s, len);
	debug_println("");
	for (size_t i = 0; i < len; i++)
		newlines = s[i] == '\n' ? newlines + 1 : 0;
	out_ind = ind_add(out_ind, s, len);
}

static void
write_indent(int new_ind)
{
	write_buffered_newlines();

	int ind = out_ind;

	if (opt.use_tabs) {
		int n = new_ind / opt.tabsize - ind / opt.tabsize;
		if (n > 0) {
			ind = ind - ind % opt.tabsize + n * opt.tabsize;
			while (n-- > 0)
				fputc('\t', output);
			newlines = 0;
		}
	}

	for (; ind < new_ind; ind++) {
		fputc(' ', output);
		newlines = 0;
	}

	debug_println("write_indent %d", ind);
	out_ind = ind;
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
	if (opt.blank_line_around_conditional_compilation) {
		if (out.prev_line_kind != lk_pre_if
		    && out.line_kind == lk_pre_if)
			return true;
		if (out.prev_line_kind == lk_pre_endif
		    && out.line_kind != lk_pre_endif)
			return true;
	}
	if (opt.blank_line_after_proc && out.prev_line_kind == lk_func_end
	    && out.line_kind != lk_pre_endif && out.line_kind != lk_pre_other)
		return true;
	if (opt.blank_line_before_block_comment
	    && out.line_kind == lk_block_comment)
		return true;
	return false;
}

static bool
is_blank_line_optional(void)
{
	if (out.prev_line_kind == lk_stmt_head)
		return newlines >= 1;
	if (ps.psyms.len >= 3)
		return newlines >= 2;
	return newlines >= 3;
}

static int
compute_case_label_indent(void)
{
	size_t i = ps.psyms.len - 1;
	while (i > 0 && ps.psyms.sym[i] != psym_switch_expr)
		i--;
	float case_ind = (float)ps.psyms.ind_level[i] + opt.case_indent;
	// TODO: case_ind may become negative here.
	return (int)(case_ind * (float)opt.indent_size);
}

int
compute_label_indent(void)
{
	if (out.line_kind == lk_case_or_default)
		return compute_case_label_indent();
	if (lab.s[0] == '#')
		return 0;
	// TODO: the indentation may become negative here.
	return opt.indent_size * (ps.ind_level - 2);
}

static void
output_line_label(void)
{
	write_indent(compute_label_indent());
	write_range(lab.s, lab.len);
}

static int
compute_lined_up_code_indent(int base_ind)
{
	int ind = paren_indent;
	int overflow = ind_add(ind, code.s, code.len) - opt.max_line_length;
	if (overflow >= 0
	    && ind_add(base_ind, code.s, code.len) < opt.max_line_length) {
		ind -= 2 + overflow;
		if (ind < base_ind)
			ind = base_ind;
	}

	if (ps.extra_expr_indent != eei_no
	    && ind == base_ind + opt.indent_size)
		ind += opt.continuation_indent;
	return ind;
}

int
compute_code_indent(void)
{
	int base_ind = ps.ind_level * opt.indent_size;

	if (ps.ind_paren_level == 0) {
		if (ps.line_is_stmt_cont)
			return base_ind + opt.continuation_indent;
		return base_ind;
	}

	if (opt.lineup_to_parens) {
		if (opt.lineup_to_parens_always)
			return paren_indent;
		return compute_lined_up_code_indent(base_ind);
	}

	int rel_ind = opt.continuation_indent * ps.ind_paren_level;
	if (ps.extra_expr_indent != eei_no && rel_ind == opt.indent_size)
		rel_ind += opt.continuation_indent;
	return base_ind + rel_ind;
}

static void
output_line_code(void)
{
	int target_ind = compute_code_indent();
	for (size_t i = 0; i < ps.paren.len; i++) {
		int paren_ind = ps.paren.item[i].indent;
		if (paren_ind >= 0) {
			ps.paren.item[i].indent =
			    -1 - (paren_ind + target_ind);
			debug_println(
			    "setting paren_indents[%zu] from %d to %d "
			    "for column %d",
			    i, paren_ind,
			    ps.paren.item[i].indent, target_ind + 1);
		}
	}

	if (lab.len > 0 && target_ind <= out_ind)
		write_range(" ", 1);
	write_indent(target_ind);
	write_range(code.s, code.len);
}

static void
output_comment(void)
{
	int target_ind = ps.comment_ind;
	const char *p;

	if (ps.comment_cont)
		target_ind += ps.comment_shift;
	ps.comment_cont = true;

	/* consider the original indentation in case this is a box comment */
	for (p = com.s; *p == '\t'; p++)
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

	if (out_ind > target_ind)
		add_buffered_newline();

	while (com.s + com.len > p && ch_isspace(com.s[com.len - 1]))
		com.len--;
	buf_terminate(&com);

	write_indent(target_ind);
	write_range(p, com.len - (size_t)(p - com.s));
}

/*
 * Write a line of formatted source to the output file. The line consists of
 * the label, the code and the comment.
 */
static void
output_indented_line(void)
{
	if (lab.len == 0 && code.len == 0 && com.len == 0)
		out.line_kind = lk_blank;

	if (want_blank_line() && newlines < 2 && out.line_kind != lk_blank)
		add_buffered_newline();

	/* This kludge aligns function definitions correctly. */
	if (ps.ind_level == 0)
		ps.line_is_stmt_cont = false;

	if (opt.blank_line_after_decl && ps.declaration == decl_end
	    && ps.psyms.len > 2) {
		ps.declaration = decl_no;
		ps.blank_line_after_decl = true;
	}

	if (opt.swallow_optional_blank_lines
	    && out.line_kind == lk_blank
	    && is_blank_line_optional())
		return;

	if (lab.len > 0)
		output_line_label();
	if (code.len > 0)
		output_line_code();
	if (com.len > 0)
		output_comment();
	add_buffered_newline();
	if (out.line_kind != lk_blank)
		write_buffered_newlines();

	out.prev_line_kind = out.line_kind;
}

static bool
is_stmt_cont(void)
{
	if (ps.psyms.len >= 2
	    && ps.psyms.sym[ps.psyms.len - 2] == psym_lbrace_enum
	    && ps.prev_lsym == lsym_comma
	    && ps.paren.len == 0)
		return false;
	return ps.in_stmt_or_decl
	    && (!ps.in_decl || ps.in_init)
	    && ps.init_level == 0;
}

static void
prepare_next_line(void)
{
	ps.line_has_decl = ps.in_decl;
	ps.line_has_func_def = false;
	ps.line_is_stmt_cont = is_stmt_cont();
	ps.decl_indent_done = false;
	if (ps.extra_expr_indent == eei_last)
		ps.extra_expr_indent = eei_no;
	if (!(ps.psyms.sym[ps.psyms.len - 1] == psym_if_expr_stmt_else
		&& ps.paren.len > 0))
		ps.ind_level = ps.ind_level_follow;
	ps.ind_paren_level = (int)ps.paren.len;
	ps.want_blank = false;

	if (ps.paren.len > 0) {
		/* TODO: explain what negative indentation means */
		paren_indent = -1 - ps.paren.item[ps.paren.len - 1].indent;
		debug_println("paren_indent is now %d", paren_indent);
	}

	out.line_kind = lk_other;
}

void
output_line(void)
{
	debug_blank_line();
	debug_printf("%s", __func__);
	debug_buffers();

	if (indent_enabled == indent_on)
		output_indented_line();
	else if (indent_enabled == indent_last_off_line) {
		indent_enabled = indent_on;
		write_range(out.indent_off_text.s, out.indent_off_text.len);
		buf_clear(&out.indent_off_text);
	}

	buf_clear(&lab);
	buf_clear(&code);
	buf_clear(&com);

	prepare_next_line();
}

void
finish_output(void)
{
	output_line();
	if (indent_enabled != indent_on) {
		indent_enabled = indent_last_off_line;
		output_line();
	}
	fflush(output);
}
