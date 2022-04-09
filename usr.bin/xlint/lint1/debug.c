/* $NetBSD: debug.c,v 1.15 2022/04/09 16:02:14 rillig Exp $ */

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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: debug.c,v 1.15 2022/04/09 16:02:14 rillig Exp $");
#endif

#include <stdlib.h>

#include "lint1.h"
#include "cgram.h"


#ifdef DEBUG

static int debug_indentation = 0;


void __printflike(1, 2)
debug_printf(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	(void)vfprintf(stdout, fmt, va);
	va_end(va);
}

void
debug_print_indent(void)
{

	debug_printf("%*s", 2 * debug_indentation, "");
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
(debug_enter)(const char *func)
{

	printf("%*s+ %s\n", 2 * debug_indentation++, "", func);
}

void __printflike(1, 2)
debug_step(const char *fmt, ...)
{
	va_list va;

	debug_print_indent();
	va_start(va, fmt);
	(void)vfprintf(stdout, fmt, va);
	va_end(va);
	printf("\n");
}

void
(debug_leave)(const char *func)
{

	printf("%*s- %s\n", 2 * --debug_indentation, "", func);
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
	debug_printf("'%s' with type '%s'%s%s%s",
	    op == CVT && !tn->tn_cast ? "convert" : modtab[op].m_name,
	    type_name(tn->tn_type), tn->tn_lvalue ? ", lvalue" : "",
	    tn->tn_parenthesized ? ", parenthesized" : "",
	    tn->tn_sys ? ", sys" : "");

	if (op == NAME)
		debug_printf(" %s %s\n", tn->tn_sym->s_name,
		    storage_class_name(tn->tn_sym->s_scl));
	else if (op == CON && is_floating(tn->tn_type->t_tspec))
		debug_printf(", value %Lg", tn->tn_val->v_ldbl);
	else if (op == CON && is_uinteger(tn->tn_type->t_tspec))
		debug_printf(", value %llu\n",
		    (unsigned long long)tn->tn_val->v_quad);
	else if (op == CON && is_integer(tn->tn_type->t_tspec))
		debug_printf(", value %lld\n",
		    (long long)tn->tn_val->v_quad);
	else if (op == CON && tn->tn_type->t_tspec == BOOL)
		debug_printf(", value %s\n",
		    tn->tn_val->v_quad != 0 ? "true" : "false");
	else if (op == CON)
		debug_printf(", unknown value\n");
	else if (op == STRING && tn->tn_string->st_char)
		debug_printf(", length %zu, \"%s\"\n",
		    tn->tn_string->st_len,
		    (const char *)tn->tn_string->st_mem);
	else if (op == STRING) {
		size_t n = MB_CUR_MAX * (tn->tn_string->st_len + 1);
		char *s = xmalloc(n);
		(void)wcstombs(s, tn->tn_string->st_mem, n);
		debug_printf(", length %zu, L\"%s\"",
		    tn->tn_string->st_len, s);
		free(s);

	} else {
		debug_printf("\n");

		debug_indent_inc();
		debug_node(tn->tn_left);
		if (is_binary(tn) || tn->tn_right != NULL)
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
scl_name(scl_t scl)
{
	static const char *const name[] = {
		"none",
		"extern",
		"static",
		"auto",
		"register",
		"typedef",
		"struct",
		"union",
		"enum",
		"member-of-struct",
		"member-of-union",
		"bool-constant",
		"enum-constant",
		"abstract",
		"old-style-function-argument",
		"prototype-argument",
		"inline",
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
tqual_name(tqual_t qual)
{
	static const char *const name[] = {
		"const",
		"volatile",
		"restrict",
		"_Thread_local",
	};

	return name[qual];
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

	if (is_member(sym) && sym->u.s_member.sm_sou_type != NULL) {
		struct_or_union *sou_type = sym->u.s_member.sm_sou_type;
		const char *tag = sou_type->sou_tag->s_name;
		const sym_t *def = sou_type->sou_first_typedef;
		if (tag == unnamed && def != NULL)
			debug_printf(" sou='typedef %s'", def->s_name);
		else
			debug_printf(" sou=%s", tag);
	}

	if (sym->s_keyword != NULL) {
		int t = sym->u.s_keyword.sk_token;
		if (t == T_TYPE || t == T_STRUCT_OR_UNION)
			debug_printf(" %s",
			    tspec_name(sym->u.s_keyword.sk_tspec));
		else if (t == T_QUAL)
			debug_printf(" %s",
			    tqual_name(sym->u.s_keyword.sk_qualifier));
	}

	debug_word(sym->s_osdef && sym->u.s_old_style_args != NULL,
	    "old-style-args");

	debug_printf("%s", suffix);
}

void
debug_dinfo(const dinfo_t *d) // NOLINT(misc-no-recursion)
{

	debug_print_indent();
	debug_printf("dinfo: %s", scl_name(d->d_ctx));
	if (d->d_scl != NOSCL)
		debug_printf(" %s", scl_name(d->d_scl));
	if (d->d_type != NULL) {
		debug_printf(" '%s'", type_name(d->d_type));
	} else {
		if (d->d_abstract_type != NOTSPEC)
			debug_printf(" %s", tspec_name(d->d_abstract_type));
		if (d->d_complex_mod != NOTSPEC)
			debug_printf(" %s", tspec_name(d->d_complex_mod));
		if (d->d_sign_mod != NOTSPEC)
			debug_printf(" %s", tspec_name(d->d_sign_mod));
		if (d->d_rank_mod != NOTSPEC)
			debug_printf(" %s", tspec_name(d->d_rank_mod));
	}
	if (d->d_redeclared_symbol != NULL)
		debug_sym(" redeclared=(", d->d_redeclared_symbol, ")");
	if (d->d_offset != 0)
		debug_printf(" offset=%u", d->d_offset);
	if (d->d_sou_align_in_bits != 0)
		debug_printf(" align=%u", (unsigned)d->d_sou_align_in_bits);

	if (d->d_const)
		debug_printf(" const");
	if (d->d_volatile)
		debug_printf(" volatile");
	if (d->d_inline)
		debug_printf(" inline");
	if (d->d_multiple_storage_classes)
		debug_printf(" multiple_storage_classes");
	if (d->d_invalid_type_combination)
		debug_printf(" invalid_type_combination");
	if (d->d_nonempty_decl)
		debug_printf(" nonempty_decl");
	if (d->d_vararg)
		debug_printf(" vararg");
	if (d->d_proto)
		debug_printf(" prototype");
	if (d->d_notyp)
		debug_printf(" no_type_specifier");
	if (d->d_asm)
		debug_printf(" asm");
	if (d->d_packed)
		debug_printf(" packed");
	if (d->d_used)
		debug_printf(" used");

	if (d->d_tagtyp != NULL)
		debug_printf(" tagtyp='%s'", type_name(d->d_tagtyp));
	for (const sym_t *arg = d->d_func_args;
	     arg != NULL; arg = arg->s_next)
		debug_sym(" arg(", arg, ")");
	if (d->d_func_def_pos.p_file != NULL)
		debug_printf(" func_def_pos=%s:%d:%d",
		    d->d_func_def_pos.p_file, d->d_func_def_pos.p_line,
		    d->d_func_def_pos.p_uniq);
	for (const sym_t *sym = d->d_func_proto_syms;
	     sym != NULL; sym = sym->s_next)
		debug_sym("func_proto_sym(", sym, ")");
	debug_printf("\n");

	if (d->d_enclosing != NULL) {
		debug_indent_inc();
		debug_dinfo(d->d_enclosing);
		debug_indent_dec();
	}
}
#endif
