/*	Id: builtins.c,v 1.66 2016/02/09 17:57:35 ragge Exp 	*/	
/*	$NetBSD: builtins.c,v 1.1.1.6 2016/02/09 20:28:55 plunky Exp $	*/
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

#define	ccopy p1tcopy

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef NO_C_BUILTINS

extern int dimfuncnt;
/*
 * replace an alloca function with direct allocation on stack.
 * return a destination temp node.
 */
static P1ND *
builtin_alloca(const struct bitable *bt, P1ND *a)
{
	P1ND *t, *u;

#ifdef notyet
	if (xnobuiltins)
		return NULL;
#endif

	t = tempnode(0, VOID|PTR, 0, 0);
	u = tempnode(regno(t), VOID|PTR, 0, 0);
	spalloc(t, a, SZCHAR);
	return u;
}

/*
 * Determine if a value is known to be constant at compile-time and
 * hence that PCC can perform constant-folding on expressions involving
 * that value.
 */
static P1ND *
builtin_constant_p(const struct bitable *bt, P1ND *a)
{
	P1ND *f;
	int isconst;

	p1walkf(a, putjops, 0);
	for (f = a; f->n_op == COMOP; f = f->n_right)
		;
	isconst = nncon(f);
	p1tfree(a);
	return bcon(isconst);
}

/*
 * Hint to the compiler whether this expression will evaluate true or false.
 * Just ignored for now.
 */
static P1ND *
builtin_expect(const struct bitable *bt, P1ND *a)
{
	P1ND *f;

	if (a && a->n_op == CM) {
		p1tfree(a->n_right);
		f = a->n_left;
		p1nfree(a);
		a = f;
	}

	return a;
}

/*
 * Take integer absolute value.
 * Simply does: ((((x)>>(8*sizeof(x)-1))^(x))-((x)>>(8*sizeof(x)-1)))
 */
static P1ND *
builtin_abs(const struct bitable *bt, P1ND *a)
{
	P1ND *p, *q, *r, *t, *t2, *t3;
	int tmp1, tmp2, shift;

	if (a->n_type != INT)
		a = cast(a, INT, 0);

	if (a->n_op == ICON) {
		if (glval(a) < 0)
			slval(a, -glval(a));
		p = a;
	} else {
		t = tempnode(0, a->n_type, a->n_df, a->n_ap);
		tmp1 = regno(t);
		p = buildtree(ASSIGN, t, a);

		t = tempnode(tmp1, a->n_type, a->n_df, a->n_ap);
		shift = (int)tsize(a->n_type, a->n_df, a->n_ap) - 1;
		q = buildtree(RS, t, bcon(shift));

		t2 = tempnode(0, a->n_type, a->n_df, a->n_ap);
		tmp2 = regno(t2);
		q = buildtree(ASSIGN, t2, q);

		t = tempnode(tmp1, a->n_type, a->n_df, a->n_ap);
		t2 = tempnode(tmp2, a->n_type, a->n_df, a->n_ap);
		t3 = tempnode(tmp2, a->n_type, a->n_df, a->n_ap);
		r = buildtree(MINUS, buildtree(ER, t, t2), t3);

		p = buildtree(COMOP, p, buildtree(COMOP, q, r));
	}

	return p;
}

#define	cmop(x,y) buildtree(COMOP, x, y)
#define	lblnod(l) nlabel(l)

#ifndef TARGET_BSWAP
static P1ND *
builtin_bswap16(const struct bitable *bt, P1ND *a)
{
	P1ND *f, *t1, *t2;

	t1 = buildtree(LS, buildtree(AND, ccopy(a), bcon(255)), bcon(8));
	t2 = buildtree(AND, buildtree(RS, a, bcon(8)), bcon(255));
	f = buildtree(OR, t1, t2);
	return f;
}

static P1ND *
builtin_bswap32(const struct bitable *bt, P1ND *a)
{
	P1ND *f, *t1, *t2, *t3, *t4;

	t1 = buildtree(LS, buildtree(AND, ccopy(a), bcon(255)), bcon(24));
	t2 = buildtree(LS, buildtree(AND, ccopy(a), bcon(255 << 8)), bcon(8));
	t3 = buildtree(AND, buildtree(RS, ccopy(a), bcon(8)), bcon(255 << 8));
	t4 = buildtree(AND, buildtree(RS, a, bcon(24)), bcon(255));
	f = buildtree(OR, buildtree(OR, t1, t2), buildtree(OR, t3, t4));
	return f;
}

static P1ND *
builtin_bswap64(const struct bitable *bt, P1ND *a)
{
	P1ND *f, *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8;

#define	X(x) xbcon(x, NULL, ctype(ULONGLONG))
	t1 = buildtree(LS, buildtree(AND, ccopy(a), X(255)), bcon(56));
	t2 = buildtree(LS, buildtree(AND, ccopy(a), X(255 << 8)), bcon(40));
	t3 = buildtree(LS, buildtree(AND, ccopy(a), X(255 << 16)), bcon(24));
	t4 = buildtree(LS, buildtree(AND, ccopy(a), X(255 << 24)), bcon(8));
	t5 = buildtree(AND, buildtree(RS, ccopy(a), bcon(8)), X(255 << 24));
	t6 = buildtree(AND, buildtree(RS, ccopy(a), bcon(24)), X(255 << 16));
	t7 = buildtree(AND, buildtree(RS, ccopy(a), bcon(40)), X(255 << 8));
	t8 = buildtree(AND, buildtree(RS, a, bcon(56)), X(255));
	f = buildtree(OR,
	    buildtree(OR, buildtree(OR, t1, t2), buildtree(OR, t3, t4)),
	    buildtree(OR, buildtree(OR, t5, t6), buildtree(OR, t7, t8)));
	return f;
#undef X
}

