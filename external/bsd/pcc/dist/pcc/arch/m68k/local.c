/*	Id: local.c,v 1.16 2016/01/30 17:26:19 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.2 2016/02/09 20:28:34 plunky Exp $	*/
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


#include "pass1.h"

#undef NIL
#define NIL NULL

#ifdef LANG_CXX
#define P1ND NODE
#define p1nfree nfree
#define p1fwalk fwalk
#define p1tcopy tcopy
#define p1alloc talloc
#else
#define	NODE P1ND
#define	nfree p1nfree
#define	fwalk p1fwalk
#endif


/*	this file contains code which is dependent on the target machine */

int gotnr;

/*
 * Make a symtab entry for PIC use.
 */
static struct symtab *
picsymtab(char *p, char *s, char *s2)
{
	struct symtab *sp = permalloc(sizeof(struct symtab));
	size_t len = strlen(p) + strlen(s) + strlen(s2) + 1;
	
	sp->sname = permalloc(len);
	strlcpy(sp->sname, p, len);
	strlcat(sp->sname, s, len);
	strlcat(sp->sname, s2, len);
	sp->sap = attr_new(ATTR_SONAME, 1);
	sp->sap->sarg(0) = sp->sname;
	sp->sclass = EXTERN;
	sp->sflags = sp->slevel = 0;
	sp->stype = 0xdeadbeef;
	return sp;
}

/*
 * Create a reference for an extern variable.
 */
static NODE *
picext(NODE *p)
{
	NODE *q, *r;
	struct symtab *sp;
	char *name;

	q = tempnode(gotnr, PTR|VOID, 0, 0);
	name = getexname(p->n_sp);

#ifdef notdef
	struct attr *ga;
	if ((ga = attr_find(p->n_sp->sap, GCC_ATYP_VISIBILITY)) &&
	    strcmp(ga->sarg(0), "hidden") == 0) {
		/* For hidden vars use GOTOFF */
		sp = picsymtab("", name, "@GOTOFF");
		r = xbcon(0, sp, INT);
		q = buildtree(PLUS, q, r);
		q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
		q->n_sp = p->n_sp; /* for init */
		nfree(p);
		return q;
	}
#endif

	sp = picsymtab("", name, "@GOT");
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, PTR|VOID, 0, 0);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	nfree(p);
	return q;
}

static char *
getsoname(struct symtab *sp)
{
	struct attr *ap;
	return (ap = attr_find(sp->sap, ATTR_SONAME)) ?
	    ap->sarg(0) : sp->sname;
	
}


