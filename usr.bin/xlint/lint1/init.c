/*	$NetBSD: init.c,v 1.129 2021/03/25 21:36:41 rillig Exp $	*/

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
__RCSID("$NetBSD: init.c,v 1.129 2021/03/25 21:36:41 rillig Exp $");
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
 *		designator_push_name		for each '.member' before '='
 *		designator_push_subscript	for each '[123]' before '='
 *		init_using_expr			for each expression
 *		init_rbrace			for each '}'
 *	end_initialization
 *
 * The state of the current initialization is stored in initstk, a stack of
 * initstack_element, one element per type aggregate level.
 * XXX: Or is that "one element per brace level"?  C99 mandates in 6.7.8p17
 * that "each brace-enclosed initializer list has an associated current
 * object".
 *
 * Most of the time, the topmost level of initstk contains a scalar type, and
 * its remaining count toggles between 1 and 0.
 *
 * See also:
 *	C99 6.7.8 "Initialization"
 *	d_c99_init.c for more examples
 */


/*
 * Type of stack which is used for initialization of aggregate types.
 *
 * XXX: Since C99, the initializers can be listed in arbitrary order by using
 * designators to specify the sub-object to be initialized.  The member names
 * of non-leaf structs may thus appear repeatedly, as demonstrated in
 * d_init_pop_member.c.
 *
 * See C99 6.7.8, which spans 6 pages full of tricky details and carefully
 * selected examples.
 */
typedef	struct initstack_element {

	/*
	 * The type to be initialized at this level.
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
	type_t	*i_type;

	/*
	 * The type that will be initialized at the next initialization level,
	 * usually enclosed by another pair of braces.
	 *
	 * For an array, it is the element type, but without 'const'.
	 *
	 * For a struct or union type, it is one of the member types, but
	 * without 'const'.
	 *
	 * The outermost stack element has no i_type but nevertheless has
	 * i_subt.  For example, in 'int var = { 12345 }', initially there is
	 * an initstack_element with i_subt 'int'.  When the '{' is processed,
	 * an element with i_type 'int' is pushed to the stack.  When the
	 * corresponding '}' is processed, the inner element is popped again.
	 *
	 * During initialization, only the top 2 elements of the stack are
	 * looked at.
	 *
	 * XXX: Having i_subt here is the wrong approach, it should not be
	 * necessary at all; see i_type.
	 */
	type_t	*i_subt;

	/*
	 * Whether this level of the initializer requires a '}' to be
	 * completed.
	 *
	 * Multidimensional arrays do not need a closing brace to complete
	 * an inner array; for example, { 1, 2, 3, 4 } is a valid initializer
	 * for 'int arr[2][2]'.
	 *
	 * TODO: Do structs containing structs need a closing brace?
	 * TODO: Do arrays of structs need a closing brace after each struct?
	 *
	 * XXX: Double-check whether this is the correct approach at all; see
	 * i_type.
	 */
	bool i_brace: 1;

	/* Whether i_type is an array of unknown size. */
	bool i_array_of_unknown_size: 1;

	/*
	 * XXX: This feels wrong.  Whether or not there has been a named
	 * initializer (called 'designation' since C99) should not matter at
	 * all.  Even after an initializer with designation, counting of the
	 * remaining elements continues, see C99 6.7.8p17.
	 */
	bool i_seen_named_member: 1;

	/*
	 * For structs, the next member to be initialized by a designator-less
	 * initializer.
	 */
	sym_t *i_next_member;

	/* TODO: Add i_next_subscript for arrays. */

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
	 * It is probably clearer to use i_next_member and i_next_subscript
	 * for this purpose.
	 */
	int i_remaining;

	/*
	 * The initialization state of the enclosing data structure
	 * (struct, union, array).
	 *
	 * XXX: Or for a scalar, for the top-level element, or for expressions
	 * in redundant braces such as '{{{{ 0 }}}}' (not yet implemented as
	 * of 2021-03-25).
	 */
	struct initstack_element *i_enclosing;

} initstack_element;

/*
 * Leads the path to the sub-object to initialize using the expression.
 * Example designators are '.member' or '.member[123].member.member[1][1]'.
 *
 * See also: C99 6.7.8 "Initialization"
 */
