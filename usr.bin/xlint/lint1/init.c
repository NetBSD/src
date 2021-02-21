/*	$NetBSD: init.c,v 1.79 2021/02/21 09:24:32 rillig Exp $	*/

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
__RCSID("$NetBSD: init.c,v 1.79 2021/02/21 09:24:32 rillig Exp $");
#endif

#include <stdlib.h>
#include <string.h>

#include "lint1.h"


/*
 * Type of stack which is used for initialisation of aggregate types.
 *
 * XXX: Since C99, a stack is an inappropriate data structure for modelling
 * an initialization, since the designators don't have to be listed in a
 * particular order and can designate parts of sub-objects.  The member names
 * of non-leaf structs may thus appear repeatedly, as demonstrated in
 * d_init_pop_member.c.
 *
 * XXX: During initialization, there may be members of the top-level struct
 * that are partially initialized.  The simple i_remaining cannot model this
 * appropriately.
 *
 * See C99 6.7.8, which spans 6 pages full of tricky details and carefully
 * selected examples.
 */
typedef	struct initstack_element {

	/* XXX: Why is i_type often null? */
	type_t	*i_type;		/* type of initialisation */
	type_t	*i_subt;		/* type of next level */

	/* need '}' for pop; XXX: explain this */
	bool i_brace: 1;
	bool i_array_of_unknown_size: 1;
	bool i_seen_named_member: 1;

	/*
	 * For structs, the next member to be initialized by an initializer
	 * without an optional designator.
	 */
	sym_t *i_current_object;

	/*
	 * The number of remaining elements.
	 *
	 * XXX: for scalars?
	 * XXX: for structs?
	 * XXX: for unions?
	 * XXX: for arrays?
	 */
	int i_remaining;

	/*
	 * The initialization state of the enclosing data structure
	 * (struct, union, array).
	 */
	struct initstack_element *i_enclosing;
} initstack_element;

/*
 * The names for a nested C99 initialization designator, in a circular list.
 *
 * Example:
 *	struct stat st = {
 *		.st_size = 123,
 *		.st_mtim.tv_sec = 45,
 *		.st_mtim.tv_nsec
 *	};
 *
 *	During initialization, this list first contains ["st_size"], then
 *	["st_mtim", "tv_sec"], then ["st_mtim", "tv_nsec"].
 */
typedef struct namlist {
	const char *n_name;
	struct namlist *n_prev;
	struct namlist *n_next;
} namlist_t;


/*
 * initerr is set as soon as a fatal error occurred in an initialisation.
 * The effect is that the rest of the initialisation is ignored (parsed
 * by yacc, expression trees built, but no initialisation takes place).
 */
bool	initerr;

/* Pointer to the symbol which is to be initialized. */
sym_t	*initsym;

/* Points to the top element of the initialisation stack. */
initstack_element *initstk;

/* Points to a c9x named member; */
namlist_t	*namedmem = NULL;


static	bool	initstack_string(tnode_t *);

#ifndef DEBUG
#define debug_printf(fmt, ...)	do { } while (false)
#define debug_indent()		do { } while (false)
#define debug_enter(a)		do { } while (false)
#define debug_step(fmt, ...)	do { } while (false)
#define debug_leave(a)		do { } while (false)
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

#define debug_enter() debug_enter(__func__)
#define debug_leave() debug_leave(__func__)

#endif

void
push_member(sbuf_t *sb)
{
	namlist_t *nam = xcalloc(1, sizeof (namlist_t));
	nam->n_name = sb->sb_name;

	debug_step("%s: '%s' %p", __func__, nam->n_name, nam);

	if (namedmem == NULL) {
		/*
		 * XXX: Why is this a circular list?
		 * XXX: Why is this a doubly-linked list?
		 * A simple stack should suffice.
		 */
		nam->n_prev = nam->n_next = nam;
		namedmem = nam;
	} else {
		namedmem->n_prev->n_next = nam;
		nam->n_prev = namedmem->n_prev;
		nam->n_next = namedmem;
		namedmem->n_prev = nam;
	}
}

