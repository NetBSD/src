/*      Id: gcc_compat.c,v 1.106 2014/06/07 07:04:09 plunky Exp      */	
/*      $NetBSD: gcc_compat.c,v 1.4.2.1 2014/08/10 07:10:07 tls Exp $     */
/*
 * Copyright (c) 2004 Anders Magnusson (ragge@ludd.luth.se).
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
 * Routines to support some of the gcc extensions to C.
 */
#ifdef GCC_COMPAT

#include "pass1.h"
#include "cgram.h"

#include <string.h>

static struct kw {
	char *name, *ptr;
	int rv;
} kw[] = {
/*
 * Do NOT change the order of these entries unless you know 
 * what you're doing!
 */
/* 0 */	{ "__asm", NULL, C_ASM },
/* 1 */	{ "__signed", NULL, 0 },
/* 2 */	{ "__inline", NULL, C_FUNSPEC },
/* 3 */	{ "__const", NULL, 0 },
/* 4 */	{ "__asm__", NULL, C_ASM },
/* 5 */	{ "__inline__", NULL, C_FUNSPEC },
/* 6 */	{ "__thread", NULL, 0 },
/* 7 */	{ "__FUNCTION__", NULL, 0 },
/* 8 */	{ "__volatile", NULL, 0 },
/* 9 */	{ "__volatile__", NULL, 0 },
/* 10 */{ "__restrict", NULL, -1 },
/* 11 */{ "__typeof__", NULL, C_TYPEOF },
/* 12 */{ "typeof", NULL, C_TYPEOF },
/* 13 */{ "__extension__", NULL, -1 },
/* 14 */{ "__signed__", NULL, 0 },
/* 15 */{ "__attribute__", NULL, 0 },
/* 16 */{ "__attribute", NULL, 0 },
/* 17 */{ "__real__", NULL, 0 },
/* 18 */{ "__imag__", NULL, 0 },
/* 19 */{ "__builtin_offsetof", NULL, PCC_OFFSETOF },
/* 20 */{ "__PRETTY_FUNCTION__", NULL, 0 },
/* 21 */{ "__alignof__", NULL, C_ALIGNOF },
/* 22 */{ "__typeof", NULL, C_TYPEOF },
/* 23 */{ "__alignof", NULL, C_ALIGNOF },
/* 24 */{ "__restrict__", NULL, -1 },
	{ NULL, NULL, 0 },
};

/* g77 stuff */
#if SZFLOAT == SZLONG
#define G77_INTEGER LONG
#define G77_UINTEGER ULONG
#elif SZFLOAT == SZINT
#define G77_INTEGER INT
#define G77_UINTEGER UNSIGNED
#else
#error fix g77 stuff
#endif
#if SZFLOAT*2 == SZLONG
#define G77_LONGINT LONG
#define G77_ULONGINT ULONG
#elif SZFLOAT*2 == SZLONGLONG
#define G77_LONGINT LONGLONG
#define G77_ULONGINT ULONGLONG
#else
#error fix g77 long stuff
#endif

static TWORD g77t[] = { G77_INTEGER, G77_UINTEGER, G77_LONGINT, G77_ULONGINT };
static char *g77n[] = { "__g77_integer", "__g77_uinteger",
	"__g77_longint", "__g77_ulongint" };

#ifdef TARGET_TIMODE
static char *loti, *hiti, *TISTR;
static struct symtab *tisp, *ucmpti2sp, *cmpti2sp, *subvti3sp,
	*addvti3sp, *mulvti3sp, *divti3sp, *udivti3sp, *modti3sp, *umodti3sp,
	*ashldi3sp, *ashrdi3sp, *lshrdi3sp, *floatuntixfsp;

static struct symtab *
addftn(char *n, TWORD t)
{
	NODE *p = block(TYPE, 0, 0, 0, 0, 0);
	struct symtab *sp;

	sp = lookup(addname(n), 0);
	p->n_type = INCREF(t) + (FTN-PTR);
	p->n_sp = sp;
	p->n_df = memset(permalloc(sizeof(union dimfun)), 0,
	    sizeof(union dimfun));
	defid(p, EXTERN);
	nfree(p);
	return sp;
}

