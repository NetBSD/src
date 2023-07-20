/*	$NetBSD: tree.c,v 1.573 2023/07/15 15:51:22 rillig Exp $	*/

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
__RCSID("$NetBSD: tree.c,v 1.573 2023/07/15 15:51:22 rillig Exp $");
#endif

#include <float.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"


typedef struct integer_constraints {
	int64_t		smin;	/* signed minimum */
	int64_t		smax;	/* signed maximum */
	uint64_t	umin;	/* unsigned minimum */
	uint64_t	umax;	/* unsigned maximum */
	uint64_t	bset;	/* bits that are definitely set */
	uint64_t	bclr;	/* bits that are definitely clear */
} integer_constraints;


static uint64_t
u64_fill_right(uint64_t x)
{
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	return x;
}

static bool
str_ends_with(const char *haystack, const char *needle)
{
	size_t hlen = strlen(haystack);
	size_t nlen = strlen(needle);

	return nlen <= hlen &&
	       memcmp(haystack + hlen - nlen, needle, nlen) == 0;
}

static unsigned
width_in_bits(const type_t *tp)
{

	lint_assert(is_integer(tp->t_tspec));
	return tp->t_bitfield
	    ? tp->t_bit_field_width
	    : size_in_bits(tp->t_tspec);
}

static int
portable_rank_cmp(tspec_t t1, tspec_t t2) {
	const ttab_t *p1 = type_properties(t1), *p2 = type_properties(t2);
	lint_assert(p1->tt_rank_kind == p2->tt_rank_kind);
	lint_assert(p1->tt_rank_value > 0);
	lint_assert(p2->tt_rank_value > 0);
	return (int)p1->tt_rank_value - (int)p2->tt_rank_value;
}

static bool
ic_maybe_signed(const type_t *tp, const integer_constraints *ic)
{
	return !is_uinteger(tp->t_tspec) && ic->bclr >> 63 == 0;
}

static integer_constraints
ic_any(const type_t *tp)
{
	integer_constraints c;

	uint64_t vbits = value_bits(width_in_bits(tp));
	if (is_uinteger(tp->t_tspec)) {
		c.smin = INT64_MIN;
		c.smax = INT64_MAX;
		c.umin = 0;
		c.umax = vbits;
		c.bset = 0;
		c.bclr = ~c.umax;
	} else {
		c.smin = (int64_t)-1 - (int64_t)(vbits >> 1);
		c.smax = (int64_t)(vbits >> 1);
		c.umin = 0;
		c.umax = UINT64_MAX;
		c.bset = 0;
		c.bclr = 0;
	}
	return c;
}

static integer_constraints
ic_con(const type_t *tp, const val_t *v)
{
	integer_constraints c;

	lint_assert(is_integer(tp->t_tspec));
	int64_t si = v->u.integer;
	uint64_t ui = (uint64_t)si;
	c.smin = si;
	c.smax = si;
	c.umin = ui;
	c.umax = ui;
	c.bset = ui;
	c.bclr = ~ui;
	return c;
}

static integer_constraints
ic_cvt(const type_t *ntp, const type_t *otp, integer_constraints a)
{
	unsigned nw = width_in_bits(ntp);
	unsigned ow = width_in_bits(otp);
	bool nu = is_uinteger(ntp->t_tspec);
	bool ou = is_uinteger(otp->t_tspec);

	if (nw >= ow && nu == ou)
		return a;
	if (nw > ow && ou)
		return a;
	return ic_any(ntp);
}

static integer_constraints
ic_bitand(integer_constraints a, integer_constraints b)
{
	integer_constraints c;

	c.smin = INT64_MIN;
	c.smax = INT64_MAX;
	c.umin = 0;
	c.umax = UINT64_MAX;
	c.bset = a.bset & b.bset;
	c.bclr = a.bclr | b.bclr;
	return c;
}

static integer_constraints
ic_bitor(integer_constraints a, integer_constraints b)
{
	integer_constraints c;

	c.smin = INT64_MIN;
	c.smax = INT64_MAX;
	c.umin = 0;
	c.umax = UINT64_MAX;
	c.bset = a.bset | b.bset;
	c.bclr = a.bclr & b.bclr;
	return c;
}

static integer_constraints
ic_mod(const type_t *tp, integer_constraints a, integer_constraints b)
{
	integer_constraints c;

	if (ic_maybe_signed(tp, &a) || ic_maybe_signed(tp, &b))
		return ic_any(tp);

	c.smin = INT64_MIN;
	c.smax = INT64_MAX;
	c.umin = 0;
	c.umax = b.umax - 1;
	c.bset = 0;
	c.bclr = ~u64_fill_right(c.umax);
	return c;
}

static integer_constraints
ic_shl(const type_t *tp, integer_constraints a, integer_constraints b)
{
	integer_constraints c;
	unsigned int amount;

	if (ic_maybe_signed(tp, &a))
		return ic_any(tp);

	if (b.smin == b.smax && b.smin >= 0 && b.smin < 64)
		amount = (unsigned int)b.smin;
	else if (b.umin == b.umax && b.umin < 64)
		amount = (unsigned int)b.umin;
	else
		return ic_any(tp);

	c.smin = INT64_MIN;
	c.smax = INT64_MAX;
	c.umin = 0;
	c.umax = UINT64_MAX;
	c.bset = a.bset << amount;
	c.bclr = a.bclr << amount | (((uint64_t)1 << amount) - 1);
	return c;
}

static integer_constraints
ic_shr(const type_t *tp, integer_constraints a, integer_constraints b)
{
	integer_constraints c;
	unsigned int amount;

	if (ic_maybe_signed(tp, &a))
		return ic_any(tp);

	if (b.smin == b.smax && b.smin >= 0 && b.smin < 64)
		amount = (unsigned int)b.smin;
	else if (b.umin == b.umax && b.umin < 64)
		amount = (unsigned int)b.umin;
	else
		return ic_any(tp);

	c.smin = INT64_MIN;
	c.smax = INT64_MAX;
	c.umin = 0;
	c.umax = UINT64_MAX;
	c.bset = a.bset >> amount;
	c.bclr = a.bclr >> amount | ~(~(uint64_t)0 >> amount);
	return c;
}

static integer_constraints
ic_cond(integer_constraints a, integer_constraints b)
{
	integer_constraints c;

	c.smin = a.smin < b.smin ? a.smin : b.smin;
	c.smax = a.smax > b.smax ? a.smax : b.smax;
	c.umin = a.umin < b.umin ? a.umin : b.umin;
	c.umax = a.umax > b.umax ? a.umax : b.umax;
	c.bset = a.bset | b.bset;
	c.bclr = a.bclr & b.bclr;
	return c;
}

static integer_constraints
ic_expr(const tnode_t *tn)
{
	integer_constraints lc, rc;

	lint_assert(is_integer(tn->tn_type->t_tspec));

	switch (tn->tn_op) {
	case CON:
		return ic_con(tn->tn_type, &tn->tn_val);
	case CVT:
		if (!is_integer(tn->tn_left->tn_type->t_tspec))
			return ic_any(tn->tn_type);
		lc = ic_expr(tn->tn_left);
		return ic_cvt(tn->tn_type, tn->tn_left->tn_type, lc);
	case MOD:
		lc = ic_expr(before_conversion(tn->tn_left));
		rc = ic_expr(before_conversion(tn->tn_right));
		return ic_mod(tn->tn_type, lc, rc);
	case SHL:
		lc = ic_expr(tn->tn_left);
		rc = ic_expr(tn->tn_right);
		return ic_shl(tn->tn_type, lc, rc);
	case SHR:
		lc = ic_expr(tn->tn_left);
		rc = ic_expr(tn->tn_right);
		return ic_shr(tn->tn_type, lc, rc);
	case BITAND:
		lc = ic_expr(tn->tn_left);
		rc = ic_expr(tn->tn_right);
		return ic_bitand(lc, rc);
	case BITOR:
		lc = ic_expr(tn->tn_left);
		rc = ic_expr(tn->tn_right);
		return ic_bitor(lc, rc);
	case QUEST:
		lc = ic_expr(tn->tn_right->tn_left);
		rc = ic_expr(tn->tn_right->tn_right);
		return ic_cond(lc, rc);
	default:
		return ic_any(tn->tn_type);
	}
}

/* Build 'pointer to tp', 'array of tp' or 'function returning tp'. */
type_t *
block_derive_type(type_t *tp, tspec_t t)
{
	type_t *tp2;

	tp2 = block_zero_alloc(sizeof(*tp2), "type");
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
	type_t *tp2;

	tp2 = expr_zero_alloc(sizeof(*tp2), "type");
	tp2->t_tspec = t;
	tp2->t_subt = tp;
	return tp2;
}

/* Create an expression from a unary or binary operator and its operands. */
static tnode_t *
build_op(op_t op, bool sys, type_t *type, tnode_t *ln, tnode_t *rn)
{

	tnode_t *ntn = expr_alloc_tnode();
	ntn->tn_op = op;
	ntn->tn_type = type;
	ntn->tn_sys = sys;
	ntn->tn_left = ln;
	ntn->tn_right = rn;

	if (op == INDIR || op == FSEL) {
		lint_assert(ln->tn_type->t_tspec == PTR);
		tspec_t t = ln->tn_type->t_subt->t_tspec;
		ntn->tn_lvalue = t != FUNC && t != VOID;
	}

	return ntn;
}

tnode_t *
build_constant(type_t *tp, val_t *v)
{

	tnode_t *n = expr_alloc_tnode();
	n->tn_op = CON;
	n->tn_type = tp;
	n->tn_val = *v;
	n->tn_val.v_tspec = tp->t_tspec;
	free(v);
	return n;
}

static tnode_t *
build_integer_constant(tspec_t t, int64_t si)
{

	tnode_t *n = expr_alloc_tnode();
	n->tn_op = CON;
	n->tn_type = gettyp(t);
	n->tn_val.v_tspec = t;
	n->tn_val.v_unsigned_since_c90 = false;
	n->tn_val.v_char_constant = false;
	n->tn_val.u.integer = si;
	return n;
}

