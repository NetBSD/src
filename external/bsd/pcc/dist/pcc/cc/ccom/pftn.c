/*	Id: pftn.c,v 1.379 2014/07/02 15:31:41 ragge Exp 	*/	
/*	$NetBSD: pftn.c,v 1.9 2014/07/24 20:12:50 plunky Exp $	*/
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
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Many changes from the 32V sources, among them:
 * - New symbol table manager (moved to another file).
 * - Prototype saving/checks.
 */

# include "pass1.h"
#include "unicode.h"

#include "cgram.h"

struct symtab *cftnsp;
int arglistcnt, dimfuncnt;	/* statistics */
int symtabcnt, suedefcnt;	/* statistics */
int autooff,		/* the next unused automatic offset */
    maxautooff,		/* highest used automatic offset in function */
    argoff;		/* the next unused argument offset */
int retlab = NOLAB;	/* return label for subroutine */
int brklab;
int contlab;
int flostat;
int blevel;
int reached, prolab;

struct params;

#define MKTY(p, t, d, s) r = talloc(); *r = *p; \
	r = argcast(r, t, d, s); *p = *r; nfree(r);

/*
 * Linked list stack while reading in structs.
 */
struct rstack {
	struct	rstack *rnext;
	int	rsou;
	int	rstr;
	struct	symtab *rsym;
	struct	symtab *rb;
	struct	attr *ap;
	int	flags;
#define	LASTELM	1
} *rpole;

/*
 * Linked list for parameter (and struct elements) declaration.
 */
static struct params {
	struct params *prev;
	struct symtab *sym;
} *lparam;
static int nparams;

/* defines used for getting things off of the initialization stack */

NODE *arrstk[10];
int arrstkp;
static int intcompare;
NODE *parlink;

void fixtype(NODE *p, int class);
int fixclass(int class, TWORD type);
static void dynalloc(struct symtab *p, int *poff);
static void evalidx(struct symtab *p);
int isdyn(struct symtab *p);
void inforce(OFFSZ n);
void vfdalign(int n);
static void ssave(struct symtab *);
#ifdef PCC_DEBUG
static void alprint(union arglist *al, int in);
#endif
static void lcommadd(struct symtab *sp);
static NODE *mkcmplx(NODE *p, TWORD dt);
extern int fun_inline;

void
defid(NODE *q, int class)
{
	defid2(q, class, 0);
}

/*
 * Declaration of an identifier.  Handles redeclarations, hiding,
 * incomplete types and forward declarations.
 *
 * q is a TYPE node setup after parsing with n_type, n_df and n_ap.
 * n_sp is a pointer to the not-yet initalized symbol table entry
 * unless it's a redeclaration or supposed to hide a variable.
 */

void
defid2(NODE *q, int class, char *astr)
{
	struct attr *ap;
	struct symtab *p;
	TWORD type, qual;
	TWORD stp, stq;
	int scl;
	union dimfun *dsym, *ddef;
	int slev, temp, changed;

	if (q == NIL)
		return;  /* an error was detected */

#ifdef GCC_COMPAT
	gcc_modefix(q);
#endif
	p = q->n_sp;

	if (p->sname == NULL)
		cerror("defining null identifier");

#ifdef PCC_DEBUG
	if (ddebug) {
		printf("defid(%s '%s'(%p), ", p->sname, p->soname , p);
		tprint(q->n_type, q->n_qual);
		printf(", %s, (%p)), level %d\n\t", scnames(class),
		    q->n_df, blevel);
#ifdef GCC_COMPAT
		dump_attr(q->n_ap);
#endif
	}
#endif

	fixtype(q, class);

	type = q->n_type;
	qual = q->n_qual;
	class = fixclass(class, type);

	stp = p->stype;
	stq = p->squal;
	slev = p->slevel;

#ifdef PCC_DEBUG
	if (ddebug) {
		printf("	modified to ");
		tprint(type, qual);
		printf(", %s\n", scnames(class));
		printf("	previous def'n: ");
		tprint(stp, stq);
		printf(", %s, (%p,%p)), level %d\n",
		    scnames(p->sclass), p->sdf, p->sap, slev);
	}
#endif

	if (blevel == 1) {
		switch (class) {
		default:
			if (!(class&FIELD) && !ISFTN(type))
				uerror("declared argument %s missing",
				    p->sname );
			break;
		case MOS:
		case MOU:
			cerror("field5");
		case TYPEDEF:
		case PARAM:
			break;
		}
	}

	if (stp == UNDEF)
		goto enter; /* New symbol */

	if (type != stp)
		goto mismatch;

	if (blevel > slev && (class == AUTO || class == REGISTER))
		/* new scope */
		goto mismatch;

	/*
	 * test (and possibly adjust) dimensions.
	 * also check that prototypes are correct.
	 */
	dsym = p->sdf;
	ddef = q->n_df;
	changed = 0;
	for (temp = type; temp & TMASK; temp = DECREF(temp)) {
		if (ISARY(temp)) {
			if (dsym->ddim == NOOFFSET) {
				dsym->ddim = ddef->ddim;
				changed = 1;
			} else if (ddef->ddim != NOOFFSET &&
			    dsym->ddim!=ddef->ddim) {
				goto mismatch;
			}
			++dsym;
			++ddef;
		} else if (ISFTN(temp)) {
			/* add a late-defined prototype here */
			if (!oldstyle && dsym->dfun == NULL)
				dsym->dfun = ddef->dfun;
			if (!oldstyle && ddef->dfun != NULL &&
			    chkftn(dsym->dfun, ddef->dfun))
				uerror("declaration doesn't match prototype");
			dsym++, ddef++;
		}
	}
#ifdef STABS
	if (changed && gflag)
		stabs_chgsym(p); /* symbol changed */
#endif

	/* check that redeclarations are to the same structure */
	if (temp == STRTY || temp == UNIONTY) {
		if (strmemb(p->sap) != strmemb(q->n_ap))
			goto mismatch;
	}

	scl = p->sclass;

#ifdef PCC_DEBUG
	if (ddebug)
		printf("	previous class: %s\n", scnames(scl));
#endif

	/*
	 * Its allowed to add attributes to existing declarations.
	 * Be careful though not to trash existing attributes.
	 * XXX - code below is probably not correct.
	 */
	if (p->sap && p->sap->atype <= ATTR_MAX) {
		/* nothing special, just overwrite */
		p->sap = q->n_ap;
	} else {
		if (p->slevel == blevel) {
			for (ap = q->n_ap; ap; ap = ap->next) {
				if (ap->atype > ATTR_MAX)
					p->sap = attr_add(p->sap, attr_dup(ap, 3));
			}
		} else
			p->sap = q->n_ap;
	}

	if (class & FIELD)
		cerror("field1");
	switch(class) {

	case EXTERN:
		if (astr)
			p->soname = astr;
		switch( scl ){
		case STATIC:
		case USTATIC:
			if( slev==0 )
				goto done;
			break;
		case EXTDEF:
		case EXTERN:
			goto done;
		case SNULL:
			if (p->sflags & SINLINE) {
				p->sclass = EXTDEF;
				inline_ref(p);
				goto done;
			}
			break;
		}
		break;

	case STATIC:
		if (astr)
			p->soname = astr;
		if (scl==USTATIC || (scl==EXTERN && blevel==0)) {
			p->sclass = STATIC;
			goto done;
		}
		if (changed || (scl == STATIC && blevel == slev))
			goto done; /* identical redeclaration */
		break;

	case USTATIC:
		if (scl==STATIC || scl==USTATIC)
			goto done;
		break;

	case TYPEDEF:
		if (scl == class)
			goto done;
		break;

	case MOU:
	case MOS:
		cerror("field6");

	case EXTDEF:
		switch (scl) {
		case EXTERN:
			p->sclass = EXTDEF;
			goto done;
		case USTATIC:
			p->sclass = STATIC;
			goto done;
		case SNULL:
#ifdef GCC_COMPAT
			/*
			 * Handle redeclarations of inlined functions.
			 * This is allowed if the previous declaration is of
			 * type gnu_inline.
			 */
			if (attr_find(p->sap, GCC_ATYP_GNU_INLINE))
				goto done;
#endif
			break;
		}
		break;

	case AUTO:
	case REGISTER:
		break;  /* mismatch.. */
	case SNULL:
		if (fun_inline && ISFTN(type)) {
			if (scl == EXTERN) {
				p->sclass = EXTDEF;
				inline_ref(p);
			}
			goto done;
		}
		break;
	}

	mismatch:

	/*
	 * Only allowed for automatic variables.
	 */
	if ((blevel == 2 && slev == 1) || blevel <= slev || class == EXTERN) {
		uerror("redeclaration of %s", p->sname);
		return;
	}
	q->n_sp = p = hide(p);

	enter:  /* make a new entry */

#ifdef PCC_DEBUG
	if(ddebug)
		printf("	new entry made\n");
#endif
	p->stype = type;
	p->squal = qual;
	p->sclass = (char)class;
	p->slevel = (char)blevel;
	p->soffset = NOOFFSET;
	if (q->n_ap)
		p->sap = attr_add(q->n_ap, p->sap);

	/* copy dimensions */
	p->sdf = q->n_df;
	/* Do not save param info for old-style functions */
	if (ISFTN(type) && oldstyle)
		p->sdf->dfun = NULL;

	if (arrstkp)
		evalidx(p);

	/* allocate offsets */
	if (class&FIELD) {
		cerror("field2");  /* new entry */
	} else switch (class) {

	case REGISTER:
		if (astr != NULL)
			werror("no register assignment (yet)");
		p->sclass = class = AUTO;
		/* FALLTHROUGH */
	case AUTO:
		if (isdyn(p)) {
			p->sflags |= SDYNARRAY;
			dynalloc(p, &autooff);
		} else
			oalloc(p, &autooff);
		break;

	case PARAM:
		if (q->n_type != FARG)
			oalloc(p, &argoff);
		break;
		
	case STATIC:
	case EXTDEF:
	case EXTERN:
		p->soffset = getlab();
		/* FALLTHROUGH */
	case USTATIC:
		if (astr)
			p->soname = astr;
		break;

	case MOU:
	case MOS:
		cerror("field7");
	case SNULL:
#ifdef notdef
		if (fun_inline) {
			p->slevel = 1;
			p->soffset = getlab();
		}
#endif
		break;
	}

#ifdef STABS
	if (gflag && p->stype != FARG)
		stabs_newsym(p);
#endif

done:
	fixdef(p);	/* Leave last word to target */
#ifndef HAVE_WEAKREF
	{
		struct attr *at;

		/* Refer renamed function */
		if ((at = attr_find(p->sap, GCC_ATYP_WEAKREF)))
			p->soname = at->sarg(0);
	}
#endif
#ifdef PCC_DEBUG
	if (ddebug) {
		printf( "	sdf, offset: %p, %d\n\t",
		    p->sdf, p->soffset);
#ifdef GCC_COMPAT
		dump_attr(p->sap);
#endif
	}
#endif
}

void
ssave(struct symtab *sym)
{
	struct params *p;

	p = tmpalloc(sizeof(struct params));
	p->prev = lparam;
	p->sym = sym;
	lparam = p;
}

/*
 * end of function
 */