static NODE *
picstatic(NODE *p)
{
	NODE *q, *r;
	struct symtab *sp;

	q = tempnode(gotnr, PTR|VOID, 0, 0);
	if (p->n_sp->slevel > 0) {
		char buf[32];
		if ((p->n_sp->sflags & SMASK) == SSTRING)
			p->n_sp->sflags |= SASG;
		snprintf(buf, 32, LABFMT, (int)p->n_sp->soffset);
		sp = picsymtab("", buf, "@GOT");
	} else {
		sp = picsymtab("", getsoname(p->n_sp), "@GOT");
	}
	
	sp->sclass = STATIC;
	sp->stype = p->n_sp->stype;
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, PTR|VOID, 0, 0);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	nfree(p);
	return q;
}

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

	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	TWORD t;

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
			slval(r, 0);
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case REGISTER:
			p->n_op = REG;
			slval(p, 0);
			p->n_rval = q->soffset;
			break;

		case USTATIC:
		case STATIC:
			if (kflag == 0)
				break;
			if (blevel > 0 && !statinit)
				p = picstatic(p);
			break;

		case EXTERN:
		case EXTDEF:
			if (kflag == 0)
				break;
			if (blevel > 0 && !statinit)
				p = picext(p);
			break;
		}
		break;

	case ADDROF:
		if (kflag == 0 || blevel == 0 || statinit)
			break;
		/* char arrays may end up here */
		l = p->n_left;
		if (l->n_op != NAME ||
		    (l->n_type != ARY+CHAR && l->n_type != ARY+WCHAR_TYPE))
			break;
		l = p;
		p = picstatic(p->n_left);
		nfree(l);
		if (p->n_op != UMUL)
			cerror("ADDROF error");
		l = p;
		p = p->n_left;
		nfree(l);
		break;

	case STASG: /* convert struct assignment to call memcpy */
		l = p->n_left;
		if (l->n_op == NAME && ISFTN(l->n_sp->stype))
			break; /* struct return, do nothing */
		/* first construct arg list */
		p->n_left = buildtree(ADDROF, p->n_left, 0);
		r = bcon(tsize(STRTY, p->n_df, p->n_ap)/SZCHAR);
		p->n_left = buildtree(CM, p->n_left, p->n_right);
		p->n_right = r;
		p->n_op = CM;
		p->n_type = INT;

		r = block(NAME, NIL, NIL, INT, 0, 0);
		r->n_sp = lookup(addname("memcpy"), SNORMAL);
		if (r->n_sp->sclass == SNULL) {
			r->n_sp->sclass = EXTERN;
			r->n_sp->stype = INCREF(VOID+PTR)+(FTN-PTR);
		}
		r->n_type = r->n_sp->stype;
		p = buildtree(CALL, r, p);
		break;

	case SCONV:
		l = p->n_left;
		if (l->n_op == ICON && ISPTR(l->n_type)) {
			/* Do immediate cast here */
			/* Should be common code */
			q = l->n_sp;
			l->n_sp = NULL;
			l->n_type = UNSIGNED;
			if (concast(l, p->n_type) == 0)
				cerror("clocal");
			p = nfree(p);
			p->n_sp = q;
		}
		break;

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		t = p->n_type;
		if (ISITY(t))
			t = t - (FIMAG-FLOAT);
		p->n_left->n_rval = RETREG(t);
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

#define IALLOC(sz)	(isinlining ? permalloc(sz) : tmpalloc(sz))

void
myp2tree(NODE *p)
{
	struct symtab *sp;
	NODE *l;

	if (cdope(p->n_op) & CALLFLG) {
		if (p->n_left->n_op == ADDROF &&
		    p->n_left->n_left->n_op == NAME) {
			p->n_left = nfree(p->n_left);
			l = p->n_left;
			l->n_op = ICON;
			if (l->n_sp->sclass != STATIC &&
			    l->n_sp->sclass != USTATIC)
				l->n_sp =
				    picsymtab(l->n_sp->sname, "@PLTPC", "");
		}
	}

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
	sp->sname = NULL;

	locctr(DATA, sp);
	defloc(sp);
	ninval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

	p->n_op = NAME;
	slval(p, 0);
	p->n_sp = sp;

}

/*
 * Convert ADDROF NAME to ICON?
 */
int
andable(NODE *p)
{
#ifdef notdef
	/* shared libraries cannot have direct referenced static syms */
	if (p->n_sp->sclass == STATIC || p->n_sp->sclass == USTATIC)
		return 1;
#endif
	return 1;
}

/*
 * Return 1 if a variable of type type is OK to put in register.
 */
