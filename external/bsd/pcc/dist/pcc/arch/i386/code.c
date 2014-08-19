/*	Id: code.c,v 1.80 2014/06/04 07:18:02 gmcgarry Exp 	*/	
/*	$NetBSD: code.c,v 1.1.1.4.8.1 2014/08/19 23:52:08 tls Exp $	*/
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
#ifdef MACHOABI
	case CTORS: name = ".mod_init_func\n\t.align 2"; break;
	case DTORS: name = ".mod_term_func\n\t.align 2"; break;
#else
	case CTORS: name = ".section\t.ctors,\"aw\",@progbits"; break;
	case DTORS: name = ".section\t.dtors,\"aw\",@progbits"; break;
#endif
	case NMSEG: 
		printf("\t.section %s,\"a%c\",@progbits\n", name,
		    cftnsp ? 'x' : 'w');
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

int structrettemp;

/*
 * code for the end of a function
 * deals with struct return here
 */
void
efcode(void)
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
	q = tempnode(structrettemp, PTR+STRTY, 0, cftnsp->sap);
	q = buildtree(UMUL, q, NIL);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);

	/* put hidden arg in eax on return */
	q = tempnode(structrettemp, INT, 0, 0);
	p = block(REG, NIL, NIL, INT, 0, 0);
	regno(p) = EAX;
	ecomp(buildtree(ASSIGN, p, q));
}

static TWORD longregs[] = { EAXEDX, EDXECX };
static TWORD regpregs[] = { EAX, EDX, ECX };
static TWORD charregs[] = { AL, DL, CL };

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 *
 * Classifying args on i386; not simple:
 * - Args may be on stack or in registers (regparm)
 * - There may be a hidden first arg, unless OpenBSD struct return.
 * - Regparm syntax is not well documented.
 * - There may be stdcall functions, where the called function pops stack
 * - ...probably more
 */
void
bfcode(struct symtab **sp, int cnt)
{
	extern int argstacksize;
#ifdef GCC_COMPAT
	struct attr *ap;
#endif
	struct symtab *sp2;
	extern int gotnr;
	NODE *n, *p;
	int i, regparmarg;
	int argbase, nrarg, sz;

	/* Take care of PIC stuff first */
        if (kflag) {
#define STL     200
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
                    "call " LABFMT "\n" LABFMT ":\n\tpopl %%0\n"
                    "\taddl $_GLOBAL_OFFSET_TABLE_+[.-" LABFMT "], %%0\n",
                    l, l, l) >= STL)
                        cerror("bfcode");
#endif
                p->n_name = str;
                p->n_right->n_type = STRTY;
                ecomp(p);
        }

	argbase = ARGINIT;
	nrarg = regparmarg = 0;

#ifdef GCC_COMPAT
        if (attr_find(cftnsp->sap, GCC_ATYP_STDCALL) != NULL)
                cftnsp->sflags |= SSTDCALL;
        if ((ap = attr_find(cftnsp->sap, GCC_ATYP_REGPARM)))
                regparmarg = ap->iarg(0);
#endif

	/* Function returns struct, create return arg node */
	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
#if defined(os_openbsd)
		/* OpenBSD uses non-standard return for small structs */
		int sz = tsize(BTYPE(cftnsp->stype), cftnsp->sdf, cftnsp->sap);
		if (sz <= SZLONGLONG)
