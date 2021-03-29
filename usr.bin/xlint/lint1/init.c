/*	$NetBSD: init.c,v 1.178 2021/03/29 21:34:17 rillig Exp $	*/

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
__RCSID("$NetBSD: init.c,v 1.178 2021/03/29 21:34:17 rillig Exp $");
#endif

#include <stdlib.h>
#include <string.h>

#include "lint1.h"


/*
 * Initialization
 *
 * Handles initializations of global or local objects, like in:
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
 * Any scalar expression in the initializer may be surrounded by arbitrarily
 * many extra pairs of braces, like in the example 'number_with_braces' (C99
 * 6.7.8p11).
 *
 * For multi-dimensional arrays, the inner braces may be omitted like in
 * array_flat or spelled out like in array_nested.
 *
 * For the initializer, the grammar parser calls these functions:
 *
 *	begin_initialization
 *		init_lbrace			for each '{'
 *		add_designator_member		for each '.member' before '='
 *		add_designator_subscript	for each '[123]' before '='
 *		init_using_expr			for each expression
 *		init_rbrace			for each '}'
 *	end_initialization
 *
 * Each '{' begins a new brace level, each '}' ends the current brace level.
 * Each brace level has an associated "current object".
 *
 * Most of the time, the topmost level of brace_level contains a scalar type,
 * and its remaining count toggles between 1 and 0.
 *
 * See also:
 *	C99 6.7.8 "Initialization"
 *	d_c99_init.c for more examples
 */


/*
 * Describes a single brace level of an ongoing initialization.
 *
 * XXX: Since C99, the initializers can be listed in arbitrary order by using
 * designators to specify the sub-object to be initialized.  The member names
 * of non-leaf structs may thus appear repeatedly, as demonstrated in
 * d_init_pop_member.c.
 *
 * See C99 6.7.8, which spans 6 pages full of tricky details and carefully
 * selected examples.
 */
struct brace_level {

	/*
	 * The type of the current object that is initialized at this brace
	 * level.
	 *
	 * On the outermost element, this is always NULL since the outermost
	 * initializer-expression may be enclosed in an optional pair of
	 * braces, as of the current implementation.
	 *
	 * FIXME: This approach is wrong.  It's not that the outermost
	 * initializer may be enclosed in additional braces, it's every scalar
	 * that may be enclosed in additional braces, as of C99 6.7.8p11.
	 *
	 * Everywhere else it is nonnull.
	 */
	type_t	*bl_type;

	/*
	 * The type that will be initialized at the next initialization level,
	 * usually enclosed by another pair of braces.
	 *
	 * For an array, it is the element type, but without 'const'.
	 *
	 * For a struct or union type, it is one of the member types, but
	 * without 'const'.
	 *
	 * The outermost stack element has no bl_type but nevertheless has
	 * bl_subtype.  For example, in 'int var = { 12345 }', initially there
	 * is a brace_level with bl_subtype 'int'.  When the '{' is processed,
	 * an element with bl_type 'int' is pushed to the stack.  When the
	 * corresponding '}' is processed, the inner element is popped again.
	 *
	 * During initialization, only the top 2 elements of the stack are
	 * looked at.
	 *
	 * XXX: Having bl_subtype here is the wrong approach, it should not be
	 * necessary at all; see bl_type.
	 */
	type_t	*bl_subtype;

	/*
	 * Whether this level of the initializer requires a '}' to be
	 * completed.
	 *
	 * Multidimensional arrays do not need a closing brace to complete
	 * an inner array; for example, { 1, 2, 3, 4 } is a valid initializer
	 * for 'int arr[2][2]'.
	 *
	 * XXX: Double-check whether this is the correct approach at all; see
	 * bl_type.
	 */
	bool bl_brace: 1;

	/* Whether bl_type is an array of unknown size. */
	bool bl_array_of_unknown_size: 1;

	/*
	 * XXX: This feels wrong.  Whether or not there has been a named
	 * initializer (called 'designation' since C99) should not matter at
	 * all.  Even after an initializer with designation, counting of the
	 * remaining elements continues, see C99 6.7.8p17.
	 */
	bool bl_seen_named_member: 1;

	/*
	 * For structs, the next member to be initialized by a designator-less
	 * initializer.
	 */
	sym_t *bl_next_member;

	/* TODO: Add bl_next_subscript for arrays. */

	/* TODO: Understand C99 6.7.8p17 and footnote 128 for unions. */

