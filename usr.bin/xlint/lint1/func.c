/*	$NetBSD: func.c,v 1.130 2022/04/02 20:12:46 rillig Exp $	*/

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
 *      This product includes software developed by Jochen Pohl for
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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: func.c,v 1.130 2022/04/02 20:12:46 rillig Exp $");
#endif

#include <stdlib.h>
#include <string.h>

#include "lint1.h"
#include "cgram.h"

/*
 * Contains a pointer to the symbol table entry of the current function
 * definition.
 */
sym_t	*funcsym;

/* Is set as long as a statement can be reached. Must be set at level 0. */
bool	reached = true;

/*
 * Is true by default, can be cleared by NOTREACHED.
 * Is reset to true whenever 'reached' changes.
 */
bool	warn_about_unreachable;

/*
 * In conjunction with 'reached', controls printing of "fallthrough on ..."
 * warnings.
 * Reset by each statement and set by FALLTHROUGH, switch (switch1())
 * and case (label()).
 *
 * Control statements if, for, while and switch do not reset seen_fallthrough
 * because this must be done by the controlled statement. At least for if this
 * is important because ** FALLTHROUGH ** after "if (expr) statement" is
 * evaluated before the following token, which causes reduction of above.
 * This means that ** FALLTHROUGH ** after "if ..." would always be ignored.
 */
bool	seen_fallthrough;

/* The innermost control statement */
control_statement *cstmt;

/*
 * Number of arguments which will be checked for usage in following
 * function definition. -1 stands for all arguments.
 *
 * The position of the last ARGSUSED comment is stored in argsused_pos.
 */
int	nargusg = -1;
pos_t	argsused_pos;

/*
 * Number of arguments of the following function definition whose types
 * shall be checked by lint2. -1 stands for all arguments.
 *
 * The position of the last VARARGS comment is stored in vapos.
 */
int	nvararg = -1;
pos_t	vapos;

/*
 * Both printflike_argnum and scanflike_argnum contain the 1-based number
 * of the string argument which shall be used to check the types of remaining
 * arguments (for PRINTFLIKE and SCANFLIKE).
 *
 * printflike_pos and scanflike_pos are the positions of the last PRINTFLIKE
 * or SCANFLIKE comment.
 */
int	printflike_argnum = -1;
int	scanflike_argnum = -1;
pos_t	printflike_pos;
pos_t	scanflike_pos;

/*
 * If both plibflg and llibflg are set, prototypes are written as function
 * definitions to the output file.
 */
bool	plibflg;

/*
 * True means that no warnings about constants in conditional
 * context are printed.
 */
bool	constcond_flag;

/*
 * llibflg is set if a lint library shall be created. The effect of
 * llibflg is that all defined symbols are treated as used.
 * (The LINTLIBRARY comment also resets vflag.)
 */
bool	llibflg;

/*
 * Nonzero if warnings are suppressed by a LINTED directive
 * LWARN_BAD:	error
 * LWARN_ALL:	warnings on
 * LWARN_NONE:	all warnings ignored
 * 0..n: warning n ignored
 */
int	lwarn = LWARN_ALL;

/*
 * Whether bitfield type errors are suppressed by a BITFIELDTYPE
 * directive.
 */
bool	bitfieldtype_ok;

/*
 * Whether complaints about use of "long long" are suppressed in
 * the next statement or declaration.
 */
bool	quadflg;

/*
 * Puts a new element at the top of the stack used for control statements.
 */
void
begin_control_statement(control_statement_kind kind)
{
	control_statement *cs;

	cs = xcalloc(1, sizeof(*cs));
	cs->c_kind = kind;
	cs->c_surrounding = cstmt;
	cstmt = cs;
}

/*
 * Removes the top element of the stack used for control statements.
 */
