/*	Id: local.c,v 1.16 2014/06/03 20:19:50 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.4.20.1 2014/08/10 07:10:06 tls Exp $	*/
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
	struct symtab *q;
	NODE *r, *l;
	TWORD t;
	int o;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal\n");
		fwalk(p, eprint, 0);
	}
#endif

	switch( o = p->n_op ){
	case NAME:
		/* handle variables */
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */
		switch (q->sclass) {
		case PARAM:
		case AUTO:
			if (0 && q->soffset < MAXZP * SZINT &&
			    q->sclass != PARAM) {
				p->n_lval = -(q->soffset/SZCHAR) + ZPOFF*2;
				p->n_sp = NULL;
			} else {
				/* fake up a structure reference */
				r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
				r->n_lval = 0;
				r->n_rval = FPREG;
				p = stref(block(STREF, r, p, 0, 0, 0));
			}
			break;
		default:
			break;
		}
		break;

	case PMCONV:
	case PVCONV:
                if( p->n_right->n_op != ICON ) cerror( "bad conversion", 0);
                nfree(p);
                p = (buildtree(o==PMCONV?MUL:DIV, p->n_left, p->n_right));
		break;

	case PCONV:
		t = p->n_type;
		if (t == INCREF(CHAR) || t == INCREF(UCHAR) ||
		    t == INCREF(BOOL) || t == INCREF(VOID))
			break;
		l = p->n_left;
		t = l->n_type;
		if (t == INCREF(CHAR) || t == INCREF(UCHAR) ||
		    t == INCREF(BOOL) || t == INCREF(VOID))
			break;
		if (p->n_type <= UCHAR || l->n_type <= UCHAR)
			break; /* must do runtime ptr conv */
		/* if conversion to another pointer type, just remove */
		if (p->n_type > BTMASK && l->n_type > BTMASK)
			goto delp;
		if (l->n_op == ICON && l->n_sp == NULL)
			goto delp;
		break;

	delp:	l->n_type = p->n_type;
		l->n_qual = p->n_qual;
		l->n_df = p->n_df;
		l->n_ap = p->n_ap;
		p = nfree(p);
		break;
	}

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end\n");
		fwalk(p, eprint, 0);
	}
#endif

#if 0
	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	register int m;
	TWORD t;

