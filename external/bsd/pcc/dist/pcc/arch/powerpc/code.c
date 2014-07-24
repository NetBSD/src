/*	Id: code.c,v 1.31 2014/05/29 19:20:03 plunky Exp 	*/	
/*	$NetBSD: code.c,v 1.1.1.5 2014/07/24 19:20:36 plunky Exp $	*/
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

#include <assert.h>
#include <stdlib.h>

#include "pass1.h"
#include "pass2.h"

static void genswitch_bintree(int num, TWORD ty, struct swents **p, int n);

#if 0
static void genswitch_table(int num, struct swents **p, int n);
static void genswitch_mrst(int num, struct swents **p, int n);
#endif

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
#ifdef MACHOABI
	case PICLDATA:
	case PICDATA: name = ".section .data.rel.rw,\"aw\""; break;
	case PICRDATA: name = ".section .data.rel.ro,\"aw\""; break;
	case STRNG: name = ".cstring"; break;
	case RDATA: name = ".const_data"; break;
#else
	case PICLDATA: name = ".section .data.rel.local,\"aw\",@progbits";break;
	case PICDATA: name = ".section .data.rel.rw,\"aw\",@progbits"; break;
	case PICRDATA: name = ".section .data.rel.ro,\"aw\",@progbits"; break;
	case STRNG:
#ifdef AOUTABI
	case RDATA: name = ".data"; break;
#else
	case RDATA: name = ".section .rodata"; break;
#endif
#endif
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

void
defalign(int al)
{
	if (ispow2(al/ALCHAR))
		printf("\t.p2align %d\n", ispow2(al/ALCHAR));
}


/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	char *name;

	name = sp->soname ? sp->soname : exname(sp->sname);
	if (sp->sclass == EXTDEF)
		printf("	.globl %s\n", name);
	if (sp->slevel == 0)
		printf("%s:\n", name);
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
#endif

        navail = NARGREGS - argofs;

        if (navail < 2) {
		/* half in and half out of the registers */
		q = block(REG, NIL, NIL, INT, 0, 0);
		regno(q) = R3 + argofs;
		p = block(REG, NIL, NIL, INT, 0, 0);
		regno(p) = FPREG;
		p = block(PLUS, p, bcon(sym->soffset/SZCHAR), PTR+INT, 0, 0);
		p = block(UMUL, p, NIL, INT, 0, 0);
        } else {
	        q = block(REG, NIL, NIL, sym->stype, sym->sdf, sym->sap);
		regno(q) = R3R4 + argofs;
		if (dotemps) {
			p = tempnode(0, sym->stype, sym->sdf, sym->sap);
			sym->soffset = regno(p);
			sym->sflags |= STNODE;
		} else {
			p = nametree(sym);
		}
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
        regno(q) = R3 + (*argofsp)++;
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
         * we have to dump the double from the general register
         * into a temp, since the register allocator doesn't like
         * floats to be in CLASSA.  This may not work for -xtemps.
         */

	if (xtemps) {
	        q = block(REG, NIL, NIL, ULONGLONG, 0, 0);
		regno(q) = R3R4 + *argofsp;
		p = block(REG, NIL, NIL, PTR+ULONGLONG, 0, 0);
		regno(p) = SPREG;
		p = block(PLUS, p, bcon(-8), INT, 0, 0);
		p = block(UMUL, p, NIL, ULONGLONG, 0, 0);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	
	        t = tempnode(0, sym->stype, sym->sdf, sym->sap);
		tmpnr = regno(t);
		p = block(REG, NIL, NIL,
		    INCREF(sym->stype), sym->sdf, sym->sap);
		regno(p) = SPREG;
		p = block(PLUS, p, bcon(-8), INT, 0, 0);
		p = block(UMUL, p, NIL, sym->stype, sym->sdf, sym->sap);
		p = buildtree(ASSIGN, t, p);
		ecomp(p);
	} else {
		/* bounce straight into temp */
		p = block(REG, NIL, NIL, ULONGLONG, 0, 0);
		regno(p) = R3R4 + *argofsp;
		t = tempnode(0, ULONGLONG, 0, 0);
		tmpnr = regno(t);
		p = buildtree(ASSIGN, t, p);
		ecomp(p);
	}

	(*argofsp) += 2;
	
	sym->soffset = tmpnr;
	sym->sflags |= STNODE;
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

	if (xtemps) {
		/* bounce onto TOS */
		q = block(REG, NIL, NIL, INT, 0, 0);
		regno(q) = R3 + (*argofsp);
		p = block(REG, NIL, NIL, INT, 0, 0);
		regno(p) = SPREG;
		p = block(PLUS, p, bcon(-4), INT, 0, 0);
		p = block(UMUL, p, NIL, INT, 0, 0);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);

		t = tempnode(0, sym->stype, sym->sdf, sym->sap);
		tmpnr = regno(t);
		p = block(REG, NIL, NIL, INCREF(sym->stype),
		    sym->sdf, sym->sap);
		regno(p) = SPREG;
		p = block(PLUS, p, bcon(-4), INT, 0, 0);
		p = block(UMUL, p, NIL, sym->stype, sym->sdf, sym->sap);
		p = buildtree(ASSIGN, t, p);
		ecomp(p);
	} else {
		/* bounce straight into temp */
		p = block(REG, NIL, NIL, INT, 0, 0);
		regno(p) = R3 + (*argofsp);
		t = tempnode(0, INT, 0, 0);
		tmpnr = regno(t);
		p = buildtree(ASSIGN, t, p);
		ecomp(p);
	}

	(*argofsp)++;

	sym->soffset = tmpnr;
	sym->sflags |= STNODE;
}

