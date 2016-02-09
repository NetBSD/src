/*	Id: inline.c,v 1.65 2015/11/17 19:19:40 ragge Exp 	*/	
/*	$NetBSD: inline.c,v 1.1.1.7 2016/02/09 20:28:50 plunky Exp $	*/
/*
 * Copyright (c) 2003, 2008 Anders Magnusson (ragge@ludd.luth.se).
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

#include <stdarg.h>

/*
 * Simple description of how the inlining works:
 * A function found with the keyword "inline" is always saved.
 * If it also has the keyword "extern" it is written out thereafter.
 * If it has the keyword "static" it will be written out if it is referenced.
 * inlining will only be done if -xinline is given, and only if it is 
 * possible to inline the function.
 */
static void printip(struct interpass *pole);

struct ntds {
	int temp;
	TWORD type;
	union dimfun *df;
	struct attr *attr;
};

/*
 * ilink from ipole points to the next struct in the list of functions.
 */
static struct istat {
	SLIST_ENTRY(istat) link;
	struct symtab *sp;
	int flags;
#define	CANINL	1	/* function is possible to inline */
#define	WRITTEN	2	/* function is written out */
#define	REFD	4	/* Referenced but not yet written out */
	struct ntds *nt;/* Array of arg temp type data */
	int nargs;	/* number of args in array */
	int retval;	/* number of return temporary, if any */
	struct interpass shead;
} *cifun;

static SLIST_HEAD(, istat) ipole = { NULL, &ipole.q_forw };
static int nlabs, svclass;

#define	IP_REF	(MAXIP+1)
#ifdef PCC_DEBUG
#define	SDEBUG(x)	if (sdebug) printf x
#else
#define	SDEBUG(x)
#endif

int isinlining;
int inlstatcnt;

#define	SZSI	sizeof(struct istat)
int istatsz = SZSI;
#define	ialloc() memset(permalloc(SZSI), 0, SZSI); inlstatcnt++

/*
 * Get prolog/epilog for a function.
 */
static struct interpass_prolog *
getprol(struct istat *is, int type)
{
	struct interpass *ip;

	DLIST_FOREACH(ip, &is->shead, qelem)
		if (ip->type == type)
			return (struct interpass_prolog *)ip;
	cerror("getprol: %d not found", type);
	return 0; /* XXX */
}

static struct istat *
findfun(struct symtab *sp)
{
	struct istat *is;

	SLIST_FOREACH(is, &ipole, link)
		if (is->sp == sp)
			return is;
	return NULL;
}

static void
refnode(struct symtab *sp)
{
	struct interpass *ip;

	SDEBUG(("refnode(%s)\n", sp->sname));

	ip = permalloc(sizeof(*ip));
	ip->type = IP_REF;
	ip->ip_name = (char *)sp;
	inline_addarg(ip);
}

/*
 * Save attributes permanent.
 */
static struct attr *
inapcopy(struct attr *ap)
{
	struct attr *nap = attr_dup(ap);

	if (ap->next)
		nap->next = inapcopy(ap->next);
	return nap;
}

/*
 * Copy a tree onto the permanent heap to save for inline.
 */
static NODE *
intcopy(NODE *p)
{
	NODE *q = permalloc(sizeof(NODE));
	int o = coptype(p->n_op); /* XXX pass2 optype? */

	*q = *p;
	if (nlabs > 1 && (p->n_op == REG || p->n_op == OREG) &&
	    regno(p) == FPREG)
		SLIST_FIRST(&ipole)->flags &= ~CANINL; /* no stack refs */
	if (q->n_ap)
		q->n_ap = inapcopy(q->n_ap);
	if (q->n_op == NAME || q->n_op == ICON ||
	    q->n_op == XASM || q->n_op == XARG) {
		if (*q->n_name)
			q->n_name = xstrdup(q->n_name); /* XXX permstrdup */
		else
			q->n_name = "";
	}
	if (o == BITYPE)
		q->n_right = intcopy(q->n_right);
	if (o != LTYPE)
		q->n_left = intcopy(q->n_left);
	return q;
}

