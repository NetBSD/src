/*	$Id: code.c,v 1.1.1.1 2009/09/04 00:27:30 gmcgarry Exp $	*/
/*
 * Copyright (c) 2008 Michael Shalayeff
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

static int nsse, ngpr, nrsp, rsaoff;
static int thissse, thisgpr, thisrsp;
enum { INTEGER = 1, INTMEM, SSE, SSEMEM, X87, STRREG, STRMEM };
static const int argregsi[] = { RDI, RSI, RDX, RCX, R08, R09 };
/*
 * The Register Save Area looks something like this.
 * It is put first on stack with fixed offsets.
 * struct {
 *	long regs[6];
 *	double xmm[8][2]; // 16 byte in width
 * };
 */
#define	RSASZ		(6*SZLONG+8*2*SZDOUBLE)
#define	RSALONGOFF(x)	(RSASZ-(x)*SZLONG)
#define	RSADBLOFF(x)	((8*2*SZDOUBLE)-(x)*SZDOUBLE*2)
/* va_list */
#define	VAARGSZ		(SZINT*2+SZPOINT(CHAR)*2)
#define	VAGPOFF(x)	(x)
#define	VAFPOFF(x)	(x-SZINT)
#define	VAOFA(x)	(x-SZINT-SZINT)
#define	VARSA(x)	(x-SZINT-SZINT-SZPOINT(0))

int lastloc = -1;

static int argtyp(TWORD t, union dimfun *df, struct suedef *sue);
static NODE *movtomem(NODE *p, int off, int reg);

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	extern char *nextsect;
	static char *loctbl[] = { "text", "data", "section .rodata" };
	int weak = 0;
	char *name = NULL;
	TWORD t;
	int s;

	if (sp == NULL) {
		lastloc = -1;
		return;
	}
	t = sp->stype;
	s = ISFTN(t) ? PROG : ISCON(cqual(t, sp->squal)) ? RDATA : DATA;
#ifdef TLS
	if (sp->sflags & STLS) {
		if (s != DATA)
			cerror("non-data symbol in tls section");
		nextsect = ".tdata";
	}
#endif
#ifdef GCC_COMPAT
	{
		struct gcc_attrib *ga;

		if ((ga = gcc_get_attr(sp->ssue, GCC_ATYP_SECTION)) != NULL)
			nextsect = ga->a1.sarg;
		if ((ga = gcc_get_attr(sp->ssue, GCC_ATYP_WEAK)) != NULL)
			weak = 1;
	}
#endif

	if (nextsect) {
		printf("	.section %s\n", nextsect);
		nextsect = NULL;
		s = -1;
	} else if (s != lastloc)
		printf("	.%s\n", loctbl[s]);
	lastloc = s;
	while (ISARY(t))
		t = DECREF(t);
	s = ISFTN(t) ? ALINT : talign(t, sp->ssue);
	if (s > ALCHAR)
		printf("	.align %d\n", s/ALCHAR);
	if (weak || sp->sclass == EXTDEF || sp->slevel == 0 || ISFTN(t))
		if ((name = sp->soname) == NULL)
			name = exname(sp->sname);
	if (weak)
		printf("        .weak %s\n", name);
	else if (sp->sclass == EXTDEF)
		printf("	.globl %s\n", name);
	if (ISFTN(t))
		printf("\t.type %s,@function\n", name);
	if (sp->slevel == 0)
		printf("%s:\n", name);
	else
		printf(LABFMT ":\n", sp->soffset);
}

/*
 * code for the end of a function
 * deals with struct return here
 */
