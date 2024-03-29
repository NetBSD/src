/*	$NetBSD: func.c,v 1.186 2024/03/29 08:35:32 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *	This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: func.c,v 1.186 2024/03/29 08:35:32 rillig Exp $");
#endif

#include <stdlib.h>
#include <string.h>

#include "lint1.h"
#include "cgram.h"

/*
 * Contains a pointer to the symbol table entry of the current function
 * definition.
 */
sym_t *funcsym;

/* Is set as long as a statement can be reached. Must be set at level 0. */
bool reached = true;

/*
 * Is true by default, can be cleared by NOTREACHED.
 * Is reset to true whenever 'reached' changes.
 */
bool warn_about_unreachable;

/*
 * In conjunction with 'reached', controls printing of "fallthrough on ..."
 * warnings.
 * Reset by each statement and set by FALLTHROUGH, stmt_switch_expr and
 * case_label.
 *
 * The control statements 'if', 'for', 'while' and 'switch' do not reset
 * suppress_fallthrough because this must be done by the controlled statement.
 * At least for 'if', this is important because ** FALLTHROUGH ** after "if
 * (expr) statement" is evaluated before the following token, which causes
 * reduction of above. This means that ** FALLTHROUGH ** after "if ..." would
 * always be ignored.
 */
bool suppress_fallthrough;

/* The innermost control statement */
static control_statement *cstmt;

/*
 * Number of parameters which will be checked for usage in following
 * function definition. -1 stands for all parameters.
 *
 * The position of the last ARGSUSED comment is stored in argsused_pos.
 */
int nargusg = -1;
pos_t argsused_pos;

/*
 * Number of parameters of the following function definition whose types
 * shall be checked by lint2. -1 stands for all parameters.
 *
 * The position of the last VARARGS comment is stored in vapos.
 */
int nvararg = -1;
pos_t vapos;

/*
 * Both printflike_argnum and scanflike_argnum contain the 1-based number
 * of the string parameter which shall be used to check the types of remaining
 * arguments (for PRINTFLIKE and SCANFLIKE).
 *
 * printflike_pos and scanflike_pos are the positions of the last PRINTFLIKE
 * or SCANFLIKE comment.
 */
int printflike_argnum = -1;
int scanflike_argnum = -1;
pos_t printflike_pos;
pos_t scanflike_pos;

/*
 * If both plibflg and llibflg are set, prototypes are written as function
 * definitions to the output file.
 */
bool plibflg;

/* Temporarily suppress warnings about constants in conditional context. */
bool suppress_constcond;

/*
 * Whether a lint library shall be created. The effect of this flag is that
 * all defined symbols are treated as used.
 * (The LINTLIBRARY comment also resets vflag.)
 */
bool llibflg;

/*
 * Determines the warnings that are suppressed by a LINTED directive.  For
 * globally suppressed warnings, see 'msgset'.
 *
 * LWARN_ALL:	all warnings are enabled
 * LWARN_NONE:	all warnings are suppressed
 * n >= 0:	warning n is ignored, the others are active
 */
int lwarn = LWARN_ALL;

/* Temporarily suppress warnings about wrong types for bit-fields. */
bool suppress_bitfieldtype;

/* Temporarily suppress warnings about use of 'long long'. */
bool suppress_longlong;

void
begin_control_statement(control_statement_kind kind)
{
	control_statement *cs;

	cs = xcalloc(1, sizeof(*cs));
	cs->c_kind = kind;
	cs->c_surrounding = cstmt;
	cstmt = cs;
}

void
end_control_statement(control_statement_kind kind)
{
	while (cstmt->c_kind != kind)
		cstmt = cstmt->c_surrounding;

	control_statement *cs = cstmt;
	cstmt = cs->c_surrounding;

	free(cs->c_case_labels.vals);
	free(cs->c_switch_type);
	free(cs);
}