/* setup the hidden pointer to struct return parameter
 * used by bfcode() */
static void
param_retstruct(void)
{
	NODE *p, *q;

	p = tempnode(0, INCREF(cftnsp->stype), 0, cftnsp->sap);
	rvnr = regno(p);
	q = block(REG, NIL, NIL, INCREF(cftnsp->stype),
	    cftnsp->sdf, cftnsp->sap);
	regno(q) = R3;
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
                regno(q) = R3 + argofs++;
                p = block(REG, NIL, NIL, INT, 0, 0);
                regno(p) = SPREG;
                p = block(PLUS, p, bcon(4*off++), INT, 0, 0);
                p = block(UMUL, p, NIL, INT, 0, 0);
                p = buildtree(ASSIGN, p, q);
                ecomp(p);
        }

        *argofsp = argofs;
}

/*
 * code for the beginning of a function
 * sp is an array of indices in symtab for the arguments
 * cnt is the number of arguments
 */
void
bfcode(struct symtab **sp, int cnt)
{
#ifdef USE_GOTNR
	extern int gotnr;
#endif

	struct symtab *sp2;
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

	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		param_retstruct();
		++argofs;
	}

#ifdef USE_GOTNR
	if (kflag) {
		/* put GOT register into temporary */
		NODE *q, *p;
		q = block(REG, NIL, NIL, INT, 0, 0);
		regno(q) = GOTREG;
		p = tempnode(0, INT, 0, 0);
		gotnr = regno(p);
		ecomp(buildtree(ASSIGN, p, q));
	}
#endif

        /* recalculate the arg offset and create TEMP moves */
        for (i = 0; i < cnt; i++) {

		if (sp[i] == NULL)
			continue;

                if ((argofs >= NARGREGS) && !xtemps)
                        break;

                if (argofs >= NARGREGS) {
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
		/* int off = (ARGINIT+FIXEDSTACKSIZE*SZCHAR)/SZINT + argofs; */
		int off = ARGINIT/SZINT + argofs;
		q = block(REG, NIL, NIL, INT, 0, 0);
		regno(q) = R3 + argofs++;
		p = block(REG, NIL, NIL, INT, 0, 0);
		regno(p) = FPREG;
		p = block(PLUS, p, bcon(4*off), INT, 0, 0);
		p = block(UMUL, p, NIL, INT, 0, 0);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);
	}

	/* profiling */
	if (pflag) {
		NODE *p;

#if defined(ELFABI)

		sp2 = lookup("_mcount", 0);
		sp2->stype = EXTERN;
		p = nametree(sp2);
		p->n_sp->sclass = EXTERN;
		p = clocal(p);
		p = buildtree(ADDROF, p, NIL);
		p = block(UCALL, p, NIL, INT, 0, 0);
		ecomp(funcode(p));


#elif defined(MACHOABI)

		NODE *q;
		int tmpnr;

                q = block(REG, NIL, NIL, INT, 0, 0);
                regno(q) = R0;
		p = tempnode(0, INT, 0, 0);
		tmpnr = regno(p);
		p = buildtree(ASSIGN, p, q);
		ecomp(p);

		q = tempnode(tmpnr, INT, 0, 0);

		sp2 = lookup("mcount", 0);
		sp2->stype = EXTERN;
		p = nametree(sp2);
		p->n_sp->sclass = EXTERN;
		p = clocal(p);
		p = buildtree(ADDROF, p, NIL);
		p = block(CALL, p, q, INT, 0, 0);
		ecomp(funcode(p));

#endif
	}
}

