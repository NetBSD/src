/*	$NetBSD: tree.c,v 1.405 2022/02/27 08:31:26 rillig Exp $	*/

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
__RCSID("$NetBSD: tree.c,v 1.405 2022/02/27 08:31:26 rillig Exp $");
#endif

#include <float.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"
#include "cgram.h"

static	tnode_t	*build_integer_constant(tspec_t, int64_t);
static	void	check_pointer_comparison(op_t,
					 const tnode_t *, const tnode_t *);
static	bool	check_assign_types_compatible(op_t, int,
					      const tnode_t *, const tnode_t *);
static	void	check_bad_enum_operation(op_t,
					 const tnode_t *, const tnode_t *);
static	void	check_enum_type_mismatch(op_t, int,
				         const tnode_t *, const tnode_t *);
static	void	check_enum_int_mismatch(op_t, int,
					const tnode_t *, const tnode_t *);
static	tnode_t	*new_tnode(op_t, bool, type_t *, tnode_t *, tnode_t *);
static	void	balance(op_t, tnode_t **, tnode_t **);
static	void	warn_incompatible_types(op_t, const type_t *, tspec_t,
					const type_t *, tspec_t);
static	void	warn_incompatible_pointers(const mod_t *,
					   const type_t *, const type_t *);
static	bool	has_constant_member(const type_t *);
static	void	check_prototype_conversion(int, tspec_t, tspec_t, type_t *,
					   tnode_t *);
static	void	check_integer_conversion(op_t, int, tspec_t, tspec_t, type_t *,
					 tnode_t *);
static	void	check_pointer_integer_conversion(op_t, tspec_t, type_t *,
						 tnode_t *);
static	void	check_pointer_conversion(tnode_t *, type_t *);
static	tnode_t	*build_struct_access(op_t, bool, tnode_t *, tnode_t *);
static	tnode_t	*build_prepost_incdec(op_t, bool, tnode_t *);
static	tnode_t	*build_real_imag(op_t, bool, tnode_t *);
static	tnode_t	*build_address(bool, tnode_t *, bool);
static	tnode_t	*build_plus_minus(op_t, bool, tnode_t *, tnode_t *);
static	tnode_t	*build_bit_shift(op_t, bool, tnode_t *, tnode_t *);
static	tnode_t	*build_colon(bool, tnode_t *, tnode_t *);
static	tnode_t	*build_assignment(op_t, bool, tnode_t *, tnode_t *);
static	tnode_t	*plength(type_t *);
static	tnode_t	*fold(tnode_t *);
static	tnode_t	*fold_test(tnode_t *);
static	tnode_t	*fold_float(tnode_t *);
static	tnode_t	*check_function_arguments(type_t *, tnode_t *);
static	tnode_t	*check_prototype_argument(int, type_t *, tnode_t *);
static	void	check_null_effect(const tnode_t *);
static	void	check_array_index(tnode_t *, bool);
static	void	check_integer_comparison(op_t, tnode_t *, tnode_t *);
static	void	check_precedence_confusion(tnode_t *);

extern sig_atomic_t fpe;

static const char *
op_name(op_t op)
{
	return modtab[op].m_name;
}

/* Build 'pointer to tp', 'array of tp' or 'function returning tp'. */
type_t *
derive_type(type_t *tp, tspec_t t)
{
	type_t	*tp2;

	tp2 = block_zero_alloc(sizeof(*tp2));
	tp2->t_tspec = t;
	tp2->t_subt = tp;
	return tp2;
}

/*
 * Derive 'pointer to tp' or 'function returning tp'.
 * The memory is freed at the end of the current expression.
 */
type_t *
expr_derive_type(type_t *tp, tspec_t t)
{
	type_t	*tp2;

	tp2 = expr_zero_alloc(sizeof(*tp2));
	tp2->t_tspec = t;
	tp2->t_subt = tp;
	return tp2;
}

/*
 * Create a node for a constant.
 */
tnode_t *
build_constant(type_t *tp, val_t *v)
{
	tnode_t	*n;

	n = expr_alloc_tnode();
	n->tn_op = CON;
	n->tn_type = tp;
	n->tn_val = expr_zero_alloc(sizeof(*n->tn_val));
	n->tn_val->v_tspec = tp->t_tspec;
	n->tn_val->v_unsigned_since_c90 = v->v_unsigned_since_c90;
	n->tn_val->v_u = v->v_u;
	free(v);
	return n;
}

static tnode_t *
build_integer_constant(tspec_t t, int64_t q)
{
	tnode_t	*n;

	n = expr_alloc_tnode();
	n->tn_op = CON;
	n->tn_type = gettyp(t);
	n->tn_val = expr_zero_alloc(sizeof(*n->tn_val));
	n->tn_val->v_tspec = t;
	n->tn_val->v_quad = q;
	return n;
}

static void
fallback_symbol(sym_t *sym)
{

	if (fallback_symbol_strict_bool(sym))
		return;

	if (block_level > 0 && (strcmp(sym->s_name, "__FUNCTION__") == 0 ||
			   strcmp(sym->s_name, "__PRETTY_FUNCTION__") == 0)) {
		/* __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension */
		gnuism(316);
		sym->s_type = derive_type(gettyp(CHAR), PTR);
		sym->s_type->t_const = true;
		return;
	}

	if (block_level > 0 && strcmp(sym->s_name, "__func__") == 0) {
		if (!Sflag)
			/* __func__ is a C9X feature */
			warning(317);
		sym->s_type = derive_type(gettyp(CHAR), PTR);
		sym->s_type->t_const = true;
		return;
	}

	/* '%s' undefined */
	error(99, sym->s_name);
}

/*
 * Functions that are predeclared by GCC or other compilers can be called
 * with arbitrary arguments.  Since lint usually runs after a successful
 * compilation, it's the compiler's job to catch any errors.
 */
bool
is_compiler_builtin(const char *name)
{
	/* https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html */
	if (gflag) {
		if (strncmp(name, "__atomic_", 9) == 0 ||
		    strncmp(name, "__builtin_", 10) == 0 ||
		    strcmp(name, "alloca") == 0 ||
		    /* obsolete but still in use, as of 2021 */
		    strncmp(name, "__sync_", 7) == 0)
			return true;
	}

	/* https://software.intel.com/sites/landingpage/IntrinsicsGuide/ */
	if (strncmp(name, "_mm_", 4) == 0)
		return true;

	return false;
}

static bool
str_endswith(const char *haystack, const char *needle)
{
	size_t hlen = strlen(haystack);
	size_t nlen = strlen(needle);

	return nlen <= hlen &&
	       memcmp(haystack + hlen - nlen, needle, nlen) == 0;
}

/* https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html */
static bool
is_gcc_bool_builtin(const char *name)
{
	return strncmp(name, "__builtin_", 10) == 0 &&
	       (str_endswith(name, "_overflow") ||
		str_endswith(name, "_overflow_p"));
}

static void
build_name_call(sym_t *sym)
{

	if (is_compiler_builtin(sym->s_name)) {
		/*
		 * Do not warn about these, just assume that
		 * they are regular functions compatible with
		 * non-prototype calling conventions.
		 */
		if (gflag && is_gcc_bool_builtin(sym->s_name))
			sym->s_type = gettyp(BOOL);

	} else if (Sflag) {
		/* function '%s' implicitly declared to return int */
		error(215, sym->s_name);
	} else if (sflag) {
		/* function '%s' implicitly declared to return int */
		warning(215, sym->s_name);
	}

	/* XXX if tflag is set, the symbol should be exported to level 0 */
	sym->s_type = derive_type(sym->s_type, FUNC);
}

/* Create a node for a name (symbol table entry). */
tnode_t *
build_name(sym_t *sym, bool is_funcname)
{
	tnode_t	*n;

	if (sym->s_scl == NOSCL) {
		sym->s_scl = EXTERN;
		sym->s_def = DECL;
		if (is_funcname)
			build_name_call(sym);
		else
			fallback_symbol(sym);
	}

	lint_assert(sym->s_kind == FVFT || sym->s_kind == FMEMBER);

	n = expr_alloc_tnode();
	n->tn_type = sym->s_type;
	if (sym->s_scl == CTCONST) {
		n->tn_op = CON;
		n->tn_val = expr_zero_alloc(sizeof(*n->tn_val));
		*n->tn_val = sym->s_value;
	} else {
		n->tn_op = NAME;
		n->tn_sym = sym;
		if (sym->s_kind == FVFT && sym->s_type->t_tspec != FUNC)
			n->tn_lvalue = true;
	}

	return n;
}

tnode_t *
build_string(strg_t *strg)
{
	size_t	len;
	tnode_t	*n;
	type_t *tp;

	len = strg->st_len;

	n = expr_alloc_tnode();

	tp = expr_zero_alloc(sizeof(*tp));
	tp->t_tspec = ARRAY;
	tp->t_subt = gettyp(strg->st_tspec);
	tp->t_dim = (int)(len + 1);

	n->tn_op = STRING;
	n->tn_type = tp;
	n->tn_lvalue = true;

	n->tn_string = expr_zero_alloc(sizeof(*n->tn_string));
	n->tn_string->st_tspec = strg->st_tspec;
	n->tn_string->st_len = len;

	if (strg->st_tspec == CHAR) {
		n->tn_string->st_cp = expr_zero_alloc(len + 1);
		(void)memcpy(n->tn_string->st_cp, strg->st_cp, len + 1);
		free(strg->st_cp);
	} else {
		size_t size = (len + 1) * sizeof(*n->tn_string->st_wcp);
		n->tn_string->st_wcp = expr_zero_alloc(size);
		(void)memcpy(n->tn_string->st_wcp, strg->st_wcp, size);
		free(strg->st_wcp);
	}
	free(strg);

	return n;
}

/*
 * Returns a symbol which has the same name as the msym argument and is a
 * member of the struct or union specified by the tn argument.
 */
static sym_t *
struct_or_union_member(tnode_t *tn, op_t op, sym_t *msym)
{
	struct_or_union	*str;
	type_t	*tp;
	sym_t	*sym, *csym;
	bool	eq;
	tspec_t	t;

	/*
	 * Remove the member if it was unknown until now, which means
	 * that no defined struct or union has a member with the same name.
	 */
	if (msym->s_scl == NOSCL) {
		/* type '%s' does not have member '%s' */
		error(101, type_name(tn->tn_type), msym->s_name);
		rmsym(msym);
		msym->s_kind = FMEMBER;
		msym->s_scl = MOS;
		msym->s_styp = expr_zero_alloc(sizeof(*msym->s_styp));
		msym->s_styp->sou_tag = expr_zero_alloc(
		    sizeof(*msym->s_styp->sou_tag));
		msym->s_styp->sou_tag->s_name = unnamed;
		msym->s_value.v_tspec = INT;
		return msym;
	}

	/* Set str to the tag of which msym is expected to be a member. */
	str = NULL;
	t = (tp = tn->tn_type)->t_tspec;
	if (op == POINT) {
		if (t == STRUCT || t == UNION)
			str = tp->t_str;
	} else if (op == ARROW && t == PTR) {
		t = (tp = tp->t_subt)->t_tspec;
		if (t == STRUCT || t == UNION)
			str = tp->t_str;
	}

	/*
	 * If this struct/union has a member with the name of msym, return it.
	 */
	if (str != NULL) {
		for (sym = msym; sym != NULL; sym = sym->s_link) {
			if (sym->s_scl != MOS && sym->s_scl != MOU)
				continue;
			if (sym->s_styp != str)
				continue;
			if (strcmp(sym->s_name, msym->s_name) != 0)
				continue;
			return sym;
		}
	}

	/*
	 * Set eq to false if there are struct/union members with the same
	 * name and different types and/or offsets.
	 */
	eq = true;
	for (csym = msym; csym != NULL; csym = csym->s_link) {
		if (csym->s_scl != MOS && csym->s_scl != MOU)
			continue;
		if (strcmp(msym->s_name, csym->s_name) != 0)
			continue;
		for (sym = csym->s_link; sym != NULL; sym = sym->s_link) {
			bool w;

			if (sym->s_scl != MOS && sym->s_scl != MOU)
				continue;
			if (strcmp(csym->s_name, sym->s_name) != 0)
				continue;
			if (csym->s_value.v_quad != sym->s_value.v_quad) {
				eq = false;
				break;
			}
			w = false;
			eq = eqtype(csym->s_type, sym->s_type,
			    false, false, &w) && !w;
			if (!eq)
				break;
			if (csym->s_bitfield != sym->s_bitfield) {
				eq = false;
				break;
			}
			if (csym->s_bitfield) {
				type_t	*tp1, *tp2;

				tp1 = csym->s_type;
				tp2 = sym->s_type;
				if (tp1->t_flen != tp2->t_flen) {
					eq = false;
					break;
				}
				if (tp1->t_foffs != tp2->t_foffs) {
					eq = false;
					break;
				}
			}
		}
		if (!eq)
			break;
	}

	/*
	 * Now handle the case in which the left operand refers really
	 * to a struct/union, but the right operand is not member of it.
	 */
	if (str != NULL) {
		if (eq && tflag) {
			/* illegal member use: %s */
			warning(102, msym->s_name);
		} else {
			/* illegal member use: %s */
			error(102, msym->s_name);
		}
		return msym;
	}

	/*
	 * Now the left operand of ARROW does not point to a struct/union
	 * or the left operand of POINT is no struct/union.
	 */
	if (eq) {
		if (op == POINT) {
			if (tflag) {
				/* left operand of '.' must be struct ... */
				warning(103, type_name(tn->tn_type));
			} else {
				/* left operand of '.' must be struct ... */
				error(103, type_name(tn->tn_type));
			}
		} else {
			if (tflag && tn->tn_type->t_tspec == PTR) {
				/* left operand of '->' must be pointer ... */
				warning(104, type_name(tn->tn_type));
			} else {
				/* left operand of '->' must be pointer ... */
				error(104, type_name(tn->tn_type));
			}
		}
	} else {
		if (tflag) {
			/* non-unique member requires struct/union %s */
			error(105, op == POINT ? "object" : "pointer");
		} else {
			/* unacceptable operand of '%s' */
			error(111, op_name(op));
		}
	}

	return msym;
}

tnode_t *
build_generic_selection(const tnode_t *expr,
			struct generic_association *sel)
{
	tnode_t *default_result = NULL;

	for (; sel != NULL; sel = sel->ga_prev)
		if (expr != NULL &&
		    eqtype(sel->ga_arg, expr->tn_type, false, false, NULL))
			return sel->ga_result;
		else if (sel->ga_arg == NULL)
			default_result = sel->ga_result;
	return default_result;
}

/*
 * Create a tree node for a binary operator and its two operands. Also called
 * for unary operators; in that case rn is NULL.
 *
 * Function calls, sizeof and casts are handled elsewhere.
 */