void
ftnend(void)
{
#ifdef GCC_COMPAT
	struct attr *gc, *gd;
#endif
	extern int *mkclabs(void);
	extern NODE *cftnod;
	extern struct savbc *savbc;
	extern struct swdef *swpole;
	extern int tvaloff;
	char *c;

	if (retlab != NOLAB && nerrors == 0) { /* inside a real function */
		plabel(retlab);
		if (cftnod)
			ecomp(buildtree(FORCE, cftnod, NIL));
		efcode(); /* struct return handled here */
		if ((c = cftnsp->soname) == NULL)
			c = addname(exname(cftnsp->sname));
		SETOFF(maxautooff, ALCHAR);
		send_passt(IP_EPILOG, maxautooff/SZCHAR, c,
		    cftnsp->stype, cftnsp->sclass == EXTDEF,
		    retlab, tvaloff, mkclabs());
	}

	cftnod = NIL;
	tcheck();
	brklab = contlab = retlab = NOLAB;
	flostat = 0;
	if (nerrors == 0) {
		if (savbc != NULL)
			cerror("bcsave error");
		if (lparam != NULL)
			cerror("parameter reset error");
		if (swpole != NULL)
			cerror("switch error");
	}
#ifdef GCC_COMPAT
	if (cftnsp) {
		gc = attr_find(cftnsp->sap, GCC_ATYP_CONSTRUCTOR);
		gd = attr_find(cftnsp->sap, GCC_ATYP_DESTRUCTOR);
		if (gc || gd) {
			struct symtab sts = *cftnsp;
			NODE *p;
			sts.stype = INCREF(sts.stype);
			p = nametree(&sts);
			p->n_op = ICON;
			if (gc) {
				locctr(CTORS, NULL);
				inval(0, SZPOINT(0), p);
			}
			if (gd) {
				locctr(DTORS, NULL);
				inval(0, SZPOINT(0), p);
			}
			tfree(p);
		}
	}
#endif
	savbc = NULL;
	lparam = NULL;
	cftnsp = NULL;
	maxautooff = autooff = AUTOINIT;
	reached = 1;

	if (isinlining)
		inline_end();
	inline_prtout();

	tmpfree(); /* Release memory resources */
}

static struct symtab nulsym = {
	NULL, 0, 0, 0, 0, "null", "null", INT, 0, NULL, NULL
};

void
dclargs(void)
{
	union dimfun *df;
	union arglist *al, *al2, *alb;
	struct params *a;
	struct symtab *p, **parr = NULL; /* XXX gcc */
	int i;

	/*
	 * Deal with fun(void) properly.
	 */
	if (nparams == 1 && lparam->sym && lparam->sym->stype == VOID)
		goto done;

	/*
	 * Generate a list for bfcode().
	 * Parameters were pushed in reverse order.
	 */
	if (nparams != 0)
		parr = tmpalloc(sizeof(struct symtab *) * nparams);

	if (nparams)
	    for (a = lparam, i = 0; a != NULL; a = a->prev) {
		p = a->sym;
		parr[i++] = p;
		if (p == NULL) {
			uerror("parameter %d name missing", i);
			p = &nulsym; /* empty symtab */
		}
		if (p->stype == FARG)
			p->stype = INT;
		if (ISARY(p->stype)) {
			p->stype += (PTR-ARY);
			p->sdf++;
		} else if (ISFTN(p->stype)) {
			werror("function declared as argument");
			p->stype = INCREF(p->stype);
		}
#ifdef STABS
		if (gflag)
			stabs_newsym(p);
#endif
	}
	if (oldstyle && (df = cftnsp->sdf) && (al = df->dfun)) {
		/*
		 * Check against prototype of oldstyle function.
		 */
		alb = al2 = tmpalloc(sizeof(union arglist) * nparams * 3 + 1);
		for (i = 0; i < nparams; i++) {
			TWORD type = parr[i]->stype;
			(al2++)->type = type;
			if (ISSOU(BTYPE(type)))
				(al2++)->sap = parr[i]->sap;
			while (!ISFTN(type) && !ISARY(type) && type > BTMASK)
				type = DECREF(type);
			if (type > BTMASK)
				(al2++)->df = parr[i]->sdf;
		}
		al2->type = TNULL;
		intcompare = 1;
		if (chkftn(al, alb))
			uerror("function doesn't match prototype");
		intcompare = 0;

	}

	if (oldstyle && nparams) {
		/* Must recalculate offset for oldstyle args here */
		argoff = ARGINIT;
		for (i = 0; i < nparams; i++) {
			parr[i]->soffset = NOOFFSET;
			oalloc(parr[i], &argoff);
		}
	}

done:	autooff = AUTOINIT;

	plabel(prolab); /* after prolog, used in optimization */
	retlab = getlab();
	bfcode(parr, nparams);
	if (fun_inline && (xinline
#ifdef GCC_COMPAT
 || attr_find(cftnsp->sap, GCC_ATYP_ALW_INL)
#endif
		))
		inline_args(parr, nparams);
	plabel(getlab()); /* used when spilling */
	if (parlink)
		ecomp(parlink);
	parlink = NIL;
	lparam = NULL;
	nparams = 0;
	symclear(1);	/* In case of function pointer args */
}

/*
 * basic attributes for structs and enums
 */
static struct attr *
seattr(void)
{
	return attr_add(attr_new(ATTR_ALIGNED, 4), attr_new(ATTR_STRUCT, 2));
}

/*
 * Struct/union/enum symtab construction.
 */
static void
defstr(struct symtab *sp, int class)
{
	sp->sclass = (char)class;
	if (class == STNAME)
		sp->stype = STRTY;
	else if (class == UNAME)
		sp->stype = UNIONTY;
	else if (class == ENAME)
		sp->stype = ENUMTY;
}

/*
 * Declare a struct/union/enum tag.
 * If not found, create a new tag with UNDEF type.
 */
static struct symtab *
deftag(char *name, int class)
{
	struct symtab *sp;

	if ((sp = lookup(name, STAGNAME))->sap == NULL) {
		/* New tag */
		defstr(sp, class);
	} else if (sp->sclass != class)
		uerror("tag %s redeclared", name);
	return sp;
}

/*
 * reference to a structure or union, with no definition
 */
NODE *
rstruct(char *tag, int soru)
{
	struct symtab *sp;

	sp = deftag(tag, soru);
	if (sp->sap == NULL)
		sp->sap = seattr();
	return mkty(sp->stype, 0, sp->sap);
}

static int enumlow, enumhigh;
int enummer;

/*
 * Declare a member of enum.
 */
void
moedef(char *name)
{
	struct symtab *sp;

	sp = lookup(name, SNORMAL);
	if (sp->stype == UNDEF || (sp->slevel < blevel)) {
		if (sp->stype != UNDEF)
			sp = hide(sp);
		sp->stype = INT; /* always */
		sp->sclass = MOE;
		sp->soffset = enummer;
	} else
		uerror("%s redeclared", name);
	if (enummer < enumlow)
		enumlow = enummer;
	if (enummer > enumhigh)
		enumhigh = enummer;
	enummer++;
}

/*
 * Declare an enum tag.  Complain if already defined.
 */
struct symtab *
enumhd(char *name)
{
	struct attr *ap;
	struct symtab *sp;

	enummer = enumlow = enumhigh = 0;
	if (name == NULL)
		return NULL;

	sp = deftag(name, ENAME);
	if (sp->stype != ENUMTY) {
		if (sp->slevel == blevel)
			uerror("%s redeclared", name);
		sp = hide(sp);
		defstr(sp, ENAME);
	}
	if (sp->sap == NULL)
		ap = sp->sap = attr_new(ATTR_STRUCT, 4);
	else
		ap = attr_find(sp->sap, ATTR_STRUCT);
	ap->amlist = sp;
	return sp;
}

/*
 * finish declaration of an enum
 */
NODE *
enumdcl(struct symtab *sp)
{
	NODE *p;
	TWORD t;

#ifdef ENUMSIZE
	t = ENUMSIZE(enumhigh, enumlow);
#else
	t = ctype(enumlow < 0 ? INT : UNSIGNED);
#ifdef notdef
	if (enumhigh <= MAX_CHAR && enumlow >= MIN_CHAR)
		t = ctype(CHAR);
	else if (enumhigh <= MAX_SHORT && enumlow >= MIN_SHORT)
		t = ctype(SHORT);
	else
		t = ctype(INT);
#endif
#endif
	
	if (sp)
		sp->stype = t;
	p = mkty(t, 0, 0);
	p->n_sp = sp;
	return p;
}

/*
 * Handle reference to an enum
 */
NODE *
enumref(char *name)
{
	struct symtab *sp;
	NODE *p;

	sp = lookup(name, STAGNAME);

#ifdef notdef
	/*
	 * 6.7.2.3 Clause 2:
	 * "A type specifier of the form 'enum identifier' without an
	 *  enumerator list shall only appear after the type it specifies
	 *  is complete."
	 */
	if (sp->sclass != ENAME)
		uerror("enum %s undeclared", name);
#endif
	if (sp->sclass == SNULL) {
		/* declare existence of enum */
		sp = enumhd(name);
		sp->stype = ENUMTY;
	}

	p = mkty(sp->stype, 0, sp->sap);
	p->n_sp = sp;
	return p;
}

/*
 * begining of structure or union declaration
 * It's an error if this routine is called twice with the same struct.
 */
struct rstack *
bstruct(char *name, int soru, NODE *gp)
{
	struct rstack *r;
	struct symtab *sp;
	struct attr *ap, *gap;

#ifdef GCC_COMPAT
	gap = gp ? gcc_attr_parse(gp) : NULL;
#else
	gap = NULL;
#endif

	if (name != NULL) {
		sp = deftag(name, soru);
		if (sp->sap == NULL)
			sp->sap = seattr();
		ap = attr_find(sp->sap, ATTR_ALIGNED);
		if (ap->iarg(0) != 0) {
			if (sp->slevel < blevel) {
				sp = hide(sp);
				defstr(sp, soru);
				sp->sap = seattr();
			} else
				uerror("%s redeclared", name);
		}
		gap = sp->sap = attr_add(sp->sap, gap);
	} else {
		gap = attr_add(seattr(), gap);
		sp = NULL;
	}

	r = tmpcalloc(sizeof(struct rstack));
	r->rsou = soru;
	r->rsym = sp;
	r->rb = NULL;
	r->ap = gap;
	r->rnext = rpole;
	rpole = r;

	return r;
}

/*
 * Called after a struct is declared to restore the environment.
 * - If ALSTRUCT is defined, this will be the struct alignment and the
 *   struct size will be a multiple of ALSTRUCT, otherwise it will use
 *   the alignment of the largest struct member.
 */
NODE *
dclstruct(struct rstack *r)
{
	NODE *n;
	struct attr *aps, *apb;
	struct symtab *sp;
	int al, sa, sz;

	apb = attr_find(r->ap, ATTR_ALIGNED);
	aps = attr_find(r->ap, ATTR_STRUCT);
	aps->amlist = r->rb;

#ifdef ALSTRUCT
	al = ALSTRUCT;
#else
	al = ALCHAR;
#endif

	/*
	 * extract size and alignment, calculate offsets
	 */
	for (sp = r->rb; sp; sp = sp->snext) {
		sa = talign(sp->stype, sp->sap);
		if (sp->sclass & FIELD)
			sz = sp->sclass&FLDSIZ;
		else
			sz = (int)tsize(sp->stype, sp->sdf, sp->sap);
		if (sz > rpole->rstr)
			rpole->rstr = sz;  /* for use with unions */
		/*
		 * set al, the alignment, to the lcm of the alignments
		 * of the members.
		 */
		SETOFF(al, sa);
	}

	SETOFF(rpole->rstr, al);

	aps->amsize = rpole->rstr;
	apb->iarg(0) = al;

#ifdef PCC_DEBUG
	if (ddebug) {
		printf("dclstruct(%s): size=%d, align=%d\n",
		    r->rsym ? r->rsym->sname : "??",
		    aps->amsize, apb->iarg(0));
	}
	if (ddebug>1) {
		printf("\tsize %d align %d link %p\n",
		    aps->amsize, apb->iarg(0), aps->amlist);
		for (sp = aps->amlist; sp != NULL; sp = sp->snext) {
			printf("\tmember %s(%p)\n", sp->sname, sp);
		}
	}
#endif

#ifdef STABS
	if (gflag)
		stabs_struct(r->rsym, r->ap);
#endif

	rpole = r->rnext;
	n = mkty(r->rsou == STNAME ? STRTY : UNIONTY, 0, r->ap);
	n->n_sp = r->rsym;

	n->n_qual |= 1; /* definition place XXX used by attributes */
	return n;
}

