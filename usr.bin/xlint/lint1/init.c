/*	$NetBSD: init.c,v 1.234 2022/04/30 21:38:03 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * Copyright (c) 2021 Roland Illig
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
__RCSID("$NetBSD: init.c,v 1.234 2022/04/30 21:38:03 rillig Exp $");
#endif

#include <stdlib.h>
#include <string.h>

#include "lint1.h"


/*
 * Initialization of global or local objects, like in:
 *
 *	int number = 12345;
 *	int number_with_braces = { 12345 };
 *	int array_of_unknown_size[] = { 111, 222, 333 };
 *	struct { int x, y; } point = { .y = 4, .x = 3 };
 *
 * During an initialization, the grammar parser calls these functions:
 *
 *	begin_initialization
 *		init_lbrace			for each '{'
 *		add_designator_member		for each '.member' before '='
 *		add_designator_subscript	for each '[123]' before '='
 *		init_expr			for each expression
 *		init_rbrace			for each '}'
 *	end_initialization
 *
 * Each '{' begins a new brace level, each '}' ends the current brace level.
 * Each brace level has an associated "current object", which is the starting
 * point for resolving the optional designations such as '.member[3]'.
 *
 * See also:
 *	C99 6.7.8 "Initialization"
 *	C11 6.7.9 "Initialization"
 *	d_c99_init.c for more examples
 */


typedef enum designator_kind {
	DK_STRUCT,		/* .member */
	DK_UNION,		/* .member */
	DK_ARRAY,		/* [subscript] */
	/* TODO: actually necessary? */
	DK_SCALAR		/* no textual representation, not generated
				 * by the parser */
} designator_kind;

/*
 * A single component on the path from the "current object" of a brace level
 * to the sub-object that is initialized by an expression.
 *
 * C99 6.7.8p6, 6.7.8p7
 */
typedef struct designator {
	designator_kind	dr_kind;
	const sym_t	*dr_member;	/* for DK_STRUCT and DK_UNION */
	size_t		dr_subscript;	/* for DK_ARRAY */
	bool		dr_done;
} designator;

/*
 * The path from the "current object" of a brace level to the sub-object that
 * is initialized by an expression.  Examples for designations are '.member'
 * or '.member[123].member.member[1][1]'.
 *
 * C99 6.7.8p6, 6.7.8p7
 */
typedef struct designation {
	designator	*dn_items;
	size_t		dn_len;
	size_t		dn_cap;
} designation;

/*
 * Everything that happens between a '{' and the corresponding '}', as part
 * of an initialization.
 *
 * Each brace level has a "current object".   For the outermost brace level,
 * it is the same as the object to be initialized.  Each nested '{' begins a
 * nested brace level, for the sub-object pointed to by the designator of the
 * outer brace level.
 *
 * C99 6.7.8p17
 */
typedef struct brace_level {
	/* The type of the "current object". */
	const type_t	*bl_type;

	/*
	 * The path from the "current object" to the sub-object that is
	 * initialized by the next expression.
	 *
	 * Initially, the designation is empty.  Before handling an
	 * expression, the designation is updated to point to the
	 * corresponding sub-object to be initialized.  After handling an
	 * expression, the designation is marked as done.  It is later
	 * advanced as necessary.
	 */
	designation	bl_designation;

	struct brace_level *bl_enclosing;
} brace_level;

/*
 * An ongoing initialization.
 *
 * In most cases there is only ever a single initialization at a time.  See
 * pointer_to_compound_literal in msg_171.c for a real-life counterexample.
 */
typedef struct initialization {
	/* The symbol that is to be initialized. */
	sym_t		*in_sym;

	/* The innermost brace level. */
	brace_level	*in_brace_level;

	/*
	 * The maximum subscript that has ever been seen for an array of
	 * unknown size, which can only occur at the outermost brace level.
	 */
	size_t		in_max_subscript;

	/*
	 * Is set when a structural error occurred in the initialization.
	 * If set, the rest of the initialization is still parsed, but the
	 * initialization assignments are not checked.
	 */
	bool		in_err;

	struct initialization *in_enclosing;
} initialization;