static struct symtab *
addstr(char *n)
{
	NODE *p = block(NAME, NIL, NIL, FLOAT, 0, 0);
	struct symtab *sp;
	NODE *q;
	struct attr *ap;
	struct rstack *rp;
	extern struct rstack *rpole;

	p->n_type = ctype(ULONGLONG);
	rpole = rp = bstruct(NULL, STNAME, NULL);
	soumemb(p, loti, 0);
	soumemb(p, hiti, 0);
	q = dclstruct(rp);
	sp = q->n_sp = lookup(addname(n), 0);
	defid(q, TYPEDEF);
	ap = attr_new(GCC_ATYP_MODE, 3);
	ap->sarg(0) = addname("TI");
	ap->iarg(1) = 0;
	sp->sap = attr_add(sp->sap, ap);
	nfree(q);
	nfree(p);

	return sp;
}
#endif

void
gcc_init(void)
{
	struct kw *kwp;
	NODE *p;
	TWORD t;
	int i, d_debug;

	d_debug = ddebug;
	ddebug = 0;
	for (kwp = kw; kwp->name; kwp++)
		kwp->ptr = addname(kwp->name);

	for (i = 0; i < 4; i++) {
		struct symtab *sp;
		t = ctype(g77t[i]);
		p = block(NAME, NIL, NIL, t, NULL, 0);
		sp = lookup(addname(g77n[i]), 0);
		p->n_sp = sp;
		defid(p, TYPEDEF);
		nfree(p);
	}
	ddebug = d_debug;
#ifdef TARGET_TIMODE
	{
		struct attr *ap;

		loti = addname("__loti");
		hiti = addname("__hiti");
		TISTR = addname("TI");

		tisp = addstr("0ti");

		cmpti2sp = addftn("__cmpti2", INT);
		ucmpti2sp = addftn("__ucmpti2", INT);

		addvti3sp = addftn("__addvti3", STRTY);
		addvti3sp->sap = tisp->sap;
		subvti3sp = addftn("__subvti3", STRTY);
		subvti3sp->sap = tisp->sap;
		mulvti3sp = addftn("__mulvti3", STRTY);
		mulvti3sp->sap = tisp->sap;
		divti3sp = addftn("__divti3", STRTY);
		divti3sp->sap = tisp->sap;
		modti3sp = addftn("__modti3", STRTY);
		modti3sp->sap = tisp->sap;

		ap = attr_new(GCC_ATYP_MODE, 3);
		ap->sarg(0) = TISTR;
		ap->iarg(1) = 1;
		ap = attr_add(tisp->sap, ap);
		udivti3sp = addftn("__udivti3", STRTY);
		udivti3sp->sap = ap;
		umodti3sp = addftn("__umodti3", STRTY);
		umodti3sp->sap = ap;
		ashldi3sp = addftn("__ashldi3", ctype(LONGLONG));
		ashldi3sp->sap = ap;
		ashrdi3sp = addftn("__ashrdi3", ctype(LONGLONG));
		ashrdi3sp->sap = ap;
		lshrdi3sp = addftn("__lshrdi3", ctype(LONGLONG));
		lshrdi3sp->sap = ap;

		floatuntixfsp = addftn("__floatuntixf", LDOUBLE);
	}
#endif
}

#define	TS	"\n#pragma tls\n# %d\n"
#define	TLLEN	sizeof(TS)+10
/*
 * See if a string matches a gcc keyword.
 */
int
gcc_keyword(char *str, NODE **n)
{
	extern int inattr, parlvl, parbal;
	YYSTYPE *yyl = (YYSTYPE *)n; /* XXX should pass yylval */
	char tlbuf[TLLEN], *tw;
	struct kw *kwp;
	int i;

	/* XXX hack, should pass everything in expressions */
	if (str == kw[21].ptr)
		return kw[21].rv;

	if (inattr)
		return 0;

	for (i = 0, kwp = kw; kwp->name; kwp++, i++)
		if (str == kwp->ptr)
			break;
	if (kwp->name == NULL)
		return 0;
	if (kwp->rv)
		return kwp->rv;
	switch (i) {
	case 1:  /* __signed */
	case 14: /* __signed__ */
		*n = mkty((TWORD)SIGNED, 0, 0);
		return C_TYPE;
	case 3: /* __const */
		*n = block(QUALIFIER, NIL, NIL, CON, 0, 0);
		(*n)->n_qual = CON;
		return C_QUALIFIER;
	case 6: /* __thread */
		snprintf(tlbuf, TLLEN, TS, lineno);
		tw = &tlbuf[strlen(tlbuf)];
		while (tw > tlbuf)
			cunput(*--tw);
		return -1;
	case 7: /* __FUNCTION__ */
	case 20: /* __PRETTY_FUNCTION__ */
		if (cftnsp == NULL) {
			uerror("%s outside function", kwp->name);
			yylval.strp = "";
		} else
			yylval.strp = cftnsp->sname; /* XXX - not C99 */
		return C_STRING;
	case 8: /* __volatile */
	case 9: /* __volatile__ */
		*n = block(QUALIFIER, NIL, NIL, VOL, 0, 0);
		(*n)->n_qual = VOL;
		return C_QUALIFIER;
	case 15: /* __attribute__ */
	case 16: /* __attribute */
		inattr = 1;
		parlvl = parbal;
		return C_ATTRIBUTE;
	case 17: /* __real__ */
		yyl->intval = XREAL;
		return C_UNOP;
	case 18: /* __imag__ */
		yyl->intval = XIMAG;
		return C_UNOP;
	}
	cerror("gcc_keyword");
	return 0;
}

