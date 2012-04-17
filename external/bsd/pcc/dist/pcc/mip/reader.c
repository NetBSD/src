/*	Id: reader.c,v 1.276 2011/08/12 19:24:40 plunky Exp 	*/	
/*	$NetBSD: reader.c,v 1.1.1.4.2.1 2012/04/17 00:04:06 yamt Exp $	*/
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
 * Everything is entered via pass2_compile().  No functions are 
 * allowed to recurse back into pass2_compile().
 */

# include "pass2.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

/*	some storage declarations */
int nrecur;
int thisline;
int fregs;
int p2autooff, p2maxautooff;

NODE *nodepole;
FILE *prfil;
struct interpass prepole;

void saveip(struct interpass *ip);
void deltemp(NODE *p, void *);
static void cvtemps(struct interpass *ipole, int op, int off);
NODE *store(NODE *);
static void fixxasm(struct p2env *);

static void gencode(NODE *p, int cookie);
static void genxasm(NODE *p);
static void afree(void);

struct p2env p2env;

int
getlab2(void)
{
	extern int getlab(void);
	int rv = getlab();
#ifdef PCC_DEBUG
	if (p2env.epp->ip_lblnum != rv)
		comperr("getlab2 error: %d != %d", p2env.epp->ip_lblnum, rv);
#endif
	p2env.epp->ip_lblnum++;
	return rv;
}

#ifdef PCC_DEBUG
static int *lbldef, *lbluse;
static void
cktree(NODE *p, void *arg)
{
	int i;

	if (p->n_op > MAXOP)
		cerror("%p) op %d slipped through", p, p->n_op);
#ifndef FIELDOPS
	if (p->n_op == FLD)
		cerror("%p) FLD slipped through", p);
#endif
	if (BTYPE(p->n_type) > MAXTYPES)
		cerror("%p) type %x slipped through", p, p->n_type);
	if (p->n_op == CBRANCH) {
		 if (!logop(p->n_left->n_op))
			cerror("%p) not logop branch", p);
		i = (int)p->n_right->n_lval;
		if (i < p2env.ipp->ip_lblnum || i >= p2env.epp->ip_lblnum)
			cerror("%p) label %d outside boundaries %d-%d",
			    p, i, p2env.ipp->ip_lblnum, p2env.epp->ip_lblnum);
		lbluse[i-p2env.ipp->ip_lblnum] = 1;
	}
	if ((dope[p->n_op] & ASGOPFLG) && p->n_op != RETURN)
		cerror("%p) asgop %d slipped through", p, p->n_op);
	if (p->n_op == TEMP &&
	    (regno(p) < p2env.ipp->ip_tmpnum || regno(p) >= p2env.epp->ip_tmpnum))
		cerror("%p) temporary %d outside boundaries %d-%d",
		    p, regno(p), p2env.ipp->ip_tmpnum, p2env.epp->ip_tmpnum);
	if (p->n_op == GOTO && p->n_left->n_op == ICON) {
		i = (int)p->n_left->n_lval;
		if (i < p2env.ipp->ip_lblnum || i >= p2env.epp->ip_lblnum)
			cerror("%p) label %d outside boundaries %d-%d",
			    p, i, p2env.ipp->ip_lblnum, p2env.epp->ip_lblnum);
		lbluse[i-p2env.ipp->ip_lblnum] = 1;
	}
}

/*
 * Check that the trees are in a suitable state for pass2.
 */
static void
sanitychecks(struct p2env *p2e)
{
	struct interpass *ip;
	int i;
#ifdef notyet
	TMPMARK();
#endif
	lbldef = tmpcalloc(sizeof(int) * (p2e->epp->ip_lblnum - p2e->ipp->ip_lblnum));
	lbluse = tmpcalloc(sizeof(int) * (p2e->epp->ip_lblnum - p2e->ipp->ip_lblnum));

	DLIST_FOREACH(ip, &p2env.ipole, qelem) {
		if (ip->type == IP_DEFLAB) {
			i = ip->ip_lbl;
			if (i < p2e->ipp->ip_lblnum || i >= p2e->epp->ip_lblnum)
				cerror("label %d outside boundaries %d-%d",
				    i, p2e->ipp->ip_lblnum, p2e->epp->ip_lblnum);
			lbldef[i-p2e->ipp->ip_lblnum] = 1;
		}
		if (ip->type == IP_NODE)
			walkf(ip->ip_node, cktree, 0);
	}
	for (i = 0; i < (p2e->epp->ip_lblnum - p2e->ipp->ip_lblnum); i++)
		if (lbluse[i] != 0 && lbldef[i] == 0)
			cerror("internal label %d not defined",
			    i + p2e->ipp->ip_lblnum);

#ifdef notyet
	TMPFREE();
#endif
}
#endif

/*
 * Look if a temporary comes from a on-stack argument, in that case
 * use the already existing stack position instead of moving it to
 * a new place, and remove the move-to-temp statement.
 */
static int
stkarg(int tnr, int (*soff)[2])
{
	struct p2env *p2e = &p2env;
	struct interpass *ip;
	NODE *p;

	ip = DLIST_NEXT((struct interpass *)p2e->ipp, qelem);
	while (ip->type != IP_DEFLAB) /* search for first DEFLAB */
		ip = DLIST_NEXT(ip, qelem);

	ip = DLIST_NEXT(ip, qelem); /* first NODE */

	for (; ip->type != IP_DEFLAB; ip = DLIST_NEXT(ip, qelem)) {
		if (ip->type != IP_NODE)
			continue;

		p = ip->ip_node;
		if (p->n_op == XASM)
			continue; /* XXX - hack for x86 PIC */
#ifdef notdef
		if (p->n_op != ASSIGN || p->n_left->n_op != TEMP)
			comperr("temparg");
#endif
		if (p->n_op != ASSIGN || p->n_left->n_op != TEMP)
			continue; /* unknown tree */

		if (p->n_right->n_op != OREG && p->n_right->n_op != UMUL)
			continue; /* arg in register */
		if (tnr != regno(p->n_left))
			continue; /* wrong assign */
		p = p->n_right;
		if (p->n_op == UMUL &&
		    p->n_left->n_op == PLUS &&
		    p->n_left->n_left->n_op == REG &&
		    p->n_left->n_right->n_op == ICON) {
			soff[0][0] = regno(p->n_left->n_left);
			soff[0][1] = (int)p->n_left->n_right->n_lval;
		} else if (p->n_op == OREG) {
			soff[0][0] = regno(p);
			soff[0][1] = (int)p->n_lval;
		} else
			comperr("stkarg: bad arg");
		tfree(ip->ip_node);
		DLIST_REMOVE(ip, qelem);
		return 1;
	}
	return 0;
}