#endif

#ifndef TARGET_CXZ
/*
 * Find number of beginning 0's in a word of type t.
 * t should be deunsigned.
 */
static P1ND *
builtin_cxz(P1ND *a, TWORD t, int isclz)
{
	P1ND *t101, *t102;
	P1ND *rn, *p;
	int l15, l16, l17;
	int sz;

	t = ctype(t);
	sz = (int)tsize(t, 0, 0);

	t101 = tempnode(0, INT, 0, 0);
	t102 = tempnode(0, t, 0, 0);
	l15 = getlab();
	l16 = getlab();
	l17 = getlab();
	rn = buildtree(ASSIGN, ccopy(t102), a);
	rn = cmop(rn, buildtree(ASSIGN, ccopy(t101), bcon(0)));
	rn = cmop(rn, lblnod(l16));

	p = buildtree(CBRANCH, buildtree(GE, ccopy(t101), bcon(sz)), bcon(l15));
	rn = cmop(rn, p);
	if (isclz) {
		p = buildtree(CBRANCH,
		    buildtree(GE, ccopy(t102), bcon(0)), bcon(l17));
	} else {
		p = buildtree(CBRANCH,
		    buildtree(EQ, buildtree(AND, ccopy(t102), bcon(1)),
		    bcon(0)), bcon(l17));
	}
	rn = cmop(rn, p);

	rn = cmop(rn, block(GOTO, bcon(l15), NULL, INT, 0, 0));

	rn = cmop(rn, lblnod(l17));
	rn = cmop(rn, buildtree(isclz ? LSEQ : RSEQ , t102, bcon(1)));

	rn = cmop(rn, buildtree(INCR, ccopy(t101), bcon(1)));

	rn = cmop(rn, block(GOTO, bcon(l16), NULL, INT, 0, 0));
	rn = cmop(rn, lblnod(l15));
	return cmop(rn, t101);
}

static P1ND *
builtin_clz(const struct bitable *bt, P1ND *a)
{
	return builtin_cxz(a, INT, 1);
}

static P1ND *
builtin_clzl(const struct bitable *bt, P1ND *a)
{
	return builtin_cxz(a, LONG, 1);
}

static P1ND *
builtin_clzll(const struct bitable *bt, P1ND *a)
{
	return builtin_cxz(a, LONGLONG, 1);
}

static P1ND *
builtin_ctz(const struct bitable *bt, P1ND *a)
{
	return builtin_cxz(a, INT, 0);
}

static P1ND *
builtin_ctzl(const struct bitable *bt, P1ND *a)
{
	return builtin_cxz(a, LONG, 0);
}

static P1ND *
builtin_ctzll(const struct bitable *bt, P1ND *a)
{
	return builtin_cxz(a, LONGLONG, 0);
}
#endif

#ifndef TARGET_ERA
static P1ND *
builtin_era(const struct bitable *bt, P1ND *a)
{
	return a;	/* Just pass through */
}
#endif

#ifndef TARGET_FFS
/*
 * Find number of beginning 0's in a word of type t.
 * t should be deunsigned.
 */
static P1ND *
builtin_ff(P1ND *a, TWORD t)
{
	P1ND *t101, *t102;
	P1ND *rn, *p;
	int l15, l16, l17;
	int sz;

	t = ctype(t);
	sz = (int)tsize(t, 0, 0)+1;

	t101 = tempnode(0, INT, 0, 0);
	t102 = tempnode(0, t, 0, 0);
	l15 = getlab();
	l16 = getlab();
	l17 = getlab();
	rn = buildtree(ASSIGN, ccopy(t101), bcon(0));
	rn = cmop(rn, buildtree(ASSIGN, ccopy(t102), a));

	p = buildtree(CBRANCH, buildtree(EQ, ccopy(t102), bcon(0)), bcon(l15));
	rn = cmop(rn, p);

	rn = cmop(rn, buildtree(INCR, ccopy(t101), bcon(1)));

	rn = cmop(rn, lblnod(l16));

	p = buildtree(CBRANCH, buildtree(GE, ccopy(t101), bcon(sz)), bcon(l15));
	rn = cmop(rn, p);

	p = buildtree(CBRANCH,
	    buildtree(EQ, buildtree(AND, ccopy(t102), bcon(1)),
	    bcon(0)), bcon(l17));
	rn = cmop(rn, p);

	rn = cmop(rn, block(GOTO, bcon(l15), NULL, INT, 0, 0));

	rn = cmop(rn, lblnod(l17));
	rn = cmop(rn, buildtree(RSEQ, t102, bcon(1)));

	rn = cmop(rn, buildtree(INCR, ccopy(t101), bcon(1)));

	rn = cmop(rn, block(GOTO, bcon(l16), NULL, INT, 0, 0));
	rn = cmop(rn, lblnod(l15));
	return cmop(rn, t101);
}