#ifndef TARGET_ATTR
#define	TARGET_ATTR(p, sue)		0
#endif
#ifndef	ALMAX
#define	ALMAX (ALLDOUBLE > ALLONGLONG ? ALLDOUBLE : ALLONGLONG)
#endif

/* allowed number of args */
#define	A_0ARG	0x01
#define	A_1ARG	0x02
#define	A_2ARG	0x04
#define	A_3ARG	0x08
/* arg # is a name */
#define	A1_NAME	0x10
#define	A2_NAME	0x20
#define	A3_NAME	0x40
#define	A_MANY	0x80
/* arg # is "string" */
#define	A1_STR	0x100
#define	A2_STR	0x200
#define	A3_STR	0x400

#ifdef __MSC__
#define	CS(x)
#else
#define CS(x) [x] =
#endif

struct atax {
	int typ;
	char *name;
} atax[GCC_ATYP_MAX] = {
	CS(ATTR_NONE)		{ 0, NULL },
	CS(ATTR_COMPLEX)	{ 0, NULL },
	CS(xxxATTR_BASETYP)	{ 0, NULL },
	CS(ATTR_QUALTYP)	{ 0, NULL },
	CS(ATTR_STRUCT)		{ 0, NULL },
	CS(ATTR_ALIGNED)	{ A_0ARG|A_1ARG, "aligned" },
	CS(GCC_ATYP_PACKED)	{ A_0ARG|A_1ARG, "packed" },
	CS(GCC_ATYP_SECTION)	{ A_1ARG|A1_STR, "section" },
	CS(GCC_ATYP_TRANSP_UNION) { A_0ARG, "transparent_union" },
	CS(GCC_ATYP_UNUSED)	{ A_0ARG, "unused" },
	CS(GCC_ATYP_DEPRECATED)	{ A_0ARG, "deprecated" },
	CS(GCC_ATYP_MAYALIAS)	{ A_0ARG, "may_alias" },
	CS(GCC_ATYP_MODE)	{ A_1ARG|A1_NAME, "mode" },
	CS(GCC_ATYP_NORETURN)	{ A_0ARG, "noreturn" },
	CS(GCC_ATYP_FORMAT)	{ A_3ARG|A1_NAME, "format" },
	CS(GCC_ATYP_NONNULL)	{ A_MANY, "nonnull" },
	CS(GCC_ATYP_SENTINEL)	{ A_0ARG|A_1ARG, "sentinel" },
	CS(GCC_ATYP_WEAK)	{ A_0ARG, "weak" },
	CS(GCC_ATYP_FORMATARG)	{ A_1ARG, "format_arg" },
	CS(GCC_ATYP_GNU_INLINE)	{ A_0ARG, "gnu_inline" },
	CS(GCC_ATYP_MALLOC)	{ A_0ARG, "malloc" },
	CS(GCC_ATYP_NOTHROW)	{ A_0ARG, "nothrow" },
	CS(GCC_ATYP_CONST)	{ A_0ARG, "const" },
	CS(GCC_ATYP_PURE)	{ A_0ARG, "pure" },
	CS(GCC_ATYP_CONSTRUCTOR) { A_0ARG, "constructor" },
	CS(GCC_ATYP_DESTRUCTOR)	{ A_0ARG, "destructor" },
	CS(GCC_ATYP_VISIBILITY)	{ A_1ARG|A1_STR, "visibility" },
	CS(GCC_ATYP_STDCALL)	{ A_0ARG, "stdcall" },
	CS(GCC_ATYP_CDECL)	{ A_0ARG, "cdecl" },
	CS(GCC_ATYP_WARN_UNUSED_RESULT) { A_0ARG, "warn_unused_result" },
	CS(GCC_ATYP_USED)	{ A_0ARG, "used" },
	CS(GCC_ATYP_NO_INSTR_FUN) { A_0ARG, "no_instrument_function" },
	CS(GCC_ATYP_NOINLINE)	{ A_0ARG, "noinline" },
	CS(GCC_ATYP_ALIAS)	{ A_1ARG|A1_STR, "alias" },
	CS(GCC_ATYP_WEAKREF)	{ A_0ARG|A_1ARG|A1_STR, "weakref" },
	CS(GCC_ATYP_ALLOCSZ)	{ A_1ARG|A_2ARG, "alloc_size" },
	CS(GCC_ATYP_ALW_INL)	{ A_0ARG, "always_inline" },
	CS(GCC_ATYP_TLSMODEL)	{ A_1ARG|A1_STR, "tls_model" },
	CS(GCC_ATYP_ALIASWEAK)	{ A_1ARG|A1_STR, "aliasweak" },
	CS(GCC_ATYP_RETURNS_TWICE) { A_0ARG, "returns_twice" },
	CS(GCC_ATYP_WARNING)	{ A_1ARG|A1_STR, "warning" },
	CS(GCC_ATYP_NOCLONE)	{ A_0ARG, "noclone" },
	CS(GCC_ATYP_REGPARM)	{ A_1ARG, "regparm" },

	CS(GCC_ATYP_BOUNDED)	{ A_3ARG|A_MANY|A1_NAME, "bounded" },
};

