/*	Id: local.c,v 1.66 2012/03/23 17:03:09 ragge Exp 	*/	
/*	$NetBSD: local.c,v 1.1.1.4.2.1 2013/02/25 00:26:45 tls Exp $	*/
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

/*	this file contains code which is dependent on the target machine */

/*
 * Check if a constant is too large for a type.
 */
#ifdef notyet
static int
toolarge(TWORD t, CONSZ con)
{
	U_CONSZ ucon = con;

	switch (t) {
	case ULONG:
	case LONG:
	case ULONGLONG:
	case LONGLONG:
		break; /* cannot be too large */
#define	SCHK(i)	case i: if (con > MAX_##i || con < MIN_##i) return 1; break
#define	UCHK(i)	case i: if (ucon > MAX_##i) return 1; break
	SCHK(INT);
	SCHK(SHORT);
	case BOOL:
	SCHK(CHAR);
	UCHK(UNSIGNED);
	UCHK(USHORT);
	UCHK(UCHAR);
	default:
		cerror("toolarge");
	}
	return 0;
}
#endif

#define	IALLOC(sz)	(isinlining ? permalloc(sz) : tmpalloc(sz))

/*
 * Make a symtab entry for PIC use.
 */
static struct symtab *
picsymtab(char *p, char *s, char *s2)
{
	struct symtab *sp = IALLOC(sizeof(struct symtab));
	size_t len = strlen(p) + strlen(s) + strlen(s2) + 1;

	sp->sname = sp->soname = IALLOC(len);
	strlcpy(sp->soname, p, len);
	strlcat(sp->soname, s, len);
	strlcat(sp->soname, s2, len);
	sp->sclass = EXTERN;
	sp->sflags = sp->slevel = 0;
	return sp;
}

int gotnr; /* tempnum for GOT register */
int argstacksize;

/*
 * Create a reference for an extern variable or function.
 */
static NODE *
picext(NODE *p)
{
#if defined(ELFABI)

	NODE *q;
	struct symtab *sp;
	char *c;

	if (p->n_sp->sflags & SBEENHERE)
		return p;

	c = p->n_sp->soname ? p->n_sp->soname : exname(p->n_sp->sname);
	sp = picsymtab("", c, "@GOTPCREL");
	sp->sflags |= SBEENHERE;
	q = block(NAME, NIL, NIL, INCREF(p->n_type), p->n_df, p->n_ap);
	q->n_sp = sp;
	q = block(UMUL, q, 0, p->n_type, p->n_df, p->n_ap);
	q->n_sp = sp;
	nfree(p);
	return q;

#elif defined(MACHOABI)

	return p;

#endif
}

static NODE *
cmop(NODE *l, NODE *r)
{
	return block(CM, l, r, INT, 0, 0);
}

static NODE *
mkx(char *s, NODE *p)
{
	p = block(XARG, p, NIL, INT, 0, 0);
	p->n_name = s;
	return p;
}

static char *
mk3str(char *s1, char *s2, char *s3)
{
	int len = strlen(s1) + strlen(s2) + strlen(s3) + 1;
	char *sd;

	sd = inlalloc(len);
	strlcpy(sd, s1, len);
	strlcat(sd, s2, len);
	strlcat(sd, s3, len);
	return sd;
}

/*
 * Create a reference for a TLS variable.
 * This is the "General dynamic" version.
 */
static NODE *
tlspic(NODE *p)
{
	NODE *q, *r, *s;
	char *s1, *s2;

	/*
	 * .byte   0x66
	 * leaq x@TLSGD(%rip),%rdi
	 * .word   0x6666
	 * rex64
	 * call __tls_get_addr@PLT
	 */

	/* Need the .byte stuff around.  Why? */
	/* Use inline assembler */
	q = mkx("%rdx", bcon(0));
	q = cmop(q, mkx("%rcx", bcon(0)));
	q = cmop(q, mkx("%rsi", bcon(0)));
	q = cmop(q, mkx("%rdi", bcon(0)));
	q = cmop(q, mkx("%r8", bcon(0)));
	q = cmop(q, mkx("%r9", bcon(0)));
	q = cmop(q, mkx("%r10", bcon(0)));
	q = cmop(q, mkx("%r11", bcon(0)));

	s = ccopy(r = tempnode(0, INCREF(p->n_type), p->n_df, p->n_ap));
	r = mkx("=a", r);
	r = block(XASM, r, q, INT, 0, 0);

	/* Create the magic string */
	s1 = ".byte 0x66\n\tleaq ";
	s2 = "@TLSGD(%%rip),%%rdi\n"
	    "\t.word 0x6666\n\trex64\n\tcall __tls_get_addr@PLT";
	if (p->n_sp->soname == NULL)
		p->n_sp->soname = p->n_sp->sname;
	r->n_name = mk3str(s1, p->n_sp->soname, s2);

	r = block(COMOP, r, s, INCREF(p->n_type), p->n_df, p->n_ap);
	r = buildtree(UMUL, r, NIL);
	tfree(p);
	return r;
}

