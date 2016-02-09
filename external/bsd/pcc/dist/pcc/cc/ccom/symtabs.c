/*	Id: symtabs.c,v 1.38 2015/09/15 20:01:10 ragge Exp 	*/	
/*	$NetBSD: symtabs.c,v 1.1.1.5 2016/02/09 20:28:53 plunky Exp $	*/
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


#include "pass1.h"
#include "unicode.h"
#include <stdlib.h>

#define	NODE P1ND
#define	fwalk p1fwalk

/*
 * These definitions are used in the patricia tree that stores
 * the strings.
 */
#define	LEFT_IS_LEAF		0x80000000
#define	RIGHT_IS_LEAF		0x40000000
#define	IS_LEFT_LEAF(x)		(((x) & LEFT_IS_LEAF) != 0)
#define	IS_RIGHT_LEAF(x)	(((x) & RIGHT_IS_LEAF) != 0)
#define	BITNO(x)		((x) & ~(LEFT_IS_LEAF|RIGHT_IS_LEAF))
#define	CHECKBITS		8

struct tree {
	int bitno;
	struct tree *lr[2];
};

extern int dimfuncnt;
static struct tree *firstname;
int nametabs, namestrlen;
static struct tree *firststr;
int strtabs, strstrlen, symtreecnt;
static char *symtab_add(char *key, struct tree **, int *, int *);
int lastloc = NOSEG;
int treestrsz = sizeof(struct tree);

#define	P_BIT(key, bit) (key[bit >> 3] >> (bit & 7)) & 1
#define	getree() permalloc(sizeof(struct tree))

char *
addname(char *key)      
{
	return symtab_add(key, &firstname, &nametabs, &namestrlen);
}

char *
addstring(char *key)
{
	return symtab_add(key, &firststr, &strtabs, &strstrlen);
}

/*
 * Add a name to the name stack (if its non-existing),
 * return its address.
 * This is a simple patricia implementation.
 */