tnode_t *
build_binary(tnode_t *ln, op_t op, bool sys, tnode_t *rn)
{
	const mod_t *mp;
	tnode_t	*ntn;
	type_t	*rettp;

	mp = &modtab[op];

	/* If there was an error in one of the operands, return. */
	if (ln == NULL || (mp->m_binary && rn == NULL))
		return NULL;

	/*
	 * Apply class conversions to the left operand, but only if its
	 * value is needed or it is compared with null.
	 */
	if (mp->m_left_value_context || mp->m_left_test_context)
		ln = cconv(ln);
	/*
	 * The right operand is almost always in a test or value context,
	 * except if it is a struct or union member.
	 */
	if (mp->m_binary && op != ARROW && op != POINT)
		rn = cconv(rn);

	/*
	 * Print some warnings for comparisons of unsigned values with
	 * constants lower than or equal to null. This must be done
	 * before promote() because otherwise unsigned char and unsigned
	 * short would be promoted to int. Also types are tested to be
	 * CHAR, which would also become int.
	 */
	if (mp->m_comparison)
		check_integer_comparison(op, ln, rn);

	/*
	 * Promote the left operand if it is in a test or value context
	 */
	if (mp->m_left_value_context || mp->m_left_test_context)
		ln = promote(op, false, ln);
	/*
	 * Promote the right operand, but only if it is no struct or
	 * union member, or if it is not to be assigned to the left operand
	 */
	if (mp->m_binary && op != ARROW && op != POINT &&
	    op != ASSIGN && op != RETURN && op != INIT) {
		rn = promote(op, false, rn);
	}

	/*
	 * If the result of the operation is different for signed or
	 * unsigned operands and one of the operands is signed only in
	 * ANSI C, print a warning.
	 */
	if (mp->m_warn_if_left_unsigned_in_c90 &&
	    ln->tn_op == CON && ln->tn_val->v_unsigned_since_c90) {
		/* ANSI C treats constant as unsigned, op %s */
		warning(218, mp->m_name);
		ln->tn_val->v_unsigned_since_c90 = false;
	}
	if (mp->m_warn_if_right_unsigned_in_c90 &&
	    rn->tn_op == CON && rn->tn_val->v_unsigned_since_c90) {
		/* ANSI C treats constant as unsigned, op %s */
		warning(218, mp->m_name);
		rn->tn_val->v_unsigned_since_c90 = false;
	}

	/* Make sure both operands are of the same type */
	if (mp->m_balance_operands || (tflag && (op == SHL || op == SHR)))
		balance(op, &ln, &rn);

	/*
	 * Check types for compatibility with the operation and mutual
	 * compatibility. Return if there are serious problems.
	 */
	if (!typeok(op, 0, ln, rn))
		return NULL;

	/* And now create the node. */
	switch (op) {
	case POINT:
	case ARROW:
		ntn = build_struct_access(op, sys, ln, rn);
		break;
	case INCAFT:
	case DECAFT:
	case INCBEF:
	case DECBEF:
		ntn = build_prepost_incdec(op, sys, ln);
		break;
	case ADDR:
		ntn = build_address(sys, ln, false);
		break;
	case INDIR:
		ntn = new_tnode(INDIR, sys, ln->tn_type->t_subt, ln, NULL);
		break;
	case PLUS:
	case MINUS:
		ntn = build_plus_minus(op, sys, ln, rn);
		break;
	case SHL:
	case SHR:
		ntn = build_bit_shift(op, sys, ln, rn);
		break;
	case COLON:
		ntn = build_colon(sys, ln, rn);
		break;
	case ASSIGN:
	case MULASS:
	case DIVASS:
	case MODASS:
	case ADDASS:
	case SUBASS:
	case SHLASS:
	case SHRASS:
	case ANDASS:
	case XORASS:
	case ORASS:
	case RETURN:
	case INIT:
		ntn = build_assignment(op, sys, ln, rn);
		break;
	case COMMA:
	case QUEST:
		ntn = new_tnode(op, sys, rn->tn_type, ln, rn);
		break;
	case REAL:
	case IMAG:
		ntn = build_real_imag(op, sys, ln);
		break;
	default:
		rettp = mp->m_returns_bool
		    ? gettyp(Tflag ? BOOL : INT) : ln->tn_type;
		lint_assert(mp->m_binary || rn == NULL);
		ntn = new_tnode(op, sys, rettp, ln, rn);
		break;
	}

	/* Return if an error occurred. */
	if (ntn == NULL)
		return NULL;

	/* Print a warning if precedence confusion is possible */
	if (mp->m_possible_precedence_confusion)
		check_precedence_confusion(ntn);

	/*
	 * Print a warning if one of the operands is in a context where
	 * it is compared with null and if this operand is a constant.
	 */
	if (mp->m_left_test_context) {
		if (ln->tn_op == CON ||
		    ((mp->m_binary && op != QUEST) && rn->tn_op == CON)) {
			if (hflag && !constcond_flag &&
			    !ln->tn_system_dependent)
				/* constant in conditional context */
				warning(161);
		}
	}

	/* Fold if the operator requires it */
	if (mp->m_fold_constant_operands) {
		if (ln->tn_op == CON && (!mp->m_binary || rn->tn_op == CON)) {
			if (mp->m_left_test_context) {
				ntn = fold_test(ntn);
			} else if (is_floating(ntn->tn_type->t_tspec)) {
				ntn = fold_float(ntn);
			} else {
				ntn = fold(ntn);
			}
		} else if (op == QUEST && ln->tn_op == CON) {
			ntn = ln->tn_val->v_quad != 0
			    ? rn->tn_left : rn->tn_right;
		}
	}

	return ntn;
}

tnode_t *
build_unary(op_t op, bool sys, tnode_t *tn)
{
	return build_binary(tn, op, sys, NULL);
}

tnode_t *
build_member_access(tnode_t *ln, op_t op, bool sys, sbuf_t *member)
{
	sym_t	*msym;

	if (ln == NULL)
		return NULL;

	if (op == ARROW) {
		/* must do this before struct_or_union_member is called */
		ln = cconv(ln);
	}
	msym = struct_or_union_member(ln, op, getsym(member));
	return build_binary(ln, op, sys, build_name(msym, false));
}

/*
 * Perform class conversions.
 *
 * Arrays of type T are converted into pointers to type T.
 * Functions are converted to pointers to functions.
 * Lvalues are converted to rvalues.
 *
 * C99 6.3 "Conversions"
 * C99 6.3.2 "Other operands"
 * C99 6.3.2.1 "Lvalues, arrays, and function designators"
 */
tnode_t *
cconv(tnode_t *tn)
{
	type_t	*tp;

	/*
	 * Array-lvalue (array of type T) is converted into rvalue
	 * (pointer to type T)
	 */
	if (tn->tn_type->t_tspec == ARRAY) {
		if (!tn->tn_lvalue) {
			/* XXX print correct operator */
			/* %soperand of '%s' must be lvalue */
			gnuism(114, "", op_name(ADDR));
		}
		tn = new_tnode(ADDR, tn->tn_sys,
		    expr_derive_type(tn->tn_type->t_subt, PTR), tn, NULL);
	}

	/*
	 * Expression of type function (function with return value of type T)
	 * in rvalue-expression (pointer to function with return value
	 * of type T)
	 */
	if (tn->tn_type->t_tspec == FUNC)
		tn = build_address(tn->tn_sys, tn, true);

	/* lvalue to rvalue */
	if (tn->tn_lvalue) {
		tp = expr_dup_type(tn->tn_type);
		/* C99 6.3.2.1p2 sentence 2 says to remove the qualifiers. */
		tp->t_const = tp->t_volatile = false;
		tn = new_tnode(LOAD, tn->tn_sys, tp, tn, NULL);
	}

	return tn;
}

const tnode_t *
before_conversion(const tnode_t *tn)
{
	while (tn->tn_op == CVT && !tn->tn_cast)
		tn = tn->tn_left;
	return tn;
}

static bool
is_null_pointer(const tnode_t *tn)
{
	tspec_t t = tn->tn_type->t_tspec;

	return ((t == PTR && tn->tn_type->t_subt->t_tspec == VOID) ||
		is_integer(t))
	       && (tn->tn_op == CON && tn->tn_val->v_quad == 0);
}

/*
 * Most errors required by ANSI C are reported in struct_or_union_member().
 * Here we only check for totally wrong things.
 */
static bool
typeok_point(const tnode_t *ln, const type_t *ltp, tspec_t lt)
{
	if (lt == FUNC || lt == VOID || ltp->t_bitfield ||
	    ((lt != STRUCT && lt != UNION) && !ln->tn_lvalue)) {
		/* Without tflag we got already an error */
		if (tflag)
			/* unacceptable operand of '%s' */
			error(111, op_name(POINT));
		return false;
	}
	return true;
}

static bool
typeok_arrow(tspec_t lt)
{
	if (lt == PTR || (tflag && is_integer(lt)))
		return true;

	/* Without tflag we got already an error */
	if (tflag)
		/* unacceptable operand of '%s' */
		error(111, op_name(ARROW));
	return false;
}

static bool
typeok_incdec(op_t op, const tnode_t *tn, const type_t *tp)
{
	/* operand has scalar type (checked in typeok) */
	if (!tn->tn_lvalue) {
		if (tn->tn_op == CVT && tn->tn_cast &&
		    tn->tn_left->tn_op == LOAD) {
			/* a cast does not yield an lvalue */
			error(163);
		}
		/* %soperand of '%s' must be lvalue */
		error(114, "", op_name(op));
		return false;
	} else if (tp->t_const) {
		if (!tflag)
			/* %soperand of '%s' must be modifiable lvalue */
			warning(115, "", op_name(op));
	}
	return true;
}

static bool
typeok_address(const mod_t *mp,
	       const tnode_t *tn, const type_t *tp, tspec_t t)
{
	if (t == ARRAY || t == FUNC) {
		/* ok, a warning comes later (in build_address()) */
	} else if (!tn->tn_lvalue) {
		if (tn->tn_op == CVT && tn->tn_cast &&
		    tn->tn_left->tn_op == LOAD) {
			/* a cast does not yield an lvalue */
			error(163);
		}
		/* %soperand of '%s' must be lvalue */
		error(114, "", mp->m_name);
		return false;
	} else if (is_scalar(t)) {
		if (tp->t_bitfield) {
			/* cannot take address of bit-field */
			error(112);
			return false;
		}
	} else if (t != STRUCT && t != UNION) {
		/* unacceptable operand of '%s' */
		error(111, mp->m_name);
		return false;
	}
	if (tn->tn_op == NAME && tn->tn_sym->s_reg) {
		/* cannot take address of register %s */
		error(113, tn->tn_sym->s_name);
		return false;
	}
	return true;
}

static bool
typeok_indir(tspec_t t)
{
	/* until now there were no type checks for this operator */
	if (t != PTR) {
		/* cannot dereference non-pointer type */
		error(96);
		return false;
	}
	return true;
}

static bool
typeok_plus(op_t op,
	    const type_t *ltp, tspec_t lt,
	    const type_t *rtp, tspec_t rt)
{
	/* operands have scalar types (checked above) */
	if ((lt == PTR && !is_integer(rt)) || (rt == PTR && !is_integer(lt))) {
		warn_incompatible_types(op, ltp, lt, rtp, rt);
		return false;
	}
	return true;
}

static bool
typeok_minus(op_t op,
	     const type_t *ltp, tspec_t lt,
	     const type_t *rtp, tspec_t rt)
{
	/* operands have scalar types (checked above) */
	if (lt == PTR && (!is_integer(rt) && rt != PTR)) {
		warn_incompatible_types(op, ltp, lt, rtp, rt);
		return false;
	} else if (rt == PTR && lt != PTR) {
		warn_incompatible_types(op, ltp, lt, rtp, rt);
		return false;
	}
	if (lt == PTR && rt == PTR) {
		if (!eqtype(ltp->t_subt, rtp->t_subt, true, false, NULL)) {
			/* illegal pointer subtraction */
			error(116);
		}
	}
	return true;
}

static void
typeok_shr(const mod_t *mp,
	   const tnode_t *ln, tspec_t lt,
	   const tnode_t *rn, tspec_t rt)
{
	tspec_t olt, ort;

	olt = before_conversion(ln)->tn_type->t_tspec;
	ort = before_conversion(rn)->tn_type->t_tspec;

	/* operands have integer types (checked above) */
	if (pflag && !is_uinteger(olt)) {
		/*
		 * The left operand is signed. This means that
		 * the operation is (possibly) nonportable.
		 */
		if (ln->tn_op != CON) {
			/* bitwise '%s' on signed value possibly nonportable */
			warning(117, mp->m_name);
		} else if (ln->tn_val->v_quad < 0) {
			/* bitwise '%s' on signed value nonportable */
			warning(120, mp->m_name);
		}
	} else if (!tflag && !sflag && !is_uinteger(olt) && is_uinteger(ort)) {
		/*
		 * The left operand would become unsigned in
		 * traditional C.
		 */
		if (hflag && !Sflag &&
		    (ln->tn_op != CON || ln->tn_val->v_quad < 0)) {
			/* semantics of '%s' change in ANSI C; use ... */
			warning(118, mp->m_name);
		}
	} else if (!tflag && !sflag && !is_uinteger(olt) && !is_uinteger(ort) &&
		   portable_size_in_bits(lt) < portable_size_in_bits(rt)) {
		/*
		 * In traditional C the left operand would be extended,
		 * possibly with 1, and then shifted.
		 */
		if (hflag && !Sflag &&
		    (ln->tn_op != CON || ln->tn_val->v_quad < 0)) {
			/* semantics of '%s' change in ANSI C; use ... */
			warning(118, mp->m_name);
		}
	}
}

static void
typeok_shl(const mod_t *mp, tspec_t lt, tspec_t rt)
{
	/*
	 * C90 does not perform balancing for shift operations,
	 * but traditional C does. If the width of the right operand
	 * is greater than the width of the left operand, then in
	 * traditional C the left operand would be extended to the
	 * width of the right operand. For SHL this may result in
	 * different results.
	 */
	if (portable_size_in_bits(lt) < portable_size_in_bits(rt)) {
		/*
		 * XXX If both operands are constant, make sure
		 * that there is really a difference between
		 * ANSI C and traditional C.
		 */
		if (hflag && !Sflag)
			/* semantics of '%s' change in ANSI C; use ... */
			warning(118, mp->m_name);
	}
}

static void
typeok_shift(tspec_t lt, const tnode_t *rn, tspec_t rt)
{
	if (rn->tn_op != CON)
		return;

	if (!is_uinteger(rt) && rn->tn_val->v_quad < 0) {
		/* negative shift */
		warning(121);
	} else if ((uint64_t)rn->tn_val->v_quad ==
		   (uint64_t)size_in_bits(lt)) {
		/* shift equal to size of object */
		warning(267);
	} else if ((uint64_t)rn->tn_val->v_quad > (uint64_t)size_in_bits(lt)) {
		/* shift amount %llu is greater than bit-size %llu of '%s' */
		warning(122, (unsigned long long)rn->tn_val->v_quad,
		    (unsigned long long)size_in_bits(lt),
		    tspec_name(lt));
	}
}

static bool
is_typeok_eq(const tnode_t *ln, tspec_t lt, const tnode_t *rn, tspec_t rt)
{
	if (lt == PTR && is_null_pointer(rn))
		return true;
	if (rt == PTR && is_null_pointer(ln))
		return true;
	return false;
}

static bool
typeok_compare(op_t op,
	       const tnode_t *ln, const type_t *ltp, tspec_t lt,
	       const tnode_t *rn, const type_t *rtp, tspec_t rt)
{
	const char *lx, *rx;

	if (lt == PTR && rt == PTR) {
		check_pointer_comparison(op, ln, rn);
		return true;
	}

	if (lt != PTR && rt != PTR)
		return true;

	if (!is_integer(lt) && !is_integer(rt)) {
		warn_incompatible_types(op, ltp, lt, rtp, rt);
		return false;
	}

	lx = lt == PTR ? "pointer" : "integer";
	rx = rt == PTR ? "pointer" : "integer";
	/* illegal combination of %s '%s' and %s '%s', op '%s' */
	warning(123, lx, type_name(ltp), rx, type_name(rtp), op_name(op));
	return true;
}

static bool
typeok_quest(tspec_t lt, const tnode_t *rn)
{
	if (!is_scalar(lt)) {
		/* first operand must have scalar type, op ? : */
		error(170);
		return false;
	}
	lint_assert(before_conversion(rn)->tn_op == COLON);
	return true;
}

static void
typeok_colon_pointer(const mod_t *mp, const type_t *ltp, const type_t *rtp)
{
	type_t *lstp = ltp->t_subt;
	type_t *rstp = rtp->t_subt;
	tspec_t lst = lstp->t_tspec;
	tspec_t rst = rstp->t_tspec;

	if ((lst == VOID && rst == FUNC) || (lst == FUNC && rst == VOID)) {
		/* (void *)0 handled above */
		if (sflag)
			/* ANSI C forbids conversion of %s to %s, op %s */
			warning(305, "function pointer", "'void *'",
			    mp->m_name);
		return;
	}

	if (eqptrtype(lstp, rstp, true))
		return;
	if (!eqtype(lstp, rstp, true, false, NULL))
		warn_incompatible_pointers(mp, ltp, rtp);
}

static bool
typeok_colon(const mod_t *mp,
	     const tnode_t *ln, const type_t *ltp, tspec_t lt,
	     const tnode_t *rn, const type_t *rtp, tspec_t rt)
{

	if (is_arithmetic(lt) && is_arithmetic(rt))
		return true;
	if (lt == BOOL && rt == BOOL)
		return true;

	if (lt == STRUCT && rt == STRUCT && ltp->t_str == rtp->t_str)
		return true;
	if (lt == UNION && rt == UNION && ltp->t_str == rtp->t_str)
		return true;

	if (lt == PTR && is_null_pointer(rn))
		return true;
	if (rt == PTR && is_null_pointer(ln))
		return true;

	if ((lt == PTR && is_integer(rt)) || (is_integer(lt) && rt == PTR)) {
		const char *lx = lt == PTR ? "pointer" : "integer";
		const char *rx = rt == PTR ? "pointer" : "integer";
		/* illegal combination of %s '%s' and %s '%s', op '%s' */
		warning(123, lx, type_name(ltp),
		    rx, type_name(rtp), mp->m_name);
		return true;
	}

	if (lt == VOID || rt == VOID) {
		if (lt != VOID || rt != VOID)
			/* incompatible types '%s' and '%s' in conditional */
			warning(126, type_name(ltp), type_name(rtp));
		return true;
	}

	if (lt == PTR && rt == PTR) {
		typeok_colon_pointer(mp, ltp, rtp);
		return true;
	}

	/* incompatible types '%s' and '%s' in conditional */
	error(126, type_name(ltp), type_name(rtp));
	return false;
}

static bool
typeok_assign(op_t op, const tnode_t *ln, const type_t *ltp, tspec_t lt)
{
	if (op == RETURN || op == INIT || op == FARG)
		return true;

	if (!ln->tn_lvalue) {
		if (ln->tn_op == CVT && ln->tn_cast &&
		    ln->tn_left->tn_op == LOAD) {
			/* a cast does not yield an lvalue */
			error(163);
		}
		/* %soperand of '%s' must be lvalue */
		error(114, "left ", op_name(op));
		return false;
	} else if (ltp->t_const || ((lt == STRUCT || lt == UNION) &&
				    has_constant_member(ltp))) {
		if (!tflag)
			/* %soperand of '%s' must be modifiable lvalue */
			warning(115, "left ", op_name(op));
	}
	return true;
}