static void
set_reached(bool new_reached)
{
	debug_step("%s -> %s",
	    reached ? "reachable" : "unreachable",
	    new_reached ? "reachable" : "unreachable");
	reached = new_reached;
	warn_about_unreachable = true;
}

/*
 * Prints a warning if a statement cannot be reached.
 */
void
check_statement_reachable(void)
{
	if (!reached && warn_about_unreachable) {
		/* statement not reached */
		warning(193);
		warn_about_unreachable = false;
	}
}

/*
 * Called after a function declaration which introduces a function definition
 * and before an (optional) old-style parameter declaration list.
 *
 * Puts all symbols declared in the prototype or in an old-style parameter
 * list back to the symbol table.
 *
 * Does the usual checking of storage class, type (return value),
 * redeclaration, etc.
 */
void
begin_function(sym_t *fsym)
{
	funcsym = fsym;

	/*
	 * Put all symbols declared in the parameter list back to the symbol
	 * table.
	 */
	for (sym_t *sym = dcs->d_func_proto_syms; sym != NULL;
	    sym = sym->s_level_next) {
		if (sym->s_block_level != -1) {
			lint_assert(sym->s_block_level == 1);
			inssym(1, sym);
		}
	}

	/*
	 * In old_style_function() we did not know whether it is an old style
	 * function definition or only an old-style declaration, if there are
	 * no parameters inside the parameter list ("f()").
	 */
	if (!fsym->s_type->t_proto && fsym->u.s_old_style_params == NULL)
		fsym->s_osdef = true;

	check_type(fsym);

	/*
	 * check_type() checks for almost all possible errors, but not for
	 * incomplete return values (these are allowed in declarations)
	 */
	if (fsym->s_type->t_subt->t_tspec != VOID &&
	    is_incomplete(fsym->s_type->t_subt)) {
		/* cannot return incomplete type */
		error(67);
	}

	fsym->s_def = DEF;

	if (fsym->s_scl == TYPEDEF) {
		fsym->s_scl = EXTERN;
		/* illegal storage class */
		error(8);
	}

	if (dcs->d_inline)
		fsym->s_inline = true;

	/*
	 * Parameters in new-style function declarations need a name. ('void'
	 * is already removed from the list of parameters.)
	 */
	int n = 1;
	for (const sym_t *param = fsym->s_type->u.params;
	    param != NULL; param = param->s_next) {
		if (param->s_scl == ABSTRACT) {
			lint_assert(param->s_name == unnamed);
			/* formal parameter #%d lacks name */
			error(59, n);
		} else {
			lint_assert(param->s_name != unnamed);
		}
		n++;
	}

	/*
	 * We must also remember the position. s_def_pos is overwritten if this
	 * is an old-style definition, and we had already a prototype.
	 */
	dcs->d_func_def_pos = fsym->s_def_pos;

	sym_t *rdsym = dcs->d_redeclared_symbol;
	if (rdsym != NULL) {
		bool dowarn = false;
		if (!check_redeclaration(fsym, &dowarn)) {

			/*
			 * Print nothing if the newly defined function is
			 * defined in old style. A better warning will be
			 * printed in check_func_lint_directives().
			 */
			if (dowarn && !fsym->s_osdef) {
				/* TODO: error in C99 mode as well? */
				if (!allow_trad && !allow_c99)
					/* redeclaration of '%s' */
					error(27, fsym->s_name);
				else
					/* redeclaration of '%s' */
					warning(27, fsym->s_name);
				print_previous_declaration(rdsym);
			}

			copy_usage_info(fsym, rdsym);

			/*
			 * If the old symbol was a prototype and the new one is
			 * none, overtake the position of the declaration of
			 * the prototype.
			 */
			if (fsym->s_osdef && rdsym->s_type->t_proto)
				fsym->s_def_pos = rdsym->s_def_pos;

			complete_type(fsym, rdsym);

			if (rdsym->s_inline)
				fsym->s_inline = true;
		}

		symtab_remove_forever(rdsym);
	}

	if (fsym->s_osdef && !fsym->s_type->t_proto) {
		/* TODO: Make this an error in C99 mode as well. */
		if (!allow_trad && !allow_c99 && hflag &&
		    strcmp(fsym->s_name, "main") != 0)
			/* function definition is not a prototype */
			warning(286);
	}

	if (dcs->d_no_type_specifier)
		fsym->s_return_type_implicit_int = true;

	set_reached(true);
}

