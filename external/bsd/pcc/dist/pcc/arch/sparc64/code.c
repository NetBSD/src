/*	Id: code.c,v 1.22 2014/04/19 07:47:51 ragge Exp 	*/	
/*	$NetBSD: code.c,v 1.1.1.4.8.1 2014/08/19 23:52:09 tls Exp $	*/

/*
 * Copyright (c) 2008 David Crawshaw <david@zentus.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "pass1.h"

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
	case PICDATA:
	case PICRDATA:
	case TLSDATA:
	case TLSUDATA:
	case CTORS:
	case DTORS:
		uerror("FIXME: unknown section");
	case NMSEG: 
		printf("\t.section %s,\"a%c\",@progbits\n", name,
		    cftnsp ? 'x' : 'w');
		return;
	}
	printf("\t%s\n", name);
}


void
defloc(struct symtab *sp)
{
	TWORD t;
	char *name;

	t = sp->stype;

	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);

	if (!ISFTN(t)) {
		printf("\t.type %s,#object\n", name);
		printf("\t.size %s," CONFMT "\n", name,
			tsize(sp->stype, sp->sdf, sp->sap) / SZCHAR);
	}
	if (sp->sclass == EXTDEF)
		printf("\t.global %s\n", name);
	if (sp->slevel == 0) {
		printf("%s:\n", name);
	} else
		printf(LABFMT ":\n", sp->soffset);
}

void
efcode(void)
{
	/* XXX */
}

void
bfcode(struct symtab **sp, int cnt)
{
	int i, off;
	NODE *p, *q;
	struct symtab *sym;

	/* Process the first six arguments. */
	for (i=0; i < cnt && i < 6; i++) {
		sym = sp[i];
		q = block(REG, NIL, NIL, sym->stype, sym->sdf, sym->sap);
		q->n_rval = RETREG_PRE(sym->stype) + i;
		p = tempnode(0, sym->stype, sym->sdf, sym->sap);
		sym->soffset = regno(p);
		sym->sflags |= STNODE;
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	}

	/* Process the remaining arguments. */
	for (off = V9RESERVE; i < cnt; i++) {
		sym = sp[i];
		p = tempnode(0, sym->stype, sym->sdf, sym->sap);
		off = ALIGN(off, (tlen(p) - 1));
		sym->soffset = off * SZCHAR;
		off += tlen(p);
		p = buildtree(ASSIGN, p, nametree(sym));
		sym->soffset = regno(p->n_left);
		sym->sflags |= STNODE;
		ecomp(p);
	}
}

void
ejobcode(int flag)
{
}

void
bjobcode(void)
{
	astypnames[USHORT] = astypnames[SHORT] = "\t.half";
	astypnames[INT] = astypnames[UNSIGNED] = "\t.long";
	astypnames[LONG] = astypnames[ULONG] = 
	    astypnames[LONGLONG] = astypnames[ULONGLONG] = "\t.xword";
}

/*
 * The first six 64-bit arguments are saved in the registers O0 to O5,
 * which become I0 to I5 after the "save" instruction moves the register
 * window. Arguments 7 and up must be saved on the stack to %sp+BIAS+176.
 *
 * For a pretty picture, see Figure 3-16 in the SPARC Compliance Def 2.4.
 */
static NODE *
moveargs(NODE *p, int *regp, int *stacksize)
{
	NODE *r, *q;

	if (p->n_op == CM) {
		p->n_left = moveargs(p->n_left, regp, stacksize);
		r = p->n_right;
	} else {
		r = p;
	}

	/* XXX more than six FP args can and should be passed in registers. */
	if (*regp > 5 && r->n_op != STARG) {
		/* We are storing the stack offset in n_rval. */
		r = block(FUNARG, r, NIL, r->n_type, r->n_df, r->n_ap);
		/* Make sure we are appropriately aligned. */
		*stacksize = ALIGN(*stacksize, (tlen(r) - 1));
		r->n_rval = *stacksize;
		*stacksize += tlen(r);
	} else if (r->n_op == STARG)
		cerror("op STARG in moveargs");
	else {
		q = block(REG, NIL, NIL, r->n_type, r->n_df, r->n_ap);

		/*
		 * The first six non-FP arguments go in the registers O0 - O5.
		 * Float arguments are stored in %fp1, %fp3, ..., %fp29, %fp31.
		 * Double arguments are stored in %fp0, %fp2, ..., %fp28, %fp30.
		 * A non-fp argument still increments register, eg.
		 *     test(int a, int b, float b)
		 * takes %o0, %o1, %fp5.
		 */
		if (q->n_type == FLOAT)
			q->n_rval = F0 + (*regp++ * 2) + 1;
		else if (q->n_type == DOUBLE)
			q->n_rval = D0 + *regp++;
		else if (q->n_type == LDOUBLE)
			cerror("long double support incomplete");
		else
			q->n_rval = O0 + (*regp)++;

		r = buildtree(ASSIGN, q, r);
	}

	if (p->n_op == CM) {
		p->n_right = r;
		return p;
	}

	return r;
}

NODE *
funcode(NODE *p)
{
	NODE *r, *l;
	int reg = 0, stacksize = 0;

	r = l = 0;

	p->n_right = moveargs(p->n_right, &reg, &stacksize);

	/*
	 * This is a particularly gross and inefficient way to handle
	 * argument overflows. First, we calculate how much stack space
	 * we need in moveargs(). Then we assign it by moving %sp, make
	 * the function call, and then move %sp back.
	 *
	 * What we should be doing is getting the maximum of all the needed
	 * stacksize values to the prologue and doing it all in the "save"
	 * instruction.
	 */
	if (stacksize != 0) {
		stacksize = V9STEP(stacksize); /* 16-bit alignment. */

		r = block(REG, NIL, NIL, INT, 0, 0);
		r->n_lval = 0;
		r->n_rval = SP;
		r = block(MINUS, r, bcon(stacksize), INT, 0, 0);

		l = block(REG, NIL, NIL, INT, 0, 0);
		l->n_lval = 0;
		l->n_rval = SP;
		r = buildtree(ASSIGN, l, r);

		p = buildtree(COMOP, r, p);

		r = block(REG, NIL, NIL, INT, 0, 0);
		r->n_lval = 0;
		r->n_rval = SP;
		r = block(PLUS, r, bcon(stacksize), INT, 0, 0);

		l = block(REG, NIL, NIL, INT, 0, 0);
		l->n_lval = 0;
		l->n_rval = SP;
		r = buildtree(ASSIGN, l, r);

		p = buildtree(COMOP, p, r);

	}
	return p;
}

void
fldty(struct symtab *p)
{
}

int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}