/* Check the types using the information from modtab[]. */
static bool
typeok_scalar(op_t op, const mod_t *mp,
	      const type_t *ltp, tspec_t lt,
	      const type_t *rtp, tspec_t rt)
{
	if (mp->m_takes_bool && lt == BOOL && rt == BOOL)
		return true;
	if (mp->m_requires_integer) {
		if (!is_integer(lt) || (mp->m_binary && !is_integer(rt))) {
			warn_incompatible_types(op, ltp, lt, rtp, rt);
			return false;
		}
	} else if (mp->m_requires_integer_or_complex) {
		if ((!is_integer(lt) && !is_complex(lt)) ||
		    (mp->m_binary && (!is_integer(rt) && !is_complex(rt)))) {
			warn_incompatible_types(op, ltp, lt, rtp, rt);
			return false;
		}
	} else if (mp->m_requires_scalar) {
		if (!is_scalar(lt) || (mp->m_binary && !is_scalar(rt))) {
			warn_incompatible_types(op, ltp, lt, rtp, rt);
			return false;
		}
	} else if (mp->m_requires_arith) {
		if (!is_arithmetic(lt) ||
		    (mp->m_binary && !is_arithmetic(rt))) {
			warn_incompatible_types(op, ltp, lt, rtp, rt);
			return false;
		}
	}
	return true;
}

/*
 * Check the types for specific operators and type combinations.
 *
 * At this point, the operands already conform to the type requirements of
 * the operator, such as being integer, floating or scalar.
 */
static bool
typeok_op(op_t op, const mod_t *mp, int arg,
	  const tnode_t *ln, const type_t *ltp, tspec_t lt,
	  const tnode_t *rn, const type_t *rtp, tspec_t rt)
{
	switch (op) {
	case ARROW:
		return typeok_arrow(lt);
	case POINT:
		return typeok_point(ln, ltp, lt);
	case INCBEF:
	case DECBEF:
	case INCAFT:
	case DECAFT:
		return typeok_incdec(op, ln, ltp);
	case INDIR:
		return typeok_indir(lt);
	case ADDR:
		return typeok_address(mp, ln, ltp, lt);
	case PLUS:
		return typeok_plus(op, ltp, lt, rtp, rt);
	case MINUS:
		return typeok_minus(op, ltp, lt, rtp, rt);
	case SHL:
		typeok_shl(mp, lt, rt);
		goto shift;
	case SHR:
		typeok_shr(mp, ln, lt, rn, rt);
	shift:
		typeok_shift(lt, rn, rt);
		break;
	case LT:
	case LE:
	case GT:
	case GE:
	compare:
		return typeok_compare(op, ln, ltp, lt, rn, rtp, rt);
	case EQ:
	case NE:
		if (is_typeok_eq(ln, lt, rn, rt))
			break;
		goto compare;
	case QUEST:
		return typeok_quest(lt, rn);
	case COLON:
		return typeok_colon(mp, ln, ltp, lt, rn, rtp, rt);
	case ASSIGN:
	case INIT:
	case FARG:
	case RETURN:
		if (!check_assign_types_compatible(op, arg, ln, rn))
			return false;
		goto assign;
	case MULASS:
	case DIVASS:
	case MODASS:
		goto assign;
	case ADDASS:
	case SUBASS:
		if ((lt == PTR && !is_integer(rt)) || rt == PTR) {
			warn_incompatible_types(op, ltp, lt, rtp, rt);
			return false;
		}
		goto assign;
	case SHLASS:
		goto assign;
	case SHRASS:
		if (pflag && !is_uinteger(lt) && !(tflag && is_uinteger(rt))) {
			/* bitwise '%s' on signed value possibly nonportable */
			warning(117, mp->m_name);
		}
		goto assign;
	case ANDASS:
	case XORASS:
	case ORASS:
	assign:
		return typeok_assign(op, ln, ltp, lt);
	case COMMA:
		if (!modtab[ln->tn_op].m_has_side_effect)
			check_null_effect(ln);
		break;
	default:
		break;
	}
	return true;
}

static void
typeok_enum(op_t op, const mod_t *mp, int arg,
	    const tnode_t *ln, const type_t *ltp,
	    const tnode_t *rn, const type_t *rtp)
{
	if (mp->m_bad_on_enum &&
	    (ltp->t_is_enum || (mp->m_binary && rtp->t_is_enum))) {
		check_bad_enum_operation(op, ln, rn);
	} else if (mp->m_valid_on_enum &&
		   (ltp->t_is_enum && rtp != NULL && rtp->t_is_enum)) {
		check_enum_type_mismatch(op, arg, ln, rn);
	} else if (mp->m_valid_on_enum &&
		   (ltp->t_is_enum || (rtp != NULL && rtp->t_is_enum))) {
		check_enum_int_mismatch(op, arg, ln, rn);
	}
}

/* Perform most type checks. Return whether the types are ok. */
bool
typeok(op_t op, int arg, const tnode_t *ln, const tnode_t *rn)
{
	const mod_t *mp;
	tspec_t	lt, rt;
	type_t	*ltp, *rtp;

	mp = &modtab[op];

	lint_assert((ltp = ln->tn_type) != NULL);
	lt = ltp->t_tspec;

	if (mp->m_binary) {
		lint_assert((rtp = rn->tn_type) != NULL);
		rt = rtp->t_tspec;
	} else {
		rtp = NULL;
		rt = NOTSPEC;
	}

	if (Tflag && !typeok_scalar_strict_bool(op, mp, arg, ln, rn))
		return false;
	if (!typeok_scalar(op, mp, ltp, lt, rtp, rt))
		return false;

	if (!typeok_op(op, mp, arg, ln, ltp, lt, rn, rtp, rt))
		return false;

	typeok_enum(op, mp, arg, ln, ltp, rn, rtp);
	return true;
}

static void
check_pointer_comparison(op_t op, const tnode_t *ln, const tnode_t *rn)
{
	type_t	*ltp, *rtp;
	tspec_t	lst, rst;
	const	char *lsts, *rsts;

	lst = (ltp = ln->tn_type)->t_subt->t_tspec;
	rst = (rtp = rn->tn_type)->t_subt->t_tspec;

	if (lst == VOID || rst == VOID) {
		if (sflag && (lst == FUNC || rst == FUNC)) {
			/* (void *)0 already handled in typeok() */
			*(lst == FUNC ? &lsts : &rsts) = "function pointer";
			*(lst == VOID ? &lsts : &rsts) = "'void *'";
			/* ANSI C forbids comparison of %s with %s */
			warning(274, lsts, rsts);
		}
		return;
	}

	if (!eqtype(ltp->t_subt, rtp->t_subt, true, false, NULL)) {
		warn_incompatible_pointers(&modtab[op], ltp, rtp);
		return;
	}

	if (lst == FUNC && rst == FUNC) {
		if (sflag && op != EQ && op != NE)
			/* ANSI C forbids ordered comparisons of ... */
			warning(125);
	}
}

static bool
is_direct_function_call(const tnode_t *tn, const char **out_name)
{

	if (!(tn->tn_op == CALL &&
	      tn->tn_left->tn_op == ADDR &&
	      tn->tn_left->tn_left->tn_op == NAME))
		return false;

	*out_name = tn->tn_left->tn_left->tn_sym->s_name;
	return true;
}

static bool
is_unconst_function(const char *name)
{

	return strcmp(name, "memchr") == 0 ||
	       strcmp(name, "strchr") == 0 ||
	       strcmp(name, "strpbrk") == 0 ||
	       strcmp(name, "strrchr") == 0 ||
	       strcmp(name, "strstr") == 0;
}

static bool
is_const_char_pointer(const tnode_t *tn)
{
	const type_t *tp;

	/*
	 * For traditional reasons, C99 6.4.5p5 defines that string literals
	 * have type 'char[]'.  They are often implicitly converted to
	 * 'char *', for example when they are passed as function arguments.
	 *
	 * C99 6.4.5p6 further defines that modifying a string that is
	 * constructed from a string literal invokes undefined behavior.
	 *
	 * Out of these reasons, string literals are treated as 'effectively
	 * const' here.
	 */
	if (tn->tn_op == CVT &&
	    tn->tn_left->tn_op == ADDR &&
	    tn->tn_left->tn_left->tn_op == STRING)
		return true;

	tp = before_conversion(tn)->tn_type;
	return tp->t_tspec == PTR &&
	       tp->t_subt->t_tspec == CHAR &&
	       tp->t_subt->t_const;
}

static bool
is_const_pointer(const tnode_t *tn)
{
	const type_t *tp;

	tp = before_conversion(tn)->tn_type;
	return tp->t_tspec == PTR && tp->t_subt->t_const;
}

static bool
is_first_arg_const_char_pointer(const tnode_t *tn)
{
	const tnode_t *an;

	an = tn->tn_right;
	if (an == NULL)
		return false;

	while (an->tn_right != NULL)
		an = an->tn_right;
	return is_const_char_pointer(an->tn_left);
}

static bool
is_second_arg_const_pointer(const tnode_t *tn)
{
	const tnode_t *an;

	an = tn->tn_right;
	if (an == NULL || an->tn_right == NULL)
		return false;

	while (an->tn_right->tn_right != NULL)
		an = an->tn_right;
	return is_const_pointer(an->tn_left);
}

static void
check_unconst_function(const type_t *lstp, const tnode_t *rn)
{
	const char *function_name;

	if (lstp->t_tspec == CHAR && !lstp->t_const &&
	    is_direct_function_call(rn, &function_name) &&
	    is_unconst_function(function_name) &&
	    is_first_arg_const_char_pointer(rn)) {
		/* call to '%s' effectively discards 'const' from argument */
		warning(346, function_name);
	}

	if (!lstp->t_const &&
	    is_direct_function_call(rn, &function_name) &&
	    strcmp(function_name, "bsearch") == 0 &&
	    is_second_arg_const_pointer(rn)) {
		/* call to '%s' effectively discards 'const' from argument */
		warning(346, function_name);
	}
}

static void
check_assign_void_pointer(op_t op, int arg,
			  tspec_t lt, tspec_t lst,
			  tspec_t rt, tspec_t rst)
{
	const char *lts, *rts;

	if (!(lt == PTR && rt == PTR && (lst == VOID || rst == VOID)))
		return;
	/* two pointers, at least one pointer to void */

	if (!(sflag && (lst == FUNC || rst == FUNC)))
		return;
	/* comb. of ptr to func and ptr to void */

	*(lst == FUNC ? &lts : &rts) = "function pointer";
	*(lst == VOID ? &lts : &rts) = "'void *'";

	switch (op) {
	case INIT:
	case RETURN:
		/* ANSI C forbids conversion of %s to %s */
		warning(303, rts, lts);
		break;
	case FARG:
		/* ANSI C forbids conversion of %s to %s, arg #%d */
		warning(304, rts, lts, arg);
		break;
	default:
		/* ANSI C forbids conversion of %s to %s, op %s */
		warning(305, rts, lts, op_name(op));
		break;
	}
}

static bool
check_assign_void_pointer_compat(op_t op, int arg,
				 const type_t *const ltp, tspec_t const lt,
				 const type_t *const lstp, tspec_t const lst,
				 const tnode_t *const rn,
				 const type_t *const rtp, tspec_t const rt,
				 const type_t *const rstp, tspec_t const rst)
{
	if (!(lt == PTR && rt == PTR && (lst == VOID || rst == VOID ||
					 eqtype(lstp, rstp, true, false, NULL))))
		return false;

	/* compatible pointer types (qualifiers ignored) */
	if (!tflag &&
	    ((!lstp->t_const && rstp->t_const) ||
	     (!lstp->t_volatile && rstp->t_volatile))) {
		/* left side has not all qualifiers of right */
		switch (op) {
		case INIT:
		case RETURN:
			/* incompatible pointer types (%s != %s) */
			warning(182, type_name(lstp), type_name(rstp));
			break;
		case FARG:
			/* converting '%s' to incompatible '%s' ... */
			warning(153,
			    type_name(rtp), type_name(ltp), arg);
			break;
		default:
			/* operands have incompatible pointer type... */
			warning(128, op_name(op),
			    type_name(lstp), type_name(rstp));
			break;
		}
	}

	if (!tflag)
		check_unconst_function(lstp, rn);

	return true;
}

static bool
check_assign_pointer_integer(op_t op, int arg,
			     const type_t *const ltp, tspec_t const lt,
			     const type_t *const rtp, tspec_t const rt)
{
	const char *lx, *rx;

	if (!((lt == PTR && is_integer(rt)) || (is_integer(lt) && rt == PTR)))
		return false;

	lx = lt == PTR ? "pointer" : "integer";
	rx = rt == PTR ? "pointer" : "integer";

	switch (op) {
	case INIT:
	case RETURN:
		/* illegal combination of %s (%s) and %s (%s) */
		warning(183, lx, type_name(ltp), rx, type_name(rtp));
		break;
	case FARG:
		/* illegal combination of %s (%s) and %s (%s), arg #%d */
		warning(154,
		    lx, type_name(ltp), rx, type_name(rtp), arg);
		break;
	default:
		/* illegal combination of %s '%s' and %s '%s', op '%s' */
		warning(123,
		    lx, type_name(ltp), rx, type_name(rtp), op_name(op));
		break;
	}
	return true;
}

static bool
check_assign_pointer(op_t op, int arg,
		     const type_t *ltp, tspec_t lt,
		     const type_t *rtp, tspec_t rt)
{
	if (!(lt == PTR && rt == PTR))
		return false;

	switch (op) {
	case RETURN:
		warn_incompatible_pointers(NULL, ltp, rtp);
		break;
	case FARG:
		/* converting '%s' to incompatible '%s' for ... */
		warning(153, type_name(rtp), type_name(ltp), arg);
		break;
	default:
		warn_incompatible_pointers(&modtab[op], ltp, rtp);
		break;
	}
	return true;
}

static void
warn_assign(op_t op, int arg,
	    const type_t *ltp, tspec_t lt,
	    const type_t *rtp, tspec_t rt)
{
	switch (op) {
	case INIT:
		/* cannot initialize '%s' from '%s' */
		error(185, type_name(ltp), type_name(rtp));
		break;
	case RETURN:
		/* return value type mismatch (%s) and (%s) */
		error(211, type_name(ltp), type_name(rtp));
		break;
	case FARG:
		/* passing '%s' to incompatible '%s', arg #%d */
		warning(155, type_name(rtp), type_name(ltp), arg);
		break;
	default:
		warn_incompatible_types(op, ltp, lt, rtp, rt);
		break;
	}
}

/*
 * Checks type compatibility for ASSIGN, INIT, FARG and RETURN
 * and prints warnings/errors if necessary.
 * Returns whether the types are (almost) compatible.
 */
static bool
check_assign_types_compatible(op_t op, int arg,
			      const tnode_t *ln, const tnode_t *rn)
{
	tspec_t	lt, rt, lst = NOTSPEC, rst = NOTSPEC;
	type_t	*ltp, *rtp, *lstp = NULL, *rstp = NULL;

	if ((lt = (ltp = ln->tn_type)->t_tspec) == PTR)
		lst = (lstp = ltp->t_subt)->t_tspec;
	if ((rt = (rtp = rn->tn_type)->t_tspec) == PTR)
		rst = (rstp = rtp->t_subt)->t_tspec;

	if (lt == BOOL && is_scalar(rt))	/* C99 6.3.1.2 */
		return true;

	if (is_arithmetic(lt) && (is_arithmetic(rt) || rt == BOOL))
		return true;

	if ((lt == STRUCT || lt == UNION) && (rt == STRUCT || rt == UNION))
		/* both are struct or union */
		return ltp->t_str == rtp->t_str;

	/* a null pointer may be assigned to any pointer */
	if (lt == PTR && is_null_pointer(rn))
		return true;

	check_assign_void_pointer(op, arg, lt, lst, rt, rst);

	if (check_assign_void_pointer_compat(op, arg,
	    ltp, lt, lstp, lst, rn, rtp, rt, rstp, rst))
		return true;

	if (check_assign_pointer_integer(op, arg, ltp, lt, rtp, rt))
		return true;

	if (check_assign_pointer(op, arg, ltp, lt, rtp, rt))
		return true;

	warn_assign(op, arg, ltp, lt, rtp, rt);
	return false;
}

/* Prints a warning if a strange operator is used on an enum type. */
static void
check_bad_enum_operation(op_t op, const tnode_t *ln, const tnode_t *rn)
{

	if (!eflag)
		return;

	/*
	 * Enum as offset to a pointer is an exception (otherwise enums
	 * could not be used as array indices).
	 */
	if (op == PLUS &&
	    ((ln->tn_type->t_is_enum && rn->tn_type->t_tspec == PTR) ||
	     (rn->tn_type->t_is_enum && ln->tn_type->t_tspec == PTR))) {
		return;
	}

	/* dubious operation on enum, op %s */
	warning(241, op_name(op));
}

/*
 * Prints a warning if an operator is applied to two different enum types.
 */
static void
check_enum_type_mismatch(op_t op, int arg, const tnode_t *ln, const tnode_t *rn)
{
	const mod_t *mp;

	mp = &modtab[op];

	if (ln->tn_type->t_enum != rn->tn_type->t_enum) {
		switch (op) {
		case INIT:
			/* enum type mismatch between '%s' and '%s' in ... */
			warning(210,
			    type_name(ln->tn_type), type_name(rn->tn_type));
			break;
		case FARG:
			/* enum type mismatch, arg #%d (%s != %s) */
			warning(156, arg,
			    type_name(ln->tn_type), type_name(rn->tn_type));
			break;
		case RETURN:
			/* return value type mismatch (%s) and (%s) */
			warning(211,
			    type_name(ln->tn_type), type_name(rn->tn_type));
			break;
		default:
			/* enum type mismatch: '%s' '%s' '%s' */
			warning(130, type_name(ln->tn_type), mp->m_name,
			    type_name(rn->tn_type));
			break;
		}
	} else if (Pflag && mp->m_comparison && op != EQ && op != NE) {
		if (eflag)
			/* dubious comparison of enums, op %s */
			warning(243, mp->m_name);
	}
}