/*
 * Add a new member to the current struct or union being declared.
 */
void
soumemb(NODE *n, char *name, int class)
{
	struct symtab *sp, *lsp;
	int incomp, tsz, al;
	TWORD t;
 
	if (rpole == NULL)
		cerror("soumemb");
 
	/* check if tag name exists */
	lsp = NULL;
	for (sp = rpole->rb; sp != NULL; lsp = sp, sp = sp->snext)
		if (*name != '*' && sp->sname == name)
			uerror("redeclaration of %s", name);

	sp = getsymtab(name, SMOSNAME);
	if (rpole->rb == NULL)
		rpole->rb = sp;
	else
		lsp->snext = sp;

	n->n_sp = sp;
	sp->stype = n->n_type;
	sp->squal = n->n_qual;
	sp->slevel = blevel;
	sp->sap = n->n_ap;
	sp->sdf = n->n_df;

	if (class & FIELD) {
		sp->sclass = (char)class;
		falloc(sp, class&FLDSIZ, NIL);
	} else if (rpole->rsou == STNAME || rpole->rsou == UNAME) {
		sp->sclass = rpole->rsou == STNAME ? MOS : MOU;
		if (sp->sclass == MOU)
			rpole->rstr = 0;
		al = talign(sp->stype, sp->sap);
		tsz = (int)tsize(sp->stype, sp->sdf, sp->sap);
		sp->soffset = upoff(tsz, al, &rpole->rstr);
	}

	/*
	 * 6.7.2.1 clause 16:
	 * "...the last member of a structure with more than one
	 *  named member may have incomplete array type;"
	 */
	if (ISARY(sp->stype) && sp->sdf->ddim == NOOFFSET)
		incomp = 1;
	else
		incomp = 0;
	if ((rpole->flags & LASTELM) || (rpole->rb == sp && incomp == 1))
		uerror("incomplete array in struct");
	if (incomp == 1)
		rpole->flags |= LASTELM;

	/*
	 * 6.7.2.1 clause 2:
	 * "...such a structure shall not be a member of a structure
	 *  or an element of an array."
	 */
	t = sp->stype;
	if (rpole->rsou != STNAME || BTYPE(t) != STRTY)
		return; /* not for unions */
	while (ISARY(t))
		t = DECREF(t);
	if (ISPTR(t))
		return;

	if ((lsp = strmemb(sp->sap)) != NULL) {
		for (; lsp->snext; lsp = lsp->snext)
			;
		if (ISARY(lsp->stype) && lsp->snext &&
		    lsp->sdf->ddim == NOOFFSET)
			uerror("incomplete struct in struct");
	}
}

/*
 * error printing routine in parser
 */
void
yyerror(char *s)
{
	uerror(s);
}

void yyaccpt(void);
void
yyaccpt(void)
{
	ftnend();
}

/*
 * p is top of type list given to tymerge later.
 * Find correct CALL node and declare parameters from there.
 */
void
ftnarg(NODE *p)
{
	NODE *q;

#ifdef PCC_DEBUG
	if (ddebug > 2)
		printf("ftnarg(%p)\n", p);
#endif
	/*
	 * Push argument symtab entries onto param stack in reverse order,
	 * due to the nature of the stack it will be reclaimed correct.
	 */
	for (; p->n_op != NAME; p = p->n_left) {
		if (p->n_op == UCALL && p->n_left->n_op == NAME)
			return;	/* Nothing to enter */
		if (p->n_op == CALL && p->n_left->n_op == NAME)
			break;
	}

	p = p->n_right;
	while (p->n_op == CM) {
		q = p->n_right;
		if (q->n_op != ELLIPSIS) {
			ssave(q->n_sp);
			nparams++;
#ifdef PCC_DEBUG
			if (ddebug > 2)
				printf("	saving sym %s (%p) from (%p)\n",
				    q->n_sp->sname, q->n_sp, q);
#endif
		}
		p = p->n_left;
	}
	ssave(p->n_sp);
	if (p->n_type != VOID)
		nparams++;

#ifdef PCC_DEBUG
	if (ddebug > 2)
		printf("	saving sym %s (%p) from (%p)\n",
		    nparams ? p->n_sp->sname : "<noname>", p->n_sp, p);
#endif
}

/*
 * compute the alignment of an object with type ty, sizeoff index s
 */
int
talign(unsigned int ty, struct attr *apl)
{
	struct attr *al;
	int a;

	for (; ty > BTMASK; ty = DECREF(ty)) {
		switch (ty & TMASK) {
		case PTR:
			return(ALPOINT);
		case ARY:
			continue;
		case FTN:
			cerror("compiler takes alignment of function");
		}
	}

	/* check for alignment attribute */
	if ((al = attr_find(apl, ATTR_ALIGNED))) {
		if ((a = al->iarg(0)) == 0) {
			uerror("no alignment");
			a = ALINT;
		} 
		return a;
	}

#ifndef NO_COMPLEX
	if (ISITY(ty))
		ty -= (FIMAG-FLOAT);
#endif
	ty = BTYPE(ty);
	if (ty >= CHAR && ty <= ULONGLONG && ISUNSIGNED(ty))
		ty = DEUNSIGN(ty);

	switch (ty) {
	case BOOL: a = ALBOOL; break;
	case CHAR: a = ALCHAR; break;
	case SHORT: a = ALSHORT; break;
	case INT: a = ALINT; break;
	case LONG: a = ALLONG; break;
	case LONGLONG: a = ALLONGLONG; break;
	case FLOAT: a = ALFLOAT; break;
	case DOUBLE: a = ALDOUBLE; break;
	case LDOUBLE: a = ALLDOUBLE; break;
	default:
		uerror("no alignment");
		a = ALINT;
	}
	return a;
}

short sztable[] = { 0, SZBOOL, SZCHAR, SZCHAR, SZSHORT, SZSHORT, SZINT, SZINT,
	SZLONG, SZLONG, SZLONGLONG, SZLONGLONG, SZFLOAT, SZDOUBLE, SZLDOUBLE };

/* compute the size associated with type ty,
 *  dimoff d, and sizoff s */
/* BETTER NOT BE CALLED WHEN t, d, and s REFER TO A BIT FIELD... */
OFFSZ
tsize(TWORD ty, union dimfun *d, struct attr *apl)
{
	struct attr *ap, *ap2;
	OFFSZ mult, sz;

	mult = 1;

	for (; ty > BTMASK; ty = DECREF(ty)) {
		switch (ty & TMASK) {

		case FTN:
			uerror( "cannot take size of function");
		case PTR:
			return( SZPOINT(ty) * mult );
		case ARY:
			if (d->ddim == NOOFFSET)
				return 0;
			if (d->ddim < 0)
				cerror("tsize: dynarray");
			mult *= d->ddim;
			d++;
		}
	}

#ifndef NO_COMPLEX
	if (ISITY(ty))
		ty -= (FIMAG-FLOAT);
#endif

	if (ty == VOID)
		ty = CHAR;
	if (ty <= LDOUBLE)
		sz = sztable[ty];
	else if (ISSOU(ty)) {
		if ((ap = strattr(apl)) == NULL ||
		    (ap2 = attr_find(apl, ATTR_ALIGNED)) == NULL ||
		    (ap2->iarg(0) == 0)) {
			uerror("unknown structure/union/enum");
			sz = SZINT;
		} else
			sz = ap->amsize;
	} else {
		uerror("unknown type");
		sz = SZINT;
	}

	return((unsigned int)sz * mult);
}

/*
 * Save string (and print it out).  If wide then wide string.
 */
NODE *
strend(int wide, char *str)
{
	struct symtab *sp;
	NODE *p;

	/* If an identical string is already emitted, just forget this one */
	if (wide) {
		/* Do not save wide strings, at least not now */
		sp = getsymtab(str, SSTRING|STEMP);
	} else {
		str = addstring(str);	/* enter string in string table */
		sp = lookup(str, SSTRING);	/* check for existence */
	}

	if (sp->soffset == 0) { /* No string */
		char *wr;
		int i;

		sp->sclass = STATIC;
		sp->slevel = 1;
		sp->soffset = getlab();
		sp->squal = (CON >> TSHIFT);
		sp->sdf = permalloc(sizeof(union dimfun));
		if (wide) {
			sp->stype = WCHAR_TYPE+ARY;
		} else {
			if (xuchar) {
				sp->stype = UCHAR+ARY;
			} else {
				sp->stype = CHAR+ARY;
			}
		}
		if (wide) {
			for (wr = sp->sname, i = 1; *wr; i++)
				u82cp(&wr);
			sp->sdf->ddim = i;
			inwstring(sp);
		} else {
			sp->sdf->ddim = strlen(sp->sname)+1;
			instring(sp);
		}
	}

	p = block(NAME, NIL, NIL, sp->stype, sp->sdf, sp->sap);
	p->n_sp = sp;
	return(clocal(p));
}

/*
 * Print out a wide string by calling ninval().
 */
void
inwstring(struct symtab *sp)
{
	char *s = sp->sname;
	NODE *p;

	locctr(STRNG, sp);
	defloc(sp);
	p = xbcon(0, NULL, WCHAR_TYPE);
	do {
		p->n_lval=u82cp(&s);
		inval(0, tsize(WCHAR_TYPE, NULL, NULL), p);
	} while (s[-1] != 0);
	nfree(p);
}

#ifndef MYINSTRING
/*
 * Print out a string of characters.
 * Assume that the assembler understands C-style escape
 * sequences.
 * Do not break UTF-8 sequences between lines and ensure
 * the code path is 8-bit clean.
 */
void
instring(struct symtab *sp)
{
	char *s = sp->sname;

	locctr(STRNG, sp);
	defloc(sp);

	char line[70], *t = line;
	printf("\t.ascii \"");
	while(s[0] != 0) {
		unsigned int c=(unsigned char)*s++;
		if(c<' ') t+=sprintf(t,"\\%03o",c);
		else if(c=='\\') *t++='\\', *t++='\\';
		else if(c=='\"') *t++='\\', *t++='\"';
		else if(c=='\'') *t++='\\', *t++='\'';
		else {
			*t++=c;
			int sz=u8len(&s[-1]);
			int i;
			for(i=1;i<sz;i++) *t++=*s++;
		}
		if(t>=&line[60]) {
			fwrite(line, 1, t-&line[0], stdout);
			printf("\"\n\t.ascii \"");
			t = line;
		}
	}
	fwrite(line, 1, t-&line[0], stdout);
	printf("\\0\"\n");
}
#endif

/*
 * update the offset pointed to by poff; return the
 * offset of a value of size `size', alignment `alignment',
 * given that off is increasing
 */
int
upoff(int size, int alignment, int *poff)
{
	int off;

	off = *poff;
	SETOFF(off, alignment);
	if (off < 0)
		cerror("structure or stack overgrown"); /* wrapped */
	*poff = off+size;
	return (off);
}

/*
 * allocate p with offset *poff, and update *poff
 */
