/*	Id: local.c,v 1.199 2015/11/17 19:19:40 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.7 2016/02/09 20:28:19 plunky Exp $	*/
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

#undef NIL
#define	NIL NULL

#ifdef LANG_CXX
#define	P1ND NODE
#define	p1nfree nfree
#define	p1fwalk fwalk
#define	p1tcopy tcopy
#endif

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

static char *
getsoname(struct symtab *sp)
{
	struct attr *ap;
	return (ap = attr_find(sp->sap, ATTR_SONAME)) ?
	    ap->sarg(0) : sp->sname;
	
}

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

#define	IALLOC(sz)	(isinlining ? permalloc(sz) : tmpalloc(sz))

/*
 * Make a symtab entry for PIC use.
 */
static struct symtab *
picsymtab(char *p, char *s, char *s2)
{
	struct symtab *sp = IALLOC(sizeof(struct symtab));
	size_t len = strlen(p) + strlen(s) + strlen(s2) + 1;
	
	sp->sname = IALLOC(len);
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

#ifdef PECOFFABI
static P1ND *
import(P1ND *p)
{
	struct attr *ap;
	P1ND *q;
	char *name;
	struct symtab *sp;

	name = getexname(p->n_sp);

	sp = picsymtab("__imp_", name, "");
	q = xbcon(0, sp, PTR+VOID);
	q = block(UMUL, q, 0, PTR|VOID, 0, 0);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	p1nfree(p);

	return q;
}
#endif

int gotnr; /* tempnum for GOT register */
int argstacksize;

/*
 * Create a reference for an extern variable.
 */
static P1ND *
picext(P1ND *p)
{
	struct attr *ap;

#if defined(ELFABI)
	P1ND *q, *r;
	struct symtab *sp;
	char *name;

	q = tempnode(gotnr, PTR|VOID, 0, 0);
	name = getexname(p->n_sp);

#ifdef GCC_COMPAT
	if ((ap = attr_find(p->n_sp->sap, GCC_ATYP_VISIBILITY)) &&
	    strcmp(ap->sarg(0), "hidden") == 0) {
		/* For hidden vars use GOTOFF */
		sp = picsymtab("", name, "@GOTOFF");
		r = xbcon(0, sp, INT);
		q = buildtree(PLUS, q, r);
		q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
		q->n_sp = p->n_sp; /* for init */
		p1nfree(p);
		return q;
	}
#endif

	sp = picsymtab("", name, "@GOT");
#ifdef GCC_COMPAT
	if (attr_find(p->n_sp->sap, GCC_ATYP_STDCALL) != NULL)
		p->n_sp->sflags |= SSTDCALL;
#endif
	sp->sflags = p->n_sp->sflags & SSTDCALL;
	sp->sap = attr_add(p->n_sp->sap, sp->sap);
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, PTR|VOID, 0, 0);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	p1nfree(p);
	return q;

#elif defined(MACHOABI)

	P1ND *q, *r;
	struct symtab *sp;
	char buf2[256], *name, *pspn;

	name = getsoname(cftnsp);
	pspn = getexname(p->n_sp);

	if (p->n_sp->sclass == EXTDEF) {
		snprintf(buf2, 256, "-L%s$pb", name);
		sp = picsymtab("", pspn, buf2);
	} else {
		snprintf(buf2, 256, "$non_lazy_ptr-L%s$pb", name);
		sp = picsymtab("L", pspn, buf2);
		addstub(&nlplist, pspn);
	}

	sp->stype = p->n_sp->stype;

	q = tempnode(gotnr, PTR+VOID, 0, 0);
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);

	if (p->n_sp->sclass != EXTDEF)
		q = block(UMUL, q, 0, PTR+VOID, 0, 0);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	p1nfree(p);
	return q;

#else /* defined(PECOFFABI) || defined(AOUTABI) */

	return p;

#endif

}

/*
 * Create a reference for a static variable.
 */