/*
 * The "initial exec" tls model.
 */
static NODE *
tlsinitialexec(NODE *p)
{
	NODE *q, *r, *s;
	char *s1, *s2;

	/*
	 * movq %fs:0,%rax
	 * addq x@GOTTPOFF(%rip),%rax
	 */

	q = bcon(0);
	q->n_type = STRTY;

	s = ccopy(r = tempnode(0, INCREF(p->n_type), p->n_df, p->n_ap));
	r = mkx("=r", r);
	r = block(XASM, r, q, INT, 0, 0);

	s1 = "movq %%fs:0,%0\n\taddq ";
	s2 = "@GOTTPOFF(%%rip),%0";
	if (p->n_sp->soname == NULL)
		p->n_sp->soname = p->n_sp->sname;
	r->n_name = mk3str(s1, p->n_sp->soname, s2);

	r = block(COMOP, r, s, INCREF(p->n_type), p->n_df, p->n_ap);
	r = buildtree(UMUL, r, NIL);
	tfree(p);
	return r;
}

static NODE *
tlsref(NODE *p)
{
	struct symtab *sp = p->n_sp;
	struct attr *ga;
	char *c;

	if ((ga = attr_find(sp->sap, GCC_ATYP_TLSMODEL)) != NULL) {
		c = ga->sarg(0);
		if (strcmp(c, "initial-exec") == 0)
			return tlsinitialexec(p);
		else if (strcmp(c, "global-dynamic") == 0)
			;
		else
			werror("unsupported tls model '%s'", c);
	}
	return tlspic(p);
}

/* clocal() is called to do local transformations on
 * an expression tree preparitory to its being
 * written out in intermediate code.
 *
 * the major essential job is rewriting the
 * automatic variables and arguments in terms of
 * REG and OREG nodes
 * conversion ops which are not necessary are also clobbered here
 * in addition, any special features (such as rewriting
 * exclusive or) are easily handled here as well
 */