static P1ND *
builtin_ffs(const struct bitable *bt, P1ND *a)
{
	return builtin_ff(a, INT);
}

static P1ND *
builtin_ffsl(const struct bitable *bt, P1ND *a)
{
	return builtin_ff(a, LONG);
}

static P1ND *
builtin_ffsll(const struct bitable *bt, P1ND *a)
{
	return builtin_ff(a, LONGLONG);
}
#endif

/*
 * Get size of object, if possible.
 * 0 = whole object, 1 == closest object.  Return -1 if not available.
 * 2 == max of rem object, 3 == min of rem obj.  Return 0 if not available.
 */
static P1ND *
builtin_object_size(const struct bitable *bt, P1ND *a)
{
	CONSZ v = icons(a->n_right);
	int r;

	if (v < 0 || v > 3)
		uerror("arg2 must be between 0 and 3");
	r = v < 2 ? -1 : 0;

	a = p1nfree(a);
#ifdef notyet
	if (ISPTR(a->n_type)) {
		a = buildtree(UMUL, a, 0);
		a = optloop(a);
		a = doszof(a);
	}
#else
	p1walkf(a, putjops, 0); /* if ?: exists */
	p1tfree(a);
#endif
	return xbcon(r, NULL, bt->rt);
}

#ifndef TARGET_STDARGS
static P1ND *
builtin_stdarg_start(const struct bitable *bt, P1ND *a)
{
	P1ND *p, *q;
	int sz;

	/* must first deal with argument size; use int size */
	p = a->n_right;
	if (p->n_type < INT) {
		sz = (int)(SZINT/tsize(p->n_type, p->n_df, p->n_ap));
	} else
		sz = 1;

	/* do the real job */
	p = buildtree(ADDROF, p, NULL); /* address of last arg */
#ifdef BACKAUTO
	p = optim(buildtree(PLUS, p, bcon(sz))); /* add one to it (next arg) */
#else
	p = optim(buildtree(MINUS, p, bcon(sz))); /* add one to it (next arg) */
#endif
	q = block(NAME, NULL, NULL, PTR+VOID, 0, 0); /* create cast node */
	q = buildtree(CAST, q, p); /* cast to void * (for assignment) */
	p = q->n_right;
	p1nfree(q->n_left);
	p1nfree(q);
	p = buildtree(ASSIGN, a->n_left, p); /* assign to ap */
	p1nfree(a);
	return p;
}

static P1ND *
builtin_va_arg(const struct bitable *bt, P1ND *a)
{
	P1ND *p, *q, *r, *rv;
	int sz, nodnum;

	/* create a copy to a temp node of current ap */
	p = ccopy(a->n_left);
	q = tempnode(0, p->n_type, p->n_df, p->n_ap);
	nodnum = regno(q);
	rv = buildtree(ASSIGN, q, p);

	r = a->n_right;
	sz = (int)tsize(r->n_type, r->n_df, r->n_ap);
#ifdef MYVAARGSZ
	SETOFF(sz, MYVAARGSZ);
#endif
	sz /= SZCHAR;
	/* add one to ap */
#ifdef BACKAUTO
	rv = buildtree(COMOP, rv , buildtree(PLUSEQ, a->n_left, bcon(sz)));
#else
#error fix wrong eval order in builtin_va_arg
	ecomp(buildtree(MINUSEQ, a->n_left, bcon(sz)));
#endif

	p1nfree(a->n_right);
	p1nfree(a);
	r = tempnode(nodnum, INCREF(r->n_type), r->n_df, r->n_ap);
	return buildtree(COMOP, rv, buildtree(UMUL, r, NULL));

}

static P1ND *
builtin_va_end(const struct bitable *bt, P1ND *a)
{
	return a; /* may have side effects */
}

static P1ND *
builtin_va_copy(const struct bitable *bt, P1ND *a)
{
	P1ND *f;

	f = buildtree(ASSIGN, a->n_left, a->n_right);
	p1nfree(a);
	return f;
}
#endif /* TARGET_STDARGS */

/*
 * For unimplemented "builtin" functions, try to invoke the
 * non-builtin name
 */
static P1ND *
binhelp(P1ND *a, TWORD rt, char *n)
{
	P1ND *f = block(NAME, NULL, NULL, INT, 0, 0);
	int oblvl = blevel;

	blevel = 0;
	f->n_sp = lookup(addname(n), SNORMAL);
	blevel = oblvl;
	if (f->n_sp->sclass == SNULL) {
		f->n_sp->sclass = EXTERN;
		f->n_sp->stype = INCREF(rt)+(FTN-PTR);
		f->n_sp->sdf = permalloc(sizeof(union dimfun));
		dimfuncnt++;
		f->n_sp->sdf->dfun = NULL;
	}
	f->n_type = f->n_sp->stype;
	f = clocal(f);
	return buildtree(CALL, f, a);
}

