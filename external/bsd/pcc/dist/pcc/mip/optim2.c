/*	$Id: optim2.c,v 1.1.1.2 2009/09/04 00:27:34 gmcgarry Exp $	*/
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

#include "pass2.h"

#include <string.h>
#include <stdlib.h>

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define	BDEBUG(x)	if (b2debug) printf x

#define	mktemp(n, t)	mklnode(TEMP, 0, n, t)

/* main switch for new things not yet ready for all-day use */
/* #define ENABLE_NEW */


static int dfsnum;

void saveip(struct interpass *ip);
void deljumps(struct p2env *);
void optdump(struct interpass *ip);
void printip(struct interpass *pole);

static struct varinfo defsites;
struct interpass *storesave;

void bblocks_build(struct p2env *, struct labelinfo *, struct bblockinfo *);
void cfg_build(struct p2env *, struct labelinfo *labinfo);
void cfg_dfs(struct basicblock *bb, unsigned int parent, 
	     struct bblockinfo *bbinfo);
void dominators(struct p2env *, struct bblockinfo *bbinfo);
struct basicblock *
ancestorwithlowestsemi(struct basicblock *bblock, struct bblockinfo *bbinfo);
void link(struct basicblock *parent, struct basicblock *child);
void computeDF(struct basicblock *bblock, struct bblockinfo *bbinfo);
void printDF(struct p2env *p2e, struct bblockinfo *bbinfo);
void findTemps(struct interpass *ip);
void placePhiFunctions(struct p2env *, struct bblockinfo *bbinfo);
void renamevar(struct p2env *p2e,struct basicblock *bblock, struct bblockinfo *bbinfo);
void removephi(struct p2env *p2e, struct labelinfo *,struct bblockinfo *bbinfo);
void remunreach(struct p2env *);

/* create "proper" basic blocks, add labels where needed (so bblocks have labels) */
/* run before bb generate */
static void add_labels(struct p2env*) ;

/* Perform trace scheduling, try to get rid of gotos as much as possible */
void TraceSchedule(struct p2env*) ;

#ifdef ENABLE_NEW
static void do_cse(struct p2env* p2e) ;
#endif

/* Walk the complete set, performing a function on each node. 
 * if type is given, apply function on only that type */
void WalkAll(struct p2env* p2e, void (*f) (NODE*, void*), void* arg, int type) ;

void BBLiveDead(struct basicblock* bblock, int what, unsigned int variable) ;

/* Fill the live/dead code */
void LiveDead(struct p2env* p2e, int what, unsigned int variable) ;

#ifdef PCC_DEBUG
void printflowdiagram(struct p2env *,struct labelinfo *labinfo,struct bblockinfo *bbinfo,char *);
#endif

void
optimize(struct p2env *p2e)
{
	struct interpass *ipole = &p2e->ipole;
	struct labelinfo labinfo;
	struct bblockinfo bbinfo;

	if (b2debug) {
		printf("initial links\n");
		printip(ipole);
	}

	if (xdeljumps)
		deljumps(p2e); /* Delete redundant jumps and dead code */

	if (xssaflag)
		add_labels(p2e) ;
#ifdef ENABLE_NEW
	do_cse(p2e);
#endif

#ifdef PCC_DEBUG
	if (b2debug) {
		printf("links after deljumps\n");
		printip(ipole);
	}
#endif
	if (xssaflag || xtemps) {
		DLIST_INIT(&p2e->bblocks, bbelem);
		bblocks_build(p2e, &labinfo, &bbinfo);
		BDEBUG(("Calling cfg_build\n"));
		cfg_build(p2e, &labinfo);
	
#ifdef PCC_DEBUG
		printflowdiagram(p2e, &labinfo, &bbinfo,"first");
#endif
	}
	if (xssaflag) {
		BDEBUG(("Calling dominators\n"));
		dominators(p2e, &bbinfo);
		BDEBUG(("Calling computeDF\n"));
		computeDF(DLIST_NEXT(&p2e->bblocks, bbelem), &bbinfo);

		if (b2debug) {
			printDF(p2e,&bbinfo);
		}

		BDEBUG(("Calling placePhiFunctions\n"));

		placePhiFunctions(p2e, &bbinfo);

		BDEBUG(("Calling renamevar\n"));

		renamevar(p2e,DLIST_NEXT(&p2e->bblocks, bbelem), &bbinfo);

		BDEBUG(("Calling removephi\n"));

#ifdef PCC_DEBUG
		printflowdiagram(p2e, &labinfo, &bbinfo,"ssa");
#endif

		removephi(p2e,&labinfo,&bbinfo);

		BDEBUG(("Calling remunreach\n"));
/*		remunreach(p2e); */
		
		/*
		 Recalculate basic blocks and cfg that was destroyed
		 by removephi
		 */
		/* first, clean up all what deljumps should have done, and more */

		/*TODO: add the basic blocks done by the ssa code by hand. 
		 * The trace scheduler should not change the order in which blocks
		 * are executed or what data is calculated. Thus, the BBlock order
		 * should remain correct.
		 */

#ifdef ENABLE_NEW
		DLIST_INIT(&p2e->bblocks, bbelem);
		bblocks_build(p2e, &labinfo, &bbinfo);
		BDEBUG(("Calling cfg_build\n"));
		cfg_build(p2e, &labinfo);

		TraceSchedule(p2e);
#ifdef PCC_DEBUG
		printflowdiagram(p2e, &labinfo, &bbinfo,"sched_trace");

		if (b2debug) {
			printf("after tracesched\n");
			printip(ipole);
			fflush(stdout) ;
		}
#endif
#endif

		/* Now, clean up the gotos we do not need any longer */
		if (xdeljumps)
			deljumps(p2e); /* Delete redundant jumps and dead code */

		DLIST_INIT(&p2e->bblocks, bbelem);
		bblocks_build(p2e, &labinfo, &bbinfo);
		BDEBUG(("Calling cfg_build\n"));
		cfg_build(p2e, &labinfo);

#ifdef PCC_DEBUG
		printflowdiagram(p2e, &labinfo, &bbinfo,"no_phi");

		if (b2debug) {
			printf("new tree\n");
			printip(ipole);
		}
#endif
	}

#ifdef PCC_DEBUG
	{
		int i;
		for (i = NIPPREGS; i--; )
			if (p2e->epp->ipp_regs[i] != 0)
				comperr("register error");
	}
#endif

	myoptim(ipole);
}

/*
 * Delete unused labels, excess of labels, gotos to gotos.
 * This routine can be made much more efficient.
 */