#if SZPOINT(CHAR) == SZLONGLONG
#define	GPT	LONGLONG
#else
#define	GPT	INT
#endif

struct atax mods[] = {
	{ 0, NULL },
	{ INT, "SI" },
	{ INT, "word" },
	{ GPT, "pointer" },
	{ CHAR, "byte" },
	{ CHAR, "QI" },
	{ SHORT, "HI" },
	{ LONGLONG, "DI" },
	{ FLOAT, "SF" },
	{ DOUBLE, "DF" },
	{ LDOUBLE, "XF" },
	{ FCOMPLEX, "SC" },
	{ COMPLEX, "DC" },
	{ LCOMPLEX, "XC" },
	{ INT, "libgcc_cmp_return" },
	{ INT, "libgcc_shift_count" },
	{ LONG, "unwind_word" },
#ifdef TARGET_TIMODE
	{ 800, "TI" },
#endif
#ifdef TARGET_MODS
	TARGET_MODS
#endif
};
#define	ATSZ	(sizeof(mods)/sizeof(mods[0]))

static int
amatch(char *s, struct atax *at, int mx)
{
	int i, len;

	if (s[0] == '_' && s[1] == '_')
		s += 2;
	len = strlen(s);
	if (len > 2 && s[len-1] == '_' && s[len-2] == '_')
		len -= 2;
	for (i = 0; i < mx; i++) {
		char *t = at[i].name;
		if (t != NULL && strncmp(s, t, len) == 0 && t[len] == 0)
			return i;
	}
	return 0;
}

static void
setaarg(int str, union aarg *aa, NODE *p)
{
	if (str) {
		if (((str & (A1_STR|A2_STR|A3_STR)) && p->n_op != STRING) ||
		    ((str & (A1_NAME|A2_NAME|A3_NAME)) && p->n_op != NAME))
			uerror("bad arg to attribute");
		if (p->n_op == STRING) {
			aa->sarg = newstring(p->n_name, strlen(p->n_name));
		} else
			aa->sarg = (char *)p->n_sp;
		nfree(p);
	} else
		aa->iarg = (int)icons(eve(p));
}

/*
 * Parse attributes from an argument list.
 */
