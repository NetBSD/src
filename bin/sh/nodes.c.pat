/*	$NetBSD: nodes.c.pat,v 1.13.32.1 2018/06/25 07:25:04 pgoyette Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)nodes.c.pat	8.2 (Berkeley) 5/4/95
 */

#include <stdlib.h>
#include <stddef.h>

/*
 * Routine for dealing with parsed shell commands.
 */

#include "shell.h"
#include "nodes.h"
#include "memalloc.h"
#include "machdep.h"
#include "mystring.h"


/* used to accumulate sizes of nodes */
struct nodesize {
	int bsize;		/* size of structures in function */
	int ssize;		/* size of strings in node */
};

/* provides resources for node copies */
struct nodecopystate {
	pointer block;		/* block to allocate function from */
	char *string;		/* block to allocate strings from */
};


%SIZES



STATIC void calcsize(union node *, struct nodesize *);
STATIC void sizenodelist(struct nodelist *, struct nodesize *);
STATIC union node *copynode(union node *, struct nodecopystate *);
STATIC struct nodelist *copynodelist(struct nodelist *, struct nodecopystate *);
STATIC char *nodesavestr(char *, struct nodecopystate *);

struct funcdef {
	unsigned int refcount;
	union node n;		/* must be last */
};


/*
 * Make a copy of a parse tree.
 */

struct funcdef *
copyfunc(union node *n)
{
	struct nodesize sz;
	struct nodecopystate st;
	struct funcdef *fn;

	if (n == NULL)
		return NULL;
	sz.bsize = offsetof(struct funcdef, n);
	sz.ssize = 0;
	calcsize(n, &sz);
	fn = ckmalloc(sz.bsize + sz.ssize);
	fn->refcount = 1;
	st.block = (char *)fn + offsetof(struct funcdef, n);
	st.string = (char *)fn + sz.bsize;
	copynode(n, &st);
	return fn;
}

union node *
getfuncnode(struct funcdef *fn)
{
	if (fn == NULL)
		return NULL;
	return &fn->n;
}


STATIC void
calcsize(union node *n, struct nodesize *res)
{
	%CALCSIZE
}



STATIC void
sizenodelist(struct nodelist *lp, struct nodesize *res)
{
	while (lp) {
		res->bsize += SHELL_ALIGN(sizeof(struct nodelist));
		calcsize(lp->n, res);
		lp = lp->next;
	}
}



STATIC union node *
copynode(union node *n, struct nodecopystate *st)
{
	union node *new;

	%COPY
	return new;
}


STATIC struct nodelist *
copynodelist(struct nodelist *lp, struct nodecopystate *st)
{
	struct nodelist *start;
	struct nodelist **lpp;

	lpp = &start;
	while (lp) {
		*lpp = st->block;
		st->block = (char *)st->block +
		    SHELL_ALIGN(sizeof(struct nodelist));
		(*lpp)->n = copynode(lp->n, st);
		lp = lp->next;
		lpp = &(*lpp)->next;
	}
	*lpp = NULL;
	return start;
}



STATIC char *
nodesavestr(char *s, struct nodecopystate *st)
{
	register char *p = s;
	register char *q = st->string;
	char   *rtn = st->string;

	while ((*q++ = *p++) != 0)
		continue;
	st->string = q;
	return rtn;
}



/*
 * Handle making a reference to a function, and releasing it.
 * Free the func code when there are no remaining references.
 */

void
reffunc(struct funcdef *fn)
{
	if (fn != NULL)
		fn->refcount++;
}

void
unreffunc(struct funcdef *fn)
{
	if (fn != NULL) {
		if (--fn->refcount > 0)
			return;
		ckfree(fn);
	}
}

/*
 * this is used when we need to free the func, regardless of refcount
 * which only happens when re-initing the shell for a SHELLPROC
 */
void
freefunc(struct funcdef *fn)
{
	if (fn != NULL)
		ckfree(fn);
}