static void
pop_member(void)
{
	debug_step("%s: %s %p", __func__, namedmem->n_name, namedmem);
	if (namedmem->n_next == namedmem) {
		free(namedmem);
		namedmem = NULL;
	} else {
		namlist_t *nam = namedmem;
		namedmem = namedmem->n_next;
		nam->n_prev->n_next = nam->n_next;
		nam->n_next->n_prev = nam->n_prev;
		free(nam);
	}
}

#ifdef DEBUG
static void
debug_named_member(void)
{
	namlist_t *name;

	if (namedmem == NULL)
		return;
	name = namedmem;
	debug_indent();
	debug_printf("named member:");
	do {
		debug_printf(" %s", name->n_name);
		name = name->n_next;
	} while (name != namedmem);
	debug_printf("\n");
}
#else
#define debug_named_member()	do { } while (false)
#endif

#ifdef DEBUG
static void
debug_initstack_element(const initstack_element *elem)
{
	if (elem->i_type != NULL)
		debug_step("  i_type           = %s", type_name(elem->i_type));
	if (elem->i_subt != NULL)
		debug_step("  i_subt           = %s", type_name(elem->i_subt));

	if (elem->i_brace)
		debug_step("  i_brace");
	if (elem->i_array_of_unknown_size)
		debug_step("  i_array_of_unknown_size");
	if (elem->i_seen_named_member)
		debug_step("  i_seen_named_member");

	const type_t *eff_type = elem->i_type != NULL
	    ? elem->i_type : elem->i_subt;
	if (eff_type->t_tspec == STRUCT && elem->i_current_object != NULL)
		debug_step("  i_current_object = %s",
		    elem->i_current_object->s_name);

	debug_step("  i_remaining      = %d", elem->i_remaining);
}

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
		debug_step("initstk[%zu]:", i);
		debug_initstack_element(elem);
		i++;
	}
}
#else
#define debug_initstack()	do { } while (false)
#endif

/*
 * Initialize the initialisation stack by putting an entry for the object
 * which is to be initialized on it.
 */
void
initstack_init(void)
{
	initstack_element *istk;

	if (initerr)
		return;

	/* free memory used in last initialisation */
	while ((istk = initstk) != NULL) {
		initstk = istk->i_enclosing;
		free(istk);
	}

	debug_enter();

	/*
	 * If the type which is to be initialized is an incomplete array,
	 * it must be duplicated.
	 */
	if (initsym->s_type->t_tspec == ARRAY && is_incomplete(initsym->s_type))
		initsym->s_type = duptyp(initsym->s_type);

	istk = initstk = xcalloc(1, sizeof (initstack_element));
	istk->i_subt = initsym->s_type;
	istk->i_remaining = 1;

	debug_initstack();
	debug_leave();
}

