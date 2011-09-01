/*	Id: local.c,v 1.20 2011/06/05 10:29:10 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.4 2011/09/01 12:46:36 plunky Exp $	*/
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


# include "pass1.h"

/*	this file contains code which is dependent on the target machine */

NODE *
clocal(NODE *p)
{
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

	struct symtab *q;
	NODE *l, *r;
	int o;
	TWORD ml;

	switch( o = p->n_op ){

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */

		switch (q->sclass) {

		case PARAM:
		case AUTO:
			/* fake up a structure reference */
			r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			r->n_lval = 0;
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case STATIC:
			if (q->slevel == 0)
				break;
			p->n_lval = 0;
			p->n_sp = q;
			break;

		case REGISTER:
			p->n_op = REG;
			p->n_lval = 0;
			p->n_rval = q->soffset;
			break;

			}
		break;

	case PCONV:
		ml = p->n_left->n_type;
		l = p->n_left;
		if ((ml == CHAR || ml == UCHAR) && l->n_op != ICON)
			break;
		l->n_type = p->n_type;
		l->n_qual = p->n_qual;
		l->n_df = p->n_df;
		l->n_sue = p->n_sue;
		nfree(p);
		p = l;
		break;

	case SCONV:
		l = p->n_left;
		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT) {
			nfree(p);
			return l;
		}
		if (l->n_op == ICON) {
			CONSZ val = l->n_lval;
			switch (p->n_type) {
			case CHAR:
				l->n_lval = (char)val;
				break;
			case UCHAR:
				l->n_lval = val & 0377;
				break;
			case SHORT:
			case INT:
				l->n_lval = (short)val;
				break;
			case USHORT:
			case UNSIGNED:
				l->n_lval = val & 0177777;
				break;
			case ULONG:
			case ULONGLONG:
				l->n_lval = val & 0xffffffff;
				break;
			case LONG:
			case LONGLONG:
				l->n_lval = (int)val;
				break;
			case VOID:
				break;
			case LDOUBLE:
			case DOUBLE:
			case FLOAT:
				l->n_op = FCON;
				l->n_dcon = val;
				break;
			default:
				cerror("unknown type %d", p->n_type);
			}
			l->n_type = p->n_type;
			nfree(p);
			return l;
		}
		break;
		

	}

	return(p);
}

/*ARGSUSED*/
int
andable(NODE *p)
{
	return(1);  /* all names can have & taken on them */
}

/*
 * is an automatic variable of type t OK for a register variable
 */
int
cisreg(TWORD t)
{
	if (t == INT || t == UNSIGNED || t == CHAR || t == UCHAR ||
		ISPTR(t))
		return(1);
	return 0; /* XXX - fix reg assignment in pftn.c */
}


/*
 * Allocate off bits on the stack.  p is a tree that when evaluated
 * is the multiply count for off, t is a NAME node where to write
 * the allocated address.
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *sp;

	if ((off % SZINT) == 0)
		p =  buildtree(MUL, p, bcon(off/SZINT));
	else if ((off % SZSHORT) == 0) {
		p = buildtree(MUL, p, bcon(off/SZSHORT));
		p = buildtree(PLUS, p, bcon(1));
		p = buildtree(RS, p, bcon(1));
	} else if ((off % SZCHAR) == 0) {
		p = buildtree(MUL, p, bcon(off/SZCHAR));
		p = buildtree(PLUS, p, bcon(3));
		p = buildtree(RS, p, bcon(2));
	} else
		cerror("roundsp");

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_sue);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

	/* add the size to sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(PLUSEQ, sp, p));
}

/*
 * print out a constant node
 * mat be associated with a label
 */
int
ninval(NODE *p)
{
	struct symtab *q;
	TWORD t;

	p = p->n_left;
	t = p->n_type;
	if (t > BTMASK)
		t = p->n_type = INT; /* pointer */

	switch (t) {
	case LONGLONG:
	case ULONGLONG:
		inval(p->n_lval & 0xffffffff);
		inval(p->n_lval >> 32);
		break;
	case LONG:
	case ULONG:
	case INT:
	case UNSIGNED:
		printf("\t.long 0x%x", (int)p->n_lval);
		if ((q = p->n_sp) != NULL) {
			if ((q->sclass == STATIC && q->slevel > 0)) {
				printf("+" LABFMT, q->soffset);
			} else
				printf("+%s", exname(q->soname));
		}
		printf("\n");
		break;
	default:
		fwalk(p, eprint, 0);
		return 0;
	}
	return 1;
}