/*
 * code for the end of a function
 * deals with struct return here
 */
void
efcode(void)
{
        NODE *p, *q;
        int tempnr;
        int ty;

#ifdef USE_GOTNR
	extern int gotnr;
	gotnr = 0;
#endif

        if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
                return;

        ty = cftnsp->stype - FTN;

        q = block(REG, NIL, NIL, INCREF(ty), 0, cftnsp->sap);
        regno(q) = R3;
        p = tempnode(0, INCREF(ty), 0, cftnsp->sap);
        tempnr = regno(p);
        p = buildtree(ASSIGN, p, q);
        ecomp(p);

        q = tempnode(tempnr, INCREF(ty), 0, cftnsp->sap);
        q = buildtree(UMUL, q, NIL);

        p = tempnode(rvnr, INCREF(ty), 0, cftnsp->sap);
        p = buildtree(UMUL, p, NIL);

        p = buildtree(ASSIGN, p, q);
        ecomp(p);

        q = tempnode(rvnr, INCREF(ty), 0, cftnsp->sap);
        p = block(REG, NIL, NIL, INCREF(ty), 0, cftnsp->sap);
        regno(p) = R3;
        p = buildtree(ASSIGN, p, q);
        ecomp(p);
}

struct stub stublist;
struct stub nlplist;

/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag)
{

#if defined(MACHOABI)
	/*
	 * iterate over the stublist and output the PIC stubs
`	 */
	if (kflag) {
		struct stub *p;

		DLIST_FOREACH(p, &stublist, link) {
			printf("\t.section __TEXT, __picsymbolstub1,symbol_stubs,pure_instructions,32\n");
			printf("\t.align 5\n");
			printf("L%s$stub:\n", p->name);
			if (strcmp(p->name, "mcount") == 0)
				printf("\t.indirect_symbol %s\n", p->name);
			else
				printf("\t.indirect_symbol %s\n", p->name);
			printf("\tmflr r0\n");
			printf("\tbcl 20,31,L%s$spb\n", p->name);
			printf("L%s$spb:\n", p->name);
			printf("\tmflr r11\n");
			printf("\taddis r11,r11,ha16(L%s$lazy_ptr-L%s$spb)\n",
			    p->name, p->name);
			printf("\tmtlr r0\n");
			printf("\tlwzu r12,lo16(L%s$lazy_ptr-L%s$spb)(r11)\n",
			    p->name, p->name);
			printf("\tmtctr r12\n");
			printf("\tbctr\n");
			printf("\t.lazy_symbol_pointer\n");
			printf("L%s$lazy_ptr:\n", p->name);
			if (strcmp(p->name, "mcount") == 0)
				printf("\t.indirect_symbol %s\n", p->name);
			else
				printf("\t.indirect_symbol %s\n", p->name);
			printf("\t.long	dyld_stub_binding_helper\n");
			printf("\t.subsections_via_symbols\n");
		}

		printf("\t.non_lazy_symbol_pointer\n");
		DLIST_FOREACH(p, &nlplist, link) {
			printf("L%s$non_lazy_ptr:\n", p->name);
			if (strcmp(p->name, "mcount") == 0)
				printf("\t.indirect_symbol %s\n", p->name);
			else
				printf("\t.indirect_symbol %s\n", p->name);
			printf("\t.long 0\n");
	        }

	}
#endif

#ifndef os_darwin
	printf("\t.ident \"PCC: %s\"\n", VERSSTR);
#endif

}

void
bjobcode(void)
{
	DLIST_INIT(&stublist, link);
	DLIST_INIT(&nlplist, link);
}

#ifdef notdef
/*
 * Print character t at position i in one string, until t == -1.
 * Locctr & label is already defined.
 */
void
bycode(int t, int i)
{
	static	int	lastoctal = 0;

	/* put byte i+1 in a string */

	if (t < 0) {
		if (i != 0)
			puts("\"");
	} else {
		if (i == 0)
			printf("\t.ascii \"");
		if (t == '\\' || t == '"') {
			lastoctal = 0;
			putchar('\\');
			putchar(t);
		} else if (t < 040 || t >= 0177) {
			lastoctal++;
			printf("\\%o",t);
		} else if (lastoctal && '0' <= t && t <= '9') {
			lastoctal = 0;
			printf("\"\n\t.ascii \"%c", t);
		} else {	
			lastoctal = 0;
			putchar(t);
		}
	}
}
#endif

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
	if (num < 0) {
		genswitch_bintree(num, type, p, n);
		return 1;
	}

	return 0;

