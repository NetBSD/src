/*	$NetBSD: stree.c,v 1.11.4.1 2008/01/09 02:02:32 matt Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * stree.c -- SUP Tree Routines
 *
 **********************************************************************
 * HISTORY
 * Revision 1.4  92/08/11  12:06:32  mrt
 * 	Added copyright. Delinted
 * 	[92/08/10            mrt]
 *
 *
 * Revision 1.3  89/08/15  15:30:57  bww
 * 	Changed code in Tlookup to Tsearch for each subpart of path.
 * 	Added indent formatting code to Tprint.
 * 	From "[89/06/24            gm0w]" at CMU.
 * 	[89/08/15            bww]
 *
 * 20-May-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to please lint.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to initialize new fields.  Added Tfree routine.
 *
 * 27-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include <sys/param.h>
#include <assert.h>
#include "supcdefs.h"
#include "supextern.h"
#include "libc.h"
#include "c.h"

static TREE *Tmake(char *);
static TREE *Trotll(TREE *, TREE *);
static TREE *Trotlh(TREE *, TREE *);
static TREE *Trothl(TREE *, TREE *);
static TREE *Trothh(TREE *, TREE *);
static void Tbalance(TREE **);
static TREE *Tinsertavl(TREE **, char *, int, int *);
static int Tsubprocess(TREE *, int, int (*f) (TREE *, void *), void *);
static int Tprintone(TREE *, void *);


/*************************************************************
 ***    T R E E   P R O C E S S I N G   R O U T I N E S    ***
 *************************************************************/

void
Tfree(TREE ** t)
{
	if (*t == NULL)
		return;
	Tfree(&((*t)->Tlink));
	Tfree(&((*t)->Texec));
	Tfree(&((*t)->Tlo));
	Tfree(&((*t)->Thi));
	if ((*t)->Tname)
		free((*t)->Tname);
	if ((*t)->Tuser)
		free((*t)->Tuser);
	if ((*t)->Tgroup)
		free((*t)->Tgroup);
	free(*(char **) t);
	*t = NULL;
}

static TREE *
Tmake(char *p)
{
	TREE *t;
	t = (TREE *) malloc(sizeof(TREE));
	if (t == NULL)
		goaway("Cannot allocate memory");
	t->Tname = (p == NULL) ? NULL : estrdup(p);
	t->Tflags = 0;
	t->Tuid = 0;
	t->Tgid = 0;
	t->Tuser = NULL;
	t->Tgroup = NULL;
	t->Tmode = 0;
	t->Tctime = 0;
	t->Tmtime = 0;
	t->Tlink = NULL;
	t->Texec = NULL;
	t->Tbf = 0;
	t->Tlo = NULL;
	t->Thi = NULL;
	return (t);
}

static TREE *
Trotll(TREE * tp, TREE * tl)
{
	tp->Tlo = tl->Thi;
	tl->Thi = tp;
	tp->Tbf = tl->Tbf = 0;
	return (tl);
}

static TREE *
Trotlh(TREE * tp, TREE * tl)
{
	TREE *th;

	th = tl->Thi;
	tp->Tlo = th->Thi;
	tl->Thi = th->Tlo;
	th->Thi = tp;
	th->Tlo = tl;
	tp->Tbf = tl->Tbf = 0;
	if (th->Tbf == 1)
		tp->Tbf = -1;
	else if (th->Tbf == -1)
		tl->Tbf = 1;
	th->Tbf = 0;
	return (th);
}

static TREE *
Trothl(TREE * tp, TREE * th)
{
	TREE *tl;

	tl = th->Tlo;
	tp->Thi = tl->Tlo;
	th->Tlo = tl->Thi;
	tl->Tlo = tp;
	tl->Thi = th;
	tp->Tbf = th->Tbf = 0;
	if (tl->Tbf == -1)
		tp->Tbf = 1;
	else if (tl->Tbf == 1)
		th->Tbf = -1;
	tl->Tbf = 0;
	return (tl);
}

static TREE *
Trothh(TREE * tp, TREE * th)
{
	tp->Thi = th->Tlo;
	th->Tlo = tp;
	tp->Tbf = th->Tbf = 0;
	return (th);
}

static void
Tbalance(TREE ** t)
{
	if ((*t)->Tbf < 2 && (*t)->Tbf > -2)
		return;
	if ((*t)->Tbf > 0) {
		if ((*t)->Tlo->Tbf > 0)
			*t = Trotll(*t, (*t)->Tlo);
		else
			*t = Trotlh(*t, (*t)->Tlo);
	} else {
		if ((*t)->Thi->Tbf > 0)
			*t = Trothl(*t, (*t)->Thi);
		else
			*t = Trothh(*t, (*t)->Thi);
	}
}