void
inline_addarg(struct interpass *ip)
{
	struct interpass_prolog *ipp;
	NODE *q;
	static int g = 0;
	extern P1ND *cftnod;

	SDEBUG(("inline_addarg(%p)\n", ip));
	DLIST_INSERT_BEFORE(&cifun->shead, ip, qelem);
	switch (ip->type) {
	case IP_ASM:
		ip->ip_asm = xstrdup(ip->ip_asm);
		break;
	case IP_DEFLAB:
		nlabs++;
		break;
	case IP_NODE:
		q = ip->ip_node;
		ip->ip_node = intcopy(ip->ip_node);
		tfree(q);
		break;
	case IP_EPILOG:
		ipp = (struct interpass_prolog *)ip;
		if (ipp->ip_labels[0])
			uerror("no computed goto in inlined functions");
		ipp->ip_labels = &g;
		break;
	}
	if (cftnod)
		cifun->retval = regno(cftnod);
}

/*
 * Called to setup for inlining of a new function.
 */
void
inline_start(struct symtab *sp, int class)
{
	struct istat *is;

	SDEBUG(("inline_start(\"%s\")\n", sp->sname));

	if (isinlining)
		cerror("already inlining function");

	svclass = class;
	if ((is = findfun(sp)) != 0) {
		if (!DLIST_ISEMPTY(&is->shead, qelem))
			uerror("inline function already defined");
	} else {
		is = ialloc();
		is->sp = sp;
		SLIST_INSERT_FIRST(&ipole, is, link);
		DLIST_INIT(&is->shead, qelem);
	}
	cifun = is;
	nlabs = 0;
	isinlining++;
}

/*
 * End of an inline function. In C99 an inline function declared "extern"
 * should also have external linkage and are therefore printed out.
 *
 * Gcc inline syntax is a mess, see matrix below on emitting functions:
 *		    without extern
 *	-std=		-	gnu89	gnu99	
 *	gcc 3.3.5:	ja	ja	ja
 *	gcc 4.1.3:	ja	ja	ja
 *	gcc 4.3.1	ja	ja	nej	
 * 
 *		     with extern
 *	gcc 3.3.5:	nej	nej	nej
 *	gcc 4.1.3:	nej	nej	nej
 *	gcc 4.3.1	nej	nej	ja	
 *
 * The above is only true if extern is given on the same line as the
 * function declaration.  If given as a separate definition it do not count.
 *
 * The attribute gnu_inline sets gnu89 behaviour.
 * Since pcc mimics gcc 4.3.1 that is the behaviour we emulate.
 */
void
inline_end(void)
{
	struct symtab *sp = cifun->sp;

	SDEBUG(("inline_end()\n"));

	if (sdebug)printip(&cifun->shead);
	isinlining = 0;

	if (xgnu89 && svclass == SNULL)
		sp->sclass = EXTERN;

#ifdef GCC_COMPAT
	if (sp->sclass != STATIC &&
	    (attr_find(sp->sap, GCC_ATYP_GNU_INLINE) || xgnu89)) {
		if (sp->sclass == EXTDEF)
			sp->sclass = EXTERN;
		else
			sp->sclass = EXTDEF;
	}
#endif

	if (sp->sclass == EXTDEF) {
		cifun->flags |= REFD;
		inline_prtout();
	}
}

/*
 * Called when an inline function is found, to be sure that it will
 * be written out.
 * The function may not be defined when inline_ref() is called.
 */
void
inline_ref(struct symtab *sp)
{
	struct istat *w;

	SDEBUG(("inline_ref(\"%s\")\n", sp->sname));
	if (sp->sclass == SNULL)
		return; /* only inline, no references */
	if (isinlining) {
		refnode(sp);
	} else {
		SLIST_FOREACH(w,&ipole, link) {
			if (w->sp != sp)
				continue;
			w->flags |= REFD;
			return;
		}
		/* function not yet defined, print out when found */
		w = ialloc();
		w->sp = sp;
		w->flags |= REFD;
		SLIST_INSERT_FIRST(&ipole, w, link);
		DLIST_INIT(&w->shead, qelem);
	}
}

