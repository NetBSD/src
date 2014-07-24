/*      Id: code.c,v 1.28 2014/04/19 07:47:51 ragge Exp     */	
/*      $NetBSD: code.c,v 1.1.1.6 2014/07/24 19:16:08 plunky Exp $    */
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

/*
 *  Stuff for pass1.
 */

#include <assert.h>

#include "pass1.h"
#include "pass2.h"

static int rvnr;

/*
 * Print out assembler segment name.
 */
void
setseg(int seg, char *name)
{
	switch (seg) {
	case PROG: name = ".text"; break;
	case DATA:
	case LDATA: name = ".data"; break;
	case UDATA: break;
	case PICLDATA: name = ".section .data.rel.local,\"aw\",@progbits";break;
	case PICDATA: name = ".section .data.rel.rw,\"aw\",@progbits"; break;
	case PICRDATA: name = ".section .data.rel.ro,\"aw\",@progbits"; break;
	case STRNG:
	case RDATA: name = ".section .rodata"; break;
	case TLSDATA: name = ".section .tdata,\"awT\",@progbits"; break;
	case TLSUDATA: name = ".section .tbss,\"awT\",@nobits"; break;
	case CTORS: name = ".section\t.ctors,\"aw\",@progbits"; break;
	case DTORS: name = ".section\t.dtors,\"aw\",@progbits"; break;
	case NMSEG: 
		printf("\t.section %s,\"a%c\",@progbits\n", name,
		    cftnsp ? 'x' : 'w');
		return;
	}
	printf("\t%s\n", name);
}

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	char *n;

	n = sp->soname ? sp->soname : exname(sp->sname);
#ifdef USE_GAS
	if (ISFTN(t))
		printf("\t.type %s,%%function\n", n);
#endif
	if (sp->sclass == EXTDEF)
		printf("\t.global %s\n", n);
	if (sp->slevel == 0)
		printf("%s:\n", n);
	else
		printf(LABFMT ":\n", sp->soffset);
}

/* Put a symbol in a temporary
 * used by bfcode() and its helpers
 */
static void
putintemp(struct symtab *sym)
{
	NODE *p;

	p = tempnode(0, sym->stype, sym->sdf, sym->sap);
	p = buildtree(ASSIGN, p, nametree(sym));
	sym->soffset = regno(p->n_left);
	sym->sflags |= STNODE;
	ecomp(p);
}

/* setup a 64-bit parameter (double/ldouble/longlong)
 * used by bfcode() */
static void
param_64bit(struct symtab *sym, int *argofsp, int dotemps)
{
	int argofs = *argofsp;
	NODE *p, *q;
	int navail;

#if ALLONGLONG == 64
	/* alignment */
	++argofs;
	argofs &= ~1;
	*argofsp = argofs;
#endif

	navail = NARGREGS - argofs;

	if (navail < 2) {
		/* half in and half out of the registers */
		if (features(FEATURE_BIGENDIAN)) {
			cerror("param_64bit");
			p = q = NULL;
		} else {
			q = block(REG, NIL, NIL, INT, 0, 0);
			regno(q) = R0 + argofs;
			if (dotemps) {
				q = block(SCONV, q, NIL,
				    ULONGLONG, 0, 0);
				p = nametree(sym);
				p->n_type = ULONGLONG;
				p->n_df = 0;
				p->n_ap = NULL;
				p = block(LS, p, bcon(32), ULONGLONG, 0, 0);
				q = block(PLUS, p, q, ULONGLONG, 0, 0);
				p = tempnode(0, ULONGLONG, 0, 0);
				sym->soffset = regno(p);
				sym->sflags |= STNODE;
			} else {
				p = nametree(sym);
				regno(p) = sym->soffset;
				p->n_type = INT;
				p->n_df = 0;
				p->n_ap = NULL;
			}
		}
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
		*argofsp = argofs + 2;
		return;
	}

	q = block(REG, NIL, NIL, sym->stype, sym->sdf, sym->sap);
	regno(q) = R0R1 + argofs;
	if (dotemps) {
		p = tempnode(0, sym->stype, sym->sdf, sym->sap);
		sym->soffset = regno(p);
		sym->sflags |= STNODE;
	} else {
		p = nametree(sym);
	}
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
	*argofsp = argofs + 2;
}

