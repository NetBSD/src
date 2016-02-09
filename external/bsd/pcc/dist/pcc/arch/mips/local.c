/*	Id: local.c,v 1.37 2015/12/31 16:21:57 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.6 2016/02/09 20:28:21 plunky Exp $	*/
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

/*
 * MIPS port by Jan Enoksson (janeno-1@student.ltu.se) and
 * Simon Olsson (simols-1@student.ltu.se) 2005.
 */

#include "pass1.h"

#ifndef LANG_CXX
#define NODE P1ND
#define ccopy p1tcopy
#define tcopy p1tcopy
#define tfree p1tfree
#define nfree p1nfree
#define fwalk p1fwalk
#define talloc p1alloc
#endif

#define IALLOC(sz) (isinlining ? permalloc(sz) : tmpalloc(sz))

/* this is called to do local transformations on
 * an expression tree preparitory to its being
 * written out in intermediate code.
 */
NODE *
clocal(NODE *p)
{
	struct symtab *q;
	NODE *r, *l;
	int o;
	int m;
	TWORD ty;
	int tmpnr, isptrvoid = 0;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal in: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif

	switch (o = p->n_op) {

	case UCALL:
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
		r = tempnode(0, p->n_type, p->n_df, p->n_ap);
		tmpnr = regno(r);
		r = block(ASSIGN, r, p, p->n_type, p->n_df, p->n_ap);

		p = tempnode(tmpnr, r->n_type, r->n_df, r->n_ap);
		if (isptrvoid) {
			p = block(PCONV, p, NIL, PTR+VOID, p->n_df, 0);
		}
		p = buildtree(COMOP, r, p);
		break;

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */

		switch (q->sclass) {

		case PARAM:
		case AUTO:
			/* fake up a structure reference */
			r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			slval(r, 0);
			r->n_rval = FP;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case STATIC:
			if (q->slevel == 0)
				break;
			slval(p, 0);
			p->n_sp = q;
			break;

		case REGISTER:
			p->n_op = REG;
			slval(p, 0);
			p->n_rval = q->soffset;
			break;

		}
		break;

	case FUNARG:
		/* Args smaller than int are given as int */
		if (p->n_type != CHAR && p->n_type != UCHAR && 
		    p->n_type != SHORT && p->n_type != USHORT)
			break;
		p->n_left = block(SCONV, p->n_left, NIL, INT, 0, 0);
		p->n_type = INT;
		p->n_ap = 0;
		p->n_rval = SZINT;
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
				ty = r->n_type;
				nfree(l->n_left);
				l->n_left = r;
				l->n_type = ty;
				l->n_right->n_type = ty;
			}
#if 0
			  else if (l->n_right->n_op == SCONV &&
			    l->n_left->n_type == l->n_right->n_type) {
				r = l->n_left->n_left;
				nfree(l->n_left);
				l->n_left = r;
				r = l->n_right->n_left;
				nfree(l->n_right);
				l->n_right = r;
			}
#endif
		}
		break;

	case PCONV:
		/* Remove redundant PCONV's. Be careful */
		l = p->n_left;
		if (l->n_op == ICON) {
			slval(l, (unsigned)glval(l));
			goto delp;
		}
		if (l->n_type < INT || DEUNSIGN(l->n_type) == LONGLONG) {
			/* float etc? */
			p->n_left = block(SCONV, l, NIL, UNSIGNED, 0, 0);
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
			p = l;
			break;
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
					p = l;
					break;
				}
			}
		}

		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT &&
		    coptype(l->n_op) == BITYPE) {
			l->n_type = p->n_type;
			nfree(p);
			p = l;
		}

		if (DEUNSIGN(p->n_type) == SHORT &&
		    DEUNSIGN(l->n_type) == SHORT) {
			nfree(p);
			p = l;
		}

		/* convert float/double to int before to (u)char/(u)short */
		if ((DEUNSIGN(p->n_type) == CHAR ||
		    DEUNSIGN(p->n_type) == SHORT) &&
                    (l->n_type == FLOAT || l->n_type == DOUBLE ||
		    l->n_type == LDOUBLE)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_ap);
			p->n_left->n_type = INT;
			break;
                }

		/* convert (u)char/(u)short to int before float/double */
		if  ((p->n_type == FLOAT || p->n_type == DOUBLE ||
		    p->n_type == LDOUBLE) && (DEUNSIGN(l->n_type) == CHAR ||
		    DEUNSIGN(l->n_type) == SHORT)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_ap);
			p->n_left->n_type = INT;
			break;
                }

		o = l->n_op;
		m = p->n_type;

		if (o == ICON) {
			CONSZ val = glval(l);

			if (!ISPTR(m)) /* Pointers don't need to be conv'd */
			    switch (m) {
			case BOOL:
				val = nncon(l) ? (val != 0) : 1;
				slval(l, val);
				l->n_sp = NULL;
				break;
			case CHAR:
				slval(l, (char)val);
				break;
			case UCHAR:
				slval(l, val & 0377);
				break;
			case SHORT:
				slval(l, (short)val);
				break;
			case USHORT:
				slval(l, val & 0177777);
				break;
			case ULONG:
			case UNSIGNED:
				slval(l, val & 0xffffffff);
				break;
			case LONG:
			case INT:
				slval(l, (int)val);
				break;
			case LONGLONG:
				slval(l, (long long)val);
				break;
			case ULONGLONG:
				slval(l, val);
				break;
			case VOID:
				break;
			case LDOUBLE:
			case DOUBLE:
			case FLOAT:
				l->n_op = FCON;
				l->n_dcon = tmpalloc(sizeof(union flt));
				((union flt *)l->n_dcon)->fp = val;
				break;
			default:
				cerror("unknown type %d", m);
			}
			l->n_type = m;
			nfree(p);
			p = l;
		} else if (o == FCON) {
			CONSZ lv;
			if (p->n_type == BOOL)
				lv = !FLOAT_ISZERO(((union flt *)l->n_dcon));
			else {
				FLOAT_FP2INT(lv, ((union flt *)l->n_dcon), m);
			}
			slval(l, lv);
			l->n_sp = NULL;
			l->n_op = ICON;
			l->n_type = m;
			l->n_ap = 0;
			nfree(p);
			p = clocal(l);
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

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = RETREG(p->n_type);
		break;
	}

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal out: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif

	return(p);
}

