/*	Id: local.c,v 1.78 2014/05/29 19:20:03 plunky Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.5 2014/07/24 19:19:10 plunky Exp $	*/
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

static int pointp(TWORD t);
static struct symtab *newfun(char *name, TWORD type);

#define	PTRNORMAL	1
#define	PTRCHAR		2
#define	PTRSHORT	3
static int xptype(TWORD t);

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

	register struct symtab *q;
	register NODE *r, *l, *oop;
	register int o;
	register int m, ml;
	int siz;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif

	switch( o = p->n_op ){

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */

		switch (q->sclass) {

		case PARAM:
			/* First 7 parameters are in registers */
			/* XXX last may be double */
			if (q->soffset/SZINT < 7) {
				p->n_op = REG;
				p->n_rval = q->soffset/SZINT;
				break;
			} else
				q->soffset -= 7*SZINT;
				
		case AUTO:
			/* fake up a structure reference */
			if (q->stype == CHAR || q->stype == UCHAR ||
			    q->stype == SHORT || q->stype == USHORT)
				r = block(REG, NIL, NIL, PTR+q->stype, 0, 0);
			else
				r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			r->n_lval = 0;
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case STATIC:
			if (q->slevel == 0)
				break;
			p->n_lval = 0;
			break;

		case REGISTER:
			p->n_op = REG;
			p->n_lval = 0;
			p->n_rval = q->soffset;
			break;

			}
		break;

	case CALL:
		/* avoid recursive calls */
		r = tempnode(0, p->n_type, p->n_df, p->n_sue);
		l = tempnode(regno(r), p->n_type, p->n_df, p->n_sue);
		ecomp(buildtree(ASSIGN, r, p));
		p = l;
		break;

	case PCONV:
		l = p->n_left;
		/*
		 * Handle frame pointer directly without conversion,
		 * for efficiency.
		 */
		if (l->n_op == REG && l->n_rval == 0) {
rmpc:			l->n_type = p->n_type;
			l->n_df = p->n_df;
			l->n_sue = p->n_sue;
			nfree(p);
			return l;
		}
		/* Convert ICON with name to new type */
		if (l->n_op == ICON && l->n_sp != NULL &&
		    l->n_type == INCREF(STRTY) && 
		    (p->n_type == INCREF(CHAR) ||
		    p->n_type == INCREF(UCHAR) ||
		    p->n_type == INCREF(SHORT) ||
		    p->n_type == INCREF(USHORT))) {
			l->n_lval *= (BTYPE(p->n_type) == CHAR ||
			    BTYPE(p->n_type) == UCHAR ? 4 : 2);
			goto rmpc;
		}
		/* Convert only address constants, never convert other */
		if (l->n_op == ICON) {
			if (l->n_sp == NULL)
				goto rmpc;
			if (p->n_type == INCREF(CHAR) ||
			    p->n_type == INCREF(UCHAR) ||
			    p->n_type == INCREF(VOID))
				l->n_lval = (l->n_lval & 07777777777) |
				    0700000000000LL;
			else if (p->n_type == INCREF(SHORT) ||
			    p->n_type == INCREF(USHORT))
				l->n_lval = (l->n_lval & 07777777777) |
				    0750000000000LL;
			else
				l->n_lval = l->n_lval & 07777777777;
			goto rmpc;
		}

		/* Remove more conversions of identical pointers */
		/* Be careful! optim() may do bad things */
		if (ISPTR(DECREF(p->n_type))) {
			if (ISPTR(DECREF(l->n_type))) {
				if ((coptype(l->n_op) == UTYPE ||
				    coptype(l->n_op) == BITYPE) &&
				    (l->n_left->n_op == REG))
					l->n_left->n_type = p->n_type;
				goto rmpc;
			}
		}

		/* Change PCONV from int to double pointer to right shift */
		if (ISPTR(p->n_type) && ISPTR(DECREF(p->n_type)) &&
		    (l->n_type == INT || l->n_type == UNSIGNED)) {
			p->n_op = RS;
			p->n_right = bcon(2);
			break;
		}
		
		/* Check for cast integral -> pointer */
		if (BTYPE(l->n_type) == l->n_type)
			break;

		/* Remove conversions to identical pointers */
		switch (xptype(p->n_type)) {
		case PTRNORMAL:
			if (xptype(l->n_type) == PTRNORMAL)
				goto rmpc;
			break;

		case PTRSHORT:
			if (xptype(l->n_type) == PTRSHORT)
				goto rmpc;
			break;

		case PTRCHAR:
			if (xptype(l->n_type) == PTRCHAR)
				goto rmpc;
			break;
		}

		break;

	case SCONV:
		l = p->n_left;

		if ((p->n_type & TMASK) == 0 && (l->n_type & TMASK) == 0 &&
		    btdims[p->n_type].suesize == btdims[l->n_type].suesize) {
			if (p->n_type != FLOAT && p->n_type != DOUBLE &&
			     l->n_type != FLOAT && l->n_type != DOUBLE) {
				nfree(p);
				return l;
			}
		}
		/* cast to (void) XXX should be removed in MI code */
		if (p->n_type == VOID) {
			nfree(p);
			return l;
		}
		m = p->n_type;
		ml = l->n_type;
		if (m == ml) {
			nfree(p);
			return l;
		}
		o = l->n_op;
		if (ml == FLOAT || ml == DOUBLE) {
			if (o != FCON)
				break;
			ml = ISUNSIGNED(m) ? UNSIGNED : INT; /* LONG? */
			r = xbcon(ml == INT ? (int)p->n_left->n_dcon :
			                      (unsigned)p->n_left->n_dcon,
			          NULL, ml);
			nfree(p->n_left);
			p->n_left = r;
			o = ICON;
			if (m == ml) {
				r = p->n_left;
				nfree(p);
				return r;
			}
		}
		if (o == ICON) {
			CONSZ val = l->n_lval;

			switch (m) {
			case CHAR:
				l->n_lval = val & 0777;
				if (val & 0400)
					l->n_lval |= ~((CONSZ)0777);
				break;
			case UCHAR:
				l->n_lval = val & 0777;
				break;
			case USHORT:
				l->n_lval = val & 0777777;
				break;
			case SHORT:
				l->n_lval = val & 0777777;
				if (val & 0400000)
					l->n_lval |= ~((CONSZ)0777777);
				break;
			case UNSIGNED:
				l->n_lval = val & 0777777777777LL;
				break;
			case INT:
				l->n_lval = val & 0777777777777LL;
				if (val & 0400000000000LL)
					l->n_lval |= ~(0777777777777LL);
				break;
			case LONGLONG:	/* XXX */
			case ULONGLONG:
				l->n_lval = val;
				break;
			case VOID:
				break;
			case DOUBLE:
			case FLOAT:
				l->n_op = FCON;
				l->n_dcon = 0;
				break;
			default:
				cerror("unknown type %d", m);
			}
			l->n_type = m;
			l->n_sue = 0;
			nfree(p);
			return l;
		}
		break;

	case PMCONV:
	case PVCONV:
/*                if( p->n_right->n_op != ICON ) cerror( "bad conversion", 0); */
                nfree(p);
                return(buildtree(o==PMCONV?MUL:DIV, p->n_left, p->n_right));

	case RS:
	case RSEQ:
		/* convert >> to << with negative shift count */
		/* Beware! constant shifts will be converted back in optim() */

		if (p->n_right->n_op != UMINUS) {
			p->n_right = buildtree(UMINUS, p->n_right, NIL);
		} else {
			r = p->n_right;
			p->n_right = p->n_right->n_left;
			nfree(r);
		}
		if (p->n_op == RS)
			p->n_op = LS;
		else
			p->n_op = LSEQ;
		break;

	case UMUL: /* Convert structure assignment to memcpy() */
		if (p->n_left->n_op == PLUS &&
		    p->n_left->n_left->n_op == PCONV &&
		    p->n_left->n_right->n_op == ICON &&
		    (p->n_type == CHAR || p->n_type == UCHAR ||
		    p->n_type == SHORT || p->n_type == USHORT)) {
			/* Can remove the left SCONV */
			l = p->n_left->n_left;
			p->n_left->n_left = l->n_left;
			nfree(l);
			break;

		}
		if (p->n_left->n_op != STASG)
			break;
		oop = p;
		p = p->n_left;
		siz = p->n_sue->suesize/SZCHAR;
		l = p->n_left;
		r = p->n_right;
		if (l->n_type == STRTY || l->n_type == UNIONTY) {
			if (l->n_op == UMUL) {
				p->n_left = l->n_left;
				nfree(l);
				l = p->n_left;
			} else {
				l = block(ADDROF, l, NIL, INCREF(l->n_type),
				    0, 0);
			}
		}
		if ((l->n_type != (STRTY+PTR) && l->n_type != (UNIONTY+PTR)) ||
		    (r->n_type != (STRTY+PTR) && r->n_type != (UNIONTY+PTR)))
			cerror("bad stasg, l = %o, r = %o", l->n_type, r->n_type);
		q = newfun("__structcpy", p->n_type);

		/* structure pointer block */
		l = block(CM, l, r, INT, 0, 0);
		/* Size block */
		r = block(CM, l, bcon(siz), INT, 0, 0);

		l = xbcon(0, q, q->stype);
		p->n_left = l;
		p->n_right = r;
		p->n_op = CALL;
		oop->n_left = p;
		return oop;

	case FORCE:
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = RETREG(p->n_type);
		break;

	}

	return(p);
}