/* setup a 32-bit param on the stack
 * used by bfcode() */
static void
param_32bit(struct symtab *sym, int *argofsp, int dotemps)
{
	NODE *p, *q;

	q = block(REG, NIL, NIL, sym->stype, sym->sdf, sym->sap);
	regno(q) = R0 + (*argofsp)++;
	if (dotemps) {
		p = tempnode(0, sym->stype, sym->sdf, sym->sap);
		sym->soffset = regno(p);
		sym->sflags |= STNODE;
	} else {
		p = nametree(sym);
	}
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
}

/* setup a double param on the stack
 * used by bfcode() */
static void
param_double(struct symtab *sym, int *argofsp, int dotemps)
{
	NODE *p, *q, *t;
	int tmpnr;

	/*
	 * we have to dump the float from the general register
	 * into a temp, since the register allocator doesn't like
	 * floats to be in CLASSA.  This may not work for -xtemps.
	 */

	t = tempnode(0, ULONGLONG, 0, 0);
	tmpnr = regno(t);
	q = block(REG, NIL, NIL, INT, 0, 0);
	q->n_rval = R0R1 + (*argofsp)++;
	p = buildtree(ASSIGN, t, q);
	ecomp(p);

	if (dotemps) {
		sym->soffset = tmpnr;
		sym->sflags |= STNODE;
	} else {
		q = tempnode(tmpnr, sym->stype, sym->sdf, sym->sap);
		p = nametree(sym);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	}
}

/* setup a float param on the stack
 * used by bfcode() */
static void
param_float(struct symtab *sym, int *argofsp, int dotemps)
{
	NODE *p, *q, *t;
	int tmpnr;

	/*
	 * we have to dump the float from the general register
	 * into a temp, since the register allocator doesn't like
	 * floats to be in CLASSA.  This may not work for -xtemps.
	 */

	t = tempnode(0, INT, 0, 0);
	tmpnr = regno(t);
	q = block(REG, NIL, NIL, INT, 0, 0);
	q->n_rval = R0 + (*argofsp)++;
	p = buildtree(ASSIGN, t, q);
	ecomp(p);

	if (dotemps) {
		sym->soffset = tmpnr;
		sym->sflags |= STNODE;
	} else {
		q = tempnode(tmpnr, sym->stype, sym->sdf, sym->sap);
		p = nametree(sym);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	}
}

/* setup the hidden pointer to struct return parameter
 * used by bfcode() */
static void
param_retstruct(void)
{
	NODE *p, *q;

	p = tempnode(0, PTR-FTN+cftnsp->stype, 0, cftnsp->sap);
	rvnr = regno(p);
	q = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	regno(q) = R0;
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
}


/* setup struct parameter
 * push the registers out to memory
 * used by bfcode() */
static void
param_struct(struct symtab *sym, int *argofsp)
{
	int argofs = *argofsp;
	NODE *p, *q;
	int navail;
	int sz;
	int off;
	int num;
	int i;

	navail = NARGREGS - argofs;
	sz = tsize(sym->stype, sym->sdf, sym->sap) / SZINT;
	off = ARGINIT/SZINT + argofs;
	num = sz > navail ? navail : sz;
	for (i = 0; i < num; i++) {
		q = block(REG, NIL, NIL, INT, 0, 0);
		regno(q) = R0 + argofs++;
		p = block(REG, NIL, NIL, INT, 0, 0);
		regno(p) = SP;
		p = block(PLUS, p, bcon(4*off++), INT, 0, 0);
		p = block(UMUL, p, NIL, INT, 0, 0);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	}

	*argofsp = argofs;
}


