/*	Id: common.c,v 1.122 2015/09/30 20:04:30 ragge Exp 	*/	
/*	$NetBSD: common.c,v 1.7 2016/02/09 20:37:32 plunky Exp $	*/
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

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pass2.h"
#include "unicode.h"

# ifndef EXIT
# define EXIT exit
# endif

int nerrors = 0;  /* number of errors */
extern char *ftitle;
int lineno;
int savstringsz, newattrsz, nodesszcnt;

int warniserr = 0;

#ifndef WHERE
#define	WHERE(ch) fprintf(stderr, "%s, line %d: ", ftitle, lineno);
#endif

static void
incerr(void)
{
	if (++nerrors > 30)
		cerror("too many errors");
}

/*
 * nonfatal error message
 * the routine where is different for pass 1 and pass 2;
 * it tells where the error took place
 */
void
uerror(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('u');
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	incerr();
}

/*
 * compiler error: die
 */
void
cerror(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('c');

	/* give the compiler the benefit of the doubt */
	if (nerrors && nerrors <= 30) {
		fprintf(stderr,
		    "cannot recover from earlier errors: goodbye!\n");
	} else {
		fprintf(stderr, "compiler error: ");
		vfprintf(stderr, s, ap);
		fprintf(stderr, "\n");
	}
	va_end(ap);
	EXIT(1);
}

/*
 * warning
 */
void
u8error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	WHERE('w');
	fprintf(stderr, "warning: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	if (warniserr)
		incerr();
}

#ifdef MKEXT
int wdebug;
#endif

/*
 * warning
 */