void
myp2tree(NODE *p)
{
	NODE *r;

	switch (p->n_op) {
	case ULT: /* exor sign bit to avoid unsigned comparitions */
	case ULE:
	case UGT:
	case UGE:
		if (ISLONGLONG(p->n_left->n_type)) {
			/* XXX */
			r = xbcon(0x8000000000000000ULL, NULL, LONGLONG);
		} else
			r = xbcon(0400000000000LL, NULL, INT);
		p->n_left = buildtree(ER, p->n_left, r);
		if (ISUNSIGNED(p->n_left->n_type))
			p->n_left->n_type = DEUNSIGN(p->n_left->n_type);

		if (ISLONGLONG(p->n_right->n_type)) {
			/* XXX */
			r = xbcon(0x8000000000000000ULL, NULL, LONGLONG);
		} else
			r = xbcon(0400000000000LL, NULL, INT);
		p->n_right = buildtree(ER, p->n_right, r);
		if (ISUNSIGNED(p->n_right->n_type))
			p->n_right->n_type = DEUNSIGN(p->n_right->n_type);

		p->n_op -= (ULT-LT);
		break;
	case FCON:
		cerror("fix float constants");
	}
}


struct symtab *
newfun(char *name, TWORD type)
{
	struct symtab *sp;

	sp = lookup(name, 0);
	if (sp->stype == VOID) {
		sp->stype = INCREF(type | FTN);
		sp->sclass = EXTERN;
		sp->soffset = 0;
	}
#ifdef notdef
	else if (!ISFTN(DECREF(sp->stype)))
		uerror("reserved name '%s' used illegally", name);
#endif
	return sp;
}

