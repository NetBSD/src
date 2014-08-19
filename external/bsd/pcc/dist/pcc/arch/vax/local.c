/*	Id: local.c,v 1.28 2012/12/13 16:01:25 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.4.8.1 2014/08/19 23:52:09 tls Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.	IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

# include "pass1.h"

static void r1arg(NODE *p, NODE *q);


/*	this file contains code which is dependent on the target machine */

NODE *
clocal(p) NODE *p; {

	/* this is called to do local transformations on
	   an expression tree preparitory to its being
	   written out in intermediate code.
	*/

	/* the major essential job is rewriting the
	   automatic variables and arguments in terms of
	   REG and OREG nodes */
	/* conversion ops which are not necessary are also clobbered here */
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	register int ml;


#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal(%p)\n", p);
		if (xdebug>1)
			fwalk(p, eprint, 0);
	}
#endif

	switch( o = p->n_op ){

	case NAME:
		if((q = p->n_sp) == 0 ) { /* already processed; ignore... */
			return(p);
			}
		switch( q->sclass ){

		case REGISTER:
		case AUTO:
		case PARAM:
			/* fake up a structure reference */
			r = block( REG, NIL, NIL, PTR+STRTY, 0, 0 );
			r->n_lval = 0;
			r->n_rval = (q->sclass==PARAM?ARGREG:FPREG);
			p = stref( block( STREF, r, p, 0, 0, 0 ) );
			break;
		}
		break;

	case FORCE:
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(CHAR) : RETREG(p->n_type);
		break;

	case SCONV:
		l = p->n_left;
		ml = p->n_type;
#if 0
		if (ml == INT && l->n_type == UNSIGNED) {
			p = nfree(p);
			break;
		}
#endif
		if (l->n_op == ICON) {
			if (l->n_sp == 0) {
				p->n_type = UNSIGNED;
				concast(l, p->n_type);
			} else if (ml != INT && ml != UNSIGNED)
				break;
			l->n_type = ml;
			l->n_ap = 0;
			p = nfree(p);
			break;
		}
		break;

	case STCALL:
		/* see if we have been here before */
		for (r = p->n_right; r->n_op == CM; r = r->n_left)
			;
		if (r->n_op == ASSIGN)
			break;

		/* FALLTHROUGH */
	case USTCALL:
		/* Allocate buffer on stack to bounce via */
		/* create fake symtab here */
		q = getsymtab("77fake", STEMP);
		q->stype = BTYPE(p->n_type);
		q->sdf = p->n_df;
		q->sap = p->n_ap;
		q->soffset = NOOFFSET;
		q->sclass = AUTO;
		oalloc(q, &autooff);
		r1arg(p, buildtree(ADDROF, nametree(q), 0));
		break;
	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end(%p)\n", p);
		if (xdebug>1)
			fwalk(p, eprint, 0);
	}
#endif

	return(p);
}

/*
 * Add R1 with the dest address as arg to a struct return call.
 */
static void
r1arg(NODE *p, NODE *q)
{
	NODE *r;

	r = block(REG, NIL, NIL, PTR|VOID, 0, 0);
	regno(r) = R1;
	r = buildtree(ASSIGN, r, q);
	if (p->n_op == USTCALL) {
		p->n_op = STCALL;
		p->n_right = r;
	} else if (p->n_right->n_op != CM) {
		p->n_right = block(CM, r, p->n_right, INT, 0, 0);
	} else {
		for (q = p->n_right; q->n_left->n_op == CM; q = q->n_left)
			;
		q->n_left = block(CM, r, q->n_left, INT, 0, 0);
	}
}

void
myp2tree(NODE *p)
{
	struct symtab *sp;

	if ((cdope(p->n_op) & CALLFLG) && p->n_left->n_op == ADDROF &&
	    p->n_left->n_left->n_op == NAME) {
		NODE *q = p->n_left->n_left;
		nfree(p->n_left);
		p->n_left = q;
		q->n_op = ICON;
	}

	if (p->n_op != FCON) 
		return;

	sp = inlalloc(sizeof(struct symtab));
	sp->sclass = STATIC;
	sp->sap = 0;
	sp->slevel = 1; /* fake numeric label */
	sp->soffset = getlab();
	sp->sflags = 0;
	sp->stype = p->n_type;
	sp->squal = (CON >> TSHIFT);

	defloc(sp);
	inval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

	p->n_op = NAME;
	p->n_lval = 0;
	p->n_sp = sp;

}

/*
 * Can we take & of a NAME?
 */
int
andable(NODE *p)
{
	/* for now, delay name reference to table, for PIC code generation */
	/* functions are called by name, convert they in myp2tree */
	return 0;
}

/* is an automatic variable of type t OK for a register variable */
int
cisreg(TWORD t)
{
	return(1);	/* all are now */
}

/*
 * Allocate off bits on the stack.  p is a tree that when evaluated
 * is the multiply count for off, t is a storeable node where to write
 * the allocated address.
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *sp;

	p = buildtree(MUL, p, bcon(off/SZCHAR)); /* XXX word alignment? */

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(MINUSEQ, sp, p));

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_ap);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

}

char *
exname( p ) char *p; {
	/* make a name look like an external name in the local machine */
	/* vad is elf now */
	if (p == NULL)
		return "";
	return( p );
	}

/* map types which are not defined on the local machine */
TWORD
ctype(TWORD type) {
	switch( BTYPE(type) ){

	case LONG:
		MODTYPE(type,INT);
		break;

	case ULONG:
		MODTYPE(type,UNSIGNED);
		break;

	case LDOUBLE:	/* for now */
		MODTYPE(type,DOUBLE);
		}
	return( type );
	}

void
calldec(NODE *p, NODE *q) 
{
}

void
extdec(struct symtab *q)
{
}

/* make a common declaration for id, if reasonable */
void
defzero(struct symtab *sp)
{
	int off, al;
	char *name;

	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);
	off = tsize(sp->stype, sp->sdf, sp->sap);
	SETOFF(off,SZCHAR);
	off /= SZCHAR;
	al = talign(sp->stype, sp->sap)/SZCHAR;

	if (sp->sclass == STATIC) {
		if (sp->slevel == 0)
			printf("\t.local %s\n", name);
		else
			printf("\t.local " LABFMT "\n", sp->soffset);
	}
	if (sp->slevel == 0) {
		printf("\t.comm %s,0%o,%d\n", name, off, al);
	} else
		printf("\t.comm " LABFMT ",0%o,%d\n", sp->soffset, off, al);
}

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 * XXX - floating point constants may be wrong if cross-compiling.
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
	union { float f; double d; long double l; int i[3]; } u;

	switch (p->n_type) {
	case LDOUBLE:
		u.i[2] = 0;
		u.l = (long double)p->n_dcon;
		printf("\t.long\t0x%x,0x%x,0x%x\n", u.i[0], u.i[1], u.i[2]);
		break;
	case DOUBLE:
		u.d = (double)p->n_dcon;
		printf("\t.long\t0x%x,0x%x\n", u.i[0], u.i[1]);
		break;
	case FLOAT:
		u.f = (float)p->n_dcon;
		printf("\t.long\t0x%x\n", u.i[0]);
		break;
	default:
		return 0;
	}
	return 1;

}
/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	return 0;
}

/*
 * Called when a identifier has been declared, to give target last word.
 */
void
fixdef(struct symtab *sp)
{
}

void
pass1_lastchance(struct interpass *ip)
{
}
