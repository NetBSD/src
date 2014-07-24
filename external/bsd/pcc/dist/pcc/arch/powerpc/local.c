/*	Id: local.c,v 1.34 2012/09/06 13:07:29 plunky Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.5 2014/07/24 19:20:27 plunky Exp $	*/
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

#include <assert.h>
#include "pass1.h"

#define IALLOC(sz) (isinlining ? permalloc(sz) : tmpalloc(sz))

extern int kflag;

static void simmod(NODE *p);

/*	this file contains code which is dependent on the target machine */

#if defined(MACHOABI)

/*
 *  Keep track of PIC stubs.
 */

void
addstub(struct stub *list, char *name)
{
        struct stub *s;

        DLIST_FOREACH(s, list, link) {
                if (strcmp(s->name, name) == 0)
                        return;
        }

        s = permalloc(sizeof(struct stub));
	s->name = newstring(name, strlen(name));
        DLIST_INSERT_BEFORE(list, s, link);
}

#endif


/*
 * Make a symtab entry for PIC use.
 */
static struct symtab *
picsymtab(char *p, char *s, char *s2)
{
	struct symtab *sp = IALLOC(sizeof(struct symtab));
	size_t len = strlen(p) + strlen(s) + strlen(s2) + 1;

	sp->sname = sp->soname = IALLOC(len);
	strlcpy(sp->soname, p, len);
	strlcat(sp->soname, s, len);
	strlcat(sp->soname, s2, len);
	sp->sclass = EXTERN;
	sp->sflags = sp->slevel = 0;

	return sp;
}

int gotnr;  /* tempnum for GOT register */

/*
 * Create a reference for an extern variable.
 */
static NODE *
picext(NODE *p)
{
	NODE *q;
	struct symtab *sp;
	char *name;

	name = p->n_sp->soname ? p->n_sp->soname : exname(p->n_sp->sname);

	if (strncmp(name, "__builtin", 9) == 0)
		return p;

#if defined(ELFABI)

	sp = picsymtab("", name, "@got(31)");
	q = xbcon(0, sp, PTR+VOID);
	q = block(UMUL, q, 0, PTR+VOID, 0, 0);

#elif defined(MACHOABI)

	char buf2[64];
	NODE *r;
	char *fname;

	fname = cftnsp->soname ? cftnsp->soname : cftnsp->sname;

	if (p->n_sp->sclass == EXTDEF) {
		snprintf(buf2, 64, "-L%s$pb", fname);
		sp = picsymtab("", name, buf2);
	} else {
		snprintf(buf2, 64, "$non_lazy_ptr-L%s$pb", fname);
		sp = picsymtab("L", name, buf2);
		addstub(&nlplist, name);
	}
#if USE_GOTNR
	q = tempnode(gotnr, PTR+VOID, 0, 0);
#else
	q = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(q) = GOTREG;
#endif
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);

	if (p->n_sp->sclass != EXTDEF)
		q = block(UMUL, q, 0, PTR+VOID, 0, 0);

#endif

	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	nfree(p);

	return q;
}

/*
 * Create a reference for a static variable
 */

static NODE *
picstatic(NODE *p)
{
	NODE *q;
	struct symtab *sp;

#if defined(ELFABI)
	char *n;

	if (p->n_sp->slevel > 0) {
		char buf[64];
		snprintf(buf, 64, LABFMT, (int)p->n_sp->soffset);
		sp = picsymtab("", buf, "@got(31)");
	} else  {
		n = p->n_sp->soname ? p->n_sp->soname : p->n_sp->sname;
		sp = picsymtab("", exname(n), "@got(31)");
	}
	sp->sclass = STATIC;
	sp->stype = p->n_sp->stype;
	q = xbcon(0, sp, PTR+VOID);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp;
	nfree(p);

#elif defined(MACHOABI)

	char buf2[64];
	NODE *r;

	snprintf(buf2, 64, "-L%s$pb",
	    cftnsp->soname ? cftnsp->soname : cftnsp->sname);

	if (p->n_sp->slevel > 0) {
		char buf1[64];
		snprintf(buf1, 64, LABFMT, (int)p->n_sp->soffset);
		sp = picsymtab("", buf1, buf2);
	} else  {
		char *name = p->n_sp->soname ? p->n_sp->soname : exname(p->n_sp->sname);
		sp = picsymtab("", name, buf2);
	}
	sp->sclass = STATIC;
	sp->stype = p->n_sp->stype;
#if USE_GOTNR
	q = tempnode(gotnr, PTR+VOID, 0, 0);
#else
	q = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(q) = GOTREG;
#endif
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp;
	nfree(p);

#endif

	return q;
}

