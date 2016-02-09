/*	Id: local.c,v 1.3 2015/07/24 08:00:12 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.1 2016/02/09 20:28:38 plunky Exp $	*/
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


#include "pass1.h"

/*	this file contains code which is dependent on the target machine */

#ifdef notyet
/*
 * Check if a constant is too large for a type.
 */
static int
toolarge(TWORD t, CONSZ con)
{
	U_CONSZ ucon = con;

	switch (t) {
	case ULONGLONG:
	case LONGLONG:
		break; /* cannot be too large */
#define	SCHK(i)	case i: if (con > MAX_##i || con < MIN_##i) return 1; break
#define	UCHK(i)	case i: if (ucon > MAX_##i) return 1; break
	SCHK(INT);
	SCHK(SHORT);
	case BOOL:
	SCHK(CHAR);
	UCHK(UNSIGNED);
	UCHK(USHORT);
	UCHK(UCHAR);
	default:
		cerror("toolarge");
	}
	return 0;
}
#endif

#define	IALLOC(sz)	(isinlining ? permalloc(sz) : tmpalloc(sz))


int gotnr; /* tempnum for GOT register */
int argstacksize;

/* clocal() is called to do local transformations on
 * an expression tree preparitory to its being
 * written out in intermediate code.
 *
 * the major essential job is rewriting the
 * automatic variables and arguments in terms of
 * REG and OREG nodes
 * conversion ops which are not necessary are also clobbered here
 * in addition, any special features (such as rewriting
 * exclusive or) are easily handled here as well
 */
NODE *
clocal(NODE *p)
{

	struct attr *ap;
	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	register int m;

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
		case AUTO:
			/* fake up a structure reference */
			r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			r->n_lval = 0;
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case USTATIC:
			break;
		case STATIC:
		        break;

		case REGISTER:
			p->n_op = REG;
			p->n_lval = 0;
			p->n_rval = q->soffset;
			break;

		case EXTERN:
		case EXTDEF:
			break;
		}
		break;

	case ADDROF:
		break;

	case UCALL:
		break;

	case USTCALL:
		/* Add hidden arg0 */
		r = block(REG, NIL, NIL, INCREF(VOID), 0, 0);
		regno(r) = BP;
		if ((ap = attr_find(p->n_ap, GCC_ATYP_REGPARM)) != NULL &&
		    ap->iarg(0) > 0) {
			l = block(REG, NIL, NIL, INCREF(VOID), 0, 0);
			regno(l) = AX;
			p->n_right = buildtree(ASSIGN, l, r);
		} else
			p->n_right = block(FUNARG, r, NIL, INCREF(VOID), 0, 0);
		p->n_op -= (UCALL-CALL);

		if (kflag == 0)
			break;
		l = block(REG, NIL, NIL, INT, 0, 0);
		regno(l) = BX;
		r = buildtree(ASSIGN, l, tempnode(gotnr, INT, 0, 0));
		p->n_right = block(CM, r, p->n_right, INT, 0, 0);
		break;

#ifdef notyet
	/* XXX breaks sometimes */
	case CBRANCH:
		l = p->n_left;

		/*
		 * Remove unnecessary conversion ops.
		 */
		if (!clogop(l->n_op) || l->n_left->n_op != SCONV)
			break;
		if (coptype(l->n_op) != BITYPE)
			break;
		if (l->n_right->n_op != ICON)
			break;
		r = l->n_left->n_left;
		if (r->n_type >= FLOAT)
			break;
		if (toolarge(r->n_type, l->n_right->n_lval))
			break;
		l->n_right->n_type = r->n_type;
		if (l->n_op >= ULE && l->n_op <= UGT)
			l->n_op -= (UGT-ULE);
		p->n_left = buildtree(l->n_op, r, l->n_right);
		nfree(l->n_left);
		nfree(l);
		break;
