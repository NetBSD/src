/* $NetBSD: ckbool.c,v 1.32 2024/05/12 12:32:39 rillig Exp $ */

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
__RCSID("$NetBSD: ckbool.c,v 1.32 2024/05/12 12:32:39 rillig Exp $");
#endif

#include <string.h>

#include "lint1.h"


/*
 * The option -T treats _Bool as incompatible with all other scalar types.
 * See d_c99_bool_strict.c for the detailed rules and for examples.
 */


static bool
is_int_constant_zero(const tnode_t *tn, tspec_t t)
{
	return t == INT && tn->tn_op == CON && tn->u.value.u.integer == 0;
}

static bool
is_typeok_strict_bool_binary(op_t op,
			     const tnode_t *ln, tspec_t lt,
			     const tnode_t *rn, tspec_t rt)
{
	if ((lt == BOOL) == (rt == BOOL))
		return true;

	if (op == FARG && rn->tn_sys)
		return false;

	if ((ln->tn_sys || rn->tn_sys) &&
	    (is_int_constant_zero(ln, lt) || is_int_constant_zero(rn, rt)))
		return true;

	if (op == ASSIGN || op == ANDASS || op == XORASS || op == ORASS ||
	    op == RETURN || op == INIT || op == FARG)
		return lt != BOOL && (ln->tn_sys || rn->tn_sys);

	if (op == EQ || op == NE ||
	    op == BITAND || op == BITXOR || op == BITOR ||
	    op == COLON)
		return false;

	return true;
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

	if (op == FARG)
		/* parameter %d expects '%s', gets passed '%s' */
		error(334, arg, tspec_name(lt), tspec_name(rt));
	else if (op == RETURN)
		/* function has return type '%s' but returns '%s' */
		error(211, tspec_name(lt), tspec_name(rt));
	else
		/* operands of '%s' have incompatible types '%s' and '%s' */
		error(107, op_name(op), tspec_name(lt), tspec_name(rt));

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
	ln = before_conversion(ln);
	tspec_t lt = ln->tn_type->t_tspec;
	tspec_t rt = NO_TSPEC;
	if (rn != NULL) {
		rn = before_conversion(rn);
		rt = rn->tn_type->t_tspec;
	}

	if (rn != NULL &&
	    !typeok_strict_bool_binary_compatible(op, arg, ln, lt, rn, rt))
		return false;

	if (mp->m_compares_with_zero) {
		bool binary = mp->m_binary;
		bool lbool = is_typeok_bool_compares_with_zero(ln, false);
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
		if (binary && op != QUEST &&
		    !is_typeok_bool_compares_with_zero(rn, false)) {
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

bool
is_typeok_bool_compares_with_zero(const tnode_t *tn, bool is_do_while)
{
	while (tn->tn_op == COMMA)
		tn = tn->u.ops.right;
	tn = before_conversion(tn);

	return tn->tn_type->t_tspec == BOOL
	    || tn->tn_op == BITAND
	    || (is_do_while && is_int_constant_zero(tn, tn->tn_type->t_tspec))
	    || (tn->tn_sys && is_scalar(tn->tn_type->t_tspec));
}

bool
fallback_symbol_strict_bool(sym_t *sym)
{
	if (strcmp(sym->s_name, "__lint_false") == 0) {
		sym->s_scl = BOOL_CONST;
		sym->s_type = gettyp(BOOL);
		sym->u.s_bool_constant = false;
		return true;
	}

	if (strcmp(sym->s_name, "__lint_true") == 0) {
		sym->s_scl = BOOL_CONST;
		sym->s_type = gettyp(BOOL);
		sym->u.s_bool_constant = true;
		return true;
	}

	return false;
}