static P1ND *
picstatic(P1ND *p)
{
#if defined(ELFABI)

	P1ND *q, *r;
	struct symtab *sp;

	q = tempnode(gotnr, PTR|VOID, 0, 0);
	if (p->n_sp->slevel > 0) {
		char buf[32];
		if ((p->n_sp->sflags & SMASK) == SSTRING)
			p->n_sp->sflags |= SASG;
		snprintf(buf, 32, LABFMT, (int)p->n_sp->soffset);
		sp = picsymtab("", buf, "@GOTOFF");
	} else
		sp = picsymtab("", getsoname(p->n_sp), "@GOTOFF");
	
	sp->sclass = STATIC;
	sp->stype = p->n_sp->stype;
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */
	p1nfree(p);
	return q;

#elif defined(MACHOABI)

	P1ND *q, *r;
	struct symtab *sp;
	char buf2[256];

	snprintf(buf2, 256, "-L%s$pb",
	    cftnsp->soname ? cftnsp->soname : cftnsp->sname);

	if (p->n_sp->slevel > 0) {
		char buf1[32];
		snprintf(buf1, 32, LABFMT, (int)p->n_sp->soffset);
		sp = picsymtab("", buf1, buf2);
	} else  {
		char *name = getexname(p->n_sp);
		sp = picsymtab("", name, buf2);
	}
	sp->sclass = STATIC;
	sp->stype = p->n_sp->stype;
	q = tempnode(gotnr, PTR+VOID, 0, 0);
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp;
	p1nfree(p);
	return q;

#else /* defined(PECOFFABI) || defined(AOUTABI) */

	return p;

#endif

}

#ifdef TLS
/*
 * Create a reference for a TLS variable.
 */
static P1ND *
tlspic(P1ND *p)
{
	P1ND *q, *r;
	struct symtab *sp, *sp2;
	char *name;

	/*
	 * creates:
	 *   leal var@TLSGD(%ebx),%eax
	 *   call ___tls_get_addr@PLT
	 */

	/* calc address of var@TLSGD */
	q = tempnode(gotnr, PTR|VOID, 0, 0);
	name = getsoname(p->n_sp);
	sp = picsymtab("", name, "@TLSGD");
	r = xbcon(0, sp, INT);
	q = buildtree(PLUS, q, r);

	/* assign to %eax */
	r = block(REG, NIL, NIL, PTR|VOID, 0, 0);
	r->n_rval = EAX;
	q = buildtree(ASSIGN, r, q);

	/* call ___tls_get_addr */
	sp2 = lookup("___tls_get_addr@PLT", 0);
	sp2->stype = EXTERN|INT|FTN;
	r = nametree(sp2);
	r = buildtree(ADDROF, r, NIL);
	r = block(UCALL, r, NIL, INT, 0, 0);

	/* fusion both parts together */
	q = buildtree(COMOP, q, r);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */

	p1nfree(p);
	return q;
}

static P1ND *
tlsnonpic(P1ND *p)
{
	P1ND *q, *r;
	struct symtab *sp, *sp2;
	int ext = p->n_sp->sclass;
	char *name;

	name = getsoname(p->n_sp);
	sp = picsymtab("", name,
	    ext == EXTERN ? "@INDNTPOFF" : "@NTPOFF");
	q = xbcon(0, sp, INT);
	if (ext == EXTERN)
		q = block(UMUL, q, NIL, PTR|VOID, 0, 0);

	sp2 = lookup("%gs:0", 0);
	sp2->stype = EXTERN|INT;
	r = nametree(sp2);

	q = buildtree(PLUS, q, r);
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = p->n_sp; /* for init */

	p1nfree(p);
	return q;
}