#endif

	case PCONV:
		l = p->n_left;

		/* Make int type before pointer */
		if (l->n_type < INT || l->n_type == LONGLONG || 
		    l->n_type == ULONGLONG || l->n_type == BOOL) {
			/* float etc? */
			p->n_left = block(SCONV, l, NIL, UNSIGNED, 0, 0);
		}
		break;

	case SCONV:
		if (p->n_left->n_op == COMOP)
			break;  /* may propagate wrong type later */
		l = p->n_left;

		if (p->n_type == l->n_type) {
			nfree(p);
			return l;
		}

		if ((p->n_type & TMASK) == 0 && (l->n_type & TMASK) == 0 &&
		    tsize(p->n_type, p->n_df, p->n_ap) ==
		    tsize(l->n_type, l->n_df, l->n_ap)) {
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
		    coptype(l->n_op) == BITYPE && l->n_op != COMOP &&
		    l->n_op != QUEST) {
			l->n_type = p->n_type;
			nfree(p);
			return l;
		}

		o = l->n_op;
		m = p->n_type;

		if (o == ICON) {
			/*
			 * Can only end up here if o is an address,
			 * and in that case the only compile-time conversion
			 * possible is to int.
			 */
			if ((TMASK & l->n_type) == 0 && l->n_sp == NULL)
				cerror("SCONV ICON");
			if (l->n_sp == 0) {
				p->n_type = UNSIGNED;
				concast(l, m);
			} else if (m != INT && m != UNSIGNED)
				break;
			l->n_type = m;
			l->n_ap = 0;
			nfree(p);
			return l;
		} else if (l->n_op == FCON)
			cerror("SCONV FCON");
		if ((p->n_type == CHAR || p->n_type == UCHAR ||
		    p->n_type == SHORT || p->n_type == USHORT) &&
		    (l->n_type == FLOAT || l->n_type == DOUBLE ||
		    l->n_type == LDOUBLE)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_ap);
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
		p->n_left = makety(p->n_left, INT, 0, 0, 0);
		p->n_right = makety(p->n_right, INT, 0, 0, 0);
		o = p->n_type;
		p->n_type = INT;
		p = makety(p, o, 0, 0, 0);
		break;

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(CHAR) : RETREG(p->n_type);
		break;

	case LS:
	case RS:
		/* shift count must be in a char */
		if (p->n_right->n_type == CHAR || p->n_right->n_type == UCHAR)
			break;
		p->n_right = block(SCONV, p->n_right, NIL, CHAR, 0, 0);
		break;
	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	return(p);
}

static void mangle(NODE *p);

void
myp2tree(NODE *p)
{
	struct symtab *sp;

	mangle(p);

	if (p->n_op != FCON)
		return;

	sp = IALLOC(sizeof(struct symtab));
	sp->sclass = STATIC;
	sp->sap = 0;
	sp->slevel = 1; /* fake numeric label */
	sp->soffset = getlab();
	sp->sflags = 0;
	sp->stype = p->n_type;
	sp->squal = (CON >> TSHIFT);
	sp->sname = sp->soname = NULL;

	locctr(DATA, sp);
	defloc(sp);
	ninval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

	p->n_op = NAME;
	p->n_lval = 0;
	p->n_sp = sp;
}

/*ARGSUSED*/
int
andable(NODE *p)
{
	return(1);	/* all names can have & taken on them */
}

/*
 * Return 1 if a variable of type type is OK to put in register.
 */
int
cisreg(TWORD t)
{
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return 0; /* not yet */
	return 1;
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

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
	union { float f; double d; long double l; int i[3]; } u;
	int i;

	switch (p->n_type) {
	case LONGLONG:
	case ULONGLONG:
		i = (int)(p->n_lval >> 32);
		p->n_lval &= 0xffffffff;
		p->n_type = INT;
		inval(off, 32, p);
		p->n_lval = i;
		inval(off+32, 32, p);
		break;
	case LDOUBLE:
		u.i[2] = 0;
		u.l = (long double)p->n_dcon;
#if defined(HOST_BIG_ENDIAN)
		/* XXX probably broken on most hosts */
		printf("\t.long\t0x%x,0x%x,0x%x\n", u.i[2], u.i[1], u.i[0]);
#else
		printf("\t.long\t%d,%d,%d\n", u.i[0], u.i[1], u.i[2] & 0177777);
#endif
		break;
	case DOUBLE:
		u.d = (double)p->n_dcon;
#if defined(HOST_BIG_ENDIAN)
		printf("\t.long\t0x%x,0x%x\n", u.i[1], u.i[0]);
#else
		printf("\t.long\t%d,%d\n", u.i[0], u.i[1]);
#endif
		break;
	case FLOAT:
		u.f = (float)p->n_dcon;
		printf("\t.long\t%d\n", u.i[0]);
		break;
	default:
		return 0;
	}
	return 1;
}

/* make a name look like an external name in the local machine */
char *
exname(char *p)
{
#define NCHNAM  256
	static char text[NCHNAM+1];
	int i;

	if (p == NULL)
		return "";

	text[0] = '_';
	for (i=1; *p && i<NCHNAM; ++i)
		text[i] = *p++;

	text[i] = '\0';
	text[NCHNAM] = '\0';  /* truncate */

	return (text);
}