static void
initstack_pop_item(void)
{
	initstack_element *istk;
	sym_t	*m;

	debug_enter();

	istk = initstk;
	debug_step("pop type=%s, brace=%d remaining=%d named=%d",
	    type_name(istk->i_type != NULL ? istk->i_type : istk->i_subt),
	    istk->i_brace, istk->i_remaining, istk->i_seen_named_member);

	initstk = istk->i_enclosing;
	free(istk);
	istk = initstk;
	lint_assert(istk != NULL);

	debug_step("top type=%s, brace=%d remaining=%d named=%d",
	    type_name(istk->i_type != NULL ? istk->i_type : istk->i_subt),
	    istk->i_brace, istk->i_remaining, istk->i_seen_named_member);

	istk->i_remaining--;
	lint_assert(istk->i_remaining >= 0);

	debug_step("top remaining=%d rhs.name=%s",
	    istk->i_remaining, namedmem != NULL ? namedmem->n_name : "*null*");

	if (istk->i_remaining >= 0 && namedmem != NULL) {

		debug_step("named remaining=%d type=%s, rhs.name=%s",
		    istk->i_remaining, type_name(istk->i_type),
		    namedmem->n_name);

		for (m = istk->i_type->t_str->sou_first_member;
		     m != NULL; m = m->s_next) {
			debug_step("pop lhs.name=%s rhs.name=%s",
			    m->s_name, namedmem->n_name);
			if (m->s_bitfield && m->s_name == unnamed)
				continue;
			if (strcmp(m->s_name, namedmem->n_name) == 0) {
				istk->i_subt = m->s_type;
				istk->i_remaining++;
				pop_member();
				debug_initstack();
				debug_leave();
				return;
			}
		}
		/* undefined struct/union member: %s */
		error(101, namedmem->n_name);
		debug_step("end rhs.name=%s", namedmem->n_name);
		pop_member();
		istk->i_seen_named_member = true;
		debug_initstack();
		debug_leave();
		return;
	}
	/*
	 * If the removed element was a structure member, we must go
	 * to the next structure member.
	 */
	if (istk->i_remaining > 0 && istk->i_type->t_tspec == STRUCT &&
	    !istk->i_seen_named_member) {
		do {
			m = istk->i_current_object =
			    istk->i_current_object->s_next;
			lint_assert(m != NULL);
			debug_step("pop %s", m->s_name);
		} while (m->s_bitfield && m->s_name == unnamed);
		istk->i_subt = m->s_type;
	}
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
static void
initstack_pop_nobrace(void)
{

	debug_enter();
	debug_initstack();
	while (!initstk->i_brace && initstk->i_remaining == 0 &&
	       !initstk->i_array_of_unknown_size)
		initstack_pop_item();
	debug_initstack();
	debug_leave();
}

static void
initstack_push(void)
{
	initstack_element *istk, *inxt;
	int	cnt;
	sym_t	*m;

	debug_enter();
	debug_initstack();

	istk = initstk;

	/* Extend an incomplete array type by one element */
	if (istk->i_remaining == 0) {
		debug_step("(extend) %s", type_name(istk->i_type));
		/*
		 * Inside of other aggregate types must not be an incomplete
		 * type.
		 */
		lint_assert(istk->i_enclosing->i_enclosing == NULL);
		istk->i_remaining = 1;
		lint_assert(istk->i_type->t_tspec == ARRAY);
		istk->i_type->t_dim++;
		setcomplete(istk->i_type, true);
	}

	lint_assert(istk->i_remaining > 0);
	lint_assert(istk->i_type == NULL || !is_scalar(istk->i_type->t_tspec));

	initstk = xcalloc(1, sizeof (initstack_element));
	initstk->i_enclosing = istk;
	initstk->i_type = istk->i_subt;
	lint_assert(initstk->i_type->t_tspec != FUNC);

again:
	istk = initstk;

	debug_step("typename %s", type_name(istk->i_type));
	switch (istk->i_type->t_tspec) {
	case ARRAY:
		if (namedmem != NULL) {
			debug_step("ARRAY %s brace=%d",
			    namedmem->n_name, istk->i_brace);
			goto pop;
		} else if (istk->i_enclosing->i_seen_named_member) {
			istk->i_brace = true;
			debug_step("ARRAY brace=%d, namedmem=%d",
			    istk->i_brace,
			    istk->i_enclosing->i_seen_named_member);
		}

		if (is_incomplete(istk->i_type) &&
		    istk->i_enclosing->i_enclosing != NULL) {
			/* initialisation of an incomplete type */
			error(175);
			initerr = true;
			debug_initstack();
			debug_leave();
			return;
		}
		istk->i_subt = istk->i_type->t_subt;
		istk->i_array_of_unknown_size = is_incomplete(istk->i_type);
		istk->i_remaining = istk->i_type->t_dim;
		debug_step("elements array %s[%d] %s",
		    type_name(istk->i_subt), istk->i_remaining,
		    namedmem != NULL ? namedmem->n_name : "*none*");
		break;
	case UNION:
		if (tflag)
			/* initialisation of union is illegal in trad. C */
			warning(238);
		/* FALLTHROUGH */
	case STRUCT:
		if (is_incomplete(istk->i_type)) {
			/* initialisation of an incomplete type */
			error(175);
			initerr = true;
			debug_initstack();
			debug_leave();
			return;
		}
		cnt = 0;
		debug_step("lookup type=%s, name=%s named=%d",
		    type_name(istk->i_type),
		    namedmem != NULL ? namedmem->n_name : "*none*",
		    istk->i_seen_named_member);
		for (m = istk->i_type->t_str->sou_first_member;
		     m != NULL; m = m->s_next) {
			if (m->s_bitfield && m->s_name == unnamed)
				continue;
			if (namedmem != NULL) {
				debug_step("named lhs.member=%s, rhs.member=%s",
				    m->s_name, namedmem->n_name);
				if (strcmp(m->s_name, namedmem->n_name) == 0) {
					cnt++;
					break;
				} else
					continue;
			}
			if (++cnt == 1) {
				istk->i_current_object = m;
				istk->i_subt = m->s_type;
			}
		}
		if (namedmem != NULL) {
			if (m == NULL) {
				debug_step("pop struct");
				goto pop;
			}
			istk->i_current_object = m;
			istk->i_subt = m->s_type;
			istk->i_seen_named_member = true;
			debug_step("named name=%s", namedmem->n_name);
			pop_member();
			cnt = istk->i_type->t_tspec == STRUCT ? 2 : 1;
		}
		istk->i_brace = true;
		debug_step("unnamed type=%s, brace=%d",
		    type_name(
			istk->i_type != NULL ? istk->i_type : istk->i_subt),
		    istk->i_brace);
		if (cnt == 0) {
			/* cannot init. struct/union with no named member */
			error(179);
			initerr = true;
			debug_initstack();
			debug_leave();
			return;
		}
		istk->i_remaining = istk->i_type->t_tspec == STRUCT ? cnt : 1;
		break;
	default:
		if (namedmem != NULL) {
			debug_step("pop");
	pop:
			inxt = initstk->i_enclosing;
			free(istk);
			initstk = inxt;
			goto again;
		}
		istk->i_remaining = 1;
		break;
	}

	debug_initstack();
	debug_leave();
}

static void
initstack_check_too_many(void)
{
	initstack_element *istk;

	istk = initstk;

	/*
	 * If a closing brace is expected we have at least one initializer
	 * too much.
	 */
	if (istk->i_remaining == 0 && !istk->i_array_of_unknown_size &&
	    !istk->i_seen_named_member) {
		switch (istk->i_type->t_tspec) {
		case ARRAY:
			/* too many array initializers, expected %d */
			error(173, istk->i_type->t_dim);
			break;
		case STRUCT:
		case UNION:
			/* too many struct/union initializers */
			error(172);
			break;
		default:
			/* too many initializers */
			error(174);
			break;
		}
		initerr = true;
	}
}

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
		initstack_check_too_many();
	if (!initerr)
		initstack_push();
	if (!initerr) {
		initstk->i_brace = true;
		debug_step("%p %s", namedmem, type_name(
		    initstk->i_type != NULL ? initstk->i_type
			: initstk->i_subt));
	}

	debug_initstack();
	debug_leave();
}