static struct attr *
gcc_attribs(NODE *p)
{
	NODE *q, *r;
	struct attr *ap;
	char *name = NULL, *c;
	int cw, attr, narg;

	if (p->n_op == NAME) {
		name = (char *)p->n_sp;
	} else if (p->n_op == CALL || p->n_op == UCALL) {
		name = (char *)p->n_left->n_sp;
	} else if (p->n_op == ICON && p->n_type == STRTY) {
		return NULL;
	} else
		cerror("bad variable attribute");

	if ((attr = amatch(name, atax, GCC_ATYP_MAX)) == 0) {
		warner(Wattributes, name);
		ap = NULL;
		goto out;
	}
	narg = 0;
	if (p->n_op == CALL)
		for (narg = 1, q = p->n_right; q->n_op == CM; q = q->n_left)
			narg++;

	cw = atax[attr].typ;
	if (!(cw & A_MANY) && ((narg > 3) || ((cw & (1 << narg)) == 0))) {
		uerror("wrong attribute arg count");
		return NULL;
	}
	ap = attr_new(attr, 3); /* XXX should be narg */
	q = p->n_right;

	switch (narg) {
	default:
		/* XXX */
		while (narg-- > 3) {
			r = q;
			q = q->n_left;
			tfree(r->n_right);
			nfree(r);
		}
		/* FALLTHROUGH */
	case 3:
		setaarg(cw & (A3_NAME|A3_STR), &ap->aa[2], q->n_right);
		r = q;
		q = q->n_left;
		nfree(r);
		/* FALLTHROUGH */
	case 2:
		setaarg(cw & (A2_NAME|A2_STR), &ap->aa[1], q->n_right);
		r = q;
		q = q->n_left;
		nfree(r);
		/* FALLTHROUGH */
	case 1:
		setaarg(cw & (A1_NAME|A1_STR), &ap->aa[0], q);
		p->n_op = UCALL;
		/* FALLTHROUGH */
	case 0:
		break;
	}

	/* some attributes must be massaged special */
	switch (attr) {
	case ATTR_ALIGNED:
		if (narg == 0)
			ap->aa[0].iarg = ALMAX;
		else
			ap->aa[0].iarg *= SZCHAR;
		break;
	case GCC_ATYP_PACKED:
		if (narg == 0)
			ap->aa[0].iarg = 1; /* bitwise align */
		else
			ap->aa[0].iarg *= SZCHAR;
		break;

	case GCC_ATYP_VISIBILITY:
		c = ap->aa[0].sarg;
		if (strcmp(c, "default") && strcmp(c, "hidden") &&
		    strcmp(c, "internal") && strcmp(c, "protected"))
			werror("unknown visibility %s", c);
		break;

	case GCC_ATYP_TLSMODEL:
		c = ap->aa[0].sarg;
		if (strcmp(c, "global-dynamic") && strcmp(c, "local-dynamic") &&
		    strcmp(c, "initial-exec") && strcmp(c, "local-exec"))
			werror("unknown tls model %s", c);
		break;

	default:
		break;
	}
out:
	return ap;
}

/*
 * Extract attributes from a node tree and return attribute entries 
 * based on its contents.
 */
struct attr *
gcc_attr_parse(NODE *p)
{
	struct attr *b, *c;

	if (p == NIL)
		return NULL;

	if (p->n_op != CM) {
		b = gcc_attribs(p);
		tfree(p);
	} else {
		b = gcc_attr_parse(p->n_left);
		c = gcc_attr_parse(p->n_right);
		nfree(p);
		b = b ? attr_add(b, c) : c;
	}
	return b;
}

/*
 * Fixup struct/unions depending on attributes.
 */
void
gcc_tcattrfix(NODE *p)
{
	struct symtab *sp;
	struct attr *ap;
	int sz, coff, csz, al, oal, mxal;

	if ((ap = attr_find(p->n_ap, GCC_ATYP_PACKED)) == NULL)
		return; /* nothing to fix */

	al = ap->iarg(0);
	mxal = 0;

	/* Must repack struct */
	coff = csz = 0;
	for (sp = strmemb(ap); sp; sp = sp->snext) {
		oal = talign(sp->stype, sp->sap);
		if (oal > al)
			oal = al;
		if (mxal < oal)
			mxal = oal;
		if (sp->sclass & FIELD)
			sz = sp->sclass&FLDSIZ;
		else
			sz = (int)tsize(sp->stype, sp->sdf, sp->sap);
		sp->soffset = upoff(sz, oal, &coff);
		if (coff > csz)
			csz = coff;
		if (p->n_type == UNIONTY)
			coff = 0;
	}
	if (mxal < ALCHAR)
		mxal = ALCHAR; /* for bitfields */
	SETOFF(csz, mxal); /* Roundup to whatever */

	ap = attr_find(p->n_ap, ATTR_STRUCT);
	ap->amsize = csz;
	ap = attr_find(p->n_ap, ATTR_ALIGNED);
	ap->iarg(0) = mxal;

}

/*
 * gcc-specific pragmas.
 */
int
pragmas_gcc(char *t)
{
	char u;
	extern char *pragstore;

	if (strcmp((t = pragtok(NULL)), "diagnostic") == 0) {
		int warn, err;

		if (strcmp((t = pragtok(NULL)), "ignored") == 0)
			warn = 0, err = 0;
		else if (strcmp(t, "warning") == 0)
			warn = 1, err = 0;
		else if (strcmp(t, "error") == 0)
			warn = 1, err = 1;
		else
			return 1;

		if (eat('\"') || eat('-'))
			return 1;

		for (t = pragstore; *t && *t != '\"'; t++)
			;

		u = *t;
		*t = 0;
		Wset(pragstore + 1, warn, err);
		*t = u;
	} else if (strcmp(t, "poison") == 0) {
		/* currently ignore */;
	} else if (strcmp(t, "visibility") == 0) {
		/* currently ignore */;
	} else if (strcmp(t, "system_header") == 0) {
		/* currently ignore */;
	} else
		werror("gcc pragma unsupported");
	return 0;
}