static P1ND *
tlsref(P1ND *p)
{
	if (kflag)
		return (tlspic(p));
	else
		return (tlsnonpic(p));
}
#endif

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
P1ND *
clocal(P1ND *p)
{

	struct attr *ap;
	register struct symtab *q;
	register P1ND *r, *l, *n, *s;
	register int o;
	register int m;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal: %p\n", p);
		p1fwalk(p, eprint, 0);
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

		case USTATIC:
			if (kflag == 0)
				break;
			/* FALLTHROUGH */
		case STATIC:
#ifdef TLS
			if (q->sflags & STLS) {
				p = tlsref(p);
				break;
			}
#endif
			if (kflag == 0) {
				if (q->slevel == 0)
					break;
			} else if (kflag && !statinit && blevel > 0 &&
#ifdef GCC_COMPAT
			    attr_find(q->sap, GCC_ATYP_WEAKREF)) {
#else
			    0) {
#endif
				/* extern call */
				p = picext(p);
			} else if (blevel > 0 && !statinit)
				p = picstatic(p);
			break;

		case REGISTER:
			p->n_op = REG;
			slval(p, 0);
			p->n_rval = q->soffset;
			break;

		case EXTERN:
		case EXTDEF:
#ifdef TLS
			if (q->sflags & STLS) {
				p = tlsref(p);
				break;
			}
#endif

#ifdef PECOFFABI
			if (q->sflags & SDLLINDIRECT)
				p = import(p);
#endif
#ifdef GCC_COMPAT
			if ((ap = attr_find(q->sap,
			    GCC_ATYP_VISIBILITY)) != NULL &&
			    strcmp(ap->sarg(0), "hidden") == 0)
				printf(PRTPREF "\t.hidden %s\n", getsoname(q));
#endif
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
		p1nfree(l);
		if (p->n_op != UMUL)
			cerror("ADDROF error");
		l = p;
		p = p->n_left;
		p1nfree(l);
		break;

	case UCALL:
		if (kflag == 0)
			break;
		l = block(REG, NIL, NIL, INT, 0, 0);
		l->n_rval = EBX;
		p->n_right = buildtree(ASSIGN, l, tempnode(gotnr, INT, 0, 0));
		p->n_op -= (UCALL-CALL);
		break;

	case USTCALL:
#if defined(os_openbsd)
		ap = strattr(p->n_left->n_ap);
		if (ap->amsize == SZCHAR || ap->amsize == SZSHORT ||
		    ap->amsize == SZINT || ap->amsize == SZLONGLONG)
#else
		if (attr_find(p->n_left->n_ap, ATTR_COMPLEX) &&
		    (ap = strattr(p->n_left->n_ap)) &&
		    ap->amsize == SZLONGLONG)
#endif
		{
			/* float complex */
			/* fake one arg to make pass2 happy */
			p->n_right = block(FUNARG, bcon(0), NIL, INT, 0, 0);
			p->n_op -= (UCALL-CALL);
			break;
		}

		/* Add hidden arg0 */
		r = block(REG, NIL, NIL, INCREF(VOID), 0, 0);
		regno(r) = EBP;
#ifdef GCC_COMPAT
		if ((ap = attr_find(p->n_ap, GCC_ATYP_REGPARM)) != NULL &&
		    ap->iarg(0) > 0) {
			l = block(REG, NIL, NIL, INCREF(VOID), 0, 0);
			regno(l) = EAX;
			p->n_right = buildtree(ASSIGN, l, r);
		} else
#endif
			p->n_right = block(FUNARG, r, NIL, INCREF(VOID), 0, 0);
		p->n_op -= (UCALL-CALL);

		if (kflag == 0)
			break;
		l = block(REG, NIL, NIL, INT, 0, 0);
		regno(l) = EBX;
		r = buildtree(ASSIGN, l, tempnode(gotnr, INT, 0, 0));
		p->n_right = block(CM, r, p->n_right, INT, 0, 0);
		break;
	
	/* FALLTHROUGH */
#if defined(MACHOABI)
	case CALL:
	case STCALL:
		if (p->n_type == VOID)
			break;

		r = tempnode(0, p->n_type, p->n_df, p->n_ap);
		l = tcopy(r);
		p = buildtree(COMOP, buildtree(ASSIGN, r, p), l);
#endif
			
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
		p1nfree(l->n_left);
		p1nfree(l);
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
			p1nfree(p);
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
					p1nfree(p);
					return l;
				}
			}
		}

		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT &&
		    coptype(l->n_op) == BITYPE && l->n_op != COMOP &&
		    l->n_op != QUEST && l->n_op != ASSIGN) {
			l->n_type = p->n_type;
			p1nfree(p);
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
			p1nfree(p);
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

		/* If not using pcc struct return */
	case STASG:
		r = p->n_right;
		if (r->n_op != STCALL && r->n_op != USTCALL)
			break;
		m = tsize(BTYPE(r->n_type), r->n_df, r->n_ap);
		if (m == SZCHAR)
			m = CHAR;
		else if (m == SZSHORT)
			m = SHORT;
		else if (m == SZINT)
			m = INT;
		else if (m == SZLONGLONG)
			m = LONGLONG;
		else
			break;
#if !defined(os_openbsd)
		if (attr_find(r->n_ap, ATTR_COMPLEX) == 0)
			break;	/* float _Complex always in regs */
#endif
		l = buildtree(ADDROF, p->n_left, NIL);
		p1nfree(p);

		r->n_op -= (STCALL-CALL);
		r->n_type = m;

		/* r = long, l = &struct */

		n = tempnode(0, m, r->n_df, r->n_ap);
		r = buildtree(ASSIGN, p1tcopy(n), r);

		s = tempnode(0, l->n_type, l->n_df, l->n_ap);
		l = buildtree(ASSIGN, p1tcopy(s), l);

		p = buildtree(COMOP, r, l);

		l = buildtree(CAST,
		    block(NAME, NIL, NIL, m|PTR, 0, 0), p1tcopy(s));
		r = l->n_right;
		p1nfree(l->n_left);
		p1nfree(l);

		r = buildtree(ASSIGN, buildtree(UMUL, r, NIL), n);
		p = buildtree(COMOP, p, r);
		p = buildtree(COMOP, p, s);
		break;
	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end: %p\n", p);
		p1fwalk(p, eprint, 0);
	}