static void
check_missing_return_value(void)
{
	if (funcsym->s_type->t_subt->t_tspec == VOID)
		return;
	if (funcsym->s_return_type_implicit_int)
		return;

	/* C99 5.1.2.2.3 "Program termination" p1 */
	if (allow_c99 && strcmp(funcsym->s_name, "main") == 0)
		return;

	/* function '%s' falls off bottom without returning value */
	warning(217, funcsym->s_name);
}

void
end_function(void)
{
	if (reached) {
		cstmt->c_had_return_noval = true;
		check_missing_return_value();
	}

	if (cstmt->c_had_return_noval && cstmt->c_had_return_value &&
	    funcsym->s_return_type_implicit_int)
		/* function '%s' has 'return expr' and 'return' */
		warning(216, funcsym->s_name);

	/* Warn about unused parameters. */
	int n = nargusg;
	nargusg = -1;
	for (const sym_t *param = dcs->d_func_params;
	    param != NULL && n != 0; param = param->s_next, n--)
		check_usage_sym(dcs->d_asm, param);

	if (dcs->d_scl == EXTERN && funcsym->s_inline)
		outsym(funcsym, funcsym->s_scl, DECL);
	else
		outfdef(funcsym, &dcs->d_func_def_pos,
		    cstmt->c_had_return_value, funcsym->s_osdef,
		    dcs->d_func_params);

	/* clean up after syntax errors, see test stmt_for.c. */
	while (dcs->d_enclosing != NULL)
		dcs = dcs->d_enclosing;

	lint_assert(dcs->d_enclosing == NULL);
	lint_assert(dcs->d_kind == DLK_EXTERN);
	symtab_remove_level(dcs->d_func_proto_syms);

	/* must be set on level 0 */
	set_reached(true);

	funcsym = NULL;
}

void
named_label(sym_t *sym)
{

	if (sym->s_set)
		/* label '%s' redefined */
		error(194, sym->s_name);
	else
		mark_as_set(sym);

	/* XXX: Assuming that each label is reachable is wrong. */
	set_reached(true);
}

static void
check_case_label_bitand(const tnode_t *case_expr, const tnode_t *switch_expr)
{
	if (switch_expr->tn_op != BITAND ||
	    switch_expr->u.ops.right->tn_op != CON)
		return;

	lint_assert(case_expr->tn_op == CON);
	uint64_t case_value = (uint64_t)case_expr->u.value.u.integer;
	uint64_t mask = (uint64_t)switch_expr->u.ops.right->u.value.u.integer;

	if ((case_value & ~mask) != 0)
		/* statement not reached */
		warning(193);
}

static void
check_case_label_enum(const tnode_t *tn, const control_statement *cs)
{
	/* similar to typeok_enum in tree.c */

	if (!(tn->tn_type->t_is_enum || cs->c_switch_type->t_is_enum))
		return;
	if (tn->tn_type->t_is_enum && cs->c_switch_type->t_is_enum &&
	    tn->tn_type->u.enumer == cs->c_switch_type->u.enumer)
		return;

#if 0 /* not yet ready, see msg_130.c */
	/* enum type mismatch: '%s' '%s' '%s' */
	warning(130, type_name(cs->c_switch_type), op_name(EQ),
	    type_name(tn->tn_type));
#endif
}