/*
 * Beginning-of-function code:
 *
 * 'sp' is an array of indices in symtab for the arguments
 * 'cnt' is the number of arguments
 */
void
bfcode(struct symtab **sp, int cnt)
{
	union arglist *usym;
	int saveallargs = 0;
	int i, argofs = 0;

	/*
	 * Detect if this function has ellipses and save all
	 * argument registers onto stack.
	 */
	usym = cftnsp->sdf->dfun;
	while (usym && usym->type != TNULL) {
		if (usym->type == TELLIPSIS) {
			saveallargs = 1;
			break;
		}
		++usym;
	}

	/* if returning a structure, move the hidden argument into a TEMP */
	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		param_retstruct();
		++argofs;
	}

	/* recalculate the arg offset and create TEMP moves */
	for (i = 0; i < cnt; i++) {

		if (sp[i] == NULL)
			continue;

		if ((argofs >= NARGREGS) && !xtemps)
			break;

		if (argofs > NARGREGS) {
			putintemp(sp[i]);
		} else if (sp[i]->stype == STRTY || sp[i]->stype == UNIONTY) {
			param_struct(sp[i], &argofs);
		} else if (DEUNSIGN(sp[i]->stype) == LONGLONG) {
			param_64bit(sp[i], &argofs, xtemps && !saveallargs);
		} else if (sp[i]->stype == DOUBLE || sp[i]->stype == LDOUBLE) {
			if (features(FEATURE_HARDFLOAT))
				param_double(sp[i], &argofs,
				    xtemps && !saveallargs);
			else
				param_64bit(sp[i], &argofs,
				    xtemps && !saveallargs);
		} else if (sp[i]->stype == FLOAT) {
			if (features(FEATURE_HARDFLOAT))
				param_float(sp[i], &argofs,
				    xtemps && !saveallargs);
			else
				param_32bit(sp[i], &argofs,
				    xtemps && !saveallargs);
		} else {
			param_32bit(sp[i], &argofs, xtemps && !saveallargs);
		}
	}

	/* if saveallargs, save the rest of the args onto the stack */
	while (saveallargs && argofs < NARGREGS) {
      		NODE *p, *q;
		int off = ARGINIT/SZINT + argofs;
		q = block(REG, NIL, NIL, INT, 0, 0);
		regno(q) = R0 + argofs++;
		p = block(REG, NIL, NIL, INT, 0, 0);
		regno(p) = FPREG;
		p = block(PLUS, p, bcon(4*off), INT, 0, 0);
		p = block(UMUL, p, NIL, INT, 0, 0);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	}

}

/*
 * End-of-Function code:
 */
void
efcode(void)
{
	NODE *p, *q;
	int tempnr;

	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;

	/*
	 * At this point, the address of the return structure on
	 * has been FORCEd to RETREG, which is R0.
	 * We want to copy the contents from there to the address
	 * we placed into the tempnode "rvnr".
	 */

	/* move the pointer out of R0 to a tempnode */
	q = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	q->n_rval = R0;
	p = tempnode(0, PTR+STRTY, 0, cftnsp->sap);
	tempnr = regno(p);
	p = buildtree(ASSIGN, p, q);
	ecomp(p);

	/* get the address from the tempnode */
	q = tempnode(tempnr, PTR+STRTY, 0, cftnsp->sap);
	q = buildtree(UMUL, q, NIL);
	
	/* now, get the structure destination */
	p = tempnode(rvnr, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(UMUL, p, NIL);

	/* struct assignment */
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
}

/*
 * End-of-job: called just before final exit.
 */
void
ejobcode(int flag)
{
	printf("\t.ident \"PCC: %s\"\n", VERSSTR);
}

/*
 * Beginning-of-job: called before compilation starts
 *
 * Initialise data structures specific for the local machine.
 */
void
bjobcode(void)
{
}

/*
 * fix up type of field p
 */
void
fldty(struct symtab *p)
{
}

/*
 * Build target-dependent switch tree/table.
 *
 * Return 1 if successfull, otherwise return 0 and the
 * target-independent tree will be used.
 */
int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}