	/*
	 * The number of remaining elements to be used by expressions without
	 * designator.
	 *
	 * This says nothing about which members have been initialized or not
	 * since starting with C99, members may be initialized in arbitrary
	 * order by using designators.
	 *
	 * For an array of unknown size, this is always 0 and thus irrelevant.
	 *
	 * XXX: for scalars?
	 * XXX: for structs?
	 * XXX: for unions?
	 * XXX: for arrays?
	 *
	 * XXX: Having the count of remaining objects should not be necessary.
	 * It is probably clearer to use bl_next_member and bl_next_subscript
	 * for this purpose.
	 */
	int bl_remaining;

	/*
	 * The initialization state of the enclosing data structure
	 * (struct, union, array).
	 *
	 * XXX: Or for a scalar, for the top-level element, or for expressions
	 * in redundant braces such as '{{{{ 0 }}}}' (not yet implemented as
	 * of 2021-03-25).
	 */
	struct brace_level *bl_enclosing;
};

/*
 * A single component on the path to the sub-object that is initialized by an
 * initializer expression.  Either a struct or union member, or an array
 * subscript.
 *
 * See also: C99 6.7.8 "Initialization"
 */
struct designator {
	const char	*dr_name;		/* for struct and union */
	/* TODO: add 'dr_subscript' for arrays */
	struct designator *dr_next;
};

/*
 * The optional designation for an initializer, saying which sub-object to
 * initialize.  Examples for designations are '.member' or
 * '.member[123].member.member[1][1]'.
 *
 * See also: C99 6.7.8 "Initialization"
 */
struct designation {
	struct designator *dn_head;
	struct designator *dn_tail;
};

struct initialization {
	/*
	 * is set as soon as a fatal error occurred in the initialization.
	 * The effect is that the rest of the initialization is ignored
	 * (parsed by yacc, expression trees built, but no initialization
	 * takes place).
	 */
	bool		in_err;

	/* The symbol that is to be initialized. */
	sym_t		*in_sym;

	/* The innermost brace level. */
	struct brace_level *in_brace_level;

	/*
	 * The C99 designator, if any, for the current initialization
	 * expression.
	 */
	struct designation in_designation;

	struct initialization *in_next;
};


static struct initialization *init;


#ifdef DEBUG
static int debug_ind = 0;
#endif


#ifdef DEBUG

static void __printflike(1, 2)
debug_printf(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stdout, fmt, va);
	va_end(va);
}

static void
debug_indent(void)
{

	debug_printf("%*s", 2 * debug_ind, "");
}

static void
debug_enter(const char *func)
{

	printf("%*s+ %s\n", 2 * debug_ind++, "", func);
}

static void __printflike(1, 2)
debug_step(const char *fmt, ...)
{
	va_list va;

	debug_indent();
	va_start(va, fmt);
	vfprintf(stdout, fmt, va);
	va_end(va);
	printf("\n");
}

static void
debug_leave(const char *func)
{

	printf("%*s- %s\n", 2 * --debug_ind, "", func);
}

#define debug_enter()		(debug_enter)(__func__)
#define debug_leave()		(debug_leave)(__func__)

#else

#define debug_printf(fmt, ...)	do { } while (false)
#define debug_indent()		do { } while (false)
#define debug_enter()		do { } while (false)
#define debug_step(fmt, ...)	do { } while (false)
#define debug_leave()		do { } while (false)

#endif


static void *
unconst_cast(const void *p)
{
	void *r;

	memcpy(&r, &p, sizeof r);
	return r;
}

static bool
is_struct_or_union(tspec_t t)
{

	return t == STRUCT || t == UNION;
}

static bool
has_automatic_storage_duration(const sym_t *sym)
{

	return sym->s_scl == AUTO || sym->s_scl == REG;
}

/* C99 6.7.8p14, 6.7.8p15 */
static bool
is_string_array(const type_t *tp, tspec_t t)
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
is_unnamed_member(const sym_t *m)
{

	return m->s_bitfield && m->s_name == unnamed;
}

