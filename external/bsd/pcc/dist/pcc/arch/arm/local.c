/*      Id: local.c,v 1.30 2011/06/05 17:21:17 ragge Exp     */	
/*      $NetBSD: local.c,v 1.1.1.4 2011/09/01 12:46:29 plunky Exp $    */
/*
 * Copyright (c) 2007 Gregory McGarry (g.mcgarry@ieee.org).
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

/*
 * We define location operations which operate on the expression tree
 * during the first pass (before sending to the backend for code generation.)
 */

#include <assert.h>

#include "pass1.h"

extern void defalign(int);

/*
 * clocal() is called to do local transformations on
 * an expression tree before being sent to the backend.
 */
NODE *
clocal(NODE *p)
{
	struct symtab *q;
	NODE *l, *r, *t;
	int o;
	int ty;
	int tmpnr, isptrvoid = 0;
	char *n;

	o = p->n_op;
	switch (o) {

	case STASG:

		l = p->n_left;
		r = p->n_right;
		if (r->n_op != STCALL && r->n_op != USTCALL)
			return p;

		/* assign left node as first argument to function */
		nfree(p);
		t = block(REG, NIL, NIL, r->n_type, r->n_df, r->n_sue);
		l->n_rval = R0;
		l = buildtree(ADDROF, l, NIL);
		l = buildtree(ASSIGN, t, l);

		if (r->n_right->n_op != CM) {
			r->n_right = block(CM, l, r->n_right, INT, 0, 0);
		} else {
			for (t = r->n_right; t->n_left->n_op == CM;
			    t = t->n_left)
				;
			t->n_left = block(CM, l, t->n_left, INT, 0, 0);
		}
		return r;

	case CALL:
	case STCALL:
	case USTCALL:
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
                r = tempnode(0, p->n_type, p->n_df, p->n_sue);
                tmpnr = regno(r);
                r = block(ASSIGN, r, p, p->n_type, p->n_df, p->n_sue);

                p = tempnode(tmpnr, r->n_type, r->n_df, r->n_sue);
                if (isptrvoid) {
                        p = block(PCONV, p, NIL, PTR+VOID, p->n_df, 0);
                }
                p = buildtree(COMOP, r, p);
                break;

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p;
		if (blevel == 0)
			return p;

		switch (q->sclass) {
		case PARAM:
		case AUTO:
                        /* fake up a structure reference */
                        r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
                        r->n_lval = 0;
                        r->n_rval = FPREG;
                        p = stref(block(STREF, r, p, 0, 0, 0));
                        break;
                case REGISTER:
                        p->n_op = REG;
                        p->n_lval = 0;
                        p->n_rval = q->soffset;
                        break;
                case STATIC:
                        if (q->slevel > 0) {
				p->n_lval = 0;
				p->n_sp = q;
			}
			/* FALL-THROUGH */
		default:
			ty = p->n_type;
			n = p->n_sp->soname ? p->n_sp->soname : p->n_sp->sname;
			if (strncmp(n, "__builtin", 9) == 0)
				break;
			p = block(ADDROF, p, NIL, INCREF(ty), p->n_df, p->n_sue);
			p = block(UMUL, p, NIL, ty, p->n_df, p->n_sue);
			break;
		}
		break;

	case STNAME:
		if ((q = p->n_sp) == NULL)
			return p;
		if (q->sclass != STNAME)
			return p;
		ty = p->n_type;
		p = block(ADDROF, p, NIL, INCREF(ty),
		    p->n_df, p->n_sue);
		p = block(UMUL, p, NIL, ty, p->n_df, p->n_sue);
		break;

        case FORCE:
                /* put return value in return reg */
                p->n_op = ASSIGN;
                p->n_right = p->n_left;
                p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
                p->n_left->n_rval = p->n_left->n_type == BOOL ? 
                    RETREG(BOOL_TYPE) : RETREG(p->n_type);
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

                if (l->n_op == ICON) {
                        CONSZ val = l->n_lval;

                        if (!ISPTR(p->n_type)) /* Pointers don't need to be conv'd */
                        switch (p->n_type) {
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
                                cerror("unknown type %d", l->n_type);
                        }
			l->n_type = p->n_type;
			l->n_sue = 0;
                        nfree(p);
                        return l;
                } else if (p->n_op == FCON) {
			l->n_lval = l->n_dcon;
			l->n_sp = NULL;
			l->n_op = ICON;
			l->n_type = p->n_type;
			l->n_sue = 0;
			nfree(p);
			return clocal(l);
		}
                if ((DEUNSIGN(p->n_type) == CHAR ||
                    DEUNSIGN(p->n_type) == SHORT) &&
                    (l->n_type == FLOAT || l->n_type == DOUBLE ||
                    l->n_type == LDOUBLE)) {
                        p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_sue);
                        p->n_left->n_type = INT;
                        return p;
                }
		break;

	case PCONV:
		l = p->n_left;
		if (l->n_op == ICON) {
			l->n_lval = (unsigned)l->n_lval;
			goto delp;
		}
		if (l->n_type < INT || DEUNSIGN(l->n_type) == LONGLONG) {
			p->n_left = block(SCONV, l, NIL, UNSIGNED, 0, 0);
			break;
		}
		if (l->n_op == SCONV)
			break;
		if (l->n_op == ADDROF && l->n_left->n_op == TEMP)
			goto delp;
		if (p->n_type > BTMASK && l->n_type > BTMASK)
			goto delp;
		break;

	delp:
		l->n_type = p->n_type;
		l->n_qual = p->n_qual;
		l->n_df = p->n_df;
		l->n_sue = p->n_sue;
		nfree(p);
		p = l;
		break;
	}

	return p;
}