static void
puto(struct istat *w)
{
	struct interpass_prolog *ipp, *epp, *pp;
	struct interpass *ip, *nip;
	extern int crslab;
	int lbloff = 0;

	/* Copy the saved function and print it out */
	ipp = 0; /* XXX data flow analysis */
	DLIST_FOREACH(ip, &w->shead, qelem) {
		switch (ip->type) {
		case IP_EPILOG:
		case IP_PROLOG:
			if (ip->type == IP_PROLOG) {
				ipp = (struct interpass_prolog *)ip;
				/* fix label offsets */
				lbloff = crslab - ipp->ip_lblnum;
			} else {
				epp = (struct interpass_prolog *)ip;
				crslab += (epp->ip_lblnum - ipp->ip_lblnum);
			}
			pp = xmalloc(sizeof(struct interpass_prolog));
			memcpy(pp, ip, sizeof(struct interpass_prolog));
			pp->ip_lblnum += lbloff;
#ifdef PCC_DEBUG
			if (ip->type == IP_EPILOG && crslab != pp->ip_lblnum)
				cerror("puto: %d != %d", crslab, pp->ip_lblnum);
#endif
			pass2_compile((struct interpass *)pp);
			break;

		case IP_REF:
			inline_ref((struct symtab *)ip->ip_name);
			break;

		default:
			nip = xmalloc(sizeof(struct interpass));
			*nip = *ip;
			if (nip->type == IP_NODE) {
				NODE *p;

				p = nip->ip_node = tcopy(nip->ip_node);
				if (p->n_op == GOTO)
					slval(p->n_left,
					    glval(p->n_left) + lbloff);
				else if (p->n_op == CBRANCH)
					slval(p->n_right,
					    glval(p->n_right) + lbloff);
			} else if (nip->type == IP_DEFLAB)
				nip->ip_lbl += lbloff;
			pass2_compile(nip);
			break;
		}
	}
	w->flags |= WRITTEN;
}

/*
 * printout functions that are referenced.
 */
void
inline_prtout(void)
{
	struct istat *w;
	int gotone = 0;

	SLIST_FOREACH(w, &ipole, link) {
		if ((w->flags & (REFD|WRITTEN)) == REFD &&
		    !DLIST_ISEMPTY(&w->shead, qelem)) {
			locctr(PROG, w->sp);
			defloc(w->sp);
			puto(w);
			w->flags |= WRITTEN;
			gotone++;
		}
	}
	if (gotone)
		inline_prtout();
}

#if 1
static void
printip(struct interpass *pole)
{
	static char *foo[] = {
	   0, "NODE", "PROLOG", "STKOFF", "EPILOG", "DEFLAB", "DEFNAM", "ASM" };
	struct interpass *ip;
	struct interpass_prolog *ipplg, *epplg;

	DLIST_FOREACH(ip, pole, qelem) {
		if (ip->type > MAXIP)
			printf("IP(%d) (%p): ", ip->type, ip);
		else
			printf("%s (%p): ", foo[ip->type], ip);
		switch (ip->type) {
		case IP_NODE: printf("\n");
#ifdef PCC_DEBUG
#ifndef TWOPASS
			{ extern void e2print(NODE *p, int down, int *a, int *b);
			fwalk(ip->ip_node, e2print, 0); break; }
#endif
#endif
		case IP_PROLOG:
			ipplg = (struct interpass_prolog *)ip;
			printf("%s %s regs %lx autos %d mintemp %d minlbl %d\n",
			    ipplg->ipp_name, ipplg->ipp_vis ? "(local)" : "",
			    (long)ipplg->ipp_regs[0], ipplg->ipp_autos,
			    ipplg->ip_tmpnum, ipplg->ip_lblnum);
			break;
		case IP_EPILOG:
			epplg = (struct interpass_prolog *)ip;
			printf("%s %s regs %lx autos %d mintemp %d minlbl %d\n",
			    epplg->ipp_name, epplg->ipp_vis ? "(local)" : "",
			    (long)epplg->ipp_regs[0], epplg->ipp_autos,
			    epplg->ip_tmpnum, epplg->ip_lblnum);
			break;
		case IP_DEFLAB: printf(LABFMT "\n", ip->ip_lbl); break;
		case IP_DEFNAM: printf("\n"); break;
		case IP_ASM: printf("%s", ip->ip_asm); break;
		default:
			break;
		}
	}
}
#endif