static bool
check_duplicate_case_label(control_statement *cs, const val_t *nv)
{
	case_labels *labels = &cs->c_case_labels;
	size_t i = 0, n = labels->len;

	while (i < n && labels->vals[i].u.integer != nv->u.integer)
		i++;

	if (i < n) {
		if (is_uinteger(nv->v_tspec))
			/* duplicate case '%ju' in switch */
			error(200, (uintmax_t)nv->u.integer);
		else
			/* duplicate case '%jd' in switch */
			error(199, (intmax_t)nv->u.integer);
		return false;
	}

	if (labels->len >= labels->cap) {
		labels->cap = 16 + 2 * labels->cap;
		labels->vals = xrealloc(labels->vals,
		    sizeof(*labels->vals) * labels->cap);
	}
	labels->vals[labels->len++] = *nv;
	return true;
}

static void
check_case_label(tnode_t *tn)
{
	control_statement *cs;
	for (cs = cstmt; cs != NULL && !cs->c_switch; cs = cs->c_surrounding)
		continue;

	if (cs == NULL) {
		/* case not in switch */
		error(195);
		return;
	}

	if (tn == NULL)
		return;

	if (tn->tn_op != CON) {
		/* non-constant case expression */
		error(197);
		return;
	}

	if (!is_integer(tn->tn_type->t_tspec)) {
		/* non-integral case expression */
		error(198);
		return;
	}

	check_case_label_bitand(tn, cs->c_switch_expr);
	check_case_label_enum(tn, cs);

	lint_assert(cs->c_switch_type != NULL);

	if (reached && !suppress_fallthrough) {
		if (hflag)
			/* fallthrough on case statement */
			warning(220);
	}

	tspec_t t = tn->tn_type->t_tspec;
	if ((t == LONG || t == ULONG || t == LLONG || t == ULLONG)
	    && !allow_c90)
		/* case label must be of type 'int' in traditional C */
		warning(203);

	val_t *v = integer_constant(tn, true);
	val_t nv;
	(void)memset(&nv, 0, sizeof(nv));
	convert_constant(CASE, 0, cs->c_switch_type, &nv, v);
	free(v);

	if (check_duplicate_case_label(cs, &nv))
		check_getopt_case_label(nv.u.integer);
}

void
case_label(tnode_t *tn)
{
	check_case_label(tn);
	expr_free_all();
	set_reached(true);
}

void
default_label(void)
{
	/* find the innermost switch statement */
	control_statement *cs;
	for (cs = cstmt; cs != NULL && !cs->c_switch; cs = cs->c_surrounding)
		continue;

	if (cs == NULL)
		/* default outside switch */
		error(201);
	else if (cs->c_default)
		/* duplicate default in switch */
		error(202);
	else {
		if (reached && !suppress_fallthrough) {
			if (hflag)
				/* fallthrough on default statement */
				warning(284);
		}
		cs->c_default = true;
	}

	set_reached(true);
}

static tnode_t *
check_controlling_expression(tnode_t *tn)
{
	tn = cconv(tn);
	if (tn != NULL)
		tn = promote(NOOP, false, tn);

	if (tn != NULL && !is_scalar(tn->tn_type->t_tspec)) {
		/* C99 6.5.15p4 for the ?: operator; see typeok:QUEST */
		/* C99 6.8.4.1p1 for if statements */
		/* C99 6.8.5p2 for while, do and for loops */
		/* controlling expressions must have scalar type */
		error(204);
		return NULL;
	}

	if (tn != NULL && Tflag && !is_typeok_bool_compares_with_zero(tn)) {
		/* controlling expression must be bool, not '%s' */
		error(333, tn->tn_type->t_is_enum ? type_name(tn->tn_type)
		    : tspec_name(tn->tn_type->t_tspec));
	}

	return tn;
}

void
stmt_if_expr(tnode_t *tn)
{
	if (tn != NULL)
		tn = check_controlling_expression(tn);
	if (tn != NULL)
		expr(tn, false, true, false, false);
	begin_control_statement(CS_IF);

	if (tn != NULL && tn->tn_op == CON && !tn->tn_system_dependent) {
		/* XXX: what if inside 'if (0)'? */
		set_reached(constant_is_nonzero(tn));
		/* XXX: what about always_else? */
		cstmt->c_always_then = reached;
	}
}