static void *
unconst_cast(const void *p)
{
	void *r;

	memcpy(&r, &p, sizeof(r));
	return r;
}

static bool
has_automatic_storage_duration(const sym_t *sym)
{

	return sym->s_scl == AUTO || sym->s_scl == REG;
}

/*
 * Test whether rn is a string literal that can initialize ltp.
 *
 * See also:
 *	C99 6.7.8p14		for plain character strings
 *	C99 6.7.8p15		for wide character strings
 */
static bool
can_init_character_array(const type_t *ltp, const tnode_t *rn)
{
	tspec_t lst, rst;

	if (!(ltp != NULL && ltp->t_tspec == ARRAY && rn->tn_op == STRING))
		return false;

	lst = ltp->t_subt->t_tspec;
	rst = rn->tn_type->t_subt->t_tspec;

	return rst == CHAR
	    ? lst == CHAR || lst == UCHAR || lst == SCHAR
	    : lst == WCHAR;
}

/* C99 6.7.8p9 */
static const sym_t *
skip_unnamed(const sym_t *m)
{

	while (m != NULL && m->s_name == unnamed)
		m = m->s_next;
	return m;
}

static const sym_t *
first_named_member(const type_t *tp)
{

	lint_assert(is_struct_or_union(tp->t_tspec));
	return skip_unnamed(tp->t_str->sou_first_member);
}

static const sym_t *
look_up_member(const type_t *tp, const char *name)
{
	const sym_t *m;

	lint_assert(is_struct_or_union(tp->t_tspec));
	for (m = tp->t_str->sou_first_member; m != NULL; m = m->s_next)
		if (strcmp(m->s_name, name) == 0)
			return m;
	return NULL;
}

/*
 * C99 6.7.8p22 says that the type of an array of unknown size becomes known
 * at the end of its initializer list.
 */
static void
update_type_of_array_of_unknown_size(sym_t *sym, size_t size)
{
	type_t *tp;

	tp = block_dup_type(sym->s_type);
	tp->t_dim = (int)size;
	tp->t_incomplete_array = false;
	sym->s_type = tp;
	debug_step("completed array type is '%s'", type_name(sym->s_type));
}


/* In traditional C, bit-fields can be initialized only by integer constants. */
static void
check_bit_field_init(const tnode_t *ln, tspec_t lt, tspec_t rt)
{

	if (!allow_c90 &&
	    is_integer(lt) &&
	    ln->tn_type->t_bitfield &&
	    !is_integer(rt)) {
		/* bit-field initialization is illegal in traditional C */
		warning(186);
	}
}

static void
check_non_constant_initializer(const tnode_t *tn, const sym_t *sym)
{
	const sym_t *unused_sym;
	ptrdiff_t unused_offs;

	if (tn == NULL || tn->tn_op == CON)
		return;

	if (constant_addr(tn, &unused_sym, &unused_offs))
		return;

	if (has_automatic_storage_duration(sym)) {
		/* non-constant initializer */
		c99ism(177);
	} else {
		/* non-constant initializer */
		error(177);
	}
}

static void
check_trad_no_auto_aggregate(const sym_t *sym)
{

	if (has_automatic_storage_duration(sym) &&
	    !is_scalar(sym->s_type->t_tspec)) {
		/* no automatic aggregate initialization in traditional C */
		warning(188);
	}
}