static NODE *
convert_ulltof(NODE *p)
{
	NODE *q, *r, *l, *t;
	int ty;
	int tmpnr;

	ty = p->n_type;
	l = p->n_left;
	nfree(p);

	q = tempnode(0, ULONGLONG, 0, 0);
	tmpnr = regno(q);
	t = buildtree(ASSIGN, q, l);
	ecomp(t);

#if 0
	q = tempnode(tmpnr, ULONGLONG, 0, 0);
	q = block(SCONV, q, NIL, LONGLONG, 0, 0);
#endif
	q = tempnode(tmpnr, LONGLONG, 0, 0);
	r = block(SCONV, q, NIL, ty, 0, 0);

	q = tempnode(tmpnr, ULONGLONG, 0, 0);
	q = block(RS, q, bcon(1), ULONGLONG, 0, 0);
	q = block(SCONV, q, NIL, LONGLONG, 0, 0);
	q = block(SCONV, q, NIL, ty, 0, 0);
	t = block(FCON, NIL, NIL, ty, 0, 0);
	t->n_dcon = 2;
	l = block(MUL, q, t, ty, 0, 0);

	r = buildtree(COLON, l, r);

	q = tempnode(tmpnr, ULONGLONG, 0, 0);
	q = block(SCONV, q, NIL, LONGLONG, 0, 0);
	l = block(LE, q, xbcon(0, NULL, LONGLONG), INT, 0, 0);

	return clocal(buildtree(QUEST, l, r));

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

	struct symtab *q;
	NODE *r, *l;
	int o;
	int m;
	TWORD t;
	int isptrvoid = 0;
	int tmpnr;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	switch (o = p->n_op) {

	case ADDROF:
#ifdef PCC_DEBUG
		if (xdebug) {
			printf("clocal(): ADDROF\n");
			printf("type: 0x%x\n", p->n_type);
		}
#endif
		/* XXX cannot takes addresses of PARAMs */

		if (kflag == 0 || blevel == 0)
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
			if (kflag == 0)
				break;
			/* FALLTHROUGH */

		case STATIC:
			if (kflag == 0) {
				if (q->slevel == 0)
					break;
				p->n_lval = 0;
			} else if (blevel > 0) {
				p = picstatic(p);
			}
			break;

		case REGISTER:
			p->n_op = REG;
			p->n_lval = 0;
			p->n_rval = q->soffset;
			break;

		case EXTERN: 
		case EXTDEF:
			if (kflag == 0)
				break;
			if (blevel > 0)
				p = picext(p);
			break;
		}
		break;

	case UCALL:
	case CALL:
	case USTCALL:
	case STCALL:
                if (p->n_type == VOID)
                        break;
                /*
                 * if the function returns void*, ecode() invokes
                 * delvoid() to convert it to uchar*.
                 * We just let this happen on the ASSIGN to the temp,
                 * and cast the pointer back to void* on access
                 * from the temp.
                 */
                if (p->n_type == PTR+VOID)
                        isptrvoid = 1;
                r = tempnode(0, p->n_type, p->n_df, p->n_ap);
                tmpnr = regno(r);
		r = buildtree(ASSIGN, r, p);

                p = tempnode(tmpnr, r->n_type, r->n_df, r->n_ap);
                if (isptrvoid) {
                        p = block(PCONV, p, NIL, PTR+VOID,
                            p->n_df, 0);
                }
#if 1
                p = buildtree(COMOP, r, p);
#else
		/* XXX this doesn't work if the call is already in a COMOP */
		r = clocal(r);
		ecomp(r);
#endif
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
		if (l->n_type < INT || DEUNSIGN(l->n_type) == LONGLONG) {
			/* float etc? */
			p->n_left = block(SCONV, l, NIL,
			    UNSIGNED, 0, 0);
			break;
		}
		/* if left is SCONV, cannot remove */
		if (l->n_op == SCONV)
			break;

		/* avoid ADDROF TEMP */
		if (l->n_op == ADDROF && l->n_left->n_op == TEMP)
			break;

		/* if conversion to another pointer type, just remove */
		if (p->n_type > BTMASK && l->n_type > BTMASK)
			goto delp;
		break;

	delp:	l->n_type = p->n_type;
		l->n_qual = p->n_qual;
		l->n_df = p->n_df;
		l->n_ap = p->n_ap;
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
		    coptype(l->n_op) == BITYPE) {
			l->n_type = p->n_type;
			nfree(p);
			return l;
		}

		/*
		 * if converting ULONGLONG to FLOAT/(L)DOUBLE,
		 * replace ___floatunsdidf() with ___floatdidf()
		 */
		if (l->n_type == ULONGLONG && p->n_type >= FLOAT &&
		    p->n_type <= LDOUBLE) {
			return convert_ulltof(p);
		}

		o = l->n_op;
		m = p->n_type;

		if (o == ICON) {
			CONSZ val = l->n_lval;

			if (!ISPTR(m)) /* Pointers don't need to be conv'd */
			    switch (m) {
			case BOOL:
				l->n_lval = l->n_lval != 0;
				break;
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
			l->n_ap = 0;
			nfree(p);
			return l;
		} else if (o == FCON) {
			l->n_lval = l->n_dcon;
			l->n_sp = NULL;
			l->n_op = ICON;
			l->n_type = m;
			l->n_ap = 0;
			nfree(p);
			return clocal(l);
		}
		if (DEUNSIGN(p->n_type) == SHORT &&
		    DEUNSIGN(l->n_type) == SHORT) {
			nfree(p);
			p = l;
		}
		if ((DEUNSIGN(p->n_type) == CHAR ||
		    DEUNSIGN(p->n_type) == SHORT) &&
		    (l->n_type == FLOAT || l->n_type == DOUBLE ||
		    l->n_type == LDOUBLE)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_ap);
			p->n_left->n_type = INT;
			return p;
		}
		if ((DEUNSIGN(l->n_type) == CHAR ||
		    DEUNSIGN(l->n_type) == SHORT) &&
		    (p->n_type == FLOAT || p->n_type == DOUBLE ||
		    p->n_type == LDOUBLE)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_ap);
			p->n_left->n_type = INT;
			return p;
		}
		break;

	case MOD:
		simmod(p);
		break;

	case DIV:
		if (o == DIV && p->n_type != CHAR && p->n_type != SHORT)
			break;
		/* make it an int division by inserting conversions */
		p->n_left = block(SCONV, p->n_left, NIL, INT, 0, 0);
		p->n_right = block(SCONV, p->n_right, NIL, INT, 0, 0);
		p = block(SCONV, p, NIL, p->n_type, 0, 0);
		p->n_left->n_type = INT;
		break;

	case STNAME:
		if ((q = p->n_sp) == NULL)
			return p;
		if (q->sclass != STNAME)
			return p;
		t = p->n_type;
		p = block(ADDROF, p, NIL, INCREF(t), p->n_df, p->n_ap);
		p = block(UMUL, p, NIL, t, p->n_df, p->n_ap);
		break;

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(BOOL_TYPE) : RETREG(p->n_type);
		break;

	case LS:
	case RS:
		if (p->n_right->n_op == ICON)
			break; /* do not do anything */
		if (DEUNSIGN(p->n_right->n_type) == INT)
			break;
		p->n_right = block(SCONV, p->n_right, NIL, INT, 0, 0);
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