void
efcode()
{
	extern int gotnr;
	NODE *p, *q;

	gotnr = 0;	/* new number for next fun */
	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;
	/* Create struct assignment */
	q = block(OREG, NIL, NIL, PTR+STRTY, 0, cftnsp->ssue);
	q->n_rval = RBP;
	q->n_lval = 8; /* return buffer offset */
	q = buildtree(UMUL, q, NIL);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->ssue);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **s, int cnt)
{
	union arglist *al;
	struct symtab *sp;
	NODE *p, *r;
	int i, rno, typ;

	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		/* Function returns struct, adjust arg offset */
		for (i = 0; i < cnt; i++) 
			s[i]->soffset += SZPOINT(LONG);
	}

	/* recalculate the arg offset and create TEMP moves */
	/* Always do this for reg, even if not optimizing, to free arg regs */
	nsse = ngpr = 0;
	nrsp = ARGINIT;
	for (i = 0; i < cnt; i++) {
		sp = s[i];

		if (sp == NULL)
			continue; /* XXX when happens this? */

		switch (typ = argtyp(sp->stype, sp->sdf, sp->ssue)) {
		case INTEGER:
		case SSE:
			if (typ == SSE)
				rno = XMM0 + nsse++;
			else
				rno = argregsi[ngpr++];
			r = block(REG, NIL, NIL, sp->stype, sp->sdf, sp->ssue);
			regno(r) = rno;
			p = tempnode(0, sp->stype, sp->sdf, sp->ssue);
			sp->soffset = regno(p);
			sp->sflags |= STNODE;
			ecomp(buildtree(ASSIGN, p, r));
			break;

		case INTMEM:
			sp->soffset = nrsp;
			nrsp += SZLONG;
			if (xtemps) {
				p = tempnode(0, sp->stype, sp->sdf, sp->ssue);
				p = buildtree(ASSIGN, p, nametree(sp));
				sp->soffset = regno(p->n_left);
				sp->sflags |= STNODE;
				ecomp(p);
			}
			break;

		default:
			cerror("bfcode: %d", typ);
		}
	}

	/* Check if there are varargs */
	if (cftnsp->sdf == NULL || cftnsp->sdf->dfun == NULL)
		return; /* no prototype */
	al = cftnsp->sdf->dfun;

	for (; al->type != TELLIPSIS; al++) {
		if (al->type == TNULL)
			return;
		if (BTYPE(al->type) == STRTY || BTYPE(al->type) == UNIONTY ||
		    ISARY(al->type))
			al++;
	}

	/* fix stack offset */
	SETOFF(autooff, ALMAX);

	/* Save reg arguments in the reg save area */
	p = NIL;
	for (i = ngpr; i < 6; i++) {
		r = block(REG, NIL, NIL, LONG, 0, MKSUE(LONG));
		regno(r) = argregsi[i];
		r = movtomem(r, -RSALONGOFF(i)-autooff, FPREG);
		p = (p == NIL ? r : block(COMOP, p, r, INT, 0, MKSUE(INT)));
	}
	for (i = nsse; i < 8; i++) {
		r = block(REG, NIL, NIL, DOUBLE, 0, MKSUE(DOUBLE));
		regno(r) = i + XMM0;
		r = movtomem(r, -RSADBLOFF(i)-autooff, FPREG);
		p = (p == NIL ? r : block(COMOP, p, r, INT, 0, MKSUE(INT)));
	}
	autooff += RSASZ;
	rsaoff = autooff;
	thissse = nsse;
	thisgpr = ngpr;
	thisrsp = nrsp;

	ecomp(p);
}


/*
 * by now, the automatics and register variables are allocated
 */
void
bccode()
{
	SETOFF(autooff, SZINT);
}

/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag )
{
#define _MKSTR(x) #x
#define MKSTR(x) _MKSTR(x)
#define OS MKSTR(TARGOS)
        printf("\t.ident \"PCC: %s (%s)\"\n\t.end\n", PACKAGE_STRING, OS);
}

/*
 * Varargs stuff:
 * The ABI says that va_list should be declared as this typedef.
 * We handcraft it here and then just reference it.
 *
 * typedef struct {
 *	unsigned int gp_offset;
 *	unsigned int fp_offset;
 *	void *overflow_arg_area;
 *	void *reg_save_area;
 * } __builtin_va_list[1];
 */

static char *gp_offset, *fp_offset, *overflow_arg_area, *reg_save_area;

void
bjobcode()
{
	struct rstack *rp;
	NODE *p, *q;
	char *c;

	gp_offset = addname("gp_offset");
	fp_offset = addname("fp_offset");
	overflow_arg_area = addname("overflow_arg_area");
	reg_save_area = addname("reg_save_area");

	rp = bstruct(NULL, STNAME, NULL);
	p = block(NAME, NIL, NIL, UNSIGNED, 0, MKSUE(UNSIGNED));
	soumemb(p, gp_offset, 0);
	soumemb(p, fp_offset, 0);
	p->n_type = VOID+PTR;
	p->n_sue = MKSUE(VOID);
	soumemb(p, overflow_arg_area, 0);
	soumemb(p, reg_save_area, 0);
	nfree(p);
	q = dclstruct(rp);
	c = addname("__builtin_va_list");
	p = block(LB, bdty(NAME, c), bcon(1), INT, 0, MKSUE(INT));
	p = tymerge(q, p);
	p->n_sp = lookup(c, 0);
	defid(p, TYPEDEF);
	nfree(q);
	nfree(p);
}

