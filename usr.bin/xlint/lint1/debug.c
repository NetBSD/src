/* $NetBSD: debug.c,v 1.55 2023/07/13 23:27:20 rillig Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: debug.c,v 1.55 2023/07/13 23:27:20 rillig Exp $");
#endif

#include <stdlib.h>

#include "lint1.h"
#include "cgram.h"


#ifdef DEBUG

static int debug_indentation = 0;


static FILE *
debug_file(void)
{
	/*
	 * Using stdout preserves the order between the debug messages and
	 * lint's diagnostics.
	 */
	return yflag ? stderr : stdout;
}

void
debug_printf(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	(void)vfprintf(debug_file(), fmt, va);
	va_end(va);
}

void
debug_print_indent(void)
{

	debug_printf("%s%*s", yflag ? "| " : "", 2 * debug_indentation, "");
}

void
debug_indent_inc(void)
{

	debug_indentation++;
}

void
debug_indent_dec(void)
{

	debug_indentation--;
}

void
debug_step(const char *fmt, ...)
{
	va_list va;

	debug_print_indent();
	va_start(va, fmt);
	(void)vfprintf(debug_file(), fmt, va);
	va_end(va);
	fprintf(debug_file(), "\n");
}

void
debug_enter_func(const char *func)
{

	debug_step("+ %s", func);
	debug_indent_inc();
}

void
debug_leave_func(const char *func)
{

	debug_indent_dec();
	debug_step("- %s", func);
}

static void
debug_type_details(const type_t *tp)
{

	if (is_struct_or_union(tp->t_tspec)) {
		debug_indent_inc();
		debug_step("size %u bits, align %u bits, %s",
		    tp->t_sou->sou_size_in_bits, tp->t_sou->sou_align_in_bits,
		    tp->t_sou->sou_incomplete ? "incomplete" : "complete");

		for (const sym_t *mem = tp->t_sou->sou_first_member;
		     mem != NULL; mem = mem->s_next) {
			debug_sym("", mem, "\n");
			debug_type_details(mem->s_type);
		}
		debug_indent_dec();
	}
	if (tp->t_is_enum) {
		debug_indent_inc();
		for (const sym_t *en = tp->t_enum->en_first_enumerator;
		     en != NULL; en = en->s_next) {
			debug_sym("", en, "\n");
		}
		debug_indent_dec();
	}
}

void
debug_type(const type_t *tp)
{

	debug_step("type details for '%s':", type_name(tp));
	debug_type_details(tp);
}

void
debug_node(const tnode_t *tn) // NOLINT(misc-no-recursion)
{
	op_t op;

	if (tn == NULL) {
		debug_step("null");
		return;
	}

	op = tn->tn_op;
	debug_print_indent();
	debug_printf("'%s'",
	    op == CVT && !tn->tn_cast ? "convert" : op_name(op));
	if (op == NAME)
		debug_printf(" '%s' with %s",
		    tn->tn_sym->s_name,
		    storage_class_name(tn->tn_sym->s_scl));
	else
		debug_printf(" type");
	debug_printf(" '%s'", type_name(tn->tn_type));
	if (tn->tn_lvalue)
		debug_printf(", lvalue");
	if (tn->tn_parenthesized)
		debug_printf(", parenthesized");
	if (tn->tn_sys)
		debug_printf(", sys");

	switch (op) {
	case NAME:
		debug_printf("\n");
		break;
	case CON:
		if (is_floating(tn->tn_type->t_tspec))
			debug_printf(", value %Lg", tn->tn_val.u.floating);
		else if (is_uinteger(tn->tn_type->t_tspec))
			debug_printf(", value %llu",
			    (unsigned long long)tn->tn_val.u.integer);
		else if (is_integer(tn->tn_type->t_tspec))
			debug_printf(", value %lld",
			    (long long)tn->tn_val.u.integer);
		else {
			lint_assert(tn->tn_type->t_tspec == BOOL);
			debug_printf(", value %s",
			    tn->tn_val.u.integer != 0 ? "true" : "false");
		}
		if (tn->tn_val.v_unsigned_since_c90)
			debug_printf(", unsigned_since_c90");
		if (tn->tn_val.v_char_constant)
			debug_printf(", char_constant");
		debug_printf("\n");
		break;
	case STRING:
		if (tn->tn_string->st_char)
			debug_printf(", length %zu, \"%s\"\n",
			    tn->tn_string->st_len,
			    (const char *)tn->tn_string->st_mem);
		else {
			size_t n = MB_CUR_MAX * (tn->tn_string->st_len + 1);
			char *s = xmalloc(n);
			(void)wcstombs(s, tn->tn_string->st_mem, n);
			debug_printf(", length %zu, L\"%s\"\n",
			    tn->tn_string->st_len, s);
			free(s);
		}
		break;
	default:
		debug_printf("\n");

		debug_indent_inc();
		lint_assert(tn->tn_left != NULL);
		debug_node(tn->tn_left);
		if (op != INCBEF && op != INCAFT
		    && op != DECBEF && op != DECAFT
		    && op != CALL && op != ICALL && op != PUSH)
			lint_assert(is_binary(tn) == (tn->tn_right != NULL));
		if (tn->tn_right != NULL)
			debug_node(tn->tn_right);
		debug_indent_dec();
	}
}