void
myp2tree(NODE *p)
{
	struct symtab *sp;

	if (p->n_op != FCON) 
		return;

	/* Write float constants to memory */
 
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
	slval(p, 0);
	p->n_sp = sp;

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
	if (t == INT || t == UNSIGNED || t == LONG || t == ULONG)
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
	int nbytes = off / SZCHAR;

	p = buildtree(MUL, p, bcon(nbytes));
	p = buildtree(PLUS, p, bcon(7));
	p = buildtree(AND, p, bcon(~7));

	/* subtract the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	slval(sp, 0);
	sp->n_rval = SP;
	ecomp(buildtree(MINUSEQ, sp, p));

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_ap);
	sp->n_rval = SP;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */
}

/*
 * print out a constant node
 * mat be associated with a label
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
        union { float f; double d; int i[2]; } u;
        struct symtab *q;
        TWORD t;
#ifndef USE_GAS
        int i, j;
#endif

        t = p->n_type;
        if (t > BTMASK)
		p->n_type = t = INT; /* pointer */

        if (p->n_op == ICON && p->n_sp != NULL && DEUNSIGN(t) != INT)
                uerror("element not constant");

        switch (t) {
        case LONGLONG:
        case ULONGLONG:
#ifdef USE_GAS
                printf("\t.dword %lld\n", (long long)glval(p));
#else
		i = glval(p) >> 32;
                j = glval(p) & 0xffffffff;
                p->n_type = INT;
		if (bigendian) {
			slval(p, j);
	                ninval(off, 32, p);
			slval(p, i);
			ninval(off+32, 32, p);
		} else {
			slval(p, i);
	                ninval(off, 32, p);
			slval(p, j);
			ninval(off+32, 32, p);
		}
#endif
                break;
        case INT:
        case UNSIGNED:
                printf("\t.word " CONFMT, (CONSZ)glval(p));
                if ((q = p->n_sp) != NULL) {
                        if ((q->sclass == STATIC && q->slevel > 0)) {
                                printf("+" LABFMT, q->soffset);
                        } else
                                printf("+%s", getexname(q));
                }
                printf("\n");
                break;
        case SHORT:
        case USHORT:
		astypnames[SHORT] = astypnames[USHORT] = "\t.half";
                return 0;
        case LDOUBLE:
        case DOUBLE:
                u.d = (double)((union flt *)p->n_dcon)->fp;
		if (bigendian) {
	                printf("\t.word\t%d\n", u.i[0]);
			printf("\t.word\t%d\n", u.i[1]);
		} else {
			printf("\t.word\t%d\n", u.i[1]);
	                printf("\t.word\t%d\n", u.i[0]);
		}
                break;
        case FLOAT:
                u.f = (float)((union flt *)p->n_dcon)->fp;
                printf("\t.word\t0x%x\n", u.i[0]);
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

	off = tsize(sp->stype, sp->sdf, sp->sap);
	off = (off+(SZCHAR-1))/SZCHAR;
	printf("	.%scomm ", sp->sclass == STATIC ? "l" : "");
	if (sp->slevel == 0)
		printf("%s,0%o\n", getexname(sp), off);
	else
		printf(LABFMT ",0%o\n", sp->soffset, off);
}