static void
check_init_expr(const type_t *ltp, sym_t *lsym, tnode_t *rn)
{
	tnode_t *ln;
	type_t *lutp;
	tspec_t lt, rt;
	struct memory_block *tmem;

	lutp = expr_unqualified_type(ltp);

	/* Create a temporary node for the left side. */
	ln = expr_zero_alloc(sizeof(*ln));
	ln->tn_op = NAME;
	ln->tn_type = lutp;
	ln->tn_lvalue = true;
	ln->tn_sym = lsym;

	rn = cconv(rn);

	lt = ln->tn_type->t_tspec;
	rt = rn->tn_type->t_tspec;

	debug_step("typeok '%s', '%s'",
	    type_name(ln->tn_type), type_name(rn->tn_type));
	if (!typeok(INIT, 0, ln, rn))
		return;

	/*
	 * Preserve the tree memory. This is necessary because otherwise
	 * expr() would free it.
	 */
	tmem = expr_save_memory();
	expr(rn, true, false, true, false);
	expr_restore_memory(tmem);

	check_bit_field_init(ln, lt, rt);

	/*
	 * XXX: Is it correct to do this conversion _after_ the typeok above?
	 */
	if (lt != rt || (ltp->t_bitfield && rn->tn_op == CON))
		rn = convert(INIT, 0, unconst_cast(ltp), rn);

	check_non_constant_initializer(rn, lsym);
}


static const type_t *
designator_type(const designator *dr, const type_t *tp)
{
	switch (tp->t_tspec) {
	case STRUCT:
	case UNION:
		if (dr->dr_kind != DK_STRUCT && dr->dr_kind != DK_UNION) {
			const sym_t *fmem = first_named_member(tp);
			/* syntax error '%s' */
			error(249, "designator '[...]' is only for arrays");
			return fmem != NULL ? fmem->s_type : NULL;
		}

		lint_assert(dr->dr_member != NULL);
		return dr->dr_member->s_type;
	case ARRAY:
		if (dr->dr_kind != DK_ARRAY) {
			/* syntax error '%s' */
			error(249,
			    "designator '.member' is only for struct/union");
		}
		if (!tp->t_incomplete_array)
			lint_assert(dr->dr_subscript < (size_t)tp->t_dim);
		return tp->t_subt;
	default:
		if (dr->dr_kind != DK_SCALAR) {
			/* syntax error '%s' */
			error(249, "scalar type cannot use designator");
		}
		return tp;
	}
}


#ifdef DEBUG
static void
designator_debug(const designator *dr)
{

	if (dr->dr_kind == DK_STRUCT || dr->dr_kind == DK_UNION) {
		lint_assert(dr->dr_subscript == 0);
		debug_printf(".%s",
		    dr->dr_member != NULL
			? dr->dr_member->s_name
			: "<end>");
	} else if (dr->dr_kind == DK_ARRAY) {
		lint_assert(dr->dr_member == NULL);
		debug_printf("[%zu]", dr->dr_subscript);
	} else {
		lint_assert(dr->dr_member == NULL);
		lint_assert(dr->dr_subscript == 0);
		debug_printf("<scalar>");
	}

	if (dr->dr_done)
		debug_printf(" (done)");
}

static void
designation_debug(const designation *dn)
{
	size_t i;

	if (dn->dn_len == 0) {
		debug_step("designation: (empty)");
		return;
	}

	debug_print_indent();
	debug_printf("designation: ");
	for (i = 0; i < dn->dn_len; i++)
		designator_debug(dn->dn_items + i);
	debug_printf("\n");
}
#else
#define designation_debug(dn) do { } while (false)
#endif

static designator *
designation_last(designation *dn)
{

	lint_assert(dn->dn_len > 0);
	return &dn->dn_items[dn->dn_len - 1];
}

static void
designation_push(designation *dn, designator_kind kind,
		 const sym_t *member, size_t subscript)
{
	designator *dr;

	if (dn->dn_len == dn->dn_cap) {
		dn->dn_cap += 4;
		dn->dn_items = xrealloc(dn->dn_items,
		    dn->dn_cap * sizeof(dn->dn_items[0]));
	}

	dr = &dn->dn_items[dn->dn_len++];
	dr->dr_kind = kind;
	dr->dr_member = member;
	dr->dr_subscript = subscript;
	dr->dr_done = false;
	designation_debug(dn);
}

/*
 * Extend the designation as appropriate for the given type.
 *
 * C11 6.7.9p17
 */
