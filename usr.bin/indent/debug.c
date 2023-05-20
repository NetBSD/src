/*	$NetBSD: debug.c,v 1.19 2023/05/20 11:53:53 rillig Exp $	*/

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
__RCSID("$NetBSD: debug.c,v 1.19 2023/05/20 11:53:53 rillig Exp $");

#include <stdarg.h>

#include "indent.h"

#ifdef debug

/*-
 * false	show only the changes to the parser state
 * true		show unchanged parts of the parser state as well
 */
static bool debug_full_parser_state = true;

const char *const lsym_name[] = {
	"eof",
	"preprocessing",
	"newline",
	"comment",
	"lparen_or_lbracket",
	"rparen_or_rbracket",
	"lbrace",
	"rbrace",
	"period",
	"unary_op",
	"binary_op",
	"postfix_op",
	"question",
	"colon",
	"comma",
	"semicolon",
	"typedef",
	"storage_class",
	"type_outside_parentheses",
	"type_in_parentheses",
	"tag",
	"case_label",
	"sizeof",
	"offsetof",
	"word",
	"funcname",
	"do",
	"else",
	"for",
	"if",
	"switch",
	"while",
	"return",
};

const char *const psym_name[] = {
	"0",
	"lbrace",
	"rbrace",
	"decl",
	"stmt",
	"stmt_list",
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

static const char *const in_enum_name[] = {
	"no",
	"enum",
	"type",
	"brace",
};

const char *const paren_level_cast_name[] = {
	"(unknown cast)",
	"(maybe cast)",
	"(no cast)",
};

const char *const line_kind_name[] = {
	"other",
	"#if",
	"#endif",
	"}",
	"block comment",
};

void
debug_printf(const char *fmt, ...)
{
	FILE *f = output == stdout ? stderr : stdout;
	va_list ap;

	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
}

void
debug_println(const char *fmt, ...)
{
	FILE *f = output == stdout ? stderr : stdout;
	va_list ap;

	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fprintf(f, "\n");
}

void
debug_vis_range(const char *prefix, const char *s, size_t len,
    const char *suffix)
{
	debug_printf("%s", prefix);
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
	debug_printf("%s", suffix);
}

static void
debug_print_buf(const char *name, const struct buffer *buf)
{
	if (buf->len > 0) {
		debug_printf("%s ", name);
		debug_vis_range("\"", buf->st, buf->len, "\"\n");
	}
}

void
debug_buffers(void)
{
	if (lab.len > 0) {
		debug_printf(" label ");
		debug_vis_range("\"", lab.st, lab.len, "\"");
	}
	if (code.len > 0) {
		debug_printf(" code ");
		debug_vis_range("\"", code.st, code.len, "\"");
	}
	if (com.len > 0) {
		debug_printf(" comment ");
		debug_vis_range("\"", com.st, com.len, "\"");
	}
}

#define debug_ps_bool(name) \
	if (ps.name != prev_ps.name) \
	    debug_println("[%c] -> [%c] ps." #name, \
		prev_ps.name ? 'x' : ' ', ps.name ? 'x' : ' '); \
	else if (debug_full_parser_state) \
	    debug_println("       [%c] ps." #name, ps.name ? 'x' : ' ')
#define debug_ps_int(name) \
	if (ps.name != prev_ps.name) \
	    debug_println("%3d -> %3d ps." #name, prev_ps.name, ps.name); \
	else if (debug_full_parser_state) \
	    debug_println("       %3d ps." #name, ps.name)
#define debug_ps_enum(name, names) \
	if (ps.name != prev_ps.name) \
	    debug_println("%3s -> %3s ps." #name, \
		(names)[prev_ps.name], (names)[ps.name]); \
	else if (debug_full_parser_state) \
	    debug_println("%10s ps." #name, (names)[ps.name])

static bool
ps_paren_has_changed(const struct parser_state *prev_ps)
{
	if (prev_ps->nparen != ps.nparen)
		return true;

	const paren_level_props *prev = prev_ps->paren, *curr = ps.paren;
	for (int i = 0; i < ps.nparen; i++)
		if (curr[i].indent != prev[i].indent
		    || curr[i].cast != prev[i].cast)
			return true;
	return false;
}

static void
debug_ps_paren(const struct parser_state *prev_ps)
{
	if (!debug_full_parser_state && !ps_paren_has_changed(prev_ps))
		return;

	debug_printf("           ps.paren:");
	for (int i = 0; i < ps.nparen; i++) {
		debug_printf(" %s%d",
		    paren_level_cast_name[ps.paren[i].cast],
		    ps.paren[i].indent);
	}
	if (ps.nparen == 0)
		debug_printf(" none");
	debug_println("");
}

static bool
ps_di_stack_has_changed(const struct parser_state *prev_ps)
{
	if (prev_ps->decl_level != ps.decl_level)
		return true;
	for (int i = 0; i < ps.decl_level; i++)
		if (prev_ps->di_stack[i] != ps.di_stack[i])
			return true;
	return false;
}

static void
debug_ps_di_stack(const struct parser_state *prev_ps)
{
	bool changed = ps_di_stack_has_changed(prev_ps);
	if (!debug_full_parser_state && !changed)
		return;

	debug_printf("    %s     ps.di_stack:", changed ? "->" : "  ");
	for (int i = 0; i < ps.decl_level; i++)
		debug_printf(" %d", ps.di_stack[i]);
	if (ps.decl_level == 0)
		debug_printf(" none");
	debug_println("");
}

void
debug_parser_state(lexer_symbol lsym)
{
	static struct parser_state prev_ps;

	debug_println("");
	debug_printf("line %d: %s", line_no, lsym_name[lsym]);
	debug_vis_range(" \"", token.st, token.len, "\"\n");

	debug_print_buf("label", &lab);
	debug_print_buf("code", &code);
	debug_print_buf("comment", &com);

	debug_println("           ps.prev_token = %s",
	    lsym_name[ps.prev_token]);
	debug_ps_bool(curr_col_1);
	debug_ps_bool(next_col_1);
	debug_ps_bool(next_unary);
	debug_ps_bool(is_function_definition);
	debug_ps_bool(want_blank);
	debug_ps_bool(force_nl);
	debug_ps_int(line_start_nparen);
	debug_ps_int(nparen);
	debug_ps_paren(&prev_ps);

	debug_ps_int(comment_delta);
	debug_ps_int(n_comment_delta);
	debug_ps_int(com_ind);

	debug_ps_bool(block_init);
	debug_ps_int(block_init_level);
	debug_ps_bool(init_or_struct);

	debug_ps_int(ind_level);
	debug_ps_int(ind_level_follow);

	debug_ps_int(decl_level);
	debug_ps_di_stack(&prev_ps);
	debug_ps_bool(decl_on_line);
	debug_ps_bool(in_decl);
	debug_ps_enum(declaration, declaration_name);
	debug_ps_bool(blank_line_after_decl);
	debug_ps_bool(in_func_def_params);
	debug_ps_enum(in_enum, in_enum_name);
	debug_ps_bool(decl_indent_done);
	debug_ps_int(decl_ind);
	debug_ps_bool(tabs_to_var);

	debug_ps_bool(in_stmt_or_decl);
	debug_ps_bool(in_stmt_cont);
	debug_ps_bool(is_case_label);
	debug_ps_bool(seen_case);

	// The debug output for the parser symbols is done in 'parse' instead.

	debug_ps_enum(spaced_expr_psym, psym_name);
	debug_ps_int(quest_level);

	prev_ps = ps;
}

void
debug_parse_stack(const char *situation)
{
	printf("parse stack %s:", situation);
	for (int i = 1; i <= ps.tos; ++i)
		printf(" %s %d", psym_name[ps.s_sym[i]], ps.s_ind_level[i]);
	if (ps.tos == 0)
		printf(" empty");
	printf("\n");
}
#endif