/* Prints a warning if the operands mix between enum and integer. */
static void
check_enum_int_mismatch(op_t op, int arg, const tnode_t *ln, const tnode_t *rn)
{

	if (!eflag)
		return;

	switch (op) {
	case INIT:
		/*
		 * Initialization with 0 is allowed. Otherwise, all implicit
		 * initializations would need to be warned upon as well.
		 */
		if (!rn->tn_type->t_is_enum && rn->tn_op == CON &&
		    is_integer(rn->tn_type->t_tspec) &&
		    rn->tn_val->v_quad == 0) {
			return;
		}
		/* initialization of '%s' with '%s' */
		warning(277, type_name(ln->tn_type), type_name(rn->tn_type));
		break;
	case FARG:
		/* combination of '%s' and '%s', arg #%d */
		warning(278,
		    type_name(ln->tn_type), type_name(rn->tn_type), arg);
		break;
	case RETURN:
		/* combination of '%s' and '%s' in return */
		warning(279, type_name(ln->tn_type), type_name(rn->tn_type));
		break;
	default:
		/* combination of '%s' and '%s', op %s */
		warning(242, type_name(ln->tn_type), type_name(rn->tn_type),
		    op_name(op));
		break;
	}
}

static void
check_enum_array_index(const tnode_t *ln, const tnode_t *rn)
{
	int max_array_index;
	int64_t max_enum_value;
	const struct sym *ec, *max_ec;
	const type_t *lt, *rt;

	if (ln->tn_op != ADDR || ln->tn_left->tn_op != NAME)
		return;

	lt = ln->tn_left->tn_type;
	if (lt->t_tspec != ARRAY || lt->t_incomplete_array)
		return;

	if (rn->tn_op != CVT || !rn->tn_type->t_is_enum)
		return;
	if (rn->tn_left->tn_op != LOAD)
		return;

	rt = rn->tn_left->tn_type;
	ec = rt->t_enum->en_first_enumerator;
	max_ec = ec;
	lint_assert(ec != NULL);
	for (ec = ec->s_next; ec != NULL; ec = ec->s_next)
		if (ec->s_value.v_quad > max_ec->s_value.v_quad)
			max_ec = ec;

	max_enum_value = max_ec->s_value.v_quad;
	lint_assert(INT_MIN <= max_enum_value && max_enum_value <= INT_MAX);

	max_array_index = lt->t_dim - 1;
	if (max_enum_value == max_array_index)
		return;

	/*
	 * If the largest enum constant is named '*_NUM_*', it is typically
	 * not part of the allowed enum values but a marker for the number
	 * of actual enum values.
	 */
	if (max_enum_value == max_array_index + 1 &&
	    (strstr(max_ec->s_name, "NUM") != NULL ||
	     strstr(max_ec->s_name, "num") != NULL))
		return;

	/* maximum value %d of '%s' does not match maximum array index %d */
	warning(348, (int)max_enum_value, type_name(rt), max_array_index);
	print_previous_declaration(-1, max_ec);
}

/*
 * Build and initialize a new node.
 */
static tnode_t *
new_tnode(op_t op, bool sys, type_t *type, tnode_t *ln, tnode_t *rn)
{
	tnode_t	*ntn;
	tspec_t	t;
#if 0 /* not yet */
	size_t l;
	uint64_t rnum;
#endif

	ntn = expr_alloc_tnode();

	ntn->tn_op = op;
	ntn->tn_type = type;
	ntn->tn_sys = sys;
	ntn->tn_left = ln;
	ntn->tn_right = rn;

	switch (op) {
#if 0 /* not yet */
	case SHR:
		if (rn->tn_op != CON)
			break;
		rnum = rn->tn_val->v_quad;
		l = type_size_in_bits(ln->tn_type) / CHAR_SIZE;
		t = ln->tn_type->t_tspec;
		switch (l) {
		case 8:
			if (rnum >= 56)
				t = UCHAR;
			else if (rnum >= 48)
				t = USHORT;
			else if (rnum >= 32)
				t = UINT;
			break;
		case 4:
			if (rnum >= 24)
				t = UCHAR;
			else if (rnum >= 16)
				t = USHORT;
			break;
		case 2:
			if (rnum >= 8)
				t = UCHAR;
			break;
		default:
			break;
		}
		if (t != ln->tn_type->t_tspec)
			ntn->tn_type->t_tspec = t;
		break;
#endif
	case INDIR:
	case FSEL:
		lint_assert(ln->tn_type->t_tspec == PTR);
		t = ln->tn_type->t_subt->t_tspec;
		if (t != FUNC && t != VOID)
			ntn->tn_lvalue = true;
		break;
	default:
		break;
	}

	return ntn;
}

/*
 * Performs the "integer promotions" (C99 6.3.1.1p2), which convert small
 * integer types to either int or unsigned int.
 *
 * If tflag is set or the operand is a function argument with no type
 * information (no prototype or variable # of args), converts float to double.
 */
tnode_t *
promote(op_t op, bool farg, tnode_t *tn)
{
	tspec_t	t;
	type_t	*ntp;
	unsigned int len;

	t = tn->tn_type->t_tspec;

	if (!is_arithmetic(t))
		return tn;

	if (!tflag) {
		/*
		 * C99 6.3.1.1p2 requires for types with lower rank than int
		 * that "If an int can represent all the values of the
		 * original type, the value is converted to an int; otherwise
		 * it is converted to an unsigned int", and that "All other
		 * types are unchanged by the integer promotions".
		 */
		if (tn->tn_type->t_bitfield) {
			len = tn->tn_type->t_flen;
			if (len < size_in_bits(INT)) {
				t = INT;
			} else if (len == size_in_bits(INT)) {
				t = is_uinteger(t) ? UINT : INT;
			}
		} else if (t == CHAR || t == UCHAR || t == SCHAR) {
			t = (size_in_bits(CHAR) < size_in_bits(INT)
			     || t != UCHAR) ? INT : UINT;
		} else if (t == SHORT || t == USHORT) {
			t = (size_in_bits(SHORT) < size_in_bits(INT)
			     || t == SHORT) ? INT : UINT;
		} else if (t == ENUM) {
			t = INT;
		} else if (farg && t == FLOAT) {
			t = DOUBLE;
		}
	} else {
		/*
		 * In traditional C, keep unsigned and promote FLOAT
		 * to DOUBLE.
		 */
		if (t == UCHAR || t == USHORT) {
			t = UINT;
		} else if (t == CHAR || t == SCHAR || t == SHORT) {
			t = INT;
		} else if (t == FLOAT) {
			t = DOUBLE;
		} else if (t == ENUM) {
			t = INT;
		}
	}

	if (t != tn->tn_type->t_tspec) {
		ntp = expr_dup_type(tn->tn_type);
		ntp->t_tspec = t;
		/*
		 * Keep t_is_enum even though t_tspec gets converted from
		 * ENUM to INT, so we are later able to check compatibility
		 * of enum types.
		 */
		tn = convert(op, 0, ntp, tn);
	}

	return tn;
}

/*
 * Apply the "usual arithmetic conversions" (C99 6.3.1.8).
 *
 * This gives both operands the same type.
 * This is done in different ways for traditional C and C90.
 */
static void
balance(op_t op, tnode_t **lnp, tnode_t **rnp)
{
	tspec_t	lt, rt, t;
	int	i;
	bool	u;
	type_t	*ntp;
	static const tspec_t tl[] = {
		LDOUBLE, DOUBLE, FLOAT,
#ifdef INT128_SIZE
		UINT128, INT128,
#endif
		UQUAD, QUAD,
		ULONG, LONG,
		UINT, INT,
	};

	lt = (*lnp)->tn_type->t_tspec;
	rt = (*rnp)->tn_type->t_tspec;

	if (!is_arithmetic(lt) || !is_arithmetic(rt))
		return;

	if (!tflag) {
		if (lt == rt) {
			t = lt;
		} else if (lt == LCOMPLEX || rt == LCOMPLEX) {
			t = LCOMPLEX;
		} else if (lt == DCOMPLEX || rt == DCOMPLEX) {
			t = DCOMPLEX;
		} else if (lt == FCOMPLEX || rt == FCOMPLEX) {
			t = FCOMPLEX;
		} else if (lt == LDOUBLE || rt == LDOUBLE) {
			t = LDOUBLE;
		} else if (lt == DOUBLE || rt == DOUBLE) {
			t = DOUBLE;
		} else if (lt == FLOAT || rt == FLOAT) {
			t = FLOAT;
		} else {
			/*
			 * If type A has more bits than type B it should
			 * be able to hold all possible values of type B.
			 */
			if (size_in_bits(lt) > size_in_bits(rt)) {
				t = lt;
			} else if (size_in_bits(lt) < size_in_bits(rt)) {
				t = rt;
			} else {
				for (i = 3; tl[i] != INT; i++) {
					if (tl[i] == lt || tl[i] == rt)
						break;
				}
				if ((is_uinteger(lt) || is_uinteger(rt)) &&
				    !is_uinteger(tl[i])) {
					i--;
				}
				t = tl[i];
			}
		}
	} else {
		/* Keep unsigned in traditional C */
		u = is_uinteger(lt) || is_uinteger(rt);
		for (i = 0; tl[i] != INT; i++) {
			if (lt == tl[i] || rt == tl[i])
				break;
		}
		t = tl[i];
		if (u && is_integer(t) && !is_uinteger(t))
			t = unsigned_type(t);
	}

	if (t != lt) {
		ntp = expr_dup_type((*lnp)->tn_type);
		ntp->t_tspec = t;
		*lnp = convert(op, 0, ntp, *lnp);
	}
	if (t != rt) {
		ntp = expr_dup_type((*rnp)->tn_type);
		ntp->t_tspec = t;
		*rnp = convert(op, 0, ntp, *rnp);
	}
}

/*
 * Insert a conversion operator, which converts the type of the node
 * to another given type.
 * If op is FARG, arg is the number of the argument (used for warnings).
 */
tnode_t *
convert(op_t op, int arg, type_t *tp, tnode_t *tn)
{
	tnode_t	*ntn;
	tspec_t	nt, ot;

	nt = tp->t_tspec;
	ot = tn->tn_type->t_tspec;

	if (!tflag && !sflag && op == FARG)
		check_prototype_conversion(arg, nt, ot, tp, tn);
	if (is_integer(nt) && is_integer(ot)) {
		check_integer_conversion(op, arg, nt, ot, tp, tn);
	} else if (nt == PTR && is_null_pointer(tn)) {
		/* a null pointer may be assigned to any pointer. */
	} else if (is_integer(nt) && nt != BOOL && ot == PTR) {
		check_pointer_integer_conversion(op, nt, tp, tn);
	} else if (nt == PTR && ot == PTR && op == CVT) {
		check_pointer_conversion(tn, tp);
	}

	ntn = expr_alloc_tnode();
	ntn->tn_op = CVT;
	ntn->tn_type = tp;
	ntn->tn_cast = op == CVT;
	ntn->tn_sys |= tn->tn_sys;
	ntn->tn_right = NULL;
	if (tn->tn_op != CON || nt == VOID) {
		ntn->tn_left = tn;
	} else {
		ntn->tn_op = CON;
		ntn->tn_val = expr_zero_alloc(sizeof(*ntn->tn_val));
		convert_constant(op, arg, ntn->tn_type, ntn->tn_val,
		    tn->tn_val);
	}

	return ntn;
}

static bool
should_warn_about_prototype_conversion(tspec_t nt,
				       tspec_t ot, const tnode_t *ptn)
{

	if (nt == ot)
		return false;

	if (nt == ENUM && ot == INT)
		return false;

	if (is_floating(nt) != is_floating(ot) ||
	    portable_size_in_bits(nt) != portable_size_in_bits(ot)) {
		/* representation and/or width change */
		if (!is_integer(ot))
			return true;
		/*
		 * XXX: Investigate whether this rule makes sense; see
		 * tests/usr.bin/xlint/lint1/platform_long.c.
		 */
		return portable_size_in_bits(ot) > portable_size_in_bits(INT);
	}

	if (!hflag)
		return false;

	/*
	 * If the types differ only in sign and the argument has the same
	 * representation in both types, print no warning.
	 */
	if (ptn->tn_op == CON && is_integer(nt) &&
	    signed_type(nt) == signed_type(ot) &&
	    !msb(ptn->tn_val->v_quad, ot))
		return false;

	return true;
}

/*
 * Warn if a prototype causes a type conversion that is different from what
 * would happen to the same argument in the absence of a prototype.  This
 * check is intended for code that needs to stay compatible with pre-C90 C.
 *
 * Errors/warnings about illegal type combinations are already printed
 * in check_assign_types_compatible().
 */
static void
check_prototype_conversion(int arg, tspec_t nt, tspec_t ot, type_t *tp,
			   tnode_t *tn)
{
	tnode_t	*ptn;

	if (!is_arithmetic(nt) || !is_arithmetic(ot))
		return;

	/*
	 * If the type of the formal parameter is char/short, a warning
	 * would be useless, because functions declared the old style
	 * can't expect char/short arguments.
	 */
	if (nt == CHAR || nt == SCHAR || nt == UCHAR ||
	    nt == SHORT || nt == USHORT)
		return;

	/* apply the default promotion */
	ptn = promote(NOOP, true, tn);
	ot = ptn->tn_type->t_tspec;

	if (should_warn_about_prototype_conversion(nt, ot, ptn)) {
		/* argument #%d is converted from '%s' to '%s' ... */
		warning(259, arg, type_name(tn->tn_type), type_name(tp));
	}
}

/*
 * Print warnings for conversions of integer types which may cause problems.
 */
static void
check_integer_conversion(op_t op, int arg, tspec_t nt, tspec_t ot, type_t *tp,
			 tnode_t *tn)
{

	if (tn->tn_op == CON)
		return;

	if (op == CVT)
		return;

	if (Sflag && nt == BOOL)
		return;		/* See C99 6.3.1.2 */

	if (Pflag && portable_size_in_bits(nt) > portable_size_in_bits(ot) &&
	    is_uinteger(nt) != is_uinteger(ot)) {
		if (aflag > 0 && pflag) {
			if (op == FARG) {
				/* conversion to '%s' may sign-extend ... */
				warning(297, type_name(tp), arg);
			} else {
				/* conversion to '%s' may sign-extend ... */
				warning(131, type_name(tp));
			}
		}
	}

	if (Pflag && portable_size_in_bits(nt) > portable_size_in_bits(ot)) {
		switch (tn->tn_op) {
		case PLUS:
		case MINUS:
		case MULT:
		case SHL:
			/* suggest cast from '%s' to '%s' on op %s to ... */
			warning(324, type_name(gettyp(ot)), type_name(tp),
			    op_name(tn->tn_op));
			break;
		default:
			break;
		}
	}

	if (portable_size_in_bits(nt) < portable_size_in_bits(ot) &&
	    (ot == LONG || ot == ULONG || ot == QUAD || ot == UQUAD ||
	     aflag > 1)) {
		/* conversion from '%s' may lose accuracy */
		if (aflag > 0) {
			if (op == FARG) {
				/* conversion from '%s' to '%s' may ... */
				warning(298,
				    type_name(tn->tn_type), type_name(tp), arg);
			} else {
				/* conversion from '%s' to '%s' may ... */
				warning(132,
				    type_name(tn->tn_type), type_name(tp));
			}
		}
	}
}

/*
 * Print warnings for dubious conversions of pointer to integer.
 */
static void
check_pointer_integer_conversion(op_t op, tspec_t nt, type_t *tp, tnode_t *tn)
{

	if (tn->tn_op == CON)
		return;
	if (op != CVT)
		return;		/* We got already an error. */
	if (portable_size_in_bits(nt) >= portable_size_in_bits(PTR))
		return;

	if (pflag && size_in_bits(nt) >= size_in_bits(PTR)) {
		/* conversion of pointer to '%s' may lose bits */
		warning(134, type_name(tp));
	} else {
		/* conversion of pointer to '%s' loses bits */
		warning(133, type_name(tp));
	}
}

static bool
should_warn_about_pointer_cast(const type_t *nstp, tspec_t nst,
			       const type_t *ostp, tspec_t ost)
{
	/*
	 * Casting a pointer to 'struct S' to a pointer to another struct that
	 * has 'struct S' as its first member is ok, see msg_247.c, 'struct
	 * counter'.
	 */
	if (nst == STRUCT && ost == STRUCT &&
	    nstp->t_str->sou_first_member != NULL &&
	    nstp->t_str->sou_first_member->s_type == ostp)
		return false;

	if (is_incomplete(nstp) || is_incomplete(ostp))
		return false;

	if ((nst == STRUCT || nst == UNION) && nstp->t_str != ostp->t_str)
		return true;

	if (nst == CHAR || nst == UCHAR)
		return false;	/* for the sake of traditional C code */
	if (ost == CHAR || ost == UCHAR)
		return false;	/* for the sake of traditional C code */

	return portable_size_in_bits(nst) != portable_size_in_bits(ost);
}