/*
 * Called before sending the tree to the backend.
 */
void
myp2tree(NODE *p)
{
	struct symtab *sp;

	if (p->n_op != FCON)
		return;

#define IALLOC(sz)	(isinlining ? permalloc(sz) : tmpalloc(sz))

	sp = IALLOC(sizeof(struct symtab));
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

}

/*
 * Called during the first pass to determine if a NAME can be addressed.
 *
 * Return nonzero if supported, otherwise return 0.
 */
int
andable(NODE *p)
{
	if (blevel == 0)
		return 1;
	if (ISFTN(p->n_type))
		return 1;
	return 0;
}

/*
 * Return 1 if a variable of type 't' is OK to put in register.
 */
int
cisreg(TWORD t)
{
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return 0; /* not yet */
	return 1;
}

/*
 * Allocate bits from the stack for dynamic-sized arrays.
 *
 * 'p' is the tree which represents the type being allocated.
 * 'off' is the number of 'p's to be allocated.
 * 't' is the storeable node where the address is written.
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *sp;

	p = buildtree(MUL, p, bcon(off/SZCHAR)); /* XXX word alignment? */

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = SP;
	ecomp(buildtree(MINUSEQ, sp, p));

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_sue);
	sp->n_lval = 0;
	sp->n_rval = SP;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp));
}

/*
 * Print an integer constant node, may be associated with a label.
 * Do not free the node after use.
 * 'off' is bit offset from the beginning of the aggregate
 * 'fsz' is the number of bits this is referring to
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
	union { float f; double d; int i[2]; } u;
	struct symtab *q;
	TWORD t;
	int i, j;

	t = p->n_type;
	if (t > BTMASK)
		t = p->n_type = INT; /* pointer */

	if (p->n_op == ICON && p->n_sp != NULL && DEUNSIGN(t) != INT)
		uerror("element not constant");

	switch (t) {
	case LONGLONG:
	case ULONGLONG:
		i = (p->n_lval >> 32);
		j = (p->n_lval & 0xffffffff);
		p->n_type = INT;
		if (features(FEATURE_BIGENDIAN)) {
			p->n_lval = i;
			ninval(off+32, 32, p);
			p->n_lval = j;
			ninval(off, 32, p);
		} else {
			p->n_lval = j;
			ninval(off, 32, p);
			p->n_lval = i;
			ninval(off+32, 32, p);
		}
		break;
	case INT:
	case UNSIGNED:
		printf("\t.word 0x%x", (int)p->n_lval);
		if ((q = p->n_sp) != NULL) {
			if ((q->sclass == STATIC && q->slevel > 0)) {
				printf("+" LABFMT, q->soffset);
			} else
				printf("+%s",
				    q->soname ? q->soname : exname(q->sname));
		}
		printf("\n");
		break;
	case LDOUBLE:
	case DOUBLE:
		u.d = (double)p->n_dcon;
#if defined(HOST_BIG_ENDIAN)
		if (features(FEATURE_BIGENDIAN))
#else
		if (!features(FEATURE_BIGENDIAN))
#endif
			printf("\t.word\t0x%x\n\t.word\t0x%x\n",
			    u.i[0], u.i[1]);
		else
			printf("\t.word\t0x%x\n\t.word\t0x%x\n",
			    u.i[1], u.i[0]);
		break;
	case FLOAT:
		u.f = (float)p->n_dcon;
		printf("\t.word\t0x%x\n", u.i[0]);
		break;
	default:
		return 0;
	}
	return 1;
}