static NODE *
mkstkref(int off, TWORD typ)
{
	NODE *p;

	p = block(REG, NIL, NIL, PTR|typ, 0, MKSUE(LONG));
	regno(p) = FPREG;
	return buildtree(PLUS, p, bcon(off/SZCHAR));
}

NODE *
amd64_builtin_stdarg_start(NODE *f, NODE *a)
{
	NODE *p, *r;

	/* check num args and type */
	if (a == NULL || a->n_op != CM || a->n_left->n_op == CM ||
	    !ISPTR(a->n_left->n_type))
		goto bad;

	/* use the values from the function header */
	p = a->n_left;
	r = buildtree(ASSIGN, structref(ccopy(p), STREF, reg_save_area),
	    mkstkref(-rsaoff, VOID));
	r = buildtree(COMOP, r,
	    buildtree(ASSIGN, structref(ccopy(p), STREF, overflow_arg_area),
	    mkstkref(thisrsp, VOID)));
	r = buildtree(COMOP, r,
	    buildtree(ASSIGN, structref(ccopy(p), STREF, gp_offset),
	    bcon(thisgpr*(SZLONG/SZCHAR))));
	r = buildtree(COMOP, r,
	    buildtree(ASSIGN, structref(ccopy(p), STREF, fp_offset),
	    bcon(thissse*(SZDOUBLE*2/SZCHAR)+48)));

	tfree(f);
	tfree(a);
	return r;
bad:
	uerror("bad argument to __builtin_stdarg_start");
	return bcon(0);
}

/*
 * Create a tree that should be like the expression
 *	((long *)(l->gp_offset >= 48 ?
 *	    l->overflow_arg_area += 8, l->overflow_arg_area :
 *	    l->gp_offset += 8, l->reg_save_area + l->gp_offset))[-1]
 * ...or similar for floats.
 */
static NODE *
bva(NODE *ap, TWORD dt, char *ot, int addto, int max)
{
	NODE *cm1, *cm2, *gpo, *ofa, *l1, *qc;
	TWORD nt;

	ofa = structref(ccopy(ap), STREF, overflow_arg_area);
	l1 = buildtree(PLUSEQ, ccopy(ofa), bcon(addto));
	cm1 = buildtree(COMOP, l1, ofa);

	gpo = structref(ccopy(ap), STREF, ot);
	l1 = buildtree(PLUSEQ, ccopy(gpo), bcon(addto));
	cm2 = buildtree(COMOP, l1, buildtree(PLUS, ccopy(gpo),
	    structref(ccopy(ap), STREF, reg_save_area)));
	qc = buildtree(QUEST,
	    buildtree(GE, gpo, bcon(max)),
	    buildtree(COLON, cm1, cm2));

	nt = (dt == DOUBLE ? DOUBLE : LONG);
	l1 = block(NAME, NIL, NIL, nt|PTR, 0, MKSUE(nt));
	l1 = buildtree(CAST, l1, qc);
	qc = l1->n_right;
	nfree(l1->n_left);
	nfree(l1);

	/* qc has now a real type, for indexing */
	addto = dt == DOUBLE ? 2 : 1;
	qc = buildtree(UMUL, buildtree(PLUS, qc, bcon(-addto)), NIL);

	l1 = block(NAME, NIL, NIL, dt, 0, MKSUE(BTYPE(dt)));
	l1 = buildtree(CAST, l1, qc);
	qc = l1->n_right;
	nfree(l1->n_left);
	nfree(l1);

	return qc;
}

