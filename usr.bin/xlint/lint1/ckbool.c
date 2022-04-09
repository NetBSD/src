/* $NetBSD: ckbool.c,v 1.12 2022/04/09 15:43:41 rillig Exp $ */

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
__RCSID("$NetBSD: ckbool.c,v 1.12 2022/04/09 15:43:41 rillig Exp $");
#endif

#include <string.h>

#include "lint1.h"


/*
 * The option -T treats _Bool as incompatible with all other scalar types.
 * See d_c99_bool_strict.c for the exact rules and for examples.
 */


static const char *
op_name(op_t op)
{
	return modtab[op].m_name;
}

/*
 * See if in strict bool mode, the operator takes either two bool operands
 * or two arbitrary other operands.
 */
static bool
is_assignment_bool_or_other(op_t op)
{
	return op == ASSIGN ||
	       op == ANDASS || op == XORASS || op == ORASS ||
	       op == RETURN || op == INIT || op == FARG;
}

static bool
is_symmetric_bool_or_other(op_t op)
{
	return op == EQ || op == NE ||
	       op == BITAND || op == BITXOR || op == BITOR ||
	       op == COLON;
}

static bool
is_int_constant_zero(const tnode_t *tn, tspec_t t)
{
	return t == INT && tn->tn_op == CON && tn->tn_val->v_quad == 0;
}

static bool
is_typeok_strict_bool_binary(op_t op,
			     const tnode_t *ln, tspec_t lt,
			     const tnode_t *rn, tspec_t rt)
{
	if ((lt == BOOL) == (rt == BOOL))
		return true;

	if ((ln->tn_sys || rn->tn_sys) &&
	    (is_int_constant_zero(ln, lt) || is_int_constant_zero(rn, rt)))
		return true;

	if (is_assignment_bool_or_other(op))
		return lt != BOOL && (ln->tn_sys || rn->tn_sys);

	return !is_symmetric_bool_or_other(op);
}

/*
 * Some operators require that either both operands are bool or both are
 * scalar.
 *
 * Code that passes this check can be compiled in a pre-C99 environment that
 * doesn't implement the special rule C99 6.3.1.2, without silent change in
 * behavior.
 */
static bool
typeok_strict_bool_binary_compatible(op_t op, int arg,
				     const tnode_t *ln, tspec_t lt,
				     const tnode_t *rn, tspec_t rt)
{
	if (is_typeok_strict_bool_binary(op, ln, lt, rn, rt))
		return true;

	if (op == FARG) {
		/* argument #%d expects '%s', gets passed '%s' */
		error(334, arg, tspec_name(lt), tspec_name(rt));
	} else if (op == RETURN) {
		/* return value type mismatch (%s) and (%s) */
		error(211, tspec_name(lt), tspec_name(rt));
	} else {
		/* operands of '%s' have incompatible types (%s != %s) */
		error(107, op_name(op), tspec_name(lt), tspec_name(rt));
	}

	return false;
}

/*
 * In strict bool mode, check whether the types of the operands match the
 * operator.
 */
bool
typeok_scalar_strict_bool(op_t op, const mod_t *mp, int arg,
			  const tnode_t *ln,
			  const tnode_t *rn)

{
	tspec_t lt, rt;

	ln = before_conversion(ln);
	lt = ln->tn_type->t_tspec;

	if (rn != NULL) {
		rn = before_conversion(rn);
		rt = rn->tn_type->t_tspec;
	} else {
		rt = NOTSPEC;
	}

	if (rn != NULL &&
	    !typeok_strict_bool_binary_compatible(op, arg, ln, lt, rn, rt))
		return false;

	if (mp->m_requires_bool || op == QUEST) {
		bool binary = mp->m_binary;
		bool lbool = is_typeok_bool_operand(ln);
		bool ok = true;

		if (!binary && !lbool) {
			/* operand of '%s' must be bool, not '%s' */
			error(330, op_name(op), tspec_name(lt));
			ok = false;
		}
		if (binary && !lbool) {
			/* left operand of '%s' must be bool, not '%s' */
			error(331, op_name(op), tspec_name(lt));
			ok = false;
		}
		if (binary && op != QUEST && !is_typeok_bool_operand(rn)) {
			/* right operand of '%s' must be bool, not '%s' */
			error(332, op_name(op), tspec_name(rt));
			ok = false;
		}
		return ok;
	}

	if (!mp->m_takes_bool) {
		bool binary = mp->m_binary;
		bool lbool = lt == BOOL;
		bool ok = true;

		if (!binary && lbool) {
			/* operand of '%s' must not be bool */
			error(335, op_name(op));
			ok = false;
		}
		if (binary && lbool) {
			/* left operand of '%s' must not be bool */
			error(336, op_name(op));
			ok = false;
		}
		if (binary && rt == BOOL) {
			/* right operand of '%s' must not be bool */
			error(337, op_name(op));
			ok = false;
		}
		return ok;
	}

	return true;
}

/*
 * See if the node is valid as operand of an operator that compares its
 * argument with 0.
 */
bool
is_typeok_bool_operand(const tnode_t *tn)
{
	tspec_t t;

	lint_assert(Tflag);

	while (tn->tn_op == COMMA)
		tn = tn->tn_right;
	tn = before_conversion(tn);
	t = tn->tn_type->t_tspec;

	if (t == BOOL)
		return true;

	if (tn->tn_sys && is_scalar(t))
		return true;

	/* For enums that are used as bit sets, allow "flags & FLAG". */
	if (tn->tn_op == BITAND &&
	    tn->tn_left->tn_op == CVT &&
	    tn->tn_left->tn_type->t_is_enum &&
	    tn->tn_right->tn_type->t_is_enum)
		return true;

	return false;
}

bool
fallback_symbol_strict_bool(sym_t *sym)
{
	if (Tflag && strcmp(sym->s_name, "__lint_false") == 0) {
		sym->s_scl = BOOL_CONST;
		sym->s_type = gettyp(BOOL);
		sym->u.s_bool_constant = false;
		return true;
	}

	if (Tflag && strcmp(sym->s_name, "__lint_true") == 0) {
		sym->s_scl = BOOL_CONST;
		sym->s_type = gettyp(BOOL);
		sym->u.s_bool_constant = true;
		return true;
	}

	return false;
}