/*
 * Fixup types when modes given in defid().
 */
void
gcc_modefix(NODE *p)
{
	struct attr *ap;
#ifdef TARGET_TIMODE
	struct attr *a2;
#endif
	struct symtab *sp;
	char *s;
	int i, u;

	if ((ap = attr_find(p->n_ap, GCC_ATYP_MODE)) == NULL)
		return;

	u = ISUNSIGNED(BTYPE(p->n_type));
	if ((i = amatch(ap->aa[0].sarg, mods, ATSZ)) == 0) {
		werror("unknown mode arg %s", ap->aa[0].sarg);
		return;
	}
	i = mods[i].typ;
	switch (i) {
#ifdef TARGET_TIMODE
	case 800:
		if (BTYPE(p->n_type) == STRTY)
			break;
		MODTYPE(p->n_type, tisp->stype);
		p->n_df = tisp->sdf;
		p->n_ap = tisp->sap;
		if (ap->iarg(1) == u)
			break;
		/* must add a new mode struct to avoid overwriting */
		a2 = attr_new(GCC_ATYP_MODE, 3);
		a2->sarg(0) = ap->sarg(0);
		a2->iarg(1) = u;
		p->n_ap = attr_add(p->n_ap, a2);
		break;
#endif
	case 1 ... MAXTYPES:
		MODTYPE(p->n_type, ctype(i));
		if (u)
			p->n_type = ENUNSIGN(p->n_type);
		break;

	case FCOMPLEX:
	case COMPLEX:
	case LCOMPLEX:
		/* Destination should have been converted to a struct already */
		if (BTYPE(p->n_type) != STRTY)
			uerror("gcc_modefix: complex not STRTY");
		i -= (FCOMPLEX-FLOAT);
		ap = strattr(p->n_ap);
		sp = ap->amlist;
		if (sp->stype == (unsigned)i)
			return; /* Already correct type */
		/* we must change to another struct */
		s = i == FLOAT ? "0f" :
		    i == DOUBLE ? "0d" :
		    i == LDOUBLE ? "0l" : 0;
		sp = lookup(addname(s), 0);
		for (ap = sp->sap; ap != NULL; ap = ap->next)
			p->n_ap = attr_add(p->n_ap, attr_dup(ap, 3));
		break;

	default:
		cerror("gcc_modefix");
	}
}

#ifdef TARGET_TIMODE

/*
 * Return ap if this node is a TI node, else NULL.
 */
struct attr *
isti(NODE *p)
{
	struct attr *ap;

	if (p->n_type != STRTY)
		return NULL;
	if ((ap = attr_find(p->n_ap, GCC_ATYP_MODE)) == NULL)
		return NULL;
	if (strcmp(ap->sarg(0), TISTR))
		return NULL;
	return ap;
}

static char *
tistack(void)
{
	struct symtab *sp, *sp2;
	char buf[12];
	NODE *q;
	char *n;

	/* allocate space on stack */
	snprintf(buf, 12, "%d", getlab());
	n = addname(buf);
	sp = lookup(n, 0);
	sp2 = tisp;
	q = block(TYPE, NIL, NIL, sp2->stype, sp2->sdf, sp2->sap);
	q->n_sp = sp;
	nidcl2(q, AUTO, 0);
	nfree(q);
	return n;
}

#define	biop(x,y,z) block(x, y, z, INT, 0, 0)
/*
 * Create a ti node from something not a ti node.
 * This usually means:  allocate space on stack, store val, give stack address.
 */