int
oalloc(struct symtab *p, int *poff )
{
	int al, off, tsz;
	int noff;

	/*
	 * Only generate tempnodes if we are optimizing,
	 * and only for integers, floats or pointers,
	 * and not if the type on this level is volatile.
	 */
	if (xtemps && ((p->sclass == AUTO) || (p->sclass == REGISTER)) &&
	    (p->stype < STRTY || ISPTR(p->stype)) &&
	    !(cqual(p->stype, p->squal) & VOL) && cisreg(p->stype)) {
		NODE *tn = tempnode(0, p->stype, p->sdf, p->sap);
		p->soffset = regno(tn);
		p->sflags |= STNODE;
		nfree(tn);
		return 0;
	}

	al = talign(p->stype, p->sap);
	noff = off = *poff;
	tsz = (int)tsize(p->stype, p->sdf, p->sap);
#ifdef BACKAUTO
	if (p->sclass == AUTO) {
		noff = off + tsz;
		if (noff < 0)
			cerror("stack overflow");
		SETOFF(noff, al);
		off = -noff;
	} else
#endif
	if (p->sclass == PARAM && (p->stype == CHAR || p->stype == UCHAR ||
	    p->stype == SHORT || p->stype == USHORT || p->stype == BOOL)) {
		off = upoff(SZINT, ALINT, &noff);
#if TARGET_ENDIAN == TARGET_BE
		off = noff - tsz;
#endif
	} else {
		off = upoff(tsz, al, &noff);
	}

	if (p->sclass != REGISTER) {
	/* in case we are allocating stack space for register arguments */
		if (p->soffset == NOOFFSET)
			p->soffset = off;
		else if(off != p->soffset)
			return(1);
	}

	*poff = noff;
	return(0);
}

/*
 * Delay emission of code generated in argument headers.
 */
static void
edelay(NODE *p)
{
	if (blevel == 1) {
		/* Delay until after declarations */
		if (parlink == NULL)
			parlink = p;
		else
			parlink = block(COMOP, parlink, p, 0, 0, 0);
	} else
		ecomp(p);
}

/*
 * Traverse through the array args, evaluate them and put the 
 * resulting temp numbers in the dim fields.
 */
static void
evalidx(struct symtab *sp)
{
	union dimfun *df;
	NODE *p;
	TWORD t;
	int astkp = 0;

	if (arrstk[0] == NIL)
		astkp++; /* for parameter arrays */

	if (isdyn(sp))
		sp->sflags |= SDYNARRAY;

	df = sp->sdf;
	for (t = sp->stype; t > BTMASK; t = DECREF(t)) {
		if (!ISARY(t))
			continue;
		if (df->ddim == -1) {
			p = tempnode(0, INT, 0, 0);
			df->ddim = -regno(p);
			edelay(buildtree(ASSIGN, p, arrstk[astkp++]));
		}
		df++;
	}
	arrstkp = 0;
}

/*
 * Return 1 if dynamic array, 0 otherwise.
 */
int
isdyn(struct symtab *sp)
{
	union dimfun *df = sp->sdf;
	TWORD t;

	for (t = sp->stype; t > BTMASK; t = DECREF(t)) {
		if (!ISARY(t))
			return 0;
		if (df->ddim < 0 && df->ddim != NOOFFSET)
			return 1;
		df++;
	}
	return 0;
}

/*
 * Allocate space on the stack for dynamic arrays (or at least keep track
 * of the index).
 * Strategy is as follows:
 * - first entry is a pointer to the dynamic datatype.
 * - if it's a one-dimensional array this will be the only entry used.
 * - if it's a multi-dimensional array the following (numdim-1) integers
 *   will contain the sizes to multiply the indexes with.
 * - code to write the dimension sizes this will be generated here.
 * - code to allocate space on the stack will be generated here.
 */
static void
dynalloc(struct symtab *p, int *poff)
{
	union dimfun *df;
	NODE *n, *tn, *pol;
	TWORD t;

	/*
	 * The pointer to the array is not necessarily stored in a
	 * TEMP node, but if it is, its number is in the soffset field;
	 */
	t = p->stype;
	p->sflags |= STNODE;
	p->stype = INCREF(p->stype); /* Make this an indirect pointer */
	tn = tempnode(0, p->stype, p->sdf, p->sap);
	p->soffset = regno(tn);

	df = p->sdf;

	pol = bcon(1);
	for (; t > BTMASK; t = DECREF(t)) {
		if (!ISARY(t))
			break;
		if (df->ddim < 0)
			n = tempnode(-df->ddim, INT, 0, 0);
		else
			n = bcon(df->ddim);

		pol = buildtree(MUL, pol, n);
		df++;
	}
	/* Create stack gap */
	spalloc(tn, pol, tsize(t, 0, p->sap));
}

/*
 * allocate a field of width w
 * new is 0 if new entry, 1 if redefinition, -1 if alignment
 */
int
falloc(struct symtab *p, int w, NODE *pty)
{
	TWORD otype, type;
	int al,sz;

	otype = type = p ? p->stype : pty->n_type;

	if (type == BOOL)
		type = BOOL_TYPE;
	if (!ISINTEGER(type)) {
		uerror("illegal field type");
		type = INT;
	}

	al = talign(type, NULL);
	sz = tsize(type, NULL, NULL);

	if (w > sz) {
		uerror("field too big");
		w = sz;
	}

	if (w == 0) { /* align only */
		SETOFF(rpole->rstr, al);
		if (p != NULL)
			uerror("zero size field");
		return(0);
	}

	if (rpole->rstr%al + w > sz)
		SETOFF(rpole->rstr, al);
	if (p == NULL) {
		rpole->rstr += w;  /* we know it will fit */
		return(0);
	}

	/* establish the field */

	p->soffset = rpole->rstr;
	rpole->rstr += w;
	p->stype = otype;
	fldty(p);
	return(0);
}

/*
 * Check if this symbol should be a common or must be handled in data seg.
 */
static void
commchk(struct symtab *sp)
{
	if ((sp->sflags & STLS)
#ifdef GCC_COMPAT
		|| attr_find(sp->sap, GCC_ATYP_SECTION)
#endif
	    ) {
		/* TLS handled in data segment */
		if (sp->sclass == EXTERN)
			sp->sclass = EXTDEF;
		beginit(sp);
		endinit(1);
	} else {
		symdirec(sp);
		defzero(sp);
	}
}

void
nidcl(NODE *p, int class)
{
	nidcl2(p, class, 0);
}

/*
 * handle unitialized declarations assumed to be not functions:
 * int a;
 * extern int a;
 * static int a;
 */
void
nidcl2(NODE *p, int class, char *astr)
{
	struct symtab *sp;
	int commflag = 0;

	/* compute class */
	if (class == SNULL) {
		if (blevel > 1)
			class = AUTO;
		else if (blevel != 0 || rpole)
			cerror( "nidcl error" );
		else /* blevel = 0 */
			commflag = 1, class = EXTERN;
	}

	defid2(p, class, astr);

	sp = p->n_sp;
	/* check if forward decl */
	if (ISARY(sp->stype) && sp->sdf->ddim == NOOFFSET)
		return;

	if (sp->sflags & SASG)
		return; /* already initialized */

	switch (class) {
	case EXTDEF:
		/* simulate initialization by 0 */
		simpleinit(p->n_sp, bcon(0));
		break;
	case EXTERN:
		if (commflag)
			lcommadd(p->n_sp);
		else
			extdec(p->n_sp);
		break;
	case STATIC:
		if (blevel == 0)
			lcommadd(p->n_sp);
		else
			commchk(p->n_sp);
		break;
	}
}

struct lcd {
	SLIST_ENTRY(lcd) next;
	struct symtab *sp;
};

static SLIST_HEAD(, lcd) lhead = { NULL, &lhead.q_forw};

/*
 * Add a local common statement to the printout list.
 */
void
lcommadd(struct symtab *sp)
{
	struct lcd *lc, *lcp;

	lcp = NULL;
	SLIST_FOREACH(lc, &lhead, next) {
		if (lc->sp == sp)
			return; /* already exists */
		if (lc->sp == NULL && lcp == NULL)
			lcp = lc;
	}
	if (lcp == NULL) {
		lc = permalloc(sizeof(struct lcd));
		lc->sp = sp;
		SLIST_INSERT_LAST(&lhead, lc, next);
	} else
		lcp->sp = sp;
}

/*
 * Delete a local common statement.
 */
void
lcommdel(struct symtab *sp)
{
	struct lcd *lc;

	SLIST_FOREACH(lc, &lhead, next) {
		if (lc->sp == sp) {
			lc->sp = NULL;
			return;
		}
	}
}

/*
 * Print out the remaining common statements.
 */
void
lcommprint(void)
{
	struct lcd *lc;

	SLIST_FOREACH(lc, &lhead, next) {
		if (lc->sp != NULL)
			commchk(lc->sp);
	}
}

/*
 * Merge given types to a single node.
 * Any type can end up here.
 * p is the old node, q is the old (if any).
 * CLASS is AUTO, EXTERN, REGISTER, STATIC or TYPEDEF.
 * QUALIFIER is VOL or CON
 * TYPE is CHAR, SHORT, INT, LONG, SIGNED, UNSIGNED, VOID, BOOL, FLOAT,
 * 	DOUBLE, STRTY, UNIONTY.
 */
struct typctx {
	int class, qual, sig, uns, cmplx, imag, err;
	TWORD type;
	NODE *saved;
	struct attr *pre, *post;
};

static void
typwalk(NODE *p, void *arg)
{
	struct typctx *tc = arg;

#define	cmop(x,y) block(CM, x, y, INT, 0, 0)
	switch (p->n_op) {
	case ATTRIB:
#ifdef GCC_COMPAT
		if (tc->saved && (tc->saved->n_qual & 1)) {
			tc->post = attr_add(tc->post,gcc_attr_parse(p->n_left));
		} else {
			tc->pre = attr_add(tc->pre, gcc_attr_parse(p->n_left));
		}
		p->n_left = bcon(0); /* For tfree() */
#else
		uerror("gcc type attribute used");
#endif
		break;
	case CLASS:
		if (p->n_type == 0)
			break;	/* inline hack */
		if (tc->class)
			tc->err = 1; /* max 1 class */
		tc->class = p->n_type;
		break;

	case QUALIFIER:
		if (p->n_qual == 0 && tc->saved && !ISPTR(tc->saved->n_type))
			uerror("invalid use of 'restrict'");
		tc->qual |= p->n_qual >> TSHIFT;
		break;

	case TYPE:
		if (p->n_sp != NULL || ISSOU(p->n_type)) {
			/* typedef, enum or struct/union */
			if (tc->saved || tc->type)
				tc->err = 1;
#ifdef GCC_COMPAT
			if (ISSOU(p->n_type) && p->n_left) {
				if (tc->post)
					cerror("typwalk");
				tc->post = gcc_attr_parse(p->n_left);
			}
#endif
			tc->saved = ccopy(p);
			break;
		}

		switch (p->n_type) {
		case BOOL:
		case CHAR:
		case FLOAT:
		case VOID:
			if (tc->type)
				tc->err = 1;
			tc->type = p->n_type;
			break;
		case DOUBLE:
			if (tc->type == 0)
				tc->type = DOUBLE;
			else if (tc->type == LONG)
				tc->type = LDOUBLE;
			else
				tc->err = 1;
			break;
		case SHORT:
			if (tc->type == 0 || tc->type == INT)
				tc->type = SHORT;
			else
				tc->err = 1;
			break;
		case INT:
			if (tc->type == SHORT || tc->type == LONG ||
			    tc->type == LONGLONG)
				break;
			else if (tc->type == 0)
				tc->type = INT;
			else
				tc->err = 1;
			break;
		case LONG:
			if (tc->type == 0)
				tc->type = LONG;
			else if (tc->type == INT)
				break;
			else if (tc->type == LONG)
				tc->type = LONGLONG;
			else if (tc->type == DOUBLE)
				tc->type = LDOUBLE;
			else
				tc->err = 1;
			break;
		case SIGNED:
			if (tc->sig || tc->uns)
				tc->err = 1;
			tc->sig = 1;
			break;
		case UNSIGNED:
			if (tc->sig || tc->uns)
				tc->err = 1;
			tc->uns = 1;
			break;
		case COMPLEX:
			tc->cmplx = 1;
			break;
		case IMAG:
			tc->imag = 1;
			break;
		default:
			cerror("typwalk");
		}
	}

}

