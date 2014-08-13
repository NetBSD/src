/*	Id: code.c,v 1.74 2014/07/03 14:03:50 ragge Exp 	*/	
/*	$NetBSD: code.c,v 1.2 2014/08/13 13:18:08 plunky Exp $	*/
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
enum { INTEGER = 1, INTMEM, SSE, SSEMEM, X87,
	STRREG, STRMEM, STRSSE, STRIF, STRFI, STRX87 };
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

static int stroffset;

static int varneeds;
#define	NEED_1FPREF	 001
#define	NEED_2FPREF	 002
#define	NEED_1REGREF	 004
#define	NEED_2REGREF	 010
#define	NEED_MEMREF	 020
#define	NEED_STRFI	 040
#define	NEED_STRIF	0100

static int argtyp(TWORD t, union dimfun *df, struct attr *ap);
static NODE *movtomem(NODE *p, int off, int reg);
static NODE *movtoreg(NODE *p, int rno);
void varattrib(char *name, struct attr *sap);

/*
 * Print out assembler segment name.
 */
#ifdef MACHOABI
void
setseg(int seg, char *name)
{
	switch (seg) {
	case PROG: name = ".text"; break;
	case DATA:
	case LDATA: name = ".data"; break;
	case RDATA: name = ".const"; break;
	case STRNG: name = ".cstring"; break;
	case UDATA: break;
	case CTORS: name = ".mod_init_func"; break;
	case DTORS: name = ".mod_term_func"; break;
	default:
		cerror("unknown seg %d", seg);
	}
	printf("\t%s\n", name);
}

#else
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
		printf("\t.globl %s\n", name);
#ifndef MACHOABI
		if (ISFTN(sp->stype)) {
			printf("\t.type %s,@function\n", name);
		} else {
			printf("\t.type %s,@object\n", name);
			printf("\t.size %s,%d\n", name,
			    (int)tsize(sp->stype, sp->sdf, sp->sap)/SZCHAR);
		}
