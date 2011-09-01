/*	Id: code.c,v 1.67 2011/06/23 13:43:04 ragge Exp 	*/	
/*	$NetBSD: code.c,v 1.1.1.4 2011/09/01 12:46:34 plunky Exp $	*/
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
		printf("\t.section %s,\"aw\",@progbits\n", name);
		return;
	}
	printf("\t%s\n", name);
}

#ifdef MACHOABI
void
defalign(int al)
{
	printf("\t.align %d\n", ispow2(al/ALCHAR));
}
#endif

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
		printf("	.globl %s\n", name);
#if defined(ELFABI)
		printf("\t.type %s,@%s\n", name,
		    ISFTN(sp->stype)? "function" : "object");
#endif
	}
#if defined(ELFABI)
	if (!ISFTN(sp->stype)) {
		if (sp->slevel == 0)
			printf("\t.size %s,%d\n", name,
			    (int)tsize(sp->stype, sp->sdf, sp->sap)/SZCHAR);
		else
			printf("\t.size " LABFMT ",%d\n", sp->soffset,
			    (int)tsize(sp->stype, sp->sdf, sp->sap)/SZCHAR);
	}
#endif
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
#if defined(os_openbsd)
	/* struct return for small structs */
	int sz = tsize(BTYPE(cftnsp->stype), cftnsp->sdf, cftnsp->sap);
	if (sz == SZCHAR || sz == SZSHORT || sz == SZINT || sz == SZLONGLONG) {
		/* Pointer to struct in eax */
		if (sz == SZLONGLONG) {
			q = block(OREG, NIL, NIL, INT, 0, 0);
			q->n_lval = 4;
			p = block(REG, NIL, NIL, INT, 0, 0);
			p->n_rval = EDX;
			ecomp(buildtree(ASSIGN, p, q));
		}
		if (sz < SZSHORT) sz = CHAR;
		else if (sz > SZSHORT) sz = INT;
		else sz = SHORT;
		q = block(OREG, NIL, NIL, sz, 0, 0);
		p = block(REG, NIL, NIL, sz, 0, 0);
		ecomp(buildtree(ASSIGN, p, q));
		return;
	}
#endif
	/* Create struct assignment */
	q = block(OREG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	q->n_rval = EBP;
	q->n_lval = 8; /* return buffer offset */
	q = buildtree(UMUL, q, NIL);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);

	/* put hidden arg in eax on return */
	q = block(OREG, NIL, NIL, INT, 0, 0);
	regno(q) = FPREG;
	q->n_lval = 8;
	p = block(REG, NIL, NIL, INT, 0, 0);
	regno(p) = EAX;
	ecomp(buildtree(ASSIGN, p, q));
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **sp, int cnt)
{
	extern int argstacksize;
	struct symtab *sp2;
	extern int gotnr;
	NODE *n, *p;
	int i;

	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		/* Function returns struct, adjust arg offset */
#if defined(os_openbsd)
		/* OpenBSD uses non-standard return for small structs */
		int sz = tsize(BTYPE(cftnsp->stype), cftnsp->sdf, cftnsp->sap);
		if (sz != SZCHAR && sz != SZSHORT &&
		    sz != SZINT && sz != SZLONGLONG)
#endif
			for (i = 0; i < cnt; i++) 
				sp[i]->soffset += SZPOINT(INT);
	}

#ifdef GCC_COMPAT
	if (attr_find(cftnsp->sap, GCC_ATYP_STDCALL) != NULL)
		cftnsp->sflags |= SSTDCALL;
