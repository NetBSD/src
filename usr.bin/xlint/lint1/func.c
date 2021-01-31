/*	$NetBSD: func.c,v 1.67 2021/01/31 12:44:34 rillig Exp $	*/

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
__RCSID("$NetBSD: func.c,v 1.67 2021/01/31 12:44:34 rillig Exp $");
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
 * Is set as long as NOTREACHED is in effect.
 * Is reset everywhere where reached can become 0.
 */
bool	rchflg;

/*
 * In conjunction with 'reached', controls printing of "fallthrough on ..."
 * warnings.
 * Reset by each statement and set by FALLTHROUGH, switch (switch1())
 * and case (label()).
 *
 * Control statements if, for, while and switch do not reset ftflg because
 * this must be done by the controlled statement. At least for if this is
 * important because ** FALLTHROUGH ** after "if (expr) statement" is
 * evaluated before the following token, which causes reduction of above.
 * This means that ** FALLTHROUGH ** after "if ..." would always be ignored.
 */
bool	ftflg;

/* The innermost control statement */
cstk_t	*cstmt;

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
pushctrl(int env)
{
	cstk_t	*ci;

	ci = xcalloc(1, sizeof (cstk_t));
	ci->c_env = env;
	ci->c_surrounding = cstmt;
	cstmt = ci;
}

/*
 * Removes the top element of the stack used for control statements.
 */
void
popctrl(int env)
{
	cstk_t	*ci;
	clst_t	*cl, *next;

	lint_assert(cstmt != NULL);
	lint_assert(cstmt->c_env == env);

	ci = cstmt;
	cstmt = ci->c_surrounding;

	for (cl = ci->c_clst; cl != NULL; cl = next) {
		next = cl->cl_next;
		free(cl);
	}

	free(ci->c_swtype);
	free(ci);
}

/*
 * Prints a warning if a statement cannot be reached.
 */