#endif
	return(p);
}

/*
 * Change CALL references to either direct (static) or PLT.
 */
static void
fixnames(P1ND *p, void *arg)
{
#if defined(ELFABI) || defined(MACHOABI)

	struct symtab *sp;
	struct attr *ap, *ap2;
	P1ND *q;
	char *c;
	int isu;

	if ((cdope(p->n_op) & CALLFLG) == 0)
		return;
	isu = 0;
	q = p->n_left;
	ap = q->n_ap;
	if (q->n_op == UMUL)
		q = q->n_left, isu = 1;

	if (q->n_op == PLUS && q->n_left->n_op == TEMP &&
	    q->n_right->n_op == ICON) {
		sp = q->n_right->n_sp;

		if (sp == NULL)
			return;	/* nothing to do */
		if (sp->sclass == STATIC && !ISFTN(sp->stype))
			return; /* function pointer */

		if (sp->sclass != STATIC && sp->sclass != EXTERN &&
		    sp->sclass != EXTDEF)
			cerror("fixnames");
		c = NULL;
#if defined(ELFABI)

		if ((ap2 = attr_find(sp->sap, ATTR_SONAME)) == NULL ||
		    (c = strstr(ap2->sarg(0), "@GOT")) == NULL)
			cerror("fixnames2: %p %s", ap2, c);
		if (isu) {
			memcpy(c, "@PLT", sizeof("@PLT"));
		} else
			*c = 0;

#elif defined(MACHOABI)

		if (sp->soname == NULL ||
		    ((c = strstr(sp->soname, "$non_lazy_ptr")) == NULL &&
		    (c = strstr(sp->soname, "-L")) == NULL))
				cerror("fixnames2");

		if (!ISFTN(sp->stype))
			return; /* function pointer */

		if (isu) {
			*c = 0;
			addstub(&stublist, sp->soname+1);
			memcpy(c, "$stub", sizeof("$stub"));
		} else 
			*c = 0;

#endif

		p1nfree(q->n_left);
		q = q->n_right;
		if (isu)
			p1nfree(p->n_left->n_left);
		p1nfree(p->n_left);
		p->n_left = q;
		q->n_ap = ap;
	}
#endif
}

static void mangle(P1ND *p);

void
myp2tree(P1ND *p)
{
	struct symtab *sp;

	if (kflag)
		fixnames(p, 0);

	mangle(p);

	if ((p->n_op == STCALL || p->n_op == USTCALL) && 
	    attr_find(p->n_ap, ATTR_COMPLEX) &&
	    strmemb(p->n_ap)->stype == FLOAT)
		p->n_ap = attr_add(p->n_ap, attr_new(ATTR_I386_FCMPLRET, 1));

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

	if (kflag) {
		P1ND *q = optim(picstatic(p1tcopy(p)));
		*p = *q;
		p1nfree(q);
	}
}