//printf("in:\n");
//fwalk(p, eprint, 0);
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

	case STCALL:
	case CALL:
		/* Fix function call arguments. On x86, just add funarg */
		for (r = p->n_right; r->n_op == CM; r = r->n_left) {
			if (r->n_right->n_op != STARG &&
			    r->n_right->n_op != FUNARG)
				r->n_right = block(FUNARG, r->n_right, NIL, 
				    r->n_right->n_type, r->n_right->n_df,
				    r->n_right->n_sue);
		}
		if (r->n_op != STARG && r->n_op != FUNARG) {
			l = talloc();
			*l = *r;
			r->n_op = FUNARG; r->n_left = l; r->n_type = l->n_type;
		}
		break;
		
	case CBRANCH:
		l = p->n_left;

		/*
		 * Remove unnecessary conversion ops.
		 */
		if (clogop(l->n_op) && l->n_left->n_op == SCONV) {
			if (coptype(l->n_op) != BITYPE)
				break;
			if (l->n_right->n_op == ICON) {
				r = l->n_left->n_left;
				if (r->n_type >= FLOAT && r->n_type <= LDOUBLE)
					break;
				/* Type must be correct */
				t = r->n_type;
				nfree(l->n_left);
				l->n_left = r;
				l->n_type = t;
				l->n_right->n_type = t;
			}
		}
		break;

	case PCONV:
		/* Remove redundant PCONV's. Be careful */
		l = p->n_left;
		if (l->n_op == ICON) {
			l->n_lval = (unsigned)l->n_lval;
			goto delp;
		}
		if (l->n_type < INT || l->n_type == LONGLONG || 
		    l->n_type == ULONGLONG) {
			/* float etc? */
			p->n_left = block(SCONV, l, NIL,
			    UNSIGNED, 0, 0);
			break;
		}
		/* if left is SCONV, cannot remove */
		if (l->n_op == SCONV)
			break;
		/* if conversion to another pointer type, just remove */
		if (p->n_type > BTMASK && l->n_type > BTMASK)
			goto delp;
		break;

	delp:	l->n_type = p->n_type;
		l->n_qual = p->n_qual;
		l->n_df = p->n_df;
		l->n_sue = p->n_sue;
		nfree(p);
		p = l;
		break;

	case SCONV:
		l = p->n_left;

		if (p->n_type == l->n_type) {
			nfree(p);
			return l;
		}

		if ((p->n_type & TMASK) == 0 && (l->n_type & TMASK) == 0 &&
		    btdims[p->n_type].suesize == btdims[l->n_type].suesize) {
			if (p->n_type != FLOAT && p->n_type != DOUBLE &&
			    l->n_type != FLOAT && l->n_type != DOUBLE &&
			    l->n_type != LDOUBLE && p->n_type != LDOUBLE) {
				if (l->n_op == NAME || l->n_op == UMUL ||
				    l->n_op == TEMP) {
					l->n_type = p->n_type;
					nfree(p);
					return l;
				}
			}
		}

		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT &&
		    coptype(l->n_op) == BITYPE) {
			l->n_type = p->n_type;
			nfree(p);
			return l;
		}

		o = l->n_op;
		m = p->n_type;

		if (o == ICON) {
			CONSZ val = l->n_lval;

			if (!ISPTR(m)) /* Pointers don't need to be conv'd */
			    switch (m) {
			case CHAR:
				l->n_lval = (char)val;
				break;
			case UCHAR:
				l->n_lval = val & 0377;
				break;
			case SHORT:
				l->n_lval = (short)val;
				break;
			case USHORT:
				l->n_lval = val & 0177777;
				break;
			case ULONG:
			case UNSIGNED:
				l->n_lval = val & 0xffffffff;
				break;
			case LONG:
			case INT:
				l->n_lval = (int)val;
				break;
			case LONGLONG:
				l->n_lval = (long long)val;
				break;
			case ULONGLONG:
				l->n_lval = val;
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
				cerror("unknown type %d", m);
			}
			l->n_type = m;
			l->n_sue = 0;
			nfree(p);
			return l;
		}
		if (DEUNSIGN(p->n_type) == SHORT &&
		    DEUNSIGN(l->n_type) == SHORT) {
			nfree(p);
			p = l;
		}
		if ((p->n_type == CHAR || p->n_type == UCHAR ||
		    p->n_type == SHORT || p->n_type == USHORT) &&
		    (l->n_type == FLOAT || l->n_type == DOUBLE ||
		    l->n_type == LDOUBLE)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_sue);
			p->n_left->n_type = INT;
			return p;
		}
		break;

	case MOD:
	case DIV:
		if (o == DIV && p->n_type != CHAR && p->n_type != SHORT)
			break;
		if (o == MOD && p->n_type != CHAR && p->n_type != SHORT)
			break;
		/* make it an int division by inserting conversions */
		p->n_left = block(SCONV, p->n_left, NIL, INT, 0, 0);
		p->n_right = block(SCONV, p->n_right, NIL, INT, 0, 0);
		p = block(SCONV, p, NIL, p->n_type, 0, 0);
		p->n_left->n_type = INT;
		break;

	case PMCONV:
	case PVCONV:
                if( p->n_right->n_op != ICON ) cerror( "bad conversion", 0);
                nfree(p);
                return(buildtree(o==PMCONV?MUL:DIV, p->n_left, p->n_right));

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = RETREG(p->n_type);
		break;

	case LS:
	case RS:
		/* shift count must be in a char
		 * unless longlong, where it must be int */
		if (p->n_right->n_op == ICON)
			break; /* do not do anything */
		if (p->n_type == LONGLONG || p->n_type == ULONGLONG) {
			if (p->n_right->n_type != INT)
				p->n_right = block(SCONV, p->n_right, NIL,
				    INT, 0, 0);
			break;
		}
		if (p->n_right->n_type == CHAR || p->n_right->n_type == UCHAR)
			break;
		p->n_right = block(SCONV, p->n_right, NIL,
		    CHAR, 0, 0);
		break;
	}
//printf("ut:\n");
//fwalk(p, eprint, 0);

#endif

	return(p);
}