static const sym_t *
look_up_member(const sym_t *m, const char *name)
{

	for (; m != NULL; m = m->s_next)
		if (!is_unnamed_member(m) && strcmp(m->s_name, name) == 0)
			return m;
	return NULL;
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
check_init_expr(const type_t *tp, sym_t *sym, tnode_t *tn)
{
	tnode_t *ln;
	tspec_t lt, rt;
	struct mbl *tmem;

	/* Create a temporary node for the left side. */
	ln = tgetblk(sizeof *ln);
	ln->tn_op = NAME;
	ln->tn_type = tduptyp(tp);
	ln->tn_type->t_const = false;
	ln->tn_lvalue = true;
	ln->tn_sym = sym;

	tn = cconv(tn);

	lt = ln->tn_type->t_tspec;
	rt = tn->tn_type->t_tspec;

	debug_step("typeok '%s', '%s'",
	    type_name(ln->tn_type), type_name(tn->tn_type));
	if (!typeok(INIT, 0, ln, tn))
		return;

	/*
	 * Preserve the tree memory. This is necessary because otherwise
	 * expr() would free it.
	 */
	tmem = tsave();
	expr(tn, true, false, true, false);
	trestor(tmem);

	check_bit_field_init(ln, lt, rt);

	/*
	 * XXX: Is it correct to do this conversion _after_ the typeok above?
	 */
	if (lt != rt || (tp->t_bitfield && tn->tn_op == CON))
		tn = convert(INIT, 0, unconst_cast(tp), tn);

	check_non_constant_initializer(tn, sym);
}


static struct designator *
designator_new(const char *name)
{
	struct designator *dr;

	dr = xcalloc(1, sizeof *dr);
	dr->dr_name = name;
	return dr;
}

static void
designator_free(struct designator *dr)
{

	free(dr);
}


#ifdef DEBUG
static void
designation_debug(const struct designation *dn)
{
	const struct designator *dr;

	if (dn->dn_head == NULL)
		return;

	debug_indent();
	debug_printf("designation: ");
	for (dr = dn->dn_head; dr != NULL; dr = dr->dr_next)
		debug_printf(".%s", dr->dr_name);
	debug_printf("\n");
}
#else
#define designation_debug(dn) do { } while (false)
#endif

static void
designation_add(struct designation *dn, struct designator *dr)
{

	if (dn->dn_head != NULL) {
		dn->dn_tail->dr_next = dr;
		dn->dn_tail = dr;
	} else {
		dn->dn_head = dr;
		dn->dn_tail = dr;
	}

	designation_debug(dn);
}

/* TODO: add support for array subscripts, not only named members */
/*
 * TODO: This function should not be necessary at all.  There is no need to
 *  remove the head of the list.
 */
static void
designation_shift_level(struct designation *dn)
{
	lint_assert(dn->dn_head != NULL);

	if (dn->dn_head == dn->dn_tail) {
		designator_free(dn->dn_head);
		dn->dn_head = NULL;
		dn->dn_tail = NULL;
	} else {
		struct designator *head = dn->dn_head;
		dn->dn_head = dn->dn_head->dr_next;
		designator_free(head);
	}

	designation_debug(dn);
}


static struct brace_level *
brace_level_new(type_t *type, type_t *subtype, int remaining,
		struct brace_level *enclosing)
{
	struct brace_level *bl = xcalloc(1, sizeof(*bl));

	bl->bl_type = type;
	bl->bl_subtype = subtype;
	bl->bl_remaining = remaining;
	bl->bl_enclosing = enclosing;

	return bl;
}

static void
brace_level_free(struct brace_level *bl)
{
	free(bl);
}

#ifdef DEBUG
/*
 * TODO: only log the top of the stack after each modifying operation
 *
 * TODO: wrap all write accesses to brace_level in setter functions
 */
static void
brace_level_debug(const struct brace_level *bl)
{
	if (bl->bl_type != NULL)
		debug_printf("type '%s'", type_name(bl->bl_type));
	if (bl->bl_type != NULL && bl->bl_subtype != NULL)
		debug_printf(", ");
	if (bl->bl_subtype != NULL)
		debug_printf("subtype '%s'", type_name(bl->bl_subtype));

	if (bl->bl_brace)
		debug_printf(", needs closing brace");
	if (bl->bl_array_of_unknown_size)
		debug_printf(", array of unknown size");
	if (bl->bl_seen_named_member)
		debug_printf(", seen named member");

	const type_t *eff_type = bl->bl_type != NULL
	    ? bl->bl_type : bl->bl_subtype;
	if (eff_type->t_tspec == STRUCT && bl->bl_next_member != NULL)
		debug_printf(", next member '%s'",
		    bl->bl_next_member->s_name);

	debug_printf(", remaining %d\n", bl->bl_remaining);
}
#else
#define brace_level_debug(bl) do { } while (false)
#endif

static void
brace_level_assert_struct_or_union(const struct brace_level *bl)
{
	lint_assert(is_struct_or_union(bl->bl_type->t_tspec));
}

static void
brace_level_assert_array(const struct brace_level *bl)
{
	lint_assert(bl->bl_type->t_tspec == ARRAY);
}

static type_t *
brace_level_subtype(struct brace_level *bl)
{

	if (bl->bl_subtype != NULL)
		return bl->bl_subtype;

	return bl->bl_type;
}

static void
brace_level_set_array_dimension(struct brace_level *bl, int dim)
{
	brace_level_assert_array(bl);

	debug_step("setting the array size to %d", dim);
	bl->bl_type->t_dim = dim;
	debug_indent();
	brace_level_debug(bl);
}

static void
brace_level_next_member(struct brace_level *bl)
{
	const sym_t *m;

	brace_level_assert_struct_or_union(bl);
	do {
		m = bl->bl_next_member = bl->bl_next_member->s_next;
		/* XXX: can this assertion be made to fail? */
		lint_assert(m != NULL);
	} while (m->s_bitfield && m->s_name == unnamed);

	debug_indent();
	brace_level_debug(bl);
}

static const sym_t *
brace_level_look_up_member(const struct brace_level *bl, const char *name)
{

	brace_level_assert_struct_or_union(bl);
	return look_up_member(bl->bl_type->t_str->sou_first_member, name);
}

static sym_t *
brace_level_look_up_first_member_named(struct brace_level *bl,
				       const char *name, int *count)
{
	sym_t *m;

	for (m = bl->bl_type->t_str->sou_first_member;
	     m != NULL; m = m->s_next) {
		if (is_unnamed_member(m))
			continue;
		if (strcmp(m->s_name, name) != 0)
			continue;
		(*count)++;
		break;
	}

	return m;
}

static sym_t *
brace_level_look_up_first_member_unnamed(struct brace_level *bl, int *count)
{
	sym_t *m;

	brace_level_assert_struct_or_union(bl);

	for (m = bl->bl_type->t_str->sou_first_member;
	     m != NULL; m = m->s_next) {
		if (is_unnamed_member(m))
			continue;
		/* XXX: What is this code for? */
		if (++(*count) == 1) {
			bl->bl_next_member = m;
			bl->bl_subtype = m->s_type;
		}
	}

	return m;
}

/* TODO: document me */
/* TODO: think of a better name than 'push' */
static bool
brace_level_push_array(struct brace_level *bl)
{
	brace_level_assert_array(bl);

	if (bl->bl_enclosing->bl_seen_named_member) {
		bl->bl_brace = true;
		debug_step("ARRAY, seen named member, needs closing brace");
	}

	if (is_incomplete(bl->bl_type) &&
	    bl->bl_enclosing->bl_enclosing != NULL) {
		/* initialization of an incomplete type */
		error(175);
		return false;
	}

	bl->bl_subtype = bl->bl_type->t_subt;
	bl->bl_array_of_unknown_size = is_incomplete(bl->bl_type);
	bl->bl_remaining = bl->bl_type->t_dim;
	debug_step("type '%s' remaining %d",
	    type_name(bl->bl_type), bl->bl_remaining);
	return true;
}

/*
 * If the removed element was a structure member, we must go
 * to the next structure member.
 *
 * XXX: Nothing should ever be "removed" at this point.
 *
 * TODO: think of a better name than 'pop'
 */
static void
brace_level_pop_item_unnamed(struct brace_level *bl)
{
	if (bl->bl_remaining > 0 && bl->bl_type->t_tspec == STRUCT &&
	    !bl->bl_seen_named_member) {
		brace_level_next_member(bl);
		bl->bl_subtype = bl->bl_next_member->s_type;
	}
}

static bool
brace_level_check_too_many_initializers(struct brace_level *bl)
{
	if (bl->bl_remaining > 0)
		return true;
	/*
	 * FIXME: even with named members, there can be too many initializers
	 */
	if (bl->bl_array_of_unknown_size || bl->bl_seen_named_member)
		return true;

	tspec_t t = bl->bl_type->t_tspec;
	if (t == ARRAY) {
		/* too many array initializers, expected %d */
		error(173, bl->bl_type->t_dim);
	} else if (is_struct_or_union(t)) {
		/* too many struct/union initializers */
		error(172);
	} else {
		/* too many initializers */
		error(174);
	}
	return false;
}

/* Extend an array of unknown size by one element */
static void
brace_level_extend_if_array_of_unknown_size(struct brace_level *bl)
{

	if (bl->bl_remaining != 0)
		return;
	/*
	 * XXX: According to the function name, there should be a 'return' if
	 * bl_array_of_unknown_size is false.  There's probably a test missing
	 * for that case.
	 */

	/*
	 * The only place where an incomplete array may appear is at the
	 * outermost aggregate bl of the object to be initialized.
	 */
	lint_assert(bl->bl_enclosing->bl_enclosing == NULL);
	lint_assert(bl->bl_type->t_tspec == ARRAY);

	debug_step("extending array of unknown size '%s'",
	    type_name(bl->bl_type));
	bl->bl_remaining = 1;
	bl->bl_type->t_dim++;
	setcomplete(bl->bl_type, true);

	debug_step("extended type is '%s'", type_name(bl->bl_type));
}


static struct initialization *
initialization_new(sym_t *sym)
{
	struct initialization *in = xcalloc(1, sizeof(*in));

	in->in_sym = sym;

	return in;
}

static void
initialization_free(struct initialization *in)
{
	struct brace_level *bl, *next;

	for (bl = in->in_brace_level; bl != NULL; bl = next) {
		next = bl->bl_enclosing;
		brace_level_free(bl);
	}

	free(in);
}

#ifdef DEBUG
/*
 * TODO: only call debug_initstack after each push/pop.
 */
static void
initialization_debug(const struct initialization *in)
{
	if (in->in_brace_level == NULL) {
		debug_step("no brace level in the current initialization");
		return;
	}

	size_t i = 0;
	for (const struct brace_level *bl = in->in_brace_level;
	     bl != NULL; bl = bl->bl_enclosing) {
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
 * Initialize the initialization stack by putting an entry for the object
 * which is to be initialized on it.
 *
 * TODO: merge into initialization_new if possible
 */
static void
initialization_init(struct initialization *in)
{
	if (in->in_err)
		return;

	debug_enter();

	/*
	 * If the type which is to be initialized is an incomplete array,
	 * it must be duplicated.
	 */
	if (in->in_sym->s_type->t_tspec == ARRAY && is_incomplete(in->in_sym->s_type))
		in->in_sym->s_type = duptyp(in->in_sym->s_type);
	/* TODO: does 'duptyp' create a memory leak? */

	in->in_brace_level = brace_level_new(NULL, in->in_sym->s_type, 1, NULL);

	initialization_debug(in);
	debug_leave();
}

static void
initialization_set_error(struct initialization *in)
{
	in->in_err = true;
}

/* TODO: document me */
/* TODO: think of a better name than 'push' */
static bool
initialization_push_struct_or_union(struct initialization *in)
{
	struct brace_level *bl = in->in_brace_level;
	int cnt;
	sym_t *m;

	if (is_incomplete(bl->bl_type)) {
		/* initialization of an incomplete type */
		error(175);
		initialization_set_error(in);
		return false;
	}

	cnt = 0;
	designation_debug(&in->in_designation);
	debug_step("lookup for '%s'%s",
	    type_name(bl->bl_type),
	    bl->bl_seen_named_member ? ", seen named member" : "");

	if (in->in_designation.dn_head != NULL)
		m = brace_level_look_up_first_member_named(bl,
		    in->in_designation.dn_head->dr_name, &cnt);
	else
		m = brace_level_look_up_first_member_unnamed(bl, &cnt);

	if (in->in_designation.dn_head != NULL) {
		if (m == NULL) {
			debug_step("pop struct");
			return true;
		}
		bl->bl_next_member = m;
		bl->bl_subtype = m->s_type;
		bl->bl_seen_named_member = true;
		debug_step("named member '%s'",
		    in->in_designation.dn_head->dr_name);
		designation_shift_level(&in->in_designation);
		cnt = bl->bl_type->t_tspec == STRUCT ? 2 : 1;
	}
	bl->bl_brace = true;
	debug_step("unnamed element with type '%s'%s",
	    type_name(
		bl->bl_type != NULL ? bl->bl_type : bl->bl_subtype),
	    bl->bl_brace ? ", needs closing brace" : "");
	if (cnt == 0) {
		/* cannot init. struct/union with no named member */
		error(179);
		initialization_set_error(in);
		return false;
	}
	bl->bl_remaining = bl->bl_type->t_tspec == STRUCT ? cnt : 1;
	return false;
}

static void
initialization_end_brace_level(struct initialization *in)
{
	struct brace_level *bl = in->in_brace_level;
	in->in_brace_level = bl->bl_enclosing;
	brace_level_free(bl);
}

/* TODO: document me */
/* TODO: think of a better name than 'push' */
static void
initialization_push(struct initialization *in)
{
	struct brace_level *bl;

	debug_enter();

	brace_level_extend_if_array_of_unknown_size(in->in_brace_level);

	bl = in->in_brace_level;
	lint_assert(bl->bl_remaining > 0);

	in->in_brace_level = brace_level_new(brace_level_subtype(bl), NULL, 0,
	    bl);
	lint_assert(in->in_brace_level->bl_type != NULL);
	lint_assert(in->in_brace_level->bl_type->t_tspec != FUNC);

again:
	bl = in->in_brace_level;

	debug_step("expecting type '%s'", type_name(bl->bl_type));
	lint_assert(bl->bl_type != NULL);
	switch (bl->bl_type->t_tspec) {
	case ARRAY:
		if (in->in_designation.dn_head != NULL) {
			debug_step("pop array, named member '%s'%s",
			    in->in_designation.dn_head->dr_name,
			    bl->bl_brace ? ", needs closing brace" : "");
			goto pop;
		}

		if (!brace_level_push_array(bl))
			initialization_set_error(in);
		break;

	case UNION:
		if (tflag)
			/* initialization of union is illegal in trad. C */
			warning(238);
		/* FALLTHROUGH */
	case STRUCT:
		if (initialization_push_struct_or_union(in))
			goto pop;
		break;
	default:
		if (in->in_designation.dn_head != NULL) {
			debug_step("pop scalar");
		pop:
			initialization_end_brace_level(in);
			goto again;
		}
		/* The initialization stack now expects a single scalar. */
		bl->bl_remaining = 1;
		break;
	}

	initialization_debug(in);
	debug_leave();
}

/* TODO: document me */
static void
initialization_pop_item_named(struct initialization *in, const char *name)
{
	struct brace_level *bl = in->in_brace_level;
	const sym_t *m;

	/*
	 * TODO: fix wording of the debug message; this doesn't seem to be
	 * related to initializing the named member.
	 */
	debug_step("initializing named member '%s'", name);

	if (!is_struct_or_union(bl->bl_type->t_tspec)) {
		/* syntax error '%s' */
		error(249, "named member must only be used with struct/union");
		initialization_set_error(in);
		return;
	}

	m = brace_level_look_up_member(bl, name);
	if (m == NULL) {
		/* TODO: add type information to the message */
		/* undefined struct/union member: %s */
		error(101, name);

		designation_shift_level(&in->in_designation);
		bl->bl_seen_named_member = true;
		return;
	}

	debug_step("found matching member");
	bl->bl_subtype = m->s_type;
	/* XXX: why ++? */
	bl->bl_remaining++;
	/* XXX: why is bl_seen_named_member not set? */
	designation_shift_level(&in->in_designation);
}

/* TODO: think of a better name than 'pop' */
static void
initialization_pop_item(struct initialization *in)
{
	struct brace_level *bl;

	debug_enter();

	bl = in->in_brace_level;
	debug_indent();
	debug_printf("popping: ");
	brace_level_debug(bl);

	in->in_brace_level = bl->bl_enclosing;
	brace_level_free(bl);
	bl = in->in_brace_level;
	lint_assert(bl != NULL);

	bl->bl_remaining--;
	lint_assert(bl->bl_remaining >= 0);
	debug_step("%d elements remaining", bl->bl_remaining);

	if (in->in_designation.dn_head != NULL && in->in_designation.dn_head->dr_name != NULL)
		initialization_pop_item_named(in, in->in_designation.dn_head->dr_name);
	else
		brace_level_pop_item_unnamed(bl);

	initialization_debug(in);
	debug_leave();
}

/*
 * Take all entries which cannot be used for further initializers from the
 * stack, but do this only if they do not require a closing brace.
 */
/* TODO: think of a better name than 'pop' */
static void
initialization_pop_nobrace(struct initialization *in)
{

	debug_enter();
	while (!in->in_brace_level->bl_brace &&
	       in->in_brace_level->bl_remaining == 0 &&
	       !in->in_brace_level->bl_array_of_unknown_size)
		initialization_pop_item(in);
	debug_leave();
}

/*
 * Process a '{' in an initializer by starting the initialization of the
 * nested data structure, with bl_type being the bl_subtype of the outer
 * initialization level.
 */
static void
initialization_next_brace(struct initialization *in)
{

	debug_enter();
	initialization_debug(in);

	if (!in->in_err &&
	    !brace_level_check_too_many_initializers(in->in_brace_level))
		initialization_set_error(in);

	if (!in->in_err)
		initialization_push(in);

	if (!in->in_err) {
		in->in_brace_level->bl_brace = true;
		designation_debug(&in->in_designation);
		if (in->in_brace_level->bl_type != NULL)
			debug_step("expecting type '%s'",
			    type_name(in->in_brace_level->bl_type));
	}

	initialization_debug(in);
	debug_leave();
}

static void
check_no_auto_aggregate(scl_t sclass, const struct brace_level *bl)
{
	if (!tflag)
		return;
	if (!(sclass == AUTO || sclass == REG))
		return;
	if (!(bl->bl_enclosing == NULL))
		return;
	if (is_scalar(bl->bl_subtype->t_tspec))
		return;

	/* no automatic aggregate initialization in trad. C */
	warning(188);
}

static void
initialization_lbrace(struct initialization *in)
{
	if (in->in_err)
		return;

	debug_enter();
	initialization_debug(in);

	check_no_auto_aggregate(in->in_sym->s_scl, in->in_brace_level);

	/*
	 * Remove all entries which cannot be used for further initializers
	 * and do not expect a closing brace.
	 */
	initialization_pop_nobrace(in);

	initialization_next_brace(in);

	initialization_debug(in);
	debug_leave();
}

/*
 * Process a '}' in an initializer by finishing the current level of the
 * initialization stack.
 *
 * Take all entries, including the first which requires a closing brace,
 * from the stack.
 */
static void
initialization_rbrace(struct initialization *in)
{
	bool brace;

	if (in->in_err)
		return;

	debug_enter();
	do {
		brace = in->in_brace_level->bl_brace;
		/* TODO: improve wording of the debug message */
		debug_step("loop brace=%d", brace);
		initialization_pop_item(in);
	} while (!brace);
	initialization_debug(in);
	debug_leave();
}

/*
 * A sub-object of an array is initialized using a designator.  This does not
 * have to be an array element directly, it can also be used to initialize
 * only a sub-object of the array element.
 *
 * C99 example: struct { int member[4]; } var = { [2] = 12345 };
 *
 * GNU example: struct { int member[4]; } var = { [1 ... 3] = 12345 };
 *
 * TODO: test the following initialization with an outer and an inner type:
 *
 * .deeply[0].nested = {
 *	.deeply[1].nested = {
 *		12345,
 *	},
 * }
 */
static void
initialization_add_designator_subscript(struct initialization *in,
					range_t range)
{
	struct brace_level *bl;

	debug_enter();
	if (range.lo == range.hi)
		debug_step("subscript is %zu", range.hi);
	else
		debug_step("subscript range is %zu ... %zu",
		    range.lo, range.hi);

	/* XXX: This call is wrong here, it must be somewhere else. */
	initialization_pop_nobrace(in);

	bl = in->in_brace_level;
	if (bl->bl_array_of_unknown_size) {
		/* No +1 here, extend_if_array_of_unknown_size will add it. */
		int auto_dim = (int)range.hi;
		if (auto_dim > bl->bl_type->t_dim)
			brace_level_set_array_dimension(bl, auto_dim);
	}

	debug_leave();
}

/* Initialize a character array or wchar_t array with a string literal. */
static bool
initialization_init_array_using_string(struct initialization *in, tnode_t *tn)
{
	struct brace_level *bl;
	strg_t	*strg;

	if (tn->tn_op != STRING)
		return false;

	debug_enter();

	bl = in->in_brace_level;
	strg = tn->tn_string;

	/*
	 * Check if we have an array type which can be initialized by
	 * the string.
	 */
	if (is_string_array(bl->bl_subtype, strg->st_tspec)) {
		debug_step("subtype is an array");

		/* Put the array at top of stack */
		initialization_push(in);
		bl = in->in_brace_level;

	} else if (is_string_array(bl->bl_type, strg->st_tspec)) {
		debug_step("type is an array");

		/*
		 * If the array is already partly initialized, we are
		 * wrong here.
		 */
		if (bl->bl_remaining != bl->bl_type->t_dim)
			goto nope;
	} else
		goto nope;

	if (bl->bl_array_of_unknown_size) {
		bl->bl_array_of_unknown_size = false;
		bl->bl_type->t_dim = (int)(strg->st_len + 1);
		setcomplete(bl->bl_type, true);
	} else {
		/*
		 * TODO: check for buffer overflow in the object to be
		 * initialized
		 */
		/* XXX: double-check for off-by-one error */
		if (bl->bl_type->t_dim < (int)strg->st_len) {
			/* non-null byte ignored in string initializer */
			warning(187);
		}

		/*
		 * TODO: C99 6.7.8p14 allows a string literal to be enclosed
		 * in optional redundant braces, just like scalars.  Add tests
		 * for this.
		 */
	}

	/* In every case the array is initialized completely. */
	bl->bl_remaining = 0;

	initialization_debug(in);
	debug_leave();
	return true;
nope:
	debug_leave();
	return false;
}

/*
 * Initialize a non-array object with automatic storage duration and only a
 * single initializer expression without braces by delegating to ASSIGN.
 */
static bool
initialization_init_using_assign(struct initialization *in, tnode_t *rn)
{
	tnode_t *ln, *tn;

	if (in->in_sym->s_type->t_tspec == ARRAY)
		return false;
	if (in->in_brace_level->bl_enclosing != NULL)
		return false;

	debug_step("handing over to ASSIGN");

	ln = new_name_node(in->in_sym, 0);
	ln->tn_type = tduptyp(ln->tn_type);
	ln->tn_type->t_const = false;

	tn = build(ASSIGN, ln, rn);
	expr(tn, false, false, false, false);

	/* XXX: why not clean up the initstack here already? */
	return true;
}

/* TODO: document me, or think of a better name */
static void
initialization_next_nobrace(struct initialization *in, tnode_t *tn)
{
	debug_enter();

	if (in->in_brace_level->bl_type == NULL &&
	    !is_scalar(in->in_brace_level->bl_subtype->t_tspec)) {
		/* {}-enclosed initializer required */
		error(181);
		/* XXX: maybe set initerr here */
	}

	if (!in->in_err &&
	    !brace_level_check_too_many_initializers(in->in_brace_level))
		initialization_set_error(in);

	while (!in->in_err) {
		struct brace_level *bl = in->in_brace_level;

		if (tn->tn_type->t_tspec == STRUCT &&
		    bl->bl_type == tn->tn_type &&
		    bl->bl_enclosing != NULL &&
		    bl->bl_enclosing->bl_enclosing != NULL) {
			bl->bl_brace = false;
			bl->bl_remaining = 1; /* the struct itself */
			break;
		}

		if (bl->bl_type != NULL &&
		    is_scalar(bl->bl_type->t_tspec))
			break;
		initialization_push(in);
	}

	initialization_debug(in);
	debug_leave();
}

static void
initialization_expr(struct initialization *in, tnode_t *tn)
{

	debug_enter();
	initialization_debug(in);
	designation_debug(&in->in_designation);
	debug_step("expr:");
	debug_node(tn, debug_ind + 1);

	if (in->in_err || tn == NULL)
		goto done;

	if (has_automatic_storage_duration(in->in_sym) &&
	    initialization_init_using_assign(in, tn))
		goto done;

	initialization_pop_nobrace(in);

	if (initialization_init_array_using_string(in, tn)) {
		debug_step("after initializing the string:");
		goto done_debug;
	}

	initialization_next_nobrace(in, tn);
	if (in->in_err || tn == NULL)
		goto done_debug;

	/* Using initsym here is better than nothing. */
	check_init_expr(in->in_brace_level->bl_type, in->in_sym, tn);

	in->in_brace_level->bl_remaining--;
	debug_step("%d elements remaining", in->in_brace_level->bl_remaining);

done_debug:
	initialization_debug(in);

done:
	while (in->in_designation.dn_head != NULL)
		designation_shift_level(&in->in_designation);

	debug_leave();
}

static struct initialization *
current_init(void)
{

	lint_assert(init != NULL);
	return init;
}

bool *
current_initerr(void)
{

	return &current_init()->in_err;
}

sym_t **
current_initsym(void)
{

	return &current_init()->in_sym;
}

void
begin_initialization(sym_t *sym)
{
	struct initialization *in;

	debug_step("begin initialization of '%s'", type_name(sym->s_type));
	in = initialization_new(sym);
	in->in_next = init;
	init = in;
}

void
end_initialization(void)
{
	struct initialization *in;

	in = init;
	init = init->in_next;
	initialization_free(in);
	debug_step("end initialization");
}

void
add_designator_member(sbuf_t *sb)
{

	designation_add(&current_init()->in_designation,
	    designator_new(sb->sb_name));
}

void
add_designator_subscript(range_t range)
{

	initialization_add_designator_subscript(current_init(), range);
}

void
initstack_init(void)
{

	initialization_init(current_init());
}

void
init_lbrace(void)
{

	initialization_lbrace(current_init());
}

void
init_using_expr(tnode_t *tn)
{

	initialization_expr(current_init(), tn);
}

void
init_rbrace(void)
{

	initialization_rbrace(current_init());
}