/*
 * See if an ADDROF is somewhere inside the expression tree.
 * If so, fill in the offset table.
 */
static void
findaof(NODE *p, void *arg)
{
	int (*aof)[2] = arg;
	int tnr;

	if (p->n_op != ADDROF || p->n_left->n_op != TEMP)
		return;
	tnr = regno(p->n_left);
	if (aof[tnr][0])
		return; /* already gotten stack address */
	if (stkarg(tnr, &aof[tnr]))
		return;	/* argument was on stack */
	aof[tnr][0] = FPREG;
	aof[tnr][1] = BITOOR(freetemp(szty(p->n_left->n_type)));
}

/*
 * Check if a node has side effects.
 */
static int
isuseless(NODE *n)
{
	switch (n->n_op) {
	case XASM:
	case FUNARG:
	case UCALL:
	case UFORTCALL:
	case FORCE:
	case ASSIGN:
	case CALL:
	case FORTCALL:
	case CBRANCH:
	case RETURN:
	case GOTO:
	case STCALL:
	case USTCALL:
	case STASG:
	case STARG:
		return 0;
	default:
		return 1;
	}
}

/*
 * Delete statements with no meaning (like a+b; or 513.4;)
 */
NODE *
deluseless(NODE *p)
{
	struct interpass *ip;
	NODE *l, *r;

	if (optype(p->n_op) == LTYPE) {
		nfree(p);
		return NULL;
	}
	if (isuseless(p) == 0)
		return p;

	if (optype(p->n_op) == UTYPE) {
		l = p->n_left;
		nfree(p);
		return deluseless(l);
	}

	/* Be sure that both leaves may be valid */
	l = deluseless(p->n_left);
	r = deluseless(p->n_right);
	nfree(p);
	if (l && r) {
		ip = ipnode(l);
		DLIST_INSERT_AFTER(&prepole, ip, qelem);
		return r;
	} else if (l)
		return l;
	else if (r)
		return r;
	return NULL;
}

/*
 * Receives interpass structs from pass1.
 */
void
pass2_compile(struct interpass *ip)
{
	void deljumps(struct p2env *);
	struct p2env *p2e = &p2env;
	int (*addrp)[2];
	MARK mark;

	if (ip->type == IP_PROLOG) {
		memset(p2e, 0, sizeof(struct p2env));
		p2e->ipp = (struct interpass_prolog *)ip;
		DLIST_INIT(&p2e->ipole, qelem);
	}
	DLIST_INSERT_BEFORE(&p2e->ipole, ip, qelem);
	if (ip->type != IP_EPILOG)
		return;

#ifdef PCC_DEBUG
	if (e2debug) {
		printf("Entering pass2\n");
		printip(&p2e->ipole);
	}
#endif

	afree();
	p2e->epp = (struct interpass_prolog *)DLIST_PREV(&p2e->ipole, qelem);
	p2maxautooff = p2autooff = p2e->epp->ipp_autos;

#ifdef PCC_DEBUG
	sanitychecks(p2e);
#endif
	myreader(&p2e->ipole); /* local massage of input */

	/*
	 * Do initial modification of the trees.  Two loops;
	 * - first, search for ADDROF of TEMPs, these must be
	 *   converterd to OREGs on stack.
	 * - second, do the actual conversions, in case of not xtemps
	 *   convert all temporaries to stack references.
	 */
	markset(&mark);
	if (p2e->epp->ip_tmpnum != p2e->ipp->ip_tmpnum) {
		addrp = tmpcalloc(sizeof(*addrp) *
		    (p2e->epp->ip_tmpnum - p2e->ipp->ip_tmpnum));
		addrp -= p2e->ipp->ip_tmpnum;
	} else
		addrp = NULL;
	if (xtemps) {
		DLIST_FOREACH(ip, &p2e->ipole, qelem) {
			if (ip->type == IP_NODE)
				walkf(ip->ip_node, findaof, addrp);
		}
	}
	DLIST_FOREACH(ip, &p2e->ipole, qelem)
		if (ip->type == IP_NODE)
			walkf(ip->ip_node, deltemp, addrp);
	markfree(&mark);

#ifdef PCC_DEBUG
	if (e2debug) {
		printf("Efter ADDROF/TEMP\n");
		printip(&p2e->ipole);
	}
#endif

	DLIST_INIT(&prepole, qelem);
	DLIST_FOREACH(ip, &p2e->ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		canon(ip->ip_node);
		if ((ip->ip_node = deluseless(ip->ip_node)) == NULL) {
			DLIST_REMOVE(ip, qelem);
		} else while (!DLIST_ISEMPTY(&prepole, qelem)) {
			struct interpass *tipp;

			tipp = DLIST_NEXT(&prepole, qelem);
			DLIST_REMOVE(tipp, qelem);
			DLIST_INSERT_BEFORE(ip, tipp, qelem);
		}
	}

	fixxasm(p2e); /* setup for extended asm */

	optimize(p2e);
	ngenregs(p2e);

	if (xssa && xtemps && xdeljumps)
		deljumps(p2e);

	DLIST_FOREACH(ip, &p2e->ipole, qelem)
		emit(ip);
}