void
deljumps(struct p2env *p2e)
{
	struct interpass *ipole = &p2e->ipole;
	struct interpass *ip, *n, *ip2, *start;
	int gotone,low, high;
	int *lblary, *jmpary, sz, o, i, j, lab1, lab2;
	int del;
	extern int negrel[];
	extern size_t negrelsize;

	low = p2e->ipp->ip_lblnum;
	high = p2e->epp->ip_lblnum;

#ifdef notyet
	mark = tmpmark(); /* temporary used memory */
#endif

	sz = (high-low) * sizeof(int);
	lblary = tmpalloc(sz);
	jmpary = tmpalloc(sz);

	/*
	 * XXX: Find the first two labels. They may not be deleted,
	 * because the register allocator expects them to be there.
	 * These will not be coalesced with any other label.
	 */
	lab1 = lab2 = -1;
	start = NULL;
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_DEFLAB)
			continue;
		if (lab1 < 0)
			lab1 = ip->ip_lbl;
		else if (lab2 < 0) {
			lab2 = ip->ip_lbl;
			start = ip;
		} else	/* lab1 >= 0 && lab2 >= 0, we're done. */
			break;
	}
	if (lab1 < 0 || lab2 < 0)
		comperr("deljumps");

again:	gotone = 0;
	memset(lblary, 0, sz);
	lblary[lab1 - low] = lblary[lab2 - low] = 1;
	memset(jmpary, 0, sz);

	/* refcount and coalesce all labels */
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type == IP_DEFLAB && ip->ip_lbl != lab1 &&
		    ip->ip_lbl != lab2) {
			n = DLIST_NEXT(ip, qelem);

			/*
			 * Find unconditional jumps directly following a
			 * label. Jumps jumping to themselves are not
			 * taken into account.
			 */
			if (n->type == IP_NODE && n->ip_node->n_op == GOTO) {
				i = (int)n->ip_node->n_left->n_lval;
				if (i != ip->ip_lbl)
					jmpary[ip->ip_lbl - low] = i;
			}

			while (n->type == IP_DEFLAB) {
				if (n->ip_lbl != lab1 && n->ip_lbl != lab2 &&
				    lblary[n->ip_lbl-low] >= 0) {
					/*
					 * If the label is used, mark the
					 * label to be coalesced with as
					 * used, too.
					 */
					if (lblary[n->ip_lbl - low] > 0 &&
					    lblary[ip->ip_lbl - low] == 0)
						lblary[ip->ip_lbl - low] = 1;
					lblary[n->ip_lbl - low] = -ip->ip_lbl;
				}
				n = DLIST_NEXT(n, qelem);
			}
		}
		if (ip->type != IP_NODE)
			continue;
		o = ip->ip_node->n_op;
		if (o == GOTO)
			i = (int)ip->ip_node->n_left->n_lval;
		else if (o == CBRANCH)
			i = (int)ip->ip_node->n_right->n_lval;
		else
			continue;

		/*
		 * Mark destination label i as used, if it is not already.
		 * If i is to be merged with label j, mark j as used, too.
		 */
		if (lblary[i - low] == 0)
			lblary[i-low] = 1;
		else if ((j = lblary[i - low]) < 0 && lblary[-j - low] == 0)
			lblary[-j - low] = 1;
	}

	/* delete coalesced/unused labels and rename gotos */
	DLIST_FOREACH(ip, ipole, qelem) {
		n = DLIST_NEXT(ip, qelem);
		if (n->type == IP_DEFLAB) {
			if (lblary[n->ip_lbl-low] <= 0) {
				DLIST_REMOVE(n, qelem);
				gotone = 1;
			}
		}
		if (ip->type != IP_NODE)
			continue;
		o = ip->ip_node->n_op;
		if (o == GOTO)
			i = (int)ip->ip_node->n_left->n_lval;
		else if (o == CBRANCH)
			i = (int)ip->ip_node->n_right->n_lval;
		else
			continue;

		/* Simplify (un-)conditional jumps to unconditional jumps. */
		if (jmpary[i - low] > 0) {
			gotone = 1;
			i = jmpary[i - low];
			if (o == GOTO)
				ip->ip_node->n_left->n_lval = i;
			else
				ip->ip_node->n_right->n_lval = i;
		}

		/* Fixup for coalesced labels. */
		if (lblary[i-low] < 0) {
			if (o == GOTO)
				ip->ip_node->n_left->n_lval = -lblary[i-low];
			else
				ip->ip_node->n_right->n_lval = -lblary[i-low];
		}
	}

	/* Delete gotos to the next statement */
	DLIST_FOREACH(ip, ipole, qelem) {
		n = DLIST_NEXT(ip, qelem);
		if (n->type != IP_NODE)
			continue;
		o = n->ip_node->n_op;
		if (o == GOTO)
			i = (int)n->ip_node->n_left->n_lval;
#if 0 /* XXX must check for side effects in expression */
		else if (o == CBRANCH)
			i = (int)n->ip_node->n_right->n_lval;
#endif
		else
			continue;

		ip2 = n;
		ip2 = DLIST_NEXT(ip2, qelem);

		if (ip2->type != IP_DEFLAB)
			continue;
		if (ip2->ip_lbl == i && i != lab1 && i != lab2) {
			tfree(n->ip_node);
			DLIST_REMOVE(n, qelem);
			gotone = 1;
		}
	}

	/*
	 * Transform cbranch cond, 1; goto 2; 1: ... into
	 * cbranch !cond, 2; 1: ...
	 */
	DLIST_FOREACH(ip, ipole, qelem) {
		n = DLIST_NEXT(ip, qelem);
		ip2 = DLIST_NEXT(n, qelem);
		if (ip->type != IP_NODE || ip->ip_node->n_op != CBRANCH)
			continue;
		if (n->type != IP_NODE || n->ip_node->n_op != GOTO)
			continue;
		if (ip2->type != IP_DEFLAB)
			continue;
		i = (int)ip->ip_node->n_right->n_lval;
		j = (int)n->ip_node->n_left->n_lval;
		if (j == lab1 || j == lab2)
			continue;
		if (i != ip2->ip_lbl || i == lab1 || i == lab2)
			continue;
		ip->ip_node->n_right->n_lval = j;
		i = ip->ip_node->n_left->n_op;
		if (i < EQ || i - EQ >= (int)negrelsize)
			comperr("deljumps: unexpected op");
		ip->ip_node->n_left->n_op = negrel[i - EQ];
		tfree(n->ip_node);
		DLIST_REMOVE(n, qelem);
		gotone = 1;
	}

	/* Delete everything after a goto up to the next label. */
	for (ip = start, del = 0; ip != DLIST_ENDMARK(ipole);
	     ip = DLIST_NEXT(ip, qelem)) {
loop:
		if ((n = DLIST_NEXT(ip, qelem)) == DLIST_ENDMARK(ipole))
			break;
		if (n->type != IP_NODE) {
			del = 0;
			continue;
		}
		if (del) {
			tfree(n->ip_node);
			DLIST_REMOVE(n, qelem);
			gotone = 1;
			goto loop;
		} else if (n->ip_node->n_op == GOTO)
			del = 1;
	}

	if (gotone)
		goto again;