typedef struct designator {
	const char *name;		/* for struct and union */
	/* TODO: add 'subscript' for arrays */
	struct designator *next;
} designator;

/*
 * See also: C99 6.7.8 "Initialization"
 */
typedef struct {
	designator *head;
	designator *tail;
} designation;

struct initialization {
	/*
	 * is set as soon as a fatal error occurred in the initialization.
	 * The effect is that the rest of the initialization is ignored
	 * (parsed by yacc, expression trees built, but no initialization
	 * takes place).
	 */
	bool	initerr;

	/* Pointer to the symbol which is to be initialized. */
	sym_t	*initsym;

	/* Points to the top element of the initialization stack. */
	initstack_element *initstk;

	/*
	 * The C99 designator, if any, for the current initialization
	 * expression.
	 */
	designation designation;

	struct initialization *next;
};

static struct initialization *init;


/* XXX: unnecessary prototype since it is not recursive */
static	bool	init_array_using_string(tnode_t *);


/* TODO: replace the following functions with current_init */

bool *
current_initerr(void)
{
	lint_assert(init != NULL);
	return &init->initerr;
}

sym_t **
current_initsym(void)
{
	lint_assert(init != NULL);
	return &init->initsym;
}

static designation *
current_designation_mod(void)
{
	lint_assert(init != NULL);
	return &init->designation;
}

static designation
current_designation(void)
{
	return *current_designation_mod();
}

static const initstack_element *
current_initstk(void)
{
	lint_assert(init != NULL);
	return init->initstk;
}

static initstack_element **
current_initstk_lvalue(void)
{
	lint_assert(init != NULL);
	return &init->initstk;
}

static void
free_initialization(struct initialization *in)
{
	initstack_element *el, *next;

	for (el = in->initstk; el != NULL; el = next) {
		next = el->i_enclosing;
		free(el);
	}

	free(in);
}

#define initerr		(*current_initerr())
#define initsym		(*current_initsym())
#define initstk		(current_initstk())
#define initstk_lvalue	(*current_initstk_lvalue())

#ifndef DEBUG

#define debug_printf(fmt, ...)	do { } while (false)
#define debug_indent()		do { } while (false)
#define debug_enter(a)		do { } while (false)
#define debug_step(fmt, ...)	do { } while (false)
#define debug_leave(a)		do { } while (false)
#define debug_designation()	do { } while (false)
#define debug_initstack_element(elem) do { } while (false)
#define debug_initstack()	do { } while (false)

#else

static int debug_ind = 0;

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

	printf("%*s", 2 * debug_ind, "");
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

static void
debug_designation(void)
{
	const designator *head = current_designation().head, *p;
	if (head == NULL)
		return;

	debug_indent();
	debug_printf("designation: ");
	for (p = head; p != NULL; p = p->next)
		debug_printf(".%s", p->name);
	debug_printf("\n");
}

/*
 * TODO: try whether a single-line output is more readable
 *
 * TODO: only log the top of the stack after each modifying operation
 *
 * TODO: wrap all write accesses to initstack_element in setter functions
 */
static void
debug_initstack_element(const initstack_element *elem)
{
	if (elem->i_type != NULL)
		debug_printf("type '%s'", type_name(elem->i_type));
	if (elem->i_type != NULL && elem->i_subt != NULL)
		debug_printf(", ");
	if (elem->i_subt != NULL)
		debug_printf("subtype '%s'", type_name(elem->i_subt));

	if (elem->i_brace)
		debug_printf(", needs closing brace");
	if (elem->i_array_of_unknown_size)
		debug_printf(", array of unknown size");
	if (elem->i_seen_named_member)
		debug_printf(", seen named member");

	const type_t *eff_type = elem->i_type != NULL
	    ? elem->i_type : elem->i_subt;
	if (eff_type->t_tspec == STRUCT && elem->i_next_member != NULL)
		debug_printf(", next member '%s'",
		    elem->i_next_member->s_name);

	debug_printf(", remaining %d\n", elem->i_remaining);
}

/*
 * TODO: only call debug_initstack after each push/pop.
 */