void
end_control_statement(control_statement_kind kind)
{
	control_statement *cs;
	case_label_t *cl, *next;

	lint_assert(cstmt != NULL);

	while (cstmt->c_kind != kind)
		cstmt = cstmt->c_surrounding;

	cs = cstmt;
	cstmt = cs->c_surrounding;

	for (cl = cs->c_case_labels; cl != NULL; cl = next) {
		next = cl->cl_next;
		free(cl);
	}

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
 * and before an (optional) old style argument declaration list.
 *
 * Puts all symbols declared in the prototype or in an old style argument
 * list back to the symbol table.
 *
 * Does the usual checking of storage class, type (return value),
 * redeclaration, etc.
 */
void
funcdef(sym_t *fsym)
{
	int	n;
	bool	dowarn;
	sym_t	*arg, *sym, *rdsym;

	funcsym = fsym;

	/*
	 * Put all symbols declared in the argument list back to the
	 * symbol table.
	 */
	for (sym = dcs->d_func_proto_syms; sym != NULL;
	    sym = sym->s_level_next) {
		if (sym->s_block_level != -1) {
			lint_assert(sym->s_block_level == 1);
			inssym(1, sym);
		}
	}

	/*
	 * In old_style_function() we did not know whether it is an old
	 * style function definition or only an old style declaration,
	 * if there are no arguments inside the argument list ("f()").
	 */
	if (!fsym->s_type->t_proto && fsym->s_args == NULL)
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
	 * Arguments in new style function declarations need a name.
	 * (void is already removed from the list of arguments)
	 */
	n = 1;
	for (arg = fsym->s_type->t_args; arg != NULL; arg = arg->s_next) {
		if (arg->s_scl == ABSTRACT) {
			lint_assert(arg->s_name == unnamed);
			/* formal parameter lacks name: param #%d */
			error(59, n);
		} else {
			lint_assert(arg->s_name != unnamed);
		}
		n++;
	}

	/*
	 * We must also remember the position. s_def_pos is overwritten
	 * if this is an old style definition and we had already a
	 * prototype.
	 */
	dcs->d_func_def_pos = fsym->s_def_pos;

	if ((rdsym = dcs->d_redeclared_symbol) != NULL) {

		if (!check_redeclaration(fsym, (dowarn = false, &dowarn))) {

			/*
			 * Print nothing if the newly defined function
			 * is defined in old style. A better warning will
			 * be printed in check_func_lint_directives().
			 */
			if (dowarn && !fsym->s_osdef) {
				if (sflag)
					/* redeclaration of %s */
					error(27, fsym->s_name);
				else
					/* redeclaration of %s */
					warning(27, fsym->s_name);
				print_previous_declaration(-1, rdsym);
			}

			copy_usage_info(fsym, rdsym);

			/*
			 * If the old symbol was a prototype and the new
			 * one is none, overtake the position of the
			 * declaration of the prototype.
			 */
			if (fsym->s_osdef && rdsym->s_type->t_proto)
				fsym->s_def_pos = rdsym->s_def_pos;

			complete_type(fsym, rdsym);

			if (rdsym->s_inline)
				fsym->s_inline = true;

		}

		/* remove the old symbol from the symbol table */
		rmsym(rdsym);

	}

	if (fsym->s_osdef && !fsym->s_type->t_proto) {
		if (sflag && hflag && strcmp(fsym->s_name, "main") != 0)
			/* function definition is not a prototype */
			warning(286);
	}

	if (dcs->d_notyp)
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
	if (Sflag && strcmp(funcsym->s_name, "main") == 0)
		return;

	/* function %s falls off bottom without returning value */
	warning(217, funcsym->s_name);
}

/*
 * Called at the end of a function definition.
 */
void
funcend(void)
{
	sym_t	*arg;
	int	n;

	if (reached) {
		cstmt->c_had_return_noval = true;
		check_missing_return_value();
	}

	/*
	 * This warning is printed only if the return value was implicitly
	 * declared to be int. Otherwise the wrong return statement
	 * has already printed a warning.
	 */
	if (cstmt->c_had_return_noval && cstmt->c_had_return_value &&
	    funcsym->s_return_type_implicit_int)
		/* function %s has return (e); and return; */
		warning(216, funcsym->s_name);

	/* Print warnings for unused arguments */
	arg = dcs->d_func_args;
	n = 0;
	while (arg != NULL && (nargusg == -1 || n < nargusg)) {
		check_usage_sym(dcs->d_asm, arg);
		arg = arg->s_next;
		n++;
	}
	nargusg = -1;

	/*
	 * write the information about the function definition to the
	 * output file
	 * inline functions explicitly declared extern are written as
	 * declarations only.
	 */
	if (dcs->d_scl == EXTERN && funcsym->s_inline) {
		outsym(funcsym, funcsym->s_scl, DECL);
	} else {
		outfdef(funcsym, &dcs->d_func_def_pos,
		    cstmt->c_had_return_value, funcsym->s_osdef,
		    dcs->d_func_args);
	}

	/* clean up after syntax errors, see test stmt_for.c. */
	while (dcs->d_enclosing != NULL)
		dcs = dcs->d_enclosing;

	/*
	 * remove all symbols declared during argument declaration from
	 * the symbol table
	 */
	lint_assert(dcs->d_enclosing == NULL);
	lint_assert(dcs->d_ctx == EXTERN);
	rmsyms(dcs->d_func_proto_syms);

	/* must be set on level 0 */
	set_reached(true);
}

void
named_label(sym_t *sym)
{

	if (sym->s_set) {
		/* label %s redefined */
		error(194, sym->s_name);
	} else {
		mark_as_set(sym);
	}

	set_reached(true);
}

static void
check_case_label_bitand(const tnode_t *case_expr, const tnode_t *switch_expr)
{
	uint64_t case_value, mask;

	if (switch_expr->tn_op != BITAND ||
	    switch_expr->tn_right->tn_op != CON)
		return;

	lint_assert(case_expr->tn_op == CON);
	case_value = case_expr->tn_val->v_quad;
	mask = switch_expr->tn_right->tn_val->v_quad;

	if ((case_value & ~mask) != 0) {
		/* statement not reached */
		warning(193);
	}
}

static void
check_case_label_enum(const tnode_t *tn, const control_statement *cs)
{
	/* similar to typeok_enum in tree.c */

	if (!(tn->tn_type->t_is_enum || cs->c_switch_type->t_is_enum))
		return;
	if (tn->tn_type->t_is_enum && cs->c_switch_type->t_is_enum &&
	    tn->tn_type->t_enum == cs->c_switch_type->t_enum)
		return;

#if 0 /* not yet ready, see msg_130.c */
	/* enum type mismatch: '%s' '%s' '%s' */
	warning(130, type_name(cs->c_switch_type), op_name(EQ),
	    type_name(tn->tn_type));
#endif
}

static void
check_case_label(tnode_t *tn, control_statement *cs)
{
	case_label_t *cl;
	val_t	*v;
	val_t	nv;
	tspec_t	t;

	if (cs == NULL) {
		/* case not in switch */
		error(195);
		return;
	}

	if (tn != NULL && tn->tn_op != CON) {
		/* non-constant case expression */
		error(197);
		return;
	}

	if (tn != NULL && !is_integer(tn->tn_type->t_tspec)) {
		/* non-integral case expression */
		error(198);
		return;
	}

	check_case_label_bitand(tn, cs->c_switch_expr);
	check_case_label_enum(tn, cs);

	lint_assert(cs->c_switch_type != NULL);

	if (reached && !seen_fallthrough) {
		if (hflag)
			/* fallthrough on case statement */
			warning(220);
	}

	t = tn->tn_type->t_tspec;
	if (t == LONG || t == ULONG ||
	    t == QUAD || t == UQUAD) {
		if (tflag)
			/* case label must be of type 'int' in traditional C */
			warning(203);
	}

	/*
	 * get the value of the expression and convert it
	 * to the type of the switch expression
	 */
	v = constant(tn, true);
	(void)memset(&nv, 0, sizeof(nv));
	convert_constant(CASE, 0, cs->c_switch_type, &nv, v);
	free(v);

	/* look if we had this value already */
	for (cl = cs->c_case_labels; cl != NULL; cl = cl->cl_next) {
		if (cl->cl_val.v_quad == nv.v_quad)
			break;
	}
	if (cl != NULL && is_uinteger(nv.v_tspec)) {
		/* duplicate case in switch: %lu */
		error(200, (unsigned long)nv.v_quad);
	} else if (cl != NULL) {
		/* duplicate case in switch: %ld */
		error(199, (long)nv.v_quad);
	} else {
		check_getopt_case_label(nv.v_quad);

		/* append the value to the list of case values */
		cl = xcalloc(1, sizeof(*cl));
		cl->cl_val = nv;
		cl->cl_next = cs->c_case_labels;
		cs->c_case_labels = cl;
	}
}

void
case_label(tnode_t *tn)
{
	control_statement *cs;

	/* find the innermost switch statement */
	for (cs = cstmt; cs != NULL && !cs->c_switch; cs = cs->c_surrounding)
		continue;

	check_case_label(tn, cs);

	expr_free_all();

	set_reached(true);
}

void
default_label(void)
{
	control_statement *cs;

	/* find the innermost switch statement */
	for (cs = cstmt; cs != NULL && !cs->c_switch; cs = cs->c_surrounding)
		continue;

	if (cs == NULL) {
		/* default outside switch */
		error(201);
	} else if (cs->c_default) {
		/* duplicate default in switch */
		error(202);
	} else {
		if (reached && !seen_fallthrough) {
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

	if (tn != NULL && Tflag && !is_typeok_bool_operand(tn)) {
		/* controlling expression must be bool, not '%s' */
		error(333, tspec_name(tn->tn_type->t_tspec));
	}

	return tn;
}

/*
 * T_IF T_LPAREN expr T_RPAREN
 */
void
if1(tnode_t *tn)
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

/*
 * if_without_else
 * if_without_else T_ELSE
 */
void
if2(void)
{

	cstmt->c_reached_end_of_then = reached;
	/* XXX: what if inside 'if (0)'? */
	set_reached(!cstmt->c_always_then);
}

/*
 * if_without_else
 * if_without_else T_ELSE statement
 */
void
if3(bool els)
{
	if (cstmt->c_reached_end_of_then)
		set_reached(true);
	else if (cstmt->c_always_then)
		set_reached(false);
	else if (!els)
		set_reached(true);

	end_control_statement(CS_IF);
}

/*
 * T_SWITCH T_LPAREN expr T_RPAREN
 */
void
switch1(tnode_t *tn)
{
	tspec_t	t;
	type_t	*tp;

	if (tn != NULL)
		tn = cconv(tn);
	if (tn != NULL)
		tn = promote(NOOP, false, tn);
	if (tn != NULL && !is_integer(tn->tn_type->t_tspec)) {
		/* switch expression must have integral type */
		error(205);
		tn = NULL;
	}
	if (tn != NULL && tflag) {
		t = tn->tn_type->t_tspec;
		if (t == LONG || t == ULONG || t == QUAD || t == UQUAD) {
			/* switch expression must be of type 'int' in ... */
			warning(271);
		}
	}

	/*
	 * Remember the type of the expression. Because it's possible
	 * that (*tp) is allocated on tree memory, the type must be
	 * duplicated. This is not too complicated because it is
	 * only an integer type.
	 */
	tp = xcalloc(1, sizeof(*tp));
	if (tn != NULL) {
		tp->t_tspec = tn->tn_type->t_tspec;
		if ((tp->t_is_enum = tn->tn_type->t_is_enum) != false)
			tp->t_enum = tn->tn_type->t_enum;
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
	seen_fallthrough = true;
}

/*
 * switch_expr statement
 */
void
switch2(void)
{
	int	nenum = 0, nclab = 0;
	sym_t	*esym;
	case_label_t *cl;

	lint_assert(cstmt->c_switch_type != NULL);

	if (cstmt->c_switch_type->t_is_enum) {
		/*
		 * Warn if the number of case labels is different from the
		 * number of enumerators.
		 */
		nenum = nclab = 0;
		lint_assert(cstmt->c_switch_type->t_enum != NULL);
		for (esym = cstmt->c_switch_type->t_enum->en_first_enumerator;
		     esym != NULL; esym = esym->s_next) {
			nenum++;
		}
		for (cl = cstmt->c_case_labels; cl != NULL; cl = cl->cl_next)
			nclab++;
		if (hflag && eflag && nenum != nclab && !cstmt->c_default) {
			/* enumeration value(s) not handled in switch */
			warning(206);
		}
	}

	check_getopt_end_switch();

	if (cstmt->c_break) {
		/*
		 * The end of the switch statement is always reached since
		 * c_break is only set if a break statement can actually
		 * be reached.
		 */
		set_reached(true);
	} else if (cstmt->c_default ||
		   (hflag && cstmt->c_switch_type->t_is_enum &&
		    nenum == nclab)) {
		/*
		 * The end of the switch statement is reached if the end
		 * of the last statement inside it is reached.
		 */
	} else {
		/*
		 * There are possible values that are not handled in the
		 * switch statement.
		 */
		set_reached(true);
	}

	end_control_statement(CS_SWITCH);
}

/*
 * T_WHILE T_LPAREN expr T_RPAREN
 */
void
while1(tnode_t *tn)
{
	bool body_reached;

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
	body_reached = !is_zero(tn);

	check_getopt_begin_while(tn);
	expr(tn, false, true, true, false);

	set_reached(body_reached);
}

/*
 * while_expr statement
 * while_expr error
 */
void
while2(void)
{

	/*
	 * The end of the loop can be reached if it is no endless loop
	 * or there was a break statement which was reached.
	 */
	set_reached(!cstmt->c_maybe_endless || cstmt->c_break);

	check_getopt_end_while();
	end_control_statement(CS_WHILE);
}

/*
 * T_DO
 */
void
do1(void)
{

	if (!reached) {
		/* loop not entered at top */
		warning(207);
		set_reached(true);
	}

	begin_control_statement(CS_DO_WHILE);
	cstmt->c_loop = true;
}

/*
 * do statement do_while_expr
 * do error
 */
void
do2(tnode_t *tn)
{

	/*
	 * If there was a continue statement, the expression controlling the
	 * loop is reached.
	 */
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

/*
 * T_FOR T_LPAREN opt_expr T_SEMI opt_expr T_SEMI opt_expr T_RPAREN
 */
void
for1(tnode_t *tn1, tnode_t *tn2, tnode_t *tn3)
{

	/*
	 * If there is no initialization expression it is possible that
	 * it is intended not to enter the loop at top.
	 */
	if (tn1 != NULL && !reached) {
		/* loop not entered at top */
		warning(207);
		set_reached(true);
	}

	begin_control_statement(CS_FOR);
	cstmt->c_loop = true;

	/*
	 * Store the tree memory for the reinitialization expression.
	 * Also remember this expression itself. We must check it at
	 * the end of the loop to get "used but not set" warnings correct.
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

	/* Checking the reinitialization expression is done in for2() */

	set_reached(!is_zero(tn2));
}

/*
 * for_exprs statement
 * for_exprs error
 */
void
for2(void)
{
	pos_t	cpos, cspos;
	tnode_t	*tn3;

	if (cstmt->c_continue)
		set_reached(true);

	cpos = curr_pos;
	cspos = csrc_pos;

	/* Restore the tree memory for the reinitialization expression */
	expr_restore_memory(cstmt->c_for_expr3_mem);
	tn3 = cstmt->c_for_expr3;
	curr_pos = cstmt->c_for_expr3_pos;
	csrc_pos = cstmt->c_for_expr3_csrc_pos;

	/* simply "statement not reached" would be confusing */
	if (!reached && warn_about_unreachable) {
		/* end-of-loop code not reached */
		warning(223);
		set_reached(true);
	}

	if (tn3 != NULL) {
		expr(tn3, false, false, true, false);
	} else {
		expr_free_all();
	}

	curr_pos = cpos;
	csrc_pos = cspos;

	/* An endless loop without break will never terminate */
	/* TODO: What if the loop contains a 'return'? */
	set_reached(cstmt->c_break || !cstmt->c_maybe_endless);

	end_control_statement(CS_FOR);
}

/*
 * T_GOTO identifier T_SEMI
 */
void
do_goto(sym_t *lab)
{

	mark_as_used(lab, false, false);

	check_statement_reachable();

	set_reached(false);
}

/*
 * T_BREAK T_SEMI
 */
void
do_break(void)
{
	control_statement *cs;

	cs = cstmt;
	while (cs != NULL && !cs->c_loop && !cs->c_switch)
		cs = cs->c_surrounding;

	if (cs == NULL) {
		/* break outside loop or switch */
		error(208);
	} else {
		if (reached)
			cs->c_break = true;
	}

	if (bflag)
		check_statement_reachable();

	set_reached(false);
}

/*
 * T_CONTINUE T_SEMI
 */
void
do_continue(void)
{
	control_statement *cs;

	for (cs = cstmt; cs != NULL && !cs->c_loop; cs = cs->c_surrounding)
		continue;

	if (cs == NULL) {
		/* continue outside loop */
		error(209);
	} else {
		/* TODO: only if reachable, for symmetry with c_break */
		cs->c_continue = true;
	}

	check_statement_reachable();

	set_reached(false);
}

/*
 * T_RETURN T_SEMI
 * T_RETURN expr T_SEMI
 */
void
do_return(bool sys, tnode_t *tn)
{
	tnode_t	*ln, *rn;
	control_statement *cs;
	op_t	op;

	cs = cstmt;
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
		/* void function %s cannot return value */
		error(213, funcsym->s_name);
		expr_free_all();
		tn = NULL;
	} else if (tn == NULL && funcsym->s_type->t_subt->t_tspec != VOID) {
		/*
		 * Assume that the function has a return value only if it
		 * is explicitly declared.
		 */
		if (!funcsym->s_return_type_implicit_int)
			/* function '%s' expects to return value */
			warning(214, funcsym->s_name);
	}

	if (tn != NULL) {

		/* Create a temporary node for the left side */
		ln = expr_zero_alloc(sizeof(*ln));
		ln->tn_op = NAME;
		ln->tn_type = expr_unqualified_type(funcsym->s_type->t_subt);
		ln->tn_lvalue = true;
		ln->tn_sym = funcsym;		/* better than nothing */

		tn = build_binary(ln, RETURN, sys, tn);

		if (tn != NULL) {
			rn = tn->tn_right;
			while ((op = rn->tn_op) == CVT || op == PLUS)
				rn = rn->tn_left;
			if (rn->tn_op == ADDR && rn->tn_left->tn_op == NAME &&
			    rn->tn_left->tn_sym->s_scl == AUTO) {
				/* %s returns pointer to automatic object */
				warning(302, funcsym->s_name);
			}
		}

		expr(tn, true, false, true, false);

	} else {

		check_statement_reachable();

	}

	set_reached(false);
}

/*
 * Do some cleanup after a global declaration or definition.
 * Especially remove information about unused lint comments.
 */
void
global_clean_up_decl(bool silent)
{

	if (nargusg != -1) {
		if (!silent) {
			/* must precede function definition: ** %s ** */
			warning_at(282, &argsused_pos, "ARGSUSED");
		}
		nargusg = -1;
	}
	if (nvararg != -1) {
		if (!silent) {
			/* must precede function definition: ** %s ** */
			warning_at(282, &vapos, "VARARGS");
		}
		nvararg = -1;
	}
	if (printflike_argnum != -1) {
		if (!silent) {
			/* must precede function definition: ** %s ** */
			warning_at(282, &printflike_pos, "PRINTFLIKE");
		}
		printflike_argnum = -1;
	}
	if (scanflike_argnum != -1) {
		if (!silent) {
			/* must precede function definition: ** %s ** */
			warning_at(282, &scanflike_pos, "SCANFLIKE");
		}
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
 * ARGSUSED comment
 *
 * Only the first n arguments of the following function are checked
 * for usage. A missing argument is taken to be 0.
 */
void
argsused(int n)
{

	if (n == -1)
		n = 0;

	if (dcs->d_ctx != EXTERN) {
		/* must be outside function: ** %s ** */
		warning(280, "ARGSUSED");
		return;
	}
	if (nargusg != -1) {
		/* duplicate use of ** %s ** */
		warning(281, "ARGSUSED");
	}
	nargusg = n;
	argsused_pos = curr_pos;
}

/*
 * VARARGS comment
 *
 * Causes lint2 to check only the first n arguments for compatibility
 * with the function definition. A missing argument is taken to be 0.
 */
void
varargs(int n)
{

	if (n == -1)
		n = 0;

	if (dcs->d_ctx != EXTERN) {
		/* must be outside function: ** %s ** */
		warning(280, "VARARGS");
		return;
	}
	if (nvararg != -1) {
		/* duplicate use of ** %s ** */
		warning(281, "VARARGS");
	}
	nvararg = n;
	vapos = curr_pos;
}

/*
 * PRINTFLIKE comment
 *
 * Check all arguments until the (n-1)-th as usual. The n-th argument is
 * used the check the types of remaining arguments.
 */
void
printflike(int n)
{

	if (n == -1)
		n = 0;

	if (dcs->d_ctx != EXTERN) {
		/* must be outside function: ** %s ** */
		warning(280, "PRINTFLIKE");
		return;
	}
	if (printflike_argnum != -1) {
		/* duplicate use of ** %s ** */
		warning(281, "PRINTFLIKE");
	}
	printflike_argnum = n;
	printflike_pos = curr_pos;
}

/*
 * SCANFLIKE comment
 *
 * Check all arguments until the (n-1)-th as usual. The n-th argument is
 * used the check the types of remaining arguments.
 */
void
scanflike(int n)
{

	if (n == -1)
		n = 0;

	if (dcs->d_ctx != EXTERN) {
		/* must be outside function: ** %s ** */
		warning(280, "SCANFLIKE");
		return;
	}
	if (scanflike_argnum != -1) {
		/* duplicate use of ** %s ** */
		warning(281, "SCANFLIKE");
	}
	scanflike_argnum = n;
	scanflike_pos = curr_pos;
}

/*
 * Set the line number for a CONSTCOND comment. At this and the following
 * line no warnings about constants in conditional contexts are printed.
 */
/* ARGSUSED */
void
constcond(int n)
{

	constcond_flag = true;
}

/*
 * Suppress printing of "fallthrough on ..." warnings until next
 * statement.
 */
/* ARGSUSED */
void
fallthru(int n)
{

	seen_fallthrough = true;
}

/*
 * Stop warnings about statements which cannot be reached. Also tells lint
 * that the following statements cannot be reached (e.g. after exit()).
 */
/* ARGSUSED */
void
not_reached(int n)
{

	set_reached(false);
	warn_about_unreachable = false;
}

/* ARGSUSED */
void
lintlib(int n)
{

	if (dcs->d_ctx != EXTERN) {
		/* must be outside function: ** %s ** */
		warning(280, "LINTLIBRARY");
		return;
	}
	llibflg = true;
	vflag = false;
}

/*
 * Suppress most warnings at the current and the following line.
 */
/* ARGSUSED */
void
linted(int n)
{

	debug_step("set lwarn %d", n);
	lwarn = n;
}

/*
 * Suppress bitfield type errors on the current line.
 */
/* ARGSUSED */
void
bitfieldtype(int n)
{

	debug_step("%s, %d: bitfieldtype_ok = true",
	    curr_pos.p_file, curr_pos.p_line);
	bitfieldtype_ok = true;
}

/*
 * PROTOLIB in conjunction with LINTLIBRARY can be used to handle
 * prototypes like function definitions. This is done if the argument
 * to PROTOLIB is nonzero. Otherwise prototypes are handled normally.
 */
void
protolib(int n)
{

	if (dcs->d_ctx != EXTERN) {
		/* must be outside function: ** %s ** */
		warning(280, "PROTOLIB");
		return;
	}
	plibflg = n != 0;
}

/* The next statement/declaration may use "long long" without a diagnostic. */
/* ARGSUSED */
void
longlong(int n)
{

	quadflg = true;
}
