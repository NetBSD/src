/*	Id: order.c,v 1.5 2016/01/30 17:26:19 ragge Exp 	*/	
/*	$NetBSD: order.c,v 1.1.1.2 2016/02/09 20:28:35 plunky Exp $	*/
/*
 * Copyright (c) 2014 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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


# include "pass2.h"

#include <string.h>

int canaddr(NODE *);

/* is it legal to make an OREG or NAME entry which has an
 * offset of off, (from a register of r), if the
 * resulting thing had type t */
int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	if (off > MAX_SHORT || off < MIN_SHORT)
		return 1; /* max signed 16-bit offset */
	return(0);  /* YES */
}

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 * Especially on m68k we must be careful about class changes;
 * Pointers can only have a scalar added to it if PLUS, but when
 * MINUS may be either only pointers or left pointer and right scalar.
 *
 * So far we only handle the trivial OREGs here.
 */
void
offstar(NODE *p, int shape)
{
	NODE *q;

	if (x2debug) {
		printf("offstar(%p)\n", p);
		fwalk(p, e2print, 0);
	}

	if (isreg(p))
		return; /* Matched (%a0) */

	q = p->n_right;
	if ((p->n_op == PLUS || p->n_op == MINUS) && q->n_op == ICON &&
	    notoff(0, 0, getlval(q), 0) == 0 && !isreg(p->n_left)) {
		(void)geninsn(p->n_left, INBREG);
		return;
	}
	(void)geninsn(p, INBREG);
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 * For simple OREGs conversion should already be done.
 */
void
myormake(NODE *q)
{
}

/*
 * Shape matches for UMUL.  Cooperates with offstar().
 */
int
shumul(NODE *p, int shape)
{

	if (x2debug)
		printf("shumul(%p)\n", p);

	/* Turns currently anything into OREG on x86 */
	if (shape & SOREG)
		return SROREG;
	return SRNOPE;
}

/*
 * Rewrite operations on binary operators (like +, -, etc...).
 * Called as a result of table lookup.
 */
int
setbin(NODE *p)
{

	if (x2debug)
		printf("setbin(%p)\n", p);
	return 0;

}

/* setup for assignment operator */
int
setasg(NODE *p, int cookie)
{
	if (x2debug)
		printf("setasg(%p)\n", p);
	return(0);
}

/* setup for unary operator */
int
setuni(NODE *p, int cookie)
{
	return 0;
}

/*
 * Special handling of some instruction register allocation.
 */
struct rspecial *
nspecial(struct optab *q)
{
	switch (q->op) {
	case STASG:
		{
			static struct rspecial s[] = {
				{ NEVER, D0 }, { NEVER, D1 },
				{ NEVER, A0 }, { NEVER, A1 },
				/* { NRES, A0 }, */ { 0 } };
			return s;
		}
	}
	comperr("nspecial entry %d", q - table);
	return 0; /* XXX gcc */
}

/*
 * Set evaluation order of a binary node if it differs from default.
 */
int
setorder(NODE *p)
{
	return 0;
}

/*
 * set registers in calling conventions live.
 */
int *
livecall(NODE *p)
{
	static int r[] = { A0, -1 };

	if (p->n_op == STCALL)
		return r; /* only if struct return */
	return &r[1];
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