#if 0
	if (0)
	genswitch_table(num, p, n);
	if (0)
	genswitch_bintree(num, p, n);
	genswitch_mrst(num, p, n);
#endif
}

static void bintree_rec(TWORD ty, int num,
	struct swents **p, int n, int s, int e);

static void
genswitch_bintree(int num, TWORD ty, struct swents **p, int n)
{
	int lab = getlab();

	if (p[0]->slab == 0)
		p[0]->slab = lab;

	bintree_rec(ty, num, p, n, 1, n);

	plabel(lab);
}

static void
bintree_rec(TWORD ty, int num, struct swents **p, int n, int s, int e)
{
	NODE *r;
	int rlabel;
	int h;

	if (s == e) {
		r = tempnode(num, ty, 0, 0);
		r = buildtree(NE, r, bcon(p[s]->sval));
		cbranch(buildtree(NOT, r, NIL), bcon(p[s]->slab));
		branch(p[0]->slab);
		return;
	}

	rlabel = getlab();

	h = s + (e - s) / 2;

	r = tempnode(num, ty, 0, 0);
	r = buildtree(GT, r, bcon(p[h]->sval));
	cbranch(r, bcon(rlabel));
	bintree_rec(ty, num, p, n, s, h);
	plabel(rlabel);
	bintree_rec(ty, num, p, n, h+1, e);
}


#if 0

static void
genswitch_table(int num, struct swents **p, int n)
{
	NODE *r, *t;
	int tval;
	int minval, maxval, range;
	int deflabel, tbllabel;
	int i, j;

	minval = p[1]->sval;
	maxval = p[n]->sval;

	range = maxval - minval + 1;

	if (n < 10 || range > 3 * n) {
		/* too small or too sparse for jump table */
		genswitch_simple(num, p, n);
		return;
	}

	r = tempnode(num, UNSIGNED, 0, 0);
	r = buildtree(MINUS, r, bcon(minval));
	t = tempnode(0, UNSIGNED, 0, 0);
	tval = regno(t);
	r = buildtree(ASSIGN, t, r);
	ecomp(r);

	deflabel = p[0]->slab;
	if (deflabel == 0)
		deflabel = getlab();

	t = tempnode(tval, UNSIGNED, 0, 0);
	cbranch(buildtree(GT, t, bcon(maxval-minval)), bcon(deflabel));

	tbllabel = getlab();
	struct symtab *strtbl = lookup("__switch_table", SLBLNAME|STEMP);
	strtbl->soffset = tbllabel;
	strtbl->sclass = ILABEL;
	strtbl->stype = INCREF(UCHAR);

	t = block(NAME, NIL, NIL, UNSIGNED, 0, 0);
	t->n_sp = strtbl;
	t = buildtree(ADDROF, t, NIL);
	r = tempnode(tval, UNSIGNED, 0, 0);
	r = buildtree(PLUS, t, r);
	t = tempnode(0, INCREF(UNSIGNED), 0, 0);
	r = buildtree(ASSIGN, t, r);
	ecomp(r);

	r = tempnode(regno(t), INCREF(UNSIGNED), 0, 0);
	r = buildtree(UMUL, r, NIL);
	t = block(NAME, NIL, NIL, UCHAR, 0, 0);
	t->n_sp = strtbl;
	t = buildtree(ADDROF, t, NIL);
	r = buildtree(PLUS, t, r);
	r = block(GOTO, r, NIL, 0, 0, 0);
	ecomp(r);

	plabel(tbllabel);
	for (i = minval, j=1; i <= maxval; i++) {
		char *entry = tmpalloc(20);
		int lab = deflabel;
		//printf("; minval=%d, maxval=%d, i=%d, j=%d p[j]=%lld\n", minval, maxval, i, j, p[j]->sval);
		if (p[j]->sval == i) {
			lab = p[j]->slab;
			j++;
		}
		snprintf(entry, 20, "\t.long " LABFMT "-" LABFMT "\n", lab, tbllabel);
		send_passt(IP_ASM, entry);
	}

	if (p[0]->slab <= 0)
		plabel(deflabel);
}