void
stmt_if_then_stmt(void)
{
	cstmt->c_reached_end_of_then = reached;
	/* XXX: what if inside 'if (0)'? */
	set_reached(!cstmt->c_always_then);
}

void
stmt_if_else_stmt(bool els)
{
	if (cstmt->c_reached_end_of_then)
		set_reached(true);
	else if (cstmt->c_always_then)
		set_reached(false);
	else if (!els)
		set_reached(true);

	end_control_statement(CS_IF);
}

void
stmt_switch_expr(tnode_t *tn)
{
	if (tn != NULL)
		tn = cconv(tn);
	if (tn != NULL)
		tn = promote(NOOP, false, tn);
	if (tn != NULL && !is_integer(tn->tn_type->t_tspec)) {
		/* switch expression must have integral type */
		error(205);
		tn = NULL;
	}
	if (tn != NULL && !allow_c90) {
		tspec_t t = tn->tn_type->t_tspec;
		if (t == LONG || t == ULONG || t == LLONG || t == ULLONG)
			/* switch expression must be of type 'int' in ... */
			warning(271);
	}

	/*
	 * Remember the type of the expression. Because it's possible that
	 * (*tp) is allocated on tree memory, the type must be duplicated. This
	 * is not too complicated because it is only an integer type.
	 */
	type_t *tp = xcalloc(1, sizeof(*tp));
	if (tn != NULL) {
		tp->t_tspec = tn->tn_type->t_tspec;
		if ((tp->t_is_enum = tn->tn_type->t_is_enum) != false)
			tp->u.enumer = tn->tn_type->u.enumer;
	} else {
		tp->t_tspec = INT;
	}

	/* leak the memory, for check_case_label_bitand */
	(void)expr_save_memory();

	check_getopt_begin_switch();
	expr(tn, true, false, false, false);

	begin_control_statement(CS_SWITCH);
	cstmt->c_switch = true;
	cstmt->c_switch_type = tp;
	cstmt->c_switch_expr = tn;

	set_reached(false);
	suppress_fallthrough = true;
}

void
stmt_switch_expr_stmt(void)
{
	int nenum = 0, nclab = 0;
	sym_t *esym;

	lint_assert(cstmt->c_switch_type != NULL);

	if (cstmt->c_switch_type->t_is_enum) {
		/*
		 * Warn if the number of case labels is different from the
		 * number of enumerators.
		 */
		nenum = nclab = 0;
		lint_assert(cstmt->c_switch_type->u.enumer != NULL);
		for (esym = cstmt->c_switch_type->u.enumer->en_first_enumerator;
		    esym != NULL; esym = esym->s_next) {
			nenum++;
		}
		nclab = (int)cstmt->c_case_labels.len;
		if (hflag && eflag && nclab < nenum && !cstmt->c_default)
			/* enumeration value(s) not handled in switch */
			warning(206);
	}

	check_getopt_end_switch();

	if (cstmt->c_break) {
		/*
		 * The end of the switch statement is always reached since
		 * c_break is only set if a break statement can actually be
		 * reached.
		 */
		set_reached(true);
	} else if (cstmt->c_default ||
		   (hflag && cstmt->c_switch_type->t_is_enum &&
		    nenum == nclab)) {
		/*
		 * The end of the switch statement is reached if the end of the
		 * last statement inside it is reached.
		 */
	} else {
		/*
		 * There are possible values that are not handled in the switch
		 * statement.
		 */
		set_reached(true);
	}

	end_control_statement(CS_SWITCH);
}

void
stmt_while_expr(tnode_t *tn)
{
	if (!reached) {
		/* loop not entered at top */
		warning(207);
		/* FIXME: that's plain wrong. */
		set_reached(true);
	}

	if (tn != NULL)
		tn = check_controlling_expression(tn);

	begin_control_statement(CS_WHILE);
	cstmt->c_loop = true;
	cstmt->c_maybe_endless = is_nonzero(tn);
	bool body_reached = !is_zero(tn);

	check_getopt_begin_while(tn);
	expr(tn, false, true, true, false);

	set_reached(body_reached);
}