void
emit(struct interpass *ip)
{
	NODE *p, *r;
	struct optab *op;
	int o;

	switch (ip->type) {
	case IP_NODE:
		p = ip->ip_node;

		nodepole = p;
		canon(p); /* may convert stuff after genregs */
		if (c2debug > 1) {
			printf("emit IP_NODE:\n");
			fwalk(p, e2print, 0);
		}
		switch (p->n_op) {
		case CBRANCH:
			/* Only emit branch insn if RESCC */
			/* careful when an OPLOG has been elided */
			if (p->n_left->n_su == 0 && p->n_left->n_left != NULL) {
				op = &table[TBLIDX(p->n_left->n_left->n_su)];
				r = p->n_left;
			} else {
				op = &table[TBLIDX(p->n_left->n_su)];
				r = p;
			}
			if (op->rewrite & RESCC) {
				o = p->n_left->n_op;
				gencode(r, FORCC);
				cbgen(o, p->n_right->n_lval);
			} else {
				gencode(r, FORCC);
			}
			break;
		case FORCE:
			gencode(p->n_left, INREGS);
			break;
		case XASM:
			genxasm(p);
			break;
		default:
			if (p->n_op != REG || p->n_type != VOID) /* XXX */
				gencode(p, FOREFF); /* Emit instructions */
		}

		tfree(p);
		break;
	case IP_PROLOG:
		prologue((struct interpass_prolog *)ip);
		break;
	case IP_EPILOG:
		eoftn((struct interpass_prolog *)ip);
		p2maxautooff = p2autooff = AUTOINIT/SZCHAR;
		break;
	case IP_DEFLAB:
		deflab(ip->ip_lbl);
		break;
	case IP_ASM:
		printf("%s", ip->ip_asm);
		break;
	default:
		cerror("emit %d", ip->type);
	}
}

#ifdef PCC_DEBUG
char *cnames[] = {
	"SANY",
	"SAREG",
	"SBREG",
	"SCREG",
	"SDREG",
	"SCC",
	"SNAME",
	"SCON",
	"SFLD",
	"SOREG",
	"STARNM",
	"STARREG",
	"INTEMP",
	"FORARG",
	"SWADD",
	0,
};

/*
 * print a nice-looking description of cookie
 */
char *
prcook(int cookie)
{
	static char buf[50];
	int i, flag;

	if (cookie & SPECIAL) {
		switch (cookie) {
		case SZERO:
			return "SZERO";
		case SONE:
			return "SONE";
		case SMONE:
			return "SMONE";
		default:
			snprintf(buf, sizeof(buf), "SPECIAL+%d", cookie & ~SPECIAL);
			return buf;
		}
	}

	flag = 0;
	buf[0] = 0;
	for (i = 0; cnames[i]; ++i) {
		if (cookie & (1<<i)) {
			if (flag)
				strlcat(buf, "|", sizeof(buf));
			++flag;
			strlcat(buf, cnames[i], sizeof(buf));
		}
	}
	return buf;
}
#endif

int
geninsn(NODE *p, int cookie)
{
	NODE *p1, *p2;
	int q, o, rv = 0;

#ifdef PCC_DEBUG
	if (o2debug) {
		printf("geninsn(%p, %s)\n", p, prcook(cookie));
		fwalk(p, e2print, 0);
	}
#endif

	q = cookie & QUIET;
	cookie &= ~QUIET; /* XXX - should not be necessary */

again:	switch (o = p->n_op) {
	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
	case ULE:
	case ULT:
	case UGE:
	case UGT:
		p1 = p->n_left;
		p2 = p->n_right;
		if (p2->n_op == ICON && p2->n_lval == 0 && *p2->n_name == 0 &&
		    (dope[p1->n_op] & (FLOFLG|DIVFLG|SIMPFLG|SHFFLG))) {
#ifdef mach_pdp11 /* XXX all targets? */
			if ((rv = geninsn(p1, FORCC|QUIET)) != FFAIL)
				break;
#else
			if (findops(p1, FORCC) > 0)
				break;
#endif
		}
		rv = relops(p);
		break;

	case PLUS:
	case MINUS:
	case MUL:
	case DIV:
	case MOD:
	case AND:
	case OR:
	case ER:
	case LS:
	case RS:
		rv = findops(p, cookie);
		break;

	case ASSIGN:
#ifdef FINDMOPS
		if ((rv = findmops(p, cookie)) != FFAIL)
			break;
		/* FALLTHROUGH */
#endif
	case STASG:
		rv = findasg(p, cookie);
		break;

	case UMUL: /* May turn into an OREG */
		rv = findumul(p, cookie);
		break;

	case REG:
	case TEMP:
	case NAME:
	case ICON:
	case FCON:
	case OREG:
		rv = findleaf(p, cookie);
		break;

	case STCALL:
	case CALL:
		/* CALL arguments are handled special */
		for (p1 = p->n_right; p1->n_op == CM; p1 = p1->n_left)
			(void)geninsn(p1->n_right, FOREFF);
		(void)geninsn(p1, FOREFF);
		/* FALLTHROUGH */
	case FLD:
	case COMPL:
	case UMINUS:
	case PCONV:
	case SCONV:
/*	case INIT: */
	case GOTO:
	case FUNARG:
	case STARG:
	case UCALL:
	case USTCALL:
	case ADDROF:
		rv = finduni(p, cookie);
		break;

	case CBRANCH:
		p1 = p->n_left;
		p2 = p->n_right;
		p1->n_label = (int)p2->n_lval;
		(void)geninsn(p1, FORCC);
		p->n_su = 0;
		break;

	case FORCE: /* XXX needed? */
		(void)geninsn(p->n_left, INREGS);
		p->n_su = 0; /* su calculations traverse left */
		break;

	case XASM:
		for (p1 = p->n_left; p1->n_op == CM; p1 = p1->n_left)
			(void)geninsn(p1->n_right, FOREFF);
		(void)geninsn(p1, FOREFF);
		break;	/* all stuff already done? */

	case XARG:
		/* generate code for correct class here */
#if 0
		geninsn(p->n_left, 1 << p->n_label);
#endif
		break;

	default:
		comperr("geninsn: bad op %s, node %p", opst[o], p);
	}
	if (rv == FFAIL && !q)
		comperr("Cannot generate code, node %p op %s", p,opst[p->n_op]);
	if (rv == FRETRY)
		goto again;
#ifdef PCC_DEBUG
	if (o2debug) {
		printf("geninsn(%p, %s) rv %d\n", p, prcook(cookie), rv);
		fwalk(p, e2print, 0);
	}
#endif
	return rv;
}

