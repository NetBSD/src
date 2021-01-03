/*	$NetBSD: init.c,v 1.60 2021/01/03 20:31:08 rillig Exp $	*/

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
__RCSID("$NetBSD: init.c,v 1.60 2021/01/03 20:31:08 rillig Exp $");
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
typedef	struct istk {
	type_t	*i_type;		/* type of initialisation */
	type_t	*i_subt;		/* type of next level */
	bool	i_brace : 1;		/* need } for pop */
	bool	i_nolimit : 1;		/* incomplete array type */
	bool	i_namedmem : 1;		/* has c9x named members */
	sym_t	*i_mem;			/* next structure member */
	int	i_remaining;		/* # of remaining elements */
	struct	istk *i_next;		/* previous level */
} istk_t;

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
int	initerr;

/* Pointer to the symbol which is to be initialized. */
sym_t	*initsym;

/* Points to the top element of the initialisation stack. */
istk_t	*initstk;

/* Points to a c9x named member; */
namlist_t	*namedmem = NULL;


static	int	initstack_string(tnode_t *);

#ifndef DEBUG
#define DPRINTF(a)
#else
#define DPRINTF(a) printf a
#endif

void
push_member(sbuf_t *sb)
{
	namlist_t *nam = xcalloc(1, sizeof (namlist_t));
	nam->n_name = sb->sb_name;
	DPRINTF(("%s: %s %p\n", __func__, nam->n_name, nam));
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
	DPRINTF(("%s: %s %p\n", __func__, namedmem->n_name, namedmem));
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

static void
named_member_dprint(void)
{
	namlist_t *name;

	if (namedmem == NULL)
		return;
	name = namedmem;
	DPRINTF(("named member:"));
	do {
		DPRINTF((" %s", name->n_name));
		name = name->n_next;
	} while (name != namedmem);
	DPRINTF(("\n"));
}

/*
 * Initialize the initialisation stack by putting an entry for the variable
 * which is to be initialized on it.
 */
void
initstack_init(void)
{
	istk_t	*istk;

	if (initerr)
		return;

	/* free memory used in last initialisation */
	while ((istk = initstk) != NULL) {
		initstk = istk->i_next;
		free(istk);
	}

	DPRINTF(("%s\n", __func__));

	/*
	 * If the type which is to be initialized is an incomplete type,
	 * it must be duplicated.
	 */
	if (initsym->s_type->t_tspec == ARRAY && incompl(initsym->s_type))
		initsym->s_type = duptyp(initsym->s_type);

	istk = initstk = xcalloc(1, sizeof (istk_t));
	istk->i_subt = initsym->s_type;
	istk->i_remaining = 1;
}

static void
initstack_pop_item(void)
{
	istk_t	*istk;
	sym_t	*m;

	istk = initstk;
	DPRINTF(("%s: pop type=%s, brace=%d remaining=%d named=%d\n", __func__,
	    type_name(istk->i_type ? istk->i_type : istk->i_subt),
	    istk->i_brace, istk->i_remaining, istk->i_namedmem));

	initstk = istk->i_next;
	free(istk);
	istk = initstk;
	lint_assert(istk != NULL);

	DPRINTF(("%s: top type=%s, brace=%d remaining=%d named=%d\n", __func__,
	    type_name(istk->i_type ? istk->i_type : istk->i_subt),
	    istk->i_brace, istk->i_remaining, istk->i_namedmem));

	istk->i_remaining--;
	lint_assert(istk->i_remaining >= 0);

	DPRINTF(("%s: top remaining=%d rhs.name=%s\n", __func__,
	    istk->i_remaining, namedmem ? namedmem->n_name : "*null*"));

	if (istk->i_remaining >= 0 && namedmem != NULL) {

		DPRINTF(("%s: named remaining=%d type=%s, rhs.name=%s\n",
		    __func__, istk->i_remaining,
		    type_name(istk->i_type), namedmem->n_name));

		for (m = istk->i_type->t_str->memb; m != NULL; m = m->s_next) {
			DPRINTF(("%s: pop lhs.name=%s rhs.name=%s\n", __func__,
			    m->s_name, namedmem->n_name));
			if (m->s_bitfield && m->s_name == unnamed)
				continue;
			if (strcmp(m->s_name, namedmem->n_name) == 0) {
				istk->i_subt = m->s_type;
				istk->i_remaining++;
				pop_member();
				return;
			}
		}
		/* undefined struct/union member: %s */
		error(101, namedmem->n_name);
		DPRINTF(("%s: end rhs.name=%s\n", __func__, namedmem->n_name));
		pop_member();
		istk->i_namedmem = 1;
		return;
	}
	/*
	 * If the removed element was a structure member, we must go
	 * to the next structure member.
	 */
	if (istk->i_remaining > 0 && istk->i_type->t_tspec == STRUCT &&
	    !istk->i_namedmem) {
		do {
			m = istk->i_mem = istk->i_mem->s_next;
			lint_assert(m != NULL);
			DPRINTF(("%s: pop %s\n", __func__, m->s_name));
		} while (m->s_bitfield && m->s_name == unnamed);
		istk->i_subt = m->s_type;
	}
}

/*
 * Take all entries, including the first which requires a closing brace,
 * from the stack.
 */
static void
initstack_pop_brace(void)
{
	int brace;

	DPRINTF(("%s\n", __func__));
	do {
		brace = initstk->i_brace;
		DPRINTF(("%s: loop brace=%d\n", __func__, brace));
		initstack_pop_item();
	} while (!brace);
	DPRINTF(("%s: done\n", __func__));
}

/*
 * Take all entries which cannot be used for further initializers from the
 * stack, but do this only if they do not require a closing brace.
 */
static void
initstack_pop_nobrace(void)
{

	DPRINTF(("%s\n", __func__));
	while (!initstk->i_brace && initstk->i_remaining == 0 &&
	       !initstk->i_nolimit)
		initstack_pop_item();
	DPRINTF(("%s: done\n", __func__));
}

static void
initstack_push(void)
{
	istk_t	*istk, *inxt;
	int	cnt;
	sym_t	*m;

	istk = initstk;

	/* Extend an incomplete array type by one element */
	if (istk->i_remaining == 0) {
		DPRINTF(("%s(extend) %s\n", __func__, type_name(istk->i_type)));
		/*
		 * Inside of other aggregate types must not be an incomplete
		 * type.
		 */
		lint_assert(istk->i_next->i_next == NULL);
		istk->i_remaining = 1;
		lint_assert(istk->i_type->t_tspec == ARRAY);
		istk->i_type->t_dim++;
		setcomplete(istk->i_type, 1);
	}

	lint_assert(istk->i_remaining > 0);
	lint_assert(istk->i_type == NULL ||
	    !tspec_is_scalar(istk->i_type->t_tspec));

	initstk = xcalloc(1, sizeof (istk_t));
	initstk->i_next = istk;
	initstk->i_type = istk->i_subt;
	lint_assert(initstk->i_type->t_tspec != FUNC);

again:
	istk = initstk;

	DPRINTF(("%s(%s)\n", __func__, type_name(istk->i_type)));
	switch (istk->i_type->t_tspec) {
	case ARRAY:
		if (namedmem) {
			DPRINTF(("%s: ARRAY %s brace=%d\n", __func__,
			    namedmem->n_name, istk->i_brace));
			goto pop;
		} else if (istk->i_next->i_namedmem) {
			istk->i_brace = 1;
			DPRINTF(("%s ARRAY brace=%d, namedmem=%d\n", __func__,
			    istk->i_brace, istk->i_next->i_namedmem));
		}

		if (incompl(istk->i_type) && istk->i_next->i_next != NULL) {
			/* initialisation of an incomplete type */
			error(175);
			initerr = 1;
			return;
		}
		istk->i_subt = istk->i_type->t_subt;
		istk->i_nolimit = incompl(istk->i_type);
		istk->i_remaining = istk->i_type->t_dim;
		DPRINTF(("%s: elements array %s[%d] %s\n", __func__,
		    type_name(istk->i_subt), istk->i_remaining,
		    namedmem ? namedmem->n_name : "*none*"));
		break;
	case UNION:
		if (tflag)
			/* initialisation of union is illegal in trad. C */
			warning(238);
		/* FALLTHROUGH */
	case STRUCT:
		if (incompl(istk->i_type)) {
			/* initialisation of an incomplete type */
			error(175);
			initerr = 1;
			return;
		}
		cnt = 0;
		DPRINTF(("%s: lookup type=%s, name=%s named=%d\n", __func__,
		    type_name(istk->i_type),
		    namedmem ? namedmem->n_name : "*none*", istk->i_namedmem));
		for (m = istk->i_type->t_str->memb; m != NULL; m = m->s_next) {
			if (m->s_bitfield && m->s_name == unnamed)
				continue;
			if (namedmem != NULL) {
				DPRINTF(("%s: named lhs.member=%s, rhs.member=%s\n",
				    __func__, m->s_name, namedmem->n_name));
				if (strcmp(m->s_name, namedmem->n_name) == 0) {
					cnt++;
					break;
				} else
					continue;
			}
			if (++cnt == 1) {
				istk->i_mem = m;
				istk->i_subt = m->s_type;
			}
		}
		if (namedmem != NULL) {
			if (m == NULL) {
				DPRINTF(("%s: pop struct\n", __func__));
				goto pop;
			}
			istk->i_mem = m;
			istk->i_subt = m->s_type;
			istk->i_namedmem = 1;
			DPRINTF(("%s: named name=%s\n", __func__,
			    namedmem->n_name));
			pop_member();
			cnt = istk->i_type->t_tspec == STRUCT ? 2 : 1;
		}
		istk->i_brace = 1;
		DPRINTF(("%s: unnamed type=%s, brace=%d\n", __func__,
		    type_name(istk->i_type ? istk->i_type : istk->i_subt),
		    istk->i_brace));
		if (cnt == 0) {
			/* cannot init. struct/union with no named member */
			error(179);
			initerr = 1;
			return;
		}
		istk->i_remaining = istk->i_type->t_tspec == STRUCT ? cnt : 1;
		break;
	default:
		if (namedmem) {
			DPRINTF(("%s: pop\n", __func__));
	pop:
			inxt = initstk->i_next;
			free(istk);
			initstk = inxt;
			goto again;
		}
		istk->i_remaining = 1;
		break;
	}
}

static void
initstack_check_too_many(void)
{
	istk_t	*istk;

	istk = initstk;

	/*
	 * If a closing brace is expected we have at least one initializer
	 * too much.
	 */
	if (istk->i_remaining == 0 && !istk->i_nolimit && !istk->i_namedmem) {
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
		initerr = 1;
	}
}

static void
initstack_next_brace(void)
{

	DPRINTF(("%s\n", __func__));
	if (initstk->i_type != NULL &&
	    tspec_is_scalar(initstk->i_type->t_tspec)) {
		/* invalid initializer type %s */
		error(176, type_name(initstk->i_type));
		initerr = 1;
	}
	if (!initerr)
		initstack_check_too_many();
	if (!initerr)
		initstack_push();
	if (!initerr) {
		initstk->i_brace = 1;
		DPRINTF(("%s: %p %s\n", __func__, namedmem, type_name(
			initstk->i_type ? initstk->i_type : initstk->i_subt)));
	}
}

static void
initstack_next_nobrace(void)
{

	DPRINTF(("%s\n", __func__));
	if (initstk->i_type == NULL &&
	    !tspec_is_scalar(initstk->i_subt->t_tspec)) {
		/* {}-enclosed initializer required */
		error(181);
	}

	/* Make sure an entry with a scalar type is at the top of the stack. */
	if (!initerr)
		initstack_check_too_many();
	while (!initerr) {
		if ((initstk->i_type != NULL &&
		     tspec_is_scalar(initstk->i_type->t_tspec)))
			break;
		initstack_push();
	}
}

void
init_lbrace(void)
{
	DPRINTF(("%s\n", __func__));

	if (initerr)
		return;

	if ((initsym->s_scl == AUTO || initsym->s_scl == REG) &&
	    initstk->i_next == NULL) {
		if (tflag && !tspec_is_scalar(initstk->i_subt->t_tspec))
			/* no automatic aggregate initialization in trad. C */
			warning(188);
	}

	/*
	 * Remove all entries which cannot be used for further initializers
	 * and do not expect a closing brace.
	 */
	initstack_pop_nobrace();

	initstack_next_brace();
}

void
init_rbrace(void)
{
	DPRINTF(("%s\n", __func__));

	if (initerr)
		return;

	initstack_pop_brace();
}

void
mkinit(tnode_t *tn)
{
	ptrdiff_t offs;
	sym_t	*sym;
	tspec_t	lt, rt;
	tnode_t	*ln;
	struct	mbl *tmem;
	scl_t	sc;
#ifdef DEBUG
	char	sbuf[64];
#endif

	DPRINTF(("%s: type=%s, value=%s\n", __func__,
	    type_name(tn->tn_type),
	    print_tnode(sbuf, sizeof(sbuf), tn)));
	named_member_dprint();

	if (initerr || tn == NULL)
		return;

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
	    initsym->s_type->t_tspec != ARRAY && initstk->i_next == NULL) {
		ln = new_name_node(initsym, 0);
		ln->tn_type = tduptyp(ln->tn_type);
		ln->tn_type->t_const = 0;
		tn = build(ASSIGN, ln, tn);
		expr(tn, 0, 0, 0);
		return;
	}

	/*
	 * Remove all entries which cannot be used for further initializers
	 * and do not require a closing brace.
	 */
	initstack_pop_nobrace();

	/* Initialisations by strings are done in initstack_string(). */
	if (initstack_string(tn))
		return;

	initstack_next_nobrace();
	if (initerr || tn == NULL)
		return;

	initstk->i_remaining--;
	DPRINTF(("%s: remaining=%d tn=%p\n", __func__,
	    initstk->i_remaining, tn));
	/* Create a temporary node for the left side. */
	ln = tgetblk(sizeof (tnode_t));
	ln->tn_op = NAME;
	ln->tn_type = tduptyp(initstk->i_type);
	ln->tn_type->t_const = 0;
	ln->tn_lvalue = 1;
	ln->tn_sym = initsym;		/* better than nothing */

	tn = cconv(tn);

	lt = ln->tn_type->t_tspec;
	rt = tn->tn_type->t_tspec;

	lint_assert(tspec_is_scalar(lt));

	if (!typeok(INIT, 0, ln, tn))
		return;

	/*
	 * Store the tree memory. This is necessary because otherwise
	 * expr() would free it.
	 */
	tmem = tsave();
	expr(tn, 1, 0, 1);
	trestor(tmem);

	if (tspec_is_int(lt) && ln->tn_type->t_bitfield && !tspec_is_int(rt)) {
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
		if (conaddr(tn, &sym, &offs) == -1) {
			if (sc == AUTO || sc == REG) {
				/* non-constant initializer */
				c99ism(177);
			} else {
				/* non-constant initializer */
				error(177);
			}
		}
	}
}


