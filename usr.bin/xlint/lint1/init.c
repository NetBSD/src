/*	$NetBSD: init.c,v 1.225 2021/12/21 16:50:11 rillig Exp $	*/

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
__RCSID("$NetBSD: init.c,v 1.225 2021/12/21 16:50:11 rillig Exp $");
#endif

#include <stdlib.h>
#include <string.h>

#include "lint1.h"


/*
 * Initialization of global or local objects, like in:
 *
 *	int number = 12345;
 *	int number_with_braces = { 12345 };
 *
 *	int array_of_unknown_size[] = { 111, 222, 333 };
 *	int array_flat[2][2] = { 11, 12, 21, 22 };
 *	int array_nested[2][2] = { { 11, 12 }, { 21, 22 } };
 *
 *	struct { int x, y; } point = { 3, 4 };
 *	struct { int x, y; } point = { .y = 4, .x = 3 };
 *
 * Any scalar expression or string in the initializer may be surrounded by
 * additional pairs of braces.
 *
 * For nested aggregate objects, the inner braces may be omitted like in
 * array_flat or spelled out like in array_nested.  This is unusual in
 * practice and therefore only supported very basically.
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

/*
 * A single component on the path to the sub-object that is initialized by an
 * initializer expression.  Either a struct or union member, or an array
 * subscript.
 *
 * C99 6.7.8p6, 6.7.8p7
 */
typedef struct designator {
	const char	*dr_name;	/* for struct and union */
	size_t		dr_subscript;	/* for array */
} designator;

/*
 * The path from the "current object" of a brace level to the sub-object that
 * will be initialized by the next expression.  Examples for designations are
 * '.member' or '.member[123].member.member[1][1]'.
 *
 * C99 6.7.8p6, 6.7.8p7
 */
typedef struct designation {
	designator	*dn_items;
	size_t		dn_len;
	size_t		dn_cap;
} designation;

/*
 * Everything that happens between a '{' and the corresponding '}' of an
 * initialization.
 *
 * C99 6.7.8p17
 */
typedef struct brace_level {
	/* The type of the "current object". */
	const type_t	*bl_type;

	/*
	 * The path to the sub-object of the "current object" that is
	 * initialized by the next expression.
	 *
	 * TODO: use this not only for explicit designations but also for
	 *  implicit designations, like in C90.
	 */
	designation	bl_designation;

	/*
	 * The next member of the struct or union that is to be initialized,
	 * unless a specific member is selected by a designator.
	 *
	 * TODO: use bl_designation instead.
	 */
	const sym_t	*bl_member;
	/*
	 * The subscript of the next array element that is to be initialized,
	 * unless a specific subscript is selected by a designator.
	 *
	 * TODO: use bl_designation instead.
	 */
	size_t		bl_subscript;

	/*
	 * Whether the designation is used up, that is, there is no next
	 * sub-object left to be initialized.
	 */
	bool		bl_scalar_done:1;	/* for scalars */

	/*
	 * Whether lint has been confused by allowed but extra or omitted
	 * braces.  In such a case, lint skips further type checks on the
	 * initializer expressions.
	 *
	 * TODO: properly handle the omitted braces.
	 */
	bool		bl_confused:1;

	struct brace_level *bl_enclosing;
} brace_level;

/*
 * An ongoing initialization.
 *
 * In most cases there is only ever a single initialization going on.  See
 * pointer_to_compound_literal in msg_171.c for an exception.
 */