static void
initstack_next_nobrace(void)
{
	debug_enter();
	debug_initstack();

	if (initstk->i_type == NULL && !is_scalar(initstk->i_subt->t_tspec)) {
		/* {}-enclosed initializer required */
		error(181);
	}

	/*
	 * Make sure an entry with a scalar type is at the top of the stack.
	 *
	 * FIXME: Since C99 an initializer for an object with automatic
	 *  storage need not be a constant expression anymore.  It is
	 *  perfectly fine to initialize a struct with a struct expression,
	 *  see d_struct_init_nested.c for a demonstration.
	 */
	if (!initerr)
		initstack_check_too_many();
	while (!initerr) {
		if ((initstk->i_type != NULL &&
		     is_scalar(initstk->i_type->t_tspec)))
			break;
		initstack_push();
	}

	debug_initstack();
	debug_leave();
}

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

void
init_rbrace(void)
{
	if (initerr)
		return;

	debug_enter();
	debug_initstack();

	initstack_pop_brace();

	debug_initstack();
	debug_leave();
}

void
init_using_expr(tnode_t *tn)
{
	ptrdiff_t offs;
	sym_t	*sym;
	tspec_t	lt, rt;
	tnode_t	*ln;
	struct	mbl *tmem;
	scl_t	sc;

	debug_enter();
	debug_named_member();
	debug_node(tn, debug_ind);
	debug_initstack();

	if (initerr || tn == NULL) {
		debug_leave();
		return;
	}

	sc = initsym->s_scl;

	/*
	 * Do not test for automatic aggregate initialisation. If the
	 * initializer starts with a brace we have the warning already.
	 * If not, an error will be printed that the initializer must
	 * be enclosed by braces.
	 */

	/*
	 * Local initialisation of non-array-types with only one expression
	 * without braces is done by ASSIGN
	 */
	if ((sc == AUTO || sc == REG) &&
	    initsym->s_type->t_tspec != ARRAY && initstk->i_enclosing == NULL) {
		ln = new_name_node(initsym, 0);
		ln->tn_type = tduptyp(ln->tn_type);
		ln->tn_type->t_const = false;
		tn = build(ASSIGN, ln, tn);
		expr(tn, false, false, false, false);
		debug_initstack();
		debug_leave();
		return;
	}

	/*
	 * Remove all entries which cannot be used for further initializers
	 * and do not require a closing brace.
	 */
	initstack_pop_nobrace();

	/* Initialisations by strings are done in initstack_string(). */
	if (initstack_string(tn)) {
		debug_initstack();
		debug_leave();
		return;
	}

	initstack_next_nobrace();
	if (initerr || tn == NULL) {
		debug_initstack();
		debug_leave();
		return;
	}

	initstk->i_remaining--;
	debug_step("remaining=%d tn=%p", initstk->i_remaining, tn);
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

	lint_assert(is_scalar(lt));

	if (!typeok(INIT, 0, ln, tn)) {
		debug_initstack();
		debug_leave();
		return;
	}

	/*
	 * Store the tree memory. This is necessary because otherwise
	 * expr() would free it.
	 */
	tmem = tsave();
	expr(tn, true, false, true, false);
	trestor(tmem);

	if (is_integer(lt) && ln->tn_type->t_bitfield && !is_integer(rt)) {
		/*
		 * Bit-fields can be initialized in trad. C only by integer
		 * constants.
		 */
		if (tflag)
			/* bit-field initialisation is illegal in trad. C */
			warning(186);
	}

	if (lt != rt || (initstk->i_type->t_bitfield && tn->tn_op == CON))
		tn = convert(INIT, 0, initstk->i_type, tn);

	if (tn != NULL && tn->tn_op != CON) {
		sym = NULL;
		offs = 0;
		if (!constant_addr(tn, &sym, &offs)) {
			if (sc == AUTO || sc == REG) {
				/* non-constant initializer */
				c99ism(177);
			} else {
				/* non-constant initializer */
				error(177);
			}
		}
	}

	debug_initstack();
	debug_leave();
}


static bool
initstack_string(tnode_t *tn)
{
	tspec_t	t;
	initstack_element *istk;
	int	len;
	strg_t	*strg;

	if (tn->tn_op != STRING)
		return false;

	debug_enter();
	debug_initstack();

	istk = initstk;
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
		/* Put the array at top of stack */
		initstack_push();
		istk = initstk;
	} else if (istk->i_type != NULL && istk->i_type->t_tspec == ARRAY) {
		debug_step("type array");
		t = istk->i_type->t_subt->t_tspec;
		if (!((strg->st_tspec == CHAR &&
		       (t == CHAR || t == UCHAR || t == SCHAR)) ||
		      (strg->st_tspec == WCHAR && t == WCHAR))) {
			debug_leave();
			return false;
		}
		/*
		 * If the array is already partly initialized, we are
		 * wrong here.
		 */
		if (istk->i_remaining != istk->i_type->t_dim)
			debug_leave();
			return false;
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
		if (istk->i_type->t_dim < len) {
			/* non-null byte ignored in string initializer */
			warning(187);
		}
	}

	/* In every case the array is initialized completely. */
	istk->i_remaining = 0;

	debug_initstack();
	debug_leave();
	return true;
}