#ifdef notyet
	tmpfree(mark);
#endif
}

void
optdump(struct interpass *ip)
{
	static char *nm[] = { "node", "prolog", "newblk", "epilog", "locctr",
		"deflab", "defnam", "asm" };
	printf("type %s\n", nm[ip->type-1]);
	switch (ip->type) {
	case IP_NODE:
#ifdef PCC_DEBUG
		fwalk(ip->ip_node, e2print, 0);
#endif
		break;
	case IP_DEFLAB:
		printf("label " LABFMT "\n", ip->ip_lbl);
		break;
	case IP_ASM:
		printf(": %s\n", ip->ip_asm);
		break;
	}
}

/*
 * Build the basic blocks, algorithm 9.1, pp 529 in Compilers.
 *
 * Also fills the labelinfo struct with information about which bblocks
 * that contain which label.
 */

void
bblocks_build(struct p2env *p2e, struct labelinfo *labinfo,
    struct bblockinfo *bbinfo)
{
	struct interpass *ipole = &p2e->ipole;
	struct interpass *ip;
	struct basicblock *bb = NULL;
	int low, high;
	int count = 0;
	int i;

	BDEBUG(("bblocks_build (%p, %p)\n", labinfo, bbinfo));
	low = p2e->ipp->ip_lblnum;
	high = p2e->epp->ip_lblnum;

	/* 
	 * First statement is a leader.
	 * Any statement that is target of a jump is a leader.
	 * Any statement that immediately follows a jump is a leader.
	 */
	DLIST_FOREACH(ip, ipole, qelem) {
		if (bb == NULL || (ip->type == IP_EPILOG) ||
		    (ip->type == IP_DEFLAB) || (ip->type == IP_DEFNAM)) {
			bb = tmpalloc(sizeof(struct basicblock));
			bb->first = ip;
			SLIST_INIT(&bb->children);
			SLIST_INIT(&bb->parents);
			bb->dfnum = 0;
			bb->dfparent = 0;
			bb->semi = 0;
			bb->ancestor = 0;
			bb->idom = 0;
			bb->samedom = 0;
			bb->bucket = NULL;
			bb->df = NULL;
			bb->dfchildren = NULL;
			bb->Aorig = NULL;
			bb->Aphi = NULL;
			SLIST_INIT(&bb->phi);
			bb->bbnum = count;
			DLIST_INSERT_BEFORE(&p2e->bblocks, bb, bbelem);
			count++;
		}
		bb->last = ip;
		if ((ip->type == IP_NODE) && (ip->ip_node->n_op == GOTO || 
		    ip->ip_node->n_op == CBRANCH))
			bb = NULL;
		if (ip->type == IP_PROLOG)
			bb = NULL;
	}
	p2e->nbblocks = count;

	if (b2debug) {
		printf("Basic blocks in func: %d, low %d, high %d\n",
		    count, low, high);
		DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
			printf("bb %p: first %p last %p\n", bb,
			    bb->first, bb->last);
		}
	}

	labinfo->low = low;
	labinfo->size = high - low + 1;
	labinfo->arr = tmpalloc(labinfo->size * sizeof(struct basicblock *));
	for (i = 0; i < labinfo->size; i++) {
		labinfo->arr[i] = NULL;
	}
	
	bbinfo->size = count + 1;
	bbinfo->arr = tmpalloc(bbinfo->size * sizeof(struct basicblock *));
	for (i = 0; i < bbinfo->size; i++) {
		bbinfo->arr[i] = NULL;
	}

	/* Build the label table */
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		if (bb->first->type == IP_DEFLAB)
			labinfo->arr[bb->first->ip_lbl - low] = bb;
	}

	if (b2debug) {
		printf("Label table:\n");
		for (i = 0; i < labinfo->size; i++)
			if (labinfo->arr[i])
				printf("Label %d bblock %p\n", i+low,
				    labinfo->arr[i]);
	}
}

/*
 * Build the control flow graph.
 */

void
cfg_build(struct p2env *p2e, struct labelinfo *labinfo)
{
	/* Child and parent nodes */
	struct cfgnode *cnode; 
	struct cfgnode *pnode;
	struct basicblock *bb;
	
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {

		if (bb->first->type == IP_EPILOG) {
			break;
		}

		cnode = tmpalloc(sizeof(struct cfgnode));
		pnode = tmpalloc(sizeof(struct cfgnode));
		pnode->bblock = bb;

		if ((bb->last->type == IP_NODE) && 
		    (bb->last->ip_node->n_op == GOTO) &&
		    (bb->last->ip_node->n_left->n_op == ICON))  {
			if (bb->last->ip_node->n_left->n_lval - labinfo->low > 
			    labinfo->size) {
				comperr("Label out of range: %d, base %d", 
					bb->last->ip_node->n_left->n_lval, 
					labinfo->low);
			}
			cnode->bblock = labinfo->arr[bb->last->ip_node->n_left->n_lval - labinfo->low];
			SLIST_INSERT_LAST(&cnode->bblock->parents, pnode, cfgelem);
			SLIST_INSERT_LAST(&bb->children, cnode, cfgelem);
			continue;
		}
		if ((bb->last->type == IP_NODE) && 
		    (bb->last->ip_node->n_op == CBRANCH)) {
			if (bb->last->ip_node->n_right->n_lval - labinfo->low > 
			    labinfo->size) 
				comperr("Label out of range: %d", 
					bb->last->ip_node->n_left->n_lval);

			cnode->bblock = labinfo->arr[bb->last->ip_node->n_right->n_lval - labinfo->low];
			SLIST_INSERT_LAST(&cnode->bblock->parents, pnode, cfgelem);
			SLIST_INSERT_LAST(&bb->children, cnode, cfgelem);
			cnode = tmpalloc(sizeof(struct cfgnode));
			pnode = tmpalloc(sizeof(struct cfgnode));
			pnode->bblock = bb;
		}

		cnode->bblock = DLIST_NEXT(bb, bbelem);
		SLIST_INSERT_LAST(&cnode->bblock->parents, pnode, cfgelem);
		SLIST_INSERT_LAST(&bb->children, cnode, cfgelem);
	}
}

void
cfg_dfs(struct basicblock *bb, unsigned int parent, struct bblockinfo *bbinfo)
{
	struct cfgnode *cnode;
	
	if (bb->dfnum != 0)
		return;

	bb->dfnum = ++dfsnum;
	bb->dfparent = parent;
	bbinfo->arr[bb->dfnum] = bb;
	SLIST_FOREACH(cnode, &bb->children, cfgelem) {
		cfg_dfs(cnode->bblock, bb->dfnum, bbinfo);
	}
	/* Don't bring in unreachable nodes in the future */
	bbinfo->size = dfsnum + 1;
}