#ifdef notdef
/* make a common declaration for id, if reasonable */
void
commdec(struct symtab *q)
{
	int off;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;

	printf("	.comm %s,%d\n", exname(q->soname), off);
}

/* make a local common declaration for id, if reasonable */
void
lcommdec(struct symtab *q)
{
	int off;

	off = tsize(q->stype, q->sdf, q->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;
	if (q->slevel == 0)
		printf("\t.lcomm %s,%d\n", exname(q->soname), off);
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

/* ro-text, rw-data, ro-data, ro-strings */
static char *loctbl[] = { "text", "data", "rdata", "rdata" };

void
setloc1(int locc)
{
	if (locc == lastloc && locc != STRNG)
		return;
	if (locc == RDATA && lastloc == STRNG)
		return;

	if (locc != lastloc) {
		lastloc = locc;
		printf("\t.%s\n", loctbl[locc]);
	}

	if (locc == STRNG)
		printf("\t.align 2\n");
}
#endif

/*
 * va_start(ap, last) implementation.
 *
 * f is the NAME node for this builtin function.
 * a is the argument list containing:
 *	   CM
 *	ap   last
 *
 * It turns out that this is easy on MIPS.  Just write the
 * argument registers to the stack in va_arg_start() and
 * use the traditional method of walking the stackframe.
 */
NODE *
mips_builtin_stdarg_start(const struct bitable *bt, NODE *a)
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

	p = buildtree(ADDROF, p, NIL);	/* address of last arg */
	p = optim(buildtree(PLUS, p, bcon(sz)));
	q = block(NAME, NIL, NIL, PTR+VOID, 0, 0);
	q = buildtree(CAST, q, p);
	p = q->n_right;
	nfree(q->n_left);
	nfree(q);
	p = buildtree(ASSIGN, a->n_left, p);
	nfree(a);

	return p;

bad:
	uerror("bad argument to __builtin_stdarg_start");
	return bcon(0);
}

NODE *
mips_builtin_va_arg(const struct bitable *bt, NODE *a)
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
			r->n_type & 1 ? "unsigned " : "",
			DEUNSIGN(r->n_type) == SHORT ? "short" : "char");
		sz = SZINT/SZCHAR;
	}

	/* alignment */
	p = tcopy(a->n_left);
	if (sz > SZINT/SZCHAR && r->n_type != UNIONTY && r->n_type != STRTY) {
		p = buildtree(PLUS, p, bcon(7));
		p = block(AND, p, bcon(-8), p->n_type, p->n_df, p->n_ap);
	}

	/* create a copy to a temp node */
	q = tempnode(0, p->n_type, p->n_df, p->n_ap);
	tmpnr = regno(q);
	p = buildtree(ASSIGN, q, p);

	q = tempnode(tmpnr, p->n_type, p->n_df,p->n_ap);
	q = buildtree(PLUS, q, bcon(sz));
	q = buildtree(ASSIGN, a->n_left, q);

	q = buildtree(COMOP, p, q);

	nfree(a->n_right);
	nfree(a);

	p = tempnode(tmpnr, INCREF(r->n_type), r->n_df, r->n_ap);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(COMOP, q, p);

	return p;

bad:
	uerror("bad argument to __builtin_va_arg");
	return bcon(0);
}

NODE *
mips_builtin_va_end(const struct bitable *bt, NODE *a)
{
	tfree(a);
	return bcon(0);
}

NODE *
mips_builtin_va_copy(const struct bitable *bt, NODE *a)
{
	NODE *f;

	if (a == NULL || a->n_op != CM || a->n_left->n_op == CM)
		goto bad;
	f = buildtree(ASSIGN, a->n_left, a->n_right);
	nfree(a);
	return f;

bad:
	uerror("bad argument to __buildtin_va_copy");
	return bcon(0);
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