void
check_statement_reachable(void)
{
	if (!reached && !rchflg) {
		/* statement not reached */
		warning(193);
		reached = true;
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
	for (sym = dcs->d_fpsyms; sym != NULL; sym = sym->s_dlnxt) {
		if (sym->s_blklev != -1) {
			lint_assert(sym->s_blklev == 1);
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
	dcs->d_fdpos = fsym->s_def_pos;

	if ((rdsym = dcs->d_rdcsym) != NULL) {

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
		/* return value is implicitly declared to be int */
		fsym->s_rimpl = true;

	reached = true;
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
		if (funcsym->s_type->t_subt->t_tspec != VOID &&
		    !funcsym->s_rimpl) {
			/* func. %s falls off bottom without returning value */
			warning(217, funcsym->s_name);
		}
	}

	/*
	 * This warning is printed only if the return value was implicitly
	 * declared to be int. Otherwise the wrong return statement
	 * has already printed a warning.
	 */
	if (cstmt->c_had_return_noval && cstmt->c_had_return_value &&
	    funcsym->s_rimpl)
		/* function %s has return (e); and return; */
		warning(216, funcsym->s_name);

	/* Print warnings for unused arguments */
	arg = dcs->d_fargs;
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
		outfdef(funcsym, &dcs->d_fdpos, cstmt->c_had_return_value,
			funcsym->s_osdef, dcs->d_fargs);
	}

	/*
	 * remove all symbols declared during argument declaration from
	 * the symbol table
	 */
	lint_assert(dcs->d_next == NULL);
	lint_assert(dcs->d_ctx == EXTERN);
	rmsyms(dcs->d_fpsyms);

	/* must be set on level 0 */
	reached = true;
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

	reached = true;
}

static void
check_case_label(tnode_t *tn, cstk_t *ci)
{
	clst_t	*cl;
	val_t	*v;
	val_t	nv;
	tspec_t	t;

	if (ci == NULL) {
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

	lint_assert(ci->c_swtype != NULL);

	if (reached && !ftflg) {
		if (hflag)
			/* fallthrough on case statement */
			warning(220);
	}

	t = tn->tn_type->t_tspec;
	if (t == LONG || t == ULONG ||
	    t == QUAD || t == UQUAD) {
		if (tflag)
			/* case label must be of type `int' in traditional C */
			warning(203);
	}

	/*
	 * get the value of the expression and convert it
	 * to the type of the switch expression
	 */
	v = constant(tn, true);
	(void)memset(&nv, 0, sizeof nv);
	convert_constant(CASE, 0, ci->c_swtype, &nv, v);
	free(v);

	/* look if we had this value already */
	for (cl = ci->c_clst; cl != NULL; cl = cl->cl_next) {
		if (cl->cl_val.v_quad == nv.v_quad)
			break;
	}
	if (cl != NULL && is_uinteger(nv.v_tspec)) {
		/* duplicate case in switch: %lu */
		error(200, (u_long)nv.v_quad);
	} else if (cl != NULL) {
		/* duplicate case in switch: %ld */
		error(199, (long)nv.v_quad);
	} else {
		/*
		 * append the value to the list of
		 * case values
		 */
		cl = xcalloc(1, sizeof (clst_t));
		cl->cl_val = nv;
		cl->cl_next = ci->c_clst;
		ci->c_clst = cl;
	}
}

void
case_label(tnode_t *tn)
{
	cstk_t	*ci;

	/* find the innermost switch statement */
	for (ci = cstmt; ci != NULL && !ci->c_switch; ci = ci->c_surrounding)
		continue;

	check_case_label(tn, ci);

	tfreeblk();

	reached = true;
}

void
default_label(void)
{
	cstk_t	*ci;

	/* find the innermost switch statement */
	for (ci = cstmt; ci != NULL && !ci->c_switch; ci = ci->c_surrounding)
		continue;

	if (ci == NULL) {
		/* default outside switch */
		error(201);
	} else if (ci->c_default) {
		/* duplicate default in switch */
		error(202);
	} else {
		if (reached && !ftflg) {
			if (hflag)
				/* fallthrough on default statement */
				warning(284);
		}
		ci->c_default = true;
	}

	reached = true;
}

static tnode_t *
check_controlling_expression(tnode_t *tn)
{

	if (tn != NULL)
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
		return NULL;
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
	pushctrl(T_IF);
}

/*
 * if_without_else
 * if_without_else T_ELSE
 */
void
if2(void)
{

	cstmt->c_rchif = reached;
	reached = true;
}

/*
 * if_without_else
 * if_without_else T_ELSE statement
 */
void
if3(bool els)
{

	if (els) {
		reached |= cstmt->c_rchif;
	} else {
		reached = true;
	}
	popctrl(T_IF);
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
			/* switch expr. must be of type `int' in trad. C */
			warning(271);
		}
	}

	/*
	 * Remember the type of the expression. Because it's possible
	 * that (*tp) is allocated on tree memory, the type must be
	 * duplicated. This is not too complicated because it is
	 * only an integer type.
	 */
	tp = xcalloc(1, sizeof (type_t));
	if (tn != NULL) {
		tp->t_tspec = tn->tn_type->t_tspec;
		if ((tp->t_isenum = tn->tn_type->t_isenum) != false)
			tp->t_enum = tn->tn_type->t_enum;
	} else {
		tp->t_tspec = INT;
	}

	expr(tn, true, false, true, false);

	pushctrl(T_SWITCH);
	cstmt->c_switch = true;
	cstmt->c_swtype = tp;

	reached = rchflg = false;
	ftflg = true;
}

/*
 * switch_expr statement
 */
void
switch2(void)
{
	int	nenum = 0, nclab = 0;
	sym_t	*esym;
	clst_t	*cl;

	lint_assert(cstmt->c_swtype != NULL);

	/*
	 * If the switch expression was of type enumeration, count the case
	 * labels and the number of enumerators. If both counts are not
	 * equal print a warning.
	 */
	if (cstmt->c_swtype->t_isenum) {
		nenum = nclab = 0;
		lint_assert(cstmt->c_swtype->t_enum != NULL);
		for (esym = cstmt->c_swtype->t_enum->elem;
		     esym != NULL; esym = esym->s_next) {
			nenum++;
		}
		for (cl = cstmt->c_clst; cl != NULL; cl = cl->cl_next)
			nclab++;
		if (hflag && eflag && nenum != nclab && !cstmt->c_default) {
			/* enumeration value(s) not handled in switch */
			warning(206);
		}
	}

	if (cstmt->c_break) {
		/*
		 * end of switch alway reached (c_break is only set if the
		 * break statement can be reached).
		 */
		reached = true;
	} else if (!cstmt->c_default &&
		   (!hflag || !cstmt->c_swtype->t_isenum || nenum != nclab)) {
		/*
		 * there are possible values which are not handled in
		 * switch
		 */
		reached = true;
	}	/*
		 * otherwise the end of the switch expression is reached
		 * if the end of the last statement inside it is reached.
		 */

	popctrl(T_SWITCH);
}

/*
 * T_WHILE T_LPAREN expr T_RPAREN
 */
void
while1(tnode_t *tn)
{

	if (!reached) {
		/* loop not entered at top */
		warning(207);
		reached = true;
	}

	if (tn != NULL)
		tn = check_controlling_expression(tn);

	pushctrl(T_WHILE);
	cstmt->c_loop = true;
	if (tn != NULL && tn->tn_op == CON)
		cstmt->c_infinite = is_nonzero(tn);

	expr(tn, false, true, true, false);
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
	reached = !cstmt->c_infinite || cstmt->c_break;
	rchflg = false;

	popctrl(T_WHILE);
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
		reached = true;
	}

	pushctrl(T_DO);
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
	if (cstmt->c_cont)
		reached = true;

	if (tn != NULL)
		tn = check_controlling_expression(tn);

	if (tn != NULL && tn->tn_op == CON) {
		cstmt->c_infinite = is_nonzero(tn);
		if (!cstmt->c_infinite && cstmt->c_cont)
			/* continue in 'do ... while (0)' loop */
			error(323);
	}

	expr(tn, false, true, true, true);

	/*
	 * The end of the loop is only reached if it is no endless loop
	 * or there was a break statement which could be reached.
	 */
	reached = !cstmt->c_infinite || cstmt->c_break;
	rchflg = false;

	popctrl(T_DO);
}