NODE *
typenode(NODE *p)
{
	struct symtab *sp;
	struct typctx tc;
	NODE *q;
	char *c;

	memset(&tc, 0, sizeof(struct typctx));

	flist(p, typwalk, &tc);
	tfree(p);

	if (tc.err)
		goto bad;

	if (tc.cmplx || tc.imag) {
		if (tc.type == 0)
			tc.type = DOUBLE;
		if ((tc.cmplx && tc.imag) || tc.sig || tc.uns ||
		    !ISFTY(tc.type))
			goto bad;
		if (tc.cmplx) {
			c = tc.type == DOUBLE ? "0d" :
			    tc.type == FLOAT ? "0f" : "0l";
			sp = lookup(addname(c), 0);
			tc.type = STRTY;
			tc.saved = mkty(tc.type, sp->sdf, sp->sap);
			tc.saved->n_sp = sp;
			tc.type = 0;
		} else
			tc.type += (FIMAG-FLOAT);
	}

	if (tc.saved && tc.type)
		goto bad;
	if (tc.sig || tc.uns) {
		if (tc.type == 0)
			tc.type = tc.sig ? INT : UNSIGNED;
		if (tc.type > ULONGLONG)
			goto bad;
		if (tc.uns)
			tc.type = ENUNSIGN(tc.type);
	}

	if (xuchar && tc.type == CHAR && tc.sig == 0)
		tc.type = UCHAR;

#ifdef GCC_COMPAT
	if (pragma_packed) {
		q = bdty(CALL, bdty(NAME, "packed"), bcon(pragma_packed));
		tc.post = attr_add(tc.post, gcc_attr_parse(q));
	}
	if (pragma_aligned) {
		/* Deal with relevant pragmas */
		q = bdty(CALL, bdty(NAME, "aligned"), bcon(pragma_aligned));
		tc.post = attr_add(tc.post, gcc_attr_parse(q));
	}
	pragma_aligned = pragma_packed = 0;
#endif
	if ((q = tc.saved) == NULL) {
		TWORD t;
		if ((t = BTYPE(tc.type)) > LDOUBLE && t != VOID &&
		    t != BOOL && !(t >= FIMAG && t <= LIMAG))
			cerror("typenode2 t %x", tc.type);
		if (t == UNDEF) {
			t = INT;
			MODTYPE(tc.type, INT);
		}
		q =  mkty(tc.type, 0, 0);
	}
	q->n_ap = attr_add(q->n_ap, tc.post);
	q->n_qual = tc.qual;
	q->n_lval = tc.class;
#ifdef GCC_COMPAT
	if (tc.post) {
		/* Can only occur for TYPEDEF, STRUCT or UNION */
		if (tc.saved == NULL)
			cerror("typenode");
		if (tc.saved->n_sp) /* trailer attributes for structs */
			tc.saved->n_sp->sap = q->n_ap;
	}
	if (tc.pre)
		q->n_ap = attr_add(q->n_ap, tc.pre);
	gcc_tcattrfix(q);
#endif
	return q;

bad:	uerror("illegal type combination");
	return mkty(INT, 0, 0);
}

struct tylnk {
	struct tylnk *next;
	union dimfun df;
};

/*
 * Retrieve all CM-separated argument types, sizes and dimensions and
 * put them in an array.
 * XXX - can only check first type level, side effects?
 */
static union arglist *
arglist(NODE *n)
{
	union arglist *al;
	NODE *w = n, **ap;
	int num, cnt, i, j, k;
	TWORD ty;

#ifdef PCC_DEBUG
	if (pdebug) {
		printf("arglist %p\n", n);
		fwalk(n, eprint, 0);
	}
#endif
	/* First: how much to allocate */
	for (num = cnt = 0, w = n; w->n_op == CM; w = w->n_left) {
		cnt++;	/* Number of levels */
		num++;	/* At least one per step */
		if (w->n_right->n_op == ELLIPSIS)
			continue;
		ty = w->n_right->n_type;
		if (ty == ENUMTY) {
			uerror("arg %d enum undeclared", cnt);
			ty = w->n_right->n_type = INT;
		}
		if (BTYPE(ty) == STRTY || BTYPE(ty) == UNIONTY)
			num++;
		while (!ISFTN(ty) && !ISARY(ty) && ty > BTMASK)
			ty = DECREF(ty);
		if (ty > BTMASK)
			num++;
	}
	cnt++;
	ty = w->n_type;
	if (ty == ENUMTY) {
		uerror("arg %d enum undeclared", cnt);
		ty = w->n_type = INT;
	}
	if (BTYPE(ty) == STRTY || BTYPE(ty) == UNIONTY)
		num++;
	while (!ISFTN(ty) && !ISARY(ty) && ty > BTMASK)
		ty = DECREF(ty);
	if (ty > BTMASK)
		num++;
	num += 2; /* TEND + last arg type */

	/* Second: Create list to work on */
	ap = tmpalloc(sizeof(NODE *) * cnt);
	al = permalloc(sizeof(union arglist) * num);
	arglistcnt += num;

	for (w = n, i = 0; w->n_op == CM; w = w->n_left)
		ap[i++] = w->n_right;
	ap[i] = w;

	/* Third: Create actual arg list */
	for (k = 0, j = i; j >= 0; j--) {
		if (ap[j]->n_op == ELLIPSIS) {
			al[k++].type = TELLIPSIS;
			ap[j]->n_op = ICON; /* for tfree() */
			continue;
		}
		/* Convert arrays to pointers */
		if (ISARY(ap[j]->n_type)) {
			ap[j]->n_type += (PTR-ARY);
			ap[j]->n_df++;
		}
		/* Convert (silently) functions to pointers */
		if (ISFTN(ap[j]->n_type))
			ap[j]->n_type = INCREF(ap[j]->n_type);
		ty = ap[j]->n_type;
#ifdef GCC_COMPAT
		if (ty == UNIONTY &&
		    attr_find(ap[j]->n_ap, GCC_ATYP_TRANSP_UNION)){
			/* transparent unions must have compatible types
			 * shortcut here: if pointers, set void *, 
			 * otherwise btype.
			 */
			struct symtab *sp = strmemb(ap[j]->n_ap);
			ty = ISPTR(sp->stype) ? PTR|VOID : sp->stype;
		}
#endif
		al[k++].type = ty;
		if (BTYPE(ty) == STRTY || BTYPE(ty) == UNIONTY)
			al[k++].sap = ap[j]->n_ap;
		while (!ISFTN(ty) && !ISARY(ty) && ty > BTMASK)
			ty = DECREF(ty);
		if (ty > BTMASK)
			al[k++].df = ap[j]->n_df;
	}
	al[k++].type = TNULL;
	if (k > num)
		cerror("arglist: k%d > num%d", k, num);
	tfree(n);
#ifdef PCC_DEBUG
	if (pdebug)
		alprint(al, 0);
#endif
	return al;
}

static void
tylkadd(union dimfun dim, struct tylnk **tylkp, int *ntdim)
{
	(*tylkp)->next = tmpalloc(sizeof(struct tylnk));
	*tylkp = (*tylkp)->next;
	(*tylkp)->next = NULL;
	(*tylkp)->df = dim;
	(*ntdim)++;
}

/*
 * build a type, and stash away dimensions,
 * from a parse tree of the declaration
 * the type is build top down, the dimensions bottom up
 */
static void
tyreduce(NODE *p, struct tylnk **tylkp, int *ntdim)
{
	union dimfun dim;
	NODE *r = NULL;
	int o;
	TWORD t, q;

	o = p->n_op;
	if (o == NAME) {
		p->n_qual = DECQAL(p->n_qual);
		return;
	}

	t = INCREF(p->n_type);
	q = p->n_qual;
	switch (o) {
	case CALL:
		t += (FTN-PTR);
		dim.dfun = arglist(p->n_right);
		break;
	case UCALL:
		t += (FTN-PTR);
		dim.dfun = NULL;
		break;
	case LB:
		t += (ARY-PTR);
		if (p->n_right->n_op != ICON) {
			r = p->n_right;
			o = RB;
		} else {
			dim.ddim = (int)p->n_right->n_lval;
			nfree(p->n_right);
#ifdef notdef
	/* XXX - check dimensions at usage time */
			if (dim.ddim == NOOFFSET && p->n_left->n_op == LB)
				uerror("null dimension");
#endif
		}
		break;
	}

	p->n_left->n_type = t;
	p->n_left->n_qual = INCQAL(q) | p->n_left->n_qual;
	tyreduce(p->n_left, tylkp, ntdim);

	if (o == LB || o == UCALL || o == CALL)
		tylkadd(dim, tylkp, ntdim);
	if (o == RB) {
		dim.ddim = -1;
		tylkadd(dim, tylkp, ntdim);
		arrstk[arrstkp++] = r;
	}

	p->n_sp = p->n_left->n_sp;
	p->n_type = p->n_left->n_type;
	p->n_qual = p->n_left->n_qual;
}

/*
 * merge type typ with identifier idp.
 * idp is returned as a NAME node with correct types,
 * typ is untouched since multiple declarations uses it.
 * typ has type attributes, idp can never carry such attributes
 * so on return just a pointer to the typ attributes is returned.
 */
NODE *
tymerge(NODE *typ, NODE *idp)
{
	TWORD t;
	NODE *p;
	union dimfun *j;
	struct tylnk *base, tylnk, *tylkp;
	struct attr *bap;
	int ntdim, i;

#ifdef PCC_DEBUG
	if (ddebug > 2) {
		printf("tymerge(%p,%p)\n", typ, idp);
		fwalk(typ, eprint, 0);
		fwalk(idp, eprint, 0);
	}
#endif

	if (typ->n_op != TYPE)
		cerror("tymerge: arg 1");

	bap = typ->n_ap;

	idp->n_type = typ->n_type;
	idp->n_qual |= typ->n_qual;

	tylkp = &tylnk;
	tylkp->next = NULL;
	ntdim = 0;

	tyreduce(idp, &tylkp, &ntdim);

	for (t = typ->n_type, j = typ->n_df; t&TMASK; t = DECREF(t))
		if (ISARY(t) || ISFTN(t))
			tylkadd(*j++, &tylkp, &ntdim);

	if (ntdim) {
		union dimfun *a = permalloc(sizeof(union dimfun) * ntdim);
		dimfuncnt += ntdim;
		for (i = 0, base = tylnk.next; base; base = base->next, i++)
			a[i] = base->df;
		idp->n_df = a;
	} else
		idp->n_df = NULL;

	/* now idp is a single node: fix up type */
	if ((t = ctype(idp->n_type)) != idp->n_type)
		idp->n_type = t;
	
	if (idp->n_op != NAME) {
		for (p = idp->n_left; p->n_op != NAME; p = p->n_left)
			nfree(p);
		nfree(p);
		idp->n_op = NAME;
	}
	/* carefully not destroy any type attributes */
	if (idp->n_ap != NULL) {
		struct attr *ap = idp->n_ap;
		while (ap->next)
			ap = ap->next;
		ap->next = bap;
	} else
		idp->n_ap = bap;

	return(idp);
}

static NODE *
argcast(NODE *p, TWORD t, union dimfun *d, struct attr *ap)
{
	NODE *u, *r = talloc();

	r->n_op = NAME;
	r->n_type = t;
	r->n_qual = 0; /* XXX */
	r->n_df = d;
	r->n_ap = ap;

	u = buildtree(CAST, r, p);
	nfree(u->n_left);
	r = u->n_right;
	nfree(u);
	return r;
}

#ifdef PCC_DEBUG
/*
 * Print a prototype.
 */