#define DPRINTF(x)	if (xdebug) printf x
//#define DPRINTF(x)	do { } while(0)

#define MIN_TABLE_SIZE	8

/*
 *  Multi-way Radix Search Tree (MRST)
 */

static void mrst_rec(int num, struct swents **p, int n, int *state, int lab);
static unsigned long mrst_find_window(struct swents **p, int n, int *state, int lab, int *len, int *lowbit);
void mrst_put_entry_and_recurse(int num, struct swents **p, int n, int *state, int tbllabel, int lab, unsigned long j, unsigned long tblsize, unsigned long Wmax, int lowbit);

static void
genswitch_mrst(int num, struct swents **p, int n)
{
	int *state;
	int i;
	int putlabel = 0;

	if (n < 10) {
		/* too small for MRST */
		genswitch_simple(num, p, n);
		return;
	}

	state = tmpalloc((n+1)*sizeof(int));
	for (i = 0; i <= n; i++)
		state[i] = 0;

	if (p[0]->slab == 0) {
		p[0]->slab = getlab();
		putlabel = 1;
	}

	mrst_rec(num, p, n, state, 0);

	if (putlabel)
		plabel(p[0]->slab);
}


/*
 *  Look through the cases and generate a table or
 *  list of simple comparisons.  If generating a table,
 *  invoke mrst_put_entry_and_recurse() to put
 *  an entry in the table and recurse.
 */
static void
mrst_rec(int num, struct swents **p, int n, int *state, int lab)
{
	int len, lowbit;
	unsigned long Wmax;
	unsigned int tblsize;
	NODE *t;
	NODE *r;
	int tval;
	int i;

	DPRINTF(("mrst_rec: num=%d, n=%d, lab=%d\n", num, n, lab));

	/* find best window to cover set*/
	Wmax = mrst_find_window(p, n, state, lab, &len, &lowbit);
	tblsize = (1 << len);
	assert(len > 0 && tblsize > 0);

	DPRINTF(("mrst_rec: Wmax=%lu, lowbit=%d, tblsize=%u\n",
		Wmax, lowbit, tblsize));

	if (lab)
		plabel(lab);

	if (tblsize <= MIN_TABLE_SIZE) {
		DPRINTF(("msrt_rec: break the recursion\n"));
		for (i = 1; i <= n; i++) {
			if (state[i] == lab) {
				t = tempnode(num, UNSIGNED, 0, 0);
				cbranch(buildtree(EQ, t, bcon(p[i]->sval)),
				    bcon(p[i]->slab));
			}
		}
		branch(p[0]->slab);
		return;
	}

	DPRINTF(("generating table with %d elements\n", tblsize));

	// AND with Wmax
	t = tempnode(num, UNSIGNED, 0, 0);
	r = buildtree(AND, t, bcon(Wmax));

	// RS lowbits
	r = buildtree(RS, r, bcon(lowbit));

	t = tempnode(0, UNSIGNED, 0, 0);
	tval = regno(t);
	r = buildtree(ASSIGN, t, r);
	ecomp(r);

	int tbllabel = getlab();
	struct symtab *strtbl = lookup("__switch_table", SLBLNAME|STEMP);
	strtbl->sclass = STATIC;
	strtbl->sap = 0;
	strtbl->slevel = 1;
	strtbl->soffset = tbllabel;
	strtbl->stype = INCREF(UCHAR);
	strtbl->squal = (CON >> TSHIFT);

	t = block(NAME, NIL, NIL, UNSIGNED, 0, 0);
	t->n_sp = strtbl;
	t = buildtree(ADDROF, t, NIL);
	r = tempnode(tval, UNSIGNED, 0, 0);
	r = buildtree(PLUS, t, r);
	t = tempnode(0, INCREF(UNSIGNED), 0, 0);
	r = buildtree(ASSIGN, t, r);
	ecomp(r);

	r = tempnode(regno(t), INCREF(UNSIGNED), 0, 0);
	r = buildtree(UMUL, r, NIL);
	t = block(NAME, NIL, NIL, UCHAR, 0, 0);
	t->n_sp = strtbl;
	t = buildtree(ADDROF, t, NIL);
	r = buildtree(PLUS, t, r);
	r = block(GOTO, r, NIL, 0, 0, 0);
	ecomp(r);

	plabel(tbllabel);
	
	mrst_put_entry_and_recurse(num, p, n, state, tbllabel, lab,
		0, tblsize, Wmax, lowbit);
}