static NODE *
ticast(NODE *p, int u)
{
	CONSZ val;
	NODE *q;
	char *n;
	int u2;

	n = tistack();

	/* store val */
	switch (p->n_op) {
	case ICON:
		val = 0;
		if (u == 0 && p->n_lval < 0)
			val = -1;
		q = eve(biop(DOT, bdty(NAME, n), bdty(NAME, loti)));
		q = buildtree(ASSIGN, q, p);
		p = biop(DOT, bdty(NAME, n), bdty(NAME, hiti));
		p = eve(biop(ASSIGN, p, bcon(val)));
		q = buildtree(COMOP, q, p);
		p = buildtree(COMOP, q, eve(bdty(NAME, n)));
		break;

	default:
		u2 = ISUNSIGNED(p->n_type);
		q = eve(biop(DOT, bdty(NAME, n), bdty(NAME, loti)));
		q = buildtree(ASSIGN, q, p);
		p = biop(DOT, bdty(NAME, n), bdty(NAME, hiti));
		if (u2) {
			p = eve(biop(ASSIGN, p, bcon(0)));
		} else {
			q = buildtree(ASSIGN, eve(ccopy(p)), q);
			p = buildtree(RSEQ, eve(p), bcon(SZLONG-1));
		}
		q = buildtree(COMOP, q, p);
		p = buildtree(COMOP, q, eve(bdty(NAME, n)));
		break;
	}
	return p;
}

/*
 * Check if we may have to do a cast to/from TI.
 */
NODE *
gcc_eval_ticast(int op, NODE *p1, NODE *p2)
{
	struct attr *a1, *a2;
	int t;

	if ((a1 = isti(p1)) == NULL && (a2 = isti(p2)) == NULL)
		return NIL;

	if (op == RETURN)
		p1 = ccopy(p1);
	if (a1 == NULL) {
		if (a2 == NULL)
			cerror("gcc_eval_ticast error");
		switch (p1->n_type) {
		case LDOUBLE:
			p2 = doacall(floatuntixfsp,
			    nametree(floatuntixfsp), p2);
			tfree(p1);
			break;
		case ULONG:
		case LONG:
			p2 = cast(structref(p2, DOT, loti), p1->n_type, 0);
			tfree(p1);
			break;
		case VOID:
			return NIL;
		default:
			uerror("gcc_eval_ticast: %d", p1->n_type);
		}
		return p2;
	}
	/* p2 can be anything, but we must cast it to p1 */
	t = a1->iarg(1);

	if (p2->n_type == STRTY &&
	    (a2 = attr_find(p2->n_ap, GCC_ATYP_MODE)) &&
	    strcmp(a2->sarg(0), TISTR) == 0) {
		/* Already TI, just add extra mode bits */
		a2 = attr_new(GCC_ATYP_MODE, 3);
		a2->sarg(0) = TISTR;
		a2->iarg(1) = t;
		p2->n_ap = attr_add(p2->n_ap, a2);
	} else  {
		p2 = ticast(p2, t);
	}
	tfree(p1);
	return p2;
}

/*
 * Apply a unary op on a TI value.
 */
NODE *
gcc_eval_tiuni(int op, NODE *p1)
{
	struct attr *a1;
	NODE *p;

	if ((a1 = isti(p1)) == NULL)
		return NULL;

	switch (op) {
	case UMINUS:
		p = ticast(bcon(0), 0);
		p = buildtree(CM, p, p1);
		p = doacall(subvti3sp, nametree(subvti3sp), p);
		break;

	case UMUL:
		p = NULL;
	default:
		uerror("unsupported unary TI mode op %d", op);
		p = NULL;
	}
	return p;
}

/*
 * Evaluate AND/OR/ER.  p1 and p2 are pointers to ti struct.
 */
static NODE *
gcc_andorer(int op, NODE *p1, NODE *p2)
{
	char *n = tistack();
	NODE *p, *t1, *t2, *p3;

	t1 = tempnode(0, p1->n_type, p1->n_df, p1->n_ap);
	t2 = tempnode(0, p2->n_type, p2->n_df, p2->n_ap);

	p1 = buildtree(ASSIGN, ccopy(t1), p1);
	p2 = buildtree(ASSIGN, ccopy(t2), p2);
	p = buildtree(COMOP, p1, p2);

	p3 = buildtree(ADDROF, eve(bdty(NAME, n)), NIL);
	p1 = buildtree(ASSIGN, structref(ccopy(p3), STREF, hiti),
	    buildtree(op, structref(ccopy(t1), STREF, hiti),
	    structref(ccopy(t2), STREF, hiti)));
	p = buildtree(COMOP, p, p1);
	p1 = buildtree(ASSIGN, structref(ccopy(p3), STREF, loti),
	    buildtree(op, structref(t1, STREF, loti),
	    structref(t2, STREF, loti)));
	p = buildtree(COMOP, p, p1);
	p = buildtree(COMOP, p, buildtree(UMUL, p3, NIL));
	return p;
}

/*
 * Ensure that a 128-bit assign succeeds.
 * If left is not TI, make right not TI,
 * else if left _is_ TI, make right TI,
 * else do nothing.
 */