static void
alprint(union arglist *al, int in)
{
	TWORD t;
	int i = 0, j;

	for (; al->type != TNULL; al++) {
		for (j = in; j > 0; j--)
			printf("  ");
		printf("arg %d: ", i++);
		t = al->type;
		tprint(t, 0);
		while (t > BTMASK) {
			if (ISARY(t)) {
				al++;
				printf(" dim %d ", al->df->ddim);
			} else if (ISFTN(t)) {
				al++;
				if (al->df->dfun) {
					printf("\n");
					alprint(al->df->dfun, in+1);
				}
			}
			t = DECREF(t);
		}
		if (ISSOU(t)) {
			al++;
			printf(" (size %d align %d)", (int)tsize(t, 0, al->sap),
			    (int)talign(t, al->sap));
		}
		printf("\n");
	}
	if (in == 0)
		printf("end arglist\n");
}
#endif

int
suemeq(struct attr *s1, struct attr *s2)
{

	return (strmemb(s1) == strmemb(s2));
}

/*
 * Sanity-check old-style args.
 */
static NODE *
oldarg(NODE *p)
{
	if (p->n_op == TYPE)
		uerror("type is not an argument");
	if (p->n_type == FLOAT)
		return cast(p, DOUBLE, p->n_qual);
	return p;
}

/*
 * Do prototype checking and add conversions before calling a function.
 * Argument f is function and a is a CM-separated list of arguments.
 * Returns a merged node (via buildtree() of function and arguments.
 */
NODE *
doacall(struct symtab *sp, NODE *f, NODE *a)
{
	NODE *w, *r;
	union arglist *al;
	struct ap {
		struct ap *next;
		NODE *node;
	} *at, *apole = NULL;
	int argidx/* , hasarray = 0*/;
	TWORD type, arrt;

#ifdef PCC_DEBUG
	if (ddebug) {
		printf("doacall.\n");
		fwalk(f, eprint, 0);
		if (a)
			fwalk(a, eprint, 0);
	}
#endif

	/* First let MD code do something */
	calldec(f, a);
/* XXX XXX hack */
	if ((f->n_op == CALL) &&
	    f->n_left->n_op == ADDROF &&
	    f->n_left->n_left->n_op == NAME &&
	    (f->n_left->n_left->n_type & 0x7e0) == 0x4c0)
		goto build;
/* XXX XXX hack */

	/* Check for undefined or late defined enums */
	if (BTYPE(f->n_type) == ENUMTY) {
		/* not-yet check if declared enum */
		struct symtab *sq = strmemb(f->n_ap);
		if (sq->stype != ENUMTY)
			MODTYPE(f->n_type, sq->stype);
		if (BTYPE(f->n_type) == ENUMTY)
			uerror("enum %s not declared", sq->sname);
	}

	/*
	 * Do some basic checks.
	 */
	if (f->n_df == NULL || (al = f->n_df[0].dfun) == NULL) {
		/*
		 * Handle non-prototype declarations.
		 */
		if (f->n_op == NAME && f->n_sp != NULL) {
			if (strncmp(f->n_sp->sname, "__builtin", 9) != 0 &&
			    (f->n_sp->sflags & SINSYS) == 0)
				warner(Wmissing_prototypes, f->n_sp->sname);
		} else
			warner(Wmissing_prototypes, "<pointer>");

		/* floats must be cast to double */
		if (a == NULL)
			goto build;
		if (a->n_op != CM) {
			a = oldarg(a);
		} else {
			for (w = a; w->n_left->n_op == CM; w = w->n_left)
				w->n_right = oldarg(w->n_right);
			w->n_left = oldarg(w->n_left);
			w->n_right = oldarg(w->n_right);
		}
		goto build;
	}
	if (al->type == VOID) {
		if (a != NULL)
			uerror("function takes no arguments");
		goto build; /* void function */
	} else {
		if (a == NULL) {
			uerror("function needs arguments");
			goto build;
		}
	}
#ifdef PCC_DEBUG
	if (pdebug) {
		printf("arglist for %s\n",
		    f->n_sp != NULL ? f->n_sp->sname : "function pointer");
		alprint(al, 0);
	}
#endif

	/*
	 * Create a list of pointers to the nodes given as arg.
	 */
	for (w = a; w->n_op == CM; w = w->n_left) {
		at = tmpalloc(sizeof(struct ap));
		at->node = w->n_right;
		at->next = apole;
		apole = at;
	}
	at = tmpalloc(sizeof(struct ap));
	at->node = w;
	at->next = apole;
	apole = at;

	/*
	 * Do the typechecking by walking up the list.
	 */
	argidx = 1;
	while (al->type != TNULL) {
		if (al->type == TELLIPSIS) {
			/* convert the rest of float to double */
			for (; apole; apole = apole->next) {
				if (apole->node->n_type != FLOAT)
					continue;
				MKTY(apole->node, DOUBLE, 0, 0);
			}
			goto build;
		}
		if (apole == NULL) {
			uerror("too few arguments to function");
			goto build;
		}
/* al = prototyp, apole = argument till ftn */
/* type = argumentets typ, arrt = prototypens typ */
		type = apole->node->n_type;
		arrt = al->type;
#if 0
		if ((hasarray = ISARY(arrt)))
			arrt += (PTR-ARY);
#endif
		/* Taking addresses of arrays are meaningless in expressions */
		/* but people tend to do that and also use in prototypes */
		/* this is mostly a problem with typedefs */
		if (ISARY(type)) {
			if (ISPTR(arrt) && ISARY(DECREF(arrt)))
				type = INCREF(type);
			else
				type += (PTR-ARY);
		} else if (ISPTR(type) && !ISARY(DECREF(type)) &&
		    ISPTR(arrt) && ISARY(DECREF(arrt))) {
			type += (ARY-PTR);
			type = INCREF(type);
		}

		/* Check structs */
		if (type <= BTMASK && arrt <= BTMASK) {
			if (type != arrt) {
				if (ISSOU(BTYPE(type)) || ISSOU(BTYPE(arrt))) {
incomp:					uerror("incompatible types for arg %d",
					    argidx);
				} else {
					MKTY(apole->node, arrt, 0, 0)
				}
#ifndef NO_COMPLEX
			} else if (type == STRTY &&
			    attr_find(apole->node->n_ap, ATTR_COMPLEX) &&
			    attr_find(al[1].sap, ATTR_COMPLEX)) {
				/* Both are complex */
				if (strmemb(apole->node->n_ap)->stype !=
				    strmemb(al[1].sap)->stype) {
					/* must convert to correct type */
					w = talloc();
					*w = *apole->node;
					w = mkcmplx(w,
					    strmemb(al[1].sap)->stype);
					*apole->node = *w;
					nfree(w);
				}
				goto out;
#endif
			} else if (ISSOU(BTYPE(type))) {
				if (!suemeq(apole->node->n_ap, al[1].sap))
					goto incomp;
			}
			goto out;
		}

		/* XXX should (recusively) check return type and arg list of
		   func ptr arg XXX */
		if (ISFTN(DECREF(arrt)) && ISFTN(type))
			type = INCREF(type);

		/* Hereafter its only pointers (or arrays) left */
		/* Check for struct/union intermixing with other types */
		if (((type <= BTMASK) && ISSOU(BTYPE(type))) ||
		    ((arrt <= BTMASK) && ISSOU(BTYPE(arrt))))
			goto incomp;

		/* Check for struct/union compatibility */
		if (type == arrt) {
			if (ISSOU(BTYPE(type))) {
				if (suemeq(apole->node->n_ap, al[1].sap))
					goto out;
			} else
				goto out;
		}
		if (BTYPE(arrt) == VOID && type > BTMASK)
			goto skip; /* void *f = some pointer */
		if (arrt > BTMASK && BTYPE(type) == VOID)
			goto skip; /* some *f = void pointer */
		if (apole->node->n_op == ICON && apole->node->n_lval == 0)
			goto skip; /* Anything assigned a zero */

		if ((type & ~BTMASK) == (arrt & ~BTMASK)) {
			/* do not complain for pointers with signedness */
			if ((DEUNSIGN(BTYPE(type)) == DEUNSIGN(BTYPE(arrt))) &&
			    (BTYPE(type) != BTYPE(arrt))) {
				warner(Wpointer_sign, NULL);
				goto skip;
			}
		}

		werror("implicit conversion of argument %d due to prototype",
		    argidx);

skip:		if (ISSOU(BTYPE(arrt))) {
			MKTY(apole->node, arrt, 0, al[1].sap)
		} else {
			MKTY(apole->node, arrt, 0, 0)
		}

out:		al++;
		if (ISSOU(BTYPE(arrt)))
			al++;
#if 0
		while (arrt > BTMASK && !ISFTN(arrt))
			arrt = DECREF(arrt);
		if (ISFTN(arrt) || hasarray)
			al++;
#else
		while (arrt > BTMASK) {
			if (ISARY(arrt) || ISFTN(arrt)) {
				al++;
				break;
			}
			arrt = DECREF(arrt);
		}
#endif
		apole = apole->next;
		argidx++;
	}
	if (apole != NULL)
		uerror("too many arguments to function");

build:	if (sp != NULL && (sp->sflags & SINLINE) && (w = inlinetree(sp, f, a)))
		return w;
	return buildtree(a == NIL ? UCALL : CALL, f, a);
}

static int
chk2(TWORD type, union dimfun *dsym, union dimfun *ddef)
{
	while (type > BTMASK) {
		switch (type & TMASK) {
		case ARY:
			/* may be declared without dimension */
			if (dsym->ddim == NOOFFSET)
				dsym->ddim = ddef->ddim;
			if (dsym->ddim < 0 && ddef->ddim < 0)
				; /* dynamic arrays as arguments */
			else if (ddef->ddim > 0 && dsym->ddim != ddef->ddim)
				return 1;
			dsym++, ddef++;
			break;
		case FTN:
			/* old-style function headers with function pointers
			 * will most likely not have a prototype.
			 * This is not considered an error.  */
			if (ddef->dfun == NULL) {
#ifdef notyet
				werror("declaration not a prototype");
#endif
			} else if (chkftn(dsym->dfun, ddef->dfun))
				return 1;
			dsym++, ddef++;
			break;
		}
		type = DECREF(type);
	}
	return 0;
}

/*
 * Compare two function argument lists to see if they match.
 */
int
chkftn(union arglist *usym, union arglist *udef)
{
	TWORD t2;
	int ty, tyn;

	if (usym == NULL)
		return 0;
	if (cftnsp != NULL && udef == NULL && usym->type == VOID)
		return 0; /* foo() { function with foo(void); prototype */
	if (udef == NULL && usym->type != TNULL)
		return 1;
	while (usym->type != TNULL) {
		if (usym->type == udef->type)
			goto done;
		/*
		 * If an old-style declaration, then all types smaller than
		 * int are given as int parameters.
		 */
		if (intcompare) {
			ty = BTYPE(usym->type);
			tyn = BTYPE(udef->type);
			if (ty == tyn || ty != INT)
				return 1;
			if (tyn == CHAR || tyn == UCHAR ||
			    tyn == SHORT || tyn == USHORT)
				goto done;
			return 1;
		} else
			return 1;

done:		ty = BTYPE(usym->type);
		t2 = usym->type;
		if (ISSOU(ty)) {
			usym++, udef++;
			if (suemeq(usym->sap, udef->sap) == 0)
				return 1;
		}

		while (!ISFTN(t2) && !ISARY(t2) && t2 > BTMASK)
			t2 = DECREF(t2);
		if (t2 > BTMASK) {
			usym++, udef++;
			if (chk2(t2, usym->df, udef->df))
				return 1;
		}
		usym++, udef++;
	}
	if (usym->type != udef->type)
		return 1;
	return 0;
}