/*
 * Put an entry into the table and recurse to the next entry
 * in the table.  On the way back through the recursion, invoke
 * mrst_rec() to check to see if we should generate another
 * table.
 */
void
mrst_put_entry_and_recurse(int num, struct swents **p, int n, int *state,
	int tbllabel, int labval,
	unsigned long j, unsigned long tblsize, unsigned long Wmax, int lowbit)
{
	int i;
	int found = 0;
	int lab = getlab();

	/*
	 *  Look for labels which map to this table entry.
	 *  Mark each one in "state" that they fall inside this table.
	 */
	for (i = 1; i <= n; i++) {
		unsigned int val = (p[i]->sval & Wmax) >> lowbit;
		if (val == j && state[i] == labval) {
			found = 1;
			state[i] = lab;
		}
	}

	/* couldn't find any labels?  goto the default label */
	if (!found)
		lab = p[0]->slab;

	/* generate the table entry */
	char *entry = tmpalloc(20);
	snprintf(entry, 20, "\t.long " LABFMT "-" LABFMT "\n", lab, tbllabel);
	send_passt(IP_ASM, entry);

	DPRINTF(("mrst_put_entry: table=%d, pos=%lu/%lu, label=%d\n",
	    tbllabel, j, tblsize, lab));

	/* go to the next table entry */
	if (j+1 < tblsize) {
		mrst_put_entry_and_recurse(num, p, n, state, tbllabel, labval,
			j+1, tblsize, Wmax, lowbit);
	}

	/* if we are going to the default label, bail now */
	if (!found)
		return;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("state: ");
		for (i = 1; i <= n; i++)
			printf("%d ", state[i]);
		printf("\n");
	}
#endif

	/* build another table */
	mrst_rec(num, p, n, state, lab);
}

/*
 * counts the number of entries in a table of size (1 << L) which would
 * be used given the cases and the mask (W, lowbit).
 */
static unsigned int
mrst_cardinality(struct swents **p, int n, int *state, int step, unsigned long W, int L, int lowbit)
{
	unsigned int count = 0;
	int i;

	if (W == 0)
		return 0;

	int *vals = (int *)calloc(1 << L, sizeof(int));
	assert(vals);

	DPRINTF(("mrst_cardinality: "));
	for (i = 1; i <= n; i++) {
		int idx;
		if (state[i] != step)
			continue;
		idx = (p[i]->sval & W) >> lowbit;
		DPRINTF(("%llu->%d, ", p[i]->sval, idx));
		if (!vals[idx]) {
			count++;
		}
		vals[idx] = 1;
	}
	DPRINTF((": found %d entries\n", count));
	free(vals);

	return count;
}

/*
 *  Find the maximum window (table size) which would best cover
 *  the set of labels.  Algorithm explained in:
 *
 *  Ulfar Erlingsson, Mukkai Krishnamoorthy and T.V. Raman.
 *  Efficient Multiway Radix Search Trees.
 *  Information Processing Letters 60:3 115-120 (November 1996)
 */