static bool
designation_descend(designation *dn, const type_t *tp)
{

	if (is_struct_or_union(tp->t_tspec)) {
		const sym_t *member = first_named_member(tp);
		if (member == NULL)
			return false;
		designation_push(dn,
		    tp->t_tspec == STRUCT ? DK_STRUCT : DK_UNION, member, 0);
	} else if (tp->t_tspec == ARRAY)
		designation_push(dn, DK_ARRAY, NULL, 0);
	else
		designation_push(dn, DK_SCALAR, NULL, 0);
	return true;
}

/*
 * Starting at the type of the current object, resolve the type of the
 * sub-object by following each designator in the list.
 *
 * C99 6.7.8p18
 */
static const type_t *
designation_type(const designation *dn, const type_t *tp)
{
	size_t i;

	for (i = 0; i < dn->dn_len && tp != NULL; i++)
		tp = designator_type(dn->dn_items + i, tp);
	return tp;
}

static const type_t *
designation_parent_type(const designation *dn, const type_t *tp)
{
	size_t i;

	for (i = 0; i + 1 < dn->dn_len && tp != NULL; i++)
		tp = designator_type(dn->dn_items + i, tp);
	return tp;
}


static brace_level *
brace_level_new(const type_t *tp, brace_level *enclosing)
{
	brace_level *bl;

	bl = xcalloc(1, sizeof(*bl));
	bl->bl_type = tp;
	bl->bl_enclosing = enclosing;

	return bl;
}

static void
brace_level_free(brace_level *bl)
{

	free(bl->bl_designation.dn_items);
	free(bl);
}

#ifdef DEBUG
static void
brace_level_debug(const brace_level *bl)
{

	lint_assert(bl->bl_type != NULL);

	debug_printf("type '%s'\n", type_name(bl->bl_type));
	debug_indent_inc();
	designation_debug(&bl->bl_designation);
	debug_indent_dec();
}
#else
#define brace_level_debug(level) do { } while (false)
#endif

/* Return the type of the sub-object that is currently being initialized. */
static const type_t *
brace_level_sub_type(const brace_level *bl)
{

	return designation_type(&bl->bl_designation, bl->bl_type);
}

/*
 * After initializing a sub-object, advance the designation to point after
 * the sub-object that has just been initialized.
 *
 * C99 6.7.8p17
 * C11 6.7.9p17
 */
static void
brace_level_advance(brace_level *bl, size_t *max_subscript)
{
	const type_t *tp;
	designation *dn;
	designator *dr;

	debug_enter();
	dn = &bl->bl_designation;
	tp = designation_parent_type(dn, bl->bl_type);

	if (bl->bl_designation.dn_len == 0)
		(void)designation_descend(dn, bl->bl_type);
	dr = designation_last(dn);
	/* TODO: try to switch on dr->dr_kind instead */
	switch (tp->t_tspec) {
	case STRUCT:
		lint_assert(dr->dr_member != NULL);
		dr->dr_member = skip_unnamed(dr->dr_member->s_next);
		if (dr->dr_member == NULL)
			dr->dr_done = true;
		break;
	case UNION:
		dr->dr_member = NULL;
		dr->dr_done = true;
		break;
	case ARRAY:
		dr->dr_subscript++;
		if (tp->t_incomplete_array &&
		    dr->dr_subscript > *max_subscript)
			*max_subscript = dr->dr_subscript;
		if (!tp->t_incomplete_array &&
		    dr->dr_subscript >= (size_t)tp->t_dim)
			dr->dr_done = true;
		break;
	default:
		dr->dr_done = true;
		break;
	}
	designation_debug(dn);
	debug_leave();
}

static void
warn_too_many_initializers(designator_kind kind, const type_t *tp)
{

	if (kind == DK_STRUCT || kind == DK_UNION) {
		/* too many struct/union initializers */
		error(172);
	} else if (kind == DK_ARRAY) {
		lint_assert(tp->t_tspec == ARRAY);
		lint_assert(!tp->t_incomplete_array);
		/* too many array initializers, expected %d */
		error(173, tp->t_dim);
	} else {
		/* too many initializers */
		error(174);
	}

}