/*
 * Store a given subtree in a temporary location.
 * Return an OREG node where it is located.
 */
NODE *
store(NODE *p)
{
	extern struct interpass *storesave;
	struct interpass *ip;
	NODE *q, *r;
	int s;

	s = BITOOR(freetemp(szty(p->n_type)));
	q = mklnode(OREG, s, FPREG, p->n_type);
	r = mklnode(OREG, s, FPREG, p->n_type);
	ip = ipnode(mkbinode(ASSIGN, q, p, p->n_type));

	storesave = ip;
	return r;
}

#ifdef PCC_DEBUG
#define	CDEBUG(x) if (c2debug) printf x
#else
#define	CDEBUG(x)
#endif

/*
 * Do a register-register move if necessary.
 * Called if a RLEFT or RRIGHT is found.
 */
static void
ckmove(NODE *p, NODE *q)
{
	struct optab *t = &table[TBLIDX(p->n_su)];
	int reg;

	if (q->n_op != REG || p->n_reg == -1)
		return; /* no register */

	/* do we have a need for special reg? */
	if ((t->needs & NSPECIAL) &&
	    (reg = rspecial(t, p->n_left == q ? NLEFT : NRIGHT)) >= 0)
		;
	else
		reg = DECRA(p->n_reg, 0);

	if (reg < 0 || reg == DECRA(q->n_reg, 0))
		return; /* no move necessary */

	CDEBUG(("rmove: node %p, %s -> %s\n", p, rnames[DECRA(q->n_reg, 0)],
	    rnames[reg]));
	rmove(DECRA(q->n_reg, 0), reg, p->n_type);
	q->n_reg = q->n_rval = reg;
}

/*
 * Rewrite node to register after instruction emit.
 */
static void
rewrite(NODE *p, int dorewrite, int cookie)
{
	NODE *l, *r;
	int o;

	l = getlr(p, 'L');
	r = getlr(p, 'R');
	o = p->n_op;
	p->n_op = REG;
	p->n_lval = 0;
	p->n_name = "";

	if (o == ASSIGN || o == STASG) {
		/* special rewrite care */
		int reg = DECRA(p->n_reg, 0);
#define	TL(x) (TBLIDX(x->n_su) || x->n_op == REG)
		if (p->n_reg == -1)
			;
		else if (TL(l) && (DECRA(l->n_reg, 0) == reg))
			;
		else if (TL(r) && (DECRA(r->n_reg, 0) == reg))
			;
		else if (TL(l))
			rmove(DECRA(l->n_reg, 0), reg, p->n_type);
		else if (TL(r))
			rmove(DECRA(r->n_reg, 0), reg, p->n_type);
#if 0
		else
			comperr("rewrite");
#endif
#undef TL
	}
	if (optype(o) != LTYPE)
		tfree(l);
	if (optype(o) == BITYPE)
		tfree(r);
	if (dorewrite == 0)
		return;
	CDEBUG(("rewrite: %p, reg %s\n", p,
	    p->n_reg == -1? "<none>" : rnames[DECRA(p->n_reg, 0)]));
	p->n_rval = DECRA(p->n_reg, 0);
}

#ifndef XASM_TARGARG
#define	XASM_TARGARG(x,y) 0
#endif

/*
 * printout extended assembler.
 */
void
genxasm(NODE *p)
{
	NODE *q, **nary;
	int n = 1, o = 0;
	char *w;

	if (p->n_left->n_op != ICON || p->n_left->n_type != STRTY) {
		for (q = p->n_left; q->n_op == CM; q = q->n_left)
			n++;
		nary = tmpcalloc(sizeof(NODE *)*(n+1));
		o = n;
		for (q = p->n_left; q->n_op == CM; q = q->n_left) {
			gencode(q->n_right->n_left, INREGS);
			nary[--o] = q->n_right;
		}
		gencode(q->n_left, INREGS);
		nary[--o] = q;
	} else
		nary = 0;

	w = p->n_name;
	putchar('\t');
	while (*w != 0) {
		if (*w == '%') {
			if (w[1] == '%')
				putchar('%');
			else if (XASM_TARGARG(w, nary))
				; /* handled by target */
			else if (w[1] < '0' || w[1] > (n + '0'))
				uerror("bad xasm arg number %c", w[1]);
			else {
				if (w[1] == (n + '0'))
					q = nary[(int)w[1]-'0' - 1]; /* XXX */
				else
					q = nary[(int)w[1]-'0'];
				adrput(stdout, q->n_left);
			}
			w++;
		} else if (*w == '\\') { /* Always 3-digit octal */
			int num = *++w - '0';
			num = (num << 3) + *++w - '0';
			num = (num << 3) + *++w - '0';
			putchar(num);
		} else
			putchar(*w);
		w++;
	}
	putchar('\n');
}

/*
 * Allocate temporary registers for use while emitting this table entry.
 */
static void
allo(NODE *p, struct optab *q)
{
	extern int stktemp;
	int i, n = ncnt(q->needs);

	for (i = 0; i < NRESC; i++)
		if (resc[i].n_op != FREE)
			comperr("allo: used reg");
	if (n == 0 && (q->needs & NTMASK) == 0)
		return;
	for (i = 0; i < n+1; i++) {
		resc[i].n_op = REG;
		resc[i].n_type = p->n_type; /* XXX should be correct type */
		resc[i].n_rval = DECRA(p->n_reg, i);
		resc[i].n_su = p->n_su; /* ??? */
	}
	if (i > NRESC)
		comperr("allo: too many allocs");
	if (q->needs & NTMASK) {
		resc[i].n_op = OREG;
		resc[i].n_lval = stktemp;
		resc[i].n_rval = FPREG;
		resc[i].n_su = p->n_su; /* ??? */
		resc[i].n_name = "";
	}
}