NODE *
clocal(NODE *p)
{

	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	register int m;
	TWORD t;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	switch( o = p->n_op ){

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */

		switch (q->sclass) {

		case PARAM:
		case AUTO:
			/* fake up a structure reference */
			r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			r->n_lval = 0;
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case USTATIC:
			if (kflag == 0)
				break;
			/* FALLTHROUGH */
		case STATIC:
#ifdef TLS
			if (q->sflags & STLS) {
				p = tlsref(p);
				break;
			}
#endif
			break;

		case REGISTER:
			p->n_op = REG;
			p->n_lval = 0;
			p->n_rval = q->soffset;
			break;

		case EXTERN:
		case EXTDEF:
			if (q->sflags & STLS) {
				p = tlsref(p);
				break;
			}
			if (kflag == 0 || statinit)
				break;
			if (blevel > 0)
				p = picext(p);
			break;
		}
		break;

	case UCALL:
	case USTCALL:
		/* For now, always clear eax */
		l = block(REG, NIL, NIL, INT, 0, 0);
		regno(l) = RAX;
		p->n_right = clocal(buildtree(ASSIGN, l, bcon(0)));
		p->n_op -= (UCALL-CALL);
		break;

	case SCONV:
		/* Special-case shifts */
		if (p->n_type == LONG && (l = p->n_left)->n_op == LS && 
		    l->n_type == INT && l->n_right->n_op == ICON) {
			p->n_left = l->n_left;
			p = buildtree(LS, p, l->n_right);
			nfree(l);
			break;
		}

		l = p->n_left;

		/* Float conversions may need extra casts */
		if (p->n_type == FLOAT || p->n_type == DOUBLE ||
		    p->n_type == LDOUBLE) {
			if (l->n_type < INT || l->n_type == BOOL) {
				p->n_left = block(SCONV, l, NIL,
				    ISUNSIGNED(l->n_type) ? UNSIGNED : INT,
				    l->n_df, l->n_ap);
				break;
			}
		}

		if (p->n_type == l->n_type) {
			nfree(p);
			return l;
		}

		if ((p->n_type & TMASK) == 0 && (l->n_type & TMASK) == 0 &&
		    tsize(p->n_type, p->n_df, p->n_ap) ==
		    tsize(l->n_type, l->n_df, l->n_ap)) {
			if (p->n_type != FLOAT && p->n_type != DOUBLE &&
			    l->n_type != FLOAT && l->n_type != DOUBLE &&
			    l->n_type != LDOUBLE && p->n_type != LDOUBLE) {
				if (l->n_op == NAME || l->n_op == UMUL ||
				    l->n_op == TEMP) {
					l->n_type = p->n_type;
					nfree(p);
					return l;
				}
			}
		}

		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT &&
		    coptype(l->n_op) == BITYPE && l->n_op != COMOP &&
		    l->n_op != QUEST) {
			l->n_type = p->n_type;
			nfree(p);
			return l;
		}

		o = l->n_op;
		m = p->n_type;

		if (o == ICON) {
			CONSZ val = l->n_lval;

			if (!ISPTR(m)) /* Pointers don't need to be conv'd */
			    switch (m) {
			case BOOL:
				l->n_lval = nncon(l) ? (l->n_lval != 0) : 1;
				l->n_sp = NULL;
				break;
			case CHAR:
				l->n_lval = (char)val;
				break;
			case UCHAR:
				l->n_lval = val & 0377;
				break;
			case SHORT:
				l->n_lval = (short)val;
				break;
			case USHORT:
				l->n_lval = val & 0177777;
				break;
			case UNSIGNED:
				l->n_lval = val & 0xffffffff;
				break;
			case INT:
				l->n_lval = (int)val;
				break;
			case LONG:
			case LONGLONG:
				l->n_lval = (long long)val;
				break;
			case ULONG:
			case ULONGLONG:
				l->n_lval = val;
				break;
			case VOID:
				break;
			case LDOUBLE:
			case DOUBLE:
			case FLOAT:
				l->n_op = FCON;
				l->n_dcon = val;
				break;
			default:
				cerror("unknown type %d", m);
			}
			l->n_type = m;
			l->n_ap = NULL;
			nfree(p);
			return l;
		} else if (l->n_op == FCON) {
			if (p->n_type == BOOL)
				l->n_lval = l->n_dcon != 0.0;
			else
				l->n_lval = l->n_dcon;
			l->n_sp = NULL;
			l->n_op = ICON;
			l->n_type = m;
			l->n_ap = NULL;
			nfree(p);
			return clocal(l);
		}
		if ((p->n_type == CHAR || p->n_type == UCHAR ||
		    p->n_type == SHORT || p->n_type == USHORT) &&
		    (l->n_type == FLOAT || l->n_type == DOUBLE ||
		    l->n_type == LDOUBLE)) {
			p = block(SCONV, p, NIL, p->n_type, p->n_df, p->n_ap);
			p->n_left->n_type = INT;
			return p;
		}
		break;

	case MOD:
	case DIV:
		if (o == DIV && p->n_type != CHAR && p->n_type != SHORT)
			break;
		if (o == MOD && p->n_type != CHAR && p->n_type != SHORT)
			break;
		/* make it an int division by inserting conversions */
		p->n_left = makety(p->n_left, INT, 0, 0, 0);
		p->n_right = makety(p->n_right, INT, 0, 0, 0);
		p = makety(p, p->n_type, 0, 0, 0);
		p->n_left->n_type = INT;
		break;

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		t = p->n_type;
		if (ISITY(t))
			t = t - (FIMAG-FLOAT);
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(CHAR) : RETREG(t);
		break;

	case LS:
	case RS:
		/* shift count must be in a char */
		if (p->n_right->n_type == CHAR || p->n_right->n_type == UCHAR)
			break;
		p->n_right = makety(p->n_right, CHAR, 0, 0, 0);
		break;
	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	return(p);
}

