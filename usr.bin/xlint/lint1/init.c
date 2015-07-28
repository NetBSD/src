/*	$NetBSD: init.c,v 1.27 2015/07/28 17:55:13 christos Exp $	*/

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
__RCSID("$NetBSD: init.c,v 1.27 2015/07/28 17:55:13 christos Exp $");
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lint1.h"

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

typedef struct namlist {
	const char *n_name;
	struct namlist *n_prev;
	struct namlist *n_next;
} namlist_t;

/* Points to a c9x named member; */
namlist_t	*namedmem = NULL;


static	void	popi2(void);
static	void	popinit(int);
static	void	pushinit(void);
static	void	testinit(void);
static	void	nextinit(int);
static	int	strginit(tnode_t *);
static	void	memberpop(void);

#ifndef DEBUG
#define DPRINTF(a)
#else
#define DPRINTF(a) printf a
#endif

void
memberpush(sb)
	sbuf_t *sb;
{
	namlist_t *nam = xcalloc(1, sizeof (namlist_t)); 
	nam->n_name = sb->sb_name;
	DPRINTF(("%s: %s %p\n", __func__, nam->n_name, nam));
	if (namedmem == NULL) {
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
memberpop(void)
{
	DPRINTF(("%s: %s %p\n", __func__, namedmem->n_name, namedmem));
	if (namedmem->n_next == namedmem) {
		free(namedmem);
		namedmem = NULL;
	} else {
		namlist_t *nam = namedmem;
		namedmem = namedmem->n_next;
		namedmem->n_next = nam->n_next;
		namedmem->n_prev = nam->n_prev;
		free(nam);
	}
}


/*
 * Initialize the initialisation stack by putting an entry for the variable
 * which is to be initialized on it.
 */
void
prepinit(void)
{
	istk_t	*istk;

	if (initerr)
		return;

	/* free memory used in last initialisation */
	while ((istk = initstk) != NULL) {
		initstk = istk->i_nxt;
		free(istk);
	}

	/*
	 * If the type which is to be initialized is an incomplete type,
	 * it must be duplicated.
	 */
	if (initsym->s_type->t_tspec == ARRAY && incompl(initsym->s_type))
		initsym->s_type = duptyp(initsym->s_type);

	istk = initstk = xcalloc(1, sizeof (istk_t));
	istk->i_subt = initsym->s_type;
	istk->i_cnt = 1;

}

static void
popi2(void)
{
#ifdef DEBUG
	char	buf[64];
#endif
	istk_t	*istk;
	sym_t	*m;

	DPRINTF(("%s+(%s): brace=%d count=%d namedmem %d\n", __func__,
	    tyname(buf, sizeof(buf),
	    initstk->i_type ? initstk->i_type : initstk->i_subt),
	    initstk->i_brace, initstk->i_cnt, initstk->i_namedmem));
	initstk = (istk = initstk)->i_nxt;
	free(istk);

	istk = initstk;
	if (istk == NULL)
		LERROR("popi2()");

	DPRINTF(("%s-(%s): brace=%d count=%d namedmem %d\n", __func__,
	    tyname(buf, sizeof(buf),
	    initstk->i_type ? initstk->i_type : initstk->i_subt),
	    initstk->i_brace, initstk->i_cnt, initstk->i_namedmem));

	istk->i_cnt--;
	if (istk->i_cnt < 0)
		LERROR("popi2()");

	DPRINTF(("%s(): %d %s\n", __func__, istk->i_cnt,
	    namedmem ? namedmem->n_name : "*null*"));
	if (istk->i_cnt >= 0 && namedmem != NULL) {
		DPRINTF(("%s(): %d %s %s\n", __func__, istk->i_cnt,
		    tyname(buf, sizeof(buf), istk->i_type), namedmem->n_name));
		for (m = istk->i_type->t_str->memb; m != NULL; m = m->s_nxt) {
			DPRINTF(("%s(): pop [%s %s]\n", __func__,
			    namedmem->n_name, m->s_name));
			if (m->s_field && m->s_name == unnamed)
				continue;
			if (strcmp(m->s_name, namedmem->n_name) == 0) {
				istk->i_subt = m->s_type;
				istk->i_cnt++;
				memberpop();
				return;
			}
		}
		error(101, namedmem->n_name);
		DPRINTF(("%s(): namedmem %s\n", __func__, namedmem->n_name));
		memberpop();
		istk->i_namedmem = 1;
		return;
	}
	/*
	 * If the removed element was a structure member, we must go
	 * to the next structure member.
	 */
	if (istk->i_cnt > 0 && istk->i_type->t_tspec == STRUCT &&
	    !istk->i_namedmem) {
		do {
			m = istk->i_mem = istk->i_mem->s_nxt;
			if (m == NULL)
				LERROR("popi2()");
			DPRINTF(("%s(): pop %s\n", __func__, m->s_name));
		} while (m->s_field && m->s_name == unnamed);
		istk->i_subt = m->s_type;
	}
}

static void
popinit(int brace)
{
	DPRINTF(("%s(%d)\n", __func__, brace));

	if (brace) {
		/*
		 * Take all entries, including the first which requires
		 * a closing brace, from the stack.
		 */
		DPRINTF(("%s: brace\n", __func__));
		do {
			brace = initstk->i_brace;
			DPRINTF(("%s: loop brace %d\n", __func__, brace));
			popi2();
		} while (!brace);
		DPRINTF(("%s: brace done\n", __func__));
	} else {
		/*
		 * Take all entries which cannot be used for further
		 * initializers from the stack, but do this only if
		 * they do not require a closing brace.
		 */
		DPRINTF(("%s: no brace\n", __func__));
		while (!initstk->i_brace &&
		       initstk->i_cnt == 0 && !initstk->i_nolimit) {
			popi2();
		}
		DPRINTF(("%s: no brace done\n", __func__));
	}
}

static void
pushinit(void)
{
#ifdef DEBUG
	char	buf[64];
#endif
	istk_t	*istk, *inxt;
	int	cnt;
	sym_t	*m;

	istk = initstk;

	/* Extend an incomplete array type by one element */
	if (istk->i_cnt == 0) {
		DPRINTF(("%s(extend) %s\n", __func__, tyname(buf, sizeof(buf),
		    istk->i_type)));
		/*
		 * Inside of other aggregate types must not be an incomplete
		 * type.
		 */
		if (istk->i_nxt->i_nxt != NULL)
			LERROR("pushinit()");
		istk->i_cnt = 1;
		if (istk->i_type->t_tspec != ARRAY)
			LERROR("pushinit()");
		istk->i_type->t_dim++;
		/* from now its an complete type */
		setcompl(istk->i_type, 0);
	}

	if (istk->i_cnt <= 0)
		LERROR("pushinit()");
	if (istk->i_type != NULL && issclt(istk->i_type->t_tspec))
		LERROR("pushinit()");

	initstk = xcalloc(1, sizeof (istk_t));
	initstk->i_nxt = istk;
	initstk->i_type = istk->i_subt;
	if (initstk->i_type->t_tspec == FUNC)
		LERROR("pushinit()");

again:
	istk = initstk;

	DPRINTF(("%s(%s)\n", __func__, tyname(buf, sizeof(buf), istk->i_type)));
	switch (istk->i_type->t_tspec) {
	case ARRAY:
		if (namedmem) {
			DPRINTF(("%s: ARRAY %s brace=%d\n", __func__,
			    namedmem->n_name, istk->i_brace));
			goto pop;
		} else if (istk->i_nxt->i_namedmem) {
			istk->i_brace = 1;
			DPRINTF(("%s ARRAY brace=%d, namedmem=%d\n", __func__,
			    istk->i_brace, istk->i_nxt->i_namedmem));
		}

		if (incompl(istk->i_type) && istk->i_nxt->i_nxt != NULL) {
			/* initialisation of an incomplete type */
			error(175);
			initerr = 1;
			return;
		}
		istk->i_subt = istk->i_type->t_subt;
		istk->i_nolimit = incompl(istk->i_type);
		istk->i_cnt = istk->i_type->t_dim;
		DPRINTF(("%s: elements array %s[%d] %s\n", __func__,
		    tyname(buf, sizeof(buf), istk->i_subt), istk->i_cnt,
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
		DPRINTF(("%s: 2. member lookup %s %s i_namedmem=%d\n", __func__,
		    tyname(buf, sizeof(buf), istk->i_type),
		    namedmem ? namedmem->n_name : "*none*", istk->i_namedmem));
		for (m = istk->i_type->t_str->memb; m != NULL; m = m->s_nxt) {
			if (m->s_field && m->s_name == unnamed)
				continue;
			if (namedmem != NULL) {
				DPRINTF(("%s():[member:%s, looking:%s]\n",
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
				DPRINTF(("%s(): struct pop\n", __func__));
				goto pop;
			} 
			istk->i_mem = m;
			istk->i_subt = m->s_type;
			istk->i_namedmem = 1;
			DPRINTF(("%s(): namedmem %s\n", __func__,
			    namedmem->n_name));
			memberpop();
			cnt = istk->i_type->t_tspec == STRUCT ? 2 : 1;
		}
		istk->i_brace = 1;
		DPRINTF(("%s(): %s brace=%d\n", __func__,
		    tyname(buf, sizeof(buf),
		    istk->i_type ? istk->i_type : istk->i_subt),
		    istk->i_brace));
		if (cnt == 0) {
			/* cannot init. struct/union with no named member */
			error(179);
			initerr = 1;
			return;
		}
		istk->i_cnt = istk->i_type->t_tspec == STRUCT ? cnt : 1;
		break;
	default:
		if (namedmem) {
			DPRINTF(("%s(): pop\n", __func__));
	pop:
			inxt = initstk->i_nxt;
			free(istk);
			initstk = inxt;
			goto again;
		}
		istk->i_cnt = 1;
		break;
	}
}

static void
testinit(void)
{
	istk_t	*istk;

	istk = initstk;

	/*
	 * If a closing brace is expected we have at least one initializer
	 * too much.
	 */
	if (istk->i_cnt == 0 && !istk->i_nolimit && !istk->i_namedmem) {
		switch (istk->i_type->t_tspec) {
		case ARRAY:
			/* too many array initializers */
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
nextinit(int brace)
{
	char buf[64];

	DPRINTF(("%s(%d)\n", __func__, brace));
	if (!brace) {
		if (initstk->i_type == NULL &&
		    !issclt(initstk->i_subt->t_tspec)) {
			/* {}-enclosed initializer required */
			error(181);
		}
		/*
		 * Make sure an entry with a scalar type is at the top
		 * of the stack.
		 */
		if (!initerr)
			testinit();
		while (!initerr && (initstk->i_type == NULL ||
				    !issclt(initstk->i_type->t_tspec))) {
			if (!initerr)
				pushinit();
		}
	} else {
		if (initstk->i_type != NULL &&
		    issclt(initstk->i_type->t_tspec)) {
			/* invalid initializer */
			error(176, tyname(buf, sizeof(buf), initstk->i_type));
			initerr = 1;
		}
		if (!initerr)
			testinit();
		if (!initerr)
			pushinit();
		if (!initerr) {
			initstk->i_brace = 1;
			DPRINTF(("%s(): %p %s brace=%d\n", __func__,
			    namedmem, tyname(buf, sizeof(buf),
			    initstk->i_type ? initstk->i_type :
			    initstk->i_subt), initstk->i_brace));
		}
	}
}

void
initlbr(void)
{
	DPRINTF(("%s\n", __func__));

	if (initerr)
		return;

	if ((initsym->s_scl == AUTO || initsym->s_scl == REG) &&
	    initstk->i_nxt == NULL) {
		if (tflag && !issclt(initstk->i_subt->t_tspec))
			/* no automatic aggregate initialization in trad. C*/
			warning(188);
	}

	/*
	 * Remove all entries which cannot be used for further initializers
	 * and do not expect a closing brace.
	 */
	popinit(0);

	nextinit(1);
}

void
initrbr(void)
{
	DPRINTF(("%s\n", __func__));

	if (initerr)
		return;

	popinit(1);
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
	char	buf[64], sbuf[64];
#endif

	DPRINTF(("%s(%s %s)\n", __func__, tyname(buf, sizeof(buf), tn->tn_type),
	    prtnode(sbuf, sizeof(sbuf), tn)));
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
	    initsym->s_type->t_tspec != ARRAY && initstk->i_nxt == NULL) {
		ln = getnnode(initsym, 0);
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
	popinit(0);

	/* Initialisations by strings are done in strginit(). */
	if (strginit(tn))
		return;

	nextinit(0);
	if (initerr || tn == NULL)
		return;

	initstk->i_cnt--;
	DPRINTF(("%s() cnt=%d tn=%p\n", __func__, initstk->i_cnt, tn));
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

	if (!issclt(lt))
		LERROR("mkinit()");

	if (!typeok(INIT, 0, ln, tn))
		return;

	/*
	 * Store the tree memory. This is nessesary because otherwise
	 * expr() would free it.
	 */
	tmem = tsave();
	expr(tn, 1, 0, 1);
	trestor(tmem);

	if (isityp(lt) && ln->tn_type->t_isfield && !isityp(rt)) {
		/*
		 * Bit-fields can be initialized in trad. C only by integer
		 * constants.
		 */
		if (tflag)
			/* bit-field initialisation is illegal in trad. C */
			warning(186);
	}

	if (lt != rt || (initstk->i_type->t_isfield && tn->tn_op == CON))
		tn = convert(INIT, 0, initstk->i_type, tn);

	if (tn != NULL && tn->tn_op != CON) {
		sym = NULL;
		offs = 0;
		if (conaddr(tn, &sym, &offs) == -1) {
			if (sc == AUTO || sc == REG) {
				/* non-constant initializer */
				(void)c99ism(177);
			} else {
				/* non-constant initializer */
				error(177);
			}
		}
	}
}


static int
strginit(tnode_t *tn)
{
	tspec_t	t;
	istk_t	*istk;
	int	len;
	strg_t	*strg;

	if (tn->tn_op != STRING)
		return (0);

	istk = initstk;
	strg = tn->tn_strg;

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
			return (0);
		}
		/* Put the array at top of stack */
		pushinit();
		istk = initstk;
	} else if (istk->i_type != NULL && istk->i_type->t_tspec == ARRAY) {
		DPRINTF(("%s: type array\n", __func__));
		t = istk->i_type->t_subt->t_tspec;
		if (!((strg->st_tspec == CHAR &&
		       (t == CHAR || t == UCHAR || t == SCHAR)) ||
		      (strg->st_tspec == WCHAR && t == WCHAR))) {
			return (0);
		}
		/*
		 * If the array is already partly initialized, we are
		 * wrong here.
		 */
		if (istk->i_cnt != istk->i_type->t_dim)
			return (0);
	} else {
		return (0);
	}

	/* Get length without trailing NUL character. */
	len = strg->st_len;

	if (istk->i_nolimit) {
		istk->i_nolimit = 0;
		istk->i_type->t_dim = len + 1;
		/* from now complete type */
		setcompl(istk->i_type, 0);
	} else {
		if (istk->i_type->t_dim < len) {
			/* non-null byte ignored in string initializer */
			warning(187);
		}
	}

	/* In every case the array is initialized completely. */
	istk->i_cnt = 0;

	return (1);
}