static bittype *
setalloc(int nelem)
{
	bittype *b;
	int sz = (nelem+NUMBITS-1)/NUMBITS;

	b = tmpalloc(sz * sizeof(bittype));
	memset(b, 0, sz * sizeof(bittype));
	return b;
}

/*
 * Algorithm 19.9, pp 414 from Appel.
 */

void
dominators(struct p2env *p2e, struct bblockinfo *bbinfo)
{
	struct cfgnode *cnode;
	struct basicblock *bb, *y, *v;
	struct basicblock *s, *sprime, *p;
	int h, i;

	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		bb->bucket = setalloc(bbinfo->size);
		bb->df = setalloc(bbinfo->size);
		bb->dfchildren = setalloc(bbinfo->size);
	}

	dfsnum = 0;
	cfg_dfs(DLIST_NEXT(&p2e->bblocks, bbelem), 0, bbinfo);

	if (b2debug) {
		struct basicblock *bbb;
		struct cfgnode *ccnode;

		DLIST_FOREACH(bbb, &p2e->bblocks, bbelem) {
			printf("Basic block %d, parents: ", bbb->dfnum);
			SLIST_FOREACH(ccnode, &bbb->parents, cfgelem) {
				printf("%d, ", ccnode->bblock->dfnum);
			}
			printf("\nChildren: ");
			SLIST_FOREACH(ccnode, &bbb->children, cfgelem) {
				printf("%d, ", ccnode->bblock->dfnum);
			}
			printf("\n");
		}
	}

	for(h = bbinfo->size - 1; h > 1; h--) {
		bb = bbinfo->arr[h];
		p = s = bbinfo->arr[bb->dfparent];
		SLIST_FOREACH(cnode, &bb->parents, cfgelem) {
			if (cnode->bblock->dfnum ==0)
				continue; /* Ignore unreachable code */

			if (cnode->bblock->dfnum <= bb->dfnum) 
				sprime = cnode->bblock;
			else 
				sprime = bbinfo->arr[ancestorwithlowestsemi
					      (cnode->bblock, bbinfo)->semi];
			if (sprime->dfnum < s->dfnum)
				s = sprime;
		}
		bb->semi = s->dfnum;
		BITSET(s->bucket, bb->dfnum);
		link(p, bb);
		for (i = 1; i < bbinfo->size; i++) {
			if(TESTBIT(p->bucket, i)) {
				v = bbinfo->arr[i];
				y = ancestorwithlowestsemi(v, bbinfo);
				if (y->semi == v->semi) 
					v->idom = p->dfnum;
				else
					v->samedom = y->dfnum;
			}
		}
		memset(p->bucket, 0, (bbinfo->size + 7)/8);
	}

	if (b2debug) {
		printf("Num\tSemi\tAncest\tidom\n");
		DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
			printf("%d\t%d\t%d\t%d\n", bb->dfnum, bb->semi,
			    bb->ancestor, bb->idom);
		}
	}

	for(h = 2; h < bbinfo->size; h++) {
		bb = bbinfo->arr[h];
		if (bb->samedom != 0) {
			bb->idom = bbinfo->arr[bb->samedom]->idom;
		}
	}
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		if (bb->idom != 0 && bb->idom != bb->dfnum) {
			BDEBUG(("Setting child %d of %d\n",
			    bb->dfnum, bbinfo->arr[bb->idom]->dfnum));
			BITSET(bbinfo->arr[bb->idom]->dfchildren, bb->dfnum);
		}
	}
}


struct basicblock *
ancestorwithlowestsemi(struct basicblock *bblock, struct bblockinfo *bbinfo)
{
	struct basicblock *u = bblock;
	struct basicblock *v = bblock;

	while (v->ancestor != 0) {
		if (bbinfo->arr[v->semi]->dfnum < 
		    bbinfo->arr[u->semi]->dfnum) 
			u = v;
		v = bbinfo->arr[v->ancestor];
	}
	return u;
}

void
link(struct basicblock *parent, struct basicblock *child)
{
	child->ancestor = parent->dfnum;
}

void
computeDF(struct basicblock *bblock, struct bblockinfo *bbinfo)
{
	struct cfgnode *cnode;
	int h, i;
	
	SLIST_FOREACH(cnode, &bblock->children, cfgelem) {
		if (cnode->bblock->idom != bblock->dfnum)
			BITSET(bblock->df, cnode->bblock->dfnum);
	}
	for (h = 1; h < bbinfo->size; h++) {
		if (!TESTBIT(bblock->dfchildren, h))
			continue;
		computeDF(bbinfo->arr[h], bbinfo);
		for (i = 1; i < bbinfo->size; i++) {
			if (TESTBIT(bbinfo->arr[h]->df, i) && 
			    (bbinfo->arr[i] == bblock ||
			     (bblock->dfnum != bbinfo->arr[i]->idom))) 
			    BITSET(bblock->df, i);
		}
	}
}

void printDF(struct p2env *p2e, struct bblockinfo *bbinfo)
{
	struct basicblock *bb;
	int i;

	printf("Dominance frontiers:\n");
    
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		printf("bb %d : ", bb->dfnum);
	
		for (i=1; i < bbinfo->size;i++) {
			if (TESTBIT(bb->df,i)) {
				printf("%d ",i);
			}
		}
	    
		printf("\n");
	}
    
}



static struct basicblock *currbb;
static struct interpass *currip;

/* Helper function for findTemps, Find assignment nodes. */
static void
searchasg(NODE *p, void *arg)
{
	struct pvarinfo *pv;
	int tempnr;
	struct varstack *stacke;
    
	if (p->n_op != ASSIGN)
		return;

	if (p->n_left->n_op != TEMP)
		return;

	tempnr=regno(p->n_left)-defsites.low;
    
	BITSET(currbb->Aorig, tempnr);
	
	pv = tmpcalloc(sizeof(struct pvarinfo));
	pv->next = defsites.arr[tempnr];
	pv->bb = currbb;
	pv->n_type = p->n_left->n_type;
	
	defsites.arr[tempnr] = pv;
	
	
	if (SLIST_FIRST(&defsites.stack[tempnr])==NULL) {
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=0;
		SLIST_INSERT_FIRST(&defsites.stack[tempnr],stacke,varstackelem);
	}
}

/* Walk the interpass looking for assignment nodes. */
void findTemps(struct interpass *ip)
{
	if (ip->type != IP_NODE)
		return;

	currip = ip;

	walkf(ip->ip_node, searchasg, 0);
}

/*
 * Algorithm 19.6 from Appel.
 */