/*ARGSUSED*/
int
andable(P1ND *p)
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
spalloc(P1ND *t, P1ND *p, OFFSZ off)
{
	P1ND *sp;

	p = buildtree(MUL, p, bcon(off/SZCHAR)); /* XXX word alignment? */

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	slval(sp, 0);
	sp->n_rval = STKREG;
	ecomp(buildtree(MINUSEQ, sp, p));

#ifdef MACHOABI	
	/* align to 16 bytes */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(PLUSEQ, sp, bcon(15)));
	
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(RSEQ, sp, bcon(4)));
	
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(LSEQ, sp, bcon(4)));
#endif
	

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_ap);
	slval(sp, 0);
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
ninval(CONSZ off, int fsz, P1ND *p)
{
	union { float f; double d; long double l; int i[3]; } u;
	int i;

	switch (p->n_type) {
	case LONGLONG:
	case ULONGLONG:
		i = (int)(glval(p) >> 32);
		slval(p, glval(p) & 0xffffffff);
		p->n_type = INT;
		inval(off, 32, p);
		slval(p, i);
		inval(off+32, 32, p);
		break;
	case LDOUBLE:
		u.i[2] = 0;
		u.l = (long double)((union flt *)p->n_dcon)->fp;
#if defined(HOST_BIG_ENDIAN)
		printf(PRTPREF "\t.long\t0x%x,0x%x,0x%x\n", u.i[2], u.i[1], u.i[0]);
#else
		printf(PRTPREF "\t.long\t%d,%d,%d\n", u.i[0], u.i[1], u.i[2] & 0177777);
#endif
		break;
	case DOUBLE:
		u.d = (double)((union flt *)p->n_dcon)->fp;
#if defined(HOST_BIG_ENDIAN)
		printf(PRTPREF "\t.long\t0x%x,0x%x\n", u.i[1], u.i[0]);
#else
		printf(PRTPREF "\t.long\t%d,%d\n", u.i[0], u.i[1]);
#endif
		break;
	case FLOAT:
		u.f = (float)((union flt *)p->n_dcon)->fp;
		printf(PRTPREF "\t.long\t%d\n", u.i[0]);
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
#if !defined(ELFABI)

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

#else

	return (p == NULL ? "" : p);

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
calldec(P1ND *p, P1ND *q) 
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

	name = getexname(sp);
	al = talign(sp->stype, sp->sap)/SZCHAR;
	off = (int)tsize(sp->stype, sp->sdf, sp->sap);
	SETOFF(off,SZCHAR);
	off /= SZCHAR;
#if defined(MACHOABI) || defined(PECOFFABI)/* && binutils>2.20 */
	al = ispow2(al);
	if (sp->sclass == STATIC) {
		if (sp->slevel == 0)
			printf(PRTPREF "\t.lcomm %s,0%o,%d\n", name, off, al);
		else
			printf(PRTPREF "\t.lcomm  " LABFMT ",0%o,%d\n", sp->soffset, off, al);
	} else {
		if (sp->slevel == 0)
			printf(PRTPREF "\t.comm %s,0%o,%d\n", name, off, al);
		else
			printf(PRTPREF "\t.comm  " LABFMT ",0%o,%d\n", sp->soffset, off, al);
	}
#elif defined(ELFABI)
#ifdef GCC_COMPAT
	if (attr_find(sp->sap, GCC_ATYP_WEAKREF) != NULL)
		return;
#endif
	if (sp->sclass == STATIC) {
		if (sp->slevel == 0) {
			printf(PRTPREF "\t.local %s\n", name);
		} else
			printf(PRTPREF "\t.local " LABFMT "\n", sp->soffset);
	}
	if (sp->slevel == 0)
		printf(PRTPREF "\t.comm %s,0%o,%d\n", name, off, al);
	else
		printf(PRTPREF "\t.comm  " LABFMT ",0%o,%d\n", sp->soffset, off, al);
#else
	if (attr_find(sp->sap, GCC_ATYP_WEAKREF) != NULL)
		return;
	if (sp->slevel == 0)
		printf(PRTPREF "\t.%scomm %s,0%o\n",
			sp->sclass == STATIC ? "l" : "", name, off);
	else
		printf(PRTPREF "\t.%scomm  " LABFMT ",0%o\n", 
			sp->sclass == STATIC ? "l" : "", sp->soffset, off);
#endif
}

#ifdef TLS
static int gottls;
#endif
#ifdef PECOFFABI
static int dllindirect;
#endif
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

#ifdef TLS
	if (strcmp(str, "tls") == 0 && a2 == NULL) {
		gottls = 1;
		return 1;
	}
#endif
#ifdef PECOFFABI
	if (strcmp(str, "dllimport") == 0) {
		dllindirect = 1;
		return 1;
	}
	if (strcmp(str, "dllexport") == 0) {
		dllindirect = 1;
		return 1;
	}
#endif
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

#ifdef TLS
	/* may have sanity checks here */
	if (gottls)
		sp->sflags |= STLS;
	gottls = 0;
#endif
#ifdef GCC_COMPAT
#ifdef HAVE_WEAKREF
	/* not many as'es have this directive */
	if ((ap = attr_find(sp->sap, GCC_ATYP_WEAKREF)) != NULL) {
		char *wr = ap->sarg(0);
		char *sn = getsoname(sp);
		if (sp->sclass != STATIC && sp->sclass != USTATIC)
			uerror("weakref %s must be static", sp->sname);
		if (wr == NULL) {
			if ((ap = attr_find(sp->sap, GCC_ATYP_ALIAS))) {
				wr = ap->sarg(0);
			}
		}
		if (wr == NULL)
			printf(PRTPREF "\t.weak %s\n", sn);
		else
			printf(PRTPREF "\t.weakref %s,%s\n", sn, wr);
	} else
#endif
	    if ((ap = attr_find(sp->sap, GCC_ATYP_ALIAS)) != NULL) {
		char *an = ap->sarg(0);	 
		char *v;
		char *sn = getsoname(sp);

		v = attr_find(sp->sap, GCC_ATYP_WEAK) ? "weak" : "globl";
		printf(PRTPREF "\t.%s %s\n", v, sn);
		printf(PRTPREF "\t.set %s,%s\n", sn, an);
	}	
#endif
	if (alias != NULL && (sp->sclass != PARAM)) {
		char *name = getexname(sp);
		printf(PRTPREF "\t.globl %s\n", name);
		printf(PRTPREF "%s = ", name);
		printf("%s\n", exname(alias));
		alias = NULL;
	}
	if ((constructor || destructor) && (sp->sclass != PARAM)) {
#if defined(ELFABI)
		printf(PRTPREF "\t.section .%ctors,\"aw\",@progbits\n",
		    constructor ? 'c' : 'd');
#elif defined(PECOFFABI)
		printf(PRTPREF "\t.section .%ctors,\"w\"\n",
		    constructor ? 'c' : 'd');
#elif defined(MACHOABI)
		if (kflag) {
			if (constructor)
				printf(PRTPREF "\t.mod_init_func\n");
			else
				printf(PRTPREF "\t.mod_term_func\n");
		} else {
			if (constructor)
				printf(PRTPREF "\t.constructor\n");
			else
				printf(PRTPREF "\t.destructor\n");
		}
#elif defined(AOUTABI)
		uerror("constructor/destructor are not supported for this target");
#endif
		printf(PRTPREF "\t.p2align 2\n");
		printf(PRTPREF "\t.long %s\n", exname(sp->sname));
#if defined(ELFABI)
		printf(PRTPREF "\t.previous\n");
#else
		printf(PRTPREF "\t.text\n");
#endif
		constructor = destructor = 0;
	}
#ifdef PECOFFABI
	if (dllindirect && (sp->sclass != PARAM)) {
		sp->sflags |= SDLLINDIRECT;
		dllindirect = 0;
	}
#endif
}