/*
 * map types which are not defined on the local machine
 *
 * For 8086 we map short/ushort onto int/unsigned int which
 * removes some of the table stuff later on
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

	}
	return (type);
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
	int al;
	char *name;

	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);
	al = talign(sp->stype, sp->sap)/SZCHAR;
	off = (int)tsize(sp->stype, sp->sdf, sp->sap);
	SETOFF(off,SZCHAR);
	off /= SZCHAR;
	printf("\t.align %d\n", al);
	printf("%s:\t.blkb %d\n", name, off);
}

static int stdcall;
static char *alias;
static int constructor;
static int destructor;

/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	char *a2 = pragtok(NULL);

	if (strcmp(str, "stdcall") == 0) {
		stdcall = 1;
		return 1;
	}
	if (strcmp(str, "cdecl") == 0) {
		stdcall = 0;
		return 1;
	}
#ifndef AOUTABI
	if (strcmp(str, "constructor") == 0 || strcmp(str, "init") == 0) {
		constructor = 1;
		return 1;
	}
	if (strcmp(str, "destructor") == 0 || strcmp(str, "fini") == 0) {
		destructor = 1;
		return 1;
	}
#endif
	if (strcmp(str, "alias") == 0 && a2 != NULL) {
		alias = tmpstrdup(a2);
		return 1;
	}

	return 0;
}

/*
 * Called when a identifier has been declared.
 */
void
fixdef(struct symtab *sp)
{
#ifdef GCC_COMPAT
	struct attr *ap;
#endif
#ifdef GCC_COMPAT
#ifdef HAVE_WEAKREF
	/* not many as'es have this directive */
	if ((ap = attr_find(sp->sap, GCC_ATYP_WEAKREF)) != NULL) {
		char *wr = ap->sarg(0);
		char *sn = sp->soname ? sp->soname : sp->sname;
		if (sp->sclass != STATIC && sp->sclass != USTATIC)
			uerror("weakref %s must be static", sp->sname);
		if (wr == NULL) {
			if ((ap = attr_find(sp->sap, GCC_ATYP_ALIAS))) {
				wr = ap->sarg(0);
			}
		}
		if (wr == NULL)
			printf("\t.weak %s\n", sn);
		else
			printf("\t.weakref %s,%s\n", sn, wr);
	} else
#endif
	    if ((ap = attr_find(sp->sap, GCC_ATYP_ALIAS)) != NULL) {
		char *an = ap->sarg(0);	 
		char *sn = sp->soname ? sp->soname : sp->sname; 
		char *v;

		v = attr_find(sp->sap, GCC_ATYP_WEAK) ? "weak" : "globl";
		printf("\t.%s %s\n", v, sn);
		printf("\t.set %s,%s\n", sn, an);
	}	
#endif
	if (alias != NULL && (sp->sclass != PARAM)) {
		char *name;
		if ((name = sp->soname) == NULL)
			name = exname(sp->sname);
		printf("\t.globl %s\n", name);
		printf("%s = ", name);
		printf("%s\n", exname(alias));
		alias = NULL;
	}
	if ((constructor || destructor) && (sp->sclass != PARAM)) {
		printf("\t.section .%ctors,\"w\"\n",
		    constructor ? 'c' : 'd');
		printf("\t.p2align 2\n");
		printf("\t.long %s\n", exname(sp->sname));
		printf("\t.previous\n");
		constructor = destructor = 0;
	}
	if (stdcall && (sp->sclass != PARAM)) {
		sp->sflags |= SSTDCALL;
		stdcall = 0;
	}
}

/*
 *  Postfix external functions with the arguments size.
 */
static void
mangle(NODE *p)
{
	NODE *l;

	if (p->n_op != CALL && p->n_op != STCALL &&
	    p->n_op != UCALL && p->n_op != USTCALL)
		return;

	l = p->n_left;
	while (cdope(l->n_op) & CALLFLG)
		l = l->n_left;
	if (l->n_op == TEMP)
		return;
	if (l->n_op == ADDROF)
		l = l->n_left;
	if (l->n_sp == NULL)
		return;
#ifdef GCC_COMPAT
	if (attr_find(l->n_sp->sap, GCC_ATYP_STDCALL) != NULL)
		l->n_sp->sflags |= SSTDCALL;
#endif
}

void
pass1_lastchance(struct interpass *ip)
{
	if (ip->type == IP_NODE &&
	    (ip->ip_node->n_op == CALL || ip->ip_node->n_op == UCALL) &&
	    ISFTY(ip->ip_node->n_type))
		ip->ip_node->n_ap = attr_add(ip->ip_node->n_ap,
		    attr_new(ATTR_I86_FPPOP, 1));
	if (ip->type == IP_EPILOG) {
		struct interpass_prolog *ipp = (struct interpass_prolog *)ip;
		ipp->ipp_argstacksize = argstacksize;
	}
}