static int toff;

static P1ND *
mnode(struct ntds *nt, P1ND *p)
{
	P1ND *q;
	int num = nt->temp + toff;

	if (p->n_op == CM) {
		q = p->n_right;
		q = tempnode(num, nt->type, nt->df, nt->attr);
		nt--;
		p->n_right = buildtree(ASSIGN, q, p->n_right);
		p->n_left = mnode(nt, p->n_left);
		p->n_op = COMOP;
	} else {
		p = pconvert(p);
		q = tempnode(num, nt->type, nt->df, nt->attr);
		p = buildtree(ASSIGN, q, p);
	}
	return p;
}

static void
rtmps(NODE *p, void *arg)
{
	if (p->n_op == TEMP)
		regno(p) += toff;
}

/*
 * Inline a function. Returns the return value.
 * There are two major things that must be converted when 
 * inlining a function:
 * - Label numbers must be updated with an offset.
 * - The stack block must be relocated (add to REG or OREG).
 * - Temporaries should be updated (but no must)
 *
 * Extra tricky:  The call is P1ND, nut the resulting tree is already NODE...
 */
P1ND *
inlinetree(struct symtab *sp, P1ND *f, P1ND *ap)
{
	extern int crslab, tvaloff;
	struct istat *is = findfun(sp);
	struct interpass *ip, *ipf, *ipl;
	struct interpass_prolog *ipp, *ipe;
	int lmin, l0, l1, l2, gainl, n;
	NODE *pp;
	P1ND *p, *rp;

	if (is == NULL || nerrors) {
		inline_ref(sp); /* prototype of not yet declared inline ftn */
		return NULL;
	}

	SDEBUG(("inlinetree(%p,%p) OK %d\n", f, ap, is->flags & CANINL));

#ifdef GCC_COMPAT
	gainl = attr_find(sp->sap, GCC_ATYP_ALW_INL) != NULL;
#else
	gainl = 0;
#endif

	n = nerrors;
	if ((is->flags & CANINL) == 0 && gainl)
		werror("cannot inline but always_inline");
	nerrors = n;

	if ((is->flags & CANINL) == 0 || (xinline == 0 && gainl == 0)) {
		if (is->sp->sclass == STATIC || is->sp->sclass == USTATIC)
			inline_ref(sp);
		return NULL;
	}

	if (isinlining && cifun->sp == sp) {
		/* Do not try to inline ourselves */
		inline_ref(sp);
		return NULL;
	}

#ifdef mach_i386
	if (kflag) {
		is->flags |= REFD; /* if static inline, emit */
		return NULL; /* XXX cannot handle hidden ebx arg */
	}
#endif

	/* emit jumps to surround inline function */
	branch(l0 = getlab());
	plabel(l1 = getlab());
	l2 = getlab();
	SDEBUG(("branch labels %d,%d,%d\n", l0, l1, l2));

	/* From here it is NODE */

	ipp = getprol(is, IP_PROLOG);
	ipe = getprol(is, IP_EPILOG);

	/* Fix label & temp offsets */

	SDEBUG(("pre-offsets crslab %d tvaloff %d\n", crslab, tvaloff));
	lmin = crslab - ipp->ip_lblnum;
	crslab += (ipe->ip_lblnum - ipp->ip_lblnum) + 2;
	toff = tvaloff - ipp->ip_tmpnum;
	tvaloff += (ipe->ip_tmpnum - ipp->ip_tmpnum) + 1;
	SDEBUG(("offsets crslab %d lmin %d tvaloff %d toff %d\n",
	    crslab, lmin, tvaloff, toff));

	/* traverse until first real label */
	n = 0;
	DLIST_FOREACH(ipf, &is->shead, qelem) {
		if (ipf->type == IP_REF)
			inline_ref((struct symtab *)ipf->ip_name);
		if (ipf->type == IP_DEFLAB && n++ == 1)
			break;
	}

	/* traverse backwards to last label */
	DLIST_FOREACH_REVERSE(ipl, &is->shead, qelem) {
		if (ipl->type == IP_REF)
			inline_ref((struct symtab *)ipl->ip_name);
		if (ipl->type == IP_DEFLAB)
			break;
	}
		
	/* So, walk over all statements and emit them */
	for (ip = ipf; ip != ipl; ip = DLIST_NEXT(ip, qelem)) {
		switch (ip->type) {
		case IP_NODE:
			pp = tcopy(ip->ip_node);
			if (pp->n_op == GOTO)
				slval(pp->n_left, glval(pp->n_left) + lmin);
			else if (pp->n_op == CBRANCH)
				slval(pp->n_right, glval(pp->n_right) + lmin);
			walkf(pp, rtmps, 0);
#ifdef PCC_DEBUG
#ifndef TWOPASS
			if (sdebug) {
				extern void e2print(NODE *p, int down, int *a, int *b);
				printf("converted node\n");
				fwalk(ip->ip_node, e2print, 0);
				fwalk(pp, e2print, 0);
			}
#endif
#endif
			send_passt(IP_NODE, pp);
			break;

		case IP_DEFLAB:
			SDEBUG(("converted label %d to %d\n",
			    ip->ip_lbl, ip->ip_lbl + lmin));
			send_passt(IP_DEFLAB, ip->ip_lbl + lmin);
			break;

		case IP_ASM:
			send_passt(IP_ASM, ip->ip_asm);
			break;

		case IP_REF:
			inline_ref((struct symtab *)ip->ip_name);
			break;

		default:
			cerror("bad inline stmt %d", ip->type);
		}
	}
	SDEBUG(("last label %d to %d\n", ip->ip_lbl, ip->ip_lbl + lmin));
	send_passt(IP_DEFLAB, ip->ip_lbl + lmin);

	branch(l2);
	plabel(l0);

	/* Here we are P1ND again */
	rp = block(GOTO, bcon(l1), NULL, INT, 0, 0);
	if (is->retval)
		p = tempnode(is->retval + toff, DECREF(sp->stype),
		    sp->sdf, sp->sap);
	else
		p = xbcon(0, NULL, DECREF(sp->stype));
	rp = buildtree(COMOP, rp, p);

	if (is->nargs) {
		p = mnode(&is->nt[is->nargs-1], ap);
		rp = buildtree(COMOP, p, rp);
	}

	p1tfree(f);
	return rp;
}