void
myp2tree(NODE *p)
{
	struct symtab *sp;
	NODE *l, *r;
	int o = p->n_op;

	switch (o) {
	case NAME: /* reading from a name must be done with a subroutine */
		if (p->n_type != CHAR && p->n_type != UCHAR)
			break;
		l = buildtree(ADDROF, ccopy(p), NIL);
		r = block(NAME, NIL, NIL, INT, 0, 0);

		r->n_sp = lookup(addname("__nova_rbyte"), SNORMAL);
		if (r->n_sp->sclass == SNULL) {
			r->n_sp->sclass = EXTERN;
			r->n_sp->stype = INCREF(p->n_type)+(FTN-PTR);
		}
		r->n_type = r->n_sp->stype;
		r = clocal(r);
		r = optim(buildtree(CALL, r, l));
		*p = *r;
		nfree(r);
		break;

	case FCON:
		sp = inlalloc(sizeof(struct symtab));
		sp->sclass = STATIC;
		sp->sap = 0;
		sp->slevel = 1; /* fake numeric label */
		sp->soffset = getlab();
		sp->sflags = 0;
		sp->stype = p->n_type;
		sp->squal = (CON >> TSHIFT);

		defloc(sp);
		ninval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

		p->n_op = NAME;
		p->n_lval = 0;
		p->n_sp = sp;
	}
}

/*ARGSUSED*/
int
andable(NODE *p)
{
	return(1);  /* all names can have & taken on them */
}

/*
 * Return 1 if a variable of type type is OK to put in register.
 */
int
cisreg(TWORD t)
{
	return 1; /* try to put anything in a register */
}

/*
 * return a node, for structure references, which is suitable for
 * being added to a pointer of type t, in order to be off bits offset
 * into a structure
 * t, d, and s are the type, dimension offset, and sizeoffset
 * For nova, return the type-specific index number which calculation
 * is based on its size. For example, char a[3] would return 3.
 * Be careful about only handling first-level pointers, the following
 * indirections must be fullword.
 */
NODE *
offcon(OFFSZ off, TWORD t, union dimfun *d, struct attr *ap)
{
	register NODE *p;

	if (xdebug)
		printf("offcon: OFFSZ %ld type %x dim %p siz %ld\n",
		    off, t, d, tsize(t, d, ap));

	p = bcon(off/SZINT);
	if (t == INCREF(CHAR) || t == INCREF(UCHAR) || t == INCREF(VOID))
		p->n_lval = off/SZCHAR; /* pointer to char */
	return(p);
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

cerror("spalloc");
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
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_ap);
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
ninval(CONSZ off, int fsz, NODE *p)
{
	switch (p->n_type) {
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
		cerror("ninval");
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
	case LONGLONG:
		MODTYPE(type,LONG);
		break;

	case ULONGLONG:
		MODTYPE(type,ULONG);
		break;
	case SHORT:
		MODTYPE(type,INT);
		break;
	case USHORT:
		MODTYPE(type,UNSIGNED);
		break;
	}
	return (type);
}

#if 0
/* curid is a variable which is defined but
 * is not initialized (and not a function );
 * This routine returns the storage class for an uninitialized declaration
 */
int
noinit()
{
	return(EXTERN);
}
#endif

void
calldec(NODE *p, NODE *q) 
{
}

void
extdec(struct symtab *q)
{
}

#if 0
/* make a common declaration for id, if reasonable */
void
commdec(struct symtab *q)
{
	int off;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;
	printf("	.comm %s,0%o\n", exname(q->soname), off);
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

static char *loctbl[] = { "text", "data", "section .rodata", "section .rodata" };

void
setloc1(int locc)
{
	if (locc == lastloc)
		return;
	lastloc = locc;
	printf("	.%s\n", loctbl[locc]);
}
#endif

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
 * On Nova we put symbols over the size of an int above 24 bytes in
 * offset and leave zeropage for small vars.
 */
void
fixdef(struct symtab *sp)
{
#if 0
	if (sp->sclass != AUTO)
		return; /* not our business */
	if (ISPTR(sp->stype) || sp->stype < LONG)
		return;
	if (sp->soffset >= (MAXZP * SZINT))
		return; /* already above */
	/* have to move */
	/* XXX remember old autooff for reorg of smaller vars */
	if (autooff < MAXZP * SZINT)
		autooff = MAXZP * SZINT;
	oalloc(sp, &autooff);
#endif
}

void
pass1_lastchance(struct interpass *ip)
{
}