static bool
brace_level_pop_done(brace_level *bl, size_t *max_subscript)
{
	designation *dn = &bl->bl_designation;
	designator_kind dr_kind = designation_last(dn)->dr_kind;
	const type_t *sub_type = designation_parent_type(dn, bl->bl_type);

	while (designation_last(dn)->dr_done) {
		dn->dn_len--;
		designation_debug(dn);
		if (dn->dn_len == 0) {
			warn_too_many_initializers(dr_kind, sub_type);
			return false;
		}
		brace_level_advance(bl, max_subscript);
	}
	return true;
}

static void
brace_level_pop_final(brace_level *bl, size_t *max_subscript)
{
	designation *dn = &bl->bl_designation;

	while (dn->dn_len > 0 && designation_last(dn)->dr_done) {
		dn->dn_len--;
		designation_debug(dn);
		if (dn->dn_len == 0)
			return;
		brace_level_advance(bl, max_subscript);
	}
}

/*
 * Make the designation point to the sub-object to be initialized next.
 * Initially or after a previous expression, the designation is not advanced
 * yet since the place to stop depends on the next expression, especially for
 * string literals.
 */
static bool
brace_level_goto(brace_level *bl, const tnode_t *rn, size_t *max_subscript)
{
	const type_t *ltp;
	designation *dn;

	dn = &bl->bl_designation;
	if (dn->dn_len == 0 && can_init_character_array(bl->bl_type, rn))
		return true;
	if (dn->dn_len == 0 && !designation_descend(dn, bl->bl_type))
		return false;

again:
	if (!brace_level_pop_done(bl, max_subscript))
		return false;

	ltp = brace_level_sub_type(bl);
	if (eqtype(ltp, rn->tn_type, true, false, NULL))
		return true;

	if (is_struct_or_union(ltp->t_tspec) || ltp->t_tspec == ARRAY) {
		if (can_init_character_array(ltp, rn))
			return true;
		if (!designation_descend(dn, ltp))
			return false;
		goto again;
	}

	return true;
}


static initialization *
initialization_new(sym_t *sym, initialization *enclosing)
{
	initialization *in;

	in = xcalloc(1, sizeof(*in));
	in->in_sym = sym;
	in->in_enclosing = enclosing;

	return in;
}

static void
initialization_free(initialization *in)
{
	brace_level *bl, *next;

	/* TODO: lint_assert(in->in_brace_level == NULL) */
	for (bl = in->in_brace_level; bl != NULL; bl = next) {
		next = bl->bl_enclosing;
		brace_level_free(bl);
	}

	free(in);
}

#ifdef DEBUG
static void
initialization_debug(const initialization *in)
{
	size_t i;
	const brace_level *bl;

	if (in->in_err)
		debug_step("initialization error");
	if (in->in_brace_level == NULL) {
		debug_step("no brace level");
		return;
	}

	i = 0;
	for (bl = in->in_brace_level; bl != NULL; bl = bl->bl_enclosing) {
		debug_print_indent();
		debug_printf("brace level %zu: ", i);
		brace_level_debug(bl);
		i++;
	}
}
#else
#define initialization_debug(in) do { } while (false)
#endif

/*
 * Return the type of the object or sub-object that is currently being
 * initialized.
 */
static const type_t *
initialization_sub_type(initialization *in)
{
	const type_t *tp;

	if (in->in_brace_level == NULL)
		return in->in_sym->s_type;

	tp = brace_level_sub_type(in->in_brace_level);
	if (tp == NULL)
		in->in_err = true;
	return tp;
}