static void
afree(void)
{
	int i;

	for (i = 0; i < NRESC; i++)
		resc[i].n_op = FREE;
}

void
gencode(NODE *p, int cookie)
{
	struct optab *q = &table[TBLIDX(p->n_su)];
	NODE *p1, *l, *r;
	int o = optype(p->n_op);
#ifdef FINDMOPS
	int ismops = (p->n_op == ASSIGN && (p->n_flags & 1));
#endif

	l = p->n_left;
	r = p->n_right;

	if (TBLIDX(p->n_su) == 0) {
		if (o == BITYPE && (p->n_su & DORIGHT))
			gencode(r, 0);
		if (optype(p->n_op) != LTYPE)
			gencode(l, 0);
		if (o == BITYPE && !(p->n_su & DORIGHT))
			gencode(r, 0);
		return;
	}

	CDEBUG(("gencode: node %p\n", p));

	if (p->n_op == REG && DECRA(p->n_reg, 0) == p->n_rval)
		return; /* meaningless move to itself */

	if (callop(p->n_op))
		lastcall(p); /* last chance before function args */
	if (p->n_op == CALL || p->n_op == FORTCALL || p->n_op == STCALL) {
		/* Print out arguments first */
		for (p1 = r; p1->n_op == CM; p1 = p1->n_left)
			gencode(p1->n_right, FOREFF);
		gencode(p1, FOREFF);
		o = UTYPE; /* avoid going down again */
	}

	if (o == BITYPE && (p->n_su & DORIGHT)) {
		gencode(r, INREGS);
		if (q->rewrite & RRIGHT)
			ckmove(p, r);
	}
	if (o != LTYPE) {
		gencode(l, INREGS);
#ifdef FINDMOPS
		if (ismops)
			;
		else
#endif
		     if (q->rewrite & RLEFT)
			ckmove(p, l);
	}
	if (o == BITYPE && !(p->n_su & DORIGHT)) {
		gencode(r, INREGS);
		if (q->rewrite & RRIGHT)
			ckmove(p, r);
	}

#ifdef FINDMOPS
	if (ismops) {
		/* reduce right tree to make expand() work */
		if (optype(r->n_op) != LTYPE) {
			p->n_op = r->n_op;
			r = tcopy(r->n_right);
			tfree(p->n_right);
			p->n_right = r;
		}
	}
#endif

	canon(p);

	if (q->needs & NSPECIAL) {
		int rr = rspecial(q, NRIGHT);
		int lr = rspecial(q, NLEFT);

		if (rr >= 0) {
#ifdef PCC_DEBUG
			if (optype(p->n_op) != BITYPE)
				comperr("gencode: rspecial borked");
#endif
			if (r->n_op != REG)
				comperr("gencode: rop != REG");
			if (rr != r->n_rval)
				rmove(r->n_rval, rr, r->n_type);
			r->n_rval = r->n_reg = rr;
		}
		if (lr >= 0) {
			if (l->n_op != REG)
				comperr("gencode: %p lop != REG", p);
			if (lr != l->n_rval)
				rmove(l->n_rval, lr, l->n_type);
			l->n_rval = l->n_reg = lr;
		}
		if (rr >= 0 && lr >= 0 && (l->n_reg == rr || r->n_reg == lr))
			comperr("gencode: cross-reg-move");
	}

	if (p->n_op == ASSIGN &&
	    p->n_left->n_op == REG && p->n_right->n_op == REG &&
	    p->n_left->n_rval == p->n_right->n_rval &&
	    (p->n_su & RVCC) == 0) { /* XXX should check if necessary */
		/* do not emit anything */
		CDEBUG(("gencode(%p) assign nothing\n", p));
		rewrite(p, q->rewrite, cookie);
		return;
	}

	CDEBUG(("emitting node %p\n", p));
	if (TBLIDX(p->n_su) == 0)
		return;

	allo(p, q);
	expand(p, cookie, q->cstring);

#ifdef FINDMOPS
	if (ismops && DECRA(p->n_reg, 0) != regno(l) && cookie != FOREFF) {
		CDEBUG(("gencode(%p) rmove\n", p));
		rmove(regno(l), DECRA(p->n_reg, 0), p->n_type);
	} else
#endif
	if (callop(p->n_op) && cookie != FOREFF &&
	    DECRA(p->n_reg, 0) != RETREG(p->n_type)) {
		CDEBUG(("gencode(%p) retreg\n", p));
		rmove(RETREG(p->n_type), DECRA(p->n_reg, 0), p->n_type);
	} else if (q->needs & NSPECIAL) {
		int rr = rspecial(q, NRES);

		if (rr >= 0 && DECRA(p->n_reg, 0) != rr) {
			CDEBUG(("gencode(%p) nspec retreg\n", p));
			rmove(rr, DECRA(p->n_reg, 0), p->n_type);
		}
	} else if ((q->rewrite & RESC1) &&
	    (DECRA(p->n_reg, 1) != DECRA(p->n_reg, 0))) {
		CDEBUG(("gencode(%p) RESC1 retreg\n", p));
		rmove(DECRA(p->n_reg, 1), DECRA(p->n_reg, 0), p->n_type);
	}
#if 0
		/* XXX - kolla upp det h{r */
	   else if (p->n_op == ASSIGN) {
		/* may need move added if RLEFT/RRIGHT */
		/* XXX should be handled in sucomp() */
		if ((q->rewrite & RLEFT) && (p->n_left->n_op == REG) &&
		    (p->n_left->n_rval != DECRA(p->n_reg, 0)) &&
		    TCLASS(p->n_su)) {
			rmove(p->n_left->n_rval, DECRA(p->n_reg, 0), p->n_type);
		} else if ((q->rewrite & RRIGHT) && (p->n_right->n_op == REG) &&
		    (p->n_right->n_rval != DECRA(p->n_reg, 0)) &&
		    TCLASS(p->n_su)) {
			rmove(p->n_right->n_rval, DECRA(p->n_reg, 0), p->n_type);
		}
	}
#endif
	rewrite(p, q->rewrite, cookie);
	afree();
}