void
stmt_while_expr_stmt(void)
{
	set_reached(!cstmt->c_maybe_endless || cstmt->c_break);
	check_getopt_end_while();
	end_control_statement(CS_WHILE);
}

void
stmt_do(void)
{
	if (!reached) {
		/* loop not entered at top */
		warning(207);
		set_reached(true);
	}

	begin_control_statement(CS_DO_WHILE);
	cstmt->c_loop = true;
}

void
stmt_do_while_expr(tnode_t *tn)
{
	if (cstmt->c_continue)
		set_reached(true);

	if (tn != NULL)
		tn = check_controlling_expression(tn);

	if (tn != NULL && tn->tn_op == CON) {
		cstmt->c_maybe_endless = constant_is_nonzero(tn);
		if (!cstmt->c_maybe_endless && cstmt->c_continue)
			/* continue in 'do ... while (0)' loop */
			error(323);
	}

	expr(tn, false, true, true, true);

	if (cstmt->c_maybe_endless)
		set_reached(false);
	if (cstmt->c_break)
		set_reached(true);

	end_control_statement(CS_DO_WHILE);
}

void
stmt_for_exprs(tnode_t *tn1, tnode_t *tn2, tnode_t *tn3)
{
	/*
	 * If there is no initialization expression it is possible that it is
	 * intended not to enter the loop at top.
	 */
	if (tn1 != NULL && !reached) {
		/* loop not entered at top */
		warning(207);
		set_reached(true);
	}

	begin_control_statement(CS_FOR);
	cstmt->c_loop = true;

	/*
	 * Store the tree memory for the reinitialization expression. Also
	 * remember this expression itself. We must check it at the end of the
	 * loop to get "used but not set" warnings correct.
	 */
	cstmt->c_for_expr3_mem = expr_save_memory();
	cstmt->c_for_expr3 = tn3;
	cstmt->c_for_expr3_pos = curr_pos;
	cstmt->c_for_expr3_csrc_pos = csrc_pos;

	if (tn1 != NULL)
		expr(tn1, false, false, true, false);

	if (tn2 != NULL)
		tn2 = check_controlling_expression(tn2);
	if (tn2 != NULL)
		expr(tn2, false, true, true, false);

	cstmt->c_maybe_endless = tn2 == NULL || is_nonzero(tn2);

	/* The tn3 expression is checked in stmt_for_exprs_stmt. */

	set_reached(!is_zero(tn2));
}

void
stmt_for_exprs_stmt(void)
{
	if (cstmt->c_continue)
		set_reached(true);

	expr_restore_memory(cstmt->c_for_expr3_mem);
	tnode_t *tn3 = cstmt->c_for_expr3;

	pos_t saved_curr_pos = curr_pos;
	pos_t saved_csrc_pos = csrc_pos;
	curr_pos = cstmt->c_for_expr3_pos;
	csrc_pos = cstmt->c_for_expr3_csrc_pos;

	/* simply "statement not reached" would be confusing */
	if (!reached && warn_about_unreachable) {
		/* end-of-loop code not reached */
		warning(223);
		set_reached(true);
	}

	if (tn3 != NULL)
		expr(tn3, false, false, true, false);
	else
		expr_free_all();

	curr_pos = saved_curr_pos;
	csrc_pos = saved_csrc_pos;

	set_reached(cstmt->c_break || !cstmt->c_maybe_endless);

	end_control_statement(CS_FOR);
}

void
stmt_goto(sym_t *lab)
{
	mark_as_used(lab, false, false);
	check_statement_reachable();
	set_reached(false);
}

void
stmt_break(void)
{
	control_statement *cs = cstmt;
	while (cs != NULL && !cs->c_loop && !cs->c_switch)
		cs = cs->c_surrounding;

	if (cs == NULL)
		/* break outside loop or switch */
		error(208);
	else if (reached)
		cs->c_break = true;

	if (bflag)
		check_statement_reachable();

	set_reached(false);
}