static P1ND *
builtin_unimp(const struct bitable *bt, P1ND *a)
{
	return binhelp(a, bt->rt, &bt->name[10]);
}

#if 0
static P1ND *
builtin_unimp_f(P1ND *f, P1ND *a, TWORD rt)
{
	return binhelp(f, a, rt, f->n_sp->sname);
}
#endif

#ifndef TARGET_PREFETCH
static P1ND *
builtin_prefetch(const struct bitable *bt, P1ND *a)
{
	p1tfree(a);
	return bcon(0);
}
#endif

/*
 * check if compatible types.
 * XXX - all enum are considered equal types
 */
static P1ND *
builtin_tc(const struct bitable *bt, P1ND *a)
{
	P1ND *p;

	if (a == NULL || a->n_op != CM ||
	    a->n_left->n_op != TYPE || a->n_right->n_op != TYPE)
		uerror("bad %s arg", bt->name);
	
	p = bcon(a->n_left->n_type == a->n_right->n_type);
	p1nfree(a->n_left);
	p1nfree(a->n_right);
	p1nfree(a);
	return p;
}

static void
putinlbl(P1ND *p, void *arg)
{
	if (p->n_op == COMOP && p->n_left->n_op == GOTO) {
		int v = (int)glval(p->n_left->n_left);
		send_passt(IP_DEFLAB, v+1);
	}
}

/*
 * Similar to ?:
 */
static P1ND *
builtin_ce(const struct bitable *bt, P1ND *a)
{
	P1ND *p;

	if (a == NULL || a->n_op != CM ||
	    a->n_left->n_op != CM || a->n_left->n_left->n_op == CM)
		uerror("bad %s arg", bt->name);
	if (nncon(a->n_left->n_left) == 0)
		uerror("arg not constant");
	if (glval(a)) {
		p = a->n_left->n_right;
		a->n_left->n_op = UMUL; /* for p1tfree() */
		p1walkf(a->n_right, putinlbl, 0);
	} else {
		p = a->n_right;
		a->n_op = UMUL; /* for p1tfree() */
		p1walkf(a->n_left->n_right, putinlbl, 0);
	}
	p1tfree(a);
	return p;
	
}

static P1ND *
builtin_classify_type(const struct bitable *bt, P1ND *a)
{
	TWORD t = a->n_type;
	int rv;

	if (t == BOOL)
		rv = 4;
	else if (t == CHAR || t == UCHAR)
		rv = 2;
	else if (t <= ULONGLONG)
		rv = 1;
	else if (t == STRTY)
		rv = 12;
	else if (t == UNIONTY)
		rv = 13;
	else if (ISPTR(t))
		rv = 5;
	else if (ISFTY(t))
		rv = 8;
	else if (ISFTN(t))
		rv = 10;
	else if (ISCTY(t))
		rv = 9;
	else
		rv = -1;

	p1tfree(a);
	return bcon(rv);
}


#ifndef TARGET_ISMATH
/*
 * Handle the builtin macros for the math functions is*
 * To get something that is be somewhat generic assume that 
 * isnan() is a real function and that cast of a NaN type 
 * to double will still be a NaN.
 */
static P1ND *
mtisnan(P1ND *p)
{

	return binhelp(cast(ccopy(p), DOUBLE, 0), INT, "isnan");
}

static TWORD
mtcheck(P1ND *p)
{
	TWORD t1 = p->n_left->n_type, t2 = p->n_right->n_type;

	if ((t1 >= FLOAT && t1 <= LDOUBLE) ||
	    (t2 >= FLOAT && t2 <= LDOUBLE))
		return MAX(t1, t2);
	return 0;
}

static P1ND *
builtin_isunordered(const struct bitable *bt, P1ND *a)
{
	P1ND *p;

	if (mtcheck(a) == 0)
		return bcon(0);

	p = buildtree(OROR, mtisnan(a->n_left), mtisnan(a->n_right));
	p1tfree(a);
	return p;
}
static P1ND *
builtin_isany(P1ND *a, TWORD rt, int cmpt)
{
	P1ND *p, *q;
	TWORD t;

	if ((t = mtcheck(a)) == 0)
		return bcon(0);
	p = buildtree(OROR, mtisnan(a->n_left), mtisnan(a->n_right));
	p = buildtree(NOT, p, NULL);
	q = buildtree(cmpt, cast(ccopy(a->n_left), t, 0),
	    cast(ccopy(a->n_right), t, 0));
	p = buildtree(ANDAND, p, q);
	p1tfree(a);
	return p;
}
static P1ND *
builtin_isgreater(const struct bitable *bt, P1ND *a)
{
	return builtin_isany(a, bt->rt, GT);
}
static P1ND *
builtin_isgreaterequal(const struct bitable *bt, P1ND *a)
{
	return builtin_isany(a, bt->rt, GE);
}
static P1ND *
builtin_isless(const struct bitable *bt, P1ND *a)
{
	return builtin_isany(a, bt->rt, LT);
}
static P1ND *
builtin_islessequal(const struct bitable *bt, P1ND *a)
{
	return builtin_isany(a, bt->rt, LE);
}
static P1ND *
builtin_islessgreater(const struct bitable *bt, P1ND *a)
{
	P1ND *p, *q, *r;
	TWORD t;

	if ((t = mtcheck(a)) == 0)
		return bcon(0);
	p = buildtree(OROR, mtisnan(a->n_left), mtisnan(a->n_right));
	p = buildtree(NOT, p, NULL);
	q = buildtree(GT, cast(ccopy(a->n_left), t, 0),
	    cast(ccopy(a->n_right), t, 0));
	r = buildtree(LT, cast(ccopy(a->n_left), t, 0),
	    cast(ccopy(a->n_right), t, 0));
	q = buildtree(OROR, q, r);
	p = buildtree(ANDAND, p, q);
	p1tfree(a);
	return p;
}
#endif