static unsigned long
mrst_find_window(struct swents **p, int n, int *state, int lab, int *len, int *lowbit)
{
	unsigned int tblsize;
	unsigned long W = 0;
	unsigned long Wmax = 0;
	unsigned long Wleft = (1 << (SZLONG-1));
	unsigned int C = 0;
	unsigned int Cmax = 0;
	int L = 0;
	int Lmax = 0;
	int lowmax = 0;
	int no_b = SZLONG-1;
	unsigned long b = (1 << (SZLONG-1));

	DPRINTF(("mrst_find_window: n=%d, lab=%d\n", n, lab));

	for (; b > 0; b >>= 1, no_b--) {

		// select the next bit
		W |= b;
		L += 1;

		tblsize = 1 << L;
		assert(tblsize > 0);

		DPRINTF(("no_b=%d, b=0x%lx, Wleft=0x%lx, W=0x%lx, Wmax=0x%lx, L=%d, Lmax=%d, Cmax=%u, lowmax=%d, tblsize=%u\n", no_b, b, Wleft, W, Wmax, L, Lmax, Cmax, lowmax, tblsize));

		C = mrst_cardinality(p, n, state, lab, W, L, no_b);
		DPRINTF((" -> cardinality is %d\n", C));

		if (2*C >= tblsize) {
			DPRINTF(("(found good match, keep adding to table)\n"));
			Wmax = W;
			Lmax = L;
			lowmax = no_b;
			Cmax = C;
		} else {
			DPRINTF(("(too sparse)\n"));
			assert((W & Wleft) != 0);

			/* flip the MSB and see if we get a better match */
			W ^= Wleft;
			Wleft >>= 1;
			L -= 1;

			DPRINTF((" --> trying W=0x%lx and L=%d and Cmax=%u\n", W, L, Cmax));
			C = mrst_cardinality(p, n, state, lab, W, L, no_b);
			DPRINTF((" --> C=%u\n", C));
			if (C > Cmax) {
				Wmax = W;
				Lmax = L;
				lowmax = no_b;
				Cmax = C;
				DPRINTF((" --> better!\n"));
			} else {
				DPRINTF((" --> no better\n"));
			}
		}

	}

#ifdef PCC_DEBUG
	if (xdebug) {
		int i;
		int hibit = lowmax + Lmax;
		printf("msrt_find_window: Wmax=0x%lx, lowbit=%d, result=", Wmax, lowmax);
		for (i = 31; i >= 0; i--) {
			int mask = (1 << i);
			if (i == hibit)
				printf("[");
			if (Wmax & mask)
				printf("1");
			else
				printf("0");
			if (i == lowmax)
				printf("]");
		}
		printf("\n");
	}
#endif

	assert(Lmax > 0);
	*len = Lmax;
	*lowbit = lowmax;

	DPRINTF(("msrt_find_window: returning Wmax=%lu, len=%d, lowbit=%d [tblsize=%u, entries=%u]\n", Wmax, Lmax, lowmax, tblsize, C));

	return Wmax;
}
#endif

/*
 * Straighten a chain of CM ops so that the CM nodes
 * only appear on the left node.
 *
 *          CM               CM
 *        CM  CM           CM  b
 *       x y  a b        CM  a
 *                      x  y
 *
 *           CM             CM
 *        CM    CM        CM c
 *      CM z  CM  c      CM b
 *     x y    a b      CM a
 *                   CM z
 *                  x y
 */
static NODE *
straighten(NODE *p)
{
        NODE *r = p->n_right;

        if (p->n_op != CM || r->n_op != CM)
                return p;

        p->n_right = r->n_left;
        r->n_left = straighten(p);

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
	int off;

        /* convert to register size, if smaller */
        sz = tsize(p->n_type, p->n_df, p->n_ap);
        if (sz < SZINT)
                p = block(SCONV, p, NIL, INT, 0, 0);

        q = block(REG, NIL, NIL, INCREF(p->n_type), p->n_df, p->n_ap);
        regno(q) = SPREG;

	off = ARGINIT/SZCHAR + 4 * (*regp - R3);
	q = block(PLUS, q, bcon(off), INT, 0, 0);
        q = block(UMUL, q, NIL, p->n_type, p->n_df, p->n_ap);
	(*regp) += szty(p->n_type);

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
#endif

        if (reg > R10) {
                *regp = reg;
                q = pusharg(p, regp);
	} else if (reg == R10) {
		/* half in and half out of the registers */
		r = tcopy(p);
		if (!features(FEATURE_BIGENDIAN)) {
			q = block(SCONV, p, NIL, INT, 0, 0);
			q = movearg_32bit(q, regp);	/* little-endian */
			r = buildtree(RS, r, bcon(32));
			r = block(SCONV, r, NIL, INT, 0, 0);
			r = pusharg(r, regp); /* little-endian */
		} else {
			q = buildtree(RS, p, bcon(32));
			q = block(SCONV, q, NIL, INT, 0, 0);
			q = movearg_32bit(q, regp);	/* big-endian */
			r = block(SCONV, r, NIL, INT, 0, 0);
			r = pusharg(r, regp); /* big-endian */
		}
		q = straighten(block(CM, q, r, p->n_type, p->n_df, p->n_ap));
        } else {
                q = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_ap);
                regno(q) = R3R4 + (reg - R3);
                q = buildtree(ASSIGN, q, p);
                *regp = reg + 2;
        }

        return q;
}