/*
 * Warn about questionable pointer conversions.
 */
static void
check_pointer_conversion(tnode_t *tn, type_t *ntp)
{
	const type_t *nstp, *otp, *ostp;
	tspec_t nst, ost;
	const char *nts, *ots;

	nstp = ntp->t_subt;
	otp = tn->tn_type;
	ostp = otp->t_subt;
	nst = nstp->t_tspec;
	ost = ostp->t_tspec;

	if (nst == VOID || ost == VOID) {
		if (sflag && (nst == FUNC || ost == FUNC)) {
			/* null pointers are already handled in convert() */
			*(nst == FUNC ? &nts : &ots) = "function pointer";
			*(nst == VOID ? &nts : &ots) = "'void *'";
			/* ANSI C forbids conversion of %s to %s */
			warning(303, ots, nts);
		}
		return;
	} else if (nst == FUNC && ost == FUNC) {
		return;
	} else if (nst == FUNC || ost == FUNC) {
		/* converting '%s' to '%s' is questionable */
		warning(229, type_name(otp), type_name(ntp));
		return;
	}

	if (hflag && alignment_in_bits(nstp) > alignment_in_bits(ostp) &&
	    ost != CHAR && ost != UCHAR &&
	    !is_incomplete(ostp)) {
		/* converting '%s' to '%s' may cause alignment problem */
		warning(135, type_name(otp), type_name(ntp));
	}

	if (cflag && should_warn_about_pointer_cast(nstp, nst, ostp, ost)) {
		/* pointer cast from '%s' to '%s' may be troublesome */
		warning(247, type_name(otp), type_name(ntp));
	}
}

static void
convert_constant_floating(op_t op, int arg, tspec_t ot, const type_t *tp,
			  tspec_t nt, val_t *v, val_t *nv)
{
	ldbl_t	max = 0.0, min = 0.0;

	switch (nt) {
	case CHAR:
		max = TARG_CHAR_MAX;	min = TARG_CHAR_MIN;	break;
	case UCHAR:
		max = TARG_UCHAR_MAX;	min = 0;		break;
	case SCHAR:
		max = TARG_SCHAR_MAX;	min = TARG_SCHAR_MIN;	break;
	case SHORT:
		max = TARG_SHRT_MAX;	min = TARG_SHRT_MIN;	break;
	case USHORT:
		max = TARG_USHRT_MAX;	min = 0;		break;
	case ENUM:
	case INT:
		max = TARG_INT_MAX;	min = TARG_INT_MIN;	break;
	case UINT:
		max = TARG_UINT_MAX;	min = 0;		break;
	case LONG:
		max = TARG_LONG_MAX;	min = TARG_LONG_MIN;	break;
	case ULONG:
		max = TARG_ULONG_MAX;	min = 0;		break;
	case QUAD:
		max = QUAD_MAX;		min = QUAD_MIN;		break;
	case UQUAD:
		max = UQUAD_MAX;	min = 0;		break;
	case FLOAT:
	case FCOMPLEX:
		max = FLT_MAX;		min = -FLT_MAX;		break;
	case DOUBLE:
	case DCOMPLEX:
		max = DBL_MAX;		min = -DBL_MAX;		break;
	case PTR:
		/* Got already an error because of float --> ptr */
	case LDOUBLE:
	case LCOMPLEX:
		/* LINTED 248 */
		max = LDBL_MAX;		min = -max;		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}
	if (v->v_ldbl > max || v->v_ldbl < min) {
		lint_assert(nt != LDOUBLE);
		if (op == FARG) {
			/* conversion of '%s' to '%s' is out of range, ... */
			warning(295,
			    type_name(gettyp(ot)), type_name(tp), arg);
		} else {
			/* conversion of '%s' to '%s' is out of range */
			warning(119,
			    type_name(gettyp(ot)), type_name(tp));
		}
		v->v_ldbl = v->v_ldbl > 0 ? max : min;
	}

	if (nt == FLOAT) {
		nv->v_ldbl = (float)v->v_ldbl;
	} else if (nt == DOUBLE) {
		nv->v_ldbl = (double)v->v_ldbl;
	} else if (nt == LDOUBLE) {
		nv->v_ldbl = v->v_ldbl;
	} else {
		nv->v_quad = (int64_t)v->v_ldbl;
	}
}

static bool
convert_constant_to_floating(tspec_t nt, val_t *nv,
			     tspec_t ot, const val_t *v)
{
	if (nt == FLOAT) {
		nv->v_ldbl = (ot == PTR || is_uinteger(ot)) ?
		    (float)(uint64_t)v->v_quad : (float)v->v_quad;
	} else if (nt == DOUBLE) {
		nv->v_ldbl = (ot == PTR || is_uinteger(ot)) ?
		    (double)(uint64_t)v->v_quad : (double)v->v_quad;
	} else if (nt == LDOUBLE) {
		nv->v_ldbl = (ot == PTR || is_uinteger(ot)) ?
		    (ldbl_t)(uint64_t)v->v_quad : (ldbl_t)v->v_quad;
	} else
		return false;
	return true;
}

/*
 * Print a warning if bits which were set are lost due to the conversion.
 * This can happen with operator ORASS only.
 */
static void
convert_constant_check_range_bitor(size_t nsz, size_t osz, const val_t *v,
				   uint64_t xmask, op_t op)
{
	if (nsz < osz && (v->v_quad & xmask) != 0) {
		/* constant truncated by conversion, op %s */
		warning(306, op_name(op));
	}
}

/*
 * Print a warning if additional bits are not all 1
 * and the most significant bit of the old value is 1,
 * or if at least one (but not all) removed bit was 0.
 */
static void
convert_constant_check_range_bitand(size_t nsz, size_t osz,
				    uint64_t xmask, const val_t *nv,
				    tspec_t ot, const val_t *v,
				    const type_t *tp, op_t op)
{
	if (nsz > osz &&
	    (nv->v_quad & bit((unsigned int)(osz - 1))) != 0 &&
	    (nv->v_quad & xmask) != xmask) {
		/* extra bits set to 0 in conversion of '%s' to '%s', ... */
		warning(309, type_name(gettyp(ot)),
		    type_name(tp), op_name(op));
	} else if (nsz < osz &&
		   (v->v_quad & xmask) != xmask &&
		   (v->v_quad & xmask) != 0) {
		/* constant truncated by conversion, op %s */
		warning(306, op_name(op));
	}
}

static void
convert_constant_check_range_signed(op_t op, int arg)
{
	if (op == ASSIGN) {
		/* assignment of negative constant to unsigned type */
		warning(164);
	} else if (op == INIT) {
		/* initialization of unsigned with negative constant */
		warning(221);
	} else if (op == FARG) {
		/* conversion of negative constant to unsigned type, ... */
		warning(296, arg);
	} else if (modtab[op].m_comparison) {
		/* handled by check_integer_comparison() */
	} else {
		/* conversion of negative constant to unsigned type */
		warning(222);
	}
}

/*
 * Loss of significant bit(s). All truncated bits
 * of unsigned types or all truncated bits plus the
 * msb of the target for signed types are considered
 * to be significant bits. Loss of significant bits
 * means that at least one of the bits was set in an
 * unsigned type or that at least one but not all of
 * the bits was set in a signed type.
 * Loss of significant bits means that it is not
 * possible, also not with necessary casts, to convert
 * back to the original type. A example for a
 * necessary cast is:
 *	char c;	int	i; c = 128;
 *	i = c;			** yields -128 **
 *	i = (unsigned char)c;	** yields 128 **
 */
static void
convert_constant_check_range_truncated(op_t op, int arg, const type_t *tp,
				       tspec_t ot)
{
	if (op == ASSIGN && tp->t_bitfield) {
		/* precision lost in bit-field assignment */
		warning(166);
	} else if (op == ASSIGN) {
		/* constant truncated by assignment */
		warning(165);
	} else if (op == INIT && tp->t_bitfield) {
		/* bit-field initializer does not fit */
		warning(180);
	} else if (op == INIT) {
		/* initializer does not fit */
		warning(178);
	} else if (op == CASE) {
		/* case label affected by conversion */
		warning(196);
	} else if (op == FARG) {
		/* conversion of '%s' to '%s' is out of range, arg #%d */
		warning(295,
		    type_name(gettyp(ot)), type_name(tp), arg);
	} else {
		/* conversion of '%s' to '%s' is out of range */
		warning(119,
		    type_name(gettyp(ot)), type_name(tp));
	}
}

static void
convert_constant_check_range_loss(op_t op, int arg, const type_t *tp,
				  tspec_t ot)
{
	if (op == ASSIGN && tp->t_bitfield) {
		/* precision lost in bit-field assignment */
		warning(166);
	} else if (op == INIT && tp->t_bitfield) {
		/* bit-field initializer out of range */
		warning(11);
	} else if (op == CASE) {
		/* case label affected by conversion */
		warning(196);
	} else if (op == FARG) {
		/* conversion of '%s' to '%s' is out of range, arg #%d */
		warning(295,
		    type_name(gettyp(ot)), type_name(tp), arg);
	} else {
		/* conversion of '%s' to '%s' is out of range */
		warning(119,
		    type_name(gettyp(ot)), type_name(tp));
	}
}

static void
convert_constant_check_range(tspec_t ot, const type_t *tp, tspec_t nt,
			     op_t op, int arg, const val_t *v, val_t *nv)
{
	unsigned int osz, nsz;
	uint64_t xmask, xmsk1;

	osz = size_in_bits(ot);
	nsz = tp->t_bitfield ? tp->t_flen : size_in_bits(nt);
	xmask = value_bits(nsz) ^ value_bits(osz);
	xmsk1 = value_bits(nsz) ^ value_bits(osz - 1);
	/*
	 * For bitwise operations we are not interested in the
	 * value, but in the bits itself.
	 */
	if (op == ORASS || op == BITOR || op == BITXOR) {
		convert_constant_check_range_bitor(nsz, osz, v, xmask, op);
	} else if (op == ANDASS || op == BITAND) {
		convert_constant_check_range_bitand(nsz, osz, xmask, nv, ot,
		    v, tp, op);
	} else if ((nt != PTR && is_uinteger(nt)) &&
		   (ot != PTR && !is_uinteger(ot)) &&
		   v->v_quad < 0) {
		convert_constant_check_range_signed(op, arg);
	} else if (nv->v_quad != v->v_quad && nsz <= osz &&
		   (v->v_quad & xmask) != 0 &&
		   (is_uinteger(ot) || (v->v_quad & xmsk1) != xmsk1)) {
		convert_constant_check_range_truncated(op, arg, tp, ot);
	} else if (nv->v_quad != v->v_quad) {
		convert_constant_check_range_loss(op, arg, tp, ot);
	}
}

/*
 * Converts a typed constant to a constant of another type.
 *
 * op		operator which requires conversion
 * arg		if op is FARG, # of argument
 * tp		type in which to convert the constant
 * nv		new constant
 * v		old constant
 */
void
convert_constant(op_t op, int arg, const type_t *tp, val_t *nv, val_t *v)
{
	tspec_t ot, nt;
	unsigned int sz;
	bool range_check;

	/*
	 * TODO: make 'v' const; the name of this function does not suggest
	 *  that it modifies 'v'.
	 */
	ot = v->v_tspec;
	nt = nv->v_tspec = tp->t_tspec;
	range_check = false;

	if (nt == BOOL) {	/* C99 6.3.1.2 */
		nv->v_unsigned_since_c90 = false;
		nv->v_quad = is_nonzero_val(v) ? 1 : 0;
		return;
	}

	if (ot == FLOAT || ot == DOUBLE || ot == LDOUBLE) {
		convert_constant_floating(op, arg, ot, tp, nt, v, nv);
	} else if (!convert_constant_to_floating(nt, nv, ot, v)) {
		range_check = true;	/* Check for lost precision. */
		nv->v_quad = v->v_quad;
	}

	if ((v->v_unsigned_since_c90 && is_floating(nt)) ||
	    (v->v_unsigned_since_c90 && (is_integer(nt) && !is_uinteger(nt) &&
			    portable_size_in_bits(nt) >
			    portable_size_in_bits(ot)))) {
		/* ANSI C treats constant as unsigned */
		warning(157);
		v->v_unsigned_since_c90 = false;
	}

	if (is_integer(nt)) {
		sz = tp->t_bitfield ? tp->t_flen : size_in_bits(nt);
		nv->v_quad = convert_integer(nv->v_quad, nt, sz);
	}

	if (range_check && op != CVT)
		convert_constant_check_range(ot, tp, nt, op, arg, v, nv);
}

/*
 * Called if incompatible types were detected.
 * Prints a appropriate warning.
 */
static void
warn_incompatible_types(op_t op,
			const type_t *ltp, tspec_t lt,
			const type_t *rtp, tspec_t rt)
{
	const mod_t *mp;

	mp = &modtab[op];

	if (lt == VOID || (mp->m_binary && rt == VOID)) {
		/* void type illegal in expression */
		error(109);
	} else if (op == ASSIGN) {
		if ((lt == STRUCT || lt == UNION) &&
		    (rt == STRUCT || rt == UNION)) {
			/* assignment of different structures (%s != %s) */
			error(240, tspec_name(lt), tspec_name(rt));
		} else {
			/* cannot assign to '%s' from '%s' */
			error(171, type_name(ltp), type_name(rtp));
		}
	} else if (mp->m_binary) {
		/* operands of '%s' have incompatible types (%s != %s) */
		error(107, mp->m_name, tspec_name(lt), tspec_name(rt));
	} else {
		lint_assert(rt == NOTSPEC);
		/* operand of '%s' has invalid type (%s) */
		error(108, mp->m_name, tspec_name(lt));
	}
}

/*
 * Called if incompatible pointer types are detected.
 * Print an appropriate warning.
 */
static void
warn_incompatible_pointers(const mod_t *mp,
			   const type_t *ltp, const type_t *rtp)
{
	tspec_t	lt, rt;

	lint_assert(ltp->t_tspec == PTR);
	lint_assert(rtp->t_tspec == PTR);

	lt = ltp->t_subt->t_tspec;
	rt = rtp->t_subt->t_tspec;

	if ((lt == STRUCT || lt == UNION) && (rt == STRUCT || rt == UNION)) {
		if (mp == NULL) {
			/* illegal structure pointer combination */
			warning(244);
		} else {
			/* incompatible structure pointers: '%s' '%s' '%s' */
			warning(245, type_name(ltp), mp->m_name, type_name(rtp));
		}
	} else {
		if (mp == NULL) {
			/* illegal combination of '%s' and '%s' */
			warning(184, type_name(ltp), type_name(rtp));
		} else {
			/* illegal combination of '%s' and '%s', op '%s' */
			warning(124,
			    type_name(ltp), type_name(rtp), mp->m_name);
		}
	}
}

/* Return a type based on tp1, with added qualifiers from tp2. */
static type_t *
merge_qualifiers(type_t *tp1, const type_t *tp2)
{
	type_t *ntp, *nstp;
	bool c1, c2, v1, v2;

	lint_assert(tp1->t_tspec == PTR);
	lint_assert(tp2->t_tspec == PTR);

	c1 = tp1->t_subt->t_const;
	c2 = tp2->t_subt->t_const;
	v1 = tp1->t_subt->t_volatile;
	v2 = tp2->t_subt->t_volatile;

	if (c1 == (c1 | c2) && v1 == (v1 | v2))
		return tp1;

	nstp = expr_dup_type(tp1->t_subt);
	nstp->t_const |= c2;
	nstp->t_volatile |= v2;

	ntp = expr_dup_type(tp1);
	ntp->t_subt = nstp;
	return ntp;
}

/*
 * Returns true if the given structure or union has a constant member
 * (maybe recursively).
 */
static bool
has_constant_member(const type_t *tp)
{
	sym_t *m;

	lint_assert(is_struct_or_union(tp->t_tspec));

	for (m = tp->t_str->sou_first_member; m != NULL; m = m->s_next) {
		const type_t *mtp = m->s_type;
		if (mtp->t_const)
			return true;
		if (is_struct_or_union(mtp->t_tspec) &&
		    has_constant_member(mtp))
			return true;
	}
	return false;
}

/*
 * Create a new node for one of the operators POINT and ARROW.
 */