void
fixtype(NODE *p, int class)
{
	unsigned int t, type;
	int mod1, mod2;
	/* fix up the types, and check for legality */

	/* forward declared enums */
	if (BTYPE(p->n_sp->stype) == ENUMTY) {
		MODTYPE(p->n_sp->stype, strmemb(p->n_sp->sap)->stype);
	}

	if( (type = p->n_type) == UNDEF ) return;
	if ((mod2 = (type&TMASK))) {
		t = DECREF(type);
		while( mod1=mod2, mod2 = (t&TMASK) ){
			if( mod1 == ARY && mod2 == FTN ){
				uerror( "array of functions is illegal" );
				type = 0;
				}
			else if( mod1 == FTN && ( mod2 == ARY || mod2 == FTN ) ){
				uerror( "function returns illegal type" );
				type = 0;
				}
			t = DECREF(t);
			}
		}

	/* detect function arguments, watching out for structure declarations */
	if (rpole && ISFTN(type)) {
		uerror("function illegal in structure or union");
		type = INCREF(type);
	}
	p->n_type = type;
}

/*
 * give undefined version of class
 */
int
uclass(int class)
{
	if (class == SNULL)
		return(EXTERN);
	else if (class == STATIC)
		return(USTATIC);
	else
		return(class);
}

int
fixclass(int class, TWORD type)
{
	extern int fun_inline;

	/* first, fix null class */
	if (class == SNULL) {
		if (fun_inline && ISFTN(type))
			return SNULL;
		if (rpole)
			cerror("field8");
		else if (blevel == 0)
			class = EXTDEF;
		else
			class = AUTO;
	}

	/* now, do general checking */

	if( ISFTN( type ) ){
		switch( class ) {
		default:
			uerror( "function has illegal storage class" );
		case AUTO:
			class = EXTERN;
		case EXTERN:
		case EXTDEF:
		case TYPEDEF:
		case STATIC:
		case USTATIC:
			;
			}
		}

	if (class & FIELD) {
		cerror("field3");
	}

	switch (class) {

	case MOS:
	case MOU:
		cerror("field4");

	case REGISTER:
		if (blevel == 0)
			uerror("illegal register declaration");
		if (blevel == 1)
			return(PARAM);
		else
			return(REGISTER);

	case AUTO:
		if( blevel < 2 ) uerror( "illegal ULABEL class" );
		return( class );

	case EXTERN:
	case STATIC:
	case EXTDEF:
	case TYPEDEF:
	case USTATIC:
	case PARAM:
		return( class );

	default:
		cerror( "illegal class: %d", class );
		/* NOTREACHED */

	}
	return 0; /* XXX */
}

/*
 * Generates a goto statement; sets up label number etc.
 */
void
gotolabel(char *name)
{
	struct symtab *s = lookup(name, SLBLNAME);

	if (s->soffset == 0) {
		s->soffset = -getlab();
		s->sclass = STATIC;
	}
	branch(s->soffset < 0 ? -s->soffset : s->soffset);
}

/*
 * Sets a label for gotos.
 */
void
deflabel(char *name, NODE *p)
{
	struct symtab *s = lookup(name, SLBLNAME);

#ifdef GCC_COMPAT
	s->sap = gcc_attr_parse(p);
#endif
	if (s->soffset > 0)
		uerror("label '%s' redefined", name);
	if (s->soffset == 0) {
		s->soffset = getlab();
		s->sclass = STATIC;
	}
	if (s->soffset < 0)
		s->soffset = -s->soffset;
	plabel( s->soffset);
}

struct symtab *
getsymtab(char *name, int flags)
{
	struct symtab *s;

	if (flags & STEMP) {
		s = tmpalloc(sizeof(struct symtab));
	} else {
		s = permalloc(sizeof(struct symtab));
		symtabcnt++;
	}
	s->sname = name;
	s->soname = NULL;
	s->snext = NULL;
	s->stype = UNDEF;
	s->squal = 0;
	s->sclass = SNULL;
	s->sflags = (short)(flags & SMASK);
	s->soffset = 0;
	s->slevel = (char)blevel;
	s->sdf = NULL;
	s->sap = NULL;
	return s;
}

int
fldchk(int sz)
{
	if (rpole->rsou != STNAME && rpole->rsou != UNAME)
		uerror("field outside of structure");
	if (sz < 0 || sz >= FIELD) {
		uerror("illegal field size");
		return 1;
	}
	return 0;
}

#ifdef PCC_DEBUG
static char *
ccnames[] = { /* names of storage classes */
	"SNULL",
	"AUTO",
	"EXTERN",
	"STATIC",
	"REGISTER",
	"EXTDEF",
	"LABEL",
	"ULABEL",
	"MOS",
	"PARAM",
	"STNAME",
	"MOU",
	"UNAME",
	"TYPEDEF",
	"FORTRAN",
	"ENAME",
	"MOE",
	"UFORTRAN",
	"USTATIC",
	};

char *
scnames(int c)
{
	/* return the name for storage class c */
	static char buf[12];
	if( c&FIELD ){
		snprintf( buf, sizeof(buf), "FIELD[%d]", c&FLDSIZ );
		return( buf );
		}
	return( ccnames[c] );
	}
#endif

#ifdef os_openbsd
static char *stack_chk_fail = "__stack_smash_handler";
static char *stack_chk_guard = "__guard";
#else
static char *stack_chk_fail = "__stack_chk_fail";
static char *stack_chk_guard = "__stack_chk_guard";
#endif
static char *stack_chk_canary = "__stack_chk_canary";

void
sspinit(void)
{
	NODE *p;

	p = block(NAME, NIL, NIL, FTN+VOID, 0, 0);
	p->n_sp = lookup(stack_chk_fail, SNORMAL);
	defid(p, EXTERN);
	nfree(p);

	p = block(NAME, NIL, NIL, INT, 0, 0);
	p->n_sp = lookup(stack_chk_guard, SNORMAL);
	defid(p, EXTERN);
	nfree(p);
}

void
sspstart(void)
{
	NODE *p, *q;

	q = block(NAME, NIL, NIL, INT, 0, 0);
 	q->n_sp = lookup(stack_chk_guard, SNORMAL);
	q = clocal(q);

	p = block(REG, NIL, NIL, INCREF(INT), 0, 0);
	p->n_lval = 0;
	p->n_rval = FPREG;
	p = cast(p, INT, 0);
	q = buildtree(ER, p, q);

	p = block(NAME, NIL, NIL, INT, 0, 0);
	p->n_qual = VOL >> TSHIFT;
	p->n_sp = lookup(stack_chk_canary, SNORMAL);
	defid(p, AUTO);
	p = clocal(p);
	ecomp(buildtree(ASSIGN, p, q));
}

void
sspend(void)
{
	NODE *p, *q;
	TWORD t;
	int lab;

	if (retlab != NOLAB) {
		plabel(retlab);
		retlab = getlab();
	}

	t = DECREF(cftnsp->stype);
	if (t == BOOL)
		t = BOOL_TYPE;

	p = block(NAME, NIL, NIL, INT, 0, 0);
	p->n_sp = lookup(stack_chk_canary, SNORMAL);
	p = clocal(p);

	q = block(REG, NIL, NIL, INCREF(INT), 0, 0);
	q->n_lval = 0;
	q->n_rval = FPREG;
	q = cast(q, INT, 0);
	q = buildtree(ER, p, q);

	p = block(NAME, NIL, NIL, INT, 0, 0);
	p->n_sp = lookup(stack_chk_guard, SNORMAL);
	p = clocal(p);

	lab = getlab();
	cbranch(buildtree(EQ, p, q), bcon(lab));

	p = block(NAME, NIL, NIL, FTN+VOID, 0, 0);
	p->n_sp = lookup(stack_chk_fail, SNORMAL);
	p = clocal(p);

	q = eve(bdty(STRING, cftnsp->sname, 0));
	ecomp(buildtree(CALL, p, q));

	plabel(lab);
}

/*
 * Allocate on the permanent heap for inlines, otherwise temporary heap.
 */
void *
blkalloc(int size)
{
	return isinlining || blevel < 2 ?  permalloc(size) : tmpalloc(size);
}

/*
 * Allocate on the permanent heap for inlines, otherwise temporary heap.
 */
void *
inlalloc(int size)
{
	return isinlining ?  permalloc(size) : tmpalloc(size);
}

struct attr *
attr_new(int type, int nelem)
{
	struct attr *ap;
	int sz;

	sz = sizeof(struct attr) + nelem * sizeof(union aarg);

	ap = memset(blkalloc(sz), 0, sz);
	ap->atype = type;
	return ap;
}

/*
 * Add attribute list new before old and return new.
 */
struct attr *
attr_add(struct attr *old, struct attr *new)
{
	struct attr *ap;

	if (new == NULL)
		return old; /* nothing to add */

	for (ap = new; ap->next; ap = ap->next)
		;
	ap->next = old;
	return new;
}

/*
 * Search for attribute type in list ap.  Return entry or NULL.
 */
struct attr *
attr_find(struct attr *ap, int type)
{

	for (; ap && ap->atype != type; ap = ap->next)
		;
	return ap;
}

/*
 * Copy an attribute struct.
 * Return destination.
 */
struct attr *
attr_copy(struct attr *aps, struct attr *apd, int n)
{
	int sz = sizeof(struct attr) + n * sizeof(union aarg);
	return memcpy(apd, aps, sz);
}

/*
 * Duplicate an attribute, like strdup.
 */
struct attr *
attr_dup(struct attr *ap, int n)
{
	int sz = sizeof(struct attr) + n * sizeof(union aarg);
	ap = memcpy(blkalloc(sz), ap, sz);
	ap->next = NULL;
	return ap;
}

/*
 * Fetch pointer to first member in a struct list.
 */
struct symtab *
strmemb(struct attr *ap)
{

	if ((ap = attr_find(ap, ATTR_STRUCT)) == NULL)
		cerror("strmemb");
	return ap->amlist;
}

#ifndef NO_COMPLEX

static char *real, *imag;
static struct symtab *cxsp[3], *cxmul[3], *cxdiv[3];
static char *cxnmul[] = { "__mulsc3", "__muldc3", "__mulxc3" };
static char *cxndiv[] = { "__divsc3", "__divdc3", "__divxc3" };
/*
 * As complex numbers internally are handled as structs, create
 * these by hand-crafting them.
 */
void
complinit(void)
{
	struct attr *ap;
	struct rstack *rp;
	NODE *p, *q;
	char *n[] = { "0f", "0d", "0l" };
	int i, d_debug;

	d_debug = ddebug;
	ddebug = 0;
	real = addname("__real");
	imag = addname("__imag");
	p = block(NAME, NIL, NIL, FLOAT, 0, 0);
	for (i = 0; i < 3; i++) {
		p->n_type = FLOAT+i;
		rpole = rp = bstruct(NULL, STNAME, NULL);
		soumemb(p, real, 0);
		soumemb(p, imag, 0);
		q = dclstruct(rp);
		cxsp[i] = q->n_sp = lookup(addname(n[i]), 0);
		defid(q, TYPEDEF);
		ap = attr_new(ATTR_COMPLEX, 0);
		q->n_sp->sap = attr_add(q->n_sp->sap, ap);
		nfree(q);
	}
	/* create function declarations for external ops */
	for (i = 0; i < 3; i++) {
		cxnmul[i] = addname(cxnmul[i]);
		p->n_sp = cxmul[i] = lookup(cxnmul[i], 0);
		p->n_type = FTN|STRTY;
		p->n_ap = cxsp[i]->sap;
		p->n_df = cxsp[i]->sdf;
		defid2(p, EXTERN, 0);
		cxmul[i]->sdf = permalloc(sizeof(union dimfun));
		cxmul[i]->sdf->dfun = NULL;
		cxndiv[i] = addname(cxndiv[i]);
		p->n_sp = cxdiv[i] = lookup(cxndiv[i], 0);
		p->n_type = FTN|STRTY;
		p->n_ap = cxsp[i]->sap;
		p->n_df = cxsp[i]->sdf;
		defid2(p, EXTERN, 0);
		cxdiv[i]->sdf = permalloc(sizeof(union dimfun));
		cxdiv[i]->sdf->dfun = NULL;
	}
	nfree(p);
	ddebug = d_debug;
}