static char *
symtab_add(char *key, struct tree **first, int *tabs, int *stlen)
{
	struct tree *w, *new, *last;
	int cix, bit, fbit, svbit, ix, bitno, len;
	char *m, *k, *sm;

	/* Count full string length */
	for (k = key, len = 0; *k; k++, len++)
		;
	
	switch (*tabs) {
	case 0:
		*first = (struct tree *)newstring(key, len);
		*stlen += (len + 1);
		(*tabs)++;
		return (char *)*first;

	case 1:
		m = (char *)*first;
		svbit = 0; /* XXX why? */
		break;

	default:
		w = *first;
		bitno = len * CHECKBITS;
		for (;;) {
			bit = BITNO(w->bitno);
			fbit = bit > bitno ? 0 : P_BIT(key, bit);
			svbit = fbit ? IS_RIGHT_LEAF(w->bitno) :
			    IS_LEFT_LEAF(w->bitno);
			w = w->lr[fbit];
			if (svbit) {
				m = (char *)w;
				break;
			}
		}
	}

	sm = m;
	k = key;

	/* Check for correct string and return */
	for (cix = 0; *m && *k && *m == *k; m++, k++, cix += CHECKBITS)
		;
	if (*m == 0 && *k == 0)
		return sm;

	ix = *m ^ *k;
	while ((ix & 1) == 0)
		ix >>= 1, cix++;

	/* Create new node */
	new = getree();
	bit = P_BIT(key, cix);
	new->bitno = cix | (bit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	new->lr[bit] = (struct tree *)newstring(key, len);
	*stlen += (len + 1);

	if ((*tabs)++ == 1) {
		new->lr[!bit] = *first;
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
		*first = new;
		return (char *)new->lr[bit];
	}


	w = *first;
	last = NULL;
	for (;;) {
		fbit = w->bitno;
		bitno = BITNO(w->bitno);
		if (bitno == cix)
			cerror("bitno == cix");
		if (bitno > cix)
			break;
		svbit = P_BIT(key, bitno);
		last = w;
		w = w->lr[svbit];
		if (fbit & (svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF))
			break;
	}

	new->lr[!bit] = w;
	if (last == NULL) {
		*first = new;
	} else {
		last->lr[svbit] = new;
		last->bitno &= ~(svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	}
	if (bitno < cix)
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
	return (char *)new->lr[bit];
}

static struct tree *sympole[NSTYPES];
static struct symtab *tmpsyms[NSTYPES];
int numsyms[NSTYPES];

/*
 * Inserts a symbol into the symbol tree.
 * Returns a struct symtab.
 */
struct symtab *
lookup(char *key, int stype)
{
	struct symtab *sym;
	struct tree *w, *new, *last;
	int cix, bit, fbit, svbit, bitno;
	int type, uselvl;
	intptr_t ix, match, code = (intptr_t)key;

	type = stype & SMASK;
	uselvl = (blevel > 0 && type != SSTRING);

	/*
	 * The local symbols are kept in a simple linked list.
	 * Check this list first.
	 */
	if (blevel > 0)
		for (sym = tmpsyms[type]; sym; sym = sym->snext)
			if (sym->sname == key)
				return sym;

	switch (numsyms[type]) {
	case 0:
		if (stype & SNOCREAT)
			return NULL;
		if (uselvl) {
			if (type == SNORMAL)
				stype |= SBLK;
			sym = getsymtab(key, stype);
			sym->snext = tmpsyms[type];
			tmpsyms[type] = sym;
			return sym;
		}
		sympole[type] = (struct tree *)getsymtab(key, stype);
		numsyms[type]++;
		return (struct symtab *)sympole[type];

	case 1:
		w = (struct tree *)sympole[type];
		svbit = 0; /* XXX why? */
		break;

	default:
		w = sympole[type];
		for (;;) {
			bit = BITNO(w->bitno);
			fbit = (code >> bit) & 1;
			svbit = fbit ? IS_RIGHT_LEAF(w->bitno) :
			    IS_LEFT_LEAF(w->bitno);
			w = w->lr[fbit];
			if (svbit)
				break;
		}
	}

	sym = (struct symtab *)w;
	match = (intptr_t)sym->sname;

	ix = code ^ match;
	if (ix == 0)
		return sym;
	else if (stype & SNOCREAT)
		return NULL;

#ifdef PCC_DEBUG
	if (ddebug)
		printf("	adding %s as %s at level %d\n",
		    key, uselvl ? "temp" : "perm", blevel);
#endif

	/*
	 * Insert into the linked list, if feasible.
	 */
	if (uselvl) {
		sym = getsymtab(key, stype|STEMP);
		sym->snext = tmpsyms[type];
		tmpsyms[type] = sym;
		return sym;
	}

	/*
	 * Need a new node. If type is SNORMAL and inside a function
	 * the node must be allocated as permanent anyway.
	 * This could be optimized by adding a remove routine, but it
	 * may be more trouble than it is worth.
	 */
	if (stype == (STEMP|SNORMAL))
		stype = SNORMAL;

	for (cix = 0; (ix & 1) == 0; ix >>= 1, cix++)
		;

	new = (symtreecnt++, permalloc(sizeof(struct tree)));
	bit = (code >> cix) & 1;
	new->bitno = cix | (bit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	new->lr[bit] = (struct tree *)getsymtab(key, stype);
	if (numsyms[type]++ == 1) {
		new->lr[!bit] = sympole[type];
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
		sympole[type] = new;
		return (struct symtab *)new->lr[bit];
	}


	w = sympole[type];
	last = NULL;
	for (;;) {
		fbit = w->bitno;
		bitno = BITNO(w->bitno);
		if (bitno == cix)
			cerror("bitno == cix");
		if (bitno > cix)
			break;
		svbit = (code >> bitno) & 1;
		last = w;
		w = w->lr[svbit];
		if (fbit & (svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF))
			break;
	}

	new->lr[!bit] = w;
	if (last == NULL) {
		sympole[type] = new;
	} else {
		last->lr[svbit] = new;
		last->bitno &= ~(svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	}
	if (bitno < cix)
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
	return (struct symtab *)new->lr[bit];
}

void
symclear(int level)
{
	struct symtab *s;
	int i;

#ifdef PCC_DEBUG
	if (ddebug)
		printf("symclear(%d)\n", level);
#endif
	if (level < 1) {
		for (i = 0; i < NSTYPES; i++) {
			s = tmpsyms[i];
			tmpsyms[i] = 0;
			if (i != SLBLNAME)
				continue;
			while (s != NULL) {
				if (s->soffset < 0)
					uerror("label '%s' undefined",s->sname);
				s = s->snext;
			}
		}
	} else {
		for (i = 0; i < NSTYPES; i++) {
			if (i == SLBLNAME)
				continue; /* function scope */
			while (tmpsyms[i] != NULL &&
			    tmpsyms[i]->slevel > level) {
				tmpsyms[i] = tmpsyms[i]->snext;
			}
		}
	}
}

struct symtab *
hide(struct symtab *sym)
{
	struct symtab *new;
	int typ = sym->sflags & SMASK;

	new = getsymtab(sym->sname, typ|STEMP);
	new->snext = tmpsyms[typ];
	tmpsyms[typ] = new;

#ifdef PCC_DEBUG
	if (ddebug)
		printf("\t%s hidden at level %d (%p -> %p)\n",
		    sym->sname, blevel, sym, new);
#endif
	return new;
}

/*
 * Extract correct segment for the specified symbol and call
 * target routines to print it out.
 * If symtab entry is specified, output alignment as well.
 */
void
locctr(int seg, struct symtab *sp)
{
#ifdef GCC_COMPAT
	struct attr *ga;
#endif

	if (seg == NOSEG) {
		;
	} else if (sp == NULL) {
		if (lastloc != seg)
			setseg(seg, NULL);
#ifdef GCC_COMPAT
	} else if ((ga = attr_find(sp->sap, GCC_ATYP_SECTION)) != NULL) {
		setseg(NMSEG, ga->sarg(0));
		seg = NOSEG;
#endif
	} else {
		if (seg == DATA) {
			if (ISCON(cqual(sp->stype, sp->squal)))
				seg = RDATA;
			else if (sp->sclass == STATIC)
				seg = LDATA;
		}
		if (sp->sflags & STLS) {
			if (seg == DATA || seg == LDATA)
				seg = TLSDATA;
			if (seg == UDATA) seg = TLSUDATA;
		} else if (kflag) {
			if (seg == DATA) seg = PICDATA;
			if (seg == RDATA) seg = PICRDATA;
			if (seg == LDATA) seg = PICLDATA;
		}
		if (lastloc != seg)
			setseg(seg, NULL);
	}
	lastloc = seg;

	/* setup alignment */
#ifndef ALFTN
#define	ALFTN	ALINT
#endif
	if (sp) {
		int al;

		if (ISFTN(sp->stype)) {
			al = ALFTN;
		} else
			al = talign(sp->stype, sp->sap);
		defalign(al);
		symdirec(sp);
	}
}

#ifndef MYALIGN
void
defalign(int al)
{
#ifdef	HASP2ALIGN
#define	P2ALIGN(x)	ispow2(x)
#else
#define	P2ALIGN(x)	(x)
#endif
	if (al != ALCHAR)
		printf(PRTPREF "\t.align %d\n", P2ALIGN(al/ALCHAR));
}
#endif

#ifndef MYDIREC
/*
 * Directives given as attributes to symbols.
 */
void
symdirec(struct symtab *sp)
{
#ifdef GCC_COMPAT
	struct attr *ga;
	char *name;

	name = getexname(sp);
	if ((ga = attr_find(sp->sap, GCC_ATYP_WEAK)) != NULL)
		printf(PRTPREF "\t.weak %s\n", name);
	if ((ga = attr_find(sp->sap, GCC_ATYP_VISIBILITY)) &&
	    strcmp(ga->sarg(0), "default"))
		printf(PRTPREF "\t.%s %s\n", ga->sarg(0), name);
	if ((ga = attr_find(sp->sap, GCC_ATYP_ALIASWEAK))) {
		printf(PRTPREF "\t.weak %s\n", ga->sarg(0));
		printf(PRTPREF "\t.set %s,%s\n", ga->sarg(0), name);
	}
#endif
}
#endif

char *
getexname(struct symtab *sp)
{
	struct attr *ap = attr_find(sp->sap, ATTR_SONAME);

	return (ap ? ap->sarg(0) : addname(exname(sp->sname)));
}

static char *csbuf;
static int csbufp, cssz, strtype;
#ifndef NO_STRING_SAVE
static struct symtab *strpole;
#endif
#define	STCHNK	128

static void
savch(int ch)
{
	if (csbufp == cssz) {
		cssz += STCHNK;
		csbuf = realloc(csbuf, cssz);
	}
	csbuf[csbufp++] = ch;
}

/*
 * save value as 3-digit octal escape sequence
 */
static void
voct(unsigned int v)
{
	savch('\\');
	savch(((v & 0700) >> 6) + '0');
	savch(((v & 0070) >> 3) + '0');
	savch((v & 0007) + '0');
}


/*
 * Add string new to string old.  
 * String new must come directly after old.
 * new is expected to be utf-8.  Will be cleaned slightly here.
 */
char *
stradd(char *old, char *new)
{
	if (old == NULL) {
		strtype = 0;
		csbufp = 0;
	} else if (old != csbuf)
		cerror("string synk error");

	/* special hack for function names */
	for (old = new; *old; old++)
		;
	if (old[-1] != '\"') {
		do {
			savch(*new);
		} while (*new++);
		return csbuf;
	}

	if (*new != '\"') {
		int ny = *new++;
		if (ny == 'u' && *new == '8')
			ny = '8', new++;
		if (strtype && ny != strtype)
			uerror("clash in string types");
		strtype = ny;
	}
	if (*new++ != '\"')
		cerror("snuff synk error");

	while (*new != '\"') {
		if (*new == '\\') {
			voct(esccon(&new));
		} else if (*new < ' ' || *new > '~') {
			voct(*(unsigned char *)new++);
		} else {
			savch(*new++);
		}
	}
	savch(0);
	csbufp--;
	return csbuf;
}

TWORD
styp(void)
{
	TWORD t;

	if (strtype == 0 || strtype == '8')
		t = xuchar ? UCHAR+ARY : CHAR+ARY;
	else if (strtype == 'u')
		t = ctype(USHORT)+ARY;
	else if (strtype == 'L')
		t = WCHAR_TYPE+ARY;
	else
		t = ctype(SZINT < 32 ? ULONG : UNSIGNED)+ARY;
	return t;
}

/*
 * Create a string struct.
 */
static void
strst(struct symtab *sp, TWORD t)
{
	char *wr;
	int i;

	sp->sclass = STATIC;
	sp->slevel = 1;
	sp->soffset = getlab();
	sp->squal = (CON >> TSHIFT);
#ifndef NO_STRING_SAVE
	sp->sdf = permalloc(sizeof(union dimfun));
#else
	sp->sdf = stmtalloc(sizeof(union dimfun));
#endif
	dimfuncnt++;
	sp->stype = t;

	for (wr = sp->sname, i = 1; *wr; i++) {
		if (strtype == 'L' || strtype == 'U' || strtype == 'u')
			(void)u82cp(&wr);
		else if (*wr == '\\')
			(void)esccon(&wr);
		else
			wr++;
	}
	sp->sdf->ddim = i;
#ifndef NO_STRING_SAVE
	sp->snext = strpole;
	strpole = sp;
#endif
}

/*
 * Save string (if needed) and return NODE for it.
 * String is already in utf-8 format.
 */
NODE *
strend(char *s, TWORD t)
{
	struct symtab *sp, *sp2;
	NODE *p;

#ifdef NO_STRING_SAVE
	sp = getsymtab(s, SSTRING|SSTMT);
#else
	s = addstring(s);
	sp = lookup(s, SSTRING);
#endif

	if (sp->soffset && sp->stype != t) {
		/* same string stored but different type */
		/* This is uncommon, create a new symtab struct for it */
		sp2 = permalloc(sizeof(*sp));
		*sp2 = *sp;
		strst(sp2, t);
		sp = sp2;
	} else if (sp->soffset == 0) { /* No string */
		strst(sp, t);
	}
	if (cssz > STCHNK) {
		cssz = STCHNK;
		csbuf = realloc(csbuf, cssz);
	}
#ifdef NO_STRING_SAVE
	instring(sp);
#endif
	p = block(NAME, NIL, NIL, sp->stype, sp->sdf, sp->sap);
	p->n_sp = sp;
	return(clocal(p));
}

#ifndef NO_STRING_SAVE
/*
 * Print out strings that have been referenced.
 */
void
strprint(void)
{
	struct symtab *sp;

	for (sp = strpole; sp; sp = sp->snext) {
		if ((sp->sflags & SASG) == 0)
			continue; /* not referenced */
		instring(sp);
	}
}
#endif