static tnode_t *
build_struct_access(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{
	tnode_t	*ntn, *ctn;
	bool	nolval;

	lint_assert(rn->tn_op == NAME);
	lint_assert(rn->tn_sym->s_value.v_tspec == INT);
	lint_assert(rn->tn_sym->s_scl == MOS || rn->tn_sym->s_scl == MOU);

	/*
	 * Remember if the left operand is an lvalue (structure members
	 * are lvalues if and only if the structure itself is an lvalue).
	 */
	nolval = op == POINT && !ln->tn_lvalue;

	if (op == POINT) {
		ln = build_address(sys, ln, true);
	} else if (ln->tn_type->t_tspec != PTR) {
		lint_assert(tflag);
		lint_assert(is_integer(ln->tn_type->t_tspec));
		ln = convert(NOOP, 0, expr_derive_type(gettyp(VOID), PTR), ln);
	}

	ctn = build_integer_constant(PTRDIFF_TSPEC,
	    rn->tn_sym->s_value.v_quad / CHAR_SIZE);

	ntn = new_tnode(PLUS, sys, expr_derive_type(rn->tn_type, PTR),
	    ln, ctn);
	if (ln->tn_op == CON)
		ntn = fold(ntn);

	if (rn->tn_type->t_bitfield) {
		ntn = new_tnode(FSEL, sys, ntn->tn_type->t_subt, ntn, NULL);
	} else {
		ntn = new_tnode(INDIR, sys, ntn->tn_type->t_subt, ntn, NULL);
	}

	if (nolval)
		ntn->tn_lvalue = false;

	return ntn;
}

/*
 * Create a node for INCAFT, INCBEF, DECAFT and DECBEF.
 */
static tnode_t *
build_prepost_incdec(op_t op, bool sys, tnode_t *ln)
{
	tnode_t	*cn, *ntn;

	lint_assert(ln != NULL);

	if (ln->tn_type->t_tspec == PTR) {
		cn = plength(ln->tn_type);
	} else {
		cn = build_integer_constant(INT, (int64_t)1);
	}
	ntn = new_tnode(op, sys, ln->tn_type, ln, cn);

	return ntn;
}

/*
 * Create a node for REAL, IMAG
 */
static tnode_t *
build_real_imag(op_t op, bool sys, tnode_t *ln)
{
	tnode_t	*cn, *ntn;

	lint_assert(ln != NULL);

	if (ln->tn_op == NAME) {
		/*
		 * This may be too much, but it avoids wrong warnings.
		 * See d_c99_complex_split.c.
		 */
		mark_as_used(ln->tn_sym, false, false);
		mark_as_set(ln->tn_sym);
	}

	switch (ln->tn_type->t_tspec) {
	case LCOMPLEX:
		/* XXX: integer and LDOUBLE don't match. */
		cn = build_integer_constant(LDOUBLE, (int64_t)1);
		break;
	case DCOMPLEX:
		/* XXX: integer and DOUBLE don't match. */
		cn = build_integer_constant(DOUBLE, (int64_t)1);
		break;
	case FCOMPLEX:
		/* XXX: integer and FLOAT don't match. */
		cn = build_integer_constant(FLOAT, (int64_t)1);
		break;
	default:
		/* __%s__ is illegal for type %s */
		error(276, op == REAL ? "real" : "imag",
		    type_name(ln->tn_type));
		return NULL;
	}
	ntn = new_tnode(op, sys, cn->tn_type, ln, cn);
	ntn->tn_lvalue = true;

	return ntn;
}

/*
 * Create a tree node for the unary & operator
 */
static tnode_t *
build_address(bool sys, tnode_t *tn, bool noign)
{
	tspec_t	t;

	if (!noign && ((t = tn->tn_type->t_tspec) == ARRAY || t == FUNC)) {
		if (tflag)
			/* '&' before array or function: ignored */
			warning(127);
		return tn;
	}

	/* eliminate &* */
	if (tn->tn_op == INDIR &&
	    tn->tn_left->tn_type->t_tspec == PTR &&
	    tn->tn_left->tn_type->t_subt == tn->tn_type) {
		return tn->tn_left;
	}

	return new_tnode(ADDR, sys, expr_derive_type(tn->tn_type, PTR),
	    tn, NULL);
}

/*
 * Create a node for operators PLUS and MINUS.
 */
static tnode_t *
build_plus_minus(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{
	tnode_t	*ntn, *ctn;
	type_t	*tp;

	/* If pointer and integer, then pointer to the lhs. */
	if (rn->tn_type->t_tspec == PTR && is_integer(ln->tn_type->t_tspec)) {
		ntn = ln;
		ln = rn;
		rn = ntn;
	}

	if (ln->tn_type->t_tspec == PTR && rn->tn_type->t_tspec != PTR) {
		lint_assert(is_integer(rn->tn_type->t_tspec));

		check_ctype_macro_invocation(ln, rn);
		check_enum_array_index(ln, rn);

		ctn = plength(ln->tn_type);
		if (rn->tn_type->t_tspec != ctn->tn_type->t_tspec)
			rn = convert(NOOP, 0, ctn->tn_type, rn);
		rn = new_tnode(MULT, sys, rn->tn_type, rn, ctn);
		if (rn->tn_left->tn_op == CON)
			rn = fold(rn);
		ntn = new_tnode(op, sys, ln->tn_type, ln, rn);

	} else if (rn->tn_type->t_tspec == PTR) {

		lint_assert(ln->tn_type->t_tspec == PTR);
		lint_assert(op == MINUS);
		tp = gettyp(PTRDIFF_TSPEC);
		ntn = new_tnode(op, sys, tp, ln, rn);
		if (ln->tn_op == CON && rn->tn_op == CON)
			ntn = fold(ntn);
		ctn = plength(ln->tn_type);
		balance(NOOP, &ntn, &ctn);
		ntn = new_tnode(DIV, sys, tp, ntn, ctn);

	} else {

		ntn = new_tnode(op, sys, ln->tn_type, ln, rn);

	}
	return ntn;
}

/*
 * Create a node for operators SHL and SHR.
 */
static tnode_t *
build_bit_shift(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{
	tspec_t	t;
	tnode_t	*ntn;

	if ((t = rn->tn_type->t_tspec) != INT && t != UINT)
		rn = convert(CVT, 0, gettyp(INT), rn);
	ntn = new_tnode(op, sys, ln->tn_type, ln, rn);
	return ntn;
}

/*
 * Create a node for COLON.
 */
static tnode_t *
build_colon(bool sys, tnode_t *ln, tnode_t *rn)
{
	tspec_t	lt, rt, pdt;
	type_t	*tp;
	tnode_t	*ntn;

	lt = ln->tn_type->t_tspec;
	rt = rn->tn_type->t_tspec;
	pdt = PTRDIFF_TSPEC;

	/*
	 * Arithmetic types are balanced, all other type combinations
	 * still need to be handled.
	 */
	if (is_arithmetic(lt) && is_arithmetic(rt)) {
		tp = ln->tn_type;
	} else if (lt == BOOL && rt == BOOL) {
		tp = ln->tn_type;
	} else if (lt == VOID || rt == VOID) {
		tp = gettyp(VOID);
	} else if (lt == STRUCT || lt == UNION) {
		/* Both types must be identical. */
		lint_assert(rt == STRUCT || rt == UNION);
		lint_assert(ln->tn_type->t_str == rn->tn_type->t_str);
		if (is_incomplete(ln->tn_type)) {
			/* unknown operand size, op %s */
			error(138, op_name(COLON));
			return NULL;
		}
		tp = ln->tn_type;
	} else if (lt == PTR && is_integer(rt)) {
		if (rt != pdt) {
			rn = convert(NOOP, 0, gettyp(pdt), rn);
			rt = pdt;
		}
		tp = ln->tn_type;
	} else if (rt == PTR && is_integer(lt)) {
		if (lt != pdt) {
			ln = convert(NOOP, 0, gettyp(pdt), ln);
			lt = pdt;
		}
		tp = rn->tn_type;
	} else if (lt == PTR && ln->tn_type->t_subt->t_tspec == VOID) {
		tp = merge_qualifiers(rn->tn_type, ln->tn_type);
	} else if (rt == PTR && rn->tn_type->t_subt->t_tspec == VOID) {
		tp = merge_qualifiers(ln->tn_type, rn->tn_type);
	} else {
		/*
		 * XXX For now we simply take the left type. This is
		 * probably wrong, if one type contains a function prototype
		 * and the other one, at the same place, only an old style
		 * declaration.
		 */
		tp = merge_qualifiers(ln->tn_type, rn->tn_type);
	}

	ntn = new_tnode(COLON, sys, tp, ln, rn);

	return ntn;
}

/*
 * Create a node for an assignment operator (both = and op= ).
 */
static tnode_t *
build_assignment(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{
	tspec_t	lt, rt;
	tnode_t	*ntn, *ctn;

	lint_assert(ln != NULL);
	lint_assert(rn != NULL);

	lt = ln->tn_type->t_tspec;
	rt = rn->tn_type->t_tspec;

	if ((op == ADDASS || op == SUBASS) && lt == PTR) {
		lint_assert(is_integer(rt));
		ctn = plength(ln->tn_type);
		if (rn->tn_type->t_tspec != ctn->tn_type->t_tspec)
			rn = convert(NOOP, 0, ctn->tn_type, rn);
		rn = new_tnode(MULT, sys, rn->tn_type, rn, ctn);
		if (rn->tn_left->tn_op == CON)
			rn = fold(rn);
	}

	if ((op == ASSIGN || op == RETURN || op == INIT) &&
	    (lt == STRUCT || rt == STRUCT)) {
		lint_assert(lt == rt);
		lint_assert(ln->tn_type->t_str == rn->tn_type->t_str);
		if (is_incomplete(ln->tn_type)) {
			if (op == RETURN) {
				/* cannot return incomplete type */
				error(212);
			} else {
				/* unknown operand size, op %s */
				error(138, op_name(op));
			}
			return NULL;
		}
	}

	if (op == SHLASS) {
		if (portable_size_in_bits(lt) < portable_size_in_bits(rt)) {
			if (hflag)
				/* semantics of '%s' change in ANSI C; ... */
				warning(118, "<<=");
		}
	} else if (op != SHRASS) {
		if (op == ASSIGN || lt != PTR) {
			if (lt != rt ||
			    (ln->tn_type->t_bitfield && rn->tn_op == CON)) {
				rn = convert(op, 0, ln->tn_type, rn);
				rt = lt;
			}
		}
	}

	ntn = new_tnode(op, sys, ln->tn_type, ln, rn);

	return ntn;
}

/*
 * Get length of type tp->t_subt, as a constant expression of type ptrdiff_t
 * as seen from the target platform.
 */
static tnode_t *
plength(type_t *tp)
{
	int elem, elsz_in_bits;

	lint_assert(tp->t_tspec == PTR);
	tp = tp->t_subt;

	elem = 1;
	elsz_in_bits = 0;

	while (tp->t_tspec == ARRAY) {
		elem *= tp->t_dim;
		tp = tp->t_subt;
	}

	switch (tp->t_tspec) {
	case FUNC:
		/* pointer to function is not allowed here */
		error(110);
		break;
	case VOID:
		/* cannot do pointer arithmetic on operand of unknown size */
		gnuism(136);
		break;
	case STRUCT:
	case UNION:
		if ((elsz_in_bits = tp->t_str->sou_size_in_bits) == 0)
			/* cannot do pointer arithmetic on operand of ... */
			error(136);
		break;
	case ENUM:
		if (is_incomplete(tp)) {
			/* cannot do pointer arithmetic on operand of ... */
			warning(136);
		}
		/* FALLTHROUGH */
	default:
		if ((elsz_in_bits = size_in_bits(tp->t_tspec)) == 0) {
			/* cannot do pointer arithmetic on operand of ... */
			error(136);
		} else {
			lint_assert(elsz_in_bits != -1);
		}
		break;
	}

	if (elem == 0 && elsz_in_bits != 0) {
		/* cannot do pointer arithmetic on operand of unknown size */
		error(136);
	}

	if (elsz_in_bits == 0)
		elsz_in_bits = CHAR_SIZE;

	return build_integer_constant(PTRDIFF_TSPEC,
	    (int64_t)(elem * elsz_in_bits / CHAR_SIZE));
}

/*
 * XXX
 * Note: There appear to be a number of bugs in detecting overflow in
 * this function. An audit and a set of proper regression tests are needed.
 *     --Perry Metzger, Nov. 16, 2001
 */
/*
 * Do only as much as necessary to compute constant expressions.
 * Called only if the operator allows folding and all operands are constants.
 */
static tnode_t *
fold(tnode_t *tn)
{
	val_t *v;
	tspec_t t;
	bool utyp, ovfl;
	int64_t sl, sr = 0, q = 0, mask;
	uint64_t ul, ur = 0;
	tnode_t *cn;

	v = xcalloc(1, sizeof(*v));
	v->v_tspec = tn->tn_type->t_tspec;

	t = tn->tn_left->tn_type->t_tspec;
	utyp = !is_integer(t) || is_uinteger(t);
	ul = sl = tn->tn_left->tn_val->v_quad;
	if (is_binary(tn))
		ur = sr = tn->tn_right->tn_val->v_quad;

	mask = value_bits(size_in_bits(t));
	ovfl = false;

	switch (tn->tn_op) {
	case UPLUS:
		q = sl;
		break;
	case UMINUS:
		q = -sl;
		if (sl != 0 && msb(q, t) == msb(sl, t))
			ovfl = true;
		break;
	case COMPL:
		q = ~sl;
		break;
	case MULT:
		if (utyp) {
			q = ul * ur;
			if (q != (q & mask))
				ovfl = true;
			else if ((ul != 0) && ((q / ul) != ur))
				ovfl = true;
		} else {
			q = sl * sr;
			if (msb(q, t) != (msb(sl, t) ^ msb(sr, t)))
				ovfl = true;
		}
		break;
	case DIV:
		if (sr == 0) {
			/* division by 0 */
			error(139);
			q = utyp ? -1 : INT64_MAX;
		} else {
			q = utyp ? (int64_t)(ul / ur) : sl / sr;
		}
		break;
	case MOD:
		if (sr == 0) {
			/* modulus by 0 */
			error(140);
			q = 0;
		} else {
			q = utyp ? (int64_t)(ul % ur) : sl % sr;
		}
		break;
	case PLUS:
		q = utyp ? (int64_t)(ul + ur) : sl + sr;
		if (msb(sl, t) && msb(sr, t) && !msb(q, t))
			ovfl = true;
		if (!utyp && !msb(sl, t) && !msb(sr, t) && msb(q, t))
			ovfl = true;
		break;
	case MINUS:
		q = utyp ? (int64_t)(ul - ur) : sl - sr;
		if (!utyp && msb(sl, t) && !msb(sr, t) && !msb(q, t))
			ovfl = true;
		if (!msb(sl, t) && msb(sr, t) && msb(q, t))
			ovfl = true;
		break;
	case SHL:
		q = utyp ? (int64_t)(ul << sr) : sl << sr;
		break;
	case SHR:
		/*
		 * The sign must be explicitly extended because
		 * shifts of signed values are implementation dependent.
		 */
		q = ul >> sr;
		q = convert_integer(q, t, size_in_bits(t) - (int)sr);
		break;
	case LT:
		q = (utyp ? ul < ur : sl < sr) ? 1 : 0;
		break;
	case LE:
		q = (utyp ? ul <= ur : sl <= sr) ? 1 : 0;
		break;
	case GE:
		q = (utyp ? ul >= ur : sl >= sr) ? 1 : 0;
		break;
	case GT:
		q = (utyp ? ul > ur : sl > sr) ? 1 : 0;
		break;
	case EQ:
		q = (utyp ? ul == ur : sl == sr) ? 1 : 0;
		break;
	case NE:
		q = (utyp ? ul != ur : sl != sr) ? 1 : 0;
		break;
	case BITAND:
		q = utyp ? (int64_t)(ul & ur) : sl & sr;
		break;
	case BITXOR:
		q = utyp ? (int64_t)(ul ^ ur) : sl ^ sr;
		break;
	case BITOR:
		q = utyp ? (int64_t)(ul | ur) : sl | sr;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}

	/* XXX does not work for quads. */
	if (ovfl ||
	    ((uint64_t)(q | mask) != ~(uint64_t)0 && (q & ~mask) != 0)) {
		if (hflag)
			/* integer overflow detected, op '%s' */
			warning(141, op_name(tn->tn_op));
	}

	v->v_quad = convert_integer(q, t, 0);

	cn = build_constant(tn->tn_type, v);
	if (tn->tn_left->tn_system_dependent)
		cn->tn_system_dependent = true;
	if (is_binary(tn) && tn->tn_right->tn_system_dependent)
		cn->tn_system_dependent = true;

	return cn;
}

/*
 * Fold constant nodes, as much as is needed for comparing the value with 0
 * (test context, for controlling expressions).
 */
static tnode_t *
fold_test(tnode_t *tn)
{
	bool	l, r;
	val_t	*v;

	v = xcalloc(1, sizeof(*v));
	v->v_tspec = tn->tn_type->t_tspec;
	lint_assert(v->v_tspec == INT || (Tflag && v->v_tspec == BOOL));

	l = constant_is_nonzero(tn->tn_left);
	r = is_binary(tn) && constant_is_nonzero(tn->tn_right);

	switch (tn->tn_op) {
	case NOT:
		if (hflag && !constcond_flag)
			/* constant argument to '!' */
			warning(239);
		v->v_quad = !l ? 1 : 0;
		break;
	case LOGAND:
		v->v_quad = l && r ? 1 : 0;
		break;
	case LOGOR:
		v->v_quad = l || r ? 1 : 0;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}

	return build_constant(tn->tn_type, v);
}

static ldbl_t
floating_error_value(tspec_t t, ldbl_t lv)
{
	if (t == FLOAT) {
		return lv < 0 ? -FLT_MAX : FLT_MAX;
	} else if (t == DOUBLE) {
		return lv < 0 ? -DBL_MAX : DBL_MAX;
	} else {
		/* LINTED 248: floating-point constant out of range */
		ldbl_t max = LDBL_MAX;
		return lv < 0 ? -max : max;
	}
}

/*
 * Fold constant nodes having operands with floating point type.
 */
static tnode_t *
fold_float(tnode_t *tn)
{
	val_t	*v;
	tspec_t	t;
	ldbl_t	lv, rv = 0;

	fpe = 0;
	v = xcalloc(1, sizeof(*v));
	v->v_tspec = t = tn->tn_type->t_tspec;

	lint_assert(is_floating(t));
	lint_assert(t == tn->tn_left->tn_type->t_tspec);
	lint_assert(!is_binary(tn) || t == tn->tn_right->tn_type->t_tspec);

	lv = tn->tn_left->tn_val->v_ldbl;
	if (is_binary(tn))
		rv = tn->tn_right->tn_val->v_ldbl;

	switch (tn->tn_op) {
	case UPLUS:
		v->v_ldbl = lv;
		break;
	case UMINUS:
		v->v_ldbl = -lv;
		break;
	case MULT:
		v->v_ldbl = lv * rv;
		break;
	case DIV:
		if (rv == 0.0) {
			/* division by 0 */
			error(139);
			v->v_ldbl = floating_error_value(t, lv);
		} else {
			v->v_ldbl = lv / rv;
		}
		break;
	case PLUS:
		v->v_ldbl = lv + rv;
		break;
	case MINUS:
		v->v_ldbl = lv - rv;
		break;
	case LT:
		v->v_quad = lv < rv ? 1 : 0;
		break;
	case LE:
		v->v_quad = lv <= rv ? 1 : 0;
		break;
	case GE:
		v->v_quad = lv >= rv ? 1 : 0;
		break;
	case GT:
		v->v_quad = lv > rv ? 1 : 0;
		break;
	case EQ:
		v->v_quad = lv == rv ? 1 : 0;
		break;
	case NE:
		v->v_quad = lv != rv ? 1 : 0;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}

	lint_assert(fpe != 0 || isnan((double)v->v_ldbl) == 0);
	if (fpe != 0 || isfinite((double)v->v_ldbl) == 0 ||
	    (t == FLOAT &&
	     (v->v_ldbl > FLT_MAX || v->v_ldbl < -FLT_MAX)) ||
	    (t == DOUBLE &&
	     (v->v_ldbl > DBL_MAX || v->v_ldbl < -DBL_MAX))) {
		/* floating point overflow detected, op %s */
		warning(142, op_name(tn->tn_op));
		v->v_ldbl = floating_error_value(t, v->v_ldbl);
		fpe = 0;
	}

	return build_constant(tn->tn_type, v);
}


/*
 * Create a constant node for sizeof.
 */
tnode_t *
build_sizeof(const type_t *tp)
{
	unsigned int size_in_bytes = type_size_in_bits(tp) / CHAR_SIZE;
	tnode_t *tn = build_integer_constant(SIZEOF_TSPEC, size_in_bytes);
	tn->tn_system_dependent = true;
	return tn;
}

/*
 * Create a constant node for offsetof.
 */
/* ARGSUSED */ /* See implementation comments. */
tnode_t *
build_offsetof(const type_t *tp, const sym_t *sym)
{
	unsigned int offset_in_bytes;
	tnode_t *tn;

	if (!is_struct_or_union(tp->t_tspec))
		/* unacceptable operand of '%s' */
		error(111, "offsetof");

	/* XXX: wrong size, no checking for sym fixme */
	offset_in_bytes = type_size_in_bits(tp) / CHAR_SIZE;
	tn = build_integer_constant(SIZEOF_TSPEC, offset_in_bytes);
	tn->tn_system_dependent = true;
	return tn;
}

unsigned int
type_size_in_bits(const type_t *tp)
{
	unsigned int elem, elsz;
	bool	flex;

	elem = 1;
	flex = false;
	while (tp->t_tspec == ARRAY) {
		flex = true;	/* allow c99 flex arrays [] [0] */
		elem *= tp->t_dim;
		tp = tp->t_subt;
	}
	if (elem == 0) {
		if (!flex) {
			/* cannot take size/alignment of incomplete type */
			error(143);
			elem = 1;
		}
	}
	switch (tp->t_tspec) {
	case FUNC:
		/* cannot take size/alignment of function */
		error(144);
		elsz = 1;
		break;
	case STRUCT:
	case UNION:
		if (is_incomplete(tp)) {
			/* cannot take size/alignment of incomplete type */
			error(143);
			elsz = 1;
		} else {
			elsz = tp->t_str->sou_size_in_bits;
		}
		break;
	case ENUM:
		if (is_incomplete(tp)) {
			/* cannot take size/alignment of incomplete type */
			warning(143);
		}
		/* FALLTHROUGH */
	default:
		if (tp->t_bitfield) {
			/* cannot take size/alignment of bit-field */
			error(145);
		}
		if (tp->t_tspec == VOID) {
			/* cannot take size/alignment of void */
			error(146);
			elsz = 1;
		} else {
			elsz = size_in_bits(tp->t_tspec);
			lint_assert(elsz > 0);
		}
		break;
	}

	return elem * elsz;
}

tnode_t *
build_alignof(const type_t *tp)
{
	switch (tp->t_tspec) {
	case ARRAY:
		break;

	case FUNC:
		/* cannot take size/alignment of function */
		error(144);
		return 0;

	case STRUCT:
	case UNION:
		if (is_incomplete(tp)) {
			/* cannot take size/alignment of incomplete type */
			error(143);
			return 0;
		}
		break;
	case ENUM:
		break;
	default:
		if (tp->t_bitfield) {
			/* cannot take size/alignment of bit-field */
			error(145);
			return 0;
		}
		if (tp->t_tspec == VOID) {
			/* cannot take size/alignment of void */
			error(146);
			return 0;
		}
		break;
	}

	return build_integer_constant(SIZEOF_TSPEC,
	    (int64_t)alignment_in_bits(tp) / CHAR_SIZE);
}

/*
 * Type casts.
 */
tnode_t *
cast(tnode_t *tn, type_t *tp)
{
	tspec_t	nt, ot;

	if (tn == NULL)
		return NULL;

	/*
	 * XXX: checking for tp == NULL is only a quick fix for PR 22119.
	 *  The proper fix needs to be investigated properly.
	 *  See d_pr_22119.c for how to get here.
	 */
	if (tp == NULL)
		return NULL;

	tn = cconv(tn);

	nt = tp->t_tspec;
	ot = tn->tn_type->t_tspec;

	if (nt == VOID) {
		/*
		 * XXX ANSI C requires scalar types or void (Plauger & Brodie).
		 * But this seems really questionable.
		 */
	} else if (nt == UNION) {
		sym_t *m;
		struct_or_union *str = tp->t_str;
		if (!gflag) {
			/* union cast is a GCC extension */
			error(328);
			return NULL;
		}
		for (m = str->sou_first_member; m != NULL; m = m->s_next) {
			if (eqtype(m->s_type, tn->tn_type,
			    false, false, NULL)) {
				tn = expr_alloc_tnode();
				tn->tn_op = CVT;
				tn->tn_type = tp;
				tn->tn_cast = true;
				tn->tn_right = NULL;
				return tn;
			}
		}
		/* type '%s' is not a member of '%s' */
		error(329, type_name(tn->tn_type), type_name(tp));
		return NULL;
	} else if (nt == STRUCT || nt == ARRAY || nt == FUNC) {
		/* Casting to a struct is an undocumented GCC extension. */
		if (!(gflag && nt == STRUCT))
			goto invalid_cast;
	} else if (ot == STRUCT || ot == UNION) {
		goto invalid_cast;
	} else if (ot == VOID) {
		/* improper cast of void expression */
		error(148);
		return NULL;
	} else if (is_integer(nt) && is_scalar(ot)) {
		/* ok */
	} else if (is_floating(nt) && is_arithmetic(ot)) {
		/* ok */
	} else if (nt == PTR && is_integer(ot)) {
		/* ok */
	} else if (nt == PTR && ot == PTR) {
		if (!tp->t_subt->t_const && tn->tn_type->t_subt->t_const) {
			if (hflag)
				/* cast discards 'const' from type '%s' */
				warning(275, type_name(tn->tn_type));
		}
	} else
		goto invalid_cast;

	tn = convert(CVT, 0, tp, tn);
	tn->tn_cast = true;

	return tn;

invalid_cast:
	/* invalid cast from '%s' to '%s' */
	error(147, type_name(tn->tn_type), type_name(tp));
	return NULL;
}

/*
 * Create the node for a function argument.
 * All necessary conversions and type checks are done in
 * build_function_call because build_function_argument has no
 * information about expected argument types.
 */
tnode_t *
build_function_argument(tnode_t *args, tnode_t *arg)
{
	tnode_t	*ntn;

	/*
	 * If there was a serious error in the expression for the argument,
	 * create a dummy argument so the positions of the remaining arguments
	 * will not change.
	 */
	if (arg == NULL)
		arg = build_integer_constant(INT, 0);

	ntn = new_tnode(PUSH, arg->tn_sys, arg->tn_type, arg, args);

	return ntn;
}

/*
 * Create the node for a function call. Also check types of
 * function arguments and insert conversions, if necessary.
 */
tnode_t *
build_function_call(tnode_t *func, bool sys, tnode_t *args)
{
	tnode_t	*ntn;
	op_t	fcop;

	if (func == NULL)
		return NULL;

	if (func->tn_op == NAME && func->tn_type->t_tspec == FUNC) {
		fcop = CALL;
	} else {
		fcop = ICALL;
	}

	check_ctype_function_call(func, args);

	/*
	 * after cconv() func will always be a pointer to a function
	 * if it is a valid function designator.
	 */
	func = cconv(func);

	if (func->tn_type->t_tspec != PTR ||
	    func->tn_type->t_subt->t_tspec != FUNC) {
		/* illegal function (type %s) */
		error(149, type_name(func->tn_type));
		return NULL;
	}

	args = check_function_arguments(func->tn_type->t_subt, args);

	ntn = new_tnode(fcop, sys, func->tn_type->t_subt->t_subt, func, args);

	return ntn;
}

/*
 * Check types of all function arguments and insert conversions,
 * if necessary.
 */
static tnode_t *
check_function_arguments(type_t *ftp, tnode_t *args)
{
	tnode_t	*arg;
	sym_t	*asym;
	tspec_t	at;
	int	narg, npar, n, i;

	/* get # of args in the prototype */
	npar = 0;
	for (asym = ftp->t_args; asym != NULL; asym = asym->s_next)
		npar++;

	/* get # of args in function call */
	narg = 0;
	for (arg = args; arg != NULL; arg = arg->tn_right)
		narg++;

	asym = ftp->t_args;
	if (ftp->t_proto && npar != narg && !(ftp->t_vararg && npar < narg)) {
		/* argument mismatch: %d arg%s passed, %d expected */
		error(150, narg, narg > 1 ? "s" : "", npar);
		asym = NULL;
	}

	for (n = 1; n <= narg; n++) {

		/*
		 * The rightmost argument is at the top of the argument
		 * subtree.
		 */
		for (i = narg, arg = args; i > n; i--, arg = arg->tn_right)
			continue;

		/* some things which are always not allowed */
		if ((at = arg->tn_left->tn_type->t_tspec) == VOID) {
			/* void expressions may not be arguments, arg #%d */
			error(151, n);
			return NULL;
		} else if ((at == STRUCT || at == UNION) &&
			   is_incomplete(arg->tn_left->tn_type)) {
			/* argument cannot have unknown size, arg #%d */
			error(152, n);
			return NULL;
		} else if (is_integer(at) &&
			   arg->tn_left->tn_type->t_is_enum &&
			   is_incomplete(arg->tn_left->tn_type)) {
			/* argument cannot have unknown size, arg #%d */
			warning(152, n);
		}

		/* class conversions (arg in value context) */
		arg->tn_left = cconv(arg->tn_left);

		if (asym != NULL) {
			arg->tn_left = check_prototype_argument(
			    n, asym->s_type, arg->tn_left);
		} else {
			arg->tn_left = promote(NOOP, true, arg->tn_left);
		}
		arg->tn_type = arg->tn_left->tn_type;

		if (asym != NULL)
			asym = asym->s_next;
	}

	return args;
}

/*
 * Compare the type of an argument with the corresponding type of a
 * prototype parameter. If it is a valid combination, but both types
 * are not the same, insert a conversion to convert the argument into
 * the type of the parameter.
 */
static tnode_t *
check_prototype_argument(
	int	n,		/* pos of arg */
	type_t	*tp,		/* expected type (from prototype) */
	tnode_t	*tn)		/* argument */
{
	tnode_t	*ln;
	bool	dowarn;

	ln = xcalloc(1, sizeof(*ln));
	ln->tn_type = expr_unqualified_type(tp);
	ln->tn_lvalue = true;
	if (typeok(FARG, n, ln, tn)) {
		if (!eqtype(tp, tn->tn_type,
		    true, false, (dowarn = false, &dowarn)) || dowarn)
			tn = convert(FARG, n, tp, tn);
	}
	free(ln);
	return tn;
}

/*
 * Return the value of an integral constant expression.
 * If the expression is not constant or its type is not an integer
 * type, an error message is printed.
 */
val_t *
constant(tnode_t *tn, bool required)
{
	val_t	*v;

	if (tn != NULL)
		tn = cconv(tn);
	if (tn != NULL)
		tn = promote(NOOP, false, tn);

	v = xcalloc(1, sizeof(*v));

	if (tn == NULL) {
		lint_assert(nerr != 0);
		debug_step("constant node is null; returning 1 instead");
		v->v_tspec = INT;
		v->v_quad = 1;
		return v;
	}

	v->v_tspec = tn->tn_type->t_tspec;

	if (tn->tn_op == CON) {
		lint_assert(tn->tn_type->t_tspec == tn->tn_val->v_tspec);
		if (is_integer(tn->tn_val->v_tspec)) {
			v->v_unsigned_since_c90 =
			    tn->tn_val->v_unsigned_since_c90;
			v->v_quad = tn->tn_val->v_quad;
			return v;
		}
		v->v_quad = tn->tn_val->v_ldbl;
	} else {
		v->v_quad = 1;
	}

	if (required)
		/* integral constant expression expected */
		error(55);
	else
		/* variable array dimension is a C99/GCC extension */
		c99ism(318);

	if (!is_integer(v->v_tspec))
		v->v_tspec = INT;

	return v;
}

static bool
is_constcond_false(const tnode_t *tn, tspec_t t)
{
	return (t == BOOL || t == INT) &&
	       tn->tn_op == CON && tn->tn_val->v_quad == 0;
}

/*
 * Perform some tests on expressions which can't be done in build_binary()
 * and functions called by build_binary(). These tests must be done here
 * because we need some information about the context in which the operations
 * are performed.
 * After all tests are performed and dofreeblk is true, expr() frees the
 * memory which is used for the expression.
 */
void
expr(tnode_t *tn, bool vctx, bool tctx, bool dofreeblk, bool is_do_while)
{

	if (tn == NULL) {	/* in case of errors */
		expr_free_all();
		return;
	}

	/* expr() is also called in global initializations */
	if (dcs->d_ctx != EXTERN && !is_do_while)
		check_statement_reachable();

	check_expr_misc(tn, vctx, tctx, !tctx, false, false, false);
	if (tn->tn_op == ASSIGN) {
		if (hflag && tctx)
			/* assignment in conditional context */
			warning(159);
	} else if (tn->tn_op == CON) {
		if (hflag && tctx && !constcond_flag &&
		    !tn->tn_system_dependent &&
		    !(is_do_while &&
		      is_constcond_false(tn, tn->tn_type->t_tspec)))
			/* constant in conditional context */
			warning(161);
	}
	if (!modtab[tn->tn_op].m_has_side_effect) {
		/*
		 * for left operands of COMMA this warning is already
		 * printed
		 */
		if (tn->tn_op != COMMA && !vctx && !tctx)
			check_null_effect(tn);
	}
	debug_node(tn);

	/* free the tree memory */
	if (dofreeblk)
		expr_free_all();
}

static bool
has_side_effect(const tnode_t *tn) /* NOLINT(misc-no-recursion) */
{
	op_t op = tn->tn_op;

	if (modtab[op].m_has_side_effect)
		return true;

	if (op == CVT && tn->tn_type->t_tspec == VOID)
		return has_side_effect(tn->tn_left);

	/* XXX: Why not has_side_effect(tn->tn_left) as well? */
	if (op == LOGAND || op == LOGOR)
		return has_side_effect(tn->tn_right);

	/* XXX: Why not has_side_effect(tn->tn_left) as well? */
	if (op == QUEST)
		return has_side_effect(tn->tn_right);

	if (op == COLON || op == COMMA) {
		return has_side_effect(tn->tn_left) ||
		       has_side_effect(tn->tn_right);
	}

	return false;
}

static bool
is_void_cast(const tnode_t *tn)
{

	return tn->tn_op == CVT && tn->tn_cast &&
	       tn->tn_type->t_tspec == VOID;
}

static bool
is_local_symbol(const tnode_t *tn)
{

	return tn->tn_op == LOAD &&
	       tn->tn_left->tn_op == NAME &&
	       tn->tn_left->tn_sym->s_scl == AUTO;
}

static bool
is_int_constant_zero(const tnode_t *tn)
{

	return tn->tn_op == CON &&
	       tn->tn_type->t_tspec == INT &&
	       tn->tn_val->v_quad == 0;
}

static void
check_null_effect(const tnode_t *tn)
{

	if (!hflag)
		return;
	if (has_side_effect(tn))
		return;
	if (is_void_cast(tn) && is_local_symbol(tn->tn_left))
		return;
	if (is_void_cast(tn) && is_int_constant_zero(tn->tn_left))
		return;

	/* expression has null effect */
	warning(129);
}

static void
check_expr_addr(const tnode_t *ln, bool szof, bool fcall)
{
	/* XXX: Taking warn_about_unreachable into account here feels wrong. */
	if (ln->tn_op == NAME && (reached || !warn_about_unreachable)) {
		if (!szof)
			mark_as_set(ln->tn_sym);
		mark_as_used(ln->tn_sym, fcall, szof);
	}
	if (ln->tn_op == INDIR && ln->tn_left->tn_op == PLUS)
		/* check the range of array indices */
		check_array_index(ln->tn_left, true);
}

static void
check_expr_load(const tnode_t *ln)
{
	if (ln->tn_op == INDIR && ln->tn_left->tn_op == PLUS)
		/* check the range of array indices */
		check_array_index(ln->tn_left, false);
}

static void
check_expr_side_effect(const tnode_t *ln, bool szof)
{
	scl_t sc;
	dinfo_t *di;

	/* XXX: Taking warn_about_unreachable into account here feels wrong. */
	if (ln->tn_op == NAME && (reached || !warn_about_unreachable)) {
		sc = ln->tn_sym->s_scl;
		/*
		 * Look if there was a asm statement in one of the
		 * compound statements we are in. If not, we don't
		 * print a warning.
		 */
		for (di = dcs; di != NULL; di = di->d_next) {
			if (di->d_asm)
				break;
		}
		if (sc != EXTERN && sc != STATIC &&
		    !ln->tn_sym->s_set && !szof && di == NULL) {
			/* %s may be used before set */
			warning(158, ln->tn_sym->s_name);
			mark_as_set(ln->tn_sym);
		}
		mark_as_used(ln->tn_sym, false, false);
	}
}

static void
check_expr_assign(const tnode_t *ln, bool szof)
{
	/* XXX: Taking warn_about_unreachable into account here feels wrong. */
	if (ln->tn_op == NAME && !szof && (reached || !warn_about_unreachable)) {
		mark_as_set(ln->tn_sym);
		if (ln->tn_sym->s_scl == EXTERN)
			outusg(ln->tn_sym);
	}
	if (ln->tn_op == INDIR && ln->tn_left->tn_op == PLUS)
		/* check the range of array indices */
		check_array_index(ln->tn_left, false);
}

static void
check_expr_call(const tnode_t *tn, const tnode_t *ln,
		bool szof, bool vctx, bool tctx, bool retval_discarded)
{
	lint_assert(ln->tn_op == ADDR);
	lint_assert(ln->tn_left->tn_op == NAME);
	if (!szof &&
	    !is_compiler_builtin(ln->tn_left->tn_sym->s_name))
		outcall(tn, vctx || tctx, retval_discarded);
}

static bool
check_expr_op(const tnode_t *tn, op_t op, const tnode_t *ln,
	      bool szof, bool fcall, bool vctx, bool tctx,
	      bool retval_discarded, bool eqwarn)
{
	switch (op) {
	case ADDR:
		check_expr_addr(ln, szof, fcall);
		break;
	case LOAD:
		check_expr_load(ln);
		/* FALLTHROUGH */
	case PUSH:
	case INCBEF:
	case DECBEF:
	case INCAFT:
	case DECAFT:
	case ADDASS:
	case SUBASS:
	case MULASS:
	case DIVASS:
	case MODASS:
	case ANDASS:
	case ORASS:
	case XORASS:
	case SHLASS:
	case SHRASS:
	case REAL:
	case IMAG:
		check_expr_side_effect(ln, szof);
		break;
	case ASSIGN:
		check_expr_assign(ln, szof);
		break;
	case CALL:
		check_expr_call(tn, ln, szof, vctx, tctx, retval_discarded);
		break;
	case EQ:
		if (hflag && eqwarn)
			/* operator '==' found where '=' was expected */
			warning(160);
		break;
	case CON:
	case NAME:
	case STRING:
		return false;
		/* LINTED206: (enumeration values not handled in switch) */
	case BITOR:
	case BITXOR:
	case NE:
	case GE:
	case GT:
	case LE:
	case LT:
	case SHR:
	case SHL:
	case MINUS:
	case PLUS:
	case MOD:
	case DIV:
	case MULT:
	case INDIR:
	case UMINUS:
	case UPLUS:
	case DEC:
	case INC:
	case COMPL:
	case NOT:
	case POINT:
	case ARROW:
	case NOOP:
	case BITAND:
	case FARG:
	case CASE:
	case INIT:
	case RETURN:
	case ICALL:
	case CVT:
	case COMMA:
	case FSEL:
	case COLON:
	case QUEST:
	case LOGOR:
	case LOGAND:
		break;
	}
	return true;
}

/* ARGSUSED */
void
check_expr_misc(const tnode_t *tn, bool vctx, bool tctx,
		bool eqwarn, bool fcall, bool retval_discarded, bool szof)
{
	tnode_t *ln, *rn;
	const mod_t *mp;
	op_t op;
	bool cvctx, ctctx, eq, discard;

	if (tn == NULL)
		return;

	ln = tn->tn_left;
	rn = tn->tn_right;
	mp = &modtab[op = tn->tn_op];

	if (!check_expr_op(tn, op, ln,
	    szof, fcall, vctx, tctx, retval_discarded, eqwarn))
		return;

	cvctx = mp->m_left_value_context;
	ctctx = mp->m_left_test_context;
	eq = mp->m_warn_if_operand_eq &&
	     !ln->tn_parenthesized &&
	     rn != NULL && !rn->tn_parenthesized;

	/*
	 * values of operands of ':' are not used if the type of at least
	 * one of the operands (for gcc compatibility) is void
	 * XXX test/value context of QUEST should probably be used as
	 * context for both operands of COLON
	 */
	if (op == COLON && tn->tn_type->t_tspec == VOID)
		cvctx = ctctx = false;
	discard = op == CVT && tn->tn_type->t_tspec == VOID;
	check_expr_misc(ln, cvctx, ctctx, eq, op == CALL, discard, szof);

	switch (op) {
	case PUSH:
		if (rn != NULL)
			check_expr_misc(rn, false, false, eq, false, false,
			    szof);
		break;
	case LOGAND:
	case LOGOR:
		check_expr_misc(rn, false, true, eq, false, false, szof);
		break;
	case COLON:
		check_expr_misc(rn, cvctx, ctctx, eq, false, false, szof);
		break;
	case COMMA:
		check_expr_misc(rn, vctx, tctx, eq, false, false, szof);
		break;
	default:
		if (mp->m_binary)
			check_expr_misc(rn, true, false, eq, false, false,
			    szof);
		break;
	}
}

/*
 * Checks the range of array indices, if possible.
 * amper is set if only the address of the element is used. This
 * means that the index is allowed to refer to the first element
 * after the array.
 */
static void
check_array_index(tnode_t *tn, bool amper)
{
	int	dim;
	tnode_t	*ln, *rn;
	int	elsz;
	int64_t	con;

	ln = tn->tn_left;
	rn = tn->tn_right;

	/* We can only check constant indices. */
	if (rn->tn_op != CON)
		return;

	/* Return if the left node does not stem from an array. */
	if (ln->tn_op != ADDR)
		return;
	if (ln->tn_left->tn_op != STRING && ln->tn_left->tn_op != NAME)
		return;
	if (ln->tn_left->tn_type->t_tspec != ARRAY)
		return;

	/*
	 * For incomplete array types, we can print a warning only if
	 * the index is negative.
	 */
	if (is_incomplete(ln->tn_left->tn_type) && rn->tn_val->v_quad >= 0)
		return;

	/* Get the size of one array element */
	if ((elsz = length(ln->tn_type->t_subt, NULL)) == 0)
		return;
	elsz /= CHAR_SIZE;

	/* Change the unit of the index from bytes to element size. */
	if (is_uinteger(rn->tn_type->t_tspec)) {
		con = (uint64_t)rn->tn_val->v_quad / elsz;
	} else {
		con = rn->tn_val->v_quad / elsz;
	}

	dim = ln->tn_left->tn_type->t_dim + (amper ? 1 : 0);

	if (!is_uinteger(rn->tn_type->t_tspec) && con < 0) {
		/* array subscript cannot be negative: %ld */
		warning(167, (long)con);
	} else if (dim > 0 && (uint64_t)con >= (uint64_t)dim) {
		/* array subscript cannot be > %d: %ld */
		warning(168, dim - 1, (long)con);
	}
}

static bool
is_out_of_char_range(const tnode_t *tn)
{
	return tn->tn_op == CON &&
	       !(0 <= tn->tn_val->v_quad &&
		 tn->tn_val->v_quad < 1 << (CHAR_SIZE - 1));
}

/*
 * Check for ordered comparisons of unsigned values with 0.
 */
static void
check_integer_comparison(op_t op, tnode_t *ln, tnode_t *rn)
{
	tspec_t	lt, rt;

	lt = ln->tn_type->t_tspec;
	rt = rn->tn_type->t_tspec;

	if (ln->tn_op != CON && rn->tn_op != CON)
		return;

	if (!is_integer(lt) || !is_integer(rt))
		return;

	if (hflag || pflag) {
		if (lt == CHAR && is_out_of_char_range(rn)) {
			/* nonportable character comparison '%s %d' */
			warning(230, op_name(op), (int)rn->tn_val->v_quad);
			return;
		}
		if (rt == CHAR && is_out_of_char_range(ln)) {
			/* nonportable character comparison '%s %d' */
			warning(230, op_name(op), (int)ln->tn_val->v_quad);
			return;
		}
	}

	if (is_uinteger(lt) && !is_uinteger(rt) &&
	    rn->tn_op == CON && rn->tn_val->v_quad <= 0) {
		if (rn->tn_val->v_quad < 0) {
			/* comparison of %s with %s, op %s */
			warning(162, type_name(ln->tn_type),
			    "negative constant", op_name(op));
		} else if (op == LT || op == GE) {
			/* comparison of %s with %s, op %s */
			warning(162, type_name(ln->tn_type), "0", op_name(op));
		}
		return;
	}
	if (is_uinteger(rt) && !is_uinteger(lt) &&
	    ln->tn_op == CON && ln->tn_val->v_quad <= 0) {
		if (ln->tn_val->v_quad < 0) {
			/* comparison of %s with %s, op %s */
			warning(162, "negative constant",
			    type_name(rn->tn_type), op_name(op));
		} else if (op == GT || op == LE) {
			/* comparison of %s with %s, op %s */
			warning(162, "0", type_name(rn->tn_type), op_name(op));
		}
		return;
	}
}

/*
 * Return whether the expression can be used for static initialization.
 *
 * Constant initialization expressions must be constant or an address
 * of a static object with an optional offset. In the first case,
 * the result is returned in *offsp. In the second case, the static
 * object is returned in *symp and the offset in *offsp.
 *
 * The expression can consist of PLUS, MINUS, ADDR, NAME, STRING and
 * CON. Type conversions are allowed if they do not change binary
 * representation (including width).
 *
 * C99 6.6 "Constant expressions"
 * C99 6.7.8p4 restricts initializers for static storage duration
 */
bool
constant_addr(const tnode_t *tn, const sym_t **symp, ptrdiff_t *offsp)
{
	const sym_t *sym;
	ptrdiff_t offs1, offs2;
	tspec_t	t, ot;

	switch (tn->tn_op) {
	case MINUS:
		if (tn->tn_right->tn_op == CVT)
			return constant_addr(tn->tn_right, symp, offsp);
		else if (tn->tn_right->tn_op != CON)
			return false;
		/* FALLTHROUGH */
	case PLUS:
		offs1 = offs2 = 0;
		if (tn->tn_left->tn_op == CON) {
			offs1 = (ptrdiff_t)tn->tn_left->tn_val->v_quad;
			if (!constant_addr(tn->tn_right, &sym, &offs2))
				return false;
		} else if (tn->tn_right->tn_op == CON) {
			offs2 = (ptrdiff_t)tn->tn_right->tn_val->v_quad;
			if (tn->tn_op == MINUS)
				offs2 = -offs2;
			if (!constant_addr(tn->tn_left, &sym, &offs1))
				return false;
		} else {
			return false;
		}
		*symp = sym;
		*offsp = offs1 + offs2;
		return true;
	case ADDR:
		if (tn->tn_left->tn_op == NAME) {
			*symp = tn->tn_left->tn_sym;
			*offsp = 0;
			return true;
		} else {
			/*
			 * If this would be the front end of a compiler we
			 * would return a label instead of 0, at least if
			 * 'tn->tn_left->tn_op == STRING'.
			 */
			*symp = NULL;
			*offsp = 0;
			return true;
		}
	case CVT:
		t = tn->tn_type->t_tspec;
		ot = tn->tn_left->tn_type->t_tspec;
		if ((!is_integer(t) && t != PTR) ||
		    (!is_integer(ot) && ot != PTR)) {
			return false;
		}
#if 0
		/*
		 * consider:
		 *	struct foo {
		 *		unsigned char a;
		 *	} f = {
		 *		(unsigned char)(unsigned long)
		 *		    (&(((struct foo *)0)->a))
		 *	};
		 * since psize(unsigned long) != psize(unsigned char),
		 * this fails.
		 */
		else if (psize(t) != psize(ot))
			return -1;
#endif
		return constant_addr(tn->tn_left, symp, offsp);
	default:
		return false;
	}
}

/* Append s2 to s1, then free s2. */
strg_t *
cat_strings(strg_t *s1, strg_t *s2)
{
	size_t len1, len2, sz;

	if (s1->st_tspec != s2->st_tspec) {
		/* cannot concatenate wide and regular string literals */
		error(292);
		return s1;
	}

	len1 = s1->st_len;
	len2 = s2->st_len;

	if (s1->st_tspec == CHAR) {
		sz = sizeof(*s1->st_cp);
		s1->st_cp = xrealloc(s1->st_cp, (len1 + len2 + 1) * sz);
		memcpy(s1->st_cp + len1, s2->st_cp, (len2 + 1) * sz);
		free(s2->st_cp);
	} else {
		sz = sizeof(*s1->st_wcp);
		s1->st_wcp = xrealloc(s1->st_wcp, (len1 + len2 + 1) * sz);
		memcpy(s1->st_wcp + len1, s2->st_wcp, (len2 + 1) * sz);
		free(s2->st_wcp);
	}

	s1->st_len = len1 + len2;
	free(s2);

	return s1;
}

static bool
is_confusing_precedence(op_t op, op_t lop, bool lparen, op_t rop, bool rparen)
{

	if (op == SHL || op == SHR) {
		if (!lparen && (lop == PLUS || lop == MINUS))
			return true;
		if (!rparen && (rop == PLUS || rop == MINUS))
			return true;
		return false;
	}

	if (op == LOGOR) {
		if (!lparen && lop == LOGAND)
			return true;
		if (!rparen && rop == LOGAND)
			return true;
		return false;
	}

	lint_assert(op == BITAND || op == BITXOR || op == BITOR);
	if (!lparen && lop != op) {
		if (lop == PLUS || lop == MINUS)
			return true;
		if (lop == BITAND || lop == BITXOR)
			return true;
	}
	if (!rparen && rop != op) {
		if (rop == PLUS || rop == MINUS)
			return true;
		if (rop == BITAND || rop == BITXOR)
			return true;
	}
	return false;
}

/*
 * Print a warning if the given node has operands which should be
 * parenthesized.
 *
 * XXX Does not work if an operand is a constant expression. Constant
 * expressions are already folded.
 */
static void
check_precedence_confusion(tnode_t *tn)
{
	tnode_t *ln, *rn;

	if (!hflag)
		return;

	debug_node(tn);

	lint_assert(is_binary(tn));
	for (ln = tn->tn_left; ln->tn_op == CVT; ln = ln->tn_left)
		continue;
	for (rn = tn->tn_right; rn->tn_op == CVT; rn = rn->tn_left)
		continue;

	if (is_confusing_precedence(tn->tn_op,
	    ln->tn_op, ln->tn_parenthesized,
	    rn->tn_op, rn->tn_parenthesized)) {
		/* precedence confusion possible: parenthesize! */
		warning(169);
	}
}

typedef struct stmt_expr {
	struct memory_block *se_mem;
	sym_t *se_sym;
	struct stmt_expr *se_enclosing;
} stmt_expr;

static stmt_expr *stmt_exprs;

void
begin_statement_expr(void)
{
	stmt_expr *se = xmalloc(sizeof(*se));
	se->se_mem = expr_save_memory();
	se->se_sym = NULL;
	se->se_enclosing = stmt_exprs;
	stmt_exprs = se;
}

void
do_statement_expr(tnode_t *tn)
{
	block_level--;
	mem_block_level--;
	stmt_exprs->se_sym = mktempsym(dup_type(tn->tn_type));
	mem_block_level++;
	block_level++;
	/* ({ }) is a GCC extension */
	gnuism(320);

}

tnode_t *
end_statement_expr(void)
{
	stmt_expr *se = stmt_exprs;
	tnode_t *tn = build_name(se->se_sym, false);
	expr_save_memory();	/* leak */
	expr_restore_memory(se->se_mem);
	stmt_exprs = se->se_enclosing;
	free(se);
	return tn;
}