static void
fallback_symbol(sym_t *sym)
{

	if (Tflag && fallback_symbol_strict_bool(sym))
		return;

	if (block_level > 0 && (strcmp(sym->s_name, "__FUNCTION__") == 0 ||
			   strcmp(sym->s_name, "__PRETTY_FUNCTION__") == 0)) {
		/* __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension */
		gnuism(316);
		// XXX: Should probably be ARRAY instead of PTR.
		sym->s_type = block_derive_type(gettyp(CHAR), PTR);
		sym->s_type->t_const = true;
		return;
	}

	if (block_level > 0 && strcmp(sym->s_name, "__func__") == 0) {
		if (!allow_c99)
			/* __func__ is a C99 feature */
			warning(317);
		/* C11 6.4.2.2 */
		sym->s_type = block_derive_type(gettyp(CHAR), ARRAY);
		sym->s_type->t_const = true;
		sym->s_type->t_dim = (int)strlen(funcsym->s_name) + 1;
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
	if (allow_gcc) {
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

/* https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html */
static bool
is_gcc_bool_builtin(const char *name)
{
	return strncmp(name, "__builtin_", 10) == 0 &&
	       (str_ends_with(name, "_overflow") ||
		str_ends_with(name, "_overflow_p"));
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
		if (allow_gcc && is_gcc_bool_builtin(sym->s_name))
			sym->s_type = gettyp(BOOL);
	} else if (allow_c99) {
		/* function '%s' implicitly declared to return int */
		error(215, sym->s_name);
	} else if (!allow_trad) {
		/* function '%s' implicitly declared to return int */
		warning(215, sym->s_name);
	}

	/* XXX if !allow_c90, the symbol should be exported to level 0 */
	sym->s_type = block_derive_type(sym->s_type, FUNC);
}

/* Create a node for a name (symbol table entry). */
tnode_t *
build_name(sym_t *sym, bool is_funcname)
{

	if (sym->s_scl == NOSCL && !in_gcc_attribute) {
		sym->s_scl = EXTERN;
		sym->s_def = DECL;
		if (is_funcname)
			build_name_call(sym);
		else
			fallback_symbol(sym);
	}

	lint_assert(sym->s_kind == FVFT || sym->s_kind == FMEMBER);

	tnode_t *n = expr_alloc_tnode();
	n->tn_type = sym->s_type;
	if (sym->s_scl == BOOL_CONST) {
		n->tn_op = CON;
		n->tn_val.v_tspec = BOOL;
		n->tn_val.v_unsigned_since_c90 = false;
		n->tn_val.v_char_constant = false;
		n->tn_val.u.integer = sym->u.s_bool_constant ? 1 : 0;
	} else if (sym->s_scl == ENUM_CONST) {
		n->tn_op = CON;
		n->tn_val.v_tspec = INT;	/* ENUM is in n->tn_type */
		n->tn_val.v_unsigned_since_c90 = false;
		n->tn_val.v_char_constant = false;
		n->tn_val.u.integer = sym->u.s_enum_constant;
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
	size_t len = strg->st_len;

	type_t *tp = expr_zero_alloc(sizeof(*tp), "type");
	tp->t_tspec = ARRAY;
	tp->t_subt = gettyp(strg->st_char ? CHAR : WCHAR_TSPEC);
	tp->t_dim = (int)(len + 1);

	tnode_t *n = expr_alloc_tnode();
	n->tn_op = STRING;
	n->tn_type = tp;
	n->tn_lvalue = true;

	n->tn_string = expr_zero_alloc(sizeof(*n->tn_string), "type.string");
	n->tn_string->st_char = strg->st_char;
	n->tn_string->st_len = len;

	size_t chsize = strg->st_char ? sizeof(char) : sizeof(wchar_t);
	size_t size = (len + 1) * chsize;
	n->tn_string->st_mem = expr_zero_alloc(size, "type.string.data");
	(void)memcpy(n->tn_string->st_mem, strg->st_mem, size);
	free(strg->st_mem);
	free(strg);

	return n;
}

tnode_t *
build_generic_selection(const tnode_t *expr,
			struct generic_association *sel)
{
	tnode_t *default_result = NULL;

	for (; sel != NULL; sel = sel->ga_prev) {
		if (expr != NULL &&
		    types_compatible(sel->ga_arg, expr->tn_type,
			false, false, NULL))
			return sel->ga_result;
		else if (sel->ga_arg == NULL)
			default_result = sel->ga_result;
	}
	return default_result;
}

static bool
is_out_of_char_range(const tnode_t *tn)
{
	return tn->tn_op == CON &&
	       !tn->tn_val.v_char_constant &&
	       !(0 <= tn->tn_val.u.integer &&
		 tn->tn_val.u.integer < 1 << (CHAR_SIZE - 1));
}

static void
check_integer_comparison(op_t op, tnode_t *ln, tnode_t *rn)
{

	tspec_t lt = ln->tn_type->t_tspec;
	tspec_t rt = rn->tn_type->t_tspec;

	if (ln->tn_op != CON && rn->tn_op != CON)
		return;

	if (!is_integer(lt) || !is_integer(rt))
		return;

	if (any_query_enabled && !in_system_header) {
		if (lt == CHAR && rn->tn_op == CON &&
		    !rn->tn_val.v_char_constant) {
			/* comparison '%s' of 'char' with plain integer %d */
			query_message(14,
			    op_name(op), (int)rn->tn_val.u.integer);
		}
		if (rt == CHAR && ln->tn_op == CON &&
		    !ln->tn_val.v_char_constant) {
			/* comparison '%s' of 'char' with plain integer %d */
			query_message(14,
			    op_name(op), (int)ln->tn_val.u.integer);
		}
	}

	if (hflag || pflag) {
		if (lt == CHAR && is_out_of_char_range(rn)) {
			char buf[128];
			(void)snprintf(buf, sizeof(buf), "%s %d",
			    op_name(op), (int)rn->tn_val.u.integer);
			/* nonportable character comparison '%s' */
			warning(230, buf);
			return;
		}
		if (rt == CHAR && is_out_of_char_range(ln)) {
			char buf[128];
			(void)snprintf(buf, sizeof(buf), "%d %s ?",
			    (int)ln->tn_val.u.integer, op_name(op));
			/* nonportable character comparison '%s' */
			warning(230, buf);
			return;
		}
	}

	if (is_uinteger(lt) && !is_uinteger(rt) &&
	    rn->tn_op == CON && rn->tn_val.u.integer <= 0) {
		if (rn->tn_val.u.integer < 0) {
			/* operator '%s' compares '%s' with '%s' */
			warning(162, op_name(op),
			    type_name(ln->tn_type), "negative constant");
		} else if (op == LT || op == GE) {
			/* operator '%s' compares '%s' with '%s' */
			warning(162, op_name(op), type_name(ln->tn_type), "0");
		}
		return;
	}
	if (is_uinteger(rt) && !is_uinteger(lt) &&
	    ln->tn_op == CON && ln->tn_val.u.integer <= 0) {
		if (ln->tn_val.u.integer < 0) {
			/* operator '%s' compares '%s' with '%s' */
			warning(162, op_name(op),
			    "negative constant", type_name(rn->tn_type));
		} else if (op == GT || op == LE) {
			/* operator '%s' compares '%s' with '%s' */
			warning(162, op_name(op), "0", type_name(rn->tn_type));
		}
		return;
	}
}

static const tspec_t arith_rank[] = {
	LDOUBLE, DOUBLE, FLOAT,
#ifdef INT128_SIZE
	UINT128, INT128,
#endif
	ULLONG, LLONG,
	ULONG, LONG,
	UINT, INT,
};

/* Keep unsigned in traditional C */
static tspec_t
usual_arithmetic_conversion_trad(tspec_t lt, tspec_t rt)
{

	size_t i;
	for (i = 0; arith_rank[i] != INT; i++)
		if (lt == arith_rank[i] || rt == arith_rank[i])
			break;

	tspec_t t = arith_rank[i];
	if (is_uinteger(lt) || is_uinteger(rt))
		if (is_integer(t) && !is_uinteger(t))
			return unsigned_type(t);
	return t;
}

static tspec_t
usual_arithmetic_conversion_c90(tspec_t lt, tspec_t rt)
{

	if (lt == rt)
		return lt;

	if (lt == LCOMPLEX || rt == LCOMPLEX)
		return LCOMPLEX;
	if (lt == DCOMPLEX || rt == DCOMPLEX)
		return DCOMPLEX;
	if (lt == FCOMPLEX || rt == FCOMPLEX)
		return FCOMPLEX;
	if (lt == LDOUBLE || rt == LDOUBLE)
		return LDOUBLE;
	if (lt == DOUBLE || rt == DOUBLE)
		return DOUBLE;
	if (lt == FLOAT || rt == FLOAT)
		return FLOAT;

	/*
	 * If type A has more bits than type B, it should be able to hold all
	 * possible values of type B.
	 */
	if (size_in_bits(lt) > size_in_bits(rt))
		return lt;
	if (size_in_bits(lt) < size_in_bits(rt))
		return rt;

	size_t i;
	for (i = 3; arith_rank[i] != INT; i++)
		if (arith_rank[i] == lt || arith_rank[i] == rt)
			break;
	if ((is_uinteger(lt) || is_uinteger(rt)) &&
	    !is_uinteger(arith_rank[i]))
		i--;
	return arith_rank[i];
}

static tnode_t *
apply_usual_arithmetic_conversions(op_t op, tnode_t *tn, tspec_t t)
{
	type_t *ntp = expr_dup_type(tn->tn_type);
	ntp->t_tspec = t;
	if (tn->tn_op != CON) {
		/* usual arithmetic conversion for '%s' from '%s' to '%s' */
		query_message(4, op_name(op),
		    type_name(tn->tn_type), type_name(ntp));
	}
	return convert(op, 0, ntp, tn);
}

/*
 * Apply the "usual arithmetic conversions" (C99 6.3.1.8), which gives both
 * operands the same type.
 */
static void
balance(op_t op, tnode_t **lnp, tnode_t **rnp)
{

	tspec_t lt = (*lnp)->tn_type->t_tspec;
	tspec_t rt = (*rnp)->tn_type->t_tspec;
	if (!is_arithmetic(lt) || !is_arithmetic(rt))
		return;

	tspec_t t = allow_c90
	    ? usual_arithmetic_conversion_c90(lt, rt)
	    : usual_arithmetic_conversion_trad(lt, rt);

	if (t != lt)
		*lnp = apply_usual_arithmetic_conversions(op, *lnp, t);
	if (t != rt)
		*rnp = apply_usual_arithmetic_conversions(op, *rnp, t);
}

/*
 * Create a tree node for the unary & operator
 */
static tnode_t *
build_address(bool sys, tnode_t *tn, bool noign)
{
	tspec_t t;

	if (!noign && ((t = tn->tn_type->t_tspec) == ARRAY || t == FUNC)) {
		if (!allow_c90)
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

	return build_op(ADDR, sys, expr_derive_type(tn->tn_type, PTR),
	    tn, NULL);
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

	val_t *v = xcalloc(1, sizeof(*v));
	v->v_tspec = tn->tn_type->t_tspec;

	tspec_t t = tn->tn_left->tn_type->t_tspec;
	bool utyp = !is_integer(t) || is_uinteger(t);
	int64_t sl = tn->tn_left->tn_val.u.integer, sr = 0;
	uint64_t ul = sl, ur = 0;
	if (is_binary(tn))
		ur = sr = tn->tn_right->tn_val.u.integer;

	int64_t mask = (int64_t)value_bits(size_in_bits(t));
	bool ovfl = false;

	int64_t si;
	switch (tn->tn_op) {
	case UPLUS:
		si = sl;
		break;
	case UMINUS:
		si = sl == INT64_MIN ? sl : -sl;
		if (sl != 0 && msb(si, t) == msb(sl, t))
			ovfl = true;
		break;
	case COMPL:
		si = ~sl;
		break;
	case MULT:
		if (utyp) {
			si = (int64_t)(ul * ur);
			if (si != (si & mask))
				ovfl = true;
			else if ((ul != 0) && ((si / ul) != ur))
				ovfl = true;
		} else {
			si = sl * sr;
			if (msb(si, t) != (msb(sl, t) ^ msb(sr, t)))
				ovfl = true;
		}
		break;
	case DIV:
		if (sr == 0) {
			/* division by 0 */
			error(139);
			si = utyp ? -1 : INT64_MAX;
		} else {
			si = utyp ? (int64_t)(ul / ur) : sl / sr;
		}
		break;
	case MOD:
		if (sr == 0) {
			/* modulus by 0 */
			error(140);
			si = 0;
		} else {
			si = utyp ? (int64_t)(ul % ur) : sl % sr;
		}
		break;
	case PLUS:
		si = utyp ? (int64_t)(ul + ur) : sl + sr;
		if (msb(sl, t) && msb(sr, t) && !msb(si, t))
			ovfl = true;
		if (!utyp && !msb(sl, t) && !msb(sr, t) && msb(si, t))
			ovfl = true;
		break;
	case MINUS:
		si = utyp ? (int64_t)(ul - ur) : sl - sr;
		if (!utyp && msb(sl, t) && !msb(sr, t) && !msb(si, t))
			ovfl = true;
		if (!msb(sl, t) && msb(sr, t) && msb(si, t))
			ovfl = true;
		break;
	case SHL:
		/* TODO: warn about out-of-bounds 'sr'. */
		/* TODO: warn about overflow in signed '<<'. */
		si = utyp ? (int64_t)(ul << (sr & 63)) : sl << (sr & 63);
		break;
	case SHR:
		/*
		 * The sign must be explicitly extended because
		 * shifts of signed values are implementation dependent.
		 */
		/* TODO: warn about out-of-bounds 'sr'. */
		si = (int64_t)(ul >> (sr & 63));
		si = convert_integer(si, t, size_in_bits(t) - (int)sr);
		break;
	case LT:
		si = (utyp ? ul < ur : sl < sr) ? 1 : 0;
		break;
	case LE:
		si = (utyp ? ul <= ur : sl <= sr) ? 1 : 0;
		break;
	case GE:
		si = (utyp ? ul >= ur : sl >= sr) ? 1 : 0;
		break;
	case GT:
		si = (utyp ? ul > ur : sl > sr) ? 1 : 0;
		break;
	case EQ:
		si = (utyp ? ul == ur : sl == sr) ? 1 : 0;
		break;
	case NE:
		si = (utyp ? ul != ur : sl != sr) ? 1 : 0;
		break;
	case BITAND:
		si = utyp ? (int64_t)(ul & ur) : sl & sr;
		break;
	case BITXOR:
		si = utyp ? (int64_t)(ul ^ ur) : sl ^ sr;
		break;
	case BITOR:
		si = utyp ? (int64_t)(ul | ur) : sl | sr;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}

	/* XXX: The overflow check does not work for 64-bit integers. */
	if (ovfl ||
	    ((uint64_t)(si | mask) != ~(uint64_t)0 && (si & ~mask) != 0)) {
		if (hflag)
			/* operator '%s' produces integer overflow */
			warning(141, op_name(tn->tn_op));
	}

	v->u.integer = convert_integer(si, t, 0);

	tnode_t *cn = build_constant(tn->tn_type, v);
	if (tn->tn_left->tn_system_dependent)
		cn->tn_system_dependent = true;
	if (is_binary(tn) && tn->tn_right->tn_system_dependent)
		cn->tn_system_dependent = true;

	return cn;
}

/*
 * Create a new node for one of the operators POINT and ARROW.
 */
static tnode_t *
build_struct_access(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{

	lint_assert(rn->tn_op == NAME);
	lint_assert(is_member(rn->tn_sym));

	bool lvalue = op == ARROW || ln->tn_lvalue;

	if (op == POINT) {
		ln = build_address(sys, ln, true);
	} else if (ln->tn_type->t_tspec != PTR) {
		lint_assert(!allow_c90);
		lint_assert(is_integer(ln->tn_type->t_tspec));
		ln = convert(NOOP, 0, expr_derive_type(gettyp(VOID), PTR), ln);
	}

	tnode_t *ctn = build_integer_constant(PTRDIFF_TSPEC,
	    rn->tn_sym->u.s_member.sm_offset_in_bits / CHAR_SIZE);

	type_t *ptr_tp = expr_derive_type(rn->tn_type, PTR);
	tnode_t *ntn = build_op(PLUS, sys, ptr_tp, ln, ctn);
	if (ln->tn_op == CON)
		ntn = fold(ntn);

	op_t nop = rn->tn_type->t_bitfield ? FSEL : INDIR;
	ntn = build_op(nop, sys, ntn->tn_type->t_subt, ntn, NULL);
	if (!lvalue)
		ntn->tn_lvalue = false;

	return ntn;
}

/*
 * Get the size in bytes of type tp->t_subt, as a constant expression of type
 * ptrdiff_t as seen from the target platform.
 */
static tnode_t *
subt_size_in_bytes(type_t *tp)
{

	lint_assert(tp->t_tspec == PTR);
	tp = tp->t_subt;

	int elem = 1;
	while (tp->t_tspec == ARRAY) {
		elem *= tp->t_dim;
		tp = tp->t_subt;
	}

	int elsz_in_bits = 0;
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
		if ((elsz_in_bits = (int)tp->t_sou->sou_size_in_bits) == 0)
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
 * Create a node for INCAFT, INCBEF, DECAFT and DECBEF.
 */
static tnode_t *
build_prepost_incdec(op_t op, bool sys, tnode_t *ln)
{

	lint_assert(ln != NULL);
	tnode_t *cn = ln->tn_type->t_tspec == PTR
	    ? subt_size_in_bytes(ln->tn_type)
	    : build_integer_constant(INT, 1);
	return build_op(op, sys, ln->tn_type, ln, cn);
}

static void
check_enum_array_index(const tnode_t *ln, const tnode_t *rn)
{

	if (ln->tn_op != ADDR || ln->tn_left->tn_op != NAME)
		return;

	const type_t *ltp = ln->tn_left->tn_type;
	if (ltp->t_tspec != ARRAY || ltp->t_incomplete_array)
		return;

	if (rn->tn_op != CVT || !rn->tn_type->t_is_enum)
		return;
	if (rn->tn_left->tn_op != LOAD)
		return;

	const type_t *rtp = rn->tn_left->tn_type;
	const sym_t *ec = rtp->t_enum->en_first_enumerator;
	const sym_t *max_ec = ec;
	lint_assert(ec != NULL);
	for (ec = ec->s_next; ec != NULL; ec = ec->s_next)
		if (ec->u.s_enum_constant > max_ec->u.s_enum_constant)
			max_ec = ec;

	int64_t max_enum_value = max_ec->u.s_enum_constant;
	lint_assert(INT_MIN <= max_enum_value && max_enum_value <= INT_MAX);

	int max_array_index = ltp->t_dim - 1;
	if (max_enum_value == max_array_index)
		return;

	/*
	 * If the name of the largest enum constant contains 'MAX' or 'NUM',
	 * that constant is typically not part of the allowed enum values but
	 * a marker for the number of actual enum values.
	 */
	if (max_enum_value == max_array_index + 1 &&
	    (strstr(max_ec->s_name, "MAX") != NULL ||
	     strstr(max_ec->s_name, "max") != NULL ||
	     strstr(max_ec->s_name, "NUM") != NULL ||
	     strstr(max_ec->s_name, "num") != NULL))
		return;

	/* maximum value %d of '%s' does not match maximum array index %d */
	warning(348, (int)max_enum_value, type_name(rtp), max_array_index);
	print_previous_declaration(max_ec);
}

/*
 * Create a node for operators PLUS and MINUS.
 */
static tnode_t *
build_plus_minus(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{

	/* If pointer and integer, move the pointer to the left. */
	if (rn->tn_type->t_tspec == PTR && is_integer(ln->tn_type->t_tspec)) {
		tnode_t *tmp = ln;
		ln = rn;
		rn = tmp;
		/* pointer addition has integer on the left-hand side */
		query_message(5);
	}

	/* pointer +- integer */
	if (ln->tn_type->t_tspec == PTR && rn->tn_type->t_tspec != PTR) {
		lint_assert(is_integer(rn->tn_type->t_tspec));

		check_ctype_macro_invocation(ln, rn);
		check_enum_array_index(ln, rn);

		tnode_t *elsz = subt_size_in_bytes(ln->tn_type);
		if (rn->tn_type->t_tspec != elsz->tn_type->t_tspec)
			rn = convert(NOOP, 0, elsz->tn_type, rn);

		tnode_t *prod = build_op(MULT, sys, rn->tn_type, rn, elsz);
		if (rn->tn_op == CON)
			prod = fold(prod);

		return build_op(op, sys, ln->tn_type, ln, prod);
	}

	/* pointer - pointer */
	if (rn->tn_type->t_tspec == PTR) {
		lint_assert(ln->tn_type->t_tspec == PTR);
		lint_assert(op == MINUS);

		type_t *ptrdiff = gettyp(PTRDIFF_TSPEC);
		tnode_t *raw_diff = build_op(op, sys, ptrdiff, ln, rn);
		if (ln->tn_op == CON && rn->tn_op == CON)
			raw_diff = fold(raw_diff);

		tnode_t *elsz = subt_size_in_bytes(ln->tn_type);
		balance(NOOP, &raw_diff, &elsz);

		return build_op(DIV, sys, ptrdiff, raw_diff, elsz);
	}

	return build_op(op, sys, ln->tn_type, ln, rn);
}

/*
 * Create a node for operators SHL and SHR.
 */
static tnode_t *
build_bit_shift(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{

	if (!allow_c90 && rn->tn_type->t_tspec != INT)
		// XXX: C1978 7.5 says: "Both [operators] perform the usual
		// arithmetic conversions on their operands."
		// TODO: Add a test to exercise this part of the code.
		rn = convert(NOOP, 0, gettyp(INT), rn);
	return build_op(op, sys, ln->tn_type, ln, rn);
}

static bool
is_null_pointer(const tnode_t *tn)
{
	tspec_t t = tn->tn_type->t_tspec;

	// TODO: Investigate how other pointers are stored, in particular,
	// whether a pointer constant can have a non-zero value.
	// If not, simplify the code below.
	return ((t == PTR && tn->tn_type->t_subt->t_tspec == VOID) ||
		is_integer(t))
	       && (tn->tn_op == CON && tn->tn_val.u.integer == 0);
}

/* Return a type based on tp1, with added qualifiers from tp2. */
static type_t *
merge_qualifiers(type_t *tp1, const type_t *tp2)
{

	lint_assert(tp1->t_tspec == PTR);
	lint_assert(tp2->t_tspec == PTR);

	bool c1 = tp1->t_subt->t_const;
	bool c2 = tp2->t_subt->t_const;
	bool v1 = tp1->t_subt->t_volatile;
	bool v2 = tp2->t_subt->t_volatile;

	if (c1 == (c1 | c2) && v1 == (v1 | v2))
		return tp1;

	type_t *nstp = expr_dup_type(tp1->t_subt);
	nstp->t_const |= c2;
	nstp->t_volatile |= v2;

	type_t *ntp = expr_dup_type(tp1);
	ntp->t_subt = nstp;
	return ntp;
}

/* See C99 6.5.15 "Conditional operator". */
static tnode_t *
build_colon(bool sys, tnode_t *ln, tnode_t *rn)
{

	tspec_t lt = ln->tn_type->t_tspec;
	tspec_t rt = rn->tn_type->t_tspec;

	type_t *tp;
	if (is_arithmetic(lt) && is_arithmetic(rt)) {
		/* The operands were already balanced in build_binary. */
		tp = ln->tn_type;
	} else if (lt == BOOL && rt == BOOL) {
		tp = ln->tn_type;
	} else if (lt == VOID || rt == VOID) {
		tp = gettyp(VOID);
	} else if (is_struct_or_union(lt)) {
		/* Both types must be identical. */
		lint_assert(is_struct_or_union(rt));
		lint_assert(ln->tn_type->t_sou == rn->tn_type->t_sou);
		if (is_incomplete(ln->tn_type)) {
			/* unknown operand size, op '%s' */
			error(138, op_name(COLON));
			return NULL;
		}
		tp = ln->tn_type;
	} else if (lt == PTR && is_integer(rt)) {
		if (rt != PTRDIFF_TSPEC)
			rn = convert(NOOP, 0, gettyp(PTRDIFF_TSPEC), rn);
		tp = ln->tn_type;
	} else if (rt == PTR && is_integer(lt)) {
		if (lt != PTRDIFF_TSPEC)
			ln = convert(NOOP, 0, gettyp(PTRDIFF_TSPEC), ln);
		tp = rn->tn_type;
	} else if (lt == PTR && is_null_pointer(rn)) {
		tp = merge_qualifiers(ln->tn_type, rn->tn_type);
	} else if (rt == PTR && is_null_pointer(ln)) {
		tp = merge_qualifiers(rn->tn_type, ln->tn_type);
	} else if (lt == PTR && ln->tn_type->t_subt->t_tspec == VOID) {
		tp = merge_qualifiers(ln->tn_type, rn->tn_type);
	} else if (rt == PTR && rn->tn_type->t_subt->t_tspec == VOID) {
		tp = merge_qualifiers(rn->tn_type, ln->tn_type);
	} else {
		/*
		 * XXX For now we simply take the left type. This is
		 * probably wrong, if one type contains a function prototype
		 * and the other one, at the same place, only an old-style
		 * declaration.
		 */
		tp = merge_qualifiers(ln->tn_type, rn->tn_type);
	}

	return build_op(COLON, sys, tp, ln, rn);
}

/* TODO: check for varargs */
static bool
is_cast_redundant(const tnode_t *tn)
{
	const type_t *ntp = tn->tn_type, *otp = tn->tn_left->tn_type;
	tspec_t nt = ntp->t_tspec, ot = otp->t_tspec;

	if (nt == BOOL || ot == BOOL)
		return nt == BOOL && ot == BOOL;

	if (is_integer(nt) && is_integer(ot)) {
		unsigned int nw = width_in_bits(ntp), ow = width_in_bits(otp);
		if (is_uinteger(nt) == is_uinteger(ot))
		       return nw >= ow;
		return is_uinteger(ot) && nw > ow;
	}

	if (is_complex(nt) || is_complex(ot))
		return is_complex(nt) && is_complex(ot) &&
		       size_in_bits(nt) >= size_in_bits(ot);

	if (is_floating(nt) && is_floating(ot))
		return size_in_bits(nt) >= size_in_bits(ot);

	if (nt == PTR && ot == PTR) {
		if (!ntp->t_subt->t_const && otp->t_subt->t_const)
			return false;
		if (!ntp->t_subt->t_volatile && otp->t_subt->t_volatile)
			return false;

		if (ntp->t_subt->t_tspec == VOID ||
		    otp->t_subt->t_tspec == VOID ||
		    types_compatible(ntp->t_subt, otp->t_subt,
			false, false, NULL))
			return true;
	}

	return false;
}

static bool
is_assignment(op_t op)
{

	return op == ASSIGN ||
	       op == MULASS ||
	       op == DIVASS ||
	       op == MODASS ||
	       op == ADDASS ||
	       op == SUBASS ||
	       op == SHLASS ||
	       op == SHRASS ||
	       op == ANDASS ||
	       op == XORASS ||
	       op == ORASS ||
	       op == RETURN ||
	       op == INIT;
}

/* Create a node for an assignment operator (both '=' and 'op='). */
static tnode_t *
build_assignment(op_t op, bool sys, tnode_t *ln, tnode_t *rn)
{

	tspec_t lt = ln->tn_type->t_tspec;
	tspec_t rt = rn->tn_type->t_tspec;

	if (any_query_enabled && is_assignment(rn->tn_op)) {
		/* chained assignment with '%s' and '%s' */
		query_message(10, op_name(op), op_name(rn->tn_op));
	}

	if ((op == ADDASS || op == SUBASS) && lt == PTR) {
		lint_assert(is_integer(rt));
		tnode_t *ctn = subt_size_in_bytes(ln->tn_type);
		if (rn->tn_type->t_tspec != ctn->tn_type->t_tspec)
			rn = convert(NOOP, 0, ctn->tn_type, rn);
		rn = build_op(MULT, sys, rn->tn_type, rn, ctn);
		if (rn->tn_left->tn_op == CON)
			rn = fold(rn);
	}

	if ((op == ASSIGN || op == RETURN || op == INIT) &&
	    (lt == STRUCT || rt == STRUCT)) {
		lint_assert(lt == rt);
		lint_assert(ln->tn_type->t_sou == rn->tn_type->t_sou);
		if (is_incomplete(ln->tn_type)) {
			if (op == RETURN) {
				/* cannot return incomplete type */
				error(212);
			} else {
				/* unknown operand size, op '%s' */
				error(138, op_name(op));
			}
			return NULL;
		}
	}

	if (op == SHLASS && hflag && allow_trad && allow_c90
	    && portable_rank_cmp(lt, rt) < 0)
		/* semantics of '%s' change in ANSI C; ... */
		warning(118, "<<=");

	if (op != SHLASS && op != SHRASS
	    && (op == ASSIGN || lt != PTR)
	    && (lt != rt || (ln->tn_type->t_bitfield && rn->tn_op == CON))) {
		rn = convert(op, 0, ln->tn_type, rn);
		rt = lt;
	}

	if (any_query_enabled && rn->tn_op == CVT && rn->tn_cast &&
	    types_compatible(ln->tn_type, rn->tn_type, false, false, NULL) &&
	    is_cast_redundant(rn)) {
		/* redundant cast from '%s' to '%s' before assignment */
		query_message(7,
		    type_name(rn->tn_left->tn_type), type_name(rn->tn_type));
	}

	return build_op(op, sys, ln->tn_type, ln, rn);
}

/*
 * Create a node for REAL, IMAG
 */
static tnode_t *
build_real_imag(op_t op, bool sys, tnode_t *ln)
{

	lint_assert(ln != NULL);
	if (ln->tn_op == NAME) {
		/*
		 * This may be too much, but it avoids wrong warnings.
		 * See d_c99_complex_split.c.
		 */
		mark_as_used(ln->tn_sym, false, false);
		mark_as_set(ln->tn_sym);
	}

	tspec_t t;
	switch (ln->tn_type->t_tspec) {
	case LCOMPLEX:
		t = LDOUBLE;
		break;
	case DCOMPLEX:
		t = DOUBLE;
		break;
	case FCOMPLEX:
		t = FLOAT;
		break;
	default:
		/* '__%s__' is illegal for type '%s' */
		error(276, op == REAL ? "real" : "imag",
		    type_name(ln->tn_type));
		return NULL;
	}

	tnode_t *ntn = build_op(op, sys, gettyp(t), ln, NULL);
	ntn->tn_lvalue = true;
	return ntn;
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

/*
 * Fold constant nodes, as much as is needed for comparing the value with 0.
 */
static tnode_t *
fold_bool(tnode_t *tn)
{

	val_t *v = xcalloc(1, sizeof(*v));
	v->v_tspec = tn->tn_type->t_tspec;
	lint_assert(v->v_tspec == INT || (Tflag && v->v_tspec == BOOL));

	bool l = constant_is_nonzero(tn->tn_left);
	bool r = is_binary(tn) && constant_is_nonzero(tn->tn_right);

	switch (tn->tn_op) {
	case NOT:
		if (hflag && !suppress_constcond)
			/* constant argument to '!' */
			warning(239);
		v->u.integer = !l ? 1 : 0;
		break;
	case LOGAND:
		v->u.integer = l && r ? 1 : 0;
		break;
	case LOGOR:
		v->u.integer = l || r ? 1 : 0;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}

	return build_constant(tn->tn_type, v);
}

static long double
floating_error_value(tspec_t t, long double lv)
{
	if (t == FLOAT)
		return lv < 0 ? -FLT_MAX : FLT_MAX;
	if (t == DOUBLE)
		return lv < 0 ? -DBL_MAX : DBL_MAX;
	/*
	 * When NetBSD is cross-built in MKLINT=yes mode on x86_64 for
	 * sparc64, tools/lint checks this code while building usr.bin/xlint.
	 * In that situation, lint uses the preprocessor for sparc64, in which
	 * the type 'long double' is IEEE-754-binary128, affecting the macro
	 * LDBL_MAX below. The type 'long double', as well as the strtold
	 * implementation, comes from the host platform x86_64 though, where
	 * 'long double' consumes 128 bits as well but only uses 80 of them.
	 * The exponent range of the two 'long double' types is the same, but
	 * the maximum finite value differs due to the extended precision on
	 * sparc64.
	 *
	 * To properly handle the data types of the target platform, lint
	 * would have to implement the floating-point types in a
	 * platform-independent way, which is not worth the effort, given how
	 * few programs practically use 'long double'.
	 */
	/* LINTED 248: floating-point constant out of range */
	long double max = LDBL_MAX;
	return lv < 0 ? -max : max;
}

static bool
is_floating_overflow(tspec_t t, long double val)
{
	if (fpe != 0 || isfinite(val) == 0)
		return true;
	if (t == FLOAT && (val > FLT_MAX || val < -FLT_MAX))
		return true;
	if (t == DOUBLE && (val > DBL_MAX || val < -DBL_MAX))
		return true;
	return false;
}

/*
 * Fold constant nodes having operands with floating point type.
 */
static tnode_t *
fold_float(tnode_t *tn)
{

	fpe = 0;

	tspec_t t = tn->tn_type->t_tspec;

	val_t *v = xcalloc(1, sizeof(*v));
	v->v_tspec = t;

	lint_assert(is_floating(t));
	lint_assert(t == tn->tn_left->tn_type->t_tspec);
	lint_assert(!is_binary(tn) || t == tn->tn_right->tn_type->t_tspec);

	long double lv = tn->tn_left->tn_val.u.floating;
	long double rv = is_binary(tn) ? tn->tn_right->tn_val.u.floating : 0.0;

	switch (tn->tn_op) {
	case UPLUS:
		v->u.floating = lv;
		break;
	case UMINUS:
		v->u.floating = -lv;
		break;
	case MULT:
		v->u.floating = lv * rv;
		break;
	case DIV:
		if (rv == 0.0) {
			/* division by 0 */
			error(139);
			v->u.floating = floating_error_value(t, lv);
		} else {
			v->u.floating = lv / rv;
		}
		break;
	case PLUS:
		v->u.floating = lv + rv;
		break;
	case MINUS:
		v->u.floating = lv - rv;
		break;
	case LT:
		v->u.integer = lv < rv ? 1 : 0;
		break;
	case LE:
		v->u.integer = lv <= rv ? 1 : 0;
		break;
	case GE:
		v->u.integer = lv >= rv ? 1 : 0;
		break;
	case GT:
		v->u.integer = lv > rv ? 1 : 0;
		break;
	case EQ:
		v->u.integer = lv == rv ? 1 : 0;
		break;
	case NE:
		v->u.integer = lv != rv ? 1 : 0;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}

	// XXX: Must not access u.floating after setting u.integer.
	lint_assert(fpe != 0 || isnan(v->u.floating) == 0);
	if (is_complex(v->v_tspec)) {
		/*
		 * Don't warn, as lint doesn't model the imaginary part of
		 * complex numbers.
		 */
		fpe = 0;
	} else if (is_floating_overflow(t, v->u.floating)) {
		/* operator '%s' produces floating point overflow */
		warning(142, op_name(tn->tn_op));
		v->u.floating = floating_error_value(t, v->u.floating);
		fpe = 0;
	}

	return build_constant(tn->tn_type, v);
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
	const mod_t *mp = &modtab[op];

	/* If there was an error in one of the operands, return. */
	if (ln == NULL || (mp->m_binary && rn == NULL))
		return NULL;

	/*
	 * Apply class conversions to the left operand, but only if its
	 * value is needed or compared with zero.
	 */
	if (mp->m_value_context || mp->m_compares_with_zero)
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
	 * short would be promoted to int. Types are also tested to be
	 * CHAR, which would also become int.
	 */
	if (mp->m_comparison)
		check_integer_comparison(op, ln, rn);

	if (mp->m_value_context || mp->m_compares_with_zero)
		ln = promote(op, false, ln);
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
	    ln->tn_op == CON && ln->tn_val.v_unsigned_since_c90) {
		/* ANSI C treats constant as unsigned, op '%s' */
		warning(218, op_name(op));
		ln->tn_val.v_unsigned_since_c90 = false;
	}
	if (mp->m_warn_if_right_unsigned_in_c90 &&
	    rn->tn_op == CON && rn->tn_val.v_unsigned_since_c90) {
		/* ANSI C treats constant as unsigned, op '%s' */
		warning(218, op_name(op));
		rn->tn_val.v_unsigned_since_c90 = false;
	}

	/* Make sure both operands are of the same type */
	if (mp->m_balance_operands || (!allow_c90 && (op == SHL || op == SHR)))
		balance(op, &ln, &rn);

	/*
	 * Check types for compatibility with the operation and mutual
	 * compatibility. Return if there are serious problems.
	 */
	if (!typeok(op, 0, ln, rn))
		return NULL;

	/* And now create the node. */
	tnode_t *ntn;
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
		ntn = build_op(INDIR, sys, ln->tn_type->t_subt, ln, NULL);
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
		if (any_query_enabled) {
			/* comma operator with types '%s' and '%s' */
			query_message(12,
			    type_name(ln->tn_type), type_name(rn->tn_type));
		}
		/* FALLTHROUGH */
	case QUEST:
		ntn = build_op(op, sys, rn->tn_type, ln, rn);
		break;
	case REAL:
	case IMAG:
		ntn = build_real_imag(op, sys, ln);
		break;
	default:
		lint_assert(mp->m_binary == (rn != NULL));
		type_t *rettp = mp->m_returns_bool
		    ? gettyp(Tflag ? BOOL : INT) : ln->tn_type;
		ntn = build_op(op, sys, rettp, ln, rn);
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
	 * it is compared with zero and if this operand is a constant.
	 */
	if (hflag && !suppress_constcond &&
	    mp->m_compares_with_zero &&
	    (ln->tn_op == CON ||
	     ((mp->m_binary && op != QUEST) && rn->tn_op == CON)) &&
	    /* XXX: rn->tn_system_dependent should be checked as well */
	    !ln->tn_system_dependent) {
		/* constant in conditional context */
		warning(161);
	}

	/* Fold if the operator requires it */
	if (mp->m_fold_constant_operands) {
		if (ln->tn_op == CON && (!mp->m_binary || rn->tn_op == CON)) {
			if (mp->m_compares_with_zero) {
				ntn = fold_bool(ntn);
			} else if (is_floating(ntn->tn_type->t_tspec)) {
				ntn = fold_float(ntn);
			} else {
				ntn = fold(ntn);
			}
		} else if (op == QUEST && ln->tn_op == CON) {
			ntn = ln->tn_val.u.integer != 0
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

static bool
are_members_compatible(const sym_t *a, const sym_t *b)
{
	if (a->u.s_member.sm_offset_in_bits != b->u.s_member.sm_offset_in_bits)
		return false;

	const type_t *atp = a->s_type;
	const type_t *btp = b->s_type;
	bool w = false;
	if (!types_compatible(atp, btp, false, false, &w) && !w)
		return false;
	if (a->s_bitfield != b->s_bitfield)
		return false;
	if (a->s_bitfield) {
		if (atp->t_bit_field_width != btp->t_bit_field_width)
			return false;
		if (atp->t_bit_field_offset != btp->t_bit_field_offset)
			return false;
	}
	return true;
}

/*
 * Return whether all struct/union members with the same name have the same
 * type and offset.
 */
static bool
all_members_compatible(const sym_t *msym)
{
	for (const sym_t *csym = msym;
	     csym != NULL; csym = csym->s_symtab_next) {
		if (!is_member(csym))
			continue;
		if (strcmp(msym->s_name, csym->s_name) != 0)
			continue;

		for (const sym_t *sym = csym->s_symtab_next;
		     sym != NULL; sym = sym->s_symtab_next) {
			if (is_member(sym)
			    && strcmp(csym->s_name, sym->s_name) == 0
			    && !are_members_compatible(csym, sym))
				return false;
		}
	}
	return true;
}

sym_t *
find_member(const struct_or_union *sou, const char *name)
{
	for (sym_t *mem = sou->sou_first_member;
	     mem != NULL; mem = mem->s_next) {
		lint_assert(is_member(mem));
		lint_assert(mem->u.s_member.sm_containing_type == sou);
		if (strcmp(mem->s_name, name) == 0)
			return mem;
	}

	for (sym_t *mem = sou->sou_first_member;
	     mem != NULL; mem = mem->s_next) {
		if (is_struct_or_union(mem->s_type->t_tspec)
		    && mem->s_name == unnamed) {
			sym_t *nested_mem =
			    find_member(mem->s_type->t_sou, name);
			if (nested_mem != NULL)
				return nested_mem;
		}
	}
	return NULL;
}

/*
 * Remove the member if it was unknown until now, which means
 * that no defined struct or union has a member with the same name.
 */
static void
remove_unknown_member(tnode_t *tn, sym_t *msym)
{
	/* type '%s' does not have member '%s' */
	error(101, type_name(tn->tn_type), msym->s_name);
	rmsym(msym);
	msym->s_kind = FMEMBER;
	msym->s_scl = STRUCT_MEMBER;

	struct_or_union *sou = expr_zero_alloc(sizeof(*sou),
	    "struct_or_union");
	sou->sou_tag = expr_zero_alloc(sizeof(*sou->sou_tag), "sym");
	sou->sou_tag->s_name = unnamed;

	msym->u.s_member.sm_containing_type = sou;
	/*
	 * The member sm_offset_in_bits is not needed here since this
	 * symbol can only be used for error reporting.
	 */
}

/*
 * Returns a symbol which has the same name as the msym argument and is a
 * member of the struct or union specified by the tn argument.
 */
static sym_t *
struct_or_union_member(tnode_t *tn, op_t op, sym_t *msym)
{

	/* Determine the tag type of which msym is expected to be a member. */
	const type_t *tp = NULL;
	if (op == POINT && is_struct_or_union(tn->tn_type->t_tspec))
		tp = tn->tn_type;
	if (op == ARROW && tn->tn_type->t_tspec == PTR
	    && is_struct_or_union(tn->tn_type->t_subt->t_tspec))
		tp = tn->tn_type->t_subt;
	struct_or_union *sou = tp != NULL ? tp->t_sou : NULL;

	if (sou != NULL) {
		sym_t *nested_mem = find_member(sou, msym->s_name);
		if (nested_mem != NULL)
			return nested_mem;
	}

	if (msym->s_scl == NOSCL) {
		remove_unknown_member(tn, msym);
		return msym;
	}

	bool eq = all_members_compatible(msym);

	/*
	 * Now handle the case in which the left operand refers really
	 * to a struct/union, but the right operand is not member of it.
	 */
	if (sou != NULL) {
		if (eq && !allow_c90) {
			/* illegal use of member '%s' */
			warning(102, msym->s_name);
		} else {
			/* illegal use of member '%s' */
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
			if (!allow_c90) {
				/* left operand of '.' must be struct ... */
				warning(103, type_name(tn->tn_type));
			} else {
				/* left operand of '.' must be struct ... */
				error(103, type_name(tn->tn_type));
			}
		} else {
			if (!allow_c90 && tn->tn_type->t_tspec == PTR) {
				/* left operand of '->' must be pointer ... */
				warning(104, type_name(tn->tn_type));
			} else {
				/* left operand of '->' must be pointer ... */
				error(104, type_name(tn->tn_type));
			}
		}
	} else {
		if (!allow_c90) {
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
build_member_access(tnode_t *ln, op_t op, bool sys, sbuf_t *member)
{
	sym_t *msym;

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
		tn = build_op(ADDR, tn->tn_sys,
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
		type_t *tp = expr_dup_type(tn->tn_type);
		/* C99 6.3.2.1p2 sentence 2 says to remove the qualifiers. */
		tp->t_const = tp->t_volatile = false;
		tn = build_op(LOAD, tn->tn_sys, tp, tn, NULL);
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

/*
 * Most errors required by ANSI C are reported in struct_or_union_member().
 * Here we only check for totally wrong things.
 */
static bool
typeok_point(const tnode_t *ln, const type_t *ltp, tspec_t lt)
{
	if (is_struct_or_union(lt))
		return true;

	if (lt == FUNC || lt == VOID || ltp->t_bitfield)
		goto wrong;

	/*
	 * Some C dialects from before C90 tolerated any lvalue on the
	 * left-hand side of the '.' operator, allowing things like
	 * char st[100]; st.st_mtime, assuming that the member 'st_mtime'
	 * only occurred in a single struct; see typeok_arrow.
	 */
	if (ln->tn_lvalue)
		return true;

wrong:
	/* With allow_c90 we already got an error */
	if (!allow_c90)
		/* unacceptable operand of '%s' */
		error(111, op_name(POINT));

	return false;
}

static bool
typeok_arrow(tspec_t lt)
{
	/*
	 * C1978 Appendix A 14.1 says: <quote>In fact, any lvalue is allowed
	 * before '.', and that lvalue is then assumed to have the form of
	 * the structure of which the name of the right is a member. [...]
	 * Such constructions are non-portable.</quote>
	 */
	if (lt == PTR || (!allow_c90 && is_integer(lt)))
		return true;

	/* With allow_c90 we already got an error */
	if (!allow_c90)
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
	}
	if (tp->t_const && allow_c90) {
		/* %soperand of '%s' must be modifiable lvalue */
		warning(115, "", op_name(op));
	}
	return true;
}

static bool
typeok_address(op_t op, const tnode_t *tn, const type_t *tp, tspec_t t)
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
		error(114, "", op_name(op));
		return false;
	} else if (is_scalar(t)) {
		if (tp->t_bitfield) {
			/* cannot take address of bit-field */
			error(112);
			return false;
		}
	} else if (t != STRUCT && t != UNION) {
		/* unacceptable operand of '%s' */
		error(111, op_name(op));
		return false;
	}
	if (tn->tn_op == NAME && tn->tn_sym->s_register) {
		/* cannot take address of register '%s' */
		error(113, tn->tn_sym->s_name);
		return false;
	}
	return true;
}

static bool
typeok_indir(const type_t *tp, tspec_t t)
{

	if (t != PTR) {
		/* cannot dereference non-pointer type '%s' */
		error(96, type_name(tp));
		return false;
	}
	return true;
}

static void
warn_incompatible_types(op_t op,
			const type_t *ltp, tspec_t lt,
			const type_t *rtp, tspec_t rt)
{
	bool binary = modtab[op].m_binary;

	if (lt == VOID || (binary && rt == VOID)) {
		/* void type illegal in expression */
		error(109);
	} else if (op == ASSIGN) {
		/* cannot assign to '%s' from '%s' */
		error(171, type_name(ltp), type_name(rtp));
	} else if (binary) {
		/* operands of '%s' have incompatible types '%s' and '%s' */
		error(107, op_name(op), type_name(ltp), type_name(rtp));
	} else {
		lint_assert(rt == NO_TSPEC);
		/* operand of '%s' has invalid type '%s' */
		error(108, op_name(op), type_name(ltp));
	}
}

static bool
typeok_plus(op_t op,
	    const type_t *ltp, tspec_t lt,
	    const type_t *rtp, tspec_t rt)
{
	/* operands have scalar types (checked in typeok) */
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
	/* operands have scalar types (checked in typeok) */
	if ((lt == PTR && rt != PTR && !is_integer(rt)) ||
	    (lt != PTR && rt == PTR)) {
		warn_incompatible_types(op, ltp, lt, rtp, rt);
		return false;
	}
	if (lt == PTR && rt == PTR &&
	    !types_compatible(ltp->t_subt, rtp->t_subt, true, false, NULL)) {
		/* illegal pointer subtraction */
		error(116);
	}
	return true;
}

static void
typeok_shr(op_t op,
	   const tnode_t *ln, tspec_t lt,
	   const tnode_t *rn, tspec_t rt)
{
	tspec_t olt = before_conversion(ln)->tn_type->t_tspec;
	tspec_t ort = before_conversion(rn)->tn_type->t_tspec;

	/* operands have integer types (checked in typeok) */
	if (pflag && !is_uinteger(olt)) {
		integer_constraints lc = ic_expr(ln);
		if (!ic_maybe_signed(ln->tn_type, &lc))
			return;

		/*
		 * The left operand is signed. This means that
		 * the operation is (possibly) nonportable.
		 */
		if (ln->tn_op != CON) {
			/* bitwise '%s' on signed value possibly nonportable */
			warning(117, op_name(op));
		} else if (ln->tn_val.u.integer < 0) {
			/* bitwise '%s' on signed value nonportable */
			warning(120, op_name(op));
		}
	} else if (allow_trad && allow_c90 &&
		   !is_uinteger(olt) && is_uinteger(ort)) {
		/* The left operand would become unsigned in traditional C. */
		if (hflag && (ln->tn_op != CON || ln->tn_val.u.integer < 0)) {
			/* semantics of '%s' change in ANSI C; use ... */
			warning(118, op_name(op));
		}
	} else if (allow_trad && allow_c90 &&
		   !is_uinteger(olt) && !is_uinteger(ort) &&
	    portable_rank_cmp(lt, rt) < 0) {
		/*
		 * In traditional C, the left operand would be extended
		 * (possibly sign-extended) and then shifted.
		 */
		if (hflag && (ln->tn_op != CON || ln->tn_val.u.integer < 0)) {
			/* semantics of '%s' change in ANSI C; use ... */
			warning(118, op_name(op));
		}
	}
}

static void
typeok_shl(op_t op, tspec_t lt, tspec_t rt)
{
	/*
	 * C90 does not perform balancing for shift operations,
	 * but traditional C does. If the width of the right operand
	 * is greater than the width of the left operand, then in
	 * traditional C the left operand would be extended to the
	 * width of the right operand. For SHL this may result in
	 * different results.
	 */
	if (portable_rank_cmp(lt, rt) < 0) {
		/*
		 * XXX If both operands are constant, make sure
		 * that there is really a difference between
		 * ANSI C and traditional C.
		 */
		if (hflag && allow_trad && allow_c90)
			/* semantics of '%s' change in ANSI C; use ... */
			warning(118, op_name(op));
	}
}

static void
typeok_shift(const type_t *ltp, tspec_t lt, const tnode_t *rn, tspec_t rt)
{
	if (rn->tn_op != CON)
		return;

	if (!is_uinteger(rt) && rn->tn_val.u.integer < 0) {
		/* negative shift */
		warning(121);
	} else if ((uint64_t)rn->tn_val.u.integer == size_in_bits(lt)) {
		/* shift amount %u equals bit-size of '%s' */
		warning(267, (unsigned)rn->tn_val.u.integer, type_name(ltp));
	} else if ((uint64_t)rn->tn_val.u.integer > size_in_bits(lt)) {
		/* shift amount %llu is greater than bit-size %llu of '%s' */
		warning(122, (unsigned long long)rn->tn_val.u.integer,
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

/*
 * Called if incompatible pointer types are detected.
 * Print an appropriate warning.
 */
static void
warn_incompatible_pointers(op_t op, const type_t *ltp, const type_t *rtp)
{
	lint_assert(ltp->t_tspec == PTR);
	lint_assert(rtp->t_tspec == PTR);

	tspec_t lt = ltp->t_subt->t_tspec;
	tspec_t rt = rtp->t_subt->t_tspec;

	if (is_struct_or_union(lt) && is_struct_or_union(rt)) {
		if (op == RETURN) {
			/* illegal structure pointer combination */
			warning(244);
		} else {
			/* incompatible structure pointers: '%s' '%s' '%s' */
			warning(245, type_name(ltp),
			    op_name(op), type_name(rtp));
		}
	} else {
		if (op == RETURN) {
			/* illegal combination of '%s' and '%s' */
			warning(184, type_name(ltp), type_name(rtp));
		} else {
			/* illegal combination of '%s' and '%s', op '%s' */
			warning(124,
			    type_name(ltp), type_name(rtp), op_name(op));
		}
	}
}

static void
check_pointer_comparison(op_t op, const tnode_t *ln, const tnode_t *rn)
{
	type_t *ltp = ln->tn_type, *rtp = rn->tn_type;
	tspec_t lst = ltp->t_subt->t_tspec, rst = rtp->t_subt->t_tspec;

	if (lst == VOID || rst == VOID) {
		/* TODO: C99 behaves like C90 here. */
		if ((!allow_trad && !allow_c99) &&
		    (lst == FUNC || rst == FUNC)) {
			/* (void *)0 is already handled in typeok() */
			const char *lsts, *rsts;
			*(lst == FUNC ? &lsts : &rsts) = "function pointer";
			*(lst == VOID ? &lsts : &rsts) = "'void *'";
			/* ANSI C forbids comparison of %s with %s */
			warning(274, lsts, rsts);
		}
		return;
	}

	if (!types_compatible(ltp->t_subt, rtp->t_subt, true, false, NULL)) {
		warn_incompatible_pointers(op, ltp, rtp);
		return;
	}

	if (lst == FUNC && rst == FUNC) {
		/* TODO: C99 behaves like C90 here, see C99 6.5.8p2. */
		if ((!allow_trad && !allow_c99) && op != EQ && op != NE)
			/* ANSI C forbids ordered comparisons of ... */
			warning(125);
	}
}

static bool
typeok_compare(op_t op,
	       const tnode_t *ln, const type_t *ltp, tspec_t lt,
	       const tnode_t *rn, const type_t *rtp, tspec_t rt)
{
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

	const char *lx = lt == PTR ? "pointer" : "integer";
	const char *rx = rt == PTR ? "pointer" : "integer";
	/* illegal combination of %s '%s' and %s '%s', op '%s' */
	warning(123, lx, type_name(ltp), rx, type_name(rtp), op_name(op));
	return true;
}

static bool
typeok_quest(tspec_t lt, const tnode_t *rn)
{
	if (!is_scalar(lt)) {
		/* first operand of '?' must have scalar type */
		error(170);
		return false;
	}
	lint_assert(before_conversion(rn)->tn_op == COLON);
	return true;
}

static void
typeok_colon_pointer(const type_t *ltp, const type_t *rtp)
{
	type_t *lstp = ltp->t_subt;
	type_t *rstp = rtp->t_subt;
	tspec_t lst = lstp->t_tspec;
	tspec_t rst = rstp->t_tspec;

	if ((lst == VOID && rst == FUNC) || (lst == FUNC && rst == VOID)) {
		/* (void *)0 is handled in typeok_colon */
		/* TODO: C99 behaves like C90 here. */
		if (!allow_trad && !allow_c99)
			/* ANSI C forbids conversion of %s to %s, op %s */
			warning(305, "function pointer", "'void *'",
			    op_name(COLON));
		return;
	}

	if (pointer_types_are_compatible(lstp, rstp, true))
		return;
	if (!types_compatible(lstp, rstp, true, false, NULL))
		warn_incompatible_pointers(COLON, ltp, rtp);
}

static bool
typeok_colon(const tnode_t *ln, const type_t *ltp, tspec_t lt,
	     const tnode_t *rn, const type_t *rtp, tspec_t rt)
{

	if (is_arithmetic(lt) && is_arithmetic(rt))
		return true;
	if (lt == BOOL && rt == BOOL)
		return true;

	if (lt == STRUCT && rt == STRUCT && ltp->t_sou == rtp->t_sou)
		return true;
	if (lt == UNION && rt == UNION && ltp->t_sou == rtp->t_sou)
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
		    rx, type_name(rtp), op_name(COLON));
		return true;
	}

	if (lt == VOID || rt == VOID) {
		if (lt != VOID || rt != VOID)
			/* incompatible types '%s' and '%s' in conditional */
			warning(126, type_name(ltp), type_name(rtp));
		return true;
	}

	if (lt == PTR && rt == PTR) {
		typeok_colon_pointer(ltp, rtp);
		return true;
	}

	/* incompatible types '%s' and '%s' in conditional */
	error(126, type_name(ltp), type_name(rtp));
	return false;
}

/*
 * Returns true if the given structure or union has a constant member
 * (maybe recursively).
 */
static bool
has_constant_member(const type_t *tp)
{
	lint_assert(is_struct_or_union(tp->t_tspec));

	for (sym_t *m = tp->t_sou->sou_first_member;
	     m != NULL; m = m->s_next) {
		const type_t *mtp = m->s_type;
		if (mtp->t_const)
			return true;
		if (is_struct_or_union(mtp->t_tspec) &&
		    has_constant_member(mtp))
			return true;
	}
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
	} else if (ltp->t_const
	    || (is_struct_or_union(lt) && has_constant_member(ltp))) {
		if (allow_c90)
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

static void
check_assign_void_pointer(op_t op, int arg,
			  tspec_t lt, tspec_t lst,
			  tspec_t rt, tspec_t rst)
{

	if (!(lt == PTR && rt == PTR && (lst == VOID || rst == VOID)))
		return;
	/* two pointers, at least one pointer to void */

	/* TODO: C99 behaves like C90 here. */
	if (!((!allow_trad && !allow_c99) && (lst == FUNC || rst == FUNC)))
		return;
	/* comb. of ptr to func and ptr to void */

	const char *lts, *rts;
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

	const type_t *tp = before_conversion(tn)->tn_type;
	return tp->t_tspec == PTR &&
	       tp->t_subt->t_tspec == CHAR &&
	       tp->t_subt->t_const;
}

static bool
is_first_arg_const_char_pointer(const tnode_t *tn)
{
	const tnode_t *an = tn->tn_right;
	if (an == NULL)
		return false;

	while (an->tn_right != NULL)
		an = an->tn_right;
	return is_const_char_pointer(an->tn_left);
}

static bool
is_const_pointer(const tnode_t *tn)
{
	const type_t *tp = before_conversion(tn)->tn_type;
	return tp->t_tspec == PTR && tp->t_subt->t_const;
}

static bool
is_second_arg_const_pointer(const tnode_t *tn)
{
	const tnode_t *an = tn->tn_right;
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

static bool
check_assign_void_pointer_compat(op_t op, int arg,
				 const type_t *const ltp, tspec_t const lt,
				 const type_t *const lstp, tspec_t const lst,
				 const tnode_t *const rn,
				 const type_t *const rtp, tspec_t const rt,
				 const type_t *const rstp, tspec_t const rst)
{
	if (!(lt == PTR && rt == PTR && (lst == VOID || rst == VOID ||
					 types_compatible(lstp, rstp,
					     true, false, NULL))))
		return false;

	/* compatible pointer types (qualifiers ignored) */
	if (allow_c90 &&
	    ((!lstp->t_const && rstp->t_const) ||
	     (!lstp->t_volatile && rstp->t_volatile))) {
		/* left side has not all qualifiers of right */
		switch (op) {
		case INIT:
		case RETURN:
			/* incompatible pointer types to '%s' and '%s' */
			warning(182, type_name(lstp), type_name(rstp));
			break;
		case FARG:
			/* converting '%s' to incompatible '%s' ... */
			warning(153,
			    type_name(rtp), type_name(ltp), arg);
			break;
		default:
			/* operands of '%s' have incompatible pointer ... */
			warning(128, op_name(op),
			    type_name(lstp), type_name(rstp));
			break;
		}
	}

	if (allow_c90)
		check_unconst_function(lstp, rn);

	return true;
}

static bool
check_assign_pointer_integer(op_t op, int arg,
			     const type_t *const ltp, tspec_t const lt,
			     const type_t *const rtp, tspec_t const rt)
{

	if (!((lt == PTR && is_integer(rt)) || (is_integer(lt) && rt == PTR)))
		return false;

	const char *lx = lt == PTR ? "pointer" : "integer";
	const char *rx = rt == PTR ? "pointer" : "integer";

	switch (op) {
	case INIT:
	case RETURN:
		/* illegal combination of %s '%s' and %s '%s' */
		warning(183, lx, type_name(ltp), rx, type_name(rtp));
		break;
	case FARG:
		/* illegal combination of %s '%s' and %s '%s', arg #%d */
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

	if (op == FARG)
		/* converting '%s' to incompatible '%s' for ... */
		warning(153, type_name(rtp), type_name(ltp), arg);
	else
		warn_incompatible_pointers(op, ltp, rtp);
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
		/* function has return type '%s' but returns '%s' */
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
	tspec_t lt, rt, lst = NO_TSPEC, rst = NO_TSPEC;
	type_t *ltp, *rtp, *lstp = NULL, *rstp = NULL;

	if ((lt = (ltp = ln->tn_type)->t_tspec) == PTR)
		lst = (lstp = ltp->t_subt)->t_tspec;
	if ((rt = (rtp = rn->tn_type)->t_tspec) == PTR)
		rst = (rstp = rtp->t_subt)->t_tspec;

	if (lt == BOOL && is_scalar(rt))	/* C99 6.3.1.2 */
		return true;

	if (is_arithmetic(lt) && (is_arithmetic(rt) || rt == BOOL))
		return true;

	if (is_struct_or_union(lt) && is_struct_or_union(rt))
		/* both are struct or union */
		return ltp->t_sou == rtp->t_sou;

	/* a null pointer may be assigned to any pointer */
	if (lt == PTR && is_null_pointer(rn)) {
		if (is_integer(rn->tn_type->t_tspec))
			/* implicit conversion from integer 0 to pointer ... */
			query_message(15, type_name(ltp));
		return true;
	}

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
	       tn->tn_val.u.integer == 0;
}

static void
check_null_effect(const tnode_t *tn)
{

	if (hflag &&
	    !has_side_effect(tn) &&
	    !(is_void_cast(tn) && is_local_symbol(tn->tn_left)) &&
	    !(is_void_cast(tn) && is_int_constant_zero(tn->tn_left))) {
		/* expression has null effect */
		warning(129);
	}
}

/*
 * Check the types for specific operators and type combinations.
 *
 * At this point, the operands already conform to the type requirements of
 * the operator, such as being integer, floating or scalar.
 */
static bool
typeok_op(op_t op, int arg,
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
		return typeok_indir(ltp, lt);
	case ADDR:
		return typeok_address(op, ln, ltp, lt);
	case PLUS:
		return typeok_plus(op, ltp, lt, rtp, rt);
	case MINUS:
		return typeok_minus(op, ltp, lt, rtp, rt);
	case SHL:
		typeok_shl(op, lt, rt);
		goto shift;
	case SHR:
		typeok_shr(op, ln, lt, rn, rt);
	shift:
		typeok_shift(ltp, lt, rn, rt);
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
		return typeok_colon(ln, ltp, lt, rn, rtp, rt);
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
		if (pflag && !is_uinteger(lt) &&
		    !(!allow_c90 && is_uinteger(rt))) {
			/* bitwise '%s' on signed value possibly nonportable */
			warning(117, op_name(op));
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

/* Prints a warning if a strange operator is used on an enum type. */
static void
check_bad_enum_operation(op_t op, const tnode_t *ln, const tnode_t *rn)
{

	if (!eflag)
		return;

	/* Allow enum in array indices. */
	if (op == PLUS &&
	    ((ln->tn_type->t_is_enum && rn->tn_type->t_tspec == PTR) ||
	     (rn->tn_type->t_is_enum && ln->tn_type->t_tspec == PTR))) {
		return;
	}

	/* dubious operation '%s' on enum */
	warning(241, op_name(op));
}

/* Prints a warning if an operator is applied to two different enum types. */
static void
check_enum_type_mismatch(op_t op, int arg, const tnode_t *ln, const tnode_t *rn)
{
	const mod_t *mp = &modtab[op];

	if (ln->tn_type->t_enum != rn->tn_type->t_enum) {
		switch (op) {
		case INIT:
			/* enum type mismatch between '%s' and '%s' in ... */
			warning(210,
			    type_name(ln->tn_type), type_name(rn->tn_type));
			break;
		case FARG:
			/* function expects '%s', passing '%s' for arg #%d */
			warning(156,
			    type_name(ln->tn_type), type_name(rn->tn_type),
			    arg);
			break;
		case RETURN:
			/* function has return type '%s' but returns '%s' */
			warning(211,
			    type_name(ln->tn_type), type_name(rn->tn_type));
			break;
		default:
			/* enum type mismatch: '%s' '%s' '%s' */
			warning(130, type_name(ln->tn_type), op_name(op),
			    type_name(rn->tn_type));
			break;
		}
	} else if (Pflag && eflag && mp->m_comparison && op != EQ && op != NE)
		/* operator '%s' assumes that '%s' is ordered */
		warning(243, op_name(op), type_name(ln->tn_type));
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
		    rn->tn_val.u.integer == 0) {
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
		/* combination of '%s' and '%s', op '%s' */
		warning(242, type_name(ln->tn_type), type_name(rn->tn_type),
		    op_name(op));
		break;
	}
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

	const mod_t *mp = &modtab[op];

	type_t *ltp = ln->tn_type;
	tspec_t lt = ltp->t_tspec;

	type_t *rtp = mp->m_binary ? rn->tn_type : NULL;
	tspec_t rt = mp->m_binary ? rtp->t_tspec : NO_TSPEC;

	if (Tflag && !typeok_scalar_strict_bool(op, mp, arg, ln, rn))
		return false;
	if (!typeok_scalar(op, mp, ltp, lt, rtp, rt))
		return false;

	if (!typeok_op(op, arg, ln, ltp, lt, rn, rtp, rt))
		return false;

	typeok_enum(op, mp, arg, ln, ltp, rn, rtp);
	return true;
}

/* In traditional C, keep unsigned and promote FLOAT to DOUBLE. */
static tspec_t
promote_trad(tspec_t t)
{

	if (t == UCHAR || t == USHORT)
		return UINT;
	if (t == CHAR || t == SCHAR || t == SHORT)
		return INT;
	if (t == FLOAT)
		return DOUBLE;
	if (t == ENUM)
		return INT;
	return t;
}

/*
 * C99 6.3.1.1p2 requires for types with lower rank than int that "If an int
 * can represent all the values of the original type, the value is converted
 * to an int; otherwise it is converted to an unsigned int", and that "All
 * other types are unchanged by the integer promotions".
 */
static tspec_t
promote_c90(const tnode_t *tn, tspec_t t, bool farg)
{
	if (tn->tn_type->t_bitfield) {
		unsigned int width = tn->tn_type->t_bit_field_width;
		unsigned int int_width = size_in_bits(INT);
		// XXX: What about _Bool bit-fields, since C99?
		if (width < int_width)
			return INT;
		if (width == int_width)
			return is_uinteger(t) ? UINT : INT;
		return t;
	}

	if (t == CHAR || t == SCHAR)
		return INT;
	if (t == UCHAR)
		return size_in_bits(CHAR) < size_in_bits(INT) ? INT : UINT;
	if (t == SHORT)
		return INT;
	if (t == USHORT)
		return size_in_bits(SHORT) < size_in_bits(INT) ? INT : UINT;
	if (t == ENUM)
		return INT;
	if (farg && t == FLOAT)
		return DOUBLE;
	return t;
}

/*
 * Performs the "integer promotions" (C99 6.3.1.1p2), which convert small
 * integer types to either int or unsigned int.
 *
 * If allow_c90 is unset or the operand is a function argument with no type
 * information (no prototype or variable # of args), converts float to double.
 */
tnode_t *
promote(op_t op, bool farg, tnode_t *tn)
{

	tspec_t ot = tn->tn_type->t_tspec;
	if (!is_arithmetic(ot))
		return tn;

	tspec_t nt = allow_c90 ? promote_c90(tn, ot, farg) : promote_trad(ot);
	if (nt == ot)
		return tn;

	type_t *ntp = expr_dup_type(tn->tn_type);
	ntp->t_tspec = nt;
	/*
	 * Keep t_is_enum even though t_tspec gets converted from
	 * ENUM to INT, so we are later able to check compatibility
	 * of enum types.
	 */
	return convert(op, 0, ntp, tn);
}

static void
convert_integer_from_floating(op_t op, const type_t *tp, const tnode_t *tn)
{

	if (op == CVT)
		/* cast from floating point '%s' to integer '%s' */
		query_message(2, type_name(tn->tn_type), type_name(tp));
	else
		/* implicit conversion from floating point '%s' to ... */
		query_message(1, type_name(tn->tn_type), type_name(tp));
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
	    portable_rank_cmp(nt, ot) != 0) {
		/* representation and/or width change */
		if (!is_integer(ot))
			return true;
		/*
		 * XXX: Investigate whether this rule makes sense; see
		 * tests/usr.bin/xlint/lint1/platform_long.c.
		 */
		return portable_rank_cmp(ot, INT) > 0;
	}

	if (!hflag)
		return false;

	/*
	 * If the types differ only in sign and the argument has the same
	 * representation in both types, print no warning.
	 */
	if (ptn->tn_op == CON && is_integer(nt) &&
	    signed_type(nt) == signed_type(ot) &&
	    !msb(ptn->tn_val.u.integer, ot))
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
	tnode_t *ptn = promote(NOOP, true, tn);
	ot = ptn->tn_type->t_tspec;

	if (should_warn_about_prototype_conversion(nt, ot, ptn)) {
		/* argument %d is converted from '%s' to '%s' ... */
		warning(259, arg, type_name(tn->tn_type), type_name(tp));
	}
}

/*
 * When converting a large integer type to a small integer type, in some
 * cases the value of the actual expression is further restricted than the
 * type bounds, such as in (expr & 0xFF) or (expr % 100) or (expr >> 24).
 */
static bool
can_represent(const type_t *tp, const tnode_t *tn)
{

	debug_step("%s: type '%s'", __func__, type_name(tp));
	debug_node(tn);

	uint64_t nmask = value_bits(width_in_bits(tp));
	if (!is_uinteger(tp->t_tspec))
		nmask >>= 1;

	integer_constraints c = ic_expr(tn);
	if ((~c.bclr & ~nmask) == 0)
		return true;

	integer_constraints tpc = ic_any(tp);
	if (is_uinteger(tp->t_tspec)
	    ? tpc.umin <= c.umin && tpc.umax >= c.umax
	    : tpc.smin <= c.smin && tpc.smax >= c.smax)
		return true;

	return false;
}

static void
convert_integer_from_integer(op_t op, int arg, tspec_t nt, tspec_t ot,
			     type_t *tp, tnode_t *tn)
{

	if (tn->tn_op == CON)
		return;

	if (op == CVT)
		return;

	if (Pflag && pflag && aflag > 0 &&
	    portable_rank_cmp(nt, ot) > 0 &&
	    is_uinteger(nt) != is_uinteger(ot)) {
		if (op == FARG) {
			/* conversion to '%s' may sign-extend ... */
			warning(297, type_name(tp), arg);
		} else {
			/* conversion to '%s' may sign-extend ... */
			warning(131, type_name(tp));
		}
	}

	if (Pflag && portable_rank_cmp(nt, ot) > 0 &&
	    (tn->tn_op == PLUS || tn->tn_op == MINUS || tn->tn_op == MULT ||
	     tn->tn_op == SHL)) {
		/* suggest cast from '%s' to '%s' on op '%s' to ... */
		warning(324, type_name(gettyp(ot)), type_name(tp),
		    op_name(tn->tn_op));
	}

	if (aflag > 0 &&
	    portable_rank_cmp(nt, ot) < 0 &&
	    (portable_rank_cmp(ot, LONG) >= 0 || aflag > 1) &&
	     // XXX: The portable_rank_cmp above aims at portable mode,
	     // independent of the current platform, while can_represent acts
	     // on the actual type sizes from the current platform.  This mix
	     // is inconsistent, but anything else would make the exact
	     // conditions too complicated to grasp.
	    !can_represent(tp, tn)) {
		if (op == FARG) {
			/* conversion from '%s' to '%s' may lose ... */
			warning(298,
			    type_name(tn->tn_type), type_name(tp), arg);
		} else {
			/* conversion from '%s' to '%s' may lose accuracy */
			warning(132,
			    type_name(tn->tn_type), type_name(tp));
		}
	}

	if (any_query_enabled && is_uinteger(nt) != is_uinteger(ot))
		/* implicit conversion changes sign from '%s' to '%s' */
		query_message(3, type_name(tn->tn_type), type_name(tp));
}

static void
convert_integer_from_pointer(op_t op, tspec_t nt, type_t *tp, tnode_t *tn)
{

	if (tn->tn_op == CON)
		return;
	if (op != CVT)
		return;		/* We already got an error. */
	if (portable_rank_cmp(nt, PTR) >= 0)
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
struct_starts_with(const type_t *struct_tp, const type_t *member_tp)
{

	return struct_tp->t_sou->sou_first_member != NULL &&
	       types_compatible(struct_tp->t_sou->sou_first_member->s_type,
		   member_tp, true, false, NULL);
}

static bool
is_byte_array(const type_t *tp)
{

	return tp->t_tspec == ARRAY &&
	       (tp->t_subt->t_tspec == CHAR || tp->t_subt->t_tspec == UCHAR);
}

static bool
union_contains(const type_t *utp, const type_t *mtp)
{
	for (const sym_t *mem = utp->t_sou->sou_first_member;
	     mem != NULL; mem = mem->s_next) {
		if (types_compatible(mem->s_type, mtp, true, false, NULL))
			return true;
	}
	return false;
}

static bool
should_warn_about_pointer_cast(const type_t *nstp, tspec_t nst,
			       const type_t *ostp, tspec_t ost)
{

	while (nst == ARRAY)
		nstp = nstp->t_subt, nst = nstp->t_tspec;
	while (ost == ARRAY)
		ostp = ostp->t_subt, ost = ostp->t_tspec;

	if (nst == STRUCT && ost == STRUCT &&
	    (struct_starts_with(nstp, ostp) ||
	     struct_starts_with(ostp, nstp)))
		return false;

	if (is_incomplete(nstp) || is_incomplete(ostp))
		return false;

	if (nst == CHAR || nst == UCHAR)
		return false;	/* for the sake of traditional C code */
	if (ost == CHAR || ost == UCHAR)
		return false;	/* for the sake of traditional C code */

	/* Allow cast between pointers to sockaddr variants. */
	if (nst == STRUCT && ost == STRUCT) {
		const sym_t *nmem = nstp->t_sou->sou_first_member;
		const sym_t *omem = ostp->t_sou->sou_first_member;
		while (nmem != NULL && omem != NULL &&
		       types_compatible(nmem->s_type, omem->s_type,
			   true, false, NULL))
			nmem = nmem->s_next, omem = omem->s_next;
		if (nmem != NULL && is_byte_array(nmem->s_type))
			return false;
		if (omem != NULL && is_byte_array(omem->s_type))
			return false;
		if (nmem == NULL && omem == NULL)
			return false;
	}

	if (nst == UNION || ost == UNION) {
		const type_t *union_tp = nst == UNION ? nstp : ostp;
		const type_t *other_tp = nst == UNION ? ostp : nstp;
		if (union_contains(union_tp, other_tp))
			return false;
	}

	if (is_struct_or_union(nst) && is_struct_or_union(ost))
		return nstp->t_sou != ostp->t_sou;

	enum rank_kind rk1 = type_properties(nst)->tt_rank_kind;
	enum rank_kind rk2 = type_properties(ost)->tt_rank_kind;
	if (rk1 != rk2 || rk1 == RK_NONE)
		return true;

	return portable_rank_cmp(nst, ost) != 0;
}

static void
convert_pointer_from_pointer(type_t *ntp, tnode_t *tn)
{
	const type_t *nstp = ntp->t_subt;
	const type_t *otp = tn->tn_type;
	const type_t *ostp = otp->t_subt;
	tspec_t nst = nstp->t_tspec;
	tspec_t ost = ostp->t_tspec;

	if (nst == VOID || ost == VOID) {
		/* TODO: C99 behaves like C90 here. */
		if ((!allow_trad && !allow_c99) && (nst == FUNC || ost == FUNC)) {
			const char *nts, *ots;
			/* null pointers are already handled in convert() */
			*(nst == FUNC ? &nts : &ots) = "function pointer";
			*(nst == VOID ? &nts : &ots) = "'void *'";
			/* ANSI C forbids conversion of %s to %s */
			warning(303, ots, nts);
		}
		return;
	}
	if (nst == FUNC && ost == FUNC)
		return;
	if (nst == FUNC || ost == FUNC) {
		/* converting '%s' to '%s' is questionable */
		warning(229, type_name(otp), type_name(ntp));
		return;
	}

	if (hflag && alignment_in_bits(nstp) > alignment_in_bits(ostp) &&
	    ost != CHAR && ost != UCHAR &&
	    !is_incomplete(ostp) &&
	    !(nst == UNION && union_contains(nstp, ostp))) {
		/* converting '%s' to '%s' increases alignment ... */
		warning(135, type_name(otp), type_name(ntp),
		    alignment_in_bits(ostp) / CHAR_SIZE,
		    alignment_in_bits(nstp) / CHAR_SIZE);
	}

	if (cflag && should_warn_about_pointer_cast(nstp, nst, ostp, ost)) {
		/* pointer cast from '%s' to '%s' may be troublesome */
		warning(247, type_name(otp), type_name(ntp));
	}
}

/*
 * Insert a conversion operator, which converts the type of the node
 * to another given type.
 *
 * Possible values for 'op':
 *	CVT	a cast-expression
 *	binary	integer promotion for one of the operands, or a usual
 *		arithmetic conversion
 *	binary	plain or compound assignments to bit-fields
 *	FARG	'arg' is the number of the argument (used for warnings)
 *	NOOP	several other implicit conversions
 *	...
 */
tnode_t *
convert(op_t op, int arg, type_t *tp, tnode_t *tn)
{
	tspec_t nt = tp->t_tspec;
	tspec_t ot = tn->tn_type->t_tspec;

	if (allow_trad && allow_c90 && op == FARG)
		check_prototype_conversion(arg, nt, ot, tp, tn);

	if (nt == BOOL) {
		/* No further checks. */

	} else if (is_integer(nt)) {
		if (ot == BOOL) {
			/* No further checks. */
		} else if (is_integer(ot))
			convert_integer_from_integer(op, arg, nt, ot, tp, tn);
		else if (is_floating(ot))
			convert_integer_from_floating(op, tp, tn);
		else if (ot == PTR)
			convert_integer_from_pointer(op, nt, tp, tn);

	} else if (is_floating(nt)) {
		/* No further checks. */

	} else if (nt == PTR) {
		if (is_null_pointer(tn)) {
			/* a null pointer may be assigned to any pointer. */
		} else if (ot == PTR && op == CVT)
			convert_pointer_from_pointer(tp, tn);
	}

	tnode_t *ntn = expr_alloc_tnode();
	ntn->tn_op = CVT;
	ntn->tn_type = tp;
	ntn->tn_cast = op == CVT;
	ntn->tn_sys |= tn->tn_sys;
	ntn->tn_right = NULL;
	if (tn->tn_op != CON || nt == VOID) {
		ntn->tn_left = tn;
	} else {
		ntn->tn_op = CON;
		convert_constant(op, arg, ntn->tn_type, &ntn->tn_val,
		    &tn->tn_val);
	}

	return ntn;
}

static void
convert_constant_floating(op_t op, int arg, tspec_t ot, const type_t *tp,
			  tspec_t nt, val_t *v, val_t *nv)
{
	long double max = 0.0, min = 0.0;

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
	case LLONG:
		max = LLONG_MAX;	min = LLONG_MIN;	break;
	case ULLONG:
		max = ULLONG_MAX;	min = 0;		break;
	case FLOAT:
	case FCOMPLEX:
		max = FLT_MAX;		min = -FLT_MAX;		break;
	case DOUBLE:
	case DCOMPLEX:
		max = DBL_MAX;		min = -DBL_MAX;		break;
	case PTR:
		/* Already got an error because of float --> ptr */
	case LDOUBLE:
	case LCOMPLEX:
		/* LINTED 248 */
		max = LDBL_MAX;		min = -max;		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}
	if (v->u.floating > max || v->u.floating < min) {
		lint_assert(nt != LDOUBLE);
		if (op == FARG) {
			/* conversion of '%s' to '%s' is out of range, ... */
			warning(295,
			    type_name(gettyp(ot)), type_name(tp), arg);
		} else {
			/* conversion of '%s' to '%s' is out of range */
			warning(119, type_name(gettyp(ot)), type_name(tp));
		}
		v->u.floating = v->u.floating > 0 ? max : min;
	}

	if (nt == FLOAT || nt == FCOMPLEX)
		nv->u.floating = (float)v->u.floating;
	else if (nt == DOUBLE || nt == DCOMPLEX)
		nv->u.floating = (double)v->u.floating;
	else if (nt == LDOUBLE || nt == LCOMPLEX)
		nv->u.floating = v->u.floating;
	else
		nv->u.integer = (int64_t)v->u.floating;
}

static bool
convert_constant_to_floating(tspec_t nt, val_t *nv,
			     tspec_t ot, const val_t *v)
{
	if (nt == FLOAT) {
		nv->u.floating = (ot == PTR || is_uinteger(ot)) ?
		    (float)(uint64_t)v->u.integer : (float)v->u.integer;
	} else if (nt == DOUBLE) {
		nv->u.floating = (ot == PTR || is_uinteger(ot)) ?
		    (double)(uint64_t)v->u.integer : (double)v->u.integer;
	} else if (nt == LDOUBLE) {
		nv->u.floating = (ot == PTR || is_uinteger(ot))
		    ? (long double)(uint64_t)v->u.integer
		    : (long double)v->u.integer;
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
	if (nsz < osz && (v->u.integer & xmask) != 0) {
		/* constant truncated by conversion, op '%s' */
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
	    (nv->u.integer & bit((unsigned int)(osz - 1))) != 0 &&
	    (nv->u.integer & xmask) != xmask) {
		/* extra bits set to 0 in conversion of '%s' to '%s', ... */
		warning(309, type_name(gettyp(ot)),
		    type_name(tp), op_name(op));
	} else if (nsz < osz &&
		   (v->u.integer & xmask) != xmask &&
		   (v->u.integer & xmask) != 0) {
		/* constant truncated by conversion, op '%s' */
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
 * Loss of significant bit(s). All truncated bits of unsigned types or all
 * truncated bits plus the msb of the target for signed types are considered
 * to be significant bits. Loss of significant bits means that at least one
 * of the bits was set in an unsigned type or that at least one but not all
 * of the bits was set in a signed type. Loss of significant bits means that
 * it is not possible, also not with necessary casts, to convert back to the
 * original type. An example for a necessary cast is:
 *	char c;	int	i; c = 128;
 *	i = c;			** yields -128 **
 *	i = (unsigned char)c;	** yields 128 **
 */
static void
warn_constant_check_range_truncated(op_t op, int arg, const type_t *tp,
				    tspec_t ot)
{
	if (op == ASSIGN && tp->t_bitfield)
		/* precision lost in bit-field assignment */
		warning(166);
	else if (op == ASSIGN)
		/* constant truncated by assignment */
		warning(165);
	else if (op == INIT && tp->t_bitfield)
		/* bit-field initializer does not fit */
		warning(180);
	else if (op == INIT)
		/* initializer does not fit */
		warning(178);
	else if (op == CASE)
		/* case label affected by conversion */
		warning(196);
	else if (op == FARG)
		/* conversion of '%s' to '%s' is out of range, arg #%d */
		warning(295, type_name(gettyp(ot)), type_name(tp), arg);
	else
		/* conversion of '%s' to '%s' is out of range */
		warning(119, type_name(gettyp(ot)), type_name(tp));
}

static void
warn_constant_check_range_loss(op_t op, int arg, const type_t *tp,
				  tspec_t ot)
{
	if (op == ASSIGN && tp->t_bitfield)
		/* precision lost in bit-field assignment */
		warning(166);
	else if (op == INIT && tp->t_bitfield)
		/* bit-field initializer out of range */
		warning(11);
	else if (op == CASE)
		/* case label affected by conversion */
		warning(196);
	else if (op == FARG)
		/* conversion of '%s' to '%s' is out of range, arg #%d */
		warning(295, type_name(gettyp(ot)), type_name(tp), arg);
	else
		/* conversion of '%s' to '%s' is out of range */
		warning(119, type_name(gettyp(ot)), type_name(tp));
}

static void
convert_constant_check_range(tspec_t ot, const type_t *tp, tspec_t nt,
			     op_t op, int arg, const val_t *v, val_t *nv)
{
	unsigned int obitsz, nbitsz;
	uint64_t xmask, xmsk1;

	obitsz = size_in_bits(ot);
	nbitsz = tp->t_bitfield ? tp->t_bit_field_width : size_in_bits(nt);
	xmask = value_bits(nbitsz) ^ value_bits(obitsz);
	xmsk1 = value_bits(nbitsz) ^ value_bits(obitsz - 1);
	/*
	 * For bitwise operations we are not interested in the arithmetic
	 * value, but in the bits itself.
	 */
	if (op == ORASS || op == BITOR || op == BITXOR) {
		convert_constant_check_range_bitor(
		    nbitsz, obitsz, v, xmask, op);
	} else if (op == ANDASS || op == BITAND) {
		convert_constant_check_range_bitand(
		    nbitsz, obitsz, xmask, nv, ot, v, tp, op);
	} else if ((nt != PTR && is_uinteger(nt)) &&
		   (ot != PTR && !is_uinteger(ot)) &&
		   v->u.integer < 0)
		convert_constant_check_range_signed(op, arg);
	else if (nv->u.integer != v->u.integer && nbitsz <= obitsz &&
		 (v->u.integer & xmask) != 0 &&
		 (is_uinteger(ot) || (v->u.integer & xmsk1) != xmsk1))
		warn_constant_check_range_truncated(op, arg, tp, ot);
	else if (nv->u.integer != v->u.integer)
		warn_constant_check_range_loss(op, arg, tp, ot);
}

/*
 * Converts a typed constant to a constant of another type.
 *
 * op		operator which requires conversion
 * arg		if op is FARG, # of argument
 * tp		type to which to convert the constant
 * nv		new constant
 * v		old constant
 */
void
convert_constant(op_t op, int arg, const type_t *tp, val_t *nv, val_t *v)
{
	/*
	 * TODO: make 'v' const; the name of this function does not suggest
	 *  that it modifies 'v'.
	 */
	tspec_t ot = v->v_tspec;
	tspec_t nt = nv->v_tspec = tp->t_tspec;
	bool range_check = false;

	if (nt == BOOL) {	/* C99 6.3.1.2 */
		nv->v_unsigned_since_c90 = false;
		nv->u.integer = is_nonzero_val(v) ? 1 : 0;
		return;
	}

	if (ot == FLOAT || ot == DOUBLE || ot == LDOUBLE)
		convert_constant_floating(op, arg, ot, tp, nt, v, nv);
	else if (!convert_constant_to_floating(nt, nv, ot, v)) {
		range_check = true;	/* Check for lost precision. */
		nv->u.integer = v->u.integer;
	}

	if (allow_trad && allow_c90 && v->v_unsigned_since_c90 &&
	    (is_floating(nt) || (
		(is_integer(nt) && !is_uinteger(nt) &&
		    portable_rank_cmp(nt, ot) > 0)))) {
		/* ANSI C treats constant as unsigned */
		warning(157);
		v->v_unsigned_since_c90 = false;
	}

	if (is_integer(nt)) {
		nv->u.integer = convert_integer(nv->u.integer, nt,
		    tp->t_bitfield ? tp->t_bit_field_width : size_in_bits(nt));
	}

	if (range_check && op != CVT)
		convert_constant_check_range(ot, tp, nt, op, arg, v, nv);
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
	debug_step("build_sizeof '%s' = %u", type_name(tp), size_in_bytes);
	return tn;
}

/*
 * Create a constant node for offsetof.
 */
/* ARGSUSED */ /* FIXME: See implementation comments. */
tnode_t *
build_offsetof(const type_t *tp, const sym_t *sym)
{

	if (!is_struct_or_union(tp->t_tspec))
		/* unacceptable operand of '%s' */
		error(111, "offsetof");

	/* FIXME: Don't wrongly use the size of the whole type, use sym. */
	unsigned int offset_in_bytes = type_size_in_bits(tp) / CHAR_SIZE;
	tnode_t *tn = build_integer_constant(SIZEOF_TSPEC, offset_in_bytes);
	tn->tn_system_dependent = true;
	return tn;
}

unsigned int
type_size_in_bits(const type_t *tp)
{

	unsigned int elem = 1;
	bool flex = false;
	lint_assert(tp != NULL);
	while (tp->t_tspec == ARRAY) {
		flex = true;	/* allow c99 flex arrays [] [0] */
		elem *= tp->t_dim;
		tp = tp->t_subt;
	}
	if (elem == 0 && !flex) {
		/* cannot take size/alignment of incomplete type */
		error(143);
		elem = 1;
	}

	unsigned int elsz;
	switch (tp->t_tspec) {
	case VOID:
		/* cannot take size/alignment of void */
		error(146);
		elsz = 1;
		break;
	case FUNC:
		/* cannot take size/alignment of function type '%s' */
		error(144, type_name(tp));
		elsz = 1;
		break;
	case STRUCT:
	case UNION:
		if (is_incomplete(tp)) {
			/* cannot take size/alignment of incomplete type */
			error(143);
			elsz = 1;
		} else {
			elsz = tp->t_sou->sou_size_in_bits;
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
		elsz = size_in_bits(tp->t_tspec);
		lint_assert(elsz > 0);
		break;
	}

	return elem * elsz;
}

/* C11 6.5.3.4, GCC */
tnode_t *
build_alignof(const type_t *tp)
{
	if (tp->t_tspec == FUNC) {
		/* cannot take size/alignment of function type '%s' */
		error(144, type_name(tp));
		return NULL;
	}
	if (tp->t_tspec == VOID) {
		/* cannot take size/alignment of void */
		error(146);
		return NULL;
	}
	if (is_incomplete(tp)) {
		/* cannot take size/alignment of incomplete type */
		error(143);
		return NULL;
	}
	if (tp->t_bitfield) {
		/* cannot take size/alignment of bit-field */
		error(145);
		return NULL;
	}
	return build_integer_constant(SIZEOF_TSPEC,
	    (int64_t)alignment_in_bits(tp) / CHAR_SIZE);
}

static tnode_t *
cast_to_union(tnode_t *otn, type_t *ntp)
{

	if (!allow_gcc) {
		/* union cast is a GCC extension */
		error(328);
		return NULL;
	}

	for (const sym_t *m = ntp->t_sou->sou_first_member;
	    m != NULL; m = m->s_next) {
		if (types_compatible(m->s_type, otn->tn_type,
		    false, false, NULL)) {
			tnode_t *ntn = expr_alloc_tnode();
			ntn->tn_op = CVT;
			ntn->tn_type = ntp;
			ntn->tn_cast = true;
			ntn->tn_left = otn;
			ntn->tn_right = NULL;
			return ntn;
		}
	}

	/* type '%s' is not a member of '%s' */
	error(329, type_name(otn->tn_type), type_name(ntp));
	return NULL;
}

/*
 * Type casts.
 */
tnode_t *
cast(tnode_t *tn, type_t *tp)
{

	if (tn == NULL)
		return NULL;

	tn = cconv(tn);

	lint_assert(tp != NULL);
	tspec_t nt = tp->t_tspec;
	tspec_t ot = tn->tn_type->t_tspec;

	if (nt == VOID) {
		/*
		 * C90 6.3.4, C99 6.5.4p2 and C11 6.5.4p2 allow any type to
		 * be cast to void.  The only other allowed casts are from a
		 * scalar type to a scalar type.
		 */
	} else if (nt == UNION)
		return cast_to_union(tn, tp);
	else if (nt == STRUCT || nt == ARRAY || nt == FUNC) {
		/* Casting to a struct is an undocumented GCC extension. */
		if (!(allow_gcc && nt == STRUCT))
			goto invalid_cast;
	} else if (is_struct_or_union(ot))
		goto invalid_cast;
	else if (ot == VOID) {
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

	if (any_query_enabled && types_compatible(tp, tn->tn_type,
	    false, false, NULL)) {
		/* no-op cast from '%s' to '%s' */
		query_message(6, type_name(tn->tn_type), type_name(tp));
	}

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
	/*
	 * If there was a serious error in the expression for the argument,
	 * create a dummy argument so the positions of the remaining arguments
	 * will not change.
	 */
	if (arg == NULL)
		arg = build_integer_constant(INT, 0);

	return build_op(PUSH, arg->tn_sys, arg->tn_type, arg, args);
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
	tnode_t *ln = xcalloc(1, sizeof(*ln));
	ln->tn_type = expr_unqualified_type(tp);
	ln->tn_lvalue = true;
	if (typeok(FARG, n, ln, tn)) {
		bool dowarn;
		if (!types_compatible(tp, tn->tn_type,
		    true, false, (dowarn = false, &dowarn)) || dowarn)
			tn = convert(FARG, n, tp, tn);
	}
	free(ln);
	return tn;
}

/*
 * Check types of all function arguments and insert conversions,
 * if necessary.
 */
static tnode_t *
check_function_arguments(type_t *ftp, tnode_t *args)
{
	/* get # of parameters in the prototype */
	int npar = 0;
	for (sym_t *asym = ftp->t_args; asym != NULL; asym = asym->s_next)
		npar++;

	/* get # of arguments in the function call */
	int narg = 0;
	for (tnode_t *arg = args; arg != NULL; arg = arg->tn_right)
		narg++;

	sym_t *asym = ftp->t_args;
	if (ftp->t_proto && npar != narg && !(ftp->t_vararg && npar < narg)) {
		/* argument mismatch: %d %s passed, %d expected */
		error(150, narg, narg > 1 ? "arguments" : "argument", npar);
		asym = NULL;
	}

	for (int n = 1; n <= narg; n++) {

		/*
		 * The rightmost argument is at the top of the argument
		 * subtree.
		 */
		tnode_t *arg = args;
		for (int i = narg; i > n; i--, arg = arg->tn_right)
			continue;

		/* some things which are always not allowed */
		tspec_t at = arg->tn_left->tn_type->t_tspec;
		if (at == VOID) {
			/* void expressions may not be arguments, arg #%d */
			error(151, n);
			return NULL;
		} else if (is_struct_or_union(at) &&
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
		} else
			arg->tn_left = promote(NOOP, true, arg->tn_left);
		arg->tn_type = arg->tn_left->tn_type;

		if (asym != NULL)
			asym = asym->s_next;
	}

	return args;
}

/*
 * Create the node for a function call. Also check types of
 * function arguments and insert conversions, if necessary.
 */
tnode_t *
build_function_call(tnode_t *func, bool sys, tnode_t *args)
{

	if (func == NULL)
		return NULL;

	op_t fcop = func->tn_op == NAME && func->tn_type->t_tspec == FUNC
	    ? CALL : ICALL;

	check_ctype_function_call(func, args);

	/* Turn the function name into a pointer to the function. */
	func = cconv(func);

	if (func->tn_type->t_tspec != PTR ||
	    func->tn_type->t_subt->t_tspec != FUNC) {
		/* cannot call '%s', must be a function */
		error(149, type_name(func->tn_type));
		return NULL;
	}

	args = check_function_arguments(func->tn_type->t_subt, args);

	return build_op(fcop, sys, func->tn_type->t_subt->t_subt, func, args);
}

/*
 * Return the value of an integral constant expression.
 * If the expression is not constant or its type is not an integer
 * type, an error message is printed.
 */
val_t *
integer_constant(tnode_t *tn, bool required)
{

	if (tn != NULL)
		tn = cconv(tn);
	if (tn != NULL)
		tn = promote(NOOP, false, tn);

	val_t *v = xcalloc(1, sizeof(*v));

	if (tn == NULL) {
		lint_assert(seen_error);
		debug_step("constant node is null; returning 1 instead");
		v->v_tspec = INT;
		v->u.integer = 1;
		return v;
	}

	v->v_tspec = tn->tn_type->t_tspec;

	if (tn->tn_op == CON) {
		lint_assert(tn->tn_type->t_tspec == tn->tn_val.v_tspec);
		if (is_integer(tn->tn_val.v_tspec)) {
			v->v_unsigned_since_c90 =
			    tn->tn_val.v_unsigned_since_c90;
			v->u.integer = tn->tn_val.u.integer;
			return v;
		}
		v->u.integer = (int64_t)tn->tn_val.u.floating;
	} else {
		v->u.integer = 1;
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
	       tn->tn_op == CON && tn->tn_val.u.integer == 0;
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
expr(tnode_t *tn, bool vctx, bool cond, bool dofreeblk, bool is_do_while)
{

	if (tn == NULL) {	/* in case of errors */
		expr_free_all();
		return;
	}

	/* expr() is also called in global initializations */
	if (dcs->d_kind != DLK_EXTERN && !is_do_while)
		check_statement_reachable();

	check_expr_misc(tn, vctx, cond, !cond, false, false, false);
	if (tn->tn_op == ASSIGN && !tn->tn_parenthesized) {
		if (hflag && cond)
			/* assignment in conditional context */
			warning(159);
	} else if (tn->tn_op == CON) {
		if (hflag && cond && !suppress_constcond &&
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
		if (tn->tn_op != COMMA && !vctx && !cond)
			check_null_effect(tn);
	}
	debug_node(tn);

	/* free the tree memory */
	if (dofreeblk)
		expr_free_all();
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
	const tnode_t *ln = tn->tn_left;
	const tnode_t *rn = tn->tn_right;

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
	if (is_incomplete(ln->tn_left->tn_type) && rn->tn_val.u.integer >= 0)
		return;

	/* Get the size of one array element */
	int elsz = length_in_bits(ln->tn_type->t_subt, NULL);
	if (elsz == 0)
		return;
	elsz /= CHAR_SIZE;

	/* Change the unit of the index from bytes to element size. */
	int64_t con = is_uinteger(rn->tn_type->t_tspec)
	    ? (int64_t)((uint64_t)rn->tn_val.u.integer / elsz)
	    : rn->tn_val.u.integer / elsz;

	int dim = ln->tn_left->tn_type->t_dim + (amper ? 1 : 0);

	if (!is_uinteger(rn->tn_type->t_tspec) && con < 0) {
		/* array subscript cannot be negative: %ld */
		warning(167, (long)con);
	} else if (dim > 0 && (uint64_t)con >= (uint64_t)dim) {
		/* array subscript cannot be > %d: %ld */
		warning(168, dim - 1, (long)con);
	}
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

/*
 * If there is an asm statement in one of the compound statements around,
 * there may be other side effects, so don't warn.
 */
static bool
is_asm_around(void)
{
	for (decl_level *dl = dcs; dl != NULL; dl = dl->d_enclosing)
		if (dl->d_asm)
			return true;
	return false;
}

static void
check_expr_side_effect(const tnode_t *ln, bool szof)
{

	/* XXX: Taking warn_about_unreachable into account here feels wrong. */
	if (ln->tn_op == NAME && (reached || !warn_about_unreachable)) {
		scl_t sc = ln->tn_sym->s_scl;
		if (sc != EXTERN && sc != STATIC &&
		    !ln->tn_sym->s_set && !szof && !is_asm_around()) {
			/* '%s' may be used before set */
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
		bool szof, bool vctx, bool cond, bool retval_discarded)
{
	lint_assert(ln->tn_op == ADDR);
	lint_assert(ln->tn_left->tn_op == NAME);
	if (!szof && !is_compiler_builtin(ln->tn_left->tn_sym->s_name))
		outcall(tn, vctx || cond, retval_discarded);
}

static bool
check_expr_op(const tnode_t *tn, op_t op, const tnode_t *ln,
	      bool szof, bool fcall, bool vctx, bool cond,
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
		check_expr_call(tn, ln, szof, vctx, cond, retval_discarded);
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
	default:
		break;
	}
	return true;
}

/*
 *	vctx			???
 *	cond			whether the expression is a condition that
 *				will be compared with 0
 *	eqwarn			whether the operator '==' might be a
 *				misspelled '='
 *	fcall			whether the expression is a function call
 *	retval_discarded	whether the return value of a function call
 *				is discarded; such calls will be analyzed by
 *				lint2 in messages 4, 8 and 9
 *	szof			whether the expression is part of a sizeof
 *				expression, which means that its value is
 *				discarded since only the type is relevant
 */
void
check_expr_misc(const tnode_t *tn, bool vctx, bool cond,
		bool eqwarn, bool fcall, bool retval_discarded, bool szof)
{

	if (tn == NULL)
		return;

	tnode_t *ln = tn->tn_left;
	tnode_t *rn = tn->tn_right;
	op_t op = tn->tn_op;
	const mod_t *mp = &modtab[op];

	if (!check_expr_op(tn, op, ln,
	    szof, fcall, vctx, cond, retval_discarded, eqwarn))
		return;

	bool cvctx = mp->m_value_context;
	bool ccond = mp->m_compares_with_zero;
	bool eq = mp->m_warn_if_operand_eq &&
	     !ln->tn_parenthesized &&
	     rn != NULL && !rn->tn_parenthesized;

	/*
	 * values of operands of ':' are not used if the type of at least
	 * one of the operands (for gcc compatibility) is void
	 * XXX test/value context of QUEST should probably be used as
	 * context for both operands of COLON
	 */
	if (op == COLON && tn->tn_type->t_tspec == VOID)
		cvctx = ccond = false;
	bool discard = op == CVT && tn->tn_type->t_tspec == VOID;
	check_expr_misc(ln, cvctx, ccond, eq, op == CALL, discard, szof);

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
		check_expr_misc(rn, cvctx, ccond, eq, false, false, szof);
		break;
	case COMMA:
		check_expr_misc(rn, vctx, cond, false, false, false, szof);
		break;
	default:
		if (mp->m_binary)
			check_expr_misc(rn, true, false, eq, false, false,
			    szof);
		break;
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
	tspec_t t, ot;

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
			offs1 = (ptrdiff_t)tn->tn_left->tn_val.u.integer;
			if (!constant_addr(tn->tn_right, &sym, &offs2))
				return false;
		} else if (tn->tn_right->tn_op == CON) {
			offs2 = (ptrdiff_t)tn->tn_right->tn_val.u.integer;
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
			 * If this were the front end of a compiler, we
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

	if (s1->st_char != s2->st_char) {
		/* cannot concatenate wide and regular string literals */
		error(292);
		return s1;
	}

	size_t len1 = s1->st_len;
	size_t len2 = s2->st_len;
	size_t chsize = s1->st_char ? sizeof(char) : sizeof(wchar_t);
	size_t size1 = len1 * chsize;
	size_t size2 = (len2 + 1) * chsize;
	s1->st_mem = xrealloc(s1->st_mem, size1 + size2);
	memcpy((char *)s1->st_mem + size1, s2->st_mem, size2);
	free(s2->st_mem);

	s1->st_len = len1 + len2;
	free(s2);

	return s1;
}


typedef struct stmt_expr {
	memory_pool se_mem;
	sym_t *se_sym;
	struct stmt_expr *se_enclosing;
} stmt_expr;

static stmt_expr *stmt_exprs;

void
begin_statement_expr(void)
{
	debug_enter();

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
	stmt_exprs->se_sym = tn != NULL
	    ? mktempsym(block_dup_type(tn->tn_type))
	    : NULL;		/* after a syntax error */
	mem_block_level++;
	block_level++;
	/* '({ ... })' is a GCC extension */
	gnuism(320);
}

tnode_t *
end_statement_expr(void)
{
	tnode_t *tn;

	stmt_expr *se = stmt_exprs;
	if (se->se_sym == NULL) {
		tn = NULL;	/* after a syntax error */
		goto end;
	}

	tn = build_name(se->se_sym, false);
	(void)expr_save_memory();	/* leak */
	expr_restore_memory(se->se_mem);
	stmt_exprs = se->se_enclosing;
	free(se);

end:
	debug_leave();
	return tn;
}

bool
in_statement_expr(void)
{
	return stmt_exprs != NULL;
}