static NODE *
timodeassign(NODE *p1, NODE *p2)
{
	struct attr *a1, *a2;

	a1 = isti(p1);
	a2 = isti(p2);
	if (a1 && a2 == NULL) {
		p2 = ticast(p2, a1->iarg(1));
	} else if (a1 == NULL && a2) {
		if (ISFTY(p1->n_type))
			cerror("cannot TI float convert");
		p2 = structref(p2, DOT, loti);
	}
	return buildtree(ASSIGN, p1, p2);
}

/*
 * Evaluate 128-bit operands.
 */
NODE *
gcc_eval_timode(int op, NODE *p1, NODE *p2)
{
	struct attr *a1, *a2;
	struct symtab *sp;
	NODE *p;
	int isu = 0, gotti, isaop;

	if (op == CM)
		return buildtree(op, p1, p2);

	a1 = isti(p1);
	a2 = isti(p2);

	if (a1 == NULL && a2 == NULL)
		return NULL;

	if (op == ASSIGN)
		return timodeassign(p1, p2);

	gotti = (a1 != NULL);
	gotti += (a2 != NULL);

	if (gotti == 0)
		return NULL;

	if (a1 != NULL)
		isu = a1->iarg(1);
	if (a2 != NULL && !isu)
		isu = a2->iarg(1);

	if (a1 == NULL) {
		p1 = ticast(p1, isu);
		a1 = attr_find(p1->n_ap, GCC_ATYP_MODE);
	}
	if (a2 == NULL && (cdope(op) & SHFFLG) == 0) {
		p2 = ticast(p2, isu);
		a2 = attr_find(p2->n_ap, GCC_ATYP_MODE);
	}

	switch (op) {
	case GT:
	case GE:
	case LT:
	case LE:
	case EQ:
	case NE:
		/* change to call */
		sp = isu ? ucmpti2sp : cmpti2sp;
		p = doacall(sp, nametree(sp), buildtree(CM, p1, p2));
		p = buildtree(op, p, bcon(1));
		break;

	case AND:
	case ER:
	case OR:
		if (!ISPTR(p1->n_type))
			p1 = buildtree(ADDROF, p1, NIL);
		if (!ISPTR(p2->n_type))
			p2 = buildtree(ADDROF, p2, NIL);
		p = gcc_andorer(op, p1, p2);
		break;

	case LSEQ:
	case RSEQ:
	case LS:
	case RS:
		sp = op == LS || op == LSEQ ? ashldi3sp :
		    isu ? lshrdi3sp : ashrdi3sp;
		p2 = cast(p2, INT, 0);
		/* XXX p1 ccopy may have side effects */
		p = doacall(sp, nametree(sp), buildtree(CM, ccopy(p1), p2));
		if (op == LSEQ || op == RSEQ) {
			p = buildtree(ASSIGN, p1, p);
		} else
			tfree(p1);
		break;

	case PLUSEQ:
	case MINUSEQ:
	case MULEQ:
	case DIVEQ:
	case MODEQ:
	case PLUS:
	case MINUS:
	case MUL:
	case DIV:
	case MOD:
		isaop = (cdope(op)&ASGOPFLG);
		if (isaop)
			op = UNASG op;
		sp = op == PLUS ? addvti3sp :
		    op == MINUS ? subvti3sp :
		    op == MUL ? mulvti3sp :
		    op == DIV ? (isu ? udivti3sp : divti3sp) :
		    op == MOD ? (isu ? umodti3sp : modti3sp) : 0;
		/* XXX p1 ccopy may have side effects */
		p = doacall(sp, nametree(sp), buildtree(CM, ccopy(p1), p2));
		if (isaop)
			p = buildtree(ASSIGN, p1, p);
		else
			tfree(p1);
		break;

	default:
		uerror("unsupported TImode op %d", op);
		p = bcon(0);
	}
	return p;
}
#endif

#ifdef PCC_DEBUG
void
dump_attr(struct attr *ap)
{
	printf("attributes; ");
	for (; ap; ap = ap->next) {
		if (ap->atype >= GCC_ATYP_MAX) {
			printf("bad type %d, ", ap->atype);
		} else if (atax[ap->atype].name == 0) {
			char *c = ap->atype == ATTR_COMPLEX ? "complex" :
			    ap->atype == ATTR_STRUCT ? "struct" : "badtype";
			printf("%s, ", c);
		} else {
			printf("%s: ", atax[ap->atype].name);
			printf("%d %d %d, ", ap->iarg(0),
			    ap->iarg(1), ap->iarg(2));
		}
	}
	printf("\n");
}
#endif
#endif