void
placePhiFunctions(struct p2env *p2e, struct bblockinfo *bbinfo)
{
	struct basicblock *bb;
	struct basicblock *y;
	struct interpass *ip;
	int maxtmp, i, j, k;
	struct pvarinfo *n;
	struct cfgnode *cnode;
	TWORD ntype;
	struct pvarinfo *pv;
	struct phiinfo *phi;
	int phifound;

	bb = DLIST_NEXT(&p2e->bblocks, bbelem);
	defsites.low = ((struct interpass_prolog *)bb->first)->ip_tmpnum;
	bb = DLIST_PREV(&p2e->bblocks, bbelem);
	maxtmp = ((struct interpass_prolog *)bb->first)->ip_tmpnum;
	defsites.size = maxtmp - defsites.low + 1;
	defsites.arr = tmpcalloc(defsites.size*sizeof(struct pvarinfo *));
	defsites.stack = tmpcalloc(defsites.size*sizeof(SLIST_HEAD(, varstack)));
	
	for (i=0;i<defsites.size;i++)
		SLIST_INIT(&defsites.stack[i]);	
	
	/* Find all defsites */
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		currbb = bb;
		ip = bb->first;
		bb->Aorig = setalloc(defsites.size);
		bb->Aphi = setalloc(defsites.size);
		
		while (ip != bb->last) {
			findTemps(ip);
			ip = DLIST_NEXT(ip, qelem);
		}
		/* Make sure we get the last statement in the bblock */
		findTemps(ip);
	}
    
	/* For each variable */
	for (i = 0; i < defsites.size; i++) {
		/* While W not empty */
		while (defsites.arr[i] != NULL) {
			/* Remove some node n from W */
			n = defsites.arr[i];
			defsites.arr[i] = n->next;
			/* For each y in n->bb->df */
			for (j = 0; j < bbinfo->size; j++) {
				if (!TESTBIT(n->bb->df, j))
					continue;
				
				if (TESTBIT(bbinfo->arr[j]->Aphi, i))
					continue;

				y=bbinfo->arr[j];
				ntype = n->n_type;
				k = 0;
				/* Amount of predecessors for y */
				SLIST_FOREACH(cnode, &y->parents, cfgelem) 
					k++;
				/* Construct phi(...) 
				*/
			    
				phifound=0;
			    
				SLIST_FOREACH(phi, &y->phi, phielem) {
				    if (phi->tmpregno==i+defsites.low)
					phifound++;
				}
			    
				if (phifound==0) {
					if (b2debug)
					    printf("Phi in %d (%p) for %d\n",y->dfnum,y,i+defsites.low);

					phi = tmpcalloc(sizeof(struct phiinfo));
			    
					phi->tmpregno=i+defsites.low;
					phi->size=k;
					phi->n_type=ntype;
					phi->intmpregno=tmpcalloc(k*sizeof(int));
			    
					SLIST_INSERT_LAST(&y->phi,phi,phielem);
				} else {
				    if (b2debug)
					printf("Phi already in %d for %d\n",y->dfnum,i+defsites.low);
				}

				BITSET(bbinfo->arr[j]->Aphi, i);
				if (!TESTBIT(bbinfo->arr[j]->Aorig, i)) {
					pv = tmpalloc(sizeof(struct pvarinfo));
					pv->bb = y;
				        pv->n_type=ntype;
					pv->next = defsites.arr[i];
					defsites.arr[i] = pv;
				}
					

			}
		}
	}
}

/* Helper function for renamevar. */
static void
renamevarhelper(struct p2env *p2e,NODE *t,void *poplistarg)
{	
	SLIST_HEAD(, varstack) *poplist=poplistarg;
	int opty;
	int tempnr;
	int newtempnr;
	int x;
	struct varstack *stacke;
	
	if (t->n_op == ASSIGN && t->n_left->n_op == TEMP) {
		renamevarhelper(p2e,t->n_right,poplist);
				
		tempnr=regno(t->n_left)-defsites.low;
		
		newtempnr=p2e->epp->ip_tmpnum++;
		regno(t->n_left)=newtempnr;
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=newtempnr;
		SLIST_INSERT_FIRST(&defsites.stack[tempnr],stacke,varstackelem);
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=tempnr;
		SLIST_INSERT_FIRST(poplist,stacke,varstackelem);
	} else {
		if (t->n_op == TEMP) {
			tempnr=regno(t)-defsites.low;
		
			if (SLIST_FIRST(&defsites.stack[tempnr])!=NULL) {
				x=SLIST_FIRST(&defsites.stack[tempnr])->tmpregno;
				regno(t)=x;
			}
		}
		
		opty = optype(t->n_op);
		
		if (opty != LTYPE)
			renamevarhelper(p2e, t->n_left,poplist);
		if (opty == BITYPE)
			renamevarhelper(p2e, t->n_right,poplist);
	}
}


void
renamevar(struct p2env *p2e,struct basicblock *bb, struct bblockinfo *bbinfo)
{
    	struct interpass *ip;
	int h,j;
	SLIST_HEAD(, varstack) poplist;
	struct varstack *stacke;
	struct cfgnode *cfgn;
	struct cfgnode *cfgn2;
	int tmpregno,newtmpregno;
	struct phiinfo *phi;
	
	SLIST_INIT(&poplist);
	
	SLIST_FOREACH(phi,&bb->phi,phielem) {
		tmpregno=phi->tmpregno-defsites.low;
		
		newtmpregno=p2e->epp->ip_tmpnum++;
		phi->newtmpregno=newtmpregno;
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=newtmpregno;
		SLIST_INSERT_FIRST(&defsites.stack[tmpregno],stacke,varstackelem);
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=tmpregno;
		SLIST_INSERT_FIRST(&poplist,stacke,varstackelem);		
	}
	
	
	ip=bb->first;
	
	while (1) {		
		if ( ip->type == IP_NODE) {
			renamevarhelper(p2e,ip->ip_node,&poplist);
		}
		
		if (ip==bb->last)
			break;
		
		ip = DLIST_NEXT(ip, qelem);
	}
	
	SLIST_FOREACH(cfgn,&bb->children,cfgelem) {
		j=0;
		
		SLIST_FOREACH(cfgn2, &cfgn->bblock->parents, cfgelem) { 
			if (cfgn2->bblock->dfnum==bb->dfnum)
				break;
			
			j++;
		}

		SLIST_FOREACH(phi,&cfgn->bblock->phi,phielem) {
			phi->intmpregno[j]=SLIST_FIRST(&defsites.stack[phi->tmpregno-defsites.low])->tmpregno;
		}
		
	}
	
	for (h = 1; h < bbinfo->size; h++) {
		if (!TESTBIT(bb->dfchildren, h))
			continue;
		
		renamevar(p2e,bbinfo->arr[h], bbinfo);
	}
	
	SLIST_FOREACH(stacke,&poplist,varstackelem) {
		tmpregno=stacke->tmpregno;
		
		defsites.stack[tmpregno].q_forw=defsites.stack[tmpregno].q_forw->varstackelem.q_forw;
	}
}