void
myp2tree(NODE *p)
{
	struct symtab *sp, sps;
	static int dblxor, fltxor;

	if (p->n_op == UMINUS && (p->n_type == FLOAT || p->n_type == DOUBLE)) {
		/* Store xor code for sign change */
		if (dblxor == 0) {
			dblxor = getlab();
			fltxor = getlab();
			sps.stype = LDOUBLE;
			sps.squal = CON >> TSHIFT;
			sps.sflags = sps.sclass = 0;
			sps.sname = sps.soname = "";
			sps.slevel = 1;
			sps.sap = NULL;
			sps.soffset = dblxor;
			locctr(DATA, &sps);
			defloc(&sps);
			printf("\t.long 0,0x80000000,0,0\n");
			printf(LABFMT ":\n", fltxor);
			printf("\t.long 0x80000000,0,0,0\n");
		}
		p->n_label = p->n_type == FLOAT ? fltxor : dblxor;
		return;
	}
	if (kflag && (cdope(p->n_op) & CALLFLG) && p->n_left->n_op == NAME) {
		/* Convert @GOTPCREL to @PLT */
		char *s;

		sp = p->n_left->n_sp;
		if ((s = strstr(sp->sname, "@GOTPCREL")) != NULL) {
			strcpy(s, "@PLT");
			p->n_left->n_op = ICON;
		}
		return;
	}
	if (p->n_op != FCON)
		return;

#ifdef mach_amd64
	{
		/* Do not lose negative zeros */
		long long ll[2];
		short ss;
		memcpy(ll, &p->n_dcon, sizeof(ll));
		memcpy(&ss, &ll[1], sizeof(ss));
		if (ll[0] == 0 && ss == 0)
			return;
	}
#else
#error fixme
#endif

	/* XXX should let float constants follow */
	sp = IALLOC(sizeof(struct symtab));
	sp->sclass = STATIC;
	sp->sap = NULL;
	sp->slevel = 1; /* fake numeric label */
	sp->soffset = getlab();
	sp->sflags = 0;
	sp->stype = p->n_type;
	sp->squal = (CON >> TSHIFT);
	sp->sname = sp->soname = NULL;

	locctr(DATA, sp);
	defloc(sp);
	ninval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

	p->n_op = NAME;
	p->n_lval = 0;
	p->n_sp = sp;
}

/*
 * Convert ADDROF NAME to ICON?
 */
int
andable(NODE *p)
{
	if (p->n_sp->sclass == STATIC || p->n_sp->sclass == USTATIC)
		return 1;
	return !kflag;
}

/*
 * Return 1 if a variable of type type is OK to put in register.
 */
int
cisreg(TWORD t)
{
	if (t == LDOUBLE)
		return 0;
	return 1;
}

/*
 * Allocate off bits on the stack.  p is a tree that when evaluated
 * is the multiply count for off, t is a storeable node where to write
 * the allocated address.
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *sp;

	p = buildtree(MUL, p, bcon(off/SZCHAR));
	p = buildtree(PLUS, p, bcon(30));
	p = buildtree(AND, p, xbcon(-16, NULL, LONG));

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(MINUSEQ, sp, p));

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+LONG, t->n_df, t->n_ap);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

}

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
	union { float f; double d; long double l; int i[3]; } u;

	switch (p->n_type) {
	case LDOUBLE:
		u.i[2] = 0;
		u.l = (long double)p->n_dcon;
#if defined(HOST_BIG_ENDIAN)
		/* XXX probably broken on most hosts */
		printf("\t.long\t0x%x,0x%x,0x%x,0\n", u.i[2], u.i[1], u.i[0]);
#else
		printf("\t.long\t0x%x,0x%x,0x%x,0\n", u.i[0], u.i[1], u.i[2]);
#endif
		break;
	case DOUBLE:
		u.d = (double)p->n_dcon;
#if defined(HOST_BIG_ENDIAN)
		printf("\t.long\t0x%x,0x%x\n", u.i[1], u.i[0]);
#else
		printf("\t.long\t0x%x,0x%x\n", u.i[0], u.i[1]);
#endif
		break;
	case FLOAT:
		u.f = (float)p->n_dcon;
		printf("\t.long\t0x%x\n", u.i[0]);
		break;
	default:
		return 0;
	}
	return 1;
}

/* make a name look like an external name in the local machine */
char *
exname(char *p)
{
#ifdef MACHOABI

#define NCHNAM	256
	static char text[NCHNAM+1];
	int i;

	if (p == NULL)
		return "";

	text[0] = '_';
	for (i=1; *p && i<NCHNAM; ++i)
		text[i] = *p++;

	text[i] = '\0';
	text[NCHNAM] = '\0';  /* truncate */

	return (text);
#else
	return (p == NULL ? "" : p);
#endif
}

/*
 * map types which are not defined on the local machine
 */
TWORD
ctype(TWORD type)
{
	switch (BTYPE(type)) {
	case LONGLONG:
		MODTYPE(type,LONG);
		break;

	case ULONGLONG:
		MODTYPE(type,ULONG);

	}
	return (type);
}

void
calldec(NODE *p, NODE *q) 
{
}