/*
 * Straighten a chain of CM ops so that the CM nodes
 * only appear on the left node.
 *
 *	  CM	       CM
 *	CM  CM	   CM  b
 *       x y  a b	CM  a
 *		      x  y
 */
static NODE *
straighten(NODE *p)
{
	NODE *r = p->n_right;

	if (p->n_op != CM || r->n_op != CM)
		return p;

	p->n_right = r->n_left;
	r->n_left = p;

	return r;
}

static NODE *
reverse1(NODE *p, NODE *a)
{
	NODE *l = p->n_left;
	NODE *r = p->n_right;

	a->n_right = r;
	p->n_left = a;

	if (l->n_op == CM) {
		return reverse1(l, p);
	} else {
		p->n_right = l;
		return p;
	}
}

/*
 * Reverse a chain of CM ops
 */
static NODE *
reverse(NODE *p)
{
	NODE *l = p->n_left;
	NODE *r = p->n_right;

	p->n_left = r;

	if (l->n_op == CM)
		return reverse1(l, p);

	p->n_right = l;

	return p;
}


/* push arg onto the stack */
/* called by moveargs() */
static NODE *
pusharg(NODE *p, int *regp)
{
	NODE *q;
	int sz;

	/* convert to register size, if smaller */
	sz = tsize(p->n_type, p->n_df, p->n_ap);
	if (sz < SZINT)
		p = block(SCONV, p, NIL, INT, 0, 0);

	q = block(REG, NIL, NIL, INT, 0, 0);
	regno(q) = SP;

	if (szty(p->n_type) == 1) {
		++(*regp);
		q = block(MINUSEQ, q, bcon(4), INT, 0, 0);
	} else {
		(*regp) += 2;
		q = block(MINUSEQ, q, bcon(8), INT, 0, 0);
	}

	q = block(UMUL, q, NIL, p->n_type, p->n_df, p->n_ap);

	return buildtree(ASSIGN, q, p);
}

/* setup call stack with 32-bit argument */
/* called from moveargs() */
static NODE *
movearg_32bit(NODE *p, int *regp)
{
	int reg = *regp;
	NODE *q;

	q = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_ap);
	regno(q) = reg++;
	q = buildtree(ASSIGN, q, p);

	*regp = reg;
	return q;
}

/* setup call stack with 64-bit argument */
/* called from moveargs() */
static NODE *
movearg_64bit(NODE *p, int *regp)
{
	int reg = *regp;
	NODE *q, *r;

#if ALLONGLONG == 64
	/* alignment */
	++reg;
	reg &= ~1;
	*regp = reg;
#endif

	if (reg > R3) {
		q = pusharg(p, regp);
	} else if (reg == R3) {
		/* half in and half out of the registers */
		r = tcopy(p);
		if (!features(FEATURE_BIGENDIAN)) {
			q = block(SCONV, p, NIL, INT, 0, 0);
			q = movearg_32bit(q, regp);     /* little-endian */
			r = buildtree(RS, r, bcon(32));
			r = block(SCONV, r, NIL, INT, 0, 0);
			r = pusharg(r, regp); /* little-endian */
		} else {
			q = buildtree(RS, p, bcon(32));
			q = block(SCONV, q, NIL, INT, 0, 0);
			q = movearg_32bit(q, regp);     /* big-endian */
			r = block(SCONV, r, NIL, INT, 0, 0);
			r = pusharg(r, regp); /* big-endian */
		}
		q = straighten(block(CM, q, r, p->n_type, p->n_df, p->n_ap));
	} else {
		q = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_ap);
		regno(q) = R0R1 + (reg - R0);
		q = buildtree(ASSIGN, q, p);
		*regp = reg + 2;
	}

	return q;
}