/*
 * Return the highest real floating point type.
 * Known that at least one type is complex or imaginary.
 */
static TWORD
maxtyp(NODE *l, NODE *r)
{
	TWORD tl, tr, t;

	tl = ANYCX(l) ? strmemb(l->n_ap)->stype : l->n_type;
	tr = ANYCX(r) ? strmemb(r->n_ap)->stype : r->n_type;
	if (ISITY(tl))
		tl -= (FIMAG - FLOAT);
	if (ISITY(tr))
		tr -= (FIMAG - FLOAT);
	t = tl > tr ? tl : tr;
	if (!ISFTY(t))
		cerror("maxtyp");
	return t;
}

/*
 * Fetch space on stack for complex struct.
 */
static NODE *
cxstore(TWORD t)
{
	struct symtab s;

	s = *cxsp[t - FLOAT];
	s.sclass = AUTO;
	s.soffset = NOOFFSET;
	oalloc(&s, &autooff);
	return nametree(&s);
}

#define	comop(x,y) buildtree(COMOP, x, y)

/*
 * Convert node p to complex type dt.
 */
static NODE *
mkcmplx(NODE *p, TWORD dt)
{
	NODE *q, *r, *i, *t;

	if (!ANYCX(p)) {
		/* Not complex, convert to complex on stack */
		q = cxstore(dt);
		if (ISITY(p->n_type)) {
			p->n_type = p->n_type - FIMAG + FLOAT;
			r = bcon(0);
			i = p;
		} else {
			r = p;
			i = bcon(0);
		}
		p = buildtree(ASSIGN, structref(ccopy(q), DOT, real), r);
		p = comop(p, buildtree(ASSIGN, structref(ccopy(q), DOT, imag), i));
		p = comop(p, q);
	} else {
		if (strmemb(p->n_ap)->stype != dt) {
			q = cxstore(dt);
			p = buildtree(ADDROF, p, NIL);
			t = tempnode(0, p->n_type, p->n_df, p->n_ap);
			p = buildtree(ASSIGN, ccopy(t), p);
			p = comop(p, buildtree(ASSIGN,
			    structref(ccopy(q), DOT, real),
			    structref(ccopy(t), STREF, real)));
			p = comop(p, buildtree(ASSIGN,
			    structref(ccopy(q), DOT, imag),
			    structref(t, STREF, imag)));
			p = comop(p, q);
		}
	}
	return p;
}

static NODE *
cxasg(NODE *l, NODE *r)
{
	TWORD tl, tr;

	tl = strattr(l->n_ap) ? strmemb(l->n_ap)->stype : 0;
	tr = strattr(r->n_ap) ? strmemb(r->n_ap)->stype : 0;

	if (ANYCX(l) && ANYCX(r) && tl != tr) {
		/* different types in structs */
		r = mkcmplx(r, tl);
	} else if (!ANYCX(l))
		r = structref(r, DOT, ISITY(l->n_type) ? imag : real);
	else if (!ANYCX(r))
		r = mkcmplx(r, tl);
	return buildtree(ASSIGN, l, r);
}

/*
 * Fixup complex operations.
 * At least one operand is complex.
 */
NODE *
cxop(int op, NODE *l, NODE *r)
{
	TWORD mxtyp;
	NODE *p, *q;
	NODE *ltemp, *rtemp;
	NODE *real_l, *imag_l;
	NODE *real_r, *imag_r;

	if (op == ASSIGN)
		return cxasg(l, r);

	mxtyp = maxtyp(l, r);
	l = mkcmplx(l, mxtyp);
	if (op != UMINUS)
		r = mkcmplx(r, mxtyp);


	/* put a pointer to left and right elements in a TEMP */
	l = buildtree(ADDROF, l, NIL);
	ltemp = tempnode(0, l->n_type, l->n_df, l->n_ap);
	l = buildtree(ASSIGN, ccopy(ltemp), l);

	if (op != UMINUS) {
		r = buildtree(ADDROF, r, NIL);
		rtemp = tempnode(0, r->n_type, r->n_df, r->n_ap);
		r = buildtree(ASSIGN, ccopy(rtemp), r);

		p = comop(l, r);
	} else
		p = l;

	/* create the four trees needed for calculation */
	real_l = structref(ccopy(ltemp), STREF, real);
	imag_l = structref(ltemp, STREF, imag);
	if (op != UMINUS) {
		real_r = structref(ccopy(rtemp), STREF, real);
		imag_r = structref(rtemp, STREF, imag);
	}

	/* get storage on stack for the result */
	q = cxstore(mxtyp);

	switch (op) {
	case NE:
	case EQ:
		tfree(q);
		p = buildtree(op, comop(p, real_l), real_r);
		q = buildtree(op, imag_l, imag_r);
		p = buildtree(op == EQ ? ANDAND : OROR, p, q);
		return p;

	case ANDAND:
	case OROR: /* go via EQ to get INT of it */
		tfree(q);
		p = buildtree(NE, comop(p, real_l), bcon(0)); /* gets INT */
		q = buildtree(NE, imag_l, bcon(0));
		p = buildtree(OR, p, q);

		q = buildtree(NE, real_r, bcon(0));
		q = buildtree(OR, q, buildtree(NE, imag_r, bcon(0)));

		p = buildtree(op, p, q);
		return p;

	case UMINUS:
		p = comop(p, buildtree(ASSIGN, structref(ccopy(q), DOT, real), 
		    buildtree(op, real_l, NIL)));
		p = comop(p, buildtree(ASSIGN, structref(ccopy(q), DOT, imag), 
		    buildtree(op, imag_l, NIL)));
		break;

	case PLUS:
	case MINUS:
		p = comop(p, buildtree(ASSIGN, structref(ccopy(q), DOT, real), 
		    buildtree(op, real_l, real_r)));
		p = comop(p, buildtree(ASSIGN, structref(ccopy(q), DOT, imag), 
		    buildtree(op, imag_l, imag_r)));
		break;

	case MUL:
	case DIV:
		/* Complex mul is "complex" */
		/* (u+iv)*(x+iy)=((u*x)-(v*y))+i(v*x+y*u) */
		/* Complex div is even more "complex" */
		/* (u+iv)/(x+iy)=(u*x+v*y)/(x*x+y*y)+i((v*x-u*y)/(x*x+y*y)) */
		/* but we need to do it via a subroutine */
		tfree(q);
		p = buildtree(CM, comop(p, real_l), imag_l);
		p = buildtree(CM, p, real_r);
		p = buildtree(CM, p, imag_r);
		q = nametree(op == DIV ?
		    cxdiv[mxtyp-FLOAT] : cxmul[mxtyp-FLOAT]);
		return buildtree(CALL, q, p);
		break;
	default:
		uerror("illegal operator %s", copst(op));
	}
	return comop(p, q);
}

/*
 * Fixup imaginary operations.
 * At least one operand is imaginary, none is complex.
 */
NODE *
imop(int op, NODE *l, NODE *r)
{
	NODE *p, *q;
	TWORD mxtyp;
	int li, ri;

	li = ri = 0;
	if (ISITY(l->n_type))
		li = 1, l->n_type = l->n_type - (FIMAG-FLOAT);
	if (ISITY(r->n_type))
		ri = 1, r->n_type = r->n_type - (FIMAG-FLOAT);

	mxtyp = maxtyp(l, r);
	switch (op) {
	case ASSIGN:
		/* if both are imag, store value, otherwise store 0.0 */
		if (!(li && ri)) {
			tfree(r);
			r = bcon(0);
		}
		p = buildtree(ASSIGN, l, r);
		p->n_type += (FIMAG-FLOAT);
		break;

	case PLUS:
		if (li && ri) {
			p = buildtree(PLUS, l, r);
			p->n_type += (FIMAG-FLOAT);
		} else {
			/* If one is imaginary and one is real, make complex */
			if (li)
				q = l, l = r, r = q; /* switch */
			q = cxstore(mxtyp);
			p = buildtree(ASSIGN,
			    structref(ccopy(q), DOT, real), l);
			p = comop(p, buildtree(ASSIGN,
			    structref(ccopy(q), DOT, imag), r));
			p = comop(p, q);
		}
		break;

	case MINUS:
		if (li && ri) {
			p = buildtree(MINUS, l, r);
			p->n_type += (FIMAG-FLOAT);
		} else if (li) {
			q = cxstore(mxtyp);
			p = buildtree(ASSIGN, structref(ccopy(q), DOT, real),
			    buildtree(UMINUS, r, NIL));
			p = comop(p, buildtree(ASSIGN,
			    structref(ccopy(q), DOT, imag), l));
			p = comop(p, q);
		} else /* if (ri) */ {
			q = cxstore(mxtyp);
			p = buildtree(ASSIGN,
			    structref(ccopy(q), DOT, real), l);
			p = comop(p, buildtree(ASSIGN,
			    structref(ccopy(q), DOT, imag),
			    buildtree(UMINUS, r, NIL)));
			p = comop(p, q);
		}
		break;

	case MUL:
		p = buildtree(MUL, l, r);
		if (li && ri)
			p = buildtree(UMINUS, p, NIL);
		if (li ^ ri)
			p->n_type += (FIMAG-FLOAT);
		break;

	case DIV:
		p = buildtree(DIV, l, r);
		if (ri && !li)
			p = buildtree(UMINUS, p, NIL);
		if (li ^ ri)
			p->n_type += (FIMAG-FLOAT);
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		if (li ^ ri) { /* always 0 */
			tfree(l);
			tfree(r);
			p = bcon(0);
		} else
			p = buildtree(op, l, r);
		break;

	default:
		cerror("imop");
		p = NULL;
	}
	return p;
}

NODE *
cxelem(int op, NODE *p)
{

	if (ANYCX(p)) {
		p = structref(p, DOT, op == XREAL ? real : imag);
	} else if (op == XIMAG) {
		/* XXX  sanitycheck? */
		tfree(p);
		p = bcon(0);
	}
	return p;
}

NODE *
cxconj(NODE *p)
{
	NODE *q, *r;

	/* XXX side effects? */
	q = cxstore(strmemb(p->n_ap)->stype);
	r = buildtree(ASSIGN, structref(ccopy(q), DOT, real),
	    structref(ccopy(p), DOT, real));
	r = comop(r, buildtree(ASSIGN, structref(ccopy(q), DOT, imag),
	    buildtree(UMINUS, structref(p, DOT, imag), NIL)));
	return comop(r, q);
}

/*
 * Prepare for return.
 * There may be implicit casts to other types.
 */
NODE *
cxret(NODE *p, NODE *q)
{
	if (ANYCX(q)) { /* Return complex type */
		p = mkcmplx(p, strmemb(q->n_ap)->stype);
	} else if (ISFTY(q->n_type) || ISITY(q->n_type)) { /* real or imag */
		p = structref(p, DOT, ISFTY(q->n_type) ? real : imag);
		if (p->n_type != q->n_type)
			p = cast(p, q->n_type, 0);
	} else 
		cerror("cxred failing type");
	return p;
}

/*
 * either p1 or p2 is complex, so fixup the remaining type accordingly.
 */
NODE *
cxcast(NODE *p1, NODE *p2)
{
	if (ANYCX(p1) && ANYCX(p2)) {
		if (p1->n_type != p2->n_type)
			p2 = mkcmplx(p2, p1->n_type);
	} else if (ANYCX(p1)) {
		p2 = mkcmplx(p2, p1->n_type);
	} else /* if (ANYCX(p2)) */ {
		p2 = cast(structref(p2, DOT, real), p1->n_type, 0);
	}
	nfree(p1);
	return p2;
}
#endif