enum pred_type {
    pred_unknown    = 0,
    pred_goto       = 1,
    pred_cond       = 2,
    pred_falltrough = 3,
} ;

void
removephi(struct p2env *p2e, struct labelinfo *labinfo,struct bblockinfo *bbinfo)
{
	struct basicblock *bb,*bbparent;
	struct cfgnode *cfgn;
	struct phiinfo *phi;
	int i;
	struct interpass *ip;
	struct interpass *pip;
	TWORD n_type;

	enum pred_type complex = pred_unknown ;

	int label=0;
	int newlabel;
	
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {		
		SLIST_FOREACH(phi,&bb->phi,phielem) {
			/* Look at only one, notice break at end */
			i=0;
			
			SLIST_FOREACH(cfgn, &bb->parents, cfgelem) { 
				bbparent=cfgn->bblock;
				
				pip=bbparent->last;
				
				complex = pred_unknown ;
				
				BDEBUG(("removephi: %p in %d",pip,bb->dfnum));

				if (pip->type == IP_NODE && pip->ip_node->n_op == GOTO) {
					BDEBUG((" GOTO "));
					label = (int)pip->ip_node->n_left->n_lval;
					complex = pred_goto ;
				} else if (pip->type == IP_NODE && pip->ip_node->n_op == CBRANCH) {
					BDEBUG((" CBRANCH "));
					label = (int)pip->ip_node->n_right->n_lval;
					
					if (bb==labinfo->arr[label - p2e->ipp->ip_lblnum])
						complex = pred_cond ;
                                        else
                                                complex = pred_falltrough ;

		        		} else if (DLIST_PREV(bb, bbelem) == bbparent) {
                                                complex = pred_falltrough ;
                                        } else {
                                            /* PANIC */
                                            comperr("Assumption blown in rem-phi") ;
                                        }
       
        				BDEBUG((" Complex: %d\n",complex)) ;

	        			switch (complex) {
	        			  case pred_goto:
	        				/* gotos can only go to this place. No bounce tab needed */
	        				SLIST_FOREACH(phi,&bb->phi,phielem) {
	        					if (phi->intmpregno[i]>0) {
	        						n_type=phi->n_type;
	        						ip = ipnode(mkbinode(ASSIGN,
								     mktemp(phi->newtmpregno, n_type),
								     mktemp(phi->intmpregno[i],n_type),
								     n_type));
					
	        						DLIST_INSERT_BEFORE((bbparent->last), ip, qelem);
	        					}
	        				}
                                            break ;
	        			  case pred_cond:
	        				/* Here, we need a jump pad */
	        				newlabel=getlab2();
				
	        				ip = tmpalloc(sizeof(struct interpass));
	        				ip->type = IP_DEFLAB;
	        				/* Line number?? ip->lineno; */
	        				ip->ip_lbl = newlabel;
	        				DLIST_INSERT_BEFORE((bb->first), ip, qelem);


	        				SLIST_FOREACH(phi,&bb->phi,phielem) {
	        					if (phi->intmpregno[i]>0) {
	        						n_type=phi->n_type;
	        						ip = ipnode(mkbinode(ASSIGN,
								     mktemp(phi->newtmpregno, n_type),
								     mktemp(phi->intmpregno[i],n_type),
								     n_type));
					
	        						DLIST_INSERT_BEFORE((bb->first), ip, qelem);
	        					}
	        				}
	        				/* add a jump to us */
	        				ip = ipnode(mkunode(GOTO, mklnode(ICON, label, 0, INT), 0, INT));
	        				DLIST_INSERT_BEFORE((bb->first), ip, qelem);
	        				pip->ip_node->n_right->n_lval=newlabel;
                                            break ;
	        			  case pred_falltrough:
                                		if (bb->first->type == IP_DEFLAB) { 
                                                    label = bb->first->ip_lbl; 
                                                    BDEBUG(("falltrough label %d\n", label));
                                                } else {
                                                    comperr("BBlock has no label?") ;
                                                }

	                                        /* 
                                                 * add a jump to us. We _will_ be, or already have, added code in between.
                                                 * The code is created in the wrong order and switched at the insert, thus
                                                 * comming out correctly
                                                 */

                        	                ip = ipnode(mkunode(GOTO, mklnode(ICON, label, 0, INT), 0, INT));
	                                        DLIST_INSERT_AFTER((bbparent->last), ip, qelem);

                	                        /* Add the code to the end, add a jump to us. */
	                                        SLIST_FOREACH(phi,&bb->phi,phielem) {
        		                                if (phi->intmpregno[i]>0) {
        			                                n_type=phi->n_type;
                			                        ip = ipnode(mkbinode(ASSIGN,
        				                                mktemp(phi->newtmpregno, n_type),
        				                                mktemp(phi->intmpregno[i],n_type),
        				                                n_type));
	
        			                                DLIST_INSERT_AFTER((bbparent->last), ip, qelem);
        		                                }
                	                        }
                                            break ;
        				  default:
                                            comperr("assumption blown, complex is %d\n", complex) ;
                                }

				i++;
			}
			break;
		}
	}
}

    
/*
 * Remove unreachable nodes in the CFG.
 */ 

void
remunreach(struct p2env *p2e)
{
	struct basicblock *bb, *nbb;
	struct interpass *next, *ctree;

	bb = DLIST_NEXT(&p2e->bblocks, bbelem);
	while (bb != &p2e->bblocks) {
		nbb = DLIST_NEXT(bb, bbelem);

		/* Code with dfnum 0 is unreachable */
		if (bb->dfnum != 0) {
			bb = nbb;
			continue;
		}

		/* Need the epilogue node for other parts of the
		   compiler, set its label to 0 and backend will
		   handle it. */ 
		if (bb->first->type == IP_EPILOG) {
			bb->first->ip_lbl = 0;
			bb = nbb;
			continue;
		}

		next = bb->first;
		do {
			ctree = next;
			next = DLIST_NEXT(ctree, qelem);
			
			if (ctree->type == IP_NODE)
				tfree(ctree->ip_node);
			DLIST_REMOVE(ctree, qelem);
		} while (ctree != bb->last);
			
		DLIST_REMOVE(bb, bbelem);
		bb = nbb;
	}
}