static void
debug_initstack(void)
{
	if (initstk == NULL) {
		debug_step("initstk is empty");
		return;
	}

	size_t i = 0;
	for (const initstack_element *elem = initstk;
	     elem != NULL; elem = elem->i_enclosing) {
		debug_indent();
		debug_printf("initstk[%zu]: ", i);
		debug_initstack_element(elem);
		i++;
	}
}

#define debug_enter() debug_enter(__func__)
#define debug_leave() debug_leave(__func__)

#endif


void
begin_initialization(sym_t *sym)
{
	struct initialization *curr_init;

	debug_step("begin initialization of '%s'", type_name(sym->s_type));
	curr_init = xcalloc(1, sizeof *curr_init);
	curr_init->next = init;
	init = curr_init;
	initsym = sym;
}

void
end_initialization(void)
{
	struct initialization *curr_init;

	curr_init = init;
	init = init->next;
	free_initialization(curr_init);
	debug_step("end initialization");
}

void
designator_push_name(sbuf_t *sb)
{
	designation *dd = current_designation_mod();

	designator *d = xcalloc(1, sizeof *d);
	d->name = sb->sb_name;

	if (dd->head != NULL) {
		dd->tail->next = d;
		dd->tail = d;
	} else {
		dd->head = d;
		dd->tail = d;
	}

	debug_designation();
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
void
designator_push_subscript(range_t range)
{
	debug_enter();
	debug_step("subscript range is %zu ... %zu", range.lo, range.hi);
	debug_initstack();
	debug_leave();
}

/* TODO: add support for array subscripts, not only named members */
static void
designator_shift_name(void)
{
	/* TODO: remove direct access to 'init' */
	lint_assert(init != NULL);
	lint_assert(init->designation.head != NULL);
	if (init->designation.head == init->designation.tail) {
		free(init->designation.head);
		init->designation.head = NULL;
		init->designation.tail = NULL;
	} else {
		designator *head = init->designation.head;
		init->designation.head = init->designation.head->next;
		free(head);
	}

	debug_designation();
}

/*
 * Initialize the initialization stack by putting an entry for the object
 * which is to be initialized on it.
 *
 * TODO: merge into begin_initialization
 */
void
initstack_init(void)
{
	initstack_element *istk;

	if (initerr)
		return;

	/* TODO: merge into init_using_expr */
	while (current_designation().head != NULL)
		designator_shift_name();

	debug_enter();

	/*
	 * If the type which is to be initialized is an incomplete array,
	 * it must be duplicated.
	 */
	if (initsym->s_type->t_tspec == ARRAY && is_incomplete(initsym->s_type))
		initsym->s_type = duptyp(initsym->s_type);
	/* TODO: does 'duptyp' create a memory leak? */

	istk = initstk_lvalue = xcalloc(1, sizeof (initstack_element));
	istk->i_subt = initsym->s_type;
	istk->i_remaining = 1;

	debug_initstack();
	debug_leave();
}

/* TODO: document me */
static void
initstack_pop_item_named_member(void)
{
	initstack_element *istk = initstk_lvalue;
	sym_t *m;

	/*
	 * TODO: fix wording of the debug message; this doesn't seem to be
	 * related to initializing the named member.
	 */
	debug_step("initializing named member '%s'",
	    current_designation().head->name);

	if (istk->i_type->t_tspec != STRUCT &&
	    istk->i_type->t_tspec != UNION) {
		/* syntax error '%s' */
		error(249, "named member must only be used with struct/union");
		initerr = true;
		return;
	}

	for (m = istk->i_type->t_str->sou_first_member;
	     m != NULL; m = m->s_next) {

		if (m->s_bitfield && m->s_name == unnamed)
			continue;

		if (strcmp(m->s_name, current_designation().head->name) == 0) {
			debug_step("found matching member");
			istk->i_subt = m->s_type;
			/* XXX: why ++? */
			istk->i_remaining++;
			/* XXX: why is i_seen_named_member not set? */
			designator_shift_name();
			return;
		}
	}

	/* undefined struct/union member: %s */
	error(101, current_designation().head->name);

	designator_shift_name();
	istk->i_seen_named_member = true;
}

/* TODO: think of a better name than 'pop' */
static void
initstack_pop_item_unnamed(void)
{
	initstack_element *istk = initstk_lvalue;
	sym_t *m;

	/*
	 * If the removed element was a structure member, we must go
	 * to the next structure member.
	 */
	if (istk->i_remaining > 0 && istk->i_type->t_tspec == STRUCT &&
	    !istk->i_seen_named_member) {
		do {
			m = istk->i_next_member =
			    istk->i_next_member->s_next;
			/* XXX: can this assertion be made to fail? */
			lint_assert(m != NULL);
			debug_step("pop %s", m->s_name);
		} while (m->s_bitfield && m->s_name == unnamed);
		/* XXX: duplicate code for skipping unnamed bit-fields */
		istk->i_subt = m->s_type;
	}
}

/* TODO: think of a better name than 'pop' */
static void
initstack_pop_item(void)
{
	initstack_element *istk;

	debug_enter();

	istk = initstk_lvalue;
	debug_indent();
	debug_printf("popping: ");
	debug_initstack_element(istk);

	initstk_lvalue = istk->i_enclosing;
	free(istk);
	istk = initstk_lvalue;
	lint_assert(istk != NULL);

	istk->i_remaining--;
	lint_assert(istk->i_remaining >= 0);
	debug_step("%d elements remaining", istk->i_remaining);

	if (current_designation().head != NULL)
		initstack_pop_item_named_member();
	else
		initstack_pop_item_unnamed();

	debug_initstack();
	debug_leave();
}

/*
 * Take all entries, including the first which requires a closing brace,
 * from the stack.
 */
static void
initstack_pop_brace(void)
{
	bool brace;

	debug_enter();
	debug_initstack();
	do {
		brace = initstk->i_brace;
		/* TODO: improve wording of the debug message */
		debug_step("loop brace=%d", brace);
		initstack_pop_item();
	} while (!brace);
	debug_initstack();
	debug_leave();
}

/*
 * Take all entries which cannot be used for further initializers from the
 * stack, but do this only if they do not require a closing brace.
 */
/* TODO: think of a better name than 'pop' */
static void
initstack_pop_nobrace(void)
{

	debug_enter();
	while (!initstk->i_brace && initstk->i_remaining == 0 &&
	       !initstk->i_array_of_unknown_size)
		initstack_pop_item();
	debug_leave();
}

/* Extend an array of unknown size by one element */
static void
extend_if_array_of_unknown_size(void)
{
	initstack_element *istk = initstk_lvalue;

	if (istk->i_remaining != 0)
		return;

	/*
	 * The only place where an incomplete array may appear is at the
	 * outermost aggregate level of the object to be initialized.
	 */
	lint_assert(istk->i_enclosing->i_enclosing == NULL);
	lint_assert(istk->i_type->t_tspec == ARRAY);

	debug_step("extending array of unknown size '%s'",
	    type_name(istk->i_type));
	istk->i_remaining = 1;
	istk->i_type->t_dim++;
	setcomplete(istk->i_type, true);

	debug_step("extended type is '%s'", type_name(istk->i_type));
}

/* TODO: document me */
/* TODO: think of a better name than 'push' */
static void
initstack_push_array(void)
{
	initstack_element *istk = initstk_lvalue;

	if (istk->i_enclosing->i_seen_named_member) {
		istk->i_brace = true;
		debug_step("ARRAY%s%s",
		    istk->i_brace ? ", needs closing brace" : "",
		    istk->i_enclosing->i_seen_named_member
			? ", seen named member" : "");
	}

	if (is_incomplete(istk->i_type) &&
	    istk->i_enclosing->i_enclosing != NULL) {
		/* initialization of an incomplete type */
		error(175);
		initerr = true;
		return;
	}

	istk->i_subt = istk->i_type->t_subt;
	istk->i_array_of_unknown_size = is_incomplete(istk->i_type);
	istk->i_remaining = istk->i_type->t_dim;
	debug_designation();
	debug_step("type '%s' remaining %d",
	    type_name(istk->i_type), istk->i_remaining);
}

/* TODO: document me */
/* TODO: think of a better name than 'push' */
static bool
initstack_push_struct_or_union(void)
{
	/*
	 * TODO: remove unnecessary 'const' for variables in functions that
	 * fit on a single screen.  Keep it for larger functions.
	 */
	initstack_element *istk = initstk_lvalue;
	int cnt;
	sym_t *m;

	if (is_incomplete(istk->i_type)) {
		/* initialization of an incomplete type */
		error(175);
		initerr = true;
		return false;
	}

	cnt = 0;
	debug_designation();
	debug_step("lookup for '%s'%s",
	    type_name(istk->i_type),
	    istk->i_seen_named_member ? ", seen named member" : "");

	for (m = istk->i_type->t_str->sou_first_member;
	     m != NULL; m = m->s_next) {
		if (m->s_bitfield && m->s_name == unnamed)
			continue;
		/*
		 * TODO: split into separate functions:
		 *
		 * look_up_array_next
		 * look_up_array_designator
		 * look_up_struct_next
		 * look_up_struct_designator
		 */
		if (current_designation().head != NULL) {
			/* XXX: this log entry looks unnecessarily verbose */
			debug_step("have member '%s', want member '%s'",
			    m->s_name, current_designation().head->name);
			if (strcmp(m->s_name,
			    current_designation().head->name) == 0) {
				cnt++;
				break;
			} else
				continue;
		}
		if (++cnt == 1) {
			istk->i_next_member = m;
			istk->i_subt = m->s_type;
		}
	}

	if (current_designation().head != NULL) {
		if (m == NULL) {
			debug_step("pop struct");
			return true;
		}
		istk->i_next_member = m;
		istk->i_subt = m->s_type;
		istk->i_seen_named_member = true;
		debug_step("named member '%s'",
		    current_designation().head->name);
		designator_shift_name();
		cnt = istk->i_type->t_tspec == STRUCT ? 2 : 1;
	}
	istk->i_brace = true;
	debug_step("unnamed element with type '%s'%s",
	    type_name(istk->i_type != NULL ? istk->i_type : istk->i_subt),
	    istk->i_brace ? ", needs closing brace" : "");
	if (cnt == 0) {
		/* cannot init. struct/union with no named member */
		error(179);
		initerr = true;
		return false;
	}
	istk->i_remaining = istk->i_type->t_tspec == STRUCT ? cnt : 1;
	return false;
}

/* TODO: document me */
/* TODO: think of a better name than 'push' */
static void
initstack_push(void)
{
	initstack_element *istk, *inxt;

	debug_enter();

	extend_if_array_of_unknown_size();

	istk = initstk_lvalue;
	lint_assert(istk->i_remaining > 0);
	lint_assert(istk->i_type == NULL || !is_scalar(istk->i_type->t_tspec));

	initstk_lvalue = xcalloc(1, sizeof (initstack_element));
	initstk_lvalue->i_enclosing = istk;
	initstk_lvalue->i_type = istk->i_subt;
	lint_assert(initstk_lvalue->i_type->t_tspec != FUNC);

again:
	istk = initstk_lvalue;

	debug_step("expecting type '%s'", type_name(istk->i_type));
	lint_assert(istk->i_type != NULL);
	switch (istk->i_type->t_tspec) {
	case ARRAY:
		if (current_designation().head != NULL) {
			debug_step("pop array, named member '%s'%s",
			    current_designation().head->name,
			    istk->i_brace ? ", needs closing brace" : "");
			goto pop;
		}

		initstack_push_array();
		break;

	case UNION:
		if (tflag)
			/* initialization of union is illegal in trad. C */
			warning(238);
		/* FALLTHROUGH */
	case STRUCT:
		if (initstack_push_struct_or_union())
			goto pop;
		break;
	default:
		if (current_designation().head != NULL) {
			debug_step("pop scalar");
	pop:
			/* TODO: extract this into end_initializer_level */
			inxt = initstk->i_enclosing;
			free(istk);
			initstk_lvalue = inxt;
			goto again;
		}
		/* The initialization stack now expects a single scalar. */
		istk->i_remaining = 1;
		break;
	}

	debug_initstack();
	debug_leave();
}

static void
check_too_many_initializers(void)
{

	const initstack_element *istk = initstk;
	if (istk->i_remaining > 0)
		return;
	/*
	 * FIXME: even with named members, there can be too many initializers
	 */
	if (istk->i_array_of_unknown_size || istk->i_seen_named_member)
		return;

	tspec_t t = istk->i_type->t_tspec;
	if (t == ARRAY) {
		/* too many array initializers, expected %d */
		error(173, istk->i_type->t_dim);
	} else if (t == STRUCT || t == UNION) {
		/* too many struct/union initializers */
		error(172);
	} else {
		/* too many initializers */
		error(174);
	}
	initerr = true;
}

/*
 * Process a '{' in an initializer by starting the initialization of the
 * nested data structure, with i_type being the i_subt of the outer
 * initialization level.
 */
static void
initstack_next_brace(void)
{

	debug_enter();
	debug_initstack();

	if (initstk->i_type != NULL && is_scalar(initstk->i_type->t_tspec)) {
		/* invalid initializer type %s */
		error(176, type_name(initstk->i_type));
		initerr = true;
	}
	if (!initerr)
		check_too_many_initializers();
	if (!initerr)
		initstack_push();
	if (!initerr) {
		initstk_lvalue->i_brace = true;
		debug_designation();
		debug_step("expecting type '%s'",
		    type_name(initstk->i_type != NULL ? initstk->i_type
			: initstk->i_subt));
	}

	debug_initstack();
	debug_leave();
}

/* TODO: document me, or think of a better name */
static void
initstack_next_nobrace(tnode_t *tn)
{
	debug_enter();

	if (initstk->i_type == NULL && !is_scalar(initstk->i_subt->t_tspec)) {
		/* {}-enclosed initializer required */
		error(181);
		/* XXX: maybe set initerr here */
	}

	if (!initerr)
		check_too_many_initializers();

	while (!initerr) {
		initstack_element *istk = initstk_lvalue;

		if (tn->tn_type->t_tspec == STRUCT &&
		    istk->i_type == tn->tn_type &&
		    istk->i_enclosing != NULL &&
		    istk->i_enclosing->i_enclosing != NULL) {
			istk->i_brace = false;
			istk->i_remaining = 1; /* the struct itself */
			break;
		}

		if ((istk->i_type != NULL && is_scalar(istk->i_type->t_tspec)))
			break;
		initstack_push();
	}

	debug_initstack();
	debug_leave();
}

/* TODO: document me */
void
init_lbrace(void)
{
	if (initerr)
		return;

	debug_enter();
	debug_initstack();

	if ((initsym->s_scl == AUTO || initsym->s_scl == REG) &&
	    initstk->i_enclosing == NULL) {
		if (tflag && !is_scalar(initstk->i_subt->t_tspec))
			/* no automatic aggregate initialization in trad. C */
			warning(188);
	}

	/*
	 * Remove all entries which cannot be used for further initializers
	 * and do not expect a closing brace.
	 */
	initstack_pop_nobrace();

	initstack_next_brace();

	debug_initstack();
	debug_leave();
}

/*
 * Process a '}' in an initializer by finishing the current level of the
 * initialization stack.
 */
void
init_rbrace(void)
{
	if (initerr)
		return;

	debug_enter();
	initstack_pop_brace();
	debug_leave();
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
check_non_constant_initializer(const tnode_t *tn, scl_t sclass)
{
	/* TODO: rename CON to CONSTANT to avoid ambiguity with CONVERT */
	if (tn == NULL || tn->tn_op == CON)
		return;

	sym_t *sym;
	ptrdiff_t offs;
	if (constant_addr(tn, &sym, &offs))
		return;

	if (sclass == AUTO || sclass == REG) {
		/* non-constant initializer */
		c99ism(177);
	} else {
		/* non-constant initializer */
		error(177);
	}
}

/*
 * Initialize a non-array object with automatic storage duration and only a
 * single initializer expression without braces by delegating to ASSIGN.
 */
static bool
init_using_assign(tnode_t *rn)
{
	tnode_t *ln, *tn;

	if (initsym->s_type->t_tspec == ARRAY)
		return false;
	if (initstk->i_enclosing != NULL)
		return false;

	debug_step("handing over to ASSIGN");

	ln = new_name_node(initsym, 0);
	ln->tn_type = tduptyp(ln->tn_type);
	ln->tn_type->t_const = false;

	tn = build(ASSIGN, ln, rn);
	expr(tn, false, false, false, false);

	/* XXX: why not clean up the initstack here already? */
	return true;
}

static void
check_init_expr(tnode_t *tn, scl_t sclass)
{
	tnode_t *ln;
	tspec_t lt, rt;
	struct mbl *tmem;

	/* Create a temporary node for the left side. */
	ln = tgetblk(sizeof (tnode_t));
	ln->tn_op = NAME;
	ln->tn_type = tduptyp(initstk->i_type);
	ln->tn_type->t_const = false;
	ln->tn_lvalue = true;
	ln->tn_sym = initsym;		/* better than nothing */

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
	if (lt != rt || (initstk->i_type->t_bitfield && tn->tn_op == CON))
		tn = convert(INIT, 0, initstk->i_type, tn);

	check_non_constant_initializer(tn, sclass);
}

void
init_using_expr(tnode_t *tn)
{
	scl_t	sclass;

	debug_enter();
	debug_initstack();
	debug_designation();
	debug_step("expr:");
	debug_node(tn, debug_ind + 1);

	if (initerr || tn == NULL)
		goto done;

	sclass = initsym->s_scl;
	if ((sclass == AUTO || sclass == REG) && init_using_assign(tn))
		goto done;

	initstack_pop_nobrace();

	if (init_array_using_string(tn)) {
		debug_step("after initializing the string:");
		/* XXX: why not clean up the initstack here already? */
		goto done_initstack;
	}

	initstack_next_nobrace(tn);
	if (initerr || tn == NULL)
		goto done_initstack;

	initstk_lvalue->i_remaining--;
	debug_step("%d elements remaining", initstk->i_remaining);

	check_init_expr(tn, sclass);

done_initstack:
	debug_initstack();
done:
	debug_leave();
}


/* Initialize a character array or wchar_t array with a string literal. */
static bool
init_array_using_string(tnode_t *tn)
{
	tspec_t	t;
	initstack_element *istk;
	int	len;
	strg_t	*strg;

	if (tn->tn_op != STRING)
		return false;

	debug_enter();
	debug_initstack();

	istk = initstk_lvalue;
	strg = tn->tn_string;

	/*
	 * Check if we have an array type which can be initialized by
	 * the string.
	 */
	if (istk->i_subt != NULL && istk->i_subt->t_tspec == ARRAY) {
		debug_step("subt array");
		t = istk->i_subt->t_subt->t_tspec;
		if (!((strg->st_tspec == CHAR &&
		       (t == CHAR || t == UCHAR || t == SCHAR)) ||
		      (strg->st_tspec == WCHAR && t == WCHAR))) {
			debug_leave();
			return false;
		}
		/* XXX: duplicate code, see below */

		/* Put the array at top of stack */
		initstack_push();
		istk = initstk_lvalue;


		/* TODO: what if both i_type and i_subt are ARRAY? */

	} else if (istk->i_type != NULL && istk->i_type->t_tspec == ARRAY) {
		debug_step("type array");
		t = istk->i_type->t_subt->t_tspec;
		if (!((strg->st_tspec == CHAR &&
		       (t == CHAR || t == UCHAR || t == SCHAR)) ||
		      (strg->st_tspec == WCHAR && t == WCHAR))) {
			debug_leave();
			return false;
		}
		/* XXX: duplicate code, see above */

		/*
		 * TODO: is this really not needed in the branch above this
		 * one?
		 */
		/*
		 * If the array is already partly initialized, we are
		 * wrong here.
		 */
		if (istk->i_remaining != istk->i_type->t_dim) {
			debug_leave();
			return false;
		}
	} else {
		debug_leave();
		return false;
	}

	/* Get length without trailing NUL character. */
	len = strg->st_len;

	if (istk->i_array_of_unknown_size) {
		istk->i_array_of_unknown_size = false;
		istk->i_type->t_dim = len + 1;
		setcomplete(istk->i_type, true);
	} else {
		/*
		 * TODO: check for buffer overflow in the object to be
		 * initialized
		 */
		/* XXX: double-check for off-by-one error */
		if (istk->i_type->t_dim < len) {
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
	istk->i_remaining = 0;

	debug_initstack();
	debug_leave();
	return true;
}
