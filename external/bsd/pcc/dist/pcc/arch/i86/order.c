/*	Id: order.c,v 1.3 2014/12/27 21:18:19 ragge Exp 	*/	
/*	$NetBSD: order.c,v 1.1.1.1 2016/02/09 20:28:39 plunky Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
	return(0);  /* YES */
}

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 */
void
offstar(NODE *p, int shape)
{
	NODE *r;

	if (x2debug)
		printf("offstar(%p)\n", p);

	if (isreg(p))
		return; /* Is already OREG */

	r = p->n_right;
	if( p->n_op == PLUS || p->n_op == MINUS ){
		if( r->n_op == ICON ){
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			/* Converted in ormake() */
			return;
		}
		if (r->n_op == LS && r->n_right->n_op == ICON &&
		    r->n_right->n_lval == 2 && p->n_op == PLUS) {
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			if (isreg(r->n_left) == 0)
				(void)geninsn(r->n_left, INAREG);
			return;
		}
	}
	(void)geninsn(p, INAREG);
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 */
void
myormake(NODE *q)
{
	NODE *p, *r;

	if (x2debug)
		printf("myormake(%p)\n", q);

	p = q->n_left;
	if (p->n_op == PLUS && (r = p->n_right)->n_op == LS &&
	    r->n_right->n_op == ICON && r->n_right->n_lval == 2 &&
	    p->n_left->n_op == REG && r->n_left->n_op == REG) {
		q->n_op = OREG;
		q->n_lval = 0;
		q->n_rval = R2PACK(p->n_left->n_rval, r->n_left->n_rval, 0);
		tfree(p);
	}
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
 *
 * FIXME: review 32bit specials, a lot can go as we use helpers
 */
struct rspecial *
nspecial(struct optab *q)
{
	switch (q->op) {
	case OPLOG:
		{
			static struct rspecial s[] = { { NEVER, AX }, { 0 } };
			return s;
		}

	case STASG:
		{
			static struct rspecial s[] = {
				{ NEVER, DI },
				{ NRIGHT, SI }, { NOLEFT, SI },
				{ NOLEFT, CX }, { NORIGHT, CX },
				{ NEVER, CX }, { 0 } };
			return s;
		}

	case STARG:
		{
			static struct rspecial s[] = {
				{ NEVER, DI }, { NEVER, CX },
				{ NLEFT, SI }, { 0 } };
			return s;
		}

	case SCONV:
		/* Force signed conversions into al/ax to use cbw */
		if ((q->ltype & TCHAR) && 
		    q->rtype == (TINT|TUNSIGNED)) {
			static struct rspecial s[] = { 
				{ NLEFT, AL }, { NRES, AX },
				{ NEVER, AX }, { NEVER, AH}, {NEVER, AL },
				{ 0 } };
			return s;
		}
		/* FIXME: do we need this rule with and based moves ? */
		/* Force signed unconversions to avoid si and di */
		if ((q->ltype & TUCHAR) && 
		    q->rtype == (TINT|TUNSIGNED)) {
			static struct rspecial s[] = { 
				{ NEVER, SI }, { NEVER, DI },
				{ 0 } };
			return s;
		} else if ((q->ltype & (TINT|TUNSIGNED)) && 
		    q->rtype == (TCHAR|TUCHAR)) {
			static struct rspecial s[] = { 
				{ NOLEFT, SI }, { NOLEFT, DI }, { 0 } };
			return s;
                /* Need to be able to use cwd */
		} else if ((q->ltype & (TINT|TSHORT)) &&
		    q->rtype == (TLONG|TULONG)) {
			static struct rspecial s[] = {
				{ NLEFT, AX }, { NRES, AXDX },
				{ NEVER, AX }, { NEVER, DX }, 
				{ 0 } };
			return s;
		/* Need to be use cbw cwd */
		} else if (q->ltype == TCHAR &&
		    q->rtype == (TLONG|TULONG)) {
			static struct rspecial s[] = {
				{ NRES, AXDX }, { NLEFT, AL },
				{ NEVER, AX }, { NEVER, DX },
				{ NEVER, AH }, { NEVER, AL },
				{ 0 } };
			return s;
		}
		break;
	case DIV:
		if (q->lshape == SBREG) {
			static struct rspecial s[] = {
				{ NEVER, AL }, { NEVER, AH },
				{ NLEFT, AL }, { NRES, AL },
				{ NORIGHT, AH }, { NORIGHT, AL }, { 0 } };
				return s;
		} else if (q->lshape == SAREG) {
			static struct rspecial s[] = {
				{ NEVER, AX }, { NEVER, DX },
				{ NLEFT, AX }, { NRES, AX },
				{ NORIGHT, DX }, { NORIGHT, AX }, { 0 } };
			return s;
		}
/* using helpers */		
		 else if (q->lshape & SCREG) {
			static struct rspecial s[] = {
				{ NEVER, AX }, { NEVER, DX },
				{ NEVER, CX }, { NRES, AXDX }, { 0 } };
			return s;
		}
		break;
	case MOD:
		if (q->lshape == SBREG) {
			static struct rspecial s[] = {
				{ NEVER, AL }, { NEVER, AH },
				{ NLEFT, AL }, { NRES, AH },
				{ NORIGHT, AH }, { NORIGHT, AL }, { 0 } };
			return s;
		} else if (q->lshape == SAREG) {
			static struct rspecial s[] = {
				{ NEVER, AX }, { NEVER, DX },
				{ NLEFT, AX }, { NRES, DX },
				{ NORIGHT, DX }, { NORIGHT, AX }, { 0 } };
			return s;
		} else if (q->lshape & SCREG) {
			static struct rspecial s[] = {
				{ NEVER, AX }, { NEVER, DX },
				{ NEVER, CX }, { NRES, AXDX }, { 0 } };
			return s;
		}
		break;
	case MUL:
		if (q->lshape == SAREG) {
			static struct rspecial s[] = {
				{ NEVER, AX }, { NEVER, DX },
				{ NLEFT, AX }, { NRES, AX },
				{ NORIGHT, DX }, {NORIGHT, AX },
			        { 0 } };
			return s;
		}
		if (q->lshape == SBREG) {
			static struct rspecial s[] = {
				{ NEVER, AL }, { NEVER, AH },
				{ NLEFT, AL }, { NRES, AL }, { 0 } };
			return s;
		} else if (q->lshape & SCREG) {
			static struct rspecial s[] = {
				{ NEVER, AX }, { NEVER, DX },
				{ NEVER, CX }, { NRES, AXDX }, { 0 } };
			return s;
		}
		break;
	case LS:
	case RS:
		if (q->visit & (INAREG|INBREG)) {
			static struct rspecial s[] = {
				{ NRIGHT, CL }, { NOLEFT, CX }, { 0 } };
			return s;
		} else if (q->visit & INCREG) {
			static struct rspecial s[] = {
				{ NLEFT, AXDX }, { NRIGHT, CL },
				{ NRES, AXDX }, { 0 } };
			return s;
		}
		break;

	default:
		break;
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
	return 0; /* nothing differs on x86 */
}

/*
 * set registers in calling conventions live.
 */
int *
livecall(NODE *p)
{
	static int r[] = { AX, BX, -1 };
	int off = 1;
	return kflag ? &r[off] : &r[2];
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