static int
initstack_string(tnode_t *tn)
{
	tspec_t	t;
	istk_t	*istk;
	int	len;
	strg_t	*strg;

	if (tn->tn_op != STRING)
		return 0;

	istk = initstk;
	strg = tn->tn_string;

	/*
	 * Check if we have an array type which can be initialized by
	 * the string.
	 */
	if (istk->i_subt != NULL && istk->i_subt->t_tspec == ARRAY) {
		DPRINTF(("%s: subt array\n", __func__));
		t = istk->i_subt->t_subt->t_tspec;
		if (!((strg->st_tspec == CHAR &&
		       (t == CHAR || t == UCHAR || t == SCHAR)) ||
		      (strg->st_tspec == WCHAR && t == WCHAR))) {
			return 0;
		}
		/* Put the array at top of stack */
		initstack_push();
		istk = initstk;
	} else if (istk->i_type != NULL && istk->i_type->t_tspec == ARRAY) {
		DPRINTF(("%s: type array\n", __func__));
		t = istk->i_type->t_subt->t_tspec;
		if (!((strg->st_tspec == CHAR &&
		       (t == CHAR || t == UCHAR || t == SCHAR)) ||
		      (strg->st_tspec == WCHAR && t == WCHAR))) {
			return 0;
		}
		/*
		 * If the array is already partly initialized, we are
		 * wrong here.
		 */
		if (istk->i_remaining != istk->i_type->t_dim)
			return 0;
	} else {
		return 0;
	}

	/* Get length without trailing NUL character. */
	len = strg->st_len;

	if (istk->i_nolimit) {
		istk->i_nolimit = 0;
		istk->i_type->t_dim = len + 1;
		setcomplete(istk->i_type, 1);
	} else {
		if (istk->i_type->t_dim < len) {
			/* non-null byte ignored in string initializer */
			warning(187);
		}
	}

	/* In every case the array is initialized completely. */
	istk->i_remaining = 0;

	return 1;
}