/*
 * T_FOR T_LPAREN opt_expr T_SEMI opt_expr T_SEMI opt_expr T_RPAREN
 */
void
for1(tnode_t *tn1, tnode_t *tn2, tnode_t *tn3)
{

	/*
	 * If there is no initialisation expression it is possible that
	 * it is intended not to enter the loop at top.
	 */
	if (tn1 != NULL && !reached) {
		/* loop not entered at top */
		warning(207);
		reached = true;
	}

	pushctrl(T_FOR);
	cstmt->c_loop = true;

	/*
	 * Store the tree memory for the reinitialisation expression.
	 * Also remember this expression itself. We must check it at
	 * the end of the loop to get "used but not set" warnings correct.
	 */
	cstmt->c_fexprm = tsave();
	cstmt->c_f3expr = tn3;
	cstmt->c_fpos = curr_pos;
	cstmt->c_cfpos = csrc_pos;

	if (tn1 != NULL)
		expr(tn1, false, false, true, false);

	if (tn2 != NULL)
		tn2 = check_controlling_expression(tn2);
	if (tn2 != NULL)
		expr(tn2, false, true, true, false);

	cstmt->c_infinite =
	    tn2 == NULL || (tn2->tn_op == CON && is_nonzero(tn2));

	/* Checking the reinitialisation expression is done in for2() */

	reached = true;
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

	if (cstmt->c_cont)
		reached = true;

	cpos = curr_pos;
	cspos = csrc_pos;

	/* Restore the tree memory for the reinitialisation expression */
	trestor(cstmt->c_fexprm);
	tn3 = cstmt->c_f3expr;
	curr_pos = cstmt->c_fpos;
	csrc_pos = cstmt->c_cfpos;

	/* simply "statement not reached" would be confusing */
	if (!reached && !rchflg) {
		/* end-of-loop code not reached */
		warning(223);
		reached = true;
	}

	if (tn3 != NULL) {
		expr(tn3, false, false, true, false);
	} else {
		tfreeblk();
	}

	curr_pos = cpos;
	csrc_pos = cspos;

	/* An endless loop without break will never terminate */
	reached = cstmt->c_break || !cstmt->c_infinite;
	rchflg = false;

	popctrl(T_FOR);
}

/*
 * T_GOTO identifier T_SEMI
 */
void
dogoto(sym_t *lab)
{

	mark_as_used(lab, false, false);

	check_statement_reachable();

	reached = rchflg = false;
}