/*ARGSUSED*/
int
andable(NODE *p)
{
	return(1);  /* all names can have & taken on them */
}

/*
 * is an automatic variable of type t OK for a register variable
 * Everything is trusted to be in register here.
 */
int
cisreg(TWORD t)
{
	return(1);
}

int
xptype(TWORD t)
{
	int tt = BTYPE(t);
	int e, rv;

	if (!ISPTR(t))
		cerror("not a pointer");

	e = t & ~BTMASK;
	rv = e;
	while (e) {
		rv = e;
		if (DECREF(e) == 0)
			break;
		e = DECREF(e);
	}
	if (ISFTN(rv))
		return PTRNORMAL;

	switch (tt) {
	case INT:
	case LONG:
	case LONGLONG:
	case FLOAT:
	case DOUBLE:
	case STRTY:
	case UNIONTY:
	case UNSIGNED:
	case ULONG:
	case ULONGLONG:
		return PTRNORMAL;
	case VOID:
	case CHAR:
	case UCHAR:
		if (DECREF(t) == tt || ISARY(rv))
			return PTRCHAR;
		return PTRNORMAL;
	case SHORT:
	case USHORT:
		if (DECREF(t) == tt || ISARY(rv))
			return PTRSHORT;
		return PTRNORMAL;
	default:
		break;
	}
	cerror("unknown type");
	return PTRNORMAL; /* XXX */
}

/*
 * Help routine to the one below; return true if it's not a word pointer.
 */
static int
pointp(TWORD t)
{
	int rv = 0;

	if (ISPTR(t) && ((t & TMASK1) == 0))
		return 1;

	t &= ~BTMASK;
	while (t) {
		rv = ISARY(t);
		t = DECREF(t);
	}
	return rv;
}

/*
 * return a node, for structure references, which is suitable for
 * being added to a pointer of type t, in order to be off bits offset
 * into a structure
 * t, d, and s are the type, dimension offset, and sizeoffset
 * For pdp10, return the type-specific index number which calculation
 * is based on its size. For example, short a[3] would return 3.
 * Be careful about only handling first-level pointers, the following
 * indirections must be fullword.
 */
NODE *
offcon(OFFSZ off, TWORD t, union dimfun *d, struct suedef *sue)
{
	register NODE *p;

	if (xdebug)
		printf("offcon: OFFSZ %lld type %x dim %p siz %d\n",
		    off, t, d, sue->suesize);

	p = bcon(0);
	p->n_lval = off/SZINT;	/* Default */
	if (ISPTR(DECREF(t)))
		return p;	/* Pointer/pointer reference */
	switch (BTMASK & t) {
	case INT:
	case UNSIGNED:
	case LONG:
	case ULONG:
	case STRTY:
	case UNIONTY:
	case LONGLONG:
	case ULONGLONG:
	case FLOAT:
	case DOUBLE:
		break;

	case SHORT:
	case USHORT:
		if (pointp(t))
			p->n_lval = off/SZSHORT;
		break;

	case VOID: /* void pointers */
	case CHAR:
	case UCHAR:
		if (pointp(t))
			p->n_lval = off/SZCHAR;
		break;

	default:
		cerror("offcon, off %llo size %d type %x", off, sue->suesize, t);
	}
	if (xdebug)
		printf("offcon return 0%llo\n", p->n_lval);
	return(p);
}

