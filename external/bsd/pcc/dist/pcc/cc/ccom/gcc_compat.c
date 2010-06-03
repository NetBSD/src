/*      Id: gcc_compat.c,v 1.52 2010/05/15 15:58:33 ragge Exp      */	
/*      $NetBSD: gcc_compat.c,v 1.1.1.3 2010/06/03 18:57:39 plunky Exp $     */
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

void
gcc_init()
{
	struct kw *kwp;
	NODE *p;
	TWORD t;
	int i;

	for (kwp = kw; kwp->name; kwp++)
		kwp->ptr = addname(kwp->name);

	for (i = 0; i < 4; i++) {
		struct symtab *sp;
		t = ctype(g77t[i]);
		p = block(NAME, NIL, NIL, t, NULL, MKSUE(t));
		sp = lookup(addname(g77n[i]), 0);
		p->n_sp = sp;
		defid(p, TYPEDEF);
		nfree(p);
	}

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
		*n = mkty((TWORD)SIGNED, 0, MKSUE(SIGNED));
		return C_TYPE;
	case 3: /* __const */
		*n = block(QUALIFIER, NIL, NIL, CON, 0, 0);
		return C_QUALIFIER;
	case 6: /* __thread */
		snprintf(tlbuf, TLLEN, TS, lineno);
		tw = &tlbuf[strlen(tlbuf)];
		while (tw > tlbuf)
			cunput(*--tw);
		return -1;
	case 7: /* __FUNCTION__ */
		if (cftnsp == NULL) {
			uerror("__FUNCTION__ outside function");
			yylval.strp = "";
		} else
			yylval.strp = cftnsp->sname; /* XXX - not C99 */
		return C_STRING;
	case 8: /* __volatile */
	case 9: /* __volatile__ */
		*n = block(QUALIFIER, NIL, NIL, VOL, 0, 0);
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

struct atax {
	int typ;
	char *name;
} atax[GCC_ATYP_MAX] = {
#ifndef __MSC__
	[GCC_ATYP_ALIGNED] =	{ A_0ARG|A_1ARG, "aligned" },
	[GCC_ATYP_PACKED] =	{ A_0ARG|A_1ARG, "packed" },
	[GCC_ATYP_SECTION] = 	{ A_1ARG|A1_STR, "section" },
	[GCC_ATYP_UNUSED] =	{ A_0ARG, "unused" },
	[GCC_ATYP_DEPRECATED] =	{ A_0ARG, "deprecated" },
	[GCC_ATYP_NORETURN] =	{ A_0ARG, "noreturn" },
	[GCC_ATYP_FORMAT] =	{ A_3ARG|A1_NAME, "format" },
	[GCC_ATYP_BOUNDED] =	{ A_3ARG|A_MANY|A1_NAME, "bounded" },
	[GCC_ATYP_NONNULL] =	{ A_MANY, "nonnull" },
	[GCC_ATYP_SENTINEL] =	{ A_0ARG|A_1ARG, "sentinel" },
	[GCC_ATYP_WEAK] =	{ A_0ARG, "weak" },
	[GCC_ATYP_FORMATARG] =	{ A_1ARG, "format_arg" },
	[GCC_ATYP_GNU_INLINE] =	{ A_0ARG, "gnu_inline" },
	[GCC_ATYP_MALLOC] =	{ A_0ARG, "malloc" },
	[GCC_ATYP_NOTHROW] =	{ A_0ARG, "nothrow" },
	[GCC_ATYP_MODE] =	{ A_1ARG|A1_NAME, "mode" },
	[GCC_ATYP_CONST] =	{ A_0ARG, "const" },
	[GCC_ATYP_PURE] =	{ A_0ARG, "pure" },
	[GCC_ATYP_CONSTRUCTOR] ={ A_0ARG, "constructor" },
	[GCC_ATYP_DESTRUCTOR] =	{ A_0ARG, "destructor" },
	[GCC_ATYP_VISIBILITY] =	{ A_1ARG|A1_STR, "visibility" },
	[GCC_ATYP_STDCALL] =	{ A_0ARG, "stdcall" },
	[GCC_ATYP_CDECL] =	{ A_0ARG, "cdecl" },
	[GCC_ATYP_WARN_UNUSED_RESULT] = { A_0ARG, "warn_unused_result" },
	[GCC_ATYP_USED] =	{ A_0ARG, "used" },
#else
	{ 0, NULL },
	{ A_0ARG|A_1ARG, "aligned" },
	{ A_0ARG, "packed" },
	{ A_1ARG|A1_STR, "section" },
	{ 0, NULL }, 	/* GCC_ATYP_TRANSP_UNION */
	{ A_0ARG, "unused" },
	{ A_0ARG, "deprecated" },
	{ 0, NULL }, 	/* GCC_ATYP_MAYALIAS */
	{ A_1ARG|A1_NAME, "mode" },
	{ A_0ARG, "noreturn" },
	{ A_3ARG|A1_STR, "format" },
	{ A_MANY, "nonnull" },
	{ A_0ARG|A_1ARG, "sentinel" },
	{ A_0ARG, "weak" },
	{ A_1ARG, "format_arg" },
	{ A_0ARG, "gnu_inline" },
	{ A_0ARG, "malloc" },
	{ A_0ARG, "nothrow" },
	{ A_0ARG, "const" },
	{ A_0ARG, "pure" },
	{ A_0ARG, "constructor" },
	{ A_0ARG, "destructor" },
	{ A_1ARG|A1_STR, "visibility" },
	{ A_0ARG, "stdcall" },
	{ A_0ARG, "cdecl" },
	{ A_0ARG, "warn_unused_result" },
	{ A_0ARG, "used" },
	{ A_3ARG|A_MANY|A1_STR, "bounded" },
	{ 0, NULL },	/* ATTR_COMPLEX */
#endif
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
setaarg(int str, union gcc_aarg *aa, NODE *p)
{
	if (str) {
		if (((str & (A1_STR|A2_STR|A3_STR)) && p->n_op != STRING) ||
		    ((str & (A1_NAME|A2_NAME|A3_NAME)) && p->n_op != NAME))
			uerror("bad arg to attribute");
		aa->sarg = p->n_op == STRING ? p->n_name : (char *)p->n_sp;
		nfree(p);
	} else
		aa->iarg = (int)icons(eve(p));
}

/*
 * Parse attributes from an argument list.
 */
static void
gcc_attribs(NODE *p, void *arg)
{
	NODE *q, *r;
	gcc_ap_t *gap = arg;
	char *name = NULL, *c;
	int num, cw, attr, narg, i;

	if (p->n_op == NAME) {
		name = (char *)p->n_sp;
	} else if (p->n_op == CALL || p->n_op == UCALL) {
		name = (char *)p->n_left->n_sp;
	} else
		cerror("bad variable attribute");

	if ((attr = amatch(name, atax, GCC_ATYP_MAX)) == 0) {
		werror("unsupported attribute '%s'", name);
		goto out;
	}
	narg = 0;
	if (p->n_op == CALL)
		for (narg = 1, q = p->n_right; q->n_op == CM; q = q->n_left)
			narg++;

	cw = atax[attr].typ;
	if (!(cw & A_MANY) && ((narg > 3) || ((cw & (1 << narg)) == 0))) {
		uerror("wrong attribute arg count");
		return;
	}
	num = gap->num;
	gap->ga[num].atype = attr;
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
		setaarg(cw & (A3_NAME|A3_STR), &gap->ga[num].a3, q->n_right);
		r = q;
		q = q->n_left;
		nfree(r);
		/* FALLTHROUGH */
	case 2:
		setaarg(cw & (A2_NAME|A2_STR), &gap->ga[num].a2, q->n_right);
		r = q;
		q = q->n_left;
		nfree(r);
		/* FALLTHROUGH */
	case 1:
		setaarg(cw & (A1_NAME|A1_STR), &gap->ga[num].a1, q);
		p->n_op = UCALL;
		/* FALLTHROUGH */
	case 0:
		break;
	}

	/* some attributes must be massaged special */
	switch (attr) {
	case GCC_ATYP_ALIGNED:
		if (narg == 0)
			gap->ga[num].a1.iarg = ALMAX;
		else
			gap->ga[num].a1.iarg *= SZCHAR;
		break;
	case GCC_ATYP_PACKED:
		if (narg == 0)
			gap->ga[num].a1.iarg = 1; /* bitwise align */
		else
			gap->ga[num].a1.iarg *= SZCHAR;
		break;

	case GCC_ATYP_MODE:
		if ((i = amatch(gap->ga[num].a1.sarg, mods, ATSZ)) == 0)
			werror("unknown mode arg %s", gap->ga[num].a1.sarg);
		gap->ga[num].a1.iarg = mods[i].typ;
		break;

	case GCC_ATYP_VISIBILITY:
		c = gap->ga[num].a1.sarg;
		if (strcmp(c, "default") && strcmp(c, "hidden") &&
		    strcmp(c, "internal") && strcmp(c, "protected"))
			werror("unknown visibility %s", c);
		break;

	default:
		break;
	}
out:
	gap->num++;
}

/*
 * Extract type attributes from a node tree and create a gcc_attr_pack
 * struct based on its contents.
 */
gcc_ap_t *
gcc_attr_parse(NODE *p)
{
	gcc_ap_t *gap;
	NODE *q, *r;
	int i, sz;

	/* count number of elems and build tower to the left */
	for (q = p, i = 1; q->n_op == CM; q = q->n_left, i++)
		if (q->n_right->n_op == CM)
			r = q->n_right, q->n_right = q->n_left, q->n_left = r;

	/* get memory for struct */
	sz = sizeof(struct gcc_attr_pack) + sizeof(struct gcc_attrib) * i;
	if (blevel == 0)
		gap = memset(permalloc(sz), 0, sz);
	else
		gap = tmpcalloc(sz);

	flist(p, gcc_attribs, gap);
	if (gap->num != i)
		cerror("attribute sync error");

	tfree(p);
	return gap;
}

/*
 * Fixup struct/unions depending on attributes.
 */
void
gcc_tcattrfix(NODE *p, NODE *q)
{
	struct symtab *sp;
	struct suedef *sue;
	gcc_ap_t *gap;
	int align = 0;
	int i, sz, coff, csz;

	gap = gcc_attr_parse(q);
	sue = p->n_sue;
	if (sue->suega) {
		if (p->n_sp == NULL)
			cerror("gcc_tcattrfix");
	}

	/* must know about align first */
	for (i = 0; i < gap->num; i++)
		if (gap->ga[i].atype == GCC_ATYP_ALIGNED)
			align = gap->ga[i].a1.iarg;

	/* Check following attributes */
	for (i = 0; i < gap->num; i++) {
		switch (gap->ga[i].atype) {
		case GCC_ATYP_PACKED:
			/* Must repack struct */
			/* XXX - aligned types inside? */
			coff = csz = 0;
			for (sp = sue->suem; sp; sp = sp->snext) {
				if (sp->sclass & FIELD)
					sz = sp->sclass&FLDSIZ;
				else
					sz = (int)tsize(sp->stype, sp->sdf, sp->ssue);
				SETOFF(sz, gap->ga[i].a1.iarg);
				sp->soffset = coff;
				coff += sz;
				if (coff > csz)
					csz = coff;
				if (p->n_type == UNIONTY)
					coff = 0;
			}
			SETOFF(csz, SZCHAR);
			sue->suesize = csz;
			sue->suealign = gap->ga[i].a1.iarg;
			break;

		case GCC_ATYP_ALIGNED:
			break;

		default:
			werror("unsupported attribute %d", gap->ga[i].atype);
		}
	}
	if (align) {
		sue->suealign = align;
		SETOFF(sue->suesize, sue->suealign);
	}
	sue->suega = gap;
}

/*
 * Search for a specific attribute type.
 */
struct gcc_attrib *
gcc_get_attr(struct suedef *sue, int typ)
{
	gcc_ap_t *gap;
	int i;

	if (sue == NULL)
		return NULL;

	if ((gap = sue->suega) == NULL)
		return NULL;

	for (i = 0; i < gap->num; i++)
		if (gap->ga[i].atype == typ)
			return &gap->ga[i];
	if (sue->suep)
		return gcc_get_attr(sue->suep, typ);
	return NULL;
}

#ifdef PCC_DEBUG
void
dump_attr(gcc_ap_t *gap)
{
	int i;

	printf("GCC attributes\n");
	if (gap == NULL)
		return;
	for (i = 0; i < gap->num; i++) {
		printf("%d: ", gap->ga[i].atype);
		printf("%d %d %d", gap->ga[i].a1.iarg,
		    gap->ga[i].a2.iarg, gap->ga[i].a3.iarg);
		printf("\n");
	}
}
#endif
#endif