void
stmt_continue(void)
{
	control_statement *cs;

	for (cs = cstmt; cs != NULL && !cs->c_loop; cs = cs->c_surrounding)
		continue;

	if (cs == NULL)
		/* continue outside loop */
		error(209);
	else
		/* TODO: only if reachable, for symmetry with c_break */
		cs->c_continue = true;

	check_statement_reachable();

	set_reached(false);
}

static bool
is_parenthesized(const tnode_t *tn)
{
	while (!tn->tn_parenthesized && tn->tn_op == COMMA)
		tn = tn->u.ops.right;
	return tn->tn_parenthesized && !tn->tn_sys;
}

static void
check_return_value(bool sys, tnode_t *tn)
{
	if (any_query_enabled && is_parenthesized(tn)) {
		/* parenthesized return value */
		query_message(9);
	}

	/* Create a temporary node for the left side */
	tnode_t *ln = expr_zero_alloc(sizeof(*ln), "tnode");
	ln->tn_op = NAME;
	ln->tn_type = expr_unqualified_type(funcsym->s_type->t_subt);
	ln->tn_lvalue = true;
	ln->u.sym = funcsym;	/* better than nothing */

	tnode_t *retn = build_binary(ln, RETURN, sys, tn);

	if (retn != NULL) {
		const tnode_t *rn = retn->u.ops.right;
		while (rn->tn_op == CVT || rn->tn_op == PLUS)
			rn = rn->u.ops.left;
		if (rn->tn_op == ADDR && rn->u.ops.left->tn_op == NAME &&
		    rn->u.ops.left->u.sym->s_scl == AUTO)
			/* '%s' returns pointer to automatic object */
			warning(302, funcsym->s_name);
	}

	expr(retn, true, false, true, false);
}

void
stmt_return(bool sys, tnode_t *tn)
{
	control_statement *cs = cstmt;

	if (cs == NULL) {
		/* syntax error '%s' */
		error(249, "return outside function");
		return;
	}

	for (; cs->c_surrounding != NULL; cs = cs->c_surrounding)
		continue;

	if (tn != NULL)
		cs->c_had_return_value = true;
	else
		cs->c_had_return_noval = true;

	if (tn != NULL && funcsym->s_type->t_subt->t_tspec == VOID) {
		/* void function '%s' cannot return value */
		error(213, funcsym->s_name);
		expr_free_all();
		tn = NULL;
	}
	if (tn == NULL && funcsym->s_type->t_subt->t_tspec != VOID
	    && !funcsym->s_return_type_implicit_int) {
		if (allow_c99)
			/* function '%s' expects to return value */
			error(214, funcsym->s_name);
		else
			/* function '%s' expects to return value */
			warning(214, funcsym->s_name);
	}

	if (tn != NULL)
		check_return_value(sys, tn);
	else
		check_statement_reachable();

	set_reached(false);
}

void
global_clean_up_decl(bool silent)
{
	if (nargusg != -1) {
		if (!silent)
			/* comment ** %s ** must precede function definition */
			warning_at(282, &argsused_pos, "ARGSUSED");
		nargusg = -1;
	}
	if (nvararg != -1) {
		if (!silent)
			/* comment ** %s ** must precede function definition */
			warning_at(282, &vapos, "VARARGS");
		nvararg = -1;
	}
	if (printflike_argnum != -1) {
		if (!silent)
			/* comment ** %s ** must precede function definition */
			warning_at(282, &printflike_pos, "PRINTFLIKE");
		printflike_argnum = -1;
	}
	if (scanflike_argnum != -1) {
		if (!silent)
			/* comment ** %s ** must precede function definition */
			warning_at(282, &scanflike_pos, "SCANFLIKE");
		scanflike_argnum = -1;
	}

	dcs->d_asm = false;

	/*
	 * Needed for BSD yacc in case of parse errors; GNU Bison 3.0.4 is
	 * fine.  See test gcc_attribute.c, function_with_unknown_attribute.
	 */
	in_gcc_attribute = false;
	while (dcs->d_enclosing != NULL)
		end_declaration_level();
}