static void
initialization_lbrace(initialization *in)
{
	const type_t *tp;
	brace_level *outer_bl;

	if (in->in_err)
		return;

	debug_enter();

	tp = initialization_sub_type(in);
	if (tp == NULL)
		goto done;

	outer_bl = in->in_brace_level;
	if (!allow_c90 && outer_bl == NULL)
		check_trad_no_auto_aggregate(in->in_sym);

	if (!allow_c90 && tp->t_tspec == UNION) {
		/* initialization of union is illegal in traditional C */
		warning(238);
	}

	if (is_struct_or_union(tp->t_tspec) && tp->t_str->sou_incomplete) {
		/* initialization of incomplete type '%s' */
		error(175, type_name(tp));
		in->in_err = true;
		goto done;
	}

	if (outer_bl != NULL && outer_bl->bl_designation.dn_len == 0) {
		designation *dn = &outer_bl->bl_designation;
		(void)designation_descend(dn, outer_bl->bl_type);
		tp = designation_type(dn, outer_bl->bl_type);
	}

	in->in_brace_level = brace_level_new(tp, outer_bl);
	if (is_struct_or_union(tp->t_tspec) &&
	    first_named_member(tp) == NULL) {
		/* cannot initialize struct/union with no named member */
		error(179);
		in->in_err = true;
	}

done:
	initialization_debug(in);
	debug_leave();
}

static void
initialization_rbrace(initialization *in)
{
	brace_level *inner_bl, *outer_bl;

	debug_enter();

	if (in->in_brace_level != NULL)
		brace_level_pop_final(in->in_brace_level,
		    &in->in_max_subscript);

	/* C99 6.7.8p22 */
	if (in->in_sym->s_type->t_incomplete_array &&
	    in->in_brace_level->bl_enclosing == NULL) {

		/* prevent "empty array declaration: %s" */
		size_t dim = in->in_max_subscript;
		if (dim == 0 && in->in_err)
			dim = 1;

		update_type_of_array_of_unknown_size(in->in_sym, dim);
	}

	if (in->in_err)
		goto done;

	inner_bl = in->in_brace_level;
	outer_bl = inner_bl->bl_enclosing;
	in->in_brace_level = outer_bl;
	brace_level_free(inner_bl);

	if (outer_bl != NULL)
		brace_level_advance(outer_bl, &in->in_max_subscript);

done:
	initialization_debug(in);
	debug_leave();
}

static void
initialization_add_designator_member(initialization *in, const char *name)
{
	brace_level *bl;
	const type_t *tp;
	const sym_t *member;

	if (in->in_err)
		return;

	bl = in->in_brace_level;
	lint_assert(bl != NULL);

	tp = brace_level_sub_type(bl);
	if (is_struct_or_union(tp->t_tspec))
		goto proceed;
	else if (tp->t_tspec == ARRAY) {
		/* syntax error '%s' */
		error(249, "designator '.member' is only for struct/union");
		in->in_err = true;
		return;
	} else {
		/* syntax error '%s' */
		error(249, "scalar type cannot use designator");
		in->in_err = true;
		return;
	}

proceed:
	member = look_up_member(tp, name);
	if (member == NULL) {
		/* type '%s' does not have member '%s' */
		error(101, type_name(tp), name);
		in->in_err = true;
		return;
	}

	designation_push(&bl->bl_designation,
	    tp->t_tspec == STRUCT ? DK_STRUCT : DK_UNION, member, 0);
}

static void
initialization_add_designator_subscript(initialization *in, size_t subscript)
{
	brace_level *bl;
	const type_t *tp;

	if (in->in_err)
		return;

	bl = in->in_brace_level;
	lint_assert(bl != NULL);

	tp = brace_level_sub_type(bl);
	if (tp->t_tspec != ARRAY) {
		/* syntax error '%s' */
		error(249, "designator '[...]' is only for arrays");
		in->in_err = true;
		return;
	}

	if (!tp->t_incomplete_array && subscript >= (size_t)tp->t_dim) {
		/* array subscript cannot be > %d: %ld */
		error(168, tp->t_dim - 1, (long)subscript);
		subscript = 0;	/* suppress further errors */
	}

	if (tp->t_incomplete_array && subscript > in->in_max_subscript)
		in->in_max_subscript = subscript;

	designation_push(&bl->bl_designation, DK_ARRAY, NULL, subscript);
}

/*
 * Initialize an object with automatic storage duration that has an
 * initializer expression without braces.
 */