NODE *
amd64_builtin_va_arg(NODE *f, NODE *a)
{
	NODE *ap, *r;
	TWORD dt;

	/* check num args and type */
	if (a == NULL || a->n_op != CM || a->n_left->n_op == CM ||
	    !ISPTR(a->n_left->n_type) || a->n_right->n_op != TYPE)
		goto bad;

	ap = a->n_left;
	dt = a->n_right->n_type;
	if (dt <= ULONGLONG || ISPTR(dt)) {
		/* type might be in general register */
		r = bva(ap, dt, gp_offset, 8, 48);
	} else if (dt == FLOAT || dt == DOUBLE) {
		/* Float are promoted to double here */
		if (dt == FLOAT)
			dt = DOUBLE;
		r = bva(ap, dt, fp_offset, 16, RSASZ/SZCHAR);
	} else {
		uerror("amd64_builtin_va_arg not supported type");
		goto bad;
	}
	tfree(a);
	tfree(f);
	return r;
bad:
	uerror("bad argument to __builtin_va_arg");
	return bcon(0);
}

NODE *
amd64_builtin_va_end(NODE *f, NODE *a)
{
	tfree(f);
	tfree(a);
	return bcon(0); /* nothing */
}

NODE *
amd64_builtin_va_copy(NODE *f, NODE *a) { cerror("amd64_builtin_va_copy"); return NULL; }

static NODE *
movtoreg(NODE *p, int rno)
{
	NODE *r;

	r = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_sue);
	regno(r) = rno;
	return clocal(buildtree(ASSIGN, r, p));
}  

static NODE *
movtomem(NODE *p, int off, int reg)
{
	struct symtab s;
	NODE *r, *l;

	s.stype = p->n_type;
	s.sdf = p->n_df;
	s.ssue = p->n_sue;
	s.soffset = off;
	s.sclass = AUTO;

	l = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
	l->n_lval = 0;
	regno(l) = reg;

	r = block(NAME, NIL, NIL, p->n_type, p->n_df, p->n_sue);
	r->n_sp = &s;
	r = stref(block(STREF, l, r, 0, 0, 0));

	return clocal(buildtree(ASSIGN, r, p));
}  


/*
 * AMD64 parameter classification.
 */
static int
argtyp(TWORD t, union dimfun *df, struct suedef *sue)
{
	int cl = 0;

	if (t <= ULONG || ISPTR(t)) {
		cl = ngpr < 6 ? INTEGER : INTMEM;
	} else if (t == FLOAT || t == DOUBLE) {
		cl = nsse < 8 ? SSE : SSEMEM;
	} else if (t == LDOUBLE) {
		cl = X87; /* XXX */
	} else if (t == STRTY) {
		if (tsize(t, df, sue) > 4*SZLONG)
			cl = STRMEM;
		else
			cerror("clasif");
	} else
		cerror("FIXME: classify");
	return cl;
}

static void
argput(NODE *p)
{
	NODE *q;
	int typ, r;

	/* first arg may be struct return pointer */
	/* XXX - check if varargs; setup al */
	switch (typ = argtyp(p->n_type, p->n_df, p->n_sue)) {
	case INTEGER:
	case SSE:
		q = talloc();
		*q = *p;
		if (typ == SSE)
			r = XMM0 + nsse++;
		else
			r = argregsi[ngpr++];
		q = movtoreg(q, r);
		*p = *q;
		nfree(q);
		break;
	case X87:
		cerror("no long double yet");
		break;

	case INTMEM:
		q = talloc();
		*q = *p;
		r = nrsp;
		nrsp += SZLONG;
		q = movtomem(q, r, STKREG);
		*p = *q;
		nfree(q);
		break;

	case STRMEM:
		/* Struct moved to memory */
	case STRREG:
		/* Struct in registers */
	default:
		cerror("argument %d", typ);
	}
}


/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */
NODE *
funcode(NODE *p)
{
	NODE *l, *r;

	nsse = ngpr = nrsp = 0;
	listf(p->n_right, argput);

	/* Always emit number of SSE regs used */
	l = movtoreg(bcon(nsse), RAX);
	if (p->n_right->n_op != CM) {
		p->n_right = block(CM, l, p->n_right, INT, 0, MKSUE(INT));
	} else {
		for (r = p->n_right; r->n_left->n_op == CM; r = r->n_left)
			;
		r->n_left = block(CM, l, r->n_left, INT, 0, MKSUE(INT));
	}
	return p;
}

/*
 * return the alignment of field of type t
 */
int
fldal(unsigned int t)
{
	uerror("illegal field type");
	return(ALINT);
}

/* fix up type of field p */
void
fldty(struct symtab *p)
{
}

/*
 * XXX - fix genswitch.
 */
int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}