void
werror(const char *s, ...)
{
	extern int wdebug;
	va_list ap;

	if (wdebug)
		return;
	va_start(ap, s);
	WHERE('w');
	fprintf(stderr, "warning: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	if (warniserr)
		incerr();
}

#ifndef MKEXT

struct Warning {
	char *flag;
	char warn;
	char err;
	char *fmt;
};

/*
 * conditional warnings
 */
struct Warning Warnings[] = {
	{
		"truncate", 0, 0,
		"conversion from '%s' to '%s' may alter its value"
	}, {
		"strict-prototypes", 0, 0,
		"function declaration isn't a prototype"
	}, {
		"missing-prototypes", 0, 0,
		"no previous prototype for `%s'"
	}, {
		"implicit-int", 0, 0,
		"return type defaults to `int'",
	}, {
		"implicit-function-declaration", 0, 0,
		"implicit declaration of function '%s'"
	}, {
		"shadow", 0, 0,
		"declaration of '%s' shadows a %s declaration"
	}, {
		"pointer-sign", 0, 0,
		"illegal pointer combination"
	}, {
		"sign-compare", 0, 0,
		"comparison between signed and unsigned"
	}, {
		"unknown-pragmas", 0, 0,
		"ignoring #pragma %s %s"
	}, {
		"unreachable-code", 0, 0,
		"statement not reached"
	}, {
		"deprecated-declarations", 1, 0,
		"`%s' is deprecated"
	}, {
		"attributes", 1, 0,
		"unsupported attribute `%s'"
	}, {	NULL	}
};

/*
 * set the warn/err status of a conditional warning
 */
int
Wset(char *str, int warn, int err)
{
	struct Warning *w = Warnings;

	for (w = Warnings; w->flag; w++) {
		if (strcmp(w->flag, str) == 0) {
			w->warn = warn;
			w->err = err;
			return 0;
		}
	}
	return 1;
}

/*
 * handle a conditional warning flag.
 */
void
Wflags(char *str)
{
	struct Warning *w;
	int isset, iserr;

	/* handle -Werror specially */
	if (strcmp("error", str) == 0) {
		for (w = Warnings; w->flag; w++)
			w->err = 1;

		warniserr = 1;
		return;
	}

	isset = 1;
	if (strncmp("no-", str, 3) == 0) {
		str += 3;
		isset = 0;
	}

	iserr = 0;
	if (strncmp("error=", str, 6) == 0) {
		str += 6;
		iserr = 1;
	}

	for (w = Warnings; w->flag; w++) {
		if (strcmp(w->flag, str) != 0)
			continue;

		if (isset) {
			if (iserr)
				w->err = 1;
			w->warn = 1;
		} else if (iserr) {
			w->err = 0;
		} else {
			w->warn = 0;
		}

		return;
	}

	fprintf(stderr, "unrecognised warning option '%s'\n", str);
}

/*
 * emit a conditional warning
 */
void
warner(int type, ...)
{
	va_list ap;
	char *t;
#ifndef PASS2
	extern int issyshdr;

	if (issyshdr && type == Wtruncate)
		return; /* Too many false positives */
#endif

	if (Warnings[type].warn == 0)
		return; /* no warning */
	if (Warnings[type].err) {
		t = "error";
		incerr();
	} else
		t = "warning";

	va_start(ap, type);
	fprintf(stderr, "%s:%d: %s: ", ftitle, lineno, t);
	vfprintf(stderr, Warnings[type].fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}
#endif /* MKEXT */

#ifndef MKEXT
static NODE *freelink;
int usednodes;

#ifndef LANG_F77
NODE *
talloc(void)
{
	register NODE *p;

	usednodes++;

	if (freelink != NULL) {
		p = freelink;
		freelink = p->n_left;
		if (p->n_op != FREE)
			cerror("node not FREE: %p", p);
		if (ndebug)
			printf("alloc node %p from freelist\n", p);
		return p;
	}

	p = permalloc(sizeof(NODE));
	nodesszcnt += sizeof(NODE);
	p->n_op = FREE;
	if (ndebug)
		printf("alloc node %p from memory\n", p);
	return p;
}
#endif

/*
 * make a fresh copy of p
 */
NODE *
tcopy(NODE *p)
{
	NODE *q;

	q = talloc();
	*q = *p;

	switch (optype(q->n_op)) {
	case BITYPE:
		q->n_right = tcopy(p->n_right);
	case UTYPE:
		q->n_left = tcopy(p->n_left);
	}

	return(q);
}

#ifndef LANG_F77
/*
 * ensure that all nodes have been freed
 */
void
tcheck(void)
{
#ifdef LANG_CXX
	extern int inlnodecnt;
#else
#define	inlnodecnt 0
#endif

	if (nerrors)
		return;

	if ((usednodes - inlnodecnt) != 0)
		cerror("usednodes == %d, inlnodecnt %d", usednodes, inlnodecnt);
}
#endif

/*
 * free the tree p
 */
void
tfree(NODE *p)
{
	if (p->n_op != FREE)
		walkf(p, (void (*)(NODE *, void *))nfree, 0);
}

/*
 * Free a node, and return its left descendant.
 * It is up to the caller to know whether the return value is usable.
 */
NODE *
nfree(NODE *p)
{
	NODE *l;
#ifdef PCC_DEBUG_NODES
	NODE *q;
#endif

	if (p == NULL)
		cerror("freeing blank node!");
		
	l = p->n_left;
	if (p->n_op == FREE)
		cerror("freeing FREE node", p);
#ifdef PCC_DEBUG_NODES
	q = freelink;
	while (q != NULL) {
		if (q == p)
			cerror("freeing free node %p", p);
		q = q->n_left;
	}
#endif

	if (ndebug)
		printf("freeing node %p\n", p);
	p->n_op = FREE;
	p->n_left = freelink;
	freelink = p;
	usednodes--;
	return l;
}
#endif

#ifdef LANG_F77
#define OPTYPE(x) optype(x)
#else
#define OPTYPE(x) coptype(x)
#endif

#ifdef MKEXT
#define coptype(o)	(dope[o]&TYFLG)
#else
#ifndef PASS2
int cdope(int);
#define coptype(o)	(cdope(o)&TYFLG)
#else
#define coptype(o)	(dope[o]&TYFLG)
#endif
#endif

void
fwalk(NODE *t, void (*f)(NODE *, int, int *, int *), int down)
{

	int down1, down2;

	more:
	down1 = down2 = 0;

	(*f)(t, down, &down1, &down2);

	switch (OPTYPE( t->n_op )) {

	case BITYPE:
		fwalk( t->n_left, f, down1 );
		t = t->n_right;
		down = down2;
		goto more;

	case UTYPE:
		t = t->n_left;
		down = down1;
		goto more;

	}
}

void
walkf(NODE *t, void (*f)(NODE *, void *), void *arg)
{
	int opty;


	opty = OPTYPE(t->n_op);

	if (opty != LTYPE)
		walkf( t->n_left, f, arg );
	if (opty == BITYPE)
		walkf( t->n_right, f, arg );
	(*f)(t, arg);
}

int dope[DSIZE];
char *opst[DSIZE];

struct dopest {
	int dopeop;
	char opst[8];
	int dopeval;
} indope[] = {
	{ NAME, "NAME", LTYPE, },
	{ REG, "REG", LTYPE, },
	{ OREG, "OREG", LTYPE, },
	{ TEMP, "TEMP", LTYPE, },
	{ ICON, "ICON", LTYPE, },
	{ FCON, "FCON", LTYPE, },
	{ CCODES, "CCODES", LTYPE, },
	{ UMINUS, "U-", UTYPE, },
	{ UMUL, "U*", UTYPE, },
	{ FUNARG, "FUNARG", UTYPE, },
	{ UCALL, "UCALL", UTYPE|CALLFLG, },
	{ UFORTCALL, "UFCALL", UTYPE|CALLFLG, },
	{ COMPL, "~", UTYPE, },
	{ FORCE, "FORCE", UTYPE, },
	{ XARG, "XARG", UTYPE, },
	{ XASM, "XASM", BITYPE, },
	{ SCONV, "SCONV", UTYPE, },
	{ PCONV, "PCONV", UTYPE, },
	{ PLUS, "+", BITYPE|FLOFLG|SIMPFLG|COMMFLG, },
	{ MINUS, "-", BITYPE|FLOFLG|SIMPFLG, },
	{ MUL, "*", BITYPE|FLOFLG|MULFLG, },
	{ AND, "&", BITYPE|SIMPFLG|COMMFLG, },
	{ CM, ",", BITYPE, },
	{ ASSIGN, "=", BITYPE|ASGFLG, },
	{ DIV, "/", BITYPE|FLOFLG|MULFLG|DIVFLG, },
	{ MOD, "%", BITYPE|DIVFLG, },
	{ LS, "<<", BITYPE|SHFFLG, },
	{ RS, ">>", BITYPE|SHFFLG, },
	{ OR, "|", BITYPE|COMMFLG|SIMPFLG, },
	{ ER, "^", BITYPE|COMMFLG|SIMPFLG, },
	{ CALL, "CALL", BITYPE|CALLFLG, },
	{ FORTCALL, "FCALL", BITYPE|CALLFLG, },
	{ EQ, "==", BITYPE|LOGFLG, },
	{ NE, "!=", BITYPE|LOGFLG, },
	{ LE, "<=", BITYPE|LOGFLG, },
	{ LT, "<", BITYPE|LOGFLG, },
	{ GE, ">=", BITYPE|LOGFLG, },
	{ GT, ">", BITYPE|LOGFLG, },
	{ UGT, "UGT", BITYPE|LOGFLG, },
	{ UGE, "UGE", BITYPE|LOGFLG, },
	{ ULT, "ULT", BITYPE|LOGFLG, },
	{ ULE, "ULE", BITYPE|LOGFLG, },
	{ CBRANCH, "CBRANCH", BITYPE, },
	{ FLD, "FLD", UTYPE, },
	{ PMCONV, "PMCONV", BITYPE, },
	{ PVCONV, "PVCONV", BITYPE, },
	{ RETURN, "RETURN", BITYPE|ASGFLG|ASGOPFLG, },
	{ GOTO, "GOTO", UTYPE, },
	{ STASG, "STASG", BITYPE|ASGFLG, },
	{ STARG, "STARG", UTYPE, },
	{ STCALL, "STCALL", BITYPE|CALLFLG, },
	{ USTCALL, "USTCALL", UTYPE|CALLFLG, },
	{ ADDROF, "U&", UTYPE, },

	{ -1,	"",	0 },
};

void
mkdope(void)
{
	struct dopest *q;

	for( q = indope; q->dopeop >= 0; ++q ){
		dope[q->dopeop] = q->dopeval;
		opst[q->dopeop] = q->opst;
	}
}

/*
 * output a nice description of the type of t
 */
void
tprint(TWORD t, TWORD q)
{
	static char * tnames[BTMASK+1] = {
		"undef",
		"bool",
		"char",
		"uchar",
		"short",
		"ushort",
		"int",
		"unsigned",
		"long",
		"ulong",
		"longlong",
		"ulonglong",
		"float",
		"double",
		"ldouble",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"void",
		"signed", /* pass1 */
		"farg", /* pass1 */
		"fimag", /* pass1 */
		"dimag", /* pass1 */
		"limag", /* pass1 */
		"fcomplex", /* pass1 */
		"dcomplex", /* pass1 */
		"lcomplex", /* pass1 */
		"enumty", /* pass1 */
		"?", "?", "?"
		};

	for(;; t = DECREF(t), q = DECREF(q)) {
		if (ISCON(q))
			putchar('C');
		if (ISVOL(q))
			putchar('V');

		if (ISPTR(t))
			printf("PTR ");
		else if (ISFTN(t))
			printf("FTN ");
		else if (ISARY(t))
			printf("ARY ");
		else {
			printf("%s%s%s", ISCON(q << TSHIFT) ? "const " : "",
			    ISVOL(q << TSHIFT) ? "volatile " : "", tnames[t]);
			return;
		}
	}
}

/*
 * Memory allocation routines.
 * Memory are allocated from the system in MEMCHUNKSZ blocks.
 * permalloc() returns a bunch of memory that is never freed.
 * Memory allocated through tmpalloc() will be released the
 * next time a function is ended (via tmpfree()).
 */

#define	MEMCHUNKSZ 8192	/* 8k per allocation */
struct balloc {
	char a1;
	union {
		long long l;
		long double d;
	} a2;
};

#define	ALIGNMENT offsetof(struct balloc, a2)
#define	ROUNDUP(x) (((x) + ((ALIGNMENT)-1)) & ~((ALIGNMENT)-1))

static char *allocpole;
static size_t allocleft;
size_t permallocsize, tmpallocsize, lostmem;

void *
permalloc(size_t size)
{
	void *rv;

	if (size > MEMCHUNKSZ) {
		if ((rv = malloc(size)) == NULL)
			cerror("permalloc: missing %d bytes", size);
		return rv;
	}
	if (size == 0)
		cerror("permalloc2");
	if (allocleft < size) {
		/* loses unused bytes */
		lostmem += allocleft;
		if ((allocpole = malloc(MEMCHUNKSZ)) == NULL)
			cerror("permalloc: out of memory");
		allocleft = MEMCHUNKSZ;
	}
	size = ROUNDUP(size);
	rv = &allocpole[MEMCHUNKSZ-allocleft];
	allocleft -= size;
	permallocsize += size;
	return rv;
}

void *
tmpcalloc(size_t size)
{
	void *rv;

	rv = tmpalloc(size);
	memset(rv, 0, size);
	return rv;
}

/*
 * Duplicate a string onto the temporary heap.
 */
char *
tmpstrdup(char *str)
{
	size_t len;

	len = strlen(str) + 1;
	return memcpy(tmpalloc(len), str, len);
}

/*
 * Allocation routines for temporary memory.
 */
#if 0
#define	ALLDEBUG(x)	printf x
#else
#define	ALLDEBUG(x)
#endif

#define	NELEM	((MEMCHUNKSZ-ROUNDUP(sizeof(struct xalloc *)))/ALIGNMENT)
#define	ELEMSZ	(ALIGNMENT)
#define	MAXSZ	(NELEM*ELEMSZ)
struct xalloc {
	struct xalloc *next;
	union {
		struct balloc b; /* for initial alignment */
		char elm[MAXSZ];
	} u;
} *tapole, *tmpole;
int uselem = NELEM; /* next unused element */

void *
tmpalloc(size_t size)
{
	struct xalloc *xp;
	void *rv;
	size_t nelem;

	nelem = ROUNDUP(size)/ELEMSZ;
	ALLDEBUG(("tmpalloc(%ld,%ld) %zd (%zd) ", ELEMSZ, NELEM, size, nelem));
	if (nelem > NELEM/2) {
		size += ROUNDUP(sizeof(struct xalloc *));
		if ((xp = malloc(size)) == NULL)
			cerror("out of memory");
		ALLDEBUG(("XMEM! (%zd,%p) ", size, xp));
		xp->next = tmpole;
		tmpole = xp;
		ALLDEBUG(("rv %p\n", &xp->u.elm[0]));
		return &xp->u.elm[0];
	}
	if (nelem + uselem >= NELEM) {
		ALLDEBUG(("MOREMEM! "));
		/* alloc more */
		if ((xp = malloc(sizeof(struct xalloc))) == NULL)
			cerror("out of memory");
		xp->next = tapole;
		tapole = xp;
		uselem = 0;
	} else
		xp = tapole;
	rv = &xp->u.elm[uselem * ELEMSZ];
	ALLDEBUG(("elemno %d ", uselem));
	uselem += nelem;
	ALLDEBUG(("new %d rv %p\n", uselem, rv));
	return rv;
}

void
tmpfree(void)
{
	struct xalloc *x1;

	while (tmpole) {
		x1 = tmpole;
		tmpole = tmpole->next;
		ALLDEBUG(("XMEM! free %p\n", x1));
		free(x1);
	}
	while (tapole && tapole->next) {
		x1 = tapole;
		tapole = tapole->next;
		ALLDEBUG(("MOREMEM! free %p\n", x1));
		free(x1);
	}
	if (tapole)
		uselem = 0;
}

/*
 * Set a mark for later removal from the temp heap.
 */
void
markset(struct mark *m)
{
	m->tmsav = tmpole;
	m->tasav = tapole;
	m->elem = uselem;
}

/*
 * Remove everything on tmp heap from a mark.
 */
void
markfree(struct mark *m)
{
	struct xalloc *x1;

	while (tmpole != m->tmsav) {
		x1 = tmpole;
		tmpole = tmpole->next;
		free(x1);
	}
	while (tapole != m->tasav) {
		x1 = tapole;
		tapole = tapole->next;
		free(x1);
	}
	uselem = m->elem;
}

/*
 * Allocate space on the permanent stack for a string of length len+1
 * and copy it there.
 * Return the new address.
 */
char *
newstring(char *s, size_t len)
{
	char *u, *c;

	len++;
	savstringsz += len;
	if (allocleft < len) {
		u = c = permalloc(len);
	} else {
		u = c = &allocpole[MEMCHUNKSZ-allocleft];
		allocleft -= ROUNDUP(len);
		permallocsize += ROUNDUP(len);
	}
	while (len--)
		*c++ = *s++;
	return u;
}

/*
 * Do a preorder walk of the CM list p and apply function f on each element.
 */
void
flist(NODE *p, void (*f)(NODE *, void *), void *arg)
{
	if (p->n_op == CM) {
		(*f)(p->n_right, arg);
		flist(p->n_left, f, arg);
	} else
		(*f)(p, arg);
}

/*
 * The same as flist but postorder.
 */
void
listf(NODE *p, void (*f)(NODE *))
{
	if (p->n_op == CM) {
		listf(p->n_left, f);
		(*f)(p->n_right);
	} else
		(*f)(p);
}

/*
 * Get list argument number n from list, or NIL if out of list.
 */
NODE *
listarg(NODE *p, int n, int *cnt)
{
	NODE *r;

	if (p->n_op == CM) {
		r = listarg(p->n_left, n, cnt);
		if (n == ++(*cnt))
			r = p->n_right;
	} else {
		*cnt = 0;
		r = n == 0 ? p : NIL;
	}
	return r;
}

/*
 * Make a type unsigned, if possible.
 */
TWORD
enunsign(TWORD t)
{
	if (BTYPE(t) >= CHAR && BTYPE(t) <= ULONGLONG)
		t |= 1;
	return t;
}

/*
 * Make a type signed, if possible.
 */
TWORD
deunsign(TWORD t)
{
	if (BTYPE(t) >= CHAR && BTYPE(t) <= ULONGLONG)
		t &= ~1;
	return t;
}

/*
 * Attribute functions.
 */
struct attr *
attr_new(int type, int nelem)
{
	struct attr *ap;
	int sz;

	sz = sizeof(struct attr) + nelem * sizeof(union aarg);

	ap = memset(permalloc(sz), 0, sz);
	newattrsz += sz;
	ap->atype = type;
	ap->sz = nelem;
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
attr_dup(struct attr *ap)
{
	int sz = sizeof(struct attr) + ap->sz * sizeof(union aarg);
	ap = memcpy(permalloc(sz), ap, sz);
	ap->next = NULL;
	return ap;
}

void *
xmalloc(int size)
{
	void *rv;

	if ((rv = malloc(size)) == NULL)
		cerror("out of memory!");
	return rv;
}

void *
xstrdup(char *s)
{
	void *rv;

	if ((rv = strdup(s)) == NULL)
		cerror("out of memory!");
	return rv;
}

void *
xcalloc(int a, int b)
{
	void *rv;

	if ((rv = calloc(a, b)) == NULL)
		cerror("out of memory!");
	return rv;
}