typedef struct initialization {
	/* The symbol that is to be initialized. */
	sym_t		*in_sym;

	/* The innermost brace level. */
	brace_level	*in_brace_level;

	/*
	 * The maximum subscript that has ever be seen for an array of
	 * unknown size, which can only occur at the outermost brace level.
	 */
	size_t		in_max_subscript;

	/*
	 * Is set when a structural error occurred in the initialization.
	 * The effect is that the rest of the initialization is ignored
	 * (parsed by yacc, expression trees built, but no initialization
	 * takes place).
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

/* C99 6.7.8p14, 6.7.8p15 */
static bool
is_character_array(const type_t *tp, tspec_t t)
{
	tspec_t st;

	if (tp == NULL || tp->t_tspec != ARRAY)
		return false;

	st = tp->t_subt->t_tspec;
	return t == CHAR
	    ? st == CHAR || st == UCHAR || st == SCHAR
	    : st == WCHAR;
}

/* C99 6.7.8p9 */
static bool
is_unnamed(const sym_t *m)
{

	return m->s_bitfield && m->s_name == unnamed;
}

/* C99 6.7.8p9 */
static const sym_t *
skip_unnamed(const sym_t *m)
{

	while (m != NULL && is_unnamed(m))
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
		if (!is_unnamed(m) && strcmp(m->s_name, name) == 0)
			return m;
	return NULL;
}

static const type_t *
sym_type(const sym_t *sym)
{

	return sym != NULL ? sym->s_type : NULL;
}

static const type_t *
look_up_member_type(const type_t *tp, const char *name)
{
	const sym_t *member;

	member = look_up_member(tp, name);
	if (member == NULL) {
		/* type '%s' does not have member '%s' */
		error(101, type_name(tp), name);
	}

	return sym_type(member);
}

/*
 * C99 6.7.8p22 says that the type of an array of unknown size becomes known
 * at the end of its initializer list.
 */
static void
update_type_of_array_of_unknown_size(sym_t *sym, size_t size)
{
	type_t *tp;

	tp = dup_type(sym->s_type);
	tp->t_dim = (int)size;
	tp->t_incomplete_array = false;
	sym->s_type = tp;
}


/* In traditional C, bit-fields can be initialized only by integer constants. */
static void
check_bit_field_init(const tnode_t *ln, tspec_t lt, tspec_t rt)
{

	if (tflag &&
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
	ln = expr_zalloc(sizeof(*ln));
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
		if (dr->dr_name == NULL) {
			/* syntax error '%s' */
			error(249, "designator '[...]' is only for arrays");
			return sym_type(first_named_member(tp));
		}

		return look_up_member_type(tp, dr->dr_name);
	case ARRAY:
		if (dr->dr_name != NULL) {
			/* syntax error '%s' */
			error(249,
			    "designator '.member' is only for struct/union");
		}
		if (!tp->t_incomplete_array &&
		    dr->dr_subscript >= (size_t)tp->t_dim) {
			/* array subscript cannot be > %d: %ld */
			error(168, tp->t_dim - 1, (long)dr->dr_subscript);
		}
		return tp->t_subt;
	default:
		/* syntax error '%s' */
		error(249, "scalar type cannot use designator");
		return tp;
	}
}


#ifdef DEBUG
static void
designation_debug(const designation *dn)
{
	size_t i;

	if (dn->dn_len == 0)
		return;

	debug_indent();
	debug_printf("designation: ");
	for (i = 0; i < dn->dn_len; i++) {
		const designator *dr = dn->dn_items + i;
		if (dr->dr_name != NULL) {
			debug_printf(".%s", dr->dr_name);
			lint_assert(dr->dr_subscript == 0);
		} else
			debug_printf("[%zu]", dr->dr_subscript);
	}
	debug_printf("\n");
}
#else
#define designation_debug(dn) do { } while (false)
#endif

static void
designation_add(designation *dn, const char *name, size_t subscript)
{

	if (dn->dn_len == dn->dn_cap) {
		dn->dn_cap += 4;
		dn->dn_items = xrealloc(dn->dn_items,
		    dn->dn_cap * sizeof(dn->dn_items[0]));
	}

	dn->dn_items[dn->dn_len].dr_name = name;
	dn->dn_items[dn->dn_len].dr_subscript = subscript;
	dn->dn_len++;
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

static void
designation_reset(designation *dn)
{

	dn->dn_len = 0;
}

static void
designation_free(designation *dn)
{

	free(dn->dn_items);
}


static brace_level *
brace_level_new(const type_t *tp, brace_level *enclosing)
{
	brace_level *bl;

	bl = xcalloc(1, sizeof(*bl));
	bl->bl_type = tp;
	bl->bl_enclosing = enclosing;
	if (is_struct_or_union(tp->t_tspec))
		bl->bl_member = first_named_member(tp);

	return bl;
}

static void
brace_level_free(brace_level *bl)
{

	designation_free(&bl->bl_designation);
	free(bl);
}

#ifdef DEBUG
static void
brace_level_debug(const brace_level *bl)
{

	lint_assert(bl->bl_type != NULL);
	lint_assert(bl->bl_member == NULL || !is_unnamed(bl->bl_member));

	debug_printf("type '%s'", type_name(bl->bl_type));

	if (is_struct_or_union(bl->bl_type->t_tspec) && bl->bl_member != NULL)
		debug_printf(", member '%s'", bl->bl_member->s_name);
	if (bl->bl_type->t_tspec == ARRAY)
		debug_printf(", subscript %zu", bl->bl_subscript);

	debug_printf("\n");
}
#else
#define brace_level_debug(level) do { } while (false)
#endif

/* Return the type of the sub-object that is currently being initialized. */
static const type_t *
brace_level_sub_type(const brace_level *bl, bool is_string)
{

	if (bl->bl_designation.dn_len > 0)
		return designation_type(&bl->bl_designation, bl->bl_type);

	switch (bl->bl_type->t_tspec) {
	case STRUCT:
	case UNION:
		if (bl->bl_member == NULL) {
			/* too many struct/union initializers */
			error(172);
			return NULL;
		}

		lint_assert(!is_unnamed(bl->bl_member));
		return sym_type(bl->bl_member);

	case ARRAY:
		if (!bl->bl_confused && !bl->bl_type->t_incomplete_array &&
		    bl->bl_subscript >= (size_t)bl->bl_type->t_dim) {
			/* too many array initializers, expected %d */
			error(173, bl->bl_type->t_dim);
		}

		if (is_string && bl->bl_subscript == 0 &&
		    bl->bl_type->t_subt->t_tspec != ARRAY)
			return bl->bl_type;
		return bl->bl_type->t_subt;

	default:
		if (bl->bl_scalar_done) {
			/* too many initializers */
			error(174);
		}

		return bl->bl_type;
	}
}

/* C99 6.7.8p17 */
static void
brace_level_apply_designation(brace_level *bl)
{
	const designator *dr;

	if (bl->bl_designation.dn_len == 0)
		return;
	dr = &bl->bl_designation.dn_items[0];

	designation_debug(&bl->bl_designation);

	switch (bl->bl_type->t_tspec) {
	case STRUCT:
	case UNION:
		if (dr->dr_name == NULL)
			return;	/* error, silently ignored */
		bl->bl_member = look_up_member(bl->bl_type, dr->dr_name);
		break;
	case ARRAY:
		if (dr->dr_name != NULL)
			return;	/* error, silently ignored */
		bl->bl_subscript = dr->dr_subscript;
		break;
	default:
		break;		/* error, silently ignored */
	}
}

/*
 * After initializing a sub-object, advance to the next sub-object.
 *
 * C99 6.7.8p17
 */
static void
brace_level_advance(brace_level *bl, size_t *max_subscript)
{

	switch (bl->bl_type->t_tspec) {
	case STRUCT:
		lint_assert(bl->bl_member != NULL);
		bl->bl_member = skip_unnamed(bl->bl_member->s_next);
		break;
	case UNION:
		bl->bl_member = NULL;
		break;
	case ARRAY:
		bl->bl_subscript++;
		if (bl->bl_subscript > *max_subscript)
			*max_subscript = bl->bl_subscript;
		break;
	default:
		bl->bl_scalar_done = true;
		break;
	}
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

	if (in->in_brace_level == NULL) {
		debug_step("no brace level");
		return;
	}

	i = 0;
	for (bl = in->in_brace_level; bl != NULL; bl = bl->bl_enclosing) {
		debug_indent();
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
initialization_sub_type(initialization *in, bool is_string)
{
	const type_t *tp;

	tp = in->in_brace_level != NULL
	    ? brace_level_sub_type(in->in_brace_level, is_string)
	    : in->in_sym->s_type;
	if (tp == NULL)
		in->in_err = true;
	return tp;
}

static void
initialization_lbrace(initialization *in)
{
	const type_t *tp;

	if (in->in_err)
		return;

	debug_enter();

	tp = initialization_sub_type(in, false);
	if (tp == NULL) {
		in->in_err = true;
		goto done;
	}

	if (tflag && in->in_brace_level == NULL)
		check_trad_no_auto_aggregate(in->in_sym);

	if (tflag && tp->t_tspec == UNION) {
		/* initialization of union is illegal in traditional C */
		warning(238);
	}

	if (tp->t_tspec == STRUCT && tp->t_str->sou_incomplete) {
		/* initialization of incomplete type '%s' */
		error(175, type_name(tp));
		in->in_err = true;
		goto done;
	}

	if (in->in_brace_level != NULL)
		brace_level_apply_designation(in->in_brace_level);

	in->in_brace_level = brace_level_new(tp, in->in_brace_level);

done:
	initialization_debug(in);
	debug_leave();
}

/* C99 6.7.8p22 */
static void
initialization_set_size_of_unknown_array(initialization *in)
{
	size_t dim;

	if (!(in->in_sym->s_type->t_incomplete_array &&
	      in->in_brace_level->bl_enclosing == NULL))
		return;

	dim = in->in_max_subscript;
	if (dim == 0 && (in->in_err || in->in_brace_level->bl_confused))
		dim = 1;	/* prevent "empty array declaration: %s" */

	update_type_of_array_of_unknown_size(in->in_sym, dim);
}

static void
initialization_rbrace(initialization *in)
{
	brace_level *inner_bl, *outer_bl;

	debug_enter();

	initialization_set_size_of_unknown_array(in);
	if (in->in_err)
		goto done;

	inner_bl = in->in_brace_level;
	outer_bl = inner_bl->bl_enclosing;
	in->in_brace_level = outer_bl;
	brace_level_free(inner_bl);

	if (outer_bl != NULL) {
		brace_level_advance(outer_bl, &in->in_max_subscript);
		designation_reset(&outer_bl->bl_designation);
	}

done:
	initialization_debug(in);
	debug_leave();
}

static void
initialization_add_designator(initialization *in,
			      const char *name, size_t subscript)
{

	if (in->in_err)
		return;

	lint_assert(in->in_brace_level != NULL);
	designation_add(&in->in_brace_level->bl_designation, name, subscript);
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
	strg_t	*strg;

	if (tn->tn_op != STRING)
		return false;

	bl = in->in_brace_level;
	tp = initialization_sub_type(in, true);
	strg = tn->tn_string;

	if (!is_character_array(tp, strg->st_tspec)) {
		/* TODO: recursively try first member or [0] */
		if (is_struct_or_union(tp->t_tspec) ||
		    (tp->t_tspec == ARRAY &&
		     is_struct_or_union(tp->t_subt->t_tspec)))
			in->in_err = true;
		return false;
	}

	if (bl != NULL && tp->t_tspec != ARRAY && bl->bl_subscript != 0)
		return false;

	if (!tp->t_incomplete_array && tp->t_dim < (int)strg->st_len) {
		/* non-null byte ignored in string initializer */
		warning(187);
	}

	if (tp == in->in_sym->s_type && tp->t_incomplete_array) {
		if (bl != NULL) {
			bl->bl_subscript = strg->st_len;
			/* see brace_level_advance for the +1 */
			/* see initialization_set_size_of_unknown_array */
		} else
			update_type_of_array_of_unknown_size(in->in_sym,
			    strg->st_len + 1);
	}

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

	if (in->in_err)
		return;

	bl = in->in_brace_level;
	if (bl != NULL && bl->bl_confused)
		return;

	debug_enter();

	if (tn == NULL)
		goto advance;
	if (initialization_expr_using_op(in, tn))
		goto done;
	if (initialization_init_array_from_string(in, tn))
		goto advance;
	if (in->in_err)
		goto done;

	if (bl != NULL)
		brace_level_apply_designation(bl);
	tp = initialization_sub_type(in, false);
	if (tp == NULL)
		goto done;

	if (bl == NULL && !is_scalar(tp->t_tspec)) {
		/* {}-enclosed initializer required */
		error(181);
		goto done;
	}

	/*
	 * Hack to accept initializations with omitted braces, see
	 * c99_6_7_8_p28_example5 in test d_c99_init.c.  Since both GCC and
	 * Clang already warn about this at level -Wall, there is no point
	 * in repeating the same check in lint.  If needed, support for these
	 * edge cases could be added, but that would increase the complexity.
	 */
	if ((is_scalar(tn->tn_type->t_tspec) ||
	     tn->tn_type->t_tspec == FUNC) &&
	    (tp->t_tspec == ARRAY || is_struct_or_union(tp->t_tspec)) &&
	    bl != NULL) {
		bl->bl_confused = true;
		goto done;
	}

	debug_step("expecting '%s', expression has '%s'",
	    type_name(tp), type_name(tn->tn_type));
	check_init_expr(tp, in->in_sym, tn);

advance:
	if (bl != NULL)
		brace_level_advance(bl, &in->in_max_subscript);
done:
	if (bl != NULL)
		designation_reset(&bl->bl_designation);

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
	brace_level *bl;

	bl = current_init()->in_brace_level;
	lint_assert(bl != NULL);
	designation_reset(&bl->bl_designation);
}

void
add_designator_member(sbuf_t *sb)
{

	initialization_add_designator(current_init(), sb->sb_name, 0);
}

void
add_designator_subscript(range_t range)
{

	initialization_add_designator(current_init(), NULL, range.hi);
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