/* setup call stack with float/double argument */
/* called from moveargs() */
static NODE *
movearg_float(NODE *p, int *regp)
{
	NODE *q, *r;
	TWORD ty = INCREF(p->n_type);
	int tmpnr;

	/*
	 * Floats are passed in the general registers for
	 * compatibily with libraries compiled to handle soft-float.
	 */

	if (xtemps) {
		/* bounce on TOS */
		r = block(REG, NIL, NIL, ty, p->n_df, p->n_ap);
		regno(r) = SP;
		r = block(PLUS, r, bcon(-4), ty, p->n_df, p->n_ap);
		r = block(UMUL, r, NIL, p->n_type, p->n_df, p->n_ap);
		r = buildtree(ASSIGN, r, p);
		ecomp(r);

		/* bounce into temp */
		r = block(REG, NIL, NIL, PTR+INT, 0, 0);
		regno(r) = SP;
		r = block(PLUS, r, bcon(-8), PTR+INT, 0, 0);
		r = block(UMUL, r, NIL, INT, 0, 0);
		q = tempnode(0, INT, 0, 0);
		tmpnr = regno(q);
		r = buildtree(ASSIGN, q, r);
		ecomp(r);
	} else {
		/* copy directly into temp */
		q = tempnode(0, p->n_type, p->n_df, p->n_ap);
		tmpnr = regno(q);
		r = buildtree(ASSIGN, q, p);
		ecomp(r);
	}

	/* copy from temp to register parameter */
	r = tempnode(tmpnr, INT, 0, 0);
	q = block(REG, NIL, NIL, INT, 0, 0);
	regno(q) = (*regp)++;
	p = buildtree(ASSIGN, q, r);

	return p;
}

/* setup call stack with float/double argument */
/* called from moveargs() */
static NODE *
movearg_double(NODE *p, int *regp)
{
	NODE *q, *r;
	TWORD ty = INCREF(p->n_type);
	int tmpnr;

	if (xtemps) {
		/* bounce on TOS */
		r = block(REG, NIL, NIL, ty, p->n_df, p->n_ap);
		regno(r) = SP;
		r = block(PLUS, r, bcon(-8), ty, p->n_df, p->n_ap);
		r = block(UMUL, r, NIL, p->n_type, p->n_df, p->n_ap);
		r = buildtree(ASSIGN, r, p);
		ecomp(r);

		/* bounce into temp */
		r = block(REG, NIL, NIL, PTR+LONGLONG, 0, 0);
		regno(r) = SP;
		r = block(PLUS, r, bcon(-8), PTR+LONGLONG, 0, 0);
		r = block(UMUL, r, NIL, LONGLONG, 0, 0);
		q = tempnode(0, LONGLONG, 0, 0);
		tmpnr = regno(q);
		r = buildtree(ASSIGN, q, r);
		ecomp(r);
	} else {
		/* copy directly into temp */
		q = tempnode(0, p->n_type, p->n_df, p->n_ap);
		tmpnr = regno(q);
		r = buildtree(ASSIGN, q, p);
		ecomp(r);
	}

	/* copy from temp to register parameter */
	r = tempnode(tmpnr, LONGLONG, 0, 0);
	q = block(REG, NIL, NIL, LONGLONG, 0, 0);
	regno(q) = R0R1 - R0 + (*regp);
	p = buildtree(ASSIGN, q, r);

	(*regp) += 2;

	return p;
}