void
inline_args(struct symtab **sp, int nargs)
{
	union arglist *al;
	struct istat *cf;
	TWORD t;
	int i;

	SDEBUG(("inline_args\n"));
	cf = cifun;
	/*
	 * First handle arguments.  We currently do not inline anything if:
	 * - function has varargs
	 * - function args are volatile, checked if no temp node is asg'd.
	 */
	/* XXX - this is ugly, invent something better */
	if (cf->sp->sdf->dfun == NULL)
		return; /* no prototype */
	for (al = cf->sp->sdf->dfun; al->type != TNULL; al++) {
		t = al->type;
		if (t == TELLIPSIS)
			return; /* cannot inline */
		if (ISSOU(BTYPE(t)))
			al++;
		for (; t > BTMASK; t = DECREF(t))
			if (ISARY(t) || ISFTN(t))
				al++;
	}

	if (nargs) {
		for (i = 0; i < nargs; i++)
			if ((sp[i]->sflags & STNODE) == 0)
				return; /* not temporary */
		cf->nt = permalloc(sizeof(struct ntds)*nargs);
		for (i = 0; i < nargs; i++) {
			cf->nt[i].temp = sp[i]->soffset;
			cf->nt[i].type = sp[i]->stype;
			cf->nt[i].df = sp[i]->sdf;
			cf->nt[i].attr = sp[i]->sap;
		}
	}
	cf->nargs = nargs;
	cf->flags |= CANINL;
}