static const char *
def_name(def_t def)
{
	static const char *const name[] = {
		"not-declared",
		"declared",
		"tentative-defined",
		"defined",
	};

	return name[def];
}

const char *
decl_level_kind_name(decl_level_kind kind)
{
	static const char *const name[] = {
		"extern",
		"struct",
		"union",
		"enum",
		"old-style-function-arguments",
		"prototype-parameters",
		"auto",
		"abstract",
	};

	return name[kind];
}

const char *
scl_name(scl_t scl)
{
	static const char *const name[] = {
		"none",
		"extern",
		"static",
		"auto",
		"register",
		"typedef",
		"thread_local",
		"struct",
		"union",
		"enum",
		"member-of-struct",
		"member-of-union",
		"abstract",
		"old-style-function-argument",
		"prototype-argument",
	};

	return name[scl];
}

const char *
symt_name(symt_t kind)
{
	static const char *const name[] = {
		"var-func-type",
		"member",
		"tag",
		"label",
	};

	return name[kind];
}

const char *
type_qualifiers_string(type_qualifiers tq)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "%s%s%s%s",
	    tq.tq_const ? " const" : "",
	    tq.tq_restrict ? " restrict" : "",
	    tq.tq_volatile ? " volatile" : "",
	    tq.tq_atomic ? " atomic" : "");
	return buf[0] != '\0' ? buf + 1 : "none";
}

const char *
function_specifier_name(function_specifier spec)
{
	static const char *const name[] = {
		"inline",
		"_Noreturn",
	};

	return name[spec];
}

static void
debug_word(bool flag, const char *name)
{

	if (flag)
		debug_printf(" %s", name);
}

void
debug_sym(const char *prefix, const sym_t *sym, const char *suffix)
{

	if (suffix[0] == '\n')
		debug_print_indent();
	debug_printf("%s%s", prefix, sym->s_name);
	if (sym->s_type != NULL)
		debug_printf(" type='%s'", type_name(sym->s_type));
	if (sym->s_rename != NULL)
		debug_printf(" rename=%s", sym->s_rename);
	debug_printf(" %s", symt_name(sym->s_kind));
	debug_word(sym->s_keyword != NULL, "keyword");
	debug_word(sym->s_bitfield, "bit-field");
	debug_word(sym->s_set, "set");
	debug_word(sym->s_used, "used");
	debug_word(sym->s_arg, "argument");
	debug_word(sym->s_register, "register");
	debug_word(sym->s_defarg, "old-style-undefined");
	debug_word(sym->s_return_type_implicit_int, "return-int");
	debug_word(sym->s_osdef, "old-style");
	debug_word(sym->s_inline, "inline");
	debug_word(sym->s_ext_sym != NULL, "has-external");
	debug_word(sym->s_scl != NOSCL, scl_name(sym->s_scl));
	debug_word(sym->s_keyword == NULL, def_name(sym->s_def));

	if (sym->s_def_pos.p_file != NULL)
		debug_printf(" defined-at=%s:%d",
		    sym->s_def_pos.p_file, sym->s_def_pos.p_line);
	if (sym->s_set_pos.p_file != NULL)
		debug_printf(" set-at=%s:%d",
		    sym->s_set_pos.p_file, sym->s_set_pos.p_line);
	if (sym->s_use_pos.p_file != NULL)
		debug_printf(" used-at=%s:%d",
		    sym->s_use_pos.p_file, sym->s_use_pos.p_line);

	if (sym->s_type != NULL && sym->s_type->t_is_enum)
		debug_printf(" value=%d", sym->u.s_enum_constant);
	if (sym->s_type != NULL && sym->s_type->t_tspec == BOOL)
		debug_printf(" value=%s",
		    sym->u.s_bool_constant ? "true" : "false");

	if (is_member(sym)) {
		struct_or_union *sou = sym->u.s_member.sm_containing_type;
		const char *tag = sou->sou_tag->s_name;
		const sym_t *def = sou->sou_first_typedef;
		if (tag == unnamed && def != NULL)
			debug_printf(" sou='typedef %s'", def->s_name);
		else
			debug_printf(" sou='%s'", tag);
	}

	if (sym->s_keyword != NULL) {
		int t = sym->u.s_keyword.sk_token;
		if (t == T_TYPE || t == T_STRUCT_OR_UNION)
			debug_printf(" %s",
			    tspec_name(sym->u.s_keyword.u.sk_tspec));
		if (t == T_QUAL)
			debug_printf(" %s", type_qualifiers_string(
			    sym->u.s_keyword.u.sk_type_qualifier));
		if (t == T_FUNCTION_SPECIFIER)
			debug_printf(" %s", function_specifier_name(
			    sym->u.s_keyword.u.function_specifier));
	}

	debug_word(sym->s_osdef && sym->u.s_old_style_args != NULL,
	    "old-style-args");

	debug_printf("%s", suffix);
}