/*
 * Only the first n parameters of the following function are checked for usage.
 * A missing argument is taken to be 0.
 */
static void
argsused(int n)
{
	if (dcs->d_kind != DLK_EXTERN) {
		/* comment ** %s ** must be outside function */
		warning(280, "ARGSUSED");
		return;
	}
	if (nargusg != -1)
		/* duplicate comment ** %s ** */
		warning(281, "ARGSUSED");
	nargusg = n != -1 ? n : 0;
	argsused_pos = curr_pos;
}

static void
varargs(int n)
{
	if (dcs->d_kind != DLK_EXTERN) {
		/* comment ** %s ** must be outside function */
		warning(280, "VARARGS");
		return;
	}
	if (nvararg != -1)
		/* duplicate comment ** %s ** */
		warning(281, "VARARGS");
	nvararg = n != -1 ? n : 0;
	vapos = curr_pos;
}

/*
 * Check all parameters until the (n-1)-th as usual. The n-th argument is
 * used to check the types of the remaining arguments.
 */
static void
printflike(int n)
{
	if (dcs->d_kind != DLK_EXTERN) {
		/* comment ** %s ** must be outside function */
		warning(280, "PRINTFLIKE");
		return;
	}
	if (printflike_argnum != -1)
		/* duplicate comment ** %s ** */
		warning(281, "PRINTFLIKE");
	printflike_argnum = n != -1 ? n : 0;
	printflike_pos = curr_pos;
}

/*
 * Check all parameters until the (n-1)-th as usual. The n-th argument is
 * used the check the types of remaining arguments.
 */
static void
scanflike(int n)
{
	if (dcs->d_kind != DLK_EXTERN) {
		/* comment ** %s ** must be outside function */
		warning(280, "SCANFLIKE");
		return;
	}
	if (scanflike_argnum != -1)
		/* duplicate comment ** %s ** */
		warning(281, "SCANFLIKE");
	scanflike_argnum = n != -1 ? n : 0;
	scanflike_pos = curr_pos;
}

static void
lintlib(void)
{
	if (dcs->d_kind != DLK_EXTERN) {
		/* comment ** %s ** must be outside function */
		warning(280, "LINTLIBRARY");
		return;
	}
	llibflg = true;
	vflag = true;
}

/*
 * PROTOLIB in conjunction with LINTLIBRARY can be used to handle
 * prototypes like function definitions. This is done if the argument
 * to PROTOLIB is nonzero. Otherwise, prototypes are handled normally.
 */
static void
protolib(int n)
{
	if (dcs->d_kind != DLK_EXTERN) {
		/* comment ** %s ** must be outside function */
		warning(280, "PROTOLIB");
		return;
	}
	plibflg = n != 0;
}

void
handle_lint_comment(lint_comment comment, int arg)
{
	switch (comment) {
	case LC_ARGSUSED:	argsused(arg);			break;
	case LC_BITFIELDTYPE:	suppress_bitfieldtype = true;	break;
	case LC_CONSTCOND:	suppress_constcond = true;	break;
	case LC_FALLTHROUGH:	suppress_fallthrough = true;	break;
	case LC_LINTLIBRARY:	lintlib();			break;
	case LC_LINTED:		debug_step("set lwarn %d", arg);
				lwarn = arg;			break;
	case LC_LONGLONG:	suppress_longlong = true;	break;
	case LC_NOTREACHED:	set_reached(false);
				warn_about_unreachable = false;	break;
	case LC_PRINTFLIKE:	printflike(arg);		break;
	case LC_PROTOLIB:	protolib(arg);			break;
	case LC_SCANFLIKE:	scanflike(arg);			break;
	case LC_VARARGS:	varargs(arg);			break;
	}
}