#endif

	/*
	 * Count the arguments
	 */
	argstacksize = 0;
	if (cftnsp->sflags & SSTDCALL) {
#ifdef os_win32

		char buf[256];
		char *name;
#endif

		for (i = 0; i < cnt; i++) {
			TWORD t = sp[i]->stype;
			if (t == STRTY || t == UNIONTY)
				argstacksize +=
				    tsize(t, sp[i]->sdf, sp[i]->sap);
			else
				argstacksize += szty(t) * SZINT / SZCHAR;
		}
#ifdef os_win32
		/*
		 * mangle name in symbol table as a callee.
		 */
		if ((name = cftnsp->soname) == NULL)
			name = exname(cftnsp->sname);
		snprintf(buf, 256, "%s@%d", name, argstacksize);
		cftnsp->soname = addname(buf);
#endif
	}

	if (kflag) {
#define	STL	200
		char *str = inlalloc(STL);
#if !defined(MACHOABI)
		int l = getlab();
#else
		char *name;
#endif

		/* Generate extended assembler for PIC prolog */
		p = tempnode(0, INT, 0, 0);
		gotnr = regno(p);
		p = block(XARG, p, NIL, INT, 0, 0);
		p->n_name = "=g";
		p = block(XASM, p, bcon(0), INT, 0, 0);

#if defined(MACHOABI)
		if ((name = cftnsp->soname) == NULL)
			name = cftnsp->sname;
		if (snprintf(str, STL, "call L%s$pb\nL%s$pb:\n\tpopl %%0\n",
		    name, name) >= STL)
			cerror("bfcode");
#else
		if (snprintf(str, STL,
		    "call " LABFMT "\n" LABFMT ":\n	popl %%0\n"
		    "	addl $_GLOBAL_OFFSET_TABLE_+[.-" LABFMT "], %%0\n",
		    l, l, l) >= STL)
			cerror("bfcode");
#endif
		p->n_name = str;
		p->n_right->n_type = STRTY;
		ecomp(p);
	}
	if (xtemps == 0)
		return;

	/* put arguments in temporaries */
	for (i = 0; i < cnt; i++) {
		if (sp[i]->stype == STRTY || sp[i]->stype == UNIONTY ||
		    cisreg(sp[i]->stype) == 0)
			continue;
		if (cqual(sp[i]->stype, sp[i]->squal) & VOL)
			continue;
		sp2 = sp[i];
		n = tempnode(0, sp[i]->stype, sp[i]->sdf, sp[i]->sap);
		n = buildtree(ASSIGN, n, nametree(sp2));
		sp[i]->soffset = regno(n->n_left);
		sp[i]->sflags |= STNODE;
		ecomp(n);
	}
}


#if defined(MACHOABI)
struct stub stublist;
struct stub nlplist;
#endif

/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag )
{
#if defined(MACHOABI)
	/*
	 * iterate over the stublist and output the PIC stubs
`	 */
	if (kflag) {
		struct stub *p;

		DLIST_FOREACH(p, &stublist, link) {
			printf("\t.section __IMPORT,__jump_table,symbol_stubs,self_modifying_code+pure_instructions,5\n");
			printf("L%s$stub:\n", p->name);
			printf("\t.indirect_symbol %s\n", p->name);
			printf("\thlt ; hlt ; hlt ; hlt ; hlt\n");
			printf("\t.subsections_via_symbols\n");
		}

		printf("\t.section __IMPORT,__pointers,non_lazy_symbol_pointers\n");
		DLIST_FOREACH(p, &nlplist, link) {
			printf("L%s$non_lazy_ptr:\n", p->name);
			printf("\t.indirect_symbol %s\n", p->name);
			printf("\t.long 0\n");
	        }

	}
#endif

	printf("\t.ident \"PCC: %s\"\n", VERSSTR);
}

void
bjobcode()
{
#ifdef os_sunos
	astypnames[SHORT] = astypnames[USHORT] = "\t.2byte";
#endif
	astypnames[INT] = astypnames[UNSIGNED] = "\t.long";
#if defined(MACHOABI)
	DLIST_INIT(&stublist, link);
	DLIST_INIT(&nlplist, link);
#endif
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */
NODE *
funcode(NODE *p)
{
	extern int gotnr;
	NODE *r, *l;

	/* Fix function call arguments. On x86, just add funarg */
	for (r = p->n_right; r->n_op == CM; r = r->n_left) {
		if (r->n_right->n_op != STARG)
			r->n_right = block(FUNARG, r->n_right, NIL,
			    r->n_right->n_type, r->n_right->n_df,
			    r->n_right->n_ap);
	}
	if (r->n_op != STARG) {
		l = talloc();
		*l = *r;
		r->n_op = FUNARG;
		r->n_left = l;
		r->n_type = l->n_type;
	}
	if (kflag == 0)
		return p;
#if defined(ELFABI)
	/* Create an ASSIGN node for ebx */
	l = block(REG, NIL, NIL, INT, 0, 0);
	l->n_rval = EBX;
	l = buildtree(ASSIGN, l, tempnode(gotnr, INT, 0, 0));
	if (p->n_right->n_op != CM) {
		p->n_right = block(CM, l, p->n_right, INT, 0, 0);
	} else {
		for (r = p->n_right; r->n_left->n_op == CM; r = r->n_left)
			;
		r->n_left = block(CM, l, r->n_left, INT, 0, 0);
	}
#endif
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