/*
 *  Postfix external functions with the arguments size.
 */
static void
mangle(P1ND *p)
{
	P1ND *l;

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
#ifdef PECOFFABI
	if (l->n_sp->sflags & SSTDCALL) {
		if (strchr(l->n_name, '@') == NULL) {
			int size = 0;
			char buf[256];
			P1ND *r;
			TWORD t;

			if (p->n_op == CALL || p->n_op == STCALL) {
				for (r = p->n_right;	
				    r->n_op == CM; r = r->n_left) {
					t = r->n_type;
					if (t == STRTY || t == UNIONTY)
						size += tsize(t, r->n_df, r->n_ap);
					else
						size += szty(t) * SZINT / SZCHAR;
				}
				t = r->n_type;
				if (t == STRTY || t == UNIONTY)
					size += tsize(t, r->n_df, r->n_ap);
				else
					size += szty(t) * SZINT / SZCHAR;
			}
			size = snprintf(buf, 256, "%s@%d", l->n_name, size) + 1;
	        	l->n_name = IALLOC(size);
			memcpy(l->n_name, buf, size);
		}
	}
#endif
}

void
pass1_lastchance(struct interpass *ip)
{
	if (ip->type == IP_NODE &&
	    (ip->ip_node->n_op == CALL || ip->ip_node->n_op == UCALL) &&
	    ISFTY(ip->ip_node->n_type))
		ip->ip_node->n_ap = attr_add(ip->ip_node->n_ap,
		    attr_new(ATTR_I386_FPPOP, 1));
 
	if (ip->type == IP_EPILOG) {
		struct interpass_prolog *ipp = (struct interpass_prolog *)ip;
		ipp->ipp_argstacksize = argstacksize;
	}
}

#ifdef PASS1
void
mflags(char *s)
{
}
#endif