/*
 * Change CALL references to either direct (static) or PLT.
 */
static void
fixnames(NODE *p, void *arg)
{
        struct symtab *sp;
        struct attr *ap;
        NODE *q;
        char *c;
        int isu;

        if ((cdope(p->n_op) & CALLFLG) == 0)
                return;

        isu = 0;
        q = p->n_left;
        ap = q->n_ap;
        if (q->n_op == UMUL)
                q = q->n_left, isu = 1;

#if defined(ELFABI)

        if (q->n_op == ICON) {
                sp = q->n_sp;

#elif defined(MACHOABI)

#ifdef USE_GOTNR
	if (q->n_op == PLUS && q->n_left->n_op == TEMP &&
#else
	if (q->n_op == PLUS && q->n_left->n_op == REG &&
#endif
	    q->n_right->n_op == ICON) {
                sp = q->n_right->n_sp;
#endif

                if (sp == NULL)
                        return; /* nothing to do */
                if (sp->sclass == STATIC && !ISFTN(sp->stype))
                        return; /* function pointer */

                if (sp->sclass != STATIC && sp->sclass != EXTERN &&
                    sp->sclass != EXTDEF)
                        cerror("fixnames");
		c = NULL;
#if defined(ELFABI)

		if (sp->soname == NULL ||
                    (c = strstr(sp->soname, "@got(31)")) == NULL)
                        cerror("fixnames2");
                if (isu) {
			memcpy(c, "@plt", sizeof("@plt"));
                } else
                        *c = 0;

#elif defined(MACHOABI)

		if (sp->soname == NULL ||
		    ((c = strstr(sp->soname, "$non_lazy_ptr")) == NULL &&
		    (c = strstr(sp->soname, "-L")) == NULL))
				cerror("fixnames2");
		if (isu) {
			addstub(&stublist, sp->soname+1);
			memcpy(c, "$stub", sizeof("$stub"));
		} else 
			*c = 0;

		nfree(q->n_left);
		q = q->n_right;
		if (isu)
			nfree(p->n_left->n_left);
		nfree(p->n_left);
		p->n_left = q;
		q->n_ap = ap;

#endif
        }
}

void
myp2tree(NODE *p)
{
	int o = p->n_op;
	struct symtab *sp;

	if (kflag)
		walkf(p, fixnames, 0);
	if (o != FCON) 
		return;

	/* Write float constants to memory */
	/* Should be voluntary per architecture */
 
	sp = IALLOC(sizeof(struct symtab));
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
	return 1;
}

/*
 * Allocate bits on the stack.
 * 'off' is the number of bits to allocate
 * 'p' is a tree that when evaluated is the multiply count for 'off'
 * 't' is a storeable node where to write the allocated address
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *q, *r;
	int nbytes = off / SZCHAR;
	int stacksize = 24+40; /* this should be p2stacksize */

	/*
	 * After we subtract the requisite bytes
	 * off the stack, we need to step back over
	 * the 40 bytes for the arguments registers
	 * *and* any other parameters which will get
	 * saved to the stack.  Unfortunately, we
	 * don't have that information in pass1 and
	 * the parameters will stomp on the allocated
	 * space for alloca().
	 *
	 * No fix yet.
	 */
	werror("parameters may stomp on alloca()");

	/* compute size */
	p = buildtree(MUL, p, bcon(nbytes));
	p = buildtree(PLUS, p, bcon(ALSTACK/SZCHAR));

	/* load the top-of-stack */
	q = block(REG, NIL, NIL, PTR+INT, 0, 0);
	regno(q) = SPREG;
	q = block(UMUL, q, NIL, INT, 0, 0);

	/* save old top-of-stack value to new top-of-stack position */
	r = block(REG, NIL, NIL, PTR+INT, 0, 0);
	regno(r) = SPREG;
	r = block(MINUSEQ, r, p, INT, 0, 0);
	r = block(UMUL, r, NIL, INT, 0, 0);
	ecomp(buildtree(ASSIGN, r, q));

	r = block(REG, NIL, NIL, PTR+INT, 0, 0);
	regno(r) = SPREG;

	/* skip over the arguments space and align to 16 bytes */
	r = block(PLUS, r, bcon(stacksize + 15), INT, 0, 0);
	r = block(RS, r, bcon(4), INT, 0, 0);
	r = block(LS, r, bcon(4), INT, 0, 0);

	t->n_type = p->n_type;
	ecomp(buildtree(ASSIGN, t, r));
}

/*
 * Print out a string of characters.
 * Unfortunately, this code assumes that the assembler understands
 * C-style escape sequences. (which it doesn't!)
 * Location is already set.
 */
void
instring(struct symtab *sp)
{
	char *s, *str = sp->sname;

#if defined(ELFABI)

	defloc(sp);

#elif defined(MACHOABI)

	extern int lastloc;
	if (lastloc != STRNG)
		printf("	.cstring\n");
	lastloc = STRNG;
	printf("\t.p2align 2\n");
	printf(LABFMT ":\n", sp->soffset);

#endif

	/* be kind to assemblers and avoid long strings */
	printf("\t.ascii \"");
	for (s = str; *s != 0; ) {
		if (*s++ == '\\') {
			(void)esccon(&s);
		}
		if (s - str > 64) {
			fwrite(str, 1, s - str, stdout);
			printf("\"\n\t.ascii \"");
			str = s;
		}
	}
	fwrite(str, 1, s - str, stdout);
	printf("\\0\"\n");
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
	struct symtab *q;
	char *c;
	TWORD t;
	int i;

	t = p->n_type;
	if (t > BTMASK)
		p->n_type = t = INT; /* pointer */


	if (kflag && (p->n_op == PLUS || p->n_op == UMUL)) {
		if (p->n_op == UMUL)
			p = p->n_left;
		p = p->n_right;
		q = p->n_sp;

#if defined(ELFABI)

		if (q->soname && (c = strstr(q->soname, "@got(31)")) != NULL)
			*c = 0; /* ignore GOT ref here */

#elif defined(MACHOABI)

		if  ((c = strstr(q->soname, "$non_lazy_ptr")) != NULL) {
			q->soname++;	/* skip "L" */
			*c = 0; /* ignore GOT ref here */
		}
		else if ((c = strstr(q->soname, "-L")) != NULL)
			*c = 0; /* ignore GOT ref here */

#endif

	}

	if (p->n_op == ICON && p->n_sp != NULL && DEUNSIGN(t) != INT)
		uerror("element not constant");

	switch (t) {
	case LONGLONG:
	case ULONGLONG:
#if 0
		/* little-endian */
		i = (p->n_lval >> 32);
		p->n_lval &= 0xffffffff;
		p->n_type = INT;
		ninval(off, 32, p);
		p->n_lval = i;
		ninval(off+32, 32, p);
#endif
		/* big-endian */
		i = (p->n_lval & 0xffffffff);
		p->n_lval >>= 32;
		p->n_type = INT;
		ninval(off, 32, p);
		p->n_lval = i;
		ninval(off+32, 32, p);

		break;
	case INT:
	case UNSIGNED:
		printf("\t.long %d", (int)p->n_lval);
		if ((q = p->n_sp) != NULL) {
			if ((q->sclass == STATIC && q->slevel > 0)) {
				printf("+" LABFMT, q->soffset);
			} else {
				char *name = q->soname ? q->soname : exname(q->sname);
				printf("+%s", name);
			}
		}
		printf("\n");
		break;
	case LDOUBLE:
		u.i[2] = 0;
		u.l = (long double)p->n_dcon;
#if 0
		/* little-endian */
		printf("\t.long 0x%x,0x%x,0x%x\n", u.i[0], u.i[1], u.i[2]);
#endif
		/* big-endian */
		printf("\t.long 0x%x,0x%x,0x%x\n", u.i[0], u.i[1], u.i[2]);
		break;
	case DOUBLE:
		u.d = (double)p->n_dcon;
		printf("\t.long 0x%x\n\t.long 0x%x\n", u.i[0], u.i[1]);
		break;
	case FLOAT:
		u.f = (float)p->n_dcon;
		printf("\t.long 0x%x\n", u.i[0]);
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
#if defined(ELFABI)

	return (p == NULL ? "" : p);

#elif defined(MACHOABI)

#define NCHNAM	256
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

#endif
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
#ifdef PCC_DEBUG
	if (xdebug)
		printf("calldec:\n");
#endif
}

void
extdec(struct symtab *q)
{
#ifdef PCC_DEBUG
	if (xdebug)
		printf("extdec:\n");
#endif
}

/* make a common declaration for id, if reasonable */
void
defzero(struct symtab *sp)
{
	char *n;
	int off;

	off = tsize(sp->stype, sp->sdf, sp->sap);
	off = (off+(SZCHAR-1))/SZCHAR;
	printf("\t.%scomm ", sp->sclass == STATIC ? "l" : "");
	n = sp->soname ? sp->soname : exname(sp->sname);
	if (sp->slevel == 0)
		printf("%s,%d\n", n, off);
	else
		printf(LABFMT ",%d\n", sp->soffset, off);
}


#ifdef notdef
/* make a common declaration for id, if reasonable */
void
commdec(struct symtab *q)
{
	int off;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;
	printf("\t.comm %s,0%o\n", q->soname ? q->soname : exname(q->sname), off);
}

/* make a local common declaration for id, if reasonable */
void
lcommdec(struct symtab *q)
{
	int off;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;
	if (q->slevel == 0)
		printf("\t.lcomm %s,%d\n", q->soname ? q->soname : exname(q->sname), off);
	else
		printf("\t.lcomm " LABFMT ",%d\n", q->soffset, off);
}

/*
 * print a (non-prog) label.
 */
void
deflab1(int label)
{
	printf(LABFMT ":\n", label);
}

#if defined(ELFABI)

static char *loctbl[] = { "text", "data", "section .rodata,",
    "section .rodata" };

#elif defined(MACHOABI)

static char *loctbl[] = { "text", "data", "section .rodata,", "cstring" };

#endif

void
setloc1(int locc)
{
#ifdef PCC_DEBUG
	if (xdebug)
		printf("setloc1: locc=%d, lastloc=%d\n", locc, lastloc);
#endif

	if (locc == lastloc)
		return;
	lastloc = locc;
	printf("	.%s\n", loctbl[locc]);
}
#endif

/* simulate and optimise the MOD opcode */
static void
simmod(NODE *p)
{
	NODE *r = p->n_right;

	assert(p->n_op == MOD);

	if (!ISUNSIGNED(p->n_type))
		return;

#define ISPOW2(n) ((n) && (((n)&((n)-1)) == 0))

	/* if the right is a constant power of two, then replace with AND */
	if (r->n_op == ICON && ISPOW2(r->n_lval)) {
		p->n_op = AND;
		r->n_lval--;
		return;
	}

#undef ISPOW2

	/* other optimizations can go here */
}

static int constructor;
static int destructor;

/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	if (strcmp(str, "tls") == 0) {
		uerror("thread-local storage not supported for this target");
		return 1;
	}
	if (strcmp(str, "constructor") == 0 || strcmp(str, "init") == 0) {
		constructor = 1;
		return 1;
	}
	if (strcmp(str, "destructor") == 0 || strcmp(str, "fini") == 0) {
		destructor = 1;
		return 1;
	}

	return 0;
}

/*
 * Called when a identifier has been declared, to give target last word.
 */
void
fixdef(struct symtab *sp)
{
	/* may have sanity checks here */
	if ((constructor || destructor) && (sp->sclass != PARAM)) {
#ifdef MACHOABI
		if (kflag) {
			if (constructor)
				printf("\t.mod_init_func\n");
			else
				printf("\t.mod_term_func\n");
		} else {
			if (constructor)
				printf("\t.constructor\n");
			else
				printf("\t.destructor\n");
		}
		printf("\t.p2align 2\n");
		printf("\t.long %s\n", exname(sp->sname));
		printf("\t.text\n");
		constructor = destructor = 0;
#endif
	}
}

/*
 * There is very little different here to the standard builtins.
 * It basically handles promotion of types smaller than INT.
 */

NODE *
powerpc_builtin_stdarg_start(NODE *f, NODE *a, TWORD t)
{
        NODE *p, *q;
        int sz = 1;

        /* check num args and type */
        if (a == NULL || a->n_op != CM || a->n_left->n_op == CM ||
            !ISPTR(a->n_left->n_type))
                goto bad;

        /* must first deal with argument size; use int size */
        p = a->n_right;
        if (p->n_type < INT) {
                /* round up to word */
                sz = SZINT / tsize(p->n_type, p->n_df, p->n_ap);
        }

        p = buildtree(ADDROF, p, NIL);  /* address of last arg */
        p = optim(buildtree(PLUS, p, bcon(sz)));
        q = block(NAME, NIL, NIL, PTR+VOID, 0, 0);
        q = buildtree(CAST, q, p);
        p = q->n_right;
        nfree(q->n_left);
        nfree(q);
        p = buildtree(ASSIGN, a->n_left, p);
        tfree(f);
        nfree(a);

        return p;

bad:
        uerror("bad argument to __builtin_stdarg_start");
        return bcon(0);
}

NODE *
powerpc_builtin_va_arg(NODE *f, NODE *a, TWORD t)
{
        NODE *p, *q, *r;
        int sz, tmpnr;

        /* check num args and type */
        if (a == NULL || a->n_op != CM || a->n_left->n_op == CM ||
            !ISPTR(a->n_left->n_type) || a->n_right->n_op != TYPE)
                goto bad;

        r = a->n_right;

        /* get type size */
        sz = tsize(r->n_type, r->n_df, r->n_ap) / SZCHAR;
        if (sz < SZINT/SZCHAR) {
                werror("%s%s promoted to int when passed through ...",
                        ISUNSIGNED(r->n_type) ? "unsigned " : "",
                        DEUNSIGN(r->n_type) == SHORT ? "short" : "char");
                sz = SZINT/SZCHAR;
		r->n_type = INT;
		r->n_ap = 0;
        }

        p = tcopy(a->n_left);

#if defined(ELFABI)

        /* alignment */
        if (SZINT/SZCHAR && r->n_type != UNIONTY && r->n_type != STRTY) {
                p = buildtree(PLUS, p, bcon(ALSTACK/8 - 1));
                p = block(AND, p, bcon(-ALSTACK/8), p->n_type, p->n_df, p->n_ap);
        }

#endif

        /* create a copy to a temp node */
        q = tempnode(0, p->n_type, p->n_df, p->n_ap);
        tmpnr = regno(q);
        p = buildtree(ASSIGN, q, p);

        q = tempnode(tmpnr, p->n_type, p->n_df, p->n_ap);
        q = buildtree(PLUS, q, bcon(sz));
        q = buildtree(ASSIGN, a->n_left, q);

        q = buildtree(COMOP, p, q);

        nfree(a->n_right);
        nfree(a);
        nfree(f);

        p = tempnode(tmpnr, INCREF(r->n_type), r->n_df, r->n_ap);
        p = buildtree(UMUL, p, NIL);
        p = buildtree(COMOP, q, p);

        return p;

bad:
        uerror("bad argument to __builtin_va_arg");
        return bcon(0);
}

NODE *
powerpc_builtin_va_end(NODE *f, NODE *a, TWORD t)
{
        tfree(f);
        tfree(a);
 
        return bcon(0);
}

NODE *
powerpc_builtin_va_copy(NODE *f, NODE *a, TWORD t)
{
        if (a == NULL || a->n_op != CM || a->n_left->n_op == CM)
                goto bad;
        tfree(f);
        f = buildtree(ASSIGN, a->n_left, a->n_right);
        nfree(a);
        return f;

bad:
        uerror("bad argument to __buildtin_va_copy");
        return bcon(0);
}

NODE *
powerpc_builtin_return_address(NODE *f, NODE *a, TWORD t)
{
	int nframes;
	int i = 0;

	if (a == NULL || a->n_op != ICON)
		goto bad;

	nframes = a->n_lval;

	tfree(f);
	tfree(a);

	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = SPREG;

	do {
		f = block(UMUL, f, NIL, PTR+VOID, 0, 0);
	} while (i++ < nframes);

	f = block(PLUS, f, bcon(8), INCREF(PTR+VOID), 0, 0);
	f = buildtree(UMUL, f, NIL);

	return f;
bad:
        uerror("bad argument to __builtin_return_address");
        return bcon(0);
}

NODE *
powerpc_builtin_frame_address(NODE *f, NODE *a, TWORD t)
{
	int nframes;
	int i = 0;

	if (a == NULL || a->n_op != ICON)
		goto bad;

	nframes = a->n_lval;

	tfree(f);
	tfree(a);

	if (nframes == 0) {
		f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
		regno(f) = FPREG;
	} else {
		f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
		regno(f) = SPREG;
		do {
			f = block(UMUL, f, NIL, PTR+VOID, 0, 0);
		} while (i++ < nframes);
		f = block(PLUS, f, bcon(24), INCREF(PTR+VOID), 0, 0);
		f = buildtree(UMUL, f, NIL);
	}

	return f;
bad:
        uerror("bad argument to __builtin_frame_address");
        return bcon(0);
}

void
pass1_lastchance(struct interpass *ip)
{
}
