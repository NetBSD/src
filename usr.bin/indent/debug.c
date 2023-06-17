/*	$NetBSD: debug.c,v 1.67 2023/06/17 22:28:49 rillig Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig <rillig@NetBSD.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: debug.c,v 1.67 2023/06/17 22:28:49 rillig Exp $");

#include <stdarg.h>
#include <string.h>

#include "indent.h"

#ifdef debug

static struct {
	// false	show only the changes to the parser state
	// true		show unchanged parts of the parser state as well
	bool full_parser_state;
} config = {
	.full_parser_state = false,
};

const char *const lsym_name[] = {
	"eof",
	"preprocessing",
	"newline",
	"comment",
	"lparen",
	"rparen",
	"lbracket",
	"rbracket",
	"lbrace",
	"rbrace",
	"period",
	"unary_op",
	"sizeof",
	"offsetof",
	"postfix_op",
	"binary_op",
	"question",
	"question_colon",
	"comma",
	"typedef",
	"modifier",
	"tag",
	"type",
	"word",
	"funcname",
	"label_colon",
	"other_colon",
	"semicolon",
	"case",
	"default",
	"do",
	"else",
	"for",
	"if",
	"switch",
	"while",
	"return",
};

const char *const psym_name[] = {
	"-",
	"{block",
	"{struct",
	"{union",
	"{enum",
	"}",
	"decl",
	"stmt",
	"for_exprs",
	"if_expr",
	"if_expr_stmt",
	"if_expr_stmt_else",
	"else",
	"switch_expr",
	"do",
	"do_stmt",
	"while_expr",
};

static const char *const declaration_name[] = {
	"no",
	"begin",
	"end",
};

const char *const paren_level_cast_name[] = {
	"(unknown cast)",
	"(maybe cast)",
	"(no cast)",
};

const char *const line_kind_name[] = {
	"other",
	"blank",
	"#if",
	"#endif",
	"#other",
	"stmt head",
	"}",
	"block comment",
	"case/default",
};

static const char *const extra_expr_indent_name[] = {
	"no",
	"maybe",
	"last",
};

static struct {
	struct parser_state prev_ps;
	bool ps_first;
	const char *heading;
	unsigned wrote_newlines;
} state = {
	.ps_first = true,
	.wrote_newlines = 1,
};

void
debug_printf(const char *fmt, ...)
{
	FILE *f = output == stdout ? stderr : stdout;
	va_list ap;

	if (state.heading != NULL) {
		fprintf(f, "%s\n", state.heading);
		state.heading = NULL;
	}
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	state.wrote_newlines = 0;
}

void
debug_println(const char *fmt, ...)
{
	FILE *f = output == stdout ? stderr : stdout;
	va_list ap;

	if (state.heading != NULL) {
		fprintf(f, "%s\n", state.heading);
		state.heading = NULL;
		state.wrote_newlines = 1;
	}
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fprintf(f, "\n");
	state.wrote_newlines = fmt[0] == '\0' ? state.wrote_newlines + 1 : 1;
}

void
debug_blank_line(void)
{
	while (state.wrote_newlines < 2)
		debug_println("");
}

void
debug_vis_range(const char *s, size_t len)
{
	debug_printf("\"");
	for (size_t i = 0; i < len; i++) {
		const char *p = s + i;
		if (*p == '\\' || *p == '"')
			debug_printf("\\%c", *p);
		else if (isprint((unsigned char)*p))
			debug_printf("%c", *p);
		else if (*p == '\n')
			debug_printf("\\n");
		else if (*p == '\t')
			debug_printf("\\t");
		else
			debug_printf("\\x%02x", (unsigned char)*p);
	}
	debug_printf("\"");
}

void
debug_print_buf(const char *name, const struct buffer *buf)
{
	if (buf->len > 0) {
		debug_printf(" %s ", name);
		debug_vis_range(buf->s, buf->len);
	}
}

void
debug_buffers(void)
{
	debug_print_buf("label", &lab);
	debug_print_buf("code", &code);
	debug_print_buf("comment", &com);
	debug_blank_line();
}

static void
debug_ps_bool_member(const char *name, bool prev, bool curr)
{
	if (!state.ps_first && curr != prev) {
		char diff = " -+x"[(prev ? 1 : 0) + (curr ? 2 : 0)];
		debug_println("        [%c]  ps.%s", diff, name);
	} else if (config.full_parser_state || state.ps_first)
		debug_println("        [%c]  ps.%s", curr ? 'x' : ' ', name);
}

static void
debug_ps_int_member(const char *name, int prev, int curr)
{
	if (!state.ps_first && curr != prev)
		debug_println(" %3d -> %3d  ps.%s", prev, curr, name);
	else if (config.full_parser_state || state.ps_first)
		debug_println("        %3d  ps.%s", curr, name);
}

static void
debug_ps_enum_member(const char *name, const char *prev, const char *curr)
{
	if (!state.ps_first && strcmp(prev, curr) != 0)
		debug_println(" %3s -> %3s  ps.%s", prev, curr, name);
	else if (config.full_parser_state || state.ps_first)
		debug_println(" %10s  ps.%s", curr, name);
}

static bool
paren_stack_equal(const struct paren_stack *a, const struct paren_stack *b)
{
	if (a->len != b->len)
		return false;

	for (size_t i = 0, n = a->len; i < n; i++)
		if (a->item[i].indent != b->item[i].indent
		    || a->item[i].cast != b->item[i].cast)
			return true;
	return false;
}

static void
debug_ps_paren(void)
{
	if (!config.full_parser_state
	    && paren_stack_equal(&state.prev_ps.paren, &ps.paren)
	    && !state.ps_first)
		return;

	debug_printf("             ps.paren:");
	for (size_t i = 0; i < ps.paren.len; i++) {
		debug_printf(" %s%d",
		    paren_level_cast_name[ps.paren.item[i].cast],
		    ps.paren.item[i].indent);
	}
	if (ps.paren.len == 0)
		debug_printf(" none");
	debug_println("");
}

static bool
ps_di_stack_has_changed(void)
{
	if (state.prev_ps.decl_level != ps.decl_level)
		return true;
	for (int i = 0; i < ps.decl_level; i++)
		if (state.prev_ps.di_stack[i] != ps.di_stack[i])
			return true;
	return false;
}

static void
debug_ps_di_stack(void)
{
	bool changed = ps_di_stack_has_changed();
	if (!config.full_parser_state && !changed && !state.ps_first)
		return;

	debug_printf("     %s      ps.di_stack:", changed ? "->" : "  ");
	for (int i = 0; i < ps.decl_level; i++)
		debug_printf(" %d", ps.di_stack[i]);
	if (ps.decl_level == 0)
		debug_printf(" none");
	debug_println("");
}

#define debug_ps_bool(name) \
	debug_ps_bool_member(#name, state.prev_ps.name, ps.name)
#define debug_ps_int(name) \
	debug_ps_int_member(#name, state.prev_ps.name, ps.name)
#define debug_ps_enum(name, names) \
        debug_ps_enum_member(#name, (names)[state.prev_ps.name], \
	    (names)[ps.name])

void
debug_parser_state(void)
{
	debug_blank_line();

	state.heading = "token classification";
	debug_ps_enum(prev_lsym, lsym_name);
	debug_ps_bool(in_stmt_or_decl);
	debug_ps_bool(in_decl);
	debug_ps_bool(in_typedef_decl);
	debug_ps_bool(in_var_decl);
	debug_ps_bool(in_init);
	debug_ps_int(init_level);
	debug_ps_bool(line_has_func_def);
	debug_ps_bool(in_func_def_params);
	debug_ps_bool(line_has_decl);
	debug_ps_enum(lbrace_kind, psym_name);
	debug_ps_enum(spaced_expr_psym, psym_name);
	debug_ps_bool(seen_case);
	debug_ps_bool(prev_paren_was_cast);
	debug_ps_int(quest_level);

	state.heading = "indentation of statements and declarations";
	debug_ps_int(ind_level);
	debug_ps_int(ind_level_follow);
	debug_ps_bool(line_is_stmt_cont);
	debug_ps_int(decl_level);
	debug_ps_di_stack();
	debug_ps_bool(decl_indent_done);
	debug_ps_int(decl_ind);
	debug_ps_bool(tabs_to_var);
	debug_ps_enum(extra_expr_indent, extra_expr_indent_name);

	// The parser symbol stack is printed in debug_psyms_stack instead.

	state.heading = "spacing inside a statement or declaration";
	debug_ps_bool(next_unary);
	debug_ps_bool(want_blank);
	debug_ps_int(ind_paren_level);
	debug_ps_paren();

	state.heading = "indentation of comments";
	debug_ps_int(comment_ind);
	debug_ps_int(comment_shift);
	debug_ps_bool(comment_cont);

	state.heading = "vertical spacing";
	debug_ps_bool(break_after_comma);
	debug_ps_bool(want_newline);
	debug_ps_enum(declaration, declaration_name);
	debug_ps_bool(blank_line_after_decl);

	state.heading = NULL;
	debug_blank_line();

	state.prev_ps = ps;
	state.ps_first = false;
}

void
debug_psyms_stack(const char *situation)
{
	debug_printf("parse stack %s:", situation);
	const struct psym_stack *psyms = &ps.psyms;
	for (size_t i = 0; i < psyms->len; i++)
		debug_printf(" %d %s",
		    psyms->ind_level[i], psym_name[psyms->sym[i]]);
	debug_println("");
}
#endif