int negrel[] = { NE, EQ, GT, GE, LT, LE, UGT, UGE, ULT, ULE } ;  /* negatives of relationals */
size_t negrelsize = sizeof negrel / sizeof negrel[0];

#ifdef PCC_DEBUG
#undef	PRTABLE
void
e2print(NODE *p, int down, int *a, int *b)
{
#ifdef PRTABLE
	extern int tablesize;
#endif

	prfil = stdout;
	*a = *b = down+1;
	while( down >= 2 ){
		fprintf(prfil, "\t");
		down -= 2;
		}
	if( down-- ) fprintf(prfil, "    " );


	fprintf(prfil, "%p) %s", p, opst[p->n_op] );
	switch( p->n_op ) { /* special cases */

	case FLD:
		fprintf(prfil, " sz=%d, shift=%d",
		    UPKFSZ(p->n_rval), UPKFOFF(p->n_rval));
		break;

	case REG:
		fprintf(prfil, " %s", rnames[p->n_rval] );
		break;

	case TEMP:
		fprintf(prfil, " %d", regno(p));
		break;

	case XASM:
	case XARG:
		fprintf(prfil, " '%s'", p->n_name);
		break;

	case ICON:
	case NAME:
	case OREG:
		fprintf(prfil, " " );
		adrput(prfil, p );
		break;

	case STCALL:
	case USTCALL:
	case STARG:
	case STASG:
		fprintf(prfil, " size=%d", p->n_stsize );
		fprintf(prfil, " align=%d", p->n_stalign );
		break;
		}

	fprintf(prfil, ", " );
	tprint(prfil, p->n_type, p->n_qual);
	fprintf(prfil, ", " );

	prtreg(prfil, p);
	fprintf(prfil, ", SU= %d(%cREG,%s,%s,%s,%s,%s,%s)\n",
	    TBLIDX(p->n_su), 
	    TCLASS(p->n_su)+'@',
#ifdef PRTABLE
	    TBLIDX(p->n_su) >= 0 && TBLIDX(p->n_su) <= tablesize ?
	    table[TBLIDX(p->n_su)].cstring : "",
#else
	    "",
#endif
	    p->n_su & LREG ? "LREG" : "", p->n_su & RREG ? "RREG" : "",
	    p->n_su & RVEFF ? "RVEFF" : "", p->n_su & RVCC ? "RVCC" : "",
	    p->n_su & DORIGHT ? "DORIGHT" : "");
}
#endif

/*
 * change left TEMPs into OREGs
 */
void
deltemp(NODE *p, void *arg)
{
	int (*aor)[2] = arg;
	NODE *l, *r;

	if (p->n_op == TEMP) {
		if (aor[regno(p)][0] == 0) {
			if (xtemps)
				return;
			aor[regno(p)][0] = FPREG;
			aor[regno(p)][1] = BITOOR(freetemp(szty(p->n_type)));
		}
		l = mklnode(REG, 0, aor[regno(p)][0], INCREF(p->n_type));
		r = mklnode(ICON, aor[regno(p)][1], 0, INT);
		p->n_left = mkbinode(PLUS, l, r, INCREF(p->n_type));
		p->n_op = UMUL;
	} else if (p->n_op == ADDROF && p->n_left->n_op == OREG) {
		p->n_op = PLUS;
		l = p->n_left;
		l->n_op = REG;
		l->n_type = INCREF(l->n_type);
		p->n_right = mklnode(ICON, l->n_lval, 0, INT);
	} else if (p->n_op == ADDROF && p->n_left->n_op == UMUL) {
		l = p->n_left;
		*p = *p->n_left->n_left;
		nfree(l->n_left);
		nfree(l);
	}
}

/*
 * for pointer/integer arithmetic, set pointer at left node
 */
static void
setleft(NODE *p, void *arg)
{        
	NODE *q;

	/* only additions for now */
	if (p->n_op != PLUS)
		return;
	if (ISPTR(p->n_right->n_type) && !ISPTR(p->n_left->n_type)) {
		q = p->n_right;
		p->n_right = p->n_left;
		p->n_left = q;
	}
}

/* It is OK to have these as externals */
static int oregr;
static CONSZ oregtemp;
static char *oregcp;
/*
 * look for situations where we can turn * into OREG
 * If sharp then do not allow temps.
 */