/*
 * Math-specific builtins that expands to constants.
 * Versions here are for IEEE FP, vax needs its own versions.
 */
#if TARGET_ENDIAN == TARGET_LE
static const unsigned char vFLOAT[] = { 0, 0, 0x80, 0x7f };
static const unsigned char vDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
#ifdef LDBL_128
static const unsigned char vLDOUBLE[] = { 0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f };
#else /* LDBL_80 */
static const unsigned char vLDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f,0,0,0,0,0,0 };
#endif
static const unsigned char nFLOAT[] = { 0, 0, 0xc0, 0x7f };
static const unsigned char nDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f };
#ifdef LDBL_128
static const unsigned char nLDOUBLE[] = { 0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0x7f };
#else /* LDBL_80 */
static const unsigned char nLDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0x7f,0,0,0,0,0,0 };
#endif
#else
static const unsigned char vFLOAT[] = { 0x7f, 0x80, 0, 0 };
static const unsigned char vDOUBLE[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
#ifdef LDBL_128
static const unsigned char vLDOUBLE[] = { 0x7f, 0xff, 0x80, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0 };
#else /* LDBL_80 */
static const unsigned char vLDOUBLE[] = { 0x7f, 0xff, 0x80, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0 };
#endif
static const unsigned char nFLOAT[] = { 0x7f, 0xc0, 0, 0 };
static const unsigned char nDOUBLE[] = { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 };
#ifdef LDBL_128
static const unsigned char nLDOUBLE[] = { 0x7f, 0xff, 0xc0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0 };
#else /* LDBL_80 */
static const unsigned char nLDOUBLE[] = { 0x7f, 0xff, 0xc0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0 };
#endif
#endif

#define VALX(typ,TYP) {						\
	typ d;							\
	int x;							\
	P1ND *f;						\
	x = MIN(sizeof(n ## TYP), sizeof(d));			\
	memcpy(&d, v ## TYP, x);				\
	f = block(FCON, NULL, NULL, TYP, NULL, 0);		\
	f->n_dcon = stmtalloc(sizeof(union flt));		\
	f->n_dcon->fp = d;					\
	return f;						\
}

static P1ND *
builtin_huge_valf(const struct bitable *bt, P1ND *a) VALX(float,FLOAT)
static P1ND *
builtin_huge_val(const struct bitable *bt, P1ND *a) VALX(double,DOUBLE)
static P1ND *
builtin_huge_vall(const struct bitable *bt, P1ND *a) VALX(long double,LDOUBLE)

#define	builtin_inff	builtin_huge_valf
#define	builtin_inf	builtin_huge_val
#define	builtin_infl	builtin_huge_vall

/*
 * Return NANs, if reasonable.
 */
static P1ND *
builtin_nanx(const struct bitable *bt, P1ND *a)
{

	if (a == NULL || a->n_op == CM) {
		uerror("%s bad argument", bt->name);
		a = bcon(0);
	} else if (a->n_op == STRING && *a->n_name == '\0') {
		a->n_op = FCON;
		a->n_type = bt->rt;
		if (sizeof(nLDOUBLE) < sizeof(long double))
			cerror("nLDOUBLE too small");
		a->n_dcon = stmtalloc(sizeof(union flt));
		memcpy(&a->n_dcon->fp, nLDOUBLE, sizeof(long double));
	} else
		a = binhelp(eve(a), bt->rt, &bt->name[10]);
	return a;
}

#ifndef NO_COMPLEX
static P1ND *
builtin_cir(const struct bitable *bt, P1ND *a)
{
	char *n;

	if (a == NULL || a->n_op == CM) {
		uerror("wrong argument count to %s", bt->name);
		return bcon(0);
	}

	n = addname(bt->name[1] == 'r' ? "__real" : "__imag");
	return cast(structref(a, DOT, n), bt->rt, 0);
}

#endif

/*
 * Target defines, to implement target versions of the generic builtins
 */
#ifndef TARGET_MEMCMP
#define	builtin_memcmp builtin_unimp
#endif
#ifndef TARGET_MEMCPY
#define	builtin_memcpy builtin_unimp
#endif
#ifndef TARGET_MEMPCPY
#define	builtin_mempcpy builtin_unimp
#endif
#ifndef TARGET_MEMSET
#define	builtin_memset builtin_unimp
#endif

/* Reasonable type of size_t */
#ifndef SIZET
#if SZINT == SZSHORT
#define	SIZET UNSIGNED
#elif SZLONG > SZINT
#define SIZET ULONG
#else
#define SIZET UNSIGNED
#endif
#endif

static TWORD memcpyt[] = { VOID|PTR, VOID|PTR, SIZET, INT };
static TWORD memsett[] = { VOID|PTR, INT, SIZET, INT };
static TWORD allocat[] = { SIZET };
static TWORD expectt[] = { LONG, LONG };
static TWORD strcmpt[] = { CHAR|PTR, CHAR|PTR };
static TWORD strcpyt[] = { CHAR|PTR, CHAR|PTR, INT };
static TWORD strncpyt[] = { CHAR|PTR, CHAR|PTR, SIZET, INT };
static TWORD strchrt[] = { CHAR|PTR, INT };
static TWORD strcspnt[] = { CHAR|PTR, CHAR|PTR };
static TWORD strspnt[] = { CHAR|PTR, CHAR|PTR };
static TWORD strpbrkt[] = { CHAR|PTR, CHAR|PTR };
static TWORD nant[] = { CHAR|PTR };
static TWORD bitt[] = { UNSIGNED };
static TWORD bsw16t[] = { USHORT };
static TWORD bitlt[] = { ULONG };
static TWORD bitllt[] = { ULONGLONG };
static TWORD abst[] = { INT };
static TWORD fmaxft[] = { FLOAT, FLOAT };
static TWORD fmaxt[] = { DOUBLE, DOUBLE };
static TWORD fmaxlt[] = { LDOUBLE, LDOUBLE };
static TWORD scalbnft[] = { FLOAT, INT };
static TWORD scalbnt[] = { DOUBLE, INT };
static TWORD scalbnlt[] = { LDOUBLE, INT };

static const struct bitable bitable[] = {
	/* gnu universe only */
	{ "alloca", builtin_alloca, BTGNUONLY, 1, allocat, VOID|PTR },

#ifndef NO_COMPLEX
	/* builtins for complex operations */
	{ "crealf", builtin_cir, BTNOPROTO, 1, 0, FLOAT },
	{ "creal", builtin_cir, BTNOPROTO, 1, 0, DOUBLE },
	{ "creall", builtin_cir, BTNOPROTO, 1, 0, LDOUBLE },
	{ "cimagf", builtin_cir, BTNOPROTO, 1, 0, FLOAT },
	{ "cimag", builtin_cir, BTNOPROTO, 1, 0, DOUBLE },
	{ "cimagl", builtin_cir, BTNOPROTO, 1, 0, LDOUBLE },
#endif
	/* always existing builtins */
	{ "__builtin___memcpy_chk", builtin_unimp, 0, 4, memcpyt, VOID|PTR },
	{ "__builtin___mempcpy_chk", builtin_unimp, 0, 4, memcpyt, VOID|PTR },
	{ "__builtin___memmove_chk", builtin_unimp, 0, 4, memcpyt, VOID|PTR },
	{ "__builtin___memset_chk", builtin_unimp, 0, 4, memsett, VOID|PTR },

	{ "__builtin___strcat_chk", builtin_unimp, 0, 3, strcpyt, CHAR|PTR },
	{ "__builtin___strcpy_chk", builtin_unimp, 0, 3, strcpyt, CHAR|PTR },
	{ "__builtin___stpcpy_chk", builtin_unimp, 0, 3, strcpyt, CHAR|PTR },
	{ "__builtin___strncat_chk", builtin_unimp, 0, 4, strncpyt,CHAR|PTR },
	{ "__builtin___strncpy_chk", builtin_unimp, 0, 4, strncpyt,CHAR|PTR },
	{ "__builtin___stpncpy_chk", builtin_unimp, 0, 4, strncpyt,CHAR|PTR },

	{ "__builtin___printf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___fprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___sprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___snprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___vprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___vfprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___vsprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },
	{ "__builtin___vsnprintf_chk", builtin_unimp, BTNOPROTO, -1, 0, INT },

	{ "__builtin_alloca", builtin_alloca, 0, 1, allocat, VOID|PTR },
	{ "__builtin_abs", builtin_abs, 0, 1, abst, INT },
	{ "__builtin_bswap16", builtin_bswap16, 0, 1, bsw16t, USHORT },
	{ "__builtin_bswap32", builtin_bswap32, 0, 1, bitt, UNSIGNED },
	{ "__builtin_bswap64", builtin_bswap64, 0, 1, bitllt, ULONGLONG },
	{ "__builtin_choose_expr", builtin_ce, BTNOPROTO|BTNORVAL, 0, 0, 0 },
	{ "__builtin_clz", builtin_clz, 0, 1, bitt, INT },
	{ "__builtin_clzl", builtin_clzl, 0, 1, bitlt, INT },
	{ "__builtin_clzll", builtin_clzll, 0, 1, bitllt, INT },
	{ "__builtin_ctz", builtin_ctz, 0, 1, bitt, INT },
	{ "__builtin_ctzl", builtin_ctzl, 0, 1, bitlt, INT },
	{ "__builtin_ctzll", builtin_ctzll, 0, 1, bitllt, INT },
	{ "__builtin_extract_return_addr", builtin_era, 0, 1, memcpyt, VOID|PTR },
	{ "__builtin_ffs", builtin_ffs, 0, 1, bitt, INT },
	{ "__builtin_ffsl", builtin_ffsl, 0, 1, bitlt, INT },
	{ "__builtin_ffsll", builtin_ffsll, 0, 1, bitllt, INT },
	{ "__builtin_popcount", builtin_unimp, 0, 1, bitt, UNSIGNED },
	{ "__builtin_popcountl", builtin_unimp, 0, 1, bitlt, ULONG },
	{ "__builtin_popcountll", builtin_unimp, 0, 1, bitllt, ULONGLONG },

	{ "__builtin_classify_type", builtin_classify_type, 0, 1, 0, INT },
	{ "__builtin_constant_p", builtin_constant_p, 0, 1, 0, INT },
	{ "__builtin_copysignf", builtin_unimp, 0, 2, fmaxft, FLOAT },
	{ "__builtin_copysign", builtin_unimp, 0, 2, fmaxt, DOUBLE },
	{ "__builtin_copysignl", builtin_unimp, 0, 2, fmaxlt, LDOUBLE },
	{ "__builtin_expect", builtin_expect, 0, 2, expectt, LONG },
	{ "__builtin_memcmp", builtin_memcmp, 0, 3, memcpyt, INT },
	{ "__builtin_memcpy", builtin_memcpy, 0, 3, memcpyt, VOID|PTR },
	{ "__builtin_mempcpy", builtin_mempcpy, 0, 3, memcpyt, VOID|PTR },
	{ "__builtin_memset", builtin_memset, 0, 3, memsett, VOID|PTR },
	{ "__builtin_fabsf", builtin_unimp, 0, 1, fmaxft, FLOAT },
	{ "__builtin_fabs", builtin_unimp, 0, 1, fmaxt, DOUBLE },
	{ "__builtin_fabsl", builtin_unimp, 0, 1, fmaxlt, LDOUBLE },
	{ "__builtin_fmaxf", builtin_unimp, 0, 2, fmaxft, FLOAT },
	{ "__builtin_fmax", builtin_unimp, 0, 2, fmaxt, DOUBLE },
	{ "__builtin_fmaxl", builtin_unimp, 0, 2, fmaxlt, LDOUBLE },
	{ "__builtin_huge_valf", builtin_huge_valf, 0, 0, 0, FLOAT },
	{ "__builtin_huge_val", builtin_huge_val, 0, 0, 0, DOUBLE },
	{ "__builtin_huge_vall", builtin_huge_vall, 0, 0, 0, LDOUBLE },
	{ "__builtin_inff", builtin_inff, 0, 0, 0, FLOAT },
	{ "__builtin_inf", builtin_inf, 0, 0, 0, DOUBLE },
	{ "__builtin_infl", builtin_infl, 0, 0, 0, LDOUBLE },
	{ "__builtin_isgreater", builtin_isgreater, 0, 2, NULL, INT },
	{ "__builtin_isgreaterequal", builtin_isgreaterequal, 0, 2, NULL, INT },
	{ "__builtin_isinff", builtin_unimp, 0, 1, fmaxft, INT },
	{ "__builtin_isinf", builtin_unimp, 0, 1, fmaxt, INT },
	{ "__builtin_isinfl", builtin_unimp, 0, 1, fmaxlt, INT },
	{ "__builtin_isless", builtin_isless, 0, 2, NULL, INT },
	{ "__builtin_islessequal", builtin_islessequal, 0, 2, NULL, INT },
	{ "__builtin_islessgreater", builtin_islessgreater, 0, 2, NULL, INT },
	{ "__builtin_isnanf", builtin_unimp, 0, 1, fmaxft, INT },
	{ "__builtin_isnan", builtin_unimp, 0, 1, fmaxt, INT },
	{ "__builtin_isnanl", builtin_unimp, 0, 1, fmaxlt, INT },
	{ "__builtin_isunordered", builtin_isunordered, 0, 2, NULL, INT },
	{ "__builtin_logbf", builtin_unimp, 0, 1, fmaxft, FLOAT },
	{ "__builtin_logb", builtin_unimp, 0, 1, fmaxt, DOUBLE },
	{ "__builtin_logbl", builtin_unimp, 0, 1, fmaxlt, LDOUBLE },
	{ "__builtin_nanf", builtin_nanx, BTNOEVE, 1, nant, FLOAT },
	{ "__builtin_nan", builtin_nanx, BTNOEVE, 1, nant, DOUBLE },
	{ "__builtin_nanl", builtin_nanx, BTNOEVE, 1, nant, LDOUBLE },
	{ "__builtin_object_size", builtin_object_size, BTNOPROTO, 2, memsett, SIZET },
	{ "__builtin_prefetch", builtin_prefetch, 0, 1, memsett, VOID },
	{ "__builtin_scalbnf", builtin_unimp, 0, 2, scalbnft, FLOAT },
	{ "__builtin_scalbn", builtin_unimp, 0, 2, scalbnt, DOUBLE },
	{ "__builtin_scalbnl", builtin_unimp, 0, 2, scalbnlt, LDOUBLE },
	{ "__builtin_strcmp", builtin_unimp, 0, 2, strcmpt, INT },
	{ "__builtin_strcpy", builtin_unimp, 0, 2, strcpyt, CHAR|PTR },
	{ "__builtin_stpcpy", builtin_unimp, 0, 2, strcpyt, CHAR|PTR },
	{ "__builtin_strchr", builtin_unimp, 0, 2, strchrt, CHAR|PTR },
	{ "__builtin_strlen", builtin_unimp, 0, 1, strcmpt, SIZET },
	{ "__builtin_strrchr", builtin_unimp, 0, 2, strchrt, CHAR|PTR },
	{ "__builtin_strncpy", builtin_unimp, 0, 3, strncpyt, CHAR|PTR },
	{ "__builtin_strncat", builtin_unimp, 0, 3, strncpyt, CHAR|PTR },
	{ "__builtin_strcspn", builtin_unimp, 0, 2, strcspnt, SIZET },
	{ "__builtin_strspn", builtin_unimp, 0, 2, strspnt, SIZET },
	{ "__builtin_strstr", builtin_unimp, 0, 2, strcmpt, CHAR|PTR },
	{ "__builtin_strpbrk", builtin_unimp, 0, 2, strpbrkt, CHAR|PTR },
	{ "__builtin_types_compatible_p", builtin_tc, BTNOPROTO, 2, 0, INT },
#ifndef TARGET_STDARGS
	{ "__builtin_stdarg_start", builtin_stdarg_start, 0, 2, 0, VOID },
	{ "__builtin_va_start", builtin_stdarg_start, 0, 2, 0, VOID },
	{ "__builtin_va_arg", builtin_va_arg, BTNORVAL|BTNOPROTO, 2, 0, 0 },
	{ "__builtin_va_end", builtin_va_end, 0, 1, 0, VOID },
	{ "__builtin_va_copy", builtin_va_copy, 0, 2, 0, VOID },
#endif
	{ "__builtin_dwarf_cfa", builtin_cfa, 0, 0, 0, VOID|PTR },
	{ "__builtin_frame_address",
	    builtin_frame_address, 0, 1, bitt, VOID|PTR },
	{ "__builtin_return_address",
	    builtin_return_address, 0, 1, bitt, VOID|PTR },
#ifdef TARGET_BUILTINS
	TARGET_BUILTINS
#endif
};

/*
 * Check and cast arguments for builtins.
 */
static int
acnt(P1ND *a, int narg, TWORD *tp)
{
	P1ND *q;
	TWORD t;

	if (a == NULL)
		return narg;
	for (; a->n_op == CM; a = a->n_left, narg--) {
		if (tp == NULL)
			continue;
		q = a->n_right;
		t = ctype(tp[narg-1]);
		if (q->n_type == t)
			continue;
		a->n_right = ccast(q, t, 0, NULL, 0);
	}

	/* Last arg is ugly to deal with */
	if (narg == 1 && tp != NULL && a->n_type != tp[0]) {
		q = p1alloc();
		*q = *a;
		q = ccast(q, ctype(tp[0]), 0, NULL, 0);
		*a = *q;
		p1nfree(q);
	}
	return narg != 1;
}

P1ND *
builtin_check(struct symtab *sp, P1ND *a)
{
	const struct bitable *bt;

	if (sp->soffset < 0 ||
	    sp->soffset >= (int)(sizeof(bitable)/sizeof(bitable[0])))
		cerror("builtin_check");

	bt = &bitable[sp->soffset];
	if ((bt->flags & BTNOEVE) == 0 && a != NULL)
		a = eve(a);
	if (((bt->flags & BTNOPROTO) == 0) && acnt(a, bt->narg, bt->tp)) {
		uerror("wrong argument count to %s", bt->name);
		return bcon(0);
	}
	return (*bt->fun)(bt, a);
}

/*
 * Put all builtin functions into the global symbol table.
 */
void
builtin_init()
{
	const struct bitable *bt;
	P1ND *p = block(TYPE, 0, 0, 0, 0, 0);
	struct symtab *sp;
	int i, d_debug;

	d_debug = ddebug;
	ddebug = 0;
	for (i = 0; i < (int)(sizeof(bitable)/sizeof(bitable[0])); i++) {
		bt = &bitable[i];
		if ((bt->flags & BTGNUONLY) && xgnu99 == 0 && xgnu89 == 0)
			continue; /* not in c99 universe, at least for now */
		sp = lookup(addname(bt->name), 0);
		if (bt->rt == 0 && (bt->flags & BTNORVAL) == 0)
			cerror("function '%s' has no return type", bt->name);
		p->n_type = INCREF(bt->rt) + (FTN-PTR);
		p->n_df = memset(permalloc(sizeof(union dimfun)), 0,
		    sizeof(union dimfun));
		dimfuncnt++;
		p->n_sp = sp;
		defid(p, EXTERN);
		sp->soffset = i;
		sp->sflags |= SBUILTIN;
	}
	p1nfree(p);
	ddebug = d_debug;
}
#endif