static bool
initialization_expr_using_op(initialization *in, tnode_t *rn)
{
	tnode_t *ln, *tn;

	if (!has_automatic_storage_duration(in->in_sym))
		return false;
	if (in->in_brace_level != NULL)
		return false;
	if (in->in_sym->s_type->t_tspec == ARRAY)
		return false;

	debug_step("handing over to INIT");

	ln = build_name(in->in_sym, false);
	ln->tn_type = expr_unqualified_type(ln->tn_type);

	tn = build_binary(ln, INIT, false /* XXX */, rn);
	expr(tn, false, false, false, false);

	return true;
}

/* Initialize a character array or wchar_t array with a string literal. */
static bool
initialization_init_array_from_string(initialization *in, tnode_t *tn)
{
	brace_level *bl;
	const type_t *tp;
	size_t len;

	if (tn->tn_op != STRING)
		return false;

	tp = initialization_sub_type(in);

	if (!can_init_character_array(tp, tn))
		return false;

	len = tn->tn_string->st_len;
	if (!tp->t_incomplete_array && (size_t)tp->t_dim < len) {
		/* string literal too long (%lu) for target array (%lu) */
		warning(187, (unsigned long)len, (unsigned long)tp->t_dim);
	}

	bl = in->in_brace_level;
	if (bl != NULL && bl->bl_designation.dn_len == 0)
		(void)designation_descend(&bl->bl_designation, bl->bl_type);
	if (bl != NULL)
		brace_level_advance(bl, &in->in_max_subscript);

	if (tp->t_incomplete_array)
		update_type_of_array_of_unknown_size(in->in_sym, len + 1);

	return true;
}

/*
 * Initialize a single sub-object as part of the currently ongoing
 * initialization.
 */
static void
initialization_expr(initialization *in, tnode_t *tn)
{
	brace_level *bl;
	const type_t *tp;

	if (in->in_err || tn == NULL)
		return;

	debug_enter();

	bl = in->in_brace_level;
	if (bl != NULL && !brace_level_goto(bl, tn, &in->in_max_subscript)) {
		in->in_err = true;
		goto done;
	}
	if (initialization_expr_using_op(in, tn))
		goto done;
	if (initialization_init_array_from_string(in, tn))
		goto done;
	if (in->in_err)
		goto done;

	tp = initialization_sub_type(in);
	if (tp == NULL)
		goto done;

	if (bl == NULL && !is_scalar(tp->t_tspec)) {
		/* {}-enclosed initializer required */
		error(181);
		goto done;
	}

	debug_step("expecting '%s', expression has '%s'",
	    type_name(tp), type_name(tn->tn_type));
	check_init_expr(tp, in->in_sym, tn);
	if (bl != NULL)
		brace_level_advance(bl, &in->in_max_subscript);

done:
	initialization_debug(in);
	debug_leave();
}


static initialization *init;


static initialization *
current_init(void)
{

	lint_assert(init != NULL);
	return init;
}

sym_t **
current_initsym(void)
{

	return &current_init()->in_sym;
}

void
begin_initialization(sym_t *sym)
{

	debug_step("begin initialization of '%s'", type_name(sym->s_type));
	debug_indent_inc();

	init = initialization_new(sym, init);
}

void
end_initialization(void)
{
	initialization *in;

	in = init;
	init = in->in_enclosing;
	initialization_free(in);

	debug_indent_dec();
	debug_step("end initialization");
}

void
begin_designation(void)
{
	initialization *in;
	brace_level *bl;

	in = current_init();
	if (in->in_err)
		return;

	bl = in->in_brace_level;
	lint_assert(bl != NULL);
	bl->bl_designation.dn_len = 0;
	designation_debug(&bl->bl_designation);
}

void
add_designator_member(sbuf_t *sb)
{

	initialization_add_designator_member(current_init(), sb->sb_name);
}

void
add_designator_subscript(range_t range)
{

	initialization_add_designator_subscript(current_init(), range.hi);
}

void
init_lbrace(void)
{

	initialization_lbrace(current_init());
}

void
init_expr(tnode_t *tn)
{

	initialization_expr(current_init(), tn);
}

void
init_rbrace(void)
{

	initialization_rbrace(current_init());
}
