/*	Id: code.c,v 1.4 2014/04/19 07:47:51 ragge Exp 	*/	
/*	$NetBSD: code.c,v 1.1.1.1 2014/07/24 19:21:16 plunky Exp $	*/
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


# include "pass1.h"

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
	case STRNG:
	case RDATA: name = ".section .rodata"; break;
	case UDATA: break;
	case PICLDATA:
	case PICDATA: name = ".section .data.rel.rw,\"aw\",@progbits"; break;
	case PICRDATA: name = ".section .data.rel.ro,\"aw\",@progbits"; break;
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
	char *name;

	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);

	if (sp->sclass == EXTDEF) {
		printf("\t.globl %s\n", name);
		if (ISFTN(sp->stype)) {
			printf("\t.type %s,@function\n", name);
		} else {
			printf("\t.type %s,@object\n", name);
			printf("\t.size %s,%d\n", name,
			    (int)tsize(sp->stype, sp->sdf, sp->sap)/SZCHAR);
		}
	}
	if (sp->slevel == 0)
		printf("%s:\n", name);
	else
		printf(LABFMT ":\n", sp->soffset);
}

/*
 * code for the end of a function
 * deals with struct return here
 * The return value is in (or pointed to by) RETREG.
 */
int sttemp;

void
efcode(void)
{
	NODE *p, *q;

	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;
	/* use stasg to get call to memcpy() */
	p = buildtree(UMUL, tempnode(sttemp, INCREF(STRTY),
	    cftnsp->sdf, cftnsp->sap), NIL);
	q = block(REG, 0, 0, INCREF(STRTY), cftnsp->sdf, cftnsp->sap);
	regno(q) = A0;
	q = buildtree(UMUL, q, NIL);
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **s, int cnt)
{
	struct symtab *sp2;
	NODE *n, *p;
	int i;

	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		n = tempnode(0, INCREF(CHAR), 0, 0);
		p = block(REG, 0, 0, INCREF(CHAR), 0, 0);
		regno(p) = A0;
		sttemp = regno(n);
		ecomp(buildtree(ASSIGN, n, p));
	}

	if (xtemps == 0)
		return;

        /* put arguments in temporaries */
        for (i = 0; i < cnt; i++) {
                if (s[i]->stype == STRTY || s[i]->stype == UNIONTY ||
                    cisreg(s[i]->stype) == 0)
                        continue;
                if (cqual(s[i]->stype, s[i]->squal) & VOL)
                        continue;
                sp2 = s[i];
                n = tempnode(0, s[i]->stype, s[i]->sdf, s[i]->sap);
                n = buildtree(ASSIGN, n, nametree(sp2));
                s[i]->soffset = regno(n->n_left);
                s[i]->sflags |= STNODE;
                ecomp(n);
        }
}


/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag)
{
	if (flag)
		return;

	printf("\t.ident \"PCC: %s\"\n", VERSSTR);
}

void
bjobcode(void)
{
	/* Set correct names for our types */
	astypnames[SHORT] = astypnames[USHORT] = "\t.word";
	astypnames[INT] = astypnames[UNSIGNED] = "\t.long";
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */
NODE *
funcode(NODE *p)
{
	NODE *r, *l;

	/* Fix function call arguments. On m68k, just add funarg */
	for (r = p->n_right; r->n_op == CM; r = r->n_left) {
		if (r->n_right->n_op != STARG) {
			r->n_right = intprom(r->n_right);
			r->n_right = block(FUNARG, r->n_right, NIL,
			    r->n_right->n_type, r->n_right->n_df,
			    r->n_right->n_ap);
		}
	}
	if (r->n_op != STARG) {
		l = talloc();
		*l = *r;
		r->n_op = FUNARG;
		r->n_left = l;
		r->n_left = intprom(r->n_left);
		r->n_type = r->n_left->n_type;
	}
	return p;

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

/*
 * Return return as given by a.
 */
NODE *
builtin_return_address(const struct bitable *bt, NODE *a)
{
	int nframes;
	NODE *f;

cerror((char *)__func__);
	nframes = a->n_lval;
	tfree(a);

	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = FPREG;

	while (nframes--)
		f = block(UMUL, f, NIL, PTR+VOID, 0, 0);

	f = block(PLUS, f, bcon(8), INCREF(PTR+VOID), 0, 0);
	f = buildtree(UMUL, f, NIL);

	return f;
}

/*
 * Return frame as given by a.
 */
NODE *
builtin_frame_address(const struct bitable *bt, NODE *a)
{
	int nframes;
	NODE *f;

cerror((char *)__func__);
	nframes = a->n_lval;
	tfree(a);

	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = FPREG;

	while (nframes--)
		f = block(UMUL, f, NIL, PTR+VOID, 0, 0);

	return f;
}

/*
 * Return "canonical frame address".
 */
NODE *
builtin_cfa(const struct bitable *bt, NODE *a)
{
	NODE *f;

cerror((char *)__func__);
	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = FPREG;
	return block(PLUS, f, bcon(16), INCREF(PTR+VOID), 0, 0);
}