/*
 * Allocate off bits on the stack.  p is a tree that when evaluated
 * is the multiply count for off, t is a NAME node where to write
 * the allocated address.
 * Be aware that a pointer conversion may be needed when saving 
 * to node t!
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
	/* Cast sp to destination type (may be redundant) */
	sp = buildtree(CAST,
	    block(NAME, NIL, NIL, t->n_type, t->n_df, t->n_sue), sp);
	nfree(sp->n_left);
	nfree(sp);
	sp = sp->n_right;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

	/* add the size to sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(PLUSEQ, sp, p));
}

#if 0
static int inwd;	/* current bit offsed in word */
static CONSZ word;	/* word being built from fields */

/*
 * Generate initialization code for assigning a constant c
 * to a field of width sz
 * we assume that the proper alignment has been obtained
 * inoff is updated to have the proper final value
 * we also assume sz  < SZINT
 */
void
incode(NODE *p, int sz)
{
	char *s;

	inoff += sz;
	if ((sz + inwd) > SZINT)
		cerror("incode: field > int");

	word |= ((p->n_lval & ((1 << sz) - 1)) << (36 - inwd - sz));

	inwd += sz;
	if (inoff % SZINT == 0) {
		s = isinlining ? permalloc(30) : tmpalloc(30);
		sprintf(s, "\t.long 0%llo\n", word);
		send_passt(IP_ASM, s);
		word = inwd = 0;
	}
	tfree(p);
}

/* output code to initialize space of size sz to the value d */
/* the proper alignment has been obtained */
/* inoff is updated to have the proper final value */
/* on the target machine, write it out in octal! */
void
fincode(NODE *p, int sz)
{
	double d = p->n_dcon;

	if(!nerrors)
		printf("	%s	0%c%.20e\n",
		    sz == SZDOUBLE ? ".double" : ".float",
		sz == SZDOUBLE ? 'd' : 'f', d);
	inoff += sz;
}

void
cinit(NODE *p, int sz)
{
	NODE *l;

	/*
	 * as a favor (?) to people who want to write
	 *     int i = 9600/134.5;
	 * we will, under the proper circumstances, do
	 * a coercion here.
	 */
	switch (p->n_type) {
	case INT:
	case UNSIGNED:
		l = p->n_left;
		if (l->n_op != SCONV || l->n_left->n_op != FCON)
			break;
		nfree(l);
		l = l->n_left;
		l->n_lval = (long)(l->n_dcon);
		l->n_sp = NULL;
		l->n_op = ICON;
		l->n_type = INT;
		p->n_left = l;
		break;
	}
	/* arrange for the initialization of p into a space of size sz */
	/* the proper alignment has been opbtained */
	/* inoff is updated to have the proper final value */
	ecode( p );
	inoff += sz;
}

/*
 * define n bits of zeros in a vfd
 */
void
vfdzero(int n)
{
	char *s;

	inoff += n;
	inwd += n;
	if (inoff%ALINT ==0) {
		s = isinlining ? permalloc(30) : tmpalloc(30);
		sprintf(s, "\t.long 0%llo\n", word);
		send_passt(IP_ASM, s);
		word = inwd = 0;
	}
}
#endif

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
	case LONG:
		MODTYPE(type,INT);
		break;
	case ULONG:
		MODTYPE(type,UNSIGNED);
		break;
	case LDOUBLE:
		MODTYPE(type,DOUBLE);
		break;
	}
	return (type);
}

/* curid is a variable which is defined but
 * is not initialized (and not a function );
 * This routine returns the stroage class for an uninitialized declaration
 */
int
noinit()
{
	return(EXTERN);
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
	int off;
 
	off = tsize(sp->stype, sp->sdf, sp->ssue);
	off = (off+(SZINT-1))/SZINT;
	printf("        .%scomm ", sp->sclass == STATIC ? "l" : "");
	if (sp->slevel == 0)
		printf("%s,0%o\n", exname(sp->soname), off);
	else
		printf(LABFMT ",0%o\n", sp->soffset, off);
}

/*
 * set fsz bits in sequence to zero.
 */
void
zbits(OFFSZ off, int fsz)
{
	cerror("zbits");
}

/*
 * Initialize a bitfield.
 */
void
infld(CONSZ off, int fsz, CONSZ val)
{
//	if (idebug)
//		printf("infld off %lld, fsz %d, val %lld inbits %d\n",
//		    off, fsz, val, inbits);
	cerror("infld");
}

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
	cerror("ninval");
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