void
printip(struct interpass *pole)
{
	static char *foo[] = {
	   0, "NODE", "PROLOG", "STKOFF", "EPILOG", "DEFLAB", "DEFNAM", "ASM" };
	struct interpass *ip;
	struct interpass_prolog *ipplg, *epplg;
	unsigned i;

	DLIST_FOREACH(ip, pole, qelem) {
		if (ip->type > MAXIP)
			printf("IP(%d) (%p): ", ip->type, ip);
		else
			printf("%s (%p): ", foo[ip->type], ip);
		switch (ip->type) {
		case IP_NODE: printf("\n");
#ifdef PCC_DEBUG
			fwalk(ip->ip_node, e2print, 0); break;
#endif
		case IP_PROLOG:
			ipplg = (struct interpass_prolog *)ip;
			printf("%s %s regs",
			    ipplg->ipp_name, ipplg->ipp_vis ? "(local)" : "");
			for (i = 0; i < NIPPREGS; i++)
				printf("%s0x%lx", i? ":" : " ",
				    (long) ipplg->ipp_regs[i]);
			printf(" autos %d mintemp %d minlbl %d\n",
			    ipplg->ipp_autos, ipplg->ip_tmpnum, ipplg->ip_lblnum);
			break;
		case IP_EPILOG:
			epplg = (struct interpass_prolog *)ip;
			printf("%s %s regs",
			    epplg->ipp_name, epplg->ipp_vis ? "(local)" : "");
			for (i = 0; i < NIPPREGS; i++)
				printf("%s0x%lx", i? ":" : " ",
				    (long) epplg->ipp_regs[i]);
			printf(" autos %d mintemp %d minlbl %d\n",
			    epplg->ipp_autos, epplg->ip_tmpnum, epplg->ip_lblnum);
			break;
		case IP_DEFLAB: printf(LABFMT "\n", ip->ip_lbl); break;
		case IP_DEFNAM: printf("\n"); break;
		case IP_ASM: printf("%s\n", ip->ip_asm); break;
		default:
			break;
		}
	}
}

#ifdef PCC_DEBUG
void flownodeprint(NODE *p,FILE *flowdiagramfile);

void
flownodeprint(NODE *p,FILE *flowdiagramfile)
{	
	int opty;
	char *o;

	fprintf(flowdiagramfile,"{");

	o=opst[p->n_op];
	
	while (*o != 0) {
		if (*o=='<' || *o=='>')
			fputc('\\',flowdiagramfile);
		
		fputc(*o,flowdiagramfile);
		o++;
	}
	
	
	switch( p->n_op ) {			
		case REG:
			fprintf(flowdiagramfile, " %s", rnames[p->n_rval] );
			break;
			
		case TEMP:
			fprintf(flowdiagramfile, " %d", regno(p));
			break;
			
		case XASM:
		case XARG:
			fprintf(flowdiagramfile, " '%s'", p->n_name);
			break;
			
		case ICON:
		case NAME:
		case OREG:
			fprintf(flowdiagramfile, " " );
			adrput(flowdiagramfile, p );
			break;
			
		case STCALL:
		case USTCALL:
		case STARG:
		case STASG:
			fprintf(flowdiagramfile, " size=%d", p->n_stsize );
			fprintf(flowdiagramfile, " align=%d", p->n_stalign );
			break;
	}
	
	opty = optype(p->n_op);
	
	if (opty != LTYPE) {
		fprintf(flowdiagramfile,"| {");
	
		flownodeprint(p->n_left,flowdiagramfile);
	
		if (opty == BITYPE) {
			fprintf(flowdiagramfile,"|");
			flownodeprint(p->n_right,flowdiagramfile);
		}
		fprintf(flowdiagramfile,"}");
	}
	
	fprintf(flowdiagramfile,"}");
}

void printflowdiagram(struct p2env *p2e,struct labelinfo *labinfo,struct bblockinfo *bbinfo,char *type) {
	struct basicblock *bbb;
	struct cfgnode *ccnode;
	struct interpass *ip;
	struct interpass_prolog *plg;
	struct phiinfo *phi;
	char *name;
	char *filename;
	int filenamesize;
	char *ext=".dot";
	FILE *flowdiagramfile;
	
	if (!g2debug)
		return;
	
	bbb=DLIST_NEXT(&p2e->bblocks, bbelem);
	ip=bbb->first;

	if (ip->type != IP_PROLOG)
		return;
	plg = (struct interpass_prolog *)ip;

	name=plg->ipp_name;
	
	filenamesize=strlen(name)+1+strlen(type)+strlen(ext)+1;
	filename=tmpalloc(filenamesize);
	snprintf(filename,filenamesize,"%s-%s%s",name,type,ext);
	
	flowdiagramfile=fopen(filename,"w");
	
	fprintf(flowdiagramfile,"digraph {\n");
	fprintf(flowdiagramfile,"rankdir=LR\n");
	
	DLIST_FOREACH(bbb, &p2e->bblocks, bbelem) {
		ip=bbb->first;
		
		fprintf(flowdiagramfile,"bb%p [shape=record ",bbb);
		
		if (ip->type==IP_PROLOG)
			fprintf(flowdiagramfile,"root ");

		fprintf(flowdiagramfile,"label=\"");
		
		SLIST_FOREACH(phi,&bbb->phi,phielem) {
			fprintf(flowdiagramfile,"Phi %d|",phi->tmpregno);
		}		
		
		
		while (1) {
			switch (ip->type) {
				case IP_NODE: 
					flownodeprint(ip->ip_node,flowdiagramfile);
					break;
					
				case IP_DEFLAB: 
					fprintf(flowdiagramfile,"Label: %d", ip->ip_lbl);
					break;
					
				case IP_PROLOG:
					plg = (struct interpass_prolog *)ip;
	
					fprintf(flowdiagramfile,"%s %s",plg->ipp_name,type);
					break;
			}
			
			fprintf(flowdiagramfile,"|");
			fprintf(flowdiagramfile,"|");
			
			if (ip==bbb->last)
				break;
			
			ip = DLIST_NEXT(ip, qelem);
		}
		fprintf(flowdiagramfile,"\"]\n");
		
		SLIST_FOREACH(ccnode, &bbb->children, cfgelem) {
			char *color="black";
			struct interpass *pip=bbb->last;

			if (pip->type == IP_NODE && pip->ip_node->n_op == CBRANCH) {
				int label = (int)pip->ip_node->n_right->n_lval;
				
				if (ccnode->bblock==labinfo->arr[label - p2e->ipp->ip_lblnum])
					color="red";
			}
			
			fprintf(flowdiagramfile,"bb%p -> bb%p [color=%s]\n", bbb,ccnode->bblock,color);
		}
	}
	
	fprintf(flowdiagramfile,"}\n");
	fclose(flowdiagramfile);
}

#endif