/* make a name look like an external name in the local machine */
char *
exname(char *p)
{
	if (p == NULL)
		return "";
	return p;
}

/*
 * map types which are not defined on the local machine
 */
TWORD
ctype(TWORD type)
{
	switch (BTYPE(type)) {
	case SHORT:
		MODTYPE(type,INT);
		break;

	case USHORT:
		MODTYPE(type,UNSIGNED);
		break;

	case LONGLONG:
		MODTYPE(type,LONG);
		break;

	case ULONGLONG:
		MODTYPE(type,ULONG);
		break;

	case LDOUBLE:
		MODTYPE(type,DOUBLE);
		break;
	}
	return (type);
}

/* curid is a variable which is defined but
 * is not initialized (and not a function );
 * This routine returns the storage class for an uninitialized declaration
 */
int
noinit()
{
	return(EXTERN);
}

/*
 * Extern variable not necessary common.
 */
void
extdec(struct symtab *q)
{
	extern void addsym(struct symtab *);
	addsym(q);
}

/*
 * Call to a function
 */
void
calldec(NODE *p, NODE *r)
{
	struct symtab *q = p->n_sp;
	extern void addsym(struct symtab *);
	addsym(q);
}

/* make a common declaration for id, if reasonable */
void
commdec(struct symtab *q)
{
	int off;
	char *c = q->soname;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;

	printf("	PUBLIC %s\n", c);
	/* XXX - NOROOT??? */
	printf("	RSEG DATA16_Z:NEARDATA:SORT:NOROOT(1)\n");
	printf("%s:\n", c);
	printf("	DS8 %d\n", off);
	printf("	REQUIRE __data16_zero\n");
}

/* make a local common declaration for id, if reasonable */
void
lcommdec(struct symtab *q)
{
	int off;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;
	if (q->slevel == 0)
		printf("	.lcomm %s,0%o\n", exname(q->soname), off);
	else
		printf("	.lcomm " LABFMT ",0%o\n", q->soffset, off);
}

/*
 * print a (non-prog) label.
 */
void
deflab1(int label)
{
	printf(LABFMT ":\n", label);
}

void
setloc1(int locc)
{
	if (locc == lastloc)
		return;
	lastloc = locc;
}

/*
 * special handling before tree is written out.
 */
void
myp2tree(NODE *p)
{
	struct symtab *sp;
	union dimfun *df;
	union arglist *al;
	NODE *q;
	int i;

	switch (p->n_op) {
	case MOD:
	case DIV:
		if (p->n_type == LONG || p->n_type == ULONG) {
			/* Swap arguments for hardops() later */
			q = p->n_left;
			p->n_left = p->n_right;
			p->n_right = q;
		}
		break;

	case CALL:
	case STCALL:
		/*
		 * inform pass2 about varargs.
		 * store first variadic argument number in n_stalign
		 * in the CM node.
		 */
		if (p->n_right->n_op != CM)
			break; /* nothing to care about */
		df = p->n_left->n_df;
		if (df && (al = df->dfun)) {
			for (i = 0; i < 6; i++, al++) {
				if (al->type == TELLIPSIS || al->type == TNULL)
					break;
			}
			p->n_right->n_stalign = al->type == TELLIPSIS ? i : 0;
		} else
			p->n_right->n_stalign = 0;
		break;

	case FCON:
		/* Write float constants to memory */
		sp = inlalloc(sizeof(struct symtab));
		sp->sclass = STATIC;
		sp->ssue = 0;
		sp->slevel = 1; /* fake numeric label */
		sp->soffset = getlab();
		sp->sflags = 0;
		sp->stype = p->n_type;
		sp->squal = (CON >> TSHIFT);

		defloc(sp);
		ninval(0, sp->ssue->suesize, p);

		p->n_op = NAME;
		p->n_lval = 0;
		p->n_sp = sp;
		break;
	}

}
/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char **ary)
{
	return 0; }

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