#endif
		{
			if (regparmarg) {
				n = block(REG, 0, 0, INT, 0, 0);
				regno(n) = regpregs[nrarg++];
			} else {
				n = block(OREG, 0, 0, INT, 0, 0);
				n->n_lval = argbase/SZCHAR;
				argbase += SZINT;
				regno(n) = FPREG;
			}
			p = tempnode(0, INT, 0, 0);
			structrettemp = regno(p);
			p = buildtree(ASSIGN, p, n);
			ecomp(p);
		}
	}

	/*
	 * Find where all params are so that they end up at the right place.
	 * At the same time recalculate their arg offset on stack.
	 * We also get the "pop size" for stdcall.
	 */
	for (i = 0; i < cnt; i++) {
		sp2 = sp[i];
		sz = tsize(sp2->stype, sp2->sdf, sp2->sap);

		SETOFF(sz, SZINT);

		if (cisreg(sp2->stype) == 0 ||
		    ((regparmarg - nrarg) * SZINT < sz)) {	/* not in reg */
			sp2->soffset = argbase;
			argbase += sz;
			nrarg = regparmarg;	/* no more in reg either */
		} else {					/* in reg */
			sp2->soffset = nrarg;
			nrarg += sz/SZINT;
			sp2->sclass = REGISTER;
		}
	}

	/*
	 * Now (argbase - ARGINIT) is used space on stack.
	 * Move (if necessary) the args to something new.
	 */
	for (i = 0; i < cnt; i++) {
		int reg, j;

		sp2 = sp[i];

		if (ISSOU(sp2->stype) && sp2->sclass == REGISTER) {
			/* must move to stack */
			sz = tsize(sp2->stype, sp2->sdf, sp2->sap);
			SETOFF(sz, SZINT);
			SETOFF(autooff, SZINT);
			reg = sp2->soffset;
			sp2->sclass = AUTO;
			sp2->soffset = NOOFFSET;
			oalloc(sp2, &autooff);
                        for (j = 0; j < sz/SZCHAR; j += 4) {
                                p = block(OREG, 0, 0, INT, 0, 0);
                                p->n_lval = sp2->soffset/SZCHAR + j;
                                regno(p) = FPREG;
                                n = block(REG, 0, 0, INT, 0, 0);
                                regno(n) = regpregs[reg++];
                                p = block(ASSIGN, p, n, INT, 0, 0);
                                ecomp(p);
                        }
		} else if (cisreg(sp2->stype) && !ISSOU(sp2->stype) &&
		    ((cqual(sp2->stype, sp2->squal) & VOL) == 0)) {
			/* just put rest in temps */
			if (sp2->sclass == REGISTER) {
				n = block(REG, 0, 0, sp2->stype,
				    sp2->sdf, sp2->sap);
				if (ISLONGLONG(sp2->stype))
					regno(n) = longregs[sp2->soffset];
				else if (DEUNSIGN(sp2->stype) == CHAR || sp2->stype == BOOL)
					regno(n) = charregs[sp2->soffset];
				else
					regno(n) = regpregs[sp2->soffset];
			} else {
                                n = block(OREG, 0, 0, sp2->stype,
				    sp2->sdf, sp2->sap);
                                n->n_lval = sp2->soffset/SZCHAR;
                                regno(n) = FPREG;
			}
			p = tempnode(0, sp2->stype, sp2->sdf, sp2->sap);
			sp2->soffset = regno(p);
			sp2->sflags |= STNODE;
			n = buildtree(ASSIGN, p, n);
			ecomp(n);
		}
	}

        argstacksize = 0;
        if (cftnsp->sflags & SSTDCALL) {
		argstacksize = (argbase - ARGINIT)/SZCHAR;
#ifdef os_win32

                char buf[256];
                char *name;
                /*
                 * mangle name in symbol table as a callee.
                 */
                if ((name = cftnsp->soname) == NULL)
                        name = exname(cftnsp->sname);
                snprintf(buf, 256, "%s@%d", name, argstacksize);
                cftnsp->soname = addname(buf);
#endif
        }

}

#if defined(MACHOABI)
struct stub stublist;
struct stub nlplist;
#endif

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
bjobcode(void)
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
 * Convert FUNARG to assign in case of regparm.
 */