void
extdec(struct symtab *q)
{
}

/* make a common declaration for id, if reasonable */
void
defzero(struct symtab *sp)
{
	int off, al;
	char *name;

	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);
	off = tsize(sp->stype, sp->sdf, sp->sap);
	SETOFF(off,SZCHAR);
	off /= SZCHAR;
	al = talign(sp->stype, sp->sap)/SZCHAR;

	if (sp->sclass == STATIC) {
		if (sp->slevel == 0) {
			printf("\t.local %s\n", name);
		} else
			printf("\t.local " LABFMT "\n", sp->soffset);
	}
	if (sp->slevel == 0) {
		printf("\t.comm %s,0%o,%d\n", name, off, al);
	} else
		printf("\t.comm " LABFMT ",0%o,%d\n", sp->soffset, off, al);
}

static char *
section2string(char *name, int len)
{
	char *s;
	int n;

	if (strncmp(name, "link_set", 8) == 0) {
		const char *postfix = ",\"aw\",@progbits";
		n = len + strlen(postfix) + 1;
		s = IALLOC(n);
		strlcpy(s, name, n);
		strlcat(s, postfix, n);
		return s;
	}

	return newstring(name, len);
}

char *nextsect;
static int gottls;
static char *alias;
static int constructor;
static int destructor;

/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	char *a2 = pragtok(NULL);

	if (strcmp(str, "tls") == 0 && a2 == NULL) {
		gottls = 1;
		return 1;
	}
	if (strcmp(str, "constructor") == 0 || strcmp(str, "init") == 0) {
		constructor = 1;
		return 1;
	}
	if (strcmp(str, "destructor") == 0 || strcmp(str, "fini") == 0) {
		destructor = 1;
		return 1;
	}
	if (strcmp(str, "section") == 0 && a2 != NULL) {
		nextsect = section2string(a2, strlen(a2));
		return 1;
	}
	if (strcmp(str, "alias") == 0 && a2 != NULL) {
		alias = tmpstrdup(a2);
		return 1;
	}

	return 0;
}

/*
 * Called when a identifier has been declared.
 */
void
fixdef(struct symtab *sp)
{
	struct attr *ga;

	/* may have sanity checks here */
	if (gottls)
		sp->sflags |= STLS;
	gottls = 0;

#ifdef HAVE_WEAKREF
	/* not many as'es have this directive */
	if ((ga = gcc_get_attr(sp->sap, GCC_ATYP_WEAKREF)) != NULL) {
		char *wr = ga->a1.sarg;
		char *sn = sp->soname ? sp->soname : sp->sname;
		if (wr == NULL) {
			if ((ga = gcc_get_attr(sp->sap, GCC_ATYP_ALIAS))) {
				wr = ga->a1.sarg;
			}
		}
		if (wr == NULL)
			printf("\t.weak %s\n", sn);
		else
			printf("\t.weakref %s,%s\n", sn, wr);
	} else
#endif
	       if ((ga = attr_find(sp->sap, GCC_ATYP_ALIAS)) != NULL) {
		char *an = ga->sarg(0);
		char *sn = sp->soname ? sp->soname : sp->sname;
		char *v;

		v = attr_find(sp->sap, GCC_ATYP_WEAK) ? "weak" : "globl";
		printf("\t.%s %s\n", v, sn);
		printf("\t.set %s,%s\n", sn, an);
	}
	if (alias != NULL && (sp->sclass != PARAM)) {
		printf("\t.globl %s\n", exname(sp->soname));
		printf("%s = ", exname(sp->soname));
		printf("%s\n", exname(alias));
		alias = NULL;
	}
	if ((constructor || destructor) && (sp->sclass != PARAM)) {
		NODE *p = talloc();

		p->n_op = NAME;
		p->n_sp =
		  (struct symtab *)(constructor ? "constructor" : "destructor");
		sp->sap = attr_add(sp->sap, gcc_attr_parse(p));
		constructor = destructor = 0;
	}
}

NODE *
i386_builtin_return_address(NODE *f, NODE *a, TWORD t)
{
	int nframes;

	if (a == NULL || a->n_op != ICON)
		goto bad;

	nframes = a->n_lval;

	tfree(f);
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
i386_builtin_frame_address(NODE *f, NODE *a, TWORD t)
{
	int nframes;

	if (a == NULL || a->n_op != ICON)
		goto bad;

	nframes = a->n_lval;

	tfree(f);
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

void
pass1_lastchance(struct interpass *ip)
{
}