/*
 * T_BREAK T_SEMI
 */
void
dobreak(void)
{
	cstk_t	*ci;

	ci = cstmt;
	while (ci != NULL && !ci->c_loop && !ci->c_switch)
		ci = ci->c_surrounding;

	if (ci == NULL) {
		/* break outside loop or switch */
		error(208);
	} else {
		if (reached)
			ci->c_break = true;
	}

	if (bflag)
		check_statement_reachable();

	reached = rchflg = false;
}

/*
 * T_CONTINUE T_SEMI
 */
void
docont(void)
{
	cstk_t	*ci;

	for (ci = cstmt; ci != NULL && !ci->c_loop; ci = ci->c_surrounding)
		continue;

	if (ci == NULL) {
		/* continue outside loop */
		error(209);
	} else {
		ci->c_cont = true;
	}

	check_statement_reachable();

	reached = rchflg = false;
}

/*
 * T_RETURN T_SEMI
 * T_RETURN expr T_SEMI
 */
void
doreturn(tnode_t *tn)
{
	tnode_t	*ln, *rn;
	cstk_t	*ci;
	op_t	op;

	for (ci = cstmt; ci->c_surrounding != NULL; ci = ci->c_surrounding)
		continue;

	if (tn != NULL) {
		ci->c_had_return_value = true;
	} else {
		ci->c_had_return_noval = true;
	}

	if (tn != NULL && funcsym->s_type->t_subt->t_tspec == VOID) {
		/* void function %s cannot return value */
		error(213, funcsym->s_name);
		tfreeblk();
		tn = NULL;
	} else if (tn == NULL && funcsym->s_type->t_subt->t_tspec != VOID) {
		/*
		 * Assume that the function has a return value only if it
		 * is explicitly declared.
		 */
		if (!funcsym->s_rimpl)
			/* function %s expects to return value */
			warning(214, funcsym->s_name);
	}

	if (tn != NULL) {

		/* Create a temporary node for the left side */
		ln = tgetblk(sizeof (tnode_t));
		ln->tn_op = NAME;
		ln->tn_type = tduptyp(funcsym->s_type->t_subt);
		ln->tn_type->t_const = false;
		ln->tn_lvalue = true;
		ln->tn_sym = funcsym;		/* better than nothing */

		tn = build(RETURN, ln, tn);

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

	reached = rchflg = false;
}

/*
 * Do some cleanup after a global declaration or definition.
 * Especially remove information about unused lint comments.
 */
void
global_clean_up_decl(bool silent)
{
	pos_t	cpos;

	cpos = curr_pos;

	if (nargusg != -1) {
		if (!silent) {
			curr_pos = argsused_pos;
			/* must precede function definition: ** %s ** */
			warning(282, "ARGSUSED");
		}
		nargusg = -1;
	}
	if (nvararg != -1) {
		if (!silent) {
			curr_pos = vapos;
			/* must precede function definition: ** %s ** */
			warning(282, "VARARGS");
		}
		nvararg = -1;
	}
	if (printflike_argnum != -1) {
		if (!silent) {
			curr_pos = printflike_pos;
			/* must precede function definition: ** %s ** */
			warning(282, "PRINTFLIKE");
		}
		printflike_argnum = -1;
	}
	if (scanflike_argnum != -1) {
		if (!silent) {
			curr_pos = scanflike_pos;
			/* must precede function definition: ** %s ** */
			warning(282, "SCANFLIKE");
		}
		scanflike_argnum = -1;
	}

	curr_pos = cpos;

	dcs->d_asm = false;
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

	ftflg = true;
}

/*
 * Stop warnings about statements which cannot be reached. Also tells lint
 * that the following statements cannot be reached (e.g. after exit()).
 */
/* ARGSUSED */
void
notreach(int n)
{

	reached = false;
	rchflg = true;
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

#ifdef DEBUG
	printf("%s, %d: lwarn = %d\n", curr_pos.p_file, curr_pos.p_line, n);
#endif
	lwarn = n;
}

/*
 * Suppress bitfield type errors on the current line.
 */
/* ARGSUSED */
void
bitfieldtype(int n)
{

#ifdef DEBUG
	printf("%s, %d: bitfieldtype_ok = true\n", curr_pos.p_file,
	    curr_pos.p_line);
#endif
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