static int regcvt, rparg;
static void
addreg(NODE *p)
{
	TWORD t;
	NODE *q;
	int sz, r;

	sz = tsize(p->n_type, p->n_df, p->n_ap)/SZCHAR;
	sz = (sz + 3) >> 2;	/* sz in regs */
	if ((regcvt+sz) > rparg) {
		regcvt = rparg;
		return;
	}
	if (sz > 2)
		uerror("cannot put struct in 3 regs (yet)");

	if (sz == 2)
		r = regcvt == 0 ? EAXEDX : EDXECX;
	else
		r = regcvt == 0 ? EAX : regcvt == 1 ? EDX : ECX;

	if (p->n_op == FUNARG) {
		/* at most 2 regs */
		if (p->n_type < INT) {
			p->n_left = ccast(p->n_left, INT, 0, 0, 0);
			p->n_type = INT;
		}

		p->n_op = ASSIGN;
		p->n_right = p->n_left;
	} else if (p->n_op == STARG) {
		/* convert to ptr, put in reg */
		q = p->n_left;
		t = sz == 2 ? LONGLONG : INT;
		q = cast(q, INCREF(t), 0);
		q = buildtree(UMUL, q, NIL);
		p->n_op = ASSIGN;
		p->n_type = t;
		p->n_right = q;
	} else
		cerror("addreg");
	p->n_left = block(REG, 0, 0, p->n_type, 0, 0);
	regno(p->n_left) = r;
	regcvt += sz;
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
#ifdef GCC_COMPAT
	struct attr *ap;
#endif
	NODE *r, *l;
	TWORD t = DECREF(DECREF(p->n_left->n_type));
	int stcall;

	stcall = ISSOU(t);
	/*
	 * We may have to prepend:
	 * - Hidden arg0 for struct return (in reg or on stack).
	 * - ebx in case of PIC code.
	 */

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
	if (stcall) {
		/* Prepend a placeholder for struct address. */
		/* Use EBP, can never show up under normal circumstances */
		l = talloc();
		*l = *r;
		r->n_op = CM;
		r->n_right = l;
		r->n_type = INT;
		l = block(REG, 0, 0, INCREF(VOID), 0, 0);
		regno(l) = EBP;
		l = block(FUNARG, l, 0, INCREF(VOID), 0, 0);
		r->n_left = l;
	}

#ifdef GCC_COMPAT
	if ((ap = attr_find(p->n_left->n_ap, GCC_ATYP_REGPARM)))
		rparg = ap->iarg(0);
	else
#endif
		rparg = 0;

	regcvt = 0;
	if (rparg)
		listf(p->n_right, addreg);

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

NODE *	
builtin_return_address(const struct bitable *bt, NODE *a)
{	
	int nframes;
	NODE *f; 
	
	if (a->n_op != ICON)
		goto bad;

	nframes = (int)a->n_lval;
  
	tfree(a);	
			
	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = FPREG;
 
	while (nframes--)
		f = block(UMUL, f, NIL, PTR+VOID, 0, 0);
				    
	f = block(PLUS, f, bcon(4), INCREF(PTR+VOID), 0, 0);
	f = buildtree(UMUL, f, NIL);	
   
	return f;
bad:						
	uerror("bad argument to __builtin_return_address");
	return bcon(0);
}

NODE *
builtin_frame_address(const struct bitable *bt, NODE *a)
{
	int nframes;
	NODE *f;

	if (a->n_op != ICON)
		goto bad;

	nframes = (int)a->n_lval;

	tfree(a);

	f = block(REG, NIL, NIL, PTR+VOID, 0, 0);
	regno(f) = FPREG;

	while (nframes--)
		f = block(UMUL, f, NIL, PTR+VOID, 0, 0);

	return f;
bad:
	uerror("bad argument to __builtin_frame_address");
	return bcon(0);
}

/*
 * Return "canonical frame address".
 */
NODE *
builtin_cfa(const struct bitable *bt, NODE *a)
{
	uerror("missing builtin_cfa");
	return bcon(0);
}