static void
debug_decl_level(const decl_level *dl)
{

	debug_print_indent();
	debug_printf("decl_level: %s", decl_level_kind_name(dl->d_kind));
	if (dl->d_scl != NOSCL)
		debug_printf(" %s", scl_name(dl->d_scl));
	if (dl->d_type != NULL)
		debug_printf(" '%s'", type_name(dl->d_type));
	else {
		if (dl->d_abstract_type != NO_TSPEC)
			debug_printf(" %s", tspec_name(dl->d_abstract_type));
		if (dl->d_complex_mod != NO_TSPEC)
			debug_printf(" %s", tspec_name(dl->d_complex_mod));
		if (dl->d_sign_mod != NO_TSPEC)
			debug_printf(" %s", tspec_name(dl->d_sign_mod));
		if (dl->d_rank_mod != NO_TSPEC)
			debug_printf(" %s", tspec_name(dl->d_rank_mod));
	}
	if (dl->d_redeclared_symbol != NULL)
		debug_sym(" redeclared=(", dl->d_redeclared_symbol, ")");
	if (dl->d_sou_size_in_bits != 0)
		debug_printf(" size=%u", dl->d_sou_size_in_bits);
	if (dl->d_sou_align_in_bits != 0)
		debug_printf(" align=%u", dl->d_sou_align_in_bits);

	debug_word(dl->d_qual.tq_const, "const");
	debug_word(dl->d_qual.tq_restrict, "restrict");
	debug_word(dl->d_qual.tq_volatile, "volatile");
	debug_word(dl->d_qual.tq_atomic, "atomic");
	debug_word(dl->d_inline, "inline");
	debug_word(dl->d_multiple_storage_classes, "multiple_storage_classes");
	debug_word(dl->d_invalid_type_combination, "invalid_type_combination");
	debug_word(dl->d_nonempty_decl, "nonempty_decl");
	debug_word(dl->d_vararg, "vararg");
	debug_word(dl->d_prototype, "prototype");
	debug_word(dl->d_no_type_specifier, "no_type_specifier");
	debug_word(dl->d_asm, "asm");
	debug_word(dl->d_packed, "packed");
	debug_word(dl->d_used, "used");

	if (dl->d_tag_type != NULL)
		debug_printf(" tag_type='%s'", type_name(dl->d_tag_type));
	for (const sym_t *arg = dl->d_func_args;
	     arg != NULL; arg = arg->s_next)
		debug_sym(" arg(", arg, ")");
	if (dl->d_func_def_pos.p_file != NULL)
		debug_printf(" func_def_pos=%s:%d:%d",
		    dl->d_func_def_pos.p_file, dl->d_func_def_pos.p_line,
		    dl->d_func_def_pos.p_uniq);
	for (const sym_t *sym = dl->d_func_proto_syms;
	     sym != NULL; sym = sym->s_next)
		debug_sym(" func_proto_sym(", sym, ")");
	debug_printf("\n");
}

void
debug_dcs(bool all)
{
	int prev_indentation = debug_indentation;
	for (const decl_level *dl = dcs; dl != NULL; dl = dl->d_enclosing) {
		debug_decl_level(dl);
		if (!all)
			return;
		debug_indentation++;
	}
	debug_indentation = prev_indentation;
}
#endif