/* walk all the programm */
void WalkAll(struct p2env* p2e, void (*f) (NODE*, void*), void* arg, int type)
{
	struct interpass *ipole = &p2e->ipole;
	struct interpass *ip ;
        if (0 != type) {
        	DLIST_FOREACH(ip, ipole, qelem) {
	        	if (ip->type == IP_NODE && ip->ip_node->n_op == type)
                                walkf(ip->ip_node, f, arg) ;
                }
        } else {
        	DLIST_FOREACH(ip, ipole, qelem) {
			if (ip->type == IP_NODE)
	                        walkf(ip->ip_node, f, arg) ;
                }
        }
}
#if 0
static int is_goto_label(struct interpass*g, struct interpass* l)
{
	if (!g || !l)
		return 0 ;
	if (g->type != IP_NODE) return 0 ;
	if (l->type != IP_DEFLAB) return 0 ;
	if (g->ip_node->n_op != GOTO) return 0 ;
	if (g->ip_node->n_left->n_lval != l->ip_lbl) return 0 ;
	return 1 ;
}
#endif

/*
 * iterate over the basic blocks. 
 * In case a block has only one successor and that one has only one pred, and the link is a goto:
 * place the second one immediately behind the first one (in terms of nodes, means cut&resplice). 
 * This should take care of a lot of jumps.
 * one problem: Cannot move the first or last basic block (prolog/epilog). This is taken care of by
 * moving block #1 to #2, not #2 to #1
 * More complex (on the back cooker;) : first coalesc the inner loops (L1 cache), then go from inside out.
 */

static unsigned long count_blocks(struct p2env* p2e)
{
	struct basicblock* bb ;
	unsigned long count = 0 ;
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		++count ;
	}
	return count ;
}

struct block_map {
	struct basicblock* block ;
	unsigned long index ;
	unsigned long thread ;
} ;

static unsigned long map_blocks(struct p2env* p2e, struct block_map* map, unsigned long count)
{
	struct basicblock* bb ;
	unsigned long indx = 0 ;
	int ignore = 2 ;
	unsigned long thread ;
	unsigned long changes ;

	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		map[indx].block = bb ;
		map[indx].index = indx ;

		/* ignore the first 2 labels, maybe up to 3 BBs */
		if (ignore) {
			if (bb->first->type == IP_DEFLAB) 
				--ignore;

			map[indx].thread = 1 ;	/* these are "fixed" */
		} else
			map[indx].thread = 0 ;

		indx++ ;
	}

	thread = 1 ;
	do {
		changes = 0 ;
		
		for (indx=0; indx < count; indx++) {
			/* find block without trace */
			if (map[indx].thread == 0) {
				/* do new thread */
				struct cfgnode *cnode ;
				struct basicblock *block2 = 0;
				unsigned long i ;
				unsigned long added ;
				
				BDEBUG (("new thread %ld at block %ld\n", thread, indx)) ;

				bb = map[indx].block ;
				do {
					added = 0 ;

					for (i=0; i < count; i++) {
						if (map[i].block == bb && map[i].thread == 0) {
							map[i].thread = thread ;

							BDEBUG(("adding block %ld to trace %ld\n", i, thread)) ;

							changes ++ ;
							added++ ;

							/* 
							 * pick one from followers. For now (simple), pick the 
							 * one who is directly following us. If none of that exists,
							 * this code picks the last one.
							 */

							SLIST_FOREACH(cnode, &bb->children, cfgelem) {
								block2=cnode->bblock ;
#if 1
								if (i+1 < count && map[i+1].block == block2)
									break ;	/* pick that one */
#else
								if (block2) break ;
#endif
							}

							if (block2)
								bb = block2 ;
						}
					}
				} while (added) ;
				thread++ ;
			}
		}
	} while (changes);

	/* Last block is also a thread on it's own, and the highest one. */
/*
	thread++ ;
	map[count-1].thread = thread ;
*/
	if (b2debug) {
		printf("Threads\n");
		for (indx=0; indx < count; indx++) {
			printf("Block #%ld (lbl %d) Thread %ld\n", indx, map[indx].block->first->ip_lbl, map[indx].thread);
		}
	}
	return thread ;
}


void TraceSchedule(struct p2env* p2e)
{
	struct block_map* map ;
	unsigned long block_count = count_blocks(p2e);
	unsigned long i ;
	unsigned long threads;
	struct interpass *front, *back ;

	map = tmpalloc(block_count * sizeof(struct block_map));

	threads = map_blocks(p2e, map, block_count) ;

	back = map[0].block->last ;
	for (i=1; i < block_count; i++) {
		/* collect the trace */
		unsigned long j ;
		unsigned long thread = map[i].thread ;
		if (thread) {
			BDEBUG(("Thread %ld\n", thread)) ;
			for (j=i; j < block_count; j++) {
				if (map[j].thread == thread) {
					front = map[j].block->first ;

					BDEBUG(("Trace %ld, old BB %ld, next BB %ld\t",
						thread, i, j)) ;
					BDEBUG(("Label %d\n", front->ip_lbl)) ;
					DLIST_NEXT(back, qelem) = front ;
					DLIST_PREV(front, qelem) = back ;
					map[j].thread = 0 ;	/* done */
					back = map[j].block->last ;
					DLIST_NEXT(back, qelem) = 0 ;
				}
			}
		}
	}
	DLIST_NEXT(back, qelem) = &(p2e->ipole) ;
	DLIST_PREV(&p2e->ipole, qelem) = back ;
}

static void add_labels(struct p2env* p2e)
{
	struct interpass *ipole = &p2e->ipole ;
	struct interpass *ip ;

	DLIST_FOREACH(ip, ipole, qelem) {
        	if (ip->type == IP_NODE && ip->ip_node->n_op == CBRANCH) {
			struct interpass *n = DLIST_NEXT(ip, qelem);
			if (n && n->type != IP_DEFLAB) {
				struct interpass* lab;
				int newlabel=getlab2() ;

				BDEBUG(("add_label L%d\n", newlabel));

				lab = tmpalloc(sizeof(struct interpass));
				lab->type = IP_DEFLAB;
				/* Line number?? ip->lineno; */
				lab->ip_lbl = newlabel;
				DLIST_INSERT_AFTER(ip, lab, qelem);
			}
		}
	}
}

/* node map */
#ifdef ENABLE_NEW
struct node_map {
	NODE* node ;		/* the node */
	unsigned int node_num ;	/* node is equal to that one */
	unsigned int var_num ;	/* node computes this variable */
} ;

static unsigned long nodes_counter ;
static void node_map_count_walker(NODE* n, void* x)
{
	nodes_counter ++ ;
}

static void do_cse(struct p2env* p2e)
{
	nodes_counter = 0 ;
	WalkAll(p2e, node_map_count_walker, 0, 0) ;
	BDEBUG(("Found %ld nodes\n", nodes_counter)) ;
}
#endif