/* setup call stack with a structure */
/* called from moveargs() */
static NODE *
movearg_struct(NODE *p, int *regp)
{
	int reg = *regp;
	NODE *l, *q, *t, *r;
	int tmpnr;
	int navail;
	int num;
	int sz;
	int ty;
	int i;

	assert(p->n_op == STARG);

	navail = NARGREGS - (reg - R0);
	navail = navail < 0 ? 0 : navail;
	sz = tsize(p->n_type, p->n_df, p->n_ap) / SZINT;
	num = sz > navail ? navail : sz;

	/* remove STARG node */
	l = p->n_left;
	nfree(p);
	ty = l->n_type;

	/*
	 * put it into a TEMP, rather than tcopy(), since the tree
	 * in p may have side-affects
	 */
	t = tempnode(0, ty, l->n_df, l->n_ap);
	tmpnr = regno(t);
	q = buildtree(ASSIGN, t, l);

	/* copy structure into registers */
	for (i = 0; i < num; i++) {
		t = tempnode(tmpnr, ty, 0, 0);
		t = block(SCONV, t, NIL, PTR+INT, 0, 0);
		t = block(PLUS, t, bcon(4*i), PTR+INT, 0, 0);
		t = buildtree(UMUL, t, NIL);

		r = block(REG, NIL, NIL, INT, 0, 0);
		regno(r) = reg++;
		r = buildtree(ASSIGN, r, t);

		q = block(CM, q, r, INT, 0, 0);
	}

	/* put the rest of the structure on the stack */
	for (i = num; i < sz; i++) {
		t = tempnode(tmpnr, ty, 0, 0);
		t = block(SCONV, t, NIL, PTR+INT, 0, 0);
		t = block(PLUS, t, bcon(4*i), PTR+INT, 0, 0);
		t = buildtree(UMUL, t, NIL);
		r = pusharg(t, &reg);
		q = block(CM, q, r, INT, 0, 0);
	}

	q = reverse(q);

	*regp = reg;
	return q;
}


static NODE *
moveargs(NODE *p, int *regp)
{
	NODE *r, **rp;
	int reg;

	if (p->n_op == CM) {
		p->n_left = moveargs(p->n_left, regp);
		r = p->n_right;
		rp = &p->n_right;
	} else {
		r = p;
		rp = &p;
	}

	reg = *regp;

	if (reg > R3 && r->n_op != STARG) {
		*rp = pusharg(r, regp);
	} else if (r->n_op == STARG) {
		*rp = movearg_struct(r, regp);
	} else if (DEUNSIGN(r->n_type) == LONGLONG) {
		*rp = movearg_64bit(r, regp);
	} else if (r->n_type == DOUBLE || r->n_type == LDOUBLE) {
		*rp = movearg_double(r, regp);
	} else if (r->n_type == FLOAT) {
		*rp = movearg_float(r, regp);
	} else {
		*rp = movearg_32bit(r, regp);
	}

	return straighten(p);
}

/*
 * Fixup arguments to pass pointer-to-struct as first argument.
 *
 * called from funcode().
 */
static NODE *
retstruct(NODE *p)
{
	NODE *l, *r, *t, *q;
	TWORD ty;

	l = p->n_left;
	r = p->n_right;

	ty = DECREF(l->n_type) - FTN;

//      assert(tsize(ty, l->n_df, l->n_ap) == SZINT);

	/* structure assign */
	q = tempnode(0, ty, l->n_df, l->n_ap);
	q = buildtree(ADDROF, q, NIL);

	/* insert hidden assignment at beginning of list */
	if (r->n_op != CM) {
		p->n_right = block(CM, q, r, INCREF(ty), l->n_df, l->n_ap);
	} else {
		for (t = r; t->n_left->n_op == CM; t = t->n_left)
			;
		t->n_left = block(CM, q, t->n_left, INCREF(ty),
			    l->n_df, l->n_ap);
	}

	return p;
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 */
NODE *
funcode(NODE *p)
{
	int reg = R0;

	if (p->n_type == STRTY+FTN || p->n_type == UNIONTY+FTN) {
		p = retstruct(p);
		reg = R1;
	}

	p->n_right = moveargs(p->n_right, &reg);

	if (p->n_right == NULL)
		p->n_op += (UCALL - CALL);

	return p;
}