#endif
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
void
efcode(void)
{
	struct symtab *sp;
	extern int gotnr;
	TWORD t;
	NODE *p, *r, *l;
	int typ;

	gotnr = 0;	/* new number for next fun */
	sp = cftnsp;
	t = DECREF(sp->stype);
	if (t != STRTY && t != UNIONTY)
		return;

	/* XXX should have one routine for this */
	ngpr = nsse = 0;
	typ = argtyp(t, sp->sdf, sp->sap);
	if (typ == STRMEM) {
		r = block(REG, NIL, NIL, INCREF(t), sp->sdf, sp->sap);
		regno(r) = RAX;
		r = buildtree(UMUL, r, NIL);
		l = tempnode(stroffset, INCREF(t), sp->sdf, sp->sap);
		l = buildtree(UMUL, l, NIL);
		ecomp(buildtree(ASSIGN, l, r));
		l = block(REG, NIL, NIL, LONG, 0, 0);
		regno(l) = RAX;
		r = tempnode(stroffset, LONG, 0, 0);
		ecomp(buildtree(ASSIGN, l, r));
	} else if (typ == STRX87) {
		p = block(REG, NIL, NIL, INCREF(LDOUBLE), 0, 0);
		regno(p) = RAX;
		p = buildtree(UMUL, buildtree(PLUS, p, bcon(1)), NIL);
		ecomp(movtoreg(p, 041));
		p = block(REG, NIL, NIL, INCREF(LDOUBLE), 0, 0);
		regno(p) = RAX;
		p = buildtree(UMUL, p, NIL);
		ecomp(movtoreg(p, 040));
	} else {
		TWORD t1, t2;
		int r1, r2;
		if (typ == STRSSE || typ == STRFI)
			r1 = XMM0, t1 = DOUBLE;
		else
			r1 = RAX, t1 = LONG;
		if (typ == STRSSE || typ == STRIF)
			r2 = XMM1, t2 = DOUBLE;
		else
			r2 = RDX, t2 = LONG;

		if (tsize(t, sp->sdf, sp->sap) > SZLONG) {
			p = block(REG, NIL, NIL, INCREF(t2), 0, 0);
			regno(p) = RAX;
			p = buildtree(UMUL, buildtree(PLUS, p, bcon(1)), NIL);
			ecomp(movtoreg(p, r2));
		}
		p = block(REG, NIL, NIL, INCREF(t1), 0, 0);
		regno(p) = RAX;
		p = buildtree(UMUL, p, NIL);
		ecomp(movtoreg(p, r1));
	}
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
	TWORD t;
	int i, rno, typ, ssz;

	/* recalculate the arg offset and create TEMP moves */
	/* Always do this for reg, even if not optimizing, to free arg regs */
	nsse = ngpr = 0;
	nrsp = ARGINIT;
	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		sp = cftnsp;
		if (argtyp(DECREF(sp->stype), sp->sdf, sp->sap) == STRMEM) {
			r = block(REG, NIL, NIL, LONG, 0, 0);
			regno(r) = argregsi[ngpr++];
			p = tempnode(0, r->n_type, r->n_df, r->n_ap);
			stroffset = regno(p);
			ecomp(buildtree(ASSIGN, p, r));
		}
	}

	for (i = 0; i < cnt; i++) {
		sp = s[i];

		if (sp == NULL)
			continue; /* XXX when happens this? */

		ssz = tsize(sp->stype, sp->sdf, sp->sap);
		switch (typ = argtyp(sp->stype, sp->sdf, sp->sap)) {
		case INTEGER:
		case SSE:
			if (typ == SSE)
				rno = XMM0 + nsse++;
			else
				rno = argregsi[ngpr++];
			r = block(REG, NIL, NIL, sp->stype, sp->sdf, sp->sap);
			regno(r) = rno;
			p = tempnode(0, sp->stype, sp->sdf, sp->sap);
			sp->soffset = regno(p);
			sp->sflags |= STNODE;
			ecomp(buildtree(ASSIGN, p, r));
			break;

		case SSEMEM:
			sp->soffset = nrsp;
			nrsp += SZDOUBLE;
			if (xtemps) {
				p = tempnode(0, sp->stype, sp->sdf, sp->sap);
				p = buildtree(ASSIGN, p, nametree(sp));
				sp->soffset = regno(p->n_left);
				sp->sflags |= STNODE;
				ecomp(p);
			}
			break;

		case INTMEM:
			sp->soffset = nrsp;
			nrsp += SZLONG;
			if (xtemps) {
				p = tempnode(0, sp->stype, sp->sdf, sp->sap);
				p = buildtree(ASSIGN, p, nametree(sp));
				sp->soffset = regno(p->n_left);
				sp->sflags |= STNODE;
				ecomp(p);
			}
			break;

		case STRX87:
		case STRMEM: /* Struct in memory */
			sp->soffset = nrsp;
			nrsp += ssz;
			break;

		case X87: /* long double args */
			sp->soffset = nrsp;
			nrsp += SZLDOUBLE;
			break;

		case STRFI:
		case STRIF:
		case STRSSE:
		case STRREG: /* Struct in register */
			autooff += (2*SZLONG);

			if (typ == STRSSE || typ == STRFI) {
				rno = XMM0 + nsse++;
				t = DOUBLE;
			} else {
				rno = argregsi[ngpr++];
				t = LONG;
			}
			r = block(REG, NIL, NIL, t, 0, 0);
			regno(r) = rno;
			ecomp(movtomem(r, -autooff, FPREG));

			if (ssz > SZLONG) {
				if (typ == STRSSE || typ == STRIF) {
					rno = XMM0 + nsse++;
					t = DOUBLE;
				} else {
					rno = argregsi[ngpr++];
					t = LONG;
				}
				r = block(REG, NIL, NIL, t, 0, 0);
				regno(r) = rno;
				ecomp(movtomem(r, -autooff+SZLONG, FPREG));
			}
			sp->soffset = -autooff;
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
		t = al->type;
		if (t == TNULL)
			return;
		if (ISSOU(BTYPE(t)))
			al++;
		for (i = 0; t > BTMASK; t = DECREF(t))
			if (ISARY(t) || ISFTN(t))
				i++;
		if (i)
			al++;
	}

	/* fix stack offset */
	SETOFF(autooff, ALMAX);

	/* Save reg arguments in the reg save area */
	p = NIL;
	for (i = ngpr; i < 6; i++) {
		r = block(REG, NIL, NIL, LONG, 0, 0);
		regno(r) = argregsi[i];
		r = movtomem(r, -RSALONGOFF(i)-autooff, FPREG);
		p = (p == NIL ? r : block(COMOP, p, r, INT, 0, 0));
	}
	for (i = nsse; i < 8; i++) {
		r = block(REG, NIL, NIL, DOUBLE, 0, 0);
		regno(r) = i + XMM0;
		r = movtomem(r, -RSADBLOFF(i)-autooff, FPREG);
		p = (p == NIL ? r : block(COMOP, p, r, INT, 0, 0));
	}
	autooff += RSASZ;
	rsaoff = autooff;
	thissse = nsse;
	thisgpr = ngpr;
	thisrsp = nrsp;

	ecomp(p);
}