int
cisreg(TWORD t)
{
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

	p = buildtree(MUL, p, bcon(off/SZCHAR));
	p = buildtree(PLUS, p, bcon(30));
	p = buildtree(AND, p, xbcon(-16, NULL, UNSIGNED));
	p = cast(p, UNSIGNED, 0);

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, UNSIGNED+PTR, 0, 0);
	slval(sp, 0);
	sp->n_rval = STKREG;
	p = (buildtree(MINUSEQ, sp, p));
	ecomp(p);

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+UNSIGNED, t->n_df, t->n_ap);
	slval(sp, 0);
	sp->n_rval = STKREG;
	t->n_type = sp->n_type;
	p = (buildtree(ASSIGN, t, sp)); /* Emit! */
	ecomp(p);

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

	switch (p->n_type) {
	case LDOUBLE:
		u.i[2] = 0;
		u.l = (long double)((union flt *)p->n_dcon)->fp;
#if defined(HOST_LITTLE_ENDIAN)
		/* XXX probably broken on most hosts */
		printf("\t.long\t0x%x,0x%x,0x%x\n", u.i[2], u.i[1], u.i[0]);
#else
		printf("\t.long\t0x%x,0x%x,0x%x\n", u.i[0], u.i[1], u.i[2]);
#endif
		break;
	case DOUBLE:
		u.d = (double)((union flt *)p->n_dcon)->fp;
#if defined(HOST_LITTLE_ENDIAN)
		printf("\t.long\t0x%x,0x%x\n", u.i[1], u.i[0]);
#else
		printf("\t.long\t0x%x,0x%x\n", u.i[0], u.i[1]);
#endif
		break;
	case FLOAT:
		u.f = (float)((union flt *)p->n_dcon)->fp;
		printf("\t.long\t0x%x\n", u.i[0]);
		break;

	case LONGLONG:
	case ULONGLONG:
		printf("\t.long\t0x%x\n", (int)(off >> 32) & 0xffffffff);
		printf("\t.long\t0x%x\n", (int)(off) & 0xffffffff);
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
	return (p == NULL ? "" : p);
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
	int off, al;
	char *name;

	name = getexname(sp);
	off = tsize(sp->stype, sp->sdf, sp->sap);
	SETOFF(off,SZCHAR);
	off /= SZCHAR;
	al = talign(sp->stype, sp->sap)/SZCHAR;

	if (sp->sclass == STATIC) {
		if (sp->slevel == 0) {
			printf("\t.local %s\n", name);
		} else
			printf("\t.local " LABFMT "\n", sp->soffset);
	}
	if (sp->slevel == 0) {
		printf("\t.comm %s,0%o,%d\n", name, off, al);
	} else
		printf("\t.comm " LABFMT ",0%o,%d\n", sp->soffset, off, al);
}

char *nextsect;
static char *alias;
static int constructor;
static int destructor;

/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	return 0;
}

/*
 * Called when a identifier has been declared.
 */
void
fixdef(struct symtab *sp)
{
	struct attr *ga;

#ifdef HAVE_WEAKREF
	/* not many as'es have this directive */
	if ((ga = attr_find(sp->sap, GCC_ATYP_WEAKREF)) != NULL) {
		char *wr = ga->sarg(0);
		char *sn = getsoname(sp);
		if (wr == NULL) {
			if ((ga = attr_find(sp->sap, GCC_ATYP_ALIAS))) {
				wr = ga->sarg(0);
			}
		}
		if (wr == NULL)
			printf("\t.weak %s\n", sn);
		else
			printf("\t.weakref %s,%s\n", sn, wr);
	} else
	       if ((ga = attr_find(sp->sap, GCC_ATYP_ALIAS)) != NULL) {
		char *an = ga->sarg(0);
		char *sn = getsoname(sp);
		char *v;

		v = attr_find(sp->sap, GCC_ATYP_WEAK) ? "weak" : "globl";
		printf("\t.%s %s\n", v, sn);
		printf("\t.set %s,%s\n", sn, an);
	}
	if (alias != NULL && (sp->sclass != PARAM)) {
		char *name = getexname(sp);
		printf("\t.globl %s\n", name);
		printf("%s = ", name);
		printf("%s\n", exname(alias));
		alias = NULL;
	}
	if ((constructor || destructor) && (sp->sclass != PARAM)) {
		NODE *p = p1alloc();

		p->n_op = NAME;
		p->n_sp =
		  (struct symtab *)(constructor ? "constructor" : "destructor");
		sp->sap = attr_add(sp->sap, gcc_attr_parse(p));
		constructor = destructor = 0;
	}
#endif
}

void
pass1_lastchance(struct interpass *ip)
{
}