/*
 * Prefix a leading underscore to a global variable (if necessary).
 */
char *
exname(char *p)
{
	return (p == NULL ? "" : p);
}

/*
 * Map types which are not defined on the local machine.
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
	}
	return (type);
}

/*
 * Before calling a function do any tree re-writing for the local machine.
 *
 * 'p' is the function tree (NAME)
 * 'q' is the CM-separated list of arguments.
 */
void
calldec(NODE *p, NODE *q) 
{
}

/*
 * While handling uninitialised variables, handle variables marked extern.
 */
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
	off = (off+(SZCHAR-1))/SZCHAR;
	printf("        .%scomm ", sp->sclass == STATIC ? "l" : "");
	if (sp->slevel == 0)
		printf("%s,0%o\n",
		    sp->soname ? sp->soname : exname(sp->sname), off);
	else
		printf(LABFMT ",0%o\n", sp->soffset, off);
}

/*
 * va_start(ap, last) implementation.
 *
 * f is the NAME node for this builtin function.
 * a is the argument list containing:
 *	   CM
 *	ap   last
 */
NODE *
arm_builtin_stdarg_start(NODE *f, NODE *a)
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
                sz = SZINT / tsize(p->n_type, p->n_df, p->n_sue);
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
arm_builtin_va_arg(NODE *f, NODE *a)
{
        NODE *p, *q, *r;
        int sz, tmpnr;

        /* check num args and type */
        if (a == NULL || a->n_op != CM || a->n_left->n_op == CM ||
            !ISPTR(a->n_left->n_type) || a->n_right->n_op != TYPE)
                goto bad;

        r = a->n_right;

        /* get type size */
        sz = tsize(r->n_type, r->n_df, r->n_sue) / SZCHAR;
        if (sz < SZINT/SZCHAR) {
                werror("%s%s promoted to int when passed through ...",
                        ISUNSIGNED(r->n_type) ? "unsigned " : "",
                        DEUNSIGN(r->n_type) == SHORT ? "short" : "char");
                sz = SZINT/SZCHAR;
        }

        /* alignment */
        p = tcopy(a->n_left);
        if (sz > SZINT/SZCHAR && r->n_type != UNIONTY && r->n_type != STRTY) {
                p = buildtree(PLUS, p, bcon(ALSTACK/8 - 1));
                p = block(AND, p, bcon(-ALSTACK/8), p->n_type, p->n_df, p->n_sue);
        }

        /* create a copy to a temp node */
        q = tempnode(0, p->n_type, p->n_df, p->n_sue);
        tmpnr = regno(q);
        p = buildtree(ASSIGN, q, p);

        q = tempnode(tmpnr, p->n_type, p->n_df,p->n_sue);
        q = buildtree(PLUS, q, bcon(sz));
        q = buildtree(ASSIGN, a->n_left, q);

        q = buildtree(COMOP, p, q);

        nfree(a->n_right);
        nfree(a);
        nfree(f);

        p = tempnode(tmpnr, INCREF(r->n_type), r->n_df, r->n_sue);
        p = buildtree(UMUL, p, NIL);
        p = buildtree(COMOP, q, p);

        return p;

bad:
        uerror("bad argument to __builtin_va_arg");
        return bcon(0);
}

NODE *
arm_builtin_va_end(NODE *f, NODE *a)
{
	tfree(f);
	tfree(a);
 
	return bcon(0);
}

NODE *
arm_builtin_va_copy(NODE *f, NODE *a)
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

char *nextsect;
static int constructor;
static int destructor;

/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	char *a2 = pragtok(NULL);

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
	if (strcmp(str, "section") || s2 == NULL)
		return 0;
	nextsect = newstring(s2, strlen(s2));
	return 1;
}

/*
 * Called when a identifier has been declared, to give target last word.
 */
void
fixdef(struct symtab *sp)
{
	if ((constructor || destructor) && (sp->sclass != PARAM)) {
		printf("\t.section .%ctors,\"aw\",@progbits\n",
		    constructor ? 'c' : 'd');
		printf("\t.p2align 2\n");
		printf("\t.long %s\n", exname(sp->sname));
		printf("\t.previous\n");
		constructor = destructor = 0;
	}
}

void
pass1_lastchance(struct interpass *ip)
{
}