/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag)
{
	if (flag)
		return;

#ifdef MACHOABI
#define PT(x)
#else
#define	PT(x) printf(".type __pcc_" x ",@function\n")
#endif

#define	P(x) printf(x "\n")
	/* printout varargs routines if used */
	if (varneeds & NEED_STRFI) {	/* struct with one float and then int */
		P(".text\n.align 4");
		PT("strif");
		P("__pcc_strif:");
		P("cmpl $176,4(%%rdi)\njae .Ladd16");
		P("cmpl $48,(%%rdi)\njae .Ladd16\n");
		P("movl 4(%%rdi),%%eax\naddq 16(%%rdi),%%rax");
		P("movq (%%rax),%%rdx\nmovq %%rdx,24(%%rdi)");
		P("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax");
		P("movq 16(%%rax),%%rdx\nmovq %%rdx,32(%%rdi)");
		P("leaq 24(%%rdi),%%rax\nret");
	}
	if (varneeds & NEED_STRIF) {	/* struct with one int and one float */
		P(".text\n.align 4");
		PT("strif");
		P("__pcc_strif:");
		P("cmpl $176,4(%%rdi)\njae .Ladd16");
		P("cmpl $48,(%%rdi)\njae .Ladd16\n");
		P("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax");
		P("movq (%%rax),%%rdx\nmovq %%rdx,24(%%rdi)");
		P("movl 4(%%rdi),%%eax\naddq 16(%%rdi),%%rax");
		P("movq 16(%%rax),%%rdx\nmovq %%rdx,32(%%rdi)");
		P("leaq 24(%%rdi),%%rax\nret");
	}
	if (varneeds & NEED_2FPREF) {	/* struct with two float regs */
		P(".text\n.align 4");
		PT("2fpref");
		P("__pcc_2fpref:");
		P("cmpl $160,4(%%rdi)\njae .Ladd16");
		P("movl 4(%%rdi),%%eax\naddq 16(%%rdi),%%rax");
		P("addl $32,4(%%rdi)");
		P("movq (%%rax),%%rdx\nmovq %%rdx,24(%%rdi)");
		P("movq 16(%%rax),%%rdx\nmovq %%rdx,32(%%rdi)");
		P("leaq 24(%%rdi),%%rax\nret");
	}
	if (varneeds & NEED_1FPREF) {
		printf(".text\n.align 4\n");
		PT("1fpref");
		printf("__pcc_1fpref:\n");
		printf("cmpl $176,4(%%rdi)\njae .Ladd8\n");
		printf("movl 4(%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("addl $16,4(%%rdi)\nret\n");
	}
	if (varneeds & NEED_1REGREF) {
		printf(".text\n.align 4\n");
		PT("1regref");
		printf("__pcc_1regref:\n");
		printf("cmpl $48,(%%rdi)\njae .Ladd8\n");
		printf("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("addl $8,(%%rdi)\nret\n");
	}
	if (varneeds & NEED_2REGREF) {
		printf(".text\n.align 4\n");
		PT("2regref");
		printf("__pcc_2regref:\n");
		printf("cmpl $40,(%%rdi)\njae .Ladd16\n");
		printf("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("addl $16,(%%rdi)\nret\n");
	}
	if (varneeds & NEED_MEMREF) {
		printf(".text\n.align 4\n");
		PT("memref");
		printf("__pcc_memref:\n");
		printf("movq 8(%%rdi),%%rax\n");
		printf("addq %%rsi,8(%%rdi)\nret\n");
	}

	if (varneeds & (NEED_1FPREF|NEED_1REGREF)) {
		P(".Ladd8:");
		P("movq 8(%%rdi),%%rax");
		P("addq $8,8(%%rdi)");
		P("ret");
	}
	if (varneeds & (NEED_2FPREF|NEED_2REGREF|NEED_STRFI|NEED_STRIF)) {
		P(".Ladd16:");
		P("movq 8(%%rdi),%%rax");
		P("addq $16,8(%%rdi)");
		P("ret");
	}

#ifdef MACHOABI
	printf("\t.ident \"PCC: %s\"\n", VERSSTR);
#else
	printf("\t.ident \"PCC: %s\"\n\t.end\n", VERSSTR);
#endif
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
 *
 * ...actually, we allocate two of them and use the second one as 
 * bounce buffers for floating point structs...
 *
 * There are a number of asm routines printed out if varargs are used:
 *	long __pcc_gpnext(va)	- get a gpreg value
 *	long __pcc_fpnext(va)	- get a fpreg value
 *	void *__pcc_1regref(va)	- get reference to a onereg struct 
 *	void *__pcc_2regref(va)	- get reference to a tworeg struct 
 *	void *__pcc_memref(va,sz)	- get reference to a large struct 
 */

static char *gp_offset, *fp_offset, *overflow_arg_area, *reg_save_area;
static char *_1fpref, *_2fpref, *_1regref, *_2regref, *memref;
static char *strif, *strfi;

void
bjobcode(void)
{
	struct symtab *sp;
	struct rstack *rp;
	NODE *p, *q;
	char *c;

	/* amd64 names for some asm constant printouts */
	astypnames[INT] = astypnames[UNSIGNED] = "\t.long";
	astypnames[LONG] = astypnames[ULONG] = "\t.quad";

	gp_offset = addname("gp_offset");
	fp_offset = addname("fp_offset");
	overflow_arg_area = addname("overflow_arg_area");
	reg_save_area = addname("reg_save_area");

	rp = bstruct(NULL, STNAME, NULL);
	p = block(NAME, NIL, NIL, UNSIGNED, 0, 0);
	soumemb(p, gp_offset, 0);
	soumemb(p, fp_offset, 0);
	p->n_type = VOID+PTR;
	p->n_ap = NULL;
	soumemb(p, overflow_arg_area, 0);
	soumemb(p, reg_save_area, 0);
	nfree(p);
	q = dclstruct(rp);
	c = addname("__builtin_va_list");
	p = block(LB, bdty(NAME, c), bcon(2), INT, 0, 0);
	p = tymerge(q, p);
	p->n_sp = lookup(c, 0);
	defid(p, TYPEDEF);
	nfree(q);
	nfree(p);

	/* for the static varargs functions */
#define	MKN(vn, rn) \
	{ vn = addname(rn); sp = lookup(vn, SNORMAL); \
	  sp->sclass = USTATIC; sp->stype = FTN|VOID|(PTR<<TSHIFT); }

	MKN(strfi, "__pcc_strfi");
	MKN(strif, "__pcc_strif");
	MKN(_1fpref, "__pcc_1fpref");
	MKN(_2fpref, "__pcc_2fpref");
	MKN(_1regref, "__pcc_1regref");
	MKN(_2regref, "__pcc_2regref");
	MKN(memref, "__pcc_memref");
}

static NODE *
mkstkref(int off, TWORD typ)
{
	NODE *p;

	p = block(REG, NIL, NIL, PTR|typ, 0, 0);
	regno(p) = FPREG;
	return buildtree(PLUS, p, bcon(off/SZCHAR));
}

NODE *
amd64_builtin_stdarg_start(const struct bitable *bt, NODE *a)
{
	NODE *p, *r;

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

	tfree(a);
	return r;
}

static NODE *
mkvacall(char *fun, NODE *a, int typ)
{
	NODE *r, *f = block(NAME, NIL, NIL, INT, 0, 0);
	NODE *ap = a->n_left;
	NODE *dp = a->n_right;
	int sz = tsize(dp->n_type, dp->n_df, dp->n_ap);

	f->n_sp = lookup(fun, SNORMAL);
	varneeds |= typ;
	f->n_type = f->n_sp->stype;
	f = clocal(f);
	SETOFF(sz, ALLONG);
	r = buildtree(CALL, f,
	    buildtree(CM, ccopy(ap), bcon(sz/SZCHAR)));
	r = ccast(r, INCREF(dp->n_type), 0, dp->n_df, dp->n_ap);
	r = buildtree(UMUL, r, NIL);
	return r;
}

NODE *
amd64_builtin_va_arg(const struct bitable *bt, NODE *a)
{
	NODE *r, *dp;
	int typ, sz;

	dp = a->n_right;

	nsse = ngpr = 0;
	sz = tsize(dp->n_type, dp->n_df, dp->n_ap);
	switch (typ = argtyp(dp->n_type, dp->n_df, dp->n_ap)) {
	case INTEGER:
		r = mkvacall(_1regref, a, NEED_1REGREF);
		break;

	case SSE:
		r = mkvacall(_1fpref, a, NEED_1FPREF);
		break;

	default:
		cerror("va_arg: bad type %d", typ);

	case X87:
	case STRX87:
	case STRMEM: /* stored in memory */
		r = mkvacall(memref, a, NEED_MEMREF);
		break;

	case STRREG: /* struct in general regs */
		if (sz <= SZLONG)
			r = mkvacall(_1regref, a, NEED_1REGREF);
		else
			r = mkvacall(_2regref, a, NEED_2REGREF);
		break;

	case STRSSE:
		if (sz <= SZLONG)
			r = mkvacall(_1fpref, a, NEED_1FPREF);
		else
			r = mkvacall(_2fpref, a, NEED_2FPREF);
		break;

	case STRIF:
		r = mkvacall(strif, a, NEED_STRIF);
		break;

	case STRFI:
		r = mkvacall(strfi, a, NEED_STRFI);
		break;
	}

	tfree(a);
	return r;
}

NODE *
amd64_builtin_va_end(const struct bitable *bt, NODE *a)
{
	tfree(a);
	return bcon(0); /* nothing */
}

NODE *
amd64_builtin_va_copy(const struct bitable *bt, NODE *a)
{
	NODE *f;

	f = buildtree(ASSIGN, buildtree(UMUL, a->n_left, NIL),
	    buildtree(UMUL, a->n_right, NIL));
	nfree(a);
	return f;
}

static NODE *
movtoreg(NODE *p, int rno)
{
	NODE *r;

	r = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_ap);
	regno(r) = rno;
	return clocal(buildtree(ASSIGN, r, p));
}  

static NODE *
movtomem(NODE *p, int off, int reg)
{
	struct symtab s;
	NODE *r, *l;

	s.stype = p->n_type;
	s.squal = 0;
	s.sdf = p->n_df;
	s.sap = p->n_ap;
	s.soffset = off;
	s.sclass = AUTO;

	l = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
	l->n_lval = 0;
	regno(l) = reg;

	r = block(NAME, NIL, NIL, p->n_type, p->n_df, p->n_ap);
	r->n_sp = &s;
	r = stref(block(STREF, l, r, 0, 0, 0));

	return clocal(buildtree(ASSIGN, r, p));
}  

/*
 * Check what to do with a struct.  We traverse down in the struct to 
 * find which types it is and where the struct really should be.
 * The return vals we may end up with are:
 *	STRREG - The whole struct is saved in general registers.
 *	STRMEM - the struct is saved in memory.
 *	STRSSE - the whole struct is saved in SSE registers.
 *	STRIF  - First word of struct is saved in general reg, other SSE.
 *	STRFI  - First word of struct is saved in SSE, next in general reg.
 *
 * - If size > 16 bytes or there are packed fields, use memory.
 * - If any part of an eight-byte should be in a general register,
 *    the eight-byte is stored in a general register
 * - If the eight-byte only contains float or double, use a SSE register
 * - Otherwise use memory.
 *
 * sp below is a pointer to a member list.
 * off tells whether is is the first or second eight-byte to check.
 */
static int
classifystruct(struct symtab *sp, int off)
{
	TWORD t;
	int cl, cl2;

	for (cl = 0; sp; sp = sp->snext) {
		t = sp->stype;

		if (off == 0) {
			if (sp->soffset >= SZLONG)
				continue;
		} else {
			if (sp->soffset < SZLONG)
				continue;
		}

		if (t <= ULONGLONG || ISPTR(t)) {
			if (cl == 0 || cl == STRSSE)
				cl = STRREG;
		} else if (t <= DOUBLE) {
			if (cl == 0)
				cl = STRSSE;
		} else if (t == LDOUBLE) {
			return STRMEM;
		} else if (ISSOU(t)) {
			if (attr_find(sp->sap, GCC_ATYP_PACKED)) {
				cl = STRMEM;
			} else {
				cl2 = classifystruct(strmemb(sp->sap), off);
				if (cl2 == STRMEM) {
					cl = STRMEM;
				} else if (cl2 == STRREG) {
					if (cl == 0 || cl == STRSSE)
						cl = STRREG;
				} else if (cl2 == STRSSE) {
					if (cl == 0)
						cl = STRSSE;
				}
			}
		} else
			cerror("classifystruct: unknown type %x", t);
		if (cl == STRMEM)
			break;
	}
	if (cl == 0)
		cerror("classifystruct: failed classify");
	return cl;
}

/*
 * Check for long double complex structs.
 */
static int
iscplx87(struct symtab *sp)
{
	if (sp->stype == LDOUBLE && sp->snext->stype == LDOUBLE &&
	    sp->snext->snext == NULL)
		return STRX87;
	return 0;
}

/*
 * AMD64 parameter classification.
 */
static int
argtyp(TWORD t, union dimfun *df, struct attr *ap)
{
	int cl2, cl = 0;

	if (t <= ULONG || ISPTR(t) || t == BOOL) {
		cl = ngpr < 6 ? INTEGER : INTMEM;
	} else if (t == FLOAT || t == DOUBLE || t == FIMAG || t == IMAG) {
		cl = nsse < 8 ? SSE : SSEMEM;
	} else if (t == LDOUBLE || t == LIMAG) {
		cl = X87; /* XXX */
	} else if (t == STRTY || t == UNIONTY) {
		int sz = tsize(t, df, ap);

		if (attr_find(ap, GCC_ATYP_PACKED)) {
			cl = STRMEM;
		} else if (iscplx87(strmemb(ap)) == STRX87) {
			cl = STRX87;
		} else if (sz > 2*SZLONG) {
			cl = STRMEM;
		} else if (sz <= SZLONG) {
			/* only one member to check */
			cl = classifystruct(strmemb(ap), 0);
			if (cl == STRREG && ngpr > 5)
				cl = STRMEM;
			else if (cl == STRSSE && nsse > 7)
				cl = STRMEM;
		} else {
			cl = classifystruct(strmemb(ap), 0);
			cl2 = classifystruct(strmemb(ap), 1);
			if (cl == STRMEM || cl2 == STRMEM)
				cl = STRMEM;
			else if (cl == STRREG && cl2 == STRSSE)
				cl = STRIF;
			else if (cl2 == STRREG && cl == STRSSE)
				cl = STRFI;

			if (cl == STRREG && ngpr > 4)
				cl = STRMEM;
			else if (cl == STRSSE && nsse > 6)
				cl = STRMEM;
			else if ((cl == STRIF || cl == STRFI) &&
			    (ngpr > 5 || nsse > 7))
				cl = STRMEM;
		}
	} else
		cerror("FIXME: classify");
	return cl;
}

/*
 * Do the "hard work" in assigning correct destination for arguments.
 * Also convert arguments < INT to inte (default argument promotions).
 * XXX - should be dome elsewhere.
 */
static NODE *
argput(NODE *p)
{
	NODE *q, *ql;
	TWORD ty;
	int typ, r, ssz, rn;

	if (p->n_op == CM) {
		p->n_left = argput(p->n_left);
		p->n_right = argput(p->n_right);
		return p;
	}

	/* first arg may be struct return pointer */
	/* XXX - check if varargs; setup al */
	switch (typ = argtyp(p->n_type, p->n_df, p->n_ap)) {
	case INTEGER:
	case SSE:
		if (typ == SSE)
			r = XMM0 + nsse++;
		else
			r = argregsi[ngpr++];
		if (p->n_type < INT || p->n_type == BOOL)
			p = cast(p, INT, 0);
		p = movtoreg(p, r);
		break;

	case X87:
		r = nrsp;
		nrsp += SZLDOUBLE;
		p = movtomem(p, r, STKREG);
		break;

	case SSEMEM:
		r = nrsp;
		nrsp += SZDOUBLE;
		p = movtomem(p, r, STKREG);
		break;

	case INTMEM:
		r = nrsp;
		nrsp += SZLONG;
		p = movtomem(p, r, STKREG);
		break;

	case STRFI:
	case STRIF:
	case STRSSE:
	case STRREG: /* Struct in registers */
		/* Cast to long/sse pointer and move to the registers */
		/* XXX can overrun struct size */
		ssz = tsize(p->n_type, p->n_df, p->n_ap);

		if (typ == STRSSE || typ == STRFI) {
			r = XMM0 + nsse++;
			ty = DOUBLE;
		} else {
			r = argregsi[ngpr++];
			ty = LONG;
		}

		p = nfree(p);	/* remove STARG */
		p = makety(p, PTR|ty, 0, 0, 0);
		ql = tempnode(0, PTR|ty, 0, 0);
		rn = regno(ql);
		p = buildtree(ASSIGN, ql, p);
		ql = tempnode(rn, PTR|ty, 0, 0);
		ql = movtoreg(buildtree(UMUL, ql, NIL), r);
		p = buildtree(COMOP, p, ql);

		if (ssz > SZLONG) {
			if (typ == STRSSE || typ == STRIF) {
				r = XMM0 + nsse++;
				ty = DOUBLE;
			} else {
				r = argregsi[ngpr++];
				ty = LONG;
			}

			ql = tempnode(rn, PTR|ty, 0, 0);
			ql = buildtree(UMUL, buildtree(PLUS, ql, bcon(1)), NIL);
			ql = movtoreg(ql, r);

			p = buildtree(CM, p, ql);
		}
		break;

	case STRX87:
	case STRMEM: {
		struct symtab s;
		NODE *l, *t;

		q = buildtree(UMUL, p->n_left, NIL);

		s.stype = p->n_type;
		s.squal = 0;
		s.sdf = p->n_df;
		s.sap = p->n_ap;
		s.soffset = nrsp;
		s.sclass = AUTO;

		nrsp += tsize(p->n_type, p->n_df, p->n_ap);

		l = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
		l->n_lval = 0;
		regno(l) = STKREG;

		t = block(NAME, NIL, NIL, p->n_type, p->n_df, p->n_ap);
		t->n_sp = &s;
		t = stref(block(STREF, l, t, 0, 0, 0));

		t = (buildtree(ASSIGN, t, q));
		nfree(p);
		p = t->n_left;
		nfree(t);
		break;
		}

	default:
		cerror("argument %d", typ);
	}
	return p;
}

/*
 * Sort arglist so that register assignments ends up last.
 */
static int
argsort(NODE *p)
{
	NODE *q, *r;
	int rv = 0;

	if (p->n_op != CM) {
		if (p->n_op == ASSIGN && p->n_left->n_op == REG &&
		    coptype(p->n_right->n_op) != LTYPE) {
			q = tempnode(0, p->n_type, p->n_df, p->n_ap);
			r = ccopy(q);
			p->n_right = buildtree(COMOP,
			    buildtree(ASSIGN, q, p->n_right), r);
		}
		return rv;
	}
	if (p->n_right->n_op == CM) {
		/* fixup for small structs in regs */
		q = p->n_right->n_left;
		p->n_right->n_left = p->n_left;
		p->n_left = p->n_right;
		p->n_right = p->n_left->n_right;
		p->n_left->n_right = q;
	}
	if (p->n_right->n_op == ASSIGN && p->n_right->n_left->n_op == REG &&
	    coptype(p->n_right->n_right->n_op) != LTYPE) {
		/* move before everything to avoid reg trashing */
		q = tempnode(0, p->n_right->n_type,
		    p->n_right->n_df, p->n_right->n_ap);
		r = ccopy(q);
		p->n_right->n_right = buildtree(COMOP,
		    buildtree(ASSIGN, q, p->n_right->n_right), r);
	}
	if (p->n_right->n_op == ASSIGN && p->n_right->n_left->n_op == REG) {
		if (p->n_left->n_op == CM &&
		    p->n_left->n_right->n_op == STASG) {
			q = p->n_left->n_right;
			p->n_left->n_right = p->n_right;
			p->n_right = q;
			rv = 1;
		} else if (p->n_left->n_op == STASG) {
			q = p->n_left;
			p->n_left = p->n_right;
			p->n_right = q;
			rv = 1;
		}
	}
	return rv | argsort(p->n_left);
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
	TWORD t;
	int i;

	nsse = ngpr = nrsp = 0;
	/* Check if hidden arg needed */
	/* If so, add it in pass2 */
	if ((l = p->n_left)->n_type == INCREF(FTN)+STRTY ||
	    l->n_type == INCREF(FTN)+UNIONTY) {
		int ssz = tsize(BTYPE(l->n_type), l->n_df, l->n_ap);
		struct symtab *sp = strmemb(l->n_ap);
		if (ssz == 2*SZLDOUBLE && sp->stype == LDOUBLE &&
		    sp->snext->stype == LDOUBLE)
			; /* long complex struct */
		else if (ssz > 2*SZLONG)
			ngpr++;
	}

	/* Convert just regs to assign insn's */
	p->n_right = argput(p->n_right);

	/* Must sort arglist so that STASG ends up first */
	/* This avoids registers being clobbered */
	while (argsort(p->n_right))
		;
	/* Check if there are varargs */
	if (nsse || l->n_df == NULL || l->n_df->dfun == NULL) {
		; /* Need RAX */
	} else {
		union arglist *al = l->n_df->dfun;

		for (; al->type != TELLIPSIS; al++) {
			if ((t = al->type) == TNULL)
				return p; /* No need */
			if (ISSOU(BTYPE(t)))
				al++;
			for (i = 0; t > BTMASK; t = DECREF(t))
				if (ISARY(t) || ISFTN(t))
					i++;
			if (i)
				al++;
		}
	}

	/* Always emit number of SSE regs used */
	l = movtoreg(bcon(nsse), RAX);
	if (p->n_right->n_op != CM) {
		p->n_right = block(CM, l, p->n_right, INT, 0, 0);
	} else {
		for (r = p->n_right; r->n_left->n_op == CM; r = r->n_left)
			;
		r->n_left = block(CM, l, r->n_left, INT, 0, 0);
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

	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = FPREG;
	return block(PLUS, f, bcon(16), INCREF(PTR+VOID), 0, 0);
}

int codeatyp(NODE *);
int
codeatyp(NODE *p)
{
	int typ;

	ngpr = nsse = 0;
	typ = argtyp(DECREF(p->n_type), p->n_df, p->n_ap);
	return typ;
}