static TREE *
Tinsertavl(TREE ** t, char *p, int find, int *dh)
{
	TREE *newt;
	int cmp;
	int deltah;

	if (*t == NULL) {
		*t = Tmake(p);
		*dh = 1;
		return (*t);
	}
	if ((cmp = strcmp(p, (*t)->Tname)) == 0) {
		if (!find)
			return (NULL);	/* node already exists */
		*dh = 0;
		return (*t);
	} else if (cmp < 0) {
		if ((newt = Tinsertavl(&((*t)->Tlo), p, find, &deltah)) == NULL)
			return (NULL);
		(*t)->Tbf += deltah;
	} else {
		if ((newt = Tinsertavl(&((*t)->Thi), p, find, &deltah)) == NULL)
			return (NULL);
		(*t)->Tbf -= deltah;
	}
	Tbalance(t);
	if ((*t)->Tbf == 0)
		deltah = 0;
	*dh = deltah;
	return (newt);
}

TREE *
Tinsert(TREE ** t, char *p, int find)
{
	int deltah;

	if (p != NULL && p[0] == '.' && p[1] == '/') {
		p += 2;
		while (*p == '/')
			p++;
		if (*p == 0)
			p = ".";
	}
	return (Tinsertavl(t, p, find, &deltah));
}

TREE *
Tsearch(TREE * t, char *p)
{
	TREE *x;
	int cmp;

	x = t;
	while (x) {
		cmp = strcmp(p, x->Tname);
		if (cmp == 0)
			return (x);
		if (cmp < 0)
			x = x->Tlo;
		else
			x = x->Thi;
	}
	return (NULL);
}

TREE *
Tlookup(TREE * t, char *p)
{
	TREE *x;
	char buf[MAXPATHLEN + 1];

	if (p == NULL)
		return (NULL);
	if (p[0] == '.' && p[1] == '/') {
		p += 2;
		while (*p == '/')
			p++;
		if (*p == 0)
			p = ".";
	}
	if ((x = Tsearch(t, p)) != NULL)
		return (x);
	if (*p != '/' && (x = Tsearch(t, ".")) != NULL)
		return (x);
	(void) strncpy(buf, p, sizeof(buf) - 1);
	buf[MAXPATHLEN] = '\0';
	while ((p = rindex(buf, '/')) != NULL) {
		while (p >= buf && *(p - 1) == '/')
			p--;
		if (p == buf)
			*(p + 1) = '\0';
		else
			*p = '\0';
		if ((x = Tsearch(t, buf)) != NULL)
			return (x);
		if (p == buf)
			break;
	}
	return (NULL);
}

static int process_level;

static int
Tsubprocess(TREE * t, int reverse, int (*f) (TREE *, void *), void *argp)
{
	int x = SCMOK;
	process_level++;
	if (reverse ? t->Thi : t->Tlo)
		x = Tsubprocess(reverse ? t->Thi : t->Tlo,
		    reverse, f, argp);
	if (x == SCMOK) {
		x = (*f) (t, argp);
		if (x == SCMOK) {
			if (reverse ? t->Tlo : t->Thi)
				x = Tsubprocess(reverse ? t->Tlo : t->Thi,
				    reverse, f, argp);
		}
	}
	process_level--;
	return (x);
}

/* VARARGS2 */
int
Trprocess(TREE * t, int (*f) (TREE *, void *), void *args)
{
	if (t == NULL)
		return (SCMOK);
	process_level = 0;
	return (Tsubprocess(t, TRUE, f, args));
}

/* VARARGS2 */
int
Tprocess(TREE * t, int (*f) (TREE *, void *), void *args)
{
	if (t == NULL)
		return (SCMOK);
	process_level = 0;
	return (Tsubprocess(t, FALSE, f, args));
}

static int 
Tprintone(TREE * t, void *v __unused)
{
	int i;
	for (i = 0; i < (process_level * 2); i++)
		(void) putchar(' ');
	printf("Node at %p name '%s' flags %o hi %p lo %p\n", t, t->Tname, t->Tflags, t->Thi, t->Tlo);
	return (SCMOK);
}

void
Tprint(TREE * t, char *p)
{				/* print tree -- for debugging */
	printf("%s\n", p);
	(void) Tprocess(t, Tprintone, NULL);
	printf("End of tree\n");
	(void) fflush(stdout);
}