int
oregok(NODE *p, int sharp)
{

	NODE *q;
	NODE *ql, *qr;
	int r;
	CONSZ temp;
	char *cp;

	q = p->n_left;
#if 0
	if ((q->n_op == REG || (q->n_op == TEMP && !sharp)) &&
	    q->n_rval == DECRA(q->n_reg, 0)) {
#endif
	if (q->n_op == REG || (q->n_op == TEMP && !sharp)) {
		temp = q->n_lval;
		r = q->n_rval;
		cp = q->n_name;
		goto ormake;
	}

	if (q->n_op != PLUS && q->n_op != MINUS)
		return 0;
	ql = q->n_left;
	qr = q->n_right;

#ifdef R2REGS

	/* look for doubly indexed expressions */
	/* XXX - fix checks */

	if( q->n_op == PLUS) {
		int i;
		if( (r=base(ql))>=0 && (i=offset(qr, tlen(p)))>=0) {
			makeor2(p, ql, r, i);
			return 1;
		} else if((r=base(qr))>=0 && (i=offset(ql, tlen(p)))>=0) {
			makeor2(p, qr, r, i);
			return 1;
		}
	}


#endif

#if 0
	if( (q->n_op==PLUS || q->n_op==MINUS) && qr->n_op == ICON &&
			(ql->n_op==REG || (ql->n_op==TEMP && !sharp)) &&
			szty(qr->n_type)==1 &&
			(ql->n_rval == DECRA(ql->n_reg, 0) ||
			/* XXX */
			 ql->n_rval == FPREG || ql->n_rval == STKREG)) {
#endif
	if ((q->n_op==PLUS || q->n_op==MINUS) && qr->n_op == ICON &&
	    (ql->n_op==REG || (ql->n_op==TEMP && !sharp))) {
	    
		temp = qr->n_lval;
		if( q->n_op == MINUS ) temp = -temp;
		r = ql->n_rval;
		temp += ql->n_lval;
		cp = qr->n_name;
		if( *cp && ( q->n_op == MINUS || *ql->n_name ) )
			return 0;
		if( !*cp ) cp = ql->n_name;

		ormake:
		if( notoff( p->n_type, r, temp, cp ))
			return 0;
		oregtemp = temp;
		oregr = r;
		oregcp = cp;
		return 1;
	}
	return 0;
}

static void
ormake(NODE *p)
{
	NODE *q = p->n_left;

	p->n_op = OREG;
	p->n_rval = oregr;
	p->n_lval = oregtemp;
	p->n_name = oregcp;
	tfree(q);
}

/*
 * look for situations where we can turn * into OREG
 */
void
oreg2(NODE *p, void *arg)
{
	if (p->n_op != UMUL)
		return;
	if (oregok(p, 1))
		ormake(p);
	if (p->n_op == UMUL)
		myormake(p);
}

void
canon(p) NODE *p; {
	/* put p in canonical form */

	walkf(p, setleft, 0);	/* ptrs at left node for arithmetic */
	walkf(p, oreg2, 0);	/* look for and create OREG nodes */
	mycanon(p);		/* your own canonicalization routine(s) */

}

void
comperr(char *str, ...)
{
	extern char *ftitle;
	va_list ap;

	if (nerrors) {
		fprintf(stderr,
		    "cannot recover from earlier errors: goodbye!\n");
		exit(1);
	}

	va_start(ap, str);
	fprintf(stderr, "%s, line %d: compiler error: ", ftitle, thisline);
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	prfil = stderr;

#ifdef PCC_DEBUG
	if (nodepole && nodepole->n_op != FREE)
		fwalk(nodepole, e2print, 0);
#endif
	exit(1);
}

/*
 * allocate k integers worth of temp space
 * we also make the convention that, if the number of words is
 * more than 1, it must be aligned for storing doubles...
 * Returns bits offset from base register.
 * XXX - redo this.
 */
int
freetemp(int k)
{
#ifndef BACKTEMP
	int t;

	if (k > 1) {
		SETOFF(p2autooff, ALDOUBLE/ALCHAR);
	} else {
		SETOFF(p2autooff, ALINT/ALCHAR);
	}

	t = p2autooff;
	p2autooff += k*(SZINT/SZCHAR);
	if (p2autooff > p2maxautooff)
		p2maxautooff = p2autooff;
	return (t);

#else
	p2autooff += k*(SZINT/SZCHAR);
	if (k > 1) {
		SETOFF(p2autooff, ALDOUBLE/ALCHAR);
	} else {
		SETOFF(p2autooff, ALINT/ALCHAR);
	}

	if (p2autooff > p2maxautooff)
		p2maxautooff = p2autooff;
	return( -p2autooff );
#endif
	}

NODE *
mklnode(int op, CONSZ lval, int rval, TWORD type)
{
	NODE *p = talloc();

	p->n_name = "";
	p->n_qual = 0;
	p->n_op = op;
	p->n_label = 0;
	p->n_lval = lval;
	p->n_rval = rval;
	p->n_type = type;
	p->n_regw = NULL;
	p->n_su = 0;
	return p;
}

NODE *
mkbinode(int op, NODE *left, NODE *right, TWORD type)
{
	NODE *p = talloc();

	p->n_name = "";
	p->n_qual = 0;
	p->n_op = op;
	p->n_label = 0;
	p->n_left = left;
	p->n_right = right;
	p->n_type = type;
	p->n_regw = NULL;
	p->n_su = 0;
	return p;
}

NODE *
mkunode(int op, NODE *left, int rval, TWORD type)
{
	NODE *p = talloc();

	p->n_name = "";
	p->n_qual = 0;
	p->n_op = op;
	p->n_label = 0;
	p->n_left = left;
	p->n_rval = rval;
	p->n_type = type;
	p->n_regw = NULL;
	p->n_su = 0;
	return p;
}

struct interpass *
ipnode(NODE *p)
{
	struct interpass *ip = tmpalloc(sizeof(struct interpass));

	ip->ip_node = p;
	ip->type = IP_NODE;
	ip->lineno = thisline;
	return ip;
}

int
rspecial(struct optab *q, int what)
{
	struct rspecial *r = nspecial(q);
	while (r->op) {
		if (r->op == what)
			return r->num;
		r++;
	}
	return -1;
}

#ifndef XASM_NUMCONV
#define	XASM_NUMCONV(x,y,z)	0
#endif

/*
 * change numeric argument redirections to the correct node type after 
 * cleaning up the other nodes.
 * be careful about input operands that may have different value than output.
 */
static void
delnums(NODE *p, void *arg)
{
	struct interpass *ip = arg, *ip2;
	NODE *r = ip->ip_node->n_left;
	NODE *q;
	TWORD t;
	int cnt, num;

	if (p->n_name[0] < '0' || p->n_name[0] > '9')
		return; /* not numeric */
	if ((q = listarg(r, p->n_name[0] - '0', &cnt)) == NIL)
		comperr("bad delnums");

	/* target may have opinions whether to do this conversion */
	if (XASM_NUMCONV(ip, p, q))
		return;

	/* Delete number by adding move-to/from-temp.  Later on */
	/* the temps may be rewritten to other LTYPEs */
	num = p2env.epp->ip_tmpnum++;

	/* pre node */
	t = p->n_left->n_type;
	r = mklnode(TEMP, 0, num, t);
	ip2 = ipnode(mkbinode(ASSIGN, tcopy(r), p->n_left, t));
	DLIST_INSERT_BEFORE(ip, ip2, qelem);
	p->n_left = r;

	/* post node */
	t = q->n_left->n_type;
	r = mklnode(TEMP, 0, num, t);
	ip2 = ipnode(mkbinode(ASSIGN, q->n_left, tcopy(r), t));
	DLIST_INSERT_AFTER(ip, ip2, qelem);
	q->n_left = r;

	p->n_name = tmpstrdup(q->n_name);
	if (*p->n_name == '=')
		p->n_name++;
}

/*
 * Ensure that a node is correct for the destination.
 */
static void
ltypify(NODE *p, void *arg)
{
	struct interpass *ip = arg;
	struct interpass *ip2;
	TWORD t = p->n_left->n_type;
	NODE *q, *r;
	int cw, ooff, ww;
	char *c;

again:
	if (myxasm(ip, p))
		return;	/* handled by target-specific code */

	cw = xasmcode(p->n_name);
	switch (ww = XASMVAL(cw)) {
	case 'p':
		/* pointer */
		/* just make register of it */
		p->n_name = tmpstrdup(p->n_name);
		c = strchr(p->n_name, XASMVAL(cw)); /* cannot fail */
		*c = 'r';
		/* FALLTHROUGH */
	case 'g':  /* general; any operand */
		if (ww == 'g' && p->n_left->n_op == ICON) {
			/* should only be input */
			p->n_name = "i";
			break;
		}
	case 'r': /* general reg */
		/* set register class */
		p->n_label = gclass(p->n_left->n_type);
		if (p->n_left->n_op == REG)
			break;
		q = p->n_left;
		r = (cw & XASMINOUT ? tcopy(q) : q);
		p->n_left = mklnode(TEMP, 0, p2env.epp->ip_tmpnum++, t);
		if ((cw & XASMASG) == 0) {
			ip2 = ipnode(mkbinode(ASSIGN, tcopy(p->n_left), r, t));
			DLIST_INSERT_BEFORE(ip, ip2, qelem);
		}
		if (cw & (XASMASG|XASMINOUT)) {
			/* output parameter */
			ip2 = ipnode(mkbinode(ASSIGN, q, tcopy(p->n_left), t));
			DLIST_INSERT_AFTER(ip, ip2, qelem);
		}
		break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		break;

	case 'm': /* memory operand */
		/* store and reload value */
		q = p->n_left;
		if (optype(q->n_op) == LTYPE) {
			if (q->n_op == TEMP) {
				ooff = BITOOR(freetemp(szty(t)));
				cvtemps(ip, q->n_rval, ooff);
			} else if (q->n_op == REG)
				comperr("xasm m and reg");
		} else if (q->n_op == UMUL && 
		    (q->n_left->n_op != TEMP && q->n_left->n_op != REG)) {
			t = q->n_left->n_type;
			ooff = p2env.epp->ip_tmpnum++;
			ip2 = ipnode(mkbinode(ASSIGN,
			    mklnode(TEMP, 0, ooff, t), q->n_left, t));
			q->n_left = mklnode(TEMP, 0, ooff, t);
			DLIST_INSERT_BEFORE(ip, ip2, qelem);
		}
		break;

	case 'i': /* immediate constant */
	case 'n': /* numeric constant */
		if (p->n_left->n_op == ICON)
			break;
		p->n_name = tmpstrdup(p->n_name);
		c = strchr(p->n_name, XASMVAL(cw)); /* cannot fail */
		if (c[1]) {
			c[0] = c[1], c[1] = 0;
			goto again;
		} else
			uerror("constant required");
		break;

	default:
		uerror("unsupported xasm option string '%s'", p->n_name);
	}
}

/* Extended assembler hacks */
static void
fixxasm(struct p2env *p2e)
{
	struct interpass *pole = &p2e->ipole;
	struct interpass *ip;
	NODE *p;

	DLIST_FOREACH(ip, pole, qelem) {
		if (ip->type != IP_NODE || ip->ip_node->n_op != XASM)
			continue;
		thisline = ip->lineno;
		p = ip->ip_node->n_left;

		if (p->n_op == ICON && p->n_type == STRTY)
			continue;

		/* replace numeric redirections with its underlying type */
		flist(p, delnums, ip);

		/*
		 * Ensure that the arg nodes can be directly addressable
		 * We decide that everything shall be LTYPE here.
		 */
		flist(p, ltypify, ip);
	}
}

/*
 * Extract codeword from xasm string */
int
xasmcode(char *s)
{
	int cw = 0, nm = 0;

	while (*s) {
		switch ((int)*s) {
		case '=': cw |= XASMASG; break;
		case '&': cw |= XASMCONSTR; break;
		case '+': cw |= XASMINOUT; break;
		case '%': break;
		default:
			if ((*s >= 'a' && *s <= 'z') ||
			    (*s >= 'A' && *s <= 'Z') ||
			    (*s >= '0' && *s <= '9')) {
				if (nm == 0)
					cw |= *s;
				else
					cw |= (*s << ((nm + 1) * 8));
				nm++;
				break;
			}
			uerror("bad xasm constraint %c", *s);
		}
		s++;
	}
	return cw;
}

static int xasnum, xoffnum;

static void
xconv(NODE *p, void *arg)
{
	if (p->n_op != TEMP || p->n_rval != xasnum)
		return;
	p->n_op = OREG;
	p->n_rval = FPREG;
	p->n_lval = xoffnum;
}

/*
 * Convert nodes of type TEMP to op with lval off.
 */
static void
cvtemps(struct interpass *ipl, int tnum, int off)
{
	struct interpass *ip;

	xasnum = tnum;
	xoffnum = off;

	DLIST_FOREACH(ip, ipl, qelem)
		if (ip->type == IP_NODE)
			walkf(ip->ip_node, xconv, 0);
	walkf(ipl->ip_node, xconv, 0);
}