/* setup call stack with float argument */
/* called from moveargs() */
static NODE *
movearg_float(NODE *p, int *fregp, int *regp)
{
#if defined(MACHOABI)
	NODE *q, *r;
	TWORD ty = INCREF(p->n_type);
	int tmpnr;
#endif

	p = movearg_32bit(p, fregp);

	/*
	 * On OS/X, floats are passed in the floating-point registers
	 * and in the general registers for compatibily with libraries
	 * compiled to handle soft-float.
	 */

#if defined(MACHOABI)

	if (xtemps) {
		/* bounce into TOS */
		r = block(REG, NIL, NIL, ty, p->n_df, p->n_ap);
		regno(r) = SPREG;
		r = block(PLUS, r, bcon(-4), INT, 0, 0);
		r = block(UMUL, r, NIL, p->n_type, p->n_df, p->n_ap);
		r = buildtree(ASSIGN, r, p);
		ecomp(r);

		/* bounce into temp */
		r = block(REG, NIL, NIL, PTR+INT, 0, 0);
		regno(r) = SPREG;
		r = block(PLUS, r, bcon(-4), INT, 0, 0);
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

#endif
	return p;

}

/* setup call stack with float/double argument */
/* called from moveargs() */
static NODE *
movearg_double(NODE *p, int *fregp, int *regp)
{
#if defined(MACHOABI)
	NODE *q, *r;
	TWORD ty = INCREF(p->n_type);
	int tmpnr;
#endif

	/* this does the move to a single register for us */
	p = movearg_32bit(p, fregp);

	/*
	 * On OS/X, doubles are passed in the floating-point registers
	 * and in the general registers for compatibily with libraries
	 * compiled to handle soft-float.
	 */

#if defined(MACHOABI)

	if (xtemps) {
		/* bounce on TOS */
		r = block(REG, NIL, NIL, ty, p->n_df, p->n_ap);
		regno(r) = SPREG;
		r = block(PLUS, r, bcon(-8), ty, p->n_df, p->n_ap);
		r = block(UMUL, r, NIL, p->n_type, p->n_df, p->n_ap);
		r = buildtree(ASSIGN, r, p);
		ecomp(r);

		/* bounce into temp */
		r = block(REG, NIL, NIL, PTR+LONGLONG, 0, 0);
		regno(r) = SPREG;
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
	regno(q) = R3R4 - R3 + (*regp);
	p = buildtree(ASSIGN, q, r);

	(*regp) += 2;

#endif
	
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

        navail = NARGREGS - (reg - R3);
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
moveargs(NODE *p, int *regp, int *fregp)
{
        NODE *r, **rp;
        int reg, freg;

        if (p->n_op == CM) {
                p->n_left = moveargs(p->n_left, regp, fregp);
                r = p->n_right;
                rp = &p->n_right;
        } else {
                r = p;
                rp = &p;
        }

        reg = *regp;
	freg = *fregp;

#define	ISFLOAT(p)	(p->n_type == FLOAT || \
			p->n_type == DOUBLE || \
			p->n_type == LDOUBLE)

        if (reg > R10 && r->n_op != STARG) {
                *rp = pusharg(r, regp);
	} else if (r->n_op == STARG) {
		*rp = movearg_struct(r, regp);
        } else if (DEUNSIGN(r->n_type) == LONGLONG) {
                *rp = movearg_64bit(r, regp);
	} else if (r->n_type == DOUBLE || r->n_type == LDOUBLE) {
		if (features(FEATURE_HARDFLOAT))
			*rp = movearg_double(r, fregp, regp);
		else
                	*rp = movearg_64bit(r, regp);
	} else if (r->n_type == FLOAT) {
		if (features(FEATURE_HARDFLOAT))
			*rp = movearg_float(r, fregp, regp);
		else
                	*rp = movearg_32bit(r, regp);
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
	struct symtab s;
	NODE *l, *r, *t, *q;
	TWORD ty;

	l = p->n_left;
	r = p->n_right;

	ty = DECREF(l->n_type) - FTN;

	s.sclass = AUTO;
	s.stype = ty;
	s.sdf = l->n_df;
	s.sap = l->n_ap;
	oalloc(&s, &autooff);
	q = block(REG, NIL, NIL, INCREF(ty), l->n_df, l->n_ap);
	regno(q) = FPREG;
	q = block(MINUS, q, bcon(autooff/SZCHAR), INCREF(ty),
	    l->n_df, l->n_ap);

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
	int regnum = R3;
	int fregnum = F1;

        if (DECREF(p->n_left->n_type) == STRTY+FTN ||
            DECREF(p->n_left->n_type) == UNIONTY+FTN)
		p = retstruct(p);

	p->n_right = moveargs(p->n_right, &regnum, &fregnum);

	if (p->n_right == NULL)
		p->n_op += (UCALL - CALL);

	return p;
}
